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

#ifndef VICROE_STEREO_PM_H
#define VICROE_STEREO_PM_H

#include <linux/platform_device.h>

extern int vicore_stereo_suspend_ops(struct device *dev);
extern int vicore_stereo_resume_ops(struct device *dev);
extern int vicore_stereo_runtime_suspend(struct device *dev);
extern int vicore_stereo_runtime_resume(struct device *dev);
#endif
