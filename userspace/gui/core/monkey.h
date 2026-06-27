#ifndef _GUI_MONKEY_H
#define _GUI_MONKEY_H

#include <stdint.h>

/*
 * On-target soak-test input generator (build: make MONKEY=1).
 *
 * Replaces the /dev/input0 touch source with a stream of randomized but
 * SEEDED user gestures so the GUI drives itself across every screen and widget
 * on the real target build — the only place the arm-none-eabi -O2 LVGL
 * miscompiles reproduce (no host config does). The seed is printed at start
 * and every gesture is logged with a step counter, so a crash's UART tail
 * shows the exact path and the same seed replays it bit-for-bit.
 */
void monkey_init(uint32_t seed);

/*
 * Advance the gesture state machine by one input sample and output the current
 * pointer in LOGICAL portrait coordinates (x: 0..479, y: 0..799) plus the
 * pressed state. The indev port rotates this to the panel's physical space.
 */
void monkey_read(int *lx, int *ly, int *pressed);

#endif /* _GUI_MONKEY_H */
