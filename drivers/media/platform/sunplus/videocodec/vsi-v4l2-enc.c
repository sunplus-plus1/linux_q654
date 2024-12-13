/*
 *    VSI V4L2 encoder entry.
 *
 *    Copyright (c) 2019, VeriSilicon Inc.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License, version 2, as
 *    published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License version 2 for more details.
 *
 *    You may obtain a copy of the GNU General Public License
 *    Version 2 at the following locations:
 *    https://opensource.org/licenses/gpl-2.0.php
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/videodev2.h>
#include <linux/v4l2-dv-timings.h>
#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include "vsi-v4l2-priv.h"
#include "vsi-dma-priv.h"

static int vsi_enc_querycap(
	struct file *file,
	void *priv,
	struct v4l2_capability *cap)
{
	struct vsi_v4l2_dev_info *hwinfo;

	v4l2_klog(LOGLVL_CONFIG, "%s", __func__);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	hwinfo = vsiv4l2_get_hwinfo();
	if (hwinfo->enc_corenum == 0)
		return -ENODEV;

	strlcpy(cap->driver, "vsi_v4l2", sizeof("vsi_v4l2"));
	strlcpy(cap->card, "vsi_v4l2enc", sizeof("vsi_v4l2enc"));
	strlcpy(cap->bus_info, "platform:vsi_v4l2enc", sizeof("platform:vsi_v4l2enc"));

	cap->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int vsi_enc_reqbufs(
	struct file *filp,
	void *priv,
	struct v4l2_requestbuffers *p)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);
	int ret;
	struct vb2_queue *q;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(p->type, ctx->flag))
		return -EINVAL;

	if (binputqueue(p->type))
		q = &ctx->input_que;
	else
		q = &ctx->output_que;
	ret = vb2_reqbufs(q, p);
	if (!binputqueue(p->type) && p->count == 0)
		set_bit(CTX_FLAG_ENC_FLUSHBUF, &ctx->flag);
	v4l2_klog(LOGLVL_BRIEF, "%lx:%s:%d ask for %d buffer, got %d:%d:%d",
		ctx->ctxid, __func__, p->type, p->count, q->num_buffers, ret, ctx->status);
	return ret;
}

static int vsi_enc_create_bufs(struct file *filp, void *priv,
				struct v4l2_create_buffers *create)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);
	int ret;
	struct vb2_queue *q;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(create->format.type, ctx->flag))
		return -EINVAL;

	if (binputqueue(create->format.type))
		q = &ctx->input_que;
	else
		q = &ctx->output_que;

	ret = vb2_create_bufs(q, create);

	if (!binputqueue(create->format.type) && create->count == 0)
		set_bit(CTX_FLAG_ENC_FLUSHBUF, &ctx->flag);
	v4l2_klog(LOGLVL_BRIEF, "%lx:%s:%d create for %d buffer, got %d:%d:%d\n",
		ctx->ctxid, __func__, create->format.type, create->count,
		q->num_buffers, ret, ctx->status);
	return ret;
}
///////////////////////

static void __dmaengine_callback_result(void *dma_async_param,
	                                 const struct dmaengine_result *result)
{
	struct vsi_v4l2_ctx *ctx = dma_async_param;
	ctx->dma_result = result->result;
	complete(&ctx->memcpy_done);
}
static int __do_dma_copy(struct vsi_v4l2_ctx *ctx, dma_addr_t dma_dst, dma_addr_t dma_src, int size)
{
	int ret = -1;
	struct dma_async_tx_descriptor *desc;
	struct dma_chan *chan;
	dma_cap_mask_t mask;
	dma_cookie_t cookie;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	chan = dma_request_channel(mask, NULL, NULL);
	if (!chan){
		printk(KERN_ERR "request dma channel failed");
		return -1;
	}

	if (ctx->comp_init == 0) {
        init_completion(&ctx->memcpy_done);
		ctx->comp_init = 1;
	}

	reinit_completion(&ctx->memcpy_done);

	// dma code
	desc = dmaengine_prep_dma_memcpy(chan, dma_dst, dma_src, size, 0);
	if (!desc){
		printk(KERN_ERR "get dma descriptor failed!\n");
		goto out_dma_mem_free;
	}

	desc->callback_result = __dmaengine_callback_result;
	desc->callback_param = ctx;

	cookie = dmaengine_submit(desc);
	if (dma_submit_error(cookie) != 0){
		printk(KERN_ERR "dms submit failed!\n");
		goto out_dma_mem_free;
	}

	dma_async_issue_pending(chan);

	if (!wait_for_completion_timeout(&ctx->memcpy_done, msecs_to_jiffies(5000))) {
		printk(KERN_ERR "DMA memcpy timeout");
		goto out_dma_mem_free;
	}

	if (ctx->dma_result != DMA_TRANS_NOERROR) {
		printk(KERN_ERR "DMA memcpy error %d\n", ctx->dma_result);
		goto out_dma_mem_free;
	}

	ret = 0;

out_dma_mem_free:
	// printk(KERN_ERR "DMA copy %llx to %llx result %d\n",dma_src, dma_dst dma_result);
	dma_release_channel(chan);

	return ret;
}

static int __vsi_do_dma_copy(struct vsi_v4l2_ctx *ctx, struct v4l2_streamparm *parm)
{
	__s32 exp_fd;
	__u32 exp_buf_idx;
	__u32 buf_idx;
	int size;

	struct dma_buf *dma_buf_s;
	struct vb2_dc_buf *dc_buf_s;

	struct vb2_queue *q;
	struct vb2_buffer *vb_buf_d;
	struct vb2_dc_buf *dc_buf_d;

	dma_addr_t dma_src;
	dma_addr_t dma_dst;

	int *data_ptr = (int*)&parm->parm.raw_data[100];

	exp_buf_idx = data_ptr[0];
	exp_fd = data_ptr[1];
	buf_idx = data_ptr[2];
	size = data_ptr[3];

	/* record the fd for release */
	if(!ctx->exp_fd[exp_buf_idx]){
		ctx->exp_fd[exp_buf_idx] = exp_fd;
	}

	/* get the source dma buffer */
	dma_buf_s = dma_buf_get(exp_fd);
	dc_buf_s = dma_buf_s->priv;

	/* get the target dma buffer */
	q = &ctx->input_que;
	vb_buf_d = q->bufs[buf_idx];
	dc_buf_d = vb_buf_d->planes[0].mem_priv;

	/* get the dma addr */
	dma_src = dc_buf_s->dma_addr;
	dma_dst = dc_buf_d->dma_addr;

	// printk(KERN_ERR "##idx %d addr %x",buf_idx, dc_buf_d->dma_addr);

	return __do_dma_copy(ctx, dma_dst, dma_src, size);
}
static int vsi_enc_s_parm(struct file *filp, void *priv, struct v4l2_streamparm *parm)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);

	v4l2_klog(LOGLVL_CONFIG, "%s", __func__);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(parm->type, ctx->flag))
		return -EINVAL;

	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;
	if (binputqueue(parm->type)) {
		//for dma copy
		if( parm->parm.raw_data[99] == 0xf1){

			if(__vsi_do_dma_copy(ctx, parm)){
				mutex_unlock(&ctx->ctxlock);
				return -1;
			}

			mutex_unlock(&ctx->ctxlock);
			return 0;
		}

		memset(parm->parm.output.reserved, 0, sizeof(parm->parm.output.reserved));
		if (!parm->parm.output.timeperframe.denominator)
			parm->parm.output.timeperframe.denominator = 25;		//default val
		if (!parm->parm.output.timeperframe.numerator)
			parm->parm.output.timeperframe.numerator = 1;			//default val
		if (ctx->mediacfg.m_encparams.m_framerate.inputRateDenom != parm->parm.output.timeperframe.numerator ||
			ctx->mediacfg.m_encparams.m_framerate.inputRateNumer != parm->parm.output.timeperframe.denominator) {
			ctx->mediacfg.m_encparams.m_framerate.inputRateDenom = parm->parm.output.timeperframe.numerator;
			ctx->mediacfg.m_encparams.m_framerate.inputRateNumer = parm->parm.output.timeperframe.denominator;
			flag_updateparam(m_framerate)
		}
		parm->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	} else {
		memset(parm->parm.capture.reserved, 0, sizeof(parm->parm.capture.reserved));
		if (!parm->parm.capture.timeperframe.denominator)
			parm->parm.capture.timeperframe.denominator = 25;
		if (!parm->parm.capture.timeperframe.numerator)
			parm->parm.capture.timeperframe.numerator = 1;
		if (ctx->mediacfg.m_encparams.m_framerate.outputRateDenom != parm->parm.capture.timeperframe.numerator ||
			ctx->mediacfg.m_encparams.m_framerate.outputRateNumer != parm->parm.capture.timeperframe.denominator) {
			ctx->mediacfg.m_encparams.m_framerate.outputRateDenom = parm->parm.capture.timeperframe.numerator;
			ctx->mediacfg.m_encparams.m_framerate.outputRateNumer = parm->parm.capture.timeperframe.denominator;
			flag_updateparam(m_framerate)
		}
		parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
 	}
	mutex_unlock(&ctx->ctxlock);
	return 0;
}

static int vsi_enc_g_parm(struct file *filp, void *priv, struct v4l2_streamparm *parm)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);
	struct v4l2_enccfg_framerate *pfr = &ctx->mediacfg.m_encparams.m_framerate;

	v4l2_klog(LOGLVL_CONFIG, "%s", __func__);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(parm->type, ctx->flag))
		return -EINVAL;
	if (binputqueue(parm->type)) {
		memset(parm->parm.output.reserved, 0, sizeof(parm->parm.output.reserved));
		parm->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
		parm->parm.output.timeperframe.denominator = pfr->inputRateNumer;
		parm->parm.output.timeperframe.numerator = pfr->inputRateDenom;
	} else {
		memset(parm->parm.capture.reserved, 0, sizeof(parm->parm.capture.reserved));
		parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		parm->parm.capture.timeperframe.denominator = pfr->outputRateNumer;
		parm->parm.capture.timeperframe.numerator = pfr->inputRateDenom;
	}
	return 0;
}

static int vsi_enc_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);

	v4l2_klog(LOGLVL_CONFIG, "%s:%d", __func__, f->type);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(f->type, ctx->flag))
		return -EINVAL;
	return vsiv4l2_getfmt(ctx, f);
}

static int vsi_enc_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	int ret;

	v4l2_klog(LOGLVL_CONFIG, "%s:%d:%d:%x",
		__func__, f->fmt.pix_mp.width, f->fmt.pix_mp.height, f->fmt.pix_mp.pixelformat);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(f->type, ctx->flag))
		return -EINVAL;
	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;
	if(f->fmt.raw_data[0] == 0xf1){
		ctx->dma_remap = f->fmt.raw_data[1];
		mutex_unlock(&ctx->ctxlock);
		return 0;
	}
	ret = vsiv4l2_setfmt(ctx, f);
	mutex_unlock(&ctx->ctxlock);
	return ret;
}

static int vsi_enc_querybuf(
	struct file *filp,
	void *priv,
	struct v4l2_buffer *buf)
{
	int ret;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);
	struct vb2_queue *q;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(buf->type, ctx->flag))
		return -EINVAL;
	if (binputqueue(buf->type))
		q = &ctx->input_que;
	else
		q = &ctx->output_que;
	v4l2_klog(LOGLVL_FLOW, "%s:%lx:%d:%d", __func__, ctx->flag, buf->type, buf->index);
	ret = vb2_querybuf(q, buf);
	if (buf->memory == V4L2_MEMORY_MMAP) {
		if (ret == 0 && q == &ctx->output_que) {
			buf->m.planes[0].m.mem_offset += OUTF_BASE;
			if (ctx->mediacfg.dstplanes == 2 && buf->length > 1)
				buf->m.planes[1].m.mem_offset += OUTF_BASE;
		}
	}

	return ret;
}

static int vsi_enc_trystartenc(struct vsi_v4l2_ctx *ctx)
{
	int ret = 0;

	v4l2_klog(LOGLVL_BRIEF, "%s:streaming:%d:%d, queued buf:%d:%d",
		__func__, ctx->input_que.streaming, ctx->output_que.streaming,
		ctx->input_que.queued_count, ctx->output_que.queued_count);
	if (vb2_is_streaming(&ctx->input_que) && vb2_is_streaming(&ctx->output_que)) {
		if ((ctx->status == VSI_STATUS_INIT ||
			ctx->status == ENC_STATUS_STOPPED ||
			ctx->status == ENC_STATUS_EOS) &&
			ctx->input_que.queued_count >= ctx->input_que.min_buffers_needed &&
			ctx->output_que.queued_count >= ctx->output_que.min_buffers_needed) {
			ret = vsiv4l2_execcmd(ctx, V4L2_DAEMON_VIDIOC_STREAMON, NULL);
			if (ret == 0) {
				ctx->status = ENC_STATUS_ENCODING;
				if (test_and_clear_bit(CTX_FLAG_PRE_DRAINING_BIT, &ctx->flag)) {
					ret |= vsiv4l2_execcmd(ctx, V4L2_DAEMON_VIDIOC_CMD_STOP, NULL);
					ctx->status = ENC_STATUS_DRAINING;
				}
			}
		}
	}
	return ret;
}

static int vsi_enc_qbuf(struct file *filp, void *priv, struct v4l2_buffer *buf)
{
	int ret;
	//struct vb2_queue *vq = vb->vb2_queue;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);
	struct video_device *vdev = ctx->dev->venc;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(buf->type, ctx->flag))
		return -EINVAL;

	//ignore input buf in spec's STOP state
	if (binputqueue(buf->type) &&
		(ctx->status == ENC_STATUS_STOPPED) &&
		!vb2_is_streaming(&ctx->input_que))
		return 0;

	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;

	if (!binputqueue(buf->type))
		ret = vb2_qbuf(&ctx->output_que, vdev->v4l2_dev->mdev, buf);
	else {
		if (test_and_clear_bit(CTX_FLAG_FORCEIDR_BIT, &ctx->flag))
			ctx->srcvbufflag[buf->index] |= FORCE_IDR;

		ret = vb2_qbuf(&ctx->input_que, vdev->v4l2_dev->mdev, buf);
	}
	v4l2_klog(LOGLVL_FLOW, "%lx:%s:%d:%d:%d, %d:%d, %d:%d",
		ctx->ctxid, __func__, buf->type, buf->index, buf->bytesused,
		buf->m.planes[0].bytesused, buf->m.planes[0].length,
		buf->m.planes[1].bytesused, buf->m.planes[1].length);
	if (ret == 0 && ctx->status != ENC_STATUS_ENCODING && ctx->status != ENC_STATUS_EOS)
		ret = vsi_enc_trystartenc(ctx);
	mutex_unlock(&ctx->ctxlock);
	return ret;
}

static int vsi_enc_streamon(struct file *filp, void *priv, enum v4l2_buf_type type)
{
	int ret = 0;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);

	v4l2_klog(LOGLVL_BRIEF, "%s:%d", __func__, type);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(type, ctx->flag))
		return -EINVAL;
	if (ctx->status == ENC_STATUS_ENCODING)
		return 0;

	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;
	if (!binputqueue(type)) {
		ret = vb2_streamon(&ctx->output_que, type);
		printbufinfo(&ctx->output_que);
	} else {
		ret = vb2_streamon(&ctx->input_que, type);
		printbufinfo(&ctx->input_que);
	}

	if (ret == 0) {
		if (ctx->status == ENC_STATUS_EOS) {
			//to avoid no queued buf when streamon
			ctx->status = ENC_STATUS_STOPPED;
		}
		ret = vsi_enc_trystartenc(ctx);
	}

	mutex_unlock(&ctx->ctxlock);
	return ret;
}

static int vsi_enc_streamoff(
	struct file *file,
	void *priv,
	enum v4l2_buf_type type)
{
	int i, ret;
	int binput = binputqueue(type);
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *q;

	v4l2_klog(LOGLVL_BRIEF, "%s:%d", __func__, type);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(type, ctx->flag))
		return -EINVAL;
	if (ctx->status == VSI_STATUS_INIT)
		return 0;

	if (binputqueue(type))
		q = &ctx->input_que;
	else
		q = &ctx->output_que;

	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;
	if (binput)
		vsiv4l2_execcmd(ctx, V4L2_DAEMON_VIDIOC_STREAMOFF_OUTPUT, &binput);
	else
		vsiv4l2_execcmd(ctx, V4L2_DAEMON_VIDIOC_STREAMOFF_CAPTURE, &binput);
	mutex_unlock(&ctx->ctxlock);
	if (binput)
		ret = wait_event_interruptible(ctx->capoffdone_queue, vsi_checkctx_outputoffdone(ctx));
	else
		ret = wait_event_interruptible(ctx->capoffdone_queue, vsi_checkctx_capoffdone(ctx));
	if (ret != 0)
		v4l2_klog(LOGLVL_WARNING, "%lx binput:%d, enc wait strmoff done fail\n", ctx->ctxid, binput);

	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;
	ctx->status = ENC_STATUS_STOPPED;
	if (binput) {
		clear_bit(CTX_FLAG_FORCEIDR_BIT, &ctx->flag);
		for (i = 0; i < VIDEO_MAX_FRAME; i++)
			ctx->srcvbufflag[i] = 0;

		for (i = 0; i < VIDEO_MAX_FRAME; i++){
			dma_export_buf_release(ctx->exp_fd[i]);
			ctx->exp_fd[i] = 0;
		}
	}

	return_all_buffers(q, VB2_BUF_STATE_DONE, 1);
	ret = vb2_streamoff(q, type);
	mutex_unlock(&ctx->ctxlock);
	return ret;
}

static int vsi_enc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	int ret = 0;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	struct vb2_queue *q;
	struct vb2_buffer *vb;
	struct vsi_vpu_buf *vsibuf;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(p->type, ctx->flag))
		return -EINVAL;
	if (binputqueue(p->type))
		q = &ctx->input_que;
	else
		q = &ctx->output_que;

	if (ctx->status == ENC_STATUS_STOPPED ||
		ctx->status == ENC_STATUS_EOS) {
		p->bytesused = 0;
		return -EPIPE;
	}

	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;
	ret = vb2_dqbuf(q, p, file->f_flags & O_NONBLOCK);
	if (ret == 0) {
		vb = q->bufs[p->index];
		vsibuf = vb_to_vsibuf(vb);
		list_del(&vsibuf->list);
		p->flags &= ~(V4L2_BUF_FLAG_KEYFRAME | V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME);
		if (!binputqueue(p->type)) {
			if (ctx->vbufflag[p->index] & FRAMETYPE_I)
				p->flags |= V4L2_BUF_FLAG_KEYFRAME;
			else  if (ctx->vbufflag[p->index] & FRAMETYPE_P)
				p->flags |= V4L2_BUF_FLAG_PFRAME;
			else  if (ctx->vbufflag[p->index] & FRAMETYPE_B)
				p->flags |= V4L2_BUF_FLAG_BFRAME;
		}
	}
	if (!binputqueue(p->type)) {
		if (ret == 0) {
			if (ctx->vbufflag[p->index] & LAST_BUFFER_FLAG) {
				vsi_v4l2_sendeos(ctx);
				if (ctx->status == ENC_STATUS_DRAINING)
					ctx->status = ENC_STATUS_EOS;
				v4l2_klog(LOGLVL_BRIEF, "dqbuf get eos flag");
			}
		}
	}
	mutex_unlock(&ctx->ctxlock);
	v4l2_klog(LOGLVL_FLOW, "%s:%d:%d:%d:%x:%d", __func__, p->type, p->index, ret, p->flags, ctx->status);
	return ret;
}

static int vsi_enc_prepare_buf(
	struct file *file,
	void *priv,
	struct v4l2_buffer *p)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	struct vb2_queue *q;
	struct video_device *vdev = ctx->dev->venc;

	v4l2_klog(LOGLVL_FLOW, "%s:%d", __func__, p->type);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(p->type, ctx->flag))
		return -EINVAL;
	if (binputqueue(p->type))
		q = &ctx->input_que;
	else
		q = &ctx->output_que;
	return vb2_prepare_buf(q, vdev->v4l2_dev->mdev, p);
}

static int vsi_enc_expbuf(
	struct file *file,
	void *priv,
	struct v4l2_exportbuffer *p)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	struct vb2_queue *q;

	v4l2_klog(LOGLVL_FLOW, "%s:%d", __func__, p->type);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(p->type, ctx->flag))
		return -EINVAL;

	if (binputqueue(p->type))
		q = &ctx->input_que;
	else
		q = &ctx->output_que;
	return vb2_expbuf(q, p);
}

static int vsi_enc_try_fmt(struct file *file, void *prv, struct v4l2_format *f)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);

	v4l2_klog(LOGLVL_CONFIG, "%s:%d", __func__, f->type);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(f->type, ctx->flag))
		return -EINVAL;
	if (vsi_find_format(ctx, f) == NULL)
		return -EINVAL;

	return 0;
}

static int vsi_enc_enum_fmt(struct file *file, void *prv, struct v4l2_fmtdesc *f)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	struct vsi_video_fmt *pfmt;
	int braw = brawfmt(ctx->flag, f->type);

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (!isvalidtype(f->type, ctx->flag))
		return -EINVAL;

	pfmt = vsi_enum_encformat(f->index, braw);
	if (pfmt == NULL)
		return -EINVAL;

	if (pfmt->name && strlen(pfmt->name))
		strlcpy(f->description, pfmt->name, strlen(pfmt->name) + 1);
	f->pixelformat = pfmt->fourcc;
	f->flags = pfmt->flag;
	v4l2_klog(LOGLVL_CONFIG, "%s:%d:%d:%x", __func__, f->index, f->type, pfmt->fourcc);
	return 0;
}

static int vsi_enc_set_selection(struct file *file, void *prv, struct v4l2_selection *s)
{
	int ret = 0;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	struct vsi_v4l2_encparams *pcfg = &ctx->mediacfg.m_encparams;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
		s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;
	if (s->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;
	ret = vsiv4l2_verifycrop(ctx, s);
	if (!ret) {
		if (mutex_lock_interruptible(&ctx->ctxlock))
			return -EBUSY;
		pcfg->m_cropinfo.horOffsetSrc = s->r.left;
		pcfg->m_cropinfo.verOffsetSrc = s->r.top;
		pcfg->m_cropinfo.width = s->r.width;
		pcfg->m_cropinfo.height = s->r.height;
 		flag_updateparam(m_cropinfo)
 		mutex_unlock(&ctx->ctxlock);
	}
	v4l2_klog(LOGLVL_CONFIG, "%lx:%s:%d,%d,%d,%d",
		ctx->ctxid, __func__, s->r.left, s->r.top, s->r.width, s->r.height);

	return ret;
}

static int vsi_enc_get_selection(struct file *file, void *prv, struct v4l2_selection *s)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	struct vsi_v4l2_encparams *pcfg = &ctx->mediacfg.m_encparams;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
		s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;
	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
		s->r.left = pcfg->m_cropinfo.horOffsetSrc;
		s->r.top = pcfg->m_cropinfo.verOffsetSrc;
		s->r.width = pcfg->m_cropinfo.width;
		s->r.height = pcfg->m_cropinfo.height;
		break;
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = pcfg->m_srcinfo.lumWidthSrc;
		s->r.height = pcfg->m_srcinfo.lumHeightSrc;
		break;
	default:
		return -EINVAL;
	}
	v4l2_klog(LOGLVL_CONFIG, "%lx:%s:%d,%d,%d,%d",
		ctx->ctxid, __func__, s->r.left, s->r.top, s->r.width, s->r.height);

	return 0;
}

static int vsi_enc_subscribe_event(
	struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;

	v4l2_klog(LOGLVL_CONFIG, "%s:%d", __func__, sub->type);
	switch (sub->type) {
	case V4L2_EVENT_CTRL:
		return v4l2_ctrl_subscribe_event(fh, sub);
	case V4L2_EVENT_SKIP:
		return v4l2_event_subscribe(fh, sub, 16, NULL);
	case V4L2_EVENT_EOS:
	case V4L2_EVENT_CODEC_ERROR:
	case V4L2_EVENT_INVALID_OPTION:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
//by spec should return error, but current gst will fail, so leave it here
		return 0;//-EINVAL;
	}
}

static int vsi_enc_try_encoder_cmd(struct file *file, void *fh, struct v4l2_encoder_cmd *cmd)
{
	switch (cmd->cmd) {
	case V4L2_ENC_CMD_STOP:
	case V4L2_ENC_CMD_START:
	case V4L2_ENC_CMD_PAUSE:
	case V4L2_ENC_CMD_RESUME:
		cmd->flags = 0;
		return 0;
	default:
		return -EINVAL;
	}
}

static int vsi_enc_encoder_cmd(struct file *file, void *fh, struct v4l2_encoder_cmd *cmd)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	int ret = 0;

	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (mutex_lock_interruptible(&ctx->ctxlock))
		return -EBUSY;
	v4l2_klog(LOGLVL_BRIEF, "%s:%d:%d", __func__, ctx->status, cmd->cmd);
	switch (cmd->cmd) {
	case V4L2_ENC_CMD_STOP:
		set_bit(CTX_FLAG_PRE_DRAINING_BIT, &ctx->flag);
		if (ctx->status == ENC_STATUS_ENCODING) {
			ret = vsiv4l2_execcmd(ctx, V4L2_DAEMON_VIDIOC_CMD_STOP, cmd);
			if (ret == 0) {
				ctx->status = ENC_STATUS_DRAINING;
				clear_bit(CTX_FLAG_PRE_DRAINING_BIT, &ctx->flag);
			}
		}
		break;
	case V4L2_ENC_CMD_START:
		if (ctx->status == ENC_STATUS_STOPPED ||
			ctx->status == ENC_STATUS_EOS) {
			vb2_streamon(&ctx->input_que, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
			vb2_streamon(&ctx->output_que, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
			ret = vsi_enc_trystartenc(ctx);
		}
		break;
	case V4L2_ENC_CMD_PAUSE:
	case V4L2_ENC_CMD_RESUME:
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&ctx->ctxlock);
	return ret;
}

static int vsi_enc_encoder_enum_framesizes(struct file *file, void *priv,
				  struct v4l2_frmsizeenum *fsize)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_format fmt;

	v4l2_klog(LOGLVL_CONFIG, "%s:%x", __func__, fsize->pixel_format);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	if (fsize->index != 0)		//only stepwise
		return -EINVAL;

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.pixelformat = fsize->pixel_format;
	if (vsi_find_format(ctx,  &fmt) != NULL)
		vsi_enum_encfsize(fsize, ctx->mediacfg.outfmt_fourcc);
	else {
		fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		fmt.fmt.pix_mp.pixelformat = fsize->pixel_format;
		if (vsi_find_format(ctx,  &fmt) == NULL)
			return -EINVAL;
		vsi_enum_encfsize(fsize, fsize->pixel_format);
	}

	return 0;
}

/* ioctl handler */
/* take VIDIOC_S_INPUT for example, ioctl goes to V4l2-ioctl.c.: v4l_s_input() -> V4l2-dev.c: v4l2_ioctl_ops.vidioc_s_input() */
/* ioctl cmd could be disabled by v4l2_disable_ioctl() */
static const struct v4l2_ioctl_ops vsi_enc_ioctl = {
	.vidioc_querycap = vsi_enc_querycap,
	.vidioc_reqbufs             = vsi_enc_reqbufs,
	.vidioc_create_bufs         = vsi_enc_create_bufs,
	.vidioc_prepare_buf         = vsi_enc_prepare_buf,
	//create_buf can be provided now since we don't know buf type in param
	.vidioc_querybuf            = vsi_enc_querybuf,
	.vidioc_qbuf                = vsi_enc_qbuf,
	.vidioc_dqbuf               = vsi_enc_dqbuf,
	.vidioc_streamon        = vsi_enc_streamon,
	.vidioc_streamoff       = vsi_enc_streamoff,
	.vidioc_s_parm		= vsi_enc_s_parm,
	.vidioc_g_parm		= vsi_enc_g_parm,
	//.vidioc_g_fmt_vid_cap = vsi_enc_g_fmt,
	.vidioc_g_fmt_vid_cap_mplane = vsi_enc_g_fmt,
	//.vidioc_s_fmt_vid_cap = vsi_enc_s_fmt,
	.vidioc_s_fmt_vid_cap_mplane = vsi_enc_s_fmt,
	.vidioc_expbuf = vsi_enc_expbuf,		//this is used to export MMAP ptr as prime fd to user space app

	//.vidioc_g_fmt_vid_out = vsi_enc_g_fmt,
	.vidioc_g_fmt_vid_out_mplane = vsi_enc_g_fmt,
	//.vidioc_s_fmt_vid_out = vsi_enc_s_fmt,
	.vidioc_s_fmt_vid_out_mplane = vsi_enc_s_fmt,
	//.vidioc_try_fmt_vid_cap = vsi_enc_try_fmt,
	.vidioc_try_fmt_vid_cap_mplane = vsi_enc_try_fmt,
	//.vidioc_try_fmt_vid_out = vsi_enc_try_fmt,
	.vidioc_try_fmt_vid_out_mplane = vsi_enc_try_fmt,
	.vidioc_enum_fmt_vid_cap = vsi_enc_enum_fmt,
	.vidioc_enum_fmt_vid_out = vsi_enc_enum_fmt,

	//.vidioc_g_fmt_vid_out = vsi_enc_g_fmt,
	.vidioc_g_fmt_vid_out_mplane = vsi_enc_g_fmt,

	.vidioc_s_selection = vsi_enc_set_selection,		//VIDIOC_S_SELECTION, VIDIOC_S_CROP
	.vidioc_g_selection = vsi_enc_get_selection,		//VIDIOC_G_SELECTION, VIDIOC_G_CROP

	.vidioc_subscribe_event = vsi_enc_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_try_encoder_cmd = vsi_enc_try_encoder_cmd,

	//fixme: encoder cmd stop will make streamoff not coming from ffmpeg. Maybe this is the right way to get finished, check later
	.vidioc_encoder_cmd = vsi_enc_encoder_cmd,
	.vidioc_enum_framesizes = vsi_enc_encoder_enum_framesizes,
};

/*setup buffer information before real allocation*/
static int vsi_enc_queue_setup(
	struct vb2_queue *vq,
	unsigned int *nbuffers,
	unsigned int *nplanes,
	unsigned int sizes[],
	struct device *alloc_devs[])
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(vq->drv_priv);
	int i, ret;

	v4l2_klog(LOGLVL_CONFIG, "%lx:%s:%d,%d,%d", ctx->ctxid, __func__, *nbuffers, *nplanes, sizes[0]);
	ret = vsiv4l2_buffer_config(ctx, vq->type, nbuffers, nplanes, sizes);
	if (ret == 0) {
		for (i = 0; i < *nplanes; i++)
			alloc_devs[i] = ctx->dev->dev;
	}
	return ret;
}

static void vsi_enc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(vq->drv_priv);
	struct vsi_vpu_buf *vsibuf;
	int ret;

	v4l2_klog(LOGLVL_FLOW, "%s:%d:%d", __func__, vb->type, vb->index);
	vsibuf = vb_to_vsibuf(vb);
	if (!binputqueue(vq->type))
		list_add_tail(&vsibuf->list, &ctx->output_list);
	else
		list_add_tail(&vsibuf->list, &ctx->input_list);
	ret = vsiv4l2_execcmd(ctx, V4L2_DAEMON_VIDIOC_BUF_RDY, vb);
}

static int vsi_enc_buf_init(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(vq->drv_priv);
	struct vb2_dc_buf *dc_buf;

	dc_buf = vb->planes[0].mem_priv;
	dc_buf->remap = ctx->dma_remap;

	return 0;
}

static int vsi_enc_buf_prepare(struct vb2_buffer *vb)
{
	/*any valid init operation on buffer vb*/
	/*gspca and rockchip both check buffer size here*/
	//like vb2_set_plane_payload(vb, 0, 1920*1080);
	return 0;
}

static int vsi_enc_start_streaming(struct vb2_queue *q, unsigned int count)
{
	return 0;
}
static void vsi_enc_stop_streaming(struct vb2_queue *vq)
{
}

static void vsi_enc_buf_finish(struct vb2_buffer *vb)
{
}

static void vsi_enc_buf_cleanup(struct vb2_buffer *vb)
{
}

static void vsi_enc_buf_wait_finish(struct vb2_queue *vq)
{
	vb2_ops_wait_finish(vq);
}

static void vsi_enc_buf_wait_prepare(struct vb2_queue *vq)
{
	vb2_ops_wait_prepare(vq);
}

static struct vb2_ops vsi_enc_qops = {
	.queue_setup = vsi_enc_queue_setup,
	.wait_prepare = vsi_enc_buf_wait_prepare,	/*these two are just mutex protection for done_que*/
	.wait_finish = vsi_enc_buf_wait_finish,
	.buf_init = vsi_enc_buf_init,
	.buf_prepare = vsi_enc_buf_prepare,
	.buf_finish = vsi_enc_buf_finish,
	.buf_cleanup = vsi_enc_buf_cleanup,
	.start_streaming = vsi_enc_start_streaming,
	.stop_streaming = vsi_enc_stop_streaming,
	.buf_queue = vsi_enc_buf_queue,
	//fill_user_buffer
	//int (*buf_out_validate)(struct vb2_buffer *vb);
	//void (*buf_request_complete)(struct vb2_buffer *vb);
};

static void vsi_enc_updateV4l2Aspect(struct vsi_v4l2_ctx *ctx, int id, int param)
{
	struct v4l2_enccfg_aspect *pasp = &ctx->mediacfg.m_encparams.m_aspect;
	u32 width = pasp->sarWidth;
	u32 height = pasp->sarHeight;

	switch (id) {
	case V4L2_CID_MPEG_VIDEO_ASPECT:
		if (param == V4L2_MPEG_VIDEO_ASPECT_1x1) {
			width = 1; height = 1;
		} else if (param == V4L2_MPEG_VIDEO_ASPECT_4x3) {
			width = 4; height = 3;
		} else if (param == V4L2_MPEG_VIDEO_ASPECT_16x9) {
			width = 16; height = 9;
		} else if (param == V4L2_MPEG_VIDEO_ASPECT_221x100) {
			width = 221; height = 100;
		};
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH:
		width = param;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT:
		height = param;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC:
		{
			switch (param) {
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_1x1:
				width = 1; height = 1;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_12x11:
				width = 12; height = 11;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_10x11:
				width = 10; height = 11;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_16x11:
				width = 1; height = 1;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_40x33:
				width = 40; height = 33;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_24x11:
				width = 24; height = 11;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_20x11:
				width = 20; height = 11;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_32x11:
				width = 30; height = 11;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_80x33:
				width = 80; height = 33;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_18x11:
				width = 18; height = 11;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_15x11:
				width = 15; height = 11;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_64x33:
				width = 64; height = 33;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_160x99:
				width = 160; height = 99;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_4x3:
				width = 4; height = 3;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_3x2:
				width = 3; height = 2;
				break;
			case V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_2x1:
				width = 2; height = 1;
				break;
			default:
				break;
			}
		}
	default:
		break;
	}
	if (width != pasp->sarWidth || height != pasp->sarHeight) {
		pasp->sarHeight = height;
		pasp->sarWidth = width;
		flag_updateparam(m_aspect)
	}
}

static s32 vsi_enc_convertV4l2Quality(s32 v4l2val)
{
	if (v4l2val < 0)
		return v4l2val;
	if (v4l2val >= 100)
		return 0;
	else
		return (101 - v4l2val) /2 + 1;	//ours is from 0-51
}

static int vsi_v4l2_enc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret;
	s32 val, val2;
	u64 temp64;
	struct vsi_v4l2_ctx *ctx = ctrl_to_ctx(ctrl);
	struct vsi_v4l2_encparams *pencparm = &ctx->mediacfg.m_encparams;

	v4l2_klog(LOGLVL_CONFIG, "%s:%x=%d", __func__, ctrl->id, ctrl->val);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_B_FRAMES:
		pencparm->m_gopsize.gopSize = ctrl->val + 1;
		flag_checkparam(m_gopsize)
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		if (pencparm->m_intrapicrate.intraPicRate != ctrl->val) {
			pencparm->m_intrapicrate.intraPicRate = ctrl->val;
			flag_updateparam(m_intrapicrate)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_PROFILE:
	case V4L2_CID_MPEG_VIDEO_VP9_PROFILE:
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
	case V4L2_CID_MPEG_VIDEO_HEVC_PROFILE:
		ret = vsi_set_profile(ctx, ctrl->id, ctrl->val);
		//flag_updateparam(m_profile) is left to get_fmtprofile
		return ret;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		if (pencparm->m_bitrate.bitPerSecond != ctrl->val) {
			pencparm->m_bitrate.bitPerSecond = ctrl->val;
			flag_updateparam(m_bitrate)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
	case V4L2_CID_MPEG_VIDEO_HEVC_LEVEL:
	case V4L2_CID_MPEG_VIDEO_VP9_LEVEL:
		ret = vsi_set_Level(ctx, ctrl->id, ctrl->val);
		if (ret == 0) {
			flag_checkparam(m_level)
		}
		return ret;
	case V4L2_CID_MPEG_VIDEO_VPX_MAX_QP:
		ctx->mediacfg.qpMax_vpx = ctrl->val;
		flag_checkparam(m_qprange)
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
		ctx->mediacfg.qpMax_h26x = ctrl->val;
		flag_checkparam(m_qprange)
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_MIN_QP:
		ctx->mediacfg.qpMin_vpx = ctrl->val;
		flag_checkparam(m_qprange)
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
		ctx->mediacfg.qpMin_h26x = ctrl->val;
		flag_checkparam(m_qprange)
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		val = pencparm->m_bitratemode.hrdConformance;
		val2 = pencparm->m_bitratemode.tolMovingBitRate;
		if (ctrl->val == V4L2_MPEG_VIDEO_BITRATE_MODE_VBR)
			pencparm->m_bitratemode.hrdConformance = 0;
		else {
			pencparm->m_bitratemode.hrdConformance = 1;
			pencparm->m_bitratemode.tolMovingBitRate = -1;
		}
		if (val != pencparm->m_bitratemode.hrdConformance ||
			val2 != pencparm->m_bitratemode.tolMovingBitRate)
			flag_updateparam(m_bitratemode)
		if (pencparm->m_bitratemode.bitrateMode != ctrl->val) {
			pencparm->m_bitratemode.bitrateMode = ctrl->val;
			flag_updateparam(m_bitratemode)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME:
		set_bit(CTX_FLAG_FORCEIDR_BIT, &ctx->flag);
		break;
	case V4L2_CID_MPEG_VIDEO_HEADER_MODE:
		pencparm->m_headermode.headermode = ctrl->val;
		flag_updateparam(m_headermode)
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE:
		pencparm->m_sliceinfo.multislice_mode = ctrl->val;
		flag_checkparam(m_sliceinfo)
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB:
		pencparm->m_sliceinfo.sliceSize = ctrl->val;
		flag_checkparam(m_sliceinfo)
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		if (pencparm->m_rcmode.picRc != ctrl->val) {
			pencparm->m_rcmode.picRc = ctrl->val;
			flag_updateparam(m_rcmode)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE:
		if (pencparm->m_rcmode.ctbRc != ctrl->val) {
			pencparm->m_rcmode.ctbRc = ctrl->val;
			flag_updateparam(m_rcmode)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP:
		ctx->mediacfg.qpHdrI_h26x = ctrl->val;
		flag_checkparam(m_qphdrip)
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_I_FRAME_QP:
		ctx->mediacfg.qpHdrI_vpx = ctrl->val;
		flag_checkparam(m_qphdrip)
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP:
		ctx->mediacfg.qpHdrP_h26x = ctrl->val;
		flag_checkparam(m_qphdrip)
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_P_FRAME_QP:
		ctx->mediacfg.qpHdrP_vpx = ctrl->val;
		flag_checkparam(m_qphdrip)
		break;
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP:
		if (pencparm->m_qphdrip.qpHdrB != ctrl->val) {
			pencparm->m_qphdrip.qpHdrB = ctrl->val;
			flag_updateparam(m_qphdrip)
		}
		break;
	case V4L2_CID_ROTATE:
		if (ctrl->val != pencparm->m_rotation.rotation) {
			pencparm->m_rotation.rotation = ctrl->val;
			flag_updateparam(m_rotation)
		}
		break;
	case V4L2_CID_ROI:
		if (pencparm->m_gdrduration.gdrDuration > 0)
			return -EINVAL;
		if (ctrl->p_new.p)
			vsiv4l2_setROI(ctx, ctrl->p_new.p);
		break;
	case V4L2_CID_IPCM:
		if (pencparm->m_gdrduration.gdrDuration > 0)
			return -EINVAL;
		if (ctrl->p_new.p)
			vsiv4l2_setIPCM(ctx, ctrl->p_new.p);
		break;
	case V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER:
		if (pencparm->m_idrhdr.idrHdr != ctrl->val) {
			pencparm->m_idrhdr.idrHdr = ctrl->val;
			flag_updateparam(m_idrhdr)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_PEAK:
		if(pencparm->m_bitratemode.hrdConformance == 1) {
			v4l2_klog(LOGLVL_BRIEF, "CBR,can not set peak\n");
			return -EINVAL;
		}
		if (pencparm->m_bitrate.bitPerSecond <= 0 || pencparm->m_bitrate.bitPerSecond > ctrl->val)
			return -EINVAL;
		temp64 = (u64)(ctrl->val - pencparm->m_bitrate.bitPerSecond);
		temp64 = (temp64 * 100ul);
		temp64 = div_u64(temp64, pencparm->m_bitrate.bitPerSecond);
		if (pencparm->m_bitratemode.tolMovingBitRate != (s32)temp64) {
			pencparm->m_bitratemode.tolMovingBitRate = (s32)temp64;
			flag_updateparam(m_bitratemode)
		}
		break;
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		val = vsi_enc_convertV4l2Quality(ctrl->val);
		if (pencparm->m_jpgfixqp.fixedQP != val) {
			pencparm->m_jpgfixqp.fixedQP = val;
			flag_updateparam(m_jpgfixqp)
		}
		break;
	case V4L2_CID_JPEG_RESTART_INTERVAL:
		if (pencparm->m_restartinterval.restartInterval != ctrl->val) {
			pencparm->m_restartinterval.restartInterval = ctrl->val;
			flag_updateparam(m_restartinterval)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD:
		if (pencparm->m_gdrduration.gdrDuration != ctrl->val) {
			pencparm->m_gdrduration.gdrDuration = ctrl->val;
			flag_updateparam(m_gdrduration)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_PERIOD:	//wait for ctrlSW to realize the API
		if (pencparm->m_refresh.idr_interval != ctrl->val) {
			pencparm->m_refresh.idr_interval = ctrl->val;
			flag_updateparam(m_refresh)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE:
		if (pencparm->m_entropymode.enableCabac != ctrl->val) {
			pencparm->m_entropymode.enableCabac = ctrl->val;
			flag_updateparam(m_entropymode)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE:
		if (pencparm->m_cpbsize.cpbSize != ctrl->val) {
			pencparm->m_cpbsize.cpbSize = ctrl->val;
			flag_updateparam(m_cpbsize)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM:
		if (pencparm->m_trans8x8.transform8x8Enable != ctrl->val) {
			pencparm->m_trans8x8.transform8x8Enable = ctrl->val;
			flag_updateparam(m_trans8x8)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_CONSTANT_QUALITY:
		pencparm->m_rcmode.picRc =
			pencparm->m_rcmode.ctbRc = -1;
		val = vsi_enc_convertV4l2Quality(ctrl->val);
		if (pencparm->m_qphdr.qpHdr != val) {
			pencparm->m_qphdr.qpHdr = val;
			flag_updateparam(m_qphdr)
		}
		flag_updateparam(m_rcmode)
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CONSTRAINED_INTRA_PREDICTION:
		if (pencparm->m_ctrintrapred.constrained_intra_pred_flag != ctrl->val) {
			pencparm->m_ctrintrapred.constrained_intra_pred_flag = ctrl->val;
			flag_updateparam(m_ctrintrapred)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE:
		if (pencparm->m_aspect.enable != ctrl->val) {
			pencparm->m_aspect.enable = ctrl->val;
			flag_updateparam(m_aspect)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_ASPECT:
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC:
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT:
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH:
		vsi_enc_updateV4l2Aspect(ctx, ctrl->id, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE:
		pencparm->m_skipmode.pictureSkip = ctrl->val - 1;
		flag_updateparam(m_skipmode)
		break;
	case V4L2_CID_MPEG_VIDEO_AU_DELIMITER:
		if (pencparm->m_aud.sendAud != ctrl->val) {
			pencparm->m_aud.sendAud = ctrl->val;
			flag_updateparam(m_aud)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE:
		ctx->mediacfg.disableDeblockingFilter_h264 = ctrl->val;
		flag_checkparam(m_loopfilter)
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE:
		ctx->mediacfg.disableDeblockingFilter_hevc = ctrl->val ? 0 : 1;
		flag_checkparam(m_loopfilter)
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA:
	case V4L2_CID_MPEG_VIDEO_HEVC_LF_TC_OFFSET_DIV2:
		if (pencparm->m_loopfilter.tc_Offset != ctrl->val) {
			pencparm->m_loopfilter.tc_Offset = ctrl->val;
			flag_updateparam(m_loopfilter)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA:
	case V4L2_CID_MPEG_VIDEO_HEVC_LF_BETA_OFFSET_DIV2:
		if (pencparm->m_loopfilter.beta_Offset != ctrl->val) {
			pencparm->m_loopfilter.beta_Offset = ctrl->val;
			flag_updateparam(m_loopfilter)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CHROMA_QP_INDEX_OFFSET:
		if (pencparm->m_chromaqpoffset.chroma_qp_offset != ctrl->val) {
			pencparm->m_chromaqpoffset.chroma_qp_offset = ctrl->val;
			flag_updateparam(m_chromaqpoffset)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MIN_QP:
		if (pencparm->m_iqprange.qpMinI != ctrl->val) {
			pencparm->m_iqprange.qpMinI = ctrl->val;
			flag_updateparam(m_iqprange)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MAX_QP:
		if (pencparm->m_iqprange.qpMaxI != ctrl->val) {
			pencparm->m_iqprange.qpMaxI = ctrl->val;
			flag_updateparam(m_iqprange)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MIN_QP:
		if (pencparm->m_pqprange.qpMinPB != ctrl->val) {
			pencparm->m_pqprange.qpMinPB = ctrl->val;
			flag_updateparam(m_pqprange)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MAX_QP:
		if (pencparm->m_pqprange.qpMaxPB != ctrl->val) {
			pencparm->m_pqprange.qpMaxPB = ctrl->val;
			flag_updateparam(m_pqprange)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_MV_V_SEARCH_RANGE:
		pencparm->m_mvrange.meVertSearchRange = ctrl->val;
		flag_checkparam(m_mvrange)
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_NUM_PARTITIONS:
		if (pencparm->m_vpxpartitions.dctPartitions != ctrl->val) {
			pencparm->m_vpxpartitions.dctPartitions = ctrl->val;
			flag_updateparam(m_vpxpartitions)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_NUM_REF_FRAMES:
		ctx->mediacfg.vpx_PRefN = ctrl->val + 1;
		flag_checkparam(m_refno)
		break;
	case V4L2_CID_MPEG_VIDEO_REF_NUMBER_FOR_PFRAMES:
		ctx->mediacfg.hevc_PRefN = ctrl->val;
		flag_checkparam(m_refno)
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_FILTER_LEVEL:
		pencparm->m_vpxfilterlvl.filterLevel = ctrl->val;
		flag_checkparam(m_vpxfilterlvl)
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_FILTER_SHARPNESS:
		pencparm->m_vpxfiltersharp.filterSharpness = ctrl->val;
		flag_checkparam(m_vpxfiltersharp)
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_GOLDEN_FRAME_REF_PERIOD:
		pencparm->m_goldenperiod.goldenPictureRate = ctrl->val;
		flag_checkparam(m_goldenperiod)
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_TIER:
		if (pencparm->m_tier.tier != ctrl->val) {
			pencparm->m_tier.tier = ctrl->val;
			flag_updateparam(m_tier)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_TYPE:
		if (pencparm->m_refresh.refreshtype != ctrl->val) {
			pencparm->m_refresh.refreshtype = ctrl->val;
			flag_updateparam(m_refresh)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_PERIOD:
		if (pencparm->m_intrapicrate.intraPicRate != ctrl->val) {
			pencparm->m_intrapicrate.intraPicRate = ctrl->val;
			flag_updateparam(m_intrapicrate)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_TEMPORAL_ID:
		if (pencparm->m_temporalid.temporalId != ctrl->val) {
			pencparm->m_temporalid.temporalId = ctrl->val;
			flag_updateparam(m_temporalid)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_STRONG_SMOOTHING:
		if (pencparm->m_strongsmooth.strong_intra_smoothing_enabled_flag != ctrl->val) {
			pencparm->m_strongsmooth.strong_intra_smoothing_enabled_flag = ctrl->val;
			flag_updateparam(m_strongsmooth)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_TMV_PREDICTION:
		if (pencparm->m_tmvp.enableTMVP != ctrl->val) {
			pencparm->m_tmvp.enableTMVP = ctrl->val;
			flag_updateparam(m_tmvp)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_WITHOUT_STARTCODE:
		if (pencparm->m_startcode.streamType != ctrl->val) {
			pencparm->m_startcode.streamType = ctrl->val;
			flag_updateparam(m_startcode)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR:
		if (pencparm->m_resendSPSPPS.resendSPSPPS != ctrl->val) {
			pencparm->m_resendSPSPPS.resendSPSPPS = ctrl->val;
			flag_updateparam(m_resendSPSPPS)
		}
		break;
	case V4L2_CID_JPEG_CHROMA_SUBSAMPLING:
		if (!vsiv4l2_has_jpgcodingmode(ctrl->val))
			break;
		if (pencparm->m_jpgcodingmode.codingMode != ctrl->val) {
			pencparm->m_jpgcodingmode.codingMode = ctrl->val;
			flag_updateparam(m_jpgcodingmode)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING:
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_QP:
		if (pencparm->m_gopcfg.hierachy_enable != ctrl->val) {
			pencparm->m_gopcfg.hierachy_enable = ctrl->val;
			flag_updateparam(m_gopcfg)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE:
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_TYPE:
		if (pencparm->m_gopcfg.hierachy_codingtype != ctrl->val) {
			pencparm->m_gopcfg.hierachy_codingtype = ctrl->val;
			flag_updateparam(m_gopcfg)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER:
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_LAYER:
		if (pencparm->m_gopcfg.codinglayer != ctrl->val) {
			pencparm->m_gopcfg.codinglayer = ctrl->val;
			flag_updateparam(m_gopcfg)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP:
		val = vsiv4l2_verify_h264_codinglayerqp(ctrl->val);
		if (val != -1) {
			u32 layern = (u32)ctrl->val >> 16;
			if (val != pencparm->m_gopcfg.codinglayerqp[layern]) {
				pencparm->m_gopcfg.codinglayerqp[layern] = val;
				flag_updateparam(m_gopcfg)
			}
			break;
		} else
			return -EINVAL;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L0_QP:
		if (vsiv4l2_get_maxhevclayern() >= 0) {
			pencparm->m_gopcfg.codinglayerqp[0] = ctrl->val;
			flag_updateparam(m_gopcfg)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L1_QP:
		if (vsiv4l2_get_maxhevclayern() >= 1) {
			pencparm->m_gopcfg.codinglayerqp[1] = ctrl->val;
			flag_updateparam(m_gopcfg)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L2_QP:
		if (vsiv4l2_get_maxhevclayern() >= 2) {
			pencparm->m_gopcfg.codinglayerqp[2] = ctrl->val;
			flag_updateparam(m_gopcfg)
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L3_QP:
		if (vsiv4l2_get_maxhevclayern() >= 3) {
			pencparm->m_gopcfg.codinglayerqp[3] = ctrl->val;
			flag_updateparam(m_gopcfg)
		}
		break;
	case V4L2_CID_SECUREMODE:
		if (pencparm->m_securemode.secureModeOn != ctrl->val) {
			pencparm->m_securemode.secureModeOn = ctrl->val;
			flag_updateparam(m_securemode)
		}
		break;
	case V4L2_CID_ADDR_OFFSET:
		if (ctrl->p_new.p_s64)
			ctx->addr_offset = *ctrl->p_new.p_s64;
		break;
	case V4L2_CID_ENC_SCALE_INFO:
		if (ctrl->p_new.p) {
			if (vsiv4l2_configScaleOutput(ctx, ctrl->p_new.p) < 0)
				return -EINVAL;
		}
		break;
	default:
		return 0;
	}
 	return 0;
}

static int vsi_v4l2_enc_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vsi_v4l2_ctx *ctx = ctrl_to_ctx(ctrl);

	v4l2_klog(LOGLVL_CONFIG, "%s:%x", __func__, ctrl->id);
	if (!vsi_v4l2_daemonalive())
		return -ENODEV;
	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
		ctrl->val = ctx->mediacfg.minbuf_4capture;	//these two may come from resoultion change
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_OUTPUT:
		ctrl->val = ctx->mediacfg.minbuf_4output;
		break;
	case V4L2_CID_ROI_COUNT:
		ctrl->val = vsiv4l2_getROIcount();
		break;
	case V4L2_CID_IPCM_COUNT:
		ctrl->val = vsiv4l2_getIPCMcount();
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/********* for ext ctrl *************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static bool vsi_enc_ctrl_equal(const struct v4l2_ctrl *ctrl,
		      union v4l2_ctrl_ptr ptr1,
		      union v4l2_ctrl_ptr ptr2)
#else
static bool vsi_enc_ctrl_equal(const struct v4l2_ctrl *ctrl, u32 idx,
		      union v4l2_ctrl_ptr ptr1,
		      union v4l2_ctrl_ptr ptr2)
#endif
{
	//always update now, fix it later
	return 0;
}

static void vsi_enc_ctrl_init(const struct v4l2_ctrl *ctrl, u32 idx,
		     union v4l2_ctrl_ptr ptr)
{
	void *p = ptr.p + idx * ctrl->elem_size;

	memset(p, 0, ctrl->elem_size);
}

static void vsi_enc_ctrl_log(const struct v4l2_ctrl *ctrl)
{
	//do nothing now
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int vsi_enc_ctrl_validate(const struct v4l2_ctrl *ctrl,
			union v4l2_ctrl_ptr ptr)
#else
static int vsi_enc_ctrl_validate(const struct v4l2_ctrl *ctrl, u32 idx,
			union v4l2_ctrl_ptr ptr)
#endif
{
	//always true
	return 0;
}

static const struct v4l2_ctrl_type_ops vsi_enc_type_ops = {
	.equal = vsi_enc_ctrl_equal,
	.init = vsi_enc_ctrl_init,
	.log = vsi_enc_ctrl_log,
	.validate = vsi_enc_ctrl_validate,
};
/********* for ext ctrl *************/

static const struct v4l2_ctrl_ops vsi_encctrl_ops = {
	.s_ctrl = vsi_v4l2_enc_s_ctrl,
	.g_volatile_ctrl = vsi_v4l2_enc_g_volatile_ctrl,
};

static struct v4l2_ctrl_config vsi_v4l2_encctrl_defs[] = {
	{
		.ops = &vsi_encctrl_ops,
		.id = V4L2_CID_ROI_COUNT,
		.name = "get max ROI region number",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
		.min = 0,
		.max = V4L2_MAX_ROI_REGIONS,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &vsi_encctrl_ops,
		.type_ops = &vsi_enc_type_ops,
		.id = V4L2_CID_ROI,
		.name = "vsi priv v4l2 roi params set",
		.type = VSI_V4L2_CMPTYPE_ROI,
		.min = 0,
		.max = V4L2_MAX_ROI_REGIONS,
		.step = 1,
		.def = 0,
		.elem_size = sizeof(struct v4l2_enc_roi_params),
	},
	{
		.ops = &vsi_encctrl_ops,
		.id = V4L2_CID_IPCM_COUNT,
		.name = "get max IPCM region number",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
		.min = 0,
		.max = V4L2_MAX_IPCM_REGIONS,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &vsi_encctrl_ops,
		.type_ops = &vsi_enc_type_ops,
		.id = V4L2_CID_IPCM,
		.name = "vsi priv v4l2 ipcm params set",
		.type = VSI_V4L2_CMPTYPE_IPCM,
		.min = 0,
		.max = V4L2_MAX_IPCM_REGIONS,
		.step = 1,
		.def = 0,
		.elem_size = sizeof(struct v4l2_enc_ipcm_params),
	},
	/* kernel defined controls */
	{
		.id = V4L2_CID_MPEG_VIDEO_GOP_SIZE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = MAX_INTRA_PIC_RATE,
		.step = 1,
		.def = DEFAULT_INTRA_PIC_RATE,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_BITRATE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 10000,
		.max = 288000000,
		.step = 1,
		.def = 2097152,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_PROFILE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
		.max = V4L2_MPEG_VIDEO_H264_PROFILE_MULTIVIEW_HIGH,
		.def = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VP8_PROFILE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_VP8_PROFILE_0,
		.max = V4L2_MPEG_VIDEO_VP8_PROFILE_3,
		.def = V4L2_MPEG_VIDEO_VP8_PROFILE_0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VP9_PROFILE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_VP9_PROFILE_0,
		.max = V4L2_MPEG_VIDEO_VP9_PROFILE_3,
		.def = V4L2_MPEG_VIDEO_VP9_PROFILE_0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_PROFILE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min =  V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN,
		.max = V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_10,
		.def = V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_LEVEL,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_H264_LEVEL_1_0,
		.max = V4L2_MPEG_VIDEO_H264_LEVEL_5_2,
		.def = V4L2_MPEG_VIDEO_H264_LEVEL_5_0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_LEVEL,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_HEVC_LEVEL_1,
		.max = V4L2_MPEG_VIDEO_HEVC_LEVEL_5_1,
		.def = V4L2_MPEG_VIDEO_HEVC_LEVEL_5,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VP9_LEVEL,
		.name = "set vp9 encoding level",
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_VP9_LEVEL_1_0,
		.max = V4L2_MPEG_VIDEO_VP9_LEVEL_6_2,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_VP9_LEVEL_1_0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 51,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 51,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEADER_MODE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE,
		.max = V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME,
		.def = V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_B_FRAMES,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = MAX_GOP_SIZE - 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 51,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 51,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_BITRATE_MODE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_BITRATE_MODE_VBR,
		.max = V4L2_MPEG_VIDEO_BITRATE_MODE_CRF,
		.def = V4L2_MPEG_VIDEO_BITRATE_MODE_VBR,
	},
	{
		.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_VOLATILE,	//volatile contains read
		.min = 1,
		.max = MAX_MIN_BUFFERS_FOR_CAPTURE,
		.step = 1,
		.def = 1,
	},
	{
		.id = V4L2_CID_MIN_BUFFERS_FOR_OUTPUT,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
		.min = 1,
		.max = MAX_MIN_BUFFERS_FOR_OUTPUT,
		.step = 1,
		.def = 1,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME,
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 51,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 51,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 51,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 51,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE,
		.max = V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_MAX_MB,
		.def = V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 1,
		.max = 120,		//1920 div 16
		.step = 1,
		.def = 1,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_I_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 127,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_P_FRAME_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 127,
		.step = 1,
		.def = DEFAULT_QP,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_MIN_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 127,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 127,
		.step = 1,
		.def = 127,
	},
	{
		.id = V4L2_CID_ROTATE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 270,
		.step = 90,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 1,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_BITRATE_PEAK,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 10000,
		.max = 288000000,
		.step = 1,
		.def = 2097152,
	},
	{
		.id = V4L2_CID_JPEG_COMPRESSION_QUALITY,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 100,
		.step = 1,
		.def = 40,
	},
	{
		.id = V4L2_CID_JPEG_RESTART_INTERVAL,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 7,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD,
		.name = "set refresh rate",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = MAX_INTRA_PIC_RATE,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC,
		.max = V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC,
		.def = V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 175000,
		.max = 240000000,
		.step = 1,
		.def = 135000000,				//sync to default h264 level = L5
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_CONSTANT_QUALITY,
		.name = "video encoding constant quality",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -1,
		.max = 100,
		.step = 1,
		.def = 40,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE,
		.name = "frame skip mode select",
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED,
		.max = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT,
		.step = 1,
		.def = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_AU_DELIMITER,
		.name = "set au delimiter",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_ASPECT,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_ASPECT_1x1,
		.max = V4L2_MPEG_VIDEO_ASPECT_221x100,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_ASPECT_4x3,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_UNSPECIFIED,
		.max = V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_UNSPECIFIED,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 65535,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 65535,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_ENABLED,
		.max = V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_DISABLED,
		.max = V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_ENABLED,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE_DISABLED,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min =-6,
		.max = 6,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_LF_TC_OFFSET_DIV2,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min =-6,
		.max = 6,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min =-6,
		.max = 6,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_LF_BETA_OFFSET_DIV2,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min =-6,
		.max = 6,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_CHROMA_QP_INDEX_OFFSET,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min =-12,
		.max = 12,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_MIN_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MIN_QP,
		.name = "set hevc Iframe min qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MAX_QP,
		.name = "set hevc Iframe max qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_P_FRAME_MIN_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MIN_QP,
		.name = "set 264 Bframe min qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_P_FRAME_MAX_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MAX_QP,
		.name = "set 264 Bframe max qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MIN_QP,
		.name = "set hevc Pframe min qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MIN_QP,
		.name = "set hevc Bframe min qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MAX_QP,
		.name = "set hevc pframe max qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MAX_QP,
		.name = "set hevc Bframe max qp",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_MV_V_SEARCH_RANGE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 64,
		.step = 8,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_NUM_PARTITIONS,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_CID_MPEG_VIDEO_VPX_1_PARTITION,
		.max = V4L2_CID_MPEG_VIDEO_VPX_8_PARTITIONS,
		.step = 1,
		.def = V4L2_CID_MPEG_VIDEO_VPX_1_PARTITION,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_NUM_REF_FRAMES,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_CID_MPEG_VIDEO_VPX_1_REF_FRAME,
		.max = V4L2_CID_MPEG_VIDEO_VPX_3_REF_FRAME,
		.step = 1,
		.def = V4L2_CID_MPEG_VIDEO_VPX_1_REF_FRAME,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_REF_NUMBER_FOR_PFRAMES,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 1,
		.max = 2,
		.step = 1,
		.def = 1,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_FILTER_LEVEL,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 64,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_FILTER_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 8,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_VPX_GOLDEN_FRAME_REF_PERIOD,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 1000,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_TIER,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_HEVC_TIER_MAIN,
		.max = V4L2_MPEG_VIDEO_HEVC_TIER_HIGH,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_HEVC_TIER_MAIN,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_TYPE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_HEVC_REFRESH_NONE,
		.max = V4L2_MPEG_VIDEO_HEVC_REFRESH_IDR,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_HEVC_REFRESH_NONE,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_PERIOD,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 1000,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_TEMPORAL_ID,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_TMV_PREDICTION,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_WITHOUT_STARTCODE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_JPEG_CHROMA_SUBSAMPLING,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_JPEG_CHROMA_SUBSAMPLING_444,
		.max = V4L2_JPEG_CHROMA_SUBSAMPLING_GRAY,
		.step = 1,
		.def = V4L2_JPEG_CHROMA_SUBSAMPLING_420,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 1,
		.max = 4,
		.step = 1,
		.def = 1,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 0x30033,		//layer 3, max qp 51
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIER_QP,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_TYPE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_B,
		.max = V4L2_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_P,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_B,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_LAYER,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 1,
		.max = 4,
		.step = 1,
		.def = 1,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L0_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L1_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L2_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L3_QP,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 51,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE,
		.type = V4L2_CTRL_TYPE_MENU,
		.min = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_B,
		.max = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_P,
		.step = 1,
		.def = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_B,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_HEVC_STRONG_SMOOTHING,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDEO_H264_CONSTRAINED_INTRA_PREDICTION,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &vsi_encctrl_ops,
		.id = V4L2_CID_SECUREMODE,
		.name = "en/disable enc secure mode",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
	},
	{
		.ops = &vsi_encctrl_ops,
		.id = V4L2_CID_ADDR_OFFSET,
		.name = "set addr offset between VPU and CPU",
		.type = V4L2_CTRL_TYPE_INTEGER64,
		.min = 0x8000000000000000,
		.max = 0x7fffffffffffffff,
		.step = 1,
		.def = 0,
		.elem_size = sizeof(s64),
	},
	{
		.ops = &vsi_encctrl_ops,
		.type_ops = &vsi_enc_type_ops,
		.id = V4L2_CID_ENC_SCALE_INFO,
		.name = "vsi priv v4l2 enable enc scale output channel",
		.type = VSI_V4L2_CMPTYPE_ENCSCALEOUT,
		.min = 0,
		.max = 1,
		.step = 1,
		.def = 0,
		.elem_size = sizeof(struct v4l2_enc_scaleinfo),
	},
};

void vsi_enc_update_ctrltbl(int ctrlid, int index, s32 val)
{
	int i, ctrl_num = ARRAY_SIZE(vsi_v4l2_encctrl_defs);
	for (i = 0; i < ctrl_num; i++) {
		if (vsi_v4l2_encctrl_defs[i].id == ctrlid) {
			if (index == CTRL_IDX_MIN) {
				vsi_v4l2_encctrl_defs[i].min = val;
				if (vsi_v4l2_encctrl_defs[i].def < val)
					vsi_v4l2_encctrl_defs[i].min = val;
			}
			if (index == CTRL_IDX_MAX) {
				vsi_v4l2_encctrl_defs[i].max = val;
				if (vsi_v4l2_encctrl_defs[i].def > val)
					vsi_v4l2_encctrl_defs[i].def = val;
			}
			if (index == CTRL_IDX_DEF) {
				vsi_v4l2_encctrl_defs[i].def = val;
				if (vsi_v4l2_encctrl_defs[i].type == V4L2_CTRL_TYPE_INTEGER &&
					val < vsi_v4l2_encctrl_defs[i].min &&
					val == -1)
				vsi_v4l2_encctrl_defs[i].min = val;
			}
		}
	}
}

static void vsi_enc_ctx_defctrl(struct vsi_v4l2_ctx *ctx, int ctrlid, s32 defval)
{
	struct vsi_v4l2_encparams *pencparm = &ctx->mediacfg.m_encparams;

	switch (ctrlid) {
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		pencparm->m_intrapicrate.intraPicRate = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		pencparm->m_bitrate.bitPerSecond = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		ctx->mediacfg.profile_h264 = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VP8_PROFILE:
		ctx->mediacfg.profile_vp8 = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_PROFILE:
		ctx->mediacfg.profile_vp9 = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_PROFILE:
		ctx->mediacfg.profile_hevc = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
		ctx->mediacfg.avclevel = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LEVEL:
		ctx->mediacfg.hevclevel = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VP9_LEVEL:
		ctx->mediacfg.vp9level = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
		ctx->mediacfg.qpMax_h26x = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
		ctx->mediacfg.qpMin_h26x = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_B_FRAMES:
		pencparm->m_gopsize.gopSize = defval + 1;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP:
		pencparm->m_qphdrip.qpHdrB = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		if (defval == VSI_V4L2_BITRATE_MODE_VBR)
			pencparm->m_bitratemode.hrdConformance = 0;
		else {
			pencparm->m_bitratemode.hrdConformance = 1;
			pencparm->m_bitratemode.tolMovingBitRate = -1;
		}
		pencparm->m_bitratemode.bitrateMode = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP:
		ctx->mediacfg.qpHdrI_h26x = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP:
		ctx->mediacfg.qpHdrP_h26x = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE:
		pencparm->m_sliceinfo.multislice_mode = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB:
		pencparm->m_sliceinfo.sliceSize = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		pencparm->m_rcmode.picRc = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE:
		pencparm->m_rcmode.ctbRc = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_I_FRAME_QP:
		ctx->mediacfg.qpHdrI_vpx = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_P_FRAME_QP:
		ctx->mediacfg.qpHdrP_vpx = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_MIN_QP:
		ctx->mediacfg.qpMin_vpx = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_MAX_QP:
		ctx->mediacfg.qpMax_vpx = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER:
		pencparm->m_idrhdr.idrHdr = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_PEAK:
		pencparm->m_bitratemode.tolMovingBitRate = defval;
		break;
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		pencparm->m_jpgfixqp.fixedQP = vsi_enc_convertV4l2Quality(defval);
		break;
	case V4L2_CID_JPEG_RESTART_INTERVAL:
		pencparm->m_restartinterval.restartInterval = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_INTRA_REFRESH_PERIOD:
		pencparm->m_gdrduration.gdrDuration = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE:
		pencparm->m_entropymode.enableCabac = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE:
		pencparm->m_cpbsize.cpbSize = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM:
		pencparm->m_trans8x8.transform8x8Enable = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_CONSTANT_QUALITY:
		pencparm->m_qphdr.qpHdr = vsi_enc_convertV4l2Quality(defval);
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_SKIP_MODE:
		pencparm->m_skipmode.pictureSkip = defval - 1;
		break;
	case V4L2_CID_MPEG_VIDEO_AU_DELIMITER:
		pencparm->m_aud.sendAud = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE:
		pencparm->m_aspect.enable = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH:
		pencparm->m_aspect.sarWidth = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT:
		pencparm->m_aspect.sarHeight = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE:
		ctx->mediacfg.disableDeblockingFilter_h264 = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LOOP_FILTER_MODE:
		ctx->mediacfg.disableDeblockingFilter_hevc = (defval ? 0:1);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA:
	case V4L2_CID_MPEG_VIDEO_HEVC_LF_TC_OFFSET_DIV2:
		pencparm->m_loopfilter.tc_Offset = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA:
	case V4L2_CID_MPEG_VIDEO_HEVC_LF_BETA_OFFSET_DIV2:
		pencparm->m_loopfilter.beta_Offset = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CHROMA_QP_INDEX_OFFSET:
		pencparm->m_chromaqpoffset.chroma_qp_offset = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MIN_QP:
		pencparm->m_iqprange.qpMinI = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_MAX_QP:
		pencparm->m_iqprange.qpMaxI = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MIN_QP:
		pencparm->m_pqprange.qpMinPB = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_H264_B_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_MAX_QP:
	case V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_MAX_QP:
		pencparm->m_pqprange.qpMaxPB = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_MV_V_SEARCH_RANGE:
		pencparm->m_mvrange.meVertSearchRange = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_NUM_PARTITIONS:
		pencparm->m_vpxpartitions.dctPartitions = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_NUM_REF_FRAMES:
		ctx->mediacfg.vpx_PRefN = defval + 1;
		break;
	case V4L2_CID_MPEG_VIDEO_REF_NUMBER_FOR_PFRAMES:
		ctx->mediacfg.hevc_PRefN = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_FILTER_LEVEL:
		pencparm->m_vpxfilterlvl.filterLevel = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_FILTER_SHARPNESS:
		pencparm->m_vpxfiltersharp.filterSharpness = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_VPX_GOLDEN_FRAME_REF_PERIOD:
		pencparm->m_goldenperiod.goldenPictureRate = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_TIER:
		pencparm->m_tier.tier = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_TYPE:
		pencparm->m_refresh.refreshtype = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_REFRESH_PERIOD:
		pencparm->m_intrapicrate.intraPicRate = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_TEMPORAL_ID:
		pencparm->m_temporalid.temporalId = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_TMV_PREDICTION:
		pencparm->m_tmvp.enableTMVP = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_WITHOUT_STARTCODE:
		pencparm->m_startcode.streamType = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_PREPEND_SPSPPS_TO_IDR:
		pencparm->m_resendSPSPPS.resendSPSPPS = defval;
		break;
	case V4L2_CID_JPEG_CHROMA_SUBSAMPLING:
		pencparm->m_jpgcodingmode.codingMode = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING:
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_QP:
		pencparm->m_gopcfg.hierachy_enable = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER:
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_LAYER:
		pencparm->m_gopcfg.codinglayer = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP:
		pencparm->m_gopcfg.codinglayerqp[0] = defval;
		pencparm->m_gopcfg.codinglayerqp[1] = defval | (1 << 16);
		pencparm->m_gopcfg.codinglayerqp[2] = defval | (2 << 16);
		pencparm->m_gopcfg.codinglayerqp[3] = defval | (3 << 16);
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_TYPE:
	case V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE:
		pencparm->m_gopcfg.hierachy_codingtype = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L0_QP:
		pencparm->m_gopcfg.codinglayerqp[0] = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L1_QP:
		pencparm->m_gopcfg.codinglayerqp[1] = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L2_QP:
		pencparm->m_gopcfg.codinglayerqp[2] = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_HIER_CODING_L3_QP:
		pencparm->m_gopcfg.codinglayerqp[3] = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_STRONG_SMOOTHING:
		pencparm->m_strongsmooth.strong_intra_smoothing_enabled_flag = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CONSTRAINED_INTRA_PREDICTION:
		pencparm->m_ctrintrapred.constrained_intra_pred_flag = defval;
		break;
	case V4L2_CID_MPEG_VIDEO_HEADER_MODE:
		pencparm->m_headermode.headermode = defval;
		break;
	default:
		break;
	}
}

static int vsi_v4l2_enc_verify_ctrl_typeNrange(struct v4l2_ctrl_config *pctrl)
{
	if (pctrl->type == V4L2_CTRL_TYPE_MENU ||
		pctrl->type == V4L2_CTRL_TYPE_BOOLEAN)
		if (pctrl->max <= pctrl->min)
			return 0;
	return 1;
}

static int vsi_setup_enc_ctrls(struct vsi_v4l2_ctx *ctx)
{
	int i, ctrl_num = ARRAY_SIZE(vsi_v4l2_encctrl_defs);
	struct v4l2_ctrl_handler *handler = &ctx->ctrlhdl;
	struct v4l2_ctrl *ctrl = NULL;

	v4l2_ctrl_handler_init(handler, ctrl_num);

	if (handler->error)
		return handler->error;

	for (i = 0; i < ctrl_num; i++) {
		vsi_enc_ctx_defctrl(ctx, vsi_v4l2_encctrl_defs[i].id, vsi_v4l2_encctrl_defs[i].def);
		if (!vsi_v4l2_enc_verify_ctrl_typeNrange(&vsi_v4l2_encctrl_defs[i]))
			continue;
		if (is_vsi_ctrl(vsi_v4l2_encctrl_defs[i].id)) {
			vsi_v4l2_encctrl_defs[i].ops = &vsi_encctrl_ops;
			if (vsi_v4l2_encctrl_defs[i].type == V4L2_CTRL_TYPE_MENU) {
				vsi_v4l2_encctrl_defs[i].type = V4L2_CTRL_TYPE_INTEGER;
				vsi_v4l2_encctrl_defs[i].step = 1;
			}
			ctrl = v4l2_ctrl_new_custom(handler, &vsi_v4l2_encctrl_defs[i], NULL);
		}
		else {
			if (vsi_v4l2_encctrl_defs[i].type == V4L2_CTRL_TYPE_MENU) {
				ctrl = v4l2_ctrl_new_std_menu(handler, &vsi_encctrl_ops,
					vsi_v4l2_encctrl_defs[i].id,
					vsi_v4l2_encctrl_defs[i].max,
					0,
					vsi_v4l2_encctrl_defs[i].def);
			} else {
				ctrl = v4l2_ctrl_new_std(handler,
					&vsi_encctrl_ops,
					vsi_v4l2_encctrl_defs[i].id,
					vsi_v4l2_encctrl_defs[i].min,
					vsi_v4l2_encctrl_defs[i].max,
					vsi_v4l2_encctrl_defs[i].step,
					vsi_v4l2_encctrl_defs[i].def);
			}
		}
		if (ctrl && (vsi_v4l2_encctrl_defs[i].flags & V4L2_CTRL_FLAG_VOLATILE))
			ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

		if (handler->error) {
			v4l2_klog(LOGLVL_WARNING, "fail to set ctrl %d 0x%x:%d",
								i, vsi_v4l2_encctrl_defs[i].id, handler->error);
			handler->error = 0;
			/*To avoid vsi_daemon return failed*/
			udelay(50);
		}
	}

	v4l2_ctrl_handler_setup(handler);
	return handler->error;
}

static int v4l2_enc_open(struct file *filp)
{
	//struct video_device *vdev = video_devdata(filp);
	struct vsi_v4l2_device *dev = video_drvdata(filp);
	struct vsi_v4l2_ctx *ctx = NULL;
	struct vb2_queue *q;
	int ret = 0;
	struct v4l2_fh *vfh;
	pid_t  pid;

	/* Allocate memory for context */
	//fh->video_devdata = struct video_device, struct video_device->video_drvdata = struct vsi_v4l2_device
	if (vsi_v4l2_addinstance(&pid) < 0)
		return -EBUSY;

	ctx = vsi_create_ctx();
	if (ctx == NULL) {
		vsi_v4l2_quitinstance();
		return -ENOMEM;
	}

	v4l2_fh_init(&ctx->fh, video_devdata(filp));
	filp->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);
	ctx->dev = dev;
	mutex_init(&ctx->ctxlock);
	ctx->flag = CTX_FLAG_ENC;
 	set_bit(CTX_FLAG_ENC_FLUSHBUF, &ctx->flag);

	ctx->frameidx = 0;
	q = &ctx->input_que;
	q->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->min_buffers_needed = MIN_FRAME_4ENC;
	q->drv_priv = &ctx->fh;
	q->lock = &ctx->ctxlock;
	q->buf_struct_size = sizeof(struct vsi_vpu_buf);		//used to alloc mem control structures in reqbuf
	q->ops = &vsi_enc_qops;		/*it might be used to identify input and output */
	q->mem_ops = get_vsi_mmop();
	q->memory = VB2_MEMORY_UNKNOWN;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	INIT_LIST_HEAD(&ctx->input_list);
	ret = vb2_queue_init(q);
	/*q->buf_ops = &v4l2_buf_ops is set here*/
	if (ret)
		goto err_enc_dec_exit;

	q = &ctx->output_que;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->min_buffers_needed = 1;
	q->drv_priv = &ctx->fh;
	q->lock = &ctx->ctxlock;
	q->buf_struct_size = sizeof(struct vsi_vpu_buf);
	q->ops = &vsi_enc_qops;
	q->mem_ops = get_vsi_mmop();
	q->memory = VB2_MEMORY_UNKNOWN;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	INIT_LIST_HEAD(&ctx->output_list);
	ret = vb2_queue_init(q);
	if (ret) {
		vb2_queue_release(&ctx->input_que);
		goto err_enc_dec_exit;
	}
	vsiv4l2_initcfg(ctx);
	vsi_setup_enc_ctrls(ctx);
	vfh = (struct v4l2_fh *)filp->private_data;
	vfh->ctrl_handler = &ctx->ctrlhdl;
	atomic_set(&ctx->srcframen, 0);
	atomic_set(&ctx->dstframen, 0);
	ctx->status = VSI_STATUS_INIT;

	return 0;

err_enc_dec_exit:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	vsi_remove_ctx(ctx);
	kfree(ctx);
	vsi_v4l2_quitinstance();
	return ret;
}

static int v4l2_enc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(filp->private_data);
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	int ret;

	v4l2_klog(LOGLVL_FLOW, "%s", __func__);
	if (offset < OUTF_BASE) {
		ret = vb2_mmap(&ctx->input_que, vma);
	} else {
		vma->vm_pgoff -= (OUTF_BASE >> PAGE_SHIFT);
		offset -= OUTF_BASE;
		ret = vb2_mmap(&ctx->output_que, vma);
	}
	return ret;
}

static __poll_t vsi_enc_poll(struct file *file, poll_table *wait)
{
	__poll_t ret = 0;
	struct vsi_v4l2_ctx *ctx = fh_to_ctx(file->private_data);
	int dstn = atomic_read(&ctx->dstframen);
	int srcn = atomic_read(&ctx->srcframen);

	if (!vsi_v4l2_daemonalive())
		ret |= POLLERR;

	if (v4l2_event_pending(&ctx->fh)) {
		v4l2_klog(LOGLVL_BRIEF, "%s event", __func__);
		ret |= POLLPRI;
	}
	ret |= vb2_poll(&ctx->output_que, file, wait);
	ret |= vb2_poll(&ctx->input_que, file, wait);

	/*recheck for poll hang*/
	if (ret == 0) {
		if (dstn != atomic_read(&ctx->dstframen))
			ret |= vb2_poll(&ctx->output_que, file, wait);
		if (srcn != atomic_read(&ctx->srcframen))
			ret |= vb2_poll(&ctx->input_que, file, wait);
	}
	if (ctx->error < 0)
		ret |= POLLERR;

	v4l2_klog(LOGLVL_VERBOSE, "%s %x", __func__, ret);
	return ret;
}

static const struct v4l2_file_operations v4l2_enc_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_enc_open,
	.release = vsi_v4l2_release,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_enc_mmap,
	.poll = vsi_enc_poll,
};

struct video_device *vsi_v4l2_probe_enc(struct platform_device *pdev, struct vsi_v4l2_device *vpu)
{
	struct video_device *venc;
	int ret = 0;

	v4l2_klog(LOGLVL_BRIEF, "%s", __func__);

	/*init video device0, encoder */
	venc = video_device_alloc();
	if (!venc) {
		v4l2_err(&vpu->v4l2_dev, "Failed to allocate enc device\n");
		ret = -ENOMEM;
		goto err;
	}

	venc->fops = &v4l2_enc_fops;
	venc->ioctl_ops = &vsi_enc_ioctl;
	venc->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	venc->release = video_device_release;
	venc->lock = &vpu->lock;
	venc->v4l2_dev = &vpu->v4l2_dev;
	venc->vfl_dir = VFL_DIR_M2M;
	venc->vfl_type = VSI_DEVTYPE;
	venc->queue = NULL;

	video_set_drvdata(venc, vpu);

	ret = video_register_device(venc, VSI_DEVTYPE, VIDEO_NODE_OFFSET);
	if (ret) {
		v4l2_err(&vpu->v4l2_dev, "Failed to register enc device\n");
		video_device_release(venc);
		goto err;
	}

	return venc;
err:
	return NULL;
}

void vsi_v4l2_release_enc(struct video_device *venc)
{
	video_unregister_device(venc);
}

