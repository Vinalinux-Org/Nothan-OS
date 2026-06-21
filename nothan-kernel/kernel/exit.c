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

		/* Switch off this task's address space (TTBR0 → swapper) before
		 * freeing its page tables, since they are the active TTBR0. */
		mmu_switch_mm(NULL);

		/* Free the private L1 + its L2 tables. */
		pgd_free(tsk->mm);

		/* Compute orders matching how spawn allocated. */
		unsigned int code_order = 0;
		while ((1u << code_order) < tsk->mm->code_pages)
			code_order++;

		struct page *cp = pfn_to_page(zone,
			(tsk->mm->code_pa - zone->base_pa) >> PAGE_SHIFT);
		if (cp)
			__free_pages(cp, code_order);

		if (tsk->mm->bss_pa) {
			unsigned int bss_order = 0;
			while ((1u << bss_order) < tsk->mm->bss_pages)
				bss_order++;
			struct page *bp = pfn_to_page(zone,
				(tsk->mm->bss_pa - zone->base_pa) >> PAGE_SHIFT);
			if (bp)
				__free_pages(bp, bss_order);
		}

		unsigned int stack_order = 0;
		while ((1u << stack_order) < tsk->mm->stack_pages)
			stack_order++;
		struct page *sp = pfn_to_page(zone,
			(tsk->mm->stack_pa - zone->base_pa) >> PAGE_SHIFT);
		if (sp)
			__free_pages(sp, stack_order);

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
