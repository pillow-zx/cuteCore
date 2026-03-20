#ifndef PROC_H
#define PROC_H

#include "config.h"
#include "fs.h"
#include "riscv.h"

enum procstate : u8 { SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

struct trapframe {
	/* User general-purpose registers */
	u64 ra;
	u64 sp;
	u64 gp;
	u64 tp;
	u64 t0;
	u64 t1;
	u64 t2;
	u64 s0;
	u64 s1;
	u64 a0;
	u64 a1;
	u64 a2;
	u64 a3;
	u64 a4;
	u64 a5;
	u64 a6;
	u64 a7;
	u64 s2;
	u64 s3;
	u64 s4;
	u64 s5;
	u64 s6;
	u64 s7;
	u64 s8;
	u64 s9;
	u64 s10;
	u64 s11;
	u64 t3;
	u64 t4;
	u64 t5;
	u64 t6;

	/* Trap-related CSRs */
	u64 sstatus;
	u64 sepc;
	u64 scause;
	u64 stval;
};

/* Size of trapframe, must match TRAPFRAME_SZ in trapvec.S */
#define TRAPFRAME_SZ sizeof(struct trapframe)

struct context {
	u64 ra;
	u64 sp;

	u64 s0;
	u64 s1;
	u64 s2;
	u64 s3;
	u64 s4;
	u64 s5;
	u64 s6;
	u64 s7;
	u64 s8;
	u64 s9;
	u64 s10;
	u64 s11;
};

struct proc {
	enum procstate state;
	u64 pid;

	pagetable_t root;
	u64 sz;
	u64 brk;
	u64 stack_bottom;
	u64 stack_top;

	u64 kstack;

	struct trapframe *trapframe;

	struct context context;

	struct proc *parent;

	struct file *fdtable[NOFILE];

	struct proc *prev;
	struct proc *next;

	char name[16];
};

struct cpu {
	struct proc *proc;
	struct context context;
};

extern struct proc *ready_list_head;
extern struct proc *ready_list_tail;
extern struct cpu cpus[NCPU];

#endif
