/*
 * pd_libc.c - external definitions of the freestanding mem* functions GCC may
 * emit calls to (struct/array initialisation, struct copies). lib/string.h
 * provides static-inline versions for explicit calls; these external symbols
 * satisfy the compiler's generated libcalls, since the daemon links without
 * newlib (-lc). Do NOT include string.h here (would clash with the inlines).
 */

typedef unsigned long pd_size_t;

/**
 * memset() - Fill a block of memory with a specific value
 * @dst: Pointer to the block of memory to fill
 * @c: Value to be set
 * @n: Number of bytes to be set to the value
 *
 * Return: Pointer to the memory area dst.
 */
void *memset(void *dst, int c, pd_size_t n)
{
	unsigned char *d = dst;
	while (n--)
		*d++ = (unsigned char)c;
	return dst;
}

/**
 * memcpy() - Copy a block of memory from source to destination
 * @dst: Pointer to the destination array where the content is to be copied
 * @src: Pointer to the source of data to be copied
 * @n: Number of bytes to copy
 *
 * Return: Pointer to destination.
 */
void *memcpy(void *dst, const void *src, pd_size_t n)
{
	unsigned char       *d = dst;
	const unsigned char *s = src;
	while (n--)
		*d++ = *s++;
	return dst;
}

/**
 * memmove() - Move a block of memory safely (handling overlapping regions)
 * @dst: Pointer to the destination array
 * @src: Pointer to the source array
 * @n: Number of bytes to move
 *
 * Return: Pointer to destination.
 */
void *memmove(void *dst, const void *src, pd_size_t n)
{
	unsigned char       *d = dst;
	const unsigned char *s = src;
	if (d < s) {
		while (n--)
			*d++ = *s++;
	} else {
		d += n; s += n;
		while (n--)
			*--d = *--s;
	}
	return dst;
}
