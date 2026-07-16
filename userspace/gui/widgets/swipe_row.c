/*
 * widgets/swipe_row.c - iOS-style swipe-to-delete list row
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "swipe_row.h"
#include "../theme/theme.h"

#define DEL_W      88	/* width of the red delete panel revealed on swipe-left */
#define AXIS_LOCK  8	/* px of travel before a drag commits to an axis */
#define SLIDE_MS   140	/* snap-open / snap-close animation duration */

/* One finger, one open row at a time → a handful of file-statics is enough.
 *   drag_fg   : the foreground being dragged this press (NULL = none)
 *   drag_start: press point, to measure travel
 *   drag_base : the fg's translate_x at press start (0 or -DEL_W)
 *   drag_axis : 0 undecided, 1 horizontal (we slide), 2 vertical (list scrolls)
 *   drag_moved: a horizontal slide happened, so the release must NOT tap
 *   open_fg   : the row currently snapped open (revealing delete), or NULL
 *   s_on_tap  : tap handler for the rows of the current screen */
static lv_obj_t         *drag_fg;
static lv_point_t        drag_start;
static int32_t           drag_base;
static int               drag_axis;
static int               drag_moved;
static lv_obj_t         *open_fg;
static swipe_row_tap_cb  s_on_tap;

void swipe_row_reset(swipe_row_tap_cb on_tap)
{
	drag_fg  = NULL;
	open_fg  = NULL;
	s_on_tap = on_tap;
}

static void slide_anim_cb(void *var, int32_t v)
{
	lv_obj_set_style_translate_x((lv_obj_t *)var, v, 0);
}

/* Animate a foreground to `to` (0 = closed, -DEL_W = open). translate_x moves
 * the object's real coords (not just its draw), so the delete button under the
 * shifted foreground stays hit-testable. */
static void slide_to(lv_obj_t *fg, int32_t to)
{
	int32_t from = lv_obj_get_style_translate_x(fg, LV_PART_MAIN);
	if (from == to) {
		return;
	}
	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, fg);
	lv_anim_set_exec_cb(&a, slide_anim_cb);
	lv_anim_set_values(&a, from, to);
	lv_anim_set_duration(&a, SLIDE_MS);
	lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
	lv_anim_start(&a);
}

static void close_row(lv_obj_t *fg)
{
	slide_to(fg, 0);
	if (open_fg == fg) {
		open_fg = NULL;
	}
}

static void open_row(lv_obj_t *fg)
{
	slide_to(fg, -DEL_W);
	open_fg = fg;
}

void swipe_row_scroll_begin_cb(lv_event_t *e)
{
	(void)e;
	if (open_fg) {
		close_row(open_fg);
	}
}

/* Foreground handler: owns the horizontal drag, the snap on release, and the
 * tap. Vertical drags fall through to the list's scrolling. */
static void on_fg_event(lv_event_t *e)
{
	lv_obj_t       *fg    = lv_event_get_target(e);
	void           *user  = lv_event_get_user_data(e);
	lv_event_code_t code  = lv_event_get_code(e);
	lv_indev_t     *indev = lv_indev_active();

	switch (code) {
	case LV_EVENT_PRESSED:
		lv_anim_delete(fg, slide_anim_cb);   /* stop any in-flight snap */
		if (open_fg && open_fg != fg) {
			close_row(open_fg);          /* only one row open at a time */
		}
		if (indev) {
			lv_indev_get_point(indev, &drag_start);
		}
		drag_base  = lv_obj_get_style_translate_x(fg, LV_PART_MAIN);
		drag_fg    = fg;
		drag_axis  = 0;
		drag_moved = 0;
		break;

	case LV_EVENT_PRESSING: {
		if (drag_fg != fg || !indev) {
			break;
		}
		lv_point_t p;
		lv_indev_get_point(indev, &p);
		int32_t dx = p.x - drag_start.x;
		int32_t dy = p.y - drag_start.y;
		if (drag_axis == 0) {
			if (LV_ABS(dx) > AXIS_LOCK && LV_ABS(dx) >= LV_ABS(dy)) {
				drag_axis = 1;
			} else if (LV_ABS(dy) > AXIS_LOCK) {
				drag_axis = 2;   /* let the list scroll vertically */
			}
		}
		if (drag_axis == 1) {
			int32_t off = drag_base + dx;
			if (off > 0)      off = 0;
			if (off < -DEL_W) off = -DEL_W;
			lv_obj_set_style_translate_x(fg, off, 0);
			drag_moved = 1;
		}
		break;
	}

	case LV_EVENT_RELEASED:
	case LV_EVENT_PRESS_LOST:
		if (drag_fg == fg && drag_axis == 1) {
			int32_t off = lv_obj_get_style_translate_x(fg, LV_PART_MAIN);
			if (off < -DEL_W / 2) {
				open_row(fg);
			} else {
				close_row(fg);
			}
		}
		drag_fg = NULL;
		break;

	case LV_EVENT_CLICKED:
		if (drag_moved) {           /* was a swipe, not a tap — swallow */
			drag_moved = 0;
			break;
		}
		if (open_fg == fg) {        /* tap on an open row just closes it */
			close_row(fg);
			break;
		}
		if (s_on_tap) {
			s_on_tap(user);
		}
		break;

	default:
		break;
	}
}

/* Drop dangling references if a row is freed while it is the drag/open one. */
static void on_fg_delete(lv_event_t *e)
{
	lv_obj_t *fg = lv_event_get_target(e);
	if (drag_fg == fg) drag_fg = NULL;
	if (open_fg == fg) open_fg = NULL;
}

static const lv_event_code_t fg_events[] = {
	LV_EVENT_PRESSED, LV_EVENT_PRESSING, LV_EVENT_RELEASED,
	LV_EVENT_PRESS_LOST, LV_EVENT_CLICKED,
};

lv_obj_t *swipe_row_create(lv_obj_t *list, int height, void *user,
			   lv_obj_t **del_btn_out)
{
	/* Cell = swipe container: clips its children (default), holds the delete
	 * panel behind and the sliding foreground on top. */
	lv_obj_t *cell = lv_obj_create(list);
	lv_obj_remove_style_all(cell);
	lv_obj_set_size(cell, lv_pct(100), height);
	lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

	/* Delete panel — created first so it sits BEHIND the foreground. */
	lv_obj_t *del = lv_button_create(cell);
	lv_obj_remove_style_all(del);
	lv_obj_set_size(del, DEL_W, lv_pct(100));
	lv_obj_align(del, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_set_style_radius(del, RADIUS_MD, 0);
	lv_obj_set_style_bg_opa(del, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(del, lv_color_hex(0xEF4444), 0);
	lv_obj_set_style_bg_color(del, lv_color_hex(0xDC2626), LV_STATE_PRESSED);
	lv_obj_clear_flag(del, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *trash = lv_label_create(del);
	lv_label_set_text(trash, LV_SYMBOL_TRASH);
	lv_obj_set_style_text_color(trash, theme_color(THEME_TEXT), 0);
	lv_obj_set_style_text_font(trash, &lv_font_montserrat_20, 0);
	lv_obj_center(trash);

	/* Foreground — opaque (same as the screen bg) so it hides the delete
	 * panel until swiped. The caller adds flow/padding/content. */
	lv_obj_t *fg = lv_button_create(cell);
	lv_obj_remove_style_all(fg);
	lv_obj_set_size(fg, lv_pct(100), lv_pct(100));
	lv_obj_set_style_radius(fg, RADIUS_MD, 0);
	lv_obj_set_style_bg_color(fg, theme_color(THEME_BG), 0);
	lv_obj_set_style_bg_opa(fg, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(fg, theme_color(THEME_TEXT), LV_STATE_PRESSED);
	lv_obj_set_style_bg_opa(fg, LV_OPA_10, LV_STATE_PRESSED);
	lv_obj_clear_flag(fg, LV_OBJ_FLAG_SCROLLABLE);
	for (int i = 0; i < (int)(sizeof(fg_events) / sizeof(fg_events[0])); i++) {
		lv_obj_add_event_cb(fg, on_fg_event, fg_events[i], user);
	}
	lv_obj_add_event_cb(fg, on_fg_delete, LV_EVENT_DELETE, NULL);

	if (del_btn_out) {
		*del_btn_out = del;
	}
	return fg;
}
