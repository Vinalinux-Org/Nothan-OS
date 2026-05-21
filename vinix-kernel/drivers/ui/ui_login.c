/*
 * drivers/ui/ui_login.c — Login screen for Nothan-OS (800×480)
 *
 * Layout:
 *   Top bar     y=0   h=44   brand + status icons + clock
 *   Center      y=44  h=394  avatar, username, password, sign-in
 *   Bottom bar  y=438 h=42   power buttons + language
 */

#include "lvgl/lvgl.h"
#include "vinix/printk.h"
#include "screen_transition.h"

lv_obj_t *ui_home_create(void);

static lv_obj_t *s_clock_lbl;
static lv_obj_t *s_pw_ta;
static lv_obj_t *s_error_lbl;
static lv_obj_t *s_eye_btn;
static lv_obj_t *s_signin_btn;
static lv_obj_t *s_spinner;

static uint8_t s_h = 17, s_m = 43;
static lv_timer_t *s_clock_timer;

/* ── Clock timer ─────────────────────────────────────────────────────── */

static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (++s_m >= 60) { s_m = 0; if (++s_h >= 24) s_h = 0; }
    if (s_clock_lbl) {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)s_h, (unsigned)s_m);
        lv_label_set_text(s_clock_lbl, buf);
    }
}

static void to_home_cb(lv_timer_t *t)
{
    (void)t;
    pr_info("[UI] Login -> home\n");
    transition_to(ui_home_create(), 1);
}

static void eye_toggle_cb(lv_event_t *e)
{
    (void)e;
    if (!s_pw_ta) return;
    bool mode = lv_textarea_get_password_mode(s_pw_ta);
    lv_textarea_set_password_mode(s_pw_ta, !mode);
    if (s_eye_btn)
        lv_label_set_text(s_eye_btn,
                          mode ? LV_SYMBOL_EYE_OPEN : LV_SYMBOL_EYE_CLOSE);
}

static void signin_cb(lv_event_t *e)
{
    (void)e;
    if (!s_pw_ta) return;
    const char *txt = lv_textarea_get_text(s_pw_ta);
    if (!txt || txt[0] == '\0') {
        if (s_error_lbl) lv_obj_clear_flag(s_error_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (s_error_lbl)  lv_obj_add_flag(s_error_lbl,  LV_OBJ_FLAG_HIDDEN);
    if (s_signin_btn) lv_obj_add_flag(s_signin_btn, LV_OBJ_FLAG_HIDDEN);
    if (s_spinner) {
        lv_obj_clear_flag(s_spinner, LV_OBJ_FLAG_HIDDEN);
        lv_timer_t *t = lv_timer_create(to_home_cb, 1500, NULL);
        lv_timer_set_repeat_count(t, 1);
    }
}

/* ── Top bar (h=44) ──────────────────────────────────────────────────── */

static void make_top_bar(lv_obj_t *scr)
{
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 800, 44);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0d0a1a), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = lv_label_create(bar);
    lv_label_set_text(brand, "NOTHAN-OS");
    lv_obj_set_style_text_color(brand, lv_color_white(), 0);
    lv_obj_set_style_text_opa(brand, 102, 0);
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(brand, 2, 0);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 16, 0);

    s_clock_lbl = lv_label_create(bar);
    lv_label_set_text(s_clock_lbl, "17:43");
    lv_obj_set_style_text_color(s_clock_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_opa(s_clock_lbl, 89, 0);
    lv_obj_set_style_text_font(s_clock_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(s_clock_lbl, LV_ALIGN_RIGHT_MID, -16, 0);

    lv_obj_t *ic_bat = lv_label_create(bar);
    lv_label_set_text(ic_bat, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(ic_bat, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_bat, 89, 0);
    lv_obj_align_to(ic_bat, s_clock_lbl, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_t *ic_bt = lv_label_create(bar);
    lv_label_set_text(ic_bt, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(ic_bt, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_bt, 89, 0);
    lv_obj_align_to(ic_bt, ic_bat, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    lv_obj_t *ic_wifi = lv_label_create(bar);
    lv_label_set_text(ic_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(ic_wifi, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_wifi, 89, 0);
    lv_obj_align_to(ic_wifi, ic_bt, LV_ALIGN_OUT_LEFT_MID, -10, 0);
}

/* ── Center area (y=44, h=394) ───────────────────────────────────────── */

static void make_center(lv_obj_t *scr)
{
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 800, 394);
    lv_obj_set_pos(cont, 0, 44);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 12, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *avatar = lv_obj_create(cont);
    lv_obj_remove_style_all(avatar);
    lv_obj_set_size(avatar, 72, 72);
    lv_obj_set_style_radius(avatar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(avatar, lv_color_hex(0x3d2b6e), 0);
    lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(avatar, lv_color_white(), 0);
    lv_obj_set_style_border_opa(avatar, 38, 0);
    lv_obj_set_style_border_width(avatar, 2, 0);
    lv_obj_set_scrollbar_mode(avatar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *person = lv_label_create(avatar);
    lv_label_set_text(person, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(person, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(person, lv_color_white(), 0);
    lv_obj_set_style_text_opa(person, 187, 0);
    lv_obj_center(person);

    lv_obj_t *uname = lv_label_create(cont);
    lv_label_set_text(uname, "nothan-user");
    lv_obj_set_style_text_color(uname, lv_color_white(), 0);
    lv_obj_set_style_text_font(uname, &lv_font_montserrat_18, 0);

    lv_obj_t *pw_row = lv_obj_create(cont);
    lv_obj_remove_style_all(pw_row);
    lv_obj_set_size(pw_row, 280, 42);
    lv_obj_set_style_radius(pw_row, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pw_row, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(pw_row, 18, 0);
    lv_obj_set_style_border_color(pw_row, lv_color_white(), 0);
    lv_obj_set_style_border_opa(pw_row, 31, 0);
    lv_obj_set_style_border_width(pw_row, 2, 0);
    lv_obj_set_style_pad_left(pw_row, 18, 0);
    lv_obj_set_style_pad_right(pw_row, 36, 0);
    lv_obj_set_scrollbar_mode(pw_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(pw_row, LV_OBJ_FLAG_SCROLLABLE);

    s_pw_ta = lv_textarea_create(pw_row);
    lv_obj_remove_style_all(s_pw_ta);
    lv_obj_set_size(s_pw_ta, 220, 38);
    lv_obj_set_pos(s_pw_ta, 0, 2);
    lv_obj_set_style_text_color(s_pw_ta, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_pw_ta, &lv_font_montserrat_14, 0);
    lv_textarea_set_password_mode(s_pw_ta, true);
    lv_textarea_set_placeholder_text(s_pw_ta, "Enter password");
    lv_textarea_set_one_line(s_pw_ta, true);
    lv_obj_set_style_bg_opa(s_pw_ta, 0, 0);
    lv_obj_set_style_border_width(s_pw_ta, 0, 0);

    s_eye_btn = lv_label_create(pw_row);
    lv_label_set_text(s_eye_btn, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(s_eye_btn, lv_color_hex(0x888888), 0);
    lv_obj_align(s_eye_btn, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_add_flag(s_eye_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_eye_btn, eye_toggle_cb, LV_EVENT_CLICKED, NULL);

    s_signin_btn = lv_obj_create(cont);
    lv_obj_remove_style_all(s_signin_btn);
    lv_obj_set_size(s_signin_btn, 140, 38);
    lv_obj_set_style_radius(s_signin_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_signin_btn, lv_color_hex(0x7c3aed), 0);
    lv_obj_set_style_bg_opa(s_signin_btn, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(s_signin_btn, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_signin_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_signin_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_signin_btn, signin_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(s_signin_btn);
    lv_label_set_text(btn_lbl, "Sign in");
    lv_obj_set_style_text_color(btn_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(btn_lbl);

    s_error_lbl = lv_label_create(cont);
    lv_label_set_text(s_error_lbl, "Password cannot be empty");
    lv_obj_set_style_text_color(s_error_lbl, lv_color_hex(0xf87171), 0);
    lv_obj_set_style_text_font(s_error_lbl, &lv_font_montserrat_14, 0);
    lv_obj_add_flag(s_error_lbl, LV_OBJ_FLAG_HIDDEN);

    s_spinner = lv_spinner_create(cont, 1000, 60);
    lv_obj_set_size(s_spinner, 34, 34);
    lv_obj_set_style_arc_color(s_spinner, lv_color_hex(0x7c3aed), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_spinner, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_spinner, lv_color_hex(0x2a1a4a), LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_spinner, 3, LV_PART_MAIN);
    lv_obj_add_flag(s_spinner, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *sec_row = lv_obj_create(cont);
    lv_obj_remove_style_all(sec_row);
    lv_obj_set_size(sec_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(sec_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sec_row, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(sec_row, 24, 0);
    lv_obj_set_scrollbar_mode(sec_row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(sec_row, LV_OBJ_FLAG_SCROLLABLE);

    static const char *const sec_txt[3] = {
        "Switch user", "Forgot PIN", "Accessibility"
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = lv_label_create(sec_row);
        lv_label_set_text(b, sec_txt[i]);
        lv_obj_set_style_text_color(b, lv_color_white(), 0);
        lv_obj_set_style_text_opa(b, 85, 0);
        lv_obj_set_style_text_font(b, &lv_font_montserrat_14, 0);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    }
}

/* ── Bottom bar (y=438, h=42) ────────────────────────────────────────── */

static void make_bottom_bar(lv_obj_t *scr)
{
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 800, 42);
    lv_obj_set_pos(bar, 0, 438);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0d0a1a), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *left = lv_obj_create(bar);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, LV_SIZE_CONTENT, 42);
    lv_obj_set_pos(left, 16, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, 16, 0);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    static const char *const pw_sym[3] = {
        LV_SYMBOL_POWER, LV_SYMBOL_REFRESH, LV_SYMBOL_PAUSE
    };
    static const char *const pw_txt[3] = { "Shut down", "Restart", "Sleep" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *b = lv_obj_create(left);
        lv_obj_remove_style_all(b);
        lv_obj_set_size(b, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(b, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(b, 4, 0);
        lv_obj_set_scrollbar_mode(b, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *ic = lv_label_create(b);
        lv_label_set_text(ic, pw_sym[i]);
        lv_obj_set_style_text_color(ic, lv_color_white(), 0);
        lv_obj_set_style_text_opa(ic, 102, 0);

        lv_obj_t *t = lv_label_create(b);
        lv_label_set_text(t, pw_txt[i]);
        lv_obj_set_style_text_color(t, lv_color_white(), 0);
        lv_obj_set_style_text_opa(t, 102, 0);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
    }

    lv_obj_t *lang = lv_label_create(bar);
    lv_label_set_text(lang, "EN");
    lv_obj_set_style_text_color(lang, lv_color_white(), 0);
    lv_obj_set_style_text_opa(lang, 102, 0);
    lv_obj_set_style_text_font(lang, &lv_font_montserrat_14, 0);
    lv_obj_align(lang, LV_ALIGN_RIGHT_MID, -40, 0);
    lv_obj_add_flag(lang, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *set_ic = lv_label_create(bar);
    lv_label_set_text(set_ic, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(set_ic, lv_color_white(), 0);
    lv_obj_set_style_text_opa(set_ic, 102, 0);
    lv_obj_align(set_ic, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_obj_add_flag(set_ic, LV_OBJ_FLAG_CLICKABLE);
}

/* ── Public API ──────────────────────────────────────────────────────── */

lv_obj_t *ui_login_create(void)
{
    s_clock_lbl  = NULL;
    s_pw_ta      = NULL;
    s_error_lbl  = NULL;
    s_eye_btn    = NULL;
    s_signin_btn = NULL;
    s_spinner    = NULL;

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
    make_bottom_bar(scr);

    s_clock_timer = lv_timer_create(clock_timer_cb, 60000, NULL);

    pr_info("[UI] login created\n");
    return scr;
}
