// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus SP7350 SoC Display driver
 * (Builtin RASPBERRYPI 7inch TOUCHSCREEN I2C controller)
 *
 * Author: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 * This raspberrypi 7" touchscreen panel driver is inspired 
 * from the Linux Kernel driver
 * drivers/gpu/drm/panel/panel-raspberrypi-touchscreen.c
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>

#include "sp7350_display.h"
#include "sp7350_disp_regs.h"

extern void sp7350_mipitx_phy_init_dsi(void);

/* I2C registers of the Atmel microcontroller. */
enum REG_ADDR {
	REG_ID = 0x80,
	REG_PORTA, /* BIT(2) for horizontal flip, BIT(3) for vertical flip */
	REG_PORTB,
	REG_PORTC,
	REG_PORTD,
	REG_POWERON,
	REG_PWM,
	REG_DDRA,
	REG_DDRB,
	REG_DDRC,
	REG_DDRD,
	REG_TEST,
	REG_WR_ADDRL,
	REG_WR_ADDRH,
	REG_READH,
	REG_READL,
	REG_WRITEH,
	REG_WRITEL,
	REG_ID2,
};

struct rpi_touchscreen {
	struct i2c_client *i2c;
};

struct rpi_touchscreen *g_ts;

int rpi_touchscreen_i2c_read(struct rpi_touchscreen *ts, u8 reg)
{
	return i2c_smbus_read_byte_data(ts->i2c, reg);
}

void rpi_touchscreen_i2c_write(struct rpi_touchscreen *ts,
				      u8 reg, u8 val)
{
	int ret;

	msleep(10);
	ret = i2c_smbus_write_byte_data(ts->i2c, reg, val);
	if (ret)
		pr_err("I2C write failed: %d\n", ret);
}

static int rpi_touchscreen_probe(struct i2c_client *i2c,
				 const struct i2c_device_id *id)
{
	struct device *dev = &i2c->dev;
	struct rpi_touchscreen *ts;
	int ver;
	int i;

	//pr_info("%s at display driver\n", __func__);

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	i2c_set_clientdata(i2c, ts);

	ts->i2c = i2c;
	g_ts = ts;

	ver = rpi_touchscreen_i2c_read(ts, REG_ID);

	if (ver < 0) {
		dev_err(dev, "Atmel I2C read failed: %d\n", ver);
		return -ENODEV;
	}

	//pr_info("Atmel I2C read ok: ver 0x%02x\n", ver);

	switch (ver) {
	case 0xde: /* ver 1 */
	case 0xc3: /* ver 2 */
		break;
	default:
		dev_err(dev, "Unknown Atmel firmware revision: 0x%02x\n", ver);
		return -ENODEV;
	}

	sp7350_mipitx_phy_init_dsi();

	/* Turn off the backlight. */
	rpi_touchscreen_i2c_write(ts, REG_PWM, 0);
	/* Turn off at boot, so we can cleanly sequence powering on. */
	
	rpi_touchscreen_i2c_write(ts, REG_POWERON, 0);
	/* Turn on sequence powering on. */
	msleep(20);
	rpi_touchscreen_i2c_write(ts, REG_POWERON, 1);
	/* Wait for nPWRDWN to go low to indicate poweron is done. */
	for (i = 0; i < 100; i++) {
		if (rpi_touchscreen_i2c_read(ts, REG_PORTB) & 1)
			break;
	}

	//pr_info("MIPITX DSI Panel : RASPBERRYPI_DSI_PANEL(800x480)\n");
	sp7350_mipitx_phy_init_dsi();

	/* Turn on the backlight. */
	rpi_touchscreen_i2c_write(ts, REG_PWM, 255);

	rpi_touchscreen_i2c_write(ts, REG_PORTA, BIT(2));
	//rpi_touchscreen_i2c_write(ts, REG_PORTA, BIT(3));

	return 0;
}

static int rpi_touchscreen_remove(struct i2c_client *i2c)
{
	struct rpi_touchscreen *ts = i2c_get_clientdata(i2c);

	rpi_touchscreen_i2c_write(ts, REG_PWM, 0);

	rpi_touchscreen_i2c_write(ts, REG_POWERON, 0);
	udelay(1);

	return 0;
}

static const struct of_device_id rpi_touchscreen_of_ids[] = {
	{ .compatible = "raspberrypi,7inch-touchscreen-panel-builtin" },
	{ } /* sentinel */
};
MODULE_DEVICE_TABLE(of, rpi_touchscreen_of_ids);

static struct i2c_driver rpi_touchscreen_driver = {
	.driver = {
		.name = "rpi_touchscreen",
		.of_match_table = rpi_touchscreen_of_ids,
	},
	.probe = rpi_touchscreen_probe,
	.remove = rpi_touchscreen_remove,
};

static int __init rpi_touchscreen_init(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&rpi_touchscreen_driver);
}
module_init(rpi_touchscreen_init);

static void __exit rpi_touchscreen_exit(void)
{
	i2c_del_driver(&rpi_touchscreen_driver);
}
module_exit(rpi_touchscreen_exit);

MODULE_AUTHOR("Hammer Hsieh <hammer.hsieh@sunplus.com>");
MODULE_DESCRIPTION("Raspberry Pi 7-inch touchscreen driver");
MODULE_LICENSE("GPL v2");
