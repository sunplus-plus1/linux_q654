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
#include "bmi088.h"

static int bmi088_probe(struct spi_device *spi)
{
    struct regmap *regmap;

	regmap = devm_regmap_init_spi(spi, &bmi088_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&spi->dev, "Failed to register spi regmap: %pe\n",
			regmap);
		return PTR_ERR(regmap);
	}
	spi->mode = SPI_MODE_3;
	return bmi088_core_probe(&spi->dev, regmap, spi->irq,
					BOSCH_BMI088);
}

static int bmi088_remove(struct spi_device *spi)
{
	return bmi088_core_remove(&spi->dev);
}

static const struct of_device_id bmi088_of_match[] = {
	{ .compatible = "bosch,bmi088" },
	{}
};
MODULE_DEVICE_TABLE(of, bmi088_of_match);

static const struct spi_device_id bmi088_id[] = {
	{"bmi088",  BOSCH_BMI088},
	{}
};
MODULE_DEVICE_TABLE(spi, bmi088_id);

static struct spi_driver bmi088_driver = {
	.driver = {
		.name	= "bmi088_spi",
		.of_match_table = bmi088_of_match,
	},
	.probe		= bmi088_probe,
	.remove		= bmi088_remove,
	.id_table	= bmi088_id,
};

module_spi_driver(bmi088_driver);
MODULE_AUTHOR("Jason <jason.wang@vicoretek.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BMI088 accelerometer and gyroscope driver (SPI)");
MODULE_IMPORT_NS(IIO_BMI088);