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

static const struct drm_mode_config_funcs sp7350_drm_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
};

int sp7350_drm_modeset_init(struct sp7350_drm_device *sdev)
{
	int ret;

	ret = drmm_mode_config_init(sdev->ddev);
	if (ret)
		return ret;

	//drm_kms_helper_poll_init(sdev->ddev);

	sdev->ddev->mode_config.min_width = 0;
	sdev->ddev->mode_config.min_height = 0;
	sdev->ddev->mode_config.max_width = 4095;
	sdev->ddev->mode_config.max_height = 4095;
	sdev->ddev->mode_config.funcs = &sp7350_drm_mode_config_funcs;
	//sdev->ddev->mode_config.helper_private = &vkms_mode_config_helpers;

	return sp7350_drm_output_init(sdev, 0);

	return 0;
}
