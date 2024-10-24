/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __GDC_SUNPLUS_H
#define __GDC_SUNPLUS_H

#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>

#define GDC_DRIVER_NAME "sunplus-gdc"
#define GDC_DEV_NAME "sunplus-gdc"
#define GDC_BUS_NAME "platform:gdc"
#define GDC_DEV_NODE_NUM -1
#define GDC_DEFAULT_WIDTH 1920
#define GDC_DEFAULT_HEIGHT 1080
#define GDC_MAX_PLANES 3

struct gdc_fmt {
	u32 fourcc;
	int depth;
	u8 w_div;
	u8 h_div;
	u8 nplanes;
};

struct gdc_frame {
	struct gdc_fmt *fmt;
	u32 width;
	u32 height;
	u32 colorspace;
	u32 bytesperline;
	u32 size;
};

struct gdc_ctx {
	struct v4l2_fh fh;
	struct gdc_device *gdev;
	struct gdc_frame in_frame;
	struct gdc_frame out_frame;
	void *config_addr;
	dma_addr_t config_dma_addr;
	size_t config_size;
	size_t max_config_size;
};

struct gdc_v4l2 {
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	struct v4l2_m2m_dev *m2m_dev;
	struct mutex lock;
	spinlock_t spin_lock;
	struct gdc_ctx *curr_ctx;
	int devnode_number;
};

struct gdc_device {
	struct device *dev;
	struct gdc_v4l2 v4l2;
	struct clk *sclk;
	struct clk *aclk;
	struct clk *pclk;
	struct reset_control *srst;
	struct reset_control *arst;
	struct reset_control *prst;
	struct regulator *supply;
	struct regulator *iso;
	int irq;
	u32 max_config_size;
	void __iomem *reg_base;
};

struct gdc_frame *gdc_get_frame(struct gdc_ctx *ctx, enum v4l2_buf_type type);
int gdc_run(struct gdc_ctx *ctx, struct vb2_buffer *in_buf,
	    struct vb2_buffer *out_buf);

extern const struct vb2_ops gdc_qops;

#endif
