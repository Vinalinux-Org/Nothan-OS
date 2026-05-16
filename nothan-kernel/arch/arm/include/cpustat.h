/*
 * arch/arm/include/cpustat.h — ARM Cortex-A8 CPU usage measurement via PMCCNTR
 *
 * Measures real CPU% by comparing actual cycles in WFI vs. total elapsed cycles.
 * Captures IRQ handler time — invisible to task-based tick counting.
 */

#ifndef CPUSTAT_H
#define CPUSTAT_H

#include "types.h"

int cpustat_init(void);

/* Called in idle_task around WFI — feeds idle cycle data into the measurement. */
uint32_t cpustat_wfi_begin(void);
void     cpustat_wfi_end(uint32_t start_cc);

/* Called once per second from net_task — computes cpu_pct over the last window. */
void     cpustat_update(void);
uint32_t cpustat_pct(void);

#endif /* CPUSTAT_H */
