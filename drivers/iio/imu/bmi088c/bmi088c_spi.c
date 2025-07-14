// SPDX-License-Identifier: GPL-2.0
/*
 * 3-axis accelerometer driver supporting following Bosch-Sensortec chips:
 *  - BMI088
 *
 * Copyright (c) 2018-2020, Topic Embedded Products
 */
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/of_platform.h>
#include "bmi088c.h"

static int bmi088c_probe(struct platform_device *pdev)
{
	return bmi088c_core_probe(pdev, BOSCH_BMI088);
}

static int bmi088c_remove(struct platform_device *pdev)
{
	return bmi088c_core_remove(pdev);
}

static const struct of_device_id bmi088_of_match[] = {
	{ .compatible = "bosch,bmi088c" },
	{}
};
MODULE_DEVICE_TABLE(of, bmi088_of_match);

static struct platform_driver bmi088_driver = {
	.driver = {
		.name	= "bmi088c",
		.of_match_table = bmi088_of_match,
	},
	.probe		= bmi088c_probe,
	.remove		= bmi088c_remove,
};

module_platform_driver(bmi088_driver);
MODULE_AUTHOR("Jason <jason.wang@vicoretek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BMI088 accelerometer and gyroscope driver");
MODULE_IMPORT_NS(IIO_BMI088);