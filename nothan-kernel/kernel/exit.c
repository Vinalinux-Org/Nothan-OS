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
#include <nothan/fs.h>
#include <nothan/signal.h>

static void __do_exit(unsigned int how, int value);

static const char *signame(int sig)
{
	switch (sig) {
	case SIGILL:	return "SIGILL";
	case SIGBUS:	return "SIGBUS";
	case SIGKILL:	return "SIGKILL";
	case SIGSEGV:	return "SIGSEGV";
	default:	return "SIG?";
	}
}

/**
 * do_exit() - terminate the current task, having finished of its own accord
 * @code: exit status
 */
void do_exit(int code)
{
	__do_exit(EXIT_HOW_EXITED, code);
}

/**
 * do_exit_killed() - terminate the current task, killed from outside
 * @sig: what killed it (SIGSEGV, SIGILL, SIGBUS, SIGKILL)
 *
 * Split from do_exit() because the two are not the same event, and until now
 * the kernel could not tell them apart: every fault handler called
 * do_exit(-1), so "the task crashed" and "the task called exit(-1)" produced
 * byte-identical state. The kernel is the ONLY observer of that difference -
 * it has the fault class, DFAR and PC - and it was discarding it. On a machine
 * whose only debugging tool is the UART, throwing away the one fact only the
 * kernel knows is the expensive kind of simplification.
 */
void do_exit_killed(int sig)
{
	__do_exit(EXIT_HOW_KILLED, sig);
}

/**
 * __do_exit() - the single teardown path both entry points share
 * @how:   EXIT_HOW_EXITED / EXIT_HOW_KILLED
 * @value: exit status, or signal number
 *
 * Linux: kernel/exit.c do_exit().
 * Freezes the task, releases user pages, then calls schedule()
 * never to return.
 */
static void __do_exit(unsigned int how, int value)
{
	struct task_struct *tsk = runqueue.curr;

	/*
	 * Earliest-possible marker: ANY task death lands here first, whether
	 * from a fault (preceded by a [DABT]/[PABT] line) or a clean exit
	 * syscall (main() returning -> crt0 svc, with no fault line).
	 *
	 * The two are printed differently, and at DIFFERENT LEVELS, on purpose:
	 *
	 *   killed  -> pr_err, so it survives the default build. A task dying
	 *              involuntarily is never routine, and classifying the death
	 *              is pointless if the classification is compiled out - the
	 *              kernel would know why it died and still not say so, on a
	 *              machine where the UART is the only way to find out.
	 *   exited  -> pr_debug. Expected, routine, and noisy if always on.
	 *
	 * Reading a log, the question is never "what number" but "did this
	 * thing crash or finish"; one word answers it with no status decoding.
	 */
	if (how == EXIT_HOW_KILLED)
		pr_err("\n[DOEXIT] >>> pid=%d \"%s\" KILLED by %s (%d) <<<\n",
		       tsk->pid, tsk->comm, signame(value), value);
	else
		pr_debug("\n[DOEXIT] >>> pid=%d \"%s\" exited, status=%d <<<\n",
			 tsk->pid, tsk->comm, value);

	tsk->exit_how = how;
	tsk->exit_value = value;
	tsk->__state = TASK_DEAD;	/* EXIT_DEAD: done, on dead_list, awaiting reap */

	/*
	 * Close whatever this task left open, before the address space goes.
	 * Until this existed, every task that died with a file open leaked the
	 * struct file, its inode and the driver's private_data - not
	 * occasionally, every time, because nothing else ever closed them.
	 *
	 * This is also the mechanism the rest of the kernel's resource
	 * lifetimes are meant to hang off: anything given an fd gets released
	 * here for free, with no per-resource teardown code (see D5 in
	 * Documentation/process-mm-design.md).
	 */
	files_free(tsk->files);
	tsk->files = NULL;

	/*
	 * Hand any children to init BEFORE this task_struct can be freed.
	 * A child holds a raw pointer to its parent and has no other way to
	 * reach it; leaving it pointing at a corpse is a dangling pointer that
	 * nothing detects until something follows it.
	 */
	reparent_to_init(tsk);

	/* Release user-space resources if any */
	if (tsk->mm) {
		struct zone *zone = get_zone();
		unsigned long f_start __attribute__((unused)) =
			zone->free_pages;			/* MEMCHK (pr_debug) */

		/* Switch off this task's address space (TTBR0 → swapper) before
		 * freeing its page tables, since they are the active TTBR0. */
		mmu_switch_mm(NULL);

		/* Free the private L1 + its L2 tables. */
		pgd_free(tsk->mm);
		unsigned long f_pgd __attribute__((unused)) =
			zone->free_pages;			/* MEMCHK (pr_debug) */

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
		pr_debug("[MEMCHK] exit pid=%d: free %lu->%lu pages (pgd +%lu, rest +%lu)\n",
			 tsk->pid, f_start, zone->free_pages,
			 f_pgd - f_start, zone->free_pages - f_pgd);
		pr_debug("[EXIT] task \"%s\" pid=%d: user pages freed\n",
			 tsk->comm, tsk->pid);
	}

	pr_debug("[EXIT] task \"%s\" pid=%d teardown done\n",
		 tsk->comm, tsk->pid);

	/* We're still executing on this task's kernel stack, so we can't free
	 * it (or the task_struct) here. Hand both to the reaper, which runs in
	 * the next task's context. */
	sched_defer_free(tsk);

	schedule();

	/* NOTREACHED */
	while (1)
		;
}
