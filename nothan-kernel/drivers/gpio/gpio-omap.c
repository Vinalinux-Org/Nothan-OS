/*
 * drivers/gpio/gpio-omap.c - AM335x GPIO bank driver
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/gpio.h>
#include <nothan/platform.h>
#include <nothan/mmio.h>
#include <nothan/printk.h>
#include <nothan/init.h>

/* PRCM clock control — VA (PA 0x44E00000 → VA 0xF0E00000) */
#define CM_WKUP_GPIO0_CLKCTRL	0xF0E00408	/* PA 0x44E00408 */
#define CM_PER_GPIO1_CLKCTRL	0xF0E000AC	/* PA 0x44E000AC */
#define CM_PER_GPIO2_CLKCTRL	0xF0E000B0	/* PA 0x44E000B0 */
#define CM_PER_GPIO3_CLKCTRL	0xF0E000B4	/* PA 0x44E000B4 */
#define MODULEMODE_ENABLE	0x02

/* GPIO register offsets */
#define GPIO_CTRL		0x0130	/* module control: bit0=DISABLEMODULE */
#define GPIO_OE			0x0134	/* output enable: 1=input, 0=output */
#define GPIO_DATAIN		0x0138	/* sampled pin state (input path) */
#define GPIO_DATAOUT		0x013C	/* driven output value */
#define GPIO_CLEARDATAOUT	0x0190	/* write 1 to drive output low */
#define GPIO_SETDATAOUT		0x0194	/* write 1 to drive output high */

struct omap_gpio_chip {
	struct gpio_chip	chip;
	u32			base;
};

static int omap_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct omap_gpio_chip *omap = (struct omap_gpio_chip *)chip;
	u32 oe = mmio_read32(omap->base + GPIO_OE);

	/* OE=1 → pin sampled as input */
	oe |= (1U << offset);
	mmio_write32(omap->base + GPIO_OE, oe);
	return 0;
}

static int omap_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct omap_gpio_chip *omap = (struct omap_gpio_chip *)chip;
	u32 oe;

	if (value)
		mmio_write32(omap->base + GPIO_SETDATAOUT,   1U << offset);
	else
		mmio_write32(omap->base + GPIO_CLEARDATAOUT, 1U << offset);

	oe  = mmio_read32(omap->base + GPIO_OE);
	oe &= ~(1U << offset);  /* OE=0 → output driver enabled */
	mmio_write32(omap->base + GPIO_OE, oe);
	return 0;
}

static int omap_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct omap_gpio_chip *omap = (struct omap_gpio_chip *)chip;
	u32 oe = mmio_read32(omap->base + GPIO_OE);

	if (oe & (1U << offset))
		return (int)((mmio_read32(omap->base + GPIO_DATAIN) >> offset) & 1U);
	return (int)((mmio_read32(omap->base + GPIO_DATAOUT) >> offset) & 1U);
}

static void omap_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct omap_gpio_chip *omap = (struct omap_gpio_chip *)chip;

	if (value)
		mmio_write32(omap->base + GPIO_SETDATAOUT,   1U << offset);
	else
		mmio_write32(omap->base + GPIO_CLEARDATAOUT, 1U << offset);
}

static const u32 gpio_clkctrl_regs[4] = {
	CM_WKUP_GPIO0_CLKCTRL,
	CM_PER_GPIO1_CLKCTRL,
	CM_PER_GPIO2_CLKCTRL,
	CM_PER_GPIO3_CLKCTRL,
};

static struct omap_gpio_chip omap_gpio_banks[4];
static int omap_gpio_next_bank;

static int omap_gpio_probe(struct platform_device *pdev)
{
	struct omap_gpio_chip *omap;
	int bank_id;

	if (omap_gpio_next_bank >= 4)
		return -1;

	bank_id = omap_gpio_next_bank++;
	omap       = &omap_gpio_banks[bank_id];
	omap->base = phys_to_mmio(pdev->base);

	mmio_write32(gpio_clkctrl_regs[bank_id], MODULEMODE_ENABLE);
	while ((mmio_read32(gpio_clkctrl_regs[bank_id]) & 0x30000) != 0)
		;

	mmio_write32(omap->base + GPIO_CTRL, 0);

	omap->chip.label            = "omap-gpio";
	omap->chip.base             = bank_id * 32;
	omap->chip.ngpio            = 32;
	omap->chip.direction_input  = omap_gpio_direction_input;
	omap->chip.direction_output = omap_gpio_direction_output;
	omap->chip.get              = omap_gpio_get;
	omap->chip.set              = omap_gpio_set;
	omap->chip.to_irq           = 0;
	omap->chip.priv             = 0;

	if (gpiochip_add(&omap->chip) < 0) {
		printk("[GPIO] bank%d: gpiochip_add failed\n", bank_id);
		return -1;
	}

	printk("[GPIO] bank%d base=0x%lx gpio%d..gpio%d\n",
	       bank_id, (unsigned long)omap->base,
	       omap->chip.base, omap->chip.base + 31);
	return 0;
}

static struct platform_driver omap_gpio_driver = {
	.drv   = { .name = "omap_gpio" },
	.probe = omap_gpio_probe,
};

static int __init omap_gpio_init(void)
{
	return platform_driver_register(&omap_gpio_driver);
}
device_initcall(omap_gpio_init);
