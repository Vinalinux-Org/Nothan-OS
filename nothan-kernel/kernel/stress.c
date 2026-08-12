/*
 * kernel/stress.c - concurrency stress, the acceptance test for Phase 1
 *
 * Every "test" up to this point has been: boot, type ls, type ps. For a race
 * that is close to meaningless. A race needs two flows of control touching the
 * same thing at the same time, and a kernel running one shell almost never is
 * in that state — which is exactly why the locking audit needs something that
 * puts it there deliberately.
 *
 * Three kernel tasks at the same priority, so the round-robin tick keeps
 * cutting between them, plus the tick itself interrupting all of them:
 *
 *   two printers   the console path, which has three callers and one masked
 *                  writer.  Each prints a line built from a single repeated
 *                  character, so interleaving is visible by eye: a line
 *                  containing both letters, or a short line, means a write was
 *                  cut in half.
 *
 *   one allocator  the buddy and slab free lists, mutated while the tick and
 *                  the other two tasks are running.  Page accounting is
 *                  checked against the starting value at the end, so a leak or
 *                  a double free shows up as a number rather than as a crash
 *                  three minutes later.
 *
 * All three sleep in between, which drags in msleep, the timer list, the wait
 * path and schedule() under the same load.  They exit when done, so the exit
 * and reap paths get exercised while the others are still running.
 *
 * Kernel-side on purpose: no SIM, no display, no USB. The board is bare and
 * this has to work on a bare board.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/mm.h>
#include <nothan/slab.h>
#include <nothan/delay.h>
#include <nothan/printk.h>

#define STRESS_LINES		60	/* lines per printer */
#define STRESS_ALLOC_ROUNDS	200	/* alloc/free rounds */
#define STRESS_PAYLOAD		48	/* characters in a printer's line body */

static volatile int stress_done;	/* counts tasks that finished */

/*
 * One repeated character per printer.  A correct console produces lines that
 * are all A or all B; any mixture, or a line shorter than STRESS_PAYLOAD, is a
 * write that got cut into by another writer.
 */
static void stress_printer(char tag)
{
	char line[STRESS_PAYLOAD + 1];
	int i;

	for (i = 0; i < STRESS_PAYLOAD; i++)
		line[i] = tag;
	line[STRESS_PAYLOAD] = '\0';

	for (i = 0; i < STRESS_LINES; i++) {
		printk("[STRESS-%c] %s %d\n", tag, line, i);
		msleep(5);
	}

	printk("[STRESS-%c] done, %d lines\n", tag, STRESS_LINES);
	stress_done++;
}

static void stress_printer_a(void) { stress_printer('A'); }
static void stress_printer_b(void) { stress_printer('B'); }

/*
 * Hammer both allocators while the others run.  Orders 0-3 so the buddy has to
 * split and merge rather than handing back the same block every time.
 */
static void stress_alloc(void)
{
	struct zone *zone = get_zone();
	unsigned long free_before = zone->free_pages;
	unsigned long free_after;
	int round;

	for (round = 0; round < STRESS_ALLOC_ROUNDS; round++) {
		struct page *pg[4];
		void *slab[4];
		unsigned int i;

		for (i = 0; i < 4; i++)
			pg[i] = alloc_pages(GFP_KERNEL, i);

		for (i = 0; i < 4; i++)
			slab[i] = kmalloc(32u << i, GFP_KERNEL);

		/* Free in a different order than allocated, so the buddy has to
		 * cope with holes rather than a clean unwind. */
		for (i = 0; i < 4; i++)
			if (slab[i])
				kfree(slab[i]);

		for (i = 4; i-- > 0; )
			if (pg[i])
				__free_pages(pg[i], i);

		if ((round % 50) == 0)
			msleep(5);
	}

	free_after = zone->free_pages;

	if (free_after == free_before)
		printk("[STRESS-M] done, %d rounds, pages balanced at %lu\n",
		       STRESS_ALLOC_ROUNDS, free_after);
	else
		printk("[STRESS-M] LEAK: %lu pages before, %lu after (%ld lost)\n",
		       free_before, free_after,
		       (long)free_before - (long)free_after);

	stress_done++;
}

/**
 * stress_start() - launch the stress tasks
 *
 * Called from kernel_main with interrupts already masked, alongside the other
 * initial tasks.
 */
void stress_start(void)
{
	static const struct {
		void (*fn)(void);
		const char *name;
	} tasks[] = {
		{ stress_printer_a, "stress-a" },
		{ stress_printer_b, "stress-b" },
		{ stress_alloc,     "stress-m" },
	};
	unsigned int i;

	if (!CONFIG_STRESS_TEST)
		return;

	printk("[STRESS] starting: 2 printers x %d lines, %d alloc rounds\n",
	       STRESS_LINES, STRESS_ALLOC_ROUNDS);
	printk("[STRESS] pass = every line is one repeated letter, no short lines,"
	       " and pages balanced\n");

	for (i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
		struct task_struct *t = task_create(tasks[i].fn, DEFAULT_PRIO,
						    tasks[i].name);
		if (t)
			enqueue_task(&runqueue, t);
		else
			printk("[STRESS] could not create %s\n", tasks[i].name);
	}
}
