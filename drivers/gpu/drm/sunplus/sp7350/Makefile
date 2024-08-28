# SPDX-License-Identifier: GPL-2.0-only
sp7350-drm-y := \
	sp7350_drm_plane.o \
	sp7350_drm_kms.o \
	sp7350_drm_crtc.o \
	sp7350_drm_dsi.o  \
	sp7350_drm_drv.o

#sp7350-drm-$(CONFIG_DRM_FBDEV_EMULATION) += sp7350_drm_fbdev.o
sp7350-drm-$(CONFIG_DRM_PANEL_SP7350_SIMPLE_DSI) += sp7350_drm_dsi_panel_simple.o

sp7350-drm-y += sp7350_drm_debugfs.o

obj-$(CONFIG_DRM_SP7350) += sp7350-drm.o
