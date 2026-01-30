#ifndef __RISCV_H__
#define __RISCV_H__


#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable


#define r_csr(reg)                                                                                                     \
    ({                                                                                                                 \
        uint64_t _val;                                                                                                 \
        asm volatile("csrr %0, " #reg : "=r"(_val));                                                                   \
        _val;                                                                                                          \
    })

#define w_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"r"(val)); })

#define sfence_vma() ({ asm volatile("sfence.vma zero, zero"); })


#endif // __RISCV_H__
