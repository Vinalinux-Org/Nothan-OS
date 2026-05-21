/*
 * drivers/video/home_ui.c — Home screen click handler
 *
 * Kernel task: polls cursor + buttons at ~60fps.  Left-click on an app
 * icon opens a content panel; clicking [X] in the title bar closes it
 * and redraws the home screen.
 */

#include "types.h"
#include "fb.h"
#include "boot_screen.h"
#include "timer.h"
#include "vinix/mouse_cursor.h"

/* ---- Icon grid — must match boot_screen.c ---- */
#define ICON_R   55
#define N_ICONS  8

static const uint32_t ICON_CX[N_ICONS] = { 100, 300, 500, 700, 100, 300, 500, 700 };
static const uint32_t ICON_CY[N_ICONS] = { 184, 184, 184, 184, 407, 407, 407, 407 };

static const char *APP_NAME[N_ICONS] = {
    "Terminal", "Files", "Settings", "Network",
    "System",   "Tasks", "Clock",    "About",
};

static const uint16_t APP_COLOR[N_ICONS] = {
    FB_RGB(255, 193,   7),   /* Amber      — Terminal */
    FB_RGB( 76, 175,  80),   /* Green      — Files    */
    FB_RGB( 96, 125, 139),   /* Blue-Grey  — Settings */
    FB_RGB(  0, 150, 136),   /* Teal       — Network  */
    FB_RGB( 63,  81, 181),   /* Indigo     — System   */
    FB_RGB(156,  39, 176),   /* Purple     — Tasks    */
    FB_RGB(255,  87,  34),   /* Deep Orange— Clock    */
    FB_RGB(  0, 188, 212),   /* Cyan       — About    */
};

/* ---- Panel layout ---- */
#define PANEL_X    40
#define PANEL_Y    60
#define PANEL_W   720
#define PANEL_H   480
#define TITLE_H    44

#define CONTENT_X  PANEL_X
#define CONTENT_Y  (PANEL_Y + TITLE_H)
#define CONTENT_W  PANEL_W
#define CONTENT_H  (PANEL_H - TITLE_H)

/* Close button — rightmost 60px of title bar */
#define CLOSE_X   (PANEL_X + PANEL_W - 60)
#define CLOSE_Y    PANEL_Y
#define CLOSE_W    60
#define CLOSE_H    TITLE_H

static int ui_panel_open;

/* ---- Geometry helpers ---- */

static int in_circle(int32_t mx, int32_t my,
                     int32_t cx, int32_t cy, int32_t r)
{
    int32_t dx = mx - cx;
    int32_t dy = my - cy;
    return (dx * dx + dy * dy) <= (r * r);
}

static int in_rect(int32_t mx, int32_t my,
                   int32_t rx, int32_t ry, int32_t rw, int32_t rh)
{
    return mx >= rx && mx < rx + rw && my >= ry && my < ry + rh;
}

/* ---- Panel drawing ---- */

static void draw_title_bar(int idx)
{
    uint16_t tc = APP_COLOR[idx];
    fb_fillrect(PANEL_X, PANEL_Y, PANEL_W, TITLE_H, tc);

    uint32_t ty = PANEL_Y + (TITLE_H - FB_FONT_H * 2) / 2;
    fb_puts_scaled(PANEL_X + 16, ty, APP_NAME[idx], FB_WHITE, tc, 2);

    /* Close button — dark red band */
    fb_fillrect(CLOSE_X, CLOSE_Y, CLOSE_W, CLOSE_H, FB_RGB(180, 30, 30));
    uint32_t xw = FB_FONT_W * 2;
    uint32_t xt = CLOSE_X + (CLOSE_W - xw) / 2;
    fb_puts_scaled(xt, ty, "X", FB_WHITE, FB_RGB(180, 30, 30), 2);
}

static void draw_content_text(int idx)
{
    uint16_t bg  = FB_RGB(22, 22, 44);
    uint16_t txt = FB_WHITE;
    uint16_t dim = FB_RGB(160, 170, 190);
    uint16_t grn = FB_RGB(100, 220, 100);
    uint16_t yel = FB_YELLOW;

    fb_fillrect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, bg);
    fb_fillrect(CONTENT_X, CONTENT_Y, CONTENT_W, 1, FB_RGB(50, 60, 90));

    uint32_t cx   = CONTENT_X + 24;
    uint32_t cy0  = CONTENT_Y + 20;
    uint32_t lh   = FB_FONT_H + 6;

#define LINE(n, str, col) fb_puts(cx, cy0 + (n) * lh, str, col, bg)

    switch (idx) {
    case 0: /* Terminal */
        LINE( 0, "VinixOS Terminal  v0.1.0-alpha",       txt);
        LINE( 1, "Board: BeagleBone Black  (AM335x)",     dim);
        LINE( 2, "-----------------------------------",   dim);
        LINE( 3, "$ ls /",                               grn);
        LINE( 4, "  bin   dev   etc   proc   tmp",        txt);
        LINE( 5, "$ cat /etc/version",                   grn);
        LINE( 6, "  VinixOS 0.1.0-alpha  BeagleBone Black", txt);
        LINE( 7, "$ uname -a",                           grn);
        LINE( 8, "  VinixOS 0.1.0 armv7l ARM Cortex-A8", txt);
        LINE( 9, "$ ps",                                 grn);
        LINE(10, "  PID  NAME      STATE",               dim);
        LINE(11, "  0    idle      RUNNING",              txt);
        LINE(12, "  1    init      RUNNING",              txt);
        LINE(13, "  2    home-ui   RUNNING",              txt);
        LINE(14, "$ _",                                  FB_GREEN);
        break;

    case 1: /* Files */
        LINE( 0, "File Manager  —  /",                   txt);
        LINE( 1, "-----------------------------------",   dim);
        LINE( 2, "bin/",                                 yel);
        LINE( 3, "  sh   ls   cat   ps   echo",          dim);
        LINE( 4, "dev/",                                 yel);
        LINE( 5, "  tty0   mmcblk0   null   zero",       dim);
        LINE( 6, "etc/",                                 yel);
        LINE( 7, "  version   hostname   passwd",        dim);
        LINE( 8, "proc/",                                yel);
        LINE( 9, "  0/  1/  2/  cpuinfo  meminfo",       dim);
        LINE(10, "tmp/",                                 yel);
        LINE(12, "4 directories,  15 files  (FAT32)",    dim);
        break;

    case 2: /* Settings */
        LINE( 0, "System Settings",                      txt);
        LINE( 1, "-----------------------------------",   dim);
        LINE( 2, "Display     800x600  RGB565  HDMI",    dim);
        LINE( 3, "CPU         ARM Cortex-A8 @ 800 MHz",  dim);
        LINE( 4, "Memory      256 MB DDR3 @ 400 MHz",    dim);
        LINE( 5, "Storage     4 GB SD Card  (FAT32)",    dim);
        LINE( 6, "USB Host    HID Mouse  (boot proto)",  dim);
        LINE( 7, "I2C0        TDA19988 HDMI encoder",    dim);
        LINE( 8, "UART0       Console  115200 8N1",       dim);
        LINE( 9, "Timer       DMTimer2  10 ms tick",      dim);
        LINE(10, "Scheduler   Round-robin preemptive",   dim);
        break;

    case 3: /* Network */
        LINE( 0, "Network Status",                       txt);
        LINE( 1, "-----------------------------------",   dim);
        LINE( 2, "eth0     DOWN  (no cable)",            FB_RGB(220, 80, 80));
        LINE( 3, "lo       127.0.0.1 / 8",               dim);
        LINE( 4, "USB      HID Mouse  (no network)",     dim);
        LINE( 6, "Network stack not yet implemented",    dim);
        LINE( 7, "in this build.  Coming in P4.",        dim);
        break;

    case 4: /* System */
        LINE( 0, "System Monitor",                       txt);
        LINE( 1, "-----------------------------------",   dim);
        LINE( 2, "VinixOS  0.1.0-alpha",                 FB_RGB(100, 200, 255));
        LINE( 3, "Board:   BeagleBone Black  Rev.C",     dim);
        LINE( 4, "SoC:     Texas Instruments  AM3358",   dim);
        LINE( 5, "-----------------------------------",   dim);
        LINE( 6, "Uptime:   00:01:23",                   dim);
        LINE( 7, "Tasks:    3  (idle / init / home-ui)", dim);
        LINE( 8, "Heap:     ~1.2 MB used / 256 MB",      dim);
        LINE( 9, "CPU load: < 5 %",                      dim);
        break;

    case 5: /* Tasks */
        LINE( 0, "Task Manager",                         txt);
        LINE( 1, "-----------------------------------",   dim);
        LINE( 2, "PID  NAME       STATE",                FB_RGB(150, 180, 220));
        LINE( 3, "0    idle       RUNNING",               dim);
        LINE( 4, "1    init       RUNNING",               dim);
        LINE( 5, "2    home-ui    RUNNING",               grn);
        LINE( 7, "3 tasks  —  round-robin  10 ms tick",  dim);
        break;

    case 6: /* Clock */
        {
            /* "10:07" at scale 4: 5*8*4=160px wide, 16*4=64px tall */
            uint32_t cw = 5 * FB_FONT_W * 4;
            uint32_t ch = FB_FONT_H * 4;
            uint32_t tx = CONTENT_X + (CONTENT_W - cw) / 2;
            uint32_t ty = CONTENT_Y + (CONTENT_H - ch) / 2 - 30;
            fb_puts_scaled(tx, ty, "10:07", txt, bg, 4);

            uint32_t mid = CONTENT_X + CONTENT_W / 2;
            fb_puts(mid - 4 * FB_FONT_W,  ty + ch + 22, "Wednesday",    dim, bg);
            fb_puts(mid - 6 * FB_FONT_W,  ty + ch + 44, "May 21, 2026", dim, bg);
        }
        break;

    case 7: /* About */
        {
            uint32_t aw = 7 * FB_FONT_W * 3;
            fb_puts_scaled(CONTENT_X + (CONTENT_W - aw) / 2,
                           CONTENT_Y + 28, "VinixOS",
                           FB_RGB(100, 200, 255), bg, 3);
        }
        LINE( 5, "Version:    0.1.0-alpha",              dim);
        LINE( 6, "Platform:   BeagleBone Black (ARMv7-A)", dim);
        LINE( 7, "SoC:        Texas Instruments AM335x", dim);
        LINE( 8, "Developer:  Vinalinux",                dim);
        LINE( 9, "Purpose:    Vietnamese OS Research",   dim);
        LINE(10, "License:    Original Work",            dim);
        break;
    }

#undef LINE
}

static void panel_open(int idx)
{
    cursor_hide();

    /* Dim the screen behind the panel */
    fb_clear(FB_RGB(4, 4, 10));

    /* Drop shadow */
    fb_fillrect(PANEL_X + 5, PANEL_Y + 5, PANEL_W, PANEL_H, FB_RGB(2, 2, 6));

    draw_title_bar(idx);
    draw_content_text(idx);

    cursor_show();
    ui_panel_open = 1;
}

static void panel_close(void)
{
    cursor_hide();
    show_home();
    cursor_show();
    ui_panel_open = 0;
}

/* ---- Main task loop ---- */

void home_ui_run(void)
{
    /* Cursor late_initcall runs at level 7, just before scheduler_start.
     * A short yield ensures it has fully populated fb_buf before we poll. */
    delay_ms(100);

    uint8_t prev_btn = 0;

    while (1) {
        delay_ms(16);

        int32_t mx, my;
        cursor_get_pos(&mx, &my);
        uint8_t btn = cursor_get_buttons();

        uint8_t left_clicked = (btn & 1) && !(prev_btn & 1);
        prev_btn = btn;

        if (!left_clicked)
            continue;

        if (ui_panel_open) {
            if (in_rect(mx, my, CLOSE_X, CLOSE_Y, CLOSE_W, CLOSE_H))
                panel_close();
        } else {
            for (int i = 0; i < N_ICONS; i++) {
                if (in_circle(mx, my,
                              (int32_t)ICON_CX[i],
                              (int32_t)ICON_CY[i],
                              ICON_R)) {
                    panel_open(i);
                    break;
                }
            }
        }
    }
}
