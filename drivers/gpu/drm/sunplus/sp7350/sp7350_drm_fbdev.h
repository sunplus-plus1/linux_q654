/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM fbdev
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */


#ifndef _SP7350_DRM_FBDEV_H_
#define _SP7350_DRM_FBDEV_H_

#ifdef CONFIG_DRM_FBDEV_EMULATION

int sp7350_drm_fbdev_init(struct drm_device *dev,
			     unsigned int preferred_bpp);
//void sp7350_drm_fbdev_fini(struct drm_device *dev);

#else

static inline int sp7350_drm_fbdev_init(struct drm_device *dev,
			     unsigned int preferred_bpp)
{
	return 0;
}

//static inline void sp7350_drm_fbdev_fini(struct drm_device *dev)
//{
//}

#endif

#endif
