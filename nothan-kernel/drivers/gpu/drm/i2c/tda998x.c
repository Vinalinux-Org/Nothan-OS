/*
 * drivers/gpu/drm/i2c/tda998x.c — NXP TDA19988 HDMI transmitter driver
 *
 * Controls the TDA19988 over I2C0 to configure video output at
 * 800x600@60Hz (VESA DMT).  Handles PLL setup, AVI InfoFrame,
 * and HDMI vs DVI mode selection.
 */

#include "tda19988.h"
#include "i2c.h"
#include "uart.h"

/*
 * 800x600@60Hz timing (VESA DMT)
 *
 * H: active=800, FP=40, SW=128, BP=88, total=1056
 * V: active=600, FP=1,  SW=4,   BP=23, total=628
 * Pixel clock: 40 MHz
 * Sync: PHSYNC, PVSYNC (both positive)
 */

#define TSVGA_HTOTAL        1056
#define TSVGA_VTOTAL        628
#define TSVGA_HACTIVE       800
#define TSVGA_VACTIVE       600
#define TSVGA_HFP           40
#define TSVGA_HSW           128
#define TSVGA_HBP           88
#define TSVGA_VFP           1
#define TSVGA_VSW           4
#define TSVGA_VBP           23
#define TSVGA_HSKEW         128

/* TDA timing register values derived from the VESA DMT parameters above. */
#define TDA_REF_PIX         (3 + TSVGA_HFP + TSVGA_HSKEW)              /* 171 */
#define TDA_REF_LINE        (1 + TSVGA_VFP)                             /* 2   */
#define TDA_DE_PIX_S        (TSVGA_HTOTAL - TSVGA_HACTIVE)              /* 256 */
#define TDA_DE_PIX_E        (TDA_DE_PIX_S + TSVGA_HACTIVE)              /* 1056*/
#define TDA_HS_PIX_S        TSVGA_HFP                                   /* 40  */
#define TDA_HS_PIX_E        (TSVGA_HFP + TSVGA_HSW)                     /* 168 */
#define TDA_VS1_LINE_S      TSVGA_VFP                                   /* 1   */
#define TDA_VS1_LINE_E      (TSVGA_VFP + TSVGA_VSW)                     /* 5   */
#define TDA_VWIN1_LINE_S    (TSVGA_VTOTAL - TSVGA_VACTIVE - 1)          /* 27  */
#define TDA_VWIN1_LINE_E    (TDA_VWIN1_LINE_S + TSVGA_VACTIVE)          /* 627 */

/*
 * Internal state
 */
static uint8_t g_current_page = 0xFF;   /* invalid sentinel */

/*
 * Low-level I2C primitives — go through generic i2c-core.
 */

#include "nothan/i2c.h"

static struct i2c_adapter *tda_adap = 0;

static int tda_xfer_write(uint8_t addr, uint8_t reg, uint8_t val)
{
    if (!tda_adap) tda_adap = i2c_get_adapter(0);
    if (!tda_adap) return -1;

    uint8_t buf[2] = { reg, val };
    struct i2c_msg msg = {
        .addr = addr, .flags = 0, .len = 2, .buf = buf,
    };
    return (i2c_transfer(tda_adap, &msg, 1) == 1) ? 0 : -1;
}

static int tda_xfer_read(uint8_t addr, uint8_t reg, uint8_t *val)
{
    if (!tda_adap) tda_adap = i2c_get_adapter(0);
    if (!tda_adap) return -1;

    uint8_t reg_buf = reg;
    struct i2c_msg msgs[2] = {
        { .addr = addr, .flags = 0,        .len = 1, .buf = &reg_buf },
        { .addr = addr, .flags = I2C_M_RD, .len = 1, .buf = val      },
    };
    return (i2c_transfer(tda_adap, msgs, 2) == 2) ? 0 : -1;
}

static void tda_set_page(uint8_t page)
{
    if (page != g_current_page) {
        if (tda_xfer_write(TDA_HDMI_I2C_ADDR, TDA_CURPAGE_ADDR, page) != 0)
            pr_err("[TDA] ERR: page switch to 0x%02x failed\n", page);
        g_current_page = page;
    }
}

static int tda_write(uint16_t reg, uint8_t val)
{
    tda_set_page(TDA_PAGE(reg));
    return tda_xfer_write(TDA_HDMI_I2C_ADDR, TDA_ADDR(reg), val);
}

static int tda_read(uint16_t reg, uint8_t *val)
{
    tda_set_page(TDA_PAGE(reg));
    return tda_xfer_read(TDA_HDMI_I2C_ADDR, TDA_ADDR(reg), val);
}

/* Read-modify-write: set bits */
static void tda_set(uint16_t reg, uint8_t bits)
{
    uint8_t val = 0;
    tda_read(reg, &val);
    val |= bits;
    tda_write(reg, val);
}

/* Read-modify-write: clear bits */
static void tda_clear(uint16_t reg, uint8_t bits)
{
    uint8_t val = 0;
    tda_read(reg, &val);
    val &= ~bits;
    tda_write(reg, val);
}

static int tda_cec_write(uint8_t reg, uint8_t val)
{
    return tda_xfer_write(TDA_CEC_I2C_ADDR, reg, val);
}

static void tda_mdelay(volatile uint32_t ms)
{
    /* Empirically calibrated from UART log timing:
     * tda_mdelay(100) with 50000 multiplier took ~20-30 seconds real time.
     * That means 1 iteration ≈ 4-6 µs → CPU effective rate ~200 KHz equiv.
     * Using 200 iters/ms to get ~1ms actual wall time.
     * This accounts for volatile access overhead on slow peripheral bus. */
    volatile uint32_t count = ms * 200;
    while (count--);
}

static void tda_write16(uint16_t reg_msb, uint16_t val16)
{
    tda_write(reg_msb,     (uint8_t)((val16 >> 8) & 0xFF));
    tda_write(reg_msb + 1, (uint8_t)(val16 & 0xFF));
}

/*
 * tda_probe - initialize CEC, soft reset, PLL, DDC
 */

static void tda_probe(void)
{
    tda_cec_write(TDA_CEC_ENAMODS, ENAMODS_RXSENS | ENAMODS_HDMI);

    /* Soft reset */
    tda_write(REG_SOFTRESET, SOFTRESET_AUDIO | SOFTRESET_I2C);
    tda_mdelay(50);
    tda_write(REG_SOFTRESET, 0);
    tda_mdelay(50);

    /* Core reset */
    tda_set(REG_MAIN_CNTRL0, MAIN_CNTRL0_SR);
    tda_clear(REG_MAIN_CNTRL0, MAIN_CNTRL0_SR);
    g_current_page = 0xFF;

    /* PLL common config */
    tda_write(REG_PLL_SERIAL_1, 0x00);
    tda_write(REG_PLL_SERIAL_2, PLL_SERIAL_2_SRL_NOSC(1));
    tda_write(REG_PLL_SERIAL_3, 0x00);
    tda_write(REG_SERIALIZER,   0x00);
    tda_write(REG_BUFFER_OUT,   0x00);
    tda_write(REG_PLL_SCG1,     0x00);
    tda_write(REG_AUDIO_DIV,    AUDIO_DIV_SERCLK_8);
    tda_write(REG_SEL_CLK,      SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK);
    tda_write(REG_PLL_SCGN1,    0xFA);
    tda_write(REG_PLL_SCGN2,    0x00);
    tda_write(REG_PLL_SCGR1,    0x5B);
    tda_write(REG_PLL_SCGR2,    0x00);
    tda_write(REG_PLL_SCG2,     0x10);
    tda_write(REG_MUX_VP_VIP_OUT, 0x24);

    /* DDC + FRO */
    tda_write(REG_DDC_DISABLE, 0x00);
    tda_set(REG_I2C_MASTER, I2C_MASTER_DIS_MM);
    tda_cec_write(TDA_CEC_FRO_IM_CLK_CTRL,
                  FRO_IM_CLK_CTRL_GHOST_DIS | FRO_IM_CLK_CTRL_IMCLK_SEL);
}

/*
 * AVI InfoFrame — mandatory in HDMI mode
 * Without this, TV will blank after detecting the source.
 */

static void tda_write_avi_infoframe(void)
{
    /* NXP BSL pattern: disable IF2 → write all bytes → re-enable IF2. */
    uint8_t buf[17];
    uint8_t sum;

    tda_clear(REG_DIP_IF_FLAGS, DIP_IF_FLAGS_IF1);

    buf[0]  = 0x82;   /* HB0: AVI InfoFrame type */
    buf[1]  = 0x02;   /* HB1: version 2 */
    buf[2]  = 0x0D;   /* HB2: length = 13 bytes */
    buf[4]  = 0x10;   /* PB1: Y=00 (RGB), A0=1 (active format valid) */
    buf[5]  = 0x18;   /* PB2: C=00, M=01 (4:3), R=1000 (same as coded) */
    buf[6]  = 0x00;
    buf[7]  = 0x00;   /* PB4: VIC=0 (800x600 is VESA-only, no CEA VIC) */
    buf[8]  = 0x00;
    buf[9]  = 0x00;   buf[10] = 0x00;
    buf[11] = 0x00;   buf[12] = 0x00;
    buf[13] = 0x00;   buf[14] = 0x00;
    buf[15] = 0x00;   buf[16] = 0x00;

    /* Checksum byte: total of all 17 bytes must be 0 mod 256. */
    sum = 0;
    for (int i = 0; i < 17; i++) {
        if (i == 3) continue;
        sum += buf[i];
    }
    buf[3] = (uint8_t)(256 - sum);

    for (int i = 0; i < 17; i++) {
        tda_write(REG_AVI_IF + i, buf[i]);
    }

    tda_set(REG_DIP_IF_FLAGS, DIP_IF_FLAGS_IF1);
}

/*
 * Enable: video path configuration + output enable
 */

static void tda_enable_video(void)
{
    /* Enable ports + VIP mux */
    tda_write(REG_ENA_AP, 0x03);    /* enable audio ports */
    tda_write(REG_ENA_VP_0, 0xFF);
    tda_write(REG_ENA_VP_1, 0xFF);
    tda_write(REG_ENA_VP_2, 0xFF);
    /* VIP muxing for 16-bit RGB565 */
    tda_write(REG_VIP_CNTRL_0, 0x23);  /* SWAP_A=2, SWAP_B=3 */
    tda_write(REG_VIP_CNTRL_1, 0x01);  /* SWAP_C=0, SWAP_D=1 */
    tda_write(REG_VIP_CNTRL_2, 0x45);  /* SWAP_E=4, SWAP_F=5 */

    /* Mute audio FIFO */
    tda_set(REG_AIP_CNTRL_0, AIP_CNTRL_0_RST_FIFO);

    /* Disable output during config, set HDMI mode (not DVI).
     * Modern TVs on HDMI input often require HDMI signaling — DVI mode
     * causes TV to detect source but show black screen. */
    tda_set(REG_TBG_CNTRL_1, TBG_CNTRL_1_DWIN_DIS);
    tda_set(REG_TX33, TX33_HDMI);
    tda_write(REG_ENC_CNTRL, ENC_CNTRL_CTL_CODE(0));

    /* No pre-filter or interpolator */
    tda_write(REG_HVF_CNTRL_0, HVF_CNTRL_0_PREFIL(0) | HVF_CNTRL_0_INTPOL(0));

    /* QNX does NOT write FEAT_POWERDOWN — leave at default */

    tda_write(REG_VIP_CNTRL_5, VIP_CNTRL_5_SP_CNT(0));
    /* No test pattern — pass through LCDC pixel data */
    tda_write(REG_VIP_CNTRL_4, VIP_CNTRL_4_BLANKIT(0) | VIP_CNTRL_4_BLC(0));

    /* PLL + serializer */
    tda_clear(REG_PLL_SERIAL_3, PLL_SERIAL_3_SRL_CCIR);
    tda_clear(REG_PLL_SERIAL_1, PLL_SERIAL_1_SRL_MAN_IZ);
    tda_clear(REG_PLL_SERIAL_3, PLL_SERIAL_3_SRL_DE);
    tda_write(REG_SERIALIZER, 0);
    tda_write(REG_HVF_CNTRL_1, HVF_CNTRL_1_VQR(0));
    tda_write(REG_RPT_CNTRL, 0);
    tda_write(REG_SEL_CLK, SEL_CLK_SEL_VRF_CLK(0) |
              SEL_CLK_SEL_CLK1 | SEL_CLK_ENA_SC_CLK);
    /* NOSC = 148500/pixel_clock_kHz - 1, clamped to 3.
     * 800x600@40MHz: 148500/40000 = 3, then -1 = 2 */
    tda_write(REG_PLL_SERIAL_2, PLL_SERIAL_2_SRL_NOSC(2) |
              PLL_SERIAL_2_SRL_PR(0));

    /* Color matrix: bypass only */
    tda_set(REG_MAT_CONTRL, MAT_CONTRL_MAT_BP);

    /* Analog output — critical for TMDS signal generation */
    tda_write(REG_ANA_GENERAL, 0x09);

    /* Timing registers — computed from VESA DMT 800x600@60Hz parameters. */

    tda_write(REG_VIDFORMAT, 0x00);

    tda_write16(REG_REFPIX_MSB,  TDA_REF_PIX);                   /* 153  */
    tda_write16(REG_REFLINE_MSB, TDA_REF_LINE);                  /* 6    */

    tda_write16(REG_NPIX_MSB,  TSVGA_HTOTAL);                     /* 1056 */
    tda_write16(REG_NLINE_MSB, TSVGA_VTOTAL);                    /* 628  */

    /* VSYNC (progressive) */
    tda_write16(REG_VS_LINE_STRT_1_MSB, TDA_VS1_LINE_S);         /* 5   */
    tda_write16(REG_VS_PIX_STRT_1_MSB,  TDA_HS_PIX_S);           /* 110 */
    tda_write16(REG_VS_LINE_END_1_MSB,  TDA_VS1_LINE_E);         /* 10  */
    tda_write16(REG_VS_PIX_END_1_MSB,   TDA_HS_PIX_S);           /* 110 */

    /* Progressive: field 2 unused */
    tda_write16(REG_VS_LINE_STRT_2_MSB, 0);
    tda_write16(REG_VS_PIX_STRT_2_MSB,  0);
    tda_write16(REG_VS_LINE_END_2_MSB,  0);
    tda_write16(REG_VS_PIX_END_2_MSB,   0);

    /* HSYNC */
    tda_write16(REG_HS_PIX_START_MSB, TDA_HS_PIX_S);             /* 110 */
    tda_write16(REG_HS_PIX_STOP_MSB,  TDA_HS_PIX_E);             /* 150 */

    /* VWIN: active video window (QNX formula: vtotal - vactive - 1) */
    tda_write16(REG_VWIN_START_1_MSB, TDA_VWIN1_LINE_S);         /* 29  */
    tda_write16(REG_VWIN_END_1_MSB,   TDA_VWIN1_LINE_E);         /* 749 */
    tda_write16(REG_VWIN_START_2_MSB, 0);
    tda_write16(REG_VWIN_END_2_MSB,   0);

    /* DE: data enable (QNX formula: htotal - hactive to htotal) */
    tda_write16(REG_DE_START_MSB, TDA_DE_PIX_S);                 /* 370  */
    tda_write16(REG_DE_STOP_MSB,  TDA_DE_PIX_E);                 /* 1650 */

    /* TDA19988: enable active space fill */
    tda_write(REG_ENABLE_SPACE, 0x01);

    tda_clear(REG_TBG_CNTRL_0, TBG_CNTRL_0_SYNC_MTHD);

    /* VIP_CNTRL_3: sync on HSYNC.
     * 800x600 has PHSYNC/PVSYNC → no toggle needed. */
    tda_write(REG_VIP_CNTRL_3, 0);
    tda_set(REG_VIP_CNTRL_3, VIP_CNTRL_3_SYNC_HS);

    /* TBG: TGL_EN only, no H_TGL/V_TGL for positive sync */
    tda_write(REG_TBG_CNTRL_1, TBG_CNTRL_1_TGL_EN);

    /* Enable HDMI + encoder */
    tda_clear(REG_TBG_CNTRL_1, TBG_CNTRL_1_DWIN_DIS);
    tda_write(REG_ENC_CNTRL, ENC_CNTRL_CTL_CODE(1));

    /* MUST BE LAST: latch all timing changes */
    tda_clear(REG_TBG_CNTRL_0, TBG_CNTRL_0_SYNC_ONCE);

    /* AVI InfoFrame — write AFTER video path is fully enabled.
     * NXP BSL pattern: disable IF2 → write → enable IF2. */
    tda_write_avi_infoframe();

    /* Video path ready */
}

/*
 * Public API
 */

void tda19988_init(void)
{
    tda_probe();
    tda_enable_video();
    pr_info("[HDMI] TDA19988 ready\n");
}