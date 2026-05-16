/*
 * arch/arm/entry/reset.S — CPU reset handler
 *
 * This is the first code executed when the CPU comes out of reset or is
 * released from boot ROM.  The processor state on entry is undefined
 * except for what the ARM architecture guarantees: supervisor mode,
 * IRQ and FIQ are architecturally unmasked (CPSR.I and CPSR.F may be
 * set or clear depending on the core revision), and neither the MMU
 * nor the data cache are enabled.
 *
 * This handler performs the minimum safe pre-MMU initialization and
 * hands off to the boot trampoline in entry.S.
 */

/*
 * VA_OFFSET_BOOT — offset between kernel link VAs (0xC0000000) and the
 * physical SDRAM base (0x80000000).  All linker symbols (_*_stack_top,
 * _bss_start, etc.) are emitted as virtual addresses; subtracting this
 * constant converts them to the physical addresses accessible before
 * the MMU is on.  entry.S reloads the stacks to their VAs after the
 * MMU has been enabled.
 */
.equ VA_OFFSET_BOOT, 0x40000000   @ 0x01 ROR 2 — valid ARM immediate

    .section .text.reset, "ax"
    .align 4
    .global reset_handler

reset_handler:
    /*
     * Mask IRQ and FIQ unconditionally as the very first instruction.
     * The ARM core can take an interrupt as soon as the vector table is
     * set up; without masking, a spurious peripheral interrupt before
     * any stack exists would cause an unrecoverable exception.
     */
    cpsid   if

    /*
     * Force the processor into SVC32 mode (mode field = 0x13).
     * Writing CPSR via MRS/BIC/ORR/MSR preserves all other CPSR bits
     * (endianness, IT state, interrupt masks) unchanged.
     */
    mrs     r0, cpsr
    bic     r0, r0, #0x1F
    orr     r0, r0, #0x13
    msr     cpsr_c, r0

    /*
     * Set per-mode stack pointers to physical addresses.
     * All linker-generated _*_stack_top symbols are virtual; subtract
     * VA_OFFSET_BOOT to obtain the corresponding physical address while
     * the MMU is still off.
     *
     * All mode stacks must be initialized before writing VBAR below.
     * Once VBAR is set, any exception (including an IRQ caused by a
     * stray peripheral) will immediately attempt to use whichever
     * mode's stack was current — an uninitialized SP would cause a
     * cascading data abort with no recovery path.
     *
     * CPS mode field encoding (ARMv7-A):
     *   0x11 = FIQ   0x12 = IRQ   0x13 = SVC
     *   0x17 = Abort 0x1B = Undefined
     */
    cps     #0x11
    ldr     sp, =_fiq_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    cps     #0x12
    ldr     sp, =_irq_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    cps     #0x17
    ldr     sp, =_abt_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    cps     #0x1B
    ldr     sp, =_und_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    cps     #0x13
    ldr     sp, =_svc_stack_top
    sub     sp, sp, #VA_OFFSET_BOOT

    /*
     * Point VBAR at the physical address of the boot vector table.
     * _boot_start is placed in .text.boot (VMA == LMA == PA 0x80000000),
     * so no VA-to-PA adjustment is needed here.  mmu_init() will update
     * VBAR to the corresponding high virtual address after the permanent
     * kernel page tables are installed.
     */
    ldr     r0, =_boot_start
    mcr     p15, 0, r0, c12, c0, 0
    isb

    b       entry

    .extern _boot_start
    .extern _fiq_stack_top
    .extern _irq_stack_top
    .extern _abt_stack_top
    .extern _und_stack_top
    .extern _svc_stack_top
    .extern entry
