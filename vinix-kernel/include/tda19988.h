/* ============================================================
 * tda19988.h
 * ------------------------------------------------------------
 * NXP TDA19988 HDMI transmitter driver interface.
 * ============================================================ */

#ifndef TDA19988_H
#define TDA19988_H

#include "types.h"

/* ============================================================
 * Register encoding: (page << 8) | addr
 * Page switch via CURPAGE register 0xFF at HDMI I2C addr
 * CEC has no page mechanism — direct register access
 * ============================================================ */

#define TDA_MKREG(page, addr)   (((uint16_t)(page) << 8) | (addr))
#define TDA_PAGE(reg)           (((reg) >> 8) & 0xFF)
#define TDA_ADDR(reg)           ((reg) & 0xFF)

/* ============================================================
 * I2C slave addresses (7-bit)
 * A1=0, A0=0 → HDMI=0x70, CEC=0x34
 * ============================================================ */
#define TDA_HDMI_I2C_ADDR       0x70
#define TDA_CEC_I2C_ADDR        0x34

/* Page switch register (write at HDMI addr to select page) */
#define TDA_CURPAGE_ADDR        0xFF

/* ============================================================
 * CEC registers (no page; direct access via CEC addr 0x34)
 * ============================================================ */
#define TDA_CEC_ENAMODS         0xFF    /* Module enable */
#define   ENAMODS_RXSENS        (1 << 2)  /* Receiver sense */
#define   ENAMODS_HDMI          (1 << 1)  /* HDMI core */

#define TDA_CEC_FRO_IM_CLK_CTRL  0xFB    /* FRO internal clock control */
#define   FRO_IM_CLK_CTRL_GHOST_DIS (1 << 7)  /* Disable ghost addresses */
#define   FRO_IM_CLK_CTRL_IMCLK_SEL (1 << 1)  /* Select internal master clock */

#define TDA_CEC_RXSHPDLEV       0xFE    /* HPD/RXSENS status (read-only) */
#define   RXSHPDLEV_HPD          (1 << 0)  /* Hot plug detect level */
#define   RXSHPDLEV_RXSENS      (1 << 1)  /* Receiver sense level */

/* ============================================================
 * HDMI core — Page 0x00 (General Control)
 * ============================================================ */
#define REG_VERSION         TDA_MKREG(0x00, 0x00)  /* R: chip version LSB */
#define REG_MAIN_CNTRL0     TDA_MKREG(0x00, 0x01)  /* Main control */
#define   MAIN_CNTRL0_SR    (1 << 0)                /* Soft reset */
#define REG_VERSION_MSB     TDA_MKREG(0x00, 0x02)  /* R: chip version MSB */
#define REG_SOFTRESET       TDA_MKREG(0x00, 0x0A)  /* Soft reset control */
#define   SOFTRESET_AUDIO   (1 << 0)
#define   SOFTRESET_I2C     (1 << 1)
#define REG_CCLK_ON         TDA_MKREG(0x00, 0x0C)  /* CEC clock enable */
#define REG_I2C_MASTER      TDA_MKREG(0x00, 0x0D)  /* I2C master control */
#define   I2C_MASTER_DIS_MM (1 << 0)               /* Disable DDC master */
#define   I2C_MASTER_DIS_FILT (1 << 1)
#define REG_DDC_DISABLE     TDA_MKREG(0x00, 0x0B)  /* DDC interface disable */
#define REG_FEAT_POWERDOWN  TDA_MKREG(0x00, 0x0E)  /* Feature powerdown */
#define   FEAT_POWERDOWN_PREFILT (1 << 0)
#define   FEAT_POWERDOWN_CSC   (1 << 1)
#define   FEAT_POWERDOWN_SPDIF (1 << 3)
#define REG_ENA_VP_0        TDA_MKREG(0x00, 0x18)  /* Enable video port A */
#define REG_ENA_VP_1        TDA_MKREG(0x00, 0x19)  /* Enable video port B */
#define REG_ENA_VP_2        TDA_MKREG(0x00, 0x1A)  /* Enable video port C */
#define REG_ENA_AP          TDA_MKREG(0x00, 0x1E)  /* Enable audio ports */

/* Video Input Port control — SWAP/MIRROR for channel routing */
/* Values for RGB 4:4:4 external sync: VIP_CNTRL_0=0x23, _1=0x45, _2=0x01 */
#define REG_VIP_CNTRL_0     TDA_MKREG(0x00, 0x20)
#define REG_VIP_CNTRL_1     TDA_MKREG(0x00, 0x21)
#define REG_VIP_CNTRL_2     TDA_MKREG(0x00, 0x22)
#define REG_VIP_CNTRL_3     TDA_MKREG(0x00, 0x23)
#define   VIP_CNTRL_3_X_TGL (1 << 0)
#define   VIP_CNTRL_3_H_TGL (1 << 1)
#define   VIP_CNTRL_3_V_TGL (1 << 2)
#define   VIP_CNTRL_3_EMB   (1 << 3)               /* Embedded sync */
#define   VIP_CNTRL_3_SYNC_DE (1 << 4)             /* Use DE for sync */
#define   VIP_CNTRL_3_SYNC_HS (1 << 5)             /* Use HSYNC for sync */
#define   VIP_CNTRL_3_DE_INT (1 << 6)              /* Internal DE */
#define   VIP_CNTRL_3_EDGE  (1 << 7)               /* Latch on falling edge */
#define REG_VIP_CNTRL_4     TDA_MKREG(0x00, 0x24)
#define   VIP_CNTRL_4_BLC(x) (((x) & 0x3) << 0)
#define   VIP_CNTRL_4_BLANKIT(x) (((x) & 0x3) << 2)
#define   VIP_CNTRL_4_CLK_INV (1 << 4)
#define   VIP_CNTRL_4_CCIR656 (1 << 5)
#define   VIP_CNTRL_4_TST_PAT (1 << 7)
#define REG_VIP_CNTRL_5     TDA_MKREG(0x00, 0x25)
#define   VIP_CNTRL_5_SP_CNT(x) (((x) & 0x3) << 0)

#define REG_MUX_VP_VIP_OUT  TDA_MKREG(0x00, 0x27)
#define REG_MAT_CONTRL      TDA_MKREG(0x00, 0x80)  /* Color matrix control */
#define   MAT_CONTRL_MAT_SC(x) (((x) & 0x3) << 0)
#define   MAT_CONTRL_MAT_BP (1 << 2)               /* Bypass matrix */

/* Video format and timing */
#define REG_VIDFORMAT       TDA_MKREG(0x00, 0xA0)
#define REG_REFPIX_MSB      TDA_MKREG(0x00, 0xA1)  /* Horizontal ref pixel */
#define REG_REFPIX_LSB      TDA_MKREG(0x00, 0xA2)
#define REG_REFLINE_MSB     TDA_MKREG(0x00, 0xA3)  /* Vertical ref line */
#define REG_REFLINE_LSB     TDA_MKREG(0x00, 0xA4)
#define REG_NPIX_MSB        TDA_MKREG(0x00, 0xA5)  /* Total pixels/line */
#define REG_NPIX_LSB        TDA_MKREG(0x00, 0xA6)
#define REG_NLINE_MSB       TDA_MKREG(0x00, 0xA7)  /* Total lines/frame */
#define REG_NLINE_LSB       TDA_MKREG(0x00, 0xA8)

/* VSYNC timing (progressive: field 2 = 0) */
#define REG_VS_LINE_STRT_1_MSB  TDA_MKREG(0x00, 0xA9)
#define REG_VS_LINE_STRT_1_LSB  TDA_MKREG(0x00, 0xAA)
#define REG_VS_PIX_STRT_1_MSB   TDA_MKREG(0x00, 0xAB)
#define REG_VS_PIX_STRT_1_LSB   TDA_MKREG(0x00, 0xAC)
#define REG_VS_LINE_END_1_MSB   TDA_MKREG(0x00, 0xAD)
#define REG_VS_LINE_END_1_LSB   TDA_MKREG(0x00, 0xAE)
#define REG_VS_PIX_END_1_MSB    TDA_MKREG(0x00, 0xAF)
#define REG_VS_PIX_END_1_LSB    TDA_MKREG(0x00, 0xB0)
#define REG_VS_LINE_STRT_2_MSB  TDA_MKREG(0x00, 0xB1)
#define REG_VS_LINE_STRT_2_LSB  TDA_MKREG(0x00, 0xB2)
#define REG_VS_PIX_STRT_2_MSB   TDA_MKREG(0x00, 0xB3)
#define REG_VS_PIX_STRT_2_LSB   TDA_MKREG(0x00, 0xB4)
#define REG_VS_LINE_END_2_MSB   TDA_MKREG(0x00, 0xB5)
#define REG_VS_LINE_END_2_LSB   TDA_MKREG(0x00, 0xB6)
#define REG_VS_PIX_END_2_MSB    TDA_MKREG(0x00, 0xB7)
#define REG_VS_PIX_END_2_LSB    TDA_MKREG(0x00, 0xB8)

/* HSYNC timing */
#define REG_HS_PIX_START_MSB    TDA_MKREG(0x00, 0xB9)
#define REG_HS_PIX_START_LSB    TDA_MKREG(0x00, 0xBA)
#define REG_HS_PIX_STOP_MSB     TDA_MKREG(0x00, 0xBB)
#define REG_HS_PIX_STOP_LSB     TDA_MKREG(0x00, 0xBC)

/* Active video window */
#define REG_VWIN_START_1_MSB    TDA_MKREG(0x00, 0xBD)
#define REG_VWIN_START_1_LSB    TDA_MKREG(0x00, 0xBE)
#define REG_VWIN_END_1_MSB      TDA_MKREG(0x00, 0xBF)
#define REG_VWIN_END_1_LSB      TDA_MKREG(0x00, 0xC0)
#define REG_VWIN_START_2_MSB    TDA_MKREG(0x00, 0xC1)
#define REG_VWIN_START_2_LSB    TDA_MKREG(0x00, 0xC2)
#define REG_VWIN_END_2_MSB      TDA_MKREG(0x00, 0xC3)
#define REG_VWIN_END_2_LSB      TDA_MKREG(0x00, 0xC4)

/* Data Enable window */
#define REG_DE_START_MSB        TDA_MKREG(0x00, 0xC5)
#define REG_DE_START_LSB        TDA_MKREG(0x00, 0xC6)
#define REG_DE_STOP_MSB         TDA_MKREG(0x00, 0xC7)
#define REG_DE_STOP_LSB         TDA_MKREG(0x00, 0xC8)

/* Timing bus generator */
#define REG_TBG_CNTRL_0         TDA_MKREG(0x00, 0xCA)
#define   TBG_CNTRL_0_TOP_TGL   (1 << 0)
#define   TBG_CNTRL_0_TOP_SEL   (1 << 1)
#define   TBG_CNTRL_0_DE_EXT    (1 << 2)
#define   TBG_CNTRL_0_TOP_EXT   (1 << 3)
#define   TBG_CNTRL_0_FRAME_DIS (1 << 5)
#define   TBG_CNTRL_0_SYNC_MTHD (1 << 6)
#define   TBG_CNTRL_0_SYNC_ONCE (1 << 7)
#define REG_TBG_CNTRL_1         TDA_MKREG(0x00, 0xCB)
#define   TBG_CNTRL_1_H_TGL     (1 << 0)           /* Invert HSYNC */
#define   TBG_CNTRL_1_V_TGL     (1 << 1)           /* Invert VSYNC */
#define   TBG_CNTRL_1_TGL_EN    (1 << 2)
#define   TBG_CNTRL_1_X_EXT     (1 << 3)           /* External sync */
#define   TBG_CNTRL_1_H_EXT     (1 << 4)
#define   TBG_CNTRL_1_V_EXT     (1 << 5)
#define   TBG_CNTRL_1_DWIN_DIS  (1 << 6)           /* Disable DE window */

/* HVF (horizontal/vertical filter) — Page 0x00 */
#define REG_HVF_CNTRL_0         TDA_MKREG(0x00, 0xE4)
#define   HVF_CNTRL_0_INTPOL(x) (((x) & 0x3) << 0)
#define   HVF_CNTRL_0_PREFIL(x) (((x) & 0x3) << 2)
#define REG_HVF_CNTRL_1         TDA_MKREG(0x00, 0xE5)
#define   HVF_CNTRL_1_FOR       (1 << 0)           /* Force output */
#define   HVF_CNTRL_1_YUVBLK    (1 << 1)
#define   HVF_CNTRL_1_VQR(x)    (((x) & 0x3) << 2)
#define   HVF_CNTRL_1_PAD(x)    (((x) & 0x3) << 4)
#define   HVF_CNTRL_1_SEMI_PLANAR (1 << 6)
#define REG_RPT_CNTRL           TDA_MKREG(0x00, 0xF0)  /* Pixel repetition */

/* TDA19988-specific registers — Page 0x00 */
#define REG_ENABLE_SPACE        TDA_MKREG(0x00, 0xD6)  /* Active space fill */

/* Page 0x11: Audio / Encoder / InfoFrame control */
#define REG_AIP_CNTRL_0         TDA_MKREG(0x11, 0x00)
#define   AIP_CNTRL_0_RST_FIFO (1 << 0)
#define REG_ENC_CNTRL           TDA_MKREG(0x11, 0x0D)
#define   ENC_CNTRL_CTL_CODE(x) (((x) & 0x3) << 2)
#define REG_DIP_IF_FLAGS        TDA_MKREG(0x11, 0x0F)  /* InfoFrame enable flags */
#define   DIP_IF_FLAGS_IF1      (1 << 1)               /* AVI InfoFrame slot */

/* Page 0x12: HDCP / OTP */
#define REG_TX33                TDA_MKREG(0x12, 0xB8)
#define   TX33_HDMI             (1 << 1)

/* ============================================================
 * HDMI core — Page 0x02 (PLL / Serializer / Analog)
 * Addresses verified against NXP BSL (tmbslTDA9989_local.h)
 * ============================================================ */
#define REG_PLL_SERIAL_1        TDA_MKREG(0x02, 0x00)
#define   PLL_SERIAL_1_SRL_FDN  (1 << 0)
#define   PLL_SERIAL_1_SRL_IZ(x) (((x) & 0x3) << 1)
#define   PLL_SERIAL_1_SRL_MAN_IZ (1 << 6)
#define REG_PLL_SERIAL_2        TDA_MKREG(0x02, 0x01)
#define   PLL_SERIAL_2_SRL_NOSC(x) (((x) & 0x3) << 0)
#define   PLL_SERIAL_2_SRL_PR(x) (((x) & 0xF) << 4)
#define REG_PLL_SERIAL_3        TDA_MKREG(0x02, 0x02)
#define   PLL_SERIAL_3_SRL_CCIR (1 << 0)
#define   PLL_SERIAL_3_SRL_DE   (1 << 2)
#define   PLL_SERIAL_3_SRL_PXIN_SEL (1 << 4)
#define REG_SERIALIZER          TDA_MKREG(0x02, 0x03)
#define REG_BUFFER_OUT          TDA_MKREG(0x02, 0x04)
#define REG_PLL_SCG1            TDA_MKREG(0x02, 0x05)
#define REG_PLL_SCG2            TDA_MKREG(0x02, 0x06)
#define REG_PLL_SCGN1           TDA_MKREG(0x02, 0x07)
#define REG_PLL_SCGN2           TDA_MKREG(0x02, 0x08)
#define REG_PLL_SCGR1           TDA_MKREG(0x02, 0x09)
#define REG_PLL_SCGR2           TDA_MKREG(0x02, 0x0A)
#define REG_AUDIO_DIV           TDA_MKREG(0x02, 0x0E)  /* Audio clock divider */
#define   AUDIO_DIV_SERCLK_8   0x03                    /* SERCLK / 8 */
#define REG_SEL_CLK             TDA_MKREG(0x02, 0x11)
#define   SEL_CLK_SEL_CLK1      (1 << 0)
#define   SEL_CLK_SEL_VRF_CLK(x) (((x) & 0x3) << 1)
#define   SEL_CLK_ENA_SC_CLK    (1 << 3)
#define REG_ANA_GENERAL         TDA_MKREG(0x02, 0x12)

/* ============================================================
 * HDMI core — Page 0x10 (InfoFrame / Packet)
 * ============================================================ */
#define REG_AVI_IF              TDA_MKREG(0x10, 0x40)  /* AVI infoframe base */
#define REG_IF2                 TDA_MKREG(0x10, 0x60)  /* InfoFrame slot 2 */
#define REG_IF3                 TDA_MKREG(0x10, 0x80)
#define REG_IF4                 TDA_MKREG(0x10, 0xA0)

/* ============================================================
 * Expected chip version
 * ============================================================ */
#define TDA19988_VERSION        0x0331

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Initialize TDA19988 HDMI transmitter for 800x600@60Hz RGB output
 *
 * Full init sequence matching QNX production driver order:
 * 1. CEC enable + soft reset
 * 2. PLL common config
 * 3. Version check + DDC/FRO setup
 * 4. Enable video/audio ports + VIP mux (dpms)
 * 5. Full video path config: timing, TBG, encoder (mode_set)
 *
 * Called after lcdc_start_raster() — TDA needs pixel clock from LCDC.
 *
 * CONTRACT:
 * - Must be called after i2c_init()
 * - I2C0 must be functional at 100kHz
 * - LCDC raster must NOT be running yet
 */
void tda19988_init(void);

#endif /* TDA19988_H */
