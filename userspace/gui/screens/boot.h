#ifndef __GUI_BOOT_H
#define __GUI_BOOT_H

#include "lvgl/lvgl.h"

/* Total boot duration (must be ≥ BAR_FILL_MS in boot.c + render margin). */
#define BOOT_DURATION_MS  8700

/*
 * boot_create() - MyNuong splash (wordmark + tagline + progress bar)
 * built into @screen. A nav builder; @arg is unused. The first screen
 * shown, replaced by the home launcher once boot completes.
 */
void boot_create(lv_obj_t *screen, void *arg);

#endif
