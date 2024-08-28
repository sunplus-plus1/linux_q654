// SPDX-License-Identifier: GPL-2.0-or-later

/**************************************************************************************************/
/* How to test RTC:										  */
/*												  */
/* 1. use kernel commands									  */
/* hwclock - query and set the hardware clock (RTC)						  */
/*												  */
/* (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' */
/*								&& hwclock -r; sleep 1); done)	  */
/* date 121209002014 # Set system to 2014/Dec/12 09:00						  */
/* (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' */
/*								&& hwclock -r; sleep 1); done)	  */
/* hwclock -s # Set the System Time from the Hardware Clock					  */
/* (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' */
/*								&& hwclock -r; sleep 1); done)	  */
/* date 121213002014 # Set system to 2014/Dec/12 13:00						  */
/* (for i in `seq 5`; do (echo ------ && echo -n 'date      : ' && date && echo -n 'hwclock -r: ' */
/*								&& hwclock -r; sleep 1); done)	  */
/* hwclock -w # Set the Hardware Clock to the current System Time				  */
/* (for i in `seq 10000`; do (echo ------ && echo -n 'date  : ' && date && echo -n 'hwclock -r: ' */
/*								&& hwclock -r; sleep 1); done)	  */
/*												  */
/* How to setup alarm (e.g., 10 sec later):							  */
/*     echo 0 > /sys/class/rtc/rtc0/wakealarm && nnn=`date '+%s'` && echo $nnn && \		  */
/*     nnn=`expr $nnn + 10` && echo $nnn > /sys/class/rtc/rtc0/wakealarm			  */
/*												  */
/* 2. use RTC Driver Test Program (\linux\application\module_test\rtc\rtc-test.c)		  */
/*												  */
/**************************************************************************************************/
#include <linux/module.h>
#include <linux/err.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/ktime.h>
#include <linux/io.h>
#include <linux/delay.h>

/* ---------------------------------------------------------------------------------------------- */
#define FUNC_DEBUG() pr_debug("[RTC] Debug: %s(%d)\n", __func__, __LINE__)

#define RTC_DEBUG(fmt, args ...) pr_debug("[RTC] Debug: " fmt, ## args)
#define RTC_INFO(fmt, args ...) pr_info("[RTC] Info: " fmt, ## args)
#define RTC_WARN(fmt, args ...) pr_warn("[RTC] Warning: " fmt, ## args)
#define RTC_ERR(fmt, args ...) pr_err("[RTC] Error: " fmt, ## args)
/* ---------------------------------------------------------------------------------------------- */

struct sunplus_rtc {
	struct clk *rtcclk;
	struct reset_control *rstc;
	unsigned long set_alarm_again;
	u32 __iomem *mbox_base;
	u32 rtc_irq;
	u32 rtc_back_ctrl;
	u32 rtc_back_ontime_set;
};

struct sunplus_rtc sp_rtc;

#define RTC_REG_NAME		"rtc_reg"
#define MBOX_REG_NAME		"mbox_reg"
#define MBOX_RTC_SUSPEND_IN      (0x11225566)
#define MBOX_RTC_SUSPEND_OUT     (0x33447788)

#define INT_STATUS_MASK		0x1
#define INT_STATUS_UPDATE	0x0
#define INT_STATUS_ALARM	0x1

struct sp_rtc_reg {
	unsigned int rsv00;
	unsigned int rtc_ctrl;
	unsigned int rtc_timer;
	unsigned int rtc_ontime_set;
	unsigned int rtc_clock_set;
	unsigned int rsv05;
	unsigned int rtc_periodic_set;
	unsigned int rtc_int_status;
	unsigned int rsv08;
	unsigned int rsv09;
	unsigned int sys_rtc_cnt_31_0;
	unsigned int sys_rtc_cnt_63_32;
	unsigned int rsv12;
	unsigned int rsv13;
	unsigned int rsv14;
	unsigned int rsv15;
	unsigned int rsv16;
	unsigned int rsv17;
	unsigned int rsv18;
	unsigned int rsv19;
	unsigned int rsv20;
	unsigned int rsv21;
	unsigned int rsv22;
	unsigned int rsv23;
	unsigned int rsv24;
	unsigned int rsv25;
	unsigned int rsv26;
	unsigned int rsv27;
	unsigned int rsv28;
	unsigned int rsv29;
	unsigned int rsv30;
	unsigned int rsv31;
};

static struct sp_rtc_reg *rtc_reg_ptr;

static void sp_get_seconds(unsigned long *secs)
{
	*secs = (unsigned long)readl(&rtc_reg_ptr->rtc_timer);
}

static void sp_set_seconds(unsigned long secs)
{
	writel((u32)secs, &rtc_reg_ptr->rtc_clock_set);
}

static int sp_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long secs;

	sp_get_seconds(&secs);
	rtc_time64_to_tm(secs, tm);
	RTC_DEBUG("%s:  RTC date/time to %d-%d-%d, %02d:%02d:%02d.\r\n",
		  __func__, tm->tm_mday, tm->tm_mon + 1, tm->tm_year,
		  tm->tm_hour, tm->tm_min, tm->tm_sec);

	return rtc_valid_tm(tm);
}

int sp_rtc_get_time(struct rtc_time *tm)
{
	unsigned long secs;

	sp_get_seconds(&secs);
	rtc_time64_to_tm(secs, tm);
	return 0;
}
EXPORT_SYMBOL(sp_rtc_get_time);

static int sp_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	FUNC_DEBUG();

	// Keep RTC from system reset
	writel(MBOX_RTC_SUSPEND_IN, sp_rtc.mbox_base); // tell CM4 in suspend by mailbox

	//backup rtc value, maybe clear by cm4 rtc wakeup
	sp_rtc.rtc_back_ctrl = readl(&rtc_reg_ptr->rtc_ctrl);
	sp_rtc.rtc_back_ontime_set = readl(&rtc_reg_ptr->rtc_ontime_set);

	writel(0x13, &rtc_reg_ptr->rtc_ctrl);//enable rtc interrupt

	if (sp_rtc.rtc_irq > 0) {
		if (device_may_wakeup(&pdev->dev))
			enable_irq_wake(sp_rtc.rtc_irq);
		else
			disable_irq_wake(sp_rtc.rtc_irq);
	}

	return 0;
}

static int sp_rtc_resume(struct platform_device *pdev)
{
	/*						*/
	/* Because RTC is still powered during suspend,	*/
	/* there is nothing to do here.			*/
	/*						*/
	FUNC_DEBUG();

	writel(MBOX_RTC_SUSPEND_OUT, sp_rtc.mbox_base); // tell CM4 out suspend by mailbox

	writel(sp_rtc.rtc_back_ctrl, &rtc_reg_ptr->rtc_ctrl);
	writel(sp_rtc.rtc_back_ontime_set, &rtc_reg_ptr->rtc_ontime_set);

	if (sp_rtc.rtc_irq > 0) {
		if (device_may_wakeup(&pdev->dev))
			disable_irq_wake(sp_rtc.rtc_irq);
	}

	return 0;
}

static int sp_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long secs;

	secs = rtc_tm_to_time64(tm);
	RTC_DEBUG("%s, secs = %lu\n", __func__, secs);
	sp_set_seconds(secs);

	return 0;
}

static int sp_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_device *rtc = dev_get_drvdata(dev);
	unsigned long alarm_time;

	alarm_time = rtc_tm_to_time64(&alrm->time);
	RTC_DEBUG("%s, alarm_time: %u\n", __func__, (u32)(alarm_time));

	if (alarm_time > 0xFFFFFFFF)
		return -EINVAL;

	if (rtc->aie_timer.enabled && rtc->aie_timer.node.expires == ktime_set(alarm_time, 0)) {
		if (rtc->uie_rtctimer.enabled)
			sp_rtc.set_alarm_again = 1;
	}

	writel((u32)alarm_time, &rtc_reg_ptr->rtc_ontime_set);
	wmb();			// make sure settings are effective.

	// enable alarm here after enabling update IRQ
	//writel(0x13, &rtc_reg_ptr->rtc_ctrl);
	if (rtc->uie_rtctimer.enabled)
		writel(0x13, &rtc_reg_ptr->rtc_ctrl);
	else if (!rtc->aie_timer.enabled)
		writel(0x10, &rtc_reg_ptr->rtc_ctrl);

	usleep_range(10);

	return 0;
}

static int sp_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	unsigned int alarm_time;

	alarm_time = readl(&rtc_reg_ptr->rtc_ontime_set);
	RTC_DEBUG("%s, alarm_time: %u\n", __func__, alarm_time);
	rtc_time64_to_tm((unsigned long)(alarm_time), &alrm->time);

	return 0;
}

static int sp_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct rtc_device *rtc = dev_get_drvdata(dev);

	if (enabled)
		writel(0x13, &rtc_reg_ptr->rtc_ctrl);
	else if (!rtc->uie_rtctimer.enabled)
		writel(0x10, &rtc_reg_ptr->rtc_ctrl);

	return 0;
}

static const struct rtc_class_ops sp_rtc_ops = {
	.read_time =		sp_rtc_read_time,
	.set_time =		sp_rtc_set_time,
	.set_alarm =		sp_rtc_set_alarm,
	.read_alarm =		sp_rtc_read_alarm,
	.alarm_irq_enable =	sp_rtc_alarm_irq_enable,
};

static irqreturn_t rtc_irq_handler(int irq, void *dev_id)
{
	struct platform_device *plat_dev = dev_id;
	struct rtc_device *rtc = platform_get_drvdata(plat_dev);

	if ((readl(&rtc_reg_ptr->rtc_int_status) & INT_STATUS_MASK) == INT_STATUS_ALARM) {
		if (rtc->uie_rtctimer.enabled) {
			rtc_update_irq(rtc, 1, RTC_IRQF | RTC_UF);
			RTC_DEBUG("[RTC] update irq\n");

			if (sp_rtc.set_alarm_again == 1) {
				sp_rtc.set_alarm_again = 0;
				rtc_update_irq(rtc, 1, RTC_IRQF | RTC_AF);
				RTC_DEBUG("[RTC] alarm irq\n");
			}
		} else {
			rtc_update_irq(rtc, 1, RTC_IRQF | RTC_AF);
			RTC_DEBUG("[RTC] alarm irq\n");
		}
	}

	return IRQ_HANDLED;
}

static int sp_rtc_probe(struct platform_device *plat_dev)
{
	int ret;
	int err, irq;
	struct rtc_device *rtc = NULL;
	struct resource *res;
	void __iomem *reg_base = NULL;

	FUNC_DEBUG();

	memset(&sp_rtc, 0, sizeof(sp_rtc));

	// find and map our resources
	res = platform_get_resource_byname(plat_dev, IORESOURCE_MEM, RTC_REG_NAME);
	RTC_DEBUG("res = 0x%llx\n", res->start);

	if (res) {
		reg_base = devm_ioremap_resource(&plat_dev->dev, res);
		if (IS_ERR(reg_base))
			RTC_ERR("%s devm_ioremap_resource fail\n", RTC_REG_NAME);
	}
	RTC_DEBUG("reg_base = 0x%lx\n", (unsigned long)reg_base);

	res = platform_get_resource_byname(plat_dev, IORESOURCE_MEM, MBOX_REG_NAME);
	RTC_DEBUG("res = 0x%llx\n", res->start);

	if (res) {
		sp_rtc.mbox_base = devm_ioremap_resource(&plat_dev->dev, res);
		if (IS_ERR(sp_rtc.mbox_base))
			RTC_ERR("%s devm_ioremap_resource fail\n", MBOX_REG_NAME);
	}
	RTC_DEBUG("mailbox_base = 0x%lx\n", (unsigned long)sp_rtc.mbox_base);

	// clk
	sp_rtc.rtcclk = devm_clk_get(&plat_dev->dev, NULL);
	RTC_DEBUG("sp_rtc->clk = 0x%lx\n", (unsigned long)sp_rtc.rtcclk);
	if (IS_ERR(sp_rtc.rtcclk))
		RTC_DEBUG("devm_clk_get fail\n");

	ret = clk_prepare_enable(sp_rtc.rtcclk);

	// reset
	sp_rtc.rstc = devm_reset_control_get(&plat_dev->dev, NULL);
	RTC_DEBUG("sp_rtc->rstc = 0x%lx\n", (unsigned long)sp_rtc.rstc);
	if (IS_ERR(sp_rtc.rstc)) {
		ret = PTR_ERR(sp_rtc.rstc);
		RTC_ERR("SPI failed to retrieve reset controller: %d\n", ret);
		goto free_clk;
	}

	ret = reset_control_deassert(sp_rtc.rstc);
	if (ret)
		goto free_clk;

	rtc_reg_ptr = (struct sp_rtc_reg *)(reg_base);

	// Keep RTC from system reset
	writel((1 << (16 + 4)) | (1 << 4), &rtc_reg_ptr->rtc_ctrl);

	// request irq
	irq = platform_get_irq(plat_dev, 0);
	if (irq < 0) {
		RTC_ERR("platform_get_irq failed\n");
		goto free_reset_assert;
	}

	err = devm_request_irq(&plat_dev->dev, irq, rtc_irq_handler,
			       IRQF_TRIGGER_RISING, "rtc irq", plat_dev);
	if (err) {
		RTC_ERR("devm_request_irq failed: %d\n", err);
		goto free_reset_assert;
	}

	device_init_wakeup(&plat_dev->dev, 1);
	sp_rtc.rtc_irq = irq;

	rtc = devm_rtc_device_register(&plat_dev->dev, "sp7350-rtc", &sp_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		ret = PTR_ERR(rtc);
		goto free_reset_assert;
	}

	platform_set_drvdata(plat_dev, rtc);
	RTC_INFO("sp7350-rtc loaded\n");

	return 0;

free_reset_assert:
	reset_control_assert(sp_rtc.rstc);
free_clk:
	clk_disable_unprepare(sp_rtc.rtcclk);

	return ret;
}

static int sp_rtc_remove(struct platform_device *plat_dev)
{
	reset_control_assert(sp_rtc.rstc);

	return 0;
}

static const struct of_device_id sp_rtc_of_match[] = {
	{ .compatible = "sunplus,sp7350-rtc" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_rtc_of_match);

static struct platform_driver sp_rtc_driver = {
	.probe   = sp_rtc_probe,
	.remove  = sp_rtc_remove,
	.suspend = sp_rtc_suspend,
	.resume  = sp_rtc_resume,
	.driver  = {
		.name = "sp7350-rtc",
		.owner = THIS_MODULE,
		.of_match_table = sp_rtc_of_match,
	},
};
module_platform_driver(sp_rtc_driver);

MODULE_AUTHOR("Vincent Shih <vincent.shih@sunplus.com>");
MODULE_DESCRIPTION("Sunplus RTC driver");
MODULE_LICENSE("GPL v2");

