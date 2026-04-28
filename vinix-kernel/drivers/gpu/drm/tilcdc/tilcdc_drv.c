/*
 * AM335x LCDC raster driver — 800x600@60Hz, RGB565.
 *
 * Pattern reference: U-Boot am335x-fb.c — re-implemented.
 * AM335x TRM Ch.13.
 */

#include "types.h"
#include "lcdc.h"
#include "mmio.h"
#include "uart.h"
#include "mach/prcm.h"
#include "mach/control.h"

/*
 * BBB connects lcd_data[15:0] + sync signals to TDA19988. These pins
 * default to GPIO after reset — must be muxed to LCD function (mode 0)
 * before LCDC can drive them.
 */
#define CONF_LCD_DATA0          (CTRL_MODULE_BASE + 0x8A0)
#define CONF_LCD_VSYNC          (CTRL_MODULE_BASE + 0x8E0)
#define CONF_LCD_HSYNC          (CTRL_MODULE_BASE + 0x8E4)
#define CONF_LCD_PCLK           (CTRL_MODULE_BASE + 0x8E8)
#define CONF_LCD_AC_BIAS_EN     (CTRL_MODULE_BASE + 0x8EC)  /* DE signal */

/* GPMC_AD pins for lcd_data[16:23] — 24-bit mode (unused on BBB PCB). */
#define CONF_GPMC_AD8           (CTRL_MODULE_BASE + 0x820)

/* Pad config matching TI StarterWare: RXACTIVE for LCDC feedback */
#define LCD_PAD_MODE0           0x28    /* mode 0, pull disabled, RX active */
#define LCD_PAD_MODE1           0x21    /* mode 1, pull enabled, RX active */

static void lcd_pinmux_setup(void)
{
    /* lcd_data[0:15]: dedicated LCD pins, mode 0 */
    for (int i = 0; i < 16; i++) {
        mmio_write32(CONF_LCD_DATA0 + (i * 4), LCD_PAD_MODE0);
    }

    /* Note: GPMC_AD[8:15] NOT routed to TDA19988 on BBB PCB.
     * They go to P8 expansion header only. 16-bit is the max. */

    /* Sync and clock signals */
    mmio_write32(CONF_LCD_VSYNC, LCD_PAD_MODE0);
    mmio_write32(CONF_LCD_HSYNC, LCD_PAD_MODE0);
    mmio_write32(CONF_LCD_PCLK,  LCD_PAD_MODE0);
    mmio_write32(CONF_LCD_AC_BIAS_EN, LCD_PAD_MODE0);

    pr_info("[LCDC] LCD pinmux configured (20 pins, 16-bit RGB565)\n");
}

/* LCDC pixel clock source select — TRM 8.1.6.21 */
#define CLKSEL_LCDC_PIXEL_CLK   (CM_DPLL_BASE + 0x34)

/* DPLL mode bits */
#define DPLL_EN_LOCK            0x7
#define DPLL_EN_BYPASS          0x4
#define DPLL_EN_MASK            0x7

/* DPLL status bits */
#define ST_DPLL_CLK             (1 << 0)
#define ST_MN_BYPASS            (1 << 8)

/* LCDC register offsets. */

#define LCD_PID             0x00    /* Module ID */
#define LCD_CTRL            0x04    /* Control */
#define LCD_LIDD_CTRL       0x0C    /* LIDD control */
#define LCD_RASTER_CTRL     0x28    /* Raster control */
#define LCD_RASTER_TIMING_0 0x2C    /* Horizontal timing */
#define LCD_RASTER_TIMING_1 0x30    /* Vertical timing */
#define LCD_RASTER_TIMING_2 0x34    /* Sync/polarity/MSBs */
#define LCD_RASTER_SUBPANEL 0x38    /* Subpanel */
#define LCD_RASTER_SUBPANEL2 0x3C   /* Subpanel 2 */
#define LCD_LCDDMA_CTRL     0x40    /* DMA control */
#define LCD_LCDDMA_FB0_BASE 0x44    /* Framebuffer 0 base */
#define LCD_LCDDMA_FB0_CEIL 0x48    /* Framebuffer 0 ceiling */
#define LCD_LCDDMA_FB1_BASE 0x4C    /* Framebuffer 1 base */
#define LCD_LCDDMA_FB1_CEIL 0x50    /* Framebuffer 1 ceiling */
#define LCD_SYSCONFIG       0x54    /* System config */
#define LCD_IRQSTATUS_RAW   0x58    /* IRQ raw status */
#define LCD_IRQSTATUS       0x5C    /* IRQ status */
#define LCD_IRQENABLE_SET   0x60    /* IRQ enable set */
#define LCD_IRQENABLE_CLR   0x64    /* IRQ enable clear */
#define LCD_CLKC_ENABLE     0x6C    /* Clock enable */
#define LCD_CLKC_RESET      0x70    /* Clock reset */

/* LCD_CTRL bits */
#define LCD_CLK_DIVISOR(x)      ((x) << 8)
#define LCD_AUTO_UFLOW_RESTART  (1 << 1)        /* Auto-restart on underflow */
#define LCD_RASTER_MODE         0x01

/* LCD_CLKC_ENABLE bits */
#define LCD_CORECLKEN           (1 << 0)
#define LCD_LIDDCLKEN           (1 << 1)
#define LCD_DMACLKEN            (1 << 2)

/* LCD_LCDDMA_CTRL bits */
#define LCD_DMA_BURST_16        (0x4 << 4)  /* Burst size 16 words, bits [6:4]=100 */
#define LCD_DMA_TH_FIFO(x)     ((x) << 8)  /* FIFO threshold, bits [10:8] */
#define LCD_DMA_TH_FIFO_512    6            /* 110b = 512 words (QNX production value) */

/* LCD_RASTER_CTRL bits */
#define LCD_TFT_24BPP_MODE      (1 << 25)
#define LCD_TFT_24BPP_UNPACK    (1 << 26)  /* 32bpp unpacked (1 pixel = 4 bytes) */
#define LCD_PALMODE_RAWDATA     (0x02 << 20)
#define LCD_FDD_RASTER(x)       ((x) << 12)    /* FIFO DMA delay in RASTER_CTRL */
#define LCD_TFT_MODE            (1 << 7)
#define LCD_RASTER_ENABLE       (1 << 0)

/* Timing register encoding macros
 * PPL encoding: pixels_per_line = 16 × (ppl_field + 1)
 * ppl_field = (width / 16) - 1, split across PPLLSB [9:4] and PPLMSB [bit 3]
 *
 * HARDWARE VERIFIED: bit 2 is reserved (write ignored on real AM335x).
 * TRM 13.5.1.11 confirms PPLMSB at bit 3. U-Boot >>4 targets bit 2 and
 * only works by accident for resolutions ≤ 1024 where MSB is not needed. */
#define LCD_HORLSB(x)   (((((x) >> 4) - 1) & 0x3F) << 4)
#define LCD_HORMSB(x)    (((((x) >> 4) - 1) & 0x40) >> 3)
#define LCD_HFPLSB(x)   ((((x) - 1) & 0xFF) << 16)
#define LCD_HBPLSB(x)   ((((x) - 1) & 0xFF) << 24)
#define LCD_HSWLSB(x)   ((((x) - 1) & 0x3F) << 10)
#define LCD_VERLSB(x)   (((x) - 1) & 0x3FF)
#define LCD_VFP(x)       ((x) << 16)
#define LCD_VBP(x)       ((x) << 24)
#define LCD_VSW(x)       (((x) - 1) << 10)
#define LCD_HSWMSB(x)    ((((x) - 1) & 0x3C0) << 21)
#define LCD_VERMSB(x)    ((((x) - 1) & 0x400) << 16)
#define LCD_HBPMSB(x)    ((((x) - 1) & 0x300) >> 4)
#define LCD_HFPMSB(x)    ((((x) - 1) & 0x300) >> 8)

/* Polarity/sync bits in TIMING_2 */
#define SYNC_CTRL       (1 << 25)   /* Drive HSYNC/VSYNC during blanking */
#define SYNC_EDGE       (1 << 24)   /* 0=rising, 1=falling */
#define PCLK_INVERT     (1 << 22)   /* Drive data on falling edge of PCLK */
#define HSYNC_INVERT    (1 << 21)
#define VSYNC_INVERT    (1 << 20)

/* ============================================================
/*
 * 800x600 @ 60Hz VESA DMT timing.
 *
 * Pixel clock: 40 MHz
 * Sync: PHSYNC, PVSYNC (both positive)
 * Source: QNX drm_800x600 struct (hdmi.c line 157-172)
 */

#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  600
#define DISPLAY_BPP     16      /* RGB565 = 2 bytes per pixel */
#define DISPLAY_HFP     40
#define DISPLAY_HBP     88
#define DISPLAY_HSW     128
#define DISPLAY_VFP     1
#define DISPLAY_VBP     23
#define DISPLAY_VSW     4
#define PIXEL_CLOCK     40000000    /* 40 MHz */

/*
 * Framebuffer layout.
 *
 * LCDC requires a 32-byte palette header at the start of the
 * framebuffer, even in raw data mode. First word must be 0x4000
 * to indicate raw data (bypass palette lookup).
 *
 * Memory layout at FB_PA_BASE:
 *   [0x00 - 0x1F]  Palette header (32 bytes)
 *   [0x20 - ...]   Pixel data (width * height * 2 bytes, RGB565)
 */

#define PALETTE_SIZE    32
#define FB_SIZE         (DISPLAY_WIDTH * DISPLAY_HEIGHT * (DISPLAY_BPP / 8))

/* Framebuffer VA = identity mapped (same as PA for peripheral-like access)
 * Actually mapped via dedicated section in mmu.c */
#define FB_VA_BASE      FB_PA_BASE

static uint16_t *fb_pixels = NULL;  /* pointer to first pixel (after palette), RGB565 */

/*
 * Display PLL configuration.
 *
 * DPLL_DISP generates LCD_CLK.
 * LCD_PCLK = LCD_CLK / CLKDIV (CLKDIV is in LCD_CTRL register).
 *
 * Target: 40 MHz pixel clock (800x600@60Hz)
 *
 * DPLL formula: Fdpll = CLKINP * M / (N + 1) / M2
 * CLKINP = 24 MHz
 *
 * CLKDIV must be >= 2 (AM335x TRM: CLKDIV=0 or 1 is invalid).
 * M=10, N=2, M2=1: Fdpll = 24 * 10 / 3 = 80 MHz
 * CLKDIV=2: LCD_PCLK = 80 / 2 = 40 MHz ✓
 */

#define DPLL_DISP_M     10
#define DPLL_DISP_N     2       /* divider = N + 1 = 3 */
#define DPLL_DISP_M2    1
#define LCDC_CLKDIV     2

static void dpll_disp_setup(void)
{
    uint32_t val;
    uint32_t timeout;

    /* Configure Display PLL */

    /* Put DPLL in MN bypass before reconfiguring */
    val = mmio_read32(CM_CLKMODE_DPLL_DISP);
    val = (val & ~DPLL_EN_MASK) | DPLL_EN_BYPASS;
    mmio_write32(CM_CLKMODE_DPLL_DISP, val);

    /* Wait for bypass status */
    timeout = 100000;
    while (!(mmio_read32(CM_IDLEST_DPLL_DISP) & ST_MN_BYPASS) && timeout--);
    if (!timeout) {
        pr_err("[LCDC] ERROR: DPLL bypass timeout\n");
        return;
    }

    /* Set M and N: CLKSEL = (M << 8) | N */
    mmio_write32(CM_CLKSEL_DPLL_DISP, (DPLL_DISP_M << 8) | DPLL_DISP_N);

    /* Set M2 divider */
    mmio_write32(CM_DIV_M2_DPLL_DISP, DPLL_DISP_M2);

    /* Lock DPLL */
    val = mmio_read32(CM_CLKMODE_DPLL_DISP);
    val = (val & ~DPLL_EN_MASK) | DPLL_EN_LOCK;
    mmio_write32(CM_CLKMODE_DPLL_DISP, val);

    /* Wait for lock */
    timeout = 100000;
    while (!(mmio_read32(CM_IDLEST_DPLL_DISP) & ST_DPLL_CLK) && timeout--);
    if (!timeout) {
        pr_err("[LCDC] ERROR: DPLL lock timeout\n");
        return;
    }

    /* Select DISP DPLL CLKOUT as LCDC pixel clock source (value 0x0) */
    mmio_write32(CLKSEL_LCDC_PIXEL_CLK, 0x0);

    pr_info("[LCDC] Display PLL locked at %d MHz\n",
                (24 * DPLL_DISP_M) / (DPLL_DISP_N + 1));
}

/* ============================================================
 * LCDC Clock Enable
 * ============================================================ */

static void lcdc_clock_enable(void)
{
    uint32_t val;
    uint32_t timeout;

    pr_info("[LCDC] Enabling LCDC clock...\n");

    /* Wake up LCDC clock domain */
    mmio_write32(CM_PER_LCDC_CLKSTCTRL, 0x2);

    /* Enable LCDC module */
    mmio_write32(CM_PER_LCDC_CLKCTRL, MODULEMODE_ENABLE);

    /* Wait for module to become functional */
    timeout = 100000;
    while (timeout--) {
        val = mmio_read32(CM_PER_LCDC_CLKCTRL);
        if ((val & IDLEST_MASK) == IDLEST_FUNCTIONAL) {
            pr_info("[LCDC] Module clock enabled\n");
            return;
        }
    }
    pr_err("[LCDC] ERROR: Clock enable timeout\n");
}

/* ============================================================
 * Framebuffer Setup
 * ============================================================ */

static void framebuffer_setup(void)
{
    volatile uint32_t *fb = (volatile uint32_t *)FB_VA_BASE;

    /* Write palette header — first word = 0x4000 means bypass palette (raw data) */
    fb[0] = 0x4000;
    for (int i = 1; i < (PALETTE_SIZE / 4); i++) {
        fb[i] = 0;
    }

    /* Pixel data starts after palette */
    fb_pixels = (uint16_t *)(FB_VA_BASE + PALETTE_SIZE);

    /* Clear screen to black */
    for (uint32_t i = 0; i < (FB_SIZE / 2); i++) {
        fb_pixels[i] = 0x0000;
    }

    pr_info("[LCDC] Framebuffer at PA 0x%x, %dx%d, RGB565 (16bpp)\n",
                FB_PA_BASE, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

/* ============================================================
 * LCDC Initialization
 * ============================================================ */

void lcdc_init(void)
{
    lcd_pinmux_setup();
    dpll_disp_setup();
    lcdc_clock_enable();
    framebuffer_setup();

    mmio_write32(LCDC_BASE + LCD_CLKC_ENABLE,
                 LCD_CORECLKEN | LCD_LIDDCLKEN | LCD_DMACLKEN);

    /* Reset clock domains — without it, stale ROM/bootloader state
     * triggers SYNC_LOST when raster starts. */
    mmio_write32(LCDC_BASE + LCD_CLKC_RESET, 0x0F);
    for (volatile int i = 0; i < 1000; i++);
    mmio_write32(LCDC_BASE + LCD_CLKC_RESET, 0x00);
    for (volatile int i = 0; i < 1000; i++);
    pr_info("[LCDC] Clock domain reset complete\n");

    /* Raster must be off while reconfiguring. */
    mmio_write32(LCDC_BASE + LCD_RASTER_CTRL, 0);

    /* AUTO_UFLOW_RESTART auto-resumes raster after FIFO underflow
     * instead of halting — survives transient stalls. */
    mmio_write32(LCDC_BASE + LCD_CTRL,
                 LCD_CLK_DIVISOR(LCDC_CLKDIV) |
                 LCD_AUTO_UFLOW_RESTART | LCD_RASTER_MODE);

    /* DMA reads palette header first (0x4000 = raw bypass) then pixels.
     * Priority bits [18:16] default to 000 = highest, leave as-is. */
    mmio_write32(LCDC_BASE + LCD_LCDDMA_FB0_BASE, FB_PA_BASE);
    mmio_write32(LCDC_BASE + LCD_LCDDMA_FB0_CEIL, FB_PA_BASE + PALETTE_SIZE + FB_SIZE);
    mmio_write32(LCDC_BASE + LCD_LCDDMA_FB1_BASE, FB_PA_BASE);
    mmio_write32(LCDC_BASE + LCD_LCDDMA_FB1_CEIL, FB_PA_BASE + PALETTE_SIZE + FB_SIZE);
    mmio_write32(LCDC_BASE + LCD_LCDDMA_CTRL,
                 LCD_DMA_BURST_16 | LCD_DMA_TH_FIFO(LCD_DMA_TH_FIFO_512));

    /* Step 8: Configure raster timing for 640x480 @ 60Hz */
    mmio_write32(LCDC_BASE + LCD_RASTER_TIMING_0,
                 LCD_HORLSB(DISPLAY_WIDTH) |
                 LCD_HORMSB(DISPLAY_WIDTH) |
                 LCD_HFPLSB(DISPLAY_HFP) |
                 LCD_HBPLSB(DISPLAY_HBP) |
                 LCD_HSWLSB(DISPLAY_HSW));

    mmio_write32(LCDC_BASE + LCD_RASTER_TIMING_1,
                 LCD_VBP(DISPLAY_VBP) |
                 LCD_VFP(DISPLAY_VFP) |
                 LCD_VSW(DISPLAY_VSW) |
                 LCD_VERLSB(DISPLAY_HEIGHT));

    /* TIMING_2 polarity — 800x600 has PHSYNC/PVSYNC (positive).
     * QNX: PHSYNC → set IHS, PVSYNC → don't set IVS. */
    mmio_write32(LCDC_BASE + LCD_RASTER_TIMING_2,
                 LCD_HSWMSB(DISPLAY_HSW) |
                 LCD_VERMSB(DISPLAY_HEIGHT) |
                 LCD_HBPMSB(DISPLAY_HBP) |
                 LCD_HFPMSB(DISPLAY_HFP) |
                 SYNC_CTRL |
                 SYNC_EDGE |
                 PCLK_INVERT |
                 HSYNC_INVERT |
                 0x0000FF00);    /* AC bias frequency */

    pr_info("[LCDC] 800x600 @ %dMHz ready\n",
                (24 * DPLL_DISP_M) / (DPLL_DISP_N + 1) / LCDC_CLKDIV);
}

/* ============================================================
 * Start Raster Output
 * ============================================================
 * Must be called AFTER TDA19988 is fully configured.
 * QNX production driver: TDA config → LCDC config → raster enable (last).
 * ============================================================ */

void lcdc_start_raster(void)
{
    mmio_write32(LCDC_BASE + LCD_RASTER_CTRL,
                 LCD_PALMODE_RAWDATA |
                 LCD_TFT_MODE |
                 LCD_RASTER_ENABLE);

    /* Brief wait for DMA to fill FIFO */
    for (volatile uint32_t i = 0; i < 20000; i++);

    /* Clear startup transient IRQ flags */
    mmio_write32(LCDC_BASE + LCD_IRQSTATUS, 0xFFFFFFFF);

    pr_info("[LCDC] Raster active\n");
}

/* ============================================================
 * Public Accessors — legacy. New code reads from the
 * registered fb_info via fb_get_buffer() / fb_get_width().
 * ============================================================ */

uint16_t *lcdc_get_framebuffer(void)
{
    return fb_pixels;
}

uint32_t lcdc_get_width(void)
{
    return DISPLAY_WIDTH;
}

uint32_t lcdc_get_height(void)
{
    return DISPLAY_HEIGHT;
}

uint32_t lcdc_get_pitch(void)
{
    return DISPLAY_WIDTH * (DISPLAY_BPP / 8);
}

/* ============================================================
 * fb_info wiring — register the LCDC framebuffer with fbdev so
 * fbmem can read geometry through the subsystem instead of via
 * direct lcdc_get_* accessors.
 * ============================================================ */

#include "vinix/fb.h"

static struct fb_info tilcdc_fb_info = {
    .var = {
        .xres            = DISPLAY_WIDTH,
        .yres            = DISPLAY_HEIGHT,
        .bits_per_pixel  = DISPLAY_BPP,
    },
    .fix = {
        .id              = "tilcdc",
        .line_length     = DISPLAY_WIDTH * (DISPLAY_BPP / 8),
        .smem_len        = DISPLAY_WIDTH * DISPLAY_HEIGHT * (DISPLAY_BPP / 8),
    },
};

void lcdc_register_fb(void)
{
    tilcdc_fb_info.screen_base       = fb_pixels;
    tilcdc_fb_info.fix.smem_start    = (uint32_t)fb_pixels;
    register_framebuffer(&tilcdc_fb_info);
}