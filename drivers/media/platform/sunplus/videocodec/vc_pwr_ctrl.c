/*****************************************************************************
 *
 *    The GPL License (GPL)
 *
 *    Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved.
 *    Copyright (c) 2011-2014, Google Inc. All rights reserved.
 *    Copyright (c) 2007-2010, Hantro OY. All rights reserved.
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
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include "vc_pwr_ctrl.h"

#define _DEBUG 0
#if _DEBUG
#define _PRINTK printk
#else
#define _PRINTK(text, ...){ }
#endif

#define PWR_CONTROL_EN		1

#define CHIP_VERSION_CHK	1
#define CHIP_VERSION_REG 	0xF8800000

static void __iomem *_chip_version_reg_base = NULL;

static int _chip_version = 0x0;
static bool _vc_pwr_on = false;

/* power is configure to default on in dts */
static atomic_t _vc_atom = ATOMIC_INIT(1);
static DEFINE_MUTEX(_vc_mtx);

/* vc video codec */
static struct regulator *_vc_regl = NULL;
static struct regulator *_vc_iso_regl = NULL;

static struct clk *_vc_clk = NULL;
static struct reset_control *_vc_rstc;

/* vc decode */
static struct clk *_vc_dec_clk=NULL;
static struct reset_control *_vc_dec_rstc;

/* vc encode */
static struct clk *_vc_enc_clk = NULL;
static struct reset_control *_vc_enc_rstc;

/* vc rst */
struct reset_control {
	struct reset_controller_dev *rcdev;
	struct list_head list;
	unsigned int id;
	struct kref refcnt;
	bool acquired;
	bool shared;
	bool array;
	atomic_t deassert_count;
	atomic_t triggered_count;
};

static void _vc_iso_control(int enable){
    int ret;

    if (IS_ERR(_vc_iso_regl)) {
        printk(KERN_ERR "failed to get iso base\n");
        return;
    }

    if (enable)
        ret = regulator_enable(_vc_iso_regl);
    else
        ret = regulator_disable(_vc_iso_regl);

    _PRINTK(KERN_DEBUG "VC ISO %s\n", (enable?"enable":"disable"));
}

static void _vc_reset_control(int deassert){

    if (IS_ERR(_vc_dec_rstc)) {
        printk(KERN_ERR "can't find decode reset control\n");
        return;
    }

    if (IS_ERR(_vc_enc_rstc)) {
        printk(KERN_ERR "can't find encode reset control\n");
        return;
    }

    if (IS_ERR(_vc_rstc)) {
        printk(KERN_ERR "can't find vc reset control\n");
        return;
    }

    if(deassert){
        if(reset_control_status(_vc_dec_rstc)){
            if(reset_control_deassert(_vc_dec_rstc)){
                printk(KERN_ERR "failed to deassert vc dec reset line\n");
                return;
            }
        }
        if(reset_control_status(_vc_enc_rstc)){
            if(reset_control_deassert(_vc_enc_rstc)){
                printk(KERN_ERR "failed to deassert vc enc reset line\n");
                return;
            }
        }
        if(reset_control_status(_vc_rstc)){
            if(reset_control_deassert(_vc_rstc)){
                printk(KERN_ERR "failed to deassert vc reset line\n");
                return;
            }
        }
        _PRINTK(KERN_DEBUG "VC HW reset deassert\n");
    }
    else{
        if(!reset_control_status(_vc_rstc)){
            if(reset_control_assert(_vc_rstc)){
                printk(KERN_ERR "failed to assert vc reset line\n");
                return;
            }
        }
        if(!reset_control_status(_vc_enc_rstc)){
            if(reset_control_assert(_vc_enc_rstc)){
                printk(KERN_ERR "failed to assert vc enc reset line\n");
                return;
            }
        }
        if(!reset_control_status(_vc_dec_rstc)){
            if(reset_control_assert(_vc_dec_rstc)){
                printk(KERN_ERR "failed to assert dec reset line\n");
                return;
            }
        }
        _PRINTK(KERN_DEBUG "VC HW reset assert\n");
    }
}

static void _vc_clk_control(int enable){
    if (IS_ERR(_vc_enc_clk)) {
        printk(KERN_ERR "can't find encode clock source\n");
        return;
    }

    if (IS_ERR(_vc_dec_clk)) {
        printk(KERN_ERR "can't find decode clock source\n");
        return;
    }

    if (IS_ERR(_vc_clk)) {
        printk(KERN_ERR "can't find vc clock source\n");
        return;
    }

    if(enable){
        clk_enable(_vc_enc_clk);
        clk_enable(_vc_dec_clk);
        clk_enable(_vc_clk);

        _PRINTK(KERN_DEBUG "VC HW clock enable\n");
    }
    else{
        clk_disable(_vc_clk);
        clk_disable(_vc_dec_clk);
        clk_disable(_vc_enc_clk);

        _PRINTK(KERN_DEBUG "VC HW clock disable\n");
    }
}

static void _vc_pwr_control(int enable){

    int ret = 0;

    if (IS_ERR(_vc_regl)) {
        printk(KERN_NOTICE "failed to get regulator\n");
        return;
    }

    if(enable){
        ret = regulator_enable(_vc_regl);
        if (ret != 0){
            printk(KERN_ERR "regulator enable failed: %d\n", ret);
        }
    }
    else{
        ret = regulator_disable(_vc_regl);
        if (ret != 0){
            printk(KERN_ERR "regulator disable failed: %d\n", ret);
        }
    }
}

static int _chip_version_get(void){
    if(_chip_version == 0x0){
        if(_chip_version_reg_base == NULL){
            _chip_version_reg_base = ioremap(CHIP_VERSION_REG, 4);
        }

        if (_chip_version_reg_base == NULL) {
            printk(KERN_ERR "failed to get chip version base\n");
            return 0;
        }

        _chip_version = readl(_chip_version_reg_base);
        iounmap(_chip_version_reg_base);
        _chip_version_reg_base = NULL;
    }

    return _chip_version;
}

void vc_regulator_control(struct platform_device *dev, int ctrl){
    int ret;

    if(!_vc_regl){
        _vc_regl = devm_regulator_get(&dev->dev, "video_codec");

        if(IS_ERR(_vc_regl)) {
            dev_info(&dev->dev, "failed to get vc regulator\n");
            return;
        }

        /* To avoid regulator late cleanup */
        if(regulator_is_enabled(_vc_regl)){
            ret = regulator_enable(_vc_regl);
            _vc_pwr_on = true;
        }
    }

    if(!_vc_iso_regl){
         _vc_iso_regl = devm_regulator_get(&dev->dev, "video_codec_iso");

        if(IS_ERR(_vc_iso_regl)) {
            dev_info(&dev->dev, "failed to get vc iso regulator\n");
            return;
        }
        ret = regulator_enable(_vc_iso_regl);
        _vc_pwr_on = true;
    }

#if PWR_CONTROL_EN
#if CHIP_VERSION_CHK
    /* No support for C3V-W version A */
    if(_chip_version_get() == 0xa30){
        return;
    }
#endif
    if(ctrl > -1){
        _vc_pwr_control(ctrl);
    }
#endif
    return;
}

void vc_power_on(void){
#if PWR_CONTROL_EN
#if CHIP_VERSION_CHK
    /* No support for C3V-W version A */
    if(_chip_version_get() == 0xa30){
        return;
    }
#endif
    /* No regulator, return */
    if(!_vc_regl || !_vc_iso_regl){
        return;
    }

    mutex_lock(&_vc_mtx);

    atomic_inc(&_vc_atom);

    if(atomic_read(&_vc_atom) == 1){
        /* VC power on */
        _vc_pwr_control(1);

        /* Disable VC ISO (Register G36. ISO_CTRL_ENABLE [5]) */
        _vc_iso_control(1);

        /* VC HW reset deassert */
        _vc_reset_control(1);

        /* VC HW clock enable */
        _vc_clk_control(1);

        mdelay(1);

        _vc_pwr_on = true;

        _PRINTK(KERN_INFO "VC power on\n");
    }

    mutex_unlock(&_vc_mtx);
#endif
}

void vc_power_off(void){
#if	PWR_CONTROL_EN
#if CHIP_VERSION_CHK
    /* No support for C3V-W version A */
    if(_chip_version_get() == 0xa30){
        return;
    }
#endif
    /* No regulator, return */
    if(!_vc_regl || !_vc_iso_regl){
        return;
    }

    mutex_lock(&_vc_mtx);

    if(atomic_read(&_vc_atom)
        && atomic_dec_and_test(&_vc_atom)){

        /* VC HW clock disable */
        _vc_clk_control(0);

        /* VC HW Reset assert */
        _vc_reset_control(0);

        /* enable VC ISO (Register G36. ISO_CTRL_ENABLE [5]) */
        _vc_iso_control(0);

        /* VC Power off */
        _vc_pwr_control(0);

        _vc_pwr_on = false;

        _PRINTK(KERN_INFO "VC power off\n");
    }

    mutex_unlock(&_vc_mtx);
#endif
}

bool vc_power_is_on(void){
    return _vc_pwr_on;
}

int vc_power_ctrl_init(struct platform_device *dev, struct reset_control *rstc, struct clk *clk){
    int i = 0;
#if PWR_CONTROL_EN
    printk(KERN_INFO "chip version: 0x%x\n", _chip_version_get());
#if CHIP_VERSION_CHK
    if(_chip_version != 0xa30){
        printk(KERN_INFO "VC power control enable\n");
    }
    else{
        printk(KERN_INFO "VC power control unsupport\n");
    }
#endif
#endif
    _vc_clk = clk;
    if (IS_ERR(_vc_clk)) {
        dev_err(&dev->dev, "can't find clock source\n");
        return PTR_ERR(_vc_clk);
    }

    _vc_rstc = rstc;
    if (IS_ERR(_vc_rstc)) {
        dev_err(&dev->dev, "can't find reset control\n");
        return PTR_ERR(_vc_rstc);
    }

    return 0;
}

int vc_power_ctrl_init_dec(struct platform_device *dev, struct reset_control *rstc, struct clk *clk){

    _vc_dec_clk = clk;
    if (IS_ERR(_vc_dec_clk)) {
    dev_err(&dev->dev, "can't find clock source\n");
        return PTR_ERR(_vc_dec_clk);
    }

    _vc_dec_rstc = rstc;
    if (IS_ERR(_vc_dec_rstc)) {
        dev_err(&dev->dev, "can't find reset control\n");
        return PTR_ERR(_vc_dec_rstc);
    }

    return 0;
}

int vc_power_ctrl_init_enc(struct platform_device *dev, struct reset_control *rstc, struct clk *clk){

    _vc_enc_clk = clk;
    if (IS_ERR(_vc_enc_clk)) {
        dev_err(&dev->dev, "can't find clock source\n");
        return PTR_ERR(_vc_enc_clk);
    }

    _vc_enc_rstc = rstc;
    if (IS_ERR(_vc_enc_rstc)) {
        dev_err(&dev->dev, "can't find reset control\n");
        return PTR_ERR(_vc_enc_rstc);
    }

    return 0;
}

int vc_power_ctrl_terminate(void){
  
    _vc_regl = NULL;
    _vc_iso_regl = NULL;

    return 0;
}

