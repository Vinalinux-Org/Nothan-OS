/* ============================================================
 * vinix/errno.h
 * ------------------------------------------------------------
 * Linux-aligned errno values. Use as `return -EINVAL;` etc.
 *
 * Old E_* names (E_OK, E_FAIL, E_INVAL, ...) in syscalls.h
 * still resolve for now. New code should prefer the -E* form
 * so signatures match Linux conventions.
 * ============================================================ */

#ifndef VINIX_ERRNO_H
#define VINIX_ERRNO_H

#define EPERM            1   /* Operation not permitted */
#define ENOENT           2   /* No such file or directory */
#define ESRCH            3   /* No such process */
#define EINTR            4   /* Interrupted system call */
#define EIO              5   /* I/O error */
#define ENXIO            6   /* No such device or address */
#define E2BIG            7   /* Argument list too long */
#define ENOEXEC          8   /* Exec format error */
#define EBADF            9   /* Bad file number */
#define ECHILD          10   /* No child processes */
#define EAGAIN          11   /* Try again */
#define ENOMEM          12   /* Out of memory */
#define EACCES          13   /* Permission denied */
#define EFAULT          14   /* Bad address */
#define EBUSY           16   /* Device or resource busy */
#define EEXIST          17   /* File exists */
#define EXDEV           18   /* Cross-device link */
#define ENODEV          19   /* No such device */
#define ENOTDIR         20   /* Not a directory */
#define EISDIR          21   /* Is a directory */
#define EINVAL          22   /* Invalid argument */
#define ENFILE          23   /* File table overflow */
#define EMFILE          24   /* Too many open files */
#define ENOTTY          25   /* Not a typewriter */
#define EFBIG           27   /* File too large */
#define ENOSPC          28   /* No space left on device */
#define ESPIPE          29   /* Illegal seek */
#define EROFS           30   /* Read-only file system */
#define EPIPE           32   /* Broken pipe */
#define ERANGE          34   /* Math result not representable */
#define ENAMETOOLONG    36   /* File name too long */
#define ENOSYS          38   /* Function not implemented */
#define ENOTEMPTY       39   /* Directory not empty */
#define ELOOP           40   /* Too many symbolic links */
#define EOVERFLOW       75   /* Value too large for defined data type */

#endif /* VINIX_ERRNO_H */
