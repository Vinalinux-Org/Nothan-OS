/*
 * include/boot_screen.h — Boot screen interface
 */

#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

/**
 * Run full boot screen sequence:
 * 1. Boot log with [ OK ] status lines
 * 2. Splash "VINIX OS" with subtitle + loading animation
 *
 * Must be called after fb_init().
 * Blocks until animation completes, then returns.
 */
void boot_screen_run(void);

/* Redraw the home screen (icon grid + status bar).
 * Called by home_ui when a panel is closed. */
void show_home(void);

#endif /* BOOT_SCREEN_H */
