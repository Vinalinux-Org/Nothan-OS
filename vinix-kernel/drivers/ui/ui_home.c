/*
 * drivers/ui/ui_home.c — Home screen for Nothan-OS (800×480)
 *
 * Layout:
 *   Zone 1  y=0   h=52   Status bar
 *   Zone 2  y=52  h=332  Main content
 *     Left  40%  308×308  Clock card + system stats
 *     Right 60%  456×308  App grid 2×3
 *   Zone 3  y=384 h=96   Dock bar
 */

#include "lvgl/lvgl.h"
#include "vinix/printk.h"
#include "ui_home.h"

/* ── Font fallbacks ─────────────────────────────────────────────────── */
#if LV_FONT_MONTSERRAT_48
#  define F_TIME_LG  (&lv_font_montserrat_48)
#else
#  define F_TIME_LG  (&lv_font_montserrat_32)
#endif
#if LV_FONT_MONTSERRAT_28
#  define F_ICON_LG  (&lv_font_montserrat_28)
#else
#  define F_ICON_LG  (&lv_font_montserrat_24)
#endif
#if LV_FONT_MONTSERRAT_18
#  define F_TIME_SM  (&lv_font_montserrat_18)
#else
#  define F_TIME_SM  (&lv_font_montserrat_16)
#endif
#if LV_FONT_MONTSERRAT_10
#  define F_DOCK     (&lv_font_montserrat_10)
#else
#  define F_DOCK     (&lv_font_montserrat_14)
#endif

/* ── Static widget handles ──────────────────────────────────────────── */
static lv_obj_t *s_lbl_time;
static lv_obj_t *s_lbl_digi_time;
static lv_obj_t *s_lbl_digi_day;
static lv_obj_t *s_lbl_digi_date;
static lv_obj_t *s_lbl_temp;
static lv_obj_t *s_lbl_desc;
static lv_obj_t *s_lbl_location;
static lv_obj_t *s_ic_bluetooth;
static lv_obj_t *s_ic_wifi;
static lv_obj_t *s_ic_battery;
static lv_obj_t *s_app_items[6];
static lv_obj_t *s_dock_items[7];
static lv_obj_t *s_dock_active_bar;
static uint8_t   s_dock_active_idx;
static lv_obj_t *s_dock_names[7];

static const uint32_t k_dock_icol[7] = {
    0xff9800, 0x4db8ff, 0x4db8ff, 0xFFFFFF, 0xff4daa, 0xff9800, 0x7b1fa2
};
static const lv_opa_t k_dock_iopa[7] = {
    LV_OPA_COVER, LV_OPA_COVER, LV_OPA_COVER, 153,
    LV_OPA_COVER, LV_OPA_COVER, LV_OPA_COVER
};

static uint8_t     s_h = 17, s_m = 43, s_sec = 0;
static lv_timer_t *s_clock_timer;

static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (++s_sec >= 60) {
        s_sec = 0;
        if (++s_m >= 60) { s_m = 0; if (++s_h >= 24) s_h = 0; }
    }
    if (s_sec == 0) {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)s_h, (unsigned)s_m);
        if (s_lbl_time)      lv_label_set_text(s_lbl_time,      buf);
        if (s_lbl_digi_time) lv_label_set_text(s_lbl_digi_time, buf);
    }
}

/* ════════════════════════════════════════════════════════════════════
 * Zone 1 — Status bar (h=52)
 * ════════════════════════════════════════════════════════════════════ */

static void make_status_bar(lv_obj_t *scr)
{
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 800, 52);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x05081e), 0);
    lv_obj_set_style_bg_opa(bar, 242, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(bar, lv_color_white(), 0);
    lv_obj_set_style_border_opa(bar, 18, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    static const char *const k_nav[3] = {
        LV_SYMBOL_HOME, LV_SYMBOL_LIST, LV_SYMBOL_SETTINGS
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_obj_create(bar);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, 36, 36);
        lv_obj_set_pos(btn, 18 + i * 42, (52 - 36) / 2);
        lv_obj_set_style_radius(btn, 11, 0);
        lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(btn, i == 0 ? 38 : 18, 0);
        lv_obj_set_style_border_color(btn, lv_color_white(), 0);
        lv_obj_set_style_border_opa(btn, 30, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *ico = lv_label_create(btn);
        lv_label_set_text(ico, k_nav[i]);
        lv_obj_set_style_text_color(ico, lv_color_white(), 0);
        lv_obj_set_style_text_opa(ico, 204, 0);
        lv_obj_center(ico);
    }

    lv_obj_t *ic_bat = lv_label_create(bar);
    lv_label_set_text(ic_bat, LV_SYMBOL_BATTERY_3);
    lv_obj_set_style_text_color(ic_bat, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_bat, 178, 0);
    lv_obj_align(ic_bat, LV_ALIGN_RIGHT_MID, -18, 0);
    s_ic_battery = ic_bat;

    lv_obj_t *ic_wifi = lv_label_create(bar);
    lv_label_set_text(ic_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(ic_wifi, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_wifi, 178, 0);
    lv_obj_align_to(ic_wifi, ic_bat, LV_ALIGN_OUT_LEFT_MID, -12, 0);
    s_ic_wifi = ic_wifi;

    lv_obj_t *ic_bt = lv_label_create(bar);
    lv_label_set_text(ic_bt, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(ic_bt, lv_color_white(), 0);
    lv_obj_set_style_text_opa(ic_bt, 178, 0);
    lv_obj_align_to(ic_bt, ic_wifi, LV_ALIGN_OUT_LEFT_MID, -12, 0);
    s_ic_bluetooth = ic_bt;

    lv_obj_t *lbl_t = lv_label_create(bar);
    lv_label_set_text(lbl_t, "17:43");
    lv_obj_set_style_text_color(lbl_t, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_t, F_TIME_SM, 0);
    lv_obj_set_style_text_letter_space(lbl_t, 1, 0);
    lv_obj_align_to(lbl_t, ic_bt, LV_ALIGN_OUT_LEFT_MID, -12, 0);
    s_lbl_time = lbl_t;
}

/* ════════════════════════════════════════════════════════════════════
 * Zone 2 left panel — Clock card (308×308)
 * ════════════════════════════════════════════════════════════════════ */

static void make_left_panel(lv_obj_t *content)
{
    lv_obj_t *card = lv_obj_create(content);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, 308, 308);
    lv_obj_set_pos(card, 12, 12);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x0d1233), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x7c3aed), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 20, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_digi_time = lv_label_create(card);
    lv_label_set_text(s_lbl_digi_time, "17:43");
    lv_obj_set_style_text_color(s_lbl_digi_time, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_lbl_digi_time, F_TIME_LG, 0);

    s_lbl_digi_day = lv_label_create(card);
    lv_label_set_text(s_lbl_digi_day, "Friday");
    lv_obj_set_style_text_color(s_lbl_digi_day, lv_color_hex(0x6366f1), 0);
    lv_obj_set_style_text_font(s_lbl_digi_day, &lv_font_montserrat_16, 0);

    s_lbl_digi_date = lv_label_create(card);
    lv_label_set_text(s_lbl_digi_date, "2025/05/16");
    lv_obj_set_style_text_color(s_lbl_digi_date, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(s_lbl_digi_date, &lv_font_montserrat_14, 0);

    lv_obj_t *sep = lv_obj_create(card);
    lv_obj_remove_style_all(sep);
    lv_obj_set_size(sep, lv_pct(100), 2);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x29b6f6), 0);
    lv_obj_set_style_bg_opa(sep, 80, 0);

    static const char *const sys_sym[3]  = { LV_SYMBOL_WARNING, LV_SYMBOL_AUDIO, LV_SYMBOL_SETTINGS };
    static const char *const sys_name[3] = { "CPU Temp", "Volume", "Brightness" };
    static const char *const sys_val[3]  = { "42°", "68%", "80%" };
    static const uint32_t    sys_col[3]  = { 0xffca28, 0xab47bc, 0x29b6f6 };

    for (int i = 0; i < 3; i++) {
        lv_obj_t *row = lv_obj_create(card);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, lv_pct(100), 40);

        lv_obj_t *ic = lv_label_create(row);
        lv_label_set_text(ic, sys_sym[i]);
        lv_obj_set_style_text_color(ic, lv_color_hex(sys_col[i]), 0);
        lv_obj_align(ic, LV_ALIGN_LEFT_MID, 0, -7);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, sys_name[i]);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 28, -7);

        lv_obj_t *val = lv_label_create(row);
        lv_label_set_text(val, sys_val[i]);
        lv_obj_set_style_text_color(val, lv_color_white(), 0);
        lv_obj_set_style_text_font(val, &lv_font_montserrat_14, 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, 0, -7);

        int pct = (i == 0) ? 42 : (i == 1) ? 68 : 80;
        lv_obj_t *track = lv_obj_create(row);
        lv_obj_remove_style_all(track);
        lv_obj_set_size(track, 200, 4);
        lv_obj_align(track, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        lv_obj_set_style_bg_color(track, lv_color_hex(0x2a2f4c), 0);
        lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(track, 2, 0);
        lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ind = lv_obj_create(track);
        lv_obj_remove_style_all(ind);
        lv_obj_set_size(ind, 200 * pct / 100, 4);
        lv_obj_set_pos(ind, 0, 0);
        lv_obj_set_style_bg_color(ind, lv_color_hex(sys_col[i]), 0);
        lv_obj_set_style_bg_opa(ind, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(ind, 2, 0);
    }
}

/* ════════════════════════════════════════════════════════════════════
 * Zone 2 right panel — App grid (456×308)
 * ════════════════════════════════════════════════════════════════════ */

static void make_right_panel(lv_obj_t *content)
{
    lv_obj_t *grid = lv_obj_create(content);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, 456, 308);
    lv_obj_set_pos(grid, 332, 12);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(grid, 10, 0);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    static lv_style_t s_press;
    static bool s_inited = false;
    if (!s_inited) {
        static const lv_style_prop_t props[] = { LV_STYLE_BG_COLOR, (lv_style_prop_t)0 };
        static lv_style_transition_dsc_t trans;
        lv_style_init(&s_press);
        lv_style_set_bg_color(&s_press, lv_color_hex(0x252a4a));
        lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_out, 200, 0, NULL);
        lv_style_set_transition(&s_press, &trans);
        s_inited = true;
    }

    static const char *const k_name[6] = {
        "Files", "Music", "CH Play", "Maps", "Settings", "YouTube"
    };
    static const char *const k_sym[6] = {
        LV_SYMBOL_DIRECTORY, LV_SYMBOL_AUDIO, LV_SYMBOL_DOWNLOAD,
        LV_SYMBOL_GPS, LV_SYMBOL_SETTINGS, LV_SYMBOL_PLAY
    };
    static const uint32_t k_col[6] = {
        0xffca28, 0x29b6f6, 0xab47bc, 0x26a69a, 0x9e9e9e, 0xef5350
    };

    for (int i = 0; i < 6; i++) {
        lv_obj_t *tile = lv_obj_create(grid);
        lv_obj_remove_style_all(tile);
        lv_obj_set_size(tile, 124, 120);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x1a1f3a), 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(tile, 12, 0);
        lv_obj_set_style_border_color(tile, lv_color_white(), 0);
        lv_obj_set_style_border_opa(tile, 60, 0);
        lv_obj_set_style_border_width(tile, 2, 0);
        lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(tile, 8, 0);
        lv_obj_set_scrollbar_mode(tile, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_style(tile, &s_press, LV_STATE_PRESSED);

        lv_obj_t *ico = lv_label_create(tile);
        lv_label_set_text(ico, k_sym[i]);
        lv_obj_set_style_text_font(ico, F_ICON_LG, 0);
        lv_obj_set_style_text_color(ico, lv_color_hex(k_col[i]), 0);

        lv_obj_t *name = lv_label_create(tile);
        lv_label_set_text(name, k_name[i]);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name, lv_color_white(), 0);
        lv_obj_set_style_text_opa(name, 204, 0);

        s_app_items[i] = tile;
    }
}

/* ════════════════════════════════════════════════════════════════════
 * Zone 2 — Main content (800×332 at y=52)
 * ════════════════════════════════════════════════════════════════════ */

static void make_main_content(lv_obj_t *scr)
{
    lv_obj_t *area = lv_obj_create(scr);
    lv_obj_remove_style_all(area);
    lv_obj_set_size(area, 800, 332);
    lv_obj_set_pos(area, 0, 52);
    lv_obj_set_scrollbar_mode(area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(area, LV_OBJ_FLAG_SCROLLABLE);

    make_left_panel(area);
    make_right_panel(area);
}

/* ════════════════════════════════════════════════════════════════════
 * Zone 3 — Dock bar (800×96 at y=384)
 * ════════════════════════════════════════════════════════════════════ */

static void make_dock(lv_obj_t *scr)
{
    lv_obj_t *dock = lv_obj_create(scr);
    lv_obj_remove_style_all(dock);
    lv_obj_set_size(dock, 800, 96);
    lv_obj_set_pos(dock, 0, 384);
    lv_obj_set_style_bg_color(dock, lv_color_hex(0x05081e), 0);
    lv_obj_set_style_bg_opa(dock, 234, 0);
    lv_obj_set_style_border_side(dock, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(dock, lv_color_white(), 0);
    lv_obj_set_style_border_opa(dock, 15, 0);
    lv_obj_set_style_border_width(dock, 1, 0);
    lv_obj_set_scrollbar_mode(dock, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(dock, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *row = lv_obj_create(dock);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 800, 96);
    lv_obj_set_pos(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_AROUND,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    static const char *const k_dname[7] = {
        "Navigate", "Bluetooth", "Music", "Settings",
        "Radio",    "Video",     "Apps"
    };
    static const char *const k_dsym[7] = {
        LV_SYMBOL_GPS, LV_SYMBOL_BLUETOOTH, LV_SYMBOL_AUDIO,
        LV_SYMBOL_SETTINGS, LV_SYMBOL_WIFI, LV_SYMBOL_VIDEO, ""
    };

    for (int i = 0; i < 7; i++) {
        lv_obj_t *item = lv_obj_create(row);
        lv_obj_remove_style_all(item);
        lv_obj_set_size(item, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(item, 4, 0);
        lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);

        if (i < 6) {
            lv_obj_t *ico = lv_label_create(item);
            lv_label_set_text(ico, k_dsym[i]);
            lv_obj_set_style_text_font(ico, F_ICON_LG, 0);
            lv_obj_set_style_text_color(ico, lv_color_hex(k_dock_icol[i]), 0);
            lv_obj_set_style_text_opa(ico, k_dock_iopa[i], 0);
        } else {
            lv_obj_t *box = lv_obj_create(item);
            lv_obj_remove_style_all(box);
            lv_obj_set_size(box, 36, 36);
            lv_obj_set_style_radius(box, 10, 0);
            lv_obj_set_style_bg_color(box, lv_color_hex(0x7b1fa2), 0);
            lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
            lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
            lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *plus = lv_label_create(box);
            lv_label_set_text(plus, LV_SYMBOL_PLUS);
            lv_obj_set_style_text_font(plus, F_TIME_SM, 0);
            lv_obj_set_style_text_color(plus, lv_color_white(), 0);
            lv_obj_center(plus);
        }

        lv_obj_t *name = lv_label_create(item);
        lv_label_set_text(name, k_dname[i]);
        lv_obj_set_style_text_font(name, F_DOCK, 0);
        lv_obj_set_style_text_color(name, lv_color_white(), 0);
        lv_obj_set_style_text_opa(name, 140, 0);

        s_dock_items[i] = item;
        s_dock_names[i] = name;
    }

    /* Rainbow accent line at top of dock */
    static const uint32_t k_rainbow[7] = {
        0xff6b35, 0xf7c948, 0x4de8a0, 0x4db8ff,
        0xa56bff, 0xff4daa, 0xff6b35
    };
    int cx = 0;
    for (int i = 0; i < 7; i++) {
        lv_obj_t *seg = lv_obj_create(dock);
        lv_obj_remove_style_all(seg);
        int w = (i == 0 || i == 6) ? 115 : 114;
        lv_obj_set_size(seg, w, 2);
        lv_obj_set_pos(seg, cx, 0);
        lv_obj_set_style_bg_color(seg, lv_color_hex(k_rainbow[i]), 0);
        lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
        lv_obj_set_scrollbar_mode(seg, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
        cx += w;
    }

    s_dock_active_bar = lv_obj_create(s_dock_items[0]);
    lv_obj_remove_style_all(s_dock_active_bar);
    lv_obj_set_size(s_dock_active_bar, 28, 3);
    lv_obj_set_style_radius(s_dock_active_bar, 2, 0);
    lv_obj_set_style_bg_color(s_dock_active_bar, lv_color_hex(0xff9800), 0);
    lv_obj_set_style_bg_opa(s_dock_active_bar, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(s_dock_active_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_dock_active_bar, LV_OBJ_FLAG_SCROLLABLE);
#ifdef LV_OBJ_FLAG_FLOATING
    lv_obj_add_flag(s_dock_active_bar, LV_OBJ_FLAG_FLOATING);
#endif
    lv_obj_align(s_dock_active_bar, LV_ALIGN_BOTTOM_MID, 0, 4);

    lv_obj_set_style_text_color(s_dock_names[0], lv_color_hex(k_dock_icol[0]), 0);
    lv_obj_set_style_text_opa(s_dock_names[0], k_dock_iopa[0], 0);
}

/* ════════════════════════════════════════════════════════════════════
 * Public API
 * ════════════════════════════════════════════════════════════════════ */

lv_obj_t *ui_home_create(void)
{
    s_lbl_time = s_lbl_digi_time = s_lbl_digi_day = s_lbl_digi_date = NULL;
    s_lbl_temp = s_lbl_desc = s_lbl_location = NULL;
    s_ic_bluetooth = s_ic_wifi = s_ic_battery = NULL;
    s_dock_active_bar = NULL;
    s_dock_active_idx = 0;
    for (int i = 0; i < 6; i++) s_app_items[i] = NULL;
    for (int i = 0; i < 7; i++) { s_dock_items[i] = NULL; s_dock_names[i] = NULL; }

    if (s_clock_timer) { lv_timer_del(s_clock_timer); s_clock_timer = NULL; }

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0d1025), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    make_status_bar(scr);
    make_main_content(scr);
    make_dock(scr);

    s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);

    pr_info("[UI] home created\n");
    return scr;
}

void ui_home_update_clock(uint8_t h, uint8_t m, uint8_t s,
                          const char *date_str, const char *day_str)
{
    s_h = h; s_m = m; s_sec = s;
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)h, (unsigned)m);
    if (s_lbl_time)      lv_label_set_text(s_lbl_time,      buf);
    if (s_lbl_digi_time) lv_label_set_text(s_lbl_digi_time, buf);
    if (s_lbl_digi_date && date_str) lv_label_set_text(s_lbl_digi_date, date_str);
    if (s_lbl_digi_day  && day_str)  lv_label_set_text(s_lbl_digi_day,  day_str);
}

void ui_home_update_weather(const char *temp_str, const char *desc_str,
                            const char *location_str)
{
    if (s_lbl_temp     && temp_str)     lv_label_set_text(s_lbl_temp,     temp_str);
    if (s_lbl_desc     && desc_str)     lv_label_set_text(s_lbl_desc,     desc_str);
    if (s_lbl_location && location_str) lv_label_set_text(s_lbl_location, location_str);
}

void ui_home_update_status(bool bt, bool wifi, bool battery_ok)
{
    const lv_opa_t on = 178, off = 51;
    if (s_ic_bluetooth) lv_obj_set_style_text_opa(s_ic_bluetooth, bt         ? on : off, 0);
    if (s_ic_wifi)      lv_obj_set_style_text_opa(s_ic_wifi,      wifi       ? on : off, 0);
    if (s_ic_battery)   lv_obj_set_style_text_opa(s_ic_battery,   battery_ok ? on : off, 0);
}

void ui_home_set_app_cb(uint8_t idx, lv_event_cb_t cb)
{
    if (idx < 6 && s_app_items[idx])
        lv_obj_add_event_cb(s_app_items[idx], cb, LV_EVENT_CLICKED, NULL);
}

void ui_home_set_dock_cb(uint8_t idx, lv_event_cb_t cb)
{
    if (idx < 7 && s_dock_items[idx])
        lv_obj_add_event_cb(s_dock_items[idx], cb, LV_EVENT_CLICKED, NULL);
}

void ui_home_set_dock_active(uint8_t idx)
{
    if (idx >= 7 || !s_dock_active_bar) return;
    uint8_t prev = s_dock_active_idx;
    if (prev == idx) return;

    lv_obj_set_style_text_color(s_dock_names[prev], lv_color_white(), 0);
    lv_obj_set_style_text_opa(s_dock_names[prev], 140, 0);
    lv_obj_set_style_text_color(s_dock_names[idx], lv_color_hex(k_dock_icol[idx]), 0);
    lv_obj_set_style_text_opa(s_dock_names[idx], k_dock_iopa[idx], 0);

    lv_obj_set_parent(s_dock_active_bar, s_dock_items[idx]);
#ifdef LV_OBJ_FLAG_FLOATING
    lv_obj_add_flag(s_dock_active_bar, LV_OBJ_FLAG_FLOATING);
#endif
    lv_obj_align(s_dock_active_bar, LV_ALIGN_BOTTOM_MID, 0, 4);
    s_dock_active_idx = idx;
}
