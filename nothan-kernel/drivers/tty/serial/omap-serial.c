/*
 * drivers/tty/serial/omap-serial.c - OMAP UART driver (multi-instance)
 *
 * UART0 = debug console (/dev/ttyS0), clocked by the bootloader.
 * UART1 = SIM7600 modem (/dev/uart1), clocked here via CM_PER_UART1_CLKCTRL.
 * Both share one per-instance driver: interrupt-driven RX ring, polled TX.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

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

static struct uart_inst uarts[] = {
	{ .base = UART_BASE,  .pa = UART0_PA, .irq = UART_IRQ,  .clkctrl = 0 },
	{ .base = UART1_VA,   .pa = UART1_PA, .irq = UART1_IRQ, .clkctrl = CM_PER_UART1_CLKCTRL },
};
#define NR_UART		(sizeof(uarts) / sizeof(uarts[0]))

static unsigned int uart_current_baud = 115200;	/* console (UART0) */

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

static int uart_inst_write(struct uart_inst *u, const char *buf, size_t count)
{
	for (size_t i = 0; i < count; i++)
		uart_tx_char(u->base, (unsigned char)buf[i]);
	return (int)count;
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
	return uart_inst_write(&uarts[0], buf, count);
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
