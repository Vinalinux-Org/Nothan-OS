/* ============================================================
 * signal.c — handler registration is cosmetic; kill() is the
 * only delivery path and it lives in unistd.c.
 * ============================================================ */

#include "signal.h"
#include "unistd.h"

sighandler_t signal(int signum, sighandler_t handler)
{
    /* Accept without storing. Userspace signal handlers aren't
     * plumbed through the kernel yet — call sites that assume
     * they work keep the source portable but see no callbacks. */
    (void)signum;
    return handler;
}

int raise(int sig)
{
    return kill(getpid(), sig);
}
