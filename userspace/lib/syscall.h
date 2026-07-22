#ifndef __USER_SYSCALL_H
#define __USER_SYSCALL_H

#define __NR_yield      0
#define __NR_exit       1
#define __NR_getpid     2
#define __NR_write      3
#define __NR_open       5
#define __NR_read       6
#define __NR_writefile  7
#define __NR_close      8
#define __NR_gettasklist 9
#define __NR_sysinfo    10
#define __NR_listdir    11
#define __NR_kill       13
#define __NR_reboot     14
#define __NR_uname      15
#define __NR_ioctl      16
#define __NR_chdir      17
#define __NR_getcwd     18
#define __NR_getticks   19
/* 20 = __NR_sleep (kernel) — not wrapped here */
#define __NR_msgq_send  21
#define __NR_msgq_recv  22

#define REBOOT_WARM     0
#define REBOOT_HALT     1

/* open() flags — must match kernel include/nothan/fs.h */
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0040

#define TASK_NAME_LEN 16
#define UNAME_LEN     16
#define FILE_NAME_LEN 32

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

struct file_entry {
	char name[FILE_NAME_LEN];
	unsigned long size;
};

static inline long __syscall2(long nr, long a0, long a1)
{
	register long r0 __asm__("r0") = a0;
	register long r1 __asm__("r1") = a1;
	register long r7 __asm__("r7") = nr;
	__asm__ __volatile__("svc #0"
			     : "+r"(r0)
			     : "r"(r7), "r"(r1)
			     : "r2", "r3", "memory");
	return r0;
}

static inline long __syscall3(long nr, long a0, long a1, long a2)
{
	register long r0 __asm__("r0") = a0;
	register long r1 __asm__("r1") = a1;
	register long r2 __asm__("r2") = a2;
	register long r7 __asm__("r7") = nr;
	__asm__ __volatile__("svc #0"
			     : "+r"(r0)
			     : "r"(r7), "r"(r1), "r"(r2)
			     : "memory");
	return r0;
}

static inline long __syscall1(long nr, long a0)
{
	register long r0 __asm__("r0") = a0;
	register long r7 __asm__("r7") = nr;
	__asm__ __volatile__("svc #0"
			     : "+r"(r0)
			     : "r"(r7)
			     : "r1", "r2", "r3", "memory");
	return r0;
}

static inline long __syscall0(long nr)
{
	register long r0 __asm__("r0") = 0;
	register long r7 __asm__("r7") = nr;
	__asm__ __volatile__("svc #0"
			     : "+r"(r0)
			     : "r"(r7)
			     : "r1", "r2", "r3", "memory");
	return r0;
}

static inline void yield(void)		{ __syscall0(__NR_yield); }
static inline void user_exit(int s)	{ __syscall1(__NR_exit, s); }
static inline long getpid(void)		{ return __syscall0(__NR_getpid); }
static inline long write(const char *b){ return __syscall1(__NR_write, (long)b); }

static inline long open(const char *path, int flags)
{
	return __syscall2(__NR_open, (long)path, (long)flags);
}

static inline long read(int fd, void *buf, unsigned long count)
{
	return __syscall3(__NR_read, (long)fd, (long)buf, (long)count);
}

static inline long writefile(int fd, const void *buf, unsigned long count)
{
	return __syscall3(__NR_writefile, (long)fd, (long)buf, (long)count);
}

static inline long close(int fd)
{
	return __syscall1(__NR_close, (long)fd);
}

static inline long gettasklist(struct task_info *buf, unsigned long max)
{
	return __syscall2(__NR_gettasklist, (long)buf, (long)max);
}

static inline long sysinfo(struct sys_info *buf)
{
	return __syscall1(__NR_sysinfo, (long)buf);
}

static inline long listdir(const char *path, struct file_entry *buf, unsigned long max)
{
	return __syscall3(__NR_listdir, (long)path, (long)buf, (long)max);
}

struct uname_info {
	char sysname[UNAME_LEN];
	char version[UNAME_LEN];
	char machine[UNAME_LEN];
};

static inline long kill(int pid)
{
	return __syscall1(__NR_kill, (long)pid);
}

static inline long reboot(int cmd)
{
	return __syscall1(__NR_reboot, (long)cmd);
}

static inline long uname(struct uname_info *buf)
{
	return __syscall1(__NR_uname, (long)buf);
}

static inline long ioctl(int fd, unsigned int cmd, unsigned long arg)
{
	return __syscall3(__NR_ioctl, (long)fd, (long)cmd, (long)arg);
}

static inline long chdir(const char *path)
{
	return __syscall1(__NR_chdir, (long)path);
}

static inline long getcwd(char *buf, unsigned long size)
{
	return __syscall2(__NR_getcwd, (long)buf, (long)size);
}

static inline unsigned long getticks(void)
{
	return (unsigned long)__syscall0(__NR_getticks);
}

/* IPC: send/recv a fixed-size message to/from system queue @qid.
 * msgq_recv blocks while the queue is empty; msgq_send blocks while full. */
static inline long msgq_send(int qid, const void *msg, unsigned long len)
{
	return __syscall3(__NR_msgq_send, (long)qid, (long)msg, (long)len);
}

static inline long msgq_recv(int qid, void *out, unsigned long len)
{
	return __syscall3(__NR_msgq_recv, (long)qid, (long)out, (long)len);
}

#endif
