// SPDX-License-Identifier: (GPL OR MIT)
/****************************************************************************
 *
 *    The MIT License (MIT)
 *
 *    Copyright (c) 2014 - 2018 Vivante Corporation
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a
 *    copy of this software and associated documentation files (the "Software"),
 *    to deal in the Software without restriction, including without limitation
 *    the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *    and/or sell copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in
 *    all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *    DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    Copyright (C) 2014 - 2018 Vivante Corporation
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *****************************************************************************
 *
 *    Note: This software is released under dual MIT and GPL licenses. A
 *    recipient may use this file under the terms of either the MIT license or
 *    GPL License. If you wish to use only one license not the other, you can
 *    indicate your decision by deleting one of the above license notices in your
 *    version of this file.
 *
 *****************************************************************************/

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"
#include <dt-bindings/clock/sp-sp7350.h>

#define WORKAROUND_FOR_NPU_SMS_RESET_BUG 1

#if WORKAROUND_FOR_NPU_SMS_RESET_BUG
#define NPU_WORKAROUND_BIT	0xF880007C

static void __iomem *npu_sms_reset_reg_base;
#endif
static void __iomem *npu_ios_reg_base;
static struct clk *clk;
static struct regulator *vcc_reg;
static struct reset_control *dec_rstc;

gceSTATUS
_AdjustParam(
	IN gcsPLATFORM * platform,
	OUT gcsMODULE_PARAMETERS * args
	)
{
	int ret = gcvSTATUS_OK;
	uint clk_real;
	int npu_clock = 0;
	struct platform_device *pdev = platform->device;
	struct device *dev = &platform->device->dev;

	int irqLine = platform_get_irq_byname(pdev, "galcore");

	dev_info(dev, "galcore irq number is %d.\n", irqLine);
	if (irqLine < 0) {
		dev_err(dev, "get galcore irq resource error\n");
		irqLine = platform_get_irq(pdev, 0);
		dev_info(dev, "galcore irq number is %d\n", irqLine);
	}
	if (irqLine < 0)
		return gcvSTATUS_OUT_OF_RESOURCES;

	args->irqs[gcvCORE_MAJOR] = irqLine;
	args->registerBases[0] = 0xF8140000;
	args->registerSizes[0] = 0x20000;

	if (of_property_read_u32(pdev->dev.of_node, "clock-frequency", &npu_clock))
		npu_clock = 500000000;

	dev_info(dev, "NPU dts clock= %d\n", npu_clock);

	dev_info(dev, "enable NPU clock(s)\n");
	if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
		return PTR_ERR(clk);
	}

	dev_info(dev, "Find clock source\n");
	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(&pdev->dev, "enabled clock failed\n");
		return gcvSTATUS_OUT_OF_RESOURCES;
	}
	dev_info(dev, "NPU clock enabled\n");

	dev_info(dev, "Set clock to %d\n", npu_clock);
	clk_set_rate(clk, npu_clock);

	clk_real = clk_get_rate(clk);
	dev_info(dev, "NPU clock getback: %d\n", clk_real);

	return ret;
}

gceSTATUS
_GetPower(IN gcsPLATFORM * platform)
{
	struct platform_device *pdev = platform->device;
	struct device *dev = &platform->device->dev;

	vcc_reg = devm_regulator_get(dev, "npu_core");
	if (IS_ERR(vcc_reg)) {
		dev_err(dev, "failed to get regulator\n");
		return gcvSTATUS_FALSE;
	}

	clk = devm_clk_get(&pdev->dev, 0);
	if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
		return PTR_ERR(clk);
	}

	dec_rstc = devm_reset_control_get(dev, "rstc_vip9000");
	if (IS_ERR(dec_rstc)) {
		dev_err(dev, "failed to retrieve reset controller\n");
		return PTR_ERR(dec_rstc);
	}

	npu_ios_reg_base = of_iomap(dev->of_node, 1);
	if (!npu_ios_reg_base) {
		dev_err(dev, "failed to get ios base\n");
		return gcvSTATUS_OUT_OF_MEMORY;
	}

#if WORKAROUND_FOR_NPU_SMS_RESET_BUG
	npu_sms_reset_reg_base = ioremap(NPU_WORKAROUND_BIT, 4);
	if (npu_sms_reset_reg_base == NULL) {
		dev_err(dev, "failed to get workaroud base\n");
		return gcvSTATUS_OUT_OF_MEMORY;
	}
#endif
	dev_info(dev, "NPU get power success\n");

	return gcvSTATUS_OK;
}

gceSTATUS
_PutPower(gcsPLATFORM *platform)
{
	struct device *dev = &platform->device->dev;

	dev_info(dev, "NPU put power success\n");

	clk = NULL;
	vcc_reg = NULL;
	dec_rstc = NULL;
	iounmap(npu_ios_reg_base);

#if WORKAROUND_FOR_NPU_SMS_RESET_BUG
	iounmap(npu_sms_reset_reg_base);
#endif

	return gcvSTATUS_OK;
}

gceSTATUS
_SetPower(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu, gctBOOL enable)
{
	int ret = gcvSTATUS_FALSE;
	int npu_clock = 0;

	unsigned int  reg_read_value;

	struct device *dev = &platform->device->dev;
	struct platform_device *pdev = platform->device;

	dev_dbg(dev, "%s %s\n", __func__, enable?"enable":"disable");
	if (enable) {
		/*NPU Power on*/
		if (IS_ERR(vcc_reg)) {
			dev_err(dev, "regulator get failed\n");
			return ret;
		}

		if (!regulator_is_enabled(vcc_reg)) {
			ret = regulator_enable(vcc_reg);
			if (ret != 0) {
				dev_err(dev, "regulator get failed: %d\n", ret);
				return ret;
			}

			mdelay(10);
			dev_dbg(dev, "regulator enable success\n");
		}

		/*NPU HW clock enable and configure*/
		if (of_property_read_u32(pdev->dev.of_node, "clock-frequency", &npu_clock))
			npu_clock = 900000000;

		if (IS_ERR(clk)) {
			dev_err(dev, "Can't find clock source\n");
			return PTR_ERR(clk);
		}

		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_err(&pdev->dev, "enabled clock failed\n");
			return gcvSTATUS_OUT_OF_RESOURCES;
		}
		clk_set_rate(clk, npu_clock);
		dev_dbg(&pdev->dev, "NPU clock enabled\n");

		/*disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
		dev_dbg(dev, "disable NPU ISO\n");
		reg_read_value = readl(npu_ios_reg_base);
		reg_read_value = reg_read_value&0x0;
		writel(reg_read_value, npu_ios_reg_base);

#if WORKAROUND_FOR_NPU_SMS_RESET_BUG
		/*NPU HW work-around*/
		dev_dbg(dev, "NPU HW work-around\n");
		writel(0x5811, npu_sms_reset_reg_base);
		udelay(300);
		writel(0x5807, npu_sms_reset_reg_base);
#endif

		/*NPU HW Reset deassert*/
		if (IS_ERR(dec_rstc)) {
			dev_err(dev, "failed to retrieve reset controller\n");
			return PTR_ERR(dec_rstc);
		}

		dev_dbg(dev, "NPU HW Deassert\n");
		ret = reset_control_deassert(dec_rstc);
		if (ret) {
			dev_err(dev, "failed to deassert reset line\n");
			return ret;
		}

		dev_dbg(dev, "reset okay\n");
	} else {
		/*NPU HW Reset assert*/
		if (IS_ERR(dec_rstc)) {
			dev_err(dev, "failed to retrieve reset controller\n");
			return PTR_ERR(dec_rstc);
		}

		dev_dbg(dev, "NPU HW Assert\n");
		ret = reset_control_assert(dec_rstc);
		if (ret) {
			dev_err(dev, "failed to deassert reset line\n");
			return ret;
		}
		dev_dbg(dev, "reset okay\n");

#if WORKAROUND_FOR_NPU_SMS_RESET_BUG
		/*NPU HW work-around*/
		dev_dbg(dev, "NPU HW work-around\n");
		writel(0x5811, npu_sms_reset_reg_base);
		udelay(300);
		writel(0x5807, npu_sms_reset_reg_base);
#endif

		/*enable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
		dev_dbg(dev, "enable NPU ISO\n");
		reg_read_value = readl(npu_ios_reg_base);
		reg_read_value = reg_read_value|(0x1<<4);
		writel(reg_read_value, npu_ios_reg_base);

		/*NPU HW clock disable*/
		if (IS_ERR(clk)) {
			dev_err(dev, "Can't find clock source\n");
			return PTR_ERR(clk);
		}

		dev_dbg(&pdev->dev, "NPU clock disabled\n");
		clk_disable_unprepare(clk);

		/*NPU Power off*/
		if (IS_ERR(vcc_reg)) {
			dev_err(dev, "regulator resource failed");
			return gcvSTATUS_FALSE;
		}
		if (regulator_is_enabled(vcc_reg)) {
			ret = regulator_disable(vcc_reg);
			if (ret != 0)
				dev_err(dev, "regulator get failed: %d\n", ret);
			else
				dev_dbg(dev, "regulator disable success\n");
		}
	}
	mdelay(2);

	dev_dbg(dev, "%s ret=%d\n", __func__, ret);

	return ret;
}

gceSTATUS
_SetClock(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu, gctBOOL enable)
{
	int ret = gcvSTATUS_OK;
	struct device *dev = &platform->device->dev;

	if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
		return PTR_ERR(clk);
	}

	if (enable)
		ret = clk_enable(clk);
	else
		clk_disable(clk);

	dev_dbg(dev, "NPU clock %s %s\n", enable?"enable":"disable", (ret == gcvSTATUS_OK)?"success":"fail");

	return ret;
}

gceSTATUS
_Reset(gcsPLATFORM *platform, gctUINT32 devIndex, gceCORE gpu)
{
	return gcvSTATUS_OK;
}

static struct _gcsPLATFORM_OPERATIONS default_ops = {

	.adjustParam   = _AdjustParam,
	.getPower  = _GetPower,
	.putPower = _PutPower,
	.setPower = _SetPower,
//	.setClock = _SetClock,
	.reset = _Reset,
};

static struct _gcsPLATFORM default_platform = {

	.name = __FILE__,
	.ops  = &default_ops,
};

static struct platform_device *default_dev;

static const struct of_device_id galcore_dev_match[] = {
	{
	.compatible = "galcore",
	},
	{ },
};

int gckPLATFORM_Init(struct platform_driver *pdrv,
		struct _gcsPLATFORM **platform)
{
	*platform = &default_platform;

	return 0;
}

int gckPLATFORM_Terminate(struct _gcsPLATFORM *platform)
{
	struct device *dev = &platform->device->dev;

	dev_info(dev, "%s\n", __func__);

	if (default_dev) {
		platform_device_unregister(default_dev);
		default_dev = NULL;
	}

	return 0;
}
