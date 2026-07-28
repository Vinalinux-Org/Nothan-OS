/*
 * tools/test/msgq_test.c - run kernel/ipc/msgq.c unmodified, off-target
 *
 * WHAT IS ACTUALLY UNDER TEST
 *
 * kernel/ipc/msgq.c and kernel/sched/wait.c are compiled here from the SAME
 * sources the board builds - not a copy, not a rewrite.  The harness only
 * supplies the scheduler underneath (tools/test/host_sched.c).
 *
 * WHY THIS EXISTS
 *
 * Three failures in this code are invisible over UART, which is the only
 * debugging tool the board has:
 *
 *   1. LOST WAKEUP.  A waker fires in the window between "the sleeper decided
 *      the condition is false" and "the sleeper is both marked non-running and
 *      present on the wait queue".  The wakeup lands on nobody; the sleeper
 *      never runs again.  Symptom on the board: silence.  Not a crash, not a
 *      log line - a machine that simply stops.  Here it is a timeout.
 *
 *   2. LOST OR DUPLICATED MESSAGE.  q->head/q->tail/q->used are updated across
 *      a blocking point.  If the ring is touched outside the region that last
 *      tested the condition, two tasks can claim the same slot.  Symptom on the
 *      board: an app misbehaves minutes later, far from the cause.
 *
 *   3. UNBALANCED MASKING.  A path that returns without restoring the IRQ
 *      state leaves the whole kernel running masked.  host_irq_restore()
 *      aborts on an unmatched restore, and the run would deadlock on an
 *      unmatched save.
 *
 * SHAPE OF THE TEST
 *
 * A queue with ONE slot and several producers and consumers: the narrowest
 * ring maximises how often both sides have to block, which is the only
 * interesting path.  Every message carries {producer, seq}, so the checker can
 * tell "lost" from "duplicated" instead of only noticing the totals disagree.
 *
 * A watchdog turns hangs into failures.  Without it, a lost wakeup would just
 * make the test hang forever - reporting nothing, like the board does.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nothan/msgq.h>
#include <nothan/sched.h>
#include "host_sched.h"

#define NR_PRODUCERS	4
#define NR_CONSUMERS	3
#define PER_PRODUCER	500
#define QUEUE_SLOTS	1		/* narrowest ring: force blocking */

#define TOTAL_MSGS	(NR_PRODUCERS * PER_PRODUCER)
#define WATCHDOG_SECS	15

struct msg {
	int producer;
	int seq;
};

static struct msgq	q;
static char		qbuf[QUEUE_SLOTS * sizeof(struct msg)];

/* seen[p][s] - how many times message {p,s} came out. Must end as all 1s. */
static unsigned char	seen[NR_PRODUCERS][PER_PRODUCER];
static pthread_mutex_t	seen_lock = PTHREAD_MUTEX_INITIALIZER;

static int		received;	/* under seen_lock */
static volatile int	finished;
static int		consumers_done;	/* atomic */

static void producer(void *arg)
{
	int id = (int)(long)arg;

	for (int s = 0; s < PER_PRODUCER; s++) {
		struct msg m = { .producer = id, .seq = s };

		msgq_send(&q, &m);
	}
}

/*
 * Consumers run until killed, exactly the way a real daemon does.  There is no
 * "stop" message: msgq_recv() blocks forever on an empty queue, so the only
 * way out is the kill check inside it (task_should_exit -> bail without
 * touching the ring).  Shutting the test down therefore exercises the
 * kill-a-blocked-task path for free, which is real kernel logic and not
 * otherwise covered.
 */
static void consumer(void *arg)
{
	struct host_task *me = arg;

	while (!task_should_exit(&me->t)) {
		/* Sentinel: msgq_recv() returns WITHOUT writing *out when it
		 * bails on a kill, so a never-touched buffer is how we tell
		 * "killed" from "got a message". */
		struct msg m = { .producer = -1, .seq = -1 };

		msgq_recv(&q, &m);

		if (m.producer == -1)
			break;			/* killed while blocked */

		if (m.producer < 0 || m.producer >= NR_PRODUCERS ||
		    m.seq < 0 || m.seq >= PER_PRODUCER) {
			fprintf(stderr,
				"FAIL: garbage message {producer=%d, seq=%d} - "
				"ring index torn\n", m.producer, m.seq);
			_exit(1);
		}

		pthread_mutex_lock(&seen_lock);
		seen[m.producer][m.seq]++;
		received++;
		pthread_mutex_unlock(&seen_lock);
	}

	__atomic_fetch_add(&consumers_done, 1, __ATOMIC_SEQ_CST);
}

static void *watchdog(void *arg)
{
	(void)arg;

	for (int i = 0; i < WATCHDOG_SECS; i++) {
		sleep(1);
		if (finished)
			return NULL;
	}

	pthread_mutex_lock(&seen_lock);
	fprintf(stderr,
		"FAIL: stuck after %d s - %d/%d messages through, used=%u.\n"
		"      A blocked task was never woken (lost wakeup): the wake\n"
		"      landed in the window between deciding to sleep and being\n"
		"      visible on the wait queue.\n",
		WATCHDOG_SECS, received, TOTAL_MSGS, q.used);
	pthread_mutex_unlock(&seen_lock);
	_exit(1);
}

int main(void)
{
	struct host_task *prod[NR_PRODUCERS];
	struct host_task *cons[NR_CONSUMERS];
	pthread_t wd;

	msgq_init(&q, qbuf, sizeof(struct msg), QUEUE_SLOTS);

	pthread_create(&wd, NULL, watchdog, NULL);

	for (int i = 0; i < NR_CONSUMERS; i++) {
		char name[16];

		snprintf(name, sizeof(name), "cons%d", i);
		cons[i] = host_task_create(name);
	}
	for (int i = 0; i < NR_PRODUCERS; i++) {
		char name[16];

		snprintf(name, sizeof(name), "prod%d", i);
		prod[i] = host_task_create(name);
	}

	for (int i = 0; i < NR_CONSUMERS; i++)
		host_task_run(cons[i], consumer, cons[i]);
	for (int i = 0; i < NR_PRODUCERS; i++)
		host_task_run(prod[i], producer, (void *)(long)i);

	for (int i = 0; i < NR_PRODUCERS; i++)
		host_task_join(prod[i]);

	/* Producers are done; let the consumers drain what is still in flight. */
	for (;;) {
		pthread_mutex_lock(&seen_lock);
		int done = (received == TOTAL_MSGS);

		pthread_mutex_unlock(&seen_lock);
		if (done)
			break;
		usleep(1000);
	}

	/*
	 * Now kill the consumers the way sys_kill does: set the request bit,
	 * then wake_up_task() to drag them off the wait queue so they reach
	 * their kill check.  Repeat, because a consumer may be between the
	 * check and the block when we first nudge it - the same reason a real
	 * kill is a request and not an instant.
	 */
	for (int i = 0; i < NR_CONSUMERS; i++)
		cons[i]->t.flags |= TASK_SHOULD_EXIT;

	while (__atomic_load_n(&consumers_done, __ATOMIC_SEQ_CST) < NR_CONSUMERS) {
		for (int i = 0; i < NR_CONSUMERS; i++)
			wake_up_task(&cons[i]->t);
		usleep(1000);
	}

	for (int i = 0; i < NR_CONSUMERS; i++)
		host_task_join(cons[i]);

	finished = 1;

	/* ---- checks ---- */
	int lost = 0, dup = 0;

	for (int p = 0; p < NR_PRODUCERS; p++)
		for (int s = 0; s < PER_PRODUCER; s++) {
			if (seen[p][s] == 0)
				lost++;
			else if (seen[p][s] > 1)
				dup++;
		}

	if (lost || dup) {
		fprintf(stderr, "FAIL: %d lost, %d duplicated message(s)\n", lost, dup);
		return 1;
	}
	if (received != TOTAL_MSGS) {
		fprintf(stderr, "FAIL: received %d, sent %d\n", received, TOTAL_MSGS);
		return 1;
	}
	if (q.used != 0) {
		fprintf(stderr, "FAIL: queue ends with used=%u, expected 0\n", q.used);
		return 1;
	}

	printf("MSGQ TEST: PASS  (%d msgs, %dP/%dC, %d-slot ring; "
	       "no loss, no dup, no lost wakeup)\n",
	       TOTAL_MSGS, NR_PRODUCERS, NR_CONSUMERS, QUEUE_SLOTS);
	return 0;
}
