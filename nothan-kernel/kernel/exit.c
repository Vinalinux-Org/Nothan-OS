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
	sched_defer_free(tsk);

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
