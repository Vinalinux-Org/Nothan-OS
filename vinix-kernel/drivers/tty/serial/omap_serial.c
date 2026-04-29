/*
 * drivers/tty/serial/omap_serial.c — AM335x UART0 driver
 *
 * Implements TX (polling) and RX (interrupt-driven).  Received bytes
 * are pushed into the serial_core ring buffer via uart_serial_rx_push().
 */

#include "types.h"
#include "uart.h"
#include "mmio.h"
#include "cpu.h"
#include "irq.h"
#include "intc.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/errno.h"
#include "mach/prcm.h"
#include "mach/memmap.h"
#include "mach/irqs.h"

/* UART0_BASE from mach/memmap.h — used pre-probe for earlycon TX path.
 * RX interrupt setup uses PLATFORM_IRQ_UART0 from mach/irqs.h. */
#define UART0_BASE      OMAP_UART0_BASE

/* UART Register Offsets */
#define UART_RHR        0x00
#define UART_THR        0x00
#define UART_IER        0x04
#define UART_IIR        0x08
#define UART_FCR        0x08
#define UART_LCR        0x0C
#define UART_LSR        0x14
#define UART_SCR        0x40

/* IER bits */
#define IER_RHR_IT      (1 << 0)

/* IIR bits */
#define IIR_IT_PENDING  (1 << 0)
#define IIR_IT_TYPE_MASK 0x3E

/* LSR bits */
#define LSR_DR          (1 << 0)
#define LSR_OE          (1 << 1)
#define LSR_PE          (1 << 2)
#define LSR_FE          (1 << 3)
#define LSR_BI          (1 << 4)
#define LSR_THRE        (1 << 5)
#define LSR_TEMT        (1 << 6)    /* TX shift register + FIFO empty */
#define LSR_ERROR_MASK  (LSR_OE | LSR_PE | LSR_FE | LSR_BI)

/* FCR bits */
#define FCR_FIFO_EN     (1 << 0)
#define FCR_RX_CLR      (1 << 1)
#define FCR_TX_CLR      (1 << 2)

/* RX interrupt handler — drains FIFO, hands bytes to serial_core. */

static void uart_rx_irq_handler(void *data)
{
    uart_serial_rx_irq_count();

    /* Read IIR to identify interrupt type */
    uint32_t iir = mmio_read32(UART0_BASE + UART_IIR);
    if (iir & IIR_IT_PENDING) return;

    uint32_t it_type = (iir & IIR_IT_TYPE_MASK) >> 1;
    uint32_t lsr;

    /* LINE_STS (IT_TYPE = 0x06) — read LSR clears error flags. */
    if (it_type == 0x06) {
        lsr = mmio_read32(UART0_BASE + UART_LSR);
        if (!(lsr & LSR_DR)) return;
    }

    /* Drain RX FIFO, push every byte into the serial_core ring. */
    while (1) {
        lsr = mmio_read32(UART0_BASE + UART_LSR);

        if (lsr & LSR_ERROR_MASK) {
            (void)mmio_read32(UART0_BASE + UART_RHR);
            continue;
        }
        if (!(lsr & LSR_DR)) break;

        uint8_t ch = mmio_read32(UART0_BASE + UART_RHR) & 0xFF;
        (void)uart_serial_rx_push(ch);
    }
}

void uart_putc(char c)
{
    while (!(mmio_read32(UART0_BASE + UART_LSR) & LSR_THRE));
    mmio_write32(UART0_BASE + UART_THR, c);
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

/* uart_getc / uart_rx_available / uart_rx_clear / uart_get_irq_fire_count
 * live in kernel/tty/serial/serial_core.c — they consume the ring
 * buffer that uart_serial_rx_push fills from this driver's IRQ. */

static int omap_uart_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    uint32_t base = mem ? mem->start : UART0_BASE;

    pr_info("[UART] probing %s @ 0x%08x irq %d\n", pdev->name, base, irq);

    /* Wait for TX shift register idle before clock reconfiguration;
     * touching PRCM while the shift register is transmitting drops bytes. */
    while (!(mmio_read32(base + UART_LSR) & LSR_TEMT));

    /* UART0 module clock must be enabled before any register access. */
    mmio_write32(CM_PER_UART0_CLKCTRL, 0x2);
    while ((mmio_read32(CM_PER_UART0_CLKCTRL) & 0x30000) != 0);

    uint32_t lcr_save = mmio_read32(base + UART_LCR);
    mmio_write32(base + UART_LCR, lcr_save & 0x7F);
    mmio_write32(base + UART_IER, 0x00);

    /* Enable FIFO, clear RX/TX FIFOs, set 8-char trigger level. */
    mmio_write32(base + UART_FCR, 0x07);
    for (volatile int i = 0; i < 1000; i++);

    /* SCR 0xC0: 1-char granularity for RX DMA trigger. */
    mmio_write32(base + UART_SCR, 0xC0);
    mmio_write32(base + UART_LCR, lcr_save);
    mmio_write32(base + UART_IER, IER_RHR_IT);

    if (request_irq(irq, uart_rx_irq_handler, 0, "omap-uart", NULL) != 0)
        return -EIO;

    enable_irq(irq);
    return 0;
}

static struct platform_driver omap_uart_driver = {
    .drv   = { .name = "omap-uart" },
    .probe = omap_uart_probe,
};

static int __init omap_uart_driver_init(void)
{
    return platform_driver_register(&omap_uart_driver);
}
subsys_initcall(omap_uart_driver_init);