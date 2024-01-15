/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM driver
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/slab.h>

#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_irq.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

#include "sp7350_drm_drv.h"
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
	//.driver_features	= DRIVER_MODESET | DRIVER_ATOMIC | DRIVER_GEM,
	.driver_features	= DRIVER_MODESET | DRIVER_GEM,
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

	drm_kms_helper_poll_disable(sdev->ddev);
	//sp7350_drm_crtc_suspend(&sdev->crtc);

	return 0;
}

static int sp7350_drm_pm_resume(struct device *dev)
{
	struct sp7350_drm_device *sdev = dev_get_drvdata(dev);

	drm_modeset_lock_all(sdev->ddev);
	//sp7350_drm_crtc_resume(&sdev->crtc);
	drm_modeset_unlock_all(sdev->ddev);

	drm_kms_helper_poll_enable(sdev->ddev);
	return 0;
}
#endif

static const struct dev_pm_ops sp7350_drm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp7350_drm_pm_suspend, sp7350_drm_pm_resume)
};

/* -----------------------------------------------------------------------------
 * Platform driver
 */

static int sp7350_drm_remove(struct platform_device *pdev)
{
	struct sp7350_drm_device *sdev = platform_get_drvdata(pdev);
	struct drm_device *ddev = sdev->ddev;

	drm_dev_unregister(ddev);
	drm_kms_helper_poll_fini(ddev);
	drm_irq_uninstall(ddev);
	drm_dev_put(ddev);

	return 0;
}

static int sp7350_drm_probe(struct platform_device *pdev)
{
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

	ret = sp7350_drm_modeset_init(sdev);
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

module_platform_driver(sp7350_drm_platform_driver);

MODULE_AUTHOR("dx.jiang <dx.jiang@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus SP7350 DRM Driver");
MODULE_LICENSE("GPL");

