OUTPUT_ARCH(arm)
ENTRY(stext)

SECTIONS
{
	. = 0x80000000;

	.head.text : {
		*(.head.text)
	}

	.text : {
		*(.text)
	}

	.rodata : {
		*(.rodata)
		*(.rodata.*)
	}

	.data : {
		*(.data)
	}

	. = ALIGN(8);

	.bss : {
		__bss_start = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_stop = .;
	}

	_end = .;
}
