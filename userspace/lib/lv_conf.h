/**
 * lv_conf.h — LVGL configuration for NothanOS
 * Target: ARM Cortex-A8 (BeagleBone Black), bare-metal
 */

#if 1

#ifndef LV_CONF_H
#define LV_CONF_H

/* Color depth: 16 = RGB565 */
#define LV_COLOR_DEPTH 16

/*=========================
 * STDLIB
 *=========================*/
/* Use LVGL built-in allocator — no need for userspace malloc */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

/* 128 KB heap for LVGL */
#define LV_MEM_SIZE (128 * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR 0

/*=========================
 * HAL
 *=========================*/
#define LV_DEF_REFR_PERIOD  33      /* ~30 fps */
#define LV_DPI_DEF          130

/*=========================
 * OS
 *=========================*/
#define LV_USE_OS   LV_OS_NONE

/*=========================
 * RENDERING
 *=========================*/
#define LV_DRAW_BUF_STRIDE_ALIGN    4
#define LV_DRAW_BUF_ALIGN           4
#define LV_DRAW_TRANSFORM_USE_MATRIX 0
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (16 * 1024)
#define LV_DRAW_THREAD_STACK_SIZE     (8 * 1024)

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
    #define LV_DRAW_SW_SUPPORT_RGB565       1
    #define LV_DRAW_SW_SUPPORT_RGB565A8     0
    #define LV_DRAW_SW_SUPPORT_RGB888       0
    #define LV_DRAW_SW_SUPPORT_XRGB8888     0
    #define LV_DRAW_SW_SUPPORT_ARGB8888     0
    #define LV_DRAW_SW_SUPPORT_L8           0
    #define LV_DRAW_SW_SUPPORT_AL88         0
    #define LV_DRAW_SW_SUPPORT_A8           1
    #define LV_DRAW_SW_SUPPORT_I1           0
    #define LV_DRAW_SW_DRAW_UNIT_CNT        1
    #define LV_USE_DRAW_ARM2D_SYNC          0
    #define LV_USE_NATIVE_HELIUM_ASM        0
    #define LV_DRAW_SW_COMPLEX              1
    #if LV_DRAW_SW_COMPLEX == 1
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE    0
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE    4
    #endif
    #define LV_USE_DRAW_SW_ASM  LV_DRAW_SW_ASM_NONE
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0
#endif

/* Disable GPU backends */
#define LV_USE_DRAW_VGLITE  0
#define LV_USE_DRAW_PXP     0
#define LV_USE_DRAW_DAVE2D  0
#define LV_USE_DRAW_SDL     0
#define LV_USE_DRAW_VG_LITE 0

/*=========================
 * LOGGING & DEBUG
 *=========================*/
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_ASSERT_HANDLER_INCLUDE   <stdint.h>
#define LV_ASSERT_HANDLER           while(1);

#define LV_USE_REFR_DEBUG           0
#define LV_USE_LAYER_DEBUG          0
#define LV_USE_PARALLEL_DRAW_DEBUG  0

/*=========================
 * MISC
 *=========================*/
#define LV_ENABLE_GLOBAL_CUSTOM     0
#define LV_CACHE_DEF_SIZE           0
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
#define LV_GRADIENT_MAX_STOPS       2
#define LV_COLOR_MIX_ROUND_OFS      0
#define LV_OBJ_STYLE_CACHE          0
#define LV_USE_OBJ_ID               0
#define LV_OBJ_ID_AUTO_ASSIGN       0
#define LV_USE_OBJ_ID_BUILTIN       1
#define LV_USE_OBJ_PROPERTY         0
#define LV_USE_OBJ_PROPERTY_NAME    0
#define LV_USE_GESTURE_RECOGNITION  0

/*=========================
 * FONT
 *=========================*/
#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   0
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_26   0
#define LV_FONT_MONTSERRAT_28   0
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   0
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   0

#define LV_FONT_DEFAULT         &lv_font_montserrat_14
#define LV_FONT_FMT_TXT_LARGE   0
#define LV_USE_FONT_PLACEHOLDER 1
#define LV_USE_FONT_COMPRESSED  0
#define LV_USE_FONT_SUBPX       0

/*=========================
 * TEXT
 *=========================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_)!?\n"
#define LV_TXT_LINE_BREAK_LONG_LEN      0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 1
#define LV_TXT_COLOR_CMD "#"
#define LV_USE_BIDI         0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*=========================
 * WIDGETS
 *=========================*/
#define LV_USE_ANIMIMAGE    0
#define LV_USE_ARC          0
#define LV_USE_BAR          1
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       0
#define LV_USE_CHART        0
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMAGE        1
#define LV_USE_IMAGEBUTTON  0
#define LV_USE_KEYBOARD     0
#define LV_USE_LABEL        1
#define LV_USE_LED          0
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_MENU         1
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       0
#define LV_USE_SCALE        0
#define LV_USE_SLIDER       1
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      0
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        0
#define LV_USE_TABVIEW      1
#define LV_USE_TEXTAREA     1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          0

/*=========================
 * THEMES
 *=========================*/
#define LV_USE_THEME_DEFAULT    1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK       0
    #define LV_THEME_DEFAULT_GROW       0
    #define LV_THEME_DEFAULT_TRANSITION_TIME 0  /* no animation */
#endif
#define LV_USE_THEME_SIMPLE     1
#define LV_USE_THEME_MONO       1

/*=========================
 * LAYOUTS
 *=========================*/
#define LV_USE_FLEX     1
#define LV_USE_GRID     1

/*=========================
 * DISABLE UNUSED EXTRAS
 *=========================*/
#define LV_USE_SNAPSHOT         0
#define LV_USE_SYSMON           0
#define LV_USE_PROFILER         0
#define LV_USE_MONKEY           0
#define LV_USE_GRIDNAV          0
#define LV_USE_FRAGMENT         0
#define LV_USE_IMGFONT          0
#define LV_USE_OBSERVER         0
#define LV_USE_IME_PINYIN       0
#define LV_USE_FILE_EXPLORER    0
#define LV_USE_LODEPNG          0
#define LV_USE_LIBPNG           0
#define LV_USE_BMP              0
#define LV_USE_TJPGD            0
#define LV_USE_LIBJPEG_TURBO   0
#define LV_USE_GIF              0
#define LV_USE_QRCODE           0
#define LV_USE_BARCODE          0
#define LV_USE_FFMPEG           0
#define LV_USE_FREETYPE         0
#define LV_USE_TINY_TTF         0
#define LV_USE_RLOTTIE          0
#define LV_USE_THORVG           0
#define LV_USE_LZ4              0
#define LV_USE_VECTOR_GRAPHIC   0
#define LV_USE_DRAW_TURBO_JPEG  0

/* Disable all built-in drivers */
#define LV_USE_SDL              0
#define LV_USE_X11              0
#define LV_USE_WAYLAND          0
#define LV_USE_WINDOWS          0
#define LV_USE_OPENGLES         0
#define LV_USE_NUTTX            0
#define LV_USE_EVDEV            0
#define LV_USE_LIBINPUT         0

#define LV_USE_DEMO_WIDGETS         0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK       0
#define LV_USE_DEMO_RENDER          0
#define LV_USE_DEMO_STRESS          0
#define LV_USE_DEMO_MUSIC           0
#define LV_USE_DEMO_FLEX_LAYOUT     0
#define LV_USE_DEMO_MULTILANG       0
#define LV_USE_DEMO_TRANSFORM       0
#define LV_USE_DEMO_SCROLL          0
#define LV_USE_DEMO_VECTOR_GRAPHIC  0

#endif /* LV_CONF_H */
#endif /* End of "Content enable" */
