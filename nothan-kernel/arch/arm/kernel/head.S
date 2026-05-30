/*
 * Bootloader hands off with MMU off, D-cache off, I-cache off,
 * CPU in SVC mode, UART already initialised.
 *
 * r0 = 0
 * r1 = machine type (3589 for BBB)
 * r2 = pointer to boot_params
 */

	.section ".head.text", "ax"

	/*
	 * First instruction must pass bootloader's magic check:
	 * a B (0xEA......) or LDR-pc (0xE59FF...), not zero, not 0xFFFFFFFF.
	 */
	b	stext

	.globl	stext
	.type	stext, %function
stext:
	/*
	 * Safe state: disable interrupts, stay in SVC mode.
	 * Bootloader already left us in SVC, but be explicit.
	 */
	cpsid	if

	ldr	sp, =init_stack

	/* Save boot parameters for kernel_main(). */
	ldr	r8, =machine_type
	str	r1, [r8]
	ldr	r8, =boot_params_ptr
	str	r2, [r8]

	/* Clear BSS. */
	ldr	r4, =__bss_start
	ldr	r5, =__bss_stop
	mov	r6, #0
1:	cmp	r4, r5
	strlo	r6, [r4], #4
	blo	1b

	b	kernel_main
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
