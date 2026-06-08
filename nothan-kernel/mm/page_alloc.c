#include <nothan/types.h>
#include <nothan/mm.h>
#include <asm/memory.h>

/*
 * DDR pool end address.
 * The BeagleBone Black has 512 MB DDR3 at 0x80000000–0x9FFFFFFF.
 * Pool ends at 0xA0000000.
 */
#define DDR_END			0xA0000000UL
#define PAGE_ARRAY_GAP		(4UL << 20)

extern u32 _end;

static struct zone mem_zone;

struct zone *get_zone(void)
{
	return &mem_zone;
}

/**
 * page_alloc_init() - Initialize the page allocator and memory zone
 *
 * Scans the memory layout, reserves space for the struct page array,
 * and initializes all physical pages into the buddy allocator pool.
 */
void page_alloc_init(void)
{
	struct zone *zone = &mem_zone;
	unsigned long kernel_end_pa, pool_start_pa;
	unsigned long total_pages;

	kernel_end_pa = __virt_to_phys((unsigned long)&_end);
	zone->page_array = (struct page *)__phys_to_virt(kernel_end_pa);

	pool_start_pa = (kernel_end_pa + PAGE_ARRAY_GAP) & PAGE_MASK;
	zone->base_pa = pool_start_pa;
	zone->end_pa  = DDR_END & PAGE_MASK;

	total_pages = (zone->end_pa - zone->base_pa) >> PAGE_SHIFT;
	zone->managed_pages = total_pages;
	zone->free_pages = 0;

	/* Initialise free lists. */
	for (unsigned int order = 0; order < NR_PAGE_ORDERS; order++)
		list_init(&zone->free_area[order].free_list);

	/*
	 * Initialise every page and enqueue it at order 0.
	 * lru MUST be initialised before any list_del() is called;
	 * the DDR backing page_array is not zero-initialised.
	 */
	for (unsigned long i = 0; i < total_pages; i++) {
		struct page *p = &zone->page_array[i];

		list_init(&p->lru);
		p->flags = 0;
		p->private = 0;
		p->_refcount = 0;
		p->slab = NULL;
		set_page_flag(p, PG_BUDDY);
		set_page_order(p, 0);
		__add_to_free_list(p, zone, 0);
	}
	zone->free_pages = total_pages;

	/*
	 * Build buddy free lists: for each page, attempt to merge upward.
	 * After the merge loop, the block sits at its highest achievable order.
	 */
	for (unsigned long i = 0; i < total_pages; ) {
		struct page *page = &zone->page_array[i];
		unsigned long pfn = i;
		unsigned int order = 0;

		while (order < MAX_ORDER) {
			unsigned long buddy_pfn = __find_buddy_pfn(pfn, order);
			if (buddy_pfn >= total_pages)
				break;

			struct page *buddy = &zone->page_array[buddy_pfn];
			if (!page_is_buddy(buddy, order))
				break;

			__del_from_free_list(buddy, zone, order);
			clear_page_flag(buddy, PG_BUDDY);
			buddy->private = 0;

			if (buddy_pfn < pfn) {
				pfn = buddy_pfn;
				page = buddy;
			}
			order++;
		}

		set_page_flag(page, PG_BUDDY);
		set_page_order(page, order);
		__add_to_free_list(page, zone, order);
		zone->free_pages += (1UL << order);

		i += (1UL << order);
	}
}

static void expand(struct page *page, struct zone *zone, unsigned int low, unsigned int high)
{
	unsigned long size = 1UL << high;

	while (high > low) {
		high--;
		size >>= 1;

		struct page *buddy = page + size;

		set_page_flag(buddy, PG_BUDDY);
		set_page_order(buddy, high);
		__add_to_free_list(buddy, zone, high);
	}
}

/**
 * alloc_pages() - Allocate contiguous pages
 * @gfp: GFP flags (e.g. GFP_KERNEL)
 * @order: Log2 of the number of pages to allocate
 *
 * Return: Pointer to the first struct page, or NULL if out of memory.
 */
struct page *alloc_pages(gfp_t gfp, unsigned int order)
{
	struct zone *zone = &mem_zone;

	(void)gfp;

	if (order > MAX_ORDER)
		return NULL;

	for (unsigned int current_order = order; current_order <= MAX_ORDER; current_order++) {
		struct free_area *area = &zone->free_area[current_order];

		if (list_empty(&area->free_list))
			continue;

		struct page *page = (struct page *)area->free_list.next;
		__del_from_free_list(page, zone, current_order);
		clear_page_flag(page, PG_BUDDY);
		page->private = 0;
		page->_refcount = 1;

		zone->free_pages -= (1UL << order);

		if (current_order > order)
			expand(page, zone, order, current_order);

		return page;
	}

	return NULL;
}

/**
 * __free_pages() - Free contiguous pages back to the buddy allocator
 * @page: Pointer to the first struct page to free
 * @order: Log2 of the number of pages to free
 */
void __free_pages(struct page *page, unsigned int order)
{
	struct zone *zone = &mem_zone;
	unsigned long pfn = page - zone->page_array;

	while (order < MAX_ORDER) {
		unsigned long buddy_pfn = __find_buddy_pfn(pfn, order);
		if (buddy_pfn >= zone->managed_pages)
			break;

		struct page *buddy = &zone->page_array[buddy_pfn];
		if (!page_is_buddy(buddy, order))
			break;

		__del_from_free_list(buddy, zone, order);
		clear_page_flag(buddy, PG_BUDDY);
		buddy->private = 0;

		if (buddy_pfn < pfn) {
			pfn = buddy_pfn;
			page = buddy;
		}
		order++;
	}

	set_page_flag(page, PG_BUDDY);
	set_page_order(page, order);
	__add_to_free_list(page, zone, order);
	zone->free_pages += (1UL << order);
	page->_refcount = 0;
}
