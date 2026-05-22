/*
 * drivers/usb/musb/omap_usb1.c — AM335x USB1 platform driver
 *
 * Binds the BBB USB1 platform device and verifies its MEM/IRQ resources.
 */

#include "types.h"
#include "mmio.h"
#include "platform_device.h"
#include "nothan/init.h"
#include "nothan/errno.h"
#include "nothan/printk.h"
#include "nothan/kbd_input.h"
#include "timer.h"
#include "sleep.h"
#include "task.h"
#include "usb_kbd_task.h"
#include "mach/control.h"
#include "mach/prcm.h"

#define USB1_CLOCK_TIMEOUT 10000
#define USB1_RESET_TIMEOUT 10000
#define USB1_SESSION_TIMEOUT_MS 200
#define USB1_CONNECT_TIMEOUT_MS 3000
#define USB1_BUS_RESET_MS 50
#define USB1_BUS_RESET_SETTLE_MS 10
#define USB1_EP0_TIMEOUT_MS 500
#define USB1_SET_ADDRESS_SETTLE_MS 2
#define USB1_KBD_POLL_STEP_MS 1
#define USB1_PHY_READY_POLL_MS 20
#define USB1_STOP_AFTER_L4_PHY_POLICY 0
#define USB1_CLKDCOLDO_GATE_CTRL       (1 << 8)
#define USB1_CLKDCOLDO_STATUS          (1 << 9)

#define USB1_TO_USBSS_OFFSET          0x1800
#define USB1_TO_PHY_OFFSET            0x0300
#define USB1_TO_CORE_OFFSET           0x0400

#define USBSS_REVREG                  0x000
#define USBSS_SYSCONFIG               0x010
#define USBSS_SYSCONFIG_USB1_OCP_EN_N (1 << 9)
#define USBSS_SYSCONFIG_PHY1_UTMI_EN_N (1 << 8)
#define USBSS_SYSCONFIG_STANDBY_MASK  (3 << 4)
#define USBSS_SYSCONFIG_STANDBY_SHIFT 4
#define USBSS_SYSCONFIG_IDLE_MASK     (3 << 2)
#define USBSS_SYSCONFIG_IDLE_SHIFT    2
#define USBSS_SYSCONFIG_NO_STANDBY    (1 << 4)
#define USBSS_SYSCONFIG_NO_IDLE       (1 << 2)
#define USBSS_SYSCONFIG_SOFT_RESET    (1 << 0)

#define USB1_REV                      0x000
#define USB1_CTRL                     0x014
#define USB1_STAT                     0x018
#define USB1_IRQSTATRAW1              0x02C
#define USB1_IRQSTAT1                 0x034
#define USB1_IRQENABLESET0            0x038
#define USB1_IRQENABLESET1            0x03C
#define USB1_IRQENABLECLR0            0x040
#define USB1_IRQENABLECLR1            0x044
#define USB1_UTMI                     0x0E0
#define USB1_UTMILB                   0x0E4
#define USB1_MODE                     0x0E8
#define USB1_CTRL_SOFT_RESET_ISOLATION (1 << 5)
#define USB1_CTRL_SOFT_RESET          (1 << 0)
#define USB1_STAT_DRVVBUS             (1 << 0)
#define USB1_CORE_INTR_MASK           0x000001F7
#define USB1_CORE_INTR_RESET          (1 << 2)
#define USB1_CORE_INTR_CONNECT        (1 << 4)
#define USB1_CORE_INTR_DISCONNECT     (1 << 5)
#define USB1_CORE_INTR_DRVVBUS        (1 << 8)
#define USB1_CORE_INTR_VBUSERROR      (1 << 7)
#define USB1_EP_INTR_MASK             0xFFFF
#define USB1_UTMI_OTGDISABLE          (1 << 21)
#define USB1_UTMI_FSDATAEXT           (1 << 1)
#define USB1_UTMI_HOST_VALUE          USB1_UTMI_FSDATAEXT
#define USB1_MODE_IDDIG               (1 << 8)
#define USB1_MODE_IDDIG_MUX           (1 << 7)
#define USB1_MODE_ROLE_MASK           (USB1_MODE_IDDIG | USB1_MODE_IDDIG_MUX)
#define USB1_MODE_SOFTWARE_HOST       USB1_MODE_IDDIG_MUX
#define USB1_UTMILB_DRVVBUS           (1 << 21)
#define USB1_UTMILB_IDDIG             (1 << 11)
#define USB1_UTMILB_VBUSVALID         (1 << 7)
#define USB1_UTMILB_LINESTATE_MASK    (3 << 2)
#define USB1_UTMILB_LINESTATE_SHIFT   2

#define USB2PHY_PWR_CNTL              0x18
#define USB2PHY_UTMI_CNTL_1           0x1C
#define USB2PHY_UTMI_CNTL_2           0x20
#define USB2PHYCM_CONFIG              0x3C
#define USB2PHY_PWR_RESET_DONE_VMAIN  (1 << 30)
#define USB2PHY_PWR_VMAIN_DONE        (1 << 29)
#define USB2PHY_PWR_PLLLOCK           (1 << 6)
#define USB2PHY_PWR_FORCELDOON        (1 << 10)
#define USB2PHY_PWR_FORCEPLLON        (1 << 9)
#define USB2PHY_PWR_USEPLLLOCK        (1 << 5)
#define USB2PHY_PWR_USE_PD_REG        (1 << 2)
#define USB2PHY_PWR_PD                (1 << 1)
#define USB2PHY_UTMI1_OVERRIDESUSRESET (1 << 8)
#define USB2PHY_UTMI1_SUSPENDM        (1 << 7)
#define USB2PHY_UTMI1_UTMIRESET       (1 << 6)
#define USB2PHY_UTMI_RESET_DONE       (1 << 20)
#define USB2PHY_UTMI2_TXREADY         (1 << 21)
#define USB2PHY_UTMI2_LINESTATE_MASK  (3 << 26)
#define USB2PHY_UTMI2_LINESTATE_SHIFT 26
#define USB2PHY_UTMI2_HOSTDISCON      (1 << 28)
#define USB2PHYCM_LDOSTATUS_MASK      0x3
#define USB2PHYCM_CMSTATUS_MASK       (0x3F << 18)
#define USB2PHYCM_CMSTATUS_SHIFT      18

#define CTRL_USB_CTRL1                (CTRL_MODULE_BASE + 0x628)
#define CTRL_USB_STS1                 (CTRL_MODULE_BASE + 0x62C)
#define CTRL_USB_WKUP_CTRL            (CTRL_MODULE_BASE + 0x648)
#define CONF_USB1_DRVVBUS             (CTRL_MODULE_BASE + 0xA34)
#define USB_CTRL_RESERVED_HIGH_MASK   (0xFF << 24)
#define USB_CTRL_RESERVED_HIGH_VALUE  (0x3C << 24)
#define USB_CTRL_DATAPOLARITY_INV     (1 << 23)
#define USB_CTRL_OTGSESSEND_EN        (1 << 20)
#define USB_CTRL_OTGVDET_EN           (1 << 19)
#define USB_CTRL_DMGPIO_PD            (1 << 18)
#define USB_CTRL_DPGPIO_PD            (1 << 17)
#define USB_CTRL_GPIO_SIG_CROSS       (1 << 14)
#define USB_CTRL_GPIO_SIG_INV         (1 << 13)
#define USB_CTRL_GPIOMODE             (1 << 12)
#define USB_CTRL_CDET_EXTCTL          (1 << 10)
#define USB_CTRL_DPPULLUP             (1 << 9)
#define USB_CTRL_DMPULLDN             (1 << 8)
#define USB_CTRL_CHGDET_DIS           (1 << 2)
#define USB_CTRL_OTG_PWRDN            (1 << 1)
#define USB_CTRL_CM_PWRDN             (1 << 0)
#define USB_STS_CHGDETSTS_MASK        (7 << 5)
#define USB_STS_CHGDETSTS_SHIFT       5
#define USB_STS_CDET_DMDET            (1 << 4)
#define USB_STS_CDET_DPDET            (1 << 3)
#define USB_STS_CDET_DATADET          (1 << 2)
#define USB_STS_CHGDETECT             (1 << 1)
#define USB_STS_CHGDETDONE            (1 << 0)
#define USB_WKUP_PHY1_WUEN            (1 << 8)
#define USB1_DRVVBUS_PAD_MODE         0x08
#define USB1_DRVVBUS_PAD_MMODE_MASK   0x07

#define MUSB_POWER                    0x01
#define MUSB_FADDR                    0x00
#define MUSB_INTRTX                   0x02
#define MUSB_INTRTXE                  0x06
#define MUSB_INTRRXE                  0x08
#define MUSB_INTRUSB                  0x0A
#define MUSB_INTRUSBE                 0x0B
#define MUSB_INDEX                    0x0E
#define MUSB_TESTMODE                 0x0F
#define MUSB_EP0_FIFO                 0x20
#define MUSB_INDEXED_EP_BASE          0x10
#define MUSB_BUSCTL_BASE              0x80
#define MUSB_BUSCTL_STRIDE            0x08
#define MUSB_TXFUNCADDR               0x00
#define MUSB_TXHUBADDR                0x02
#define MUSB_TXHUBPORT                0x03
#define MUSB_RXFUNCADDR               0x04
#define MUSB_RXHUBADDR                0x06
#define MUSB_RXHUBPORT                0x07
#define MUSB_CSR0                     0x02
#define MUSB_RXMAXP                   0x04
#define MUSB_RXCSR                    0x06
#define MUSB_COUNT0                   0x08
#define MUSB_RXCOUNT                  0x08
#define MUSB_TYPE0                    0x0A
#define MUSB_NAKLIMIT0                0x0B
#define MUSB_RXTYPE                   0x0C
#define MUSB_RXINTERVAL               0x0D
#define MUSB_DEVCTL                   0x60
#define MUSB_BABBLE_CTL               0x61
#define MUSB_POWER_ISOUPDATE          0x80
#define MUSB_POWER_SOFTCONN           0x40
#define MUSB_POWER_HSENAB             0x20
#define MUSB_POWER_HSMODE             0x10
#define MUSB_POWER_RESET              0x08
#define MUSB_POWER_RESUME             0x04
#define MUSB_POWER_SUSPENDM           0x02
#define MUSB_POWER_ENSUSPEND          0x01
#define MUSB_BABBLE_SW_SESSION_CTRL   0x40
#define MUSB_BABBLE_RCV_DISABLE       0x04
#define MUSB_INTRTXE_EP0              (1 << 0)
#define MUSB_INTRRXE_RX_EP_MASK       0xFFFE
#define MUSB_INTRUSBE_MASK            0xF7
#define MUSB_DEVCTL_BDEVICE           (1 << 7)
#define MUSB_DEVCTL_VBUS_MASK         (3 << 3)
#define MUSB_DEVCTL_VBUS_SHIFT        3
#define MUSB_DEVCTL_VBUS_VALID        (3 << MUSB_DEVCTL_VBUS_SHIFT)
#define MUSB_DEVCTL_HM                (1 << 2)
#define MUSB_DEVCTL_SESSION           (1 << 0)
#define MUSB_INTRUSB_CONNECT          (1 << 4)
#define MUSB_INTRUSB_DISCONNECT       (1 << 5)
#define MUSB_INTRUSB_RESET            (1 << 2)
#define MUSB_INTRUSB_VBUSERROR        (1 << 7)
#define MUSB_DEVCTL_FSDEV             (1 << 6)
#define MUSB_DEVCTL_LSDEV             (1 << 5)
#define MUSB_CSR0_FLUSHFIFO           0x0100
#define MUSB_CSR0_H_NAKTIMEOUT        0x0080
#define MUSB_CSR0_H_STATUSPKT         0x0040
#define MUSB_CSR0_H_REQPKT            0x0020
#define MUSB_CSR0_H_ERROR             0x0010
#define MUSB_CSR0_H_SETUPPKT          0x0008
#define MUSB_CSR0_H_RXSTALL           0x0004
#define MUSB_CSR0_TXPKTRDY            0x0002
#define MUSB_CSR0_RXPKTRDY            0x0001
#define MUSB_CSR0_H_DIS_PING          0x0800
#define MUSB_CSR0_H_ERROR_BITS        \
    (MUSB_CSR0_H_RXSTALL | MUSB_CSR0_H_ERROR | MUSB_CSR0_H_NAKTIMEOUT)
#define MUSB_RXCSR_CLRDATATOG         0x0080
#define MUSB_RXCSR_H_RXSTALL          0x0040
#define MUSB_RXCSR_H_REQPKT           0x0020
#define MUSB_RXCSR_FLUSHFIFO          0x0010
#define MUSB_RXCSR_DATAERROR          0x0008
#define MUSB_RXCSR_H_ERROR            0x0004
#define MUSB_RXCSR_RXPKTRDY           0x0001
#define MUSB_RXCSR_H_ERROR_BITS       \
    (MUSB_RXCSR_H_RXSTALL | MUSB_RXCSR_H_ERROR | MUSB_RXCSR_DATAERROR)
#define MUSB_TYPE_SPEED_HIGH          0x40
#define MUSB_TYPE_SPEED_FULL          0x80
#define MUSB_TYPE_SPEED_LOW           0xC0
#define MUSB_TYPE_PROTO_INTR          0x30
#define USB_REQ_GET_DESCRIPTOR        0x06
#define USB_REQ_SET_ADDRESS           0x05
#define USB_REQ_SET_CONFIGURATION     0x09
#define USB_REQ_SET_IDLE              0x0A
#define USB_REQ_SET_PROTOCOL          0x0B
#define USB_DIR_OUT                   0x00
#define USB_DIR_IN                    0x80
#define USB_TYPE_CLASS                0x20
#define USB_RECIP_INTERFACE           0x01
#define USB_DT_DEVICE                 0x01
#define USB_DT_CONFIG                 0x02
#define USB_DT_INTERFACE              0x04
#define USB_DT_ENDPOINT               0x05
#define USB_EP0_DESCRIPTOR8_LEN       8
#define USB_DEVICE_DESCRIPTOR_LEN     18
#define USB_CONFIG_HEAD_LEN           9
#define USB_CONFIG_BUFFER_LEN         128
#define USB_DEVICE_ADDRESS            1
#define USB_CLASS_HID                 0x03
#define USB_HID_SUBCLASS_BOOT         0x01
#define USB_HID_PROTOCOL_KEYBOARD     0x01
#define USB_ENDPOINT_DIR_IN           0x80
#define USB_ENDPOINT_NUMBER_MASK      0x0F
#define USB_ENDPOINT_XFER_MASK        0x03
#define USB_ENDPOINT_XFER_INT         0x03
#define USB_BOOT_PROTOCOL             0x00
#define USB_KBD_REPORT_LEN            8
#define USB_KBD_CTRL_EP               1

#define USB1_UTMILB_LINESTATE(v) \
    (((v) & USB1_UTMILB_LINESTATE_MASK) >> USB1_UTMILB_LINESTATE_SHIFT)
#define USB2PHY_UTMI2_LINESTATE(v) \
    (((v) & USB2PHY_UTMI2_LINESTATE_MASK) >> USB2PHY_UTMI2_LINESTATE_SHIFT)

static int omap_usb1_setup_pads(void)
{
    uint32_t pad;

    mmio_write32(CONF_USB1_DRVVBUS, USB1_DRVVBUS_PAD_MODE);
    pad = mmio_read32(CONF_USB1_DRVVBUS);

    if ((pad & USB1_DRVVBUS_PAD_MMODE_MASK) != 0) {
        pr_err("[USB1] pad setup failed\n");
        return -EIO;
    }

    return 0;
}

static int omap_usb1_disable_phy_wakeup(void)
{
    uint32_t wkup;

    wkup = mmio_read32(CTRL_USB_WKUP_CTRL);
    wkup &= ~USB_WKUP_PHY1_WUEN;
    mmio_write32(CTRL_USB_WKUP_CTRL, wkup);
    wkup = mmio_read32(CTRL_USB_WKUP_CTRL);

    if (wkup & USB_WKUP_PHY1_WUEN) {
        pr_err("[USB1] PHY wakeup setup failed\n");
        return -EIO;
    }

    return 0;
}

static int omap_usb1_enable_clock(void)
{
    uint32_t clkctrl;
    uint32_t clkdcoldo;
    int timeout;

    mmio_write32(CM_CLKDCOLDO_DPLL_PER, USB1_CLKDCOLDO_GATE_CTRL);
    clkdcoldo = mmio_read32(CM_CLKDCOLDO_DPLL_PER);
    if (!(clkdcoldo & USB1_CLKDCOLDO_GATE_CTRL) ||
        !(clkdcoldo & USB1_CLKDCOLDO_STATUS)) {
        pr_err("[USB1] clock setup failed\n");
        return -EIO;
    }

    mmio_write32(CM_PER_USB_OTG_HS_CLKCTRL, MODULEMODE_ENABLE);

    timeout = USB1_CLOCK_TIMEOUT;
    while (timeout--) {
        clkctrl = mmio_read32(CM_PER_USB_OTG_HS_CLKCTRL);
        if ((clkctrl & IDLEST_MASK) == IDLEST_FUNCTIONAL)
            return 0;
    }

    pr_err("[USB1] clock enable timeout\n");
    return -EIO;
}

static int omap_usb1_configure_usbss_power(uint32_t usbss_base)
{
    uint32_t before;
    uint32_t after;
    uint32_t sysconfig;

    before = mmio_read32(usbss_base + USBSS_SYSCONFIG);
    sysconfig = before;
    sysconfig &= ~(USBSS_SYSCONFIG_STANDBY_MASK | USBSS_SYSCONFIG_IDLE_MASK |
                   USBSS_SYSCONFIG_SOFT_RESET);
    sysconfig |= USBSS_SYSCONFIG_NO_STANDBY | USBSS_SYSCONFIG_NO_IDLE;
    mmio_write32(usbss_base + USBSS_SYSCONFIG, sysconfig);
    after = mmio_read32(usbss_base + USBSS_SYSCONFIG);

    if ((after & USBSS_SYSCONFIG_SOFT_RESET) ||
        ((after & USBSS_SYSCONFIG_STANDBY_MASK) !=
         USBSS_SYSCONFIG_NO_STANDBY) ||
        ((after & USBSS_SYSCONFIG_IDLE_MASK) != USBSS_SYSCONFIG_NO_IDLE)) {
        pr_err("[USB1] controller power setup failed\n");
        return -EIO;
    }

    return 0;
}

static int omap_usb1_enable_phy(void)
{
    uint32_t ctrl;
    uint32_t after;

    ctrl = mmio_read32(CTRL_USB_CTRL1);
    ctrl &= ~(USB_CTRL_GPIOMODE | USB_CTRL_OTGVDET_EN |
              USB_CTRL_GPIO_SIG_CROSS | USB_CTRL_GPIO_SIG_INV |
              USB_CTRL_OTG_PWRDN | USB_CTRL_CM_PWRDN);
    ctrl &= ~USB_CTRL_RESERVED_HIGH_MASK;
    ctrl |= USB_CTRL_RESERVED_HIGH_VALUE;
    ctrl |= USB_CTRL_OTGSESSEND_EN;
    mmio_write32(CTRL_USB_CTRL1, ctrl);

    after = mmio_read32(CTRL_USB_CTRL1);
    if ((after & USB_CTRL_RESERVED_HIGH_MASK) != USB_CTRL_RESERVED_HIGH_VALUE ||
        (after & (USB_CTRL_GPIOMODE | USB_CTRL_OTG_PWRDN |
                  USB_CTRL_CM_PWRDN)) ||
        (after & USB_CTRL_OTGVDET_EN) ||
        !(after & USB_CTRL_OTGSESSEND_EN)) {
        pr_err("[USB1] PHY policy setup failed\n");
        return -EIO;
    }

    delay_ms(1);
    return 0;
}

static int omap_usb1_check_usb2phy_power(uint32_t base)
{
    uint32_t phy_base;
    uint32_t pwr;

    phy_base = base + USB1_TO_PHY_OFFSET;
    pwr = mmio_read32(phy_base + USB2PHY_PWR_CNTL);

    if (!(pwr & USB2PHY_PWR_PLLLOCK)) {
        pr_err("[USB1] PHY clock not ready\n");
        return -EIO;
    }
    return 0;
}

static int omap_usb1_power_phy(uint32_t base)
{
    uint32_t phy_base;
    uint32_t after;
    uint32_t pwr;
    int elapsed_ms;

    phy_base = base + USB1_TO_PHY_OFFSET;
    pwr = mmio_read32(phy_base + USB2PHY_PWR_CNTL);
    pwr |= USB2PHY_PWR_FORCELDOON | USB2PHY_PWR_FORCEPLLON |
           USB2PHY_PWR_USEPLLLOCK | USB2PHY_PWR_USE_PD_REG;
    pwr &= ~USB2PHY_PWR_PD;
    mmio_write32(phy_base + USB2PHY_PWR_CNTL, pwr);
    after = mmio_read32(phy_base + USB2PHY_PWR_CNTL);

    if (!(after & USB2PHY_PWR_FORCELDOON) ||
        !(after & USB2PHY_PWR_FORCEPLLON) ||
        !(after & USB2PHY_PWR_USE_PD_REG) ||
        (after & USB2PHY_PWR_PD)) {
        pr_err("[USB1] PHY power setup failed\n");
        return -EIO;
    }

    elapsed_ms = 0;
    while (elapsed_ms < USB1_PHY_READY_POLL_MS) {
        pwr = mmio_read32(phy_base + USB2PHY_PWR_CNTL);
        if ((pwr & USB2PHY_PWR_VMAIN_DONE) &&
            (pwr & USB2PHY_PWR_PLLLOCK)) {
            return 0;
        }
        delay_ms(1);
        elapsed_ms++;
    }

    pwr = mmio_read32(phy_base + USB2PHY_PWR_CNTL);
    pr_err("[USB1] PHY power failed\n");
    return -EIO;
}

static int omap_usb1_init_phy(uint32_t base)
{
    uint32_t phy_base;
    uint32_t before;
    uint32_t after;
    uint32_t utmi1;
    uint32_t utmi2;
    uint32_t pwr;
    int elapsed_ms;

    phy_base = base + USB1_TO_PHY_OFFSET;
    before = mmio_read32(phy_base + USB2PHY_UTMI_CNTL_1);
    utmi1 = before;
    utmi1 |= USB2PHY_UTMI1_OVERRIDESUSRESET | USB2PHY_UTMI1_SUSPENDM;
    utmi1 &= ~USB2PHY_UTMI1_UTMIRESET;
    mmio_write32(phy_base + USB2PHY_UTMI_CNTL_1, utmi1);
    after = mmio_read32(phy_base + USB2PHY_UTMI_CNTL_1);

    if (!(after & USB2PHY_UTMI1_OVERRIDESUSRESET) ||
        !(after & USB2PHY_UTMI1_SUSPENDM) ||
        (after & USB2PHY_UTMI1_UTMIRESET)) {
        pr_err("[USB1] PHY init setup failed\n");
        return -EIO;
    }

    elapsed_ms = 0;
    while (elapsed_ms < USB1_PHY_READY_POLL_MS) {
        pwr = mmio_read32(phy_base + USB2PHY_PWR_CNTL);
        utmi2 = mmio_read32(phy_base + USB2PHY_UTMI_CNTL_2);
        if ((pwr & USB2PHY_PWR_VMAIN_DONE) &&
            (pwr & USB2PHY_PWR_RESET_DONE_VMAIN) &&
            (utmi2 & USB2PHY_UTMI_RESET_DONE)) {
            return 0;
        }

        delay_ms(1);
        elapsed_ms++;
    }

    pwr = mmio_read32(phy_base + USB2PHY_PWR_CNTL);
    utmi2 = mmio_read32(phy_base + USB2PHY_UTMI_CNTL_2);
    pr_err("[USB1] PHY init failed\n");
    return -EIO;
}

static int omap_usb1_check_usbss(uint32_t usbss_base)
{
    uint32_t rev;

    rev = mmio_read32(usbss_base + USBSS_REVREG);
    if (!rev) {
        pr_err("[USB1] controller not accessible\n");
        return -EIO;
    }
    return 0;
}

static int omap_usb1_reset_port(uint32_t base)
{
    uint32_t ctrl;
    int timeout;

    ctrl = mmio_read32(base + USB1_CTRL);
    ctrl |= USB1_CTRL_SOFT_RESET_ISOLATION | USB1_CTRL_SOFT_RESET;
    mmio_write32(base + USB1_CTRL, ctrl);

    timeout = USB1_RESET_TIMEOUT;
    while (timeout--) {
        ctrl = mmio_read32(base + USB1_CTRL);
        if (!(ctrl & (USB1_CTRL_SOFT_RESET_ISOLATION | USB1_CTRL_SOFT_RESET))) {
            return 0;
        }
    }

    pr_err("[USB1] controller reset timeout\n");
    return -EIO;
}

static int omap_usb1_set_host_role(uint32_t base, const char *tag)
{
    uint32_t mode;
    uint32_t utmi;

    (void)tag;

    mode = mmio_read32(base + USB1_MODE);
    mode &= ~USB1_MODE_IDDIG;
    mode |= USB1_MODE_IDDIG_MUX;
    mmio_write32(base + USB1_MODE, mode);
    mode = mmio_read32(base + USB1_MODE);

    if ((mode & USB1_MODE_ROLE_MASK) != USB1_MODE_SOFTWARE_HOST) {
        pr_err("[USB1] host role setup failed\n");
        return -EIO;
    }

    mmio_write32(base + USB1_UTMI, USB1_UTMI_HOST_VALUE);
    utmi = mmio_read32(base + USB1_UTMI);
    if ((utmi & USB1_UTMI_OTGDISABLE) ||
        !(utmi & USB1_UTMI_FSDATAEXT)) {
        pr_err("[USB1] host PHY setup failed\n");
        return -EIO;
    }

    return 0;
}

static void omap_usb1_idle_core(uint32_t base)
{
    uint32_t core_base;

    core_base = base + USB1_TO_CORE_OFFSET;

    mmio_write32(base + USB1_IRQENABLECLR0, USB1_EP_INTR_MASK);
    mmio_write32(base + USB1_IRQENABLECLR1, USB1_CORE_INTR_MASK);
    mmio_write16(core_base + MUSB_INTRTXE, 0);
    mmio_write16(core_base + MUSB_INTRRXE, 0);
    mmio_write8(core_base + MUSB_INTRUSBE, 0);
    mmio_write8(core_base + MUSB_DEVCTL, 0);
}

static void omap_usb1_enable_core_irqs(uint32_t base)
{
    mmio_write32(base + USB1_IRQENABLESET1, USB1_CORE_INTR_MASK);
}

static void omap_usb1_enable_musb_irqs(uint32_t core_base)
{
    mmio_write16(core_base + MUSB_INTRTXE, MUSB_INTRTXE_EP0);
    mmio_write16(core_base + MUSB_INTRRXE, MUSB_INTRRXE_RX_EP_MASK);
    mmio_write8(core_base + MUSB_INTRUSBE, MUSB_INTRUSBE_MASK);
}

static int omap_usb1_enable_babble_session_control(uint32_t core_base)
{
    uint8_t ctl;
    uint8_t after;

    ctl = mmio_read8(core_base + MUSB_BABBLE_CTL);

    if (!(ctl & MUSB_BABBLE_RCV_DISABLE)) {
        return 0;
    }

    mmio_write8(core_base + MUSB_BABBLE_CTL,
                ctl | MUSB_BABBLE_SW_SESSION_CTRL);
    after = mmio_read8(core_base + MUSB_BABBLE_CTL);
    if (!(after & MUSB_BABBLE_SW_SESSION_CTRL)) {
        pr_err("[USB1] session setup failed\n");
        return -EIO;
    }
    return 0;
}

static int omap_usb1_start_host_session(uint32_t base)
{
    uint32_t core_base;
    uint32_t mode;
    uint32_t stat;
    uint8_t devctl;
    uint8_t power;
    int elapsed_ms;

    core_base = base + USB1_TO_CORE_OFFSET;

    mode = mmio_read32(base + USB1_MODE);
    omap_usb1_enable_musb_irqs(core_base);
    omap_usb1_enable_core_irqs(base);

    if (omap_usb1_enable_babble_session_control(core_base))
        return -EIO;

    mmio_write8(core_base + MUSB_TESTMODE, 0);
    mmio_write8(core_base + MUSB_POWER,
                MUSB_POWER_ISOUPDATE | MUSB_POWER_HSENAB);
    power = mmio_read8(core_base + MUSB_POWER);
    if ((power & MUSB_POWER_HSENAB) != MUSB_POWER_HSENAB) {
        pr_err("[USB1] power setup failed\n");
        return -EIO;
    }

    mmio_write8(core_base + MUSB_DEVCTL, 0);

    if ((mode & USB1_MODE_ROLE_MASK) != USB1_MODE_SOFTWARE_HOST) {
        if (omap_usb1_set_host_role(base, "pre-session"))
            return -EIO;

        mode = mmio_read32(base + USB1_MODE);

        if ((mode & USB1_MODE_ROLE_MASK) != USB1_MODE_SOFTWARE_HOST) {
            pr_err("[USB1] host role setup failed\n");
            return -EIO;
        }
    }
    mmio_write8(core_base + MUSB_DEVCTL, MUSB_DEVCTL_SESSION);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_SESSION_TIMEOUT_MS) {
        devctl = mmio_read8(core_base + MUSB_DEVCTL);
        stat = mmio_read32(base + USB1_STAT);
        if ((devctl & MUSB_DEVCTL_SESSION) &&
            !(devctl & MUSB_DEVCTL_BDEVICE) &&
            ((devctl & MUSB_DEVCTL_VBUS_MASK) == MUSB_DEVCTL_VBUS_VALID) &&
            (stat & USB1_STAT_DRVVBUS)) {
            pr_info("[USB1] host ready\n");
            return 0;
        }
        delay_ms(1);
        elapsed_ms++;
    }
    pr_err("[USB1] host start failed\n");
    return -EIO;
}

static int omap_usb1_wait_device_connect(uint32_t base)
{
    uint32_t raw1;
    uint32_t irq1;
    int elapsed_ms;
    int drvvbus_logged;

    pr_info("[USB1] connect gate\n");

    pr_info("[USB1] connect read irq\n");
    raw1 = mmio_read32(base + USB1_IRQSTATRAW1);
    irq1 = mmio_read32(base + USB1_IRQSTAT1);

    pr_info("[USB1] connect state raw1=0x%x irq1=0x%x\n", raw1, irq1);

    if ((irq1 & USB1_CORE_INTR_CONNECT) ||
        (raw1 & USB1_CORE_INTR_CONNECT)) {
        pr_info("[USB1] keyboard connected\n");
        return 0;
    }

    if (irq1) {
        mmio_write32(base + USB1_IRQSTAT1, irq1);
    }

    pr_info("[USB1] waiting for keyboard\n");

    drvvbus_logged = 0;
    elapsed_ms = 0;
    while (elapsed_ms < USB1_CONNECT_TIMEOUT_MS) {
        raw1 = mmio_read32(base + USB1_IRQSTATRAW1);
        irq1 = mmio_read32(base + USB1_IRQSTAT1);

        if ((irq1 & USB1_CORE_INTR_CONNECT) ||
            (raw1 & USB1_CORE_INTR_CONNECT)) {
            pr_info("[USB1] keyboard connected\n");
            return 0;
        }

        if ((irq1 & USB1_CORE_INTR_VBUSERROR) ||
            (raw1 & USB1_CORE_INTR_VBUSERROR)) {
            pr_err("[USB1] connect failed\n");
            return -EIO;
        }

        if ((irq1 & USB1_CORE_INTR_DISCONNECT) ||
            (raw1 & USB1_CORE_INTR_DISCONNECT)) {
            pr_err("[USB1] keyboard disconnected\n");
            return -EIO;
        }

        if ((irq1 & USB1_CORE_INTR_DRVVBUS) && !drvvbus_logged) {
            mmio_write32(base + USB1_IRQSTAT1, USB1_CORE_INTR_DRVVBUS);
            drvvbus_logged = 1;
        }

        delay_ms(1);
        elapsed_ms++;
    }

    pr_info("[USB1] no keyboard detected\n");
    return -ENODEV;
}

static int omap_usb1_reset_connected_device(uint32_t base)
{
    uint32_t core_base;
    uint32_t raw1;
    uint32_t irq1;
    uint8_t power_before;
    uint8_t power_after;
    uint8_t intrusb;

    core_base = base + USB1_TO_CORE_OFFSET;

    irq1 = mmio_read32(base + USB1_IRQSTAT1);
    if (irq1 & USB1_CORE_INTR_CONNECT) {
        mmio_write32(base + USB1_IRQSTAT1, USB1_CORE_INTR_CONNECT);
    }

    mmio_write8(core_base + MUSB_FADDR, 0);
    power_before = mmio_read8(core_base + MUSB_POWER);
    mmio_write8(core_base + MUSB_POWER, (power_before & 0xF0) |
                MUSB_POWER_RESET);
    power_after = mmio_read8(core_base + MUSB_POWER);
    raw1 = mmio_read32(base + USB1_IRQSTATRAW1);
    irq1 = mmio_read32(base + USB1_IRQSTAT1);

    if (!(power_after & MUSB_POWER_RESET)) {
        pr_err("[USB1] keyboard reset failed\n");
        return -EIO;
    }

    delay_ms(USB1_BUS_RESET_MS);

    power_before = mmio_read8(core_base + MUSB_POWER);
    mmio_write8(core_base + MUSB_POWER, power_before & ~MUSB_POWER_RESET);
    power_after = mmio_read8(core_base + MUSB_POWER);
    intrusb = mmio_read8(core_base + MUSB_INTRUSB);
    raw1 = mmio_read32(base + USB1_IRQSTATRAW1);
    irq1 = mmio_read32(base + USB1_IRQSTAT1);

    delay_ms(USB1_BUS_RESET_SETTLE_MS);

    power_after = mmio_read8(core_base + MUSB_POWER);
    intrusb = mmio_read8(core_base + MUSB_INTRUSB);
    raw1 = mmio_read32(base + USB1_IRQSTATRAW1);
    irq1 = mmio_read32(base + USB1_IRQSTAT1);

    if ((intrusb & MUSB_INTRUSB_VBUSERROR) ||
        (raw1 & USB1_CORE_INTR_VBUSERROR) ||
        (irq1 & USB1_CORE_INTR_VBUSERROR)) {
        pr_err("[USB1] keyboard reset failed\n");
        return -EIO;
    }

    if ((intrusb & MUSB_INTRUSB_DISCONNECT) ||
        (raw1 & USB1_CORE_INTR_DISCONNECT) ||
        (irq1 & USB1_CORE_INTR_DISCONNECT)) {
        pr_err("[USB1] keyboard disconnected\n");
        return -EIO;
    }
    return 0;
}

static uint32_t omap_usb1_ep0_base(uint32_t core_base)
{
    mmio_write8(core_base + MUSB_INDEX, 0);
    return core_base + MUSB_INDEXED_EP_BASE;
}

static uint32_t omap_usb1_busctl(uint8_t epnum, uint16_t offset)
{
    return MUSB_BUSCTL_BASE + (MUSB_BUSCTL_STRIDE * epnum) + offset;
}

static void omap_usb1_ep0_set_target(uint32_t core_base, uint8_t address)
{
    mmio_write8(core_base + MUSB_FADDR, address);
    mmio_write8(core_base + omap_usb1_busctl(0, MUSB_TXFUNCADDR), address);
    mmio_write8(core_base + omap_usb1_busctl(0, MUSB_RXFUNCADDR), address);
    mmio_write8(core_base + omap_usb1_busctl(0, MUSB_TXHUBADDR), 0);
    mmio_write8(core_base + omap_usb1_busctl(0, MUSB_RXHUBADDR), 0);
    mmio_write8(core_base + omap_usb1_busctl(0, MUSB_TXHUBPORT), 0);
    mmio_write8(core_base + omap_usb1_busctl(0, MUSB_RXHUBPORT), 0);
}

static void omap_usb1_fifo_write(uint32_t fifo, const uint8_t *buf,
                                 uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
        mmio_write8(fifo, buf[i]);
}

static void omap_usb1_fifo_read(uint32_t fifo, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
        buf[i] = mmio_read8(fifo);
}

struct usb_kbd_endpoint {
    uint8_t interface_number;
    uint8_t configuration_value;
    uint8_t endpoint_address;
    uint8_t endpoint_number;
    uint8_t interval;
    uint16_t max_packet;
};

struct usb_kbd_runtime {
    uint32_t base;
    uint8_t address;
    struct usb_kbd_endpoint kbd;
    volatile int ready;
    volatile int rx_ready;
};

static struct usb_kbd_runtime usb_kbd_rt;

static uint16_t omap_usb1_get_le16(const uint8_t *buf)
{
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

static uint8_t omap_usb1_core_speed_type(uint32_t core_base)
{
    uint8_t devctl;

    devctl = mmio_read8(core_base + MUSB_DEVCTL);

    if (devctl & MUSB_DEVCTL_LSDEV)
        return MUSB_TYPE_SPEED_LOW;
    if (devctl & MUSB_DEVCTL_FSDEV)
        return MUSB_TYPE_SPEED_FULL;
    return MUSB_TYPE_SPEED_HIGH;
}

static int omap_usb1_kbd_key_in_report(const uint8_t *report, uint8_t code)
{
    int i;

    for (i = 2; i < USB_KBD_REPORT_LEN; i++) {
        if (report[i] == code)
            return 1;
    }

    return 0;
}

static char omap_usb1_kbd_ascii(uint8_t code, int shift)
{
    static const char normal_digits[] = "1234567890";
    static const char shifted_digits[] = "!@#$%^&*()";

    if (code >= 0x04 && code <= 0x1d)
        return (char)((shift ? 'A' : 'a') + code - 0x04);
    if (code >= 0x1e && code <= 0x27)
        return shift ? shifted_digits[code - 0x1e] :
               normal_digits[code - 0x1e];
    if (code == 0x2c)
        return ' ';
    if (code == 0x2b)
        return '\t';

    switch (code) {
    case 0x2d: return shift ? '_' : '-';
    case 0x2e: return shift ? '+' : '=';
    case 0x2f: return shift ? '{' : '[';
    case 0x30: return shift ? '}' : ']';
    case 0x31: return shift ? '|' : '\\';
    case 0x33: return shift ? ':' : ';';
    case 0x34: return shift ? '"' : '\'';
    case 0x35: return shift ? '~' : '`';
    case 0x36: return shift ? '<' : ',';
    case 0x37: return shift ? '>' : '.';
    case 0x38: return shift ? '?' : '/';
    default: break;
    }

    return 0;
}

static void omap_usb1_kbd_push_key(uint8_t code, uint8_t mod)
{
    int shift;
    char ch;

    shift = !!(mod & 0x22);
    ch = omap_usb1_kbd_ascii(code, shift);

    if (ch) {
        (void)kbd_input_publish_char((uint8_t)ch);
        return;
    }

    if (code == 0x28) {
        (void)kbd_input_publish_char('\n');
        return;
    }
    if (code == 0x2a) {
        (void)kbd_input_publish_char(0x08);
        return;
    }
}

static int omap_usb1_ep0_fail_if_error(uint32_t base, uint32_t ep0_base,
                                       const char *tag, int elapsed_ms)
{
    uint16_t csr0;

    (void)base;
    (void)tag;
    (void)elapsed_ms;

    csr0 = mmio_read16(ep0_base + MUSB_CSR0);
    if (!(csr0 & MUSB_CSR0_H_ERROR_BITS))
        return 0;

    pr_err("[USB1] control transfer failed\n");
    return -EIO;
}

static int omap_usb1_ep0_get_device_descriptor8(uint32_t base)
{
    uint32_t core_base;
    uint32_t ep0_base;
    uint32_t fifo;
    uint16_t csr0;
    uint8_t setup[8];
    uint8_t desc[USB_EP0_DESCRIPTOR8_LEN];
    uint8_t count0;
    int elapsed_ms;
    int ret;

    core_base = base + USB1_TO_CORE_OFFSET;
    ep0_base = omap_usb1_ep0_base(core_base);
    fifo = core_base + MUSB_EP0_FIFO;

    setup[0] = USB_DIR_IN;
    setup[1] = USB_REQ_GET_DESCRIPTOR;
    setup[2] = 0;
    setup[3] = USB_DT_DEVICE;
    setup[4] = 0;
    setup[5] = 0;
    setup[6] = USB_EP0_DESCRIPTOR8_LEN;
    setup[7] = 0;

    mmio_write8(core_base + MUSB_INDEX, 0);
    omap_usb1_ep0_set_target(core_base, 0);
    mmio_write8(ep0_base + MUSB_NAKLIMIT0, 0);
    mmio_write8(ep0_base + MUSB_TYPE0, 0);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    omap_usb1_fifo_write(fifo, setup, sizeof(setup));
    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY);

    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "SETUP",
                                          elapsed_ms);
        if (ret)
            return ret;

        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;

        delay_ms(1);
        elapsed_ms++;
    }

    if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
        pr_err("[USB1] device descriptor timeout\n");
        return -EIO;
    }

    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_H_REQPKT);
    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "DATA",
                                          elapsed_ms);
        if (ret)
            return ret;

        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (csr0 & MUSB_CSR0_RXPKTRDY)
            break;

        delay_ms(1);
        elapsed_ms++;
    }

    if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
        pr_err("[USB1] device descriptor timeout\n");
        return -EIO;
    }

    count0 = mmio_read8(ep0_base + MUSB_COUNT0);
    if (count0 < USB_EP0_DESCRIPTOR8_LEN) {
        pr_err("[USB1] short device descriptor\n");
        return -EIO;
    }

    omap_usb1_fifo_read(fifo, desc, USB_EP0_DESCRIPTOR8_LEN);

    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_TXPKTRDY |
                 MUSB_CSR0_H_DIS_PING);
    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "STATUS",
                                          elapsed_ms);
        if (ret)
            return ret;

        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;

        delay_ms(1);
        elapsed_ms++;
    }

    if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
        pr_err("[USB1] device descriptor status timeout\n");
        return -EIO;
    }
    return 0;
}

static int omap_usb1_ep0_set_address(uint32_t base, uint8_t address)
{
    uint32_t core_base;
    uint32_t ep0_base;
    uint32_t fifo;
    uint16_t csr0;
    uint8_t setup[8];
    int elapsed_ms;
    int ret;

    core_base = base + USB1_TO_CORE_OFFSET;
    ep0_base = omap_usb1_ep0_base(core_base);
    fifo = core_base + MUSB_EP0_FIFO;

    setup[0] = 0;
    setup[1] = USB_REQ_SET_ADDRESS;
    setup[2] = address;
    setup[3] = 0;
    setup[4] = 0;
    setup[5] = 0;
    setup[6] = 0;
    setup[7] = 0;

    mmio_write8(core_base + MUSB_INDEX, 0);
    omap_usb1_ep0_set_target(core_base, 0);
    mmio_write8(ep0_base + MUSB_NAKLIMIT0, 0);
    mmio_write8(ep0_base + MUSB_TYPE0, 0);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    omap_usb1_fifo_write(fifo, setup, sizeof(setup));
    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY);

    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "SETADDR-SETUP",
                                          elapsed_ms);
        if (ret)
            return ret;

        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;

        delay_ms(1);
        elapsed_ms++;
    }

    if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
        pr_err("[USB1] set address timeout\n");
        return -EIO;
    }

    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_H_REQPKT);
    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "SETADDR-STATUS",
                                          elapsed_ms);
        if (ret)
            return ret;

        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_H_REQPKT))
            break;

        delay_ms(1);
        elapsed_ms++;
    }

    if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
        pr_err("[USB1] set address timeout\n");
        return -EIO;
    }

    mmio_write16(ep0_base + MUSB_CSR0, 0);
    csr0 = mmio_read16(ep0_base + MUSB_CSR0);
    omap_usb1_ep0_set_target(core_base, address);
    delay_ms(USB1_SET_ADDRESS_SETTLE_MS);

    if (mmio_read8(core_base + MUSB_FADDR) != address ||
        mmio_read8(core_base + omap_usb1_busctl(0, MUSB_TXFUNCADDR)) !=
        address) {
        pr_err("[USB1] set address failed\n");
        return -EIO;
    }

    return 0;
}

static int omap_usb1_ep0_get_full_device_descriptor(uint32_t base,
                                                    uint8_t address)
{
    uint32_t core_base;
    uint32_t ep0_base;
    uint32_t fifo;
    uint16_t csr0;
    uint8_t setup[8];
    uint8_t desc[USB_DEVICE_DESCRIPTOR_LEN];
    uint8_t count0;
    uint8_t packet_len;
    uint8_t remaining;
    uint8_t actual;
    int elapsed_ms;
    int ret;

    core_base = base + USB1_TO_CORE_OFFSET;
    ep0_base = omap_usb1_ep0_base(core_base);
    fifo = core_base + MUSB_EP0_FIFO;

    setup[0] = USB_DIR_IN;
    setup[1] = USB_REQ_GET_DESCRIPTOR;
    setup[2] = 0;
    setup[3] = USB_DT_DEVICE;
    setup[4] = 0;
    setup[5] = 0;
    setup[6] = USB_DEVICE_DESCRIPTOR_LEN;
    setup[7] = 0;

    mmio_write8(core_base + MUSB_INDEX, 0);
    omap_usb1_ep0_set_target(core_base, address);
    mmio_write8(ep0_base + MUSB_NAKLIMIT0, 0);
    mmio_write8(ep0_base + MUSB_TYPE0, 0);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    omap_usb1_fifo_write(fifo, setup, sizeof(setup));
    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY);

    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "FULL-SETUP",
                                          elapsed_ms);
        if (ret)
            return ret;

        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;

        delay_ms(1);
        elapsed_ms++;
    }

    if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
        pr_err("[USB1] full descriptor timeout\n");
        return -EIO;
    }

    actual = 0;
    while (actual < USB_DEVICE_DESCRIPTOR_LEN) {
        mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_H_REQPKT);
        csr0 = mmio_read16(ep0_base + MUSB_CSR0);

        elapsed_ms = 0;
        while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
            ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "FULL-DATA",
                                              elapsed_ms);
            if (ret)
                return ret;

            csr0 = mmio_read16(ep0_base + MUSB_CSR0);
            if (csr0 & MUSB_CSR0_RXPKTRDY)
                break;

            delay_ms(1);
            elapsed_ms++;
        }

        if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
            pr_err("[USB1] full descriptor timeout\n");
            return -EIO;
        }

        count0 = mmio_read8(ep0_base + MUSB_COUNT0);
        remaining = USB_DEVICE_DESCRIPTOR_LEN - actual;
        packet_len = count0;
        if (packet_len > remaining)
            packet_len = remaining;
        if (!packet_len) {
            pr_err("[USB1] empty descriptor packet\n");
            return -EIO;
        }

        omap_usb1_fifo_read(fifo, desc + actual, packet_len);
        actual += packet_len;

        if (count0 < desc[7] || actual == USB_DEVICE_DESCRIPTOR_LEN)
            break;
    }

    if (actual < USB_DEVICE_DESCRIPTOR_LEN) {
        pr_err("[USB1] short full descriptor\n");
        return -EIO;
    }

    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_TXPKTRDY |
                 MUSB_CSR0_H_DIS_PING);
    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, "FULL-STATUS",
                                          elapsed_ms);
        if (ret)
            return ret;

        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;

        delay_ms(1);
        elapsed_ms++;
    }

    if (elapsed_ms == USB1_EP0_TIMEOUT_MS) {
        pr_err("[USB1] full descriptor status timeout\n");
        return -EIO;
    }
    return 0;
}

static int omap_usb1_ep0_control_read(uint32_t base, uint8_t address,
                                      uint16_t value, uint16_t index,
                                      uint16_t length, const char *tag,
                                      uint8_t *data, uint16_t *actual_out)
{
    uint32_t core_base;
    uint32_t ep0_base;
    uint32_t fifo;
    uint16_t csr0;
    uint16_t actual;
    uint8_t setup[8];
    uint8_t count0;
    uint8_t take;
    int elapsed_ms;
    int ret;

    core_base = base + USB1_TO_CORE_OFFSET;
    ep0_base = omap_usb1_ep0_base(core_base);
    fifo = core_base + MUSB_EP0_FIFO;

    setup[0] = USB_DIR_IN;
    setup[1] = USB_REQ_GET_DESCRIPTOR;
    setup[2] = value & 0xff;
    setup[3] = value >> 8;
    setup[4] = index & 0xff;
    setup[5] = index >> 8;
    setup[6] = length & 0xff;
    setup[7] = length >> 8;

    mmio_write8(core_base + MUSB_INDEX, 0);
    omap_usb1_ep0_set_target(core_base, address);
    mmio_write8(ep0_base + MUSB_NAKLIMIT0, 0);
    mmio_write8(ep0_base + MUSB_TYPE0, 0);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    omap_usb1_fifo_write(fifo, setup, sizeof(setup));
    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY);

    csr0 = mmio_read16(ep0_base + MUSB_CSR0);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, tag, elapsed_ms);
        if (ret)
            return ret;
        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;
        delay_ms(1);
        elapsed_ms++;
    }
    if (elapsed_ms == USB1_EP0_TIMEOUT_MS)
        return -EIO;

    actual = 0;
    while (actual < length) {
        mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_H_REQPKT);
        elapsed_ms = 0;
        while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
            ret = omap_usb1_ep0_fail_if_error(base, ep0_base, tag,
                                              elapsed_ms);
            if (ret)
                return ret;
            csr0 = mmio_read16(ep0_base + MUSB_CSR0);
            if (csr0 & MUSB_CSR0_RXPKTRDY)
                break;
            delay_ms(1);
            elapsed_ms++;
        }
        if (elapsed_ms == USB1_EP0_TIMEOUT_MS)
            return -EIO;

        count0 = mmio_read8(ep0_base + MUSB_COUNT0);
        take = count0;
        if (take > length - actual)
            take = length - actual;
        if (!take)
            return -EIO;

        omap_usb1_fifo_read(fifo, data + actual, take);
        actual += take;

        if (count0 < USB_EP0_DESCRIPTOR8_LEN)
            break;
    }

    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_TXPKTRDY |
                 MUSB_CSR0_H_DIS_PING);
    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, tag, elapsed_ms);
        if (ret)
            return ret;
        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;
        delay_ms(1);
        elapsed_ms++;
    }
    if (elapsed_ms == USB1_EP0_TIMEOUT_MS)
        return -EIO;

    *actual_out = actual;
    return 0;
}

static int omap_usb1_ep0_zero_request(uint32_t base, uint8_t address,
                                      uint8_t request_type, uint8_t request,
                                      uint16_t value, uint16_t index,
                                      const char *tag)
{
    uint32_t core_base;
    uint32_t ep0_base;
    uint32_t fifo;
    uint16_t csr0;
    uint8_t setup[8];
    int elapsed_ms;
    int ret;

    core_base = base + USB1_TO_CORE_OFFSET;
    ep0_base = omap_usb1_ep0_base(core_base);
    fifo = core_base + MUSB_EP0_FIFO;

    setup[0] = request_type;
    setup[1] = request;
    setup[2] = value & 0xff;
    setup[3] = value >> 8;
    setup[4] = index & 0xff;
    setup[5] = index >> 8;
    setup[6] = 0;
    setup[7] = 0;

    mmio_write8(core_base + MUSB_INDEX, 0);
    omap_usb1_ep0_set_target(core_base, address);
    mmio_write8(ep0_base + MUSB_NAKLIMIT0, 0);
    mmio_write8(ep0_base + MUSB_TYPE0, 0);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    mmio_write16(ep0_base + MUSB_CSR0, MUSB_CSR0_FLUSHFIFO);
    omap_usb1_fifo_write(fifo, setup, sizeof(setup));
    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_SETUPPKT | MUSB_CSR0_TXPKTRDY);

    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, tag, elapsed_ms);
        if (ret)
            return ret;
        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_TXPKTRDY))
            break;
        delay_ms(1);
        elapsed_ms++;
    }
    if (elapsed_ms == USB1_EP0_TIMEOUT_MS)
        return -EIO;

    mmio_write16(ep0_base + MUSB_CSR0,
                 MUSB_CSR0_H_STATUSPKT | MUSB_CSR0_H_REQPKT);
    elapsed_ms = 0;
    while (elapsed_ms < USB1_EP0_TIMEOUT_MS) {
        ret = omap_usb1_ep0_fail_if_error(base, ep0_base, tag, elapsed_ms);
        if (ret)
            return ret;
        csr0 = mmio_read16(ep0_base + MUSB_CSR0);
        if (!(csr0 & MUSB_CSR0_H_REQPKT))
            break;
        delay_ms(1);
        elapsed_ms++;
    }
    if (elapsed_ms == USB1_EP0_TIMEOUT_MS)
        return -EIO;

    mmio_write16(ep0_base + MUSB_CSR0, 0);
    return 0;
}

static int omap_usb1_read_config_descriptor(uint32_t base, uint8_t address,
                                            struct usb_kbd_endpoint *kbd)
{
    uint8_t head[USB_CONFIG_HEAD_LEN];
    uint8_t cfg[USB_CONFIG_BUFFER_LEN];
    uint16_t actual;
    uint16_t total;
    uint16_t pos;
    uint8_t current_if;
    uint8_t len;
    uint8_t type;
    uint8_t epaddr;
    uint8_t attrs;
    int matched;
    int ret;

    ret = omap_usb1_ep0_control_read(base, address,
                                     (USB_DT_CONFIG << 8), 0,
                                     USB_CONFIG_HEAD_LEN, "CFG9",
                                     head, &actual);
    if (ret)
        return ret;
    if (actual < USB_CONFIG_HEAD_LEN || head[1] != USB_DT_CONFIG)
        return -EIO;

    total = omap_usb1_get_le16(head + 2);
    if (total > USB_CONFIG_BUFFER_LEN)
        total = USB_CONFIG_BUFFER_LEN;

    ret = omap_usb1_ep0_control_read(base, address,
                                     (USB_DT_CONFIG << 8), 0, total,
                                     "CFG", cfg, &actual);
    if (ret)
        return ret;

    kbd->configuration_value = cfg[5];
    kbd->interface_number = 0xff;
    kbd->endpoint_address = 0;
    kbd->endpoint_number = 0;
    kbd->max_packet = 0;
    kbd->interval = 0;

    pos = cfg[0];
    current_if = 0xff;
    matched = 0;
    while (pos + 1 < actual) {
        len = cfg[pos];
        type = cfg[pos + 1];

        if (!len || pos + len > actual)
            break;

        if (type == USB_DT_INTERFACE && len >= 9) {
            current_if = cfg[pos + 2];
            matched = (cfg[pos + 5] == USB_CLASS_HID &&
                       cfg[pos + 6] == USB_HID_SUBCLASS_BOOT &&
                       cfg[pos + 7] == USB_HID_PROTOCOL_KEYBOARD);
            if (matched)
                kbd->interface_number = current_if;
        } else if (type == USB_DT_ENDPOINT && len >= 7 && matched) {
            epaddr = cfg[pos + 2];
            attrs = cfg[pos + 3];
            if ((epaddr & USB_ENDPOINT_DIR_IN) &&
                ((attrs & USB_ENDPOINT_XFER_MASK) ==
                 USB_ENDPOINT_XFER_INT)) {
                kbd->endpoint_address = epaddr;
                kbd->endpoint_number = epaddr & USB_ENDPOINT_NUMBER_MASK;
                kbd->max_packet = omap_usb1_get_le16(cfg + pos + 4);
                kbd->interval = cfg[pos + 6];
                return 0;
            }
        }

        pos += len;
    }

    pr_err("[USB1] no boot keyboard interface\n");
    return -ENODEV;
}

static int omap_usb1_configure_keyboard(uint32_t base, uint8_t address,
                                        struct usb_kbd_endpoint *kbd)
{
    int ret;

    ret = omap_usb1_ep0_zero_request(base, address, USB_DIR_OUT,
                                     USB_REQ_SET_CONFIGURATION,
                                     kbd->configuration_value, 0,
                                     "SETCFG");
    if (ret)
        return ret;

    ret = omap_usb1_ep0_zero_request(base, address,
                                     USB_DIR_OUT | USB_TYPE_CLASS |
                                     USB_RECIP_INTERFACE,
                                     USB_REQ_SET_PROTOCOL,
                                     USB_BOOT_PROTOCOL,
                                     kbd->interface_number,
                                     "HID-SETPROTO");
    if (ret)
        return ret;

    ret = omap_usb1_ep0_zero_request(base, address,
                                     USB_DIR_OUT | USB_TYPE_CLASS |
                                     USB_RECIP_INTERFACE,
                                     USB_REQ_SET_IDLE, 0,
                                     kbd->interface_number,
                                     "HID-SETIDLE");
    if (ret)
        return ret;
    pr_info("[USB1] keyboard ready\n");
    return 0;
}

static void omap_usb1_kbd_setup_rx(uint32_t base, uint8_t address,
                                   const struct usb_kbd_endpoint *kbd)
{
    uint32_t core_base;
    uint32_t ep_base;
    uint8_t type;
    uint16_t maxp;

    core_base = base + USB1_TO_CORE_OFFSET;
    ep_base = core_base + MUSB_INDEXED_EP_BASE;
    maxp = kbd->max_packet;
    if (maxp > USB_KBD_REPORT_LEN)
        maxp = USB_KBD_REPORT_LEN;

    mmio_write8(core_base + MUSB_INDEX, USB_KBD_CTRL_EP);
    mmio_write8(core_base + omap_usb1_busctl(USB_KBD_CTRL_EP,
                                             MUSB_RXFUNCADDR), address);
    mmio_write8(core_base + omap_usb1_busctl(USB_KBD_CTRL_EP,
                                             MUSB_RXHUBADDR), 0);
    mmio_write8(core_base + omap_usb1_busctl(USB_KBD_CTRL_EP,
                                             MUSB_RXHUBPORT), 0);
    type = omap_usb1_core_speed_type(core_base) | MUSB_TYPE_PROTO_INTR |
           kbd->endpoint_number;
    mmio_write16(ep_base + MUSB_RXMAXP, maxp);
    mmio_write8(ep_base + MUSB_RXTYPE, type);
    mmio_write8(ep_base + MUSB_RXINTERVAL, kbd->interval);
    mmio_write16(ep_base + MUSB_RXCSR,
                 MUSB_RXCSR_CLRDATATOG | MUSB_RXCSR_FLUSHFIFO);
    mmio_write16(ep_base + MUSB_RXCSR,
                 MUSB_RXCSR_CLRDATATOG | MUSB_RXCSR_FLUSHFIFO);
    mmio_write16(ep_base + MUSB_RXCSR, 0);
    mmio_write16(core_base + MUSB_INTRRXE,
                 mmio_read16(core_base + MUSB_INTRRXE) |
                 (1 << USB_KBD_CTRL_EP));
}

static int omap_usb1_poll_keyboard_once(uint32_t base, uint8_t address,
                                        const struct usb_kbd_endpoint *kbd,
                                        uint8_t *prev)
{
    uint32_t core_base;
    uint32_t ep_base;
    uint32_t fifo;
    uint8_t report[USB_KBD_REPORT_LEN];
    uint16_t csr;
    uint8_t count;
    int i;

    (void)address;
    (void)kbd;

    core_base = base + USB1_TO_CORE_OFFSET;
    ep_base = core_base + MUSB_INDEXED_EP_BASE;
    fifo = core_base + 0x20 + (USB_KBD_CTRL_EP * 4);

    mmio_write8(core_base + MUSB_INDEX, USB_KBD_CTRL_EP);
    csr = mmio_read16(ep_base + MUSB_RXCSR);
    if (csr & MUSB_RXCSR_H_ERROR_BITS) {
        mmio_write16(ep_base + MUSB_RXCSR,
                     csr & ~(MUSB_RXCSR_H_REQPKT |
                             MUSB_RXCSR_H_ERROR_BITS));
        return -EIO;
    }

    if (!(csr & MUSB_RXCSR_RXPKTRDY)) {
        if (!(csr & MUSB_RXCSR_H_REQPKT))
            mmio_write16(ep_base + MUSB_RXCSR, MUSB_RXCSR_H_REQPKT);
        return 0;
    }

    count = mmio_read8(ep_base + MUSB_RXCOUNT);
    if (count > USB_KBD_REPORT_LEN)
        count = USB_KBD_REPORT_LEN;
    for (i = 0; i < USB_KBD_REPORT_LEN; i++)
        report[i] = 0;
    omap_usb1_fifo_read(fifo, report, count);
    mmio_write16(ep_base + MUSB_RXCSR, 0);

    for (i = 2; i < USB_KBD_REPORT_LEN; i++) {
        if (report[i] && !omap_usb1_kbd_key_in_report(prev, report[i]))
            omap_usb1_kbd_push_key(report[i], report[0]);
    }
    for (i = 0; i < USB_KBD_REPORT_LEN; i++)
        prev[i] = report[i];

    return 0;
}

static int omap_usb1_probe(struct platform_device *pdev)
{
    struct usb_kbd_endpoint kbd;
    struct resource *mem;
    uint32_t base;
    int irq;
    int ret;

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    irq = platform_get_irq(pdev, 0);

    if (!mem)
        return -EINVAL;
    if (irq < 0)
        return -EINVAL;

    base = mem->start;
    pr_info("[USB1] starting keyboard host\n");

    ret = omap_usb1_setup_pads();
    if (ret)
        return ret;

    ret = omap_usb1_disable_phy_wakeup();
    if (ret)
        return ret;

    ret = omap_usb1_enable_clock();
    if (ret)
        return ret;

    ret = omap_usb1_configure_usbss_power(base - USB1_TO_USBSS_OFFSET);
    if (ret)
        return ret;

    ret = omap_usb1_enable_phy();
    if (ret)
        return ret;

    ret = omap_usb1_power_phy(base);
    if (ret)
        return ret;

    ret = omap_usb1_init_phy(base);
    if (ret)
        return ret;

#if USB1_STOP_AFTER_L4_PHY_POLICY
    pr_info("[USB1] PHY policy ready\n");
    return 0;
#endif

    ret = omap_usb1_check_usb2phy_power(base);
    if (ret) {
        pr_err("[USB1] PHY check failed\n");
        return ret;
    }

    ret = omap_usb1_check_usbss(base - USB1_TO_USBSS_OFFSET);
    if (ret)
        return ret;

    omap_usb1_idle_core(base);

    ret = omap_usb1_set_host_role(base, "pre-reset");
    if (ret)
        return ret;

    ret = omap_usb1_reset_port(base);
    if (ret)
        return ret;

    ret = omap_usb1_set_host_role(base, "post-reset");
    if (ret)
        return ret;

    ret = omap_usb1_start_host_session(base);
    if (ret)
        return ret;

    ret = omap_usb1_wait_device_connect(base);
    if (ret == -ENODEV)
        return 0;
    if (ret)
        return ret;

    ret = omap_usb1_reset_connected_device(base);
    if (ret)
        return ret;

    ret = omap_usb1_ep0_get_device_descriptor8(base);
    if (ret)
        return ret;

    ret = omap_usb1_ep0_set_address(base, USB_DEVICE_ADDRESS);
    if (ret)
        return ret;

    ret = omap_usb1_ep0_get_full_device_descriptor(base,
                                                   USB_DEVICE_ADDRESS);
    if (ret)
        return ret;

    ret = omap_usb1_read_config_descriptor(base, USB_DEVICE_ADDRESS, &kbd);
    if (ret)
        return ret;

    ret = omap_usb1_configure_keyboard(base, USB_DEVICE_ADDRESS, &kbd);
    if (ret)
        return ret;

    usb_kbd_rt.base = base;
    usb_kbd_rt.address = USB_DEVICE_ADDRESS;
    usb_kbd_rt.kbd = kbd;
    usb_kbd_rt.rx_ready = 0;
    usb_kbd_rt.ready = 1;
    pr_info("[USB1] keyboard input ready\n");

    return 0;
}

#define USB_KBD_STACK_SIZE 4096

static uint8_t usb_kbd_stack[USB_KBD_STACK_SIZE] __attribute__((aligned(4096)));
static struct task_struct usb_kbd_task_struct;

static void usb_kbd_task_fn(void)
{
    uint8_t prev[USB_KBD_REPORT_LEN];
    uint32_t sleep_ms;
    int i;

    for (i = 0; i < USB_KBD_REPORT_LEN; i++)
        prev[i] = 0;

    while (1) {
        if (!usb_kbd_rt.ready) {
            msleep(100);
            continue;
        }

        if (!usb_kbd_rt.rx_ready) {
            omap_usb1_kbd_setup_rx(usb_kbd_rt.base, usb_kbd_rt.address,
                                   &usb_kbd_rt.kbd);
            usb_kbd_rt.rx_ready = 1;
        }

        (void)omap_usb1_poll_keyboard_once(usb_kbd_rt.base,
                                           usb_kbd_rt.address,
                                           &usb_kbd_rt.kbd,
                                           prev);

        sleep_ms = usb_kbd_rt.kbd.interval;
        if (sleep_ms == 0)
            sleep_ms = 10;
        msleep(sleep_ms);
    }
}

struct task_struct *usb_kbd_get_task(void)
{
    usb_kbd_task_struct.name = "usbkbd";
    task_stack_init(&usb_kbd_task_struct, usb_kbd_task_fn,
                    usb_kbd_stack, USB_KBD_STACK_SIZE);
    return &usb_kbd_task_struct;
}

static struct platform_driver omap_usb1_driver = {
    .drv.name = "omap-usb1",
    .probe    = omap_usb1_probe,
};

module_platform_driver(omap_usb1_driver);
