/*
 * kernel/irq/irq_core.c - IRQ descriptor table and handler dispatch
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>
#include <nothan/sched.h>
#include <nothan/wait.h>
#include <asm/barrier.h>
#include <asm/irqflags.h>

/**
 * struct irq_desc - what the kernel knows about one interrupt line
 * @handler: the registered handler, or NULL if nobody claimed this line
 * @name:    who claimed it, for logs and irq_show_stats()
 * @count:   how many times it has fired
 *
 * @name and @count are not decoration. With no JTAG, "which lines are live and
 * how often do they fire" is otherwise only answerable by adding a printk to an
 * ISR and rebuilding - and a printk in an ISR distorts the timing of the very
 * thing being measured. A counter costs one add per interrupt and can be read
 * later, from outside interrupt context.
 */
struct irq_desc {
	irq_handler_t handler;		/* hard-only line (request_irq) */
	const char   *name;
	unsigned long count;

	/* Threaded line (request_threaded_irq); all NULL/0 otherwise. */
	irq_hard_t             hard;
	irq_thread_t           thread_fn;
	struct task_struct    *thread;
	struct wait_queue_head thread_wq;
	volatile int           pending;
};

static struct irq_desc irq_desc[NR_IRQS];

/* A line is claimed if either kind of handler sits on it. */
static inline int irq_taken(const struct irq_desc *d)
{
	return d->handler || d->hard;
}

/*
 * Faults the dispatcher can see but no individual driver can. Counted rather
 * than printed per occurrence: both arrive in storms by nature, and a printk
 * per event would hold IRQs off long enough to cause more of them.
 */
static unsigned long irq_spurious;	/* sorter reported no valid number */
static unsigned long irq_unhandled;	/* valid number, nobody registered */

/**
 * request_irq() - claim an interrupt line
 * @irq:     line number
 * @handler: called from IRQ context, with IRQs masked
 * @name:    owner, for logs
 *
 * Refuses a line that already has an owner rather than taking it. Two drivers
 * wanting the same line is always a bug - a wrong constant, a copy-pasted
 * probe, a board file listing a device twice - and the old behaviour (last
 * writer wins, silently) turned that bug into a DIFFERENT driver's mystery:
 * the first driver simply stopped receiving interrupts, with nothing anywhere
 * connecting the two.
 *
 * Return: 0, IRQ_ERR_RANGE, or IRQ_ERR_BUSY.
 */
int request_irq(unsigned int irq, irq_handler_t handler, const char *name)
{
	unsigned long flags;
	int ret = 0;

	if (irq >= NR_IRQS || !handler) {
		pr_err("[IRQ] request_irq(%u, \"%s\"): no such line (max %u)\n",
		       irq, name ? name : "?", NR_IRQS - 1);
		return IRQ_ERR_RANGE;
	}

	/*
	 * Claim under the mask. Probes all run before the tick starts today, so
	 * nothing races them yet; a driver brought up later would be registering
	 * for interrupts that can already fire, and the test and the store must
	 * not be separable or two claimants both see a free slot.
	 */
	local_irq_save(flags);
	if (irq_taken(&irq_desc[irq])) {
		ret = IRQ_ERR_BUSY;
	} else {
		irq_desc[irq].handler = handler;
		irq_desc[irq].name    = name;
		irq_desc[irq].count   = 0;
	}
	local_irq_restore(flags);

	if (ret == IRQ_ERR_BUSY)
		pr_err("[IRQ] request_irq(%u, \"%s\"): already owned by \"%s\"\n",
		       irq, name ? name : "?",
		       irq_desc[irq].name ? irq_desc[irq].name : "?");

	return ret;
}

/*
 * The body every per-line IRQ thread runs. One function, not one per line:
 * @arg is the line number, delivered through task_create()'s argument slot.
 */
static void irq_thread_main(void *arg)
{
	unsigned int irq = (unsigned int)(unsigned long)arg;
	struct irq_desc *d = &irq_desc[irq];

	for (;;) {
		wait_event(&d->thread_wq, d->pending);
		d->pending = 0;

		d->thread_fn(irq);

		/*
		 * Re-arm only now. The line has been masked since the hard
		 * handler asked for us, which is what has been keeping a
		 * still-asserted source from re-entering the dispatcher in a
		 * loop. Unmasking any earlier - before the work that clears the
		 * source is done - puts that loop back.
		 *
		 * It also means no second event for this line can be in flight,
		 * which is why @pending can be a plain flag rather than a count.
		 */
		intc_enable_irq(irq);
	}
}

/**
 * request_threaded_irq() - claim a line with a hard handler and a thread
 * @irq:       line number
 * @hard:      runs in interrupt context; must silence the device and return
 *             IRQ_HANDLED or IRQ_WAKE_THREAD
 * @thread_fn: runs in task context with interrupts enabled
 * @name:      owner, and the thread's name in ps
 *
 * Return: 0, IRQ_ERR_RANGE, or IRQ_ERR_BUSY.
 */
int request_threaded_irq(unsigned int irq, irq_hard_t hard,
			 irq_thread_t thread_fn, const char *name)
{
	unsigned long flags;
	struct task_struct *t;

	if (irq >= NR_IRQS || !hard || !thread_fn) {
		pr_err("[IRQ] request_threaded_irq(%u, \"%s\"): bad arguments\n",
		       irq, name ? name : "?");
		return IRQ_ERR_RANGE;
	}

	/*
	 * Claim the slot before creating the thread. The other order would
	 * leave a thread with nothing to serve if the line turned out to be
	 * taken, and it would already be on the runqueue by the time we found
	 * out — a kernel thread cannot be un-created.
	 */
	local_irq_save(flags);
	if (irq_taken(&irq_desc[irq])) {
		local_irq_restore(flags);
		pr_err("[IRQ] request_threaded_irq(%u, \"%s\"): already owned by \"%s\"\n",
		       irq, name ? name : "?",
		       irq_desc[irq].name ? irq_desc[irq].name : "?");
		return IRQ_ERR_BUSY;
	}
	irq_desc[irq].hard      = hard;
	irq_desc[irq].thread_fn = thread_fn;
	irq_desc[irq].name      = name;
	irq_desc[irq].count     = 0;
	irq_desc[irq].pending   = 0;
	init_waitqueue_head(&irq_desc[irq].thread_wq);
	local_irq_restore(flags);

	t = task_create(irq_thread_main, (void *)(unsigned long)irq,
			DEFAULT_PRIO, name);
	if (!t) {
		local_irq_save(flags);
		irq_desc[irq].hard      = NULL;
		irq_desc[irq].thread_fn = NULL;
		irq_desc[irq].name      = NULL;
		local_irq_restore(flags);
		pr_err("[IRQ] request_threaded_irq(%u, \"%s\"): no thread\n",
		       irq, name ? name : "?");
		return IRQ_ERR_BUSY;
	}

	irq_desc[irq].thread = t;
	enqueue_task(&runqueue, t);
	return 0;
}

/**
 * free_irq() - release a line claimed by request_irq()
 *
 * Masks the line at the controller BEFORE dropping the handler. The other
 * order leaves a window in which the line is still enabled and the handler
 * pointer is already NULL; the dispatcher would count that as unhandled and
 * mask it anyway - the same end state, reached via an error message that
 * describes a bug nobody made.
 */
void free_irq(unsigned int irq)
{
	unsigned long flags;

	if (irq >= NR_IRQS)
		return;

	intc_disable_irq(irq);

	local_irq_save(flags);
	irq_desc[irq].handler = NULL;
	irq_desc[irq].name    = NULL;
	local_irq_restore(flags);
}

/**
 * irq_show_stats() - dump the interrupt table
 *
 * Task context only: this is several printk lines, each of which busy-waits on
 * the UART with IRQs masked.
 */
void irq_show_stats(void)
{
	printk("[IRQ] line  count       kind      owner\n");
	for (unsigned int i = 0; i < NR_IRQS; i++)
		if (irq_taken(&irq_desc[i]))
			printk("[IRQ] %4u  %-10lu  %-8s  %s\n",
			       i, irq_desc[i].count,
			       irq_desc[i].hard ? "threaded" : "hard",
			       irq_desc[i].name ? irq_desc[i].name : "?");
	printk("[IRQ] spurious=%lu unhandled=%lu\n", irq_spurious, irq_unhandled);
}

/**
 * intc_handle_irq() - Top-level IRQ handler
 *
 * Called by the assembly IRQ vector. Resolves the active line, runs its
 * handler, and acknowledges the controller.
 */
void intc_handle_irq(void)
{
	u32 sir = mmio_read32(INTC_BASE + INTC_SIR_IRQ);
	u32 irq = sir & INTC_SIR_ACTIVE_MASK;

	/*
	 * A set spurious field means the priority sorter had nothing valid to
	 * report and [6:0] is meaningless. Masking the field off (as this used
	 * to) turns every spurious interrupt into a plausible-looking "line
	 * 127", so a genuine spurious storm reads as a driver problem on a line
	 * no driver owns - and the search starts in the wrong place entirely.
	 */
	if (sir & INTC_SIR_SPURIOUS_MASK) {
		irq_spurious++;
		goto ack;
	}

	if (irq_desc[irq].hard) {
		irq_desc[irq].count++;
		if (irq_desc[irq].hard(irq) == IRQ_WAKE_THREAD) {
			/*
			 * Mask BEFORE waking, and before the ack below. The
			 * source may still be asserted - clearing it is the
			 * thread's job - so an ack with the line live would
			 * re-enter this function at once and go on doing so,
			 * never yielding to the thread that would stop it.
			 */
			intc_disable_irq(irq);
			irq_desc[irq].pending = 1;
			wake_up(&irq_desc[irq].thread_wq);
		}
		goto ack;
	}

	if (irq_desc[irq].handler) {
		irq_desc[irq].count++;
		irq_desc[irq].handler(irq);
		goto ack;
	}

	/*
	 * Nobody owns this line. Mask it, once, and say so.
	 *
	 * Masking is survival, not tidiness. AM335x peripheral interrupts are
	 * level-triggered: the source stays asserted until a driver clears it,
	 * and with no driver there is nothing to clear it. Acknowledging
	 * without masking re-arms the controller into the same still-asserted
	 * line, so the CPU re-enters this function immediately, and forever.
	 * The machine does not crash - it stops making progress while producing
	 * no output, which is the hardest failure there is to diagnose over a
	 * UART, because the UART is starved along with everything else.
	 *
	 * Masked, the line goes quiet and the single error below survives to
	 * name the number nobody claimed.
	 */
	irq_unhandled++;
	intc_disable_irq(irq);
	pr_err("[IRQ] line %u fired with no handler - masked\n", (unsigned int)irq);

ack:
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
