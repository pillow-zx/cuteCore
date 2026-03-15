.section .text.entry
.global _start
_start:
	mv s0, a0
	mv s1, a1

	bnez s0, wait_for_boot

	la t0, sbss
	la t1, ebss

clear_bss:
	beq t0, t1, clear_done
	sd zero, 0(t0)
	addi t0, t0, 8
	j clear_bss
clear_done:
	csrw sscratch, zero
	la sp, boot_stack_top
	mv a0, s0
	mv a1, s1
	call kernel_main

wait_for_boot:
	wfi
	j wait_for_boot

.section .data.stack
.global boot_stack_lower_bound
boot_stack_lower_bound:
	.space 0x90000
.global boot_stack_top
boot_stack_top:
