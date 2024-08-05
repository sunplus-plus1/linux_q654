// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM Planes
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 *         hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_plane_helper.h>

#include "sp7350_drm_drv.h"
#include "sp7350_drm_crtc.h"
#include "sp7350_drm_plane.h"
#include "sp7350_drm_regs.h"

/*
 *  For IMGREAD Q654, Max Resolution 3840x2880
 *  For VSCL Q654, Max Resolution 2688x2880 or 2688x1944???.
 *  For OSD Q654, Max Resolution 1920x1080???.
 */
/* from spec */
#define XRES_VSCL_MAX       2688
#define YRES_VSCL_MAX       2880
#define XRES_VIMGREAD_MAX   3840
#define YRES_VIMGREAD_MAX   2880
#define XRES_OSD_MAX        1920
#define YRES_OSD_MAX        1280
/* from test */
//#define XRES_VSCL_MAX  2880
//#define YRES_VSCL_MAX  4096

#define SP7350_DRM_VPP_SCL_AUTO_ADJUST    1

/* TODO, should add property for it. */
#define SP7350_DRM_VPP_SCL_AUTO_ADJUST    1

#define SP7350_PLANE_READ(offset) readl(sp_dev->crtc_regs + (offset))
#define SP7350_PLANE_WRITE(offset, val) writel(val, sp_dev->crtc_regs + (offset))

/* sp7350-hw-format translate table */
struct sp7350_plane_format {
	u32 pixel_format;
	u32 hw_format;
};

u32 sp7350_kms_vpp_formats[5] = {
	DRM_FORMAT_YUYV,  /* SP7350_VPP_IMGREAD_DATA_FMT_YUY2 */
	DRM_FORMAT_UYVY,  /* SP7350_VPP_IMGREAD_DATA_FMT_UYVY */
	DRM_FORMAT_NV12,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV12 */
	DRM_FORMAT_NV16,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV16 */
	DRM_FORMAT_NV24,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV24 */
};

u32 sp7350_kms_osd_formats[8] = {
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

void sp7350_dmix_layer_set(struct drm_plane *plane, int fg_sel, int layer_mode)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	u32 value, value1, tmp_value;
	u32 layer, i;

	/* Find Layer number */
	value = SP7350_PLANE_READ(DMIX_LAYER_CONFIG_0);
	for (i = 0; i < SP7350_DMIX_MAX_LAYER; i++) {
		//tmp_value = FIELD_GET(GENMASK(3, 0), value);
		tmp_value = (value >> (i * 4 + 4)) & GENMASK(3, 0);
		if (tmp_value == fg_sel) {
			layer = i;
			break;
		}
	}

	/* Set layer mode */
	value1 = SP7350_PLANE_READ(DMIX_LAYER_CONFIG_1);
	if (layer != SP7350_DMIX_BG) {
		value1 &= ~(GENMASK(1, 0) << ((layer - 1) << 1));
		value1 |= (layer_mode << ((layer - 1) << 1));
	}
	SP7350_PLANE_WRITE(DMIX_LAYER_CONFIG_1, value1);
}

int sp7350_vpp_vpost_opif_alpha_set(struct drm_plane *plane, int alpha, int mask_alpha)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	u32 value;

	value = SP7350_PLANE_READ(VPOST_OPIF_CONFIG);
	value |= SP7350_VPP_VPOST_WIN_ALPHA_EN;
	SP7350_PLANE_WRITE(VPOST_OPIF_CONFIG, value);

	/*set alpha value*/
	value = SP7350_PLANE_READ(VPOST_OPIF_ALPHA);
	value &= ~(SP7350_VPP_VPOST_WIN_ALPHA_MASK | SP7350_VPP_VPOST_VPP_ALPHA_MASK);
	value |= (SP7350_VPP_VPOST_WIN_ALPHA_SET(mask_alpha) |
		SP7350_VPP_VPOST_VPP_ALPHA_SET(alpha));
	SP7350_PLANE_WRITE(VPOST_OPIF_ALPHA, value);

	return 0;
}

int sp7350_vpp_imgread_set(struct drm_plane *plane, u32 data_addr1, int x, int y, int img_src_w, int img_src_h, int input_w, int input_h, int yuv_fmt)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	u32 value;

	value = SP7350_PLANE_READ(IMGREAD_GLOBAL_CONTROL);
	value |= SP7350_VPP_IMGREAD_FETCH_EN;
	SP7350_PLANE_WRITE(IMGREAD_GLOBAL_CONTROL, value);

	value = SP7350_PLANE_READ(IMGREAD_CONFIG);
	value &= ~(SP7350_VPP_IMGREAD_DATA_FMT |
		SP7350_VPP_IMGREAD_YC_SWAP | SP7350_VPP_IMGREAD_UV_SWAP);
	if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_YUY2)
		value |= SP7350_VPP_IMGREAD_DATA_FMT_SEL(SP7350_VPP_IMGREAD_DATA_FMT_YUY2);
	else if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_NV12)
		value |= SP7350_VPP_IMGREAD_DATA_FMT_SEL(SP7350_VPP_IMGREAD_DATA_FMT_NV12);
	else if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_NV16)
		value |= SP7350_VPP_IMGREAD_DATA_FMT_SEL(SP7350_VPP_IMGREAD_DATA_FMT_NV16);
	else if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_NV24)
		value |= SP7350_VPP_IMGREAD_DATA_FMT_SEL(SP7350_VPP_IMGREAD_DATA_FMT_NV24);
	else if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_UYVY)
		value |= SP7350_VPP_IMGREAD_DATA_FMT_SEL(SP7350_VPP_IMGREAD_DATA_FMT_YUY2) |
			SP7350_VPP_IMGREAD_YC_SWAP;
	else
		value |= SP7350_VPP_IMGREAD_DATA_FMT_SEL(SP7350_VPP_IMGREAD_DATA_FMT_YUY2);

	value |= SP7350_VPP_IMGREAD_FM_MODE;

	//if (disp_dev->out_res.mipitx_mode == SP7350_MIPITX_DSI)
	//	;//TBD
	//else
	//	value |= SP7350_VPP_IMGREAD_UV_SWAP; //for CSI YUY2 test only
	SP7350_PLANE_WRITE(IMGREAD_CONFIG, value);

	value = 0;
	//#if (SP7350_VPP_SCALE_METHOD == 1) //method 1 , set imgread xstart&ystart
	value |= (y << 16) | (x << 0);
	//#endif
	SP7350_PLANE_WRITE(IMGREAD_CROP_START, value);

	value = 0;
	value |= (img_src_h << 16) | (img_src_w << 0);
	SP7350_PLANE_WRITE(IMGREAD_FRAME_SIZE, value);

	value = 0;
	if ((yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_YUY2) ||
		 (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_UYVY))
		value |= (0 << 16) | (input_w * 2);
	else if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_NV12)
		//value |= ((img_src_w / 2) << 16) | (img_src_w);
		value |= (input_w << 16) | (input_w);
	else if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_NV16)
		value |= (0 << 16) | (input_w);
	else if (yuv_fmt == SP7350_VPP_IMGREAD_DATA_FMT_NV24)
		value |= ((input_w * 2) << 16) | (input_w);
	else
		value |= (0 << 16) | (input_w * 2);

	SP7350_PLANE_WRITE(IMGREAD_LINE_STRIDE_SIZE, value);

	SP7350_PLANE_WRITE(IMGREAD_DATA_ADDRESS_1, (u32)data_addr1);
	SP7350_PLANE_WRITE(IMGREAD_DATA_ADDRESS_2, (u32)data_addr1 + (input_w * input_h));

	return 0;
}

int sp7350_vpp_vscl_set(struct drm_plane *plane, int x, int y, int img_src_w, int img_src_h, int img_dest_x, int img_dest_y, int img_dest_w, int img_dest_h, int output_w, int output_h)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	u64 factor64, factor_init64;
	int crop_xlen, crop_ylen;
	u32 source_w, source_h;
	u32 value;

	//#if (SP7350_VPP_SCALE_METHOD == 1) //method 1 , set imgread xstart&ystart
	x = 0;
	y = 0;
	//#endif

	value = SP7350_PLANE_READ(VSCL_CONFIG2);
	value |= (SP7350_VPP_VSCL_BUFR_EN |
		SP7350_VPP_VSCL_VINT_EN |
		SP7350_VPP_VSCL_HINT_EN |
		SP7350_VPP_VSCL_DCTRL_EN |
		SP7350_VPP_VSCL_ACTRL_EN);
	SP7350_PLANE_WRITE(VSCL_CONFIG2, value);

	if (img_src_w < 128) img_src_w = 128;
	if (img_src_h < 128) img_src_h = 128;

	if (x > (img_src_w - 128)) x = img_src_w / 2;
	if (y > (img_src_h - 128)) y = img_src_h / 2;

	crop_xlen = img_src_w - x;
	crop_ylen = img_src_h - y;

	/* FIXME: For DRM Driver, L3 with VPP0 for overlay(media) plane, L1 with OSD3 for primary plane. */
	#if  0//CONFIG_DRM_SP7350
	SP7350_PLANE_WRITE(VSCL_ACTRL_I_XLEN, output_w);
	SP7350_PLANE_WRITE(VSCL_ACTRL_I_YLEN, output_h);
	#else
	SP7350_PLANE_WRITE(VSCL_ACTRL_I_XLEN, crop_xlen);
	SP7350_PLANE_WRITE(VSCL_ACTRL_I_YLEN, crop_ylen);
	#endif

	//#if (SP7350_VPP_SCALE_METHOD == 1) //method 1 , set imgread xstart&ystart, fix 0 here
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_XSTART, 0);
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_YSTART, 0);
	//#elif (SP7350_VPP_SCALE_METHOD == 2) //method 2 , set vscl xstart&ystart
	//SP7350_PLANE_WRITE(VSCL_ACTRL_S_XSTART, x);
	//SP7350_PLANE_WRITE(VSCL_ACTRL_S_YSTART, y);
	//#endif

	#if 1 //crop left & top only
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_XLEN, crop_xlen);
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_YLEN, crop_ylen);
	#else //crop left & top & right & bot
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_XLEN, crop_xlen - x);
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_YLEN, crop_ylen - y);
	#endif

	SP7350_PLANE_WRITE(VSCL_DCTRL_D_XSTART, img_dest_x);
	SP7350_PLANE_WRITE(VSCL_DCTRL_D_YSTART, img_dest_y);
	SP7350_PLANE_WRITE(VSCL_DCTRL_D_XLEN, img_dest_w);
	SP7350_PLANE_WRITE(VSCL_DCTRL_D_YLEN, img_dest_h);

	SP7350_PLANE_WRITE(VSCL_DCTRL_O_XLEN, output_w);
	SP7350_PLANE_WRITE(VSCL_DCTRL_O_YLEN, output_h);

	/*
	 * VSCL SETTING for HORIZONTAL
	 */
	value = SP7350_PLANE_READ(VSCL_HINT_CTRL);
	value |= SP7350_VPP_VSCL_HINT_FLT_EN;
	SP7350_PLANE_WRITE(VSCL_HINT_CTRL, value);

	source_w = SP7350_PLANE_READ(VSCL_ACTRL_S_XLEN);
	/* cal h_factor */
	if (source_w == img_dest_w)
		factor64 = DIV64_U64_ROUND_CLOSEST((u64)source_w << 22, (u64)img_dest_w);
	else {
		factor64 = DIV64_U64_ROUND_UP((u64)source_w << 22, (u64)img_dest_w);
		//#if (SP7350_VPP_FACTOR_METHOD == 0)
		factor64 -= 1;
		//#endif
	}
	value = ((u32)factor64 >> 0) & 0x0000ffff;
	SP7350_PLANE_WRITE(VSCL_HINT_HFACTOR_LOW, value);
	value = ((u32)factor64 >> 16) & 0x000001ff;
	SP7350_PLANE_WRITE(VSCL_HINT_HFACTOR_HIGH, value);

	/* cal h_factor_init */
	if (source_w == img_dest_w)
		factor_init64 = 0;
	else {
		factor_init64 = ((u64)source_w << 22) % ((u64)img_dest_w);
		//#if (SP7350_VPP_FACTOR_METHOD == 1)
		//factor_init64 = img_dest_w - factor_init64;
		//#endif
	}
	value = ((u32)factor_init64 >> 0) & 0x0000ffff;
	SP7350_PLANE_WRITE(VSCL_HINT_INITF_LOW, value);
	value = ((u32)factor_init64 >> 16) & 0x0000003f;
	//#if (SP7350_VPP_FACTOR_METHOD == 1)
	//value |= SP7350_VPP_VSCL_HINT_INITF_PN;
	//#endif
	SP7350_PLANE_WRITE(VSCL_HINT_INITF_HIGH, value);

	/*
	 * VSCL SETTING for VERTICAL
	 */
	value = SP7350_PLANE_READ(VSCL_VINT_CTRL);
	value |= SP7350_VPP_VSCL_VINT_FLT_EN;
	SP7350_PLANE_WRITE(VSCL_VINT_CTRL, value);

	source_h = SP7350_PLANE_READ(VSCL_ACTRL_S_YLEN);
	/* cal v_factor */
	if (source_h == img_dest_h)
		factor64 = DIV64_U64_ROUND_CLOSEST((u64)source_h << 22, (u64)img_dest_h);
	else {
		factor64 = DIV64_U64_ROUND_UP((u64)source_h << 22, (u64)img_dest_h);
		//#if (SP7350_VPP_FACTOR_METHOD == 0)
		factor64 -= 1;
		//#endif
	}
	value = ((u32)factor64 >> 0) & 0x0000ffff;
	SP7350_PLANE_WRITE(VSCL_VINT_VFACTOR_LOW, value);
	value = ((u32)factor64 >> 16) & 0x000001ff;
	SP7350_PLANE_WRITE(VSCL_VINT_VFACTOR_HIGH, value);

	/* cal v_factor_init */
	if (source_h == img_dest_h)
		factor_init64 = 0;
	else {
		factor_init64 = ((u64)source_h << 22) % ((u64)img_dest_h);
		//#if (SP7350_VPP_FACTOR_METHOD == 1)
		//factor_init64 = img_dest_h - factor_init64;
		//#endif
	}
	value = ((u32)factor_init64 >> 0) & 0x0000ffff;
	SP7350_PLANE_WRITE(VSCL_VINT_INITF_LOW, value);
	value = ((u32)factor_init64 >> 16) & 0x0000003f;
	//#if (SP7350_VPP_FACTOR_METHOD == 1)
	//value |= SP7350_VPP_VSCL_VINT_INITF_PN;
	//#endif
	SP7350_PLANE_WRITE(VSCL_VINT_INITF_HIGH, value);

	return 0;
}

#define SP7350_VPP_VPOST_WIN_ALPHA_VALUE	0xff
#define SP7350_VPP_VPOST_VPP_ALPHA_VALUE	0xff

int sp7350_vpp_vpost_opif_set(struct drm_plane *plane, int act_x, int act_y, int act_w, int act_h, int output_w, int output_h)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	u32 value;

	/*
	 * VPOST SETTING
	 */
	value = SP7350_PLANE_READ(VPOST_CONFIG);
	value |= SP7350_VPP_VPOST_OPIF_EN;
	SP7350_PLANE_WRITE(VPOST_CONFIG, value);

	value = SP7350_PLANE_READ(VPOST_OPIF_CONFIG);
	value |= SP7350_VPP_VPOST_WIN_ALPHA_EN | SP7350_VPP_VPOST_WIN_YUV_EN;
	SP7350_PLANE_WRITE(VPOST_OPIF_CONFIG, value);

	/*set alpha value*/
	value = SP7350_PLANE_READ(VPOST_OPIF_ALPHA);
	value &= ~(SP7350_VPP_VPOST_WIN_ALPHA_MASK | SP7350_VPP_VPOST_VPP_ALPHA_MASK);
	value |= (SP7350_VPP_VPOST_WIN_ALPHA_SET(0) |
		SP7350_VPP_VPOST_VPP_ALPHA_SET(SP7350_VPP_VPOST_VPP_ALPHA_VALUE));
	SP7350_PLANE_WRITE(VPOST_OPIF_ALPHA, value);

	/*set mask region*/
	value = SP7350_PLANE_READ(VPOST_OPIF_MSKTOP);
	value &= ~SP7350_VPP_VPOST_OPIF_TOP_MASK;
	value |= SP7350_VPP_VPOST_OPIF_TOP_SET(act_y);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKTOP, value);

	value = SP7350_PLANE_READ(VPOST_OPIF_MSKBOT);
	value &= ~SP7350_VPP_VPOST_OPIF_BOT_MASK;
	value |= SP7350_VPP_VPOST_OPIF_BOT_SET(output_h -act_h - act_y);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKBOT, value);

	value = SP7350_PLANE_READ(VPOST_OPIF_MSKLEFT);
	value &= ~SP7350_VPP_VPOST_OPIF_LEFT_MASK;
	value |= SP7350_VPP_VPOST_OPIF_LEFT_SET(act_x);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKLEFT, value);

	value = SP7350_PLANE_READ(VPOST_OPIF_MSKRIGHT);
	value &= ~SP7350_VPP_VPOST_OPIF_RIGHT_MASK;
	value |= SP7350_VPP_VPOST_OPIF_RIGHT_SET(output_w - act_w - act_x);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKRIGHT, value);

	return 0;
}

void sp7350_dmix_plane_alpha_config(struct drm_plane *plane, int layer, int enable, int fix_alpha, int alpha_value)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	u32 value1, value2;

	value1 = SP7350_PLANE_READ(DMIX_PLANE_ALPHA_CONFIG_0);
	value2 = SP7350_PLANE_READ(DMIX_PLANE_ALPHA_CONFIG_1);

	switch (layer) {
		case SP7350_DMIX_L1:
			value1 &= ~(GENMASK(15, 8));
			value1 |= FIELD_PREP(GENMASK(15, 15), enable) |
				FIELD_PREP(GENMASK(14, 14), fix_alpha) |
				FIELD_PREP(GENMASK(13, 8), alpha_value);
			break;
		case SP7350_DMIX_L2:
			break;
		case SP7350_DMIX_L3:
			value2 &= ~(GENMASK(31, 24));
			value2 |= FIELD_PREP(GENMASK(31, 31), enable) |
				FIELD_PREP(GENMASK(30, 30), fix_alpha) |
				FIELD_PREP(GENMASK(29, 24), alpha_value);
			break;
		case SP7350_DMIX_L4:
			value2 &= ~(GENMASK(23, 16));
			value2 |= FIELD_PREP(GENMASK(23, 23), enable) |
				FIELD_PREP(GENMASK(22, 22), fix_alpha) |
				FIELD_PREP(GENMASK(21, 16), alpha_value);
			break;
		case SP7350_DMIX_L5:
			value2 &= ~(GENMASK(15, 8));
			value2 |= FIELD_PREP(GENMASK(15, 15), enable) |
				FIELD_PREP(GENMASK(14, 14), fix_alpha) |
				FIELD_PREP(GENMASK(13, 8), alpha_value);
			break;
		case SP7350_DMIX_L6:
			value2 &= ~(GENMASK(7, 0));
			value2 |= FIELD_PREP(GENMASK(7, 7), enable) |
				FIELD_PREP(GENMASK(6, 6), fix_alpha) |
				FIELD_PREP(GENMASK(5, 0), alpha_value);
			break;
		default:
			break;
	}
	SP7350_PLANE_WRITE(DMIX_PLANE_ALPHA_CONFIG_0, value1);
	SP7350_PLANE_WRITE(DMIX_PLANE_ALPHA_CONFIG_1, value2);

}

void sp7350_osd_layer_set_by_region(struct drm_plane *plane, struct sp7350_osd_region *info, int osd_layer_sel)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);

	u32 tmp_width, tmp_height, tmp_color_mode;
	u32 tmp_alpha = 0, value = 0;
	u32 tmp_key = 0;
	u32 *osd_header, *osd_palette;
	//int i;

	if (!sp_dev->osd_hdr[osd_layer_sel]) {
		pr_warn("%s osd%d not exist!!\n", __func__, osd_layer_sel);
		return;
	}

	osd_header = (u32 *)sp_dev->osd_hdr[osd_layer_sel];
	osd_palette = (u32 *)sp_dev->osd_hdr[osd_layer_sel] + 32;

	/*
	 * Fill OSD Layer Header info
	 */
	tmp_color_mode = info->color_mode;
	//tmp_alpha = SP7350_OSD_HDR_BL | SP7350_OSD_HDR_ALPHA;
	//value |= (tmp_color_mode << 24) | SP7350_OSD_HDR_BS | tmp_alpha;
	//value = osd_header[0];

	value |= (tmp_color_mode << 24) | SP7350_OSD_HDR_BS;
	if(info->alpha_info.region_alpha_en){
		tmp_alpha = info->alpha_info.region_alpha;
		if(tmp_alpha <0 || tmp_alpha > 255){
			tmp_alpha = 255;
		}
		value |= SP7350_OSD_HDR_BL |tmp_alpha;
	}

	if(info->alpha_info.color_key_en){
		value |= SP7350_OSD_HDR_KEY;
	}

	if (info->color_mode == SP7350_OSD_COLOR_MODE_8BPP)
		value |= SP7350_OSD_HDR_CULT;
	osd_header[0] = SWAP32(value);

	/* Fill disp_v_size & disp_h_size */
	tmp_width = info->region_info.act_width;
	tmp_height = info->region_info.act_height;
	DRM_DEBUG_DRIVER("wxh %dx%d\n", tmp_width, tmp_height);
	osd_header[1] = SWAP32(tmp_height << 16 | tmp_width << 0);

	/* Fill disp_start_row & disp_start_column */
	tmp_width = info->region_info.act_x;
	tmp_height = info->region_info.act_y;
	osd_header[2] = SWAP32(tmp_height << 16 | tmp_width << 0);

	/* Fill color key value */
	if(info->alpha_info.color_key_en){
		tmp_key = 0xffffffff & info->alpha_info.color_key;
		osd_header[3] = SWAP32(tmp_key);
	}else{
		osd_header[3] = 0;

	}

	/* Fill DATA_start_row & DATA_start_column */
	tmp_width = info->region_info.start_x;
	tmp_height = info->region_info.start_y;
	osd_header[4] = SWAP32(tmp_height << 16 | tmp_width << 0);

	/* Fill DATA_width and csc_mode_sel */
	value = 0;
	value |= info->region_info.buf_width;
	if (info->color_mode == SP7350_OSD_COLOR_MODE_YUY2)
		value |= SP7350_OSD_HDR_CSM_SET(SP7350_OSD_CSM_BYPASS);
	else
		value |= SP7350_OSD_HDR_CSM_SET(SP7350_OSD_CSM_RGB_BT601);
	osd_header[5] = SWAP32(value);

	/* Fill NEXT LINE header address NULL */
	osd_header[6] = SWAP32(0xFFFFFFE0);

	/* Fill OSD buffer data address */
	osd_header[7] = SWAP32((u32)info->buf_addr_phy);

	/*
	 * update sp7350 osd layer register
	 */
	value = 0;
	value = OSD_CTRL_COLOR_MODE_RGB
		| OSD_CTRL_CLUT_FMT_ARGB
		| OSD_CTRL_LATCH_EN
		| OSD_CTRL_A32B32_EN
		| OSD_CTRL_FIFO_DEPTH;
	SP7350_PLANE_WRITE((OSD_CTRL + (osd_layer_sel << 7)), value);

	SP7350_PLANE_WRITE((OSD_BASE_ADDR + (osd_layer_sel << 7)),
		sp_dev->osd_hdr_phy[osd_layer_sel]);

	SP7350_PLANE_WRITE((OSD_HVLD_OFFSET + (osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_VVLD_OFFSET + (osd_layer_sel << 7)), 0);

	SP7350_PLANE_WRITE((OSD_HVLD_WIDTH + (osd_layer_sel << 7)),
		info->region_info.act_width + info->region_info.act_x);
	SP7350_PLANE_WRITE((OSD_VVLD_HEIGHT + (osd_layer_sel << 7)),
		info->region_info.act_height + info->region_info.act_y);

	SP7350_PLANE_WRITE((OSD_BIST_CTRL + (osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_3D_H_OFFSET + (osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_SRC_DECIMATION_SEL + (osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_EN + (osd_layer_sel << 7)), 1);

	//GPOST bypass
	SP7350_PLANE_WRITE((GPOST_CONFIG + (osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((GPOST_MASTER_EN + (osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((GPOST_BG1 + (osd_layer_sel << 7)), 0x8010);
	SP7350_PLANE_WRITE((GPOST_BG2 + (osd_layer_sel << 7)), 0x0080);

	//GPOST PQ disable
	SP7350_PLANE_WRITE((GPOST_CONTRAST_CONFIG + (osd_layer_sel << 7)), 0);

}

#define SP7350_PLANE_TYPE_OSD 0
#define SP7350_PLANE_TYPE_VPP 1
static u32 sp7350_get_plane_format(u32 pixel_format, int plane_type)
{
	int i;

	if (plane_type == SP7350_PLANE_TYPE_VPP) {
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
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	DRM_DEBUG_ATOMIC("property.name:%s val:%llu\n", property->name, val);
	//DRM_INFO("%s property.values:%d\n", __func__, *property->values);

	if (sp_plane->type != DRM_PLANE_TYPE_OVERLAY) {
		DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
		return -EINVAL;
	}

	if (!strcmp(property->name, "region alpha")) {
		struct sp7350_plane_region_alpha_info *info;
		struct drm_property_blob *blob = drm_property_lookup_blob(plane->dev, val);

		if (!(sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND)) {
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
					 sp_plane->base.base.id, sp_plane->base.name,
					 blob->length);
			return -EINVAL;
		}
		info = blob->data;

		/* check value validity */
		if (info->alpha > 255 || info->alpha < 0) {
			DRM_DEBUG_ATOMIC("Outof limit! property alpha region [0, 255]!\n");
			return -EINVAL;
		}

		if (blob == sp_plane->state.region_alpha_blob)
			return 0;

		drm_property_blob_put(sp_plane->state.region_alpha_blob);
		sp_plane->state.region_alpha_blob = NULL;

		if (sp_plane->is_media_plane) {
			/*
			 * WARNING:
			 * vpp plane region alpha setting by vpp opif mask function.
			 * So only one region for media plane, the parameter "regionid" is invalid.
			 */
			DRM_DEBUG_ATOMIC("update vpp opif alpha value by the property!\n");
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
		sp_plane->state.region_alpha.regionid = info->regionid;
		sp_plane->state.region_alpha.alpha = info->alpha;
		sp_plane->state.region_alpha_blob = drm_property_blob_get(blob);

		DRM_DEBUG_ATOMIC("Set plane[%d] region alpha: regionid:%d, alpha:%d\n",
				 plane->index, info->regionid, info->alpha);

		drm_property_blob_put(blob);
	} else if (!strcmp(property->name, "color keying")) {
		u32 global_color_keying_val = val;

		if (!(sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		DRM_DEBUG_ATOMIC("Set plane[%d] color keying: 0x%08x\n", plane->index, global_color_keying_val);

		if (sp_plane->is_media_plane) {
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

		sp_plane->state.color_keying = global_color_keying_val;

	} else if (!strcmp(property->name, "region color keying")) {
		struct drm_property_blob *blob = drm_property_lookup_blob(plane->dev, val);
		struct sp7350_plane_region_color_keying_info *info;

		if (!(sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		if (blob->length != sizeof(struct sp7350_plane_region_color_keying_info)) {
			DRM_DEBUG_ATOMIC("[plane:%d:%s] bad mode blob length: %zu\n",
					 sp_plane->base.base.id, sp_plane->base.name,
					 blob->length);
			return -EINVAL;
		}
		info = blob->data;

		if (sp_plane->is_media_plane) {
			DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
			return -EINVAL;
		}

		if (blob == sp_plane->state.region_color_keying_blob)
			return 0;

		drm_property_blob_put(sp_plane->state.region_color_keying_blob);
		sp_plane->state.region_color_keying_blob = NULL;

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
		sp_plane->state.region_color_keying.regionid = info->regionid;
		sp_plane->state.region_color_keying.keying = info->keying;

		sp_plane->state.region_color_keying_blob = drm_property_blob_get(blob);
		DRM_DEBUG_ATOMIC("Set plane-%d region color keying: regionid:%d, keying:0x%08x\n",
				 plane->index, info->regionid, info->keying);

		drm_property_blob_put(blob);
	} else {
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
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	DRM_DEBUG_ATOMIC("Get plane-%d property.name:%s\n", plane->index, property->name);

	if (property == sp_plane->region_alpha_property) {
		if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND) {
			*val = (sp_plane->state.region_alpha_blob) ?
				 sp_plane->state.region_alpha_blob->base.id : 0;
			return 0;
		}
	} else if (property == sp_plane->region_color_keying_property) {
		if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING) {
			*val = (sp_plane->state.region_color_keying_blob) ?
				 sp_plane->state.region_color_keying_blob->base.id : 0;
			return 0;
		}
	} else if (property == sp_plane->color_keying_property) {
		if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING) {
			*val = sp_plane->state.color_keying;
			return 0;
		}
	}

	DRM_DEBUG_ATOMIC("the property \"%s\" isn't implemented for plane-%d!\n", property->name, plane->index);
	return -EINVAL;
}

static const struct drm_plane_funcs sp7350_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.atomic_set_property = sp7350_plane_atomic_set_property,
	.atomic_get_property = sp7350_plane_atomic_get_property,
};

#if SP7350_DRM_VPP_SCL_AUTO_ADJUST
static void sp7350_scl_coordinate_adjust(int32_t src_w, int32_t src_h,
	int32_t *dst_x, int32_t *dst_y, int32_t *dst_w, int32_t *dst_h)
{
	//int32_t factor_1000x;
	int32_t factor_1000x_w = 1000;
	int32_t factor_1000x_h = 1000;
	int32_t dst_val;

	factor_1000x_w = *dst_w * 1000 / src_w;
	factor_1000x_h = *dst_h * 1000 / src_h;
	if (factor_1000x_w < factor_1000x_h) {
		dst_val = *dst_h;
		*dst_h = src_h * factor_1000x_w / 1000;
		//*dst_h = (*dst_h / 16) *16;
		*dst_y += (dst_val - *dst_h) / 2;
	} else if (factor_1000x_w > factor_1000x_h) {
		dst_val = *dst_w;
		*dst_w = src_w * factor_1000x_h / 1000;
		*dst_w = (*dst_w / 16) * 16;
		*dst_x += (dst_val - *dst_w) / 2;
	}
}
#endif

static void sp7350_kms_plane_vpp_atomic_update(struct drm_plane *plane,
					       struct drm_plane_state *old_state)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	//struct drm_device *drm = sp_plane->base.dev;
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct drm_plane_state *state = plane->state;
	struct drm_gem_cma_object *obj = NULL;
	#if SP7350_DRM_VPP_SCL_AUTO_ADJUST
	int32_t dst_w;
	int32_t dst_h;
	int32_t dst_x;
	int32_t dst_y;
	#endif

	/* reference to ade_plane_atomic_update */
	if (!state->fb || !state->crtc) {
		/* do nothing */
		sp7350_dmix_layer_set(plane, SP7350_DMIX_VPP0, SP7350_DMIX_TRANSPARENT);
		return;
	}

	obj = drm_fb_cma_get_gem_obj(state->fb, 0);
	if (!obj || !obj->paddr) {
		DRM_DEBUG_ATOMIC("drm_fb_cma_get_gem_obj fail.\n");
		return;
	}

	DRM_DEBUG_ATOMIC("\n  src x,y:(%d, %d)  w,h:(%d, %d)\n crtc x,y:(%d, %d)  w,h:(%d, %d)",
			 state->src_x >> 16, state->src_y >> 16, state->src_w >> 16, state->src_h >> 16,
			 state->crtc_x, state->crtc_y, state->crtc_w, state->crtc_h);

	DRM_DEBUG_ATOMIC("plane-%d zpos:%d\n", plane->index, sp_plane->zpos);

	sp7350_vpp_imgread_set(plane, (u32)obj->paddr,
			       state->src_x >> 16, state->src_y >> 16,
			       state->src_w >> 16, state->src_h >> 16,
			       state->fb->width, state->fb->height,
			       sp7350_get_plane_format(state->fb->format->format, SP7350_PLANE_TYPE_VPP));

	#if SP7350_DRM_VPP_SCL_AUTO_ADJUST
	dst_x = state->crtc_x;
	dst_y = state->crtc_y;
	dst_w = state->crtc_w;
	dst_h = state->crtc_h;

	sp7350_scl_coordinate_adjust(state->src_w >> 16, state->src_h >> 16, &dst_x, &dst_y, &dst_w, &dst_h);
	DRM_DEBUG_ATOMIC("vscl adjust dst[%d, %d]\n", dst_w, dst_h);
	#endif
	sp7350_vpp_vscl_set(plane, state->src_x >> 16, state->src_y >> 16,
			    state->src_w >> 16, state->src_h >> 16,
			    #if SP7350_DRM_VPP_SCL_AUTO_ADJUST
			    dst_x, dst_y,
			    dst_w, dst_h,
			    #else
			    state->crtc_x, state->crtc_y,
			    state->crtc_w, state->crtc_h,
			    #endif
			    state->crtc->mode.hdisplay, state->crtc->mode.vdisplay);

	/* default setting for VPP OPIF(MASK function) */
	sp7350_vpp_vpost_opif_set(plane, state->crtc_x, state->crtc_y,
				  state->crtc_w, state->crtc_h,
				  state->crtc->mode.hdisplay, state->crtc->mode.vdisplay);
	/*
	 * for support letterbox boundary smoothly cropping,
	 * should update opif setting with another plane window size.
	 */

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND) {
		/*
		 * Auto resize alpha region from 0 ~ 0xffff to 0 ~ 0x3f.
		 *  0x00 ~ 0x3f remark as 0, 0x00xx ~ 0xffxx remark as 0 ~0x3f.
		 */
		DRM_DEBUG_ATOMIC("Set plane[%d] alpha %d(src:%d)\n",
				 plane->index, state->alpha >> 10, state->alpha);
		/* NOTES: vpp layer(SP7350_DMIX_L3) used for media plane fixed. */
		sp7350_dmix_plane_alpha_config(plane, SP7350_DMIX_L3, 1, 0, state->alpha >> 10);
	}

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND) {
		DRM_DEBUG_ATOMIC("Set plane[%d] region alpha: regionid:%d, alpha:%d\n",
				plane->index, sp_plane->state.region_alpha.regionid,
				sp_plane->state.region_alpha.alpha);
		/*
		 * WARNING:
		 * vpp plane region alpha setting by vpp opif mask function.
		 * So only one region for media plane, the parameter "regionid" is invalid.
		 */
		sp7350_vpp_vpost_opif_alpha_set(plane, sp_plane->state.region_alpha.alpha, 0);
	}

	sp7350_dmix_layer_set(plane, SP7350_DMIX_VPP0, SP7350_DMIX_BLENDING);
}

static void sp7350_kms_plane_osd_atomic_update(struct drm_plane *plane,
					       struct drm_plane_state *old_state)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	//struct drm_device *drm = sp_plane->base.dev;
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct drm_plane_state *state = plane->state;
	struct drm_gem_cma_object *obj = NULL;
	struct sp7350_osd_region info;
	int osd_layer_sel = 0;
	int layer = 0;
	struct drm_format_name_buf format_name;

	/* reference to ade_plane_atomic_update */
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

	if (!state->fb || !state->crtc) {
		/* disable this plane */
		sp7350_dmix_layer_set(plane, SP7350_DMIX_OSD0 + osd_layer_sel, SP7350_DMIX_TRANSPARENT);
		return;
	}

	obj = drm_fb_cma_get_gem_obj(state->fb, 0);
	if (!obj || !obj->paddr) {
		DRM_DEBUG_ATOMIC("drm_fb_cma_get_gem_obj fail.\n");
		return;
	}

	DRM_DEBUG_ATOMIC("\n  src x,y:(%d, %d)  w,h:(%d, %d)\n crtc x,y:(%d, %d)  w,h:(%d, %d)",
			 state->src_x >> 16, state->src_y >> 16, state->src_w >> 16, state->src_h >> 16,
			 state->crtc_x, state->crtc_y, state->crtc_w, state->crtc_h);

	memset(&info, 0, sizeof(info));
	info.color_mode = sp7350_get_plane_format(state->fb->format->format, SP7350_PLANE_TYPE_OSD);
	info.buf_addr_phy = (u32)obj->paddr;
	info.region_info.buf_width = state->fb->width;
	info.region_info.buf_height = state->fb->height;
	info.region_info.start_x = state->src_x >> 16;
	info.region_info.start_y = state->src_y >> 16;
	info.region_info.act_x = state->crtc_x;
	info.region_info.act_y = state->crtc_y;
	info.region_info.act_width = state->crtc_w;
	info.region_info.act_height = state->crtc_h;

	DRM_DEBUG_ATOMIC("plane-%d zpos:%d\n", plane->index, sp_plane->zpos);

	if (sp_plane->state.region_alpha_blob &&
		 (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_BLEND)) {
		DRM_DEBUG_ATOMIC("Set plane-%d region alpha: regionid:%d, alpha:%d\n",
				 plane->index, sp_plane->state.region_alpha.regionid,
				 sp_plane->state.region_alpha.alpha);
		/*
		 * TODO:
		 * osd plane region alpha setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		info.alpha_info.region_alpha_en = 1;
		info.alpha_info.region_alpha = sp_plane->state.region_alpha.alpha;
	} else {
		info.alpha_info.region_alpha_en = 0;
	}
	if (sp_plane->state.region_color_keying_blob &&
		 (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING)) {
		DRM_DEBUG_ATOMIC("Set plane[%d] region color keying: regionid:%d, keying:0x%08x\n",
				 plane->index, sp_plane->state.region_color_keying.regionid,
				 sp_plane->state.region_color_keying.keying);
		/*
		 * TODO:
		 * osd plane region alpha setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		info.alpha_info.color_key_en = 1;
		info.alpha_info.color_key = sp_plane->state.region_color_keying.keying;
	} else if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING) {
		DRM_DEBUG_ATOMIC("Set plane[%d] color keying:0x%08x\n",
				 plane->index, sp_plane->state.color_keying);
		if (!sp_plane->state.color_keying)
			info.alpha_info.color_key_en = 0;
		else
			info.alpha_info.color_key_en = 1;
		info.alpha_info.color_key = sp_plane->state.color_keying;
	}

	sp7350_osd_layer_set_by_region(plane, &info, osd_layer_sel);

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND) {
		/*
		 * Auto resize alpha region from 0 ~ 0xffff to 0 ~ 0x3f.
		 *  0x00 ~ 0x3f remark as 0, 0x00xx ~ 0xffxx remark as 0 ~0x3f.
		 */
		DRM_DEBUG_ATOMIC("Set plane[%d] alpha %d(src:%d)\n",
				 plane->index, state->alpha >> 10, state->alpha);

		sp7350_dmix_plane_alpha_config(plane, layer, 1, 0, state->alpha >> 10);
	}

	DRM_DEBUG_ATOMIC("\n set osd region x,y:(%d, %d)  w,h:(%d, %d)\n act x,y:(%d, %d)  w,h:(%d, %d)",
			 info.region_info.start_x, info.region_info.start_y,
			 info.region_info.buf_width, info.region_info.buf_height,
			 info.region_info.act_x, info.region_info.act_y,
			 info.region_info.act_width, info.region_info.act_height);

	DRM_DEBUG_ATOMIC("Pixel format %s, modifier 0x%llx, C3V format:0x%X\n",
			 drm_get_format_name(state->fb->format->format, &format_name),
			 state->fb->modifier, info.color_mode);

	sp7350_dmix_layer_set(plane, SP7350_DMIX_OSD0 + osd_layer_sel, SP7350_DMIX_BLENDING);
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

	if (((state->src_w >> 16) + (state->src_x >> 16)) > XRES_VIMGREAD_MAX
		|| ((state->src_h >> 16) + (state->src_y >> 16)) > YRES_VIMGREAD_MAX) {
		DRM_DEBUG_ATOMIC("Check fail[src(%d,%d)-(%d,%d)], outof limit[MAX %dx%d]!\n",
				state->src_x >> 16, state->src_y >> 16, state->src_w >> 16, state->src_h >> 16,
				XRES_VIMGREAD_MAX, YRES_VIMGREAD_MAX);
		return -EINVAL;
	}
	if ((state->src_w >> 16) > XRES_VSCL_MAX || (state->src_h >> 16) > YRES_VSCL_MAX) {
		DRM_DEBUG_ATOMIC("Check fail[src(%d,%d)], outof limit[MAX %dx%d]!\n",
				state->src_x >> 16, state->src_w >> 16, XRES_VSCL_MAX, YRES_VSCL_MAX);
		return -EINVAL;
	}
	if ((state->crtc_w + state->crtc_x) > XRES_VSCL_MAX || (state->crtc_h + state->crtc_y) > YRES_VSCL_MAX) {
		DRM_DEBUG_ATOMIC("Check fail[crtc(%d,%d)-(%d,%d)], outof limit[MAX %dx%d]!\n",
				state->crtc_x, state->crtc_y, state->crtc_w, state->crtc_h, XRES_VSCL_MAX, YRES_VSCL_MAX);
		return -EINVAL;
	}

	if (state->crtc_w % 16 /*|| state->crtc_h % 16*/) {
		DRM_DEBUG_ATOMIC("Check fail[crtc(%d,%d)], must be align to 16 byets.\n",
				state->crtc_w, state->crtc_h);
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

	if (((state->src_w >> 16) + (state->src_x >> 16)) > XRES_OSD_MAX
		|| ((state->src_h >> 16) + (state->src_y >> 16)) > YRES_OSD_MAX) {
		DRM_DEBUG_ATOMIC("Check fail[src(%d,%d)-(%d,%d)], outof limit[MAX %dx%d]!\n",
				state->src_x >> 16, state->src_y >> 16, state->src_w >> 16, state->src_h >> 16,
				XRES_OSD_MAX, YRES_OSD_MAX);
		return -EINVAL;
	}
	if ((state->crtc_w + state->crtc_x) > XRES_OSD_MAX || (state->crtc_h + state->crtc_y) > YRES_OSD_MAX) {
		DRM_DEBUG_ATOMIC("Check fail[crtc(%d,%d)-(%d,%d)], outof limit[MAX %dx%d]!\n",
				state->crtc_x, state->crtc_y, state->crtc_w, state->crtc_h, XRES_OSD_MAX, YRES_OSD_MAX);
		return -EINVAL;
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
	.atomic_update = sp7350_kms_plane_osd_atomic_update,
	.atomic_check  = sp7350_kms_plane_osd_atomic_check,
};

static void sp7350_plane_create_propertys(struct sp7350_plane *plane)
{

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_ZPOS)
		drm_plane_create_zpos_property(&plane->base, plane->zpos, 0, SP7350_MAX_PLANE - 1);
	else
		drm_plane_create_zpos_immutable_property(&plane->base, plane->zpos);

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_ROTATION) {
		drm_plane_create_rotation_property(&plane->base, DRM_MODE_ROTATE_0,
					  DRM_MODE_ROTATE_0 | DRM_MODE_ROTATE_90 | DRM_MODE_REFLECT_Y);
	}

	if (plane->capabilities & SP7350_DRM_PLANE_CAP_PIX_BLEND)
		drm_plane_create_blend_mode_property(&plane->base, BIT(DRM_MODE_BLEND_PREMULTI));

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

static void sp7350_plane_destroy_propertys(struct sp7350_plane *plane)
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

static const char * const sp_drm_plane_name[] = {
	"OVERLAY_PLANE", "PRIMARY_PLANE", "CURSOR_PLANE"};

static const char * const sp_plane_name[] = {
	"OSD3", "VPP0", "OSD2", "OSD1", "OSD0"};

struct drm_plane *sp7350_plane_init(struct drm_device *drm,
	enum drm_plane_type type, int sptype)
{
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct drm_plane *plane = NULL;
	struct sp7350_plane *sp_plane = NULL;
	//struct drm_crtc *crtc = NULL;
	int ret = 0;

	sp_plane = devm_kzalloc(drm->dev, sizeof(*sp_plane), GFP_KERNEL);
	if (!sp_plane)
		//return -ENOMEM;
		return ERR_PTR(-ENOMEM);

	plane = &sp_plane->base;

	if (sptype == SP7350_DRM_LAYER_TYPE_VPP0) {
		sp_plane->pixel_formats = (u32 *)&sp7350_kms_vpp_formats;
		sp_plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_vpp_formats);
		sp_plane->funcs = &sp7350_kms_vpp_helper_funcs;
		sp_plane->zpos = SP7350_DRM_LAYER_TYPE_VPP0;
		sp_plane->type = DRM_PLANE_TYPE_OVERLAY;
		sp_plane->is_media_plane = true;
		sp_plane->capabilities = SP7350_DRM_PLANE_CAP_SCALE |
			SP7350_DRM_PLANE_CAP_PIX_BLEND | SP7350_DRM_PLANE_CAP_WIN_BLEND |
			SP7350_DRM_PLANE_CAP_REGION_BLEND;
	} else {
		sp_plane->pixel_formats = (u32 *)&sp7350_kms_osd_formats;
		sp_plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_osd_formats);
		sp_plane->funcs = &sp7350_kms_osd_helper_funcs;
		sp_plane->zpos = sptype;
		/*
		 * it could be
		 * DRM_PLANE_TYPE_PRIMARY or
		 * DRM_PLANE_TYPE_OVERLAY or
		 * DRM_PLANE_TYPE_CURSOR
		 */
		sp_plane->type = type;
		sp_plane->is_media_plane = false;

		if (type == DRM_PLANE_TYPE_PRIMARY)
			sp_plane->capabilities = 0;
		else if (type == DRM_PLANE_TYPE_CURSOR)
			sp_plane->capabilities = 0;
		else
			sp_plane->capabilities = SP7350_DRM_PLANE_CAP_PIX_BLEND |
				SP7350_DRM_PLANE_CAP_WIN_BLEND | SP7350_DRM_PLANE_CAP_REGION_BLEND |
				SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING | SP7350_DRM_PLANE_CAP_COLOR_KEYING;
	}

	ret = drm_universal_plane_init(drm, plane, 0,
					&sp7350_plane_funcs,
				sp_plane->pixel_formats, sp_plane->num_pixel_formats,
				NULL, type, NULL);
	if (ret)
		return ERR_PTR(ret);

	drm_plane_helper_add(plane, sp_plane->funcs);

#if 0
	drm_plane_create_alpha_property(plane);
	drm_plane_create_rotation_property(plane, DRM_MODE_ROTATE_0,
					   DRM_MODE_ROTATE_0 |
					   DRM_MODE_ROTATE_180 |
					   DRM_MODE_REFLECT_X |
					   DRM_MODE_REFLECT_Y);
#endif

	//sp_plane->index = plane->base->index;
	sp_plane->base.index = plane->index;

	sp7350_plane_create_propertys(sp_plane);

	return plane;
}

int sp7350_plane_release(struct drm_device *drm, struct drm_plane *plane)
{
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	/* DO ANY??? */
	sp7350_plane_destroy_propertys(sp_plane);

	return 0;
}
