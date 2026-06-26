/*
 * drivers/video/lcdc.c - AM335x LCD Controller driver
 *
 * Drives the LCDC in raster TFT mode for 800x480@60Hz HDMI output via TDA19988.
 * Single framebuffer: the GUI renders straight into the one buffer the DMA
 * scans; FB_FLIP cleans the D-cache and paces to the frame-done IRQ to limit
 * tearing (swapping the DMA base per update raced the raster scan).
 *
 * The panel is run at native 800×480 landscape but mounted physically
 * rotated 90°, so the user sees a 480×800 portrait screen. Rotation is done in
 * userspace (lv_draw_sw_rotate); lcdc_flush() just blits the already-landscape
 * region 1:1 into the framebuffer.
 *
 * Framebuffer layout:
 *   [0..31]       palette (entry 0 = 0x4000 → 16bpp indicator)
 *   [32..768031]  800×480 RGB565 pixel data
 *
 * Pixel clock: DPLL_DISP M=113 N=7 → 339 MHz, M2=5 → 67.8 MHz,
 *              LCDC CLKDIV=2 → 33.9 MHz (panel EDID mode, htotal 1056×535).
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
/*
 * Frame-pacing IRQ. Per the AM335x TRM (Ch.13, IRQSTATUS_RAW 0x58) bit 1 is
 * "recurrent_raster_done" — fires when a frame has been scanned OUT to the
 * panel pins, which is exactly the boundary we want to pace flips to (EOF0/bit8
 * only signals the DMA finished filling the FIFO, before scanout). Despite the
 * name this is NOT EOF0.
 */
#define IRQBIT_FRAME_DONE   (1 << 1)   /* recurrent_raster_done */
#define IRQBIT_FUF          (1 << 5)   /* FIFO underflow (DMA starved) */
#define IRQBIT_SYNC_LOST    (1 << 2)   /* frame sync lost */

/* CM_PER / DPLL_DISP clock registers */
#define CM_PER_LCDC_CLKCTRL     0xF0E00018
#define CM_CLKMODE_DPLL_DISP    0xF0E00498
#define CM_CLKSEL_DPLL_DISP     0xF0E00454
#define CM_DIV_M2_DPLL_DISP     0xF0E004A4
#define CM_IDLEST_DPLL_DISP     0xF0E00448
#define CLKSEL_LCDC_PIXEL_CLK   0xF0E00534

/* DPLL_DISP for 33.9 MHz pixel clock — the panel's EDID preferred DTD
 * (800×480, htotal 1056, vtotal 535, +hsync +vsync). This frames cleanest
 * through our LCDC→TDA19988 chain (kernel-direct test = clean bar a ~46px
 * stripe). NOTE: a Linux PC drives this panel at CVT 29.5MHz/-hsync, but the PC
 * goes GPU→HDMI directly; through OUR TDA framer, CVT/-hsync wraps the image, so
 * we use the EDID DTD here.
 *   DCO = 24 × 113 / 8 = 339 MHz, M2 = 5 → 67.8 MHz, CLKDIV = 2 → 33.9 MHz. */
#define DPLL_DISP_M     113U
#define DPLL_DISP_N     7U
#define DPLL_DISP_M2    5U
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
 * RASTER_TIMING_0 for panel EDID mode (800 active, htotal 1056):
 *   PPL=800: (800/16-1)=49=0x31 → bits[9:4]
 *   HSW=88:  (HSW-1=87)&0x3F=23  → bits[15:10]; high bit (0x40) → TIMING2 HSWMSB
 *   HFP=44:  (HFP-1=43)=0x2B     → bits[23:16]
 *   HBP=124: (HBP-1=123)=0x7B    → bits[31:24]
 */
#define TIMING0_VAL     0x7B2B5F10U

/*
 * RASTER_TIMING_1 for 480 lines EDID, VFP=3, VSW=6, VBP=46 (vtotal 535).
 * Confirmed by dumping the live tilcdc regs on a stock BBB Debian driving this
 * exact panel cleanly: RASTER_TIMING_1 = 0x2e0315df.
 *   LPP=(480-1)=0x1DF, VSW=(6-1)<<10=0x1400,
 *   VFP(raw 3)=0x00030000, VBP(raw 46)=0x2E000000
 */
#define TIMING1_VAL     0x2E0315DFU

/* RASTER_TIMING_2: data driven on falling PCLK edge, TDA19988 samples on rising.
 * INVERT_HSYNC is SET — confirmed from a stock BBB Debian driving this panel
 * cleanly: RASTER_TIMING_2 = 0x0b60ff00. The LCDC drives HSYNC active-low and
 * the TDA takes it as-is (VIP_CNTRL_3 = SYNC_HS, no H_TGL). VSYNC stays active
 * high. AC bias 0xFF, PHSVS on falling edge, HSWMSB carries HSW=88's high bit.
 * (= 0x0b60ff00) */
#define TIMING2_VAL     (TIMING2_PHSVS_ON | TIMING2_PHSVS_FALL | \
			 TIMING2_PCLK_INV | TIMING2_HSYNC_INV | \
			 TIMING2_HSWMSB | TIMING2_ACB_DEFAULT)

/* Single framebuffer the LCDC DMA scans; the GUI renders straight into it. */
static u32   fb_pa;
static void *fb_va;

/* Set by the frame-done IRQ handler */
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

/* Frame-done IRQ handler — fires once per frame scanned out to the panel
 * (recurrent_raster_done). Used by wait_eof() to pace flips to the frame
 * boundary. Clears the underflow/sync-lost error latches too so they don't
 * stay asserted. */
static void lcdc_eof_handler(unsigned int irq)
{
	(void)irq;
	mmio_write32(LCDC_BASE + LCDC_IRQSTATUS,
		     IRQBIT_FRAME_DONE | IRQBIT_FUF | IRQBIT_SYNC_LOST);
	eof_flag = 1;
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

	u16       *pixels = (u16 *)((u8 *)fb_va + PALETTE_SZ);
	const u16 *src    = (const u16 *)data;

	for (int row = 0; row < rows; row++) {
		u16 *dst = pixels + (unsigned int)(y1 + row) * FB_W + (unsigned int)x1;
		for (int col = 0; col < width; col++)
			*dst++ = *src++;
	}
}

static void lcdc_flip(void)
{
	/*
	 * Single buffer: the GUI renders straight into the one displayed buffer.
	 * We do NOT swap the DMA base — swapping it per flush while content updates
	 * many times/sec raced the raster scan and rolled the whole image. Here we
	 * just push the CPU's writes to DRAM and pace to the frame boundary to
	 * limit tearing.
	 */
	clean_dcache_range((unsigned long)fb_va,
			   (unsigned long)fb_va + FB_SZ);
	wait_eof();
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
	struct page *pg0;
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

	/* Allocate one 2 MB framebuffer (order 9 = 512 pages) */
	zone = get_zone();
	pg0  = alloc_pages(GFP_KERNEL, 9);
	if (!pg0) {
		printk("[LCDC] framebuffer alloc failed\n");
		return -1;
	}

	fb_pa = (u32)page_to_phys(zone, pg0);
	fb_va = phys_to_kva(fb_pa);

	/* Zero the buffer; palette entry 0 = 0x4000 (16bpp indicator).
	 * Clean D-cache so LCDC DMA sees zeroed DRAM from the first frame. */
	p = (u8 *)fb_va;
	for (unsigned int i = 0; i < FB_SZ; i++)
		p[i] = 0;
	*(u16 *)fb_va = 0x4000;
	clean_dcache_range((unsigned long)fb_va, (unsigned long)fb_va + FB_SZ);

	mmio_write32(LCDC_BASE + LCDC_DMA_CTRL,    DMA_BURST_16 | DMA_TH_FIFO_512);
	/* BASE points at first pixel (skip 32-byte palette); CEILING = last byte of pixel data.
	 * DMA reads exactly 800×480×2 = 768000 bytes — no stray palette pixels. */
	mmio_write32(LCDC_BASE + LCDC_FB0_BASE,    fb_pa + PALETTE_SZ);
	mmio_write32(LCDC_BASE + LCDC_FB0_CEILING, fb_pa + FB_SZ - 4);
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

	/* Enable the frame-done pacing IRQ + the underflow/sync-lost error
	 * interrupts so the handler can sample and report them. */
	mmio_write32(LCDC_BASE + LCDC_IRQENABLE_SET,
		     IRQBIT_FRAME_DONE | IRQBIT_FUF | IRQBIT_SYNC_LOST);
	request_irq(LCDC_IRQ, lcdc_eof_handler);
	intc_enable_irq(LCDC_IRQ);

	fb_register_ops(&lcdc_fb_ops);

	printk("[LCDC] 800x480 RGB565 single-buffer, fb=PA 0x%08lx\n",
	       (unsigned long)fb_pa);

	return 0;
}
device_initcall(lcdc_init);
