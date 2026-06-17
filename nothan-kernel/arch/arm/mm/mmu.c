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
	/* Zero L2 table (256 entries = 1 KB). */
	for (unsigned int i = 0; i < 256; i++)
		l2[i] = 0;

	/*
	 * L2 attribute flags (extended format, TRE=0 → C,B only for memory type):
	 *   AP1|nG = user RO,  AP1|AP0|nG = user RW
	 */
#define L2_ATTR_MEM     (PTE_SMALL_C | PTE_SMALL_B)
#define L2_USER_RW      (PTE_SMALL_AP_BOTH | PTE_SMALL_NG)

	/*
	 * Code pages start at VA 0x10000 (L2 index 0x10). entry_va is 0x10010
	 * (after the binary header), so mask off the low 12 bits.
	 */
	unsigned int code_l2_idx = (mm->entry_va & 0x000FF000) >> 12;
	for (unsigned int i = 0; i < mm->code_pages; i++) {
		unsigned long pa = mm->code_pa + (i << 12);
		l2[code_l2_idx + i] = (pa & 0xFFFFF000)
			  | PTE_TYPE_SMALL | L2_ATTR_MEM | L2_USER_RW;
	}

	/* BSS pages: RW + XN, placed right after the code pages */
	unsigned int bss_l2_idx = code_l2_idx + mm->code_pages;
	for (unsigned int i = 0; i < mm->bss_pages; i++) {
		unsigned long pa = mm->bss_pa + (i << 12);
		l2[bss_l2_idx + i] = (pa & 0xFFFFF000)
			  | PTE_TYPE_SMALL | PTE_SMALL_XN
			  | L2_ATTR_MEM | L2_USER_RW;
	}

	/*
	 * Stack pages: RW + XN, mapped right below sp_top.
	 * stack_pa is the lowest stack page (highest VA index = sp_top - 1).
	 */
	unsigned int stack_top_idx = ((mm->sp_top - 1) & 0x000FF000) >> 12;
	for (unsigned int i = 0; i < mm->stack_pages; i++) {
		unsigned long pa = mm->stack_pa + (i << 12);
		l2[stack_top_idx - (mm->stack_pages - 1) + i] =
			(pa & 0xFFFFF000)
			| PTE_TYPE_SMALL | PTE_SMALL_XN
			| L2_ATTR_MEM | L2_USER_RW;
	}

	/* Save L2 physical address for context-switch by schedule() */
	mm->l2_pa = (u32)(unsigned long)l2 - (PAGE_OFFSET - PHYS_OFFSET);

	/* Cache maintenance: clean L2 table to PoC */
	for (unsigned int i = 0; i < 1024; i += 64)
		__asm__ __volatile__(
			"mcr p15, 0, %0, c7, c10, 1\n"
			: : "r" ((char *)l2 + i) : "memory");
	dsb();
}

/*
 * mmu_switch_mm() - switch L1[0] to a task's L2 table
 * @mm: the new task's mm_struct (NULL clears user mapping)
 *
 * Called during schedule() after __switch_to.  Writes L1[0] in the
 * global PGD to point to @mm->l2_pa so the new task's user pages
 * are visible.  Flushes the TLB after the change.
 */
void mmu_switch_mm(struct mm_struct *mm)
{
	u32 *pgd = pgd_kva();

	if (mm && mm->l2_pa) {
		u32 l1_entry = (mm->l2_pa & 0xFFFFFC00)
			     | PMD_SECT_DOMAIN(DOMAIN_USER)
			     | PMD_TYPE_TABLE;
		pgd[0] = l1_entry;
	} else {
		pgd[0] = PMD_TYPE_FAULT;
	}
	dsb();

	/* Clean PGD entry + flush TLB */
	__asm__ __volatile__("mcr p15, 0, %0, c7, c10, 1\n" : : "r" (&pgd[0]) : "memory");
	dsb();
	__asm__ __volatile__("mcr p15, 0, %0, c8, c7, 0\n" "dsb\n" "isb\n" : : "r"(0) : "memory");
}
