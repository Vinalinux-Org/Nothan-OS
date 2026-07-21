#ifndef _NOTHAN_SCHED_H
#define _NOTHAN_SCHED_H

#include <nothan/types.h>
#include <nothan/mm.h>

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

/* Scheduling constants */
#define MAX_PRIO			32	/* 32 fixed priority levels */
#define IDLE_PRIO			(MAX_PRIO - 1)	/* lowest: idle task */
#define DEFAULT_PRIO		16	/* mid-point for normal tasks */
#define RR_TIMESLICE		1	/* ticks per timeslice (1 tick = 10 ms) */

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
	unsigned long			user_sp;
	unsigned long			user_lr;
	unsigned int			__state;
	unsigned int			flags;	/* TASK_SHOULD_EXIT etc. — request bits, not state */
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
void enqueue_task(struct rq *rq, struct task_struct *p);
void dequeue_task(struct rq *rq, struct task_struct *p);
struct task_struct *pick_next_task(struct rq *rq);
void schedule(void);
void __schedule(void);	/* core reschedule; caller must hold IRQs masked */
void scheduler_tick(void);

extern struct rq runqueue;
extern int need_resched;
extern bool sched_running;  /* true after first real context switch */

void do_exit(int code);
void sched_defer_free(struct task_struct *tsk);

/*
 * Flat task registry — every task (kernel + user + idle) is registered here.
 * Bounded by MAX_TASKS. Lets kill find a task by pid including ones blocked
 * off the runqueue, and caps the number of live tasks.
 */
int task_register(struct task_struct *p);	/* 0 ok, -1 if table full */
void task_unregister(struct task_struct *p);
struct task_struct *task_find(int pid);

#endif /* _NOTHAN_SCHED_H */
