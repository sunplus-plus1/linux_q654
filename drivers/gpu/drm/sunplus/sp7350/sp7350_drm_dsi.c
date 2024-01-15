// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM CRTCs and Encoder/Connecter
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include "sp7350_drm_drv.h"
#include "sp7350_drm_plane.h"

static void sp7350_drm_connector_destroy(struct drm_connector *connector)
{
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs sp7350_drm_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = sp7350_drm_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static int sp7350_drm_conn_get_modes(struct drm_connector *connector)
{
	int count;

	count = drm_add_modes_noedid(connector, XRES_MAX, YRES_MAX);
	drm_set_preferred_mode(connector, XRES_DEF, YRES_DEF);

	return count;
}

static const struct drm_connector_helper_funcs sp7350_drm_conn_helper_funcs = {
	.get_modes    = sp7350_drm_conn_get_modes,
};

int sp7350_drm_output_init(struct sp7350_drm_device *sdev, int index)
{
	struct sp7350_drm_output *output = &sdev->output;
	struct drm_device *dev = sdev->ddev;
	struct drm_connector *connector = &output->connector;
	struct drm_encoder *encoder = &output->encoder;
	struct drm_crtc *crtc = &output->crtc;
	struct drm_plane *primary;
	//unsigned int i;
	int ret;

	primary = sp7350_drm_plane_init(sdev, DRM_PLANE_TYPE_PRIMARY, index);
	if (IS_ERR(primary))
		return PTR_ERR(primary);

#if 0
	for (i = 0; i < 4; ++i) {
		primary = sp7350_drm_plane_init(sdev, DRM_PLANE_TYPE_OVERLAY, index);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to create plane %u\n", i);
			goto err_modeset_cleanup;
		}
	}
#endif

	ret = sp7350_drm_crtc_init(dev, crtc, primary, NULL);
	if (ret)
		goto err_crtc;

	ret = drm_connector_init(dev, connector, &sp7350_drm_connector_funcs,
				 DRM_MODE_CONNECTOR_DSI);
	if (ret) {
		DRM_ERROR("Failed to init connector\n");
		goto err_connector;
	}

	drm_connector_helper_add(connector, &sp7350_drm_conn_helper_funcs);

	ret = drm_simple_encoder_init(dev, encoder, DRM_MODE_ENCODER_DSI);
	if (ret) {
		DRM_ERROR("Failed to init encoder\n");
		goto err_encoder;
	}
	//encoder->possible_crtcs = 1;
	encoder->possible_crtcs = 1 << crtc->index;
	/* FIXME !!!!!!!!!
	 * Currently bound CRTC, only really meaningful for non-atomic
	 * drivers.  Atomic drivers should instead check
	 * &drm_connector_state.crtc. */
	//encoder->crtc = crtc;

	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret) {
		DRM_ERROR("Failed to attach connector to encoder\n");
		goto err_attach;
	}



	//ret = sp7350_drm_enable_writeback_connector(sdev);
	//if (ret)
	//	DRM_ERROR("Failed to init writeback connector\n");

	drm_mode_config_reset(dev);

	return 0;

err_attach:
	drm_encoder_cleanup(encoder);

err_encoder:
	drm_connector_cleanup(connector);

err_connector:
	drm_crtc_cleanup(crtc);

err_crtc:

	drm_plane_cleanup(primary);

	return ret;
}

