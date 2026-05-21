/*
 * include/vinix/input.h — input device abstraction
 *
 * Drivers call input_register_device() in probe().
 * Consumers set dev->event before or after registration.
 * input_report_event() dispatches to the handler callback.
 */

#ifndef VINIX_INPUT_H
#define VINIX_INPUT_H

#include "types.h"

#define INPUT_MAX_DEVICES 8

#define EV_KEY   0x01
#define EV_REL   0x02
#define EV_SYN   0x00

#define BTN_LEFT   0x110
#define BTN_RIGHT  0x111
#define BTN_MIDDLE 0x112

#define REL_X     0x00
#define REL_Y     0x01
#define REL_WHEEL 0x08

struct input_dev;

struct input_event {
    uint32_t type;
    uint32_t code;
    int32_t  value;
};

typedef void (*input_handler_t)(struct input_dev *dev,
                                struct input_event *ev);

struct input_dev {
    char             name[32];
    input_handler_t  event;
    void            *priv;
};

int input_register_device  (struct input_dev *dev);
void input_unregister_device(struct input_dev *dev);
void input_report_event     (struct input_dev *dev, struct input_event *ev);

int input_device_count(void);
struct input_dev *input_device_at(uint32_t index);

#endif
