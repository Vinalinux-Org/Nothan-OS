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
 * Top of the user stack REGION: the stack occupies the pages just below this,
 * always page-aligned and always at this address.
 *
 * Not the same thing as mm->sp_top, and the distinction matters. sp_top is
 * where a task's sp STARTS, which sits below the argc/argv block the kernel
 * writes at the top of the region - so it is neither page-aligned nor equal to
 * the region top. Deriving the mapping from sp_top (as this once did) shifts
 * the whole stack mapping down by the size of that block, and then the argv
 * pointers, computed against the region top, address pages that are not there.
 *
 * Lives high, near TASK_SIZE, so it stays far from the low code+bss region -
 * bss can grow without ever reaching the stack.
 */
#define USER_STACK_TOP		0xBF000000UL

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
 * A user region is a handful of physically-contiguous chunks rather than one
 * contiguous block, each chunk at most a buddy MAX_ORDER allocation.
 *
 * Two things fall out of that. The region can exceed the ~4 MB a single
 * power-of-2 allocation is capped at; and, more importantly, it can be built
 * out of whatever the buddy allocator still has. A single-block region needs
 * one run of free pages of exactly the right size, so a long-lived system
 * whose memory has fragmented can refuse to start a program while holding
 * plenty of free memory - the failure that looks like a leak and is not one.
 *
 * The chunks need not be adjacent to each other: mmu_map_user() lays them into
 * CONSECUTIVE user virtual addresses, so the program sees one flat region no
 * matter how scattered the physical pages are. That is what page tables are
 * for, and it is why nothing above this layer has to know.
 */
struct mm_chunk {
	unsigned long pa;      /* physical base of this chunk */
	unsigned int  order;   /* buddy order (chunk = 2^order pages) */
};
#define MM_MAX_CHUNKS  16

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

	/*
	 * Code and bss are both scatter-allocated; the stack is not.
	 *
	 * The stack stays a single block because it is one fixed, modest size
	 * (128 KB, order 5) that the buddy allocator can nearly always satisfy,
	 * and because nothing makes it grow. Code and bss are as large as the
	 * program says they are - the GUI's code alone is a 1 MB order-8 run -
	 * which is exactly the size that fragmentation starts refusing.
	 */
	struct mm_chunk code_chunks[MM_MAX_CHUNKS];
	unsigned int  nr_code_chunks;
	struct mm_chunk bss_chunks[MM_MAX_CHUNKS];
	unsigned int  nr_bss_chunks;

	unsigned long stack_pa;  /* physical address of user stack pages */
	unsigned long entry_va;  /* user-space entry point VA */
	unsigned long sp_top;    /* user stack top VA (initial sp) */
	unsigned int  code_pages;  /* total 4KB code pages (across chunks)  */
	unsigned int  bss_pages;   /* total 4KB BSS pages (across chunks) */
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

/* Non-safe walk: do NOT del/free pos inside the loop (use _safe for that). */
#define list_for_each_entry(pos, head, type, member)			\
	for (pos = list_entry((head)->next, type, member);		\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, type, member))

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

/*
 * list_del_init - unlink @entry and re-point it at itself.
 * After this, list_empty(entry) is true — lets a caller tell "on a list"
 * from "not on any list" (needed by wake_up_task to know whether a task is
 * queued on a waitq via run_list or sleeping off all lists).
 */
static inline void list_del_init(struct list_head *entry)
{
	list_del(entry);
	list_init(entry);
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

/*
 * Release every chunk of a region back to the buddy allocator.
 *
 * Takes the array rather than the mm so code and bss share one implementation;
 * @nr is zeroed through the pointer so a second call is a no-op, which is what
 * lets the spawn unwind path free a region it may or may not have built yet.
 */
static inline void mm_free_chunks(struct mm_chunk *chunks, unsigned int *nr,
				  struct zone *zone)
{
	for (unsigned int i = 0; i < *nr; i++) {
		struct page *pg = pfn_to_page(zone,
			(chunks[i].pa - zone->base_pa) >> PAGE_SHIFT);
		if (pg)
			__free_pages(pg, chunks[i].order);
	}
	*nr = 0;
}

#endif /* _MM_H */
