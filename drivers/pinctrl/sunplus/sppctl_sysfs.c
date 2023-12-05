// SPDX-License-Identifier: GPL-2.0
/*
 * SP7021 pinmux controller driver.
 * Copyright (C) Sunplus Tech/Tibbo Tech. 2020
 * Author: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "sppctl_sysfs.h"
#include "sppctl_gpio_ops.h"
#include "sppctl_pinctrl.h"


static ssize_t sppctl_sop_dbgi_R(struct device *_d, struct device_attribute *_a, char *_b)
{
	struct sppctl_pdata_t *_p = (struct sppctl_pdata_t *)_d->platform_data;

	return sprintf(_b, "%d\n", _p->debug);
}

static ssize_t sppctl_sop_dbgi_W(struct device *_d, struct device_attribute *_a, const char *_b,
				 size_t _c)
{
	int x;

	struct sppctl_pdata_t *_p = (struct sppctl_pdata_t *)_d->platform_data;

	if (kstrtoint(_b, 10, &x) < 0)
		return -EIO;
	_p->debug = x;

	return _c;
}

static ssize_t sppctl_sop_txt_map_R(struct file *filp, struct kobject *_k,
	struct bin_attribute *_a, char *_b, loff_t off, size_t count)
{
	int i = -1, j = 0, ret = 0, pos = off;
	char tmps[SPPCTL_MAX_NAM + 3];
	uint8_t pin = 0;
	struct sppctl_pdata_t *_p = NULL;
	struct func_t *f;
	struct device *_pdev = container_of(_k, struct device, kobj);

	if (!_pdev)
		return -ENXIO;

	_p = (struct sppctl_pdata_t *)_pdev->platform_data;
	if (!_p)
		return -ENXIO;

	for (i = 0; i < list_funcsSZ; i++) {
		f = &(list_funcs[i]);
		pin = 0;
		if (f->freg == fOFF_0)
			continue;
		if (f->freg == fOFF_I)
			continue;
		memset(tmps, 0, SPPCTL_MAX_NAM + 3);

		// muxable pins are P1_xx, stored -7, absolute idx = +7
		pin = sppctl_fun_get(_p, j++);
		if (f->freg == fOFF_M && pin > 0)
			pin += 7;
		if (f->freg == fOFF_G)
			pin = sppctl_gmx_get(_p, f->roff, f->boff, f->blen);
		sprintf(tmps, "%03d %s", pin, f->name);

		if (pos > 0) {
			pos -= (strlen(tmps) + 1);
			continue;
		}
		sprintf(_b + ret, "%s\n", tmps);
		ret += strlen(tmps) + 1;
		if (ret > SPPCTL_MAX_BUF - SPPCTL_MAX_NAM)
			break;
	}

	return ret;
}

static struct device_attribute sppctl_sysfs_attrsD[] = {
	__ATTR(dbgi,   0644, sppctl_sop_dbgi_R,   sppctl_sop_dbgi_W),
};

static struct bin_attribute sppctl_sysfs_attrsB[] = {
	__BIN_ATTR(txt_map,    0444, sppctl_sop_txt_map_R,    NULL,            SPPCTL_MAX_BUF),
};

struct bin_attribute *sppctl_sysfs_Fap;

// ---------- main (exported) functions
void sppctl_sysfs_init(struct platform_device *_pd)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_attrsD); i++) {
		ret = device_create_file(&(_pd->dev), &sppctl_sysfs_attrsD[i]);
		if (ret)
			KERR(&(_pd->dev), "createD[%d] error\n", i);
	}

	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_attrsB); i++) {
		ret = device_create_bin_file(&(_pd->dev), &sppctl_sysfs_attrsB[i]);
		if (ret)
			KERR(&(_pd->dev), "createB[%d] error\n", i);
	}
}

void sppctl_sysfs_clean(struct platform_device *_pd)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_attrsD); i++)
		device_remove_file(&(_pd->dev), &sppctl_sysfs_attrsD[i]);
	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_attrsB); i++)
		device_remove_bin_file(&(_pd->dev), &sppctl_sysfs_attrsB[i]);
}
