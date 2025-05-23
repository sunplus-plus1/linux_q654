/*!
 * vicore_stereo_tuningfs.h - stereo tuning system for stereo hardware
 * @file vicore_stereo_tuningfs.h
 * @brief  stereo tuning file system for stereo hardware
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

#ifndef VICORE_STEREO_TUNINGFS_H
#define VICORE_STEREO_TUNINGFS_H

extern int vicore_stereo_tuningfs_init(struct kobject *stereo_kobj, struct clk *clk_gate);

#endif /* VICORE_STEREO_TUNINGFS_H */
