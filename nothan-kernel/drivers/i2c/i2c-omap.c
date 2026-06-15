/*
 * drivers/i2c/i2c-omap.c - AM335x HS-I2C hardware adapter
 *
 * Implements struct i2c_adapter xfer() for I2C1 and I2C2.
 * Uses polling (no IRQ) — only called during device init sequences.
 *
 * Register map: HS-I2C IP (same as OMAP4, 16-bit registers).
 *
 * Clock: fclk = 48 MHz, Fast Mode 400 kHz.
 *   PSC  = fclk / 12 MHz - 1 = 3   → internal_clk = 12 MHz
 *   scl  = 12000 / 400 = 30 ticks
 *   SCLL = 30/2 - 7 = 8
 *   SCLH = 30/2 - 5 = 10
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/i2c.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/time.h>

/* VA mapping: L4_WKUP VA 0xF0E00000 → PA 0x44E00000
 *             L4_PER  VA 0xF0000000 → PA 0x48000000 */
#define I2C0_VA     0xF0E0B000      /* PA 0x44E0B000 */
#define I2C1_VA     0xF002A000      /* PA 0x4802A000 */
#define I2C2_VA     0xF019C000      /* PA 0x4819C000 */

/* CM_PER clock enables — base VA 0xF0E00000 */
#define CM_PER_I2C1_CLKCTRL    0xF0E00048
#define CM_PER_I2C2_CLKCTRL    0xF0E00044

/* HS-I2C register offsets (OMAP4 / AM335x map) */
#define I2C_SYSC        0x10
#define I2C_IRQSTATUS   0x28
#define I2C_SYSS        0x90
#define I2C_CNT         0x98
#define I2C_DATA        0x9C
#define I2C_CON         0xA4
#define I2C_OA          0xA8
#define I2C_SA          0xAC
#define I2C_PSC         0xB0
#define I2C_SCLL        0xB4
#define I2C_SCLH        0xB8

/* IRQSTATUS bits */
#define I2C_STAT_XRDY   (1 << 4)
#define I2C_STAT_RRDY   (1 << 3)
#define I2C_STAT_ARDY   (1 << 2)
#define I2C_STAT_NACK   (1 << 1)
#define I2C_STAT_BB     (1 << 12)

/* CON bits */
#define I2C_CON_EN      (1 << 15)
#define I2C_CON_MST     (1 << 10)
#define I2C_CON_TRX     (1 << 9)
#define I2C_CON_STP     (1 << 1)
#define I2C_CON_STT     (1 << 0)

/* SYSC / SYSS bits */
#define I2C_SYSC_SRST   (1 << 1)
#define I2C_SYSS_RDONE  (1 << 0)

/* Clock settings for 400 kHz Fast Mode (fclk = 48 MHz) */
#define I2C_PSC_400K    3
#define I2C_SCLL_400K   8
#define I2C_SCLH_400K   10

static const u32 i2c_bases[3] = { I2C0_VA, I2C1_VA, I2C2_VA };

static inline u16 reg_r(u32 base, u32 off)
{
	return (u16)mmio_read32(base + off);
}

static inline void reg_w(u32 base, u32 off, u16 val)
{
	mmio_write32(base + off, (u32)val);
}

static int wait_flag(u32 base, u16 flag, int set, unsigned long ms)
{
	unsigned long end = get_jiffies() + ms / (1000 / HZ) + 1;

	while (1) {
		u16 st = reg_r(base, I2C_IRQSTATUS);
		if (set  && (st & flag))  return 0;
		if (!set && !(st & flag)) return 0;
		if (get_jiffies() >= end) return -1;
	}
}

static int wait_bus_free(u32 base)
{
	unsigned long end = get_jiffies() + 10;  /* 100 ms */

	while (reg_r(base, I2C_IRQSTATUS) & I2C_STAT_BB) {
		if (get_jiffies() >= end) {
			printk("[I2C] bus stuck busy\n");
			return -1;
		}
	}
	return 0;
}

static void hw_init(u32 base)
{
	unsigned long end;

	reg_w(base, I2C_CON, 0);
	reg_w(base, I2C_SYSC, I2C_SYSC_SRST);

	/* Must enable module before reset-done bit becomes readable */
	reg_w(base, I2C_CON, I2C_CON_EN);
	end = get_jiffies() + 5;
	while (!(reg_r(base, I2C_SYSS) & I2C_SYSS_RDONE)) {
		if (get_jiffies() >= end) {
			printk("[I2C] reset timeout\n");
			break;
		}
	}

	reg_w(base, I2C_CON,  0);
	reg_w(base, I2C_PSC,  I2C_PSC_400K);
	reg_w(base, I2C_SCLL, I2C_SCLL_400K);
	reg_w(base, I2C_SCLH, I2C_SCLH_400K);
	reg_w(base, I2C_OA,   0x01);
	reg_w(base, I2C_CON,  I2C_CON_EN);
}

static int omap_i2c_xfer(struct i2c_adapter *adap,
			 struct i2c_msg *msgs, int num)
{
	u32 base = i2c_bases[adap->nr];

	for (int m = 0; m < num; m++) {
		struct i2c_msg *msg = &msgs[m];
		int is_read = (msg->flags & I2C_M_RD);
		u16 con;

		if (wait_bus_free(base) < 0)
			return -1;

		reg_w(base, I2C_IRQSTATUS, 0xFFFF);
		reg_w(base, I2C_SA,  msg->addr);
		reg_w(base, I2C_CNT, msg->len);

		/* STOP only on last message — allows repeated START */
		con = I2C_CON_EN | I2C_CON_MST | I2C_CON_STT;
		if (m == num - 1)
			con |= I2C_CON_STP;
		if (!is_read)
			con |= I2C_CON_TRX;
		reg_w(base, I2C_CON, con);

		if (!is_read) {
			for (int i = 0; i < msg->len; i++) {
				if (wait_flag(base, I2C_STAT_XRDY, 1, 100) < 0) {
					printk("[I2C] TX timeout byte %d\n", i);
					return -1;
				}
				reg_w(base, I2C_DATA, msg->buf[i]);
				reg_w(base, I2C_IRQSTATUS, I2C_STAT_XRDY);
			}
		} else {
			for (int i = 0; i < msg->len; i++) {
				if (wait_flag(base, I2C_STAT_RRDY, 1, 100) < 0) {
					printk("[I2C] RX timeout byte %d\n", i);
					return -1;
				}
				msg->buf[i] = (u8)reg_r(base, I2C_DATA);
				reg_w(base, I2C_IRQSTATUS, I2C_STAT_RRDY);
			}
		}

		if (wait_flag(base, I2C_STAT_ARDY, 1, 100) < 0) {
			printk("[I2C] ARDY timeout\n");
			return -1;
		}
		reg_w(base, I2C_IRQSTATUS, I2C_STAT_ARDY);

		if (reg_r(base, I2C_IRQSTATUS) & I2C_STAT_NACK) {
			printk("[I2C] NACK from 0x%02x\n", msg->addr);
			return -1;
		}
	}
	return num;
}

static struct i2c_adapter i2c1_adapter = {
	.name = "omap-i2c.1",
	.nr   = 1,
	.xfer = omap_i2c_xfer,
};

static struct i2c_adapter i2c2_adapter = {
	.name = "omap-i2c.2",
	.nr   = 2,
	.xfer = omap_i2c_xfer,
};

static int __init omap_i2c_init(void)
{
	/* Enable I2C1 and I2C2 functional clocks via CM_PER */
	mmio_write32(CM_PER_I2C1_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_I2C1_CLKCTRL) & 0x30000) != 0)
		;

	mmio_write32(CM_PER_I2C2_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_I2C2_CLKCTRL) & 0x30000) != 0)
		;

	hw_init(I2C1_VA);
	hw_init(I2C2_VA);

	printk("[I2C] I2C1/I2C2 ready at 400 kHz\n");

	i2c_add_adapter(&i2c1_adapter);
	i2c_add_adapter(&i2c2_adapter);

	return 0;
}
arch_initcall(omap_i2c_init);
