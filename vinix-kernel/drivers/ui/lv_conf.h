/*
 * lv_conf.h — LVGL v8 configuration for Nothan-OS bare-metal
 * Display: 800×480 RGB565, USB HID mouse input.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ── Display ──────────────────────────────────────────────────────────── */
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0
#define LV_COLOR_SCREEN_TRANSP  0
#define LV_COLOR_MIX_ROUND_OFS  0
#define LV_COLOR_CHROMA_KEY     lv_color_hex(0xFF00FF)  /* magenta = cursor transparent */

/* ── Memory ───────────────────────────────────────────────────────────── */
#define LV_MEM_CUSTOM           0
#define LV_MEM_SIZE             (256 * 1024U)
#define LV_MEM_ADR              0
#define LV_MEMCPY_MEMSET_STD    1

/* ── HAL ──────────────────────────────────────────────────────────────── */
#define LV_DISP_DEF_REFR_PERIOD         30
#define LV_INDEV_DEF_READ_PERIOD        10      /* ms — faster for mouse */
#undef  LV_INDEV_DEF_GESTURE_LIMIT
#define LV_INDEV_DEF_GESTURE_LIMIT      30
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY 3
#define LV_DPI_DEF                      130
#define LV_TICK_CUSTOM                  0

/* ── Debug / logging ─────────────────────────────────────────────────── */
#define LV_USE_LOG          0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0

/* ── Assert ───────────────────────────────────────────────────────────── */
#define LV_USE_ASSERT_NULL          0
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* ── Compiler / attributes ───────────────────────────────────────────── */
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TASK_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN      __attribute__((aligned(4)))
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_ATTRIBUTE_DMA
#define LV_EXPORT_CONST_INT(int_value)
#define LV_USE_LARGE_COORD  0

/* ── Fonts ────────────────────────────────────────────────────────────── */
#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   1
#define LV_FONT_MONTSERRAT_12   0
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_26   0
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   1
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   1
#define LV_FONT_MONTSERRAT_12_SUBPX         0
#define LV_FONT_MONTSERRAT_28_COMPRESSED    0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0
#define LV_FONT_SIMSUN_16_CJK               0
#define LV_FONT_UNSCII_8    0
#define LV_FONT_UNSCII_16   0
#define LV_FONT_CUSTOM_DECLARE
#define LV_FONT_DEFAULT     &lv_font_montserrat_16

/* ── Text ─────────────────────────────────────────────────────────────── */
#define LV_TXT_ENC                          LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS                  " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN          0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD                    "#"
#define LV_USE_BIDI                         0
#define LV_USE_ARABIC_PERSIAN_CHARS         0

/* ── Widgets ──────────────────────────────────────────────────────────── */
#define LV_USE_ARC          1
#define LV_USE_BAR          0
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    0
#define LV_USE_CANVAS       1
#define LV_USE_CHECKBOX     0
#define LV_USE_DROPDOWN     0
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_LABEL_TEXT_SELECTION     0
#define LV_LABEL_LONG_TXT_HINT      0
#define LV_USE_LINE         1
#define LV_USE_ROLLER       0
#define LV_USE_SLIDER       0
#define LV_USE_SWITCH       0
#define LV_USE_TEXTAREA     1
#define LV_USE_TABLE        0

/* ── Extra / theme ────────────────────────────────────────────────────── */
#define LV_USE_THEME_DEFAULT    0
#define LV_THEME_DEFAULT_DARK   1
#define LV_THEME_DEFAULT_GROW   0
#define LV_USE_THEME_SIMPLE     0
#define LV_USE_THEME_MONO       1
#define LV_USE_THEME_BASIC      0

#define LV_USE_FLEX     1
#define LV_USE_GRID     0

#define LV_USE_ANIMIMG      0
#define LV_USE_CALENDAR     0
#define LV_USE_CHART        0
#define LV_USE_COLORWHEEL   0
#define LV_USE_IMGBTN       0
#define LV_USE_KEYBOARD     0
#define LV_USE_LED          0
#define LV_USE_LIST         0
#define LV_USE_MENU         0
#define LV_USE_METER        0
#define LV_USE_MSGBOX       0
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      1
#define LV_USE_TABVIEW      0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0

/* ── File system / images / demos — all off ──────────────────────────── */
#define LV_USE_FS_STDIO     0
#define LV_USE_FS_POSIX     0
#define LV_USE_FS_WIN32     0
#define LV_USE_FS_FATFS     0
#define LV_USE_PNG          0
#define LV_USE_BMP          0
#define LV_USE_SJPG         0
#define LV_USE_GIF          0
#define LV_USE_QRCODE       0
#define LV_USE_RLOTTIE      0
#define LV_USE_FFMPEG       0
#define LV_BUILD_EXAMPLES               0
#define LV_USE_DEMO_WIDGETS             0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER  0
#define LV_USE_DEMO_BENCHMARK           0
#define LV_USE_DEMO_STRESS              0
#define LV_USE_DEMO_MUSIC               0

#endif /* LV_CONF_H */
