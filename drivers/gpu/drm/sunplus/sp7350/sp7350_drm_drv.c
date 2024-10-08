// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM driver
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 *         hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

#include "sp7350_drm_drv.h"
#if defined(DRM_GEM_DMA_AVAILABLE)
#include <drm/drm_fbdev_generic.h>
#include <drm/drm_gem_dma_helper.h>
#else
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#endif

#include "sp7350_drm_crtc.h"
#include "sp7350_drm_plane.h"
#include "sp7350_drm_regs.h"

#include "sp7350_drm_kms.h"
#include "sp7350_drm_fbdev.h"

#include <linux/dma-mapping.h>

/* -----------------------------------------------------------------------------
 * Hardware initialization
 */

/* -----------------------------------------------------------------------------
 * DRM operations
 */

#if defined(DRM_GEM_DMA_AVAILABLE)
DEFINE_DRM_GEM_DMA_FOPS(sp7350_drm_fops);
#else
DEFINE_DRM_GEM_CMA_FOPS(sp7350_drm_fops);
#endif

static struct drm_driver sp7350_drm_driver = {
	.driver_features	= DRIVER_MODESET | DRIVER_ATOMIC | DRIVER_GEM,
	//.irq_handler		= sp7350_drm_irq,
	.fops			= &sp7350_drm_fops,
#if defined(DRM_GEM_DMA_AVAILABLE)
	DRM_GEM_DMA_DRIVER_OPS,
#else
	DRM_GEM_CMA_DRIVER_OPS,
#endif
//#if 1//defined(CONFIG_DEBUG_FS)
	.debugfs_init = sp7350_debugfs_init,
//#endif

	.name			= "sp7350-drm",
	.desc			= "Sunplus SP7350 DRM",
	.date			= "20240828",
	.major			= 2,
	.minor			= 0,
};

/* -----------------------------------------------------------------------------
 * Power management
 */

#ifdef CONFIG_PM_SLEEP
static int sp7350_drm_pm_suspend(struct device *dev)
{
	struct sp7350_dev *sp_dev = dev_get_drvdata(dev);

	DRM_DEV_DEBUG_DRIVER(dev, "[FIXME]drm dev driver suspend.\n");

	/*
	 * FIXME!!!
	 * Should call drm_mode_config_helper_suspend,
	 * but abort in drm_kms_helper_poll_disable.
	 */
	//drm_mode_config_helper_suspend(&sp_dev->base);
	drm_fb_helper_set_suspend_unlocked(sp_dev->base.fb_helper, 1);

	return 0;
}

static int sp7350_drm_pm_resume(struct device *dev)
{
	struct sp7350_dev *sp_dev = dev_get_drvdata(dev);

	DRM_DEV_DEBUG_DRIVER(dev, "[FIXME]drm dev driver resume.\n");

	/*
	 * FIXME!!!
	 * Should call drm_mode_config_helper_resume,
	 * but abort in drm_mode_config_helper_suspend.
	 */
	//drm_mode_config_helper_resume(&sp_dev->base);
	drm_fb_helper_set_suspend_unlocked(sp_dev->base.fb_helper, 0);

	return 0;
}
#endif

static const struct dev_pm_ops sp7350_drm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp7350_drm_pm_suspend, sp7350_drm_pm_resume)
};

static int sp7350_drm_bind(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sp7350_dev *sp_dev;
	struct drm_device *drm;
	//struct drm_crtc *crtc;
	int ret = 0;
	int i;

	DRM_DEV_DEBUG_DRIVER(dev, "start\n");
	/* using device-specific reserved memorym,
	 *  defined at dts with label drm_disp_reserve
	 */
	ret = of_reserved_mem_device_init(dev);
	if (!ret) {
		DRM_DEV_DEBUG_DRIVER(dev, "using device-specific reserved memory\n");
		ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
		if (ret) {
			DRM_DEV_ERROR(dev, "32-bit consistent DMA enable failed\n");
			return ret;
		}
	}

	sp_dev = devm_drm_dev_alloc(dev, &sp7350_drm_driver,
							  struct sp7350_dev, base);
	if (IS_ERR(sp_dev))
		return PTR_ERR(sp_dev);

	drm = &sp_dev->base;
	platform_set_drvdata(pdev, drm);
	INIT_LIST_HEAD(&sp_dev->debugfs_list);

	DRM_DEV_DEBUG_DRIVER(dev, "drmm_mode_config_init\n");
	ret = drmm_mode_config_init(drm);
	if (ret)
		return ret;

	DRM_DEV_DEBUG_DRIVER(dev, "component_bind_all\n");
	ret = component_bind_all(dev, drm);
	if (ret)
		return ret;

	for (i = 0; i < SP_DISP_MAX_OSD_LAYER; i++) {
		sp_dev->osd_hdr[i] = dma_alloc_coherent(&pdev->dev,
				0x100000, /* reserved 1MB for OSD header */
				&sp_dev->osd_hdr_phy[i],
				GFP_KERNEL | __GFP_ZERO);
		if (!sp_dev->osd_hdr[i]) {
			pr_err("[DRV]%s malloc osd header fail\n", __func__);
			goto err_free_mem;
		}
	}

	//DRM_DEV_DEBUG_DRIVER(dev, "drm_fb_helper_remove_conflicting_framebuffers\n");
	//drm_fb_helper_remove_conflicting_framebuffers(NULL, "sp7350drmfb", false);

	/* display controller init */
	DRM_DEV_DEBUG_DRIVER(dev, "sp7350_drm_modeset_init\n");
	ret = sp7350_drm_modeset_init(drm);
	if (ret < 0)
		goto err_unbind_all;

	/* init kms poll for handling hpd */
	DRM_DEV_DEBUG_DRIVER(dev, "drm_kms_helper_poll_init\n");
	drm_kms_helper_poll_init(drm);

	//drm_for_each_crtc(crtc, drm) {
	//	struct sp7350_crtc *sp_crtc;
	//	sp_crtc = to_sp7350_crtc(crtc);
	//}

	DRM_DEV_DEBUG_DRIVER(dev, "drm_dev_register\n");
	ret = drm_dev_register(drm, 0);
	if (ret < 0)
		goto err_cleanup_poll;

	DRM_DEV_DEBUG_DRIVER(dev, "drm_fbdev_generic_setup\n");
	drm_fbdev_generic_setup(drm, 16);

	DRM_DEV_DEBUG_DRIVER(dev, "drm bind success.\n");

	return 0;

err_free_mem:
	for (i = 0; i < SP_DISP_MAX_OSD_LAYER; i++) {
		dma_free_coherent(&pdev->dev, 0x100000, sp_dev->osd_hdr[i],
				  sp_dev->osd_hdr_phy[i]);
	}
err_cleanup_poll:
	DRM_DEV_DEBUG_DRIVER(dev, "drm_kms_helper_poll_fini\n");
	drm_kms_helper_poll_fini(drm);

err_unbind_all:
	component_unbind_all(dev, drm);

	DRM_DEV_DEBUG_DRIVER(dev, "drm bind fail.\n");

	return ret;
}

static void sp7350_drm_unbind(struct device *dev)
{
	struct drm_device *drm = dev_get_drvdata(dev);


	drm_dev_unregister(drm);

	drm_kms_helper_poll_fini(drm);

	drm_atomic_helper_shutdown(drm);
	component_unbind_all(drm->dev, drm);

}

static const struct component_master_ops sp7350_drm_ops = {
	.bind = sp7350_drm_bind,
	.unbind = sp7350_drm_unbind,
};

static struct platform_driver *const component_drivers[] = {
	&sp7350_crtc_driver,
	&sp7350_dsi_driver,
};

static int compare_dev(struct device *dev, void *data)
{
	return dev == data;
}

static int sp7350_drm_probe(struct platform_device *pdev)
{
	struct component_match *match = NULL;
	struct device *dev = &pdev->dev;
	int count = ARRAY_SIZE(component_drivers);
	int i;

	for (i = 0; i < count; i++) {
		struct device_driver *drv = &component_drivers[i]->driver;
		struct device *p = NULL, *d;

		DRM_DEV_DEBUG_DRIVER(&pdev->dev, "%s component %d\n", __func__, i);
		while ((d = platform_find_device_by_driver(p, drv))) {
			put_device(p);
			component_match_add(dev, &match, compare_dev, d);
			p = d;
		}
		put_device(p);
	}

	return component_master_add_with_match(dev, &sp7350_drm_ops, match);
}

static int sp7350_drm_remove(struct platform_device *pdev)
{
	component_master_del(&pdev->dev, &sp7350_drm_ops);

	return 0;
}

static const struct of_device_id sp7350_drm_drv_of_table[] = {
	{ .compatible = "sunplus,sp7350-display-subsystem" },
	{ }
};
MODULE_DEVICE_TABLE(of, sp7350_drm_drv_of_table);

static struct platform_driver sp7350_drm_platform_driver = {
	.probe		= sp7350_drm_probe,
	.remove		= sp7350_drm_remove,
	.driver		= {
		.name	= "sp7350-drm",
		.of_match_table = sp7350_drm_drv_of_table,
		.pm	= &sp7350_drm_pm_ops,
	},
};

static int __init sp7350_drm_register(void)
{
	int ret;

	ret = platform_register_drivers(component_drivers,
					ARRAY_SIZE(component_drivers));
	if (ret)
		return ret;

	ret = platform_driver_register(&sp7350_drm_platform_driver);
	if (ret) {
		platform_unregister_drivers(component_drivers,
					    ARRAY_SIZE(component_drivers));
	}

	return ret;
}

static void __exit sp7350_drm_unregister(void)
{
	platform_unregister_drivers(component_drivers,
				    ARRAY_SIZE(component_drivers));
	platform_driver_unregister(&sp7350_drm_platform_driver);
}

module_init(sp7350_drm_register);
module_exit(sp7350_drm_unregister);

MODULE_AUTHOR("dx.jiang <dx.jiang@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus SP7350 DRM Driver");
MODULE_LICENSE("GPL");
