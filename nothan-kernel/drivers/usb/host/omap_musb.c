/*
 * drivers/usb/host/omap_musb.c — OMAP MUSB Host Controller Driver
 *
 * Provides hardware initialization and interrupt handling for AM335x USBSS.
 */

#include "types.h"
#include "nothan/printk.h"
#include "nothan/init.h"
#include "nothan/errno.h"
#include "nothan/usb.h"
#include "mmio.h"
#include "irq.h"
#include "platform_device.h"
#include "mach/irqs.h"
#include "mach/prcm.h"
#include "mach/memmap.h"

/* --- Control Module Registers --- */
#define USB_CTRL0                   (OMAP_CTRL_MODULE_BASE + 0x620)
#define USB_CTRL1                   (OMAP_CTRL_MODULE_BASE + 0x628)
#define CONF_USB1_DRVVBUS           (OMAP_CTRL_MODULE_BASE + 0xA34)

#define USB_CTRL_CM_PWRDN           (1 << 0)
#define USB_CTRL_OTG_PWRDN          (1 << 1)
#define USB_CTRL_OTG_VDAT_DET_EN    (1 << 19)
#define USB_CTRL_OTG_SESS_END_EN    (1 << 20)

/* --- DSPS Wrapper Registers --- */
#define USBSS_REV                   0x000
#define USBSS_SYSCONFIG_OFFSET      0x010
#define USBSS_IRQ_EOI_OFFSET        0x028
#define USBSS_IRQENABLER_OFFSET     0x02C

#define USB0_REV                    0x1000
#define USB1_REV                    0x1800

#define USB_MODE_OFFSET             0xE8
#define USB_MODE_IDDIG_MUX          (1 << 7)
#define USB_MODE_IDDIG_VAL          (1 << 8)

#define USB_IRQ_STAT_0              0x30
#define USB_IRQ_STAT_1              0x34
#define USB_IRQ_EN_SET_0            0x38
#define USB_IRQ_EN_SET_1            0x3C

#define USBSS_IRQ_USB1              (1 << 1)
#define USBSS_IRQ_EOI_LINE          (1 << 1)
#define WRAPPER_IRQ_EP1_RX          (1 << 17)

/* --- MUSB Core Registers --- */
#define USB0_CORE                   0x1400
#define USB1_CORE                   0x1C00

#define MUSB_FADDR_OFFSET           0x00
#define MUSB_POWER_OFFSET           0x01
#define MUSB_INTRTX_OFFSET          0x02
#define MUSB_INTRRX_OFFSET          0x04
#define MUSB_INTRTXE_OFFSET         0x06
#define MUSB_INTRRXE_OFFSET         0x08
#define MUSB_INTRUSB_OFFSET         0x0A
#define MUSB_INTRUSBE_OFFSET        0x0B
#define MUSB_INDEX_OFFSET           0x0E
#define MUSB_DEVCTL_OFFSET          0x60

/* Indexed EP Registers */
#define MUSB_TXMAXP_OFFSET          0x10
#define MUSB_CSR0_OFFSET            0x12
#define MUSB_RXMAXP_OFFSET          0x14
#define MUSB_RXCSR1_OFFSET          0x16
#define MUSB_TYPE0_OFFSET           0x1A
#define MUSB_RXTYPE_OFFSET          0x1C
#define MUSB_RXINTERVAL_OFFSET      0x1D

/* Flat EP Registers (Multipoint) */
#define MUSB_TXFUNCADDR0_OFFSET     0x80
#define MUSB_RXFUNCADDR0_OFFSET     0x84
#define MUSB_TXFUNCADDR1_OFFSET     0x88
#define MUSB_RXFUNCADDR1_OFFSET     0x8C

/* FIFO Registers */
#define MUSB_FIFO_OFFSET            0x20
#define MUSB_FIFO1_OFFSET           0x24

/* --- Bit Fields --- */
#define MUSB_POWER_RESET            (1 << 3)

#define MUSB_INTR_SUSPEND           (1 << 0)
#define MUSB_INTR_RESUME            (1 << 1)
#define MUSB_INTR_RESET             (1 << 2)
#define MUSB_INTR_BABBLE            (1 << 2)
#define MUSB_INTR_SOF               (1 << 3)
#define MUSB_INTR_CONNECT           (1 << 4)
#define MUSB_INTR_DISCONNECT        (1 << 5)
#define MUSB_INTR_VBUSERROR         (1 << 7)

#define MUSB_DEVCTL_SESSION         (1 << 0)
#define MUSB_DEVCTL_LSDEV           (1 << 5)
#define MUSB_DEVCTL_FSDEV           (1 << 6)

#define MUSB_CSR0_RXPKTRDY          (1 << 0)
#define MUSB_CSR0_TXPKTRDY          (1 << 1)
#define MUSB_CSR0_RXSTALL           (1 << 2)
#define MUSB_CSR0_SETUPPKT          (1 << 3)
#define MUSB_CSR0_ERROR             (1 << 4)
#define MUSB_CSR0_REQPKT            (1 << 5)
#define MUSB_CSR0_STATUSPKT         (1 << 6)
#define MUSB_CSR0_NAK_TIMEOUT       (1 << 7)

#define MUSB_RXCSR1_RXPKTRDY        (1 << 0)
#define MUSB_RXCSR1_REQPKT          (1 << 5)

static void musb_delay(volatile uint32_t count)
{
    while (count--) {
    }
}

static struct usb_device musb_device;
static uint32_t musb_base;
static struct usb_request *ep1_rx_req = NULL;

static void omap_musb_irq_handler(void *dev_id)
{
    uint32_t base = musb_base;
    uint32_t stat0, stat1;
    uint8_t power;

    mmio_read8(base + USB1_CORE + MUSB_INTRUSB_OFFSET);
    mmio_read16(base + USB1_CORE + MUSB_INTRTX_OFFSET);
    mmio_read16(base + USB1_CORE + MUSB_INTRRX_OFFSET);

    stat0 = mmio_read32(base + USB1_REV + USB_IRQ_STAT_0);
    stat1 = mmio_read32(base + USB1_REV + USB_IRQ_STAT_1);

    mmio_write32(base + USB1_REV + USB_IRQ_STAT_0, stat0);
    mmio_write32(base + USB1_REV + USB_IRQ_STAT_1, stat1);

    mmio_write32(base + USBSS_IRQ_EOI_OFFSET, USBSS_IRQ_EOI_LINE);

    if (stat1 & MUSB_INTR_CONNECT) {
        pr_info("[MUSB] Device connected\n");
        
        power = mmio_read8(base + USB1_CORE + MUSB_POWER_OFFSET);
        power |= MUSB_POWER_RESET;
        mmio_write8(base + USB1_CORE + MUSB_POWER_OFFSET, power);
        
        musb_delay(50000);
        
        power &= ~MUSB_POWER_RESET;
        mmio_write8(base + USB1_CORE + MUSB_POWER_OFFSET, power);
        
        usb_enumerate_device(&musb_device);
    }
    
    if (stat0 & WRAPPER_IRQ_EP1_RX) {
        uint16_t rxcsr;
        
        mmio_write8(base + USB1_CORE + MUSB_INDEX_OFFSET, 1);
        rxcsr = mmio_read16(base + USB1_CORE + MUSB_RXCSR1_OFFSET);
        
        if (rxcsr & MUSB_RXCSR1_RXPKTRDY) {
            if (ep1_rx_req) {
                uint8_t *buf = (uint8_t *)ep1_rx_req->buffer;
                int len = ep1_rx_req->length;
                if (len > 8) len = 8;
                
                for (int i = 0; i < len; i++) {
                    buf[i] = mmio_read8(base + USB1_CORE + MUSB_FIFO1_OFFSET);
                }
                ep1_rx_req->actual_length = len;
                ep1_rx_req->status = 0;
                
                mmio_write16(base + USB1_CORE + MUSB_RXCSR1_OFFSET, 0);
                
                if (ep1_rx_req->complete) {
                    ep1_rx_req->complete(ep1_rx_req);
                }
            } else {
                mmio_write16(base + USB1_CORE + MUSB_RXCSR1_OFFSET, 0);
            }
        }
    }
    
    if (stat1 & MUSB_INTR_DISCONNECT) {
        uint8_t devctl;
        
        pr_info("[MUSB] Device disconnected\n");
        devctl = mmio_read8(base + USB1_CORE + MUSB_DEVCTL_OFFSET);
        devctl |= MUSB_DEVCTL_SESSION;
        mmio_write8(base + USB1_CORE + MUSB_DEVCTL_OFFSET, devctl);
    }
    
    if (stat1 & MUSB_INTR_VBUSERROR) {
        uint8_t devctl;
        
        pr_err("[MUSB] VBUS error, restarting session\n");
        devctl = mmio_read8(base + USB1_CORE + MUSB_DEVCTL_OFFSET);
        devctl |= MUSB_DEVCTL_SESSION;
        mmio_write8(base + USB1_CORE + MUSB_DEVCTL_OFFSET, devctl);
    }
}

int omap_musb_submit_request(struct usb_request *req)
{
    uint16_t csr0;
    uint8_t *setup_data;
    uint8_t *data_buf;
    int i;
    int timeout;
    int received = 0;
    uint8_t devctl;
    uint8_t speed = 0;

    if (req->endpoint == 0x81) {
        uint16_t rx_irq;
        uint32_t wrapper_irq;

        ep1_rx_req = req;
        
        mmio_write8(musb_base + USB1_CORE + MUSB_TXFUNCADDR1_OFFSET, req->dev->devnum);
        mmio_write8(musb_base + USB1_CORE + MUSB_RXFUNCADDR1_OFFSET, req->dev->devnum);
        mmio_write8(musb_base + USB1_CORE + MUSB_INDEX_OFFSET, 1);
        
        devctl = mmio_read8(musb_base + USB1_CORE + MUSB_DEVCTL_OFFSET);
        if (devctl & MUSB_DEVCTL_LSDEV) speed = 2;
        else if (devctl & MUSB_DEVCTL_FSDEV) speed = 1;
        
        mmio_write8(musb_base + USB1_CORE + MUSB_RXTYPE_OFFSET, (speed << 6) | (3 << 4) | 1);
        mmio_write16(musb_base + USB1_CORE + MUSB_RXMAXP_OFFSET, 8);
        mmio_write8(musb_base + USB1_CORE + MUSB_RXINTERVAL_OFFSET, 10);
        
        rx_irq = mmio_read16(musb_base + USB1_CORE + MUSB_INTRRXE_OFFSET);
        mmio_write16(musb_base + USB1_CORE + MUSB_INTRRXE_OFFSET, rx_irq | (1 << 1));
        
        wrapper_irq = mmio_read32(musb_base + USB1_REV + USB_IRQ_EN_SET_0);
        mmio_write32(musb_base + USB1_REV + USB_IRQ_EN_SET_0, wrapper_irq | WRAPPER_IRQ_EP1_RX);
        
        mmio_write16(musb_base + USB1_CORE + MUSB_RXCSR1_OFFSET, MUSB_RXCSR1_REQPKT);
        return 0;
    }

    if (req->endpoint != 0) {
        pr_err("[MUSB] Unsupported endpoint: 0x%02x\n", req->endpoint);
        return -EINVAL;
    }

    mmio_write8(musb_base + USB1_CORE + MUSB_FADDR_OFFSET, req->dev->devnum);
    
    mmio_write8(musb_base + USB1_CORE + MUSB_TXFUNCADDR0_OFFSET, req->dev->devnum);
    mmio_write8(musb_base + USB1_CORE + MUSB_RXFUNCADDR0_OFFSET, req->dev->devnum);
    
    mmio_write8(musb_base + USB1_CORE + MUSB_INDEX_OFFSET, 0);

    devctl = mmio_read8(musb_base + USB1_CORE + MUSB_DEVCTL_OFFSET);
    if (devctl & MUSB_DEVCTL_LSDEV) {
        speed = 2;
    } else if (devctl & MUSB_DEVCTL_FSDEV) {
        speed = 1;
    }
    mmio_write8(musb_base + USB1_CORE + MUSB_TYPE0_OFFSET, speed << 6);

    for (timeout = 0; timeout < 100000; timeout++) {
        csr0 = mmio_read16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET);
        if (!(csr0 & (MUSB_CSR0_TXPKTRDY | MUSB_CSR0_RXPKTRDY)))
            break;
    }

    setup_data = (uint8_t *)req->setup_packet;
    for (i = 0; i < 8; i++) {
        mmio_write8(musb_base + USB1_CORE + MUSB_FIFO_OFFSET, setup_data[i]);
    }

    mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, MUSB_CSR0_TXPKTRDY | MUSB_CSR0_SETUPPKT);

    for (timeout = 0; timeout < 5000000; timeout++) {
        csr0 = mmio_read16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;
        if (csr0 & MUSB_CSR0_NAK_TIMEOUT) {
            mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, csr0 & ~MUSB_CSR0_NAK_TIMEOUT);
        }
    }

    if (csr0 & MUSB_CSR0_RXSTALL) {
        pr_err("[MUSB] SETUP Stalled!\n");
        mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, 0);
        return -EIO;
    }

    if (req->length > 0 && req->buffer) {
        if (req->setup_packet->bRequestType & 0x80) {
            data_buf = (uint8_t *)req->buffer;
            
            while (received < req->length) {
                mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, MUSB_CSR0_REQPKT);
                
                for (timeout = 0; timeout < 5000000; timeout++) {
                    csr0 = mmio_read16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET);
                    if (csr0 & MUSB_CSR0_RXPKTRDY)
                        break;
                    if (csr0 & MUSB_CSR0_NAK_TIMEOUT) {
                        mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, csr0 & ~MUSB_CSR0_NAK_TIMEOUT);
                    }
                }
                
                if (!(csr0 & MUSB_CSR0_RXPKTRDY)) {
                    pr_err("[MUSB] DATA IN Timeout! CSR0=0x%04x, devctl=0x%02x\n", 
                           csr0, mmio_read8(musb_base + USB1_CORE + MUSB_DEVCTL_OFFSET));
                    return -EIO;
                }
                
                for (i = 0; i < req->dev->ep0_maxpacket && received < req->length; i++) {
                    data_buf[received++] = mmio_read8(musb_base + USB1_CORE + MUSB_FIFO_OFFSET);
                }
                
                mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, 0);
            }
            req->actual_length = received;
        }
    }

    if (req->setup_packet->bRequestType & 0x80) {
        mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, MUSB_CSR0_STATUSPKT | MUSB_CSR0_TXPKTRDY);
        for (timeout = 0; timeout < 500000; timeout++) {
            if (!(mmio_read16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET) & MUSB_CSR0_TXPKTRDY))
                break;
        }
    } else {
        mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, MUSB_CSR0_STATUSPKT | MUSB_CSR0_REQPKT);
        for (timeout = 0; timeout < 500000; timeout++) {
            csr0 = mmio_read16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET);
            if (csr0 & MUSB_CSR0_RXPKTRDY) {
                mmio_write16(musb_base + USB1_CORE + MUSB_CSR0_OFFSET, 0);
                break;
            }
        }
    }

    return 0;
}

static int omap_musb_probe(struct platform_device *pdev)
{
    struct resource *res_mem;
    uint32_t base;
    uint32_t val;
    uint32_t mode;
    uint8_t devctl1;
    int irq;

    res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    if (!res_mem)
        return -EINVAL;

    base = res_mem->start;
    irq = pdev->irq;

    pr_info("[MUSB] probing %s @ 0x%08x irq %d\n", pdev->name, base, irq);

    musb_base = base;

    mmio_write32(CM_PER_L3S_CLKSTCTRL, 0x2);
    while ((mmio_read32(CM_PER_L3S_CLKSTCTRL) & 0x3) != 0x2);

    mmio_write32(CM_PER_USB0_CLKCTRL, MODULEMODE_ENABLE);
    while ((mmio_read32(CM_PER_USB0_CLKCTRL) & 0x30000) != 0);
    mmio_write32(CM_CLKDCOLDO_DPLL_PER, 0x100); 
    while ((mmio_read32(CM_CLKDCOLDO_DPLL_PER) & 0x300) != 0x300); 

    val = mmio_read32(USB_CTRL0);
    val &= ~USB_CTRL_CM_PWRDN;
    val &= ~(0xFF << 24);
    val |= (0x3C << 24);
    mmio_write32(USB_CTRL0, val);
    musb_delay(10000);

    val = mmio_read32(USB_CTRL1);
    val |= (USB_CTRL_CM_PWRDN | USB_CTRL_OTG_PWRDN);
    mmio_write32(USB_CTRL1, val);
    musb_delay(10000);

    val = mmio_read32(USB_CTRL1);
    val &= ~(USB_CTRL_CM_PWRDN | USB_CTRL_OTG_PWRDN);
    val &= ~(0xFF << 24);
    val |= (0x3C << 24);
    mmio_write32(USB_CTRL1, val);
    musb_delay(50000);

    mmio_write32(base + USBSS_SYSCONFIG_OFFSET, 0x01);
    
    /* CRITICAL: Hardware settling time to prevent L3 interconnect Data Abort.
     * The USBSS drops off the OCP bus during soft reset, so immediate read will fault. */
    musb_delay(50000);
    while (mmio_read32(base + USBSS_SYSCONFIG_OFFSET) & 0x01);

    mode = mmio_read32(base + USB1_REV + USB_MODE_OFFSET);
    mode |= USB_MODE_IDDIG_MUX;
    mode &= ~USB_MODE_IDDIG_VAL;
    mmio_write32(base + USB1_REV + USB_MODE_OFFSET, mode);
    
    if ((mmio_read32(base + USB1_REV + USB_MODE_OFFSET) & USB_MODE_IDDIG_MUX) == 0)
        pr_err("[MUSB] Failed to force Host Mode\n");
    
    /* PHY needs time to detect ID pin override and transition to Host mode */
    musb_delay(50000);

    mmio_write32(CONF_USB1_DRVVBUS, 0x00);
    
    val = mmio_read32(USB_CTRL0);
    if (val & USB_CTRL_CM_PWRDN)
        pr_err("[MUSB] PHY power up failed\n");

    devctl1 = mmio_read8(base + USB1_CORE + MUSB_DEVCTL_OFFSET);
    devctl1 |= MUSB_DEVCTL_SESSION;
    mmio_write8(base + USB1_CORE + MUSB_DEVCTL_OFFSET, devctl1);

    pr_info("[MUSB] Host controller initialized\n");

    mmio_write8(base + USB1_CORE + MUSB_INTRUSBE_OFFSET, MUSB_INTR_CONNECT | MUSB_INTR_DISCONNECT | MUSB_INTR_VBUSERROR);
    mmio_write16(base + USB1_CORE + MUSB_INTRTXE_OFFSET, 0x0000);
    mmio_write16(base + USB1_CORE + MUSB_INTRRXE_OFFSET, 0x0000);

    mmio_write32(base + USB1_REV + USB_IRQ_EN_SET_0, 0x00000000);
    mmio_write32(base + USB1_REV + USB_IRQ_EN_SET_1, 
                 (MUSB_INTR_CONNECT | MUSB_INTR_DISCONNECT | MUSB_INTR_VBUSERROR));

    mmio_write32(base + USBSS_IRQENABLER_OFFSET, USBSS_IRQ_USB1);

    if (request_irq(irq, omap_musb_irq_handler, 0, "omap-musb", NULL) < 0)
        return -EIO;

    enable_irq(irq);

    return 0;
}

static int omap_musb_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver omap_musb_driver = {
    .drv = {
        .name = "omap-musb",
    },
    .probe = omap_musb_probe,
    .remove = omap_musb_remove,
};

static int __init omap_musb_driver_init(void)
{
    return platform_driver_register(&omap_musb_driver);
}
subsys_initcall(omap_musb_driver_init);
