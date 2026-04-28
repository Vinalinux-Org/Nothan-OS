/* ============================================================
 * string.h
 * ------------------------------------------------------------
 * Kernel libc string + memory subset.
 * ============================================================ */

#ifndef STRING_H
#define STRING_H

#include "types.h"

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
void strcpy(char *dst, const char *src);
int strlen(const char *s);

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#endif /* STRING_H */
