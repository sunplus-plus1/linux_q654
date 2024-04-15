// SPDX-License-Identifier: GPL-2.0-only
/*
 * STC Driver
 *
 */

#include <linux/cdev.h>
#include <linux/compat.h>
#include <linux/clk.h>
#include <linux/err.h>		/* For IS_ERR macros */
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/idr.h>		/* For ida_* macros */
#include <linux/interrupt.h>
#include <linux/kernel.h>	/* For printk/panic/... */
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>	/* For mutexes */
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>		/* For memory functions */
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm-generic/siginfo.h>
#include <linux/init.h>

#define TRACE(s) printk("### %s:%d %s\n", __FUNCTION__, __LINE__, s)

//#define CONFIG_SP_STC_DEBUG
#ifdef CONFIG_SP_STC_DEBUG
	#define TAG "STC: "
	#define sp_stc_dbg(fmt, ...) printk(KERN_INFO TAG fmt, ##__VA_ARGS__);
#else
	#define sp_stc_dbg(fmt, ...) do {} while (0)
#endif

/* STC register offset */
#define STC_PRESCALER		0x0C
#define STC_SRC_MSK		BIT(15)		/* 0:sys_clk  1:ext_clk */

#define STC_SRC(x)		((x) << 15)
#define	STC_SRC_SYS		0
#define STC_SRC_EXT		1

#define STC_EXTCFG		0x10
#define STC_EXT_EN		BIT(9)		/* enable ext clk prescaler */
#define STC_EXT_SEL_MSK		BIT(8)		/* 0:25MHz  1:32KHz */
#define STC_EXT_DIV_MSK		GENMASK(7, 0)

#define STC_EXT_SEL(x)		((x) << 8)
#define	STC_EXT_25M		0
#define STC_EXT_32K		1


struct sp_stc_group {
	void __iomem *base;
	struct device *dev;

	struct clk *sys_clk;	/* used if src select sysclk  		*/
	u32 src_freq;		/* frequency of system clock(bus clock)	*/
	u32 stc_freq;		/* work frequency of STC 		*/

	u8 src;			/* clock source of STC 			*/
	u8 stc_id;
};

static void of_stc_register_devices(struct sp_stc_group *stc_group)
{
	struct platform_device *pdev;
	struct device_node *node;
	struct device_node *stc_group_node = stc_group->dev->of_node;
	int child_idx = 0;
	char child_name[32];

	/* Only register child devices if the adapter has a node pointer set */
	if (!stc_group_node)
		return;

	dev_dbg(stc_group->dev, "of_stc_group: walking child nodes\n");

	of_node_get(stc_group_node);

	for_each_available_child_of_node(stc_group_node, node) {
		snprintf(child_name, sizeof(child_name), "%s-timer%d",
                         dev_name(stc_group->dev), child_idx++);
		//printk("child_name %s\n", child_name);

		pdev = of_platform_device_create(node, child_name, stc_group->dev);
		if (!pdev) {
			dev_warn(stc_group->dev, "Failed to create child %s dev\n",
                                 child_name);
		}
	}

	of_node_put(stc_group_node);
}

static void sp_stc_set_freq(struct sp_stc_group *priv)
{
	u32 val;

	/* Set clock source */
	val = STC_SRC(priv->src);

	/* Cal the prescaler val */
	if (priv->src == STC_SRC_EXT) {
		writel(val, priv->base + STC_PRESCALER);
		/* External clock configture */
		val = (priv->src_freq / priv->stc_freq) / 2 - 1;
		val |=  STC_EXT_SEL(STC_EXT_25M) | STC_EXT_EN;
		writel(val, priv->base + STC_EXTCFG);
	} else {
		val |= priv->src_freq / priv->stc_freq - 1;
		writel(val, priv->base + STC_PRESCALER);
	}
}

#if 0
static void sp_clk_disable_unprepare(void *data)
{
	clk_disable_unprepare(data);
}

static void sp_reset_control_assert(void *data)
{
	reset_control_assert(data);
}
#endif

static int sp_stc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp_stc_group *priv;
	int ret = 0;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

#if 0 // TODO: Is clk neccessary ?
	priv->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return dev_err_probe(dev, PTR_ERR(priv->clk), "Failed to get clock\n");

	ret = clk_prepare_enable(priv->clk);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to enable clock\n");

	ret = devm_add_action_or_reset(dev, sp_clk_disable_unprepare, priv->clk);
	if (ret)
		return ret;

	/* The timer and watchdog shared the STC reset */
	priv->rstc = devm_reset_control_get_shared(dev, NULL);
	if (IS_ERR(priv->rstc))
		return dev_err_probe(dev, PTR_ERR(priv->rstc), "Failed to get reset\n");

	reset_control_deassert(priv->rstc);

	ret = devm_add_action_or_reset(dev, sp_reset_control_assert, priv->rstc);
	if (ret)
		return ret;
#endif
	priv->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	/* Get STC source */
	ret = of_property_read_u8(dev->of_node, "stc-src-sel", &priv->src);
	if (ret) {
		dev_err(dev, "Failed to get STC clock source selection (%d)\n", ret);
 		return ret;
	}

	/* Get STC frequence */
	ret = of_property_read_u32(dev->of_node, "stc-freq", &priv->stc_freq);
	if (ret) {
		dev_err(dev, "Failed to get STC frequency (%d)\n", ret);
 		return ret;
	}

	//priv->src = STC_SRC_SYS;//xtdebug

	if (priv->src == STC_SRC_EXT) {
		/* default 25M don't consider 32k */
		priv->src_freq = 25000000;
	} else { /* system clock as source */
		priv->sys_clk = devm_clk_get(dev, "clk_sys");
		if (IS_ERR(priv->sys_clk))
			return dev_err_probe(dev, PTR_ERR(priv->sys_clk), "Failed to get system clock\n");

		priv->src_freq = clk_get_rate(priv->sys_clk);
	}

	sp_stc_set_freq(priv);

	priv->dev = dev;

	sp_stc_dbg("src=%d, src_freq=%d, stc_freq=%d\n", priv->src, priv->src_freq, priv->stc_freq);

	of_stc_register_devices(priv);

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static u32 regs[5];
static int sp_stc_suspend(struct device *dev)
{
	struct sp_stc_group *priv = dev_get_drvdata(dev);
	void __iomem *base = priv->base;
	int i;

	//sp_stc_dbg(">>>>>> [DEBUG] STC suspend <<<<<<\n");

	/* Save the reg val */
	for(i = 0; i < 5; i++)
		regs[i] = readl_relaxed(base + i * 0x4);

	//clk_disable_unprepare(priv->clk);
	//reset_control_assert(priv->rstc);

	return 0;
}

static int sp_stc_resume(struct device *dev)
{
	struct sp_stc_group *priv = dev_get_drvdata(dev);
	void __iomem *base = priv->base;
	int i;

	//sp_stc_dbg(">>>>>> [DEBUG] STC resume <<<<<<\n");

	//reset_control_deassert(priv->rstc);
	//clk_prepare_enable(priv->clk);

	/* Restore the reg val */
	for(i = 0; i < 5; i++)
		writel(regs[i], base + i * 0x4);

	return 0;
}
#endif

static const struct of_device_id sp_stc_of_match[] = {
	{ .compatible = "sunplus,sp7350-stc" },
	{},
};
MODULE_DEVICE_TABLE(of, sp_stc_of_match);

static const struct dev_pm_ops sp_stc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp_stc_suspend,
				sp_stc_resume)
};

static struct platform_driver sp_stc_driver = {
	.probe		= sp_stc_probe,
	.driver		= {
		.name	= "sunplus_stc",
		.pm		= &sp_stc_pm_ops,
		.of_match_table = of_match_ptr(sp_stc_of_match),
	},
};

module_platform_driver(sp_stc_driver);

MODULE_AUTHOR("Xiantao Hu <xt.hu@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus STC Driver");
MODULE_LICENSE("GPL v2");
