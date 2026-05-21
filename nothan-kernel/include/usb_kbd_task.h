/*
 * include/usb_kbd_task.h — USB keyboard polling task interface
 */

#ifndef NOTHAN_USB_KBD_TASK_H
#define NOTHAN_USB_KBD_TASK_H

#include "task.h"

struct task_struct *usb_kbd_get_task(void);

#endif /* NOTHAN_USB_KBD_TASK_H */
