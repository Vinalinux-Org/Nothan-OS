/*
 * drivers/tty/common_subsystem.c — common kernel output path.
 */

#include "nothan/common_subsystem.h"
#include "uart.h"

void common_subsystem_putc(char ch)
{
    if (ch == '\n')
        uart_putc('\r');
    uart_putc(ch);
}

int common_subsystem_write(const void *buf, uint32_t len)
{
    const char *s;
    uint32_t i;

    s = (const char *)buf;
    for (i = 0; i < len; i++)
        common_subsystem_putc(s[i]);

    return (int)len;
}

void common_subsystem_write_string(const char *s)
{
    if (!s)
        s = "(null)";

    while (*s)
        common_subsystem_putc(*s++);
}
