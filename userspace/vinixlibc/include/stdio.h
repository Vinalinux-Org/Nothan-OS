/* ============================================================
 * stdio.h — printf family + FILE buffered I/O
 * ============================================================ */

#ifndef _VINIXLIBC_STDIO_H
#define _VINIXLIBC_STDIO_H

#include "types.h"
#include <stdarg.h>

#define EOF (-1)
#define BUFSIZ 512

/* Forward declaration — FILE internals live in stdio.c. */
typedef struct _FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* --- formatted output --- */
int  printf(const char *fmt, ...);
int  fprintf(FILE *fp, const char *fmt, ...);
int  sprintf(char *buf, const char *fmt, ...);
int  snprintf(char *buf, size_t size, const char *fmt, ...);
int  vprintf(const char *fmt, va_list ap);
int  vfprintf(FILE *fp, const char *fmt, va_list ap);
int  vsprintf(char *buf, const char *fmt, va_list ap);
int  vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

/* --- FILE streams --- */
FILE *fopen(const char *path, const char *mode);
int   fclose(FILE *fp);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
char *fgets(char *buf, int size, FILE *fp);
int   fputs(const char *s, FILE *fp);
int   fgetc(FILE *fp);
int   fputc(int c, FILE *fp);
int   fflush(FILE *fp);
int   feof(FILE *fp);
int   ferror(FILE *fp);

/* --- stdout shortcuts --- */
int   puts(const char *s);
int   putchar(int c);
int   getchar(void);

#endif /* _VINIXLIBC_STDIO_H */
