/* ============================================================
 * signal.h — minimal MVP subset. Only SIGKILL / SIGSEGV land on
 * the wire; everything else is accepted for source compatibility
 * but ignored.
 * ============================================================ */

#ifndef _VINIXLIBC_SIGNAL_H
#define _VINIXLIBC_SIGNAL_H

#include "unistd.h"

#define SIGHUP   1
#define SIGINT   2
#define SIGKILL  9
#define SIGSEGV 11
#define SIGTERM 15
#define SIGCHLD 17

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)

/* Signal handlers are not delivered to userspace yet — signal()
 * accepts the registration but never invokes the callback. */
sighandler_t signal(int signum, sighandler_t handler);

int raise(int sig);
/* kill() is declared by unistd.h already. */

#endif /* _VINIXLIBC_SIGNAL_H */
