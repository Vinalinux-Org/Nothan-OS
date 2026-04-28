/*
 * arch/arm/mm/mmu_enable.S — ARMv7-A MMU activation sequence
 *
 * Runs at physical address (called from the boot trampoline before the
 * MMU is on).  Sets TTBR0, TTBCR, and DACR, flushes the TLB, then
 * enables the MMU, D-cache, branch prediction, and I-cache in SCTLR
 * with a single MCR.  The identity map installed by
 * mmu_build_page_table_boot() keeps the fetch pipeline valid for the
 * few instructions between the SCTLR write and the ISB.
 */

    .section .text.mmu_trampoline, "ax"
    .align 4
    .global mmu_enable

/* mmu_enable(uint32_t *pgd_base) — r0: L1 table PA (16KB aligned). Clobbers r0, r1. */
mmu_enable:
    dsb

    /* Invalidate I-Cache — stale entries from MMU-off state
     * would have wrong cacheability attributes after MMU on */
    mov     r1, #0
    mcr     p15, 0, r1, c7, c5, 0      /* ICIALLU */
    dsb
    isb

    /* TTBR0 = page table base + table walk cacheability
     * 0x4A = Inner WB/WA (bit[6]=1), Outer WB/WA (bit[3]=1),
     *        Shareable (bit[1]=1) */
    orr     r0, r0, #0x4A
    mcr     p15, 0, r0, c2, c0, 0      /* Write TTBR0 */

    /* TTBCR = 0 → TTBR0 covers entire 4GB, short-descriptor */
    mov     r1, #0
    mcr     p15, 0, r1, c2, c0, 2      /* Write TTBCR */

    /* DACR: D0=Client, D1=Client, rest=No Access
     * Client enforces AP bits; Manager would bypass all checks */
    mov     r1, #0x5
    mcr     p15, 0, r1, c3, c0, 0      /* Write DACR */

    mov     r1, #0
    mcr     p15, 0, r1, c8, c7, 0      /* TLBIALL */
    dsb
    isb

    /* M+C must be set together — C without M is UNPREDICTABLE. */
    mrc     p15, 0, r1, c1, c0, 0      /* Read SCTLR */
    orr     r1, r1, #(1 << 0)          /* M — MMU */
    orr     r1, r1, #(1 << 2)          /* C — D-Cache */
    orr     r1, r1, #(1 << 11)         /* Z — Branch prediction */
    orr     r1, r1, #(1 << 12)         /* I — I-Cache */
    mcr     p15, 0, r1, c1, c0, 0      /* Write SCTLR */

    /* Pipeline sync — next fetch goes through MMU.
     * Identity map (PA→PA) keeps this code reachable. */
    isb

    bx      lr
