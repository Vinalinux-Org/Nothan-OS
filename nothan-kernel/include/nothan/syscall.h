#ifndef _NOTHAN_SYSCALL_H
#define _NOTHAN_SYSCALL_H

#include <nothan/types.h>

/*
 * Syscall numbers — passed in r7 (Linux convention), svc #0 used as trigger.
 */
#define __NR_yield      0   /* yield CPU to next runnable task    */
#define __NR_exit       1   /* terminate the current task         */
#define __NR_getpid     2   /* return current task PID            */
#define __NR_write      3   /* write NUL-terminated string to log */
#define __NR_open       5   /* open a file                       */
#define __NR_read       6   /* read from file descriptor         */
#define __NR_writefile  7   /* write to file descriptor          */
#define __NR_close      8   /* close a file descriptor           */
#define __NR_gettasklist 9  /* get list of running tasks         */
#define __NR_sysinfo    10  /* get system info                   */
#define __NR_listdir    11  /* list directory contents           */
#define __NR_kill       13  /* terminate a task by PID           */
#define __NR_reboot     14  /* reboot or halt the system         */
#define __NR_uname      15  /* get OS/kernel name string         */
#define __NR_ioctl      16  /* device control                    */
#define __NR_chdir      17  /* change working directory          */
#define __NR_getcwd     18  /* get current working directory     */
#define __NR_getticks   19  /* get system tick count in ms       */
#define __NR_sleep      20  /* block the task for N milliseconds  */
#define __NR_sock_open  21  /* bind a UDP port, return a descriptor */
#define __NR_sock_send  22  /* send one datagram to an address    */
#define __NR_sock_recv  23  /* take one datagram, without blocking */

#define NR_SYSCALLS     24

/* reboot commands */
#define REBOOT_WARM     0
#define REBOOT_HALT     1

/* Data structures for syscall arguments */
#define TASK_NAME_LEN 16

/*
 * Duplicated verbatim in userspace/lib/syscall.h — the two must stay
 * identical, since this crosses the syscall boundary and nothing checks it.
 */
struct task_info {
	int pid;
	char name[TASK_NAME_LEN];
	int state;
	int prio;
	unsigned long cpu_us;	/* CPU time consumed */
	unsigned long picked;	/* times the scheduler chose it */
};

struct sys_info {
	unsigned long total_pages;
	unsigned long free_pages;
};

/*
 * A datagram and the address at the other end of it.
 *
 * One struct for both directions — @addr is where it is going on a send and
 * where it came from on a receive — because the ABI here passes three
 * registers and a send needs five values.  Splitting it into two structs would
 * describe the same bytes twice, and this header already carries a warning
 * about what happens when one fact has two definitions.
 *
 * @len on a receive is the size of @buf going in and the size of the datagram
 * coming out; a datagram longer than @buf is truncated, and the return value
 * says how much was kept, not how much arrived.
 */
struct sock_addr {
	unsigned char  ip[4];
	unsigned short port;		/* host byte order */
};

struct sock_msg {
	void            *buf;
	unsigned int     len;
	struct sock_addr addr;
};

/**
 * do_syscall() - central syscall dispatcher
 * @nr:   syscall number (from r7)
 * @arg0: first argument  (caller's r0)
 * @arg1: second argument (caller's r1)
 * @arg2: third argument  (caller's r2)
 *
 * Called from vector_svc trampoline in vectors.S.
 * Return: value placed in caller's r0 on return.
 */
long do_syscall(unsigned int nr, unsigned long arg0,
		unsigned long arg1, unsigned long arg2);

#endif /* _NOTHAN_SYSCALL_H */
