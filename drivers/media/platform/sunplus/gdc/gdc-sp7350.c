// SPDX-License-Identifier: GPL-2.0

#include "gdc.h"

#define GDC_REG_ID (0x00)
#define GDC_REG_CONFIG_ADDR (0x10)
#define GDC_REG_CONFIG_SIZE (0x14)
#define GDC_REG_DATAIN_WIDTH (0x20)
#define GDC_REG_DATAIN_HEIGHT (0x24)
#define GDC_REG_DATA1IN_ADDR (0x28)
#define GDC_REG_DATA1IN_LINE_OFFSET (0x2C)
#define GDC_REG_DATA2IN_ADDR (0x30)
#define GDC_REG_DATA2IN_LINE_OFFSET (0x34)
#define GDC_REG_DATA3IN_ADDR (0x38)
#define GDC_REG_DATA3IN_LINE_OFFSET (0x3C)
#define GDC_REG_DATAOUT_WIDTH (0x40)
#define GDC_REG_DATAOUT_HEIGHT (0x44)
#define GDC_REG_DATA1OUT_ADDR (0x48)
#define GDC_REG_DATA1OUT_LINE_OFFSET (0x4C)
#define GDC_REG_DATA2OUT_ADDR (0x50)
#define GDC_REG_DATA2OUT_LINE_OFFSET (0x54)
#define GDC_REG_DATA3OUT_ADDR (0x58)
#define GDC_REG_DATA3OUT_LINE_OFFSET (0x5C)
#define GDC_REG_ERROR (0x60)
#define GDC_REG_CTRL (0x64)
#define GDC_REG_CAPABILITY (0x68)

static u32 __maybe_unused gdc_id_read(void __iomem *base)
{
	return readl(base + GDC_REG_ID);
}

static void gdc_config_addr_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_CONFIG_ADDR);
}

static u32 __maybe_unused gdc_config_addr_read(void __iomem *base)
{
	return readl(base + GDC_REG_CONFIG_ADDR);
}

static void gdc_config_size_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_CONFIG_SIZE);
}

static u32 __maybe_unused gdc_config_size_read(void __iomem *base)
{
	return readl(base + GDC_REG_CONFIG_SIZE);
}

static void gdc_datain_width_write(void __iomem *base, uint16_t data)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAIN_WIDTH);
	reg_value = ((u32)(data & 0xffff)) << 0 | (reg_value & 0xffff0000);

	writel(reg_value, base + GDC_REG_DATAIN_WIDTH);
}

static uint16_t __maybe_unused gdc_datain_width_read(void __iomem *base)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAIN_WIDTH);

	return (uint16_t)((reg_value & 0xffff) >> 0);
}

static void gdc_datain_height_write(void __iomem *base, uint16_t data)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAIN_HEIGHT);
	reg_value = ((u32)(data & 0xffff)) << 0 | (reg_value & 0xffff0000);

	writel(reg_value, base + GDC_REG_DATAIN_HEIGHT);
}

static uint16_t __maybe_unused gdc_datain_height_read(void __iomem *base)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAIN_HEIGHT);

	return (uint16_t)((reg_value & 0xffff) >> 0);
}

static void gdc_data1in_addr_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA1IN_ADDR);
}

static u32 __maybe_unused gdc_data1in_addr_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA1IN_ADDR);
}

static void gdc_data1in_line_offset_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA1IN_LINE_OFFSET);
}

static u32 __maybe_unused gdc_data1in_line_offset_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA1IN_LINE_OFFSET);
}

static void gdc_data2in_addr_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA2IN_ADDR);
}

static u32 __maybe_unused gdc_data2in_addr_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA2IN_ADDR);
}

static void gdc_data2in_line_offset_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA2IN_LINE_OFFSET);
}

static u32 __maybe_unused gdc_data2in_line_offset_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA2IN_LINE_OFFSET);
}

static void gdc_data3in_addr_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA3IN_ADDR);
}

static u32 __maybe_unused gdc_data3in_addr_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA3IN_ADDR);
}

static void gdc_data3in_line_offset_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA3IN_LINE_OFFSET);
}

static u32 __maybe_unused gdc_data3in_line_offset_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA3IN_LINE_OFFSET);
}

static void gdc_dataout_width_write(void __iomem *base, uint16_t data)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAOUT_WIDTH);
	reg_value = ((u32)(data & 0xffff)) << 0 | (reg_value & 0xffff0000);

	writel(reg_value, base + GDC_REG_DATAOUT_WIDTH);
}

static uint16_t __maybe_unused gdc_dataout_width_read(void __iomem *base)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAOUT_WIDTH);

	return (uint16_t)((reg_value & 0xffff) >> 0);
}

static void gdc_dataout_height_write(void __iomem *base, uint16_t data)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAOUT_HEIGHT);
	reg_value = ((u32)(data & 0xffff)) << 0 | (reg_value & 0xffff0000);

	writel(reg_value, base + GDC_REG_DATAOUT_HEIGHT);
}

static uint16_t __maybe_unused gdc_dataout_height_read(void __iomem *base)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_DATAOUT_HEIGHT);

	return (uint16_t)((reg_value & 0xffff) >> 0);
}

static void gdc_data1out_addr_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA1OUT_ADDR);
}

static u32 __maybe_unused gdc_data1out_addr_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA1OUT_ADDR);
}

static void gdc_data1out_line_offset_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA1OUT_LINE_OFFSET);
}

static u32 __maybe_unused gdc_data1out_line_offset_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA1OUT_LINE_OFFSET);
}

static void gdc_data2out_addr_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA2OUT_ADDR);
}

static u32 __maybe_unused gdc_data2out_addr_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA2OUT_ADDR);
}

static void gdc_data2out_line_offset_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA2OUT_LINE_OFFSET);
}

static u32 __maybe_unused gdc_data2out_line_offset_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA2OUT_LINE_OFFSET);
}

static void gdc_data3out_addr_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA3OUT_ADDR);
}

static u32 __maybe_unused gdc_data3out_addr_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA3OUT_ADDR);
}

static void gdc_data3out_line_offset_write(void __iomem *base, u32 data)
{
	writel(data, base + GDC_REG_DATA3OUT_LINE_OFFSET);
}

static u32 __maybe_unused gdc_data3out_line_offset_read(void __iomem *base)
{
	return readl(base + GDC_REG_DATA3OUT_LINE_OFFSET);
}

static void gdc_start_flag_write(void __iomem *base, uint8_t data)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_CTRL);
	reg_value = ((u32)(data & 0x1)) << 0 | (reg_value & 0xfffffffe);

	writel(reg_value, base + GDC_REG_CTRL);
}

static uint8_t __maybe_unused gdc_start_flag_read(void __iomem *base)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_CTRL);

	return (uint8_t)((reg_value & BIT(0)) >> 0);
}

static uint8_t __maybe_unused gdc_axi_data_width_read(void __iomem *base)
{
	u32 reg_value;

	reg_value = readl(base + GDC_REG_CAPABILITY);

	return (uint8_t)((reg_value & GENMASK(29, 27)) >> 27);
}

static int gdc_plane_set(int index, void __iomem *reg_base, u32 in_addr,
			 u32 out_addr, u32 in_lineoffset, u32 out_lineoffset)
{
	switch (index) {
	case 0:
		gdc_data1in_addr_write(reg_base, in_addr);
		gdc_data1in_line_offset_write(reg_base, in_lineoffset);
		gdc_data1out_addr_write(reg_base, out_addr);
		gdc_data1out_line_offset_write(reg_base, out_lineoffset);

		break;
	case 1:
		gdc_data2in_addr_write(reg_base, in_addr);
		gdc_data2in_line_offset_write(reg_base, in_lineoffset);
		gdc_data2out_addr_write(reg_base, out_addr);
		gdc_data2out_line_offset_write(reg_base, out_lineoffset);

		break;
	case 2:
		gdc_data3in_addr_write(reg_base, in_addr);
		gdc_data3in_line_offset_write(reg_base, in_lineoffset);
		gdc_data3out_addr_write(reg_base, out_addr);
		gdc_data3out_line_offset_write(reg_base, out_lineoffset);

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int gdc_run(struct gdc_ctx *ctx, struct vb2_buffer *in_buf,
	    struct vb2_buffer *out_buf)
{
	struct gdc_device *gdev = ctx->gdev;
	u8 nplanes;
	u32 in_lineoffset, out_lineoffset;
	u32 in_height, out_height;
	u32 in_addr, out_addr;
	int i;

	if (ctx->in_frame.fmt->fourcc != ctx->out_frame.fmt->fourcc) {
		dev_err(ctx->gdev->dev,
			"capture pixel format not match output pixel format\n");
		return -EINVAL;
	}

	if (in_buf->num_planes != out_buf->num_planes) {
		dev_err(gdev->dev,
			"number of input planes not equals to number of output planes[%d-%d]\n",
			in_buf->num_planes, out_buf->num_planes);
		return -EINVAL;
	}

	nplanes = in_buf->num_planes;

	if (nplanes > GDC_MAX_PLANES) {
		dev_err(gdev->dev, "nPlanes:%d exceed limitation(%d)\n",
			nplanes, GDC_MAX_PLANES);
		return -EINVAL;
	}

	if (nplanes > 1 && nplanes != ctx->out_frame.fmt->nplanes) {
		dev_err(gdev->dev,
			"number of planes not match the pixel format[%d-%d]\n",
			nplanes, ctx->out_frame.fmt->nplanes);
		return -EINVAL;
	}

	gdc_start_flag_write(gdev->reg_base, 0);

	gdc_config_addr_write(gdev->reg_base, ctx->config_dma_addr);
	gdc_config_size_write(gdev->reg_base, ctx->config_size >> 2);

	gdc_datain_width_write(gdev->reg_base, ctx->in_frame.width);
	gdc_datain_height_write(gdev->reg_base, ctx->in_frame.height);
	gdc_dataout_width_write(gdev->reg_base, ctx->out_frame.width);
	gdc_dataout_height_write(gdev->reg_base, ctx->out_frame.height);

	if (nplanes == 1) {
		//single-planar
		in_addr = (u32)*((dma_addr_t *)vb2_plane_cookie(in_buf, 0));
		out_addr = (u32)*((dma_addr_t *)vb2_plane_cookie(out_buf, 0));
		for (i = 0; i < ctx->out_frame.fmt->nplanes; i++) {
			if (i == 0) {
				in_lineoffset = ctx->in_frame.width;
				in_height = ctx->in_frame.height;
				out_lineoffset = ctx->out_frame.width;
				out_height = ctx->out_frame.height;
			} else {
				in_lineoffset = ctx->in_frame.width >>
						(ctx->in_frame.fmt->w_div - 1);
				in_height = ctx->in_frame.height >>
					    (ctx->in_frame.fmt->h_div - 1);
				out_lineoffset =
					ctx->out_frame.width >>
					(ctx->out_frame.fmt->w_div - 1);
				out_height = ctx->out_frame.height >>
					     (ctx->out_frame.fmt->h_div - 1);
			}

			gdc_plane_set(i, gdev->reg_base, in_addr, out_addr,
				      in_lineoffset, out_lineoffset);

			in_addr += in_height * in_lineoffset;
			out_addr += out_height * out_lineoffset;
		}
	} else {
		// multi-planar
		for (i = 0; i < nplanes; i++) {
			in_addr = (u32)*
				  ((dma_addr_t *)vb2_plane_cookie(in_buf, i));
			out_addr = (u32)*
				   ((dma_addr_t *)vb2_plane_cookie(out_buf, i));
			if (i == 0) {
				in_lineoffset = ctx->in_frame.width;
				out_lineoffset = ctx->out_frame.width;
			} else {
				in_lineoffset = ctx->in_frame.width >>
						(ctx->in_frame.fmt->w_div - 1);
				out_lineoffset =
					ctx->out_frame.width >>
					(ctx->out_frame.fmt->w_div - 1);
			}

			gdc_plane_set(i, gdev->reg_base, in_addr, out_addr,
				      in_lineoffset, out_lineoffset);
		}
	}

	gdc_start_flag_write(gdev->reg_base, 1);

	return 0;
}
