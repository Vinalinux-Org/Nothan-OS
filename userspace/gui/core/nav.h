#ifndef __GUI_NAV_H
#define __GUI_NAV_H

#include "lvgl/lvgl.h"

/*
 * Phone-style navigation stack — see Documentation/01-gui-platform.md.
 *
 * One full-screen app screen is shown at a time. Each screen is an LVGL
 * screen object built by a nav_builder_fn into the empty screen it is
 * handed; the optional @arg passes context (e.g. which contact to show).
 *
 * The system nav bar (Back / Home / Recents) lives on the display top
 * layer, so it stays fixed while screens slide underneath. Back pops,
 * Home returns to the root. There is no input device yet (HDMI output),
 * so those keys are inert today — the demo drives transitions directly —
 * but the wiring is complete and lands the moment touch/keypad arrives.
 */
typedef void (*nav_builder_fn)(lv_obj_t *screen, void *arg);

/* Mount the nav bar (hidden) on the top layer. Call once at startup. */
void nav_init(void);

/* Show/hide the system nav bar — hidden during boot, shown on home. */
void nav_show_chrome(bool show);

/* Replace the whole stack with a fresh root (boot -> home). */
void nav_set_root(nav_builder_fn builder, void *arg);

/* Push a new screen on top (slide in from the right). */
void nav_push(nav_builder_fn builder, void *arg);

/* Pop the top screen (slide back to the right). No-op at the root. */
void nav_pop(void);

/* Collapse back to the root screen, discarding everything above it. */
void nav_to_root(void);

#endif
