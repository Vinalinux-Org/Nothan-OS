/*
 * kernel/sched/completion.c - Completion synchronisation primitives
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/mm.h>
#include <nothan/sched.h>
#include <nothan/wait.h>
#include <nothan/completion.h>
#include <asm/irqflags.h>

/**
 * wait_for_completion() - block until a completion is signalled
 * @c: pointer to the completion structure
 */
void wait_for_completion(struct completion *c)
{
	unsigned long flags;

	/*
	 * Boot context: scheduler initialized (sched_init done) but no real
	 * context switch has happened yet (sched_running=false).  We cannot
	 * block — spin until ISR calls complete().  IRQs must be enabled.
	 */
	if (!sched_running) {
		while (!c->done)
			__asm__ __volatile__ ("" : : : "memory");
		c->done--;
		return;
	}

	for (;;) {
		/*
		 * One masked region covers checking the count, marking
		 * ourselves asleep, joining the wait list, and giving up the
		 * CPU.  schedule() returns still masked (see its contract), so
		 * there is no window anywhere in the middle for complete() to
		 * run from an ISR against a half-linked list node.
		 */
		flags = local_irq_save();

		if (c->done) {
			c->done--;
			local_irq_restore(flags);
			set_current_state(TASK_RUNNING);
			return;
		}

		set_current_state(TASK_UNINTERRUPTIBLE);
		list_add_tail(&runqueue.curr->rt.run_list, &c->wait.task_list);

		schedule();

		local_irq_restore(flags);
		set_current_state(TASK_RUNNING);
	}
}

/**
 * complete() - signal a completion
 * @c: pointer to the completion structure
 *
 * Callable from interrupt context, which is why this uses save/restore: the
 * previous version ended with an unconditional enable, so calling it from an
 * ISR turned interrupts back on in the middle of that handler — nested
 * interrupts in a kernel that is not built for them.
 */
void complete(struct completion *c)
{
	unsigned long flags = local_irq_save();

	c->done++;

	if (!list_empty(&c->wait.task_list)) {
		struct task_struct *p = list_first_entry(&c->wait.task_list,
					struct task_struct, rt.run_list);
		list_del(&p->rt.run_list);
		p->__state = TASK_RUNNING;
		enqueue_task(&runqueue, p);
	}

	local_irq_restore(flags);
}
