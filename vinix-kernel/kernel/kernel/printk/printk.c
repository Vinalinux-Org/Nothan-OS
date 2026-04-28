/* ============================================================
 * printk.c
 * ------------------------------------------------------------
 * Linux-style kernel log entry point. Formats arguments through
 * a tiny vsprintf-like parser and emits each byte via the UART
 * console. Supports %d %u %x %X %s %c %% with optional %0NN
 * width.
 *
 * Today the only console backend is uart_putc; future
 * register_console() can route to multiple sinks.
 * ============================================================ */

#include "uart.h"
#include "vinix/printk.h"
#include <stdarg.h>

/* libgcc-free 32-bit divmod — kept inline so printk has zero
 * external code dependencies (matters in early boot / panic). */
static void udivmod(uint32_t n, uint32_t d, uint32_t *q, uint32_t *r)
{
    uint32_t quotient = 0, remainder = 0;
    if (d == 0) { *q = 0; *r = 0; return; }
    for (int i = 31; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (n >> i) & 1;
        if (remainder >= d) {
            remainder -= d;
            quotient |= (1u << i);
        }
    }
    *q = quotient;
    *r = remainder;
}

static void emit_uint(uint32_t num, int base)
{
    char buf[32];
    int  i = 0;
    static const char digits[] = "0123456789abcdef";

    if (num == 0) { uart_putc('0'); return; }

    while (num > 0) {
        uint32_t q, r;
        udivmod(num, (uint32_t)base, &q, &r);
        buf[i++] = digits[r];
        num = q;
    }
    while (i > 0) uart_putc(buf[--i]);
}

void printk(const char *fmt, ...)
{
    va_list args;
    const char *p;

    va_start(args, fmt);

    for (p = fmt; *p; p++) {
        if (*p != '%') {
            if (*p == '\n') uart_putc('\r');
            uart_putc(*p);
            continue;
        }

        p++;

        /* width — only accepts %0NN form, value ignored (parsed
         * for compat with legacy "%08x" callers). */
        if (*p == '0') {
            p++;
            while (*p >= '0' && *p <= '9') p++;
        }

        switch (*p) {
        case 'd':
        case 'u':
            emit_uint(va_arg(args, uint32_t), 10);
            break;
        case 'x':
        case 'X':
            emit_uint(va_arg(args, uint32_t), 16);
            break;
        case 's': {
            const char *s = va_arg(args, const char *);
            uart_puts(s ? s : "(null)");
            break;
        }
        case 'c':
            uart_putc((char)va_arg(args, int));
            break;
        case '%':
            uart_putc('%');
            break;
        default:
            uart_putc('%');
            uart_putc(*p);
            break;
        }
    }

    va_end(args);
}
