/* SPDX-License-Identifier: GPL-2.0+ */
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
#include "sp7350_drm_plane.h"

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

int sp7350_drm_crtc_init(struct drm_device *drm, struct drm_crtc *crtc,
		   struct drm_plane *primary, struct drm_plane *cursor)
{
	struct device_node *port;
	struct sp7350_drm_crtc *sp7350_crtc = to_sp7350_drm_crtc(crtc);
	struct drm_plane *primary_plane = primary;
	int ret;

	/* set crtc port so that
	 * drm_of_find_possible_crtcs call works
	 */
	port = of_get_child_by_name(sp7350_crtc->pdev->dev.of_node, "port");
	if (!port) {
		DRM_DEV_ERROR(drm->dev,"no port node found in %pOF\n", sp7350_crtc->pdev->dev.of_node);
		return -EINVAL;
	}
	of_node_put(port);
	crtc->port = port;

	if (!primary_plane) {
		/* For now, we create just the primary and the legacy cursor
		 * planes.  We should be able to stack more planes on easily,
		 * but to do that we would need to compute the bandwidth
		 * requirement of the plane configuration, and reject ones
		 * that will take too much.
		 */
		primary_plane = sp7350_drm_plane_init(drm, DRM_PLANE_TYPE_PRIMARY, 0);
		if (IS_ERR(primary_plane)) {
			DRM_DEV_ERROR(drm->dev, "failed to construct primary plane\n");
			return PTR_ERR(primary_plane);
		}
	}

	ret = drm_crtc_init_with_planes(drm, crtc, primary_plane, cursor,
					&sp7350_drm_crtc_funcs, NULL);
	if (ret) {
		DRM_DEV_ERROR(drm->dev, "Failed to init CRTC\n");
		return ret;
	}
	drm_crtc_helper_add(crtc, &sp7350_drm_crtc_helper_funcs);

	/* TODO: set C3V SOC DISPLAY HW TCON feature... */
	#if 0
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

	sp7350_drm_crtc->ao_moon3 = sp7350_display_ioremap_regs(1);
	if (IS_ERR(sp7350_drm_crtc->ao_moon3))
		return dev_err_probe(&pdev->dev, PTR_ERR(sp7350_drm_crtc->ao_moon3), "reg ao_moon3 not found\n");

	sp7350_drm_crtc->regset.base = sp7350_drm_crtc->regs;
	sp7350_drm_crtc->ao_moon3_regset.base = sp7350_drm_crtc->ao_moon3;
	/* TODO: setting debugfs_regset32 for C3V DISPLAY REGISTER */
	//sp7350_drm_crtc->regset.regs = crtc_regs;
	//sp7350_drm_crtc->regset.nregs = ARRAY_SIZE(crtc_regs);

	ret = sp7350_drm_crtc_init(drm, crtc, NULL, NULL);
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
	struct platform_device *pdev = to_platform_device(dev);
	struct sp7350_drm_crtc *sp7350_crtc = dev_get_drvdata(dev);

	drm_crtc_cleanup(&sp7350_crtc->crtc);

	/* TODO Set S3V SOC DISPLAY REG, disable crtc things... */

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
	component_del(&pdev->dev, &sp7350_crtc_ops);
	return 0;
}

static const struct of_device_id sp7350_crtc_dt_match[] = {
	{ .compatible = "sunplus,sp7350-crtc0" },
	{}
};
struct platform_driver sp7350_crtc_driver = {
	.probe = sp7350_crtc_dev_probe,
	.remove = sp7350_crtc_dev_remove,
	.driver = {
		.name = "sp7350_crtc",
		.of_match_table = sp7350_crtc_dt_match,
	},
};

