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

/*
 * struct mm_struct - per-process memory descriptor (Phase 10b)
 *
 * Tracks the L2 page table and user-space pages for a single
 * user task.  The global L1 table entry pointing to this L2 is
 * updated by mmu_map_user() at task creation time.
 */
struct mm_struct {
	u32  *l2;                /* L2 page table (1 KB, 256 entries × 4 B) */
	u32   l1_idx;            /* L1 index used (VA >> 20) */
	unsigned long code_pa;   /* physical address of user code pages   */
	unsigned long bss_pa;    /* physical address of BSS pages (0 if none) */
	unsigned long stack_pa;  /* physical address of user stack pages */
	unsigned long entry_va;  /* user-space entry point VA */
	unsigned long sp_top;    /* user stack top VA (initial sp) */
	unsigned int  code_pages;  /* number of 4KB code pages  */
	unsigned int  bss_pages;   /* number of 4KB BSS pages   */
	unsigned int  stack_pages; /* number of 4KB stack pages */
	unsigned long l2_pa;       /* physical address of L2 table */
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

#endif /* _MM_H */
