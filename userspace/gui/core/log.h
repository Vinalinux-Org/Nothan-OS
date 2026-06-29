#ifndef __GUI_LOG_H
#define __GUI_LOG_H

/*
 * GUI event logging over UART — the only debug channel on hardware (no
 * JTAG). Every line is prefixed "[GUI] " to match the kernel log style.
 * Used to trace navigation, screen lifecycle, the demo tour, and every
 * widget event (the event handlers are wired but inert until an input
 * device lands, so these are how we see them fire).
 */

/* Log a constant string (should end with '\n'). */
void gui_log(const char *msg);

/* Log a printf-style formatted line ("[GUI] " is prepended). */
void gui_logf(const char *fmt, ...);

/* Show a brief on-screen message on lv_layer_top that disappears after 2s. */
void gui_toast(const char *msg);

#endif
