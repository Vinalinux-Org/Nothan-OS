/*
 * drivers/input/input_core.c — input device registry and event dispatch
 *
 * Maintains a static table of registered input_dev pointers.
 * input_report_event() calls the handler callback set by the consumer.
 */

#include "vinix/printk.h"
#include "vinix/input.h"

static struct input_dev *input_devices[INPUT_MAX_DEVICES];
static int input_dev_count;

int input_register_device(struct input_dev *dev)
{
    if (input_dev_count >= INPUT_MAX_DEVICES) {
        pr_err("[INPUT] registry full, cannot register '%s'\n", dev->name);
        return -1;
    }
    input_devices[input_dev_count++] = dev;
    pr_info("[INPUT] registered '%s'\n", dev->name);
    return 0;
}

void input_unregister_device(struct input_dev *dev)
{
    for (int i = 0; i < input_dev_count; i++) {
        if (input_devices[i] == dev) {
            input_devices[i] = input_devices[--input_dev_count];
            input_devices[input_dev_count] = NULL;
            return;
        }
    }
}

void input_report_event(struct input_dev *dev, struct input_event *ev)
{
    if (dev && dev->event)
        dev->event(dev, ev);
}

int input_device_count(void)
{
    return input_dev_count;
}

struct input_dev *input_device_at(uint32_t index)
{
    if (index >= (uint32_t)input_dev_count)
        return NULL;
    return input_devices[index];
}
