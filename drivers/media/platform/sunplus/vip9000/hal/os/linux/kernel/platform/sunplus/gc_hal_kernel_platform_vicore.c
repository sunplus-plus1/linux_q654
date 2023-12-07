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
#include <linux/regulator/consumer.h>
#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"
#include <linux/io.h>
#include <linux/delay.h>

#define MOON3_SCFG_17     0xF88001C4        //npu clk set reg
#define MOON3_SCFG_18     0xF88001C8        //npu clk set reg
#define MOON3_SCFG_19     0xF88001CC        //npu clk set reg

#define MOON2_SCFG_7      0xF880011C        //clk enable reg
#define ISO_CTRL_PWD      0xF8801258        //iso passwd set reg
#define ISO_CTRL_EBABLE   0xF880125C        //iso ctrl set reg
#define MOON0_SCFG_7      0xF880001C        //hw reset reg 
#define PD_GRP20          0xF8000F50        //power domain group control reg

#define RF_MASK_V(_mask, _val)          (((_mask) << 16) | (_val))
#define RF_MASK_V_CLR(_mask)            (((_mask) << 16) | 0)

// #define MOON3_SCFG_17_SET_VALUE ((0x3<<(0+16)) | (0x1<<(0+0)) | (0x3<<(3+16)) | (0x0<<(3+0))|(0x1<<(2+16))|(0x1<<(2+0)))
// #define MOON3_SCFG_19_SET_VALUE ((0xff<<(6+16)) | (0x8<<(6+0)) | (0x3<<(0+16)) | (0x1<<(2+0)))

#define MOON3_SCFG_17_SET_VALUE  RF_MASK_V(0xFFFF, 0x0408)
#define MOON3_SCFG_18_SET_VALUE  RF_MASK_V(0xFFFF, 0xC0BE)
#define MOON3_SCFG_19_SET_VALUE  RF_MASK_V(0xFFFF, 0x010B)

#define ISO_PWD 0xFFAA5500

gceSTATUS
_AdjustParam(
    IN gcsPLATFORM *Platform,
    OUT gcsMODULE_PARAMETERS *Args
    )
{
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

    return gcvSTATUS_OK;
}


static int reg_write(unsigned long long reg, unsigned int writeval)
{
    void __iomem *vaddr;
    printk("reg_write reg: 0x%llx, value: 0x%x\n",reg,writeval);
    printk("write value");
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

    printk("reg_read reg:0x%llx, read val:0x%d\n",reg,*readval);

    return 0;
}


/*上电流程
1. 通知PMIC 上电,判断上电是否完成 (相关寄存是多少？)
2. 配置NPU CLK 900M (npu每次唤醒重新power up,如果npu clk没有修改,该设置只设置一次) 
3. 使能NPU CLK(register G2.7(m2_sfg_7) NPU_CLKEN[14]
4. 设置ISO pwd (Register G36.22(pmc_iso_pwd)  = 0xFFAA5500) （复位后设置一次即可）
5. disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])
6. hw reset (Register G0.7(m0_scfg_7) NPU_RESEET[14])
*/
gceSTATUS
_GetPower(IN gcsPLATFORM *Platform)
{
    unsigned int  mo3_sft_cfg19_value;
    unsigned int  mo3_sft_cfg17_value;
    unsigned int  reg_read_value;

    struct device *dev = &Platform->device->dev;
    struct regulator *vcc_reg = NULL;

    printk("start \n");
    /*1.通知PMIC 上电,判断上电是否完成*/
    vcc_reg = devm_regulator_get(dev, "npu");
    regulator_enable(vcc_reg);
    
    printk("power on \n");
    /*2.配置NPU CLK 900M */
    reg_read(MOON3_SCFG_19, &mo3_sft_cfg19_value);

    reg_read(MOON3_SCFG_17, &mo3_sft_cfg17_value);

    printk("mo3_sft_cfg19_value: 0x%x",mo3_sft_cfg19_value);

    printk("mo3_sft_cfg17_value: 0x%x",mo3_sft_cfg17_value);
    
    printk("clk set \n");
    if((mo3_sft_cfg19_value!=MOON3_SCFG_19_SET_VALUE)&&(MOON3_SCFG_17!=MOON3_SCFG_17_SET_VALUE))
    {
        reg_write(MOON3_SCFG_17, MOON3_SCFG_17_SET_VALUE);
        reg_write(MOON3_SCFG_18, MOON3_SCFG_18_SET_VALUE);
        reg_write(MOON3_SCFG_19, MOON3_SCFG_19_SET_VALUE);
    }

    printk("enable npu clk \n");
    /*3.使能NPU CLK(register G2.7(m2_sfg_7) NPU_CLKEN[14]*/
    reg_read(MOON2_SCFG_7,&reg_read_value);
    reg_read_value = reg_read_value|(0xffff<<(16))|(0x1<15);
    reg_write(MOON2_SCFG_7, reg_read_value);

    printk("ios pwd \n");
    /*4.设置ISO pwd (Register G36.22(pmc_iso_pwd)  = 0xFFAA5500)*/
    reg_read(ISO_CTRL_PWD, &reg_read_value);
    if(reg_read_value!=ISO_PWD)
    {
        reg_write(ISO_CTRL_PWD,ISO_PWD);
    }

    printk("ios disable \n");
    /*5.disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
    reg_read(ISO_CTRL_EBABLE, &reg_read_value);
    reg_read_value=reg_read_value&0x0;
    reg_write(ISO_CTRL_EBABLE,reg_read_value);

    printk("hw reset \n");
    /*6.hw reset (Register G0.7(m0_scfg_7) NPU_RESEET[14])*/
   // reg_read(MOON0_SCFG_7, &reg_read_value);
    reg_read_value = 0x40004000;
    reg_write(MOON0_SCFG_7, reg_read_value);
    
    //reg_read(MOON0_SCFG_7, &reg_read_value);
    reg_read_value = 0x40000000;
    reg_write(MOON0_SCFG_7, reg_read_value);

    printk("get power down\n");   
	return gcvSTATUS_OK;
}

/*下点流程
1. SW 用QCTL 确认NPU 处于Idle 状态( register G30.22(pd_grp20) mo_lp_pd_npu_inter_qacp_b[0])；
2. Enable NPU ISO (register G36. ISO_CTRL_ENABLE [4])；
3. 通知PMIC 下电 ,判断下电是否完成(相关寄存是多少？)
*/
gceSTATUS
_SetPower(gcsPLATFORM *Platform, gctUINT32 DevIndex, gceCORE GPU, gctBOOL Enable)
{
    struct device *dev = &Platform->device->dev;
    struct regulator *vcc_reg = NULL;
    unsigned int reg_read_value;

    unsigned int mo3_sft_cfg19_value;
    unsigned int mo3_sft_cfg17_value;

    vcc_reg = devm_regulator_get(dev, "npu");  
    if(Enable)
    {
        printk("power on \n");   
        /*1.通知PMIC 上电,判断上电是否完成*/
        vcc_reg = devm_regulator_get(dev, "npu");
        if(vcc_reg!=NULL)
        {
            if(vcc_reg->enable_count!=0)
            {
                    regulator_enable(vcc_reg);
            }
        }
        else
        {
            return -1;
        }

        printk("power on \n");
        /*2.配置NPU CLK 900M */
        reg_read(MOON3_SCFG_19, &mo3_sft_cfg19_value);

        reg_read(MOON3_SCFG_17, &mo3_sft_cfg17_value);

        printk("mo3_sft_cfg19_value: 0x%x",mo3_sft_cfg19_value);

        printk("mo3_sft_cfg17_value: 0x%x",mo3_sft_cfg17_value);

        printk("clk set \n");
        if((mo3_sft_cfg19_value!=MOON3_SCFG_19_SET_VALUE)&&(MOON3_SCFG_17!=MOON3_SCFG_17_SET_VALUE))
        {
            reg_write(MOON3_SCFG_17, MOON3_SCFG_17_SET_VALUE);
            reg_write(MOON3_SCFG_18, MOON3_SCFG_18_SET_VALUE);
            reg_write(MOON3_SCFG_19, MOON3_SCFG_19_SET_VALUE);
        }

        printk("enable npu clk \n");
        /*3.使能NPU CLK(register G2.7(m2_sfg_7) NPU_CLKEN[14]*/
        reg_read(MOON2_SCFG_7,&reg_read_value);
        reg_read_value = reg_read_value|(0xffff<<(16))|(0x1<15);
        reg_write(MOON2_SCFG_7, reg_read_value);

        printk("ios pwd \n");
        /*4.设置ISO pwd (Register G36.22(pmc_iso_pwd)  = 0xFFAA5500)*/
        reg_read(ISO_CTRL_PWD, &reg_read_value);
        if(reg_read_value!=ISO_PWD)
        {
            reg_write(ISO_CTRL_PWD,ISO_PWD);
        }

        printk("ios disable \n");
        /*5.disable NPU ISO (Register G36. ISO_CTRL_ENABLE [4])*/
        reg_read(ISO_CTRL_EBABLE, &reg_read_value);
        reg_read_value=reg_read_value&0x0;
        reg_write(ISO_CTRL_EBABLE,reg_read_value);

        printk("hw reset \n");
        /*6.hw reset (Register G0.7(m0_scfg_7) NPU_RESEET[14])*/
        // reg_read(MOON0_SCFG_7, &reg_read_value);
        reg_read_value = 0x40004000;
        reg_write(MOON0_SCFG_7, reg_read_value);

        //reg_read(MOON0_SCFG_7, &reg_read_value);
        reg_read_value = 0x40000000;
        reg_write(MOON0_SCFG_7, reg_read_value);
        
    }else{
        printk("power off \n");   
        printk("PD_GRP20 \n");
        /*1.SW 用QCTL 确认NPU 处于Idle 状态( register G30.22(pd_grp20) mo_lp_pd_npu_inter_qacp_b[0])*/
        //  reg_read(PD_GRP20, &reg_read_status);
        //  reg_read_status = reg_read_status&0x00000001;
        //  if(reg_read_status!=0)
        //  {
        //      return gcvSTATUS_EXECUTED;
        //  }
        printk("ISO_CTRL_EBABLE \n");     
        /*2.Enable NPU ISO (register G36. ISO_CTRL_ENABLE [4])*/
        reg_read(ISO_CTRL_EBABLE, &reg_read_value);
        reg_read_value=reg_read_value|(0x1<<4);
        reg_write(ISO_CTRL_EBABLE,reg_read_value);

        /*3.通知PMIC 下电 ,判断下电是否完成(相关寄存是多少？)*/
        printk("enable count %d\n", vcc_reg->enable_count);       
        regulator_disable(vcc_reg);
        printk("set power down\n");       
    }    
  
	return gcvSTATUS_OK;
}

static struct _gcsPLATFORM_OPERATIONS default_ops =
{
    .adjustParam   = _AdjustParam,
    // .getPower  = _GetPower,
    // .setPower = _SetPower,
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
        .compatible = "vicore, galcore",
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
