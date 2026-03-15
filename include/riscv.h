#ifndef RISCV_H
#define RISCV_H

#include "utils.h"
#include "config.h"

typedef uint64_t pte_t;
typedef uint64_t vaddr_t;
typedef uint64_t paddr_t;
typedef uint64_t *pagetable_t;

enum Pteflgas : u8 {
	PTE_V = BITS_U8(0),
	PTE_R = BITS_U8(1),
	PTE_W = BITS_U8(2),
	PTE_X = BITS_U8(3),
	PTE_U = BITS_U8(4),
	PTE_G = BITS_U8(5),
	PTE_A = BITS_U8(6),
	PTE_D = BITS_U8(7),
};

#define PXSHIFT(level) (PGSHIFT + (9 * (level)))
#define PX(level, va) ((((u64)(va)) >> PXSHIFT(level)) & PXMASK)

#define PA2PTE(pa) ((((u64)(pa)) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)

#define MAKESATP(token) ((8UL << 60) | ((u64)(token) >> 12))

#define SSTATUS_SPP (1L << 8)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_UPIE (1L << 4)
#define SSTATUS_SIE (1L << 1)
#define SSTATUS_UIE (1L << 0)

#define SIE_SSIE (1L << 1)
#define SIE_STIE (1L << 5)
#define SIE_SEIE (1L << 9)

#define r_csr(reg)                                           \
	({                                                   \
		u64 _val;                                    \
		asm volatile("csrr %0, " #reg : "=r"(_val)); \
		_val;                                        \
	})

#define w_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"r"(val)); })

#define intr_on() w_csr(sstatus, r_csr(sstatus) | SSTATUS_SIE)
#define intr_off() w_csr(sstatus, r_csr(sstatus) & ~SSTATUS_SIE)

enum Interrupt : u64 {
	UserSoft = BITS_U64(63) | 0,
	SupervisorSoft = BITS_U64(63) | 1,
	VirtualSupervisorSoft = BITS_U64(63) | 2,
	UserTimer = BITS_U64(63) | 4,
	SupervisorTimer = BITS_U64(63) | 5,
	VirtualSupervisorTimer = BITS_U64(63) | 6,
	UserExternal = BITS_U64(63) | 8,
	SupervisorExternal = BITS_U64(63) | 9,
	VirtualSupervisorExternal = BITS_U64(63) | 10,
};

enum Exception : u64 {
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

__always_inline void sfence_vma()
{
	asm volatile("sfence.vma zero, zero");
}

__always_inline u64 r_tp(void)
{
	u64 x;
	asm volatile("mv %0, tp" : "=r"(x));
	return x;
}

#endif
