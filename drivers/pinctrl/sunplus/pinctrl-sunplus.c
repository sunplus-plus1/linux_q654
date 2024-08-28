// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/io.h>
#include "../core.h"
#include "gpio-sp7350.h"

__attribute((unused)) static irqreturn_t gpio_int_0(int irq, void *data)
{
	pr_info("register gpio int0 trigger\n");
	return IRQ_HANDLED;
}

static int sppctl_gpio_new(struct platform_device *pdev, void *platform_data)
{
	struct sppctl_pdata_t *pdata;
	struct sppctlgpio_chip_t *pc;
	struct gpio_chip *gchip;
	struct device_node *npi;
	struct device_node *np;
	int err = 0;
	int i = 0;

	pdata = (struct sppctl_pdata_t *)platform_data;

	np = pdev->dev.of_node;
	if (!np) {
		KERR(&pdev->dev, "invalid devicetree node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(np)) {
		KERR(&pdev->dev, "devicetree status is not available\n");
		return -ENODEV;
	}

	// print_device_tree_node(np, 0);
	for_each_child_of_node(np, npi) {
		if (of_find_property(npi, "gpio-controller", NULL)) {
			i = 1;
			break;
		}
	}

	if (of_find_property(np, "gpio-controller", NULL))
		i = 1;
	if (i == 0) {
		KERR(&pdev->dev, "is not gpio-controller\n");
		return -ENODEV;
	}

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;
	gchip = &pc->chip;

	pc->gpioxt_regs_base = pdata->gpioxt_regs_base;
	pc->first_regs_base = pdata->first_regs_base;
	pc->padctl1_regs_base = pdata->padctl1_regs_base;
	pc->padctl2_regs_base = pdata->padctl2_regs_base;
#if defined(SUPPORT_GPIO_AO_INT)
	pc->gpio_ao_int_regs_base = pdata->gpio_ao_int_regs_base;
#endif
	pdata->gpiod = pc;

#if defined(SUPPORT_GPIO_AO_INT)
	if (of_property_read_u32(np, "sunplus,ao-pin-prescale",
				 &pc->gpio_ao_int_prescale) == 0) {
		writel(pc->gpio_ao_int_prescale & 0xfffff,
		       pc->gpio_ao_int_regs_base); // PRESCALE
	}

	if (of_property_read_u32(np, "sunplus,ao-pin-debounce",
				 &pc->gpio_ao_int_debounce) == 0) {
		writel(pc->gpio_ao_int_prescale & 0xff,
		       pc->gpio_ao_int_regs_base + 4); // DEB_TIME
	}

	for (i = 0; i < 32; i++)
		pc->gpio_ao_int_pins[i] = -1;
#endif

	gchip->label = MNAME;
	gchip->parent = &pdev->dev;
	gchip->owner = THIS_MODULE;
	gchip->request = sppctl_gpio_request;
	gchip->free = sppctl_gpio_free;
	gchip->get_direction = sppctl_gpio_get_direction;
	gchip->direction_input = sppctl_gpio_direction_input;
	gchip->direction_output = sppctl_gpio_direction_output;
	gchip->get = sppctl_gpio_get_value;
	gchip->set = sppctl_gpio_set_value;
	gchip->set_config = sppctl_gpio_set_config;
	gchip->dbg_show = sppctl_gpio_dbg_show;
	gchip->base = 0; // it is main platform GPIO controller
	gchip->ngpio = GPIS_list_size;
	gchip->names = sppctlgpio_list_s;
	gchip->can_sleep = 0;
	gchip->to_irq = sppctl_gpio_to_irq;

	pdata->gpio_range.npins = gchip->ngpio;
	pdata->gpio_range.base = gchip->base;
	pdata->gpio_range.name = gchip->label;
	pdata->gpio_range.gc = gchip;

	// FIXME: can't set pc globally
	err = devm_gpiochip_add_data(&pdev->dev, gchip, pc);
	if (err < 0) {
		KERR(&pdev->dev, "gpiochip add failed\n");
		return err;
	}

	spin_lock_init(&pc->lock);

	return 0;
}

static int sppctl_gpio_del(struct platform_device *pdev, void *platform_data)
{
	//struct sppctlgpio_chip_t *cp;

	// FIXME: can't use globally now
	//cp = platform_get_drvdata(pdev);
	//if (cp == NULL)
	//	return -ENODEV;
	//gpiochip_remove(&(cp->chip));
	// FIX: remove spinlock_t ?
	return 0;
}

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

static void sppctl_sysfs_init(struct platform_device *pdev)
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

static void sppctl_sysfs_clean(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_dev_attrs); i++)
		device_remove_file(&pdev->dev, &sppctl_sysfs_dev_attrs[i]);
	for (i = 0; i < ARRAY_SIZE(sppctl_sysfs_bin_attrs); i++)
		device_remove_bin_file(&pdev->dev, &sppctl_sysfs_bin_attrs[i]);
}

__attribute((unused)) static void
print_device_tree_node(struct device_node *node, int depth)
{
	struct device_node *child;
	struct property *properties;
	char indent[255] = "";
	int i = 0;

	for (i = 0; i < depth * 3; i++)
		indent[i] = ' ';
	indent[i] = '\0';

	++depth;
	if (depth == 1) {
		pr_info("%s{ name = %s\n", indent, node->name);
		for (properties = node->properties; properties;
		     properties = properties->next)
			pr_info("%s  %s (%d)\n", indent, properties->name,
				properties->length);
		pr_info("%s}\n", indent);
	}

	for_each_child_of_node(node, child) {
		pr_info("%s{ name = %s\n", indent, child->name);
		for (properties = child->properties; properties;
		     properties = properties->next)
			pr_info("%s  %s (%d)\n", indent, properties->name,
				properties->length);
		print_device_tree_node(child, depth);
		pr_info("%s}\n", indent);
	}
}

__attribute((unused)) void sppctl_pin_set(struct sppctl_pdata_t *pdata,
					  u8 pin_selector, u8 func_selector)
{
}

u8 sppctl_fun_get(struct sppctl_pdata_t *pdata, u8 func_selector)
{
	return 0;
}

static int sppctl_pctl_resmap(struct platform_device *pdev,
			      struct sppctl_pdata_t *pdata)
{
	struct resource *rp;

	// res0
	rp = platform_get_resource_byname(pdev, IORESOURCE_MEM, "gpioxt");
	if (IS_ERR(rp)) {
		KERR(&pdev->dev, "%s get res#0 ERR\n", __func__);
		return PTR_ERR(rp);
	}
	KDBG(&pdev->dev, "mres #0:%p\n", rp);
	if (!rp)
		return -EFAULT;
	KDBG(&pdev->dev, "mapping [%pa-%pa]\n", &rp->start, &rp->end);

	pdata->gpioxt_regs_base = devm_ioremap_resource(&pdev->dev, rp);
	if (IS_ERR(pdata->gpioxt_regs_base)) {
		KERR(&pdev->dev, "%s map res#0 ERR\n", __func__);
		return PTR_ERR(pdata->gpioxt_regs_base);
	}

	// res2
	rp = platform_get_resource_byname(pdev, IORESOURCE_MEM, "first");
	if (IS_ERR(rp)) {
		KERR(&pdev->dev, "%s get res#2 ERR\n", __func__);
		return PTR_ERR(rp);
	}
	KDBG(&pdev->dev, "mres #2:%p\n", rp);
	if (!rp)
		return -EFAULT;
	KDBG(&pdev->dev, "mapping [%pa-%pa]\n", &rp->start, &rp->end);

	pdata->first_regs_base = devm_ioremap_resource(&pdev->dev, rp);
	if (IS_ERR(pdata->first_regs_base)) {
		KERR(&pdev->dev, "%s map res#2 ERR\n", __func__);
		return PTR_ERR(pdata->first_regs_base);
	}

	//res3
	rp = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pad_ctl_1");
	if (IS_ERR(rp)) {
		KERR(&pdev->dev, "%s get res#3 ERR\n", __func__);
		return PTR_ERR(rp);
	}
	KDBG(&pdev->dev, "mres #3:%p\n", rp);
	if (!rp)
		return -EFAULT;
	KDBG(&pdev->dev, "mapping [%pa-%pa]\n", &rp->start, &rp->end);

	pdata->padctl1_regs_base = devm_ioremap_resource(&pdev->dev, rp);
	if (IS_ERR(pdata->padctl1_regs_base)) {
		KERR(&pdev->dev, "%s map res#3 ERR\n", __func__);
		return PTR_ERR(pdata->padctl1_regs_base);
	}

	//res 4
	rp = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pad_ctl_2");
	if (IS_ERR(rp)) {
		KERR(&pdev->dev, "%s get res#4 ERR\n", __func__);
		return PTR_ERR(rp);
	}
	KDBG(&pdev->dev, "mres #4:%p\n", rp);
	if (!rp)
		return -EFAULT;
	KDBG(&pdev->dev, "mapping [%pa-%pa]\n", &rp->start, &rp->end);

	pdata->padctl2_regs_base = devm_ioremap_resource(&pdev->dev, rp);
	if (IS_ERR(pdata->padctl2_regs_base)) {
		KERR(&pdev->dev, "%s map res#4 ERR\n", __func__);
		return PTR_ERR(pdata->padctl2_regs_base);
	}

	// iop
	rp = platform_get_resource_byname(pdev, IORESOURCE_MEM, "moon1");
	if (IS_ERR(rp)) {
		KERR(&pdev->dev, "%s get res#I ERR\n", __func__);
		return PTR_ERR(rp);
	}
	KDBG(&pdev->dev, "mres #I:%p\n", rp);
	if (!rp)
		return -EFAULT;
	KDBG(&pdev->dev, "mapping [%pa-%pa]\n", &rp->start, &rp->end);

	pdata->moon1_regs_base = devm_ioremap_resource(&pdev->dev, rp);
	if (IS_ERR(pdata->moon1_regs_base)) {
		KERR(&pdev->dev, "%s map res#I ERR\n", __func__);
		return PTR_ERR(pdata->moon1_regs_base);
	}

#if defined(SUPPORT_GPIO_AO_INT)
	rp = platform_get_resource_byname(pdev, IORESOURCE_MEM, "gpio_ao_int");
	if (IS_ERR(rp)) {
		KERR(&pdev->dev, "%s get res#A ERR\n", __func__);
		return PTR_ERR(rp);
	}
	KDBG(&pdev->dev, "mres #A:%p\n", rp);
	if (!rp)
		return -EFAULT;
	KDBG(&pdev->dev, "mapping [%pa-%pa]\n", &rp->start, &rp->end);

	pdata->gpio_ao_int_regs_base = devm_ioremap_resource(&pdev->dev, rp);
	if (IS_ERR(pdata->gpio_ao_int_regs_base)) {
		KERR(&pdev->dev, "%s map res#A ERR\n", __func__);
		return PTR_ERR(pdata->gpio_ao_int_regs_base);
	}
#endif

	return 0;
}

static int sppctl_pinctrl_mode_select(struct pinctrl_dev *pctldev,
				      struct device_node *np_config)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	struct device_node *np;
	unsigned int value;
	const char *ms_val;
	int ret;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;
	np = of_node_get(np_config);

	ret = of_property_read_string(np_config, "sunplus,ms-dvio-group-0",
				      &ms_val);
	if (!ret) {
		if (!strcmp(ms_val, "3V0"))
			value = 0;
		else
			value = 1;

		sppctl_gpio_voltage_mode_select_set(chip, G_MX_MS_TOP_0, value);
	}

	ret = of_property_read_string(np_config, "sunplus,ms-dvio-group-1",
				      &ms_val);
	if (!ret) {
		if (!strcmp(ms_val, "3V0"))
			value = 0;
		else
			value = 1;

		sppctl_gpio_voltage_mode_select_set(chip, G_MX_MS_TOP_1, value);
	}

	ret = of_property_read_string(np_config, "sunplus,ms-dvio-ao-group-0",
				      &ms_val);
	if (!ret) {
		if (!strcmp(ms_val, "3V0"))
			value = 0;
		else
			value = 1;

		sppctl_gpio_voltage_mode_select_set(chip, AO_MX_MS_TOP_0,
						    value);
	}

	ret = of_property_read_string(np_config, "sunplus,ms-dvio-ao-group-1",
				      &ms_val);
	if (!ret) {
		if (!strcmp(ms_val, "3V0"))
			value = 0;
		else
			value = 1;

		sppctl_gpio_voltage_mode_select_set(chip, AO_MX_MS_TOP_1,
						    value);
	}

	ret = of_property_read_string(np_config, "sunplus,ms-dvio-ao-group-2",
				      &ms_val);
	if (!ret) {
		if (!strcmp(ms_val, "3V0"))
			value = 0;
		else
			value = 1;

		sppctl_gpio_voltage_mode_select_set(chip, AO_MX_MS_TOP_2,
						    value);
	}

	of_node_put(np);

	return 0;
}

static int sppctl_dnew(struct platform_device *pdev)
{
	struct sppctl_pdata_t *pdata;
	struct device_node *np;
	int ret = -ENODEV;

	np = pdev->dev.of_node;
	if (!np) {
		KERR(&pdev->dev, "Invalid dtb node\n");
		return -EINVAL;
	}
	if (!of_device_is_available(np)) {
		KERR(&pdev->dev, "dtb is not available\n");
		return -ENODEV;
	}

	// print_device_tree_node(np, 0);

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	ret = sppctl_pctl_resmap(pdev, pdata);
	if (ret != 0)
		return ret;

	// set gpio_chip
	pdev->dev.platform_data = pdata;
	sppctl_sysfs_init(pdev);

	ret = sppctl_gpio_new(pdev, pdata);
	if (ret != 0)
		return ret;

	ret = sppctl_pinctrl_init(pdev);
	if (ret != 0)
		return ret;

	pinctrl_add_gpio_range(pdata->pcdp, &pdata->gpio_range);
	pr_info(M_NAM " by " M_ORG "" M_CPR);

	//voltage mode select
	sppctl_pinctrl_mode_select(pdata->pcdp, pdev->dev.of_node);

	return 0;
}

static int sppctl_ddel(struct platform_device *pdev)
{
	struct sppctl_pdata_t *pdata =
		(struct sppctl_pdata_t *)pdev->dev.platform_data;

	sppctl_gpio_del(pdev, pdata);
	sppctl_sysfs_clean(pdev);
	sppctl_pinctrl_clean(pdev);

	return 0;
}

static const struct of_device_id sppctl_dt_ids[] = {
	{ .compatible = "sunplus,sp7350-pctl" },
	{ /* zero */ }
};

MODULE_DEVICE_TABLE(of, sppctl_dt_ids);
MODULE_ALIAS("platform:" MNAME);

static struct platform_driver sppctl_driver = {
	.driver = {
		.name           = MNAME,
		.owner          = THIS_MODULE,
		.of_match_table = of_match_ptr(sppctl_dt_ids),
	},
	.probe  = sppctl_dnew,
	.remove = sppctl_ddel,
};

static int __init sppctl_drv_reg(void)
{
	return platform_driver_register(&sppctl_driver);
}
postcore_initcall(sppctl_drv_reg);

static void __exit sppctl_drv_exit(void)
{
	platform_driver_unregister(&sppctl_driver);
}
module_exit(sppctl_drv_exit);

MODULE_AUTHOR(M_AUT);
MODULE_DESCRIPTION(M_NAM);
MODULE_LICENSE(M_LIC);
