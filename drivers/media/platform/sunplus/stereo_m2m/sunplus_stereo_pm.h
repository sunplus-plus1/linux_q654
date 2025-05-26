/*!
 * sunplus_stereo_pm.h - stereo function control system
 * @file sunplus_stereo_pm.h
 * @brief  stereo power control file system
 * @author Saxen Ko <saxen.ko@sunplus.com>
 * @version 1.0
 * @copyright  Copyright (C) 2025 Sunplus
 * @note
 * Copyright (C) 2025 Sunplus
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifndef SUNPLUS_STEREO_PM_H
#define SUNPLUS_STEREO_PM_H

#include <linux/platform_device.h>

extern int sunplus_stereo_suspend_ops(struct device *dev);
extern int sunplus_stereo_resume_ops(struct device *dev);
extern int sunplus_stereo_runtime_suspend(struct device *dev);
extern int sunplus_stereo_runtime_resume(struct device *dev);
#endif
