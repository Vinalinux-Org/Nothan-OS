/*
 * arch/arm/exceptions/vectors.S — ARM exception vector table
 *
 * The ARM vector table consists of eight 4-byte slots at fixed offsets
 * from VBAR.  Each slot holds a single instruction; only a branch or
 * LDR PC is practical here.  This file uses the LDR PC pattern, which
 * loads the handler address from a literal pool placed immediately after
 * the eight vector slots.  This allows handlers anywhere in the 32-bit
 * address space, which is required here because the exception_entry stubs
 * linked at kernel VA (0xC0xxxxxx) are not reachable with a 24-bit
 * branch-offset from the vector table's physical address.
 *
 * Table layout (16 words = 64 bytes):
 *   offset 0x00–0x1C : 8 × LDR PC instructions
 *   offset 0x20–0x3C : 8 × handler address words (literal pool)
 *
 * VBAR points at _start.  reset.S sets VBAR to this physical address
 * at boot; mmu_init() updates it to the corresponding virtual address.
 */

.section .vectors, "ax"
.global _start

_start:
    ldr     pc, _vec_reset           /* +0x00: Reset */
    ldr     pc, _vec_undef           /* +0x04: Undefined Instruction */
    ldr     pc, _vec_svc             /* +0x08: Supervisor Call */
    ldr     pc, _vec_pabort          /* +0x0C: Prefetch Abort */
    ldr     pc, _vec_dabort          /* +0x10: Data Abort */
    ldr     pc, _vec_reserved        /* +0x14: Reserved */
    ldr     pc, _vec_irq             /* +0x18: IRQ */
    ldr     pc, _vec_fiq             /* +0x1C: FIQ */

_vec_reset:     .word reset_handler
_vec_undef:     .word exception_entry_undefined
_vec_svc:       .word exception_entry_svc
_vec_pabort:    .word exception_entry_prefetch_abort
_vec_dabort:    .word exception_entry_data_abort
_vec_reserved:  .word 0
_vec_irq:       .word exception_entry_irq
_vec_fiq:       .word exception_entry_fiq