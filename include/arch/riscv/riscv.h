#ifndef __RISCV_H__
#define __RISCV_H__

#include <stdint.h>

#include "generated/autoconf.h"

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

#define SIE_SSIE (1L << 1)
#define SIE_STIE (1L << 5)
#define SIE_SEIE (1L << 9)

#define r_csr(reg)                                                                                                     \
    ({                                                                                                                 \
        uint64_t _val;                                                                                                 \
        asm volatile("csrr %0, " #reg : "=r"(_val));                                                                   \
        _val;                                                                                                          \
    })

#define w_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"r"(val)); })

#define intr_on() w_csr(sstatus, r_csr(sstatus) | SSTATUS_SIE)
#define intr_off() w_csr(sstatus, r_csr(sstatus) & ~SSTATUS_SIE)

#define sfence_vma() ({ asm volatile("sfence.vma zero, zero"); })

#ifdef CONFIG_RV_QEMU
#define CLOCK_FREQ 12500000ULL
#define TICKS_PRE_SEC 100L
#define MSEC_PER_SEC 1000L

#define SIFIVE_TEST_ADDR 0x100000
#define UART0 0x10000000L

// UART device
#define RBR 0
#define THR 0
#define LSR 5
#define LSR_TX_IDLE (1 << 5)
#define LSR_RX_READY (1 << 0)
#define UART_REG(reg) ((volatile unsigned char *)(UART0 + reg))

#endif // CONFIG_RV_QEMU

#endif // __RISCV_H__
