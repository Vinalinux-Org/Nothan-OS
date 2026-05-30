OUTPUT_ARCH(arm)
ENTRY(stext)

PHYS_OFFSET = 0x80000000;
PAGE_OFFSET = 0xC0000000;
MMU_OFFSET  = PAGE_OFFSET - PHYS_OFFSET;

SECTIONS
{
	. = PAGE_OFFSET;

	.head.text : {
		*(.head.text)
	}

	.idmap.text : {
		__idmap_start = .;
		*(.idmap.text)
		__idmap_end = .;
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

	. = ALIGN(16384);
	__pgd_start = .;
	.pgd : {
		*(.pgd)
	}
	_end = .;
}
