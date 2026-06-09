/*
 * drivers/irqchip/irq-omap-intc.c - OMAP INTC interrupt controller driver
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/irq.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>
#include <nothan/platform.h>
#include <nothan/init.h>

static int intc_probe(struct platform_device *pdev)
{
	(void)pdev;

	mmio_write32(INTC_BASE + INTC_MIR_SET(0), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(1), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(2), 0xFFFFFFFF);
	mmio_write32(INTC_BASE + INTC_MIR_SET(3), 0xFFFFFFFF);

	mmio_write32(INTC_BASE + INTC_THRESHOLD, 0xFF);

	mmio_write32(INTC_BASE + INTC_CONTROL, NEWIRQAGR);

	printk("[INTC] AM335x INTC ready\n");
	return 0;
}

static struct platform_driver intc_driver = {
	.probe = intc_probe,
};

static int __init omap_intc_init(void)
{
	intc_driver.drv.name = "omap_intc";
	return platform_driver_register(&intc_driver);
}
subsys_initcall(omap_intc_init);

/**
 * intc_enable_irq() - Unmask an interrupt
 * @irq: The IRQ number to enable
 */
void intc_enable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	mmio_write32(INTC_BASE + INTC_ILR(irq), 0);
	mmio_write32(INTC_BASE + INTC_MIR_CLEAR(bank), (1 << bit));
}

/**
 * intc_disable_irq() - Mask an interrupt
 * @irq: The IRQ number to disable
 */
void intc_disable_irq(unsigned int irq)
{
	unsigned int bank = irq / 32;
	unsigned int bit  = irq % 32;

	mmio_write32(INTC_BASE + INTC_MIR_SET(bank), (1 << bit));
}
