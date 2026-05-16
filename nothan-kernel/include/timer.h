/*
 * include/timer.h — AM335x DMTimer2 driver interface — init/tick/delay functions.
 */

#ifndef TIMER_H
#define TIMER_H

#include "types.h"



/* DMTimer2 base address */
#define DMTIMER2_BASE       0x48040000

/* Timer IRQ number */
#define TIMER2_IRQ          68



/* Configures Timer2 for periodic IRQ. Caller must enable IRQ in CPSR. */
void timer_init(void);

uint32_t timer_get_ticks(void);

/* Polled delay; requires timer_init() (DMTimer2 free-run via TCRR) first. */
void delay_ms(uint32_t ms);

#endif /* TIMER_H */