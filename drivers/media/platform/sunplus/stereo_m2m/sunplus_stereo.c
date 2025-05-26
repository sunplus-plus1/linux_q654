/*!
 * sunplus_stereo.c - stereo out data
 * @file sunplus_stereo.c
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/of_address.h>

#ifdef CONFIG_PM
#include <linux/pm_runtime.h>
#include "sunplus_stereo_pm.h"
#endif

#include "sunplus_stereo_reg.h"
#include "sunplus_stereo.h"
#include "sunplus_stereo_tuningfs.h"
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>

#define MODE_NAME "sunplus-stereo"

#define STEREO_MIN_WIDTH 160
#define STEREO_MIN_HEIGHT 20
#define STEREO_MAX_WIDTH 1280
#define STEREO_MAX_HEIGHT 1020
#define STEREO_W_STEP 160
#define STEREO_H_STEP 20

#define STEREO_DEFAULT_WIDTH 1280
#define STEREO_DEFAULT_HEIGHT 720

#define STEREO_FWRR_MEM_SIZE	0x21C0000
#define STEREO_RWFR_MEM_SIZE	0x10E0000

struct kobject *sp_stereo_kobj = NULL;
static int stereo_init_controls(struct stereo_video_fh *sfh);

// workqueue relative
static struct workqueue_struct *stereo_wq = NULL;
bool sgm_8dir_available = false;
static int clk_gating = 1;

static irqreturn_t sunplus_stereo_interrupt(int irq, void *dev_id)
{
	sunplus_stereo_ISR_handler(dev_id);
	return IRQ_HANDLED;
}

static int job_ready(void *priv)
{
	struct stereo_video_fh *sfh = priv;
	// struct sunplus_stereo *video = sfh->video;

	if (v4l2_m2m_num_src_bufs_ready(sfh->vfh.m2m_ctx) < 1 ||
		v4l2_m2m_num_dst_bufs_ready(sfh->vfh.m2m_ctx) < 1)
	{
		// v4l2_info(video->vdev.v4l2_dev, "Not enough buffers available\n\n");
		return 0;
	}

	return 1;
}

static void job_abort(void *priv)
{
	/* make sure hw is idle */
	if (g_stereo_status == SP_STEREO_RUN) {
		long timeout_jiff = msecs_to_jiffies(VCL_STEREO_TIMEOUT);

		// if (wait_event_interruptible_timeout(stereo_process_wait,
		// 	!g_stereo_is_running, timeout_jiff)<= 0) { // wait timeout
		if (wait_for_completion_timeout(&stereo_comp, timeout_jiff)) {
			pr_err("job abort: wait finish timeout\n");
		}
	}

	pr_debug("job aborted!\n");
}

static void device_run(void *priv)
{
	int ret;
	struct stereo_video_fh *sfh = priv;

	ret = queue_work(stereo_wq, &sfh->hw_work);
	if (ret == 0)
		pr_debug("work_queue task exist\n");
}

static void stereo_run(struct work_struct *work)
{
	struct stereo_video_fh *sfh = container_of((void *)work, struct stereo_video_fh, hw_work);
	struct v4l2_m2m_dev *m2m_dev = sfh->video->m2m_dev;
	struct v4l2_m2m_ctx *m2m_ctx = sfh->vfh.m2m_ctx;
	long timeout_jiff = msecs_to_jiffies(VCL_STEREO_TIMEOUT);
	int ret;

	pr_debug("stereo %p %s enter\n", sfh, __func__);

	mutex_lock(&sfh->video->device_lock);

	sunplus_stereo_clk_gating(sfh->video->clk_gate, false);
	g_stereo_status = SP_STEREO_RUN;

	// Get buffers
	ret = sunplus_stereo_dma_update(&sfh->vfh);
	if (ret < 0) {
		pr_err("apply Stereo setting error!\n");
		goto job_unlock;
	}
	// Load setting
	sunplus_stereo_apply_setting(&sfh->vfh);

	// HW trigger start
	reinit_completion(&stereo_comp); //stereo_comp.done = 0;
	sunplus_stereo_start_stereo();

	ret = wait_for_completion_timeout(&stereo_comp, timeout_jiff);
	// ret = wait_event_interruptible_timeout(stereo_process_wait,
		// !g_stereo_is_running, timeout_jiff);
	if (ret > 0) {
		v4l2_m2m_buf_done_and_job_finish(m2m_dev, m2m_ctx, VB2_BUF_STATE_DONE);
	} else { // wait timeout
		pr_err("stereo timeout\n");
		if (g_stereo_status == SP_STEREO_RUN) {
			// FIXME: trigger again?
			pr_err("trigger again\n");
			sunplus_stereo_start_stereo();
		}
		// no finish job with error, just let it timeout
		// v4l2_m2m_buf_done_and_job_finish(m2m_dev, m2m_ctx, VB2_BUF_STATE_ERROR);
	}

job_unlock:
	g_stereo_status = SP_STEREO_IDLE;
	sunplus_stereo_clk_gating(sfh->video->clk_gate, true);
	mutex_unlock(&sfh->video->device_lock);

	pr_debug("stereo %p %s exit\n", sfh, __func__);
}

static struct v4l2_m2m_ops m2m_ops = {
	.device_run = device_run,
	.job_ready = job_ready,
	.job_abort = job_abort,
};

int stereo_input_queue_setup(struct vb2_queue *queue,
				 unsigned int *num_buffers,
				 unsigned int *num_planes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	struct stereo_video_fh *sfh = vb2_get_drv_priv(queue);
	int i;

	if (sfh->config.tof_mode == STEREO_WO_TOF)
		*num_planes = 2;
	else
		*num_planes = 4;

	for (i = 0; i < *num_planes; i++) {
		sizes[i] = sfh->out_format.fmt.pix_mp.plane_fmt[i].sizeimage;
		if (sizes[i] == 0) {
			pr_cont(" error with image size=0\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void stereo_input_buffer_queue(struct vb2_buffer *buf)
{
	struct stereo_video_fh *sfh = vb2_get_drv_priv(buf->vb2_queue);

	v4l2_m2m_buf_queue(sfh->vfh.m2m_ctx, to_vb2_v4l2_buffer(buf));
}

int stereo_output_queue_setup(struct vb2_queue *queue,
				 unsigned int *num_buffers,
				 unsigned int *num_planes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	struct stereo_video_fh *sfh = vb2_get_drv_priv(queue);

	*num_planes = 1;

	sizes[0] = sfh->cap_format.fmt.pix.sizeimage;
	if (sizes[0] == 0) {
		pr_cont(" error with image size=0\n");
		return -EINVAL;
	}

	return 0;
}

static void stereo_output_buffer_queue(struct vb2_buffer *buf)
{
	struct stereo_video_fh *sfh = vb2_get_drv_priv(buf->vb2_queue);

	v4l2_m2m_buf_queue(sfh->vfh.m2m_ctx, to_vb2_v4l2_buffer(buf));
}

static const struct vb2_ops stereo_queue_input_ops = {
	.queue_setup = stereo_input_queue_setup,
	.buf_queue = stereo_input_buffer_queue,
};

static const struct vb2_ops stereo_queue_output_ops = {
	.queue_setup = stereo_output_queue_setup,
	.buf_queue = stereo_output_buffer_queue,
};

static int stereo_queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct stereo_video_fh *sfh = priv;
	struct sunplus_stereo *video = sfh->video;
	int ret = 0;

	mutex_init(&sfh->queue_lock);

	// mutex_lock(&sfh->queue_lock);
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = sfh;
	src_vq->ops = &stereo_queue_input_ops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &sfh->queue_lock;
	src_vq->dev = video->v4l2_dev.dev;
	src_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(src_vq);
	if (ret < 0) {
		v4l2_err(&video->v4l2_dev, "src vb2_queue_init failed\n");
		goto exit;
	}

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = sfh;
	dst_vq->ops = &stereo_queue_output_ops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &sfh->queue_lock;
	dst_vq->dev = video->v4l2_dev.dev;
	dst_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(dst_vq);
	if (ret < 0)
		v4l2_err(&video->v4l2_dev, "dst vb2_queue_init failed\n");

exit:
	// mutex_unlock(&sfh->queue_lock);
	return ret;
}

static int stereo_video_open(struct file *file)
{
	struct sunplus_stereo *video = video_drvdata(file);
	struct video_device *vdev = video_devdata(file);
	struct stereo_video_fh *sfh;
	struct stereo_config *pCfg;
	struct v4l2_m2m_ctx *ctx = NULL;
	int ret;

#ifdef CONFIG_PM
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	pm_runtime_get(video->v4l2_dev.dev);
#endif
#endif
	mutex_lock(&video->device_lock);

	sfh = kzalloc(sizeof(*sfh), GFP_KERNEL);
	if (!sfh) {
		mutex_unlock(&video->device_lock);
		return -ENOMEM;
	}
	pr_info("%s %px\n", __func__, sfh);

	v4l2_fh_init(&sfh->vfh, vdev);
	v4l2_fh_add(&sfh->vfh);

	sfh->video = video;
	file->private_data = &sfh->vfh;

	ctx = v4l2_m2m_ctx_init(video->m2m_dev, sfh, stereo_queue_init);
	if (IS_ERR(ctx))
	{
		ret = PTR_ERR(ctx);
		goto done;
	}

	sfh->vfh.m2m_ctx = ctx;
	sfh->vfh.vdev = vdev;

	ret = stereo_init_controls(sfh);
	if (ret) {
		pr_err("stereo_init_controls failed\n");
		goto done;
	}
	sfh->vfh.ctrl_handler = &sfh->ctrl_handler;

	// __INIT_WORK(&sfh->hw_work, stereo_run, stereo_video);
	INIT_WORK(&sfh->hw_work, stereo_run);

	pCfg = &sfh->config;
	pCfg->tof_mode = STEREO_WO_TOF;
	pCfg->sgm_mode = STEREO_4_DIR;
	pCfg->output_fmt = DEPTH_W_THREHOLD;
	pCfg->crop_en = 0;

	/* enable stereo interrupt */
	sunplus_stereo_clk_gating(video->clk_gate, false);
	sunplus_stereo_intr_enable(1);
	sunplus_stereo_clk_gating(video->clk_gate, true);

	mutex_unlock(&video->device_lock);

	return 0;

done:
	if (ret < 0) {
		v4l2_fh_del(&sfh->vfh);
		v4l2_fh_exit(&sfh->vfh);
		kfree(sfh);
	}

	mutex_unlock(&video->device_lock);
	return ret;
}

static int stereo_video_querycap(struct file *file, void *priv,
			      struct v4l2_capability *cap)
{
	strscpy(cap->driver, "stereo_driver", sizeof(cap->driver));
	strscpy(cap->card, "vio_stereo", sizeof(cap->card));
	strscpy(cap->bus_info, "vio_stereo", sizeof(cap->bus_info));
	return 0;
}

int stereo_video_get_format(struct file *file, void *fh,
				struct v4l2_format *format)
{
	struct stereo_video_fh *sfh = to_stereo_video_fh(fh);

	pr_debug("get format type=%d\n", format->type);

	switch (format->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		*format = sfh->cap_format;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		*format = sfh->out_format;
		break;
	default:
		pr_err("stereo get_format type=%d is not support\n", format->type);
		return -EINVAL;
	}

	return 0;
}

static int stereo_video_set_format(struct file *file, void *fh,
				struct v4l2_format *format)
{
	struct stereo_video_fh *sfh = to_stereo_video_fh(fh);
	struct v4l2_format *fmt;
	struct v4l2_pix_format *pix = &format->fmt.pix;
	struct v4l2_pix_format_mplane *pix_mp = &format->fmt.pix_mp;
	struct stereo_config *pCfg = &sfh->config;
	u32 width, height;
	u32 i;

	switch (format->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		/* save input image resolution */
		width = pix_mp->width;
		height = pix_mp->height;
		pCfg->res_input.width = width;
		pCfg->res_input.height = height;

		pr_info("set format w=%d h=%d type=%d\n", width, height, format->type);

		fmt = &sfh->out_format;
		if (pix_mp->pixelformat != V4L2_PIX_FMT_GREY) {
			pr_err("set output pixelformat %d error\n", pix_mp->pixelformat);
			return -EINVAL;
		}
		for (i = 0; i < pix_mp->num_planes; i++) {
			if (pix_mp->plane_fmt[i].bytesperline < width) {
				pr_err("image bytesperline too small!\n");
				return -EINVAL;
			}
			if (pix_mp->plane_fmt[i].sizeimage < (width * height)) {
				pr_err("image size too small!\n");
				return -EINVAL;
			}
		}

		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		/* save output image resolution */
		width = pix->width;
		height = pix->height;
		/* Check support resolution */
		if ((width > STEREO_MAX_WIDTH) || (height > STEREO_MAX_HEIGHT) ||
			(width % STEREO_W_STEP) ||  (height % STEREO_H_STEP)) {
			pr_err("set %d x %d error\n", width, height);
			return -EINVAL;
		}

		pCfg->res.width = width;
		pCfg->res.height = height;

		pr_info("set format w=%d h=%d type=%d\n", width, height, format->type);

		fmt = &sfh->cap_format;
		if (pix->pixelformat != V4L2_PIX_FMT_Y16) {
			pr_err("set capture pixelformat %d error\n", pix->pixelformat);
			return -EINVAL;
		}
		if (pix->sizeimage < (width * height << 1)) {
			pr_err("image size %d too small!\n", pix->sizeimage);
			return -EINVAL;
		}
		break;
	default:
		pr_err("stereo set_format type=%d is not support\n", format->type);
		return -EINVAL;
	}

	*fmt = *format;
	// memcpy(fmt, format, sizeof(struct v4l2_format));

	return 0;
}

int stereo_video_try_format(struct file *file, void *fh,
				struct v4l2_format *format)
{
	struct v4l2_pix_format *pix = &format->fmt.pix;
	u32 width = pix->width;
	u32 height = pix->height;

	pr_info("try format w=%d h=%d type=%d\n", width, height, format->type);

	if ((width > STEREO_MAX_WIDTH) || (height > STEREO_MAX_HEIGHT) ||
		(width % STEREO_W_STEP) || (height % STEREO_H_STEP)) {
		pr_err("try format error\n");
		return -EINVAL;
	}

	switch (format->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		if (pix->pixelformat != V4L2_PIX_FMT_Y16)
			return -EINVAL;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (pix->pixelformat != V4L2_PIX_FMT_GREY)
			return -EINVAL;
		break;
	default:
		pr_err("stereo set_format type=%d is not support\n", format->type);
		return -EINVAL;
	}

	return 0;
}

static int stereo_video_release(struct file *file)
{
	struct sunplus_stereo *video = video_drvdata(file);
	struct v4l2_fh *vfh = file->private_data;
	struct stereo_video_fh *handle = to_stereo_video_fh(vfh);
	// struct video_device *vdev = video_devdata(file);
	struct stereo_ioctl_param *param = &handle->config.param_tb;

	mutex_lock(&video->device_lock);
	sunplus_stereo_clk_gating(video->clk_gate , false);

	pr_info("%s %px\n", __func__, handle);

	/* wake up all waiting thread */
	// wake_up_all(&stereo_process_wait);
	cancel_work_sync(&handle->hw_work);

	// free m2m
	{
		struct vb2_v4l2_buffer *vbuf;
		struct v4l2_m2m_dev *m2m_dev = video->m2m_dev;
		struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;

		/* return of all pending buffers to vb2 (in error state) */
		while ((vbuf = v4l2_m2m_dst_buf_remove(ctx)) != NULL) {
			v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
			v4l2_m2m_job_finish(m2m_dev, ctx);
		}

		/* return of all pending buffers to vb2 (in error state) */
		while ((vbuf = v4l2_m2m_src_buf_remove(ctx)) != NULL) {
			v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
			v4l2_m2m_job_finish(m2m_dev, ctx);
		}

		v4l2_m2m_streamoff(file, ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
		v4l2_m2m_streamoff(file, ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);

		/* release m2m_ctx */
		v4l2_m2m_ctx_release(vfh->m2m_ctx);
	}

	/* free saved tuning data memory*/
	param->size = 0;
	if (param->paddr) {
		kfree(param->paddr);
		param->paddr = NULL;
	}
	if (param->pdata) {
		kfree(param->pdata);
		param->pdata = NULL;
	}

	/* Release the file handle. */
	v4l2_ctrl_handler_free(&handle->ctrl_handler);
	mutex_destroy(&handle->queue_lock);
	v4l2_fh_del(vfh);
	v4l2_fh_exit(vfh);
	kfree(handle);
	file->private_data = NULL;

	sunplus_stereo_clk_gating(video->clk_gate,true);
	mutex_unlock(&video->device_lock);

#ifdef CONFIG_PM
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	pm_runtime_put(video->v4l2_dev.dev);
#endif
#endif

	pr_info("%s done\n\n", __func__);
	return 0;
}

static int stereo_enum_cap_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index == 0) {
		f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		f->pixelformat = V4L2_PIX_FMT_Y16;
		return 0;
	}

	return -EINVAL;
}

static int stereo_enum_out_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index == 0) {
		f->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		f->pixelformat = V4L2_PIX_FMT_GREY;

		return 0;
	}

	return -EINVAL;
}

static int stereo_enum_framesizes(struct file *file, void *priv, struct v4l2_frmsizeenum *fsize)
{
	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;

	if ((fsize->pixel_format != V4L2_PIX_FMT_GREY) ||
		(fsize->pixel_format != V4L2_PIX_FMT_Y16)) {
		return -EINVAL;
	}

	fsize->stepwise.min_width = STEREO_MIN_WIDTH;
	fsize->stepwise.max_width = STEREO_MAX_WIDTH;
	fsize->stepwise.step_width = STEREO_W_STEP;
	fsize->stepwise.min_height = STEREO_MIN_HEIGHT;
	fsize->stepwise.max_height = STEREO_MAX_HEIGHT;
	fsize->stepwise.step_height = STEREO_H_STEP;

	return 0;
}

/* static int stereo_video_qbuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct v4l2_fh *vfh = file->private_data;
	int ret;

	pr_debug("%s type=%d", __func__, buf->type);
	ret = v4l2_m2m_qbuf(file, vfh->m2m_ctx, buf);
	pr_cont(",return %d\n", ret);

	{
		struct vb2_v4l2_buffer *dst, *src;

		src = v4l2_m2m_next_src_buf(vfh->m2m_ctx);
		dst = v4l2_m2m_next_dst_buf(vfh->m2m_ctx);
		if (!src)
			pr_debug("src buf is empty\n");
		else if (!dst)
			pr_debug("dst buf is empty\n");
		else
			pr_debug("src and dst buf are ready\n");
	}

	return ret;
}
 */

static int stereo_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct stereo_video_fh *sfh = to_stereo_video_fh(fh);
	struct stereo_config *pCfg = &sfh->config;
	struct v4l2_rect *crop = &pCfg->crop;
	// struct v4l2_m2m_dev *m2m_dev = sfh->video->m2m_dev;

	pr_info("%s type=%d\n", __func__, type);
	// check ROI setting
	if (pCfg->crop_en) {
		if (((crop->left + crop->width) > pCfg->res_input.width) ||
			((crop->top + crop->height) > pCfg->res_input.height)) {
			pr_err("stereo ROI resolution setting Error!\n");
			return -EINVAL;
			}
	} else {
		if (pCfg->res_input.width != pCfg->res.width || pCfg->res_input.height != pCfg->res.height) {
			pr_err("stereo check resolution setting Error!\n");
			return -EINVAL;
		}
	}

	// v4l2_m2m_resume(m2m_dev);
	return v4l2_m2m_ioctl_streamon(file, fh, type);
}

static int stereo_streamoff(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct stereo_video_fh *sfh = to_stereo_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = sfh->vfh.m2m_ctx;
	struct v4l2_m2m_dev *m2m_dev = sfh->video->m2m_dev;
	struct vb2_v4l2_buffer *vbuf;

	pr_info("%s type=%d\n", __func__, type);

	// v4l2_m2m_suspend(m2m_dev);
	cancel_work_sync(&sfh->hw_work);

	/* return of all pending buffers to vb2 (in error state) */
	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(type))
			vbuf = v4l2_m2m_src_buf_remove(ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx);

		if (vbuf == NULL)
			break;
		v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
		v4l2_m2m_job_finish(m2m_dev, ctx);
	}

	return v4l2_m2m_ioctl_streamoff(file, fh, type);
}

/* crop */
int stereo_g_selection(struct file *file, void *fh,
			   struct v4l2_selection *sel)
{
	struct stereo_video_fh *vfh = to_stereo_video_fh(fh);
	struct stereo_config *pCfg = &vfh->config;
	struct v4l2_rect *crop;

	if (sel->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		pr_err("%s type = %d != V4L2_BUF_TYPE_VIDEO_CAPTURE\n", __func__, sel->type);
		return -EINVAL;
	}

	crop = &pCfg->crop;

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = crop->left;
		sel->r.top = crop->top;
		sel->r.width = crop->width;
		sel->r.height = crop->height;
		break;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
		return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int stereo_s_selection(struct file *file, void *fh,
			   struct v4l2_selection *sel)
{
	struct stereo_video_fh *vfh = to_stereo_video_fh(fh);
	struct stereo_config *pCfg = &vfh->config;
	struct v4l2_rect *crop = &pCfg->crop;
	struct v4l2_area *frame = &pCfg->res_input;

	if (sel->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (((sel->r.left + sel->r.width) > frame->width) ||
		((sel->r.top + sel->r.height) > frame->height)) {
		pr_err("%s set stereo ROI failed\n", __func__);
		return -EINVAL;
	}

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		crop->left = sel->r.left;
		crop->top = sel->r.top;
		crop->width = sel->r.width;
		crop->height = sel->r.height;
		pCfg->crop_en = 1;
		break;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
		return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ioctl_ops stereo_video_ioctl_ops = {
	.vidioc_querycap = stereo_video_querycap,
	.vidioc_enum_framesizes = stereo_enum_framesizes,
	.vidioc_enum_fmt_vid_cap = stereo_enum_cap_fmt,
	.vidioc_enum_fmt_vid_out = stereo_enum_out_fmt,
	.vidioc_s_fmt_vid_cap = stereo_video_set_format,
	.vidioc_s_fmt_vid_out_mplane = stereo_video_set_format,

	.vidioc_g_fmt_vid_cap = stereo_video_get_format,
	.vidioc_g_fmt_vid_out_mplane = stereo_video_get_format,

	.vidioc_try_fmt_vid_cap = stereo_video_try_format,
	.vidioc_try_fmt_vid_out_mplane = stereo_video_try_format,

	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,

	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_streamon = stereo_streamon,	//v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = stereo_streamoff,	//v4l2_m2m_ioctl_streamoff,
	.vidioc_g_selection = stereo_g_selection,
	.vidioc_s_selection = stereo_s_selection,
};

static long stereo_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *const udata = (void __user *)arg;
	int ret = 0;

	switch (cmd) {
	case SP_STEREO_IOCTL_S_TUNE_DATA:
	{
		struct v4l2_fh *vfh = file->private_data;
		struct stereo_video_fh *sfh = to_stereo_video_fh(vfh);
		struct stereo_config *pCfg = &sfh->config;

		struct stereo_ioctl_param usr_param;
		struct stereo_ioctl_param *param = &sfh->config.param_tb;
		unsigned int size;
		uint32_t i, addr;

		if (copy_from_user(&usr_param, udata, sizeof(struct stereo_ioctl_param))) {
			ret = -EFAULT;
			break;
		}

		pr_info("Stereo load tuning data (%d)\n", usr_param.size);

		param->size = 0;
		if (!usr_param.size) {
			pr_err("Error! set stereo tune data size = 0\n");
			return -EINVAL;
		}
		size = usr_param.size * sizeof(u32);

		if (param->paddr) {
			kfree(param->paddr);
			param->paddr = NULL;
		}

		if (param->pdata) {
			kfree(param->pdata);
			param->pdata = NULL;
		}

		param->paddr = kmalloc(size, GFP_KERNEL);
		if (!param->paddr) {
			pr_err("kmalloc failed (%d)", size);
			return -ENOMEM;
		}
		param->pdata = kmalloc(size, GFP_KERNEL);
		if (!param->pdata) {
			pr_err("kmalloc failed (%d)", size);
			return -ENOMEM;
		}

		if (copy_from_user(param->paddr, usr_param.paddr, size)) {
			ret = -EFAULT;
			break;
		}
		if (copy_from_user(param->pdata, usr_param.pdata, size)) {
			ret = -EFAULT;
			break;
		}

		// check if address is safe
		for (i = 0; i < usr_param.size; i++) {
			addr = param->paddr[i];
			if (!((addr >= 0x4 && addr <= 0x44) ||
				(addr >= 0x100 && addr <= 0x17C) ||
				(addr >= 0x200 && addr <= 0x4FC))) {
				pr_err("stereo tuning addr=0x%.3x is not allow\n", addr);
				return -EINVAL;
			}

			if (addr == 0x4) {
				STEREO_FUNC_EN_0_RBUS fun_reg;

				fun_reg.RegValue = param->pdata[i];
				//pCfg->tof_mode = fun_reg.f_tof_fusion_en;
				pCfg->sgm_mode = fun_reg.f_sgm_8direction_en & sgm_8dir_available;
				pr_debug("read sgm_8dir_en=%d\n", fun_reg.f_sgm_8direction_en);
			} else if (addr == 0x40) {
				STEREO_OUT_FMT_0_RBUS fmt_reg;

				fmt_reg.RegValue = param->pdata[i];
				pCfg->calibration_data = fmt_reg.f_FB_val;
				pCfg->thresh_val = fmt_reg.f_thresh_val;
				pr_debug("read fb_val=%d thresh_val=%d\n", fmt_reg.f_FB_val, fmt_reg.f_thresh_val);
			}
		}
		param->size = usr_param.size;
		sfh_last = NULL;

		pr_info("Stereo load tuning data OK\n");

		break;
	}
	default:
		return video_ioctl2(file, cmd, arg);
	}

	return ret;
}

static __poll_t stereo_poll(struct file *file, poll_table *wait)
{
	__poll_t ret = v4l2_m2m_fop_poll(file, wait);

	// user can't handle EPOLLERR, just return nothing
	if (ret == EPOLLERR)
		return 0;

	return ret;
}

static const struct v4l2_file_operations stereo_video_fops = {
	.owner = THIS_MODULE,
	.open = stereo_video_open,
	.release = stereo_video_release,
	.poll = stereo_poll, // v4l2_m2m_fop_poll,
	.unlocked_ioctl = stereo_ioctl, // video_ioctl2
	.mmap = v4l2_m2m_fop_mmap,
};

static inline int stereo_register_video_dev(struct sunplus_stereo *video)
{
	int ret;
	struct video_device *vdev = &video->vdev;

	video->m2m_dev = v4l2_m2m_init(&m2m_ops);
	if (IS_ERR(video->m2m_dev)) {
		v4l2_err(&video->v4l2_dev, "Failed to init m2m device\n");
		return PTR_ERR(video->m2m_dev);
	}

	vdev->v4l2_dev = &video->v4l2_dev;
	vdev->release = video_device_release_empty;
	vdev->fops = &stereo_video_fops;
	vdev->ioctl_ops = &stereo_video_ioctl_ops;
	vdev->vfl_dir = VFL_DIR_M2M;
	vdev->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT_MPLANE;
	//vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT_MPLANE;
	vdev->lock = &video->device_lock;
	ret = video_register_device(vdev, VFL_TYPE_VIDEO, STEREO_VIDEO_NUMBER);
	if (ret < 0) {
		v4l2_err(&video->v4l2_dev, "video_register_device failed\n");
		return ret;
	}

	video->type = V4L2_BUF_TYPE_VIDEO_CAPTURE | V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	video_set_drvdata(vdev, video);

	return 0;
}

static int stereo_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct stereo_video_fh *sfh = ctrl->priv;

	switch (ctrl->id) {
	case SUNPLUS_CID_CALIBRATION_DATA:
		pr_info("set focal baseline=%u\n", ctrl->val);
		sfh->config.calibration_data = ctrl->val;
		break;
	case SUNPLUS_CID_TOF_MODE:
		pr_info("set tof mode=%u\n", ctrl->val);
		sfh->config.tof_mode = ctrl->val;
		break;
	case SUNPLUS_CID_SGM_MODE:
		pr_info("set sgm mode=%u\n", ctrl->val);
		sfh->config.sgm_mode = ctrl->val & sgm_8dir_available;
		break;
	case SUNPLUS_CID_OUTPUT_FMT:
		pr_info("set output format=%u\n", ctrl->val);
		sfh->config.output_fmt = ctrl->val;
		break;
	case SUNPLUS_CID_OUTPUT_THRESHOLD:
		pr_info("set output threshold=%u\n", ctrl->val);
		sfh->config.thresh_val = ctrl->val;
		break;
	default:
		pr_err("not support ctrl id: %d\n", ctrl->id);
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops stereo_ctrl_ops = {
	.s_ctrl = stereo_s_ctrl,
};

static const struct v4l2_ctrl_config stereo_calibration_control = {
	.ops = &stereo_ctrl_ops,
	.id = SUNPLUS_CID_CALIBRATION_DATA,
	.name = "SUNPLUS_CID_CALIBRATION_DATA",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 0xFFFF,
	.def = 0,
	.step = 1,
};

static const struct v4l2_ctrl_config stereo_tof_control = {
	.ops = &stereo_ctrl_ops,
	.id = SUNPLUS_CID_TOF_MODE,
	.name = "SUNPLUS_CID_TOF_MODE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 1,
	.def = 0,
	.step = 1,
};

static const struct v4l2_ctrl_config stereo_sgm_control = {
	.ops = &stereo_ctrl_ops,
	.id = SUNPLUS_CID_SGM_MODE,
	.name = "SUNPLUS_CID_SGM_MODE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 1,
	.def = 0,
	.step = 1,
};

static const struct v4l2_ctrl_config stereo_output_threshold_control = {
	.ops = &stereo_ctrl_ops,
	.id = SUNPLUS_CID_OUTPUT_THRESHOLD,
	.name = "SUNPLUS_CID_OUTPUT_THRESHOLD",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 0x7F,
	.def = 0,
	.step = 1,
};

static const struct v4l2_ctrl_config stereo_output_fmt_control = {
	.ops = &stereo_ctrl_ops,
	.id = SUNPLUS_CID_OUTPUT_FMT,
	.name = "SUNPLUS_CID_OUTPUT_FMT",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 3,
	.def = 2,
	.step = 1,
};

static int stereo_init_controls(struct stereo_video_fh *sfh)
{
	struct v4l2_ctrl_handler *hdlr = &sfh->ctrl_handler;

	v4l2_ctrl_handler_init(hdlr, 8);
	v4l2_ctrl_new_custom(hdlr, &stereo_calibration_control, sfh);
	v4l2_ctrl_new_custom(hdlr, &stereo_tof_control, sfh);
	v4l2_ctrl_new_custom(hdlr, &stereo_sgm_control, sfh);
	v4l2_ctrl_new_custom(hdlr, &stereo_output_fmt_control, sfh);
	v4l2_ctrl_new_custom(hdlr, &stereo_output_threshold_control, sfh);

	if (hdlr->error) {
		v4l2_ctrl_handler_free(hdlr);
		return hdlr->error;
	}

	return 0;
}

int sunplus_stereo_probe(struct platform_device *pdev)
{
	struct sunplus_stereo *video;
	struct device *stereo_dev = &pdev->dev;

	struct resource *res;
	struct resource mem_res;
	struct device_node *node;
	size_t mem_size;
	int ret;

	pr_debug("%s enter\n", __func__);

	if (!pdev->dev.of_node) {
		pr_err("device node fail\n");
		return -ENODEV;
	}

	sp_stereo_kobj = kobject_create_and_add("sunplus_stereo", NULL);
	if (!sp_stereo_kobj) {
		return -ENOMEM;
	}

	video = devm_kzalloc(&pdev->dev, sizeof(struct sunplus_stereo), GFP_KERNEL);
	if (!video) {
		pr_err("alloce stereo device fail\n");
		kobject_put(sp_stereo_kobj);
		sp_stereo_kobj = NULL;
		return -ENOMEM;
	}

	video->v4l2_dev.dev = &pdev->dev;

	mutex_init(&video->device_lock);
	spin_lock_init(&stereo_lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	stereo_base = devm_ioremap_resource(stereo_dev, res);
	if (IS_ERR(stereo_base)) {
		dev_err(stereo_dev, "stereo base:fail\n");
		return PTR_ERR(stereo_base);
	}

	video->irq = platform_get_irq(pdev, 0);
	if (video->irq < 0) {
		dev_err(stereo_dev, "irq:fail\n");
		return -ENXIO;
	}

	ret = devm_request_irq(stereo_dev, video->irq, sunplus_stereo_interrupt, 0, dev_name(stereo_dev), video);
	if (ret < 0)
		dev_err(stereo_dev, "failed to request irq\n");

	if (clk_gating) {
		video->vcl_clk_gate = devm_clk_get(stereo_dev, "vcl_clk");
		if (IS_ERR(video->vcl_clk_gate)) {
			dev_err(stereo_dev, "%s Failed to get vcl clock gate\n", __func__);
			video->vcl_clk_gate = NULL;
			//return PTR_ERR(video->vcl_clk_gate);
		} else {
			ret = clk_prepare(video->vcl_clk_gate);
			if (ret) {
				dev_err(stereo_dev, "%s prepare vcl clock failed\n", __func__);
				return -ENODEV;
			} else {
				dev_dbg(stereo_dev, "%s prepare vcl clock \n", __func__);
			}

			clk_enable(video->vcl_clk_gate);
		}

		video->vcl5_clk_gate = devm_clk_get(stereo_dev, "vcl5_clk");
		if (IS_ERR(video->vcl5_clk_gate)) {
			dev_err(stereo_dev, "%s Failed to get vcl5 clock gate\n", __func__);
			video->vcl5_clk_gate = NULL;
			//return PTR_ERR(video->vcl5_clk_gate);
		} else {
			ret = clk_prepare(video->vcl5_clk_gate);
			if (ret) {
				dev_err(stereo_dev, "%s prepare vcl5 clock failed\n", __func__);
				return -ENODEV;
			} else {
				dev_dbg(stereo_dev, "%s prepare vcl5 clock \n", __func__);
			}

			clk_enable(video->vcl5_clk_gate);
		}

		video->clk_gate = devm_clk_get(stereo_dev, "sdp_clk");
		if (IS_ERR(video->clk_gate)) {
			dev_err(stereo_dev, "%s Failed to get clock gate\n", __func__);
			video->clk_gate = NULL;
			//return PTR_ERR(video->clk_gate);
		} else {
			ret = clk_prepare(video->clk_gate);
			if (ret) {
				dev_err(stereo_dev, "%s prepare clock failed\n", __func__);
				return -ENODEV;
			} else {
				dev_dbg(stereo_dev, "%s prepare clock \n", __func__);
			}
		}
	} else {
		dev_err(stereo_dev, "%s clock gate is disabled\n", __func__);
		video->clk_gate = NULL;
	}

	/*
	video->rst = devm_reset_control_get(stereo_dev, "sdp_reset");
	if (IS_ERR(video->rst)){
		pr_err("%s, get devm_reset_control_get() fail\n", __func__);
		video->rst = NULL;
	} else {
		reset_control_deassert(video->rst);
	}

	video->regulator = devm_regulator_get(stereo_dev, "ext_buck1_0v8");
	if (IS_ERR(video->regulator)) {
		pr_err("%s, get devm_regulator_get() fail\n", __func__);
		video->regulator = NULL;
	} else {
		ret = regulator_enable(video->regulator);
		if (ret != 0)
			pr_err("%s, regulator_enable() fail, ret = %d\n", __func__, ret);
	}
	*/

	platform_set_drvdata(pdev, video);

	/* sunplus tuning FS*/
	ret = sunplus_stereo_tuningfs_init(sp_stereo_kobj, video->clk_gate);
	if (ret)
		dev_err(stereo_dev, "cannot create tuningfs (%d)\n", ret);

	/*Get reserved memory from DTS */
	sgm_8dir_available = false;
	node = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!node) {
		dev_dbg(stereo_dev, "no SGM-8 memory-region specified\n");
		goto _SKIP_SGM_MEM;
	}

	ret = of_address_to_resource(node, 0, &mem_res);
	if (ret) {
		dev_err(stereo_dev, "get address fail from memory-region\n");
		goto _SKIP_SGM_MEM;
	}

	mem_size = resource_size(&mem_res);

	if (mem_size < 2 * (STEREO_FWRR_MEM_SIZE + STEREO_RWFR_MEM_SIZE)) {
		dev_err(stereo_dev, "allocate memory size is too small\n");
		sgm_8dir_available = false;
	} else {
		stereo_fwrr_r_mem_phys = mem_res.start;
		/* FWRR_R memory handler */
		mem_size = STEREO_FWRR_MEM_SIZE;
		stereo_fwrr_r_mem_visu = memremap(stereo_fwrr_r_mem_phys, mem_size, MEMREMAP_WC);
		if (!stereo_fwrr_r_mem_visu) {
			dev_err(stereo_dev, "unable to map memory region: %pa+%zx\n", &mem_res.start, mem_size);
			return -ENOMEM;
		}
		dev_dbg(stereo_dev, "RDMA_FWRR_R:pa:0x%0llX, size:0x%lx\n",stereo_fwrr_r_mem_phys, mem_size);

		/* FWRR_W memory handler */
		stereo_fwrr_w_mem_phys = stereo_fwrr_r_mem_phys + STEREO_FWRR_MEM_SIZE;
		mem_size = STEREO_FWRR_MEM_SIZE;
		stereo_fwrr_w_mem_visu = memremap(stereo_fwrr_w_mem_phys, mem_size, MEMREMAP_WC);
		if (!stereo_fwrr_w_mem_visu) {
			dev_err(stereo_dev, "unable to map memory region: %pa+%zx\n", &mem_res.start, mem_size);
			return -ENOMEM;
		}
		dev_dbg(stereo_dev, "RDMA_FWRR_W:pa:0x%0llX, size:0x%lx\n",stereo_fwrr_w_mem_phys, mem_size);

		/* RWFR_R memory handler */
		stereo_rwfr_r_mem_phys = stereo_fwrr_w_mem_phys + STEREO_FWRR_MEM_SIZE;
		mem_size = STEREO_RWFR_MEM_SIZE;
		stereo_rwfr_r_mem_visu = memremap(stereo_rwfr_r_mem_phys, mem_size, MEMREMAP_WC);
		if (!stereo_rwfr_r_mem_visu) {
			dev_err(stereo_dev, "unable to map memory region: %pa+%zx\n", &mem_res.start, mem_size);
			return -ENOMEM;
		}
		dev_dbg(stereo_dev, "RDMA_RWFR_R:pa:0x%0llX, size:0x%lx\n",stereo_rwfr_r_mem_phys, mem_size);

		/* RWFR_W_memory handler */
		stereo_rwfr_w_mem_phys = stereo_rwfr_r_mem_phys + STEREO_RWFR_MEM_SIZE;
		mem_size = STEREO_RWFR_MEM_SIZE;
		stereo_rwfr_w_mem_visu = memremap(stereo_rwfr_w_mem_phys, mem_size, MEMREMAP_WC);
		if (!stereo_rwfr_w_mem_visu) {
			dev_err(stereo_dev, "unable to map memory region: %pa+%zx\n", &mem_res.start, mem_size);
			return -ENOMEM;
		}
		dev_dbg(stereo_dev, "RDMA_RWFR_W:pa:0x%0llX, size:0x%lx\n",stereo_rwfr_w_mem_phys, mem_size);

		sgm_8dir_available = true;
	}

_SKIP_SGM_MEM:
	if (!sgm_8dir_available)
		dev_info(stereo_dev, "SGM-8 is not support\n");

	video->v4l2_dev.ctrl_handler = NULL;
	ret = v4l2_device_register(stereo_dev, &video->v4l2_dev);
	if (ret < 0) {
		dev_err(stereo_dev, "%s: V4L2 device registration failed (%d)\n", __func__, ret);
		goto error_modules;
	}

	ret = stereo_register_video_dev(video);
	if (ret < 0)
		goto error_exit;

	/* vicore stereo initialize setting */
	sunplus_stereo_clk_gating(video->clk_gate, false);
	ret = sunplus_stereo_driver_init();
	if (ret)
		dev_err(stereo_dev, "sunplus stereo initialize (%d)\n", ret);
	sunplus_stereo_clk_gating(video->clk_gate, true);

	// work queue
	// stereo_wq = alloc_workqueue("stereo_run_wq", WQ_UNBOUND, 1);
	stereo_wq = create_singlethread_workqueue("stereo_run_wq");
	if (!stereo_wq) {
		pr_err("%s(%d): initial wq fail\n", __func__, __LINE__);
		goto error_exit;
	}

	/*
	#ifdef CONFIG_PM
	#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
		pm_runtime_set_active(stereo_dev);
		pm_runtime_enable(stereo_dev);
	#endif
	#endif
	*/
	dev_info(stereo_dev, "load stereo driver successful\n");

	return 0;

error_exit:
	v4l2_device_unregister(&video->v4l2_dev);
error_modules:
	// v4l2_ctrl_handler_free(&video->v4l2_dev.ctrl_handler);
	/*
	if (video->rst)
		reset_control_assert(video->rst);

	if (video->regulator)
		regulator_disable(video->regulator);
	*/

	return -ENOMEM;
}

int sunplus_stereo_remove(struct platform_device *pdev)
{
	struct sunplus_stereo *video = platform_get_drvdata(pdev);

	pr_info("%s enter\n", __func__);
	sunplus_stereo_clk_gating(video->clk_gate, false);
	/* disable stereo interrupt */
	sunplus_stereo_intr_enable(0);

	/* release m2m device */
	if (video->m2m_dev)
		v4l2_m2m_release(video->m2m_dev);

	if (stereo_wq != NULL) {
		flush_workqueue(stereo_wq);
		destroy_workqueue(stereo_wq);
		stereo_wq = NULL;
	}

	if (video) {
		/*
		if (video->regulator) {
			ret = regulator_enable(video->regulator);
			if (ret != 0)
				pr_err("%s, regulator_enable() fail, ret = %d\n", __func__, ret);
		}
		*/

		video_unregister_device(&video->vdev);
		// v4l2_ctrl_handler_free(&stereo->ctrl_handler);
		v4l2_device_unregister(&video->v4l2_dev);

		/*
		#ifdef CONFIG_PM
		#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
		pm_runtime_disable(&pdev->dev);
		#endif
		#endif

		if (video->regulator) {
			ret = regulator_disable(video->regulator);
			if (ret != 0)
				pr_err("%s, regulator_disable() fail, ret = %d\n", __func__, ret);
		}
		*/
		devm_kfree(&pdev->dev, video);
	}

	if (stereo_fwrr_r_mem_visu)
		memunmap(stereo_fwrr_r_mem_visu);
	if (stereo_fwrr_w_mem_visu)
		memunmap(stereo_fwrr_w_mem_visu);
	if (stereo_rwfr_r_mem_visu)
		memunmap(stereo_rwfr_r_mem_visu);
	if (stereo_rwfr_w_mem_visu)
		memunmap(stereo_rwfr_w_mem_visu);

	if (sp_stereo_kobj) {
		kobject_put(sp_stereo_kobj);
		sp_stereo_kobj = NULL;
	}
	sunplus_stereo_clk_gating(video->clk_gate, true);
	if (video->clk_gate) {
		clk_unprepare(video->clk_gate);
	}

	return 0;
}

#ifdef CONFIG_PM
static const struct dev_pm_ops vicore_stereo_pm = {
	#ifdef CONFIG_PM_SYSTEM_VPU_DEFAULT
	SET_SYSTEM_SLEEP_PM_OPS(sunplus_stereo_suspend_ops, sunplus_stereo_resume_ops)
	#endif
	#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	//SET_RUNTIME_PM_OPS(sunplus_stereo_suspend_ops, sunplus_stereo_resume_ops, NULL)
	#endif
};
#endif

static const struct of_device_id vicore_stereo_dt_match[] = {
	{ .compatible = "sunplus,sp7350-stereo",},
	{ },
};
MODULE_DEVICE_TABLE(of, vicore_stereo_dt_match);

static struct platform_driver vicore_stereo_driver = {
	.probe	= sunplus_stereo_probe,
	.remove	= sunplus_stereo_remove,
	.driver	= {
		.name = MODE_NAME,
#ifdef CONFIG_PM
		.pm = &vicore_stereo_pm,
#endif
		.of_match_table = of_match_ptr(vicore_stereo_dt_match),
	},
};

int __init vicore_stereo_mod_init(void)
{
	pr_debug("%s init\n", MODE_NAME);
	return platform_driver_register(&vicore_stereo_driver);
}

void __exit vicroe_stereo_mod_exit(void)
{
	pr_debug("%s exit\n", MODE_NAME);
	platform_driver_unregister(&vicore_stereo_driver);
}

module_param(clk_gating, int, 0644);

module_init(vicore_stereo_mod_init);
module_exit(vicroe_stereo_mod_exit);

MODULE_DESCRIPTION("Sunplus Stereo Engine M2M Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Saxen Ko");
MODULE_ALIAS("platform:" MODE_NAME);
