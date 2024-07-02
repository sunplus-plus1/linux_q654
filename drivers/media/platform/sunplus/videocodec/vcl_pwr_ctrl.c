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

#include "vcl_pwr_ctrl.h"

#define _DEBUG 0
#if _DEBUG
#define _PRINTK printk
#else
#define _PRINTK(text, ...){ }
#endif

#define _PWR_CONTROL_EN		1

#define CHIP_VERSION_REG 	0xF8800000

static void __iomem *_chip_version_reg_base = NULL;
static void __iomem *_vcl_iso_reg_base = NULL;

static int _chip_version = 0x0;

/* power is configure to default on in dts */
static atomic_t _vcl_atom = ATOMIC_INIT(1);
static DEFINE_MUTEX(_vcl_mtx);

/* vcl video codec */
static struct regulator *_vcl_regl = NULL;

static struct clk *_vcl_clk = NULL;
static struct reset_control *_vcl_rstc;

/* vcl decode */
static struct clk *_vcl_dec_clk=NULL;
static struct reset_control *_vcl_dec_rstc;

/* vcl encode */
static struct clk *_vcl_enc_clk = NULL;
static struct reset_control *_vcl_enc_rstc;

static void _vcl_iso_control(int enable){
	
	unsigned int reg_read_value;

	if (IS_ERR(_vcl_iso_reg_base)) {
		printk(KERN_ERR "failed to get iso base\n");
        return;
	}

	reg_read_value = readl(_vcl_iso_reg_base);

	if (enable)
		reg_read_value = reg_read_value|0x20;
	else
		reg_read_value = reg_read_value&0xFFFFFFDF;
	writel(reg_read_value, _vcl_iso_reg_base);

	_PRINTK(KERN_DEBUG "VCL ISO %s\n", (enable?"enable":"disable"));
}

static void _vcl_reset_control(int deassert){

	if (IS_ERR(_vcl_dec_rstc)) {
		printk(KERN_ERR "can't find decode reset control\n");
		return;
	}

	if (IS_ERR(_vcl_enc_rstc)) {
		printk(KERN_ERR "can't find encode reset control\n");
		return;
	}

	if (IS_ERR(_vcl_rstc)) {
		printk(KERN_ERR "can't find vc reset control\n");
		return;
	}

	if(deassert){
		if(reset_control_deassert(_vcl_dec_rstc)){
			printk(KERN_ERR "failed to deassert vc dec reset line\n");
			return;
		}
		if(reset_control_deassert(_vcl_enc_rstc)){
			printk(KERN_ERR "failed to deassert vc enc reset line\n");
			return;
		}
		if(reset_control_deassert(_vcl_rstc)){
			printk(KERN_ERR "failed to deassert vcl reset line\n");
			return;
		}
		_PRINTK(KERN_DEBUG "VCL HW reset deassert\n");
	}	
	else{
		if(reset_control_assert(_vcl_rstc)){
			printk(KERN_ERR "failed to assert vc  reset line\n");
			return;
		}
		if(reset_control_assert(_vcl_enc_rstc)){
			printk(KERN_ERR "failed to assert vc enc reset line\n");
			return;
		}
		if(reset_control_assert(_vcl_dec_rstc)){
			printk(KERN_ERR "failed to assert dec reset line\n");
			return;
		}
		_PRINTK(KERN_DEBUG "VCL HW reset assert\n");
	}
}

static void _vcl_clk_control(int enable){
	if (IS_ERR(_vcl_enc_clk)) {
		printk(KERN_ERR "can't find encode clock source\n");
		return;
	}

	if (IS_ERR(_vcl_dec_clk)) {
		printk(KERN_ERR "can't find decode clock source\n");
		return;
	}

	if (IS_ERR(_vcl_clk)) {
		printk(KERN_ERR "can't find vc clock source\n");
		return;
	}

	if(enable){
		clk_enable(_vcl_enc_clk);
		clk_enable(_vcl_dec_clk);
		clk_enable(_vcl_clk);

		_PRINTK(KERN_DEBUG "VCL HW clock enable\n");
	}
	else{
		clk_disable(_vcl_clk);
		clk_disable(_vcl_dec_clk);
		clk_disable(_vcl_enc_clk);

		_PRINTK(KERN_DEBUG "VCL HW clock disale\n");
	}
}

static void _vcl_pwr_control(int enable){

	int ret = 0;

	if (IS_ERR(_vcl_regl)) {
		printk(KERN_NOTICE "failed to get regulator\n");
		return;
	}
	
	if(enable){
		if(!regulator_is_enabled(_vcl_regl)) {
			ret = regulator_enable(_vcl_regl);
			if (ret != 0){ 
				printk(KERN_ERR "regulator enable failed: %d\n", ret);
				return;
			}	
			_PRINTK(KERN_DEBUG "VCL regulator enable %s\n", 
				regulator_is_enabled(_vcl_regl)?"success":"failed");

			mdelay(50);
		}
	}
	else{
		if(regulator_is_enabled(_vcl_regl)) {			
			ret = regulator_disable(_vcl_regl);
			if (ret != 0){
				printk(KERN_ERR "regulator disable failed: %d\n", ret);
				return;
			}
			_PRINTK(KERN_DEBUG "VCL regulator disable %s\n", 
				regulator_is_enabled(_vcl_regl)?"failed":"success");
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

void vcl_regulator_control(struct platform_device *dev, int ctrl){
	if(!_vcl_regl){
		_vcl_regl = devm_regulator_get(&dev->dev, "video_codec");

		if(IS_ERR(_vcl_regl)) {
			dev_info(&dev->dev, "failed to get regulator\n");	
			return;
		}	

		/* To avoid regulator late cleanup */
		if(regulator_is_enabled(_vcl_regl)){
			regulator_enable(_vcl_regl);
		}
	}	
#if _PWR_CONTROL_EN
	/* No support for C3V-W version A */
	if(_chip_version_get() == 0xa30){
		return;
	}

	if(ctrl > -1){
		_vcl_pwr_control(ctrl);		
	}
#endif	
}

void vcl_power_on(void){
#if _PWR_CONTROL_EN
	/* No support for C3V-W version A */
	if(_chip_version_get() == 0xa30){
		return;
	}

	/* No regulator, return */
	if(!_vcl_regl){
		return;
	}

	mutex_lock(&_vcl_mtx);

	atomic_inc(&_vcl_atom);

	if(atomic_read(&_vcl_atom) == 1){
		/* VCL power on */
		_vcl_pwr_control(1);

		/* Disable VCL ISO (Register G36. ISO_CTRL_ENABLE [5]) */
		_vcl_iso_control(0);

		/* VCL HW reset deassert */
		_vcl_reset_control(1);

		/* VCL HW clock enable */
		_vcl_clk_control(1);

		_PRINTK(KERN_INFO "VCL power on\n");
	}

	mutex_unlock(&_vcl_mtx);
#endif	
}

void vcl_power_off(void){
#if	_PWR_CONTROL_EN
	/* No support for C3V-W version A */
	if(_chip_version_get() == 0xa30){
		return;
	}

	/* No regulator, return */
	if(!_vcl_regl){
		return;
	}
	
	mutex_lock(&_vcl_mtx);
	
	if(atomic_read(&_vcl_atom) 
		&& atomic_dec_and_test(&_vcl_atom)){

		/* VCL HW clock disable */
		_vcl_clk_control(0);

		/* VCL HW Reset assert */
		_vcl_reset_control(0);

		/* enable VCL ISO (Register G36. ISO_CTRL_ENABLE [5]) */
		_vcl_iso_control(1);

		/* VCL Power off */
		_vcl_pwr_control(0);
		
		_PRINTK(KERN_INFO "VCL power off\n");		
	}
		
	mutex_unlock(&_vcl_mtx);
#endif		
}

int vcl_power_ctrl_init(struct platform_device *dev, struct reset_control *rstc, struct clk *clk){
#if	_PWR_CONTROL_EN
	printk(KERN_INFO "chip version: 0x%x\n", _chip_version_get());
	if(_chip_version != 0xa30){
		printk(KERN_INFO "VCL power control enable\n");
	}
	else{
		printk(KERN_INFO "VCL power control unsupport\n");
	}
#endif
	_vcl_clk = clk;
	if (IS_ERR(_vcl_clk)) {
		dev_err(&dev->dev, "can't find clock source\n");
        return PTR_ERR(_vcl_clk);
	}

	_vcl_rstc = rstc;
	if (IS_ERR(_vcl_rstc)) {
		dev_err(&dev->dev, "can't find reset control\n");
        return PTR_ERR(_vcl_rstc);
	}	

	_vcl_iso_reg_base = of_iomap((&dev->dev)->of_node, 0);
	if (!_vcl_iso_reg_base) {
		dev_err(&dev->dev, "failed to get iso base\n");
		return -1;
	}
	
	return 0;
}

int vcl_power_ctrl_init_dec(struct platform_device *dev, struct reset_control *rstc, struct clk *clk){
	
	_vcl_dec_clk = clk;
	if (IS_ERR(_vcl_dec_clk)) {
		dev_err(&dev->dev, "can't find clock source\n");
        return PTR_ERR(_vcl_dec_clk);
	}
	
	_vcl_dec_rstc = rstc;
	if (IS_ERR(_vcl_dec_rstc)) {
		dev_err(&dev->dev, "can't find reset control\n");
		return PTR_ERR(_vcl_dec_rstc);
	}	

	return 0;
}

int vcl_power_ctrl_init_enc(struct platform_device *dev, struct reset_control *rstc, struct clk *clk){

	_vcl_enc_clk = clk;
	if (IS_ERR(_vcl_enc_clk)) {
		dev_err(&dev->dev, "can't find clock source\n");
        return PTR_ERR(_vcl_enc_clk);
	}

	_vcl_enc_rstc = rstc;
	if (IS_ERR(_vcl_enc_rstc)) {
		dev_err(&dev->dev, "can't find reset control\n");
        return PTR_ERR(_vcl_enc_rstc);
	}	

	return 0;
}

int vcl_power_ctrl_terminate(void){

	if(_vcl_iso_reg_base){
		iounmap(_vcl_iso_reg_base);
		_vcl_iso_reg_base = NULL;
	}

	_vcl_regl = NULL;

	return 0;
}

