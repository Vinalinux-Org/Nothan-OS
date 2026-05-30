#include <asm/pgtable.h>

	.section ".idmap.text", "ax"

/*
 * Enable MMU and jump to virtual address.
 *
 * r0 = physical address of L1 page table
 *
 * Identity map covers this section so execution survives MMU enable.
 */
	.globl	__turn_mmu_on
	.type	__turn_mmu_on, %function
__turn_mmu_on:
	orr	r1, r0, #0x4A
	mcr	p15, 0, r1, c2, c0, 0		/* TTBR0 = pgd */

	mov	r1, #0
	mcr	p15, 0, r1, c2, c0, 2		/* TTBCR = 0 */

	ldr	r1, =DACR_INIT
	mcr	p15, 0, r1, c3, c0, 0		/* DACR */

	dsb
	isb

	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0		/* TLBIALL */
	mcr	p15, 0, r1, c7, c5, 0		/* ICIALLU */
	dsb
	isb

	mrc	p15, 0, r1, c1, c0, 0		/* SCTLR */
	orr	r1, r1, #0x1000			/* I-cache */
	orr	r1, r1, #0x0005			/* MMU + D-cache */
	mcr	p15, 0, r1, c1, c0, 0		/* enable MMU */
	isb

	/* Identity map keeps us alive; jump to kernel_main at VA. */
	ldr	pc, =kernel_main
	.size	__turn_mmu_on, . - __turn_mmu_on
__turn_mmu_on_end:
