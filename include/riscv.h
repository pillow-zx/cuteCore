/**
 * @file riscv.h
 * @brief RISC-V 架构相关定义
 *
 * 本文件包含 RISC-V 64 位架构的核心定义，包括：
 * - 页表项（PTE）结构与时操作宏
 * - CSR 寄存器访问宏
 * - 中断与异常编码
 * - Sv39 虚拟内存支持
 */

#ifndef RISCV_H
#define RISCV_H

#include "utils.h"
#include "config.h"

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

/** @brief 页表项类型（64 位无符号整数） */
typedef uint64_t pte_t;

/** @brief 虚拟地址类型 */
typedef uint64_t vaddr_t;

/** @brief 物理地址类型 */
typedef uint64_t paddr_t;

/** @brief 页表根节点指针 */
typedef uint64_t *pagetable_t;

/* ============================================================================
 * Page Table Entry Flags (PTE)
 * ============================================================================ */

/** @brief Valid - 页表项有效 */
#define PTE_V BITS_U8(0)

/** @brief Readable - 可读 */
#define PTE_R BITS_U8(1)

/** @brief Writable - 可写 */
#define PTE_W BITS_U8(2)

/** @brief Executable - 可执行 */
#define PTE_X BITS_U8(3)

/** @brief User - 用户态可访问（未设置时仅 S/M mode 可访问） */
#define PTE_U BITS_U8(4)

/** @brief Global - 全局映射（所有地址空间共享，用于内核映射） */
#define PTE_G BITS_U8(5)

/** @brief Accessed - 已被访问（硬件自动设置） */
#define PTE_A BITS_U8(6)

/** @brief Dirty - 已被写入（硬件自动设置） */
#define PTE_D BITS_U8(7)

/* ============================================================================
 * Page Table Walking Macros (Sv39)
 * ============================================================================ */

/**
 * @brief 计算指定级别的页表索引位移量
 *
 * Sv39 使用三级页表，每级 9 位索引：
 * - Level 2: bits 38-30
 * - Level 1: bits 29-21
 * - Level 0: bits 20-12
 *
 * @param level 页表级别（0, 1, 2）
 */
#define PXSHIFT(level) (PGSHIFT + (9 * (level)))

/**
 * @brief 提取虚拟地址在指定级别的页表索引
 *
 * @param level 页表级别（0, 1, 2）
 * @param va    虚拟地址
 *
 * @return 9 位页表索引（0-511）
 */
#define PX(level, va) ((((u64)(va)) >> PXSHIFT(level)) & PXMASK)

/* ============================================================================
 * Address Translation Macros
 * ============================================================================ */

/**
 * @brief 物理地址转换为页表项值
 *
 * 将物理地址右移 12 位（去除页内偏移），再左移 10 位（对齐到 PTE 格式）。
 *
 * @param pa 物理地址（必须页对齐）
 *
 * @return PTE 格式的物理页号
 */
#define PA2PTE(pa) ((((u64)(pa)) >> 12) << 10)

/**
 * @brief 页表项值转换为物理地址
 *
 * 从 PTE 中提取物理页号，恢复为完整物理地址。
 *
 * @param pte 页表项值
 *
 * @return 物理地址
 */
#define PTE2PA(pte) (((pte) >> 10) << 12)

/**
 * @brief 构造 satp 寄存器值
 *
 * satp 格式（Sv39）：
 * - bits 63-60: MODE = 8 (Sv39)
 * - bits 59-44: ASID（未使用，置 0）
 * - bits 43-0:  PPN（页表根节点物理页号）
 *
 * @param token 页表根节点地址
 *
 * @return 可直接写入 satp CSR 的值
 */
#define MAKESATP(token) ((8UL << 60) | ((u64)(token) >> 12))

/* ============================================================================
 * SSTATUS Register Bits
 * ============================================================================ */

/** @brief Supervisor Previous Privilege - 进入 trap 前的模式（1=S-mode, 0=U-mode） */
#define SSTATUS_SPP (1L << 8)

/** @brief Supervisor Previous Interrupt Enable - trap 前中断使能状态 */
#define SSTATUS_SPIE (1L << 5)

/** @brief User Previous Interrupt Enable - U-mode 下 trap 前中断使能状态 */
#define SSTATUS_UPIE (1L << 4)

/** @brief Supervisor Interrupt Enable - S-mode 中断使能 */
#define SSTATUS_SIE (1L << 1)

/** @brief User Interrupt Enable - U-mode 中断使能 */
#define SSTATUS_UIE (1L << 0)

/* ============================================================================
 * SIE (Supervisor Interrupt Enable) Register Bits
 * ============================================================================ */

/** @brief Supervisor Software Interrupt Enable */
#define SIE_SSIE (1L << 1)

/** @brief Supervisor Timer Interrupt Enable */
#define SIE_STIE (1L << 5)

/** @brief Supervisor External Interrupt Enable */
#define SIE_SEIE (1L << 9)

/* ============================================================================
 * CSR Access Macros
 * ============================================================================ */

/**
 * @brief 读取 CSR 寄存器
 *
 * @param reg CSR 寄存器名称（如 sstatus, sepc, scause）
 *
 * @return CSR 当前值（u64）
 *
 * @example u64 val = r_csr(sstatus);
 */
#define r_csr(reg)                                           \
	({                                                   \
		u64 _val;                                    \
		asm volatile("csrr %0, " #reg : "=r"(_val)); \
		_val;                                        \
	})

/**
 * @brief 写入 CSR 寄存器
 *
 * @param reg CSR 寄存器名称
 * @param val 要写入的值
 *
 * @example w_csr(satp, 0x80000000012345000);
 */
#define w_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"r"(val)); })

/* ============================================================================
 * Interrupt Control
 * ============================================================================ */

/** @brief 开启 S-mode 中断 */
#define intr_on() w_csr(sstatus, r_csr(sstatus) | SSTATUS_SIE)

/** @brief 关闭 S-mode 中断 */
#define intr_off() w_csr(sstatus, r_csr(sstatus) & ~SSTATUS_SIE)

/* ============================================================================
 * Interrupt Codes (scause with bit 63 set)
 * ============================================================================ */

/**
 * @brief 中断类型编码
 *
 * scause 寄存器 bit 63 = 1 表示中断，bits 62:0 为中断原因。
 */
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

/* ============================================================================
 * Exception Codes (scause with bit 63 clear)
 * ============================================================================ */

/**
 * @brief 异常类型编码
 *
 * scause 寄存器 bit 63 = 0 表示异常，bits 62:0 为异常原因。
 */
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

/* ============================================================================
 * Memory Barrier Functions
 * ============================================================================ */

/**
 * @brief 刷新 TLB（Translation Lookaside Buffer）
 *
 * 执行 sfence.vma zero, zero 指令，刷新所有 ASID 的所有虚拟地址映射。
 * 在修改页表或切换 satp 后必须调用。
 */
__always_inline void sfence_vma()
{
	asm volatile("sfence.vma zero, zero");
}

/* ============================================================================
 * CPU Identification
 * ============================================================================ */

/**
 * @brief 读取 tp 寄存器
 *
 * tp 寄存器在多核系统中用于存储 CPU ID（hart ID）。
 * 内核启动时由 QEMU/OpenSBI 设置。
 *
 * @return 当前 CPU 的 hart ID
 */
__always_inline u64 r_tp(void)
{
	u64 x;
	asm volatile("mv %0, tp" : "=r"(x));
	return x;
}

#endif
