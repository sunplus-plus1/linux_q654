// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM Mode Setting
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <linux/clk.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_fourcc.h>

#include "sp7350_drm_drv.h"

static struct drm_framebuffer *
sp7350_drm_gem_fb_create(struct drm_device *dev, struct drm_file *file_priv,
			 const struct drm_mode_fb_cmd2 *mode_cmd)
{
	if (!drm_any_plane_has_format(dev,
				      mode_cmd->pixel_format,
				      mode_cmd->modifier[0])) {
		struct drm_format_name_buf format_name;

		DRM_DEV_DEBUG_DRIVER(dev->dev,
				     "unsupported pixel format %s / modifier 0x%llx\n",
				     drm_get_format_name(mode_cmd->pixel_format, &format_name),
				     mode_cmd->modifier[0]);
		return ERR_PTR(-EINVAL);
	}

	return drm_gem_fb_create(dev, file_priv, mode_cmd);
}

static const struct drm_mode_config_funcs sp7350_drm_mode_config_funcs = {
	.fb_create = sp7350_drm_gem_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

int sp7350_drm_modeset_init(struct drm_device *drm)
{
	int ret;

	/* Set support for vblank irq fast disable, before drm_vblank_init() */
	drm->vblank_disable_immediate = true;

	drm->irq_enabled = true;
	//drm->irq_enabled = false;
	ret = drm_vblank_init(drm, drm->mode_config.num_crtc);
	if (ret < 0) {
		DRM_DEV_ERROR(drm->dev, "failed to initialize vblank\n");
		return ret;
	}

	drm->mode_config.min_width = XRES_MIN;
	drm->mode_config.min_height = YRES_MIN;
	drm->mode_config.max_width = XRES_MAX;
	drm->mode_config.max_height = YRES_MAX;
	drm->mode_config.funcs = &sp7350_drm_mode_config_funcs;
	//drm->mode_config.helper_private = &vkms_mode_config_helpers;
	drm->mode_config.preferred_depth = 16;
	drm->mode_config.async_page_flip = true;
	drm->mode_config.allow_fb_modifiers = true;

	drm_mode_config_reset(drm);

	return 0;
}
