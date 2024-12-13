/*
 * Copyright (C) 2017-2021 InvenSense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _INV_MPU_IIO_REG_43600_H_
#define _INV_MPU_IIO_REG_43600_H_

/* Comment out to use lower power feature
 * by BIT_DMP_POWER_SAVE_EN
 */
#define NOT_SET_DMP_POWER_SAVE

/* Comment out not to use lower power mode on accel */
#define SUPPORT_ACCEL_LPM

/* Registers and associated bit definitions */
/* Bank 0 */
#define REG_MISC_1				0x00
#define REG_CHIP_CONFIG_REG			0x01
#define REG_SIGNAL_PATH_RESET			0x02
#define REG_DRIVE_CONFIG_REG1			0x03
#define REG_DRIVE_CONFIG_REG2			0x04
#define REG_DRIVE_CONFIG_REG3			0x05
#define REG_INT_CONFIG_REG			0x06
#define REG_ODRGRID0				0x07
#define REG_ODRGRID1				0x08
#define REG_TEMP_DATA0_UI			0x09
#define REG_TEMP_DATA1_UI			0x0a
#define REG_ACCEL_DATA_X0_UI			0x0b
#define REG_ACCEL_DATA_X1_UI			0x0c
#define REG_ACCEL_DATA_Y0_UI			0x0d
#define REG_ACCEL_DATA_Y1_UI			0x0e
#define REG_ACCEL_DATA_Z0_UI			0x0f
#define REG_ACCEL_DATA_Z1_UI			0x10
#define REG_GYRO_DATA_X0_UI			0x11
#define REG_GYRO_DATA_X1_UI			0x12
#define REG_GYRO_DATA_Y0_UI			0x13
#define REG_GYRO_DATA_Y1_UI			0x14
#define REG_GYRO_DATA_Z0_UI			0x15
#define REG_GYRO_DATA_Z1_UI			0x16
#define REG_TMST_FSYNC1				0x17
#define REG_TMST_FSYNC2				0x18
#define REG_ODR_LP_STATUS			0x19
#define REG_PWR_MGMT_0				0x1f
#define REG_GYRO_CONFIG0			0x20
#define REG_ACCEL_CONFIG0			0x21
#define REG_TEMP_CONFIG0			0x22
#define REG_GYRO_CONFIG1			0x23
#define REG_ACCEL_CONFIG1			0x24
#define REG_APEX_CONFIG0			0x25
#define REG_APEX_CONFIG1			0x26
#define REG_WOM_CONFIG				0x27
#define REG_FIFO_CONFIG1			0x28
#define REG_FIFO_CONFIG2			0x29
#define REG_FIFO_CONFIG3			0x2a
#define REG_INT_SOURCE0				0x2b
#define REG_INT_SOURCE1				0x2c
#define REG_INT_SOURCE3				0x2d
#define REG_INT_SOURCE4				0x2e
#define REG_FIFO_LOST_PKT0			0x2f
#define REG_FIFO_LOST_PKT1			0x30
#define REG_APEX_DATA0				0x31
#define REG_APEX_DATA1				0x32
#define REG_APEX_DATA2				0x33
#define REG_APEX_DATA3				0x34
#define REG_INTF_CONFIG0			0x35
#define REG_INTF_CONFIG1			0x36
#define REG_INT_STATUS_DRDY			0x39
#define REG_INT_STATUS				0x3a
#define REG_INT_STATUS2				0x3b
#define REG_INT_STATUS3				0x3c
#define REG_FIFO_BYTE_COUNT1			0x3d
#define REG_FIFO_BYTE_COUNT2			0x3e
#define REG_FIFO_DATA_REG			0x3f
#define REG_S4S_GYRO_TPH1			0x40
#define REG_S4S_GYRO_TPH2			0x41
#define REG_S4S_ACCEL_TPH1			0x42
#define REG_S4S_ACCEL_TPH2			0x43
#define REG_S4S_RR				0x44
#define REG_GYR_BIAS_CFG1			0x46
#define REG_WHO_AM_I				0x75
#define REG_S4S_ST				0x76
#define REG_S4S_ST_CLONE			0x77
#define REG_S4S_DT				0x78
#define REG_BLK_SEL_W				0x79
#define REG_MADDR_W				0x7a
#define REG_M_W					0x7b
#define REG_BLK_SEL_R				0x7c
#define REG_MADDR_R				0x7d
#define REG_M_R					0x7e

/* MREG_TOP1 */
#define REG_TMST_CONFIG1_MREG_TOP1		0x00
#define REG_FIFO_CONFIG5_MREG_TOP1		0x01
#define REG_FIFO_CONFIG6_MREG_TOP1		0x02
#define REG_FSYNC_CONFIG_MREG_TOP1		0x03
#define REG_INT_CONFIG0_MREG_TOP1		0x04
#define REG_INT_CONFIG1_MREG_TOP1		0x05
#define REG_AFSR_CONFIG0_MREG_TOP1		0x07
#define REG_AFSR_CONFIG1_MREG_TOP1		0x08
#define REG_TBC_RCOSC_MREG_TOP1			0x0d
#define REG_TBC_PLL_MREG_TOP1			0x0e
#define REG_ST_CONFIG_MREG_TOP1			0x13
#define REG_SELFTEST_MREG_TOP1			0x14
#define REG_PADS_CONFIG3_MREG_TOP1		0x17
#define REG_TEMP_CONFIG1_MREG_TOP1		0x1c
#define REG_TEMP_CONFIG3_MREG_TOP1		0x1e
#define REG_S4S_CONFIG1_MREG_TOP1		0x1f
#define REG_S4S_CONFIG2_MREG_TOP1		0x20
#define REG_S4S_FREQ_RATIO1_MREG_TOP1		0x21
#define REG_S4S_FREQ_RATIO2_MREG_TOP1		0x22
#define REG_INTF_CONFIG6_MREG_TOP1		0x23
#define REG_INTF_CONFIG10_MREG_TOP1		0x25
#define REG_INTF_CONFIG7_MREG_TOP1		0x28
#define REG_OTP_CONFIG_MREG_TOP1		0x2b
#define REG_INT_SOURCE6_MREG_TOP1		0x2f
#define REG_INT_SOURCE7_MREG_TOP1		0x30
#define REG_INT_SOURCE8_MREG_TOP1		0x31
#define REG_INT_SOURCE9_MREG_TOP1		0x32
#define REG_INT_SOURCE10_MREG_TOP1		0x33
#define REG_GYRO_PWR_CFG0_MREG_TOP1		0x38
#define REG_ACCEL_CP_CFG0_MREG_TOP1		0x39
#define REG_APEX_CONFIG2_MREG_TOP1		0x44
#define REG_APEX_CONFIG3_MREG_TOP1		0x45
#define REG_APEX_CONFIG4_MREG_TOP1		0x46
#define REG_APEX_CONFIG5_MREG_TOP1		0x47
#define REG_APEX_CONFIG9_MREG_TOP1		0x48
#define REG_APEX_CONFIG10_MREG_TOP1		0x49
#define REG_APEX_CONFIG11_MREG_TOP1		0x4a
#define REG_APEX_CONFIG12_MREG_TOP1		0x67
#define REG_ACCEL_WOM_X_THR_MREG_TOP1		0x4b
#define REG_ACCEL_WOM_Y_THR_MREG_TOP1		0x4c
#define REG_ACCEL_WOM_Z_THR_MREG_TOP1		0x4d
#define REG_GOS_USER0_MREG_TOP1			0x4e
#define REG_GOS_USER1_MREG_TOP1			0x4f
#define REG_GOS_USER2_MREG_TOP1			0x50
#define REG_GOS_USER3_MREG_TOP1			0x51
#define REG_GOS_USER4_MREG_TOP1			0x52
#define REG_GOS_USER5_MREG_TOP1			0x53
#define REG_GOS_USER6_MREG_TOP1			0x54
#define REG_GOS_USER7_MREG_TOP1			0x55
#define REG_GOS_USER8_MREG_TOP1			0x56
#define REG_ST_STATUS1_MREG_TOP1		0x63
#define REG_ST_STATUS2_MREG_TOP1		0x64

/* MMEM_TOP */
#define REG_XG_ST_DATA_MMEM_TOP			0x5000
#define REG_YG_ST_DATA_MMEM_TOP			0x5001
#define REG_ZG_ST_DATA_MMEM_TOP			0x5002
#define REG_XA_ST_DATA_MMEM_TOP			0x5003
#define REG_YA_ST_DATA_MMEM_TOP			0x5004
#define REG_ZA_ST_DATA_MMEM_TOP			0x5005

/* MREG_OTP */
#define REG_OTP_CTRL7_MREG_OTP			0x2806


/* Bank0 REG_GYRO_CONFIG0/REG_ACCEL_CONFIG0 */
#define SHIFT_GYRO_FS_SEL			5
#define SHIFT_ACCEL_FS_SEL			5
#define SHIFT_ODR_CONF				0
#define BIT_GYRO_FSR				0x60
#define BIT_GYRO_ODR				0x0F
#define BIT_ACCEL_FSR				0x60
#define BIT_ACCEL_ODR				0x0F
#define BIT_SENSOR_ODR_800HZ			0x06
#define BIT_SENSOR_ODR_400HZ			0x07
#define BIT_SENSOR_ODR_200HZ			0x08
#define BIT_SENSOR_ODR_100HZ			0x09
#define BIT_SENSOR_ODR_50HZ			0x0A
#define BIT_SENSOR_ODR_25HZ			0x0B
#define BIT_SENSOR_ODR_12HZ			0x0C
#define BIT_SENSOR_ODR_6HZ			0x0D
#define BIT_SENSOR_ODR_3HZ			0x0E

/* Bank0 REG_GYRO_CONFIG1 */
#define BIT_GYR_UI_FLT_BW_BYPASS		0x00
#define BIT_GYR_UI_FLT_BW_180HZ			0x01
#define BIT_GYR_UI_FLT_BW_121HZ			0x02
#define BIT_GYR_UI_FLT_BW_73HZ			0x03
#define BIT_GYR_UI_FLT_BW_53HZ			0x04
#define BIT_GYR_UI_FLT_BW_34HZ			0x05
#define BIT_GYR_UI_FLT_BW_25HZ			0x06
#define BIT_GYR_UI_FLT_BW_16HZ			0x07
#define BIT_GYR_UI_AVG_IND_2X			0x00
#define BIT_GYR_UI_AVG_IND_4X			0x10
#define BIT_GYR_UI_AVG_IND_8X			0x20
#define BIT_GYR_UI_AVG_IND_16X			0x30
#define BIT_GYR_UI_AVG_IND_32X			0x40
#define BIT_GYR_UI_AVG_IND_64X			0x50

/* Bank0 REG_ACCEL_CONFIG1 */
#define BIT_ACC_FILT_BW_IND_BYPASS		0x00
#define BIT_ACC_FILT_BW_IND_180HZ		0x01
#define BIT_ACC_FILT_BW_IND_121HZ		0x02
#define BIT_ACC_FILT_BW_IND_73HZ		0x03
#define BIT_ACC_FILT_BW_IND_53HZ		0x04
#define BIT_ACC_FILT_BW_IND_34HZ		0x05
#define BIT_ACC_FILT_BW_IND_25HZ		0x06
#define BIT_ACC_FILT_BW_IND_16HZ		0x07
#define BIT_ACC_UI_AVG_IND_2X			0x00
#define BIT_ACC_UI_AVG_IND_4X			0x10
#define BIT_ACC_UI_AVG_IND_8X			0x20
#define BIT_ACC_UI_AVG_IND_16X			0x30
#define BIT_ACC_UI_AVG_IND_32X			0x40
#define BIT_ACC_UI_AVG_IND_64X			0x50

/* Bank0 REG_INT_CONFIG_REG */
#define SHIFT_INT1_MODE				0x02
#define SHIFT_INT1_DRIVE_CIRCUIT		0x01
#define SHIFT_INT1_POLARITY			0x00

/* Bank0 REG_PWR_MGMT_0 */
#define BIT_ACCEL_MODE_OFF			0x00
#define BIT_ACCEL_MODE_LPM			0x02
#define BIT_ACCEL_MODE_LNM			0x03
#define BIT_ACCEL_MODE_MASK			0x03
#define BIT_GYRO_MODE_OFF			0x00
#define BIT_GYRO_MODE_STBY			0x04
#define BIT_GYRO_MODE_LPM			0x08
#define BIT_GYRO_MODE_LNM			0x0c
#define BIT_GYRO_MODE_MASK			0x0c
#define BIT_IDLE				0x10
#define BIT_ACCEL_LP_CLK_SEL			0x80

/* Bank0 REG_SIGNAL_PATH_RESET */
#define BIT_FIFO_FLUSH				0x04
#define BIT_SOFT_RESET_CHIP_CONFIG		0x10

/* Bank0 REG_INTF_CONFIG0 */
#define BIT_SIFS_CFG_I2C_ONLY			0x02
#define BIT_SIFS_CFG_SPI_ONLY			0x03
#define BIT_SENSOR_DATA_ENDIAN			0x10
#define BIT_FIFO_COUNT_ENDIAN			0x20
#define BIT_FIFO_COUNT_FORMAT			0x40
#define BIT_FIFO_SREG_INVALID_IND_DIS		0x80

/* Bank0 REG_INTF_CONFIG1 */
#define BIT_CLK_SEL_RC				0x00
#define BIT_CLK_SEL_PLL				0x01
#define BIT_CLK_SEL_DIS				0x03
#define BIT_I3C_DDR_EN				0x04
#define BIT_I3C_SDR_EN				0x08
#define BIT_GYRO_AFSR_MODE_LFS			0x00
#define BIT_GYRO_AFSR_MODE_HFS			0x20
#define BIT_GYRO_AFSR_MODE_DYN			0x40

/* Bank0 REG_FIFO_CONFIG1 */
#define BIT_FIFO_MODE_NO_BYPASS			0x00
#define BIT_FIFO_MODE_BYPASS			0x01
#define BIT_FIFO_MODE_STREAM			0x00
#define BIT_FIFO_MODE_STOPFULL			0x02

/* Bank 0 REG_INT_SOURCE0 */
#define BIT_INT_AGC_RDY_INT1_EN			0x01
#define BIT_INT_FIFO_FULL_INT1_EN		0x02
#define BIT_INT_FIFO_THS_INT1_EN		0x04
#define BIT_INT_DRDY_INT_EN			0x08
#define BIT_INT_RESET_DONE_INT1_EN		0x10
#define BIT_INT_PLL_RDY_INT1_EN			0x20
#define BIT_INT_FSYNC_INT1_EN			0x40
#define BIT_INT_ST_DONE_INT1_EN			0x80

/* Bank 0 REG_INT_SOURCE1 */
#define BIT_INT_WOM_X_INT1_EN			0x01
#define BIT_INT_WOM_Y_INT1_EN			0x02
#define BIT_INT_WOM_Z_INT1_EN			0x04
#define BIT_INT_WOM_XYZ_INT1_EN (BIT_INT_WOM_X_INT1_EN | \
		BIT_INT_WOM_Y_INT1_EN | BIT_INT_WOM_Z_INT1_EN)
#define BIT_INT_SMD_INT1_EN			0x08
#define BIT_INT_I3C_PROTCL_ERR_INT1_EN		0x40

/* Bank0 REG_INT_STATUS_DRDY */
#define BIT_INT_STATUS_DRDY			0x01

/* Bank0 REG_INT_STATUS */
#define BIT_INT_STATUS_AGC_RDY			0x01
#define BIT_INT_STATUS_FIFO_FULL		0x02
#define BIT_INT_STATUS_FIFO_THS			0x04
#define BIT_INT_STATUS_RESET_DONE		0x10
#define BIT_INT_STATUS_PLL_RDY			0x20
#define BIT_INT_STATUS_FSYNC			0x40
#define BIT_INT_STATUS_ST_DONE			0x80

/* Bank0 REG_INT_STATUS2 */
#define BIT_INT_STATUS_WOM_Z			0x01
#define BIT_INT_STATUS_WOM_Y			0x02
#define BIT_INT_STATUS_WOM_X			0x04
#define BIT_INT_STATUS_WOM_XYZ (BIT_INT_STATUS_WOM_X | \
		BIT_INT_STATUS_WOM_Y | BIT_INT_STATUS_WOM_Z)
#define BIT_INT_STATUS_SMD			0x08

/* Bank 0 REG_INT_STATUS3 */
#define BIT_INT_STATUS_LOWG_DET			0x02
#define BIT_INT_STATUS_FF_DET			0x04
#define BIT_INT_STATUS_TILT_DET			0x08
#define BIT_INT_STATUS_STEP_CNT_OVFL		0x10
#define BIT_INT_STATUS_STEP_DET			0x20

/* Bank0 REG_WOM_CONFIG */
#define BIT_WOM_EN_OFF				0x00
#define BIT_WOM_EN_ON				0x01
#define BIT_WOM_MODE_INITIAL			0x00
#define BIT_WOM_MODE_PREV			0x02
#define BIT_WOM_INT_MODE_OR			0x00
#define BIT_WOM_INT_MODE_AND			0x04
#define BIT_WOM_INT_DUR_LEGACY			0x00
#define BIT_WOM_INT_DUR_2ND			0x08
#define BIT_WOM_INT_DUR_3RD			0x10
#define BIT_WOM_INT_DUR_4TH			0x18

/* Bank0 REG_APEX_CONFIG0 */
#define BIT_DMP_SRAM_RESET_APEX			0x01
#define BIT_DMP_INIT_EN				0x04
#define BIT_DMP_POWER_SAVE_EN			0x08

/* Bank0 REG_APEX_CONFIG1 */
#define BIT_DMP_ODR_25HZ			0x00
#define BIT_DMP_ODR_50HZ			0x02
#define BIT_DMP_ODR_100HZ			0x03
#define BIT_DMP_PEDO_EN				0x08
#define BIT_DMP_TILT_EN				0x10
#define BIT_DMP_FF_EN				0x20
#define BIT_DMP_SMD_EN				0x40

/* REG_OTP_CONFIG_MREG_TOP1 */
#define BIT_OTP_COPY_NORMAL			0x04
#define BIT_OTP_COPY_ST_DATA			0x0C
#define OTP_COPY_MODE_MASK                      0x0C

/* REG_INT_SOURCE6_MREG_TOP1 */
#define BIT_INT_TLT_DET_INT1_EN			0x08
#define BIT_INT_STEP_CNT_OVFL_INT1_EN		0x10
#define BIT_INT_STEP_DET_INT1_EN		0x20
#define BIT_INT_LOWG_INT1_EN			0x40
#define BIT_INT_FF_INT1_EN			0x80

/* REG_TMST_CONFIG1_MREG_TOP1 */
#define BIT_TMST_EN				0x01
#define BIT_TMST_FSYNC_EN			0x02
#define BIT_TMST_DELTA_EN			0x04
#define BIT_TMST_RESOL				0x08
#define BIT_TMST_ON_SREG_EN			0x10
#define BIT_ODR_EN_WITHOUT_SENSOR		0x40

/* REG_FIFO_CONFIG5_MREG_TOP1 */
#define BIT_FIFO_ACCEL_EN			0x01
#define BIT_FIFO_GYRO_EN			0x02
#define BIT_FIFO_TMST_FSYNC_EN			0x04
#define BIT_FIFO_HIRES_EN			0x08
#define BIT_RESUME_PARTIAL_RD			0x10
#define BIT_WM_GT_TH				0x20

/* REG_SELFTEST_MREG_TOP1 */
#define BIT_EN_AX_ST				0x01
#define BIT_EN_AY_ST				0x02
#define BIT_EN_AZ_ST				0x04
#define BIT_EN_GX_ST				0x08
#define BIT_EN_GY_ST				0x10
#define BIT_EN_GZ_ST				0x20
#define BIT_ACCEL_ST_EN				0x40
#define BIT_GYRO_ST_EN				0x80

/* REG_ST_CONFIG_MREG_TOP1 */
#define BIT_PD_ACCEL_CP45_ST_REG		0x80
#define SHIFT_GYRO_ST_LIM			0
#define SHIFT_ACCEL_ST_LIM			3
#define SHIFT_ST_NUM_SAMPLE			6

/* REG_ST_STATUS1_MREG_TOP1 */
#define BIT_DMP_AX_ST_PASS			0x02
#define BIT_DMP_AY_ST_PASS			0x04
#define BIT_DMP_AZ_ST_PASS			0x08
#define BIT_DMP_ACCEL_ST_DONE			0x10
#define BIT_DMP_ACCEL_ST_PASS			0x20

/* REG_ST_STATUS2_MREG_TOP1 */
#define BIT_DMP_GX_ST_PASS			0x02
#define BIT_DMP_GY_ST_PASS			0x04
#define BIT_DMP_GZ_ST_PASS			0x08
#define BIT_DMP_GYRO_ST_DONE			0x10
#define BIT_DMP_GYRO_ST_PASS			0x20
#define BIT_DMP_ST_INCOMPLETE			0x40

/* REG_OTP_CTRL7_MREG_OTP */
#define BIT_OTP_RELOAD				0x08
#define BIT_OTP_PWR_DOWN			0x02


/* fifo data packet header */
#define BIT_FIFO_HEAD_MSG			0x80
#define BIT_FIFO_HEAD_ACCEL			0x40
#define BIT_FIFO_HEAD_GYRO			0x20
#define BIT_FIFO_HEAD_20			0x10
#define BIT_FIFO_HEAD_TMSP_ODR			0x08
#define BIT_FIFO_HEAD_TMSP_NO_ODR		0x04
#define BIT_FIFO_HEAD_TMSP_FSYNC		0x0C
#define BIT_FIFO_HEAD_ODR_ACCEL			0x02
#define BIT_FIFO_HEAD_ODR_GYRO			0x01

/* data definitions */
#define FIFO_PACKET_BYTE_SINGLE			8
#define FIFO_PACKET_BYTE_6X			16
#define FIFO_PACKET_BYTE_HIRES			20
#define FIFO_COUNT_BYTE				2

/* sensor startup time */
#define INV_ICM43600_GYRO_START_TIME		100
#define INV_ICM43600_ACCEL_START_TIME		100

/* sensor stop time */
#define INV_ICM43600_GYRO_STOP_TIME		20

/* M-reg access wait tile */
#define INV_ICM43600_MCLK_WAIT_US		20
#define INV_ICM43600_BLK_SEL_WAIT_US		10
#define INV_ICM43600_MADDR_WAIT_US		10
#define INV_ICM43600_M_RW_WAIT_US		10

/* temperature sensor */
#define TEMP_SCALE				100 /* scale by 100 */
#define TEMP_LSB_PER_DEG			2   /* 2LSB=1degC */
#define TEMP_OFFSET				25  /* 25 degC */


/* enum for sensor */
enum INV_SENSORS {
	SENSOR_ACCEL = 0,
	SENSOR_TEMP,
	SENSOR_GYRO,
	SENSOR_COMPASS,
	SENSOR_NUM_MAX,
	SENSOR_INVALID,
};

#define BASE_SAMPLE_RATE		800
#define GESTURE_ACCEL_RATE		50
#define ESI_GYRO_RATE			800
#define MPU_INIT_SENSOR_RATE_LNM	12	/* min Hz in LNM */
#define MPU_INIT_SENSOR_RATE_LPM	3	/* min Hz in LPM */
#define MAX_FIFO_PACKET_READ		16
#define HARDWARE_FIFO_SIZE		1024
/* ~7/8 of hardware FIFO and a multiple of packet sizes 8/16/20 */
#define FIFO_SIZE			880
#define LEFT_OVER_BYTES			128
#define POWER_UP_TIME			100
#define REG_UP_TIME_USEC		100
#define IIO_BUFFER_BYTES		8
#define REG_FIFO_COUNT_H		REG_FIFO_BYTE_COUNT1
#define BYTES_PER_SENSOR		6
#define BYTES_FOR_TEMP			1
#define MAX_BATCH_FIFO_SIZE		FIFO_SIZE
#define FIRST_DROP_SAMPLES_ACC_800HZ	20
#define FIRST_DROP_SAMPLES_ACC_200HZ	10
#define FIRST_DROP_SAMPLES_GYR_800HZ	20
#define FIRST_DROP_SAMPLES_GYR_200HZ	10
#define WOM_THRESHOLD			13 /* 1000 / 256 * 13 = 50.7mg */


/*
 * INT configurations
 * Polarity: 0 -> Active Low, 1 -> Active High
 * Drive circuit: 0 -> Open Drain, 1 -> Push-Pull
 * Mode: 0 -> Pulse, 1 -> Latch
 */
#define INT_POLARITY			1
#define INT_DRIVE_CIRCUIT		1
#define INT_MODE			0

#define ACC_LPM_MAX_RATE		(400)

typedef union {
	unsigned char Byte;
	struct {
		unsigned char g_odr_change_bit:1;
		unsigned char a_odr_change_bit:1;
		unsigned char timestamp_bit:2;
		unsigned char twentybits_bit:1;
		unsigned char gyro_bit:1;
		unsigned char accel_bit:1;
		unsigned char msg_bit:1;
	} bits;
} icm4x6xx_fifo_header_t;

enum inv_devices {
	ICM20608D,
	ICM20789,
	ICM20690,
	ICM20602,
	IAM20680,
	ICM42600,
	ICM42686,
	ICM42688,
	ICM40609D,
	ICM43600,
	ICM45600,
	INV_NUM_PARTS,
};

/* chip specific functions */
struct inv_mpu_state;
int inv_get_43600_pedometer_steps(struct inv_mpu_state *st,
	int *ped, int *update);
int inv_set_idle(struct inv_mpu_state *st);
int inv_reset_idle(struct inv_mpu_state *st);
int inv_mreg_single_write(struct inv_mpu_state *st, int addr, u8 data);
int inv_mreg_read(struct inv_mpu_state *st, int addr, int len, u8 *data);
bool inv_get_apex_enabled(struct inv_mpu_state *st);
int inv_get_apex_odr(struct inv_mpu_state *st);

#endif /* _INV_MPU_IIO_REG_43600_H_ */
