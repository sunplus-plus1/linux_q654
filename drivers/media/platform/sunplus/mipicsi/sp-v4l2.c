// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for VIN
 *
 */
#include <linux/pm_runtime.h>

#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mc.h>
#include <media/v4l2-rect.h>

#include "sp-vin.h"

#define VIN_DEFAULT_FORMAT	V4L2_PIX_FMT_SBGGR8
#define VIN_DEFAULT_WIDTH	640
#define VIN_DEFAULT_HEIGHT	480
#define VIN_DEFAULT_FIELD	V4L2_FIELD_NONE
#define VIN_DEFAULT_COLORSPACE	V4L2_COLORSPACE_SRGB

/* -----------------------------------------------------------------------------
 * Format Conversions
 */

static const struct vin_video_format vin_formats[] = {
	/* YUY2 */
	{
		.fourcc			= V4L2_PIX_FMT_YUYV,
		.mbus_code		= MEDIA_BUS_FMT_YUYV8_2X8,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_YVYU,
		.mbus_code		= MEDIA_BUS_FMT_YVYU8_2X8,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_UYVY,
		.mbus_code		= MEDIA_BUS_FMT_UYVY8_2X8,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_VYUY,
		.mbus_code		= MEDIA_BUS_FMT_VYUY8_2X8,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_YUYV,
		.mbus_code		= MEDIA_BUS_FMT_YUYV8_1X16,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_YVYU,
		.mbus_code		= MEDIA_BUS_FMT_YVYU8_1X16,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_UYVY,
		.mbus_code		= MEDIA_BUS_FMT_UYVY8_1X16,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_VYUY,
		.mbus_code		= MEDIA_BUS_FMT_VYUY8_1X16,
		.bpp			= 16,
		.bpc			= 8,
	},
	/* RGB565 */
	{
		.fourcc			= V4L2_PIX_FMT_RGB565,
		.mbus_code		= MEDIA_BUS_FMT_RGB565_2X8_LE,
		.bpp			= 16,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_RGB565X,
		.mbus_code		= MEDIA_BUS_FMT_RGB565_2X8_BE,
		.bpp			= 16,
		.bpc			= 8,
	},
	/* RGB888 */
	{
		.fourcc			= V4L2_PIX_FMT_RGB24,
		.mbus_code		= MEDIA_BUS_FMT_RGB888_1X24,
		.bpp			= 24,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_BGR24,
		.mbus_code		= MEDIA_BUS_FMT_BGR888_1X24,
		.bpp			= 24,
		.bpc			= 8,
	},
	/* RAW8 */
	{
		.fourcc			= V4L2_PIX_FMT_SBGGR8,
		.mbus_code		= MEDIA_BUS_FMT_SBGGR8_1X8,
		.bpp			= 8,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGBRG8,
		.mbus_code		= MEDIA_BUS_FMT_SGBRG8_1X8,
		.bpp			= 8,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGRBG8,
		.mbus_code		= MEDIA_BUS_FMT_SGRBG8_1X8,
		.bpp			= 8,
		.bpc			= 8,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SRGGB8,
		.mbus_code		= MEDIA_BUS_FMT_SRGGB8_1X8,
		.bpp			= 8,
		.bpc			= 8,
	},

	/* RAW10 */
	{
		.fourcc			= V4L2_PIX_FMT_SBGGR10,
		.mbus_code		= MEDIA_BUS_FMT_SBGGR10_1X10,
		.bpp			= 16,
		.bpc			= 10,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGBRG10,
		.mbus_code		= MEDIA_BUS_FMT_SGBRG10_1X10,
		.bpp			= 16,
		.bpc			= 10,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGRBG10,
		.mbus_code		= MEDIA_BUS_FMT_SGRBG10_1X10,
		.bpp			= 16,
		.bpc			= 10,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SRGGB10,
		.mbus_code		= MEDIA_BUS_FMT_SRGGB10_1X10,
		.bpp			= 16,
		.bpc			= 10,
	},

	/* RAW10 packed mode */
	{
		.fourcc			= V4L2_PIX_FMT_SBGGR10P,
		.mbus_code		= MEDIA_BUS_FMT_SBGGR10_1X10,
		.bpp			= 10,
		.bpc			= 10,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGBRG10P,
		.mbus_code		= MEDIA_BUS_FMT_SGBRG10_1X10,
		.bpp			= 10,
		.bpc			= 10,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGRBG10P,
		.mbus_code		= MEDIA_BUS_FMT_SGRBG10_1X10,
		.bpp			= 10,
		.bpc			= 10,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SRGGB10P,
		.mbus_code		= MEDIA_BUS_FMT_SRGGB10_1X10,
		.bpp			= 10,
		.bpc			= 10,
	},

	/* RAW12 */
	{
		.fourcc			= V4L2_PIX_FMT_SBGGR12,
		.mbus_code		= MEDIA_BUS_FMT_SBGGR12_1X12,
		.bpp			= 16,
		.bpc			= 12,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGBRG12,
		.mbus_code		= MEDIA_BUS_FMT_SGBRG12_1X12,
		.bpp			= 16,
		.bpc			= 12,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGRBG12,
		.mbus_code		= MEDIA_BUS_FMT_SGRBG12_1X12,
		.bpp			= 16,
		.bpc			= 12,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SRGGB12,
		.mbus_code		= MEDIA_BUS_FMT_SRGGB12_1X12,
		.bpp			= 16,
		.bpc			= 12,
	},

	/* RAW12 packed mode */
	{
		.fourcc			= V4L2_PIX_FMT_SBGGR12P,
		.mbus_code		= MEDIA_BUS_FMT_SBGGR12_1X12,
		.bpp			= 12,
		.bpc			= 12,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGBRG12P,
		.mbus_code		= MEDIA_BUS_FMT_SGBRG12_1X12,
		.bpp			= 12,
		.bpc			= 12,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SGRBG12P,
		.mbus_code		= MEDIA_BUS_FMT_SGRBG12_1X12,
		.bpp			= 12,
		.bpc			= 12,
	},
	{
		.fourcc			= V4L2_PIX_FMT_SRGGB12P,
		.mbus_code		= MEDIA_BUS_FMT_SRGGB12_1X12,
		.bpp			= 12,
		.bpc			= 12,
	},
};

/*  Print Four-character-code (FOURCC) */
static char *fourcc_to_str(u32 fmt)
{
	static char code[5];

	code[0] = (unsigned char)(fmt & 0xff);
	code[1] = (unsigned char)((fmt >> 8) & 0xff);
	code[2] = (unsigned char)((fmt >> 16) & 0xff);
	code[3] = (unsigned char)((fmt >> 24) & 0xff);
	code[4] = '\0';

	return code;
}

const struct vin_video_format *vin_format_from_pixel(struct vin_dev *vin,
						     u32 pixelformat)
{
	int i;

	switch (pixelformat) {
	case V4L2_PIX_FMT_NV12:
		/*
		 * If NV12 is supported it's only supported on channels 0, 1, 4,
		 * 5, 8, 9, 12 and 13.
		 */
		if (!vin->info->nv12 || !(BIT(vin->id) & 0x3333))
			return NULL;
		break;
	default:
		break;
	}

	for (i = 0; i < ARRAY_SIZE(vin_formats); i++)
		if (vin_formats[i].fourcc == pixelformat)
			return vin_formats + i;

	return NULL;
}

static const struct vin_video_format *vin_find_format_by_fourcc(struct vin_dev *vin,
								unsigned int fourcc)
{
	unsigned int num_formats = vin->num_of_sd_formats;
	const struct vin_video_format *fmt;
	unsigned int i;

	for (i = 0; i < num_formats; i++) {
		fmt = vin->sd_formats[i];
		if (fmt->fourcc == fourcc)
			return fmt;
	}

	return NULL;
}

static u32 vin_format_bytesperline(struct vin_dev *vin,
				   struct v4l2_pix_format *pix)
{
	const struct vin_video_format *fmt;
	u32 align;

	fmt = vin_format_from_pixel(vin, pix->pixelformat);

	if (WARN_ON(!fmt))
		return -EINVAL;

	switch (pix->pixelformat) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
		align = 0x20;
		break;
	case V4L2_PIX_FMT_SBGGR10P:
	case V4L2_PIX_FMT_SBGGR12P:
		align = 0x01;
		break;
	default:
		align = 0x10;
		break;
	}

	if (V4L2_FIELD_IS_SEQUENTIAL(pix->field))
		align = 0x80;

	return ALIGN((pix->width * fmt->bpp) / 8, align);
}

static u32 vin_format_sizeimage(struct v4l2_pix_format *pix)
{
	switch (pix->pixelformat) {
	case V4L2_PIX_FMT_NV12:
		return pix->bytesperline * pix->height * 3 / 2;
	case V4L2_PIX_FMT_NV16:
		return pix->bytesperline * pix->height * 2;
	default:
		return pix->bytesperline * pix->height;
	}
}

static void vin_format_align(struct vin_dev *vin, struct v4l2_pix_format *pix)
{
	u32 walign;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	if (!vin_format_from_pixel(vin, pix->pixelformat))
		pix->pixelformat = VIN_DEFAULT_FORMAT;

	switch (pix->field) {
	case V4L2_FIELD_TOP:
	case V4L2_FIELD_BOTTOM:
	case V4L2_FIELD_NONE:
	case V4L2_FIELD_INTERLACED_TB:
	case V4L2_FIELD_INTERLACED_BT:
	case V4L2_FIELD_INTERLACED:
	case V4L2_FIELD_ALTERNATE:
	case V4L2_FIELD_SEQ_TB:
	case V4L2_FIELD_SEQ_BT:
		break;
	default:
		pix->field = VIN_DEFAULT_FIELD;
		break;
	}

	/* HW limit width to a multiple of 32 (2^5) for NV12/16 else 2 (2^1) */
	switch (pix->pixelformat) {
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV16:
		walign = 5;
		break;
	default:
		walign = 1;
		break;
	}

	/* Limit to VIN capabilities */
	v4l_bound_align_image(&pix->width, 2, vin->info->max_width, walign,
			      &pix->height, 4, vin->info->max_height, 0, 0);

	pix->bytesperline = vin_format_bytesperline(vin, pix);
	pix->sizeimage = vin_format_sizeimage(pix);

	vin_dbg(vin, "Format %ux%u bpl: %u size: %u\n",
		pix->width, pix->height, pix->bytesperline, pix->sizeimage);
}

/* -----------------------------------------------------------------------------
 * V4L2
 */

#if !((defined(MIPI_CSI_BIST) || defined(MIPI_CSI_XTOR)) || defined(MIPI_CSI_QUALITY))
static int vin_find_pad(struct v4l2_subdev *sd, int direction)
{
	unsigned int pad;

	if (sd->entity.num_pads <= 1)
		return 0;

	for (pad = 0; pad < sd->entity.num_pads; pad++)
		if (sd->entity.pads[pad].flags & direction)
			return pad;

	return -EINVAL;
}
#endif

static int vin_try_format(struct vin_dev *vin, u32 which,
			  struct v4l2_pix_format *pix,
			  struct v4l2_rect *src_rect)
{
	struct v4l2_subdev_state *sd_state;
	static struct lock_class_key key;
	struct v4l2_subdev_format format = {
		.which = which,
	};
	const struct vin_video_format *fmt;
	struct v4l2_subdev *sd;
	struct media_pad *pad;
	enum v4l2_field field;
	u32 width, height;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	dev_dbg(vin->dev, "%s, pad->entity->name: %s, pad->index: %d\n",
		__func__, pad->entity->name, pad->index);

	sd = media_entity_to_v4l2_subdev(pad->entity);

	/*
	 * FIXME: Drop this call, drivers are not supposed to use
	 * __v4l2_subdev_state_alloc().
	 */
	sd_state = __v4l2_subdev_state_alloc(sd, "vin:state->lock", &key);
	if (IS_ERR(sd_state))
		return PTR_ERR(sd_state);

	dev_dbg(vin->dev, "%s, pad_cfg: code: 0x%04x %ux%u\n",
		__func__, sd_state->pads->try_fmt.code, sd_state->pads->try_fmt.width, sd_state->pads->try_fmt.height);
	dev_dbg(vin->dev, "%s, pix->pixelformat: %s\n", __func__, fourcc_to_str(pix->pixelformat));

	if (!vin_format_from_pixel(vin, pix->pixelformat))
		pix->pixelformat = VIN_DEFAULT_FORMAT;

#if defined(MIPI_CSI_BIST) || defined(MIPI_CSI_XTOR) || defined(MIPI_CSI_QUALITY)
	format.pad = pad->index;		/* Set the source pad for BIST mode */
#else
	/* Get the sink pad index of CSI */
	ret = vin_find_pad(sd, MEDIA_PAD_FL_SINK);
	format.pad = ret < 0 ? 0 : ret;
#endif

	dev_dbg(vin->dev, "%s, ret: %d, format.pad: %d\n", __func__, ret, format.pad);

	/* Get mbut code from pixel format*/
	fmt = vin_format_from_pixel(vin, pix->pixelformat);
	vin->mbus_code = fmt->mbus_code;
	vin->vin_fmt = fmt;
	v4l2_fill_mbus_format(&format.format, pix, vin->mbus_code);

	dev_dbg(vin->dev, "%s, format.format.code: 0x%04x\n", __func__, format.format.code);

	/* Allow the video device to override field and to scale */
	field = pix->field;
	width = pix->width;
	height = pix->height;

	ret = v4l2_subdev_call(sd, pad, set_fmt, sd_state, &format);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		goto done;
	ret = 0;

	dev_dbg(vin->dev, "%s, format.format.code: 0x%04x\n", __func__, format.format.code);

	v4l2_fill_pix_format(pix, &format.format);

	if (src_rect) {
		src_rect->top = 0;
		src_rect->left = 0;
		src_rect->width = pix->width;
		src_rect->height = pix->height;
	}

	//if (field != V4L2_FIELD_ANY)
	//	pix->field = field;

	//pix->width = width;
	//pix->height = height;

	vin_format_align(vin, pix);

done:
	__v4l2_subdev_state_free(sd_state);

	return ret;
}

static int vin_querycap(struct file *file, void *priv,
			struct v4l2_capability *cap)
{
	struct vin_dev *vin = video_drvdata(file);

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	strscpy(cap->driver, KBUILD_MODNAME, sizeof(cap->driver));
	strscpy(cap->card, "SP_VIN", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(vin->dev));
	return 0;
}

static int vin_try_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct vin_dev *vin = video_drvdata(file);

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	return vin_try_format(vin, V4L2_SUBDEV_FORMAT_TRY, &f->fmt.pix, NULL);
}

static int vin_s_fmt_vid_cap(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct vin_dev *vin = video_drvdata(file);
	struct v4l2_rect fmt_rect, src_rect;
	const struct vin_video_format *sd_fmt;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	dev_dbg(vin->dev, "%s, pixelformat: %s\n", __func__, fourcc_to_str(f->fmt.pix.pixelformat));

	if (vb2_is_busy(&vin->queue))
		return -EBUSY;

	/* Check if the camera supports this format */
	sd_fmt = vin_find_format_by_fourcc(vin, f->fmt.pix.pixelformat);
	if (!sd_fmt)
		return -EINVAL;

	ret = vin_try_format(vin, V4L2_SUBDEV_FORMAT_ACTIVE, &f->fmt.pix,
			     &src_rect);
	if (ret)
		return ret;

	vin->fmt = *f;
	vin->sd_format = sd_fmt;
	vin->sd_framesize.width = f->fmt.pix.width;
	vin->sd_framesize.height = f->fmt.pix.height;
	vin->format = f->fmt.pix;

	fmt_rect.top = 0;
	fmt_rect.left = 0;
	fmt_rect.width = vin->format.width;
	fmt_rect.height = vin->format.height;

	v4l2_rect_map_inside(&vin->crop, &src_rect);
	v4l2_rect_map_inside(&vin->compose, &fmt_rect);
	vin->src_rect = src_rect;

	return 0;
}

static int vin_g_fmt_vid_cap(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct vin_dev *vin = video_drvdata(file);

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	f->fmt.pix = vin->format;

	return 0;
}

static int vin_g_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct vin_dev *vin = video_drvdata(file);
	struct v4l2_subdev_frame_interval fi;
	struct v4l2_subdev *sd;
	struct media_pad *pad;
	int ret;

	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(&fi, 0, sizeof(fi));

	sd = media_entity_to_v4l2_subdev(pad->entity);
	ret = v4l2_subdev_call(sd, video, g_frame_interval, &fi);
	if (ret < 0)
		return ret;

	a->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.capture.timeperframe = fi.interval;

	return 0;
}

static int vin_s_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct vin_dev *vin = video_drvdata(file);
	struct v4l2_subdev_frame_interval fi;
	struct v4l2_subdev *sd;
	struct media_pad *pad;
	int ret;

	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(&fi, 0, sizeof(fi));
	fi.interval = a->parm.capture.timeperframe;
	sd = media_entity_to_v4l2_subdev(pad->entity);
	ret = v4l2_subdev_call(sd, video, s_frame_interval, &fi);
	if (ret < 0)
		return ret;
	a->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.capture.timeperframe = fi.interval;

	return 0;
}

static int vin_enum_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_fmtdesc *f)
{
	struct vin_dev *vin = video_drvdata(file);

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	dev_dbg(vin->dev, "%s, f->index: 0x%04x\n", __func__, f->index);
	dev_dbg(vin->dev, "%s, f->mbus_code: 0x%04x\n", __func__, f->mbus_code);

	if (f->index >= vin->num_of_sd_formats)
		return -EINVAL;

	f->pixelformat = vin->sd_formats[f->index]->fourcc;
	return 0;
}

static int vin_enum_input(struct file *file, void *priv,
			  struct v4l2_input *i)
{
	pr_info("%s, %d", __func__, __LINE__);

	if (i->index != 0)
		return -EINVAL;

	i->type = V4L2_INPUT_TYPE_CAMERA;
	strscpy(i->name, "Camera", sizeof(i->name));
	return 0;
}

static int vin_g_input(struct file *file, void *priv, unsigned int *i)
{
	pr_info("%s, %d", __func__, __LINE__);

	*i = 0;
	return 0;
}

static int vin_s_input(struct file *file, void *priv, unsigned int i)
{
	pr_info("%s, %d", __func__, __LINE__);

	if (i > 0)
		return -EINVAL;
	return 0;
}

static int vin_enum_framesizes(struct file *file, void *fh,
			       struct v4l2_frmsizeenum *fsize)
{
	struct vin_dev *vin = video_drvdata(file);
	struct media_pad *pad;
	struct v4l2_subdev *subdev;
	const struct vin_video_format *sd_fmt;
	struct v4l2_subdev_frame_size_enum fse = {
		.index = fsize->index,
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	int ret;

	sd_fmt = vin_find_format_by_fourcc(vin, fsize->pixel_format);
	if (!sd_fmt)
		return -EINVAL;

	fse.code = sd_fmt->mbus_code;

	/* Use media pad to get v4l2 subdevice */
	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	dev_dbg(vin->dev, "%s, pad->entity->name: %s, pad->index: %d\n",
		__func__, pad->entity->name, pad->index);

	subdev = media_entity_to_v4l2_subdev(pad->entity);

	ret = v4l2_subdev_call(subdev, pad, enum_frame_size,
			       NULL, &fse);
	if (ret)
		return ret;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = fse.max_width;
	fsize->discrete.height = fse.max_height;

	return 0;
}

static int vin_enum_frameintervals(struct file *file, void *fh,
				   struct v4l2_frmivalenum *fival)
{
	struct vin_dev *vin = video_drvdata(file);
	struct media_pad *pad;
	struct v4l2_subdev *subdev;
	const struct vin_video_format *sd_fmt;
	struct v4l2_subdev_frame_interval_enum fie = {
		.index = fival->index,
		.width = fival->width,
		.height = fival->height,
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};
	int ret;

	sd_fmt = vin_find_format_by_fourcc(vin, fival->pixel_format);
	if (!sd_fmt)
		return -EINVAL;

	fie.code = sd_fmt->mbus_code;

	/* Use media pad to get v4l2 subdevice */
	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	dev_dbg(vin->dev, "%s, pad->entity->name: %s, pad->index: %d\n",
		__func__, pad->entity->name, pad->index);

	subdev = media_entity_to_v4l2_subdev(pad->entity);

	ret = v4l2_subdev_call(subdev, pad,
			       enum_frame_interval, NULL, &fie);
	if (ret)
		return ret;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete = fie.interval;

	return 0;
}

static int vin_subscribe_event(struct v4l2_fh *fh,
			       const struct v4l2_event_subscription *sub)
{
	pr_info("%s, %d", __func__, __LINE__);

	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_event_subscribe(fh, sub, 4, NULL);
	}
	return v4l2_ctrl_subscribe_event(fh, sub);
}

static const struct v4l2_ioctl_ops vin_ioctl_ops = {
	.vidioc_querycap		= vin_querycap,
	.vidioc_try_fmt_vid_cap		= vin_try_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= vin_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= vin_s_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap	= vin_enum_fmt_vid_cap,

	.vidioc_enum_input		= vin_enum_input,
	.vidioc_g_input			= vin_g_input,
	.vidioc_s_input			= vin_s_input,

	.vidioc_enum_framesizes		= vin_enum_framesizes,
	.vidioc_enum_frameintervals	= vin_enum_frameintervals,
	.vidioc_g_parm				= vin_g_parm,
	.vidioc_s_parm				= vin_s_parm,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,

	.vidioc_log_status		= v4l2_ctrl_log_status,
	.vidioc_subscribe_event		= vin_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

/* -----------------------------------------------------------------------------
 * V4L2 Media Controller
 */

#if defined(MC_MODE_DEFAULT)
static int vin_mc_try_format_default(struct vin_dev *vin, u32 which,
				     struct v4l2_pix_format *pix,
				     struct v4l2_rect *src_rect)
{
	struct v4l2_subdev_pad_config *pad_cfg;
	struct v4l2_subdev_format format = {
		.which = which,
	};
	const struct vin_video_format *fmt;
	struct v4l2_subdev *sd;
	struct media_pad *pad;
	enum v4l2_field field;
	u32 width, height;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	dev_dbg(vin->dev, "%s, pad->index: %d\n", __func__, pad->index);

	sd = media_entity_to_v4l2_subdev(pad->entity);

	pad_cfg = v4l2_subdev_alloc_pad_config(sd);
	if (!pad_cfg)
		return -ENOMEM;

	dev_dbg(vin->dev, "%s, pix->pixelformat: %s\n", __func__, fourcc_to_str(pix->pixelformat));

	if (!vin_format_from_pixel(vin, pix->pixelformat))
		pix->pixelformat = VIN_DEFAULT_FORMAT;

	/* Get mbut code from pixel format*/
	fmt = vin_format_from_pixel(vin, pix->pixelformat);
	vin->mbus_code = fmt->mbus_code;
	format.pad = pad->index;
	v4l2_fill_mbus_format(&format.format, pix, vin->mbus_code);

	dev_dbg(vin->dev, "%s, format.format.code: 0x%04x\n", __func__, format.format.code);

	/* Allow the video device to override field and to scale */
	field = pix->field;
	width = pix->width;
	height = pix->height;

	ret = v4l2_subdev_call(sd, pad, set_fmt, pad_cfg, &format);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		goto done;
	ret = 0;

	dev_dbg(vin->dev, "%s, format.format.code: 0x%04x\n", __func__, format.format.code);

	v4l2_fill_pix_format(pix, &format.format);

	if (src_rect) {
		src_rect->top = 0;
		src_rect->left = 0;
		src_rect->width = pix->width;
		src_rect->height = pix->height;
	}

	if (field != V4L2_FIELD_ANY)
		pix->field = field;

	pix->width = width;
	pix->height = height;

	vin_format_align(vin, pix);
done:
	v4l2_subdev_free_pad_config(pad_cfg);

	return ret;
}
#endif

static void vin_mc_try_format(struct vin_dev *vin,
			      struct v4l2_pix_format *pix)
{
	const struct vin_video_format *fmt;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	/*
	 * The V4L2 specification clearly documents the colorspace fields
	 * as being set by drivers for capture devices. Using the values
	 * supplied by userspace thus wouldn't comply with the API. Until
	 * the API is updated force fixed values.
	 */
	pix->colorspace = VIN_DEFAULT_COLORSPACE;
	pix->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(pix->colorspace);
	pix->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(pix->colorspace);
	pix->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(true, pix->colorspace,
							  pix->ycbcr_enc);
	fmt = vin_format_from_pixel(vin, pix->pixelformat);
	vin->mbus_code = fmt->mbus_code;
	vin->vin_fmt = fmt;
	vin_format_align(vin, pix);
}

static int vin_mc_try_fmt_vid_cap(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	struct vin_dev *vin = video_drvdata(file);

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	vin_mc_try_format(vin, &f->fmt.pix);

	return 0;
}

static int vin_mc_s_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct vin_dev *vin = video_drvdata(file);
#if defined(MC_MODE_DEFAULT)
	int mode = 1;
#endif

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	dev_dbg(vin->dev, "%s, pixelformat: %s\n", __func__, fourcc_to_str(f->fmt.pix.pixelformat));

	if (vb2_is_busy(&vin->queue))
		return -EBUSY;

#if defined(MC_MODE_DEFAULT)
	if (mode) {
		//struct vin_dev *vin = video_drvdata(file);
		struct v4l2_rect fmt_rect, src_rect;
		int ret;

		//if (vb2_is_busy(&vin->queue))
		//	return -EBUSY;

		ret = vin_mc_try_format_default(vin, V4L2_SUBDEV_FORMAT_ACTIVE, &f->fmt.pix,
						&src_rect);
		if (ret)
			return ret;

		vin->format = f->fmt.pix;

		fmt_rect.top = 0;
		fmt_rect.left = 0;
		fmt_rect.width = vin->format.width;
		fmt_rect.height = vin->format.height;

		v4l2_rect_map_inside(&vin->crop, &src_rect);
		v4l2_rect_map_inside(&vin->compose, &fmt_rect);
		vin->src_rect = src_rect;
	} else {
		vin_mc_try_format(vin, &f->fmt.pix);

		vin->format = f->fmt.pix;

		vin->crop.top = 0;
		vin->crop.left = 0;
		vin->crop.width = vin->format.width;
		vin->crop.height = vin->format.height;
		vin->compose = vin->crop;
	}
#else
	vin_mc_try_format(vin, &f->fmt.pix);

	vin->format = f->fmt.pix;

	vin->crop.top = 0;
	vin->crop.left = 0;
	vin->crop.width = vin->format.width;
	vin->crop.height = vin->format.height;
	vin->compose = vin->crop;
#endif

	return 0;
}

static int vin_mc_enum_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	struct vin_dev *vin = video_drvdata(file);
	unsigned int i;
	int matched;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	dev_dbg(vin->dev, "%s, f->index: 0x%04x\n", __func__, f->index);
	dev_dbg(vin->dev, "%s, f->mbus_code: 0x%04x\n", __func__, f->mbus_code);

	/*
	 * If mbus_code is set only enumerate supported pixel formats for that
	 * bus code. Converting from YCbCr to RGB and RGB to YCbCr is possible
	 * with VIN, so all supported YCbCr and RGB media bus codes can produce
	 * all of the related pixel formats. If mbus_code is not set enumerate
	 * all possible pixelformats.
	 *
	 * TODO: Once raw MEDIA_BUS_FMT_SRGGB12_1X12 format is added to the
	 * driver this needs to be extended so raw media bus code only result in
	 * raw pixel format.
	 */
	switch (f->mbus_code) {
	case 0:
	case MEDIA_BUS_FMT_YUYV8_1X16:
	case MEDIA_BUS_FMT_UYVY8_1X16:
	case MEDIA_BUS_FMT_UYVY8_2X8:
	case MEDIA_BUS_FMT_RGB565_2X8_LE:
	case MEDIA_BUS_FMT_RGB565_2X8_BE:
	case MEDIA_BUS_FMT_UYVY10_2X10:
	case MEDIA_BUS_FMT_RGB888_1X24:
	case MEDIA_BUS_FMT_BGR888_1X24:
		break;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		if (f->index)
			return -EINVAL;
		f->pixelformat = V4L2_PIX_FMT_SBGGR8;
		return 0;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		if (f->index)
			return -EINVAL;
		f->pixelformat = V4L2_PIX_FMT_SGBRG8;
		return 0;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		if (f->index)
			return -EINVAL;
		f->pixelformat = V4L2_PIX_FMT_SGRBG8;
		return 0;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		if (f->index)
			return -EINVAL;
		f->pixelformat = V4L2_PIX_FMT_SRGGB8;
		return 0;
	default:
		return -EINVAL;
	}

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	matched = -1;
	for (i = 0; i < ARRAY_SIZE(vin_formats); i++) {
		//dev_dbg(vin->dev, "i: %d, vin_formats[i].fourcc: %s\n", i,
		//	fourcc_to_str(vin_formats[i].fourcc));

		if (vin_format_from_pixel(vin, vin_formats[i].fourcc))
			matched++;

		//dev_dbg(vin->dev, "matched: %d, f->index: %d\n", matched, f->index);

		if (matched == f->index) {
			f->pixelformat = vin_formats[i].fourcc;
			dev_dbg(vin->dev, "matched: %d, f->pixelformat: %s\n",
				matched, fourcc_to_str(f->pixelformat));

			return 0;
		}
	}

	return -EINVAL;
}

static const struct v4l2_ioctl_ops vin_mc_ioctl_ops = {
	.vidioc_querycap		= vin_querycap,
	.vidioc_try_fmt_vid_cap		= vin_mc_try_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= vin_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= vin_mc_s_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap	= vin_mc_enum_fmt_vid_cap,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,

	.vidioc_log_status		= v4l2_ctrl_log_status,
	.vidioc_subscribe_event		= vin_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

/* -----------------------------------------------------------------------------
 * File Operations
 */

static int vin_open(struct file *file)
{
	struct vin_dev *vin = video_drvdata(file);
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

#ifdef CONFIG_PM_RUNTIME_MIPICSI
	ret = pm_runtime_get_sync(vin->dev);
	if (ret < 0) {
		pm_runtime_put_noidle(vin->dev);
		return ret;
	}
#endif

	ret = mutex_lock_interruptible(&vin->lock);
	if (ret)
		goto err_pm;

	file->private_data = vin;

	ret = v4l2_fh_open(file);
	if (ret)
		goto err_unlock;

	ret = v4l2_pipeline_pm_get(&vin->vdev.entity);

	if (ret < 0)
		goto err_open;

	ret = v4l2_ctrl_handler_setup(&vin->ctrl_handler);
	if (ret)
		goto err_power;

	mutex_unlock(&vin->lock);

	return 0;
err_power:
	v4l2_pipeline_pm_put(&vin->vdev.entity);
err_open:
	v4l2_fh_release(file);
err_unlock:
	mutex_unlock(&vin->lock);
err_pm:
#ifdef CONFIG_PM_RUNTIME_MIPICSI
	pm_runtime_put(vin->dev);
#endif

	return ret;
}

static int vin_release(struct file *file)
{
	struct vin_dev *vin = video_drvdata(file);
	bool fh_singular;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	mutex_lock(&vin->lock);

	/* Save the singular status before we call the clean-up helper */
	fh_singular = v4l2_fh_is_singular_file(file);

	/* the release helper will cleanup any on-going streaming */
	ret = _vb2_fop_release(file, NULL);

	v4l2_pipeline_pm_put(&vin->vdev.entity);

	mutex_unlock(&vin->lock);

#ifdef CONFIG_PM_RUNTIME_MIPICSI
	pm_runtime_put(vin->dev);
#endif

	return ret;
}

static const struct v4l2_file_operations vin_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= video_ioctl2,
	.open		= vin_open,
	.release	= vin_release,
	.poll		= vb2_fop_poll,
	.mmap		= vb2_fop_mmap,
	.read		= vb2_fop_read,
};

void vin_v4l2_unregister(struct vin_dev *vin)
{
	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	if (!video_is_registered(&vin->vdev))
		return;

	v4l2_info(&vin->v4l2_dev, "Removing %s\n",
		  video_device_node_name(&vin->vdev));

	/* Checks internally if vdev have been init or not */
	video_unregister_device(&vin->vdev);
}

static void vin_notify(struct v4l2_subdev *sd,
		       unsigned int notification, void *arg)
{
	struct vin_dev *vin =
		container_of(sd->v4l2_dev, struct vin_dev, v4l2_dev);

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	switch (notification) {
	case V4L2_DEVICE_NOTIFY_EVENT:
		v4l2_event_queue(&vin->vdev, arg);
		break;
	default:
		break;
	}
}

#if defined(MIPI_CSI_BIST) || defined(MIPI_CSI_XTOR) || defined(MIPI_CSI_QUALITY)
int vin_v4l2_formats_init(struct vin_dev *vin)
{
	const struct vin_video_format *sd_fmts[ARRAY_SIZE(vin_formats)];
	unsigned int num_fmts = 0, i, j, k;
	u32 bist_fmt[2] = {MEDIA_BUS_FMT_YUYV8_2X8, MEDIA_BUS_FMT_SBGGR12_1X12};

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	for (k = 0; k < ARRAY_SIZE(bist_fmt); k++) {
		for (i = 0; i < ARRAY_SIZE(vin_formats); i++) {
			if (vin_formats[i].mbus_code != bist_fmt[k])
				continue;

			/* Exclude JPEG if BT656 bus is selected */
			//if (vin_formats[i].fourcc == V4L2_PIX_FMT_JPEG &&
			//    vin->bus_type == V4L2_MBUS_BT656)
			//	continue;

			/* Code supported, have we got this fourcc yet? */
			for (j = 0; j < num_fmts; j++)
				if (sd_fmts[j]->fourcc ==
						vin_formats[i].fourcc) {
					/* Already available */
					dev_dbg(vin->dev, "Skipping fourcc/code: %4.4s/0x%x\n",
						(char *)&sd_fmts[j]->fourcc,
						bist_fmt[k]);
					break;
				}
			if (j == num_fmts) {
				/* New */
				sd_fmts[num_fmts++] = vin_formats + i;
				dev_dbg(vin->dev, "Supported fourcc/code: %4.4s/0x%x\n",
					(char *)&sd_fmts[num_fmts - 1]->fourcc,
					sd_fmts[num_fmts - 1]->mbus_code);
			}
		}
	}

	if (!num_fmts)
		return -ENXIO;

	vin->num_of_sd_formats = num_fmts;
	vin->sd_formats = devm_kcalloc(vin->dev,
				       num_fmts, sizeof(struct vin_video_format *),
				       GFP_KERNEL);
	if (!vin->sd_formats) {
		dev_err(vin->dev, "Could not allocate memory\n");
		return -ENOMEM;
	}

	memcpy(vin->sd_formats, sd_fmts,
	       num_fmts * sizeof(struct vin_video_format *));
	vin->sd_format = vin->sd_formats[0];

	return 0;
}

int vin_v4l2_framesizes_init(struct vin_dev *vin)
{
	unsigned int num_fsize = 2;
	unsigned int i;
	struct vin_video_framesize bist_fsizes[2] = {{1280, 720}, {1920, 1080}};

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	vin->num_of_sd_framesizes = num_fsize;
	vin->sd_framesizes = devm_kcalloc(vin->dev, num_fsize,
					  sizeof(struct vin_video_framesize),
					  GFP_KERNEL);
	if (!vin->sd_framesizes) {
		dev_err(vin->dev, "Could not allocate memory\n");
		return -ENOMEM;
	}

	/* Fill array with sensor supported framesizes */
	dev_dbg(vin->dev, "Sensor supports %u frame sizes:\n", num_fsize);
	for (i = 0; i < vin->num_of_sd_framesizes; i++) {
		vin->sd_framesizes[i].width = bist_fsizes[i].width;
		vin->sd_framesizes[i].height = bist_fsizes[i].height;
		dev_dbg(vin->dev, "%ux%u\n",
			vin->sd_framesizes[i].width, vin->sd_framesizes[i].height);
	}

	return 0;
}

#else
int vin_v4l2_formats_init(struct vin_dev *vin)
{
	const struct vin_video_format *sd_fmts[ARRAY_SIZE(vin_formats)];
	unsigned int num_fmts = 0, i, j;
	struct v4l2_subdev *subdev;
	struct media_pad *pad;
	struct v4l2_subdev_mbus_code_enum mbus_code = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
	};

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	dev_dbg(vin->dev, "%s, pad->entity->name: %s, pad->index: %d\n",
		__func__, pad->entity->name, pad->index);

	subdev = media_entity_to_v4l2_subdev(pad->entity);

	while (!v4l2_subdev_call(subdev, pad, enum_mbus_code,
				 NULL, &mbus_code)) {
		dev_dbg(vin->dev, "%s, %d, mbus_code.code: 0x%x\n",
			__func__, __LINE__, mbus_code.code);

		for (i = 0; i < ARRAY_SIZE(vin_formats); i++) {
			if (vin_formats[i].mbus_code != mbus_code.code)
				continue;

			/* Exclude JPEG if BT656 bus is selected */
			//if (vin_formats[i].fourcc == V4L2_PIX_FMT_JPEG &&
			//    vin->bus_type == V4L2_MBUS_BT656)
			//	continue;

			/* Code supported, have we got this fourcc yet? */
			for (j = 0; j < num_fmts; j++)
				if (sd_fmts[j]->fourcc ==
						vin_formats[i].fourcc) {
					/* Already available */
					dev_dbg(vin->dev, "Skipping fourcc/code: %4.4s/0x%x\n",
						(char *)&sd_fmts[j]->fourcc,
						mbus_code.code);
					break;
				}
			if (j == num_fmts) {
				/* New */
				sd_fmts[num_fmts++] = vin_formats + i;
				dev_dbg(vin->dev, "Supported fourcc/code: %4.4s/0x%x\n",
					(char *)&sd_fmts[num_fmts - 1]->fourcc,
					sd_fmts[num_fmts - 1]->mbus_code);
			}
		}
		mbus_code.index++;
	}

	if (!num_fmts)
		return -ENXIO;

	vin->num_of_sd_formats = num_fmts;
	vin->sd_formats = devm_kcalloc(vin->dev, num_fmts,
				       sizeof(struct vin_video_format *),
				       GFP_KERNEL);
	if (!vin->sd_formats) {
		dev_err(vin->dev, "Could not allocate memory\n");
		return -ENOMEM;
	}

	memcpy(vin->sd_formats, sd_fmts,
	       num_fmts * sizeof(struct vin_video_format *));
	vin->sd_format = vin->sd_formats[0];

	return 0;
}

int vin_v4l2_framesizes_init(struct vin_dev *vin)
{
	unsigned int num_fsize = 0;
	struct v4l2_subdev *subdev;
	struct media_pad *pad;
	struct v4l2_subdev_frame_size_enum fse = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
		.code = vin->sd_format->mbus_code,
	};
	unsigned int ret;
	unsigned int i;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	pad = media_pad_remote_pad_first(&vin->pad);
	if (!pad)
		return -EPIPE;

	dev_dbg(vin->dev, "%s, pad->entity->name: %s, pad->index: %d\n",
		__func__, pad->entity->name, pad->index);

	subdev = media_entity_to_v4l2_subdev(pad->entity);

	/* Allocate discrete framesizes array */
	while (!v4l2_subdev_call(subdev, pad, enum_frame_size,
				 NULL, &fse))
		fse.index++;

	num_fsize = fse.index;
	if (!num_fsize)
		return 0;

	vin->num_of_sd_framesizes = num_fsize;
	vin->sd_framesizes = devm_kcalloc(vin->dev, num_fsize,
					  sizeof(struct vin_video_framesize),
					  GFP_KERNEL);
	if (!vin->sd_framesizes) {
		dev_err(vin->dev, "Could not allocate memory\n");
		return -ENOMEM;
	}

	/* Fill array with sensor supported framesizes */
	dev_dbg(vin->dev, "Sensor supports %u frame sizes:\n", num_fsize);
	for (i = 0; i < vin->num_of_sd_framesizes; i++) {
		fse.index = i;
		ret = v4l2_subdev_call(subdev, pad, enum_frame_size,
				       NULL, &fse);
		if (ret)
			return ret;
		vin->sd_framesizes[fse.index].width = fse.max_width;
		vin->sd_framesizes[fse.index].height = fse.max_height;
		dev_dbg(vin->dev, "%ux%u\n", fse.max_width, fse.max_height);
	}

	return 0;
}
#endif

int vin_v4l2_set_default_fmt(struct vin_dev *vin)
{
	struct v4l2_rect fmt_rect, src_rect;
	struct v4l2_format f = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.fmt.pix = {
			.width = 352,
			.height = 288,
			.field = V4L2_FIELD_NONE,
			.pixelformat = vin->sd_formats[0]->fourcc,
		},
	};
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);
	dev_dbg(vin->dev, "%s, pixelformat: %s\n",
		__func__, fourcc_to_str(f.fmt.pix.pixelformat));

	ret = vin_try_format(vin, V4L2_SUBDEV_FORMAT_ACTIVE, &f.fmt.pix,
			     &src_rect);
	if (ret)
		return ret;

	vin->fmt = f;
	vin->sd_format = vin->sd_formats[0];
	vin->sd_framesize.width = f.fmt.pix.width;
	vin->sd_framesize.height = f.fmt.pix.height;
	vin->format = f.fmt.pix;

	fmt_rect.top = 0;
	fmt_rect.left = 0;
	fmt_rect.width = vin->format.width;
	fmt_rect.height = vin->format.height;

	v4l2_rect_map_inside(&vin->crop, &src_rect);
	v4l2_rect_map_inside(&vin->compose, &fmt_rect);
	vin->src_rect = src_rect;

	return 0;
}

int vin_v4l2_register(struct vin_dev *vin)
{
	struct video_device *vdev = &vin->vdev;
	int ret;

	dev_dbg(vin->dev, "%s, %d\n", __func__, __LINE__);

	vin->v4l2_dev.notify = vin_notify;

	/* video node */
	vdev->v4l2_dev = &vin->v4l2_dev;
	vdev->queue = &vin->queue;
	snprintf(vdev->name, sizeof(vdev->name), "VIN%u output", vin->id);
	vdev->release = video_device_release_empty;
	vdev->lock = &vin->lock;
	vdev->fops = &vin_fops;
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
		V4L2_CAP_READWRITE;

	/* Set a default format */
	vin->format.pixelformat	= VIN_DEFAULT_FORMAT;
	vin->format.width = VIN_DEFAULT_WIDTH;
	vin->format.height = VIN_DEFAULT_HEIGHT;
	vin->format.field = VIN_DEFAULT_FIELD;
	vin->format.colorspace = VIN_DEFAULT_COLORSPACE;

	if (vin->info->use_mc) {
		vdev->device_caps |= V4L2_CAP_IO_MC;
		vdev->ioctl_ops = &vin_mc_ioctl_ops;
	} else {
		vdev->device_caps |= V4L2_CAP_IO_MC;
		vdev->ioctl_ops = &vin_ioctl_ops;
	}

	vin_format_align(vin, &vin->format);

#if defined(MIPI_CSI_VIDEO_SEQ)
	ret = video_register_device(&vin->vdev, VFL_TYPE_VIDEO, -1);
#else
	ret = video_register_device(&vin->vdev, VFL_TYPE_VIDEO, VIDEO_NODE_OFFSET + vin->id);
#endif
	if (ret) {
		vin_err(vin, "Failed to register video device\n");
		return ret;
	}

	video_set_drvdata(&vin->vdev, vin);

	v4l2_info(&vin->v4l2_dev, "Device VIN%u registered as %s\n",
		  vin->id, video_device_node_name(&vin->vdev));

	return ret;
}
