#ifndef _NOTHAN_SCHED_H
#define _NOTHAN_SCHED_H

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/timer.h>	/* TICK_MS, for the timeslice below */

/* Task state constants (Linux v6.17 compatible, bitmask-style) */
#define TASK_RUNNING		0x00000000	/* running or on runqueue */
#define TASK_INTERRUPTIBLE	0x00000001	/* sleep, wakeable by signal */
#define TASK_UNINTERRUPTIBLE	0x00000002	/* sleep, no signal wakeup */
#define __TASK_STOPPED		0x00000004	/* paused (SIGSTOP) */
#define __TASK_TRACED		0x00000008	/* ptrace (gdb/strace) */

#define TASK_STOPPED		__TASK_STOPPED
#define TASK_TRACED		__TASK_TRACED

/* Flag-like modifiers (ORed with basic states): */
#define TASK_WAKEKILL		0x00000100	/* allow SIGKILL while unkillable */
#define TASK_KILLABLE		(TASK_UNINTERRUPTIBLE | TASK_WAKEKILL)

#define TASK_NEW		0x00000800	/* just spawned, not yet seen by scheduler */

/* Scheduling constants */
#define MAX_PRIO			32	/* 32 fixed priority levels */
#define IDLE_PRIO			(MAX_PRIO - 1)	/* lowest: idle task */

/*
 * Priority bands — Documentation/kernel-roadmap.md §5.2.
 *
 * Lower number is more urgent.  A band is named after the *kind of work*, not
 * after an application: the mute button belongs to UI even though it is about
 * sound, because priority means urgency x shortness, not importance.
 *
 * Two different rules apply, and mixing them up is the whole point of naming
 * the bands:
 *
 *   AUDIO/NET/VIDEO/UI  deadline bands.  One task per level, enforced at boot
 *                       (sched_claim_prio).  Strict priority: a task here runs
 *                       until it blocks or something more urgent wakes.  The
 *                       tick never rotates it — rotation would hand the CPU to
 *                       an equal just as a deadline approached.
 *
 *   BG                  no deadline, and must not starve.  Tasks share one
 *                       level and rotate on the tick.  This is where the seven
 *                       box applications live: giving Word and Excel unique
 *                       priorities would mean the lower of the two never runs
 *                       at all while the other is busy.
 */
#define PRIO_AUDIO		0	/* 0-3   audio in/out, 10 ms deadline */
#define PRIO_NET		4	/* 4-7   packet RX/TX, budgeted */
#define PRIO_VIDEO		8	/* 8-11  capture, frame assembly */
#define PRIO_UI			12	/* 12-19 compositor, input, focused app */
#define PRIO_BG			20	/* 20-27 daemons and applications, shared */
#define PRIO_BG_LAST		27

/* Priority assignment for the tasks this box actually runs. */
#define PRIO_SHELL		(PRIO_UI + 0)	/* echoing a keypress: short, urgent */
#define PRIO_GUI		(PRIO_UI + 1)	/* redraw: long, so below the shell */

/*
 * True for levels where several tasks may share a priority and take turns.
 * Everything else is a deadline level and is exclusive.
 */
static inline int prio_is_shared(int prio)
{
	return prio >= PRIO_BG && prio <= PRIO_BG_LAST;
}

/*
 * Timeslice for the shared band, expressed in milliseconds and converted to
 * ticks here.
 *
 * The old constant was "1 tick" with a comment noting that a tick was 10 ms.
 * Roadmap §5.3 takes the tick to 1 ms, which would have silently turned a
 * 10 ms timeslice into a 1 ms one — a thousand context switches a second for
 * tasks that have no deadline and no reason to react quickly.  Deriving it
 * from TICK_MS keeps the duration meaning what it says through that change.
 */
#define BG_TIMESLICE_MS		20
#define BG_TIMESLICE		(BG_TIMESLICE_MS / TICK_MS)

/**
 * struct sched_rt_entity - per-task scheduling entity
 * @run_list:   node linking this entity into rt_prio_array.queue[prio]
 * @time_slice: remaining ticks before RR rotation
 * @on_rq:      1 if currently enqueued in the runqueue
 */
struct sched_rt_entity {
	struct list_head	run_list;
	unsigned int		time_slice;
	int					on_rq;
	u64			wake_ts;	/* CONFIG_SCHED_LATENCY: when it became runnable */
	int			ran_once;	/* has the scheduler ever switched to it */
};

/**
 * struct task_struct - per-task descriptor
 * @stack:     saved kernel SP (top of saved register frame on kernel stack)
 * @__state:   TASK_RUNNING / TASK_INTERRUPTIBLE / TASK_UNINTERRUPTIBLE / ...
 * @pid:       process identifier (monotonically increasing)
 * @prio:      static priority, 0 = highest, MAX_PRIO-1 = lowest
 * @rt:        embedded scheduling entity
 * @comm:      human-readable task name (for printk debugging)
 * @mm:        NULL = kernel thread
 * @exit_code: exit status code set by do_exit()
 */
struct task_struct {
	void				*stack;
	void				*kstack_base;	/* kmalloc base of kernel stack (free on exit) */
	unsigned int			kstack_size;	/* so a fault dump can bound it */
	unsigned long			user_sp;
	unsigned long			user_lr;
	unsigned int			__state;
	int				pid;
	int				prio;
	struct sched_rt_entity		rt;
	char				comm[16];
	struct mm_struct		*mm;    /* NULL = kernel thread */
	int				exit_code;
	char				cwd[64];
};

/**
 * struct rt_prio_array - O(1) priority queue with bitmap
 * @bitmap: u32 bitmask of active priority levels (bit 0 = prio 0)
 * @queue:  per-priority circular doubly-linked list of task entities
 */
struct rt_prio_array {
	u32					bitmap;
	struct list_head	queue[MAX_PRIO];
};

/**
 * struct rq - the global runqueue
 * @active:     the priority array holding all runnable tasks
 * @nr_running: number of tasks currently on the runqueue
 * @curr:       pointer to the currently executing task
 */
struct rq {
	struct rt_prio_array	active;
	unsigned int			nr_running;
	struct task_struct		*curr;
};

/* Bitmap helpers */
static inline void sched_set_bit(struct rq *rq, int prio)
{
	rq->active.bitmap |= (1u << prio);
}

static inline void sched_clear_bit(struct rq *rq, int prio)
{
	rq->active.bitmap &= ~(1u << prio);
}

/*
 * sched_find_first_bit - find highest priority occupied level
 * @bitmap: the active bitmap
 *
 * Returns the index of the least-significant set bit.
 * Return: priority index, or MAX_PRIO if bitmap is zero.
 */
static inline int sched_find_first_bit(u32 bitmap)
{
	if (!bitmap)
		return MAX_PRIO;
	return __builtin_ctz(bitmap);
}

/* list_move_tail: re-append entry to tail of list */
static inline void list_move_tail(struct list_head *entry, struct list_head *head)
{
	list_del(entry);
	list_add_tail(entry, head);
}

/* list_first_entry: return pointer to first struct in list */
#define list_first_entry(head, type, member) \
	((type *)((char *)((head)->next) - __builtin_offsetof(type, member)))

#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - __builtin_offsetof(type, member)))

#define set_current_state(state)			\
	do { runqueue.curr->__state = (state); } while (0)

void sched_init(void);
struct task_struct *task_create(void (*fn)(void), int prio, const char *name);

/*
 * Take ownership of a deadline-band priority, or panic naming both claimants.
 *
 * The app set is fixed at build time, so a duplicate is a build mistake that
 * happens to be discovered at boot — not a condition to recover from.  Two
 * tasks on one deadline level means the scheduler's "next task is a pure
 * function of the ready set" property is gone, and with it the ability to
 * reproduce a scheduling bug from a log (design-philosophy.md §1).  Better to
 * refuse to boot than to run a machine whose timing cannot be explained.
 *
 * A shared-band priority claims nothing and always succeeds.
 */
void sched_claim_prio(int prio, const char *name);

/* Band name for logs: "AUDIO preempted VIDEO" explains itself, "3" does not. */
const char *prio_band_name(int prio);

/* The idle loop: enable interrupts, wait for one, reschedule.  Never returns. */
void cpu_idle(void);

/*
 * Decide whether @p becoming runnable should displace whatever is running.
 * Called from enqueue_task(), so every path that makes a task runnable is
 * covered without any of them having to remember — which is the point.
 */
void check_preempt_curr(struct rq *rq, struct task_struct *p);
void enqueue_task(struct rq *rq, struct task_struct *p);
void dequeue_task(struct rq *rq, struct task_struct *p);
struct task_struct *pick_next_task(struct rq *rq);
void schedule(void);
void scheduler_tick(void);

extern struct rq runqueue;
extern int need_resched;
extern bool sched_running;  /* true after first real context switch */

void do_exit(int code);
void sched_defer_free(struct task_struct *tsk);

#endif /* _NOTHAN_SCHED_H */
