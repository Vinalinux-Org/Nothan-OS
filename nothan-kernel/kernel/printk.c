#include <nothan/types.h>
#include <nothan/uart.h>

/**
 * printk() - Print a string to the kernel console
 * @s: The null-terminated string to print
 *
 * This function handles translating newline characters to carriage
 * return + newline for standard serial terminals.
 */
void printk(const char *s)
{
	while (*s) {
		if (*s == '\n')
			uart_putchar('\r');
		uart_putchar(*s++);
	}
}
