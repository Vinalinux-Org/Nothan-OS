/*
 * drivers/video/tda19988.c - NXP TDA19988 HDMI framer driver
 *
 * Configures the TDA19988 via I2C0 for 1280x720@60Hz HDMI output.
 * The TDA19988 receives parallel RGB from the LCDC and outputs HDMI.
 *
 * I2C addresses (BBB):
 *   HDMI sub-device: 0x70 (I2C0)
 *   CEC  sub-device: 0x34 (I2C0, same adapter, different addr)
 *
 * Register scheme: paged. Write 0xFF to select page, then access registers
 * within that page. CEC registers are unpagedand use the CEC address.
 *
 * Init sequence derived from Linux tda998x_drv.c (Rob Clark, TI 2012).
 * Written independently for NothanOS.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/i2c.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/delay.h>

/* I2C sub-device addresses */
#define TDA_HDMI_ADDR   0x70
#define TDA_CEC_ADDR    0x34    /* 0x34 + (0x70 & 0x03) = 0x34 */

/* Paged register encoding: REG(page, addr) */
#define REG(page, addr)     (((u16)(page) << 8) | (u8)(addr))
#define REG2PAGE(r)         ((u8)((r) >> 8))
#define REG2ADDR(r)         ((u8)((r) & 0xFF))
#define REG_CURPAGE         0xFF    /* page select register */

/* Page 00h: general control */
#define REG_VERSION_LSB     REG(0x00, 0x00)
#define REG_MAIN_CNTRL0     REG(0x00, 0x01)
# define MAIN_CNTRL0_SR         (1 << 0)
#define REG_VERSION_MSB     REG(0x00, 0x02)
#define REG_SOFTRESET       REG(0x00, 0x0A)
# define SOFTRESET_AUDIO        (1 << 0)
# define SOFTRESET_I2C_MASTER   (1 << 1)
#define REG_DDC_DISABLE     REG(0x00, 0x0B)
#define REG_FEAT_POWERDOWN  REG(0x00, 0x0E)
# define FEAT_POWERDOWN_PREFILT (1 << 0)
# define FEAT_POWERDOWN_CSC     (1 << 1)
#define REG_ENA_VP_0        REG(0x00, 0x18)
#define REG_ENA_VP_1        REG(0x00, 0x19)
#define REG_ENA_VP_2        REG(0x00, 0x1A)
#define REG_VIP_CNTRL_0     REG(0x00, 0x20)
#define REG_VIP_CNTRL_1     REG(0x00, 0x21)
#define REG_VIP_CNTRL_2     REG(0x00, 0x22)
#define REG_VIP_CNTRL_3     REG(0x00, 0x23)
# define VIP_CNTRL_3_SYNC_HS    (1 << 5)
#define REG_VIP_CNTRL_4     REG(0x00, 0x24)
#define REG_VIP_CNTRL_5     REG(0x00, 0x25)
#define REG_MUX_VP_VIP_OUT  REG(0x00, 0x27)
#define REG_MAT_CONTRL      REG(0x00, 0x80)
# define MAT_CONTRL_MAT_BP      (1 << 2)
# define MAT_CONTRL_MAT_SC(x)   (((x) & 3) << 0)
#define REG_VIDFORMAT       REG(0x00, 0xA0)
#define REG_REFPIX_MSB      REG(0x00, 0xA1)
#define REG_REFLINE_MSB     REG(0x00, 0xA3)
#define REG_NPIX_MSB        REG(0x00, 0xA5)
#define REG_NLINE_MSB       REG(0x00, 0xA7)
#define REG_VS_LINE_STRT_1_MSB  REG(0x00, 0xA9)
#define REG_VS_PIX_STRT_1_MSB  REG(0x00, 0xAB)
#define REG_VS_LINE_END_1_MSB  REG(0x00, 0xAD)
#define REG_VS_PIX_END_1_MSB   REG(0x00, 0xAF)
#define REG_VS_LINE_STRT_2_MSB REG(0x00, 0xB1)
#define REG_VS_PIX_STRT_2_MSB  REG(0x00, 0xB3)
#define REG_VS_LINE_END_2_MSB  REG(0x00, 0xB5)
#define REG_VS_PIX_END_2_MSB   REG(0x00, 0xB7)
#define REG_HS_PIX_START_MSB   REG(0x00, 0xB9)
#define REG_HS_PIX_STOP_MSB    REG(0x00, 0xBB)
#define REG_VWIN_START_1_MSB   REG(0x00, 0xBD)
#define REG_VWIN_END_1_MSB     REG(0x00, 0xBF)
#define REG_VWIN_START_2_MSB   REG(0x00, 0xC1)
#define REG_VWIN_END_2_MSB     REG(0x00, 0xC3)
#define REG_DE_START_MSB       REG(0x00, 0xC5)
#define REG_DE_STOP_MSB        REG(0x00, 0xC7)
#define REG_TBG_CNTRL_0     REG(0x00, 0xCA)
#define REG_TBG_CNTRL_1     REG(0x00, 0xCB)
# define TBG_CNTRL_1_DWIN_DIS   (1 << 6)
# define TBG_CNTRL_1_TGL_EN     (1 << 2)
#define REG_HVF_CNTRL_0     REG(0x00, 0xE4)
#define REG_HVF_CNTRL_1     REG(0x00, 0xE5)
#define REG_RPT_CNTRL       REG(0x00, 0xF0)
#define REG_ENABLE_SPACE    REG(0x00, 0xD6)

/* Page 02h: PLL */
#define REG_PLL_SERIAL_1    REG(0x02, 0x00)
#define REG_PLL_SERIAL_2    REG(0x02, 0x01)
# define PLL_SERIAL_2_SRL_NOSC(x) ((x) << 0)
#define REG_PLL_SERIAL_3    REG(0x02, 0x02)
#define REG_SERIALIZER      REG(0x02, 0x03)
#define REG_BUFFER_OUT      REG(0x02, 0x04)
#define REG_PLL_SCG1        REG(0x02, 0x05)
#define REG_PLL_SCG2        REG(0x02, 0x06)
#define REG_PLL_SCGN1       REG(0x02, 0x07)
#define REG_PLL_SCGN2       REG(0x02, 0x08)
#define REG_PLL_SCGR1       REG(0x02, 0x09)
#define REG_PLL_SCGR2       REG(0x02, 0x0A)
#define REG_AUDIO_DIV       REG(0x02, 0x0E)
# define AUDIO_DIV_SERCLK_8     3
#define REG_SEL_CLK         REG(0x02, 0x11)
# define SEL_CLK_SEL_CLK1       (1 << 0)
# define SEL_CLK_ENA_SC_CLK     (1 << 3)
#define REG_ANA_GENERAL     REG(0x02, 0x12)

/* Page 11h: audio */
#define REG_AIP_CNTRL_0     REG(0x11, 0x00)
# define AIP_CNTRL_0_RST_FIFO   (1 << 0)
#define REG_ENC_CNTRL       REG(0x11, 0x0D)
# define ENC_CNTRL_CTL_CODE(x)  (((x) & 3) << 2)

/* Page 12h: HDCP/OTP */
#define REG_TX3             REG(0x12, 0x9A)
#define REG_TX33            REG(0x12, 0xB8)
# define TX33_HDMI              (1 << 1)

/* CEC registers (unpagedwrite to TDA_CEC_ADDR) */
#define CEC_ENAMODS         0xFF
# define CEC_ENAMODS_EN_RXSENS  (1 << 2)
# define CEC_ENAMODS_EN_HDMI    (1 << 1)
#define CEC_FRO_IM_CLK_CTRL 0xFB
# define CEC_FRO_IM_CLK_CTRL_GHOST_DIS  (1 << 7)
# define CEC_FRO_IM_CLK_CTRL_IMCLK_SEL  (1 << 1)
#define CEC_RXSHPDINTENA    0xFC

/* TDA19988 chip revision */
#define TDA19988_REV        0x0301

/*
 * 720p@60Hz CEA-861 timing (pixel clock 74.25 MHz):
 *   H: 1280 active + HFP=110 + HSW=40 + HBP=220 = htotal 1650
 *   V:  720 active + VFP=5   + VSW=5  + VBP=20  = vtotal  750
 *   Sync: PHSYNC, PVSYNC
 */
#define N_PIX           1650U
#define N_LINE           750U
#define HS_PIX_S         110U   /* hsync_start - hdisplay */
#define HS_PIX_E         150U   /* hsync_end   - hdisplay */
#define DE_PIX_S         370U   /* htotal - hdisplay */
#define DE_PIX_E        1650U   /* htotal */
#define REF_PIX          113U   /* 3 + hs_pix_s */
#define REF_LINE           6U   /* 1 + (vsync_start - vdisplay) */
#define VWIN1_LINE_S      29U   /* vtotal - vdisplay - 1 */
#define VWIN1_LINE_E     749U   /* vwin1_line_s + vdisplay */
#define VS1_LINE_S         5U   /* vsync_start - vdisplay */
#define VS1_LINE_E        10U   /* vs1_line_s + vsw */
#define VS1_PIX          110U   /* = hs_pix_s */

/*
 * PLL divisor for 74.25 MHz TMDS: loop breaks at div=1
 * (condition: 80000 >> div <= 74250 kHz → div=1 → NOSC=1)
 */
#define PLL_NOSC    1

/* VIP channel swap for BBB LCDC→TDA19988 wiring (from working old-OS) */
#define VIP_CNTRL_0_VAL  0x23   /* SWAP_A=2, SWAP_B=3 */
#define VIP_CNTRL_1_VAL  0x01   /* SWAP_C=0, SWAP_D=1 */
#define VIP_CNTRL_2_VAL  0x45   /* SWAP_E=4, SWAP_F=5 */

/* Tracks current HDMI page to avoid redundant page-select writes */
static u8 s_page = 0xFF;

/* ------------------------------------------------------------------ */
/* Low-level I/O primitives                                             */
/* ------------------------------------------------------------------ */

static int cec_write(struct i2c_client *hdmi, u8 reg, u8 val)
{
	u8 buf[2] = { reg, val };
	struct i2c_msg msg = {
		.addr  = TDA_CEC_ADDR,
		.flags = 0,
		.len   = 2,
		.buf   = buf,
	};
	return hdmi->adapter->xfer(hdmi->adapter, &msg, 1);
}

static int cec_read(struct i2c_client *hdmi, u8 reg, u8 *val)
{
	struct i2c_msg msgs[2] = {
		{ .addr = TDA_CEC_ADDR, .flags = 0,          .len = 1, .buf = &reg  },
		{ .addr = TDA_CEC_ADDR, .flags = I2C_M_RD,   .len = 1, .buf = val   },
	};
	return hdmi->adapter->xfer(hdmi->adapter, msgs, 2);
}

static int page_select(struct i2c_client *c, u8 page)
{
	u8 buf[2] = { REG_CURPAGE, page };

	if (s_page == page)
		return 0;
	if (i2c_master_send(c, buf, 2) < 0)
		return -1;
	s_page = page;
	return 0;
}

static int reg_write(struct i2c_client *c, u16 reg, u8 val)
{
	u8 buf[2] = { REG2ADDR(reg), val };

	if (page_select(c, REG2PAGE(reg)) < 0)
		return -1;
	return i2c_master_send(c, buf, 2) < 0 ? -1 : 0;
}

static int reg_read(struct i2c_client *c, u16 reg, u8 *val)
{
	u8 addr = REG2ADDR(reg);

	if (page_select(c, REG2PAGE(reg)) < 0)
		return -1;
	if (i2c_master_send(c, &addr, 1) < 0)
		return -1;
	return i2c_master_recv(c, val, 1) < 0 ? -1 : 0;
}

static int reg_set(struct i2c_client *c, u16 reg, u8 bits)
{
	u8 val;

	if (reg_read(c, reg, &val) < 0)
		return -1;
	return reg_write(c, reg, val | bits);
}

static int reg_clear(struct i2c_client *c, u16 reg, u8 bits)
{
	u8 val;

	if (reg_read(c, reg, &val) < 0)
		return -1;
	return reg_write(c, reg, val & (u8)~bits);
}

/* Write 16-bit value as MSB then LSB to consecutive register addresses */
static int reg_write16(struct i2c_client *c, u16 reg, u16 val)
{
	if (reg_write(c, reg,     (u8)(val >> 8))   < 0) return -1;
	if (reg_write(c, reg + 1, (u8)(val & 0xFF)) < 0) return -1;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Init, mode-set, enable                                               */
/* ------------------------------------------------------------------ */

static int tda19988_reset(struct i2c_client *c)
{
	printk("[TDA19988] reset: softreset audio+i2c_master\n");
	/* Reset audio and I2C master */
	if (reg_write(c, REG_SOFTRESET, SOFTRESET_AUDIO | SOFTRESET_I2C_MASTER) < 0)
		return -1;
	mdelay(50);
	if (reg_write(c, REG_SOFTRESET, 0x00) < 0)
		return -1;
	mdelay(50);

	/* Reset transmitter */
	if (reg_set(c,   REG_MAIN_CNTRL0, MAIN_CNTRL0_SR) < 0) return -1;
	if (reg_clear(c, REG_MAIN_CNTRL0, MAIN_CNTRL0_SR) < 0) return -1;

	/* PLL common configuration */
	if (reg_write(c, REG_PLL_SERIAL_1, 0x00) < 0) return -1;
	if (reg_write(c, REG_PLL_SERIAL_2, PLL_SERIAL_2_SRL_NOSC(1)) < 0) return -1;
	if (reg_write(c, REG_PLL_SERIAL_3, 0x00) < 0) return -1;
	if (reg_write(c, REG_SERIALIZER,   0x00) < 0) return -1;
	if (reg_write(c, REG_BUFFER_OUT,   0x00) < 0) return -1;
	if (reg_write(c, REG_PLL_SCG1,     0x00) < 0) return -1;
	if (reg_write(c, REG_AUDIO_DIV,    AUDIO_DIV_SERCLK_8) < 0) return -1;
	if (reg_write(c, REG_SEL_CLK,      SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK) < 0) return -1;
	if (reg_write(c, REG_PLL_SCGN1,    0xFA) < 0) return -1;
	if (reg_write(c, REG_PLL_SCGN2,    0x00) < 0) return -1;
	if (reg_write(c, REG_PLL_SCGR1,    0x5B) < 0) return -1;
	if (reg_write(c, REG_PLL_SCGR2,    0x00) < 0) return -1;
	if (reg_write(c, REG_PLL_SCG2,     0x10) < 0) return -1;
	if (reg_write(c, REG_MUX_VP_VIP_OUT, 0x24) < 0) return -1;

	printk("[TDA19988] reset: PLL config done\n");
	return 0;
}

static int tda19988_mode_set_720p(struct i2c_client *c)
{
	printk("[TDA19988] mode_set: 720p@60Hz\n");
	/* Mute audio FIFO */
	if (reg_set(c, REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_FIFO) < 0) return -1;

	/* Disable data window and HDCP */
	if (reg_write(c, REG_TBG_CNTRL_1, TBG_CNTRL_1_DWIN_DIS) < 0) return -1;
	if (reg_clear(c, REG_TX33, TX33_HDMI) < 0) return -1;
	if (reg_write(c, REG_ENC_CNTRL, ENC_CNTRL_CTL_CODE(0)) < 0) return -1;

	/* No pre-filter or interpolator */
	if (reg_write(c, REG_HVF_CNTRL_0, 0x00) < 0) return -1;
	if (reg_set(c,   REG_FEAT_POWERDOWN, FEAT_POWERDOWN_PREFILT) < 0) return -1;
	if (reg_write(c, REG_VIP_CNTRL_5, 0x00) < 0) return -1;
	if (reg_write(c, REG_VIP_CNTRL_4, 0x00) < 0) return -1;

	/* PLL for 74.25 MHz TMDS (div=1 → NOSC=1) */
	if (reg_write(c, REG_SERIALIZER,   0x00) < 0) return -1;
	if (reg_write(c, REG_HVF_CNTRL_1,  0x00) < 0) return -1;
	if (reg_write(c, REG_RPT_CNTRL,    0x00) < 0) return -1;  /* no pixel repeat */
	if (reg_write(c, REG_SEL_CLK,
		      SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK) < 0) return -1;
	if (reg_write(c, REG_PLL_SERIAL_2,
		      PLL_SERIAL_2_SRL_NOSC(PLL_NOSC)) < 0) return -1;

	/* Color matrix: bypass (full-range RGB, no EDID available) */
	if (reg_write(c, REG_MAT_CONTRL,
		      MAT_CONTRL_MAT_BP | MAT_CONTRL_MAT_SC(1)) < 0) return -1;
	if (reg_set(c, REG_FEAT_POWERDOWN, FEAT_POWERDOWN_CSC) < 0) return -1;

	/* BIAS for TMDS */
	if (reg_write(c, REG_ANA_GENERAL, 0x09) < 0) return -1;

	/* Sync: rising edge HS (PHSYNC, PVSYNC → no toggle needed) */
	if (reg_write(c, REG_VIP_CNTRL_3, VIP_CNTRL_3_SYNC_HS) < 0) return -1;

	if (reg_write(c, REG_VIDFORMAT, 0x00) < 0) return -1;

	/* Timing registers (16-bit: MSB at reg, LSB at reg+1) */
	if (reg_write16(c, REG_REFPIX_MSB,        REF_PIX)       < 0) return -1;
	if (reg_write16(c, REG_REFLINE_MSB,        REF_LINE)      < 0) return -1;
	if (reg_write16(c, REG_NPIX_MSB,           N_PIX)         < 0) return -1;
	if (reg_write16(c, REG_NLINE_MSB,          N_LINE)        < 0) return -1;
	if (reg_write16(c, REG_VS_LINE_STRT_1_MSB, VS1_LINE_S)   < 0) return -1;
	if (reg_write16(c, REG_VS_PIX_STRT_1_MSB,  VS1_PIX)      < 0) return -1;
	if (reg_write16(c, REG_VS_LINE_END_1_MSB,   VS1_LINE_E)   < 0) return -1;
	if (reg_write16(c, REG_VS_PIX_END_1_MSB,    VS1_PIX)      < 0) return -1;
	if (reg_write16(c, REG_VS_LINE_STRT_2_MSB,  0)            < 0) return -1;
	if (reg_write16(c, REG_VS_PIX_STRT_2_MSB,   0)            < 0) return -1;
	if (reg_write16(c, REG_VS_LINE_END_2_MSB,    0)            < 0) return -1;
	if (reg_write16(c, REG_VS_PIX_END_2_MSB,     0)            < 0) return -1;
	if (reg_write16(c, REG_HS_PIX_START_MSB,   HS_PIX_S)     < 0) return -1;
	if (reg_write16(c, REG_HS_PIX_STOP_MSB,    HS_PIX_E)     < 0) return -1;
	if (reg_write16(c, REG_VWIN_START_1_MSB,   VWIN1_LINE_S) < 0) return -1;
	if (reg_write16(c, REG_VWIN_END_1_MSB,     VWIN1_LINE_E) < 0) return -1;
	if (reg_write16(c, REG_VWIN_START_2_MSB,    0)            < 0) return -1;
	if (reg_write16(c, REG_VWIN_END_2_MSB,      0)            < 0) return -1;
	if (reg_write16(c, REG_DE_START_MSB,       DE_PIX_S)     < 0) return -1;
	if (reg_write16(c, REG_DE_STOP_MSB,        DE_PIX_E)     < 0) return -1;

	/* TDA19988-specific: fill active space */
	if (reg_write(c, REG_ENABLE_SPACE, 0x00) < 0) return -1;

	/* Output sync polarity: regenerate from input, toggle enabled */
	if (reg_write(c, REG_TBG_CNTRL_1,
		      TBG_CNTRL_1_DWIN_DIS | TBG_CNTRL_1_TGL_EN) < 0) return -1;

	/*
	 * Enable HDMI mode (TX33_HDMI + ENC_CNTRL CTL_CODE=1).
	 * Clear DWIN_DIS to allow data islands through.
	 * Must happen before TBG_CNTRL_0 is written.
	 */
	if (reg_write(c, REG_TBG_CNTRL_1, TBG_CNTRL_1_TGL_EN) < 0) return -1;
	if (reg_write(c, REG_ENC_CNTRL, ENC_CNTRL_CTL_CODE(1)) < 0) return -1;
	if (reg_set(c,   REG_TX33, TX33_HDMI) < 0) return -1;

	/* Commit timing — must be the last register written */
	if (reg_write(c, REG_TBG_CNTRL_0, 0x00) < 0) return -1;

	printk("[TDA19988] mode_set: timing committed\n");
	return 0;
}

static int tda19988_enable(struct i2c_client *c)
{
	printk("[TDA19988] enable: VP pins + VIP mux\n");
	/* Enable all 24 video port pins */
	if (reg_write(c, REG_ENA_VP_0, 0xFF) < 0) return -1;
	if (reg_write(c, REG_ENA_VP_1, 0xFF) < 0) return -1;
	if (reg_write(c, REG_ENA_VP_2, 0xFF) < 0) return -1;

	/* Set input channel mux after enabling ports */
	if (reg_write(c, REG_VIP_CNTRL_0, VIP_CNTRL_0_VAL) < 0) return -1;
	if (reg_write(c, REG_VIP_CNTRL_1, VIP_CNTRL_1_VAL) < 0) return -1;
	if (reg_write(c, REG_VIP_CNTRL_2, VIP_CNTRL_2_VAL) < 0) return -1;

	return 0;
}

static int tda19988_probe(struct i2c_client *client)
{
	u8 rev_lo, rev_hi;
	u16 rev;

	printk("[TDA19988] probe: CEC wake-up (addr=0x%02x)\n",
	       (unsigned)TDA_CEC_ADDR);
	/*
	 * Wake up HDMI sub-device via CEC. Non-fatal: TDA19988 may already be
	 * awake on BBB (bootloader uses I2C0 for PMIC, not TDA19988 CEC).
	 * Proceed regardless — read version to confirm HDMI sub-device responds.
	 */
	if (cec_write(client, CEC_ENAMODS,
		      CEC_ENAMODS_EN_RXSENS | CEC_ENAMODS_EN_HDMI) < 0)
		printk("[TDA19988] CEC wake-up failed (continuing)\n");
	else {
		printk("[TDA19988] CEC wake-up OK\n");
		mdelay(5);
	}

	if (tda19988_reset(client) < 0) {
		printk("[TDA19988] reset failed\n");
		return -1;
	}

	/* Verify chip revision */
	if (reg_read(client, REG_VERSION_LSB, &rev_lo) < 0 ||
	    reg_read(client, REG_VERSION_MSB, &rev_hi) < 0) {
		printk("[TDA19988] version read failed\n");
		return -1;
	}

	rev = (u16)rev_lo | ((u16)rev_hi << 8);
	printk("[TDA19988] version raw=0x%04x\n", (unsigned)rev);
	rev &= ~0x30U;  /* mask non-hdcp / non-scalar bits */

	if (rev != TDA19988_REV) {
		printk("[TDA19988] unexpected rev 0x%04x\n", (unsigned)rev);
		return -1;
	}
	printk("[TDA19988] found rev 0x%04x\n", (unsigned)rev);

	/* Post-reset: enable DDC, set clock, configure CEC oscillator */
	if (reg_write(client, REG_DDC_DISABLE, 0x00) < 0) return -1;
	if (reg_write(client, REG_TX3,         0x27) < 0) return -1;
	if (cec_write(client, CEC_FRO_IM_CLK_CTRL,
		      CEC_FRO_IM_CLK_CTRL_GHOST_DIS |
		      CEC_FRO_IM_CLK_CTRL_IMCLK_SEL) < 0) return -1;

	/* Disable CEC interrupts (not using CEC) */
	if (cec_write(client, CEC_RXSHPDINTENA, 0x00) < 0) return -1;

	if (tda19988_mode_set_720p(client) < 0) {
		printk("[TDA19988] mode set failed\n");
		return -1;
	}

	if (tda19988_enable(client) < 0) {
		printk("[TDA19988] enable failed\n");
		return -1;
	}

	printk("[TDA19988] HDMI 720p@60Hz ready\n");
	return 0;
}

static struct i2c_driver tda19988_driver = {
	.name  = "tda19988",
	.probe = tda19988_probe,
};

static int tda19988_init(void)
{
	return i2c_add_driver(&tda19988_driver);
}
device_initcall(tda19988_init);
