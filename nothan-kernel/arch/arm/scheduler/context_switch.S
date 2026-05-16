/*
 * arch/arm/scheduler/context_switch.S — low-level context switch
 *
 * Saves and restores the callee-saved register set (r3–r11, lr) and
 * the per-mode stack pointers (SP_svc and SP_usr/LR_usr) between two
 * task_struct instances.  TTBR0 is also switched when the incoming
 * task has its own page table (pgd_pa != 0).
 */

.global context_switch
.global start_first_task

.section .text

/* context_switch(struct task_struct *current, struct task_struct *next)
 * Saves r3–r11+lr (callee-saved) + SP to current, restores from next. */
context_switch:
    stmfd   sp!, {r3-r11, lr}
    str     sp, [r0, #52]      @ current->context.sp; offset matches task_struct layout
    dsb

    /* System Mode gives access to banked SP_usr and LR_usr. */
    cps     #0x1F               @ System Mode
    add     r2, r0, #64         @ context.sp_usr
    stmia   r2, {sp, lr}        @ save SP_usr + LR_usr
    cps     #0x13               @ SVC Mode

    /* Switch TTBR0 before loading next's SP: next's stack VA is only valid
     * in next's address space, and prev's stack is already saved. */
    ldr     r2, [r1, #116]      @ next->pgd_pa; 0 = kernel task, keep current TTBR0
    cmp     r2, #0
    beq     1f
    orr     r2, r2, #0x4A       @ Inner/Outer WB-WA + Shareable
    mcr     p15, 0, r2, c2, c0, 0
    dsb
    mov     r3, #0
    mcr     p15, 0, r3, c8, c7, 0
    dsb
    isb
1:

    ldr     sp, [r1, #52]

    cps     #0x1F               @ System Mode
    add     r2, r1, #64         @ context.sp_usr
    ldmia   r2, {sp, lr}        @ restore SP_usr + LR_usr
    cps     #0x13               @ SVC Mode

    ldmfd   sp!, {r3-r11, lr}

    /* Mode switch (SVC→USR) happens in svc_exit_trampoline, not here —
     * we return to svc_handler which then returns through the trampoline. */
    bx      lr

/* start_first_task(struct task_struct *first)
 * No current task — only restore, do not save. */
start_first_task:
    ldr     sp, [r0, #52]

    cps     #0x1F               @ System Mode
    add     r2, r0, #64         @ context.sp_usr
    ldmia   r2, {sp, lr}        @ restore SP_usr + LR_usr
    cps     #0x13               @ SVC Mode

    ldmfd   sp!, {r3-r11, lr}
    bx      lr

.end
