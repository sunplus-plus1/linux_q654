// SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
/*
 * SP7021/SP7350 reset driver
 *
 * Copyright (C) Sunplus Technology Co., Ltd.
 *       All rights reserved.
 */

#include <linux/io.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/reboot.h>

/* HIWORD_MASK_REG BITS */
#define BITS_PER_HWM_REG	16

struct sp_reset {
	struct reset_controller_dev rcdev;
	struct notifier_block notifier;
	void __iomem *base;
};

static inline struct sp_reset *to_sp_reset(struct reset_controller_dev *rcdev)
{
	return container_of(rcdev, struct sp_reset, rcdev);
}

static int sp_reset_update(struct reset_controller_dev *rcdev,
			   unsigned long id, bool assert)
{
	struct sp_reset *reset = to_sp_reset(rcdev);
	int index = id / BITS_PER_HWM_REG;
	int shift = id % BITS_PER_HWM_REG;
	u32 val;

	val = (1 << (16 + shift)) | (assert << shift);
	writel(val, reset->base + (index * 4));

	return 0;
}

static int sp_reset_assert(struct reset_controller_dev *rcdev,
			   unsigned long id)
{
	return sp_reset_update(rcdev, id, true);
}

static int sp_reset_deassert(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	return sp_reset_update(rcdev, id, false);
}

static int sp_reset_status(struct reset_controller_dev *rcdev,
			   unsigned long id)
{
	struct sp_reset *reset = to_sp_reset(rcdev);
	int index = id / BITS_PER_HWM_REG;
	int shift = id % BITS_PER_HWM_REG;
	u32 reg;

	reg = readl(reset->base + (index * 4));

	return !!(reg & BIT(shift));
}

static const struct reset_control_ops sp_reset_ops = {
	.assert   = sp_reset_assert,
	.deassert = sp_reset_deassert,
	.status   = sp_reset_status,
};

static int sp_restart(struct notifier_block *nb, unsigned long mode,
		      void *cmd)
{
	struct sp_reset *reset = container_of(nb, struct sp_reset, notifier);

	sp_reset_assert(&reset->rcdev, 0);
	sp_reset_deassert(&reset->rcdev, 0);

	return NOTIFY_DONE;
}

static int sp_reset_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp_reset *reset;
	struct resource *res;
	int ret;

	reset = devm_kzalloc(dev, sizeof(*reset), GFP_KERNEL);
	if (!reset)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reset->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(reset->base))
		return PTR_ERR(reset->base);

	reset->rcdev.ops = &sp_reset_ops;
	reset->rcdev.owner = THIS_MODULE;
	reset->rcdev.of_node = dev->of_node;
	reset->rcdev.nr_resets = resource_size(res) / 4 * BITS_PER_HWM_REG;

	ret = devm_reset_controller_register(dev, &reset->rcdev);
	if (ret)
		return ret;

	reset->notifier.notifier_call = sp_restart;
	reset->notifier.priority = 192;

	return register_restart_handler(&reset->notifier);
}

static const struct of_device_id sp_reset_dt_ids[] = {
	{.compatible = "sunplus,sp7021-reset",},
	{.compatible = "sunplus,sp7350-reset",},
	{ /* sentinel */ },
};

static struct platform_driver sp_reset_driver = {
	.probe = sp_reset_probe,
	.driver = {
		.name			= "sunplus-reset",
		.of_match_table		= sp_reset_dt_ids,
		.suppress_bind_attrs	= true,
	},
};

static int __init sp_reset_init(void)
{
	return platform_driver_register(&sp_reset_driver);
}
arch_initcall(sp_reset_init);
