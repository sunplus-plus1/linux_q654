// SPDX-License-Identifier: GPL-2.0
/*
 * Sunplus SP7350 DRM dsi panel simple driver.
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/videomode.h>

#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_print.h>
#include <drm/drm_panel.h>

/* always keep 0 */
#define SP7350_DRM_TODO    0

struct sp7350_dsi_panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;
	unsigned int bpc;

	/**
	 * @width_mm: width of the panel's active display area
	 * @height_mm: height of the panel's active display area
	 */
	struct {
		unsigned int width_mm;
		unsigned int height_mm;
	} size;

	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	const struct panel_init_cmd *init_cmds;
	unsigned int lanes;
};

struct sp7350_panel_simple_dsi {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;
	const struct sp7350_dsi_panel_desc *desc;
	enum drm_panel_orientation orientation;
	//struct backlight_device *bl_dev;

	bool prepared;
	bool enabled;

	struct regulator *supply;
	struct i2c_adapter *ddc;

	struct gpio_desc *enable_gpio;
};

enum dsi_cmd_type {
	INIT_DCS_CMD,
	DELAY_CMD,
};

struct panel_init_cmd {
	enum dsi_cmd_type type;
	size_t len;
	const char *data;
};

#define _INIT_DCS_CMD(...) { \
	.type = INIT_DCS_CMD, \
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

#define _INIT_DELAY_CMD(...) { \
	.type = DELAY_CMD,\
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

static const struct panel_init_cmd lx_hxm0686tft_001_init_cmd[] = {
	_INIT_DCS_CMD(0xB9, 0xF1, 0x12, 0x87),
	_INIT_DCS_CMD(0xB2, 0x40, 0x05, 0x78),
	_INIT_DCS_CMD(0xB3, 0x10, 0x10, 0x28, 0x28, 0x03, 0xFF, 0x00,
		      0x00, 0x00, 0x00),
	_INIT_DCS_CMD(0xB4, 0x80),
	_INIT_DCS_CMD(0xB5, 0x09, 0x09),
	_INIT_DCS_CMD(0xB6, 0x8D, 0x8D),
	_INIT_DCS_CMD(0xB8, 0x26, 0x22, 0xF0, 0x63),
	_INIT_DCS_CMD(0xBA, 0x33, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44,
		      0x25, 0x00, 0x91, 0x0A, 0x00, 0x00, 0x01, 0x4F,
		      0x01, 0x00, 0x00, 0x37),
	_INIT_DCS_CMD(0xBC, 0x47),
	_INIT_DCS_CMD(0xBF, 0x02, 0x10, 0x00, 0x80, 0x04),
	_INIT_DCS_CMD(0xC0, 0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x12,
		      0x70, 0x00),
	_INIT_DCS_CMD(0xC1, 0x65, 0xC0, 0x32, 0x32, 0x77, 0xF4, 0x77,
		      0x77, 0xCC, 0xCC, 0xFF, 0xFF, 0x11, 0x11, 0x00,
		      0x00, 0x32),
	_INIT_DCS_CMD(0xC7, 0x10, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0xED, 0xC7, 0x00, 0xA5),
	_INIT_DCS_CMD(0xC8, 0x10, 0x40, 0x1E, 0x03),
	_INIT_DCS_CMD(0xCC, 0x0B),
	_INIT_DCS_CMD(0xE0, 0x00, 0x04, 0x06, 0x32, 0x3F, 0x3F, 0x36,
		      0x34, 0x06, 0x0B, 0x0E, 0x11, 0x11, 0x10, 0x12,
		      0x10, 0x13, 0x00, 0x04, 0x06, 0x32, 0x3F, 0x3F,
		      0x36, 0x34, 0x06, 0x0B, 0x0E, 0x11, 0x11, 0x10,
		      0x12, 0x10, 0x13),
	_INIT_DCS_CMD(0xE1, 0x11, 0x11, 0x91, 0x00, 0x00, 0x00, 0x00),
	_INIT_DCS_CMD(0xE3, 0x03, 0x03, 0x0B, 0x0B, 0x00, 0x03, 0x00,
		      0x00, 0x00, 0x00, 0xFF, 0x84, 0xC0, 0x10),
	_INIT_DCS_CMD(0xE9, 0xC8, 0x10, 0x06, 0x05, 0x18, 0xD2, 0x81,
		      0x12, 0x31, 0x23, 0x47, 0x82, 0xB0, 0x81, 0x23,
		      0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00,
		      0x04, 0x04, 0x00, 0x00, 0x00, 0x88, 0x0B, 0xA8,
		      0x10, 0x32, 0x4F, 0x88, 0x88, 0x88, 0x88, 0x88,
		      0x88, 0x0B, 0xA8, 0x10, 0x32, 0x4F, 0x88, 0x88,
		      0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
	_INIT_DCS_CMD(0xEA, 0x96, 0x0C, 0x01, 0x01, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x4B, 0xA8,
		      0x23, 0x01, 0x08, 0xF8, 0x88, 0x88, 0x88, 0x88,
		      0x88, 0x4B, 0xA8, 0x23, 0x01, 0x08, 0xF8, 0x88,
		      0x88, 0x88, 0x88, 0x23, 0x10, 0x00, 0x00, 0x92,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
		      0x80, 0x81, 0x00, 0x00, 0x00, 0x00),
	_INIT_DCS_CMD(0xEF, 0xFF, 0xFF, 0x01),

	_INIT_DCS_CMD(0x36, 0x14),
	_INIT_DCS_CMD(0x35, 0x00),
	_INIT_DCS_CMD(0x11),
	_INIT_DELAY_CMD(120),
	_INIT_DCS_CMD(0x29),
	_INIT_DELAY_CMD(20),
	{},
};

static const struct panel_init_cmd xx_tcxd024iblon_n2_init_cmd[] = {
	_INIT_DCS_CMD(0xDF, 0x98, 0x51, 0xE9),

	/* Page 00 setting */
	_INIT_DCS_CMD(0xDE, 0x00),
	_INIT_DCS_CMD(0xB7, 0x22, 0x85, 0x22, 0x2B),
	_INIT_DCS_CMD(0xC8, 0x3F, 0x38, 0x33, 0x2F, 0x32, 0x34, 0x2F,
		      0x2F, 0x2D, 0x2C, 0x27, 0x1A, 0x14, 0x0A, 0x06,
		      0x0E, 0x3F, 0x38, 0x33, 0x2F, 0x32, 0x34, 0x2F,
		      0x2F, 0x2D, 0x2C, 0x27, 0x1A, 0x14, 0x0A, 0x06,
		      0x0E),
	_INIT_DCS_CMD(0xB9, 0x33, 0x08, 0xCC),
	_INIT_DCS_CMD(0xBB, 0x46, 0x7A, 0x30, 0x40, 0x7C, 0x60, 0x70,
		      0x70),
	_INIT_DCS_CMD(0xBC, 0x38, 0x3C),
	_INIT_DCS_CMD(0xC0, 0x31, 0x20),
	_INIT_DCS_CMD(0xC1, 0x12),
	_INIT_DCS_CMD(0xC3, 0x08, 0x00, 0x0A, 0x10, 0x08, 0x54, 0x45,
		      0x71, 0x2C),
	_INIT_DCS_CMD(0xC4, 0x00, 0xA0, 0x79, 0x0E, 0x0A, 0x16, 0x79,
		      0x0E, 0x0A, 0x16, 0x79, 0x0E, 0x0A, 0x16, 0x82,
		      0x00, 0x03),
	_INIT_DCS_CMD(0xD0, 0x04, 0x0C, 0x6B, 0x0F, 0x07, 0x03),
	_INIT_DCS_CMD(0xD7, 0x13, 0x00),

	/* Page 02 setting */
	_INIT_DCS_CMD(0xDE, 0x02),
	_INIT_DELAY_CMD(1),
	_INIT_DCS_CMD(0xB8, 0x1D, 0xA0, 0x2F, 0x04, 0x33),
	_INIT_DCS_CMD(0xC1, 0x10, 0x66, 0x66, 0x01),

	/* Page 00 setting */
	_INIT_DCS_CMD(0xDE, 0x00),
	//_INIT_DCS_CMD(0xC2, 0x08), /* BIST_EN */
	_INIT_DCS_CMD(0x11),
	_INIT_DELAY_CMD(120),

	/* Page 02 setting */
	_INIT_DCS_CMD(0xDE, 0x02),
	_INIT_DELAY_CMD(1),
	_INIT_DCS_CMD(0xC5, 0x4E, 0x00, 0x00),
	_INIT_DELAY_CMD(1),
	_INIT_DCS_CMD(0xCA, 0x30, 0x20, 0xF4),
	_INIT_DELAY_CMD(1),

	/* Page 04 setting */
	_INIT_DCS_CMD(0xDE, 0x04),
	_INIT_DELAY_CMD(1),
	_INIT_DCS_CMD(0xD3, 0x3C),
	_INIT_DELAY_CMD(1),

	/* Page 00 setting */
	_INIT_DCS_CMD(0xDE, 0x00),
	_INIT_DELAY_CMD(1),
	_INIT_DCS_CMD(0x29),
	{},
};

static const struct drm_display_mode lx_hxm0686tft_001_modes[] = {
	{
		/* from specification. */
		.clock = 40000,
		.hdisplay    = 480,
		.hsync_start = 480 + 16,
		.hsync_end   = 480 + 16 + 4,
		.htotal      = 480 + 16 + 4 + 12,
		.vdisplay    = 1280,
		.vsync_start = 1280 + 16,
		.vsync_end   = 1280 + 16 + 4,
		.vtotal      = 1280 + 16 + 4 + 12,
	},
	{
	/* from sp7350 display driver: */
		/* (w   h)   HSA  HFP HBP HACT VSA  VFP  VBP   VACT */
	/*  { 480, 1280, 0x4,  0, 0x4,  0, 0x1,0x10, 0x10, 1280},  */
		.clock = 38500,
		.hdisplay    = 480,
		.hsync_start = 480 + 0,
		.hsync_end   = 480 + 0 + 4,
		.htotal      = 480 + 0 + 4 + 4,
		.vdisplay    = 1280,
		.vsync_start = 1280 + 16,
		.vsync_end   = 1280 + 16 + 1,
		.vtotal      = 1280 + 16 + 1 + 16,
	},
};

static const struct sp7350_dsi_panel_desc lx_hxm0686tft_001_desc = {
	.modes = lx_hxm0686tft_001_modes,
	.num_modes = ARRAY_SIZE(lx_hxm0686tft_001_modes),
	.bpc = 8,
	.size = {
		.width_mm = 60,
		.height_mm = 160,
	},
	.lanes = 4,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		      MIPI_DSI_MODE_LPM,
	.init_cmds = lx_hxm0686tft_001_init_cmd,
};

/* from sp7350 display driver. */
	/* (w   h)   HSA  HFP HBP HACT VSA  VFP  VBP   VACT */
/*	{ 240,  320, 0x4,  0, 0x5,  0, 0x1, 0x8, 0x19, 320},  */
static const struct drm_display_mode xx_tcxd024iblon_n2_mode = {
	.clock = 5300,
	.hdisplay = 240,
	.hsync_start = 240 + 0,
	.hsync_end = 240 + 0 + 4,
	.htotal = 240 + 0 + 4 + 5,
	.vdisplay = 320,
	.vsync_start = 320 + 8,
	.vsync_end = 320 + 8 + 1,
	.vtotal = 320 + 8 + 1 + 25,
};

/* FIXME: size mm not truth!!! */
static const struct sp7350_dsi_panel_desc xx_tcxd024iblon_n2_desc = {
	.modes = &xx_tcxd024iblon_n2_mode,
	.num_modes = 1,
	.bpc = 8,
	.size = {
		.width_mm = 30,
		.height_mm = 40,
	},
	.lanes = 1,
	.format = MIPI_DSI_FMT_RGB888,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
		      MIPI_DSI_MODE_LPM,
	.init_cmds = xx_tcxd024iblon_n2_init_cmd,
};

static inline struct sp7350_panel_simple_dsi *to_simple_panel(struct drm_panel *panel)
{
	return container_of(panel, struct sp7350_panel_simple_dsi, base);
}

static unsigned int sp7350_panel_simple_get_display_modes(struct sp7350_panel_simple_dsi *panel,
							  struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	unsigned int i, num = 0;

	for (i = 0; i < panel->desc->num_modes; i++) {
		const struct drm_display_mode *m = &panel->desc->modes[i];

		mode = drm_mode_duplicate(connector->dev, m);
		if (!mode) {
			dev_err(panel->base.dev, "failed to add mode %ux%u@%u\n",
				m->hdisplay, m->vdisplay,
				drm_mode_vrefresh(m));
			continue;
		}
		//DRM_DEV_DEBUG_DRIVER(panel->base.dev, "add mode %ux%u\n", m->hdisplay, m->vdisplay);

		mode->type |= DRM_MODE_TYPE_DRIVER;

		if (i == 1/*panel->desc->num_modes == 1*/)
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		drm_mode_set_name(mode);

		drm_mode_probed_add(connector, mode);
		num++;
	}

	return num;
}

static int sp7350_panel_init_dcs_cmd(struct sp7350_panel_simple_dsi *panel)
{
	struct mipi_dsi_device *dsi = panel->dsi;
	int i, err = 0;

	if (panel->desc->init_cmds) {
		const struct panel_init_cmd *init_cmds = panel->desc->init_cmds;

		for (i = 0; init_cmds[i].len != 0; i++) {
			const struct panel_init_cmd *cmd = &init_cmds[i];

			switch (cmd->type) {
			case DELAY_CMD:
				msleep(cmd->data[0]);
				err = 0;
				break;

			case INIT_DCS_CMD:
				err = mipi_dsi_dcs_write(dsi, cmd->data[0],
							 cmd->len <= 1 ? NULL :
							 &cmd->data[1],
							 cmd->len - 1);
				break;

			default:
				err = -EINVAL;
			}

			if (err < 0) {
				dev_err(&dsi->dev,
					"failed to write command %u\n", i);
				return err;
			}
		}
	}
	return 0;
}

static int sp7350_panel_simple_disable(struct drm_panel *panel)
{
	struct sp7350_panel_simple_dsi *sp7350_panel = to_simple_panel(panel);
	//struct mipi_dsi_device *dsi = to_mipi_dsi_device(panel->dev);
	int ret;

	DRM_DEV_DEBUG_DRIVER(panel->dev, "start.\n");
	if (!sp7350_panel->enabled)
		return 0; /* This is not an issue so we return 0 here */

	//backlight_disable(sp7350_panel->bl_dev);

	ret = mipi_dsi_dcs_set_display_off(sp7350_panel->dsi);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_enter_sleep_mode(sp7350_panel->dsi);
	if (ret)
		return ret;

	msleep(120);

	sp7350_panel->enabled = false;

	return 0;
}

static int sp7350_panel_simple_unprepare(struct drm_panel *panel)
{
	struct sp7350_panel_simple_dsi *sp7350_panel = to_simple_panel(panel);

	DRM_DEV_DEBUG_DRIVER(panel->dev, "start.\n");
	if (!sp7350_panel->prepared)
		return 0;

	if (sp7350_panel->enable_gpio) {
		gpiod_set_value_cansleep(sp7350_panel->enable_gpio, 0);
		msleep(20);
	}

	regulator_disable(sp7350_panel->supply);

	sp7350_panel->prepared = false;

	return 0;
}

static int sp7350_panel_simple_prepare(struct drm_panel *panel)
{
	struct sp7350_panel_simple_dsi *sp7350_panel = to_simple_panel(panel);
	int ret;

	DRM_DEV_DEBUG_DRIVER(panel->dev, "start.\n");
	if (sp7350_panel->prepared)
		return 0;

	ret = regulator_enable(sp7350_panel->supply);
	if (ret < 0) {
		dev_err(panel->dev, "failed to enable supply: %d\n", ret);
		return ret;
	}

	if (sp7350_panel->enable_gpio) {
		gpiod_set_value_cansleep(sp7350_panel->enable_gpio, 0);
		msleep(20);
		gpiod_set_value_cansleep(sp7350_panel->enable_gpio, 1);
	}

	ret = sp7350_panel_init_dcs_cmd(sp7350_panel);
	if (ret < 0) {
		dev_err(panel->dev, "failed to init panel: %d\n", ret);
		goto poweroff;
	}

	sp7350_panel->prepared = true;

	return 0;

poweroff:
	regulator_disable(sp7350_panel->supply);

	if (sp7350_panel->enable_gpio) {
		gpiod_set_value_cansleep(sp7350_panel->enable_gpio, 0);
		msleep(20);
	}

	return ret;
}

static int sp7350_panel_simple_enable(struct drm_panel *panel)
{
	struct sp7350_panel_simple_dsi *sp7350_panel = to_simple_panel(panel);

	DRM_DEV_DEBUG_DRIVER(panel->dev, "start.\n");
	if (sp7350_panel->enabled)
		return 0;

	//backlight_enable(sp7350_panel->bl_dev);

	sp7350_panel->enabled = true;

	return 0;
}

static int sp7350_panel_simple_get_modes(struct drm_panel *panel,
					 struct drm_connector *connector)
{
	struct sp7350_panel_simple_dsi *sp7350_panel = to_simple_panel(panel);
	int num = 0;

	/* probe EDID if a DDC bus is available */
	if (sp7350_panel->ddc) {
		struct edid *edid = drm_get_edid(connector, sp7350_panel->ddc);

		drm_connector_update_edid_property(connector, edid);
		if (edid) {
			num += drm_add_edid_modes(connector, edid);
			kfree(edid);
		}
	}
	num += sp7350_panel_simple_get_display_modes(sp7350_panel, connector);

	connector->display_info.width_mm = sp7350_panel->desc->size.width_mm;
	connector->display_info.height_mm = sp7350_panel->desc->size.height_mm;
	connector->display_info.bpc = sp7350_panel->desc->bpc;
	drm_connector_set_panel_orientation(connector, sp7350_panel->orientation);

	return num;
}

static const struct drm_panel_funcs sp7350_panel_funcs = {
	.disable   = sp7350_panel_simple_disable,
	.unprepare = sp7350_panel_simple_unprepare,
	.prepare   = sp7350_panel_simple_prepare,
	.enable    = sp7350_panel_simple_enable,
	.get_modes = sp7350_panel_simple_get_modes,
};

/*
 * DSI-BASED BACKLIGHT
 */
#if SP7350_DRM_TODO
static int sp7350_panel_simple_backlight_update_status(struct backlight_device *bd)
{
	return 0;
}

static const struct backlight_ops sp7350_panel_simple_backlight_ops = {
	.update_status = sp7350_panel_simple_backlight_update_status,
};
#endif

static int sp7350_panel_simple_add(struct sp7350_panel_simple_dsi *panel)
{
	struct device *dev = &panel->dsi->dev;
	struct device_node *ddc;
	int err;

	panel->supply = devm_regulator_get(dev, "power");
	if (IS_ERR(panel->supply))
		return PTR_ERR(panel->supply);

	panel->enable_gpio = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(panel->enable_gpio)) {
		dev_err(dev, "cannot get enable-gpios %ld\n",
			PTR_ERR(panel->enable_gpio));
		return PTR_ERR(panel->enable_gpio);
	}
	if (panel->enable_gpio)
		gpiod_set_value_cansleep(panel->enable_gpio, 0);

	ddc = of_parse_phandle(dev->of_node, "ddc-i2c-bus", 0);
	if (ddc) {
		panel->ddc = of_find_i2c_adapter_by_node(ddc);
		of_node_put(ddc);

		if (!panel->ddc)
			return -EPROBE_DEFER;
	}

	drm_panel_init(&panel->base, dev, &sp7350_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	err = of_drm_get_panel_orientation(dev->of_node, &panel->orientation);
	if (err < 0) {
		dev_err(dev, "%p: failed to get orientation %d\n", dev->of_node, err);
		return err;
	}

	err = drm_panel_of_backlight(&panel->base);
	if (err)
		return err;

	drm_panel_add(&panel->base);

	return 0;
}

static int sp7350_panel_simple_dsi_probe(struct mipi_dsi_device *dsi)
{
	struct sp7350_panel_simple_dsi *panel;
	int ret;
	const struct sp7350_dsi_panel_desc *desc;

	panel = devm_kzalloc(&dsi->dev, sizeof(*panel), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	desc = of_device_get_match_data(&dsi->dev);
	dsi->lanes = desc->lanes;
	dsi->format = desc->format;
	dsi->mode_flags = desc->mode_flags;
	panel->desc = desc;
	panel->dsi = dsi;

	ret = sp7350_panel_simple_add(panel);
	if (ret < 0)
		return ret;

	mipi_dsi_set_drvdata(dsi, panel);

	ret = mipi_dsi_attach(dsi);
	if (ret)
		drm_panel_remove(&panel->base);

	DRM_DEV_DEBUG_DRIVER(&dsi->dev, "finish.\n");

	return ret;
}

static void sp7350_panel_simple_dsi_shutdown(struct mipi_dsi_device *dsi)
{
	struct sp7350_panel_simple_dsi *panel = mipi_dsi_get_drvdata(dsi);

	drm_panel_disable(&panel->base);
	drm_panel_unprepare(&panel->base);
}

static int sp7350_panel_simple_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct sp7350_panel_simple_dsi *panel = mipi_dsi_get_drvdata(dsi);
	int ret;

	sp7350_panel_simple_dsi_shutdown(dsi);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to detach from DSI host: %d\n", ret);

	if (panel->base.dev)
		drm_panel_remove(&panel->base);

	return 0;
}

static const struct of_device_id panel_simple_dsi_of_match[] = {
	{
		.compatible = "lx,hxm0686tft-001", .data = &lx_hxm0686tft_001_desc
	}, {
		.compatible = "fitipower,tcxd024iblon-2", .data = &xx_tcxd024iblon_n2_desc
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, panel_simple_dsi_of_match);

static struct mipi_dsi_driver panel_simple_dsi_driver = {
	.driver = {
		.name = "panel-simple-dsi",
		.of_match_table = panel_simple_dsi_of_match,
	},
	.probe = sp7350_panel_simple_dsi_probe,
	.remove = sp7350_panel_simple_dsi_remove,
	.shutdown = sp7350_panel_simple_dsi_shutdown,
};

module_mipi_dsi_driver(panel_simple_dsi_driver);

MODULE_AUTHOR("dx.jiang <dx.jiang@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus SP7350 DRM Driver for DSI Simple Panels");
MODULE_LICENSE("GPL v2");
