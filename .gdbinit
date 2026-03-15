add-auto-load-safe-path /home/pillow/code/cuteCore/.gdbinit
set architecture riscv:rv64
set mem inaccessible-by-default off
add-symbol-file build/kernel.elf 0x80200000
target remote :26000
