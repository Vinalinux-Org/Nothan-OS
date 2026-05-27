/*
 * drivers/hid/usbhid.c — Generic USB HID Class Driver
 *
 * Periodically polls the Interrupt IN endpoint of HID devices
 * to read input reports.
 */

#include "types.h"
#include "nothan/printk.h"
#include "nothan/init.h"
#include "nothan/errno.h"
#include "nothan/usb.h"
#include "nothan/hid.h"

static struct usb_request hid_poll_req;
static struct hid_keyboard_report kbd_report;

static void usbhid_poll_complete(struct usb_request *req)
{
    if (req->status == 0 && req->actual_length == sizeof(kbd_report))
        hid_keyboard_process_report((struct hid_keyboard_report *)req->buffer);
    
    usb_submit_request(req);
}

int usbhid_probe(struct usb_device *dev)
{
    hid_poll_req.dev           = dev;
    hid_poll_req.endpoint      = 1 | USB_DIR_IN;
    hid_poll_req.setup_packet  = NULL;
    hid_poll_req.buffer        = &kbd_report;
    hid_poll_req.length        = sizeof(kbd_report);
    hid_poll_req.actual_length = 0;
    hid_poll_req.status        = 0;
    hid_poll_req.complete      = usbhid_poll_complete;

    return usb_submit_request(&hid_poll_req);
}

static int __init usbhid_init(void)
{
    return 0;
}
device_initcall(usbhid_init);
