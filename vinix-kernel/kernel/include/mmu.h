/* ============================================================
 * mmu.h
 * ------------------------------------------------------------
 * MMU interface — 3G user / 1G kernel split.
 * ============================================================ */

/* VA layout:
 *   User    : 0x00000000 – 0xBFFFFFFF  (3GB)
 *   Kernel  : 0xC0000000 – 0xFFFFFFFF  (1GB)
 * DDR PA 0x80000000 (128MB) → kernel VA 0xC0000000.
 * Peripherals identity-mapped (PA == VA), kernel-only (AP=01). */

#ifndef MMU_H
#define MMU_H

#include "types.h"
#include "mach/memory.h"
#include "mach/memmap.h"

/* ============================================================
 * L1 Section Descriptor Bit Fields
 * ============================================================
 *
 * 31        20 19 18 17 16 15 14:12 11:10 9 8:5 4 3 2 1 0
 * [base addr ] NS  0  nG  S APX TEX  AP  imp Domain XN C B 1 0
 * ============================================================ */

#define MMU_DESC_SECTION (0x2) /* bits[1:0] = 10 → Section */

/* ============================================================
 * Access Permission (APX=0)
 * ============================================================ */
#define MMU_AP_KERN_RW (0x1 << 10)     /* AP=01: Kernel RW, User no-access */
#define MMU_AP_FULL_ACCESS (0x3 << 10) /* AP=11: Kernel RW, User RW */

/* ============================================================
 * Domain — bits[8:5], 16 domains (0–15)
 * ============================================================ */
#define MMU_DOMAIN(d) (((d) & 0xF) << 5)
#define MMU_DOMAIN_KERNEL 0
#define MMU_DOMAIN_USER 1

/* ============================================================
 * TEX, C, B — Memory Attributes (no TEX remap)
 * ============================================================ */
#define MMU_TEX(t) (((t) & 0x7) << 12)
#define MMU_CACHED (1 << 3)     /* C bit */
#define MMU_BUFFERED (1 << 2)   /* B bit */
#define MMU_SHAREABLE (1 << 16) /* S bit */
#define MMU_XN (1 << 4)         /* Execute-Never */

/* ============================================================
 * Composite Section Attributes
 * ============================================================ */

/* Strongly Ordered, Kernel-only, no execute (peripherals) */
#define MMU_SECT_PERIPHERAL \
    (MMU_DESC_SECTION | MMU_AP_KERN_RW | MMU_DOMAIN(MMU_DOMAIN_KERNEL) | MMU_TEX(0) | MMU_XN)

/* Normal Cached WB/WA, Kernel-only (kernel code + data) */
#define MMU_SECT_KERNEL_RAM \
    (MMU_DESC_SECTION | MMU_AP_KERN_RW | MMU_DOMAIN(MMU_DOMAIN_KERNEL) | MMU_TEX(1) | MMU_CACHED | MMU_BUFFERED | MMU_SHAREABLE)

/* Normal Cached WB/WA, Kernel+User RW (user memory) */
#define MMU_SECT_USER_RAM \
    (MMU_DESC_SECTION | MMU_AP_FULL_ACCESS | MMU_DOMAIN(MMU_DOMAIN_USER) | MMU_TEX(1) | MMU_CACHED | MMU_BUFFERED | MMU_SHAREABLE)

/* Normal Non-Cacheable, Kernel-only, XN (framebuffer — CPU writes, LCDC DMA reads)
 * Non-cacheable ensures DMA always reads fresh data from DDR without explicit flushes */
#define MMU_SECT_FB_RAM \
    (MMU_DESC_SECTION | MMU_AP_KERN_RW | MMU_DOMAIN(MMU_DOMAIN_KERNEL) | MMU_TEX(1) | MMU_XN)

/* ============================================================
 * DACR — Domain Access Control
 * ============================================================ */
#define DACR_NO_ACCESS 0x0
#define DACR_CLIENT 0x1  /* Enforce AP bits */
#define DACR_MANAGER 0x3 /* Bypass AP checks */

/* D0=Client, D1=Client, D2–15=No access → 0x00000005 */
#define MMU_DACR_VALUE \
    ((DACR_CLIENT << (MMU_DOMAIN_KERNEL * 2)) | (DACR_CLIENT << (MMU_DOMAIN_USER * 2)))

/* ============================================================
 * SCTLR Bits
 * ============================================================ */
#define SCTLR_M (1 << 0)  /* MMU enable */
#define SCTLR_A (1 << 1)  /* Alignment check */
#define SCTLR_C (1 << 2)  /* D-Cache enable */
#define SCTLR_Z (1 << 11) /* Branch prediction */
#define SCTLR_I (1 << 12) /* I-Cache enable */

/* ============================================================
 * Page Table Constants
 * ============================================================ */
#define MMU_L1_ENTRIES 4096
#define MMU_L1_ALIGN 16384        /* 16KB alignment */
#define MMU_SECTION_SIZE 0x100000 /* 1MB */
#define MMU_SECTION_SHIFT 20

/* ============================================================
 * 3G/1G Virtual Memory Layout
 * ============================================================ */

/* Kernel virtual address base (1GB kernel window) */
#define KERNEL_VA_BASE 0xC0000000

/* True User Space virtual address base */
#define USER_VA_BASE 0x40000000

#define DDR_PA_BASE PLATFORM_DDR_PA_BASE
#define DDR_SIZE_MB PLATFORM_DDR_SIZE_MB

/* PA ↔ VA conversion offset */
#define VA_OFFSET (KERNEL_VA_BASE - DDR_PA_BASE) /* 0x40000000 */

/* Convert between physical and virtual addresses (kernel space) */
#define PA_TO_VA(pa) ((pa) + VA_OFFSET)
#define VA_TO_PA(va) ((va) - VA_OFFSET)

/* Kernel DDR: The first 5MB is for Kernel */
#define KERNEL_DDR_VA KERNEL_VA_BASE
#define KERNEL_DDR_PA DDR_PA_BASE
#define KERNEL_DDR_MB 5

/* Page pool kernel-VA window.
 * PA 0x81000000..0x87FFFFFF (112 MB) reachable at VA 0xC1000000. */
#define POOL_KERNEL_VA_BASE  0xC1000000
#define POOL_KERNEL_PA_BASE  0x81000000
#define POOL_KERNEL_MB       112

/* User App DDR: Starts after Kernel (Memory pool)
 * PA 0x80500000 -> VA 0x40000000 (1MB for User App) */
#define USER_SPACE_PA (DDR_PA_BASE + (KERNEL_DDR_MB * MMU_SECTION_SIZE))
#define USER_SPACE_VA USER_VA_BASE
#define USER_SPACE_MB 1

/* Peripherals mapped PA == VA below 0xC0000000, kernel-only. */
#define PERIPH_L4_WKUP_PA        PLATFORM_PERIPH_L4_WKUP_PA
#define PERIPH_L4_WKUP_SECTIONS  PLATFORM_PERIPH_L4_WKUP_SECTIONS
#define PERIPH_L4_PER_PA         PLATFORM_PERIPH_L4_PER_PA
#define PERIPH_L4_PER_SECTIONS   PLATFORM_PERIPH_L4_PER_SECTIONS

#define FB_PA_BASE   PLATFORM_FB_PA_BASE
#define FB_SECTIONS  PLATFORM_FB_SIZE_MB

/* ============================================================
 * Boot-time Temporary Identity Mapping
 * ============================================================
 * During the MMU trampoline, we need both identity (PA==VA)
 * and high (PA→VA 0xC0xxxxxx) mappings for the DDR range.
 * After jumping to high VA, the identity map is removed.
 */
#define BOOT_IDENTITY_PA DDR_PA_BASE
#define BOOT_IDENTITY_MB DDR_SIZE_MB /* 128MB temporary */

/* ============================================================
 * L2 Small-Page Descriptor (4 KB page)
 * ============================================================
 *
 * 31..12 11 10  9  8..6 5..4  3 2 1 0
 * [PA  ] nG  S AP2 TEX  AP10  C B 1 XN
 *
 * Kernel RW cached WB/WA shareable: PA | 0x45E */
#define MMU_L2_SMALL_PAGE_TYPE   0x2        /* bit 1 = 1 */
#define MMU_L2_SMALL_KERN_RW     (MMU_L2_SMALL_PAGE_TYPE \
                                  | (0x1 << 4)           /* AP[1:0]=01 */ \
                                  | (0x1 << 6)           /* TEX=001   */ \
                                  | (1 << 3)             /* C         */ \
                                  | (1 << 2)             /* B         */ \
                                  | (1 << 10))           /* S         */

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Build L1 page table and enable MMU + caches.
 * Includes the trampoline: identity map → enable MMU →
 * jump to high VA → remove identity map.
 *
 * Must be called after uart_init(), before intc_init().
 * After this function returns, the kernel runs at VA 0xC0xxxxxx.
 */
void mmu_init(void);

/* Install an L1 page-table descriptor at the 1 MB section covering
 * section_va. The section must not currently hold a Section mapping
 * that is in use — caller owns the range.
 *
 * l2_table_va must be 1 KB aligned (256 entries * 4 bytes). */
void mmu_install_page_table(uint32_t section_va, uint32_t *l2_table_va,
                             uint32_t domain);

/* Flush the entire TLB + DSB/ISB. Used after updating page tables. */
void mmu_flush_tlb(void);

/* Returns PA of the shared kernel L1 table (used when a task has no
 * private pgd). */
uint32_t mmu_kernel_pgd_pa(void);

/* Switch TTBR0 to the given L1 table PA + flush the TLB. Caller must
 * hold IRQs off and be sure the new pgd contains valid kernel
 * mappings (mmu_new_pgd clones them). */
void mmu_switch_pgd(uint32_t pgd_pa);

/* Install a 1 MB user section entry in the given L1 table. */
void mmu_install_user_section(uint32_t *pgd_va, uint32_t user_va,
                               uint32_t user_pa);

/* Allocate a fresh L1 page table for a new address space.
 *   - 16 KB aligned (order-2 page allocation)
 *   - kernel VA range (0xC0000000+) mirrors the current pgd
 *   - peripheral + framebuffer identity entries copied so kernel-mode
 *     code still reaches I/O after a TTBR switch
 *   - user VA range (below 0xC0000000) left as Faults; caller fills
 *     user mappings when loading an ELF / forking
 * Returns PA of new pgd, or 0 on OOM. */
uint32_t mmu_new_pgd(void);

/* Release an L1 page table previously returned by mmu_new_pgd. */
void mmu_free_pgd(uint32_t pgd_pa);

#endif /* MMU_H */
