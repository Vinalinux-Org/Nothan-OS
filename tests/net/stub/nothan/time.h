#ifndef _NOTHAN_TIME_H
#define _NOTHAN_TIME_H
/*
 * Host stand-in for the tick.  The harness drives frames one at a time and
 * never waits, so there is no real time here to measure — the value only has
 * to make DEFINE_RATELIMIT's arithmetic compile.
 */
#define HZ	1000UL
unsigned long get_jiffies(void);
#endif
