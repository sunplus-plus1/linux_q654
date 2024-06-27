// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM CRTCs
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <linux/component.h>
#include <linux/of_device.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

#include "sp7350_drm_crtc.h"
#include "sp7350_drm_drv.h"

/* FIXME: For DISPLAY TGEN HW SETTING Temporary.
 *  The correct way is map TGEN registers in crtc controller.
 */
#include <media/sunplus/disp/sp7350/sp7350_disp_osd.h>
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_vpp.h"
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_dmix.h"
#include "../../../../media/platform/sunplus/display/sp7350/sp7350_disp_tgen.h"
extern int sp7350_resolution_set(unsigned int width, unsigned int height);

/* always keep 0 */
#define SP7350_DRM_TODO    0

static const struct drm_crtc_funcs sp7350_drm_crtc_funcs = {
	.destroy	= drm_crtc_cleanup,
	.set_config	= drm_atomic_helper_set_config,
	.page_flip	= drm_atomic_helper_page_flip,
	.reset		= drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state	= drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_crtc_destroy_state,
};

static int sp7350_drm_crtc_atomic_check(struct drm_crtc *crtc,
					struct drm_crtc_state *state)
{
	/* TODO reference to vkms_crtc_atomic_check */
	DRM_DEBUG_DRIVER("[TODO]\n");
	if (state->mode_changed) {
		struct drm_display_mode *adj_mode = &state->adjusted_mode;
		DRM_DEBUG_DRIVER("Update TGEN Timing...\n");
		sp7350_resolution_set(adj_mode->hdisplay, adj_mode->vdisplay);
		/* TODO: Check mode valid first. refer to sp7350_dsi_mode_check */
		//sp7350_tgen_init();
	}
	return 0;
}

static void sp7350_drm_crtc_atomic_enable(struct drm_crtc *crtc,
					  struct drm_crtc_state *old_state)
{
	/* FIXME, NO vblank ??? */
	//drm_crtc_vblank_on(crtc);
	DRM_DEBUG_DRIVER("[TODO]\n");
}

static void sp7350_drm_crtc_atomic_disable(struct drm_crtc *crtc,
					   struct drm_crtc_state *old_state)
{
	/* FIXME, NO vblank ??? */
	//drm_crtc_vblank_off(crtc);
	DRM_DEBUG_DRIVER("[TODO]\n");
}

static void sp7350_drm_crtc_atomic_begin(struct drm_crtc *crtc,
					 struct drm_crtc_state *old_crtc_state)
{
	/* TODO reference to vkms_crtc_atomic_begin */
	DRM_DEBUG_DRIVER("[TODO]\n");
}

static void sp7350_drm_crtc_atomic_flush(struct drm_crtc *crtc,
					 struct drm_crtc_state *old_crtc_state)
{
	/* TODO reference to vkms_crtc_atomic_flush */
	//struct sp7350_drm_crtc *sp7350_crtc = to_sp7350_drm_crtc(crtc);
	struct drm_pending_vblank_event *event = crtc->state->event;

	DRM_DEBUG_DRIVER("Start\n");

	if (crtc->state->mode_changed) {
		sp7350_tgen_init();
	}
	if (event) {
		crtc->state->event = NULL;

		spin_lock_irq(&crtc->dev->event_lock);
		if (drm_crtc_vblank_get(crtc) == 0)
			drm_crtc_arm_vblank_event(crtc, event);
		else
			drm_crtc_send_vblank_event(crtc, event);
		spin_unlock_irq(&crtc->dev->event_lock);
	}
}

static const struct drm_crtc_helper_funcs sp7350_drm_crtc_helper_funcs = {
	.atomic_check	= sp7350_drm_crtc_atomic_check,
	.atomic_begin	= sp7350_drm_crtc_atomic_begin,
	.atomic_flush	= sp7350_drm_crtc_atomic_flush,
	.atomic_enable	= sp7350_drm_crtc_atomic_enable,
	.atomic_disable	= sp7350_drm_crtc_atomic_disable,
};

static void sp7350_set_crtc_possible_masks(struct drm_device *drm,
					   struct drm_crtc *crtc)
{
	struct sp7350_drm_crtc *sp7350_crtc = to_sp7350_drm_crtc(crtc);
	const enum sp7350_drm_encoder_type *encoder_types = sp7350_crtc->encoder_types;
	struct drm_encoder *encoder;

	drm_for_each_encoder(encoder, drm) {
		struct sp7350_drm_encoder *sp7350_encoder;
		int i;

		if (encoder->encoder_type == DRM_MODE_ENCODER_VIRTUAL)
			continue;

		sp7350_encoder = to_sp7350_drm_encoder(encoder);
		for (i = 0; i < SP7350_DRM_ENCODER_TYPE_MAX; i++) {
			if (sp7350_encoder->type == encoder_types[i]) {
				sp7350_encoder->clock_select = i;
				encoder->possible_crtcs |= drm_crtc_mask(crtc);
				break;
			}
		}
	}
}

int sp7350_drm_crtc_init(struct drm_device *drm, struct drm_crtc *crtc)
{
	struct device_node *port;
	struct sp7350_drm_crtc *sp7350_crtc = to_sp7350_drm_crtc(crtc);
	int ret;

	/* set crtc port so that
	 * drm_of_find_possible_crtcs call works
	 */
	port = of_get_child_by_name(sp7350_crtc->pdev->dev.of_node, "port");
	if (!port) {
		DRM_DEV_ERROR(drm->dev, "no port node found in %p\n", sp7350_crtc->pdev->dev.of_node);
		return -EINVAL;
	}
	of_node_put(port);
	crtc->port = port;

	/* For now, we create just the primary and the legacy cursor
	 * planes.  We should be able to stack more planes on easily,
	 * but to do that we would need to compute the bandwidth
	 * requirement of the plane configuration, and reject ones
	 * that will take too much.
	 */
	ret = sp7350_plane_create_primary_plane(drm, &sp7350_crtc->primary_plane);
	if (ret) {
		DRM_DEV_ERROR(drm->dev, "failed to construct primary plane\n");
		return ret;
	}

	ret = drm_crtc_init_with_planes(drm, crtc, &sp7350_crtc->primary_plane.base, NULL,
					&sp7350_drm_crtc_funcs, NULL);
	if (ret) {
		DRM_DEV_ERROR(drm->dev, "Failed to init CRTC\n");
		return ret;
	}
	drm_crtc_helper_add(crtc, &sp7350_drm_crtc_helper_funcs);

#if !DRM_PRIMARY_PLANE_ONLY
	ret = sp7350_plane_create_media_plane(drm, &sp7350_crtc->media_plane);
	if (ret) {
		DRM_DEV_ERROR(drm->dev, "failed to construct media(overlay) plane\n");
		return ret;
	}

	ret = sp7350_plane_create_overlay_plane(drm, &sp7350_crtc->overlay_planes[0], 0);
	if (ret) {
		DRM_DEV_ERROR(drm->dev, "failed to construct overlay plane\n");
		return ret;
	}
	ret = sp7350_plane_create_overlay_plane(drm, &sp7350_crtc->overlay_planes[1], 1);
	if (ret) {
		DRM_DEV_ERROR(drm->dev, "failed to construct overlay plane\n");
		return ret;
	}
#endif

	/* TODO: set C3V SOC DISPLAY HW TCON feature... */
	#if SP7350_DRM_TODO
	drm_mode_crtc_set_gamma_size(crtc, ARRAY_SIZE(sp7350_crtc->lut_r));

	drm_crtc_enable_color_mgmt(crtc, 0, false, crtc->gamma_size);

	/* We support CTM(Color Transformation Matrix), but only for one CRTC at a time. It's therefore
	 * implemented as private driver state in sp7350_drm_kms, not here.
	 */
	drm_crtc_enable_color_mgmt(crtc, 0, true, crtc->gamma_size);
	#endif

	return ret;
}

static int sp7350_crtc_bind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm = dev_get_drvdata(master);
	struct sp7350_drm_crtc *sp7350_drm_crtc;
	struct drm_crtc *crtc;
	int ret;

	DRM_DEV_DEBUG_DRIVER(dev, "start.\n");
	sp7350_drm_crtc = devm_kzalloc(dev, sizeof(*sp7350_drm_crtc), GFP_KERNEL);
	if (!sp7350_drm_crtc)
		return -ENOMEM;
	crtc = &sp7350_drm_crtc->crtc;

	sp7350_drm_crtc->pdev = pdev;

	/* setting for C3V DISPLAY REGISTER */
	/*
	 * get reg base resource
	 */
	sp7350_drm_crtc->regs = sp7350_display_ioremap_regs(0);
	if (IS_ERR(sp7350_drm_crtc->regs))
		return dev_err_probe(&pdev->dev, PTR_ERR(sp7350_drm_crtc->regs), "reg base not found\n");

	sp7350_drm_crtc->regset.base = sp7350_drm_crtc->regs;
	/* TODO: setting debugfs_regset32 for C3V DISPLAY REGISTER */
	//sp7350_drm_crtc->regset.regs = crtc_regs;
	//sp7350_drm_crtc->regset.nregs = ARRAY_SIZE(crtc_regs);

	/* FIXME: Do once at fisrt. */
	//dmix must first init
	sp7350_dmix_init();
	sp7350_osd_init();
	sp7350_osd_header_init();

	ret = sp7350_drm_crtc_init(drm, crtc);
	if (ret)
		return ret;
	sp7350_set_crtc_possible_masks(drm, crtc);

	/* TODO: SETTING REGISTER and devm_request_irq for C3V SOC Display. */

	platform_set_drvdata(pdev, sp7350_drm_crtc);

	//sp7350_debugfs_add_regset32(drm, pv_data->debugfs_name,
	//			 &sp7350_drm_crtc->regset);

	return 0;
}

static void sp7350_crtc_unbind(struct device *dev, struct device *master,
			       void *data)
{
	struct drm_device *drm = dev_get_drvdata(master);
	struct platform_device *pdev = to_platform_device(dev);
	struct sp7350_drm_crtc *sp7350_crtc = dev_get_drvdata(dev);

	drm_crtc_cleanup(&sp7350_crtc->crtc);

	/* TODO Set S3V SOC DISPLAY REG, disable crtc things... */

	sp7350_plane_release_plane(drm, &sp7350_crtc->primary_plane);
	sp7350_plane_release_plane(drm, &sp7350_crtc->media_plane);
	sp7350_plane_release_plane(drm, &sp7350_crtc->overlay_planes[0]);
	sp7350_plane_release_plane(drm, &sp7350_crtc->overlay_planes[1]);
	sp7350_plane_release_plane(drm, &sp7350_crtc->cursor_plane);

	platform_set_drvdata(pdev, NULL);
}

static const struct component_ops sp7350_crtc_ops = {
	.bind   = sp7350_crtc_bind,
	.unbind = sp7350_crtc_unbind,
};

static int sp7350_crtc_dev_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &sp7350_crtc_ops);
}

static int sp7350_crtc_dev_remove(struct platform_device *pdev)
{
	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "crtc driver remove.\n");

	component_del(&pdev->dev, &sp7350_crtc_ops);
	return 0;
}

static int sp7350_crtc_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "[TODO]crtc driver suspend.\n");

	return 0;
}

static int sp7350_crtc_dev_resume(struct platform_device *pdev)
{
	DRM_DEV_DEBUG_DRIVER(&pdev->dev, "[TODO]crtc driver resume.\n");

	return 0;
}

static const struct of_device_id sp7350_crtc_dt_match[] = {
	{ .compatible = "sunplus,sp7350-crtc0" },
	{}
};

struct platform_driver sp7350_crtc_driver = {
	.probe   = sp7350_crtc_dev_probe,
	.remove  = sp7350_crtc_dev_remove,
	.suspend = sp7350_crtc_dev_suspend,
	.resume  = sp7350_crtc_dev_resume,
	.driver  = {
		.name = "sp7350_crtc",
		.of_match_table = sp7350_crtc_dt_match,
	},
};

