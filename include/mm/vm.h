#ifndef MM_VM_H
#define MM_VM_H

#include "riscv.h"

void kvminit(void);
void kvminithart(void);

bool vmmap(const pagetable_t root, const vaddr_t va, const u64 size,
	   const paddr_t pa, const u8 perm);
pagetable_t uvmcreate(void);
void uvmmapkernel(pagetable_t uroot);
void uvmfree(pagetable_t root, u64 size, u64 stack_bottom, u64 stack_top);
bool copyout(pagetable_t pagetable, u64 dstva, const char *src, u64 len);

#endif
