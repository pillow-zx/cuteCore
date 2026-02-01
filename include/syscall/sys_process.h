#ifndef __SYSCALL_PROCESS_H__
#define __SYSCALL_PROCESS_H__

#include "utils/utils.h"
#include <stdint.h>

// sys_process.c
__noreturn void sys_exit(int64_t code);
intptr_t sys_yield();
intptr_t sys_get_time();

#endif // __SYSCALL_PROCESS_H__