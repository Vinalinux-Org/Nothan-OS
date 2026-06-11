/*
 * drivers/mmc/omap-hsmmc.c - OMAP High Speed MMC Host Controller Driver
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/platform.h>
#include <nothan/printk.h>
#include <nothan/mmio.h>
#include <nothan/genhd.h>
#include <nothan/init.h>

/* Register offsets — mmc_base = module_base + 0x100 */
#define MMCHS_SYSCONFIG  0x010
#define MMCHS_SYSSTATUS  0x014
#define MMCHS_CON        0x02C
#define MMCHS_BLK        0x104
#define MMCHS_ARG        0x108
#define MMCHS_CMD        0x10C
#define MMCHS_RSP10      0x110
#define MMCHS_DATA       0x120
#define MMCHS_PSTATE     0x124
#define MMCHS_HCTL       0x128
#define MMCHS_SYSCTL     0x12C
#define MMCHS_STAT       0x130
#define MMCHS_IE         0x134
#define MMCHS_ISE        0x138
#define MMCHS_CAPA       0x140

/* STAT bits */
#define STAT_CC          (1 << 0)
#define STAT_TC          (1 << 1)
#define STAT_BWR         (1 << 4)
#define STAT_BRR         (1 << 5)
#define STAT_ERRI        (1 << 15)

/* SYSCTL bits */
#define SYSCTL_ICE       (1 << 0)
#define SYSCTL_ICS       (1 << 1)
#define SYSCTL_CEN       (1 << 2)
#define SYSCTL_SRC       (1 << 25)
#define SYSCTL_SRD       (1 << 26)

/* CON bits */
#define CON_INIT         (1 << 1)

/* PSTATE bits */
#define PSTATE_CMDFREE   (1 << 0)

/* HCTL bits */
#define HCTL_SDBP        (1 << 8)
#define HCTL_SDVS_3_3V   (7 << 9)

/* CAPA bits */
#define CAPA_VS30        (1 << 24)

/* CMD register response type and flag bits */
#define RSP_NONE         (0 << 16)
#define RSP_48           (2 << 16)
#define RSP_48_BUSY      (3 << 16)
#define RSP_136          (1 << 16)
#define CMD_CCCE         (1 << 19)
#define CMD_CICE         (1 << 20)

#define RSP_R1           (RSP_48     | CMD_CCCE | CMD_CICE)
#define RSP_R1B          (RSP_48_BUSY | CMD_CCCE | CMD_CICE)
#define RSP_R2           (RSP_136   | CMD_CCCE)
#define RSP_R3           (RSP_48)
#define RSP_R6           (RSP_48     | CMD_CCCE | CMD_CICE)
#define RSP_R7           (RSP_48     | CMD_CCCE | CMD_CICE)

#define CM_PER_MMC0_CLKCTRL    0xF0E0003C
#define MODULEMODE_ENABLE      2
#define IDLEST_MASK            (3 << 16)
#define IDLEST_FUNCTIONAL      0

#define CONF_MMC0_DAT3   (0xF0E10000 + 0x8F0)
#define CONF_MMC0_DAT2   (0xF0E10000 + 0x8F4)
#define CONF_MMC0_DAT1   (0xF0E10000 + 0x8F8)
#define CONF_MMC0_DAT0   (0xF0E10000 + 0x8FC)
#define CONF_MMC0_CLK    (0xF0E10000 + 0x900)
#define CONF_MMC0_CMD    (0xF0E10000 + 0x904)
#define PIN_MODE_0        0
#define PIN_INPUT_EN     (1 << 5)
#define PIN_PULLUP_EN    (1 << 3)

static uint32_t mmc_base;
static uint32_t card_rca;

static void mmc_delay(volatile uint32_t count)
{
	while (count--)
		;
}

static inline void mmc_write(uint32_t reg, uint32_t val)
{
	mmio_write32(mmc_base + reg, val);
}

static inline uint32_t mmc_read(uint32_t reg)
{
	return mmio_read32(mmc_base + reg);
}

/*
 * mmc_reset_cmd_line - Soft reset the MMC command line
 */
static void mmc_reset_cmd_line(void)
{
	uint32_t sysctl = mmc_read(MMCHS_SYSCTL);
	mmc_write(MMCHS_SYSCTL, sysctl | SYSCTL_SRC);
	int timeout = 100000;
	while ((mmc_read(MMCHS_SYSCTL) & SYSCTL_SRC) && --timeout)
		;
	mmc_write(MMCHS_STAT, 0xFFFFFFFF);
}

/*
 * mmc_enable_clocks - Enable PRCM clocks for MMC0
 */
static int mmc_enable_clocks(void)
{
	int timeout;
	uint32_t val;

	val = mmio_read32(CM_PER_MMC0_CLKCTRL);
	val = (val & ~3) | MODULEMODE_ENABLE;
	mmio_write32(CM_PER_MMC0_CLKCTRL, val);

	timeout = 100000;
	while ((mmio_read32(CM_PER_MMC0_CLKCTRL) & IDLEST_MASK) != IDLEST_FUNCTIONAL) {
		if (--timeout == 0) {
			printk("[MMC] PRCM clock enable timeout\n");
			return -1;
		}
	}
	return 0;
}

/*
 * mmc_config_pins - Configure pinmux for MMC0
 */
static void mmc_config_pins(void)
{
	mmio_write32(CONF_MMC0_DAT3, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_DAT2, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_DAT1, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_DAT0, PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_CLK,  PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
	mmio_write32(CONF_MMC0_CMD,  PIN_MODE_0 | PIN_INPUT_EN | PIN_PULLUP_EN);
}

static int mmc_send_cmd(uint32_t cmd, uint32_t arg, uint32_t flags)
{
	int timeout;

	mmc_write(MMCHS_STAT, 0xFFFFFFFF);

	/* Wait for command line free */
	timeout = 100000;
	while (mmc_read(MMCHS_PSTATE) & PSTATE_CMDFREE) {
		if (--timeout == 0) {
			printk("[MMC] CMD line free timeout\n");
			return -1;
		}
	}

	mmc_write(MMCHS_ARG, arg);
	mmc_write(MMCHS_CMD, (cmd << 24) | flags);

	/* Wait for command complete or error */
	timeout = 10000000;
	while (1) {
		uint32_t stat = mmc_read(MMCHS_STAT);
		if (stat & STAT_ERRI) {
			mmc_reset_cmd_line();
			return -1;
		}
		if (stat & STAT_CC)
			break;
		if (--timeout == 0) {
			mmc_reset_cmd_line();
			return -1;
		}
	}
	mmc_write(MMCHS_STAT, STAT_CC);
	return 0;
}

static int omap_hsmmc_read_block(struct gendisk *disk, u64 block, void *buf)
{
	int timeout;

	(void)disk;

	mmc_write(MMCHS_BLK, 512 | (1 << 16));

	if (mmc_send_cmd(17, (uint32_t)block, RSP_R1 | (1 << 21) | (1 << 4)) != 0) {
		printk("[MMC] Read block %u failed\n", (unsigned int)block);
		return -1;
	}

	/* Wait for Buffer Read Ready */
	timeout = 10000000;
	while (!(mmc_read(MMCHS_STAT) & STAT_BRR)) {
		if (mmc_read(MMCHS_STAT) & STAT_ERRI) {
			printk("[MMC] BRR error at block %llu\n", (unsigned long long)block);
			return -1;
		}
		if (--timeout == 0) {
			printk("[MMC] BRR timeout at block %llu\n", (unsigned long long)block);
			return -1;
		}
	}
	mmc_write(MMCHS_STAT, STAT_BRR);

	/* Read 512 bytes (128 words) */
	uint32_t *p = (uint32_t *)buf;
	for (int i = 0; i < 128; i++)
		p[i] = mmc_read(MMCHS_DATA);

	/* Wait for Transfer Complete */
	timeout = 10000000;
	while (!(mmc_read(MMCHS_STAT) & STAT_TC)) {
		if (mmc_read(MMCHS_STAT) & STAT_ERRI) {
			printk("[MMC] TC error at block %llu\n", (unsigned long long)block);
			return -1;
		}
		if (--timeout == 0) {
			printk("[MMC] TC timeout at block %llu\n", (unsigned long long)block);
			return -1;
		}
	}
	mmc_write(MMCHS_STAT, STAT_TC);

	return 0;
}

static const struct block_device_operations omap_hsmmc_ops = {
	.read_block  = omap_hsmmc_read_block,
	.write_block = NULL,
};

static struct gendisk omap_hsmmc_disk = {
	.disk_name    = "sda",
	.major        = 8,
	.first_minor  = 0,
	.fops         = &omap_hsmmc_ops,
	.private_data = NULL,
	.capacity     = 7744512, /* approx 3.7 GB in 512-byte sectors */
};

static int omap_hsmmc_probe(struct platform_device *pdev)
{
	int timeout;

	mmc_base = phys_to_mmio(pdev->base) + 0x100;
	printk("[MMC] Probing OMAP HSMMC at 0x%x\n", (unsigned int)mmc_base);

	if (mmc_enable_clocks() != 0)
		return -1;

	mmc_config_pins();

	mmc_write(MMCHS_SYSCONFIG, 0x2);
	timeout = 1000000;
	while (!(mmc_read(MMCHS_SYSSTATUS) & 0x1)) {
		if (--timeout == 0) {
			printk("[MMC] Soft reset timeout\n");
			return -1;
		}
	}

	mmc_write(MMCHS_SYSCONFIG, 0x00000308);
	mmc_write(MMCHS_CON, 0x00000000);
	mmc_write(MMCHS_SYSCTL, mmc_read(MMCHS_SYSCTL) | SYSCTL_SRC | SYSCTL_SRD);
	timeout = 10000000;
	while (mmc_read(MMCHS_SYSCTL) & (SYSCTL_SRC | SYSCTL_SRD)) {
		if (--timeout == 0) {
			printk("[MMC] CMD/DAT reset timeout\n");
			return -1;
		}
	}

	mmc_write(MMCHS_CAPA, mmc_read(MMCHS_CAPA) | CAPA_VS30);
	mmc_write(MMCHS_HCTL, HCTL_SDVS_3_3V);
	mmc_write(MMCHS_HCTL, mmc_read(MMCHS_HCTL) | HCTL_SDBP);
	timeout = 100000;
	while (!(mmc_read(MMCHS_HCTL) & HCTL_SDBP)) {
		if (--timeout == 0) {
			printk("[MMC] SDBP timeout\n");
			return -1;
		}
	}

	mmc_write(MMCHS_SYSCTL, 0x000E3C01);
	mmc_delay(1000);
	timeout = 1000000;
	while (!(mmc_read(MMCHS_SYSCTL) & SYSCTL_ICS)) {
		if (--timeout == 0) {
			printk("[MMC] ICS timeout\n");
			return -1;
		}
	}
	mmc_write(MMCHS_SYSCTL, mmc_read(MMCHS_SYSCTL) | SYSCTL_CEN);
	mmc_delay(10000);

	mmc_write(MMCHS_IE,  0xFFFFFFFF);
	mmc_write(MMCHS_ISE, 0xFFFFFFFF);
	mmc_write(MMCHS_STAT, 0xFFFFFFFF);

	/* Retry INIT stream up to 3 times — bootloader state can affect timing */
	int init_ok = 0;
	for (int attempt = 0; attempt < 3 && !init_ok; attempt++) {
		mmc_write(MMCHS_STAT, 0xFFFFFFFF);
		mmc_write(MMCHS_CON, mmc_read(MMCHS_CON) | CON_INIT);
		mmc_write(MMCHS_CMD, 0);
		mmc_delay(100000);
		timeout = 1000000;
		while (!(mmc_read(MMCHS_STAT) & STAT_CC)) {
			if (--timeout == 0)
				break;
		}
		if (mmc_read(MMCHS_STAT) & STAT_CC)
			init_ok = 1;
		mmc_write(MMCHS_CON, mmc_read(MMCHS_CON) & ~CON_INIT);
		mmc_write(MMCHS_STAT, 0xFFFFFFFF);
		if (!init_ok)
			mmc_delay(50000);
	}
	if (!init_ok) {
		printk("[MMC] INIT stream timeout\n");
		return -1;
	}

	mmc_send_cmd(0, 0, RSP_NONE);
	mmc_delay(5000);

	mmc_send_cmd(8, 0x1AA, RSP_R7);

	timeout = 2000;
	while (1) {
		mmc_send_cmd(55, 0, RSP_R1);
		if (mmc_send_cmd(41, 0x40300000, RSP_R3) == 0) {
			uint32_t ocr = mmc_read(MMCHS_RSP10);
			if (ocr & (1 << 31))
				break;
		}
		mmc_delay(10000);
		if (--timeout == 0) {
			printk("[MMC] ACMD41 timeout\n");
			return -1;
		}
	}

	if (mmc_send_cmd(2, 0, RSP_R2) != 0)
		return -1;
	if (mmc_send_cmd(3, 0, RSP_R6) != 0)
		return -1;
	card_rca = mmc_read(MMCHS_RSP10) >> 16;
	printk("[MMC] Card detected, RCA=0x%04x\n", (unsigned int)card_rca);

	if (mmc_send_cmd(7, card_rca << 16, RSP_R1B) != 0)
		return -1;
	if (mmc_send_cmd(16, 512, RSP_R1) != 0)
		return -1;

	add_disk(&omap_hsmmc_disk);
	printk("[MMC] Registered disk 'sda'\n");
	return 0;
}

static struct platform_driver omap_hsmmc_driver = {
	.drv = {
		.name = "omap_mmc",
	},
	.probe = omap_hsmmc_probe,
};

static int __init omap_hsmmc_init(void)
{
	return platform_driver_register(&omap_hsmmc_driver);
}
device_initcall(omap_hsmmc_init);
