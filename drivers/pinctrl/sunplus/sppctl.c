// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/io.h>

#include "sppctl.h"
#include "../core.h"

void print_device_tree_node(struct device_node *node, int depth)
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

void sppctl_gmx_set(struct sppctl_pdata_t *pdata, u8 reg_offset, u8 bit_offset,
		    u8 bit_nums, u8 bit_value)
{
	struct sppctl_reg_t x;
	u32 *r;

	x.m = (~(~0 << bit_nums)) << bit_offset;
	x.v = ((uint16_t)bit_value) << bit_offset;

	if (pdata->debug > 1)
		KDBG(pdata->pcdp->dev,
		     "%s(reg_off[0x%X],bit_off[0x%X],bit_nums[0x%X],bit_val[0x%X]) mask_bit:0x%X control_bit:0x%X\n",
		     __func__, reg_offset, bit_offset, bit_nums, bit_value, x.m,
		     x.v);
	r = (uint32_t *)&x;
	writel(*r, pdata->moon1_regs_base + (reg_offset << 2));
}

u8 sppctl_gmx_get(struct sppctl_pdata_t *pdata, u8 reg_offset, u8 bit_offset,
		  u8 bit_nums)
{
	struct sppctl_reg_t *x;
	u8 bit_value;
	u32 r;

	r = readl(pdata->moon1_regs_base + (reg_offset << 2));

	x = (struct sppctl_reg_t *)&r;
	bit_value = (x->v >> bit_offset) & (~(~0 << bit_nums));

	if (pdata->debug > 1)
		KDBG(pdata->pcdp->dev,
		     "%s(reg_off[0x%X],bit_off[0x%X],bit_nums[0x%X]) control_bit:0x%X bit_val:0x%X\n",
		     __func__, reg_offset, bit_offset, bit_nums, x->v,
		     bit_value);

	return bit_value;
}

void sppctl_pin_set(struct sppctl_pdata_t *pdata, u8 pin_selector,
		    u8 func_selector)
{
}

u8 sppctl_fun_get(struct sppctl_pdata_t *pdata, u8 func_selector)
{
	return 0;
}

int sppctl_pctl_resmap(struct platform_device *pdev,
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
