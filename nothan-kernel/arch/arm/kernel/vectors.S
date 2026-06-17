/*
 * arch/arm/kernel/vectors.S - ARM exception vector table and trampolines
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
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
/*
 * Stack layout after entry:
 *   push {r0-r12, lr} → 14 regs, ascending: r0=sp+4, r1=sp+8, …, r7=sp+32, …, lr=sp+56
 *   push {r0= SPSR}  → SPSR at sp+0
 *
 * Syscall convention (Linux style):
 *   r7 = syscall number
 *   r0-r3 = arguments
 *   svc #0   (immediate is ignored)
 *   r0 = return value
 */
vector_svc:
	push	{r0-r12, lr}		@ save registers (56 bytes)
	mrs	r0, spsr
	push	{r0}			@ save SPSR (4 bytes)

	@ Re-enable IRQs — hardware sets I-bit on SVC entry. Without this,
	@ long syscalls that wait on jiffies (FB_FLIP → wait_eof) hang because
	@ the timer tick never reaches scheduler_tick().
	cpsie	i

	@ --- Read syscall number from r7 (stack offset +32) ---
	ldr	r4, [sp, #32]		@ r4 = r7 = syscall number

	@ --- Call do_syscall(nr, arg0, arg1, arg2) ---
	mov	r0, r4			@ r0 = syscall nr
	ldr	r1, [sp, #4]		@ r1 = arg0 (original r0)
	ldr	r2, [sp, #8]		@ r2 = arg1 (original r1)
	ldr	r3, [sp, #12]		@ r3 = arg2 (original r2)
	bl	do_syscall

	@ --- Store return value into r0 slot on stack ---
	str	r0, [sp, #4]		@ [sp+4] → r0 on ldmfd

	@ --- Restore state and return ---
	pop	{r0}			@ r0 = SPSR
	msr	spsr_cxsf, r0
	ldmfd	sp!, {r0-r12, pc}^	@ restore regs, pc = lr, CPSR = SPSR

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

	@ Preemption check: if need_resched is set, switch tasks
	ldr	r0, =need_resched
	ldr	r0, [r0]
	cmp	r0, #0
	beq	.Lirq_restore

	bl	schedule
	cpsid	i			@ Re-disable interrupts for the restore sequence

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
