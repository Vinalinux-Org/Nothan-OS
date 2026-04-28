/* ============================================================
 * boot_screen.c — Boot Log + Splash Screen
 * Draws directly to framebuffer via fb.h primitives.
 * Completely independent of UART — no serial dependency.
 * ============================================================ */

#include "types.h"
#include "boot_screen.h"
#include "fb.h"
#include "lcdc.h"
#include "timer.h"

/* ============================================================
 * Screen 1: Boot Log
 * ============================================================ */

static void show_boot_log(void)
{
    uint32_t sw = lcdc_get_width();
    uint32_t sh = lcdc_get_height();
    uint16_t bg       = FB_BLACK;
    uint16_t border   = FB_RGB(180, 180, 180);
    uint16_t ok_col   = FB_RGB(0, 220, 0);
    uint16_t txt_col  = FB_WHITE;
    uint16_t done_col = FB_RGB(220, 200, 60);
    uint16_t title_col= FB_RGB(180, 180, 180);

    fb_clear(bg);

    /* Thin border inset 20px */
    uint32_t bx = 20, by = 20, bw = sw - 40, bh = sh - 40;
    fb_fillrect(bx, by, bw, 1, border);
    fb_fillrect(bx, by + bh - 1, bw, 1, border);
    fb_fillrect(bx, by, 1, bh, border);
    fb_fillrect(bx + bw - 1, by, 1, bh, border);

    uint32_t cx = bx + 24;
    uint32_t cy = by + 20;
    uint32_t line_h = FB_FONT_H + 4;

    /* Title */
    fb_puts(cx, cy, "VinixOS Boot", title_col, bg);
    cy += line_h + 8;

    /* Status lines — appear one by one */
    static const char *labels[] = {
        "CPU: ARM Cortex-A8 800MHz (ARMv7-A)",
        "Board: BeagleBone Black Rev.C",
        "SoC: Texas Instruments AM335x",
        "Memory: 256MB DDR3 @ 400MHz",
        "MMU: Virtual memory enabled",
        "Watchdog: Disabled",
        "I2C0: Bus initialized (100kHz)",
        "LCDC: Framebuffer 800x600 RGB565",
        "DPLL: Display PLL locked @ 40MHz",
        "HDMI: TDA19988 TMDS link active",
        "INTC: Interrupt controller ready",
        "Timer: DMTimer2 (10ms preemptive tick)",
        "UART0: Console 115200 8N1",
        "Storage: SD/MMC card mounted",
        "VFS: Virtual filesystem mounted at /",
        "Scheduler: Round-robin preemptive ready",
    };
    uint32_t n_labels = sizeof(labels) / sizeof(labels[0]);

    for (uint32_t i = 0; i < n_labels; i++) {
        fb_puts(cx, cy, "[ ", txt_col, bg);
        fb_puts(cx + 2 * FB_FONT_W, cy, "OK", ok_col, bg);
        fb_puts(cx + 4 * FB_FONT_W, cy, " ]  ", txt_col, bg);
        fb_puts(cx + 8 * FB_FONT_W, cy, labels[i], txt_col, bg);
        cy += line_h;
        delay_ms(300);
    }

    /* Boot complete */
    cy += line_h / 2;
    fb_puts(cx, cy, "Boot complete. All services ready.", done_col, bg);

    delay_ms(1000);
}

/* ============================================================
 * Screen 2: Splash
 * ============================================================ */

static void show_splash(void)
{
    uint32_t sw = lcdc_get_width();
    uint32_t sh = lcdc_get_height();
    uint16_t bg = FB_RGB(10, 10, 46);

    fb_clear(bg);

    /* "VINIX OS" — scale 6 */
    uint32_t scale = 6;
    uint32_t tw = 8 * FB_FONT_W * scale;
    uint32_t th = FB_FONT_H * scale;
    uint32_t tx = (sw - tw) / 2;
    uint32_t ty = sh / 3 - th / 2;
    fb_puts_scaled(tx, ty, "VINIX OS", FB_WHITE, bg, scale);

    /* Subtitle 1 */
    const char *sub1 = "Reference ARM Software Platform";
    uint32_t s1w = 31 * FB_FONT_W;
    fb_puts((sw - s1w) / 2, ty + th + 30, sub1, FB_RGB(170, 170, 170), bg);

    /* Subtitle 2 */
    const char *sub2 = "Developed by Vinalinux - Vietnamese Operating System";
    uint32_t s2w = 52 * FB_FONT_W;
    fb_puts((sw - s2w) / 2, ty + th + 56, sub2, FB_RGB(100, 200, 200), bg);

    /* "Starting environment" + dots animation */
    uint32_t dots_y = sh * 2 / 3 + 20;
    const char *msg = "Starting environment";
    uint32_t mw = 20 * FB_FONT_W;
    uint32_t mx = (sw - (mw + 3 * FB_FONT_W)) / 2;
    uint16_t dot_col = FB_RGB(136, 136, 136);

    fb_puts(mx, dots_y, msg, dot_col, bg);

    uint32_t dot_x = mx + mw;
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int dots = 1; dots <= 3; dots++) {
            fb_fillrect(dot_x, dots_y, 3 * FB_FONT_W, FB_FONT_H, bg);
            for (int d = 0; d < dots; d++)
                fb_draw_char(dot_x + d * FB_FONT_W, dots_y, '.', dot_col, bg);
            delay_ms(500);
        }
    }
}

/* ============================================================
 * Screen 3: Home — Professional App Launcher
 *
 * Layout (800x600):
 *   Status bar  y=0..43    clock | VINIX OS | wifi + battery
 *   Grid        y=44..575  4 cols x 2 rows, icon r=55px
 *   Footer      y=576..599 slim bar, version only
 *
 * Grid geometry (equal spacing top/mid/bottom = 85px each):
 *   Col centers: x = 100, 300, 500, 700
 *   Row centers: y = 184, 407
 *
 * Colors: Material Design palette (lv_palette.c reference)
 * ============================================================ */

/* App icon descriptor */
struct app_icon {
    const char *label;      /* Label shown below circle         */
    const char *symbol;     /* Glyph drawn inside circle        */
    uint8_t     sym_scale;  /* Font scale: 2 = 16x32, 3 = 24x48 */
    uint16_t    color;      /* Circle fill color (RGB565)       */
};

/* Minimal strlen — no stdlib in bare-metal */
static uint32_t icon_slen(const char *s)
{
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}

/* ---- Status bar widgets ----------------------------------------- */

/* WiFi signal: 3 ascending vertical bars, 11×12px total
 *   Bar 1: w=3 h=4  (weak)
 *   Bar 2: w=3 h=8  (medium)
 *   Bar 3: w=3 h=12 (strong, active)
 * All bars lit = full signal */
static void draw_wifi(uint32_t x, uint32_t y, uint16_t active, uint16_t dim)
{
    fb_fillrect(x,     y + 8, 3, 4,  dim);     /* bar 1 — weak   */
    fb_fillrect(x + 4, y + 4, 3, 8,  active);  /* bar 2 — medium */
    fb_fillrect(x + 8, y,     3, 12, active);  /* bar 3 — strong */
}

/* Battery icon: body 22×12px + nub 3×6px on right = 25×12px total
 *   level: 0-100 percentage fill
 *   Hollow interior filled with bar_bg to show remaining space */
static void draw_battery(uint32_t x, uint32_t y,
                         uint32_t level, uint16_t color, uint16_t bar_bg)
{
    /* Outer body */
    fb_fillrect(x,      y,      22, 12, color);
    fb_fillrect(x + 1,  y + 1,  20, 10, bar_bg);  /* hollow interior */

    /* Terminal nub */
    fb_fillrect(x + 22, y + 3,  3, 6, color);

    /* Charge fill — 16px usable width inside body */
    uint32_t fill_w = (level * 16) / 100;
    if (fill_w > 0)
        fb_fillrect(x + 2, y + 2, fill_w, 8, color);
}

/* Page indicator: 3 dots centered at bottom (● ○ ○)
 *   active = current page (bright), inactive = dim
 *   dot radius 4px, spacing 16px center-to-center */
static void draw_page_dots(uint32_t cx, uint32_t cy,
                           uint16_t active, uint16_t inactive)
{
    fb_fillcircle(cx - 16, cy, 4, active);    /* ● page 1 — current */
    fb_fillcircle(cx,      cy, 4, inactive);  /* ○ page 2           */
    fb_fillcircle(cx + 16, cy, 4, inactive);  /* ○ page 3           */
}

/* ---- Icon renderer ---------------------------------------------- */

/* Draw one app icon:
 *   1. Drop shadow (offset +2,+3, very dark) — gives depth
 *   2. Main colored circle
 *   3. Symbol text centered in circle
 *   4. Label text below circle */
static void draw_app_icon(uint32_t cx, uint32_t cy, uint32_t r,
                          const struct app_icon *icon, uint16_t bg)
{
    /* Drop shadow */
    fb_fillcircle(cx + 2, cy + 3, r, FB_RGB(8, 8, 20));

    /* Main circle */
    fb_fillcircle(cx, cy, r, icon->color);

    /* Symbol centered inside circle */
    uint32_t sym_w = icon_slen(icon->symbol) * FB_FONT_W * icon->sym_scale;
    uint32_t sym_h = FB_FONT_H * icon->sym_scale;
    fb_puts_scaled(cx - sym_w / 2, cy - sym_h / 2,
                   icon->symbol, FB_WHITE, icon->color, icon->sym_scale);

    /* Label below — white on main background */
    uint32_t lbl_w = icon_slen(icon->label) * FB_FONT_W;
    fb_puts(cx - lbl_w / 2, cy + r + 12, icon->label, FB_WHITE, bg);
}

/* ---- Main home screen ------------------------------------------- */

static void show_home(void)
{
    uint32_t sw = lcdc_get_width();   /* 800 */
    uint32_t sh = lcdc_get_height();  /* 600 */

    /* ---- Color palette ---- */
    uint16_t bg_main  = FB_RGB( 18,  18,  38);  /* deep navy — wallpaper  */
    uint16_t bar_bg   = FB_RGB( 12,  12,  26);  /* status/footer bar      */
    uint16_t bar_sep  = FB_RGB( 40,  50,  75);  /* 1px separator line     */
    uint16_t bar_txt  = FB_RGB(200, 210, 230);  /* bar primary text       */
    uint16_t clk_col  = FB_WHITE;               /* clock digits           */
    uint16_t sig_col  = FB_RGB(  0, 200,  80);  /* wifi/battery active    */
    uint16_t sig_dim  = FB_RGB( 50,  60,  80);  /* wifi bar 1 dimmed      */
    uint16_t dot_dim  = FB_RGB( 55,  65,  90);  /* page dot inactive      */

    /* ---- Icon data (Material Design palette, LVGL lv_palette.c) ---- */
    static const struct app_icon icons[8] = {
        { "Terminal", ">_", 2, FB_RGB(255, 193,   7) }, /* Amber      */
        { "Files",    "F",  3, FB_RGB( 76, 175,  80) }, /* Green      */
        { "Settings", "#",  3, FB_RGB( 96, 125, 139) }, /* Blue-Grey  */
        { "Network",  "~",  3, FB_RGB(  0, 150, 136) }, /* Teal       */
        { "System",   "S",  3, FB_RGB( 63,  81, 181) }, /* Indigo     */
        { "Tasks",    "T",  3, FB_RGB(156,  39, 176) }, /* Purple     */
        { "Clock",    "O",  3, FB_RGB(255,  87,  34) }, /* Deep Orange*/
        { "About",    "?",  3, FB_RGB(  0, 188, 212) }, /* Cyan       */
    };

    /* ---- Grid geometry ---- */
    static const uint32_t cx_list[4] = { 100, 300, 500, 700 };
    static const uint32_t cy_list[2] = { 184, 407 };
    uint32_t icon_r  = 55;
    uint32_t bar_h   = 44;   /* status bar height */
    /* ================================================================
     * Background
     * ================================================================ */
    fb_clear(bg_main);

    /* ================================================================
     * Status bar  y = 0..43
     *   Left  : "00:00" (clock placeholder — no RTC)
     *   Center: "VINIX OS"
     *   Right : wifi bars + battery icon
     * ================================================================ */
    fb_fillrect(0, 0, sw, bar_h, bar_bg);
    fb_fillrect(0, bar_h - 1, sw, 1, bar_sep);

    /* Clock — scale 2 (80×32px), vertically centered */
    uint32_t clk_ty = (bar_h - FB_FONT_H * 2) / 2;
    fb_puts_scaled(16, clk_ty, "10:07", clk_col, bar_bg, 2);

    /* "VINIX OS" — scale 2, centered */
    uint32_t ttw = 8 * FB_FONT_W * 2;
    uint32_t tth = FB_FONT_H * 2;
    fb_puts_scaled((sw - ttw) / 2, (bar_h - tth) / 2,
                   "VINIX OS", bar_txt, bar_bg, 2);

    /* Right-side indicators, right-margin 16px:
     *   battery: 25px wide (body 22 + nub 3)  → x = sw-16-25 = 759
     *   gap 8px
     *   wifi   : 11px wide                    → x = sw-16-25-8-11 = 740  */
    uint32_t ind_y   = (bar_h - 12) / 2;  /* vertically centered, 12px tall */
    uint32_t batt_x  = sw - 16 - 25;       /* 759 */
    uint32_t wifi_x  = batt_x - 8 - 11;    /* 740 */

    draw_wifi(wifi_x, ind_y, sig_col, sig_dim);
    draw_battery(batt_x, ind_y, 80, sig_col, bar_bg);

    /* ================================================================
     * Footer  y = 576..599  (slim 24px bar, version only)
     * ================================================================ */
    /* ================================================================
     * Icon grid — 4 cols × 2 rows
     *
     * Grid area: y=44..599 (556px, no footer bar)
     * Equal spacing top/mid/bot ≈ 85px:
     *   row 0 cy = 44 + 85 + 55 = 184
     *   row 1 cy = 184 + 110 + 28 + 85 + 55 ≈ 407 (label bottom ~490)
     *   page dots at cy = 582, gap above ~88px ✓
     * ================================================================ */
    for (uint32_t row = 0; row < 2; row++) {
        for (uint32_t col = 0; col < 4; col++) {
            draw_app_icon(cx_list[col], cy_list[row], icon_r,
                          &icons[row * 4 + col], bg_main);
        }
    }

    /* ================================================================
     * Page indicator dots — centered bottom  (● ○ ○)
     * cy = sh - 18 = 582, gap from last label ~88px
     * ================================================================ */
    draw_page_dots(sw / 2, sh - 18, FB_WHITE, dot_dim);
}

/* ============================================================
 * Public API
 * ============================================================ */

void boot_screen_run(void)
{
    show_boot_log();
    show_splash();
    show_home();
}
