/*
 * include/nothan/usb.h — USB Core Subsystem API
 *
 * Provides USB 2.0 standard descriptors, core device structures,
 * and the USB Request Block (URB) submission interface.
 */

#ifndef NOTHAN_USB_H
#define NOTHAN_USB_H

#include "types.h"

#define USB_DIR_OUT                 0
#define USB_DIR_IN                  0x80

#define USB_TYPE_STANDARD           (0x00 << 5)
#define USB_TYPE_CLASS              (0x01 << 5)
#define USB_TYPE_VENDOR             (0x02 << 5)

#define USB_RECIP_DEVICE            0x00
#define USB_RECIP_INTERFACE         0x01
#define USB_RECIP_ENDPOINT          0x02

#define USB_REQ_GET_STATUS          0x00
#define USB_REQ_CLEAR_FEATURE       0x01
#define USB_REQ_SET_FEATURE         0x03
#define USB_REQ_SET_ADDRESS         0x05
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_SET_DESCRIPTOR      0x07
#define USB_REQ_GET_CONFIGURATION   0x08
#define USB_REQ_SET_CONFIGURATION   0x09

#define USB_DT_DEVICE               0x01
#define USB_DT_CONFIG               0x02
#define USB_DT_STRING               0x03
#define USB_DT_INTERFACE            0x04
#define USB_DT_ENDPOINT             0x05

#define USB_CLASS_PER_INTERFACE     0x00
#define USB_CLASS_HID               0x03
#define USB_CLASS_HUB               0x09

/**
 * struct usb_ctrlrequest - USB Control Setup Packet
 * @bRequestType: direction, type and recipient
 * @bRequest: request code
 * @wValue: request specific parameter
 * @wIndex: request specific parameter
 * @wLength: length of data phase
 */
struct usb_ctrlrequest {
    uint8_t  bRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__ ((packed));

/**
 * struct usb_device_descriptor - Standard USB Device Descriptor
 * @bLength: size of this descriptor
 * @bDescriptorType: USB_DT_DEVICE
 * @bcdUSB: USB spec release number
 * @bDeviceClass: class code
 * @bDeviceSubClass: subclass code
 * @bDeviceProtocol: protocol code
 * @bMaxPacketSize0: max packet size for endpoint 0
 * @idVendor: vendor ID
 * @idProduct: product ID
 * @bcdDevice: device release number
 * @iManufacturer: index of manufacturer string
 * @iProduct: index of product string
 * @iSerialNumber: index of serial number string
 * @bNumConfigurations: number of possible configurations
 */
struct usb_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__ ((packed));

struct usb_device;

/**
 * struct usb_request - USB Request Block
 * @dev: target USB device
 * @endpoint: target endpoint address
 * @setup_packet: control transfer setup packet
 * @buffer: data buffer
 * @length: length of data buffer
 * @actual_length: number of bytes transferred
 * @status: request completion status
 */
struct usb_request {
    struct usb_device      *dev;
    uint8_t                 endpoint;
    struct usb_ctrlrequest *setup_packet;
    void                   *buffer;
    uint32_t                length;
    uint32_t                actual_length;
    int                     status;
    void                    (*complete)(struct usb_request *req);
};

/**
 * struct usb_device - runtime state of a USB device
 * @devnum: assigned USB address
 * @descriptor: cached device descriptor
 * @ep0_maxpacket: endpoint 0 max packet size
 */
struct usb_device {
    uint8_t                      devnum;
    struct usb_device_descriptor descriptor;
    uint32_t                     ep0_maxpacket;
};

int usb_submit_request(struct usb_request *req);
int usb_enumerate_device(struct usb_device *dev);
int omap_musb_submit_request(struct usb_request *req);

#endif /* NOTHAN_USB_H */
