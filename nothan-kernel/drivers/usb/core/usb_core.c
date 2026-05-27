/*
 * drivers/usb/core/usb_core.c — USB Core Subsystem
 *
 * Implements the standard USB enumeration sequence and control
 * message helpers.
 */

#include "types.h"
#include "nothan/usb.h"
#include "nothan/printk.h"
#include "nothan/init.h"
#include "nothan/errno.h"
#include "nothan/hid.h"

int usb_submit_request(struct usb_request *req)
{
    if (!req || !req->dev)
        return -EINVAL;
    
    return omap_musb_submit_request(req);
}

static int usb_control_msg(struct usb_device *dev, uint8_t request_type,
                           uint8_t request, uint16_t value, uint16_t index,
                           void *data, uint16_t size)
{
    struct usb_ctrlrequest setup;
    struct usb_request req;

    setup.bRequestType = request_type;
    setup.bRequest     = request;
    setup.wValue       = value;
    setup.wIndex       = index;
    setup.wLength      = size;

    req.dev           = dev;
    req.endpoint      = 0;
    req.setup_packet  = &setup;
    req.buffer        = data;
    req.length        = size;
    req.actual_length = 0;
    req.status        = 0;

    return usb_submit_request(&req);
}

static int usb_set_address(struct usb_device *dev, uint8_t address)
{
    return usb_control_msg(dev,
                           USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                           USB_REQ_SET_ADDRESS,
                           address,
                           0,
                           NULL,
                           0);
}

static int usb_get_descriptor(struct usb_device *dev, uint8_t type,
                              uint8_t index, void *buf, uint16_t size)
{
    uint16_t value = (type << 8) | index;

    return usb_control_msg(dev,
                           USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                           USB_REQ_GET_DESCRIPTOR,
                           value,
                           0,
                           buf,
                           size);
}

static int usb_set_configuration(struct usb_device *dev, uint8_t configuration)
{
    return usb_control_msg(dev,
                           USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                           USB_REQ_SET_CONFIGURATION,
                           configuration,
                           0,
                           NULL,
                           0);
}

int usb_enumerate_device(struct usb_device *dev)
{
    int ret;

    if (!dev)
        return -EINVAL;

    dev->devnum = 0;
    dev->ep0_maxpacket = 8; 

    ret = usb_get_descriptor(dev, USB_DT_DEVICE, 0, &dev->descriptor, 8);
    if (ret < 0) {
        pr_err("[USB] Failed to read initial device descriptor\n");
        return ret;
    }

    dev->ep0_maxpacket = dev->descriptor.bMaxPacketSize0;

    ret = usb_set_address(dev, 1);
    if (ret < 0) {
        pr_err("[USB] Failed to set address\n");
        return ret;
    }
    
    /* Device needs a small delay to process Set Address command */
    for (volatile int i = 0; i < 100000; i++);
    
    dev->devnum = 1;

    ret = usb_get_descriptor(dev, USB_DT_DEVICE, 0, &dev->descriptor,
                             sizeof(struct usb_device_descriptor));
    if (ret < 0) {
        pr_err("[USB] Failed to read full device descriptor\n");
        return ret;
    }

    pr_info("[USB] Found Device: VID 0x%04x PID 0x%04x\n",
            dev->descriptor.idVendor, dev->descriptor.idProduct);

    ret = usb_set_configuration(dev, 1);
    if (ret < 0) {
        pr_err("[USB] Failed to set configuration\n");
        return ret;
    }

    usbhid_probe(dev);

    return 0;
}

static int __init usb_core_init(void)
{
    pr_info("[USB] Core subsystem initialized\n");
    return 0;
}
subsys_initcall(usb_core_init);
