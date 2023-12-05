// SPDX-License-Identifier: GPL-2.0-only
/*
 * sunplus Watchdog Driver
 *
 * Copyright (C) 2021 Sunplus Technology Co., Ltd.
 *
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/watchdog.h>
#include <linux/interrupt.h>

#define WDT_CTRL		0x00
#define WDT_CNT			0x04

#define RBUS_WDT_RST		BIT(0)
#define STC_WDT_RST		BIT(4)
#define MASK_SET(mask)		((mask) | (mask << 16))
/* Mode selection */
#define WDT_MODE_INTR		0x0
#define WDT_MODE_RST		0x2
#define WDT_MODE_INTR_RST	0x3

#define WDT_STOP		0x3877
#define WDT_RESUME		0x4A4B
#define WDT_CLRIRQ		0x7482
#define WDT_UNLOCK		0xAB00
#define WDT_LOCK		0xAB01
#define WDT_CONMAX		0xDEAF

#define STC_CLK			1000000
/* HW_TIMEOUT_MAX = 0xffffffff/1MHz = 4294 */
#define SP_WDT_MAX_TIMEOUT	4294U
#define SP_WDT_DEFAULT_TIMEOUT	10

#define DEVICE_NAME		"sunplus-wdt"

static int irqflag = 0;

static unsigned int timeout;
module_param(timeout, int, 0);
MODULE_PARM_DESC(timeout, "Watchdog timeout in seconds");

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
			__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

struct sp_wdt_priv {
	struct watchdog_device wdev;
	void __iomem *base;
	void __iomem *base_pt;
	void __iomem *rst_en;
	void __iomem *prescaler;
	int irqn;
	struct clk *clk;
	struct reset_control *rstc;
	u32 mode;
};

static int sp_wdt_restart(struct watchdog_device *wdev,
			  unsigned long action, void *data)
{
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	void __iomem *base = priv->base;
	void __iomem *base_pt = priv->base_pt;
	
	writel(WDT_STOP, base + WDT_CTRL);
	writel(WDT_UNLOCK, base + WDT_CTRL);
	writel(0x1, base + WDT_CNT);
	writel(0x1, base_pt + WDT_CNT);
	
	writel(WDT_LOCK, base + WDT_CTRL);
	writel(WDT_RESUME, base + WDT_CTRL);

	return 0;
}

static int sp_wdt_ping(struct watchdog_device *wdev)
{
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	void __iomem *base = priv->base;
	void __iomem *base_pt = priv->base_pt;
	u32 count_f;
	u32 count_b;
	u32 time_f;
	u32 time_b;
	int irqn = priv->irqn;

	time_f = wdev->timeout - wdev->pretimeout;
	time_b = wdev->pretimeout;

	count_f = time_f * STC_CLK;
	count_b = time_b * STC_CLK;

	if (wdev->timeout > SP_WDT_MAX_TIMEOUT) {
		/* WDT_CONMAX sets the count to the maximum (down-counting). */
		writel(WDT_CONMAX, base + WDT_CTRL);
	} else {
		writel(WDT_UNLOCK, base + WDT_CTRL);
		writel(count_f, base + WDT_CNT);
		writel(count_b, base_pt + WDT_CNT);
		writel(WDT_LOCK, base + WDT_CTRL);
	}

	/* Solution for WARNING unbalanced enable irq */
	if(!irqflag)
		disable_irq(irqn);
	else
		irqflag--;

	/*
	 * Workaround for pretimeout counter. See the function sp_wdt_isr()
	 * for details .
	 */
	writel(WDT_CLRIRQ, base + WDT_CTRL);
	enable_irq(irqn);

	return 0;
}

static int sp_wdt_stop(struct watchdog_device *wdev)
{
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	void __iomem *base = priv->base;

	writel(WDT_STOP, base + WDT_CTRL);

	return 0;
}

static int sp_wdt_start(struct watchdog_device *wdev)
{
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	void __iomem *base = priv->base;

	writel(WDT_RESUME, base + WDT_CTRL);

	return 0;
}

static unsigned int sp_wdt_get_timeleft(struct watchdog_device *wdev)
{
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	void __iomem *base = priv->base;
	u32 val;

	void __iomem *base_pt = priv->base_pt;
	u32 count_f;
	u32 count_b;
	u32 time_f;

	count_f = readl(base + WDT_CNT);
	count_b = readl(base_pt + WDT_CNT);

	/* count_f is always running, to 0 and recycle */
	time_f = wdev->timeout - wdev->pretimeout;
	if(count_f < time_f * STC_CLK)
		val = count_f + count_b;
	else
		val = count_b;

	return val;
}

static int sp_wdt_set_mode(struct watchdog_device *wdev)
{
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	void __iomem *base_pt = priv->base_pt;
	u32 val = priv->mode;

	writel(val, base_pt);

	return 0;
}

static int sp_wdt_hw_init(struct watchdog_device *wdev)
{
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	void __iomem *base = priv->base;
	void __iomem *rst_en = priv->rst_en;
	void __iomem *prescaler = priv->prescaler;
	u32 val;

	/* Set watchdog prescaler */
	val = 500000000 / STC_CLK - 1;
	writel(val, prescaler);

	writel(WDT_CLRIRQ, base + WDT_CTRL);
	/* Enable watchdog reset bus */
	val = readl(rst_en);
	val |= MASK_SET(STC_WDT_RST);
	writel(val, rst_en);

	/* Stop counter and maximize the val */
	writel(WDT_STOP, base + WDT_CTRL);
	writel(WDT_CONMAX, base + WDT_CTRL);

	return 0;
}

static irqreturn_t sp_wdt_isr(int irq, void *arg)
{
	struct watchdog_device *wdev = arg;
	struct sp_wdt_priv *priv = watchdog_get_drvdata(wdev);
	//void __iomem *base = priv->base;
	int irqn = priv->irqn;

	//printk(">>> entry the isr \n");
	watchdog_notify_pretimeout(wdev);

	/*
	 * There are two counter in WDG, wdg_cnt and wdg_intrst_cnt.
	 * When the wdg_cnt down to 0, interrupt flag = 1. For wdg_intrst_cnt
	 * to keep counting, we need to keep the flag = 1. But if flag = 1,
	 * the interrupt handle always acknowledge. So mask the irq, clear
	 * the flag and enable the irq before reloading the counters.
	 */
	disable_irq_nosync(irqn);
	irqflag++;

	return IRQ_HANDLED;
}

static const struct watchdog_info sp_wdt_info = {
	.identity	= DEVICE_NAME,
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_MAGICCLOSE |
			  WDIOF_KEEPALIVEPING,
};

static const struct watchdog_info sp_wdt_pt_info = {
	.identity	= DEVICE_NAME,
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_PRETIMEOUT |
			  WDIOF_MAGICCLOSE |
			  WDIOF_KEEPALIVEPING,
};

static const struct watchdog_ops sp_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= sp_wdt_start,
	.stop		= sp_wdt_stop,
	.ping		= sp_wdt_ping,
	.get_timeleft	= sp_wdt_get_timeleft,
	.restart	= sp_wdt_restart,
};

static void sp_clk_disable_unprepare(void *data)
{
	clk_disable_unprepare(data);
}

static void sp_reset_control_assert(void *data)
{
	reset_control_assert(data);
}

static int sp_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp_wdt_priv *priv;
	struct watchdog_device *wdd;
	int ret;
	int irq;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

	wdd = &priv->wdev;
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

	priv->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->base_pt = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(priv->base_pt))
		return PTR_ERR(priv->base_pt);

	priv->rst_en = devm_platform_ioremap_resource(pdev, 2);
	if (IS_ERR(priv->rst_en))
		return PTR_ERR(priv->rst_en);

	priv->prescaler = devm_platform_ioremap_resource(pdev, 3);
	if (IS_ERR(priv->prescaler))
		return PTR_ERR(priv->prescaler);

	irq = platform_get_irq_optional(pdev, 0);
	if (irq > 0) {
		ret = devm_request_irq(&pdev->dev, irq, sp_wdt_isr, 0, "sp_wdg",
				       &priv->wdev);
		if (ret)
			return ret;

		priv->wdev.info = &sp_wdt_pt_info;
		priv->wdev.pretimeout = SP_WDT_DEFAULT_TIMEOUT / 2;
		priv->mode = WDT_MODE_INTR_RST;
		priv->irqn = irq;
	} else {
		if (irq == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		priv->wdev.info = &sp_wdt_info;
		priv->mode = WDT_MODE_RST;
	}

	priv->wdev.ops = &sp_wdt_ops;
	priv->wdev.timeout = SP_WDT_DEFAULT_TIMEOUT;
	priv->wdev.max_hw_heartbeat_ms = SP_WDT_MAX_TIMEOUT * 1000;
	priv->wdev.min_timeout = 1;
	priv->wdev.parent = dev;

	watchdog_set_drvdata(&priv->wdev, priv);

	sp_wdt_set_mode(&priv->wdev);
	sp_wdt_hw_init(&priv->wdev);

	watchdog_init_timeout(&priv->wdev, timeout, dev);
	watchdog_set_nowayout(&priv->wdev, nowayout);
	watchdog_stop_on_reboot(&priv->wdev);
	watchdog_set_restart_priority(&priv->wdev, 128);

	return devm_watchdog_register_device(dev, &priv->wdev);
}

#ifdef CONFIG_PM_SLEEP
static u32 regs[5];
static int sp_wdt_suspend(struct device *dev)
{
	struct sp_wdt_priv *priv = dev_get_drvdata(dev);
	void __iomem *base = priv->base;
	void __iomem *base_pt = priv->base_pt;
	void __iomem *prescaler = priv->prescaler;

	//printk(">>>>>> [DEBUG] WDT suspend <<<<<<\n");

	if (watchdog_active(&priv->wdev))
		sp_wdt_stop(&priv->wdev);

	/* Save the reg val */
	regs[0] = readl(base + WDT_CTRL);
	regs[1] = readl(base + WDT_CNT);
	regs[2] = readl(base_pt + WDT_CTRL);
	regs[3] = readl(base_pt + WDT_CNT);
	regs[4] = readl(prescaler);

	clk_disable_unprepare(priv->clk);
	reset_control_assert(priv->rstc);

	return 0;
}

static int sp_wdt_resume(struct device *dev)
{
	struct sp_wdt_priv *priv = dev_get_drvdata(dev);
	void __iomem *base = priv->base;
	void __iomem *base_pt = priv->base_pt;
	void __iomem *prescaler = priv->prescaler;

	//printk(">>>>>> [DEBUG] WDT resume <<<<<<\n");

	reset_control_deassert(priv->rstc);
	clk_prepare_enable(priv->clk);

	/* Restore the reg val */
	writel(regs[0], base + WDT_CTRL);
	writel(regs[1], base + WDT_CNT);
	writel(regs[2], base_pt + WDT_CTRL);
	writel(regs[3], base_pt + WDT_CNT);
	writel(regs[4], prescaler);

	if (watchdog_active(&priv->wdev)) {
		sp_wdt_start(&priv->wdev);
		sp_wdt_ping(&priv->wdev);
	}

	return 0;
}
#endif

static const struct of_device_id sp_wdt_of_match[] = {
	{.compatible = "sunplus,sp7350-wdt", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_wdt_of_match);

static const struct dev_pm_ops sp_wdt_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp_wdt_suspend,
				sp_wdt_resume)
};

static struct platform_driver sp_wdt_driver = {
	.probe = sp_wdt_probe,
	.driver = {
		.name		= DEVICE_NAME,
		.pm		= &sp_wdt_pm_ops,
		.of_match_table	= sp_wdt_of_match,
	},
};

module_platform_driver(sp_wdt_driver);

MODULE_AUTHOR("Xiantao Hu <xt.hu@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus Watchdog Timer Driver");
MODULE_LICENSE("GPL v2");
