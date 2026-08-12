#ifndef _NOTHAN_TIMER_H
#define _NOTHAN_TIMER_H

#include <nothan/types.h>
#include <nothan/mm.h>

struct timer_list {
	struct list_head entry;
	unsigned long expires;
	void (*function)(struct timer_list *);
	unsigned long data;
};

#define init_timer(timer)						\
	do {								\
		(timer)->entry.next = NULL;				\
		(timer)->function = NULL;				\
		(timer)->data = 0;					\
	} while (0)

#define timer_pending(timer)	((timer)->entry.next != NULL)

void add_timer(struct timer_list *timer);
int  del_timer(struct timer_list *timer);
int  mod_timer(struct timer_list *timer, unsigned long expires);
void run_local_timers(void);
void timer_start(void);

/*
 * Free-running 24 MHz clocksource (DMTimer3, no interrupt).
 * timer_cycles() is monotonic and safe from any context.
 * cycles_to_us() takes a 32-bit delta only — see the note in the driver.
 *
 * clocksource_ready() is false until the driver has started the counter;
 * anything that busy-waits before then has to fall back to counting
 * instructions, because timer_cycles() would just keep returning the same
 * value and the wait would never end.
 */
#define TSC_CYCLES_PER_US	24u

u64 timer_cycles(void);
u32 cycles_to_us(u32 cycles);
int clocksource_ready(void);

#endif /* _NOTHAN_TIMER_H */
