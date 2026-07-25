/*
 * tools/test/fair_sim.c - Host simulation of the CFS-lite scheduling algorithm.
 *
 * Validates the DESIGN (not board behaviour) using the real red-black tree:
 *   1. min-vruntime picking distributes CPU fairly among equal tasks,
 *   2. place_entity() stops a long-sleeping task from hogging the CPU on wake.
 *
 * The pick/accrue/place logic here mirrors what kernel/sched/fair.c implements,
 * so a green run here means the algorithm is sound before it ever runs on hw.
 *
 *   cc -I include -o /tmp/fair_sim tools/test/fair_sim.c lib/rbtree.c && /tmp/fair_sim
 */

#include <stdio.h>
#include <stdlib.h>
#include <nothan/rbtree.h>

typedef long long s64;
typedef unsigned long long u64;

#define SCHED_LATENCY   24000000ULL	/* one scheduling period (ns) */
#define SLICE            6000000ULL	/* a task's run chunk before re-pick  */

struct ent { u64 vruntime; u64 ran; int id; int queued; struct rbnode node; };

/* wrap-safe: is a strictly before b? */
static int vbefore(u64 a, u64 b) { return (s64)(a - b) < 0; }

static int cmp(const struct rbnode *a, const struct rbnode *b)
{
	struct ent *ea = rb_entry(a, struct ent, node);
	struct ent *eb = rb_entry(b, struct ent, node);
	if (vbefore(ea->vruntime, eb->vruntime)) return -1;
	if (vbefore(eb->vruntime, ea->vruntime)) return 1;
	return 0;
}

static struct rbtree rq;
static u64 min_vruntime;

static void enqueue(struct ent *e) { rb_insert(&rq, &e->node, cmp); e->queued = 1; }
static void dequeue(struct ent *e) { rb_erase(&rq, &e->node); e->queued = 0; }
static struct ent *pick(void)
{
	struct rbnode *n = rb_first(&rq);
	return n ? rb_entry(n, struct ent, node) : NULL;
}
static void update_min(struct ent *curr)
{
	u64 v = curr->vruntime;
	struct rbnode *lm = rb_first(&rq);
	if (lm) {
		struct ent *e = rb_entry(lm, struct ent, node);
		if (vbefore(e->vruntime, v)) v = e->vruntime;
	}
	if (vbefore(min_vruntime, v)) min_vruntime = v;	/* monotonic floor */
}

/* place a waking (initial=0) or new (initial=1) task's vruntime */
static void place_entity(struct ent *e, int initial)
{
	u64 v = min_vruntime;
	if (!initial)
		v -= SCHED_LATENCY;			/* let a sleeper start slightly behind */
	if (vbefore(e->vruntime, v))			/* == max(vruntime, min - L) */
		e->vruntime = v;
}

static int fails;
static void expect(int cond, const char *msg) { if (!cond) { printf("  FAIL: %s\n", msg); fails++; } }

int main(void)
{
	rb_init(&rq); min_vruntime = 0;

	/* --- Test 1: fairness among 3 always-runnable equal tasks --- */
	struct ent t[3] = {{0}};
	for (int i = 0; i < 3; i++) { t[i].id = i; enqueue(&t[i]); }
	for (int round = 0; round < 300000; round++) {
		struct ent *c = pick();
		dequeue(c);
		c->vruntime += SLICE;			/* ran a slice */
		c->ran += SLICE;
		update_min(c);
		enqueue(c);				/* still runnable */
	}
	u64 lo = t[0].ran, hi = t[0].ran;
	for (int i = 1; i < 3; i++) { if (t[i].ran < lo) lo = t[i].ran; if (t[i].ran > hi) hi = t[i].ran; }
	expect(hi - lo <= SLICE, "fairness: shares differ by more than one slice");
	for (int i = 0; i < 3; i++) dequeue(&t[i]);

	/* --- Test 2: a long sleeper must NOT hog the CPU on wake --- */
	rb_init(&rq); min_vruntime = 0;
	struct ent A = {.id=0}, B = {.id=1}, C = {.id=2};
	enqueue(&A); enqueue(&B);
	/* run A/B a long time so min_vruntime climbs high; C stays asleep at ~0 */
	for (int round = 0; round < 100000; round++) {
		struct ent *c = pick(); dequeue(c);
		c->vruntime += SLICE; c->ran += SLICE; update_min(c); enqueue(c);
	}
	u64 pack = min_vruntime;
	u64 c_stale = 20;				/* C's ancient vruntime */
	C.vruntime = c_stale;

	place_entity(&C, 0);				/* wake C */
	expect(C.vruntime > c_stale,               "wakeup: vruntime not lifted off stale value");
	expect(!vbefore(pack, C.vruntime),         "wakeup: C placed ahead of the pack (would lose priority)");
	expect(vbefore(pack - SCHED_LATENCY - 1, C.vruntime), "wakeup: C placed too far back (would hog)");
	enqueue(&C);

	/* measure how long C runs before it catches up to A/B (must be bounded by ~L) */
	u64 c_before = C.ran;
	int guard = 0;
	while (vbefore(C.vruntime, A.vruntime) && guard++ < 100000) {
		struct ent *c = pick(); dequeue(c);
		c->vruntime += SLICE; c->ran += SLICE; update_min(c); enqueue(c);
	}
	u64 c_burst = C.ran - c_before;
	expect(c_burst <= SCHED_LATENCY + SLICE, "wakeup: C ran far more than one period before yielding (hog!)");

	if (fails) { printf("FAIR SIM: %d FAILURES\n", fails); return 1; }
	printf("FAIR SIM: PASS  (fairness within 1 slice; woken task bounded to ~1 period)\n");
	return 0;
}
