// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM Planes
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 *         hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_blend.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_plane_helper.h>

#include "sp7350_drm_drv.h"
#if defined(DRM_GEM_DMA_AVAILABLE)
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_gem_dma_helper.h>
#else
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#endif

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

#define SP7350_LAYER_OSD0   0x0
#define SP7350_LAYER_OSD1   0x1
#define SP7350_LAYER_OSD2   0x2
#define SP7350_LAYER_OSD3   0x3

#define SP7350_PLANE_READ(offset) readl(sp_dev->crtc_regs + (offset))
#define SP7350_PLANE_WRITE(offset, val) writel(val, sp_dev->crtc_regs + (offset))

/* sp7350-hw-format translate table */
struct sp7350_plane_format {
	u32 pixel_format;
	u32 hw_format;
};

static const u64 sp7350_kms_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	//DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 |
	//			AFBC_FORMAT_MOD_SPLIT |
	//			AFBC_FORMAT_MOD_SPARSE),
	DRM_FORMAT_MOD_INVALID,
};

static const u32 sp7350_kms_vpp_formats[5] = {
	DRM_FORMAT_YUYV,  /* SP7350_VPP_IMGREAD_DATA_FMT_YUY2 */
	DRM_FORMAT_UYVY,  /* SP7350_VPP_IMGREAD_DATA_FMT_UYVY */
	DRM_FORMAT_NV12,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV12 */
	DRM_FORMAT_NV16,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV16 */
	DRM_FORMAT_NV24,  /* SP7350_VPP_IMGREAD_DATA_FMT_NV24 */
};

static const u32 sp7350_kms_osd_formats[8] = {
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

static void sp7350_drm_plane_set(struct drm_plane *plane, int fg_sel, int layer_mode)
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
	value = value1 = SP7350_PLANE_READ(DMIX_LAYER_CONFIG_1);
	if (layer != SP7350_DMIX_BG) {
		value1 &= ~(GENMASK(1, 0) << ((layer - 1) << 1));
		value1 |= (layer_mode << ((layer - 1) << 1));
	}

	if (value != value1) {
		SP7350_PLANE_WRITE(DMIX_LAYER_CONFIG_1, value1);
		DRM_DEBUG_DRIVER("Update DMIX_LAYER_CONFIG_1:0x%X =>0x%X.\n", value, value1);
	}
	sp_plane->layer_mode = layer_mode;
}

static int sp7350_vpp_plane_vpost_opif_alpha_set(struct drm_plane *plane, int alpha, int mask_alpha)
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

static int sp7350_vpp_plane_imgread_set(struct drm_plane *plane, u32 data_addr1,
								 int x, int y, int img_src_w, int img_src_h,
								 int input_w, int input_h, int yuv_fmt)
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

static int sp7350_vpp_plane_vscl_set(struct drm_plane *plane, int x, int y, int img_src_w, int img_src_h,
							  int img_dest_x, int img_dest_y, int img_dest_w, int img_dest_h,
							  int output_w, int output_h)
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

	if (img_src_w < 128)
		img_src_w = 128;
	if (img_src_h < 128)
		img_src_h = 128;

	if (x > (img_src_w - 128))
		x = img_src_w / 2;
	if (y > (img_src_h - 128))
		y = img_src_h / 2;

	crop_xlen = img_src_w - x;
	crop_ylen = img_src_h - y;

	SP7350_PLANE_WRITE(VSCL_ACTRL_I_XLEN, crop_xlen);
	SP7350_PLANE_WRITE(VSCL_ACTRL_I_YLEN, crop_ylen);

	//#if (SP7350_VPP_SCALE_METHOD == 1) //method 1 , set imgread xstart&ystart, fix 0 here
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_XSTART, 0);
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_YSTART, 0);
	//#elif (SP7350_VPP_SCALE_METHOD == 2) //method 2 , set vscl xstart&ystart
	//SP7350_PLANE_WRITE(VSCL_ACTRL_S_XSTART, x);
	//SP7350_PLANE_WRITE(VSCL_ACTRL_S_YSTART, y);
	//#endif

	//#if 1 //crop left & top only
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_XLEN, crop_xlen);
	SP7350_PLANE_WRITE(VSCL_ACTRL_S_YLEN, crop_ylen);
	//#else //crop left & top & right & bot
	//SP7350_PLANE_WRITE(VSCL_ACTRL_S_XLEN, crop_xlen - x);
	//SP7350_PLANE_WRITE(VSCL_ACTRL_S_YLEN, crop_ylen - y);
	//#endif

	if (img_dest_x < 0)
		SP7350_PLANE_WRITE(VSCL_DCTRL_D_XSTART, 0-img_dest_x);
	else
		SP7350_PLANE_WRITE(VSCL_DCTRL_D_XSTART, img_dest_x);
	if (img_dest_y < 0)
		SP7350_PLANE_WRITE(VSCL_DCTRL_D_YSTART, 0-img_dest_y);
	else
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

int sp7350_vpp_plane_vpost_opif_set(struct drm_plane *plane, int act_x, int act_y,
									 int act_w, int act_h, int output_w, int output_h)
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
	if (act_y > 0)
		value |= SP7350_VPP_VPOST_OPIF_TOP_SET(act_y);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKTOP, value);

	value = SP7350_PLANE_READ(VPOST_OPIF_MSKBOT);
	value &= ~SP7350_VPP_VPOST_OPIF_BOT_MASK;
	if (act_h + act_y < output_h)
		value |= SP7350_VPP_VPOST_OPIF_BOT_SET(output_h - act_h - act_y);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKBOT, value);

	value = SP7350_PLANE_READ(VPOST_OPIF_MSKLEFT);
	value &= ~SP7350_VPP_VPOST_OPIF_LEFT_MASK;
	if (act_x > 0)
		value |= SP7350_VPP_VPOST_OPIF_LEFT_SET(act_x);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKLEFT, value);

	value = SP7350_PLANE_READ(VPOST_OPIF_MSKRIGHT);
	value &= ~SP7350_VPP_VPOST_OPIF_RIGHT_MASK;
	if (act_w + act_x < output_w)
		value |= SP7350_VPP_VPOST_OPIF_RIGHT_SET(output_w - act_w - act_x);
	SP7350_PLANE_WRITE(VPOST_OPIF_MSKRIGHT, value);

	return 0;
}

int sp7350_vpp_vpost_adj_enable(struct drm_plane *plane, int enable)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;


	value = SP7350_PLANE_READ(VPOST_CONFIG);
	if (enable)
		value |= SP7350_VPP_VPOST_ADJ_EN;
	else
		value &= ~SP7350_VPP_VPOST_ADJ_EN;

	SP7350_PLANE_WRITE(VPOST_CONFIG, value);

	return 0;
}

int sp7350_vpp_vpost_adj_turning_point_set(struct drm_plane *plane,
						const u8 *cp_src, const u8 *cp_sdt, u32 cp_size)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;

	if (cp_size != 2)
		return -1;

	value = 0;
	value = cp_src[0] + (cp_src[1] << 8);
	SP7350_PLANE_WRITE(VPOST_ADJ_SRC, value);

	value = 0;
	value = cp_sdt[0] + (cp_sdt[1] << 8);
	SP7350_PLANE_WRITE(VPOST_ADJ_DES, value);

	return 0;
}

int sp7350_vpp_vpost_adj_slope_set(struct drm_plane *plane, const u16 *slope, u32 slope_size)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;

	if (slope_size != 3)
		return -1;

	value = slope[0] & 0x1FF;
	SP7350_PLANE_WRITE(VPOST_ADJ_SLOPE0, value);

	value = slope[1] & 0x1FF;
	SP7350_PLANE_WRITE(VPOST_ADJ_SLOPE1, value);

	value = slope[2] & 0x1FF;
	SP7350_PLANE_WRITE(VPOST_ADJ_SLOPE2, value);

	return 0;
}

int sp7350_osd_gpost_contrast_adj_enable(struct drm_plane *plane, int enable)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;


	value = SP7350_PLANE_READ(GPOST_CONTRAST_CONFIG + (sp_plane->osd_layer_sel << 7));
	if (enable)
		value |= 0x1;
	else
		value &= ~0x1;

	SP7350_PLANE_WRITE(GPOST_CONTRAST_CONFIG + (sp_plane->osd_layer_sel << 7), value);

	return 0;
}

int sp7350_osd_gpost_contrast_adj_turning_point_set(struct drm_plane *plane,
						const u8 *cp_src, const u8 *cp_sdt, u32 cp_size)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;

	if (cp_size != 2)
		return -1;

	value = 0;
	value = cp_src[0] + (cp_src[1] << 8);
	SP7350_PLANE_WRITE(GPOST_ADJ_SRC + (sp_plane->osd_layer_sel << 7), value);

	value = 0;
	value = cp_sdt[0] + (cp_sdt[1] << 8);
	SP7350_PLANE_WRITE(GPOST_ADJ_DES + (sp_plane->osd_layer_sel << 7), value);

	return 0;
}

int sp7350_osd_gpost_contrast_adj_slope_set(struct drm_plane *plane,
						const u16 *slope, u32 slope_size)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;

	if (slope_size != 3)
		return -1;

	value = slope[0] & 0x1FF;
	SP7350_PLANE_WRITE(GPOST_ADJ_SLOPE0 + (sp_plane->osd_layer_sel << 7), value);

	value = slope[1] & 0x1FF;
	SP7350_PLANE_WRITE(GPOST_ADJ_SLOPE1 + (sp_plane->osd_layer_sel << 7), value);

	value = slope[2] & 0x1FF;
	SP7350_PLANE_WRITE(GPOST_ADJ_SLOPE2 + (sp_plane->osd_layer_sel << 7), value);

	return 0;
}

int sp7350_osd_gpost_brightness_adj_set(struct drm_plane *plane, int brightness)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;

	value = 0;
	value = brightness < 0 ? (0x80 | (brightness & 0x7F)) : (brightness & 0x7F);
	SP7350_PLANE_WRITE(GPOST_BRI_VALUE + (sp_plane->osd_layer_sel << 7), value);

	return 0;
}

int sp7350_vpp_vpost_adj_luma_boundary_set(struct drm_plane *plane, u8 upper, u8 lower)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct sp7350_dev *sp_dev = to_sp7350_dev(sp_plane->base.dev);
	u32 value;

	//value = SP7350_PLANE_READ(VPOST_ADJ_BOUND);
	value = 0;
	value = (upper << 8) | lower;
	SP7350_PLANE_WRITE(VPOST_ADJ_BOUND, value);

	/* New luma boundary enable, bit[3:2]
	 * 0x0 : Disable (default)
	 * 0x1 : Only use new lower boundary value
	 * 0x2 : Only use new upper boundary value
	 * 0x3 : Use both new boundary value
	 */
	value = SP7350_PLANE_READ(VPOST_ADJ_CONFIG);
	value |= (0x3 << 2);
	SP7350_PLANE_WRITE(VPOST_ADJ_CONFIG, value);

	return 0;
}

static u32 sp7350_plane_brightness_setting(struct drm_plane *plane, int brightness)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	if (sp_plane->is_media_plane) {
		u8 srcY[2];
		u8 destY[2];
		u16 slope[3];

		if (brightness >= 0) {
			srcY[0] = 0;
			srcY[1] = 0xFF - brightness;
			destY[0] = brightness;
			destY[1] = 0xFF;
		} else {
			srcY[0] = 0 - brightness;
			srcY[1] = 0xFF;
			destY[0] = 0;
			destY[1] = 0xFF + brightness;
		}

		slope[0] = slope[2] = 0;
		slope[1] = 0x100;
		sp7350_vpp_vpost_adj_turning_point_set(plane, srcY, destY, 2);
		sp7350_vpp_vpost_adj_slope_set(plane, slope, 3);
		sp7350_vpp_vpost_adj_enable(plane, 1);
	} else {
		/* [-128, 127] */
		sp7350_osd_gpost_brightness_adj_set(plane, brightness);
	}
	return 0;
}

static u32 sp7350_plane_contrast_setting(struct drm_plane *plane, int contrast)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	u8 srcY[2];
	u8 destY[2];
	u16 slope[3];

	if (contrast >= 0) {
		slope[0] = 0x100 - contrast;
		slope[1] = 0x100;
		slope[2] = 0x100 + contrast;
		srcY[0] = 0x55 + contrast / 6; /* 0xFF/3=0x55 */
		srcY[1] = 0xAA - contrast / 6; /* 0xFF/3*2=0xAA */
		destY[0] = srcY[0];
		destY[1] = srcY[1];
		//destY[0] = srcY[0]*slope[0]/256;
		//destY[1] = srcY[1]*slope[1]/256;
	} else {
		slope[0] = slope[2] = 0;
		slope[1] = 0x100 + contrast;
		srcY[0] = 0;
		srcY[1] = 0xFF;
		destY[0] = 0 - contrast / 2;
		destY[1] = 0xFF + contrast / 2;
	}

	if (sp_plane->is_media_plane) {
		sp7350_vpp_vpost_adj_turning_point_set(plane, srcY, destY, 2);
		sp7350_vpp_vpost_adj_slope_set(plane, slope, 3);
		sp7350_vpp_vpost_adj_enable(plane, 1);
	} else {
		sp7350_osd_gpost_contrast_adj_turning_point_set(plane, srcY, destY, 2);
		sp7350_osd_gpost_contrast_adj_slope_set(plane, slope, 3);
		sp7350_osd_gpost_contrast_adj_enable(plane, 1);
	}

	return 0;
}

static void sp7350_drm_plane_alpha_config(struct drm_plane *plane, int layer, int enable,
								    int fix_alpha, int alpha_value)
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

static void sp7350_osd_plane_init(struct drm_plane *plane)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	u32 value = 0;

	DRM_DEBUG_DRIVER("plane-%d init with osd-%d\n", plane->index, sp_plane->osd_layer_sel);
	//DRM_WARN("osd plane init(%dx%d)\n", plane->crtc->mode.hdisplay, plane->crtc->mode.vdisplay);

	/*
	 * update sp7350 osd layer register
	 */
	value = 0;
	value = OSD_CTRL_COLOR_MODE_RGB
		| OSD_CTRL_CLUT_FMT_ARGB
		| OSD_CTRL_LATCH_EN
		| OSD_CTRL_A32B32_EN
		| OSD_CTRL_FIFO_DEPTH;
	SP7350_PLANE_WRITE((OSD_CTRL + (sp_plane->osd_layer_sel << 7)), value);

	SP7350_PLANE_WRITE((OSD_HVLD_OFFSET + (sp_plane->osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_VVLD_OFFSET + (sp_plane->osd_layer_sel << 7)), 0);

	/* TODO: How get display mode? */
	//SP7350_PLANE_WRITE((OSD_HVLD_WIDTH + (sp_plane->osd_layer_sel << 7)),
	//	plane->crtc->mode.hdisplay);
	//SP7350_PLANE_WRITE((OSD_VVLD_HEIGHT + (sp_plane->osd_layer_sel << 7)),
	//	plane->crtc->mode.vdisplay);

	SP7350_PLANE_WRITE((OSD_BIST_CTRL + (sp_plane->osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_3D_H_OFFSET + (sp_plane->osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_SRC_DECIMATION_SEL + (sp_plane->osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((OSD_EN + (sp_plane->osd_layer_sel << 7)), 1);

	//GPOST bypass
	SP7350_PLANE_WRITE((GPOST_CONFIG + (sp_plane->osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((GPOST_MASTER_EN + (sp_plane->osd_layer_sel << 7)), 0);
	SP7350_PLANE_WRITE((GPOST_BG1 + (sp_plane->osd_layer_sel << 7)), 0x8010);
	SP7350_PLANE_WRITE((GPOST_BG2 + (sp_plane->osd_layer_sel << 7)), 0x0080);

	//GPOST PQ disable
	SP7350_PLANE_WRITE((GPOST_CONTRAST_CONFIG + (sp_plane->osd_layer_sel << 7)), 0);
}

static void sp7350_osd_plane_set_by_region(struct drm_plane *plane, struct sp7350_osd_region *info)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_device *drm = sp_plane->base.dev;
	struct sp7350_dev *sp_dev = to_sp7350_dev(drm);

	u32 tmp_width, tmp_height, tmp_color_mode;
	u32 value = 0;
	u32 tmp_key = 0;
	u32 *osd_header, *osd_palette;
	//int i;

	if (!sp_dev->osd_hdr[sp_plane->osd_layer_sel]) {
		pr_warn("%s osd%d not exist!!\n", __func__, sp_plane->osd_layer_sel);
		return;
	}

	osd_header = (u32 *)sp_dev->osd_hdr[sp_plane->osd_layer_sel];
	osd_palette = (u32 *)sp_dev->osd_hdr[sp_plane->osd_layer_sel] + 32;

	/*
	 * Fill OSD Layer Header info
	 */
	tmp_color_mode = info->color_mode;
	//tmp_alpha = SP7350_OSD_HDR_BL | SP7350_OSD_HDR_ALPHA;
	//value |= (tmp_color_mode << 24) | SP7350_OSD_HDR_BS | tmp_alpha;
	//value = osd_header[0];

	value |= (tmp_color_mode << 24) | SP7350_OSD_HDR_BS;

	/* setting region alpha */
	if (info->alpha_info.region_alpha_en)
		value |= SP7350_OSD_HDR_BL | info->alpha_info.region_alpha;

	if (info->alpha_info.color_key_en)
		value |= SP7350_OSD_HDR_KEY;

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
	if (info->alpha_info.color_key_en) {
		tmp_key = 0xffffffff & info->alpha_info.color_key;
		osd_header[3] = SWAP32(tmp_key);
	} else {
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

	SP7350_PLANE_WRITE((OSD_BASE_ADDR + (sp_plane->osd_layer_sel << 7)),
		sp_dev->osd_hdr_phy[sp_plane->osd_layer_sel]);

	/* TODO: Should set at init flow? */
	SP7350_PLANE_WRITE((OSD_HVLD_WIDTH + (sp_plane->osd_layer_sel << 7)),
		info->region_info.act_width + info->region_info.act_x);
	SP7350_PLANE_WRITE((OSD_VVLD_HEIGHT + (sp_plane->osd_layer_sel << 7)),
		info->region_info.act_height + info->region_info.act_y);
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
	struct sp7350_plane_state *sp_state = to_sp7350_plane_state(state);

	DRM_DEBUG_ATOMIC("Set plane-%d property.name:%s val:%llu\n",
					 plane->index, property->name, val);
	//DRM_INFO("%s property.values:%d\n", __func__, *property->values);

	//if (sp_plane->type != DRM_PLANE_TYPE_OVERLAY) {
	//	DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
	//	return -EINVAL;
	//}

	if (!strcmp(property->name, "win alpha")) {
		u32 win_alpha_val = val;

		if (!(sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		sp_state->win_alpha = win_alpha_val;
		DRM_DEBUG_ATOMIC("Set plane[%d] window alpha: %d\n",
						  plane->index, sp_state->win_alpha);

	} else if (!strcmp(property->name, "color keying")) {
		u32 global_color_keying_val = val;

		if (!(sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		DRM_DEBUG_ATOMIC("Set plane[%d] color keying: 0x%08x\n",
						  plane->index, global_color_keying_val);

		//if (sp_plane->is_media_plane) {
		//	DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
		//	return -EINVAL;
		//}

		DRM_DEBUG_ATOMIC("update color keying value by the property!\n");

		sp_state->color_keying = global_color_keying_val;

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

		//if (sp_plane->is_media_plane) {
		//	DRM_DEBUG_ATOMIC("the property is supported for overlay plane only!\n");
		//	return -EINVAL;
		//}

		if (blob == sp_plane->region_color_keying_blob)
			return 0;

		drm_property_blob_put(sp_plane->region_color_keying_blob);
		sp_plane->region_color_keying_blob = NULL;

		/*
		 * TODO:
		 * osd plane color keying setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		DRM_DEBUG_ATOMIC("update region color keying value by the property!\n");

		/*
		 * Save to plane state, but not update to hw reg.
		 * All hw reg updated at atomic update.
		 */
		sp_state->region_color_keying.regionid = info->regionid;
		sp_state->region_color_keying.keying = info->keying;

		sp_plane->region_color_keying_blob = drm_property_blob_get(blob);
		DRM_DEBUG_ATOMIC("Set plane-%d region color keying: regionid:%d, keying:0x%08x\n",
				 plane->index, info->regionid, info->keying);

		drm_property_blob_put(blob);

	} else if (!strcmp(property->name, "SCL_ADJ")) {
		if (!(sp_plane->capabilities & SP7350_DRM_PLANE_CAP_SCALE)) {
			DRM_DEBUG_ATOMIC("the property isn't supported by the driver!\n");
			return -EINVAL;
		}

		DRM_DEBUG_ATOMIC("Set plane[%d] %s scaling adjustment ability by the property.\n",
						  plane->index, val ? "enable" : "disable");

		sp_state->scaling_adjustment_enable = val ? true : false;

	} else if (!strcmp(property->name, "BG_ALPHA") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_BLEND)) {
		sp_state->bg_alpha = val;

	} else if (!strcmp(property->name, "BG_FORMAT") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_FORMAT)) {
		sp_state->bg_format = val;

	} else if (!strcmp(property->name, "BG_COLOR") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_COLOR)) {
		sp_state->bg_color = val;

	} else if (!strcmp(property->name, "brightness") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BRIGHTNESS)) {
		sp_state->brightness = val;
		if (sp_plane->is_media_plane && sp_state->contrast)
			sp_state->contrast = 0;

	} else if (!strcmp(property->name, "contrast") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_CONTRAST)) {
		sp_state->contrast = val;
		if (sp_plane->is_media_plane && sp_state->brightness)
			sp_state->brightness = 0;

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
	struct sp7350_plane_state *sp_state = to_sp7350_plane_state(state);

	DRM_DEBUG_ATOMIC("Get plane-%d property.name:%s\n", plane->index, property->name);

	if ((property == sp_plane->win_alpha_property) &&
		(sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND)) {
		*val = sp_state->win_alpha;

	} else if ((property == sp_plane->region_color_keying_property) &&
		       (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING)) {
		*val = (sp_plane->region_color_keying_blob) ?
			 sp_plane->region_color_keying_blob->base.id : 0;

	} else if ((property == sp_plane->color_keying_property) &&
		       (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING)) {
		*val = sp_state->color_keying;

	} else if (!strcmp(property->name, "SCL_ADJ") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_SCALE)) {
		*val = sp_state->scaling_adjustment_enable;

	} else if (!strcmp(property->name, "BG_ALPHA") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_BLEND)) {
		*val = sp_state->bg_alpha;

	} else if (!strcmp(property->name, "BG_FORMAT") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_FORMAT)) {
		*val = sp_state->bg_format;

	} else if (!strcmp(property->name, "BG_COLOR") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_COLOR)) {
		*val = sp_state->bg_color;

	} else if (!strcmp(property->name, "brightness") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BRIGHTNESS)) {
		*val = sp_state->brightness;

	} else if (!strcmp(property->name, "contrast") &&
		      (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_CONTRAST)) {
		*val = sp_state->contrast;

	} else {
		DRM_DEBUG_ATOMIC("the property \"%s\" isn't implemented for plane-%d!\n",
						 property->name, plane->index);
		return -EINVAL;
	}

	return 0;
}

/**
 * sp7350_plane_atomic_duplicate_state - sp7350 state duplicate hook
 * @plane: drm plane
 */
static struct drm_plane_state *
sp7350_plane_atomic_duplicate_state(struct drm_plane *plane)
{
	struct sp7350_plane_state *sp_state;

	if (WARN_ON(!plane->state))
		return NULL;

	DRM_DEBUG_ATOMIC("plane-%d atomic_duplicate_state.\n", plane->index);
	sp_state = kmemdup(to_sp7350_plane_state(plane->state),
			sizeof(*sp_state), GFP_KERNEL);
	if (!sp_state)
		return NULL;

	__drm_atomic_helper_plane_duplicate_state(plane, &sp_state->base);

	WARN_ON(sp_state->base.plane != plane);

	return &sp_state->base;
}

/**
 * sp7350_plane_atomic_destroy_state - sp7350 state destroy hook
 * @plane: drm plane
 * @state: plane state object to release
 */
static void sp7350_plane_atomic_destroy_state(struct drm_plane *plane,
					   struct drm_plane_state *state)
{
	struct sp7350_plane_state *sp_state = to_sp7350_plane_state(state);

	DRM_DEBUG_ATOMIC("plane-%d atomic_destroy_state.\n", plane->index);
	__drm_atomic_helper_plane_destroy_state(state);
	kfree(sp_state);
}

static void sp7350_plane_reset(struct drm_plane *plane)
{
	struct sp7350_plane_state *sp_state;
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	DRM_DEBUG_DRIVER("reset plane-%d state.\n", plane->index);

	WARN_ON(plane->state);

	if (plane->state)
		sp7350_plane_atomic_destroy_state(plane, plane->state);

	sp_state = kzalloc(sizeof(*sp_state), GFP_KERNEL);
	if (!sp_state)
		return;

	__drm_atomic_helper_plane_reset(plane, &sp_state->base);

	/* reset to default plane property parameters */
	sp_state->base.zpos = sp_plane->zpos;
	sp_state->base.normalized_zpos = sp_plane->zpos;
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_SCALE)
		sp_state->scaling_adjustment_enable = true;
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ROTATION)
		sp_state->base.rotation = DRM_MODE_ROTATE_0;
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_BLEND)
		sp_state->bg_alpha = 0;
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_FORMAT)
		sp_state->bg_format = 1;
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_COLOR)
		sp_state->bg_color = SP7350_DMIX_PTG_BLACK;
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BRIGHTNESS)
		sp_state->brightness = 0;
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_CONTRAST)
		sp_state->contrast = 0;

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ALPHA_BLEND) {
		sp_state->base.alpha = DRM_BLEND_ALPHA_OPAQUE;
		sp_plane->updated_alpha = sp_state->base.alpha;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND) {
		sp_state->win_alpha = 255;
		sp_plane->updated_win_alpha = sp_state->win_alpha;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING) {
		sp_state->region_color_keying.regionid = 0;
		sp_state->region_color_keying.keying = 0;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING)
		sp_state->color_keying = 0;

	/* clear old property setting. */
	if (sp_plane->region_color_keying_blob) {
		drm_property_blob_put(sp_plane->region_color_keying_blob);
		sp_plane->region_color_keying_blob = NULL;
	}

	if (sp_plane->is_media_plane) {
		sp_state->hdisplay = sp_plane->out_w_max;
		sp_state->vdisplay = sp_plane->out_h_max;
	} else {
		sp_state->hdisplay = XRES_OSD_MAX;
		sp_state->vdisplay = YRES_OSD_MAX;
	}
}

static const struct drm_plane_funcs sp7350_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = sp7350_plane_reset,
	.atomic_duplicate_state = sp7350_plane_atomic_duplicate_state,
	.atomic_destroy_state = sp7350_plane_atomic_destroy_state,
	.atomic_set_property = sp7350_plane_atomic_set_property,
	.atomic_get_property = sp7350_plane_atomic_get_property,
};

static void sp7350_scaling_coordinate_adjust(int32_t src_w, int32_t src_h,
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

static void sp7350_kms_plane_vpp_atomic_update(struct drm_plane *plane,
					       struct drm_atomic_state *state)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	//struct drm_device *drm = sp_plane->base.dev;
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct drm_plane_state *old_state = NULL;
	struct drm_plane_state *new_state = NULL;
	struct sp7350_plane_state *sp_state = NULL;
	struct sp7350_plane_state *old_sp_state = NULL;

	if (state) {
		old_state = drm_atomic_get_old_plane_state(state, plane);
		new_state = drm_atomic_get_new_plane_state(state, plane);
	} else {
		new_state = plane->state;
	}

	/* reference to ade_plane_atomic_update */
	if (!new_state || !new_state->fb || !new_state->crtc) {
		/* do nothing */
		DRM_DEBUG_ATOMIC("plane-%d, %d SP7350_DMIX_TRANSPARENT VPP.\n", plane->index, sp_plane->zpos);
		sp7350_drm_plane_set(plane, sp_plane->dmix_fg_sel, SP7350_DMIX_TRANSPARENT);
		return;
	}
	DRM_DEBUG_ATOMIC("plane-%d zpos:%d\n", plane->index, sp_plane->zpos);
	sp_state = to_sp7350_plane_state(new_state);
	if (old_state)
		old_sp_state = to_sp7350_plane_state(old_state);

	/* Check parameter updates first  */
	/* for crop state check */
	if (!old_state || new_state->fb != old_state->fb
		|| new_state->src_x != old_state->src_x || new_state->src_y != old_state->src_y
		|| new_state->src_w != old_state->src_w || new_state->src_h != old_state->src_h) {
		#if defined(DRM_GEM_DMA_AVAILABLE)
		struct drm_gem_dma_object *obj = drm_fb_dma_get_gem_obj(new_state->fb, 0);
		dma_addr_t paddr = obj->dma_addr;
		#else
		struct drm_gem_cma_object *obj = drm_fb_cma_get_gem_obj(new_state->fb, 0);
		dma_addr_t paddr = obj->paddr;
		#endif

		if (!obj || !paddr) {
			DRM_DEBUG_ATOMIC("plane-%d drm_fb_cma_get_gem_obj fail.\n", plane->index);
			return;
		}

		DRM_DEBUG_ATOMIC("plane-%d\n  src x,y:(%d, %d)  w,h:(%d, %d)\n"
						 " crtc x,y:(%d, %d)  w,h:(%d, %d)",
				 plane->index,
				 new_state->src_x >> 16, new_state->src_y >> 16,
				 new_state->src_w >> 16, new_state->src_h >> 16,
				 new_state->crtc_x, new_state->crtc_y,
				 new_state->crtc_w, new_state->crtc_h);

		sp7350_vpp_plane_imgread_set(plane, (u32)paddr,
				 new_state->src_x >> 16, new_state->src_y >> 16,
				 new_state->src_w >> 16, new_state->src_h >> 16,
				 new_state->fb->width, new_state->fb->height,
				 sp7350_get_plane_format(new_state->fb->format->format, SP7350_PLANE_TYPE_VPP));
	}

	/* for scale state check */
	if (!old_state || new_state->src_x != old_state->src_x || new_state->src_y != old_state->src_y
		|| new_state->src_w != old_state->src_w || new_state->src_h != old_state->src_h
		|| new_state->crtc_x != old_state->crtc_x || new_state->crtc_y != old_state->crtc_y
		|| new_state->crtc_w != old_state->crtc_w || new_state->crtc_h != old_state->crtc_h
		|| (old_state->crtc && (new_state->crtc->mode.hdisplay != old_state->crtc->mode.hdisplay
		     || new_state->crtc->mode.vdisplay != old_state->crtc->mode.vdisplay))) {
		if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ALPHA_BLEND)
			&& sp_state->scaling_adjustment_enable) {
			sp7350_scaling_coordinate_adjust(new_state->src_w >> 16, new_state->src_h >> 16,
				  &new_state->crtc_x, &new_state->crtc_y, &new_state->crtc_w, &new_state->crtc_h);
			DRM_DEBUG_ATOMIC("plane-%d scaling adjust to crtc x,y:(%d, %d)  w,h:(%d, %d)\n",
					 plane->index,
					 new_state->crtc_x, new_state->crtc_y,
					 new_state->crtc_w, new_state->crtc_h);
		}
		sp7350_vpp_plane_vscl_set(plane, new_state->src_x >> 16, new_state->src_y >> 16,
				    new_state->src_w >> 16, new_state->src_h >> 16,
				    new_state->crtc_x, new_state->crtc_y,
				    new_state->crtc_w, new_state->crtc_h,
				    new_state->crtc_w + new_state->crtc_x,
				    new_state->crtc_h + new_state->crtc_y);

		/* default setting for VPP OPIF(MASK function) */
		sp7350_vpp_plane_vpost_opif_set(plane, new_state->crtc_x, new_state->crtc_y,
					  new_state->crtc_w, new_state->crtc_h,
					  new_state->crtc_w + new_state->crtc_x,
					  new_state->crtc_h + new_state->crtc_y);
	}

	/*
	 * for support letterbox boundary smoothly cropping,
	 * should update opif setting with another plane window size.
	 */

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ALPHA_BLEND)
		&& (new_state->alpha != sp_plane->updated_alpha)) {
		/*
		 * Auto resize alpha region from 0 ~ 0xffff to 0 ~ 0x3f.
		 *  0x00 ~ 0x3f remark as 0, 0x00xx ~ 0xffxx remark as 0 ~ 0x3f.
		 */
		DRM_DEBUG_ATOMIC("Set plane[%d] alpha %d(src:%d)\n",
				 plane->index, new_state->alpha >> 10, new_state->alpha);
		/* NOTES: vpp layer(SP7350_DMIX_L3) used for media plane fixed. */
		sp7350_drm_plane_alpha_config(plane, sp_plane->dmix_layer, 1, 0, new_state->alpha >> 10);
		sp_plane->updated_alpha = new_state->alpha;
	}

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND)
		&& (sp_state->win_alpha != sp_plane->updated_win_alpha)) {
		DRM_DEBUG_ATOMIC("Set plane[%d] window alpha: alpha:%d\n",
				plane->index, sp_state->win_alpha);

		sp7350_vpp_plane_vpost_opif_alpha_set(plane, sp_state->win_alpha, 0);
		sp_plane->updated_win_alpha = sp_state->win_alpha;
	}

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BRIGHTNESS)
		&& (!old_sp_state || (!sp_state->contrast &&
		    (sp_state->brightness != old_sp_state->brightness)))) {
		sp7350_plane_brightness_setting(plane, sp_state->brightness);
	}

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_CONTRAST)
		&& (!old_sp_state || (!sp_state->brightness &&
		   (sp_state->contrast != old_sp_state->contrast)))) {
		sp7350_plane_contrast_setting(plane, sp_state->contrast);
	}

	sp7350_drm_plane_set(plane, sp_plane->dmix_fg_sel, SP7350_DMIX_BLENDING);
}

static void sp7350_kms_plane_osd_atomic_update(struct drm_plane *plane,
					       struct drm_atomic_state *state)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	//struct drm_device *drm = sp_plane->base.dev;
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct drm_plane_state *old_state = NULL;
	struct drm_plane_state *new_state = NULL;
	struct sp7350_plane_state *sp_state = NULL;
	struct sp7350_plane_state *old_sp_state = NULL;
	struct sp7350_osd_region *info = NULL;
	//struct drm_format_name_buf format_name;
	bool updated = false;

	if (state) {
		old_state = drm_atomic_get_old_plane_state(state, plane);
		new_state = drm_atomic_get_new_plane_state(state, plane);
	} else {
		new_state = plane->state;
	}

	if (!new_state || !new_state->fb || !new_state->crtc) {
		/* disable this plane */
		DRM_DEBUG_ATOMIC("plane-%d, %d SP7350_DMIX_TRANSPARENT OSD.\n", plane->index, sp_plane->zpos);
		sp7350_drm_plane_set(plane, sp_plane->dmix_fg_sel, SP7350_DMIX_TRANSPARENT);
		return;
	}

	DRM_DEBUG_ATOMIC("plane-%d zpos:%d\n", plane->index, sp_plane->zpos);
	sp_state = to_sp7350_plane_state(new_state);
	if (old_state)
		old_sp_state = to_sp7350_plane_state(old_state);
	info = &sp_state->info;

	/* Check parameter updates first  */
	if (!old_state || new_state->fb != old_state->fb) {
		/* Check parameter updates first  */
		#if defined(DRM_GEM_DMA_AVAILABLE)
		struct drm_gem_dma_object *obj = drm_fb_dma_get_gem_obj(new_state->fb, 0);
		dma_addr_t paddr = obj->dma_addr;
		#else
		struct drm_gem_cma_object *obj = drm_fb_cma_get_gem_obj(new_state->fb, 0);
		dma_addr_t paddr = obj->paddr;
		#endif

		DRM_DEBUG_ATOMIC("plane-%d update fb (%dx%d)",
				 plane->index, new_state->fb->width, new_state->fb->height);

		if (!obj || !paddr) {
			DRM_DEBUG_ATOMIC("plane-%d drm_fb_cma_get_gem_obj fail.\n", plane->index);
			return;
		}
		updated = true;
		info->buf_addr_phy = (u32)paddr;
		info->region_info.buf_width = new_state->fb->width;
		info->region_info.buf_height = new_state->fb->height;
	}

	/* for crop and pan state check */
	if (!old_state
		|| new_state->src_x != old_state->src_x || new_state->src_y != old_state->src_y
		|| new_state->src_w != old_state->src_w || new_state->src_h != old_state->src_h
		|| new_state->crtc_x != old_state->crtc_x || new_state->crtc_y != old_state->crtc_y
		|| new_state->crtc_w != old_state->crtc_w || new_state->crtc_h != old_state->crtc_h) {
		DRM_DEBUG_ATOMIC("plane-%d:\n  src x,y:(%d, %d)  w,h:(%d, %d)\n"
						 "crtc x,y:(%d, %d)  w,h:(%d, %d)",
				 plane->index,
				 new_state->src_x >> 16, new_state->src_y >> 16,
				 new_state->src_w >> 16, new_state->src_h >> 16,
				 new_state->crtc_x, new_state->crtc_y,
				 new_state->crtc_w, new_state->crtc_h);

		updated = true;
		info->region_info.start_x = new_state->src_x >> 16;
		info->region_info.start_y = new_state->src_y >> 16;
		info->region_info.act_x = new_state->crtc_x;
		info->region_info.act_y = new_state->crtc_y;
		info->region_info.act_width = new_state->crtc_w;
		info->region_info.act_height = new_state->crtc_h;
		if (new_state->crtc_x < 0) {
			info->region_info.act_x = 0;
			info->region_info.start_x -= new_state->crtc_x;
			info->region_info.act_width += new_state->crtc_x;
		} else if ((new_state->crtc_x + new_state->crtc_w) > sp_state->hdisplay) {
			info->region_info.act_width -=
				(new_state->crtc_x + new_state->crtc_w - sp_state->hdisplay);
		}
		if (new_state->crtc_y < 0) {
			info->region_info.act_y = 0;
			info->region_info.start_y -= new_state->crtc_y;
			info->region_info.act_height += new_state->crtc_y;
		} else if ((new_state->crtc_y + new_state->crtc_h) > sp_state->vdisplay) {
			info->region_info.act_height -=
				(new_state->crtc_y + new_state->crtc_h - sp_state->vdisplay);
		}
	}

	/* no scale state */

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND)
		&& (sp_state->win_alpha != sp_plane->updated_win_alpha)) {
		DRM_DEBUG_ATOMIC("Set plane[%d] window alpha: alpha:%d\n",
				plane->index, sp_state->win_alpha);
		/*
		 * TODO:
		 * osd plane region alpha setting by osd region header.
		 * So support muti-region, the parameter "regionid" is related to osd region.
		 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
		 */
		info->alpha_info.region_alpha_en = 1;
		info->alpha_info.region_alpha = sp_state->win_alpha;
		sp_plane->updated_win_alpha = sp_state->win_alpha;
		updated = true;
	}

	if (sp_plane->region_color_keying_blob) {
		/* color_key from region_color_keying */
		if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING) &&
			(sp_state->region_color_keying.regionid !=
			   sp_plane->updated_region_color_keying.regionid ||
			 sp_state->region_color_keying.keying !=
			    sp_plane->updated_region_color_keying.keying)) {
			DRM_DEBUG_ATOMIC("Set plane[%d] region color keying: regionid:%d, keying:0x%08x\n",
					 plane->index, sp_state->region_color_keying.regionid,
					 sp_state->region_color_keying.keying);
			/*
			 * TODO:
			 * osd plane region alpha setting by osd region header.
			 * So support muti-region, the parameter "regionid" is related to osd region.
			 *  BUT now, only one osd region support, so the parameter "regionid" is invalid now.
			 */
			info->alpha_info.color_key_en = 1;
			info->alpha_info.color_key = sp_state->region_color_keying.keying;
			sp_plane->updated_region_color_keying.regionid = sp_state->region_color_keying.regionid;
			sp_plane->updated_region_color_keying.keying = sp_state->region_color_keying.keying;
			updated = true;
		}
	} else {
		/* color_key from global color_keying */
		if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING)
			&& (sp_state->color_keying != sp_plane->updated_color_keying)) {
			DRM_DEBUG_ATOMIC("Set plane[%d] color keying:0x%08x\n",
					 plane->index, sp_state->color_keying);
			if (!sp_state->color_keying)
				info->alpha_info.color_key_en = 0;
			else
				info->alpha_info.color_key_en = 1;
			info->alpha_info.color_key = sp_state->color_keying;
			sp_plane->updated_color_keying = sp_state->color_keying;
			updated = true;
		}
	}

	if (updated) {
		info->color_mode = sp7350_get_plane_format(new_state->fb->format->format,
												   SP7350_PLANE_TYPE_OSD);
		sp7350_osd_plane_set_by_region(plane, info);

		DRM_DEBUG_ATOMIC("plane-%d set osd region\n"
						 "buf x,y:(%d, %d)  w,h:(%d, %d)\n act x,y:(%d, %d)  w,h:(%d, %d)",
				 plane->index,
				 info->region_info.start_x, info->region_info.start_y,
				 info->region_info.buf_width, info->region_info.buf_height,
				 info->region_info.act_x, info->region_info.act_y,
				 info->region_info.act_width, info->region_info.act_height);
		DRM_DEBUG_ATOMIC("plane-%d Pixel format %4.4s, modifier 0x%llx, C3V format:0x%X\n",
				 plane->index, (char *)&new_state->fb->format->format,
				 new_state->fb->modifier, info->color_mode);
		//DRM_DEBUG_ATOMIC("plane-%d Pixel format %4.4s, modifier 0x%llx, C3V format:0x%X\n",
		//		 plane->index,
		//		 drm_get_format_name(state->fb->format->format, &format_name),
		//		 state->fb->modifier, info->color_mode);
	}

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ALPHA_BLEND)
		&& (new_state->alpha != sp_plane->updated_alpha)) {
		/*
		 * Auto resize alpha region from 0 ~ 0xffff to 0 ~ 0x3f.
		 *  0x00 ~ 0x3f remark as 0, 0x00xx ~ 0xffxx remark as 0 ~0x3f.
		 */
		DRM_DEBUG_ATOMIC("Set plane[%d] alpha %d(src:%d)\n",
				 plane->index, new_state->alpha >> 10, new_state->alpha);
		sp7350_drm_plane_alpha_config(plane, sp_plane->dmix_layer, 1, 0, new_state->alpha >> 10);
		sp_plane->updated_alpha = new_state->alpha;
	}

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BRIGHTNESS)
		&& (!old_sp_state || sp_state->brightness != old_sp_state->brightness)) {
		sp7350_plane_brightness_setting(plane, sp_state->brightness);
	}

	if ((sp_plane->capabilities & SP7350_DRM_PLANE_CAP_CONTRAST)
		&& (!old_sp_state || sp_state->contrast != old_sp_state->contrast)) {
		sp7350_plane_contrast_setting(plane, sp_state->contrast);
	}

	sp7350_drm_plane_set(plane, sp_plane->dmix_fg_sel, SP7350_DMIX_BLENDING);
}

static int sp7350_kms_plane_atomic_check(struct drm_plane *plane,
					     struct drm_atomic_state *state)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);
	struct drm_plane_state *new_state = drm_atomic_get_new_plane_state(state, plane);
	struct sp7350_plane_state *sp_state = to_sp7350_plane_state(new_state);
	struct drm_crtc_state *crtc_state = NULL;

	if (!new_state->fb || WARN_ON(!new_state->crtc)) {
		DRM_DEBUG_ATOMIC("plane-%d return 0.\n", plane->index);
		return 0;
	}

	DRM_DEBUG_ATOMIC("plane-%d zpos:%d\n", plane->index, new_state->zpos);
	crtc_state = drm_atomic_get_crtc_state(new_state->state, new_state->crtc);
	if (IS_ERR(crtc_state)) {
		DRM_DEBUG_ATOMIC("plane-%d drm_atomic_get_crtc_state is err\n", plane->index);
		return PTR_ERR(crtc_state);
	}
	if (crtc_state->adjusted_mode.crtc_hdisplay && crtc_state->adjusted_mode.crtc_vdisplay) {
		sp_state->hdisplay = crtc_state->adjusted_mode.crtc_hdisplay;
		sp_state->vdisplay = crtc_state->adjusted_mode.crtc_vdisplay;
	}

	if ((new_state->crtc_w + new_state->crtc_x) <= 0 ||
		(new_state->crtc_h + new_state->crtc_y) <= 0 ||
		new_state->crtc_x >= sp_state->hdisplay ||
		new_state->crtc_y >= sp_state->vdisplay) {
		DRM_DEBUG_ATOMIC("plane-%d no visible areas.[crtc(%d,%d)-(%d,%d)]\n",
				plane->index,
				new_state->crtc_x, new_state->crtc_y,
				new_state->crtc_w, new_state->crtc_h);
		return -EINVAL;
	}

	/* [FIXME]check for VPP layer only!!!!WHY??? */
	if (sp_plane->is_media_plane && (new_state->crtc_y < -150 ||
		new_state->crtc_w + new_state->crtc_x - 110 > sp_state->hdisplay)) {
		DRM_DEBUG_ATOMIC("plane-%d invalid visible areas.[crtc(%d,%d)-(%d,%d)]\n",
				plane->index,
				new_state->crtc_x, new_state->crtc_y,
				new_state->crtc_w, new_state->crtc_h);
		return -EINVAL;
	}

	if (((new_state->src_w >> 16) + (new_state->src_x >> 16)) > sp_plane->src_w_max
		|| ((new_state->src_h >> 16) + (new_state->src_y >> 16)) > sp_plane->src_h_max) {
		DRM_DEBUG_ATOMIC("plane-%d Check SRC fail[src(%d,%d)-(%d,%d)], outof limit[MAX %dx%d]!\n",
				plane->index,
				new_state->src_x >> 16, new_state->src_y >> 16,
				new_state->src_w >> 16, new_state->src_h >> 16,
				sp_plane->src_w_max, sp_plane->src_h_max);
		return -EINVAL;
	}

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_SCALE) {
		if ((new_state->src_w >> 16) > sp_plane->scl_w_max ||
			(new_state->src_h >> 16) > sp_plane->scl_h_max) {
			DRM_DEBUG_ATOMIC("plane-%d Check VSCL Fail[src(%d,%d)], outof limit[MAX %dx%d]!\n",
					plane->index,
					new_state->src_x >> 16, new_state->src_w >> 16,
					sp_plane->scl_w_max, sp_plane->scl_h_max);
			return -EINVAL;
		}
	} else {
		if (new_state->crtc_w != new_state->src_w >> 16
			|| new_state->crtc_h != new_state->src_h >> 16) {
			DRM_DEBUG_ATOMIC("plane-%d Check VSCL fail[src(%d, %d), crtc(%d,%d)]",
					 plane->index,
					 new_state->src_w >> 16, new_state->src_h >> 16,
					 new_state->crtc_w, new_state->crtc_h);
			DRM_DEBUG_ATOMIC("plane-%d scale function unsuppord.\n", plane->index);
			return -EINVAL;
		}
	}

	/* Check HW Restricted. */
	if ((new_state->crtc_w + new_state->crtc_x) > sp_plane->out_w_max
		|| (new_state->crtc_h + new_state->crtc_y) > sp_plane->out_h_max) {
		DRM_DEBUG_ATOMIC("plane-%d Check OUT fail[crtc(%d,%d)-(%d,%d)], outof limit[MAX %dx%d]!\n",
				plane->index,
				new_state->crtc_x, new_state->crtc_y,
				new_state->crtc_w, new_state->crtc_h,
				sp_plane->out_w_max, sp_plane->out_h_max);
		return -EINVAL;
	}

	//DRM_DEBUG_ATOMIC("plane-%d Pixel format %4.4s, %s, Block %dx%d, subsampling factor (%u, %u)\n",
	//		 plane->index, (char *)&new_state->fb->format->format,
	//		 new_state->fb->format->is_yuv ? "YUV" : "RGB",
	//		 drm_format_info_block_width(new_state->fb->format,0),
	//		 drm_format_info_block_height(new_state->fb->format,0),
	//		 new_state->fb->format->hsub, new_state->fb->format->vsub);
	if (new_state->fb->format->is_yuv) {
		if (new_state->crtc_w % 16 /*|| state->crtc_h % 16*/) {
			DRM_DEBUG_ATOMIC("plane-%d Check fail[crtc(%d,%d)], must be align to 16 byets.\n",
					plane->index,
					new_state->crtc_w, new_state->crtc_h);
			return -EINVAL;
		}
	}

	DRM_DEBUG_ATOMIC("plane-%d atomic check end\n", plane->index);

	return 0;
}

static const struct drm_plane_helper_funcs sp7350_kms_vpp_helper_funcs = {
	.atomic_update = sp7350_kms_plane_vpp_atomic_update,
	.atomic_check  = sp7350_kms_plane_atomic_check,
};

static const struct drm_plane_helper_funcs sp7350_kms_osd_helper_funcs = {
	.atomic_update = sp7350_kms_plane_osd_atomic_update,
	.atomic_check  = sp7350_kms_plane_atomic_check,
};

static void sp7350_plane_create_propertys(struct sp7350_plane *sp_plane)
{

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_SCALE) {
		sp_plane->scaling_adjustment_property =
			 drm_property_create_bool(sp_plane->base.dev, DRM_MODE_PROP_ATOMIC,
					 "SCL_ADJ");
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->scaling_adjustment_property, 1);
		if (sp_plane->state)
			sp_plane->state->scaling_adjustment_enable = true;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_BLEND) {
		sp_plane->background_alpha_property =
			 drm_property_create_range(sp_plane->base.dev, DRM_MODE_PROP_ATOMIC,
					 "BG_ALPHA", 0, 255);
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->background_alpha_property, 0);
		if (sp_plane->state)
			sp_plane->state->bg_alpha = 0;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_FORMAT) {
		sp_plane->background_format_property = drm_property_create(sp_plane->base.dev,
			 DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_IMMUTABLE | DRM_MODE_PROP_ENUM, "BG_FORMAT", 1);
		drm_property_add_enum(sp_plane->background_format_property,
					    1, "YCbCr888");
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->background_format_property, 1);
		if (sp_plane->state)
			sp_plane->state->bg_format = 1;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BG_COLOR) {
		/* background color format:YCbCr888, region: 0x000000~0xffffff */
		sp_plane->background_color_property = drm_property_create_range(sp_plane->base.dev,
					 DRM_MODE_PROP_ATOMIC, "BG_COLOR", 0, 0xffffffff);
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->background_color_property, SP7350_DMIX_PTG_BLACK);
		if (sp_plane->state)
			sp_plane->state->bg_color = SP7350_DMIX_PTG_BLACK;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_BRIGHTNESS) {
		/* region: 0~100 */
		if (sp_plane->is_media_plane)
			sp_plane->brightness_property = drm_property_create_signed_range(sp_plane->base.dev,
						 DRM_MODE_PROP_ATOMIC, "brightness", -255, 255);
		else
			sp_plane->brightness_property = drm_property_create_signed_range(sp_plane->base.dev,
						 DRM_MODE_PROP_ATOMIC, "brightness", -128, 127);
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->brightness_property, 0);
		if (sp_plane->state)
			sp_plane->state->brightness = 0;
	}
	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_CONTRAST) {
		/* region: 0~100 */
		sp_plane->contrast_property = drm_property_create_signed_range(sp_plane->base.dev,
					 DRM_MODE_PROP_ATOMIC, "contrast", -255, 255);
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->contrast_property, 0);
		if (sp_plane->state)
			sp_plane->state->contrast = 0;
	}

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ZPOS)
		/* SP7350_MAX_PLANE - 2, cursor not support adjustable zpos */
		drm_plane_create_zpos_property(&sp_plane->base, sp_plane->zpos, 0, SP7350_MAX_PLANE - 2);
	else
		drm_plane_create_zpos_immutable_property(&sp_plane->base, sp_plane->zpos);

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ROTATION) {
		drm_plane_create_rotation_property(&sp_plane->base, DRM_MODE_ROTATE_0,
					  DRM_MODE_ROTATE_0 | DRM_MODE_ROTATE_90 | DRM_MODE_REFLECT_Y);
	}

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_PIX_BLEND)
		drm_plane_create_blend_mode_property(&sp_plane->base, BIT(DRM_MODE_BLEND_PREMULTI));

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_ALPHA_BLEND)
		drm_plane_create_alpha_property(&sp_plane->base);

	if (sp_plane->base.state)
		DRM_DEBUG_ATOMIC("plane->state->alpha=%d\n", sp_plane->base.state->alpha);
	else
		DRM_DEBUG_ATOMIC("alpha no default value.\n");

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_WIN_BLEND) {
		/* region alpha region: 0~255 */
		sp_plane->win_alpha_property
			 = drm_property_create_range(sp_plane->base.dev, DRM_MODE_PROP_ATOMIC,
						  "win alpha", 0, 0xff);
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->win_alpha_property, 0xff);
		if (sp_plane->state)
			sp_plane->state->win_alpha = 255;
	}

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_REGION_COLOR_KEYING) {
		/* region color keying region: 0~0xffffffff */
		sp_plane->region_color_keying_property =
			 drm_property_create(sp_plane->base.dev,
						  DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
						  "region color keying", 0);
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->region_color_keying_property, 0);
		if (sp_plane->state) {
			sp_plane->state->region_color_keying.regionid = 0;
			sp_plane->state->region_color_keying.keying = 0;
		}
	}

	if (sp_plane->capabilities & SP7350_DRM_PLANE_CAP_COLOR_KEYING) {
		/* color_keying format:RGBA8888, region: 0~0xffffffff */
		sp_plane->color_keying_property =
			 drm_property_create_range(sp_plane->base.dev, DRM_MODE_PROP_ATOMIC,
					 "color keying", 0, 0xffffffff);
		drm_object_attach_property(&sp_plane->base.base,
			 sp_plane->color_keying_property, 0);
		if (sp_plane->state)
			sp_plane->state->color_keying = 0;
	}
}

static void sp7350_plane_destroy_propertys(struct sp7350_plane *sp_plane)
{
	if (sp_plane->scaling_adjustment_property) {
		drm_property_destroy(sp_plane->base.dev, sp_plane->scaling_adjustment_property);
		sp_plane->scaling_adjustment_property = NULL;
	}
	if (sp_plane->win_alpha_property) {
		drm_property_destroy(sp_plane->base.dev, sp_plane->win_alpha_property);
		sp_plane->win_alpha_property = NULL;
	}

	if (sp_plane->region_color_keying_property) {
		drm_property_destroy(sp_plane->base.dev, sp_plane->region_color_keying_property);
		sp_plane->region_color_keying_property = NULL;
	}

	if (sp_plane->color_keying_property) {
		drm_property_destroy(sp_plane->base.dev, sp_plane->color_keying_property);
		sp_plane->color_keying_property = NULL;
	}

	if (sp_plane->region_color_keying_blob) {
		drm_property_blob_put(sp_plane->region_color_keying_blob);
		sp_plane->region_color_keying_blob = NULL;
	}
}

static const char * const sp_drm_plane_name[] = {
	"OVERLAY_PLANE", "PRIMARY_PLANE", "CURSOR_PLANE"};

static const char * const sp_plane_name[] = {
	"OSD3", "VPP0", "OSD2", "OSD1", "OSD0"};

struct drm_plane *sp7350_plane_init(struct drm_device *drm,
	enum drm_plane_type type, enum sp7350_plane_type sptype, int init_zpos)
{
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct drm_plane *plane = NULL;
	struct sp7350_plane *sp_plane = NULL;
	//struct drm_crtc *crtc = NULL;
	int ret = 0;

	sp_plane = devm_kzalloc(drm->dev, sizeof(*sp_plane), GFP_KERNEL);
	if (!sp_plane)
		return ERR_PTR(-ENOMEM);

	plane = &sp_plane->base;

	if (sptype == SP7350_PLANE_TYPE_VPP0) {
		sp_plane->pixel_formats = (u32 *)&sp7350_kms_vpp_formats;
		sp_plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_vpp_formats);
		sp_plane->funcs = &sp7350_kms_vpp_helper_funcs;
		sp_plane->is_media_plane = true;
		sp_plane->capabilities = SP7350_DRM_PLANE_CAP_SCALE |
			SP7350_DRM_PLANE_CAP_PIX_BLEND | SP7350_DRM_PLANE_CAP_ALPHA_BLEND |
			SP7350_DRM_PLANE_CAP_WIN_BLEND | SP7350_DRM_PLANE_CAP_BRIGHTNESS |
			SP7350_DRM_PLANE_CAP_CONTRAST;

		/* HW Restricted. */
		sp_plane->src_w_max = XRES_VIMGREAD_MAX;
		sp_plane->src_h_max = YRES_VIMGREAD_MAX;
		sp_plane->out_w_max = XRES_VSCL_MAX;
		sp_plane->out_h_max = YRES_VSCL_MAX;
		sp_plane->scl_w_max = XRES_VSCL_MAX;
		sp_plane->scl_h_max = YRES_VSCL_MAX;

		sp_plane->dmix_fg_sel = SP7350_DMIX_VPP0_SEL;
		sp_plane->osd_layer_sel = -1;
	} else {
		sp_plane->pixel_formats = (u32 *)&sp7350_kms_osd_formats;
		sp_plane->num_pixel_formats = ARRAY_SIZE(sp7350_kms_osd_formats);
		sp_plane->funcs = &sp7350_kms_osd_helper_funcs;
		sp_plane->is_media_plane = false;

		if (type != DRM_PLANE_TYPE_CURSOR)
			sp_plane->capabilities = SP7350_DRM_PLANE_CAP_PIX_BLEND |
				SP7350_DRM_PLANE_CAP_ALPHA_BLEND | SP7350_DRM_PLANE_CAP_WIN_BLEND |
				SP7350_DRM_PLANE_CAP_COLOR_KEYING | SP7350_DRM_PLANE_CAP_BRIGHTNESS |
				SP7350_DRM_PLANE_CAP_CONTRAST;
		/* HW Restricted. */
		sp_plane->src_w_max = XRES_OSD_MAX;
		sp_plane->src_h_max = YRES_OSD_MAX;
		sp_plane->out_w_max = XRES_OSD_MAX*2;  /* support frame base offset */
		sp_plane->out_h_max = YRES_OSD_MAX*2;  /* support frame base offset */
		sp_plane->scl_w_max = 0;
		sp_plane->scl_h_max = 0;

		switch (sptype) {
		case SP7350_PLANE_TYPE_OSD0:
			sp_plane->osd_layer_sel = SP7350_LAYER_OSD0;
			sp_plane->dmix_fg_sel   = SP7350_DMIX_OSD0_SEL;
			break;
		case SP7350_PLANE_TYPE_OSD1:
			sp_plane->osd_layer_sel = SP7350_LAYER_OSD1;
			sp_plane->dmix_fg_sel   = SP7350_DMIX_OSD1_SEL;
			break;
		case SP7350_PLANE_TYPE_OSD2:
			sp_plane->osd_layer_sel = SP7350_LAYER_OSD2;
			sp_plane->dmix_fg_sel   = SP7350_DMIX_OSD2_SEL;
			break;
		case SP7350_PLANE_TYPE_OSD3:
			sp_plane->osd_layer_sel = SP7350_LAYER_OSD3;
			sp_plane->dmix_fg_sel   = SP7350_DMIX_OSD3_SEL;
			break;
		default:
			DRM_DEBUG_ATOMIC("plane-%d Invalid sptype select index!!!.\n", plane->index);
			return ERR_PTR(-EINVAL);
		}
	}
	/* support zpos updated by user. */
	if (type != DRM_PLANE_TYPE_CURSOR)
		sp_plane->capabilities |= SP7350_DRM_PLANE_CAP_ZPOS;

	/*
	 * it could be
	 * DRM_PLANE_TYPE_PRIMARY or
	 * DRM_PLANE_TYPE_OVERLAY or
	 * DRM_PLANE_TYPE_CURSOR
	 */
	sp_plane->type = type;
	sp_plane->zpos = init_zpos;

	ret = drm_universal_plane_init(drm, plane, GENMASK(drm->mode_config.num_crtc, 0),
					&sp7350_plane_funcs,
				sp_plane->pixel_formats, sp_plane->num_pixel_formats,
				sp7350_kms_modifiers, type, NULL);
	if (ret)
		return ERR_PTR(ret);

	drm_plane_helper_add(plane, sp_plane->funcs);

	//sp_plane->index = plane->base->index;
	sp_plane->base.index = plane->index;

	sp7350_plane_create_propertys(sp_plane);

	switch (init_zpos) {
	case 4:
		sp_plane->dmix_layer = SP7350_DMIX_L6;
		sp_plane->dtg_adjust = SP7350_TGEN_DTG_ADJ_DMIX_L6;
		break;
	case 3:
		sp_plane->dmix_layer = SP7350_DMIX_L5;
		sp_plane->dtg_adjust = SP7350_TGEN_DTG_ADJ_DMIX_L5;
		break;
	case 2:
		sp_plane->dmix_layer = SP7350_DMIX_L4;
		sp_plane->dtg_adjust = SP7350_TGEN_DTG_ADJ_DMIX_L4;
		break;
	case 1:
		sp_plane->dmix_layer = SP7350_DMIX_L3;
		sp_plane->dtg_adjust = SP7350_TGEN_DTG_ADJ_DMIX_L3;
		break;
	case 0:
		sp_plane->dmix_layer = SP7350_DMIX_L1;
		sp_plane->dtg_adjust = SP7350_TGEN_DTG_ADJ_DMIX_L1;
		break;
	default:
		DRM_DEBUG_ATOMIC("plane-%d Invalid dmix_layer select index!!!.\n", plane->index);
		return ERR_PTR(-EINVAL);
	}
	/* default SP7350_DMIX_TRANSPARENT */
	sp_plane->layer_mode = SP7350_DMIX_TRANSPARENT;

	if (!sp_plane->is_media_plane)
		sp7350_osd_plane_init(plane);

	return plane;
}

int sp7350_plane_release(struct drm_device *drm, struct drm_plane *plane)
{
	//struct sp7350_dev *sp_dev = to_sp7350_dev(drm);
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	if (!plane || !sp_plane)
		return -1;

	/* DO ANY??? */
	sp7350_plane_destroy_propertys(sp_plane);

	return 0;
}

int sp7350_plane_dev_suspend(struct device *dev, struct drm_plane *plane)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	if (!plane || !sp_plane)
		return -1;

	DRM_DEV_DEBUG_DRIVER(dev, "plane-%d suspend.\n", plane->index);

	/* reset mixer setting */
	sp_plane->updated_alpha = DRM_BLEND_ALPHA_OPAQUE;
	sp_plane->updated_win_alpha = 255;
	sp_plane->updated_region_color_keying.regionid = 0;
	sp_plane->updated_region_color_keying.keying = 0;
	sp_plane->updated_color_keying = 0;

	return 0;
}

int sp7350_plane_dev_resume(struct device *dev, struct drm_plane *plane)
{
	struct sp7350_plane *sp_plane = to_sp7350_plane(plane);

	if (!plane || !sp_plane)
		return -1;

	DRM_DEV_DEBUG_DRIVER(dev, "plane-%d resume.\n", plane->index);

	if (!sp_plane->is_media_plane) {
		sp7350_osd_plane_init(plane);
		sp7350_kms_plane_osd_atomic_update(plane, NULL);
	} else {
		sp7350_kms_plane_vpp_atomic_update(plane, NULL);
	}

	return 0;
}

