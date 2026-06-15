#ifndef __USER_FB_H
#define __USER_FB_H

#include "ioctl.h"

#define FB_MAGIC     'F'
#define FB_GET_INFO  _IOR(FB_MAGIC, 0, struct fb_info)
#define FB_FLUSH     _IOW(FB_MAGIC, 1, struct fb_flush)

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

#endif /* __USER_FB_H */
