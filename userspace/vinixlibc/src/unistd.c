/* ============================================================
 * unistd.c — POSIX-shape wrappers over the VinixOS sys_* ABI.
 *
 * Errors in POSIX-style return -1 + errno; VinixOS syscalls
 * return a negative E_* code directly. These helpers fold the
 * mapping so callers can follow POSIX idioms.
 * ============================================================ */

#include "unistd.h"
#include "errno.h"
#include "user_syscall.h"
#include "syscalls.h"
#include "string.h"

/* Shared translator from kernel E_* (negative) to POSIX errno. */
static int set_errno_from(int rc)
{
    if (rc >= 0) return rc;
    switch (rc) {
    case E_INVAL: errno = EINVAL; break;
    case E_ARG:   errno = EINVAL; break;
    case E_PTR:   errno = EFAULT; break;
    case E_PERM:  errno = EACCES; break;
    case E_NOENT: errno = ENOENT; break;
    case E_BADF:  errno = EBADF;  break;
    case E_MFILE: errno = EMFILE; break;
    case E_FAIL:
    default:      errno = EIO;    break;
    }
    return -1;
}

/* ============================================================
 * File I/O
 * ============================================================ */

ssize_t read(int fd, void *buf, size_t count)
{
    int rc = (fd == 0) ? sys_read(buf, (uint32_t)count)
                       : sys_read_file(fd, buf, (uint32_t)count);
    return (ssize_t)set_errno_from(rc);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    int rc = (fd == 1 || fd == 2) ? sys_write(buf, (uint32_t)count)
                                  : sys_write_file(fd, buf, (uint32_t)count);
    return (ssize_t)set_errno_from(rc);
}

int open(const char *path, int flags)
{
    return set_errno_from(sys_open(path, flags));
}

int close(int fd)
{
    return set_errno_from(sys_close(fd));
}

int dup(int oldfd)
{
    return set_errno_from(sys_dup(oldfd));
}

int dup2(int oldfd, int newfd)
{
    return set_errno_from(sys_dup2(oldfd, newfd));
}

/* ============================================================
 * Process control
 * ============================================================ */

pid_t fork(void)
{
    return (pid_t)set_errno_from(sys_fork());
}

int execve(const char *path, char *const argv[], char *const envp[])
{
    (void)envp;
    int rc = sys_exec(path, (char **)argv);
    /* sys_exec returns only on failure. */
    return set_errno_from(rc);
}

void _exit(int status)
{
    sys_exit(status);
    while (1) { }
}

pid_t wait(int *status)
{
    int rc = sys_wait(status);
    if (rc < 0) { errno = ECHILD; return -1; }
    return rc;
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    /* MVP kernel: wait-any only. Honour only pid == -1 (any child). */
    (void)options;
    if (pid != -1) { errno = EINVAL; return -1; }
    return wait(status);
}

pid_t getpid(void)  { return sys_getpid(); }
pid_t getppid(void) { return sys_getppid(); }

int kill(pid_t pid, int sig)
{
    return set_errno_from(sys_kill(pid, sig));
}

/* ============================================================
 * Time-ish
 *
 * The kernel has no sleep() syscall exposed to userspace yet — we
 * spin the scheduler via sys_yield which, combined with idle-task
 * time slicing, at least surrenders the CPU for the calling task.
 * Not accurate, but matches the "no polling busy-waits" rule.
 * ============================================================ */

unsigned int sleep(unsigned int seconds)
{
    /* 100 iterations per ~1 tick (10 ms). No hard real-time guarantee. */
    uint32_t ticks = seconds * 100;
    while (ticks--) sys_yield();
    return 0;
}

int usleep(unsigned int microseconds)
{
    (void)microseconds;
    sys_yield();
    return 0;
}

/* ============================================================
 * Unsupported — stub with errno = ENOSYS-equivalent (EINVAL today)
 * ============================================================ */

int pipe(int pipefd[2])     { (void)pipefd; errno = EINVAL; return -1; }
int mkdir(const char *p, int m) { (void)p; (void)m; errno = EINVAL; return -1; }
int rmdir(const char *p)    { (void)p; errno = EINVAL; return -1; }
int chdir(const char *p)    { (void)p; errno = EINVAL; return -1; }
int access(const char *p, int mode) { (void)p; (void)mode; errno = EINVAL; return -1; }

int unlink(const char *path)
{
    int rc;
    __asm__ __volatile__(
        "mov r7, #20\n\t"       /* SYS_UNLINK */
        "mov r0, %1\n\t"
        "svc #0\n\t"
        "mov %0, r0\n\t"
        : "=r"(rc) : "r"(path) : "r0", "r7", "memory");
    return set_errno_from(rc);
}

int rename(const char *oldp, const char *newp)
{
    int rc;
    __asm__ __volatile__(
        "mov r7, #21\n\t"       /* SYS_RENAME */
        "mov r0, %1\n\t"
        "mov r1, %2\n\t"
        "svc #0\n\t"
        "mov %0, r0\n\t"
        : "=r"(rc) : "r"(oldp), "r"(newp) : "r0", "r1", "r7", "memory");
    return set_errno_from(rc);
}

char *getcwd(char *buf, size_t size)
{
    if (!buf || size < 2) { errno = ERANGE; return 0; }
    buf[0] = '/';
    buf[1] = '\0';
    return buf;
}
