/*
 * kernel/sched/core.c - Core scheduler: runqueue, schedule(), and tick
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <asm/irqflags.h>
#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/printk.h>
#include <nothan/config.h>
#include <nothan/timer.h>
#include <nothan/panic.h>
#include <asm/task-offsets.h>

/*
 * switch_to.S reads and writes task_struct by byte offset.  Nothing in C or in
 * assembly connects the two, so the field order of this struct is part of an
 * .S file's contract — and it was broken silently for as long as kstack_base
 * and kstack_size sat where user_sp and user_lr used to be.
 *
 * These turn that into a build failure.  Adding a field ahead of user_sp now
 * stops the compiler instead of overwriting a kernel stack pointer at every
 * context switch, which is the only kind of protection worth having for a
 * mistake whose symptom appears in a different subsystem entirely.
 */
_Static_assert(__builtin_offsetof(struct task_struct, stack) == TSK_STACK,
	       "switch_to.S expects task_struct.stack at TSK_STACK");
_Static_assert(__builtin_offsetof(struct task_struct, user_sp) == TSK_USER_SP,
	       "switch_to.S expects task_struct.user_sp at TSK_USER_SP");
_Static_assert(__builtin_offsetof(struct task_struct, user_lr) == TSK_USER_LR,
	       "switch_to.S expects task_struct.user_lr at TSK_USER_LR");

#if CONFIG_RING_TEST
void ringtest_produce(void);
#endif

/*
 * PROTECTION: the interrupt mask (asm/irqflags.h), held by whoever mutates.
 *
 * Every path that adds to or removes from the runqueue already holds it:
 * schedule() by its contract, wake_up() and complete() by their own
 * local_irq_save(), msleep_callback() by running in the tick handler,
 * sys_kill() around its scan, and the boot-time spawns in kernel_main.
 * enqueue_task()/dequeue_task() in rt.c therefore do NOT mask — they assume
 * the caller did, which keeps the mask in one place per operation instead of
 * nested inside every list edit.
 *
 * @need_resched is a single word written by the tick and read by the IRQ
 * return path in vectors.S; a 32-bit load or store is atomic on this core, so
 * it needs no more than that.
 */
struct rq runqueue;
int need_resched;
int resched_cause;
#if CONFIG_SCHED_LATENCY
u32 sched_wake_max;			/* worst wake->run seen, in 24 MHz cycles */
unsigned long sched_wake_count;		/* wakeups measured */
#endif
bool sched_running = false;

extern void __switch_to(struct task_struct *prev, struct task_struct *next);

/*
 * Deferred-free list. A task that calls do_exit() is still running on its own
 * kernel stack, so it cannot free that stack itself. do_exit() queues the
 * dying task here; the next task scheduled in frees it.
 *
 * PROTECTION: the interrupt mask, held by both sides.  reap_dead() runs inside
 * schedule() and is covered by its contract; do_exit() masks before calling
 * sched_defer_free().  The two must not overlap because they link through
 * rt.run_list, the same field the runqueue uses — a half-added node here is a
 * corrupt runqueue as well.
 */
static struct list_head dead_list;

void sched_defer_free(struct task_struct *tsk)
{
	list_add(&tsk->rt.run_list, &dead_list);
	printk("[DEAD] queued pid=%d kstack=%p\n",
	       tsk->pid, tsk->kstack_base);
}

static void reap_dead(void)
{
	struct sched_rt_entity *rt, *tmp;

	list_for_each_entry_safe(rt, tmp, &dead_list,
				 struct sched_rt_entity, run_list) {
		struct task_struct *z = container_of(rt, struct task_struct, rt);

		if (z == runqueue.curr)
			continue;	/* never free the stack we're running on */
		printk("[REAP] free pid=%d kstack=%p\n", z->pid, z->kstack_base);
		list_del(&rt->run_list);
		if (z->kstack_base)
			kfree(z->kstack_base);
		kfree(z);
	}
}

/*
 * The last SCHED_EVENTS context switches, and why each happened.
 *
 * panic_dump_tasks() answers "what state did the machine die in".  This
 * answers the question that usually matters more and that nothing could
 * answer before: how it got there.  A deadline missed, a task that never ran,
 * a rotation that did not happen — none of those leave a trace in the final
 * state, only in the sequence leading to it.
 *
 * Always on, unlike CONFIG_SCHED_LATENCY.  The difference is what each is
 * for: that one produces a number, so it is switched on, read, and switched
 * off again.  This one exists for the moment something unexpected happens,
 * and an instrument that has to be enabled before the surprise is an
 * instrument that is not there when the surprise arrives.
 *
 * The cost is one clocksource read per real context switch.  Names are copied
 * rather than pointed at: a task can be reaped and its task_struct freed long
 * before the panic that reads this, and a dump that dereferences freed memory
 * to explain a crash is not much of a witness.
 *
 * PROTECTION: the interrupt mask, held by schedule()'s contract — every write
 * happens inside it.  sched_dump_switches() reads from panic(), which has
 * already stopped everything else.
 */
#define SCHED_EVENTS		32	/* power of two: index masks, no modulo */

struct sched_event {
	u32	ts;		/* low 32 bits of the cycle counter at the switch */
	u32	ran;		/* cycles @from had held the CPU */
	int	from_pid;
	int	to_pid;
	short	from_prio;
	short	to_prio;
	u8	reason;
	u8	baseline;	/* @ran is not a measurement, see schedule() */
	char	from[16];
	char	to[16];
};

static struct sched_event sched_events[SCHED_EVENTS];
static unsigned int sched_event_next;	/* total switches; index = & (N-1) */

static const char *resched_reason_name(int cause)
{
	switch (cause) {
	case RESCHED_TICK:	return "tick";
	case RESCHED_WAKEUP:	return "wakeup";
	case RESCHED_BLOCK:	return "block";
	case RESCHED_EXIT:	return "exit";
	case RESCHED_VOLUNTARY:	return "yield";
	default:		return "?";
	}
}

static void sched_copy_comm(char *dst, const char *src)
{
	unsigned int i = 0;

	for (; i < 15 && src[i]; i++)
		dst[i] = src[i];
	dst[i] = '\0';
}

static void sched_record_switch(struct task_struct *from,
				struct task_struct *to,
				u32 ts, u32 ran, int reason, int baseline)
{
	struct sched_event *e = &sched_events[sched_event_next & (SCHED_EVENTS - 1)];

	e->ts        = ts;
	e->ran       = ran;
	e->from_pid  = from->pid;
	e->to_pid    = to->pid;
	e->from_prio = (short)from->prio;
	e->to_prio   = (short)to->prio;
	e->reason    = (u8)reason;
	e->baseline  = (u8)baseline;
	sched_copy_comm(e->from, from->comm);
	sched_copy_comm(e->to, to->comm);

	sched_event_next++;
}

void sched_dump_switches(void)
{
	unsigned int have = sched_event_next < SCHED_EVENTS
			  ? sched_event_next : SCHED_EVENTS;
	unsigned int i;

	if (!have) {
		printk("  switches: none yet\n");
		return;
	}

	printk("  last %u of %u switches (oldest first):\n",
	       have, sched_event_next);

	/*
	 * Oldest first, so the dump reads in the direction things happened.
	 * A cause is easier to spot at the top of a story than at the bottom.
	 */
	for (i = sched_event_next - have; i < sched_event_next; i++) {
		struct sched_event *e = &sched_events[i & (SCHED_EVENTS - 1)];

		/*
		 * Right-aligned width only: this printk understands a width and
		 * zero-padding but not the '-' flag, and an unsupported flag
		 * lands in the default arm rather than being ignored.
		 */
		if (e->baseline) {
			/*
			 * The first switch has nothing to measure against, so
			 * say that instead of printing a zero.  A zero here
			 * would read as "idle ran for no time at all", which is
			 * false in a way that invites chasing.
			 */
			printk("    %6s  %d:%s(p%d) -> %d:%s(p%d)"
			       "  ran ? (clock baseline)\n",
			       resched_reason_name(e->reason),
			       e->from_pid, e->from, e->from_prio,
			       e->to_pid, e->to, e->to_prio);
			continue;
		}

		printk("    %6s  %d:%s(p%d) -> %d:%s(p%d)  ran %lu us\n",
		       resched_reason_name(e->reason),
		       e->from_pid, e->from, e->from_prio,
		       e->to_pid, e->to, e->to_prio,
		       (unsigned long)cycles_to_us(e->ran));
	}
}

/*
 * Tasks that have died and not yet had their kernel stack reclaimed.
 *
 * Printed by panic() for two reasons.  The plain one: a task that died and was
 * never reaped is worth seeing in a post-mortem, and until now the dump said
 * nothing about the dead at all.
 *
 * The immediate one: sched_defer_free() prints "[DEAD] queued" and that line
 * has gone missing from the log, while the format string is present in the
 * image and the console reports no dropped bytes.  Since the ring is a FIFO,
 * text that entered it before other text cannot come out after — so those
 * bytes never entered it.  Asking whether the *list* has the task on it
 * answers whether the function ran at all, using a data structure instead of
 * another printk that might vanish the same way.
 */
void sched_dump_dead(void)
{
	struct list_head *pos;
	unsigned int n = 0;

	list_for_each(pos, &dead_list) {
		struct sched_rt_entity *rt =
			list_entry(pos, struct sched_rt_entity, run_list);
		struct task_struct *z = container_of(rt, struct task_struct, rt);

		printk("    dead: pid=%d \"%s\" kstack=%p\n",
		       z->pid, z->comm, z->kstack_base);
		n++;
	}

	if (!n)
		printk("  dead list: empty\n");
	else
		printk("  dead list: %u task(s) awaiting reap\n", n);
}

/*
 * Owner of each deadline-band priority, or NULL if free.
 *
 * PROTECTION: none needed.  Every entry is written once, during boot, from the
 * task-creation path with interrupts already masked, and read only by panic
 * paths afterwards.  Nothing claims a priority once the machine is running —
 * the app set is fixed at build time, which is exactly what makes a table this
 * simple sufficient.
 */
static const char *prio_owner[MAX_PRIO];

const char *prio_band_name(int prio)
{
	if (prio >= IDLE_PRIO)
		return "IDLE";
	if (prio >= PRIO_BG)
		return "BG";
	if (prio >= PRIO_UI)
		return "UI";
	if (prio >= PRIO_VIDEO)
		return "VIDEO";
	if (prio >= PRIO_NET)
		return "NET";
	return "AUDIO";
}

void sched_claim_prio(int prio, const char *name)
{
	if (prio < 0 || prio >= MAX_PRIO)
		panic("prio %d out of range for '%s'", prio, name);

	/*
	 * The shared band is shared on purpose — that is the whole difference
	 * between it and the deadline bands, so there is nothing to claim.
	 */
	if (prio_is_shared(prio))
		return;

	if (prio_owner[prio])
		panic("prio %d (%s) wanted by '%s', already held by '%s'",
		      prio, prio_band_name(prio), name, prio_owner[prio]);

	prio_owner[prio] = name;
}

void task_stack_arm(struct task_struct *p)
{
	u32 *guard = (u32 *)p->kstack_base;
	unsigned int i;

	if (!guard)
		return;

	for (i = 0; i < KSTACK_CANARY_WORDS; i++)
		guard[i] = KSTACK_CANARY;
}

void task_stack_check(struct task_struct *p)
{
	const u32 *guard = (const u32 *)p->kstack_base;
	unsigned int i;

	if (!guard)
		return;

	/*
	 * Check the pointer before following it.
	 *
	 * A diagnostic that faults is worse than no diagnostic: the report
	 * names the checker instead of the fault it was built to explain, and
	 * whoever reads it starts in the wrong file.  This one did exactly
	 * that — it took a data abort inside itself on a kstack_base holding
	 * 0x402f5770, and the dump described task_stack_check rather than
	 * whatever had put that value there.
	 *
	 * Every kernel stack is either inside the image (idle's is static) or
	 * from kmalloc, which returns direct-map addresses at or above
	 * PAGE_OFFSET.  Anything below that cannot be a stack, so say so with
	 * the task's name attached rather than dereferencing it to find out.
	 */
	if ((unsigned long)guard < PAGE_OFFSET)
		panic("task \"%s\" (pid=%d) has a corrupt kstack_base %p"
		      " — not a kernel address", p->comm, p->pid, guard);

	for (i = 0; i < KSTACK_CANARY_WORDS; i++) {
		if (guard[i] == KSTACK_CANARY)
			continue;

		/*
		 * Report the task that owns this stack, not whatever is running
		 * now — they are the same for the outgoing task and different
		 * for the incoming one, and the difference is the whole point:
		 * a corrupted guard on a task that was asleep means somebody
		 * else wrote through it.
		 */
		panic("kernel stack overflow: pid=%d \"%s\" stack %p..%p,"
		      " guard word %u = 0x%08lx",
		      p->pid, p->comm, p->kstack_base,
		      (char *)p->kstack_base + p->kstack_size,
		      i, (unsigned long)guard[i]);
	}
}

/* Idle task — always runnable, lowest priority, no kmalloc needed. */
#define IDLE_STACK_WORDS 256
static unsigned long idle_stack[IDLE_STACK_WORDS];
static struct task_struct idle_tsk;

/*
 * kernel_main runs in the idle task's context and hands over here once the
 * initial tasks exist — see the call at the end of kernel_main.  It must,
 * because the first schedule() saves kernel_main's own context into idle_tsk,
 * overwriting the entry frame built below: when idle is next picked it resumes
 * wherever kernel_main left off, not at the top of this function.  While
 * nothing ever slept that never happened; now that reads block, idle is picked
 * constantly, and landing back in kernel_main's trailing loop with interrupts
 * masked would stop the tick and wedge the machine.
 *
 * The idle task owns the global interrupt state outright — nobody is waiting
 * on a critical section it holds — so the unconditional forms are the right
 * ones here.  WFI needs interrupts enabled to be woken by one; schedule()
 * needs them masked.
 */
void cpu_idle(void)
{
	while (1) {
		__asm__ __volatile__ ("cpsie i\nwfi" : : : "memory");
		local_irq_disable();
		schedule();
	}
}

static void idle_task_init(void)
{
	unsigned long *sp = idle_stack + IDLE_STACK_WORDS;

	/* Pre-fill the switch frame (see spawn.c for layout): */
	*--sp = (unsigned long)cpu_idle;	/* lr → PC */
	*--sp = 0;				/* r11 */
	*--sp = 0;				/* r10 */
	*--sp = 0;				/* r9  */
	*--sp = 0;				/* r8  */
	*--sp = 0;				/* r7  */
	*--sp = 0;				/* r6  */
	*--sp = (unsigned long)cpu_idle;	/* r5 (fallback exit) */
	*--sp = (unsigned long)cpu_idle;	/* r4 (fn) */

		idle_tsk.stack      = sp;
		/*
		 * Idle's stack is static, but recording it here is what lets the
		 * guard below cover idle too, and what lets a fault dump bound
		 * idle's stack the way it bounds everyone else's.  The reaper's
		 * kfree() of kstack_base is unreachable for this task: cpu_idle
		 * never returns, so idle never reaches do_exit.
		 */
		idle_tsk.kstack_base = idle_stack;
		idle_tsk.kstack_size = sizeof(idle_stack);
		idle_tsk.user_sp    = 0;
		idle_tsk.user_lr    = 0;
		idle_tsk.__state    = TASK_RUNNING;
		idle_tsk.pid        = 0;
		idle_tsk.prio       = IDLE_PRIO;
		idle_tsk.rt.time_slice = BG_TIMESLICE;
		idle_tsk.rt.on_rq   = 0;
		idle_tsk.rt.ran_once = 0;
		idle_tsk.exit_code  = 0;
		idle_tsk.mm         = NULL;

	const char *name = "idle";
	unsigned int i = 0;
	for (; i < 15 && name[i]; i++)
		idle_tsk.comm[i] = name[i];
	idle_tsk.comm[i] = '\0';

	sched_claim_prio(IDLE_PRIO, idle_tsk.comm);
	task_stack_arm(&idle_tsk);
	enqueue_task(&runqueue, &idle_tsk);
		runqueue.curr = &idle_tsk;
}

/**
 * sched_init() - Initialize the scheduler runqueue
 */
void sched_init(void)
{
	struct rq *rq = &runqueue;

	rq->active.bitmap = 0;
	rq->nr_running = 0;
	rq->curr = NULL;

	for (unsigned int i = 0; i < MAX_PRIO; i++)
		list_init(&rq->active.queue[i]);

	need_resched = 0;

	list_init(&dead_list);

	idle_task_init();

	/*
	 * Bootstrap: set curr to idle_tsk so that any code running between
	 * sched_init() and the first schedule() sees a valid current task.
	 * Without this, timer ISR → scheduler_tick() → runqueue.curr->field
	 * dereferences NULL and causes a data abort.
	 */
	rq->curr = &idle_tsk;

	printk("[SCHED] bands AUDIO=%d NET=%d VIDEO=%d UI=%d BG=%d-%d IDLE=%d\n",
	       PRIO_AUDIO, PRIO_NET, PRIO_VIDEO, PRIO_UI,
	       PRIO_BG, PRIO_BG_LAST, IDLE_PRIO);
	printk("[SCHED] deadline bands strict + exclusive; BG shares one level,"
	       " %d ms slice (%d tick(s) of %d ms)\n",
	       BG_TIMESLICE_MS, BG_TIMESLICE, TICK_MS);

	/*
	 * Printed so a later complaint about this pointer can be told apart
	 * from a pointer that was never right: if the value here is sane and
	 * the value at the fault is not, something overwrote it, and that is a
	 * different search from a field that was never assigned.
	 */
	printk("[SCHED] idle kstack %p..%p, guard 0x%08lx\n",
	       idle_tsk.kstack_base,
	       (char *)idle_tsk.kstack_base + idle_tsk.kstack_size,
	       (unsigned long)KSTACK_CANARY);
}

/**
 * schedule() - Pick the next task and switch to it
 *
 * CONTRACT: enter with interrupts masked, return with interrupts masked.
 *
 * schedule() never changes the interrupt mask.  That is what lets a caller
 * make "decide to sleep, record it, give up the CPU" one atomic step:
 *
 *	flags = local_irq_save();
 *	set_current_state(TASK_UNINTERRUPTIBLE);
 *	list_add_tail(&curr->rt.run_list, &wq->task_list);
 *	schedule();
 *	local_irq_restore(flags);
 *
 * The previous version masked on entry and unmasked on every exit, which
 * meant a caller could not hold a critical section across it: the window
 * between marking a task as sleeping and actually sleeping was open to the
 * tick and to any wake_up() from an ISR, and both touch the same rt.run_list
 * the caller was mid-way through linking.
 *
 * A task resumed later by another __switch_to() returns from here with
 * interrupts still masked, exactly as it left them, and its own caller
 * restores its own saved flags.  Freshly created tasks do not come back
 * through here at all — they enter at task_entry, which enables interrupts
 * itself.
 *
 * The idle task is always on the runqueue, so pick_next_task() never returns
 * NULL.
 */
void schedule(void)
{
	reap_dead();

	struct task_struct *prev = runqueue.curr;

	/*
	 * Both ends of the switch, and they catch different things.
	 *
	 * The outgoing task is checked because this is the first moment after
	 * it stopped running, so an overflow it caused is reported against it
	 * rather than against whatever later trips over the damage.  The
	 * incoming one is checked because its guard was intact when it went to
	 * sleep: if it is broken now, something else wrote through a stack that
	 * was not running — cross-owner corruption, the case §1 says a log
	 * cannot chase, and the one worth paying two loads a switch to catch.
	 */
	if (prev)
		task_stack_check(prev);

	if (prev && prev->__state == TASK_RUNNING)
		enqueue_task(&runqueue, prev);

	struct task_struct *next = pick_next_task(&runqueue);
	if (!next) {
		/* Should never happen — idle task is always available. */
		runqueue.curr = NULL;
		return;
	}

	task_stack_check(next);

#if CONFIG_SCHED_LATENCY
	/*
	 * Close the interval opened in check_preempt_curr().  Only a task that
	 * was woken carries a stamp; one merely rotated by the tick does not,
	 * so this measures waking latency and not time-slice sharing.
	 *
	 * Reporting only on a new maximum keeps this quiet: the worst case is
	 * what matters for a deadline, and after warm-up the line stops
	 * appearing, which is itself the signal that the figure has settled.
	 */
	if (next->rt.wake_ts) {
		u32 d = (u32)(timer_cycles() - next->rt.wake_ts);

		next->rt.wake_ts = 0;
		sched_wake_count++;

		int worse = (d > sched_wake_max);

		if (worse)
			sched_wake_max = d;

		/*
		 * On a new maximum, and every eighth wakeup regardless.
		 *
		 * The maximum is the number that matters for a deadline, and
		 * reporting it as it grows means the very first wakeup produces
		 * a line — with the previous threshold of 32, hand-typing never
		 * reached it and the output was indistinguishable from a
		 * measurement that had quietly stopped working.
		 *
		 * The periodic line stays because those two silences must not
		 * look alike: one says the system is calm, the other says the
		 * instrument is broken.
		 */
		/*
		 * Cycles, not microseconds, and the pid with them.
		 *
		 * The first version of this line reported only microseconds and
		 * dropped the pid, and both omissions cost a measurement round.
		 * Integer division by 24 turns everything under a microsecond
		 * into "0 us" — which is exactly the range a working
		 * preempt-on-wakeup lands in, so the interesting answer was
		 * being rounded away.  And without the pid, two outliers at boot
		 * could not be attributed to a task at all.
		 */
		if (worse || (sched_wake_count & 7u) == 0)
			printk("[SCHED] wake->run %lu cyc, max %lu cyc (%lu us)"
			       " pid=%d n=%lu\n",
			       (unsigned long)d,
			       (unsigned long)sched_wake_max,
			       (unsigned long)cycles_to_us(sched_wake_max),
			       next->pid, sched_wake_count);
	}
#endif

	/*
	 * Accounting, on real switches only.
	 *
	 * schedule() is often reached with prev == next: nothing more urgent was
	 * ready, so the same task carries on.  Nothing happened there worth
	 * recording, and leaving switch_ts alone is also what keeps the numbers
	 * right — prev goes on running and the whole span lands on its account
	 * at whatever switch finally does take the CPU away.
	 *
	 * The reason is decided here rather than trusted from resched_cause
	 * alone, because the two can disagree honestly: the tick can ask for a
	 * reschedule and then the task blocks before the request is serviced.
	 * What prev is doing now outranks what something wanted a moment ago.
	 * Exit is the one case state cannot show, since do_exit() parks a dying
	 * task in TASK_UNINTERRUPTIBLE exactly like a sleeping one — so it says
	 * so explicitly, and that is checked first.
	 */
	if (prev && prev != next) {
		u64 now = timer_cycles();
		u32 ran;
		int reason;

		/*
		 * Establish the baseline on the first switch instead of trusting
		 * a zero.
		 *
		 * switch_ts cannot be set in sched_init(): the clocksource is
		 * brought up by an initcall, which runs later, so a stamp taken
		 * there would be meaningless.  Left at zero, the first switch
		 * would compute "now minus the beginning of time" and hand the
		 * idle task a run of several seconds — which would then be its
		 * maximum forever, and every genuine sample afterwards would be
		 * measured and never reported.
		 *
		 * That is the same trap as the uninitialised wake_ts in
		 * kernel-roadmap.md §5.1.1, and it is worth noticing that it
		 * came back in a different field: an instrument that starts
		 * measuring before it has a zero point produces numbers that
		 * look real.
		 */
		int baseline = !runqueue.switch_ts;

		if (baseline)
			runqueue.switch_ts = now;

		ran = (u32)(now - runqueue.switch_ts);

		if (resched_cause == RESCHED_EXIT)
			reason = RESCHED_EXIT;
		else if (prev->__state != TASK_RUNNING)
			reason = RESCHED_BLOCK;
		else if (resched_cause != RESCHED_NONE)
			reason = resched_cause;
		else
			reason = RESCHED_VOLUNTARY;

		/*
		 * Fold cycles into microseconds keeping the remainder, so a task
		 * switched thousands of times does not lose most of its runtime
		 * to truncation one switch at a time.  Split this way the
		 * intermediate cannot overflow: both remainders stay under 24.
		 */
		u32 rem = prev->cpu_cyc_rem + (ran % TSC_CYCLES_PER_US);

		prev->cpu_us     += ran / TSC_CYCLES_PER_US + rem / TSC_CYCLES_PER_US;
		prev->cpu_cyc_rem = rem % TSC_CYCLES_PER_US;

		if (ran > prev->max_run_cyc)
			prev->max_run_cyc = ran;

		next->nr_picked++;
		sched_record_switch(prev, next, (u32)now, ran, reason, baseline);
		runqueue.switch_ts = now;
	}

	runqueue.curr = next;
	next->rt.ran_once = 1;
	need_resched = 0;
	resched_cause = RESCHED_NONE;

	/*
	 * Interrupts stay masked across __switch_to: a timer IRQ landing
	 * between "ldr sp, [next]" and "ldmfd ... pc" inside __switch_to would
	 * corrupt the task stack mid-switch.  Under the contract above the
	 * caller already masked them, so there is nothing to do here — which
	 * is the point.  Whoever masked is the one who unmasks.
	 */
	if (prev == next)
		return;

	sched_running = true;

	if (prev) {
		/* Set up user mapping BEFORE context switch — __switch_to to a
		 * new user task branches to user_task_trampoline directly and
		 * never returns, so mmu_switch_mm after it would be skipped. */
		if (next->mm)
			mmu_switch_mm(next->mm);
		__switch_to(prev, next);
	} else {
		if (next->mm)
			mmu_switch_mm(next->mm);
		__asm__ __volatile__ (
			"cps #0x1f\n"
			"ldr sp, [%0, #4]\n"
			"ldr lr, [%0, #8]\n"
			"cps #0x13\n"
			"ldr sp, [%0, #0]\n"
			"ldmfd sp!, {r4-r11, pc}\n"
			: : "r" (next));
	}

	/*
	 * Reached only when prev is resumed by a later __switch_to, with
	 * interrupts masked exactly as they were when it gave up the CPU.
	 * Its own caller restores its own flags.
	 */
}

/*
 * Preemptive scheduling: the timer tick rotates the running task once its
 * timeslice is spent. (Was toggled to 0 during a 2026-06 A/B test; the
 * project has since chosen real preemptive multitasking so background tasks
 * can run alongside the GUI without it having to yield() cooperatively.)
 */
#define SCHED_PREEMPT  1

/*
 * The tick rotates the shared band and nothing else.
 *
 * The previous version rotated whatever was running, which is the right rule
 * when every task is equal and the wrong one here.  A task in a deadline band
 * owns its level alone, so rotating it can only hand the CPU to something less
 * urgent while its own deadline is approaching — and it buys nothing, because
 * there is no equal waiting behind it to be fair to.  Those tasks give up the
 * CPU by blocking, or lose it to a wakeup at a better priority; both paths run
 * through check_preempt_curr() already.
 *
 * Rotation exists for the shared band, where several applications sit on one
 * level with no deadline between them.  There, taking turns is the only thing
 * standing between Word and never running while Excel is busy.
 */
void scheduler_tick(void)
{
	struct task_struct *curr = runqueue.curr;

#if CONFIG_RING_TEST
	/*
	 * The ring test's producer.  Here because this is genuine interrupt
	 * context and the point of the test is the hand-off across that
	 * boundary — calling it from a task would test nothing that matters.
	 */
	ringtest_produce();
#endif

	if (!curr)
		return;

	if (!prio_is_shared(curr->prio))
		return;

	if (--curr->rt.time_slice > 0)
		return;

	curr->rt.time_slice = BG_TIMESLICE;
#if SCHED_PREEMPT
	set_need_resched(RESCHED_TICK);
#endif
}
