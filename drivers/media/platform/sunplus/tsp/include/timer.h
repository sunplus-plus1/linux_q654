/* Copyright (C)  Sunplus, Inc - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#ifndef SUNPLUS_TSP_DEV_H
#define SUNPLUS_TSP_DEV_H

#include <linux/uaccess.h>

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

#endif /* SUNPLUS_TSP_DEV_H */
