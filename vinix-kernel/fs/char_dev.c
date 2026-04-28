/* ============================================================
 * char_dev.c
 * ------------------------------------------------------------
 * Flat cdev registry — devfs walks this list to populate /dev.
 * ============================================================ */

#include "vinix/cdev.h"
#include "string.h"

#define MAX_CDEVS 16

static struct cdev devs[MAX_CDEVS];
static uint32_t    dev_count = 0;

int cdev_register(const struct cdev *c)
{
    if (dev_count >= MAX_CDEVS) return -1;
    devs[dev_count] = *c;
    return (int)dev_count++;
}

int cdev_count(void)
{
    return (int)dev_count;
}

const struct cdev *cdev_at(uint32_t index)
{
    if (index >= dev_count) return 0;
    return &devs[index];
}

int cdev_find(const char *name)
{
    for (uint32_t i = 0; i < dev_count; i++) {
        if (strcmp(devs[i].name, name) == 0) return (int)i;
    }
    return -1;
}
