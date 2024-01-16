/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM driver
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#ifndef __SUNPLUS_SP7350_DRM_DRV_H__
#define __SUNPLUS_SP7350_DRM_DRV_H__

//#include <linux/kernel.h>
#include <linux/spinlock.h>

#include <drm/drm.h>
#include <drm/drm_gem.h>

#define XRES_MIN    20
#define YRES_MIN    20

#define XRES_DEF  1024
#define YRES_DEF   768

#define XRES_MAX  8192
#define YRES_MAX  8192

struct sp7350_drm_device {
	struct device *dev;
	//const struct sp7350_drm_platform_data *pdata;
	struct platform_device *platform;

	//void __iomem *mmio;
	//struct clk *clock;
	//u32 lddckr;
	//u32 ldmt1r;

	//spinlock_t irq_lock;		/* Protects hardware LDINTR register */

	//struct drm_device *ddev;
	struct drm_device ddev;

	//struct sp7350_drm_output output;

	//struct sp7350_drm_crtc crtc;
	//struct sp7350_drm_encoder encoder;
	//struct sp7350_drm_connector connector;
};

#define to_sp7350_drm_dev(target)\
	container_of(target, struct sp7350_drm_device, ddev)

/* sp7350_drm_dsi.c */
extern struct platform_driver sp7350_dsi_driver;
/* sp7350_drm_crtc.c */
extern struct platform_driver sp7350_crtc_driver;

#endif /* __SUNPLUS_SP7350_DRM_DRV_H__ */
