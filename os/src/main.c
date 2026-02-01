#include <stddef.h>
#include <stdint.h>

#include "defs.h"
#include "log.h"
#include "klib.h"


extern char sbss[];
extern char ebss[];

void clear_bss() {
    memset(sbss, 0, ebss - sbss);
}


void main() {
    clear_bss();
    uart_init();
    trap_init();
    load_apps();
    trap_enable_timer_interrupt();
    set_next_timer();
    run_first_task();
    shutdown();
}
