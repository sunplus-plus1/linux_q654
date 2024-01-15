// SPDX-License-Identifier: GPL-2.0+

#include <linux/bits.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

#define STI8070X_REG_VSEL0 0x00
#define STI8070X_REG_VSEL1 0x01
#define STI8070X_REG_CTRL 0x02
#define STI8070X_REG_ID1 0x03
#define STI8070X_REG_ID2 0x04
#define STI8070X_REG_PGOOD 0x05

#define STI8070X_MANUFACTURER_ID1 0x88
#define STI8070X_MANUFACTURER_ID2 0x01

#define STI8070X_NUM_VOLTS 64
#define STI8070X_MIN_UV 712500
#define STI8070X_STEP_UV 12500
#define STI8070X_MIN_SEL 0

#define STI8070X_ENABLE_MASK BIT(7)
#define STI8070X_ENABLE_VALUE 1
#define STI8070X_DISABLE_VALUE 0
#define STI8070X_ENABLE_TIMEUS 800
#define STI8070X_POLL_ENABLE_TIMEUS 100
#define STI8070X_ENABLE_IS_INVERTED 0

#define STI8070X_VSEL_MASK GENMASK(5, 0)
#define STI8070X_PGOOD_MASK BIT(7)

#define STI8070X_OFF_DELAY_TIMEUS 30

#define STI8070X_DISCHARGE_MASK BIT(7)
#define STI8070X_DISCHARGE_ON 1
#define STI8070X_DISCHARGE_OFF 0

#define STI8070X_PFM_MODE 0
#define STI8070X_FPWM_MODE 1
#define STI8070X_MODE_MASK BIT(6)
#define STI8070X_RAMP_MASK GENMASK(6, 4)

struct sti8070x_priv {
	struct device *dev;
	struct regmap *regmap;
	struct regulator_desc desc;
	unsigned int vsel_pin;
	struct gpio_desc *ena_gpiod;
};

int sti8070x_is_enabled(struct regulator_dev *rdev)
{
	struct regmap *regmap = rdev_get_regmap(rdev);
	unsigned int regval;
	int ret;

	ret = regulator_is_enabled_regmap(rdev);
	if (!ret)
		return ret;

	ret = regmap_read(regmap, STI8070X_REG_PGOOD, &regval);
	if (ret)
		return -EINVAL;

	if (regval & STI8070X_PGOOD_MASK)
		return 1;
	else
		return 0;
}

static int sti8070x_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	struct sti8070x_priv *priv =
		(struct sti8070x_priv *)rdev_get_drvdata(rdev);
	struct regmap *regmap = rdev_get_regmap(rdev);
	unsigned int mode_val;
	unsigned int reg;

	switch (mode) {
	case REGULATOR_MODE_NORMAL:
		mode_val = STI8070X_PFM_MODE;
		break;
	case REGULATOR_MODE_FAST:
		mode_val = STI8070X_FPWM_MODE;
		break;
	default:
		return -EINVAL;
	}

	if (priv->vsel_pin == 0)
		reg = STI8070X_REG_VSEL0;
	else
		reg = STI8070X_REG_VSEL1;

	return regmap_update_bits(regmap, reg, STI8070X_MODE_MASK, mode_val);
}

static unsigned int sti8070x_get_mode(struct regulator_dev *rdev)
{
	struct sti8070x_priv *priv =
		(struct sti8070x_priv *)rdev_get_drvdata(rdev);
	struct regmap *regmap = rdev_get_regmap(rdev);
	unsigned int regval;
	unsigned int reg;
	int ret;

	if (priv->vsel_pin == 0)
		reg = STI8070X_REG_VSEL0;
	else
		reg = STI8070X_REG_VSEL0;

	ret = regmap_read(regmap, reg, &regval);
	if (ret)
		return REGULATOR_MODE_INVALID;

	if (regval & STI8070X_MODE_MASK)
		return REGULATOR_MODE_FAST;

	return REGULATOR_MODE_NORMAL;
}

static const struct regulator_ops sti8070x_regulator_ops = {
	.list_voltage = regulator_list_voltage_linear,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = sti8070x_is_enabled,
	.set_active_discharge = regulator_set_active_discharge_regmap,
	.set_mode = sti8070x_set_mode,
	.get_mode = sti8070x_get_mode,
	.set_ramp_delay = regulator_set_ramp_delay_regmap,
};

static unsigned int sti8070x_of_map_mode(unsigned int mode)
{
	switch (mode) {
	case STI8070X_FPWM_MODE:
		return REGULATOR_MODE_FAST;
	case STI8070X_PFM_MODE:
		return REGULATOR_MODE_NORMAL;
	default:
		return REGULATOR_MODE_INVALID;
	}
}

static const unsigned int sti8070x_ramp_delay_table[] = { 66666, 33333, 16666,
							  8333,	 4166,	2083,
							  1041,	 520 };

static int sti8070x_regulator_register(struct sti8070x_priv *priv)
{
	struct device_node *np = priv->dev->of_node;
	struct regulator_desc *reg_desc = &priv->desc;
	struct regulator_config reg_cfg;
	struct regulator_dev *rdev;
	int ret;

	reg_desc->name = "sti8070x-buck";
	reg_desc->type = REGULATOR_VOLTAGE;
	reg_desc->owner = THIS_MODULE;
	reg_desc->ops = &sti8070x_regulator_ops;
	reg_desc->n_voltages = STI8070X_NUM_VOLTS;
	reg_desc->min_uV = STI8070X_MIN_UV;
	reg_desc->uV_step = STI8070X_STEP_UV;
	reg_desc->linear_min_sel = STI8070X_MIN_SEL;

	if (priv->vsel_pin == 0)
		reg_desc->vsel_reg = STI8070X_REG_VSEL0;
	else
		reg_desc->vsel_reg = STI8070X_REG_VSEL1;

	reg_desc->vsel_mask = STI8070X_VSEL_MASK;

	if (priv->vsel_pin == 0)
		reg_desc->enable_reg = STI8070X_REG_VSEL0;
	else
		reg_desc->enable_reg = STI8070X_REG_VSEL1;

	reg_desc->enable_mask = STI8070X_ENABLE_MASK;
	reg_desc->enable_val = STI8070X_ENABLE_VALUE;
	reg_desc->disable_val = STI8070X_DISABLE_VALUE;
	reg_desc->enable_is_inverted = STI8070X_ENABLE_IS_INVERTED;
	reg_desc->enable_time = STI8070X_ENABLE_TIMEUS;
	reg_desc->poll_enabled_time = STI8070X_POLL_ENABLE_TIMEUS;

	reg_desc->ramp_delay_table = sti8070x_ramp_delay_table;
	reg_desc->n_ramp_values = ARRAY_SIZE(sti8070x_ramp_delay_table);
	reg_desc->ramp_reg = STI8070X_REG_CTRL;
	reg_desc->ramp_mask = STI8070X_RAMP_MASK;
	reg_desc->active_discharge_reg = STI8070X_REG_CTRL;
	reg_desc->active_discharge_mask = STI8070X_DISCHARGE_MASK;
	reg_desc->active_discharge_on = STI8070X_DISCHARGE_ON;
	reg_desc->active_discharge_off = STI8070X_DISCHARGE_OFF;

	reg_desc->off_on_delay = STI8070X_OFF_DELAY_TIMEUS;
	reg_desc->of_map_mode = sti8070x_of_map_mode;

	memset(&reg_cfg, 0, sizeof(reg_cfg));
	reg_cfg.dev = priv->dev;
	reg_cfg.driver_data = priv;
	reg_cfg.of_node = np;
	reg_cfg.init_data = of_get_regulator_init_data(priv->dev, np, reg_desc);
	reg_cfg.regmap = priv->regmap;
	reg_cfg.ena_gpiod = priv->ena_gpiod;

	rdev = devm_regulator_register(priv->dev, reg_desc, &reg_cfg);
	if (IS_ERR(rdev)) {
		ret = PTR_ERR(rdev);
		dev_err(priv->dev, "Failed to register regulator (%d)\n", ret);
		return ret;
	}

	return 0;
}

static int sti8070x_init_device_property(struct sti8070x_priv *priv)
{
	struct gpio_desc *ena_gpiod;
	int ret;

	ret = device_property_read_u32(priv->dev, "tmi,sel-pin-status",
				       &priv->vsel_pin);
	if (ret)
		priv->vsel_pin = 0;

	ena_gpiod = gpiod_get_optional(priv->dev, NULL, GPIOD_ASIS);
	if (!IS_ERR(ena_gpiod))
		priv->ena_gpiod = ena_gpiod;

	return 0;
}

static int sti8070x_manufacturer_check(struct sti8070x_priv *priv)
{
	unsigned int id1; /* bit[7:5] vendor; bit[4] reserved; bit[3:0] DIE_ID */
	unsigned int id2; /* bit[7:4] reserved; bit[3:0] DIE_REV */
	int ret;

	ret = regmap_read(priv->regmap, STI8070X_REG_ID1, &id1);
	if (ret)
		return ret;

	if (id1 != STI8070X_MANUFACTURER_ID1) {
		dev_err(priv->dev, "ID1 info not correct (%d)\n", id1);
		return -EINVAL;
	}

	ret = regmap_read(priv->regmap, STI8070X_REG_ID2, &id2);
	if (ret)
		return ret;

	if (id2 != STI8070X_MANUFACTURER_ID2) {
		dev_err(priv->dev, "ID2 info not correct (%d)\n", id2);
		return -EINVAL;
	}

	return 0;
}

static bool sti8070x_is_accessible_reg(struct device *dev, unsigned int reg)
{
	if (reg <= STI8070X_REG_PGOOD)
		return true;

	return false;
}

static const struct regmap_config sti8070x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = STI8070X_REG_PGOOD,
	.readable_reg = sti8070x_is_accessible_reg,
	.writeable_reg = sti8070x_is_accessible_reg,
};

static int sti8070x_probe(struct i2c_client *i2c)
{
	struct sti8070x_priv *priv;
	int ret;

	priv = devm_kzalloc(&i2c->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &i2c->dev;
	i2c_set_clientdata(i2c, priv);

	priv->regmap = devm_regmap_init_i2c(i2c, &sti8070x_regmap_config);
	if (IS_ERR(priv->regmap)) {
		ret = PTR_ERR(priv->regmap);
		dev_err(&i2c->dev, "Failed to allocate regmap (%d)\n", ret);
		return ret;
	}

	ret = sti8070x_manufacturer_check(priv);
	if (ret) {
		dev_err(&i2c->dev, "Failed to check device (%d)\n", ret);
		return ret;
	}

	ret = sti8070x_init_device_property(priv);
	if (ret) {
		dev_err(&i2c->dev, "Failed to init device (%d)\n", ret);
		return ret;
	}

	return sti8070x_regulator_register(priv);
}

static const struct of_device_id __maybe_unused
	sti8070x_device_table[] = { { .compatible = "tmi,sti8070x" }, {} };
MODULE_DEVICE_TABLE(of, sti8070x_device_table);

static struct i2c_driver sti8070x_driver = {
	.driver = {
		.name = "sti8070x",
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
		.of_match_table = of_match_ptr(sti8070x_device_table),
	},
	.probe_new = sti8070x_probe,
};
module_i2c_driver(sti8070x_driver);

MODULE_AUTHOR("YuBo Leng <yb.leng@sunmedia.com.cn>");
MODULE_DESCRIPTION("TOLL Microelectronic(TMI) STI8070x series Regulator");
MODULE_LICENSE("GPL v2");
