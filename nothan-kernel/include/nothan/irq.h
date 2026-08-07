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

/*
 * Interrupt handler. Runs in interrupt context with interrupts masked, and
 * that is the ONLY context an interrupt is handled in.
 *
 * There is no bottom half, and that is a decision rather than a gap. Deferred
 * work needs somewhere to run, and the two usual answers both cost more here
 * than they are worth: a softirq runs in a context with no name, no PID and no
 * line in ps, so when it wedges the log can only report that the machine is
 * busy; a kernel thread shares the kernel's address space with every other
 * kernel thread, which is exactly the shared mutable state
 * Documentation/design-philosophy.md §5.1.1 removes by construction everywhere
 * else.
 *
 * The model instead is: the handler does the least it can - move the device
 * out of the way, park the data - and a USER PROCESS picks it up over IPC and
 * does the real work in its own address space. storage_daemon is the shape.
 * It costs a copy and a context switch; it buys back a failure that can be
 * named, isolated, and restarted.
 *
 * The obligation that puts on a handler is real: everything it does happens
 * with interrupts off, so it has to be short, and "short" has to be true of
 * the worst path through it and not the usual one.
 */
typedef void (*irq_handler_t)(unsigned int irq);

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
void free_irq(unsigned int irq);
void irq_show_stats(void);	/* per-line counts + spurious/unhandled totals */

#endif /* _IRQ_H */
