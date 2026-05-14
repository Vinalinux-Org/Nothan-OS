/*
 * drivers/gpio/omap_gpio.c — AM335x GPIO bank driver
 *
 * Drives all 4 GPIO banks (GPIO0-GPIO3). Each bank is a separate
 * platform_device probed independently. Provides gpio_direction_output(),
 * gpio_set_value(), gpio_direction_input(), gpio_get_value() for callers
 * using the standard gpio = bank*32+pin numbering.
 */

#include "types.h"
#include "mmio.h"
#include "uart.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/errno.h"
#include "vinix/gpio.h"
#include "mach/prcm.h"

#define GPIO_CTRL         0x130
#define GPIO_OE           0x134
#define GPIO_DATAIN       0x138
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT   0x194

#define GPIO_TIMEOUT      10000
#define GPIO_NUM_BANKS    4

static const uint32_t gpio_clkctrl[GPIO_NUM_BANKS] = {
    CM_WKUP_GPIO0_CLKCTRL,
    CM_PER_GPIO1_CLKCTRL,
    CM_PER_GPIO2_CLKCTRL,
    CM_PER_GPIO3_CLKCTRL,
};

static const uint32_t gpio_hw_bases[GPIO_NUM_BANKS] = {
    0x44E07000,
    0x4804C000,
    0x481AC000,
    0x481AE000,
};

static uint32_t gpio_bases[GPIO_NUM_BANKS];

int gpio_direction_output(unsigned int gpio, int value)
{
    unsigned int bank = gpio / 32;
    unsigned int pin  = gpio % 32;
    uint32_t base     = gpio_bases[bank];

    if (bank >= GPIO_NUM_BANKS || !base)
        return -EINVAL;

    uint32_t oe = mmio_read32(base + GPIO_OE);
    mmio_write32(base + GPIO_OE, oe & ~(1u << pin));

    /* Drive initial value atomically */
    if (value)
        mmio_write32(base + GPIO_SETDATAOUT,   1u << pin);
    else
        mmio_write32(base + GPIO_CLEARDATAOUT, 1u << pin);

    return 0;
}

int gpio_direction_input(unsigned int gpio)
{
    unsigned int bank = gpio / 32;
    unsigned int pin  = gpio % 32;
    uint32_t base     = gpio_bases[bank];

    if (bank >= GPIO_NUM_BANKS || !base)
        return -EINVAL;

    uint32_t oe = mmio_read32(base + GPIO_OE);
    mmio_write32(base + GPIO_OE, oe | (1u << pin));

    return 0;
}

void gpio_set_value(unsigned int gpio, int value)
{
    unsigned int bank = gpio / 32;
    unsigned int pin  = gpio % 32;
    uint32_t base     = gpio_bases[bank];

    if (bank >= GPIO_NUM_BANKS || !base)
        return;

    if (value)
        mmio_write32(base + GPIO_SETDATAOUT,   1u << pin);
    else
        mmio_write32(base + GPIO_CLEARDATAOUT, 1u << pin);
}

int gpio_get_value(unsigned int gpio)
{
    unsigned int bank = gpio / 32;
    unsigned int pin  = gpio % 32;
    uint32_t base     = gpio_bases[bank];

    if (bank >= GPIO_NUM_BANKS || !base)
        return 0;

    return (mmio_read32(base + GPIO_DATAIN) >> pin) & 1;
}

int gpio_request(unsigned int gpio, const char *label)
{
    (void)label;
    if (gpio / 32 >= GPIO_NUM_BANKS) return -EINVAL;
    return 0;
}

void gpio_free(unsigned int gpio)
{
    (void)gpio;
}

static int omap_gpio_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    uint32_t base;
    uint32_t clkctrl;
    int bank;
    int timeout;

    pr_info("[GPIO] probing %s @ 0x%08x\n",
            pdev->name, mem ? mem->start : 0);

    if (!mem)
        return -EINVAL;

    base = mem->start;
    bank = -1;
    for (int i = 0; i < GPIO_NUM_BANKS; i++) {
        if (gpio_hw_bases[i] == base) {
            bank = i;
            break;
        }
    }

    if (bank < 0) {
        pr_err("[GPIO] unknown base 0x%08x\n", base);
        return -EINVAL;
    }

    clkctrl = gpio_clkctrl[bank];

    mmio_write32(clkctrl, MODULEMODE_ENABLE);
    timeout = GPIO_TIMEOUT;
    while (timeout--) {
        if ((mmio_read32(clkctrl) & IDLEST_MASK) == IDLEST_FUNCTIONAL)
            break;
    }
    if (!timeout) {
        pr_err("[GPIO] bank%d clock enable timeout\n", bank);
        return -EIO;
    }
    pr_info("[GPIO] clock enabled, clkctrl=0x%08x\n", mmio_read32(clkctrl));

    /* Enable module (CTRL bit0 = 0 means enabled) */
    mmio_write32(base + GPIO_CTRL, 0x0);

    gpio_bases[bank] = base;
    pr_info("[GPIO] bank%d probe ok @ 0x%08x\n", bank, base);

    return 0;
}

static int omap_gpio_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver omap_gpio_driver = {
    .drv    = { .name = "omap-gpio" },
    .probe  = omap_gpio_probe,
    .remove = omap_gpio_remove,
};

static int __init omap_gpio_driver_init(void)
{
    return platform_driver_register(&omap_gpio_driver);
}
device_initcall(omap_gpio_driver_init);
