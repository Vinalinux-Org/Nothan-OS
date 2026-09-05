#ifndef _NOTHAN_SOCKET_H
#define _NOTHAN_SOCKET_H

/*
 * include/nothan/socket.h - what a user process gets instead of a kernel task
 *
 * The syscall layer above these is a thin translation of user pointers; the
 * decisions all live in net/socket.c, and the reasoning for the pool is there
 * too.  Nothing outside the syscall path should call them.
 */

#include <nothan/types.h>
#include <nothan/fs.h>
#include <nothan/syscall.h>
/* For UDP_MAX_PAYLOAD, which the syscall layer bounds user lengths against.
 * Included here rather than restated so the limit has one definition. */
#include <nothan/udp.h>

int  sock_open(u16 port);
int  sock_send(struct file *file, const struct sock_addr *addr,
	       const void *data, unsigned int len);
int  sock_recv(struct file *file, struct sock_addr *out, void *buf,
	       unsigned int len);
void sock_dump_stats(void);

/* Identity of a socket descriptor: compare f_op against this before trusting
 * private_data, since a process can hand any fd number to any syscall. */
extern const struct file_operations sock_fops;

#endif /* _NOTHAN_SOCKET_H */
