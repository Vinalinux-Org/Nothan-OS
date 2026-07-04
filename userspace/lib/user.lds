/*
 * Binary layout for NothanOS user programs:
 *
 *   [0x10000] 16-byte header: magic 'NOSH', bss_size, reserved
 *   [0x10010] _start (entry point)
 *   .text / .rodata / .data (padded to page boundary)
 *   [page-aligned] .bss (NOLOAD — not in .bin, kernel maps zeroed pages)
 *
 * Kernel parses the header, allocates and zeros bss_size bytes of pages,
 * and maps them in the L2 right after the code pages.
 */
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	. = 0x00010000;

	.header : {
		LONG(0x4E4F5348)        /* magic 'NOSH' */
		LONG(__bss_size)
		LONG(0)                 /* reserved */
		LONG(0)                 /* reserved */
	}

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
		. = ALIGN(4096);
	}

	.bss (NOLOAD) : {
		__bss_start = .;
		*(.bss)
		/* Everyone's .bss.* EXCEPT the guarded draw buffer (placed last). */
		*(EXCLUDE_FILE(*lv_port_disp.o) .bss.*)
		*(COMMON)
		/*
		 * Diagnostic guard: the LVGL flush draw buffer (.bss.guarded) is
		 * pinned LAST in .bss on a fresh page, and .bss ends on a page
		 * boundary right after it. The kernel maps exactly ceil(bss_size)
		 * pages, so the page above __bss_end is unmapped — a forward
		 * over-run of the draw buffer faults cleanly (Data Abort, DFAR ≈
		 * __bss_end) instead of silently corrupting the TLSF pool, which
		 * now sits well below it. See lv_port_disp.c.
		 */
		. = ALIGN(4096);
		*(.bss.guarded)
		. = ALIGN(4096);
		__bss_end = .;
	}
	__bss_size = __bss_end - __bss_start;

	/DISCARD/ : {
		*(.comment)
		*(.ARM.*)
		*(.debug*)
		*(.note.*)
		*(.eh_frame*)
	}
}
