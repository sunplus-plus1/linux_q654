/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM driver
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 *         hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#ifndef __SUNPLUS_SP7350_DRM_DRV_H__
#define __SUNPLUS_SP7350_DRM_DRV_H__

//#include <linux/kernel.h>
#include <linux/spinlock.h>

#include <drm/drm.h>
#include <drm/drm_gem.h>

//#include <drm/drm_atomic.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_device.h>
//#include <drm/drm_encoder.h>
#include <drm/drm_gem_cma_helper.h>
//#include <drm/drm_managed.h>
//#include <drm/drm_mm.h>
//#include <drm/drm_modeset_lock.h>

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

#define	SP7350_DRM_LAYER_TYPE_OSD3 0x0 /* SP7350_DMIX_L0 set to OSD3 (zpos = 0) */
#define	SP7350_DRM_LAYER_TYPE_VPP0 0x1 /* SP7350_DMIX_L1 set to VPP0 (zpos = 1) */
#define	SP7350_DRM_LAYER_TYPE_OSD2 0x2 /* SP7350_DMIX_L2 set to OSD2 (zpos = 2) */
#define	SP7350_DRM_LAYER_TYPE_OSD1 0x3 /* SP7350_DMIX_L3 set to OSD1 (zpos = 3) */
#define	SP7350_DRM_LAYER_TYPE_OSD0 0x4 /* SP7350_DMIX_L4 set to OSD0 (zpos = 4) */

#define XRES_MIN    16
#define YRES_MIN    16

/* [TODO]:
 *  For VPP, Max Resolution 3840x2880.
 *  For OSD, Max Resolution 1920x1080.
 */
#define XRES_MAX  1920
#define YRES_MAX  1280

#define	SP_DISP_MAX_OSD_LAYER	4
#define	SP_DISP_MAX_VPP_LAYER	1

struct sp7350_dev {
	struct drm_device base;
	struct device *dev;

	void __iomem *crtc_regs;
	//void __iomem *dsi_regs;
	//void __iomem *ao3_regs;

	void *osd_hdr[SP_DISP_MAX_OSD_LAYER];
	dma_addr_t osd_hdr_phy[SP_DISP_MAX_OSD_LAYER];

	/*
	 * Set to true when the debug test is active.
	 * (reserved for future use) 
	 */
	bool debug_test_enabled;

	struct list_head debugfs_list;

};

#define SP7350_DRM_REG32(reg) { .name = #reg, .offset = reg }

/* sp7350_drm_debugfs.c */
void sp7350_debugfs_init(struct drm_minor *minor);
#ifdef CONFIG_DEBUG_FS
void sp7350_debugfs_add_file(struct drm_device *drm,
			  const char *filename,
			  int (*show)(struct seq_file*, void*),
			  void *data);
void sp7350_debugfs_add_regset32(struct drm_device *drm,
			      const char *filename,
			      struct debugfs_regset32 *regset);
#else
static inline void sp7350_debugfs_add_file(struct drm_device *drm,
					const char *filename,
					int (*show)(struct seq_file*, void*),
					void *data)
{
}

static inline void sp7350_debugfs_add_regset32(struct drm_device *drm,
					    const char *filename,
					    struct debugfs_regset32 *regset)
{
}
#endif

#define to_sp7350_dev(dev)\
	container_of(dev, struct sp7350_dev, base)

/* sp7350_drm_dsi.c */
extern struct platform_driver sp7350_dsi_driver;
/* sp7350_drm_crtc.c */
extern struct platform_driver sp7350_crtc_driver;

#endif /* __SUNPLUS_SP7350_DRM_DRV_H__ */
