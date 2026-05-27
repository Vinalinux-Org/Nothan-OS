/*
 * include/nothan/hid.h — Human Interface Device Subsystem
 *
 * Provides HID protocol constants and standard report structures
 * for keyboards, mice, and other input devices.
 */

#ifndef NOTHAN_HID_H
#define NOTHAN_HID_H

#include "types.h"

#define HID_REQ_GET_REPORT      0x01
#define HID_REQ_GET_IDLE        0x02
#define HID_REQ_GET_PROTOCOL    0x03
#define HID_REQ_SET_REPORT      0x09
#define HID_REQ_SET_IDLE        0x0A
#define HID_REQ_SET_PROTOCOL    0x0B

/**
 * struct hid_keyboard_report - standard 8-byte boot keyboard report
 * @modifiers: bitmask of shift, ctrl, alt, gui (left/right)
 * @reserved: typically 0
 * @keys: array of 6 key scancodes currently pressed
 */
struct hid_keyboard_report {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} __attribute__ ((packed));

/**
 * struct hid_mouse_report - standard 3-byte boot mouse report
 * @buttons: bitmask of left, right, middle clicks
 * @x_displacement: signed X movement
 * @y_displacement: signed Y movement
 */
struct hid_mouse_report {
    uint8_t buttons;
    int8_t  x_displacement;
    int8_t  y_displacement;
} __attribute__ ((packed));

void hid_keyboard_process_report(const struct hid_keyboard_report *report);

struct usb_device;
int usbhid_probe(struct usb_device *dev);

#endif /* NOTHAN_HID_H */
