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

#define NR_SYSCALLS     5

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
