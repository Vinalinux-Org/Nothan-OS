/* ============================================================
 * string.h — POSIX subset, hand-written for VinixOS
 * ============================================================ */

#ifndef _VINIXLIBC_STRING_H
#define _VINIXLIBC_STRING_H

#include "types.h"

/* Length / compare */
size_t strlen(const char *s);
int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);

/* Copy / concat */
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);

/* Search */
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strstr(const char *haystack, const char *needle);

/* Memory */
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
void  *memset(void *s, int c, size_t n);
int    memcmp(const void *s1, const void *s2, size_t n);

#endif /* _VINIXLIBC_STRING_H */
