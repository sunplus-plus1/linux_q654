/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM CRTCs
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 *         hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#ifndef __SUNPLUS_SP7350_DRM_CRTC_H__
#define __SUNPLUS_SP7350_DRM_CRTC_H__

#include <drm/drm_atomic.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_device.h>
#include <drm/drm_encoder.h>

#include "sp7350_drm_plane.h"

/*TGEN_DTG_CONFIG*/
#define SP7350_TGEN_FORMAT		GENMASK(10, 8)
#define SP7350_TGEN_FORMAT_SET(fmt)	FIELD_PREP(GENMASK(10, 8), fmt)
#define SP7350_TGEN_FORMAT_480P			0x0
#define SP7350_TGEN_FORMAT_576P			0x1
#define SP7350_TGEN_FORMAT_720P			0x2
#define SP7350_TGEN_FORMAT_1080P		0x3
#define SP7350_TGEN_FORMAT_64X64_360X100	0x6
#define SP7350_TGEN_FORMAT_64X64_144X100	0x7
#define SP7350_TGEN_FPS			GENMASK(5, 4)
#define SP7350_TGEN_FPS_SET(fps)	FIELD_PREP(GENMASK(5, 4), fps)
#define SP7350_TGEN_FPS_59P94HZ		0x0
#define SP7350_TGEN_FPS_50HZ		0x1
#define SP7350_TGEN_FPS_24HZ		0x2
#define SP7350_TGEN_USER_MODE		BIT(0)
#define SP7350_TGEN_INTERNAL		0x0
#define SP7350_TGEN_USER_DEF		0x1

struct drm_pending_vblank_event;

enum sp7350_encoder_type {
	SP7350_DRM_ENCODER_TYPE_NONE,
	/* Notes: C3V soc display controller only support one MIPI DSI interface!!!
	 *     but connected to two socket, can not be used simultaneously.
	 *     so, only one DSI inetface and one encoder actually.
	 */
	SP7350_DRM_ENCODER_TYPE_DSI0,  /* DSI interface compatible with Raspberry Pi */
	//SP7350_DRM_ENCODER_TYPE_DSI1,  /* DSI to HDMI bridge */
	//SP7350_DRM_ENCODER_TYPE_HDMI0,  /* NO any HDMI Controller onchip or onboard */
	//SP7350_DRM_ENCODER_TYPE_HDMI1, /* NO  any HDMI Controller onchip or onboard*/
	SP7350_DRM_ENCODER_TYPE_MAX
};

struct sp7350_encoder {
	struct drm_encoder base;
	enum sp7350_encoder_type type;
	u32 clock_select;

	void (*pre_crtc_configure)(struct drm_encoder *encoder);
	void (*pre_crtc_enable)(struct drm_encoder *encoder);
	void (*post_crtc_enable)(struct drm_encoder *encoder);

	void (*post_crtc_disable)(struct drm_encoder *encoder);
	void (*post_crtc_powerdown)(struct drm_encoder *encoder);
};

struct sp7350_crtc_state {
	struct drm_crtc_state base;
	/* Dlist area for this CRTC configuration. */
	struct drm_mm_node mm;
	bool feed_txp;
	bool txp_armed;
	unsigned int assigned_channel;

	struct {
		unsigned int left;
		unsigned int right;
		unsigned int top;
		unsigned int bottom;
	} margins;

	/* Transitional state below, only valid during atomic commits */
	bool update_muxing;
};

//static inline struct sp7350_crtc_state *
//to_sp7350_crtc_state(struct drm_crtc_state *crtc_state)
//{
//	return container_of(crtc_state, struct sp7350_crtc_state, base);
//}

#define to_sp7350_crtc_state(crtc_state) \
	container_of(crtc_state, struct sp7350_crtc_state, base)

#define to_sp7350_crtc(crtc) \
	container_of(crtc, struct sp7350_crtc, base)

#define to_sp7350_encoder(encoder) \
	container_of(encoder, struct sp7350_encoder, base)

#endif /* __SUNPLUS_SP7350_DRM_CRTC_H__ */
