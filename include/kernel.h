#ifndef KERNEL_H
#define KERNEL_H

#include "proc.h"
#include "riscv.h"
#include "utils.h"

void trapinit(void);
void trapret(void);

__format(printf, 1, 2) i32 printf(const char *fmt, ...);

__noreturn void shutdown(void);

void kinit(void);
void kfree(void *pa);
void *kalloc(void);

void kvminit(void);
void kvminithart(void);
pagetable_t uvmcreate(void);
void uvmmapkernel(pagetable_t uroot);
void uvmfree(pagetable_t root, const u64 size);

void procinit(void);
struct cpu *curcpu(void);
struct proc *curproc(void);

void scheduler(void);

void swtch(struct context *old, struct context *new);

void syscall(void);

i32 sys_exec(char *path, char **argv);

#endif
