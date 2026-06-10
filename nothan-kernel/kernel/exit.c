/*
 * kernel/exit.c - Task exit and resource cleanup
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

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

	tsk->exit_code = code;
	tsk->exit_state = EXIT_ZOMBIE;
	tsk->__state = TASK_UNINTERRUPTIBLE;

	/* Release user-space resources if any */
	if (tsk->mm) {
		struct zone *zone = get_zone();

		/* Clear L1[0] before freeing L2 */
		mmu_switch_mm(NULL);

		/* free L2 page table */
		if (tsk->mm->l2)
			kfree(tsk->mm->l2);

		/* free code page */
		struct page *cp = pfn_to_page(zone,
			(tsk->mm->code_pa - zone->base_pa) >> PAGE_SHIFT);
		if (cp)
			__free_pages(cp, 0);

		/* free stack page */
		struct page *sp = pfn_to_page(zone,
			(tsk->mm->stack_pa - zone->base_pa) >> PAGE_SHIFT);
		if (sp)
			__free_pages(sp, 0);

		kfree(tsk->mm);
		tsk->mm = NULL;

		printk("[EXIT] task \"%s\" pid=%d: user pages freed\n",
		       tsk->comm, tsk->pid);
	}

	printk("[EXIT] task \"%s\" pid=%d exited with code %d\n",
	       tsk->comm, tsk->pid, code);

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
