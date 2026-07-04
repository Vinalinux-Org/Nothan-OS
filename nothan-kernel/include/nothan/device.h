#ifndef _NOTHAN_DEVICE_H
#define _NOTHAN_DEVICE_H

#include <nothan/types.h>

#define DEVICE_MAX_RESOURCES 4

/* Resource flags */
#define IORESOURCE_MEM 0x01
#define IORESOURCE_IRQ 0x02

struct resource {
	u32 start;
	u32 end;
	u32 flags;
};

struct driver;
struct bus_type;

struct device {
	const char      *name;
	struct bus_type *bus;
	struct driver   *driver;
	void            *priv;
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

#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - __builtin_offsetof(type, member)))

#endif /* _NOTHAN_DEVICE_H */
