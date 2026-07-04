#ifndef __GUI_CALL_UI_H
#define __GUI_CALL_UI_H

/*
 * Call overlay — a system-level call screen on lv_layer_top(), above every
 * app and the nav bar. It observes the telephony state machine and renders
 * the incoming / in-call UI, or hides itself when idle. Because it lives on
 * the top layer it is independent of the navigation stack: a call can arrive
 * over any screen without disturbing where the user was.
 */

/* Create the (hidden) overlay and subscribe to telephony. Call once at
 * startup, after nav_init() and telephony_init(). */
void call_ui_init(void);
void call_ui_on_boot_done(void);

#endif
