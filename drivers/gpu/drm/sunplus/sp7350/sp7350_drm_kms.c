// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM Mode Setting
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_probe_helper.h>

#include "sp7350_drm_drv.h"

/* -----------------------------------------------------------------------------
 * Format helpers
 */
#if 0 /* TODO:NOT SUPPORT ATOMIC COMMIT NOW! */
static void sp7350_drm_atomic_commit_tail(struct drm_atomic_state *old_state)
{
	struct drm_device *dev = old_state->dev;
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state;
	int i;

	drm_atomic_helper_commit_modeset_disables(dev, old_state);

	drm_atomic_helper_commit_planes(dev, old_state, 0);

	drm_atomic_helper_commit_modeset_enables(dev, old_state);

	drm_atomic_helper_fake_vblank(old_state);

	drm_atomic_helper_commit_hw_done(old_state);

	drm_atomic_helper_wait_for_flip_done(dev, old_state);

	//for_each_old_crtc_in_state(old_state, crtc, old_crtc_state, i) {
	//	struct vkms_crtc_state *vkms_state =
	//		to_vkms_crtc_state(old_crtc_state);

	//	flush_work(&vkms_state->composer_work);
	//}

	drm_atomic_helper_cleanup_planes(dev, old_state);
}
#endif
static const struct drm_mode_config_funcs sp7350_drm_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	//.atomic_commit_tail = sp7350_drm_atomic_commit_tail,
};

int sp7350_drm_modeset_init(struct sp7350_drm_device *sdev)
{
	int ret;

	ret = drmm_mode_config_init(sdev->ddev);
	if (ret)
		return ret;

	//drm_kms_helper_poll_init(sdev->ddev);

	sdev->ddev->mode_config.min_width = XRES_MIN;
	sdev->ddev->mode_config.min_height = YRES_MIN;
	sdev->ddev->mode_config.max_width = XRES_MAX;
	sdev->ddev->mode_config.max_height = YRES_MAX;
	sdev->ddev->mode_config.funcs = &sp7350_drm_mode_config_funcs;
	//sdev->ddev->mode_config.helper_private = &vkms_mode_config_helpers;

	return sp7350_drm_output_init(sdev, 0);

	return 0;
}
