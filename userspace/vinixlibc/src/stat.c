/* ============================================================
 * stat.c — stub until kernel SYS_STAT exists. We still keep the
 * struct so includes type-check; callers opting into stat() see
 * -1 + EINVAL and can fall back.
 * ============================================================ */

#include "sys/stat.h"
#include "errno.h"

int stat(const char *path, struct stat *buf)
{
    (void)path; (void)buf;
    errno = EINVAL;
    return -1;
}

int fstat(int fd, struct stat *buf)
{
    (void)fd; (void)buf;
    errno = EINVAL;
    return -1;
}
