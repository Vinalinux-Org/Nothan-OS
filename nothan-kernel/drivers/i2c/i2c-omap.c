/*
 * drivers/i2c/i2c-omap.c - AM335x HS-I2C interrupt-driven adapter
 *
 * Each transfer starts the hardware, then blocks on a completion.
 * The ISR follows the Linux 2.6 omap_i2c_isr pattern:
 *   - ack everything except XRDY/RRDY upfront
 *   - check ARDY/NACK first → signal completion
 *   - then handle XRDY (write) / RRDY (read)
 *
 * IRQ numbers (AM335x TRM / am33xx.dtsi):
 *   I2C0 = 70  (L4_WKUP, PA 0x44E0B000)
 *   I2C1 = 71  (L4_PER,  PA 0x4802A000)
 *   I2C2 = 30  (L4_PER,  PA 0x4819C000)
 *
 * Clock: fclk = 48 MHz, Fast Mode 400 kHz.
 *   PSC  = fclk/12MHz - 1 = 3   → internal 12 MHz
 *   SCLL = 12000/400/2 - 7 = 8
 *   SCLH = 12000/400/2 - 5 = 10
 *
 * intc_enable_irq() must be called from main.c after do_initcalls()
 * (INTC not ready at arch_initcall time when hw_init runs).
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

extern void intc_enable_irq(unsigned int irq);

/* I2C0 pad control — re-assert after bootloader PMIC use (os-old pattern) */
#define CONF_I2C0_SDA   0xF0E10988  /* PA 0x44E10988, muxmode0 SDA */
#define CONF_I2C0_SCL   0xF0E1098C  /* PA 0x44E1098C, muxmode0 SCL */
#define PAD_I2C0_MODE   0x70        /* muxmode0 | pullup | input | slow */

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
#define I2C_WE              0x34
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
#define I2C_STAT_BB     (1 << 12)

/* CON bits */
#define I2C_CON_EN      (1 << 15)
#define I2C_CON_MST     (1 << 10)
#define I2C_CON_TRX     (1 << 9)
#define I2C_CON_STP     (1 << 1)
#define I2C_CON_STT     (1 << 0)

/* SYSC bits */
#define I2C_SYSC_SRST           (1 << 1)
#define I2C_SYSC_AUTOIDLE       (1 << 0)
#define I2C_SYSC_ENAWKUP        (1 << 2)
#define I2C_SYSC_SIDLEMODE_SMART (0x2 << 3)
#define I2C_SYSC_CLOCKACT_FCLK  (0x2 << 8)

#define I2C_SYSS_RDONE  (1 << 0)
#define I2C_WE_ALL      0x636FU
#define I2C_BUF_RXFIF_CLR  (1 << 14)
#define I2C_BUF_TXFIF_CLR  (1 << 6)

/* Clock settings: 100 kHz — matching os-old (proven on this hardware) */
#define I2C_PSC_400K    3
#define I2C_SCLL_400K   53
#define I2C_SCLH_400K   55

/* Bus-free poll count (~20 ms at 1 GHz) */
#define I2C_BUS_TIMEOUT 20000000U

/* Per-adapter transfer state */
struct omap_i2c_dev {
	int           nr;           /* bus number 0-2 */
	u32           base;         /* register VA */
	unsigned int  irq;          /* INTC IRQ number */
	struct completion done;     /* signalled by ISR on completion */
	int           err;          /* 0=ok, -1=NACK/error */
	u8           *buf;          /* pointer into current msg buffer */
	int           buf_len;      /* remaining bytes */
	int           is_read;
};

static struct omap_i2c_dev devs[3];

/* Map IRQ → dev for the ISR */
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
/* ISR — Linux 2.6 omap_i2c_isr pattern                               */
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
	 * Read raw (unmasked) status. IRQSTATUS (0x28) shows only events that
	 * are enabled in IRQENABLE_SET; if IRQENABLE_SET is not working, it
	 * always reads 0 and the ISR loops forever doing nothing.
	 * IRQSTATUS_RAW (0x24) always reflects hardware state — use it to
	 * detect events, then clear via IRQSTATUS (W1C).
	 */
	stat = reg_r(base, I2C_IRQSTATUS_RAW);

	if (!stat)
		return;

	/* Clear all events except XRDY/RRDY upfront (those are cleared after data xfer). */
	reg_w(base, I2C_IRQSTATUS, stat & ~(I2C_STAT_XRDY | I2C_STAT_RRDY));

	/* NACK or arbitration lost */
	if (stat & (I2C_STAT_NACK | I2C_STAT_AL)) {
		if (stat & I2C_STAT_NACK)
			reg_w(base, I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
		dev->err = -1;
	}

	/* ARDY (or NACK which also ends the transfer) → complete */
	if (stat & (I2C_STAT_ARDY | I2C_STAT_NACK | I2C_STAT_AL)) {
		reg_w(base, I2C_IRQSTATUS, I2C_STAT_ARDY); /* ProDB0017052: clear twice */
		complete(&dev->done);
		return;
	}

	/* XRDY: TX FIFO ready — write next byte */
	if (stat & I2C_STAT_XRDY) {
		if (dev->buf_len > 0) {
			reg_w(base, I2C_DATA, *dev->buf++);
			dev->buf_len--;
		} else {
			/* CNT mismatch: hardware wants more data than we have */
			reg_w(base, I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
			dev->err = -1;
			complete(&dev->done);
			return;
		}
		reg_w(base, I2C_IRQSTATUS, I2C_STAT_XRDY);
	}

	/* RRDY: RX data ready — read next byte */
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

/* Poll IRQSTATUS_RAW until mask bit (or NACK/AL) appears. */
static u16 i2c_poll_status(u32 base, u16 mask)
{
	unsigned int timeout = I2C_BUS_TIMEOUT * 10;
	u16 stat;

	while (timeout--) {
		stat = reg_r(base, I2C_IRQSTATUS_RAW);
		if (stat & (mask | I2C_STAT_NACK | I2C_STAT_AL))
			return stat;
	}
	return 0;
}

static int omap_i2c_xfer(struct i2c_adapter *adap,
			  struct i2c_msg *msgs, int num)
{
	u32 base = devs[adap->nr].base;

	for (int m = 0; m < num; m++) {
		struct i2c_msg *msg = &msgs[m];
		bool is_read = !!(msg->flags & I2C_M_RD);
		bool last    = (m == num - 1);
		u16 stat;
		u16 con;

		if (wait_bus_free(base) < 0)
			return -1;

		reg_w(base, I2C_IRQSTATUS, 0x7FFF);
		reg_w(base, I2C_SA,  msg->addr);
		reg_w(base, I2C_CNT, (u16)msg->len);

		con = I2C_CON_EN | I2C_CON_MST | I2C_CON_STT;
		if (last && !is_read)
			con |= I2C_CON_STP;
		if (!is_read)
			con |= I2C_CON_TRX;
		reg_w(base, I2C_CON, con);

		for (int i = 0; i < msg->len; i++) {
			if (!is_read) {
				stat = i2c_poll_status(base, I2C_STAT_XRDY);
				if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL))) {
					printk("[I2C%d] NACK/timeout WR addr=0x%02x byte=%d stat=0x%04x\n",
					       adap->nr, (unsigned)msg->addr, i, (unsigned)stat);
					i2c_flush(base);
					return -1;
				}
				reg_w(base, I2C_DATA, msg->buf[i]);
				reg_w(base, I2C_IRQSTATUS, I2C_STAT_XRDY);
			} else {
				stat = i2c_poll_status(base, I2C_STAT_RRDY);
				if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL))) {
					printk("[I2C%d] NACK/timeout RD addr=0x%02x byte=%d stat=0x%04x\n",
					       adap->nr, (unsigned)msg->addr, i, (unsigned)stat);
					i2c_flush(base);
					return -1;
				}
				msg->buf[i] = (u8)reg_r(base, I2C_DATA);
				reg_w(base, I2C_IRQSTATUS, I2C_STAT_RRDY);
			}
		}

		/* For read last message, send STOP after data */
		if (last && is_read) {
			u16 c = reg_r(base, I2C_CON);
			reg_w(base, I2C_CON, c | I2C_CON_STP);
		}

		stat = i2c_poll_status(base, I2C_STAT_ARDY);
		if (!stat) {
			printk("[I2C%d] ARDY timeout addr=0x%02x raw=0x%04x\n",
			       adap->nr, (unsigned)msg->addr,
			       (unsigned)reg_r(base, I2C_IRQSTATUS_RAW));
			i2c_flush(base);
			return -1;
		}
		reg_w(base, I2C_IRQSTATUS, I2C_STAT_ARDY);
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

	/* I2C0 only: re-assert pinmux — bootloader may have left it muxed for PMIC */
	if (dev->nr == 0) {
		mmio_write32(CONF_I2C0_SDA, PAD_I2C0_MODE);
		mmio_write32(CONF_I2C0_SCL, PAD_I2C0_MODE);
		printk("[I2C0] pinmux re-asserted (SDA=0x%x SCL=0x%x)\n",
		       (unsigned)mmio_read32(CONF_I2C0_SDA),
		       (unsigned)mmio_read32(CONF_I2C0_SCL));
	}

	reg_w(base, I2C_CON, 0);
	reg_w(base, I2C_SYSC, I2C_SYSC_SRST);

	reg_w(base, I2C_CON, I2C_CON_EN);
	timeout = 100000;
	while (!(reg_r(base, I2C_SYSS) & I2C_SYSS_RDONE) && timeout--)
		;
	if (!timeout)
		printk("[I2C%d] WARNING: SYSS reset timeout\n", dev->nr);

	reg_w(base, I2C_CON,  0);
	reg_w(base, I2C_PSC,  I2C_PSC_400K);
	reg_w(base, I2C_SCLL, I2C_SCLL_400K);
	reg_w(base, I2C_SCLH, I2C_SCLH_400K);
	reg_w(base, I2C_OA,   0x01);

	reg_w(base, I2C_IRQSTATUS, 0x7FFF);

	/* Polling mode — explicitly clear all IRQ enables, like os-old. */
	reg_w(base, I2C_IRQENABLE_CLR, 0x7FFF);

	reg_w(base, I2C_CON, I2C_CON_EN);
}

static struct i2c_adapter i2c0_adapter = { .name = "omap-i2c.0", .nr = 0, .xfer = omap_i2c_xfer };
static struct i2c_adapter i2c1_adapter = { .name = "omap-i2c.1", .nr = 1, .xfer = omap_i2c_xfer };
static struct i2c_adapter i2c2_adapter = { .name = "omap-i2c.2", .nr = 2, .xfer = omap_i2c_xfer };

static int __init omap_i2c_init(void)
{
	/* Enable clocks */
	printk("[I2C] enabling clocks\n");
	mmio_write32(CM_WKUP_I2C0_CLKCTRL, 0x02);
	while ((mmio_read32(CM_WKUP_I2C0_CLKCTRL) & 0x30000) != 0)
		;
	printk("[I2C] I2C0 clock OK (CLKCTRL=0x%08x)\n",
	       (unsigned)mmio_read32(CM_WKUP_I2C0_CLKCTRL));
	mmio_write32(CM_PER_I2C1_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_I2C1_CLKCTRL) & 0x30000) != 0)
		;
	printk("[I2C] I2C1 clock OK\n");
	mmio_write32(CM_PER_I2C2_CLKCTRL, 0x02);
	while ((mmio_read32(CM_PER_I2C2_CLKCTRL) & 0x30000) != 0)
		;
	printk("[I2C] I2C2 clock OK\n");

	devs[0].nr = 0; devs[0].base = I2C0_VA;
	devs[1].nr = 1; devs[1].base = I2C1_VA;
	devs[2].nr = 2; devs[2].base = I2C2_VA;

	printk("[I2C] hw_init I2C0\n");
	hw_init(&devs[0]);
	printk("[I2C] hw_init I2C1\n");
	hw_init(&devs[1]);
	printk("[I2C] hw_init I2C2\n");
	hw_init(&devs[2]);

	printk("[I2C] I2C0/I2C1/I2C2 ready at 400 kHz\n");

	i2c_add_adapter(&i2c0_adapter);
	i2c_add_adapter(&i2c1_adapter);
	i2c_add_adapter(&i2c2_adapter);

	return 0;
}
arch_initcall(omap_i2c_init);
