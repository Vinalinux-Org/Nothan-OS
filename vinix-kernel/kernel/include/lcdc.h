/* ============================================================
 * lcdc.h
 * ------------------------------------------------------------
 * AM335x LCDC raster-mode driver interface.
 * ============================================================ */

#ifndef LCDC_H
#define LCDC_H

#include "types.h"
#include "mmu.h"

/* ============================================================
 * LCDC Configuration
 * ============================================================ */

/* LCDC base address (L4_PER domain) */
#define LCDC_BASE           0x4830E000

/* FB_PA_BASE is defined in mmu.h as part of the platform memory map */

/* ============================================================
 * Public API
 * ============================================================ */

/* Configures raster but does NOT start output — call
 * lcdc_start_raster() afterward.
 * Precondition: mmu_init() + i2c_init() done, framebuffer PA mapped. */
void lcdc_init(void);

/* Precondition: TDA19988 fully configured — HDMI must be ready to
 * receive pixel data the instant raster scan starts. */
void lcdc_start_raster(void);

/* Registers the LCDC framebuffer with fbdev as struct fb_info.
 * Call after lcdc_start_raster() so fb_pixels is valid. */
void lcdc_register_fb(void);

/* Returns the first pixel address (palette header is skipped). */
uint16_t *lcdc_get_framebuffer(void);

uint32_t lcdc_get_width(void);
uint32_t lcdc_get_height(void);
uint32_t lcdc_get_pitch(void);

#endif /* LCDC_H */
