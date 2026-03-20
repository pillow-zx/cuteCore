.section .text
.global trapvec
.align 4

/*
 * trapvec: RISC-V trap vector
 *
 * Entry:
 *   - From user:   sscratch = user_sp, sp = user_sp
 *   - From kernel: sscratch = 0, sp = kernel_sp
 *
 * Register layout in trapframe (offset from base):
 *   0-248:  x0-x31 (32 GP registers, but x0 is unused)
 *   248:    sstatus
 *   256:    sepc
 *   264:    scause
 *   272:    stval
 */

/* Offsets within trapframe */
#define OFF_ra   0
#define OFF_sp   8
#define OFF_gp   16
#define OFF_tp   24
#define OFF_t0   32
#define OFF_t1   40
#define OFF_t2   48
#define OFF_s0   56
#define OFF_s1   64
#define OFF_a0   72
#define OFF_a1   80
#define OFF_a2   88
#define OFF_a3   96
#define OFF_a4   104
#define OFF_a5   112
#define OFF_a6   120
#define OFF_a7   128
#define OFF_s2   136
#define OFF_s3   144
#define OFF_s4   152
#define OFF_s5   160
#define OFF_s6   168
#define OFF_s7   176
#define OFF_s8   184
#define OFF_s9   192
#define OFF_s10  200
#define OFF_s11  208
#define OFF_t3   216
#define OFF_t4   224
#define OFF_t5   232
#define OFF_t6   240
#define OFF_sstatus  248
#define OFF_sepc    256
#define OFF_scause  264
#define OFF_stval   272

#define TRAPFRAME_SZ  280

trapvec:
	/* Swap sp with sscratch to detect trap source */
	csrrw sp, sscratch, sp
	bnez sp, .from_user

.from_kernel:
	/* sscratch was 0, restore kernel sp and handle kernel trap */
	csrrw sp, sscratch, sp

	/* Allocate space on kernel stack for trapframe */
	addi sp, sp, -TRAPFRAME_SZ

	/* Save all general-purpose registers */
	sd ra, OFF_ra(sp)
	sd gp, OFF_gp(sp)
	sd tp, OFF_tp(sp)
	sd t0, OFF_t0(sp)
	sd t1, OFF_t1(sp)
	sd t2, OFF_t2(sp)
	sd s0, OFF_s0(sp)
	sd s1, OFF_s1(sp)
	sd a0, OFF_a0(sp)
	sd a1, OFF_a1(sp)
	sd a2, OFF_a2(sp)
	sd a3, OFF_a3(sp)
	sd a4, OFF_a4(sp)
	sd a5, OFF_a5(sp)
	sd a6, OFF_a6(sp)
	sd a7, OFF_a7(sp)
	sd s2, OFF_s2(sp)
	sd s3, OFF_s3(sp)
	sd s4, OFF_s4(sp)
	sd s5, OFF_s5(sp)
	sd s6, OFF_s6(sp)
	sd s7, OFF_s7(sp)
	sd s8, OFF_s8(sp)
	sd s9, OFF_s9(sp)
	sd s10, OFF_s10(sp)
	sd s11, OFF_s11(sp)
	sd t3, OFF_t3(sp)
	sd t4, OFF_t4(sp)
	sd t5, OFF_t5(sp)
	sd t6, OFF_t6(sp)

	/* Save trap-related CSRs */
	csrr t0, sstatus
	csrr t1, sepc
	csrr t2, scause
	csrr t3, stval
	sd t0, OFF_sstatus(sp)
	sd t1, OFF_sepc(sp)
	sd t2, OFF_scause(sp)
	sd t3, OFF_stval(sp)

	/* Clear sscratch */
	csrw sscratch, zero

	call kerneltrap

	/* Restore CSRs */
	ld t0, OFF_sstatus(sp)
	ld t1, OFF_sepc(sp)
	csrw sstatus, t0
	csrw sepc, t1

	/* Restore all general-purpose registers */
	ld ra, OFF_ra(sp)
	ld gp, OFF_gp(sp)
	ld tp, OFF_tp(sp)
	ld t0, OFF_t0(sp)
	ld t1, OFF_t1(sp)
	ld t2, OFF_t2(sp)
	ld s0, OFF_s0(sp)
	ld s1, OFF_s1(sp)
	ld a0, OFF_a0(sp)
	ld a1, OFF_a1(sp)
	ld a2, OFF_a2(sp)
	ld a3, OFF_a3(sp)
	ld a4, OFF_a4(sp)
	ld a5, OFF_a5(sp)
	ld a6, OFF_a6(sp)
	ld a7, OFF_a7(sp)
	ld s2, OFF_s2(sp)
	ld s3, OFF_s3(sp)
	ld s4, OFF_s4(sp)
	ld s5, OFF_s5(sp)
	ld s6, OFF_s6(sp)
	ld s7, OFF_s7(sp)
	ld s8, OFF_s8(sp)
	ld s9, OFF_s9(sp)
	ld s10, OFF_s10(sp)
	ld s11, OFF_s11(sp)
	ld t3, OFF_t3(sp)
	ld t4, OFF_t4(sp)
	ld t5, OFF_t5(sp)
	ld t6, OFF_t6(sp)

	addi sp, sp, TRAPFRAME_SZ

	sret

.from_user:
	/*
	 * Entry from user mode:
	 *   - sscratch = kernel_sp (set by trapret)
	 *   - sp = user_sp
	 * After csrrw:
	 *   - sp = kernel_sp (switched to kernel stack)
	 *   - sscratch = user_sp (saved for later)
	 */
	csrrw sp, sscratch, sp

	/* Allocate space on kernel stack for trapframe */
	addi sp, sp, -TRAPFRAME_SZ

	/* Save all general-purpose registers */
	sd ra, OFF_ra(sp)
	sd gp, OFF_gp(sp)
	sd tp, OFF_tp(sp)
	sd t0, OFF_t0(sp)
	sd t1, OFF_t1(sp)
	sd t2, OFF_t2(sp)
	sd s0, OFF_s0(sp)
	sd s1, OFF_s1(sp)
	sd a0, OFF_a0(sp)
	sd a1, OFF_a1(sp)
	sd a2, OFF_a2(sp)
	sd a3, OFF_a3(sp)
	sd a4, OFF_a4(sp)
	sd a5, OFF_a5(sp)
	sd a6, OFF_a6(sp)
	sd a7, OFF_a7(sp)
	sd s2, OFF_s2(sp)
	sd s3, OFF_s3(sp)
	sd s4, OFF_s4(sp)
	sd s5, OFF_s5(sp)
	sd s6, OFF_s6(sp)
	sd s7, OFF_s7(sp)
	sd s8, OFF_s8(sp)
	sd s9, OFF_s9(sp)
	sd s10, OFF_s10(sp)
	sd s11, OFF_s11(sp)
	sd t3, OFF_t3(sp)
	sd t4, OFF_t4(sp)
	sd t5, OFF_t5(sp)
	sd t6, OFF_t6(sp)

	/* Save user sp from sscratch */
	csrr s0, sscratch
	sd s0, OFF_sp(sp)

	/* Save trap-related CSRs */
	csrr t0, sstatus
	csrr t1, sepc
	csrr t2, scause
	csrr t3, stval
	sd t0, OFF_sstatus(sp)
	sd t1, OFF_sepc(sp)
	sd t2, OFF_scause(sp)
	sd t3, OFF_stval(sp)

	/* Clear sscratch */
	csrw sscratch, zero

	call usertrap

	/* Restore CSRs */
	ld t0, OFF_sstatus(sp)
	ld t1, OFF_sepc(sp)
	csrw sstatus, t0
	csrw sepc, t1

	/* Restore all general-purpose registers */
	ld ra, OFF_ra(sp)
	ld gp, OFF_gp(sp)
	ld tp, OFF_tp(sp)
	ld t0, OFF_t0(sp)
	ld t1, OFF_t1(sp)
	ld t2, OFF_t2(sp)
	ld s0, OFF_s0(sp)
	ld s1, OFF_s1(sp)
	ld a0, OFF_a0(sp)
	ld a1, OFF_a1(sp)
	ld a2, OFF_a2(sp)
	ld a3, OFF_a3(sp)
	ld a4, OFF_a4(sp)
	ld a5, OFF_a5(sp)
	ld a6, OFF_a6(sp)
	ld a7, OFF_a7(sp)
	ld s2, OFF_s2(sp)
	ld s3, OFF_s3(sp)
	ld s4, OFF_s4(sp)
	ld s5, OFF_s5(sp)
	ld s6, OFF_s6(sp)
	ld s7, OFF_s7(sp)
	ld s8, OFF_s8(sp)
	ld s9, OFF_s9(sp)
	ld s10, OFF_s10(sp)
	ld s11, OFF_s11(sp)
	ld t3, OFF_t3(sp)
	ld t4, OFF_t4(sp)
	ld t5, OFF_t5(sp)
	ld t6, OFF_t6(sp)

	/*
	 * Restore user sp and set sscratch for next trap:
	 *   - Load user_sp into t0
	 *   - Set sscratch = user_sp (for next trap detection)
	 *   - Restore kernel sp
	 *   - Switch to user_sp via sscratch
	 */
	ld t0, OFF_sp(sp)
	csrw sscratch, t0

	addi sp, sp, TRAPFRAME_SZ

	/* Swap: sp = user_sp, sscratch = kernel_sp */
	csrrw sp, sscratch, sp

	sret
