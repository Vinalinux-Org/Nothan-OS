/* ============================================================
 * platform_device.h
 * ------------------------------------------------------------
 * Platform bus — non-discoverable memory-mapped peripherals.
 * ============================================================ */

#ifndef PLATFORM_DEVICE_H
#define PLATFORM_DEVICE_H

#include "device.h"

struct platform_device {
    struct device dev;         /* first field — container_of target */
    const char   *name;
    uint32_t      base;
    int           irq;
    const char   *clk_id;
};

struct platform_driver {
    struct driver drv;         /* first field */
    int (*probe)(struct platform_device *pdev);
    int (*remove)(struct platform_device *pdev);
};

/* Populate pdev->dev.resources from {base, irq} and attach to the
 * platform bus. Safe to call before any drivers register. */
int platform_device_register(struct platform_device *pdev);

/* Adds pdrv to the platform bus and immediately runs match+probe
 * against every device currently on the bus. */
int platform_driver_register(struct platform_driver *pdrv);

/* Returns the Nth resource whose flags include `type` (or NULL). */
struct resource *platform_get_resource(struct platform_device *pdev,
                                       uint32_t type, unsigned int index);

/* Convenience for the common single-IRQ case. */
int platform_get_irq(struct platform_device *pdev, unsigned int index);

/* Dump every platform bus device into an array of dev_info_t
 * entries. Returns number written. Stops at max_count. */
int platform_list_devices(void *user_buf, uint32_t max_count);

/* Globally reachable platform bus, for shell / selftest. */
extern struct bus_type platform_bus;

#endif /* PLATFORM_DEVICE_H */
