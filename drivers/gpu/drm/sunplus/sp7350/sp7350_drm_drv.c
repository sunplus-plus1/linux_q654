/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM driver
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
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
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

#include "sp7350_drm_drv.h"
#include "sp7350_drm_crtc.h"

#include "sp7350_drm_kms.h"
#include "sp7350_drm_plane.h"

/* -----------------------------------------------------------------------------
 * Hardware initialization
 */

/* -----------------------------------------------------------------------------
 * DRM operations
 */


DEFINE_DRM_GEM_CMA_FOPS(sp7350_drm_fops);

static struct drm_driver sp7350_drm_driver = {
	.driver_features	= DRIVER_MODESET | DRIVER_ATOMIC | DRIVER_GEM,
	//.driver_features	= DRIVER_MODESET | DRIVER_GEM,
	//.irq_handler		= sp7350_drm_irq,
	DRM_GEM_CMA_DRIVER_OPS,
	.fops			= &sp7350_drm_fops,

	.name			= "sp7350-drm",
	.desc			= "Sunplus SP7350 DRM",
	.date			= "20240111",
	.major			= 1,
	.minor			= 0,
};

/* -----------------------------------------------------------------------------
 * Power management
 */

#ifdef CONFIG_PM_SLEEP
static int sp7350_drm_pm_suspend(struct device *dev)
{
	struct sp7350_drm_device *sdev = dev_get_drvdata(dev);

	drm_kms_helper_poll_disable(&sdev->ddev);
	//sp7350_drm_crtc_suspend(&sdev->crtc);
	//drm_fb_helper_set_suspend_unlocked
	//drm_mode_config_helper_suspend
	return 0;
}

static int sp7350_drm_pm_resume(struct device *dev)
{
	struct sp7350_drm_device *sdev = dev_get_drvdata(dev);

	drm_modeset_lock_all(&sdev->ddev);
	//sp7350_drm_crtc_resume(&sdev->crtc);
	drm_modeset_unlock_all(&sdev->ddev);

	drm_kms_helper_poll_enable(&sdev->ddev);
	return 0;
}
#endif

static const struct dev_pm_ops sp7350_drm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp7350_drm_pm_suspend, sp7350_drm_pm_resume)
};

/* -----------------------------------------------------------------------------
 * Platform driver
 */
//static const struct of_device_id sp7350_dma_range_matches[] = {
//	{ .compatible = "sunplus,sp7350-dsi-dma" },
//	{}
//};

static int sp7350_drm_bind(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm;
	struct sp7350_drm_device *sdev;
	//struct drm_crtc *crtc;
	int ret = 0;

	/* using device-specific reserved memorym,
	   defined at dts with label drm_disp_reserve */
	ret = of_reserved_mem_device_init(dev);
	if (!ret) {
		dev_info(dev, "using device-specific reserved memory\n");
		ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
		if (ret) {
			dev_err(dev, "32-bit consistent DMA enable failed\n");
			return ret;
		}
	}

	sdev = devm_drm_dev_alloc(dev, &sp7350_drm_driver, struct sp7350_drm_device, ddev);
	if (IS_ERR(sdev))
		return PTR_ERR(sdev);

	drm = &sdev->ddev;
	platform_set_drvdata(pdev, drm);
	//INIT_LIST_HEAD(&sdev->debugfs_list);

	ret = drmm_mode_config_init(drm);
	if (ret)
		return ret;

	//ret = SP7350_drm_gem_init(drm);
	//if (ret)
	//	return ret;

	ret = component_bind_all(dev, drm);
	if (ret)
		return ret;

	ret = sp7350_plane_create_additional_planes(drm);
	if (ret)
		goto unbind_all;

	drm_fb_helper_remove_conflicting_framebuffers(NULL, "sp7350drmfb", false);

	/* display controller init */
	ret = sp7350_drm_modeset_init(drm);
	if (ret < 0)
		goto unbind_all;

	/* TODO. */
	//drm_for_each_crtc(crtc, drm)
	//	sp7350_drm_crtc_disable(crtc);

	ret = drm_dev_register(drm, 0);
	if (ret < 0)
		goto unbind_all;

	drm_fbdev_generic_setup(drm, 16);

	return 0;

unbind_all:
	component_unbind_all(dev, drm);

	return ret;
}

static void sp7350_drm_unbind(struct device *dev)
{
	struct drm_device *drm = dev_get_drvdata(dev);

	drm_dev_unregister(drm);

	drm_atomic_helper_shutdown(drm);
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

		while ((d = platform_find_device_by_driver(p, drv))) {
			put_device(p);
			component_match_add(dev, &match, compare_dev, d);
			p = d;
		}
		put_device(p);
	}

	return component_master_add_with_match(dev, &sp7350_drm_ops, match);
	#if 0
	//struct shmob_drm_platform_data *pdata = pdev->dev.platform_data;
	//struct sp7350fb_info *sp_fbinfo;
	struct sp7350_drm_device *sdev;
	struct drm_device *ddev;
	//struct resource *res;
	//unsigned int i;
	int ret;

	//if (pdata == NULL) {
	//	dev_err(&pdev->dev, "no platform data\n");
	//	return -EINVAL;
	//}
	pr_debug("%s: drm probe ...\n", __func__);

	/*
	 * Allocate and initialize the driver private data, I/O resources and
	 * clocks.
	 */
	sdev = devm_kzalloc(&pdev->dev, sizeof(*sdev), GFP_KERNEL);
	if (sdev == NULL)
		return -ENOMEM;

	sdev->dev = &pdev->dev;
	//sdev->pdata = pdata;
	sdev->platform = pdev;
	//spin_lock_init(&sdev->irq_lock);

	platform_set_drvdata(pdev, sdev);

	//res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	//sdev->mmio = devm_ioremap_resource(&pdev->dev, res);
	//if (IS_ERR(sdev->mmio))
	//	return PTR_ERR(sdev->mmio);

	//ret = shmob_drm_init_interface(sdev);
	//if (ret < 0)
	//	return ret;

	/* Allocate and initialize the DRM device. */
	ddev = drm_dev_alloc(&sp7350_drm_driver, &pdev->dev);
	if (IS_ERR(ddev))
		return PTR_ERR(ddev);

	sdev->ddev = ddev;
	ddev->dev_private = sdev;

	ret = sp7350_drm_modeset_init(ddev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to initialize mode setting\n");
		goto err_free_drm_dev;
	}

	ret = drm_vblank_init(ddev, 1);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to initialize vblank\n");
		goto err_modeset_cleanup;
	}

	//ret = drm_irq_install(ddev, platform_get_irq(pdev, 0));
	//if (ret < 0) {
	//	dev_err(&pdev->dev, "failed to install IRQ handler\n");
	//	goto err_modeset_cleanup;
	//}

	/*
	 * Register the DRM device with the core and the connectors with
	 * sysfs.
	 */
	ret = drm_dev_register(ddev, 0);
	if (ret < 0)
		goto err_irq_uninstall;

	pr_debug("%s: drm probe done\n", __func__);

	return 0;

err_irq_uninstall:
	//drm_irq_uninstall(ddev);
err_modeset_cleanup:
	drm_kms_helper_poll_fini(ddev);
err_free_drm_dev:
	drm_dev_put(ddev);

	return ret;
	#endif
}

static int sp7350_drm_remove(struct platform_device *pdev)
{

	component_master_del(&pdev->dev, &sp7350_drm_ops);

	#if 0
	struct sp7350_drm_device *sdev = platform_get_drvdata(pdev);
	struct drm_device *ddev = sdev->ddev;

	drm_dev_unregister(ddev);
	drm_kms_helper_poll_fini(ddev);
	drm_irq_uninstall(ddev);
	drm_dev_put(ddev);
	#endif

	return 0;
}

static const struct of_device_id sp7350_drm_drv_of_table[] = {
	{ .compatible = "sunplus,sp7350-display-engine" },
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

//module_platform_driver(sp7350_drm_platform_driver);

static int __init sp7350_drm_register(void)
{
	int ret;

	ret = platform_register_drivers(component_drivers,
					ARRAY_SIZE(component_drivers));
	if (ret)
		return ret;

	ret = platform_driver_register(&sp7350_drm_platform_driver);
	if (ret)
		platform_unregister_drivers(component_drivers,
					    ARRAY_SIZE(component_drivers));

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

