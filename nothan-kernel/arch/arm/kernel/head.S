/*
 * Bootloader hands off: MMU off, D-cache off, I-cache off,
 * CPU in SVC mode, UART already initialised.
 *
 * r0 = 0
 * r1 = machine type (3589 for BBB)
 * r2 = pointer to boot_params
 *
 * Kernel linked at VA, loaded at phys. The linker defines
 * MMU_OFFSET = PAGE_OFFSET - PHYS_OFFSET (0x40000000).
 * Before MMU, subtract MMU_OFFSET from any symbol VA to get phys.
 */

	.section ".head.text", "ax"

	.globl	_start
_start:
	b	stext

	.globl	stext
	.type	stext, %function
stext:
	cpsid	if

	/* MMU_OFFSET in r7 for all pre-MMU phys conversions. */
	ldr	r7, =MMU_OFFSET

	/* Stack at phys (symbol is VA). */
	ldr	sp, =init_stack
	sub	sp, sp, r7

	/* Set exception mode stacks at phys. */
	cps	#0x11
	ldr	sp, =fiq_stack_top
	sub	sp, sp, r7

	cps	#0x12
	ldr	sp, =irq_stack_top
	sub	sp, sp, r7

	cps	#0x17
	ldr	sp, =abt_stack_top
	sub	sp, sp, r7

	cps	#0x1B
	ldr	sp, =und_stack_top
	sub	sp, sp, r7

	cps	#0x13
	ldr	sp, =svc_stack_top
	sub	sp, sp, r7

	/* Clear BSS at phys. */
	ldr	r4, =__bss_start
	sub	r4, r4, r7
	ldr	r5, =__bss_stop
	sub	r5, r5, r7
	mov	r6, #0
1:	cmp	r4, r5
	strlo	r6, [r4], #4
	blo	1b

	/* Set VBAR = phys address of vectors. */
	ldr	r0, =vectors
	sub	r0, r0, r7
	mcr	p15, 0, r0, c12, c0, 0
	isb

	/* Save boot params (BSS is zero, now write). */
	ldr	r8, =machine_type
	sub	r8, r8, r7
	str	r1, [r8]
	ldr	r8, =boot_params_ptr
	sub	r8, r8, r7
	str	r2, [r8]

	b	mmu_init
	.size	stext, . - stext

	.section .bss
	.align	2
machine_type:
	.space	4
boot_params_ptr:
	.space	4

	.align	3
init_stack_bottom:
	.space	4096
init_stack:
