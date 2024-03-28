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

/* NOTES: Enable this!
 * This may not be a good solution, but it is indeed an effective one.
 * Becease:(VPP not support rgb pixel format)
 * 1. legacy fbdev compatible /dev/fb0 with osd0.
 * 2. work well with xorg-server for rgb format.
 ****************
 * VPP Layer map to overlay plane-1 for media plane usage.
 * Maps list:
 * DRM-Layer | primary   | overlay-1  |  overlay-2  |  overlay-3  |   cursor???
 * planeId   | plane-0   |  plane-1   |   plane-2   |  plane-3    |  plane-4???
 * HW-Later  |  OSD3     |   VPP0     |   OSD2      |   OSD1      |   OSD0???
 * usage     |  desktop  |   media    |   menu-1    |    menu-2   |    cursor
 *  z-order???
 */
#define  DRM_PRIMARY_PLANE_WITH_OSD  1

#define  DRM_PRIMARY_PLANE_ONLY  0

#define XRES_MIN    16
#define YRES_MIN    16

/* [TODO]:
 *  For VPP, Max Resolution 3840x2880.
 *  For OSD, Max Resolution 1920x1080.
 */
#define XRES_MAX  1920
#define YRES_MAX  1280

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
};

#define to_sp7350_drm_dev(target)\
	container_of(target, struct sp7350_drm_device, ddev)

/* sp7350_drm_dsi.c */
extern struct platform_driver sp7350_dsi_driver;
/* sp7350_drm_crtc.c */
extern struct platform_driver sp7350_crtc_driver;

#endif /* __SUNPLUS_SP7350_DRM_DRV_H__ */
