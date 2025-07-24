/*!
 * @file isp.c
 * @brief isp video device
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
#include "isp.h"
#include "isp_reg.h"
#include "isp_debugfs.h"
#include "isp_dbg.h"

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_PM
#include <linux/pm_runtime.h>
#endif
#include <linux/reset.h>

#define CREATE_TRACE_POINTS
#include "isp_trace.h"

void __iomem *isp_base = NULL;
void __iomem *isp_base_2 = NULL;
void __iomem *isp_cur_base = NULL;

dma_addr_t isp_sram_phy = 0;

int irq;

static unsigned long isp_clk_rate = 614 * 1000 * 1000; // 614MHz

#define DEBUG_BUFF_MESG (false || (isp_video_get_dump_log_status() == 1))
#define DEBUG_FORMAT_MESG (false)
#define DEBUG_POLL_MESG (false)
#define INPUT_REG_BUFF_DBUG (false)
#define DEBUG_SUSPEND_RESUME (true)

#define INTR_TIMEOUT_MODE (true)

//#define REG_MEM_RESERVED

// suspend / resume relative
//#define ISP_PM_RST
#ifdef ISP_PM_RST
struct reset_control *rst = NULL;
#endif

//#define ISP_REGULATOR
#ifdef ISP_REGULATOR
struct regulator *regulator = NULL;
#endif

#define CLK_GATING_FUNC_EN (true)

#define ISP_SRAM_SIZE (2) // ISP SRAM size = 88k. isp num = 2, 4k align. isp num = 4, no 4k align.
#define IS_ISP_SRAM_EN(id) (false && (isp_sram_phy != 0) && ((id) < ISP_SRAM_SIZE))

#define MAX_VIDEO_DEVICE (1)

#define ISP_DEFAULT_WIDTH 320
#define ISP_DEFAULT_HEIGHT 240
#define ISP_DEFAULT_FORMAT V4L2_PIX_FMT_SBGGR8

#define ISP_DEVICE_BASE (96)

// set ctrl
#define CID_CUSTOM_BASE (V4L2_CID_USER_BASE | 0xf000)
#define CID_ISP_REG_BUF_DONE (CID_CUSTOM_BASE + 10)
#define CID_ISP_SET_MLSC_FD (CID_CUSTOM_BASE + 11)

#define ISP_U32_MAX (4294967295)

enum moniType { MONI_OUT = 0, MONI_CAP, MONI_TYPE_MAX };

static int video_nr = -1;

static const struct vb2_mem_ops *mem_ops = &vb2_dma_contig_memops;

static enum ISP_CLK_GATE_STATUS isp_dbg_clk_status = ISP_CLK_FUNC_NONE;
static u8 isp_dbg_log_status = 0; // 0:dump log disable 1:dump log enable

wait_queue_head_t isp_hw_done_wait;
u32 isp_hw_done_flags = 0;
unsigned long flags;
spinlock_t isp_hw_done_lock;

struct isp_video_work_data {
	struct vb2_v4l2_buffer *src_buf;
	struct vb2_v4l2_buffer *dst_buf;
	size_t cap_width;
	size_t cap_height;
};
// workqueue relative
struct workqueue_struct *isp_wq = NULL;
struct isp_video_work_data isp_data;

#define to_isp_video_fh(fh) container_of(fh, struct isp_video_fh, vfh)

static struct v4l2_m2m_dev *g_m2m_dev = NULL;
static struct isp_video g_videos[MAX_VIDEO_DEVICE];
static struct mutex write_reg_mtx;

// reg queue relative
static struct list_head reg_queue_list;
static struct mutex reg_queue_mtx;

// clock gating relative
static struct clk *vcl_clk = NULL;
static struct clk *vcl5_clk = NULL;
static struct clk *isp_clk = NULL;

struct isp_framesizes {
	u32 fourcc;
	struct v4l2_frmsize_stepwise stepwise;
};

static const struct isp_framesizes isp_framesizes_arr[] = {
	{
		.fourcc = V4L2_PIX_FMT_NV12,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_NV12M,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420M,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_NV16,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_NV16M,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV422M,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_NV24,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV444M,
		.stepwise = { 10, 3000, 1, 10, 2000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR10,
		.stepwise = { 10, 5000, 1, 10, 5000, 1 },
	},
	{
		.fourcc = V4L2_PIX_FMT_SBGGR12,
		.stepwise = { 10, 5000, 1, 10, 5000, 1 },
	}
};

static int isp_s_ctrl(struct v4l2_ctrl *ctrl);
static const struct v4l2_ctrl_ops isp_ctrl_ops = {
	.s_ctrl = isp_s_ctrl,
};

static const struct v4l2_ctrl_config isp_reg_buffer_done_device_id = {
	.ops = &isp_ctrl_ops,
	.id = CID_ISP_REG_BUF_DONE,
	.name = "CID_ISP_REG_BUF_DONE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 2147483647,
	.def = 0,
	.step = 1,
};

static const struct v4l2_ctrl_config isp_set_mlsc_fd = {
	.ops = &isp_ctrl_ops,
	.id = CID_ISP_SET_MLSC_FD,
	.name = "CID_ISP_SET_MLSC_FD",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	.min = 0,
	.max = 2147483647,
	.def = 0,
	.step = 1,
};

static int isp_video_clk_enable(void);
static int isp_video_clk_disable(void);

static int isp_init_controls(struct isp_video_fh *handle);

static int isp_video_alloc_regQ(struct isp_video_fh *handle);
static void isp_video_free_regQ(struct isp_video_fh *handle);

static int isp_alloc_dma_buf(struct isp_video_fh *vfh);
static void isp_release_dma_buf(struct isp_video_fh *vfh);

static void isp_video_hw_setting(struct work_struct *work);

static void isp_release_mlsc_buf(struct isp_video_fh *vfh);

static void isp_video_free_3dnr(struct isp_video_fh *vfh);
static int isp_video_alloc_3dnr(struct isp_video_fh *vfh, int sc_en);
static void isp_video_3dnr_reset(struct isp_video_fh *vfh);
static u32 isp_video_set_3dnr_flow(struct isp_video_fh *vfh, struct statis_buff *y_buf);
static u32
isp_video_set_3dnr_addr(struct isp_video_fh *vfh, struct cap_buff *cap_bufq,
			struct out_buff *out_bufq, struct statis_buff *y_buf,
			struct statis_buff *uv_buf, struct statis_buff *v_buf,
			struct statis_buff *mot_buf);

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

static void isp_video_reset_statis(struct isp_video_fh *vfh, struct ModuleRest *reset_module)
{
	u32 func_en_0 = 0;
	u32 func_en_1 = 0;

	READ_REG(isp_cur_base, ISP_FUNC_EN_0, &func_en_0)
	READ_REG(isp_cur_base, ISP_FUNC_EN_1, &func_en_1)

	if (((reset_module->reset_all == 1) ||
		 (reset_module->reset_fcurve == 1)) &&
		((func_en_0 & FUNC_EN_0_FCURVE_EN_MASK) == FUNC_EN_0_FCURVE_EN_MASK)) {
		vfh->hw_buff.fcurve_buff.buff_idx = 1;
		vfh->hw_buff.fcurve_buff.first_frame = 1;
		reset_module->reset_fcurve = 0;
	}

	if (((reset_module->reset_all == 1) ||
		 (reset_module->reset_wdr == 1)) &&
		((func_en_1 & FUNC_EN_1_WDR_EN_MASK) == FUNC_EN_1_WDR_EN_MASK)) {
		vfh->hw_buff.wdr_buff.buff_idx = 1;
		vfh->hw_buff.wdr_buff.first_frame = 1;
		reset_module->reset_wdr = 0;
	}

	if (((reset_module->reset_all == 1) ||
		 (reset_module->reset_lce == 1)) &&
		((func_en_1 & FUNC_EN_1_LCE_ENABLE_MASK) == FUNC_EN_1_LCE_ENABLE_MASK)) {
		vfh->hw_buff.lce_buff.buff_idx = 1;
		vfh->hw_buff.lce_buff.first_frame = 1;
		reset_module->reset_lce = 0;
	}

	if (((reset_module->reset_all == 1) ||
		 (reset_module->reset_cnr == 1)) &&
		((func_en_1 & FUNC_EN_1_CNR_ENABLE_MASK) == FUNC_EN_1_CNR_ENABLE_MASK)) {
		vfh->hw_buff.cnr_buff.buff_idx = 1;
		vfh->hw_buff.cnr_buff.first_frame = 1;
		reset_module->reset_cnr = 0;
	}

	if (((reset_module->reset_all == 1) ||
		 (reset_module->reset_3dnr == 1)) &&
		((func_en_1 & FUNC_EN_1_3DNR_ENABLE_MASK) == FUNC_EN_1_3DNR_ENABLE_MASK)) {
		isp_video_3dnr_reset(vfh);
		reset_module->reset_3dnr = 0;
	}
	reset_module->reset_all = 0;
}

void isp_video_monitor_q_info(void)
{
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct reg_queue *entry = NULL;
	struct isp_video_fh *hdl = NULL;

	list_for_each_safe(listptr, listptr_next, &reg_queue_list) {
		entry = list_entry(listptr, struct reg_queue, head);
		hdl = entry->hdl;

		printk("=== regQ_idx = %d  hdl=0x%px input \n", entry->regQ_id, entry->hdl);
		printk("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			hdl->in_buff_status[0], hdl->in_buff_status[1],
			hdl->in_buff_status[2], hdl->in_buff_status[3],
			hdl->in_buff_status[4], hdl->in_buff_status[5],
			hdl->in_buff_status[6], hdl->in_buff_status[7],
			hdl->in_buff_status[8], hdl->in_buff_status[9],
			hdl->in_buff_status[10], hdl->in_buff_status[11],
			hdl->in_buff_status[12], hdl->in_buff_status[13],
			hdl->in_buff_status[14], hdl->in_buff_status[15],
			hdl->in_buff_status[16], hdl->in_buff_status[17],
			hdl->in_buff_status[18], hdl->in_buff_status[19]);

		printk("=== regQ_idx = %d  hdl=0x%px output \n", entry->regQ_id, entry->hdl);
		printk("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			hdl->out_buff_status[0], hdl->out_buff_status[1],
			hdl->out_buff_status[2], hdl->out_buff_status[3],
			hdl->out_buff_status[4], hdl->out_buff_status[5],
			hdl->out_buff_status[6], hdl->out_buff_status[7],
			hdl->out_buff_status[8], hdl->out_buff_status[9],
			hdl->out_buff_status[10], hdl->out_buff_status[11],
			hdl->out_buff_status[12], hdl->out_buff_status[13],
			hdl->out_buff_status[14], hdl->out_buff_status[15],
			hdl->out_buff_status[16], hdl->out_buff_status[17],
			hdl->out_buff_status[18], hdl->out_buff_status[19]);
	}
}

static int isp_video_mmap(struct file *fp, struct vm_area_struct *vma)
{
	int ret = 0;
	struct isp_video_fh *vfh = fp->private_data;

	if (vfh == NULL) {
		printk("(isp) handle ptr is NULL  %s(%d) \n", __func__, __LINE__);
		return ret;
	}

	if (vfh->rq_addr == NULL) {
		printk("(isp) rq_addr is NULL, vfh->regQ_id=%d  %s(%d) \n",
			vfh->regQ_id, __func__, __LINE__);
		return ret;
	}

	if (DEBUG_BUFF_MESG) {
		printk("(isp) mmap hdl_ptr = 0x%px  reg_addr = 0x%px  reg_id=%d  %s(%d)\n",
			vfh, vfh->rq_addr, vfh->regQ_id, __func__, __LINE__);
	}

	mutex_lock(&vfh->process_mtx);

	// mmap start
	// vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	ret = remap_vmalloc_range(vma, vfh->rq_addr, 0);
	if (ret) {
		pr_warn("%s ,remapping memory error:%d \n", __func__, ret);
		mutex_unlock(&vfh->process_mtx);
		return ret;
	}

	mutex_unlock(&vfh->process_mtx);
	return 0;
}

static long isp_video_reg_no_setting(size_t cap_width, size_t cap_height)
{
	if (DEBUG_BUFF_MESG) {
		printk("(isp) no setting cap_width = 0x%lx %s(%d)\n", cap_width, __func__, __LINE__);
		printk("(isp) no setting cap_height = 0x%lx %s(%d)\n", cap_height, __func__, __LINE__);
	}

	WRITE_REG(isp_cur_base, ISP_CONTROL_0, (cap_height << 16) | cap_width, 0xFFFFFFFF);
	WRITE_REG(isp_cur_base, ISP_SCALER_0, (cap_height << 16) | cap_width, 0xFFFFFFFF);
	WRITE_REG(isp_cur_base, ISP_SCALER_1, (cap_height << 16) | cap_width, 0xFFFFFFFF);
	return 0;
}

static long isp_video_write_reg(struct isp_video_fh *vfh, size_t cap_width, size_t cap_height)
{
	struct list_head *listptr, *listptr_next = NULL;
	struct usr_reg_head *entry;
	u8 *usr_reg_addr = NULL;
	bool found = false;
	u32 in_buf_sequence = 0;

	in_buf_sequence = isp_data.src_buf->sequence;

	if (DEBUG_BUFF_MESG) {
		printk("(isp) hdl_ptr=0x%px%s(%d)\n", vfh, __func__, __LINE__);
		printk("(isp) in_buf_sequence=%d  out of while %s(%d)\n",
			in_buf_sequence, __func__, __LINE__);
	}

	list_for_each_safe(listptr, listptr_next, &vfh->usr_reg_list) {
		entry = list_entry(listptr, struct usr_reg_head, head);

		if (DEBUG_BUFF_MESG) {
			printk("%s(%d) (isp) entry->usr_header->regQ_idx=%d input_sequence=%d in_buf_sequence=%d\n",
				__func__, __LINE__, entry->usr_header->regQ_idx,
				entry->usr_header->input_sequence,
				in_buf_sequence);
		}

		if (entry->usr_header->input_sequence == in_buf_sequence) {
			usr_reg_addr = (u8 *)entry->usr_reg_addr;
			if (usr_reg_addr == NULL) {
				printk("%s: null reg buff\n", __func__);
				return -EFAULT;
			}
			if (DEBUG_BUFF_MESG)
				printk("(isp) new setting %s(%d)\n", __func__, __LINE__);
			trace_reg_new_setting(vfh);

			isp_video_write_reg_from_buff(usr_reg_addr);
			entry->usr_header->status = BUF_STATUS_DONE;

			// record last reg setting
			vfh->lastTuningID = entry->usr_header->statis_id;

			if (DEBUG_BUFF_MESG)
				printk("(isp)  %s(%d) get handle=0x%px  statis_id=%lld  \n",
					__func__, __LINE__, vfh,
					entry->usr_header->statis_id);

			found = true;

			// reset statis flow
			if (DEBUG_BUFF_MESG) {
				printk("(isp) ori_reset flag: all=%d fcurve=%d wdr=%d lce=%d cnr=%d 3dnr=%d  %s(%d)\n",
					entry->usr_header->reset_module.reset_all,
					entry->usr_header->reset_module.reset_fcurve,
					entry->usr_header->reset_module.reset_wdr,
					entry->usr_header->reset_module.reset_lce,
					entry->usr_header->reset_module.reset_cnr,
					entry->usr_header->reset_module.reset_3dnr,
					__func__, __LINE__);
			}
			isp_video_reset_statis(
				vfh, &entry->usr_header->reset_module);
			if (DEBUG_BUFF_MESG) {
				printk("(isp) now reset flag: all=%d fcurve=%d wdr=%d lce=%d cnr=%d 3dnr=%d %s(%d)\n",
					entry->usr_header->reset_module.reset_all,
					entry->usr_header->reset_module.reset_fcurve,
					entry->usr_header->reset_module.reset_wdr,
					entry->usr_header->reset_module.reset_lce,
					entry->usr_header->reset_module.reset_cnr,
					entry->usr_header->reset_module.reset_3dnr,
					__func__, __LINE__);
			}

			if (!list_is_last(&entry->head, &vfh->usr_reg_list)) {
				list_del(&entry->head);
				kfree(entry);
			}

			break;
		} else if (entry->usr_header->input_sequence <
			in_buf_sequence) {
			entry->usr_header->status = BUF_STATUS_NONE;
			if (!list_is_last(&entry->head, &vfh->usr_reg_list)) {
				if (DEBUG_BUFF_MESG) {
					printk("%d delete useless setting which came too late %d < %d\n",
						entry->usr_header->regQ_idx,
						entry->usr_header->input_sequence,
						in_buf_sequence);
				}
				list_del(&entry->head);
				kfree(entry);
			}
		}
	}

	if (!found) {
		if (DEBUG_BUFF_MESG) {
			list_for_each_safe(listptr, listptr_next, &vfh->usr_reg_list) {
				entry = list_entry(listptr, struct usr_reg_head, head);
				printk("%s(%d) (isp) entry->usr_header->regQ_idx=%d input_sequence=%d \
					in_buf_sequence=%d\n",
					__func__, __LINE__,
					entry->usr_header->regQ_idx,
					entry->usr_header->input_sequence,
					in_buf_sequence);
			}
		}
		// printk("(isp) no corresponding setting, regQ_id=%d %s(%d)\n",
		// 	vfh->regQ_id, __func__, __LINE__);

		entry = list_last_entry(&vfh->usr_reg_list, struct usr_reg_head, head);
		if (entry) {
			if (DEBUG_BUFF_MESG)
				printk("(%s) frame%d use last setting\n", __func__, in_buf_sequence);

			usr_reg_addr = (u8 *)entry->usr_reg_addr;
			if (!usr_reg_addr) {
				printk("%s: null reg buff\n", __func__);
				return -EFAULT;
			}

			isp_video_write_reg_from_buff(usr_reg_addr);
		} else {
			if (DEBUG_BUFF_MESG)
				printk("(%s) use default setting(%d)\n", __func__, in_buf_sequence);

			trace_reg_no_setting(vfh);

			isp_video_reg_no_setting(cap_width, cap_height);
		}
	}

	// hard code register for debug
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 47)
	isp_dbg_hardcode_reg(isp_cur_base, vfh);
#endif

	return 0;
}

static inline void isp_video_write_reg_trigger(void)
{
	// launch
	WRITE_REG(isp_cur_base, ISP_START_LAUNCH, START_LAUNCH_FRAME_START_MASK, START_LAUNCH_FRAME_START_MASK);
}

static long isp_video_reg_buf_add(struct isp_video_fh *handle, u32 input_data)
{
	u32 offset = input_data & 0xFFFFFF;
	u8 *start_addr = NULL;
	struct SettingMmaping *usr_setting = NULL;
	struct usr_reg_head *entry = NULL;

	if (handle == NULL) {
		printk("(isp) handle is NULL. %s(%d)\n", __func__, __LINE__);
		return -EINVAL;
	}
	trace_reg_user_start(handle);

	mutex_lock(&handle->process_mtx);

	if (DEBUG_BUFF_MESG) {
		printk("(isp) add new reg buf node. hdl_ptr=0x%px  regQ_id=%d  offset=0x%x  %s(%d)\n",
			handle, handle->regQ_id, offset, __func__, __LINE__);
	}

	start_addr = handle->rq_addr + offset;
	usr_setting = (struct SettingMmaping *)start_addr;
	usr_setting->header.regQ_idx = handle->regQ_id;
	usr_setting->header.status = BUF_STATUS_DOING;

	if (DEBUG_BUFF_MESG) {
		printk("(isp)  %s(%d)  usr_done reg buffer=0x%px  \n", __func__, __LINE__, start_addr);
		printk("(isp)  %s(%d)  statis_id=%lld  \n", __func__, __LINE__, usr_setting->header.statis_id);
	}

	// add list
	entry = kzalloc(sizeof(struct usr_reg_head), GFP_KERNEL);
	if (entry == NULL)
		printk("%s: isp reg_setting entry alloc failed \n", __func__);
	else {
		entry->usr_header = &(usr_setting->header);
		entry->usr_reg_addr = &(usr_setting->module[0]);
		list_add_tail(&entry->head, &handle->usr_reg_list);
	}

	trace_reg_user_done(handle);
	mutex_unlock(&handle->process_mtx);

	return 0;
}

static void isp_video_print_reg(void)
{
	int idx = 0;
	unsigned int value = 0;

	printk("====== isp driver : print reg %s(%d) start ======\n", __func__, __LINE__);
	for (idx = 0; idx < 1026; idx++) {
		READ_REG(isp_cur_base, idx * 4, &value);
		printk("%08x%08x\n", idx * 4, value);
	}
	printk("====== isp driver : print reg %s(%d) end ======\n", __func__, __LINE__);
}

static void isp_video_debug_message(void)
{
	if (INPUT_REG_BUFF_DBUG)
		isp_video_print_reg();
}

static void set_statis_write_phy_addr(struct isp_video_fh *vfh,
					  enum ISP_DMA_ID e_module)
{
	int readIdx = 0;
	int writeIdx = 0;
	dma_addr_t phy_addr = 0;
	size_t max_size = 0;

	switch (e_module) {
	case CAP_FCURVE: {
		readIdx = vfh->hw_buff.fcurve_buff.buff_idx;
		writeIdx = (readIdx + 1) % 2;
		phy_addr = vfh->hw_buff.fcurve_buff.phy_addr;
		max_size = vfh->hw_buff.fcurve_buff.max_size;

		if (writeIdx == 0)
			WRITE_REG(isp_cur_base, ISP_DMA_6, phy_addr, 0xFFFFFFFF)
		else
			WRITE_REG(isp_cur_base, ISP_DMA_6, phy_addr + max_size,
				  0xFFFFFFFF)
	} break;
	case CAP_WDR:
		readIdx = vfh->hw_buff.wdr_buff.buff_idx;
		writeIdx = (readIdx + 1) % 2;
		phy_addr = vfh->hw_buff.wdr_buff.phy_addr;
		max_size = vfh->hw_buff.wdr_buff.max_size;

		if (writeIdx == 0)
			WRITE_REG(isp_cur_base, ISP_DMA_12, phy_addr, 0xFFFFFFFF)
		else
			WRITE_REG(isp_cur_base, ISP_DMA_12, phy_addr + max_size, 0xFFFFFFFF)

		break;
	case CAP_LCE:

		readIdx = vfh->hw_buff.lce_buff.buff_idx;
		writeIdx = (readIdx + 1) % 2;
		phy_addr = vfh->hw_buff.lce_buff.phy_addr;
		max_size = vfh->hw_buff.lce_buff.max_size;

		if (writeIdx == 0)
			WRITE_REG(isp_cur_base, ISP_DMA_16, phy_addr, 0xFFFFFFFF)
		else
			WRITE_REG(isp_cur_base, ISP_DMA_16, phy_addr + max_size, 0xFFFFFFFF)
		break;
	case CAP_CNR:

		readIdx = vfh->hw_buff.cnr_buff.buff_idx;
		writeIdx = (readIdx + 1) % 2;
		phy_addr = vfh->hw_buff.cnr_buff.phy_addr;
		max_size = vfh->hw_buff.cnr_buff.max_size;

		if (writeIdx == 0)
			WRITE_REG(isp_cur_base, ISP_DMA_20, phy_addr, 0xFFFFFFFF)
		else
			WRITE_REG(isp_cur_base, ISP_DMA_20, phy_addr + max_size, 0xFFFFFFFF)
		break;
	default:
		break;
	}
}

static u32 isp_video_set_3dnr_flow(struct isp_video_fh *vfh, struct statis_buff *y_buf)
{
	int first_en = 0;
	u32 fun_ctrl_2 = 0;
	int sc_en = 0;
	u32 sc_format = 0; // 0:444, 1:420
	enum out_data_format d3nr_fmt = 0;
	u32 func_en_1 = 0;

	if (y_buf == NULL) {
		printk("input y_buf ptr is NULL %s(%d)\n", __func__, __LINE__);
		goto err_input;
	}

	// get first frame flag
	first_en = y_buf->first_frame;

	if (first_en == 1) {
		// reset 3dnr flags
		isp_video_3dnr_reset(vfh);

		// write first frame register
		WRITE_REG(isp_cur_base, ISP_FUNC_EN_1,
			  FUNC_EN_1_3DNR_FIRST_FRAME_MASK,
			  FUNC_EN_1_3DNR_FIRST_FRAME_MASK)
	} else // disable first frame
		WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0,
			  FUNC_EN_1_3DNR_FIRST_FRAME_MASK)

	// get scaler on/off flag
	READ_REG(isp_cur_base, ISP_FUNC_EN_2, &fun_ctrl_2)
	if (((fun_ctrl_2 & FUNC_EN_2_HSCALE_EN_MASK) == FUNC_EN_2_HSCALE_EN_MASK) ||
		((fun_ctrl_2 & FUNC_EN_2_VSCALE_EN_MASK) == FUNC_EN_2_VSCALE_EN_MASK))
		sc_en = 1;
	y_buf->sc_en = sc_en;

	if (D3NR_REDUCE_BANDWITH && (sc_en == 0)) {
		// set 3dnr ref out enable
		WRITE_REG(isp_cur_base, ISP_DNR_INOUT_FORMAT_CTRL, 0,
			  DNR_INOUT_FORMAT_CTRL_NR3D_YUV_REF_OUT_EN_MASK)
		WRITE_REG(isp_cur_base, ISP_DNR_INOUT_FORMAT_CTRL, 0,
			  DNR_INOUT_FORMAT_CTRL_NR3D_MOTION_REF_OUT_EN_MASK)
	} else {
		// set 3dnr ref out enable
		WRITE_REG(isp_cur_base, ISP_DNR_INOUT_FORMAT_CTRL,
			  DNR_INOUT_FORMAT_CTRL_NR3D_YUV_REF_OUT_EN_MASK,
			  DNR_INOUT_FORMAT_CTRL_NR3D_YUV_REF_OUT_EN_MASK)
		WRITE_REG(isp_cur_base, ISP_DNR_INOUT_FORMAT_CTRL,
			  DNR_INOUT_FORMAT_CTRL_NR3D_MOTION_REF_OUT_EN_MASK,
			  DNR_INOUT_FORMAT_CTRL_NR3D_MOTION_REF_OUT_EN_MASK)
	}

	// get data format
	READ_REG(isp_cur_base, ISP_OUT_FORMAT_CTRL, &sc_format)
	switch (sc_format) {
	case 0:
		d3nr_fmt = CAP_YUV444;
		break;
	case 1:
		d3nr_fmt = CAP_YUV420;
		break;
	case 2:
		d3nr_fmt = CAP_YUV422;
		break;
	case 3:
		d3nr_fmt = CAP_YUV444_P;
		break;
	case 4:
		d3nr_fmt = CAP_YUV420_P;
		break;
	case 5:
		d3nr_fmt = CAP_YUV422_P;
		break;
	default:
		d3nr_fmt = CAP_YUV444;
		break;
	}
	y_buf->d3nr_fmt = d3nr_fmt;

	if ((y_buf->d3nr_fmt == CAP_YUV444_P) ||
		(y_buf->d3nr_fmt == CAP_YUV420_P) ||
		(y_buf->d3nr_fmt == CAP_YUV422_P))
		y_buf->out_sep_en = 1;

	WRITE_REG(isp_cur_base, ISP_DNR_INOUT_FORMAT_CTRL, sc_format,
		  DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_IN_MASK)
	WRITE_REG(isp_cur_base, ISP_DNR_INOUT_FORMAT_CTRL,
		  sc_format << DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_OUT_LSB,
		  DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_OUT_MASK)

err_input:
	READ_REG(isp_cur_base, ISP_FUNC_EN_1, &func_en_1)
	return func_en_1;
}

static u32
isp_video_set_3dnr_addr(struct isp_video_fh *vfh, struct cap_buff *cap_bufq,
			struct out_buff *out_bufq, struct statis_buff *y_buf,
			struct statis_buff *uv_buf, struct statis_buff *v_buf,
			struct statis_buff *mot_buf)
{
	int first_en = 0;
	int sc_en = 0;
	int out_sep_en = 0;
	static u32 last_y_addr = 0;
	static u32 last_uv_addr = 0;
	static u32 last_v_addr = 0;
	u32 phy_addr_temp = 0;
	void *virt_addr_temp = NULL;
	u32 func_en_1 = 0;

	if (y_buf == NULL) {
		printk("input y_buf ptr is NULL %s(%d)\n", __func__, __LINE__);
		goto err_alloc;
	}
	if (uv_buf == NULL) {
		printk("input uv_buf ptr is NULL %s(%d)\n", __func__, __LINE__);
		goto err_alloc;
	}
	if (v_buf == NULL) {
		printk("input v_buf ptr is NULL %s(%d)\n", __func__, __LINE__);
		goto err_alloc;
	}
	if (mot_buf == NULL) {
		printk("input mot_buf ptr is NULL %s(%d)\n", __func__,
			__LINE__);
		goto err_alloc;
	}
	first_en = y_buf->first_frame;
	sc_en = y_buf->sc_en;
	out_sep_en = y_buf->out_sep_en;

	// set 3dnr in/out addr
	if (first_en == 1) {
		y_buf->first_frame = 0;
		last_y_addr = 0;
		last_uv_addr = 0;
		last_v_addr = 0;
	}

	if ((vfh->hw_buff.d3nr_y_buff.max_size == 0) &&
		(vfh->hw_buff.d3nr_mot_buff.max_size == 0)) {
		if (isp_video_alloc_3dnr(vfh, sc_en) < 0) {
			// alloc failed, disable 3dnr
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_3DNR_ENABLE_MASK);
			goto err_alloc;
		}
	}
	if (D3NR_REDUCE_BANDWITH && (sc_en == 0)) {
		// 3dnr y/uv out(t) = default alloc addr(0)
		// 3dnr y/uv in(t) = scaler out(t-1)
		// 3dnr mot in/out swap
		phy_addr_temp = mot_buf->phy_addr_out;
		mot_buf->phy_addr_out = mot_buf->phy_addr;
		mot_buf->phy_addr = phy_addr_temp;

		virt_addr_temp = mot_buf->virt_addr_out;
		mot_buf->virt_addr_out = mot_buf->virt_addr;
		mot_buf->virt_addr = virt_addr_temp;
	} else {
		if (first_en == 0) {
			// 3dnr y/uv/mot in/out swap
			phy_addr_temp = y_buf->phy_addr_out;
			y_buf->phy_addr_out = y_buf->phy_addr;
			y_buf->phy_addr = phy_addr_temp;

			virt_addr_temp = y_buf->virt_addr_out;
			y_buf->virt_addr_out = y_buf->virt_addr;
			y_buf->virt_addr = virt_addr_temp;

			phy_addr_temp = uv_buf->phy_addr_out;
			uv_buf->phy_addr_out = uv_buf->phy_addr;
			uv_buf->phy_addr = phy_addr_temp;

			virt_addr_temp = uv_buf->virt_addr_out;
			uv_buf->virt_addr_out = uv_buf->virt_addr;
			uv_buf->virt_addr = virt_addr_temp;

			if (out_sep_en == 1) {
				phy_addr_temp = v_buf->phy_addr_out;
				v_buf->phy_addr_out = v_buf->phy_addr;
				v_buf->phy_addr = phy_addr_temp;

				virt_addr_temp = v_buf->virt_addr_out;
				v_buf->virt_addr_out = v_buf->virt_addr;
				v_buf->virt_addr = virt_addr_temp;
			}

			phy_addr_temp = mot_buf->phy_addr_out;
			mot_buf->phy_addr_out = mot_buf->phy_addr;
			mot_buf->phy_addr = phy_addr_temp;

			virt_addr_temp = mot_buf->virt_addr_out;
			mot_buf->virt_addr_out = mot_buf->virt_addr;
			mot_buf->virt_addr = virt_addr_temp;
		}
	}

	// set y and mot addr
	if ((y_buf->phy_addr != 0) || (last_y_addr != 0)) {
		if (last_y_addr != 0)
			WRITE_REG(isp_cur_base, ISP_DMA_22, last_y_addr, 0xFFFFFFFF)
		else
			WRITE_REG(isp_cur_base, ISP_DMA_22, y_buf->phy_addr, 0xFFFFFFFF)

		WRITE_REG(isp_cur_base, ISP_DMA_44, mot_buf->phy_addr, 0xFFFFFFFF)
	}

	WRITE_REG(isp_cur_base, ISP_DMA_24, y_buf->phy_addr_out, 0xFFFFFFFF)
	WRITE_REG(isp_cur_base, ISP_DMA_46, mot_buf->phy_addr_out, 0xFFFFFFFF)

	if (D3NR_REDUCE_BANDWITH && (sc_en == 0))
		last_y_addr = (u32)cap_bufq->y_out_buff;

	// set uv or u/v addr
	if (out_sep_en == 0) {
		// uv case
		if ((uv_buf->phy_addr != 0) || (last_uv_addr != 0)) {
			if (last_uv_addr != 0)
				WRITE_REG(isp_cur_base, ISP_DMA_26, last_uv_addr, 0xFFFFFFFF)
			else
				WRITE_REG(isp_cur_base, ISP_DMA_26, uv_buf->phy_addr, 0xFFFFFFFF)
		}

		WRITE_REG(isp_cur_base, ISP_DMA_28, uv_buf->phy_addr_out, 0xFFFFFFFF)

		if (D3NR_REDUCE_BANDWITH && (sc_en == 0))
			last_uv_addr = (u32)cap_bufq->u_out_buff;
	} else {
		// u/v case
		if ((uv_buf->phy_addr != 0) || (last_uv_addr != 0)) {
			if (last_uv_addr != 0) {
				WRITE_REG(isp_cur_base, ISP_DMA_49, last_uv_addr, 0xFFFFFFFF)
				WRITE_REG(isp_cur_base, ISP_DMA_50, last_v_addr, 0xFFFFFFFF)
			} else {
				WRITE_REG(isp_cur_base, ISP_DMA_49, uv_buf->phy_addr, 0xFFFFFFFF)
				WRITE_REG(isp_cur_base, ISP_DMA_50, v_buf->phy_addr, 0xFFFFFFFF)
			}
		}
		WRITE_REG(isp_cur_base, ISP_DMA_51, uv_buf->phy_addr_out, 0xFFFFFFFF)
		WRITE_REG(isp_cur_base, ISP_DMA_52, v_buf->phy_addr_out, 0xFFFFFFFF)

		if (D3NR_REDUCE_BANDWITH && (sc_en == 0)) {
			last_uv_addr = (u32)cap_bufq->u_out_buff;
			last_v_addr = (u32)cap_bufq->v_out_buff;
		}
	}

err_alloc:
	READ_REG(isp_cur_base, ISP_FUNC_EN_1, &func_en_1)
	return func_en_1;
}

static void set_statis_read_phy_addr(struct isp_video_fh *vfh, enum ISP_DMA_ID e_module)
{
	int first = 0;
	int readIdx = 0;
	dma_addr_t phy_addr = 0;
	size_t max_size = 0;

	switch (e_module) {
	case OUT_FCURVE: {
		first = vfh->hw_buff.fcurve_buff.first_frame;

		// fcurve subout enable
		WRITE_REG(isp_cur_base, ISP_FUNC_EN_0,
			  FUNC_EN_0_FCURVE_SUBOUT_EN_MASK,
			  FUNC_EN_0_FCURVE_SUBOUT_EN_MASK);

		if (first == 1) {
			vfh->hw_buff.fcurve_buff.first_frame = 0;

			WRITE_REG(isp_cur_base, ISP_FUNC_EN_0, 0x0,
				  FUNC_EN_0_FCURVE_LOCAL_EN_MASK)

			// fcurve disable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_0, 0x0,
				  FUNC_EN_0_FCURVE_EN_MASK)
		} else {
			readIdx = vfh->hw_buff.fcurve_buff.buff_idx;
			phy_addr = vfh->hw_buff.fcurve_buff.phy_addr;
			max_size = vfh->hw_buff.fcurve_buff.max_size;

			readIdx = (readIdx + 1) % 2;

			vfh->hw_buff.fcurve_buff.buff_idx = readIdx;

			if (readIdx == 0)
				WRITE_REG(isp_cur_base, ISP_DMA_4, phy_addr, 0xFFFFFFFF)
			else
				WRITE_REG(isp_cur_base, ISP_DMA_4, phy_addr + max_size, 0xFFFFFFFF)

			// fcurve enable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_0,
				  FUNC_EN_0_FCURVE_EN_MASK,
				  FUNC_EN_0_FCURVE_EN_MASK)
		}
	} break;

	case OUT_WDR: {
		first = vfh->hw_buff.wdr_buff.first_frame;

		// wdr subout enable
		WRITE_REG(isp_cur_base, ISP_FUNC_EN_1,
			  FUNC_EN_1_WDR_SUBOUT_EN_MASK,
			  FUNC_EN_1_WDR_SUBOUT_EN_MASK);

		if (first == 1) {
			vfh->hw_buff.wdr_buff.first_frame = 0;

			WRITE_REG(isp_cur_base, ISP_WDR_28, 0x33050000, 0xFFFF00FF)

			// wdr disable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_WDR_EN_MASK);
		} else {
			readIdx = vfh->hw_buff.wdr_buff.buff_idx;
			phy_addr = vfh->hw_buff.wdr_buff.phy_addr;
			max_size = vfh->hw_buff.wdr_buff.max_size;

			readIdx = (readIdx + 1) % 2;

			vfh->hw_buff.wdr_buff.buff_idx = readIdx;

			if (readIdx == 0)
				WRITE_REG(isp_cur_base, ISP_DMA_10, phy_addr, 0xFFFFFFFF)
			else
				WRITE_REG(isp_cur_base, ISP_DMA_10, phy_addr + max_size, 0xFFFFFFFF)

			// wdr enable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1,
				  FUNC_EN_1_WDR_EN_MASK, FUNC_EN_1_WDR_EN_MASK);
		}
	} break;

	case OUT_LCE: {
		first = vfh->hw_buff.lce_buff.first_frame;

		// lce subout enable
		WRITE_REG(isp_cur_base, ISP_FUNC_EN_1,
			  FUNC_EN_1_LCE_SUBOUT_ENABLE_MASK,
			  FUNC_EN_1_LCE_SUBOUT_ENABLE_MASK);

		if (first == 1) {
			vfh->hw_buff.lce_buff.first_frame = 0;

			WRITE_REG(isp_cur_base, ISP_LCE_8, 0x0, 0x3F3F3F3F)
			WRITE_REG(isp_cur_base, ISP_LCE_9, 0x0, 0x3F3F3F3F)
			WRITE_REG(isp_cur_base, ISP_LCE_10, 0x0, 0x0000003F)

			// lce disable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_LCE_ENABLE_MASK);
		} else {
			readIdx = vfh->hw_buff.lce_buff.buff_idx;
			phy_addr = vfh->hw_buff.lce_buff.phy_addr;
			max_size = vfh->hw_buff.lce_buff.max_size;

			readIdx = (readIdx + 1) % 2;

			vfh->hw_buff.lce_buff.buff_idx = readIdx;

			if (readIdx == 0)
				WRITE_REG(isp_cur_base, ISP_DMA_14, phy_addr, 0xFFFFFFFF)
			else
				WRITE_REG(isp_cur_base, ISP_DMA_14, phy_addr + max_size, 0xFFFFFFFF)

			// lce enable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1,
				  FUNC_EN_1_LCE_ENABLE_MASK,
				  FUNC_EN_1_LCE_ENABLE_MASK);
		}
	} break;

	case OUT_CNR: {
		first = vfh->hw_buff.cnr_buff.first_frame;

		// cnr subout enable
		WRITE_REG(isp_cur_base, ISP_FUNC_EN_1,
			  FUNC_EN_1_CNR_SUBOUT_ENABLE_MASK,
			  FUNC_EN_1_CNR_SUBOUT_ENABLE_MASK);

		if (first == 1) {
			vfh->hw_buff.cnr_buff.first_frame = 0;

			WRITE_REG(isp_cur_base, ISP_CNR_6, 64, 0xFFFFFFFF)
			WRITE_REG(isp_cur_base, ISP_CNR_7, 0x40000, 0x00FF007F)
			WRITE_REG(isp_cur_base, ISP_CNR_8, 0x3F3F3F3F, 0x3F3F3F3F)
			WRITE_REG(isp_cur_base, ISP_CNR_9, 0x3F3F3F3F, 0x3F3F3F3F)
			WRITE_REG(isp_cur_base, ISP_CNR_10, 0x3F3F3F3F, 0x3F3F3F3F)
			WRITE_REG(isp_cur_base, ISP_CNR_11, 0x3F3F3F3F, 0x3F3F3F3F)
			WRITE_REG(isp_cur_base, ISP_CNR_12, 0x3F, 0x3F)

			// cnr disable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_CNR_ENABLE_MASK);
		} else {
			readIdx = vfh->hw_buff.cnr_buff.buff_idx;
			phy_addr = vfh->hw_buff.cnr_buff.phy_addr;
			max_size = vfh->hw_buff.cnr_buff.max_size;

			readIdx = (readIdx + 1) % 2;

			vfh->hw_buff.cnr_buff.buff_idx = readIdx;

			if (readIdx == 0)
				WRITE_REG(isp_cur_base, ISP_DMA_18, phy_addr, 0xFFFFFFFF)
			else
				WRITE_REG(isp_cur_base, ISP_DMA_18, phy_addr + max_size, 0xFFFFFFFF)

			// cnr enable
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1,
				  FUNC_EN_1_CNR_ENABLE_MASK,
				  FUNC_EN_1_CNR_ENABLE_MASK);
		}
	} break;
	default:
		break;
	}
}

static void isp_video_free_3dnr(struct isp_video_fh *handle)
{
	dma_addr_t phy_addr = 0;
	dma_addr_t phy_addr_out = 0;
	void *virt_addr = NULL;
	size_t buff_size = 0;

	if (handle == NULL) {
		printk("(isp) isp handle is NULL %s(%d)\n", __func__, __LINE__);
		return;
	}

	isp_video_3dnr_reset(handle);

	if (handle->hw_buff.d3nr_y_buff.max_size != 0) {
		virt_addr = handle->hw_buff.d3nr_y_buff.virt_addr;
		if (virt_addr) {
			phy_addr = handle->hw_buff.d3nr_y_buff.phy_addr;
			phy_addr_out = handle->hw_buff.d3nr_y_buff.phy_addr_out;

			if (phy_addr_out < phy_addr)
				phy_addr = phy_addr_out;

			buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_y_buff.max_size * 2);
			dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
			handle->hw_buff.d3nr_y_buff.virt_addr = NULL;
			handle->hw_buff.d3nr_y_buff.virt_addr_out = NULL;
		}
		handle->hw_buff.d3nr_y_buff.max_size = 0;
	}

	if (handle->hw_buff.d3nr_uv_buff.max_size != 0) {
		virt_addr = handle->hw_buff.d3nr_uv_buff.virt_addr;
		if (virt_addr) {
			phy_addr = handle->hw_buff.d3nr_uv_buff.phy_addr;
			phy_addr_out =
				handle->hw_buff.d3nr_uv_buff.phy_addr_out;

			if (phy_addr_out < phy_addr)
				phy_addr = phy_addr_out;

			buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_uv_buff.max_size * 2);
			dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
			handle->hw_buff.d3nr_uv_buff.virt_addr = NULL;
			handle->hw_buff.d3nr_uv_buff.virt_addr_out = NULL;
		}
		handle->hw_buff.d3nr_uv_buff.max_size = 0;
	}

	if (handle->hw_buff.d3nr_v_buff.max_size != 0) {
		virt_addr = handle->hw_buff.d3nr_v_buff.virt_addr;
		if (virt_addr) {
			phy_addr = handle->hw_buff.d3nr_v_buff.phy_addr;
			phy_addr_out = handle->hw_buff.d3nr_v_buff.phy_addr_out;

			if (phy_addr_out < phy_addr)
				phy_addr = phy_addr_out;

			buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_v_buff.max_size * 2);
			dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
			handle->hw_buff.d3nr_v_buff.virt_addr = NULL;
			handle->hw_buff.d3nr_v_buff.virt_addr_out = NULL;
		}
		handle->hw_buff.d3nr_v_buff.max_size = 0;
	}

	if (handle->hw_buff.d3nr_mot_buff.max_size != 0) {
		virt_addr = handle->hw_buff.d3nr_mot_buff.virt_addr;
		if (virt_addr) {
			phy_addr = handle->hw_buff.d3nr_mot_buff.phy_addr;
			phy_addr_out = handle->hw_buff.d3nr_mot_buff.phy_addr_out;

			if (phy_addr_out < phy_addr)
				phy_addr = phy_addr_out;

			buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_mot_buff.max_size * 2);
			dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
			handle->hw_buff.d3nr_mot_buff.virt_addr = NULL;
			handle->hw_buff.d3nr_mot_buff.virt_addr_out = NULL;
		}
		handle->hw_buff.d3nr_mot_buff.max_size = 0;
	}
}

static void isp_video_3dnr_reset(struct isp_video_fh *handle)
{
	if (handle == NULL) {
		printk("(isp) isp handle is NULL %s(%d)\n", __func__, __LINE__);
		return;
	}

	handle->hw_buff.d3nr_y_buff.first_frame = 1;
	handle->hw_buff.d3nr_y_buff.sc_en = 0;
	handle->hw_buff.d3nr_y_buff.d3nr_fmt = 0;
	handle->hw_buff.d3nr_y_buff.out_sep_en = 0;
}

static int isp_video_alloc_3dnr(struct isp_video_fh *handle, int sc_en)
{
	dma_addr_t phy_addr = 0;
	void *virt_addr = NULL;
	size_t buff_size = 0;
	int out_sep_mode = 0;
	u32 width = 0;
	u32 height = 0;

	if (handle == NULL) {
		printk("(isp) isp handle is NULL %s(%d)\n", __func__, __LINE__);
		return -1;
	}

	out_sep_mode = handle->hw_buff.d3nr_y_buff.out_sep_en;

	width = handle->out_format.fmt.pix.width;
	height = handle->out_format.fmt.pix.height;

	if (!D3NR_REDUCE_BANDWITH || (sc_en != 0)) {
		// 3dnr y
		handle->hw_buff.d3nr_y_buff.buff_idx = 1;

		if (width > 0 && height > 0)
			handle->hw_buff.d3nr_y_buff.max_size = D3NR_Y_SIZE_FORMULA(width, height);
		else
			handle->hw_buff.d3nr_y_buff.max_size = D3NR_Y_MAX_SIZE;

		buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_y_buff.max_size * 2);
		virt_addr = dma_alloc_coherent(
			handle->video->isp_dev->dev, buff_size,
			&phy_addr, GFP_KERNEL);

		if (virt_addr == NULL) {
			printk("3dnr y dma alloc failed, buff_size=0x%lx  %s(%d)\n",
				buff_size, __func__, __LINE__);
			return -1;
		}

		memset((u8 *)virt_addr, 0, buff_size);

		handle->hw_buff.d3nr_y_buff.sram_en = 0;
		handle->hw_buff.d3nr_y_buff.phy_addr = phy_addr;
		handle->hw_buff.d3nr_y_buff.virt_addr = virt_addr;

		handle->hw_buff.d3nr_y_buff.phy_addr_out =
			phy_addr + handle->hw_buff.d3nr_y_buff.max_size;
		handle->hw_buff.d3nr_y_buff.virt_addr_out =
			virt_addr + handle->hw_buff.d3nr_y_buff.max_size;

		// 3dnr uv or 3dnr u
		handle->hw_buff.d3nr_uv_buff.buff_idx = 1;

		if (width > 0 && height > 0) {
			if (out_sep_mode == 0)
				handle->hw_buff.d3nr_uv_buff.max_size = D3NR_UV_SIZE_FORMULA(width, height);
			else
				handle->hw_buff.d3nr_uv_buff.max_size = D3NR_U_SIZE_FORMULA(width, height);
		} else {
			if (out_sep_mode == 0)
				handle->hw_buff.d3nr_uv_buff.max_size = D3NR_UV_MAX_SIZE;
			else
				handle->hw_buff.d3nr_uv_buff.max_size = D3NR_U_MAX_SIZE;
		}

		buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_uv_buff.max_size * 2);
		virt_addr = dma_alloc_coherent(
			handle->video->isp_dev->dev, buff_size,
			&phy_addr, GFP_KERNEL);
		if (virt_addr == NULL) {
			printk("3dnr u dma alloc failed, buff_size=0x%lx  %s(%d)\n",
				buff_size, __func__, __LINE__);
			isp_video_free_3dnr(handle);
			return -1;
		}

		memset((u8 *)virt_addr, 0, buff_size);

		handle->hw_buff.d3nr_uv_buff.sram_en = 0;
		handle->hw_buff.d3nr_uv_buff.phy_addr = phy_addr;
		handle->hw_buff.d3nr_uv_buff.virt_addr = virt_addr;

		handle->hw_buff.d3nr_uv_buff.phy_addr_out =
			phy_addr + handle->hw_buff.d3nr_uv_buff.max_size;
		handle->hw_buff.d3nr_uv_buff.virt_addr_out =
			virt_addr + handle->hw_buff.d3nr_uv_buff.max_size;

		// 3dnr v
		if (out_sep_mode != 0) {
			handle->hw_buff.d3nr_v_buff.buff_idx = 1;

			if (width > 0 && height > 0)
				handle->hw_buff.d3nr_v_buff.max_size = D3NR_V_SIZE_FORMULA(width, height);
			else
				handle->hw_buff.d3nr_v_buff.max_size = D3NR_V_MAX_SIZE;

			buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_v_buff.max_size * 2);
			virt_addr = dma_alloc_coherent(
				handle->video->isp_dev->dev, buff_size,
				&phy_addr, GFP_KERNEL);

			if (virt_addr == NULL) {
				printk("3dnr v dma alloc failed, buff_size=0x%lx  %s(%d)\n",
					buff_size, __func__, __LINE__);
				isp_video_free_3dnr(handle);
				return -1;
			}

			memset((u8 *)virt_addr, 0, buff_size);

			handle->hw_buff.d3nr_v_buff.sram_en = 0;
			handle->hw_buff.d3nr_v_buff.phy_addr = phy_addr;
			handle->hw_buff.d3nr_v_buff.virt_addr = virt_addr;

			handle->hw_buff.d3nr_v_buff.phy_addr_out =
				phy_addr + handle->hw_buff.d3nr_v_buff.max_size;
			handle->hw_buff.d3nr_v_buff.virt_addr_out =
				virt_addr + handle->hw_buff.d3nr_v_buff.max_size;
		}

		// 3dnr mot
		handle->hw_buff.d3nr_mot_buff.buff_idx = 1;

		if (width > 0 && height > 0)
			handle->hw_buff.d3nr_mot_buff.max_size = D3NR_MOT_SIZE_FORMULA(width, height);
		else
			handle->hw_buff.d3nr_mot_buff.max_size = D3NR_MOT_MAX_SIZE;

		buff_size = ISP_PAGE_ALIGN(handle->hw_buff.d3nr_mot_buff.max_size * 2);
		virt_addr = dma_alloc_coherent(handle->video->isp_dev->dev,
						buff_size, &phy_addr,
						GFP_KERNEL);

		if (virt_addr == NULL) {
			printk("3dnr mot dma alloc failed, buff_size=0x%lx  %s(%d)\n",
				buff_size, __func__, __LINE__);
			isp_video_free_3dnr(handle);
			return -1;
		}

		memset((u8 *)virt_addr, 0, buff_size);

		handle->hw_buff.d3nr_mot_buff.sram_en = 0;
		handle->hw_buff.d3nr_mot_buff.phy_addr = phy_addr;
		handle->hw_buff.d3nr_mot_buff.virt_addr = virt_addr;

		handle->hw_buff.d3nr_mot_buff.phy_addr_out = phy_addr + handle->hw_buff.d3nr_mot_buff.max_size;
		handle->hw_buff.d3nr_mot_buff.virt_addr_out = virt_addr + handle->hw_buff.d3nr_mot_buff.max_size;

		if (DEBUG_BUFF_MESG) {
			printk("3dnr alloc, handle=0x%px, in_w=%d, in_h=%d, \
				y max_size = 0x%lx,u max_size = 0x%lx, v max_size = 0x%lx, \
				mot max_size = 0x%lx\n ",
				handle, width, height,
				handle->hw_buff.d3nr_y_buff.max_size,
				handle->hw_buff.d3nr_uv_buff.max_size,
				handle->hw_buff.d3nr_v_buff.max_size,
				handle->hw_buff.d3nr_mot_buff.max_size);
		}
	}
	return 0;
}

static void isp_reg_check_fusion(void)
{
#define FS_INDEX_WT_BAYER_MAX (256)
#define FS_GAINS_MAX (16384)
#define FS_GAINL_MAX (16384)

	u32 fs_index_wt_bayer = 0;
	u32 fs_gainS = 0;
	u32 fs_gainL = 0;
	u32 fs_index_wt_bayer_max = FS_INDEX_WT_BAYER_MAX;
	u32 fs_gainS_max = FS_GAINS_MAX;
	u32 fs_gainL_max = FS_GAINL_MAX;

	u32 fs_low_thres = 0;
	u32 fs_high_thres = 0;
	u32 fs_wt_min = 0;
	u32 fs_wt_max = 0;

	READ_REG(isp_cur_base, ISP_HDR_8, &fs_index_wt_bayer)
	fs_index_wt_bayer = (fs_index_wt_bayer & HDR_8_FS_INDEX_WT_BAYER_MASK) >> HDR_8_FS_INDEX_WT_BAYER_LSB;
	if (fs_index_wt_bayer > fs_index_wt_bayer_max) {
		printk("(isp) fs_index_wt_bayer is out of range, val = %d (0-%d)  %s(%d)\n",
			fs_index_wt_bayer, fs_index_wt_bayer_max, __func__, __LINE__);
	}

	READ_REG(isp_cur_base, ISP_HDR_9, &fs_gainS)
	fs_gainL = (fs_gainS & HDR_9_FS_GAINL_MASK) >> HDR_9_FS_GAINL_LSB;
	fs_gainS = (fs_gainS & HDR_9_FS_GAINS_MASK);

	if (fs_gainL > fs_gainL_max) {
		printk("(isp) fs_gainL is out of range, val = %d (0-%d)  %s(%d)\n",
			fs_gainL, fs_gainL_max, __func__, __LINE__);
	}

	if (fs_gainS > fs_gainS_max) {
		printk("(isp) fs_gainS is out of range, val = %d (0-%d)  %s(%d)\n",
			fs_gainS, fs_gainS_max, __func__, __LINE__);
	}

	READ_REG(isp_cur_base, ISP_HDR_10, &fs_low_thres)
	fs_high_thres = (fs_low_thres & HDR_10_FS_HIGH_THRES_MASK) >> HDR_10_FS_HIGH_THRES_LSB;
	fs_low_thres = (fs_low_thres & HDR_10_FS_LOW_THRES_MASK);

	if (fs_low_thres > fs_high_thres) {
		printk("(isp) fs_low_thres is bigger than fs_high_thres, fs_low_thres=%d, fs_high_thres=%d  %s(%d)\n",
			fs_low_thres, fs_high_thres, __func__, __LINE__);
	}

	READ_REG(isp_cur_base, ISP_HDR_11, &fs_wt_min)
	fs_wt_max = (fs_wt_min & HDR_11_FS_WT_MAX_MASK) >> HDR_11_FS_WT_MAX_LSB;
	fs_wt_min = (fs_wt_min & HDR_11_FS_WT_MIN_MASK);

	if (fs_wt_min > fs_wt_max) {
		printk("(isp) fs_wt_min is bigger than fs_wt_max, fs_wt_min=%d, fs_wt_max=%d  %s(%d)\n",
			fs_wt_min, fs_wt_max, __func__, __LINE__);
	}
}

static int isp_reg_check_fcurve(void)
{
#define SUBOUT_BLK_W_MIN (4)
#define SUBOUT_BLK_H_MIN (4)
#define SUBOUT_BLK_W_MAX (32)
#define SUBOUT_BLK_H_MAX (32)
#define SUBIN_FLIT_MAX (9)

	int ret = 0;
	u32 block_w = 0;
	u32 block_h = 0;
	u32 block_w_min = SUBOUT_BLK_W_MIN;
	u32 block_w_max = SUBOUT_BLK_W_MAX;
	u32 block_h_min = SUBOUT_BLK_H_MIN;
	u32 block_h_max = SUBOUT_BLK_H_MAX;
	u32 blk_stcs_fact_h = 0;
	u32 blk_stcs_fact_v = 0;
	u32 scaling_fact_h = 0;
	u32 scaling_fact_v = 0;
	u32 blk_size_norm_div = 0;
	u32 blk_size_h = 0;
	u32 blk_size_v = 0;
	u32 subin_flit_w0 = 0;
	u32 subin_flit_w1 = 0;
	u32 subin_flit_w2 = 0;
	u32 subin_flit_max = SUBIN_FLIT_MAX;
	u32 subin_filt_norm_div = 0;
	u32 fcurve_blk_corner_ul = 0;

	READ_REG(isp_cur_base, ISP_FCURVE_0, &block_w)
	block_h = (block_w & FCURVE_0_FCURVE_SUB_HEIGHT_MASK) >> FCURVE_0_FCURVE_SUB_HEIGHT_LSB;
	block_w = block_w & FCURVE_0_FCURVE_SUB_WIDTH_MASK;

	if ((block_w > block_w_max) || (block_w < block_w_min)) {
		printk("(isp) fcurve sub_out width is out of range, width = %d (%d-%d)  %s(%d)\n",
			block_w, block_w_min, block_w_max, __func__, __LINE__);
		ret = -1;
	}

	if ((block_h > block_h_max) || (block_h < block_h_min)) {
		printk("(isp) fcurve sub_out height is out of range, height = %d (%d-%d)  %s(%d)\n",
			block_h, block_h_min, block_h_max, __func__, __LINE__);
		ret = -1;
	}

	// check block size even / odd
	if ((block_w & 0x1) == 0x1) {
		printk("(isp) fcurve sub_out width is odd, width = %d%s(%d)\n", block_w, __func__, __LINE__);
		ret = -1;
	}

	if ((block_h & 0x1) == 0x1) {
		printk("(isp) fcurve sub_out height is odd, height = %d%s(%d)\n", block_h, __func__, __LINE__);
		ret = -1;
	}

	// check stcs factor range
	READ_REG(isp_cur_base, ISP_FCURVE_1, &blk_stcs_fact_h)
	READ_REG(isp_cur_base, ISP_FCURVE_2, &blk_stcs_fact_v)

	if ((blk_stcs_fact_h & FCURVE_1_FCURVE_BLK_STCS_FACT_H_MASK) == 0) {
		printk("(isp) fcurve blk_stcs_fact_h is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if ((blk_stcs_fact_v & FCURVE_2_FCURVE_BLK_STCS_FACT_V_MASK) == 0) {
		printk("(isp) fcurve blk_stcs_fact_v is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check scaling factor range
	READ_REG(isp_cur_base, ISP_FCURVE_3, &scaling_fact_h)
	scaling_fact_v = (scaling_fact_h & FCURVE_3_FCURVE_SCALING_FACT_V_MASK) >> FCURVE_3_FCURVE_SCALING_FACT_V_LSB;
	scaling_fact_h = (scaling_fact_h & FCURVE_3_FCURVE_SCALING_FACT_H_MASK);

	if (scaling_fact_v == 0) {
		printk("(isp) fcurve scaling_fact_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if (scaling_fact_h == 0) {
		printk("(isp) fcurve scaling_fact_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check block num
	READ_REG(isp_cur_base, ISP_FCURVE_4, &blk_size_h)
	blk_size_v = (blk_size_h & FCURVE_4_FCURVE_BLK_SIZE_V_MASK) >> FCURVE_4_FCURVE_BLK_SIZE_V_LSB;
	blk_size_h = (blk_size_h & FCURVE_4_FCURVE_BLK_SIZE_H_MASK);

	if (blk_size_v == 0) {
		printk("(isp) fcurve blk_size_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if (blk_size_h == 0) {
		printk("(isp) fcurve blk_size_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check norm div
	READ_REG(isp_cur_base, ISP_FCURVE_5, &blk_size_norm_div)
	if ((blk_size_norm_div & FCURVE_5_FCURVE_BLK_SIZE_NORM_DIV_MASK) == 0) {
		printk("(isp) fcurve blk_size_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// subin_filt
	READ_REG(isp_cur_base, ISP_FCURVE_6, &subin_flit_w0)

	subin_filt_norm_div =
		(subin_flit_w0 & FCURVE_6_FCURVE_SUBIN_FILT_NORM_DIV_MASK) >>
		FCURVE_6_FCURVE_SUBIN_FILT_NORM_DIV_LSB;

	if (subin_filt_norm_div == 0) {
		printk("(isp) fcurve subin_filt_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	subin_flit_w1 = (subin_flit_w0 & FCURVE_6_FCURVE_SUBIN_FILT_W1_MASK) >>
			FCURVE_6_FCURVE_SUBIN_FILT_W1_LSB;
	subin_flit_w2 = (subin_flit_w0 & FCURVE_6_FCURVE_SUBIN_FILT_W2_MASK) >>
			FCURVE_6_FCURVE_SUBIN_FILT_W2_LSB;
	subin_flit_w0 = subin_flit_w0 & FCURVE_6_FCURVE_SUBIN_FILT_W0_MASK;

	if (subin_flit_w0 > subin_flit_max) {
		printk("(isp) fcurve w0 is out of range, w0=%d (0-%d) %s(%d)\n",
			subin_flit_w0, subin_flit_max, __func__, __LINE__);
		ret = -1;
	}

	if (subin_flit_w1 > subin_flit_max) {
		printk("(isp) fcurve w1 is out of range, w1=%d (0-%d) %s(%d)\n",
			subin_flit_w1, subin_flit_max, __func__, __LINE__);
		ret = -1;
	}

	if (subin_flit_w2 > subin_flit_max) {
		printk("(isp) fcurve w2 is out of range, w2=%d (0-%d) %s(%d)\n",
			subin_flit_w2, subin_flit_max, __func__, __LINE__);
		ret = -1;
	}

	if ((subin_flit_w0 == 0) && (subin_flit_w1 == 0) &&
		(subin_flit_w2 == 0)) {
		printk("(isp) fcurve w0 w1 w2 is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check fcurve_blk_corner_ul
	READ_REG(isp_cur_base, ISP_FCURVE_13, &fcurve_blk_corner_ul)
	fcurve_blk_corner_ul =
		(fcurve_blk_corner_ul & FCURVE_13_FCURVE_BLK_CORNER_UL_MASK) >>
		FCURVE_13_FCURVE_BLK_CORNER_UL_LSB;

	if (fcurve_blk_corner_ul == 0) {
		printk("(isp) fcurve_blk_corner_ul is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static int isp_reg_check_mlsc(void)
{
	int ret = 0;
#define MLSC_STR_MAX (256)
#define MLSC_BLK_MIN (4)
#define MLSC_BLK_MAX (64)

	u32 mlsc_str = 0;
	u32 mlsc_str_max = MLSC_STR_MAX;

	u32 mlsc_lut_num_x = 0;
	u32 mlsc_lut_num_y = 0;
	u32 mlsc_blk_min = MLSC_BLK_MIN;
	u32 mlsc_blk_max = MLSC_BLK_MAX;

	u32 mlsc_scaling_fact_h = 0;
	u32 mlsc_scaling_fact_v = 0;

	// check mlsc str
	READ_REG(isp_cur_base, ISP_MLSC_0, &mlsc_str)
	mlsc_str = (mlsc_str & MLSC_0_MLSC_STR_MASK);
	if (mlsc_str > mlsc_str_max) {
		printk("(isp) mlsc_str is out of range, val = %d (0-%d)  %s(%d)\n",
			mlsc_str, mlsc_str_max, __func__, __LINE__);
		ret = -1;
	}

	// check mlsc blk
	READ_REG(isp_cur_base, ISP_MLSC_1, &mlsc_lut_num_x)
	mlsc_lut_num_y = (mlsc_lut_num_x & MLSC_1_MLSC_LUT_NUM_Y_MASK) >> MLSC_1_MLSC_LUT_NUM_Y_LSB;
	mlsc_lut_num_x = (mlsc_lut_num_x & MLSC_1_MLSC_LUT_NUM_X_MASK);

	if ((mlsc_lut_num_y > mlsc_blk_max) || (mlsc_lut_num_y < mlsc_blk_min)) {
		printk("(isp) mlsc_lut_num_y is out of range, val = %d (%d-%d)  %s(%d)\n",
			mlsc_lut_num_y, mlsc_blk_min, mlsc_blk_max, __func__, __LINE__);
		ret = -1;
	}

	if ((mlsc_lut_num_x > mlsc_blk_max) || (mlsc_lut_num_x < mlsc_blk_min)) {
		printk("(isp) mlsc_lut_num_x is out of range, val = %d (%d-%d)  %s(%d)\n",
			mlsc_lut_num_x, mlsc_blk_min, mlsc_blk_max, __func__, __LINE__);
		ret = -1;
	}

	// check block size even / odd
	if ((mlsc_lut_num_y & 0x1) == 0x1) {
		printk("(isp) mlsc_lut_num_y is odd, val = %d%s(%d)\n", mlsc_lut_num_y, __func__, __LINE__);
		ret = -1;
	}
	if ((mlsc_lut_num_x & 0x1) == 0x1) {
		printk("(isp) mlsc_lut_num_x is odd, val = %d%s(%d)\n", mlsc_lut_num_x, __func__, __LINE__);
		ret = -1;
	}

	// check mlsc scaling factor
	READ_REG(isp_cur_base, ISP_MLSC_2, &mlsc_scaling_fact_h)
	mlsc_scaling_fact_v =
		(mlsc_scaling_fact_h & MLSC_2_MLSC_SCALING_FACT_V_MASK) >>
		MLSC_2_MLSC_SCALING_FACT_V_LSB;
	mlsc_scaling_fact_h =
		(mlsc_scaling_fact_h & MLSC_2_MLSC_SCALING_FACT_H_MASK);
	if (mlsc_scaling_fact_v == 0) {
		printk("(isp) mlsc_scaling_fact_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}
	if (mlsc_scaling_fact_h == 0) {
		printk("(isp) mlsc_scaling_fact_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static int isp_reg_check_bnr(void)
{
#define BNR_WT_NLM_MAX (32)
	int ret = 0;

	u32 bnr_snr_filt_w0 = 0;
	u32 bnr_snr_filt_w1 = 0;
	u32 bnr_snr_filt_w2 = 0;
	u32 bnr_snr_filt_w3 = 0;
	u32 bnr_snr_filt_w4 = 0;
	u32 bnr_snr_filt_w5 = 0;

	u32 bnr_patch_norm_div = 0;

	u32 bnr_wt_nlm = 0;
	u32 bnr_wt_nlm_max = BNR_WT_NLM_MAX;

	READ_REG(isp_cur_base, ISP_BNR_0, &bnr_snr_filt_w0)
	READ_REG(isp_cur_base, ISP_BNR_1, &bnr_snr_filt_w4)

	bnr_snr_filt_w3 = (bnr_snr_filt_w0 & BNR_0_BNR_SNR_FILT_W3_MASK) >> BNR_0_BNR_SNR_FILT_W3_LSB;
	bnr_snr_filt_w2 = (bnr_snr_filt_w0 & BNR_0_BNR_SNR_FILT_W2_MASK) >> BNR_0_BNR_SNR_FILT_W2_LSB;
	bnr_snr_filt_w1 = (bnr_snr_filt_w0 & BNR_0_BNR_SNR_FILT_W1_MASK) >> BNR_0_BNR_SNR_FILT_W1_LSB;
	bnr_snr_filt_w0 = (bnr_snr_filt_w0 & BNR_0_BNR_SNR_FILT_W0_MASK);

	bnr_snr_filt_w5 = (bnr_snr_filt_w4 & BNR_1_BNR_SNR_FILT_W5_MASK) >> BNR_1_BNR_SNR_FILT_W5_LSB;
	bnr_snr_filt_w4 = (bnr_snr_filt_w4 & BNR_1_BNR_SNR_FILT_W4_MASK);

	if ((bnr_snr_filt_w0 == 0) && (bnr_snr_filt_w1 == 0) &&
		(bnr_snr_filt_w2 == 0) && (bnr_snr_filt_w3 == 0) &&
		(bnr_snr_filt_w4 == 0) && (bnr_snr_filt_w5 == 0)) {
		printk("(isp) bnr_snr_filt_w0 w1 w2 w3 w4 w5 is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_BNR_2, &bnr_patch_norm_div)
	bnr_patch_norm_div =
		(bnr_patch_norm_div & BNR_2_BNR_PATCH_NORM_DIV_MASK) >> BNR_2_BNR_PATCH_NORM_DIV_LSB;

	if (bnr_patch_norm_div == 0) {
		printk("(isp) bnr_patch_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_BNR_3, &bnr_wt_nlm)
	bnr_wt_nlm = (bnr_wt_nlm & BNR_3_BNR_WT_NLM_MASK);
	if (bnr_wt_nlm > bnr_wt_nlm_max) {
		printk("(isp) bnr_wt_nlm is out of range, val = %d  (0-%d)  %s(%d)\n",
			bnr_wt_nlm, bnr_wt_nlm_max, __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static int isp_reg_check_wdr(void)
{
#define WDR_SUBOUT_BLK_W_MIN (4)
#define WDR_SUBOUT_BLK_H_MIN (4)
#define WDR_SUBOUT_BLK_W_MAX (32)
#define WDR_SUBOUT_BLK_H_MAX (32)
#define WDR_Y_MAX (8)
	int ret = 0;
	u32 block_w = 0;
	u32 block_h = 0;
	u32 block_w_min = WDR_SUBOUT_BLK_W_MIN;
	u32 block_w_max = WDR_SUBOUT_BLK_W_MAX;
	u32 block_h_min = WDR_SUBOUT_BLK_H_MIN;
	u32 block_h_max = WDR_SUBOUT_BLK_H_MAX;
	u32 blk_stcs_fact_h = 0;
	u32 blk_stcs_fact_v = 0;
	u32 scaling_fact_h = 0;
	u32 scaling_fact_v = 0;
	u32 blk_size_norm_div = 0;
	u32 blk_size_h = 0;
	u32 blk_size_v = 0;
	u32 subin_flit_w0 = 0;
	u32 subin_flit_w1 = 0;
	u32 subin_flit_w2 = 0;
	u32 subin_filt_norm_div = 0;
	u32 y_max = 0;

	READ_REG(isp_cur_base, ISP_WDR_0, &block_w)
	block_h = (block_w & WDR_0_WDR_SUB_HEIGHT_MASK) >> WDR_0_WDR_SUB_HEIGHT_LSB;
	block_w = block_w & WDR_0_WDR_SUB_WIDTH_MASK;

	if ((block_w > block_w_max) || (block_w < block_w_min)) {
		printk("(isp) wdr sub_out width is out of range, width = %d (%d-%d)  %s(%d)\n",
			block_w, block_w_min, block_w_max, __func__, __LINE__);
		ret = -1;
	}

	if ((block_h > block_h_max) || (block_h < block_h_min)) {
		printk("(isp) wdr sub_out height is out of range, height = %d (%d-%d)  %s(%d)\n",
			block_h, block_h_min, block_h_max, __func__, __LINE__);
		ret = -1;
	}

	// check block size even / odd
	if ((block_w & 0x1) == 0x1) {
		printk("(isp) wdr sub_out width is odd, width = %d%s(%d)\n",block_w, __func__, __LINE__);
		ret = -1;
	}
	if ((block_h & 0x1) == 0x1) {
		printk("(isp) wdr sub_out height is odd, height = %d%s(%d)\n", block_h, __func__, __LINE__);
		ret = -1;
	}

	// check stcs factor range
	READ_REG(isp_cur_base, ISP_WDR_1, &blk_stcs_fact_h)
	READ_REG(isp_cur_base, ISP_WDR_2, &blk_stcs_fact_v)

	if ((blk_stcs_fact_h & WDR_1_WDR_BLK_STCS_FACT_H_MASK) == 0) {
		printk("(isp) wdr blk_stcs_fact_h is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if ((blk_stcs_fact_v & WDR_2_WDR_BLK_STCS_FACT_V_MASK) == 0) {
		printk("(isp) wdr blk_stcs_fact_v is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check scaling factor range
	READ_REG(isp_cur_base, ISP_WDR_3, &scaling_fact_h)
	scaling_fact_v = (scaling_fact_h & WDR_3_WDR_SCALING_FACT_V_MASK) >> WDR_3_WDR_SCALING_FACT_V_LSB;
	scaling_fact_h = (scaling_fact_h & WDR_3_WDR_SCALING_FACT_H_MASK);

	if (scaling_fact_v == 0) {
		printk("(isp) wdr scaling_fact_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if (scaling_fact_h == 0) {
		printk("(isp) wdr scaling_fact_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check block num
	READ_REG(isp_cur_base, ISP_WDR_4, &blk_size_h)
	blk_size_v = (blk_size_h & WDR_4_WDR_BLK_SIZE_V_MASK) >> WDR_4_WDR_BLK_SIZE_V_LSB;
	blk_size_h = (blk_size_h & WDR_4_WDR_BLK_SIZE_H_MASK);
	if (blk_size_v == 0) {
		printk("(isp) wdr blk_size_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}
	if (blk_size_h == 0) {
		printk("(isp) wdr blk_size_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check norm div
	READ_REG(isp_cur_base, ISP_WDR_5, &blk_size_norm_div)
	if ((blk_size_norm_div & WDR_5_WDR_BLK_SIZE_NORM_DIV_MASK) == 0) {
		printk("(isp) wdr blk_size_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// subin_filt
	READ_REG(isp_cur_base, ISP_WDR_6, &subin_flit_w0)

	subin_filt_norm_div = (subin_flit_w0 & WDR_6_WDR_SUBIN_FILT_NORM_DIV_MASK) >>
		WDR_6_WDR_SUBIN_FILT_NORM_DIV_LSB;
	if (subin_filt_norm_div == 0) {
		printk("(isp) wdr subin_filt_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	subin_flit_w1 = (subin_flit_w0 & WDR_6_WDR_SUBIN_FILT_W1_MASK) >> WDR_6_WDR_SUBIN_FILT_W1_LSB;
	subin_flit_w2 = (subin_flit_w0 & WDR_6_WDR_SUBIN_FILT_W2_MASK) >> WDR_6_WDR_SUBIN_FILT_W2_LSB;
	subin_flit_w0 = subin_flit_w0 & WDR_6_WDR_SUBIN_FILT_W0_MASK;
	if ((subin_flit_w0 == 0) && (subin_flit_w1 == 0) && (subin_flit_w2 == 0)) {
		printk("(isp) wdr w0 w1 w2 is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// y max
	READ_REG(isp_cur_base, ISP_WDR_7, &y_max)
	y_max = (y_max & WDR_7_WDR_WT_YMAX_MASK);
	if (y_max > WDR_Y_MAX) {
		printk("(isp) wdr y max is too large, ymax=%d (0-8) %s(%d)\n", y_max, __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static int isp_reg_check_lce(void)
{
#define SUBOUT_BLK_W_MIN (4)
#define SUBOUT_BLK_H_MIN (4)
#define SUBOUT_BLK_W_MAX (32)
#define SUBOUT_BLK_H_MAX (32)

	int ret = 0;
	u32 block_w = 0;
	u32 block_h = 0;
	u32 block_w_min = SUBOUT_BLK_W_MIN;
	u32 block_w_max = SUBOUT_BLK_W_MAX;
	u32 block_h_min = SUBOUT_BLK_H_MIN;
	u32 block_h_max = SUBOUT_BLK_H_MAX;
	u32 blk_stcs_fact_h = 0;
	u32 blk_stcs_fact_v = 0;
	u32 scaling_fact_h = 0;
	u32 scaling_fact_v = 0;
	u32 blk_size_norm_div = 0;
	u32 blk_size_h = 0;
	u32 blk_size_v = 0;
	u32 subin_flit_w0 = 0;
	u32 subin_flit_w1 = 0;
	u32 subin_flit_w2 = 0;
	u32 subin_filt_norm_div = 0;

	READ_REG(isp_cur_base, ISP_LCE_0, &block_w)
	block_h = (block_w & LCE_0_LCE_SUB_HEIGHT_MASK) >> LCE_0_LCE_SUB_HEIGHT_LSB;
	block_w = block_w & LCE_0_LCE_SUB_WIDTH_MASK;

	if ((block_w > block_w_max) || (block_w < block_w_min)) {
		printk("(isp) lce sub_out width is out of range, width = %d (%d-%d)  %s(%d)\n",
			block_w, block_w_min, block_w_max, __func__, __LINE__);
		ret = -1;
	}

	if ((block_h > block_h_max) || (block_h < block_h_min)) {
		printk("(isp) lce sub_out height is out of range, height = %d (%d-%d)  %s(%d)\n",
			block_h, block_h_min, block_h_max, __func__, __LINE__);
		ret = -1;
	}

	// check block size even / odd
	if ((block_w & 0x1) == 0x1) {
		printk("(isp) lce sub_out width is odd, width = %d%s(%d)\n", block_w, __func__, __LINE__);
		ret = -1;
	}
	if ((block_h & 0x1) == 0x1) {
		printk("(isp) lce sub_out height is odd, height = %d%s(%d)\n", block_h, __func__, __LINE__);
		ret = -1;
	}

	// check stcs factor range
	READ_REG(isp_cur_base, ISP_LCE_1, &blk_stcs_fact_h)
	READ_REG(isp_cur_base, ISP_LCE_2, &blk_stcs_fact_v)

	if ((blk_stcs_fact_h & LCE_1_LCE_BLK_STCS_FACT_H_MASK) == 0) {
		printk("(isp) lce blk_stcs_fact_h is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if ((blk_stcs_fact_v & LCE_2_LCE_BLK_STCS_FACT_V_MASK) == 0) {
		printk("(isp) lce blk_stcs_fact_v is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check scaling factor range
	READ_REG(isp_cur_base, ISP_LCE_3, &scaling_fact_h)
	scaling_fact_v = (scaling_fact_h & LCE_3_LCE_SCALING_FACT_V_MASK) >> LCE_3_LCE_SCALING_FACT_V_LSB;
	scaling_fact_h = (scaling_fact_h & LCE_3_LCE_SCALING_FACT_H_MASK);

	if (scaling_fact_v == 0) {
		printk("(isp) lce scaling_fact_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if (scaling_fact_h == 0) {
		printk("(isp) lce scaling_fact_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check block num
	READ_REG(isp_cur_base, ISP_LCE_4, &blk_size_h)
	blk_size_v = (blk_size_h & LCE_4_LCE_BLK_SIZE_V_MASK) >> LCE_4_LCE_BLK_SIZE_V_LSB;
	blk_size_h = (blk_size_h & LCE_4_LCE_BLK_SIZE_H_MASK);
	if (blk_size_v == 0) {
		printk("(isp) lce blk_size_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}
	if (blk_size_h == 0) {
		printk("(isp) lce blk_size_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check norm div
	READ_REG(isp_cur_base, ISP_LCE_5, &blk_size_norm_div)
	if ((blk_size_norm_div & LCE_5_LCE_BLK_SIZE_NORM_DIV_MASK) == 0) {
		printk("(isp) lce blk_size_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// subin_filt
	READ_REG(isp_cur_base, ISP_LCE_6, &subin_flit_w0)

	subin_filt_norm_div = (subin_flit_w0 & LCE_6_LCE_SUBIN_FILT_NORM_DIV_MASK) >>
		LCE_6_LCE_SUBIN_FILT_NORM_DIV_LSB;
	if (subin_filt_norm_div == 0) {
		printk("(isp) lce subin_filt_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	subin_flit_w1 = (subin_flit_w0 & LCE_6_LCE_SUBIN_FILT_W1_MASK) >> LCE_6_LCE_SUBIN_FILT_W1_LSB;
	subin_flit_w2 = (subin_flit_w0 & LCE_6_LCE_SUBIN_FILT_W2_MASK) >> LCE_6_LCE_SUBIN_FILT_W2_LSB;
	subin_flit_w0 = subin_flit_w0 & LCE_6_LCE_SUBIN_FILT_W0_MASK;
	if ((subin_flit_w0 == 0) && (subin_flit_w1 == 0) && (subin_flit_w2 == 0)) {
		printk("(isp) lce w0 w1 w2 is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static int isp_reg_check_cnr(void)
{
#define CNR_SUBOUT_BLK_W_MIN (4)
#define CNR_SUBOUT_BLK_H_MIN (4)
#define CNR_SUBOUT_BLK_W_MAX (511 * 2)
#define CNR_SUBOUT_BLK_H_MAX (511 * 2)
	int ret = 0;
	u32 frame_w = 0;
	u32 frame_h = 0;
	u32 block_w = 0;
	u32 block_h = 0;
	u32 block_w_min = CNR_SUBOUT_BLK_W_MIN;
	u32 block_w_max = CNR_SUBOUT_BLK_W_MAX;
	u32 block_h_min = CNR_SUBOUT_BLK_H_MIN;
	u32 block_h_max = CNR_SUBOUT_BLK_H_MAX;
	u32 blk_stcs_fact_h = 0;
	u32 blk_stcs_fact_v = 0;
	u32 scaling_fact_h = 0;
	u32 scaling_fact_v = 0;
	u32 blk_size_norm_div = 0;
	u32 blk_size_h = 0;
	u32 blk_size_v = 0;

	READ_REG(isp_cur_base, ISP_CONTROL_0, &frame_w)
	frame_h = (frame_w & CONTROL_0_HEIGHT_MASK) >> CONTROL_0_HEIGHT_LSB;
	frame_w = frame_w & CONTROL_0_WIDTH_MASK;

	READ_REG(isp_cur_base, ISP_CNR_0, &block_w)
	block_h = (block_w & CNR_0_CNR_SUB_HEIGHT_MASK) >>
		  CNR_0_CNR_SUB_HEIGHT_LSB;
	block_w = block_w & CNR_0_CNR_SUB_WIDTH_MASK;

	// check block size range
	if ((frame_h >> 2) < block_h_max)
		block_h_max = frame_h >> 2;

	if ((frame_w >> 2) < block_w_max)
		block_w_max = frame_w >> 2;

	if ((block_w > block_w_max) || (block_w < block_w_min)) {
		printk("(isp) cnr_sub_out width is out of range, width = %d (%d-%d)  %s(%d)\n",
			block_w, block_w_min, block_w_max, __func__, __LINE__);
		ret = -1;
	}

	if ((block_h > block_h_max) || (block_h < block_h_min)) {
		printk("(isp) cnr_sub_out height is out of range, height = %d (%d-%d)  %s(%d)\n",
			block_h, block_h_min, block_h_max, __func__, __LINE__);
		ret = -1;
	}

	// check block size even / odd
	if ((block_w & 0x1) == 0x1) {
		printk("(isp) cnr_sub_out width is odd, width = %d%s(%d)\n",
			block_w, __func__, __LINE__);
		ret = -1;
	}
	if ((block_h & 0x1) == 0x1) {
		printk("(isp) cnr_sub_out height is odd, height = %d%s(%d)\n",
			block_h, __func__, __LINE__);
		ret = -1;
	}

	// check stcs factor range
	READ_REG(isp_cur_base, ISP_CNR_1, &blk_stcs_fact_h)
	READ_REG(isp_cur_base, ISP_CNR_2, &blk_stcs_fact_v)

	if ((blk_stcs_fact_h & CNR_1_CNR_BLK_STCS_FACT_H_MASK) == 0) {
		printk("(isp) cnr blk_stcs_fact_h is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if ((blk_stcs_fact_v & CNR_2_CNR_BLK_STCS_FACT_V_MASK) == 0) {
		printk("(isp) cnr blk_stcs_fact_v is zero  %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check scaling factor range
	READ_REG(isp_cur_base, ISP_CNR_3, &scaling_fact_h)
	scaling_fact_v = (scaling_fact_h & CNR_3_CNR_SCALING_FACT_V_MASK) >> CNR_3_CNR_SCALING_FACT_V_LSB;
	scaling_fact_h = (scaling_fact_h & CNR_3_CNR_SCALING_FACT_H_MASK);

	if (scaling_fact_v == 0) {
		printk("(isp) cnr scaling_fact_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	if (scaling_fact_h == 0) {
		printk("(isp) cnr scaling_fact_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check block num
	READ_REG(isp_cur_base, ISP_CNR_4, &blk_size_h)
	blk_size_v = (blk_size_h & CNR_4_CNR_BLK_SIZE_V_MASK) >> CNR_4_CNR_BLK_SIZE_V_LSB;
	blk_size_h = (blk_size_h & CNR_4_CNR_BLK_SIZE_H_MASK);
	if (blk_size_v == 0) {
		printk("(isp) cnr blk_size_v is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}
	if (blk_size_h == 0) {
		printk("(isp) cnr blk_size_h is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	// check norm div
	READ_REG(isp_cur_base, ISP_CNR_5, &blk_size_norm_div)
	if ((blk_size_norm_div & CNR_5_CNR_BLK_SIZE_NORM_DIV_MASK) == 0) {
		printk("(isp) cnr blk_size_norm_div is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static int isp_reg_check_sp(void)
{
	int ret = 0;
#define SP_SAD_LPF_WT_MAX (32)
#define SP_CORING_TH_MAX (128)
#define SP_WT_MAX (32)

	u32 sp_sad_lpf_wt = 0;
	u32 sp_sad_lpf_wt_max = SP_SAD_LPF_WT_MAX;

	u32 sp_sad_clamp_val = 0;
	u32 sp_sad_clamp_inv_norm = 0;

	u32 sp_coring_th_main_sub = 0;
	u32 sp_coring_th_diredge = 0;
	u32 sp_coring_th_2d_flat = 0;
	u32 sp_coring_th_max = SP_CORING_TH_MAX;

	u32 sp_wt_low_main_sub = 0;
	u32 sp_wt_low_diredge = 0;
	u32 sp_wt_low_2d_flat = 0;
	u32 sp_wt_high_main_sub = 0;
	u32 sp_wt_high_diredge = 0;
	u32 sp_wt_high_2d_flat = 0;
	u32 sp_wt_max = SP_WT_MAX;

	// check sp sad lpf wt
	READ_REG(isp_cur_base, ISP_SP_3, &sp_sad_lpf_wt)
	sp_sad_lpf_wt = (sp_sad_lpf_wt & SP_3_SP_SAD_LPF_WT_MASK);
	if (sp_sad_lpf_wt > sp_sad_lpf_wt_max) {
		printk("(isp)  sp_sad_lpf_wt is out of range, val=%d (0-%d) %s(%d)\n",
			sp_sad_lpf_wt, sp_sad_lpf_wt_max, __func__, __LINE__);
		ret = -1;
	}

	// check sp sad clamp val  vs.  inv_norm
	READ_REG(isp_cur_base, ISP_SP_4, &sp_sad_clamp_val)
	sp_sad_clamp_inv_norm = (sp_sad_clamp_val & SP_4_SP_SAD_CLAMP_INV_NORM_MASK) >>
		SP_4_SP_SAD_CLAMP_INV_NORM_LSB;
	sp_sad_clamp_val = (sp_sad_clamp_val & SP_4_SP_SAD_CLAMP_VAL_MASK);
	if (sp_sad_clamp_val == 0 && sp_sad_clamp_inv_norm != (1 << 15)) {
		printk("(isp)  invalid val, sp_sad_clamp_val=0x%x , sp_sad_clamp_inv_norm=0x%x %s(%d)\n",
			sp_sad_clamp_val, sp_sad_clamp_inv_norm, __func__, __LINE__);
		ret = -1;
	}

	// check sp coring th
	READ_REG(isp_cur_base, ISP_SP_5, &sp_coring_th_main_sub)
	sp_coring_th_main_sub = (sp_coring_th_main_sub & SP_5_SP_CORING_TH_MAIN_SUB_MASK);
	if (sp_coring_th_main_sub > sp_coring_th_max) {
		printk("(isp)  sp_coring_th_main_sub is out of range, val=%d (0-%d) %s(%d)\n",
			sp_coring_th_main_sub, sp_coring_th_max, __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_SP_6, &sp_coring_th_diredge)
	sp_coring_th_diredge = (sp_coring_th_diredge & SP_6_SP_CORING_TH_DIREDGE_MASK);
	if (sp_coring_th_diredge > sp_coring_th_max) {
		printk("(isp)  sp_coring_th_diredge is out of range, val=%d (0-%d) %s(%d)\n",
			sp_coring_th_diredge, sp_coring_th_max, __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_SP_7, &sp_coring_th_2d_flat)
	sp_coring_th_2d_flat = (sp_coring_th_2d_flat & SP_7_SP_CORING_TH_2D_FLAT_MASK);
	if (sp_coring_th_2d_flat > sp_coring_th_max) {
		printk("(isp)  sp_coring_th_2d_flat is out of range, val=%d (0-%d) %s(%d)\n",
			sp_coring_th_2d_flat, sp_coring_th_max, __func__, __LINE__);
		ret = -1;
	}

	// check sp wt low
	READ_REG(isp_cur_base, ISP_SP_5, &sp_wt_low_main_sub)
	sp_wt_low_main_sub = (sp_wt_low_main_sub & SP_5_SP_WT_LOW_MAIN_SUB_MASK) >> SP_5_SP_WT_LOW_MAIN_SUB_LSB;
	if (sp_wt_low_main_sub > sp_wt_max) {
		printk("(isp)  sp_wt_low_main_sub is out of range, val=%d (0-%d) %s(%d)\n",
			sp_wt_low_main_sub, sp_wt_max, __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_SP_6, &sp_wt_low_diredge)
	sp_wt_low_diredge = (sp_wt_low_diredge & SP_6_SP_WT_LOW_DIREDGE_MASK) >> SP_6_SP_WT_LOW_DIREDGE_LSB;
	if (sp_wt_low_diredge > sp_wt_max) {
		printk("(isp)  sp_wt_low_diredge is out of range, val=%d (0-%d) %s(%d)\n",
			sp_wt_low_diredge, sp_wt_max, __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_SP_7, &sp_wt_low_2d_flat)
	sp_wt_low_2d_flat = (sp_wt_low_2d_flat & SP_7_SP_WT_LOW_2D_FLAT_MASK) >> SP_7_SP_WT_LOW_2D_FLAT_LSB;
	if (sp_wt_low_2d_flat > sp_wt_max) {
		printk("(isp)  sp_wt_low_2d_flat is out of range, val=%d (0-%d) %s(%d)\n",
			sp_wt_low_2d_flat, sp_wt_max, __func__, __LINE__);
		ret = -1;
	}

	// check sp wt high
	READ_REG(isp_cur_base, ISP_SP_5, &sp_wt_high_main_sub)
	sp_wt_high_main_sub = (sp_wt_high_main_sub & SP_5_SP_WT_HIGH_MAIN_SUB_MASK) >> SP_5_SP_WT_HIGH_MAIN_SUB_LSB;
	if (sp_wt_high_main_sub > sp_wt_max) {
		printk("(isp)  sp_wt_high_main_sub is out of range, val=%d (0-%d) %s(%d)\n",
			sp_wt_high_main_sub, sp_wt_max, __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_SP_6, &sp_wt_high_diredge)
	sp_wt_high_diredge = (sp_wt_high_diredge & SP_6_SP_WT_HIGH_DIREDGE_MASK) >> SP_6_SP_WT_HIGH_DIREDGE_LSB;
	if (sp_wt_high_diredge > sp_wt_max) {
		printk("(isp)  sp_wt_high_diredge is out of range, val=%d (0-%d) %s(%d)\n",
			sp_wt_high_diredge, sp_wt_max, __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_SP_7, &sp_wt_high_2d_flat)
	sp_wt_high_2d_flat = (sp_wt_high_2d_flat & SP_7_SP_WT_HIGH_2D_FLAT_MASK) >> SP_7_SP_WT_HIGH_2D_FLAT_LSB;
	if (sp_wt_high_2d_flat > sp_wt_max) {
		printk("(isp)  sp_wt_high_2d_flat is out of range, val=%d (0-%d) %s(%d)\n",
			sp_wt_high_2d_flat, sp_wt_max, __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static int isp_reg_check_3dnr(void)
{
#define NR3D_SAT_RATIO_MAX (64)
	int ret = 0;
	u32 nr3d_sad_ratio = 0;
	u32 nr3d_sad_ratio_max = NR3D_SAT_RATIO_MAX;
	u32 nr3d_sf_filt_w0 = 0;
	u32 nr3d_sf_filt_w1 = 0;
	u32 nr3d_sf_filt_w2 = 0;
	u32 nr3d_sf_filt_w3 = 0;
	u32 nr3d_sf_filt_w4 = 0;
	u32 nr3d_sf_filt_w5 = 0;

	READ_REG(isp_cur_base, ISP_NR3D_0, &nr3d_sad_ratio)
	nr3d_sad_ratio = (nr3d_sad_ratio & NR3D_0_NR3D_SAD_RATIO_MASK);
	if (nr3d_sad_ratio > nr3d_sad_ratio_max) {
		printk("(isp)  nr3d_sad_ratio is out of range, ratio=%d (0-%d) %s(%d)\n",
			nr3d_sad_ratio, nr3d_sad_ratio_max, __func__, __LINE__);
		ret = -1;
	}

	READ_REG(isp_cur_base, ISP_NR3D_23, &nr3d_sf_filt_w0)
	READ_REG(isp_cur_base, ISP_NR3D_24, &nr3d_sf_filt_w4)

	nr3d_sf_filt_w1 = (nr3d_sf_filt_w0 & NR3D_23_NR3D_SF_FILT_W1_MASK) >> NR3D_23_NR3D_SF_FILT_W1_LSB;
	nr3d_sf_filt_w2 = (nr3d_sf_filt_w0 & NR3D_23_NR3D_SF_FILT_W2_MASK) >> NR3D_23_NR3D_SF_FILT_W2_LSB;
	nr3d_sf_filt_w3 = (nr3d_sf_filt_w0 & NR3D_23_NR3D_SF_FILT_W3_MASK) >> NR3D_23_NR3D_SF_FILT_W3_LSB;
	nr3d_sf_filt_w0 = (nr3d_sf_filt_w0 & NR3D_23_NR3D_SF_FILT_W0_MASK);

	nr3d_sf_filt_w5 = (nr3d_sf_filt_w4 & NR3D_24_NR3D_SF_FILT_W5_MASK) >> NR3D_24_NR3D_SF_FILT_W5_LSB;
	nr3d_sf_filt_w4 = (nr3d_sf_filt_w4 & NR3D_24_NR3D_SF_FILT_W4_MASK);

	if ((nr3d_sf_filt_w0 == 0) && (nr3d_sf_filt_w1 == 0) &&
		(nr3d_sf_filt_w2 == 0) && (nr3d_sf_filt_w3 == 0) &&
		(nr3d_sf_filt_w4 == 0) && (nr3d_sf_filt_w5 == 0)) {
		printk("(isp) 3dnr w0 w1 w2 w3 w4 w5 is zero %s(%d)\n", __func__, __LINE__);
		ret = -1;
	}

	return ret;
}

static void isp_reg_check_h_scaler(void)
{
	u32 x_ratio_inv = 0;

	READ_REG(isp_cur_base, ISP_SCALER_2, &x_ratio_inv)

	x_ratio_inv = (x_ratio_inv & SCALER_2_X_RATIO_INV_MASK);

	if (x_ratio_inv == 0) {
		printk("(isp) x_ratio_inv is zero %s(%d)\n", __func__, __LINE__);
	}
}

static void isp_reg_check_v_scaler(void)
{
	u32 y_ratio_inv = 0;

	READ_REG(isp_cur_base, ISP_SCALER_3, &y_ratio_inv)

	y_ratio_inv = (y_ratio_inv & SCALER_3_Y_RATIO_INV_MASK);

	if (y_ratio_inv == 0) {
		printk("(isp) y_ratio_inv is zero %s(%d)\n", __func__, __LINE__);
	}
}

static void isp_reg_check_all(void)
{
	u32 func_en_0 = 0;
	u32 func_en_1 = 0;
	u32 func_en_2 = 0;
	READ_REG(isp_cur_base, ISP_FUNC_EN_0, &func_en_0)
	READ_REG(isp_cur_base, ISP_FUNC_EN_1, &func_en_1)
	READ_REG(isp_cur_base, ISP_FUNC_EN_2, &func_en_2)

	// fusion
	if ((func_en_0 & FUNC_EN_0_HDR_FUSION_EN_MASK) == FUNC_EN_0_HDR_FUSION_EN_MASK) {
		isp_reg_check_fusion();
	}

	// fcurve
	if (((func_en_0 & FUNC_EN_0_FCURVE_EN_MASK) == FUNC_EN_0_FCURVE_EN_MASK) ||
		((func_en_0 & FUNC_EN_0_FCURVE_SUBOUT_EN_MASK) == FUNC_EN_0_FCURVE_SUBOUT_EN_MASK)) {
		if (isp_reg_check_fcurve() == -1) {
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_0, 0x0, FUNC_EN_0_FCURVE_EN_MASK);
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_0, 0x0, FUNC_EN_0_FCURVE_SUBOUT_EN_MASK);
		}
	}

	// mlsc
	if ((func_en_0 & FUNC_EN_0_MLSC_EN_MASK) == FUNC_EN_0_MLSC_EN_MASK) {
		if (isp_reg_check_mlsc() == -1)
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_0, 0x0, FUNC_EN_0_MLSC_EN_MASK);
	}

	// bnr
	if ((func_en_0 & FUNC_EN_0_BNR_EN_MASK) == FUNC_EN_0_BNR_EN_MASK) {
		if (isp_reg_check_bnr() == -1)
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_0, 0x0, FUNC_EN_0_BNR_EN_MASK);
	}

	// wdr
	if (((func_en_1 & FUNC_EN_1_WDR_EN_MASK) == FUNC_EN_1_WDR_EN_MASK) ||
		((func_en_1 & FUNC_EN_1_WDR_SUBOUT_EN_MASK) == FUNC_EN_1_WDR_SUBOUT_EN_MASK)) {
		if (isp_reg_check_wdr() == -1) {
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_WDR_EN_MASK);
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_WDR_SUBOUT_EN_MASK);
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_WDR_DIFF_CONTROL_EN_MASK);
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_WDR_OUTBLD_EN_MASK);
		}
	}

	// lce
	if (((func_en_1 & FUNC_EN_1_LCE_ENABLE_MASK) == FUNC_EN_1_LCE_ENABLE_MASK) ||
		((func_en_1 & FUNC_EN_1_LCE_SUBOUT_ENABLE_MASK) == FUNC_EN_1_LCE_SUBOUT_ENABLE_MASK)) {
		if (isp_reg_check_lce() == -1) {
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_LCE_ENABLE_MASK);
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_LCE_SUBOUT_ENABLE_MASK);
		}
	}

	// cnr
	if (((func_en_1 & FUNC_EN_1_CNR_ENABLE_MASK) == FUNC_EN_1_CNR_ENABLE_MASK) ||
		((func_en_1 & FUNC_EN_1_CNR_SUBOUT_ENABLE_MASK) == FUNC_EN_1_CNR_SUBOUT_ENABLE_MASK)) {
		if (isp_reg_check_cnr() == -1) {
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_CNR_ENABLE_MASK);
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_CNR_SUBOUT_ENABLE_MASK);
		}
	}

	// sp
	if ((func_en_1 & FUNC_EN_1_SP_ENABLE_MASK) == FUNC_EN_1_SP_ENABLE_MASK) {
		if (isp_reg_check_sp() == -1)
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_SP_ENABLE_MASK);
	}

	// 3dnr
	if ((func_en_1 & FUNC_EN_1_3DNR_ENABLE_MASK) == FUNC_EN_1_3DNR_ENABLE_MASK) {
		if (isp_reg_check_3dnr() == -1)
			WRITE_REG(isp_cur_base, ISP_FUNC_EN_1, 0x0, FUNC_EN_1_3DNR_ENABLE_MASK);
	}

	// scaler
	if ((func_en_2 & FUNC_EN_2_HSCALE_EN_MASK) == FUNC_EN_2_HSCALE_EN_MASK)
		isp_reg_check_h_scaler();

	if ((func_en_2 & FUNC_EN_2_VSCALE_EN_MASK) == FUNC_EN_2_VSCALE_EN_MASK)
		isp_reg_check_v_scaler();
}

static long isp_video_reg_handle(struct isp_video_fh *vfh,
				 struct cap_buff *cap_bufq,
				 struct out_buff *out_bufq, size_t cap_width,
				 size_t cap_height)
{
	u32 func_en_0 = 0;
	u32 func_en_1 = 0;
	long ret = 0;

	isp_cur_base = isp_video_next_reg_buff();
	if (isp_cur_base == NULL) {
		printk("%s: wrong isp hw base addr \n", __func__);
		return -EFAULT;
	}

	if (isp_video_get_reg_buff_idx() == 0) {
		WRITE_REG(isp_cur_base, ISP_APB_REG_SEL, 0x0, 0x1);
	} else {
		WRITE_REG(isp_cur_base, ISP_APB_REG_SEL, 0x1, 0x1);
	}

	// sw reset
	WRITE_REG(isp_cur_base, ISP_SW_RESET_EN, 0xFFFFFFFF, 0xFFFFFFFF);

	// cr
	WRITE_REG(isp_cur_base, ISP_CR_REGION_VLD, 0xFFFFFFFF, 0xFFFFFFFF);

	// write ker buffer to hw buffer
	ret = isp_video_write_reg(vfh, cap_width, cap_height);
	if (ret != 0)
		return -EFAULT;

	// init interrupt
	WRITE_REG(isp_cur_base, ISP_INTERRUPT_EN,
		  INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK,
		  INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK);

	isp_reg_check_all();

	if (out_bufq->input_hdr1_size != 0) {
		WRITE_REG(isp_cur_base, ISP_DMA_0,
			  (u32)out_bufq->input_hdr1_buf, 0xFFFFFFFF);
	} else if (out_bufq->input_rgbir_size != 0) {
		WRITE_REG(isp_cur_base, ISP_DMA_0,
			  (u32)out_bufq->input_rgbir_buf, 0xFFFFFFFF);
	}

	if (out_bufq->input_hdr2_size != 0)
		WRITE_REG(isp_cur_base, ISP_DMA_2,
			  (u32)out_bufq->input_hdr2_buf, 0xFFFFFFFF);

	READ_REG(isp_cur_base, ISP_FUNC_EN_0, &func_en_0)
	READ_REG(isp_cur_base, ISP_FUNC_EN_1, &func_en_1)

	// fcurve
	if ((func_en_0 & FUNC_EN_0_FCURVE_EN_MASK) ==
		FUNC_EN_0_FCURVE_EN_MASK) {
		set_statis_read_phy_addr(vfh, OUT_FCURVE);
		set_statis_write_phy_addr(vfh, CAP_FCURVE);
	}

	// mlsc
	if ((func_en_0 & FUNC_EN_0_MLSC_EN_MASK) == FUNC_EN_0_MLSC_EN_MASK) {
		if (vfh->hw_buff.mlsc_addr == 0)
			printk("%s(%d) no mlsc addr setting.\n", __func__, __LINE__);
		else
			WRITE_REG(isp_cur_base, ISP_DMA_8, (u32)vfh->hw_buff.mlsc_addr, 0xFFFFFFFF);
	}

	// wdr
	if ((func_en_1 & FUNC_EN_1_WDR_EN_MASK) == FUNC_EN_1_WDR_EN_MASK) {
		set_statis_read_phy_addr(vfh, OUT_WDR);
		set_statis_write_phy_addr(vfh, CAP_WDR);
	}

	// lce
	if ((func_en_1 & FUNC_EN_1_LCE_ENABLE_MASK) == FUNC_EN_1_LCE_ENABLE_MASK) {
		set_statis_read_phy_addr(vfh, OUT_LCE);
		set_statis_write_phy_addr(vfh, CAP_LCE);
	}

	// cnr
	if ((func_en_1 & FUNC_EN_1_CNR_ENABLE_MASK) == FUNC_EN_1_CNR_ENABLE_MASK) {
		set_statis_read_phy_addr(vfh, OUT_CNR);
		set_statis_write_phy_addr(vfh, CAP_CNR);
	}

	// 3dnr in/out
	if ((func_en_1 & FUNC_EN_1_3DNR_ENABLE_MASK) == FUNC_EN_1_3DNR_ENABLE_MASK) {
		func_en_1 = isp_video_set_3dnr_flow(vfh, &(vfh->hw_buff.d3nr_y_buff));
	}

	if ((func_en_1 & FUNC_EN_1_3DNR_ENABLE_MASK) == FUNC_EN_1_3DNR_ENABLE_MASK) {
		func_en_1 = isp_video_set_3dnr_addr(
			vfh, cap_bufq, out_bufq, &(vfh->hw_buff.d3nr_y_buff),
			&(vfh->hw_buff.d3nr_uv_buff),
			&(vfh->hw_buff.d3nr_v_buff),
			&(vfh->hw_buff.d3nr_mot_buff));
	}

	// update output frame addr : output_buf
	if (cap_bufq->v_out_size == 0) {
		if (cap_bufq->y_out_size != 0)
			WRITE_REG(isp_cur_base, ISP_DMA_30, (u32)cap_bufq->y_out_buff, 0xFFFFFFFF);

		if (cap_bufq->u_out_size != 0)
			WRITE_REG(isp_cur_base, ISP_DMA_32, (u32)cap_bufq->u_out_buff, 0xFFFFFFFF);
	} else {
		if (cap_bufq->y_out_size != 0)
			WRITE_REG(isp_cur_base, ISP_DMA_30, (u32)cap_bufq->y_out_buff, 0xFFFFFFFF);

		if (cap_bufq->u_out_size != 0)
			WRITE_REG(isp_cur_base, ISP_DMA_47, (u32)cap_bufq->u_out_buff, 0xFFFFFFFF);

		if (cap_bufq->v_out_size != 0)
			WRITE_REG(isp_cur_base, ISP_DMA_48, (u32)cap_bufq->v_out_buff, 0xFFFFFFFF);
	}

	if (cap_bufq->awb_out_size != 0)
		WRITE_REG(isp_cur_base, ISP_DMA_34, (u32)cap_bufq->awb_out_buff, 0xFFFFFFFF);

	if (cap_bufq->ae_ba_size != 0)
		WRITE_REG(isp_cur_base, ISP_DMA_36, (u32)cap_bufq->ae_ba_buff, 0xFFFFFFFF);

	if (cap_bufq->ae_hist_size != 0)
		WRITE_REG(isp_cur_base, ISP_DMA_38, (u32)cap_bufq->ae_hist_buff, 0xFFFFFFFF);

	if (cap_bufq->ae2_ba_size != 0)
		WRITE_REG(isp_cur_base, ISP_DMA_40, (u32)cap_bufq->ae2_ba_buff, 0xFFFFFFFF);

	if (cap_bufq->ae2_hist_size != 0)
		WRITE_REG(isp_cur_base, ISP_DMA_42, (u32)cap_bufq->ae2_hist_buff, 0xFFFFFFFF);

	return ret;
}

static irqreturn_t isp_video_interrupt(int irq, void *dev_id)
{
	u32 intr_status = 0;

	READ_REG(isp_cur_base, ISP_INTERRUPT_OUT, &intr_status);
	if (intr_status == INTERRUPT_OUT_HW_IRQ_MASK) {
		if (DEBUG_BUFF_MESG)
			printk(" (isp) hw intr start %s(%d)\n", __func__, __LINE__);

		// clear interrupt
		WRITE_REG(isp_cur_base, ISP_INTERRUPT_CLR,
			  INTERRUPT_CLR_LAST_PIXEL2VDMA_DONE_CLR_MASK,
			  INTERRUPT_CLR_LAST_PIXEL2VDMA_DONE_CLR_MASK);

		spin_lock_irqsave(&isp_hw_done_lock, flags);
		isp_hw_done_flags = 1;
		spin_unlock_irqrestore(&isp_hw_done_lock, flags);

		wake_up_interruptible(&isp_hw_done_wait);

		if (DEBUG_BUFF_MESG)
			printk("(isp) hw intr end %s(%d)\n", __func__, __LINE__);
	}
	return IRQ_HANDLED;
}

static void isp_video_init_reg_setting(void)
{
	// init interrupt
	WRITE_REG(isp_base, ISP_INTERRUPT_EN,
		  INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK,
		  INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK);
}

static void isp_video_intr_en(u8 en)
{
	// init interrupt
	if (en == 0) {
		WRITE_REG(isp_base, ISP_INTERRUPT_EN, 0x0,
			  INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK)
	} else {
		WRITE_REG(isp_base, ISP_INTERRUPT_EN,
			  INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK,
			  INTERRUPT_EN_LAST_PIXEL2VDMA_DONE_SEL_MASK)
	}
}

static int isp_initialize_modules(struct platform_device *pdev, struct isp_device *isp_dev)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
#ifdef REG_MEM_RESERVED
	struct resource res_haps;
	struct device_node *np_dev;
#else
	struct resource *res;
#endif

#ifdef REG_MEM_RESERVED
	np_dev = of_parse_phandle(np, "memory-region", 0);
	if (!np_dev) {
		printk("No %s specified\n", "memory-region");
		goto done;
	}

	ret = of_address_to_resource(np_dev, 0, &res_haps);
	if (ret) {
		printk("hw reg buf1:No memory address assigned to the region\n");
		goto done;
	}
	if (!request_mem_region(res_haps.start, resource_size(&res_haps), np_dev->full_name)) {
		ret = -ENOMEM;
		of_node_put(np_dev);
		goto done;
	}
	isp_base = ioremap(res_haps.start, resource_size(&res_haps));
	WARN_ON(!isp_base);

#else
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	isp_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(isp_base)) {
		printk("%s(%d): isp base failed.  \n", __func__, __LINE__);
		ret = PTR_ERR(isp_base);
	}
	isp_video_set_reg_base(isp_base, NULL, 1);
#endif
	isp_cur_base = isp_base;

	if (isp_dev == NULL) {
		printk("%s(%d): device ptr is NULL\n", __func__, __LINE__);
		goto done;
	}

	// init interrupt
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk("%s(%d): get irq fail\n", __func__, __LINE__);
		goto done;
	}

	ret = devm_request_irq(isp_dev->dev, irq, isp_video_interrupt, 0,
				dev_name(isp_dev->dev), isp_dev);
	if (ret < 0) {
		printk("%s(%d): request irq fail\n", __func__, __LINE__);
		goto done;
	}

	// supsend/resume relative
#ifdef ISP_PM_RST
	rst = devm_reset_control_get(isp_dev->dev, NULL);
	if (IS_ERR(rst)) {
		pr_err("%s, get devm_reset_control_get() fail\n", __func__);
		rst = NULL;
	} else {
		reset_control_deassert(rst);
	}
#endif

#ifdef ISP_REGULATOR
	regulator = devm_regulator_get(isp_dev->dev, "ext_buck1_0v8");
	if (IS_ERR(regulator)) {
		pr_err("%s, get devm_regulator_get() fail\n", __func__);
		regulator = NULL;
	} else {
		regulator_enable(regulator);
	}
#endif

#ifdef CONFIG_PM
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	pm_runtime_set_active(isp_dev->dev);
	pm_runtime_enable(isp_dev->dev);
	pm_runtime_idle(isp_dev->dev);
#endif
#endif

	// get isp SRAM addr
	if (!np) {
		isp_sram_phy = 0;
		pr_err("%s: can't find device tree node\n", __func__);
	} else if (of_property_read_u32(np, "sram_phy", (u32 *)&isp_sram_phy) < 0) {
		isp_sram_phy = 0;
		if (DEBUG_BUFF_MESG)
			printk("(isp) no SRAM phy %s(%d) \n", __func__, __LINE__);
	}
	if (DEBUG_BUFF_MESG)
		printk("(isp) SRAM phy = 0x%llx %s(%d) \n", isp_sram_phy, __func__, __LINE__);

	// work queue
	isp_wq = alloc_workqueue("isp_hw_setting_wq", WQ_UNBOUND, 1);
	// isp_wq = alloc_workqueue("isp_hw_setting_wq", 0, 0);
	if (!isp_wq) {
		printk("%s(%d): initial wq fail\n", __func__, __LINE__);
		goto done;
	}

	init_waitqueue_head(&isp_hw_done_wait);
	spin_lock_init(&isp_hw_done_lock);
	return 0;

done:

#ifdef ISP_PM_RST
	if (rst) {
		reset_control_assert(rst);
	}
#endif

#ifdef ISP_REGULATOR
	if (regulator) {
		regulator_disable(regulator);
	}
#endif

	return -EPERM;
}

static int isp_register_entities(struct isp_device *isp)
{
	int ret;

	ret = v4l2_device_register(isp->dev, &isp->v4l2_dev);
	if (ret < 0) {
		dev_err(isp->dev, "%s: V4L2 device registration failed (%d)\n", __func__, ret);
		goto done;
	}

done:
	return 0;
}

static int isp_output_queue_setup(struct vb2_queue *queue,
				  unsigned int *num_buffers,
				  unsigned int *num_planes,
				  unsigned int sizes[],
				  struct device *alloc_devs[])
{
	struct isp_video_fh *vfh = vb2_get_drv_priv(queue);
	int idx = 0;

	*num_planes = vfh->out_format.fmt.pix_mp.num_planes;
	for (idx = 0; idx < *num_planes; idx++) {
		sizes[idx] = vfh->out_format.fmt.pix_mp.plane_fmt[idx].sizeimage;
	}
	return 0;
}

static int isp_capture_queue_setup(struct vb2_queue *queue,
				unsigned int *num_buffers,
				unsigned int *num_planes,
				unsigned int sizes[],
				struct device *alloc_devs[])
{
	struct isp_video_fh *vfh = vb2_get_drv_priv(queue);
	int idx = 0;

	*num_planes = vfh->format.fmt.pix_mp.num_planes;
	for (idx = 0; idx < *num_planes; idx++) {
		sizes[idx] = vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage;
	}
	return 0;
}

//  (awb, ae_ba, ae_hist, ae2_ba, ae2_hist)
size_t alloc_statis_buf_size[5] = { 0x4000, 0x800, 0x400, 0x800, 0x400 };

static void isp_video_set_cap_addr(struct isp_video_fh *vfh,
				struct cap_buff *cap_buf,
				struct vb2_buffer *buf,
				struct vb2_v4l2_buffer *v4l2_buf)
{
	int mplane_num = buf->num_planes;
	size_t statis_size =
		sizeof(struct statis_info) + alloc_statis_buf_size[0] +
		alloc_statis_buf_size[1] + alloc_statis_buf_size[2] +
		alloc_statis_buf_size[3] + alloc_statis_buf_size[4];

	switch (mplane_num) {
	case PLANE_Y_UV: {
		u32 data_fmt = vfh->format.fmt.pix_mp.pixelformat;

		if ((V4L2_PIX_FMT_NV12M == data_fmt) || (V4L2_PIX_FMT_NV16M == data_fmt)) {
			// y / uv
			cap_buf->y_out_buff = vb2_dma_contig_plane_dma_addr(buf, 0);
			cap_buf->y_out_virt = vb2_plane_vaddr(buf, 0);
			cap_buf->y_out_size = vb2_plane_size(buf, 0);

			cap_buf->u_out_buff = vb2_dma_contig_plane_dma_addr(buf, 1);
			cap_buf->u_out_virt = vb2_plane_vaddr(buf, 1);
			cap_buf->u_out_size = vb2_plane_size(buf, 1);
		} else {
			// if formate is NV12 NV16 NV24 ( yuv / statis )
			u32 width = vfh->format.fmt.pix_mp.width;
			u32 height = vfh->format.fmt.pix_mp.height;

			cap_buf->y_out_buff = vb2_dma_contig_plane_dma_addr(buf, 0);
			cap_buf->y_out_virt = vb2_plane_vaddr(buf, 0);
			cap_buf->y_out_size = (size_t)width * height;

			cap_buf->u_out_buff = cap_buf->y_out_buff + cap_buf->y_out_size;
			cap_buf->u_out_virt = cap_buf->y_out_virt + cap_buf->y_out_size;
			switch (data_fmt) {
			case V4L2_PIX_FMT_NV12:
				cap_buf->u_out_size = cap_buf->y_out_size >> 1;
				break;
			case V4L2_PIX_FMT_NV16:
				cap_buf->u_out_size = cap_buf->y_out_size;
				break;
			case V4L2_PIX_FMT_NV24:
				cap_buf->u_out_size = cap_buf->y_out_size * 2;
				break;
			}

			// statistic
			if (vb2_plane_size(buf, 1) < statis_size)
				printk("%s(%d) statis ion size too small\n", __func__, __LINE__);
			else {
				cap_buf->awb_out_buff = vb2_dma_contig_plane_dma_addr(buf, 1) + 0x100;
				cap_buf->awb_virt = vb2_plane_vaddr(buf, 1) + 0x100;

				cap_buf->awb_out_size = alloc_statis_buf_size[0];

				cap_buf->statis_dma = v4l2_buf->vb2_buf.planes[1].dbuf;
			}

			if (DEBUG_FORMAT_MESG)
				printk("(isp)  plane statis size = 0x%lx%s(%d)	\n",
					vb2_plane_size(buf, 1), __func__, __LINE__);
		}

		if (DEBUG_FORMAT_MESG) {
			printk("(isp)  plane y size = 0x%lx%s(%d)	\n",
				cap_buf->y_out_size, __func__, __LINE__);
			printk("(isp)  plane uv size = 0x%lx%s(%d)	\n",
				cap_buf->u_out_size, __func__, __LINE__);
		}
	} break;

	case PLANE_Y_UV_STATIS: {
		// y, uv
		cap_buf->y_out_buff = vb2_dma_contig_plane_dma_addr(buf, 0);
		cap_buf->u_out_buff = vb2_dma_contig_plane_dma_addr(buf, 1);

		cap_buf->y_out_virt = vb2_plane_vaddr(buf, 0);
		cap_buf->u_out_virt = vb2_plane_vaddr(buf, 1);

		cap_buf->y_out_size = vb2_plane_size(buf, 0);
		cap_buf->u_out_size = vb2_plane_size(buf, 1);

		// statistic
		if (vb2_plane_size(buf, 2) < statis_size)
			printk("%s(%d) statis ion size too small\n", __func__, __LINE__);
		else {
			cap_buf->awb_out_buff = vb2_dma_contig_plane_dma_addr(buf, 2) + 0x100;
			cap_buf->awb_virt = vb2_plane_vaddr(buf, 2) + 0x100;

			cap_buf->awb_out_size = alloc_statis_buf_size[0];

			cap_buf->statis_dma = v4l2_buf->vb2_buf.planes[2].dbuf;
		}

		if (DEBUG_FORMAT_MESG) {
			printk("(isp)  plane y size = 0x%lx%s(%d)	\n",
				cap_buf->y_out_size, __func__, __LINE__);
			printk("(isp)  plane uv size = 0x%lx%s(%d)	\n",
				cap_buf->u_out_size, __func__, __LINE__);
			printk("(isp)  plane statis size = 0x%lx%s(%d)	\n",
				vb2_plane_size(buf, 2), __func__, __LINE__);
		}
	} break;

	case PLANE_Y_U_V_STATIS: {
		// y, u, v
		cap_buf->y_out_buff = vb2_dma_contig_plane_dma_addr(buf, 0);
		cap_buf->u_out_buff = vb2_dma_contig_plane_dma_addr(buf, 1);
		cap_buf->v_out_buff = vb2_dma_contig_plane_dma_addr(buf, 2);

		cap_buf->y_out_virt = vb2_plane_vaddr(buf, 0);
		cap_buf->u_out_virt = vb2_plane_vaddr(buf, 1);
		cap_buf->v_out_virt = vb2_plane_vaddr(buf, 2);

		cap_buf->y_out_size = vb2_plane_size(buf, 0);
		cap_buf->u_out_size = vb2_plane_size(buf, 1);
		cap_buf->v_out_size = vb2_plane_size(buf, 2);

		// statistic
		if (vb2_plane_size(buf, 3) < statis_size)
			printk("%s(%d) statis ion size too small\n", __func__, __LINE__);
		else {
			cap_buf->awb_out_buff = vb2_dma_contig_plane_dma_addr(buf, 3) + 0x100;
			cap_buf->awb_virt = vb2_plane_vaddr(buf, 3) + 0x100;

			cap_buf->awb_out_size = alloc_statis_buf_size[0];

			cap_buf->statis_dma = v4l2_buf->vb2_buf.planes[3].dbuf;
		}

		if (DEBUG_FORMAT_MESG) {
			printk("(isp)  plane y size = 0x%lx%s(%d)	\n",
				cap_buf->y_out_size, __func__, __LINE__);
			printk("(isp)  plane u size = 0x%lx%s(%d)	\n",
				cap_buf->u_out_size, __func__, __LINE__);
			printk("(isp)  plane v size = 0x%lx%s(%d)	\n",
				cap_buf->v_out_size, __func__, __LINE__);
			printk("(isp)  plane statis size = 0x%lx%s(%d)	\n",
				vb2_plane_size(buf, 3), __func__, __LINE__);
		}
	} break;
	case PLANE_Y_UV_3D_STATIS:
		// y, uv
		cap_buf->y_out_buff = vb2_dma_contig_plane_dma_addr(buf, 0);
		cap_buf->u_out_buff = vb2_dma_contig_plane_dma_addr(buf, 1);

		cap_buf->y_out_virt = vb2_plane_vaddr(buf, 0);
		cap_buf->u_out_virt = vb2_plane_vaddr(buf, 1);

		cap_buf->y_out_size = vb2_plane_size(buf, 0);
		cap_buf->u_out_size = vb2_plane_size(buf, 1);
		// 3d y, 3d uv, 3d mot, statistic
		cap_buf->d3nr_Y_ref_out_buff = vb2_dma_contig_plane_dma_addr(buf, 2);
		cap_buf->d3nr_UV_ref_out_buff = vb2_dma_contig_plane_dma_addr(buf, 3);
		cap_buf->d3nr_motion_cur_out_buff = vb2_dma_contig_plane_dma_addr(buf, 4);

		cap_buf->d3nr_Y_ref_out_size = vb2_plane_size(buf, 2);
		cap_buf->d3nr_UV_ref_out_size = vb2_plane_size(buf, 3);
		cap_buf->d3nr_motion_cur_out_size = vb2_plane_size(buf, 4);
		// statistic
		if (vb2_plane_size(buf, 5) < statis_size)
			printk("%s(%d) statis ion size too small\n", __func__, __LINE__);
		else {
			cap_buf->awb_out_buff = vb2_dma_contig_plane_dma_addr(buf, 5) + 0x100;
			cap_buf->awb_virt = vb2_plane_vaddr(buf, 5) + 0x100;

			cap_buf->awb_out_size = alloc_statis_buf_size[0];

			cap_buf->statis_dma = v4l2_buf->vb2_buf.planes[5].dbuf;
		}

		if (DEBUG_FORMAT_MESG) {
			printk("(isp)  plane y size = 0x%lx%s(%d)	\n",
				cap_buf->y_out_size, __func__, __LINE__);
			printk("(isp)  plane uv size = 0x%lx%s(%d)	\n",
				cap_buf->u_out_size, __func__, __LINE__);
			printk("(isp)  plane 3d y size = 0x%lx%s(%d)	\n",
				cap_buf->d3nr_Y_ref_out_size, __func__,
				__LINE__);
			printk("(isp)  plane 3d uv size = 0x%lx%s(%d)	\n",
				cap_buf->d3nr_UV_ref_out_size, __func__,
				__LINE__);
			printk("(isp)  plane 3d mot size = 0x%lx%s(%d)	\n",
				cap_buf->d3nr_motion_cur_out_size, __func__,
				__LINE__);
			printk("(isp)  plane statis size = 0x%lx%s(%d)	\n",
				vb2_plane_size(buf, 5), __func__, __LINE__);
		}
		break;
	case PLANE_Y_U_V_3D_STATIS:
		// y, u
		cap_buf->y_out_buff = vb2_dma_contig_plane_dma_addr(buf, 0);
		cap_buf->u_out_buff = vb2_dma_contig_plane_dma_addr(buf, 1);

		cap_buf->y_out_virt = vb2_plane_vaddr(buf, 0);
		cap_buf->u_out_virt = vb2_plane_vaddr(buf, 1);
		cap_buf->v_out_virt = vb2_plane_vaddr(buf, 2);

		cap_buf->y_out_size = vb2_plane_size(buf, 0);
		cap_buf->u_out_size = vb2_plane_size(buf, 1);
		// v, 3d y, 3d uv, 3d mot, statistic
		cap_buf->v_out_buff = vb2_dma_contig_plane_dma_addr(buf, 2);
		cap_buf->d3nr_Y_ref_out_buff = vb2_dma_contig_plane_dma_addr(buf, 3);
		cap_buf->d3nr_UV_ref_out_buff = vb2_dma_contig_plane_dma_addr(buf, 4);
		cap_buf->d3nr_motion_cur_out_buff = vb2_dma_contig_plane_dma_addr(buf, 5);

		cap_buf->v_out_size = vb2_plane_size(buf, 2);
		cap_buf->d3nr_Y_ref_out_size = vb2_plane_size(buf, 3);
		cap_buf->d3nr_UV_ref_out_size = vb2_plane_size(buf, 4);
		cap_buf->d3nr_motion_cur_out_size = vb2_plane_size(buf, 5);

		// statistic
		if (vb2_plane_size(buf, 6) < statis_size)
			printk("%s(%d) statis ion size too small\n", __func__, __LINE__);
		else {
			cap_buf->awb_out_buff = vb2_dma_contig_plane_dma_addr(buf, 6) + 0x100;
			cap_buf->awb_virt = vb2_plane_vaddr(buf, 6) + 0x100;

			cap_buf->awb_out_size = alloc_statis_buf_size[0];

			cap_buf->statis_dma = v4l2_buf->vb2_buf.planes[6].dbuf;
		}

		if (DEBUG_FORMAT_MESG) {
			printk("(isp)  plane y size = 0x%lx%s(%d)	\n",
				cap_buf->y_out_size, __func__, __LINE__);
			printk("(isp)  plane u size = 0x%lx%s(%d)	\n",
				cap_buf->u_out_size, __func__, __LINE__);
			printk("(isp)  plane v size = 0x%lx%s(%d)	\n",
				cap_buf->v_out_size, __func__, __LINE__);
			printk("(isp)  plane 3d y size = 0x%lx%s(%d)	\n",
				cap_buf->d3nr_Y_ref_out_size, __func__, __LINE__);
			printk("(isp)  plane 3d uv size = 0x%lx%s(%d)	\n",
				cap_buf->d3nr_UV_ref_out_size, __func__, __LINE__);
			printk("(isp)  plane 3d mot size = 0x%lx%s(%d)	\n",
				cap_buf->d3nr_motion_cur_out_size, __func__, __LINE__);
			printk("(isp)  plane statis size = 0x%lx%s(%d)	\n",
				vb2_plane_size(buf, 6), __func__, __LINE__);
		}
		break;
	default:
		printk("%s(%d)  wrong output dma format\n", __func__, __LINE__);
		break;
	}

	if (cap_buf->awb_out_size > 0) {
		cap_buf->ae_ba_buff = cap_buf->awb_out_buff + alloc_statis_buf_size[0];
		cap_buf->ae_hist_buff = cap_buf->ae_ba_buff + alloc_statis_buf_size[1];
		cap_buf->ae2_ba_buff = cap_buf->ae_hist_buff + alloc_statis_buf_size[2];
		cap_buf->ae2_hist_buff = cap_buf->ae2_ba_buff + alloc_statis_buf_size[3];

		cap_buf->ae_ba_virt = cap_buf->awb_virt + alloc_statis_buf_size[0];
		cap_buf->ae_hist_virt = cap_buf->ae_ba_virt + alloc_statis_buf_size[1];
		cap_buf->ae2_ba_virt = cap_buf->ae_hist_virt + alloc_statis_buf_size[2];
		cap_buf->ae2_hist_virt = cap_buf->ae2_ba_virt + alloc_statis_buf_size[3];

		cap_buf->ae_ba_size = alloc_statis_buf_size[1];
		cap_buf->ae_hist_size = alloc_statis_buf_size[2];
		cap_buf->ae2_ba_size = alloc_statis_buf_size[3];
		cap_buf->ae2_hist_size = alloc_statis_buf_size[4];
	}
}

static void isp_fill_statis_info(struct isp_video_fh *vfh, struct cap_buff *cap_buf)
{
	struct dma_buf *statis_dma = cap_buf->statis_dma;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 47)
	struct iosys_map map;
#endif
	size_t awb_out_size = cap_buf->awb_out_size;
	if (awb_out_size > 0 && statis_dma != NULL) {
		void *statis_vbuf = NULL;
		struct statis_info *statis_info_addr = NULL;

		unsigned int awb_blk_x = 0;
		unsigned int awb_blk_y = 0;

		unsigned int ae_blk_x = 0;
		unsigned int ae_blk_y = 0;

		unsigned int func_en = 0;
		unsigned int awb_en = 0;
		unsigned int ae1_ba_en = 0;
		unsigned int ae2_ba_en = 0;
		unsigned int ae1_hist_en = 0;
		unsigned int ae2_hist_en = 0;

		READ_REG(isp_cur_base, ISP_AWB_0, &awb_blk_x);
		awb_blk_y = (awb_blk_x >> 6) & 0x3F;
		awb_blk_x = awb_blk_x & 0x3F;

		READ_REG(isp_cur_base, ISP_AE_1, &ae_blk_x);
		ae_blk_y = (ae_blk_x >> 25) & 0x1F;
		ae_blk_x = (ae_blk_x >> 20) & 0x1F;

		READ_REG(isp_cur_base, ISP_FUNC_EN_2, &func_en);
		awb_en = (func_en >> FUNC_EN_2_AWB_EN_LSB) & 0x1;
		ae1_ba_en = (func_en >> FUNC_EN_2_AE_EN_LSB) & 0x1;
		ae2_ba_en = (func_en >> FUNC_EN_2_AE_EN2_LSB) & 0x1;
		ae1_hist_en = (func_en >> FUNC_EN_2_AE_HIST_EN_LSB) & 0x1;
		ae2_hist_en = (func_en >> FUNC_EN_2_AE_HIST_EN2_LSB) & 0x1;

		dma_buf_begin_cpu_access(statis_dma, DMA_BIDIRECTIONAL);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 47)
		if (!dma_buf_vmap_unlocked(statis_dma, &map))
			statis_vbuf = (map.is_iomem ? map.vaddr_iomem : map.vaddr);

		// BUG_ON(!iosys_map_is_equal(&statis_dma->vmap_ptr, &map));
#else
		statis_vbuf = dma_buf_vmap(statis_dma);
#endif
		if (statis_vbuf == NULL)
			return;

		statis_info_addr = (struct statis_info *)statis_vbuf;

		statis_info_addr->statis_header.tuning_id = vfh->lastTuningID;

		if (DEBUG_BUFF_MESG)
			printk("(isp) %s(%d) return statis_id=%lld  \n", __func__, __LINE__,
				statis_info_addr->statis_header.tuning_id);

		statis_info_addr->awb_statis.enable = awb_en;
		statis_info_addr->awb_statis.awb_max_size = alloc_statis_buf_size[0];
		statis_info_addr->awb_statis.data_offset = 0x100;

		statis_info_addr->awb_statis.width_blocks = awb_blk_x;
		statis_info_addr->awb_statis.height_blocks = awb_blk_y;

		statis_info_addr->ae1_ba_statis.enable = ae1_ba_en;
		statis_info_addr->ae1_ba_statis.ae_ba_max_size = alloc_statis_buf_size[1];
		statis_info_addr->ae1_ba_statis.data_offset =
			statis_info_addr->awb_statis.data_offset + alloc_statis_buf_size[0];
		statis_info_addr->ae1_ba_statis.width_blocks = ae_blk_x;
		statis_info_addr->ae1_ba_statis.height_blocks = ae_blk_y;

		statis_info_addr->ae1_hist_statis.enable = ae1_hist_en;
		statis_info_addr->ae1_hist_statis.ae_hist_max_size = alloc_statis_buf_size[2];
		statis_info_addr->ae1_hist_statis.data_offset =
			statis_info_addr->ae1_ba_statis.data_offset + alloc_statis_buf_size[1];

		statis_info_addr->ae2_ba_statis.enable = ae2_ba_en;
		statis_info_addr->ae2_ba_statis.ae_ba_max_size = alloc_statis_buf_size[3];
		statis_info_addr->ae2_ba_statis.data_offset =
			statis_info_addr->ae1_hist_statis.data_offset + alloc_statis_buf_size[2];
		statis_info_addr->ae2_ba_statis.width_blocks = ae_blk_x;
		statis_info_addr->ae2_ba_statis.height_blocks = ae_blk_y;

		statis_info_addr->ae2_hist_statis.enable = ae2_hist_en;
		statis_info_addr->ae2_hist_statis.ae_hist_max_size = alloc_statis_buf_size[4];
		statis_info_addr->ae2_hist_statis.data_offset =
			statis_info_addr->ae2_ba_statis.data_offset + alloc_statis_buf_size[3];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 47)
		dma_buf_vunmap_unlocked(statis_dma, &map);
#else
		dma_buf_vunmap_unlocked(statis_dma, statis_vbuf);
#endif
		dma_buf_end_cpu_access(statis_dma, DMA_BIDIRECTIONAL);
	}
}

static void device_run(void *priv)
{
	int r = 0;
	struct isp_video_fh *vfh = priv;

	if (DEBUG_BUFF_MESG)
		printk("(isp) in/out pair start, handle=0x%px  %s(%d)\n", vfh, __func__, __LINE__);
	trace_isp_run_start(vfh);

	r = queue_work(isp_wq, &vfh->work);
	if (r == 0)
		printk("(isp)  %s(%d)  work_queue task exist\n", __func__, __LINE__);

	if (DEBUG_BUFF_MESG)
		printk("(isp) in/out pair end %s(%d)\n", __func__, __LINE__);
	trace_isp_run_end(vfh);
}

static int job_ready(void *priv)
{
	struct isp_video_fh *vfh = priv;
	struct isp_video *video = vfh->video;

	if (v4l2_m2m_num_src_bufs_ready(vfh->m2m_ctx) < vfh->translen ||
		v4l2_m2m_num_dst_bufs_ready(vfh->m2m_ctx) < vfh->translen) {
		v4l2_info(video->vdev.v4l2_dev, "Not enough buffers available\n\n");
		return 0;
	}
	return 1;
}

static void job_abort(void *priv)
{
}

static struct v4l2_m2m_ops m2m_ops = {
	.device_run = device_run,
	.job_ready = job_ready,
	.job_abort = job_abort,
};

static void isp_output_buffer_queue(struct vb2_buffer *buf)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
	struct isp_video_fh *vfh = vb2_get_drv_priv(buf->vb2_queue);

	if (DEBUG_BUFF_MESG)
		printk("(isp) out q start idx=%d  %s(%d) \n", buf->index, __func__, __LINE__);
	trace_isp_out_q_start(buf->index);

	vbuf->sequence = vfh->sequence;
	v4l2_m2m_buf_queue(vfh->m2m_ctx, vbuf);

	if (DEBUG_BUFF_MESG)
		printk("(isp) out q end idx=%d  %s(%d) \n", buf->index, __func__, __LINE__);
	trace_isp_out_q_end(buf->index);
}

static void isp_capture_buffer_queue(struct vb2_buffer *buf)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
	struct isp_video_fh *vfh = vb2_get_drv_priv(buf->vb2_queue);

	if (DEBUG_BUFF_MESG)
		printk("(isp) cap q start idx=%d  %s(%d) \n", buf->index, __func__, __LINE__);
	trace_isp_cap_q_start(buf->index);

	vbuf->sequence = vfh->sequence;
	v4l2_m2m_buf_queue(vfh->m2m_ctx, vbuf);

	if (DEBUG_BUFF_MESG)
		printk("(isp) cap q end idx=%d  %s(%d) \n", buf->index, __func__, __LINE__);
	trace_isp_cap_q_end(buf->index);
}

static const struct vb2_ops isp_video_queue_ops = {
	.queue_setup = isp_capture_queue_setup,
	.buf_queue = isp_capture_buffer_queue,
	// .start_streaming = isp_video_start_streaming,
};

static const struct vb2_ops isp_video_queue_output_ops = {
	.queue_setup = isp_output_queue_setup,
	.buf_queue = isp_output_buffer_queue,
	// .start_streaming = isp_video_start_streaming,
};

static int queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct isp_video_fh *vfh = priv;
	struct isp_video *video = vfh->video;
	struct isp_device *isp_dev = video->isp_dev;
	int ret;

	mutex_init(&vfh->queue_lock);

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = vfh;
	src_vq->ops = &isp_video_queue_output_ops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &isp_dev->dev_mutex;
	src_vq->dev = isp_dev->v4l2_dev.dev;
	src_vq->lock = &vfh->queue_lock;
	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = vfh;
	dst_vq->ops = &isp_video_queue_ops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &isp_dev->dev_mutex;
	dst_vq->dev = isp_dev->v4l2_dev.dev;
	dst_vq->lock = &vfh->queue_lock;
	return vb2_queue_init(dst_vq);
}

static int isp_video_alloc_regQ(struct isp_video_fh *handle)
{
	u8 idx = 0;
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct reg_queue *entry = NULL;
	struct reg_queue *t_entry = NULL;
	struct SettingMmaping *usr_setting = NULL;
	u8 *start_addr = NULL;

	if (handle == NULL) {
		printk("(isp) hdl ptr is NULL.  %s(%d)\n", __func__, __LINE__);
		return -1;
	}

	entry = kzalloc(sizeof(struct reg_queue), GFP_KERNEL);
	if (entry == NULL) {
		printk("%s: isp reg_queue entry alloc failed \n", __func__);
		return -1;
	}

	handle->rq_addr = (u8 *)vmalloc_user((unsigned long)ISP_REGQ_SIZE * 2);
	if (handle->rq_addr == NULL) {
		kfree(entry);
		printk("(isp)  regQ alloc failed  %s(%d)  \n", __func__, __LINE__);
		return -1;
	}

	list_for_each_safe(listptr, listptr_next, &reg_queue_list) {
		t_entry = list_entry(listptr, struct reg_queue, head);
		if (t_entry->regQ_id != idx)
			break;
		else
			idx++;
	}

	handle->regQ_id = idx;
	entry->regQ_id = idx;
	entry->hdl = handle;

	if (t_entry == NULL) // list is empty
		list_add_tail(&entry->head, &reg_queue_list);
	else if (listptr == &reg_queue_list) // no sinkhole
		list_add_tail(&entry->head, &reg_queue_list);
	else // sinkhole
		__list_add(&entry->head, t_entry->head.prev, &(t_entry->head));

	// initial reg buffer data
	memset(handle->rq_addr, 0, ISP_REGQ_SIZE * 2);

	start_addr = handle->rq_addr;
	usr_setting = (struct SettingMmaping *)start_addr;
	usr_setting->header.regQ_idx = idx; // assign handle idx into reg_buffer

	start_addr += ISP_REGQ_SIZE;
	usr_setting = (struct SettingMmaping *)start_addr;
	usr_setting->header.regQ_idx = idx; // assign handle idx into reg_buffer

	return 0;
}

static void isp_video_free_regQ(struct isp_video_fh *handle)
{
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct reg_queue *entry = NULL;

	if (handle == NULL) {
		printk("(isp) hdl ptr is NULL.  %s(%d)\n", __func__, __LINE__);
		return;
	}

	list_for_each_safe(listptr, listptr_next, &reg_queue_list) {
		entry = list_entry(listptr, struct reg_queue, head);
		if (entry->hdl == handle) {
			list_del(listptr);
			kfree(entry);
			break;
		}
	}

	if (handle->rq_addr != NULL) {
		vfree(handle->rq_addr);
		handle->rq_addr = NULL;
	}
}

static int isp_video_open(struct file *file)
{
	struct isp_video *video = video_drvdata(file);
	struct isp_video_fh *handle;
	struct isp_device *isp_dev = video->isp_dev;
	struct v4l2_m2m_ctx *ctx = NULL;
	int ret;

	if (DEBUG_BUFF_MESG)
		printk("(isp)  file ptr=0x%px  %s(%d)	\n", file, __func__, __LINE__);

	if ((video == NULL) || (isp_dev == NULL)) {
		printk("(isp)  video or device ptr is NULL  %s(%d)\n", __func__, __LINE__);
		return -ENOMEM;
	}

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL) {
		printk("(isp)  handle alloc failed  %s(%d)\n", __func__, __LINE__);
		return -ENOMEM;
	}

	if (DEBUG_BUFF_MESG)
		printk("(isp)  handle ptr=0x%px  %s(%d)\n", handle, __func__, __LINE__);

	mutex_init(&handle->process_mtx);

	mutex_lock(&handle->process_mtx);

	// occupy regQ
	mutex_lock(&reg_queue_mtx);

	if (isp_video_alloc_regQ(handle) == -1) {
		printk("(isp)  regQ is not enough %s(%d)\n", __func__, __LINE__);
		mutex_unlock(&reg_queue_mtx);
		mutex_unlock(&handle->process_mtx);
		return -ENOMEM;
	}
	if (DEBUG_BUFF_MESG)
		isp_video_monitor_q_info();

	mutex_unlock(&reg_queue_mtx);

	// init handle
	handle->video = video;

	handle->translen = 1;

	file->private_data = &handle->vfh;

	handle->m2m_dev = g_m2m_dev;

	ctx = v4l2_m2m_ctx_init(handle->m2m_dev, handle, queue_init);
	if (IS_ERR(ctx)) {
		ret = PTR_ERR(ctx);
		goto done;
	}
	handle->m2m_ctx = ctx;

	if (-ENOMEM == isp_alloc_dma_buf(handle)) {
		goto done;
	}

	// init v4l2_ctrl
	ret = isp_init_controls(handle);
	if (ret) {
		v4l2_err(&isp_dev->v4l2_dev, "Failed to init v4l2_ctrl\n");
		goto done;
	}
	v4l2_fh_init(&handle->vfh, &handle->video->vdev);
	v4l2_fh_add(&handle->vfh);
	handle->vfh.m2m_ctx = ctx;
	handle->vfh.vdev = &handle->video->vdev;
	handle->vfh.ctrl_handler = &handle->ctrl_handler;

	// for debug tool
	handle->frame_cnt = 0;

	INIT_LIST_HEAD(&handle->usr_reg_list);

	INIT_WORK(&handle->work, isp_video_hw_setting);

#ifdef CONFIG_PM
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	// pm relative
	pm_runtime_get_sync(video->isp_dev->dev);
#endif
#endif

	isp_video_clk_enable();

	isp_video_init_reg_setting();

	isp_video_clk_disable();

	if (DEBUG_BUFF_MESG)
		printk("(isp)  open over handle=0x%px  %s(%d)	\n", handle, __func__, __LINE__);

	mutex_unlock(&handle->process_mtx);

	return 0;

done:
	// release regQ buffer
	mutex_lock(&reg_queue_mtx);
	isp_video_free_regQ(handle);
	mutex_unlock(&reg_queue_mtx);

	mutex_unlock(&handle->process_mtx);
	v4l2_fh_del(&handle->vfh);
	v4l2_fh_exit(&handle->vfh);
	if (handle != NULL)
		kfree(handle);

	return ret;
}

static int isp_video_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	strscpy(cap->driver, "sp-isp", sizeof(cap->driver));
	strscpy(cap->card, "sp-isp", sizeof(cap->card));
	strscpy(cap->bus_info, "sp-isp", sizeof(cap->bus_info));
	return 0;
}

/* crop */
static int isp_g_selection(struct file *file, void *fh, struct v4l2_selection *sel)
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

static int isp_s_selection(struct file *file, void *fh, struct v4l2_selection *sel)
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

static int isp_get_out_format(struct file *file, void *fh, struct v4l2_format *format)
{
	return 0;
}

static int isp_get_out_format_mplane(struct file *file, void *fh, struct v4l2_format *format)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	int idx = 0;

	format->type = vfh->out_format.type;
	format->fmt.pix_mp.height = vfh->out_format.fmt.pix_mp.height;
	format->fmt.pix_mp.width = vfh->out_format.fmt.pix_mp.width;
	format->fmt.pix_mp.num_planes = vfh->out_format.fmt.pix_mp.num_planes;

	for (idx = 0; idx < format->fmt.pix_mp.num_planes; idx++) {
		format->fmt.pix_mp.plane_fmt[idx].sizeimage = vfh->out_format.fmt.pix_mp.plane_fmt[idx].sizeimage;
		format->fmt.pix_mp.plane_fmt[idx].bytesperline = vfh->out_format.fmt.pix_mp.plane_fmt[idx].bytesperline;
	}
	return 0;
}

static int isp_get_cap_format(struct file *file, void *fh, struct v4l2_format *format)
{
	return 0;
}

static int isp_get_cap_format_mplane(struct file *file, void *fh, struct v4l2_format *format)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	int idx = 0;

	format->type = vfh->format.type;
	format->fmt.pix_mp.height = vfh->format.fmt.pix_mp.height;
	format->fmt.pix_mp.width = vfh->format.fmt.pix_mp.width;
	format->fmt.pix_mp.num_planes = vfh->format.fmt.pix_mp.num_planes;
	format->fmt.pix_mp.pixelformat = vfh->format.fmt.pix_mp.pixelformat;

	for (idx = 0; idx < format->fmt.pix_mp.num_planes; idx++) {
		format->fmt.pix_mp.plane_fmt[idx].sizeimage = vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage;
		format->fmt.pix_mp.plane_fmt[idx].bytesperline = vfh->format.fmt.pix_mp.plane_fmt[idx].bytesperline;
	}
	return 0;
}

static int isp_set_out_format_mplane(struct file *file, void *fh, struct v4l2_format *format)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	int idx = 0;
	struct v4l2_pix_format_mplane *mplane = NULL;

	if (format == NULL) {
		printk("%s(%d)  null pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (format->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		printk("%s(%d) wrong format type\n", __func__, __LINE__);
		return -EINVAL;
	}

	mplane = &format->fmt.pix_mp;

	if (mplane->num_planes < 1 || mplane->num_planes > 8) {
		printk("%s(%d) wrong format mplane num\n", __func__, __LINE__);
		return -EINVAL;
	}

	for (idx = 0; idx < mplane->num_planes; idx++) {
		if (mplane->plane_fmt[idx].sizeimage < 0 || mplane->plane_fmt[idx].bytesperline < 0) {
			printk("%s(%d) wrong mplane(%d) size\n", __func__, __LINE__, idx);
			return -EINVAL;
		}
	}

	vfh->out_format.type = format->type;
	vfh->out_format.fmt.pix_mp.width = mplane->width;
	vfh->out_format.fmt.pix_mp.height = mplane->height;
	vfh->out_format.fmt.pix_mp.num_planes = mplane->num_planes;

	if (DEBUG_FORMAT_MESG) {
		printk("(isp) === out set format mplane===\n");
		printk("(isp) out w = 0x%x (%s)(%d)  \n",
			vfh->out_format.fmt.pix_mp.width, __func__, __LINE__);
		printk("(isp) out h = 0x%x (%s)(%d)  \n",
			vfh->out_format.fmt.pix_mp.height, __func__, __LINE__);
		printk("(isp) out plane num = %d (%s)(%d)  \n",
			vfh->out_format.fmt.pix_mp.num_planes, __func__, __LINE__);
	}

	for (idx = 0; idx < vfh->out_format.fmt.pix_mp.num_planes; idx++) {
		vfh->out_format.fmt.pix_mp.plane_fmt[idx].sizeimage = mplane->plane_fmt[idx].sizeimage;

		vfh->out_format.fmt.pix_mp.plane_fmt[idx].bytesperline = mplane->plane_fmt[idx].bytesperline;

		if (DEBUG_FORMAT_MESG) {
			printk("(isp) plane %d (%s)(%d)  \n", idx, __func__, __LINE__);
			printk("(isp) plane size = 0x%x (%s)(%d)  \n",
				vfh->out_format.fmt.pix_mp.plane_fmt[idx].sizeimage, __func__, __LINE__);
			printk("(isp) plane bytesperline = 0x%x (%s)(%d)  \n",
				vfh->out_format.fmt.pix_mp.plane_fmt[idx].bytesperline, __func__, __LINE__);
		}
	}
	return 0;
}

static int isp_set_cap_format_mplane(struct file *file, void *fh, struct v4l2_format *format)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	int idx = 0;
	struct v4l2_pix_format_mplane *mplane = NULL;

	if (format == NULL) {
		printk("%s(%d)  null pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (format->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		printk("%s(%d) wrong format type\n", __func__, __LINE__);
		return -EINVAL;
	}

	mplane = &format->fmt.pix_mp;

	if (mplane->num_planes < 1 || mplane->num_planes > 8) {
		printk("%s(%d) wrong format mplane num\n", __func__, __LINE__);
		return -EINVAL;
	}

	for (idx = 0; idx < mplane->num_planes; idx++) {
		if (mplane->plane_fmt[idx].sizeimage < 0 ||
			mplane->plane_fmt[idx].bytesperline < 0) {
			printk("%s(%d) wrong mplane(%d) size\n", __func__, __LINE__, idx);
			return -EINVAL;
		}
	}

	vfh->format.type = format->type;
	vfh->format.fmt.pix_mp.width = mplane->width;
	vfh->format.fmt.pix_mp.height = mplane->height;
	vfh->format.fmt.pix_mp.num_planes = mplane->num_planes;
	vfh->format.fmt.pix_mp.pixelformat = mplane->pixelformat;

	if (DEBUG_FORMAT_MESG) {
		printk("(isp) === cap set format mplane===\n");
		printk("(isp) cap w = 0x%x (%s)(%d)  \n",
			vfh->format.fmt.pix_mp.width, __func__, __LINE__);
		printk("(isp) cap h = 0x%x (%s)(%d)  \n",
			vfh->format.fmt.pix_mp.height, __func__, __LINE__);
		printk("(isp) cap plane num = %d (%s)(%d)  \n",
			vfh->format.fmt.pix_mp.num_planes, __func__, __LINE__);
		printk("(isp) cap pixelformat = %s (%s)(%d)  \n",
			fourcc_to_str(vfh->format.fmt.pix_mp.pixelformat), __func__, __LINE__);
	}

	for (idx = 0; idx < vfh->format.fmt.pix_mp.num_planes; idx++) {
		vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage = mplane->plane_fmt[idx].sizeimage;

		vfh->format.fmt.pix_mp.plane_fmt[idx].bytesperline = mplane->plane_fmt[idx].bytesperline;

		if (DEBUG_FORMAT_MESG) {
			printk("(isp) plane %d (%s)(%d)  \n", idx, __func__, __LINE__);
			printk("(isp) plane size = 0x%x (%s)(%d)  \n",
				vfh->format.fmt.pix_mp.plane_fmt[idx].sizeimage, __func__, __LINE__);
			printk("(isp) plane bytesperline = 0x%x (%s)(%d)  \n",
				vfh->format.fmt.pix_mp.plane_fmt[idx].bytesperline, __func__, __LINE__);
		}
	}

	return 0;
}

static int isp_video_set_format_output(struct file *file, void *fh, struct v4l2_format *format)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_pix_format *fmt = NULL;

	if (format == NULL) {
		printk("%s(%d)  null pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (format->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		printk("%s(%d)  wrong format setting\n", __func__, __LINE__);
		return -EINVAL;
	}

	fmt = &format->fmt.pix;
	vfh->out_format.type = format->type;
	vfh->out_format.fmt.pix.width = fmt->width;
	vfh->out_format.fmt.pix.height = fmt->height;
	vfh->out_format.fmt.pix.sizeimage = fmt->sizeimage;
	vfh->out_format.fmt.pix.bytesperline = fmt->bytesperline;
	return 0;
}

static int isp_video_set_format(struct file *file, void *fh, struct v4l2_format *format)
{
	return 0;
}

static int isp_video_try_format(struct file *file, void *fh, struct v4l2_format *format)
{
	return 0;
}

static int isp_video_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *reqbufs)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;
	return v4l2_m2m_reqbufs(file, ctx, reqbufs);
}

static int isp_video_querybuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;
	return v4l2_m2m_querybuf(file, ctx, buf);
}

static int isp_video_expbuf(struct file *file, void *fh, struct v4l2_exportbuffer *eb)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;
	return v4l2_m2m_expbuf(file, ctx, eb);
}

static int isp_video_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;

	int r = 0;
	int idx = 0;

	if (vfh == NULL) {
		printk("(isp) vfh ptr is NULL  %s(%d)	\n", __func__, __LINE__);
		return -1;
	}

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp stream on start, hdl=0x%px  %s(%d)\n", vfh, __func__, __LINE__);

	trace_isp_stream_on_start(vfh);
	mutex_lock(&vfh->process_mtx);

	r = v4l2_m2m_streamon(file, ctx, type);

	if (vfh != NULL) {
		for (idx = 0; idx < MONI_Q_FRAME_NUM; idx++) {
			vfh->in_buff_status[idx] = MONI_INT;
			vfh->out_buff_status[idx] = MONI_INT;
		}
	}
	mutex_unlock(&vfh->process_mtx);

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp stream on end, hdl=0x%px  %s(%d)\n", vfh, __func__, __LINE__);
	trace_isp_stream_on_end(vfh);

	return r;
}

static int isp_video_streamoff(struct file *file, void *fh,
				enum v4l2_buf_type type)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;
	struct v4l2_m2m_dev *m2m_dev = vfh->m2m_dev;
	struct vb2_v4l2_buffer *vbuf;
	unsigned int r = 0;
	u8 *start_addr = NULL;
	struct SettingMmaping *regQ_setting = NULL;

	if (vfh == NULL) {
		printk("(isp) vfh ptr is NULL  %s(%d)	\n", __func__, __LINE__);
		return -1;
	}

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp stream off start,regQ_id=%d  hdl=0x%px  %s(%d)\n",
			vfh->regQ_id, vfh, __func__, __LINE__);
	trace_isp_stream_off_start(vfh);

	cancel_work_sync(&vfh->work);

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp stream off start,regQ_id=%d  hdl=0x%px  no v4l2_m2m_suspend %s(%d)\n",
			vfh->regQ_id, vfh, __func__, __LINE__);

	mutex_lock(&vfh->process_mtx);

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

	r = v4l2_m2m_streamoff(file, ctx, type);

	// reset sub out status
	vfh->hw_buff.fcurve_buff.buff_idx = 1;
	vfh->hw_buff.fcurve_buff.first_frame = 1;

	vfh->hw_buff.wdr_buff.buff_idx = 1;
	vfh->hw_buff.wdr_buff.first_frame = 1;

	vfh->hw_buff.lce_buff.buff_idx = 1;
	vfh->hw_buff.lce_buff.first_frame = 1;

	vfh->hw_buff.cnr_buff.buff_idx = 1;
	vfh->hw_buff.cnr_buff.first_frame = 1;

	isp_video_free_3dnr(vfh);

	vfh->frame_cnt = 0;

	if (vfh->rq_addr != NULL) {
		// reset reg queue status
		start_addr = vfh->rq_addr;
		regQ_setting = (struct SettingMmaping *)start_addr;
		regQ_setting->header.status = BUF_STATUS_NONE;

		start_addr = vfh->rq_addr + ISP_REGQ_SIZE;
		regQ_setting = (struct SettingMmaping *)start_addr;
		regQ_setting->header.status = BUF_STATUS_NONE;
	} else
		printk("(isp) regQ_id=%d rq_addr is NULL %s(%d)\n", vfh->regQ_id, __func__, __LINE__);

	mutex_unlock(&vfh->process_mtx);

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp stream off end,regQ_id=%d  hdl=0x%px  %s(%d)\n",
			vfh->regQ_id, vfh, __func__, __LINE__);
	trace_isp_stream_off_end(vfh);

	return r;
}

static int isp_video_qbuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;
	unsigned int r = 0;
	int buff_idx = buf->index;
	int type = buf->type;

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		type = MONI_CAP;
	else
		type = MONI_OUT;

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp q start (hdl 0x%px)(idx=%d)(type=%d) %s(%d)\n",
			vfh, buff_idx, type, __func__, __LINE__);
	trace_isp_q_start(vfh, buff_idx);

	vfh->sequence = buf->sequence;
	r = v4l2_m2m_qbuf(file, ctx, buf);

	if ((buff_idx < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		if (MONI_CAP == type)
			vfh->out_buff_status[buff_idx] = MONI_Q_BUFF;
		else
			vfh->in_buff_status[buff_idx] = MONI_Q_BUFF;
	}

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp q end (hdl 0x%px)(idx=%d)(type=%d) %s(%d)\n",
			vfh, buff_idx, type, __func__, __LINE__);
	trace_isp_q_end(vfh, buff_idx);

	return r;
}

static int isp_video_dqbuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct isp_video_fh *vfh = to_isp_video_fh(fh);
	struct v4l2_m2m_ctx *ctx = vfh->m2m_ctx;
	unsigned int r = 0;
	int buff_idx = 0;
	int type = buf->type;

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		type = MONI_CAP;
	else
		type = MONI_OUT;

	if (DEBUG_BUFF_MESG)
		printk("(isp) isp dq start (hdl 0x%px)(type=%d) %s(%d)\n",
			vfh, type, __func__, __LINE__);
	trace_isp_dq_start(vfh);

	r = v4l2_m2m_dqbuf(file, ctx, buf);

	if ((buf->index < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		if (MONI_CAP == type)
			vfh->out_buff_status[buf->index] = MONI_DQ_BUFF;
		else
			vfh->in_buff_status[buf->index] = MONI_DQ_BUFF;
	}

	buff_idx = buf->index;
	if (DEBUG_BUFF_MESG)
		printk("(isp) isp dq end (hdl 0x%px)(type=%d)(idx=%d) %s(%d)\n",
			vfh, type, buff_idx, __func__, __LINE__);
	trace_isp_dq_end(vfh, buff_idx);

	return r;
}

static unsigned int isp_poll(struct file *file, struct poll_table_struct *wait)
{
	__poll_t ret;

	if (DEBUG_POLL_MESG)
		printk("(isp) isp poll start %s(%d)\n", __func__, __LINE__);

	ret = v4l2_m2m_fop_poll(file, wait);

	if (DEBUG_POLL_MESG)
		printk("(isp) isp poll end %s(%d) return %d\n", __func__, __LINE__, ret);

	// user can't handle EPOLLERR, just return nothing
	if (ret == EPOLLERR)
		ret = 0;

	return ret;
}

static int isp_video_release(struct file *file)
{
	struct v4l2_fh *vfh = file->private_data;
	struct isp_video_fh *handle = to_isp_video_fh(vfh);
	struct list_head *listptr, *listptr_next = NULL;
	struct usr_reg_head *entry = NULL;
	unsigned int r = 0;

	struct v4l2_m2m_dev *m2m_dev = NULL;
	struct v4l2_m2m_ctx *ctx = NULL;
	struct vb2_v4l2_buffer *vbuf;

	if (DEBUG_BUFF_MESG)
		printk("(isp) file ptr=0x%px  %s(%d)\n", file, __func__, __LINE__);

	if (handle == NULL) {
		printk("(isp) isp handle is NULL %s(%d)\n", __func__, __LINE__);
		return -1;
	}

	if (DEBUG_BUFF_MESG)
		printk("(isp) handle ptr=0x%px  regQ_id=%d  wait hdl mtx %s(%d)\n",
			handle, handle->regQ_id, __func__, __LINE__);

	cancel_work_sync(&handle->work);
	mutex_lock(&handle->process_mtx);

	if (DEBUG_BUFF_MESG)
		printk("(isp) handle ptr=0x%px  regQ_id=%d  in hdl mtx %s(%d)\n",
			handle, handle->regQ_id, __func__, __LINE__);

	v4l2_ctrl_handler_free(&handle->ctrl_handler);

	m2m_dev = handle->m2m_dev;
	ctx = handle->m2m_ctx;

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

	r = v4l2_m2m_streamoff(file, ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	r = v4l2_m2m_streamoff(file, ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);

	// reset sub out status
	handle->hw_buff.fcurve_buff.buff_idx = 1;
	handle->hw_buff.fcurve_buff.first_frame = 1;

	handle->hw_buff.wdr_buff.buff_idx = 1;
	handle->hw_buff.wdr_buff.first_frame = 1;

	handle->hw_buff.lce_buff.buff_idx = 1;
	handle->hw_buff.lce_buff.first_frame = 1;

	handle->hw_buff.cnr_buff.buff_idx = 1;
	handle->hw_buff.cnr_buff.first_frame = 1;

	isp_video_free_3dnr(handle);
	// release buffer
	isp_release_mlsc_buf(handle);
	isp_release_dma_buf(handle);

	handle->lastTuningID = 0;

	list_for_each_safe(listptr, listptr_next, &handle->usr_reg_list) {
		entry = list_entry(listptr, struct usr_reg_head, head);

		list_del(listptr);
		kfree(entry);
	}

	v4l2_m2m_ctx_release(handle->m2m_ctx);

	// remove reg queue
	if (DEBUG_BUFF_MESG)
		printk("(isp) wait regQ_id=%d release  %s(%d)\n", handle->regQ_id, __func__, __LINE__);

	mutex_lock(&reg_queue_mtx);

	if (DEBUG_BUFF_MESG)
		printk("(isp)  regQ_id=%d release  %s(%d)\n",  handle->regQ_id, __func__, __LINE__);

	isp_video_free_regQ(handle);

	if (DEBUG_BUFF_MESG)
		isp_video_monitor_q_info();

	mutex_unlock(&reg_queue_mtx);

#ifdef CONFIG_PM
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	// pm relative
	pm_runtime_put(handle->video->isp_dev->dev);
#endif
#endif

	v4l2_fh_del(&handle->vfh);
	v4l2_fh_exit(&handle->vfh);

	mutex_unlock(&handle->process_mtx);

	if (DEBUG_BUFF_MESG)
		printk("(isp)  release over handle=0x%px  regQ_id=%d  %s(%d)\n",
			handle, handle->regQ_id, __func__, __LINE__);

	kfree(handle);
	file->private_data = NULL;

	return r;
}

static int isp_video_enum_cap_fmt(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f)
{
	if (f->index == 0) {
		// yuv 420
		f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

		// This is a multi-planar, two-plane version of the YUV 4:2:0 format.
		// Variation of V4L2_PIX_FMT_NV12 with planes non contiguous in memory.
		f->pixelformat = V4L2_PIX_FMT_NV12M;

		return 0;
	} else if (f->index == 1) {
		// yuv 444
		f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		f->pixelformat = V4L2_PIX_FMT_NV24;
		return 0;
	} else
		return -EINVAL;
}

static int isp_video_enum_out_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index == 0) {
		// raw 10 pack and unpack
		f->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		f->pixelformat = V4L2_PIX_FMT_SBGGR10;
		return 0;
	} else if (f->index == 1) {
		// raw 12 unpack
		f->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		f->pixelformat = V4L2_PIX_FMT_SBGGR12;
		return 0;
	} else
		return -EINVAL;
}

static int isp_video_try_cap_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	pix_fmt_mp->field = V4L2_FIELD_NONE;

	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return 0;
	else
		return -EINVAL;
}
static int isp_video_try_out_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_fmt_mp = &f->fmt.pix_mp;
	pix_fmt_mp->field = V4L2_FIELD_NONE;

	if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return 0;
	else
		return -EINVAL;
}

static int isp_video_enum_framesizes(struct file *file, void *priv, struct v4l2_frmsizeenum *fsize)
{
	int i = 0;

	if (fsize->index != 0)
		return -EINVAL;
	for (i = 0; i < 4; ++i) {
		if (fsize->pixel_format != isp_framesizes_arr[i].fourcc)
			continue;
		fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
		fsize->stepwise = isp_framesizes_arr[i].stepwise;
		return 0;
	}
	return -EINVAL;
}

static const struct v4l2_ioctl_ops isp_video_ioctl_ops = {
	.vidioc_querycap = isp_video_querycap,
	.vidioc_enum_framesizes = isp_video_enum_framesizes,
	.vidioc_enum_fmt_vid_cap = isp_video_enum_cap_fmt,
	.vidioc_enum_fmt_vid_out = isp_video_enum_out_fmt,
	.vidioc_try_fmt_vid_cap_mplane = isp_video_try_cap_fmt,
	.vidioc_try_fmt_vid_out_mplane = isp_video_try_out_fmt,
	.vidioc_try_fmt_vid_cap = isp_video_try_format,
	.vidioc_try_fmt_vid_out = isp_video_try_format,
	.vidioc_g_fmt_vid_cap = isp_get_cap_format,
	.vidioc_g_fmt_vid_cap_mplane = isp_get_cap_format_mplane,
	.vidioc_s_fmt_vid_cap = isp_video_set_format,
	.vidioc_s_fmt_vid_cap_mplane = isp_set_cap_format_mplane,
	.vidioc_g_fmt_vid_out = isp_get_out_format,
	.vidioc_g_fmt_vid_out_mplane = isp_get_out_format_mplane,
	.vidioc_s_fmt_vid_out = isp_video_set_format_output,
	.vidioc_s_fmt_vid_out_mplane = isp_set_out_format_mplane,
	.vidioc_reqbufs = isp_video_reqbufs,
	.vidioc_querybuf = isp_video_querybuf,
	//.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	// .vidioc_create_bufs = vb2_ioctl_create_bufs,
	// .vidioc_qbuf = vb2_ioctl_qbuf,
	// .vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_qbuf = isp_video_qbuf,
	.vidioc_dqbuf = isp_video_dqbuf,
	.vidioc_expbuf = isp_video_expbuf,
	.vidioc_streamon = isp_video_streamon,
	.vidioc_streamoff = isp_video_streamoff,
	.vidioc_g_selection = isp_g_selection,
	.vidioc_s_selection = isp_s_selection,
};

static const struct v4l2_file_operations isp_video_fops = {
	.owner = THIS_MODULE,
	.open = isp_video_open,
	.release = isp_video_release,
	.poll = isp_poll,
	.unlocked_ioctl = video_ioctl2,
	//.mmap = vb2_fop_mmap,
	.mmap = isp_video_mmap,
};

static struct video_device isp_videodev = {
	.vfl_dir = VFL_DIR_M2M,
	.device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE,
	.fops = &isp_video_fops,
	.ioctl_ops = &isp_video_ioctl_ops,
	.release = video_device_release_empty,
};

static void isp_video_set_out_addr(struct out_buff *out_buf, struct vb2_buffer *buf)
{
	int mplane_num = buf->num_planes;

	switch (mplane_num) {
	case PLANE_HDR1:
		// hdr 1
		out_buf->input_hdr1_buf = vb2_dma_contig_plane_dma_addr(buf, 0);
		out_buf->input_hdr1_virt = vb2_plane_vaddr(buf, 0);
		out_buf->input_hdr1_size = vb2_plane_size(buf, 0);
		break;

	case PLANE_HDR1_2:
		// hdr 1
		out_buf->input_hdr1_buf = vb2_dma_contig_plane_dma_addr(buf, 0);
		out_buf->input_hdr1_virt = vb2_plane_vaddr(buf, 0);
		out_buf->input_hdr1_size = vb2_plane_size(buf, 0);

		// hdr2
		out_buf->input_hdr2_buf = vb2_dma_contig_plane_dma_addr(buf, 1);
		out_buf->input_hdr2_virt = vb2_plane_vaddr(buf, 1);
		out_buf->input_hdr2_size = vb2_plane_size(buf, 1);
		break;

	case PLANE_HDR1_3D:
		// hdr 1
		out_buf->input_hdr1_buf = vb2_dma_contig_plane_dma_addr(buf, 0);
		out_buf->input_hdr1_virt = vb2_plane_vaddr(buf, 0);
		out_buf->input_hdr1_size = vb2_plane_size(buf, 0);

		// 3d
		out_buf->d3nr_Y_ref_in_dma_buf = vb2_dma_contig_plane_dma_addr(buf, 1);
		out_buf->d3nr_Y_ref_in_dma_size = vb2_plane_size(buf, 1);

		out_buf->d3nr_UV_ref_in_dma_buf = vb2_dma_contig_plane_dma_addr(buf, 2);
		out_buf->d3nr_UV_ref_in_dma_size = vb2_plane_size(buf, 2);

		out_buf->d3nr_motion_ref_in_dma_buf = vb2_dma_contig_plane_dma_addr(buf, 3);
		out_buf->d3nr_motion_ref_in_dma_size = vb2_plane_size(buf, 3);
		break;

	case PLANE_HDR1_2_3D:
		// hdr 1
		out_buf->input_hdr1_buf = vb2_dma_contig_plane_dma_addr(buf, 0);
		out_buf->input_hdr1_virt = vb2_plane_vaddr(buf, 0);
		out_buf->input_hdr1_size = vb2_plane_size(buf, 0);

		// hdr2
		out_buf->input_hdr2_buf = vb2_dma_contig_plane_dma_addr(buf, 1);
		out_buf->input_hdr2_virt = vb2_plane_vaddr(buf, 1);
		out_buf->input_hdr2_size = vb2_plane_size(buf, 1);

		// 3d
		out_buf->d3nr_Y_ref_in_dma_buf = vb2_dma_contig_plane_dma_addr(buf, 2);
		out_buf->d3nr_Y_ref_in_dma_size = vb2_plane_size(buf, 2);

		out_buf->d3nr_UV_ref_in_dma_buf = vb2_dma_contig_plane_dma_addr(buf, 3);
		out_buf->d3nr_UV_ref_in_dma_size = vb2_plane_size(buf, 3);

		out_buf->d3nr_motion_ref_in_dma_buf = vb2_dma_contig_plane_dma_addr(buf, 4);
		out_buf->d3nr_motion_ref_in_dma_size = vb2_plane_size(buf, 4);
		break;
	default:
		break;
	}
}

static int get_buf_info(struct isp_video_fh *vfh)
{
	struct vb2_v4l2_buffer *src_buf = NULL, *dst_buf = NULL;
	struct dma_buf *src_dma, *dst_dma;
	size_t length = 0;

	if (vfh == NULL) {
		printk(" %s(%d) vfh is NULL \n", __func__, __LINE__);
		return -1;
	}

	// find the corresponding src_buf and dst_buf
	src_buf = v4l2_m2m_next_src_buf(vfh->m2m_ctx);
	dst_buf = v4l2_m2m_next_dst_buf(vfh->m2m_ctx);

	if ((src_buf == NULL) || (dst_buf == NULL)) {
		printk(" %s(%d) src_buff or dst_buf is NULL\n", __func__, __LINE__);
		return -1;
	}

	length = dst_buf->planes[0].length;
	if (length <= 0) {
		printk(" %s(%d)  wrong buf length \n", __func__, __LINE__);
		return -1;
	}

	src_dma = src_buf->vb2_buf.planes[0].dbuf;
	dst_dma = dst_buf->vb2_buf.planes[0].dbuf;
	if (src_dma == NULL || dst_dma == NULL) {
		printk(" %s(%d)  wrong buf addr \n", __func__, __LINE__);
		return -1;
	}

	if (DEBUG_BUFF_MESG)
		printk("(isp) get buf info start (idx = %d) %s(%d)\n",
			src_buf->vb2_buf.index, __func__, __LINE__);

	isp_data.src_buf = src_buf;
	isp_data.dst_buf = dst_buf;
	isp_data.cap_width = vfh->format.fmt.pix_mp.width;
	isp_data.cap_height = vfh->format.fmt.pix_mp.height;

	if ((src_buf->vb2_buf.index < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		vfh->in_buff_status[src_buf->vb2_buf.index] = MONI_Q_PAIR;
	}
	if ((dst_buf->vb2_buf.index < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		vfh->out_buff_status[dst_buf->vb2_buf.index] = MONI_Q_PAIR;
	}

	if (DEBUG_BUFF_MESG)
		printk("(isp) get buf info end (idx = %d) %s(%d)\n",
			src_buf->vb2_buf.index, __func__, __LINE__);

	return 0;
}

static void isp_video_hw_setting(struct work_struct *work)
{
	struct isp_video_fh *vfh =
		container_of((void *)work, struct isp_video_fh, work);
	struct v4l2_m2m_dev *m2m_dev = NULL;
	struct vb2_buffer *src_buff = NULL;
	struct vb2_buffer *dst_buff = NULL;
	struct vb2_v4l2_buffer *dst_v4l2_buff = NULL;
	struct out_buff out_buf = { 0 };
	struct cap_buff cap_buf = { 0 };
	size_t cap_width = 0;
	size_t cap_height = 0;
	long timeout = 0;

	if (vfh == NULL) {
		printk("(isp) get vfh failed %s(%d)\n", __func__, __LINE__);
		return;
	}

	if (DEBUG_BUFF_MESG)
		printk(" (isp) wait process mtx, hdl=0x%px  %s(%d)\n", vfh, __func__, __LINE__);

	mutex_lock(&vfh->process_mtx);

	if (DEBUG_BUFF_MESG)
		printk(" (isp) wait write reg, hdl=0x%px  %s(%d)\n", vfh, __func__, __LINE__);

	mutex_lock(&write_reg_mtx);

	if (DEBUG_BUFF_MESG)
		printk(" (isp) write reg start, hdl=0x%px  %s(%d)\n", vfh, __func__, __LINE__);

	isp_video_clk_enable();

	m2m_dev = vfh->m2m_dev;

	if (get_buf_info(vfh) == -1) {
		printk("(isp) get buf info failed %s(%d)\n", __func__, __LINE__);

		isp_video_clk_disable();

		mutex_unlock(&vfh->process_mtx);
		mutex_unlock(&write_reg_mtx);
		return;
	}

	src_buff = &(isp_data.src_buf->vb2_buf);
	dst_buff = &(isp_data.dst_buf->vb2_buf);
	isp_data.dst_buf->sequence = isp_data.src_buf->sequence;

	dst_v4l2_buff = isp_data.dst_buf;
	cap_width = isp_data.cap_width;
	cap_height = isp_data.cap_height;

	if (DEBUG_BUFF_MESG)
		printk(" (isp) hdl=0x%px (idx %d) %s(%d)\n", vfh,
			src_buff->index, __func__, __LINE__);

	if (DEBUG_BUFF_MESG && (vfh->frame_cnt >= ISP_U32_MAX))
		printk("(isp_dbg) frame cnt overflow %s(%d) \n", __func__, __LINE__);
	vfh->frame_cnt++;

	// set output addr and size
	isp_video_set_out_addr(&out_buf, src_buff);

	// set capture addr and size
	isp_video_set_cap_addr(vfh, &cap_buf, dst_buff, dst_v4l2_buff);

	// write register firstly
	isp_video_reg_handle(vfh, &cap_buf, &out_buf, cap_width, cap_height);

	// for debug tool: hardcode data and dump input frame
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 47)
	isp_dbg_hardcode_input(isp_cur_base, vfh, out_buf.input_hdr1_virt, out_buf.input_hdr2_virt);
	isp_dbg_hardcode_statis(isp_cur_base, vfh);
	if (isp_dbgfs_get_dump_input_frame() == 1)
		isp_dbg_dump_input_frame(isp_cur_base, vfh, &out_buf, vfh->frame_cnt);
#endif

	// hw trigger
	isp_video_write_reg_trigger();

	// wait hw done
	if (DEBUG_BUFF_MESG)
		printk(" (isp) hw start, handle=0x%px (idx %d) %s(%d)\n", vfh,
			src_buff->index, __func__, __LINE__);
	trace_isp_hw_start(vfh, src_buff->index);

	if ((src_buff->index < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		vfh->in_buff_status[src_buff->index] = MONI_HW_DOING;
	}
	if ((dst_buff->index < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		vfh->out_buff_status[dst_buff->index] = MONI_HW_DOING;
	}

	if (INTR_TIMEOUT_MODE)
		timeout = wait_event_interruptible_timeout(isp_hw_done_wait,
			isp_hw_done_flags > 0,
			(100 * HZ) / 1000);
	else
		wait_event_interruptible_exclusive(isp_hw_done_wait, isp_hw_done_flags > 0);

	if (DEBUG_BUFF_MESG)
		printk(" (isp) hw done, handle=0x%px (idx %d) %s(%d)\n",
			vfh, src_buff->index, __func__, __LINE__);
	trace_isp_hw_end(vfh, src_buff->index);

	spin_lock_irqsave(&isp_hw_done_lock, flags);
	isp_hw_done_flags = 0;
	spin_unlock_irqrestore(&isp_hw_done_lock, flags);

	if (INTR_TIMEOUT_MODE && timeout == 0) {
		printk("(isp) interrupt time out!!! %s(%d) \n", __func__, __LINE__);
	}

	if ((src_buff->index < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		vfh->in_buff_status[src_buff->index] = MONI_HW_DONE;
	}
	if ((dst_buff->index < MONI_Q_FRAME_NUM) && (vfh != NULL)) {
		vfh->out_buff_status[dst_buff->index] = MONI_HW_DONE;
	}

	if (DEBUG_BUFF_MESG) {
		mutex_lock(&reg_queue_mtx);
		isp_video_monitor_q_info();
		mutex_unlock(&reg_queue_mtx);
	}

	// fill 3a statis info
	isp_fill_statis_info(vfh, &cap_buf);

	// release buffer
	if (DEBUG_BUFF_MESG)
		printk(" (isp) hw done: buf rel start %s(%d)\n", __func__, __LINE__);

	isp_data.src_buf = v4l2_m2m_src_buf_remove(vfh->m2m_ctx);
	isp_data.dst_buf = v4l2_m2m_dst_buf_remove(vfh->m2m_ctx);
	v4l2_m2m_buf_done(isp_data.dst_buf, VB2_BUF_STATE_DONE);
	v4l2_m2m_buf_done(isp_data.src_buf, VB2_BUF_STATE_DONE);

	v4l2_m2m_job_finish(m2m_dev, vfh->m2m_ctx);

	if (DEBUG_BUFF_MESG)
		printk(" (isp) hw done: buf rel end %s(%d)\n", __func__, __LINE__);

	isp_video_debug_message();

	// debug tool: dump output data
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 47)
	if (isp_dbgfs_get_dump_statis() == 1)
		isp_dbg_dump_statis(isp_cur_base, vfh, vfh->frame_cnt);
	if (isp_dbgfs_get_dump_3dnr() == 1)
		isp_dbg_dump_3dnr(isp_cur_base, vfh, vfh->frame_cnt);
	if (isp_dbgfs_get_dump_output_frame() == 1)
		isp_dbg_dump_output_frame(isp_cur_base, vfh, &cap_buf, vfh->frame_cnt);
	if (isp_dbgfs_get_dump_3a() == 1)
		isp_dbg_dump_3a(isp_cur_base, vfh, &cap_buf, vfh->frame_cnt);
	if (isp_dbgfs_get_dump_reg() == 1)
		isp_dbg_dump_frame_reg(isp_cur_base, vfh, vfh->frame_cnt);
#endif

	isp_video_clk_disable();

	mutex_unlock(&write_reg_mtx);
	mutex_unlock(&vfh->process_mtx);

	if (DEBUG_BUFF_MESG)
		printk(" (isp) write reg end, handle=0x%px (idx %d) %s(%d)\n",
			vfh, src_buff->index, __func__, __LINE__);
}

static void isp_release_mlsc_buf(struct isp_video_fh *handle)
{
	if (handle == NULL) {
		printk("%s(%d)  input ptr is NULL  \n", __func__, __LINE__);
		return;
	}

	if (handle->hw_buff.mlsc_mem_priv != NULL) {
		if (handle->hw_buff.mlsc_ion_buf != NULL) {
			dma_buf_put(handle->hw_buff.mlsc_ion_buf);
			handle->hw_buff.mlsc_ion_buf = NULL;
		}
		mem_ops->unmap_dmabuf(handle->hw_buff.mlsc_mem_priv);
		mem_ops->detach_dmabuf(handle->hw_buff.mlsc_mem_priv);
		handle->hw_buff.mlsc_mem_priv = NULL;

		handle->hw_buff.mlsc_addr = 0;
	}
}

static void isp_release_dma_buf(struct isp_video_fh *handle)
{
	dma_addr_t phy_addr = 0;
	void *virt_addr = NULL;
	size_t buff_size = 0;

	if (handle == NULL) {
		printk("%s(%d)  input ptr is NULL  \n", __func__, __LINE__);
		return;
	}

	// fcurve rel
	virt_addr = handle->hw_buff.fcurve_buff.virt_addr;
	if (virt_addr && (handle->hw_buff.fcurve_buff.sram_en == 0)) {
		phy_addr = handle->hw_buff.fcurve_buff.phy_addr;
		buff_size = ISP_PAGE_ALIGN(handle->hw_buff.fcurve_buff.max_size * 2);
		dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
		handle->hw_buff.fcurve_buff.virt_addr = NULL;
	}

	// wdr rel
	virt_addr = handle->hw_buff.wdr_buff.virt_addr;
	if (virt_addr && (handle->hw_buff.wdr_buff.sram_en == 0)) {
		phy_addr = handle->hw_buff.wdr_buff.phy_addr;
		buff_size = ISP_PAGE_ALIGN(handle->hw_buff.wdr_buff.max_size * 2);
		dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
		handle->hw_buff.wdr_buff.virt_addr = NULL;
	}

	// lce rel
	virt_addr = handle->hw_buff.lce_buff.virt_addr;
	if (virt_addr && (handle->hw_buff.lce_buff.sram_en == 0)) {
		phy_addr = handle->hw_buff.lce_buff.phy_addr;
		buff_size = ISP_PAGE_ALIGN(handle->hw_buff.lce_buff.max_size * 2);
		dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
		handle->hw_buff.lce_buff.virt_addr = NULL;
	}

	// cnr rel
	virt_addr = handle->hw_buff.cnr_buff.virt_addr;
	if (virt_addr && (handle->hw_buff.cnr_buff.sram_en == 0)) {
		phy_addr = handle->hw_buff.cnr_buff.phy_addr;
		buff_size = ISP_PAGE_ALIGN(handle->hw_buff.cnr_buff.max_size * 2);
		dma_free_coherent(handle->video->isp_dev->dev, buff_size, virt_addr, phy_addr);
		handle->hw_buff.cnr_buff.virt_addr = NULL;
	}

	// sram unmap
	if (IS_ISP_SRAM_EN(handle->regQ_id) &&
		(handle->isp_sram_virt != NULL)) {
		iounmap(handle->isp_sram_virt);
		handle->isp_sram_virt = NULL;
		handle->sram_size = 0;
	}
}

static void isp_init_sram(void __iomem *isp_sram_virt, size_t sram_size)
{
	if (isp_sram_virt != NULL) {
		size_t idx = 0;
		size_t loop_times = sram_size >> 2; // sram_size * 8 / 32

		for (idx = 0; idx < loop_times; idx++) {
			size_t bias = idx << 2; // idx * 4
			writel(0x0, isp_sram_virt + bias);
		}
	}
}

static int isp_alloc_dma_buf(struct isp_video_fh *handle)
{
	dma_addr_t phy_addr = 0;
	void *virt_addr = NULL;
	size_t buff_size = 0;
	u32 width = 0;
	u32 height = 0;
	size_t sram_size[3] = {
		0
	}; // SRAM arrangement: fcurve size, wdr size, lce size

	if (handle == NULL) {
		printk("%s(%d)  input ptr is NULL  \n", __func__, __LINE__);
		return -ENOMEM;
	}

	width = handle->out_format.fmt.pix.width;
	height = handle->out_format.fmt.pix.height;

	// SRAM arrangement: fcurve size, wdr size, lce size
	if (IS_ISP_SRAM_EN(handle->regQ_id)) {
		sram_size[0] = ISP_PAGE_ALIGN(FCURVE_BUFF_MAX_SIZE) * 2;
		sram_size[1] = ISP_PAGE_ALIGN(WDR_BUFF_MAX_SIZE) * 2;
		sram_size[2] = ISP_PAGE_ALIGN(LCE_BUFF_MAX_SIZE) * 2;
		handle->sram_size = sram_size[0] + sram_size[1] + sram_size[2];
		handle->isp_sram_virt = ioremap(
			isp_sram_phy + handle->regQ_id * handle->sram_size,
			handle->sram_size);
		isp_init_sram(handle->isp_sram_virt, handle->sram_size);
	}

	// 3dnr init
	isp_video_3dnr_reset(handle);

	// mlsc init
	handle->hw_buff.mlsc_addr = 0;
	handle->hw_buff.mlsc_ion_buf = NULL;
	handle->hw_buff.mlsc_mem_priv = NULL;

	// fcurve statis init
	handle->hw_buff.fcurve_buff.buff_idx = 1;
	handle->hw_buff.fcurve_buff.max_size = ISP_PAGE_ALIGN(FCURVE_BUFF_MAX_SIZE);
	handle->hw_buff.fcurve_buff.first_frame = 1;

	buff_size = ISP_PAGE_ALIGN(handle->hw_buff.fcurve_buff.max_size * 2);

	if (IS_ISP_SRAM_EN(handle->regQ_id)) {
		if (handle->isp_sram_virt != NULL) {
			handle->hw_buff.fcurve_buff.phy_addr =
				isp_sram_phy + handle->regQ_id * handle->sram_size;
			handle->hw_buff.fcurve_buff.virt_addr = handle->isp_sram_virt;
			handle->hw_buff.fcurve_buff.sram_en = 1;
		}
	} else {
		virt_addr = dma_alloc_coherent(handle->video->isp_dev->dev,
						buff_size, &phy_addr, GFP_KERNEL);

		handle->hw_buff.fcurve_buff.phy_addr = phy_addr;
		handle->hw_buff.fcurve_buff.virt_addr = virt_addr;
		handle->hw_buff.fcurve_buff.sram_en = 0;
	}

	if (DEBUG_BUFF_MESG) {
		printk("fcurve buff addr = 0x%px , handle = 0x%px\n",
			handle->hw_buff.fcurve_buff.virt_addr, handle);
	}

	// wdr statis init
	handle->hw_buff.wdr_buff.buff_idx = 1;
	handle->hw_buff.wdr_buff.max_size = ISP_PAGE_ALIGN(WDR_BUFF_MAX_SIZE);
	handle->hw_buff.wdr_buff.first_frame = 1;

	buff_size = ISP_PAGE_ALIGN(handle->hw_buff.wdr_buff.max_size * 2);

	if (IS_ISP_SRAM_EN(handle->regQ_id)) {
		if (handle->isp_sram_virt != NULL) {
			handle->hw_buff.wdr_buff.phy_addr =
				isp_sram_phy + handle->regQ_id * handle->sram_size + sram_size[0];
			handle->hw_buff.wdr_buff.virt_addr = handle->isp_sram_virt + sram_size[0];
			handle->hw_buff.wdr_buff.sram_en = 1;
		}
	} else {
		virt_addr = dma_alloc_coherent(handle->video->isp_dev->dev,
						buff_size, &phy_addr, GFP_KERNEL);

		handle->hw_buff.wdr_buff.phy_addr = phy_addr;
		handle->hw_buff.wdr_buff.virt_addr = virt_addr;
		handle->hw_buff.wdr_buff.sram_en = 0;
	}

	if (DEBUG_BUFF_MESG) {
		printk("wdr buff addr = 0x%px , handle=0x%px\n",
			handle->hw_buff.wdr_buff.virt_addr, handle);
	}

	// lce statis init
	handle->hw_buff.lce_buff.buff_idx = 1;
	handle->hw_buff.lce_buff.max_size = ISP_PAGE_ALIGN(LCE_BUFF_MAX_SIZE);
	handle->hw_buff.lce_buff.first_frame = 1;

	buff_size = ISP_PAGE_ALIGN(handle->hw_buff.lce_buff.max_size * 2);

	if (IS_ISP_SRAM_EN(handle->regQ_id)) {
		if (handle->isp_sram_virt != NULL) {
			handle->hw_buff.lce_buff.phy_addr =
				isp_sram_phy + handle->regQ_id * handle->sram_size +
				sram_size[0] + sram_size[1];
			handle->hw_buff.lce_buff.virt_addr =
				handle->isp_sram_virt + sram_size[0] + sram_size[1];
			handle->hw_buff.lce_buff.sram_en = 1;
		}
	} else {
		virt_addr = dma_alloc_coherent(handle->video->isp_dev->dev,
						buff_size, &phy_addr, GFP_KERNEL);

		handle->hw_buff.lce_buff.phy_addr = phy_addr;
		handle->hw_buff.lce_buff.virt_addr = virt_addr;
		handle->hw_buff.lce_buff.sram_en = 0;
	}

	if (DEBUG_BUFF_MESG) {
		printk("lce buff addr = 0x%px , handle=0x%px\n",
			handle->hw_buff.lce_buff.virt_addr, handle);
	}

	// cnr statis init
	handle->hw_buff.cnr_buff.buff_idx = 1;
	if (width > 0 && height > 0)
		handle->hw_buff.cnr_buff.max_size = CNR_SIZE_FORMULA(width, height);
	else
		handle->hw_buff.cnr_buff.max_size = ISP_PAGE_ALIGN(CNR_BUFF_MAX_SIZE);
	handle->hw_buff.cnr_buff.first_frame = 1;

	buff_size = ISP_PAGE_ALIGN(handle->hw_buff.cnr_buff.max_size * 2);
	virt_addr = dma_alloc_coherent(handle->video->isp_dev->dev, buff_size, &phy_addr, GFP_KERNEL);

	handle->hw_buff.cnr_buff.phy_addr = phy_addr;
	handle->hw_buff.cnr_buff.virt_addr = virt_addr;
	handle->hw_buff.cnr_buff.sram_en = 0;

	if (DEBUG_BUFF_MESG) {
		printk("cnr buff addr = 0x%px , handle=0x%px, in_w=%d, in_h=%d, max_size=0x%lx\n",
			handle->hw_buff.cnr_buff.virt_addr, handle, width,
			height, handle->hw_buff.cnr_buff.max_size);
	}

	if (IS_ISP_SRAM_EN(handle->regQ_id)) {
		if ((handle->hw_buff.cnr_buff.virt_addr == NULL) || (handle->isp_sram_virt == NULL)) {
			printk("(isp)isp sram buff alloc failed!!!\n");
			isp_release_dma_buf(handle);
			return -ENOMEM;
		}
	} else if ((handle->hw_buff.fcurve_buff.virt_addr == NULL) ||
		(handle->hw_buff.wdr_buff.virt_addr == NULL) ||
		(handle->hw_buff.lce_buff.virt_addr == NULL) ||
		(handle->hw_buff.cnr_buff.virt_addr == NULL)) {
		printk("(isp)isp dma buff alloc failed!!!\n");
		isp_release_dma_buf(handle);
		return -ENOMEM;
	}

	return 0;
}

static int isp_register_video_dev(int id, struct isp_device *isp, u8 video_number)
{
	int r;
	struct isp_video *video;
	struct video_device *vdev;

	video = &(g_videos[id]);
	video->isp_dev = isp;

	vdev = &video->vdev;
	*vdev = isp_videodev;
	vdev->v4l2_dev = &isp->v4l2_dev;

	r = video_register_device(vdev, VFL_TYPE_VIDEO, video_number + id);
	if (r < 0) {
		dev_err(isp->dev, "video_register_device failed\n");
		return r;
	} else {
		video->type = VFL_TYPE_VIDEO;
		video_set_drvdata(vdev, video);
	}

	return 0;
}

int isp_set_mlsc_addr(struct isp_video_fh *handle, u32 input_data)
{
	dma_addr_t *dma_addr;
	int ion_fd = 0;
	struct dma_buf *ion_buf = NULL;
	int r = 0;
	u32 offset = input_data & 0xFFFFFF;
	u8 *start_addr = NULL;
	struct SettingMmaping *usr_setting = NULL;
	struct device *dev;

	if (handle == NULL) {
		printk("handle ptr is NULL %s(%d)\n", __func__, __LINE__);
		return -ENOMEM;
	}
	dev = handle->video->isp_dev->dev;

	mutex_lock(&handle->process_mtx);

	start_addr = handle->rq_addr + offset;
	usr_setting = (struct SettingMmaping *)start_addr;
	usr_setting->header.regQ_idx = handle->regQ_id;
	ion_fd = usr_setting->header.mlsc_fd;

	if (DEBUG_BUFF_MESG) {
		printk("(isp) handle=0x%px ion_fd=%d offset=0x%x  %s(%d) \n",
			handle, ion_fd, offset, __func__, __LINE__);
	}

	if (ion_fd < 0) {
		printk("mlsc ion fd < 0 %s(%d)\n", __func__, __LINE__);
		mutex_unlock(&handle->process_mtx);
		return -ENOMEM;
	}

	ion_buf = dma_buf_get(ion_fd);
	if (IS_ERR(ion_buf)) {
		printk("wrong ion_fd = %d  %s(%d)\n", ion_fd, __func__, __LINE__);
		mutex_unlock(&handle->process_mtx);
		return -1;
	}

	// if not empty, remove previous mlsc table
	isp_release_mlsc_buf(handle);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 47)
	handle->hw_buff.vb2_q.dev = dev;
	handle->hw_buff.vb2_q.dma_dir = DMA_BIDIRECTIONAL;
	handle->hw_buff.vb2_buf.vb2_queue = &handle->hw_buff.vb2_q;
	handle->hw_buff.mlsc_mem_priv = mem_ops->attach_dmabuf(
		&handle->hw_buff.vb2_buf, dev, ion_buf, MLSC_BUFF_MAX_SIZE);
#else
	handle->hw_buff.mlsc_mem_priv = mem_ops->attach_dmabuf(
		dev, ion_buf, MLSC_BUFF_MAX_SIZE, DMA_BIDIRECTIONAL);
#endif
	if (IS_ERR(handle->hw_buff.mlsc_mem_priv)) {
		printk("%s(%d) input addr failed  attach_dmabuf error\n", __func__, __LINE__);
		r = PTR_ERR(handle->hw_buff.mlsc_mem_priv);
		handle->hw_buff.mlsc_mem_priv = NULL;
		// put
		dma_buf_put(ion_buf);

		mutex_unlock(&handle->process_mtx);
		return r;
	}
	r = mem_ops->map_dmabuf(handle->hw_buff.mlsc_mem_priv);
	if (r) {
		dev_dbg(dev, "map_dmabuf error\n");
		// put
		dma_buf_put(ion_buf);

		mem_ops->detach_dmabuf(handle->hw_buff.mlsc_mem_priv);
		handle->hw_buff.mlsc_mem_priv = NULL;

		mutex_unlock(&handle->process_mtx);
		return r;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 47)
	dma_addr = mem_ops->cookie(&handle->hw_buff.vb2_buf,
				handle->hw_buff.mlsc_mem_priv);
#else
	dma_addr = mem_ops->cookie(handle->hw_buff.mlsc_mem_priv);
#endif
	handle->hw_buff.mlsc_addr = *dma_addr;
	handle->hw_buff.mlsc_ion_buf = ion_buf;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 47)
	handle->hw_buff.mlsc_virt = mem_ops->vaddr(&handle->hw_buff.vb2_buf, handle->hw_buff.mlsc_mem_priv);
#else
	handle->hw_buff.mlsc_virt = mem_ops->vaddr(handle->hw_buff.mlsc_mem_priv);
#endif

	mutex_unlock(&handle->process_mtx);
	return 0;
}

static int isp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	switch (ctrl->id) {
	case CID_ISP_REG_BUF_DONE: {
		struct isp_video_fh *handle = ctrl->priv;

		if (DEBUG_BUFF_MESG)
			printk("usr_done ctrl fh=0x%px  ctrl_val=0x%x %s(%d)\n",
				handle, ctrl->val, __func__, __LINE__);

		isp_video_reg_buf_add(handle, ctrl->val);

		if (DEBUG_BUFF_MESG)
			printk("(isp) isp reg done %s(%d)\n", __func__, __LINE__);
	} break;

	case CID_ISP_SET_MLSC_FD: {
		struct isp_video_fh *handle = ctrl->priv;

		if (DEBUG_BUFF_MESG)
			printk("mlsc_fd ctrl fh=0x%px  ctrl_val=0x%x %s(%d)\n",
				handle, ctrl->val, __func__, __LINE__);

		isp_set_mlsc_addr(handle, ctrl->val);

		if (DEBUG_BUFF_MESG)
			printk("(isp) isp set mlsc fd done %s(%d)\n", __func__, __LINE__);
	} break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int isp_init_controls(struct isp_video_fh *handle)
{
	int ret;

	ret = v4l2_ctrl_handler_init(&handle->ctrl_handler, 2);
	if (ret) {
		printk("isp_init_controls fail\n");
		return ret;
	}

	v4l2_ctrl_new_custom(&handle->ctrl_handler, &isp_reg_buffer_done_device_id, handle);
	v4l2_ctrl_new_custom(&handle->ctrl_handler, &isp_set_mlsc_fd, handle);

	return ret;
}

static int isp_remove(struct platform_device *pdev)
{
	int n = 0;
	struct isp_video *video;

	isp_debugfs_remove(&pdev->dev);

#ifdef CONFIG_PM
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	pm_runtime_disable(&pdev->dev);
#endif
#endif

#ifdef ISP_REGULATOR
	if (regulator) {
		regulator_disable(regulator);
	}
#endif

#ifdef ISP_PM_RST
	if (rst) {
		reset_control_assert(rst);
	}
#endif

	/* release m2m device */
	if (g_m2m_dev)
		v4l2_m2m_release(g_m2m_dev);

	isp_video_set_reg_base(NULL, NULL, 0);

	for (n = 0; n < MAX_VIDEO_DEVICE; n++) {
		video = &(g_videos[n]);
		video_unregister_device(&video->vdev);
		v4l2_device_unregister(&video->isp_dev->v4l2_dev);

		// release isp_device
		if (video->isp_dev != NULL)
			kfree(video->isp_dev);

		video->isp_dev = NULL;
	}

	if (vcl_clk)
		clk_unprepare(vcl_clk);
	if (vcl5_clk)
		clk_unprepare(vcl5_clk);
	if (isp_clk)
		clk_unprepare(isp_clk);

	return 0;
}

u8 isp_video_get_dump_log_status(void)
{
	return isp_dbg_log_status;
}

void isp_video_set_dump_log_status(u8 log_status)
{
	isp_dbg_log_status = log_status;
}

enum ISP_CLK_GATE_STATUS isp_video_get_clk_func_status(void)
{
	if ((CLK_GATING_FUNC_EN) && (isp_clk != NULL) && (isp_dbg_clk_status != ISP_CLK_FUNC_DIS))
		return ISP_CLK_FUNC_EN;

	return ISP_CLK_FUNC_DIS;
}

void isp_video_set_clk_func_status(enum ISP_CLK_GATE_STATUS clk_status)
{
	mutex_lock(&write_reg_mtx);
	if (ISP_CLK_FUNC_DIS == clk_status) {
		isp_video_clk_enable();
		isp_dbg_clk_status = clk_status;
	} else if (ISP_CLK_FUNC_EN == clk_status) {
		isp_dbg_clk_status = clk_status;
		isp_video_clk_disable();
	}
	mutex_unlock(&write_reg_mtx);
}

static int isp_video_clk_enable(void)
{
	int ret = 0;
	if (ISP_CLK_FUNC_EN == isp_video_get_clk_func_status()) {
		ret = clk_enable(isp_clk);
		if (ret)
			printk("isp clk enable failed %s(%d) \n", __func__, __LINE__);
	}

	return ret;
}

static int isp_video_clk_disable(void)
{
	if (ISP_CLK_FUNC_EN == isp_video_get_clk_func_status()) {
		clk_disable(isp_clk);
	}

	return 0;
}

static int isp_probe(struct platform_device *pdev)
{
	int ret;
	int i = 0;
	struct isp_device *isp[MAX_VIDEO_DEVICE];
	u32 video_number;

	if (!pdev->dev.of_node) {
		pr_info("%s(%d):fail\n", __func__, __LINE__);
		return -ENODEV;
	}

	mutex_init(&write_reg_mtx);

	mutex_init(&reg_queue_mtx);

	INIT_LIST_HEAD(&reg_queue_list);

	// get clk gating info
	if (CLK_GATING_FUNC_EN) {
		vcl_clk = devm_clk_get(&pdev->dev, "vcl_clk");

		if (IS_ERR(vcl_clk)) {
			dev_err(&pdev->dev, "Failed to get vcl_clk clock\n");
			vcl_clk = NULL;
		} else {
			ret = clk_prepare_enable(vcl_clk);
			if (ret) {
				dev_err(&pdev->dev, "Failed to prepare enable vcl_clk clock\n");
			}
		}

		vcl5_clk = devm_clk_get(&pdev->dev, "vcl5_clk");

		if (IS_ERR(vcl5_clk)) {
			dev_err(&pdev->dev, "Failed to get vcl5_clk clock\n");
			vcl5_clk = NULL;
		} else {
			ret = clk_prepare_enable(vcl5_clk);
			if (ret) {
				dev_err(&pdev->dev, "Failed to prepare enable vcl5_clk clock\n");
			}
		}

		isp_clk = devm_clk_get(&pdev->dev, "isp_clk");

		if (IS_ERR(isp_clk)) {
			dev_err(&pdev->dev, "Failed to get isp_clk clock\n");
			isp_clk = NULL;
		} else {
			// change isp clock rate
			ret = clk_set_rate(isp_clk, isp_clk_rate);
			if (ret) {
				dev_err(&pdev->dev, "Failed to set isp clock=%ld\n", isp_clk_rate);
			}
			ret = clk_prepare(isp_clk);
			if (ret) {
				dev_err(&pdev->dev, "Failed to prepare enable isp_clk clock\n");
			}
		}
	}

	ret = device_property_read_u32(&pdev->dev, "sunplus,devnode-number", &video_number);
	if (ret) {
		// dev_info(&pdev->dev, "get isp video number fail, using default number\n");
		video_number = ISP_DEVICE_BASE;
	}

	for (i = 0; i < MAX_VIDEO_DEVICE; i++) {
		isp[i] = kzalloc(sizeof(struct isp_device), GFP_KERNEL);
		if (!isp[i]) {
			dev_err(&pdev->dev, "could not allocate memory\n");
			return -ENOMEM;
		}

		isp[i]->dev = &pdev->dev;
		isp[i]->v4l2_dev.ctrl_handler = NULL;

		if (i == 0) {
			ret = isp_initialize_modules(pdev, isp[0]);
			if (ret < 0)
				goto error_modules;
		}

		ret = isp_register_entities(isp[i]);
		if (ret < 0)
			goto error_modules;

		ret = isp_register_video_dev(i, isp[i], video_number);
		if (ret < 0)
			goto error_register;
	}

	// init isp debug fs
	ret = isp_debugfs_init(isp[0]->dev);
	if (ret)
		dev_err(isp[0]->dev, "cannot create isp debugfs (%d)\n", ret);

	g_m2m_dev = v4l2_m2m_init(&m2m_ops);
	if (IS_ERR(g_m2m_dev)) {
		v4l2_err(&isp[0]->v4l2_dev, "Failed to init mem2mem device\n");
		ret = PTR_ERR(g_m2m_dev);
		goto error_modules;
	}

	dev_info(&pdev->dev, "SP ISP driver probed\n");

	return 0;

error_register:
error_modules:

	for (i = 0; i < MAX_VIDEO_DEVICE; i++) {
		if (isp[i] != NULL)
			kfree(isp[i]);
		isp[i] = NULL;
	}

#ifdef ISP_PM_RST
	if (rst) {
		reset_control_deassert(rst);
	}
#endif

#ifdef ISP_REGULATOR
	if (regulator) {
		regulator_disable(regulator);
	}
#endif

	return 0;
}

#ifdef CONFIG_PM

int isp_rtpm_suspend_ops(struct device *dev)
{
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	if (DEBUG_SUSPEND_RESUME)
		printk("isp rtpm suspend start mark %s(%d) \n", __func__, __LINE__);

#if 0
	mutex_lock(&write_reg_mtx);

#ifdef ISP_PM_RST
	if (rst)
		reset_control_assert(rst);
#endif

	/* disable stereo interrupt */
	isp_video_clk_enable();

	disable_irq(irq);
	isp_video_intr_en(0);

	isp_video_clk_disable();

#ifdef ISP_REGULATOR
	if (regulator)
	{
		regulator_disable(regulator);
	}
#endif

	mutex_unlock(&write_reg_mtx);
#endif

	if (DEBUG_SUSPEND_RESUME)
		printk("isp rtpm suspend end %s(%d) \n", __func__, __LINE__);
#endif
	return 0;
}

int isp_rtpm_resume_ops(struct device *dev)
{
#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
	if (DEBUG_SUSPEND_RESUME)
		printk("isp rtpm resume start mark %s(%d) \n", __func__, __LINE__);

#if 0
	mutex_lock(&write_reg_mtx);

#ifdef ISP_REGULATOR
	if (regulator)
	{
		regulator_enable(regulator);
	}
#endif

	/* disable stereo interrupt */
	isp_video_clk_enable();

	isp_video_intr_en(1);
	enable_irq(irq);

	isp_video_clk_disable();

#ifdef ISP_PM_RST
	if (rst)
	reset_control_deassert(rst);
#endif

	mutex_unlock(&write_reg_mtx);
#endif

	if (DEBUG_SUSPEND_RESUME)
		printk("isp rtpm resume end %s(%d) \n", __func__, __LINE__);
#endif
	return 0;
}

int isp_suspend_ops(struct device *dev)
{
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct reg_queue *t_entry = NULL;

	if (DEBUG_SUSPEND_RESUME)
		printk("isp sys suspend start mark %s(%d) \n", __func__, __LINE__);

	mutex_lock(&write_reg_mtx);

	list_for_each_safe(listptr, listptr_next, &reg_queue_list) {
		t_entry = list_entry(listptr, struct reg_queue, head);

		if (t_entry->hdl != NULL) {
			if (DEBUG_BUFF_MESG)
				printk("isp sys suspend wait, hdl=0x%px %s(%d) \n",
					t_entry->hdl, __func__, __LINE__);

			mutex_lock(&t_entry->hdl->process_mtx);

			if (DEBUG_BUFF_MESG)
				printk("isp sys suspend in mutex, hdl=0x%px %s(%d) \n",
					t_entry->hdl, __func__, __LINE__);

			if (t_entry->hdl->m2m_dev != NULL)
				v4l2_m2m_suspend(t_entry->hdl->m2m_dev);

			mutex_unlock(&t_entry->hdl->process_mtx);

			if (DEBUG_BUFF_MESG)
				printk("isp sys suspend out of mutex, hdl=0x%px %s(%d) \n",
					t_entry->hdl, __func__, __LINE__);
		}
	}

#ifdef ISP_PM_RST
	if (rst)
		reset_control_assert(rst);
#endif

	/* disable stereo interrupt */
	isp_video_clk_enable();

	disable_irq(irq);
	isp_video_intr_en(0);

	isp_video_clk_disable();

#ifdef ISP_REGULATOR
	if (regulator) {
		regulator_disable(regulator);
	}
#endif

	mutex_unlock(&write_reg_mtx);

	if (DEBUG_SUSPEND_RESUME)
		printk("isp sys suspend end %s(%d) \n", __func__, __LINE__);
	return 0;
}

int isp_resume_ops(struct device *dev)
{
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct reg_queue *t_entry = NULL;

	if (DEBUG_SUSPEND_RESUME)
		printk("isp sys resume start mark %s(%d) \n", __func__, __LINE__);

	mutex_lock(&write_reg_mtx);

#ifdef ISP_REGULATOR
	if (regulator) {
		regulator_enable(regulator);
	}
#endif

	/* disable stereo interrupt */
	isp_video_clk_enable();

	isp_video_intr_en(1);
	enable_irq(irq);

	isp_video_clk_disable();

#ifdef ISP_PM_RST
	if (rst)
		reset_control_deassert(rst);
#endif

	list_for_each_safe(listptr, listptr_next, &reg_queue_list) {
		t_entry = list_entry(listptr, struct reg_queue, head);

		if (t_entry->hdl != NULL) {
			if (DEBUG_BUFF_MESG)
				printk("isp sys resume wait, hdl=0x%px %s(%d) \n",
					t_entry->hdl, __func__, __LINE__);

			mutex_lock(&t_entry->hdl->process_mtx);

			if (DEBUG_BUFF_MESG)
				printk("isp sys resume in mutex, hdl=0x%px %s(%d) \n",
					t_entry->hdl, __func__, __LINE__);

			if (t_entry->hdl->m2m_dev != NULL)
				v4l2_m2m_resume(t_entry->hdl->m2m_dev);

			mutex_unlock(&t_entry->hdl->process_mtx);

			if (DEBUG_BUFF_MESG)
				printk("isp sys resume out of mutex, hdl=0x%px %s(%d) \n",
					t_entry->hdl, __func__, __LINE__);
		}
	}

	mutex_unlock(&write_reg_mtx);

	if (DEBUG_SUSPEND_RESUME)
		printk("isp sys resume end %s(%d) \n", __func__, __LINE__);

	return 0;
}

static const struct dev_pm_ops isp_pm = {

#ifdef CONFIG_PM_SYSTEM_VPU_DEFAULT
	SET_SYSTEM_SLEEP_PM_OPS(isp_suspend_ops, isp_resume_ops)
#endif

#ifdef CONFIG_PM_RUNTIME_VPU_DEFAULT
		SET_RUNTIME_PM_OPS(isp_rtpm_suspend_ops, isp_rtpm_resume_ops,
				NULL)
#endif
};
#endif

static const struct of_device_id isp_ids[] = {
	{
		.compatible = "sunplus,sp7350-isp",
	},
	{}
};
MODULE_DEVICE_TABLE(of, isp_ids);

static struct platform_driver sp_isp_driver = {
	.probe = isp_probe,
	.remove = isp_remove,
	//.id_table = isp_id_table,
	.driver = {
		.name = "sp-isp",
#ifdef CONFIG_PM
		.pm = &isp_pm,
#endif
		.of_match_table = isp_ids,
		//.of_match_table = isp_of_table,
	},
};

static int __init isp_init(void)
{
	platform_driver_register(&sp_isp_driver);
	return 0;
}

static void __exit isp_exit(void)
{
	if (isp_wq != NULL) {
		destroy_workqueue(isp_wq);
		isp_wq = NULL;
	}
	platform_driver_unregister(&sp_isp_driver);
}

module_init(isp_init);
module_exit(isp_exit);

module_param(isp_clk_rate, long, 0644);
module_param(video_nr, int, 0444);

MODULE_INFO(version, "1.0");
MODULE_DESCRIPTION("Sunplus ISP driver");
MODULE_AUTHOR("Saxen Ko");
MODULE_LICENSE("GPL");
