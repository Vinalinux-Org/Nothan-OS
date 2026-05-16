/*
 * arch/arm/exceptions/exception_entry.S — ARM exception entry stubs
 *
 * Each ARM exception type causes the CPU to enter a dedicated processor
 * mode with its own banked SP, LR, and SPSR.  The common pattern here
 * is: adjust LR to point at the faulting/interrupted instruction, push
 * the full integer register context plus SPSR onto the exception-mode
 * stack, switch to SVC mode, then call the C handler.
 *
 * LR values on exception entry (ARMv7-A architecture):
 *   Undefined instruction : LR = PC of undefined instruction + 4
 *   Prefetch Abort        : LR = faulting instruction address  + 4
 *   Data Abort            : LR = faulting instruction address  + 8
 *   IRQ                   : LR = next instruction address      + 4
 *   FIQ                   : unused in this kernel
 */

.global exception_entry_undefined
.global exception_entry_svc
.global exception_entry_prefetch_abort
.global exception_entry_data_abort
.global exception_entry_irq
.global exception_entry_fiq

exception_entry_undefined:
    sub     lr, lr, #4              @ correct LR to point at the faulting instruction
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, spsr
    stmfd   sp!, {r0}
    mov     r0, sp                  @ pass stack pointer as context to C handler
    cps     #0x13
    bl      handle_undefined_instruction
    b       .

exception_entry_svc:
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, spsr
    mov     r1, #0                  @ padding word for stack alignment
    stmfd   sp!, {r0, r1}

    /*
     * Re-enable IRQ before calling the C handler.  The ARM core sets
     * CPSR.I on SVC entry, so IRQs are masked for the duration of the
     * system call unless explicitly re-enabled here.  Leaving IRQs
     * masked across a context switch would carry the masked state into
     * the next task.  The ^ suffix on the return ldmfd restores the
     * caller's CPSR (including its I bit) from SPSR automatically.
     */
    cpsie   i

    mov     r0, sp
    bl      svc_handler

    .global svc_exit_trampoline
svc_exit_trampoline:
    ldmfd   sp!, {r0, r1}          @ pop SPSR + padding word
    msr     spsr_cxsf, r0
    ldmfd   sp!, {r0-r12, pc}^     @ restore registers; ^ copies SPSR → CPSR

exception_entry_prefetch_abort:
    sub     lr, lr, #4
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, spsr
    stmfd   sp!, {r0}
    mov     r0, sp
    cps     #0x13
    bl      handle_prefetch_abort
    b       .

exception_entry_data_abort:
    sub     lr, lr, #8              @ data abort LR is 8 bytes past the faulting load/store
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, spsr
    stmfd   sp!, {r0}
    mov     r0, sp
    cps     #0x13
    bl      handle_data_abort
    b       .

exception_entry_irq:
    sub     lr, lr, #4
    /*
     * srsdb saves {LR_irq, SPSR_irq} to the SVC stack before the mode
     * switch.  Saving to the destination stack first means there is no
     * window where a nested exception could lose these values.
     */
    srsdb   sp!, #0x13
    cps     #0x13
    stmfd   sp!, {r0-r12, lr}
    mov     r0, sp
    bl      irq_dispatch
    ldmfd   sp!, {r0-r12, lr}
    rfefd   sp!                     @ restore PC + CPSR from the srsdb frame

exception_entry_fiq:
    /* FIQ is not used by this kernel; arriving here indicates a bug. */
    sub     lr, lr, #4
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, spsr
    stmfd   sp!, {r0}
    mov     r0, sp
    cps     #0x13
    bl      handle_fiq
    b       .
