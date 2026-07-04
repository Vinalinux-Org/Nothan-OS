/*
 * arch/arm/mm/mmu_enable.S - Enable the ARMv7 MMU and jump to virtual address
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

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

	/*
	 * MMU is on through identity map.  Set VBAR and exception mode
	 * stacks to their VAs before jumping to kernel_main.
	 */
	ldr	r0, =vectors
	mcr	p15, 0, r0, c12, c0, 0
	isb

	cps	#0x11
	ldr	sp, =fiq_stack_top
	cps	#0x12
	ldr	sp, =irq_stack_top
	cps	#0x17
	ldr	sp, =abt_stack_top
	cps	#0x1B
	ldr	sp, =und_stack_top
	cps	#0x13
	ldr	sp, =svc_stack_top

	ldr	pc, =kernel_main
	.size	__turn_mmu_on, . - __turn_mmu_on
__turn_mmu_on_end:
