# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Sunplus stereo engine drivers.
# Author:Saxen Ko <saxen.ko@sunplus.com>

obj-$(CONFIG_STEREO_SUNPLUS_DRV) += sp_stereo.o
sp_stereo-objs := sunplus_stereo.o sunplus_stereo_util.o sunplus_stereo_tuningfs.o

ifeq ($(CONFIG_PM), y)
sp_stereo-objs += sunplus_stereo_pm.o
endif
