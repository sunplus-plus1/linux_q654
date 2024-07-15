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
#include "sp7350_drm_dsi.h"
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_mipitx.h"
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_tcon.h"

/* always keep 0 */
#define SP7350_DRM_TODO    0

/* For TCON test pattern only */
#define TCON_TPG_ENABLE  0

# define DSI_PFORMAT_RGB565          0
# define DSI_PFORMAT_RGB666_PACKED   1
# define DSI_PFORMAT_RGB666          2
# define DSI_PFORMAT_RGB888          3

#define MIPITX_CMD_FIFO_FULL   0x00000001
#define MIPITX_CMD_FIFO_EMPTY  0x00000010
#define MIPITX_DATA_FIFO_FULL  0x00000100
#define MIPITX_DATA_FIFO_EMPTY 0x00001000

/* General DSI hardware state. */
struct sp7350_drm_dsi {
	struct platform_device *pdev;

	struct mipi_dsi_host dsi_host;
	struct drm_encoder *encoder;
	struct drm_bridge *bridge;
	struct list_head bridge_chain;

	void __iomem *regs;
	void __iomem *ao_moon3;

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
	struct debugfs_regset32 ao_moon3_regset;
	struct drm_display_mode adj_mode_store;
};

#define host_to_dsi(host) container_of(host, struct sp7350_drm_dsi, dsi_host)

/* DSI encoder KMS struct */
struct sp7350_dsi_encoder {
	struct sp7350_drm_encoder base;
	struct sp7350_drm_dsi *dsi;
};

#define to_sp7350_dsi_encoder(target)\
	container_of(target, struct sp7350_dsi_encoder, base.base)

/* SP7350 TCON HW config reference to sp7350_disp_tcon.c */
/* FIXME: How to generate tcon timing???
 * Just copy from sp7350_disp_tcon.c now.
 */
/*
 * sp_tcon_para_dsi[x][y]
 * y = 0-1, TCON width & height
 * y = 2-11, TCON DE_H & Vsync_H & Hsync & DE_V & VTOP_V
 */
static const u32 sp_tcon_para_dsi[11][12] = {
	/* (w   h)    DE_H       Vsync_H     Hsync       DE_V        VTOP_V     */
	{ 720,  480,    0,  719,  850,  854,  850,  854,    0,    0,  524,    0}, /* 480P */
	{ 720,  576,    0,  719,  856,  860,  856,  860,    0,    0,  624,    0}, /* 576P */
	{1280,  720,    0, 1279, 1642, 1646, 1642, 1646,    0,    0,  749,    0}, /* 720P */
	{1920, 1080,    0, 1919, 2192, 2196, 2192, 2196,    0,    0, 1124,    0}, /* 1080P */
	//{  64,   64,    0,   63,  353,  353,  353,  356,    0,    0,   99,    0}, /* 64x64 */
	{ 480, 1280,    0,  479,  612,  616,  612,  616,    0,    0, 1313,    0}, /* 480x1280 */
	//{ 480, 1280,    0,  479,  670,  674,  670,  674,    0,    0, 1311,    0}, /* 480x1280, from specification adjustment */
	{ 128,  128,    0,  127,  352,  352,  352,  356,    0,    0,  149,    0}, /* 128x128 */
	//{ 240,  320,    0,  239,  675,  679,  675,  679,    0,    0,  363,    0}, /* 240x320 */
	{ 240,  320,    0,  239,  675,  679,  675,  679,    0,    0,  353,    0}, /* 240x320 */
	{3840,   64,    0, 3839, 4600, 4600, 4600, 4604,    0,    0,   99,    0}, /* 3840x64 */
	{3840, 2880,    0, 3839, 4600, 4600, 4600, 4604,    0,    0, 3199,    0}, /* 3840x2880 */
	//{ 800,  480,    0,  799,  865,  869,  865,  869,    0,    0,  509,    0}, /* 800x480 */
	{ 800,  480,    0,  799,  909,  913,  909,  913,    0,    0,  509,    0}, /* 800x480, from sp_mipitx_input_timing_dsi */
	{1024,  600,    0, 1023, 1336, 1336, 1336, 1340,    0,    0,  634,    0}  /* 1024x600 */
};

#if TCON_TPG_ENABLE
/*
 * sp_tcon_tpg_para_dsi[x][y]
 * y = 0-1, TCON width & height
 * y = 2-9, TCON Hstep & Vstep & Hcnt & Vcnt & Hact & Vact & A_LINE & DITHER
 */
static const u32 sp_tcon_tpg_para_dsi[11][10] = {
	/* (w   h)    Hstep Vstep Hcnt  Vcnt  Hact  Vact A_LINE DITHER */
	{ 720,  480,    4,    4,  857,  524,  719,  479,  35, 0x01}, /* 480P */
	{ 720,  576,    4,    4,  863,  624,  719,  575,  17, 0x41}, /* 576P */
	{1280,  720,    4,    4, 1649,  749, 1279,  719,  24, 0x41}, /* 720P */
	{1920, 1080,    4,    4, 2199, 1124, 1919, 1079,  40, 0x01}, /* 1080P */
	//{  64,   64,    4,    4,  359,   99,   63,   63,  17, 0xC1}, /* 64x64 */
	{ 480, 1280,    4,    4,  619, 1313,  479, 1279,  16, 0x01}, /* 480x1280 */
	{ 128,  128,    4,    4,  359,  149,  127,  127,  17, 0x49}, /* 128x128 */
	{ 240,  320,    4,    4,  682,  353,  239,  319,  25, 0x01}, /* 240x320 */
	{3840,   64,    4,    4, 4607,   99, 3839,   63,  17, 0x01}, /* 3840x64 */
	{3840, 2880,    4,    4, 4607, 3199, 3839, 2879,  17, 0x01}, /* 3840x2880 */
	{ 800,  480,    4,    4,  872,  509,  799,  479,  22, 0x01}, /* 800x480 */
	{1024,  600,    4,    4, 1343,  634, 1023,  599,  17, 0x01}  /* 1024x600 */
};
#endif

/*
 * TODO: reference to sp7350_disp_mipitx.c,
 *   but should comes from panel driver parameters.
 * sp_mipitx_phy_pllclk_dsi[x][y]
 * y = 0-1, MIPITX width & height
 * y = 2-7, MIPITX PRESCALE & FBKDIV & PREDIV & POSTDIV & EN_DIV5 & BNKSEL(TXPLL)
 *
 * XTAL--[PREDIV]--------------------------->[EN_DIV5]--[POSTDIV]-->Fckout
 *                 |                       |
 *                 |<--FBKDIV<--PRESCALE<--|
 *
 *                25 * PRESCALE * FBKDIV
 *    Fckout = -----------------------------
 *              PREDIV * POSTDIV * 5^EN_DIV5
 *
 * y = 8-11, MIPITX FBKDIV_H & POSTDIV_H & MIPITX_SEL & BNKSEL_H (PLLH)
 *
 * XTAL--[PREDIV]--------------------------->[POSTDIV]--[MIPITX_SEL]-->Fckout
 *                 |                       |
 *                 |<--FBKDIV<--PRESCALE<--|
 *
 *                25M * PRESCALE * FBKDIV
 *    Fckout = -----------------------------
 *              PREDIV * POSTDIV * MIPITX_SEL
 *
 */
static const u32 sp_mipitx_phy_pllclk_dsi[11][12] = {
	/* (w   h)   P1   P2    P3   P4   P5   P6   Q1   Q2   Q3  Q4*/
	{ 720,  480,  0,  26,    0,   2,   0,   1,  14,  10,   7,  1}, /* 480P 27027KHz => 162.16MHz */
	{ 720,  576,  0,  26,    0,   2,   0,   1,  44,  15,   7,  3}, /* 576P 27000KHz => 162MHz*/
	{1280,  720,  0,  18,    0,   0,   0,   0,  43,  10,   3,  3}, /* 720P 74250KHz => 445.5MHz*/
	{1920, 1080,  0,  36,    0,   0,   0,   1,  43,  10,   1,  3}, /* 1080P 148500KHz => 891MHz */
	//{ 480, 1280,  0,  37,    0,   2,   0,   1,  13,  15,   3,  1}, /* 480x1280 38500KHz => 231MHz */
	{ 480, 1280,  0,  47,    0,   2,   0,   2,  34,  15,   3,  2}, /* 480x1280 48880KHz => 293.28MHz, from sp_mipitx_input_timing_dsi */
	//{ 480, 1280,  1,  32,    0,   0,   1,   3,  13,  10,   3,  1}, /* 480x1280 53380KHz => 320.28MHz, from specification typical values */
	//{ 128,  128, 0x1, 0x1f, 0x1, 0x4, 0x1, 0x1, 0x0, 0x0, 0x0}, /* 128x128 */
	//{  240, 320,  0x0, 0x0e, 0x0, 0x0, 0x0, 0x0,  20, 10, 15, 2}, /* 240x320 14580KHz => 349.92Mhz */
	{  240, 320,    0,  56,    0,   2,   0,   3,  20,  10,  15,  2}, /* 240x320 14507KHz => 348.17Mhz, from sp_mipitx_input_timing_dsi */
	//{3840,   64, 0x0, 0x1f, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0}, /* 3840x64 */
	//{3840, 2880, 0x0, 0x3c, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0}, /* 3840x2880 */
	//{ 800,  480,  0,  26,    0,   0,    0,   1,  42,  15,   7,  3}, /* 800x480 26563Kh => 637.51MHz */
	//{ 800,  480,  0,  25,    0,   0,    0,   1,  40,  15,   7,  3}, /* 800x480 25979KHz => 623.51MHz, from Raspberry Pi firmware */
	{ 800,  480,  0,  27,    0,   0,    0,   1,  48,  15,   7,  3}, /* 800x480 28060KHz => 673.45MHz, from sp_mipitx_input_timing_dsi */
	//{1024,  600, 0x1, 0x3d, 0x1, 0x1, 0x1, 0x3, 0x0, 0x0, 0x0}  /* 1024x600 */
};

/*
 * sp_mipitx_output_timing[x]
 * x = 0-10, MIPITX phy
 *   T_HS-EXIT / T_LPX
 *   T_CLK-PREPARE / T_CLK-ZERO
 *   T_CLK-TRAIL / T_CLK-PRE / T_CLK-POST
 *   T_HS-TRAIL / T_HS-PREPARE / T_HS-ZERO
 */
static const u32 sp_mipitx_output_timing[10] = {
	0x10,  /* T_HS-EXIT */
	0x08,  /* T_LPX */
	0x10,  /* T_CLK-PREPARE */
	0x10,  /* T_CLK-ZERO */
	0x05,  /* T_CLK-TRAIL */
	0x12,  /* T_CLK-PRE */
	0x20,  /* T_CLK-POST */
	0x05,  /* T_HS-TRAIL */
	0x05,  /* T_HS-PREPARE */
	0x10,  /* T_HS-ZERO */
};

static int sp7350_dsi_mode_check(const struct drm_display_mode *mode);
static void sp7350_dsi_tcon_init(struct sp7350_drm_dsi *dsi);
static void sp7350_dsi_tcon_timing_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode);
static void sp7350_mipitx_dsi_phy_init(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_pllclk_init(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_lane_control_set(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_pllclk_set(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode);
static void sp7350_mipitx_dsi_video_mode_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode);
static void sp7350_mipitx_dsi_cmd_mode_start(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_video_mode_on(struct sp7350_drm_dsi *dsi);
static void check_dsi_cmd_fifo_full(struct sp7350_drm_dsi *dsi);
static void check_dsi_data_fifo_full(struct sp7350_drm_dsi *dsi);

static enum drm_mode_status _sp7350_dsi_encoder_phy_mode_valid(
					struct drm_encoder *encoder,
					const struct drm_display_mode *mode)
{
	/* TODO reference to dsi_encoder_phy_mode_valid */
	DRM_DEBUG_DRIVER("[TODO]\n");

	return MODE_OK;
}

static enum drm_mode_status sp7350_dsi_encoder_mode_valid(struct drm_encoder *encoder,
							  const struct drm_display_mode *mode)

{
	const struct drm_crtc_helper_funcs *crtc_funcs = NULL;
	struct drm_crtc *crtc = NULL;
	struct drm_display_mode adj_mode;
	enum drm_mode_status ret;

	DRM_DEBUG_DRIVER("[Start]\n");

	if (sp7350_dsi_mode_check(mode))
		return MODE_NOMODE;

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
	struct drm_bridge *iter;
	struct sp7350_dsi_encoder *sp7350_encoder = to_sp7350_dsi_encoder(encoder);
	struct sp7350_drm_dsi *dsi = sp7350_encoder->dsi;

	/* TODO reference to dsi_encoder_mode_set */
	//DRM_DEBUG_DRIVER("[TODO]\n");
	DRM_DEBUG_DRIVER("SET mode[%dx%d], adj_mode[%dx%d]\n",
			 mode->hdisplay, mode->vdisplay,
		adj_mode->hdisplay, adj_mode->vdisplay);

	list_for_each_entry_reverse(iter, &dsi->bridge_chain, chain_node) {
		if (iter->funcs->mode_set)
			iter->funcs->mode_set(iter, mode, adj_mode);
	}

	sp7350_dsi_tcon_timing_setting(dsi, adj_mode);
	sp7350_mipitx_dsi_pllclk_set(dsi, adj_mode);
	sp7350_mipitx_dsi_video_mode_setting(dsi, adj_mode);

	/* store */
	drm_mode_copy(&dsi->adj_mode_store, adj_mode);
}

static int sp7350_dsi_encoder_atomic_check(struct drm_encoder *encoder,
					   struct drm_crtc_state *crtc_state,
				    struct drm_connector_state *conn_state)
{
	/* do nothing */
	DRM_DEBUG_DRIVER("[do nothing]\n");
	return 0;
}

static void sp7350_dsi_encoder_disable(struct drm_encoder *encoder)
{
	struct drm_bridge *iter;
	struct sp7350_dsi_encoder *sp7350_encoder = to_sp7350_dsi_encoder(encoder);
	struct sp7350_drm_dsi *dsi = sp7350_encoder->dsi;

	DRM_DEBUG_DRIVER("%s\n", encoder->name);

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

	DRM_DEBUG_DRIVER("%s\n", encoder->name);

	sp7350_mipitx_dsi_cmd_mode_start(dsi);
	list_for_each_entry_reverse(iter, &dsi->bridge_chain, chain_node) {
		if (iter->funcs->pre_enable)
			iter->funcs->pre_enable(iter);
	}
	sp7350_mipitx_dsi_video_mode_on(dsi);

	list_for_each_entry_reverse(iter, &dsi->bridge_chain, chain_node) {
		if (iter->funcs->enable)
			iter->funcs->enable(iter);
	}
}

static enum drm_connector_status sp7350_dsi_encoder_detect(struct drm_encoder *encoder,
							   struct drm_connector *connector)
{
	DRM_DEBUG_DRIVER("[TODO]encoder %s detect connector:%s\n", encoder->name, connector->name);
	return connector->status;
}

/*
 * MIPI DSI (Display Command Set) for SP7350
 */
static ssize_t sp7350_dsi_host_transfer(struct mipi_dsi_host *host,
					const struct mipi_dsi_msg *msg)
{
	/* reference to vc4_dsi_host_transfer */
	/* simple for send packet only! */
	//sp7350_dcs_write_buf(msg->tx_buf, msg->tx_len);

	struct sp7350_drm_dsi *dsi = host_to_dsi(host);
	int i;
	u8 *data1;
	u32 value, data_cnt;

	//DRM_DEBUG_DRIVER("len %ld\n", msg->tx_len);
	//print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1,
	//	       msg->tx_buf, msg->tx_len, false);

	data1 = (u8 *)msg->tx_buf;

	udelay(100);
	if (msg->tx_len == 0) {
		check_dsi_cmd_fifo_full(dsi);
		value = 0x00000003;
		writel(value, dsi->regs + MIPITX_SPKT_HEAD); //G204.09
	} else if (msg->tx_len == 1) {
		check_dsi_cmd_fifo_full(dsi);
		value = 0x00000013 | (data1[0] << 8);
		writel(value, dsi->regs + MIPITX_SPKT_HEAD); //G204.09
	} else if (msg->tx_len == 2) {
		check_dsi_cmd_fifo_full(dsi);
		value = 0x00000023 | (data1[0] << 8) | (data1[1] << 16);
		writel(value, dsi->regs + MIPITX_SPKT_HEAD); //G204.09
	} else if ((msg->tx_len >= 3) && (msg->tx_len <= 64)) {
		check_dsi_cmd_fifo_full(dsi);
		value = 0x00000029 | ((u32)msg->tx_len << 8);
		writel(value, dsi->regs + MIPITX_LPKT_HEAD); //G204.10

		if (msg->tx_len % 4)
			data_cnt = ((u32)msg->tx_len / 4) + 1;
		else
			data_cnt = ((u32)msg->tx_len / 4);

		for (i = 0; i < data_cnt; i++) {
			check_dsi_data_fifo_full(dsi);
			value = 0x00000000;
			if (i * 4 + 0 < msg->tx_len)
				value |= (data1[i * 4 + 0] << 0);
			if (i * 4 + 1 < msg->tx_len)
				value |= (data1[i * 4 + 1] << 8);
			if (i * 4 + 2 < msg->tx_len)
				value |= (data1[i * 4 + 2] << 16);
			if (i * 4 + 3 < msg->tx_len)
				value |= (data1[i * 4 + 3] << 24);
			writel(value, dsi->regs + MIPITX_LPKT_PAYLOAD); //G204.11
		}
	} else {
		DRM_DEV_ERROR(&dsi->pdev->dev, "data length over %ld\n", msg->tx_len);
		return -1;
	}

	return 0;
}

static int sp7350_dsi_host_attach(struct mipi_dsi_host *host,
				  struct mipi_dsi_device *device)
{
	struct sp7350_drm_dsi *dsi = host_to_dsi(host);

	DRM_DEBUG_DRIVER("[Start]\n");
	if (!dsi->regs || !dsi->ao_moon3) {
		DRM_DEV_ERROR(&dsi->pdev->dev, "dsi host probe fail!.\n");
		return -1;
	}

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
		DRM_DEV_ERROR(&dsi->pdev->dev, "Unknown DSI format: %d.\n",
			      dsi->format);
		return -1;
	}

	if (!(dsi->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		DRM_DEV_ERROR(&dsi->pdev->dev,
			      "Only VIDEO mode panels supported currently.\n");
		return -1;
	}

	sp7350_mipitx_dsi_phy_init(dsi);
	sp7350_mipitx_dsi_pllclk_init(dsi);
	sp7350_mipitx_dsi_lane_control_set(dsi);
	sp7350_dsi_tcon_init(dsi);

	return 0;
}

static int sp7350_dsi_host_detach(struct mipi_dsi_host *host,
				  struct mipi_dsi_device *device)
{
	DRM_DEBUG_DRIVER("[TODO]\n");
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

	if (!crtc_mask) {
		DRM_DEV_ERROR(dev, "failed to find crtc mask\n");
		return -EINVAL;
	}

	encoder->possible_crtcs = crtc_mask;
	DRM_DEV_DEBUG_DRIVER(dev, "crtc_mask:0x%X\n", crtc_mask);
	ret = drm_simple_encoder_init(drm_dev, encoder, DRM_MODE_ENCODER_DSI);
	if (ret) {
		DRM_DEV_ERROR(dev, "failed to init dsi encoder\n");
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
	int child_count = 0;
	u32 endpoint_id = 0;
	struct device_node  *port, *endpoint;

	DRM_DEV_DEBUG_DRIVER(dev, "start.\n");
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

	init_completion(&dsi->xfer_completion);

	/*
	 * Get the endpoint node. In our case, dsi has one output port1
	 * to which the internal panel or external HDMI bridge connected.
	 * Cannot support both at the same time, internal panel first.
	 */
	//ret = drm_of_find_panel_or_bridge(dev->of_node, 1, 0, &panel, &dsi->bridge);
	port = of_graph_get_port_by_id(dev->of_node, 1);
	if (!port) {
		DRM_DEV_ERROR(dev,
			      "can't found port point, please init lvds panel port!\n");
		return -EINVAL;
	}
	for_each_child_of_node(port, endpoint) {
		child_count++;
		of_property_read_u32(endpoint, "reg", &endpoint_id);
		DRM_DEV_DEBUG(dev, "endpoint_id:%d\n", endpoint_id);
		ret = drm_of_find_panel_or_bridge(dev->of_node, 1, endpoint_id,
						  &panel, &dsi->bridge);
		of_node_put(endpoint);
		if (!ret) {
			break;
		}
	}
	of_node_put(port);
	if (!child_count) {
		DRM_DEV_ERROR(dev, "dsi0 port does not have any children\n");
		ret = -EINVAL;
		return ret;
	}
	if (ret) {
		DRM_DEV_ERROR(dev, "drm_of_find_panel_or_bridge failed -%d\n", -ret);
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

	ret = sp7350_drm_encoder_init(dev, drm, dsi->encoder);
	if (ret)
		return ret;

	ret = drm_bridge_attach(dsi->encoder, dsi->bridge, NULL, 0);
	if (ret) {
		DRM_DEV_ERROR(dev, "bridge attach failed: %d\n", ret);
		return ret;
	}

	/* FIXME, use firmware EDID for lt8912b */
	#if IS_ENABLED(CONFIG_DRM_LOAD_EDID_FIRMWARE) && IS_ENABLED(CONFIG_DRM_LONTIUM_LT8912B)
	{
		DRM_DEV_DEBUG_DRIVER(dev, "Use firmware EDID edid/1920x1080.bin for lt8912b output\n");
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

	DRM_DEV_DEBUG_DRIVER(dev, "finish.\n");
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

	/*
	 * get reg base resource
	 */
	dsi->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(dsi->regs))
		return dev_err_probe(&pdev->dev, PTR_ERR(dsi->regs), "dsi reg not found\n");
	//dsi->regs = sp7350_display_ioremap_regs(0);
	//if (IS_ERR(dsi->regs))
	//	return PTR_ERR(dsi->regs);
	dsi->ao_moon3 = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(dsi->ao_moon3))
		return dev_err_probe(&pdev->dev, PTR_ERR(dsi->ao_moon3), "dsi reg ao_moon3 not found\n");

	//sp7350_mipitx_phy_init();
	//sp7350_mipitx_pllclk_init();

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

	DRM_DEV_DEBUG_DRIVER(dev, "dsi driver remove.\n");
	if (dsi->encoder)
		sp7350_dsi_encoder_disable(dsi->encoder);

	component_del(&pdev->dev, &sp7350_dsi_ops);
	mipi_dsi_host_unregister(&dsi->dsi_host);

	return 0;
}

static int sp7350_dsi_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sp7350_drm_dsi *dsi = dev_get_drvdata(&pdev->dev);

	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "dsi driver suspend.\n");
	if (dsi->bridge)
		pm_runtime_put(&pdev->dev);

	/*
	 * phy mipitx registers store...
	 */

	/*
	 * TODO
	 * phy power off, disable clock, disable irq...
	 */

	if (dsi->encoder)
		sp7350_dsi_encoder_disable(dsi->encoder);

	return 0;
}

static int sp7350_dsi_dev_resume(struct platform_device *pdev)
{
	struct sp7350_drm_dsi *dsi = dev_get_drvdata(&pdev->dev);

	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "dsi driver resume.\n");
	if (dsi->bridge)
		pm_runtime_get(&pdev->dev);

	/*
	 * TODO
	 * phy power on, enable clock, enable irq...
	 */

	/*
	 * phy mipitx restore...
	 */
	sp7350_mipitx_dsi_phy_init(dsi);
	sp7350_mipitx_dsi_pllclk_init(dsi);
	sp7350_mipitx_dsi_lane_control_set(dsi);
	sp7350_mipitx_dsi_pllclk_set(dsi, &dsi->adj_mode_store);
	sp7350_mipitx_dsi_video_mode_setting(dsi, &dsi->adj_mode_store);

	if (dsi->encoder)
		sp7350_dsi_encoder_enable(dsi->encoder);

	return 0;
}

struct platform_driver sp7350_dsi_driver = {
	.probe   = sp7350_dsi_dev_probe,
	.remove  = sp7350_dsi_dev_remove,
	.suspend = sp7350_dsi_dev_suspend,
	.resume  = sp7350_dsi_dev_resume,
	.driver  = {
		.name = "sp7350_dsi",
		.of_match_table = sp7350_dsi_dt_match,
	},
};

/* sp7350_tcon_timing_set_dsi */
static void sp7350_dsi_tcon_timing_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode)
{
	u32 width, height;
	int i, time_cnt = 0;
	//u32 value = 0;

	width = mode->hdisplay;
	height = mode->vdisplay;

	for (i = 0; i < 11; i++) {
		if ((sp_tcon_para_dsi[i][0] == width) &&
			(sp_tcon_para_dsi[i][1] == height)) {
				time_cnt = i;
				break;
		}
	}

	pr_info("%s (w h)(%d %d)\n", __func__,
		sp_tcon_para_dsi[time_cnt][0], sp_tcon_para_dsi[time_cnt][1]);
	/*
	 * TCON H&V timing parameter
	 */
	writel(sp_tcon_para_dsi[time_cnt][2], dsi->regs + TCON_DE_HSTART); //DE_HSTART
	writel(sp_tcon_para_dsi[time_cnt][3], dsi->regs + TCON_DE_HEND); //DE_HEND

	writel(sp_tcon_para_dsi[time_cnt][4], dsi->regs + TCON_OEV_START); //TC_VSYNC_HSTART
	writel(sp_tcon_para_dsi[time_cnt][5], dsi->regs + TCON_OEV_END); //TC_VSYNC_HEND

	writel(sp_tcon_para_dsi[time_cnt][6], dsi->regs + TCON_HSYNC_START); //HSYNC_START
	writel(sp_tcon_para_dsi[time_cnt][7], dsi->regs + TCON_HSYNC_END); //HSYNC_END

	//writel(sp_tcon_para_dsi[time_cnt][8], dsi->regs + TCON_DE_VSTART); //DE_VSTART
	//writel(sp_tcon_para_dsi[time_cnt][9], dsi->regs + TCON_DE_VEND); //DE_VEND

	writel(sp_tcon_para_dsi[time_cnt][10], dsi->regs + TCON_STVU_START); //VTOP_VSTART
	writel(sp_tcon_para_dsi[time_cnt][11], dsi->regs + TCON_STVU_END); //VTOP_VEND

#if TCON_TPG_ENABLE
	/*
	 * TPG(Test Pattern Gen) parameter
	 */
	writel(sp_tcon_tpg_para_dsi[time_cnt][4], dsi->regs + TCON_TPG_HCOUNT);
	value |= (sp_tcon_tpg_para_dsi[time_cnt][2] << 12) | sp_tcon_tpg_para_dsi[time_cnt][5];
	writel(value, dsi->regs + TCON_TPG_VCOUNT);
	writel(sp_tcon_tpg_para_dsi[time_cnt][6], dsi->regs + TCON_TPG_HACT_COUNT);
	value = 0;
	value |= (sp_tcon_tpg_para_dsi[time_cnt][3] << 12) | sp_tcon_tpg_para_dsi[time_cnt][7];
	writel(value, dsi->regs + TCON_TPG_VACT_COUNT);

	writel(sp_tcon_tpg_para_dsi[time_cnt][8], dsi->regs + TCON_TPG_ALINE_START);

	//writel(sp_tcon_tpg_para_dsi[time_cnt][9], disp_dev->base + TCON_DITHER_TVOUT);
#endif
}

static int sp7350_dsi_mode_check(const struct drm_display_mode *mode)
{
	int i, time_cnt = 0;

	for (i = 0; i < 11; i++) {
		if ((sp_tcon_para_dsi[i][0] == mode->hdisplay) &&
			(sp_tcon_para_dsi[i][1] == mode->vdisplay)) {
				time_cnt = i;
				break;
		}
	}
	if (time_cnt >= 11) {
		DRM_ERROR("invalid mode with  (w h)(%d %d)\n", mode->hdisplay, mode->vdisplay);
		return -EINVAL;
	}

	for (i = 0; i < 11; i++) {
		if ((sp_mipitx_phy_pllclk_dsi[i][0] == mode->hdisplay) &&
		    (sp_mipitx_phy_pllclk_dsi[i][1] == mode->vdisplay)) {
			time_cnt = i;
			break;
		}
	}
	if (time_cnt >= 11) {
		DRM_ERROR("invalid mode with  (w h)(%d %d)\n", mode->hdisplay, mode->vdisplay);
		return -EINVAL;
	}

	return 0;
}

/*sp7350_tcon_init*/
static void sp7350_dsi_tcon_init(struct sp7350_drm_dsi *dsi)
{
	u32 value;

	value = 0;
	if (dsi->format == DSI_PFORMAT_RGB565)
		value |= SP7350_TCON_OUT_PACKAGE_SET(SP7350_TCON_OUT_PACKAGE_DSI_RGB565);
	else if (dsi->format == DSI_PFORMAT_RGB666)
		value |= SP7350_TCON_OUT_PACKAGE_SET(SP7350_TCON_OUT_PACKAGE_DSI_RGB666_18);
	else if (dsi->format == DSI_PFORMAT_RGB666_PACKED)
		value |= SP7350_TCON_OUT_PACKAGE_SET(SP7350_TCON_OUT_PACKAGE_DSI_RGB666_24);
	else if (dsi->format == DSI_PFORMAT_RGB888)
		value |= SP7350_TCON_OUT_PACKAGE_SET(SP7350_TCON_OUT_PACKAGE_DSI_RGB888);
	else
		value |= SP7350_TCON_OUT_PACKAGE_SET(SP7350_TCON_OUT_PACKAGE_DSI_RGB888);

	value |= SP7350_TCON_DOT_RGB888_MASK |
		SP7350_TCON_DOT_ORDER_SET(SP7350_TCON_DOT_ORDER_RGB) |
		SP7350_TCON_HVIF_EN | SP7350_TCON_YU_SWAP;

	writel(value, dsi->regs + TCON_TCON0);

	value = 0;
	value |= (SP7350_TCON_EN | SP7350_TCON_YUV_UV_SWAP |
		SP7350_TCON_STHLR_DLY_SET(SP7350_TCON_STHLR_DLY_1T));
	writel(value, dsi->regs + TCON_TCON1);

	value = 0;
	//value |= SP7350_TCON_HDS_FILTER;
	writel(value, dsi->regs + TCON_TCON2); //don't care

	value = 0;
	//value |= SP7350_TCON_OEH_POL;
	writel(value, dsi->regs + TCON_TCON3); //don't care

	value = 0;
	value |= SP7350_TCON_PIX_EN_SEL_SET(SP7350_TCON_PIX_EN_DIV_1_CLK_TCON);
	writel(value, dsi->regs + TCON_TCON4); //fixed , don't change it

	value = 0;
	value |= SP7350_TCON_CHK_SUM_EN;
	writel(value, dsi->regs + TCON_TCON5); //don't care

	value = readl(dsi->regs + MIPITX_INFO_STATUS); //G204.28
	if ((FIELD_GET(GENMASK(24,24), value) == 1) && (FIELD_GET(GENMASK(0,0), value) == 0)) {
		//pr_info("  MIPITX working, skip tcon setting\n");
		return;
	}
}

/* SP7350 MIPITX HW config reference to sp7350_disp_mipitx.c */
static void sp7350_mipitx_dsi_phy_init(struct sp7350_drm_dsi *dsi)
{
	u32 value;

	value = 0x00101330; //PHY Reset(under reset)
	//if (disp_dev->mipitx_clk_edge)
	//	value |= SP7350_MIPITX_MIPI_PHY_CLK_EDGE_SEL(SP7350_MIPITX_FALLING);
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL2); //G205.06

	if (dsi->lanes == 1)
		value = 0x11000001; //lane num = 1 and DSI_EN and ANALOG_EN
	else if (dsi->lanes == 2)
		value = 0x11000011; //lane num = 2 and DSI_EN and ANALOG_EN
	else if (dsi->lanes == 4)
		value = 0x11000031; //lane num = 4 and DSI_EN and ANALOG_EN
	writel(value, dsi->regs + MIPITX_CORE_CTRL); //G204.15

	value = 0x00000000;
	if (!(dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE))
		value |= SP7350_MIPITX_FORMAT_VTF_SET(SP7350_MIPITX_VTF_SYNC_EVENT);

	if (dsi->format == DSI_PFORMAT_RGB565)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB565);
	else if (dsi->format == DSI_PFORMAT_RGB666)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB666_18BITS);
	else if (dsi->format == DSI_PFORMAT_RGB666_PACKED)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB666_24BITS);
	else if (dsi->format == DSI_PFORMAT_RGB888)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB888);
	else
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB888);
	writel(value, dsi->regs + MIPITX_FORMAT_CTRL); //G204.12

	value = readl(dsi->regs + MIPITX_ANALOG_CTRL2);
	value |= SP7350_MIPITX_NORMAL; //PHY Reset(under normal mode)
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL2); //G205.06
}

static void sp7350_mipitx_dsi_pllclk_init(struct sp7350_drm_dsi *dsi)
{
	u32 value, value1, value2;

	value = 0x80000000; //init clock
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL9); //G205.14

	//init PLLH setting
	value = 0xffff40be; //PLLH BNKSEL = 0x2 (2000~2500MHz)
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_15); //AO_G3.15
	value = 0xffff0009; //PLLH
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_16); //AO_G3.16
	/*
	 * PLLH Fvco = 2150MHz (fixed)
	 *                             2150
	 * MIPITX pixel CLK = ----------------------- = 59.72MHz
	 *                     PST_DIV * MIPITX_SEL
	 */
	value = 0xffff0b50; //PLLH PST_DIV = div9 (default)
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14
	value = 0x07800180; //PLLH MIPITX_SEL = div4
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25

	//init TXPLL setting
	value = 0x00000003; //TXPLL enable and reset
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL5); //G205.10
	/*
	 * PRESCAL = 1, FBKDIV = 48, PRE_DIV = 1, EN_DIV5 = 0, PRE_DIV = 2, POST_DIV = 1
	 *                    25 * PRESCAL * FBKDIV            25 * 48
	 * MIPITX bit CLK = ------------------------------ = ----------- = 600MHz
	 *                   PRE_DIV * POST_DIV * 5^EN_DIV5       2
	 */
	value1 = 0x00003001; //TXPLL MIPITX CLK = 600MHz
	value2 = 0x00000140; //TXPLL BNKSEL = 0x0 (320~640MHz)
	writel(value1, dsi->regs + MIPITX_ANALOG_CTRL6); //G205.11
	writel(value2, dsi->regs + MIPITX_ANALOG_CTRL7); //G205.12
	/*
	 *                      600
	 * MIPITX LP CLK = ------------ = 8.3MHz
	 *                   8 * div9
	 */
	value = 0x00000008; //(600/8/div9)=8.3MHz
	writel(value, dsi->regs + MIPITX_LP_CK); //G204.04

	value = 0x00000000; //init clock done
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL9); //G205.14

	//sp7350_mipitx_pllclk_get();
	//sp7350_mipitx_txpll_get();
}

static void sp7350_mipitx_dsi_pllclk_set(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode)
{
	int i, time_cnt = 0;
	u32 value;

	for (i = 0; i < 11; i++) {
		if ((sp_mipitx_phy_pllclk_dsi[i][0] == mode->hdisplay) &&
		    (sp_mipitx_phy_pllclk_dsi[i][1] == mode->vdisplay)) {
			time_cnt = i;
			break;
		}
	}

#if 0
	if ((mode->hdisplay == 240) && (mode->vdisplay == 320)) {
		value = 0;
		value |= 0x80000000;
		value |= 0x00780050;
		value |= (0x7f800000 | (0x14 << 7));
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

		value = 0x07800780; //PLLH MIPITX CLK = 14.583MHz
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
	} else if ((mode->hdisplay == 800) && (mode->vdisplay == 480)) {
		value = 0;
		value |= 0x00780058;
		value |= (0x7f800000 | (0x15 << 7));
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

		value = 0x07800380; //PLLH MIPITX CLK = 26.563MHz
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
	} else if ((mode->hdisplay == 720) && (mode->vdisplay == 480)) {
		value = 0;
		value |= 0x00780050;
		value |= (0x7f800000 | (0xe << 7));
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

		value = 0x07800380; //PLLH MIPITX CLK = 27.08MHz
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
	} else if ((mode->hdisplay == 1280) && (mode->vdisplay == 720)) {
		value = 0;
		value |= 0x00780038;
		value |= (0x7f800000 | (0x13 << 7));
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

		value = 0x07800180; //PLLH MIPITX CLK = 74MHz
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
	} else if ((mode->hdisplay == 1920) && (mode->vdisplay == 1080)) {
		value = 0;
		value |= 0x00780038;
		value |= (0x7f800000 | (0x13 << 7));
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

		value = 0x07800080; //PLLH MIPITX CLK = 148MHz
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
	} else {
		value = 0;
		value |= (0x00780000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][8] << 3));
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

		value = 0;
		value |= (0x07800000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][9] << 7));
		writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
	}
#else
	/* default set at sp7350_mipitx_dsi_pllclk_init:
	 * PRESCALER_H[15]=0, PREDIV_H[2:1]=0.
	 * AND:
	 * FBK_DIV_H[14:7]    = sp_mipitx_phy_pllclk_dsi[time_cnt][8]
	 * PSTDIV_H[6:3]      = sp_mipitx_phy_pllclk_dsi[time_cnt][9]
	 * MIPITX_SEL_H[11:7] = sp_mipitx_phy_pllclk_dsi[time_cnt][10]
	 * BNKSEL_H[1:0]      = sp_mipitx_phy_pllclk_dsi[time_cnt][11]
	 */

	value = 0;
	/* Update FBK_DIV_H[12:7] */
	value |= (0x7f800000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][8] << 7));
	/* Update PSTDIV_H[6:3] */
	value |= (0x00780000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][9] << 3));
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

	value = 0;
	/* Update BNKSEL_H[1:0] */
	value = 0x00030000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][11]);
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_15); //AO_G3.15

	value = 0;
	/* Update MIPITX_SEL_H[11:7] */
	value |= (0x0f800000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][10] << 7));
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
#endif

	value = 0x00000000;
	value |= (SP7350_MIPITX_MIPI_PHY_EN_DIV5(sp_mipitx_phy_pllclk_dsi[time_cnt][6]) |
			SP7350_MIPITX_MIPI_PHY_POSTDIV(sp_mipitx_phy_pllclk_dsi[time_cnt][5]) |
			SP7350_MIPITX_MIPI_PHY_FBKDIV(sp_mipitx_phy_pllclk_dsi[time_cnt][3]) |
			SP7350_MIPITX_MIPI_PHY_PRESCALE(sp_mipitx_phy_pllclk_dsi[time_cnt][2]) |
			SP7350_MIPITX_MIPI_PHY_PREDIV(sp_mipitx_phy_pllclk_dsi[time_cnt][4]));
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL6); //G205.11

	value = readl(dsi->regs + MIPITX_ANALOG_CTRL7); //G205.12
	value &= ~(SP7350_MIPITX_MIPI_PHY_BNKSEL_MASK);
	value |= SP7350_MIPITX_MIPI_PHY_BNKSEL(sp_mipitx_phy_pllclk_dsi[time_cnt][7]);
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL7); //G205.12
}

/* sp_mipitx_input_timing_dsi */
static void sp7350_mipitx_dsi_video_mode_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode)
{
	u32 width, height, data_bit;
	u32 value;

	width = mode->hdisplay;
	height = mode->vdisplay;
	data_bit = dsi->divider * dsi->lanes;

	value = 0;
	value |= SP7350_MIPITX_HSA_SET(mode->hsync_end - mode->hsync_start) |
		SP7350_MIPITX_HFP_SET(mode->hsync_start - mode->hdisplay) |
		SP7350_MIPITX_HBP_SET(mode->htotal - mode->hsync_end);
	writel(value, dsi->regs + MIPITX_VM_HT_CTRL);

	value = 0;
	value |= SP7350_MIPITX_VSA_SET(mode->vsync_end - mode->vsync_start) |
		SP7350_MIPITX_VFP_SET(mode->vsync_start - mode->vdisplay) |
		SP7350_MIPITX_VBP_SET(mode->vtotal - mode->vsync_end);
	writel(value, dsi->regs + MIPITX_VM_VT0_CTRL);

	value = 0;
	value |= SP7350_MIPITX_VACT_SET(mode->vdisplay);
	writel(value, dsi->regs + MIPITX_VM_VT1_CTRL);

	//MIPITX  Video Mode WordCount Setting
	value = 0;
	value |= ((width << 16) | ((width * data_bit) / 8));
	writel(value, dsi->regs + MIPITX_WORD_CNT); //G204.19
}

static void sp7350_mipitx_dsi_cmd_mode_start(struct sp7350_drm_dsi *dsi)
{
	u32 value;

	value = 0x00100000; //enable command transfer at LP mode
	writel(value, dsi->regs + MIPITX_OP_CTRL); //G204.14

	if (dsi->lanes == 1)
		value = 0x11000003; //lane num = 1 and command mode start
	else if (dsi->lanes == 2)
		value = 0x11000013; //lane num = 2 and command mode start
	else if (dsi->lanes == 4)
		value = 0x11000033; //lane num = 4 and command mode start
	writel(value, dsi->regs + MIPITX_CORE_CTRL); //G204.15

	value = 0x00520004; //TA GET/SURE/GO
	writel(value, dsi->regs + MIPITX_BTA_CTRL); //G204.17

	//value = 0x0000c350; //fix
	//value = 0x000000af; //fix
	//value = 0x00000aff; //default
	//writel(value, dsi->regs + MIPITX_ULPS_DELAY); //G204.29
}

static void sp7350_mipitx_dsi_lane_control_set(struct sp7350_drm_dsi *dsi)
{
	u32 value = 0;

	value |= ((sp_mipitx_output_timing[0] << 16) |
			(sp_mipitx_output_timing[1] << 0));
	writel(value, dsi->regs + MIPITX_LANE_TIME_CTRL); //G204.05

	value = 0;
	value |= ((sp_mipitx_output_timing[2] << 16) |
			(sp_mipitx_output_timing[3] << 0));
	writel(value, dsi->regs + MIPITX_CLK_TIME_CTRL0); //G204.06

	value = 0;
	value |= ((sp_mipitx_output_timing[4] << 25) |
			(sp_mipitx_output_timing[5] << 16) |
			(sp_mipitx_output_timing[6] << 0));
	writel(value, dsi->regs + MIPITX_CLK_TIME_CTRL1); //G204.07

	value = 0;
	value |= ((sp_mipitx_output_timing[7] << 25) |
			(sp_mipitx_output_timing[8] << 16) |
			(sp_mipitx_output_timing[9] << 0));
	writel(value, dsi->regs + MIPITX_DATA_TIME_CTRL0); //G204.08

	value = 0x00001100; //MIPITX Blanking Mode
	writel(value, dsi->regs + MIPITX_BLANK_POWER_CTRL); //G204.13

	value = 0x00000001; //MIPITX CLOCK CONTROL (CK_HSEN)
	writel(value, dsi->regs + MIPITX_CLK_CTRL); //G204.30
}

static void sp7350_mipitx_dsi_video_mode_on(struct sp7350_drm_dsi *dsi)
{
	u32 value;

	value = 0x11000000; //DSI_EN and ANALOG_EN
	//MIPITX SWITCH to Video Mode
	if (dsi->lanes == 1)
		value = 0x11000001; //lane num = 1 and DSI_EN and ANALOG_EN
	else if (dsi->lanes == 2)
		value = 0x11000011; //lane num = 2 and DSI_EN and ANALOG_EN
	else if (dsi->lanes == 4)
		value = 0x11000031; //lane num = 4 and DSI_EN and ANALOG_EN
	writel(value, dsi->regs + MIPITX_CORE_CTRL); //G204.15
}

static void check_dsi_cmd_fifo_full(struct sp7350_drm_dsi *dsi)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
	//pr_info("fifo_status 0x%08x\n", value);
	while ((value & MIPITX_CMD_FIFO_FULL) == MIPITX_CMD_FIFO_FULL) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("cmd fifo full timeout\n");
			break;
		}
		value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
		++mipitx_fifo_timeout;
		udelay(100);
	}
}

#if SP7350_DRM_TODO
static void check_dsi_cmd_fifo_empty(struct sp7350_drm_dsi *dsi)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
	//pr_info("fifo_status 0x%08x\n", value);
	while ((value & MIPITX_CMD_FIFO_EMPTY) != MIPITX_CMD_FIFO_EMPTY) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("cmd fifo empty timeout\n");
			break;
		}
		value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
		++mipitx_fifo_timeout;
		udelay(100);
	}
}
#endif

static void check_dsi_data_fifo_full(struct sp7350_drm_dsi *dsi)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
	//pr_info("fifo_status 0x%08x\n", value);
	while ((value & MIPITX_DATA_FIFO_FULL) == MIPITX_DATA_FIFO_FULL) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("data fifo full timeout\n");
			break;
		}
		value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
		++mipitx_fifo_timeout;
		udelay(100);
	}
}

#if SP7350_DRM_TODO
static void check_dsi_data_fifo_empty(struct sp7350_drm_dsi *dsi)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
	//pr_info("fifo_status 0x%08x\n", value);
	while ((value & MIPITX_DATA_FIFO_EMPTY) != MIPITX_DATA_FIFO_EMPTY) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("data fifo empty timeout\n");
			break;
		}
		value = readl(dsi->regs + MIPITX_CMD_FIFO); //G204.16
		++mipitx_fifo_timeout;
		udelay(100);
	}
}
#endif

