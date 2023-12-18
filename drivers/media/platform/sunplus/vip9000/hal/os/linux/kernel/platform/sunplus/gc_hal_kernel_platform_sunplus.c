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

#include <linux/io.h>
#include <linux/kernel.h>
//#include <dt-bindings/clock/amlogic,g12a-clkc.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"
#include <linux/clk.h>
#include <dt-bindings/clock/sp-sp7350.h>

static struct clk *clk = NULL;
static struct regulator *vcc_reg = NULL;
static struct reset_control *dec_rstc = NULL;

static int reg_write(unsigned long long reg, unsigned int writeval)
{
    void __iomem *vaddr;
   // reg = round_down(reg, 0x4);
    vaddr = ioremap(reg, 4);
    if(vaddr == NULL)
    {
        printk("reg_write ioremap_wc error\n");
        return -1;
    }
    writel(writeval, vaddr);
    iounmap(vaddr);

    return 0;
}

static int reg_read(unsigned long long  reg, unsigned int *readval)
{
    void __iomem *vaddr;

    //reg = round_down(reg, 0x4);
    vaddr = ioremap(reg, 4);
    if(vaddr == NULL)
    {
        printk("reg_read ioremap_wc error\n");
        return -1;
    }
    *readval = readl(vaddr);
    iounmap(vaddr);

    return 0;
}

gceSTATUS
_AdjustParam(
    IN gcsPLATFORM *Platform,
    OUT gcsMODULE_PARAMETERS *Args
    )
{
    int ret = 0;
    uint clk_real;
    int npu_clock = 0;
    struct platform_device *pdev = Platform->device;
    int irqLine = platform_get_irq_byname(pdev, "galcore");

    printk("galcore irq number is %d.\n", irqLine);
    if (irqLine< 0) {
        printk("get galcore irq resource error\n");
        irqLine = platform_get_irq(pdev, 0);
        printk("galcore irq number is %d\n", irqLine);
    }
    if (irqLine < 0) return gcvSTATUS_OUT_OF_RESOURCES;

    Args->irqs[gcvCORE_MAJOR] = irqLine;
    Args->registerBases[0] = 0xF8140000;
    Args->registerSizes[0] = 0x20000;

	if (!of_property_read_u32(pdev->dev.of_node, "clock-frequency", &npu_clock)) {
        printk("NPU dts clock= %d\n", npu_clock);
    }
    else {
        npu_clock = 500000000;
        printk("NPU clock = %d\n", npu_clock);
    }

    printk("Enable NPU clock(s)\n");
    if (IS_ERR(clk)) {
        printk("Can't find clock source\n");
        return PTR_ERR(clk);
    } else {
        printk("Find clock source\n");
        ret = clk_prepare_enable(clk);
        if (ret) {
            dev_err(&pdev->dev, "enabled clock failed\n");
            return gcvSTATUS_OUT_OF_RESOURCES;
        }
        dev_info(&pdev->dev, "NPU clock enabled\n");
        printk("NPU clock enabled\n");

        printk("Set clock to %d\n", npu_clock);
        clk_set_rate(clk, npu_clock);

        clk_real = clk_get_rate(clk);
        printk("NPU clock getback: %d\n", clk_real); 
    }

    return gcvSTATUS_OK;
}

#define MOON0_SCFG_7      	0xF880001C          //hw reset reg 
#define ISO_CTRL_EBABLE   	0xF880125C        //G36.23 ISO CTRL ENABLE
#define NPU_WORKAROUND_BIT 	0xF880007C

gceSTATUS
_GetPower(IN gcsPLATFORM *Platform)
{
	struct platform_device *pdev = Platform->device;
	struct device *dev = &Platform->device->dev;

	vcc_reg = devm_regulator_get(dev, "npu_core");
	if(IS_ERR(vcc_reg)){
        dev_err(dev, "failed to get regulator\n");
		return gcvSTATUS_FALSE;
	}

    clk = devm_clk_get(&pdev->dev, 0);
	if (IS_ERR(clk)) {
		printk("Can't find clock source\n");
		return PTR_ERR(clk);
	}

	dec_rstc = devm_reset_control_get(dev, "rstc_vip9000");
	if (IS_ERR(dec_rstc))
    {
        dev_err(dev, "failed to retrieve reset controller\n");
        return PTR_ERR(dec_rstc);
    }

	dev_info(dev, "NPU get power success\n");

	return gcvSTATUS_OK;
}

gceSTATUS
_PutPower(gcsPLATFORM *Platform)
{
	struct device *dev = &Platform->device->dev;

	dev_info(dev, "NPU get power success\n");

	clk = NULL;
	vcc_reg = NULL;
	dec_rstc = NULL;

	return gcvSTATUS_OK;
}

gceSTATUS
_SetPower(gcsPLATFORM *Platform, gctUINT32 DevIndex, gceCORE GPU, gctBOOL Enable)
{
	int ret = gcvSTATUS_FALSE;
    int npu_clock = 0;

	unsigned int  reg_read_value;

	struct device *dev = &Platform->device->dev;
    struct platform_device *pdev = Platform->device;

	dev_info(dev, "%s %s\n", __FUNCTION__, Enable?"Enable":"Disable");
	if(Enable){
		/*NPU Power on*/
		if(IS_ERR(vcc_reg)){
			dev_err(dev, "regulator get failed\n");
			return ret;
		}
		if(!regulator_is_enabled(vcc_reg)){
			ret = regulator_enable(vcc_reg);
			if(ret != 0){
				dev_err(dev, "regulator get failed: %d\n", ret);
				return ret;
			}else{
				dev_info(dev, "regulator enable successed\n");
			}
		}

		/*NPU HW clock enable and configure*/
		if (!of_property_read_u32(pdev->dev.of_node, "clock-frequency", &npu_clock)) {
		}
		else {
			npu_clock = 500000000;
		}

		if (IS_ERR(clk)) {
			dev_err(dev, "Can't find clock source\n");
			return PTR_ERR(clk);
		} else {
	        ret = clk_prepare_enable(clk);
	        if (ret) {
	            dev_err(&pdev->dev, "enabled clock failed\n");
	            return gcvSTATUS_OUT_OF_RESOURCES;
	        }
	        dev_info(&pdev->dev, "NPU clock enabled\n");
	        clk_set_rate(clk, npu_clock);
	        //clk_real = clk_get_rate(clk);
		}

		/*disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
		reg_read(ISO_CTRL_EBABLE, &reg_read_value);

		reg_read_value = reg_read_value&0x0;
		reg_write(ISO_CTRL_EBABLE, reg_read_value);

		/*NPU HW work-around*/
		reg_write(NPU_WORKAROUND_BIT,0x5811);
		reg_write(NPU_WORKAROUND_BIT,0x5807);

		/*NPU HW Reset deassert*/
	    if (IS_ERR(dec_rstc))
	    {
	        dev_err(dev, "failed to retrieve reset controller\n");
	        return PTR_ERR(dec_rstc);
	    }
	    else
	    {
	        ret = reset_control_deassert(dec_rstc);
	        if (ret)
	        {
	            dev_err(dev, "failed to deassert reset line\n");
	            return ret;
	        }
	        dev_info(dev, "reset okay\n");
	    }
	}else{
		/*NPU HW Reset assert*/
	    if (IS_ERR(dec_rstc))
	    {
	        dev_err(dev, "failed to retrieve reset controller\n");
	        return PTR_ERR(dec_rstc);
	    }
	    else
	    {
	        ret = reset_control_assert(dec_rstc);
	        if (ret)
	        {
	            dev_err(dev, "failed to deassert reset line\n");
	            return ret;
	        }
	        dev_info(dev, "reset okay\n");
	    }

		/*NPU HW work-around*/
		reg_write(NPU_WORKAROUND_BIT,0x5811);
		reg_write(NPU_WORKAROUND_BIT,0x5807);

		/*disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
		reg_read(ISO_CTRL_EBABLE, &reg_read_value);
		reg_read_value=reg_read_value|(0x1<<4);
		reg_write(ISO_CTRL_EBABLE,reg_read_value);

		/*NPU HW clock disable*/
		if (IS_ERR(clk)) {
			dev_err(dev, "Can't find clock source\n");
			return PTR_ERR(clk);
		} else {
			clk_disable_unprepare(clk);
			dev_info(&pdev->dev, "NPU clock disable\n");
		}

		/*NPU Power off*/
		if(IS_ERR(vcc_reg)){
			dev_err(dev, "regulator resource failed");
			return 0;
		}
		if(regulator_is_enabled(vcc_reg)){
			ret = regulator_disable(vcc_reg);
			if(ret != 0){
				dev_err(dev, "regulator get failed: %d\n", ret);
				return ret;
			}else{
				dev_info(dev, "regulator disable successed\n");
			}
		}
	}

	return gcvSTATUS_OK;
}

gceSTATUS
_SetClock(gcsPLATFORM *Platform, gctUINT32 DevIndex, gceCORE GPU, gctBOOL Enable)
{
	int ret = gcvSTATUS_OK;
	struct device *dev = &Platform->device->dev;

    if (IS_ERR(clk)) {
		dev_err(dev, "Can't find clock source\n");
        return PTR_ERR(clk);
    } else {
		if(Enable){
			ret = clk_enable(clk);
		}else{
			clk_disable(clk);
		}
    }

	dev_info(dev, "NPU clock %s %s\n", Enable?"Enable":"Disable", (ret==gcvSTATUS_OK)?"sucess":"fail");

	return ret;
}

gceSTATUS
_Reset(gcsPLATFORM *Platform, gctUINT32 DevIndex, gceCORE GPU)
{
	return gcvSTATUS_OK;
}

static struct _gcsPLATFORM_OPERATIONS default_ops =
{
    .adjustParam   = _AdjustParam,
    .getPower  = _GetPower,
    .putPower = _PutPower,
    .setPower = _SetPower,
    .setClock = _SetClock,
    .reset = _Reset,
};

static struct _gcsPLATFORM default_platform =
{
    .name = __FILE__,
    .ops  = &default_ops,
};

static struct platform_device *default_dev;

static const
struct of_device_id galcore_dev_match[] = {
    {
        .compatible = "galcore",
    },
    { },
};

int gckPLATFORM_Init(struct platform_driver *pdrv,
            struct _gcsPLATFORM **platform)
{
    printk("%s \n", __func__);
    //pdrv->driver.of_match_table = galcore_dev_match;
    *platform = &default_platform;
    return 0;
}

int gckPLATFORM_Terminate(struct _gcsPLATFORM *platform)
{
    printk("%s \n", __func__);

    if (default_dev) {
        platform_device_unregister(default_dev);
        default_dev = NULL;
    }

    return 0;
}
