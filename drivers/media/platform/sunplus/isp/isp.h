// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#ifndef ISP_H
#define ISP_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/media-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-vmalloc.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-mem2mem.h>

#include "isp_util.h"

#define ISP_MAX_WIDTH	2688
#define ISP_MAX_HEIGHT	1944

// mlsc relative
#define MLSC_BUFF_MAX_SIZE (64 * 64 * 4) //(64 * 64 * 32 bits)
// statis buffer relative
#define FCURVE_BUFF_MAX_SIZE (1024) // 32x32x8 bits
#define WDR_BUFF_MAX_SIZE (1024 * 8) // 32x32x64 bits
#define LCE_BUFF_MAX_SIZE (1024) // 32x32x8 bits
#define CNR_BUFF_MAX_SIZE (0x100000) // 1022x1022 = 1044484 = 0xFF004
#define D3NR_Y_MAX_SIZE (ISP_MAX_WIDTH * ISP_MAX_HEIGHT) // 2688*1944*8 bits
#define D3NR_UV_MAX_SIZE (ISP_MAX_WIDTH * ISP_MAX_HEIGHT * 2) // 2688*1944*16 bits
#define D3NR_U_MAX_SIZE (ISP_MAX_WIDTH * ISP_MAX_HEIGHT) // 2688*1944*8 bits
#define D3NR_V_MAX_SIZE (ISP_MAX_WIDTH * ISP_MAX_HEIGHT) // 2688*1944*8 bits
#define D3NR_MOT_MAX_SIZE (ISP_MAX_WIDTH * ISP_MAX_HEIGHT) // 2688*1944*8 bits
#define D3NR_REDUCE_BANDWITH (false)

#define ISP_PAGE_MASK (~(PAGE_SIZE - 1))
#define ISP_PAGE_ALIGN(x) (((x) + PAGE_SIZE - 1) & ISP_PAGE_MASK)

#define CNR_SIZE_FORMULA(width, height) \
	(ISP_PAGE_ALIGN((size_t)((width) / 4) * ((height) / 4) * 2)) // (w/4)*(h/4)*16 bits
#define D3NR_Y_SIZE_FORMULA(width, height) (ISP_PAGE_ALIGN((size_t)(width) * (height)))
#define D3NR_UV_SIZE_FORMULA(width, height) (ISP_PAGE_ALIGN((size_t)(width) * (height) * 2))
#define D3NR_U_SIZE_FORMULA(width, height) (ISP_PAGE_ALIGN((size_t)(width) * (height)))
#define D3NR_V_SIZE_FORMULA(width, height) (ISP_PAGE_ALIGN((size_t)(width) * (height)))
#define D3NR_MOT_SIZE_FORMULA(width, height) (ISP_PAGE_ALIGN((size_t)(width) * (height)))

enum ISP_CLK_GATE_STATUS {
	ISP_CLK_FUNC_NONE,
	ISP_CLK_FUNC_EN,
	ISP_CLK_FUNC_DIS,
	ISP_CLK_FUNC_ALL,
};

enum ISP_OUT_PLANE_ID {
	PLANE_HDR1 = 1,
	PLANE_HDR1_2 = 2,
	PLANE_HDR1_3D = 4,
	PLANE_HDR1_2_3D = 5,
};

enum ISP_PLANE_ID {
	PLANE_MLSC = 1,
	PLANE_Y_UV = 2,
	PLANE_Y_UV_STATIS = 3, // Y_U_V
	PLANE_Y_U_V_STATIS = 4,
	PLANE_Y_UV_3D = 5,
	PLANE_Y_UV_3D_STATIS = 6, // Y_U_V_3D
	PLANE_Y_U_V_3D_STATIS = 7,
};

enum ISP_DMA_ID {
	OUT_HDR_0 = 0, // dma_1
	OUT_HDR_1, // dma_3
	OUT_FCURVE, // dma_5
	OUT_MLSC, // dma_9
	OUT_WDR, // dma_11
	OUT_LCE, // dma_15
	OUT_CNR, // dma_19
	OUT_3DNR_Y, // dma_23
	OUT_3DNR_UV, // dma_27
	OUT_3DNR_MOT, // dma_45

	CAP_Y, // dma_31
	CAP_UV, // dma_33
	CAP_FCURVE, // dma_7
	CAP_WDR, // dma_13
	CAP_LCE, // dma_17
	CAP_CNR, // dma_21
	CAP_3DNR_Y, // dma_25
	CAP_3DNR_UV, // dma_29
	CAP_3DNR_MOT, // dma_47
	CAP_AWB, // dma_35
	CAP_AE_BA, // dma_37
	CAP_AE_HIST, // dma_39
	CAP_AE2_BA, // dma_41
	CAP_AE2_HIST, // dma_43

	ISP_DMA_ID_NUM,
};

// ioctrl relative
struct write_reg_config {
	int fd;
	int buff_idx;
};

enum moniStatus {
	MONI_INT = 0,
	MONI_Q_BUFF,
	MONI_Q_PAIR,
	MONI_HW_DOING,
	MONI_HW_DONE,
	MONI_DQ_BUFF,
	MONI_STATUS_MAX
};

struct reg_queue {
	u8 regQ_id;
	struct isp_video_fh *hdl;
	struct list_head head;
};

struct cap_buff {
	dma_addr_t y_out_buff;
	dma_addr_t u_out_buff; // including uv case
	dma_addr_t v_out_buff;
	dma_addr_t d3nr_Y_ref_out_buff;
	dma_addr_t d3nr_UV_ref_out_buff;
	dma_addr_t d3nr_motion_cur_out_buff;

	dma_addr_t fcurve_subout_buff;
	dma_addr_t wdr_subout_buff;
	dma_addr_t lce_subout_buff;
	dma_addr_t cnr_subout_buff;
	dma_addr_t awb_out_buff;
	dma_addr_t ae_ba_buff;
	dma_addr_t ae_hist_buff;
	dma_addr_t ae2_ba_buff;
	dma_addr_t ae2_hist_buff;

	size_t y_out_size;
	size_t u_out_size; // including uv case
	size_t v_out_size;
	size_t d3nr_Y_ref_out_size;
	size_t d3nr_UV_ref_out_size;
	size_t d3nr_motion_cur_out_size;

	size_t fcurve_subout_size;
	size_t wdr_subout_size;
	size_t lce_subout_size;
	size_t cnr_subout_size;
	size_t awb_out_size;
	size_t ae_ba_size;
	size_t ae_hist_size;
	size_t ae2_ba_size;
	size_t ae2_hist_size;

	struct dma_buf *statis_dma;

	void *y_out_virt;
	void *u_out_virt;
	void *v_out_virt;
	void *awb_virt;
	void *ae_ba_virt;
	void *ae_hist_virt;
	void *ae2_ba_virt;
	void *ae2_hist_virt;
};

struct out_buff {
	dma_addr_t input_hdr1_buf;
	dma_addr_t input_rgbir_buf;
	dma_addr_t input_hdr2_buf;
	dma_addr_t fcurve_subin_dma_buf;
	dma_addr_t mlsc_dma_buf;
	dma_addr_t wdr_subin_dma_buf;
	dma_addr_t lce_subin_dma_buf;
	dma_addr_t cnr_subin_dma_buf;
	dma_addr_t d3nr_Y_ref_in_dma_buf;
	dma_addr_t d3nr_UV_ref_in_dma_buf;
	dma_addr_t d3nr_motion_ref_in_dma_buf;

	size_t input_hdr1_size;
	size_t input_rgbir_size;
	size_t input_hdr2_size;
	size_t fcurve_subin_dma_size;
	size_t mlsc_dma_size;
	size_t wdr_subin_dma_size;
	size_t lce_subin_dma_size;
	size_t cnr_subin_dma_size;
	size_t d3nr_Y_ref_in_dma_size;
	size_t d3nr_UV_ref_in_dma_size;
	size_t d3nr_motion_ref_in_dma_size;

	void *input_hdr1_virt;
	void *input_hdr2_virt;
};

// 3a statis buffer

enum out_data_format {
	CAP_YUV444 = 0,
	CAP_YUV420,
	CAP_YUV422,

	CAP_YUV444_P,
	CAP_YUV420_P,
	CAP_YUV422_P,
};

struct statis_buff {
	// for 3dnr statis in, and other statis in/out
	void *virt_addr;
	dma_addr_t phy_addr;

	// for 3dnr statis out
	void *virt_addr_out;
	dma_addr_t phy_addr_out;

	size_t max_size;
	int buff_idx; // record idx of statis_in  (default 1), 3dnr do not use
	int first_frame; // 1: first frame

	int sc_en; // 1: scaler on, 0:scaler off
	int out_sep_en; // 1: y/u/v sep mode, 0: y/uv mode
	enum out_data_format d3nr_fmt;

	int sram_en; // 1: use sram 0:dma alloc
};

struct isp_hw_buff {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 47)
	struct vb2_buffer vb2_buf;
	struct vb2_queue vb2_q;
#endif
	struct dma_buf *mlsc_ion_buf;
	dma_addr_t mlsc_addr;
	void *mlsc_virt;
	void *mlsc_mem_priv;

	struct statis_buff fcurve_buff;
	struct statis_buff wdr_buff;
	struct statis_buff lce_buff;
	struct statis_buff cnr_buff;
	struct statis_buff d3nr_y_buff;
	struct statis_buff d3nr_uv_buff;
	struct statis_buff d3nr_v_buff;
	struct statis_buff d3nr_mot_buff;
};

struct usr_reg_head {
	// int sensor_id;
	struct SettingHeader *usr_header;
	struct FrameSetting *usr_reg_addr;
	struct list_head head;
};

#define HDL_MAX_SIZE (200)
struct isp_device {
	struct v4l2_device v4l2_dev;
	struct device *dev;
	struct mutex dev_mutex;
};

struct isp_video {
	struct video_device vdev;
	enum vfl_devnode_type type;
	struct isp_device *isp_dev;
};

#define MONI_Q_FRAME_NUM (20)
struct isp_video_fh {
	struct v4l2_fh vfh;
	struct isp_video *video;
	struct v4l2_format format; // for m2m caputure
	struct v4l2_format out_format; // for m2m output
	struct v4l2_fract timeperframe;

	struct v4l2_m2m_dev *m2m_dev;
	struct v4l2_m2m_ctx *m2m_ctx;

	// regQ relative
	struct v4l2_ctrl_handler ctrl_handler;
	u8 regQ_id;
	u8 *rq_addr;
	u8 in_buff_status[MONI_Q_FRAME_NUM];
	u8 out_buff_status[MONI_Q_FRAME_NUM];

	u32 translen;
	u32 sequence;
	// struct vb2_queue *queue_ptr;
	struct mutex queue_lock; /* protects the vb2 queue */

	struct mutex process_mtx;

	struct isp_hw_buff hw_buff;

	struct list_head usr_reg_list;

	u64 lastTuningID;

	struct work_struct work;

	u32 frame_cnt; // for debug tool

	void __iomem *isp_sram_virt; // sram virtual addr
	size_t sram_size;
};

struct ae_ba_info {
	int enable;
	size_t ae_ba_max_size; // 2048
	size_t width_blocks;
	size_t height_blocks;
	size_t data_offset;
};

struct ae_hist_info {
	int enable;
	size_t ae_hist_max_size; // 1024
	size_t hist_type;
	size_t data_offset;
};

struct awb_info {
	int enable;
	size_t awb_max_size; // 4096
	size_t width_blocks;
	size_t height_blocks;
	size_t data_offset;
};

struct statis_header_info {
	size_t version;
	u64 tuning_id;
};

struct statis_info {
	struct statis_header_info statis_header;
	struct awb_info awb_statis;
	struct ae_ba_info ae1_ba_statis;
	struct ae_hist_info ae1_hist_statis;
	struct ae_ba_info ae2_ba_statis;
	struct ae_hist_info ae2_hist_statis;
};

void isp_video_monitor_q_info(void);
enum ISP_CLK_GATE_STATUS isp_video_get_clk_func_status(void);
void isp_video_set_clk_func_status(enum ISP_CLK_GATE_STATUS clk_status);
u8 isp_video_get_dump_log_status(void);
void isp_video_set_dump_log_status(u8 log_status);
#endif
