.section .text
.global trapvec
.align 4

trapvec:
	csrrw sp, sscratch, sp
	bnez sp, .from_user

.from_kernel:
	csrrw sp, sscratch, sp

	addi sp, sp, -288

	sd ra, 0(sp)
	sd gp, 16(sp)
	sd tp, 24(sp)
	sd t0, 32(sp)
	sd t1, 40(sp)
	sd t2, 48(sp)
	sd s0, 56(sp)
	sd s1, 64(sp)
	sd a0, 72(sp)
	sd a1, 80(sp)
	sd a2, 88(sp)
	sd a3, 96(sp)
	sd a4, 104(sp)
	sd a5, 112(sp)
	sd a6, 120(sp)
	sd a7, 128(sp)
	sd t3, 216(sp)
	sd t4, 224(sp)
	sd t5, 232(sp)
	sd t6, 240(sp)

	csrr t0, sstatus
	csrr t1, sepc
	csrr t2, scause
	csrr t3, stval
	sd t0, 248(sp)
	sd t1, 256(sp)
	sd t2, 264(sp)
	sd t3, 272(sp)

	csrw sscratch, zero

	call kerneltrap

	ld t0, 248(sp)
	ld t1, 256(sp)
	csrw sstatus, t0
	csrw sepc, t1

	ld ra, 0(sp)
	ld gp, 16(sp)
	ld tp, 24(sp)
	ld t0, 32(sp)
	ld t1, 40(sp)
	ld t2, 48(sp)
	ld s0, 56(sp)
	ld s1, 64(sp)
	ld a0, 72(sp)
	ld a1, 80(sp)
	ld a2, 88(sp)
	ld a3, 96(sp)
	ld a4, 104(sp)
	ld a5, 112(sp)
	ld a6, 120(sp)
	ld a7, 128(sp)
	ld t3, 216(sp)
	ld t4, 224(sp)
	ld t5, 232(sp)
	ld t6, 240(sp)

	addi sp, sp, 288

	sret

.from_user:

	addi sp, sp, -288

	sd s0, 56(sp)

	csrr s0, sscratch
	sd s0, 8(sp)

	sd ra, 0(sp)
	sd gp, 16(sp)
	sd tp, 24(sp)
	sd t0, 32(sp)
	sd t1, 40(sp)
	sd t2, 48(sp)
	sd s1, 64(sp)
	sd a0, 72(sp)
	sd a1, 80(sp)
	sd a2, 88(sp)
	sd a3, 96(sp)
	sd a4, 104(sp)
	sd a5, 112(sp)
	sd a6, 120(sp)
	sd a7, 128(sp)
	sd t3, 216(sp)
	sd t4, 224(sp)
	sd t5, 232(sp)
	sd t6, 240(sp)

	csrr t0, sstatus
	csrr t1, sepc
	csrr t2, scause
	csrr t3, stval
	sd t0, 248(sp)
	sd t1, 256(sp)
	sd t2, 264(sp)
	sd t3, 272(sp)

	csrw sscratch, zero

	call usertrap

	ld t0, 248(sp)
	ld t1, 256(sp)
	csrw sstatus, t0
	csrw sepc, t1

	ld t0, 8(sp)
	csrw sscratch, t0

	ld ra, 0(sp)
	ld gp, 16(sp)
	ld tp, 24(sp)
	ld t0, 32(sp)
	ld t1, 40(sp)
	ld t2, 48(sp)
	ld s0, 56(sp)
	ld s1, 64(sp)
	ld a0, 72(sp)
	ld a1, 80(sp)
	ld a2, 88(sp)
	ld a3, 96(sp)
	ld a4, 104(sp)
	ld a5, 112(sp)
	ld a6, 120(sp)
	ld a7, 128(sp)
	ld t3, 216(sp)
	ld t4, 224(sp)
	ld t5, 232(sp)
	ld t6, 240(sp)

	addi sp, sp, 288

	csrrw sp, sscratch, sp

	sret
