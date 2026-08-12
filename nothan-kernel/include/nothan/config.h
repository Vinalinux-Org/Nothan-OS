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

#if CONFIG_GUI && !CONFIG_VIDEO
#error "CONFIG_GUI requires CONFIG_VIDEO"
#endif

#endif /* _NOTHAN_CONFIG_H */
