#ifndef _MM_H
#define _MM_H

#include <nothan/types.h>

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1UL << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE - 1))

#define MAX_ORDER		10
#define NR_PAGE_ORDERS		(MAX_ORDER + 1)

/* gfp flags */
typedef unsigned int gfp_t;

#define GFP_KERNEL		0
#define GFP_ATOMIC		1
#define GFP_USER		2   /* user-space page allocation */

/* Kernel direct-map: VA = PA + (PAGE_OFFSET - PHYS_OFFSET) */
#define PAGE_OFFSET		0xC0000000UL
#define PHYS_OFFSET		0x80000000UL
#define phys_to_kva(pa)		((void *)((unsigned long)(pa) + (PAGE_OFFSET - PHYS_OFFSET)))

/* User image base — must match userspace/lib/user.lds and mmu_map_user(). */
#define USER_CODE_VA		0x00010000UL

/*
 * Max L2 (coarse) page tables a process may own. Each covers a 1 MB VA
 * window; code+bss usually need 1-2, the high stack 1, leaving slack for
 * growth. Bump if a process maps more than ~16 MB of distinct windows.
 */
#define MM_MAX_L2  16

struct mm_l2 {
	u32         *l2;       /* kernel VA of the 1 KB L2 table */
	unsigned int l1_idx;   /* L1 slot it is installed at (VA >> 20) */
};

/*
 * BSS is allocated as a handful of physically-contiguous chunks (each up to
 * one buddy MAX_ORDER block) rather than a single contiguous block. This lifts
 * the old ~4 MB ceiling (a single power-of-2 alloc capped at MAX_ORDER) and
 * survives physical fragmentation on respawn, since the chunks need not be
 * contiguous with each other — they are mapped into consecutive user VAs.
 */
struct mm_bss_chunk {
	unsigned long pa;      /* physical base of this chunk */
	unsigned int  order;   /* buddy order (chunk = 2^order pages) */
};
#define MM_MAX_BSS_CHUNKS  16

/*
 * struct mm_struct - per-process memory descriptor
 *
 * Each user task owns a private 16 KB L1 page table (@pgd). The kernel
 * half (VA >= 0xC0000000) is copied from the master swapper table at
 * pgd_alloc() time; the user half is filled by mmu_map_user() with L2
 * tables for code, bss and the high stack. A context switch loads
 * @pgd_pa into TTBR0 (see mmu_switch_mm).
 */
struct mm_struct {
	u32          *pgd;        /* private L1 table (16 KB, 4096 entries) */
	unsigned long pgd_pa;     /* physical address of @pgd → TTBR0 base */
	struct mm_l2  l2s[MM_MAX_L2];  /* owned L2 tables (for teardown) */
	unsigned int  nr_l2;      /* number of L2 tables in use */

	unsigned long code_pa;   /* physical address of user code pages   */
	struct mm_bss_chunk bss_chunks[MM_MAX_BSS_CHUNKS]; /* scatter-allocated bss */
	unsigned int  nr_bss_chunks;
	unsigned long stack_pa;  /* physical address of user stack pages */
	unsigned long entry_va;  /* user-space entry point VA */
	unsigned long sp_top;    /* user stack top VA (initial sp) */
	unsigned int  code_pages;  /* number of 4KB code pages  */
	unsigned int  bss_pages;   /* total number of 4KB BSS pages (across chunks) */
	unsigned int  stack_pages; /* number of 4KB stack pages */
};

/**
 * struct list_head - Circular doubly linked list node
 * @next: Pointer to the next node
 * @prev: Pointer to the previous node
 */
struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

/* Page flags */
#define PG_BUDDY		0

struct slab_cache;

/**
 * struct page - Physical page metadata
 * @lru: List node for buddy allocator free lists. Must be first member.
 * @flags: Page flags (e.g. PG_BUDDY)
 * @private: Order of the page block if in buddy allocator
 * @_refcount: Number of references to this page
 * @slab: Pointer to the owning slab cache, or NULL if managed by buddy
 */
struct page {
	struct list_head lru;
	unsigned long flags;
	unsigned long private;
	int _refcount;
	struct slab_cache *slab;	/* owning slab cache (NULL if buddy) */
};

/**
 * struct free_area - A list of free page blocks of a specific order
 * @free_list: List of free page blocks
 * @nr_free: Number of free blocks in this list
 */
struct free_area {
	struct list_head free_list;
	unsigned long nr_free;
};

/**
 * struct zone - Represents a physical memory zone managed by the allocator
 * @free_area: Array of free lists for each block order
 * @managed_pages: Total number of pages managed by this zone
 * @free_pages: Current number of free pages
 * @page_array: Pointer to the array of struct page metadata
 * @base_pa: Starting physical address of the managed pool
 * @end_pa: Ending physical address of the managed pool
 */
struct zone {
	struct free_area free_area[NR_PAGE_ORDERS];
	unsigned long managed_pages;
	unsigned long free_pages;
	struct page *page_array;
	unsigned long base_pa;
	unsigned long end_pa;
};

static inline void set_page_flag(struct page *page, int bit)
{
	page->flags |= (1UL << bit);
}

static inline void clear_page_flag(struct page *page, int bit)
{
	page->flags &= ~(1UL << bit);
}

static inline int test_page_flag(struct page *page, int bit)
{
	return !!(page->flags & (1UL << bit));
}

static inline unsigned long page_to_pfn(struct zone *zone, struct page *page)
{
	return (page - zone->page_array);
}

static inline struct page *pfn_to_page(struct zone *zone, unsigned long pfn)
{
	return &zone->page_array[pfn];
}

static inline unsigned long page_to_phys(struct zone *zone, struct page *page)
{
	return zone->base_pa + (page_to_pfn(zone, page) << PAGE_SHIFT);
}

/* Buddy math */
static inline unsigned long __find_buddy_pfn(unsigned long pfn, unsigned int order)
{
	return pfn ^ (1UL << order);
}

/* List helpers */

#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr) - __builtin_offsetof(type, member)))

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry_safe(pos, tmp, head, type, member)		\
	for (pos = list_entry((head)->next, type, member),		\
	     tmp = list_entry(pos->member.next, type, member);		\
	     &pos->member != (head);					\
	     pos = tmp, tmp = list_entry(tmp->member.next, type, member))

#define LIST_HEAD(name) struct list_head name = { &(name), &(name) }

static inline void list_init(struct list_head *head)
{
	head->next = head;
	head->prev = head;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	head->next->prev = new;
	new->next = head->next;
	new->prev = head;
	head->next = new;
}

static inline void list_del(struct list_head *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
}

static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	new->next = head;
	new->prev = head->prev;
	head->prev->next = new;
	head->prev = new;
}

/* Free-list helpers */
static inline void __add_to_free_list(struct page *page, struct zone *zone,
				      unsigned int order)
{
	list_add(&page->lru, &zone->free_area[order].free_list);
	zone->free_area[order].nr_free++;
}

static inline void __del_from_free_list(struct page *page, struct zone *zone,
					unsigned int order)
{
	list_del(&page->lru);
	zone->free_area[order].nr_free--;
}

static inline int page_is_buddy(struct page *page, unsigned int order)
{
	return test_page_flag(page, PG_BUDDY) && page->private == order;
}

static inline void set_page_order(struct page *page, unsigned int order)
{
	page->private = order;
}

struct zone *get_zone(void);
void page_alloc_init(void);
struct page *alloc_pages(gfp_t gfp, unsigned int order);
void mmu_switch_mm(struct mm_struct *mm);
void __free_pages(struct page *page, unsigned int order);

/* Per-process page tables (arch/arm/mm/mmu.c) */
int  pgd_alloc(struct mm_struct *mm);
void pgd_free(struct mm_struct *mm);
int  mmu_map_user(struct mm_struct *mm);

/* Release every BSS chunk back to the buddy allocator (spawn cleanup + exit). */
static inline void mm_free_bss_chunks(struct mm_struct *mm, struct zone *zone)
{
	for (unsigned int i = 0; i < mm->nr_bss_chunks; i++) {
		struct page *pg = pfn_to_page(zone,
			(mm->bss_chunks[i].pa - zone->base_pa) >> PAGE_SHIFT);
		if (pg)
			__free_pages(pg, mm->bss_chunks[i].order);
	}
	mm->nr_bss_chunks = 0;
}

#endif /* _MM_H */
