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
		_stext = .;
		*(.text)
		*(.init.text)
		_etext = .;
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
		*(.initcall.early)
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

		/*
		 * Abort and Undef: 4 KB each, up from 1 KB.
		 *
		 * These are sized by what the HANDLER does, not by how often it
		 * runs. dabt_handler() prints the fault registers and walks a
		 * backtrace, and every printk puts a 256-byte line buffer on
		 * this stack. Overflowing here is uniquely bad: the fault would
		 * be taken while already handling a fault, and the machine stops
		 * without ever printing the original abort - losing exactly the
		 * information the handler exists to produce.
		 */
		abt_stack_bottom = .;
		. += 0x1000;
		abt_stack_top = .;

		und_stack_bottom = .;
		. += 0x1000;
		und_stack_top = .;

		/*
		 * The boot SVC stack, 16 KB (was 8 KB), and it is NOT only the
		 * boot stack: the idle task inherits it forever. __switch_to
		 * saves the running SP into prev->stack, and the first switch
		 * has prev == the idle task - so whatever kernel_main was using
		 * becomes idle's stack permanently. Idle then runs reap_dead()
		 * (printk + frees) and absorbs full IRQ nesting on it.
		 *
		 * It cannot be a guarded kstack: it has to exist before the
		 * page allocator does. A canary guards it instead - see
		 * stack_guard_check() in arch/arm/mm/kstack.c.
		 */
		svc_stack_bottom = .;
		. += 0x4000;
		svc_stack_top = .;
	}

	. = ALIGN(16384);
	__pgd_start = .;
	.pgd : {
		*(.pgd)
	}
	_end = .;
}
