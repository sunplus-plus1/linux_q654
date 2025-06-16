// SPDX-License-Identifier: GPL-2.0
/*
 * BMI160 - Bosch IMU (accel, gyro plus external magnetometer)
 *
 * Copyright (c) 2016, Intel Corporation.
 * Copyright (c) 2019, Martin Kelly.
 *
 * IIO core driver for BMI160, with support for I2C/SPI busses
 *
 * TODO: magnetometer, hardware FIFO
 */
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/regulator/consumer.h>

#include <linux/iio/iio.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger.h>

#include "bmi160.h"
#if defined(CONFIG_SOC_SP7350)
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#endif

#define BMI160_REG_CHIP_ID	0x00
#define BMI160_CHIP_ID_VAL	0xD1

#define BMI160_REG_PMU_STATUS	0x03

/* X axis data low byte address, the rest can be obtained using axis offset */
#define BMI160_REG_DATA_MAGN_XOUT_L	0x04
#define BMI160_REG_DATA_GYRO_XOUT_L	0x0C
#define BMI160_REG_DATA_ACCEL_XOUT_L	0x12

#define BMI160_REG_ACCEL_CONFIG		0x40
#define BMI160_ACCEL_CONFIG_ODR_MASK	GENMASK(3, 0)
#define BMI160_ACCEL_CONFIG_BWP_MASK	GENMASK(6, 4)

#if defined(CONFIG_SOC_SP7350)
#define BMI160_REG_ACCEL_FIFO_LENGTH	0x22
#define BMI160_REG_ACCEL_FIFO_DATA      0x24
#endif
#define BMI160_REG_ACCEL_RANGE		0x41
#define BMI160_ACCEL_RANGE_2G		0x03
#define BMI160_ACCEL_RANGE_4G		0x05
#define BMI160_ACCEL_RANGE_8G		0x08
#define BMI160_ACCEL_RANGE_16G		0x0C

#define BMI160_REG_GYRO_CONFIG		0x42
#define BMI160_GYRO_CONFIG_ODR_MASK	GENMASK(3, 0)
#define BMI160_GYRO_CONFIG_BWP_MASK	GENMASK(5, 4)

#define BMI160_REG_GYRO_RANGE		0x43
#define BMI160_GYRO_RANGE_2000DPS	0x00
#define BMI160_GYRO_RANGE_1000DPS	0x01
#define BMI160_GYRO_RANGE_500DPS	0x02
#define BMI160_GYRO_RANGE_250DPS	0x03
#define BMI160_GYRO_RANGE_125DPS	0x04

#define BMI160_REG_CMD			0x7E
#define BMI160_CMD_ACCEL_PM_SUSPEND	0x10
#define BMI160_CMD_ACCEL_PM_NORMAL	0x11
#define BMI160_CMD_ACCEL_PM_LOW_POWER	0x12
#define BMI160_CMD_GYRO_PM_SUSPEND	0x14
#define BMI160_CMD_GYRO_PM_NORMAL	0x15
#define BMI160_CMD_GYRO_PM_FAST_STARTUP	0x17
#define BMI160_CMD_SOFTRESET		0xB6
#if defined(CONFIG_SOC_SP7350)
#define BMI160_CMD_CLR_FIFO		0xB0
#endif

#define BMI160_REG_INT_EN		0x51
#define BMI160_DRDY_INT_EN		BIT(4)
#if defined(CONFIG_SOC_SP7350)
#define BMI160_FIFO_FULL_INT_EN		BIT(5)
#define BMI160_FIFO_FWM_INT_EN		BIT(6)

#define BMI160_REG_FIFO_CONFIG0		0x46
#define BMI160_REG_FIFO_CONFIG1		0x47
#define BMI160_FIFO_GYR_EN		BIT(7)
#define BMI160_FIFO_ACC_EN		BIT(6)
#define BMI160_FIFO_HEADER_EN		BIT(4)
#endif

#define BMI160_REG_INT_OUT_CTRL		0x53
#define BMI160_INT_OUT_CTRL_MASK	0x0f
#define BMI160_INT1_OUT_CTRL_SHIFT	0
#define BMI160_INT2_OUT_CTRL_SHIFT	4
#define BMI160_EDGE_TRIGGERED		BIT(0)
#define BMI160_ACTIVE_HIGH		BIT(1)
#define BMI160_OPEN_DRAIN		BIT(2)
#define BMI160_OUTPUT_EN		BIT(3)

#define BMI160_REG_INT_LATCH		0x54
#define BMI160_INT1_LATCH_MASK		BIT(4)
#define BMI160_INT2_LATCH_MASK		BIT(5)

/* INT1 and INT2 are in the opposite order as in INT_OUT_CTRL! */
#define BMI160_REG_INT_MAP		0x56
#define BMI160_INT1_MAP_DRDY_EN		0x80
#define BMI160_INT2_MAP_DRDY_EN		0x08
#if defined(CONFIG_SOC_SP7350)
#define BMI160_INT1_MAP_FIFO_FULL_EN	0x20
#define BMI160_INT1_MAP_FIFO_FWM_EN	0x40
#endif

#define BMI160_REG_DUMMY		0x7F

#define BMI160_NORMAL_WRITE_USLEEP	2
#define BMI160_SUSPENDED_WRITE_USLEEP	450

#define BMI160_ACCEL_PMU_MIN_USLEEP	3800
#define BMI160_GYRO_PMU_MIN_USLEEP	80000
#define BMI160_SOFTRESET_USLEEP		1000

#define BMI160_CHANNEL(_type, _axis, _index) {			\
	.type = _type,						\
	.modified = 1,						\
	.channel2 = IIO_MOD_##_axis,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) |  \
		BIT(IIO_CHAN_INFO_SAMP_FREQ),			\
	.scan_index = _index,					\
	.scan_type = {						\
		.sign = 's',					\
		.realbits = 16,					\
		.storagebits = 16,				\
		.endianness = IIO_LE,				\
	},							\
	.ext_info = bmi160_ext_info,				\
}

#if defined(CONFIG_SOC_SP7350)
static struct bmi160_data *g_bmi_data;
static struct iio_dev *g_indio_dev;
static struct work_struct bmi_work;

static int bmi160_write_conf_reg(struct regmap *regmap, unsigned int reg,
				 unsigned int mask, unsigned int bits,
				 unsigned int write_usleep);
#endif

/* scan indexes follow DATA register order */
enum bmi160_scan_axis {
	BMI160_SCAN_EXT_MAGN_X = 0,
	BMI160_SCAN_EXT_MAGN_Y,
	BMI160_SCAN_EXT_MAGN_Z,
	BMI160_SCAN_RHALL,
	BMI160_SCAN_GYRO_X,
	BMI160_SCAN_GYRO_Y,
	BMI160_SCAN_GYRO_Z,
	BMI160_SCAN_ACCEL_X,
	BMI160_SCAN_ACCEL_Y,
	BMI160_SCAN_ACCEL_Z,
	BMI160_SCAN_TIMESTAMP,
};

enum bmi160_sensor_type {
	BMI160_ACCEL	= 0,
	BMI160_GYRO,
	BMI160_EXT_MAGN,
	BMI160_NUM_SENSORS /* must be last */
};

enum bmi160_int_pin {
	BMI160_PIN_INT1,
	BMI160_PIN_INT2
};

const struct regmap_config bmi160_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};
EXPORT_SYMBOL(bmi160_regmap_config);

struct bmi160_regs {
	u8 data; /* LSB byte register for X-axis */
	u8 config;
	u8 config_odr_mask;
	u8 config_bwp_mask;
	u8 range;
	u8 pmu_cmd_normal;
	u8 pmu_cmd_suspend;
};

static struct bmi160_regs bmi160_regs[] = {
	[BMI160_ACCEL] = {
		.data	= BMI160_REG_DATA_ACCEL_XOUT_L,
		.config	= BMI160_REG_ACCEL_CONFIG,
		.config_odr_mask = BMI160_ACCEL_CONFIG_ODR_MASK,
		.config_bwp_mask = BMI160_ACCEL_CONFIG_BWP_MASK,
		.range	= BMI160_REG_ACCEL_RANGE,
		.pmu_cmd_normal = BMI160_CMD_ACCEL_PM_NORMAL,
		.pmu_cmd_suspend = BMI160_CMD_ACCEL_PM_SUSPEND,
	},
	[BMI160_GYRO] = {
		.data	= BMI160_REG_DATA_GYRO_XOUT_L,
		.config	= BMI160_REG_GYRO_CONFIG,
		.config_odr_mask = BMI160_GYRO_CONFIG_ODR_MASK,
		.config_bwp_mask = BMI160_GYRO_CONFIG_BWP_MASK,
		.range	= BMI160_REG_GYRO_RANGE,
		.pmu_cmd_normal = BMI160_CMD_GYRO_PM_NORMAL,
		.pmu_cmd_suspend = BMI160_CMD_GYRO_PM_SUSPEND,
	},
};

static unsigned long bmi160_pmu_time[] = {
	[BMI160_ACCEL] = BMI160_ACCEL_PMU_MIN_USLEEP,
	[BMI160_GYRO] = BMI160_GYRO_PMU_MIN_USLEEP,
};

struct bmi160_scale {
	u8 bits;
	int uscale;
};

struct bmi160_odr {
	u8 bits;
	int odr;
	int uodr;
};

static const struct bmi160_scale bmi160_accel_scale[] = {
	{ BMI160_ACCEL_RANGE_2G, 598},
	{ BMI160_ACCEL_RANGE_4G, 1197},
	{ BMI160_ACCEL_RANGE_8G, 2394},
	{ BMI160_ACCEL_RANGE_16G, 4788},
};

static const struct bmi160_scale bmi160_gyro_scale[] = {
	{ BMI160_GYRO_RANGE_2000DPS, 1065},
	{ BMI160_GYRO_RANGE_1000DPS, 532},
	{ BMI160_GYRO_RANGE_500DPS, 266},
	{ BMI160_GYRO_RANGE_250DPS, 133},
	{ BMI160_GYRO_RANGE_125DPS, 66},
};

struct bmi160_scale_item {
	const struct bmi160_scale *tbl;
	int num;
};

static const struct  bmi160_scale_item bmi160_scale_table[] = {
	[BMI160_ACCEL] = {
		.tbl	= bmi160_accel_scale,
		.num	= ARRAY_SIZE(bmi160_accel_scale),
	},
	[BMI160_GYRO] = {
		.tbl	= bmi160_gyro_scale,
		.num	= ARRAY_SIZE(bmi160_gyro_scale),
	},
};

static const struct bmi160_odr bmi160_accel_odr[] = {
	{0x01, 0, 781250},
	{0x02, 1, 562500},
	{0x03, 3, 125000},
	{0x04, 6, 250000},
	{0x05, 12, 500000},
	{0x06, 25, 0},
	{0x07, 50, 0},
	{0x08, 100, 0},
	{0x09, 200, 0},
	{0x0A, 400, 0},
	{0x0B, 800, 0},
	{0x0C, 1600, 0},
};

static const struct bmi160_odr bmi160_gyro_odr[] = {
	{0x06, 25, 0},
	{0x07, 50, 0},
	{0x08, 100, 0},
	{0x09, 200, 0},
	{0x0A, 400, 0},
	{0x0B, 800, 0},
	{0x0C, 1600, 0},
	{0x0D, 3200, 0},
};

struct bmi160_odr_item {
	const struct bmi160_odr *tbl;
	int num;
};

static const struct  bmi160_odr_item bmi160_odr_table[] = {
	[BMI160_ACCEL] = {
		.tbl	= bmi160_accel_odr,
		.num	= ARRAY_SIZE(bmi160_accel_odr),
	},
	[BMI160_GYRO] = {
		.tbl	= bmi160_gyro_odr,
		.num	= ARRAY_SIZE(bmi160_gyro_odr),
	},
};

static const struct iio_mount_matrix *
bmi160_get_mount_matrix(const struct iio_dev *indio_dev,
			const struct iio_chan_spec *chan)
{
	struct bmi160_data *data = iio_priv(indio_dev);

	return &data->orientation;
}

static const struct iio_chan_spec_ext_info bmi160_ext_info[] = {
	IIO_MOUNT_MATRIX(IIO_SHARED_BY_DIR, bmi160_get_mount_matrix),
	{ }
};

static const struct iio_chan_spec bmi160_channels[] = {
	BMI160_CHANNEL(IIO_ACCEL, X, BMI160_SCAN_ACCEL_X),
	BMI160_CHANNEL(IIO_ACCEL, Y, BMI160_SCAN_ACCEL_Y),
	BMI160_CHANNEL(IIO_ACCEL, Z, BMI160_SCAN_ACCEL_Z),
	BMI160_CHANNEL(IIO_ANGL_VEL, X, BMI160_SCAN_GYRO_X),
	BMI160_CHANNEL(IIO_ANGL_VEL, Y, BMI160_SCAN_GYRO_Y),
	BMI160_CHANNEL(IIO_ANGL_VEL, Z, BMI160_SCAN_GYRO_Z),
	IIO_CHAN_SOFT_TIMESTAMP(BMI160_SCAN_TIMESTAMP),
};

static enum bmi160_sensor_type bmi160_to_sensor(enum iio_chan_type iio_type)
{
	switch (iio_type) {
	case IIO_ACCEL:
		return BMI160_ACCEL;
	case IIO_ANGL_VEL:
		return BMI160_GYRO;
	default:
		return -EINVAL;
	}
}

static
int bmi160_set_mode(struct bmi160_data *data, enum bmi160_sensor_type t,
		    bool mode)
{
	int ret;
	u8 cmd;

	if (mode)
		cmd = bmi160_regs[t].pmu_cmd_normal;
	else
		cmd = bmi160_regs[t].pmu_cmd_suspend;

	ret = regmap_write(data->regmap, BMI160_REG_CMD, cmd);
	if (ret)
		return ret;

	usleep_range(bmi160_pmu_time[t], bmi160_pmu_time[t] + 1000);

	return 0;
}

static
int bmi160_set_scale(struct bmi160_data *data, enum bmi160_sensor_type t,
		     int uscale)
{
	int i;

	for (i = 0; i < bmi160_scale_table[t].num; i++)
		if (bmi160_scale_table[t].tbl[i].uscale == uscale)
			break;

	if (i == bmi160_scale_table[t].num)
		return -EINVAL;

	return regmap_write(data->regmap, bmi160_regs[t].range,
			    bmi160_scale_table[t].tbl[i].bits);
}

static
int bmi160_get_scale(struct bmi160_data *data, enum bmi160_sensor_type t,
		     int *uscale)
{
	int i, ret, val;

	ret = regmap_read(data->regmap, bmi160_regs[t].range, &val);
	if (ret)
		return ret;

	for (i = 0; i < bmi160_scale_table[t].num; i++)
		if (bmi160_scale_table[t].tbl[i].bits == val) {
			*uscale = bmi160_scale_table[t].tbl[i].uscale;
			return 0;
		}

	return -EINVAL;
}

static int bmi160_get_data(struct bmi160_data *data, int chan_type,
			   int axis, int *val)
{
	u8 reg;
	int ret;
	__le16 sample;
	enum bmi160_sensor_type t = bmi160_to_sensor(chan_type);

	reg = bmi160_regs[t].data + (axis - IIO_MOD_X) * sizeof(sample);

	ret = regmap_bulk_read(data->regmap, reg, &sample, sizeof(sample));
	if (ret)
		return ret;

	*val = sign_extend32(le16_to_cpu(sample), 15);

	return 0;
}

static
int bmi160_set_odr(struct bmi160_data *data, enum bmi160_sensor_type t,
		   int odr, int uodr)
{
	int i;

	for (i = 0; i < bmi160_odr_table[t].num; i++)
		if (bmi160_odr_table[t].tbl[i].odr == odr &&
		    bmi160_odr_table[t].tbl[i].uodr == uodr)
			break;

	if (i >= bmi160_odr_table[t].num)
		return -EINVAL;

	return regmap_update_bits(data->regmap,
				  bmi160_regs[t].config,
				  bmi160_regs[t].config_odr_mask,
				  bmi160_odr_table[t].tbl[i].bits);
}

static int bmi160_get_odr(struct bmi160_data *data, enum bmi160_sensor_type t,
			  int *odr, int *uodr)
{
	int i, val, ret;

	ret = regmap_read(data->regmap, bmi160_regs[t].config, &val);
	if (ret)
		return ret;

	val &= bmi160_regs[t].config_odr_mask;

	for (i = 0; i < bmi160_odr_table[t].num; i++)
		if (val == bmi160_odr_table[t].tbl[i].bits)
			break;

	if (i >= bmi160_odr_table[t].num)
		return -EINVAL;

	*odr = bmi160_odr_table[t].tbl[i].odr;
	*uodr = bmi160_odr_table[t].tbl[i].uodr;

	return 0;
}

#if defined(CONFIG_SOC_SP7350)
struct bmi160_accel_t acc_frame_arr[FIFO_FRAME_CNT];
struct bmi160_gyro_t gyro_frame_arr[FIFO_FRAME_CNT];

int bmi160_fifo_data_parse_handle(struct bmi160_data *data, struct iio_dev *indio_dev, int wq_flag)
{
	u8 frame_head = 0;
	u64 ts_gap;
	u64 ts_gap_real = 0;
	u64 ts_cur;
	u64 frm_odr;
	u16 fifo_index = 0;/* fifo data index*/
	u16 j = 0;
	u16 i = 0;
	u16 frm_cnt = 0;
	u16 scan_channel = 0;
	s8 ret = 0;
	u16 fifo_length = data->fifo_length;
	struct bmi160_gyro_t gyro;
	struct bmi160_accel_t acc;

	if (wq_flag)
		ts_cur = sunplus_tsp_read(TSP_IMU1);
	else
		ts_cur = indio_dev->pollfunc->timestamp;

	if (!data->ts_last) {
		dev_dbg(&indio_dev->dev, "skip first group data(%llu)\n", data->ts_last);
		data->ts_last = ts_cur;
		return ret;
	}
	memset(&acc, 0, sizeof(acc));
	memset(&gyro, 0, sizeof(gyro));
	for (i = 0; i < FIFO_FRAME_CNT; i++) {
		memset(&acc_frame_arr[i], 0, sizeof(struct bmi160_accel_t));
		memset(&gyro_frame_arr[i], 0, sizeof(struct bmi160_gyro_t));
	}

	dev_dbg(&indio_dev->dev, "frame head 0x%x, fifo_length: %d \n", data->fifo_data[fifo_index], fifo_length);
	for (fifo_index = 0; fifo_index < fifo_length;) {

		frame_head = data->fifo_data[fifo_index];

		switch (frame_head) {
		case FIFO_HEAD_SKIP_FRAME:
			/*fifo data frame index + 1*/
			fifo_index = fifo_index + 1;
			if (fifo_index + 1 > fifo_length) {
				ret = -1;
				dev_err(&indio_dev->dev, "skip frame fifo_index err, index = %d, fifo_length %d\n", fifo_index, fifo_length);
				break;
			}
			fifo_index = fifo_index + 1;
			dev_dbg(&indio_dev->dev, "skip frame detected, fifo_index=%d\n", fifo_index);
		break;

		case FIFO_HEAD_G_A:
		{	/*fifo data frame index + 1*/
			fifo_index = fifo_index + 1;
			if (fifo_index + GA_BYTES_FRM > fifo_length) {
				ret = -1;
				dev_err(&indio_dev->dev, "G A fifo_index err, index = %d, fifo_length %d\n", fifo_index, fifo_length);
				break;
			}
			gyro.x = data->fifo_data[fifo_index + 1] << 8 |
					data->fifo_data[fifo_index + 0];
			gyro.y = data->fifo_data[fifo_index + 3] << 8 |
					data->fifo_data[fifo_index + 2];
			gyro.z = data->fifo_data[fifo_index + 5] << 8 |
					data->fifo_data[fifo_index + 4];

			acc.x = data->fifo_data[fifo_index + 7] << 8 |
					data->fifo_data[fifo_index + 6];
			acc.y = data->fifo_data[fifo_index + 9] << 8 |
					data->fifo_data[fifo_index + 8];
			acc.z = data->fifo_data[fifo_index + 11] << 8 |
					data->fifo_data[fifo_index + 10];
			fifo_index = fifo_index + GA_BYTES_FRM;

			dev_dbg(&indio_dev->dev, "gyro and accel frame detected, fifo_index=%d\n", fifo_index);
			break;
		}
		case FIFO_HEAD_A:
		{	/*fifo data frame index + 1*/
			fifo_index = fifo_index + 1;
			if (fifo_index + A_BYTES_FRM > fifo_length) {
				ret = -1;
				dev_err(&indio_dev->dev, "acc fifo_index err, index = %d, fifo_length %d\n", fifo_index, fifo_length);
				break;
			}

			acc.x = data->fifo_data[fifo_index + 1] << 8 |
					data->fifo_data[fifo_index + 0];
			acc.y = data->fifo_data[fifo_index + 3] << 8 |
					data->fifo_data[fifo_index + 2];
			acc.z = data->fifo_data[fifo_index + 5] << 8 |
					data->fifo_data[fifo_index + 4];
			fifo_index = fifo_index + A_BYTES_FRM;
			dev_dbg(&indio_dev->dev, "accel frame detected, fifo_index=%d\n", fifo_index);
			break;
		}
		case FIFO_HEAD_G:
		{	/*fifo data frame index + 1*/
			fifo_index = fifo_index + 1;
			if (fifo_index + G_BYTES_FRM > fifo_length) {
				ret = -1;
				dev_err(&indio_dev->dev, "gyro fifo_index err, index = %d, fifo_length %d\n", fifo_index, fifo_length);
				break;
			}

			gyro.x = data->fifo_data[fifo_index + 1] << 8 |
					data->fifo_data[fifo_index + 0];
			gyro.y = data->fifo_data[fifo_index + 3] << 8 |
					data->fifo_data[fifo_index + 2];
			gyro.z = data->fifo_data[fifo_index + 5] << 8 |
					data->fifo_data[fifo_index + 4];

			fifo_index = fifo_index + G_BYTES_FRM;
			dev_dbg(&indio_dev->dev, "gyro frame detected, fifo_index=%d\n", fifo_index);
			break;
		}

		default:
			ret = -1;
			printk("frame head: 0x%x, fifo index 0x%x, fifo_length 0x%x\n", frame_head, fifo_index, fifo_length);
		break;

		}
		if (ret) {
			break;
		}
		acc_frame_arr[frm_cnt] = acc;
		gyro_frame_arr[frm_cnt] = gyro;
		frm_cnt++;

	}
	frm_odr = max(data->odr_acc, data->odr_gyro);
	ts_gap = (u64)(1000000000) * 1000000/frm_odr;
	ts_gap_real = ts_gap;
	if (frm_cnt && data->ts_last)
		ts_gap_real = (ts_cur - data->ts_last) * fifo_length / (frm_cnt * data->watermark_hwfifo * 4);
	for (i = 0; i < frm_cnt; i++) {
		j = 0;
		for_each_set_bit(scan_channel, indio_dev->active_scan_mask, indio_dev->masklength) {
			switch(scan_channel) {
				case   BMI160_SCAN_GYRO_X:
					data->buf[j++] = gyro_frame_arr[i].x;
					break;
				case   BMI160_SCAN_GYRO_Y:
					data->buf[j++] = gyro_frame_arr[i].y;
					break;
				case   BMI160_SCAN_GYRO_Z:
					data->buf[j++] = gyro_frame_arr[i].z;
					break;
				case   BMI160_SCAN_ACCEL_X:
					data->buf[j++] = acc_frame_arr[i].x;
					break;
				case   BMI160_SCAN_ACCEL_Y:
					data->buf[j++] = acc_frame_arr[i].y;
					break;
				case   BMI160_SCAN_ACCEL_Z:
					data->buf[j++] = acc_frame_arr[i].z;
					break;
			}
		}
		dev_dbg(&indio_dev->dev, "frm_cnt:%d, ts_last:%llu, ts_gap:%llu, ts_gap_real:%llu, odr:%lld, ts:%llu\n",
			frm_cnt, data->ts_last, ts_gap, ts_gap_real, frm_odr, ts_cur - (frm_cnt - 1 - i) * ts_gap_real);
		iio_push_to_buffers_with_timestamp(indio_dev, data->buf, ts_cur - (frm_cnt - 1 - i ) * ts_gap_real);
	}
	data->ts_last = ts_cur;
	dev_dbg(&indio_dev->dev, "ktime ts end:%llu\n", ktime_get_ns());

	return ret;
}

static u64 t1_old = 0, t2_old = 0, t1_new = 0, t2_new = 0, int_count = 0;
static int flag_old = 0;

#define FIFO_DATA_BUFSIZE    1024
#define BYTES_PER_SAMPLE 13
static void bmi_fifo_interrupt_handle(struct bmi160_data *data, struct iio_dev *indio_dev, int wq_flag)
{
	int err = 0;
	__le16 fifo_length = 0;
	int val, val2;
	int ret, i = 0;

	for (i = 0; i < 3; i++) {
		err = regmap_bulk_read(data->regmap, BMI160_REG_ACCEL_FIFO_LENGTH, &fifo_length, sizeof(fifo_length));
		if (fifo_length == 0 || err) {
			ret = regmap_read(data->regmap, BMI160_REG_INT_EN, &val);
			dev_err(&indio_dev->dev, "fifo zero or ret %d, wq_flag %d, val 0x%x, i %d, t2 %llu, %llu, %llu, flag_old %d, t1_new %llu, t2_new %llu, int_count %llu\n", err, wq_flag, val, i, t2_old, t1_old, t2_old - t1_old, flag_old, t1_new, t2_new, int_count);
		} else {
			if (i >= 1)
				dev_err(&indio_dev->dev, "fifo len zero or ret %d, wq_flag %d, val 0x%x, i %d\n", err, wq_flag, val, i);
			break;
		}
	}
	if (i == 3)
		return;

	dev_dbg(&indio_dev->dev, "read fifo leght: 0x%x, ktime start ts:%llu", fifo_length, ktime_get_ns());
	if (fifo_length > FIFO_DATA_BUFSIZE)
		fifo_length = FIFO_DATA_BUFSIZE;
	memset(data->fifo_data, 0, 1024);

	/*i2c DMA not support len is odd*/
	if(fifo_length%2 !=0){
		fifo_length -= BYTES_PER_SAMPLE;
	}
	err = regmap_raw_read(data->regmap, BMI160_REG_ACCEL_FIFO_DATA, data->fifo_data, fifo_length);
	if (err) {
		//clr fifo
		regmap_write(data->regmap, BMI160_REG_CMD, BMI160_CMD_CLR_FIFO);
		dev_err(&indio_dev->dev, "read fifo err");
		return;
	}
	err = bmi160_get_odr(data, BMI160_ACCEL, &val, &val2);
	if (err) {
		dev_err(&indio_dev->dev, "get acc odr err");
		return;
	}
	data->odr_acc = (val * 1000000) + (val2 * 1000000);

	err = bmi160_get_odr(data, BMI160_GYRO, &val, &val2);
	if (err) {
		dev_err(&indio_dev->dev, "get gyro odr err");
		return;
	}
	data->odr_gyro = (val * 1000000) + (val2 * 1000000);

	data->fifo_length = fifo_length;
	err = bmi160_fifo_data_parse_handle(data, indio_dev, wq_flag);
	if (err)
		dev_err(&indio_dev->dev, "parse fifo data err:%d", err);

	if (wq_flag == 1) {
		ret = regmap_write(data->regmap, BMI160_REG_CMD, BMI160_CMD_CLR_FIFO);
		if (ret) {
			printk(KERN_EMERG "%s:%d regmap write error ret %d\n", __func__, __LINE__, ret);
		}
	}
}
#define TRIGER_STARTED	0x10
#define FIRST_TRIGER	0x1
static int lanuch_flag = FIRST_TRIGER;
static void work_handler(struct work_struct *data)
{
	int wq_flag = 1;
	mutex_lock(&g_bmi_data->bmi_lock);

	t1_new = ktime_get_ns();
	bmi_fifo_interrupt_handle(g_bmi_data, g_indio_dev, wq_flag);
	if (lanuch_flag & TRIGER_STARTED) {
		mod_timer(&g_bmi_data->bmi_tmr, jiffies + msecs_to_jiffies(32));
	}
		

	t2_new = ktime_get_ns();
	flag_old = 2;
	t1_old = t1_new;
	t2_old = t2_new;
	mutex_unlock(&g_bmi_data->bmi_lock);
}

static void bmi_on_timeout(struct timer_list *t)
{
	queue_work(system_unbound_wq, &bmi_work);
}
#endif

static irqreturn_t bmi160_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct bmi160_data *data = iio_priv(indio_dev);
#if defined(CONFIG_SOC_SP7350)
	unsigned int val;
	int ret;

	mutex_lock(&g_bmi_data->bmi_lock);
	t1_new = ktime_get_ns();

	ret = regmap_read(data->regmap, BMI160_REG_FIFO_CONFIG0, &val);
	if (ret) {
		printk("%s:%d Failed to write reg 0x46:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = regmap_write(data->regmap, BMI160_REG_FIFO_CONFIG0, 0xff);
	if (ret) {
		printk("%s:%d Failed to write reg 0x46:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	int_count++;

	if ((lanuch_flag & FIRST_TRIGER)) {
		if (timer_pending(&data->bmi_tmr))
			mod_timer(&data->bmi_tmr, jiffies + msecs_to_jiffies(30));
		else {
			data->bmi_tmr.expires = jiffies + msecs_to_jiffies(30);
			add_timer(&data->bmi_tmr);
		}
		lanuch_flag &= (~FIRST_TRIGER);
	} else if (lanuch_flag & TRIGER_STARTED) {
		mod_timer(&data->bmi_tmr, jiffies + msecs_to_jiffies(30));
	}

	if (data->hwfifo_mode) {
		bmi_fifo_interrupt_handle(data, indio_dev, 0);
	} else {
		int i, ret, j = 0, base = BMI160_REG_DATA_MAGN_XOUT_L;
		__le16 sample;

		for_each_set_bit(i, indio_dev->active_scan_mask,
				 indio_dev->masklength) {
			ret = regmap_bulk_read(data->regmap, base + i * sizeof(sample),
					       &sample, sizeof(sample));
			if (ret)
				goto done;
			data->buf[j++] = sample;
		}

		iio_push_to_buffers_with_timestamp(indio_dev, data->buf, pf->timestamp);
	}
#else
	int i, ret, j = 0, base = BMI160_REG_DATA_MAGN_XOUT_L;
	__le16 sample;

	for_each_set_bit(i, indio_dev->active_scan_mask,
			 indio_dev->masklength) {
		ret = regmap_bulk_read(data->regmap, base + i * sizeof(sample),
				       &sample, sizeof(sample));
		if (ret)
			goto done;
		data->buf[j++] = sample;
	}

	iio_push_to_buffers_with_timestamp(indio_dev, data->buf, pf->timestamp);
#endif
done:
#if defined(CONFIG_SOC_SP7350)
	if ((lanuch_flag & FIRST_TRIGER)) {
		if (timer_pending(&data->bmi_tmr))
			mod_timer(&data->bmi_tmr, jiffies + msecs_to_jiffies(32));
		else {
			data->bmi_tmr.expires = jiffies + msecs_to_jiffies(32);
			add_timer(&data->bmi_tmr);
		}
		lanuch_flag &= (~FIRST_TRIGER);
	} else if (lanuch_flag & TRIGER_STARTED) {
		mod_timer(&data->bmi_tmr, jiffies + msecs_to_jiffies(32));
	}

	t2_new = ktime_get_ns();

	flag_old = 1;
	t1_old = t1_new;
	t2_old = t2_new;

	ret = regmap_write(data->regmap, BMI160_REG_FIFO_CONFIG0, val);
	if (ret) {
		printk("%s:%d Failed to write reg 0x46:%d\n", __func__, __LINE__, ret);
		return ret;
	}

	mutex_unlock(&g_bmi_data->bmi_lock);
#endif

	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

static int bmi160_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *chan,
			   int *val, int *val2, long mask)
{
	int ret;
	struct bmi160_data *data = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = bmi160_get_data(data, chan->type, chan->channel2, val);
		if (ret)
			return ret;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		*val = 0;
		ret = bmi160_get_scale(data,
				       bmi160_to_sensor(chan->type), val2);
		return ret ? ret : IIO_VAL_INT_PLUS_MICRO;
	case IIO_CHAN_INFO_SAMP_FREQ:
		ret = bmi160_get_odr(data, bmi160_to_sensor(chan->type),
				     val, val2);
		return ret ? ret : IIO_VAL_INT_PLUS_MICRO;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bmi160_write_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int val, int val2, long mask)
{
	struct bmi160_data *data = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		return bmi160_set_scale(data,
					bmi160_to_sensor(chan->type), val2);
		break;
	case IIO_CHAN_INFO_SAMP_FREQ:
		return bmi160_set_odr(data, bmi160_to_sensor(chan->type),
				      val, val2);
	default:
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_SOC_SP7350)
ssize_t bmi160_get_hwfifo_mode(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi160_data *data = iio_priv(indio_dev);

	return sprintf(buf, "%d\n", data->hwfifo_mode);
}

ssize_t bmi160_set_hwfifo_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi160_data *data = iio_priv(indio_dev);
	int err, val;

	mutex_lock(&indio_dev->mlock);
	if (iio_buffer_enabled(indio_dev)) {
		err = -EBUSY;
		goto out;
	}

	err = kstrtoint(buf, 10, &val);
	if (err < 0)
		goto out;

	data->hwfifo_mode = val;

out:
	mutex_unlock(&indio_dev->mlock);

	return err < 0 ? err : size;
}

ssize_t bmi160_get_hwfifo_watermark(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi160_data *data = iio_priv(indio_dev);
	unsigned int val;
	int err;

	err = regmap_read(data->regmap, BMI160_REG_FIFO_CONFIG0, &val);
	if (!err)
		data->watermark_hwfifo = val;
	return sprintf(buf, "%d\n", data->watermark_hwfifo);
}

ssize_t bmi160_set_hwfifo_watermark(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi160_data *data = iio_priv(indio_dev);
	int err, val;

	mutex_lock(&indio_dev->mlock);

	err = kstrtoint(buf, 10, &val);
	if (err < 0)
		goto out;

	if (val < 1 || val > 255) {
		dev_err(dev, "Failed to set fifo wm val:%d", val);
		err = -EINVAL;
		goto out;
	}

	err = regmap_write(data->regmap, BMI160_REG_FIFO_CONFIG0, val);
	if (!err)
		data->watermark_hwfifo = val;
out:
	mutex_unlock(&indio_dev->mlock);
	dev_info(dev, "set fifo wm %d(%d)\n", val, err);
	return err < 0 ? err : size;
}
#endif

static
IIO_CONST_ATTR(in_accel_sampling_frequency_available,
	       "0.78125 1.5625 3.125 6.25 12.5 25 50 100 200 400 800 1600");
static
IIO_CONST_ATTR(in_anglvel_sampling_frequency_available,
	       "25 50 100 200 400 800 1600 3200");
static
IIO_CONST_ATTR(in_accel_scale_available,
	       "0.000598 0.001197 0.002394 0.004788");
static
IIO_CONST_ATTR(in_anglvel_scale_available,
	       "0.001065 0.000532 0.000266 0.000133 0.000066");

#if defined(CONFIG_SOC_SP7350)
static
IIO_DEVICE_ATTR(hwfifo_mode, 0644, bmi160_get_hwfifo_mode,
	        bmi160_set_hwfifo_mode, 0);
static
IIO_DEVICE_ATTR(watermark_hwfifo, 0644, bmi160_get_hwfifo_watermark,
	        bmi160_set_hwfifo_watermark, 0);
#endif

static struct attribute *bmi160_attrs[] = {
	&iio_const_attr_in_accel_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_in_anglvel_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_in_accel_scale_available.dev_attr.attr,
	&iio_const_attr_in_anglvel_scale_available.dev_attr.attr,
#if defined(CONFIG_SOC_SP7350)
	&iio_dev_attr_hwfifo_mode.dev_attr.attr,
	&iio_dev_attr_watermark_hwfifo.dev_attr.attr,
#endif
	NULL,
};

static const struct attribute_group bmi160_attrs_group = {
	.attrs = bmi160_attrs,
};

static const struct iio_info bmi160_info = {
	.read_raw = bmi160_read_raw,
	.write_raw = bmi160_write_raw,
	.attrs = &bmi160_attrs_group,
};

static const char *bmi160_match_acpi_device(struct device *dev)
{
	const struct acpi_device_id *id;

	id = acpi_match_device(dev->driver->acpi_match_table, dev);
	if (!id)
		return NULL;

	return dev_name(dev);
}

static int bmi160_write_conf_reg(struct regmap *regmap, unsigned int reg,
				 unsigned int mask, unsigned int bits,
				 unsigned int write_usleep)
{
	int ret;
	unsigned int val;

	ret = regmap_read(regmap, reg, &val);
	if (ret)
		return ret;

	val = (val & ~mask) | bits;

	ret = regmap_write(regmap, reg, val);
	if (ret)
		return ret;

	/*
	 * We need to wait after writing before we can write again. See the
	 * datasheet, page 93.
	 */
	usleep_range(write_usleep, write_usleep + 1000);

	return 0;
}

static int bmi160_config_pin(struct regmap *regmap, enum bmi160_int_pin pin,
			     bool open_drain, u8 irq_mask,
			     unsigned long write_usleep)
{
	int ret;
	struct device *dev = regmap_get_device(regmap);
	u8 int_out_ctrl_shift;
	u8 int_latch_mask;
	u8 int_map_mask;
	u8 int_out_ctrl_mask;
	u8 int_out_ctrl_bits;
	const char *pin_name;
#if defined(CONFIG_SOC_SP7350)
	//struct iio_dev *indio_dev = dev_get_drvdata(dev);
	//struct bmi160_data *data = iio_priv(indio_dev);
#endif

	switch (pin) {
	case BMI160_PIN_INT1:
		int_out_ctrl_shift = BMI160_INT1_OUT_CTRL_SHIFT;
		int_latch_mask = BMI160_INT1_LATCH_MASK;
#if defined(CONFIG_SOC_SP7350)
		int_map_mask = BMI160_INT1_MAP_DRDY_EN | BMI160_INT1_MAP_FIFO_FULL_EN | BMI160_INT1_MAP_FIFO_FWM_EN;
#else
		int_map_mask = BMI160_INT1_MAP_DRDY_EN;
#endif
		break;
	case BMI160_PIN_INT2:
		int_out_ctrl_shift = BMI160_INT2_OUT_CTRL_SHIFT;
		int_latch_mask = BMI160_INT2_LATCH_MASK;
		int_map_mask = BMI160_INT2_MAP_DRDY_EN;
		break;
	}
	int_out_ctrl_mask = BMI160_INT_OUT_CTRL_MASK << int_out_ctrl_shift;

	/*
	 * Enable the requested pin with the right settings:
	 * - Push-pull/open-drain
	 * - Active low/high
	 * - Edge/level triggered
	 */
	int_out_ctrl_bits = BMI160_OUTPUT_EN;
	if (open_drain)
		/* Default is push-pull. */
		int_out_ctrl_bits |= BMI160_OPEN_DRAIN;
	int_out_ctrl_bits |= irq_mask;
	int_out_ctrl_bits <<= int_out_ctrl_shift;

	ret = bmi160_write_conf_reg(regmap, BMI160_REG_INT_OUT_CTRL,
				    int_out_ctrl_mask, int_out_ctrl_bits,
				    write_usleep);
	if (ret)
		return ret;

	/* Set the pin to input mode with no latching. */
	ret = bmi160_write_conf_reg(regmap, BMI160_REG_INT_LATCH,
				    int_latch_mask, int_latch_mask,
				    write_usleep);
	if (ret)
		return ret;

	/* Map interrupts to the requested pin. */
	ret = bmi160_write_conf_reg(regmap, BMI160_REG_INT_MAP,
				    int_map_mask, int_map_mask,
				    write_usleep);
	if (ret) {
		switch (pin) {
		case BMI160_PIN_INT1:
			pin_name = "INT1";
			break;
		case BMI160_PIN_INT2:
			pin_name = "INT2";
			break;
		}
		dev_err(dev, "Failed to configure %s IRQ pin", pin_name);
	}

	return ret;
}

int bmi160_enable_irq(struct regmap *regmap, bool enable)
{
	unsigned int enable_bit = 0;
#if defined(CONFIG_SOC_SP7350)
	unsigned int fifo_mask = 0;
	int ret;
	struct device *dev = regmap_get_device(regmap);
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi160_data *data = iio_priv(indio_dev);

	if (data->hwfifo_mode) {
		//fifo mode
		fifo_mask = BMI160_FIFO_GYR_EN | BMI160_FIFO_ACC_EN | BMI160_FIFO_HEADER_EN;
		if (enable) {
			data->ts_last = 0;
			enable_bit = BMI160_FIFO_FULL_INT_EN | BMI160_FIFO_FWM_INT_EN;
			ret = bmi160_write_conf_reg(regmap, BMI160_REG_FIFO_CONFIG1, fifo_mask, fifo_mask, BMI160_NORMAL_WRITE_USLEEP);
			if (ret)
				return ret;
		} else {
			ret = bmi160_write_conf_reg(regmap, BMI160_REG_FIFO_CONFIG1, fifo_mask, 0, BMI160_NORMAL_WRITE_USLEEP);
			if (ret)
				return ret;
		}
		ret = regmap_write(data->regmap, BMI160_REG_CMD, BMI160_CMD_CLR_FIFO);
		if (ret)
			return ret;

		dev_dbg(dev, "enable irq ts_last:%llu ",data->ts_last);

		return bmi160_write_conf_reg(regmap, BMI160_REG_INT_EN,
					     BMI160_FIFO_FULL_INT_EN | BMI160_FIFO_FWM_INT_EN, enable_bit,
					     BMI160_NORMAL_WRITE_USLEEP);
	} else {
		if (enable)
			enable_bit = BMI160_DRDY_INT_EN;

		return bmi160_write_conf_reg(regmap, BMI160_REG_INT_EN,
					     BMI160_DRDY_INT_EN, enable_bit,
					     BMI160_NORMAL_WRITE_USLEEP);
	}
#else
	if (enable)
		enable_bit = BMI160_DRDY_INT_EN;

	return bmi160_write_conf_reg(regmap, BMI160_REG_INT_EN,
				     BMI160_DRDY_INT_EN, enable_bit,
				     BMI160_NORMAL_WRITE_USLEEP);
#endif
}
EXPORT_SYMBOL(bmi160_enable_irq);

static int bmi160_get_irq(struct device_node *of_node, enum bmi160_int_pin *pin)
{
	int irq;

	/* Use INT1 if possible, otherwise fall back to INT2. */
	irq = of_irq_get_byname(of_node, "INT1");
	if (irq > 0) {
		*pin = BMI160_PIN_INT1;
		return irq;
	}

	irq = of_irq_get_byname(of_node, "INT2");
	if (irq > 0)
		*pin = BMI160_PIN_INT2;

	return irq;
}

static int bmi160_config_device_irq(struct iio_dev *indio_dev, int irq_type,
				    enum bmi160_int_pin pin)
{
	bool open_drain;
	u8 irq_mask;
	struct bmi160_data *data = iio_priv(indio_dev);
	struct device *dev = regmap_get_device(data->regmap);

	/* Level-triggered, active-low is the default if we set all zeroes. */
	if (irq_type == IRQF_TRIGGER_RISING)
		irq_mask = BMI160_ACTIVE_HIGH | BMI160_EDGE_TRIGGERED;
	else if (irq_type == IRQF_TRIGGER_FALLING)
		irq_mask = BMI160_EDGE_TRIGGERED;
	else if (irq_type == IRQF_TRIGGER_HIGH)
		irq_mask = BMI160_ACTIVE_HIGH;
	else if (irq_type == IRQF_TRIGGER_LOW)
		irq_mask = 0;
	else {
		dev_err(&indio_dev->dev,
			"Invalid interrupt type 0x%x specified\n", irq_type);
		return -EINVAL;
	}

	open_drain = of_property_read_bool(dev->of_node, "drive-open-drain");

	return bmi160_config_pin(data->regmap, pin, open_drain, irq_mask,
				 BMI160_NORMAL_WRITE_USLEEP);
}

static int bmi160_setup_irq(struct iio_dev *indio_dev, int irq,
			    enum bmi160_int_pin pin)
{
	struct irq_data *desc;
	u32 irq_type;
	int ret;

	desc = irq_get_irq_data(irq);
	if (!desc) {
		dev_err(&indio_dev->dev, "Could not find IRQ %d\n", irq);
		return -EINVAL;
	}

	irq_type = irqd_get_trigger_type(desc);

	ret = bmi160_config_device_irq(indio_dev, irq_type, pin);
	if (ret)
		return ret;

	return bmi160_probe_trigger(indio_dev, irq, irq_type);
}

static int bmi160_chip_init(struct bmi160_data *data, bool use_spi)
{
	int ret;
	unsigned int val;
	struct device *dev = regmap_get_device(data->regmap);
#if defined(CONFIG_SOC_SP7350)
	int err;
#endif

	ret = regulator_bulk_enable(ARRAY_SIZE(data->supplies), data->supplies);
	if (ret) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

#if defined(CONFIG_SOC_SP7350)
	if (data->gpio_pwr_on > 0) {
		err = gpio_request(data->gpio_pwr_on, "IMU_PWR_EN");
		if (err < 0) {
			printk("%s: gpio_request(%d) for IMU_PWR_EN failed %d\n",
				__FUNCTION__, data->gpio_pwr_on, err);
			return err;
		}

		err = gpio_direction_output(data->gpio_pwr_on, 1);
		if (err) {
			printk("%s: IMU_PWR_EN didn't output high\n", __FUNCTION__);
			return err;
		}
	}
#endif

	ret = regmap_write(data->regmap, BMI160_REG_CMD, BMI160_CMD_SOFTRESET);
	if (ret)
#if defined(CONFIG_SOC_SP7350)
		return ret;
#else
		goto disable_regulator;
#endif

	usleep_range(BMI160_SOFTRESET_USLEEP, BMI160_SOFTRESET_USLEEP + 1);

	/*
	 * CS rising edge is needed before starting SPI, so do a dummy read
	 * See Section 3.2.1, page 86 of the datasheet
	 */
	if (use_spi) {
		ret = regmap_read(data->regmap, BMI160_REG_DUMMY, &val);
		if (ret)
#if defined(CONFIG_SOC_SP7350)
			return ret;
#else
			goto disable_regulator;
#endif
	}

	ret = regmap_read(data->regmap, BMI160_REG_CHIP_ID, &val);
	if (ret) {
		dev_err(dev, "Error reading chip id\n");
#if defined(CONFIG_SOC_SP7350)
		return ret;
#else
		goto disable_regulator;
#endif
	}
	if (val != BMI160_CHIP_ID_VAL) {
		dev_err(dev, "Wrong chip id, got %x expected %x\n",
			val, BMI160_CHIP_ID_VAL);
#if defined(CONFIG_SOC_SP7350)
		return -ENODEV;
#else
		ret = -ENODEV;
		goto disable_regulator;
#endif
	}

#if defined(CONFIG_SOC_SP7350)
	data->hwfifo_mode = false;
#endif

	ret = bmi160_set_mode(data, BMI160_ACCEL, true);
	if (ret)

#if defined(CONFIG_SOC_SP7350)
		return ret;
#else
		goto disable_regulator;
#endif

	ret = bmi160_set_mode(data, BMI160_GYRO, true);
	if (ret)
#if defined(CONFIG_SOC_SP7350)
		return ret;
#else
		goto disable_accel;
#endif

	return 0;

#if !defined(CONFIG_SOC_SP7350)
disable_accel:
	bmi160_set_mode(data, BMI160_ACCEL, false);

disable_regulator:
	regulator_bulk_disable(ARRAY_SIZE(data->supplies), data->supplies);
	return ret;
#endif
}

static int bmi160_data_rdy_trigger_set_state(struct iio_trigger *trig,
					     bool enable)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct bmi160_data *data = iio_priv(indio_dev);

#if defined(CONFIG_SOC_SP7350)
	if (enable == false) {
		del_timer_sync(&g_bmi_data->bmi_tmr);
		lanuch_flag = 0;
	}
	else {
		lanuch_flag = TRIGER_STARTED | FIRST_TRIGER;
	}
#endif

	return bmi160_enable_irq(data->regmap, enable);
}

static const struct iio_trigger_ops bmi160_trigger_ops = {
	.set_trigger_state = &bmi160_data_rdy_trigger_set_state,
};

int bmi160_probe_trigger(struct iio_dev *indio_dev, int irq, u32 irq_type)
{
	struct bmi160_data *data = iio_priv(indio_dev);
	int ret;

	data->trig = devm_iio_trigger_alloc(&indio_dev->dev, "%s-dev%d",
					    indio_dev->name, indio_dev->id);

	if (data->trig == NULL)
		return -ENOMEM;

	ret = devm_request_irq(&indio_dev->dev, irq,
			       &iio_trigger_generic_data_rdy_poll,
			       irq_type, "bmi160", data->trig);
	if (ret)
		return ret;

	data->trig->dev.parent = regmap_get_device(data->regmap);
	data->trig->ops = &bmi160_trigger_ops;
	iio_trigger_set_drvdata(data->trig, indio_dev);

	ret = devm_iio_trigger_register(&indio_dev->dev, data->trig);
	if (ret)
		return ret;

	indio_dev->trig = iio_trigger_get(data->trig);

	return 0;
}

static void bmi160_chip_uninit(void *data)
{
	struct bmi160_data *bmi_data = data;
	struct device *dev = regmap_get_device(bmi_data->regmap);
	int ret;
#if defined(CONFIG_SOC_SP7350)
	int err;
#endif

	bmi160_set_mode(bmi_data, BMI160_GYRO, false);
	bmi160_set_mode(bmi_data, BMI160_ACCEL, false);

	ret = regulator_bulk_disable(ARRAY_SIZE(bmi_data->supplies),
				     bmi_data->supplies);
	if (ret)
		dev_err(dev, "Failed to disable regulators: %d\n", ret);

#if defined(CONFIG_SOC_SP7350)
	if (bmi_data->gpio_pwr_on > 0) {
		err = gpio_direction_output(bmi_data->gpio_pwr_on, 0);
		if (err) {
			printk("%s: IMU_PWR_EN didn't output high\n", __FUNCTION__);
		}
		gpio_free(bmi_data->gpio_pwr_on);
	}
#endif
}

int bmi160_core_probe(struct device *dev, struct regmap *regmap,
		      const char *name, bool use_spi)
{
	struct iio_dev *indio_dev;
	struct bmi160_data *data;
	int irq;
	enum bmi160_int_pin int_pin;
	int ret;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	dev_set_drvdata(dev, indio_dev);
	data->regmap = regmap;
#if defined(CONFIG_SOC_SP7350)
	data->watermark_hwfifo = 128;

	data->gpio_pwr_on = of_get_named_gpio(dev->of_node, "gpio_pwr_on", 0);
#endif

	data->supplies[0].supply = "vdd";
	data->supplies[1].supply = "vddio";
	ret = devm_regulator_bulk_get(dev,
				      ARRAY_SIZE(data->supplies),
				      data->supplies);
	if (ret) {
		dev_err(dev, "Failed to get regulators: %d\n", ret);
		return ret;
	}

	ret = iio_read_mount_matrix(dev, "mount-matrix",
				    &data->orientation);
	if (ret)
		return ret;

	ret = bmi160_chip_init(data, use_spi);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(dev, bmi160_chip_uninit, data);
	if (ret)
		return ret;

	if (!name && ACPI_HANDLE(dev))
		name = bmi160_match_acpi_device(dev);

	indio_dev->channels = bmi160_channels;
	indio_dev->num_channels = ARRAY_SIZE(bmi160_channels);
	indio_dev->name = name;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &bmi160_info;

	ret = devm_iio_triggered_buffer_setup(dev, indio_dev,
					      iio_pollfunc_store_time,
					      bmi160_trigger_handler, NULL);
	if (ret)
		return ret;

	irq = bmi160_get_irq(dev->of_node, &int_pin);
	if (irq > 0) {
		ret = bmi160_setup_irq(indio_dev, irq, int_pin);
		if (ret)
			dev_err(&indio_dev->dev, "Failed to setup IRQ %d\n",
				irq);
#if defined(CONFIG_SOC_SP7350)
		else
			irq_set_affinity(irq, cpumask_of(3));
#endif
	} else {
		dev_info(&indio_dev->dev, "Not setting up IRQ trigger\n");
	}

#if defined(CONFIG_SOC_SP7350)
	INIT_WORK(&bmi_work, work_handler);
	mutex_init(&data->bmi_lock);

	timer_setup(&data->bmi_tmr, bmi_on_timeout, 0);
	data->bmi_tmr.expires = jiffies + msecs_to_jiffies(30);

	g_bmi_data = data;
	g_indio_dev = indio_dev;
#endif

	return devm_iio_device_register(dev, indio_dev);
}
EXPORT_SYMBOL_GPL(bmi160_core_probe);

MODULE_AUTHOR("Daniel Baluta <daniel.baluta@intel.com>");
MODULE_DESCRIPTION("Bosch BMI160 driver");
MODULE_LICENSE("GPL v2");
