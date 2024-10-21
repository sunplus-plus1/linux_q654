// SPDX-License-Identifier: GPL-2.0

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>
#include "gdc.h"

static struct gdc_fmt gdc_v4l2_formats[] = {
	/* YUV420 */
	{
		.fourcc = V4L2_PIX_FMT_NV12,
		.nplanes = 2,
		.depth = 12,
		.h_div = 2,
		.w_div = 1,
	},
	{
		.fourcc = V4L2_PIX_FMT_NV21,
		.nplanes = 2,
		.depth = 12,
		.h_div = 2,
		.w_div = 1,
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420,
		.nplanes = 3,
		.depth = 12,
		.h_div = 2,
		.w_div = 2,
	},
	{
		.fourcc = V4L2_PIX_FMT_YVU420,
		.nplanes = 3,
		.depth = 12,
		.h_div = 2,
		.w_div = 2,
	},
	/* Greyscale */
	{
		.fourcc = V4L2_PIX_FMT_GREY,
		.nplanes = 3,
		.depth = 8,
		.h_div = 1,
		.w_div = 1,
	},
};

static struct gdc_frame gdc_default_frame = {
	.width = GDC_DEFAULT_WIDTH,
	.height = GDC_DEFAULT_HEIGHT,
	.colorspace = V4L2_COLORSPACE_DEFAULT,
	.fmt = &gdc_v4l2_formats[0],
};

static irqreturn_t gdc_irq_handler(int irq, void *prv)
{
	struct gdc_device *gdev = prv;
	struct gdc_v4l2 *v4l2 = &gdev->v4l2;
	struct vb2_v4l2_buffer *src, *dst;
	struct gdc_ctx *ctx;

	if (irq != gdev->irq) {
		dev_err(gdev->dev, "irq miss-match.\n");
		return IRQ_HANDLED;
	}

	ctx = v4l2->curr_ctx;

	WARN_ON(!ctx);

	v4l2->curr_ctx = NULL;

	src = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

	WARN_ON(!src);
	WARN_ON(!dst);

	dst->timecode = src->timecode;
	dst->vb2_buf.timestamp = src->vb2_buf.timestamp;
	dst->flags &= ~V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
	dst->flags |= src->flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK;

	v4l2_m2m_buf_done(src, VB2_BUF_STATE_DONE);
	v4l2_m2m_buf_done(dst, VB2_BUF_STATE_DONE);
	v4l2_m2m_job_finish(v4l2->m2m_dev, ctx->fh.m2m_ctx);

	return IRQ_HANDLED;
}

static int gdc_queue_init(void *priv, struct vb2_queue *src_vq,
			  struct vb2_queue *dst_vq)
{
	struct gdc_ctx *ctx = priv;
	struct gdc_v4l2 *v4l2 = &ctx->gdev->v4l2;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &gdc_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &v4l2->lock;
	src_vq->dev = v4l2->v4l2_dev.dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &gdc_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &v4l2->lock;
	dst_vq->dev = v4l2->v4l2_dev.dev;

	return vb2_queue_init(dst_vq);
}

static int gdc_fop_open(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct gdc_device *gdev = video_drvdata(file);
	struct gdc_v4l2 *v4l2 = &gdev->v4l2;
	struct gdc_ctx *ctx;
	int ret = 0;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->gdev = gdev;
	ctx->in_frame = gdc_default_frame;
	ctx->out_frame = gdc_default_frame;

	if (mutex_lock_interruptible(&v4l2->lock)) {
		kfree(ctx);
		return -ERESTARTSYS;
	}

	ctx->fh.m2m_ctx =
		v4l2_m2m_ctx_init(v4l2->m2m_dev, ctx, &gdc_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		mutex_unlock(&v4l2->lock);
		kfree(ctx);
		return ret;
	}

	v4l2_fh_init(&ctx->fh, vdev);
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	ctx->max_config_size = gdev->max_config_size;
	ctx->config_addr = dma_alloc_coherent(gdev->dev, ctx->max_config_size,
					      &ctx->config_dma_addr,
					      GFP_KERNEL);

	if (!ctx->config_addr) {
		dev_err(gdev->dev,
			"Alloc config buf error, target size:%ldBytes\n",
			ctx->max_config_size);
		mutex_unlock(&v4l2->lock);
		kfree(ctx);
		return -ENOMEM;
	}
	mutex_unlock(&v4l2->lock);

	return 0;
}

static ssize_t gdc_fop_read(struct file *file, char __user *user_buf,
			    size_t count, loff_t *ppos)
{
	struct gdc_ctx *ctx =
		container_of(file->private_data, struct gdc_ctx, fh);

	*ppos = 0;

	return simple_read_from_buffer(user_buf, count, ppos, ctx->config_addr,
				       ctx->config_size);
}

static ssize_t gdc_fop_write(struct file *file, const char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct gdc_ctx *ctx =
		container_of(file->private_data, struct gdc_ctx, fh);

	*ppos = 0;

	ctx->config_size = simple_write_to_buffer(
		ctx->config_addr, ctx->max_config_size, ppos, user_buf, count);

	return ctx->config_size;
}

static int gdc_fop_release(struct file *file)
{
	struct gdc_ctx *ctx =
		container_of(file->private_data, struct gdc_ctx, fh);
	struct gdc_device *gdev = video_drvdata(file);
	struct gdc_v4l2 *v4l2 = &gdev->v4l2;

	mutex_lock(&v4l2->lock);

	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);

	mutex_unlock(&v4l2->lock);

	return 0;
}

struct gdc_frame *gdc_get_frame(struct gdc_ctx *ctx, enum v4l2_buf_type type)
{
	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		return &ctx->in_frame;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		return &ctx->out_frame;
	default:
		return ERR_PTR(-EINVAL);
	}
}

static int gdc_vidioc_querycap(struct file *file, void *priv,
			       struct v4l2_capability *cap)
{
	strscpy(cap->driver, GDC_DRIVER_NAME, sizeof(cap->driver));
	strscpy(cap->card, GDC_DEV_NAME, sizeof(cap->card));
	strscpy(cap->bus_info, GDC_BUS_NAME, sizeof(cap->bus_info));

	return 0;
}

static int gdc_vidioc_enum_fmt(struct file *file, void *prv,
			       struct v4l2_fmtdesc *f)
{
	struct gdc_fmt *fmt;

	if (f->index >= ARRAY_SIZE(gdc_v4l2_formats))
		return -EINVAL;

	fmt = &gdc_v4l2_formats[f->index];
	f->pixelformat = fmt->fourcc;

	return 0;
}

static int gdc_vidioc_g_fmt(struct file *file, void *prv, struct v4l2_format *f)
{
	struct gdc_ctx *ctx = prv;
	struct vb2_queue *vq;
	struct gdc_frame *frm;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;

	frm = gdc_get_frame(ctx, f->type);
	if (IS_ERR(frm))
		return PTR_ERR(frm);

	f->fmt.pix.width = frm->width;
	f->fmt.pix.height = frm->height;
	f->fmt.pix.field = V4L2_FIELD_NONE;
	f->fmt.pix.pixelformat = frm->fmt->fourcc;
	f->fmt.pix.bytesperline = frm->bytesperline;
	f->fmt.pix.sizeimage = frm->size;
	f->fmt.pix.colorspace = frm->colorspace;

	return 0;
}

static struct gdc_fmt *gdc_fmt_find(struct v4l2_format *f)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(gdc_v4l2_formats); i++) {
		if (gdc_v4l2_formats[i].fourcc == f->fmt.pix.pixelformat)
			return &gdc_v4l2_formats[i];
	}
	return NULL;
}

static int gdc_vidioc_try_fmt(struct file *file, void *prv,
			      struct v4l2_format *f)
{
	struct gdc_fmt *fmt;

	fmt = gdc_fmt_find(f);
	if (!fmt) {
		fmt = &gdc_v4l2_formats[0];
		f->fmt.pix.pixelformat = fmt->fourcc;
	}

	f->fmt.pix.field = V4L2_FIELD_NONE;

	f->fmt.pix.bytesperline = (f->fmt.pix.width * fmt->depth) >> 3;

	f->fmt.pix.sizeimage =
		f->fmt.pix.height * (f->fmt.pix.width * fmt->depth) >> 3;

	return 0;
}

static int gdc_vidioc_s_fmt(struct file *file, void *prv, struct v4l2_format *f)
{
	struct gdc_ctx *ctx = prv;
	struct gdc_device *gdev = ctx->gdev;
	struct vb2_queue *vq;
	struct gdc_frame *frm;
	struct gdc_fmt *fmt;
	int ret = 0;

	ret = gdc_vidioc_try_fmt(file, prv, f);
	if (ret) {
		dev_err(gdev->dev, "try format error on %d\n", ret);
		return ret;
	}

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vb2_is_busy(vq)) {
		dev_err(gdev->dev, "queue (%d) busy\n", f->type);
		return -EBUSY;
	}
	frm = gdc_get_frame(ctx, f->type);
	if (IS_ERR(frm))
		return PTR_ERR(frm);
	fmt = gdc_fmt_find(f);
	if (!fmt) {
		dev_err(gdev->dev, "format not support\n");
		return -EINVAL;
	}
	frm->width = f->fmt.pix.width;
	frm->height = f->fmt.pix.height;
	frm->size = f->fmt.pix.sizeimage;
	frm->fmt = fmt;
	frm->bytesperline = f->fmt.pix.bytesperline;
	frm->colorspace = f->fmt.pix.colorspace;

	return 0;
}

static const struct v4l2_ioctl_ops gdc_ioctl_ops = {
	.vidioc_querycap = gdc_vidioc_querycap,

	.vidioc_enum_fmt_vid_cap = gdc_vidioc_enum_fmt,
	.vidioc_g_fmt_vid_cap = gdc_vidioc_g_fmt,
	.vidioc_try_fmt_vid_cap = gdc_vidioc_try_fmt,
	.vidioc_s_fmt_vid_cap = gdc_vidioc_s_fmt,

	.vidioc_enum_fmt_vid_out = gdc_vidioc_enum_fmt,
	.vidioc_g_fmt_vid_out = gdc_vidioc_g_fmt,
	.vidioc_try_fmt_vid_out = gdc_vidioc_try_fmt,
	.vidioc_s_fmt_vid_out = gdc_vidioc_s_fmt,

	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,

	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,

	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
};

static const struct v4l2_file_operations gdc_fops = {
	.owner = THIS_MODULE,
	.open = gdc_fop_open,
	.read = gdc_fop_read,
	.write = gdc_fop_write,
	.release = gdc_fop_release,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
	.poll = v4l2_m2m_fop_poll,
};

static void gdc_m2m_device_run(void *prv)
{
	struct gdc_ctx *ctx = prv;
	struct gdc_v4l2 *v4l2 = &ctx->gdev->v4l2;
	struct vb2_v4l2_buffer *src, *dst;
	unsigned long flags;

	spin_lock_irqsave(&v4l2->spin_lock, flags);

	v4l2->curr_ctx = ctx;

	src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	gdc_run(ctx, &src->vb2_buf, &dst->vb2_buf);

	spin_unlock_irqrestore(&v4l2->spin_lock, flags);
}

static const struct v4l2_m2m_ops gdc_m2m_ops = {
	.device_run = gdc_m2m_device_run,
};

static int gdc_v4l2_init(struct gdc_device *gdev)
{
	struct gdc_v4l2 *v4l2 = &gdev->v4l2;
	struct video_device *vdev = &v4l2->vdev;
	int ret;

	mutex_init(&v4l2->lock);
	spin_lock_init(&v4l2->spin_lock);

	ret = v4l2_device_register(gdev->dev, &v4l2->v4l2_dev);
	if (ret)
		return ret;

	v4l2->m2m_dev = v4l2_m2m_init(&gdc_m2m_ops);
	if (IS_ERR(v4l2->m2m_dev)) {
		dev_err(gdev->dev, "Failed to init mem2mem device\n");
		ret = PTR_ERR(v4l2->m2m_dev);
		goto unreg_v4l2_dev;
	}

	snprintf(vdev->name, sizeof(vdev->name), "gdc");
	vdev->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING |
			    V4L2_CAP_READWRITE;
	vdev->lock = &v4l2->lock;
	vdev->v4l2_dev = &v4l2->v4l2_dev;
	vdev->vfl_dir = VFL_DIR_M2M;
	vdev->vfl_type = VFL_TYPE_VIDEO;
	vdev->fops = &gdc_fops;
	vdev->ioctl_ops = &gdc_ioctl_ops;
	vdev->release = video_device_release_empty;

	video_set_drvdata(vdev, gdev);

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, v4l2->devnode_number);
	if (ret) {
		dev_err(gdev->dev, "Failed to register video device\n");
		goto deinit_m2m;
	}

	gdc_default_frame.bytesperline =
		(gdc_default_frame.width * gdc_default_frame.fmt->depth) >> 3;
	gdc_default_frame.size =
		gdc_default_frame.bytesperline * gdc_default_frame.height;

	return 0;

deinit_m2m:
	v4l2_m2m_release(v4l2->m2m_dev);

unreg_v4l2_dev:
	v4l2_device_unregister(&v4l2->v4l2_dev);

	return ret;
}

static int gdc_v4l2_release(struct gdc_device *gdev)
{
	struct gdc_v4l2 *v4l2 = &gdev->v4l2;

	v4l2_m2m_release(v4l2->m2m_dev);
	video_unregister_device(&v4l2->vdev);
	v4l2_device_unregister(&v4l2->v4l2_dev);

	return 0;
}

static int gdc_parse_dt(struct platform_device *pdev, struct gdc_device *gdev)
{
	struct resource *res;
	int ret;

	gdev->irq = platform_get_irq(pdev, 0);
	if (gdev->irq < 0) {
		dev_err(gdev->dev, "no gdc irq found from DT\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(gdev->dev, "no reg_base from DT\n");
		return -ENODEV;
	}

	gdev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(gdev->reg_base)) {
		dev_err(gdev->dev, "ioremap reg_base error\n");
		return -EINVAL;
	}

	gdev->supply = devm_regulator_get(&pdev->dev, "gdc");
	if (IS_ERR(gdev->supply)) {
		dev_err(&pdev->dev, "Failed to get regulator\n");
		return PTR_ERR(gdev->supply);
	}

	gdev->srst = devm_reset_control_get(&pdev->dev, "sys");
	if (IS_ERR(gdev->srst)) {
		dev_err(&pdev->dev, "Failed to get sys reset controller\n");
		return PTR_ERR(gdev->srst);
	}

	gdev->arst = devm_reset_control_get(&pdev->dev, "axi");
	if (IS_ERR(gdev->arst)) {
		dev_err(&pdev->dev, "Failed to get axi reset controller\n");
		return PTR_ERR(gdev->arst);
	}

	gdev->prst = devm_reset_control_get(&pdev->dev, "apb");
	if (IS_ERR(gdev->prst)) {
		dev_err(&pdev->dev, "Failed to get APB reset controller\n");
		return PTR_ERR(gdev->prst);
	}

	gdev->sclk = devm_clk_get(&pdev->dev, "sys");
	if (IS_ERR(gdev->sclk)) {
		dev_err(&pdev->dev, "Failed to get sys clock\n");
		return PTR_ERR(gdev->sclk);
	}

	gdev->aclk = devm_clk_get(&pdev->dev, "axi");
	if (IS_ERR(gdev->aclk)) {
		dev_err(&pdev->dev, "Failed to get axi clock\n");
		return PTR_ERR(gdev->aclk);
	}

	gdev->pclk = devm_clk_get(&pdev->dev, "apb");
	if (IS_ERR(gdev->pclk)) {
		dev_err(&pdev->dev, "Failed to get APB clock\n");
		return PTR_ERR(gdev->pclk);
	}

	ret = device_property_read_u32(&pdev->dev, "sunplus,max-config-size",
				       &gdev->max_config_size);
	if (ret) {
		dev_err(&pdev->dev, "No sunplus,max_config_size from DT\n");
		return ret;
	}

	if (gdev->max_config_size & 0x3) {
		dev_err(&pdev->dev,
			"max_config_size(0x%x) is not aligned to 4 bytes.\n",
			gdev->max_config_size);
		return -EINVAL;
	}

	ret = device_property_read_u32(&pdev->dev, "sunplus,devnode-number",
				       &gdev->v4l2.devnode_number);
	if (ret) {
		dev_info(
			&pdev->dev,
			"No sunplus,devnode_number from DT, use defaut value(%d)\n",
			GDC_DEV_NODE_NUM);
		gdev->v4l2.devnode_number = GDC_DEV_NODE_NUM;
	}

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "Could not get reserved memory!\n");
		return ret;
	}

	ret = dma_set_coherent_mask(&pdev->dev,
				    dma_get_required_mask(&pdev->dev));
	if (ret) {
		dev_warn(&pdev->dev, "%lld-bit consistent DMA enable failed\n",
			 dma_get_required_mask(&pdev->dev));
		return ret;
	}

	return 0;
}

static int gdc_clock_enable(struct gdc_device *gdev)
{
	int ret;

	ret = clk_prepare_enable(gdev->sclk);
	if (ret) {
		dev_err(gdev->dev, "Cannot enable gdc sclk: %d\n", ret);
		return ret;
	}

	if (gdev->pclk) {
		ret = clk_prepare_enable(gdev->pclk);
		if (ret) {
			dev_err(gdev->dev, "Cannot enable gdc pclk: %d\n", ret);
			goto err_disable_sclk;
		}
	}

	ret = clk_prepare_enable(gdev->aclk);
	if (ret) {
		dev_err(gdev->dev, "Cannot enable gdc aclk: %d\n", ret);
		goto err_disable_pclk;
	}

	return 0;

err_disable_pclk:
	if (gdev->pclk)
		clk_disable_unprepare(gdev->pclk);

err_disable_sclk:
	clk_disable_unprepare(gdev->sclk);

	return ret;
}

static void gdc_clock_disable(struct gdc_device *gdev)
{
	clk_disable_unprepare(gdev->sclk);

	if (gdev->pclk)
		clk_disable_unprepare(gdev->pclk);

	clk_disable_unprepare(gdev->aclk);
}

static int gdc_reset_assert(struct gdc_device *gdev)
{
	int ret;

	ret = reset_control_assert(gdev->srst);
	if (ret) {
		dev_err(gdev->dev, "Cannot assert gdc srst: %d\n", ret);
		return ret;
	}

	if (gdev->prst) {
		ret = reset_control_assert(gdev->prst);
		if (ret) {
			dev_err(gdev->dev, "Cannot assert gdc prst: %d\n", ret);
			goto err_deassert_srst;
		}
	}

	ret = reset_control_assert(gdev->arst);
	if (ret) {
		dev_err(gdev->dev, "Cannot assert gdc arst: %d\n", ret);
		goto err_deassert_prst;
	}

	return 0;

err_deassert_prst:
	if (gdev->prst)
		reset_control_deassert(gdev->prst);

err_deassert_srst:
	reset_control_deassert(gdev->srst);

	return ret;
}

static int gdc_reset_deassert(struct gdc_device *gdev)
{
	int ret;

	ret = reset_control_deassert(gdev->srst);
	if (ret) {
		dev_err(gdev->dev, "Cannot deassert gdc srst: %d\n", ret);
		return ret;
	}

	if (gdev->prst) {
		ret = reset_control_deassert(gdev->prst);
		if (ret) {
			dev_err(gdev->dev, "Cannot deassert gdc prst: %d\n",
				ret);
			goto err_assert_srst;
		}
	}

	ret = reset_control_deassert(gdev->arst);
	if (ret) {
		dev_err(gdev->dev, "Cannot deassert gdc arst: %d\n", ret);
		goto err_assert_prst;
	}

	return 0;

err_assert_prst:
	if (gdev->prst)
		reset_control_assert(gdev->prst);

err_assert_srst:
	reset_control_assert(gdev->srst);

	return ret;
}

static int gdc_probe(struct platform_device *pdev)
{
	struct gdc_device *gdev;
	int ret;

	gdev = devm_kzalloc(&pdev->dev, sizeof(*gdev), GFP_KERNEL);
	if (!gdev)
		return -ENOMEM;

	gdev->dev = &pdev->dev;
	platform_set_drvdata(pdev, gdev);

	ret = gdc_parse_dt(pdev, gdev);
	if (ret) {
		dev_err(&pdev->dev, "parse dt error\n");
		return ret;
	}

	ret = regulator_enable(gdev->supply);
	if (ret) {
		dev_err(&pdev->dev, "gdc enable regulator error\n");
		return ret;
	}

	ret = gdc_clock_enable(gdev);
	if (ret) {
		dev_err(&pdev->dev, "gdc enable clock error\n");
		return ret;
	}

	ret = gdc_reset_deassert(gdev);
	if (ret) {
		dev_err(&pdev->dev, "gdc assert reset error\n");
		goto clock_disable;
	}

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = devm_request_irq(gdev->dev, gdev->irq, gdc_irq_handler, 0,
			       dev_name(&pdev->dev), gdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "request irq error\n");
		goto pm_disable;
	}

	ret = gdc_v4l2_init(gdev);
	if (ret) {
		dev_err(&pdev->dev, "init v4l2 error\n");
		goto pm_disable;
	}

	return 0;

clock_disable:
	gdc_clock_disable(gdev);

pm_disable:
	pm_runtime_disable(&pdev->dev);
	gdc_reset_assert(gdev);

	return ret;
}

static int gdc_remove(struct platform_device *pdev)
{
	struct gdc_device *gdev = platform_get_drvdata(pdev);

	gdc_v4l2_release(gdev);
	pm_runtime_disable(gdev->dev);
	gdc_reset_assert(gdev);
	gdc_clock_disable(gdev);
	regulator_disable(gdev->supply);

	return 0;
}

static int __maybe_unused gdc_runtime_suspend(struct device *dev)
{
	struct gdc_device *gdev = dev_get_drvdata(dev);

	gdc_clock_disable(gdev);

	return 0;
}

static int __maybe_unused gdc_runtime_resume(struct device *dev)
{
	struct gdc_device *gdev = dev_get_drvdata(dev);

	return gdc_clock_enable(gdev);
}

static const struct dev_pm_ops gdc_pm_ops = { SET_RUNTIME_PM_OPS(
	gdc_runtime_suspend, gdc_runtime_resume, NULL) };

static const struct of_device_id __maybe_unused gdc_dt_match[] = {
	{
		.compatible = "sunplus,sp7350-gdc",
	},
	{}
};

MODULE_DEVICE_TABLE(of, gdc_dt_match);

static struct platform_driver gdc_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = GDC_DRIVER_NAME,
		.pm = &gdc_pm_ops,
		.of_match_table = of_match_ptr(gdc_dt_match),
	},
	.probe  = gdc_probe,
	.remove = gdc_remove,
};

module_platform_driver(gdc_driver);

MODULE_AUTHOR("YuBo Leng <yb.leng@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus Geometric Distortion Correction");
MODULE_LICENSE("GPL v2");
