#ifndef _NOTHAN_PGTABLE_H
#define _NOTHAN_PGTABLE_H

/*
 * ARMv7-A short-descriptor page table.
 * L1: 4096 entries × 4B = 16KB. Section (1MB) or Table (L2 pointer).
 * L2: 256 entries × 4B = 1KB.  Small page (4KB).
 */

/* L1 entry type (bits[1:0]) */
#define PMD_TYPE_MASK		3
#define PMD_TYPE_FAULT		0
#define PMD_TYPE_TABLE		1
#define PMD_TYPE_SECT		2

/* L1 section descriptor bits */
#define PMD_SECT_C			(1 << 3)
#define PMD_SECT_B			(1 << 2)
#define PMD_SECT_DOMAIN(x)	((x) << 5)
#define PMD_SECT_XN			(1 << 4)
#define PMD_SECT_AP_RW		(1 << 10)
#define PMD_SECT_AP_USER	(2 << 10)
#define PMD_SECT_TEX(x)		((x) << 12)
#define PMD_SECT_NG			(1 << 17)
#define PMD_SECT_S			(1 << 16)
#define PMD_SECT_PXN		(1 << 19)

/* Memory attribute combinations (TEX, C, B) */
#define MT_NORMAL			(PMD_SECT_TEX(1) | PMD_SECT_C | PMD_SECT_B)
#define MT_DEVICE			(0)
#define MT_NONCACHE			(PMD_SECT_TEX(1))

/* L1 table descriptor (points to L2 page table) */
#define PMD_TABLE_PTR(x)	(((x) & 0xFFFFFC00) | PMD_TYPE_TABLE)

/* L2 small page descriptor bits — extended format (ARMv6+, ARMv7 short-desc) */
#define PTE_TYPE_MASK		3
#define PTE_TYPE_SMALL		(1 << 1)	/* bit[1]=1, bit[0]=XN (extended format) */
#define PTE_TYPE_LARGE		1
#define PTE_TYPE_FAULT		0

#define PTE_SMALL_XN		(1 << 0)	/* Execute Never */
#define PTE_SMALL_B			(1 << 2)	/* Bufferable / Outer cacheable */
#define PTE_SMALL_C			(1 << 3)	/* Cacheable / Inner cacheable */
#define PTE_SMALL_AP0		(1 << 4)	/* AP[0] */
#define PTE_SMALL_AP1		(1 << 5)	/* AP[1] */
#define PTE_SMALL_TEX(x)	((x) << 6)	/* TEX[2:0] at bits [8:6] */
#define PTE_SMALL_APX		(1 << 9)	/* AP extension */
#define PTE_SMALL_S			(1 << 10)	/* Shareable */
#define PTE_SMALL_NG		(1 << 11)	/* Not Global */

/* AP convenience combos (APX=0 unless overridden) */
#define PTE_SMALL_AP_RW		PTE_SMALL_AP0				/* AP=01: kernel RW only */
#define PTE_SMALL_AP_URO	PTE_SMALL_AP1				/* AP=10: kernel RW, user RO */
#define PTE_SMALL_AP_BOTH	(PTE_SMALL_AP0 | PTE_SMALL_AP1)	/* AP=11: kernel RW, user RW */

#define PTE_SMALL_PAGE(x)	(((x) & 0xFFFFF000) | PTE_TYPE_SMALL)

/* Domain numbers */
#define DOMAIN_KERNEL		0
#define DOMAIN_USER			1
#define DOMAIN_IO			2
#define DOMAIN_VECTORS		3

/* DACR: 2 bits per domain, 16 domains */
#define DACR_CLIENT(x)		(0x1 << ((x) * 2))
#define DACR_MANAGER(x)		(0x3 << ((x) * 2))
#define DACR_INIT			(DACR_MANAGER(DOMAIN_KERNEL) | \
							 DACR_CLIENT(DOMAIN_USER) | \
							 DACR_CLIENT(DOMAIN_IO) | \
							 DACR_CLIENT(DOMAIN_VECTORS))

/* MMIO base addresses */
#define L4_WKUP_BASE		0x44E00000
#define L4_PER_BASE			0x48000000
#define L4_FAST_BASE		0x4A000000

#endif /* _NOTHAN_PGTABLE_H */
