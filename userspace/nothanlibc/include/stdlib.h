/* ============================================================
 * stdlib.h — POSIX subset, hand-written for NothanOS
 * ============================================================ */

#ifndef _NOTHANLIBC_STDLIB_H
#define _NOTHANLIBC_STDLIB_H

#include "types.h"

/* --- integer parsing / conversion --- */
int   atoi(const char *s);
long  atol(const char *s);
long  strtol(const char *s, char **endptr, int base);
char *itoa(int value, char *buf, int base);
int   abs(int v);

/* --- memory --- */
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);

/* --- process control --- */
void  exit(int status);

#endif /* _NOTHANLIBC_STDLIB_H */
