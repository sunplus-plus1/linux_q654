# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Vicore stereo engine drivers.
# Author:Saxen Ko <saxen.ko@vicorelogic.com>

obj-$(CONFIG_STEREO_SUNPLUS_DRV) += vcl_stereo.o
vcl_stereo-objs := vicore_stereo.o vicore_stereo_util.o vicore_stereo_tuningfs.o

ifeq ($(CONFIG_PM), y)
vcl_stereo-objs += vicore_stereo_pm.o
endif
