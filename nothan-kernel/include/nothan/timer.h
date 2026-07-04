#ifndef _NOTHAN_TIMER_H
#define _NOTHAN_TIMER_H

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

#endif /* _NOTHAN_TIMER_H */
