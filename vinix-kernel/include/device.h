/* ============================================================
 * device.h
 * ------------------------------------------------------------
 * Generic device / driver / bus model.
 * ============================================================ */

#ifndef DEVICE_H
#define DEVICE_H

#include "types.h"

#define DEVICE_MAX_RESOURCES 4

/* resource.flags values — match Linux semantics. */
#define IORESOURCE_MEM 0x01
#define IORESOURCE_IRQ 0x02

struct resource {
    uint32_t start;
    uint32_t end;
    uint32_t flags;
};

struct driver;
struct bus_type;

struct device {
    const char      *name;
    struct bus_type *bus;
    struct driver   *driver;    /* set once bound via match+probe */
    void            *priv;      /* driver-private state */
    struct resource  resources[DEVICE_MAX_RESOURCES];
    int              num_resources;
};

struct driver {
    const char      *name;
    struct bus_type *bus;
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
};

#define BUS_MAX_DEVICES 16
#define BUS_MAX_DRIVERS 16

struct bus_type {
    const char *name;
    int (*match)(struct device *dev, struct driver *drv);

    struct device *devices[BUS_MAX_DEVICES];
    int            num_devices;

    struct driver *drivers[BUS_MAX_DRIVERS];
    int            num_drivers;
};

int bus_register(struct bus_type *bus);
int bus_add_device(struct bus_type *bus, struct device *dev);
int bus_add_driver(struct bus_type *bus, struct driver *drv);

/* container_of for navigating from embedded struct device back
 * to the surrounding subclass (platform_device, etc). */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - ((uint32_t) &((type *)0)->member)))

#endif /* DEVICE_H */
