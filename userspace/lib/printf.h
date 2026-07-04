#ifndef _NOTHAN_PRINTF_H
#define _NOTHAN_PRINTF_H

/*
 * Minimal formatted output for NothanOS userspace (no newlib, no heap).
 * Supports the conversions the telephony daemon/GUI actually use:
 *   %d %u %x %X %c %s %%  with optional 0-flag, field width, and precision
 *   (incl. %.*s and zero-padded %04x). No floats.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <stdarg.h>
#include "syscall.h"
#include "string.h"

static inline int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
	size_t pos = 0;		/* chars stored in buf (excluding NUL) */
	int    total = 0;	/* chars that would be written */

#define PUTC(ch) do { \
		if (size && pos < size - 1) buf[pos++] = (char)(ch); \
		total++; \
	} while (0)

	for (; *fmt; fmt++) {
		if (*fmt != '%') { PUTC(*fmt); continue; }
		fmt++;

		int zero = 0;
		while (*fmt == '0') { zero = 1; fmt++; }

		int width = 0;
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + (*fmt++ - '0');

		int prec = -1;
		if (*fmt == '.') {
			fmt++;
			if (*fmt == '*') { prec = va_arg(ap, int); fmt++; }
			else { prec = 0; while (*fmt >= '0' && *fmt <= '9') prec = prec * 10 + (*fmt++ - '0'); }
		}

		switch (*fmt) {
		case 'd': {
			int v = va_arg(ap, int);
			unsigned int u = (v < 0) ? -(unsigned int)v : (unsigned int)v;
			char tmp[12]; int n = 0;
			if (u == 0) tmp[n++] = '0';
			while (u) { tmp[n++] = (char)('0' + u % 10); u /= 10; }
			int digits = n + (v < 0 ? 1 : 0);
			char pad = zero ? '0' : ' ';
			if (v < 0 && zero) PUTC('-');
			for (int w = digits; w < width; w++) PUTC(pad);
			if (v < 0 && !zero) PUTC('-');
			while (n) PUTC(tmp[--n]);
			break;
		}
		case 'u': {
			unsigned int u = va_arg(ap, unsigned int);
			char tmp[12]; int n = 0;
			if (u == 0) tmp[n++] = '0';
			while (u) { tmp[n++] = (char)('0' + u % 10); u /= 10; }
			char pad = zero ? '0' : ' ';
			for (int w = n; w < width; w++) PUTC(pad);
			while (n) PUTC(tmp[--n]);
			break;
		}
		case 'x':
		case 'X': {
			unsigned int u = va_arg(ap, unsigned int);
			const char *hex = (*fmt == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
			char tmp[8]; int n = 0;
			if (u == 0) tmp[n++] = '0';
			while (u) { tmp[n++] = hex[u & 0xF]; u >>= 4; }
			char pad = zero ? '0' : ' ';
			for (int w = n; w < width; w++) PUTC(pad);
			while (n) PUTC(tmp[--n]);
			break;
		}
		case 'c':
			PUTC((char)va_arg(ap, int));
			break;
		case 's': {
			const char *s = va_arg(ap, const char *);
			if (!s) s = "(null)";
			int n = 0;
			while (s[n] && (prec < 0 || n < prec)) n++;
			for (int w = n; w < width; w++) PUTC(' ');
			for (int i = 0; i < n; i++) PUTC(s[i]);
			break;
		}
		case '%':
			PUTC('%');
			break;
		case '\0':
			fmt--;
			break;
		default:
			PUTC('%');
			PUTC(*fmt);
			break;
		}
	}

	if (size)
		buf[pos < size ? pos : size - 1] = '\0';
#undef PUTC
	return total;
}

static inline int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return n;
}

/* printf() logs to the UART console (fd 1), translating '\n' -> '\r\n' to
 * match puts()/putchar(). */
static inline int printf(const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	int w = (n < (int)sizeof(buf) - 1) ? n : (int)sizeof(buf) - 1;

	/* Translate '\n' -> '\r\n' into one buffer and write it in a single
	 * syscall, so a line is far less likely to be split mid-way by another
	 * task's console output (e.g. kernel printk). */
	char out[2 * sizeof(buf)];
	int  o = 0;
	for (int i = 0; i < w && o < (int)sizeof(out) - 1; i++) {
		if (buf[i] == '\n' && o < (int)sizeof(out) - 1)
			out[o++] = '\r';
		out[o++] = buf[i];
	}
	writefile(1, out, (unsigned long)o);
	return n;
}

#endif /* _NOTHAN_PRINTF_H */
