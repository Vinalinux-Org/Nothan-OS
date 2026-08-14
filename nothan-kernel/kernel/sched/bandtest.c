/*
 * kernel/sched/bandtest.c - acceptance test for the priority bands
 *
 * Phase 3 §5.2 splits one scheduling rule into two: levels in a deadline band
 * hold one task each and are never rotated by the tick, while the BG band
 * holds many tasks on one level and rotates between them.  Booting proves
 * neither.  The log shows the priorities that were assigned, which is a
 * statement of intent — not evidence that the scheduler acts on them.
 *
 * The two rules produce *opposite* output from the same shape of task, so one
 * run distinguishes them with no instrumentation at all:
 *
 *   two tasks, exclusive levels (deadline band)
 *       strict priority, no rotation -> the higher one finishes completely
 *       before the lower one starts:   HHHHHHHHLLLLLLLL
 *       If the tick still rotated deadline tasks, they would interleave
 *       instead, and the difference is visible by eye.
 *
 *   two tasks, one shared level (BG band)
 *       rotation every BG_TIMESLICE -> they alternate in blocks of
 *       BG_TIMESLICE_MS / UNIT_MS units:  xxxxyyyyxxxxyyyy
 *       If rotation were missing here, the first would run to completion and
 *       the second would never get in until it exited — which is exactly the
 *       starvation the shared band exists to prevent.
 *
 * The expected string is printed before the tasks start.  A test whose answer
 * is only known after seeing the output is not a test, it is an observation;
 * this project has already paid for that lesson twice (roadmap §2.2, §1.2).
 *
 * Busy-wait on purpose.  msleep() would take each task off the CPU voluntarily
 * and the log would look identical whether or not the tick rotates anything —
 * the question is what the *scheduler* does to a task that does not yield, so
 * the task must not yield.
 *
 * Kernel-side, like stress.c: no SIM, no display, no USB.  The board is bare.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <asm/irqflags.h>
#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/sched.h>
#include <nothan/delay.h>
#include <nothan/printk.h>

#if CONFIG_SCHED_BAND_TEST

/*
 * One unit of work, in milliseconds, and how many each task performs.
 *
 * UNIT_MS is a quarter of BG_TIMESLICE_MS so a rotation block comes out as
 * four characters — few enough to count by eye, many enough that an accidental
 * rotation cannot be mistaken for the intended one.  UNITS is twice that
 * again, so each BG task is scheduled more than once and the alternation is
 * visible as a repeating pattern rather than a single handover.
 */
#define UNIT_MS		(BG_TIMESLICE_MS / 4)
#define UNITS		8

/*
 * Deadline-band levels for the test pair.
 *
 * Inside UI rather than AUDIO/NET/VIDEO because those bands are reserved for
 * work that does not exist yet, and claiming one here would make a future
 * collision look like a test artefact.  Above PRIO_GUI so the pair cannot be
 * cut into by a GUI build.
 */
#define PRIO_BAND_HI	(PRIO_UI + 2)
#define PRIO_BAND_LO	(PRIO_UI + 3)

static int bandtest_left;	/* tasks still running; last one prints the verdict */

/*
 * Burn UNITS x UNIT_MS of CPU, printing @mark after each unit.
 *
 * printk() of a single character is safe against the other tasks here: the
 * console has one masked writer, so a character cannot be cut in half.  What
 * the ordering of those characters shows is the whole point of the test.
 */
static void bandtest_work(char mark)
{
	unsigned int i;

	for (i = 0; i < UNITS; i++) {
		udelay(UNIT_MS * 1000u);
		printk("%c", mark);
	}

	unsigned long flags = local_irq_save();
	int last = (--bandtest_left == 0);
	local_irq_restore(flags);

	if (last)
		printk("\n[BAND] done\n");
}

static void bandtest_hi(void)  { bandtest_work('H'); }
static void bandtest_lo(void)  { bandtest_work('L'); }
static void bandtest_bgx(void) { bandtest_work('x'); }
static void bandtest_bgy(void) { bandtest_work('y'); }

#endif /* CONFIG_SCHED_BAND_TEST */

/**
 * bandtest_start() - launch the priority-band acceptance tasks
 *
 * Called from kernel_main with interrupts already masked, alongside the other
 * initial tasks.
 */
void bandtest_start(void)
{
#if CONFIG_SCHED_BAND_TEST
	static const struct {
		void (*fn)(void);
		int prio;
		const char *name;
	} tasks[] = {
		{ bandtest_hi,  PRIO_BAND_HI, "band-hi" },
		{ bandtest_lo,  PRIO_BAND_LO, "band-lo" },
		{ bandtest_bgx, PRIO_BG,      "band-bgx" },
		{ bandtest_bgy, PRIO_BG,      "band-bgy" },
	};
	unsigned int i;

	bandtest_left = sizeof(tasks) / sizeof(tasks[0]);

	printk("[BAND] %d units of %d ms each; BG slice %d ms = %d units\n",
	       UNITS, UNIT_MS, BG_TIMESLICE_MS, BG_TIMESLICE_MS / UNIT_MS);
	printk("[BAND] pass 1: every H before any L, i.e. \"%s\" exactly\n",
	       "HHHHHHHHLLLLLLLL");
	printk("[BAND] pass 2: x and y alternate in blocks of about %d,"
	       " each reaching %d total\n", BG_TIMESLICE_MS / UNIT_MS, UNITS);

	/*
	 * "About", not exactly, and the reason is worth stating rather than
	 * discovering twice.
	 *
	 * An earlier version of this predicted a single exact string for both
	 * halves.  The deadline half came out right; the BG half did not, and
	 * the test was wrong rather than the scheduler.  Two things move the
	 * early block boundaries and neither is a fault:
	 *
	 *   - Any other BG task shares this level and joins the rotation.
	 *     storage_daemon is spawned before these tasks, so it holds the
	 *     front of the queue until it blocks on its first request.
	 *   - The first task to run arrives partway through a tick period, so
	 *     its opening slice is short by whatever is left of that period.
	 *
	 * Both settle out.  What must hold regardless is the alternation itself
	 * and both tasks reaching UNITS, which is what a shared level exists to
	 * guarantee — so that is what the pass condition names.
	 */

	for (i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
		struct task_struct *t = task_create(tasks[i].fn, tasks[i].prio,
						    tasks[i].name);
		if (t)
			enqueue_task(&runqueue, t);
		else
			printk("[BAND] could not create %s\n", tasks[i].name);
	}

	/*
	 * Prove the duplicate check fires, by tripping it deliberately.
	 *
	 * sched_claim_prio() panics when two tasks want one deadline level, and
	 * until it has done so that is a claim about code that has never run.
	 * The machine stops here on purpose: the panic *is* the pass condition,
	 * so this is a separate setting rather than something left switched on.
	 */
#if CONFIG_SCHED_BAND_TEST == 2
	printk("[BAND] claiming prio %d twice on purpose"
	       " — a panic naming both tasks is the pass\n", PRIO_BAND_HI);
	(void)task_create(bandtest_hi, PRIO_BAND_HI, "band-dup");
	printk("[BAND] FAIL: duplicate claim was accepted\n");
#endif
#endif /* CONFIG_SCHED_BAND_TEST */
}
