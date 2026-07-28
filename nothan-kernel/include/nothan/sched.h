#ifndef _NOTHAN_SCHED_H
#define _NOTHAN_SCHED_H

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/rbtree.h>

/* Only ever stored as a pointer here; fs.h has the definition. Forward-declared
 * rather than included so the host test harness does not drag in the VFS. */
struct files_struct;

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

#define TASK_DEAD		0x00000010	/* done exiting, on dead_list, awaiting reap (EXIT_DEAD) */

/*
 * task_struct.flags bits — a SEPARATE field from __state. A flag is a request
 * overlaid on whatever state the task is in, not a state itself.
 */
#define TASK_SHOULD_EXIT	0x00000001	/* cooperative kill: task exits at its next syscall boundary */

/*
 * Default task priority. Vestigial under CFS-lite (no priority scheduling) —
 * kept only as a task attribute reported by the ps syscall.
 */
#define DEFAULT_PRIO		16

/**
 * struct sched_rt_entity - per-task CFS-lite scheduling entity
 * @run_list:              spare list node, now reused only for the dead_list.
 * @on_rq:                 1 if the task is runnable (in tasks_timeline).
 * @vruntime:              virtual runtime — total CPU time this task has run.
 * @exec_start:            sched_clock() at the start of the current run.
 * @sum_exec_runtime:      real CPU time accumulated (for the tick slice check).
 * @prev_sum_exec_runtime: sum_exec_runtime snapshot when last picked.
 * @run_node:              node in rq.tasks_timeline, keyed by vruntime.
 */
struct sched_rt_entity {
	struct list_head	run_list;
	int			on_rq;

	u64			vruntime;
	u64			exec_start;
	u64			sum_exec_runtime;
	u64			prev_sum_exec_runtime;
	struct rbnode		run_node;
};

/**
 * struct task_struct - per-task descriptor
 * @stack:     saved kernel SP (top of saved register frame on kernel stack)
 * @__state:   TASK_RUNNING / TASK_INTERRUPTIBLE / TASK_UNINTERRUPTIBLE / ...
 * @pid:       process identifier (monotonically increasing)
 * @prio:      task priority attribute (vestigial under CFS-lite; shown by ps)
 * @rt:        embedded scheduling entity
 * @comm:      human-readable task name (for printk debugging)
 * @mm:        NULL = kernel thread
 * @exit_code: exit status code set by do_exit()
 */
struct task_struct {
	void				*stack;
	void				*kstack_base;	/* kmalloc base of kernel stack (free on exit) */
	unsigned long			user_sp;
	unsigned long			user_lr;
	unsigned int			__state;
	unsigned int			flags;	/* TASK_SHOULD_EXIT etc. — request bits, not state */
	int				pid;
	int				prio;
	struct sched_rt_entity		rt;
	char				comm[16];
	struct mm_struct		*mm;    /* NULL = kernel thread */
	struct files_struct		*files;	/* NULL = kernel thread (opens nothing) */
	int				exit_code;
	char				cwd[64];

	/*
	 * Lifetime + registry (memory-bound registry; no fixed cap).
	 * @refcount: freed only when this drops to 0 (see put_task_struct).
	 *            1 at creation; task_find() takes an extra ref for its caller.
	 * @tasks:    node on the global all_tasks list (every live task).
	 * @pid_hash: node on pid_hash_table[pid % PID_HASH_SIZE] bucket.
	 */
	int				refcount;
	struct list_head		tasks;
	struct list_head		pid_hash;

	/*
	 * Wait-queue membership node — SEPARATE from rt.run_list (runqueue node).
	 * Keeping them distinct (à la Linux's wait_queue_entry vs se.run_node) is
	 * what lets a blocked task be removed from its wait queue unambiguously,
	 * without the stale-pointer hazard of reusing one node for both.
	 */
	struct list_head		wait_node;
};

/**
 * struct rq - the global runqueue (CFS-lite)
 * @nr_running:     runnable tasks queued in tasks_timeline (excludes curr)
 * @curr:           the currently executing task (lives off the tree)
 * @tasks_timeline: runnable tasks keyed by vruntime (leftmost = next to run)
 * @min_vruntime:   monotonic floor; see place_entity()
 *
 * Locking: every field here is protected by the IRQ-masked region that
 * surrounds __schedule().  schedule() masks for ordinary callers; blocking
 * primitives that already hold the mask (msgq, completion) call __schedule()
 * directly and keep it held across the switch.
 *
 * Which region protects which data across the whole kernel is written down in
 * one place - Documentation/locking-map.md.  Change the code, change the map.
 */
struct rq {
	unsigned int			nr_running;
	struct task_struct		*curr;
	struct rbtree			tasks_timeline;
	u64				min_vruntime;
};

/* list_first_entry: return pointer to first struct in list */
#define list_first_entry(head, type, member) \
	((type *)((char *)((head)->next) - __builtin_offsetof(type, member)))

#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - __builtin_offsetof(type, member)))

#define set_current_state(state)			\
	do { runqueue.curr->__state = (state); } while (0)

/* Cooperative-kill flag test — true once sys_kill has marked this task. */
static inline int task_should_exit(struct task_struct *t)
{
	return (t->flags & TASK_SHOULD_EXIT) != 0;
}

void sched_init(void);
struct task_struct *task_create(void (*fn)(void), int prio, const char *name);
void enqueue_task(struct rq *rq, struct task_struct *p);
void dequeue_task(struct rq *rq, struct task_struct *p);
struct task_struct *pick_next_task(struct rq *rq);

/* CFS-lite fair-scheduling core (kernel/sched/fair.c) */
void update_curr(struct rq *rq);			/* charge the running task's vruntime */
void place_entity(struct rq *rq, struct task_struct *p, int initial);	/* wakeup/new clamp */
u64  sched_slice(struct rq *rq);			/* a task's share of one period */
void schedule(void);
void __schedule(void);	/* core reschedule; caller must hold IRQs masked */
void scheduler_tick(void);

extern struct rq runqueue;
extern int need_resched;
extern bool sched_running;  /* true after first real context switch */

void do_exit(int code);
void sched_defer_free(struct task_struct *tsk);

/*
 * Force a blocked task runnable so it reaches its next kill check and exits.
 * No-op unless the task is in a killable (TASK_INTERRUPTIBLE) sleep. Used by
 * sys_kill to reach tasks blocked off the runqueue.
 */
void wake_up_task(struct task_struct *t);

/*
 * Memory-bound task registry — every task (kernel + user + idle) is linked on
 * the global all_tasks list and hashed by pid. No fixed cap: the only ceiling
 * is RAM (task creation fails earlier at kmalloc). task_find() by pid also sees
 * tasks blocked off the runqueue.
 */
#define PID_HASH_SIZE	64		/* buckets; chained with list_head */

void task_register(struct task_struct *p);	/* cannot fail */
void task_unregister(struct task_struct *p);
struct task_struct *task_find(int pid);		/* returns a REFFED task; caller put_task_struct() */

/*
 * Task refcount API. Behind these two calls so the single-core impl (plain int
 * under IRQ mask) can be swapped for atomics on SMP without touching callers.
 */
void get_task_struct(struct task_struct *p);
void put_task_struct(struct task_struct *p);	/* frees kstack + task_struct when refcount hits 0 */

extern struct list_head all_tasks;	/* global list of every live task */

#endif /* _NOTHAN_SCHED_H */
