#include "kernel.h"
#include "riscv.h"
#include "log.h"

extern char stext[];
extern char etext[];
extern char srodata[];
extern char erodata[];
extern char sdata[];
extern char edata[];
extern char sbss[];
extern char ebss[];
extern char ekernel[];

pagetable_t kvmroot;

static pte_t *walk(const pagetable_t root, const vaddr_t va, const bool alloc)
{
	pagetable_t ptes = root;
	for (i32 level = 2; level > 0; level--) {
		pte_t *pte = &ptes[PX(level, va)];
		if (*pte & PTE_V) {
			ptes = (pagetable_t)PTE2PA(*pte);
		} else {
			if (!alloc || (ptes = kalloc()) == nullptr) {
				return nullptr;
			}
			*pte = PA2PTE(ptes) | PTE_V;
		}
	}

	return &ptes[PX(0, va)];
}

static bool vmmap(const pagetable_t root, const vaddr_t va, const u64 size,
		  const paddr_t pa, const enum Pteflgas perm)
{
	if (va % PGSIZE != 0 || size % PGSIZE != 0 || size == 0) {
		panic("map");
	}

	pte_t *pte = nullptr;

	vaddr_t vaddr = va;
	paddr_t paddr = pa;
	const vaddr_t last = va + size - PGSIZE;
	while (true) {
		if ((pte = walk(root, vaddr, true)) == nullptr) {
			return false;
		}
		if (*pte & PTE_V) {
			panic("map: remap");
		}
		*pte = PA2PTE(paddr) | perm | PTE_V;
		if (vaddr == last) {
			break;
		}
		vaddr += PGSIZE;
		paddr += PGSIZE;
	}

	return true;
}

static void kvmmap(const pagetable_t kroot, const vaddr_t va, const paddr_t pa,
		   const u64 size, const enum Pteflgas perm)
{
	if (!vmmap(kroot, va, size, pa, perm)) {
		panic("kvmmap");
	}
}

static pagetable_t kvmmake(void)
{
	const auto kroot = (pagetable_t)kalloc();

	kvmmap(kroot, (vaddr_t)stext, (paddr_t)stext,
	       (paddr_t)etext - (paddr_t)stext, PTE_R | PTE_X);

	kvmmap(kroot, (vaddr_t)srodata, (paddr_t)srodata,
	       (paddr_t)erodata - (paddr_t)srodata, PTE_R);

	kvmmap(kroot, (vaddr_t)sdata, (paddr_t)sdata,
	       (paddr_t)edata - (paddr_t)sdata, PTE_R | PTE_W);

	kvmmap(kroot, (vaddr_t)sbss, (paddr_t)sbss,
	       (paddr_t)ebss - (paddr_t)sbss, PTE_R | PTE_W);

	kvmmap(kroot, (vaddr_t)ekernel, (paddr_t)ekernel,
	       (paddr_t)PHYEND - (paddr_t)ekernel, PTE_R | PTE_W);

	kvmmap(kroot, (vaddr_t)UART0, (paddr_t)UART0, PGSIZE, PTE_R | PTE_W);

	kvmmap(kroot, (vaddr_t)VIRTIO_BLK0, (paddr_t)VIRTIO_BLK0, PGSIZE,
	       PTE_R | PTE_W);

	kvmmap(kroot, (vaddr_t)SIFIVE_TEST_ADDR, (paddr_t)SIFIVE_TEST_ADDR,
	       PGSIZE, PTE_R | PTE_W);

	return kroot;
}

void kvminithart(void)
{
	sfence_vma();
	w_csr(satp, MAKESATP(kvmroot));
	sfence_vma();
	Log("Pagetable init success");
}

void kvminit(void)
{
	kvmroot = kvmmake();
	Log("Kernel vm init success");
}

bool ismapped(const pagetable_t root, const vaddr_t va)
{
	const pte_t *pte = walk(root, va, false);
	if (pte == nullptr) {
		return false;
	}
	if (*pte & PTE_V) {
		return true;
	}
	return false;
}

void freewalk(const pagetable_t root)
{
	for (i32 i = 0; i < 512; i++) {
		const pte_t pte = root[i];
		if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0) {
			const u64 child = PTE2PA(pte);
			freewalk((pagetable_t)child);
			root[i] = 0;
		} else if (pte & PTE_V) {
			panic("freewalk: leaf");
		}
	}
	kfree((void *)root);
}

static void uvmfreewalk(pagetable_t root)
{
	for (u32 i = 0; i < 512; i++) {
		pte_t pte = root[i];
		if (!(pte & PTE_V))
			continue;
		if (i >= PX(2, KERNBASE)) {
			root[i] = 0;
		} else if ((pte & (PTE_R | PTE_W | PTE_X)) == 0) {
			pagetable_t child = (pagetable_t)PTE2PA(pte);
			freewalk(child);
			root[i] = 0;
		} else {
			panic("uvmfreewalk: leaf in top-level");
		}
	}
	kfree((void *)root);
}

pagetable_t uvmcreate(void)
{
	const auto uroot = (pagetable_t)kalloc();
	if (uroot == nullptr) {
		return nullptr;
	}
	return uroot;
}

void uvmmapkernel(pagetable_t uroot)
{
	for (i32 i = 0; i < 512; i++) {
		pte_t kpte = kvmroot[i];
		if (kpte & PTE_V) {
			uroot[i] = kpte;
		}
	}
}

static void uvmunmap(const pagetable_t root, const vaddr_t va,
		     const u64 pagenum, const bool free)
{
	pte_t *pte = nullptr;

	if (va % PGSIZE != 0) {
		panic("uvmunmap: not aligned");
	}

	for (vaddr_t vaddr = va; vaddr < va + pagenum * PGSIZE;
	     vaddr += PGSIZE) {
		if ((pte = walk(root, vaddr, false)) == nullptr ||
		    (*pte & PTE_V) == 0) {
			continue;
		}
		if (free) {
			const paddr_t paddr = PTE2PA(*pte);
			kfree((void *)paddr);
		}
		*pte = 0;
	}
}

static u64 uvmdealloc(const pagetable_t root, const u64 oldsz, const u64 newsz)
{
	if (newsz >= oldsz) {
		return oldsz;
	}

	if (PGROUNDUP(newsz) < PGROUNDUP(oldsz)) {
		const i32 pagenum =
			(PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
		uvmunmap(root, PGROUNDUP(newsz), pagenum, true);
	}

	return newsz;
}

u64 uvmalloc(const pagetable_t root, u64 oldsz, const u64 newsz,
	     const enum Pteflgas perm)
{
	if (newsz < oldsz) {
		return oldsz;
	}
	oldsz = PGROUNDUP(oldsz);
	for (u64 size = oldsz; size < newsz; size += PGSIZE) {
		char *mem = kalloc();
		if (mem == nullptr) {
			uvmdealloc(root, size, oldsz);
			return 0;
		}
		if (!vmmap(root, size, PGSIZE, (paddr_t)mem,
			   PTE_R | PTE_V | perm)) {
			kfree(mem);
			uvmdealloc(root, size, oldsz);
			return 0;
		}
	}

	return newsz;
}

void uvmfree(const pagetable_t root, const u64 sz)
{
	if (sz > 0)
		uvmunmap(root, 0, PGROUNDUP(sz) / PGSIZE, true);
	uvmfreewalk(root);
}
