// SPDX-License-Identifier: GPL-2.0
/*
 * 3-axis accelerometer driver supporting following Bosch-Sensortec chips:
 *  - BMI088
 *
 * Copyright (c) 2018-2021, Topic Embedded Products
 */
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/regulator/consumer.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include "bmi088.h"
#include <linux/gpio.h>
#include <soc/vicore/timer.h>
#include <soc/vicore/timer.h>
#include "../../../pinctrl/vicore/vcpctl.h"

void vc_gpio_int_clear(AO_INT_IRQ_NUM irq_num);

#define CSB1	53
#define CSB2	56
#define DUMMY_LEN	1

#define BMI088_ACCEL_REG_TEMP_SHIFT			5
#define BMI088_ACCEL_TEMP_UNIT				125
#define BMI088_ACCEL_TEMP_OFFSET			23000
#define BMI088_ACCEL_REG_XOUT_L				0x12
#define BMI088_ACCEL_AXIS_TO_REG(axis) \
	(BMI088_ACCEL_REG_XOUT_L + ((axis) * 2))
#define BMI088_ACCEL_SYNC_AXIS_XY_TO_REG(axis) \
	(BMI08_REG_ACCEL_GP_0 + ((axis) * 2))
#define BMI088_ACCEL_SYNC_AXIS_Z_TO_REG(axis) \
	(BMI08_REG_ACCEL_GP_4 + ((axis) * 2))
#define BMI088_GYRO_AXIS_TO_REG(axis) \
	(BMI08_REG_GYRO_X_LSB + ((axis) * 2))
#define BMIO088_ACCEL_ACC_RANGE_MSK			GENMASK(1, 0)
#define BMIO088_GYRO_ACC_RANGE_MSK			GENMASK(2, 0)

#define ONE_ACC_FRAME_LEN	7
#define ONE_GYRO_FRAME_LEN	6
#define WARTER_MARK			50
#define WARTER_MARK_MAX_LEN 0x3FF

const uint8_t *config_file_ptr;

static const int bmi088_sample_freqs[] = {
	12, 500000,
	25, 0,
	50, 0,
	100, 0,
	200, 0,
	400, 0,
	800, 0,
	1600, 0,
};
static const int bmi088_gyro_sample_freqs[] = {
	2000, 0,
	2000, 0,
	1000, 0,
	400, 0,
	200, 0,
	100, 0,
	200, 0,
	100, 0,
};
enum IMU_ID {
    IMU0 = 0,
    IMU1 = 1,
    IMU2 = 2,
    IMU3 = 3,
};
/* Available OSR (over sampling rate) sets the 3dB cut-off frequency */
enum bmi088_osr_modes {
	BMI088_ACCEL_MODE_OSR_NORMAL = 0xA,
	BMI088_ACCEL_MODE_OSR_2 = 0x9,
	BMI088_ACCEL_MODE_OSR_4 = 0x8,
};
/* Available ODR (output data rates) in Hz */

enum bmi088_odr_modes {
	BMI088_ACCEL_MODE_ODR_12_5 = 0x5,
	BMI088_ACCEL_MODE_ODR_25 = 0x6,
	BMI088_ACCEL_MODE_ODR_50 = 0x7,
	BMI088_ACCEL_MODE_ODR_100 = 0x8,
	BMI088_ACCEL_MODE_ODR_200 = 0x9,
	BMI088_ACCEL_MODE_ODR_400 = 0xa,
	BMI088_ACCEL_MODE_ODR_800 = 0xb,
	BMI088_ACCEL_MODE_ODR_1600 = 0xc,
};

#define BMI088_CHANNEL(_type, _axis, _index) {			\
	.type = _type,						\
	.modified = 1,						\
	.channel2 = IIO_MOD_##_axis,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) |  \
		BIT(IIO_CHAN_INFO_SAMP_FREQ),			\
	.info_mask_shared_by_type_available = BIT(IIO_CHAN_INFO_SAMP_FREQ) | \
				BIT(IIO_CHAN_INFO_SCALE), \
	.scan_index = _index,					\
	.scan_type = {						\
		.sign = 's',					\
		.realbits = 16,					\
		.storagebits = 16,				\
		.endianness = IIO_LE,				\
	},							\
}
/* scan indexes follow DATA register order */
enum bmi088_scan_axis {
	BMI088_SCAN_GYRO_X = 0,
	BMI088_SCAN_GYRO_Y,
	BMI088_SCAN_GYRO_Z,
	BMI088_SCAN_ACCEL_X,
	BMI088_SCAN_ACCEL_Y,
	BMI088_SCAN_ACCEL_Z,
	BMI088_SCAN_TIMESTAMP,
};

static const struct iio_chan_spec bmi088_channels[] = {
	BMI088_CHANNEL(IIO_ACCEL, X, BMI088_SCAN_ACCEL_X),
	BMI088_CHANNEL(IIO_ACCEL, Y, BMI088_SCAN_ACCEL_Y),
	BMI088_CHANNEL(IIO_ACCEL, Z, BMI088_SCAN_ACCEL_Z),
	BMI088_CHANNEL(IIO_ANGL_VEL, X, BMI088_SCAN_GYRO_X),
	BMI088_CHANNEL(IIO_ANGL_VEL, Y, BMI088_SCAN_GYRO_Y),
	BMI088_CHANNEL(IIO_ANGL_VEL, Z, BMI088_SCAN_GYRO_Z),
	IIO_CHAN_SOFT_TIMESTAMP(BMI088_SCAN_TIMESTAMP),
};

enum bmi088_sensor_type {
	BMI088_ACCEL	= 0,
	BMI088_GYRO,
	BMI088_EXT_MAGN,
	BMI088_NUM_SENSORS /* must be last */
};
struct bmi088_scale {
	u8 bits;
	int uscale;
};

struct bmi088_odr {
	u8 bits;
	int odr;
	int uodr;
};
static const struct bmi088_scale bmi088_accel_scale[] = {
	{ BMI08_ACCEL_RANGE_3G, 897},
	{ BMI08_ACCEL_RANGE_6G, 1794},
	{ BMI08_ACCEL_RANGE_12G, 3589},
	{ BMI08_ACCEL_RANGE_24G, 7178},
};

static const struct bmi088_scale bmi088_gyro_scale[] = {
	{ BMI08_GYRO_RANGE_2000_DPS, 1065},
	{ BMI08_GYRO_RANGE_1000_DPS, 532},
	{ BMI08_GYRO_RANGE_500_DPS, 266},
	{ BMI08_GYRO_RANGE_250_DPS, 133},
	{ BMI08_GYRO_RANGE_125_DPS, 66},
};
static const int acc_scale_table[4][2] = {{0, 897}, {0, 1794}, {0, 3589}, {0, 7178}};
static const int gyro_scale_table[5][2] = {{0, 1065}, {0, 532}, {0, 266}, {0, 133}, {0, 66}};
struct bmi088_scale_item {
	const struct bmi088_scale *tbl;
	int num;
};

static const struct  bmi088_scale_item bmi088_scale_table[] = {
	[BMI088_ACCEL] = {
		.tbl	= bmi088_accel_scale,
		.num	= ARRAY_SIZE(bmi088_accel_scale),
	},
	[BMI088_GYRO] = {
		.tbl	= bmi088_gyro_scale,
		.num	= ARRAY_SIZE(bmi088_gyro_scale),
	},
};

struct bmi088_scale_info {
	int scale;
	u8 reg_range;
};

struct bmi088_chip_info {
	const char *name;
	u8 chip_id[2];
	const struct iio_chan_spec *channels;
	int num_channels;
};

static const struct bmi088_chip_info bmi088_chip_info_tbl = {
	.name = "bmi088",
	.chip_id = {0x1e, 0x0f},
	.channels = bmi088_channels,
	.num_channels = ARRAY_SIZE(bmi088_channels),
};

struct bmi088_data {
	struct regmap *regmap;
	const struct bmi088_chip_info *chip_info;
	__le16 buf[12] __aligned(8);
	u8 buffer[4]__aligned(4); /* shared DMA safe buffer *///todo
	struct iio_trigger *trig;
	struct regulator_bulk_data supplies[2];
	struct iio_mount_matrix orientation;
	u16 fifo_length;
	u64 odr_acc;
	u64 odr_gyro;
	u8 hwfifo_mode;
	bool use_high_freq;
	u8 watermark_hwfifo;
	u64 ts_cur;
	u64 ts_last;
	bool sync_mode;
	u8 freq_div;
	u8 sync_freq;
	u8 acc_freq;
	u8 gyro_freq;
	u32 imu_tsp_id;
	u64 max_t;
	u64 threhold_t;
	unsigned char acc_fifo_data[1024 + DUMMY_LEN] __aligned(4);
	unsigned char gyro_fifo_data[1024 + DUMMY_LEN] __aligned(4);
	struct mutex bmi_lock;
};

static const struct regmap_range bmi088_volatile_ranges[] = {
	/* All registers below 0x40 are volatile, except the CHIP ID. */
	regmap_reg_range(BMI08_REG_ACCEL_ERR, 0x3f),
	/* Mark the RESET as volatile too, it is self-clearing */
	regmap_reg_range(BMI08_REG_ACCEL_SOFTRESET, BMI08_REG_ACCEL_SOFTRESET),
};

static inline int regmap_read_csb(int csb, struct regmap *map, unsigned int reg,
			      unsigned int *val)
{
	int ret;
	if (csb == CSB1) {
		gpio_set_value(csb, 0);
		ret = regmap_read(map, reg, val);
		gpio_set_value(csb, 1);
		usleep_range(10, 20);
		gpio_set_value(csb, 0);
		ret = regmap_read(map, reg, val);
		gpio_set_value(csb, 1);
	}
	else {
		gpio_set_value(csb, 0);
		ret = regmap_read(map, reg, val);
		usleep_range(10, 20);
		gpio_set_value(csb, 1);
	}
	
	return ret;
}

int regmap_bulk_read_acc_data(struct regmap *map, unsigned int reg, void *val,
		     size_t val_count)
{
	int ret;
	gpio_set_value(CSB1, 0);
	ret = regmap_bulk_read(map, reg, val, val_count);
	gpio_set_value(CSB1, 1);
	return ret;
}

int regmap_bulk_read_csb(int csb, struct regmap *map, unsigned int reg, void *val,
		     size_t val_count)
{
	int ret;
	u8* tmp;
	gpio_set_value(csb, 0);
	if (csb == CSB1) {
		int i;
		ret = regmap_bulk_read(map, reg, val, val_count + DUMMY_LEN);
		tmp = val;
		for (i = 0; i < val_count; i++) {
			tmp[i] = tmp[i+1];
		}
		tmp[i] = 0;
	}
	else {
		ret = regmap_bulk_read(map, reg, val, val_count);
	}
	gpio_set_value(csb, 1);
	return ret;
}

int regmap_bulk_read_fifo_csb(int csb, struct regmap *map, unsigned int reg, void *val,
		     size_t val_count)
{
	int ret;
	gpio_set_value(csb, 0);
	if (csb == CSB1) {
		ret = regmap_bulk_read(map, reg, val, val_count + DUMMY_LEN);
	}
	else {
		ret = regmap_bulk_read(map, reg, val, val_count);
	}
	gpio_set_value(csb, 1);
	return ret;
}

static int regmap_read_2bytes_csb(int csb, struct regmap *map, unsigned int reg, int *val)
{
	int ret;
	u8 tmp[3] = {0};//should be 3 for CSB1
	ret = regmap_bulk_read_csb(csb, map, reg, tmp, 2);
	if (ret)
		return ret;
	*val = sign_extend32(le16_to_cpu(*(__le16 *)tmp), 15);
	return IIO_VAL_INT;
}

static inline int regmap_write_csb(int csb, struct regmap *map, unsigned int reg,
			       unsigned int val)
{
	int ret;
	gpio_set_value(csb, 0);
	ret = regmap_write(map, reg, val);
	gpio_set_value(csb, 1);
	// usleep_range(100, 200);
	udelay(100);
	return ret;
}

static inline int regmap_bulk_write_csb(int csb, struct regmap *map, unsigned int reg,
				    const void *val, size_t val_count)
{
	int ret;
	gpio_set_value(csb, 0);
	ret = regmap_bulk_write(map, reg, val, val_count);
	gpio_set_value(csb, 1);
	usleep_range(100, 200);
	return ret;
}

static int bmi088_accel_power_up(struct regmap *map)
{
	int ret;
	/* Enable accelerometer and temperature sensor */
	ret = regmap_write_csb(CSB1, map, BMI08_REG_ACCEL_PWR_CTRL, BMI08_ACCEL_POWER_ENABLE);
	if (ret)
		return ret;
	/* Datasheet recommends to wait at least 5ms before communication */
	usleep_range(5000, 6000);
	/* Disable suspend mode */
	ret = regmap_write_csb(CSB1, map, BMI08_REG_ACCEL_PWR_CONF, BMI08_ACCEL_PM_ACTIVE);
	if (ret)
		return ret;
	/* Recommended at least 1ms before further communication */
	usleep_range(1000, 1200);
	return 0;
}
static int bmi088_accel_power_down(struct regmap *map)
{
	int ret;
	/* Enable suspend mode */
	ret = regmap_write_csb(CSB1, map, BMI08_REG_ACCEL_PWR_CONF, BMI08_ACCEL_PM_SUSPEND);
	if (ret)
		return ret;
	/* Recommended at least 1ms before further communication */
	usleep_range(1000, 1200);
	/* Disable accelerometer and temperature sensor */
	ret = regmap_write_csb(CSB1, map, BMI08_REG_ACCEL_PWR_CTRL, BMI08_ACCEL_POWER_DISABLE);
	if (ret)
		return ret;
	/* Datasheet recommends to wait at least 5ms before communication */
	usleep_range(5000, 6000);
	return 0;
}
static int bmi088_gyro_power_up(struct regmap *map)
{
	int ret;
	/* Normal mode */
	ret = regmap_write_csb(CSB2, map, BMI08_REG_GYRO_LPM1, BMI08_GYRO_PM_NORMAL);
	if (ret)
		return ret;
	/* Recommended at least 1ms before further communication */
	usleep_range(1000, 1200);
	return 0;
}
static int bmi088_gyro_power_down(struct regmap *map)
{
	int ret;
	/* Suspend mode */
	ret = regmap_write_csb(CSB2, map, BMI08_REG_GYRO_LPM1, BMI08_GYRO_PM_SUSPEND);
	if (ret)
		return ret;
	/* Datasheet recommends to wait at least 5ms before communication */
	usleep_range(5000, 6000);
	return 0;
}

static int bmi088_accel_get_sample_freq(struct bmi088_data *data,
					int *val, int *val2)
{
	unsigned int value;
	int ret;
	ret = regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_CONF,
			  &value);
	if (ret)
		return ret;
	//printk("test_ %s, reg 0x%x:0x%x", __FUNCTION__, BMI08_REG_ACCEL_CONF, value);
	value &= BMI08_ACCEL_ODR_MASK;
	value -= BMI088_ACCEL_MODE_ODR_12_5;
	value <<= 1;
	if (value >= ARRAY_SIZE(bmi088_sample_freqs) - 1)
		return -EINVAL;
	*val = bmi088_sample_freqs[value];
	*val2 = bmi088_sample_freqs[value + 1];
	return IIO_VAL_INT_PLUS_MICRO;
}

#define BMI088_EDGE_TRIGGERED	BIT(0)
#define BMI088_ACTIVE_HIGH		BIT(1)
#define BMI088_OPEN_DRAIN		BIT(2)
#define BMI088_OUTPUT_EN		BIT(3)

static int bmi088_set_int1_to_input_pin(struct bmi088_data *data, bool active_high) {
	int ret, val;
	if (active_high) {
		/* Config acc int1 to input pin, push-pull, active high */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_IO_CONF, BMI08_ACCEL_INT_IN_MASK | BMI08_ACCEL_INT_LVL_MASK);
	}
	else {
		/* Config acc int1 to input pin, push-pull, active low */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_IO_CONF, BMI08_ACCEL_INT_IN_MASK);
	}
	regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_IO_CONF, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_ACCEL_INT1_IO_CONF, val);
	return ret;
}

#if 0
static int bmi088_rset_int1_pin(struct bmi088_data *data, bool active_high) {
	int ret, val;
	if (active_high) {
		/* Config acc int1 active high, not input and output */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_IO_CONF, BMI08_ACCEL_INT_LVL_MASK);
	}
	else {
		/* Config acc int1 active low, not input and output */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_IO_CONF, 0);
	}
	regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_IO_CONF, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_ACCEL_INT1_IO_CONF, val);
	return ret;
}
#endif

/* Config data synchronization */
static int bmi088_data_sync_cfg(struct bmi088_data *data, int sync_freq) {
	int ret;
	u8 sync_len = 6;
	u8 tmpr[7];
	ret = regmap_bulk_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_FEATURE_CFG, tmpr, sync_len);
	// int i = 0;
	// for (i = 0; i < 6; i++) {
	// 	printk("test_ val[%d]:0x%x\n", i, tmpr[i]);
	// }
	tmpr[4] = sync_freq;

	ret = regmap_bulk_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_FEATURE_CFG, tmpr, sync_len);
	if (ret)
		return ret;
	ret = regmap_bulk_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_FEATURE_CFG, tmpr, sync_len);
	// for (i = 0; i < 6; i++) {
	// 	printk("test_ update val[%d]:0x%x\n", i, tmpr[i]);
	// }
	usleep_range(110000, 200000);
	return ret;
}

#if 0
/* Enable data synchronization mode*/
static int bmi088_sync_mode_enable(struct bmi088_data *data, int sync_freq) {
	int ret;
	ret = bmi088_set_int1_to_input_pin(data, true);
	ret |= bmi088_data_sync_cfg(data, sync_freq);
	if (ret)
		return ret;
	printk("%s sync_freq:%d\n", __FUNCTION__, sync_freq);
	data->sync_mode = true;
	return ret;
}

/* Disable data synchronization mode*/
static int bmi088_sync_mode_disable(struct bmi088_data *data) {
	int ret;
	ret = bmi088_rset_int1_pin(data, true);
	/* Disable data synchronization */
	ret |= bmi088_data_sync_cfg(data, BMI08_ACCEL_DATA_SYNC_MODE_OFF);
	printk("%s\n", __FUNCTION__);
	data->sync_mode = false;
	return ret;
}
#endif

static int bmi088_accel_set_sample_freq(struct bmi088_data *data, int val)
{
	unsigned int regval;
	int index = 0;
	while (index < ARRAY_SIZE(bmi088_sample_freqs) &&
	       bmi088_sample_freqs[index] != val)
		index += 2;
	if (index >= ARRAY_SIZE(bmi088_sample_freqs))
		return -EINVAL;
	regval = (index >> 1) + BMI088_ACCEL_MODE_ODR_12_5;
	if (bmi088_sample_freqs[index] >= 400) {
		switch (bmi088_sample_freqs[index])
		{
		case 400:
			data->max_t = 1000000000 / 400 * 1.5;
			data->threhold_t = 1000000000 / 400 * 0.9;
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
			data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
			data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
			break;
		case 800:
			data->max_t = 1000000000 / 1000 * 1.5;
			data->threhold_t = 1000000000 / 1000 * 0.9;
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_1000HZ;
			data->acc_freq = BMI08_ACCEL_ODR_800_HZ;
			data->gyro_freq = BMI08_GYRO_BW_116_ODR_1000_HZ;
			break;
		case 1600:
			data->max_t = 1000000000 / 2000 * 1.5;
			data->threhold_t = 0;
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_2000HZ;
			data->acc_freq = BMI08_ACCEL_ODR_1600_HZ;
			data->gyro_freq = BMI08_GYRO_BW_532_ODR_2000_HZ;
			break;
		default:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
			data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
			data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
			data->max_t = 1000000000 / 400 * 1.5;
			data->threhold_t = 1000000000 / 400 * 0.9;
			break;
		}
		data->freq_div = 1;
		if (data->use_high_freq) {
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_2000HZ;
			data->acc_freq = BMI08_ACCEL_ODR_1600_HZ;
			data->gyro_freq = BMI08_GYRO_BW_532_ODR_2000_HZ;
		}
	}
	else {
		switch (bmi088_gyro_sample_freqs[index])
		{
		case 200:
			data->max_t = 1000000000 / 200 * 1.5;
			data->threhold_t = 1000000000 / 200 * 0.9;
			break;
		default:
			data->max_t = 1000000000 / 100 * 1.5;
			data->threhold_t = 1000000000 / 100 * 0.9;
			break;
		}
		data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
		data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
		data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
		data->freq_div = 400 / bmi088_sample_freqs[index];
	}
	data->odr_acc = bmi088_sample_freqs[index] * 1000000;
	return 0;
	// return regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_CONF,
	// 	(BMI08_ACCEL_CONFIG_MASK | BMI08_ACCEL_BWP_NORMAL | (regval & BMI08_ACCEL_ODR_MASK)));
}
static int bmi088_accel_set_scale(struct bmi088_data *data, int val, int val2)
{
	unsigned int i;
	for (i = 0; i < bmi088_scale_table[BMI088_ACCEL].num; i++)
		if (bmi088_scale_table[BMI088_ACCEL].tbl[i].uscale == val2)
			break;

	if (i == bmi088_scale_table[BMI088_ACCEL].num)
		return -EINVAL;
	return regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_RANGE, bmi088_scale_table[BMI088_ACCEL].tbl[i].bits);
}
static int bmi088_accel_get_temp(struct bmi088_data *data, int *val)
{
	int ret;
	s16 temp;
	ret = regmap_bulk_read_csb(CSB1, data->regmap, BMI08_REG_TEMP_MSB,
			       &data->buffer, sizeof(__be16));
	if (ret)
		return ret;
	/* data->buffer is cacheline aligned */
	temp = be16_to_cpu(*(__be16 *)data->buffer);
	*val = temp >> BMI088_ACCEL_REG_TEMP_SHIFT;
	return IIO_VAL_INT;
}

static int bmi088_gyro_get_sample_freq(struct bmi088_data *data,
					int *val, int *val2)
{
	unsigned int value;
	int ret;
	ret = regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_BANDWIDTH,
			  &value);
	if (ret)
		return ret;
	//printk("%s, reg 0x%x:0x%x", __FUNCTION__, BMI08_REG_GYRO_BANDWIDTH, value);
	value &= BMI08_GYRO_RANGE_MASK;
	value -= BMI08_GYRO_BW_532_ODR_2000_HZ;
	value <<= 1;
	if (value >= ARRAY_SIZE(bmi088_gyro_sample_freqs) - 1)
		return -EINVAL;
	*val = bmi088_gyro_sample_freqs[value];
	*val2 = bmi088_gyro_sample_freqs[value + 1];
	return IIO_VAL_INT_PLUS_MICRO;
}

static int bmi088_gyro_set_sample_freq(struct bmi088_data *data, int val)
{
	unsigned int regval;
	int index = 0;
	while (index < ARRAY_SIZE(bmi088_gyro_sample_freqs) &&
	       bmi088_gyro_sample_freqs[index] != val)
		index += 2;
	if (index >= ARRAY_SIZE(bmi088_gyro_sample_freqs))
		return -EINVAL;
	regval = (index >> 1) + BMI08_GYRO_BW_532_ODR_2000_HZ;
	if (bmi088_gyro_sample_freqs[index] >= 400) {
		switch (bmi088_gyro_sample_freqs[index])
		{
		case 400:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
			data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
			data->max_t = 1000000000 / 400 * 1.5;
			data->threhold_t = 1000000000 / 400 * 0.9;
			break;
		case 1000:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_1000HZ;
			data->gyro_freq = BMI08_GYRO_BW_116_ODR_1000_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_800_HZ;
			data->max_t = 1000000000 / 1000 * 1.5;
			data->threhold_t = 1000000000 / 1000 * 0.9;
			break;
		case 2000:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_2000HZ;
			data->gyro_freq = BMI08_GYRO_BW_532_ODR_2000_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_1600_HZ;
			data->max_t = 1000000000 / 2000 * 1.5;
			data->threhold_t = 0;
			break;
		default:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
			data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
			data->max_t = 1000000000 / 400 * 1.5;
			data->threhold_t = 1000000000 / 400 * 0.9;
			break;
		}
		data->freq_div = 1;
		if (data->use_high_freq) {
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_2000HZ;
			data->gyro_freq = BMI08_GYRO_BW_532_ODR_2000_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_1600_HZ;
		}
	}
	else {
		switch (bmi088_gyro_sample_freqs[index])
		{
		case 200:
			data->max_t = 1000000000 / 200 * 1.5;
			data->threhold_t = 1000000000 / 200 * 0.9;
			break;
		default:
			data->max_t = 1000000000 / 100 * 1.5;
			data->threhold_t = 1000000000 / 100 * 0.9;
			break;
		}
		data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
		data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
		data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
		data->freq_div = 400 / bmi088_gyro_sample_freqs[index];
	}
	data->odr_gyro = bmi088_gyro_sample_freqs[index] * 1000000;
	//printk("%s, val 0x%x", __FUNCTION__, val);
	return 0;
	// return regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_BANDWIDTH,
	// 		   BMI08_ACCEL_ODR_MASK & regval);
}
static int bmi088_gyro_set_scale(struct bmi088_data *data, int val, int val2)
{
	unsigned int i;
	for (i = 0; i < bmi088_scale_table[BMI088_GYRO].num; i++)
		if (bmi088_scale_table[BMI088_GYRO].tbl[i].uscale == val2)
			break;

	if (i == bmi088_scale_table[BMI088_GYRO].num)
		return -EINVAL;

	return regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_RANGE, bmi088_scale_table[BMI088_GYRO].tbl[i].bits);
}
static int bmi088_accel_get_axis(struct bmi088_data *data,
				 struct iio_chan_spec const *chan,
				 int *val)
{
	int ret;
	ret = regmap_read_2bytes_csb(CSB1, data->regmap,
			       BMI088_ACCEL_AXIS_TO_REG(chan->scan_index - BMI088_SCAN_ACCEL_X), val);
	if (ret)
		return ret;
	return IIO_VAL_INT;
}

static int bmi088_accel_get_sync_axis(struct bmi088_data *data,
				 struct iio_chan_spec const *chan,
				 int *val)
{
	int ret;
	if (chan->scan_index == BMI088_SCAN_ACCEL_Z) {
		ret = regmap_read_2bytes_csb(CSB1, data->regmap, 
					BMI088_ACCEL_SYNC_AXIS_Z_TO_REG(chan->scan_index - BMI088_SCAN_ACCEL_Z), val);

	}
	else {
		ret = regmap_read_2bytes_csb(CSB1, data->regmap, 
					BMI088_ACCEL_SYNC_AXIS_XY_TO_REG(chan->scan_index - BMI088_SCAN_ACCEL_X), val);
	}	
	if (ret)
		return ret;
	return IIO_VAL_INT;
}

static int bmi088_gyro_get_axis(struct bmi088_data *data,
				 struct iio_chan_spec const *chan,
				 int *val)
{
	int ret;
	ret = regmap_read_2bytes_csb(CSB2, data->regmap, 
					BMI088_GYRO_AXIS_TO_REG(chan->scan_index - BMI088_SCAN_GYRO_X), val);
	if (ret)
		return ret;
	return IIO_VAL_INT;
}


static int bmi088_read_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int *val, int *val2, long mask)
{
	//printk("test_ %s:chan->scan_index: %d, chan->type: %d, mask:0x%x\n", __FUNCTION__, chan->scan_index, chan->type, mask);
	struct bmi088_data *data = iio_priv(indio_dev);
	struct device *dev;
	int ret, i;
	int reg;
	dev = regmap_get_device(data->regmap);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->type) {
		case IIO_TEMP:
			ret = bmi088_accel_get_temp(data, val);
			goto out_read_raw_pm_put;
		case IIO_ACCEL:
			ret = iio_device_claim_direct_mode(indio_dev);
			if (ret)
				goto out_read_raw_pm_put;
			ret = bmi088_accel_get_sync_axis(data, chan, val);
			//printk("test_ %s:IIO_CHAN_INFO_RAW IIO_ACCEL, sync val:%d\n", __FUNCTION__, *val);

			ret = bmi088_accel_get_axis(data, chan, val);
			//printk("test_ %s:IIO_CHAN_INFO_RAW IIO_ACCEL, val:%d\n", __FUNCTION__, *val);
			iio_device_release_direct_mode(indio_dev);

			if (!ret)
				ret = IIO_VAL_INT;
			goto out_read_raw_pm_put;
		case IIO_ANGL_VEL:
			ret = iio_device_claim_direct_mode(indio_dev);
			if (ret)
				goto out_read_raw_pm_put;
			ret = bmi088_gyro_get_axis(data, chan, val);
			//printk("test_ %s:IIO_CHAN_INFO_RAW IIO_ANGL_VEL, val:%d\n", __FUNCTION__, *val);
			iio_device_release_direct_mode(indio_dev);
			if (!ret)
				ret = IIO_VAL_INT;
			goto out_read_raw_pm_put;
		default:
			return -EINVAL;
			break;
		}
	case IIO_CHAN_INFO_OFFSET:
		switch (chan->type) {
		case IIO_TEMP:
			/* Offset applies before scale */
			*val = BMI088_ACCEL_TEMP_OFFSET/BMI088_ACCEL_TEMP_UNIT;
			return IIO_VAL_INT;
			break;
		default:
			return -EINVAL;
			break;
		}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_TEMP:
			/* 0.125 degrees per LSB */
			*val = BMI088_ACCEL_TEMP_UNIT;
			return IIO_VAL_INT;
			break;
		case IIO_ACCEL:
			ret = regmap_read_csb(CSB1, data->regmap,
					  BMI08_REG_ACCEL_RANGE, &reg);
			//printk("test_ %s:IIO_CHAN_INFO_SCALE IIO_ACCEL, reg 0x%x:%x\n", __FUNCTION__, BMI08_REG_ACCEL_RANGE, reg);
			if (ret)
				goto out_read_raw_pm_put;
			reg = FIELD_GET(BMIO088_ACCEL_ACC_RANGE_MSK, reg);
			for (i = 0; i < bmi088_scale_table[BMI088_ACCEL].num; i++)
				if (bmi088_scale_table[BMI088_ACCEL].tbl[i].bits == reg) {
					*val  = 0;
					*val2 = bmi088_scale_table[BMI088_ACCEL].tbl[i].uscale;
					//printk("test_ %s:IIO_CHAN_INFO_SCALE IIO_ACCEL1, num %d: i %d val2 %d\n", __FUNCTION__, bmi088_scale_table[BMI088_ACCEL].num, i, *val2);
					ret = IIO_VAL_INT_PLUS_MICRO;
					goto out_read_raw_pm_put;
				}
			return -EINVAL;
			break;
		case IIO_ANGL_VEL:
			ret = regmap_read_csb(CSB2, data->regmap,
					  BMI08_REG_GYRO_RANGE, &reg);
			//printk("test_ %s:IIO_CHAN_INFO_SCALE IIO_ANGL_VEL, reg 0x%x:%x\n", __FUNCTION__, BMI08_REG_GYRO_RANGE, reg);
			if (ret)
				goto out_read_raw_pm_put;
			reg = FIELD_GET(BMIO088_GYRO_ACC_RANGE_MSK, reg);
			for (i = 0; i < bmi088_scale_table[BMI088_GYRO].num; i++)
				if (bmi088_scale_table[BMI088_GYRO].tbl[i].bits == reg) {
					*val  = 0;
					*val2 = bmi088_scale_table[BMI088_GYRO].tbl[i].uscale;
					ret = IIO_VAL_INT_PLUS_MICRO;
					goto out_read_raw_pm_put;
				}
			return -EINVAL;
			break;
		default:
			return -EINVAL;
			break;
		}
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch (chan->type) {
		case IIO_ACCEL:
			ret = bmi088_accel_get_sample_freq(data, val, val2);
			//printk("test_ %s:IIO_CHAN_INFO_SAMP_FREQ IIO_ACCEL val:0x%x, val2:%x\n", __FUNCTION__, *val, *val2);
			goto out_read_raw_pm_put;
		case IIO_ANGL_VEL:
			ret = bmi088_gyro_get_sample_freq(data, val, val2);
			//printk("test_ %s:IIO_CHAN_INFO_SAMP_FREQ IIO_ANGL_VEL val:0x%x, val2:%x\n", __FUNCTION__, *val, *val2);
			goto out_read_raw_pm_put;
		default:
			return -EINVAL;
		}
	default:
		break;
	}
	return -EINVAL;
out_read_raw_pm_put:

	return ret;
}
static int bmi088_read_avail(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     const int **vals, int *type, int *length,
			     long mask)
{
	//printk("test_ %s:IIO_CHAN_INFO_SAMP_FREQ IIO_ANGL_VEL mask:0x%x, chan->type:0x%x\n", __FUNCTION__, mask, chan->type);

	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ACCEL:
			*vals = (const int *)acc_scale_table;//(const int *)data->chip_info->scale_table;
			*length = ARRAY_SIZE(acc_scale_table) * 2;
			break;
		case IIO_ANGL_VEL:
			*vals = (const int *)gyro_scale_table;//(const int *)data->gyro_chip_info->scale_table;
			*length = ARRAY_SIZE(gyro_scale_table) * 2;
			break;
		default:
			return -EINVAL;
		}
		*type = IIO_VAL_INT_PLUS_MICRO;
		return IIO_AVAIL_LIST;
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch (chan->type) {
		case IIO_ACCEL:
			*vals = bmi088_sample_freqs;
			*length = ARRAY_SIZE(bmi088_sample_freqs);
			break;
		case IIO_ANGL_VEL:
			*vals = bmi088_gyro_sample_freqs;
			*length = ARRAY_SIZE(bmi088_gyro_sample_freqs);
			break;
		default:
			return -EINVAL;
		}
		*type = IIO_VAL_INT_PLUS_MICRO;
		return IIO_AVAIL_LIST;
	default:
		return -EINVAL;
	}
}
static int bmi088_write_raw(struct iio_dev *indio_dev,
				  struct iio_chan_spec const *chan,
				  int val, int val2, long mask)
{
	//printk("test_ bmi088_accel_write_raw:mask:0x%x, chan->type:0x%x, val:%d, val2:%d\n", chan->type, mask, val, val2);
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret;
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ACCEL:
			ret = bmi088_accel_set_scale(data, val, val2);
			break;
		case IIO_ANGL_VEL:
			ret = bmi088_gyro_set_scale(data, val, val2);
			break;
		default:
			return -EINVAL;
		}
		return ret;
	case IIO_CHAN_INFO_SAMP_FREQ:
		switch (chan->type) {
		case IIO_ACCEL:
			ret = bmi088_accel_set_sample_freq(data, val);
			break;
		case IIO_ANGL_VEL:
			ret = bmi088_gyro_set_sample_freq(data, val);
			break;
		default:
			return -EINVAL;
		}
		return ret;
	default:
		return -EINVAL;
	}
}

static int bmi088_accel_set_freq(struct bmi088_data *data, int freq)
{
	int ret;
	ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_CONF,
		(BMI08_ACCEL_CONFIG_MASK | BMI08_ACCEL_BWP_NORMAL | freq));
	usleep_range(40000, 80000);//need to delay 40ms
	return ret;
}

static int bmi088_accel_set_range(struct bmi088_data *data, int range)
{
	int ret;
	ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_RANGE,
		range);
	return ret;
}

static int bmi088_accel_chip_init(struct bmi088_data *data, enum bmi_device_type type)
{
	struct device *dev = regmap_get_device(data->regmap);
	int ret = 0;
	unsigned int val;
	if (type >= BOSCH_UNKNOWN)
		return -ENODEV;
	/* Do a dummy read to enable SPI interface, won't harm I2C */
	regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT_STAT_1, &val);	
	/*
	 * Reset chip to get it in a known good state. A delay of 1ms after
	 * reset is required according to the data sheet
	 */
	ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_SOFTRESET,
			   BMI08_SOFT_RESET_CMD);
	usleep_range(2000, 4000);
	if (ret)
		return ret;
	/* Do a dummy read again after a reset to enable the SPI interface */
	regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT_STAT_1, &val);

	/* Read chip ID */
	ret = regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_CHIP_ID, &val);
	if (ret) {
		dev_err(dev, "Error: Reading chip id\n");
		return ret;
	}
	
	/* Validate acc chip ID */
	if (data->chip_info->chip_id[BMI088_ACCEL] != val)
		dev_warn(dev, "unexpected acc chip id 0x%X\n", val);

	ret = regulator_bulk_enable(ARRAY_SIZE(data->supplies), data->supplies);
	if (ret) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	return 0;
}

static void bmi088_chip_uninit(void *data)
{
	struct bmi088_data *bmi_data = data;
	struct device *dev = regmap_get_device(bmi_data->regmap);
	int ret;
	ret = regulator_bulk_disable(ARRAY_SIZE(bmi_data->supplies),
								bmi_data->supplies);
	if (ret)
		dev_err(dev, "Failed to disable regulators: %d\n", ret);
}

static int bmi088_gyro_set_freq(struct bmi088_data *data, int freq)
{
	int ret;
	ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_BANDWIDTH,
		freq);
	usleep_range(40000, 80000);//need to delay 40ms
	return ret;
}

static int bmi088_gyro_set_range(struct bmi088_data *data, int range)
{
	int ret;
	ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_RANGE,
		range);
	usleep_range(10000, 20000);//need to delay 10ms
	return ret;
}

static int bmi088_gyro_chip_init(struct bmi088_data *data, enum bmi_device_type type)
{
	struct device *dev = regmap_get_device(data->regmap);
	int ret;
	unsigned int val;
	if (type >= BOSCH_UNKNOWN)
		return -ENODEV;
	/* Do a dummy read to enable SPI interface, won't harm I2C */
	regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT_STAT_1, &val);
	//printk("test_ %s:0x%x val:%x\n", __FUNCTION__, BMI08_REG_GYRO_INT_STAT_1, val);
	
	/*
	 * Reset chip to get it in a known good state. A delay of 1ms after
	 * reset is required according to the data sheet
	 */
	// Do not reset for gyro
	// ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_SOFTRESET,
	// 		   BMI08_SOFT_RESET_CMD);
	// usleep_range(1000, 2000);
	// if (ret)
	// 	return ret;
	/* Do a dummy read again after a reset to enable the SPI interface */
	regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT_STAT_1, &val);
	//printk("test_ %s reg:0x%x val after reset:%x\n", __FUNCTION__, BMI08_REG_GYRO_INT_STAT_1, val);

	/* Read chip ID */
	ret = regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_CHIP_ID, &val);
	//printk("test_ %s reg:0x%x val:0x%x\n", __FUNCTION__, BMI08_REG_GYRO_CHIP_ID, val);
	if (ret) {
		dev_err(dev, "Error: Reading gyro chip id\n");
		return ret;
	}
	
	/* Validate acc chip ID */
	if (data->chip_info->chip_id[BMI088_GYRO] != val)
		dev_warn(dev, "unexpected gyro chip id 0x%X\n", val);

	return 0;
}

static int bmi08xa_configure_data_synchronization(struct bmi088_data *data)
{
	int ret;

	ret = bmi088_accel_set_freq(data, data->acc_freq);
	if (ret)
		return ret;
	usleep_range(40000, 80000);
	ret = bmi088_gyro_set_freq(data, data->gyro_freq);
	if (ret)
		return ret;
	usleep_range(10000, 20000);//need to delay 10ms

	ret = bmi088_data_sync_cfg(data, data->sync_freq);
	if (ret)
		return ret;

	//set acc range, default 12G
	ret = bmi088_accel_set_range(data, BMI08_ACCEL_RANGE_12G);
	if (ret)
		return ret;

	//set gyro range, default 1000DPS
	ret = bmi088_gyro_set_range(data, BMI08_GYRO_RANGE_1000_DPS);
	// printk("test_ %s, acc_freq:%d, gyro_freq:%d, sync_freq:%d, freq_div:%d\n", 
	// 		__FUNCTION__, data->acc_freq, data->gyro_freq, data->sync_freq, data->freq_div);
	return ret;
}


static int bmi088_get_irq(struct device_node *of_node)
{
	int irq;

	/* Use INT1 if possible, otherwise fall back to INT2. */
	irq = of_irq_get_byname(of_node, "INT1");
	if (irq > 0) {
		return irq;
	}

	irq = of_irq_get_byname(of_node, "INT2");

	return irq;
}

static int bmi088_set_acc_int_mode(struct bmi088_data *data)
{
	int ret, val;
	if (data->hwfifo_mode) {
		/* Config map data ready interrupt to pin INT2 */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_INT2_MAP_DATA, BMI08_ACCEL_INT2_FWM_MASK);
	}
	else {
		/* Config map watermarker ready interrupt to pin INT2 */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_INT2_MAP_DATA, BMI08_ACCEL_INT2_DRDY_MASK);
	}
	ret |= regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT1_INT2_MAP_DATA, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_ACCEL_INT1_INT2_MAP_DATA, val);
	return ret;
}

#if 0
static int bmi088_enable_acc_interrupt(struct bmi088_data *data, bool enable, bool active_high)
{
	int ret, val;
	
	if (enable) {
		ret = bmi088_set_int1_to_input_pin(data, true);
	}
	else {
		ret = bmi088_rset_int1_pin(data, true);
		return ret;
	}
	if (active_high) {
		/* Config acc int2 to output pin, push-pull, active high */
		ret |= regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT2_IO_CONF, BMI08_ACCEL_INT_IO_MASK | BMI08_ACCEL_INT_LVL_MASK);
	}
	else {
		/* Config acc int2 to output pin, push-pull, active low */
		ret |= regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT2_IO_CONF, BMI08_ACCEL_INT_IO_MASK);
	}
	
	if (ret)
		return ret;
	regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT2_IO_CONF, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_ACCEL_INT2_IO_CONF, val);

	ret = bmi088_set_acc_int_mode(data);
	if (ret)
		return ret;
	
	return ret;
}
#endif

static int bmi088_set_gyro_int_mode(struct bmi088_data *data)
{
	int ret, val;
	bool fifo_mode = false;
	/* Config map data ready interrupt to pin INT3. should always be data ready mode now. */
	if (data->hwfifo_mode & fifo_mode) {
		/* Config map watermarker ready interrupt to pin INT3 */
		ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT3_INT4_IO_MAP, BMI08_GYRO_MAP_FIFO_INT3);
	}
	else {
		/* Config map data ready interrupt to pin INT3 and INT4*/
		ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT3_INT4_IO_MAP, BMI08_GYRO_MAP_DRDY_TO_BOTH_INT3_INT4);
	}
	
	ret |= regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT3_INT4_IO_MAP, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_GYRO_INT3_INT4_IO_MAP, val);
	return ret;
}

static int bmi088_enable_gyro_interrupt(struct bmi088_data *data, bool enable, bool active_high)
{
	int ret = 0, val;
	if (enable) {
		/* Enable gyro int3/4 */
		ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT_CTRL, BMI08_GYRO_INT_EN_MASK);
	}
	else {
		/* Disable gyro int3/4 */
		ret |= regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT_CTRL, BMI08_GYRO_DRDY_INT_DISABLE_VAL);
		return ret;
	}
	if (ret)
		return ret;
	if (active_high) {
		/* Config gyro int3 to output pin, push-pull, active high */
		ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT3_INT4_IO_CONF, BMI08_GYRO_INT3_LVL_MASK | BMI08_GYRO_INT4_LVL_MASK);
	}
	else {
		/* Config gyro int3 to output pin, push-pull, active low */
		ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT3_INT4_IO_CONF, 0);
	}
	if (ret)
		return ret;
	regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT3_INT4_IO_CONF, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_GYRO_INT3_INT4_IO_CONF, val);

	ret = bmi088_set_gyro_int_mode(data);
	
	if (ret)
		return ret;
	ret = regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_INT_CTRL, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_GYRO_INT_CTRL, val);
	return ret;
}

static int bmi088_enable_acc_fifo(struct bmi088_data *data)
{
	int ret, val, wm;
	if (data->hwfifo_mode) {
		/* Fifo config1 fifo enable set  */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_FIFO_CONFIG_1_ADDR, 0x10 | BMI08_ACCEL_EN_MASK | BMI08_ACCEL_INT1_EN_MASK);
	}
	else {
		/* Fifo config1 fifo disable set  */
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_FIFO_CONFIG_1_ADDR, 0x10);
		return ret;
	}

	wm = (ONE_ACC_FRAME_LEN * data->watermark_hwfifo) & WARTER_MARK_MAX_LEN;
	/* Fifo acc wm set  (unit of the fifo water mark is one byte, one frame is 7 byte)*/
	ret = regmap_write_csb(CSB1, data->regmap, BMI08_FIFO_WTM_0_ADDR, wm & 0xff);
	if (ret)
		return ret;
	regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_WTM_0_ADDR, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_FIFO_WTM_0_ADDR, val);

	ret = regmap_write_csb(CSB1, data->regmap, BMI08_FIFO_WTM_1_ADDR, wm >> 8);
	if (ret)
		return ret;
	regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_WTM_1_ADDR, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_FIFO_WTM_1_ADDR, val);

	/* Fifo config0 fifo mode set  */
	ret = regmap_write_csb(CSB1, data->regmap, BMI08_FIFO_CONFIG_0_ADDR, 0x2 | BMI08_ACC_FIFO_MODE);
	if (ret)
		return ret;
	ret = regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_CONFIG_0_ADDR, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_FIFO_CONFIG_0_ADDR, val);
	
	if (ret)
		return ret;
	ret = regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_CONFIG_1_ADDR, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_FIFO_CONFIG_1_ADDR, val);
	return ret;
}

static int bmi088_enable_gyro_fifo(struct bmi088_data *data)
{
	int ret, val;
	if (data->hwfifo_mode) {
		/* Fifo gyro wm enable */
		ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_WM_ENABLE, BMI08_GYRO_FIFO_WM_ENABLE_VAL);
	}
	else {
		/* Fifo gyro wm disable */
		ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_WM_ENABLE, BMI08_GYRO_FIFO_WM_DISABLE_VAL);
		return ret;
	}
	
	if (ret)
		return ret;
	regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_WM_ENABLE, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_GYRO_FIFO_WM_ENABLE, val);

	/* Fifo gyro config0 wm set (unit of the fifo water mark is one frame, one frame is 6 byte) */
	ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_CONFIG0, data->watermark_hwfifo);
	if (ret)
		return ret;
	ret = regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_CONFIG0, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_GYRO_FIFO_CONFIG0, val);

	/* Fifo gyro config1 fifo mode set  */
	ret = regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_CONFIG1, BMI08_GYRO_FIFO_MODE);
	if (ret)
		return ret;
	ret = regmap_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_CONFIG1, &val);
	//printk("test_ %s, reg:0x%x, val:0x%x\n", __FUNCTION__, BMI08_REG_GYRO_FIFO_CONFIG1, val);
	return ret;
	
}

#if 0
static int bmi088_enable_sync_irq(struct bmi088_data *data, bool enable)
{
	int ret;
	if (enable) {
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT2_MAP, BMI08_ACCEL_DATA_SYNC_INT_ENABLE);
	}
	else {
		ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT2_MAP, BMI08_ACCEL_DATA_SYNC_INT_DISABLE);
	}
	return ret;
}
#endif

static int bmi088_config_irq(struct bmi088_data *data, bool enable)
{
	int ret;
	if (enable) {
		// ret |= bmi088_enable_acc_interrupt(data, true, true);
		// ret |= bmi088_enable_sync_irq(data, true);
		ret = bmi088_set_int1_to_input_pin(data, true);
		ret = bmi088_enable_gyro_interrupt(data, true, true);
	}
	else {
		// ret |= bmi088_enable_sync_irq(data, false);
		// ret |= bmi088_enable_acc_interrupt(data, false, true);
		ret = bmi088_enable_gyro_interrupt(data, false, true);
	}
	
	return ret;
}

static int bmi088_clear_fifo(struct bmi088_data *data)
{
	int ret;
	/* clear fifo acc */
	ret = regmap_write_csb(CSB1, data->regmap, BMI08_REG_ACCEL_SOFTRESET, BMI08_ACCEL_CLEAR_FIFO);
	/* Fifo gyro config0 wm set, clear fifo gyro */
	ret |= regmap_write_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_CONFIG0, data->watermark_hwfifo);
	return ret;
}

/*!
 *  @brief This API writes the config stream data in memory using burst mode.
 */
static int8_t stream_transfer_write(struct bmi088_data *data, const uint8_t *stream_data, uint16_t index)
{
    int8_t rslt;
	int read_write_len = 32;
    uint8_t asic_msb = (uint8_t)((index / 2) >> 4);
    uint8_t asic_lsb = ((index / 2) & 0x0F);

    /* Write to feature config register */
    // rslt = bmi08a_set_regs(BMI08_REG_ACCEL_RESERVED_5B, &asic_lsb, 1, dev);
	rslt = regmap_write_csb(CSB1, data->regmap, 0x5b, asic_lsb);
	
    if (rslt == BMI08_OK)
    {
        /* Write to feature config register */
        // rslt = bmi08a_set_regs(BMI08_REG_ACCEL_RESERVED_5C, &asic_msb, 1, dev);
		rslt = regmap_write_csb(CSB1, data->regmap, 0x5c, asic_msb);

        if (rslt == BMI08_OK)
        {
            /* Write to feature config registers */
            // rslt = bmi08a_set_regs(BMI08_REG_ACCEL_FEATURE_CFG, (uint8_t *)stream_data, 1);
			// rslt = regmap_write_csb(CSB1, data->regmap, 0x5e, *stream_data);
			rslt = regmap_bulk_write_csb(CSB1, data->regmap, 0x5e, stream_data, read_write_len);
        }
    }

    return rslt;
}

/*!
 *  @brief This API uploads the bmi08 config file onto the device.
 */
int bmi08a_load_config_file(struct bmi088_data *data)
{
    int8_t rslt;

    /* Config loading disable */
    uint8_t config_load = BMI08_DISABLE;

    /* APS disable */
    uint8_t aps_disable = BMI08_DISABLE;

    uint16_t index = 0;
	int val;
	int read_write_len = 32;
	config_file_ptr = bmi08x_config_file;
        /* Check whether the read/write length is valid */
        if (read_write_len > 0)
        {
            /* Disable advanced power save mode */
			rslt = regmap_write_csb(CSB1, data->regmap, 0x7c, aps_disable);

            if (rslt == BMI08_OK)
            {
                /* Wait until APS disable is set. Refer the data-sheet for more information */
				usleep_range(450, 500);//need to delay 450us

                /* Disable config loading */
				rslt = regmap_write_csb(CSB1, data->regmap, 0x59, config_load);
            }

            if (rslt == BMI08_OK)
            {
                for (index = 0; index < BMI08_CONFIG_STREAM_SIZE;
                     index += read_write_len)
                {
                    /* Write the config stream */
                    rslt = stream_transfer_write(data, (config_file_ptr + index), index);
					// printk("test_ %s, index:%d val:0x%x\n", __FUNCTION__, index, *(config_file_ptr + index));
                }

                if (rslt == BMI08_OK)
                {
                    /* Enable config loading and FIFO mode */
                    config_load = BMI08_ENABLE;

					rslt = regmap_write_csb(CSB1, data->regmap, 0x59, config_load);

                    if (rslt == BMI08_OK)
                    {
                        /* Wait till ASIC is initialized. Refer the data-sheet for more information */
						usleep_range(150000, 200000);//need to delay 150ms

                        /* Check for config initialization status (1 = OK) */						
						regmap_read_csb(CSB1, data->regmap, 0x2a, &val);
						// printk("test_ %s, reg 0x2a val:0x%x\n", __FUNCTION__, val);
                    }
                }

                /* Check for initialization status */
                if (rslt == BMI08_OK && val != BMI08_INIT_OK)
                {
					printk("test_ %s, err reg 0x2a val:0x%x\n", __FUNCTION__, val);
                    rslt = BMI08_E_CONFIG_STREAM_ERROR;
                }
            }
        }
        else
        {
            rslt = BMI08_E_RD_WR_LENGTH_INVALID;
        }

    return rslt;
}

int bmi088_start(struct bmi088_data *data)
{
	int ret = 0;
	ret |= bmi088_accel_power_up(data->regmap);
	ret |= bmi088_gyro_power_up(data->regmap);
	ret |= bmi08xa_configure_data_synchronization(data);
	ret |= bmi088_config_irq(data, true);
	return ret;
}

int bmi088_stop(struct bmi088_data *data)
{
	int ret = 0;
	ret |= bmi088_config_irq(data, false);
	ret |= bmi088_accel_power_down(data->regmap);
	ret |= bmi088_gyro_power_down(data->regmap);
	return ret;
}

int bmi088_init(struct bmi088_data *data)
{
	int ret = 0;
	ret = bmi088_accel_chip_init(data, BOSCH_BMI088);
	ret |= bmi088_gyro_chip_init(data, BOSCH_BMI088);

	ret |= bmi088_accel_power_up(data->regmap);
	ret |= bmi088_gyro_power_up(data->regmap);

	ret |= bmi08a_load_config_file(data);
	ret |= bmi08xa_configure_data_synchronization(data);

	ret |= bmi088_config_irq(data, true);
	ret |= bmi088_stop(data);
	return ret;
}

static int bmi088_data_rdy_trigger_set_state(struct iio_trigger *trig,
					     bool enable)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret;

	if (enable) {
		ret = bmi088_start(data);
	}
	else {
		ret = bmi088_stop(data);
	}

	if (data->hwfifo_mode) {
		ret |= bmi088_clear_fifo(data);
	}
	vc_gpio_int_clear(AO_INT_IRQ_205);
	dev_info(&indio_dev->dev, "enable:%d, hwfifo_mode:%d\n", enable, data->hwfifo_mode);
	return ret;
}

static const struct iio_trigger_ops bmi088_trigger_ops = {
	.set_trigger_state = &bmi088_data_rdy_trigger_set_state,
};

int bmi088_probe_trigger(struct iio_dev *indio_dev, int irq, u32 irq_type)
{
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret;

	data->trig = devm_iio_trigger_alloc(&indio_dev->dev, "%s-dev%d",
					    indio_dev->name, indio_dev->id);

	if (data->trig == NULL) {
		//printk("test_ %s devm_iio_trigger_alloc error\n", __FUNCTION__);
		return -ENOMEM;
	}
	// printk("test_ %s irq:%d, indio_dev->id:%d\n", __FUNCTION__, irq, indio_dev->id);	
	ret = devm_request_irq(&indio_dev->dev, irq,
			       &iio_trigger_generic_data_rdy_poll,
			       irq_type, "bmi088", data->trig);
	if (ret) {
		printk("test_ %s devm_request_irq error\n", __FUNCTION__);
		return ret;
	}
		
	data->trig->dev.parent = regmap_get_device(data->regmap);
	data->trig->ops = &bmi088_trigger_ops;
	iio_trigger_set_drvdata(data->trig, indio_dev);

	ret = devm_iio_trigger_register(&indio_dev->dev, data->trig);
	if (ret) {
		//printk("test_ %s devm_request_irq error\n", __FUNCTION__);
		return ret;
	}

	indio_dev->trig = iio_trigger_get(data->trig);
	//printk("test_ %s success\n", __FUNCTION__);
	return 0;
}

static int bmi088_setup_irq(struct iio_dev *indio_dev, int irq)
{
	struct irq_data *desc;
	u32 irq_type;

	desc = irq_get_irq_data(irq);
	if (!desc) {
		dev_err(&indio_dev->dev, "Could not find IRQ %d\n", irq);
		//printk("test_ %s:Could not find IRQ\n", __FUNCTION__);
		return -EINVAL;
	}

	irq_type = irqd_get_trigger_type(desc);
			
	return bmi088_probe_trigger(indio_dev, irq, irq_type);
}

const struct regmap_config bmi088_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};
EXPORT_SYMBOL(bmi088_regmap_config);

#define FIFO_FRAME_CNT 147
/* FIFO Head definition */
#define FIFO_HEAD_NO_TAG_A        	0x84
#define FIFO_HEAD_INT1_TAG_A        0x85
#define FIFO_HEAD_SKIP_FRAME        0x40
#define ACC_DATA_BYTES_PER_FM      	6
#define ACC_FM_LEN			      	7

struct bmi088_accel_t bmi088_acc_frame_arr[FIFO_FRAME_CNT];
struct bmi088_gyro_t bmi088_gyro_frame_arr[FIFO_FRAME_CNT];

static int bmi088_get_data(struct iio_dev *indio_dev, struct bmi088_data *data)
{	
	int ret = 0;
	struct bmi088_gyro_t gyro;
	struct bmi088_accel_t acc;
	u16 scan_channel = 0;
	u64 ts_cur;
	static u64 ts_last;
	u8 data_len = 6;
	u8 tmp[8];
	int j = 0;

	ts_cur = vicore_tsp_proxy_read(data->imu_tsp_id);

	// printk("test_ %s: ts_last:%ld, ts_cur:%ld, delta:%d\n", __FUNCTION__,ts_last, ts_cur, ts_cur - ts_last);
	ts_last = ts_cur;
	memset(&acc, 0, sizeof(acc));
	memset(&gyro, 0, sizeof(gyro));
	regmap_bulk_read_csb(CSB1, data->regmap, BMI088_ACCEL_REG_XOUT_L, tmp, data_len);
	acc.x = tmp[1] << 8 |
			tmp[0];
	acc.y = tmp[3] << 8 |
			tmp[2];
	acc.z = tmp[5] << 8 |
			tmp[4];
	
	regmap_bulk_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_X_LSB, tmp, data_len);
	gyro.x = tmp[1] << 8 |
			tmp[0];
	gyro.y = tmp[3] << 8 |
			tmp[2];
	gyro.z = tmp[5] << 8 |
			tmp[4];

	
	for_each_set_bit(scan_channel, indio_dev->active_scan_mask, indio_dev->masklength) {
	switch(scan_channel) {
		case   BMI088_SCAN_GYRO_X:
			data->buf[j++] = gyro.x;
			// printk("test_ %s: bmi088_gyro_frame_arr[i].x:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_GYRO_Y:
			data->buf[j++] = gyro.y;
			// printk("test_ %s: bmi088_gyro_frame_arr[i].y:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_GYRO_Z:
			data->buf[j++] = gyro.z;
			// printk("test_ %s: bmi088_gyro_frame_arr[i].z:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_ACCEL_X:
			data->buf[j++] = acc.x;
			// printk("test_ %s: bmi088_acc_frame_arr[i].x:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_ACCEL_Y:
			data->buf[j++] = acc.y;
			// printk("test_ %s: bmi088_acc_frame_arr[i].y:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_ACCEL_Z:
			data->buf[j++] = acc.z;
			// printk("test_ %s: bmi088_acc_frame_arr[i].z:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		}
	}

	iio_push_to_buffers_with_timestamp(indio_dev, data->buf, ts_cur);
	return ret;
}

static int bmi088_get_sync_data(struct iio_dev *indio_dev, struct bmi088_data *data)
{	
	int ret;
	struct bmi088_gyro_t gyro;
	struct bmi088_accel_t acc;
	u16 scan_channel = 0;
	u8 tmp[6], num = 0;
	int j = 0, delat;

	memset(&acc, 0, sizeof(acc));
	memset(&gyro, 0, sizeof(gyro));
	
	regmap_bulk_read_acc_data(data->regmap, BMI08_REG_ACCEL_GP_0, tmp, 5);

	acc.x = tmp[2] << 8 |
			tmp[1];
	acc.y = tmp[4] << 8 |
			tmp[3];

	regmap_bulk_read_acc_data(data->regmap, BMI08_REG_ACCEL_GP_4, tmp, 3);

	acc.z = tmp[2] << 8 |
			tmp[1];
	
	regmap_bulk_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_X_LSB, tmp, 6);

	gyro.x = tmp[1] << 8 |
			tmp[0];
	gyro.y = tmp[3] << 8 |
			tmp[2];
	gyro.z = tmp[5] << 8 |
			tmp[4];

	for_each_set_bit(scan_channel, indio_dev->active_scan_mask, indio_dev->masklength) {
	switch(scan_channel) {
		case   BMI088_SCAN_GYRO_X:
			data->buf[j++] = gyro.x;
			// printk("test_ %s: bmi088_gyro_frame_arr[i].x:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_GYRO_Y:
			data->buf[j++] = gyro.y;
			// printk("test_ %s: bmi088_gyro_frame_arr[i].y:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_GYRO_Z:
			data->buf[j++] = gyro.z;
			// printk("test_ %s: bmi088_gyro_frame_arr[i].z:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_ACCEL_X:
			data->buf[j++] = acc.x;
			// printk("test_ %s: bmi088_acc_frame_arr[i].x:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_ACCEL_Y:
			data->buf[j++] = acc.y;
			// printk("test_ %s: bmi088_acc_frame_arr[i].y:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
		case   BMI088_SCAN_ACCEL_Z:
			data->buf[j++] = acc.z;
			// printk("test_ %s: bmi088_acc_frame_arr[i].z:0x%x\n", __FUNCTION__, data->buf[j-1]);
			break;
	}
	}

	delat = data->ts_cur - data->ts_last;
    if (delat > data->max_t) {
        dev_dbg(&indio_dev->dev, "error:%d, ts_cur:%lld, ts_last:%lld\n", delat, data->ts_cur, data->ts_last);
    }
    data->ts_last = data->ts_cur;

	ret = iio_push_to_buffers_with_timestamp(indio_dev, data->buf, data->ts_cur);
	while (ret && ++num < 3) {
		dev_dbg(&indio_dev->dev, "%s: push num:%d, ret:%d\n", __FUNCTION__, num, ret);
		usleep_range(60, 100);
		ret = iio_push_to_buffers_with_timestamp(indio_dev, data->buf, data->ts_cur);
	}
	if (ret) {
		dev_dbg(&indio_dev->dev, "%s: push error %d\n", __FUNCTION__, num);
	}
	return ret;
}
static int bmi088_get_fifo_data(struct bmi088_data *data)
{
	int ret, sta;
	u16 gyro_fifo_len;

	regmap_read_csb(CSB1, data->regmap, BMI08_REG_ACCEL_INT_STAT_1, &sta);
	//printk("test_ %s: reg:0x%x, sta:0x%x\n", __FUNCTION__, BMI08_REG_ACCEL_INT_STAT_1, sta);
	if (sta & BMI08_ACCEL_FIFO_WM_INT) {
		int fifo_len;
		memset(data->acc_fifo_data, 0, 1024);
		memset(data->gyro_fifo_data, 0, 1024);
		ret = regmap_read_2bytes_csb(CSB1, data->regmap, BMI08_FIFO_LENGTH_0_ADDR, &fifo_len);
		ret |= regmap_bulk_read_fifo_csb(CSB1, data->regmap, BMI08_FIFO_DATA_ADDR, data->acc_fifo_data , fifo_len);
		gyro_fifo_len = fifo_len * ACC_DATA_BYTES_PER_FM / ACC_FM_LEN;
		ret |= regmap_bulk_read_csb(CSB2, data->regmap, BMI08_REG_GYRO_FIFO_DATA, data->gyro_fifo_data , gyro_fifo_len);
		
		/* print fifo data test*/
		// int i, j;
		// printk("test_ %s, reg:0x%x, fifo_len:%d, gyro_fifo_len:%d\n\n", __FUNCTION__, BMI08_FIFO_LENGTH_0_ADDR, fifo_len, gyro_fifo_len);
		// for (i = 0, j = DUMMY_LEN; i < fifo_len; ++i, ++j) {
		// 	printk("test_ %s acc data[%d]:%x, gyro data[%d]:%x\n", __FUNCTION__, i, data->acc_fifo_data[j], i, data->gyro_fifo_data[i]);
		// }

		data->fifo_length = fifo_len + DUMMY_LEN;
	}
	return ret;
}

int bmi088_fifo_data_parse_handle(struct bmi088_data *data, struct iio_dev *indio_dev, int wq_flag)
{
	u8 frame_head = 0;
	u64 ts_gap;
	u64 ts_gap_real = 0;
	u64 ts_cur;
	u64 frm_odr;
	u16 fifo_index = 0;/* fifo data index*/
	u16 fm_index = 0 ;
	u16 j = 0;
	u16 i = 0;
	u16 frm_cnt = 0;
	u16 scan_channel = 0;
	s8 ret = 0;
	u16 fifo_length = data->fifo_length;
	struct bmi088_gyro_t gyro;
	struct bmi088_accel_t acc;
	u16 gyro_fm = 0;
	u16 good_cnt = 0;

	if (wq_flag)
		ts_cur = vicore_tsp_proxy_read(data->imu_tsp_id);
	else
		ts_cur = indio_dev->pollfunc->timestamp;
	// printk("test_ %s: ts_cur:%ld\n", __FUNCTION__, ts_cur);
	if (!data->ts_last) {
		dev_info(&indio_dev->dev, "skip first group data(%llu)\n", data->ts_last);
		data->ts_last = ts_cur;
		return ret;
	}
	memset(&acc, 0, sizeof(acc));
	memset(&gyro, 0, sizeof(gyro));
	for (i = 0; i < FIFO_FRAME_CNT; i++) {
		memset(&bmi088_acc_frame_arr[i], 0, sizeof(struct bmi088_accel_t));
		memset(&bmi088_gyro_frame_arr[i], 0, sizeof(struct bmi088_gyro_t));
	}

	dev_dbg(&indio_dev->dev, "frame head 0x%x, fifo_length: %d \n", data->acc_fifo_data[fifo_index + DUMMY_LEN], fifo_length);
	for (fifo_index = DUMMY_LEN; fifo_index < fifo_length;) {

		frame_head = data->acc_fifo_data[fifo_index];

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
			dev_info(&indio_dev->dev, "skip frame detected, fifo_index=%d\n", fifo_index);
		break;

		case FIFO_HEAD_NO_TAG_A   :
		case FIFO_HEAD_INT1_TAG_A :
		{	/*fifo data frame index + 1*/
			fifo_index = fifo_index + 1;
			if (fifo_index + ACC_DATA_BYTES_PER_FM > fifo_length) {
				ret = -1;
				dev_err(&indio_dev->dev, "acc fifo_index err, index = %d, fifo_length %d\n", fifo_index, fifo_length);
				break;
			}

			acc.x = data->acc_fifo_data[fifo_index + 1] << 8 |
					data->acc_fifo_data[fifo_index + 0];
			acc.y = data->acc_fifo_data[fifo_index + 3] << 8 |
					data->acc_fifo_data[fifo_index + 2];
			acc.z = data->acc_fifo_data[fifo_index + 5] << 8 |
					data->acc_fifo_data[fifo_index + 4];
			fifo_index = fifo_index + ACC_DATA_BYTES_PER_FM;
			dev_dbg(&indio_dev->dev, "accel frame detected, fifo_index=%d\n", fifo_index);
			break;
		}

		default:
			ret = -1;
			dev_info(&indio_dev->dev, "acc err data, fifo_index=%d\n", fifo_index);
		break;

		}
		if (ret) {
			break;
		}
		bmi088_acc_frame_arr[frm_cnt] = acc;
		frm_cnt++;
	}

	gyro_fm = fifo_length / ACC_FM_LEN;
	for (fm_index = 0; fm_index < gyro_fm;) {
		int fifo_index = fm_index * 6;
		gyro.x = data->gyro_fifo_data[fifo_index + 1] << 8 |
					data->gyro_fifo_data[fifo_index + 0];
		gyro.y = data->gyro_fifo_data[fifo_index + 3] << 8 |
				data->gyro_fifo_data[fifo_index + 2];
		gyro.z = data->gyro_fifo_data[fifo_index + 5] << 8 |
				data->gyro_fifo_data[fifo_index + 4];

		bmi088_gyro_frame_arr[fm_index++] = gyro;
	}

	//delete empty gyro frame
	for (i = frm_cnt - 1; i >= 0; i--) {
		static __le16 empty_gyro = 0x8000;
		if (bmi088_gyro_frame_arr[i].x == empty_gyro && 
			bmi088_gyro_frame_arr[i].y == empty_gyro && 
			bmi088_gyro_frame_arr[i].z == empty_gyro) {
			dev_info(&indio_dev->dev, "gyro not ready, frm_cnt:%d,good_cnt:%d\n", frm_cnt, i);
		}
		else {
			good_cnt = i + 1;
			break;
		}
	}

	frm_odr = max(data->odr_acc, data->odr_gyro);
	ts_gap = (u64)(1000000000) * 1000000 / frm_odr;
	ts_gap_real = ts_gap;
	if (frm_cnt && data->ts_last) {
		if (frm_cnt != gyro_fm) {
			bmi088_clear_fifo(data);
			ts_gap_real = (ts_cur - data->ts_last) / gyro_fm;
			dev_info(&indio_dev->dev, "acc loss data, frm_cnt:%d\n", frm_cnt);
		}
		else {
			ts_gap_real = (ts_cur - data->ts_last) / good_cnt;
		}
	}

	for (i = 0; i < good_cnt; i++) {
		j = 0;
		for_each_set_bit(scan_channel, indio_dev->active_scan_mask, indio_dev->masklength) {
			switch(scan_channel) {
				case   BMI088_SCAN_GYRO_X:
					data->buf[j++] = bmi088_gyro_frame_arr[i].x;
					// printk("test_ %s: bmi088_gyro_frame_arr[i].x:0x%x\n", __FUNCTION__, data->buf[j-1]);
					break;
				case   BMI088_SCAN_GYRO_Y:
					data->buf[j++] = bmi088_gyro_frame_arr[i].y;
					//printk("test_ %s: bmi088_gyro_frame_arr[i].y:0x%x\n", __FUNCTION__, data->buf[j-1]);
					break;
				case   BMI088_SCAN_GYRO_Z:
					data->buf[j++] = bmi088_gyro_frame_arr[i].z;
					//printk("test_ %s: bmi088_gyro_frame_arr[i].z:0x%x\n", __FUNCTION__, data->buf[j-1]);
					break;
				case   BMI088_SCAN_ACCEL_X:
					data->buf[j++] = bmi088_acc_frame_arr[i].x;
					//printk("test_ %s: bmi088_acc_frame_arr[i].x:0x%x\n", __FUNCTION__, data->buf[j-1]);
					break;
				case   BMI088_SCAN_ACCEL_Y:
					data->buf[j++] = bmi088_acc_frame_arr[i].y;
					//printk("test_ %s: bmi088_acc_frame_arr[i].y:0x%x\n", __FUNCTION__, data->buf[j-1]);
					break;
				case   BMI088_SCAN_ACCEL_Z:
					data->buf[j++] = bmi088_acc_frame_arr[i].z;
					//printk("test_ %s: bmi088_acc_frame_arr[i].z:0x%x\n", __FUNCTION__, data->buf[j-1]);
					break;
			}
		}
		// iio_push_to_buffers_with_timestamp(indio_dev, data->buf, ts_cur - (frm_cnt - 1 - i ) * ts_gap_real);
		iio_push_to_buffers_with_timestamp(indio_dev, data->buf, data->ts_last + (i + 1) * ts_gap_real);
	}
	// printk("test_ %s: ts_last:%ld, ts_cur:%ld, delta:%d, ts_gap_real:%d, frm_cnt:%d, fifo_length:%d\n", 
	// 	__FUNCTION__, data->ts_last, ts_cur, ts_cur - data->ts_last, ts_gap_real, frm_cnt, fifo_length);
	dev_dbg(&indio_dev->dev, "frm_cnt:%d, ts_last:%llu, ts_gap:%llu, ts_gap_real:%llu, odr:%lld, ts:%llu\n",
			frm_cnt, data->ts_last, ts_gap, ts_gap_real, frm_odr, ts_cur - (frm_cnt - 1 - i) * ts_gap_real);
	data->ts_last = ts_cur;
	dev_dbg(&indio_dev->dev, "ktime ts end:%llu\n", ktime_get_ns());

	return ret;
}

static irqreturn_t bmi088_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct bmi088_data *data = iio_priv(indio_dev);

	mutex_lock(&data->bmi_lock);
	vc_gpio_int_clear(AO_INT_IRQ_205);
	if (data->hwfifo_mode) {
		bmi088_get_fifo_data(data);
		bmi088_fifo_data_parse_handle(data, indio_dev, 1);
	}
	else {
		if (data->sync_mode) {
			data->ts_cur = indio_dev->pollfunc->timestamp;	

			// static u64 cnt = 0;
			// if (cnt++ % data->freq_div == 0) {
			// 	usleep_range(60, 100);//wait for acc sync ready
			// 	bmi088_get_sync_data(indio_dev, data);
			// }

			if (data->ts_cur - data->ts_last > data->threhold_t) {
				usleep_range(60, 100);//wait for acc sync ready
				bmi088_get_sync_data(indio_dev, data);
			}
		}
		else {
			bmi088_get_data(indio_dev, data);
			// printk("test_ %s sync:%d\n", __FUNCTION__, data->sync_mode);
		}
	}

	mutex_unlock(&data->bmi_lock);
	//printk("test_ %s 2:\n", __FUNCTION__);
	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

#if 0
static irqreturn_t bmi088_irq(int irq, void *map)
{
	return IRQ_HANDLED;
}
#endif

int bmi088_core_spi_csb_gpio_init(void) {
	int ret = gpio_request(CSB1, "bmi088 request csb1 gpio");
	if (ret == 0) {
		ret = gpio_direction_output(CSB1, 0);
		if (ret) {
			gpio_free(CSB1);
			return ret;
		}
	}
	gpio_set_value(CSB1, 1);

	ret = gpio_request(CSB2, "bmi088 request csb2 gpio");
	if (ret == 0) {
		ret = gpio_direction_output(CSB2, 0);
		if (ret) {
			gpio_free(CSB2);
			return ret;
		}
	}
	gpio_set_value(CSB2, 1);
	return ret;
}

ssize_t bmi088_get_hwfifo_mode(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);

	return sprintf(buf, "%d\n", data->hwfifo_mode);
}

ssize_t bmi088_set_hwfifo_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
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
	bmi088_enable_acc_fifo(data);
	bmi088_set_acc_int_mode(data);
	bmi088_enable_gyro_fifo(data);

out:
	mutex_unlock(&indio_dev->mlock);

	return err < 0 ? err : size;
}

ssize_t bmi088_get_high_freq_mode(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);

	return sprintf(buf, "%d\n", data->use_high_freq);
}

ssize_t bmi088_set_high_freq_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
	int err, val;

	mutex_lock(&indio_dev->mlock);
	if (iio_buffer_enabled(indio_dev)) {
		err = -EBUSY;
		goto out;
	}

	err = kstrtoint(buf, 10, &val);
	if (err < 0)
		goto out;

	data->use_high_freq = val;

out:
	mutex_unlock(&indio_dev->mlock);

	return err < 0 ? err : size;
}

ssize_t bmi088_get_hwfifo_watermark(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
	unsigned int val0, val1;
	int ret;

	ret = regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_WTM_0_ADDR, &val0);
	dev_info(dev, "reg:0x%x, val:0x%x\n", BMI08_FIFO_WTM_0_ADDR, val0);
	ret |= regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_WTM_1_ADDR, &val1);
	dev_info(dev, "reg:0x%x, val:0x%x\n", BMI08_FIFO_WTM_1_ADDR, val1);

	if (!ret)
		data->watermark_hwfifo = (val0 | (val1 << 8)) / ONE_ACC_FRAME_LEN;
	dev_info(dev, "watermark_hwfifo:%d\n", data->watermark_hwfifo);
	return sprintf(buf, "%d\n", data->watermark_hwfifo);
}

ssize_t bmi088_set_hwfifo_watermark(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret, val, wm;

	mutex_lock(&indio_dev->mlock);

	ret = kstrtoint(buf, 10, &wm);
	if (ret < 0)
		goto out;
	wm *= ONE_ACC_FRAME_LEN;
	if (wm < 1 || wm > WARTER_MARK_MAX_LEN) {
		dev_err(dev, "Failed to set fifo wm:%d", wm);
		ret = -EINVAL;
		goto out;
	}

	/* Fifo acc wm set  (unit of the fifo water mark is one byte, one frame is 7 byte)*/
	ret = regmap_write_csb(CSB1, data->regmap, BMI08_FIFO_WTM_0_ADDR, wm & 0xff);
	if (ret)
		return ret;
	ret |= regmap_write_csb(CSB1, data->regmap, BMI08_FIFO_WTM_1_ADDR, wm >> 8);
	if (ret)
		return ret;
	regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_WTM_0_ADDR, &val);
	dev_info(dev, "reg:0x%x, val:0x%x\n", BMI08_FIFO_WTM_0_ADDR, val);

	regmap_read_csb(CSB1, data->regmap, BMI08_FIFO_WTM_1_ADDR, &val);
	dev_info(dev, "reg:0x%x, val:0x%x\n", BMI08_FIFO_WTM_1_ADDR, val);

	if (!ret)
		data->watermark_hwfifo = wm / ONE_ACC_FRAME_LEN;
out:
	mutex_unlock(&indio_dev->mlock);
	dev_info(dev, "set fifo wm %d(%d)\n", wm, ret);
	return ret < 0 ? ret : size;
}

static
IIO_DEVICE_ATTR(hwfifo_mode, 0644, bmi088_get_hwfifo_mode,
	        bmi088_set_hwfifo_mode, 0);
static
IIO_DEVICE_ATTR(use_high_freq, 0644, bmi088_get_high_freq_mode,
	        bmi088_set_high_freq_mode, 0);
static
IIO_DEVICE_ATTR(watermark_hwfifo, 0644, bmi088_get_hwfifo_watermark,
	        bmi088_set_hwfifo_watermark, 0);

static struct attribute *bmi088_attrs[] = {
	&iio_dev_attr_hwfifo_mode.dev_attr.attr,
	&iio_dev_attr_use_high_freq.dev_attr.attr,
	&iio_dev_attr_watermark_hwfifo.dev_attr.attr,
	NULL,
};

static const struct attribute_group bmi088_attrs_group = {
	.attrs = bmi088_attrs,
};

static const struct iio_info bmi088_info = {
	.read_raw	= bmi088_read_raw,
	.write_raw	= bmi088_write_raw,
	.read_avail	= bmi088_read_avail,
	.attrs = &bmi088_attrs_group,
};

static const unsigned long bmi088_scan_masks[] = {
	BIT(BMI088_SCAN_ACCEL_X) | BIT(BMI088_SCAN_ACCEL_Y) | BIT(BMI088_SCAN_ACCEL_Z) | \
	BIT(BMI088_SCAN_GYRO_X) | BIT(BMI088_SCAN_GYRO_Y) | BIT(BMI088_SCAN_GYRO_Z),
	0
};

void bmi088_set_default_cfg(struct bmi088_data *data)
{
	data->hwfifo_mode = 0;
	data->use_high_freq = 0;
	data->watermark_hwfifo = WARTER_MARK;
	data->sync_mode = true;
	data->freq_div = 1;
	data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_1000HZ;
	data->acc_freq = BMI08_ACCEL_ODR_800_HZ;
	data->gyro_freq = BMI08_GYRO_BW_116_ODR_1000_HZ;
	data->odr_acc = 800 * 1000000;
	data->odr_gyro = 1000 * 1000000;
	data->max_t = 1000000000 / 1000 * 1.5;
	data->threhold_t = 1000000000 / 1000 * 0.9;
	return;
}

int bmi088_core_probe(struct device *dev, struct regmap *regmap,
	int irq, enum bmi_device_type type)
{
	struct bmi088_data *data;
	struct iio_dev *indio_dev;
	struct device_node *np = dev->of_node;
	int ret;

	ret = bmi088_core_spi_csb_gpio_init();
	if (ret) {
		dev_err(dev, "Failed to init csb gpio: %d\n", ret);
		return ret;
	}

	indio_dev = devm_iio_device_alloc(dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;
	data = iio_priv(indio_dev);
	dev_set_drvdata(dev, indio_dev);
	data->regmap = regmap;
	data->chip_info = &bmi088_chip_info_tbl;
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

	mutex_init(&data->bmi_lock);

	indio_dev->channels = bmi088_channels;
	indio_dev->num_channels = ARRAY_SIZE(bmi088_channels);
	indio_dev->name = data->chip_info->name;
	indio_dev->available_scan_masks = bmi088_scan_masks;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &bmi088_info;

	ret = devm_iio_triggered_buffer_setup(dev, indio_dev,
					      iio_pollfunc_store_time,
					      bmi088_trigger_handler, NULL);
	if (ret)
		return ret;

	if (of_property_read_u32(np, "imu_tsp_id", &data->imu_tsp_id)){
        dev_dbg(&indio_dev->dev, "No imu_tsp_id property found\n");
		data->imu_tsp_id = IMU1;
    }
	data->imu_tsp_id += TSP_IMU0;
	dev_dbg(dev, "data->imu_tsp_id:%d\n", data->imu_tsp_id);

	irq = bmi088_get_irq(dev->of_node);
	if (irq > 0) {
		ret = bmi088_setup_irq(indio_dev, irq);
		if (ret) {
			dev_err(&indio_dev->dev, "Failed to setup IRQ %d\n", irq);
		}		
		else {
			irq_set_affinity(irq, cpumask_of(3));
		}
	} 
	else {
		dev_err(&indio_dev->dev, "Not setting up IRQ trigger\n");
	}

	ret = devm_iio_device_register(dev, indio_dev);
	if (ret) {
		dev_err(dev, "Unable to register iio device\n");
		return ret;
	}

	if (iio_buffer_enabled(indio_dev)) {
		ret = -EBUSY;
		dev_err(dev, "iio_buffer_enabled err\n");
	}

	bmi088_set_default_cfg(data);
	ret = bmi088_init(data);
	ret |= devm_add_action_or_reset(dev, bmi088_chip_uninit, data);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(bmi088_core_probe, IIO_BMI088);

int bmi088_core_remove(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
	iio_device_unregister(indio_dev);
	bmi088_accel_power_down(data->regmap);
	bmi088_gyro_power_down(data->regmap);
	return 0;
}
EXPORT_SYMBOL_NS_GPL(bmi088_core_remove, IIO_BMI088);

MODULE_AUTHOR("Jason <jason.wang@vicoretek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BMI088 accelerometer and gyroscope driver (core)");
