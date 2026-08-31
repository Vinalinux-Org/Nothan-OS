/*
 * drivers/tty/serial/omap-serial.c - OMAP UART driver (multi-instance)
 *
 * UART0 = debug console (/dev/ttyS0), clocked by the bootloader.
 * UART1 = SIM7600 modem (/dev/uart1), clocked here via CM_PER_UART1_CLKCTRL.
 * Both share one per-instance driver: interrupt-driven RX ring, polled TX.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <asm/irqflags.h>
#include <nothan/types.h>
#include <nothan/config.h>
#include <nothan/uart.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>
#include <nothan/platform.h>
#include <nothan/init.h>
#include <nothan/cdev.h>
#include <nothan/ioctl.h>
#include <nothan/fs.h>
#include <nothan/pinctrl.h>
#include <nothan/wait.h>

#define RX_BUF_SIZE	4096u		/* power of two; headroom for long UCS2 SMS */
#define RX_BUF_MASK	(RX_BUF_SIZE - 1u)

struct uart_inst {
	u32          base;	/* register VA */
	u32          pa;	/* register PA (matches platform_device.base) */
	unsigned int irq;
	u32          clkctrl;	/* PRCM clkctrl VA; 0 = already clocked (console) */
	u8           rx_buf[RX_BUF_SIZE];
	volatile unsigned int rx_head;	/* ISR writes */
	volatile unsigned int rx_tail;	/* read() consumes */
	struct wait_queue_head rx_wait;	/* readers sleep here when it is empty */
};

/*
 * PROTECTION: the RX ring is single-producer (the IRQ handler advances rx_head)
 * and single-consumer (read() advances rx_tail), each owning its own index, so
 * the handoff itself needs no lock.
 *
 * The reader still masks, for a different reason: it has to test "is the ring
 * empty" and go to sleep as one indivisible step, or the interrupt can deliver
 * a byte and wake an empty queue in between, and the sleeper never wakes.
 *
 * TX masks because several unrelated callers push bytes at the same device.
 */
static struct uart_inst uarts[] = {
	{ .base = UART_BASE,  .pa = UART0_PA, .irq = UART_IRQ,  .clkctrl = 0 },
	{ .base = UART1_VA,   .pa = UART1_PA, .irq = UART1_IRQ, .clkctrl = CM_PER_UART1_CLKCTRL },
};
#define NR_UART		(sizeof(uarts) / sizeof(uarts[0]))

static unsigned int uart_current_baud = 115200;	/* console (UART0) */

/*
 * Sixteen kilobytes, up from four.
 *
 * Four was enough for a line at a time and not for a burst: a USB camera's
 * configuration descriptors print as about sixty lines, which is 4260 bytes,
 * and the tail went over the edge.  Sixteen holds any burst this box currently
 * produces, at 12 KB of .bss out of 506 MB.
 *
 * It only moves the threshold, though — which is why the notice below matters
 * more than the size.  A ring that drops is survivable; a ring that drops
 * without saying so has cost this project four wrong diagnoses.
 */
#define LOG_BUF_SIZE	16384u			/* power of two */
#define LOG_BUF_MASK	(LOG_BUF_SIZE - 1u)

static char log_buf[LOG_BUF_SIZE];
static volatile unsigned int log_head;		/* producers append here */
static volatile unsigned int log_tail;		/* the drainer consumes here */
static volatile int log_draining;		/* polled path: one drainer at a time */
static volatile unsigned int log_dropped;	/* bytes lost to a full ring */

/*
 * Once the console UART's interrupt is live, the ring empties itself from the
 * TX handler and no caller ever waits on the wire.  Before that — and there is
 * a lot of boot before that — there is nobody to do it, so writers fall back
 * to draining by polling.
 */
static volatile int log_irq_ready;	/* TX interrupt usable */
static volatile int log_tx_armed;	/* THR_IT enabled; the ISR owns the ring */

/* Arm the TX interrupt if there is work and it is not already running.
 * Caller must hold the interrupt mask. */
/*
 * Say that bytes were lost, from a context that cannot call printk.
 *
 * printk appends to this same ring, so calling it from the drain path is a
 * recursion into the thing being drained.  The message is therefore written
 * straight into the buffer, which is safe precisely where it is called: the
 * ring has just gone empty, so every byte of it is free.
 *
 * The caller must hold the interrupt mask and must not disarm the transmitter
 * afterwards — there is data again.
 */
static void log_note_dropped_locked(void)
{
	static const char pre[]  = "\r\n[log] ";
	static const char post[] = " bytes dropped (ring full)\r\n";
	char num[12];
	unsigned int n = log_dropped;
	unsigned int i = 0, j;

	log_dropped = 0;

	if (!n)
		num[i++] = '0';
	while (n) {
		num[i++] = (char)('0' + n % 10u);
		n /= 10u;
	}

	for (j = 0; pre[j]; j++)
		log_buf[log_head++ & LOG_BUF_MASK] = pre[j];
	while (i)
		log_buf[log_head++ & LOG_BUF_MASK] = num[--i];
	for (j = 0; post[j]; j++)
		log_buf[log_head++ & LOG_BUF_MASK] = post[j];
}

static void log_tx_arm_locked(void)
{
	if (!log_irq_ready || log_tx_armed || log_head == log_tail)
		return;

	log_tx_armed = 1;
	mmio_write32(uarts[0].base + UART_IER,
		     mmio_read32(uarts[0].base + UART_IER) | IER_THR_IT);
}


/* ------------------------------------------------------------------ */
/* RX ring (single-producer ISR / single-consumer read) + polled TX    */
/* ------------------------------------------------------------------ */

#if CONFIG_IRQ_TIMING
/*
 * Why the UART interrupted, counted by source.
 *
 * IRQ timing put a number on something that had been invisible: 237658 entries
 * to this handler in a run whose tasks accounted for about 300 ms, averaging
 * 0.82 us each — the shape of a handler being re-entered with nothing to do,
 * not of one doing work.  Nothing here ever read IIR, so nothing knew which
 * source was asserting, and for a THR interrupt reading IIR is one of only two
 * ways to clear it (the other being a write to THR).
 *
 * Bucketing by IT_TYPE says which line is responsible.  It is also a change in
 * behaviour and worth being honest about: the read below clears a pending THR
 * interrupt, so if the storm disappears with this in place, that is itself the
 * diagnosis rather than a coincidence.
 *
 * NONE counts entries where the UART reports nothing pending at all — the
 * clearest possible signal that the handler is being called for a condition it
 * has already dealt with.
 */
#define IIR_BUCKETS	32
static u32 iir_count[IIR_BUCKETS];
static u32 iir_none;

void console_dump_irq_sources(void)
{
	static const struct { u8 type; const char *name; } names[] = {
		{ IIR_TYPE_MODEM,	"modem"      },
		{ IIR_TYPE_THR,		"THR (tx)"   },
		{ IIR_TYPE_RHR,		"RHR (rx)"   },
		{ IIR_TYPE_LINE_STATUS,	"line status"},
		{ IIR_TYPE_RX_TIMEOUT,	"rx timeout" },
		{ IIR_TYPE_XOFF,	"xoff"       },
		{ IIR_TYPE_MODEM_STATE,	"modem state"},
	};
	unsigned int i;

	printk("  uart irq sources:\n");
	if (iir_none)
		printk("    nothing pending: %lu\n", (unsigned long)iir_none);
	for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
		if (iir_count[names[i].type])
			printk("    %s: %lu\n", names[i].name,
			       (unsigned long)iir_count[names[i].type]);
}
#else
void console_dump_irq_sources(void) { }
#endif

/* Drain the RX FIFO into the per-port ring.  Returns 1 if anything arrived. */
static int uart_rx_drain(struct uart_inst *u)
{
	unsigned int before = u->rx_head;

	while (mmio_read32(u->base + UART_LSR) & LSR_DR) {
		u8 c = mmio_read32(u->base + UART_RHR);
		unsigned int next = (u->rx_head + 1) & RX_BUF_MASK;

		if (next != u->rx_tail) {
			u->rx_buf[u->rx_head] = c;
			u->rx_head = next;
		}
	}

	return u->rx_head != before;
}

/*
 * Hand the transmitter as much of the log ring as its FIFO has room for.
 *
 * Filled by the FIFO's actual free space, and entered only because the THR
 * interrupt said so — neither of which used to be true.  The old version asked
 * LSR[5] TXFIFOE whether to transmit, and that bit answers a different
 * question: the THR interrupt asserts once the FIFO has TX_FIFO_TRIG free
 * spaces (eight, as FCR is programmed here), while TXFIFOE is set only when
 * the FIFO is *completely* empty.  For the whole time a full FIFO was draining
 * the interrupt was asserted and this function declined to run: no byte
 * written, nothing disarmed, the cause untouched, and immediate re-entry.
 *
 * It cost roughly 190,000 handler entries and 200 ms of CPU in a 300 ms run —
 * most of the machine, for the duration of every console transmission, and
 * invisible until CONFIG_IRQ_TIMING put a number on it.  That is the argument
 * kernel-roadmap.md §9.2 makes, arriving as a bill: priority governs tasks,
 * nothing governs interrupts, so a handler misbehaving is unaccounted work
 * taken from whatever was running.
 *
 * TXFIFO_LVL gives the exact fill count, so every entry makes progress: at
 * least the trigger level of bytes leaves, or the ring empties and the
 * interrupt is disarmed.  No burst constant to guess at, and no overflow.
 */
static void uart_tx_fill(struct uart_inst *u)
{
	unsigned int room, n;

	if (u != &uarts[0] || !log_tx_armed)
		return;

	room = UART_TX_FIFO_SIZE - (mmio_read32(u->base + UART_TXFIFO_LVL) & 0xFF);

	for (n = 0; n < room && log_head != log_tail; n++)
		mmio_write32(u->base + UART_THR,
			     log_buf[log_tail++ & LOG_BUF_MASK]);

	if (log_head == log_tail) {
		/*
		 * Empty — but first, own up to anything that was thrown away.
		 *
		 * log_dropped has been counted since this driver was written
		 * and only ever read by the boot-time polling drain, which
		 * stops running the moment the TX interrupt goes live.  So in
		 * normal operation the ring dropped bytes in complete silence,
		 * and a log that goes quiet about being incomplete is worse
		 * than no log: it was blamed on line width, on volume, and
		 * twice on two subsystems printing over each other, and all
		 * four were this.
		 */
		if (log_dropped) {
			log_note_dropped_locked();
			return;		/* data again: leave the interrupt armed */
		}

		/* Nothing left — stop the interrupt, or it re-fires forever on
		 * an empty transmitter. */
		log_tx_armed = 0;
		mmio_write32(u->base + UART_IER,
			     mmio_read32(u->base + UART_IER) & ~IER_THR_IT);
	}
}

/*
 * Ask the UART which interrupt fired, and service that one.
 *
 * The previous shape inferred the source from status bits — LSR_DR for
 * receive, LSR_THRE for transmit — and never read IIR at all.  Inference is
 * how the storm above happened: TXFIFOE was treated as "the transmit interrupt
 * fired" when it actually means "the FIFO is entirely empty", so the handler
 * silently declined to service the very interrupt that had woken it.  IIR is
 * the register that answers the question being asked, and asking it is what
 * makes servicing the right source structural rather than a deduction that can
 * be wrong.
 *
 * Looping until IIR reports nothing pending drains several sources in one
 * entry — a character arriving while the transmitter wants more is ordinary —
 * and the bound stops a source nobody clears from wedging the machine here.
 * Reaching the bound is not silent: the interrupt is still asserted, so this
 * handler is re-entered, and CONFIG_IRQ_TIMING counts every entry.
 */
#define UART_IRQ_MAX_ROUNDS	64

static void uart_irq_handler(unsigned int irq)
{
	struct uart_inst *u = &uarts[0];
	int rx_woke = 0;
	int rounds;

	for (unsigned int i = 0; i < NR_UART; i++)
		if (uarts[i].irq == irq) {
			u = &uarts[i];
			break;
		}

	for (rounds = 0; rounds < UART_IRQ_MAX_ROUNDS; rounds++) {
		u32 iir = mmio_read32(u->base + UART_IIR);
		unsigned int type;

		if (iir & IIR_IT_PENDING)
			break;

		type = (iir >> IIR_IT_TYPE_SHIFT) & IIR_IT_TYPE_MASK;

#if CONFIG_IRQ_TIMING
		iir_count[type & (IIR_BUCKETS - 1)]++;
#endif

		switch (type) {
		case IIR_TYPE_RHR:
		case IIR_TYPE_RX_TIMEOUT:
			rx_woke |= uart_rx_drain(u);
			break;

		case IIR_TYPE_THR:
			uart_tx_fill(u);
			break;

		default:
			/*
			 * Line status, modem status and the rest are cleared by
			 * reading LSR/MSR.  Nothing here acts on them, but the
			 * read has to happen or the source stays asserted.
			 */
			(void)mmio_read32(u->base + UART_LSR);
			break;
		}
	}

	/*
	 * One wakeup for the whole handler, however many rounds it took: the
	 * reader cares that bytes arrived, not how the FIFO delivered them, and
	 * a wakeup per round would be repeated work at the highest priority in
	 * the system.
	 */
	if (rx_woke)
		wake_up(&u->rx_wait);

#if CONFIG_IRQ_TIMING
	/*
	 * Entered with the UART reporting nothing pending at all.  Counted
	 * separately because it is the signature of a source being serviced
	 * somewhere else, or of an interrupt that should never have reached
	 * here — either way not the same thing as a handler doing work.
	 */
	if (!rounds)
		iir_none++;
#endif
}

static void uart_tx_char(u32 base, int c)
{
	while (!(mmio_read32(base + UART_LSR) & LSR_THRE))
		;
	mmio_write32(base + UART_THR, c);
}

/*
 * Block until at least one byte, then return everything available.
 *
 * This used to return -1 on an empty ring, which meant callers polled: the
 * shell spun on read/yield and so did storage_daemon, so two tasks at the same
 * priority round-robinned forever, idle was never picked, and wfi had never
 * once executed in this kernel.  It also meant nothing in the system ever
 * genuinely slept — so there were no wakeups, and nothing for Phase 3 to
 * measure.
 */
static int uart_inst_read(struct uart_inst *u, char *buf, size_t count)
{
	unsigned long flags;
	size_t i = 0;

	flags = local_irq_save();

	while (u->rx_tail == u->rx_head)
		wait_event_locked(&u->rx_wait);

	while (i < count && u->rx_tail != u->rx_head) {
		buf[i++] = (char)u->rx_buf[u->rx_tail];
		u->rx_tail = (u->rx_tail + 1) & RX_BUF_MASK;
	}

	local_irq_restore(flags);
	return (int)i;
}

/*
 * Multi-byte write to a non-console UART.  Masked so one write's bytes are not
 * interleaved with another's.  The console does not use this — see the ring
 * below, which gets the same atomicity without the latency.
 */
static int uart_inst_write(struct uart_inst *u, const char *buf, size_t count)
{
	unsigned long flags = local_irq_save();

	for (size_t i = 0; i < count; i++)
		uart_tx_char(u->base, (unsigned char)buf[i]);

	local_irq_restore(flags);
	return (int)count;
}

/* ------------------------------------------------------------------
 * Console ring
 *
 * Transmitting a character means spinning on the TX-holding register, about
 * 87 us at 115200 baud.  Doing that with interrupts masked — which is what
 * "one line at a time, atomically" used to require — put a 5 ms hole in
 * interrupt latency for every 60-character log line.  That is half the audio
 * period this system is eventually meant to hold, and it would sit inside
 * every microsecond-scale measurement Phase 3 wants to make.
 *
 * So separate the two things that were tangled together.  Ordering is decided
 * when the bytes enter the ring, under a mask that lasts a few instructions.
 * Transmission happens afterwards with interrupts enabled, and a single-drainer
 * flag keeps exactly one caller pulling from the ring at a time, so the bytes
 * still leave in the order they arrived.
 *
 * Draining happens in the TX interrupt.  A writer appends and returns; the
 * transmitter asks for more when it has room, and the handler fills whatever
 * room TXFIFO_LVL reports.  Nobody waits on the wire.
 *
 * Before the console interrupt is live there is nobody to do that, and a great
 * deal of boot happens before then, so writers fall back to draining by
 * polling — the same loop, with a flag keeping it to one drainer.  panic()
 * takes the ring back the same way, since the handler will never run again and
 * the tail of the log is the part worth having.
 *
 * ------------------------------------------------------------------ */

/*
 * Append under the mask.  On overflow the *new* text is dropped rather than
 * the old: a drainer may be part-way through the buffer with interrupts on,
 * and moving the tail out from under it would corrupt the output it is in the
 * middle of producing.  Losing the newest bytes is also the lesser evil for a
 * log that fills because something is spewing.
 */
static void log_put(const char *buf, size_t count, int expand_nl)
{
	unsigned long flags = local_irq_save();
	size_t i;

	for (i = 0; i < count; i++) {
		char c = buf[i];
		unsigned int need = (expand_nl && c == '\n') ? 2u : 1u;

		if (LOG_BUF_SIZE - (log_head - log_tail) < need) {
			log_dropped += (unsigned int)(count - i);
			break;
		}

		if (need == 2u)
			log_buf[log_head++ & LOG_BUF_MASK] = '\r';
		log_buf[log_head++ & LOG_BUF_MASK] = c;
	}

	/* Same masked region as the append, so the ISR cannot finish and
	 * disarm itself between us adding bytes and noticing we must arm. */
	log_tx_arm_locked();

	local_irq_restore(flags);
}

/*
 * Push the ring out to the wire.  The mask covers only the index update; the
 * ~87 us spent waiting on the UART for each character runs with interrupts
 * enabled, which is the whole point.
 */
static void log_drain(void)
{
	unsigned long flags;

	flags = local_irq_save();
	if (log_draining) {
		/* Someone else is already emptying it, and will pick up what we
		 * just appended.  Ordering is preserved either way. */
		local_irq_restore(flags);
		return;
	}
	log_draining = 1;

	/*
	 * Take the ring away from the TX interrupt for the duration.
	 *
	 * log_draining kept two callers of this function apart but said nothing
	 * to the interrupt handler, which consumes the same ring and tests only
	 * log_tx_armed.  The loop below deliberately spins on the transmitter
	 * with interrupts enabled — that is the point of it — so the handler
	 * could fire mid-drain and start emitting from the same ring.  Neither
	 * loses a byte, since both advance log_tail under the mask, but they
	 * emit in whatever order they happen to reach the wire: one holds a
	 * character in a local while the other pushes the thirty-two after it.
	 *
	 * The window is narrow, opening only while log_irq_ready flips from 0
	 * to 1 with a polled drain already in flight, which is late in initcalls
	 * — and that is exactly where a boot line came out spliced through the
	 * middle of another one.  A race inside the console is worse than a race
	 * anywhere else: design-philosophy.md §1 calls a log the only instrument
	 * this project has, and this one is the instrument corrupting itself.
	 *
	 * Disarming rather than making the handler defer: a handler that returns
	 * without draining leaves THR_IT enabled and re-fires immediately, which
	 * would be an interrupt storm for as long as the polled drain lasts.
	 */
	log_tx_armed = 0;
	mmio_write32(uarts[0].base + UART_IER,
		     mmio_read32(uarts[0].base + UART_IER) & ~IER_THR_IT);
	local_irq_restore(flags);

	for (;;) {
		unsigned int dropped;
		char c;

		flags = local_irq_save();
		if (log_head == log_tail) {
			dropped = log_dropped;
			log_dropped = 0;

			if (!dropped) {
				log_draining = 0;
				/* Hand the ring back; re-arms only if work
				 * arrived while we were finishing. */
				log_tx_arm_locked();
				local_irq_restore(flags);
				return;
			}
			local_irq_restore(flags);

			/*
			 * Report the loss, then go round again to push the
			 * report itself out.
			 *
			 * The previous version cleared log_draining, printk'd,
			 * and returned — which only appended the line to a ring
			 * that this call had just stopped emptying.  In normal
			 * running the TX interrupt picked it up soon after and
			 * nobody noticed.  On the panic path there is no TX
			 * interrupt any more: console_flush_panic() disarms it
			 * and drains by hand, so the notice that bytes were lost
			 * was itself lost, every time.  The log went quiet about
			 * the one thing it must never go quiet about — that it
			 * is incomplete.
			 *
			 * log_draining stays 1 across the printk, so the nested
			 * log_drain() inside it returns at the guard and this
			 * loop does the emitting.  It terminates because
			 * log_dropped was taken to zero above.
			 */
			printk("[log] %u bytes dropped (ring full)\n", dropped);
			continue;
		}
		c = log_buf[log_tail++ & LOG_BUF_MASK];
		local_irq_restore(flags);

		uart_tx_char(uarts[0].base, (unsigned char)c);
	}
}

/**
 * console_write() - queue a raw byte run for the console
 * @buf: bytes to send
 * @count: how many
 */
int console_write(const char *buf, size_t count)
{
	log_put(buf, count, 0);
	if (!log_irq_ready)
		log_drain();
	return (int)count;
}

/**
 * console_puts() - queue a NUL-terminated string, expanding newlines to CR LF
 * @s: string to send
 */
void console_puts(const char *s)
{
	size_t len = 0;

	while (s[len])
		len++;

	log_put(s, len, 1);
	if (!log_irq_ready)
		log_drain();
}

/**
 * console_read() - blocking read from the console UART
 * @buf: where to put the bytes
 * @count: buffer size
 *
 * Return: number of bytes read (at least one — it waits).
 */
int console_read(char *buf, size_t count)
{
	return uart_inst_read(&uarts[0], buf, count);
}

/**
 * console_flush_panic() - empty the ring when the system is dying
 *
 * panic() runs with interrupts masked and nothing else will ever run again, so
 * a drainer flag left set by whoever was interrupted would strand the tail of
 * the log — the part nearest the failure, which is the part worth having.
 * Clear it and push everything out synchronously.
 */
void console_flush_panic(void)
{
	/* Interrupts are masked and the TX handler will never run again, so
	 * take the ring back and push it out by hand. */
	log_irq_ready = 0;
	log_tx_armed = 0;
	log_draining = 0;
	mmio_write32(uarts[0].base + UART_IER,
		     mmio_read32(uarts[0].base + UART_IER) & ~IER_THR_IT);
	log_drain();
}

/* ------------------------------------------------------------------ */
/* /dev/ttyS0 (UART0 console)                                          */
/* ------------------------------------------------------------------ */

#define TIOC_MAGIC     't'
#define TIOCSETBAUD    _IOW(TIOC_MAGIC, 1, unsigned int)
#define TIOCGETBAUD    _IOR(TIOC_MAGIC, 2, unsigned int)

static int ttyS0_read(struct file *file, char *buf, size_t count)
{
	(void)file;
	return uart_inst_read(&uarts[0], buf, count);
}

static int ttyS0_write(struct file *file, const char *buf, size_t count)
{
	(void)file;
	/* Through the ring, like every other console writer — bypassing it
	 * would put these bytes on the wire out of order with the rest. */
	return console_write(buf, count);
}

static int ttyS0_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	(void)file;

	switch (cmd) {
	case TIOCSETBAUD: {
		unsigned int baud = (unsigned int)arg;
		unsigned int div = 24000000 / (16 * baud);
		u32 lcr = mmio_read32(UART_BASE + UART_LCR);
		mmio_write32(UART_BASE + UART_LCR, lcr | LCR_DLAB);
		mmio_write32(UART_BASE + UART_DLL, div & 0xFF);
		mmio_write32(UART_BASE + UART_DLH, (div >> 8) & 0xFF);
		mmio_write32(UART_BASE + UART_LCR, lcr);
		uart_current_baud = baud;
		return 0;
	}
	case TIOCGETBAUD:
		return (int)uart_current_baud;
	default:
		return -1;
	}
}

static const struct file_operations ttyS0_fops = {
	.read    = ttyS0_read,
	.write   = ttyS0_write,
	.ioctl   = ttyS0_ioctl,
};

static struct cdev ttyS0_cdev = {
	.dev  = MKDEV(4, 64),
	.fops = &ttyS0_fops,
	.name = "ttyS0",
};

/* ------------------------------------------------------------------ */
/* /dev/uart1 (SIM7600 modem) — fixed 115200, no baud ioctl needed     */
/* ------------------------------------------------------------------ */

static int uart1_read(struct file *file, char *buf, size_t count)
{
	(void)file;
	return uart_inst_read(&uarts[1], buf, count);
}

static int uart1_write(struct file *file, const char *buf, size_t count)
{
	(void)file;
	return uart_inst_write(&uarts[1], buf, count);
}

static const struct file_operations uart1_fops = {
	.read  = uart1_read,
	.write = uart1_write,
};

static struct cdev uart1_cdev = {
	.dev  = MKDEV(4, 65),
	.fops = &uart1_fops,
	.name = "uart1",
};

/* ------------------------------------------------------------------ */
/* Probe: one instance per platform_device, matched by base PA         */
/* ------------------------------------------------------------------ */

static void uart_hw_init(struct uart_inst *u)
{
	init_waitqueue_head(&u->rx_wait);

	/* Enable the module clock if the bootloader did not (UART1). */
	if (u->clkctrl) {
		mmio_write32(u->clkctrl, 0x02);
		while ((mmio_read32(u->clkctrl) & 0x30000) != 0)
			;
	}

	/*
	 * Let whatever is already in flight finish before pulling the UART out
	 * from under it.
	 *
	 * UART0 is the console and the bootloader left it running, so by the
	 * time this probe executes there is a boot log's worth of text moving
	 * through it.  The polled drain that produced that text waits on
	 * LSR[5] TXFIFOE, which the TRM defines as "transmit hold register
	 * empty (transmission not necessarily completed)" — it means a byte can
	 * be handed over, not that any byte has left the pin.  So up to a
	 * 64-byte FIFO plus a shift register of log is still queued in hardware
	 * here, and the three lines below disable the UART and reset its FIFO.
	 *
	 * The symptom was one spliced line every couple of boots: the tail of
	 * the message before this probe replaced by a short burst of garbage.
	 * Intermittent, cosmetic-looking, and in the one instrument this project
	 * has for reading itself — which is why it is worth a spin loop.
	 *
	 * LSR[6] TXSRE is the bit that means what is needed: FIFO and shift
	 * register both empty.  Bounded, because a UART that never drains must
	 * not be able to hang the boot it is supposed to be reporting on.
	 */
	unsigned int tx_wait = 100000;
	while (!(mmio_read32(u->base + UART_LSR) & LSR_TXSRE) && tx_wait--)
		;

	/* Disable the UART (MDR1 mode 0x7) while programming the divisor — the
	 * OMAP UART requires MDR1 to be set after the config registers. UART0 was
	 * left enabled by the bootloader; UART1 powers up Disabled, so without
	 * this its TX/RX never run (THRE never sets and writes spin forever). */
	mmio_write32(u->base + UART_MDR1, 0x07);

	mmio_write32(u->base + UART_FCR, FCR_FIFO_EN | FCR_RX_TRIG_8);

	u32 lcr = LCR_8N1;
	mmio_write32(u->base + UART_LCR, lcr | LCR_DLAB);
	mmio_write32(u->base + UART_DLL, 26);	/* 48 MHz / (16*26) = 115200 */
	mmio_write32(u->base + UART_DLH, 0);
	mmio_write32(u->base + UART_LCR, lcr);

	mmio_write32(u->base + UART_IER, IER_RHR_IT);

	/* Enable UART 16x mode (must come after the config registers). */
	mmio_write32(u->base + UART_MDR1, 0x00);

	request_irq(u->irq, uart_irq_handler);
	intc_enable_irq(u->irq);

	/*
	 * From here the console can empty its own ring.  Set this only for the
	 * console port, and only after the interrupt is actually live — every
	 * printk before this point still drains by polling, and there are a lot
	 * of them.
	 */
	if (u == &uarts[0]) {
		unsigned long flags = local_irq_save();

		log_irq_ready = 1;
		log_tx_arm_locked();
		local_irq_restore(flags);
	}
}

static int uart_probe(struct platform_device *pdev)
{
	struct uart_inst *u = 0;

	for (unsigned int i = 0; i < NR_UART; i++)
		if (uarts[i].pa == pdev->base) {
			u = &uarts[i];
			break;
		}
	if (!u)
		return -1;

	if (u == &uarts[0]) {
		/* UART0 console pins are already muxed by the bootloader. */
		uart_hw_init(u);
		printk("[UART] uart0 (console) 115200 8N1\n");
		cdev_register(&ttyS0_cdev);
	} else {
		/* UART1 pins are NOT touched by the bootloader — mux them to the
		 * UART1 function (P9_24 txd, P9_26 rxd) before bring-up, else the
		 * signals never reach the header pins. */
		pinctrl_select("uart1");
		uart_hw_init(u);
		printk("[UART] uart1 (modem) 115200 8N1\n");
		cdev_register(&uart1_cdev);
	}
	return 0;
}

static struct platform_driver uart_driver = {
	.probe = uart_probe,
};

static int __init omap_uart_init(void)
{
	uart_driver.drv.name = "omap_uart";
	return platform_driver_register(&uart_driver);
}
device_initcall(omap_uart_init);

/* ------------------------------------------------------------------ */
/* Console helpers (UART0) — used by printk() and sys_read(fd 0)        */
/* ------------------------------------------------------------------ */

/*
 * Direct, unbuffered, bypasses the ring.  Only for paths that must not depend
 * on the ring being in a sane state; anything ordinary belongs in
 * console_write()/console_puts() or its bytes will appear out of order with
 * everything else on the console.
 */
void uart_putchar(int c)
{
	uart_tx_char(UART_BASE, c);
}

int uart_getchar(void)
{
	struct uart_inst *u = &uarts[0];
	unsigned int tail = u->rx_tail;

	if (tail == u->rx_head)
		return -1;

	u->rx_tail = (tail + 1) & RX_BUF_MASK;
	return u->rx_buf[tail];
}
