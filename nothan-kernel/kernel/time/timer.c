#include <nothan/timer.h>
#include <nothan/time.h>

static LIST_HEAD(timer_head);		/* sorted by expires (ascending) */

/*
 * Disable / enable IRQ helpers.  On a single core these are the only
 * mutual-exclusion needed between process context and the tick ISR.
 */
static inline unsigned long irq_save(void)
{
	unsigned long cpsr;
	__asm__ __volatile__ (
		"mrs	%0, cpsr\n"
		"cpsid	i\n"
		: "=r" (cpsr)
	);
	return cpsr;
}

static inline void irq_restore(unsigned long cpsr)
{
	if (!(cpsr & 0x80))
		__asm__ __volatile__ ("cpsie i" : : : "memory");
}

/**
 * add_timer() - arm a one-shot timer
 * @timer: pointer to the timer_list structure
 *
 * Inserts into the sorted list by expiry.  Fires from the tick ISR
 * when jiffies >= expires.
 */
void add_timer(struct timer_list *timer)
{
	unsigned long flags = irq_save();
	struct list_head *pos;

	list_for_each(pos, &timer_head) {
		struct timer_list *t = list_entry(pos, struct timer_list, entry);
		if (timer->expires < t->expires)
			break;
	}

	/* Insert before pos (pos == &timer_head -> append at tail). */
	timer->entry.next = pos;
	timer->entry.prev = pos->prev;
	pos->prev->next = &timer->entry;
	pos->prev = &timer->entry;

	irq_restore(flags);
}

/**
 * del_timer() - cancel a pending timer
 * @timer: pointer to the timer_list structure
 *
 * Return: 1 if the timer was pending and has been cancelled,
 * 0 if the timer already fired or was never armed.
 */
int del_timer(struct timer_list *timer)
{
	if (!timer->entry.next)
		return 0;

	unsigned long flags = irq_save();

	if (!timer->entry.next) {
		irq_restore(flags);
		return 0;
	}

	list_del(&timer->entry);
	timer->entry.next = NULL;	/* mark detached */
	irq_restore(flags);
	return 1;
}

/**
 * mod_timer() - re-arm a timer with a new expiry
 * @timer: pointer to the timer_list structure
 * @expires: new expiration time in jiffies
 */
int mod_timer(struct timer_list *timer, unsigned long expires)
{
	unsigned long flags = irq_save();

	if (timer->entry.next)
		list_del(&timer->entry);

	timer->expires = expires;
	timer->entry.next = NULL;	/* re-validate for add_timer */
	irq_restore(flags);

	add_timer(timer);
	return 0;
}

/**
 * run_local_timers() - fire all expired timers
 *
 * Called from the timer tick ISR (IRQ already off).  Collects expired
 * entries first so callbacks may safely add/del other timers.
 */
void run_local_timers(void)
{
	struct timer_list *timer, *tmp;
	struct list_head expired;
	unsigned long now = get_jiffies();

	list_init(&expired);

	/* Collect expired entries from the sorted timer list. */
	list_for_each_entry_safe(timer, tmp, &timer_head, struct timer_list,
				 entry) {
		if (timer->expires > now)
			break;
		list_del(&timer->entry);
		list_add_tail(&timer->entry, &expired);
	}

	/* Fire outside the main list so callbacks can add/del safely. */
	list_for_each_entry_safe(timer, tmp, &expired, struct timer_list,
				 entry) {
		list_del(&timer->entry);
		timer->entry.next = NULL;	/* mark fired */
		if (timer->function)
			timer->function(timer);
	}
}
