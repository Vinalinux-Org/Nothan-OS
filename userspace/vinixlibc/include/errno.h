/* ============================================================
 * errno.h — POSIX error codes, sign-flipped from kernel's E_*
 * ============================================================ */

#ifndef _VINIXLIBC_ERRNO_H
#define _VINIXLIBC_ERRNO_H

extern int errno;

#define EPERM     1   /* Operation not permitted */
#define ENOENT    2   /* No such file or directory */
#define EINTR     4   /* Interrupted system call */
#define EIO       5   /* I/O error */
#define EBADF     9   /* Bad file descriptor */
#define EAGAIN   11   /* Try again */
#define ENOMEM   12   /* Out of memory */
#define EACCES   13   /* Permission denied */
#define EFAULT   14   /* Bad address */
#define EEXIST   17   /* File exists */
#define ENOTDIR  20   /* Not a directory */
#define EISDIR   21   /* Is a directory */
#define EINVAL   22   /* Invalid argument */
#define EMFILE   24   /* Too many open files */
#define ENOSPC   28   /* No space left on device */
#define ERANGE   34   /* Numerical result out of range */
#define ECHILD  100   /* No child processes (kept low so <127 fits in u8) */

#endif /* _VINIXLIBC_ERRNO_H */
