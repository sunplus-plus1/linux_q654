/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM CRTCs
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#ifndef __SUNPLUS_SP7350_DRM_CRTC_H__
#define __SUNPLUS_SP7350_DRM_CRTC_H__

#include <drm/drm_atomic.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_device.h>
#include <drm/drm_encoder.h>

struct drm_pending_vblank_event;

enum sp7350_drm_encoder_type {
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

struct sp7350_drm_encoder {
	struct drm_encoder base;
	enum sp7350_drm_encoder_type type;
	u32 clock_select;

	void (*pre_crtc_configure)(struct drm_encoder *encoder);
	void (*pre_crtc_enable)(struct drm_encoder *encoder);
	void (*post_crtc_enable)(struct drm_encoder *encoder);

	void (*post_crtc_disable)(struct drm_encoder *encoder);
	void (*post_crtc_powerdown)(struct drm_encoder *encoder);
};

struct sp7350_drm_crtc {
	struct drm_crtc crtc;
	struct platform_device *pdev;
	void __iomem *regs;
	void __iomem *ao_moon3;

	//struct drm_crtc_state base;

	struct drm_pending_vblank_event *event;

	enum sp7350_drm_encoder_type encoder_types[2];

	/* TODO: setting with C3V dipslay tcon feature. */
	u8 lut_r[256];
	u8 lut_g[256];
	u8 lut_b[256];

	struct debugfs_regset32 regset;
	struct debugfs_regset32 ao_moon3_regset;
};

#define to_sp7350_drm_crtc(target)\
	container_of(target, struct sp7350_drm_crtc, crtc)

#define to_sp7350_drm_encoder(target)\
	container_of(target, struct sp7350_drm_encoder, base)

int sp7350_drm_crtc_init(struct drm_device *dev, struct drm_crtc *crtc,
		   struct drm_plane *primary, struct drm_plane *cursor);
void sp7350_drm_crtc_finish_page_flip(struct sp7350_drm_crtc *scrtc);
void sp7350_drm_crtc_suspend(struct sp7350_drm_crtc *scrtc);
void sp7350_drm_crtc_resume(struct sp7350_drm_crtc *scrtc);

extern void __iomem *sp7350_display_ioremap_regs(int index);

#endif /* __SUNPLUS_SP7350_DRM_CRTC_H__ */
