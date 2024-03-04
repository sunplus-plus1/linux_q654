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

static const uint32_t sp7350_kms_vpp_formats[] = {
	DRM_FORMAT_YUYV,  /* SP7350_VPP_IMGREAD_DATA_FMT_YUY2 */
	DRM_FORMAT_UYVY,  /* SP7350_VPP_IMGREAD_DATA_FMT_UYVY */
	DRM_FORMAT_NV12,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV12 */
	DRM_FORMAT_NV16,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV16 */
	DRM_FORMAT_NV24,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV24 */
};

static const uint32_t sp7350_kms_osd_formats[] = {
	DRM_FORMAT_C8,        /* SP7350_OSD_COLOR_MODE_8BPP ??? */
	DRM_FORMAT_YUYV,      /* SP7350_OSD_COLOR_MODE_YUY2 */
	DRM_FORMAT_RGB565,    /* SP7350_OSD_COLOR_MODE_RGB565 */
	DRM_FORMAT_RGBA4444,  /* SP7350_OSD_COLOR_MODE_RGBA4444 */
	DRM_FORMAT_ARGB4444,  /* SP7350_OSD_COLOR_MODE_ARGB4444 */
	DRM_FORMAT_ARGB1555,  /* SP7350_OSD_COLOR_MODE_ARGB1555 */
	DRM_FORMAT_RGBA8888,  /* SP7350_OSD_COLOR_MODE_RGBA8888 */
	DRM_FORMAT_ARGB8888,  /* SP7350_OSD_COLOR_MODE_ARGB8888 */
	//DRM_FORMAT_XRGB8888,  /* SP7350_OSD_COLOR_MODE_ARGB8888 */
	//DRM_FORMAT_RGBX8888,  /* SP7350_OSD_COLOR_MODE_RGBA8888 */
	//DRM_FORMAT_RGBX4444,  /* SP7350_OSD_COLOR_MODE_RGBA4444 */
	//DRM_FORMAT_XRGB4444,  /* SP7350_OSD_COLOR_MODE_ARGB4444 */
	//DRM_FORMAT_XRGB1555,  /* SP7350_OSD_COLOR_MODE_ARGB1555 */
};

static const struct sp7350_plane_format sp7350_vpp_formats[] = {
	{ DRM_FORMAT_YUYV, SP7350_VPP_IMGREAD_DATA_FMT_YUY2 },
	{ DRM_FORMAT_UYVY, SP7350_VPP_IMGREAD_DATA_FMT_UYVY },
	{ DRM_FORMAT_NV12, SP7350_VPP_IMGREAD_DATA_FMT_NV12 },
	{ DRM_FORMAT_NV16, SP7350_VPP_IMGREAD_DATA_FMT_NV16 },
	{ DRM_FORMAT_NV24, SP7350_VPP_IMGREAD_DATA_FMT_NV24 },
};

static const struct sp7350_plane_format sp7350_osd_formats[] = {
	{ DRM_FORMAT_C8, SP7350_OSD_COLOR_MODE_8BPP },
	/* 16bpp RGB: */
	{ DRM_FORMAT_RGB565, SP7350_OSD_COLOR_MODE_RGB565 },
	{ DRM_FORMAT_ARGB1555, SP7350_OSD_COLOR_MODE_ARGB1555 },
	{ DRM_FORMAT_YUYV,   SP7350_OSD_COLOR_MODE_YUY2 },
	{ DRM_FORMAT_RGBA4444, SP7350_OSD_COLOR_MODE_RGBA4444 },
	{ DRM_FORMAT_ARGB4444, SP7350_OSD_COLOR_MODE_ARGB4444 },
	/* 32bpp [A]RGB: */
	{ DRM_FORMAT_RGBA8888, SP7350_OSD_COLOR_MODE_RGBA8888 },
	{ DRM_FORMAT_ARGB8888, SP7350_OSD_COLOR_MODE_ARGB8888 },
	//{ DRM_FORMAT_XRGB1555, SP7350_OSD_COLOR_MODE_ARGB1555 },
	//{ DRM_FORMAT_RGBX4444, SP7350_OSD_COLOR_MODE_RGBA4444 },
	//{ DRM_FORMAT_XRGB4444, SP7350_OSD_COLOR_MODE_ARGB4444 },
	//{ DRM_FORMAT_RGBX8888, SP7350_OSD_COLOR_MODE_RGBA8888 },
	//{ DRM_FORMAT_XRGB8888, SP7350_OSD_COLOR_MODE_ARGB8888 },
};
#define SP7350_FORMAT_UNSUPPORT 0xF

/* convert from fourcc format to sp7530 vpp/osd format.
 *   type: 0 osd layer, 1 vpp layer.
 */
static u32 sp7350_get_format(u32 pixel_format, int type)
{
	int i;

	if (type == 1) {
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
	DRM_DEBUG_DRIVER("Not found pixel format!!fourcc_format= %d\n",
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

static void sp7350_kms_plane_vpp_atomic_update(struct drm_plane *plane,
					 struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct drm_gem_cma_object *obj = NULL;

	/* reference to ade_plane_atomic_update */
	if (!state->fb || !state->crtc) {
		/* do nothing */
		sp7350_dmix_layer_set(SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
		return;
	}

	obj = drm_fb_cma_get_gem_obj(state->fb, 0);
	if (!obj || !obj->paddr) {
		DRM_DEBUG_DRIVER("drm_fb_cma_get_gem_obj fail.\n");
		return;
	}

	DRM_DEBUG_DRIVER("\n src x,y:(%d, %d)  w,h:(%d, %d) \n crtc x,y:(%d, %d)  w,h:(%d, %d)",
		      state->src_x>> 16, state->src_y>> 16, state->src_w >> 16, state->src_h >> 16,
		      state->crtc_x, state->crtc_y, state->crtc_w, state->crtc_h);

	sp7350_vpp_imgread_set((u32)obj->paddr,
			state->src_x >> 16, state->src_y >> 16,
			state->src_w >> 16, state->src_h >> 16,
			state->fb->width, state->fb->height,
			sp7350_get_format(state->fb->format->format, 1));

	sp7350_vpp_vscl_set(state->src_x >> 16, state->src_y >> 16,
				state->src_w >> 16, state->src_h >> 16,
				state->crtc_x, state->crtc_y,
				state->crtc_w, state->crtc_h,
				state->crtc->mode.hdisplay,state->crtc->mode.vdisplay);

	/* default setting for VPP OPIF(MASK function) */
	sp7350_vpp_vpost_opif_set(state->crtc_x, state->crtc_y,
			state->crtc_w, state->crtc_h,
			state->crtc->mode.hdisplay,state->crtc->mode.vdisplay);
	/* for support letterbox boundary smoothly cropping,
		should update opif setting with another plane window size. */

	sp7350_dmix_layer_set(SP7350_DMIX_VPP0, SP7350_DMIX_BLENDING);
}


static void sp7350_kms_plane_osd_atomic_update(struct drm_plane *plane,
					 struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct drm_gem_cma_object *obj = NULL;
	#if 0
	struct sp7350fb_info info;
	#else
	struct sp7350_osd_region info;
	#endif
	int osd_layer_sel = 0;
	struct drm_format_name_buf format_name;

	/* reference to ade_plane_atomic_update */
#if !DRM_PRIMARY_PLANE_WITH_OSD
	osd_layer_sel = plane->index -1;
#else
	/* osd_layer_sel  osd-layer  plane-index
	 *    0             osd0        4
	 *    1             osd1        3
	 *    2             osd2        2
	 *    3             osd3        0 (Primary plane)
	 */
	switch(plane->index) {
		case 4: osd_layer_sel = 0; break;
		case 3: osd_layer_sel = 1; break;
		case 2: osd_layer_sel = 2; break;
		case 0: osd_layer_sel = 3; break;
		default:
			DRM_DEBUG_DRIVER("Invalid osd layer select index!!!.\n");
			return;
	}
#endif

	if (!state->fb || !state->crtc) {
		/* disable this plane */
		sp7350_dmix_layer_set(SP7350_DMIX_OSD0 + osd_layer_sel, SP7350_DMIX_TRANSPARENT);
		return;
	}

	obj = drm_fb_cma_get_gem_obj(state->fb, 0);
	if (!obj || !obj->paddr) {
		DRM_DEBUG_DRIVER("drm_fb_cma_get_gem_obj fail.\n");
		return;
	}

	memset(&info, 0, sizeof(info));
	#if 0
	info.color_mode = sp7350_get_format(state->fb->format->format, 0);
	info.width = state->src_w >> 16;
	info.height = state->src_h >> 16;
	info.buf_addr_phy = (u32)obj->paddr;
	sp7350_osd_layer_set(&info, osd_layer_sel);
	#else
	info.color_mode = sp7350_get_format(state->fb->format->format, 0);
	info.buf_addr_phy = (u32)obj->paddr;
	info.region_info.buf_width = state->fb->width;
	info.region_info.buf_height = state->fb->height;
	info.region_info.start_x = state->src_x >> 16;
	info.region_info.start_y = state->src_y >> 16;
	info.region_info.act_x = state->crtc_x;
	info.region_info.act_y = state->crtc_y;
	info.region_info.act_width = state->crtc_w;
	info.region_info.act_height = state->crtc_h;

	sp7350_osd_layer_set_by_region(&info, osd_layer_sel);
	#endif


	DRM_DEBUG_DRIVER("Pixel format %s, modifier 0x%llx, C3V format:0x%X\n",
		      drm_get_format_name(state->fb->format->format, &format_name),
		      state->fb->modifier, info.color_mode);
	sp7350_dmix_layer_set(SP7350_DMIX_OSD0 + osd_layer_sel, SP7350_DMIX_BLENDING);
}

static int sp7350_kms_plane_vpp_atomic_check(struct drm_plane *plane,
				   struct drm_plane_state *state)
{
	struct drm_crtc_state *crtc_state;


	if (!state->fb || WARN_ON(!state->crtc)) {
		DRM_DEBUG_DRIVER("return 0.\n");
		return 0;
	}

	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state)) {
		DRM_DEBUG_DRIVER("drm_atomic_get_crtc_state is err\n");
		return PTR_ERR(crtc_state);
	}

	if (plane->type == DRM_PLANE_TYPE_CURSOR) {
		DRM_DEBUG_DRIVER("DRM_PLANE_TYPE_CURSOR unsupport for VPP HW.\n");
		return -EINVAL;
	}

	DRM_DEBUG_DRIVER("plane atomic check end\n");

	return 0;
}

static int sp7350_kms_plane_osd_atomic_check(struct drm_plane *plane,
				   struct drm_plane_state *state)
{
	struct drm_crtc_state *crtc_state;


	if (!state->fb || WARN_ON(!state->crtc)) {
		DRM_DEBUG_DRIVER("return 0\n");
		return 0;
	}

	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state)) {
		DRM_DEBUG_DRIVER("drm_atomic_get_crtc_state is err\n");
		return PTR_ERR(crtc_state);
	}

	if (state->crtc_w != state->src_w >> 16 || state->crtc_h != state->src_h >> 16) {
		DRM_DEBUG_DRIVER("Check fail[src(%d, %d), crtc(%d,%d)], scale function unsuppord for OSD HW.\n",
			state->src_w >> 16, state->src_h >> 16, state->crtc_w, state->crtc_h);
		return -EINVAL;
	}

	DRM_DEBUG_DRIVER("plane atomic check end\n");

	return 0;
}

static const struct drm_plane_helper_funcs sp7350_kms_vpp_helper_funcs = {
	.atomic_update		= sp7350_kms_plane_vpp_atomic_update,
	.atomic_check		=sp7350_kms_plane_vpp_atomic_check,
};

static const struct drm_plane_helper_funcs sp7350_kms_osd_helper_funcs = {
	.atomic_update		= sp7350_kms_plane_osd_atomic_update,
	.atomic_check		=sp7350_kms_plane_osd_atomic_check,
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

#if !DRM_PRIMARY_PLANE_WITH_OSD
	if (type == DRM_PLANE_TYPE_PRIMARY) {
		formats = sp7350_kms_vpp_formats;
		nformats = ARRAY_SIZE(sp7350_kms_vpp_formats);
		funcs = &sp7350_kms_vpp_helper_funcs;
	}
	else {
		formats = sp7350_kms_osd_formats;
		nformats = ARRAY_SIZE(sp7350_kms_osd_formats);
		funcs = &sp7350_kms_osd_helper_funcs;
	}
#else
	if (type == DRM_PLANE_TYPE_PRIMARY) {
		formats = sp7350_kms_osd_formats;
		nformats = ARRAY_SIZE(sp7350_kms_osd_formats);
		funcs = &sp7350_kms_osd_helper_funcs;
	}
	else {
		if (!index) {
			formats = sp7350_kms_vpp_formats;
			nformats = ARRAY_SIZE(sp7350_kms_vpp_formats);
			funcs = &sp7350_kms_vpp_helper_funcs;
		}
		else {
			formats = sp7350_kms_osd_formats;
			nformats = ARRAY_SIZE(sp7350_kms_osd_formats);
			funcs = &sp7350_kms_osd_helper_funcs;
		}
	}
#endif

	ret = drm_universal_plane_init(drm, plane, 1 << index,
					   &sp7350_drm_plane_funcs,
					   formats, nformats,
					   NULL, type, NULL);
	if (ret) {
		kfree(plane);
		return ERR_PTR(ret);
	}

	drm_plane_helper_add(plane, funcs);

	/* FIXME: First TRANSPARENT all layer */
#if DRM_PRIMARY_PLANE_ONLY
	sp7350_dmix_layer_cfg_set(4);
	//sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_OSD3, SP7350_DMIX_BLENDING);
#endif

	return plane;
}

int sp7350_plane_create_additional_planes(struct drm_device *drm)
{
	//struct drm_plane *cursor_plane;
	//struct drm_crtc *crtc;
	unsigned int i;
	unsigned int overlay_num = 3;

#if DRM_PRIMARY_PLANE_WITH_OSD
	sp7350_dmix_layer_cfg_set(4);
	//sp7350_dmix_layer_init(SP7350_DMIX_L6, SP7350_DMIX_OSD0, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L5, SP7350_DMIX_OSD1, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L4, SP7350_DMIX_OSD2, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L3, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_OSD3, SP7350_DMIX_BLENDING);
	//sp7350_dmix_layer_init(SP7350_DMIX_L2, SP7350_DMIX_VPP1, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_L1, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
	//sp7350_dmix_layer_init(SP7350_DMIX_BG, SP7350_DMIX_OSD3, SP7350_DMIX_BLENDING);
#endif

	/* for c3v soc display controller, has 4 osd layer.
	 * used for DRM OVERLAY???
	 */
	for (i = 0; i < overlay_num; i++) {
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

