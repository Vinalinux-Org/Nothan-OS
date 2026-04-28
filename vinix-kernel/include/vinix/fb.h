/*
 * include/vinix/fb.h — framebuffer subsystem interface
 *
 * fb_info wraps a framebuffer and its ops vtable.
 * Drivers fill fb_info and call register_framebuffer().
 */

#ifndef VINIX_FB_H
#define VINIX_FB_H

#include "types.h"

struct fb_info;

struct fb_var_screeninfo {
    uint32_t  xres;
    uint32_t  yres;
    uint32_t  bits_per_pixel;
    uint32_t  pixclock;       /* picoseconds per pixel */
    uint32_t  left_margin, right_margin, upper_margin, lower_margin;
    uint32_t  hsync_len, vsync_len;
};

struct fb_fix_screeninfo {
    char      id[16];
    uint32_t  smem_start;     /* physical address of frame buffer */
    uint32_t  smem_len;
    uint32_t  line_length;
};

struct fb_ops {
    int (*fb_check_var)(struct fb_var_screeninfo *var, struct fb_info *info);
    int (*fb_set_par)  (struct fb_info *info);
    int (*fb_blank)    (int blank, struct fb_info *info);
};

struct fb_info {
    int                          node;
    struct fb_var_screeninfo     var;
    struct fb_fix_screeninfo     fix;
    void                        *screen_base;   /* virt mapping */
    const struct fb_ops         *fbops;
    void                        *priv;
};

int register_framebuffer(struct fb_info *fb);
int unregister_framebuffer(struct fb_info *fb);

#endif /* VINIX_FB_H */
