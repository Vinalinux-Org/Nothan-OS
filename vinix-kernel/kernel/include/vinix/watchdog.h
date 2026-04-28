/* ============================================================
 * vinix/watchdog.h
 * ------------------------------------------------------------
 * Watchdog subsystem — drivers fill struct watchdog_device with
 * an ops vtable, call watchdog_register_device. Userspace pings
 * via /dev/watchdog (cdev backed by the registered device).
 *
 * Today omap_wdt.c only disables the IPC watchdog at boot and
 * exposes nothing further. The core defined here is the contract
 * for Phase 2.9 if/when a userspace watchdog ping path is needed.
 * ============================================================ */

#ifndef VINIX_WATCHDOG_H
#define VINIX_WATCHDOG_H

#include "types.h"

struct watchdog_device;

struct watchdog_ops {
    int (*start)  (struct watchdog_device *wdd);
    int (*stop)   (struct watchdog_device *wdd);
    int (*ping)   (struct watchdog_device *wdd);
    int (*set_timeout)(struct watchdog_device *wdd, uint32_t seconds);
};

struct watchdog_device {
    const char                 *info;       /* identification */
    const struct watchdog_ops  *ops;
    uint32_t                    timeout;    /* current timeout (s) */
    uint32_t                    min_timeout;
    uint32_t                    max_timeout;
    void                       *priv;
};

int watchdog_register_device  (struct watchdog_device *wdd);
int watchdog_unregister_device(struct watchdog_device *wdd);

#endif /* VINIX_WATCHDOG_H */
