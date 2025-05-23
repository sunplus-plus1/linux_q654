/*!
 * sunplus_stereo.h - stereo out data
 * @file sunplus_stereo.h
 * @brief stereo video device
 * @author Saxen Ko <saxen.ko@sunplus.com>
 * @version 1.0
 * @copyright  Copyright (C) 2025 Sunplus
 * @note
 * Copyright (C) 2025 Sunplus
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifndef SUNPLUS_STEREO_H
#define SUNPLUS_STEREO_H

#include <media/v4l2-device.h>
#include <media/media-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-vmalloc.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/regulator/consumer.h>

#include "sunplus_stereo_util.h"

#define STEREO_VIDEO_NUMBER 102

struct sunplus_stereo {
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	struct v4l2_m2m_dev *m2m_dev;
	struct clk *clk_gate;
	struct clk *vcl_clk_gate;
	struct clk *vcl5_clk_gate;
	struct reset_control *rst;
	struct regulator *regulator;
	unsigned int irq;
	/* Video buffer type */
	enum v4l2_buf_type type;

	/* ref counter */
	struct mutex device_lock; /* protects the device open/close */
};
#define to_vicore_stereo(dev) container_of(dev, struct sunplus_stereo, stereo_dev)

struct stereo_video_fh {
	struct v4l2_fh vfh;
	struct sunplus_stereo *video;
	// v4l2_ctrl_handler
	struct v4l2_ctrl_handler ctrl_handler;

	/* work thread */
	struct work_struct hw_work;

	// struct vb2_queue
	struct mutex queue_lock; /* protects the vb2 queue */

	struct v4l2_format cap_format;
	struct v4l2_format out_format;

	/* struct stereo_config for each file*/
	unsigned long frame_num;
	struct stereo_config config;
};

#define to_stereo_video_fh(fh) container_of(fh, struct stereo_video_fh, vfh)

/*
	API
 */
extern int stereo_video_queue_setup(struct vb2_queue *queue,
				 unsigned int *num_buffers,
				 unsigned int *num_planes, unsigned int sizes[],
				 struct device *alloc_devs[]);
extern int stereo_g_selection(struct file *file, void *fh, struct v4l2_selection *sel);
extern int stereo_s_selection(struct file *file, void *fh, struct v4l2_selection *sel);
extern int stereo_video_try_format(struct file *file, void *fh, struct v4l2_format *format);
extern int stereo_video_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *rb);
extern int stereo_video_get_format(struct file *file, void *fh, struct v4l2_format *format);

#endif
