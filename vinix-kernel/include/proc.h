/* ============================================================
 * proc.h
 * ------------------------------------------------------------
 * fork/wait/exit — process lifecycle.
 * ============================================================ */

#ifndef PROC_H
#define PROC_H

#include "types.h"
#include "svc_context.h"

int  do_fork(struct svc_context *parent_ctx);
void do_exit(int status);
int  do_wait(int *status_out);
int  do_kill_by_pid(int pid, int exit_status);
int  do_exec(const char *path, struct svc_context *ctx, char **argv_user);

#endif
