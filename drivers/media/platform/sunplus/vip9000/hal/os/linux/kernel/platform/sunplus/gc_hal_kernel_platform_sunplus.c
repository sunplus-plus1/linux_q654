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
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"
#include <linux/clk.h>
#include <dt-bindings/clock/sp-sp7350.h>

gceSTATUS
_AdjustParam(
    IN gcsPLATFORM *Platform,
    OUT gcsMODULE_PARAMETERS *Args
    )
{
    int ret = 0;
    uint clk_real;
    struct clk *clk;
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
    //Args->contiguousBase = 0x78000000;
    //Args->contiguousSize = 0x8000000;
    //Args->showArgs=1;
    //Args->recovery=0;
    //Args->powerManagement=0;

	if (!of_property_read_u32(pdev->dev.of_node, "clock-frequency", &npu_clock)) {
        printk("NPU dts clock= %d\n", npu_clock);
    }
    else {
        npu_clock = 500000000;
        printk("NPU clock = %d\n", npu_clock);
    }

    printk("Enable NPU clock(s)\n");
    clk = devm_clk_get(&pdev->dev, 0);

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


gceSTATUS
_GetPower(IN gcsPLATFORM *Platform)
{
	return gcvSTATUS_OK;
}


gceSTATUS
_DownPower(IN gcsPLATFORM *Platform)
{
	return gcvSTATUS_OK;
}

static struct _gcsPLATFORM_OPERATIONS default_ops =
{
    .adjustParam   = _AdjustParam,
 //   .getPower  = _GetPower,
 //   .setPower = _SetPower,
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
