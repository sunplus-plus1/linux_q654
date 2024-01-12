// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM Mode Setting
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#ifndef __SP7350_DRM_KMS_H__
#define __SP7350_DRM_KMS_H__

#include <linux/types.h>

struct sp7350_drm_device;


int sp7350_drm_modeset_init(struct sp7350_drm_device *sdev);

#endif /* __SP7350_DRM_KMS_H__ */
