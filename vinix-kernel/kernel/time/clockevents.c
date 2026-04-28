/*
 * kernel/time/clockevents.c — tick-source registry
 */

#include "vinix/clocksource.h"
#include "vinix/printk.h"
#include "vinix/errno.h"
#include "scheduler.h"

static struct clock_event_device *active_dev;

static void clockevents_handler(struct clock_event_device *dev)
{
    (void)dev;
    scheduler_tick();
}

int clockevents_register_device(struct clock_event_device *dev)
{
    if (!dev) return -EINVAL;

    /* Highest-rated device wins for MVP single-active model. */
    if (active_dev && dev->rating <= active_dev->rating) {
        pr_info("[CLKEVT] %s lower rating than active %s, ignored\n",
                dev->name ? dev->name : "?",
                active_dev->name ? active_dev->name : "?");
        return 0;
    }

    dev->event_handler = clockevents_handler;
    active_dev = dev;

    if (dev->set_state_periodic) {
        dev->set_state_periodic(dev);
    }

    pr_info("[CLKEVT] %s registered (rating %u)\n",
            dev->name ? dev->name : "?", dev->rating);
    return 0;
}

struct clock_event_device *clockevents_get_active(void)
{
    return active_dev;
}
