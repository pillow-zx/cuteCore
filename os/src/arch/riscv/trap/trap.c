#include "trap.h"
#include "defs.h"
#include "log.h"
#include "riscv.h"
#include <stdint.h>

void trap_init(void) {
    extern uintptr_t __alltraps;
    w_csr(stvec, (uintptr_t)&__alltraps);
    Info("trap init, stvec = %lx", r_csr(stvec));
}

TrapContext *trap_handler(TrapContext *cx) {
    uint64_t scause = r_csr(scause);
    uint64_t stval = r_csr(stval);

    if (scause == SYSCALL) {
        cx->sepc += 4;
        cx->regs[10] = syscall(cx->regs[17], (uint64_t[3]){cx->regs[10], cx->regs[11], cx->regs[12]});
    } else {
        panic("Unhandled trap: scause = %lx, stval = %lx", scause, stval);
        shutdown();
    }

    return cx;
}
