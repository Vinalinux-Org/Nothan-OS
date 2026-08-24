/*
 * drivers/usb/musb-hcd.c - AM335x MUSB USB host controller (usb1, BBB Type-A)
 *
 * Minimal PIO host for a single directly-attached device: no hub, no DMA,
 * no gadget/peripheral role. Drives only usb1 (the BeagleBone Black Type-A
 * host connector). The companion usb-core / usb-hid layers sit on top.
 *
 * Hardware map:
 *
 *   USBSS wrapper      PA 0x47400000  (revision, SYSCONFIG/soft-reset, DMA IRQ)
 *   usb1 "control"     PA 0x47401800  (per-controller reset / mode / line IRQs)
 *   usb1 MUSB core     PA 0x47401C00  (POWER, DEVCTL, INTR, indexed EPs, FIFOs)
 *   usb1 integrated PHY PA 0x47401B00
 *   usb1 host IRQ      19
 *   PHY power          Control Module usb_ctrl1  PA 0x44E10628
 *   Module clock       CM_PER_USB0_CLKCTRL       PA 0x44E0001C (gates whole SS)
 *
 * The whole 0x47400000 window is mapped at VA 0xF3000000 (see mmu.c); register
 * access goes through phys_to_mmio(). The control module / PRCM live in the
 * already-mapped L4_WKUP window.
 *
 * Milestone 0 (this file, initial): module clock on, PHY powered, USBSS and
 * usb1 controller taken out of reset, revision registers read back as a sanity
 * check. No device traffic yet.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/config.h>
#include <nothan/types.h>
#include <nothan/mmio.h>
#include <nothan/platform.h>
#include <nothan/irq.h>
#include <nothan/delay.h>
#include <nothan/printk.h>
#include <nothan/init.h>
#include <nothan/sched.h>
#include <nothan/completion.h>
#include <nothan/input.h>

/* ---- PRCM: clocks (L4_WKUP window) ----------------------------------
 * The USB functional clock is CLKDCOLDO (960 MHz) from the PER DPLL, gated
 * separately from the module clock. A bootloader that doesn't use USB leaves
 * it off, so the driver brings the whole chain up itself (matches the AM335x
 * PRCM sequence: CLKDCOLDO output → L3S domain wakeup → module clock).
 */
#define CM_PER_BASE		0x44E00000
#define CM_PER_L3S_CLKSTCTRL	(CM_PER_BASE + 0x04)
#  define CLKTRCTRL_MASK	0x3
#  define CLKTRCTRL_SW_WKUP	0x2
#define CM_PER_USB0_CLKCTRL	(CM_PER_BASE + 0x1C)
#  define MODULEMODE_MASK	0x3
#  define MODULEMODE_ENABLE	0x2
#  define IDLEST_MASK		(0x3 << 16)
#  define IDLEST_FUNC		(0x0 << 16)

#define CM_WKUP_BASE		0x44E00400
#define CM_CLKDCOLDO_DPLL_PER	(CM_WKUP_BASE + 0x7C)
#  define CLKDCOLDO_ENABLE	0x300	/* CLKDCOLDO output enable (USB 960 MHz) */

/* ---- Control Module: USB PHY power (L4_WKUP window) ------------------
 * usb_ctrl0 = 0x44E10620 (usb0), usb_ctrl1 = 0x44E10628 (usb1). */
#define USB_CTRL1		0x44E10628
#  define USBPHY_CM_PWRDN	(1u << 0)	/* 1 = CM  PHY powered down */
#  define USBPHY_OTG_PWRDN	(1u << 1)	/* 1 = OTG PHY powered down */
#  define USBPHY_OTGVDET_EN	(1u << 19)	/* VBUS-detect comparators  */
#  define USBPHY_OTGSESSEND_EN	(1u << 20)	/* session-end comparator   */

/* ---- usb1 per-controller "control" block (offset 0x1800 from USBSS) ----
 * All offsets relative to the control base. USBSS-top SYSCONFIG is left
 * alone — handled at system init, not here. */
#define USB1_CTL_OFF		0x1800
#define USB1_REV		(USB1_CTL_OFF + 0x00)	/* revision (0 = unclocked) */
#define USB1_CTRL		(USB1_CTL_OFF + 0x14)	/* control: soft reset      */
#  define USB1CTRL_RESET	(1u << 0)
#define USB1_STAT		(USB1_CTL_OFF + 0x18)
#  define USB1STAT_DRVVBUS	(1u << 8)
#define USB1_IRQSTAT0		(USB1_CTL_OFF + 0x30)	/* EP TX[15:0]/RX[31:17] */
#define USB1_IRQSTAT1		(USB1_CTL_OFF + 0x34)	/* core/USB line events  */
#define USB1_IRQENSET0		(USB1_CTL_OFF + 0x38)
#define USB1_IRQENSET1		(USB1_CTL_OFF + 0x3C)
#define USB1_IRQENCLR0		(USB1_CTL_OFF + 0x40)
#define USB1_IRQENCLR1		(USB1_CTL_OFF + 0x44)
#define USB1_TXMODE		(USB1_CTL_OFF + 0x70)
#define USB1_RXMODE		(USB1_CTL_OFF + 0x74)
#define USB1_PHY_UTMI		(USB1_CTL_OFF + 0xE0)
#  define UTMI_OTG_DISABLE	(1u << 21)	/* must clear for host mode */
#define USB1_MODE		(USB1_CTL_OFF + 0xE8)
#  define USB1MODE_IDDIG	(1u << 8)	/* 0 = A/host */
#  define USB1MODE_IDDIG_MUX	(1u << 7)	/* 1 = take ID from register */

/* Wrapper interrupt bitmaps (am33xx): the 9 core "usb" interrupts sit in
 * coreintr at bits [8:0] (usb_shift = 0); EP TX bits [15:0], RX bits [31:17]
 * in epintr. */
#define WRAP_USB_BITMAP		0x1FF
#define WRAP_TXEP_BITMAP	0xFFFF
#define WRAP_RXEP_BITMAP	(0xFFFEu << 16)

/* ---- usb1 MUSB core ("mc" block, offset 0x1C00 from USBSS) ---------- */
#define USB1_MC_OFF		0x1C00
#define MC_FADDR		(USB1_MC_OFF + 0x00)	/* 8-bit */
#define MC_POWER		(USB1_MC_OFF + 0x01)	/* 8-bit */
#  define POWER_HSENAB		0x20
#  define POWER_HSMODE		0x10	/* RO: HS handshake succeeded */
#  define POWER_RESET		0x08	/* drive bus reset while set */
#define MC_INTRUSB		(USB1_MC_OFF + 0x0A)	/* 8-bit */
#define MC_INTRUSBE		(USB1_MC_OFF + 0x0B)	/* 8-bit */
#  define INTR_RESET		0x04
#  define INTR_SOF		0x08
#  define INTR_CONNECT		0x10
#  define INTR_DISCONNECT	0x20
#  define INTR_VBUSERROR	0x80
#  define INTRUSBE_HOST		0xF7	/* all core USB ints except SOF */
#define MC_DEVCTL		(USB1_MC_OFF + 0x60)	/* 8-bit */
#  define DEVCTL_SESSION	0x01
#  define DEVCTL_HM		0x04	/* RO: 1 = host mode */
#  define DEVCTL_LSDEV		0x20	/* RO: low-speed device attached */
#  define DEVCTL_FSDEV		0x40	/* RO: full/high-speed device attached */
#define MC_INDEX		(USB1_MC_OFF + 0x0E)	/* 8-bit: select EP for indexed block */
/* Dynamic-FIFO config (per-EP, selected by INDEX). Size field = log2(bytes)-3
 * (64 bytes => 3); address is the FIFO-RAM byte offset >> 3. */
#define MC_RXFIFOSZ		(USB1_MC_OFF + 0x63)	/* 8-bit */
#define MC_RXFIFOADD		(USB1_MC_OFF + 0x66)	/* 16-bit */

/* EP0 registers — indexed block at MC+0x10 (INDEX must be 0). */
#define EP0_CSR0		(USB1_MC_OFF + 0x12)	/* 16-bit (TXCSR reused) */
#define EP0_COUNT0		(USB1_MC_OFF + 0x18)	/* RX byte count (8-bit) */
#define EP0_NAKLIMIT		(USB1_MC_OFF + 0x1B)	/* 8-bit NAK limit */
#define EP0_FIFO		(USB1_MC_OFF + 0x20)	/* EP0 FIFO data port */
#  define CSR0_RXPKTRDY		0x0001
#  define CSR0_TXPKTRDY		0x0002
#  define CSR0_H_RXSTALL	0x0004
#  define CSR0_H_SETUPPKT	0x0008
#  define CSR0_H_ERROR		0x0010
#  define CSR0_H_REQPKT		0x0020
#  define CSR0_H_STATUSPKT	0x0040
#  define CSR0_H_NAKTIMEOUT	0x0080
#  define CSR0_FLUSHFIFO	0x0100
#  define CSR0_H_DIS_PING	0x0800

/* Multipoint "busctl" target-address registers (base 0x80 + 8*epnum). AM335x
 * MUSB runs in multipoint mode — device address lives here, not in FADDR.
 * EP0 = busctl block 0. */
#define EP0_TXFUNCADDR		(USB1_MC_OFF + 0x80)
#define EP0_TXHUBADDR		(USB1_MC_OFF + 0x82)
#define EP0_TXHUBPORT		(USB1_MC_OFF + 0x83)
#define EP0_RXFUNCADDR		(USB1_MC_OFF + 0x84)
#define EP0_RXHUBADDR		(USB1_MC_OFF + 0x86)
#define EP0_RXHUBPORT		(USB1_MC_OFF + 0x87)

/* Hardware EP1 used to poll the device's interrupt-IN endpoint (the touch
 * report endpoint, 0x82). Indexed RX registers (valid when INDEX=1) + the
 * absolute busctl block for EP1 (0x80 + 8*1). */
#define EP1_RXMAXP		(USB1_MC_OFF + 0x14)	/* 16-bit, INDEX=1 */
#define EP1_RXCSR		(USB1_MC_OFF + 0x16)	/* 16-bit, INDEX=1 */
#define EP1_RXCOUNT		(USB1_MC_OFF + 0x18)	/* 16-bit, INDEX=1 */
#define EP1_RXTYPE		(USB1_MC_OFF + 0x1C)	/* 8-bit,  INDEX=1 */
#define EP1_RXINTERVAL		(USB1_MC_OFF + 0x1D)	/* 8-bit,  INDEX=1 */
#define EP1_FIFO		(USB1_MC_OFF + 0x24)	/* 0x20 + 1*4 */
#define EP1_RXFUNCADDR		(USB1_MC_OFF + 0x8C)	/* 0x80 + 8*1 + 0x04 */
#define EP1_RXHUBADDR		(USB1_MC_OFF + 0x8E)
#define EP1_RXHUBPORT		(USB1_MC_OFF + 0x8F)
#  define RXCSR_RXPKTRDY	0x0001
#  define RXCSR_H_ERROR		0x0004
#  define RXCSR_DATAERROR	0x0008	/* NAK timeout (interrupt) */
#  define RXCSR_FLUSHFIFO	0x0010
#  define RXCSR_H_REQPKT	0x0020
#  define RXCSR_H_RXSTALL	0x0040
#  define RXCSR_CLRDATATOG	0x0080
/* RXTYPE/RXINTERVAL TYPE field: speed[7:6], protocol[5:4], remote ep[3:0]. */
#  define TYPE_SPEED_FULL	(2 << 6)
#  define TYPE_PROTO_INTR	(3 << 4)

/* Driver state (single instance — only usb1 is wired as host). */
static u32 usbss_va;		/* VA of USBSS base (PA 0x47400000) */
static int musb_irq;

#define WR(off, val)	mmio_write32(usbss_va + (off), (val))
#define RD(off)		mmio_read32(usbss_va + (off))
#define CWR8(off, val)	mmio_write8(usbss_va + (off), (val))
#define CRD8(off)	mmio_read8(usbss_va + (off))

/* Bring up the full USB clock chain via PRCM and wait for the module to go
 * functional. Without CLKDCOLDO + the L3S domain awake, the USBSS register
 * interface has no clock and OCP accesses fault (synchronous external abort). */
static int musb_enable_clock(void)
{
	u32 dco = phys_to_mmio(CM_CLKDCOLDO_DPLL_PER);
	u32 l3s = phys_to_mmio(CM_PER_L3S_CLKSTCTRL);
	u32 cm  = phys_to_mmio(CM_PER_USB0_CLKCTRL);
	int timeout = 100000;

	/* 1. Enable the PER-DPLL CLKDCOLDO output (the USB 960 MHz source). */
	mmio_write32(dco, CLKDCOLDO_ENABLE);

	/* 2. Force the L3S clock domain awake (USB lives on L3S). */
	mmio_write32(l3s, (mmio_read32(l3s) & ~CLKTRCTRL_MASK) | CLKTRCTRL_SW_WKUP);

	/* 3. Enable the USB module clock and wait for it to become functional. */
	mmio_write32(cm, (mmio_read32(cm) & ~MODULEMODE_MASK) | MODULEMODE_ENABLE);

	while ((mmio_read32(cm) & IDLEST_MASK) != IDLEST_FUNC) {
		if (--timeout == 0) {
			printk("[MUSB] clock enable timeout, CLKCTRL=0x%x\n",
			       (unsigned int)mmio_read32(cm));
			return -1;
		}
	}
	return 0;
}

/* Power up the usb1 UTMI PHY through the control module, host-mode bits
 * (clear power-down + VBUS-detect, enable session-end). Other USB registers
 * must not be written for ~1ms after this transition. */
static void musb_phy_power(void)
{
	u32 reg = phys_to_mmio(USB_CTRL1);
	u32 val = mmio_read32(reg);

	val &= ~(USBPHY_CM_PWRDN | USBPHY_OTG_PWRDN | USBPHY_OTGVDET_EN);
	val |= USBPHY_OTGSESSEND_EN;
	mmio_write32(reg, val);
	mdelay(1);
}

/* Soft-reset the usb1 controller wrapper; de-asserting otg_disable is
 * required for host mode. USBSS-top SYSCONFIG is left alone (system init). */
static int musb_reset_controller(void)
{
	WR(USB1_CTRL, USB1CTRL_RESET);
	mdelay(1);

	WR(USB1_PHY_UTMI, RD(USB1_PHY_UTMI) & ~UTMI_OTG_DISABLE);
	return 0;
}

/* Put the controller in host (A-side) mode and start a session so VBUS is
 * driven and a device connect can be detected (force ID via iddig_mux). */
static void musb_host_start(void)
{
	u32 mode;

	/* Force A-side / host mode in the wrapper, ignore the PHY ID pin. */
	mode = RD(USB1_MODE);
	mode &= ~USB1MODE_IDDIG;	/* 0 = host */
	mode |= USB1MODE_IDDIG_MUX;	/* take ID from this register */
	WR(USB1_MODE, mode);
	WR(USB1_PHY_UTMI, 0x02);	/* vendor host-mode phy_utmi value */

	/* Core: allow high-speed negotiation, enable the USB interrupts (all but
	 * SOF) at both the core and the wrapper, then start the session. */
	CWR8(MC_POWER, POWER_HSENAB);
	CWR8(MC_INTRUSBE, INTRUSBE_HOST);
	WR(USB1_IRQENSET1, WRAP_USB_BITMAP & ~INTR_SOF);

	CWR8(MC_DEVCTL, CRD8(MC_DEVCTL) | DEVCTL_SESSION);
}

/* =====================================================================
 * EP0 polled control transfers + enumeration
 *
 * Runs in a dedicated kernel thread (never in IRQ context) so the long port
 * reset and per-packet polling can sleep/spin without blocking interrupts.
 * The connect IRQ just signals @musb_connect_event.
 * ===================================================================== */

static struct completion musb_connect_event;
static volatile int musb_connected;	/* set by the connect/disconnect IRQ */
static u8 ep0_mps = 8;	/* EP0 max packet size, refined from device descriptor */

/* Known touch device (eGalax 0eef:0005): we assign address 1 at enumeration,
 * its interrupt-IN report endpoint is 2 (0x82), full speed, 64-byte, 11-byte
 * reports. Report layout (ID 1): [1]bit0=tip, [4..5]=X(LE), [6..7]=Y(LE). */
#define TOUCH_ADDR	1
#define TOUCH_EP_IN	2
#define TOUCH_MAXP	64
#define TOUCH_INTERVAL	3

/* Latest raw touch state (landscape panel coords), published to /dev/input0.
 * Protected by a seqlock so readers get a consistent snapshot even under
 * preemption.  touch_seq odd  = writer active; even = data stable.
 */
static volatile unsigned int touch_seq;
static volatile int touch_x, touch_y, touch_pressed;

/* Incremented on every VBUS error or DISCONNECT so musb_touch_loop()
 * exits immediately regardless of how quickly CONNECT fires next. */
static volatile unsigned int musb_generation;

static int musb_get_pointer(int *x, int *y, int *pressed)
{
	unsigned int s1, s2;
	int lx, ly, lp;

	do {
		s1 = touch_seq;
		lx = touch_x;
		ly = touch_y;
		lp = touch_pressed;
		s2 = touch_seq;
	} while (s1 != s2 || (s1 & 1));

	*x = lx;
	*y = ly;
	*pressed = lp;
	return 1;
}

static struct input_ops musb_input_ops = {
	.get_pointer = musb_get_pointer,
};

#define CSR0_RD()	mmio_read16(usbss_va + EP0_CSR0)
#define CSR0_WR(v)	mmio_write16(usbss_va + EP0_CSR0, (u16)(v))

static void musb_fifo_write(u32 fifo, const u8 *buf, int len)
{
	int i = 0;
	for (; i + 4 <= len; i += 4)
		mmio_write32(usbss_va + fifo,
			(u32)buf[i] | (u32)buf[i + 1] << 8 |
			(u32)buf[i + 2] << 16 | (u32)buf[i + 3] << 24);
	if (i < len) {
		u32 v = 0;
		for (int s = 0; i < len; i++, s += 8)
			v |= (u32)buf[i] << s;
		mmio_write32(usbss_va + fifo, v);
	}
}

static void musb_fifo_read(u32 fifo, u8 *buf, int len)
{
	int i = 0;
	for (; i + 4 <= len; i += 4) {
		u32 v = mmio_read32(usbss_va + fifo);
		buf[i] = v; buf[i + 1] = v >> 8; buf[i + 2] = v >> 16; buf[i + 3] = v >> 24;
	}
	if (i < len) {
		u32 v = mmio_read32(usbss_va + fifo);
		for (int s = 0; i < len; i++, s += 8)
			buf[i] = v >> s;
	}
}

/* Poll CSR0 until any @set bit appears; <0 on stall/error/timeout. */
static int ep0_poll_set(u16 set)
{
	for (int t = 0; t < 200000; t++) {
		u16 csr = CSR0_RD();
		if (csr & CSR0_H_RXSTALL) return -1;
		if (csr & CSR0_H_ERROR)   return -2;
		if (csr & set) return csr;
		udelay(10);
	}
	return -3;
}

/* Poll CSR0 until @bit clears; <0 on stall/error/timeout. */
static int ep0_poll_clear(u16 bit)
{
	for (int t = 0; t < 200000; t++) {
		u16 csr = CSR0_RD();
		if (csr & CSR0_H_RXSTALL) return -1;
		if (csr & CSR0_H_ERROR)   return -2;
		if (!(csr & bit)) return 0;
		udelay(10);
	}
	return -3;
}

/* One control transfer to @addr. SETUP is 8 bytes; @data/@data_len carry an
 * IN data stage (we don't need OUT-data control requests). Returns the number
 * of data bytes received, or <0 on error. */
static int ep0_xfer(u8 addr, const u8 *setup, u8 *data, int data_len)
{
	CWR8(MC_INDEX, 0);		/* select EP0 */
	/* Multipoint addressing: target device address goes in the EP0 busctl
	 * func-addr registers (FADDR is unused in multipoint mode). No hub. */
	CWR8(EP0_TXFUNCADDR, addr);
	CWR8(EP0_RXFUNCADDR, addr);
	CWR8(EP0_TXHUBADDR, 0);
	CWR8(EP0_TXHUBPORT, 0);
	CWR8(EP0_RXHUBADDR, 0);
	CWR8(EP0_RXHUBPORT, 0);
	CWR8(MC_FADDR, addr);		/* harmless; used only in non-multipoint */
	CWR8(EP0_NAKLIMIT, 0);		/* no NAK timeout — bounded by sw poll */

	CSR0_WR(CSR0_FLUSHFIFO);
	CSR0_WR(0);

	/* SETUP stage */
	musb_fifo_write(EP0_FIFO, setup, 8);
	CSR0_WR(CSR0_H_SETUPPKT | CSR0_TXPKTRDY);
	int rc = ep0_poll_clear(CSR0_TXPKTRDY);
	if (rc < 0) {
		printk("[MUSB] ep0(req=0x%02x addr=%d) SETUP fail rc=%d csr=%04x\n",
		       setup[1], addr, rc, (unsigned int)CSR0_RD());
		return -10;
	}

	int wlen   = setup[6] | (setup[7] << 8);
	int dir_in = setup[0] & 0x80;
	int actual = 0;

	if (wlen && dir_in) {
		while (actual < wlen) {
			CSR0_WR(CSR0_H_REQPKT);
			rc = ep0_poll_set(CSR0_RXPKTRDY);
			if (rc < 0) {
				printk("[MUSB] ep0(req=0x%02x) IN fail rc=%d csr=%04x got=%d\n",
				       setup[1], rc, (unsigned int)CSR0_RD(), actual);
				return actual ? actual : -20;
			}
			int cnt  = CRD8(EP0_COUNT0);
			int take = cnt;
			if (take > data_len - actual)
				take = data_len - actual;
			musb_fifo_read(EP0_FIFO, data + actual, take);
			actual += cnt;
			if (cnt < ep0_mps) {	/* short packet ends the data stage */
				CSR0_WR(0);
				break;
			}
		}
		/* STATUS OUT (zero-length) */
		CSR0_WR(CSR0_H_STATUSPKT | CSR0_TXPKTRDY | CSR0_H_DIS_PING);
		rc = ep0_poll_clear(CSR0_TXPKTRDY);
		if (rc < 0)
			printk("[MUSB] ep0(req=0x%02x) STATUS-OUT rc=%d csr=%04x\n",
			       setup[1], rc, (unsigned int)CSR0_RD());
	} else {
		/* No-data control: STATUS IN. Wait for REQPKT to clear (the IN
		 * transaction finished), then clear RXPKTRDY+STATUSPKT — matches
		 * the proven u-boot polled host sequence. */
		CSR0_WR(CSR0_H_STATUSPKT | CSR0_H_REQPKT | CSR0_H_DIS_PING);
		rc = ep0_poll_clear(CSR0_H_REQPKT);
		if (rc < 0)
			printk("[MUSB] ep0(req=0x%02x) STATUS-IN rc=%d csr=%04x\n",
			       setup[1], rc, (unsigned int)CSR0_RD());
		u16 c = CSR0_RD();
		c &= ~(CSR0_RXPKTRDY | CSR0_H_STATUSPKT);
		CSR0_WR(c);
	}
	return actual;
}

/* Drive a USB bus reset on the root port (resets the device to address 0). */
static void musb_port_reset(void)
{
	CWR8(MC_POWER, CRD8(MC_POWER) | POWER_RESET);
	msleep(50);
	CWR8(MC_POWER, CRD8(MC_POWER) & ~POWER_RESET);
	msleep(20);
}

static void musb_dump_hex(const u8 *b, int n)
{
	for (int i = 0; i < n; i++) {
		if ((i & 15) == 0)
			printk("[MUSB]   %03d:", i);
		printk(" %02x", b[i]);
		if ((i & 15) == 15 || i == n - 1)
			printk("\n");
	}
}

/* =====================================================================
 * Descriptor walk
 *
 * A configuration descriptor is a chain of variable-length records, each
 * starting with its own length and type, and the only way to read one is to
 * walk it.  Printed rather than dumped: a webcam's configuration runs to a
 * couple of thousand bytes, which is twenty seconds of hex at 115200 baud and
 * unreadable when it arrives.  The console is the only instrument this board
 * has, so what it prints has to be the answer and not the raw material.
 *
 * What the answer has to contain, because the whole design downstream turns on
 * it: whether the device speaks high speed, whether it offers an uncompressed
 * format or only MJPEG, which frame sizes exist, and what each alternate
 * setting costs in isochronous bandwidth.  Everything else is noise.
 * ===================================================================== */

#define DT_DEVICE		0x01
#define DT_CONFIG		0x02
#define DT_STRING		0x03
#define DT_INTERFACE		0x04
#define DT_ENDPOINT		0x05
#define DT_IAD			0x0B
#define DT_CS_INTERFACE		0x24
#define DT_CS_ENDPOINT		0x25

/* UVC VideoStreaming class-specific subtypes (UVC 1.5 spec table A-6). */
#define VS_INPUT_HEADER		0x01
#define VS_FORMAT_UNCOMPRESSED	0x04
#define VS_FRAME_UNCOMPRESSED	0x05
#define VS_FORMAT_MJPEG		0x06
#define VS_FRAME_MJPEG		0x07

#define USB_CLASS_VIDEO		0x0E
#define SC_VIDEOCONTROL		0x01
#define SC_VIDEOSTREAMING	0x02

static u16 le16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }

static u32 le32(const u8 *p)
{
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/*
 * A frame interval is in 100 ns units, so frames per second is ten million
 * over it.  Reported to one decimal because the common rates are not whole
 * numbers — 333333 is 30.0, but 400000 is 25.0 and 666666 is 15.0, and a
 * camera that is actually running at 29.97 should not read as 29.
 */
static void print_fps(const char *label, u32 interval)
{
	unsigned long f10;

	if (!interval) {
		printk("%s?", label);
		return;
	}
	f10 = 100000000UL / interval;
	printk("%s%lu.%lu", label, f10 / 10UL, f10 % 10UL);
}

/*
 * Uncompressed formats are named by a GUID whose first four bytes are a FourCC.
 * Only the FourCC is printed: the remaining twelve are the same fixed suffix
 * for every format in the class, so they would be twelve bytes of noise on a
 * console that is already the bottleneck.
 */
static void print_fourcc(const u8 *guid)
{
	int i;

	for (i = 0; i < 4; i++)
		printk("%c", (guid[i] >= 0x20 && guid[i] < 0x7F) ? guid[i] : '?');
}

/*
 * What the walk concluded, kept for the code that has to act on it.
 *
 * A dump is not an answer.  The alternate settings of a video streaming
 * interface are the numbers the whole design turns on — one of them has to be
 * chosen at runtime, by bandwidth, against what UVC negotiation asks for — and
 * reading them off a console by eye works exactly until the console is busy.
 * It already failed once that way: a camera's descriptors arrive once at boot,
 * and half of them were lost under another subsystem's counters.
 */
#define VS_MAX_ALT	16

struct vs_info {
	int	interface;		/* video streaming interface, -1 if none */
	int	ep;			/* isochronous IN endpoint address */
	int	nalt;			/* alternate settings seen */
	u16	alt_bytes[VS_MAX_ALT];	/* bytes per microframe, per alt */
	int	has_yuy2;
	int	has_mjpeg;
};

static struct vs_info vs;

static void musb_dump_descriptors(const u8 *d, int len)
{
	int i = 0;
	int cur_class = 0, cur_subclass = 0;	/* of the interface being walked */
	int cur_alt = -1;			/* alt setting being walked */
	int nframe = 0;				/* frames printed on this line */

	vs.interface = -1;
	vs.ep = 0;
	vs.nalt = 0;
	vs.has_yuy2 = 0;
	vs.has_mjpeg = 0;
	for (i = 0; i < VS_MAX_ALT; i++)
		vs.alt_bytes[i] = 0;
	i = 0;

	while (i + 2 <= len) {
		int blen = d[i];
		int type = d[i + 1];
		const u8 *p = d + i;

		/*
		 * A zero or over-long length would walk off the end or spin.
		 * The raw bytes come out here and only here: once the walk has
		 * lost its place, the parsed view is worthless and the hex is
		 * the only thing that can say why.
		 */
		if (blen < 2 || i + blen > len) {
			printk("[MUSB]   bad descriptor at %d: len=%d type=0x%02x"
			       " — raw follows\n", i, blen, type);
			musb_dump_hex(d, len);
			break;
		}

		switch (type) {
		case DT_CONFIG:
			printk("[MUSB] config: %d interfaces, %d mA, attr 0x%02x\n",
			       p[4], p[8] * 2, p[7]);
			break;

		case DT_IAD:
			printk("[MUSB] function: interfaces %d..%d, class %d/%d\n",
			       p[2], p[2] + p[3] - 1, p[4], p[5]);
			break;

		case DT_INTERFACE:
			/*
			 * Close any run of frame sizes first.  They are printed
			 * several to a line with no trailing newline, so the
			 * next descriptor would otherwise begin in the middle
			 * of one — which it did, and read as a corrupt line
			 * rather than a missing one.
			 */
			if (nframe) {
				printk("\n");
				nframe = 0;
			}
			cur_class    = p[5];
			cur_subclass = p[6];
			cur_alt      = p[3];
			if (cur_class == USB_CLASS_VIDEO &&
			    cur_subclass == SC_VIDEOSTREAMING) {
				vs.interface = p[2];
				if (cur_alt + 1 > vs.nalt && cur_alt < VS_MAX_ALT)
					vs.nalt = cur_alt + 1;
			}
			printk("[MUSB] if %d alt %d: %d endpoints,"
			       " class %d/%d/%d%s\n",
			       p[2], p[3], p[4], p[5], p[6], p[7],
			       (p[5] == USB_CLASS_VIDEO &&
				p[6] == SC_VIDEOSTREAMING) ? "  <- video stream"
							   : "");
			break;

		case DT_ENDPOINT: {
			static const char *const kind[4] = {
				"control", "iso", "bulk", "interrupt"
			};
			u16 mp = le16(p + 4);
			/*
			 * On high speed, bits 12:11 of wMaxPacketSize are one
			 * less than the number of transactions per microframe.
			 * Ignoring them understates an isochronous endpoint's
			 * bandwidth by up to three times, which is exactly the
			 * number that decides whether a format fits.
			 */
			int mult = ((mp >> 11) & 3) + 1;
			int bytes = (mp & 0x7FF) * mult;

			if (cur_class == USB_CLASS_VIDEO &&
			    cur_subclass == SC_VIDEOSTREAMING &&
			    (p[3] & 3) == 1 && (p[2] & 0x80) &&
			    cur_alt >= 0 && cur_alt < VS_MAX_ALT) {
				vs.alt_bytes[cur_alt] = (u16)bytes;
				vs.ep = p[2];
			}

			printk("[MUSB]   ep 0x%02x %s %s, %d B/uframe"
			       " (%d x %d), interval %d\n",
			       p[2], (p[2] & 0x80) ? "IN " : "OUT",
			       kind[p[3] & 3], bytes, mult, mp & 0x7FF, p[6]);
			break;
		}

		case DT_CS_INTERFACE:
			if (cur_class != USB_CLASS_VIDEO ||
			    cur_subclass != SC_VIDEOSTREAMING || blen < 3)
				break;		/* control-interface units: not the question */

			switch (p[2]) {
			case VS_FORMAT_UNCOMPRESSED:
				/*
				 * YUY2 by its FourCC, which is the one thing
				 * that decides whether this box needs a JPEG
				 * decoder or not.
				 */
				if (p[5] == 'Y' && p[6] == 'U' &&
				    p[7] == 'Y' && p[8] == '2')
					vs.has_yuy2 = 1;
				printk("\n[MUSB]   format %d: uncompressed ", p[3]);
				print_fourcc(p + 5);
				printk(", %d bpp, %d sizes:", p[21], p[4]);
				nframe = 0;
				break;

			case VS_FORMAT_MJPEG:
				vs.has_mjpeg = 1;
				printk("\n[MUSB]   format %d: MJPEG,"
				       " %d sizes:", p[3], p[4]);
				nframe = 0;
				break;

			case VS_FRAME_UNCOMPRESSED:
			case VS_FRAME_MJPEG:
				/*
				 * Several to a line, and only the size and the
				 * fastest rate each one offers.
				 *
				 * It was a line per frame with the buffer size
				 * and every interval on it, and that is what
				 * cost the alternate-setting table: eighteen
				 * frame sizes in two formats is thirty-six
				 * lines of about seventy characters, and the
				 * console ring holds four thousand and ninety
				 * six bytes total.  The rest of the dump was
				 * dropped — silently, because the drop notice
				 * only exists on the boot-time polling path.
				 *
				 * The slower intervals are not lost so much as
				 * not worth the room: a camera that offers a
				 * size at all offers it at its own maximum, and
				 * the rate actually used comes back from UVC
				 * negotiation rather than from this list.
				 */
				if ((nframe++ & 3) == 0)
					printk("\n[MUSB]    ");
				printk(" %dx%d", le16(p + 5), le16(p + 7));
				print_fps("@", le32(p + 21));
				break;

			case VS_INPUT_HEADER:
				printk("[MUSB]   stream: %d formats,"
				       " endpoint 0x%02x\n", p[3], p[6]);
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}

		i += blen;
	}

	if (nframe)
		printk("\n");	/* close the last run of frame sizes */

	if (vs.interface < 0) {
		printk("[MUSB] no video streaming interface\n");
		return;
	}

	/*
	 * The conclusion, on two lines, because the thirty above them are the
	 * working and these are the answer.  Short enough to survive another
	 * subsystem printing over the middle of the dump, which is how the
	 * alternate settings were lost the first time this ran.
	 */
	printk("[MUSB] video: if %d, ep 0x%02x, %s%s%d alts B/uframe:",
	       vs.interface, vs.ep,
	       vs.has_yuy2 ? "YUY2 " : "", vs.has_mjpeg ? "MJPEG " : "",
	       vs.nalt);
	for (i = 0; i < vs.nalt; i++)
		printk(" %d", vs.alt_bytes[i]);
	printk("\n");

	/*
	 * 320x240 in YUY2 at thirty frames is 4.6 MB/s, and high speed divides
	 * a second into 8000 microframes, so it needs 576 bytes in each one.
	 * Saying whether that fits, here, turns the table above into a verdict:
	 * the alternate setting eventually chosen has to be the smallest one
	 * that clears the figure UVC negotiation asks for, and this is the same
	 * arithmetic done once by hand.
	 */
	{
		int need = (320 * 240 * 2 * 30) / 8000;
		int best = -1;

		for (i = 0; i < vs.nalt; i++)
			if (vs.alt_bytes[i] >= need) {
				best = i;
				break;
			}

		if (best >= 0)
			printk("[MUSB] 320x240 YUY2 30fps needs %d B/uframe:"
			       " alt %d has %d\n",
			       need, best, vs.alt_bytes[best]);
		else
			printk("[MUSB] 320x240 YUY2 30fps needs %d B/uframe:"
			       " NO alt is big enough\n", need);
	}
}

static int musb_enumerate(void)
{
	static const u8 GET_DEV8[8]  = {0x80,0x06,0x00,0x01,0,0,0x08,0};
	static const u8 SET_ADDR1[8] = {0x00,0x05,0x01,0x00,0,0,0x00,0};
	static const u8 GET_DEV18[8] = {0x80,0x06,0x00,0x01,0,0,0x12,0};
	static const u8 GET_CFG9[8]  = {0x80,0x06,0x00,0x02,0,0,0x09,0};
	u8 desc[18];
	/*
	 * Static, and four kilobytes of it.  It was 256 on the stack, which was
	 * enough for a touchscreen and is not enough for a camera: a webcam
	 * describes every format at every frame size at every rate, and the
	 * configuration descriptor runs to one or two thousand bytes.  A short
	 * buffer would not fail loudly either — the read would simply stop, and
	 * the formats that did not fit would look like formats the camera does
	 * not have.  Static because a kernel task has one page of stack.
	 */
	static u8 cfg[4096];
	int r;

	ep0_mps = 8;
	musb_port_reset();

	/*
	 * Speed first, because it decides what is possible rather than merely
	 * how fast.  Full speed carries 12 Mbit/s in total and less than that
	 * for isochronous, against 36.9 Mbit/s for 320x240 uncompressed at
	 * thirty frames — so a camera that comes up full speed cannot stream
	 * uncompressed video at all, whatever the rest of its descriptors say.
	 */
	printk("[MUSB] enum: device is %s speed (power=0x%02x devctl=0x%02x)\n",
	       (CRD8(MC_POWER) & POWER_HSMODE) ? "HIGH" :
	       (CRD8(MC_DEVCTL) & DEVCTL_LSDEV) ? "low" : "full",
	       (unsigned int)CRD8(MC_POWER), (unsigned int)CRD8(MC_DEVCTL));

	/* 1. First 8 bytes of the device descriptor (learn bMaxPacketSize0). */
	r = ep0_xfer(0, GET_DEV8, desc, 8);
	if (r < 8) {
		printk("[MUSB] enum: GET_DESCRIPTOR(dev,8) failed r=%d\n", r);
		return -1;
	}
	ep0_mps = desc[7] ? desc[7] : 8;
	printk("[MUSB] enum: bMaxPacketSize0=%d\n", desc[7]);

	/* 2. Assign address 1. */
	r = ep0_xfer(0, SET_ADDR1, 0, 0);
	if (r < 0) {
		printk("[MUSB] enum: SET_ADDRESS(1) failed r=%d\n", r);
		return -1;
	}
	msleep(10);

	/* 3. Full device descriptor (now at address 1). */
	r = ep0_xfer(1, GET_DEV18, desc, 18);
	if (r < 18) {
		printk("[MUSB] enum: GET_DESCRIPTOR(dev,18) failed r=%d\n", r);
		return -1;
	}
	printk("[MUSB] enum: VID=%04x PID=%04x class=%d/%d/%d nCfg=%d\n",
	       (unsigned int)(desc[8] | desc[9] << 8),
	       (unsigned int)(desc[10] | desc[11] << 8),
	       desc[4], desc[5], desc[6], desc[17]);

	/* 4. Config descriptor header → wTotalLength. */
	r = ep0_xfer(1, GET_CFG9, cfg, 9);
	if (r < 9) {
		printk("[MUSB] enum: GET_DESCRIPTOR(cfg,9) failed r=%d\n", r);
		return -1;
	}
	int wtotal = cfg[2] | cfg[3] << 8;
	if (wtotal > (int)sizeof(cfg))
		wtotal = sizeof(cfg);

	/* 5. Full config blob (interfaces + endpoints + HID descriptor). */
	u8 get_cfg_full[8] = {0x80,0x06,0x00,0x02,0,0,
			      (u8)(wtotal & 0xFF), (u8)(wtotal >> 8)};
	r = ep0_xfer(1, get_cfg_full, cfg, wtotal);
	printk("[MUSB] enum: config wTotalLength=%d read=%d cfgVal=%d nIf=%d\n",
	       wtotal, r, cfg[5], cfg[4]);
	if (r > 0)
		musb_dump_descriptors(cfg, r < wtotal ? r : wtotal);

	/* 6. Select the configuration. */
	u8 set_cfg[8] = {0x00,0x09,cfg[5],0x00,0,0,0x00,0};
	ep0_xfer(1, set_cfg, 0, 0);
	printk("[MUSB] enum: SET_CONFIGURATION(%d) done\n", cfg[5]);

	/*
	 * HID SET_IDLE, and only for a HID device.  It was sent unconditionally
	 * while the touchscreen was the only thing that could be plugged in; a
	 * camera answers it with a STALL, which is harmless but is also a line
	 * of log saying something failed when nothing did.
	 */
	if (CONFIG_USB_TOUCH && desc[4] == 3) {
		static const u8 SET_IDLE[8] = {0x21,0x0A,0,0,0,0,0,0};

		ep0_xfer(1, SET_IDLE, 0, 0);
	}
	return 0;
}

/* Configure hardware EP1 to poll the touch device's interrupt-IN endpoint. */
static void musb_touch_setup(void)
{
	CWR8(MC_INDEX, 1);
	/* Allocate EP1's RX FIFO (64 bytes) in the dynamic FIFO RAM, just past
	 * EP0's reserved 64-byte FIFO. Without this the endpoint can't buffer
	 * incoming packets and RXPKTRDY never fires. */
	CWR8(MC_RXFIFOSZ, 3);				/* 64 bytes, single buffer */
	mmio_write16(usbss_va + MC_RXFIFOADD, 64 >> 3);	/* RAM offset 64 */

	CWR8(EP1_RXFUNCADDR, TOUCH_ADDR);
	CWR8(EP1_RXHUBADDR, 0);
	CWR8(EP1_RXHUBPORT, 0);
	CWR8(EP1_RXINTERVAL, TOUCH_INTERVAL);
	CWR8(EP1_RXTYPE, TYPE_SPEED_FULL | TYPE_PROTO_INTR | (TOUCH_EP_IN & 0xF));
	mmio_write16(usbss_va + EP1_RXMAXP, TOUCH_MAXP);
	mmio_write16(usbss_va + EP1_RXCSR, RXCSR_FLUSHFIFO | RXCSR_CLRDATATOG);
}

/* Poll the interrupt-IN endpoint and log touch reports until disconnect.
 * Milestone 3: just parse and print tip/X/Y to confirm the data path. */
static void musb_touch_loop(unsigned int gen)
{
	printk("[MUSB] touch: polling EP%d-IN, feeding /dev/input0\n", TOUCH_EP_IN);

	while (musb_generation == gen) {
		CWR8(MC_INDEX, 1);
		u16 csr = mmio_read16(usbss_va + EP1_RXCSR);

		if (csr & RXCSR_RXPKTRDY) {
			u8 buf[TOUCH_MAXP];
			int cnt = mmio_read16(usbss_va + EP1_RXCOUNT) & 0x7F;
			if (cnt > TOUCH_MAXP)
				cnt = TOUCH_MAXP;
			musb_fifo_read(EP1_FIFO, buf, cnt);

			/* Ack this packet and request the next. */
			csr = mmio_read16(usbss_va + EP1_RXCSR);
			mmio_write16(usbss_va + EP1_RXCSR,
				(csr & ~RXCSR_RXPKTRDY) | RXCSR_H_REQPKT);

			if (cnt >= 8 && buf[0] == 0x01) {
				int tip = buf[1] & 1;
				touch_seq++;
				touch_x = buf[4] | buf[5] << 8;
				touch_y = buf[6] | buf[7] << 8;
				touch_pressed = tip;
				touch_seq++;
			}
		} else if (csr & (RXCSR_H_RXSTALL | RXCSR_H_ERROR | RXCSR_DATAERROR)) {
			mmio_write16(usbss_va + EP1_RXCSR, RXCSR_FLUSHFIFO);
			mmio_write16(usbss_va + EP1_RXCSR, RXCSR_H_REQPKT);
		} else {
			if (!(csr & RXCSR_H_REQPKT))
				mmio_write16(usbss_va + EP1_RXCSR, csr | RXCSR_H_REQPKT);
			msleep(5);	/* yield; ~poll rate while idle/between reports */
		}
	}

	/* Device gone: report release so the GUI doesn't latch a stuck press. */
	touch_seq++;
	touch_pressed = 0;
	touch_seq++;
}

static void musb_enum_thread(void)
{
	for (;;) {
		wait_for_completion(&musb_connect_event);
		msleep(100);	/* let the device settle after connect */

		/*
		 * Taken before enumerating, not after: a device unplugged during
		 * the attempt bumps the generation, and comparing against a
		 * value read afterwards would wait for a device that has
		 * already gone.
		 */
		unsigned int gen = musb_generation;

		/* eGalax + BBB MUSB is flaky (VBUS/babble/renumber) — retry. */
		int ok = 0;
		for (int attempt = 1; attempt <= 4 && !ok; attempt++) {
			printk("[MUSB] enumerating (attempt %d)\n", attempt);
			ok = (musb_enumerate() == 0);
			if (!ok)
				msleep(150);
		}

		if (!ok)
			continue;

		/*
		 * The touch path assumes one device — its endpoint number, its
		 * packet size, its report layout, all constants at the top of
		 * this file.  Running it against a camera would poll an
		 * endpoint that is not there and read reports that are not
		 * reports.  Enumeration by itself is the whole of what a new
		 * device gets until something knows what it is.
		 */
		if (CONFIG_USB_TOUCH) {
			musb_touch_setup();
			musb_touch_loop(musb_generation);
		} else {
			printk("[MUSB] enumerated; no class driver for it yet\n");
			while (musb_generation == gen)
				msleep(200);
		}
	}
}

static void musb_irq_handler(unsigned int irq)
{
	(void)irq;
	u32 epintr   = RD(USB1_IRQSTAT0);
	u32 coreintr = RD(USB1_IRQSTAT1);

	/* Acknowledge (write-1-to-clear) what we read. */
	if (epintr)
		WR(USB1_IRQSTAT0, epintr);
	if (coreintr)
		WR(USB1_IRQSTAT1, coreintr);

	u32 usbintr = coreintr & WRAP_USB_BITMAP;

	if (usbintr & INTR_CONNECT) {
		u8 devctl = CRD8(MC_DEVCTL);
		const char *speed = (devctl & DEVCTL_LSDEV) ? "low" :
				    (devctl & DEVCTL_FSDEV) ? "full/high" : "?";
		printk("[MUSB] CONNECT (devctl=0x%x speed=%s)\n",
		       (unsigned int)devctl, speed);
		musb_connected = 1;
		complete(&musb_connect_event);	/* wake the enumeration thread */
	}
	if (usbintr & INTR_DISCONNECT) {
		musb_generation++;
		musb_connected = 0;
		printk("[MUSB] DISCONNECT\n");
	}
	if (usbintr & INTR_VBUSERROR) {
		/* Increment generation before re-arming: musb_touch_loop() exits
		 * as soon as it sees generation != my_gen, regardless of how
		 * quickly CONNECT fires and sets musb_connected back to 1. */
		musb_generation++;
		musb_connected = 0;
		printk("[MUSB] VBUS error — restarting session\n");
		CWR8(MC_DEVCTL, CRD8(MC_DEVCTL) | DEVCTL_SESSION);
	}
}

static int musb_hcd_probe(struct platform_device *pdev)
{
	usbss_va  = phys_to_mmio(pdev->base);	/* PA 0x47400000 -> VA 0xF3000000 */
	musb_irq  = platform_get_irq(pdev, 0);

	printk("[MUSB] probe: USBSS VA=0x%x irq=%d\n",
	       (unsigned int)usbss_va, musb_irq);

	if (musb_enable_clock() != 0)
		return -1;

	printk("[MUSB] clocks: CLKDCOLDO=0x%x L3S=0x%x USB0CLKCTRL=0x%x\n",
	       (unsigned int)mmio_read32(phys_to_mmio(CM_CLKDCOLDO_DPLL_PER)),
	       (unsigned int)mmio_read32(phys_to_mmio(CM_PER_L3S_CLKSTCTRL)),
	       (unsigned int)mmio_read32(phys_to_mmio(CM_PER_USB0_CLKCTRL)));

	/* Revision reads 0 when USBSS is not yet clocked. */
	u32 ctrl_rev = RD(USB1_REV);
	printk("[MUSB] usb1 control revision=0x%08x\n", (unsigned int)ctrl_rev);
	if (!ctrl_rev) {
		printk("[MUSB] usb1 not clocked (revision reads 0)\n");
		return -1;
	}

	musb_phy_power();

	if (musb_reset_controller() != 0)
		return -1;

	/* Clear stale interrupt status before we wire the handler. */
	WR(USB1_IRQENCLR0, 0xFFFFFFFF);
	WR(USB1_IRQENCLR1, 0xFFFFFFFF);
	WR(USB1_IRQSTAT0, RD(USB1_IRQSTAT0));
	WR(USB1_IRQSTAT1, RD(USB1_IRQSTAT1));

	/* Expose touch as the /dev/input0 pointer backend. */
	input_register_ops(&musb_input_ops);

	/*
	 * Enumeration runs in its own thread, woken by the connect IRQ.
	 *
	 * BG rather than a deadline level: enumerating a newly plugged device is
	 * a long conversation of control transfers with timeouts in it, and
	 * nothing is waiting on a deadline for it to finish.  Priority means
	 * urgency x shortness (kernel-roadmap.md §9.3), and this is neither, so
	 * it belongs where tasks take turns.  Delivering input events once the
	 * device is up is a different path — short, and driven by the IRQ.
	 */
	init_completion(&musb_connect_event);
	struct task_struct *t = task_create(musb_enum_thread, PRIO_BG,
					    "musb-enum");
	if (t)
		enqueue_task(&runqueue, t);
	else
		printk("[MUSB] WARNING could not start enum thread\n");

	if (musb_irq > 0) {
		request_irq(musb_irq, musb_irq_handler);
		intc_enable_irq(musb_irq);
	}

	/* Host mode + session: drives VBUS and arms connect detection. */
	musb_host_start();

	printk("[MUSB] host mode, session started (devctl=0x%x) — plug touch to test\n",
	       (unsigned int)CRD8(MC_DEVCTL));
	printk("[MUSB] probe ok (Milestone 2: enumeration ready)\n");
	return 0;
}

static struct platform_driver musb_hcd_driver = {
	.drv = {
		.name = "musb_hcd",
	},
	.probe = musb_hcd_probe,
};

static int __init musb_hcd_init(void)
{
	/* See nothan/config.h. */
	if (!CONFIG_USB_HOST)
		return 0;

	return platform_driver_register(&musb_hcd_driver);
}
device_initcall(musb_hcd_init);
