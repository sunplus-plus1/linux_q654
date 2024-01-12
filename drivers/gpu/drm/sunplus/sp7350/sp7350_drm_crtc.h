/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM CRTCs
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#ifndef __SUNPLUS_SP7350_DRM_CRTC_H__
#define __SUNPLUS_SP7350_DRM_CRTC_H__

#include <drm/drm_crtc.h>
#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>

struct drm_pending_vblank_event;

struct sp7350_drm_crtc {
	struct drm_crtc crtc;

	struct drm_pending_vblank_event *event;
	int dpms;

	//const struct shmob_drm_format_info *format;
	unsigned long dma[2];
	unsigned int line_size;
	bool started;
};

int sp7350_drm_crtc_init(struct drm_device *dev, struct drm_crtc *crtc,
		   struct drm_plane *primary, struct drm_plane *cursor);
void sp7350_drm_crtc_finish_page_flip(struct sp7350_drm_crtc *scrtc);
void sp7350_drm_crtc_suspend(struct sp7350_drm_crtc *scrtc);
void sp7350_drm_crtc_resume(struct sp7350_drm_crtc *scrtc);

#endif /* __SUNPLUS_SP7350_DRM_CRTC_H__ */
