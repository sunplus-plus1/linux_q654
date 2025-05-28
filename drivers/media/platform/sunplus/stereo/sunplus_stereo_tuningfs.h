/*!
 * sunplus_stereo_tuningfs.h - stereo tuning system for stereo hardware
 * @file sunplus_stereo_tuningfs.h
 * @brief  stereo tuning file system for stereo hardware
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

#ifndef SUNPLUS_STEREO_TUNINGFS_H
#define SUNPLUS_STEREO_TUNINGFS_H

extern int sunplus_stereo_tuningfs_init(struct kobject *stereo_kobj, struct clk *clk_gate);

#endif /* SUNPLUS_STEREO_TUNINGFS_H */
