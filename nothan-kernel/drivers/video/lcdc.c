/*
 * drivers/video/lcdc.c - AM335x LCD Controller driver
 *
 * Drives the LCDC in raster TFT mode for 1280x720@60Hz HDMI output
 * via SiI9022A. Implements double buffering with IRQ-driven VSYNC:
 *   - front buffer: LCDC DMA reads from here
 *   - back buffer:  CPU writes here (never touches front during scan)
 *   - FB_FLIP ioctl: waits for EOF0 IRQ, swaps buffers, copies front→back
 *
 * Framebuffer layout (each buffer):
 *   [0..31]        palette (entry 0 = 0x4000 → 16bpp indicator)
 *   [32..1843231]  1280×720 RGB565 pixel data
 *
 * Pixel clock: DPLL_DISP M=99 N=7 → 297 MHz, M2=2 → 148.5 MHz,
 *              LCDC CLKDIV=2 → 74.25 MHz.
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
#define RASTER_DATA_ONLY    (0x02 << 20)

/* LCDC_RASTER_TIMING_2 bits (TRM 13.5.1.13) */
#define TIMING2_HSYNC_INV   (1 << 21)   /* ihs: 1=active low */
#define TIMING2_PCLK_INV    (1 << 22)   /* ipc: drive data on falling PCLK */
#define TIMING2_PHSVS_FALL  (1 << 24)   /* 0=rising, 1=falling edge */
#define TIMING2_PHSVS_ON    (1 << 25)   /* drive HS/VS per bit 24 */
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

#define DPLL_DISP_M     99U
#define DPLL_DISP_N     7U
#define DPLL_DISP_M2    2U
#define LCDC_CLKDIV     2U

/* Framebuffer geometry */
#define PALETTE_SZ  32U
#define FB_W        1280U
#define FB_H        720U
#define FB_BPP      2U
#define FB_SZ       (PALETTE_SZ + FB_W * FB_H * FB_BPP)

/*
 * RASTER_TIMING_0 for 1280×720@60Hz:
 *   PPL=1280: HORLSB=0x2F0, HORMSB=0x008
 *   HSW=40:   HSWLSB=0x9C00
 *   HFP=110:  HFPLSB=0x006D0000
 *   HBP=220:  HBPLSB=0xDB000000
 */
#define TIMING0_VAL     0xDB6D9CF8U

/*
 * RASTER_TIMING_1 for 720 lines, VFP=5, VSW=5, VBP=20:
 *   VERLSB(720)=0x2CF, VSW(5)=0x1000,
 *   VFP(5)=0x00050000, VBP(20)=0x14000000
 */
#define TIMING1_VAL     0x140512CFU

/* RASTER_TIMING_2: data driven on falling PCLK edge, TDA19988 samples on rising.
 * IHS active low, AC bias 0xFF, PHSVS controlled by bit 24. */
#define TIMING2_VAL     (TIMING2_PHSVS_ON | TIMING2_PHSVS_FALL | \
			 TIMING2_PCLK_INV | TIMING2_HSYNC_INV | \
			 TIMING2_ACB_DEFAULT)

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
	(void)irq;
	mmio_write32(LCDC_BASE + LCDC_IRQSTATUS, IRQBIT_EOF0);
	eof_flag = 1;
}

/* Block until EOF0 fires (max 30 ms = 2 frames at 60 Hz) */
static void wait_eof(void)
{
	unsigned long end = get_jiffies() + 3;

	eof_flag = 0;
	while (!eof_flag) {
		if (get_jiffies() >= end)
			return;
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

static void lcdc_flush(int x1, int y1, int x2, int y2,
		       const void *data, unsigned int len)
{
	int          width     = x2 - x1 + 1;
	int          rows      = y2 - y1 + 1;
	unsigned int row_bytes = (unsigned int)width * FB_BPP;
	u8          *pixels    = (u8 *)fb_va[back_idx] + PALETTE_SZ;
	const u8    *src       = (const u8 *)data;

	for (int row = 0; row < rows; row++) {
		u8 *dst = pixels + ((unsigned int)(y1 + row) * FB_W +
				    (unsigned int)x1) * FB_BPP;
		copy_buf(dst, src, row_bytes);
		src += row_bytes;
	}

	(void)len;
}

static void lcdc_flip(void)
{
	int new_front = back_idx;
	int new_back  = front_idx;

	/* Ensure all CPU writes to back_buf are visible to DMA */
	dsb();

	/* Wait for current frame scan to finish */
	wait_eof();

	/* Point LCDC DMA at the new front buffer */
	mmio_write32(LCDC_BASE + LCDC_FB0_BASE,    fb_pa[new_front]);
	mmio_write32(LCDC_BASE + LCDC_FB0_CEILING, fb_pa[new_front] + FB_SZ - 4);

	front_idx = new_front;
	back_idx  = new_back;

	/* Sync new back buffer with displayed content so partial updates work */
	copy_buf((u8 *)fb_va[back_idx], (u8 *)fb_va[front_idx], FB_SZ);
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

	/* Zero both buffers; palette entry 0 = 0x4000 (16bpp indicator) */
	for (int b = 0; b < 2; b++) {
		p = (u8 *)fb_va[b];
		for (unsigned int i = 0; i < FB_SZ; i++)
			p[i] = 0;
		*(u16 *)fb_va[b] = 0x4000;
	}

	front_idx = 0;
	back_idx  = 1;

	mmio_write32(LCDC_BASE + LCDC_DMA_CTRL,    DMA_BURST_16 | DMA_TH_FIFO_512);
	mmio_write32(LCDC_BASE + LCDC_FB0_BASE,    fb_pa[front_idx]);
	mmio_write32(LCDC_BASE + LCDC_FB0_CEILING, fb_pa[front_idx] + FB_SZ - 4);
	mmio_write32(LCDC_BASE + LCDC_TIMING0,     TIMING0_VAL);
	mmio_write32(LCDC_BASE + LCDC_TIMING1,     TIMING1_VAL);
	mmio_write32(LCDC_BASE + LCDC_TIMING2,     TIMING2_VAL);

	mmio_write32(LCDC_BASE + LCDC_RASTER_CTRL, RASTER_TFT_MODE | RASTER_DATA_ONLY);
	rctrl = mmio_read32(LCDC_BASE + LCDC_RASTER_CTRL);
	mmio_write32(LCDC_BASE + LCDC_RASTER_CTRL, rctrl | RASTER_ENABLE);

	/* Enable EOF0 interrupt and register handler */
	mmio_write32(LCDC_BASE + LCDC_IRQENABLE_SET, IRQBIT_EOF0);
	request_irq(LCDC_IRQ, lcdc_eof_handler);
	intc_enable_irq(LCDC_IRQ);

	fb_register_ops(&lcdc_fb_ops);

	printk("[LCDC] 1280x720 RGB565 double-buffer, front=PA 0x%08lx back=PA 0x%08lx\n",
	       (unsigned long)fb_pa[front_idx], (unsigned long)fb_pa[back_idx]);
	return 0;
}
device_initcall(lcdc_init);
