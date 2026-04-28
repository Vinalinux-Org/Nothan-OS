/* ============================================================
 * uart.c
 * ------------------------------------------------------------
 * AM335x UART0 driver (bootloader).
 * ============================================================ */

#include "am335x.h"
#include "boot.h"

/* UART configuration constants */
#define UART_FCLK_HZ    48000000    /* 48MHz (from PER PLL) */
#define UART_BAUDRATE   115200

#define UART_LCR_8N1    0x03        /* 8 data bits, no parity, 1 stop */
#define UART_LCR_BAUD_SETUP 0x83    /* Divisor Latch Enable + 8N1 */
#define UART_MDR1_16X   0x00        /* 16x oversampling mode */
#define UART_DISABLE    0x07        /* Disable UART mode */

/* Baud rate divisor calculation:
 * Divisor = FCLK / (16 * Baud) = 48,000,000 / (16 * 115200) ≈ 26 */

void uart_flush(void)
{
    while ((readl(UART0_LSR) & UART_LSR_TEMT) == 0);
}

void uart_init(void)
{
    uint32_t divisor;
    uint32_t i;

    /* CRITICAL: ROM only enables UART0 clock when UART is in the boot
     * sequence. On cold boot with SD-first, the clock stays gated and
     * UART registers are inaccessible — must enable here. */
    writel(0x2, CM_WKUP_UART0_CLKCTRL);
    while ((readl(CM_WKUP_UART0_CLKCTRL) & 0x3) != 0x2);

    /* Pin mux mode 0 + pull-up; RXD adds RXACTIVE. */
    writel(0x30, CONF_UART0_RXD);
    writel(0x10, CONF_UART0_TXD);

    writel(UART_DISABLE, UART0_MDR1);
    delay(10000);

    writel(0x07, UART0_FCR);
    delay(1000);

    for (i = 0; i < 16; i++) {
        if (readl(UART0_LSR) & UART_LSR_RXFIFOE) {
            (void)readl(UART0_RHR);
        } else {
            break;
        }
    }

    /* 115200 @ 48 MHz → divisor 26 (actual 115384.6, 0.16% error). */
    divisor = 26;

    writel(UART_LCR_BAUD_SETUP, UART0_LCR);

    writel(divisor & 0xFF, UART0_DLL);
    writel((divisor >> 8) & 0xFF, UART0_DLH);

    writel(UART_LCR_8N1, UART0_LCR);

    writel(0x07, UART0_FCR);
    delay(1000);

    writel(UART_16X_MODE, UART0_MDR1);

    delay(10000);

    while ((readl(UART0_LSR) & UART_LSR_TEMT) == 0);
}

void uart_putc(char c)
{
    if (c == '\n') {
        uart_putc('\r');
    }

    while ((readl(UART0_LSR) & UART_LSR_TXFIFOE) == 0);

    writeb(c, UART0_THR);

    { volatile int d; for(d=0; d<100; d++); }

    while ((readl(UART0_LSR) & UART_LSR_TXFIFOE) == 0);
}

void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_print_hex(uint32_t val)
{
    int i;
    static const char hex_digits[] = "0123456789ABCDEF";

    uart_puts("0x");

    for (i = 28; i >= 0; i -= 4) {
        int nibble = (val >> i) & 0xF;
        uart_putc(hex_digits[nibble]);
    }
}