#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdint.h>

#include "generated/autoconf.h"

#ifdef CONFIG_ISA_RISCV
#include "riscv.h"

enum TrapCause {
    SYSCALL = 8,
};

typedef struct {
    uint64_t regs[32];
    uint64_t sstatus;
    uint64_t sepc;
} TrapContext;

static inline void set_sp(TrapContext *cx, uint64_t sp) { cx->regs[2] = sp; }

static inline TrapContext app_init6_context(uint64_t entry, uint64_t sp) {
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
