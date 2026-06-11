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

	return vfs_listdir(path, buf, (int)max);
}

/**
 * sys_spawn - load and run a .bin file from the mounted filesystem
 * @a0: path to .bin file (user)
 *
 * Opens the file, reads its content, creates a new user task,
 * and enqueues it. Returns the new PID, or -1 on error.
 */
static long sys_spawn(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	const char *path = (const char *)a0;
	unsigned long blob_size = 4096;

	int fd = vfs_open(path, 0);
	if (fd < 0)
		return -1;

	u8 *buf = (u8 *)kmalloc(blob_size, GFP_KERNEL);
	if (!buf) {
		vfs_close(fd);
		return -1;
	}

	int bytes = vfs_read(fd, (char *)buf, blob_size);
	vfs_close(fd);
	if (bytes <= 0) {
		kfree(buf);
		return -1;
	}

	struct task_struct *tsk = user_task_create_bin(path, (char *)buf, (char *)(buf + bytes));
	kfree(buf);
	if (!tsk)
		return -1;

	enqueue_task(&runqueue, tsk);
	return (long)tsk->pid;
}

/**
 * sys_kill - terminate a task by PID
 * @a0: PID of the target task
 *
 * If a0 matches the current task, delegates to do_exit().
 * Otherwise finds the task in the runqueue, dequeues it,
 * and frees its resources.
 *
 * Return: 0 on success, -1 if PID not found.
 */
static long sys_kill(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;
	int target_pid = (int)a0;

	if (target_pid <= 1)
		return -1;

	if (runqueue.curr && runqueue.curr->pid == target_pid) {
		do_exit(0);
		/* NOTREACHED */
		return 0;
	}

	struct rq *rq = &runqueue;
	for (int prio = 0; prio < MAX_PRIO; prio++) {
		struct sched_rt_entity *rt, *tmp;
		list_for_each_entry_safe(rt, tmp, &rq->active.queue[prio],
					 struct sched_rt_entity, run_list) {
			struct task_struct *tsk = container_of(rt, struct task_struct, rt);
			if (tsk->pid != target_pid)
				continue;

			dequeue_task(rq, tsk);
			tsk->exit_state = EXIT_ZOMBIE;
			tsk->__state = TASK_UNINTERRUPTIBLE;

			if (tsk->mm) {
				struct zone *zone = get_zone();
				if (tsk->mm->l2)
					kfree(tsk->mm->l2);
				struct page *cp = pfn_to_page(zone,
					(tsk->mm->code_pa - zone->base_pa) >> PAGE_SHIFT);
				if (cp)
					__free_pages(cp, 0);
				struct page *sp = pfn_to_page(zone,
					(tsk->mm->stack_pa - zone->base_pa) >> PAGE_SHIFT);
				if (sp)
					__free_pages(sp, 0);
				kfree(tsk->mm);
				tsk->mm = NULL;
			}

			printk("[KILL] task \"%s\" pid=%d killed\n", tsk->comm, tsk->pid);
			return 0;
		}
	}
	return -1;
}

/**
 * sys_reboot - reboot or halt the system
 * @a0: REBOOT_WARM (0) or REBOOT_HALT (1)
 */
static long sys_reboot(unsigned long a0, unsigned long a1, unsigned long a2)
{
	(void)a1; (void)a2;

	if ((int)a0 == REBOOT_HALT) {
		printk("[SYS] Halting...\n");
		__asm__ __volatile__("cpsid i" : : : "memory");
		while (1)
			;
	}

	/* Warm reset via PRM_RSTCTRL bit 0 (RST_GLOBAL_WARM_SW)
	 * PRM_DEVICE PA 0x44E00F00 → VA 0xF0E00F00 */
	printk("[SYS] Rebooting...\n");
	mmio_write32(0xF0E00F00, 1);

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
	[__NR_open]    = sys_open,
	[__NR_read]    = sys_read,
	[__NR_writefile] = sys_writefile,
	[__NR_close]   = sys_close,
	[__NR_gettasklist] = sys_gettasklist,
	[__NR_sysinfo]     = sys_sysinfo,
	[__NR_listdir]     = sys_listdir,
	[__NR_spawn]       = sys_spawn,
	[__NR_kill]        = sys_kill,
	[__NR_reboot]      = sys_reboot,
	[__NR_uname]       = sys_uname,
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
