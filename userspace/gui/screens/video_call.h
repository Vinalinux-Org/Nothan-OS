#ifndef __GUI_VIDEO_CALL_H
#define __GUI_VIDEO_CALL_H

#include "lvgl/lvgl.h"

/* The in-call screen. @arg is the peer index, cast through (void *)(long). */
void video_call_create(lv_obj_t *screen, void *arg);

/*
 * Put the in-call screen up for an invite that just arrived.
 *
 * Wired to call.c's ring handler at startup.  Lives here rather than in main.c
 * because it is a decision about screens, and main.c should not have to know
 * which one a ringing call belongs on.
 */
void video_call_ring(int peer_idx);

#endif
