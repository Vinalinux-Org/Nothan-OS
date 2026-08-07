#ifndef _NOTHAN_SCHED_H
#define _NOTHAN_SCHED_H

#include <nothan/types.h>
#include <nothan/mm.h>
#include <nothan/rbtree.h>

/* Only ever stored as a pointer here; fs.h has the definition. Forward-declared
 * rather than included so the host test harness does not drag in the VFS. */
struct files_struct;

/*
 * Task states. Every one of these is ASSIGNED somewhere - that is the entry
 * requirement for being here.
 *
 * Numbering follows Linux (bitmask-style) because numbering is pure convention
 * and pure cost to change later; the set does not, because a state constant
 * nothing ever stores is a claim the kernel does not deliver. TASK_NEW,
 * TASK_STOPPED, TASK_TRACED, TASK_WAKEKILL and TASK_KILLABLE were all declared
 * here and never once written, so ps could not show them and no code path
 * produced them - they described a kernel with job control and a debugger,
 * which this is not. Add each back with the code that sets it.
 */
#define TASK_RUNNING		0x00000000	/* running or on runqueue */
#define TASK_INTERRUPTIBLE	0x00000001	/* sleep; wake_up_task can force it awake */
#define TASK_UNINTERRUPTIBLE	0x00000002	/* sleep; nothing can force it awake */

#define TASK_DEAD		0x00000010	/* released; nothing left to collect (EXIT_DEAD) */
#define EXIT_ZOMBIE		0x00000020	/* dead, but exit status not collected yet */

/*
 * A task dies in THREE steps, not two. The split exists because the three
 * things being released have different owners and different deadlines:
 *
 *   1. do_exit()   frees everything heavy - mm, page tables, open files - and
 *                  marks the task EXIT_ZOMBIE. Heavy resources are returned
 *                  IMMEDIATELY; nothing waits on a parent for those.
 *   2. reaper      frees the KERNEL STACK, and only that. It cannot be freed
 *                  in step 1 because the dying task is still executing on it.
 *   3. wait()      reads the exit status, then release_task() unregisters and
 *                  drops the last reference.
 *
 * What a zombie costs is therefore one task_struct - a few hundred bytes -
 * NOT the 16 KB kernel stack. That distinction is what makes keeping corpses
 * around affordable: ten zombies holding stacks would pin 160 KB.
 */

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
 * struct wait_queue_head - queue of tasks waiting for an event
 * @task_list: sleeping tasks, linked through task->wait_node
 *
 * Defined here rather than in <nothan/wait.h> because task_struct embeds one
 * by value; wait.h includes this header, so the definition cannot live there.
 */
struct wait_queue_head {
	struct list_head task_list;
};

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
 * @exit_how/@exit_value: how this task died — see the fields themselves
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

	/*
	 * How this task died, in TWO fields rather than one packed int.
	 *
	 * The kind of death and the value that goes with it are different
	 * questions, and "exited with status 11" must never be confusable with
	 * "killed by signal 11". Linux packs both into one int and then needs
	 * WIFEXITED/WTERMSIG to take them apart again - a shape it carries for
	 * history, not because it is better. Diverging here is free.
	 *
	 * @exit_how:   EXIT_HOW_EXITED or EXIT_HOW_KILLED
	 * @exit_value: exit status if EXITED, signal number if KILLED
	 */
	unsigned int			exit_how;
	int				exit_value;

	/*
	 * @parent: who created this task. NULL only for init and for PID 0.
	 *
	 * A child knows its parent and nothing else - there is no children list,
	 * because nothing needs one yet. Reparenting walks all_tasks instead;
	 * with no cascade-kill and no wait(), an O(children) index would be
	 * state to maintain (and get wrong) for no reader. Add the list when
	 * something actually traverses downward.
	 *
	 * The pointer MUST be kept alive: when a parent dies its children are
	 * handed to init, or every orphan is left holding a dangling pointer
	 * into a freed task_struct.
	 */
	struct task_struct		*parent;

	/*
	 * Where this task sleeps while waiting for one of its children to die.
	 * A dying child wakes its parent's queue - the parent never polls.
	 *
	 * Per task rather than one global queue: waking every waiter whenever
	 * any task anywhere dies would have each of them wake, walk the task
	 * list, find nothing of theirs, and sleep again.
	 */
	struct wait_queue_head		child_wait;

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

/*
 * Create a kernel thread. @arg reaches @fn as its first parameter — see
 * task_entry in arch/arm/kernel/switch_to.S, which carries it in r6 through
 * the pre-built switch frame. Threads that need no argument pass NULL.
 */
struct task_struct *task_create(void (*fn)(void *), void *arg, int prio,
				const char *name);
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

/*
 * Two ways a task can die, kept as two entry points rather than one function
 * with a mode argument - the caller always knows statically which it is, and a
 * fault handler that has to remember to pass a flag will eventually not.
 */
#define EXIT_HOW_EXITED		0	/* ran to completion / called exit() */
#define EXIT_HOW_KILLED		1	/* fault or kill; exit_value = signal */

void do_exit(int code);			/* voluntary: sys_exit, thread return */
void do_exit_killed(int sig);		/* involuntary: fault handlers, sys_kill */
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

/*
 * PID 1. The root of the process tree and the adoptive parent of every orphan.
 * NULL until init_task_create() runs, which is the first thing kernel_main
 * does after sched_init().
 */
extern struct task_struct *init_task;

struct task_struct *init_task_create(void);	/* PID 1; NULL on failure */
int spawn_path(const char *path, int argc, const char *const *argv);	/* PID, or -1 */
struct task_struct *user_task_create_file(const char *name, const char *path,
					  int argc, const char *const *argv);
void reparent_to_init(struct task_struct *dying);
void release_task(struct task_struct *p);	/* wait() collected it; delete for good */
struct exit_status;
int do_wait(int want, struct exit_status *st);	/* PID collected, or -1 */

#endif /* _NOTHAN_SCHED_H */
