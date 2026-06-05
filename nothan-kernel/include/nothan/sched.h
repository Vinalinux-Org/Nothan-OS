#ifndef _NOTHAN_SCHED_H
#define _NOTHAN_SCHED_H

#include <nothan/types.h>
#include <nothan/mm.h>

/* Task state constants */
#define TASK_RUNNING		0	/* on runqueue or currently executing */
#define TASK_SLEEPING		1	/* blocked, waiting for an event */
#define TASK_ZOMBIE			2	/* exited, not yet reaped */

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
 * @stack:   saved kernel SP (points to top of saved register frame on
 *           the task's kernel stack, set by __switch_to on context out)
 * @__state: TASK_RUNNING / TASK_SLEEPING / TASK_ZOMBIE
 * @pid:     process identifier (monotonically increasing)
 * @prio:    static priority, 0 = highest, MAX_PRIO-1 = lowest
 * @rt:      embedded scheduling entity
 * @comm:    human-readable task name (for printk debugging)
 */
struct task_struct {
	void					*stack;
	unsigned int			__state;
	int						pid;
	int						prio;
	struct sched_rt_entity	rt;
	char					comm[16];
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

/* list_add_tail: insert before head (append to tail) */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	new->next = head;
	new->prev = head->prev;
	head->prev->next = new;
	head->prev = new;
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

void sched_init(void);
struct task_struct *task_create(void (*fn)(void), int prio, const char *name);
void enqueue_task(struct rq *rq, struct task_struct *p);
void dequeue_task(struct rq *rq, struct task_struct *p);
struct task_struct *pick_next_task(struct rq *rq);
void schedule(void);
void scheduler_tick(void);

extern struct rq runqueue;
extern int need_resched;

#endif /* _NOTHAN_SCHED_H */
