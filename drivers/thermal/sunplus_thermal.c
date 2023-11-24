// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Sunplus Inc.
 * Author: Li-hao Kuo <lhjeff911@gmail.com>
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/thermal.h>
#include <linux/slab.h>

#define DISABLE_THERMAL		(BIT(31) | BIT(15))
#define ENABLE_THERMAL		BIT(31)

#define DISABLE_THERMAL_V2	(BIT(20) | BIT(4))
//#define ENABLE_THERMAL_V2	(BIT(20) | BIT(21))
#define ENABLE_THERMAL_V2	(BIT(20) | BIT(21) | BIT(17) | BIT(1))

#define SP_THERMAL_MASK		GENMASK(10, 0)
#define SP_TCODE_HIGH_MASK	GENMASK(15, 8)
#define SP_TCODE_LOW_MASK	GENMASK(7, 0)

#define SP_THERMAL_CTL0_REG	0x0000
#define SP_THERMAL_STS0_REG	0x0030

#define SP_THERMAL_CTL1_REG	0x0004		//F8800288
#define SP_THERMAL_CTL2_REG	0x000C		//F88002C0
#define SP_THERMAL_CTL3_REG	0x0010		//F88002C4


#define SP_THERMAL_STS0_V2_REG	0x0018

struct sunplus_thermal_compatible {
	int ver;
	int temp_base;
	int temp_otp_base;
	int temp_rate;
	int temp_otp_shift;
};

/* common data structures */
struct sp_thermal_data {
	struct thermal_zone_device *pcb_tz;
	struct platform_device *pdev;
	struct resource *res;
	struct clk *clk;
	struct reset_control *rstc;
	const struct sunplus_thermal_compatible *dev_comp;
	enum thermal_device_mode mode;
	void __iomem *regs;
	int otp_temp0;
	u32 id;
};

static int sp_thermal_init(struct sp_thermal_data *sp_data)
{
	if (sp_data->dev_comp->ver > 1)
		writel(ENABLE_THERMAL_V2, sp_data->regs + SP_THERMAL_CTL0_REG);
	else
		writel(ENABLE_THERMAL, sp_data->regs + SP_THERMAL_CTL0_REG);

	msleep(1);
	return 0;
}

static int sp_get_otp_temp_coef(struct sp_thermal_data *sp_data, struct device *dev)
{
	struct nvmem_cell *cell;
	ssize_t otp_l;
	char *otp_v;

	cell = nvmem_cell_get(dev, "therm_calib");
	if (IS_ERR(cell)){
		printk(KERN_ERR "Failed to get NVMEM cell: %ld\n", PTR_ERR(cell));
		sp_data->otp_temp0 = sp_data->dev_comp->temp_otp_base;
    		return 0;
	}

	otp_v = nvmem_cell_read(cell, &otp_l);
	nvmem_cell_put(cell);

	if (otp_l < 3){
		printk(KERN_ERR "Failed to read NVMEM cell: %ld\n", PTR_ERR(cell));
		sp_data->otp_temp0 = sp_data->dev_comp->temp_otp_base;
    		return 0;
	}

	sp_data->otp_temp0 = FIELD_PREP(SP_TCODE_LOW_MASK, otp_v[0]) |
			     FIELD_PREP(SP_TCODE_HIGH_MASK, otp_v[1]);

	if (sp_data->dev_comp->ver > 1)
		sp_data->otp_temp0 = sp_data->otp_temp0 >> sp_data->dev_comp->temp_otp_shift;

	sp_data->otp_temp0 = FIELD_GET(SP_THERMAL_MASK, sp_data->otp_temp0);

	if (!IS_ERR(otp_v))
		kfree(otp_v);

	if (sp_data->otp_temp0 == 0) 
		sp_data->otp_temp0 = sp_data->dev_comp->temp_otp_base;
	
	return 0;
}

static int sp_thermal_get_sensor_temp(void *data, int *temp)
{
	struct sp_thermal_data *sp_data = data;
	int t_code;

	if (sp_data->dev_comp->ver > 1){
		writel(0xFFFF0800, sp_data->regs +  SP_THERMAL_CTL1_REG);
		writel(0xFFFF9530, sp_data->regs +  SP_THERMAL_CTL2_REG);
		writel(0x00030003, sp_data->regs +  SP_THERMAL_CTL3_REG);
		t_code = readl(sp_data->regs + SP_THERMAL_STS0_V2_REG);
	}
	else
		t_code = readl(sp_data->regs + SP_THERMAL_STS0_REG);

	t_code = FIELD_GET(SP_THERMAL_MASK, t_code);

	*temp = ((sp_data->otp_temp0 - t_code) * 10000 / sp_data->dev_comp->temp_rate) + sp_data->dev_comp->temp_base;
	*temp *= 10;

	return 0;
}

static struct thermal_zone_of_device_ops sp_of_thermal_ops = {
	.get_temp = sp_thermal_get_sensor_temp,
};

static int sp_thermal_register_sensor(struct platform_device *pdev,
				      struct sp_thermal_data *data, int index)
{
	data->id = index;
	data->pcb_tz = devm_thermal_zone_of_sensor_register(&pdev->dev,
							    data->id,
							    data, &sp_of_thermal_ops);
	if (IS_ERR_OR_NULL(data->pcb_tz))
		return PTR_ERR(data->pcb_tz);
	return 0;
}

static int sp_thermal_probe(struct platform_device *pdev)
{
	struct sp_thermal_data *sp_data;
	struct resource *res;
	int ret;

	sp_data = devm_kzalloc(&pdev->dev, sizeof(*sp_data), GFP_KERNEL);
	if (!sp_data)
		return -ENOMEM;

	sp_data->dev_comp = of_device_get_match_data(&pdev->dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return dev_err_probe(&pdev->dev, PTR_ERR(res), "resource get fail\n");

	sp_data->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(sp_data->regs))
		return dev_err_probe(&pdev->dev, PTR_ERR(sp_data->regs), "mas_base get fail\n");

	sp_data->res = res;
	sp_data->pdev = pdev;

	if (sp_data->dev_comp->ver > 1) {
		sp_data->clk = devm_clk_get(&pdev->dev, NULL);
		if (IS_ERR(sp_data->clk))
			return dev_err_probe(&pdev->dev, PTR_ERR(sp_data->clk), "clk get fail\n");

		sp_data->rstc = devm_reset_control_get_exclusive(&pdev->dev, NULL);
		if (IS_ERR(sp_data->rstc))
			return dev_err_probe(&pdev->dev, PTR_ERR(sp_data->rstc), "err get reset\n");

		ret = reset_control_deassert(sp_data->rstc);
		if (ret)
			return dev_err_probe(&pdev->dev, ret, "failed to deassert reset\n");

		ret = clk_prepare_enable(sp_data->clk);
		if (ret)
			return dev_err_probe(&pdev->dev, ret, "failed to enable clk\n");
	}

	platform_set_drvdata(pdev, sp_data);
	ret = sp_thermal_init(sp_data);
	ret = sp_get_otp_temp_coef(sp_data, &pdev->dev);
	ret = sp_thermal_register_sensor(pdev, sp_data, 0);

	return ret;
}

static int sp_thermal_remove(struct platform_device *pdev)
{
	struct sp_thermal_data *sp_data = platform_get_drvdata(pdev);

	if (sp_data->dev_comp->ver > 1) {
		clk_disable_unprepare(sp_data->clk);
		reset_control_assert(sp_data->rstc);
	}
	return 0;
}

static int __maybe_unused sp_thermal_suspend(struct device *dev)
{
	struct sp_thermal_data *sp_data = dev_get_drvdata(dev);

	if (sp_data->dev_comp->ver > 1) {
		clk_disable_unprepare(sp_data->clk);        //enable clken and disable gclken
		reset_control_assert(sp_data->rstc);
	}
	return 0;
}

static int __maybe_unused sp_thermal_resume(struct device *dev)
{
	struct sp_thermal_data *sp_data = dev_get_drvdata(dev);

	if (sp_data->dev_comp->ver > 1) {
		reset_control_deassert(sp_data->rstc);
		clk_prepare_enable(sp_data->clk);
	}
	msleep(1);
	return sp_thermal_init(sp_data);
}

static int sp_thermal_runtime_suspend(struct device *dev)
{
	struct sp_thermal_data *sp_data = dev_get_drvdata(dev);

	if (sp_data->dev_comp->ver > 1) {
		clk_disable_unprepare(sp_data->clk);        //enable clken and disable gclken
		reset_control_assert(sp_data->rstc);
	}
	return 0;

}

static int sp_thermal_runtime_resume(struct device *dev)
{
	struct sp_thermal_data *sp_data = dev_get_drvdata(dev);

	if (sp_data->dev_comp->ver > 1) {
		reset_control_deassert(sp_data->rstc);   //release reset
		clk_prepare_enable(sp_data->clk);        //enable clken and disable gclken
	}
	msleep(1);
	return sp_thermal_init(sp_data);
}

static const struct dev_pm_ops sp_thermal_pm_ops = {
	SET_RUNTIME_PM_OPS(sp_thermal_runtime_suspend,
			   sp_thermal_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(sp_thermal_suspend, sp_thermal_resume)

};

#define sp_pm_ops  (&sp_thermal_pm_ops)

static const struct sunplus_thermal_compatible sp7350_compat = {
	.ver = 2,
	.temp_base = 3500,
	.temp_otp_base = 1496,
	.temp_rate = 585,
	.temp_otp_shift = 2,
};

static const struct of_device_id of_sp_thermal_ids[] = {
	{ .compatible = "sunplus,sp7350-thermal", (void *)&sp7350_compat },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_sp_thermal_ids);

static struct platform_driver sunplus_thermal_driver = {
	.probe	= sp_thermal_probe,
	.remove	= sp_thermal_remove,
	.driver	= {
		.name	= "sunplus-thermal",
		.of_match_table = of_match_ptr(of_sp_thermal_ids),
		.pm     = sp_pm_ops,
		},
};
module_platform_driver(sunplus_thermal_driver);

MODULE_AUTHOR("Li-hao Kuo <lhjeff911@gmail.com>");
MODULE_DESCRIPTION("Thermal driver for sunplus SoC");
MODULE_LICENSE("GPL");
