/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM CRTCs
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

#include "sp7350_drm_drv.h"

static const struct drm_crtc_funcs sp7350_drm_crtc_funcs = {
	.set_config             = drm_atomic_helper_set_config,
	.destroy                = drm_crtc_cleanup,
	.page_flip              = drm_atomic_helper_page_flip,
	//.reset                  = vkms_atomic_crtc_reset,
	//.atomic_duplicate_state = vkms_atomic_crtc_duplicate_state,
	//.atomic_destroy_state   = vkms_atomic_crtc_destroy_state,
	//.enable_vblank		= vkms_enable_vblank,
	//.disable_vblank		= vkms_disable_vblank,
	//.get_vblank_timestamp	= vkms_get_vblank_timestamp,
	//.get_crc_sources	= vkms_get_crc_sources,
	//.set_crc_source		= vkms_set_crc_source,
	//.verify_crc_source	= vkms_verify_crc_source,
};

static int sp7350_drm_crtc_atomic_check(struct drm_crtc *crtc,
				  struct drm_crtc_state *state)
{
	/* TODO reference to vkms_crtc_atomic_check */
	return 0;
}

static void sp7350_drm_crtc_atomic_enable(struct drm_crtc *crtc,
					struct drm_crtc_state *old_state)
{
	drm_crtc_vblank_on(crtc);
}

static void sp7350_drm_crtc_atomic_disable(struct drm_crtc *crtc,
					 struct drm_crtc_state *old_state)
{
	drm_crtc_vblank_off(crtc);
}

static void sp7350_drm_crtc_atomic_begin(struct drm_crtc *crtc,
				   struct drm_crtc_state *old_crtc_state)
{
	/* TODO reference to vkms_crtc_atomic_begin */
}

static void sp7350_drm_crtc_atomic_flush(struct drm_crtc *crtc,
				   struct drm_crtc_state *old_crtc_state)
{
	/* TODO reference to vkms_crtc_atomic_flush */
}

static const struct drm_crtc_helper_funcs sp7350_drm_crtc_helper_funcs = {
	.atomic_check	= sp7350_drm_crtc_atomic_check,
	.atomic_begin	= sp7350_drm_crtc_atomic_begin,
	.atomic_flush	= sp7350_drm_crtc_atomic_flush,
	.atomic_enable	= sp7350_drm_crtc_atomic_enable,
	.atomic_disable	= sp7350_drm_crtc_atomic_disable,
};

int sp7350_drm_crtc_init(struct drm_device *dev, struct drm_crtc *crtc,
		   struct drm_plane *primary, struct drm_plane *cursor)
{
	//struct vkms_output *vkms_out = drm_crtc_to_vkms_output(crtc);
	int ret;

	ret = drm_crtc_init_with_planes(dev, crtc, primary, cursor,
					&sp7350_drm_crtc_funcs, NULL);
	if (ret) {
		DRM_ERROR("Failed to init CRTC\n");
		return ret;
	}

	drm_crtc_helper_add(crtc, &sp7350_drm_crtc_helper_funcs);

	//spin_lock_init(&vkms_out->lock);
	//spin_lock_init(&vkms_out->composer_lock);

	//vkms_out->composer_workq = alloc_ordered_workqueue("vkms_composer", 0);
	//if (!vkms_out->composer_workq)
	//	return -ENOMEM;

	return ret;
}

