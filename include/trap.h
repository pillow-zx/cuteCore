#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdint.h>

#include "generated/autoconf.h"

#ifdef CONFIG_ISA_RISCV
#include "riscv.h"
#include "utils.h"


enum Interrupt {
    UserSoft =  (1ULL << 63) | 0,
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

#endif // CONFIG_ISA_RISCV

#endif // __CONTEXT_H__
