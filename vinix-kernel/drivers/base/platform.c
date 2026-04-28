/* ============================================================
 * platform.c
 * ------------------------------------------------------------
 * Platform bus — name-match drv↔pdev, resource lookup.
 * ============================================================ */

#include "platform_device.h"
#include "string.h"
#include "uart.h"

static int platform_match(struct device *dev, struct driver *drv)
{
    if (!dev->name || !drv->name) return 0;
    return strcmp(dev->name, drv->name) == 0;
}

struct bus_type platform_bus = {
    .name  = "platform",
    .match = platform_match,
};

int platform_device_register(struct platform_device *pdev)
{
    int n = 0;
    pdev->dev.name = pdev->name;

    if (pdev->base != 0) {
        pdev->dev.resources[n].start = pdev->base;
        pdev->dev.resources[n].end   = pdev->base + 0xFFF;  /* 4 KB window */
        pdev->dev.resources[n].flags = IORESOURCE_MEM;
        n++;
    }
    if (pdev->irq > 0) {
        pdev->dev.resources[n].start = (uint32_t)pdev->irq;
        pdev->dev.resources[n].end   = (uint32_t)pdev->irq;
        pdev->dev.resources[n].flags = IORESOURCE_IRQ;
        n++;
    }
    pdev->dev.num_resources = n;
    pdev->dev.driver = 0;

    return bus_add_device(&platform_bus, &pdev->dev);
}

/* Wrap driver probe/remove so the bus sees a generic struct device,
 * and the driver still gets a struct platform_device *. */
static int platform_probe_thunk(struct device *dev)
{
    struct driver *drv = dev->driver;
    struct platform_driver *pdrv = container_of(drv, struct platform_driver, drv);
    struct platform_device *pdev = container_of(dev, struct platform_device, dev);
    if (!pdrv->probe) return 0;
    return pdrv->probe(pdev);
}

static int platform_remove_thunk(struct device *dev)
{
    struct driver *drv = dev->driver;
    struct platform_driver *pdrv = container_of(drv, struct platform_driver, drv);
    struct platform_device *pdev = container_of(dev, struct platform_device, dev);
    if (!pdrv->remove) return 0;
    return pdrv->remove(pdev);
}

int platform_driver_register(struct platform_driver *pdrv)
{
    pdrv->drv.name   = pdrv->drv.name ? pdrv->drv.name : "(unnamed)";
    pdrv->drv.probe  = platform_probe_thunk;
    pdrv->drv.remove = platform_remove_thunk;
    return bus_add_driver(&platform_bus, &pdrv->drv);
}

struct resource *platform_get_resource(struct platform_device *pdev,
                                       uint32_t type, unsigned int index)
{
    unsigned int seen = 0;
    for (int i = 0; i < pdev->dev.num_resources; i++) {
        if (pdev->dev.resources[i].flags & type) {
            if (seen == index) return &pdev->dev.resources[i];
            seen++;
        }
    }
    return 0;
}

int platform_get_irq(struct platform_device *pdev, unsigned int index)
{
    struct resource *r = platform_get_resource(pdev, IORESOURCE_IRQ, index);
    return r ? (int)r->start : -1;
}

/* ============================================================
 * Introspection (shell `devlist`)
 * ============================================================ */

#include "syscalls.h"

static void copy_name(char *dst, const char *src, int cap)
{
    int i = 0;
    if (src) while (src[i] && i < cap - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

int platform_list_devices(void *user_buf, uint32_t max_count)
{
    dev_info_t *out = (dev_info_t *)user_buf;
    uint32_t written = 0;

    for (int i = 0; i < platform_bus.num_devices && written < max_count; i++) {
        struct device *dev = platform_bus.devices[i];
        struct platform_device *pdev =
            container_of(dev, struct platform_device, dev);

        copy_name(out[written].name, dev->name, sizeof(out[written].name));
        out[written].base = pdev->base;
        out[written].irq  = pdev->irq > 0 ? pdev->irq : -1;
        copy_name(out[written].driver,
                  dev->driver ? dev->driver->name : "",
                  sizeof(out[written].driver));
        written++;
    }
    return (int)written;
}
