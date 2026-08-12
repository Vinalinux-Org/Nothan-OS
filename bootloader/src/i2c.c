/*
 * bootloader/src/i2c.c - Polled I2C0 master
 *
 * Just enough I2C to talk to the TPS65217 PMIC before the MPU DPLL is raised.
 * Polled, not interrupt driven: there is no interrupt controller set up at this
 * point in boot, and the whole conversation is a handful of bytes.
 *
 * Every wait loop is bounded.  A bootloader that hangs on a bus glitch is worse
 * than one that gives up and reports it — the caller can still boot at the safe
 * operating point.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "am335x.h"
#include "am335x_i2c.h"
#include "boot.h"

#define I2C_TIMEOUT	100000

static inline uint32_t i2c_r(uint32_t off)
{
	return readl(I2C0_BASE + off);
}

static inline void i2c_w(uint32_t off, uint32_t val)
{
	writel(val, I2C0_BASE + off);
}

void i2c0_init(void)
{
	int timeout;

	/* Module clock first — the reset-done status below never asserts
	 * without it (TRM Ch21: "the module must receive all its clocks
	 * before it can grant a reset completed status"). */
	writel(MODULE_ENABLE, CM_WKUP_I2C0_CLKCTRL);
	timeout = I2C_TIMEOUT;
	while ((readl(CM_WKUP_I2C0_CLKCTRL) & 0x30000) != 0 && --timeout)
		;

	writel(PAD_I2C0_MODE, CONF_I2C0_SDA);
	writel(PAD_I2C0_MODE, CONF_I2C0_SCL);

	i2c_w(I2C_CON, 0);
	i2c_w(I2C_SYSC, I2C_SYSC_SRST);

	/* Reset only progresses while the module is enabled. */
	i2c_w(I2C_CON, I2C_CON_EN);
	timeout = I2C_TIMEOUT;
	while (!(i2c_r(I2C_SYSS) & I2C_SYSS_RDONE) && --timeout)
		;

	i2c_w(I2C_CON, 0);
	i2c_w(I2C_PSC, I2C_PSC_VAL);
	i2c_w(I2C_SCLL, I2C_SCLL_VAL);
	i2c_w(I2C_SCLH, I2C_SCLH_VAL);
	i2c_w(I2C_OA, 0x01);

	i2c_w(I2C_IRQENABLE_CLR, 0x7FFF);	/* polled only — no interrupts */
	i2c_w(I2C_IRQSTATUS, 0x7FFF);

	i2c_w(I2C_CON, I2C_CON_EN);
}

/* Wait until any of @mask is raised.  Returns the status, or 0 on timeout. */
static uint32_t wait_status(uint32_t mask)
{
	int timeout = I2C_TIMEOUT;
	uint32_t stat;

	do {
		stat = i2c_r(I2C_IRQSTATUS_RAW);
		if (stat & mask)
			return stat;
	} while (--timeout);

	return 0;
}

static int wait_bus_free(void)
{
	int timeout = I2C_TIMEOUT;

	while ((i2c_r(I2C_IRQSTATUS_RAW) & I2C_STAT_BB) && --timeout)
		;

	return timeout ? 0 : -1;
}

/*
 * Send @len bytes to @addr as one transfer (start ... stop).
 * For the PMIC that is always {register, value}.
 */
int i2c0_write(uint8_t addr, const uint8_t *buf, int len)
{
	uint32_t stat;
	int i;

	if (wait_bus_free())
		return -1;

	i2c_w(I2C_IRQSTATUS, 0x7FFF);
	i2c_w(I2C_SA, addr);
	i2c_w(I2C_CNT, len);
	i2c_w(I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX |
			I2C_CON_STT | I2C_CON_STP);

	for (i = 0; i < len; i++) {
		stat = wait_status(I2C_STAT_XRDY | I2C_STAT_NACK | I2C_STAT_AL);
		if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL)))
			goto fail;

		i2c_w(I2C_DATA, buf[i]);
		i2c_w(I2C_IRQSTATUS, I2C_STAT_XRDY);
	}

	stat = wait_status(I2C_STAT_ARDY | I2C_STAT_NACK | I2C_STAT_AL);
	if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL)))
		goto fail;

	i2c_w(I2C_IRQSTATUS, 0x7FFF);
	return 0;

fail:
	/* Force a stop so the bus is not left held for whoever runs next. */
	i2c_w(I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
	i2c_w(I2C_IRQSTATUS, 0x7FFF);
	return -1;
}

/*
 * Read one register: write the index without a stop, then restart into a
 * one-byte read.  A stop between the two phases would let the PMIC forget
 * which register was selected.
 */
int i2c0_read_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
	uint32_t stat;

	if (wait_bus_free())
		return -1;

	/* Phase 1 — register index, no stop. */
	i2c_w(I2C_IRQSTATUS, 0x7FFF);
	i2c_w(I2C_SA, addr);
	i2c_w(I2C_CNT, 1);
	i2c_w(I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX | I2C_CON_STT);

	stat = wait_status(I2C_STAT_XRDY | I2C_STAT_NACK | I2C_STAT_AL);
	if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL)))
		goto fail;

	i2c_w(I2C_DATA, reg);
	i2c_w(I2C_IRQSTATUS, I2C_STAT_XRDY);

	stat = wait_status(I2C_STAT_ARDY | I2C_STAT_NACK | I2C_STAT_AL);
	if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL)))
		goto fail;
	i2c_w(I2C_IRQSTATUS, I2C_STAT_ARDY);

	/* Phase 2 — restart, receive one byte, stop. */
	i2c_w(I2C_CNT, 1);
	i2c_w(I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP);

	stat = wait_status(I2C_STAT_RRDY | I2C_STAT_NACK | I2C_STAT_AL);
	if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL)))
		goto fail;

	*val = (uint8_t)i2c_r(I2C_DATA);
	i2c_w(I2C_IRQSTATUS, I2C_STAT_RRDY);

	stat = wait_status(I2C_STAT_ARDY | I2C_STAT_NACK | I2C_STAT_AL);
	if (!stat || (stat & (I2C_STAT_NACK | I2C_STAT_AL)))
		goto fail;

	i2c_w(I2C_IRQSTATUS, 0x7FFF);
	return 0;

fail:
	i2c_w(I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
	i2c_w(I2C_IRQSTATUS, 0x7FFF);
	return -1;
}
