/*
 * void __switch_to(struct task_struct *prev, struct task_struct *next)
 *
 * r0 = prev, r1 = next
 */
	.text

	.globl	__switch_to
	.type	__switch_to, %function
__switch_to:
	stmfd	sp!, {r4-r11, lr}
	str	sp, [r0, #0]
	ldr	sp, [r1, #0]
	ldmfd	sp!, {r4-r11, pc}
	.size	__switch_to, . - __switch_to

/*
 * void task_entry(void)
 *
 * Trampoline for newly created tasks.
 * Initial stack frame: r4 = fn, r5 = task_exit.
 * Sets LR so fn returns to task_exit.
 */
	.globl	task_entry
	.type	task_entry, %function
task_entry:
	cpsie	i			@ Ensure IRQs enabled (may start from IRQ context)
	mov	lr, r5
	bx	r4
	.size	task_entry, . - task_entry
