#ifndef _NOTHAN_CONFIG_H
#define _NOTHAN_CONFIG_H

/*
 * Build-time feature switches.
 *
 * NothanOS targets a device whose app set is fixed and known at build time
 * (Documentation/future-applications.md §PC).  This header is where that set
 * is declared, so "what runs on this box" is one file to read rather than a
 * hunt through spawn calls and initcalls.
 *
 * Current bring-up state (roadmap Phase 0-3): the board is bare.  None of the
 * My Nuong peripherals are attached — no SIM7600 modem, no HDMI panel, no USB
 * touchscreen.  Only the console UART and the SD card are real, so everything
 * else is off.  A driver waiting on hardware that is not there costs more than
 * log noise: it retries and times out, and those delays land inside the timing
 * measurements this phase exists to make.
 *
 * Scope note: these switches gate *execution* — a disabled subsystem's initcall
 * returns immediately and its task is never spawned, but its object is still
 * linked into the image.  Dropping it from the link as well belongs with the
 * allocation-ceiling work in Documentation/kernel-roadmap.md §7.
 *
 * Nothing here deletes code.  Every switch is reversible by changing the
 * number, and both settings of each are expected to build clean.
 */

/*
 * Video stack: LCDC controller, framebuffer, TDA19988 HDMI framer.
 *
 * Off during kernel foundation work, and not only to skip drawing: LCDC raises
 * a frame-done interrupt continuously, while Phase 3 measures wakeup-to-run
 * latency in microseconds.  A periodic interrupt landing in the middle of those
 * measurements is exactly what makes a result unreproducible.
 */
#define CONFIG_VIDEO		0

/* LVGL GUI (userspace).  Needs the video stack. */
#define CONFIG_GUI		0

/*
 * SIM7600 modem: the UART1 platform device, the phonebus character devices,
 * and the phone_daemon task that drives them.
 */
#define CONFIG_MODEM		0

/* USB host controller (MUSB) and the touchscreen input device behind it. */
#define CONFIG_USB_TOUCH	0

/* Interactive shell on the console UART.  Needs no peripheral. */
#define CONFIG_SHELL		1

/*
 * FAT write backend, keeping slow SD writes off the foreground task.
 * The SD card is genuinely present, so this stays on.
 */
#define CONFIG_STORAGE_DAEMON	1

/*
 * Boot-time cache hierarchy measurement (roadmap Phase 0.3).
 *
 * Question answered — the 256 KB L2 is enabled and working, knees measured at
 * exactly 32 KB and 256 KB — so this is off by default.  Costs ~100 ms of boot
 * and borrows 4 MB while it runs.
 *
 * Worth switching back on after any change to clocks, voltage, or the memory
 * map.  It turned out to be a better machine-integrity canary than a
 * benchmark: the chain-length check inside it detects a CPU that is computing
 * wrong answers, which is how the undervolt in kernel-roadmap.md §1.1.1 was
 * finally pinned down after the timing numbers alone had misled us twice.
 */
#define CONFIG_CACHE_BENCH	0

/*
 * Concurrency stress tasks (roadmap Phase 1 acceptance).
 *
 * Three kernel tasks hammering the console and both allocators while the tick
 * cuts between them.  This is the only thing in the tree that reliably puts
 * the kernel in the state a race needs, so turn it on whenever the locking
 * around a shared structure changes — booting and typing ls proves very
 * little about mutual exclusion.
 */
#define CONFIG_STRESS_TEST	0

/*
 * Priority-band acceptance test (roadmap Phase 3 §5.2).
 *
 * Four tasks that busy-wait and print: two on exclusive deadline levels, two
 * sharing the BG level.  The two scheduling rules produce opposite output from
 * the same shape of task, so one glance at the log tells them apart — and the
 * expected string is printed before they start, so the answer is known in
 * advance rather than inferred from whatever appears.
 *
 * Boot alone cannot check this.  It reports the priorities that were assigned,
 * which says what was intended, not what the scheduler does with them.
 *
 * 0 = off
 * 1 = run the four tasks
 * 2 = also claim one deadline level twice, which must panic naming both tasks.
 *     The panic is the pass condition, so the machine stops — hence a separate
 *     setting rather than something left on.
 * 3 = run the four tasks, then panic once they finish, so the context-switch
 *     ring (roadmap §5.4) can be read against a sequence known in advance.
 *     Nothing else in the tree produces a switch order that is predictable
 *     enough to check a post-mortem log against.
 */
#define CONFIG_SCHED_BAND_TEST	0

/*
 * Time every interrupt handler (roadmap §9.2, and the measurement §5.3 needs).
 *
 * §9.2 states that priority controls tasks while nothing controls interrupts:
 * a handler doing real work steals from the highest-priority task in the system
 * and no scheduling decision can see it happen.  It requires ISRs to be short
 * and bounded — which is a hope until there is a number.
 *
 * Immediate use is §5.3.  Taking the tick from 10 ms to 1 ms multiplies the
 * timer interrupt rate tenfold, and the cost of that cannot be read off the
 * scheduler accounting: an ISR does not switch tasks, so its time is charged to
 * whoever it interrupted.  An idle task interrupted a thousand times a second
 * looks exactly like an idle task left alone.
 *
 * A measurement mode rather than something left on, unlike the scheduler
 * accounting.  That one runs per context switch; this runs per interrupt, on
 * the shortest latency path in the machine, ahead of the handler that wakes a
 * task.  Two clocksource reads is small but not free.
 */
#define CONFIG_IRQ_TIMING	0

/*
 * Deliberately smash a kernel stack guard to exercise the overflow check.
 *
 * Same reasoning as CONFIG_PANIC_TEST below: a check that has never fired is a
 * guess.  This one overwrites the guard at the bottom of the *current* task's
 * stack — kernel_main runs in the idle task's context — and then schedules, so
 * the very next switch must panic naming "idle" and printing 0 where
 * 0x5AFEC0DE belongs.
 *
 * Writing the guard directly rather than actually overflowing: a real overflow
 * would have to run off the stack, which on the way there corrupts the
 * neighbouring allocation and makes the rest of the boot unpredictable.  The
 * question here is whether the guard is armed and checked, and that is
 * answered without the collateral damage.
 *
 * The panic is the pass condition, so the machine stops.
 */
#define CONFIG_STACK_CANARY_TEST	0

/*
 * Deliberately crash at the end of boot to exercise the panic path.
 *
 * A crash handler that has never crashed is not a handler, it is a guess.  The
 * fault it triggers is not arbitrary either: it reads a kernel address with
 * bit 30 cleared, which is exactly the corruption an undervolted CPU produced
 * for a whole day of this project before anything could say so.  If the dump
 * names that bit, the machine can now explain in one line what took hours.
 */
#define CONFIG_PANIC_TEST	0

/*
 * Measure how long a task waits between being woken and actually running.
 *
 * The number Phase 3 exists to produce.  It costs one clocksource read per
 * wakeup and one per context switch, so it is a measurement mode rather than
 * something to leave on: turn it on to get a figure, act on it, turn it off.
 *
 * Only meaningful now that tasks genuinely sleep — before blocking reads there
 * were no wakeups to time.
 *
 * Answer obtained (roadmap §5.1.1): 13-19 cycles, max 19 over 40 wakeups, so
 * 540-790 ns from the UART RX interrupt to the shell running.  Re-measured at
 * the 1 ms tick (§5.3.1): max 20 over 32, one cycle apart, no tail.  Off again.
 * Worth switching back on after any change to the wake path, the tick, or
 * priority assignment — those are the things that can put a tail on it.
 *
 * Never on at the same time as CONFIG_IRQ_TIMING.  That one reads the
 * clocksource after the handler returns, which lands inside the interval this
 * one is timing, because wake_ts is stamped within that handler.  Running both
 * cost a measurement round and nearly cost a wrong conclusion about the tick.
 */
#define CONFIG_SCHED_LATENCY	0

#if CONFIG_GUI && !CONFIG_VIDEO
#error "CONFIG_GUI requires CONFIG_VIDEO"
#endif

#endif /* _NOTHAN_CONFIG_H */
