// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM CRTCs and Encoder/Connecter
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_edid.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>
#include <drm/drm_print.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include "sp7350_drm_crtc.h"
//#include "sp7350_drm_plane.h"
//#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_regs.h"
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_mipitx.h"

# define DSI_PFORMAT_RGB565          0
# define DSI_PFORMAT_RGB666_PACKED   1
# define DSI_PFORMAT_RGB666          2
# define DSI_PFORMAT_RGB888          3

/* General DSI hardware state. */
struct sp7350_drm_dsi {
	struct platform_device *pdev;

	struct mipi_dsi_host dsi_host;
	struct drm_encoder *encoder;
	struct drm_bridge *bridge;
	struct list_head bridge_chain;

	void __iomem *regs;

	struct dma_chan *reg_dma_chan;
	dma_addr_t reg_dma_paddr;
	u32 *reg_dma_mem;
	dma_addr_t reg_paddr;

	/* DSI0 Fixed. */
	unsigned int port;

	/* DSI channel for the panel we're connected to. */
	u32 channel;
	u32 lanes;
	u32 format;
	u32 divider;
	u32 mode_flags;

	/* Input clock from CPRMAN to the digital PHY, for the DSI
	 * escape clock.
	 */
	struct clk *escape_clock;

	/* Input clock to the analog PHY, used to generate the DSI bit
	 * clock.
	 */
	struct clk *pll_phy_clock;

	/* HS Clocks generated within the DSI analog PHY. */
	struct clk_fixed_factor phy_clocks[3];

	struct clk_hw_onecell_data *clk_onecell;

	/* Pixel clock output to the pixelvalve, generated from the HS
	 * clock.
	 */
	struct clk *pixel_clock;

	struct completion xfer_completion;
	int xfer_result;

	struct debugfs_regset32 regset;
};

#define host_to_dsi(host) container_of(host, struct sp7350_drm_dsi, dsi_host)

/* DSI encoder KMS struct */
struct sp7350_dsi_encoder {
	struct sp7350_drm_encoder base;
	struct sp7350_drm_dsi *dsi;
};

#define to_sp7350_dsi_encoder(target)\
	container_of(target, struct sp7350_dsi_encoder, base.base)

#if 0
static void sp7350_drm_connector_destroy(struct drm_connector *connector)
{
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs sp7350_drm_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = sp7350_drm_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static int sp7350_drm_conn_get_modes(struct drm_connector *connector)
{
	int count;

	count = drm_add_modes_noedid(connector, XRES_MAX, YRES_MAX);
	drm_set_preferred_mode(connector, XRES_DEF, YRES_DEF);

	return count;
}

static const struct drm_connector_helper_funcs sp7350_drm_conn_helper_funcs = {
	.get_modes    = sp7350_drm_conn_get_modes,
};

int sp7350_drm_output_init(struct sp7350_drm_device *sdev, int index)
{
	struct sp7350_drm_output *output = &sdev->output;
	struct drm_device *dev = sdev->ddev;
	struct drm_connector *connector = &output->connector;
	struct drm_encoder *encoder = &output->encoder;
	struct drm_crtc *crtc = &output->crtc;
	struct drm_plane *primary;
	//unsigned int i;
	int ret;

	primary = sp7350_drm_plane_init(sdev, DRM_PLANE_TYPE_PRIMARY, index);
	if (IS_ERR(primary))
		return PTR_ERR(primary);

#if 0
	for (i = 0; i < 4; ++i) {
		primary = sp7350_drm_plane_init(sdev, DRM_PLANE_TYPE_OVERLAY, index);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to create plane %u\n", i);
			goto err_modeset_cleanup;
		}
	}
#endif

	ret = sp7350_drm_crtc_init(dev, crtc, primary, NULL);
	if (ret)
		goto err_crtc;

	ret = drm_connector_init(dev, connector, &sp7350_drm_connector_funcs,
				 DRM_MODE_CONNECTOR_DSI);
	if (ret) {
		DRM_ERROR("Failed to init connector\n");
		goto err_connector;
	}

	drm_connector_helper_add(connector, &sp7350_drm_conn_helper_funcs);

	ret = drm_simple_encoder_init(dev, encoder, DRM_MODE_ENCODER_DSI);
	if (ret) {
		DRM_ERROR("Failed to init encoder\n");
		goto err_encoder;
	}
	//encoder->possible_crtcs = 1;
	encoder->possible_crtcs = 1 << crtc->index;
	/* FIXME !!!!!!!!!
	 * Currently bound CRTC, only really meaningful for non-atomic
	 * drivers.  Atomic drivers should instead check
	 * &drm_connector_state.crtc. */
	//encoder->crtc = crtc;

	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret) {
		DRM_ERROR("Failed to attach connector to encoder\n");
		goto err_attach;
	}



	//ret = sp7350_drm_enable_writeback_connector(sdev);
	//if (ret)
	//	DRM_ERROR("Failed to init writeback connector\n");

	drm_mode_config_reset(dev);

	return 0;

err_attach:
	drm_encoder_cleanup(encoder);

err_encoder:
	drm_connector_cleanup(connector);

err_connector:
	drm_crtc_cleanup(crtc);

err_crtc:

	drm_plane_cleanup(primary);

	return ret;
}
#endif

#if 0
#define MIPITX_CMD_FIFO_FULL 0x00000001
#define MIPITX_CMD_FIFO_EMPTY 0x00000010
#define MIPITX_DATA_FIFO_FULL 0x00000100
#define MIPITX_DATA_FIFO_EMPTY 0x00001000
void _check_cmd_fifo_full(struct sp7350_drm_dsi *dsi)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
	//pr_info("fifo_status 0x%08x\n", value);
	while((value & MIPITX_CMD_FIFO_FULL) == MIPITX_CMD_FIFO_FULL) {
		if(mipitx_fifo_timeout > 10000) //over 1 second
		{
			pr_info("cmd fifo full timeout\n");
			break;
		}
		value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
		++mipitx_fifo_timeout;
		udelay(100);
	}
}

void _check_data_fifo_full(struct sp7350_drm_dsi *dsi)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
	//pr_info("fifo_status 0x%08x\n", value);
	while((value & MIPITX_DATA_FIFO_FULL) == MIPITX_DATA_FIFO_FULL) {
		if(mipitx_fifo_timeout > 10000) //over 1 second
		{
			pr_info("data fifo full timeout\n");
			break;
		}
		value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
		++mipitx_fifo_timeout;
		udelay(100);
	}
}

/*
 * MIPI DSI (Display Command Set) for SP7350
 */
static void _sp7350_dcs_write_buf(struct sp7350_drm_dsi *dsi, const void *data, size_t len)
{
	int i;
	u8 *data1;
	u32 value, data_cnt;

	data1 = (u8 *)data;

	udelay(100);
	if (len == 0) {
		_check_cmd_fifo_full(dsi);
		value = 0x00000003;
		writel(value, dsi->regs + MIPITX_SPKT_HEAD); //G204.09
	} else if (len == 1) {
		_check_cmd_fifo_full(dsi);
		value = 0x00000013 | (data1[0] << 8);
		writel(value, dsi->regs + MIPITX_SPKT_HEAD); //G204.09
	} else if (len == 2) {
		_check_cmd_fifo_full(dsi);
		value = 0x00000023 | (data1[0] << 8) | (data1[1] << 16);
		writel(value, dsi->regs + MIPITX_SPKT_HEAD); //G204.09
	} else if ((len >= 3) && (len <= 64)) {
		_check_cmd_fifo_full(dsi);
		value = 0x00000029 | ((u32)len << 8);
		writel(value, dsi->regs + MIPITX_LPKT_HEAD); //G204.10

		if (len % 4) data_cnt = ((u32)len / 4) + 1;
		else data_cnt = ((u32)len / 4);

		for (i = 0; i < data_cnt; i++) {
			_check_data_fifo_full(dsi);
			value = 0x00000000;
			#if 1
			if (i * 4 + 0 < len) value |= (data1[i * 4 + 0] << 0);
			if (i * 4 + 1 < len) value |= (data1[i * 4 + 1] << 8);
			if (i * 4 + 2 < len) value |= (data1[i * 4 + 2] << 16);
			if (i * 4 + 3 < len) value |= (data1[i * 4 + 3] << 24);
			#else
			if (i * 4 + 0 >= len) data1[i * 4 + 0] = 0x00;
			if (i * 4 + 1 >= len) data1[i * 4 + 1] = 0x00;
			if (i * 4 + 2 >= len) data1[i * 4 + 2] = 0x00;
			if (i * 4 + 3 >= len) data1[i * 4 + 3] = 0x00;
			value |= ((data1[i * 4 + 3] << 24) | (data1[i * 4 + 2] << 16) |
				 (data1[i * 4 + 1] << 8) | (data1[i * 4 + 0] << 0));
			#endif
			pr_info("W G204.11 MIPITX_LPKT_PAYLOAD 0x%08x\n", value);
			writel(value, dsi->regs + MIPITX_LPKT_PAYLOAD); //G204.11
		}
	} else {
		pr_info("data length over %ld\n", len);
	}
}
#endif

static enum drm_mode_status _sp7350_dsi_encoder_phy_mode_valid(
					struct drm_encoder *encoder,
					const struct drm_display_mode *mode)
{
	/* TODO reference to dsi_encoder_phy_mode_valid */
	DRM_INFO("[TODO]%s\n", __func__);

	return MODE_OK;
}

static enum drm_mode_status sp7350_dsi_encoder_mode_valid(struct drm_encoder *encoder,
					const struct drm_display_mode *mode)

{
	const struct drm_crtc_helper_funcs *crtc_funcs = NULL;
	struct drm_crtc *crtc = NULL;
	struct drm_display_mode adj_mode;
	enum drm_mode_status ret;

	DRM_INFO("%s\n", __func__);

	/*
	 * The crtc might adjust the mode, so go through the
	 * possible crtcs (technically just one) and call
	 * mode_fixup to figure out the adjusted mode before we
	 * validate it.
	 */
	drm_for_each_crtc(crtc, encoder->dev) {
		/*
		 * reset adj_mode to the mode value each time,
		 * so we don't adjust the mode twice
		 */
		drm_mode_copy(&adj_mode, mode);

		crtc_funcs = crtc->helper_private;
		if (crtc_funcs && crtc_funcs->mode_fixup)
			if (!crtc_funcs->mode_fixup(crtc, mode, &adj_mode))
				return MODE_BAD;

		ret = _sp7350_dsi_encoder_phy_mode_valid(encoder, &adj_mode);
		if (ret != MODE_OK)
			return ret;
	}
	return MODE_OK;
}

static void sp7350_dsi_encoder_mode_set(struct drm_encoder *encoder,
				 struct drm_display_mode *mode,
				 struct drm_display_mode *adj_mode)
{
	/* TODO reference to dsi_encoder_mode_set */
	DRM_INFO("[TODO]%s\n", __func__);
}

static int sp7350_dsi_encoder_atomic_check(struct drm_encoder *encoder,
				    struct drm_crtc_state *crtc_state,
				    struct drm_connector_state *conn_state)
{
	/* do nothing */
	DRM_INFO("%s\n", __func__);
	return 0;
}

static void sp7350_dsi_encoder_disable(struct drm_encoder *encoder)
{
	struct drm_bridge *iter;
	struct sp7350_dsi_encoder *sp7350_encoder = to_sp7350_dsi_encoder(encoder);
	struct sp7350_drm_dsi *dsi = sp7350_encoder->dsi;

	DRM_INFO("%s %s\n", __func__, encoder->name);

	list_for_each_entry_reverse(iter, &dsi->bridge_chain, chain_node) {
		if (iter->funcs->disable)
			iter->funcs->disable(iter);

		if (iter == dsi->bridge)
			break;
	}

	list_for_each_entry_from(iter, &dsi->bridge_chain, chain_node) {
		if (iter->funcs->post_disable)
			iter->funcs->post_disable(iter);
	}
}

static void sp7350_dsi_encoder_enable(struct drm_encoder *encoder)
{
	struct drm_bridge *iter;
	struct sp7350_dsi_encoder *sp7350_encoder = to_sp7350_dsi_encoder(encoder);
	struct sp7350_drm_dsi *dsi = sp7350_encoder->dsi;

	DRM_INFO("%s %s\n", __func__, encoder->name);

	list_for_each_entry_reverse(iter, &dsi->bridge_chain, chain_node) {
		if (iter->funcs->pre_enable)
			iter->funcs->pre_enable(iter);
		if (iter->funcs->enable)
			iter->funcs->enable(iter);
	}
}

static enum drm_connector_status sp7350_dsi_encoder_detect(struct drm_encoder *encoder,
					    struct drm_connector *connector)
{
	DRM_INFO("[TODO]%s encoder %s detect connector:%s\n", __func__, encoder->name, connector->name);
	return connector->status;
}

static ssize_t sp7350_dsi_host_transfer(struct mipi_dsi_host *host,
				     const struct mipi_dsi_msg *msg)
{
	//struct sp7350_drm_dsi *dsi = host_to_dsi(host);

	/* reference to vc4_dsi_host_transfer */
	//DRM_INFO("%s\n", __func__);

	//print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
	//	msg->tx_buf, msg->tx_len, false);

	/* simple for send packet only! */
	sp7350_dcs_write_buf(msg->tx_buf, msg->tx_len);
	//_sp7350_dcs_write_buf(dsi, msg->tx_buf, msg->tx_len);

	return 0;
}

static int sp7350_dsi_host_attach(struct mipi_dsi_host *host,
			       struct mipi_dsi_device *device)
{
	struct sp7350_drm_dsi *dsi = host_to_dsi(host);

	DRM_INFO("%s\n", __func__);
	dsi->lanes = device->lanes;
	dsi->channel = device->channel;
	dsi->mode_flags = device->mode_flags;

	switch (device->format) {
	case MIPI_DSI_FMT_RGB888:
		dsi->format = DSI_PFORMAT_RGB888;
		dsi->divider = 24 / dsi->lanes;
		break;
	case MIPI_DSI_FMT_RGB666:
		dsi->format = DSI_PFORMAT_RGB666;
		dsi->divider = 24 / dsi->lanes;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		dsi->format = DSI_PFORMAT_RGB666_PACKED;
		dsi->divider = 18 / dsi->lanes;
		break;
	case MIPI_DSI_FMT_RGB565:
		dsi->format = DSI_PFORMAT_RGB565;
		dsi->divider = 16 / dsi->lanes;
		break;
	default:
		dev_err(&dsi->pdev->dev, "Unknown DSI format: %d.\n",
			dsi->format);
		return 0;
	}

	if (!(dsi->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		dev_err(&dsi->pdev->dev,
			"Only VIDEO mode panels supported currently.\n");
		return 0;
	}

	return 0;
}

static int sp7350_dsi_host_detach(struct mipi_dsi_host *host,
			       struct mipi_dsi_device *device)
{
	DRM_INFO("[TODO]%s\n", __func__);
	return 0;
}

static const struct mipi_dsi_host_ops sp7350_dsi_host_ops = {
	.attach = sp7350_dsi_host_attach,
	.detach = sp7350_dsi_host_detach,
	.transfer = sp7350_dsi_host_transfer,
};


static const struct drm_encoder_helper_funcs sp7350_dsi_encoder_helper_funcs = {
	.atomic_check	= sp7350_dsi_encoder_atomic_check,
	.mode_valid	= sp7350_dsi_encoder_mode_valid,
	.mode_set	= sp7350_dsi_encoder_mode_set,
	.disable = sp7350_dsi_encoder_disable,
	.enable = sp7350_dsi_encoder_enable,
	.detect = sp7350_dsi_encoder_detect,
};

static const struct of_device_id sp7350_dsi_dt_match[] = {
	{ .compatible = "sunplus,sp7350-dsi0" },
	{}
};

static int sp7350_drm_encoder_init(struct device *dev,
			       struct drm_device *drm_dev,
			       struct drm_encoder *encoder)
{
	int ret;
	u32 crtc_mask = drm_of_find_possible_crtcs(drm_dev, dev->of_node);

	DRM_INFO("%s\n", __func__);

	if (!crtc_mask) {
		DRM_ERROR("failed to find crtc mask\n");
		return -EINVAL;
	}

	encoder->possible_crtcs = crtc_mask;
	DRM_DEBUG("crtc_mask:0x%X\n", crtc_mask);
	ret = drm_simple_encoder_init(drm_dev, encoder, DRM_MODE_ENCODER_DSI);
	if (ret) {
		DRM_ERROR("failed to init dsi encoder\n");
		return ret;
	}

	drm_encoder_helper_add(encoder, &sp7350_dsi_encoder_helper_funcs);

	return 0;
}

static int sp7350_dsi_bind(struct device *dev, struct device *master, void *data)
{
	//struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm = dev_get_drvdata(master);
	struct sp7350_drm_dsi *dsi = dev_get_drvdata(dev);
	struct sp7350_dsi_encoder *sp7350_dsi_encoder;
	struct drm_panel *panel;
	//const struct of_device_id *match;
	//dma_cap_mask_t dma_mask;
	int ret;

	//match = of_match_device(sp7350_dsi_dt_match, dev);
	//if (!match)
	//	return -ENODEV;

	dsi->port = 0;

	sp7350_dsi_encoder = devm_kzalloc(dev, sizeof(*sp7350_dsi_encoder),
				       GFP_KERNEL);
	if (!sp7350_dsi_encoder)
		return -ENOMEM;

	INIT_LIST_HEAD(&dsi->bridge_chain);
	sp7350_dsi_encoder->base.type = SP7350_DRM_ENCODER_TYPE_DSI0;
	sp7350_dsi_encoder->dsi = dsi;
	dsi->encoder = &sp7350_dsi_encoder->base.base;

	dsi->regs = sp7350_display_ioremap_regs(0);
	if (IS_ERR(dsi->regs))
		return PTR_ERR(dsi->regs);

#if 0  /* TODO: setting for C3V DISPLAY REGISTER */
	dsi->regset.base = dsi->regs;
	dsi->regset.regs = dsi->variant->regs;
	dsi->regset.nregs = dsi->variant->nregs;

	if (DSI_PORT_READ(ID) != DSI_ID_VALUE) {
		dev_err(dev, "Port returned 0x%08x for ID instead of 0x%08x\n",
			DSI_PORT_READ(ID), DSI_ID_VALUE);
		return -ENODEV;
	}
#endif

	init_completion(&dsi->xfer_completion);

#if 0  /* TODO: set with C3V actullay */
	/* At startup enable error-reporting interrupts and nothing else. */
	DSI_PORT_WRITE(INT_EN, DSI1_INTERRUPTS_ALWAYS_ENABLED);
	/* Clear any existing interrupt state. */
	DSI_PORT_WRITE(INT_STAT, DSI_PORT_READ(INT_STAT));

	if (dsi->reg_dma_mem)
		ret = devm_request_threaded_irq(dev, platform_get_irq(pdev, 0),
						vc4_dsi_irq_defer_to_thread_handler,
						vc4_dsi_irq_handler,
						IRQF_ONESHOT,
						"vc4 dsi", dsi);
	else
		ret = devm_request_irq(dev, platform_get_irq(pdev, 0),
				       vc4_dsi_irq_handler, 0, "vc4 dsi", dsi);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get interrupt: %d\n", ret);
		return ret;
	}

	dsi->escape_clock = devm_clk_get(dev, "escape");
	if (IS_ERR(dsi->escape_clock)) {
		ret = PTR_ERR(dsi->escape_clock);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get escape clock: %d\n", ret);
		return ret;
	}

	dsi->pll_phy_clock = devm_clk_get(dev, "phy");
	if (IS_ERR(dsi->pll_phy_clock)) {
		ret = PTR_ERR(dsi->pll_phy_clock);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get phy clock: %d\n", ret);
		return ret;
	}

	dsi->pixel_clock = devm_clk_get(dev, "pixel");
	if (IS_ERR(dsi->pixel_clock)) {
		ret = PTR_ERR(dsi->pixel_clock);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get pixel clock: %d\n", ret);
		return ret;
	}
#endif

	/*
	 * Get the endpoint node. In our case, dsi has one output port1
	 * to which the external HDMI bridge is connected.
	 */
	//ret = drm_of_find_panel_or_bridge(dev->of_node, 0, 0, &panel, &dsi->bridge);
	//ret = drm_of_find_panel_or_bridge(dev->of_node, 1, 0, NULL, &dsi->bridge);
	ret = drm_of_find_panel_or_bridge(dev->of_node, 1, 0, &panel, &dsi->bridge);
	if (ret) {
		DRM_ERROR("drm_of_find_panel_or_bridge failed -%d\n", -ret);
		/* If the bridge or panel pointed by dev->of_node is not
		 * enabled, just return 0 here so that we don't prevent the DRM
		 * dev from being registered. Of course that means the DSI
		 * encoder won't be exposed, but that's not a problem since
		 * nothing is connected to it.
		 */
		if (ret == -ENODEV)
			return 0;

		return ret;
	}

	if (panel) {
		dsi->bridge = devm_drm_panel_bridge_add_typed(dev, panel,
							      DRM_MODE_CONNECTOR_DSI);
		if (IS_ERR(dsi->bridge))
			return PTR_ERR(dsi->bridge);
	}

#if 0  /* TODO: set with C3V actullay */
	/* The esc clock rate is supposed to always be 100Mhz. */
	ret = clk_set_rate(dsi->escape_clock, 100 * 1000000);
	if (ret) {
		dev_err(dev, "Failed to set esc clock: %d\n", ret);
		return ret;
	}

	ret = sp7350_dsi_init_phy_clocks(dsi);
	if (ret)
		return ret;
#endif

	ret = sp7350_drm_encoder_init(dev, drm, dsi->encoder);
	if (ret)
		return ret;

	ret = drm_bridge_attach(dsi->encoder, dsi->bridge, NULL, 0);
	if (ret) {
		dev_err(dev, "bridge attach failed: %d\n", ret);
		return ret;
	}

	/* FIXME, use firmware EDID for lt8912b */
	#if IS_ENABLED(CONFIG_DRM_LOAD_EDID_FIRMWARE) && IS_ENABLED(CONFIG_DRM_LONTIUM_LT8912B)
	{
		DRM_WARN("Use firmware EDID edid/1920x1080.bin for lt8912b output\n");
		__drm_set_edid_firmware_path("edid/1920x1080.bin");
	}
	#endif

	/* Disable the atomic helper calls into the bridge.  We
	 * manually call the bridge pre_enable / enable / etc. calls
	 * from our driver, since we need to sequence them within the
	 * encoder's enable/disable paths.
	 */
	list_splice_init(&dsi->encoder->bridge_chain, &dsi->bridge_chain);

	//sp7350_debugfs_add_regset32(drm, dsi->variant->debugfs_name, &dsi->regset);

	pm_runtime_enable(dev);

	return 0;
}

static void sp7350_dsi_unbind(struct device *dev, struct device *master,
			   void *data)
{
	struct sp7350_drm_dsi *dsi = dev_get_drvdata(dev);

	if (dsi->bridge)
		pm_runtime_disable(dev);

	/*
	 * Restore the bridge_chain so the bridge detach procedure can happen
	 * normally.
	 */
	list_splice_init(&dsi->bridge_chain, &dsi->encoder->bridge_chain);
	drm_encoder_cleanup(dsi->encoder);
}

static const struct component_ops sp7350_dsi_ops = {
	.bind   = sp7350_dsi_bind,
	.unbind = sp7350_dsi_unbind,
};

static int sp7350_dsi_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp7350_drm_dsi *dsi;
	int ret;

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	if (!dsi)
		return -ENOMEM;
	dev_set_drvdata(dev, dsi);

	dsi->pdev = pdev;

	/* Note, the initialization sequence for DSI and panels is
	 * tricky.  The component bind above won't get past its
	 * -EPROBE_DEFER until the panel/bridge probes.  The
	 * panel/bridge will return -EPROBE_DEFER until it has a
	 * mipi_dsi_host to register its device to.  So, we register
	 * the host during pdev probe time, so sp7350_drm as a whole can then
	 * -EPROBE_DEFER its component bind process until the panel
	 * successfully attaches.
	 */
	dsi->dsi_host.ops = &sp7350_dsi_host_ops;
	dsi->dsi_host.dev = dev;
	mipi_dsi_host_register(&dsi->dsi_host);

	ret = component_add(&pdev->dev, &sp7350_dsi_ops);
	if (ret) {
		mipi_dsi_host_unregister(&dsi->dsi_host);
		return ret;
	}

	return 0;
}

static int sp7350_dsi_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp7350_drm_dsi *dsi = dev_get_drvdata(dev);

	component_del(&pdev->dev, &sp7350_dsi_ops);
	mipi_dsi_host_unregister(&dsi->dsi_host);

	return 0;
}

struct platform_driver sp7350_dsi_driver = {
	.probe = sp7350_dsi_dev_probe,
	.remove = sp7350_dsi_dev_remove,
	.driver = {
		.name = "sp7350_dsi",
		.of_match_table = sp7350_dsi_dt_match,
	},
};

