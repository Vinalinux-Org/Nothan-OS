/* ============================================================
 * sys/wait.h — status-decoding macros for wait() / waitpid()
 *
 * MVP encoding (kernel side):
 *   Normal exit → exit_status is the low byte of exit code.
 *   SIGSEGV / SIGKILL → 128 + signal (139 for SEGV, 137 for KILL).
 * ============================================================ */

#ifndef _VINIXLIBC_SYS_WAIT_H
#define _VINIXLIBC_SYS_WAIT_H

#include "unistd.h"

#define WIFEXITED(status)   (((status) & 0x80) == 0)
#define WEXITSTATUS(status) ((status) & 0x7F)
#define WIFSIGNALED(status) (((status) & 0x80) != 0)
#define WTERMSIG(status)    ((status) & 0x7F)

#endif /* _VINIXLIBC_SYS_WAIT_H */
