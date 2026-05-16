/*
 * include/boot_screen.h — Boot screen interface
 */

#ifndef BOOT_SCREEN_H
#define BOOT_SCREEN_H

/**
 * Run full boot screen sequence:
 * 1. Boot log with [ OK ] status lines
 * 2. Splash "NOTHAN OS" with subtitle + loading animation
 *
 * Must be called after fb_init().
 * Blocks until animation completes, then returns.
 */
void boot_screen_run(void);

#endif /* BOOT_SCREEN_H */
