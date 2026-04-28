/*
 * drivers/base/device.c — generic bus core
 *
 * Provides bus_register(), bus_add_device(), and bus_add_driver().
 * When a device or driver is added, the bus immediately attempts to
 * match and probe all unbound pairs on the same bus.
 */

#include "device.h"
#include "uart.h"

int bus_register(struct bus_type *bus)
{
    bus->num_devices = 0;
    bus->num_drivers = 0;
    pr_info("[BUS] %s registered\n", bus->name ? bus->name : "?");
    return 0;
}

static int try_bind(struct bus_type *bus, struct device *dev, struct driver *drv)
{
    if (!bus->match || !bus->match(dev, drv)) return 0;
    if (!drv->probe) return 0;

    dev->driver = drv;
    int rc = drv->probe(dev);
    if (rc != 0) {
        dev->driver = NULL;
        pr_err("[BUS] probe %s failed for %s: %d\n",
                    drv->name, dev->name, rc);
    }
    return rc;
}

int bus_add_device(struct bus_type *bus, struct device *dev)
{
    if (bus->num_devices >= BUS_MAX_DEVICES) return -1;
    bus->devices[bus->num_devices++] = dev;
    dev->bus = bus;

    for (int i = 0; i < bus->num_drivers; i++) {
        if (dev->driver) break;
        try_bind(bus, dev, bus->drivers[i]);
    }
    return 0;
}

int bus_add_driver(struct bus_type *bus, struct driver *drv)
{
    if (bus->num_drivers >= BUS_MAX_DRIVERS) return -1;
    bus->drivers[bus->num_drivers++] = drv;
    drv->bus = bus;

    int bound = 0;
    int last_err = 0;
    for (int i = 0; i < bus->num_devices; i++) {
        struct device *dev = bus->devices[i];
        if (dev->driver) continue;
        int rc = try_bind(bus, dev, drv);
        if (rc != 0) last_err = rc;
        if (dev->driver == drv) bound++;
    }
    if (bound == 0 && last_err == 0) {
        pr_info("[BUS] driver '%s' registered, no matching device yet\n",
                    drv->name);
    }
    return last_err;
}
