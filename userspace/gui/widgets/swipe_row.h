#ifndef __GUI_SWIPE_ROW_H
#define __GUI_SWIPE_ROW_H

#include "lvgl/lvgl.h"

/*
 * swipe_row — an iOS-style swipe-to-delete list row.
 *
 * A row is a cell holding a red delete button (behind, pinned right) and an
 * opaque foreground the caller fills with content. Dragging the foreground
 * left reveals the delete button; a plain tap fires the row's tap callback,
 * and vertical drags fall through to the list's own scrolling untouched.
 *
 * Only one finger and one open row exist at a time, so the drag/open state is
 * shared (file-static) — no per-row allocation. Shared by the Messages and
 * Recents lists so both behave identically.
 *
 * Usage per screen:
 *   populate():
 *     swipe_row_reset(on_tap);                 // clear state + set tap handler
 *     for each item:
 *       lv_obj_t *del;
 *       lv_obj_t *fg = swipe_row_create(list, ROW_H, user, &del);
 *       lv_obj_add_event_cb(del, on_delete, LV_EVENT_CLICKED, user);
 *       ... fill fg (flex, avatar, labels) ...
 *   create(): attach swipe_row_scroll_begin_cb to the list SCROLL_BEGIN,
 *             and call swipe_row_reset(NULL) on screen delete.
 *
 * The caller wires its own delete handler on `del` (typically an
 * lv_async_call so the list can be rebuilt safely after the event returns).
 */

typedef void (*swipe_row_tap_cb)(void *user);

/* Clear the open/drag state (rows are about to be freed) and set the tap
 * handler for rows created afterwards. Pass NULL when tearing a screen down. */
void swipe_row_reset(swipe_row_tap_cb on_tap);

/* Build a swipe row inside `list` (a vertical flex list). Returns the opaque
 * foreground container for the caller to fill; the caller sets its own flex
 * layout and padding. `user` is passed back to the tap callback and is the
 * value the caller should also hand to its delete handler. `del_btn_out`, if
 * non-NULL, receives the delete button so the caller can wire its click. */
lv_obj_t *swipe_row_create(lv_obj_t *list, int height, void *user,
			   lv_obj_t **del_btn_out);

/* Attach to the list's LV_EVENT_SCROLL_BEGIN to close an open row on scroll. */
void swipe_row_scroll_begin_cb(lv_event_t *e);

#endif
