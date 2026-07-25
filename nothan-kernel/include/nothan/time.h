#ifndef _NOTHAN_TIME_H
#define _NOTHAN_TIME_H

#include <nothan/types.h>

/* Timer tick: 10 ms */
#define HZ		100

unsigned long get_jiffies(void);

/*
 * sched_clock() - monotonic time in nanoseconds, for vruntime accounting.
 * Provisional (10 ms jiffie granularity) until P0 gives a fine-grained
 * DMTimer3 free-running clocksource.
 */
u64 sched_clock(void);

#endif /* _NOTHAN_TIME_H */
