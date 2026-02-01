#include <stddef.h>
#include <stdint.h>

#include "kernel/common.h"
#include "log.h"
#include "syscall/syscall.h"

int64_t syscall(size_t syscall_id, size_t args[]) {
    int64_t ret = -1;
    switch (syscall_id) {
        case SYSCALL_WRITE: {
            return sys_write(args[0], (const char *)args[1], args[2]);
        }
        case SYSCALL_EXIT: {
            sys_exit((int64_t)args[0]);
        }
        case SYSCALL_YIELD: {
            sys_yield();
        }
        case SYSCALL_GET_TIME: {
            return sys_get_time();
        }
        default: {
            panic("Unknown syscall id: %lu", syscall_id);
            shutdown();
        }
    }

    return ret;
}
