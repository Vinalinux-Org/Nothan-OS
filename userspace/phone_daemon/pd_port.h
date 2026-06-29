/*
 * pd_port.h - NothanOS userspace port shim for phone_daemon.
 *
 * Replaces the POSIX/newlib headers (stdio/stdlib/unistd/fcntl/errno/ctype)
 * the daemon was written against. Provides exactly what the daemon uses,
 * backed by the thin nothan_v2 lib (no newlib, no heap).
 */
#ifndef PD_PORT_H
#define PD_PORT_H

#include "../lib/syscall.h"	/* open, read, close, ioctl, getticks, yield, writefile */
#include "../lib/printf.h"	/* printf, snprintf, vsnprintf */
#include "../lib/string.h"	/* str*, mem*, atoi, size_t */
#include "../lib/types.h"	/* uintN_t, ssize_t */

#ifndef NULL
#  define NULL ((void *)0)
#endif

/* open() flags — the char-device ignores them, but the source references them. */
#ifndef O_RDWR
#  define O_RDWR     0x0002
#endif
#ifndef O_NOCTTY
#  define O_NOCTTY   0
#endif
#ifndef O_NONBLOCK
#  define O_NONBLOCK 0
#endif

/*
 * errno shim. nothan_v2 read()/write() never set errno: a read of -1 just
 * means "no data this tick". The daemon's `errno == EAGAIN` branches are made
 * harmless — errno stays 0, and the read loops are adapted to treat n<=0 as
 * no-data (recovery is driven by the AT heartbeat, not by read returns).
 */
extern int errno;
#define EAGAIN       11
#define EWOULDBLOCK  EAGAIN
#define EINTR        4
#define EINVAL       22

/* POSIX write(fd,buf,len) -> nothan writefile(); nothan's 1-arg write() unused. */
#define write(fd, buf, len)  writefile((fd), (buf), (len))

#endif /* PD_PORT_H */
