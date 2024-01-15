// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus SP7350 SoC Display driver
 * (Builtin LT8912B BRIDGE I2C controller)
 *
 * Author: Hammer Hsieh <hammer.hsieh@sunplus.com>
 *
 * This lt8912b bridge i2c driver is inspired
 * from the Linux Kernel driver
 * drivers/gpu/drm/bridge/lontium-lt8912b.c
 */

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>

#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>

#include <video/videomode.h>

#define I2C_MAIN 0
#define I2C_ADDR_MAIN 0x48

#define I2C_CEC_DSI 1
#define I2C_ADDR_CEC_DSI 0x49

#define I2C_MAX_IDX 2

struct lt8912 {
	struct device *dev;
	struct i2c_client *i2c_client[I2C_MAX_IDX];
	struct regmap *regmap[I2C_MAX_IDX];

	struct device_node *host_node;
	struct drm_bridge *hdmi_port;

	struct mipi_dsi_device *dsi;

	struct gpio_desc *gp_reset;

	struct videomode mode;
	u32 mode_sel;
	u8 data_lanes;
};

struct lt8912 *g_lt;

int lt8912_write_init_config(struct lt8912 *lt)
{
	const struct reg_sequence seq[] = {
		/* Digital clock en*/
		{0x08, 0xff},
		{0x09, 0xff},
		{0x0a, 0xff},
		{0x0b, 0x7c},
		{0x0c, 0xff},
		{0x42, 0x05},

		/*Tx Analog*/
		{0x31, 0xb1},
		{0x32, 0xb1},
		{0x33, 0x0e},
		{0x37, 0x00},
		{0x38, 0x22},
		{0x60, 0x82},

		/*Cbus Analog*/
		{0x39, 0x45},
		{0x3a, 0x00},
		{0x3b, 0x00},

		/*HDMI Pll Analog*/
		{0x44, 0x31},
		{0x55, 0x44},
		{0x57, 0x01},
		{0x5a, 0x02},

		/*MIPI Analog*/
		{0x3e, 0xd6},
		{0x3f, 0xd4},
		{0x41, 0x3c},
		{0xB2, 0x00},
	};

	return regmap_multi_reg_write(lt->regmap[I2C_MAIN], seq, ARRAY_SIZE(seq));
}

int lt8912_write_mipi_basic_config(struct lt8912 *lt)
{
	const struct reg_sequence seq[] = {
		{0x12, 0x04},
		{0x14, 0x00},
		{0x15, 0x00},
		{0x1a, 0x03},
		{0x1b, 0x03},
	};

	return regmap_multi_reg_write(lt->regmap[I2C_CEC_DSI], seq, ARRAY_SIZE(seq));
};

int lt8912_write_dds_config(struct lt8912 *lt)
{
	const struct reg_sequence seq[] = {
		{0x4e, 0xff},
		{0x4f, 0x56},
		{0x50, 0x69},
		{0x51, 0x80},
		{0x1f, 0x5e},
		{0x20, 0x01},
		{0x21, 0x2c},
		{0x22, 0x01},
		{0x23, 0xfa},
		{0x24, 0x00},
		{0x25, 0xc8},
		{0x26, 0x00},
		{0x27, 0x5e},
		{0x28, 0x01},
		{0x29, 0x2c},
		{0x2a, 0x01},
		{0x2b, 0xfa},
		{0x2c, 0x00},
		{0x2d, 0xc8},
		{0x2e, 0x00},
		{0x42, 0x64},
		{0x43, 0x00},
		{0x44, 0x04},
		{0x45, 0x00},
		{0x46, 0x59},
		{0x47, 0x00},
		{0x48, 0xf2},
		{0x49, 0x06},
		{0x4a, 0x00},
		{0x4b, 0x72},
		{0x4c, 0x45},
		{0x4d, 0x00},
		{0x52, 0x08},
		{0x53, 0x00},
		{0x54, 0xb2},
		{0x55, 0x00},
		{0x56, 0xe4},
		{0x57, 0x0d},
		{0x58, 0x00},
		{0x59, 0xe4},
		{0x5a, 0x8a},
		{0x5b, 0x00},
		{0x5c, 0x34},
		{0x1e, 0x4f},
		{0x51, 0x00},
	};

	return regmap_multi_reg_write(lt->regmap[I2C_CEC_DSI], seq, ARRAY_SIZE(seq));
}

int lt8912_write_rxlogicres_config(struct lt8912 *lt)
{
	int ret;

	ret = regmap_write(lt->regmap[I2C_MAIN], 0x03, 0x7f);
	usleep_range(10000, 20000);
	ret |= regmap_write(lt->regmap[I2C_MAIN], 0x03, 0xff);

	return ret;
};

#if 0
static int lt8912_write_lvds_config(struct lt8912 *lt)
{
	const struct reg_sequence seq[] = {
		{0x44, 0x30},
		{0x51, 0x05},
		{0x50, 0x24},
		{0x51, 0x2d},
		{0x52, 0x04},
		{0x69, 0x0e},
		{0x69, 0x8e},
		{0x6a, 0x00},
		{0x6c, 0xb8},
		{0x6b, 0x51},
		{0x04, 0xfb},
		{0x04, 0xff},
		{0x7f, 0x00},
		{0xa8, 0x13},
		{0x02, 0xf7},
		{0x02, 0xff},
		{0x03, 0xcf},
		{0x03, 0xff},
	};

	return regmap_multi_reg_write(lt->regmap[I2C_CEC_DSI], seq, ARRAY_SIZE(seq));
};
#endif

static const struct regmap_config lt8912_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
};

static int lt8912_init_i2c(struct lt8912 *lt, struct i2c_client *client)
{
	unsigned int i;
	/*
	 * At this time we only initialize 2 chips, but the lt8912 provides
	 * a third interface for the audio over HDMI configuration.
	 */
	struct i2c_board_info info[] = {
		{ I2C_BOARD_INFO("lt8912p0", I2C_ADDR_MAIN), },
		{ I2C_BOARD_INFO("lt8912p1", I2C_ADDR_CEC_DSI), },
	};

	if (!lt)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(info); i++) {
		if (i > 0) {
			lt->i2c_client[i] = i2c_new_dummy_device(client->adapter,
								 info[i].addr);
			if (IS_ERR(lt->i2c_client[i]))
				return PTR_ERR(lt->i2c_client[i]);
		}

		lt->regmap[i] = devm_regmap_init_i2c(lt->i2c_client[i],
						     &lt8912_regmap_config);
		if (IS_ERR(lt->regmap[i]))
			return PTR_ERR(lt->regmap[i]);
	}
	return 0;
}

static int lt8912_free_i2c(struct lt8912 *lt)
{
	unsigned int i;

	for (i = 1; i < I2C_MAX_IDX; i++)
		i2c_unregister_device(lt->i2c_client[i]);

	return 0;
}

int lt8912_video_setup(struct lt8912 *lt)
{
	u32 hactive, h_total, hpw, hfp, hbp;
	u32 vactive, v_total, vpw, vfp, vbp;
	u8 settle = 0x08;
	int ret;

	if (!lt)
		return -EINVAL;

	hactive = lt->mode.hactive;
	hfp = lt->mode.hfront_porch;
	hpw = lt->mode.hsync_len;
	hbp = lt->mode.hback_porch;
	h_total = hactive + hfp + hpw + hbp;

	vactive = lt->mode.vactive;
	vfp = lt->mode.vfront_porch;
	vpw = lt->mode.vsync_len;
	vbp = lt->mode.vback_porch;
	v_total = vactive + vfp + vpw + vbp;

	if (vactive <= 600)
		settle = 0x04;
	else if (vactive == 1080)
		settle = 0x0a;

	ret = regmap_write(lt->regmap[I2C_CEC_DSI], 0x10, 0x01);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x11, settle);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x18, hpw);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x19, vpw);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x1c, hactive & 0xff);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x1d, hactive >> 8);

	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x2f, 0x0c);

	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x34, h_total & 0xff);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x35, h_total >> 8);

	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x36, v_total & 0xff);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x37, v_total >> 8);

	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x38, vbp & 0xff);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x39, vbp >> 8);

	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x3a, vfp & 0xff);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x3b, vfp >> 8);

	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x3c, hbp & 0xff);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x3d, hbp >> 8);

	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x3e, hfp & 0xff);
	ret |= regmap_write(lt->regmap[I2C_CEC_DSI], 0x3f, hfp >> 8);

	return ret;
}

int lt8912_soft_power_on(struct lt8912 *lt)
{
	u32 lanes = 0;

	lanes = lt->data_lanes;

	lt8912_write_init_config(lt);
	regmap_write(lt->regmap[I2C_CEC_DSI], 0x13, lanes & 3);

	lt8912_write_mipi_basic_config(lt);

	return 0;
}

/*
 * lt8912_input_timing[x][y]
 * y = 0-1, LT8912 width & height
 * y = 2-9, LT8912 HSA & HFP & HBP & HACT & VSA & VFP & VBP & VACT
 */
static const u32 lt8912_input_timing[11][10] = {
	/* (w   h)   HSA   HFP   HBP   HACT  VSA  VFP  VBP   VACT */
	{ 720,  480, 0x3e, 0x3c, 0x10,  720, 0x6, 0x9, 0x1E,  480}, /* 480P */
	{ 720,  576, 0x80, 0x28, 0x58,  720, 0x4, 0x1, 0x17,  576}, /* 576P */
	{1280,  720, 0x28, 0xdc, 0x6e, 1280, 0x5, 0x5, 0x14,  720}, /* 720P */
	{1920, 1080, 0x2c, 0x94, 0x58, 1920, 0x5, 0x4, 0x24, 1080}, /* 1080P */
	{  64,   64, 0x14, 0x28, 0x58,   64, 0x5, 0x5, 0x24,   64}, /* 64x64 */
	{ 128,  128, 0x14, 0x28, 0x58,  128, 0x5, 0x5, 0x24,  128}, /* 128x128 */
	{  64, 2880, 0x14, 0x28, 0x58,   64, 0x5, 0x5, 0x24, 2880}, /* 64x2880 */
	{3840,   64, 0x14, 0x28, 0x58, 3840, 0x5, 0x5, 0x24,   64}, /* 3840x64 */
	{3840, 2880, 0x14, 0x28, 0x58, 3840, 0x5, 0x5, 0x24, 2880}, /* 3840x2880 */
	{ 800,  480, 0x14, 0x28, 0x58,  800, 0x5, 0x5, 0x24,  480}, /* 800x480 */
	{1024,  600, 0x14, 0x28, 0x58, 1024, 0x5, 0x5, 0x24,  600}  /* 1024x600 */
};

int lt8912_video_on(struct lt8912 *lt)
{
	int ret;
	u32 i;

	i = lt->mode_sel;
	if (i > 3) i = 3;

	lt->mode.hactive = lt8912_input_timing[i][5];
	lt->mode.hfront_porch = lt8912_input_timing[i][3];
	lt->mode.hsync_len = lt8912_input_timing[i][2];
	lt->mode.hback_porch = lt8912_input_timing[i][4];

	lt->mode.vactive = lt8912_input_timing[i][9];
	lt->mode.vfront_porch = lt8912_input_timing[i][7];
	lt->mode.vsync_len = lt8912_input_timing[i][6];
	lt->mode.vback_porch = lt8912_input_timing[i][8];

	ret = lt8912_video_setup(lt);
	if (ret < 0)
		goto end;

	ret = lt8912_write_dds_config(lt);
	if (ret < 0)
		goto end;

	ret = lt8912_write_rxlogicres_config(lt);
	if (ret < 0)
		goto end;

	//ret = lt8912_write_lvds_config(lt);
	//if (ret < 0)
	//	goto end;

end:
	return ret;
}

static int lt8912_parse_dt(struct lt8912 *lt)
{
	struct gpio_desc *gp_reset;
	struct device *dev = lt->dev;
	int ret;
	int data_lanes;
	u32 mode;
	struct device_node *port_node;
	struct device_node *endpoint;

	gp_reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(gp_reset)) {
		ret = PTR_ERR(gp_reset);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get reset gpio: %d\n", ret);
		return ret;
	}
	lt->gp_reset = gp_reset;

	ret = of_property_read_u32(dev->of_node, "hdmi-timing", &mode);
	if (ret)
		lt->mode_sel = 0;
	else
		lt->mode_sel = mode;

	endpoint = of_graph_get_endpoint_by_regs(dev->of_node, 0, -1);
	if (!endpoint)
		return -ENODEV;

	data_lanes = of_property_count_u32_elems(endpoint, "data-lanes");
	of_node_put(endpoint);
	if (data_lanes < 0) {
		dev_err(lt->dev, "%s: Bad data-lanes property\n", __func__);
		return data_lanes;
	}
	lt->data_lanes = data_lanes;

	lt->host_node = of_graph_get_remote_node(dev->of_node, 0, -1);
	if (!lt->host_node) {
		dev_err(lt->dev, "%s: Failed to get remote port\n", __func__);
		return -ENODEV;
	}

	port_node = of_graph_get_remote_node(dev->of_node, 1, -1);
	if (!port_node) {
		dev_err(lt->dev, "%s: Failed to get connector port\n", __func__);
		ret = -ENODEV;
		goto err_free_host_node;
	}

	//lt->hdmi_port = of_drm_find_bridge(port_node);
	//if (!lt->hdmi_port) {
	//	dev_err(lt->dev, "%s: Failed to get hdmi port\n", __func__);
	//	ret = -ENODEV;
	//	goto err_free_host_node;
	//}

	if (!of_device_is_compatible(port_node, "hdmi-connector")) {
		dev_err(lt->dev, "%s: Failed to get hdmi port\n", __func__);
		ret = -EINVAL;
		goto err_free_host_node;
	}

	of_node_put(port_node);
	return 0;

err_free_host_node:
	of_node_put(port_node);
	of_node_put(lt->host_node);
	return ret;
}

static int lt8912_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	static struct lt8912 *lt;
	int ret = 0;
	struct device *dev = &client->dev;

	lt = devm_kzalloc(dev, sizeof(struct lt8912), GFP_KERNEL);
	if (!lt)
		return -ENOMEM;

	lt->dev = dev;
	lt->i2c_client[0] = client;

	g_lt = lt;

	ret = lt8912_parse_dt(lt);
	if (ret)
		goto err_dt_parse;

	ret = lt8912_init_i2c(lt, client);
	if (ret)
		goto err_i2c;

	i2c_set_clientdata(client, lt);

	lt8912_soft_power_on(lt);

	lt8912_video_on(lt);

	pr_info("sp7350_disp_%s\n", __func__);

	return 0;

err_i2c:
err_dt_parse:
	return ret;
}

static int lt8912_remove(struct i2c_client *client)
{
	struct lt8912 *lt = i2c_get_clientdata(client);

	lt8912_free_i2c(lt);
	return 0;
}

static const struct of_device_id lt8912_dt_match[] = {
	{.compatible = "lontium,lt8912b-builtin"},
	{}
};
MODULE_DEVICE_TABLE(of, lt8912_dt_match);

static const struct i2c_device_id lt8912_id[] = {
	{"lt8912b", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, lt8912_id);

static struct i2c_driver lt8912_i2c_driver = {
	.driver = {
		.name = "lt8912b",
		.of_match_table = lt8912_dt_match,
		.owner = THIS_MODULE,
	},
	.probe = lt8912_probe,
	.remove = lt8912_remove,
	.id_table = lt8912_id,
};
module_i2c_driver(lt8912_i2c_driver);

MODULE_AUTHOR("Hammer Hsieh <hammer.hsieh@sunplus.com>");
MODULE_DESCRIPTION("lt8912 i2c driver");
MODULE_LICENSE("GPL v2");
