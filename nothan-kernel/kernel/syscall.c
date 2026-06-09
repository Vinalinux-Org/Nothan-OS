/*
 * kernel/syscall.c - Syscall table and handler implementations
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/syscall.h>
#include <nothan/sched.h>
#include <nothan/printk.h>
#include <nothan/slab.h>

/**
 * sys_yield - yield the CPU to the next runnable task
 *
 * Calls schedule() so the scheduler picks the next task.
 * The caller resumes when it is scheduled again.
 */
static long sys_yield(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a0; (void)a1; (void)a2;
	schedule();
	return 0;
}

/**
 * sys_exit - terminate the current task
 * @a0: exit code
 *
 * Calls do_exit() which handles resource cleanup and schedule().
 */
static long sys_exit(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	do_exit((int)a0);
	/* NOTREACHED */
	return -1;
}

/**
 * sys_getppid - return the PID of the parent process
 *
 * Return: parent->pid, or 0 if no parent.
 */
static long sys_getppid(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a0; (void)a1; (void)a2;
	struct task_struct *p = runqueue.curr->parent;
	return p ? (long)p->pid : 0;
}

/**
 * sys_getpid - return the PID of the calling task
 *
 * Return: current->pid cast to long.
 */
static long sys_getpid(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a0; (void)a1; (void)a2;
	/* Linux: getpid() returns tgid (process ID, not thread ID) */
	return (long)runqueue.curr->tgid;
}

/**
 * sys_write - write a NUL-terminated string to the kernel log
 * @a0: user VA of the string (must be a valid kernel-accessible pointer)
 *
 * Return: 0 on success, -1 if ptr is NULL.
 */
static long sys_write(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;

	const char *buf = (const char *)a0;

	if (!buf)
		return -1L;

	printk("%s", buf);
	return 0;
}

/*
 * Syscall dispatch table
 * Indexed by syscall number; must match __NR_xxx constants.
 */
typedef long (*syscall_fn_t)(unsigned long, unsigned long, unsigned long);

static const syscall_fn_t syscall_table[NR_SYSCALLS] = {
	[__NR_yield]   = sys_yield,
	[__NR_exit]    = sys_exit,
	[__NR_getpid]  = sys_getpid,
	[__NR_write]   = sys_write,
	[__NR_getppid] = sys_getppid,
};

/**
 * do_syscall - central syscall dispatcher
 * @nr:   syscall number (from SVC instruction imm24)
 * @arg0: first argument  (caller's r0)
 * @arg1: second argument (caller's r1)
 * @arg2: third argument  (caller's r2)
 *
 * Validates the syscall number, invokes the handler, and returns
 * the result which the trampoline places into the caller's r0.
 *
 * Return: handler return value, or -1 on invalid syscall.
 */
long do_syscall(unsigned int nr, unsigned long arg0,
		unsigned long arg1, unsigned long arg2)
{
	if (nr >= NR_SYSCALLS) {
		printk("[SYSCALL] invalid syscall %u from pid=%d\n",
		       nr, runqueue.curr ? runqueue.curr->pid : -1);
		return -1L;
	}

	return syscall_table[nr](arg0, arg1, arg2);
}
