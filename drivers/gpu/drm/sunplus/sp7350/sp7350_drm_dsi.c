// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM CRTCs and Encoder/Connecter
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */
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

	//struct debugfs_regset32 regset;
};

#define host_to_dsi(host) container_of(host, struct sp7350_drm_dsi, dsi_host)

/* DSI encoder KMS struct */
struct sp7350_dsi_encoder {
	struct sp7350_drm_encoder base;
	struct sp7350_drm_dsi *dsi;
};

#define to_sp7350_dsi_encoder(target)\
	container_of(target, struct sp7350_dsi_encoder, base)

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

static void sp7350_dsi_encoder_disable(struct drm_encoder *encoder)
{
	DRM_INFO("encoder disable:%s\n", encoder->name);
}
static void sp7350_dsi_encoder_enable(struct drm_encoder *encoder)
{
	DRM_INFO("encoder enable:%s\n", encoder->name);
}

static enum drm_connector_status sp7350_dsi_encoder_detect(struct drm_encoder *encoder,
					    struct drm_connector *connector)
{
	DRM_INFO("encoder %s detect connector:%s\n", encoder->name, connector->name);
	return connector->status;
}

static ssize_t sp7350_dsi_host_transfer(struct mipi_dsi_host *host,
				     const struct mipi_dsi_msg *msg)
{
	DRM_INFO("sp7350_dsi_host_transfer\n");
#if 0  /* [TODO]Set and send DSI PACKET for C3V DISPLAY */
	struct sp7350_drm_dsi *dsi = host_to_dsi(host);
	struct mipi_dsi_packet packet;
	u32 pkth = 0, pktc = 0;
	int i, ret = 0;
	bool is_long = mipi_dsi_packet_format_is_long(msg->type);
	u32 cmd_fifo_len = 0, pix_fifo_len = 0;

	mipi_dsi_create_packet(&packet, msg);

	pkth |= VC4_SET_FIELD(packet.header[0], DSI_TXPKT1H_BC_DT);
	pkth |= VC4_SET_FIELD(packet.header[1] |
			      (packet.header[2] << 8),
			      DSI_TXPKT1H_BC_PARAM);

	if (is_long) {
		/* Divide data across the various FIFOs we have available.
		 * The command FIFO takes byte-oriented data, but is of
		 * limited size. The pixel FIFO (never actually used for
		 * pixel data in reality) is word oriented, and substantially
		 * larger. So, we use the pixel FIFO for most of the data,
		 * sending the residual bytes in the command FIFO at the start.
		 *
		 * With this arrangement, the command FIFO will never get full.
		 */
		if (packet.payload_length <= 16) {
			cmd_fifo_len = packet.payload_length;
			pix_fifo_len = 0;
		} else {
			cmd_fifo_len = (packet.payload_length %
					DSI_PIX_FIFO_WIDTH);
			pix_fifo_len = ((packet.payload_length - cmd_fifo_len) /
					DSI_PIX_FIFO_WIDTH);
		}

		WARN_ON_ONCE(pix_fifo_len >= DSI_PIX_FIFO_DEPTH);

		pkth |= VC4_SET_FIELD(cmd_fifo_len, DSI_TXPKT1H_BC_CMDFIFO);
	}

	if (msg->rx_len) {
		pktc |= VC4_SET_FIELD(DSI_TXPKT1C_CMD_CTRL_RX,
				      DSI_TXPKT1C_CMD_CTRL);
	} else {
		pktc |= VC4_SET_FIELD(DSI_TXPKT1C_CMD_CTRL_TX,
				      DSI_TXPKT1C_CMD_CTRL);
	}

	for (i = 0; i < cmd_fifo_len; i++)
		DSI_PORT_WRITE(TXPKT_CMD_FIFO, packet.payload[i]);
	for (i = 0; i < pix_fifo_len; i++) {
		const u8 *pix = packet.payload + cmd_fifo_len + i * 4;

		DSI_PORT_WRITE(TXPKT_PIX_FIFO,
			       pix[0] |
			       pix[1] << 8 |
			       pix[2] << 16 |
			       pix[3] << 24);
	}

	if (msg->flags & MIPI_DSI_MSG_USE_LPM)
		pktc |= DSI_TXPKT1C_CMD_MODE_LP;
	if (is_long)
		pktc |= DSI_TXPKT1C_CMD_TYPE_LONG;

	/* Send one copy of the packet.  Larger repeats are used for pixel
	 * data in command mode.
	 */
	pktc |= VC4_SET_FIELD(1, DSI_TXPKT1C_CMD_REPEAT);

	pktc |= DSI_TXPKT1C_CMD_EN;
	if (pix_fifo_len) {
		pktc |= VC4_SET_FIELD(DSI_TXPKT1C_DISPLAY_NO_SECONDARY,
				      DSI_TXPKT1C_DISPLAY_NO);
	} else {
		pktc |= VC4_SET_FIELD(DSI_TXPKT1C_DISPLAY_NO_SHORT,
				      DSI_TXPKT1C_DISPLAY_NO);
	}

	/* Enable the appropriate interrupt for the transfer completion. */
	dsi->xfer_result = 0;
	reinit_completion(&dsi->xfer_completion);
	if (dsi->variant->port == 0) {
		DSI_PORT_WRITE(INT_STAT,
			       DSI0_INT_CMDC_DONE_MASK | DSI1_INT_PHY_DIR_RTF);
		if (msg->rx_len) {
			DSI_PORT_WRITE(INT_EN, (DSI0_INTERRUPTS_ALWAYS_ENABLED |
						DSI0_INT_PHY_DIR_RTF));
		} else {
			DSI_PORT_WRITE(INT_EN,
				       (DSI0_INTERRUPTS_ALWAYS_ENABLED |
					VC4_SET_FIELD(DSI0_INT_CMDC_DONE_NO_REPEAT,
						      DSI0_INT_CMDC_DONE)));
		}
	} else {
		DSI_PORT_WRITE(INT_STAT,
			       DSI1_INT_TXPKT1_DONE | DSI1_INT_PHY_DIR_RTF);
		if (msg->rx_len) {
			DSI_PORT_WRITE(INT_EN, (DSI1_INTERRUPTS_ALWAYS_ENABLED |
						DSI1_INT_PHY_DIR_RTF));
		} else {
			DSI_PORT_WRITE(INT_EN, (DSI1_INTERRUPTS_ALWAYS_ENABLED |
						DSI1_INT_TXPKT1_DONE));
		}
	}

	/* Send the packet. */
	DSI_PORT_WRITE(TXPKT1H, pkth);
	DSI_PORT_WRITE(TXPKT1C, pktc);

	if (!wait_for_completion_timeout(&dsi->xfer_completion,
					 msecs_to_jiffies(1000))) {
		dev_err(&dsi->pdev->dev, "transfer interrupt wait timeout");
		dev_err(&dsi->pdev->dev, "instat: 0x%08x\n",
			DSI_PORT_READ(INT_STAT));
		ret = -ETIMEDOUT;
	} else {
		ret = dsi->xfer_result;
	}

	DSI_PORT_WRITE(INT_EN, DSI_PORT_BIT(INTERRUPTS_ALWAYS_ENABLED));

	if (ret)
		goto reset_fifo_and_return;

	if (ret == 0 && msg->rx_len) {
		u32 rxpkt1h = DSI_PORT_READ(RXPKT1H);
		u8 *msg_rx = msg->rx_buf;

		if (rxpkt1h & DSI_RXPKT1H_PKT_TYPE_LONG) {
			u32 rxlen = VC4_GET_FIELD(rxpkt1h,
						  DSI_RXPKT1H_BC_PARAM);

			if (rxlen != msg->rx_len) {
				DRM_ERROR("DSI returned %db, expecting %db\n",
					  rxlen, (int)msg->rx_len);
				ret = -ENXIO;
				goto reset_fifo_and_return;
			}

			for (i = 0; i < msg->rx_len; i++)
				msg_rx[i] = DSI_READ(DSI1_RXPKT_FIFO);
		} else {
			/* FINISHME: Handle AWER */

			msg_rx[0] = VC4_GET_FIELD(rxpkt1h,
						  DSI_RXPKT1H_SHORT_0);
			if (msg->rx_len > 1) {
				msg_rx[1] = VC4_GET_FIELD(rxpkt1h,
							  DSI_RXPKT1H_SHORT_1);
			}
		}
	}

	return ret;

reset_fifo_and_return:
	DRM_ERROR("DSI transfer failed, resetting: %d\n", ret);

	DSI_PORT_WRITE(TXPKT1C, DSI_PORT_READ(TXPKT1C) & ~DSI_TXPKT1C_CMD_EN);
	udelay(1);
	DSI_PORT_WRITE(CTRL,
		       DSI_PORT_READ(CTRL) |
		       DSI_PORT_BIT(CTRL_RESET_FIFOS));

	DSI_PORT_WRITE(TXPKT1C, 0);
	DSI_PORT_WRITE(INT_EN, DSI_PORT_BIT(INTERRUPTS_ALWAYS_ENABLED));
	return ret;
#else
	return 0;
#endif
}

static int sp7350_dsi_host_attach(struct mipi_dsi_host *host,
			       struct mipi_dsi_device *device)
{
	struct sp7350_drm_dsi *dsi = host_to_dsi(host);

	DRM_INFO("sp7350_dsi_host_attach\n");
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
	DRM_INFO("sp7350_dsi_host_detach\n");
	return 0;
}

static const struct mipi_dsi_host_ops sp7350_dsi_host_ops = {
	.attach = sp7350_dsi_host_attach,
	.detach = sp7350_dsi_host_detach,
	.transfer = sp7350_dsi_host_transfer,
};


static const struct drm_encoder_helper_funcs sp7350_dsi_encoder_helper_funcs = {
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

	if (!crtc_mask) {
		DRM_ERROR("failed to find crtc mask\n");
		return -EINVAL;
	}

	encoder->possible_crtcs = crtc_mask;
	DRM_INFO("crtc_mask:0x%X\n", crtc_mask);
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

#if 0  /* TODO: setting for C3V DISPLAY REGISTER */
	dsi->regs = sp7350_ioremap_regs(pdev, 0);
	if (IS_ERR(dsi->regs))
		return PTR_ERR(dsi->regs);

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
		DRM_ERROR("drm_of_find_panel_or_bridge failed %d\n", ret);
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

