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

struct list_head {
	struct list_head *next;
	struct list_head *prev;
};

/* Page flags */
#define PG_BUDDY		0

struct page {
	struct list_head lru;		/* must be first (list_head → page cast) */
	unsigned long flags;
	unsigned long private;
	int _refcount;
};

struct free_area {
	struct list_head free_list;
	unsigned long nr_free;
};

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
void __free_pages(struct page *page, unsigned int order);

#endif /* _MM_H */
