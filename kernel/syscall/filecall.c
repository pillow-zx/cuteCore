#include "config.h"
#include "fs.h"
#include "kernel/task.h"
#include "syscall.h"
#include "types.h"

static bool argfd(const i32 n, i32 *pfd, struct file **file)
{
	i32 fd;
	struct file *f;

	argint(n, &fd);
	if (fd < 0 || fd >= NOFILE || (f = curproc()->fdtable[fd]) == nullptr)
		return false;
	if (pfd)
		*pfd = fd;
	if (file)
		*file = f;
	return true;
}

i64 sys_read(void)
{
	struct file *f;
	i32 n;
	u64 p;
	argaddr(1, &p);
	argint(2, &n);
	if (!argfd(0, nullptr, &f))
		return -1;

	return fileread(f, (void *)p, n);
}

i64 sys_write(void)
{
	struct file *f;
	i32 n;
	u64 p;
	argaddr(1, &p);
	argint(2, &n);
	if (!argfd(0, nullptr, &f))
		return -1;
	return filewrite(f, (void *)p, n);
}
