/*
 * bootloader/src/main.c - Bootloader entry: clocks, DDR, MMC, jump to kernel
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include "am335x.h"
#include "boot.h"
#include <stdbool.h>

/* Kernel lives at 1 MB offset on SD (sector 2048, 512-byte sectors) */
#define KERNEL_BASE		0x80000000
#define KERNEL_START_SECTOR	2048
#define KERNEL_SIZE_SECTORS	2048

struct boot_params {
	uint32_t	reserved;
	uint32_t	mem_desc_addr;
	uint8_t		boot_device;
	uint8_t		reset_reason;
	uint8_t		reserved2;
	uint8_t		reserved3;
};

void panic(const char *msg)
{
	uart_puts("\r\nPANIC: ");
	uart_puts(msg);
	uart_puts("\r\nSYSTEM HALTED.\r\n");
	while (1)
		;
}

void c_prefetch_abort_handler(void)
{
	uint32_t ifsr, ifar;

	asm volatile("mrc p15, 0, %0, c5, c0, 1" : "=r" (ifsr));	/* IFSR */
	asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r" (ifar));	/* IFAR */

	uart_puts("\r\nPANIC: PREFETCH ABORT\r\n");
	uart_puts("IFSR: "); uart_print_hex(ifsr); uart_puts("\r\n");
	uart_puts("IFAR: "); uart_print_hex(ifar); uart_puts("\r\n");
	while (1)
		;
}

void c_data_abort_handler(void)
{
	uint32_t dfsr, dfar;

	asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r" (dfsr));	/* DFSR */
	asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r" (dfar));	/* DFAR */

	uart_puts("\r\nPANIC: DATA ABORT\r\n");
	uart_puts("DFAR: "); uart_print_hex(dfar); uart_puts("\r\n");
	uart_puts("DFSR: "); uart_print_hex(dfsr); uart_puts("\r\n");
	while (1)
		;
}

void c_undef_handler(void)
{
	uint32_t ifsr, ifar;

	asm volatile("mrc p15, 0, %0, c5, c0, 1" : "=r" (ifsr));	/* IFSR */
	asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r" (ifar));	/* IFAR */

	uart_puts("\r\nPANIC: UNDEFINED INSTRUCTION\r\n");
	uart_puts("IFAR: "); uart_print_hex(ifar); uart_puts("\r\n");
	uart_puts("IFSR: "); uart_print_hex(ifsr); uart_puts("\r\n");
	while (1)
		;
}

void bootloader_main(void)
{
	struct boot_params params;
	uint32_t *magic;
	uint32_t first;
	bool ok;

	/* Enable clock domains first — needed before any peripheral access. */
	clock_domains_early_init();

	/* Light USR LEDs — first visual sign bootloader is alive. */
	writel(MODULE_ENABLE, CM_PER_GPIO1_CLKCTRL);
	while ((readl(CM_PER_GPIO1_CLKCTRL) & 0x30000) != 0)
		;
	writel(readl(GPIO1_OE) & ~GPIO1_USR_LEDS, GPIO1_OE);
	writel(GPIO1_USR_LEDS, GPIO1_SETDATAOUT);

	/* Configure PLLs and enable peripheral module clocks. */
	clock_init();

	uart_init();
	delay(1000000);

	for (int i = 0; i < 10; i++) {
		uart_putc('\r');
		uart_putc('\n');
	}
	delay(100000);

	uart_puts("[BOOT] NothanOS Bootloader — BeagleBone Black (AM335x)\r\n");

	/* Read back actual DPLL config and calculate real frequencies */
	{
		uint32_t mn, m, n, m2, freq_mhz;

		mn = readl(CM_CLKSEL_DPLL_MPU);
		m  = (mn >> 8) & 0x3FF;
		n  = mn & 0x7F;
		m2 = readl(CM_DIV_M2_DPLL_MPU) & 0x1F;
		freq_mhz = 24 * m / (n + 1) / m2;
		uart_puts("[CLK]  MPU="); uart_print_dec(freq_mhz); uart_puts("MHz");

		mn = readl(CM_CLKSEL_DPLL_CORE);
		m  = (mn >> 8) & 0x3FF;
		n  = mn & 0x7F;
		freq_mhz = 24 * m / (n + 1);
		uart_puts(" CORE="); uart_print_dec(freq_mhz); uart_puts("MHz");

		mn = readl(CM_CLKSEL_DPLL_PER);
		m  = (mn >> 8) & 0x3FF;
		n  = mn & 0x7F;
		m2 = readl(CM_DIV_M2_DPLL_PER) & 0x1F;
		freq_mhz = 24 * m / (n + 1) / m2;
		uart_puts(" PER="); uart_print_dec(freq_mhz); uart_puts("MHz");

		mn = readl(CM_CLKSEL_DPLL_DDR);
		m  = (mn >> 8) & 0x3FF;
		n  = mn & 0x7F;
		m2 = readl(CM_DIV_M2_DPLL_DDR) & 0x1F;
		freq_mhz = 24 * m / (n + 1) / m2;
		uart_puts(" DDR="); uart_print_dec(freq_mhz); uart_puts("MHz\r\n");
	}

	ddr_init();

	if (ddr_test() == 0)
		uart_puts("[DDR]  512MB @ 0x80000000 OK\r\n");
	else
		panic("DDR memory test FAILED!");

	if (mmc_init() != 0)
		panic("MMC initialization FAILED!");

	uart_puts("[MMC]  SD card OK\r\n");

	if (mmc_read_sectors(KERNEL_START_SECTOR, KERNEL_SIZE_SECTORS,
			     (void *)KERNEL_BASE) != 0)
		panic("Failed to load kernel from SD!");

	uart_puts("[MMC]  Kernel loaded\r\n");

	/* Validate kernel entry: must be ARM B or LDR pc instruction. */
	magic = (uint32_t *)KERNEL_BASE;
	first = *magic;
	ok = (((first & 0xFF000000) == 0xEA000000) ||	/* B */
	      ((first & 0xFFFFF000) == 0xE59FF000)) &&	/* LDR pc, [pc, #imm] */
	     first != 0x00000000 && first != 0xFFFFFFFF;

	if (!ok) {
		uart_puts("[BOOT] bad kernel magic: ");
		uart_print_hex(first);
		panic("");
	}

	uart_puts("[BOOT] Jumping to kernel @ 0x80000000\r\n");
	uart_puts("\r\n----------------------------------------\r\n\r\n");
	uart_flush();

	params.reserved      = 0;
	params.mem_desc_addr = 0;
	params.boot_device   = 0x08;	/* MMC0 */
	params.reset_reason  = 0x01;	/* power-on cold reset */
	params.reserved2     = 0;
	params.reserved3     = 0;

	/* ARM boot ABI: r0=0, r1=MACH_TYPE (BBB=3589), r2=atags ptr */
	asm volatile(
		"mov r0, #0\n"
		"ldr r1, =0x0E05\n"
		"mov r2, %0\n"
		"ldr pc, =0x80000000\n"
		:: "r" (&params)
	);

	panic("Failed to jump to kernel!");
}
