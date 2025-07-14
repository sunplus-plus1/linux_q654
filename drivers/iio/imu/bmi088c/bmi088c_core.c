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
#include "bmi088c.h"
#include <linux/gpio.h>
#include <soc/vicore/timer.h>
#include "../../../media/platform/vicore/mipicsi/vc-cm4-rpmsg.h"
#include <linux/hwspinlock.h>

/* Timeout (ms) for the trylock of hardware spinlocks */
#define VC_ADC_HWLOCK_TIMEOUT	10

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
#define WARTER_MARK			10
#define WARTER_MARK_MAX_LEN 100

/* MAILBOX2 CPU2(CM4) to CPU0(CA55)*/
#define DIRECT_CPU2_TO_CPU0_INT_TRIGGER 	0
#define DIRECT_CPU2_TO_CPU0_WRITE_LOCK	 	1
#define DIRECT_CPU2_TO_CPU0_OVERWRITE_LOCK 	2
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS00 	4
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS01 	5
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS02 	6
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS03 	7
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS04 	8
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS05 	9
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS06 	10
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS07 	11
#define DIRECT_CPU2_TO_CPU0_NORMAL_TRANS19 	23
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS0   24
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS1   25
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS2   26
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS3   27
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS4   28
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS5   29
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS6   30
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS7   31
#define DIRECT_CPU2_TO_CPU0_DIRECT_TRANS4_STA_SHIFT	(1 << 28)

#define IMU_PACKAGE_NUM_OFFSET      0
#define IMU_PACKAGE_OFFSET          4

#define	IMU_PACKAGE_LEN				7
#define IMU_PACKAGE_BYTES           28

#define BIM088_SAMPLING_FREQ_100	100
#define BIM088_SAMPLING_FREQ_200	200
#define BIM088_SAMPLING_FREQ_400	400
#define BIM088_SAMPLING_FREQ_1000	1000
#define BIM088_SAMPLING_FREQ_2000	2000

extern u64 ca55_tsp_read(int id, u64 cycle);

/*! @brief structure definition for the quaternion output data from the module */
typedef struct
{
    int x;
    int y;
    int z;
    int w;
}quaternion_t;

/*! @brief structure definition for the orientation output data from the module */
typedef struct
{
    int heading;
    int pitch;
    int roll;
    int yaw;
}euler_angles_t;

enum TRANS_MODE {
	I2C_INTERFACE = 1,
	SPI_INTERFACE = 2,
};

enum DATA_MODE {
	RAW_DATA = 1,
	QUATERNION_DATA = 2,
	EULER_DATA = 3,
};
enum IMU_ID {
    IMU0 = 0,
    IMU1 = 1,
    IMU2 = 2,
    IMU3 = 3,
};
typedef enum
{
    START_BMI088 = 1,
	STOP_BMI088 = 2,
	SET_WARTERMARKER_LEN = 3,
	SET_SYNC_FREQ = 4,
	SET_ACC_RANGE = 5,
	SET_GYRO_RANGE = 6,
	SET_SPI_CS_PIN = 7,
	SET_BMI088_INT_PIN = 8,
    SET_BMI088_TRANS_MODE = 9,
    SET_BMI088_DATA_MODE = 10,
	SET_FREQ_DIV = 11,
	SET_IMU_TSP_ID = 12,
} config_flag; 

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
	.name = "bmi088c",
	.chip_id = {0x1e, 0x0f},
	.channels = bmi088_channels,
	.num_channels = ARRAY_SIZE(bmi088_channels),
};

#define MAX_IMU_DATA_LEN             1000
struct bmi088_data {
	struct platform_device *pdev;
	const struct bmi088_chip_info *chip_info;
	__le16 buf[12] __aligned(8);
	u8 buffer[4]__aligned(4); /* shared DMA safe buffer *///todo
	struct iio_trigger *trig;
	u8 watermark_hwfifo;
	u64 cnt;
	u8 freq_div;
	u8 sync_freq;
	u8 acc_freq;
	u8 gyro_freq;
	u8 acc_scale_reg;
	u8 gyro_scale_reg;
	u64 max_t;
	u64 threhold_t;
	u64 ts_cur;
	u64 ts_last;
	u32 ts_step;
	struct mutex bmi_lock;
	u32 *mailbox2_cpu2_to_cpu0;
	u32* imu_data_addr;
	u32 imu_tsp_id;
	u32 acc_cs_pin;
	u32 gyro_cs_pin;
	u32 bmi088_int_pin;
	u32 trans_mode;
	u32 data_mode;
	struct hwspinlock *hwlock;
	u32 imu_package[MAX_IMU_DATA_LEN] __aligned(4);
};

static int bmi088_accel_get_sample_freq(struct bmi088_data *data,
					int *val, int *val2)
{
	switch (data->acc_freq)
	{
	case BMI08_ACCEL_ODR_400_HZ:
		*val = BIM088_SAMPLING_FREQ_400;
		break;
	case BMI08_ACCEL_ODR_800_HZ:
		*val = BIM088_SAMPLING_FREQ_1000;
		break;
	case BMI08_ACCEL_ODR_1600_HZ:
		*val = BIM088_SAMPLING_FREQ_2000;
		break;
	default:
		break;
	}
	*val2 = 0;
	
	return IIO_VAL_INT_PLUS_MICRO;
}

static int bmi088_accel_set_sample_freq(struct bmi088_data *data, int val)
{
	unsigned int regval;
	int index = 0;
	int cfg = 0;

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
			data->ts_step = 1000000000 / 400;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
			data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
			data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
			break;
		case 800:
			data->ts_step = 1000000000 / 1000;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_1000HZ;
			data->acc_freq = BMI08_ACCEL_ODR_800_HZ;
			data->gyro_freq = BMI08_GYRO_BW_116_ODR_1000_HZ;
			break;
		case 1600:
			data->ts_step = 1000000000 / 2000;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = 0;
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_2000HZ;
			data->acc_freq = BMI08_ACCEL_ODR_1600_HZ;
			data->gyro_freq = BMI08_GYRO_BW_532_ODR_2000_HZ;
			break;
		default:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
			data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
			data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
			data->ts_step = 1000000000 / 400;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		}
		data->freq_div = 1;
	}
	else {
		switch (bmi088_gyro_sample_freqs[index])
		{
		case 200:
			data->ts_step = 1000000000 / 200;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		default:
			data->ts_step = 1000000000 / 100;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		}
		data->ts_step = 1000000000 / 400;
		data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
		data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
		data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
		data->freq_div = 400 / bmi088_sample_freqs[index];
	}
	data->ts_step *= data->freq_div;
	cfg = SET_CONFIG_FLAG(SET_SYNC_FREQ);
	cfg |= data->sync_freq;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_FREQ_DIV);
	cfg |= data->freq_div;
	send_bmi088_control(cfg);
	return 0;

}

static int bmi088_gyro_get_sample_freq(struct bmi088_data *data,
					int *val, int *val2)
{
	switch (data->gyro_freq)
	{
	case BMI08_GYRO_BW_47_ODR_400_HZ:
		*val = BIM088_SAMPLING_FREQ_400;
		break;
	case BMI08_GYRO_BW_116_ODR_1000_HZ:
		*val = BIM088_SAMPLING_FREQ_1000;
		break;
	case BMI08_GYRO_BW_532_ODR_2000_HZ:
		*val = BIM088_SAMPLING_FREQ_2000;
		break;
	default:
		break;
	}
	*val2 = 0;
	return IIO_VAL_INT_PLUS_MICRO;
}

static int bmi088_gyro_set_sample_freq(struct bmi088_data *data, int val)
{
	unsigned int regval;
	int index = 0;
	int cfg = 0;

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
			data->ts_step = 1000000000 / 400;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		case 1000:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_1000HZ;
			data->gyro_freq = BMI08_GYRO_BW_116_ODR_1000_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_800_HZ;
			data->ts_step = 1000000000 / 1000;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		case 2000:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_2000HZ;
			data->gyro_freq = BMI08_GYRO_BW_532_ODR_2000_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_1600_HZ;
			data->ts_step = 1000000000 / 2000;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = 0;
			break;
		default:
			data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
			data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
			data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
			data->ts_step = 1000000000 / 400;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		}
		data->freq_div = 1;
	}
	else {
		switch (bmi088_gyro_sample_freqs[index])
		{
		case 200:
			data->ts_step = 1000000000 / 200;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		default:
			data->ts_step = 1000000000 / 100;
			data->max_t = data->ts_step * 1.5;
			data->threhold_t = data->ts_step * 0.9;
			break;
		}
		data->ts_step = 1000000000 / 400;
		data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
		data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
		data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
		data->freq_div = 400 / bmi088_gyro_sample_freqs[index];
	}
	data->ts_step *= data->freq_div;
	cfg = SET_CONFIG_FLAG(SET_SYNC_FREQ);
	cfg |= data->sync_freq;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_FREQ_DIV);
	cfg |= data->freq_div;
	send_bmi088_control(cfg);
	return 0;
}

static int bmi088_read_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int *val, int *val2, long mask)
{
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret, i;
	int reg;
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->type) {
		case IIO_TEMP:
			goto out_read_raw_pm_put;
		case IIO_ACCEL:
			goto out_read_raw_pm_put;
		case IIO_ANGL_VEL:
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
			reg = data->acc_scale_reg;
			if (ret)
				goto out_read_raw_pm_put;
			reg = FIELD_GET(BMIO088_ACCEL_ACC_RANGE_MSK, reg);
			for (i = 0; i < bmi088_scale_table[BMI088_ACCEL].num; i++)
				if (bmi088_scale_table[BMI088_ACCEL].tbl[i].bits == reg) {
					*val  = 0;
					*val2 = bmi088_scale_table[BMI088_ACCEL].tbl[i].uscale;
					ret = IIO_VAL_INT_PLUS_MICRO;
					goto out_read_raw_pm_put;
				}
			return -EINVAL;
			break;
		case IIO_ANGL_VEL:
			reg = data->gyro_scale_reg;
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
			goto out_read_raw_pm_put;
		case IIO_ANGL_VEL:
			ret = bmi088_gyro_get_sample_freq(data, val, val2);
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
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ACCEL:
			*vals = (const int *)acc_scale_table;
			*length = ARRAY_SIZE(acc_scale_table) * 2;
			break;
		case IIO_ANGL_VEL:
			*vals = (const int *)gyro_scale_table;
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
static int bmi088_accel_set_scale(struct bmi088_data *data, int val, int val2)
{
	unsigned int i;
	int cfg = 0;

	for (i = 0; i < bmi088_scale_table[BMI088_ACCEL].num; i++)
		if (bmi088_scale_table[BMI088_ACCEL].tbl[i].uscale == val2)
			break;

	if (i == bmi088_scale_table[BMI088_ACCEL].num)
		return -EINVAL;
	data->acc_scale_reg = bmi088_scale_table[BMI088_ACCEL].tbl[i].bits;
	cfg = SET_CONFIG_FLAG(SET_ACC_RANGE);
	cfg |= data->acc_scale_reg;
	send_bmi088_control(cfg);
	return 0;
}
static int bmi088_gyro_set_scale(struct bmi088_data *data, int val, int val2)
{
	unsigned int i;
	int cfg = 0;

	for (i = 0; i < bmi088_scale_table[BMI088_GYRO].num; i++)
		if (bmi088_scale_table[BMI088_GYRO].tbl[i].uscale == val2)
			break;

	if (i == bmi088_scale_table[BMI088_GYRO].num)
		return -EINVAL;

	data->gyro_scale_reg = bmi088_scale_table[BMI088_GYRO].tbl[i].bits;
	cfg = SET_CONFIG_FLAG(SET_GYRO_RANGE);
	cfg |= data->gyro_scale_reg;
	send_bmi088_control(cfg);
	return 0;
}

static int bmi088_write_raw(struct iio_dev *indio_dev,
				  struct iio_chan_spec const *chan,
				  int val, int val2, long mask)
{
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

static void bmi088_send_config_info(struct bmi088_data *data)
{
	int cfg;
	cfg = SET_CONFIG_FLAG(SET_SPI_CS_PIN);
	cfg |= (data->acc_cs_pin | (data->gyro_cs_pin << 8));
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_BMI088_INT_PIN);
	cfg |= data->bmi088_int_pin;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_SYNC_FREQ);
	cfg |= data->sync_freq;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_WARTERMARKER_LEN);
	cfg |= data->watermark_hwfifo;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_GYRO_RANGE);
	cfg |= data->gyro_scale_reg;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_ACC_RANGE);
	cfg |= data->acc_scale_reg;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_BMI088_TRANS_MODE);
	cfg |= data->trans_mode;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_BMI088_DATA_MODE);
	cfg |= data->data_mode;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_FREQ_DIV);
	cfg |= data->freq_div;
	send_bmi088_control(cfg);
	cfg = SET_CONFIG_FLAG(SET_IMU_TSP_ID);
	cfg |= data->imu_tsp_id;
	send_bmi088_control(cfg);
}

static int bmi088_data_rdy_trigger_set_state(struct iio_trigger *trig,
					     bool enable)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret = 0;
	int cfg;
	data->cnt = 0;

	if (enable) {
		static bool set_pin = false;
		if (!set_pin) {
			bmi088_send_config_info(data);
			set_pin = true;
		}
		cfg = SET_CONFIG_FLAG(START_BMI088);
		send_bmi088_control(cfg);
	}
	else {
		
		cfg = SET_CONFIG_FLAG(STOP_BMI088);
		send_bmi088_control(cfg);
	}

	dev_info(&indio_dev->dev, "enable:%d, cfg: 0x%x\n",  enable, cfg);
	return ret;
}

static const struct iio_trigger_ops bmi088_trigger_ops = {
	.set_trigger_state = &bmi088_data_rdy_trigger_set_state,
};

int bmi088c_probe_trigger(struct iio_dev *indio_dev, int irq, u32 irq_type)
{
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret;

	data->trig = devm_iio_trigger_alloc(&indio_dev->dev, "%s-dev%d",
					    indio_dev->name, indio_dev->id);

	if (data->trig == NULL) {
		dev_err(&indio_dev->dev, "devm_iio_trigger_alloc error\n");
		return -ENOMEM;
	}
	ret = devm_request_irq(&indio_dev->dev, irq,
			       &iio_trigger_generic_data_rdy_poll,
			       irq_type, "bmi088c", data->trig);
	if (ret) {
		dev_err(&indio_dev->dev, "devm_request_irq error\n");
		return ret;
	}
		
	data->trig->dev.parent = &(data->pdev->dev); 
	data->trig->ops = &bmi088_trigger_ops;
	iio_trigger_set_drvdata(data->trig, indio_dev);

	ret = devm_iio_trigger_register(&indio_dev->dev, data->trig);
	if (ret) {
		dev_err(&indio_dev->dev, "devm_iio_trigger_register error\n");
		return ret;
	}

	indio_dev->trig = iio_trigger_get(data->trig);
	return 0;
}

static int bmi088_setup_irq(struct iio_dev *indio_dev, int irq)
{
	struct irq_data *desc;
	u32 irq_type;

	desc = irq_get_irq_data(irq);
	if (!desc) {
		dev_err(&indio_dev->dev, "Could not find IRQ %d\n", irq);
		return -EINVAL;
	}

	irq_type = irqd_get_trigger_type(desc);
			
	return bmi088c_probe_trigger(indio_dev, irq, irq_type);
}

static int bmi088_get_cm4_data(struct iio_dev *indio_dev, struct bmi088_data *data)
{	
	int ret, i, j = 0;
	u32 tsp_lo;
	u16 scan_channel = 0;
	u32 lock = 0;
	int package_info, data_start, package_num;
	u32 *mem_data;
	u32 data_len;
	u64 firt_tsp;
	s64 delat;
	u8 num = 0;
	struct bmi088_gyro_t gyro;
	struct bmi088_accel_t acc;
	
	memset(&acc, 0, sizeof(acc));
	memset(&gyro, 0, sizeof(gyro));
	
	lock = readl(data->mailbox2_cpu2_to_cpu0 + DIRECT_CPU2_TO_CPU0_WRITE_LOCK);
	if (!(lock & DIRECT_CPU2_TO_CPU0_DIRECT_TRANS4_STA_SHIFT)) {
		dev_warn(&indio_dev->dev, "error lock:%x\n", lock);
	}

	package_info = readl(data->mailbox2_cpu2_to_cpu0 + DIRECT_CPU2_TO_CPU0_DIRECT_TRANS4);	
	data_start = (package_info >> 8) & 0xff;
	package_num = package_info & 0xff;
	
	mem_data = &(data->imu_package[0]);
	data_len = (package_num + data_start) * IMU_PACKAGE_BYTES + IMU_PACKAGE_OFFSET;
	if (data_len > (MAX_IMU_DATA_LEN << 2)) {
		package_num = MAX_IMU_DATA_LEN / IMU_PACKAGE_LEN - data_start;
		dev_warn(&indio_dev->dev, "too long data_len:%d, package_info:0x%x, package_num:%d\n", data_len, package_info, package_num);
		data_len = (MAX_IMU_DATA_LEN << 2);
	}
	dev_info(&indio_dev->dev, "data_len:%d, package_info:0x%x, package_num:%d\n", data_len, package_info, package_num);
	if (data->hwlock) {
		ret = hwspin_lock_timeout_raw(data->hwlock, VC_ADC_HWLOCK_TIMEOUT);
		if (ret) {
			dev_err(&indio_dev->dev, "timeout to get the hwspinlock\n");
			return ret;
		}
	}
	writel(0, data->imu_data_addr); //clear package info
	memcpy(mem_data++, data->imu_data_addr, data_len);//++ skip package info
	if(data->hwlock) {
		hwspin_unlock_raw(data->hwlock);
	}

	if (data_start) {
		mem_data += data_start * IMU_PACKAGE_LEN;
		dev_dbg(&indio_dev->dev, "data_start:%d\n", data_start);
	}
	if (package_num > data->watermark_hwfifo || package_num < 1) {
		dev_warn(&indio_dev->dev, "package_num:%d\n", package_num);
	}

	for (i = 1; i <= package_num; i++) {
		tsp_lo = ioread32(mem_data + i * IMU_PACKAGE_LEN - 1);
		if (tsp_lo) {
			break;
		}
		else {
			dev_err(&indio_dev->dev, "error tsp_lo[%d]:0x%x\n", i, tsp_lo);
		}
	}
	
	if (!tsp_lo) {
		dev_err(&indio_dev->dev, "tsp_lo:%d\n", tsp_lo);
		return 0;
	}
	firt_tsp = tsp_proxy_to_realtime(data->imu_tsp_id, tsp_lo);
	for (i = 0; i < package_num; ++i) {
		data->ts_cur = firt_tsp + data->ts_step * i;
		dev_dbg(&indio_dev->dev, "tsp_lo:%x, ts_cur:%lld\n", tsp_lo, data->ts_cur);
		delat = data->ts_cur - data->ts_last;
		if (delat > data->max_t) {
			dev_warn(&indio_dev->dev, "delat:%lld, ts_cur:%lld, ts_last:%lld\n", delat, data->ts_cur, data->ts_last);
		}
		if (delat <= 0) {
			dev_err(&indio_dev->dev, "delat:%lld, ts_cur:%lld, ts_last:%lld\n", delat, data->ts_cur, data->ts_last);
			continue;
		}
		data->ts_last = data->ts_cur;
		if (data->data_mode == RAW_DATA) {
			acc.x =ioread32(mem_data++);
			acc.y =ioread32(mem_data++);
			acc.z =ioread32(mem_data++);
			gyro.x =ioread32(mem_data++);
			gyro.y =ioread32(mem_data++);
			gyro.z =ioread32(mem_data++);
			tsp_lo =ioread32(mem_data++);
			dev_dbg(&indio_dev->dev, "mem acc.x:%x, acc.y:%x, acc.z:%x gyro.x:%x, gyro.y:%x, gyro.z:%x, tsp_lo:%x\n", 
				acc.x, acc.y, acc.z, gyro.x, gyro.y, gyro.z, tsp_lo);
			
			for_each_set_bit(scan_channel, indio_dev->active_scan_mask, indio_dev->masklength) {
				switch(scan_channel) {
					case   BMI088_SCAN_GYRO_X:
						data->buf[j++] = gyro.x;
						break;
					case   BMI088_SCAN_GYRO_Y:
						data->buf[j++] = gyro.y;
						break;
					case   BMI088_SCAN_GYRO_Z:
						data->buf[j++] = gyro.z;
						break;
					case   BMI088_SCAN_ACCEL_X:
						data->buf[j++] = acc.x;
						break;
					case   BMI088_SCAN_ACCEL_Y:
						data->buf[j++] = acc.y;
						break;
					case   BMI088_SCAN_ACCEL_Z:
						data->buf[j++] = acc.z;
						break;
				}
			}
		}
		else if (data->data_mode == QUATERNION_DATA) {
			quaternion_t qua;
			qua.x = ioread32(mem_data++);
			qua.y = ioread32(mem_data++);
			qua.z = ioread32(mem_data++);
			qua.w = ioread32(mem_data++);
			mem_data++;
			mem_data++;
			tsp_lo =ioread32(mem_data++);
			dev_dbg(&indio_dev->dev, "mem qua.x:%d, qua.y:%d, qua.z:%d, qua.w:%d, tsp_lo:%x\n", 
				qua.x, qua.y, qua.z, qua.w, tsp_lo);

			data->buf[j++] = qua.x & 0xffff;
			data->buf[j++] = (qua.x >> 16) & 0xffff;
			data->buf[j++] = qua.y & 0xffff;
			data->buf[j++] = (qua.y >> 16) & 0xffff;
			data->buf[j++] = qua.z & 0xffff;
			data->buf[j++] = (qua.z >> 16) & 0xffff;
			data->buf[j++] = qua.w & 0xffff;
			data->buf[j++] = (qua.w >> 16) & 0xffff;
		}

		ret = iio_push_to_buffers_with_timestamp(indio_dev, data->buf, data->ts_cur);
		while (ret && ++num < 3) {
			dev_dbg(&indio_dev->dev, "push num:%d, ret:%d\n", num, ret);
			usleep_range(60, 100);
			ret = iio_push_to_buffers_with_timestamp(indio_dev, data->buf, data->ts_cur);
		}
		if (ret) {
			dev_dbg(&indio_dev->dev, "push error %d\n", num);
		}
	}
	return ret;
}


static irqreturn_t bmi088_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct bmi088_data *data = iio_priv(indio_dev);

	mutex_lock(&data->bmi_lock);

	bmi088_get_cm4_data(indio_dev, data);
	
	mutex_unlock(&data->bmi_lock);
	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}


ssize_t bmi088c_get_hwfifo_watermark(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);

	dev_info(dev, "watermark_hwfifo:%d\n", data->watermark_hwfifo);
	return sprintf(buf, "%d\n", data->watermark_hwfifo);
}

ssize_t bmi088c_set_hwfifo_watermark(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret, wm, cfg;

	mutex_lock(&indio_dev->mlock);

	ret = kstrtoint(buf, 10, &wm);
	if (ret < 0)
		goto out;
	if (wm < 1 || wm > WARTER_MARK_MAX_LEN) {
		dev_err(dev, "Failed to set fifo wm:%d", wm);
		ret = -EINVAL;
		goto out;
	}

	if (!ret)
		data->watermark_hwfifo = wm;
out:
	mutex_unlock(&indio_dev->mlock);
	dev_info(dev, "set fifo wm %d(%d)\n", wm, ret);
	cfg = SET_CONFIG_FLAG(SET_WARTERMARKER_LEN);
	cfg |= data->watermark_hwfifo;
	send_bmi088_control(cfg);
	return ret < 0 ? ret : size;
}

static
IIO_DEVICE_ATTR(watermark_hwfifo, 0644, bmi088c_get_hwfifo_watermark,
	        bmi088c_set_hwfifo_watermark, 0);

ssize_t bmi088_get_trans_mode(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);

	dev_info(dev, "trans_mode:%d\n", data->trans_mode);
	return sprintf(buf, "%d\n", data->trans_mode);
}

ssize_t bmi088_set_trans_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret, tm, cfg;

	mutex_lock(&indio_dev->mlock);

	ret = kstrtoint(buf, 10, &tm);
	if (ret < 0)
		goto out;
	if (tm < I2C_INTERFACE || tm > SPI_INTERFACE) {
		dev_err(dev, "Failed to set tm:%d", tm);
		ret = -EINVAL;
		goto out;
	}

	if (!ret)
		data->trans_mode = tm;
out:
	mutex_unlock(&indio_dev->mlock);
	dev_info(dev, "set tm %d(%d)\n", tm, ret);
	cfg = SET_CONFIG_FLAG(SET_BMI088_TRANS_MODE);
	cfg |= data->trans_mode;
	send_bmi088_control(cfg);
	return ret < 0 ? ret : size;
}

static
IIO_DEVICE_ATTR(trans_mode, 0644, bmi088_get_trans_mode,
	        bmi088_set_trans_mode, 0);

ssize_t bmi088_get_data_mode(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);

	dev_info(dev, "data_mode:%d\n", data->data_mode);
	return sprintf(buf, "%d\n", data->data_mode);
}

ssize_t bmi088_set_data_mode(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct bmi088_data *data = iio_priv(indio_dev);
	int ret, dm, cfg;

	mutex_lock(&indio_dev->mlock);

	ret = kstrtoint(buf, 10, &dm);
	if (ret < 0)
		goto out;
	if (dm < RAW_DATA || dm > EULER_DATA) {
		dev_err(dev, "Failed to set dm:%d", dm);
		ret = -EINVAL;
		goto out;
	}

	if (!ret)
		data->data_mode = dm;
out:
	mutex_unlock(&indio_dev->mlock);
	dev_info(dev, "set dm %d(%d)\n", dm, ret);
	cfg = SET_CONFIG_FLAG(SET_BMI088_DATA_MODE);
	cfg |= data->data_mode;
	send_bmi088_control(cfg);
	return ret < 0 ? ret : size;
}

static
IIO_DEVICE_ATTR(data_mode, 0644, bmi088_get_data_mode,
	        bmi088_set_data_mode, 0);

static struct attribute *bmi088_attrs[] = {
	&iio_dev_attr_watermark_hwfifo.dev_attr.attr,
	&iio_dev_attr_trans_mode.dev_attr.attr,
	&iio_dev_attr_data_mode.dev_attr.attr,
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

void bmi088c_set_default_cfg(struct bmi088_data *data)
{
	data->trans_mode = SPI_INTERFACE;
	data->data_mode = RAW_DATA;
	data->cnt = 0;
	data->watermark_hwfifo = WARTER_MARK;
	data->freq_div = 1;
	data->sync_freq = BMI08_ACCEL_DATA_SYNC_MODE_400HZ;
	data->acc_freq = BMI08_ACCEL_ODR_400_HZ;
	data->gyro_freq = BMI08_GYRO_BW_47_ODR_400_HZ;
	data->ts_step = 1000000000 / 400;
	data->max_t = data->ts_step * 1.5;
	data->threhold_t = data->ts_step * 0.9;
	return;
}


int bmi088c_core_probe(struct platform_device *pdev, enum bmi_device_type type)
{
	struct resource *res_mem;
	struct bmi088_data *data;
	struct iio_dev *indio_dev;
	int ret, irq;
	u32 reg_base, reg_len;
	struct device_node *np = pdev->dev.of_node;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;
	data = iio_priv(indio_dev);
	dev_set_drvdata(&pdev->dev, indio_dev);
	data->pdev = pdev;
	data->chip_info = &bmi088_chip_info_tbl;

	mutex_init(&data->bmi_lock);

	indio_dev->channels = bmi088_channels;
	indio_dev->num_channels = ARRAY_SIZE(bmi088_channels);
	indio_dev->name = data->chip_info->name;
	indio_dev->available_scan_masks = bmi088_scan_masks;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &bmi088_info;

	ret = devm_iio_triggered_buffer_setup(&pdev->dev, indio_dev,
					      iio_pollfunc_store_time,
					      bmi088_trigger_handler, NULL);
	if (ret)
		return ret;

	irq = bmi088_get_irq(pdev->dev.of_node);
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

	ret = devm_iio_device_register(&pdev->dev, indio_dev);
	if (ret) {
		dev_err(&pdev->dev, "Unable to register iio device\n");
		return ret;
	}

	if (iio_buffer_enabled(indio_dev)) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "iio_buffer_enabled err\n");
	}

	bmi088c_set_default_cfg(data);
	if (of_property_read_u32(np, "gpio_acc_cs_pin", &data->acc_cs_pin)){
        dev_dbg(&pdev->dev, "No gpio_acc_cs_pin property found \n");
		data->acc_cs_pin = 53;
    }
	if (of_property_read_u32(np, "gpio_gyro_cs_pin", &data->gyro_cs_pin)){
        dev_dbg(&pdev->dev, "No gpio_gyro_cs_pin property found \n");
		data->gyro_cs_pin = 56;
    }
	if (of_property_read_u32(np, "gpio_bmi088_int_pin", &data->bmi088_int_pin)){
        dev_dbg(&pdev->dev, "No gpio_bmi088_int_pin property found \n");
		data->bmi088_int_pin = 77;
    }
    if (of_property_read_u32(np, "cpu2_to_cpu0_base", &reg_base)) {
        dev_dbg(&pdev->dev, "Failed to read my_property from device tree\n");
        return -EINVAL;
    }
	if (of_property_read_u32(np, "cpu2_to_cpu0_len", &reg_len)) {
        dev_dbg(&pdev->dev, "Failed to read my_property from device tree\n");
        return -EINVAL;
    }
	if (of_property_read_u32(np, "bmi088_trans_mode", &data->trans_mode)){
        dev_dbg(&pdev->dev, "No bmi088_trans_mode property found \n");
		data->trans_mode = SPI_INTERFACE;
    }
	if (of_property_read_u32(np, "bmi088_data_mode", &data->data_mode)){
        dev_dbg(&pdev->dev, "No bmi088_data_mode property found \n");
		data->data_mode = RAW_DATA;
    }
	if (of_property_read_u32(np, "imu_tsp_id", &data->imu_tsp_id)){
        dev_dbg(&pdev->dev, "No imu_tsp_id property found \n");
		data->imu_tsp_id = IMU1;
    }
	data->imu_tsp_id += TSP_IMU0;
	dev_dbg(&pdev->dev, "data->acc_cs_pin:%d, data->gyro_cs_pin:%d, data->bmi088_int_pin:%d, reg_base:%x, reg_len:%x, trans_mode:%d, data_mode:%d, imu_tsp_id:%d\n", 
		data->acc_cs_pin, data->gyro_cs_pin, data->bmi088_int_pin, reg_base, reg_len, data->trans_mode, data->data_mode, data->imu_tsp_id);
	data->mailbox2_cpu2_to_cpu0 = devm_ioremap(&pdev->dev, reg_base, reg_len);
	if (!data->mailbox2_cpu2_to_cpu0) {
        dev_err(&pdev->dev, "Failed to map IO region\n");
        return -ENXIO;
    }

	res_mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cm4_imu_reserved");
	data->imu_data_addr = devm_ioremap_resource(&pdev->dev, res_mem);
	if (!data->imu_data_addr) {
        dev_err(&pdev->dev, "Failed to map IO region imu_data_addr\n");
        return -ENXIO;
    }
	data->hwlock = devm_hwspin_lock_request_specific(&pdev->dev, ret);
	if (!data->hwlock) {
		dev_err(&pdev->dev, "failed to request hwspinlock\n");
		return -ENXIO;
	}

	return ret;
}
EXPORT_SYMBOL_NS_GPL(bmi088c_core_probe, IIO_BMI088);

int bmi088c_core_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(&pdev->dev);
	iio_device_unregister(indio_dev);
	return 0;
}
EXPORT_SYMBOL_NS_GPL(bmi088c_core_remove, IIO_BMI088);

MODULE_AUTHOR("Jason <jason.wang@vicoretek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BMI088 accelerometer and gyroscope driver (core)");
