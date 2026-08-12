#ifndef AM335X_I2C_H
#define AM335X_I2C_H

#include "types.h"

/*
 * I2C0 — the only I2C the bootloader needs, because the TPS65217 PMIC hangs
 * off it and VDD_MPU has to be raised before the MPU DPLL can go past OPP100.
 *
 * Addresses verified against reference/am335x/:
 *   I2C0 registers      PA 0x44E0B000   (Ch02 memory map)
 *   CM_WKUP_I2C0_CLKCTRL CM_WKUP +0xB8  (Ch08 PRCM)
 *   pad conf_i2c0_sda/scl 0x44E10988/8C (Ch09 control module)
 */
#define I2C0_BASE		0x44E0B000
#define CM_WKUP_I2C0_CLKCTRL	0x44E004B8
#define CONF_I2C0_SDA		0x44E10988
#define CONF_I2C0_SCL		0x44E1098C
#define PAD_I2C0_MODE		0x70	/* mode0 | pullup | input enable | slow slew */

/* Register offsets — TRM Ch21 */
#define I2C_SYSC		0x10
#define I2C_IRQSTATUS_RAW	0x24
#define I2C_IRQSTATUS		0x28
#define I2C_IRQENABLE_CLR	0x30
#define I2C_SYSS		0x90
#define I2C_BUF			0x94
#define I2C_CNT			0x98
#define I2C_DATA		0x9C
#define I2C_CON			0xA4
#define I2C_OA			0xA8
#define I2C_SA			0xAC
#define I2C_PSC			0xB0
#define I2C_SCLL		0xB4
#define I2C_SCLH		0xB8

/* I2C_IRQSTATUS_RAW bits */
#define I2C_STAT_AL		(1 << 0)
#define I2C_STAT_NACK		(1 << 1)
#define I2C_STAT_ARDY		(1 << 2)
#define I2C_STAT_RRDY		(1 << 3)
#define I2C_STAT_XRDY		(1 << 4)
#define I2C_STAT_BB		(1 << 12)

/* I2C_CON bits */
#define I2C_CON_EN		(1 << 15)
#define I2C_CON_MST		(1 << 10)
#define I2C_CON_TRX		(1 << 9)
#define I2C_CON_STP		(1 << 1)
#define I2C_CON_STT		(1 << 0)

#define I2C_SYSC_SRST		(1 << 1)
#define I2C_SYSS_RDONE		(1 << 0)

/*
 * Functional clock is 48 MHz (PER_CLKOUTM2/4, TRM Ch08), so the same divider
 * values the kernel driver already runs this bus at:
 *   PSC  = 48/12 - 1        = 3   -> 12 MHz internal
 *   SCLL = 12000/100/2 - 7  = 53  -> 100 kHz
 *   SCLH = 12000/100/2 - 5  = 55
 */
#define I2C_PSC_VAL		3
#define I2C_SCLL_VAL		53
#define I2C_SCLH_VAL		55

void i2c0_init(void);
int  i2c0_write(uint8_t addr, const uint8_t *buf, int len);
int  i2c0_read_reg(uint8_t addr, uint8_t reg, uint8_t *val);

#endif /* AM335X_I2C_H */
