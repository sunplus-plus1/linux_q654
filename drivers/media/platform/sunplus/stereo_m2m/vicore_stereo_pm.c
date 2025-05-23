/*!
 * vicore_stereo_pm.h - stereo function control system
 * @file vicore_stereo_pm.h
 * @brief  stereo power control file system
 * @author Saxen Ko <saxen.ko@vicorelogic.com>
 * @version 1.0
 * @copyright  Copyright (C) 2022 Vicorelogic
 * @note
 * Copyright (C) 2022 Vicorelogic
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#ifdef CONFIG_PM
#include <linux/pm_runtime.h>
#endif

#include "vicore_stereo_pm.h"
#include "vicore_stereo.h"
#include "vicore_stereo_util.h"

#ifdef CONFIG_PM
int vicore_stereo_suspend_ops(struct device *dev)
{
	struct vicore_stereo *video = dev_get_drvdata(dev);

	dev_info(dev, "stereo engine suspend\n");

	mutex_lock(&video->device_lock);

	v4l2_m2m_suspend(video->m2m_dev);

	/* disable stereo interrupt */
	vicore_stereo_clk_gating(video->clk_gate, false);
	disable_irq(video->irq);
	vicore_stereo_intr_enable(0);
	//if (video->rst)
		//reset_control_assert(video->rst);
	vicore_stereo_clk_gating(video->clk_gate, true);

	/*
	if (video->regulator) {
		ret = regulator_disable(video->regulator);
		if (ret != 0)
			pr_err("%s, regulator_disable() fail, ret = %d\n", __func__, ret);
	}
	*/

	mutex_unlock(&video->device_lock);

	return 0;
}

int vicore_stereo_resume_ops(struct device *dev)
{
	struct vicore_stereo *video = dev_get_drvdata(dev);

	dev_info(dev, "stereo engine resume\n");

	mutex_lock(&video->device_lock);

	/*
	if (video->regulator) {
		ret = regulator_enable(video->regulator);
		if (ret != 0)
			pr_err("%s, regulator_enable() fail, ret = %d\n", __func__, ret);
	}
	*/
	/* enable stereo interrupt */
	vicore_stereo_clk_gating(video->clk_gate, false);

	//if (video->rst)
		//reset_control_deassert(video->rst);
	vicore_stereo_intr_enable(1);
	enable_irq(video->irq);
	vicore_stereo_init_sgm8dir();
	vicore_stereo_clk_gating(video->clk_gate, true);

	// let sw restart all setting
	g_stereo_status = VCL_STEREO_INIT;

	mutex_unlock(&video->device_lock);

	v4l2_m2m_resume(video->m2m_dev);
	return 0;
}
#endif