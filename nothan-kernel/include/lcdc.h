/*
 * include/lcdc.h — AM335x LCDC raster-mode driver interface
 */

#ifndef LCDC_H
#define LCDC_H

#include "types.h"
#include "mmu.h"


/* LCDC base address (L4_PER domain) */
#define LCDC_BASE           0x4830E000

/* FB_PA_BASE is defined in mmu.h as part of the platform memory map */


/* Geometry accessors — used by the legacy boot path that touches the
 * framebuffer before fbdev geometry is published. New code should use
 * fb_get_width/height/buffer from fb.h instead. */
uint16_t *lcdc_get_framebuffer(void);
uint32_t lcdc_get_width(void);
uint32_t lcdc_get_height(void);
uint32_t lcdc_get_pitch(void);

#endif /* LCDC_H */
