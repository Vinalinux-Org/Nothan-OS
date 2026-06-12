/*
 * bootloader/src/arch/arm/start.S - ARM vector table and CPU mode setup
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

	.section .vectors, "ax"
	.align 5		@ 32-byte alignment required by ARMv7 vector table
	.global _vectors

_vectors:
	ldr	pc, =_start		        @ reset
	ldr	pc, =_undef_handler	    @ undefined instruction
	ldr	pc, =_svc_handler	    @ SVC
	ldr	pc, =_prefetch_handler	@ prefetch abort
	ldr	pc, =_data_handler	    @ data abort
	ldr	pc, =_reserved_handler	@ reserved
	ldr	pc, =_irq_handler	    @ IRQ
	ldr	pc, =_fiq_handler	    @ FIQ

	.section .text.start, "ax"
	.align 4
	.global _start

_start:
	cpsid	if			@ mask IRQ + FIQ before any setup

	/*
	 * Set up exception mode stacks before setting VBAR. If an exception
	 * fires with an uninitialised SP the CPU will corrupt memory and hang.
	 */
	mrs	r0, cpsr
	bic	r0, r0, #0x1F

	orr	r1, r0, #0x11		@ FIQ mode
	msr	cpsr_c, r1
	ldr	sp, =_fiq_stack_top

	orr	r1, r0, #0x12		@ IRQ mode
	msr	cpsr_c, r1
	ldr	sp, =_irq_stack_top

	orr	r1, r0, #0x17		@ Abort mode
	msr	cpsr_c, r1
	ldr	sp, =_abt_stack_top

	orr	r1, r0, #0x1B		@ Undefined mode
	msr	cpsr_c, r1
	ldr	sp, =_und_stack_top

	orr	r1, r0, #0x1F		@ System mode
	msr	cpsr_c, r1
	ldr	sp, =_sys_stack_top

	orr	r1, r0, #0x13		@ SVC mode — main bootloader mode
	msr	cpsr_c, r1
	ldr	sp, =_svc_stack_top

	@ Disable WDT1 — ROM arms it; must stop it before stack/BSS setup
	ldr	r0, =0x44E35000		@ WDT1 base
	ldr	r1, =0xAAAA
	str	r1, [r0, #0x48]		@ WDT_WSPR
1:	ldr	r2, [r0, #0x34]		@ poll WDT_WWPS.W_PEND_WSPR (bit 4)
	tst	r2, #(1 << 4)
	bne	1b
	ldr	r1, =0x5555
	str	r1, [r0, #0x48]

	ldr	r0, =_vectors
	mcr	p15, 0, r0, c12, c0, 0	@ VBAR
	isb

	ldr	r0, =_bss_start
	ldr	r1, =_bss_end
	mov	r2, #0
1:	cmp	r0, r1
	strlo	r2, [r0], #4
	blo	1b

	bl	bootloader_main

halt:
	b	halt

	.section .text.handlers, "ax"
	.align 4

/*
 * Save AAPCS scratch registers (r0-r3, r12, lr) before calling C handlers.
 * Each exception mode has its own banked LR and SP set up in _start.
 */

_undef_handler:
	push	{r0-r3, r12, lr}
	bl	c_undef_handler
	pop	{r0-r3, r12, lr}
	movs	pc, lr			@ restore CPSR from SPSR
1:	b	1b

_svc_handler:
	b	_svc_handler

_prefetch_handler:
	push	{r0-r3, r12, lr}
	bl	c_prefetch_abort_handler
	pop	{r0-r3, r12, lr}
	subs	pc, lr, #4		@ retry faulting instruction
1:	b	1b

_data_handler:
	push	{r0-r3, r12, lr}
	bl	c_data_abort_handler
	pop	{r0-r3, r12, lr}
	subs	pc, lr, #8		@ retry faulting instruction
1:	b	1b

_reserved_handler:
	b	_reserved_handler

_irq_handler:
	b	_irq_handler

_fiq_handler:
	b	_fiq_handler

	.extern _bss_start
	.extern _bss_end
	.extern _svc_stack_top
	.extern _irq_stack_top
	.extern _fiq_stack_top
	.extern _abt_stack_top
	.extern _und_stack_top
	.extern _sys_stack_top
	.extern bootloader_main
	.extern c_undef_handler
	.extern c_prefetch_abort_handler
	.extern c_data_abort_handler
