/*
 * kernel/sched/completion.c - Completion synchronisation primitives
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/mm.h>
#include <nothan/sched.h>
#include <nothan/wait.h>
#include <nothan/completion.h>

/**
 * wait_for_completion() - block until a completion is signalled
 * @c: pointer to the completion structure
 */
void wait_for_completion(struct completion *c)
{
	for (;;) {
		set_current_state(TASK_UNINTERRUPTIBLE);

		/*
		 * IRQ-off closes the race between checking c->done and
		 * adding ourselves to the wait list.  complete() may fire
		 * from timer / IRQ context.
		 */
		__asm__ __volatile__ ("cpsid i" : : : "memory");

		if (c->done) {
			c->done--;
			__asm__ __volatile__ ("cpsie i" : : : "memory");
			set_current_state(TASK_RUNNING);
			return;
		}

		list_add_tail(&runqueue.curr->rt.run_list, &c->wait.task_list);

		__asm__ __volatile__ ("cpsie i" : : : "memory");

		schedule();

		/*
		 * Woken up by complete() → loop back to re-check c->done.
		 * The outer loop handles spurious wakeups correctly.
		 */
		set_current_state(TASK_RUNNING);
	}
}

/**
 * complete() - signal a completion
 * @c: pointer to the completion structure
 */
void complete(struct completion *c)
{
	__asm__ __volatile__ ("cpsid i" : : : "memory");

	c->done++;

	if (!list_empty(&c->wait.task_list)) {
		struct task_struct *p = list_first_entry(&c->wait.task_list,
					struct task_struct, rt.run_list);
		list_del(&p->rt.run_list);
		p->__state = TASK_RUNNING;
		enqueue_task(&runqueue, p);
	}

	__asm__ __volatile__ ("cpsie i" : : : "memory");
}
