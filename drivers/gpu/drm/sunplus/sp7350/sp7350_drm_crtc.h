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

#define to_sp7350_encoder(encoder) \
	container_of(encoder, struct sp7350_encoder, base)

#endif /* __SUNPLUS_SP7350_DRM_CRTC_H__ */
