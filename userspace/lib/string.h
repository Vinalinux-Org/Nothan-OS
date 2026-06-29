#ifndef _NOTHAN_STRING_H
#define _NOTHAN_STRING_H

#ifndef size_t
#define size_t unsigned long
#endif

static inline size_t strlen(const char *s)
{
	size_t n = 0;
	while (*s++) n++;
	return n;
}

static inline int strcmp(const char *a, const char *b)
{
	while (*a && *a == *b) { a++; b++; }
	return (unsigned char)*a - (unsigned char)*b;
}

static inline int strncmp(const char *a, const char *b, size_t n)
{
	while (n && *a && *a == *b) { a++; b++; n--; }
	if (!n) return 0;
	return (unsigned char)*a - (unsigned char)*b;
}

static inline void *memcpy(void *dst, const void *src, size_t n)
{
	char *d = dst;
	const char *s = src;
	while (n--) *d++ = *s++;
	return dst;
}

static inline void *memset(void *dst, int c, size_t n)
{
	char *d = dst;
	while (n--) *d++ = (char)c;
	return dst;
}

static inline void *memmove(void *dst, const void *src, size_t n)
{
	char *d = dst;
	const char *s = src;
	if (d < s) {
		while (n--) *d++ = *s++;
	} else {
		d += n; s += n;
		while (n--) *--d = *--s;
	}
	return dst;
}

static inline char *strchr(const char *s, int c)
{
	for (; *s; s++)
		if (*s == (char)c)
			return (char *)s;
	return (c == 0) ? (char *)s : (char *)0;
}

static inline char *strstr(const char *h, const char *n)
{
	if (!*n)
		return (char *)h;
	for (; *h; h++) {
		const char *a = h, *b = n;
		while (*a && *b && *a == *b) { a++; b++; }
		if (!*b)
			return (char *)h;
	}
	return (char *)0;
}

static inline char *strncpy(char *dst, const char *src, size_t n)
{
	size_t i = 0;
	for (; i < n && src[i]; i++) dst[i] = src[i];
	for (; i < n; i++) dst[i] = '\0';
	return dst;
}

static inline int atoi(const char *s)
{
	int sign = 1, v = 0;
	while (*s == ' ' || *s == '\t') s++;
	if (*s == '-') { sign = -1; s++; }
	else if (*s == '+') s++;
	while (*s >= '0' && *s <= '9')
		v = v * 10 + (*s++ - '0');
	return sign * v;
}

#endif /* _NOTHAN_STRING_H */
