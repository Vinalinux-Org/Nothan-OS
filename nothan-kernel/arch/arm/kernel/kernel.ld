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

	.vectors : ALIGN(32) {
		vectors_start = .;
		*(.vectors)
		vectors_end = .;
	}

	.idmap.text : {
		__idmap_start = .;
		*(.idmap.text)
		__idmap_end = .;
	}

	.text : {
		*(.text)
		*(.init.text)
	}

	.rodata : {
		*(.rodata)
		*(.rodata.*)
	}

	.data : {
		*(.data)
	}

	. = ALIGN(4);
	__initcall_start = .;
	.initcall : {
		*(.initcall.0)
		*(.initcall.1)
		*(.initcall.2)
		*(.initcall.3)
		*(.initcall.4)
	}
	__initcall_end = .;

	. = ALIGN(8);

	.bss : {
		__bss_start = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_stop = .;

		. = ALIGN(8);
		fiq_stack_bottom = .;
		. += 0x200;
		fiq_stack_top = .;

		irq_stack_bottom = .;
		. += 0x1000;
		irq_stack_top = .;

		abt_stack_bottom = .;
		. += 0x400;
		abt_stack_top = .;

		und_stack_bottom = .;
		. += 0x400;
		und_stack_top = .;

		svc_stack_bottom = .;
		. += 0x2000;
		svc_stack_top = .;
	}

	. = ALIGN(16384);
	__pgd_start = .;
	.pgd : {
		*(.pgd)
	}
	_end = .;
}
