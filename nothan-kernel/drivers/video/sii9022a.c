/*
 * drivers/video/sii9022a.c - SiI9022A HDMI framer driver
 *
 * Configures the SiI9022A via I2C2 for 1280x720@60Hz HDMI output.
 * The SiI9022A receives parallel RGB from the LCDC and outputs HDMI.
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#include <nothan/types.h>
#include <nothan/i2c.h>
#include <nothan/printk.h>
#include <nothan/init.h>

/* TPI registers */
#define SII_TPI_VIDEO_DATA  0x00    /* 10-byte video format block */
#define SII_TPI_SYS_CTRL    0x1A    /* system control */
#define SII_CHIP_ID         0x1B    /* should read 0xB0 */
#define SII_PWR_STATE       0x1E    /* AVI power state D0/D2/D3 */
#define SII_TPI_RQB         0xC7    /* enter TPI transmitter mode */

/* SII_TPI_SYS_CTRL bits */
#define SYS_CTRL_OUTPUT_HDMI    (1 << 0)

/* SII_TPI_VIDEO_DATA bus format byte (offset 8 in the block) */
#define PIX_REP_BUS_24BIT       (1 << 5)
#define PIX_REP_CLK_RATIO_1X    (1 << 6)

/* 720p@60Hz: pixel clock 74.25 MHz = 7425 × 10 kHz */
#define PCLK_10KHZ  7425U
#define VREFRESH_HZ 60U
#define HDISPLAY    1280U
#define VDISPLAY    720U

static int sii9022a_probe(struct i2c_client *client)
{
	u8 chip_id;
	u8 buf[11] = {
		SII_TPI_VIDEO_DATA,
		(u8)(PCLK_10KHZ & 0xFF),
		(u8)(PCLK_10KHZ >> 8),
		(u8)VREFRESH_HZ,
		0x00,
		(u8)(HDISPLAY & 0xFF),
		(u8)(HDISPLAY >> 8),
		(u8)(VDISPLAY & 0xFF),
		(u8)(VDISPLAY >> 8),
		PIX_REP_CLK_RATIO_1X | PIX_REP_BUS_24BIT,
		0x00,   /* input: RGB, auto range */
	};
	int ret;

	/* Enter TPI transmitter mode */
	ret = i2c_smbus_write_byte_data(client, SII_TPI_RQB, 0x00);
	if (ret < 0) {
		printk("[SII9022A] TPI mode request failed\n");
		return ret;
	}

	/* Verify chip ID */
	ret = i2c_smbus_read_byte_data(client, SII_CHIP_ID, &chip_id);
	if (ret < 0) {
		printk("[SII9022A] chip ID read failed\n");
		return ret;
	}
	if (chip_id != 0xB0) {
		printk("[SII9022A] unexpected chip ID: 0x%02x\n", chip_id);
		return -1;
	}
	printk("[SII9022A] chip ID 0xB0 OK\n");

	/* Write video format block: pixel clock, refresh, resolution, bus format */
	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret < 0) {
		printk("[SII9022A] video data write failed\n");
		return ret;
	}

	/* Set HDMI output mode */
	ret = i2c_smbus_write_byte_data(client, SII_TPI_SYS_CTRL,
					SYS_CTRL_OUTPUT_HDMI);
	if (ret < 0) {
		printk("[SII9022A] HDMI mode set failed\n");
		return ret;
	}

	/* Power state D0 (full operation) */
	ret = i2c_smbus_write_byte_data(client, SII_PWR_STATE, 0x00);
	if (ret < 0) {
		printk("[SII9022A] power state set failed\n");
		return ret;
	}

	printk("[SII9022A] HDMI 720p@60Hz configured\n");
	return 0;
}

static struct i2c_driver sii9022a_driver = {
	.name  = "sii9022a",
	.probe = sii9022a_probe,
};

static int __init sii9022a_init(void)
{
	return i2c_add_driver(&sii9022a_driver);
}
device_initcall(sii9022a_init);
