OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	. = 0x00010000;

	.text : {
		*(.text)
		*(.text.*)
	}

	.rodata : ALIGN(4) {
		*(.rodata)
		*(.rodata.*)
	}

	.data : ALIGN(4) {
		*(.data)
		*(.data.*)
		. = ALIGN(4);
	}

	.bss : ALIGN(4) {
		__bss_start = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	}

	/DISCARD/ : {
		*(.comment)
		*(.ARM.*)
		*(.debug*)
		*(.note.*)
		*(.eh_frame*)
	}
}
