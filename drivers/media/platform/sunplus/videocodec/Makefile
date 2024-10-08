# SPDX-License-Identifier: GPL-2.0

ccflags-y += -Wframe-larger-than=2048

obj-y += vc_pwr_ctrl.o

obj-$(CONFIG_VIDEO_HANTRO_DECODER) += hx170dec.o
obj-$(CONFIG_VIDEO_HANTRO_ENCODER) += hx280enc.o
obj-$(CONFIG_VIDEO_HANTRO_DECODER_MEM) += hx170dec_mem.o
obj-$(CONFIG_VIDEO_HANTRO_ENCODER_MEM) += hx280enc_mem.o
obj-$(CONFIG_VIDEO_HWCODEC_DRIVERS) += hantro_codec.o

obj-$(CONFIG_VIDEO_HANTRO_V4L2_ENCODER) += vsi-v4l2-enc.o
obj-$(CONFIG_VIDEO_HANTRO_V4L2_DECODER) += vsi-v4l2-dec.o

ifeq (y,$(findstring y, $(CONFIG_VIDEO_HANTRO_V4L2_DECODER) $(CONFIG_VIDEO_HANTRO_V4L2_ENCODER)))
obj-y += vsi-v4l2.o vsi-v4l2daemon.o vsi-v4l2-config.o vsi-v4l2-params.o vsi-dma-contig.o
endif
