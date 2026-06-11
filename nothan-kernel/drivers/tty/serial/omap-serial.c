/*
 * drivers/tty/serial/omap-serial.c - OMAP UART driver
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

/*
 * PRCM clock control for UART0.
 * CM_PER is at PA 0x44E00000, mapped via L4_WKUP at VA 0xF0E00000.
 */
#define CM_PER_UART0_CLKCTRL	0xF0E0006C

#define RX_BUF_SIZE		256

static u8 rx_buf[RX_BUF_SIZE];
static volatile unsigned int rx_head;
static volatile unsigned int rx_tail;

static void uart_irq_handler(unsigned int irq)
{
	(void)irq;

	while (mmio_read32(UART_BASE + UART_LSR) & LSR_DR) {
		u8 c = mmio_read32(UART_BASE + UART_RHR);
		unsigned int next = (rx_head + 1) & (RX_BUF_SIZE - 1);
		if (next != rx_tail) {
			rx_buf[rx_head] = c;
			rx_head = next;
		}
	}
}

/* ioctl commands for /dev/ttyS0 */
#define TIOC_MAGIC     't'
#define TIOCSETBAUD    _IOW(TIOC_MAGIC, 1, unsigned int)
#define TIOCGETBAUD    _IOR(TIOC_MAGIC, 2, unsigned int)

static unsigned int uart_current_baud = 115200;

static int ttyS0_read(struct file *file, char *buf, size_t count)
{
	(void)file;
	size_t i = 0;

	while (i < count) {
		int c = uart_getchar();
		if (c < 0)
			break;
		buf[i++] = (char)c;
	}
	return i > 0 ? (int)i : -1;
}

static int ttyS0_write(struct file *file, const char *buf, size_t count)
{
	(void)file;
	for (size_t i = 0; i < count; i++)
		uart_putchar((unsigned char)buf[i]);
	return (int)count;
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

/**
 * uart_probe - initialise UART0 for 115200 8N1 with interrupt-driven RX
 * @pdev: platform device (unused; base address and IRQ come from uart.h macros)
 *
 * Return: 0 always.
 */
static int uart_probe(struct platform_device *pdev)
{
	(void)pdev;

	mmio_write32(CM_PER_UART0_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_UART0_CLKCTRL) & 0x30000) != 0)
		;

	mmio_write32(UART_BASE + UART_FCR, FCR_FIFO_EN | FCR_RX_TRIG_8);

	u32 lcr = LCR_8N1;
	mmio_write32(UART_BASE + UART_LCR, lcr | LCR_DLAB);

	mmio_write32(UART_BASE + UART_DLL, 26);
	mmio_write32(UART_BASE + UART_DLH, 0);

	mmio_write32(UART_BASE + UART_LCR, lcr);

	mmio_write32(UART_BASE + UART_IER, IER_RHR_IT);

	request_irq(UART_IRQ, uart_irq_handler);
	intc_enable_irq(UART_IRQ);

	printk("[UART] 115200 8N1\n");
	cdev_register(&ttyS0_cdev);
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
/**
 * uart_putchar - transmit a single character (polling)
 * @c: character to transmit
 *
 * Waits for the TX FIFO to have space, then writes the character.
 * Used by printk() for kernel console output.
 */
void uart_putchar(int c)
{
	while (!(mmio_read32(UART_BASE + UART_LSR) & LSR_THRE))
		;
	mmio_write32(UART_BASE + UART_THR, c);
}

/**
 * uart_getchar - read a character from the RX ring buffer
 *
 * Return: the next character, or -1 if the buffer is empty.
 */
int uart_getchar(void)
{
	unsigned int tail = rx_tail;
	unsigned int head = rx_head;

	if (tail == head)
		return -1;

	rx_tail = (tail + 1) & (RX_BUF_SIZE - 1);
	return rx_buf[tail];
}
