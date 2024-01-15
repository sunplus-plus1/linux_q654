// SPDX-License-Identifier: GPL-2.0

#include "sppctl_sysfs.h"
#include "sppctl_gpio_ops.h"
#include "sppctl_pinctrl.h"

static ssize_t sppctl_sop_dbgi_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct sppctl_pdata_t *pdata;

	pdata = (struct sppctl_pdata_t *)dev->platform_data;

	return sprintf(buf, "%d\n", pdata->debug);
}

static ssize_t sppctl_sop_dbgi_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct sppctl_pdata_t *pdata;
	int x;

	pdata = (struct sppctl_pdata_t *)dev->platform_data;

	if (kstrtoint(buf, 10, &x) < 0)
		return -EIO;
	pdata->debug = x;

	return count;
}

static ssize_t sppctl_sop_txt_map_read(struct file *file, struct kobject *kobj,
				       struct bin_attribute *attr, char *buf,
				       loff_t off, size_t count)
{
	int i = -1, j = 0, ret = 0, pos = off;
	char tmps[SPPCTL_MAX_NAM + 3];
	struct sppctl_pdata_t *pdata;
	struct func_t *func;
	struct device *dev;
	u8 pin = 0;

	dev = container_of(kobj, struct device, kobj);
	if (!dev)
		return -ENXIO;

	pdata = (struct sppctl_pdata_t *)dev->platform_data;
	if (!pdata)
		return -ENXIO;

	for (i = 0; i < list_func_nums; i++) {
		func = &list_funcs[i];
		pin = 0;
		if (func->freg == F_OFF_0)
			continue;
		if (func->freg == F_OFF_I)
			continue;
		memset(tmps, 0, SPPCTL_MAX_NAM + 3);

		// muxable pins are P1_xx, stored -7, absolute idx = +7
		pin = sppctl_fun_get(pdata, j++);
		if (func->freg == F_OFF_M && pin > 0)
			pin += 7;
		if (func->freg == F_OFF_G)
			pin = sppctl_gmx_get(pdata, func->roff, func->boff,
					     func->blen);
		sprintf(tmps, "%03d %s", pin, func->name);

		if (pos > 0) {
			pos -= (strlen(tmps) + 1);
			continue;
		}
		sprintf(buf + ret, "%s\n", tmps);
		ret += strlen(tmps) + 1;
		if (ret > SPPCTL_MAX_BUF - SPPCTL_MAX_NAM)
			break;
	}

	return ret;
}

static struct device_attribute sppctl_sysfs_dev_attrs[] = {
	__ATTR(dbgi, 0644, sppctl_sop_dbgi_show, sppctl_sop_dbgi_store),
};

static struct bin_attribute sppctl_sysfs_bin_attrs[] = {
	__BIN_ATTR(txt_map, 0444, sppctl_sop_txt_map_read, NULL,
		   SPPCTL_MAX_BUF),
};

//struct bin_attribute *sppctl_sysfs_Fap;

// ---------- main (exported) functions
void sppctl_sysfs_init(struct platform_device *pdev)
{
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_dev_attrs); i++) {
		ret = device_create_file(&pdev->dev,
					 &sppctl_sysfs_dev_attrs[i]);
		if (ret)
			KERR(&pdev->dev, "createD[%d] error\n", i);
	}

	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_bin_attrs); i++) {
		ret = device_create_bin_file(&pdev->dev,
					     &sppctl_sysfs_bin_attrs[i]);
		if (ret)
			KERR(&pdev->dev, "createB[%d] error\n", i);
	}
}

void sppctl_sysfs_clean(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_dev_attrs); i++)
		device_remove_file(&pdev->dev, &sppctl_sysfs_dev_attrs[i]);
	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_bin_attrs); i++)
		device_remove_bin_file(&pdev->dev, &sppctl_sysfs_bin_attrs[i]);
}
