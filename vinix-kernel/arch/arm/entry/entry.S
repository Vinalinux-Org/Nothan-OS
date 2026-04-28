/*
 * arch/arm/entry/entry.S — kernel boot trampoline
 *
 * Entered immediately after reset_handler completes its early-boot setup.
 * At this point the CPU is in SVC mode with IRQ and FIQ masked, the MMU
 * and data cache are both off, and all exception-mode stacks have been
 * set to their physical addresses by reset.S.
 *
 * Responsibilities:
 *   - Zero-fill .bss at physical address (C runtime requirement)
 *   - Build the initial page table and activate the MMU
 *   - Reload all per-mode stack pointers to their virtual addresses
 *   - Branch to kernel_main in high virtual address space
 */

    .section .text.boot_entry, "ax"
    .align  4
    .global entry

/*
 * VA_OFFSET_BOOT — the difference between the kernel virtual base
 * (0xC0000000) and the physical load address (0x80000000).
 * Used as an immediate subtraction operand to convert linker VAs to
 * physical addresses before the MMU is active.
 */
.equ VA_OFFSET_BOOT, 0x40000000

entry:
    /*
     * Zero .bss before any C code runs.  The linker emits _bss_start
     * and _bss_end as virtual addresses (0xC0xxxxxx); the MMU is still
     * off, so subtract VA_OFFSET_BOOT to obtain the physical addresses
     * the SDRAM cells actually sit at.
     */
    ldr     r0, =_bss_start
    sub     r0, r0, #VA_OFFSET_BOOT
    ldr     r1, =_bss_end
    sub     r1, r1, #VA_OFFSET_BOOT
    mov     r2, #0

1:  cmp     r0, r1
    strlo   r2, [r0], #4
    blo     1b

    /*
     * Build the first-level page table at its physical address.
     * mmu_build_page_table_boot() creates two mappings:
     *   - identity map:  PA 0x80000000 → VA 0x80000000  (keeps PC valid
     *                    for the few instructions after MCR TTBR0)
     *   - kernel map:    PA 0x80000000 → VA 0xC0000000  (permanent high map)
     *
     * _pgd_start is a linker VA; subtract VA_OFFSET_BOOT for the PA.
     */
    ldr     r0, =_pgd_start
    sub     r0, r0, #VA_OFFSET_BOOT
    bl      mmu_build_page_table_boot

    ldr     r0, =_pgd_start
    sub     r0, r0, #VA_OFFSET_BOOT
    bl      mmu_enable
    /*
     * MMU is now on.  Both the identity and high maps are live, so the
     * CPU continues fetching from the physical address range via the
     * identity map while addresses like stack symbols are already valid
     * at their virtual equivalents.
     */

    /*
     * Switch every ARM exception mode to its virtual-address stack.
     * This must happen before mmu_init() tears down the identity map:
     * once the identity mapping is removed, any SP that still holds a
     * physical address will point into unmapped memory, causing an
     * immediate data abort on the first stack access.
     *
     * CPS mode field encoding (ARMv7-A):
     *   0x11 = FIQ   0x12 = IRQ   0x13 = SVC
     *   0x17 = Abort 0x1B = Undefined
     */
    cps     #0x11
    ldr     sp, =_fiq_stack_top

    cps     #0x12
    ldr     sp, =_irq_stack_top

    cps     #0x17
    ldr     sp, =_abt_stack_top

    cps     #0x1B
    ldr     sp, =_und_stack_top

    cps     #0x13
    ldr     sp, =_svc_stack_top

    /*
     * Transfer control to kernel_main at its high virtual address.
     * ldr pc loads the VA from the literal pool while the CPU is still
     * at a physical PC; the instruction fetch for kernel_main then
     * proceeds through the high map, completing the PA→VA switch.
     */
    ldr     pc, =kernel_main

halt:
    b       halt

    .extern _bss_start
    .extern _bss_end
    .extern _pgd_start
    .extern mmu_build_page_table_boot
    .extern mmu_enable
    .extern _fiq_stack_top
    .extern _irq_stack_top
    .extern _abt_stack_top
    .extern _und_stack_top
    .extern _svc_stack_top
    .extern kernel_main
