/*
 * void __switch_to(struct task_struct *prev, struct task_struct *next)
 *
 * r0 = prev (current task before switch)
 * r1 = next (task to switch to)
 *
 * Saves callee-saved registers (r4-r11) and return address of
 * the outgoing task onto its kernel stack, then restores the
 * incoming task's stack frame and jumps back into it.
 *
 * task_struct.stack is the kernel SP value; on context-out it
 * points to the saved frame, on context-in it is loaded into SP.
 */

	.text

	.globl	__switch_to
	.type	__switch_to, %function
__switch_to:
	stmfd	sp!, {r4-r11, lr}
	str	sp, [r0, #0]		/* prev->stack = current SP */
	ldr	sp, [r1, #0]		/* SP = next->stack */
	ldmfd	sp!, {r4-r11, pc}
	.size	__switch_to, . - __switch_to
