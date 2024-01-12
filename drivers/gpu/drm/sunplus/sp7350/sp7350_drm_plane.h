/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM Planes
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#ifndef __SUNPLUS_SP7350_DRM_PLANE_H__
#define __SUNPLUS_SP7350_DRM_PLANE_H__
#include <drm/drm.h>

struct sp7350_drm_device;


struct drm_plane *sp7350_drm_plane_init(struct sp7350_drm_device *vkmsdev,
				  enum drm_plane_type type, int index);


#endif /* __SUNPLUS_SP7350_DRM_PLANE_H__ */
