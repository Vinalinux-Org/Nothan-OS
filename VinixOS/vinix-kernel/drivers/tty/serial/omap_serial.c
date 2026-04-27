/* ============================================================
 * uart.c
 * ------------------------------------------------------------
 * UART Driver
 * ============================================================ */

#include "types.h"
#include "uart.h"
#include "mmio.h"
#include "cpu.h"
#include "irq.h"
#include "intc.h"

/* Hardware Definitions */
#define UART0_BASE      0x44E09000
#define UART0_IRQ       72

/* Clock Module */
#define CM_PER_BASE             0x44E00000
#define CM_PER_UART0_CLKCTRL    (CM_PER_BASE + 0x6C)

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
#define LSR_ERROR_MASK  (LSR_OE | LSR_PE | LSR_FE | LSR_BI)

/* FCR bits */
#define FCR_FIFO_EN     (1 << 0)
#define FCR_RX_CLR      (1 << 1)
#define FCR_TX_CLR      (1 << 2)

/* ============================================================
 * RX Interrupt Handler — drains FIFO, hands bytes to serial_core
 * ============================================================ */

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

/* ============================================================
 * UART Initialization — ring buffer state owned by serial_core.
 * ============================================================ */

void uart_init(void)
{
    /* Hardware-side init runs in uart_enable_rx_interrupt(). */
}

void uart_enable_rx_interrupt(void)
{
    /* Enable UART0 module clock (CRITICAL!) */
    mmio_write32(CM_PER_UART0_CLKCTRL, 0x2);
    
    /* Wait for clock to be functional */
    while ((mmio_read32(CM_PER_UART0_CLKCTRL) & 0x30000) != 0);
    
    /* Save current line control register */
    uint32_t lcr_save = mmio_read32(UART0_BASE + UART_LCR);
    
    /* Ensure operational mode */
    mmio_write32(UART0_BASE + UART_LCR, lcr_save & 0x7F);
    
    /* Clear interrupt enable register */
    mmio_write32(UART0_BASE + UART_IER, 0x00);
    
    /* Configure FIFO - simple legacy mode
     * Enable FIFO + clear RX/TX + 8-char trigger */
    mmio_write32(UART0_BASE + UART_FCR, 0x07);
    
    /* Small delay for FIFO configuration to take effect */
    for (volatile int i = 0; i < 1000; i++);
    
    /* Configure supplementary control register for 1-char granularity */
    mmio_write32(UART0_BASE + UART_SCR, 0xC0);
    
    /* Restore line control register */
    mmio_write32(UART0_BASE + UART_LCR, lcr_save);
    
    /* Enable RX interrupt */
    mmio_write32(UART0_BASE + UART_IER, IER_RHR_IT);
    
    /* Register IRQ handler */
    int ret = request_irq(UART0_IRQ, uart_rx_irq_handler, 0, "omap-uart", NULL);
    if (ret != 0) {
        return;  /* Registration failed */
    }
    
    /* Enable IRQ in interrupt controller */
    enable_irq(UART0_IRQ);
}

/* ============================================================
 * TX Functions
 * ============================================================
 */

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

/* ============================================================
 * Platform driver wiring
 * ============================================================ */

#include "platform_device.h"

static int omap_uart_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    pr_info("[UART] probing %s @ 0x%08x irq %d\n",
                pdev->name, mem ? mem->start : 0, irq);
    uart_init();
    return 0;
}

static struct platform_driver omap_uart_driver = {
    .drv   = { .name = "omap-uart" },
    .probe = omap_uart_probe,
};

#include "vinix/init.h"
static int __init omap_uart_driver_init(void)
{
    return platform_driver_register(&omap_uart_driver);
}
arch_initcall(omap_uart_driver_init);