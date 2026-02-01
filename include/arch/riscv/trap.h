#ifndef __ARCH_RISCV_TRAP_H__
#define __ARCH_RISCV_TRAP_H__

#include "arch/riscv/riscv.h"
#include "generated/autoconf.h"
#include "utils/utils.h"
#include <stdint.h>

enum Interrupt {
    UserSoft = (1ULL << 63) | 0,
    SupervisorSoft = (1ULL << 63) | 1,
    VirtualSupervisorSoft = (1ULL << 63) | 2,
    UserTimer = (1ULL << 63) | 4,
    SupervisorTimer = (1ULL << 63) | 5,
    VirtualSupervisorTimer = (1ULL << 63) | 6,
    UserExternal = (1ULL << 63) | 8,
    SupervisorExternal = (1ULL << 63) | 9,
    VirtualSupervisorExternal = (1ULL << 63) | 10,
};

enum Exception {
    InstructionMisaligned = 0,
    InstructionFault = 1,
    IllegalInstruction = 2,
    Breakpoint = 3,
    LoadFault = 5,
    StoreMisaligned = 6,
    StoreFault = 7,
    UserEnvCall = 8,
    VirtualSuperVisorEnvCall = 9,
    InstructionPageFault = 12,
    LoadPageFault = 13,
    StorePageFault = 15,
    InstructionGuestPageFault = 20,
    LoadGuestPageFault = 21,
    VirtualInstruction = 22,
    StoreGuestPageFault = 23,
};

typedef struct {
    uint64_t regs[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapContext;

__always_inline void set_sp(TrapContext *cx, uint64_t sp) { cx->regs[2] = sp; }

__always_inline TrapContext app_init_context(uint64_t entry, uint64_t sp) {
    TrapContext cx = {0};
    cx.sepc = entry;
    cx.regs[2] = sp;
    uint64_t sstatus = r_csr(sstatus);
    sstatus &= ~SSTATUS_SPP;
    sstatus |= SSTATUS_SPIE;
    cx.sstatus = sstatus;

    return cx;
}

// trap.c
void trap_init();
void trap_enable_timer_interrupt();

// timer.c
__maybe_unused uint64_t get_time();
__maybe_unused uint64_t get_time_ms();
__maybe_unused void set_next_timer();

#endif // __ARCH_RISCV_TRAP_H__