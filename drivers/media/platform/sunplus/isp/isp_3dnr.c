// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP3DNR
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <linux/delay.h>

#include "isp_3dnr.h"
//#include "isp_debugfs.h"

#define CREATE_TRACE_POINTS
#include "isp_3dnr_trace.h"

#define ISP_3DNR_DEBUG_MESG (false)
#define ISP_3DNR_DEBUG_POLL_MESG (false)

#define ISP_3DNR_MAX_VIDEO_DEVICE (6)

#define ISP_3DNR_DEVICE_BASE (112)

#if 1 // Reduce the buffer size to 128KB to fix the alloc failure
#define ISP_3DNR_MOT_MAX_SIZE (0x100000) //(0x20000)
#else
#define ISP_3DNR_MOT_MAX_SIZE (0x500000) //(2688 * 1944) //2688*1944*8 bits
#endif

#define ISP_3DNR_PAGE_MASK (~(PAGE_SIZE - 1))
#define ISP_3DNR_PAGE_ALIGN(x) ((x + PAGE_SIZE - 1) & ISP_3DNR_PAGE_MASK)

#define ISP_3DNR_MONI_Q_FRAME_NUM (20)
enum moniNRType
{
	MONI_3DNR_CAP = 0,
	MONI_3DNR_TYPE_MAX
};
enum moniNRStatus
{
	MONI_3DNR_INT = 0,
	MONI_3DNR_Q_BUFF,
	MONI_3DNR_HW_DOING_IN,
	MONI_3DNR_HW_DOING_OUT,
	MONI_3DNR_HW_DONE,
	MONI_3DNR_DQ_BUFF,
	MONI_3DNR_STATUS_MAX
};

//       frame 0  1  2  3  4  5
// cap(0)
// data val: 0:init  1:q  2:hw doning_in  3:hw doning_out  4:hw done  5:dq
static int monitor3DNR_Q[ISP_3DNR_MAX_VIDEO_DEVICE][MONI_3DNR_TYPE_MAX]
						[ISP_3DNR_MONI_Q_FRAME_NUM] = {MONI_3DNR_INT};

struct list_head *last_buff_ptr[ISP_3DNR_MAX_VIDEO_DEVICE] = {NULL};

static int video_3dnr_nr = -1;

struct isp_3dnr_video_fh
{
	struct v4l2_fh vfh;
	struct isp_3dnr_video *video;
	struct vb2_queue queue;
	struct v4l2_format format;	   // for m2m caputure
	struct v4l2_format out_format; // for m2m output
	struct v4l2_fract timeperframe;
};

#define to_isp_3dnr_video_fh(fh) container_of(fh, struct isp_3dnr_video_fh, vfh)

struct isp_3dnr_video
{
	struct isp_3dnr_video_fh *video_fh;
	struct video_device vdev;
	enum v4l2_buf_type type;
	struct isp_3dnr_device *isp_dev;
	/* Video buffers queue */
	struct vb2_queue *queue;
};

static struct isp_3dnr_video g_3dnr_videos[ISP_3DNR_MAX_VIDEO_DEVICE];

struct isp_3dnr_buffer
{
	struct vb2_v4l2_buffer vb;
	struct list_head irqlist;
	dma_addr_t dma;
};

#define to_isp_3dnr_buffer(buf) container_of(buf, struct isp_3dnr_buffer, vb)

struct isp_3dnr_framesizes
{
	u32 fourcc;
	struct v4l2_frmsize_stepwise stepwise;
};

static const struct isp_3dnr_framesizes isp_3dnr_framesizes_arr[] = {
	{
		.fourcc = V4L2_PIX_FMT_NV12M,
		.stepwise = {10, 5000, 1, 10, 5000, 1},
	},
	{
		.fourcc = V4L2_PIX_FMT_NV24,
		.stepwise = {10, 5000, 1, 10, 5000, 1},
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR10,
		.stepwise = {10, 5000, 1, 10, 5000, 1},
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR12,
		.stepwise = {10, 5000, 1, 10, 5000, 1},
	},
};

int isp_3dnr_get_enough_frame(int dev_id)
{
	// 0 : not enough, 1:enough
	struct vb2_buffer *isp_3dnr_buf = NULL;
	struct vb2_queue *q = NULL;

	if (dev_id < 0 || dev_id >= ISP_3DNR_MAX_VIDEO_DEVICE)
	{
		printk("(isp) isp_3dnr dev_id=%d is wrong %s(%d)    \n", dev_id,
			   __func__, __LINE__);
		return -1;
	}

	if (g_3dnr_videos[dev_id].video_fh == NULL)
	{
		return -1;
	}

	q = &(g_3dnr_videos[dev_id].video_fh->queue);
	isp_3dnr_buf = vb2_get_buffer(q, 0);

	if (isp_3dnr_buf == NULL)
	{
		if (ISP_3DNR_DEBUG_MESG)
			printk("(isp) isp 3dnr: dev_id = %d, frame is not enough %s(%d)    \n",
				   dev_id, __func__, __LINE__);
		trace_isp_3dnr_frame_not_enough(dev_id);
		return 0;
	}
	else
	{
		if (ISP_3DNR_DEBUG_MESG)
			printk("(isp) isp 3dnr: dev_id = %d, frame is enough %s(%d)    \n",
				   dev_id, __func__, __LINE__);
		return 1;
	}
}

int isp_3dnr_get_first_frame(int dev_id)
{
	// 0 : not first frame, 1:first frame
	if (dev_id < 0 || dev_id >= ISP_3DNR_MAX_VIDEO_DEVICE)
	{
		printk("(isp) isp_3dnr dev_id=%d is wrong %s(%d)    \n", dev_id,
			   __func__, __LINE__);
		return -1;
	}

	if (g_3dnr_videos[dev_id].isp_dev == NULL)
	{
		return -1;
	}

	trace_isp_3dnr_first_or_not(dev_id,
								g_3dnr_videos[dev_id].isp_dev->first_frame);

	return g_3dnr_videos[dev_id].isp_dev->first_frame;
}

int isp_3dnr_get_device_alive(int dev_id)
{
	// 0 : not alive, 1:alive
	if (dev_id < 0 || dev_id >= ISP_3DNR_MAX_VIDEO_DEVICE)
	{
		printk("(isp) isp_3dnr dev_id=%d is wrong %s(%d)    \n", dev_id,
			   __func__, __LINE__);
		return -1;
	}

	if (g_3dnr_videos[dev_id].isp_dev == NULL)
	{
		return -1;
	}

	trace_isp_3dnr_dev_alive(dev_id,
							 g_3dnr_videos[dev_id].isp_dev->device_alive);

	return g_3dnr_videos[dev_id].isp_dev->device_alive;
}

void isp_3dnr_get_buff(int dev_id, struct isp_3dnr_ret *ret)
{
	struct vb2_queue *q = NULL;
	dma_addr_t mot_phy_addr = 0;
	int mot_read_idx = 0;
	struct vb2_buffer *entry = NULL;
	struct vb2_buffer *entry_2 = NULL;
	struct list_head *head = NULL;
	struct list_head *listptr = NULL;
	int buff_count = 0;

	if (dev_id < 0 || dev_id >= ISP_3DNR_MAX_VIDEO_DEVICE)
	{
		printk("(isp) isp_3dnr dev_id=%d is wrong %s(%d)    \n", dev_id,
			   __func__, __LINE__);
		return;
	}

	if (g_3dnr_videos[dev_id].video_fh == NULL)
	{
		return;
	}

	if (ret == NULL)
	{
		printk("(isp) isp_3dnr dev_id=%d ret ptr is NULL %s(%d)    \n",
			   dev_id, __func__, __LINE__);
		return;
	}

	ret->in_buff = NULL;
	ret->out_buff = NULL;

	mot_phy_addr = g_3dnr_videos[dev_id].isp_dev->d3nr_mot_buff.phy_addr;
	mot_read_idx = g_3dnr_videos[dev_id].isp_dev->d3nr_mot_buff.buff_idx;

	q = &(g_3dnr_videos[dev_id].video_fh->queue);

	if (q == NULL)
	{
		printk("(isp) isp_3dnr dev_id=%d q ptr is NULL %s(%d) \n",
			   dev_id, __func__, __LINE__);
		return;
	}

	head = &(q->queued_list);
	buff_count = atomic_read(&q->owned_by_drv_count);

	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) get buff, dev_id=%d ,buff_count=%d   %s(%d)\n",
			   dev_id, buff_count, __func__, __LINE__);
	trace_isp_3dnr_buf_count(dev_id, buff_count);

	if (g_3dnr_videos[dev_id].isp_dev->in_out_swap != SWAP_3DNR_NONE)
	{
		struct vb2_buffer *buf_temp =
			NULL; // temp buffer for input/output buffer swapping

		if (ISP_3DNR_DEBUG_MESG)
			printk("(isp) isp 3dnr: swap frame, swap count=%d %s(%d) \n",
				   g_3dnr_videos[dev_id].isp_dev->in_out_swap,
				   __func__, __LINE__);

		// for y uv
		buf_temp = g_3dnr_videos[dev_id].isp_dev->input_buf;
		g_3dnr_videos[dev_id].isp_dev->input_buf =
			g_3dnr_videos[dev_id].isp_dev->output_buf;
		g_3dnr_videos[dev_id].isp_dev->output_buf = buf_temp;

		if (head->next == NULL)
			printk("(isp) isp 3dnr: get next link list node failed %s(%d) \n",
				   __func__, __LINE__);
		else
		{
			if (g_3dnr_videos[dev_id].isp_dev->in_out_swap ==
				SWAP_3DNR_ODD)
				last_buff_ptr[dev_id] = head->next;
			else
				last_buff_ptr[dev_id] = head->next->next;
		}

		if (last_buff_ptr[dev_id] == NULL)
			printk("(isp) isp 3dnr: get last link list node failed %s(%d) \n",
				   __func__, __LINE__);
		// for 3dnr mot
		if (mot_read_idx == 0)
		{
			ret->in_mot_phy_addr = mot_phy_addr;
			ret->out_mot_phy_addr =
				mot_phy_addr + ISP_3DNR_MOT_MAX_SIZE;
		}
		else
		{
			ret->in_mot_phy_addr =
				mot_phy_addr + ISP_3DNR_MOT_MAX_SIZE;
			ret->out_mot_phy_addr = mot_phy_addr;
		}

		if (ISP_3DNR_DEBUG_MESG)
			printk("(isp) isp_3dnr get buf. swap case (device %d)(swap_count=%d)(buf_in_idx=%d)(buf_out_idx=%d)\n",
				   dev_id,
				   g_3dnr_videos[dev_id].isp_dev->in_out_swap,
				   g_3dnr_videos[dev_id].isp_dev->input_buf->index,
				   g_3dnr_videos[dev_id].isp_dev->output_buf->index);

		trace_isp_3dnr_buf_swap(
			dev_id, g_3dnr_videos[dev_id].isp_dev->in_out_swap,
			g_3dnr_videos[dev_id].isp_dev->input_buf->index,
			g_3dnr_videos[dev_id].isp_dev->output_buf->index);
	}
	else if (g_3dnr_videos[dev_id].isp_dev->first_frame == 1)
	{
		if (buff_count > 0)
		{
			// for 3dnr y / uv
			listptr = head->next;
			entry = list_entry(listptr, struct vb2_buffer,
							   queued_entry);

			last_buff_ptr[dev_id] = listptr;

			if ((entry == NULL) || (listptr == NULL))
			{
				printk("(isp) isp 3dnr: get first frame failed %s(%d)    \n",
					   __func__, __LINE__);

				g_3dnr_videos[dev_id].isp_dev->input_buf = NULL;
				g_3dnr_videos[dev_id].isp_dev->output_buf =
					NULL;

				ret->in_mot_phy_addr = 0;
				ret->out_mot_phy_addr = 0;
			}
			else
			{
				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp 3dnr: first frame %s(%d)    \n",
						   __func__, __LINE__);

				g_3dnr_videos[dev_id].isp_dev->input_buf = NULL;
				g_3dnr_videos[dev_id].isp_dev->output_buf =
					entry;

				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp 3dnr: id=%d  output_buf = 0x%px  %s(%d)\n",
						   dev_id,
						   g_3dnr_videos[dev_id]
							   .isp_dev->output_buf,
						   __func__, __LINE__);

				// for 3dnr mot
				ret->in_mot_phy_addr = 0;
				ret->out_mot_phy_addr = mot_phy_addr;

				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp_3dnr get buf. first frame case (device %d)(buf_out_idx=%d)\n",
						   dev_id,
						   g_3dnr_videos[dev_id]
							   .isp_dev->output_buf
							   ->index);

				trace_isp_3dnr_buf_first(
					dev_id,
					g_3dnr_videos[dev_id]
						.isp_dev->output_buf->index);
			}
		}
		else
		{
			if (ISP_3DNR_DEBUG_MESG)
				printk("(isp) isp 3dnr: no frame (device %d) %s(%d) \n",
					   dev_id, __func__, __LINE__);
			trace_isp_3dnr_buf_no_frame(dev_id);

			g_3dnr_videos[dev_id].isp_dev->input_buf = NULL;
			g_3dnr_videos[dev_id].isp_dev->output_buf = NULL;

			ret->in_mot_phy_addr = 0;
			ret->out_mot_phy_addr = 0;
		}
	}
	else
	{
		if (buff_count >= 2)
		{
			// for 3dnr y / uv
			if (last_buff_ptr[dev_id] == NULL)
			{
				printk("(isp) isp_3dnr dev_id=%d last buf ptr is NULL %s(%d)    \n",
					   dev_id, __func__, __LINE__);
				return;
			}

			listptr = last_buff_ptr[dev_id];
			entry = list_entry(listptr, struct vb2_buffer,
							   queued_entry);

			listptr = listptr->next;
			entry_2 = list_entry(listptr, struct vb2_buffer,
								 queued_entry);

			last_buff_ptr[dev_id] = listptr;

			if ((entry == NULL) || (entry_2 == NULL))
			{
				printk("(isp) isp 3dnr: get frame failed %s(%d)    \n",
					   __func__, __LINE__);
				g_3dnr_videos[dev_id].isp_dev->input_buf = NULL;
				g_3dnr_videos[dev_id].isp_dev->output_buf =
					NULL;
				ret->in_mot_phy_addr = 0;
				ret->out_mot_phy_addr = 0;
			}
			else
			{
				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp 3dnr: next frame %s(%d)    \n",
						   __func__, __LINE__);

				g_3dnr_videos[dev_id].isp_dev->input_buf =
					entry;
				g_3dnr_videos[dev_id].isp_dev->output_buf =
					entry_2;

				// for 3dnr mot
				if (mot_read_idx == 0)
				{
					ret->in_mot_phy_addr = mot_phy_addr;
					ret->out_mot_phy_addr =
						mot_phy_addr +
						ISP_3DNR_MOT_MAX_SIZE;
				}
				else
				{
					ret->in_mot_phy_addr =
						mot_phy_addr +
						ISP_3DNR_MOT_MAX_SIZE;
					ret->out_mot_phy_addr = mot_phy_addr;
				}

				if (ISP_3DNR_DEBUG_MESG)
				{
					printk("(isp) isp 3dnr: dev_id=%d  input_buf = 0x%px   %s(%d)\n",
						   dev_id,
						   g_3dnr_videos[dev_id]
							   .isp_dev->input_buf,
						   __func__, __LINE__);
					printk("(isp) isp 3dnr: dev_id=%d  output_buf = 0x%px  %s(%d)\n",
						   dev_id,
						   g_3dnr_videos[dev_id]
							   .isp_dev->output_buf,
						   __func__, __LINE__);
				}
				trace_isp_3dnr_buf_next_frame(
					dev_id,
					g_3dnr_videos[dev_id]
						.isp_dev->input_buf->index,
					g_3dnr_videos[dev_id]
						.isp_dev->output_buf->index);
			}
		}
		else
		{
			if (ISP_3DNR_DEBUG_MESG)
				printk("(isp) isp 3dnr: frame is not enough (dev_id=%d) %s(%d) \n",
					   dev_id, __func__, __LINE__);
			trace_isp_3dnr_buf_not_enough(dev_id);

			// for 3dnr y / uv
			g_3dnr_videos[dev_id].isp_dev->input_buf = NULL;
			g_3dnr_videos[dev_id].isp_dev->output_buf = NULL;

			// for 3dnr mot
			ret->in_mot_phy_addr = 0;
			ret->out_mot_phy_addr = 0;
		}
	}
	ret->in_buff = g_3dnr_videos[dev_id].isp_dev->input_buf;
	ret->out_buff = g_3dnr_videos[dev_id].isp_dev->output_buf;

	if (ret->in_buff != NULL)
		monitor3DNR_Q[dev_id][MONI_3DNR_CAP]
					 [g_3dnr_videos[dev_id].isp_dev->input_buf->index] =
						 MONI_3DNR_HW_DOING_IN;

	if (ret->out_buff != NULL)
		monitor3DNR_Q[dev_id][MONI_3DNR_CAP]
					 [g_3dnr_videos[dev_id].isp_dev->output_buf->index] =
						 MONI_3DNR_HW_DOING_OUT;

	if (ISP_3DNR_DEBUG_MESG)
	{
		if (ret->in_buff == NULL)
			printk("(isp) isp 3dnr: get in_buff is NULL %s(%d) \n",
				   __func__, __LINE__);
		else
			printk("(isp) isp 3dnr: get in_buff is 0x%px dev_id=%d in_idx=%d  %s(%d) \n",
				   ret->in_buff, dev_id,
				   g_3dnr_videos[dev_id].isp_dev->input_buf->index,
				   __func__, __LINE__);

		if (ret->out_buff == NULL)
			printk("(isp) isp 3dnr: get out_buff is NULL %s(%d) \n",
				   __func__, __LINE__);
		else
			printk("(isp) isp 3dnr: get out_buff is 0x%px dev_id=%d out_idx=%d  %s(%d) \n",
				   ret->out_buff, dev_id,
				   g_3dnr_videos[dev_id].isp_dev->output_buf->index,
				   __func__, __LINE__);
	}
}

void isp_3dnr_hw_done(int dev_id)
{
	int mot_read_idx = 0;
	struct vb2_queue *q = NULL;
	int buff_count = 0;

	if (dev_id < 0 || dev_id >= ISP_3DNR_MAX_VIDEO_DEVICE)
	{
		printk("(isp) isp_3dnr dev_id=%d is wrong %s(%d)    \n", dev_id,
			   __func__, __LINE__);
		return;
	}

	if (g_3dnr_videos[dev_id].video_fh == NULL)
	{
		return;
	}

	mot_read_idx = g_3dnr_videos[dev_id].isp_dev->d3nr_mot_buff.buff_idx;
	q = &(g_3dnr_videos[dev_id].video_fh->queue);
	if (q == NULL)
	{
		printk("(isp) isp_3dnr dev_id=%d q ptr is NULL %s(%d)    \n",
			   dev_id, __func__, __LINE__);
		return;
	}
	buff_count = atomic_read(&q->owned_by_drv_count);

	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp 3dnr: hw done, dev_id=%d , buff_count=%d  %s(%d)\n",
			   dev_id, buff_count, __func__, __LINE__);
	trace_isp_3dnr_done_count(dev_id, buff_count);

	if (g_3dnr_videos[dev_id].isp_dev->output_buf != NULL)
	{
		if (g_3dnr_videos[dev_id].isp_dev->first_frame == 1)
		{
			g_3dnr_videos[dev_id].isp_dev->in_out_swap =
				SWAP_3DNR_NONE;

			// check buffer enough
			if (buff_count > 1)
			{
				// enough
				g_3dnr_videos[dev_id].isp_dev->first_frame = 0;
				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp 3dnr: dev_id=%d hw done, first frame %s(%d)\n",
						   dev_id, __func__, __LINE__);
				trace_isp_3dnr_done_first_enough(dev_id,
												 buff_count);
			}
			else
			{
				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp 3dnr: hw done, first frame not enough case %s(%d)    \n",
						   __func__, __LINE__);
				trace_isp_3dnr_done_first_not_enough(
					dev_id, buff_count);
			}
		}
		else if (g_3dnr_videos[dev_id].isp_dev->input_buf != NULL)
		{
			// check buffer enough
			if (buff_count > 2)
			{
				if (g_3dnr_videos[dev_id].isp_dev->in_out_swap !=
					SWAP_3DNR_NONE)
				{
					if (ISP_3DNR_DEBUG_MESG)
						printk("(isp) isp 3dnr: dev_id=%d  swap_count=%d  hw done, in_out_swap off %s(%d)\n",
							   dev_id,
							   g_3dnr_videos[dev_id]
								   .isp_dev
								   ->in_out_swap,
							   __func__, __LINE__);
					trace_isp_3dnr_done_swap_off(
						dev_id,
						g_3dnr_videos[dev_id]
							.isp_dev->in_out_swap);

					if (g_3dnr_videos[dev_id]
							.isp_dev->in_out_swap ==
						SWAP_3DNR_EVEN)
					{
						vb2_buffer_done(
							g_3dnr_videos[dev_id]
								.isp_dev
								->input_buf,
							VB2_BUF_STATE_DONE);

						if (ISP_3DNR_DEBUG_MESG)
							printk("(isp) 3dnr done dev_id=%d index =%d  %s(%d) \n",
								   dev_id,
								   g_3dnr_videos[dev_id]
									   .isp_dev
									   ->input_buf
									   ->index,
								   __func__,
								   __LINE__);

						monitor3DNR_Q
							[dev_id][MONI_3DNR_CAP]
							[g_3dnr_videos[dev_id]
								 .isp_dev
								 ->input_buf
								 ->index] =
								MONI_3DNR_HW_DONE;

						g_3dnr_videos[dev_id]
							.isp_dev->input_buf =
							NULL;
					}

					g_3dnr_videos[dev_id]
						.isp_dev->in_out_swap =
						SWAP_3DNR_NONE;
				}
				else
				{
					// enough
					if (ISP_3DNR_DEBUG_MESG)
						printk("(isp) isp 3dnr: dev_id=%d  done_buf = 0x%px  in_buff_idx=%d  hw done, next frame %s(%d)\n",
							   dev_id,
							   g_3dnr_videos[dev_id]
								   .isp_dev
								   ->input_buf,
							   g_3dnr_videos[dev_id]
								   .isp_dev
								   ->input_buf
								   ->index,
							   __func__, __LINE__);
					trace_isp_3dnr_done_buf_release(
						dev_id,
						g_3dnr_videos[dev_id]
							.isp_dev->input_buf
							->index);

					vb2_buffer_done(
						g_3dnr_videos[dev_id]
							.isp_dev->input_buf,
						VB2_BUF_STATE_DONE);

					if (ISP_3DNR_DEBUG_MESG)
						printk("(isp) 3dnr done dev_id=%d index =%d  %s(%d) \n",
							   dev_id,
							   g_3dnr_videos[dev_id]
								   .isp_dev
								   ->input_buf
								   ->index,
							   __func__, __LINE__);

					monitor3DNR_Q[dev_id][MONI_3DNR_CAP]
								 [g_3dnr_videos[dev_id]
									  .isp_dev
									  ->input_buf
									  ->index] =
									 MONI_3DNR_HW_DONE;
					g_3dnr_videos[dev_id]
						.isp_dev->input_buf = NULL;
				}
			}
			else
			{
				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp 3dnr: dev_id=%d hw done, in/out swap case %s(%d) \n",
						   dev_id, __func__, __LINE__);
				if (g_3dnr_videos[dev_id].isp_dev->in_out_swap ==
					SWAP_3DNR_ODD)
					g_3dnr_videos[dev_id]
						.isp_dev->in_out_swap =
						SWAP_3DNR_EVEN;
				else
					g_3dnr_videos[dev_id]
						.isp_dev->in_out_swap =
						SWAP_3DNR_ODD;

				if (ISP_3DNR_DEBUG_MESG)
					printk("(isp) isp 3dnr: dev_id=%d hw done, swap count=%d %s(%d) \n",
						   dev_id,
						   g_3dnr_videos[dev_id]
							   .isp_dev->in_out_swap,
						   __func__, __LINE__);
				trace_isp_3dnr_done_swap_on(
					dev_id,
					g_3dnr_videos[dev_id]
						.isp_dev->in_out_swap,
					buff_count);
			}
		}
		else
		{
			if (ISP_3DNR_DEBUG_MESG)
				printk("(isp) isp 3dnr: dev_id=%d hw done, other case %s(%d)    \n",
					   dev_id, __func__, __LINE__);
			trace_isp_3dnr_done_buf_error(dev_id);
		}

		// update 3dnr mot read idx
		g_3dnr_videos[dev_id].isp_dev->d3nr_mot_buff.buff_idx =
			(mot_read_idx + 1) % 2;
	}
	else
	{
		if (ISP_3DNR_DEBUG_MESG)
			printk("(isp) isp 3dnr: dev_id=%d hw done, no output frame %s(%d)    \n",
				   dev_id, __func__, __LINE__);
		trace_isp_3dnr_done_no_frame(dev_id);
	}

	if (ISP_3DNR_DEBUG_MESG)
		isp_3dnr_video_monitor_q_info();
}

void isp_3dnr_video_monitor_q_info(void)
{
#define PRINT_3DNR_DEV_NUM (4)

	int dev_idx = 0, q_idx = 0;

	if (PRINT_3DNR_DEV_NUM > ISP_3DNR_MAX_VIDEO_DEVICE)
		printk("monitor 3dnr q device num is too large\n");
	else
	{
		for (dev_idx = 0; dev_idx < PRINT_3DNR_DEV_NUM; dev_idx++)
		{
			printk("\n===3dnr device %d ===\n", dev_idx);
			for (q_idx = 0; q_idx < MONI_3DNR_TYPE_MAX; q_idx++)
			{
				// printk 4 frame status

				printk("===3dnr output queue ===\n");

				if (ISP_3DNR_MONI_Q_FRAME_NUM >= 6)
					printk("%d %d %d %d %d %d\n",
						   monitor3DNR_Q[dev_idx][q_idx][0],
						   monitor3DNR_Q[dev_idx][q_idx][1],
						   monitor3DNR_Q[dev_idx][q_idx][2],
						   monitor3DNR_Q[dev_idx][q_idx][3],
						   monitor3DNR_Q[dev_idx][q_idx][4],
						   monitor3DNR_Q[dev_idx][q_idx][5]);
			}
		}
	}
}

static int isp_3dnr_init_modules(struct platform_device *pdev,
								 struct isp_3dnr_device *isp_dev)
{
	return 0;
}

static int isp_3dnr_register_entities(struct isp_3dnr_device *isp)
{
	int ret;

	ret = v4l2_device_register(isp->dev, &isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: V4L2 device registration failed (%d)\n",
				__func__, ret);
		goto done;
	}

done:
	return 0;
}

static int isp_3dnr_capture_queue_setup(struct vb2_queue *queue,
										unsigned int *num_buffers,
										unsigned int *num_planes,
										unsigned int sizes[],
										struct device *alloc_devs[])
{
	struct isp_3dnr_video_fh *vfh = vb2_get_drv_priv(queue);
	int idx = 0;

	*num_planes = vfh->format.fmt.pix_mp.num_planes;
	for (idx = 0; idx < *num_planes; idx++)
	{
		sizes[idx] = vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage;
	}
	return 0;
}

static void isp_3dnr_capture_buffer_queue(struct vb2_buffer *buf)
{
}

static void isp_3dnr_stop_streaming(struct vb2_queue *q)
{
	if (q != NULL)
	{
		// clean buffer
		int i = 0;
		for (i = 0; i < q->num_buffers; ++i)
			if (q->bufs[i]->state == VB2_BUF_STATE_ACTIVE)
			{
				vb2_buffer_done(q->bufs[i],
								VB2_BUF_STATE_ERROR);
			}
	}
}

static const struct vb2_ops isp_video_queue_ops = {
	.queue_setup = isp_3dnr_capture_queue_setup,
	.buf_queue = isp_3dnr_capture_buffer_queue,
	.stop_streaming = isp_3dnr_stop_streaming,
	// .start_streaming = isp_video_start_streaming,
};

static int isp_3dnr_video_open(struct file *file)
{
	struct isp_3dnr_video *video = video_drvdata(file);
	struct isp_3dnr_video_fh *handle = NULL;
	struct vb2_queue *queue;
	int ret;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	handle->video = video;
	video->video_fh = handle;

	file->private_data = &handle->vfh;

	// setup queue
	queue = &handle->queue;
	queue->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	queue->io_modes = VB2_MMAP | VB2_DMABUF;
	queue->drv_priv = handle;
	queue->ops = &isp_video_queue_ops;
	queue->mem_ops = &vb2_dma_contig_memops;
	queue->buf_struct_size = sizeof(struct isp_3dnr_buffer);
	queue->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	queue->dev = video->isp_dev->dev;
	video->vdev.queue = queue;

	ret = vb2_queue_init(&handle->queue);
	if (ret < 0)
	{
		dev_err(queue->dev, "vb2_queue_init failed\n");
		goto done;
	}
	return 0;

done:
	if (ret < 0)
		kfree(handle);

	return ret;
}

static int isp_3dnr_video_querycap(struct file *file, void *priv,
								   struct v4l2_capability *cap)
{
	strscpy(cap->driver, "sp-isp3dnr", sizeof(cap->driver));
	strscpy(cap->card, "sp-isp3dnr", sizeof(cap->card));
	strscpy(cap->bus_info, "sp-isp3dnr", sizeof(cap->bus_info));
	return 0;
}

/* crop */
static int isp_3dnr_g_selection(struct file *file, void *fh,
								struct v4l2_selection *sel)
{
	// struct v4l2_subdev_format format;
	sel->r.left = 0;
	sel->r.top = 0;
	sel->r.width = 640;
	sel->r.height = 320;

	// sel->r.width = format.format.width;
	// sel->r.height = format.format.height;
	return 0;
}

static int isp_3dnr_s_selection(struct file *file, void *fh,
								struct v4l2_selection *sel)
{
	struct v4l2_subdev_selection sdsel = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
		.target = sel->target,
		.flags = sel->flags,
		.r = sel->r,
	};

	sel->r = sdsel.r;
	return 0;
}

static int isp_3dnr_get_cap_format(struct file *file, void *fh,
								   struct v4l2_format *format)
{
	return 0;
}

static int isp_3dnr_get_cap_format_mplane(struct file *file, void *fh,
										  struct v4l2_format *format)
{
	struct isp_3dnr_video_fh *vfh = to_isp_3dnr_video_fh(fh);
	int idx = 0;

	format->type = vfh->format.type;
	format->fmt.pix_mp.height = vfh->format.fmt.pix_mp.height;
	format->fmt.pix_mp.width = vfh->format.fmt.pix_mp.width;
	format->fmt.pix_mp.num_planes = vfh->format.fmt.pix_mp.num_planes;

	for (idx = 0; idx < format->fmt.pix_mp.num_planes; idx++)
	{
		format->fmt.pix_mp.plane_fmt[idx].sizeimage =
			vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage;

		format->fmt.pix_mp.plane_fmt[idx].bytesperline =
			vfh->format.fmt.pix_mp.plane_fmt[idx].bytesperline;
	}
	return 0;
}

static int isp_3dnr_set_cap_format_mplane(struct file *file, void *fh,
										  struct v4l2_format *format)
{
	struct isp_3dnr_video_fh *vfh = to_isp_3dnr_video_fh(fh);
	int idx = 0;
	struct v4l2_pix_format_mplane *mplane = NULL;

	if (format == NULL)
	{
		printk("%s(%d)  null pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (format->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
	{
		printk("%s(%d) wrong format type\n", __func__, __LINE__);
		return -EINVAL;
	}

	mplane = &format->fmt.pix_mp;

	if (mplane->num_planes < 1 || mplane->num_planes > 3)
	{
		printk("%s(%d) wrong format mplane num\n", __func__, __LINE__);
		return -EINVAL;
	}

	for (idx = 0; idx < mplane->num_planes; idx++)
	{
		if (mplane->plane_fmt[idx].sizeimage < 0 ||
			mplane->plane_fmt[idx].bytesperline < 0)
		{
			printk("%s(%d) wrong mplane(%d) size\n", __func__,
				   __LINE__, idx);
			return -EINVAL;
		}
	}

	vfh->format.type = format->type;
	vfh->format.fmt.pix_mp.width = mplane->width;
	vfh->format.fmt.pix_mp.height = mplane->height;
	vfh->format.fmt.pix_mp.num_planes = mplane->num_planes;

	if (ISP_3DNR_DEBUG_MESG)
	{
		printk("(isp) === cap set format mplane===\n");
		printk("(isp) cap w = 0x%x (%s)(%d)  \n",
			   vfh->format.fmt.pix_mp.width, __func__, __LINE__);
		printk("(isp) cap h = 0x%x (%s)(%d)  \n",
			   vfh->format.fmt.pix_mp.height, __func__, __LINE__);
		printk("(isp) cap plane num = %d (%s)(%d)  \n",
			   vfh->format.fmt.pix_mp.num_planes, __func__, __LINE__);
	}

	for (idx = 0; idx < vfh->format.fmt.pix_mp.num_planes; idx++)
	{
		vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage =
			mplane->plane_fmt[idx].sizeimage;

		vfh->format.fmt.pix_mp.plane_fmt[idx].bytesperline =
			mplane->plane_fmt[idx].bytesperline;

		if (ISP_3DNR_DEBUG_MESG)
		{
			printk("(isp) plane %d (%s)(%d)  \n", idx, __func__,
				   __LINE__);
			printk("(isp) plane size = 0x%x (%s)(%d)  \n",
				   vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage,
				   __func__, __LINE__);
			printk("(isp) plane bytesperline = 0x%x (%s)(%d)  \n",
				   vfh->format.fmt.pix_mp.plane_fmt[idx]
					   .bytesperline,
				   __func__, __LINE__);
		}
	}

	return 0;
}

static int isp_3dnr_video_set_format(struct file *file, void *fh,
									 struct v4l2_format *format)
{
	return 0;
}

static int isp_3dnr_video_try_format(struct file *file, void *fh,
									 struct v4l2_format *format)
{
	return 0;
}

static int isp_3dnr_video_streamon(struct file *file, void *fh,
								   enum v4l2_buf_type type)
{
	struct isp_3dnr_video_fh *vfh = to_isp_3dnr_video_fh(fh);
	int r = 0;
	int idx = 0;
	int device_id = 0;

	if (vfh == NULL)
	{
		printk("(isp) isp 3dnr vfh is NULL %s(%d)\n", __func__,
			   __LINE__);
		return -1;
	}

	device_id = vfh->video->isp_dev->dev_id;
	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp stream on start %s(%d)\n", __func__,
			   __LINE__);
	trace_isp_3dnr_stream_on_start(device_id);

	vfh->video->isp_dev->first_frame = 1;
	vfh->video->isp_dev->input_buf = NULL;
	vfh->video->isp_dev->output_buf = NULL;
	vfh->video->isp_dev->d3nr_mot_buff.buff_idx = 1;
	vfh->video->isp_dev->in_out_swap = SWAP_3DNR_NONE;
	vfh->video->isp_dev->device_alive = 1;

	if (device_id < ISP_3DNR_MAX_VIDEO_DEVICE)
		last_buff_ptr[device_id] = NULL;

	r = vb2_streamon(&vfh->queue, type);

	// reset monitor q status
	if (device_id < ISP_3DNR_MAX_VIDEO_DEVICE)
	{
		for (idx = 0; idx < ISP_3DNR_MONI_Q_FRAME_NUM; idx++)
		{
			monitor3DNR_Q[device_id][MONI_3DNR_CAP][idx] =
				MONI_3DNR_INT;
		}
	}

	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp stream on end %s(%d)\n", __func__, __LINE__);
	trace_isp_3dnr_stream_on_end(device_id);

	return r;
}

static int isp_3dnr_video_streamoff(struct file *file, void *fh,
									enum v4l2_buf_type type)
{
	struct video_device *vdev = video_devdata(file);
	unsigned int r = 0;
	int device_id = 0;
	struct isp_3dnr_video_fh *vfh = to_isp_3dnr_video_fh(fh);

	device_id = vfh->video->isp_dev->dev_id;
	vfh->video->isp_dev->device_alive = 0;
	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp stream off start (device %d) %s(%d)\n",
			   device_id, __func__, __LINE__);
	trace_isp_3dnr_stream_off_start(device_id);

	r = vb2_streamoff(vdev->queue, type);

	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp stream off end %s(%d)\n", __func__, __LINE__);
	trace_isp_3dnr_stream_off_end(device_id);

	return r;
}

static int isp_3dnr_video_qbuf(struct file *file, void *fh,
							   struct v4l2_buffer *buf)
{
	struct isp_3dnr_video_fh *vfh = to_isp_3dnr_video_fh(fh);
	unsigned int r = 0;
	int dev_id = vfh->video->isp_dev->dev_id;
	int buff_idx = buf->index;
	int type = MONI_3DNR_CAP;

	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp q start (device %d)(idx=%d) %s(%d)\n", dev_id,
			   buff_idx, __func__, __LINE__);
	trace_isp_3dnr_q_start(dev_id, buff_idx);

	r = vb2_ioctl_qbuf(file, fh, buf);

	if ((buff_idx < ISP_3DNR_MONI_Q_FRAME_NUM) &&
		(dev_id < ISP_3DNR_MAX_VIDEO_DEVICE))
		monitor3DNR_Q[dev_id][type][buff_idx] = MONI_3DNR_Q_BUFF;

	if (ISP_3DNR_DEBUG_MESG)
	{
		printk("(isp) isp q_0 (buf_idx=%d)(fd=%d)(size=%x) %s(%d)\n",
			   buff_idx, buf->m.planes[0].m.fd, buf->m.planes[0].length,
			   __func__, __LINE__);

		printk("(isp) isp q_1 (buf_idx=%d)(fd=%d)(size=%x) %s(%d)\n",
			   buff_idx, buf->m.planes[1].m.fd, buf->m.planes[1].length,
			   __func__, __LINE__);

		printk("(isp) isp q end (dev_id=%d)(buf_idx=%d) %s(%d)\n",
			   dev_id, buff_idx, __func__, __LINE__);
	}
	trace_isp_3dnr_q_end(dev_id, buff_idx);

	return r;
}

static int isp_3dnr_video_dqbuf(struct file *file, void *fh,
								struct v4l2_buffer *buf)
{
	struct isp_3dnr_video_fh *vfh = to_isp_3dnr_video_fh(fh);

	unsigned int r = 0;
	int idx = 0;
	int dev_id = vfh->video->isp_dev->dev_id;
	int buff_idx = 0;
	int type = MONI_3DNR_CAP;

	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp dq start (device %d) %s(%d)\n", dev_id,
			   __func__, __LINE__);
	trace_isp_3dnr_dq_start(dev_id);

	r = vb2_ioctl_dqbuf(file, fh, buf);

	if ((buf->index < ISP_3DNR_MONI_Q_FRAME_NUM) &&
		(dev_id < ISP_3DNR_MAX_VIDEO_DEVICE))
		monitor3DNR_Q[dev_id][type][buf->index] = MONI_3DNR_DQ_BUFF;

	buff_idx = buf->index;

	for (idx = 0; idx < buf->length; idx++)
		buf->m.planes[idx].bytesused = buf->m.planes[idx].length;

	if (ISP_3DNR_DEBUG_MESG)
		printk("(isp) isp dq end (idx=%d) %s(%d)\n", buf->index,
			   __func__, __LINE__);
	trace_isp_3dnr_dq_end(dev_id, buff_idx);

	return r;
}

static __poll_t isp_3dnr_poll(struct file *file, struct poll_table_struct *wait)
{
	struct video_device *vdev = video_devdata(file);
	return vb2_poll(vdev->queue, file, wait);
}

static int isp_3dnr_video_release(struct file *file)
{
	struct v4l2_fh *vfh = file->private_data;
	struct isp_3dnr_video_fh *handle = to_isp_3dnr_video_fh(vfh);
	if (handle == NULL)
	{
		printk("(isp) isp 3dnr handle is NULL %s(%d)\n", __func__,
			   __LINE__);
		return -1;
	}
	kfree(handle);
	file->private_data = NULL;
	return 0;
}

static int isp_3dnr_video_enum_cap_fmt(struct file *file, void *priv,
									   struct v4l2_fmtdesc *f)
{
	if (f->index == 0)
	{
		// yuv 420
		f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

		// This is a multi-planar, two-plane version of the YUV 4:2:0 format.
		// Variation of V4L2_PIX_FMT_NV12 with planes non contiguous in memory.
		f->pixelformat = V4L2_PIX_FMT_NV12M;

		return 0;
	}
	else if (f->index == 1)
	{
		// yuv 444
		f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		f->pixelformat = V4L2_PIX_FMT_NV24;
		return 0;
	}
	else
		return -EINVAL;
}

static int isp_3dnr_video_try_cap_fmt(struct file *file, void *priv,
									  struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	pix_fmt_mp->field = V4L2_FIELD_NONE;

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return 0;
	else
		return -EINVAL;
}

static int isp_3dnr_video_enum_framesizes(struct file *file, void *priv,
										  struct v4l2_frmsizeenum *fsize)
{
	int i = 0;

	if (fsize->index != 0)
		return -EINVAL;
	for (i = 0; i < 4; ++i)
	{
		if (fsize->pixel_format != isp_3dnr_framesizes_arr[i].fourcc)
			continue;
		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->stepwise = isp_3dnr_framesizes_arr[i].stepwise;
		return 0;
	}
	return -EINVAL;
}

static int isp_3dnr_video_reqbufs(struct file *file, void *fh,
								  struct v4l2_requestbuffers *rb)
{
	struct isp_3dnr_video_fh *vfh = to_isp_3dnr_video_fh(fh);
	if ((rb->count > 0) && (rb->count < 3))
	{
		printk("(isp) isp 3dnr count=%d is not enough ( >= 3 ) %s(%d)    \n",
			   rb->count, __func__, __LINE__);
		return -1;
	}
	return vb2_reqbufs(&vfh->queue, rb);
}

static const struct v4l2_ioctl_ops isp_3dnr_video_ioctl_ops = {
	.vidioc_querycap = isp_3dnr_video_querycap,
	.vidioc_enum_framesizes = isp_3dnr_video_enum_framesizes,
	.vidioc_enum_fmt_vid_cap = isp_3dnr_video_enum_cap_fmt,
	//.vidioc_enum_fmt_vid_out = isp_video_enum_out_fmt,
	.vidioc_try_fmt_vid_cap_mplane = isp_3dnr_video_try_cap_fmt,
	//.vidioc_try_fmt_vid_out_mplane = isp_video_try_out_fmt,
	.vidioc_try_fmt_vid_cap = isp_3dnr_video_try_format,
	//.vidioc_try_fmt_vid_out = isp_3dnr_video_try_format,
	.vidioc_g_fmt_vid_cap = isp_3dnr_get_cap_format,
	.vidioc_g_fmt_vid_cap_mplane = isp_3dnr_get_cap_format_mplane,
	.vidioc_s_fmt_vid_cap = isp_3dnr_video_set_format,
	.vidioc_s_fmt_vid_cap_mplane = isp_3dnr_set_cap_format_mplane,
	//.vidioc_g_fmt_vid_out = isp_get_out_format,
	//.vidioc_g_fmt_vid_out_mplane = isp_get_out_format_mplane,
	//.vidioc_s_fmt_vid_out = isp_3dnr_video_set_format_output,
	//.vidioc_s_fmt_vid_out_mplane = isp_set_out_format_mplane,
	.vidioc_reqbufs = isp_3dnr_video_reqbufs,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	//.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	// .vidioc_create_bufs = vb2_ioctl_create_bufs,
	// .vidioc_qbuf = vb2_ioctl_qbuf,
	// .vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_qbuf = isp_3dnr_video_qbuf,
	.vidioc_dqbuf = isp_3dnr_video_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = isp_3dnr_video_streamon,
	.vidioc_streamoff = isp_3dnr_video_streamoff,
	.vidioc_g_selection = isp_3dnr_g_selection,
	.vidioc_s_selection = isp_3dnr_s_selection,
};

static const struct v4l2_file_operations isp_3dnr_video_fops = {
	.owner = THIS_MODULE,
	.open = isp_3dnr_video_open,
	.release = isp_3dnr_video_release,
	.poll = isp_3dnr_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = vb2_fop_mmap,
};

static struct video_device isp_3dnr_videodev = {
	.vfl_dir = VFL_DIR_RX,
	.device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE |
				   V4L2_CAP_VIDEO_OUTPUT_MPLANE,
	.fops = &isp_3dnr_video_fops,
	.ioctl_ops = &isp_3dnr_video_ioctl_ops,
	.release = video_device_release_empty,
};

static int isp_3dnr_register_video_dev(struct isp_3dnr_device *isp_3dnr[])
{
	int r, n;
	struct isp_3dnr_video *video;
	struct video_device *vdev;
	size_t buff_size = 0;
	void *virt_addr = NULL;
	dma_addr_t phy_addr = 0;

	for (n = 0; n < ISP_3DNR_MAX_VIDEO_DEVICE; n++)
	{
		video = &(g_3dnr_videos[n]);
		video->isp_dev = isp_3dnr[n];

		vdev = &video->vdev;
		*vdev = isp_3dnr_videodev;
		vdev->v4l2_dev = &isp_3dnr[n]->v4l2_dev;

		r = video_register_device(vdev, VFL_TYPE_VIDEO,
								  ISP_3DNR_DEVICE_BASE + n);
		if (r < 0)
		{
			dev_err(isp_3dnr[n]->dev,
					"video_register_device failed\n");
			return r;
		}
		else
		{
			video->type = VFL_TYPE_VIDEO;
			video_set_drvdata(vdev, video);
		}

		// 3dnr mot
		video->isp_dev->d3nr_mot_buff.buff_idx = 1;
		video->isp_dev->d3nr_mot_buff.max_size = ISP_3DNR_MOT_MAX_SIZE;

		buff_size = ISP_3DNR_PAGE_ALIGN(
			video->isp_dev->d3nr_mot_buff.max_size * 2);
		virt_addr = dma_alloc_coherent(video->isp_dev->dev, buff_size,
									   &phy_addr, GFP_KERNEL);

		video->isp_dev->d3nr_mot_buff.phy_addr = phy_addr;
		video->isp_dev->d3nr_mot_buff.virt_addr = virt_addr;
	}

	return 0;
}

static int isp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	switch (ctrl->id)
	{
	case V4L2_CID_BRIGHTNESS:
		printk("V4L2_CID_BRIGHTNESS\n");
		break;
	default:
		break;
	}
	return 0;
}

static const struct v4l2_ctrl_ops isp_ctrl_ops = {
	.s_ctrl = isp_s_ctrl,
};

static int isp_init_controls(struct isp_3dnr_device *isp)
{
	v4l2_ctrl_handler_init(&isp->ctrl_handler, 10);
	v4l2_ctrl_new_std(&isp->ctrl_handler, &isp_ctrl_ops,
					  V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	return 0;
}

static int isp_3dnr_remove(struct platform_device *pdev)
{
	int n = 0;
	struct isp_3dnr_video *video;

	for (n = 0; n < ISP_3DNR_MAX_VIDEO_DEVICE; n++)
	{
		video = &(g_3dnr_videos[n]);

		// release isp_3dnr_device
		if (video->isp_dev != NULL)
			kfree(video->isp_dev);
		video->isp_dev = NULL;
	}

	return 0;
}

static int isp_3dnr_probe(struct platform_device *pdev)
{
	struct isp_3dnr_device *isp_3dnr[ISP_3DNR_MAX_VIDEO_DEVICE];
	int ret;
	int i = 0;

	if (!pdev->dev.of_node)
	{
		pr_info("%s(%d):fail\n", __func__, __LINE__);
		return -ENODEV;
	}

	for (i = 0; i < ISP_3DNR_MAX_VIDEO_DEVICE; i++)
	{
		isp_3dnr[i] =
			kzalloc(sizeof(struct isp_3dnr_device), GFP_KERNEL);
		if (!isp_3dnr[i])
		{
			dev_err(&pdev->dev, "could not allocate memory\n");
			return -ENOMEM;
		}
		isp_3dnr[i]->dev = &pdev->dev;
		isp_3dnr[i]->v4l2_dev.ctrl_handler = &isp_3dnr[i]->ctrl_handler;
		isp_3dnr[i]->dev_id = i;
		isp_3dnr[i]->first_frame = 1;
		isp_3dnr[i]->input_buf = NULL;
		isp_3dnr[i]->output_buf = NULL;
		isp_3dnr[i]->device_alive = 0;

		isp_init_controls(isp_3dnr[i]);

		if (i == 0)
		{
			ret = isp_3dnr_init_modules(pdev, isp_3dnr[i]);
			if (ret < 0)
				goto error_modules;
		}

		ret = isp_3dnr_register_entities(isp_3dnr[i]);
		if (ret < 0)
			goto error_modules;
	}

	ret = isp_3dnr_register_video_dev(isp_3dnr);
	if (ret < 0)
		goto error_register;

	// init isp debug fs
	// ret = isp_debugfs_init(isp[0]->dev);
	// if (ret)
	//	dev_err(isp[0]->dev, "cannot create isp debugfs (%d)\n", ret);

	dev_info(&pdev->dev, "SP ISP 3DNR driver probed\n");

	return 0;

error_register:
error_modules:
	// isp_cleanup_modules(isp);
	return 0;
}

static const struct of_device_id isp_3dnr_ids[] = {
	{
		.compatible = "sunplus,sp7350-isp3dnr",
	},
	{}};
MODULE_DEVICE_TABLE(of, isp_3dnr_ids);

static struct platform_driver sp_isp3dnr_driver = {
	.probe = isp_3dnr_probe,
	.remove = isp_3dnr_remove,
	//.id_table = isp_id_table,
	.driver = {
		.name = "sp-isp3dnr",
		.of_match_table = isp_3dnr_ids,
		//.pm	= &isp_pm_ops,
		//.of_match_table = isp_of_table,
	},
};

static int __init isp_3dnr_init(void)
{
	platform_driver_register(&sp_isp3dnr_driver);
	return 0;
}

static void __exit isp_3dnr_exit(void)
{
	printk("%s\n", __func__);
}

module_init(isp_3dnr_init);
module_exit(isp_3dnr_exit);

module_param(video_3dnr_nr, int, 0444);
MODULE_INFO(version, "1.0");
MODULE_AUTHOR("Sunplus");
MODULE_DESCRIPTION("Sunplus ISP 3DNR driver");
MODULE_LICENSE("GPL");
