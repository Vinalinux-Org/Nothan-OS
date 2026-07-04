#ifndef _NOTHAN_STDIO_H
#define _NOTHAN_STDIO_H

#include "syscall.h"
#include "string.h"

static inline void putchar(char c)
{
	if (c == '\n')
		writefile(1, "\r", 1);
	writefile(1, &c, 1);
}

static inline void puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			writefile(1, "\r", 1);
		writefile(1, s, 1);
		s++;
	}
}

static inline void putnchar(char c, int n)
{
	while (n-- > 0) putchar(c);
}

static inline void putint(int n, int width)
{
	char buf[12];
	int i = 0, j;
	if (n < 0) { putchar('-'); n = -n; }
	if (n == 0) { buf[i++] = '0'; }
	while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
	for (j = i; j < width; j++) putchar(' ');
	while (i > 0) putchar(buf[--i]);
}

static inline void putpad(const char *s, int width)
{
	int len = (int)strlen(s);
	puts(s);
	putnchar(' ', width - len);
}

#endif /* _NOTHAN_STDIO_H */
