/*
 * kernel/irq/irq_core.c - IRQ descriptor table and handler dispatch
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/config.h>
#include <nothan/timer.h>
#include <nothan/printk.h>
#include <asm/barrier.h>

static irq_handler_t irq_handlers[NR_IRQS];

#if CONFIG_IRQ_OFF_TIMING
/*
 * PROTECTION: none needed — every write happens with interrupts masked, by
 * construction: irqoff_enter() runs just after the mask goes on and
 * irqoff_leave() just before it comes off.  Read from panic(), after
 * everything else has stopped.
 */
static u64 irqoff_start;	/* 0 when no measured region is open */
static void *irqoff_site;	/* who opened the region currently being timed */
static u32 irqoff_max_cyc;
static void *irqoff_max_site;
static unsigned long irqoff_regions;

void irqoff_enter(void *site)
{
	/*
	 * Overwrite rather than refuse if a region is somehow already open.
	 * One can be left dangling by the unconditional forms — the idle loop
	 * re-enables with raw asm beside its WFI — and carrying a stale start
	 * forward would turn the next ordinary critical section into a
	 * fictional maximum.  Losing a sample is the cheaper error, and a
	 * fictional worst case is the expensive one: it would send someone
	 * looking for a critical section that never existed.
	 */
	irqoff_start = timer_cycles();
	irqoff_site  = site;
}

void irqoff_leave(void)
{
	u32 d;

	if (!irqoff_start)
		return;

	d = (u32)(timer_cycles() - irqoff_start);
	irqoff_start = 0;
	irqoff_regions++;

	if (d > irqoff_max_cyc) {
		irqoff_max_cyc = d;
		irqoff_max_site = irqoff_site;
	}
}
#endif /* CONFIG_IRQ_OFF_TIMING */

void irqoff_dump(void)
{
#if CONFIG_IRQ_OFF_TIMING
	printk("  irq-off: %lu regions, worst %lu us (%lu cyc) from %p\n",
	       irqoff_regions,
	       (unsigned long)cycles_to_us(irqoff_max_cyc),
	       (unsigned long)irqoff_max_cyc, irqoff_max_site);
	printk("           budget is 100 us — 1%% of the 10 ms audio period\n");
#endif
}

#if CONFIG_IRQ_TIMING
/*
 * Time spent inside each handler.
 *
 * A measurement mode rather than something left running, and the reason is the
 * difference from the scheduler accounting, which *is* always on.  That one
 * runs per context switch — hundreds a second.  This runs per interrupt —
 * thousands a second once the tick goes to 1 ms — and it sits on the shortest
 * latency path in the machine, ahead of the handler that wakes a task.  The
 * cost is two clocksource reads, which is small but not free, and paying it
 * forever to answer a question that is asked occasionally is the wrong trade.
 *
 * PROTECTION: none needed.  Written only from interrupt context on a single
 * core, so no two updates overlap; read only from panic(), after everything
 * else has stopped.
 */
struct irq_stat {
	u32	count;
	u32	max_cyc;	/* worst single run of this handler */
	u32	total_us;	/* accumulated, microseconds */
	u32	cyc_rem;	/* cycles not yet folded into total_us */
};

static struct irq_stat irq_stats[NR_IRQS];

static void irq_account(unsigned int irq, u32 cyc)
{
	struct irq_stat *s = &irq_stats[irq];
	u32 rem;

	s->count++;

	if (cyc > s->max_cyc)
		s->max_cyc = cyc;

	/* Same split as the scheduler accounting: keep the remainder so a line
	 * interrupting thousands of times does not truncate away most of its
	 * own cost, one interrupt at a time. */
	rem = s->cyc_rem + (cyc % TSC_CYCLES_PER_US);
	s->total_us += cyc / TSC_CYCLES_PER_US + rem / TSC_CYCLES_PER_US;
	s->cyc_rem   = rem % TSC_CYCLES_PER_US;
}
#endif /* CONFIG_IRQ_TIMING */

void irq_dump_stats(void)
{
#if CONFIG_IRQ_TIMING
	unsigned int irq, seen = 0;

	for (irq = 0; irq < NR_IRQS; irq++) {
		struct irq_stat *s = &irq_stats[irq];

		if (!s->count)
			continue;

		if (!seen++)
			printk("  irq handlers (time inside the handler only):\n");

		printk("    irq %3u: n=%lu total=%lu us worst=%lu us (%lu cyc)\n",
		       irq, (unsigned long)s->count, (unsigned long)s->total_us,
		       (unsigned long)cycles_to_us(s->max_cyc),
		       (unsigned long)s->max_cyc);
	}

	if (!seen)
		printk("  irq handlers: none fired\n");
#endif
}

/**
 * request_irq() - Register an interrupt handler
 * @irq: The interrupt number to register for
 * @handler: The function to be called when the IRQ occurs
 */
void request_irq(unsigned int irq, irq_handler_t handler)
{
	if (irq < NR_IRQS)
		irq_handlers[irq] = handler;
}

/**
 * intc_handle_irq() - Top-level IRQ handler
 *
 * Called by the assembly IRQ vector. Reads the active IRQ number
 * from the INTC, calls the registered handler, and acknowledges the IRQ.
 */
void intc_handle_irq(void)
{
	u32 sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);
	u32 irq = sir & 0x7F;

	if (irq_handlers[irq]) {
#if CONFIG_IRQ_TIMING
		/*
		 * Around the handler only, not around the INTC ack below: the
		 * question §9.2 asks is how long a driver holds the CPU with
		 * interrupts off, and the ack is this file's own cost, the same
		 * on every line.
		 */
		u64 t0 = timer_cycles();

		irq_handlers[irq](irq);
		irq_account(irq, (u32)(timer_cycles() - t0));
#else
		irq_handlers[irq](irq);
#endif
	}

	/*
	 * Ensure the handler's device-side ack (posted MMIO writes clearing the
	 * source) has drained before re-arming the INTC — otherwise a level line
	 * still asserted at NEWIRQAGR time is latched as a spurious fresh IRQ.
	 * The trailing barrier makes NEWIRQAGR complete before we return and IRQs
	 * are re-enabled.
	 */
	dsb();
	mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);
	dsb();
}
