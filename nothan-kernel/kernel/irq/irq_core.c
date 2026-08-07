/*
 * kernel/irq/irq_core.c - IRQ descriptor table and handler dispatch
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>
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
	irq_handler_t handler;
	const char   *name;
	unsigned long count;
};

static struct irq_desc irq_desc[NR_IRQS];

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
	if (irq_desc[irq].handler) {
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
	printk("[IRQ] line  count       owner\n");
	for (unsigned int i = 0; i < NR_IRQS; i++)
		if (irq_desc[i].handler)
			printk("[IRQ] %4u  %-10lu  %s\n",
			       i, irq_desc[i].count,
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
