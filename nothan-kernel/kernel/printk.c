/*
 * kernel/printk.c - Formatted kernel console output (vsnprintf + printk)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <nothan/printk.h>
#include <nothan/uart.h>

/*
 * Minimal vsnprintf — %s %d %i %u %x %X %p %% + width + zero-pad
 */

static const char hex_lower[] = "0123456789abcdef";

static int number(char *buf, unsigned long size, unsigned long val,
		  int base, int width, int sign, int upper, int zp)
{
	char tmp[40];
	const char *digits = upper ? "0123456789ABCDEF" : hex_lower;
	int neg = 0, wr = 0, len = 0;
	unsigned long v;

	if (sign && (long)val < 0) {
		neg = 1;
		val = -(long)val;
	}

	v = val;
	do {
		tmp[len++] = digits[v % base];
		v /= base;
	} while (v);

	int nd = len + neg;
	int pad = (width > nd) ? width - nd : 0;

	if (!zp)
		while (pad-- > 0 && wr + 1 < (int)size)
			buf[wr++] = ' ';

	if (neg && wr + 1 < (int)size)
		buf[wr++] = '-';

	if (zp)
		while (pad-- > 0 && wr + 1 < (int)size)
			buf[wr++] = '0';

	while (len > 0 && wr + 1 < (int)size)
		buf[wr++] = tmp[--len];

	return wr;
}

/**
 * vsnprintf() - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer
 * @fmt: The format string
 * @args: Arguments for the format string
 *
 * Return: The number of characters that would have been written
 */
int vsnprintf(char *buf, unsigned long size, const char *fmt, va_list args)
{
	int pos = 0;

	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			if (pos + 1 < (int)size)
				buf[pos] = *fmt;
			pos++;
			continue;
		}

		if (*++fmt == '%') {
			if (pos + 1 < (int)size)
				buf[pos] = '%';
			pos++;
			continue;
		}

		int width = 0, zero_pad = 0;

		if (*fmt == '0') { zero_pad = 1; fmt++; }
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + (*fmt++ - '0');

		int is_long = (*fmt == 'l');
		if (is_long) fmt++;
		if (*fmt == 'l') fmt++;

		switch (*fmt) {
		case 's': {
			const char *s = va_arg(args, const char *);
			if (!s) s = "(null)";
			int slen = 0;
			while (s[slen]) slen++;
			int pad = (width > slen) ? width - slen : 0;
			while (pad-- > 0 && pos + 1 < (int)size)
				buf[pos++] = ' ';
			for (int i = 0; s[i]; i++) {
				if (pos + 1 < (int)size)
					buf[pos] = s[i];
				pos++;
			}
			break;
		}

		case 'd':
		case 'i':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, long)
					      : va_arg(args, int),
				      10, width, 1, 0, zero_pad);
			break;

		case 'u':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, unsigned long)
					      : va_arg(args, unsigned int),
				      10, width, 0, 0, zero_pad);
			break;

		case 'x':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, unsigned long)
					      : va_arg(args, unsigned int),
				      16, width, 0, 0, zero_pad);
			break;

		case 'X':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      is_long ? va_arg(args, unsigned long)
					      : va_arg(args, unsigned int),
				      16, width, 0, 1, zero_pad);
			break;

		case 'p':
			pos += number(buf + pos, (pos < (int)size) ? size - pos : 0,
				      (unsigned long)va_arg(args, void *),
				      16, 8, 0, 0, 1);
			break;

		default:
			if (pos + 1 < (int)size)
				buf[pos] = *fmt;
			pos++;
			break;
		}
	}

	if ((int)size > 0)
		buf[pos < (int)size ? pos : (int)size - 1] = '\0';

	return pos;
}

/**
 * printk() - Print a formatted string to the kernel console
 * @fmt: The format string
 * @...: Arguments for the format string
 *
 * This function formats a string and outputs it to the UART console.
 * It translates newline characters to carriage return + newline.
 *
 * Return: The number of characters written
 */
int printk(const char *fmt, ...)
{
	char buf[256];
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	/* IRQ-off ensures the UART output is not interleaved by preemption. */
	__asm__ __volatile__ ("cpsid i" : : : "memory");

	for (char *p = buf; *p; p++) {
		if (*p == '\n')
			uart_putchar('\r');
		uart_putchar(*p);
	}

	__asm__ __volatile__ ("cpsie i" : : : "memory");

	return ret;
}

/*
 * Division helpers — Cortex-A8 ARM mode has no HW divide
 */

unsigned int __aeabi_uidiv(unsigned int n, unsigned int d)
{
	unsigned int q = 0, r = 0;

	for (int i = 31; i >= 0; i--) {
		r = (r << 1) | ((n >> i) & 1);
		if (r >= d) { r -= d; q |= (1U << i); }
	}
	return q;
}

unsigned long long __aeabi_uidivmod(unsigned int n, unsigned int d)
{
	unsigned int q = 0, r = 0;

	for (int i = 31; i >= 0; i--) {
		r = (r << 1) | ((n >> i) & 1);
		if (r >= d) { r -= d; q |= (1U << i); }
	}
	return ((unsigned long long)r << 32) | q;
}
