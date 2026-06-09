/*
 * arch/arm/mm/mmu.c - ARMv7 MMU setup (page tables, TTBR, DACR)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/printk.h>
#include <nothan/mm.h>
#include <asm/memory.h>
#include <asm/pgtable.h>
#include <asm/barrier.h>

#define MMU_OFFSET	(PAGE_OFFSET - PHYS_OFFSET)

extern u32 __pgd_start;
extern void __turn_mmu_on(u32 pgd_phys);

/* Symbols in .idmap.text section. */
extern u32 __idmap_start;
extern u32 __idmap_end;

static void map_section(u32 *pgd, u32 va, u32 pa, u32 flags)
{
	pgd[va >> 20] = (pa & 0xFFF00000) | PMD_TYPE_SECT | flags;
}

/**
 * mmu_init() - Initialize the MMU page tables and enable MMU
 *
 * Sets up a 1:1 map for the kernel image and identity map for .idmap.text,
 * configures MMIO device mappings, and then turns on the MMU.
 */
void mmu_init(void)
{
	u32 pgd_phys = (u32)&__pgd_start - MMU_OFFSET;
	u32 *pgd = (u32 *)pgd_phys;

	/* Zero entire L1 table. */
	for (unsigned int i = 0; i < 4096; i++)
		pgd[i] = 0;

	/* Kernel direct: VA 0xC0000000 -> PA 0x80000000. */
	for (unsigned int i = 0; i < 512; i++)
		map_section(pgd, 0xC0000000 + (i << 20), 0x80000000 + (i << 20),
			    MT_NORMAL | PMD_SECT_DOMAIN(DOMAIN_KERNEL) |
			    PMD_SECT_AP_RW);

	/* MMIO: L4_PER (32 MB: 0x48000000-0x49FFFFFF) */
	for (unsigned int i = 0; i < 32; i++)
		map_section(pgd, 0xF0000000 + (i << 20), L4_PER_BASE + (i << 20),
			    MT_DEVICE | PMD_SECT_DOMAIN(DOMAIN_IO) |
			    PMD_SECT_AP_RW | PMD_SECT_XN);

	/* MMIO: L4_WKUP (16 MB: 0x44E00000-0x44FFFFFF) */
	for (unsigned int i = 0; i < 16; i++)
		map_section(pgd, 0xF0E00000 + (i << 20), L4_WKUP_BASE + (i << 20),
			    MT_DEVICE | PMD_SECT_DOMAIN(DOMAIN_IO) |
			    PMD_SECT_AP_RW | PMD_SECT_XN);

	/* MMIO: L4_FAST (2 MB: 0x4A000000-0x4A1FFFFF) */
	for (unsigned int i = 0; i < 2; i++)
		map_section(pgd, 0xF2000000 + (i << 20), L4_FAST_BASE + (i << 20),
			    MT_DEVICE | PMD_SECT_DOMAIN(DOMAIN_IO) |
			    PMD_SECT_AP_RW | PMD_SECT_XN);

	/* Identity map for .idmap.text -- use phys addresses. */
	unsigned int i = ((u32)&__idmap_start - MMU_OFFSET) >> 20;
	while (i <= (((u32)&__idmap_end - MMU_OFFSET) >> 20)) {
		map_section(pgd, i << 20, i << 20,
			    MT_NORMAL | PMD_SECT_DOMAIN(DOMAIN_KERNEL) | PMD_SECT_AP_RW);
		i++;
	}

	dsb();

	__turn_mmu_on(pgd_phys);
}

void mmu_log_config(void)
{
	printk("[MMU] 3G/1G split: PAGE_OFFSET=0x%lx PHYS_OFFSET=0x%lx\n",
	       PAGE_OFFSET, PHYS_OFFSET);
	printk("[MMU] Kernel VA 0xC0000000 -> PA 0x80000000 (512 MB)\n");
	printk("[MMU] L4_PER  VA 0xF0000000 -> PA 0x48000000 (32 MB, Device)\n");
	printk("[MMU] L4_WKUP VA 0xF0E00000 -> PA 0x44E00000 (16 MB, Device)\n");
	printk("[MMU] L4_FAST VA 0xF2000000 -> PA 0x4A000000 (2 MB, Device)\n");
	printk("[MMU] DACR=0x%x (D%d=Mgr D%d=Client D%d=Client)\n",
	       DACR_INIT, DOMAIN_KERNEL, DOMAIN_USER, DOMAIN_IO);
	printk("[MMU] enabled\n");
}

/*
 * pgd_kva() - return kernel VA of the global L1 page table
 *
 * After MMU is on, the PGD physical address is biased by MMU_OFFSET.
 * We access it through the kernel direct-map.
 */
static inline u32 *pgd_kva(void)
{
	u32 pgd_phys = (u32)&__pgd_start - MMU_OFFSET;
	return (u32 *)((unsigned long)pgd_phys + (PAGE_OFFSET - PHYS_OFFSET));
}

/**
 * mmu_map_user() - install a user-space L2 mapping into the global L1 table
 * @mm: mm_struct with l2, l1_idx, code_pa, stack_pa, entry_va, sp_top set.
 *
 * Builds L2 entries:
 *   code page : user RO + executable, Normal WB/WA, nG
 *   stack page: user RW + XN,         Normal WB/WA, nG
 *
 * Then patches the global L1 table at l1_idx to point to the L2 table
 * and flushes the TLB.
 *
 * VA layout (both pages in same 1MB window, L1[l1_idx]):
 *   code  VA = l1_idx << 20 | 0x10000  → L2[0x10]
 *   stack VA = l1_idx << 20 | 0xFF000  → L2[0xFF], grows down from +0x1000
 */
void mmu_map_user(struct mm_struct *mm)
{
	u32 *l2  = mm->l2;
	u32 *pgd = pgd_kva();

	/* Zero L2 table (256 entries = 1 KB). */
	for (unsigned int i = 0; i < 256; i++)
		l2[i] = 0;

	/*
	 * L2 attribute flags (extended format, TRE=0 → C,B only for memory type):
	 *   AP1|nG = user RO,  AP1|AP0|nG = user RW
	 */
#define L2_ATTR_MEM     (PTE_SMALL_C | PTE_SMALL_B)
#define L2_USER_RO      (PTE_SMALL_AP_URO | PTE_SMALL_NG)
#define L2_USER_RW      (PTE_SMALL_AP_BOTH | PTE_SMALL_NG)

	/* Code page: RO from user, executable */
	unsigned int code_l2_idx = (mm->entry_va & 0x000FF000) >> 12;
	l2[code_l2_idx] = (mm->code_pa & 0xFFFFF000)
			  | PTE_TYPE_SMALL | L2_ATTR_MEM | L2_USER_RO;

	/* Stack page: RW from user, XN */
	unsigned int stack_l2_idx = ((mm->sp_top - 1) & 0x000FF000) >> 12;
	l2[stack_l2_idx] = (mm->stack_pa & 0xFFFFF000)
			   | PTE_TYPE_SMALL | PTE_SMALL_XN
			   | L2_ATTR_MEM | L2_USER_RW;

	/*
	 * Patch L1: install table pointer for this 1MB window.
	 * Domain = USER (1), type = TABLE (coarse page table, bits[1:0]=01).
	 */
	u32 l2_phys = (u32)(unsigned long)l2 - (PAGE_OFFSET - PHYS_OFFSET);
	u32 l1_entry = (l2_phys & 0xFFFFFC00)
		       | PMD_SECT_DOMAIN(DOMAIN_USER)
		       | PMD_TYPE_TABLE;

	printk("[MMU] user map: L1[%lu] -> L2@0x%lx, code@VA=0x%lx, stack_top@VA=0x%lx\n",
	       mm->l1_idx, (unsigned long)l2_phys,
	       mm->entry_va, mm->sp_top);

	pgd[mm->l1_idx] = l1_entry;
	dsb();

	/*
	 * ARMv7 requires explicit cache maintenance after translation table
	 * updates.  Clean L2 + PGD lines to PoC so the table walker sees them.
	 * (D-side MMU table walks may not hit D-cache unless cleaned).
	 */
	for (unsigned int i = 0; i < 1024; i += 64) {
		__asm__ __volatile__(
			"mcr p15, 0, %0, c7, c10, 1\n"
			: : "r" ((char *)l2 + i) : "memory");
	}
	dsb();

	__asm__ __volatile__(
		"mcr p15, 0, %0, c7, c10, 1\n"	/* DCCMVAC */
		"dsb\n"
		: : "r" (&pgd[mm->l1_idx]) : "memory");

	/* Flush TLB so walker re-fetches from PoC */
	__asm__ __volatile__(
		"mcr p15, 0, %0, c8, c7, 0\n"	/* TLBIALL */
		"dsb\n"
		"isb\n"
		: : "r" (0) : "memory");
}

