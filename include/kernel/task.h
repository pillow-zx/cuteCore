#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include "proc.h"

void procinit(void);
struct cpu *curcpu(void);
struct proc *curproc(void);
struct proc *allocproc(void);
void freeproc(struct proc *p);
pagetable_t mapproc(void);

void scheduler(void);

void swtch(struct context *old, struct context *new);

#endif
