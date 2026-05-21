/*
 * include/home_ui.h — Home screen UI task
 */

#ifndef HOME_UI_H
#define HOME_UI_H

/* Kernel task entry point — runs as a scheduled task after boot.
 * Polls cursor position and button state; draws app panels on icon click. */
void home_ui_run(void);

#endif /* HOME_UI_H */
