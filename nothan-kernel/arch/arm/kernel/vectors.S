/*
 * ARM exception vector table and trampolines.
 *
 * VBAR points here (set after MMU is on).  Each vector entry
 * branches to a trampoline that saves context and calls the
 * corresponding C handler in traps.c.
 *
 * Vector offsets (4 bytes each):
 *   0x00  Reset         0x08  SVC (syscall)
 *   0x04  Undefined     0x0C  Prefetch Abort
 *   0x10  Data Abort    0x14  Hypervisor (unused)
 *   0x18  IRQ           0x1C  FIQ
 */

	.section ".vectors", "ax"
	.align  5		@ 32-byte aligned for VBAR

	.globl	vectors
vectors:
	b	vector_reset	@ 0x00  Reset
	b	vector_und	@ 0x04  Undefined Instruction
	b	vector_svc	@ 0x08  Software Interrupt (SVC)
	b	vector_pabt	@ 0x0C  Prefetch Abort
	b	vector_dabt	@ 0x10  Data Abort
	.word	0		@ 0x14  Reserved (Hypervisor)
	b	vector_irq	@ 0x18  IRQ
	b	vector_fiq	@ 0x1C  FIQ

/* --- Reset (should not happen after boot) --- */
vector_reset:
	b	vector_reset	@ spin

/* --- Undefined instruction --- */
vector_und:
	sub	lr, lr, #0
	push	{r0-r12, lr}
	mrs	r0, spsr
	push	{r0}
	bl	und_handler
	pop	{r0}
	msr	spsr_cxsf, r0
	ldmfd	sp!, {r0-r12, pc}^

/* --- Supervisor call (syscall) --- */
vector_svc:
	sub	lr, lr, #0
	push	{r0-r12, lr}
	mrs	r0, spsr
	push	{r0}
	bl	svc_handler
	pop	{r0}
	msr	spsr_cxsf, r0
	ldmfd	sp!, {r0-r12, pc}^

/* --- Prefetch abort --- */
vector_pabt:
	sub	lr, lr, #4
	push	{r0-r12, lr}
	mrs	r0, spsr
	push	{r0}
	bl	pabt_handler
	pop	{r0}
	msr	spsr_cxsf, r0
	ldmfd	sp!, {r0-r12, pc}^

/* --- Data abort --- */
vector_dabt:
	sub	lr, lr, #8
	push	{r0-r12, lr}
	mrs	r0, spsr
	push	{r0}
	bl	dabt_handler
	pop	{r0}
	msr	spsr_cxsf, r0
	ldmfd	sp!, {r0-r12, pc}^

/* --- IRQ --- */
vector_irq:
	sub	lr, lr, #4		@ Fix return address
	srsdb	sp!, #0x13		@ Save SPSR_IRQ + LR_IRQ to SVC stack
	cpsid	if, #0x13		@ Switch to SVC mode (IRQs/FIQ off)
	push	{r0-r12, lr}		@ Save remaining regs on SVC stack

	bl	irq_handler

	@ NOTE: schedule() must NOT be called from IRQ context.
	@ __switch_to saves SP mid-frame → corrupts .Lirq_restore.
	@ scheduler_tick() sets need_resched=1; tasks call schedule()
	@ cooperatively from thread context.

.Lirq_restore:
	pop	{r0-r12, lr}		@ Restore interrupted task's registers and original lr_svc
	rfefd	sp!			@ Atomically pop PC and CPSR (saved by srsdb) and return

/* --- FIQ --- */
vector_fiq:
	sub	lr, lr, #4
	push	{r0-r7, lr}
	mrs	r0, spsr
	push	{r0}
	bl	fiq_handler
	pop	{r0}
	msr	spsr_cxsf, r0
	ldmfd	sp!, {r0-r7, pc}^

	.size	vectors, . - vectors
