/*
 * drivers/ui/ui_lock.c — Lock screen for Nothan-OS (800×480)
 *
 * Layout:
 *   Top bar     y=0   h=40   status bar
 *   Center      y=40  h=360  clock, notifications, quick actions
 *   Unlock area y=400 h=80   swipe hint + home indicator
 *
 * Click on unlock area → transition to login.
 */

#include "lvgl/lvgl.h"
#include "vinix/printk.h"
#include "screen_transition.h"

lv_obj_t *ui_login_create(void);

static lv_obj_t *s_time_lbl;
static lv_obj_t *s_flash_btn;
static lv_obj_t *s_flash_lbl;
static lv_obj_t *s_flash_canvas;
static bool      s_flash_on;

static uint8_t s_h = 19, s_m = 20;
static lv_timer_t *s_clock_timer;

static lv_color_t s_buf_flash[LV_CANVAS_BUF_SIZE_TRUE_COLOR(20, 20)];
static lv_color_t s_buf_cam[LV_CANVAS_BUF_SIZE_TRUE_COLOR(20, 20)];
static lv_color_t s_buf_vol[LV_CANVAS_BUF_SIZE_TRUE_COLOR(20, 20)];

/* ── Clock timer ─────────────────────────────────────────────────────── */

static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (++s_m >= 60) { s_m = 0; if (++s_h >= 24) s_h = 0; }
    if (s_time_lbl) {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)s_h, (unsigned)s_m);
        lv_label_set_text(s_time_lbl, buf);
    }
}

static void to_login_cb(lv_timer_t *t)
{
    (void)t;
    pr_info("[UI] Lock -> login\n");
    transition_to(ui_login_create(), 1);
}

static void unlock_click_cb(lv_event_t *e)
{
    (void)e;
    to_login_cb(NULL);
}

/* ── Canvas icon helpers ─────────────────────────────────────────────── */

static void draw_flashlight(lv_obj_t *cv, lv_color_t col)
{
    lv_canvas_fill_bg(cv, lv_color_hex(0x000000), LV_OPA_TRANSP);
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_color = col; rd.bg_opa = LV_OPA_COVER; rd.border_width = 0; rd.radius = 2;
    lv_canvas_draw_rect(cv, 7, 3, 6, 10, &rd);
    lv_canvas_draw_rect(cv, 6, 13, 8, 3, &rd);
    lv_draw_line_dsc_t ld; lv_draw_line_dsc_init(&ld);
    ld.color = col; ld.opa = LV_OPA_COVER; ld.width = 2;
    ld.round_start = 1; ld.round_end = 1;
    lv_point_t up[] = {{10,3},{10,0}}; lv_canvas_draw_line(cv, up, 2, &ld);
    lv_point_t tl[] = {{7,3},{4,0}};   lv_canvas_draw_line(cv, tl, 2, &ld);
    lv_point_t tr[] = {{13,3},{16,0}}; lv_canvas_draw_line(cv, tr, 2, &ld);
}

static void draw_camera(lv_obj_t *cv)
{
    lv_canvas_fill_bg(cv, lv_color_hex(0x000000), LV_OPA_TRANSP);
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_opa = LV_OPA_TRANSP; rd.border_color = lv_color_white();
    rd.border_opa = LV_OPA_COVER; rd.border_width = 2; rd.radius = 2;
    rd.bg_color = lv_color_hex(0x000000);
    lv_canvas_draw_rect(cv, 1, 4, 18, 13, &rd);
    rd.radius = LV_RADIUS_CIRCLE; rd.border_width = 0;
    rd.bg_color = lv_color_white(); rd.bg_opa = LV_OPA_COVER;
    rd.border_width = 2; rd.border_color = lv_color_white();
    lv_canvas_draw_rect(cv, 5, 6, 10, 10, &rd);
    rd.radius = 0; rd.border_width = 0;
    rd.bg_color = lv_color_hex(0x000000); rd.bg_opa = LV_OPA_COVER;
    lv_canvas_draw_rect(cv, 7, 8, 6, 6, &rd);
    rd.bg_color = lv_color_white(); rd.radius = 0;
    lv_canvas_draw_rect(cv, 13, 1, 4, 3, &rd);
}

static void draw_volume(lv_obj_t *cv)
{
    lv_canvas_fill_bg(cv, lv_color_hex(0x000000), LV_OPA_TRANSP);
    lv_draw_rect_dsc_t rd; lv_draw_rect_dsc_init(&rd);
    rd.bg_color = lv_color_white(); rd.bg_opa = LV_OPA_COVER;
    rd.border_width = 0; rd.radius = 0;
    lv_canvas_draw_rect(cv, 1, 7, 3, 6, &rd);
    lv_canvas_draw_rect(cv, 4, 5, 4, 10, &rd);
    lv_draw_arc_dsc_t ad; lv_draw_arc_dsc_init(&ad);
    ad.color = lv_color_white(); ad.opa = LV_OPA_COVER; ad.width = 2;
    lv_canvas_draw_arc(cv, 8, 10, 5, -50, 50, &ad);
    lv_canvas_draw_arc(cv, 8, 10, 8, -50, 50, &ad);
}

static void flash_toggle_cb(lv_event_t *e)
{
    (void)e;
    s_flash_on = !s_flash_on;
    if (s_flash_on) {
        lv_obj_set_style_bg_color(s_flash_btn, lv_color_hex(0xffd84a), 0);
        lv_obj_set_style_bg_opa(s_flash_btn, 46, 0);
        lv_obj_set_style_border_color(s_flash_btn, lv_color_hex(0xffd84a), 0);
        lv_obj_set_style_border_opa(s_flash_btn, 128, 0);
        if (s_flash_lbl) lv_label_set_text(s_flash_lbl, "On");
        draw_flashlight(s_flash_canvas, lv_color_hex(0xffd84a));
    } else {
        lv_obj_set_style_bg_color(s_flash_btn, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(s_flash_btn, 20, 0);
        lv_obj_set_style_border_color(s_flash_btn, lv_color_white(), 0);
        lv_obj_set_style_border_opa(s_flash_btn, 38, 0);
        if (s_flash_lbl) lv_label_set_text(s_flash_lbl, "Flash");
        draw_flashlight(s_flash_canvas, lv_color_white());
    }
}

/* ── Top bar (h=40) ──────────────────────────────────────────────────── */

static void make_top_bar(lv_obj_t *scr)
{
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 800, 40);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0d0a1a), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = lv_label_create(bar);
    lv_label_set_text(brand, "NOTHAN-OS");
    lv_obj_set_style_text_color(brand, lv_color_white(), 0);
    lv_obj_set_style_text_opa(brand, 64, 0);
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(brand, 1, 0);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 14, 0);

    lv_obj_t *pct = lv_label_create(bar);
    lv_label_set_text(pct, "78%");
    lv_obj_set_style_text_color(pct, lv_color_white(), 0);
    lv_obj_set_style_text_opa(pct, 89, 0);
    lv_obj_set_style_text_font(pct, &lv_font_montserrat_14, 0);
    lv_obj_align(pct, LV_ALIGN_RIGHT_MID, -14, 0);

    lv_obj_t *ic_bat = lv_label_create(bar);
    lv_label_set_text(ic_bat, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(ic_bat, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_bat, 89, 0);
    lv_obj_align_to(ic_bat, pct, LV_ALIGN_OUT_LEFT_MID, -6, 0);

    lv_obj_t *ic_wifi = lv_label_create(bar);
    lv_label_set_text(ic_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(ic_wifi, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_wifi, 89, 0);
    lv_obj_align_to(ic_wifi, ic_bat, LV_ALIGN_OUT_LEFT_MID, -10, 0);
}

/* ── Center area (y=40, h=360) ───────────────────────────────────────── */

static void make_center(lv_obj_t *scr)
{
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 800, 360);
    lv_obj_set_pos(cont, 0, 40);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 10, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Clock */
    s_time_lbl = lv_label_create(cont);
    lv_label_set_text(s_time_lbl, "19:20");
    lv_obj_set_style_text_color(s_time_lbl, lv_color_white(), 0);
#if LV_FONT_MONTSERRAT_48
    lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_48, 0);
#else
    lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_28, 0);
#endif

    lv_obj_t *date_lbl = lv_label_create(cont);
    lv_label_set_text(date_lbl, "Sunday, May 17");
    lv_obj_set_style_text_color(date_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_opa(date_lbl, 140, 0);
    lv_obj_set_style_text_font(date_lbl, &lv_font_montserrat_14, 0);

    /* Notification cards — 2 items, width 340 (slightly narrower for 480) */
    lv_obj_t *notif_col = lv_obj_create(cont);
    lv_obj_remove_style_all(notif_col);
    lv_obj_set_size(notif_col, 340, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(notif_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(notif_col, 5, 0);
    lv_obj_set_scrollbar_mode(notif_col, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(notif_col, LV_OBJ_FLAG_SCROLLABLE);

    static const uint32_t    card_bg[2]  = { 0x7c3aed, 0xe05c2a };
    static const char *const card_sym[2] = { LV_SYMBOL_OK, LV_SYMBOL_BELL };
    static const char *const card_app[2] = { "SYSTEM", "REMINDER" };
    static const char *const card_msg[2] = {
        "Build completed successfully", "BBB deployment check at 18:00"
    };
    static const char *const card_tm[2] = { "2m", "5m" };

    for (int i = 0; i < 2; i++) {
        lv_obj_t *card = lv_obj_create(notif_col);
        lv_obj_remove_style_all(card);
        lv_obj_set_size(card, 340, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(card, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(card, 18, 0);
        lv_obj_set_style_border_color(card, lv_color_white(), 0);
        lv_obj_set_style_border_opa(card, 26, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_all(card, 8, 0);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(card, 8, 0);
        lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ico_box = lv_obj_create(card);
        lv_obj_remove_style_all(ico_box);
        lv_obj_set_size(ico_box, 26, 26);
        lv_obj_set_style_radius(ico_box, 6, 0);
        lv_obj_set_style_bg_color(ico_box, lv_color_hex(card_bg[i]), 0);
        lv_obj_set_style_bg_opa(ico_box, LV_OPA_COVER, 0);
        lv_obj_set_scrollbar_mode(ico_box, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(ico_box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *sym = lv_label_create(ico_box);
        lv_label_set_text(sym, card_sym[i]);
        lv_obj_set_style_text_color(sym, lv_color_white(), 0);
        lv_obj_center(sym);

        lv_obj_t *txt_col = lv_obj_create(card);
        lv_obj_remove_style_all(txt_col);
        lv_obj_set_size(txt_col, 210, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(txt_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scrollbar_mode(txt_col, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(txt_col, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *app = lv_label_create(txt_col);
        lv_label_set_text(app, card_app[i]);
        lv_obj_set_style_text_color(app, lv_color_white(), 0);
        lv_obj_set_style_text_opa(app, 102, 0);
        lv_obj_set_style_text_font(app, &lv_font_montserrat_14, 0);

        lv_obj_t *msg = lv_label_create(txt_col);
        lv_label_set_text(msg, card_msg[i]);
        lv_obj_set_style_text_color(msg, lv_color_white(), 0);
        lv_obj_set_style_text_opa(msg, 191, 0);
        lv_obj_set_style_text_font(msg, &lv_font_montserrat_14, 0);
        lv_label_set_long_mode(msg, LV_LABEL_LONG_DOT);
        lv_obj_set_width(msg, 210);

        lv_obj_t *tm = lv_label_create(card);
        lv_label_set_text(tm, card_tm[i]);
        lv_obj_set_style_text_color(tm, lv_color_white(), 0);
        lv_obj_set_style_text_opa(tm, 77, 0);
        lv_obj_set_style_text_font(tm, &lv_font_montserrat_14, 0);
    }

    /* Quick action buttons */
    lv_obj_t *btn_row = lv_obj_create(cont);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_row, 32, 0);
    lv_obj_set_style_pad_top(btn_row, 4, 0);
    lv_obj_set_scrollbar_mode(btn_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    static const char *const btn_name[3] = { "Flash", "Camera", "Sound" };

    for (int i = 0; i < 3; i++) {
        lv_obj_t *col = lv_obj_create(btn_row);
        lv_obj_remove_style_all(col);
        lv_obj_set_size(col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(col, 4, 0);
        lv_obj_set_scrollbar_mode(col, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *btn = lv_obj_create(col);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, 42, 42);
        lv_obj_set_style_radius(btn, 21, 0);
        lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(btn, 20, 0);
        lv_obj_set_style_border_color(btn, lv_color_white(), 0);
        lv_obj_set_style_border_opa(btn, 38, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *cv = lv_canvas_create(btn);
        lv_canvas_set_buffer(cv,
            i == 0 ? s_buf_flash : i == 1 ? s_buf_cam : s_buf_vol,
            20, 20, LV_IMG_CF_TRUE_COLOR);
        lv_obj_center(cv);

        if (i == 0) {
            s_flash_btn    = btn;
            s_flash_canvas = cv;
            draw_flashlight(cv, lv_color_white());
            lv_obj_add_event_cb(btn, flash_toggle_cb, LV_EVENT_CLICKED, NULL);
        } else if (i == 1) {
            draw_camera(cv);
        } else {
            draw_volume(cv);
        }

        lv_obj_t *lbl = lv_label_create(col);
        lv_label_set_text(lbl, btn_name[i]);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_opa(lbl, 89, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);

        if (i == 0) s_flash_lbl = lbl;
    }
}

/* ── Unlock area (y=400, h=80) ───────────────────────────────────────── */

static void make_unlock(lv_obj_t *scr)
{
    lv_obj_t *area = lv_obj_create(scr);
    lv_obj_remove_style_all(area);
    lv_obj_set_size(area, 800, 80);
    lv_obj_set_pos(area, 0, 400);
    lv_obj_set_flex_flow(area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(area, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(area, 6, 0);
    lv_obj_set_scrollbar_mode(area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(area, unlock_click_cb, LV_EVENT_CLICKED, NULL);

    static lv_point_t chevron[] = { {5,8},{10,2},{15,8} };
    lv_obj_t *chev = lv_line_create(area);
    lv_line_set_points(chev, chevron, 3);
    lv_obj_set_style_line_color(chev, lv_color_white(), 0);
    lv_obj_set_style_line_opa(chev, 128, 0);
    lv_obj_set_style_line_width(chev, 2, 0);
    lv_obj_set_style_line_rounded(chev, true, 0);

    lv_obj_t *lbl = lv_label_create(area);
    lv_label_set_text(lbl, "CLICK TO UNLOCK");
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_opa(lbl, 102, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(lbl, 1, 0);

    lv_obj_t *bar = lv_obj_create(area);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 44, 3);
    lv_obj_set_style_radius(bar, 2, 0);
    lv_obj_set_style_bg_color(bar, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(bar, 51, 0);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
}

/* ── Public API ──────────────────────────────────────────────────────── */

lv_obj_t *ui_lock_create(void)
{
    s_time_lbl     = NULL;
    s_flash_btn    = NULL;
    s_flash_lbl    = NULL;
    s_flash_canvas = NULL;
    s_flash_on     = false;

    if (s_clock_timer) {
        lv_timer_del(s_clock_timer);
        s_clock_timer = NULL;
    }

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1625), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    make_top_bar(scr);
    make_center(scr);
    make_unlock(scr);

    s_clock_timer = lv_timer_create(clock_timer_cb, 60000, NULL);

    lv_timer_t *auto_t = lv_timer_create(to_login_cb, 5000, NULL);
    lv_timer_set_repeat_count(auto_t, 1);

    pr_info("[UI] lock created\n");
    return scr;
}
