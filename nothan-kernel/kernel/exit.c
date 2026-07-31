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
#include <nothan/wait.h>
#include <nothan/syscall.h>
#include <nothan/panic.h>

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
	 * init must never die, and this is where that is enforced - not in
	 * sys_kill.
	 *
	 * sys_kill's "pid <= 1" guard stops another PROCESS from killing init.
	 * It cannot stop init from dying on its own: a fault in init's own code
	 * (SIGSEGV/SIGILL) or main() returning both arrive here having asked
	 * nobody's permission. That path exists only because init is a user
	 * process now; as a kernel thread it could not fault into exit.
	 *
	 * Continuing without init is not an option worth building. Every orphan
	 * is reparented to init, so a dead PID 1 leaves every future orphan
	 * holding a pointer into a freed task_struct - a use-after-free that
	 * surfaces later, somewhere else, as exactly the kind of bug a UART
	 * cannot trace back. Fail-fast instead: stop here, where the log still
	 * says which task died and why.
	 */
	if (tsk == init_task)
		panic("init (pid=%d) died: %s %d - nothing can be reparented",
		      tsk->pid,
		      how == EXIT_HOW_KILLED ? "killed by" : "exited with status",
		      value);

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
	tsk->__state = EXIT_ZOMBIE;	/* dead; status still to be collected */

	/*
	 * Close whatever this task left open, before the address space goes.
	 * Until this existed, every task that died with a file open leaked the
	 * struct file, its inode and the driver's private_data - not
	 * occasionally, every time, because nothing else ever closed them.
	 *
	 * This is also the mechanism the rest of the kernel's resource
	 * lifetimes are meant to hang off: anything given an fd gets released
	 * here for free, with no per-resource teardown code.
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

	/*
	 * The parent is NOT notified here, deliberately.
	 *
	 * do_exit() runs with IRQs enabled and on this task's own kernel stack.
	 * Waking the parent now lets it be scheduled immediately, call wait(),
	 * and release_task() this very corpse - freeing the stack we are still
	 * executing on. Use-after-free, inside the scheduler, on the one class
	 * of bug the UART cannot investigate.
	 *
	 * The reaper does the notifying instead: it only touches a corpse once
	 * that corpse is no longer the running task, which is exactly the
	 * condition that makes collection safe. See reap_dead().
	 */

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

		/* Code and bss are scattered; each chunk remembers its own order,
		 * so nothing here has to reconstruct how they were allocated. */
		mm_free_chunks(tsk->mm->code_chunks, &tsk->mm->nr_code_chunks, zone);
		mm_free_chunks(tsk->mm->bss_chunks, &tsk->mm->nr_bss_chunks, zone);

		/* The stack IS a single block, so its order is derived. */
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

/**
 * find_zombie_child() - one collected corpse belonging to @parent, if any
 * @want: PID to wait for, or WAIT_ANY for whichever child dies first
 *
 * Walks all_tasks rather than a per-parent children list, for the same reason
 * reparent_to_init() does: nothing else traverses downward, so an index would
 * be state to maintain and get wrong for one reader. Iterative, never
 * recursive - tree depth is deliberately unbounded.
 *
 * @any_child tells the caller whether the parent still has anything worth
 * waiting FOR - which, when @want names a PID, means that specific child and
 * not merely some child. That is what lets wait() tell "not dead yet, go to
 * sleep" apart from "nothing will ever arrive, return an error": waiting on a
 * PID that is not ours must fail immediately rather than sleep forever.
 * Computed in the same masked walk so the two answers cannot disagree.
 *
 * Caller must hold IRQs masked: this walks the registry.
 */
static struct task_struct *find_zombie_child(struct task_struct *parent,
					     int want, int *any_child)
{
	struct task_struct *p;

	*any_child = 0;
	list_for_each_entry(p, &all_tasks, struct task_struct, tasks) {
		if (p->parent != parent)
			continue;
		if (want != WAIT_ANY && p->pid != want)
			continue;	/* someone else's corpse; not what we asked for */
		*any_child = 1;
		/*
		 * kstack_base == NULL means the reaper has finished with this
		 * corpse, i.e. it is off the CPU and its stack is freed. A
		 * zombie that has not reached that point is dead but still
		 * standing on its own stack; releasing it would free memory it
		 * is executing on.
		 */
		if (p->__state == EXIT_ZOMBIE && !p->kstack_base)
			return p;
	}
	return NULL;
}

/**
 * do_wait() - collect one dead child's exit status
 * @want: PID to wait for, or WAIT_ANY for whichever child dies first
 * @st: filled in with how the child died; may be NULL to discard it
 *
 * "Come and collect", not "stand guard": if a corpse is already waiting this
 * returns immediately, and only an empty-handed caller blocks. That is what
 * the zombie state buys - a parent need not be present at the moment its child
 * dies.
 *
 * @want exists because "collect whichever died" is the wrong answer as soon as
 * a process has two children: a shell that starts a job and a helper cannot
 * wait for the job without risking collecting the helper, and a status
 * delivered to the wrong waiter is unrecoverable - the corpse is gone. Naming
 * the PID makes the caller say which death it is actually waiting for.
 *
 * Waiting on a PID that is not this task's child returns -1 at once. It cannot
 * sleep: nothing will ever wake it, because the wakeup comes from a child dying
 * and that PID is not a child.
 *
 * Callable from kernel context as well as from sys_wait().
 *
 * Return: PID of the collected child, or -1 if there is nothing to wait for.
 */
int do_wait(int want, struct exit_status *st)
{
	struct task_struct *me = runqueue.curr;
	unsigned long flags;
	int ret = -1;

	/*
	 * The whole loop runs masked, and the mask is HELD ACROSS __schedule()
	 * - the same shape msgq_send/recv use, and for the same reason: the
	 * test ("is there a zombie?") and the decision to sleep must not be
	 * separable, or a child that dies in between wakes nobody and the
	 * parent sleeps forever.
	 */
	local_irq_save(flags);
	for (;;) {
		int any_child;
		struct task_struct *z = find_zombie_child(me, want, &any_child);

		if (z) {
			int pid = z->pid;

			if (st) {
				st->how = (int)z->exit_how;
				st->value = z->exit_value;
			}
			release_task(z);	/* third step of dying: gone */
			local_irq_restore(flags);
			return pid;
		}

		if (!any_child)
			break;			/* nothing to wait for, ever */

		if (task_should_exit(me)) {	/* killed while waiting */
			__finish_wait();
			break;
		}

		__prepare_to_wait(&me->child_wait, TASK_INTERRUPTIBLE);
		__schedule();
	}
	__finish_wait();
	local_irq_restore(flags);

	return ret;
}
