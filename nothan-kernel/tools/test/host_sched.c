/*
 * tools/test/host_sched.c - just enough scheduler for the host harness
 *
 * Lets kernel/sched/wait.c, kernel/ipc/msgq.c and kernel/sched/completion.c be
 * compiled AND RUN UNMODIFIED off-target.  Those three files are the ones whose
 * bugs (lost wakeup, dropped message, torn ring index) are invisible over UART,
 * so they are the ones worth running somewhere with a debugger.
 *
 * THE MODEL
 *
 * One pthread per task.  "IRQs masked" is one global mutex (see
 * tools/test/include/asm/irqflags.h), so at most one task is ever inside a
 * critical section - the same guarantee the single core gives.
 *
 * __schedule() is where the model earns its keep.  On the board, a task that
 * blocks holds the mask across the context switch; the next task restores ITS
 * saved flags, so IRQs come back on for it, and when the first task is switched
 * back in the mask is in place again.  pthread_cond_wait() has exactly that
 * shape: it atomically drops the mutex, blocks, and re-takes it on wake.  So
 * "block with the mask held" maps one-to-one, and the sequence
 *
 *      local_irq_save -> test cond -> __prepare_to_wait -> __schedule
 *
 * is exercised here with the same interleavings the board can produce.
 *
 * WHAT THIS IS NOT
 *
 * Not a scheduler.  There is no runqueue ordering, no vruntime, no preemption:
 * the host OS decides who runs.  place_entity()/pick_next_task() are stubs.
 * Fairness is tested separately and deterministically by fair_sim.c.  What is
 * tested here is only the sleep/wake CONTRACT.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nothan/sched.h>
#include <nothan/wait.h>

#include "host_sched.h"

struct rq runqueue;

/* ------------------------------------------------------------------ */
/* "IRQs masked" == one global lock                                    */
/* ------------------------------------------------------------------ */

static pthread_mutex_t	     big_lock = PTHREAD_MUTEX_INITIALIZER;
static __thread int	     irq_depth;
static __thread struct host_task *self;

unsigned long host_irq_save(void)
{
	if (irq_depth++ == 0) {
		pthread_mutex_lock(&big_lock);
		/*
		 * Whoever holds the lock IS the running task, which is what
		 * runqueue.curr means on the board.  Setting it here (rather
		 * than in a fake context switch) keeps every kernel read of
		 * runqueue.curr correct, because every such read happens with
		 * the mask held.
		 */
		if (self)
			runqueue.curr = &self->t;
	}
	return 0;
}

/*
 * Dropping the mask is the ONLY moment another context can get in.  Every
 * ordering bug in this code therefore lives in the instructions right after an
 * unmask - and on real hardware that window is a handful of cycles wide, so it
 * is hit once in millions of runs.  A harness that just runs the code fast
 * reproduces that same near-impossibility and reports PASS on broken code.
 *
 * So force the window open: hand the CPU away at every unmask.  This does not
 * create interleavings the board cannot have - it only makes the ones it CAN
 * have happen every time instead of almost never.
 *
 * Measured, by injecting one real bug - moving the ring update outside the
 * masked region in msgq_send():
 *
 *      no yield at all      5/5 runs said PASS   (bug invisible)
 *      sched_yield()        2/3 runs caught it   (flaky)
 *      1 us nanosleep       6/6 runs caught it   (and 6/6 clean runs pass)
 *
 * sched_yield() only offers the CPU to a thread already runnable on this core;
 * a peer still blocked on the mutex is often not ready in time.  A short sleep
 * actually parks us, which reliably lets the peer through the window.
 *
 * Cost of doing this: the whole 2000-message run takes 0.28 s.
 */
void host_irq_restore(unsigned long flags)
{
	(void)flags;

	if (irq_depth == 0) {
		fprintf(stderr, "FAIL: local_irq_restore without a matching save\n");
		abort();
	}
	if (--irq_depth == 0) {
		pthread_mutex_unlock(&big_lock);

		struct timespec t = { .tv_sec = 0, .tv_nsec = 1000 };

		nanosleep(&t, NULL);
	}
}

/* ------------------------------------------------------------------ */
/* Task table                                                          */
/* ------------------------------------------------------------------ */

static struct host_task	tasks[HOST_MAX_TASKS];
static unsigned int	nr_tasks;

static struct host_task *to_host(struct task_struct *t)
{
	for (unsigned int i = 0; i < nr_tasks; i++)
		if (&tasks[i].t == t)
			return &tasks[i];

	fprintf(stderr, "FAIL: task %p is not in the host task table\n", (void *)t);
	abort();
}

struct host_task *host_task_self(void)
{
	return self;
}

/* ------------------------------------------------------------------ */
/* The parts of the scheduler that wait.c actually calls               */
/* ------------------------------------------------------------------ */

void place_entity(struct rq *rq, struct task_struct *p, int initial)
{
	(void)rq; (void)p; (void)initial;	/* vruntime is fair_sim's job */
}

struct task_struct *pick_next_task(struct rq *rq)
{
	(void)rq;
	return NULL;				/* the host OS picks */
}

void enqueue_task(struct rq *rq, struct task_struct *p)
{
	if (p->rt.on_rq)			/* idempotent, like fair.c */
		return;

	p->rt.on_rq = 1;
	rq->nr_running++;
	pthread_cond_signal(&to_host(p)->cond);
}

/**
 * __schedule - block until somebody makes us runnable again
 *
 * Called with the mask held, and returns with it held - the board contract.
 *
 * The loop (not an if) is deliberate and load-bearing: pthread_cond_wait may
 * return spuriously, and more importantly a waker can hand us a wakeup that a
 * third task consumes the resource for before we get the lock back.  Re-testing
 * __state on every pass is the same reason msgq_send/msgq_recv wrap their
 * condition in while() rather than if().
 */
void __schedule(void)
{
	struct task_struct *cur = runqueue.curr;
	struct host_task *h = to_host(cur);

	while (cur->__state != TASK_RUNNING)
		pthread_cond_wait(&h->cond, &big_lock);

	/* Running again: re-assert who we are, since another task held the
	 * lock (and so owned runqueue.curr) while we were blocked. */
	runqueue.curr = cur;
	cur->rt.on_rq = 0;
}

/* ------------------------------------------------------------------ */
/* Bring-up                                                            */
/* ------------------------------------------------------------------ */

struct host_task *host_task_create(const char *name)
{
	if (nr_tasks == HOST_MAX_TASKS) {
		fprintf(stderr, "FAIL: HOST_MAX_TASKS too small\n");
		abort();
	}

	struct host_task *h = &tasks[nr_tasks++];

	memset(h, 0, sizeof(*h));
	pthread_cond_init(&h->cond, NULL);

	h->t.__state = TASK_RUNNING;
	h->t.flags   = 0;
	h->t.pid     = (int)nr_tasks;
	h->t.rt.on_rq = 0;
	list_init(&h->t.wait_node);
	list_init(&h->t.rt.run_list);
	snprintf(h->t.comm, sizeof(h->t.comm), "%s", name);

	return h;
}

struct host_run_arg {
	struct host_task *h;
	void (*fn)(void *);
	void *arg;
};

static void *host_trampoline(void *p)
{
	struct host_run_arg *a = p;

	self = a->h;
	a->fn(a->arg);
	free(a);
	return NULL;
}

void host_task_run(struct host_task *h, void (*fn)(void *), void *arg)
{
	struct host_run_arg *a = malloc(sizeof(*a));

	a->h = h;
	a->fn = fn;
	a->arg = arg;
	pthread_create(&h->thread, NULL, host_trampoline, a);
}

void host_task_join(struct host_task *h)
{
	pthread_join(h->thread, NULL);
}
