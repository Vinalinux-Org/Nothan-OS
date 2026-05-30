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
	b	vector_und
	b	vector_svc
	b	vector_pabt
	b	vector_dabt
	.word	0		@ reserved
	b	vector_irq
	b	vector_fiq

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
	sub	lr, lr, #4
	push	{r0-r12, lr}
	mrs	r0, spsr
	push	{r0}
	bl	irq_handler
	pop	{r0}
	msr	spsr_cxsf, r0
	ldmfd	sp!, {r0-r12, pc}^

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
