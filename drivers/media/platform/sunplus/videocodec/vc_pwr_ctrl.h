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

#ifndef __VC_PWR_CTRL__
#define __VC_PWR_CTRL__

#include <linux/platform_device.h>

void vc_power_on(void);
void vc_power_off(void);
bool vc_power_is_on(void);
void vc_regulator_control(struct platform_device *dev, int ctrl);
int  vc_power_ctrl_init(struct platform_device *dev, struct reset_control *rstc, struct clk *clk);
int  vc_power_ctrl_init_enc(struct platform_device *dev, struct reset_control *rstc, struct clk *clk);
int  vc_power_ctrl_init_dec(struct platform_device *dev, struct reset_control *rstc, struct clk *clk);
int  vc_power_ctrl_terminate(void);


#endif /* !__VC_PWR_CTRL__ */

