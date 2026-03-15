#include "syscall.h"
#include "kernel.h"
#include "log.h"
#include "types.h"
#include "utils.h"

static u64 (*syscalls[])(void) = {
	[SYS_DUP] = nullptr,	   [SYS_DUP3] = nullptr,
	[SYS_UMOUNT2] = nullptr,   [SYS_MOUNT] = nullptr,
	[SYS_OPENAT] = nullptr,	   [SYS_CLOSE] = nullptr,
	[SYS_PIPE2] = nullptr,	   [SYS_READ] = nullptr,
	[SYS_WRITE] = nullptr,	   [SYS_EXIT] = nullptr,
	[SYS_WAITTID] = nullptr,   [SYS_KILL] = nullptr,
	[SYS_TKILL] = nullptr,	   [SYS_TGKILL] = nullptr,
	[SYS_UNAME] = nullptr,	   [SYS_GETPID] = nullptr,
	[SYS_GETPPID] = nullptr,   [SYS_GETUID] = nullptr,
	[SYS_BRK] = nullptr,	   [SYS_EXECVE] = nullptr,
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
