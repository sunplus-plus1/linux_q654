/* Copyright (C)  Vicorelogic, Inc - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *  Written by Neo <neo.chang@vicorelogic.com>, 2022.11
 */
#ifndef VICORE_TSP_DEV_H
#define VICORE_TSP_DEV_H

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

#endif /* VICORE_TSP_DEV_H */
