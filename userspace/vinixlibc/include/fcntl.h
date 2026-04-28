/* ============================================================
 * fcntl.h — open(2) flags. Values match the kernel's syscalls.h
 * ============================================================ */

#ifndef _VINIXLIBC_FCNTL_H
#define _VINIXLIBC_FCNTL_H

#define O_RDONLY  0x00
#define O_WRONLY  0x01
#define O_RDWR    0x02
#define O_ACCMODE 0x03
#define O_CREAT   0x04
#define O_TRUNC   0x08
#define O_APPEND  0x10

#endif /* _VINIXLIBC_FCNTL_H */
