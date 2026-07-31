/*
 * kernel/sched/fair.c - CFS-lite fair scheduler (virtual-time)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Every task carries a vruntime (virtual runtime). The rule is simple: always
 * run the task with the SMALLEST vruntime. Fairness and interactivity both fall
 * out of that one invariant.
 *
 * This IS the scheduler: __schedule() calls pick_next_task() below on every
 * switch. The algorithm was validated on the host by tools/test/fair_sim.c
 * before being wired in, and that simulation still runs as part of `make test`.
 *
 * No load weights: a task's vruntime advances by real run-time directly, so a
 * multi-app desktop shares the CPU evenly with no priority classes to juggle.
 */

#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/rbtree.h>
#include <nothan/time.h>

/* Tunables (ns). One "period" is the window every runnable task runs once in. */
#define SCHED_LATENCY_NS	24000000ULL	/* target latency / scheduling period */
#define SCHED_MIN_GRAN_NS	 3000000ULL	/* floor on a slice — avoid switch storm */
#define SCHED_NR_LATENCY	8		/* period stretches past this many tasks */

/* Wrap-safe order: is @a strictly before @b in virtual time? */
static inline int vruntime_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

static inline struct task_struct *task_of(struct sched_rt_entity *se)
{
	return container_of(se, struct task_struct, rt);
}

/* rbtree ordering: by vruntime (ties resolve to the right — insertion order). */
static int se_cmp(const struct rbnode *a, const struct rbnode *b)
{
	struct sched_rt_entity *ea = rb_entry(a, struct sched_rt_entity, run_node);
	struct sched_rt_entity *eb = rb_entry(b, struct sched_rt_entity, run_node);

	if (vruntime_before(ea->vruntime, eb->vruntime))
		return -1;
	if (vruntime_before(eb->vruntime, ea->vruntime))
		return 1;
	return 0;
}

/*
 * min_vruntime — a monotonically increasing floor tracking the smallest
 * vruntime in play (the running task and the leftmost queued task). Used to
 * place waking / new tasks so they can't sit arbitrarily far in the past.
 */
static void update_min_vruntime(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct rbnode *leftmost = rb_first(&rq->tasks_timeline);
	u64 v = rq->min_vruntime;
	int have = 0;

	if (curr && curr->__state == TASK_RUNNING) {
		v = curr->rt.vruntime;
		have = 1;
	}
	if (leftmost) {
		u64 lv = rb_entry(leftmost, struct sched_rt_entity, run_node)->vruntime;
		v = have ? (vruntime_before(lv, v) ? lv : v) : lv;
	}
	if (vruntime_before(rq->min_vruntime, v))
		rq->min_vruntime = v;		/* never goes backwards */
}

void enqueue_task(struct rq *rq, struct task_struct *p)
{
	if (p->rt.on_rq)
		return;
	rb_insert(&rq->tasks_timeline, &p->rt.run_node, se_cmp);
	p->rt.on_rq = 1;
	rq->nr_running++;
}

void dequeue_task(struct rq *rq, struct task_struct *p)
{
	if (!p->rt.on_rq)
		return;
	rb_erase(&rq->tasks_timeline, &p->rt.run_node);
	p->rt.on_rq = 0;
	rq->nr_running--;
}

/*
 * Pick the next task: the leftmost node (smallest vruntime), removed from the
 * tree. The running task lives OFF the tree — same contract as the old rt.c,
 * so __schedule() and every caller need no change. Returns NULL when nothing is
 * runnable (the caller then runs the idle task).
 */
struct task_struct *pick_next_task(struct rq *rq)
{
	struct rbnode *leftmost = rb_first(&rq->tasks_timeline);
	struct task_struct *p;

	if (!leftmost)
		return NULL;
	p = task_of(rb_entry(leftmost, struct sched_rt_entity, run_node));
	rb_erase(&rq->tasks_timeline, &p->rt.run_node);
	p->rt.on_rq = 0;
	rq->nr_running--;
	return p;
}

/* Charge the running task for the CPU time it has used since it was picked. */
void update_curr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	u64 now, delta;

	if (!curr)
		return;

	now = sched_clock();
	delta = now - curr->rt.exec_start;	/* wrap-safe over a run (< 1 slice) */
	curr->rt.exec_start = now;
	curr->rt.sum_exec_runtime += delta;
	curr->rt.vruntime += delta;		/* no weight: vruntime == real time */

	update_min_vruntime(rq);
}

/*
 * place_entity — set a waking (initial=0) or newly-created (initial=1) task's
 * vruntime. A task that slept has a stale, tiny vruntime; left as-is it would
 * monopolise the CPU until it "caught up". Clamp it to min_vruntime, minus one
 * period for a waker so an interactive task still gets a small head start.
 */
void place_entity(struct rq *rq, struct task_struct *p, int initial)
{
	u64 v = rq->min_vruntime;

	if (!initial)
		v -= SCHED_LATENCY_NS;

	if (vruntime_before(p->rt.vruntime, v))
		p->rt.vruntime = v;		/* == max(vruntime, floor) */
}

/* A task's share of one scheduling period: period / nr_running, floored. */
u64 sched_slice(struct rq *rq)
{
	unsigned int n = rq->nr_running + 1;	/* queued tasks + the running one */
	u64 period = (n > SCHED_NR_LATENCY) ? (SCHED_MIN_GRAN_NS * n)
					    : SCHED_LATENCY_NS;
	return period / n;
}
