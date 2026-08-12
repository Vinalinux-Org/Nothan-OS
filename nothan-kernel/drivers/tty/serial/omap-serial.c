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
};

/*
 * PROTECTION: the RX ring needs none — one producer (the IRQ handler advances
 * rx_head) and one consumer (read() advances rx_tail), each owning its own
 * index, with the volatile qualifiers making the handoff visible.  That is the
 * one shape in this kernel that is safe without masking, and it only holds
 * because there is exactly one reader per port.
 *
 * TX is the opposite: uart_inst_write() masks, because several unrelated
 * callers push bytes at the same device.
 */
static struct uart_inst uarts[] = {
	{ .base = UART_BASE,  .pa = UART0_PA, .irq = UART_IRQ,  .clkctrl = 0 },
	{ .base = UART1_VA,   .pa = UART1_PA, .irq = UART1_IRQ, .clkctrl = CM_PER_UART1_CLKCTRL },
};
#define NR_UART		(sizeof(uarts) / sizeof(uarts[0]))

static unsigned int uart_current_baud = 115200;	/* console (UART0) */

#define LOG_BUF_SIZE	4096u			/* power of two */
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

#define TX_BURST	32u		/* chars pushed per TX interrupt */

/* Arm the TX interrupt if there is work and it is not already running.
 * Caller must hold the interrupt mask. */
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

static void uart_irq_handler(unsigned int irq)
{
	struct uart_inst *u = &uarts[0];

	for (unsigned int i = 0; i < NR_UART; i++)
		if (uarts[i].irq == irq) {
			u = &uarts[i];
			break;
		}

	while (mmio_read32(u->base + UART_LSR) & LSR_DR) {
		u8 c = mmio_read32(u->base + UART_RHR);
		unsigned int next = (u->rx_head + 1) & RX_BUF_MASK;
		if (next != u->rx_tail) {
			u->rx_buf[u->rx_head] = c;
			u->rx_head = next;
		}
	}

	/*
	 * Console TX: the transmitter has room, so hand it more of the log
	 * ring.  This is the whole point of the exercise — the bytes leave
	 * while everything else runs, and no writer ever waits on the wire.
	 *
	 * Up to TX_BURST at a time because the writes themselves are cheap
	 * register stores into a 64-byte FIFO; it is the waiting that used to
	 * cost, and there is none here.  Fewer per interrupt would just mean
	 * more interrupts for the same bytes.
	 */
	if (u == &uarts[0] && log_tx_armed &&
	    (mmio_read32(u->base + UART_LSR) & LSR_THRE)) {
		unsigned int n;

		for (n = 0; n < TX_BURST && log_head != log_tail; n++)
			mmio_write32(u->base + UART_THR,
				     log_buf[log_tail++ & LOG_BUF_MASK]);

		if (log_head == log_tail) {
			/* Nothing left — stop the interrupt, or it re-fires
			 * forever on an empty transmitter. */
			log_tx_armed = 0;
			mmio_write32(u->base + UART_IER,
				     mmio_read32(u->base + UART_IER) & ~IER_THR_IT);
		}
	}
}

static void uart_tx_char(u32 base, int c)
{
	while (!(mmio_read32(base + UART_LSR) & LSR_THRE))
		;
	mmio_write32(base + UART_THR, c);
}

static int uart_inst_read(struct uart_inst *u, char *buf, size_t count)
{
	size_t i = 0;

	while (i < count) {
		unsigned int tail = u->rx_tail;
		if (tail == u->rx_head)
			break;
		buf[i++] = (char)u->rx_buf[tail];
		u->rx_tail = (tail + 1) & RX_BUF_MASK;
	}
	return i > 0 ? (int)i : -1;
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
 * transmitter asks for more when it has room, and the handler feeds it up to
 * TX_BURST characters at a time.  Nobody waits on the wire.
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
	local_irq_restore(flags);

	for (;;) {
		unsigned int dropped;
		char c;

		flags = local_irq_save();
		if (log_head == log_tail) {
			dropped = log_dropped;
			log_dropped = 0;
			log_draining = 0;
			local_irq_restore(flags);

			if (dropped)
				printk("[log] %u bytes dropped (ring full)\n",
				       dropped);
			return;
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
	/* Enable the module clock if the bootloader did not (UART1). */
	if (u->clkctrl) {
		mmio_write32(u->clkctrl, 0x02);
		while ((mmio_read32(u->clkctrl) & 0x30000) != 0)
			;
	}

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
