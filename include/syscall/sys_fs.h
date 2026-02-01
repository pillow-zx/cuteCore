#ifndef __SYSCALL_FS_H__
#define __SYSCALL_FS_H__

#include <stdint.h>

// sys_fs.c
int64_t sys_write(const uint64_t fd, const char *buf, uint64_t len);

#endif // __SYSCALL_FS_H__