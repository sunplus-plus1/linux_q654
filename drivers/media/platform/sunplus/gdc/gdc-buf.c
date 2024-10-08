// SPDX-License-Identifier: GPL-2.0

#include <linux/pm_runtime.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>
#include "gdc.h"

static int gdc_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			   unsigned int *nplanes, unsigned int sizes[],
			   struct device *alloc_devs[])
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	struct gdc_frame *f = gdc_get_frame(ctx, vq->type);

	if (IS_ERR(f))
		return PTR_ERR(f);

	if (*nplanes)
		return sizes[0] < f->size ? -EINVAL : 0;

	sizes[0] = f->size;
	*nplanes = 1;

	return 0;
}

static int gdc_buf_prepare(struct vb2_buffer *vb)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct gdc_frame *f = gdc_get_frame(ctx, vb->vb2_queue->type);

	if (IS_ERR(f))
		return PTR_ERR(f);

	vb2_set_plane_payload(vb, 0, f->size);

	return 0;
}

static void gdc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct gdc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static void gdc_buf_return_buffers(struct vb2_queue *q,
				   enum vb2_buffer_state state)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *vbuf;

	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(q->type))
			vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (!vbuf)
			break;
		v4l2_m2m_buf_done(vbuf, state);
	}
}

static int gdc_buf_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(q);
	struct gdc_device *gdev = ctx->gdev;
	int ret;

	ret = pm_runtime_resume_and_get(gdev->dev);
	if (ret < 0) {
		gdc_buf_return_buffers(q, VB2_BUF_STATE_QUEUED);
		return ret;
	}

	return 0;
}

static void gdc_buf_stop_streaming(struct vb2_queue *q)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(q);
	struct gdc_device *gdev = ctx->gdev;

	gdc_buf_return_buffers(q, VB2_BUF_STATE_ERROR);
	pm_runtime_put(gdev->dev);
}

const struct vb2_ops gdc_qops = {
	.queue_setup = gdc_queue_setup,
	.buf_prepare = gdc_buf_prepare,
	.buf_queue = gdc_buf_queue,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
	.start_streaming = gdc_buf_start_streaming,
	.stop_streaming = gdc_buf_stop_streaming,
};
