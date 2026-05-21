/*
 * drivers/usb/host/musb_host.c — AM335x MUSB Host Controller Driver
 *
 * USB1 host mode: hardware init, device enumeration, HID mouse detection.
 * Wrapper IRQ registers via USB1_CTRL offsets (TRM Ch.16 §4.3).
 * Mentor core registers via USB1_CORE offsets (TRM §16.2.5 confirms
 * INDEX at USBSS+0x140E, proxy at USBSS+0x1410).
 */

#include "types.h"
#include "vinix/printk.h"
#include "mmio.h"
#include "irq.h"
#include "timer.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/errno.h"
#include "vinix/input.h"
#include "mach/prcm.h"
#include "mach/memmap.h"
#include "mach/irqs.h"
#include "mach/control.h"

/* USB_CTRL wrapper register offsets from platform device base — TRM Table 16-103 */
#define CTRL_CTRL           0x14
#define CTRL_IRQSTATRAW1    0x2C    /* unmasked events, same bits as IRQSTAT1 */
#define CTRL_IRQSTAT0       0x30
#define CTRL_IRQSTAT1       0x34
#define CTRL_IRQENSET0      0x38
#define CTRL_IRQENSET1      0x3C
#define CTRL_IRQENCLR0      0x40
#define CTRL_IRQENCLR1      0x44
#define CTRL_UTMI           0xE0
#define CTRL_MODE           0xE8

#define CTRL_SOFT_RESET     (1 << 0)
#define CTRL_SOFT_RESET_ISO (1 << 5)

/* USB0_IRQSTAT1 bits — TRM §16.4.2.8 */
#define IRQ_CONNECT         (1 << 4)
#define IRQ_DISCONNECT      (1 << 5)

/* USB0_MODE bits — TRM §16.4.2.35 Table 16-107 */
#define MODE_IDDIG_MUX      (1 << 7)
#define MODE_IDDIG          (1 << 8)

/* UTMI register bits — TRM §16.4.3.33 Table 16-141 */
#define UTMI_OTGDISABLE     (1 << 21)
#define UTMI_VBUSVLDEXTSEL  (1 << 20)
#define UTMI_VBUSVLDEXT     (1 << 19)

/* Mentor MUSB HDRC core register offsets from core_base.
 * USB1 proxy space: USBSS+0x1C10 to 0x1C1F (INDEX at USBSS+0x1C0E) — TRM §16.2.5.
 * Non-indexed EP0 space: USBSS+0x1D00 to 0x1D0F (USB1). */
#define CORE_FADDR          0x00
#define CORE_POWER          0x01
#define CORE_INTRTX         0x02
#define CORE_INTRRX         0x04
#define CORE_INTRTXE        0x06
#define CORE_INTRRXE        0x08
#define CORE_INTRUSB        0x0A
#define CORE_INTRUSBE       0x0B
#define CORE_DEVCTL         0x60
#define CORE_TESTMODE       0x0F
#define CORE_INDEX          0x0E
/* EP indexed register block starts at core_base+0x10.
 * musb_regs.h: MUSB_TXMAXP=0x00, MUSB_CSR0=MUSB_TXCSR=0x02 — CSR0 is NOT at block offset 0. */
#define CORE_CSR0           0x12    /* core_base+0x10 (EP block) + 0x02 (TXCSR/CSR0) */
#define CORE_COUNT0         0x18    /* core_base+0x10 + 0x08 (RXCOUNT/COUNT0) */
/* EP0 host-mode only: proxy offset 0x1A/0x1B = EP block byte 0x0A/0x0B.
 * TYPE0 must be set before any EP0 transaction — MUSB uses it for bit-rate selection. */
#define CORE_TYPE0          0x1A
#define CORE_NAKLIMIT0      0x1B
#define CORE_FIFO_EP0       0x20

/* Host RX CSR for indexed EP — TRM §16.2.8.2.2 */
#define CORE_RXCSR          0x16
#define CORE_RXCOUNT        0x18
#define CORE_FIFO_EP(ep)    (0x20 + (ep) * 4)

#define RXCSR_RXPKTRDY      (1 << 0)
#define RXCSR_ERROR         (1 << 2)    /* host mode: data/CRC error */
#define RXCSR_NAKTIMEOUT    (1 << 3)    /* host mode: NAK timeout or data toggle error */
#define RXCSR_FLUSHFIFO     (1 << 4)
#define RXCSR_REQPKT        (1 << 5)    /* H_REQPKT: issue IN token to device */
#define RXCSR_CLRDATATOG    (1 << 7)    /* H_CLRDATATOG: reset data toggle to DATA0 */

/* Non-EP0 RX endpoint configuration registers (indexed proxy, after CORE_INDEX=ep) */
#define CORE_RXMAXP         0x14    /* EP block +0x04: max packet size */
#define CORE_RXTYPE         0x1C    /* EP block +0x0C: host speed/type/target EP */
#define CORE_RXINTERVAL     0x1D    /* EP block +0x0D: polling interval (frames) */

/* RXTYPE bit fields — TRM §16.2.8.2.2.1.1 / §16.2.8.2.3 */
#define RXTYPE_SPEED_FS     (1 << 6)    /* bits [7:6] = 01: full speed */
#define RXTYPE_TYPE_INT     (3 << 4)    /* bits [5:4] = 11: interrupt transfer */

/* Per-EP target address register for host-mode multipoint: RXFUNCADDR.
 * MUSB HDRC "additional registers" at core_base+0x80+8*ep+4 (TRM §16.2.8.2.4.1). */
#define CORE_RXFUNCADDR(ep) (0x80 + (ep) * 8 + 4)

/* USB1AUTOREQ wrapper register — auto-set REQPKT in HOST_RXCSR when RXPKTRDY cleared.
 * USB1AUTOREQ is at USBSS+0x18D0; ctrl_base = USBSS+0x1800 → offset 0xD0.
 * TRM §16.2.8.2.4.1 Table 16-138: bits [2n+1:2n] = 11b → "auto req always" for EPn. */
#define CTRL_AUTOREQ        0xD0

/* POWER bits — TRM §16.2.8.2 */
#define POWER_RESET         (1 << 3)
#define POWER_HSMODE        (1 << 4)
#define POWER_HSENAB        (1 << 5)

/* TYPE0 host speed field — proxy offset 0x1A when INDEX=0.
 * Must match the connected device speed; MUSB rejects transactions
 * if TYPE0 disagrees with POWER[HSMODE].  TRM §16.2.8.2.1. */
#define TYPE0_SPEED_HS      (0 << 6)
#define TYPE0_SPEED_FS      (1 << 6)
#define TYPE0_SPEED_LS      (2 << 6)

/* INTRUSBE bits — TRM §16.2.7 */
#define INTRUSBE_CONNECT    (1 << 4)
#define INTRUSBE_DISCONNECT (1 << 5)
#define INTRUSBE_VBUSERR    (1 << 7)

/* DEVCTL bits — TRM §16.2.7 */
#define DEVCTL_SESSION      (1 << 0)
#define DEVCTL_HOSTMODE     (1 << 2)

/* TESTMODE bits — Mentor MUSBMHDRC */
#define TESTMODE_FORCE_HOST (1 << 7)

/* HOST_CSR0 bits — TRM §16.2.8.2.1 */
#define CSR0_RXPKTRDY       (1 << 0)
#define CSR0_TXPKTRDY       (1 << 1)
#define CSR0_RXSTALL        (1 << 2)
#define CSR0_SETUPPKT       (1 << 3)
#define CSR0_ERROR          (1 << 4)
#define CSR0_REQPKT         (1 << 5)
#define CSR0_STATUSPKT      (1 << 6)
#define CSR0_NAK_TIMEOUT    (1 << 7)
/* Bit 11 — host mode: suppress PING tokens during STATUS phase.
 * Linux musb_host.c always sets H_DIS_PING when writing STATUS phase CSR0.
 * Requires 16-bit write since bit 11 is above the 8-bit boundary. */
#define CSR0_DIS_PING       (1 << 11)

/* Pad control register for USB1_DRVVBUS pin — TRM Ch.9 Table 9-10 offset A34h */
#define CONF_USB1_DRVVBUS   (CTRL_MODULE_BASE + 0xA34)

/* USBSS register offsets from USBSS base — TRM §16.4.1 */
#define USBSS_SYSCONFIG     0x10
#define USBSS_IDLE_NOIDLE   (1 << 2)
#define USBSS_STDBY_NOSTDBY (1 << 4)
#define USBSS_SOFT_RESET    (1 << 0)

#define USB_REQ_GET_DESC    0x06
#define USB_REQ_SET_ADDR    0x05
#define USB_REQ_SET_CFG     0x09
#define USB_DT_DEVICE       0x01
#define USB_DT_CONFIG       0x02
#define USB_CLASS_HID       0x03
#define USB_HID_PROTO_MOUSE 0x02
#define USB_EP0_MAX_PKT     64

enum musb_state {
    MUSB_IDLE,
    MUSB_CONNECTED,
    MUSB_ENUM,
    MUSB_CONFIGURED,
    MUSB_ERROR,
};

struct usb_dev_desc {
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
} __attribute__((packed));

struct usb_cfg_desc {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __attribute__((packed));

struct usb_if_desc {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
} __attribute__((packed));

struct usb_ep_desc {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed));

static struct {
    uint32_t ctrl_base;
    uint32_t core_base;
    uint32_t usbss_base;
    int      irq;
    enum musb_state state;
    uint8_t  dev_addr;
    uint8_t  ep0_maxpkt;
    uint8_t  hid_ep_addr;
    uint8_t  hid_ep_maxpkt;
    uint8_t  hid_ep_interval;
    uint8_t  hid_if_num;
    struct input_dev idev;
} musb;

void usb_hid_mouse_attach(struct input_dev *idev,
                          uint8_t ep_addr, uint8_t maxpkt, uint8_t interval);
void usb_hid_mouse_irq(const void *data, int len);
void usb_hid_mouse_detach(void);

static uint8_t hid_pkt_logged;

static void musb_poll_hid_ep(void)
{
    if (musb.state != MUSB_CONFIGURED || !musb.hid_ep_addr)
        return;

    mmio_write8(musb.core_base + CORE_INDEX, musb.hid_ep_addr);
    uint16_t rxcsr = mmio_read16(musb.core_base + CORE_RXCSR);

    if (!(rxcsr & RXCSR_RXPKTRDY)) {
        if (!hid_pkt_logged)
            pr_info("[MUSB] poll_hid_ep: RXPKTRDY not set, RXCSR=0x%04x\n", rxcsr);
        return;
    }

    /* Check errors before draining FIFO — a flushed FIFO cannot be re-read */
    if (rxcsr & (RXCSR_ERROR | RXCSR_NAKTIMEOUT)) {
        pr_info("[MUSB] poll_hid_ep: error RXCSR=0x%04x, flushing\n", rxcsr);
        mmio_write16(musb.core_base + CORE_RXCSR, RXCSR_FLUSHFIFO);
        /* AUTOREQ may not re-arm after error flush — manually re-issue REQPKT */
        mmio_write16(musb.core_base + CORE_RXCSR, RXCSR_REQPKT);
        return;
    }

    uint16_t count = mmio_read16(musb.core_base + CORE_RXCOUNT);
    if (count > 64) count = 64;

    uint8_t buf[64];
    uint32_t fifo = musb.core_base + CORE_FIFO_EP(musb.hid_ep_addr);
    for (uint16_t i = 0; i < count; i++)
        buf[i] = mmio_read8(fifo);

    if (!hid_pkt_logged) {
        pr_info("[MUSB] first HID pkt: count=%d b0=0x%02x b1=%d b2=%d\n",
                count, buf[0], (int8_t)buf[1], (int8_t)buf[2]);
        hid_pkt_logged = 1;
    }

    /* Clear RxPktRdy — AUTOREQ should re-arm REQPKT automatically.
     * Manual re-issue below is a fallback in case AUTOREQ is inactive. */
    mmio_write16(musb.core_base + CORE_RXCSR, 0);

    /* Fallback: AUTOREQ sets REQPKT when RXPKTRDY is cleared; if it didn't
     * (RXCSR still 0 after the write), set it manually to keep polling alive. */
    uint16_t post = mmio_read16(musb.core_base + CORE_RXCSR);
    if (!(post & RXCSR_REQPKT))
        mmio_write16(musb.core_base + CORE_RXCSR, RXCSR_REQPKT);

    usb_hid_mouse_irq(buf, count);
}

static void musb_fifo_write(const void *buf, int len)
{
    const uint8_t *p = (const uint8_t *)buf;
    for (int i = 0; i < len; i++)
        mmio_write8(musb.core_base + CORE_FIFO_EP0, p[i]);
}

static void musb_fifo_read(void *buf, int len)
{
    uint8_t *p = (uint8_t *)buf;
    for (int i = 0; i < len; i++)
        p[i] = mmio_read8(musb.core_base + CORE_FIFO_EP0);
}

static int musb_wait_tx_done(uint32_t ms)
{
    /* Primary: poll IRQSTAT0[0] (TX_EP_0) — set by wrapper on Mentor INTRTX[0].
     * Secondary: TXPKTRDY cleared means hardware processed the write (fast ACK
     * or IRQ handler already consumed IRQSTAT0 while we were in MUSB_ENUM).
     * Print at i=0 to show initial TXPKTRDY state (was the write accepted?). */
    for (uint32_t i = 0; i < ms; i++) {
        uint32_t stat0 = mmio_read32(musb.ctrl_base + CTRL_IRQSTAT0);
        if (stat0 & 1) {
            mmio_write32(musb.ctrl_base + CTRL_IRQSTAT0, 1);
            return 0;
        }
        uint8_t csr = mmio_read8(musb.core_base + CORE_CSR0);
        if (!(csr & CSR0_TXPKTRDY) && i > 0) return 0;
        if (i == 0 || i == 10 || i == 50 || i == 100)
            pr_info("[MUSB] wait_tx[%d]: CSR0=0x%02x IRQSTAT0=0x%08x DEVCTL=0x%02x\n",
                    i, csr, stat0, mmio_read8(musb.core_base + CORE_DEVCTL));
        delay_ms(1);
    }
    pr_info("[MUSB] wait_tx timeout: CSR0=0x%02x IRQSTAT0=0x%08x\n",
            mmio_read8(musb.core_base + CORE_CSR0),
            mmio_read32(musb.ctrl_base + CTRL_IRQSTAT0));
    return -1;
}

static int musb_wait_rx_ready(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        uint8_t csr = mmio_read8(musb.core_base + CORE_CSR0);
        if (csr & CSR0_RXPKTRDY) return 0;
        if (csr & (CSR0_ERROR | CSR0_RXSTALL | CSR0_NAK_TIMEOUT)) {
            pr_info("[MUSB] wait_rx[%d]: error CSR0=0x%02x\n", i, csr);
            return -1;
        }
        if (i == 0 || i == 10 || i == 100)
            pr_info("[MUSB] wait_rx[%d]: CSR0=0x%02x IRQSTAT0=0x%08x\n",
                    i, csr, mmio_read32(musb.ctrl_base + CTRL_IRQSTAT0));
        delay_ms(1);
    }
    pr_info("[MUSB] wait_rx timeout: CSR0=0x%02x IRQSTAT0=0x%08x\n",
            mmio_read8(musb.core_base + CORE_CSR0),
            mmio_read32(musb.ctrl_base + CTRL_IRQSTAT0));
    return -1;
}

static void musb_csr0_clear_errors(void)
{
    uint8_t csr = mmio_read8(musb.core_base + CORE_CSR0);
    if (csr & (CSR0_ERROR | CSR0_RXSTALL | CSR0_NAK_TIMEOUT))
        mmio_write8(musb.core_base + CORE_CSR0, 0);
}

static int musb_ep0_setup(const void *pkt, int len)
{
    mmio_write8(musb.core_base + CORE_INDEX, 0);
    uint8_t csr = mmio_read8(musb.core_base + CORE_CSR0);
    if (csr & (CSR0_RXPKTRDY | CSR0_TXPKTRDY)) return -EBUSY;

    musb_fifo_write(pkt, len);
    mmio_write32(musb.ctrl_base + CTRL_IRQSTAT0, 1);  /* clear stale EP0 TX */
    mmio_write8(musb.core_base + CORE_CSR0,
                (uint8_t)(CSR0_TXPKTRDY | CSR0_SETUPPKT));
    uint8_t rb = mmio_read8(musb.core_base + CORE_CSR0);
    pr_info("[MUSB] ep0_setup: FADDR=0x%02x wrote 0x%02x rb=0x%02x TYPE0=0x%02x\n",
            mmio_read8(musb.core_base + CORE_FADDR),
            (uint8_t)(CSR0_TXPKTRDY | CSR0_SETUPPKT), rb,
            mmio_read8(musb.core_base + CORE_TYPE0));

    if (musb_wait_tx_done(500) < 0) {
        musb_csr0_clear_errors();
        return -EIO;
    }

    csr = mmio_read8(musb.core_base + CORE_CSR0);
    pr_info("[MUSB] ep0_setup done: CSR0=0x%02x ERR=%d STALL=%d NAK=%d\n",
            csr, !!(csr & CSR0_ERROR), !!(csr & CSR0_RXSTALL),
            !!(csr & CSR0_NAK_TIMEOUT));
    if (csr & (CSR0_ERROR | CSR0_RXSTALL | CSR0_NAK_TIMEOUT)) {
        mmio_write8(musb.core_base + CORE_CSR0, 0);
        return -EIO;
    }
    return 0;
}

static int musb_ep0_in(void *buf, int len)
{
    mmio_write8(musb.core_base + CORE_INDEX, 0);
    mmio_write32(musb.ctrl_base + CTRL_IRQSTAT0, 1);  /* clear stale EP0 TX */
    mmio_write8(musb.core_base + CORE_CSR0, (uint8_t)CSR0_REQPKT);
    uint8_t rb = mmio_read8(musb.core_base + CORE_CSR0);
    pr_info("[MUSB] ep0_in: wrote REQPKT rb=0x%02x TYPE0=0x%02x\n",
            rb, mmio_read8(musb.core_base + CORE_TYPE0));
    if (musb_wait_rx_ready(500) < 0) return -EIO;

    uint8_t csr = mmio_read8(musb.core_base + CORE_CSR0);
    if (csr & (CSR0_ERROR | CSR0_RXSTALL)) {
        mmio_write8(musb.core_base + CORE_CSR0, 0);
        return -EIO;
    }

    uint8_t count = mmio_read8(musb.core_base + CORE_COUNT0);
    int actual = (count < len) ? count : len;
    musb_fifo_read(buf, actual);
    mmio_write8(musb.core_base + CORE_CSR0, 0);
    return actual;
}

static int musb_ep0_status_out(void)
{
    /* DIS_PING required: Linux musb_host.c sets H_DIS_PING on every STATUS write.
     * 16-bit write required: CSR0_DIS_PING is bit 11, above the 8-bit boundary. */
    mmio_write16(musb.core_base + CORE_CSR0,
                 (uint16_t)(CSR0_TXPKTRDY | CSR0_STATUSPKT | CSR0_DIS_PING));
    if (musb_wait_tx_done(500) < 0) return -EIO;

    uint16_t csr = mmio_read16(musb.core_base + CORE_CSR0);
    mmio_write16(musb.core_base + CORE_CSR0, 0);
    return (csr & (CSR0_ERROR | CSR0_RXSTALL)) ? -EIO : 0;
}

static int musb_ep0_status_in(void)
{
    uint16_t pre = mmio_read16(musb.core_base + CORE_CSR0);
    pr_info("[MUSB] status_in start: CSR0=0x%04x\n", pre);

    /* DIS_PING required: Linux musb_host.c sets H_DIS_PING on every STATUS write.
     * 16-bit write required: CSR0_DIS_PING is bit 11, above the 8-bit boundary. */
    mmio_write16(musb.core_base + CORE_CSR0,
                 (uint16_t)(CSR0_REQPKT | CSR0_STATUSPKT | CSR0_DIS_PING));
    if (musb_wait_rx_ready(500) < 0) return -EIO;

    uint16_t csr = mmio_read16(musb.core_base + CORE_CSR0);
    uint8_t  cnt = mmio_read8(musb.core_base + CORE_COUNT0);
    pr_info("[MUSB] status_in rxpktrdy: CSR0=0x%04x COUNT0=%d\n", csr, cnt);
    mmio_write16(musb.core_base + CORE_CSR0, 0);
    uint16_t post = mmio_read16(musb.core_base + CORE_CSR0);
    pr_info("[MUSB] status_in post-clear: CSR0=0x%04x\n", post);
    return (csr & (CSR0_ERROR | CSR0_RXSTALL)) ? -EIO : 0;
}

static int usb_ctrl_read(uint8_t rtype, uint8_t req,
                         uint16_t val, uint16_t idx,
                         void *buf, uint16_t len)
{
    uint8_t setup[8] = {
        rtype, req,
        (uint8_t)(val & 0xFF), (uint8_t)(val >> 8),
        (uint8_t)(idx & 0xFF), (uint8_t)(idx >> 8),
        (uint8_t)(len & 0xFF), (uint8_t)(len >> 8),
    };

    int rc = musb_ep0_setup(setup, 8);
    if (rc < 0) return rc;

    int total = 0;
    uint8_t *dst = (uint8_t *)buf;
    while (total < len) {
        int chunk = musb_ep0_in(dst + total, len - total);
        if (chunk < 0) return chunk;
        total += chunk;
        if (chunk < musb.ep0_maxpkt) break;
    }

    rc = musb_ep0_status_out();
    if (rc < 0) return rc;
    return total;
}

static int usb_ctrl_write(uint8_t rtype, uint8_t req,
                          uint16_t val, uint16_t idx)
{
    uint8_t setup[8] = {
        rtype, req,
        (uint8_t)(val & 0xFF), (uint8_t)(val >> 8),
        (uint8_t)(idx & 0xFF), (uint8_t)(idx >> 8),
        0, 0,
    };

    int rc = musb_ep0_setup(setup, 8);
    if (rc < 0) return rc;
    return musb_ep0_status_in();
}

static int usb_get_descriptor(uint8_t dtype, uint8_t index,
                              void *buf, uint16_t len)
{
    return usb_ctrl_read(0x80, USB_REQ_GET_DESC,
                         (dtype << 8) | index, 0, buf, len);
}

static int usb_set_configuration(uint8_t cfg_val)
{
    return usb_ctrl_write(0x00, USB_REQ_SET_CFG, cfg_val, 0);
}

/* Pattern reference: Linux usb/core/config.c — descriptor walk
 * re-implemented from USB 2.0 spec §9.6 */
static int usb_find_hid_mouse(const uint8_t *buf, int total)
{
    int off = 0;
    while (off + 2 <= total) {
        uint8_t dlen = buf[off];
        uint8_t dtype = buf[off + 1];
        if (dlen < 2 || off + dlen > total) break;

        if (dtype == 0x04) {
            struct usb_if_desc *d = (struct usb_if_desc *)(buf + off);
            if (d->bInterfaceClass == USB_CLASS_HID &&
                d->bInterfaceSubClass == 0x01 &&
                d->bInterfaceProtocol == USB_HID_PROTO_MOUSE)
                return off;
        }
        off += dlen;
    }
    return -1;
}

static int usb_find_int_in_ep(const uint8_t *buf, int total, int if_off)
{
    int off = if_off + buf[if_off];
    int limit = total;
    while (off + 2 <= limit) {
        uint8_t dlen = buf[off];
        uint8_t dtype = buf[off + 1];
        if (dlen < 2 || off + dlen > limit) break;

        if (dtype == 0x05) {
            struct usb_ep_desc *d = (struct usb_ep_desc *)(buf + off);
            if ((d->bEndpointAddress & 0x80) &&
                (d->bmAttributes & 0x03) == 0x03) {
                musb.hid_ep_addr = d->bEndpointAddress & 0x0F;
                musb.hid_ep_maxpkt = d->wMaxPacketSize & 0x3FF;
                musb.hid_ep_interval = d->bInterval;
                return 0;
            }
        }
        off += dlen;
    }
    return -1;
}

static int usb_enumerate(void)
{
    musb.state = MUSB_ENUM;

    uint8_t pwr = mmio_read8(musb.core_base + CORE_POWER);
    mmio_write8(musb.core_base + CORE_POWER, pwr | POWER_RESET);
    delay_ms(50);
    mmio_write8(musb.core_base + CORE_POWER, pwr & ~POWER_RESET);
    delay_ms(10);

    /* Select EP0 and clear any stale CSR0 state left by the OTG bus reset. */
    mmio_write8(musb.core_base + CORE_INDEX, 0);
    mmio_write8(musb.core_base + CORE_CSR0, 0);
    delay_ms(2);

    uint8_t dc_post = mmio_read8(musb.core_base + CORE_DEVCTL);
    uint8_t csr_post = mmio_read8(musb.core_base + CORE_CSR0);
    uint32_t irq_post = mmio_read32(musb.ctrl_base + CTRL_IRQSTAT1);
    pr_info("[MUSB] post-reset: DEVCTL=0x%02x HOSTMODE=%d FSDEV=%d CSR0=0x%02x IRQSTAT1=0x%08x\n",
            dc_post, !!(dc_post & DEVCTL_HOSTMODE), !!(dc_post & (1 << 6)), csr_post, irq_post);

    /* TYPE0 must match the detected device speed before any EP0 transaction.
     * MUSB silently drops TXPKTRDY/REQPKT writes when TYPE0 disagrees with
     * POWER[HSMODE] — both say different speeds → core rejects the request.
     * TRM §16.2.8.2.1: TYPE0 proxy register at core_base+0x1A when INDEX=0. */
    uint8_t type0 = (dc_post & (1 << 5)) ? TYPE0_SPEED_LS :
                    (dc_post & (1 << 6)) ? TYPE0_SPEED_FS :
                                           TYPE0_SPEED_HS;
    mmio_write8(musb.core_base + CORE_INDEX, 0);
    mmio_write8(musb.core_base + CORE_TYPE0, type0);
    mmio_write8(musb.core_base + CORE_NAKLIMIT0, 0);
    pr_info("[MUSB] EP0 TYPE0=0x%02x (rb=0x%02x) NAKLIMIT0=0x%02x\n",
            type0,
            mmio_read8(musb.core_base + CORE_TYPE0),
            mmio_read8(musb.core_base + CORE_NAKLIMIT0));

    mmio_write8(musb.core_base + CORE_FADDR, 0);
    musb.dev_addr = 0;
    musb.ep0_maxpkt = USB_EP0_MAX_PKT;

    uint8_t first[8];
    int rc = usb_get_descriptor(USB_DT_DEVICE, 0, first, 8);
    if (rc < 0) {
        pr_err("[MUSB] GET_DESCRIPTOR (8B) failed: %d\n", rc);
        musb.state = MUSB_ERROR;
        return rc;
    }
    musb.ep0_maxpkt = first[7];
    if (musb.ep0_maxpkt < 8) musb.ep0_maxpkt = 8;

    struct usb_dev_desc dev;
    rc = usb_get_descriptor(USB_DT_DEVICE, 0, &dev, sizeof(dev));
    if (rc < 0) {
        pr_err("[MUSB] GET_DESCRIPTOR (full) failed: %d\n", rc);
        musb.state = MUSB_ERROR;
        return rc;
    }
    pr_info("[MUSB] Device VID=%04x PID=%04x Class=%d\n",
            dev.idVendor, dev.idProduct, dev.bDeviceClass);

    /* SET_ADDRESS skipped: device VID=260d PID=1090 intermittently resets its USB
     * stack after STATUS_IN ACK, becoming unresponsive at both addr=0 and addr=1.
     * Single-device bus — addr=0 is valid for the session lifetime (USB 2.0 §9.4.6). */

    uint8_t cfg[256];
    rc = usb_get_descriptor(USB_DT_CONFIG, 0, cfg, sizeof(cfg));
    if (rc < 0) {
        pr_err("[MUSB] GET_DESCRIPTOR (config) failed: %d\n", rc);
        musb.state = MUSB_ERROR;
        return rc;
    }

    struct usb_cfg_desc *c = (struct usb_cfg_desc *)cfg;
    pr_info("[MUSB] Config: %d iface, total=%dB\n",
            c->bNumInterfaces, c->wTotalLength);

    int if_off = usb_find_hid_mouse(cfg, c->wTotalLength);
    if (if_off < 0) {
        pr_info("[MUSB] No HID mouse found\n");
        musb.state = MUSB_CONFIGURED;
        return 0;
    }

    struct usb_if_desc *hid_iface = (struct usb_if_desc *)(cfg + if_off);
    musb.hid_if_num = hid_iface->bInterfaceNumber;
    pr_info("[MUSB] HID mouse at iface offset %d ifnum=%d\n",
            if_off, musb.hid_if_num);

    rc = usb_set_configuration(c->bConfigurationValue);
    if (rc < 0) {
        pr_err("[MUSB] SET_CONFIGURATION failed: %d\n", rc);
        musb.state = MUSB_ERROR;
        return rc;
    }

    /* HID SET_PROTOCOL: switch mouse to Boot Protocol (3-byte reports).
     * Boot Protocol: byte 0=buttons, byte 1=X(int8), byte 2=Y(int8).
     * Device is sending 7-byte native reports; driver cannot know the
     * report descriptor layout without parsing HID descriptors.
     * Boot Protocol gives a fixed, well-known format — USB HID spec §7.2.6.
     * bmRequestType=0x21 (class, interface, host→device), bRequest=0x0B,
     * wValue=0 (boot), wIndex=interface number. */
    rc = usb_ctrl_write(0x21, 0x0B, 0, musb.hid_if_num);
    if (rc < 0)
        pr_info("[MUSB] SET_PROTOCOL (boot) failed (non-fatal): %d\n", rc);
    else
        pr_info("[MUSB] SET_PROTOCOL: switched to boot protocol\n");

    rc = usb_find_int_in_ep(cfg, c->wTotalLength, if_off);
    if (rc < 0) {
        pr_err("[MUSB] No interrupt IN endpoint\n");
        musb.state = MUSB_ERROR;
        return rc;
    }

    musb.idev = (struct input_dev){
        .name = "usb-hid-mouse",
        .event = NULL,
        .priv = NULL,
    };
    input_register_device(&musb.idev);

    pr_info("[MUSB] HID mouse: EP%d IN maxpkt=%d interval=%d\n",
            musb.hid_ep_addr, musb.hid_ep_maxpkt, musb.hid_ep_interval);

    usb_hid_mouse_attach(&musb.idev, musb.hid_ep_addr,
                         musb.hid_ep_maxpkt, musb.hid_ep_interval);

    mmio_write8(musb.core_base + CORE_INDEX, musb.hid_ep_addr);

    /* Configure EP1 RX for host-mode interrupt IN — TRM §16.2.8.2.3.
     * RXFUNCADDR: target device USB address. Without this MUSB sends IN tokens
     * to addr=0 (reset default) instead of addr=1, device never responds.
     * MUSB HDRC additional regs: core_base+0x80+8*ep+4 (TRM §16.2.8.2.4.1). */
    mmio_write8(musb.core_base + CORE_RXFUNCADDR(musb.hid_ep_addr), musb.dev_addr);
    mmio_write16(musb.core_base + CORE_RXMAXP, musb.hid_ep_maxpkt);
    /* RXTYPE: SPEED=01(FS), PROT=11(interrupt), RENDPN=target EP number. */
    mmio_write8(musb.core_base + CORE_RXTYPE,
                RXTYPE_SPEED_FS | RXTYPE_TYPE_INT | musb.hid_ep_addr);
    mmio_write8(musb.core_base + CORE_RXINTERVAL, musb.hid_ep_interval);
    /* Clear data toggle and flush any stale FIFO before first IN token. */
    mmio_write16(musb.core_base + CORE_RXCSR, RXCSR_CLRDATATOG);
    mmio_write16(musb.core_base + CORE_RXCSR, RXCSR_FLUSHFIFO);
    pr_info("[MUSB] EP%d rx cfg: FUNCADDR=%d RXMAXP=%d TYPE=0x%02x INTERVAL=%d\n",
            musb.hid_ep_addr, musb.dev_addr, musb.hid_ep_maxpkt,
            mmio_read8(musb.core_base + CORE_RXTYPE),
            mmio_read8(musb.core_base + CORE_RXINTERVAL));

    /* Core layer: tell MUSB to assert interrupt when EP receives a packet */
    mmio_write16(musb.core_base + CORE_INTRRXE, (1 << musb.hid_ep_addr));

    /* USB1AUTOREQ: auto-set REQPKT when RXPKTRDY cleared (CPU mode, TRM §16.2.8.2.4.1).
     * Bits [2*ep+1 : 2*ep] = 11b → "auto req always". Eliminates manual REQPKT
     * re-issue in IRQ handler after each received packet. */
    uint32_t autoreq = mmio_read32(musb.ctrl_base + CTRL_AUTOREQ);
    autoreq |= (3u << (musb.hid_ep_addr * 2));
    mmio_write32(musb.ctrl_base + CTRL_AUTOREQ, autoreq);

    /* Wrapper layer: TRM §16.4.2.6 CTRL_IRQSTAT0[31:16] = RX EP[15:0].
     * Without this bit the wrapper blocks the core interrupt before it
     * reaches the CPU — MUSB fires internally but the IRQ handler never runs. */
    uint32_t irqen0 = mmio_read32(musb.ctrl_base + CTRL_IRQENSET0);
    mmio_write32(musb.ctrl_base + CTRL_IRQENSET0,
                 irqen0 | (1u << (16 + musb.hid_ep_addr)));
    pr_info("[MUSB] RX EP%d IRQ enabled: IRQENSET0=0x%08x AUTOREQ=0x%08x\n",
            musb.hid_ep_addr,
            mmio_read32(musb.ctrl_base + CTRL_IRQENSET0),
            mmio_read32(musb.ctrl_base + CTRL_AUTOREQ));

    /* Issue first REQPKT — subsequent tokens auto-issued by AUTOREQ on RXPKTRDY clear. */
    mmio_write16(musb.core_base + CORE_RXCSR, RXCSR_REQPKT);

    musb.state = MUSB_CONFIGURED;
    return 0;
}

static void musb_irq_handler(void *data)
{
    (void)data;

    uint32_t stat1 = mmio_read32(musb.ctrl_base + CTRL_IRQSTAT1);
    uint32_t stat0 = mmio_read32(musb.ctrl_base + CTRL_IRQSTAT0);

    /* Bus reset during enumeration triggers DISCONNECT then CONNECT — guard
     * against re-entering usb_enumerate() while it is already running. */
    if (musb.state == MUSB_ENUM) {
        if (stat1) mmio_write32(musb.ctrl_base + CTRL_IRQSTAT1, stat1);
        if (stat0) mmio_write32(musb.ctrl_base + CTRL_IRQSTAT0, stat0);
        return;
    }

    if (stat1 & IRQ_CONNECT) {
        mmio_write32(musb.ctrl_base + CTRL_IRQSTAT1, IRQ_CONNECT);
        pr_info("[MUSB] device connected\n");
        musb.state = MUSB_CONNECTED;
        usb_enumerate();
    }

    if (stat1 & IRQ_DISCONNECT) {
        mmio_write32(musb.ctrl_base + CTRL_IRQSTAT1, IRQ_DISCONNECT);
        pr_info("[MUSB] device disconnected\n");
        usb_hid_mouse_detach();
        musb.state = MUSB_IDLE;
    }

    /* HID RX interrupt: wrapper bit (16 + ep_addr) set by hardware on packet arrival */
    if (musb.hid_ep_addr &&
        (stat0 & (1u << (16 + musb.hid_ep_addr)))) {
        mmio_write32(musb.ctrl_base + CTRL_IRQSTAT0,
                     1u << (16 + musb.hid_ep_addr));
        if (!hid_pkt_logged)
            pr_info("[MUSB] EP%d RX IRQ fired: IRQSTAT0=0x%08x\n",
                    musb.hid_ep_addr, stat0);
        musb_poll_hid_ep();
    }

    uint32_t ep0_tx_ack = stat0 & 1u;
    if (ep0_tx_ack)
        mmio_write32(musb.ctrl_base + CTRL_IRQSTAT0, ep0_tx_ack);

    uint32_t other = stat1 & ~(IRQ_CONNECT | IRQ_DISCONNECT);
    if (other)
        mmio_write32(musb.ctrl_base + CTRL_IRQSTAT1, other);
}

static int musb_host_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    uint32_t base = mem ? mem->start : OMAP_USB1_CTRL_BASE;

    musb.ctrl_base = base;
    musb.core_base = base + 0x400;
    musb.usbss_base = OMAP_USBSS_BASE;
    musb.irq = irq;
    musb.state = MUSB_IDLE;

    pr_info("[MUSB] probing %s @ 0x%08x irq %d\n", pdev->name, base, irq);

    /* USB1_DRVVBUS pin (ball F15) → Mode 0; output-only, pull disabled.
     * Default reset state is Mode 7 (GPIO) — TRM §9.3.2.
     * Without this, MUSB cannot assert DRVVBUS to the TPS2051B power switch. */
    mmio_write32(CONF_USB1_DRVVBUS, PIN_PULL_DISABLE);
    pr_info("[MUSB] USB1_DRVVBUS pinmux=0x%08x\n",
            mmio_read32(CONF_USB1_DRVVBUS));

    pr_info("[MUSB] Step 1: enable clock + check PER PLL\n");
    uint32_t per_idlest = mmio_read32(0x44E00470);
    uint32_t per_clkdcoldo = mmio_read32(0x44E0047C);
    uint32_t per_clkmode = mmio_read32(0x44E0048C);
    uint32_t per_clksel = mmio_read32(0x44E0049C);
    uint32_t per_div_m2 = mmio_read32(0x44E004AC);
    pr_info("[MUSB]   IDLEST=0x%x CLKDCOLDO=0x%x MODE=0x%x\n",
            per_idlest, per_clkdcoldo, per_clkmode);
    pr_info("[MUSB]   CLKSEL=0x%x DIV_M2=0x%x\n",
            per_clksel, per_div_m2);

    if (per_clkdcoldo == 0) {
        pr_info("[MUSB]   CLKDCOLDO disabled, enabling...\n");
        mmio_write32(0x44E0047C, 0x100);
        delay_ms(10);
        pr_info("[MUSB]   CLKDCOLDO after=0x%08x\n",
                mmio_read32(0x44E0047C));
    }
    mmio_write32(CM_PER_USB0_CLKCTRL, MODULEMODE_ENABLE);
    while ((mmio_read32(CM_PER_USB0_CLKCTRL) & IDLEST_MASK) != IDLEST_FUNCTIONAL);
    pr_info("[MUSB]   CLKCTRL=0x%08x\n", mmio_read32(CM_PER_USB0_CLKCTRL));

    pr_info("[MUSB] Step 1b: USBSS init + no-idle\n");
    uint32_t syscfg = mmio_read32(musb.usbss_base + USBSS_SYSCONFIG);
    pr_info("[MUSB]   SYSCONFIG before=0x%08x\n", syscfg);
    syscfg |= USBSS_SOFT_RESET;
    mmio_write32(musb.usbss_base + USBSS_SYSCONFIG, syscfg);
    delay_ms(100);
    syscfg = mmio_read32(musb.usbss_base + USBSS_SYSCONFIG);
    pr_info("[MUSB]   SYSCONFIG after reset=0x%08x\n", syscfg);
    if (syscfg & USBSS_SOFT_RESET) {
        pr_err("[MUSB] USBSS soft reset timeout\n");
        return -EIO;
    }
    mmio_write32(musb.usbss_base + USBSS_SYSCONFIG,
                 USBSS_IDLE_NOIDLE | USBSS_STDBY_NOSTDBY);
    delay_ms(10);
    pr_info("[MUSB]   SYSCONFIG final=0x%08x\n",
            mmio_read32(musb.usbss_base + USBSS_SYSCONFIG));

    pr_info("[MUSB] Step 1c: soft reset USB1\n");
    mmio_write32(base + CTRL_CTRL, CTRL_SOFT_RESET_ISO | CTRL_SOFT_RESET);
    delay_ms(100);
    uint32_t ctrl_rst = mmio_read32(base + CTRL_CTRL);
    pr_info("[MUSB]   CTRL after reset = 0x%08x\n", ctrl_rst);
    if (ctrl_rst & CTRL_SOFT_RESET) {
        pr_err("[MUSB] soft reset failed\n");
        return -EIO;
    }

    pr_info("[MUSB] Step 2: power up PHY + enable OTG comparators\n");
    uint32_t usb_ctrl = mmio_read32(CTRL_USB_CTRL1);
    pr_info("[MUSB]   CTRL1 before=0x%08x CM_PWRDN=%d OTG_PWRDN=%d\n",
            usb_ctrl, !!(usb_ctrl & USB_CTRL_CM_PWRDN),
            !!(usb_ctrl & USB_CTRL_OTG_PWRDN));
    /* TRM Table 9-32: bit 0=cm_pwrdn, bit 1=otg_pwrdn — clear to power up PHYs.
     * bit 12=gpiomode is 0 from reset (USB mode, not UART); no need to touch.
     * bit 19=otgvdet_en — must be set for OTG VBUS comparators to operate. */
    usb_ctrl &= ~USB_CTRL_CM_PWRDN;
    usb_ctrl &= ~USB_CTRL_OTG_PWRDN;
    usb_ctrl |= USB_CTRL_OTGVDET_EN;
    mmio_write32(CTRL_USB_CTRL1, usb_ctrl);
    delay_ms(10);
    uint32_t usb_ctrl_rb = mmio_read32(CTRL_USB_CTRL1);
    pr_info("[MUSB]   CTRL1 after=0x%08x CM_PWRDN=%d OTG_PWRDN=%d OTGVDET=%d\n",
            usb_ctrl_rb,
            !!(usb_ctrl_rb & USB_CTRL_CM_PWRDN),
            !!(usb_ctrl_rb & USB_CTRL_OTG_PWRDN),
            !!(usb_ctrl_rb & USB_CTRL_OTGVDET_EN));

    pr_info("[MUSB] Step 3: set IDDIG=0 (A-device) BEFORE enabling OTG\n");
    mmio_write32(base + CTRL_MODE, MODE_IDDIG_MUX);
    delay_ms(10);
    pr_info("[MUSB]   CTRL_MODE=0x%08x\n", mmio_read32(base + CTRL_MODE));
    uint32_t utmilb = mmio_read32(base + 0xE4);
    pr_info("[MUSB]   UTMILB=0x%08x IDDIG=%d AVALID=%d VBUSVALID=%d\n",
            utmilb, !!(utmilb & (1 << 11)),
            !!(utmilb & (1 << 8)), !!(utmilb & (1 << 7)));

    uint32_t utmi = mmio_read32(base + CTRL_UTMI);
    pr_info("[MUSB]   UTMI before=0x%08x\n", utmi);
    /* OTGDISABLE cleared so OTG comparators run.
     * VBUSVLDEXTSEL/VBUSVLDEXT intentionally NOT set — letting hardware
     * sense VBUS via analog comparator is required so A_WAIT_VRISE
     * state asserts DRVVBUS before declaring VBUS valid. */
    utmi &= ~UTMI_OTGDISABLE;
    utmi &= ~(UTMI_VBUSVLDEXTSEL | UTMI_VBUSVLDEXT);
    mmio_write32(base + CTRL_UTMI, utmi);
    delay_ms(100);
    pr_info("[MUSB]   UTMI after=0x%08x\n", mmio_read32(base + CTRL_UTMI));

    utmilb = mmio_read32(base + 0xE4);
    pr_info("[MUSB]   UTMILB after OTG en=0x%08x IDDIG=%d AVALID=%d\n",
            utmilb, !!(utmilb & (1 << 11)), !!(utmilb & (1 << 8)));

    mmio_write32(base + CTRL_IRQSTAT0, 0xFFFFFFFF);
    mmio_write32(base + CTRL_IRQSTAT1, 0xFFFFFFFF);
    mmio_write32(base + CTRL_IRQENSET1, IRQ_CONNECT | IRQ_DISCONNECT);
    mmio_write32(base + CTRL_IRQENSET0, 0x00000001);
    /* MUSB core interrupt enables — wrapper CTRL_IRQENSET alone is not enough;
     * the core must also have INTRUSBE set to forward events to the wrapper. */
    mmio_read8(musb.core_base + CORE_INTRUSB);  /* read-to-clear pending */
    mmio_write8(musb.core_base + CORE_INTRUSBE,
                INTRUSBE_CONNECT | INTRUSBE_DISCONNECT | INTRUSBE_VBUSERR);

    pr_info("[MUSB] Step 4: set POWER=HSENAB\n");
    mmio_write8(musb.core_base + CORE_POWER, POWER_HSENAB);
    /* TESTMODE_FORCE_HOST intentionally removed — it bypasses the OTG state
     * machine that drives DRVVBUS during A_WAIT_VRISE. Without DRVVBUS=1
     * the TPS2051B power switch stays off and connected devices get no power. */
    delay_ms(100);
    pr_info("[MUSB]   POWER=0x%02x DEVCTL=0x%02x\n",
            mmio_read8(musb.core_base + CORE_POWER),
            mmio_read8(musb.core_base + CORE_DEVCTL));

    pr_info("[MUSB] Step 5: set SESSION\n");
    uint8_t devctl = mmio_read8(musb.core_base + CORE_DEVCTL);
    pr_info("[MUSB]   DEVCTL before=0x%02x\n", devctl);
    mmio_write8(musb.core_base + CORE_DEVCTL, devctl | DEVCTL_SESSION);

    if (!(mmio_read8(musb.core_base + CORE_DEVCTL) & DEVCTL_SESSION)) {
        pr_err("[MUSB] failed to start session\n");
        return -EIO;
    }

    /* Poll for OTG A_WAIT_VRISE → DRVVBUS asserted by state machine.
     * TRM §24.9.3: state machine drives DRVVBUS when A-device starts session. */
    uint32_t stat = 0;
    int drvvbus_ok = 0;
    for (int i = 0; i < 50; i++) {
        stat = mmio_read32(base + 0x18);
        if (stat & 1) { drvvbus_ok = 1; break; }
        delay_ms(20);
    }
    devctl = mmio_read8(musb.core_base + CORE_DEVCTL);
    pr_info("[MUSB]   DEVCTL=0x%02x SESSION=%d HOSTMODE=%d\n",
            devctl, !!(devctl & DEVCTL_SESSION),
            !!(devctl & DEVCTL_HOSTMODE));
    pr_info("[MUSB]   USB1STAT=0x%08x DRVVBUS=%d\n", stat, stat & 1);
    if (!drvvbus_ok)
        pr_err("[MUSB] DRVVBUS never asserted — TPS2051B not powered, check CTRL1 + pinmux\n");

    if (request_irq(irq, musb_irq_handler, 0, "musb-host", NULL) != 0) {
        pr_err("[MUSB] IRQ registration failed\n");
        return -EIO;
    }
    enable_irq(irq);

    /* Diagnostics: TESTMODE and UTMILB to confirm no test-mode interference.
     * UTMILB bits [28:16] = R-only MUSB→PHY outputs; bits [11:0] = loopback test values.
     * DPPULLDOWN(18)=1, DMPULLDOWN(17)=1, TERMSEL(22)=1, XCVRSEL(23)=01 expected. */
    uint8_t  testmode = mmio_read8(musb.core_base + CORE_TESTMODE);
    uint32_t utmilb_post = mmio_read32(base + 0xE4);
    pr_info("[MUSB]   TESTMODE=0x%02x UTMILB=0x%08x LINESTATE=%d\n",
            testmode, utmilb_post, (utmilb_post >> 2) & 3);
    pr_info("[MUSB]   UTMILB: DPPDN=%d DMDN=%d XCVRSEL=%d TERMSEL=%d DRVVBUS=%d\n",
            !!(utmilb_post & (1 << 18)), !!(utmilb_post & (1 << 17)),
            (utmilb_post >> 23) & 3,
            !!(utmilb_post & (1 << 22)), !!(utmilb_post & (1 << 21)));

    /* One-shot CORE_INTRUSB read (read-to-clear) — any pending events before poll? */
    uint8_t intrusb_snap = mmio_read8(musb.core_base + CORE_INTRUSB);
    pr_info("[MUSB]   CORE_INTRUSB snap=0x%02x (cleared)\n", intrusb_snap);

    /* Step 6: poll for OTG CONNECT interrupt (up to 3s).
     * OTG A-device sequence: A_WAIT_BCON → hardware detects D+ → hardware sends
     * 50ms bus reset → A_HOST → CONNECT interrupt.
     * DEVCTL FSDEV/LSDEV are only valid after CONNECT (A_HOST state), not during
     * A_WAIT_BCON — polling them early always reads 0. */
    pr_info("[MUSB] Step 6: waiting for CONNECT (up to 3s)...\n");
    for (int i = 0; i < 150; i++) {
        uint32_t s1   = mmio_read32(musb.ctrl_base + CTRL_IRQSTAT1);
        uint32_t raw1 = mmio_read32(musb.ctrl_base + CTRL_IRQSTATRAW1);
        uint8_t  dc   = mmio_read8(musb.core_base + CORE_DEVCTL);
        if ((i % 25) == 0)
            pr_info("[MUSB]   [%dms] DEVCTL=0x%02x IRQSTAT1=0x%08x RAW1=0x%08x\n",
                    i * 20, dc, s1, raw1);
        if (s1 & IRQ_CONNECT) {
            pr_info("[MUSB] connect at probe: DEVCTL=0x%02x FSDEV=%d LSDEV=%d\n",
                    dc, !!(dc & (1 << 6)), !!(dc & (1 << 5)));
            mmio_write32(musb.ctrl_base + CTRL_IRQSTAT1, IRQ_CONNECT);
            musb.state = MUSB_CONNECTED;
            usb_enumerate();
            break;
        }
        delay_ms(20);
    }

    pr_info("[MUSB] host mode ready, waiting for device\n");
    return 0;
}

static struct platform_driver musb_host_driver = {
    .drv   = { .name = "musb-host" },
    .probe = musb_host_probe,
};

module_platform_driver(musb_host_driver);
