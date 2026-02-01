#include <stdint.h>
#include "defs.h"
#include "riscv.h"


uint64_t get_time() {
    uint64_t val;
    asm volatile("rdtime %0" : "=r"(val));
    return val;
};

uint64_t get_time_ms() {
    return get_time() / (CLOCK_FREQ / MSEC_PER_SEC);
}

void set_next_timer() {
    w_csr(stimecmp, get_time() + CLOCK_FREQ / TICKS_PRE_SEC);
}

