    .section .text.entry
    .global _start
_start:
    la sp, boot_stack_top
    call main

    .section .bss.stack
    .global boot_stack_lower_bound
boot_stack_lower_bound:
    .space 0x4000
    .global boot_stack_top
boot_stack_top:
