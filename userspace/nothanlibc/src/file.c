/* ============================================================
 * file.c — FILE * layer, unbuffered writes / lightly buffered
 * reads. The goal is source-level POSIX compatibility, not peak
 * throughput; fwrite calls directly into write(2) and fread into
 * read(2) one chunk at a time.
 * ============================================================ */

#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"

struct _FILE {
    int   fd;
    int   flags;   /* FILE_IN_USE, FILE_EOF, FILE_ERR */
    int   pushback;/* ungetc slot (EOF = empty) */
    int   owns_fd; /* close() fd at fclose() */
};

#define FILE_IN_USE 0x01
#define FILE_EOF    0x02
#define FILE_ERR    0x04

#define MAX_FILES 8

static FILE files[MAX_FILES];

static FILE std_streams[3] = {
    [0] = { .fd = 0, .flags = FILE_IN_USE, .pushback = EOF, .owns_fd = 0 },
    [1] = { .fd = 1, .flags = FILE_IN_USE, .pushback = EOF, .owns_fd = 0 },
    [2] = { .fd = 2, .flags = FILE_IN_USE, .pushback = EOF, .owns_fd = 0 },
};

FILE *stdin  = &std_streams[0];
FILE *stdout = &std_streams[1];
FILE *stderr = &std_streams[2];

static FILE *alloc_file(void)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (!(files[i].flags & FILE_IN_USE)) {
            files[i].flags    = FILE_IN_USE;
            files[i].pushback = EOF;
            return &files[i];
        }
    }
    return 0;
}

static int parse_mode(const char *mode)
{
    if (!mode || !*mode) return -1;

    int flags = 0;
    int plus  = (mode[1] == '+') || (mode[1] && mode[2] == '+');

    switch (mode[0]) {
    case 'r': flags = plus ? O_RDWR : O_RDONLY; break;
    case 'w': flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC; break;
    case 'a': flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND; break;
    default:  return -1;
    }
    return flags;
}

/* ============================================================
 * Open / close
 * ============================================================ */

FILE *fopen(const char *path, const char *mode)
{
    int flags = parse_mode(mode);
    if (flags < 0) { errno = EINVAL; return 0; }

    int fd = open(path, flags);
    if (fd < 0) return 0;

    FILE *fp = alloc_file();
    if (!fp) { close(fd); errno = EMFILE; return 0; }

    fp->fd      = fd;
    fp->owns_fd = 1;
    return fp;
}

int fclose(FILE *fp)
{
    if (!fp) return EOF;
    int rc = 0;
    if (fp->owns_fd && fp->fd >= 0) {
        rc = close(fp->fd);
    }
    fp->flags = 0;
    fp->fd    = -1;
    return rc;
}

/* ============================================================
 * Reads
 * ============================================================ */

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    if (!fp || size == 0 || nmemb == 0) return 0;
    uint8_t *out = (uint8_t *)ptr;
    size_t total = size * nmemb;
    size_t done = 0;

    if (fp->pushback != EOF && total > 0) {
        *out++ = (uint8_t)fp->pushback;
        fp->pushback = EOF;
        done++;
    }

    while (done < total) {
        ssize_t n = read(fp->fd, out + done, total - done);
        if (n < 0) { fp->flags |= FILE_ERR; break; }
        if (n == 0) { fp->flags |= FILE_EOF; break; }
        done += (size_t)n;
    }
    return done / size;
}

int fgetc(FILE *fp)
{
    if (!fp) return EOF;
    if (fp->pushback != EOF) {
        int c = fp->pushback;
        fp->pushback = EOF;
        return c;
    }
    uint8_t ch;
    ssize_t n = read(fp->fd, &ch, 1);
    if (n < 0) { fp->flags |= FILE_ERR; return EOF; }
    if (n == 0) { fp->flags |= FILE_EOF; return EOF; }
    return (int)ch;
}

int getchar(void) { return fgetc(stdin); }

char *fgets(char *buf, int size, FILE *fp)
{
    if (!buf || size <= 0 || !fp) return 0;
    int i = 0;
    while (i < size - 1) {
        int c = fgetc(fp);
        if (c == EOF) break;
        buf[i++] = (char)c;
        if (c == '\n') break;
    }
    if (i == 0) return 0;
    buf[i] = '\0';
    return buf;
}

/* ============================================================
 * Writes
 * ============================================================ */

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    if (!fp || size == 0 || nmemb == 0) return 0;
    size_t total = size * nmemb;
    ssize_t n = write(fp->fd, ptr, total);
    if (n < 0) { fp->flags |= FILE_ERR; return 0; }
    return (size_t)n / size;
}

int fputc(int c, FILE *fp)
{
    unsigned char ch = (unsigned char)c;
    if (fwrite(&ch, 1, 1, fp) != 1) return EOF;
    return (int)ch;
}

int putchar(int c) { return fputc(c, stdout); }

int fputs(const char *s, FILE *fp)
{
    if (!s || !fp) return EOF;
    size_t len = strlen(s);
    if (fwrite(s, 1, len, fp) != len) return EOF;
    return 0;
}

int puts(const char *s)
{
    if (fputs(s, stdout) < 0) return EOF;
    fputc('\n', stdout);
    return 0;
}

int fflush(FILE *fp) { (void)fp; return 0; }  /* unbuffered writes already */

int feof(FILE *fp)   { return fp ? (fp->flags & FILE_EOF) != 0 : 0; }
int ferror(FILE *fp) { return fp ? (fp->flags & FILE_ERR) != 0 : 0; }

/* ============================================================
 * fprintf / vfprintf via printf's shared engine
 * ============================================================ */

extern int vdprintf_fd(int fd, const char *fmt, va_list ap);

int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
    if (!fp) return -1;
    return vdprintf_fd(fp->fd, fmt, ap);
}

int fprintf(FILE *fp, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int n = vfprintf(fp, fmt, ap);
    va_end(ap);
    return n;
}
