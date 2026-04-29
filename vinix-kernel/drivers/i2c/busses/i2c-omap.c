/*
 * AM335x I2C0 Driver
 *
 * Provides polling-mode I2C master functionality (7-bit addressing).
 * This driver handles the low-level register configuration required to
 * transmit and receive data over the I2C bus.
 */

#include "types.h"
#include "i2c.h"
#include "mmio.h"
#include "uart.h"
#include "platform_device.h"
#include "vinix/init.h"
#include "vinix/i2c.h"
#include "mach/prcm.h"
#include "mach/control.h"

/* I2C0 pinmux pads — mode 0 selects native I2C0 (not muxed from UART). */
#define CONF_I2C0_SDA           (CTRL_MODULE_BASE + 0x988)
#define CONF_I2C0_SCL           (CTRL_MODULE_BASE + 0x98C)
/* mode 0 | pullup enabled | RX active | slow slew */
#define PAD_I2C0_MODE           0x70

/* Hardware Register Offsets */

#define I2C_REVNB_LO    0x00    /* Module revision (low) */
#define I2C_REVNB_HI    0x04    /* Module revision (high) */
#define I2C_SYSC        0x10    /* System configuration */
#define I2C_IRQSTATUS_RAW 0x24  /* IRQ raw status */
#define I2C_IRQSTATUS   0x28    /* IRQ status */
#define I2C_IRQENABLE_SET 0x2C  /* IRQ enable set */
#define I2C_IRQENABLE_CLR 0x30  /* IRQ enable clear */
#define I2C_SYSS        0x90    /* System status */
#define I2C_BUF         0x94    /* Buffer configuration */
#define I2C_CNT         0x98    /* Data counter */
#define I2C_DATA        0x9C    /* Data access */
#define I2C_CON         0xA4    /* Configuration */
#define I2C_OA          0xA8    /* Own address */
#define I2C_SA          0xAC    /* Slave address */
#define I2C_PSC         0xB0    /* Clock prescaler */
#define I2C_SCLL        0xB4    /* SCL low time */
#define I2C_SCLH        0xB8    /* SCL high time */
#define I2C_BUFSTAT     0xC0    /* Buffer status */
#define I2C_SYSTEST     0xBC    /* System test register */

/* SYSC register bit definitions */

#define I2C_SYSC_SRST   (1 << 1)    /* Software reset */

/* SYSS register bit definitions */

#define I2C_SYSS_RDONE  (1 << 0)    /* Reset done */

/* CON register bit definitions */

#define I2C_CON_EN      (1 << 15)   /* Module enable */
#define I2C_CON_MST     (1 << 10)   /* Master mode */
#define I2C_CON_TRX     (1 << 9)    /* Transmitter mode */
#define I2C_CON_STP     (1 << 1)    /* Stop condition */
#define I2C_CON_STT     (1 << 0)    /* Start condition */

/* IRQSTATUS register bit definitions */

#define I2C_STAT_BB     (1 << 12)   /* Bus busy */
#define I2C_STAT_ROVR   (1 << 11)   /* Receive overrun */
#define I2C_STAT_XUDF   (1 << 10)   /* Transmit underflow */
#define I2C_STAT_AAS    (1 << 9)    /* Addressed as slave */
#define I2C_STAT_BF     (1 << 8)    /* Bus free */
#define I2C_STAT_AERR   (1 << 7)    /* Access error */
#define I2C_STAT_GC     (1 << 5)    /* General call */
#define I2C_STAT_XRDY   (1 << 4)    /* Transmit data ready */
#define I2C_STAT_RRDY   (1 << 3)    /* Receive data ready */
#define I2C_STAT_ARDY   (1 << 2)    /* Register access ready */
#define I2C_STAT_NACK   (1 << 1)    /* No acknowledgement */
#define I2C_STAT_AL     (1 << 0)    /* Arbitration lost */

/* Default operation timeout in loops */

#define I2C_TIMEOUT     100000

/* Internal helper functions */

/**
 * Wait until bus is not busy.
 * Polls BB bit in IRQSTATUS_RAW.
 */
static int i2c_wait_bus_free(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((mmio_read32(I2C0_BASE + I2C_IRQSTATUS_RAW) & I2C_STAT_BB) && timeout--) {
        /* Busy wait */
    }

    if (!timeout) {
        pr_err("[I2C] ERROR: Bus busy timeout\n");
        return -1;
    }
    return 0;
}

/**
 * Wait for specific status flag(s) to be set.
 * Returns raw status on success, 0 on timeout.
 */
static uint32_t i2c_wait_status(uint32_t mask)
{
    uint32_t timeout = I2C_TIMEOUT;
    uint32_t stat;

    while (timeout--) {
        stat = mmio_read32(I2C0_BASE + I2C_IRQSTATUS_RAW);
        if (stat & (mask | I2C_STAT_NACK | I2C_STAT_AL)) {
            return stat;
        }
    }
    return 0;
}

/**
 * Clear all interrupt status flags.
 */
static void i2c_clear_status(void)
{
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, 0x7FFF);
}

/**
 * Flush FIFO and reset I2C state machine after a failed transaction.
 * Forces STOP on the bus before resetting the module to recover
 * from stuck-bus conditions (e.g. slave holding SDA low).
 */
static void i2c_flush(void)
{
    /* Force STOP condition to release any stuck slave */
    mmio_write32(I2C0_BASE + I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
    for (volatile int i = 0; i < 500; i++);

    /* Disable module to reset internal FIFO and state machine */
    mmio_write32(I2C0_BASE + I2C_CON, 0);
    for (volatile int i = 0; i < 100; i++);

    /* Re-enable with EN only — do NOT restore STT/STP, that would
     * immediately trigger a new transaction with stale state */
    mmio_write32(I2C0_BASE + I2C_CON, I2C_CON_EN);
    for (volatile int i = 0; i < 100; i++);

    i2c_clear_status();
}

/*
 * omap_i2c_hw_init — bring up the I2C0 controller
 *
 * Configures pin multiplexing, enables the Wakeup-domain functional clock,
 * performs a soft reset, and programs the prescaler for 100 kHz standard
 * mode. Returns 0 on success, -1 on timeout.
 */

static int omap_i2c_hw_init(void)
{
    uint32_t val;
    uint32_t timeout;

    pr_info("[I2C] Initializing I2C0...\n");

    /* Re-assert pins — ROM had them muxed for PMIC access. */
    mmio_write32(CONF_I2C0_SCL, PAD_I2C0_MODE);
    mmio_write32(CONF_I2C0_SDA, PAD_I2C0_MODE);
    pr_info("[I2C] Pinmux configured (SCL=0x%x, SDA=0x%x)\n",
                mmio_read32(CONF_I2C0_SCL), mmio_read32(CONF_I2C0_SDA));

    /* I2C0 lives in Wakeup domain; FCLK = 48 MHz (PER_CLKOUTM2 / 4). */
    mmio_write32(CM_WKUP_I2C0_CLKCTRL, MODULEMODE_ENABLE);

    timeout = I2C_TIMEOUT;
    while (timeout--) {
        val = mmio_read32(CM_WKUP_I2C0_CLKCTRL);
        if ((val & IDLEST_MASK) == IDLEST_FUNCTIONAL) {
            break;
        }
    }
    if (!timeout) {
        pr_err("[I2C] ERROR: Clock enable timeout\n");
        return -1;
    }
    pr_info("[I2C] Clock enabled\n");

    /* HW spec: module must be disabled before reconfiguring. */
    mmio_write32(I2C0_BASE + I2C_CON, 0);

    mmio_write32(I2C0_BASE + I2C_SYSC, I2C_SYSC_SRST);

    timeout = I2C_TIMEOUT;
    while (!(mmio_read32(I2C0_BASE + I2C_SYSS) & I2C_SYSS_RDONE) && timeout--) {
    }
    if (!timeout) {
        pr_err("[I2C] ERROR: Soft reset timeout\n");
        return -1;
    }
    pr_info("[I2C] Soft reset complete\n");

    /* Prescaler: ICLK = 48 MHz / (PSC+1). PSC=3 → ICLK=12 MHz. */
    mmio_write32(I2C0_BASE + I2C_PSC, 3);

    /* 100 kHz standard mode @ ICLK=12 MHz:
     * tLOW = (SCLL+7)·Tclk, tHIGH = (SCLH+5)·Tclk; want 5us each.
     * SCLL = 5e-6 · 12e6 − 7 = 53,  SCLH = 5e-6 · 12e6 − 5 = 55. */
    mmio_write32(I2C0_BASE + I2C_SCLL, 53);
    mmio_write32(I2C0_BASE + I2C_SCLH, 55);

    /* Arbitrary master address. */
    mmio_write32(I2C0_BASE + I2C_OA, 0x01);

    i2c_clear_status();

    /* Polling mode — mask every IRQ. */
    mmio_write32(I2C0_BASE + I2C_IRQENABLE_CLR, 0x7FFF);

    mmio_write32(I2C0_BASE + I2C_CON, I2C_CON_EN);

    for (volatile int i = 0; i < 1000; i++);

    val = mmio_read32(I2C0_BASE + I2C_REVNB_LO);
    pr_info("[I2C] Module revision = 0x%x\n", val);

    /* Idle bus must read SCL_I_FUNC=bit8, SDA_I_FUNC=bit6 high.
     * 0 = stuck low → pinmux or HW problem. */
    val = mmio_read32(I2C0_BASE + I2C_SYSTEST);
    pr_info("[I2C] SYSTEST=0x%x  SCL_I=%d  SDA_I=%d  (1=high=OK)\n",
                val, (val >> 8) & 1, (val >> 6) & 1);

    pr_info("[I2C] I2C0 initialized (100kHz standard mode)\n");
    return 0;
}

/*
 * i2c_write_reg - Write a single byte to an I2C device register
 * @slave_addr: The 7-bit I2C address of the target device
 * @reg: The register address to write to
 * @val: The value to write into the register
 *
 * Executes a master transmit transaction: [S] [slave_addr+W] [reg] [val] [P]
 */

int i2c_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t val)
{
    uint32_t stat;

    /* Wait for bus free */
    if (i2c_wait_bus_free() != 0) {
        i2c_flush();
        return -1;
    }

    i2c_clear_status();

    /* Set slave address */
    mmio_write32(I2C0_BASE + I2C_SA, slave_addr);

    /* Set data count = 2 (register addr + value) */
    mmio_write32(I2C0_BASE + I2C_CNT, 2);

    /* Configure: master, transmitter, start, stop */
    mmio_write32(I2C0_BASE + I2C_CON,
                 I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX | I2C_CON_STT | I2C_CON_STP);

    /* Send register address */
    stat = i2c_wait_status(I2C_STAT_XRDY);
    if (!stat || (stat & I2C_STAT_NACK)) {
        pr_info("[I2C] NACK on write addr 0x%x reg 0x%x\n", slave_addr, reg);
        i2c_flush();
        return -1;
    }
    mmio_write32(I2C0_BASE + I2C_DATA, reg);
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_XRDY);

    /* Send value */
    stat = i2c_wait_status(I2C_STAT_XRDY);
    if (!stat || (stat & I2C_STAT_NACK)) {
        pr_info("[I2C] NACK on write data 0x%x reg 0x%x\n", slave_addr, reg);
        i2c_flush();
        return -1;
    }
    mmio_write32(I2C0_BASE + I2C_DATA, val);
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_XRDY);

    /* Wait for transfer complete (ARDY = access ready) */
    stat = i2c_wait_status(I2C_STAT_ARDY);
    if (!stat) {
        uint32_t raw = mmio_read32(I2C0_BASE + I2C_IRQSTATUS_RAW);
        pr_info("[I2C] ARDY timeout write sa=0x%x reg=0x%x raw=0x%x\n",
                    slave_addr, reg, raw);
        i2c_flush();
        return -1;
    }
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_ARDY);

    return 0;
}

/*
 * i2c_read_reg - Read a single byte from an I2C device register
 * @slave_addr: The 7-bit I2C address of the target device
 * @reg: The register address to read from
 * @val: Pointer to store the read byte
 *
 * Executes a master receive transaction using a repeated start:
 * Phase 1: [S] [slave_addr+W] [reg]          (set register pointer)
 * Phase 2: [Sr] [slave_addr+R] [data] [P]    (read data)
 */

int i2c_read_reg(uint8_t slave_addr, uint8_t reg, uint8_t *val)
{
    uint32_t stat;

    /* Wait for bus free */
    if (i2c_wait_bus_free() != 0) {
        i2c_flush();
        return -1;
    }

    i2c_clear_status();

    /* Write register address without STOP — slave will hold the pointer
     * for the subsequent repeated-start read. */
    mmio_write32(I2C0_BASE + I2C_SA, slave_addr);
    mmio_write32(I2C0_BASE + I2C_CNT, 1);

    /* Master, transmitter, start, NO stop (repeated start follows) */
    mmio_write32(I2C0_BASE + I2C_CON,
                 I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX | I2C_CON_STT);

    /* Wait for XRDY and send register address */
    stat = i2c_wait_status(I2C_STAT_XRDY);
    if (!stat || (stat & I2C_STAT_NACK)) {
        pr_info("[I2C] NACK on read phase1 addr 0x%x reg 0x%x\n", slave_addr, reg);
        i2c_flush();
        return -1;
    }
    mmio_write32(I2C0_BASE + I2C_DATA, reg);
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_XRDY);

    /* Wait for ARDY (register address sent) */
    stat = i2c_wait_status(I2C_STAT_ARDY);
    if (!stat) {
        pr_info("[I2C] Timeout on read phase1 ARDY\n");
        i2c_flush();
        return -1;
    }
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_ARDY);

    /* Repeated start in receiver mode — read the byte. */
    i2c_clear_status();
    mmio_write32(I2C0_BASE + I2C_CNT, 1);

    /* Master, receiver (TRX=0), start, stop */
    mmio_write32(I2C0_BASE + I2C_CON,
                 I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP);

    /* Wait for RRDY */
    stat = i2c_wait_status(I2C_STAT_RRDY);
    if (!stat || (stat & I2C_STAT_NACK)) {
        pr_info("[I2C] NACK on read phase2 addr 0x%x reg 0x%x\n", slave_addr, reg);
        i2c_flush();
        return -1;
    }
    *val = (uint8_t)(mmio_read32(I2C0_BASE + I2C_DATA) & 0xFF);
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_RRDY);

    /* Wait for transfer complete */
    stat = i2c_wait_status(I2C_STAT_ARDY);
    if (stat) {
        mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_ARDY);
    }

    return 0;
}

/*
 * i2c_read_block - Read multiple bytes from an I2C device
 * @slave_addr: The 7-bit I2C address of the target device
 * @reg: The starting register address
 * @buf: Pointer to the buffer to store the read bytes
 * @len: Number of bytes to read
 *
 * Similar to i2c_read_reg, but continues reading multiple bytes in phase 2.
 * Often used for retrieving large data blocks like EDID structures.
 */

int i2c_read_block(uint8_t slave_addr, uint8_t reg, uint8_t *buf, int len)
{
    uint32_t stat;
    int i;

    if (len <= 0 || buf == NULL) {
        return -1;
    }

    /* Wait for bus free */
    if (i2c_wait_bus_free() != 0) {
        i2c_flush();
        return -1;
    }

    i2c_clear_status();

    mmio_write32(I2C0_BASE + I2C_SA, slave_addr);
    mmio_write32(I2C0_BASE + I2C_CNT, 1);
    mmio_write32(I2C0_BASE + I2C_CON,
                 I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX | I2C_CON_STT);

    stat = i2c_wait_status(I2C_STAT_XRDY);
    if (!stat || (stat & I2C_STAT_NACK)) {
        i2c_flush();
        return -1;
    }
    mmio_write32(I2C0_BASE + I2C_DATA, reg);
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_XRDY);

    stat = i2c_wait_status(I2C_STAT_ARDY);
    if (!stat) {
        i2c_flush();
        return -1;
    }
    mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_ARDY);

    /* Repeated start in receiver mode — read len bytes sequentially. */
    i2c_clear_status();
    mmio_write32(I2C0_BASE + I2C_CNT, len);
    mmio_write32(I2C0_BASE + I2C_CON,
                 I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP);

    for (i = 0; i < len; i++) {
        stat = i2c_wait_status(I2C_STAT_RRDY);
        if (!stat || (stat & I2C_STAT_NACK)) {
            pr_info("[I2C] Block read error at byte %d\n", i);
            i2c_flush();
            return -1;
        }
        buf[i] = (uint8_t)(mmio_read32(I2C0_BASE + I2C_DATA) & 0xFF);
        mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_RRDY);
    }

    /* Wait for transfer complete */
    stat = i2c_wait_status(I2C_STAT_ARDY);
    if (stat) {
        mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_ARDY);
    }

    return 0;
}

/*
 * omap_i2c_scan — diagnostic bus probe (0x08 .. 0x77)
 *
 * Sends a one-byte write to each address and reports every ACK. Used at
 * boot to confirm TDA19988 HDMI, PMIC and EEPROM are reachable on I2C0.
 */
static void omap_i2c_scan(void)
{
    uint8_t addr;
    int found = 0;

    pr_info("[I2C] Bus scan starting (0x08-0x77)...\n");

    for (addr = 0x08; addr <= 0x77; addr++) {
        uint32_t stat;

        if (i2c_wait_bus_free() != 0) {
            i2c_flush();
            continue;
        }

        i2c_clear_status();
        mmio_write32(I2C0_BASE + I2C_SA, addr);
        mmio_write32(I2C0_BASE + I2C_CNT, 1);
        mmio_write32(I2C0_BASE + I2C_CON,
                     I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX |
                     I2C_CON_STT | I2C_CON_STP);

        stat = i2c_wait_status(I2C_STAT_XRDY);
        if (!stat || (stat & I2C_STAT_NACK)) {
            mmio_write32(I2C0_BASE + I2C_IRQSTATUS, stat);
            i2c_flush();
            continue;
        }

        /* Write dummy byte */
        mmio_write32(I2C0_BASE + I2C_DATA, 0x00);
        mmio_write32(I2C0_BASE + I2C_IRQSTATUS, I2C_STAT_XRDY);

        stat = i2c_wait_status(I2C_STAT_ARDY);
        if (stat && !(stat & I2C_STAT_NACK)) {
            pr_info("[I2C] FOUND device at 0x%x\n", addr);
            found++;
        }
        mmio_write32(I2C0_BASE + I2C_IRQSTATUS, stat);
        i2c_flush();
    }

    if (!found)
        pr_info("[I2C] Bus scan: no devices found\n");
    else
        pr_info("[I2C] Bus scan complete: %d device(s)\n", found);
}

/*
 * I2C adapter wiring — bridges AM335x I2C0 to the generic i2c-core.
 * master_xfer maps standard i2c_msg sequences onto hardware helpers:
 *   1 msg  write  : register-pointer + payload
 *   2 msgs: write (1 byte) then read — repeated-start register read
 */

static int omap_i2c_xfer(struct i2c_adapter *adap,
                         struct i2c_msg *msgs, int count)
{
    (void)adap;

    if (count == 1 && !(msgs[0].flags & I2C_M_RD) && msgs[0].len >= 2) {
        struct i2c_msg *m = &msgs[0];
        uint8_t reg = m->buf[0];
        for (uint16_t i = 1; i < m->len; i++) {
            if (i2c_write_reg((uint8_t)m->addr, reg + i - 1, m->buf[i]) != 0)
                return -1;
        }
        return 1;
    }

    if (count == 2
        && !(msgs[0].flags & I2C_M_RD) && msgs[0].len == 1
        && (msgs[1].flags & I2C_M_RD)) {
        return (i2c_read_block((uint8_t)msgs[0].addr, msgs[0].buf[0],
                               msgs[1].buf, msgs[1].len) == 0) ? 2 : -1;
    }

    return -1;
}

static const struct i2c_algorithm omap_i2c_algo = {
    .master_xfer = omap_i2c_xfer,
};

static struct i2c_adapter omap_i2c_adapter = {
    .name = "omap-i2c0",
    .nr   = -1,
    .algo = &omap_i2c_algo,
};

static int omap_i2c_probe(struct platform_device *pdev)
{
    struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    pr_info("[I2C] probing %s @ 0x%08x\n",
                pdev->name, mem ? mem->start : 0);

    if (omap_i2c_hw_init() != 0)
        return -1;

    i2c_add_adapter(&omap_i2c_adapter);
    omap_i2c_scan();
    return 0;
}

static struct platform_driver omap_i2c_driver = {
    .drv   = { .name = "omap-i2c" },
    .probe = omap_i2c_probe,
};

static int __init omap_i2c_driver_init(void)
{
    return platform_driver_register(&omap_i2c_driver);
}
subsys_initcall(omap_i2c_driver_init);
