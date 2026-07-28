#ifndef _HOST_SCHED_H
#define _HOST_SCHED_H
/*
 * tools/test/host_sched.h - task handle for the host harness.
 * See host_sched.c for the model this implements.
 */

#include <pthread.h>
#include <nothan/sched.h>

#define HOST_MAX_TASKS	32

/*
 * task_struct FIRST: the kernel code under test only ever sees &h->t, and the
 * harness maps back with a table lookup rather than a cast, so the layout is
 * not load-bearing - but keeping it first makes the relationship obvious.
 */
struct host_task {
	struct task_struct	t;
	pthread_cond_t		cond;	/* "this task is runnable again" */
	pthread_t		thread;
};

struct host_task *host_task_create(const char *name);
void		  host_task_run(struct host_task *h, void (*fn)(void *), void *arg);
void		  host_task_join(struct host_task *h);
struct host_task *host_task_self(void);

#endif /* _HOST_SCHED_H */
