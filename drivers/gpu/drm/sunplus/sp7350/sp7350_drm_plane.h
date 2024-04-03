/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM Planes
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#ifndef __SUNPLUS_SP7350_DRM_PLANE_H__
#define __SUNPLUS_SP7350_DRM_PLANE_H__
#include <drm/drm.h>

#define SP7350_MAX_PLANE  5

#define SP7350_DRM_PLANE_CAP_SCALE      (1 << 0)
#define SP7350_DRM_PLANE_CAP_ZPOS       (1 << 1)
#define SP7350_DRM_PLANE_CAP_ROTATION   (1 << 2)
#define SP7350_DRM_PLANE_CAP_PIX_BLEND           (1 << 3)
#define SP7350_DRM_PLANE_CAP_WIN_BLEND           (1 << 4)
#define SP7350_DRM_PLANE_CAP_REGION_BLEND        (1 << 5)
#define SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING (1 << 6)
#define SP7350_DRM_PLANE_CAP_COLOR_KEYING        (1 << 7)

struct sp7350_plane_region_alpha_info {
	int regionid;
	int alpha;
};

struct sp7350_plane_region_color_keying_info {
	int regionid;
	int keying;
};

struct sp7350_drm_plane_state {
	struct sp7350_plane_region_alpha_info region_alpha;
	struct sp7350_plane_region_color_keying_info region_color_keying;
	unsigned int color_keying;
	struct drm_property_blob *region_alpha_blob;
	struct drm_property_blob *region_color_keying_blob;
};

struct sp7350_drm_plane {
	struct drm_plane base;
	enum drm_plane_type type;
	const uint32_t *pixel_formats;
	unsigned int num_pixel_formats;
	const struct drm_plane_helper_funcs *funcs;
	unsigned int capabilities;
	unsigned int zpos;
	unsigned int index;
	bool is_media_plane;
	struct drm_property *region_alpha_property;
	struct drm_property *region_color_keying_property;
	struct drm_property *color_keying_property;

	struct sp7350_drm_plane_state state;
};

#define to_sp7350_drm_plane(target)\
	container_of(target, struct sp7350_drm_plane, base)

int sp7350_plane_create_primary_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane);
int sp7350_plane_create_media_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane);
int sp7350_plane_create_overlay_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane, int index);
int sp7350_plane_release_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane);

//int sp7350_plane_create_additional_planes(struct drm_device *drm);

#endif /* __SUNPLUS_SP7350_DRM_PLANE_H__ */
