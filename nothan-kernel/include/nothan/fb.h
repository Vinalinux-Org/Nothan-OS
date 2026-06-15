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

struct fb_ops {
	void (*flush)(int x1, int y1, int x2, int y2,
		      const void *data, unsigned int len);
	void (*flip)(void);
};

void fb_register_ops(struct fb_ops *ops);

#endif /* _NOTHAN_FB_H */
