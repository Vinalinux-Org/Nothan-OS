#ifndef __USER_SYSCALL_H
#define __USER_SYSCALL_H

#define __NR_yield   0
#define __NR_exit    1
#define __NR_getpid  2
#define __NR_write   3

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

static inline void yield(void)
{
	__syscall0(__NR_yield);
}

static inline void user_exit(int status)
{
	__syscall1(__NR_exit, status);
}

static inline long getpid(void)
{
	return __syscall0(__NR_getpid);
}

static inline long write(const char *buf)
{
	return __syscall1(__NR_write, (long)buf);
}

#endif
