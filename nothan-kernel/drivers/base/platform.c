/*
 * platform.c - Platform bus — match by name string
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/platform.h>
#include <nothan/printk.h>

static int platform_match(struct device *dev, struct driver *drv)
{
	if (!dev->name || !drv->name)
		return 0;
	const char *dn = dev->name;
	const char *dr = drv->name;
	while (*dn && *dr && *dn == *dr) {
		dn++;
		dr++;
	}
	return *dn == *dr;
}

struct bus_type platform_bus = {
	.name  = "platform",
	.match = platform_match,
};

/**
 * platform_device_register() - Register a platform device on the platform bus
 * @pdev: The platform_device to register (must have .name and .base set)
 *
 * Translates pdev->base and pdev->irq into struct resource entries and
 * adds the device to the platform bus.  A matching driver will be probed
 * if already registered.
 *
 * Return: 0 on success, -1 on error.
 */
int platform_device_register(struct platform_device *pdev)
{
	int n = 0;
	pdev->dev.name = pdev->name;

	if (pdev->base) {
		pdev->dev.resources[n].start = pdev->base;
		pdev->dev.resources[n].end   = pdev->base + 0xFFF;
		pdev->dev.resources[n].flags = IORESOURCE_MEM;
		n++;
	}
	if (pdev->irq > 0) {
		pdev->dev.resources[n].start = (u32)pdev->irq;
		pdev->dev.resources[n].end   = (u32)pdev->irq;
		pdev->dev.resources[n].flags = IORESOURCE_IRQ;
		n++;
	}
	pdev->dev.num_resources = n;
	pdev->dev.driver = 0;

	printk("[PLAT] device '%s' base=0x%lx irq=%d\n",
	       pdev->name, (unsigned long)pdev->base, pdev->irq);

	return bus_add_device(&platform_bus, &pdev->dev);
}

static int platform_probe_thunk(struct device *dev)
{
	struct driver *drv = dev->driver;
	struct platform_driver *pdrv = container_of(drv, struct platform_driver, drv);
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	if (!pdrv->probe)
		return 0;
	return pdrv->probe(pdev);
}

static int platform_remove_thunk(struct device *dev)
{
	struct driver *drv = dev->driver;
	struct platform_driver *pdrv = container_of(drv, struct platform_driver, drv);
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	if (!pdrv->remove)
		return 0;
	return pdrv->remove(pdev);
}

/**
 * platform_driver_register() - Register a platform driver
 * @pdrv: The platform_driver to register
 *
 * Installs probe/remove thunks so the generic bus core can call the
 * platform-specific callbacks with the correct pdev pointer.
 *
 * Return: 0 on success, -1 on error.
 */
int platform_driver_register(struct platform_driver *pdrv)
{
	pdrv->drv.name   = pdrv->drv.name ? pdrv->drv.name : "(unnamed)";
	pdrv->drv.probe  = platform_probe_thunk;
	pdrv->drv.remove = platform_remove_thunk;
	return bus_add_driver(&platform_bus, &pdrv->drv);
}

/**
 * platform_get_resource() - Look up a resource by type and index
 * @pdev: The platform device to search
 * @type: Resource type flag (IORESOURCE_MEM or IORESOURCE_IRQ)
 * @index: Zero-based index among resources of the given type
 *
 * Return: Pointer to the matching resource, or NULL if not found.
 */
struct resource *platform_get_resource(struct platform_device *pdev,
				       u32 type, unsigned int index)
{
	unsigned int seen = 0;
	for (int i = 0; i < pdev->dev.num_resources; i++) {
		if (pdev->dev.resources[i].flags & type) {
			if (seen == index)
				return &pdev->dev.resources[i];
			seen++;
		}
	}
	return 0;
}

/**
 * platform_get_irq() - Get the IRQ number for a platform device
 * @pdev: The platform device
 * @index: Zero-based index of the IRQ resource
 *
 * Return: IRQ number on success, -1 if not found.
 */
int platform_get_irq(struct platform_device *pdev, unsigned int index)
{
	struct resource *r = platform_get_resource(pdev, IORESOURCE_IRQ, index);
	return r ? (int)r->start : -1;
}
