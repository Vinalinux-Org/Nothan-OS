/*
 * arch/arm/mach-omap2/board-bbb.c - BeagleBone Black platform devices
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/platform.h>
#include <nothan/init.h>

/* L4_PER peripherals */
#define L4_PER_BASE		0x48000000
#define DMTIMER2_BASE		(L4_PER_BASE + 0x40000)	/* 0x48040000 */
#define INTC_BASE		0x48200000

/* L4_WKUP peripherals */
#define L4_WKUP_BASE		0x44E00000
#define UART0_BASE		(L4_WKUP_BASE + 0x9000)		/* 0x44E09000 */
#define MMC0_BASE		(L4_PER_BASE + 0x60000)		/* 0x48060000 */

static struct platform_device bbb_devices[] = {
	{
		.name = "omap_intc",
		.base = INTC_BASE,
		.irq  = 0,
	},
	{
		.name = "omap_timer",
		.base = DMTIMER2_BASE,
		.irq  = 68,		/* DMTimer2 IRQ */
	},
	{
		.name = "omap_uart",
		.base = UART0_BASE,
		.irq  = 72,		/* UART0 IRQ */
	},
	{
		.name = "omap_mmc",
		.base = MMC0_BASE,
		.irq  = 64,		/* MMC0 IRQ */
	},
};

static int __init bbb_board_init(void)
{
	for (unsigned int i = 0; i < sizeof(bbb_devices) / sizeof(bbb_devices[0]); i++)
		platform_device_register(&bbb_devices[i]);

	return 0;
}
arch_initcall(bbb_board_init);
