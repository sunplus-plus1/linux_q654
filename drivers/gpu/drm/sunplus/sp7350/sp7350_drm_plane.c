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
#include "sp7350_drm_plane.h"

//#include "sp7350_display.h"
#include <media/sunplus/disp/sp7350/sp7350_disp_osd.h>
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_vpp.h"
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_dmix.h"

/* always keep 0 */
#define SP7350_DRM_TODO    0

/* sp7350-hw-format translate table */
struct sp7350_plane_format {
	u32 pixel_format;
	u32 hw_format;
};

static const u32 sp7350_kms_vpp_formats[] = {
	DRM_FORMAT_YUYV,  /* SP7350_VPP_IMGREAD_DATA_FMT_YUY2 */
	DRM_FORMAT_UYVY,  /* SP7350_VPP_IMGREAD_DATA_FMT_UYVY */
	DRM_FORMAT_NV12,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV12 */
	DRM_FORMAT_NV16,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV16 */
	DRM_FORMAT_NV24,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV24 */
};

static const u32 sp7350_kms_osd_formats[] = {
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
	} else {
		for (i = 0; i < ARRAY_SIZE(sp7350_osd_formats); i++)
			if (sp7350_osd_formats[i].pixel_format == pixel_format)
				return sp7350_osd_formats[i].hw_format;
	}

	/* not found */
	DRM_DEBUG_DRIVER("Not found pixel format!!fourcc_format= %d\n",
			 pixel_format);
	return SP7350_FORMAT_UNSUPPORT;
}

static int sp7350_plane_atomic_set_property(struct drm_plane *plane,
					    struct drm_plane_state *state,
					    struct drm_property *property,
					    uint64_t val)
{
	struct sp7350_drm_plane *sp7350_plane = to_sp7350_drm_plane(plane);

	DRM_DEBUG_ATOMIC("property.name:%s val:%llu\n", property->name, val);
	//DRM_INFO("%s property.values:%d\n", __func__, *property->values);

	if (sp7350_plane->type != DRM_PLANE_TYPE_OVERLAY) {
		DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
		return -EINVAL;
	}

	if (!strcmp(property->name, "region alpha")) {
		struct sp7350_plane_region_alpha_info *info;
		struct drm_property_blob *blob = drm_property_lookup_blob(plane->dev, val);

		if (!(sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		if (!blob) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}
		if (!blob->data) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		if (blob->length != sizeof(struct sp7350_plane_region_alpha_info)) {
			DRM_DEBUG_ATOMIC("[plane:%d:%s] bad mode blob length: %zu\n",
					 sp7350_plane->base.base.id, sp7350_plane->base.name,
					 blob->length);
			return -EINVAL;
		}
		info = blob->data;

		/* check value validity */
		if (info->alpha > 255 || info->alpha < 0) {
			DRM_DEBUG_ATOMIC("Outof limit! property alpha region [0, 255]!\n");
			return -EINVAL;
		}

		if (blob == sp7350_plane->state.region_alpha_blob)
			return 0;

		drm_property_blob_put(sp7350_plane->state.region_alpha_blob);
		sp7350_plane->state.region_alpha_blob = NULL;

		if (sp7350_plane->is_media_plane) {
			/*
			 * WARNING:
			 * vpp plane region alpha setting by vpp opif mask function.
			 * So only one region for media plane, the parameter "regionid" is invalid.
			 */
			DRM_DEBUG_ATOMIC("update vpp opif alpha value by the property!\n");

			//sp7350_vpp_vpost_opif_alpha_set(info->alpha, 0);
		} else {
			/*
			 * TODO:
			 * osd plane region alpha setting by osd region header.
			 * So support muti-region, the parameter "regionid" is related to osd region.
			 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
			 */
			DRM_DEBUG_ATOMIC("update region alpha value by the property!\n");
		}
		/*
		 * Save to plane state, but not update to hw reg.
		 * All hw reg updated at atomic update.
		 */
		sp7350_plane->state.region_alpha.regionid = info->regionid;
		sp7350_plane->state.region_alpha.alpha = info->alpha;
		sp7350_plane->state.region_alpha_blob = drm_property_blob_get(blob);

		DRM_DEBUG_ATOMIC("Set plane[%d] region alpha: regionid:%d, alpha:%d\n",
				 plane->index, info->regionid, info->alpha);

		drm_property_blob_put(blob);
	}
	else if (!strcmp(property->name, "color keying")) {
		u32 global_color_keying_val = val;

		if (!(sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		DRM_DEBUG_ATOMIC("Set plane[%d] color keying: 0x%08x\n", plane->index, global_color_keying_val);

		if (sp7350_plane->is_media_plane) {
			DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
			return -EINVAL;
		}

		/*
		 * TODO:
		 * osd plane region alpha setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		DRM_DEBUG_ATOMIC("update color keying value by the property!\n");
		sp7350_plane->state.color_keying = global_color_keying_val;
	}
	else if (!strcmp(property->name, "region color keying")) {
		struct drm_property_blob *blob = drm_property_lookup_blob(plane->dev, val);
		struct sp7350_plane_region_color_keying_info *info;

		if (!(sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		if (blob->length != sizeof(struct sp7350_plane_region_color_keying_info)) {
			DRM_DEBUG_ATOMIC("[plane:%d:%s] bad mode blob length: %zu\n",
					 sp7350_plane->base.base.id, sp7350_plane->base.name,
					 blob->length);
			return -EINVAL;
		}
		info = blob->data;

		if (sp7350_plane->is_media_plane) {
			DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
			return -EINVAL;
		}

		if (blob == sp7350_plane->state.region_color_keying_blob)
			return 0;

		drm_property_blob_put(sp7350_plane->state.region_color_keying_blob);
		sp7350_plane->state.region_color_keying_blob = NULL;

		/*
		 * TODO:
		 * osd plane region alpha setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		DRM_DEBUG_ATOMIC("update region color keying value by the property!\n");

		/*
		 * Save to plane state, but not update to hw reg.
		 * All hw reg updated at atomic update.
		 */
		sp7350_plane->state.region_color_keying.regionid = info->regionid;
		sp7350_plane->state.region_color_keying.keying = info->keying;

		sp7350_plane->state.region_color_keying_blob = drm_property_blob_get(blob);
		DRM_DEBUG_ATOMIC("Set plane-%d region color keying: regionid:%d, keying:0x%08x\n",
				 plane->index, info->regionid, info->keying);

		drm_property_blob_put(blob);
	}
	else {
		DRM_DEBUG_ATOMIC("the property isn't implemented by the driver!\n");
		return -EINVAL;
	}

	return 0;
}

static int sp7350_plane_atomic_get_property(struct drm_plane *plane,
					    const struct drm_plane_state *state,
					    struct drm_property *property,
					    uint64_t *val)
{
	struct sp7350_drm_plane *sp7350_plane = to_sp7350_drm_plane(plane);

	DRM_DEBUG_ATOMIC("Get plane-%d property.name:%s\n", plane->index, property->name);

	if (property == sp7350_plane->region_alpha_property) {
		if (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND) {
			*val = (sp7350_plane->state.region_alpha_blob) ?
				 sp7350_plane->state.region_alpha_blob->base.id : 0;
			return 0;
		}
	} else if (property == sp7350_plane->region_color_keying_property) {
		if (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING) {
			*val = (sp7350_plane->state.region_color_keying_blob) ?
				 sp7350_plane->state.region_color_keying_blob->base.id : 0;
			return 0;
		}
	} else if (property == sp7350_plane->color_keying_property) {
		if (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING) {
			*val = sp7350_plane->state.color_keying;
			return 0;
		}
	}

	DRM_DEBUG_ATOMIC("the property \"%s\" isn't implemented for plane-%d!\n", property->name, plane->index);
	return -EINVAL;
}

static const struct drm_plane_funcs sp7350_drm_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.atomic_set_property = sp7350_plane_atomic_set_property,
	.atomic_get_property = sp7350_plane_atomic_get_property,
};

static void sp7350_kms_plane_vpp_atomic_update(struct drm_plane *plane,
					       struct drm_plane_state *old_state)
{
	struct sp7350_drm_plane *sp7350_plane = to_sp7350_drm_plane(plane);
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
		DRM_DEBUG_ATOMIC("drm_fb_cma_get_gem_obj fail.\n");
		return;
	}

	DRM_DEBUG_ATOMIC("\n src x,y:(%d, %d)  w,h:(%d, %d)\n crtc x,y:(%d, %d)  w,h:(%d, %d)",
			 state->src_x >> 16, state->src_y >> 16, state->src_w >> 16, state->src_h >> 16,
			 state->crtc_x, state->crtc_y, state->crtc_w, state->crtc_h);

	DRM_DEBUG_ATOMIC("plane info[%d, %d] zpos:%d\n",
			 plane->index, sp7350_plane->index, sp7350_plane->zpos);

	sp7350_vpp_imgread_set((u32)obj->paddr,
			       state->src_x >> 16, state->src_y >> 16,
			       state->src_w >> 16, state->src_h >> 16,
			       state->fb->width, state->fb->height,
			       sp7350_get_format(state->fb->format->format, 1));

	sp7350_vpp_vscl_set(state->src_x >> 16, state->src_y >> 16,
			    state->src_w >> 16, state->src_h >> 16,
			    state->crtc_x, state->crtc_y,
			    state->crtc_w, state->crtc_h,
			    state->crtc->mode.hdisplay, state->crtc->mode.vdisplay);

	/* default setting for VPP OPIF(MASK function) */
	sp7350_vpp_vpost_opif_set(state->crtc_x, state->crtc_y,
				  state->crtc_w, state->crtc_h,
				  state->crtc->mode.hdisplay, state->crtc->mode.vdisplay);
	/*
	 * for support letterbox boundary smoothly cropping,
	 * should update opif setting with another plane window size.
	 */

	if (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND) {
		/*
		 * Auto resize alpha region from 0 ~ 0xffff to 0 ~ 0x3f.
		 *  0x00 ~ 0x3f remark as 0, 0x00xx ~ 0xffxx remark as 0 ~0x3f.
		 */

		DRM_DEBUG_ATOMIC("Set plane[%d] alpha %d(src:%d)\n",
				 plane->index, state->alpha >> 10, state->alpha);
		/* NOTES: vpp layer(SP7350_DMIX_L3) used for media plane fixed. */
		sp7350_dmix_plane_alpha_config(SP7350_DMIX_L3, 1, 0, state->alpha >> 10);
	}

	if (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND) {
		DRM_DEBUG_ATOMIC("Set plane[%d] region alpha: regionid:%d, alpha:%d\n",
				 plane->index, sp7350_plane->state.region_alpha.regionid,
			sp7350_plane->state.region_alpha.alpha);

		/*
		 * WARNING:
		 * vpp plane region alpha setting by vpp opif mask function.
		 * So only one region for media plane, the parameter "regionid" is invalid.
		 */
		sp7350_vpp_vpost_opif_alpha_set(sp7350_plane->state.region_alpha.alpha, 0);
	}

	sp7350_dmix_layer_set(SP7350_DMIX_VPP0, SP7350_DMIX_BLENDING);
}

static void sp7350_kms_plane_osd_atomic_update(struct drm_plane *plane,
					       struct drm_plane_state *old_state)
{
	struct sp7350_drm_plane *sp7350_plane = to_sp7350_drm_plane(plane);
	struct drm_plane_state *state = plane->state;
	struct drm_gem_cma_object *obj = NULL;
	struct sp7350_osd_region info;
	int osd_layer_sel = 0;
	int layer = 0;
	struct drm_format_name_buf format_name;

	/* reference to ade_plane_atomic_update */
#if !DRM_PRIMARY_PLANE_WITH_OSD
	osd_layer_sel = plane->index - 1;
#else
	/* osd_layer_sel  osd-layer  plane-index
	 *    0             osd0        4
	 *    1             osd1        3
	 *    2             osd2        2
	 *    3             osd3        0 (Primary plane)
	 */
	switch (plane->index) {
	case 4:
		osd_layer_sel = 0;
		layer = SP7350_DMIX_L6;
		break;
	case 3:
		osd_layer_sel = 1;
		layer = SP7350_DMIX_L5;
		break;
	case 2:
		osd_layer_sel = 2;
		layer = SP7350_DMIX_L4;
		break;
	case 0:
		osd_layer_sel = 3;
		layer = SP7350_DMIX_L1;
		break;
	default:
		DRM_DEBUG_ATOMIC("Invalid osd layer select index!!!.\n");
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
		DRM_DEBUG_ATOMIC("drm_fb_cma_get_gem_obj fail.\n");
		return;
	}

	DRM_DEBUG_ATOMIC("\n src x,y:(%d, %d)  w,h:(%d, %d)\n crtc x,y:(%d, %d)  w,h:(%d, %d)",
			 state->src_x >> 16, state->src_y >> 16, state->src_w >> 16, state->src_h >> 16,
			 state->crtc_x, state->crtc_y, state->crtc_w, state->crtc_h);

	memset(&info, 0, sizeof(info));
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

	DRM_DEBUG_ATOMIC("plane info[%d, %d] zpos:%d\n",
			 plane->index, sp7350_plane->index, sp7350_plane->zpos);

	if (sp7350_plane->state.region_alpha_blob &&
		 (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND)) {
		DRM_DEBUG_ATOMIC("Set plane[%d] region alpha: regionid:%d, alpha:%d\n",
				 plane->index, sp7350_plane->state.region_alpha.regionid,
				 sp7350_plane->state.region_alpha.alpha);

		/*
		 * TODO:
		 * osd plane region alpha setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		info.alpha_info.region_alpha_en = 1;
		info.alpha_info.region_alpha = sp7350_plane->state.region_alpha.alpha;
	}
	else {
		info.alpha_info.region_alpha_en = 0;
	}
	if (sp7350_plane->state.region_color_keying_blob &&
		 (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING)) {
		DRM_DEBUG_ATOMIC("Set plane[%d] region color keying: regionid:%d, keying:0x%08x\n",
				 plane->index, sp7350_plane->state.region_color_keying.regionid,
				 sp7350_plane->state.region_color_keying.keying);
		/*
		 * TODO:
		 * osd plane region alpha setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		info.alpha_info.color_key_en = 1;
		info.alpha_info.color_key = sp7350_plane->state.region_color_keying.keying;
	} else if (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING) {
		DRM_DEBUG_ATOMIC("Set plane[%d] color keying:0x%08x\n",
				 plane->index, sp7350_plane->state.color_keying);
		if (!sp7350_plane->state.color_keying)
			info.alpha_info.color_key_en = 0;
		else
			info.alpha_info.color_key_en = 1;
		info.alpha_info.color_key = sp7350_plane->state.color_keying;
	}

	sp7350_osd_layer_set_by_region(&info, osd_layer_sel);

	if (sp7350_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND) {
		/*
		 * Auto resize alpha region from 0 ~ 0xffff to 0 ~ 0x3f.
		 *  0x00 ~ 0x3f remark as 0, 0x00xx ~ 0xffxx remark as 0 ~0x3f.
		 */

		DRM_DEBUG_ATOMIC("Set plane[%d] alpha %d(src:%d)\n",
				 plane->index, state->alpha >> 10, state->alpha);
		sp7350_dmix_plane_alpha_config(layer, 1, 0, state->alpha >> 10);
	}

	DRM_DEBUG_ATOMIC("\n set osd region x,y:(%d, %d)  w,h:(%d, %d)\n act x,y:(%d, %d)  w,h:(%d, %d)",
			 info.region_info.start_x, info.region_info.start_y,
			 info.region_info.buf_width, info.region_info.buf_height,
			 info.region_info.act_x, info.region_info.act_y,
			 info.region_info.act_width, info.region_info.act_height);

	DRM_DEBUG_ATOMIC("Pixel format %s, modifier 0x%llx, C3V format:0x%X\n",
			 drm_get_format_name(state->fb->format->format, &format_name),
			 state->fb->modifier, info.color_mode);
	sp7350_dmix_layer_set(SP7350_DMIX_OSD0 + osd_layer_sel, SP7350_DMIX_BLENDING);
}

static int sp7350_kms_plane_vpp_atomic_check(struct drm_plane *plane,
					     struct drm_plane_state *state)
{
	struct drm_crtc_state *crtc_state;

	if (!state->fb || WARN_ON(!state->crtc)) {
		DRM_DEBUG_ATOMIC("return 0.\n");
		return 0;
	}

	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state)) {
		DRM_DEBUG_ATOMIC("drm_atomic_get_crtc_state is err\n");
		return PTR_ERR(crtc_state);
	}

	if (plane->type == DRM_PLANE_TYPE_CURSOR) {
		DRM_DEBUG_ATOMIC("DRM_PLANE_TYPE_CURSOR unsupport for VPP HW.\n");
		return -EINVAL;
	}

	DRM_DEBUG_ATOMIC("plane atomic check end\n");

	return 0;
}

static int sp7350_kms_plane_osd_atomic_check(struct drm_plane *plane,
					     struct drm_plane_state *state)
{
	struct drm_crtc_state *crtc_state;

	if (!state->fb || WARN_ON(!state->crtc)) {
		DRM_DEBUG_ATOMIC("return 0\n");
		return 0;
	}

	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state)) {
		DRM_DEBUG_ATOMIC("drm_atomic_get_crtc_state is err\n");
		return PTR_ERR(crtc_state);
	}

	if (state->crtc_w != state->src_w >> 16 || state->crtc_h != state->src_h >> 16) {
		DRM_DEBUG_ATOMIC("Check fail[src(%d, %d), crtc(%d,%d)], scale function unsuppord for OSD HW.\n",
				 state->src_w >> 16, state->src_h >> 16, state->crtc_w, state->crtc_h);
		return -EINVAL;
	}

	DRM_DEBUG_ATOMIC("plane atomic check end\n");

	return 0;
}

static const struct drm_plane_helper_funcs sp7350_kms_vpp_helper_funcs = {
	.atomic_update		= sp7350_kms_plane_vpp_atomic_update,
	.atomic_check = sp7350_kms_plane_vpp_atomic_check,
};

static const struct drm_plane_helper_funcs sp7350_kms_osd_helper_funcs = {
	.atomic_update		= sp7350_kms_plane_osd_atomic_update,
	.atomic_check = sp7350_kms_plane_osd_atomic_check,
};

static void sp7350_plane_create_propertys(struct sp7350_drm_plane *plane)
{
	if (plane->capabilities & SP7350_DRM_PLANE_CAP_ZPOS)
		drm_plane_create_zpos_property(&plane->base, plane->zpos, 0, SP7350_MAX_PLANE - 1);
	else
		drm_plane_create_zpos_immutable_property(&plane->base, plane->zpos);

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_ROTATION) {
		drm_plane_create_rotation_property(&plane->base, DRM_MODE_ROTATE_0,
					  DRM_MODE_ROTATE_0 | DRM_MODE_ROTATE_90 | DRM_MODE_REFLECT_Y);
	}

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_PIX_BLEND) {
		drm_plane_create_blend_mode_property(&plane->base, BIT(DRM_MODE_BLEND_PREMULTI));
	}

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND)
		drm_plane_create_alpha_property(&plane->base);

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND) {
		/* region alpha region: 0~255 */
		plane->region_alpha_property
			 = drm_property_create(plane->base.dev,
						  DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
						  "region alpha", 0);
		plane->state.region_alpha.regionid = 0;
		plane->state.region_alpha.alpha = 0xff;
		drm_object_attach_property(&plane->base.base,
			 plane->region_alpha_property, 0);
	}

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING) {
		/* region color keying region: 0~0xffffffff */
		plane->region_color_keying_property =
			 drm_property_create(plane->base.dev,
						  DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
						  "region color keying", 0);
		plane->state.region_color_keying.regionid = 0;
		plane->state.region_color_keying.keying = 0;
		drm_object_attach_property(&plane->base.base,
			 plane->region_color_keying_property, 0);
	}

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING) {
		/* color_keying format:RGBA8888, region: 0~0xffffffff */
		plane->color_keying_property =
			 drm_property_create_range(plane->base.dev, DRM_MODE_PROP_ATOMIC,
					 "color keying", 0, 0xffffffff);
		plane->state.color_keying = 0;
		drm_object_attach_property(&plane->base.base,
			 plane->color_keying_property, (uint64_t)plane->state.color_keying);
	}
}

static void sp7350_plane_destroy_propertys(struct sp7350_drm_plane *plane)
{
	if (plane->region_alpha_property) {
		drm_property_destroy(plane->base.dev, plane->region_alpha_property);
		plane->region_alpha_property = NULL;
	}

	if (plane->region_color_keying_property) {
		drm_property_destroy(plane->base.dev, plane->region_color_keying_property);
		plane->region_color_keying_property = NULL;
	}

	if (plane->color_keying_property) {
		drm_property_destroy(plane->base.dev, plane->color_keying_property);
		plane->color_keying_property = NULL;
	}

	if (plane->state.region_alpha_blob) {
		drm_property_blob_put(plane->state.region_alpha_blob);
		plane->state.region_alpha_blob = NULL;
	}
	if (plane->state.region_color_keying_blob) {
		drm_property_blob_put(plane->state.region_color_keying_blob);
		plane->state.region_color_keying_blob = NULL;
	}
}

static int sp7350_drm_plane_init(struct drm_device *drm, struct sp7350_drm_plane *plane)
{
	int ret;

	ret = drm_universal_plane_init(drm, &plane->base, plane->base.possible_crtcs,
				       &sp7350_drm_plane_funcs,
				   plane->pixel_formats, plane->num_pixel_formats,
				   NULL, plane->type, NULL);
	if (ret) {
		return ret;
	}

	drm_plane_helper_add(&plane->base, plane->funcs);

	plane->index = plane->base.index;

	sp7350_plane_create_propertys(plane);

	return 0;
}

int sp7350_plane_create_primary_plane(struct drm_device *drm, struct sp7350_drm_plane *plane)
{
#if DRM_PRIMARY_PLANE_WITH_OSD
	plane->pixel_formats = sp7350_kms_osd_formats;
	plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_osd_formats);
	plane->funcs = &sp7350_kms_osd_helper_funcs;
#else
	plane->pixel_formats = sp7350_kms_vpp_formats;
	plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_vpp_formats);
	plane->funcs = &sp7350_kms_vpp_helper_funcs;
#endif
	plane->zpos = 0;  /* SP7350_DMIX_L1 */
	plane->type = DRM_PLANE_TYPE_PRIMARY;
	plane->is_media_plane = false;
	plane->capabilities = 0;

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

	/*
	 * For c3v soc display controller.
	 */
	return sp7350_drm_plane_init(drm, plane);
}

int sp7350_plane_create_media_plane(struct drm_device *drm, struct sp7350_drm_plane *plane)
{
	plane->pixel_formats = sp7350_kms_vpp_formats;
	plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_vpp_formats);
	plane->funcs = &sp7350_kms_vpp_helper_funcs;
	plane->zpos = 1;  /* SP7350_DMIX_L2 */
	plane->type = DRM_PLANE_TYPE_OVERLAY;
	plane->is_media_plane = true;
	plane->capabilities = SP7350_DRM_PLANE_CAP_SCALE |
		SP7350_DRM_PLANE_CAP_PIX_BLEND | SP7350_DRM_PLANE_CAP_WIN_BLEND |
		SP7350_DRM_PLANE_CAP_REGION_BLEND;

	plane->base.possible_crtcs = GENMASK(drm->mode_config.num_crtc - 1, 0);

	/*
	 * For c3v soc display controller, vpp layer for media plane.
	 */
	return sp7350_drm_plane_init(drm, plane);
}

int sp7350_plane_create_overlay_plane(struct drm_device *drm, struct sp7350_drm_plane *plane, int index)
{
	plane->pixel_formats = sp7350_kms_osd_formats;
	plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_osd_formats);
	plane->funcs = &sp7350_kms_osd_helper_funcs;
	plane->zpos = 2 + index;  /* SP7350_DMIX_L2 - SP7350_DMIX_L3 */
	plane->type = DRM_PLANE_TYPE_OVERLAY;
	plane->is_media_plane = false;
	plane->capabilities = SP7350_DRM_PLANE_CAP_PIX_BLEND |
		SP7350_DRM_PLANE_CAP_WIN_BLEND | SP7350_DRM_PLANE_CAP_REGION_BLEND |
		SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING | SP7350_DRM_PLANE_CAP_COLOR_KEYING;
	plane->base.possible_crtcs = GENMASK(drm->mode_config.num_crtc - 1, 0);

	/*
	 * For c3v soc display controller, osd layer for overlay plane.
	 */
	return sp7350_drm_plane_init(drm, plane);
}

int sp7350_plane_release_plane(struct drm_device *drm, struct sp7350_drm_plane *plane)
{
	/* DO ANY??? */
	sp7350_plane_destroy_propertys(plane);

	return 0;
}

#if SP7350_DRM_TODO
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

#if SP7350_DRM_TODO  /* TODO, NOT SUPPORT cursor */
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
#endif
