/*
 * arch/arm/kernel/cpustat.c — CPU usage measurement via ARM PMCCNTR
 *
 * idle_task wraps each WFI with cpustat_wfi_begin/end to accumulate idle cycles.
 * net_task calls cpustat_update() once per second to compute the final %.
 * Captures all CPU work including IRQ handlers — task-tick counting cannot.
 */

#include "cpustat.h"
#include "types.h"
#include "vinix/init.h"

static inline uint32_t pmccntr_read(void)
{
    uint32_t c;
    __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(c));
    return c;
}

static volatile uint32_t wfi_cycles_acc  = 0;
static uint32_t          window_start_cc = 0;
static uint32_t          cpu_pct_cached  = 0;

int cpustat_init(void)
{
    uint32_t pmcr;

    /* PMCNTENSET bit 31: enable cycle counter.
     * PMCR bit 0: global enable, bit 2: reset cycle counter. */
    __asm__ volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(1u << 31));
    __asm__ volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(pmcr));
    pmcr |= (1u << 0) | (1u << 2);
    __asm__ volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(pmcr));
    window_start_cc = pmccntr_read();
    return 0;
}

arch_initcall(cpustat_init);

uint32_t cpustat_wfi_begin(void)
{
    return pmccntr_read();
}

void cpustat_wfi_end(uint32_t start_cc)
{
    wfi_cycles_acc += pmccntr_read() - start_cc;
}

void cpustat_update(void)
{
    uint32_t now_cc    = pmccntr_read();
    uint32_t window_cc = now_cc - window_start_cc;  /* uint32 wrap is safe */
    uint32_t wfi_cc    = wfi_cycles_acc;

    wfi_cycles_acc  = 0;
    window_start_cc = now_cc;

    if (window_cc > 0) {
        uint32_t busy = (window_cc > wfi_cc) ? (window_cc - wfi_cc) : 0;
        cpu_pct_cached = (busy * 100) / window_cc;
    }
}

uint32_t cpustat_pct(void)
{
    return cpu_pct_cached;
}
