/*
 * arch/arm/mm/mmu.c — ARMv7-A MMU initialization
 *
 * Manages the kernel's first-level page table (L1, 4096 section entries).
 * Provides helpers to build the boot page table at physical address,
 * activate and tear down the identity map, and switch TTBR0 per task.
 */

#include "mmu.h"
#include "page_alloc.h"
#include "uart.h"
#include "types.h"

/* 4096 entries × 4 bytes = 16KB, must be 16KB aligned.
 * Placed in .pgd section (kernel VA space).
 * Boot code accesses this at PA via mmu_build_page_table_boot().
 *
 * 'used' attribute prevents compiler from eliminating this array:
 * mmu_build_page_table_boot() populates it through a PA pointer,
 * and mmu_init() accesses it via this static symbol at VA.
 * Without 'used', compiler considers pgd dead code. */
static uint32_t pgd[MMU_L1_ENTRIES]
    __attribute__((aligned(MMU_L1_ALIGN), section(".pgd"), used));

extern uint32_t _boot_start; /* PA 0x80000000 — vector table */

void mmu_init(void)
{
    uint32_t i, pa;

    for (i = 0; i < BOOT_IDENTITY_MB; i++)
    {
        pa = BOOT_IDENTITY_PA + (i * MMU_SECTION_SIZE);
        pgd[pa >> MMU_SECTION_SHIFT] = 0; /* FAULT */
    }

    /* Re-add framebuffer mapping — cleared by identity map removal above
     * (FB_PA_BASE falls within the 128MB DDR identity range) */
    for (i = 0; i < FB_SECTIONS; i++)
    {
        pa = FB_PA_BASE + (i * MMU_SECTION_SIZE);
        pgd[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_FB_RAM;
    }

    __asm__ __volatile__(
        "mov r0, #0\n\t"
        "mcr p15, 0, r0, c8, c7, 0\n\t" /* TLBIALL */
        "dsb\n\t"
        "isb\n\t" ::: "r0", "memory");

    /* VBAR: same physical memory now reachable at VA 0xC0000000 via high map. */
    uint32_t vbar_va = (uint32_t)&_boot_start + VA_OFFSET;

    __asm__ __volatile__(
        "mcr p15, 0, %0, c12, c0, 0\n\t" /* Write VBAR */
        "isb\n\t" ::"r"(vbar_va) : "memory");

    pr_info("[MMU] True 3G/1G Virtual Memory Split\n");
    pr_info("[MMU] User Space:  VA 0x%x -> PA 0x%x (%d MB) [Cached, User RW]\n",
                USER_SPACE_VA, USER_SPACE_PA, USER_SPACE_MB);
    pr_info("[MMU] Kernel DDR:  VA 0x%x -> PA 0x%x (%d MB) [Cached, Kernel-only]\n",
                KERNEL_DDR_VA, KERNEL_DDR_PA, KERNEL_DDR_MB);
    pr_info("[MMU] Page Pool:   VA 0x%x -> PA 0x%x (%d MB) [Cached, Kernel-only]\n",
                POOL_KERNEL_VA_BASE, POOL_KERNEL_PA_BASE, POOL_KERNEL_MB);
    pr_info("[MMU] Peripheral L4_WKUP: PA 0x%x (%d MB) [Strongly Ordered, Identity]\n",
                PERIPH_L4_WKUP_PA, PERIPH_L4_WKUP_SECTIONS);
    pr_info("[MMU] Peripheral L4_PER:  PA 0x%x (%d MB) [Strongly Ordered, Identity]\n",
                PERIPH_L4_PER_PA, PERIPH_L4_PER_SECTIONS);
    pr_info("[MMU] Peripheral USB0:    PA 0x%x (%d MB) [Strongly Ordered, Identity]\n",
                PERIPH_USB0_PA, PERIPH_USB0_SECTIONS);
    pr_info("[MMU] Framebuffer:        PA 0x%x – 0x%x (%d MB) [Non-Cacheable, Identity]\n",
                FB_PA_BASE, FB_PA_BASE + (FB_SECTIONS * MMU_SECTION_SIZE) - 1, FB_SECTIONS);
    pr_info("[MMU] Identity mapping removed (VA 0x80000000 now unmapped)\n");
    pr_info("[MMU] VBAR = 0x%x\n", vbar_va);
    pr_info("[MMU] DACR = 0x%x (D0=CLIENT, D1=CLIENT)\n", MMU_DACR_VALUE);
    pr_info("[MMU] MMU enabled, running at high VA!\n");
}

void mmu_flush_tlb(void)
{
    __asm__ __volatile__(
        "mov r0, #0\n\t"
        "mcr p15, 0, r0, c8, c7, 0\n\t"   /* TLBIALL */
        "dsb\n\t"
        "isb\n\t" ::: "r0", "memory");
}

void mmu_install_page_table(uint32_t section_va, uint32_t *l2_table_va,
                             uint32_t domain)
{
    uint32_t l2_pa = (uint32_t)l2_table_va - VA_OFFSET;
    uint32_t idx   = section_va >> MMU_SECTION_SHIFT;

    pgd[idx] = (l2_pa & 0xFFFFFC00) | ((domain & 0xF) << 5) | 0x01;

    __asm__ __volatile__("dsb\n\t" ::: "memory");
    mmu_flush_tlb();
}

/* Kernel-space start index: sections >= this mirror the current pgd. */
#define PGD_KERNEL_START_IDX  (KERNEL_VA_BASE >> MMU_SECTION_SHIFT)

uint32_t mmu_new_pgd(void)
{
    /* Order-2 = 4 pages = 16 KB, which is also the TTBR alignment. */
    uint32_t pa = alloc_pages(GFP_KERNEL, 2);
    if (pa == 0)
    {
        return 0;
    }

    uint32_t *new_pgd = (uint32_t *)(pa + VA_OFFSET);

    for (uint32_t i = 0; i < PGD_KERNEL_START_IDX; i++)
    {
        new_pgd[i] = 0;
    }
    for (uint32_t i = PGD_KERNEL_START_IDX; i < MMU_L1_ENTRIES; i++)
    {
        new_pgd[i] = pgd[i];
    }

    /* Peripheral + framebuffer identity entries sit in user VA range
     * but must remain accessible from kernel mode after a TTBR switch. */
    uint32_t wkup_idx = PERIPH_L4_WKUP_PA >> MMU_SECTION_SHIFT;
    new_pgd[wkup_idx] = pgd[wkup_idx];

    for (uint32_t i = 0; i < PERIPH_L4_PER_SECTIONS; i++)
    {
        uint32_t idx = (PERIPH_L4_PER_PA >> MMU_SECTION_SHIFT) + i;
        new_pgd[idx] = pgd[idx];
    }

    for (uint32_t i = 0; i < PERIPH_USB0_SECTIONS; i++)
    {
        uint32_t idx = (PERIPH_USB0_PA >> MMU_SECTION_SHIFT) + i;
        new_pgd[idx] = pgd[idx];
    }

    for (uint32_t i = 0; i < FB_SECTIONS; i++)
    {
        uint32_t idx = (FB_PA_BASE >> MMU_SECTION_SHIFT) + i;
        new_pgd[idx] = pgd[idx];
    }

    return pa;
}

void mmu_free_pgd(uint32_t pgd_pa)
{
    free_pages(pgd_pa, 2);
}

uint32_t mmu_kernel_pgd_pa(void)
{
    return (uint32_t)pgd - VA_OFFSET;
}

void mmu_switch_pgd(uint32_t pgd_pa)
{
    /* Same TTBR0 attributes as boot: Inner/Outer WB/WA + Shareable (0x4A). */
    uint32_t ttbr0 = pgd_pa | 0x4A;

    __asm__ __volatile__(
        "mcr p15, 0, %0, c2, c0, 0\n\t"   /* TTBR0 */
        "dsb\n\t"
        "mov r0, #0\n\t"
        "mcr p15, 0, r0, c8, c7, 0\n\t"   /* TLBIALL */
        "dsb\n\t"
        "isb\n\t"
        :: "r" (ttbr0) : "r0", "memory");
}

void mmu_install_user_section(uint32_t *pgd_va, uint32_t user_va,
                               uint32_t user_pa)
{
    uint32_t idx = user_va >> MMU_SECTION_SHIFT;
    pgd_va[idx] = user_pa | MMU_SECT_USER_RAM;
    __asm__ __volatile__("dsb\n\t" ::: "memory");
}

/*
 * mmu_build_page_table_boot - populate the L1 page table before MMU enable
 * @pgd_pa: physical address of the 4096-entry L1 table (16KB aligned)
 *
 * Called from the boot trampoline while the MMU is off.  Must reside in
 * .text.boot_entry so that its VMA equals its LMA (physical address).
 * The pgd_pa argument is the physical address of the table; the VA symbol
 * _pgd_start is not usable here because the MMU is not yet active.
 *
 * Mappings installed:
 *   PA 0x80000000 → VA 0xC0000000  (kernel DDR, cached, kernel-only)
 *   PA 0x80000000 → VA 0x80000000  (identity map, temporary, removed by mmu_init)
 *   PA 0x44E00000 → VA 0x44E00000  (L4_WKUP peripherals, strongly ordered)
 *   PA 0x48000000 → VA 0x48000000  (L4_PER peripherals, strongly ordered)
 *   framebuffer PA → same VA        (normal non-cacheable)
 */
void __attribute__((section(".text.boot_entry")))
mmu_build_page_table_boot(uint32_t *pgd_pa)
{
    uint32_t i;
    uint32_t pa, va_idx;

    for (i = 0; i < MMU_L1_ENTRIES; i++)
    {
        pgd_pa[i] = 0;
    }

    /* L4_WKUP: UART0, CM_PER, WDT1 (1MB) */
    for (i = 0; i < PERIPH_L4_WKUP_SECTIONS; i++)
    {
        pa = PERIPH_L4_WKUP_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_PERIPHERAL;
    }

    /* L4_PER: INTC, DMTimer, GPIO (4MB) */
    for (i = 0; i < PERIPH_L4_PER_SECTIONS; i++)
    {
        pa = PERIPH_L4_PER_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_PERIPHERAL;
    }

    /* USB0: MUSB host controller (1MB) */
    for (i = 0; i < PERIPH_USB0_SECTIONS; i++)
    {
        pa = PERIPH_USB0_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_PERIPHERAL;
    }

    /* Framebuffer: identity mapped, normal non-cacheable (CPU writes, LCDC DMA reads) */
    for (i = 0; i < FB_SECTIONS; i++)
    {
        pa = FB_PA_BASE + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_FB_RAM;
    }

    for (i = 0; i < USER_SPACE_MB; i++)
    {
        pa = USER_SPACE_PA + (i * MMU_SECTION_SIZE);
        va_idx = (USER_SPACE_VA + (i * MMU_SECTION_SIZE)) >> MMU_SECTION_SHIFT;
        pgd_pa[va_idx] = pa | MMU_SECT_USER_RAM;
    }

    for (i = 0; i < KERNEL_DDR_MB; i++)
    {
        pa = KERNEL_DDR_PA + (i * MMU_SECTION_SIZE);
        va_idx = (KERNEL_DDR_VA + (i * MMU_SECTION_SIZE)) >> MMU_SECTION_SHIFT;
        pgd_pa[va_idx] = pa | MMU_SECT_KERNEL_RAM;
    }

    /* Page pool: VA 0xC1000000 -> PA 0x81000000 (112 MB, Kernel-only) */
    for (i = 0; i < POOL_KERNEL_MB; i++)
    {
        pa = POOL_KERNEL_PA_BASE + (i * MMU_SECTION_SIZE);
        va_idx = (POOL_KERNEL_VA_BASE + (i * MMU_SECTION_SIZE)) >> MMU_SECTION_SHIFT;
        pgd_pa[va_idx] = pa | MMU_SECT_KERNEL_RAM;
    }

    /* Temporary identity map: PA 0x80000000 → VA 0x80000000
     * Allows CPU to continue executing at PA after MMU enable.
     * Removed later by mmu_init() once we are at high VA. */
    for (i = 0; i < BOOT_IDENTITY_MB; i++)
    {
        pa = BOOT_IDENTITY_PA + (i * MMU_SECTION_SIZE);
        pgd_pa[pa >> MMU_SECTION_SHIFT] = pa | MMU_SECT_KERNEL_RAM;
    }
}
