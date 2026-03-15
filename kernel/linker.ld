OUTPUT_ARCH( "riscv" )
ENTRY( _start )
BASE_ADDRESS = 0x80200000;

SECTIONS
{
	. = BASE_ADDRESS;
	skernel = .;

	stext = .;
	.text : {
		*(.text.entry)
		. = ALIGN(0x1000);
		*(.text .text.*)
	}

	. = ALIGN(0x1000);
	etext = .;
	srodata = .;
	.rodata : {
		*(.rodata .rodata.*)
		*(.srodata .srodata.*)
	}

	 . = ALIGN(0x1000);
	erodata = .;
	sdata = .;
	.data : {
		*(.data .data.*)
	        *(.sdata .sdata.*)
		*(.data.stack)
	}

	. = ALIGN(0x1000);
	edata = .;
	sbss = .;
	.bss : {
		*(.bss .bss.*)
		*(.sbss .sbss.*)
	}

	. = ALIGN(0x1000);
	ebss = .;
	ekernel = .;

	/DISCARD/ : {
		*(.eh_frame)
	}
}
