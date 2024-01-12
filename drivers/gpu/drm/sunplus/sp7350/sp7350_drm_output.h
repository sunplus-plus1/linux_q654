// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM CRTCs and Encoder/Connecter
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#ifndef __SP7350_DRM_OUTPUT_H__
#define __SP7350_DRM_OUTPUT_H__

#include <drm/drm_crtc.h>
#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>

struct sp7350_drm_device;

struct sp7350_drm_output {
	struct drm_crtc crtc;
	struct drm_encoder encoder;
	struct drm_connector connector;
	//struct drm_writeback_connector wb_connector;
	struct hrtimer vblank_hrtimer;
	ktime_t period_ns;
	struct drm_pending_vblank_event *event;
	/* ordered wq for composer_work */
	//struct workqueue_struct *composer_workq;
	/* protects concurrent access to composer */
	spinlock_t lock;

	/* protected by @lock */
	//bool composer_enabled;
	//struct vkms_crtc_state *composer_state;

	//spinlock_t composer_lock;
};

#define drm_crtc_to_sp7350_drm_output(target) \
		container_of(target, struct sp7350_drm_output, crtc)

int sp7350_drm_output_init(struct sp7350_drm_device *sdev, int index);

#endif /* __SP7350_DRM_OUTPUT_H__ */
