// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP3DNR
 *
 */
#ifndef ISP_3DNR_H
#define ISP_3DNR_H

#include <media/v4l2-device.h>
#include <media/media-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-vmalloc.h>
#include <media/videobuf2-dma-contig.h>

enum swapStatus {
	SWAP_3DNR_NONE = 0,
	SWAP_3DNR_ODD,
	SWAP_3DNR_EVEN,
	SWAP_3DNR_MAX
};

struct isp_3dnr_ret {
	// for y, uv
	struct vb2_buffer *in_buff;
	struct vb2_buffer *out_buff;

	//for mot
	dma_addr_t in_mot_phy_addr;
	dma_addr_t out_mot_phy_addr;
};

// statis buffer
struct statis_3dnr_buff {
	void *virt_addr;
	dma_addr_t phy_addr;
	size_t max_size;
	int buff_idx; // for hw read (default 1)
};

struct isp_3dnr_device {
	struct v4l2_device v4l2_dev;
	struct mutex dev_mutex;

	struct device *dev;
	struct v4l2_ctrl_handler ctrl_handler;
	struct vb2_queue queue;

	struct buffer_queue *bq;

	unsigned int dev_id;

	int first_frame;

	struct vb2_buffer *input_buf;
	struct vb2_buffer *output_buf;
	struct statis_3dnr_buff d3nr_mot_buff;

	enum swapStatus in_out_swap; //0: no swap, 1:odd swap, 2:even swap

	int device_alive;
};

void isp_3dnr_video_monitor_q_info(void);
void isp_3dnr_get_buff(int dev_id, struct isp_3dnr_ret *ret);
void isp_3dnr_hw_done(int dev_id);
int isp_3dnr_get_first_frame(int dev_id); // 0 : not first frame, 1:first frame
int isp_3dnr_get_enough_frame(int dev_id); // 0 : not enough, 1:enough
int isp_3dnr_get_device_alive(int dev_id);
#endif