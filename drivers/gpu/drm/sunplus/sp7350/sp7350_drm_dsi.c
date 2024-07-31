// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM CRTCs and Encoder/Connecter
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 *         hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/reset.h>
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

#include "sp7350_drm_drv.h"
#include "sp7350_drm_crtc.h"
#include "sp7350_drm_dsi.h"
#include "sp7350_drm_regs.h"

#define DSI_PFORMAT_RGB565          0
#define DSI_PFORMAT_RGB666_PACKED   1
#define DSI_PFORMAT_RGB666          2
#define DSI_PFORMAT_RGB888          3

static const char * const sp7350_dsi_fmt[] = {
	"DSI_PFORMAT_RGB565", "DSI_PFORMAT_RGB666_PACKED",
	"DSI_PFORMAT_RGB666", "DSI_PFORMAT_RGB888",
};

#define MIPITX_CMD_FIFO_FULL   0x00000001
#define MIPITX_CMD_FIFO_EMPTY  0x00000010
#define MIPITX_DATA_FIFO_FULL  0x00000100
#define MIPITX_DATA_FIFO_EMPTY 0x00001000

static const struct debugfs_reg32 sp_dsi_host_g0_regs[] = {
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_00),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_01),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_02),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_03),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_04),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_05),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_06),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_07),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_08),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_09),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_10),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_11),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_12),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_13),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_14),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_15),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_16),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_17),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_18),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_19),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_20),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_21),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_22),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_23),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_24),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_25),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_26),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_27),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_28),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_29),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_30),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G0_G204_31),
};

static const struct debugfs_reg32 sp_dsi_host_g1_regs[] = {
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_00),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_01),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_02),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_03),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_04),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_05),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_06),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_07),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_08),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_09),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_10),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_11),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_12),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_13),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_14),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_15),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_16),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_17),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_18),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_19),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_20),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_21),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_22),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_23),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_24),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_25),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_26),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_27),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_28),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_29),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_30),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_G1_G205_31),
};

static const struct debugfs_reg32 sp_dsi_ao_moon3_regs[] = {
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_00),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_01),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_02),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_03),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_04),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_05),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_06),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_07),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_08),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_09),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_10),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_11),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_12),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_13),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_14),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_15),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_16),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_17),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_18),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_19),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_20),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_21),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_22),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_23),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_24),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_25),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_26),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_27),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_28),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_29),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_30),
	SP7350_DRM_REG32(SP7350_DISP_MIPITX_AO_MOON3_31),
};

/* General DSI hardware state. */
struct sp7350_dsi_host {
	struct mipi_dsi_host dsi_host;

	struct platform_device *pdev;
	struct drm_encoder *encoder;
	struct drm_bridge *bridge;
	struct list_head bridge_chain;

	void __iomem *regs;
	void __iomem *ao_moon3;

	/* clock */
	struct clk		*disp_clk[16];
	/* reset */
	struct reset_control	*disp_rstc[16];

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

	struct drm_display_mode adj_mode_store;

	/* define for debugfs */
	struct debugfs_regset32 regset_g204;
	struct debugfs_regset32 regset_g205;
	struct debugfs_regset32 regset_ao_moon3;
};

#define SP7350_DSI_HOST_READ(offset) readl(sp_dsi_host->regs + (offset))
#define SP7350_DSI_HOST_WRITE(offset, val) writel(val, sp_dsi_host->regs + (offset))

#define SP7350_DSI_TXPLL_READ(offset) readl(sp_dsi_host->regs + (offset))
#define SP7350_DSI_TXPLL_WRITE(offset, val) writel(val, sp_dsi_host->regs + (offset))

#define SP7350_DSI_PLLH_READ(offset) readl(sp_dsi_host->ao_moon3 + (offset))
#define SP7350_DSI_PLLH_WRITE(offset, val) writel(val, sp_dsi_host->ao_moon3 + (offset))

#define sp7350_host_to_dsi(host) \
	container_of(host, struct sp7350_dsi_host, dsi_host)

/* DSI encoder KMS struct */
struct sp7350_dsi_encoder {
	struct sp7350_encoder base;
	struct sp7350_dsi_host *sp_dsi_host;
};

#define to_sp7350_dsi_encoder(encoder) \
	container_of(encoder, struct sp7350_dsi_encoder, base.base)

/*
 * TODO: reference to sp7350_disp_mipitx.c,
 *   but should comes from panel driver parameters.
 * sp_mipitx_phy_pllclk_dsi[x][y]
 * y = 0-1, MIPITX width & height
 * y = 2-7, MIPITX PRESCALE & FBKDIV & PREDIV & POSTDIV & EN_DIV5 & BNKSEL(TXPLL)
 * y = 8-10, MIPITX PSTDIV & MIPITX_SEL & BNKSEL (PLLH)
 *
 * XTAL--[PREDIV]--------------------------->[EN_DIV5]--[POSTDIV]-->Fckout
 *                 |                       |
 *                 |<--FBKDIV<--PRESCALE<--|
 *
 *                25 * PRESCALE * FBKDIV
 *    Fckout = -----------------------------
 *              PREDIV * POSTDIV * 5^EN_DIV5
 */
static const u32 sp_mipitx_phy_pllclk_dsi[11][11] = {
	/* (w   h)   P1   P2    P3   P4   P5   P6   Q1   Q2   Q3 */
	{ 720,  480, 0x0, 0x1A, 0x0, 0x2, 0x0, 0x1, 0x4, 0xf, 0x0}, /* 480P */
	{ 720,  576, 0x0, 0x20, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0}, /* 576P */
	{1280,  720, 0x0, 0x24, 0x0, 0x1, 0x0, 0x1, 0x8, 0x3, 0x0}, /* 720P */
	{1920, 1080, 0x0, 0x24, 0x0, 0x0, 0x0, 0x1, 0x8, 0x1, 0x1}, /* 1080P */
	{ 480, 1280, 0x0, 0x0c, 0x0, 0x0, 0x0, 0x0, 0x5, 0x7, 0x0}, /* 480x1280 */
	{ 128,  128, 0x1, 0x1f, 0x1, 0x4, 0x1, 0x1, 0x0, 0x0, 0x0}, /* 128x128 */
	{  240, 320, 0x0, 0x0e, 0x0, 0x0, 0x0, 0x0, 0xa, 0xf, 0x2}, /* 240x320 */
	{3840,   64, 0x0, 0x1f, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0}, /* 3840x64 */
	{3840, 2880, 0x0, 0x3c, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0}, /* 3840x2880 */
	{ 800,  480, 0x1, 0x0d, 0x0, 0x0, 0x0, 0x1, 0xb, 0x7, 0x0}, /* 800x480 */
	{1024,  600, 0x1, 0x3d, 0x1, 0x1, 0x1, 0x3, 0x0, 0x0, 0x0}  /* 1024x600 */
};

static const u32 sp7350_mipitx_phy_timing[10] = {
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

static void sp7350_mipitx_phy_init(struct sp7350_dsi_host *sp_dsi_host)
{
	u32 value;

	DRM_DEBUG_DRIVER("lanes=%d flags=0x%08x format=%s\n",
		sp_dsi_host->lanes,
		sp_dsi_host->mode_flags,
		sp7350_dsi_fmt[sp_dsi_host->format]);

	//PHY Reset(under reset)
	value = SP7350_DSI_HOST_READ(MIPITX_ANALOG_CTRL2);
	value &= ~SP7350_MIPITX_NORMAL;
	SP7350_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL2, value);

	/*
	 * Setting T_HS-EXIT & T_LPX for Clock/Data Lane
	 */
	value |= (SP7350_MIPITX_T_HS_EXIT_SET(sp7350_mipitx_phy_timing[0]) |
			SP7350_MIPITX_T_LPX_SET(sp7350_mipitx_phy_timing[1]));
	SP7350_DSI_HOST_WRITE(MIPITX_LANE_TIME_CTRL, value);

	/*
	 * Setting T_CLK-PREPARE & T_CLK-ZERO for Clock Lane
	 */
	value = 0;
	value |= (SP7350_MIPITX_T_CLK_PREPARE_SET(sp7350_mipitx_phy_timing[2]) |
			SP7350_MIPITX_T_CLK_ZERO_SET(sp7350_mipitx_phy_timing[3]));
	SP7350_DSI_HOST_WRITE(MIPITX_CLK_TIME_CTRL0, value);

	/*
	 * Setting T_CLK-TRAIL & T_CLK-PRE & T_CLK-POST for Clock Lane
	 */
	value = 0;
	value |= (SP7350_MIPITX_T_CLK_TRAIL_SET(sp7350_mipitx_phy_timing[4]) |
			SP7350_MIPITX_T_CLK_PRE_SET(sp7350_mipitx_phy_timing[5]) |
			SP7350_MIPITX_T_CLK_POST_SET(sp7350_mipitx_phy_timing[6]));
	SP7350_DSI_HOST_WRITE(MIPITX_CLK_TIME_CTRL1, value);

	/*
	 * Enable HSA & HBP for Blanking Mode
	 */
	value = 0;
	value |= (SP7350_MIPITX_T_HS_TRAIL_SET(sp7350_mipitx_phy_timing[7]) |
			SP7350_MIPITX_T_HS_PREPARE_SET(sp7350_mipitx_phy_timing[8]) |
			SP7350_MIPITX_T_HS_ZERO_SET(sp7350_mipitx_phy_timing[9]));
	SP7350_DSI_HOST_WRITE(MIPITX_DATA_TIME_CTRL0, value);

	/*
	 * Enable HSA & HBP for Blanking Mode
	 */
	value = 0;
	value |= (SP7350_MIPITX_BLANK_POWER_HSA | SP7350_MIPITX_BLANK_POWER_HBP);
	SP7350_DSI_HOST_WRITE(MIPITX_BLANK_POWER_CTRL, value);

	value = 0;
	value |= (SP7350_MIPITX_CORE_CTRL_INPUT_EN |
			SP7350_MIPITX_CORE_CTRL_ANALOG_EN |
			SP7350_MIPITX_CORE_CTRL_DSI_EN);
	if ((sp_dsi_host->lanes == 1) || (sp_dsi_host->lanes == 2) ||
		(sp_dsi_host->lanes == 4)) {
		value |= SP7350_MIPITX_CORE_CTRL_LANE_NUM_SET(sp_dsi_host->lanes - 1);
		SP7350_DSI_HOST_WRITE(MIPITX_CORE_CTRL, value);
	} else
		pr_err("unsupported %d lanes\n", sp_dsi_host->lanes);

	value = 0x00000000;
	if (!(sp_dsi_host->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE))
		value |= SP7350_MIPITX_FORMAT_VTF_SET(SP7350_MIPITX_VTF_SYNC_EVENT);

	if (sp_dsi_host->format == DSI_PFORMAT_RGB565)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB565);
	else if (sp_dsi_host->format == DSI_PFORMAT_RGB666)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB666_18BITS);
	else if (sp_dsi_host->format == DSI_PFORMAT_RGB666_PACKED)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB666_24BITS);
	else if (sp_dsi_host->format == DSI_PFORMAT_RGB888)
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB888);
	else
		value |= SP7350_MIPITX_FORMAT_VPF_SET(SP7350_MIPITX_VPF_DSI_RGB888);
	SP7350_DSI_HOST_WRITE(MIPITX_FORMAT_CTRL, value);

	//PHY Reset(back to normal mode)
	value = SP7350_DSI_HOST_READ(MIPITX_ANALOG_CTRL2);
	value |= SP7350_MIPITX_NORMAL;
	SP7350_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL2, value);

}

static const u32 sp7350_pllh_pstdiv_int[] = {
	25, 30, 35, 40, 50, 55, 60, 70, 75, 80, 90, 100, 105, 110, 120, 125
};

static const u32 sp7350_pllh_mipitx_sel_int[] = {
	1,2,0,4,0,0,0,8,0,0,0,0,0,0,0,16,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static void sp7350_mipitx_pllclk_get(struct sp7350_dsi_host *sp_dsi_host)
{
	u32 tmp_value1, tmp_value2, tmp_value3;
	u32 value1, value2;

	value1 = SP7350_DSI_PLLH_READ(MIPITX_AO_MOON3_14);
	value2 = SP7350_DSI_PLLH_READ(MIPITX_AO_MOON3_25);

	tmp_value1 = 25 * ((FIELD_GET(GENMASK(15,15), value1)?2:1) *
		(FIELD_GET(GENMASK(14,7), value1) + 64)) /
		(FIELD_GET(GENMASK(2,1), value1)?2:1);
	tmp_value2 = (tmp_value1 *10 )/ ((sp7350_pllh_pstdiv_int[FIELD_GET(GENMASK(6,3), value1)]) *
		(sp7350_pllh_mipitx_sel_int[FIELD_GET(GENMASK(11,7), value2)]));
	tmp_value3 = (tmp_value1 *1000 )/ ((sp7350_pllh_pstdiv_int[FIELD_GET(GENMASK(6,3), value1)]) *
		(sp7350_pllh_mipitx_sel_int[FIELD_GET(GENMASK(11,7), value2)]));

	DRM_DEBUG_DRIVER("     PLLH FVCO %04d MHz , pix_clk %03d.%02d MHz\n",
		tmp_value1, tmp_value2, (tmp_value3 - tmp_value2*100));

}

static const char * const sp7350_txpll_prediv[] = {
	"DIV1", "DIV2", "DIV5", "DIV8",
};
static const u32 sp7350_txpll_prediv_int[] = {
	1,2,5,8
};

static const char * const sp7350_txpll_pstdiv[] = {
	"DIV1", "DIV2", "DIV4", "DIV8", "DIV16"
};
static const u32 sp7350_txpll_pstdiv_int[] = {
	1, 2, 4, 8, 16
};

static const char * const sp7350_txpll_endiv5[] = {
	"DIV1", "DIV5"
};
static const u32 sp7350_txpll_endiv5_int[] = {
	1, 5
};

static void sp7350_mipitx_txpll_get(struct sp7350_dsi_host *sp_dsi_host)
{
	u32 tmp_value1, tmp_value2, tmp_value3;
	u32 value1, value2;

	value1 = SP7350_DSI_TXPLL_READ(MIPITX_ANALOG_CTRL6);

	tmp_value1 = 25 * ((FIELD_GET(GENMASK(4,4), value1)?2:1) *
		(FIELD_GET(GENMASK(13,8), value1))) /
		(sp7350_txpll_prediv_int[FIELD_GET(GENMASK(1,0), value1)]);

	tmp_value2 = (tmp_value1)/ ((sp7350_txpll_pstdiv_int[FIELD_GET(GENMASK(18,16), value1)]) *
		(sp7350_txpll_endiv5_int[FIELD_GET(GENMASK(20,20), value1)]));
	tmp_value3 = (tmp_value1 *100 )/ ((sp7350_txpll_pstdiv_int[FIELD_GET(GENMASK(18,16), value1)]) *
		(sp7350_txpll_endiv5_int[FIELD_GET(GENMASK(20,20), value1)]));
	DRM_DEBUG_DRIVER("    TXPLL FVCO %04d MHz , bit_clk %03d.%02d MHz\n",
		tmp_value1, tmp_value2, (tmp_value3 - (tmp_value2*100)));

	tmp_value2 = (tmp_value1)/ ((sp7350_txpll_pstdiv_int[FIELD_GET(GENMASK(18,16), value1)]) *
		(sp7350_txpll_endiv5_int[FIELD_GET(GENMASK(20,20), value1)])*8);
	tmp_value3 = (tmp_value1 *100 )/ ((sp7350_txpll_pstdiv_int[FIELD_GET(GENMASK(18,16), value1)]) *
		(sp7350_txpll_endiv5_int[FIELD_GET(GENMASK(20,20), value1)])*8);
	DRM_DEBUG_DRIVER("    TXPLL ---- ---- --- , byteclk %03d.%02d MHz\n",
		tmp_value2, (tmp_value3 - (tmp_value2*100)));

	value2 = SP7350_DSI_TXPLL_READ(MIPITX_LP_CK);
	tmp_value2 = (tmp_value1)/ ((sp7350_txpll_pstdiv_int[FIELD_GET(GENMASK(18,16), value1)]) *
		(sp7350_txpll_endiv5_int[FIELD_GET(GENMASK(20,20), value1)])*8*(FIELD_GET(GENMASK(5,0), value2)+1));
	tmp_value3 = (tmp_value1 *100 )/ ((sp7350_txpll_pstdiv_int[FIELD_GET(GENMASK(18,16), value1)]) *
		(sp7350_txpll_endiv5_int[FIELD_GET(GENMASK(20,20), value1)])*8*(FIELD_GET(GENMASK(5,0), value2)+1));
	DRM_DEBUG_DRIVER("    TXPLL ---- ---- --- , LPCDclk %03d.%02d MHz\n",
		tmp_value2, (tmp_value3 - (tmp_value2*100)));

}

static void sp7350_mipitx_clock_init(struct sp7350_dsi_host *sp_dsi_host)
{
	DRM_DEBUG_DRIVER("sp7350_mipitx_clock_init\n");

	SP7350_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL9, 0x80000000); //init clock

	/*
	 * PLLH Fvco = 2150MHz (fixed)
	 *                             2150
	 * MIPITX pixel CLK = ----------------------- = 59.72MHz
	 *                     PST_DIV * MIPITX_SEL
	 */
	SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_15, 0xffff40be);
	SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_16, 0xffff0009);
	SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, 0xffff0b50); //PST_DIV = div9
	SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, 0x07800180); //MIPITX_SEL = div4

	/*
	 * TXPLL
	 * PRESCAL = 1, FBKDIV = 48, PRE_DIV = 1, EN_DIV5 = 0, PRE_DIV = 2, POST_DIV = 1
	 *                    25 * PRESCAL * FBKDIV            25 * 48
	 * MIPITX bit CLK = ------------------------------ = ----------- = 600MHz
	 *                   PRE_DIV * POST_DIV * 5^EN_DIV5       2
	 */
	SP7350_DSI_TXPLL_WRITE(MIPITX_ANALOG_CTRL5, 0x00000003); //enable and reset
	SP7350_DSI_TXPLL_WRITE(MIPITX_ANALOG_CTRL6, 0x00003001); //MIPITX CLK = 600MHz
	SP7350_DSI_TXPLL_WRITE(MIPITX_ANALOG_CTRL7, 0x00000140); //BNKSEL = 0x0 (320~640MHz)

	/*
	 *                      600
	 * MIPITX LP CLK = ------------ = 8.3MHz
	 *                   8 * div9
	 */
	SP7350_DSI_TXPLL_WRITE(MIPITX_LP_CK, 0x00000008); //(600/8/div9)=8.3MHz

	SP7350_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL9, 0x00000000); //init clock done

	/*
	 * check pll clock setting (debug only)
	 */
	sp7350_mipitx_pllclk_get(sp_dsi_host);
	sp7350_mipitx_txpll_get(sp_dsi_host);

}

static void sp7350_mipitx_clock_set(struct sp7350_dsi_host *sp_dsi_host,
	struct drm_display_mode *mode)
{
	int i, time_cnt = 0;
	u32 value;

	DRM_DEBUG_DRIVER("hdisplay %d vdisplay %d\n",
		mode->hdisplay, mode->vdisplay);

	for (i = 0; i < 11; i++) {
		if ((sp_mipitx_phy_pllclk_dsi[i][0] == mode->hdisplay) &&
		    (sp_mipitx_phy_pllclk_dsi[i][1] == mode->vdisplay)) {
			time_cnt = i;
			break;
		}
	}

	if ((mode->hdisplay == 240) && (mode->vdisplay == 320)) {
		value = 0;
		value |= 0x80000000;
		value |= 0x00780050;
		value |= (0x7f800000 | (0x14 << 7));
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, value);

		value = 0x07800780; //PLLH MIPITX CLK = 14.583MHz
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, value);
	} else if ((mode->hdisplay == 800) && (mode->vdisplay == 480)) {
		value = 0;
		value |= 0x00780058;
		value |= (0x7f800000 | (0x15 << 7));
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, value);

		value = 0x07800380; //PLLH MIPITX CLK = 26.563MHz
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, value);
	} else if ((mode->hdisplay == 720) && (mode->vdisplay == 480)) {
		value = 0;
		value |= 0x00780050;
		value |= (0x7f800000 | (0xe << 7));
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, value);

		value = 0x07800380; //PLLH MIPITX CLK = 27.08MHz
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, value);
	} else if ((mode->hdisplay == 1280) && (mode->vdisplay == 720)) {
		value = 0;
		value |= 0x00780038;
		value |= (0x7f800000 | (0x13 << 7));
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, value);

		value = 0x07800180; //PLLH MIPITX CLK = 74MHz
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, value);
	} else if ((mode->hdisplay == 1920) && (mode->vdisplay == 1080)) {
		value = 0;
		value |= 0x00780038;
		value |= (0x7f800000 | (0x13 << 7));
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, value);

		value = 0x07800080; //PLLH MIPITX CLK = 148MHz
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, value);
	} else {
		value = 0;
		value |= (0x00780000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][8] << 3));
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_14, value);

		value = 0;
		value |= (0x07800000 | (sp_mipitx_phy_pllclk_dsi[time_cnt][9] << 7));
		SP7350_DSI_PLLH_WRITE(MIPITX_AO_MOON3_25, value);
	}

	value = 0x00000000;
	value |= (SP7350_MIPITX_MIPI_PHY_EN_DIV5(sp_mipitx_phy_pllclk_dsi[time_cnt][6]) |
			SP7350_MIPITX_MIPI_PHY_POSTDIV(sp_mipitx_phy_pllclk_dsi[time_cnt][5]) |
			SP7350_MIPITX_MIPI_PHY_FBKDIV(sp_mipitx_phy_pllclk_dsi[time_cnt][3]) |
			SP7350_MIPITX_MIPI_PHY_PRESCALE(sp_mipitx_phy_pllclk_dsi[time_cnt][2]) |
			SP7350_MIPITX_MIPI_PHY_PREDIV(sp_mipitx_phy_pllclk_dsi[time_cnt][4]));
	SP7350_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL6, value);

	value = SP7350_DSI_HOST_READ(MIPITX_ANALOG_CTRL7);
	value &= ~(SP7350_MIPITX_MIPI_PHY_BNKSEL_MASK);
	value |= SP7350_MIPITX_MIPI_PHY_BNKSEL(sp_mipitx_phy_pllclk_dsi[time_cnt][7]);
	SP7350_DSI_HOST_WRITE(MIPITX_ANALOG_CTRL7, value);

	/*
	 * check pll clock setting (debug only)
	 */
	sp7350_mipitx_pllclk_get(sp_dsi_host);
	sp7350_mipitx_txpll_get(sp_dsi_host);
}

static void sp7350_mipitx_video_set(struct sp7350_dsi_host *sp_dsi_host,
	struct drm_display_mode *mode)
{
	u32 width, height, data_bit;
	u32 value;

	width = mode->hdisplay;
	height = mode->vdisplay;
	data_bit = sp_dsi_host->divider * sp_dsi_host->lanes;

	DRM_DEBUG_DRIVER("hdisplay %d vdisplay %d\n",
		mode->hdisplay, mode->vdisplay);

	value = 0;
	value |= SP7350_MIPITX_HSA_SET(mode->hsync_end - mode->hsync_start) |
		SP7350_MIPITX_HFP_SET(mode->hsync_start - mode->hdisplay) |
		SP7350_MIPITX_HBP_SET(mode->htotal - mode->hsync_end);
	SP7350_DSI_HOST_WRITE(MIPITX_VM_HT_CTRL, value);

	value = 0;
	value |= SP7350_MIPITX_VSA_SET(mode->vsync_end - mode->vsync_start) |
		SP7350_MIPITX_VFP_SET(mode->vsync_start - mode->vdisplay) |
		SP7350_MIPITX_VBP_SET(mode->vtotal - mode->vsync_end);
	SP7350_DSI_HOST_WRITE(MIPITX_VM_VT0_CTRL, value);

	value = 0;
	value |= SP7350_MIPITX_VACT_SET(mode->vdisplay);
	SP7350_DSI_HOST_WRITE(MIPITX_VM_VT1_CTRL, value);

	value = 0;
	value |= ((width << 16) | ((width * data_bit) / 8));
	SP7350_DSI_HOST_WRITE(MIPITX_WORD_CNT, value);
}

static void sp7350_mipitx_dsi_cmd_mode_start(struct sp7350_dsi_host *sp_dsi_host)
{
	u32 value;

	DRM_DEBUG_DRIVER("%s\n", __func__);

	value = 0;
	value |= SP7350_MIPITX_OP_CTRL_TXLDPT;
	SP7350_DSI_HOST_WRITE(MIPITX_OP_CTRL, value);

	value = 0;
	value |= (SP7350_MIPITX_CORE_CTRL_INPUT_EN |
			SP7350_MIPITX_CORE_CTRL_ANALOG_EN |
			SP7350_MIPITX_CORE_CTRL_CMD_TRANS_TIME |
			SP7350_MIPITX_CORE_CTRL_DSI_EN);
	if ((sp_dsi_host->lanes == 1) || (sp_dsi_host->lanes == 2) ||
		(sp_dsi_host->lanes == 4)) {
		value |= SP7350_MIPITX_CORE_CTRL_LANE_NUM_SET(sp_dsi_host->lanes - 1);
		SP7350_DSI_HOST_WRITE(MIPITX_CORE_CTRL, value);
	} else
		pr_err("unsupported %d lanes\n", sp_dsi_host->lanes);

	SP7350_DSI_HOST_WRITE(MIPITX_BTA_CTRL, 0x00520004);
	SP7350_DSI_HOST_WRITE(MIPITX_ULPS_DELAY, 0x00000aff);
}

static void sp7350_mipitx_lane_timing_init(struct sp7350_dsi_host *sp_dsi_host)
{
	u32 value = 0;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	/*
	 * Enable clock lane at High Speed Mode
	 */
	value = 0;
	value |= SP7350_MIPITX_CLK_CTRL_CKHS_EN;
	SP7350_DSI_HOST_WRITE(MIPITX_CLK_CTRL, value);
}

static void sp7350_mipitx_dsi_video_mode_on(struct sp7350_dsi_host *sp_dsi_host)
{
	u32 value;

	DRM_DEBUG_DRIVER("%s\n", __func__);

	value = 0;
	value |= (SP7350_MIPITX_CORE_CTRL_INPUT_EN |
			SP7350_MIPITX_CORE_CTRL_ANALOG_EN |
			SP7350_MIPITX_CORE_CTRL_DSI_EN);

	if ((sp_dsi_host->lanes == 1) || (sp_dsi_host->lanes == 2) ||
		(sp_dsi_host->lanes == 4)) {
		value |= SP7350_MIPITX_CORE_CTRL_LANE_NUM_SET(sp_dsi_host->lanes - 1);
		SP7350_DSI_HOST_WRITE(MIPITX_CORE_CTRL, value);
	} else
		pr_err("unsupported %d lanes\n", sp_dsi_host->lanes);

}

static void check_dsi_cmd_fifo_full(struct sp7350_dsi_host *sp_dsi_host)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = SP7350_DSI_HOST_READ(MIPITX_CMD_FIFO);
	while ((value & MIPITX_CMD_FIFO_FULL) == MIPITX_CMD_FIFO_FULL) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("cmd fifo full timeout\n");
			break;
		}
		value = SP7350_DSI_HOST_READ(MIPITX_CMD_FIFO);
		++mipitx_fifo_timeout;
		udelay(100);
	}
}

static void check_dsi_data_fifo_full(struct sp7350_dsi_host *sp_dsi_host)
{
	int mipitx_fifo_timeout = 0;
	u32 value = 0;

	value = SP7350_DSI_HOST_READ(MIPITX_CMD_FIFO);
	while ((value & MIPITX_DATA_FIFO_FULL) == MIPITX_DATA_FIFO_FULL) {
		if (mipitx_fifo_timeout > 10000) { //over 1 second
			pr_info("data fifo full timeout\n");
			break;
		}
		value = SP7350_DSI_HOST_READ(MIPITX_CMD_FIFO);
		++mipitx_fifo_timeout;
		udelay(100);
	}
}


static enum drm_mode_status _sp7350_dsi_encoder_phy_mode_valid(
					struct drm_encoder *encoder,
					const struct drm_display_mode *mode)
{
	/* TODO reference to dsi_encoder_phy_mode_valid */
	DRM_DEBUG_DRIVER("%s [TODO]\n", __func__);

	return MODE_OK;
}

static enum drm_mode_status sp7350_dsi_encoder_mode_valid(struct drm_encoder *encoder,
							  const struct drm_display_mode *mode)

{
	const struct drm_crtc_helper_funcs *crtc_funcs = NULL;
	struct drm_crtc *crtc = NULL;
	struct drm_display_mode adj_mode;
	enum drm_mode_status ret;

	DRM_DEBUG_DRIVER("%s\n", __func__);

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
	struct sp7350_dsi_encoder *sp_dsi_encoder = to_sp7350_dsi_encoder(encoder);
	struct sp7350_dsi_host *sp_dsi_host = sp_dsi_encoder->sp_dsi_host;

	/* TODO reference to dsi_encoder_mode_set */
	DRM_DEBUG_DRIVER("[TODO]\n");
	DRM_DEBUG_DRIVER("SET mode[%dx%x], adj_mode[%dx%d]\n",
			 mode->hdisplay, mode->vdisplay,
		adj_mode->hdisplay, adj_mode->vdisplay);

	sp7350_mipitx_clock_set(sp_dsi_host, adj_mode);
	sp7350_mipitx_video_set(sp_dsi_host, adj_mode);
	/* store */
	drm_mode_copy(&sp_dsi_host->adj_mode_store, adj_mode);
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
	struct sp7350_dsi_encoder *sp_dsi_encoder = to_sp7350_dsi_encoder(encoder);
	struct sp7350_dsi_host *sp_dsi_host = sp_dsi_encoder->sp_dsi_host;
	struct drm_bridge *iter;

	DRM_DEBUG_DRIVER("%s\n", encoder->name);

	list_for_each_entry_reverse(iter, &sp_dsi_host->bridge_chain, chain_node) {
		if (iter->funcs->disable)
			iter->funcs->disable(iter);

		if (iter == sp_dsi_host->bridge)
			break;
	}

	list_for_each_entry_from(iter, &sp_dsi_host->bridge_chain, chain_node) {
		if (iter->funcs->post_disable)
			iter->funcs->post_disable(iter);
	}
}

static void sp7350_dsi_encoder_enable(struct drm_encoder *encoder)
{
	struct sp7350_dsi_encoder *sp_dsi_encoder = to_sp7350_dsi_encoder(encoder);
	struct sp7350_dsi_host *sp_dsi_host = sp_dsi_encoder->sp_dsi_host;
	struct drm_bridge *iter;

	DRM_DEBUG_DRIVER("%s\n", encoder->name);

	sp7350_mipitx_dsi_cmd_mode_start(sp_dsi_host);

	list_for_each_entry_reverse(iter, &sp_dsi_host->bridge_chain, chain_node) {
		if (iter->funcs->pre_enable)
			iter->funcs->pre_enable(iter);
	}

	sp7350_mipitx_dsi_video_mode_on(sp_dsi_host);

	list_for_each_entry_reverse(iter, &sp_dsi_host->bridge_chain, chain_node) {
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
	struct sp7350_dsi_host *sp_dsi_host = sp7350_host_to_dsi(host);
	u32 value, data_cnt;
	u8 *data1;
	int i;

	DRM_DEBUG_DRIVER("len %ld\n", msg->tx_len);

	data1 = (u8 *)msg->tx_buf;

	udelay(100);
	if (msg->tx_len == 0) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000003;
		SP7350_DSI_HOST_WRITE(MIPITX_SPKT_HEAD, value);
	} else if (msg->tx_len == 1) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000013 | (data1[0] << 8);
		SP7350_DSI_HOST_WRITE(MIPITX_SPKT_HEAD, value);
	} else if (msg->tx_len == 2) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000023 | (data1[0] << 8) | (data1[1] << 16);
		SP7350_DSI_HOST_WRITE(MIPITX_SPKT_HEAD, value);
	} else if ((msg->tx_len >= 3) && (msg->tx_len <= 64)) {
		check_dsi_cmd_fifo_full(sp_dsi_host);
		value = 0x00000029 | ((u32)msg->tx_len << 8);
		SP7350_DSI_HOST_WRITE(MIPITX_LPKT_HEAD, value);

		if (msg->tx_len % 4)
			data_cnt = ((u32)msg->tx_len / 4) + 1;
		else
			data_cnt = ((u32)msg->tx_len / 4);

		for (i = 0; i < data_cnt; i++) {
			check_dsi_data_fifo_full(sp_dsi_host);
			value = 0x00000000;
			if (i * 4 + 0 < msg->tx_len)
				value |= (data1[i * 4 + 0] << 0);
			if (i * 4 + 1 < msg->tx_len)
				value |= (data1[i * 4 + 1] << 8);
			if (i * 4 + 2 < msg->tx_len)
				value |= (data1[i * 4 + 2] << 16);
			if (i * 4 + 3 < msg->tx_len)
				value |= (data1[i * 4 + 3] << 24);
			SP7350_DSI_HOST_WRITE(MIPITX_LPKT_PAYLOAD, value);
		}
	} else {
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev, "data length over %ld\n", msg->tx_len);
		return -1;
	}

	return 0;
}

static int sp7350_dsi_host_attach(struct mipi_dsi_host *host,
				  struct mipi_dsi_device *device)
{
	struct sp7350_dsi_host *sp_dsi_host = sp7350_host_to_dsi(host);

	DRM_DEBUG_DRIVER("%s\n", __func__);
	if (!sp_dsi_host->regs || !sp_dsi_host->ao_moon3) {
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev, "dsi host probe fail!.\n");
		return -1;
	}

	DRM_DEBUG_DRIVER("channel %d lanes=%d flags=0x%08lx format=%s\n",
		device->channel,
		device->lanes,
		device->mode_flags,
		sp7350_dsi_fmt[device->format]);

	sp_dsi_host->lanes = device->lanes;
	sp_dsi_host->channel = device->channel;
	sp_dsi_host->mode_flags = device->mode_flags;

	switch (device->format) {
	case MIPI_DSI_FMT_RGB888:
		sp_dsi_host->format = DSI_PFORMAT_RGB888;
		sp_dsi_host->divider = 24 / sp_dsi_host->lanes;
		break;
	case MIPI_DSI_FMT_RGB666:
		sp_dsi_host->format = DSI_PFORMAT_RGB666;
		sp_dsi_host->divider = 24 / sp_dsi_host->lanes;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		sp_dsi_host->format = DSI_PFORMAT_RGB666_PACKED;
		sp_dsi_host->divider = 18 / sp_dsi_host->lanes;
		break;
	case MIPI_DSI_FMT_RGB565:
		sp_dsi_host->format = DSI_PFORMAT_RGB565;
		sp_dsi_host->divider = 16 / sp_dsi_host->lanes;
		break;
	default:
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev, "Unknown DSI format: %d.\n",
			      sp_dsi_host->format);
		return -1;
	}

	if (!(sp_dsi_host->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		DRM_DEV_ERROR(&sp_dsi_host->pdev->dev,
			      "Only VIDEO mode panels supported currently.\n");
		return -1;
	}

	sp7350_mipitx_phy_init(sp_dsi_host);
	sp7350_mipitx_clock_init(sp_dsi_host);
	sp7350_mipitx_lane_timing_init(sp_dsi_host);

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

static int sp7350_encoder_init(struct device *dev,
				   struct drm_device *drm_dev,
				   struct drm_encoder *encoder)
{
	u32 crtc_mask = drm_of_find_possible_crtcs(drm_dev, dev->of_node);
	int ret;

	DRM_DEV_DEBUG_DRIVER(dev, "%s\n", __func__);

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
	struct sp7350_dsi_host *sp_dsi_host = dev_get_drvdata(dev);
	struct sp7350_dsi_encoder *sp_dsi_encoder;
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

	sp_dsi_host->port = 0;

	sp_dsi_encoder = devm_kzalloc(dev, sizeof(*sp_dsi_encoder),
					  GFP_KERNEL);
	if (!sp_dsi_encoder)
		return -ENOMEM;

	INIT_LIST_HEAD(&sp_dsi_host->bridge_chain);
	sp_dsi_encoder->base.type = SP7350_DRM_ENCODER_TYPE_DSI0;
	sp_dsi_encoder->sp_dsi_host = sp_dsi_host;
	sp_dsi_host->encoder = &sp_dsi_encoder->base.base;

	sp_dsi_host->regset_g204.base = sp_dsi_host->regs + (SP7350_REG_OFFSET_MIPITX_G204 << 7);
	sp_dsi_host->regset_g204.regs = sp_dsi_host_g0_regs;
	sp_dsi_host->regset_g204.nregs = ARRAY_SIZE(sp_dsi_host_g0_regs);

	sp_dsi_host->regset_g205.base = sp_dsi_host->regs + (SP7350_REG_OFFSET_MIPITX_G205 << 7);;
	sp_dsi_host->regset_g205.regs = sp_dsi_host_g1_regs;
	sp_dsi_host->regset_g205.nregs = ARRAY_SIZE(sp_dsi_host_g1_regs);

	sp_dsi_host->regset_ao_moon3.base = sp_dsi_host->ao_moon3;
	sp_dsi_host->regset_ao_moon3.regs = sp_dsi_ao_moon3_regs;
	sp_dsi_host->regset_ao_moon3.nregs = ARRAY_SIZE(sp_dsi_ao_moon3_regs);

	init_completion(&sp_dsi_host->xfer_completion);

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
						  &panel, &sp_dsi_host->bridge);
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

	DRM_DEV_DEBUG_DRIVER(dev, "devm_drm_panel_bridge_add_typed\n");
	if (panel) {
		sp_dsi_host->bridge = devm_drm_panel_bridge_add_typed(dev, panel,
							      DRM_MODE_CONNECTOR_DSI);
		if (IS_ERR(sp_dsi_host->bridge))
			return PTR_ERR(sp_dsi_host->bridge);
	}

	DRM_DEV_DEBUG_DRIVER(dev, "sp7350_encoder_init\n");
	ret = sp7350_encoder_init(dev, drm, sp_dsi_host->encoder);
	if (ret)
		return ret;

	ret = drm_bridge_attach(sp_dsi_host->encoder, sp_dsi_host->bridge, NULL, 0);
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
	list_splice_init(&sp_dsi_host->encoder->bridge_chain, &sp_dsi_host->bridge_chain);

	//sp7350_debugfs_add_regset32(drm, dsi->variant->debugfs_name, &dsi->regset);
	sp7350_debugfs_add_regset32(drm, "regs_g204", &sp_dsi_host->regset_g204);
	sp7350_debugfs_add_regset32(drm, "regs_g205", &sp_dsi_host->regset_g205);
	sp7350_debugfs_add_regset32(drm, "regs_ao_moon3", &sp_dsi_host->regset_ao_moon3);

	pm_runtime_enable(dev);

	DRM_DEV_DEBUG_DRIVER(dev, "finish.\n");

	return 0;
}

static void sp7350_dsi_unbind(struct device *dev, struct device *master,
			      void *data)
{
	struct sp7350_dsi_host *sp_dsi_host = dev_get_drvdata(dev);

	DRM_DEV_DEBUG_DRIVER(dev, "%s\n", __func__);

	if (sp_dsi_host->bridge)
		pm_runtime_disable(dev);

	/*
	 * Restore the bridge_chain so the bridge detach procedure can happen
	 * normally.
	 */
	list_splice_init(&sp_dsi_host->bridge_chain, &sp_dsi_host->encoder->bridge_chain);
	drm_encoder_cleanup(sp_dsi_host->encoder);
}

static const struct component_ops sp7350_dsi_ops = {
	.bind   = sp7350_dsi_bind,
	.unbind = sp7350_dsi_unbind,
};

static const char * const sp7350_disp_clkc[] = {
	"clkc_dispsys", "clkc_dmix",  "clkc_tgen", "clkc_tcon", "clkc_mipitx",
	"clkc_gpost0", "clkc_gpost1", "clkc_gpost2", "clkc_gpost3",
	"clkc_osd0", "clkc_osd1", "clkc_osd2", "clkc_osd3",
	"clkc_imgread0", "clkc_vscl0", "clkc_vpost0"
};

static const char * const sp7350_disp_rtsc[] = {
	"rstc_dispsys", "rstc_dmix", "rstc_tgen", "rstc_tcon", "rstc_mipitx",
	"rstc_gpost0", "rstc_gpost1", "rstc_gpost2", "rstc_gpost3",
	"rstc_osd0", "rstc_osd1", "rstc_osd2", "rstc_osd3",
	"rstc_imgread0", "rstc_vscl0", "rstc_vpost0"
};

static int sp7350_dsi_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp7350_dsi_host *sp_dsi_host;
	int ret, i;

	DRM_DEV_DEBUG_DRIVER(dev, "start\n");

	sp_dsi_host = devm_kzalloc(dev, sizeof(*sp_dsi_host), GFP_KERNEL);
	if (!sp_dsi_host)
		return -ENOMEM;
	dev_set_drvdata(dev, sp_dsi_host);

	sp_dsi_host->pdev = pdev;

	/*
	 * init clk & reset
	 */
	DRM_DEV_DEBUG_DRIVER(dev, "init clken & reset\n");
	for (i = 0; i < 16; i++) {
		sp_dsi_host->disp_clk[i] = devm_clk_get(dev, sp7350_disp_clkc[i]);
		if (IS_ERR(sp_dsi_host->disp_clk[i]))
			return PTR_ERR(sp_dsi_host->disp_clk[i]);

		sp_dsi_host->disp_rstc[i] = devm_reset_control_get_exclusive(dev, sp7350_disp_rtsc[i]);
		if (IS_ERR(sp_dsi_host->disp_rstc[i]))
			return dev_err_probe(dev, PTR_ERR(sp_dsi_host->disp_rstc[i]), "err get reset\n");

		ret = reset_control_deassert(sp_dsi_host->disp_rstc[i]);
		if (ret)
			return dev_err_probe(dev, ret, "failed to deassert reset\n");

		ret = clk_prepare_enable(sp_dsi_host->disp_clk[i]);
		if (ret)
			return ret;
	}

	/*
	 * get disp reg base (G204 - G205)
	 */
	DRM_DEV_DEBUG_DRIVER(dev, "init mipitx regs\n");
	sp_dsi_host->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sp_dsi_host->regs))
		return dev_err_probe(&pdev->dev, PTR_ERR(sp_dsi_host->regs), "dsi reg not found\n");
	/*
	 * get pllh reg base (G03_AO)
	 */
	DRM_DEV_DEBUG_DRIVER(dev, "init pllh regs\n");
	sp_dsi_host->ao_moon3 = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(sp_dsi_host->ao_moon3))
		return dev_err_probe(&pdev->dev, PTR_ERR(sp_dsi_host->ao_moon3), "dsi reg ao_moon3 not found\n");

	/* Note, the initialization sequence for DSI and panels is
	 * tricky.  The component bind above won't get past its
	 * -EPROBE_DEFER until the panel/bridge probes.  The
	 * panel/bridge will return -EPROBE_DEFER until it has a
	 * mipi_dsi_host to register its device to.  So, we register
	 * the host during pdev probe time, so sp7350_drm as a whole can then
	 * -EPROBE_DEFER its component bind process until the panel
	 * successfully attaches.
	 */
	sp_dsi_host->dsi_host.ops = &sp7350_dsi_host_ops;
	sp_dsi_host->dsi_host.dev = dev;
	mipi_dsi_host_register(&sp_dsi_host->dsi_host);

	ret = component_add(&pdev->dev, &sp7350_dsi_ops);
	if (ret) {
		mipi_dsi_host_unregister(&sp_dsi_host->dsi_host);
		return ret;
	}

	DRM_DEV_DEBUG_DRIVER(dev, "finish\n");

	return ret;
}

static int sp7350_dsi_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp7350_dsi_host *sp_dsi_host = dev_get_drvdata(dev);

	DRM_DEV_DEBUG_DRIVER(dev, "dsi driver remove.\n");

	if (sp_dsi_host->encoder)
		sp7350_dsi_encoder_disable(sp_dsi_host->encoder);

	component_del(&pdev->dev, &sp7350_dsi_ops);
	mipi_dsi_host_unregister(&sp_dsi_host->dsi_host);

	return 0;
}

static int sp7350_dsi_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sp7350_dsi_host *sp_dsi_host = dev_get_drvdata(&pdev->dev);

	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "dsi driver suspend.\n");

	if (sp_dsi_host->bridge)
		pm_runtime_put(&pdev->dev);

	/*
	 * phy mipitx registers store...
	 */

	/*
	 * TODO
	 * phy power off, disable clock, disable irq...
	 */

	if (sp_dsi_host->encoder)
		sp7350_dsi_encoder_disable(sp_dsi_host->encoder);

	return 0;
}

static int sp7350_dsi_dev_resume(struct platform_device *pdev)
{
	struct sp7350_dsi_host *sp_dsi_host = dev_get_drvdata(&pdev->dev);

	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "dsi driver resume.\n");

	if (sp_dsi_host->bridge)
		pm_runtime_get(&pdev->dev);

	/*
	 * TODO
	 * phy power on, enable clock, enable irq...
	 */

	/*
	 * phy mipitx restore...
	 */
	sp7350_mipitx_phy_init(sp_dsi_host);
	sp7350_mipitx_clock_init(sp_dsi_host);
	sp7350_mipitx_lane_timing_init(sp_dsi_host);
	sp7350_mipitx_clock_set(sp_dsi_host, &sp_dsi_host->adj_mode_store);
	sp7350_mipitx_video_set(sp_dsi_host, &sp_dsi_host->adj_mode_store);

	if (sp_dsi_host->encoder)
		sp7350_dsi_encoder_enable(sp_dsi_host->encoder);

	return 0;
}

struct platform_driver sp7350_dsi_driver = {
	.probe   = sp7350_dsi_dev_probe,
	.remove  = sp7350_dsi_dev_remove,
	.suspend = sp7350_dsi_dev_suspend,
	.resume  = sp7350_dsi_dev_resume,
	.driver  = {
		.name = "sp7350_dsi_host",
		.of_match_table = sp7350_dsi_dt_match,
	},
};
