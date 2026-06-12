/*
 * arch/arm/mach-omap2/board-bbb.c - BeagleBone Black platform devices
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/platform.h>
#include <nothan/pinctrl.h>
#include <nothan/init.h>

/* L4_PER peripherals */
#define L4_PER_BASE		0x48000000
#define DMTIMER2_BASE		(L4_PER_BASE + 0x40000)	/* 0x48040000 */
#define INTC_BASE		0x48200000

/* L4_WKUP peripherals */
#define L4_WKUP_BASE		0x44E00000
#define UART0_BASE		(L4_WKUP_BASE + 0x9000)		/* 0x44E09000 */
#define MMC0_BASE		(L4_PER_BASE + 0x60000)		/* 0x48060000 */

/* GPIO banks */
#define GPIO0_BASE		(L4_WKUP_BASE + 0x7000)		/* 0x44E07000 */
#define GPIO1_BASE		(L4_PER_BASE  + 0x4C000)	/* 0x4804C000 */
#define GPIO2_BASE		0x481AC000
#define GPIO3_BASE		0x481AE000

/* Control Module VA: PA 0x44E10000 → VA 0xF0E10000 (via L4_WKUP mapping) */
#define CM_BASE_VA		0xF0E10000
#define CM_REG(offset)		(CM_BASE_VA + (offset))

static const struct pin_desc uart0_pins[] = {
	{ CM_REG(0x970), PIN_INPUT_PULLUP  | PIN_MUXMODE(0) },	/* uart0_rxd */
	{ CM_REG(0x974), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* uart0_txd */
};

static const struct pin_desc mmc0_pins[] = {
	{ CM_REG(0x8F0), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* mmc0_dat3 */
	{ CM_REG(0x8F4), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* mmc0_dat2 */
	{ CM_REG(0x8F8), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* mmc0_dat1 */
	{ CM_REG(0x8FC), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* mmc0_dat0 */
	{ CM_REG(0x900), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* mmc0_clk  */
	{ CM_REG(0x904), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* mmc0_cmd  */
};

static const struct pin_desc i2c0_pins[] = {
	{ CM_REG(0x988), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* i2c0_sda */
	{ CM_REG(0x98C), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* i2c0_scl */
};

static const struct pin_desc spi0_pins[] = {
	{ CM_REG(0x950), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* spi0_sclk */
	{ CM_REG(0x954), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* spi0_d0   */
	{ CM_REG(0x958), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* spi0_d1   */
	{ CM_REG(0x95C), PIN_INPUT_PULLUP | PIN_MUXMODE(0) },	/* spi0_cs0  */
};

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))

static const struct pin_group bbb_pin_groups[] = {
	{ "uart0", uart0_pins, ARRAY_SIZE(uart0_pins) },
	{ "mmc0",  mmc0_pins,  ARRAY_SIZE(mmc0_pins)  },
	{ "i2c0",  i2c0_pins,  ARRAY_SIZE(i2c0_pins)  },
	{ "spi0",  spi0_pins,  ARRAY_SIZE(spi0_pins)  },
};

static struct platform_device bbb_devices[] = {
	{ .name = "omap_intc",  .base = INTC_BASE,    .irq = 0  },
	{ .name = "omap_timer", .base = DMTIMER2_BASE, .irq = 68 },
	{ .name = "omap_uart",  .base = UART0_BASE,   .irq = 72 },
	{ .name = "omap_mmc",   .base = MMC0_BASE,    .irq = 64 },
	{ .name = "omap_gpio",  .base = GPIO0_BASE,   .irq = 0  },
	{ .name = "omap_gpio",  .base = GPIO1_BASE,   .irq = 0  },
	{ .name = "omap_gpio",  .base = GPIO2_BASE,   .irq = 0  },
	{ .name = "omap_gpio",  .base = GPIO3_BASE,   .irq = 0  },
};

static int __init bbb_board_init(void)
{
	pinctrl_register(bbb_pin_groups, ARRAY_SIZE(bbb_pin_groups));

	for (unsigned int i = 0; i < ARRAY_SIZE(bbb_devices); i++)
		platform_device_register(&bbb_devices[i]);

	return 0;
}
arch_initcall(bbb_board_init);
