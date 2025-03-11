// SPDX-License-Identifier: GPL-2.0
/* Driver for VD55FLOOD ToF module
 *
 * Copyright (C) 2023 STMicroelectronics SA
 */

#include <linux/version.h>

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>

#include <asm/unaligned.h>

#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

/* Backward compatibility */
#if KERNEL_VERSION(5, 18, 0) >= LINUX_VERSION_CODE
#define MIPI_CSI2_DT_RAW12	0x2c
#else
#include <media/mipi-csi2.h>
#endif

#if KERNEL_VERSION(5, 15, 0) >= LINUX_VERSION_CODE
#define HZ_PER_MHZ		1000000UL
#else
#include <linux/units.h>
#endif

#define VD55FLOOD_REG_8BIT(n)			((1 << 16) | (n))
#define VD55FLOOD_REG_16BIT(n)			((2 << 16) | (n))
#define VD55FLOOD_REG_32BIT(n)			((4 << 16) | (n))
#define VD55FLOOD_REG_SIZE_SHIFT		16
#define VD55FLOOD_REG_ADDR_MASK			0xffff

#define VD55FLOOD_REG_MODEL_ID			VD55FLOOD_REG_16BIT(0x0000)
#define VD55FLOOD_MODEL_ID			0x4831
#define VD55FLOOD_REG_ROM_REV			VD55FLOOD_REG_16BIT(0x000a)
#define VD55FLOOD_REG_FWPATCH_START_ADDR	VD55FLOOD_REG_8BIT(0x1000)
#define VD55FLOOD_REG_SYSTEM_FSM		VD55FLOOD_REG_8BIT(0x0054)
#define VD55FLOOD_SYSTEM_FSM_UP			0x01
#define VD55FLOOD_SYSTEM_FSM_BOOT		0x04
#define VD55FLOOD_SYSTEM_FSM_SW_STBY		0x07
#define VD55FLOOD_SYSTEM_FSM_STREAMING		0x08
#define VD55FLOOD_REG_BOOT_FSM			VD55FLOOD_REG_8BIT(0x0055)
#define VD55FLOOD_BOOT_FSM_COMPLETE		0xbc
#define VD55FLOOD_REG_BOOT			VD55FLOOD_REG_8BIT(0x0400)
#define VD55FLOOD_BOOT_START			BIT(0)
#define VD55FLOOD_BOOT_FW_UPLOADED		BIT(1)
#define VD55FLOOD_BOOT_END			BIT(7)
#define VD55FLOOD_REG_STREAMING			VD55FLOOD_REG_8BIT(0x0404)
#define VD55FLOOD_STREAMING_STOP		BIT(0)
#define VD55FLOOD_STREAMING_START		BIT(1)
#define VD55FLOOD_REG_EXT_CLOCK			VD55FLOOD_REG_32BIT(0x0440)
#define VD55FLOOD_EXT_CLOCK_12MHZ		(12 * HZ_PER_MHZ)
#define VD55FLOOD_REG_VT_CTRL			VD55FLOOD_REG_16BIT(0x051f)
#define VD55FLOOD_REG_OIF_CTRL			VD55FLOOD_REG_16BIT(0x0526)
#define VD55FLOOD_REG_CLK_PLL_MIPI		VD55FLOOD_REG_16BIT(0x045C)
#define VD55FLOOD_REG_MAGIC_BYPASS		VD55FLOOD_REG_32BIT(0x0f28)
#define VD55FLOOD_REG_SAFETY_WORKAROUND		VD55FLOOD_REG_8BIT(0xbc44)
#define VD55FLOOD_REG_FWPATCH_REVISION		VD55FLOOD_REG_16BIT(0x000C)
#define VD55FLOOD_REG_ERROR_CODE		VD55FLOOD_REG_16BIT(0x00B0)
#define VD55FLOOD_REG_SPI_START_ADDRESS		VD55FLOOD_REG_8BIT(0x0418)
#define VD55FLOOD_REG_SPI_NB_OF_WORDS		VD55FLOOD_REG_8BIT(0x041a)
#define VD55FLOOD_REG_STBY			VD55FLOOD_REG_8BIT(0x0402)
#define VD55FLOOD_STBY_SPI_WRITE		BIT(7)
#define VD55FLOOD_REG_LASER_DRIVER_BASE		VD55FLOOD_REG_8BIT(0x0900)
#define VD55FLOOD_REG_FORMAT_CTRL		VD55FLOOD_REG_8BIT(0x0524)
#define VD55FLOOD_FORMAT_CTRL_TAP_MERGED	BIT(2)
#define VD55FLOOD_FORMAT_CTRL_RAW_12BITS	BIT(1)
#define VD55FLOOD_FORMAT_CTRL_SLC_SUPER_FRAME	BIT(0)
#define VD55FLOOD_REG_NB_FRAME_SEQUENCE		VD55FLOOD_REG_8BIT(0x0507)
#define VD55FLOOD_REG_FRAME_COMPOSITION(ctx) \
	VD55FLOOD_REG_8BIT(0x0580 + VD55FLOOD_CTX_STATICS_OFFSET * (ctx))
#define VD55FLOOD_FRAME_COMPOSITION_VM_3D_RAW	0x20
#define VD55FLOOD_REG_LVDS_DUTY_CYCLE(ctx) \
	VD55FLOOD_REG_8BIT(0x0582 + VD55FLOOD_CTX_STATICS_OFFSET * (ctx))
#define VD55FLOOD_LVDS_DUTY_CYCLE_180		0xc
#define VD55FLOOD_LVDS_DUTY_CYCLE_FALL_SHIFT	8
#define VD55FLOOD_REG_CONTEXT_REPEAT_COUNT(ctx) \
	VD55FLOOD_REG_8BIT(0x0586 + VD55FLOOD_CTX_STATICS_OFFSET * (ctx))
#define VD55FLOOD_REG_NEXT_CONTEXT(ctx) \
	VD55FLOOD_REG_8BIT(0x0587 + VD55FLOOD_CTX_STATICS_OFFSET * (ctx))
#define VD55FLOOD_REG_FRAME_LENGTH(ctx) \
	VD55FLOOD_REG_16BIT(0x0640 + VD55FLOOD_CTX_OFFSET * (ctx))
#define VD55FLOOD_REG_EXPOSURE_TIME_LONG(ctx) \
	VD55FLOOD_REG_32BIT(0x0644 + VD55FLOOD_CTX_OFFSET * (ctx))
#define VD55FLOOD_EXPOSURE_TIME_LONG_COARSE_SHIFT	16
#define VD55FLOOD_EXPOSURE_TIME_LONG_FINE_SHIFT	0
#define VD55FLOOD_REG_ITOF_CLK_TARGET(ctx) \
	VD55FLOOD_REG_32BIT(0x064c + VD55FLOOD_CTX_OFFSET * (ctx))
#define VD55FLOOD_REG_GLOBAL_PHASE_OFFSET(ctx) \
	VD55FLOOD_REG_8BIT(0x0650 + VD55FLOOD_CTX_OFFSET * (ctx))
#define VD55FLOOD_GLOBAL_PHASE_OFFSET_15_DEG	0x1
#define VD55FLOOD_REG_SIGNALS_CTRL		VD55FLOOD_REG_32BIT(0x0520)
#define VD55FLOOD_SIGNALS_CTRL_LD_ENABLE	0x50
#define VD55FLOOD_SIGNALS_CTRL_LD_READY		0x60
#define VD55FLOOD_SIGNALS_CTRL_HALT_WARNING_TRACKER	0xd3
#define VD55FLOOD_REG_GPIO_CONFIG		VD55FLOOD_REG_8BIT(0x0403)
#define VD55FLOOD_GPIO_CONFIG_UPDATE		0x1
#define VD55FLOOD_REG_LASER_ID			VD55FLOOD_REG_32BIT(0x0500)
#define VD55FLOOD_REG_LASER_SAFETY_35		VD55FLOOD_REG_16BIT(0xf7)

#define VD55FLOOD_NVMEM_REG_ID			0x0
#define VD55FLOOD_NVMEM_REG_BASE		0x0
#define VD55FLOOD_NVMEM_ID_VD55FLOOD		0x2082
#define VD55FLOOD_NVMEM_ID_VD55DOT		0x2c82

#define VD55FLOOD_WIDTH				1344
#define VD55FLOOD_HEIGHT			7236
#define VD55FLOOD_STATUS_LINES_NB		8
#define VD55FLOOD_DEFAULT_MODE			1
#define VD55FLOOD_WRITE_MULTIPLE_CHUNK_MAX	16
#define VD55FLOOD_NB_POLARITIES			5
#define VD55FLOOD_TIMEOUT_MS			500
#define VD55FLOOD_LD_NVMEM_DATA_SIZE		1056
#define VD55FLOOD_CTX_STATICS_OFFSET		0x20
#define VD55FLOOD_CTX_OFFSET			0x30
#define VD55FLOOD_BIN_MODE_SHIFT		4
#define VD55FLOOD_MEDIA_BUS_FMT_DEF		MEDIA_BUS_FMT_SGBRG12_1X12

#define V4L2_CID_FREQ_NB			(V4L2_CID_USER_BASE | 0x1020)

#include "st-vd55flood_patch.c"

enum vd55flood_rom_rev {
	VD55FLOOD_ROM_REV_B0 = 0x200,
	VD55FLOOD_ROM_REV_D0 = 0x302,
};

struct vd55flood_dev {
	struct i2c_client *i2c_client;
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct gpio_desc *reset_gpio;
	struct gpio_descs *supplies_gpios;
	struct clk *xclk;
	u32 clk_freq;
	u16 oif_ctrl;
	int nb_of_lane;
	int data_rate_in_mbps;
	enum vd55flood_rom_rev rev;
	struct {
		struct i2c_client *i2c_client;
		u8 nvmem_data[VD55FLOOD_LD_NVMEM_DATA_SIZE];
	} ld;
	/* Lock to protect all members below */
	struct mutex lock;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl *pixel_rate_ctrl;
	struct v4l2_ctrl *freq_nb_ctrl;
	struct v4l2_mbus_framefmt fmt;
	bool streaming;
	const struct vd55flood_mode_info *current_mode;
};

static const s64 link_freq[] = {
	/*
	 * MIPI output freq is 804Mhz / 2, as it uses both rising edge and
	 * falling edges to send data
	 */
	402000000ULL
};

enum vd55flood_bin_mode {
	VD55FLOOD_BIN_MODE_NORMAL,
	VD55FLOOD_BIN_MODE_DIGITAL_X2,
};

struct vd55flood_reg {
	u32 address;
	u32 val;
};

struct vd55flood_reg_list {
	unsigned int num_of_regs;
	const struct vd55flood_reg *regs;
};

struct vd55flood_mode_info {
	u32 width;
	u32 height;
	enum vd55flood_bin_mode bin_mode;
	u8 freq_nb;
	struct vd55flood_reg_list reg_list;
};

struct vd55flood_fmt_desc {
	u32 code;
	u8 bpp;
	u8 data_type;
};

static const struct vd55flood_fmt_desc vd55flood_supported_codes[] = {
	/* Sensor supports 1X10, but only 1X12 has been tested */
	{
		.code = MEDIA_BUS_FMT_Y12_1X12,
		.bpp = 12,
		.data_type = MIPI_CSI2_DT_RAW12,
	},
	/*
	 * The sensor is monochrome, but on mainline kernels Y12P is not
	 * supported, only on Raspberry Pi's. Trick it by sending it a bayer
	 * format instead.
	 */
	{
		.code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.bpp = 12,
		.data_type = MIPI_CSI2_DT_RAW12,
	},
};

static const struct vd55flood_reg mode_1freq_regs[] = {
	{VD55FLOOD_REG_FORMAT_CTRL,
		VD55FLOOD_FORMAT_CTRL_TAP_MERGED |
		VD55FLOOD_FORMAT_CTRL_RAW_12BITS |
		VD55FLOOD_FORMAT_CTRL_SLC_SUPER_FRAME},
	{VD55FLOOD_REG_NB_FRAME_SEQUENCE, 1},
	/* Setup context 0 */
	{VD55FLOOD_REG_FRAME_COMPOSITION(0),
		VD55FLOOD_FRAME_COMPOSITION_VM_3D_RAW},
	{VD55FLOOD_REG_LVDS_DUTY_CYCLE(0),
		VD55FLOOD_LVDS_DUTY_CYCLE_180 <<
		VD55FLOOD_LVDS_DUTY_CYCLE_FALL_SHIFT},
	{VD55FLOOD_REG_CONTEXT_REPEAT_COUNT(0), 1},
	{VD55FLOOD_REG_NEXT_CONTEXT(0), 0},
	{VD55FLOOD_REG_FRAME_LENGTH(0), 2368},
	{VD55FLOOD_REG_EXPOSURE_TIME_LONG(1),
		32 << VD55FLOOD_EXPOSURE_TIME_LONG_COARSE_SHIFT |
		0 << VD55FLOOD_EXPOSURE_TIME_LONG_FINE_SHIFT},
	{VD55FLOOD_REG_ITOF_CLK_TARGET(0), 20000000},
	{VD55FLOOD_REG_GLOBAL_PHASE_OFFSET(0),
		VD55FLOOD_GLOBAL_PHASE_OFFSET_15_DEG}
	/* Global offset TX is 0 on reset */
};

static const struct vd55flood_reg mode_2freq_regs[] = {
	{VD55FLOOD_REG_FORMAT_CTRL,
		VD55FLOOD_FORMAT_CTRL_TAP_MERGED |
		VD55FLOOD_FORMAT_CTRL_RAW_12BITS |
		VD55FLOOD_FORMAT_CTRL_SLC_SUPER_FRAME},
	{VD55FLOOD_REG_NB_FRAME_SEQUENCE, 2},
	/* Setup context 0 */
	{VD55FLOOD_REG_FRAME_COMPOSITION(0),
		VD55FLOOD_FRAME_COMPOSITION_VM_3D_RAW},
	{VD55FLOOD_REG_LVDS_DUTY_CYCLE(0),
		VD55FLOOD_LVDS_DUTY_CYCLE_180 <<
		VD55FLOOD_LVDS_DUTY_CYCLE_FALL_SHIFT},
	{VD55FLOOD_REG_CONTEXT_REPEAT_COUNT(0), 1},
	{VD55FLOOD_REG_NEXT_CONTEXT(0), 1},
	{VD55FLOOD_REG_FRAME_LENGTH(0), 1482},
	{VD55FLOOD_REG_EXPOSURE_TIME_LONG(0),
		32 << VD55FLOOD_EXPOSURE_TIME_LONG_COARSE_SHIFT |
		0 << VD55FLOOD_EXPOSURE_TIME_LONG_FINE_SHIFT},
	{VD55FLOOD_REG_ITOF_CLK_TARGET(0), 80000000},
	{VD55FLOOD_REG_GLOBAL_PHASE_OFFSET(0),
		VD55FLOOD_GLOBAL_PHASE_OFFSET_15_DEG},
	/* Setup context 1 */
	{VD55FLOOD_REG_FRAME_COMPOSITION(1),
		VD55FLOOD_FRAME_COMPOSITION_VM_3D_RAW},
	{VD55FLOOD_REG_LVDS_DUTY_CYCLE(1),
		VD55FLOOD_LVDS_DUTY_CYCLE_180 <<
		VD55FLOOD_LVDS_DUTY_CYCLE_FALL_SHIFT},
	{VD55FLOOD_REG_CONTEXT_REPEAT_COUNT(1), 1},
	{VD55FLOOD_REG_NEXT_CONTEXT(1), 0},
	{VD55FLOOD_REG_FRAME_LENGTH(1), 3252},
	{VD55FLOOD_REG_EXPOSURE_TIME_LONG(1),
		32 << VD55FLOOD_EXPOSURE_TIME_LONG_COARSE_SHIFT |
		0 << VD55FLOOD_EXPOSURE_TIME_LONG_FINE_SHIFT},
	{VD55FLOOD_REG_ITOF_CLK_TARGET(1), 60000000},
	{VD55FLOOD_REG_GLOBAL_PHASE_OFFSET(1),
		VD55FLOOD_GLOBAL_PHASE_OFFSET_15_DEG},
};

/*
 * This is a degraded 3F mode and differs from the recommended one since
 * hardware stack only supports 2 lanes and therefore can't output at the same
 * bandwidth. Recommended mode frame lengths are 1482 14782 1770.
 * FIXME: Change to recommended 3F mode once we have hardware to test it.
 */
static const struct vd55flood_reg mode_3freq_regs[] = {
	{VD55FLOOD_REG_FORMAT_CTRL,
		VD55FLOOD_FORMAT_CTRL_TAP_MERGED |
		VD55FLOOD_FORMAT_CTRL_RAW_12BITS |
		VD55FLOOD_FORMAT_CTRL_SLC_SUPER_FRAME},
	{VD55FLOOD_REG_NB_FRAME_SEQUENCE, 3},
	/* Setup context 0 */
	{VD55FLOOD_REG_FRAME_COMPOSITION(0),
		VD55FLOOD_FRAME_COMPOSITION_VM_3D_RAW},
	{VD55FLOOD_REG_LVDS_DUTY_CYCLE(0),
		VD55FLOOD_LVDS_DUTY_CYCLE_180 <<
		VD55FLOOD_LVDS_DUTY_CYCLE_FALL_SHIFT},
	{VD55FLOOD_REG_CONTEXT_REPEAT_COUNT(0), 1},
	{VD55FLOOD_REG_NEXT_CONTEXT(0), 1},
	{VD55FLOOD_REG_FRAME_LENGTH(0), 1482},
	{VD55FLOOD_REG_EXPOSURE_TIME_LONG(0),
		32 << VD55FLOOD_EXPOSURE_TIME_LONG_COARSE_SHIFT |
		0 << VD55FLOOD_EXPOSURE_TIME_LONG_FINE_SHIFT},
	{VD55FLOOD_REG_ITOF_CLK_TARGET(0), 200000000},
	{VD55FLOOD_REG_GLOBAL_PHASE_OFFSET(0),
		VD55FLOOD_GLOBAL_PHASE_OFFSET_15_DEG},
	/* Setup context 1 */
	{VD55FLOOD_REG_FRAME_COMPOSITION(1),
		VD55FLOOD_FRAME_COMPOSITION_VM_3D_RAW},
	{VD55FLOOD_REG_LVDS_DUTY_CYCLE(1),
		VD55FLOOD_LVDS_DUTY_CYCLE_180 <<
		VD55FLOOD_LVDS_DUTY_CYCLE_FALL_SHIFT},
	{VD55FLOOD_REG_CONTEXT_REPEAT_COUNT(1), 1},
	{VD55FLOOD_REG_NEXT_CONTEXT(1), 2},
	{VD55FLOOD_REG_FRAME_LENGTH(1), 1482},
	{VD55FLOOD_REG_EXPOSURE_TIME_LONG(1),
		32 << VD55FLOOD_EXPOSURE_TIME_LONG_COARSE_SHIFT |
		0 << VD55FLOOD_EXPOSURE_TIME_LONG_FINE_SHIFT},
	{VD55FLOOD_REG_ITOF_CLK_TARGET(1), 177777780},
	{VD55FLOOD_REG_GLOBAL_PHASE_OFFSET(1),
		VD55FLOOD_GLOBAL_PHASE_OFFSET_15_DEG},
	/* Setup context 2 */
	{VD55FLOOD_REG_FRAME_COMPOSITION(2),
		VD55FLOOD_FRAME_COMPOSITION_VM_3D_RAW},
	{VD55FLOOD_REG_LVDS_DUTY_CYCLE(2),
		VD55FLOOD_LVDS_DUTY_CYCLE_180 <<
		VD55FLOOD_LVDS_DUTY_CYCLE_FALL_SHIFT},
	{VD55FLOOD_REG_CONTEXT_REPEAT_COUNT(2), 1},
	{VD55FLOOD_REG_NEXT_CONTEXT(2), 0},
	{VD55FLOOD_REG_FRAME_LENGTH(2), 2262},
	{VD55FLOOD_REG_EXPOSURE_TIME_LONG(2),
		32 << VD55FLOOD_EXPOSURE_TIME_LONG_COARSE_SHIFT |
		0 << VD55FLOOD_EXPOSURE_TIME_LONG_FINE_SHIFT},
	{VD55FLOOD_REG_ITOF_CLK_TARGET(2), 133333330},
	{VD55FLOOD_REG_GLOBAL_PHASE_OFFSET(2),
		VD55FLOOD_GLOBAL_PHASE_OFFSET_15_DEG},
};

static const struct vd55flood_mode_info vd55flood_mode_data[] = {
	/* Full resolution modes */
	{
		.width = VD55FLOOD_WIDTH,
		.height = 2412 + VD55FLOOD_STATUS_LINES_NB,
		.bin_mode = VD55FLOOD_BIN_MODE_NORMAL,
		.freq_nb = 1,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_1freq_regs),
			.regs = mode_1freq_regs,
		},
	},
	{
		.width = VD55FLOOD_WIDTH,
		.height = 4824 + VD55FLOOD_STATUS_LINES_NB,
		.bin_mode = VD55FLOOD_BIN_MODE_NORMAL,
		.freq_nb = 2,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_2freq_regs),
			.regs = mode_2freq_regs,
		},
	},
	{
		.width = VD55FLOOD_WIDTH,
		.height = VD55FLOOD_HEIGHT + VD55FLOOD_STATUS_LINES_NB,
		.bin_mode = VD55FLOOD_BIN_MODE_NORMAL,
		.freq_nb = 3,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_3freq_regs),
			.regs = mode_3freq_regs,
		},
	},
	/* Binned modes */
	{
		.width = VD55FLOOD_WIDTH / 2,
		.height = 2412 + VD55FLOOD_STATUS_LINES_NB,
		.bin_mode = VD55FLOOD_BIN_MODE_DIGITAL_X2,
		.freq_nb = 2,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_2freq_regs),
			.regs = mode_2freq_regs,
		},
	},
	{
		.width = VD55FLOOD_WIDTH / 2,
		.height = 3618 + VD55FLOOD_STATUS_LINES_NB,
		.bin_mode = VD55FLOOD_BIN_MODE_DIGITAL_X2,
		.freq_nb = 3,
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_3freq_regs),
			.regs = mode_3freq_regs,
		},
	},
};

static inline struct vd55flood_dev *to_vd55flood_dev(struct v4l2_subdev *sd)
{
	return container_of(sd, struct vd55flood_dev, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct vd55flood_dev,
		ctrl_handler)->sd;
}

static u8 get_bpp_by_code(__u32 code)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(vd55flood_supported_codes); i++) {
		if (vd55flood_supported_codes[i].code == code)
			return vd55flood_supported_codes[i].bpp;
	}
	/* Should never happen */
	WARN(1, "Unsupported code %d. default to 12 bpp", code);
	return 12;
}

static s32 get_pixel_rate(struct vd55flood_dev *sensor)
{
	int pr = div64_u64((u64)sensor->data_rate_in_mbps * sensor->nb_of_lane,
			 get_bpp_by_code(sensor->fmt.code));
	return pr;
}

static int get_chunk_size(struct i2c_client *client)
{
	int max_write_len = VD55FLOOD_WRITE_MULTIPLE_CHUNK_MAX;
	struct i2c_adapter *adapter = client->adapter;

	if (adapter->quirks && adapter->quirks->max_write_len)
		max_write_len = adapter->quirks->max_write_len - 2;

	max_write_len = min(max_write_len, VD55FLOOD_WRITE_MULTIPLE_CHUNK_MAX);

	return max(max_write_len, 1);
}

static int vd55flood_read_multiple(struct i2c_client *client, u32 reg,
				   u8 *data, unsigned int len, int *err)
{
	struct i2c_msg msg[2];
	u8 buf[2];
	int ret;

	if (err && *err)
		return *err;

	if (len > VD55FLOOD_WRITE_MULTIPLE_CHUNK_MAX)
		return -EINVAL;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	msg[0].addr = client->addr;
	msg[0].flags = client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr = client->addr;
	msg[1].flags = client->flags | I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = len;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		dev_dbg(&client->dev, "%s: %x i2c_transfer, reg: %x => %d\n",
			__func__, client->addr, reg, ret);
		if (err)
			*err = ret;
		return ret;
	}

	return 0;
}

static inline int vd55flood_read_reg(struct i2c_client *client, u32 reg,
				     u32 *val)
{
	*val = 0;

	return vd55flood_read_multiple(client, reg & VD55FLOOD_REG_ADDR_MASK,
				       (u8 *)val,
				       (reg >> VD55FLOOD_REG_SIZE_SHIFT) & 7,
				       NULL);
}

static int vd55flood_write_multiple(struct vd55flood_dev *sensor, u32 reg,
				    const u8 *data, unsigned int len, int *err)
{
	struct i2c_client *client = sensor->i2c_client;
	struct i2c_msg msg;
	u8 buf[VD55FLOOD_WRITE_MULTIPLE_CHUNK_MAX + 2];
	unsigned int i;
	int ret;

	if (err && *err)
		return *err;

	if (len > VD55FLOOD_WRITE_MULTIPLE_CHUNK_MAX)
		return -EINVAL;
	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	for (i = 0; i < len; i++)
		buf[i + 2] = data[i];

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = len + 2;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_dbg(&client->dev, "%s: i2c_transfer, reg: %x => %d\n",
			__func__, reg, ret);
		if (err)
			*err = ret;
		return ret;
	}

	return 0;
}

static int vd55flood_write_array(struct vd55flood_dev *sensor, u32 reg,
				 unsigned int nb, const u8 *array, int *err)
{
	const unsigned int chunk_size = get_chunk_size(sensor->i2c_client);
	int ret;
	unsigned int sz;

	if (err && *err)
		return *err;

	while (nb) {
		sz = min(nb, chunk_size);
		ret = vd55flood_write_multiple(sensor, reg, array, sz, NULL);
		if (ret < 0) {
			if (err)
				*err = ret;
			return ret;
		}
		nb -= sz;
		reg += sz;
		array += sz;
	}

	return 0;
}

static inline int vd55flood_write_reg(struct vd55flood_dev *sensor, u32 reg,
				      u32 val, int *err)
{
	return vd55flood_write_multiple(sensor, reg & VD55FLOOD_REG_ADDR_MASK,
					(u8 *)&val,
					(reg >> VD55FLOOD_REG_SIZE_SHIFT) & 7,
					err);
}

static int vd55flood_write_regs(struct vd55flood_dev *sensor,
				const struct vd55flood_reg *regs,
				unsigned int len, int *err)
{
	int i, ret;

	if (err && *err)
		return *err;

	for (i = 0; i < len; i++) {
		ret = vd55flood_write_reg(sensor, regs[i].address, regs[i].val,
					  NULL);
		if (ret < 0) {
			if (err)
				*err = ret;
			return ret;
		}
	}

	return 0;
}

static int vd55flood_poll_reg(struct vd55flood_dev *sensor, u32 reg,
			      u8 poll_val, unsigned int timeout_ms)
{
	const unsigned int loop_delay_ms = 10;
	int ret = 0;
	u32 val = 0;
#if KERNEL_VERSION(5, 7, 0) >= LINUX_VERSION_CODE
	int loop_nb = timeout_ms / loop_delay_ms;

	while (--loop_nb) {
		ret = vd55flood_read_reg(sensor->i2c_client, reg, &val);
		if (ret)
			return ret;
		if (val == poll_val)
			return 0;
		usleep_range(loop_delay_ms * 1000, loop_delay_ms * 2 * 1000);
	}
	return -ETIMEDOUT;
#else
	return read_poll_timeout(vd55flood_read_reg, ret,
				 (ret || (val == poll_val)),
				 loop_delay_ms * 1000, timeout_ms * 1000,
				 false, sensor->i2c_client, reg, &val);
#endif
}

static int vd55flood_wait_state(struct vd55flood_dev *sensor, int state,
				unsigned int timeout_ms)
{
	return vd55flood_poll_reg(sensor, VD55FLOOD_REG_SYSTEM_FSM, state,
			       timeout_ms);
}

static int vd55flood_apply_reset(struct vd55flood_dev *sensor)
{
	gpiod_set_value_cansleep(sensor->reset_gpio, 1);
	usleep_range(5000, 10000);
	gpiod_set_value_cansleep(sensor->reset_gpio, 0);
	usleep_range(5000, 10000);
	gpiod_set_value_cansleep(sensor->reset_gpio, 1);
	usleep_range(40000, 100000);
	return vd55flood_wait_state(sensor, VD55FLOOD_SYSTEM_FSM_UP,
				 VD55FLOOD_TIMEOUT_MS);
}

static void vd55flood_fill_framefmt(struct vd55flood_dev *sensor,
				    const struct vd55flood_mode_info *mode,
				    struct v4l2_mbus_framefmt *fmt, u32 code)
{
	fmt->code = code;
	fmt->width = mode->width;
	fmt->height = mode->height;
	fmt->colorspace = V4L2_COLORSPACE_RAW;
	fmt->field = V4L2_FIELD_NONE;
	fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	fmt->quantization = V4L2_QUANTIZATION_DEFAULT;
	fmt->xfer_func = V4L2_XFER_FUNC_DEFAULT;
}

static int vd55flood_try_fmt_internal(struct v4l2_subdev *sd,
				      struct v4l2_mbus_framefmt *fmt,
				      const struct vd55flood_mode_info
				      **new_mode)
{
	struct vd55flood_dev *sensor = to_vd55flood_dev(sd);
	const struct vd55flood_mode_info *mode = vd55flood_mode_data;
	unsigned int index;

	for (index = 0; index < ARRAY_SIZE(vd55flood_supported_codes);
	     index++) {
		if (vd55flood_supported_codes[index].code == fmt->code)
			break;
	}
	if (index == ARRAY_SIZE(vd55flood_supported_codes))
		index = 0;

	mode = v4l2_find_nearest_size(vd55flood_mode_data,
				      ARRAY_SIZE(vd55flood_mode_data), width,
				      height, fmt->width, fmt->height);
	if (new_mode)
		*new_mode = mode;

	vd55flood_fill_framefmt(sensor, mode, fmt,
				vd55flood_supported_codes[index].code);

	return 0;
}

static int vd55flood_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
	struct vd55flood_dev *sensor = to_vd55flood_dev(sd);
	int ret;

	switch (ctrl->id) {
	case V4L2_CID_PIXEL_RATE:
		ret = __v4l2_ctrl_s_ctrl_int64(ctrl, get_pixel_rate(sensor));
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int vd55flood_s_ctrl(struct v4l2_ctrl *ctrl)
{
	return 0;
}

static const struct v4l2_ctrl_ops vd55flood_ctrl_ops = {
	.g_volatile_ctrl = vd55flood_g_volatile_ctrl,
	.s_ctrl = vd55flood_s_ctrl,
};

static const struct v4l2_ctrl_config vd55flood_freq_nb_ctrl = {
	.ops		= &vd55flood_ctrl_ops,
	.id		= V4L2_CID_FREQ_NB,
	.name		= "Number of frequencies",
	.type		= V4L2_CTRL_TYPE_INTEGER,
	.min		= 1,
	.max		= 3,
	.step		= 1,
	.def		= 1,
};

static int vd55flood_init_controls(struct vd55flood_dev *sensor)
{
	const struct v4l2_ctrl_ops *ops = &vd55flood_ctrl_ops;
	struct v4l2_ctrl_handler *hdl = &sensor->ctrl_handler;
	struct v4l2_ctrl *ctrl;
	int ret;

	v4l2_ctrl_handler_init(hdl, 16);
	/* we can use our own mutex for the ctrl lock */
	hdl->lock = &sensor->lock;
	ctrl = v4l2_ctrl_new_int_menu(hdl, ops, V4L2_CID_LINK_FREQ,
				      ARRAY_SIZE(link_freq) - 1, 0, link_freq);
	ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	/*
	 * Keep a pointer to these controls as we need to update them when
	 * setting the format
	 */
	sensor->pixel_rate_ctrl = v4l2_ctrl_new_std(hdl, ops,
						    V4L2_CID_PIXEL_RATE, 1,
						    INT_MAX, 1,
						    get_pixel_rate(sensor));
	if (sensor->pixel_rate_ctrl)
		sensor->pixel_rate_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	sensor->freq_nb_ctrl = v4l2_ctrl_new_custom(hdl,
						    &vd55flood_freq_nb_ctrl,
						    NULL);
	if (sensor->freq_nb_ctrl)
		sensor->freq_nb_ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	if (hdl->error) {
		ret = hdl->error;
		goto free_ctrls;
	}

	sensor->sd.ctrl_handler = hdl;

	__v4l2_ctrl_s_ctrl(sensor->freq_nb_ctrl, sensor->current_mode->freq_nb);

	return 0;

free_ctrls:
	v4l2_ctrl_handler_free(hdl);
	return ret;
}

static int vd55flood_detect(struct vd55flood_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	int ret;
	u32 id;

	ret = vd55flood_read_reg(sensor->i2c_client, VD55FLOOD_REG_MODEL_ID,
				 &id);
	if (ret)
		return id;

	if (id != VD55FLOOD_MODEL_ID) {
		dev_warn(&client->dev, "Unsupported sensor id %x", id);
		return -ENODEV;
	}

	return 0;
}

static int __maybe_unused vd55flood_dump_laser_error
		(struct vd55flood_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	u32 val;
	int ret;

	ret = vd55flood_read_reg(sensor->i2c_client,
				 VD55FLOOD_REG_LASER_SAFETY_35,
				 &val);
	if (ret)
		return ret;

	dev_dbg(&client->dev, "laser safety error (0x%x) : 0x%x\n",
		(VD55FLOOD_REG_LASER_SAFETY_35 & 7), val);

	return 0;
}

static int vd55flood_apply_settings(struct vd55flood_dev *sensor)
{
	int ret = 0;
	u32 vt_ctrl;

	ret = vd55flood_read_reg(sensor->i2c_client, VD55FLOOD_REG_VT_CTRL,
				 &vt_ctrl);
	if (ret)
		return ret;
	vt_ctrl = (vt_ctrl & ~(~0 << VD55FLOOD_BIN_MODE_SHIFT)) |
		  (sensor->current_mode->bin_mode << VD55FLOOD_BIN_MODE_SHIFT);

	vd55flood_write_reg(sensor, VD55FLOOD_REG_EXT_CLOCK, sensor->clk_freq,
			    &ret);
	vd55flood_write_reg(sensor, VD55FLOOD_REG_OIF_CTRL, sensor->oif_ctrl,
			    &ret);
	vd55flood_write_reg(sensor, VD55FLOOD_REG_VT_CTRL, vt_ctrl, &ret);
	vd55flood_write_regs(sensor, sensor->current_mode->reg_list.regs,
			     sensor->current_mode->reg_list.num_of_regs, &ret);
	vd55flood_write_reg(sensor, VD55FLOOD_REG_SIGNALS_CTRL,
			    VD55FLOOD_SIGNALS_CTRL_LD_ENABLE |
			    VD55FLOOD_SIGNALS_CTRL_LD_READY << 8 |
			    VD55FLOOD_SIGNALS_CTRL_HALT_WARNING_TRACKER << 24,
			    &ret);
	vd55flood_write_reg(sensor, VD55FLOOD_REG_GPIO_CONFIG,
			    VD55FLOOD_GPIO_CONFIG_UPDATE, &ret);
	if (ret)
		return ret;

	ret = vd55flood_poll_reg(sensor, VD55FLOOD_REG_BOOT, 0,
				 VD55FLOOD_TIMEOUT_MS);
	if (ret)
		return ret;

	return 0;
}

static int vd55flood_stream_enable(struct vd55flood_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	int ret;

	ret = vd55flood_apply_settings(sensor);
	if (ret) {
		dev_err(&client->dev, "Error while applying sensor's settings\n");
		return -EINVAL;
	}

	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_STREAMING,
				  VD55FLOOD_STREAMING_START, NULL);
	if (ret) {
		dev_err(&client->dev, "Error while asking to stream\n");
		return -EINVAL;
	}

	ret = vd55flood_poll_reg(sensor, VD55FLOOD_REG_STREAMING, 0,
				 VD55FLOOD_TIMEOUT_MS);
	if (ret) {
		dev_err(&client->dev, "Error while waiting for stream state to go to 0\n");
		return -EINVAL;
	}

	ret = vd55flood_wait_state(sensor, VD55FLOOD_SYSTEM_FSM_STREAMING,
				   VD55FLOOD_TIMEOUT_MS);
	if (ret) {
		dev_err(&client->dev, "Error while waiting FSM\n");
		return -EINVAL;
	}

	return 0;
}

static int vd55flood_stream_disable(struct vd55flood_dev *sensor)
{
	int ret;

	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_STREAMING,
				  VD55FLOOD_STREAMING_STOP, NULL);
	if (ret)
		goto err_str_dis;

	ret = vd55flood_poll_reg(sensor, VD55FLOOD_REG_STREAMING, 0, 2000);
	if (ret)
		goto err_str_dis;

	ret = vd55flood_wait_state(sensor, VD55FLOOD_SYSTEM_FSM_SW_STBY,
				   VD55FLOOD_TIMEOUT_MS);

err_str_dis:
	if (ret)
		WARN(1, "Can't disable stream");

	return ret;
}

static int vd55flood_spi_write(struct vd55flood_dev *sensor, u16 start_addr,
			       size_t size, u8 *data, int *err)
{
	int ret = 0;

	if (err && *err)
		return *err;

	vd55flood_write_reg(sensor, VD55FLOOD_REG_SPI_START_ADDRESS, start_addr,
			    &ret);
	vd55flood_write_reg(sensor, VD55FLOOD_REG_SPI_NB_OF_WORDS, size, &ret);
	vd55flood_write_array(sensor,
			      VD55FLOOD_REG_LASER_DRIVER_BASE + start_addr,
			      size, data, &ret);
	vd55flood_write_reg(sensor, VD55FLOOD_REG_STBY,
			    VD55FLOOD_STBY_SPI_WRITE, &ret);
	if (ret) {
		if (err)
			*err = ret;
		return ret;
	}

	ret = vd55flood_poll_reg(sensor, VD55FLOOD_REG_STBY, 0,
				 VD55FLOOD_TIMEOUT_MS);
	if (ret) {
		if (err)
			*err = ret;
	}

	return ret;
}

static inline int vd55flood_spi_mirror(struct vd55flood_dev *sensor,
				       u16 start_addr, size_t size, int *err)
{
	return vd55flood_spi_write(sensor, start_addr, size,
				   (u8 *)&sensor->ld.nvmem_data[start_addr + 2],
				    err);
}

static int vd55flood_write_laser_driver_nvmem(struct vd55flood_dev *sensor)
{
	int ret = 0;
	u8 slave;

	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_LASER_ID, 0x010504,
				  NULL);
	if (ret)
		return ret;

	/* Reset the data of the slave registers */
	slave = 1;
	vd55flood_spi_write(sensor, 0x2d, 1, (u8 *)&slave, &ret);
	slave = 0;
	vd55flood_spi_write(sensor, 0x2d, 1, (u8 *)&slave, &ret);
	usleep_range(10000, 15000);

	/* Write laser driver nvmem */
	vd55flood_spi_mirror(sensor, 0x00, 63, &ret);
	vd55flood_spi_mirror(sensor, 0x40, 14, &ret);
	vd55flood_spi_mirror(sensor, 0x50,  4, &ret);
	vd55flood_spi_mirror(sensor, 0x55, 14, &ret);
	vd55flood_spi_mirror(sensor, 0x63, 10, &ret);

	return ret;
}

static int vd55flood_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct vd55flood_dev *sensor = to_vd55flood_dev(sd);
	int ret = 0;

	mutex_lock(&sensor->lock);

	ret = enable ? vd55flood_stream_enable(sensor) :
		       vd55flood_stream_disable(sensor);
	if (!ret)
		sensor->streaming = enable;

	mutex_unlock(&sensor->lock);

	return ret;
}

static int vd55flood_tx_from_ep(struct vd55flood_dev *sensor,
				struct fwnode_handle *handle)
{
	struct v4l2_fwnode_endpoint *ep;
	struct i2c_client *client = sensor->i2c_client;
	u32 log2phy[VD55FLOOD_NB_POLARITIES] = {~0, ~0, ~0, ~0, ~0};
	u32 phy2log[VD55FLOOD_NB_POLARITIES] = {~0, ~0, ~0, ~0, ~0};
	int polarities[VD55FLOOD_NB_POLARITIES] = {0, 0, 0, 0, 0};
	int l_nb;
	unsigned int p, l, i;

#if KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE
	ep = v4l2_fwnode_endpoint_alloc_parse(handle);
#else
	struct v4l2_fwnode_endpoint ep_node = { .bus_type =
		V4L2_MBUS_CSI2_DPHY
	};
	int ret;

	ep = &ep_node;
	ret = v4l2_fwnode_endpoint_alloc_parse(handle, ep);
	if (ret)
		return -EINVAL;
#endif

	l_nb = ep->bus.mipi_csi2.num_data_lanes;
	if (l_nb != 1 && l_nb != 2 && l_nb != 4) {
		dev_err(&client->dev, "invalid data lane number %d\n", l_nb);
		goto error_ep;
	}

	/* Build log2phy, phy2log and polarities from ep info */
	log2phy[0] = ep->bus.mipi_csi2.clock_lane;
	phy2log[log2phy[0]] = 0;
	for (l = 1; l < l_nb + 1; l++) {
		log2phy[l] = ep->bus.mipi_csi2.data_lanes[l - 1];
		phy2log[log2phy[l]] = l;
	}
	/*
	 * Then fill remaining slots for every physical slot to have something
	 * valid for hardware stuff.
	 */
	for (p = 0; p < VD55FLOOD_NB_POLARITIES; p++) {
		if (phy2log[p] != ~0)
			continue;
		phy2log[p] = l;
		log2phy[l] = p;
		l++;
	}
	for (l = 0; l < l_nb + 1; l++)
		polarities[l] = ep->bus.mipi_csi2.lane_polarities[l];

	if (log2phy[0] != 0) {
		dev_err(&client->dev, "clk lane must be map to physical lane 0\n");
		goto error_ep;
	}
	sensor->oif_ctrl = (polarities[4] << 15) + ((phy2log[4] - 1) << 13) +
			   (polarities[3] << 12) + ((phy2log[3] - 1) << 10) +
			   (polarities[2] <<  9) + ((phy2log[2] - 1) <<  7) +
			   (polarities[1] <<  6) + ((phy2log[1] - 1) <<  4) +
			   (polarities[0] <<  3) +
			   l_nb;
	sensor->nb_of_lane = l_nb;

	dev_dbg(&client->dev, "tx uses %d lanes", l_nb);
	for (i = 0; i < VD55FLOOD_NB_POLARITIES; i++) {
		dev_dbg(&client->dev, "log2phy[%d] = %d\n", i, log2phy[i]);
		dev_dbg(&client->dev, "phy2log[%d] = %d\n", i, phy2log[i]);
		dev_dbg(&client->dev, "polarity[%d] = %d\n", i, polarities[i]);
	}
	dev_dbg(&client->dev, "oif_ctrl = 0x%04x\n", sensor->oif_ctrl);

	v4l2_fwnode_endpoint_free(ep);

	return 0;

error_ep:
	v4l2_fwnode_endpoint_free(ep);

	return -EINVAL;
}

static int vd55flood_check_sensor_rev(struct vd55flood_dev *sensor)
{
	struct i2c_client *client = sensor->i2c_client;
	int rom_rev = 0;
	int ret;

	ret = vd55flood_read_reg(sensor->i2c_client,
				 VD55FLOOD_REG_ROM_REV, &rom_rev);
	if (ret) {
		dev_err(&client->dev, "Can't get sensor rom revision");
		return -ENODEV;
	}

	switch (rom_rev) {
	case VD55FLOOD_ROM_REV_B0:
		dev_dbg(&client->dev, "Sensor revision B0\n");
		break;
	case VD55FLOOD_ROM_REV_D0:
		dev_dbg(&client->dev, "Sensor revision D0\n");
		break;
	default:
		dev_err(&client->dev, "Unknown rom revision 0x%x", rom_rev);
		return -ENODEV;
	}

	sensor->rev = rom_rev;

	return 0;
}

static int vd55flood_setup(struct vd55flood_dev *sensor)
{
	int ret;

	/* Allow the device to be patchable*/
	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_EXT_CLOCK,
				  VD55FLOOD_EXT_CLOCK_12MHZ, NULL);
	if (ret)
		return ret;
	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_MAGIC_BYPASS,
				  0xdeadbeef, NULL);
	if (ret)
		return ret;
	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_SAFETY_WORKAROUND,
				  0x07, NULL);
	if (ret)
		return ret;

	/* Put the sensor in patch ready state */
	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_BOOT,
				  VD55FLOOD_BOOT_START, NULL);
	if (ret)
		return ret;

	ret = vd55flood_poll_reg(sensor, VD55FLOOD_REG_BOOT, 0,
				 VD55FLOOD_TIMEOUT_MS);
	if (ret)
		return ret;

	return vd55flood_wait_state(sensor, VD55FLOOD_SYSTEM_FSM_BOOT,
				   VD55FLOOD_TIMEOUT_MS);
}

static int vd55flood_patch(struct vd55flood_dev *sensor)
{
	int ret;

	if (sensor->rev == VD55FLOOD_ROM_REV_B0)
		ret = vd55flood_write_array(sensor,
					    VD55FLOOD_REG_FWPATCH_START_ADDR,
					    sizeof(patch_array_b0),
					    patch_array_b0,
					    NULL);
	else
		ret = vd55flood_write_array(sensor,
					    VD55FLOOD_REG_FWPATCH_START_ADDR,
					    sizeof(patch_array_d0),
					    patch_array_d0,
					    NULL);
	if (ret)
		return ret;

	return vd55flood_write_reg(sensor, VD55FLOOD_REG_BOOT,
				   VD55FLOOD_BOOT_FW_UPLOADED, NULL);
}

static int vd55flood_boot(struct vd55flood_dev *sensor)
{
	int ret;

	ret = vd55flood_write_reg(sensor, VD55FLOOD_REG_BOOT,
				  VD55FLOOD_BOOT_END, NULL);
	if (ret)
		return ret;

	ret = vd55flood_poll_reg(sensor, VD55FLOOD_REG_BOOT, 0,
				 VD55FLOOD_TIMEOUT_MS);
	if (ret)
		return ret;

	ret = vd55flood_poll_reg(sensor, VD55FLOOD_REG_BOOT_FSM,
				 VD55FLOOD_BOOT_FSM_COMPLETE,
				 VD55FLOOD_TIMEOUT_MS);
	if (ret)
		return ret;

	ret = vd55flood_wait_state(sensor, VD55FLOOD_SYSTEM_FSM_SW_STBY,
				   VD55FLOOD_TIMEOUT_MS);
	if (ret)
		return ret;

	return 0;
}

static int vd55flood_check_patch_version(struct vd55flood_dev *sensor,
					 int major, int minor)
{
	struct i2c_client *client = sensor->i2c_client;
	int ret;
	u32 patch;

	ret = vd55flood_read_reg(sensor->i2c_client,
				 VD55FLOOD_REG_FWPATCH_REVISION, &patch);
	if (ret)
		return patch;

	if (patch != (major << 8) + minor) {
		dev_err(&client->dev,
			"bad patch version expected %d.%d got %d.%d",
			major, minor, patch >> 8, patch & 0xff);
		return -ENODEV;
	}
	dev_dbg(&client->dev, "patch %d.%d applied", patch >> 8, patch & 0xff);

	return 0;
}

static int vd55flood_get_fmt(struct v4l2_subdev *sd,
#if KERNEL_VERSION(5, 14, 0) > LINUX_VERSION_CODE
			  struct v4l2_subdev_pad_config *cfg,
#else
			  struct v4l2_subdev_state *sd_state,
#endif
			  struct v4l2_subdev_format *format)
{
	struct vd55flood_dev *sensor = to_vd55flood_dev(sd);
	struct v4l2_mbus_framefmt *fmt;

	mutex_lock(&sensor->lock);

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
#if KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE
		fmt = v4l2_subdev_get_try_format(&sensor->sd, cfg,
						 format->pad);
#else
		fmt = v4l2_subdev_get_try_format(&sensor->sd, sd_state,
						 format->pad);
#endif
	else
		fmt = &sensor->fmt;

	format->format = *fmt;

	mutex_unlock(&sensor->lock);

	return 0;
}

static int vd55flood_get_selection(struct v4l2_subdev *sd,
#if KERNEL_VERSION(5, 15, 0) >= LINUX_VERSION_CODE
				struct v4l2_subdev_pad_config *cfg,
#else
				struct v4l2_subdev_state *sd_state,
#endif
				struct v4l2_subdev_selection *sel)
{
	struct vd55flood_dev *sensor = to_vd55flood_dev(sd);

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = sensor->current_mode->width;
		sel->r.height = sensor->current_mode->height;
		return 0;
	case V4L2_SEL_TGT_NATIVE_SIZE:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = VD55FLOOD_WIDTH;
		sel->r.height = VD55FLOOD_HEIGHT + VD55FLOOD_STATUS_LINES_NB;
		return 0;
	}

	return -EINVAL;
}

static int vd55flood_enum_mbus_code(struct v4l2_subdev *sd,
#if KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE
				 struct v4l2_subdev_pad_config *cfg,
#else
				 struct v4l2_subdev_state *sd_state,
#endif
				 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= ARRAY_SIZE(vd55flood_supported_codes))
		return -EINVAL;

	code->code = vd55flood_supported_codes[code->index].code;

	return 0;
}

static int vd55flood_set_fmt(struct v4l2_subdev *sd,
#if KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE
			  struct v4l2_subdev_pad_config *cfg,
#else
			  struct v4l2_subdev_state *sd_state,
#endif
			  struct v4l2_subdev_format *format)
{
	struct vd55flood_dev *sensor = to_vd55flood_dev(sd);
	const struct vd55flood_mode_info *new_mode;
	struct v4l2_mbus_framefmt *fmt;
	int ret;

	mutex_lock(&sensor->lock);

	if (sensor->streaming) {
		ret = -EBUSY;
		goto out;
	}

	ret = vd55flood_try_fmt_internal(sd, &format->format, &new_mode);
	if (ret)
		goto out;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY) {
#if KERNEL_VERSION(5, 15, 0) >= LINUX_VERSION_CODE
		fmt = v4l2_subdev_get_try_format(sd, cfg, 0);
#else
		fmt = v4l2_subdev_get_try_format(sd, sd_state, 0);
#endif
		*fmt = format->format;
	} else if (sensor->current_mode != new_mode ||
		   sensor->fmt.code != format->format.code) {
		fmt = &sensor->fmt;
		*fmt = format->format;

		sensor->current_mode = new_mode;

		__v4l2_ctrl_s_ctrl(sensor->freq_nb_ctrl,
				   sensor->current_mode->freq_nb);
	}

out:
	mutex_unlock(&sensor->lock);

	return ret;
}

static int vd55flood_enum_frame_size(struct v4l2_subdev *sd,
#if KERNEL_VERSION(5, 15, 0) > LINUX_VERSION_CODE
				  struct v4l2_subdev_pad_config *cfg,
#else
				  struct v4l2_subdev_state *sd_state,
#endif
				  struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= ARRAY_SIZE(vd55flood_mode_data))
		return -EINVAL;

	fse->min_width = vd55flood_mode_data[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = vd55flood_mode_data[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}

static const struct v4l2_subdev_core_ops vd55flood_core_ops = {
};

static const struct v4l2_subdev_video_ops vd55flood_video_ops = {
	.s_stream = vd55flood_s_stream,
};

static const struct v4l2_subdev_pad_ops vd55flood_pad_ops = {
	.enum_mbus_code = vd55flood_enum_mbus_code,
	.get_fmt = vd55flood_get_fmt,
	.set_fmt = vd55flood_set_fmt,
	.get_selection = vd55flood_get_selection,
	.enum_frame_size = vd55flood_enum_frame_size,
};

static const struct v4l2_subdev_ops vd55flood_subdev_ops = {
	.core = &vd55flood_core_ops,
	.video = &vd55flood_video_ops,
	.pad = &vd55flood_pad_ops,
};

static const struct media_entity_operations vd55flood_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static int vd55flood_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct fwnode_handle *handle;
	struct vd55flood_dev *sensor;
	struct nvmem_device *nvmem_dev;
	/* Double data rate */
	u32 mipi_bps = link_freq[0] * 2;
	int patch_major, patch_minor;
	int ret;
	u16 val;

	sensor = devm_kzalloc(dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	sensor->i2c_client = client;
	sensor->streaming = false;

	sensor->current_mode = &vd55flood_mode_data[VD55FLOOD_DEFAULT_MODE];

	handle = fwnode_graph_get_next_endpoint(of_fwnode_handle(dev->of_node),
						NULL);
	if (!handle) {
		dev_err(dev, "handle node not found\n");
		return -EINVAL;
	}

	ret = vd55flood_tx_from_ep(sensor, handle);
	fwnode_handle_put(handle);
	if (ret) {
		dev_err(dev, "Failed to parse handle %d\n", ret);
		return ret;
	}

	sensor->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						     GPIOD_OUT_HIGH);

	sensor->xclk = devm_clk_get(dev, NULL);
	if (IS_ERR(sensor->xclk)) {
		dev_err(dev, "failed to get xclk\n");
		return PTR_ERR(sensor->xclk);
	}
	sensor->clk_freq = clk_get_rate(sensor->xclk);
	if (sensor->clk_freq != 12 * HZ_PER_MHZ) {
		dev_warn(dev,
			 "Expected %lu MHz clock, got %lu MHz. Continuing\n",
			 VD55FLOOD_EXT_CLOCK_12MHZ / HZ_PER_MHZ,
			 sensor->clk_freq / HZ_PER_MHZ);
	}

	v4l2_i2c_subdev_init(&sensor->sd, client, &vd55flood_subdev_ops);
	sensor->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sensor->pad.flags = MEDIA_PAD_FL_SOURCE;
	sensor->sd.entity.ops = &vd55flood_subdev_entity_ops;
	sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	/* Frequency to data rate is 1:1 ratio for MIPI */
	sensor->data_rate_in_mbps = mipi_bps;

	nvmem_dev = devm_nvmem_device_get(&client->dev, "vd55flood-nvmem");
	if (IS_ERR(nvmem_dev))
		return PTR_ERR(nvmem_dev);

	ret = clk_prepare_enable(sensor->xclk);
	if (ret) {
		dev_err(&client->dev, "failed to enable clock %d", ret);
		return -EINVAL;
	}

	if (sensor->reset_gpio) {
		ret = vd55flood_apply_reset(sensor);
		if (ret) {
			dev_err(&client->dev, "sensor reset failed %d\n", ret);
			goto disable_clock;
		}
	}

	ret = vd55flood_detect(sensor);
	if (ret) {
		dev_err(&client->dev, "sensor detect failed %d", ret);
		goto disable_clock;
	}

	/* Sanity check for nvmem id */
	ret = nvmem_device_read(nvmem_dev, VD55FLOOD_NVMEM_REG_ID, sizeof(val),
				&val);
	if (ret != sizeof(val)) {
		dev_err(&client->dev, "can't read nvmem identifier\n");
		goto disable_clock;
	}
	if (val != VD55FLOOD_NVMEM_ID_VD55FLOOD &&
	    val != VD55FLOOD_NVMEM_ID_VD55DOT) {
		dev_err(&client->dev,
			"error reading laser driver nvmem identifier (got 0x%x)\n",
			val);
		goto disable_clock;
	}

	ret = nvmem_device_read(nvmem_dev, VD55FLOOD_NVMEM_REG_BASE,
				ARRAY_SIZE(sensor->ld.nvmem_data),
				sensor->ld.nvmem_data);
	if (ret < 0) {
		dev_err(&client->dev,
			"error reading laser driver nvmem data %d\n", ret);
		goto disable_clock;
	}
	if (ret != ARRAY_SIZE(sensor->ld.nvmem_data)) {
		dev_err(&client->dev, "can't read nvmem data\n");
		goto disable_clock;
	}

	ret = vd55flood_setup(sensor);
	if (ret) {
		dev_err(&client->dev, "sensor setup failed %d", ret);
		goto disable_clock;
	}

	ret = vd55flood_check_sensor_rev(sensor);
	if (ret) {
		dev_err(&client->dev, "sensor setup failed %d", ret);
		goto disable_clock;
	}

	ret = vd55flood_patch(sensor);
	if (ret) {
		dev_err(&client->dev, "sensor patch failed %d", ret);
		goto disable_clock;
	}

	ret = vd55flood_boot(sensor);
	if (ret) {
		dev_err(&client->dev, "sensor boot failed %d", ret);
		goto disable_clock;
	}

	if (sensor->rev == VD55FLOOD_ROM_REV_B0) {
		patch_major = VD55FLOOD_FWPATCH_REVISION_MAJOR_B0;
		patch_minor = VD55FLOOD_FWPATCH_REVISION_MINOR_B0;
	} else {
		patch_major = VD55FLOOD_D0_FWPATCH_REVISION_MAJOR_D0;
		patch_minor = VD55FLOOD_D0_FWPATCH_REVISION_MINOR_D0;
	}
	ret = vd55flood_check_patch_version(sensor, patch_major, patch_minor);
	if (ret)
		goto disable_clock;

	ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
	if (ret) {
		dev_err(&client->dev, "pads init failed %d", ret);
		goto disable_clock;
	}

	mutex_init(&sensor->lock);

	vd55flood_fill_framefmt(sensor, sensor->current_mode, &sensor->fmt,
				VD55FLOOD_MEDIA_BUS_FMT_DEF);

	ret = vd55flood_init_controls(sensor);
	if (ret) {
		dev_err(&client->dev, "controls initialization failed %d", ret);
		goto disable_clock;
	}

	ret = vd55flood_write_laser_driver_nvmem(sensor);
	if (ret) {
		dev_err(&client->dev, "NVMEM writing failed %d", ret);
		goto disable_clock;
	}

	ret = v4l2_async_register_subdev(&sensor->sd);
	if (ret) {
		dev_err(&client->dev, "async subdev register failed %d", ret);
		goto disable_clock;
	}

	return 0;

disable_clock:
	clk_disable_unprepare(sensor->xclk);
	return ret;
}

#if KERNEL_VERSION(6, 1, 0) > LINUX_VERSION_CODE
static int vd55flood_remove(struct i2c_client *client)
#else
static void vd55flood_remove(struct i2c_client *client)
#endif
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct vd55flood_dev *sensor = to_vd55flood_dev(sd);

	v4l2_async_unregister_subdev(&sensor->sd);
	mutex_destroy(&sensor->lock);
	media_entity_cleanup(&sensor->sd.entity);

	clk_disable_unprepare(sensor->xclk);

#if KERNEL_VERSION(6, 1, 0) > LINUX_VERSION_CODE
	return 0;
#endif
}

static const struct of_device_id vd55flood_dt_ids[] = {
	{ .compatible = "st,st-vd55flood" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vd55flood_dt_ids);

static struct i2c_driver vd55flood_i2c_driver = {
	.driver = {
		.name  = "st-vd55flood",
		.of_match_table = vd55flood_dt_ids,
	},
#if KERNEL_VERSION(6, 1, 0) > LINUX_VERSION_CODE
	.probe_new = vd55flood_probe,
#else
	.probe = vd55flood_probe,
#endif
	.remove = vd55flood_remove,
};

module_i2c_driver(vd55flood_i2c_driver);

MODULE_AUTHOR("Benjamin Mugnier <benjamin.mugnier@foss.st.com>");
MODULE_DESCRIPTION("VD55flood ToF Subdev Driver");
MODULE_LICENSE("GPL v2");
