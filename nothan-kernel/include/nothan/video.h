#ifndef _NOTHAN_VIDEO_H
#define _NOTHAN_VIDEO_H

/*
 * include/nothan/video.h - the boundary between where frames come from and
 * everything that does something with them
 *
 * This seam exists before the thing behind it does, and that is the point.  A
 * call needs a camera, the camera will be a USB webcam, and the USB host on
 * this chip is a full-speed PIO driver that talks to exactly one hardcoded
 * touchscreen: high speed unproven, isochronous transfers absent, CPPI 4.1 DMA
 * absent, and no UVC class driver at all.  That is the longest piece of work
 * left in the whole video path, and none of the rest of it should be waiting.
 *
 * So the rest is built against this line, with a generated pattern standing in
 * for the camera.  When UVC lands it registers here and nothing above changes —
 * the same trade netdev.h makes for links, and made for the same reason: the
 * work that takes the time sits above the seam.
 *
 * A frame is borrowed, not copied out.  @capture hands back a pointer that
 * stays valid until @release, exactly as udp_recv() and ring.h do, because a
 * camera's frames arrive by DMA into buffers the driver owns and the only way
 * to read them without a copy is to read them where they landed.  At 400x240
 * that copy would be 192 KB thirty times a second.
 *
 * One source.  A box with one camera does not need a list, and the second one —
 * a screen share beside the camera — is a smaller change than the generality
 * saved by not writing it now.
 */

#include <nothan/types.h>

/*
 * RGB565, and no format field to say so.
 *
 * A format field is only worth its weight when something chooses between
 * values, and everything downstream of here — the framebuffer, the LCDC, the
 * wire format — is RGB565 already.  A webcam that offers YUYV or MJPEG instead
 * converts in its own driver, below this line, because that is where the
 * knowledge of what the hardware emits belongs.  The day two formats genuinely
 * reach this line, this comment is the argument for adding the field.
 */
#define VIDEO_BYTES_PER_PIXEL	2

/**
 * struct video_frame - one captured image
 * @seq:    counts captures, not deliveries.  A gap means the source dropped a
 *          frame, which a consumer may want to know and cannot otherwise tell.
 * @width:  in pixels
 * @height: in pixels
 * @pixels: @width * @height * VIDEO_BYTES_PER_PIXEL bytes, row major, no
 *          padding between rows
 */
struct video_frame {
	u32		seq;
	u16		width;
	u16		height;
	u8		*pixels;
};

struct video_source {
	const char	*name;
	u16		width;
	u16		height;

	/*
	 * The oldest frame not yet taken, sleeping until one exists.  Must not
	 * be called from interrupt context, and must not be called again
	 * before @release.
	 */
	struct video_frame *(*capture)(struct video_source *src);

	/* Hand it back.  Nothing may read through the pointer afterwards. */
	void		(*release)(struct video_source *src);
};

int video_source_register(struct video_source *src);

/* The registered source, or NULL if no camera exists on this build. */
struct video_source *video_source_get(void);

static inline unsigned int video_frame_bytes(const struct video_source *src)
{
	return (unsigned int)src->width * src->height * VIDEO_BYTES_PER_PIXEL;
}

#endif /* _NOTHAN_VIDEO_H */
