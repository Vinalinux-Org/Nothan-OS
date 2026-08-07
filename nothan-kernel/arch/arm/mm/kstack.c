/*
 * arch/arm/mm/kstack.c - Kernel stacks with guard pages
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * See <nothan/kstack.h> for why kernel stacks no longer come from kmalloc().
 * This file is the ARM half: a dedicated VA window backed by real L2 tables,
 * one slot per stack, each slot half guard and half stack.
 *
 *   slot base ->  +----------------------+  <- KSTACK_SIZE of GUARD
 *                 |     unmapped         |     (L2 entries left at 0)
 *   stack base -> +----------------------+  <- what kstack_alloc() returns
 *                 |                      |
 *                 |   usable stack       |     KSTACK_SIZE, mapped
 *                 |                      |
 *   stack top  -> +----------------------+  <- initial sp; stack grows DOWN
 *
 * The guard sits BELOW the stack because ARM stacks descend: an overflow walks
 * sp downwards out of the mapped region and into the hole, which is a
 * translation fault at the exact instruction that did it.
 */

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/kstack.h>
#include <nothan/printk.h>
#include <nothan/panic.h>
#include <asm/memory.h>
#include <asm/pgtable.h>
#include <asm/barrier.h>
#include <asm/irqflags.h>

/*
 * The window. Sits immediately above the 512 MB kernel direct map
 * (0xC0000000..0xE0000000) and well below the MMIO windows at 0xF0000000, so it
 * collides with nothing. Only VA is consumed here; RAM is taken a stack at a
 * time from the buddy allocator.
 */
#define KSTACK_VA_BASE		0xE0000000UL
#define KSTACK_SLOT_SIZE	(2u * KSTACK_SIZE)	/* guard + stack */
#define KSTACK_MAX		64
#define KSTACK_WINDOW_SIZE	((unsigned long)KSTACK_MAX * KSTACK_SLOT_SIZE)
#define KSTACK_NR_L2		(KSTACK_WINDOW_SIZE >> 20)	/* 1 MB per L2 */

/*
 * L2 tables for the window, static rather than allocated.
 *
 * They must exist before the allocator does anything, because kstack_init()
 * has to install them into the master L1 before the first process page table
 * is copied from it (see the header). Static also means they live in .bss,
 * which head.S zeroes at boot - so every entry starts as a translation fault,
 * and a guard page is simply an entry nobody ever filled in. The guard costs
 * no code: it is the default state.
 *
 * ARM requires a coarse L2 table to be 1 KB aligned.
 */
static u32 kstack_l2[KSTACK_NR_L2][256] __attribute__((aligned(1024)));

/* One bit per slot; set = in use. */
static u32 kstack_used_map[(KSTACK_MAX + 31) / 32];
static unsigned int kstack_live;
static unsigned int kstack_peak;

/*
 * Kernel stack page: Normal write-back write-allocate (matching the direct-map
 * attributes for the same physical pages), kernel RW, never executable, and
 * GLOBAL - these mappings are identical in every address space, so tagging them
 * not-global would only cost TLB entries.
 */
#define PTE_KSTACK	(PTE_SMALL_TEX(1) | PTE_SMALL_C | PTE_SMALL_B | \
			 PTE_SMALL_AP_RW | PTE_SMALL_XN)

/*
 * Poison. A fresh stack is filled with a recognisable word so the high-water
 * mark can be measured later: the deepest address that is no longer poison is
 * how far the stack has ever been used.
 *
 * This is the only way to answer "is 16 KB the right number?" on a machine with
 * no JTAG. Guessing high wastes memory per task; guessing low is the bug this
 * whole file exists to catch. Measuring costs one fill at task creation, which
 * happens rarely and never on a hot path.
 */
#define KSTACK_POISON	0xA5A5C0DEu

static inline unsigned long slot_base_va(unsigned int slot)
{
	return KSTACK_VA_BASE + (unsigned long)slot * KSTACK_SLOT_SIZE;
}

/* The usable stack starts one guard-length above the slot base. */
static inline unsigned long stack_base_va(unsigned int slot)
{
	return slot_base_va(slot) + KSTACK_SIZE;
}

static inline int slot_of_base(unsigned long base_va, unsigned int *slot)
{
	unsigned long off;

	if (base_va < KSTACK_VA_BASE ||
	    base_va >= KSTACK_VA_BASE + KSTACK_WINDOW_SIZE)
		return -1;

	off = base_va - KSTACK_VA_BASE;
	if (off % KSTACK_SLOT_SIZE != KSTACK_SIZE)
		return -1;		/* not a stack base — mid-stack or guard */

	*slot = (unsigned int)(off / KSTACK_SLOT_SIZE);
	return 0;
}

/* Leaf entry for a VA inside the window. */
static inline u32 *pte_of(unsigned long va)
{
	unsigned long off = va - KSTACK_VA_BASE;

	return &kstack_l2[off >> 20][(va >> 12) & 0xFF];
}

void kstack_init(void)
{
	u32 *pgd = swapper_pgd();

	for (unsigned int i = 0; i < KSTACK_NR_L2; i++) {
		unsigned long va = KSTACK_VA_BASE + ((unsigned long)i << 20);
		unsigned long pa = __virt_to_phys((unsigned long)kstack_l2[i]);

		pgd[va >> 20] = (pa & 0xFFFFFC00) |
				PMD_SECT_DOMAIN(DOMAIN_KERNEL) | PMD_TYPE_TABLE;
	}

	/* The table walker reads page tables from memory, not through the data
	 * cache, so every table write has to reach PoC before it counts. */
	clean_dcache_range((unsigned long)&pgd[KSTACK_VA_BASE >> 20],
			   (unsigned long)&pgd[(KSTACK_VA_BASE >> 20) + KSTACK_NR_L2]);
	flush_tlb();

	printk("[KSTACK] window VA 0x%08lx +%luKB: %u slots x %luKB stack + %luKB guard\n",
	       KSTACK_VA_BASE, KSTACK_WINDOW_SIZE >> 10, KSTACK_MAX,
	       (unsigned long)KSTACK_SIZE >> 10, (unsigned long)KSTACK_SIZE >> 10);

	stack_guard_init();
}

/* Claim a free slot. Caller-visible failure is "no slot", not a wrong slot. */
static int slot_claim(unsigned int *out)
{
	unsigned long flags;
	int found = -1;

	local_irq_save(flags);
	for (unsigned int i = 0; i < KSTACK_MAX; i++) {
		if (kstack_used_map[i / 32] & (1u << (i % 32)))
			continue;
		kstack_used_map[i / 32] |= (1u << (i % 32));
		kstack_live++;
		if (kstack_live > kstack_peak)
			kstack_peak = kstack_live;
		found = (int)i;
		break;
	}
	local_irq_restore(flags);

	if (found < 0)
		return -1;
	*out = (unsigned int)found;
	return 0;
}

static void slot_release(unsigned int slot)
{
	unsigned long flags;

	local_irq_save(flags);
	kstack_used_map[slot / 32] &= ~(1u << (slot % 32));
	kstack_live--;
	local_irq_restore(flags);
}

void *kstack_alloc(void)
{
	struct zone *zone = get_zone();
	unsigned int slot;
	struct page *pg;
	unsigned long pa, base;

	if (slot_claim(&slot)) {
		pr_err("[KSTACK] all %u slots in use - cannot create task\n",
		       KSTACK_MAX);
		return NULL;
	}

	pg = alloc_pages(GFP_KERNEL, KSTACK_ORDER);
	if (!pg) {
		slot_release(slot);
		pr_err("[KSTACK] out of memory for a %lu KB stack\n",
		       (unsigned long)KSTACK_SIZE >> 10);
		return NULL;
	}

	pa   = page_to_phys(zone, pg);
	base = stack_base_va(slot);

	/*
	 * Fill the leaf entries. No IRQ masking: the slot is exclusively ours
	 * from slot_claim() onwards, and these are independent 32-bit stores to
	 * entries no other context can be touching. The guard entries below are
	 * left exactly as they are - zero - which is what makes them guards.
	 */
	for (unsigned int i = 0; i < KSTACK_PAGES; i++)
		*pte_of(base + (unsigned long)i * PAGE_SIZE) =
			((pa + (unsigned long)i * PAGE_SIZE) & 0xFFFFF000) |
			PTE_TYPE_SMALL | PTE_KSTACK;

	/* End bound from the LAST entry plus one, not from pte_of(base + size):
	 * for the final slot in the window that address lies one 1 MB step past
	 * the table array, so the index would run off the end of kstack_l2[]. */
	clean_dcache_range((unsigned long)pte_of(base),
			   (unsigned long)(pte_of(base + KSTACK_SIZE - PAGE_SIZE) + 1));
	flush_tlb();

	/* Poison through the new mapping — which also proves it works before a
	 * task is asked to run on it. */
	for (unsigned long *p = (unsigned long *)base;
	     p < (unsigned long *)(base + KSTACK_SIZE); p++)
		*p = KSTACK_POISON;

	return (void *)base;
}

void kstack_free(void *base_ptr)
{
	struct zone *zone = get_zone();
	unsigned long base = (unsigned long)base_ptr;
	unsigned int slot;
	struct page *pg;
	unsigned long pa;

	if (!base_ptr)
		return;

	if (slot_of_base(base, &slot)) {
		pr_err("[KSTACK] kstack_free(%p): not a stack base - IGNORED\n",
		       base_ptr);
		return;
	}

	/*
	 * Recover the physical pages from the leaf entry BEFORE clearing it —
	 * the page table is the only record of which block backs this slot.
	 */
	pa = *pte_of(base) & 0xFFFFF000;

	for (unsigned int i = 0; i < KSTACK_PAGES; i++)
		*pte_of(base + (unsigned long)i * PAGE_SIZE) = 0;

	/* End bound from the LAST entry plus one, not from pte_of(base + size):
	 * for the final slot in the window that address lies one 1 MB step past
	 * the table array, so the index would run off the end of kstack_l2[]. */
	clean_dcache_range((unsigned long)pte_of(base),
			   (unsigned long)(pte_of(base + KSTACK_SIZE - PAGE_SIZE) + 1));
	flush_tlb();

	pg = phys_to_page(zone, pa);
	if (pg)
		__free_pages(pg, KSTACK_ORDER);
	else
		pr_err("[KSTACK] slot %u backed by pa=0x%lx outside zone - LEAKED\n",
		       slot, pa);

	slot_release(slot);
}

unsigned int kstack_in_use(void)
{
	return kstack_live;
}

unsigned int kstack_capacity(void)
{
	return KSTACK_MAX;
}

/* ===================================================================
 * Canaries for the stacks that cannot have a guard page
 *
 * The boot/SVC stack and the exception-mode stacks are laid out by the linker
 * and are live before the MMU, the page allocator or this file exist. They
 * cannot be moved into the guarded window - so they get the weaker tool.
 *
 * Weaker in two specific ways, and it is worth being honest about both:
 * a canary is noticed only when something checks it, so the report arrives
 * LATER than the overflow rather than at the instruction that caused it; and
 * it catches only a write that lands on the canary word itself, not one that
 * steps clean over it. What it does give is the difference between "the kernel
 * behaves strangely" and "the abort stack overflowed", which is the whole
 * distance between an unfindable bug and a fixable one.
 * =================================================================== */

#define STACK_CANARY	0xDEADBEA7u

extern char fiq_stack_bottom[], fiq_stack_top[];
extern char irq_stack_bottom[], irq_stack_top[];
extern char abt_stack_bottom[], abt_stack_top[];
extern char und_stack_bottom[], und_stack_top[];
extern char svc_stack_bottom[], svc_stack_top[];

static const struct {
	const char *name;
	char       *bottom;
	char       *top;
} static_stacks[] = {
	{ "svc/idle", svc_stack_bottom, svc_stack_top },
	{ "abort",    abt_stack_bottom, abt_stack_top },
	{ "undef",    und_stack_bottom, und_stack_top },
	{ "irq",      irq_stack_bottom, irq_stack_top },
	{ "fiq",      fiq_stack_bottom, fiq_stack_top },
};
#define NR_STATIC_STACKS \
	(sizeof(static_stacks) / sizeof(static_stacks[0]))

void stack_guard_init(void)
{
	for (unsigned int i = 0; i < NR_STATIC_STACKS; i++) {
		*(volatile u32 *)static_stacks[i].bottom = STACK_CANARY;
		printk("[KSTACK] static stack %-8s %luKB, canary at %p\n",
		       static_stacks[i].name,
		       (unsigned long)(static_stacks[i].top -
				       static_stacks[i].bottom) >> 10,
		       static_stacks[i].bottom);
	}
}

/**
 * stack_guard_check() - verify no static stack has run past its bottom
 *
 * Called from the timer tick, so the window between an overflow and the report
 * is at most one tick. panic() rather than a warning: the canary being gone
 * means memory below that stack has ALREADY been overwritten, so whatever the
 * kernel does next is running on state that may no longer mean anything.
 * Continuing turns one traceable failure into an untraceable one.
 */
void stack_guard_check(void)
{
	for (unsigned int i = 0; i < NR_STATIC_STACKS; i++)
		if (*(volatile u32 *)static_stacks[i].bottom != STACK_CANARY)
			panic("%s stack overflowed (canary at %p destroyed)",
			      static_stacks[i].name, static_stacks[i].bottom);
}
