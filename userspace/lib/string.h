#ifndef _NOTHAN_STRING_H
#define _NOTHAN_STRING_H

typedef unsigned long size_t;

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

#endif /* _NOTHAN_STRING_H */
