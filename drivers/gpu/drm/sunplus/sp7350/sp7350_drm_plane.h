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

/* for layer_mode (default, can be redefine) */
#define SP7350_DMIX_BG	0x0 //BG
#define SP7350_DMIX_L1	0x1 //VPP0
#define SP7350_DMIX_L2	0x2 //VPP1 (unsupported)
#define SP7350_DMIX_L3	0x3 //OSD3
#define SP7350_DMIX_L4	0x4 //OSD2
#define SP7350_DMIX_L5	0x5 //OSD1
#define SP7350_DMIX_L6	0x6 //OSD0
#define SP7350_DMIX_MAX_LAYER	7

#define SP7350_VPP_IMGREAD_DATA_FMT_UYVY	0x1
#define SP7350_VPP_IMGREAD_DATA_FMT_YUY2	0x2
#define SP7350_VPP_IMGREAD_DATA_FMT_NV16	0x3
#define SP7350_VPP_IMGREAD_DATA_FMT_NV24	0x6
#define SP7350_VPP_IMGREAD_DATA_FMT_NV12	0x7

/* for sp7350_osd_header*/
#define SP7350_OSD_COLOR_MODE_8BPP			0x2
#define SP7350_OSD_COLOR_MODE_YUY2			0x4
#define SP7350_OSD_COLOR_MODE_RGB565		0x8
#define SP7350_OSD_COLOR_MODE_ARGB1555		0x9
#define SP7350_OSD_COLOR_MODE_RGBA4444		0xa
#define SP7350_OSD_COLOR_MODE_ARGB4444		0xb
#define SP7350_OSD_COLOR_MODE_RGBA8888		0xd
#define SP7350_OSD_COLOR_MODE_ARGB8888		0xe

#define SP7350_MAX_PLANE  5

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

struct sp7350_drm_plane_state {
	struct sp7350_plane_region_alpha_info region_alpha;
	struct sp7350_plane_region_color_keying_info region_color_keying;
	unsigned int color_keying;
	struct drm_property_blob *region_alpha_blob;
	struct drm_property_blob *region_color_keying_blob;
};

struct sp7350_drm_plane {
	struct drm_plane base;
	enum drm_plane_type type;
	const uint32_t *pixel_formats;
	unsigned int num_pixel_formats;
	const struct drm_plane_helper_funcs *funcs;
	unsigned int capabilities;
	unsigned int zpos;
	unsigned int index;
	bool is_media_plane;
	struct drm_property *region_alpha_property;
	struct drm_property *region_color_keying_property;
	struct drm_property *color_keying_property;

	struct sp7350_drm_plane_state state;
};

#define to_sp7350_drm_plane(target)\
	container_of(target, struct sp7350_drm_plane, base)

int sp7350_plane_create_primary_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane);
int sp7350_plane_create_media_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane);
int sp7350_plane_create_overlay_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane, int index);
int sp7350_plane_release_plane(struct drm_device *drm,
	struct sp7350_drm_plane *plane);

//int sp7350_plane_create_additional_planes(struct drm_device *drm);

#endif /* __SUNPLUS_SP7350_DRM_PLANE_H__ */
