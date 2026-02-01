#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <stddef.h>
#include <stdint.h>

#define SYSCALL_WRITE 64
#define SYSCALL_EXIT 93
#define SYSCALL_YIELD 124
#define SYSCALL_GET_TIME 169

// syscall.c
int64_t syscall(uint64_t syscall_id, uint64_t args[]);

// Include specific syscall modules
#include "sys_fs.h"
#include "sys_process.h"

#endif // __SYSCALL_H__
