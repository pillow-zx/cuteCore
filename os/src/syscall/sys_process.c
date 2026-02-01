#include "kernel/common.h"
#include <stdint.h>

void sys_exit(int64_t code) {
    printf("Process exited with code %ld\n", code);
    exit_current_and_run_next();
    while (1);
}

intptr_t sys_yield() {
    suspend_current_and_run_next();
    return 0;
}

intptr_t sys_get_time() { return get_time_ms(); }
