#include <nothan/types.h>
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

	/* Kernel direct: VA 0xC0000000 → PA 0x80000000. */
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

	/* Identity map for .idmap.text — use phys addresses. */
	unsigned int i = ((u32)&__idmap_start - MMU_OFFSET) >> 20;
	while (i <= (((u32)&__idmap_end - MMU_OFFSET) >> 20)) {
		map_section(pgd, i << 20, i << 20,
			    MT_NORMAL | PMD_SECT_DOMAIN(DOMAIN_KERNEL) | PMD_SECT_AP_RW);
		i++;
	}

	dsb();

	__turn_mmu_on(pgd_phys);
}
