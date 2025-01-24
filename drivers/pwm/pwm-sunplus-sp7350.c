// SPDX-License-Identifier: GPL-2.0
/*
 * PWM device driver for SUNPLUS SP7350 SoC
 *
 * Author: Hammer Hsieh <hammerh0314@gmail.com>
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#define SP7350_PWM_MODE0		0x000
#define SP7350_PWM_MODE0_PWM_DU_MASK GENMASK(23, 20)
#define SP7350_PWM_MODE0_PWMEN(ch)	BIT(ch)
#define SP7350_PWM_MODE0_BYPASS(ch)	BIT(8 + (ch))
#define SP7350_PWM_MODE1		0x080
#define SP7350_PWM_MODE1_POL(ch)	BIT(8 + (ch))
#define SP7350_PWM_MODE1_POL_MASK	GENMASK(15, 8)
#define SP7350_PWM_POL_NORMAL		0
#define SP7350_PWM_POL_INVERTED		1
#define SP7350_PWM_FREQ(ch)		(0x044 + 4 * (ch))
#define SP7350_PWM_FREQ_MAX		GENMASK(17, 0)
#define SP7350_PWM_DUTY(ch)		(0x004 + 4 * (ch))
#define SP7350_PWM_DUTY_DD_SEL(ch)	FIELD_PREP(GENMASK(17, 16), ch)
#define SP7350_PWM_DUTY_MAX		GENMASK(11, 0)
#define SP7350_PWM_DUTY_MASK		SP7350_PWM_DUTY_MAX
#define SP7350_PWM_FREQ_SCALER		4096
#define SP7350_PWM_NUM			4

struct sunplus_pwm {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *clk;
};

static inline struct sunplus_pwm *to_sunplus_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct sunplus_pwm, chip);
}

static int sunplus_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			     const struct pwm_state *state)
{
	struct sunplus_pwm *priv = to_sunplus_pwm(chip);
	u32 dd_freq, duty, mode0, mode1;
	u64 clk_rate;

	/*
	 * apply pwm polarity setting
	 */
	printk("sunplus_pwm_apply\n");
	mode1 = readl(priv->base + SP7350_PWM_MODE1);
	if (pwm->state.polarity == PWM_POLARITY_NORMAL)
		mode1 &= ~SP7350_PWM_MODE1_POL(pwm->hwpwm);
	else
		mode1 |= SP7350_PWM_MODE1_POL(pwm->hwpwm);

	writel(mode1, priv->base + SP7350_PWM_MODE1);

	if (!state->enabled) {
		/* disable pwm channel output */
		mode0 = readl(priv->base + SP7350_PWM_MODE0);
		mode0 &= ~SP7350_PWM_MODE0_PWM_DU_MASK;
		mode0 &= ~SP7350_PWM_MODE0_PWMEN(pwm->hwpwm);
		writel(mode0, priv->base + SP7350_PWM_MODE0);
		return 0;
	}

	clk_rate = clk_get_rate(priv->clk);

	/*
	 * The following calculations might overflow if clk is bigger
	 * than 256 GHz. In practise it's 200MHz, so this limitation
	 * is only theoretic.
	 */
	if (clk_rate > (u64)SP7350_PWM_FREQ_SCALER * NSEC_PER_SEC)
		return -EINVAL;

	/*
	 * With clk_rate limited above we have dd_freq <= state->period,
	 * so this cannot overflow.
	 */
	dd_freq = mul_u64_u64_div_u64(clk_rate, state->period, (u64)SP7350_PWM_FREQ_SCALER
				* NSEC_PER_SEC);

	if (dd_freq == 0)
		return -EINVAL;

	if (dd_freq > SP7350_PWM_FREQ_MAX)
		dd_freq = SP7350_PWM_FREQ_MAX;

	writel(dd_freq, priv->base + SP7350_PWM_FREQ(pwm->hwpwm));

	/* cal and set pwm duty */
	mode0 = readl(priv->base + SP7350_PWM_MODE0);
	mode0 |= SP7350_PWM_MODE0_PWMEN(pwm->hwpwm);
	if (state->duty_cycle == state->period) {
		/* PWM channel output = high */
		mode0 |= SP7350_PWM_MODE0_BYPASS(pwm->hwpwm);
		duty = SP7350_PWM_DUTY_DD_SEL(pwm->hwpwm) | SP7350_PWM_DUTY_MAX;
	} else {
		mode0 &= ~SP7350_PWM_MODE0_BYPASS(pwm->hwpwm);
		/*
		 * duty_ns <= period_ns 27 bits, clk_rate 28 bits, won't overflow.
		 */
		duty = mul_u64_u64_div_u64(state->duty_cycle, clk_rate,
					   (u64)dd_freq * NSEC_PER_SEC);
		duty = SP7350_PWM_DUTY_DD_SEL(pwm->hwpwm) | duty;
	}
	writel(duty, priv->base + SP7350_PWM_DUTY(pwm->hwpwm));
	writel(mode0, priv->base + SP7350_PWM_MODE0);

	return 0;
}

static int sunplus_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
				 struct pwm_state *state)
{
	struct sunplus_pwm *priv = to_sunplus_pwm(chip);
	u32 mode0, mode1, mode1_pol, dd_freq, duty;
	u64 clk_rate;

	mode0 = readl(priv->base + SP7350_PWM_MODE0);

	if (mode0 & BIT(pwm->hwpwm)) {
		clk_rate = clk_get_rate(priv->clk);
		dd_freq = readl(priv->base + SP7350_PWM_FREQ(pwm->hwpwm));
		duty = readl(priv->base + SP7350_PWM_DUTY(pwm->hwpwm));
		duty = FIELD_GET(SP7350_PWM_DUTY_MASK, duty);
		/*
		 * dd_freq 18 bits, SP7350_PWM_FREQ_SCALER 12 bits
		 * NSEC_PER_SEC 30 bits, won't overflow.
		 */
		state->period = DIV64_U64_ROUND_UP((u64)dd_freq * (u64)SP7350_PWM_FREQ_SCALER
						* NSEC_PER_SEC, clk_rate);
		/*
		 * dd_freq 18 bits, duty 12 bits, NSEC_PER_SEC 30 bits, won't overflow.
		 */
		state->duty_cycle = DIV64_U64_ROUND_UP((u64)dd_freq * (u64)duty * NSEC_PER_SEC,
						       clk_rate);
		state->enabled = true;
	} else {
		state->enabled = false;
	}

	mode1 = readl(priv->base + SP7350_PWM_MODE1);
	mode1_pol = (FIELD_GET(SP7350_PWM_MODE1_POL_MASK, mode1) >> (pwm->hwpwm)) & 0x01;

	if (mode1_pol == SP7350_PWM_POL_NORMAL)
		state->polarity = PWM_POLARITY_NORMAL;
	else
		state->polarity = PWM_POLARITY_INVERSED;

	return 0;
}

static const struct pwm_ops sunplus_pwm_ops = {
	.apply = sunplus_pwm_apply,
	.get_state = sunplus_pwm_get_state,
	.owner = THIS_MODULE,
};

static void sunplus_pwm_clk_release(void *data)
{
	struct clk *clk = data;

	clk_disable_unprepare(clk);
}

static int sunplus_pwm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunplus_pwm *priv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return dev_err_probe(dev, PTR_ERR(priv->clk),
				     "get pwm clock failed\n");

	ret = clk_prepare_enable(priv->clk);
	if (ret < 0) {
		dev_err(dev, "failed to enable clock: %d\n", ret);
		return ret;
	}

	ret = devm_add_action_or_reset(dev, sunplus_pwm_clk_release, priv->clk);
	if (ret < 0) {
		dev_err(dev, "failed to release clock: %d\n", ret);
		return ret;
	}

	priv->chip.dev = dev;
	priv->chip.ops = &sunplus_pwm_ops;
	priv->chip.npwm = SP7350_PWM_NUM;

	ret = devm_pwmchip_add(dev, &priv->chip);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Cannot register sunplus PWM\n");

	return 0;
}

static const struct of_device_id sunplus_pwm_of_match[] = {
	{ .compatible = "sunplus,sp7350-pwm", },
	{}
};
MODULE_DEVICE_TABLE(of, sunplus_pwm_of_match);

static struct platform_driver sunplus_pwm_driver = {
	.probe		= sunplus_pwm_probe,
	.driver		= {
		.name	= "sunplus-pwm",
		.of_match_table = sunplus_pwm_of_match,
	},
};
module_platform_driver(sunplus_pwm_driver);

MODULE_DESCRIPTION("Sunplus SoC PWM Driver");
MODULE_AUTHOR("Hammer Hsieh <hammerh0314@gmail.com>");
MODULE_LICENSE("GPL");
