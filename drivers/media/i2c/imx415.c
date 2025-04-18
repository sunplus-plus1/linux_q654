// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for the Sony IMX415 CMOS Image Sensor.
 *
 * Copyright (C) 2023 WolfVision GmbH.
 */

#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/videodev2.h>

#include <media/v4l2-cci.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>

#define CROP_START(SRC, DST) (((SRC) - (DST)) / 2 / 4 * 4)
#define DST_WIDTH_3840 3840
#define DST_HEIGHT_2160 2160
#define DST_WIDTH_1920 1920
#define DST_HEIGHT_1080 1080

#define IMX415_PIXEL_ARRAY_TOP	  0
#define IMX415_PIXEL_ARRAY_LEFT	  0
#define IMX415_PIXEL_ARRAY_WIDTH  3864
#define IMX415_PIXEL_ARRAY_HEIGHT 2192
#define IMX415_PIXEL_ARRAY_VBLANK 58

#define IMX415_NUM_CLK_PARAM_REGS 11

#define IMX415_REG_8BIT(n)	  ((1 << 16) | (n))
#define IMX415_REG_16BIT(n)	  ((2 << 16) | (n))
#define IMX415_REG_24BIT(n)	  ((3 << 16) | (n))
#define IMX415_REG_SIZE_SHIFT	  16
#define IMX415_REG_ADDR_MASK	  0xffff
#define IMX415_MODE		  CCI_REG8(0x3000)
#define IMX415_MODE_OPERATING	  (0)
#define IMX415_MODE_STANDBY	  BIT(0)
#define IMX415_REGHOLD		  CCI_REG8(0x3001)
#define IMX415_REGHOLD_INVALID	  (0)
#define IMX415_REGHOLD_VALID	  BIT(0)
#define IMX415_XMSTA		  CCI_REG8(0x3002)
#define IMX415_XMSTA_START	  (0)
#define IMX415_XMSTA_STOP	  BIT(0)
#define IMX415_BCWAIT_TIME	  CCI_REG16_LE(0x3008)
#define IMX415_CPWAIT_TIME	  CCI_REG16_LE(0x300a)
#define IMX415_WINMODE		  CCI_REG8(0x301c)
#define IMX415_ADDMODE		  CCI_REG8(0x3022)
#define IMX415_REVERSE		  CCI_REG8(0x3030)
#define IMX415_HREVERSE_SHIFT	  (0)
#define IMX415_VREVERSE_SHIFT	  BIT(0)
#define IMX415_ADBIT		  CCI_REG8(0x3031)
#define IMX415_MDBIT		  CCI_REG8(0x3032)
#define IMX415_SYS_MODE		  CCI_REG8(0x3033)
#define IMX415_OUTSEL		  CCI_REG8(0x30c0)
#define IMX415_DRV		  CCI_REG8(0x30c1)
#define IMX415_VMAX		  CCI_REG24_LE(0x3024)
#define IMX415_HMAX		  CCI_REG16_LE(0x3028)
#define IMX415_PIX_HST	CCI_REG16_LE(0x3040)
#define IMX415_PIX_HWIDTH	CCI_REG16_LE(0x3042)
#define IMX415_PIX_VST	CCI_REG16_LE(0x3044)
#define IMX415_PIX_VWIDTH	CCI_REG16_LE(0x3046)
#define IMX415_SHR0		  CCI_REG24_LE(0x3050)
#define IMX415_GAIN_PCG_0	  CCI_REG16_LE(0x3090)
#define IMX415_AGAIN_MIN	  0
#define IMX415_AGAIN_DEFAULT  70
#define IMX415_AGAIN_MAX	  100
#define IMX415_AGAIN_STEP	  1
#define IMX415_BLKLEVEL		  CCI_REG16_LE(0x30e2)
#define IMX415_BLKLEVEL_DEFAULT	  50
#define IMX415_TPG_EN_DUOUT	  CCI_REG8(0x30e4)
#define IMX415_TPG_PATSEL_DUOUT	  CCI_REG8(0x30e6)
#define IMX415_TPG_COLORWIDTH	  CCI_REG8(0x30e8)
#define IMX415_TESTCLKEN_MIPI	  CCI_REG8(0x3110)
#define IMX415_INCKSEL1		  CCI_REG8(0x3115)
#define IMX415_INCKSEL2		  CCI_REG8(0x3116)
#define IMX415_INCKSEL3		  CCI_REG16_LE(0x3118)
#define IMX415_INCKSEL4		  CCI_REG16_LE(0x311a)
#define IMX415_INCKSEL5		  CCI_REG8(0x311e)
#define IMX415_DIG_CLP_MODE	  CCI_REG8(0x32c8)
#define IMX415_WRJ_OPEN		  CCI_REG8(0x3390)
#define IMX415_SENSOR_INFO	  CCI_REG16_LE(0x3f12)
#define IMX415_SENSOR_INFO_MASK	  0xfff
#define IMX415_CHIP_ID		  0x514
#define IMX415_LANEMODE		  CCI_REG16_LE(0x4001)
#define IMX415_LANEMODE_2	  1
#define IMX415_LANEMODE_4	  3
#define IMX415_TXCLKESC_FREQ	  CCI_REG16_LE(0x4004)
#define IMX415_INCKSEL6		  CCI_REG8(0x400c)
#define IMX415_TCLKPOST		  CCI_REG16_LE(0x4018)
#define IMX415_TCLKPREPARE	  CCI_REG16_LE(0x401a)
#define IMX415_TCLKTRAIL	  CCI_REG16_LE(0x401c)
#define IMX415_TCLKZERO		  CCI_REG16_LE(0x401e)
#define IMX415_THSPREPARE	  CCI_REG16_LE(0x4020)
#define IMX415_THSZERO		  CCI_REG16_LE(0x4022)
#define IMX415_THSTRAIL		  CCI_REG16_LE(0x4024)
#define IMX415_THSEXIT		  CCI_REG16_LE(0x4026)
#define IMX415_TLPX		  CCI_REG16_LE(0x4028)
#define IMX415_INCKSEL7		  CCI_REG8(0x4074)

static const char *const imx415_supply_names[] = {
	"dvdd",
	"ovdd",
	"avdd",
};

/*
 * The IMX415 data sheet uses lane rates but v4l2 uses link frequency to
 * describe MIPI CSI-2 speed. This driver uses lane rates wherever possible
 * and converts them to link frequencies by a factor of two when needed.
 */
static const s64 link_freq_menu_items[] = {
	594000000 / 2,	720000000 / 2,	891000000 / 2,
	1440000000 / 2, 1485000000 / 2, 1782000000 / 2,
};

struct imx415_clk_params {
	u64 lane_rate;
	u64 inck;
	struct cci_reg_sequence regs[IMX415_NUM_CLK_PARAM_REGS];
};

/* INCK Settings - includes all lane rate and INCK dependent registers */
static const struct imx415_clk_params imx415_clk_params[] = {
	{
		.lane_rate = 594000000UL,
		.inck = 27000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x05D },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x042 },
		.regs[2] = { IMX415_SYS_MODE, 0x7 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x084 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E7 },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x06C0 },
	},
	{
		.lane_rate = 594000000UL,
		.inck = 37125000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x07F },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x05B },
		.regs[2] = { IMX415_SYS_MODE, 0x7 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x24 },
		.regs[5] = { IMX415_INCKSEL3, 0x080 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x24 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0984 },
	},
	{
		.lane_rate = 594000000UL,
		.inck = 74250000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0FF },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B6 },
		.regs[2] = { IMX415_SYS_MODE, 0x7 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x080 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1290 },
	},
	{
		.lane_rate = 720000000UL,
		.inck = 24000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x054 },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x03B },
		.regs[2] = { IMX415_SYS_MODE, 0x9 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x0B4 },
		.regs[6] = { IMX415_INCKSEL4, 0x0FC },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0600 },
	},
	{
		.lane_rate = 720000000UL,
		.inck = 72000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0F8 },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B0 },
		.regs[2] = { IMX415_SYS_MODE, 0x9 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x0A0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1200 },
	},
	{
		.lane_rate = 891000000UL,
		.inck = 27000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x05D },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x042 },
		.regs[2] = { IMX415_SYS_MODE, 0x5 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x0C6 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E7 },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x06C0 },
	},
	{
		.lane_rate = 891000000UL,
		.inck = 37125000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x07F },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x05B },
		.regs[2] = { IMX415_SYS_MODE, 0x5 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x24 },
		.regs[5] = { IMX415_INCKSEL3, 0x0C0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x24 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0948 },
	},
	{
		.lane_rate = 891000000UL,
		.inck = 74250000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0FF },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B6 },
		.regs[2] = { IMX415_SYS_MODE, 0x5 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x0C0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x0 },
		.regs[9] = { IMX415_INCKSEL7, 0x1 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1290 },
	},
	{
		.lane_rate = 1440000000UL,
		.inck = 24000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x054 },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x03B },
		.regs[2] = { IMX415_SYS_MODE, 0x8 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x0B4 },
		.regs[6] = { IMX415_INCKSEL4, 0x0FC },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0600 },
	},
	{
		.lane_rate = 1440000000UL,
		.inck = 72000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0F8 },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B0 },
		.regs[2] = { IMX415_SYS_MODE, 0x8 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x0A0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1200 },
	},
	{
		.lane_rate = 1485000000UL,
		.inck = 27000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x05D },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x042 },
		.regs[2] = { IMX415_SYS_MODE, 0x8 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x0A5 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E7 },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x06C0 },
	},
	{
		.lane_rate = 1485000000UL,
		.inck = 37125000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x07F },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x05B },
		.regs[2] = { IMX415_SYS_MODE, 0x8 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x24 },
		.regs[5] = { IMX415_INCKSEL3, 0x0A0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x24 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0948 },
	},
	{
		.lane_rate = 1485000000UL,
		.inck = 74250000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0FF },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B6 },
		.regs[2] = { IMX415_SYS_MODE, 0x8 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x0A0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1290 },
	},
	{
		.lane_rate = 1782000000UL,
		.inck = 27000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x05D },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x042 },
		.regs[2] = { IMX415_SYS_MODE, 0x4 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x0C6 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E7 },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x06C0 },
	},
	{
		.lane_rate = 1782000000UL,
		.inck = 37125000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x07F },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x05B },
		.regs[2] = { IMX415_SYS_MODE, 0x4 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x24 },
		.regs[5] = { IMX415_INCKSEL3, 0x0C0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x24 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0948 },
	},
	{
		.lane_rate = 1782000000UL,
		.inck = 74250000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0FF },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B6 },
		.regs[2] = { IMX415_SYS_MODE, 0x4 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x0C0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1290 },
	},
	{
		.lane_rate = 2079000000UL,
		.inck = 27000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x05D },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x042 },
		.regs[2] = { IMX415_SYS_MODE, 0x2 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x0E7 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E7 },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x06C0 },
	},
	{
		.lane_rate = 2079000000UL,
		.inck = 37125000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x07F },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x05B },
		.regs[2] = { IMX415_SYS_MODE, 0x2 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x24 },
		.regs[5] = { IMX415_INCKSEL3, 0x0E0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x24 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0948 },
	},
	{
		.lane_rate = 2079000000UL,
		.inck = 74250000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0FF },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B6 },
		.regs[2] = { IMX415_SYS_MODE, 0x2 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x0E0 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1290 },
	},
	{
		.lane_rate = 2376000000UL,
		.inck = 27000000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x05D },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x042 },
		.regs[2] = { IMX415_SYS_MODE, 0x0 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x23 },
		.regs[5] = { IMX415_INCKSEL3, 0x108 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E7 },
		.regs[7] = { IMX415_INCKSEL5, 0x23 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x06C0 },
	},
	{
		.lane_rate = 2376000000UL,
		.inck = 37125000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x07F },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x05B },
		.regs[2] = { IMX415_SYS_MODE, 0x0 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x24 },
		.regs[5] = { IMX415_INCKSEL3, 0x100 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x24 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0948 },
	},
	{
		.lane_rate = 2376000000UL,
		.inck = 74250000,
		.regs[0] = { IMX415_BCWAIT_TIME, 0x0FF },
		.regs[1] = { IMX415_CPWAIT_TIME, 0x0B6 },
		.regs[2] = { IMX415_SYS_MODE, 0x0 },
		.regs[3] = { IMX415_INCKSEL1, 0x00 },
		.regs[4] = { IMX415_INCKSEL2, 0x28 },
		.regs[5] = { IMX415_INCKSEL3, 0x100 },
		.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
		.regs[7] = { IMX415_INCKSEL5, 0x28 },
		.regs[8] = { IMX415_INCKSEL6, 0x1 },
		.regs[9] = { IMX415_INCKSEL7, 0x0 },
		.regs[10] = { IMX415_TXCLKESC_FREQ, 0x1290 },
	},
};

static const struct imx415_clk_params imx415_clk_37125000_1782000000 = {
	.lane_rate = 1782000000UL,
	.inck = 37125000,
	.regs[0] = { IMX415_BCWAIT_TIME, 0x07F },
	.regs[1] = { IMX415_CPWAIT_TIME, 0x05B },
	.regs[2] = { IMX415_SYS_MODE, 0x4 },
	.regs[3] = { IMX415_INCKSEL1, 0x00 },
	.regs[4] = { IMX415_INCKSEL2, 0x24 },
	.regs[5] = { IMX415_INCKSEL3, 0x0C0 },
	.regs[6] = { IMX415_INCKSEL4, 0x0E0 },
	.regs[7] = { IMX415_INCKSEL5, 0x24 },
	.regs[8] = { IMX415_INCKSEL6, 0x1 },
	.regs[9] = { IMX415_INCKSEL7, 0x0 },
	.regs[10] = { IMX415_TXCLKESC_FREQ, 0x0948 },
};

/* all-pixel 2-lane 720 Mbps 15.74 Hz mode */
static const struct cci_reg_sequence imx415_mode_2_720[] = {
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x07F0 },
	{ IMX415_LANEMODE, IMX415_LANEMODE_2 },
	{ IMX415_TCLKPOST, 0x006F },
	{ IMX415_TCLKPREPARE, 0x002F },
	{ IMX415_TCLKTRAIL, 0x002F },
	{ IMX415_TCLKZERO, 0x00BF },
	{ IMX415_THSPREPARE, 0x002F },
	{ IMX415_THSZERO, 0x0057 },
	{ IMX415_THSTRAIL, 0x002F },
	{ IMX415_THSEXIT, 0x004F },
	{ IMX415_TLPX, 0x0027 },
};

/* all-pixel 2-lane 1440 Mbps 30.01 Hz mode */
static const struct cci_reg_sequence imx415_mode_2_1440[] = {
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x042A },
	{ IMX415_LANEMODE, IMX415_LANEMODE_2 },
	{ IMX415_TCLKPOST, 0x009F },
	{ IMX415_TCLKPREPARE, 0x0057 },
	{ IMX415_TCLKTRAIL, 0x0057 },
	{ IMX415_TCLKZERO, 0x0187 },
	{ IMX415_THSPREPARE, 0x005F },
	{ IMX415_THSZERO, 0x00A7 },
	{ IMX415_THSTRAIL, 0x005F },
	{ IMX415_THSEXIT, 0x0097 },
	{ IMX415_TLPX, 0x004F },
};

/* all-pixel 4-lane 891 Mbps 30 Hz mode */
static const struct cci_reg_sequence imx415_mode_4_891[] = {
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x044C },
	{ IMX415_LANEMODE, IMX415_LANEMODE_4 },
	{ IMX415_TCLKPOST, 0x007F },
	{ IMX415_TCLKPREPARE, 0x0037 },
	{ IMX415_TCLKTRAIL, 0x0037 },
	{ IMX415_TCLKZERO, 0x00F7 },
	{ IMX415_THSPREPARE, 0x003F },
	{ IMX415_THSZERO, 0x006F },
	{ IMX415_THSTRAIL, 0x003F },
	{ IMX415_THSEXIT, 0x005F },
	{ IMX415_TLPX, 0x002F },
};

/* 1932x1096 4-lane 12bit 1782 Mbps 60 Hz mode */
static const struct cci_reg_sequence imx415_mode_4_1782_22binning_12bit[] = {
	/* use binning readout mode, no flip */
	{ IMX415_WINMODE, 0x00 },
	{ IMX415_ADDMODE, 0x01 },
	/* use RAW 12-bit mode */
	{ IMX415_ADBIT, 0x00 },
	{ IMX415_MDBIT, 0x01 },
	/* output VSYNC on XVS and low on XHS */
	{ IMX415_OUTSEL, 0x22 },
	{ IMX415_DRV, 0x00 },
	{ IMX415_VMAX, 0x08CA }, //2250
	{ IMX415_HMAX, 0x0226 }, //550
	{ IMX415_LANEMODE, IMX415_LANEMODE_4 },
	{ IMX415_TCLKPOST, 0x00B7 },
	{ IMX415_TCLKPREPARE, 0x0067 },
	{ IMX415_TCLKTRAIL, 0x006F },
	{ IMX415_TCLKZERO, 0x01DF },
	{ IMX415_THSPREPARE, 0x006F },
	{ IMX415_THSZERO, 0x00CF },
	{ IMX415_THSTRAIL, 0x006F },
	{ IMX415_THSEXIT, 0x00B7 },
	{ IMX415_TLPX, 0x005F },
};

/* all-pixel 4-lane 12bit 1782 Mbps 60 Hz mode */
static const struct cci_reg_sequence imx415_mode_4_1782_12bit[] = {
	/* use all-pixel readout mode, no flip */
	{ IMX415_WINMODE, 0x00 },
	{ IMX415_ADDMODE, 0x00 },
	/* use RAW 12-bit mode */
	{ IMX415_ADBIT, 0x01 },
	{ IMX415_MDBIT, 0x0001 },
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x0226 },
	{ IMX415_LANEMODE, IMX415_LANEMODE_4 },
	{ IMX415_TCLKPOST, 0x00B7 },
	{ IMX415_TCLKPREPARE, 0x0067 },
	{ IMX415_TCLKTRAIL, 0x006F },
	{ IMX415_TCLKZERO, 0x00DF },
	{ IMX415_THSPREPARE, 0x006F },
	{ IMX415_THSZERO, 0x00CF },
	{ IMX415_THSTRAIL, 0x006F },
	{ IMX415_THSEXIT, 0x00B7 },
	{ IMX415_TLPX, 0x005F },
};

/* all-pixel 4-lane 12bit 1782 Mbps 60 Hz mode cropto 5M */
static const struct cci_reg_sequence imx415_mode_4_1782_crop_5m_12bit[] = {
	/* use all-pixel/Winmode mode, no flip */
	{ IMX415_WINMODE, 0x04 },
	{ IMX415_ADDMODE, 0x00 },
	/* use RAW 12-bit mode */
	{ IMX415_ADBIT, 0x01 },
	{ IMX415_MDBIT, 0x0001 },
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x0226 },
	{ IMX415_LANEMODE, IMX415_LANEMODE_4 },
	{ IMX415_TCLKPOST, 0x00B7 },
	{ IMX415_TCLKPREPARE, 0x0067 },
	{ IMX415_TCLKTRAIL, 0x006F },
	{ IMX415_TCLKZERO, 0x00DF },
	{ IMX415_THSPREPARE, 0x006F },
	{ IMX415_THSZERO, 0x00CF },
	{ IMX415_THSTRAIL, 0x006F },
	{ IMX415_THSEXIT, 0x00B7 },
	{ IMX415_TLPX, 0x005F },
	//crop 3864*2192 -> 2688*1944
	{ IMX415_PIX_HST, 0x024C }, //IMX415_PIX_HST=(3864 - 2688)/2 =588=0x024C
	{ IMX415_PIX_HWIDTH, 0x0A80 },//2688
	{ IMX415_PIX_VST, 0x00F8 }, // IMX415_PIX_VST/2=(2191-1944)/2=123.5
	{ IMX415_PIX_VWIDTH, 0x0F30},//IMX415_PIX_VWIDTH/2=1944
};

/* 1932x1096 4-lane 10bit 1782 Mbps 60 Hz mode */
static const struct cci_reg_sequence imx415_mode_4_1782_22binning_10bit[] = {
	/* use binning readout mode, no flip */
	{ IMX415_WINMODE, 0x00 },
	{ IMX415_ADDMODE, 0x01 },
	/* use RAW 12-bit mode */
	{ IMX415_ADBIT, 0x00 },
	{ IMX415_MDBIT, 0x00 },
	/* output VSYNC on XVS and low on XHS */
	{ IMX415_OUTSEL, 0x22 },
	{ IMX415_DRV, 0x00 },
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x0226 },
	{ IMX415_LANEMODE, IMX415_LANEMODE_4 },
	{ IMX415_TCLKPOST, 0x00B7 },
	{ IMX415_TCLKPREPARE, 0x0067 },
	{ IMX415_TCLKTRAIL, 0x006F },
	{ IMX415_TCLKZERO, 0x01DF },
	{ IMX415_THSPREPARE, 0x006F },
	{ IMX415_THSZERO, 0x00CF },
	{ IMX415_THSTRAIL, 0x006F },
	{ IMX415_THSEXIT, 0x00B7 },
	{ IMX415_TLPX, 0x005F },
};

/* all-pixel 4-lane 10bit 1782 Mbps 60 Hz mode */
static const struct cci_reg_sequence imx415_mode_4_1782[] = {
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x0226 },
	{ IMX415_LANEMODE, IMX415_LANEMODE_4 },
	{ IMX415_TCLKPOST, 0x00B7 },
	{ IMX415_TCLKPREPARE, 0x0067 },
	{ IMX415_TCLKTRAIL, 0x006F },
	{ IMX415_TCLKZERO, 0x00DF },
	{ IMX415_THSPREPARE, 0x006F },
	{ IMX415_THSZERO, 0x00CF },
	{ IMX415_THSTRAIL, 0x006F },
	{ IMX415_THSEXIT, 0x00B7 },
	{ IMX415_TLPX, 0x005F },
};

/* all-pixel 4-lane 10bit 1782 Mbps 60 Hz mode cropto 5M */
static const struct cci_reg_sequence imx415_mode_4_1782_crop_5m[] = {
	/* use all-pixel/Winmode mode, no flip */
	{ IMX415_WINMODE, 0x04 },
	{ IMX415_ADDMODE, 0x00 },
	{ IMX415_VMAX, 0x08CA },
	{ IMX415_HMAX, 0x0226 },
	{ IMX415_LANEMODE, IMX415_LANEMODE_4 },
	{ IMX415_TCLKPOST, 0x00B7 },
	{ IMX415_TCLKPREPARE, 0x0067 },
	{ IMX415_TCLKTRAIL, 0x006F },
	{ IMX415_TCLKZERO, 0x00DF },
	{ IMX415_THSPREPARE, 0x006F },
	{ IMX415_THSZERO, 0x00CF },
	{ IMX415_THSTRAIL, 0x006F },
	{ IMX415_THSEXIT, 0x00B7 },
	{ IMX415_TLPX, 0x005F },
	//crop 3864*2192 -> 2688*1944
	{ IMX415_PIX_HST, 0x024C }, //IMX415_PIX_HST=(3864 - 2688)/2 =588=0x024C
	{ IMX415_PIX_HWIDTH, 0x0A80 },//2688
	{ IMX415_PIX_VST, 0x00F8 }, // IMX415_PIX_VST/2=(2191-1944)/2=123.5
	{ IMX415_PIX_VWIDTH, 0x0F30},//IMX415_PIX_VWIDTH/2=1944
};

struct imx415_mode_reg_list {
	u32 num_of_regs;
	const struct cci_reg_sequence *regs;
};

/*
 * Mode : number of lanes, lane rate and frame rate dependent settings
 *
 * pixel_rate and hmax_pix are needed to calculate hblank for the v4l2 ctrl
 * interface. These values can not be found in the data sheet and should be
 * treated as virtual values. Use following table when adding new modes.
 *
 * lane_rate  lanes    fps     hmax_pix   pixel_rate
 *
 *     594      2     10.000     4400       99000000
 *     891      2     15.000     4400      148500000
 *     720      2     15.748     4064      144000000
 *    1782      2     30.000     4400      297000000
 *    2079      2     30.000     4400      297000000
 *    1440      2     30.019     4510      304615385
 *
 *     594      4     20.000     5500      247500000
 *     594      4     25.000     4400      247500000
 *     720      4     25.000     4400      247500000
 *     720      4     30.019     4510      304615385
 *     891      4     30.000     4400      297000000
 *    1440      4     30.019     4510      304615385
 *    1440      4     60.038     4510      609230769
 *    1485      4     60.000     4400      594000000
 *    1782      4     60.000     4400      594000000
 *    2079      4     60.000     4400      594000000
 *    2376      4     90.164     4392      891000000
 */
struct imx415_mode {
	u32 bus_fmt;
	u32 width;
	u32 height;
	u32 bpp;
	u32 vts_def;
	u32 hts_def;
	u64 lane_rate;
	u32 lanes;
	u32 hmax_pix;
	u64 pixel_rate;
	struct imx415_mode_reg_list reg_list;
	struct imx415_clk_params clk_params;
};

/* mode configs */
static const struct imx415_mode supported_modes[] = {
	{ // 60FPS inck 37125000Hz lane_rate 1782000000 pixel_rate 594000000 12bit 2*2 binning
		.bus_fmt = MEDIA_BUS_FMT_SGBRG12_1X12,
		.width = 1944,
		.height = 1096,
		.bpp = 12,
		.lane_rate = 1782000000,
		.lanes = 4,
		.hmax_pix = 2346,
		.vts_def = 3165,//2346
		.pixel_rate = 594000000,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(imx415_mode_4_1782_22binning_12bit),
			.regs = imx415_mode_4_1782_22binning_12bit,
		},
		.clk_params = imx415_clk_37125000_1782000000,
	},
	{ // 60FPS inck 37125000Hz lane_rate 1782000000 pixel_rate 594000000 12bit
		.bus_fmt = MEDIA_BUS_FMT_SGBRG12_1X12,
		.width = 3864,
		.height = 2192,
		.bpp = 12,
		.lane_rate = 1782000000,
		.lanes = 4,
		.hmax_pix = 4400,
		.vts_def = 0x08ca, // 2250
		.pixel_rate = 594000000,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(imx415_mode_4_1782_12bit),
			.regs = imx415_mode_4_1782_12bit,
		},
		.clk_params = imx415_clk_37125000_1782000000,
	},
	{ // 60FPS inck 37125000Hz lane_rate 1782000000  pixel_rate 594000000 10bit
		.bus_fmt = MEDIA_BUS_FMT_SGBRG12_1X12,
		.width = 2688,
		.height = 1944,
		.bpp = 12,
		.lane_rate = 1782000000,
		.lanes = 4,
		.hmax_pix = 4400,
		.vts_def = 0x08ca, // 2250
		.pixel_rate = 594000000,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(imx415_mode_4_1782_crop_5m_12bit),
			.regs = imx415_mode_4_1782_crop_5m_12bit,
		},
		.clk_params = imx415_clk_37125000_1782000000,
	},
	{ // 60FPS inck 37125000Hz lane_rate 1782000000 pixel_rate 594000000 10bit 2*2 binning
		.bus_fmt = MEDIA_BUS_FMT_SGBRG10_1X10,
		.width = 1944,
		.height = 1096,
		.bpp = 10,
		.lane_rate = 1782000000,
		.lanes = 4,
		.hmax_pix = 2346,
		.pixel_rate = 594000000,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(imx415_mode_4_1782_22binning_10bit),
			.regs = imx415_mode_4_1782_22binning_10bit,
		},
		.clk_params = imx415_clk_37125000_1782000000,
	},
	{ // 60FPS inck 37125000Hz lane_rate 1782000000  pixel_rate 594000000 10bit
		.bus_fmt = MEDIA_BUS_FMT_SGBRG10_1X10,
		.width = 3864,
		.height = 2192,
		.bpp = 10,
		.lane_rate = 1782000000,
		.lanes = 4,
		.hmax_pix = 4400,
		.pixel_rate = 594000000,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(imx415_mode_4_1782),
			.regs = imx415_mode_4_1782,
		},
		.clk_params = imx415_clk_37125000_1782000000,
	},
	{ // 60FPS inck 37125000Hz lane_rate 1782000000  pixel_rate 594000000 10bit
		.bus_fmt = MEDIA_BUS_FMT_SGBRG10_1X10,
		.width = 2688,
		.height = 1944,
		.bpp = 10,
		.lane_rate = 1782000000,
		.lanes = 4,
		.hmax_pix = 4400,
		.pixel_rate = 594000000,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(imx415_mode_4_1782_crop_5m),
			.regs = imx415_mode_4_1782_crop_5m,
		},
		.clk_params = imx415_clk_37125000_1782000000,
	},
};

static const char *const imx415_test_pattern_menu[] = {
	"disabled",
	"solid black",
	"solid white",
	"solid dark gray",
	"solid light gray",
	"stripes light/dark grey",
	"stripes dark/light grey",
	"stripes black/dark grey",
	"stripes dark grey/black",
	"stripes black/white",
	"stripes white/black",
	"horizontal color bar",
	"vertical color bar",
};

struct imx415 {
	struct device *dev;
	struct clk *clk;
	struct regulator_bulk_data supplies[ARRAY_SIZE(imx415_supply_names)];
	struct gpio_desc *reset;
	struct regmap *regmap;

	const struct imx415_clk_params *clk_params;

	struct v4l2_subdev subdev;
	struct media_pad pad;

	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hflip;
	struct v4l2_ctrl *vflip;
	const struct imx415_mode *supported_modes;
	const struct imx415_mode *mode;
	unsigned int num_data_lanes;
	unsigned int cfg_num;
	/*
	 * Mutex for serialized access:
	 * Protect sensor module set pad format and start/stop streaming safely.
	 */
	struct mutex mutex;
};

/*
 * This table includes fixed register settings and a bunch of undocumented
 * registers that have to be set to another value than default.
 */
static const struct cci_reg_sequence imx415_init_table[] = {
	/* use all-pixel readout mode, no flip */
	{ IMX415_WINMODE, 0x00 },
	{ IMX415_ADDMODE, 0x00 },
	{ IMX415_REVERSE, 0x00 },
	/* use RAW 10-bit mode */
	{ IMX415_ADBIT, 0x00 },
	{ IMX415_MDBIT, 0x00 },
	/* output VSYNC on XVS and low on XHS */
	{ IMX415_OUTSEL, 0x22 },
	{ IMX415_DRV, 0x00 },

	/* SONY magic registers */
	{ CCI_REG8(0x32D4), 0x21 },
	{ CCI_REG8(0x32EC), 0xA1 },
	{ CCI_REG8(0x3452), 0x7F },
	{ CCI_REG8(0x3453), 0x03 },
	{ CCI_REG8(0x358A), 0x04 },
	{ CCI_REG8(0x35A1), 0x02 },
	{ CCI_REG8(0x36BC), 0x0C },
	{ CCI_REG8(0x36CC), 0x53 },
	{ CCI_REG8(0x36CD), 0x00 },
	{ CCI_REG8(0x36CE), 0x3C },
	{ CCI_REG8(0x36D0), 0x8C },
	{ CCI_REG8(0x36D1), 0x00 },
	{ CCI_REG8(0x36D2), 0x71 },
	{ CCI_REG8(0x36D4), 0x3C },
	{ CCI_REG8(0x36D6), 0x53 },
	{ CCI_REG8(0x36D7), 0x00 },
	{ CCI_REG8(0x36D8), 0x71 },
	{ CCI_REG8(0x36DA), 0x8C },
	{ CCI_REG8(0x36DB), 0x00 },
	{ CCI_REG8(0x3724), 0x02 },
	{ CCI_REG8(0x3726), 0x02 },
	{ CCI_REG8(0x3732), 0x02 },
	{ CCI_REG8(0x3734), 0x03 },
	{ CCI_REG8(0x3736), 0x03 },
	{ CCI_REG8(0x3742), 0x03 },
	{ CCI_REG8(0x3862), 0xE0 },
	{ CCI_REG8(0x38CC), 0x30 },
	{ CCI_REG8(0x38CD), 0x2F },
	{ CCI_REG8(0x395C), 0x0C },
	{ CCI_REG8(0x3A42), 0xD1 },
	{ CCI_REG8(0x3A4C), 0x77 },
	{ CCI_REG8(0x3AE0), 0x02 },
	{ CCI_REG8(0x3AEC), 0x0C },
	{ CCI_REG8(0x3B00), 0x2E },
	{ CCI_REG8(0x3B06), 0x29 },
	{ CCI_REG8(0x3B98), 0x25 },
	{ CCI_REG8(0x3B99), 0x21 },
	{ CCI_REG8(0x3B9B), 0x13 },
	{ CCI_REG8(0x3B9C), 0x13 },
	{ CCI_REG8(0x3B9D), 0x13 },
	{ CCI_REG8(0x3B9E), 0x13 },
	{ CCI_REG8(0x3BA1), 0x00 },
	{ CCI_REG8(0x3BA2), 0x06 },
	{ CCI_REG8(0x3BA3), 0x0B },
	{ CCI_REG8(0x3BA4), 0x10 },
	{ CCI_REG8(0x3BA5), 0x14 },
	{ CCI_REG8(0x3BA6), 0x18 },
	{ CCI_REG8(0x3BA7), 0x1A },
	{ CCI_REG8(0x3BA8), 0x1A },
	{ CCI_REG8(0x3BA9), 0x1A },
	{ CCI_REG8(0x3BAC), 0xED },
	{ CCI_REG8(0x3BAD), 0x01 },
	{ CCI_REG8(0x3BAE), 0xF6 },
	{ CCI_REG8(0x3BAF), 0x02 },
	{ CCI_REG8(0x3BB0), 0xA2 },
	{ CCI_REG8(0x3BB1), 0x03 },
	{ CCI_REG8(0x3BB2), 0xE0 },
	{ CCI_REG8(0x3BB3), 0x03 },
	{ CCI_REG8(0x3BB4), 0xE0 },
	{ CCI_REG8(0x3BB5), 0x03 },
	{ CCI_REG8(0x3BB6), 0xE0 },
	{ CCI_REG8(0x3BB7), 0x03 },
	{ CCI_REG8(0x3BB8), 0xE0 },
	{ CCI_REG8(0x3BBA), 0xE0 },
	{ CCI_REG8(0x3BBC), 0xDA },
	{ CCI_REG8(0x3BBE), 0x88 },
	{ CCI_REG8(0x3BC0), 0x44 },
	{ CCI_REG8(0x3BC2), 0x7B },
	{ CCI_REG8(0x3BC4), 0xA2 },
	{ CCI_REG8(0x3BC8), 0xBD },
	{ CCI_REG8(0x3BCA), 0xBD },
};

static inline struct imx415 *to_imx415(struct v4l2_subdev *sd)
{
	return container_of(sd, struct imx415, subdev);
}

static int imx415_set_testpattern(struct imx415 *sensor, int val)
{
	int ret = 0;

	if (val) {
		cci_write(sensor->regmap, IMX415_BLKLEVEL, 0x00, &ret);
		cci_write(sensor->regmap, IMX415_TPG_EN_DUOUT, 0x01, &ret);
		cci_write(sensor->regmap, IMX415_TPG_PATSEL_DUOUT,
			  val - 1, &ret);
		cci_write(sensor->regmap, IMX415_TPG_COLORWIDTH, 0x01, &ret);
		cci_write(sensor->regmap, IMX415_TESTCLKEN_MIPI, 0x20, &ret);
		cci_write(sensor->regmap, IMX415_DIG_CLP_MODE, 0x00, &ret);
		cci_write(sensor->regmap, IMX415_WRJ_OPEN, 0x00, &ret);
	} else {
		cci_write(sensor->regmap, IMX415_BLKLEVEL,
			  IMX415_BLKLEVEL_DEFAULT, &ret);
		cci_write(sensor->regmap, IMX415_TPG_EN_DUOUT, 0x00, &ret);
		cci_write(sensor->regmap, IMX415_TESTCLKEN_MIPI, 0x00, &ret);
		cci_write(sensor->regmap, IMX415_DIG_CLP_MODE, 0x01, &ret);
		cci_write(sensor->regmap, IMX415_WRJ_OPEN, 0x01, &ret);
	}
	return 0;
}

static int imx415_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct imx415 *sensor = container_of(ctrl->handler, struct imx415,
					     ctrls);
	unsigned int vmax;
	unsigned int flip;
	int ret;

	if (!pm_runtime_get_if_in_use(sensor->dev))
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		/* clamp the exposure value to VMAX. */
		vmax = IMX415_PIXEL_ARRAY_HEIGHT + sensor->vblank->cur.val;
		ctrl->val = min_t(int, ctrl->val, vmax);
		ret = cci_write(sensor->regmap, IMX415_SHR0,
				vmax - ctrl->val, NULL);
		break;

	case V4L2_CID_ANALOGUE_GAIN:
		/* analogue gain in 0.3 dB step size */
		ret = cci_write(sensor->regmap, IMX415_GAIN_PCG_0,
				ctrl->val, NULL);
		break;

	case V4L2_CID_HFLIP:
	case V4L2_CID_VFLIP:
		flip = (sensor->hflip->val << IMX415_HREVERSE_SHIFT) |
		       (sensor->vflip->val << IMX415_VREVERSE_SHIFT);
		ret = cci_write(sensor->regmap, IMX415_REVERSE, flip, NULL);
		break;

	case V4L2_CID_TEST_PATTERN:
		ret = imx415_set_testpattern(sensor, ctrl->val);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	pm_runtime_put(sensor->dev);

	return ret;
}

static const struct v4l2_ctrl_ops imx415_ctrl_ops = {
	.s_ctrl = imx415_s_ctrl,
};

static int imx415_ctrls_init(struct imx415 *sensor)
{
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl *ctrl;
	u64 pixel_rate = sensor->mode->pixel_rate;
	u64 lane_rate = sensor->mode->lane_rate;
	u32 exposure_max = IMX415_PIXEL_ARRAY_HEIGHT +
			   IMX415_PIXEL_ARRAY_VBLANK - 8;
	u32 hblank;
	unsigned int i;
	int ret;

	ret = v4l2_fwnode_device_parse(sensor->dev, &props);
	if (ret < 0)
		return ret;

	v4l2_ctrl_handler_init(&sensor->ctrls, 10);

	for (i = 0; i < ARRAY_SIZE(link_freq_menu_items); ++i) {
		if (lane_rate == link_freq_menu_items[i] * 2)
			break;
	}
	if (i == ARRAY_SIZE(link_freq_menu_items)) {
		return dev_err_probe(sensor->dev, -EINVAL,
				     "lane rate %llu not supported\n",
				     lane_rate);
	}

	mutex_init(&sensor->mutex);
	sensor->ctrls.lock = &sensor->mutex;

	ctrl = v4l2_ctrl_new_int_menu(&sensor->ctrls, &imx415_ctrl_ops,
				      V4L2_CID_LINK_FREQ,
				      ARRAY_SIZE(link_freq_menu_items) - 1, i,
				      link_freq_menu_items);

	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	v4l2_ctrl_new_std(&sensor->ctrls, &imx415_ctrl_ops, V4L2_CID_EXPOSURE,
			  4, exposure_max, 1, exposure_max);

	v4l2_ctrl_new_std(&sensor->ctrls, &imx415_ctrl_ops,
			  V4L2_CID_ANALOGUE_GAIN, IMX415_AGAIN_MIN,
			  IMX415_AGAIN_MAX, IMX415_AGAIN_STEP,
			  IMX415_AGAIN_DEFAULT);

	hblank = sensor->mode->hmax_pix -
		 sensor->mode->width;
	ctrl = v4l2_ctrl_new_std(&sensor->ctrls, &imx415_ctrl_ops,
				 V4L2_CID_HBLANK, hblank, hblank, 1, hblank);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	sensor->vblank = v4l2_ctrl_new_std(&sensor->ctrls, &imx415_ctrl_ops,
					   V4L2_CID_VBLANK,
					   IMX415_PIXEL_ARRAY_VBLANK,
					   IMX415_PIXEL_ARRAY_VBLANK, 1,
					   IMX415_PIXEL_ARRAY_VBLANK);
	if (sensor->vblank)
		sensor->vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	/*
	 * The pixel rate used here is a virtual value and can be used for
	 * calculating the frame rate together with hblank. It may not
	 * necessarily be the physically correct pixel clock.
	 */
	v4l2_ctrl_new_std(&sensor->ctrls, NULL, V4L2_CID_PIXEL_RATE, pixel_rate,
			  pixel_rate, 1, pixel_rate);

	sensor->hflip = v4l2_ctrl_new_std(&sensor->ctrls, &imx415_ctrl_ops,
					  V4L2_CID_HFLIP, 0, 1, 1, 0);
	sensor->vflip = v4l2_ctrl_new_std(&sensor->ctrls, &imx415_ctrl_ops,
					  V4L2_CID_VFLIP, 0, 1, 1, 0);

	v4l2_ctrl_new_std_menu_items(&sensor->ctrls, &imx415_ctrl_ops,
				     V4L2_CID_TEST_PATTERN,
				     ARRAY_SIZE(imx415_test_pattern_menu) - 1,
				     0, 0, imx415_test_pattern_menu);

	v4l2_ctrl_new_fwnode_properties(&sensor->ctrls, &imx415_ctrl_ops,
					&props);

	if (sensor->ctrls.error) {
		dev_err_probe(sensor->dev, sensor->ctrls.error,
			      "failed to add controls\n");
		v4l2_ctrl_handler_free(&sensor->ctrls);
		mutex_destroy(&sensor->mutex);
		return sensor->ctrls.error;
	}
	sensor->subdev.ctrl_handler = &sensor->ctrls;

	return 0;
}

static int imx415_set_mode(struct imx415 *sensor)
{
	int ret = 0;

	cci_multi_reg_write(sensor->regmap,
			    sensor->mode->reg_list.regs,
			    sensor->mode->reg_list.num_of_regs,
			    &ret);
	cci_multi_reg_write(sensor->regmap,
			    sensor->mode->clk_params.regs,
			    IMX415_NUM_CLK_PARAM_REGS,
			    &ret);

	return 0;
}

static int imx415_setup(struct imx415 *sensor)
{
	int ret;

	ret = cci_multi_reg_write(sensor->regmap,
				  imx415_init_table,
				  ARRAY_SIZE(imx415_init_table),
				  NULL);
	if (ret)
		return ret;

	return imx415_set_mode(sensor);
}

static int imx415_wakeup(struct imx415 *sensor)
{
	int ret;

	ret = cci_write(sensor->regmap, IMX415_MODE,
			IMX415_MODE_OPERATING, NULL);
	if (ret)
		return ret;

	/*
	 * According to the datasheet we have to wait at least 63 us after
	 * leaving standby mode. But this doesn't work even after 30 ms.
	 * So probably this should be 63 ms and therefore we wait for 80 ms.
	 */
	msleep(80);

	return 0;
}

static int imx415_stream_on(struct imx415 *sensor)
{
	int ret;

	ret = imx415_wakeup(sensor);
	return cci_write(sensor->regmap, IMX415_XMSTA,
			 IMX415_XMSTA_START, &ret);
}

static int imx415_stream_off(struct imx415 *sensor)
{
	int ret;

	ret = cci_write(sensor->regmap, IMX415_XMSTA,
			IMX415_XMSTA_STOP, NULL);
	return cci_write(sensor->regmap, IMX415_MODE,
			 IMX415_MODE_STANDBY, &ret);
}

static int imx415_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct imx415 *sensor = to_imx415(sd);
	int ret;

	mutex_lock(&sensor->mutex);

	if (!enable) {
		ret = imx415_stream_off(sensor);

		pm_runtime_mark_last_busy(sensor->dev);
		pm_runtime_put_autosuspend(sensor->dev);

		goto unlock;
	}

	ret = pm_runtime_resume_and_get(sensor->dev);
	if (ret < 0)
		goto unlock;

	ret = imx415_setup(sensor);
	if (ret)
		goto err_pm;

	ret = __v4l2_ctrl_handler_setup(&sensor->ctrls);
	if (ret < 0)
		goto err_pm;

	ret = imx415_stream_on(sensor);
	if (ret)
		goto err_pm;

	ret = 0;

unlock:
	mutex_unlock(&sensor->mutex);

	return ret;

err_pm:
	/*
	 * In case of error, turn the power off synchronously as the device
	 * likely has no other chance to recover.
	 */
	pm_runtime_put_sync(sensor->dev);

	goto unlock;
}

static int imx415_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *state,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct imx415 *sensor = to_imx415(sd);

	if (code->index >= sensor->cfg_num)
		return -EINVAL;

	code->code = sensor->supported_modes[code->index].bus_fmt;
	return 0;
}

static int imx415_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct imx415 *sensor = to_imx415(sd);
	int skip_index = fse->index + 1;
	int find_count = 0;
	int i = 0;

	if (fse->index >= sensor->cfg_num)
		return -EINVAL;

	for (i = 0; i < sensor->cfg_num; i++) {
		if (fse->code == sensor->supported_modes[i].bus_fmt) {
			find_count++;
			if (find_count == skip_index) {
				fse->min_width = sensor->supported_modes[fse->index].width;
				fse->max_width = sensor->supported_modes[fse->index].width;
				fse->max_height = sensor->supported_modes[fse->index].height;
				fse->min_height = sensor->supported_modes[fse->index].height;
				return 0;
			}
		}
	}
	return -EINVAL;
}

static void imx415_reset_colorspace(struct v4l2_mbus_framefmt *fmt)
{
	fmt->colorspace = V4L2_COLORSPACE_RAW;
	fmt->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(fmt->colorspace);
	fmt->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(true,
							  fmt->colorspace,
							  fmt->ycbcr_enc);
	fmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(fmt->colorspace);
}

static void imx415_update_image_pad_format(struct imx415 *imx415,
					   const struct imx415_mode *mode,
					   struct v4l2_subdev_format *fmt)
{
	fmt->format.code = mode->bus_fmt;
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;
	imx415_reset_colorspace(&fmt->format);
}

static int imx415_get_format(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *state,
			     struct v4l2_subdev_format *fmt)
{
	struct imx415 *imx415 = to_imx415(sd);
	struct v4l2_mbus_framefmt *try_fmt;
	const struct imx415_mode *mode = imx415->mode;

	mutex_lock(&imx415->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		try_fmt = v4l2_subdev_get_try_format(sd, state, fmt->pad);
		fmt->format = *try_fmt;
	} else {
		imx415_update_image_pad_format(imx415, mode, fmt);
	}
	mutex_unlock(&imx415->mutex);

	return 0;
}

static int imx415_set_format(struct v4l2_subdev *sd,
			     struct v4l2_subdev_state *state,
			     struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *framefmt;
	struct imx415 *imx415 = to_imx415(sd);
	const struct imx415_mode *mode;

	mode = v4l2_find_nearest_size(imx415->supported_modes,
				      imx415->cfg_num,
				      width, height,
				      fmt->format.width,
				      fmt->format.height);
	mutex_lock(&imx415->mutex);
	imx415_update_image_pad_format(imx415, mode, fmt);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, state, fmt->pad);
		*framefmt = fmt->format;
	} else {
		imx415->mode = mode;
	}
	mutex_unlock(&imx415->mutex);

	return 0;
}

static int imx415_get_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *sd_state,
				struct v4l2_subdev_selection *sel)
{
	struct imx415 *imx415 = to_imx415(sd);
	const struct imx415_mode *mode = imx415->mode;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		mutex_lock(&imx415->mutex);
		if (mode->width == 3864) {
			sel->r.left = CROP_START(mode->width, DST_WIDTH_3840);
			sel->r.width = DST_WIDTH_3840;
			sel->r.top = CROP_START(mode->height, DST_HEIGHT_2160);
			sel->r.height = DST_HEIGHT_2160;
		} else if (mode->width == 1944) {
			sel->r.left = CROP_START(mode->width, DST_WIDTH_1920);
			sel->r.width = DST_WIDTH_1920;
			sel->r.top = CROP_START(mode->height, DST_HEIGHT_1080);
			sel->r.height = DST_HEIGHT_1080;
		} else {
			sel->r.left = CROP_START(mode->width, mode->width);
			sel->r.width = mode->width;
			sel->r.top = CROP_START(mode->height, mode->height);
			sel->r.height = mode->height;
		}
		mutex_unlock(&imx415->mutex);
		return 0;
	}

	return -EINVAL;
}

static int imx415_init_cfg(struct v4l2_subdev *sd,
			   struct v4l2_subdev_state *state)
{
	struct v4l2_subdev_format format = {
		.format = {
			.width = IMX415_PIXEL_ARRAY_WIDTH,
			.height = IMX415_PIXEL_ARRAY_HEIGHT,
		},
	};

	imx415_set_format(sd, state, &format);

	return 0;
}

static const struct v4l2_subdev_core_ops imx415_core_ops = {
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops imx415_subdev_video_ops = {
	.s_stream = imx415_s_stream,
};

static const struct v4l2_subdev_pad_ops imx415_subdev_pad_ops = {
	.enum_mbus_code = imx415_enum_mbus_code,
	.enum_frame_size = imx415_enum_frame_size,
	.get_fmt = imx415_get_format,
	.set_fmt = imx415_set_format,
	.get_selection = imx415_get_selection,
	.init_cfg = imx415_init_cfg,
};

static const struct v4l2_subdev_ops imx415_subdev_ops = {
	.video = &imx415_subdev_video_ops,
	.pad = &imx415_subdev_pad_ops,
	.core = &imx415_core_ops,
};

static int imx415_subdev_init(struct imx415 *sensor)
{
	struct i2c_client *client = to_i2c_client(sensor->dev);
	int ret;

	v4l2_i2c_subdev_init(&sensor->subdev, client, &imx415_subdev_ops);

	ret = imx415_ctrls_init(sensor);
	if (ret)
		return ret;

	sensor->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
				V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sensor->subdev.entity, 1, &sensor->pad);
	if (ret < 0) {
		v4l2_ctrl_handler_free(&sensor->ctrls);
		return ret;
	}

	return 0;
}

static void imx415_subdev_cleanup(struct imx415 *sensor)
{
	media_entity_cleanup(&sensor->subdev.entity);
	v4l2_ctrl_handler_free(&sensor->ctrls);
}

static int imx415_power_on(struct imx415 *sensor)
{
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(sensor->supplies),
				    sensor->supplies);
	if (ret < 0)
		return ret;

	gpiod_set_value_cansleep(sensor->reset, 0);

	udelay(1);

	ret = clk_prepare_enable(sensor->clk);
	if (ret < 0)
		goto err_reset;

	/*
	 * Data sheet states that 20 us are required before communication start,
	 * but this doesn't work in all cases. Use 100 us to be on the safe
	 * side.
	 */
	usleep_range(100, 200);

	return 0;

err_reset:
	gpiod_set_value_cansleep(sensor->reset, 1);
	regulator_bulk_disable(ARRAY_SIZE(sensor->supplies), sensor->supplies);
	return ret;
}

static void imx415_power_off(struct imx415 *sensor)
{
	clk_disable_unprepare(sensor->clk);
	gpiod_set_value_cansleep(sensor->reset, 1);
	regulator_bulk_disable(ARRAY_SIZE(sensor->supplies), sensor->supplies);
}

static int imx415_identify_model(struct imx415 *sensor)
{
	int model, ret;
	u64 chip_id;

	/*
	 * While most registers can be read when the sensor is in standby, this
	 * is not the case of the sensor info register :-(
	 */
	ret = imx415_wakeup(sensor);
	if (ret)
		return dev_err_probe(sensor->dev, ret,
				     "failed to get sensor out of standby\n");

	ret = cci_read(sensor->regmap, IMX415_SENSOR_INFO, &chip_id, NULL);
	if (ret < 0) {
		dev_err_probe(sensor->dev, ret,
			      "failed to read sensor information\n");
		goto done;
	}

	model = chip_id & IMX415_SENSOR_INFO_MASK;

	switch (model) {
	case IMX415_CHIP_ID:
		dev_info(sensor->dev, "Detected IMX415 image sensor\n");
		break;
	default:
		ret = dev_err_probe(sensor->dev, -ENODEV,
				    "invalid device model 0x%04x\n", model);
		goto done;
	}

	ret = 0;

done:
	cci_write(sensor->regmap, IMX415_MODE, IMX415_MODE_STANDBY, &ret);
	return ret;
}

static int imx415_parse_hw_config(struct imx415 *sensor)
{
	struct v4l2_fwnode_endpoint bus_cfg = {
		.bus_type = V4L2_MBUS_CSI2_DPHY,
	};
	struct fwnode_handle *ep;
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(sensor->supplies); ++i)
		sensor->supplies[i].supply = imx415_supply_names[i];

	ret = devm_regulator_bulk_get(sensor->dev, ARRAY_SIZE(sensor->supplies),
				      sensor->supplies);
	if (ret)
		return dev_err_probe(sensor->dev, ret,
				     "failed to get supplies\n");

	sensor->reset = devm_gpiod_get_optional(sensor->dev, "reset",
						GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->reset))
		return dev_err_probe(sensor->dev, PTR_ERR(sensor->reset),
				     "failed to get reset GPIO\n");

	sensor->clk = devm_clk_get(sensor->dev, "inck");
	if (IS_ERR(sensor->clk))
		return dev_err_probe(sensor->dev, PTR_ERR(sensor->clk),
				     "failed to get clock\n");

	ep = fwnode_graph_get_next_endpoint(dev_fwnode(sensor->dev), NULL);
	if (!ep)
		return -ENXIO;

	ret = v4l2_fwnode_endpoint_alloc_parse(ep, &bus_cfg);
	fwnode_handle_put(ep);
	if (ret)
		return ret;

	switch (bus_cfg.bus.mipi_csi2.num_data_lanes) {
	case 2:
	case 4:
		sensor->num_data_lanes = bus_cfg.bus.mipi_csi2.num_data_lanes;
		sensor->supported_modes = supported_modes;
		sensor->cfg_num = ARRAY_SIZE(supported_modes);
		break;
	default:
		ret = dev_err_probe(sensor->dev, -EINVAL,
				    "invalid number of CSI2 data lanes %d\n",
				    bus_cfg.bus.mipi_csi2.num_data_lanes);
		goto done_endpoint_free;
	}

	if (!bus_cfg.nr_of_link_frequencies) {
		ret = dev_err_probe(sensor->dev, -EINVAL,
				    "no link frequencies defined");
		goto done_endpoint_free;
	}

	ret = 0;
done_endpoint_free:
	v4l2_fwnode_endpoint_free(&bus_cfg);

	return ret;
}

static void imx415_set_default_format(struct imx415 *imx415)
{
	/* Set default mode to max resolution */
	imx415->mode = &supported_modes[1];
}

static int imx415_probe(struct i2c_client *client)
{
	struct imx415 *sensor;
	int ret;

	sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->dev = &client->dev;

	ret = imx415_parse_hw_config(sensor);
	if (ret)
		return ret;

	sensor->regmap = devm_cci_regmap_init_i2c(client, 16);
	if (IS_ERR(sensor->regmap))
		return PTR_ERR(sensor->regmap);

	/*
	 * Enable power management. The driver supports runtime PM, but needs to
	 * work when runtime PM is disabled in the kernel. To that end, power
	 * the sensor on manually here, identify it, and fully initialize it.
	 */
	ret = imx415_power_on(sensor);
	if (ret)
		return ret;

	ret = imx415_identify_model(sensor);
	if (ret)
		goto err_power;

	/* Initialize default format */
	imx415_set_default_format(sensor);

	ret = imx415_subdev_init(sensor);
	if (ret)
		goto err_power;

	/*
	 * Enable runtime PM. As the device has been powered manually, mark it
	 * as active, and increase the usage count without resuming the device.
	 */
	pm_runtime_set_active(sensor->dev);
	pm_runtime_get_noresume(sensor->dev);
	pm_runtime_enable(sensor->dev);

	ret = v4l2_async_register_subdev_sensor(&sensor->subdev);
	if (ret < 0)
		goto err_pm;

	/*
	 * Finally, enable autosuspend and decrease the usage count. The device
	 * will get suspended after the autosuspend delay, turning the power
	 * off.
	 */
	pm_runtime_set_autosuspend_delay(sensor->dev, 1000);
	pm_runtime_use_autosuspend(sensor->dev);
	pm_runtime_put_autosuspend(sensor->dev);

	return 0;

err_pm:
	pm_runtime_disable(sensor->dev);
	pm_runtime_put_noidle(sensor->dev);
	imx415_subdev_cleanup(sensor);
err_power:
	imx415_power_off(sensor);
	return ret;
}

static void imx415_remove(struct i2c_client *client)
{
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	struct imx415 *sensor = to_imx415(subdev);

	v4l2_async_unregister_subdev(subdev);

	imx415_subdev_cleanup(sensor);

	/*
	 * Disable runtime PM. In case runtime PM is disabled in the kernel,
	 * make sure to turn power off manually.
	 */
	pm_runtime_disable(sensor->dev);
	if (!pm_runtime_status_suspended(sensor->dev))
		imx415_power_off(sensor);
	pm_runtime_set_suspended(sensor->dev);
}

static int imx415_runtime_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	struct imx415 *sensor = to_imx415(subdev);

	return imx415_power_on(sensor);
}

static int imx415_runtime_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	struct imx415 *sensor = to_imx415(subdev);

	imx415_power_off(sensor);

	return 0;
}

static const struct dev_pm_ops imx415_pm_ops = {
	SET_RUNTIME_PM_OPS(imx415_runtime_suspend, imx415_runtime_resume, NULL)
};

static const struct of_device_id imx415_of_match[] = {
	{ .compatible = "sony,imx415" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, imx415_of_match);

static struct i2c_driver imx415_driver = {
	.probe = imx415_probe,
	.remove = imx415_remove,
	.driver = {
		.name = "imx415",
		.of_match_table = imx415_of_match,
		.pm = &imx415_pm_ops,
	},
};

module_i2c_driver(imx415_driver);

MODULE_DESCRIPTION("Sony IMX415 image sensor driver");
MODULE_AUTHOR("Gerald Loacker <gerald.loacker@wolfvision.net>");
MODULE_AUTHOR("Michael Riesch <michael.riesch@wolfvision.net>");
MODULE_LICENSE("GPL");
