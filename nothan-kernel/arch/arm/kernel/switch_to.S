/*
 * arch/arm/kernel/switch_to.S - Low-level context switch for ARM
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
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

/*
 * user_task_trampoline - drop from SVC mode into ARM User mode (PL0)
 *
 * Called via __switch_to ldmfd with a pre-built kernel stack frame:
 *   r4 = user stack top VA   (initial sp for user task)
 *   r5 = user entry point VA (initial pc for user task)
 *
 * Strategy:
 *   1. Enter System mode (0x1F) — shares User-mode banked registers
 *      (sp_usr, lr_usr) without the privilege drop.
 *   2. Set sp_usr = r4 (user stack).
 *   3. Return to SVC mode.
 *   4. Set SPSR = 0x10 (User mode, IRQs enabled, Thumb off).
 *   5. movs pc, lr — exception-style return: CPSR = SPSR → User mode,
 *      PC = lr = r5 = user entry point.
 */
	.globl	user_task_trampoline
	.type	user_task_trampoline, %function
user_task_trampoline:
	cpsid	if			@ disable IRQs/FIQs during mode switch

	@ Set user-mode sp via System mode (shares sp_usr with User mode)
	cps	#0x1F			@ System mode
	mov	sp, r4			@ sp_usr = user stack top
	cps	#0x13			@ back to SVC mode

	@ Prepare for exception return to User mode
	mov	lr, r5			@ lr = user entry VA
	mov	r0, #0x10		@ User mode | IRQs on | FIQs on | ARM
	msr	spsr_cxsf, r0

	@ Zero out r0-r3 (clean user register state)
	mov	r0, #0
	mov	r1, #0
	mov	r2, #0
	mov	r3, #0
	mov	r4, #0
	mov	r5, #0

	@ "Exception return": loads CPSR from SPSR → User mode, PC = lr
	movs	pc, lr
	.size	user_task_trampoline, . - user_task_trampoline

