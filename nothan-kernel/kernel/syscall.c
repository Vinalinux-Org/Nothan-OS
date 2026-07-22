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
#include <nothan/fs.h>
#include <nothan/uart.h>
#include <nothan/mm.h>
#include <nothan/mmio.h>
#include <nothan/time.h>
#include <nothan/delay.h>
#include <nothan/uaccess.h>
#include <nothan/msgq.h>

/* Longest path/string a syscall will scan out of user space. */
#define USER_STR_MAX	256
/* Sanity cap on user-supplied array counts, so count*sizeof can't wrap. */
#define USER_ARR_MAX	65536

extern struct task_struct *user_task_create_bin(const char *name,
	char *blob_start, char *blob_end);

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
 * sys_getpid - return the PID of the calling task
 *
 * Return: current->pid cast to long.
 */
static long sys_getpid(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a0; (void)a1; (void)a2;
	return (long)runqueue.curr->pid;
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
	/* Reject a kernel/unmapped or unterminated string before printk scans it. */
	if (strnlen_user(buf, USER_STR_MAX) < 0)
		return -1L;

	printk("%s", buf);
	return 0;
}

/**
 * sys_open - open a file
 * @a0: pathname
 * @a1: flags (O_RDONLY, O_WRONLY, etc.)
 *
 * Return: file descriptor, or -1 on error.
 */
static long sys_open(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a2;
	if (strnlen_user((const char *)a0, USER_STR_MAX) < 0)
		return -1;
	return vfs_open((const char *)a0, (int)a1);
}

/**
 * sys_read - read from a file descriptor
 * @a0: file descriptor
 * @a1: buffer
 * @a2: count
 *
 * fd=0 reads from UART (stdin). Other fds read from VFS.
 * Return: bytes read, 0 on EOF, -1 on error.
 */
static long sys_read(unsigned long a0, unsigned long a1, unsigned long a2)
{
	int fd = (int)a0;
	char *buf = (char *)a1;
	size_t count = (size_t)a2;

	/* The kernel will write up to @count bytes into @buf; prove the whole
	 * range is the caller's own user memory (not kernel/unmapped). */
	if (!access_ok(buf, count))
		return -1;

	if (fd == 0) {
		for (size_t i = 0; i < count; i++) {
			int c = uart_getchar();
			if (c < 0)
				return i > 0 ? (long)i : -1;
			buf[i] = (char)c;
		}
		return (long)count;
	}

	return vfs_read(fd, buf, count);
}

/**
 * sys_writefile - write to a file descriptor
 * @a0: file descriptor
 * @a1: buffer
 * @a2: count
 *
 * fd=1 writes to UART (stdout). Other fds write to VFS.
 * Return: bytes written, or -1 on error.
 */
static long sys_writefile(unsigned long a0, unsigned long a1, unsigned long a2)
{
	int fd = (int)a0;
	const char *buf = (const char *)a1;
	size_t count = (size_t)a2;

	/* The kernel will read @count bytes from @buf; validate the source. */
	if (!access_ok(buf, count))
		return -1;

	if (fd == 1) {
		for (size_t i = 0; i < count; i++)
			uart_putchar((unsigned char)buf[i]);
		return (long)count;
	}

	return vfs_write(fd, buf, count);
}

/**
 * sys_close - close a file descriptor
 * @a0: file descriptor
 *
 * Return: 0 on success, -1 on error.
 */
static long sys_close(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	return vfs_close((int)a0);
}

/**
 * sys_gettasklist - fill buffer with task info
 * @a0: task_info buffer (user)
 * @a1: max entries
 *
 * Return: number of tasks written, or -1 on error.
 */
static long sys_gettasklist(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a2;
	struct task_info *buf = (struct task_info *)a0;
	unsigned long max = a1;
	unsigned long count = 0;
	struct rq *rq = &runqueue;

	if (max > USER_ARR_MAX || !access_ok(buf, max * sizeof(struct task_info)))
		return -1;

	/* Always include the currently running task */
	if (count < max && rq->curr) {
		buf[count].pid = rq->curr->pid;
		buf[count].state = rq->curr->__state;
		buf[count].prio = rq->curr->prio;
		unsigned int i;
		for (i = 0; i < TASK_NAME_LEN - 1 && rq->curr->comm[i]; i++)
			buf[count].name[i] = rq->curr->comm[i];
		buf[count].name[i] = '\0';
		count++;
	}

	/* Then iterate the runqueue for other tasks */
	for (int prio = 0; prio < MAX_PRIO && count < max; prio++) {
		struct list_head *pos;
		list_for_each(pos, &rq->active.queue[prio]) {
			if (count >= max)
				break;
			struct sched_rt_entity *rt = list_entry(pos, struct sched_rt_entity, run_list);
			struct task_struct *tsk = container_of(rt, struct task_struct, rt);
			buf[count].pid = tsk->pid;
			buf[count].state = tsk->__state;
			buf[count].prio = tsk->prio;
			unsigned int i;
			for (i = 0; i < TASK_NAME_LEN - 1 && tsk->comm[i]; i++)
				buf[count].name[i] = tsk->comm[i];
			buf[count].name[i] = '\0';
			count++;
		}
	}
	return (long)count;
}

/**
 * sys_sysinfo - fill buffer with system info
 * @a0: sys_info buffer (user)
 */
static long sys_sysinfo(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	struct sys_info *buf = (struct sys_info *)a0;
	struct zone *z = get_zone();

	if (!access_ok(buf, sizeof(struct sys_info)))
		return -1;

	buf->total_pages = z->managed_pages;
	buf->free_pages = z->free_pages;
	return 0;
}

/**
 * sys_listdir - list directory files
 * @a0: path (user)
 * @a1: file_entry buffer (user)
 * @a2: max entries
 *
 * Uses VFS to look up files. Currently only supports root directory.
 */
static long sys_listdir(unsigned long a0, unsigned long a1, unsigned long a2)
{
	const char *path = (const char *)a0;
	struct file_entry *buf = (struct file_entry *)a1;
	unsigned long max = a2;

	if (strnlen_user(path, USER_STR_MAX) < 0)
		return -1;
	if (max > USER_ARR_MAX || !access_ok(buf, max * sizeof(struct file_entry)))
		return -1;

	return vfs_listdir(path, buf, (int)max);
}

/**
 * sys_kill - request a task to terminate, by PID (cooperative)
 * @a0: PID of the target task
 *
 * Does NOT free anything in place. Sets TASK_SHOULD_EXIT on the target; the
 * target then calls do_exit() itself at its next syscall boundary (a safe
 * point — never mid-FAT-write). The flat task registry is used to find the
 * target, so a task blocked off the runqueue is found too (the old runqueue
 * scan missed those).
 *
 * Return: 0 if the request was posted, -1 otherwise.
 */
static long sys_kill(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	int target_pid = (int)a0;

	if (target_pid <= 1)			/* protect idle(0) + first task/gui(1) */
		return -1;

	if (runqueue.curr && runqueue.curr->pid == target_pid) {
		do_exit(0);			/* self — exit right here */
		/* NOTREACHED */
		return 0;
	}

	struct task_struct *t = task_find(target_pid);
	if (!t)
		return -1;			/* no such pid */
	if (!t->mm)
		return -1;			/* kernel thread (idle/musb) — unkillable */
	if (t->__state == TASK_DEAD)
		return -1;			/* already dying, awaiting reap */

	t->flags |= TASK_SHOULD_EXIT;		/* raise the flag — no free here */
	/*
	 * A blocked target exits when it returns from its current syscall.
	 * Force-waking a target blocked indefinitely is deferred until blocking
	 * I/O exists (there is none yet — reads are non-blocking).
	 */
	return 0;
}

/**
 * sys_reboot - reboot or halt the system
 * @a0: REBOOT_WARM (0) or REBOOT_HALT (1)
 */
extern void lcdc_shutdown(void);

static long sys_reboot(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;

	if ((int)a0 == REBOOT_HALT) {
		printk("[SYS] Halting...\n");
		__asm__ __volatile__("cpsid i" : : : "memory");
		while (1)
			;
	}

	/* Stop LCDC DMA first — a warm reset doesn't reset every peripheral
	 * cleanly, and an in-flight LCDC burst would conflict with the
	 * bootloader's DDR test on the next boot. */
	lcdc_shutdown();
	printk("[SYS] Rebooting...\n");

	/* RST_GLOBAL_COLD_SW (bit 1): pulses DDR3 RESET# so bootloader PHY
	 * leveling starts from a clean state. Warm reset (bit 0) skips RESET#
	 * and causes ddr_init() to fail on the live chip.
	 * PRM_DEVICE PA 0x44E00F00 → VA 0xF0E00F00 */
	mmio_write32(0xF0E00F00, 2);

	/* NOTREACHED — hardware resets before this runs */
	while (1)
		;
	return 0;
}

#define UNAME_LEN 16

struct uname_info {
	char sysname[UNAME_LEN];
	char version[UNAME_LEN];
	char machine[UNAME_LEN];
};

/**
 * sys_uname - return OS identification strings
 * @a0: pointer to struct uname_info
 */
static long sys_uname(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	struct uname_info *u = (struct uname_info *)a0;
	if (!access_ok(u, sizeof(struct uname_info)))
		return -1;
	const char *sysname = "NothanOS";
	const char *version = "1.0";
	const char *machine = "armv7";

	unsigned int i;
	for (i = 0; sysname[i] && i < UNAME_LEN - 1; i++)
		u->sysname[i] = sysname[i];
	u->sysname[i] = '\0';

	for (i = 0; version[i] && i < UNAME_LEN - 1; i++)
		u->version[i] = version[i];
	u->version[i] = '\0';

	for (i = 0; machine[i] && i < UNAME_LEN - 1; i++)
		u->machine[i] = machine[i];
	u->machine[i] = '\0';

	return 0;
}

/**
 * sys_ioctl - device control
 * @a0: file descriptor
 * @a1: ioctl command (encoded with _IOC macros)
 * @a2: argument (pointer or integer value)
 *
 * Return: driver-defined value, or -1 on error.
 */
static long sys_ioctl(unsigned long a0, unsigned long a1, unsigned long a2)
{
	return vfs_ioctl((int)a0, (unsigned int)a1, a2);
}

/**
 * sys_chdir - change the current working directory
 * @a0: path string (must be "/" or a devfs path like "/dev")
 *
 * Only root and devfs paths are allowed. FAT32 subdirectories are rejected.
 * Return: 0 on success, -1 on error or restricted path.
 */
static long sys_chdir(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	const char *path = (const char *)a0;
	if (!path)
		return -1;
	if (strnlen_user(path, USER_STR_MAX) < 0)
		return -1;

	struct task_struct *tsk = runqueue.curr;

	/* Resolve ".." relative to current cwd */
	char resolved[64];
	if (path[0] == '.' && path[1] == '.' && path[2] == '\0') {
		/* Find last '/' in cwd, strip last component */
		unsigned int len = 0;
		while (tsk->cwd[len]) len++;

		/* Strip trailing slash if any (except root) */
		while (len > 1 && tsk->cwd[len - 1] == '/')
			len--;

		/* Walk back to previous '/' */
		while (len > 1 && tsk->cwd[len - 1] != '/')
			len--;

		/* Keep at least "/" */
		if (len == 0) len = 1;

		unsigned int i = 0;
		while (i < len && i < 63) {
			resolved[i] = tsk->cwd[i];
			i++;
		}
		/* Normalise: no trailing slash except root */
		while (i > 1 && resolved[i - 1] == '/')
			i--;
		resolved[i] = '\0';
		path = resolved;
	}

	if (vfs_chdir(path) != 0)
		return -1;

	/* Build normalised absolute cwd: ensure leading '/', no trailing '/' */
	unsigned int i = 0;
	if (path[0] != '/')
		tsk->cwd[i++] = '/';
	unsigned int j = 0;
	while (i < 63 && path[j])
		tsk->cwd[i++] = path[j++];
	while (i > 1 && tsk->cwd[i - 1] == '/')
		i--;
	tsk->cwd[i] = '\0';
	return 0;
}

/**
 * sys_getcwd - get current working directory
 * @a0: user buffer
 * @a1: buffer size
 *
 * Return: 0 on success, -1 on error.
 */
static long sys_getcwd(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a2;
	char *buf = (char *)a0;
	unsigned long size = a1;
	if (!buf || size == 0)
		return -1;
	if (!access_ok(buf, size))
		return -1;

	const char *cwd = runqueue.curr->cwd;
	unsigned int i = 0;
	while (i < size - 1 && cwd[i]) {
		buf[i] = cwd[i];
		i++;
	}
	buf[i] = '\0';
	return 0;
}

/**
 * sys_getticks - get system tick count in milliseconds
 *
 * Return: elapsed milliseconds since boot.
 */
static long sys_getticks(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a0; (void)a1; (void)a2;
	return (long)(get_jiffies() * (1000 / HZ));
}

/**
 * sys_sleep - block the calling task for @a0 milliseconds
 * @a0: milliseconds to sleep
 *
 * Sleeps via a kernel timer + schedule() (msleep), so the CPU can drop to
 * the idle WFI instead of spinning. A GUI/event loop calls this between
 * frames to bound its tick rate without busy-waiting.
 *
 * Return: 0 on success.
 */
static long sys_sleep(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	msleep((unsigned long)a0);
	return 0;
}

/**
 * sys_msgq_send - send a fixed-size message to system queue @a0 (qid)
 * @a0: queue id      @a1: user pointer to message      @a2: buffer length
 *
 * Bridges the user/kernel boundary: pulls the message out of user space with
 * copy_from_user() (validates it lies in the caller's mapping), then hands a
 * kernel buffer to the core msgq_send(). Blocks if the queue is full.
 * Return: 0 on success, -1 on bad qid / short buffer / bad user pointer.
 */
static long sys_msgq_send(unsigned long a0, unsigned long a1, unsigned long a2)
{
	struct msgq *q = msgq_get((unsigned int)a0);
	char kbuf[MSGQ_MSG_SIZE];

	if (!q || a2 < MSGQ_MSG_SIZE)
		return -1;
	if (copy_from_user(kbuf, (const void *)a1, MSGQ_MSG_SIZE))
		return -1;
	msgq_send(q, kbuf);
	return 0;
}

/**
 * sys_msgq_recv - receive a fixed-size message from system queue @a0 (qid)
 * @a0: queue id      @a1: user pointer to output buffer   @a2: buffer length
 *
 * Blocks if the queue is empty, then copies the message out to user space
 * with copy_to_user(). Return: 0 on success, -1 on bad qid/buffer/pointer.
 */
static long sys_msgq_recv(unsigned long a0, unsigned long a1, unsigned long a2)
{
	struct msgq *q = msgq_get((unsigned int)a0);
	char kbuf[MSGQ_MSG_SIZE];

	if (!q || a2 < MSGQ_MSG_SIZE)
		return -1;
	msgq_recv(q, kbuf);
	if (copy_to_user((void *)a1, kbuf, MSGQ_MSG_SIZE))
		return -1;
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
	[__NR_open]    = sys_open,
	[__NR_read]    = sys_read,
	[__NR_writefile] = sys_writefile,
	[__NR_close]   = sys_close,
	[__NR_gettasklist] = sys_gettasklist,
	[__NR_sysinfo]     = sys_sysinfo,
	[__NR_listdir]     = sys_listdir,
	[__NR_kill]        = sys_kill,
	[__NR_reboot]      = sys_reboot,
	[__NR_uname]       = sys_uname,
	[__NR_ioctl]       = sys_ioctl,
	[__NR_chdir]       = sys_chdir,
	[__NR_getcwd]      = sys_getcwd,
	[__NR_getticks]    = sys_getticks,
	[__NR_sleep]       = sys_sleep,
	[__NR_msgq_send]   = sys_msgq_send,
	[__NR_msgq_recv]   = sys_msgq_recv,
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
	long ret;

	if (nr >= NR_SYSCALLS) {
		printk("[SYSCALL] invalid syscall %u from pid=%d\n",
		       nr, runqueue.curr ? runqueue.curr->pid : -1);
		ret = -1L;
	} else {
		ret = syscall_table[nr](arg0, arg1, arg2);
	}

	/*
	 * Cooperative-kill safe point: a task marked TASK_SHOULD_EXIT exits
	 * HERE, at the syscall boundary — never mid-operation. do_exit() runs
	 * on this task's own kernel stack (like sys_exit) and never returns.
	 */
	if (runqueue.curr && (runqueue.curr->flags & TASK_SHOULD_EXIT))
		do_exit(0);

	return ret;
}
