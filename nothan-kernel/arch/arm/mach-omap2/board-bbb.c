/*
 * arch/arm/mach-omap2/board-bbb.c - BeagleBone Black platform devices
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/platform.h>
#include <nothan/mmio.h>
#include <nothan/pinctrl.h>
#include <nothan/i2c.h>
#include <nothan/init.h>

/* L4_PER peripherals */
#define L4_PER_BASE		0x48000000
#define DMTIMER2_BASE		(L4_PER_BASE + 0x40000)	/* 0x48040000 */
#define INTC_BASE		0x48200000

/* USB subsystem (USBSS base; usb1 = BBB Type-A host port, IRQ 19) */
#define USBSS_BASE		0x47400000
#define USB1_IRQ		19

/* CPSW Ethernet subsystem; RX_PULSE is 3PGSWRXINT0 (TRM Ch06). */
#define CPSW_BASE		0x4A100000
#define CPSW_RX_IRQ		41

/*
 * The MAC address is fused into the control module.  Byte order is the trap:
 * the register named _lo holds the *high* two bytes of the address and _hi
 * holds the low four, so reading them in the order their names suggest
 * produces a reversed address — one that fails unicast filtering silently
 * while broadcast still arrives, which is about the most confusing way this
 * could go wrong.
 *
 * Verified against two sources, as roadmap §1 requires: TRM §9.3.1.24 and
 * §9.3.1.25 for the field layout, and cpsw-common.c in the Digikey u-boot
 * for the byte assembly a working boot actually performs.  They agree.
 */
#define CTRL_MAC_ID0_LO		0x44E10630
#define CTRL_MAC_ID0_HI		0x44E10634

void board_get_mac_addr(u8 *addr)
{
	u32 lo = mmio_read32(phys_to_mmio(CTRL_MAC_ID0_LO));
	u32 hi = mmio_read32(phys_to_mmio(CTRL_MAC_ID0_HI));

	addr[0] =  hi        & 0xFF;
	addr[1] = (hi >>  8) & 0xFF;
	addr[2] = (hi >> 16) & 0xFF;
	addr[3] = (hi >> 24) & 0xFF;
	addr[4] =  lo        & 0xFF;
	addr[5] = (lo >>  8) & 0xFF;
}

/* L4_WKUP peripherals */
#define L4_WKUP_BASE		0x44E00000
#define UART0_BASE		(L4_WKUP_BASE + 0x9000)		/* 0x44E09000 */
#define UART1_BASE		(L4_PER_BASE  + 0x22000)	/* 0x48022000 (SIM7600 modem) */
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

/* UART1 → SIM7600 modem. P9_26 = uart1_rxd, P9_24 = uart1_txd (mode 0). */
static const struct pin_desc uart1_pins[] = {
	{ CM_REG(0x980), PIN_INPUT_PULLUP  | PIN_MUXMODE(0) },	/* uart1_rxd (P9_26) */
	{ CM_REG(0x984), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* uart1_txd (P9_24) */
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

/* LCDC pins — LCD_DATA0-15, VSYNC, HSYNC, PCLK, AC_BIAS_EN (all mode 0) */
static const struct pin_desc lcdc_pins[] = {
	{ CM_REG(0x8A0), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data0  */
	{ CM_REG(0x8A4), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data1  */
	{ CM_REG(0x8A8), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data2  */
	{ CM_REG(0x8AC), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data3  */
	{ CM_REG(0x8B0), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data4  */
	{ CM_REG(0x8B4), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data5  */
	{ CM_REG(0x8B8), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data6  */
	{ CM_REG(0x8BC), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data7  */
	{ CM_REG(0x8C0), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data8  */
	{ CM_REG(0x8C4), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data9  */
	{ CM_REG(0x8C8), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data10 */
	{ CM_REG(0x8CC), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data11 */
	{ CM_REG(0x8D0), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data12 */
	{ CM_REG(0x8D4), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data13 */
	{ CM_REG(0x8D8), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data14 */
	{ CM_REG(0x8DC), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_data15 */
	{ CM_REG(0x8E0), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_vsync  */
	{ CM_REG(0x8E4), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_hsync  */
	{ CM_REG(0x8E8), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_pclk   */
	{ CM_REG(0x8EC), PIN_OUTPUT_PULLDOWN | PIN_MUXMODE(0) },/* lcd_ac_bias_en */
};

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof((a)[0]))

static const struct pin_group bbb_pin_groups[] = {
	{ "uart0", uart0_pins, ARRAY_SIZE(uart0_pins) },
	{ "uart1", uart1_pins, ARRAY_SIZE(uart1_pins) },
	{ "mmc0",  mmc0_pins,  ARRAY_SIZE(mmc0_pins)  },
	{ "i2c0",  i2c0_pins,  ARRAY_SIZE(i2c0_pins)  },
	{ "lcdc",  lcdc_pins,  ARRAY_SIZE(lcdc_pins)  },
};

static struct platform_device bbb_devices[] = {
	{ .name = "omap_intc",  .base = INTC_BASE,    .irq = 0  },
	{ .name = "omap_timer", .base = DMTIMER2_BASE, .irq = 68 },
	{ .name = "omap_uart",  .base = UART0_BASE,   .irq = 72 },
#if CONFIG_MODEM
	{ .name = "omap_uart",  .base = UART1_BASE,   .irq = 73 },
#endif
	{ .name = "omap_mmc",   .base = MMC0_BASE,    .irq = 64 },
	{ .name = "omap_gpio",  .base = GPIO0_BASE,   .irq = 0  },
	{ .name = "omap_gpio",  .base = GPIO1_BASE,   .irq = 0  },
	{ .name = "omap_gpio",  .base = GPIO2_BASE,   .irq = 0  },
	{ .name = "omap_gpio",  .base = GPIO3_BASE,   .irq = 0  },
#if CONFIG_USB_TOUCH
	{ .name = "musb_hcd",   .base = USBSS_BASE,   .irq = USB1_IRQ },
#endif
#if CONFIG_ETHERNET
	/*
	 * CPSW subsystem base, and the receive interrupt: TRM Ch06 lists 41 as
	 * 3PGSWRXINT0 (c0_rx_pend).  40 is the threshold pulse, 42 transmit,
	 * 43 miscellaneous — only the receive one is wanted here.
	 */
	{ .name = "cpsw",       .base = CPSW_BASE,    .irq = CPSW_RX_IRQ },
#endif
};

#if CONFIG_VIDEO
static const struct i2c_board_info i2c0_devices[] = {
	{ "tda19988", 0x70 },   /* NXP TDA19988 HDMI framer */
};
#endif

static int __init bbb_board_init(void)
{
	pinctrl_register(bbb_pin_groups, ARRAY_SIZE(bbb_pin_groups));

#if CONFIG_VIDEO
	i2c_register_board_info(0, i2c0_devices, ARRAY_SIZE(i2c0_devices));
#endif

	for (unsigned int i = 0; i < ARRAY_SIZE(bbb_devices); i++)
		platform_device_register(&bbb_devices[i]);

	return 0;
}
arch_initcall(bbb_board_init);
