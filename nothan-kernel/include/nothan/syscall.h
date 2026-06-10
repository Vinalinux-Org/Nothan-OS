#ifndef _NOTHAN_SYSCALL_H
#define _NOTHAN_SYSCALL_H

#include <nothan/types.h>

/*
 * Syscall numbers — passed in r7 (Linux convention), svc #0 used as trigger.
 */
#define __NR_yield      0   /* yield CPU to next runnable task    */
#define __NR_exit       1   /* terminate the current task         */
#define __NR_getpid     2   /* return current task PID (tgid)     */
#define __NR_write      3   /* write NUL-terminated string to log */
#define __NR_getppid    4   /* return parent PID                  */
#define __NR_open       5   /* open a file                       */
#define __NR_read       6   /* read from file descriptor         */
#define __NR_writefile  7   /* write to file descriptor          */
#define __NR_close      8   /* close a file descriptor           */
#define __NR_gettasklist 9  /* get list of running tasks         */
#define __NR_sysinfo    10  /* get system info                   */
#define __NR_listdir    11  /* list directory contents           */
#define __NR_spawn      12  /* spawn a .bin file from VFS        */

#define NR_SYSCALLS     13

/* Data structures for syscall arguments */
#define TASK_NAME_LEN 16

struct task_info {
	int pid;
	char name[TASK_NAME_LEN];
	int state;
	int prio;
};

struct sys_info {
	unsigned long total_pages;
	unsigned long free_pages;
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
