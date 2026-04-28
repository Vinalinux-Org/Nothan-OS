/* ============================================================
 * timer.h
 * ------------------------------------------------------------
 * AM335x DMTimer2 driver interface.
 * ============================================================ */

#ifndef TIMER_H
#define TIMER_H

#include "types.h"

/* ============================================================
 * Timer Configuration
 * ============================================================ */

/* DMTimer2 base address */
#define DMTIMER2_BASE       0x48040000

/* Timer IRQ number */
#define TIMER2_IRQ          68

/* ============================================================
 * Timer API
 * ============================================================ */

/* Configures Timer2 for periodic IRQ. Caller must enable IRQ in CPSR. */
void timer_init(void);

uint32_t timer_get_ticks(void);

/* Enables DMTimer2 clock + free-running mode for delay_ms().
 * timer_init() later reconfigures it for scheduler interrupts. */
void timer_early_init(void);

/* Polled delay; requires timer_early_init() or timer_init() first. */
void delay_ms(uint32_t ms);

#endif /* TIMER_H */