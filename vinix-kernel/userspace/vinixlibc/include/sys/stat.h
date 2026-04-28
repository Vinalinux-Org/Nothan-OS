/* ============================================================
 * sys/stat.h — struct stat carrier; stat() itself is a stub
 * returning -1 + EINVAL until the kernel ships SYS_STAT.
 * ============================================================ */

#ifndef _VINIXLIBC_SYS_STAT_H
#define _VINIXLIBC_SYS_STAT_H

#include "types.h"

/* POSIX mode bits — a minimal subset; enough to type-check
 * callers that use S_ISREG / S_ISDIR today. */
#define S_IFMT   0170000
#define S_IFREG  0100000
#define S_IFDIR  0040000

#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)

struct stat {
    uint32_t st_mode;
    uint32_t st_size;
    uint32_t st_ino;
};

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);

#endif /* _VINIXLIBC_SYS_STAT_H */
