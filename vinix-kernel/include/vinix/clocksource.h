/* ============================================================
 * vinix/clocksource.h
 * ------------------------------------------------------------
 * struct clock_event_device — periodic/oneshot timer abstraction
 * for the scheduler tick. A driver fills this with set_next_event
 * and set_state callbacks plus shift/mult for ns conversions.
 *
 * Today omap_dmtimer hardcodes a 10 ms autoreload. This header is
 * the contract for swapping in a different SoC timer without
 * rewriting kernel/sched/scheduler.c.
 * ============================================================ */

#ifndef VINIX_CLOCKSOURCE_H
#define VINIX_CLOCKSOURCE_H

#include "types.h"

enum clock_event_state {
    CLOCK_EVT_STATE_DETACHED  = 0,
    CLOCK_EVT_STATE_SHUTDOWN  = 1,
    CLOCK_EVT_STATE_PERIODIC  = 2,
    CLOCK_EVT_STATE_ONESHOT   = 3,
};

struct clock_event_device {
    const char *name;
    uint32_t    rating;        /* higher wins when multiple chips probe */
    uint32_t    features;
    int  (*set_next_event)   (uint32_t evt, struct clock_event_device *);
    int  (*set_state_periodic)(struct clock_event_device *);
    int  (*set_state_oneshot)(struct clock_event_device *);
    int  (*set_state_shutdown)(struct clock_event_device *);
    void (*event_handler)    (struct clock_event_device *);
};

int                        clockevents_register_device(struct clock_event_device *dev);
struct clock_event_device *clockevents_get_active(void);

#endif /* VINIX_CLOCKSOURCE_H */
