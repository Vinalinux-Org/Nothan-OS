/*
 * include/sleep.h — Sleep and msleep interface for task-blocking delays.
 */

#ifndef SLEEP_H
#define SLEEP_H

#include "types.h"

/* 10 ms per tick (matches DMTimer2 reload). */
#define TICK_MS  10

extern volatile uint32_t jiffies;

void msleep(uint32_t ms);

/* IRQ context — wakes expired sleepers. */
void sleep_tick(void);

#endif
