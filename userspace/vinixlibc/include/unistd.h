/* ============================================================
 * unistd.h — POSIX process + I/O glue, wraps the VinixOS SVC ABI
 * ============================================================ */

#ifndef _VINIXLIBC_UNISTD_H
#define _VINIXLIBC_UNISTD_H

#include "types.h"

typedef int32_t pid_t;
typedef int32_t ssize_t;

/* --- file I/O --- */
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int     open(const char *path, int flags);
int     close(int fd);
int     dup(int oldfd);
int     dup2(int oldfd, int newfd);

/* --- process --- */
pid_t   fork(void);
int     execve(const char *path, char *const argv[], char *const envp[]);
void    _exit(int status) __attribute__((noreturn));
pid_t   wait(int *status);
pid_t   waitpid(pid_t pid, int *status, int options);
pid_t   getpid(void);
pid_t   getppid(void);
int     kill(pid_t pid, int sig);

/* --- time-ish --- */
unsigned int sleep(unsigned int seconds);
int          usleep(unsigned int microseconds);

/* --- filesystem stubs (return -1 + ENOSYS) --- */
int     pipe(int pipefd[2]);
int     mkdir(const char *path, int mode);
int     rmdir(const char *path);
int     unlink(const char *path);
int     rename(const char *oldp, const char *newp);
int     chdir(const char *path);
char   *getcwd(char *buf, size_t size);
int     access(const char *path, int mode);

#endif /* _VINIXLIBC_UNISTD_H */
