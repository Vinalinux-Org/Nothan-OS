/*
 * drivers/usb/hid/hid_mouse.c — USB HID boot mouse driver
 *
 * Parses 3-byte HID boot protocol mouse reports into input events.
 * Called from musb_host when interrupt IN data arrives.
 */

#include "types.h"
#include "vinix/printk.h"
#include "vinix/input.h"
#include "vinix/mouse_cursor.h"

struct hid_mouse_dev {
    struct input_dev *idev;
    uint8_t ep_addr;
    uint8_t maxpkt;
    uint8_t interval;
    uint8_t attached;
};

static struct hid_mouse_dev hid_mouse;

void usb_hid_mouse_attach(struct input_dev *idev,
                          uint8_t ep_addr, uint8_t maxpkt, uint8_t interval)
{
    hid_mouse.idev = idev;
    hid_mouse.ep_addr = ep_addr;
    hid_mouse.maxpkt = maxpkt;
    hid_mouse.interval = interval;
    hid_mouse.attached = 1;
    cursor_set_input_dev(idev);
    pr_info("[HID] mouse attached: EP%d maxpkt=%d interval=%d\n",
            ep_addr, maxpkt, interval);
}

void usb_hid_mouse_irq(const void *data, int len)
{
    if (!hid_mouse.attached || !hid_mouse.idev) return;
    if (len < 3) return;

    const uint8_t *r = (const uint8_t *)data;
    struct input_event ev;

    ev.type  = EV_KEY;
    ev.code  = BTN_LEFT;
    ev.value = (r[0] & 0x01) ? 1 : 0;
    input_report_event(hid_mouse.idev, &ev);

    ev.code  = BTN_RIGHT;
    ev.value = (r[0] & 0x02) ? 1 : 0;
    input_report_event(hid_mouse.idev, &ev);

    ev.code  = BTN_MIDDLE;
    ev.value = (r[0] & 0x04) ? 1 : 0;
    input_report_event(hid_mouse.idev, &ev);

    ev.type  = EV_REL;
    ev.code  = REL_X;
    ev.value = (int32_t)(int8_t)r[1];
    input_report_event(hid_mouse.idev, &ev);

    ev.code  = REL_Y;
    ev.value = (int32_t)(int8_t)r[2];
    input_report_event(hid_mouse.idev, &ev);

    /* HID boot protocol byte 4: signed scroll delta (positive = down) */
    if (len >= 4 && r[3]) {
        ev.code  = REL_WHEEL;
        ev.value = (int32_t)(int8_t)r[3];
        input_report_event(hid_mouse.idev, &ev);
    }

    ev.type  = EV_SYN;
    ev.code  = 0;
    ev.value = 0;
    input_report_event(hid_mouse.idev, &ev);
}

void usb_hid_mouse_detach(void)
{
    if (!hid_mouse.attached) return;
    input_unregister_device(hid_mouse.idev);
    pr_info("[HID] mouse detached\n");
    hid_mouse.attached = 0;
    hid_mouse.idev = NULL;
}
