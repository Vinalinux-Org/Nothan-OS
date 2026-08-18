#ifndef _NOTHAN_FB_H
#define _NOTHAN_FB_H

#include <nothan/ioctl.h>

#define FB_MAGIC        'F'
#define FB_GET_INFO     _IOR(FB_MAGIC, 0, struct fb_info)
#define FB_FLUSH        _IOW(FB_MAGIC, 1, struct fb_flush)
#define FB_FLIP         _IO(FB_MAGIC,  2)

struct fb_info {
	int width;
	int height;
	int bpp;
};

struct fb_flush {
	int x1, y1;
	int x2, y2;
	unsigned long data;
	unsigned int  len;
};

/**
 * struct fb_surface - the scanout buffer, for a writer inside the kernel
 * @pixels: RGB565, @width * @height, row major, no padding between rows
 * @width:  pixels
 * @height: pixels
 *
 * Every way into the framebuffer so far has come from userspace through the
 * FB_FLUSH ioctl, which copies a same-sized rectangle in.  A kernel task
 * painting received video cannot use that: it scales as it writes, so going
 * through flush would mean scaling into an 800x480 staging buffer and then
 * copying those 768 KB again — 46 MB/s of copying a second, to avoid handing
 * out a pointer.
 *
 * So the surface is handed out.  It is the one buffer the raster DMA scans;
 * there is no back buffer to swap, which is why a writer has to expect
 * tearing and why fb_sync() exists.
 */
struct fb_surface {
	u16		*pixels;
	unsigned int	width;
	unsigned int	height;
};

struct fb_ops {
	void (*flush)(int x1, int y1, int x2, int y2,
		      const void *data, unsigned int len);
	void (*flip)(void);

	/* Where the scanout buffer is.  Returns 0 on success. */
	int  (*get_surface)(struct fb_surface *s);

	/* Push the CPU's writes out to where the raster DMA reads them. */
	void (*sync)(void);
};

void fb_register_ops(struct fb_ops *ops);

/*
 * The same two, for callers inside the kernel.  Return -1 if no display
 * backend is registered, which is the ordinary case on a build with
 * CONFIG_VIDEO off rather than an error.
 */
int  fb_get_surface(struct fb_surface *s);
int  fb_sync(void);

#endif /* _NOTHAN_FB_H */
