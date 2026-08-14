/*
 * kernel/exit.c - Task exit and resource cleanup
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <asm/irqflags.h>
#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/printk.h>

/**
 * do_exit() - terminate the current task and release resources
 * @code: exit status code
 *
 * Linux: kernel/exit.c do_exit().
 * Freezes the task, releases user pages, then calls schedule()
 * never to return.
 */
void do_exit(int code)
{
	struct task_struct *tsk = runqueue.curr;

	/* Loud, earliest-possible marker: ANY task death lands here first,
	 * whether from a fault (preceded by a [DABT]/[PABT] line) or a clean
	 * exit syscall (main() returning -> crt0 svc, with no fault line). */
	printk("\n[DOEXIT] >>> pid=%d \"%s\" code=%d <<<\n",
	       tsk->pid, tsk->comm, code);

	tsk->exit_code = code;
	tsk->__state = TASK_UNINTERRUPTIBLE;

	/* Release user-space resources if any */
	if (tsk->mm) {
		struct zone *zone = get_zone();
		unsigned long f_start = zone->free_pages;	/* MEMCHK */

		/* Switch off this task's address space (TTBR0 → swapper) before
		 * freeing its page tables, since they are the active TTBR0. */
		mmu_switch_mm(NULL);

		/* Free the private L1 + its L2 tables. */
		pgd_free(tsk->mm);
		unsigned long f_pgd = zone->free_pages;		/* MEMCHK */

		/* Compute orders matching how spawn allocated. */
		unsigned int code_order = 0;
		while ((1u << code_order) < tsk->mm->code_pages)
			code_order++;

		struct page *cp = pfn_to_page(zone,
			(tsk->mm->code_pa - zone->base_pa) >> PAGE_SHIFT);
		if (cp)
			__free_pages(cp, code_order);

		mm_free_bss_chunks(tsk->mm, zone);

		unsigned int stack_order = 0;
		while ((1u << stack_order) < tsk->mm->stack_pages)
			stack_order++;
		struct page *sp = pfn_to_page(zone,
			(tsk->mm->stack_pa - zone->base_pa) >> PAGE_SHIFT);
		if (sp)
			__free_pages(sp, stack_order);

		kfree(tsk->mm);
		tsk->mm = NULL;

		/* MEMCHK: how many pages each stage returned. pgd should be +4
		 * (16 KB L1); code/bss/stack the rest. Kernel stack is freed later
		 * by the reaper — see [MEMCHK] reap. */
		printk("[MEMCHK] exit pid=%d: free %lu->%lu pages (pgd +%lu, rest +%lu)\n",
		       tsk->pid, f_start, zone->free_pages,
		       f_pgd - f_start, zone->free_pages - f_pgd);
		printk("[EXIT] task \"%s\" pid=%d: user pages freed\n",
		       tsk->comm, tsk->pid);
	}

	printk("[EXIT] task \"%s\" pid=%d exited with code %d\n",
	       tsk->comm, tsk->pid, code);

	/* We're still executing on this task's kernel stack, so we can't free
	 * it (or the task_struct) here. Hand both to the reaper, which runs in
	 * the next task's context. */
	/*
	 * Mask before touching dead_list, not after.  reap_dead() walks that
	 * list from inside schedule(), so a tick landing between the list_add
	 * and the schedule() call would send the reaper through a node that is
	 * half linked — and the node is this task's own rt.run_list, which the
	 * runqueue also links through.
	 *
	 * Masked and never restored: this task is dying and will not come back
	 * from schedule() to unmask anything.  Whichever task runs next
	 * restores its own flags, exactly as if this one had blocked.
	 */
	local_irq_disable();

	/*
	 * Say that this is a death, not a sleep — and say it here, masked,
	 * rather than up where the state was set.
	 *
	 * A dying task is parked in TASK_UNINTERRUPTIBLE, the same state a task
	 * waiting on a device sits in, so the switch below is indistinguishable
	 * from an ordinary block by state alone.  The distinction matters: it is
	 * the difference between "this task stopped and is never coming back"
	 * and "this task is waiting for something", two readings of a stalled
	 * machine that lead in opposite directions.
	 *
	 * The first attempt asked for a reschedule at the top of this function,
	 * with interrupts still open.  The very next interrupt — and printk()
	 * arms the console TX interrupt, so one arrives almost at once — served
	 * that request on its way out and never returned here.  The task was
	 * already non-runnable, so schedule() did not put it back either: it
	 * left the CPU for good in the middle of its own cleanup, before
	 * reaching the line below.  Nothing was queued for the reaper, so its
	 * kernel stack and task_struct were simply lost, once per task death,
	 * and the only outward sign was a missing log line.
	 *
	 * set_resched_cause() records the reason without requesting anything,
	 * which is what a caller that is about to call schedule() itself
	 * actually means.
	 */
	set_resched_cause(RESCHED_EXIT);

	sched_defer_free(tsk);
	schedule();

	/* NOTREACHED */
	while (1)
		;
}
