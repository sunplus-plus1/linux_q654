// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM Planes
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_plane_helper.h>

#include "sp7350_drm_drv.h"

//#include "sp7350_display.h"
#include <media/sunplus/disp/sp7350/sp7350_disp_osd.h>
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_vpp.h"
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_dmix.h"

/* sp7350-hw-format translate table */
struct sp7350_plane_format {
	u32 pixel_format;
	u32 hw_format;
};

static const uint32_t sp7350_kms_formats[] = {
	DRM_FORMAT_UYVY,  /* SP7350_VPP_IMGREAD_DATA_FMT_UYVY */
	DRM_FORMAT_YUYV,  /* SP7350_VPP_IMGREAD_DATA_FMT_YUY2 ??? */
	DRM_FORMAT_NV12,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV12 */
	DRM_FORMAT_NV16,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV16 */
	DRM_FORMAT_NV24,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV24 */
};

static const uint32_t sp7350_kms_osd_formats[] = {
	DRM_FORMAT_RGB565,    /* SP7350_OSD_COLOR_MODE_RGB565 */
	DRM_FORMAT_RGBA8888,  /* SP7350_OSD_COLOR_MODE_RGBA8888 */
	DRM_FORMAT_ARGB8888,  /* SP7350_OSD_COLOR_MODE_ARGB8888 */
	DRM_FORMAT_RGBA4444,  /* SP7350_OSD_COLOR_MODE_RGBA4444 */
	DRM_FORMAT_ARGB4444,  /* SP7350_OSD_COLOR_MODE_ARGB4444 */
	DRM_FORMAT_ARGB1555,  /* SP7350_OSD_COLOR_MODE_ARGB1555 */
	DRM_FORMAT_YUYV,      /* SP7350_OSD_COLOR_MODE_YUY2 ??? */
	DRM_FORMAT_R8,        /* SP7350_OSD_COLOR_MODE_8BPP ??? */
};

static const struct sp7350_plane_format sp7350_vpp_formats[] = {
	{ DRM_FORMAT_UYVY, SP7350_VPP_IMGREAD_DATA_FMT_UYVY },
	{ DRM_FORMAT_YUYV, SP7350_VPP_IMGREAD_DATA_FMT_YUY2 },
	{ DRM_FORMAT_NV12, SP7350_VPP_IMGREAD_DATA_FMT_NV12 },
	{ DRM_FORMAT_NV16, SP7350_VPP_IMGREAD_DATA_FMT_NV16 },
	{ DRM_FORMAT_NV24, SP7350_VPP_IMGREAD_DATA_FMT_NV24 },
};

static const struct sp7350_plane_format sp7350_osd_formats[] = {
	{ DRM_FORMAT_R8, SP7350_OSD_COLOR_MODE_8BPP },
	/* 16bpp RGB: */
	{ DRM_FORMAT_RGB565, SP7350_OSD_COLOR_MODE_RGB565 },
	{ DRM_FORMAT_ARGB1555, SP7350_OSD_COLOR_MODE_ARGB1555 },
	{ DRM_FORMAT_YUYV,   SP7350_OSD_COLOR_MODE_YUY2 },
	{ DRM_FORMAT_RGBA4444, SP7350_OSD_COLOR_MODE_RGBA4444 },
	{ DRM_FORMAT_ARGB4444, SP7350_OSD_COLOR_MODE_ARGB4444 },
	/* 32bpp [A]RGB: */
	{ DRM_FORMAT_RGBA8888, SP7350_OSD_COLOR_MODE_RGBA8888 },
	{ DRM_FORMAT_ARGB8888, SP7350_OSD_COLOR_MODE_ARGB8888 },
};
#define SP7350_FORMAT_UNSUPPORT 0xF

/* convert from fourcc format to sp7530 vpp/osd format */
static u32 sp7350_get_format(u32 pixel_format, enum drm_plane_type type)
{
	int i;

	if (type == DRM_PLANE_TYPE_PRIMARY) {
		for (i = 0; i < ARRAY_SIZE(sp7350_vpp_formats); i++)
			if (sp7350_vpp_formats[i].pixel_format == pixel_format)
				return sp7350_vpp_formats[i].hw_format;
	}
	else {
		for (i = 0; i < ARRAY_SIZE(sp7350_osd_formats); i++)
			if (sp7350_osd_formats[i].pixel_format == pixel_format)
				return sp7350_osd_formats[i].hw_format;
	}

	/* not found */
	DRM_ERROR("Not found pixel format!!fourcc_format= %d\n",
		  pixel_format);
	return SP7350_FORMAT_UNSUPPORT;
}

static const struct drm_plane_funcs sp7350_drm_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static void sp7350_kms_plane_atomic_update(struct drm_plane *plane,
					 struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct drm_gem_cma_object *obj = NULL;

	/* reference to ade_plane_atomic_update */
	DRM_INFO("%s\n", __func__);
	/* get data_addr1 for HW */
	//u32 stride = state->fb->pitches[0];
	//u32 addr = (u32)obj->paddr + (state->src_y >> 16) * stride;

	int src_x,src_y,src_w,src_h;
	src_x = state->src_x >> 16;
	src_y = state->src_y >> 16;
	src_w = state->src_w >> 16;
	src_h = state->src_h >> 16;

	
	DRM_INFO("%s crtc_x:%d\n", __func__,state->crtc_x);
	DRM_INFO("%s crtc_y:%d\n", __func__,state->crtc_y);
	DRM_INFO("%s crtc_w:%d\n", __func__,state->crtc_w);
	DRM_INFO("%s crtc_h:%d\n", __func__,state->crtc_h);
	DRM_INFO("%s src_x:%d\n", __func__,src_x);
	DRM_INFO("%s src_y:%d\n", __func__,src_y);
	DRM_INFO("%s src_w:%d\n", __func__,src_w);
	DRM_INFO("%s src_h:%d\n", __func__,src_h);
	DRM_INFO("%s plane->type:%d\n", __func__,plane->type);
	DRM_INFO("%s src_h:%d\n", __func__,src_h);
	
	if (!state->fb) {
		/* do nothing */
		DRM_INFO("%s fb is null\n", __func__);
		return;
	}

	obj = drm_fb_cma_get_gem_obj(state->fb, 0);
	if (!obj || !obj->paddr) {
		DRM_ERROR("drm_fb_cma_get_gem_obj fail.\n");
		return;
	}


	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		/* FOR VPP Layer */

		/* FIXME, for test only!!!
		   dmix_layer_init should be called at driver binding or probe,
		   but now sp7350 drm must run with display driver.
		 */

		/*
		sp7350_dmix_layer_set(SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
		sp7350_dmix_layer_set(SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
		sp7350_dmix_layer_set(SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
		sp7350_dmix_layer_set(SP7350_DMIX_OSD3, SP7350_DMIX_TRANSPARENT);
		*/
		sp7350_dmix_layer_set(SP7350_DMIX_VPP0, SP7350_DMIX_BLENDING);

		sp7350_vpp_imgread_set((u32)obj->paddr,
				src_x, src_y,
				src_w, src_h,
				sp7350_get_format(state->fb->format->format, plane->type));
		sp7350_vpp_vscl_set(src_x, src_y,
				src_w, src_h,
				state->crtc_w, state->crtc_h,
				1920, 1080,
				state->crtc_x, state->crtc_y);
				
				
	}
	else {
		/* FOR OSD Layer */
		struct sp7350fb_info info;

		info.color_mode = sp7350_get_format(state->fb->format->format, plane->type);
		info.width = state->src_w >> 16;
		info.height = state->src_h >> 16;
		info.buf_addr_phy = (u32)obj->paddr;

		sp7350_osd_layer_set(&info, plane->index -1);

		/* FIXME, for test only!!! */
		sp7350_dmix_layer_set(SP7350_DMIX_OSD0 + plane->index -1, SP7350_DMIX_BLENDING);
		/**
		DRM_INFO("%s plane->index:%d\n", __func__,plane->index);
		struct sp7350_dmix_plane_alpha dmix_plane;
		dmix_plane.layer = SP7350_DMIX_L6 - plane->index -1;
		dmix_plane.enable = 1;
		dmix_plane.fix_alpha = 1;
		dmix_plane.alpha_value = 50;

		sp7350_dmix_plane_alpha_config(&dmix_plane);*/

	}
}

static int sp7350_kms_plane_atomic_check(struct drm_plane *plane,
				   struct drm_plane_state *state)
{
	struct drm_crtc_state *crtc_state;
	bool can_position = false;
	int ret;

	DRM_INFO("%s\n", __func__);

	if (!state->fb || WARN_ON(!state->crtc)){
		DRM_INFO("%s ddd return 0\n", __func__);
		return 0;
	}

	DRM_INFO("%s start  get0\n", __func__);



	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state)){
		DRM_INFO("%s is err\n", __func__);
		return PTR_ERR(crtc_state);

	}
	if (plane->type == DRM_PLANE_TYPE_CURSOR)
		can_position = true;
	DRM_INFO("%s return 0\n", __func__);
	return 0;

	/**ret = drm_atomic_helper_check_plane_state(state, crtc_state,
						  DRM_PLANE_HELPER_NO_SCALING,
						  DRM_PLANE_HELPER_NO_SCALING,
						  can_position, true);

	DRM_INFO("%s return %d \n", __func__,ret);
	if (ret != 0)
		return ret;

	/* for now primary plane must be visible and full screen */

	/**
	if (!state->visible && !can_position)
		return -EINVAL;
	DRM_INFO("%s return 0 \n", __func__);

	return 0;
	*/
}

#if 0
static int sp7350_kms_prepare_fb(struct drm_plane *plane,
			   struct drm_plane_state *state)
{
	struct drm_gem_object *gem_obj;
	int ret;

	if (!state->fb)
		return 0;

	gem_obj = drm_gem_fb_get_obj(state->fb, 0);
	ret = sp7350_vgem_vmap(gem_obj);
	if (ret)
		DRM_ERROR("vmap failed: %d\n", ret);

	return drm_gem_fb_prepare_fb(plane, state);
}

static void sp7350_kms_cleanup_fb(struct drm_plane *plane,
				struct drm_plane_state *old_state)
{
	struct drm_gem_object *gem_obj;

	if (!old_state->fb)
		return;

	gem_obj = drm_gem_fb_get_obj(old_state->fb, 0);
	sp7350_vgem_vunmap(gem_obj);
}
#endif

static const struct drm_plane_helper_funcs sp7350_kms_primary_helper_funcs = {
	.atomic_update		= sp7350_kms_plane_atomic_update,
	.atomic_check		=sp7350_kms_plane_atomic_check,
	//.prepare_fb 	= sp7350_kms_prepare_fb,
	//.cleanup_fb 	= sp7350_kms_cleanup_fb,
};

struct drm_plane *sp7350_drm_plane_init(struct drm_device *drm,
				  enum drm_plane_type type, int index)
{
	const struct drm_plane_helper_funcs *funcs;
	struct drm_plane *plane;
	const u32 *formats;
	int ret, nformats;

	plane = kzalloc(sizeof(*plane), GFP_KERNEL);
	if (!plane)
		return ERR_PTR(-ENOMEM);

	if (type == DRM_PLANE_TYPE_PRIMARY) {
		formats = sp7350_kms_formats;
		nformats = ARRAY_SIZE(sp7350_kms_formats);
		funcs = &sp7350_kms_primary_helper_funcs;
	} else {
		formats = sp7350_kms_osd_formats;
		nformats = ARRAY_SIZE(sp7350_kms_osd_formats);
		funcs = &sp7350_kms_primary_helper_funcs;
	}

	ret = drm_universal_plane_init(drm, plane, 1 << index,
					   &sp7350_drm_plane_funcs,
					   formats, nformats,
					   NULL, type, NULL);
	if (ret) {
		kfree(plane);
		return ERR_PTR(ret);
	}

	drm_plane_helper_add(plane, funcs);

	return plane;
}

int sp7350_plane_create_additional_planes(struct drm_device *drm)
{
	//struct drm_plane *cursor_plane;
	//struct drm_crtc *crtc;
	unsigned int i;

	/* for c3v soc display controller, has 4 osd layer.
	 * used for DRM OVERLAY???
	 */
	for (i = 0; i < 4; i++) {
		struct drm_plane *plane =
		sp7350_drm_plane_init(drm, DRM_PLANE_TYPE_OVERLAY, i);

		if (IS_ERR(plane))
			continue;

		plane->possible_crtcs = GENMASK(drm->mode_config.num_crtc - 1, 0);
	}

#if 0  /* TODO, NOT SUPPORT cursor */
	drm_for_each_crtc(crtc, drm) {
		/* Set up the legacy cursor after overlay initialization,
		* since we overlay planes on the CRTC in the order they were
		* initialized.
		*/
		cursor_plane = sp7350_drm_plane_init(drm, DRM_PLANE_TYPE_CURSOR, 0);
		if (!IS_ERR(cursor_plane)) {
			cursor_plane->possible_crtcs = drm_crtc_mask(crtc);
			crtc->cursor = cursor_plane;
		}
	}
#endif

	return 0;
}

