/*
 * kernel/sched/completion.c - Completion synchronisation primitives
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Built on the same race-free blocking pattern as wait_event(): IRQs are
 * masked across "test c->done -> queue self -> __schedule()", and
 * __schedule() returns still masked.  This removes the window the previous
 * cpsie-then-schedule version had, where complete() from an IRQ could
 * enqueue the waiter and then __schedule() enqueue it a second time.
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
	/*
	 * Boot context: scheduler initialized (sched_init done) but no real
	 * context switch has happened yet (sched_running=false).  We cannot
	 * block — spin until an ISR calls complete().  IRQs must be enabled.
	 */
	if (!sched_running) {
		while (!c->done)
			__asm__ __volatile__ ("" : : : "memory");
		c->done--;
		return;
	}

	unsigned long flags;

	local_irq_save(flags);
	while (!c->done) {
		__prepare_to_wait(&c->wait, TASK_UNINTERRUPTIBLE);
		__schedule();
	}
	c->done--;
	__finish_wait();
	local_irq_restore(flags);
}

/**
 * complete() - signal a completion, waking one waiter
 * @c: pointer to the completion structure
 */
void complete(struct completion *c)
{
	unsigned long flags;

	local_irq_save(flags);
	c->done++;
	local_irq_restore(flags);

	wake_up(&c->wait);
}
