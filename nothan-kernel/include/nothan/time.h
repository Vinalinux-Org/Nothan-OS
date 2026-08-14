#ifndef _NOTHAN_TIME_H
#define _NOTHAN_TIME_H

#include <nothan/timer.h>

/*
 * Ticks per second, derived from the one place the tick period is stated.
 *
 * This used to be a literal 100 with a comment saying "10 ms", while the timer
 * driver held its own copy of the same fact and the scheduler a third.  Three
 * places to change and no way for two of them to disagree loudly: taking
 * TICK_MS to 1 while this stayed at 100 would have made every msleep() sleep a
 * tenth as long as asked, silently, since nothing converts milliseconds to
 * ticks except through here.
 *
 * The same shape of mistake has now cost this project three separate bugs — a
 * build that rebuilt nothing, a reload value that stopped matching its comment,
 * and this — so the rule is worth stating plainly: a number the machine
 * depends on gets exactly one definition, and everything else derives from it.
 */
#define HZ		TICK_HZ

unsigned long get_jiffies(void);

#endif /* _NOTHAN_TIME_H */
