/*
 * bootloader/src/utils.c - Memory and delay utilities
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "boot.h"

/* Fix buffer overflow */
void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (n == 0)
        return dest;

    if (d == NULL || s == NULL)
        return NULL;

    if (d == s)
        return dest;

    uintptr_t dp = (uintptr_t)d;
    uintptr_t sp = (uintptr_t)s;

    /*
     * Copy forward when dest is before src, or when the regions do not overlap.
     * Comparing uintptr_t values avoids undefined behavior when the two pointers
     * do not belong to the same object.
     */
    if (dp < sp || dp - sp >= n) {
        while (n--)
            *d++ = *s++;
    } else {
        /* dest lies inside the src region: copy backward so unread data is not overwritten */
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }

    return dest;
}

void *memset(void *s, int c, size_t n)
{
	unsigned char *p = (unsigned char *)s;

	while (n--)
		*p++ = (unsigned char)c;
	return s;
}

void delay(uint32_t count)
{
	volatile uint32_t i;

	for (i = 0; i < count; i++)
		;
}
