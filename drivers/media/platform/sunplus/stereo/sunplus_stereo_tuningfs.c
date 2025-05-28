/*!
 * sunplus_stereo_tuningfs.c - stereo tuning system for stereo hardware
 * @file sunplus_stereo_tuningfs.c
 * @brief  stereo tuning file system for stereo hardware
 * @author Saxen Ko <saxen.ko@sunplus.com>
 * @version 1.0
 * @copyright  Copyright (C) 2025 Sunplus
 * @note
 * Copyright (C) 2025 Sunplus
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

/* include Sunplus Header files */
#include "sunplus_stereo_reg.h"
#include "sunplus_stereo_util.h"

struct vcl_reg_range {
	u32 start;
	u32 end;
};

// const char stereo_tuning_group_name[] = "tuning";
const struct vcl_reg_range reg_tab[] = {
	{0, 0x44},
	{0x100, 0x17C},
	{0x200, 0x4FC},
	// {0x500, 0x5AC},
	// {0xC00, 0xC34}
};

static struct clk *stereo_clk = NULL;

static ssize_t stereo_tuning_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	u32 i, size;
	u32 addr;
	int ret;

	if (!stereo_clk) {
		pr_err("%s enable clock failed\n", __func__);
		return -ENODEV;
	}

	ret = clk_enable(stereo_clk);
	if (ret) {
		pr_err("%s prepare clock failed\n", __func__);
		return -EIO;
	}
	size = sizeof(reg_tab) / sizeof(struct vcl_reg_range);
	for (i = 0; i < size; i++) {
		for (addr = reg_tab[i].start; addr <= reg_tab[i].end; addr+=4) {
			printk("%.3x %.8x\n", addr, stereo_read(addr));
		}
	}
	clk_disable(stereo_clk);

	return 0;
}

static ssize_t stereo_tuning_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	return 0;
}

static DEVICE_ATTR_RW(stereo_tuning);

static const struct attribute *stereo_attrs[] = {
	&dev_attr_stereo_tuning.attr,
	NULL,
};

// static const struct attribute_group stereo_attr_group = {
// 	.name	= stereo_tuning_group_name,
// 	.attrs	= stereo_attrs,
// };

int sunplus_stereo_tuningfs_init(struct kobject *stereo_kobj, struct clk *clk_gate)
{
	stereo_clk = clk_gate;
	return sysfs_create_files(stereo_kobj, stereo_attrs);
}

// void sunplus_stereo_tuningfs_remove(struct kobject *stereo_kobj)
// {
// 	sysfs_remove_groups(stereo_kobj, &stereo_attr_group);
// }
