#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/trap.h"
#include "mm/kalloc.h"
#include "mm/vm.h"
#include "proc.h"
#include "config.h"
#include "log.h"
#include "riscv.h"
#include "utils.h"

struct cache *proc_cache;

struct proc *ready_list_head = nullptr;
struct proc *ready_list_tail = nullptr;

struct cpu cpus[NCPU];

unsigned char pid_bitmap[PIDBITMAP];
static i32 last_pid = 0;

__always_inline void setpid(const i32 pid)
{
	pid_bitmap[pid / 8] |= 1 << (pid % 8);
}

__always_inline void freepid(const i32 pid)
{
	pid_bitmap[pid / 8] &= ~(1 << (pid % 8));
}

__always_inline bool checkpid(const i32 pid)
{
	return pid_bitmap[pid / 8] & 1 << (pid % 8);
}

static i32 allocpid(void)
{
	i32 pid = last_pid;

	for (i32 i = 0; i < MAXPID; i++) {
		pid++;
		if (pid >= MAXPID) {
			pid = RESERVEDPID;
		}

		if (!checkpid(pid)) {
			setpid(pid);
			last_pid = pid;
			return pid;
		}
	}

	return -1;
}

void procinit(void)
{
	proc_cache = cache_create(sizeof(struct proc));
	Log("Process cache init success");
}

void forkret(void)
{
	struct proc *proc = curproc();
	static bool init = false;

	if (!init) {
		init = true;

		proc->trapframe->a0 =
			kexec("/init", (char *[]){"/init", nullptr});
		if ((i64)proc->trapframe->a0 == -1)
			panic("exec");
	}

	trapret();
}

struct proc *allocproc(void)
{
	struct proc *proc = cache_alloc(proc_cache);
	if (proc == nullptr)
		return nullptr;

	proc->pid = allocpid();
	proc->state = SLEEPING;
	proc->sz = 0;
	proc->brk = 0;
	proc->stack_bottom = 0;
	proc->stack_top = 0;

	if ((proc->kstack = (u64)kalloc()) == 0) {
		freeproc(proc);
		return nullptr;
	}
	proc->trapframe = (struct trapframe *)(proc->kstack + PGSIZE -
					       sizeof(struct trapframe));

	proc->root = mapproc();
	if (proc->root == nullptr) {
		freeproc(proc);
		return nullptr;
	}

	memset(&proc->context, 0, sizeof(proc->context));
	proc->context.sp = proc->kstack + PGSIZE;
	proc->context.ra = (u64)forkret;

	return proc;
}

void freeproc(struct proc *p)
{
	if (p == nullptr)
		return;

	uvmfree(p->root, p->sz, p->stack_bottom, p->stack_top);
	p->root = nullptr;
	p->stack_bottom = 0;
	p->stack_top = 0;

	if (p->kstack != 0) {
		kfree((void *)p->kstack);
		p->kstack = 0;
	}

	if (p->pid > RESERVEDPID) {
		freepid(p->pid);
		p->pid = 0;
	}

	p->state = SLEEPING;
	p->sz = 0;
	p->trapframe = nullptr;
	p->parent = nullptr;
	p->name[0] = '\0';

	cache_free(proc_cache, p);
}

pagetable_t mapproc(void)
{
	pagetable_t root = uvmcreate();
	if (root == nullptr)
		return nullptr;

	uvmmapkernel(root);
	return root;
}

static int cpuid(void)
{
	const i32 id = (i32)r_tp();
	return id;
}

struct cpu *curcpu(void)
{
	const i32 id = cpuid();
	struct cpu *c = &cpus[id];
	return c;
}

struct proc *curproc(void)
{
	const struct cpu *c = curcpu();
	struct proc *p = c->proc;
	return p;
}
