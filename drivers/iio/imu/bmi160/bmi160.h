/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BMI160_H_
#define BMI160_H_

#include <linux/iio/iio.h>
#include <linux/regulator/consumer.h>

#if defined(CONFIG_SOC_SP7350)
enum TSP_ID {
	TSP_MIPI0 = 0,
	TSP_MIPI1,
	TSP_MIPI2,
	TSP_MIPI3,
	TSP_MIPI4,
	TSP_MIPI5,
	TSP_IMU0,
	TSP_IMU1,
	TSP_IMU2,
	TSP_IMU3,
	TSP_GLOBAL = 100,
	TSP_ID_MAX,
};

struct bmi160_gyro_t {
	s16 x; /*gyro X  data*/
	s16 y; /*gyro Y  data*/
	s16 z; /*gyro Z  data*/
};

struct bmi160_accel_t {
	s16 x; /*accel X  data*/
	s16 y; /*accel Y  data*/
	s16 z; /*accel Z  data*/
};
#define FIFO_FRAME_CNT 170

/* FIFO Head definition */
#define FIFO_HEAD_A        0x84
#define FIFO_HEAD_G        0x88

#define FIFO_HEAD_G_A        (FIFO_HEAD_G | FIFO_HEAD_A)

#define FIFO_HEAD_SKIP_FRAME        0x40
#define A_BYTES_FRM      6
#define G_BYTES_FRM      6
#define GA_BYTES_FRM     12
#endif

struct bmi160_data {
	struct regmap *regmap;
	struct iio_trigger *trig;
	struct regulator_bulk_data supplies[2];
	struct iio_mount_matrix orientation;
#if defined(CONFIG_SOC_SP7350)
	unsigned char fifo_data[1024] __aligned(4);
	u16 fifo_length;
	u64 odr_acc;
	u64 odr_gyro;
	u8 hwfifo_mode;
	u8 watermark_hwfifo;
	u64 ts_last;
	/*
	 * Ensure natural alignment for timestamp if present.
	 * Max length needed: 2 * 3 channels + 4 bytes padding + 8 byte ts.
	 * If fewer channels are enabled, less space may be needed, as
	 * long as the timestamp is still aligned to 8 bytes.
	 */
	__le16 buf[12] __aligned(8);
	struct timer_list bmi_tmr;
	struct mutex bmi_lock;
	int gpio_pwr_on;
#else
	__le16 buf[12] __aligned(8);
#endif
};

extern const struct regmap_config bmi160_regmap_config;

int bmi160_core_probe(struct device *dev, struct regmap *regmap,
		      const char *name, bool use_spi);

int bmi160_enable_irq(struct regmap *regmap, bool enable);

int bmi160_probe_trigger(struct iio_dev *indio_dev, int irq, u32 irq_type);

#if defined(CONFIG_SOC_SP7350)
extern u64 sunplus_tsp_read(int id);
#endif

#endif  /* BMI160_H_ */
