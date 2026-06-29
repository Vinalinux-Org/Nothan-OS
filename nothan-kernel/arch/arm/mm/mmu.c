/*
 * arch/arm/mm/mmu.c - ARMv7 MMU setup (page tables, TTBR, DACR)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/printk.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
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

	/* MMIO: USB subsystem (1 MB: 0x47400000-0x474FFFFF) — USBSS + usb0/usb1
	 * MUSB cores + PHYs. Sits below L4_PER, so it needs its own window. */
	map_section(pgd, 0xF3000000, 0x47400000,
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

/**
 * mmu_log_config() - Print the current MMU mapping configuration
 *
 * Logs the virtual-to-physical address mapping layout for kernel,
 * MMIO regions, and DACR domain configuration to the kernel console.
 */
void mmu_log_config(void)
{
	printk("[MMU] 3G/1G split: PAGE_OFFSET=0x%lx PHYS_OFFSET=0x%lx\n",
	       PAGE_OFFSET, PHYS_OFFSET);
	printk("[MMU] Kernel VA 0xC0000000 -> PA 0x80000000 (512 MB)\n");
	printk("[MMU] L4_PER  VA 0xF0000000 -> PA 0x48000000 (32 MB, Device)\n");
	printk("[MMU] L4_WKUP VA 0xF0E00000 -> PA 0x44E00000 (16 MB, Device)\n");
	printk("[MMU] L4_FAST VA 0xF2000000 -> PA 0x4A000000 (2 MB, Device)\n");
	printk("[MMU] USB     VA 0xF3000000 -> PA 0x47400000 (1 MB, Device)\n");
	printk("[MMU] DACR=0x%x (D%d=Mgr D%d=Client D%d=Client)\n",
	       DACR_INIT, DOMAIN_KERNEL, DOMAIN_USER, DOMAIN_IO);
	printk("[MMU] enabled\n");
}

/*
 * pgd_kva() - return kernel VA of the global L1 page table (swapper)
 *
 * After MMU is on, the PGD physical address is biased by MMU_OFFSET.
 * We access it through the kernel direct-map. This master table holds
 * the kernel-half entries every process page table shares.
 */
static inline u32 *pgd_kva(void)
{
	u32 pgd_phys = (u32)&__pgd_start - MMU_OFFSET;
	return (u32 *)((unsigned long)pgd_phys + (PAGE_OFFSET - PHYS_OFFSET));
}

/* ===================================================================
 * Per-process page tables
 *
 * Each user task owns a private 16 KB L1 (@mm->pgd). The kernel half
 * (L1 indices >= 0xC00, i.e. VA >= 0xC0000000) is copied from the
 * swapper table so kernel/MMIO mappings stay valid while the task's
 * TTBR0 is active. The user half is built from 1 KB L2 tables, one per
 * touched 1 MB window. A context switch loads @mm->pgd_pa into TTBR0.
 * =================================================================== */

#define USER_L1_END	0xC00		/* first kernel L1 index (VA 0xC0000000) */
/* USER_CODE_VA now lives in <nothan/mm.h> (shared with uaccess). */
#define TTBR0_FLAGS	0x4A		/* walk attributes — same as boot TTBR0 */

/*
 * Normal, Inner+Outer Write-Back Write-Allocate (TEX=001, C=1, B=1) — must
 * MATCH the kernel's MT_NORMAL section attributes for RAM. The kernel writes
 * a task's code/bss through its own cacheable kernel mapping; if the user
 * mapping of the same physical page used a different memory type the two
 * views are an architecturally "mismatched memory attribute" pair.
 */
#define L2_ATTR_MEM	(PTE_SMALL_TEX(1) | PTE_SMALL_C | PTE_SMALL_B)
#define L2_USER_RW	(PTE_SMALL_AP_BOTH | PTE_SMALL_NG)
#define PTE_USER_CODE	(L2_ATTR_MEM | L2_USER_RW)			/* RW + exec */
#define PTE_USER_DATA	(PTE_SMALL_XN | L2_ATTR_MEM | L2_USER_RW)	/* RW + XN  */

static inline unsigned long kva_to_phys(void *kva)
{
	return (unsigned long)kva - MMU_OFFSET;
}

/* Clean a page-table region to PoC so the MMU table walker sees writes. */
static inline void pt_clean(void *addr, unsigned int size)
{
	clean_dcache_range((unsigned long)addr, (unsigned long)addr + size);
}

/**
 * pgd_alloc() - allocate a private L1 table for @mm
 *
 * 16 KB, naturally 16 KB-aligned (buddy order 2) as TTBR0 requires.
 * User half is zeroed; kernel half is copied from the swapper table.
 */
int pgd_alloc(struct mm_struct *mm)
{
	struct zone *zone = get_zone();
	struct page *pg = alloc_pages(GFP_KERNEL, 2);	/* 4 pages = 16 KB */
	if (!pg) {
		printk("[MMU] pgd_alloc: out of memory\n");
		return -1;
	}

	unsigned long pa = page_to_phys(zone, pg);
	u32 *pgd = (u32 *)phys_to_kva(pa);
	u32 *swapper = pgd_kva();

	if (pa & 0x3FFF)
		printk("[MMU] WARNING pgd_pa 0x%lx not 16KB-aligned\n", pa);

	for (unsigned int i = 0; i < USER_L1_END; i++)
		pgd[i] = 0;				/* user half: empty */
	for (unsigned int i = USER_L1_END; i < 4096; i++)
		pgd[i] = swapper[i];			/* kernel half: shared */

	pt_clean(pgd, 16384);

	mm->pgd    = pgd;
	mm->pgd_pa = pa;
	mm->nr_l2  = 0;
	return 0;
}

/* Find the L2 table for @l1_idx in @mm, creating + installing one if needed. */
static u32 *l2_for(struct mm_struct *mm, unsigned int l1_idx)
{
	for (unsigned int i = 0; i < mm->nr_l2; i++)
		if (mm->l2s[i].l1_idx == l1_idx)
			return mm->l2s[i].l2;

	if (mm->nr_l2 >= MM_MAX_L2) {
		printk("[MMU] l2_for: out of L2 slots (l1_idx=0x%x)\n", l1_idx);
		return NULL;
	}

	/* 1 KB slab class is 1 KB-aligned, which L2 coarse tables require. */
	u32 *l2 = (u32 *)kmalloc(1024, GFP_KERNEL);
	if (!l2) {
		printk("[MMU] l2_for: kmalloc failed\n");
		return NULL;
	}
	for (unsigned int i = 0; i < 256; i++)
		l2[i] = 0;

	unsigned long l2_pa = kva_to_phys(l2);
	mm->pgd[l1_idx] = (l2_pa & 0xFFFFFC00)
			| PMD_SECT_DOMAIN(DOMAIN_USER) | PMD_TYPE_TABLE;
	pt_clean(&mm->pgd[l1_idx], 4);

	mm->l2s[mm->nr_l2].l2     = l2;
	mm->l2s[mm->nr_l2].l1_idx = l1_idx;
	mm->nr_l2++;
	return l2;
}

/* Map @npages of [@pa..] at user VA [@va..] with @pte_flags. */
static int map_user_range(struct mm_struct *mm, unsigned long va,
			  unsigned long pa, unsigned int npages, u32 pte_flags)
{
	for (unsigned int i = 0; i < npages; i++) {
		u32 *l2 = l2_for(mm, va >> 20);
		if (!l2)
			return -1;
		l2[(va >> 12) & 0xFF] = (pa & 0xFFFFF000) | PTE_TYPE_SMALL | pte_flags;
		va += PAGE_SIZE;
		pa += PAGE_SIZE;
	}
	return 0;
}

/**
 * mmu_map_user() - build a task's user mappings into its private L1
 * @mm: mm with pgd allocated and code/bss/stack pages + sizes + sp_top set.
 *
 * Code (RW+exec) at USER_CODE_VA, bss (RW+XN) immediately after it, and
 * the stack (RW+XN) just below sp_top — the stack lives high (near
 * TASK_SIZE) so it can never collide with a growing bss/heap.
 */
int mmu_map_user(struct mm_struct *mm)
{
	unsigned long code_va  = USER_CODE_VA;
	unsigned long bss_va   = USER_CODE_VA +
				 (unsigned long)mm->code_pages * PAGE_SIZE;
	unsigned long stack_va = mm->sp_top -
				 (unsigned long)mm->stack_pages * PAGE_SIZE;

	if (map_user_range(mm, code_va, mm->code_pa, mm->code_pages, PTE_USER_CODE))
		return -1;

	/* bss: lay each scatter-allocated chunk into consecutive VAs above code. */
	unsigned long va = bss_va;
	for (unsigned int i = 0; i < mm->nr_bss_chunks; i++) {
		unsigned int npages = 1u << mm->bss_chunks[i].order;
		if (map_user_range(mm, va, mm->bss_chunks[i].pa, npages, PTE_USER_DATA))
			return -1;
		va += (unsigned long)npages * PAGE_SIZE;
	}

	if (map_user_range(mm, stack_va, mm->stack_pa, mm->stack_pages, PTE_USER_DATA))
		return -1;

	/* Clean every L2 table to PoC for the walker. */
	for (unsigned int i = 0; i < mm->nr_l2; i++)
		pt_clean(mm->l2s[i].l2, 1024);
	return 0;
}

/**
 * mmu_switch_mm() - install a task's address space into TTBR0
 * @mm: the next task's mm (NULL for a kernel thread → swapper table)
 *
 * Loads the per-process L1 into TTBR0 and flushes the whole TLB (no
 * ASIDs yet) plus the branch predictor (Cortex-A8 erratum 430973), so
 * no stale user translations survive the switch. The kernel half is
 * identical in every table, so kernel code keeps running across it.
 */
void mmu_switch_mm(struct mm_struct *mm)
{
	unsigned long pgd_pa = (mm && mm->pgd_pa)
			     ? mm->pgd_pa
			     : ((unsigned long)&__pgd_start - MMU_OFFSET);
	unsigned long ttbr0 = (pgd_pa & 0xFFFFC000) | TTBR0_FLAGS;

	__asm__ __volatile__(
		"mov	r0, #0\n"
		"mcr	p15, 0, r0, c13, c0, 1\n"	/* CONTEXTIDR = 0 (no ASID) */
		"isb\n"
		"mcr	p15, 0, %0, c2, c0, 0\n"	/* TTBR0 = pgd | flags */
		"isb\n"
		"mcr	p15, 0, r0, c8, c7, 0\n"	/* TLBIALL  — flush all TLB */
		"mcr	p15, 0, r0, c7, c5, 6\n"	/* BPIALL   — flush branch pred */
		"dsb\n"
		"isb\n"
		: : "r" (ttbr0) : "r0", "memory");
}

/**
 * pgd_free() - release a task's private page tables
 *
 * Frees the owned L2 tables and the 16 KB L1. The user code/bss/stack
 * physical pages are freed separately by the caller (do_exit).
 */
void pgd_free(struct mm_struct *mm)
{
	struct zone *zone = get_zone();

	for (unsigned int i = 0; i < mm->nr_l2; i++)
		kfree(mm->l2s[i].l2);
	mm->nr_l2 = 0;

	if (mm->pgd_pa) {
		struct page *pg = pfn_to_page(zone,
			(mm->pgd_pa - zone->base_pa) >> PAGE_SHIFT);
		if (pg)
			__free_pages(pg, 2);
		mm->pgd_pa = 0;
		mm->pgd = NULL;
	}
}
