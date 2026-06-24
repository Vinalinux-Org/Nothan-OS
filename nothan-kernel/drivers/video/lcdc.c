/*
 * drivers/video/lcdc.c - AM335x LCD Controller driver
 *
 * Drives the LCDC in raster TFT mode for 800x480@60Hz HDMI output
 * via TDA19988. Implements double buffering with IRQ-driven VSYNC:
 *   - front buffer: LCDC DMA reads from here
 *   - back buffer:  CPU writes here (never touches front during scan)
 *   - FB_FLIP ioctl: waits for EOF0 IRQ, swaps buffers, copies front→back
 *
 * The panel is run at native 800×480 landscape but mounted physically
 * rotated 90°, so the user sees a 480×800 portrait screen. The GUI renders
 * portrait (480×800); lcdc_flush() transposes each flushed region into the
 * 800×480 landscape framebuffer (see the rotation note there).
 *
 * Framebuffer layout (each buffer):
 *   [0..31]       palette (entry 0 = 0x4000 → 16bpp indicator)
 *   [32..768031]  800×480 RGB565 pixel data
 *
 * Pixel clock: DPLL_DISP M=118 N=7 → 354 MHz, M2=6 → 59 MHz,
 *              LCDC CLKDIV=2 → 29.5 MHz (CVT 800×480@60).
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/mmio.h>
#include <nothan/mm.h>
#include <nothan/fb.h>
#include <nothan/irq.h>
#include <nothan/time.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/pinctrl.h>
#include <asm/barrier.h>

/* VA mapping: L4_PER VA 0xF0000000 → PA 0x48000000 */
#define LCDC_BASE       0xF030E000      /* PA 0x4830E000 */
#define LCDC_IRQ        36

/* LCDC register offsets */
#define LCDC_CTRL           0x04
#define LCDC_RASTER_CTRL    0x28
#define LCDC_TIMING0        0x2C
#define LCDC_TIMING1        0x30
#define LCDC_TIMING2        0x34
#define LCDC_DMA_CTRL       0x40
#define LCDC_FB0_BASE       0x44
#define LCDC_FB0_CEILING    0x48
#define LCDC_CLKC_ENABLE    0x6C
#define LCDC_IRQSTATUS_RAW  0x58
#define LCDC_IRQSTATUS      0x5C
#define LCDC_IRQENABLE_SET  0x60

/* LCDC_CTRL bits */
#define CTRL_RASTER_MODE    (1 << 0)
#define CTRL_AUTO_UFLOW     (1 << 1)   /* auto-restart after FIFO underflow */
#define CTRL_CLKDIV(x)      ((x) << 8)

/* LCDC_CLKC_ENABLE bits */
#define CLKC_CORE           (1 << 0)
#define CLKC_LIDD           (1 << 1)
#define CLKC_DMA            (1 << 2)

/* LCDC_RASTER_CTRL bits */
#define RASTER_ENABLE       (1 << 0)
#define RASTER_TFT_MODE     (1 << 7)
#define RASTER_DATA_ONLY    (0x02 << 20)  /* bits 21:20 = 10: raw pixel data, no palette DMA */

/* LCDC_RASTER_TIMING_2 bits (TRM 13.5.1.13) */
#define TIMING2_HSYNC_INV   (1 << 21)   /* ihs: 1=active low */
#define TIMING2_PCLK_INV    (1 << 22)   /* ipc: drive data on falling PCLK */
#define TIMING2_PHSVS_FALL  (1 << 24)   /* 0=rising, 1=falling edge */
#define TIMING2_PHSVS_ON    (1 << 25)   /* drive HS/VS per bit 24 */
#define TIMING2_HSWMSB      (1 << 27)   /* HSW high bits when (HSW-1) > 6 bits */
#define TIMING2_ACB_DEFAULT 0x0000FF00U /* AC bias frequency */

/* DMA + interrupts */
#define DMA_BURST_16        (4 << 4)
#define DMA_TH_FIFO_512     (6 << 8)   /* 512-word refill threshold */
#define IRQBIT_EOF0         (1 << 1)

/* CM_PER / DPLL_DISP clock registers */
#define CM_PER_LCDC_CLKCTRL     0xF0E00018
#define CM_CLKMODE_DPLL_DISP    0xF0E00498
#define CM_CLKSEL_DPLL_DISP     0xF0E00454
#define CM_DIV_M2_DPLL_DISP     0xF0E004A4
#define CM_IDLEST_DPLL_DISP     0xF0E00448
#define CLKSEL_LCDC_PIXEL_CLK   0xF0E00534

/* DPLL_DISP for 29.5 MHz pixel clock (CVT 800×480@60):
 *   DCO = SYS_CLKIN(24) × M / (N+1) = 24 × 118 / 8 = 354 MHz
 *   M2 = 6 → 59 MHz, LCDC CLKDIV = 2 → 29.5 MHz.
 * DCO 354 MHz sits close to the known-good 720p config (297 MHz). */
#define DPLL_DISP_M     118U
#define DPLL_DISP_N     7U
#define DPLL_DISP_M2    6U
#define LCDC_CLKDIV     2U

/* Framebuffer geometry — native panel orientation (landscape).
 * The GUI renders portrait 480×800; lcdc_flush() transposes into this. */
#define PALETTE_SZ  32U
#define FB_W        800U
#define FB_H        480U
#define FB_BPP      2U
#define FB_SZ       (PALETTE_SZ + FB_W * FB_H * FB_BPP)

/* Portrait logical geometry the GUI flushes in (rotated 90° into FB) */
#define PORT_W      480U    /* = FB_H */
#define PORT_H      800U    /* = FB_W */

/*
 * RASTER_TIMING_0 for 800×480 CVT (htotal 992):
 *   PPL=800: HORLSB(800)=0x310, HORMSB=0 (bit10 of 799 clear)
 *   HSW=72:  (HSW-1=71)&0x3F=7 → 0x1C00; high bit → TIMING2 HSWMSB
 *   HFP=24:  (HFP-1=23)=0x17 → 0x00170000
 *   HBP=96:  (HBP-1=95)=0x5F → 0x5F000000
 */
#define TIMING0_VAL     0x5F171F10U

/*
 * RASTER_TIMING_1 for 480 lines, VFP=3, VSW=10, VBP=3:
 *   VERLSB(480)=0x1DF, VSW(10)=(10-1)<<10=0x2400,
 *   VFP(3)=0x00030000, VBP(3)=0x03000000
 */
#define TIMING1_VAL     0x030325DFU

/* RASTER_TIMING_2: data driven on falling PCLK edge, TDA19988 samples on rising.
 * IHS active low (CVT -hsync), VSYNC active high (no invert), AC bias 0xFF,
 * PHSVS controlled by bit 24. HSWMSB carries HSW=72's high bit. */
#define TIMING2_VAL     (TIMING2_PHSVS_ON | TIMING2_PHSVS_FALL | \
			 TIMING2_PCLK_INV | TIMING2_HSYNC_INV | \
			 TIMING2_HSWMSB | TIMING2_ACB_DEFAULT)

/* Double buffer state */
static u32  fb_pa[2];
static void *fb_va[2];
static int   front_idx;     /* buffer LCDC is reading */
static int   back_idx;      /* buffer CPU writes to */

/* Set by EOF0 IRQ handler */
static volatile int eof_flag;

static void dpll_disp_configure(void)
{
	unsigned long end;

	mmio_write32(CM_CLKMODE_DPLL_DISP, 0x04);
	mmio_write32(CM_CLKSEL_DPLL_DISP, (DPLL_DISP_M << 8) | DPLL_DISP_N);
	mmio_write32(CM_DIV_M2_DPLL_DISP, DPLL_DISP_M2);
	mmio_write32(CLKSEL_LCDC_PIXEL_CLK, 0x00);
	mmio_write32(CM_CLKMODE_DPLL_DISP, 0x07);

	end = 0xFFFFFF;
	while (!(mmio_read32(CM_IDLEST_DPLL_DISP) & 1) && end--)
		;
	if (!end)
		printk("[LCDC] DPLL_DISP lock timeout\n");
}

/* EOF0 IRQ handler — fires when LCDC finishes scanning a frame */
static void lcdc_eof_handler(unsigned int irq)
{
	static int eof_count;
	(void)irq;
	mmio_write32(LCDC_BASE + LCDC_IRQSTATUS, IRQBIT_EOF0);
	eof_flag = 1;
	if (++eof_count <= 3)
		printk("[LCDC] EOF0 IRQ fired (#%d)\n", eof_count);
}

/* Block until EOF0 fires (max 30 ms = 2 frames at 60 Hz) */
static void wait_eof(void)
{
	unsigned long end = get_jiffies() + 3;

	eof_flag = 0;
	while (!eof_flag) {
		if (get_jiffies() >= end) {
			/* EOF0 IRQ did not fire within 30 ms — tearing will occur.
			 * Root cause: IRQ 36 not reaching the handler. */
			printk("[LCDC] WARN wait_eof: EOF0 IRQ timeout (IRQ 36 not firing)\n");
			return;
		}
	}
}

/* Word-at-a-time copy — no memcpy in kernel */
static void copy_buf(u8 *dst, const u8 *src, unsigned int bytes)
{
	while (bytes >= 4) {
		*(u32 *)dst = *(const u32 *)src;
		dst += 4; src += 4; bytes -= 4;
	}
	while (bytes--)
		*dst++ = *src++;
}

/*
 * lcdc_flush — copy a GUI-rendered region straight into the framebuffer.
 *
 * Rotation is now done in userspace (lv_draw_sw_rotate in lv_port_disp.c),
 * so the region handed to us is already in the panel's native landscape
 * orientation. We just blit it 1:1 at (x1,y1)..(x2,y2). No transpose.
 */
static void lcdc_flush(int x1, int y1, int x2, int y2,
		       const void *data, unsigned int len)
{
	/*
	 * Reject any region outside the landscape framebuffer, inverted, or
	 * whose length doesn't match its area. The blit below turns each source
	 * pixel into an absolute framebuffer index; an out-of-range coord would
	 * write past the framebuffer into adjacent kernel memory, and a short
	 * buffer would read past the user's source. The ioctl seam is the only
	 * way userspace reaches this, so the args are not trusted here.
	 */
	if (x1 < 0 || y1 < 0 || x2 >= (int)FB_W || y2 >= (int)FB_H ||
	    x1 > x2 || y1 > y2)
		return;

	int width = x2 - x1 + 1;
	int rows  = y2 - y1 + 1;

	if (len != (unsigned int)width * (unsigned int)rows * FB_BPP)
		return;

	u16       *pixels = (u16 *)((u8 *)fb_va[back_idx] + PALETTE_SZ);
	const u16 *src    = (const u16 *)data;

	for (int row = 0; row < rows; row++) {
		u16 *dst = pixels + (unsigned int)(y1 + row) * FB_W + (unsigned int)x1;
		for (int col = 0; col < width; col++)
			*dst++ = *src++;
	}
}

static void lcdc_flip(void)
{
	int new_front = back_idx;
	int new_back  = front_idx;

	/* Clean D-cache so LCDC DMA reads the CPU's writes from DRAM */
	clean_dcache_range((unsigned long)fb_va[new_front],
			   (unsigned long)fb_va[new_front] + FB_SZ);

	/* Wait for current frame scan to finish */
	wait_eof();

	/* Point LCDC DMA at the new front buffer — skip palette, pixel-only range */
	mmio_write32(LCDC_BASE + LCDC_FB0_BASE,    fb_pa[new_front] + PALETTE_SZ);
	mmio_write32(LCDC_BASE + LCDC_FB0_CEILING, fb_pa[new_front] + FB_SZ - 4);

	front_idx = new_front;
	back_idx  = new_back;

	/* Sync new back buffer with displayed content so partial updates work */
	copy_buf((u8 *)fb_va[back_idx], (u8 *)fb_va[front_idx], FB_SZ);
}

/*
 * lcdc_shutdown() - Stop the raster DMA. Called from sys_reboot before
 * triggering PRM warm reset so the LCDC isn't mid-burst on DDR when the
 * bootloader starts its memory test on the next boot.
 */
void lcdc_shutdown(void)
{
	/* Stop the raster engine and its DMA */
	mmio_write32(LCDC_BASE + LCDC_RASTER_CTRL, 0);
	mmio_write32(LCDC_BASE + LCDC_IRQENABLE_SET, 0);
	mmio_write32(LCDC_BASE + LCDC_DMA_CTRL, 0);

	/* Disable the LCDC clocks so no pending DMA can still hit DDR */
	mmio_write32(LCDC_BASE + LCDC_CLKC_ENABLE, 0);

	/* Gate the LCDC module clock at the PRCM level */
	mmio_write32(CM_PER_LCDC_CLKCTRL, 0);

	/* Let any in-flight bus transaction drain */
	dsb();
	for (volatile int i = 0; i < 10000; i++)
		;
}

static struct fb_ops lcdc_fb_ops = {
	.flush = lcdc_flush,
	.flip  = lcdc_flip,
};

static int __init lcdc_init(void)
{
	struct zone *zone;
	struct page *pg0, *pg1;
	u8          *p;
	u32          rctrl;

	pinctrl_select("lcdc");

	mmio_write32(CM_PER_LCDC_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_LCDC_CLKCTRL) & 0x30000) != 0)
		;

	dpll_disp_configure();

	mmio_write32(LCDC_BASE + LCDC_CLKC_ENABLE, CLKC_CORE | CLKC_LIDD | CLKC_DMA);
	mmio_write32(LCDC_BASE + LCDC_CTRL,
		     CTRL_CLKDIV(LCDC_CLKDIV) | CTRL_AUTO_UFLOW | CTRL_RASTER_MODE);

	/* Allocate two 2 MB framebuffers (order 9 = 512 pages each) */
	zone = get_zone();
	pg0  = alloc_pages(GFP_KERNEL, 9);
	pg1  = alloc_pages(GFP_KERNEL, 9);
	if (!pg0 || !pg1) {
		printk("[LCDC] framebuffer alloc failed\n");
		return -1;
	}

	fb_pa[0] = (u32)page_to_phys(zone, pg0);
	fb_pa[1] = (u32)page_to_phys(zone, pg1);
	fb_va[0] = phys_to_kva(fb_pa[0]);
	fb_va[1] = phys_to_kva(fb_pa[1]);

	/* Zero both buffers; palette entry 0 = 0x4000 (16bpp indicator).
	 * Clean D-cache so LCDC DMA sees zeroed DRAM from the first frame. */
	for (int b = 0; b < 2; b++) {
		p = (u8 *)fb_va[b];
		for (unsigned int i = 0; i < FB_SZ; i++)
			p[i] = 0;
		*(u16 *)fb_va[b] = 0x4000;
		clean_dcache_range((unsigned long)fb_va[b],
				   (unsigned long)fb_va[b] + FB_SZ);
	}

	front_idx = 0;
	back_idx  = 1;

	mmio_write32(LCDC_BASE + LCDC_DMA_CTRL,    DMA_BURST_16 | DMA_TH_FIFO_512);
	/* BASE points at first pixel (skip 32-byte palette); CEILING = last byte of pixel data.
	 * DMA reads exactly 800×480×2 = 768000 bytes — no stray palette pixels. */
	mmio_write32(LCDC_BASE + LCDC_FB0_BASE,    fb_pa[front_idx] + PALETTE_SZ);
	mmio_write32(LCDC_BASE + LCDC_FB0_CEILING, fb_pa[front_idx] + FB_SZ - 4);
	mmio_write32(LCDC_BASE + LCDC_TIMING0,     TIMING0_VAL);
	mmio_write32(LCDC_BASE + LCDC_TIMING1,     TIMING1_VAL);
	mmio_write32(LCDC_BASE + LCDC_TIMING2,     TIMING2_VAL);

	/*
	 * DATA_ONLY mode (bits 21:20 = 10): LCDC DMA reads raw pixel data with
	 * no palette step. We set BASE = fb_pa + PALETTE_SZ so the DMA starts
	 * at the first pixel, not the palette header. CEILING covers exactly
	 * 800×480×2 = 768000 bytes. With BASE = fb_pa the palette bytes (32 B)
	 * were treated as 16 extra pixels per frame, filling the FIFO faster than
	 * the display drained it; AUTO_UFLOW restarted DMA mid-frame from whatever
	 * BASE was, producing the seam at a different scan line each boot.
	 */
	mmio_write32(LCDC_BASE + LCDC_RASTER_CTRL, RASTER_TFT_MODE | RASTER_DATA_ONLY);
	rctrl = mmio_read32(LCDC_BASE + LCDC_RASTER_CTRL);
	mmio_write32(LCDC_BASE + LCDC_RASTER_CTRL, rctrl | RASTER_ENABLE);

	/* Enable EOF0 interrupt and register handler */
	mmio_write32(LCDC_BASE + LCDC_IRQENABLE_SET, IRQBIT_EOF0);
	request_irq(LCDC_IRQ, lcdc_eof_handler);
	intc_enable_irq(LCDC_IRQ);

	fb_register_ops(&lcdc_fb_ops);

	printk("[LCDC] 800x480 RGB565 double-buffer, front=PA 0x%08lx back=PA 0x%08lx\n",
	       (unsigned long)fb_pa[front_idx], (unsigned long)fb_pa[back_idx]);

#define LCDC_TEST_PATTERN 1
#if LCDC_TEST_PATTERN
	/*
	 * DEBUG: fill the whole framebuffer solid red straight from the kernel
	 * (no GUI). Confirms the LCDC + TDA19988 + 800x480 path works on its own.
	 * The panel is mounted rotated 90°, so a full red fill covers the entire
	 * physical screen regardless of orientation. Spawn of the GUI is gated off
	 * in init/main.c (BOOT_GUI) so nothing overwrites this.
	 */
	{
		u16 *tp = (u16 *)((u8 *)fb_va[front_idx] + PALETTE_SZ);
		for (unsigned int i = 0; i < FB_W * FB_H; i++)
			tp[i] = 0xF800;		/* RGB565 red */
		clean_dcache_range((unsigned long)fb_va[front_idx],
				   (unsigned long)fb_va[front_idx] + FB_SZ);
		printk("[LCDC] TEST: full red screen (800x480, panel rotated 90)\n");
	}
#endif
	return 0;
}
device_initcall(lcdc_init);
