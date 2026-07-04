/*
 * drivers/i2c/i2c-omap.c - AM335x HS-I2C interrupt-driven adapter
 *
 * Each transfer starts the hardware, then blocks on a completion.
 * The ISR (hard IRQ) handles XRDY/RRDY/ARDY/NACK and signals done.
 *
 * Two wait paths inside wait_for_completion():
 *   boot context  (sched_running=false): spin-wait — ISR fires, sets done, spin exits
 *   task context  (sched_running=true):  block — ISR wakes the waiting task
 *
 * IRQ numbers (AM335x TRM):
 *   I2C0 = 70  (L4_WKUP, PA 0x44E0B000)
 *   I2C1 = 71  (L4_PER,  PA 0x4802A000)
 *   I2C2 = 30  (L4_PER,  PA 0x4819C000)
 *
 * Clock: fclk = 48 MHz, Standard Mode 100 kHz.
 *   PSC  = fclk/12MHz - 1 = 3    → internal 12 MHz
 *   SCLL = 12000/100/2 - 7 = 53
 *   SCLH = 12000/100/2 - 5 = 55
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/i2c.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/irq.h>
#include <nothan/delay.h>
#include <nothan/completion.h>

extern void intc_enable_irq(unsigned int irq);

/* I2C0 pad control — re-assert after bootloader PMIC use */
#define CONF_I2C0_SDA   0xF0E10988  /* PA 0x44E10988 */
#define CONF_I2C0_SCL   0xF0E1098C  /* PA 0x44E1098C */
#define PAD_I2C0_MODE   0x70        /* muxmode0 | pullup | input | slow slew */

/* VA mapping */
#define I2C0_VA     0xF0E0B000      /* PA 0x44E0B000 */
#define I2C1_VA     0xF002A000      /* PA 0x4802A000 */
#define I2C2_VA     0xF019C000      /* PA 0x4819C000 */

/* IRQ numbers */
#define I2C0_IRQ    70
#define I2C1_IRQ    71
#define I2C2_IRQ    30

/* CM clock enables */
#define CM_WKUP_I2C0_CLKCTRL   0xF0E004B8
#define CM_PER_I2C1_CLKCTRL    0xF0E00048
#define CM_PER_I2C2_CLKCTRL    0xF0E00044

/* Register offsets */
#define I2C_SYSC            0x10
#define I2C_IRQSTATUS_RAW   0x24
#define I2C_IRQSTATUS       0x28
#define I2C_IRQENABLE_SET   0x2C
#define I2C_IRQENABLE_CLR   0x30
#define I2C_SYSS            0x90
#define I2C_BUF             0x94
#define I2C_CNT             0x98
#define I2C_DATA            0x9C
#define I2C_CON             0xA4
#define I2C_OA              0xA8
#define I2C_SA              0xAC
#define I2C_PSC             0xB0
#define I2C_SCLL            0xB4
#define I2C_SCLH            0xB8

/* IRQSTATUS bits */
#define I2C_STAT_AL     (1 << 0)
#define I2C_STAT_NACK   (1 << 1)
#define I2C_STAT_ARDY   (1 << 2)
#define I2C_STAT_RRDY   (1 << 3)
#define I2C_STAT_XRDY   (1 << 4)
#define I2C_STAT_XUDF   (1 << 10)  /* TX underflow — clear and abort */
#define I2C_STAT_BB     (1 << 12)

/* CON bits */
#define I2C_CON_EN      (1 << 15)
#define I2C_CON_MST     (1 << 10)
#define I2C_CON_TRX     (1 << 9)
#define I2C_CON_STP     (1 << 1)
#define I2C_CON_STT     (1 << 0)

/* SYSC bits */
#define I2C_SYSC_SRST   (1 << 1)

#define I2C_SYSS_RDONE      (1 << 0)
#define I2C_BUF_RXFIF_CLR   (1 << 14)
#define I2C_BUF_TXFIF_CLR   (1 << 6)

/* Clock values: 100 kHz, fclk=48 MHz → internal 12 MHz (PSC=3) */
#define I2C_PSC_VAL     3
#define I2C_SCLL_VAL    53
#define I2C_SCLH_VAL    55

/* Bus-free poll timeout (~20 ms at 1 GHz) */
#define I2C_BUS_TIMEOUT 20000000U

/* Per-adapter transfer state */
struct omap_i2c_dev {
	int           nr;
	u32           base;
	unsigned int  irq;
	struct completion done;
	int           err;
	u8           *buf;
	int           buf_len;
	int           is_read;
};

static struct omap_i2c_dev devs[3];
static struct omap_i2c_dev *irq_to_dev[128];

static inline u16 reg_r(u32 base, u32 off)
{
	return (u16)mmio_read32(base + off);
}

static inline void reg_w(u32 base, u32 off, u16 val)
{
	mmio_write32(base + off, (u32)val);
}

/* ------------------------------------------------------------------ */
/* ISR                                                                  */
/* ------------------------------------------------------------------ */

static void omap_i2c_isr(unsigned int irq)
{
	struct omap_i2c_dev *dev = irq_to_dev[irq];
	u32 base;
	u16 stat;

	if (!dev)
		return;

	base = dev->base;

	/*
	 * Read raw status — IRQSTATUS (0x28) only shows events enabled in
	 * IRQENABLE_SET. On this hardware rev (0x000b) IRQENABLE_SET does
	 * not always propagate correctly, so read RAW and clear via IRQSTATUS.
	 */
	stat = reg_r(base, I2C_IRQSTATUS_RAW);
	if (!stat)
		return;

	reg_w(base, I2C_IRQSTATUS, stat & ~(I2C_STAT_XRDY | I2C_STAT_RRDY));

	if (stat & I2C_STAT_XUDF) {
		reg_w(base, I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
		dev->err = -1;
		complete(&dev->done);
		return;
	}

	if (stat & (I2C_STAT_NACK | I2C_STAT_AL)) {
		if (stat & I2C_STAT_NACK)
			reg_w(base, I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
		dev->err = -1;
	}

	if (stat & (I2C_STAT_ARDY | I2C_STAT_NACK | I2C_STAT_AL)) {
		reg_w(base, I2C_IRQSTATUS, I2C_STAT_ARDY); /* ProDB0017052: clear twice */
		complete(&dev->done);
		return;
	}

	if (stat & I2C_STAT_XRDY) {
		if (dev->buf_len > 0) {
			reg_w(base, I2C_DATA, *dev->buf++);
			dev->buf_len--;
		} else {
			reg_w(base, I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
			dev->err = -1;
			complete(&dev->done);
			return;
		}
		reg_w(base, I2C_IRQSTATUS, I2C_STAT_XRDY);
	}

	if (stat & I2C_STAT_RRDY) {
		if (dev->buf_len > 0) {
			*dev->buf++ = (u8)reg_r(base, I2C_DATA);
			dev->buf_len--;
		}
		reg_w(base, I2C_IRQSTATUS, I2C_STAT_RRDY);
	}
}

/* ------------------------------------------------------------------ */
/* Transfer                                                             */
/* ------------------------------------------------------------------ */

static void i2c_flush(u32 base)
{
	reg_w(base, I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
	udelay(50);
	reg_w(base, I2C_CON, 0);
	udelay(10);
	reg_w(base, I2C_CON, I2C_CON_EN);
	udelay(10);
	reg_w(base, I2C_IRQSTATUS, 0x7FFF);
}

static int wait_bus_free(u32 base)
{
	unsigned int count = I2C_BUS_TIMEOUT;

	while (reg_r(base, I2C_IRQSTATUS_RAW) & I2C_STAT_BB) {
		if (!count--) {
			printk("[I2C] bus stuck busy, flushing\n");
			i2c_flush(base);
			return 0;
		}
	}
	return 0;
}

static int omap_i2c_xfer(struct i2c_adapter *adap,
			  struct i2c_msg *msgs, int num)
{
	struct omap_i2c_dev *dev = &devs[adap->nr];
	u32 base = dev->base;

	for (int m = 0; m < num; m++) {
		struct i2c_msg *msg = &msgs[m];
		u16 con;

		if (wait_bus_free(base) < 0)
			return -1;

		reg_w(base, I2C_IRQSTATUS, 0x7FFF);
		reg_w(base, I2C_BUF,
		      reg_r(base, I2C_BUF) | I2C_BUF_RXFIF_CLR | I2C_BUF_TXFIF_CLR);

		dev->buf     = msg->buf;
		dev->buf_len = msg->len;
		dev->is_read = !!(msg->flags & I2C_M_RD);
		dev->err     = 0;
		init_completion(&dev->done);

		reg_w(base, I2C_SA,  msg->addr);
		reg_w(base, I2C_CNT, (u16)msg->len);

		con = I2C_CON_EN | I2C_CON_MST | I2C_CON_STT;
		if (m == num - 1)
			con |= I2C_CON_STP;
		if (!dev->is_read)
			con |= I2C_CON_TRX;
		reg_w(base, I2C_CON, con);

		wait_for_completion(&dev->done);

		if (dev->err) {
			printk("[I2C%d] error addr=0x%02x\n",
			       adap->nr, (unsigned)msg->addr);
			i2c_flush(base);
			return -1;
		}
	}
	return num;
}

/* ------------------------------------------------------------------ */
/* Hardware init                                                        */
/* ------------------------------------------------------------------ */

static void hw_init(struct omap_i2c_dev *dev)
{
	u32 base = dev->base;
	unsigned int timeout;

	/* I2C0: bootloader leaves pins muxed for PMIC after power-on sequence */
	if (dev->nr == 0) {
		mmio_write32(CONF_I2C0_SDA, PAD_I2C0_MODE);
		mmio_write32(CONF_I2C0_SCL, PAD_I2C0_MODE);
	}

	reg_w(base, I2C_CON, 0);
	reg_w(base, I2C_SYSC, I2C_SYSC_SRST);

	reg_w(base, I2C_CON, I2C_CON_EN);
	timeout = 100000;
	while (!(reg_r(base, I2C_SYSS) & I2C_SYSS_RDONE) && timeout--)
		;
	if (!timeout)
		printk("[I2C%d] WARNING: reset timeout\n", dev->nr);

	reg_w(base, I2C_CON,  0);
	reg_w(base, I2C_PSC,  I2C_PSC_VAL);
	reg_w(base, I2C_SCLL, I2C_SCLL_VAL);
	reg_w(base, I2C_SCLH, I2C_SCLH_VAL);
	reg_w(base, I2C_OA,   0x01);

	reg_w(base, I2C_IRQSTATUS, 0x7FFF);
	reg_w(base, I2C_IRQENABLE_SET,
	      I2C_STAT_XRDY | I2C_STAT_RRDY |
	      I2C_STAT_ARDY | I2C_STAT_NACK | I2C_STAT_AL);

	reg_w(base, I2C_CON, I2C_CON_EN);

	/*
	 * Pre-init completion so any stale IRQ firing after intc_enable_irq()
	 * sees a valid list head in complete() instead of next=NULL.
	 */
	init_completion(&dev->done);

	request_irq(dev->irq, omap_i2c_isr);
	intc_enable_irq(dev->irq);
}

static struct i2c_adapter i2c0_adapter = { .name = "omap-i2c.0", .nr = 0, .xfer = omap_i2c_xfer };
static struct i2c_adapter i2c1_adapter = { .name = "omap-i2c.1", .nr = 1, .xfer = omap_i2c_xfer };
static struct i2c_adapter i2c2_adapter = { .name = "omap-i2c.2", .nr = 2, .xfer = omap_i2c_xfer };

static int __init omap_i2c_init(void)
{
	mmio_write32(CM_WKUP_I2C0_CLKCTRL, 0x02);
	while ((mmio_read32(CM_WKUP_I2C0_CLKCTRL) & 0x30000) != 0)
		;
	mmio_write32(CM_PER_I2C1_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_I2C1_CLKCTRL) & 0x30000) != 0)
		;
	mmio_write32(CM_PER_I2C2_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_I2C2_CLKCTRL) & 0x30000) != 0)
		;

	devs[0].nr = 0; devs[0].base = I2C0_VA; devs[0].irq = I2C0_IRQ;
	devs[1].nr = 1; devs[1].base = I2C1_VA; devs[1].irq = I2C1_IRQ;
	devs[2].nr = 2; devs[2].base = I2C2_VA; devs[2].irq = I2C2_IRQ;

	irq_to_dev[I2C0_IRQ] = &devs[0];
	irq_to_dev[I2C1_IRQ] = &devs[1];
	irq_to_dev[I2C2_IRQ] = &devs[2];

	printk("[I2C] hw_init I2C0\n");
	hw_init(&devs[0]);
	printk("[I2C] hw_init I2C1\n");
	hw_init(&devs[1]);
	printk("[I2C] hw_init I2C2\n");
	hw_init(&devs[2]);

	printk("[I2C] I2C0/I2C1/I2C2 ready at 100 kHz\n");

	i2c_add_adapter(&i2c0_adapter);
	i2c_add_adapter(&i2c1_adapter);
	i2c_add_adapter(&i2c2_adapter);

	return 0;
}
arch_initcall(omap_i2c_init);
