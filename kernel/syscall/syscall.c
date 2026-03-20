#include "kernel/syscall.h"
#include "kernel/task.h"
#include "syscall.h"
#include "log.h"
#include "utils.h"

static i64 argraw(i32 no)
{
	const struct proc *p = curproc();
	switch (no) {
		case 0:
			return p->trapframe->a0;
		case 1:
			return p->trapframe->a1;
		case 2:
			return p->trapframe->a2;
		case 3:
			return p->trapframe->a3;
		case 4:
			return p->trapframe->a4;
		case 5:
			return p->trapframe->a5;
		default:
			return -1;
	}
}

void argint(const i32 n, i32 *ip)
{
	*ip = (int)argraw(n);
}

void argaddr(const i32 n, u64 *ip)
{
	*ip = argraw(n);
}

extern i64 sys_execve(void);
extern i64 sys_read(void);
extern i64 sys_write(void);

static i64 (*syscalls[])(void) = {
	[SYS_DUP] = nullptr,	   [SYS_DUP3] = nullptr,
	[SYS_UMOUNT2] = nullptr,   [SYS_MOUNT] = nullptr,
	[SYS_OPENAT] = nullptr,	   [SYS_CLOSE] = nullptr,
	[SYS_PIPE2] = nullptr,	   [SYS_READ] = sys_read,
	[SYS_WRITE] = sys_write,   [SYS_EXIT] = nullptr,
	[SYS_WAITTID] = nullptr,   [SYS_KILL] = nullptr,
	[SYS_TKILL] = nullptr,	   [SYS_TGKILL] = nullptr,
	[SYS_UNAME] = nullptr,	   [SYS_GETPID] = nullptr,
	[SYS_GETPPID] = nullptr,   [SYS_GETUID] = nullptr,
	[SYS_BRK] = nullptr,	   [SYS_EXECVE] = sys_execve,
	[SYS_MMAP] = nullptr,	   [SYS_WAIT4] = nullptr,
	[SYS_GETRANDOM] = nullptr,

};

void syscall(void)
{
	struct proc *p = curproc();

	const i32 num = p->trapframe->a7;
	if (num > 0 && num < (i32)NELEM(syscalls) && syscalls[num]) {
		p->trapframe->a0 = syscalls[num]();
	} else {
		Log("%lu %s: unsupported syscall %d\n", p->pid, p->name, num);
		p->trapframe->a0 = -1;
	}
}
