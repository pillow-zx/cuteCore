#include <stdint.h>

#include "defs.h"
#include "log.h"
#include "syscall.h"

int64_t syscall(uint64_t syscall_id, uint64_t args[]) {
    int64_t ret = -1;
    switch (syscall_id) {
        case SYSCALL_WRITE: {
            return sys_write(args[0], (const char *)args[1], args[2]);
        }
        case SYSCALL_EXIT: {
            sys_exit((int64_t)args[0]);
        }
        default: {
            panic("Unknown syscall id: %lu", syscall_id);
            shutdown();
        }
    }

    return ret;
}
