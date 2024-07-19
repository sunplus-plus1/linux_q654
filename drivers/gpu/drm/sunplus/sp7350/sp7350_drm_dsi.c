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

# define DSI_PFORMAT_RGB565          0
# define DSI_PFORMAT_RGB666_PACKED   1
# define DSI_PFORMAT_RGB666          2
# define DSI_PFORMAT_RGB888          3

#define MIPITX_CMD_FIFO_FULL   0x00000001
#define MIPITX_CMD_FIFO_EMPTY  0x00000010
#define MIPITX_DATA_FIFO_FULL  0x00000100
#define MIPITX_DATA_FIFO_EMPTY 0x00001000

/* display TCON Timing Parameters Setting.
 * HW Specification definition:
 *   TCON_HSA=4
 *   TCON_HBP=4
 *   TCON_VSA=1
 * Formula:
 *   de_hstart    = 0 (Horizontal reference starting point)
 *   de_hend      = de_hstart+hdisplay-1 = hdisplay-1
 *   hsync_start  = htotal-TCON_HSA-TCON_HBP = htotal-4-4
 *   hsync_end    = hsync_start + TCON_HSA = htotal-4
 *   de_oev_start = hsync_start
 *   de_oev_end   = hsync_end
 *   stvu_start   = vtotal-TCON_VSA = vtotal-1
 *   stvu_end     = 0 (Vertical reference starting point)
 *
 * Notes: hdisplay,htotal,vtotal are derived from drm_display_mode.
 */
struct sp7350_drm_tcon_timing_param {
	u32 de_hstart;
	u32 de_hend;
	u32 de_oev_start;
	u32 de_oev_end;
	u32 hsync_start;
	u32 hsync_end;
	u32 stvu_start;
	u32 stvu_end;
};

/* display MIPITX Sync-Timing Parameters Setting.
 * Formula:
 *   hsa = hsync_end-hsync_start
 *   hbp = htotal-hsync_end
 *   hact = hdisplay
 *   vsa = vsync_end-vsync_start
 *   vfp = vsync_start-vdisplay
 *   vbp = vtotal-vsync_end
 *   vact = vdisplay
 *
 * Notes: hsync_end,hsync_start,htotal,vsync_end,vsync_start,vtotal,vdisplay
 *   are derived from drm_display_mode.
 */
struct sp7350_drm_mpitx_sync_timing_param {
	u32 hsa;
	u32 hbp;
	u32 hact;
	u32 vsa;
	u32 vfp;
	u32 vbp;
	u32 vact;
};

/* display MIPITX Lane Clock and TXPLL Parameters Setting.
 * MIPI Lane Clock Formula:
 *                   pixel_clock * pixel_bits
 *   lane_clock = -----------------------------
 *                         data_lanes
 *
 * TXPLL Clock Formula:
 *                    25 * prescal * fbkdiv
 *   lane_clock = -----------------------------
 *                 prediv * postdiv * 5^en_div5
 *  Fvco = (25 * prescal * fbkdiv) / prediv
 *  lane_clock = Fvco / (postdiv * 5^en_div5)
 *  lane_divider = pixel_bits / data_lanes
 *==>
 *  lane_clock = pixel_clock * pixel_bits / data_lanes
 *             = pixel_clock * lane_divider
 *  Fvco = lane_clock * txpll_postdiv * 5^txpll_endiv5
 *  txpll_fbkdiv = Fvco * txpll_prediv / (25 * txpll_prescal)
 *
 * PreSetting-Rule:
 *   txpll_prescal=1
 *   txpll_prediv =1
 *   lane_clock = [80, 150)MHz, txpll_endiv5=1, txpll_postdiv=2
 *   lane_clock = [150,375)MHz, txpll_endiv5=0, txpll_postdiv=4
 *   lane_clock = [375,1500)MHz, txpll_endiv5=0, txpll_postdiv=1
 *   Fvco = [ 320,  640]MHz, txpll_bnksel=0
 *   Fvco = [ 640, 1000]MHz, txpll_bnksel=1
 *   Fvco = [1000, 1200]MHz, txpll_bnksel=2
 *   Fvco = [1200, 1500]MHz, txpll_bnksel=3
 *
 * Register Setting Formula:
 *   txpll_prescal= PRESCAL[4]+1 = {1, 2}
 *   txpll_prediv = map{PREDIV[1:0]} = {1, 2, 5, 8}
 *   txpll_postdiv= map{POSTDIV[18:16]} = {1, 2, 4, 8, 16}
 *   txpll_endiv5 = EN_DIV5[20] = {0, 1}
 *   txpll_fbkdiv = FBK_DIV[13:8] = [3, 63];
 *   txpll_bnksel = BNKSEL[2:0] = [0, 3]
 *==>
 *   PRESCAL[4]  = txpll_prescal-1
 *   PREDIV[1:0] = 3 for txpll_prediv = 8
 *   PREDIV[1:0] = txpll_prediv / 2
 *   POSTDIV[18:16] = log2(txpll_postdiv)
 *   EN_DIV5[20]   = txpll_endiv5
 *   FBKDIV[13:8] = fbkdiv;
 *   BNKSEL[2:0]   = txpll_bnksel
 *
 * Notes: pixel_clock, from drm_display_mode. lane_divider from dsi driver.
 */
struct sp7350_drm_mpitx_lane_clock {
	/**
	 * @clock:
	 *
	 * mipitx lane clock in kHz.
	 */
	int clock;
	u32 txpll_prescal;
	u32 txpll_fbkdiv;
	u32 txpll_prediv;
	u32 txpll_postdiv;
	u32 txpll_endiv5;
	u32 txpll_bnksel;
};

/* display MIPITX Pixel Clock and PLLH Parameters Setting.
 * MIPI Pixel Clock PLLH Formula:
 *                   25M * prescal * fbkdiv
 *   pixel_clock = -----------------------------
 *                 prediv * postdiv * seldiv
 *  Fvco = (25 * prescal * fbkdiv) / prediv
 *  pixel_clock = Fvco / (postdiv * seldiv)
 *  postdiv_10x = postdiv * 10
 *==>
 *  Fvco = pixel_clock * postdiv_10x * seldiv / 10
 *  fbkdiv = Fvco * prediv / (25 * prescal)
 *
 * PreSetting-Rule:
 *   prescal=1
 *   pixel_clock = [  5,   8)MHz, prediv =2, postdiv_10x=125, seldiv=16
 *   pixel_clock = [  8,  14)MHz, prediv =1, postdiv_10x=125, seldiv=16
 *   pixel_clock = [ 14,  20)MHz, prediv =1, postdiv_10x=90,  seldiv=16
 *   pixel_clock = [ 20,  29)MHz, prediv =1, postdiv_10x=125, seldiv=8
 *   pixel_clock = [ 29,  40)MHz, prediv =1, postdiv_10x=90,  seldiv=8
 *   pixel_clock = [ 40,  70)MHz, prediv =1, postdiv_10x=25,  seldiv=16
 *   pixel_clock = [ 70, 112)MHz, prediv =1, postdiv_10x=125, seldiv=2
 *   pixel_clock = [112, 160)MHz, prediv =1, postdiv_10x=90,  seldiv=2
 *   pixel_clock = [160, 230)MHz, prediv =1, postdiv_10x=125, seldiv=1
 *   pixel_clock = [230, 320)MHz, prediv =1, postdiv_10x=90,  seldiv=1
 *   pixel_clock = [320, 540)MHz, prediv =1, postdiv_10x=25,  seldiv=2
 *   pixel_clock = [540, 900)MHz, prediv =1, postdiv_10x=30,  seldiv=1
 *   pixel_clock = [900,1200)MHz, prediv =1, postdiv_10x=25,  seldiv=1
 *   Fvco = [1000, 1500]MHz, bnksel=0
 *   Fvco = [1500, 2000]MHz, bnksel=1
 *   Fvco = [2000, 2500]MHz, bnksel=2
 *   Fvco = [2500, 3000]MHz, bnksel=3
 *
 * Register Setting Formula:
 *   prescal= PRESCAL_H[15]+1 = {1, 2}
 *   prediv = PREDIV_H[2:1]+1 = {1, 2}
 *   postdiv= map{PSTDIV_H[6:3]} = {2.5, 3, 3.5, 4, 5, 5.5, 6, 7, 7.5, 8, 9, 10, 10.5, 11, 12, 12.5}
 *   postdiv_10x = postdiv * 10
 *   fbkdiv = FBKDIV_H[14:7] + 64 = [64, 127];
 *   seldiv = bitmap{MIPITX_SELDIV_H[11:7]} = {1, 2, 4, 8, 16}.
 *   bnksel = BNKSEL_H[1:0] = [0, 3]
 *==>
 *   PRESCAL_H[15] = prescal-1
 *   PREDIV_H[2:1] = prediv-1
 *   FBKDIV_H[14:7] = fbkdiv-64
 *   MIPITX_SELDIV_H[11:7] = log2(seldiv)
 *   PSTDIV_H[6:3]   = (postdiv_10x-25)/5   for postdiv_10x<=40
 *   PSTDIV_H[6:3]   = (postdiv_10x-50)/5+4 for postdiv_10x<=60
 *   PSTDIV_H[6:3]   = (postdiv_10x-70)/5+7 for postdiv_10x<=80
 *   PSTDIV_H[6:3]   = 10 for postdiv_10x=90
 *   PSTDIV_H[6:3]   = (postdiv_10x-100)/5+11 for postdiv_10x<=110
 *   PSTDIV_H[6:3]   = (postdiv_10x-120)/5+14 for postdiv_10x<=125
 *   BNKSEL_H[1:0]   = bnksel
 *
 * Notes: pixel_clock, from drm_display_mode.
 */
struct sp7350_drm_mpitx_pixel_clock {
	/**
	 * @clock:
	 *
	 * mipitx pixel clock in kHz.
	 */
	int clock;
	u32 prescal;
	u32 fbkdiv;
	u32 prediv;
	u32 postdiv_10x;
	u32 seldiv;
	u32 bnksel;
};

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
	//struct clk *escape_clock;

	/* Input clock to the analog PHY, used to generate the DSI bit
	 * clock.
	 */
	//struct clk *pll_phy_clock;

	/* HS Clocks generated within the DSI analog PHY. */
	//struct clk_fixed_factor phy_clocks[3];

	//struct clk_hw_onecell_data *clk_onecell;

	/* Pixel clock output to the pixelvalve, generated from the HS
	 * clock.
	 */
	//struct clk *pixel_clock;
	struct sp7350_drm_mpitx_lane_clock lane_clock;
	struct sp7350_drm_mpitx_pixel_clock pixel_clock;

	struct completion xfer_completion;
	int xfer_result;

	struct debugfs_regset32 regset;
	struct debugfs_regset32 ao_moon3_regset;
	struct drm_display_mode adj_mode_store;
	struct sp7350_drm_tcon_timing_param tcon_timing;
	struct sp7350_drm_mpitx_sync_timing_param mipitx_sync_timing;
};

#define host_to_dsi(host) container_of(host, struct sp7350_drm_dsi, dsi_host)

/* DSI encoder KMS struct */
struct sp7350_dsi_encoder {
	struct sp7350_drm_encoder base;
	struct sp7350_drm_dsi *dsi;
};

#define to_sp7350_dsi_encoder(target)\
	container_of(target, struct sp7350_dsi_encoder, base.base)

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

static void sp7350_dsi_tcon_init(struct sp7350_drm_dsi *dsi);
static void sp7350_dsi_tcon_timing_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode);
static void sp7350_mipitx_dsi_phy_init(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_pllclk_init(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_lane_control_set(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_lane_clock_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode);
static void sp7350_mipitx_dsi_pixel_clock_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode);
static void sp7350_mipitx_dsi_video_mode_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode);
static void sp7350_mipitx_dsi_cmd_mode_start(struct sp7350_drm_dsi *dsi);
static void sp7350_mipitx_dsi_video_mode_on(struct sp7350_drm_dsi *dsi);
static void check_dsi_cmd_fifo_full(struct sp7350_drm_dsi *dsi);
static void check_dsi_data_fifo_full(struct sp7350_drm_dsi *dsi);

static enum drm_mode_status _sp7350_dsi_encoder_phy_mode_valid(
					struct drm_encoder *encoder,
					const struct drm_display_mode *mode)
{
	/* Any display HW(TCON/MIPITX DSI/TXPLL/PLLH...) Limit??? */
	if (mode->clock > 375000)
		return MODE_CLOCK_HIGH;
	if (mode->clock < 5000)
		return MODE_CLOCK_LOW;

	if (mode->hdisplay > 1920)
		return MODE_BAD_HVALUE;

	if (mode->vdisplay > 1080)
		return MODE_BAD_VVALUE;

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
	sp7350_mipitx_dsi_pixel_clock_setting(dsi, adj_mode);
	sp7350_mipitx_dsi_lane_clock_setting(dsi, adj_mode);
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

static void sp7350_dsi_encoder_atomic_mode_set(struct drm_encoder *encoder,
				struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	sp7350_dsi_encoder_mode_set(encoder, &crtc_state->mode, &crtc_state->adjusted_mode);
}


static void sp7350_dsi_encoder_atomic_disable(struct drm_encoder *encoder,
			       struct drm_atomic_state *state)
{
	sp7350_dsi_encoder_disable(encoder);
}

static void sp7350_dsi_encoder_atomic_enable(struct drm_encoder *encoder,
			      struct drm_atomic_state *state)
{
	sp7350_dsi_encoder_enable(encoder);
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
	//.mode_set	= sp7350_dsi_encoder_mode_set,
	//.disable = sp7350_dsi_encoder_disable,
	//.enable = sp7350_dsi_encoder_enable,
	.atomic_mode_set = sp7350_dsi_encoder_atomic_mode_set,
	.atomic_disable = sp7350_dsi_encoder_atomic_disable,
	.atomic_enable = sp7350_dsi_encoder_atomic_enable,
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
	sp7350_dsi_tcon_init(dsi);
	sp7350_mipitx_dsi_phy_init(dsi);
	sp7350_mipitx_dsi_pllclk_init(dsi);
	sp7350_mipitx_dsi_lane_control_set(dsi);
	sp7350_dsi_tcon_timing_setting(dsi, NULL);
	sp7350_mipitx_dsi_pixel_clock_setting(dsi, &dsi->adj_mode_store);
	sp7350_mipitx_dsi_lane_clock_setting(dsi, NULL);
	sp7350_mipitx_dsi_video_mode_setting(dsi, NULL);

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
	struct sp7350_drm_tcon_timing_param *tcon_timing = &dsi->tcon_timing;
	if (mode) {
		u16 tcon_hsa = 4;
		u16 tcon_hbp = 4;
		u16 tcon_vsa = 1;

		tcon_timing->de_hstart    = 0;
		tcon_timing->de_hend      = tcon_timing->de_hstart + mode->hdisplay - 1;
		tcon_timing->hsync_start  = mode->htotal - tcon_hsa - tcon_hbp;
		tcon_timing->hsync_end    = tcon_timing->hsync_start + tcon_hsa;
		tcon_timing->de_oev_start = tcon_timing->hsync_start;
		tcon_timing->de_oev_end   = tcon_timing->hsync_end;
		tcon_timing->stvu_start   = mode->vtotal - tcon_vsa;
		tcon_timing->stvu_end     = 0;
	}

	/*
	 * TCON H&V timing parameter
	 */
	writel(tcon_timing->de_hstart, dsi->regs + TCON_DE_HSTART); //DE_HSTART
	writel(tcon_timing->de_hend, dsi->regs + TCON_DE_HEND); //DE_HEND

	writel(tcon_timing->de_oev_start, dsi->regs + TCON_OEV_START); //TC_VSYNC_HSTART
	writel(tcon_timing->de_oev_end, dsi->regs + TCON_OEV_END); //TC_VSYNC_HEND

	writel(tcon_timing->hsync_start, dsi->regs + TCON_HSYNC_START); //HSYNC_START
	writel(tcon_timing->hsync_end, dsi->regs + TCON_HSYNC_END); //HSYNC_END

	//writel(0, dsi->regs + TCON_DE_VSTART); //DE_VSTART
	//writel(0, dsi->regs + TCON_DE_VEND); //DE_VEND

	writel(tcon_timing->stvu_start, dsi->regs + TCON_STVU_START); //VTOP_VSTART
	writel(tcon_timing->stvu_end, dsi->regs + TCON_STVU_END); //VTOP_VEND
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

/* display MIPITX Lane Clock and TXPLL Parameters Setting. */
static void sp7350_mipitx_dsi_lane_clock_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode)
{
	u32 value;
	u32 reg_prediv;
	u32 reg_postdiv;
	struct sp7350_drm_mpitx_lane_clock *lane_clock = &dsi->lane_clock;

	if (mode) {
		int fvco; /* KHz */
		/* MIPI Lane Clock Formula:
		 *  lane_divider = pixel_bits / data_lanes
		 *  lane_clock = pixel_clock * pixel_bits / data_lanes
		 *             = pixel_clock * lane_divider
		 *
		 *  Fvco = (25 * prescal * fbkdiv) / prediv
		 *  lane_clock = Fvco / (postdiv * 5^en_div5)
		 * ==>
		 *  Fvco = lane_clock * postdiv * 5^en_div5
		 *  fbkdiv = Fvco * prediv / (25 * prescal)
		 *
		 * PreSetting-Rule:
		 *   txpll_prescal=1
		 *   txpll_prediv =1
		 *   lane_clock = [80, 150)MHz, txpll_endiv5=1, txpll_postdiv=2
		 *   lane_clock = [150MHz,375)MHz, txpll_endiv5=0, txpll_postdiv=4
		 *   lane_clock = [375MHz,1500)MHz, txpll_endiv5=0, txpll_postdiv=1
		 *   Fvco = [ 320,  640]MHz, txpll_bnksel=0
		 *   Fvco = [ 640, 1000]MHz, txpll_bnksel=1
		 *   Fvco = [1000, 1200]MHz, txpll_bnksel=2
		 *   Fvco = [1200, 1500]MHz, txpll_bnksel=3
		 */
		lane_clock->txpll_prescal = 1;
		lane_clock->txpll_prediv  = 1;

		lane_clock->clock    = mode->clock * dsi->divider;
		if (lane_clock->clock < 150000) {
			lane_clock->txpll_postdiv = 2;
			lane_clock->txpll_endiv5  = 1;
		}
		else if (lane_clock->clock < 375000) {
			lane_clock->txpll_postdiv = 4;
			lane_clock->txpll_endiv5  = 0;
		}
		else {
			lane_clock->txpll_postdiv = 1;
			lane_clock->txpll_endiv5  = 0;
		}
		fvco = lane_clock->clock * lane_clock->txpll_postdiv * (lane_clock->txpll_endiv5 ? 5 : 1);
		if (fvco < 640000) {
			lane_clock->txpll_bnksel = 0;
		}
		else if (fvco < 1000000) {
			lane_clock->txpll_bnksel = 1;
		}
		else if (fvco < 1200000) {
			lane_clock->txpll_bnksel = 2;
		}
		else {
			lane_clock->txpll_bnksel = 3;
		}
		lane_clock->txpll_fbkdiv =  fvco * lane_clock->txpll_prediv / (25000 * lane_clock->txpll_prescal);
		if ((fvco * lane_clock->txpll_prediv) % (25000 * lane_clock->txpll_prescal)) {
			lane_clock->txpll_fbkdiv += 1;
		}
		DRM_DEBUG_DRIVER("\nMIPITX Lane Clock Info:\n"
							"   %dKHz(pixel:%dKHz), Fvco=%dKHz \n"
							"   txpll_prescal=%d, txpll_prediv=%d\n"
							"   txpll_postdiv=%d, txpll_endiv5=%d\n"
							"   txpll_fbkdiv=%d, txpll_bnksel=%d\n",
			lane_clock->clock, mode->clock, fvco,
			lane_clock->txpll_prescal, lane_clock->txpll_prediv,
			lane_clock->txpll_postdiv, lane_clock->txpll_endiv5,
			lane_clock->txpll_fbkdiv, lane_clock->txpll_bnksel);
	}

	/* Register Setting Formula:
	 *   txpll_prescal= PRESCAL[4]+1 = {1, 2}
	 *   txpll_prediv = map{PREDIV[1:0]} = {1, 2, 5, 8}
	 *   txpll_postdiv= map{POSTDIV[18:16]} = {1, 2, 4, 8, 16}
	 *   txpll_endiv5 = EN_DIV5[20] = {0, 1}
	 *   txpll_fbkdiv = FBK_DIV[13:8] = [3, 63];
	 *   txpll_bnksel = BNKSEL[2:0] = [0, 3]
	 *==>
	 *   PRESCAL[4]  = txpll_prescal-1
	 *   PREDIV[1:0] = 3 for txpll_prediv = 8
	 *   PREDIV[1:0] = txpll_prediv / 2
	 *   POSTDIV[18:16] = log2(txpll_postdiv)
	 *   EN_DIV5[20]   = txpll_endiv5
	 *   FBK_DIV[13:8] = fbkdiv;
	 *   BNKSEL[2:0]   = txpll_bnksel
	 */
	reg_prediv = lane_clock->txpll_prediv / 2;
	if (reg_prediv > 3)
		reg_prediv = 3;

	reg_postdiv = 0;
	value = lane_clock->txpll_postdiv / 2;
	while (value) {
		value /= 2;
		reg_postdiv++;
	}
	DRM_DEBUG_DRIVER("\nMIPITX PLL Setting:\n"
						"   %dKHz\n"
						"   PRESCAL[4]=%d, PREDIV[1:0]=%d\n"
						"   POSTDIV[18:16]=%d, EN_DIV5[20]=%d\n"
						"   FBK_DIV[13:8]=%d, BNKSEL[2:0]=%d\n",
		lane_clock->clock,
		lane_clock->txpll_prescal-1, reg_prediv,
		reg_postdiv, lane_clock->txpll_endiv5,
		lane_clock->txpll_fbkdiv, lane_clock->txpll_bnksel);

	value = 0x00000000;
	value |= (SP7350_MIPITX_MIPI_PHY_EN_DIV5(lane_clock->txpll_endiv5) |
			SP7350_MIPITX_MIPI_PHY_POSTDIV(reg_postdiv) |
			SP7350_MIPITX_MIPI_PHY_FBKDIV(lane_clock->txpll_fbkdiv) |
			SP7350_MIPITX_MIPI_PHY_PRESCALE(lane_clock->txpll_prescal-1) |
			SP7350_MIPITX_MIPI_PHY_PREDIV(reg_prediv));
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL6); //G205.11

	value = readl(dsi->regs + MIPITX_ANALOG_CTRL7); //G205.12
	value &= ~(SP7350_MIPITX_MIPI_PHY_BNKSEL_MASK);
	value |= SP7350_MIPITX_MIPI_PHY_BNKSEL(lane_clock->txpll_bnksel);
	writel(value, dsi->regs + MIPITX_ANALOG_CTRL7); //G205.12
}

/* display MIPITX Pixel Clock and TXPLL Parameters Setting. */
static void sp7350_mipitx_dsi_pixel_clock_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode)
{
	u32 value;
	u32 reg_postdiv;
	struct sp7350_drm_mpitx_pixel_clock *pixel_clock = &dsi->pixel_clock;

	if (mode) {
		int fvco; /* KHz */
		/* MIPI Pixel Clock Formula:
		 *                   25M * prescal * fbkdiv
		 *   pixel_clock = -----------------------------
		 *                 prediv * postdiv * seldiv
		 *  Fvco = (25 * prescal * fbkdiv) / prediv
		 *  pixel_clock = Fvco / (postdiv * seldiv)
		 *  postdiv_10x = postdiv * 10
		 *==>
		 *  Fvco = pixel_clock * postdiv_10x * seldiv / 10
		 *  fbkdiv = Fvco * prediv / (25 * prescal)
		 *
		 * PreSetting-Rule:
		 *   prescal=1
		 *   pixel_clock = [  5,   8)MHz, prediv =2, postdiv_10x=125, seldiv=16
		 *   pixel_clock = [  8,  14)MHz, prediv =1, postdiv_10x=125, seldiv=16
		 *   pixel_clock = [ 14,  20)MHz, prediv =1, postdiv_10x=90,  seldiv=16
		 *   pixel_clock = [ 20,  29)MHz, prediv =1, postdiv_10x=125, seldiv=8
		 *   pixel_clock = [ 29,  40)MHz, prediv =1, postdiv_10x=90,  seldiv=8
		 *   pixel_clock = [ 40,  70)MHz, prediv =1, postdiv_10x=25,  seldiv=16
		 *   pixel_clock = [ 70, 112)MHz, prediv =1, postdiv_10x=125, seldiv=2
		 *   pixel_clock = [112, 160)MHz, prediv =1, postdiv_10x=90,  seldiv=2
		 *   pixel_clock = [160, 230)MHz, prediv =1, postdiv_10x=125, seldiv=1
		 *   pixel_clock = [230, 320)MHz, prediv =1, postdiv_10x=90,  seldiv=1
		 *   pixel_clock = [320, 540)MHz, prediv =1, postdiv_10x=25,  seldiv=2
		 *   pixel_clock = [540, 900)MHz, prediv =1, postdiv_10x=30,  seldiv=1
		 *   pixel_clock = [900,1200)MHz, prediv =1, postdiv_10x=25,  seldiv=1
		 *   Fvco = [1000, 1500]MHz, bnksel=0
		 *   Fvco = [1500, 2000]MHz, bnksel=1
		 *   Fvco = [2000, 2500]MHz, bnksel=2
		 *   Fvco = [2500, 3000]MHz, bnksel=3
		 */
		pixel_clock->prescal = 1;
		pixel_clock->clock   = mode->clock;
		if (pixel_clock->clock < 8000) {
			pixel_clock->prediv = 2;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 16;
		}
		else if (pixel_clock->clock < 14000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 16;
		}
		else if (pixel_clock->clock < 20000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 16;
		}
		else if (pixel_clock->clock < 29000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 8;
		}
		else if (pixel_clock->clock < 40000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 8;
		}
		else if (pixel_clock->clock < 70000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 25;
			pixel_clock->seldiv  = 16;
		}
		else if (pixel_clock->clock < 112000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 2;
		}
		else if (pixel_clock->clock < 160000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 2;
		}
		else if (pixel_clock->clock < 230000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 125;
			pixel_clock->seldiv  = 1;
		}
		else if (pixel_clock->clock < 320000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 90;
			pixel_clock->seldiv  = 1;
		}
		else if (pixel_clock->clock < 540000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 25;
			pixel_clock->seldiv  = 2;
		}
		else if (pixel_clock->clock < 900000) {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 30;
			pixel_clock->seldiv  = 1;
		}
		else {
			pixel_clock->prediv = 1;
			pixel_clock->postdiv_10x = 25;
			pixel_clock->seldiv  = 1;
		}
		fvco = pixel_clock->clock * pixel_clock->postdiv_10x * pixel_clock->seldiv / 10;
		if (fvco < 1500000) {
			pixel_clock->bnksel = 0;
		}
		else if (fvco < 2000000) {
			pixel_clock->bnksel = 1;
		}
		else if (fvco < 2500000) {
			pixel_clock->bnksel = 2;
		}
		else {
			pixel_clock->bnksel = 3;
		}
		pixel_clock->fbkdiv =  fvco * pixel_clock->prediv / (25000 * pixel_clock->prescal);
		if ((fvco * pixel_clock->prediv) % (25000 * pixel_clock->prescal)) {
			pixel_clock->fbkdiv += 1;
		}
		DRM_DEBUG_DRIVER("\nMIPITX Pixel Clock Info:\n"
							"   %dKHz(pixel:%dKHz), Fvco=%dKHz \n"
							"   prescal=%d, prediv=%d\n"
							"   postdiv_10x=%d, seldiv=%d\n"
							"   fbkdiv=%d, bnksel=%d\n",
			pixel_clock->clock, mode->clock, fvco,
			pixel_clock->prescal, pixel_clock->prediv,
			pixel_clock->postdiv_10x, pixel_clock->seldiv,
			pixel_clock->fbkdiv, pixel_clock->bnksel);
	}

	/* Register Setting Formula:
	 *   prescal= PRESCAL_H[15]+1 = {1, 2}
	 *   prediv = PREDIV_H[2:1]+1 = {1, 2}
	 *   postdiv= map{PSTDIV_H[6:3]} = {2.5, 3, 3.5, 4, 5, 5.5, 6, 7, 7.5, 8, 9, 10, 10.5, 11, 12, 12.5}
	 *   postdiv_10x = postdiv * 10
	 *   fbkdiv = FBKDIV_H[14:7] + 64 = [64, 127];
	 *   seldiv = MIPITX_SELDIV_H[11:7]+1 = {1, 2, 4, 8, 16}.
	 *   bnksel = BNKSEL_H[1:0] = [0, 3]
	 *==>
	 *   PRESCAL_H[15]  = prescal-1
	 *   PREDIV_H[2:1]  = prediv-1
	 *   FBKDIV_H[14:7] = fbkdiv-64
	 *   MIPITX_SELDIV_H[11:7] = seldiv-1
	 *   PSTDIV_H[6:3]   = (postdiv_10x-25)/5   for postdiv_10x<=40
	 *   PSTDIV_H[6:3]   = (postdiv_10x-50)/5+4 for postdiv_10x<=60
	 *   PSTDIV_H[6:3]   = (postdiv_10x-70)/5+7 for postdiv_10x<=80
	 *   PSTDIV_H[6:3]   = 10 for postdiv_10x=90
	 *   PSTDIV_H[6:3]   = (postdiv_10x-100)/5+11 for postdiv_10x<=110
	 *   PSTDIV_H[6:3]   = (postdiv_10x-120)/5+14 for postdiv_10x<=125
	 *   BNKSEL_H[1:0]   = bnksel
	 */
	if (pixel_clock->postdiv_10x == 90) {
		reg_postdiv = 10;
	}
	else if (pixel_clock->postdiv_10x <= 40) {
		reg_postdiv = (pixel_clock->postdiv_10x - 25) / 5;
	}
	else if (pixel_clock->postdiv_10x <= 60) {
		reg_postdiv = (pixel_clock->postdiv_10x - 50) / 5 + 4;
	}
	else if (pixel_clock->postdiv_10x <= 80) {
		reg_postdiv = (pixel_clock->postdiv_10x - 70) / 5 + 7;
	}
	else if (pixel_clock->postdiv_10x <= 110) {
		reg_postdiv = (pixel_clock->postdiv_10x - 100) / 5 + 11;
	}
	else {
		reg_postdiv = (pixel_clock->postdiv_10x - 120) / 5 + 14;
	}
	DRM_DEBUG_DRIVER("\nAO MOON3 PLLH Setting:\n"
						"   %dKHz\n"
						"   PRESCAL[15]=%d, PREDIV[2:1]=%d\n"
						"   POSTDIV[6:3]=%d, MIPITX_SELDIV_H[11:7]=%d\n"
						"   FBK_DIV[14:7]=%d, BNKSEL[1:0]=%d\n",
		pixel_clock->clock,
		pixel_clock->prescal - 1, pixel_clock->prediv - 1,
		reg_postdiv, pixel_clock->seldiv - 1,
		pixel_clock->fbkdiv-64, pixel_clock->bnksel);

	value = 0;
	/* Update PRESCAL_H[15] */
	value |= (0x80000000 | ((pixel_clock->prescal - 1) << 15));
	/* Update FBKDIV_H[14:7] */
	value |= (0x7f800000 | ((pixel_clock->fbkdiv - 64) << 7));
	/* Update PSTDIV_H[6:3] */
	value |= (0x00780000 | (reg_postdiv << 3));
	/* Update PREDIV_H[2:1] */
	value |= (0x00060000 | ((pixel_clock->prediv - 1) << 1));
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_14); //AO_G3.14

	value = 0;
	/* Update BNKSEL_H[1:0] */
	value = 0x00030000 | (pixel_clock->bnksel);
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_15); //AO_G3.15

	value = 0;
	/* Update MIPITX_SELDIV_H[11:7] */
	value |= (0x0f800000 | ((pixel_clock->seldiv - 1) << 7));
	writel(value, dsi->ao_moon3 + MIPITX_AO_MOON3_25); //AO_G3.25
}

/* mipitx dsi sync timing setting */
static void sp7350_mipitx_dsi_video_mode_setting(struct sp7350_drm_dsi *dsi, struct drm_display_mode *mode)
{
	u32 value;
	u32 data_bit;
	struct sp7350_drm_mpitx_sync_timing_param *sync_timing = &dsi->mipitx_sync_timing;

	if (mode) {
		sync_timing->hsa  = mode->hsync_end - mode->hsync_start;
		sync_timing->hbp  = mode->htotal - mode->hsync_end;
		sync_timing->hact = mode->hdisplay;
		sync_timing->vsa  = mode->vsync_end - mode->vsync_start;
		sync_timing->vfp  = mode->vsync_start - mode->vdisplay;
		sync_timing->vbp  = mode->vtotal - mode->vsync_end;
		sync_timing->vact = mode->vdisplay;
	}
	data_bit = dsi->divider * dsi->lanes;

	value = 0;
	value |= SP7350_MIPITX_HSA_SET(sync_timing->hsa) |
		SP7350_MIPITX_HBP_SET(sync_timing->hbp);
	writel(value, dsi->regs + MIPITX_VM_HT_CTRL);

	value = 0;
	value |= SP7350_MIPITX_VSA_SET(sync_timing->vsa) |
		SP7350_MIPITX_VFP_SET(sync_timing->vfp) |
		SP7350_MIPITX_VBP_SET(sync_timing->vbp);
	writel(value, dsi->regs + MIPITX_VM_VT0_CTRL);

	value = 0;
	value |= SP7350_MIPITX_VACT_SET(sync_timing->vact);
	writel(value, dsi->regs + MIPITX_VM_VT1_CTRL);

	//MIPITX  Video Mode WordCount Setting
	value = 0;
	value |= ((sync_timing->hact << 16) | ((sync_timing->hact * data_bit) / 8));
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

/* mipitx dsi lane timing setting */
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

