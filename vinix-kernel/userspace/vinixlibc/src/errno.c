/* ============================================================
 * errno.c — single-threaded global for MVP. Per-task storage
 * will arrive with threads; until then every syscall wrapper
 * stashes its error here.
 * ============================================================ */

#include "errno.h"

int errno = 0;
