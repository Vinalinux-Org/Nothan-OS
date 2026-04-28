/*
 * <chip> i2c client driver — one-line description.
 *
 * Template for new I2C client drivers (e.g. HDMI encoder, codec, EEPROM,
 * sensor on the I2C bus). Copy this file into
 * vinix-kernel/drivers/<subsystem>/<name>.c and replace every
 * "TEMPLATE:" tag with the real value.
 *
 * NOTE: as of A.2 the i2c client framework (i2c_board_info, i2c_driver
 * registration via initcall, EPROBE_DEFER) is not yet built. This
 * template documents the target shape; revisit once the i2c client
 * core lands.
 *
 * Datasheet: <chip vendor URL or part number>.
 */

#include "types.h"
#include "vinix/i2c.h"
#include "vinix/init.h"
#include "vinix/printk.h"

/* TEMPLATE: chip register map (from datasheet, not TRM). */
#define CHIP_REG_VERSION        0x00
#define CHIP_REG_CONFIG         0x01
#define CHIP_REG_DATA           0x10

struct chip_priv {
    struct i2c_client *client;
    /* TEMPLATE: chip state */
};

static int chip_read_reg(struct i2c_client *client, uint8_t reg, uint8_t *val)
{
    /* TEMPLATE: read 1 byte from chip register */
    return i2c_transfer(client->adapter, client->addr, &reg, 1, val, 1);
}

static int chip_write_reg(struct i2c_client *client, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_transfer(client->adapter, client->addr, buf, 2, NULL, 0);
}

static int chip_probe(struct i2c_client *client)
{
    uint8_t version;

    /* TEMPLATE: defer if a sibling driver this chip depends on is not
     * ready. Example: HDMI encoder needs LCDC pixel clock first. */
    /* if (!sibling_ready())
     *     return -EPROBE_DEFER;
     */

    if (chip_read_reg(client, CHIP_REG_VERSION, &version) < 0)
        return -ENODEV;

    pr_info("[CHIP] detected at i2c addr 0x%02x, ver 0x%02x\n",
            client->addr, version);

    /* TEMPLATE: HW init — write configuration registers per datasheet. */

    /* TEMPLATE: register with subsystem class
     *   register_framebuffer(...) for display encoder
     *   register_sound_card(...)  for audio codec
     *   etc.
     */
    return 0;
}

static int chip_remove(struct i2c_client *client)
{
    (void)client;
    /* TEMPLATE: unwind probe. Safe empty for MVP. */
    return 0;
}

static struct i2c_driver chip_driver = {
    .drv.name = "tda998x",           /* TEMPLATE: must match
                                        i2c_board_info.type in
                                        bbb_i2c0_devices[] */
    .probe    = chip_probe,
    .remove   = chip_remove,
};

static int __init chip_driver_init(void)
{
    return i2c_register_driver(&chip_driver);
}
device_initcall(chip_driver_init);


/*
 * TEMPLATE: corresponding entry to add to
 * vinix-kernel/arch/arm/mach-omap2/board-bbb.c (under
 * bbb_i2c0_devices[]):
 *
 *   static struct i2c_board_info bbb_i2c0_devices[] = {
 *       { .type = "tda998x", .addr = 0x70 },
 *   };
 *
 * The i2c host driver enumerates this array when its adapter
 * registers, instantiates each i2c_client, and matches against
 * registered i2c_drivers by .type vs i2c_driver.drv.name.
 */
