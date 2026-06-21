#ifndef __GUI_BOOT_H
#define __GUI_BOOT_H

#include "lvgl/lvgl.h"

/* Total boot duration (must be ≥ BAR_FILL_MS in boot.c + render margin). */
#define BOOT_DURATION_MS  7200

/*
 * boot_create() - MiNuong splash (wordmark + tagline + progress bar)
 * built into @screen. A nav builder; @arg is unused. The first screen
 * shown, replaced by the home launcher once boot completes.
 */
void boot_create(lv_obj_t *screen, void *arg);

#endif
