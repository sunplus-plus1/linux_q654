// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_opp.h>
#include <linux/regulator/driver.h>
#include <linux/gpio/consumer.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <dt-bindings/regulator/sunplus,sp7350-iso.h>

struct reg_iso_priv {
	struct regulator_desc desc;
	struct regulator_dev *dev;
	unsigned int enable_counter;
	u32 iso;
	void __iomem *reg;
	spinlock_t spin_lock;
};

static int reg_iso_remove(struct regulator_dev *rdev)
{
	struct reg_iso_priv *priv = rdev_get_drvdata(rdev);
	unsigned long flags;
	u32 value;
	u32 iso;

	if (!priv)
		return -EINVAL;

	iso = priv->iso;

	spin_lock_irqsave(&priv->spin_lock, flags);
	value = readl(priv->reg);
	value &= ~iso;
	writel(value, priv->reg);
	spin_unlock_irqrestore(&priv->spin_lock, flags);

	priv->enable_counter++;

	return 0;
}

static int reg_iso_apply(struct regulator_dev *rdev)
{
	struct reg_iso_priv *priv = rdev_get_drvdata(rdev);
	unsigned long flags;
	u32 value;
	u32 iso;

	if (!priv)
		return -EINVAL;

	iso = priv->iso;

	value = readl(priv->reg);
	value |= iso;

	spin_lock_irqsave(&priv->spin_lock, flags);
	writel(value, priv->reg);
	spin_unlock_irqrestore(&priv->spin_lock, flags);

	priv->enable_counter--;

	return 0;
}

static int reg_is_enabled(struct regulator_dev *rdev)
{
	struct reg_iso_priv *priv = rdev_get_drvdata(rdev);

	return priv->enable_counter > 0;
}

static const struct regulator_ops iso_ops = {
	.enable = reg_iso_remove,
	.disable = reg_iso_apply,
	.is_enabled = reg_is_enabled,
};

static int reg_iso_probe(struct platform_device *pdev)
{
	struct regulator_init_data init_data;
	struct reg_iso_priv *drvdata;
	struct regulator_config cfg = { };
	struct resource *res;
	int ret;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(struct reg_iso_priv),
			       GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	spin_lock_init(&drvdata->spin_lock);

	if (pdev->dev.of_node) {
		drvdata->desc.name = of_get_property(pdev->dev.of_node, "regulator-name", NULL);
		if (!drvdata->desc.name) {
			dev_err(&pdev->dev, "No regulator-nam from DT\n");
			return -EINVAL;
		}

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			dev_err(&pdev->dev, "no reg from DT\n");
			return -ENODEV;
		}

		drvdata->reg = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(drvdata->reg)) {
			dev_err(&pdev->dev, "ioremap reg error\n");
			return -EINVAL;
		}

		ret = device_property_read_u32(&pdev->dev, "sunplus,iso-selector",
					       &drvdata->iso);
		if (ret) {
			dev_err(&pdev->dev, "No sunplus,iso-selector from DT\n");
			return ret;
		}
		dev_err(&pdev->dev, "sunplus,iso-selector:%d\n", drvdata->iso);
	}

	if (!drvdata->desc.name)
		drvdata->desc.name = "sp7350-iso";

	init_data.constraints.valid_ops_mask |= REGULATOR_CHANGE_STATUS;

	drvdata->desc.type = REGULATOR_VOLTAGE;
	drvdata->desc.owner = THIS_MODULE;
	drvdata->desc.ops = &iso_ops;
	drvdata->desc.n_voltages = 0;

	cfg.dev = &pdev->dev;
	cfg.init_data = &init_data;
	cfg.driver_data = drvdata;
	cfg.of_node = pdev->dev.of_node;

	drvdata->dev = devm_regulator_register(&pdev->dev, &drvdata->desc,
					       &cfg);
	if (IS_ERR(drvdata->dev)) {
		ret = dev_err_probe(&pdev->dev, PTR_ERR(drvdata->dev),
				    "Failed to register regulator: %ld\n",
				    PTR_ERR(drvdata->dev));
		return ret;
	}

	platform_set_drvdata(pdev, drvdata);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id of_match[] = {
	{
		.compatible = "sunplus,sp7350-regulator-iso",
	},
	{
	},
};
MODULE_DEVICE_TABLE(of, of_match);
#endif

static struct platform_driver regulator_iso_driver = {
	.probe		= reg_iso_probe,
	.driver		= {
		.name		= "iso",
		.probe_type	= PROBE_PREFER_ASYNCHRONOUS,
		.of_match_table = of_match_ptr(of_match),
	},
};

static int __init regulator_iso_init(void)
{
	return platform_driver_register(&regulator_iso_driver);
}
subsys_initcall(regulator_iso_init);

static void __exit regulator_iso_exit(void)
{
	platform_driver_unregister(&regulator_iso_driver);
}
module_exit(regulator_iso_exit);

MODULE_AUTHOR("YuBo Leng <yb.leng@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus Power Isolation Regulator");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:reg-iso");
