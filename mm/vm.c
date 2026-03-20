/**
 * @file vm.c
 * @brief 虚拟内存管理
 *
 * 本文件实现了 RISC-V Sv39 虚拟内存管理，包括内核页表初始化、
 * 用户地址空间创建与销毁、页表遍历与映射等核心功能。
 *
 * 内存布局（无 KPTI）：
 * - 内核空间（高地址）：所有进程共享相同的内核映射
 * - 用户空间（低地址）：每个进程独立的地址空间
 */

#include "mm/kalloc.h"
#include "mm/vm.h"
#include "config.h"
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

/** @brief 内核页表根节点 */
pagetable_t kvmroot;

/**
 * @brief 遍历页表获取 PTE
 *
 * 从页表根节点开始，逐级查找虚拟地址 va 对应的页表项（PTE）。
 * Sv39 使用三级页表，level 从 2 到 0。
 *
 * @param root  页表根节点地址
 * @param va    要查找的虚拟地址
 * @param alloc 若为 true，缺失的中间页表将被分配；若为 false，缺失时返回 nullptr
 *
 * @return 成功返回 PTE 指针；失败返回 nullptr
 *
 * @note 返回的是最底层（level 0）的 PTE，可直接用于设置物理页映射
 */
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

/**
 * @brief 映射虚拟地址区间到物理地址区间
 *
 * 建立从虚拟地址 [va, va+size) 到物理地址 [pa, pa+size) 的映射，
 * 按 4KB 页对齐逐页设置 PTE。
 *
 * @param root 页表根节点
 * @param va   虚拟地址起始（必须页对齐）
 * @param size 映射大小（必须页对齐且非零）
 * @param pa   物理地址起始
 * @param perm 页表项权限标志（PTE_R | PTE_W | PTE_X | PTE_U 等）
 *
 * @return 成功返回 true；内存分配失败返回 false
 *
 * @note 若地址未对齐或检测到重复映射，将触发 panic
 */
bool vmmap(const pagetable_t root, const vaddr_t va, const u64 size,
	   const paddr_t pa, const u8 perm)
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

/**
 * @brief 内核地址空间映射
 *
 * 封装 vmmap()，用于内核地址空间映射。映射失败时触发 panic，
 * 因为内核初始化阶段不应失败。
 *
 * @param kroot 内核页表根节点
 * @param va    虚拟地址起始
 * @param pa    物理地址起始
 * @param size  映射大小
 * @param perm  权限标志
 */
static void kvmmap(const pagetable_t kroot, const vaddr_t va, const paddr_t pa,
		   const u64 size, const u8 perm)
{
	if (!vmmap(kroot, va, size, pa, perm)) {
		panic("kvmmap");
	}
}

/**
 * @brief 构建内核页表
 *
 * 创建并初始化内核页表，建立以下映射（恒等映射）：
 * - .text   段：只读 + 可执行
 * - .rodata 段：只读
 * - .data/.bss 段：可读 + 可写
 * - 物理内存剩余部分：可读 + 可写
 * - MMIO 设备（UART、VirtIO、SIFIVE_TEST）：可读 + 可写
 *
 * @return 成功返回页表根节点；内存分配失败时触发 panic
 */
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

/**
 * @brief 在当前 CPU 上启用内核页表
 *
 * 刷新 TLB，设置 satp 寄存器为内核页表，再次刷新 TLB。
 * 每个 CPU 在启动时调用一次。
 *
 * @note 调用前必须已完成 kvminit()
 */
void kvminithart(void)
{
	sfence_vma();
	w_csr(satp, MAKESATP(kvmroot));
	sfence_vma();
	Log("Pagetable init success");
}

/**
 * @brief 初始化内核虚拟内存
 *
 * 构建内核页表并存储到全局变量 kvmroot。
 * 在内核启动早期调用，在使用任何动态内存分配之前。
 */
void kvminit(void)
{
	kvmroot = kvmmake();
	Log("Kernel vm init success");
}

/**
 * @brief 检查虚拟地址是否已映射
 *
 * @param root 页表根节点
 * @param va   要检查的虚拟地址
 *
 * @return 已映射返回 true；未映射返回 false
 */
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

/**
 * @brief 递归释放页表
 *
 * 释放整个页表树结构，包括所有中间页表和叶子页表。
 * 要求叶子页表中的所有映射已清除，否则触发 panic。
 *
 * @param root 页表根节点
 *
 * @warning 此函数用于释放纯页表结构，不释放映射的物理页。
 *          若存在有效叶子映射，将触发 panic。
 */
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

/**
 * @brief 释放用户页表（保留内核映射部分）
 *
 * 专用于用户进程退出时释放页表。内核映射部分（高地址）
 * 仅清除引用而不递归释放，因为内核页表由全局管理。
 *
 * @param root 用户页表根节点
 *
 * @note 用户空间的物理页应在此之前通过 uvmunmap 释放
 */
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

/**
 * @brief 创建空白用户页表
 *
 * 分配一个页作为用户页表根节点，不包含任何映射。
 * 调用者应随后调用 uvmmapkernel() 复制内核映射。
 *
 * @return 成功返回页表根节点；内存分配失败返回 nullptr
 */
pagetable_t uvmcreate(void)
{
	const auto uroot = (pagetable_t)kalloc();
	if (uroot == nullptr) {
		return nullptr;
	}
	return uroot;
}

/**
 * @brief 将内核页表映射复制到用户页表
 *
 * 将全局内核页表（kvmroot）的顶层 PTE 复制到用户页表，
 * 使每个用户进程都能访问内核空间。
 *
 * 这是无 KPTI（Kernel Page Table Isolation）设计的关键：
 * 用户进程切换时无需切换到独立的内核页表。
 *
 * @param uroot 用户页表根节点
 *
 * @note 复制的是 PTE 值而非页表结构，所有进程共享同一份内核页表
 */
void uvmmapkernel(pagetable_t uroot)
{
	for (i32 i = 0; i < 512; i++) {
		pte_t kpte = kvmroot[i];
		if (kpte & PTE_V) {
			uroot[i] = kpte;
		}
	}
}

/**
 * @brief 取消用户地址空间的页映射
 *
 * 从指定虚拟地址开始，取消 pagenum 个页的映射，
 * 可选释放对应的物理页。
 *
 * @param root    页表根节点
 * @param va      起始虚拟地址（必须页对齐）
 * @param pagenum 要取消映射的页数
 * @param free    若为 true，同时释放映射的物理页
 */
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

/**
 * @brief 缩小用户地址空间
 *
 * 将用户进程的内存占用从 oldsz 缩小到 newsz，
 * 释放被截断部分的物理页并取消映射。
 *
 * @param root  页表根节点
 * @param oldsz 当前大小
 * @param newsz 目标大小
 *
 * @return 实际缩小后的大小（newsz）
 */
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

/**
 * @brief 扩展用户地址空间
 *
 * 将用户进程的内存占用从 oldsz 扩展到 newsz，
 * 分配新的物理页并建立映射。
 *
 * @param root  页表根节点
 * @param oldsz 当前大小
 * @param newsz 目标大小
 * @param perm  新页的权限标志
 *
 * @return 成功返回 newsz；内存分配失败返回 0
 *
 * @note 若分配失败，会自动回滚已分配的页
 */
u64 uvmalloc(const pagetable_t root, u64 oldsz, const u64 newsz, const u8 perm)
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

/**
 * @brief 释放整个用户地址空间
 *
 * 先释放用户空间的物理页（从地址 0 到 sz），
 * 再释放页表结构本身。
 *
 * @param root 页表根节点
 * @param sz   用户空间大小
 */
void uvmfree(const pagetable_t root, const u64 sz, const u64 stack_bottom,
	     const u64 stack_top)
{
	if (sz > 0)
		uvmunmap(root, 0, PGROUNDUP(sz) / PGSIZE, true);
	if (stack_top > stack_bottom) {
		uvmunmap(root, stack_bottom,
			 (stack_top - stack_bottom) / PGSIZE, true);
	}
	uvmfreewalk(root);
}

/**
 * @brief 从内核空间拷贝数据到用户空间
 *
 * 通过页表遍历获取用户虚拟地址对应的物理地址，然后利用内核的恒等映射
 * 直接写入物理地址。这避免了切换页表的开销。
 *
 * @param pagetable 用户进程页表
 * @param dstva     目标用户虚拟地址
 * @param src       源内核地址
 * @param len       拷贝长度（字节）
 *
 * @return 成功返回 0；失败返回 -1
 *
 * @note 要求用户地址已经通过 vmmap 映射，否则返回失败
 */
bool copyout(pagetable_t pagetable, u64 dstva, const char *src, u64 len)
{
	while (len > 0) {
		/* 计算页对齐的虚拟地址 */
		vaddr_t va = ALIGNDOWN(dstva, PGSIZE);

		/* 通过用户页表查找物理地址 */
		pte_t *pte = walk(pagetable, va, false);
		if (pte == nullptr || (*pte & PTE_V) == 0 ||
		    (*pte & PTE_U) == 0) {
			return false;
		}

		paddr_t pa = PTE2PA(*pte);

		/* 计算页内偏移和本次拷贝大小 */
		u64 offset = dstva - va;
		u64 n = PGSIZE - offset;
		if (n > len)
			n = len;

		/* 写入物理地址（内核恒等映射可直接访问） */
		memcpy((void *)(pa + offset), src, n);

		/* 更新指针 */
		len -= n;
		src += n;
		dstva += n;
	}

	return true;
}
