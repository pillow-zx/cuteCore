#include <stdint.h>

#include "arch/riscv/riscv.h"
#include "arch/riscv/trap.h"
#include "kernel/common.h"
#include "log.h"

void trap_init(void) {
    extern uintptr_t __alltraps;
    w_csr(stvec, (uintptr_t)&__alltraps);
}

void trap_enable_timer_interrupt(void) { w_csr(sie, r_csr(sie) | SIE_STIE); }

TrapContext *trap_handler(TrapContext *cx) {
    uint64_t scause = r_csr(scause);
    uint64_t stval = r_csr(stval);

    if ((int64_t)scause < 0) {
        switch (scause) {
            case SupervisorTimer: {
                suspend_current_and_run_next();
                break;
            }
            default: {
                panic("Unhandled interrupt: scause = %lx, stval = %lx", scause, stval);
            }
        }
    } else {
        switch (scause) {
            case UserEnvCall: {
                cx->sepc += 4;
                cx->regs[10] = syscall(cx->regs[17], (uint64_t[]){cx->regs[10], cx->regs[11], cx->regs[12]});
                break;
            }
            case StoreFault:
            case StorePageFault: {
                Debug("[kernel] PageFault in application, bad addr = %lx, bad instruction = %lx", stval, cx->sepc);
                exit_current_and_run_next();
                break;
            }
            case IllegalInstruction: {
                Debug("[kernel] IllegalInstruction bad instruction = %lx", cx->sepc);
                exit_current_and_run_next();
                break;
            }
            default: {
                panic("Unhandled trap: scause = %lx, stval = %lx, sepc = %lx, sstatus: %lx", scause, stval, cx->sepc,
                      cx->sstatus);
            }
        }
    }

    return cx;
}
