/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM Planes
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 *         hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#ifndef __SUNPLUS_SP7350_DRM_PLANE_H__
#define __SUNPLUS_SP7350_DRM_PLANE_H__
#include <drm/drm.h>

#include "sp7350_drm_crtc.h"

#define SP7350_MAX_PLANE  5

#define SP7350_LAYER_OSD0				0x0
#define SP7350_LAYER_OSD1				0x1
#define SP7350_LAYER_OSD2				0x2
#define SP7350_LAYER_OSD3				0x3

/* for fg_sel */
#define SP7350_DMIX_VPP0	0x0
#define SP7350_DMIX_VPP1	0x1 //unsupported
#define SP7350_DMIX_VPP2	0x2 //unsupported
#define SP7350_DMIX_OSD0	0x3
#define SP7350_DMIX_OSD1	0x4
#define SP7350_DMIX_OSD2	0x5
#define SP7350_DMIX_OSD3	0x6
#define SP7350_DMIX_PTG		0x7

/* for layer_mode */
#define SP7350_DMIX_BG	0x0 //BG
#define SP7350_DMIX_L1	0x1 //VPP0
#define SP7350_DMIX_L2	0x2 //VPP1 (unsupported)
#define SP7350_DMIX_L3	0x3 //OSD3
#define SP7350_DMIX_L4	0x4 //OSD2
#define SP7350_DMIX_L5	0x5 //OSD1
#define SP7350_DMIX_L6	0x6 //OSD0
#define SP7350_DMIX_MAX_LAYER	7

/* for pattern_sel */
#define SP7350_DMIX_BIST_BGC		0x0
#define SP7350_DMIX_BIST_COLORBAR_ROT0	0x1
#define SP7350_DMIX_BIST_COLORBAR_ROT90	0x2
#define SP7350_DMIX_BIST_BORDER_NONE	0x3
#define SP7350_DMIX_BIST_BORDER_ONE	0x4
#define SP7350_DMIX_BIST_BORDER		0x5
#define SP7350_DMIX_BIST_SNOW		0x6
#define SP7350_DMIX_BIST_SNOW_MAX	0x7
#define SP7350_DMIX_BIST_SNOW_HALF	0x8
#define SP7350_DMIX_BIST_REGION		0x9

/*
 * OSD Header config[0]
 */
#define SP7350_OSD_HDR_CULT				BIT(31) /* En Color Look Up Table */
#define SP7350_OSD_HDR_BS				BIT(12) /* BYTE SWAP */
/*
 *   BL =1 define HDR_ALPHA as fix value
 *   BL2=1 define HDR_ALPHA as factor value
 */
#define SP7350_OSD_HDR_BL2				BIT(10)
#define SP7350_OSD_HDR_BL				BIT(8)
#define SP7350_OSD_HDR_ALPHA			GENMASK(7, 0)
#define SP7350_OSD_HDR_KEY				BIT(11)

/*
 * OSD Header config[5]
 */
#define SP7350_OSD_HDR_CSM				GENMASK(19, 16) /* Color Space Mode */
#define SP7350_OSD_HDR_CSM_SET(sel)		FIELD_PREP(GENMASK(19, 16), sel)
#define SP7350_OSD_CSM_RGB_BT601		0x1 /* RGB to BT601 */
#define SP7350_OSD_CSM_BYPASS			0x4 /* Bypass */

/*
 * OSD region dirty flag for SW latch
 */
#define REGION_ADDR_DIRTY				BIT(0) //(1 << 0)
#define REGION_GSCL_DIRTY				BIT(1) //(1 << 1)

/* for sp7350_osd_header*/
#define SP7350_OSD_COLOR_MODE_8BPP			0x2
#define SP7350_OSD_COLOR_MODE_YUY2			0x4
#define SP7350_OSD_COLOR_MODE_RGB565		0x8
#define SP7350_OSD_COLOR_MODE_ARGB1555		0x9
#define SP7350_OSD_COLOR_MODE_RGBA4444		0xa
#define SP7350_OSD_COLOR_MODE_ARGB4444		0xb
#define SP7350_OSD_COLOR_MODE_RGBA8888		0xd
#define SP7350_OSD_COLOR_MODE_ARGB8888		0xe

#define ALIGNED(x, n)		((x) & (~(n - 1)))
#define EXTENDED_ALIGNED(x, n)	(((x) + ((n) - 1)) & (~(n - 1)))

#define SWAP32(x)	((((unsigned int)(x)) & 0x000000ff) << 24 \
			| (((unsigned int)(x)) & 0x0000ff00) << 8 \
			| (((unsigned int)(x)) & 0x00ff0000) >> 8 \
			| (((unsigned int)(x)) & 0xff000000) >> 24)
#define SWAP16(x)	(((x) & 0x00ff) << 8 | ((x) >> 8))

/*
 * DRM PLANE Setting
 *
 */
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

/*
 *  sp7350_osd_header
 */
struct sp7350_osd_header {
	u32 osd_header[8];
	u32 osd_rsv[24];
};

struct sp7350_osd_region_info {
	u32 buf_width;
	u32 buf_height;
	u32 start_x;
	u32 start_y;
	u32 act_width;
	u32 act_height;
	u32 act_x;
	u32 act_y;
};

struct sp7350_osd_alpha_info {
	u32 region_alpha_en;
	u32 region_alpha;
	u32 color_key_en;
	u32 color_key;
};

/*
 *  sp7350 osd region include
 *  sp7350_osd_header + sp7350_osd_palette + bitmap
 */
struct sp7350_osd_region {
	struct sp7350_osd_region_info region_info;
	struct sp7350_osd_alpha_info alpha_info;
	u32	color_mode;	/* osd color mode */
	u32	buf_num;	/* fix 2 */
	u32	buf_align;	/* fix 4096 */

	u32	buf_addr_phy;	/* buffer address physical */
	u8	*buf_addr_vir;	/* buffer address virtual */
	//palette addr in osd header
	u8	*hdr_pal;	/* palette address virtual */
	u32	buf_size;	/* buffer size */
	u32	buf_cur_id;	/* buffer current id */

	// SW latch
	u32	dirty_flag;	/* dirty flag */
	//other side palette addr, Gearing with swap buffer.
	u8	*pal;		/* palette address virtual, for swap buffer */

	struct sp7350_osd_header *hdr;
	//structure size should be 32 alignment.
	u32	reserved[4];
};

struct sp7350_plane_state {
	struct drm_plane_state base;
	struct sp7350_plane_region_alpha_info region_alpha;
	struct sp7350_plane_region_color_keying_info region_color_keying;
	unsigned int color_keying;
	struct drm_property_blob *region_alpha_blob;
	struct drm_property_blob *region_color_keying_blob;
};

struct sp7350_plane {
	struct drm_plane base;
	enum drm_plane_type type;
	u32 *pixel_formats;
	unsigned int num_pixel_formats;
	const struct drm_plane_helper_funcs *funcs;
	unsigned int capabilities;
	unsigned int zpos;
	bool is_media_plane;
	struct drm_property *region_alpha_property;
	struct drm_property *region_color_keying_property;
	struct drm_property *color_keying_property;

	struct sp7350_plane_state state;
};

#define to_sp7350_plane(plane) \
	container_of(plane, struct sp7350_plane, base)

struct drm_plane *sp7350_plane_init(struct drm_device *drm,
	enum drm_plane_type type, int sptype);
int sp7350_plane_release(struct drm_device *drm, struct drm_plane *plane);

#endif /* __SUNPLUS_SP7350_DRM_PLANE_H__ */
