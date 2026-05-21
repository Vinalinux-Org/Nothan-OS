/*
 * drivers/ui/ui_auth.c — Auth transition screen for Nothan-OS (800×480)
 *
 * Two-layer architecture on one root screen:
 *   Layer 1 (bottom): login_container — avatar, username, password dots, spinner
 *   Layer 2 (top):    lock_container  — time, date (covers login initially)
 *
 * Timeline:
 *   t=0s   lock covers login
 *   t=2s   lock slides up (400ms, ease-out), Timer2 created
 *   t=2.4s lock deleted, login revealed + spinner
 *   t=4s   transition to home screen
 */

#include "lvgl/lvgl.h"
#include "vinix/printk.h"

lv_obj_t *ui_home_create(void);

static void anim_y_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, (lv_coord_t)v);
}

static void del_anim_ready_cb(lv_anim_t *a)
{
    lv_obj_del((lv_obj_t *)a->var);
    pr_info("[UI] Auth: lock layer deleted\n");
}

static void to_home_cb(lv_timer_t *timer)
{
    lv_timer_del(timer);
    pr_info("[UI] Auth: -> home\n");
    lv_scr_load_anim(ui_home_create(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}

static void start_slide_up(lv_timer_t *timer)
{
    lv_obj_t *lock = (lv_obj_t *)timer->user_data;
    lv_timer_del(timer);

    pr_info("[UI] Auth: sliding lock up\n");

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, lock);
    lv_anim_set_exec_cb(&a, anim_y_cb);
    lv_anim_set_values(&a, 0, -lv_obj_get_height(lock));   /* 0 → -480 */
    lv_anim_set_time(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, del_anim_ready_cb);
    lv_anim_start(&a);

    lv_timer_t *t2 = lv_timer_create(to_home_cb, 2000, NULL);
    lv_timer_set_repeat_count(t2, 1);
}

static lv_obj_t *make_rect(lv_obj_t *parent,
                            lv_coord_t x, lv_coord_t y,
                            lv_coord_t w, lv_coord_t h,
                            uint32_t hex, lv_coord_t radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(hex), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
    return o;
}

static lv_obj_t *make_layer(lv_obj_t *parent, uint32_t bg_hex)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_remove_style_all(c);
    lv_obj_set_size(c, 800, 480);
    lv_obj_set_pos(c, 0, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(bg_hex), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

lv_obj_t *ui_auth_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    /* ── Layer 1: login (bottom, created first → lower z-order) ─── */
    lv_obj_t *login = make_layer(scr, 0x1a1a1a);

    lv_obj_t *avatar = lv_obj_create(login);
    lv_obj_remove_style_all(avatar);
    lv_obj_set_size(avatar, 90, 90);
    lv_obj_align(avatar, LV_ALIGN_CENTER, 0, -70);
    lv_obj_set_style_radius(avatar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(avatar, lv_color_hex(0x3e3e5a), 0);
    lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(avatar, lv_color_hex(0xd25014), 0);
    lv_obj_set_style_border_width(avatar, 2, 0);
    lv_obj_set_scrollbar_mode(avatar, LV_SCROLLBAR_MODE_OFF);

    make_rect(avatar, 25,  0, 40, 40, 0xbcbcbc, LV_RADIUS_CIRCLE);  /* head */
    make_rect(avatar, 13, 44, 64, 64, 0xbcbcbc, LV_RADIUS_CIRCLE);  /* body */

    lv_obj_t *uname = lv_label_create(login);
    lv_label_set_text(uname, "nothan-user");
    lv_obj_set_style_text_color(uname, lv_color_hex(0xe8e8f0), 0);
    lv_obj_set_style_text_font(uname, &lv_font_montserrat_20, 0);
    lv_obj_align(uname, LV_ALIGN_CENTER, 0, -5);

    lv_obj_t *pw_bg = lv_obj_create(login);
    lv_obj_remove_style_all(pw_bg);
    lv_obj_set_size(pw_bg, 220, 44);
    lv_obj_align(pw_bg, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_radius(pw_bg, 22, 0);
    lv_obj_set_style_bg_color(pw_bg, lv_color_hex(0x252525), 0);
    lv_obj_set_style_bg_opa(pw_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(pw_bg, lv_color_hex(0xd25014), 0);
    lv_obj_set_style_border_width(pw_bg, 1, 0);
    lv_obj_set_scrollbar_mode(pw_bg, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *pw_dots = lv_label_create(pw_bg);
    lv_label_set_text(pw_dots,
        "\xE2\x97\x8F \xE2\x97\x8F \xE2\x97\x8F "
        "\xE2\x97\x8F \xE2\x97\x8F \xE2\x97\x8F");
    lv_obj_set_style_text_color(pw_dots, lv_color_hex(0xd25014), 0);
    lv_obj_set_style_text_font(pw_dots, &lv_font_montserrat_16, 0);
    lv_obj_align(pw_dots, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *spinner = lv_spinner_create(login, 1000, 60);
    lv_obj_set_size(spinner, 44, 44);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 115);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0xd25014), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);

    lv_obj_t *auth_lbl = lv_label_create(login);
    lv_label_set_text(auth_lbl, "Authenticating...");
    lv_obj_set_style_text_color(auth_lbl, lv_color_hex(0x707070), 0);
    lv_obj_set_style_text_font(auth_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(auth_lbl, LV_ALIGN_CENTER, 0, 165);

    /* ── Layer 2: lock (top, covers login) ──────────────────────── */
    lv_obj_t *lock = make_layer(scr, 0x1a1a1a);

    lv_obj_t *time_lbl = lv_label_create(lock);
    lv_label_set_text(time_lbl, "19:20");
    lv_obj_set_style_text_color(time_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_32, 0);
    lv_obj_align(time_lbl, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *date_lbl = lv_label_create(lock);
    lv_label_set_text(date_lbl, "Tuesday, May 12");
    lv_obj_set_style_text_color(date_lbl, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(date_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align_to(date_lbl, time_lbl, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_timer_t *t1 = lv_timer_create(start_slide_up, 2000, lock);
    lv_timer_set_repeat_count(t1, 1);

    return scr;
}
