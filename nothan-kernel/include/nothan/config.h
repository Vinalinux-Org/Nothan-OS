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
 * Off through the kernel foundation work, and not only to skip drawing: LCDC
 * raises a frame-done interrupt continuously, while Phase 3 measures
 * wakeup-to-run latency in microseconds.  A periodic interrupt landing in the
 * middle of those measurements is exactly what makes a result unreproducible.
 *
 * On now, because the receiving half of a call has to put its frames somewhere.
 * That brings back the interrupt this comment warns about, and adds a raster
 * DMA reading 768 KB out of DDR sixty times a second, both of them landing on
 * a network path that has never run beside either.  Nothing has measured that
 * yet; the sink's two independent counters and the CPSW overrun registers are
 * what will say whether it costs anything.
 *
 * Turn it off to get the old measurement conditions back.
 */
#define CONFIG_VIDEO		1

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
 * CPSW Ethernet (port 1, LAN8710A over MII).
 *
 * The one link on this board that can carry a video call: 320x240 RGB565 at
 * 10 fps is 12.3 Mbit/s, against 0.09 Mbit/s for the SIM7600 over its UART.
 * That module is for voice and SMS, which is what phone_daemon uses it for.
 *
 * On: the PHY is on the board and answers, so this is real hardware rather
 * than a driver waiting for something absent.  Everything above the netdev
 * seam is link-agnostic, so choosing Ethernet here is not a choice about the
 * product — it is the only link this dev board offers at that bandwidth.
 */
#define CONFIG_ETHERNET		1

/*
 * Video over the network: the capture seam, a generated stand-in for the
 * camera, and the task that frames captures onto the wire.
 *
 * Separate from CONFIG_VIDEO, which is the display side — the LCDC, the
 * framebuffer and the HDMI framer.  The two halves of a call are independent
 * and it is useful to run either alone: sending needs no panel, and a box
 * showing a remote picture needs no camera.  Needs CONFIG_ETHERNET.
 */
#define CONFIG_VIDEO_STREAM	1

/*
 * The throughput benchmarks: the discard sink on port 9, the blaster on
 * port 19, the echo on port 7.
 *
 * They are how every number this project has about the link was obtained, and
 * they are also three tasks holding two deadline levels in a band four wide.
 * Kept on because the measurements are not finished; the switch exists so that
 * the day the video path wants those levels, taking them is one line rather
 * than an argument.
 */
#define CONFIG_NET_BENCH	1

/*
 * Send ARP requests at boot, to check that a transmitted frame is well formed.
 *
 * Not a network stack and not the start of one.  It asks the machine on the
 * other end of the cable for an address that machine actually holds, and a
 * real operating system answering is the judge: the reply proves the frame
 * left the port, that a real stack found it valid, and that it came back
 * addressed to this board's own MAC — the only check so far that exercises
 * the address programmed into the port rather than broadcast.
 *
 * Addresses are hard-coded to the shared link the dev laptop uses; change them
 * in cpsw.c if that link changes.  Off unless a transmit path is being tested.
 */
#define CONFIG_NET_ARP_PROBE	0

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
 * Measure how long interrupts stay masked, and where the worst region began.
 *
 * The last thing in this kernel that can miss a deadline leaving no trace.
 * Priority does not govern a masked region — a task holding the mask is not
 * preempted by anything, at any band.  The scheduler accounting cannot see it,
 * because no switch happens.  CONFIG_IRQ_TIMING cannot, because it is not a
 * handler.  asm/irqflags.h states the rule — critical sections short and
 * bounded, no unbounded loops, no printk — and until this runs, that is a hope.
 *
 * Budget: 100 us, one percent of the 10 ms audio period.
 *
 * A measurement mode: two clocksource reads per outermost critical section,
 * which is a lot of critical sections.  Turn it on, read the worst case and
 * the address that produced it, look the address up in build/kernel.map, act,
 * turn it off.
 */
#define CONFIG_IRQ_OFF_TIMING	0

/*
 * Lock-free ISR-to-task ring acceptance test (os-architecture.md §3.3).
 *
 * nothan/ring.h claims one producer and one consumer need no lock, because
 * each index has a single writer.  That is an argument about memory ordering
 * on a machine with a store buffer, and such arguments read as obviously
 * correct while being wrong — the §1 cell where a log cannot help.
 *
 * So it runs in the shape real users will have: the producer is the timer
 * interrupt, the consumer an ordinary BG task, nothing masked.  The producer
 * advances its sequence only on a successful put, so the consumer must see
 * every value exactly once and in order; a gap, repeat or reordering panics
 * naming expected and actual.  A momentarily full ring is an ordinary
 * condition, counted separately so a contended run can be told from a quiet
 * one.
 */
#define CONFIG_RING_TEST	0

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

#if CONFIG_VIDEO_STREAM && !CONFIG_ETHERNET
#error "CONFIG_VIDEO_STREAM requires CONFIG_ETHERNET"
#endif

#endif /* _NOTHAN_CONFIG_H */
