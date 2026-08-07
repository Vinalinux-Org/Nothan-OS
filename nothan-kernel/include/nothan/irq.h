#ifndef _IRQ_H
#define _IRQ_H

/*
 * INTC is at PA 0x48200000, mapped to VA 0xF0200000
 * via the MMU's L4_PER section (VA 0xF0000000 → PA 0x48000000).
 */
#define INTC_BASE		0xF0200000

#define INTC_SIR_IRQ		0x40
#define INTC_SIR_FIQ		0x44

/*
 * INTC_SIR_IRQ layout (AM335x TRM 6.6.1.4, reset = 0xFFFFFF80):
 *   [6:0]  ActiveIRQ    - the interrupt number the priority sorter picked
 *   [31:7] SpuriousIRQ  - non-zero means that number is NOT valid
 *
 * The spurious field mattering is why the mask below is not the whole story:
 * reading only [6:0] turns "the sorter had nothing to tell you" into
 * "interrupt 127", and 127 then looks like an ordinary unhandled line. The
 * two have to stay distinguishable, because a spurious storm and a driver
 * that forgot to register are different faults with different fixes.
 */
#define INTC_SIR_ACTIVE_MASK	0x0000007FU
#define INTC_SIR_SPURIOUS_MASK	0xFFFFFF80U
#define INTC_CONTROL		0x48
#define INTC_THRESHOLD		0x68
#define INTC_MIR(n)		(0x84 + ((n) * 0x20))
#define INTC_MIR_CLEAR(n)	(0x88 + ((n) * 0x20))
#define INTC_MIR_SET(n)		(0x8C + ((n) * 0x20))
#define INTC_ILR(n)		(0x100 + ((n) * 4))

#define NEWIRQAGR		(1 << 0)

#define NR_IRQS			128

/* Hard-IRQ handler: runs in interrupt context, with interrupts masked. */
typedef void (*irq_handler_t)(unsigned int irq);

/*
 * THREADED INTERRUPTS
 *
 * Everything used to happen in hard-IRQ context, with interrupts masked for
 * the whole handler. That is fine for what exists today - the tick increments
 * a counter, the UART pushes a byte into a ring - and wrong for what comes
 * next. Handling a received packet means a checksum, a table lookup, a queue
 * insertion and possibly a wakeup; doing that with interrupts off means the
 * next packet's interrupt is late or lost, and the deafness lasts as long as
 * the slowest path through the handler.
 *
 * The split: a small hard handler silences the device and says whether there
 * is more to do; a per-line kernel thread does the rest with interrupts ON.
 *
 *   hard handler  -> IRQ_HANDLED      nothing further; done in interrupt ctx
 *                 -> IRQ_WAKE_THREAD  core masks the line and wakes the thread
 *   thread        runs thread_fn(irq), then the core re-arms the line
 *
 * Masking the line before waking is not optional. AM335x peripheral
 * interrupts are level-triggered, so a source the hard handler did not fully
 * clear is still asserted when the ISR returns; without the mask the CPU would
 * re-enter the dispatcher immediately and keep re-entering it, and the thread
 * that was supposed to clear the source would never get to run.
 *
 * WHY THREADS RATHER THAN SOFTIRQS. A softirq would be faster - no context
 * switch - but it runs in a context that has no name, no PID, and no line in
 * ps, so when it hangs the log can only report that the machine is busy. Every
 * deferred handler here is a task: it can be seen, timed, and blamed. On a
 * board whose only instrument is a UART, being able to name the thing that
 * stopped is worth more than the switch it costs.
 *
 * request_irq() remains, and is still right for handlers that genuinely finish
 * in a few instructions - the tick and the UART receive path. Deferring those
 * would cost a context switch to do less work than the switch.
 */
#define IRQ_HANDLED		0
#define IRQ_WAKE_THREAD		1

typedef int  (*irq_hard_t)(unsigned int irq);	/* interrupt context */
typedef void (*irq_thread_t)(unsigned int irq);	/* task context, IRQs on */

/*
 * request_irq() failures. There are exactly two, and both used to be silent.
 *
 *   IRQ_ERR_RANGE  the line does not exist on this controller
 *   IRQ_ERR_BUSY   somebody already owns it
 *
 * BUSY is the one that matters. The old request_irq() returned void and simply
 * overwrote irq_handlers[irq], so the second driver to claim a line won and the
 * first went deaf - with no message, no failed probe, and nothing in the log to
 * connect the two. The symptom is "driver X stopped receiving interrupts",
 * which is about the hardest thing there is to trace from a UART, and the cause
 * is a line in a different driver's probe.
 *
 * Registration can now fail, which means probes must check it. That is the
 * point: a driver that cannot get its interrupt has not probed successfully and
 * should say so, not run half-alive.
 */
#define IRQ_ERR_RANGE		(-1)
#define IRQ_ERR_BUSY		(-2)

void intc_init(void);
void intc_enable_irq(unsigned int irq);
void intc_disable_irq(unsigned int irq);
void intc_handle_irq(void);

int  request_irq(unsigned int irq, irq_handler_t handler, const char *name);
int  request_threaded_irq(unsigned int irq, irq_hard_t hard,
			  irq_thread_t thread_fn, const char *name);
void free_irq(unsigned int irq);
void irq_show_stats(void);	/* per-line counts + spurious/unhandled totals */

#endif /* _IRQ_H */
