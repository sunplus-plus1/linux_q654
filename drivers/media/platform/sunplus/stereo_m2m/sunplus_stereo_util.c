/*!
 * sunplus_stereo_utility.c - stereo utility tool box
 * @file sunplus_stereo_utility.c
 * @brief stereo utility tool box
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

#include "sunplus_stereo_reg.h"
#include "sunplus_stereo_util.h"
#include "sunplus_stereo_init_tab.h"

#include "sunplus_stereo.h"

enum STEREO_CTRL_STATE g_stereo_status = SP_STEREO_INIT;

/*! stereo spin lock for register R/W */
spinlock_t stereo_lock;
/*! stereo register base for register IO mapping */
void __iomem *stereo_base;

/*! stereo FWRR read buffer physical address */
phys_addr_t stereo_fwrr_r_mem_phys;

/*! stereo FWRR read buffer visual address */
void *stereo_fwrr_r_mem_visu = NULL;

/*! stereo FWRR write buffer physical address */
phys_addr_t stereo_fwrr_w_mem_phys;

/*! stereo FWRR write buffer visual address*/
void *stereo_fwrr_w_mem_visu = NULL;

/*! stereo RWFR read buffer physical address */
phys_addr_t stereo_rwfr_r_mem_phys;

/*! stereo RWFR read buffer visual address*/
void *stereo_rwfr_r_mem_visu = NULL;

/*! stereo RWFR write buffer physical address */
phys_addr_t stereo_rwfr_w_mem_phys;

/*! stereo RWFR write buffer visual address */
void *stereo_rwfr_w_mem_visu = NULL;

/*! wait queue */
// wait_queue_head_t stereo_process_wait;
struct completion stereo_comp;

/*! save last running param */
struct stereo_video_fh *sfh_last = NULL;

inline void sunplus_stereo_clk_gating(struct clk *stereo_clk, bool isGating)
{
	if (stereo_clk) {
		if (isGating) {
			//clk_disable(stereo_clk);
		} else {
			clk_enable(stereo_clk);
		}
	}
}

inline void sunplus_stereo_intr_enable(u32 on)
{
	stereo_write(!on, STEREO_SDP_IRQ_MASK_OFFSET);
}

inline void sunplus_stereo_top_reset(void)
{
	if (!stereo_read(STEREO_SDP_ADDR_STATUS_OFFSET))
		stereo_write(1, STEREO_SOFT_RESET_OFFSET);
}

inline void sunplus_stereo_clr_done(void)
{
	u32 i;

	// Check HW is not running
	for (i = 0; i < VCL_STEREO_TIMEOUT; i++) {
		if (!stereo_read(STEREO_SDP_ADDR_STATUS_OFFSET))
			break;

		fsleep(1000); // 1ms
	}
	// Clear IRQ
	stereo_read(STEREO_SDP_FRM_STATUS_OFFSET);
}

void sunplus_stereo_init_sgm8dir(void)
{
	if (sgm_8dir_available) {
		// FWRR Read
		stereo_write(LODWORD(stereo_fwrr_w_mem_phys), STEREO_FWRR_MEM_ADDR_0_OFFSET);
		stereo_write(HIDWORD(stereo_fwrr_w_mem_phys), STEREO_FWRR_MEM_ADDR_2_OFFSET);
		// FWRR Write
		stereo_write(LODWORD(stereo_fwrr_r_mem_phys), STEREO_FWRR_MEM_ADDR_1_OFFSET);
		stereo_write(HIDWORD(stereo_fwrr_r_mem_phys), STEREO_FWRR_MEM_ADDR_3_OFFSET);
		// RWFR Read
		stereo_write(LODWORD(stereo_rwfr_w_mem_phys), STEREO_RWFR_MEM_ADDR_0_OFFSET);
		stereo_write(HIDWORD(stereo_rwfr_w_mem_phys), STEREO_RWFR_MEM_ADDR_2_OFFSET);
		// RWFR Write
		stereo_write(LODWORD(stereo_rwfr_r_mem_phys), STEREO_RWFR_MEM_ADDR_1_OFFSET);
		stereo_write(HIDWORD(stereo_rwfr_r_mem_phys), STEREO_RWFR_MEM_ADDR_3_OFFSET);
	}
}

s32 sunplus_stereo_driver_init(void)
{
	unsigned int i;
	unsigned int data_len;

	data_len = sizeof(stereo_register_setting) / sizeof(reg_rw_test_unit);

	for (i = 0; i < data_len; i++ )
		stereo_write(stereo_register_setting[i].value, stereo_register_setting[i].offset);

	sunplus_stereo_init_sgm8dir();

	/* disable all stereo interrupt */
	sunplus_stereo_intr_enable(0);
	/* clear IRQ status */
	sunplus_stereo_clr_done();
	/* initial wait queue */
	// init_waitqueue_head(&stereo_process_wait);
	init_completion(&stereo_comp);

	return 0;
}

inline void sunplus_stereo_update_window(int width, int height, struct stereo_video_fh *sfh)
{
	struct stereo_config *pCfg = &sfh->config;
	struct v4l2_rect *crop = NULL;
	STEREO_img_fram_cfg_0_RBUS reg_in;
	STEREO_Control_0_RBUS reg_out;
	unsigned long flags;

	reg_in.f_width = width;
	reg_in.f_height = height;

	if (pCfg->crop_en) {
		crop = &pCfg->crop;
		reg_out.f_width = crop->width;
		reg_out.f_height = crop->height;
	} else {
		reg_out.f_width = width;
		reg_out.f_height = height;
	}

	spin_lock_irqsave(&stereo_lock, flags);

	/* apply the resolution setting to stereo DMA */
	stereo_write(reg_in.RegValue, STEREO_L_IMG_FRAM_CFG_0_OFFSET); ///< left image resolution
	stereo_write(reg_in.RegValue, STEREO_R_IMG_FRAM_CFG_0_OFFSET); ///< right image resolution
	stereo_write(reg_in.RegValue, STEREO_TOF_FRAM_CFG_0_OFFSET); ///< tof resolution

	/* apply the resolution setting to stereo hardware */
	stereo_write(reg_out.RegValue, STEREO_CONTROL_0_OFFSET);

	spin_unlock_irqrestore(&stereo_lock, flags);
}

inline void sunplus_stereo_update_roi(struct stereo_video_fh *sfh)
{
	struct stereo_config *pCfg = &sfh->config;
	struct v4l2_rect *crop = NULL;
	STEREO_img_fram_cfg_1_RBUS sgr_en;
	STEREO_img_fram_cfg_2_RBUS sgr_cnt;
	STEREO_img_hsgr_RBUS hsgr;
	STEREO_img_vsgr_RBUS vsgr;

	if (!pCfg->crop_en) {
		pr_debug("No ROI\n");
		stereo_write(0, STEREO_DIS_FRAM_CFG_0_OFFSET);
		stereo_write(0, STEREO_L_IMG_FRAM_CFG_1_OFFSET);
		stereo_write(0, STEREO_R_IMG_FRAM_CFG_1_OFFSET);
		stereo_write(0, STEREO_TOF_FRAM_CFG_1_OFFSET);
		return;
	}

	pr_debug("ROI setup\n");
	// setup ROI of STEREO_OUT

	crop = &pCfg->crop;
	sgr_en.f_hsgr_en = 1;
	sgr_en.f_vsgr_en = 1;
	sgr_cnt.f_hsgr_count = crop->width;
	sgr_cnt.f_vsgr_count = crop->height;
	hsgr.f_first_interval = crop->left;
	hsgr.f_interfval = sfh->out_format.fmt.pix.width; // automatically cropped if OOB
	vsgr.f_first_interval = crop->top;
	vsgr.f_interfval = sfh->out_format.fmt.pix.height; // automatically cropped if OOB

	// STEREO_OUT
	stereo_write(sgr_en.RegValue, STEREO_DIS_FRAM_CFG_0_OFFSET);
	stereo_write(sgr_cnt.RegValue, STEREO_DIS_FRAM_CFG_1_OFFSET);
	stereo_write(hsgr.RegValue, STEREO_DIS_HDSR_OFFSET);
	stereo_write(vsgr.RegValue, STEREO_DIS_VDSR_OFFSET);
	// STEREO_EYE_L
	stereo_write(sgr_en.RegValue, STEREO_L_IMG_FRAM_CFG_1_OFFSET);
	stereo_write(sgr_cnt.RegValue, STEREO_L_IMG_FRAM_CFG_2_OFFSET);
	stereo_write(hsgr.RegValue, STEREO_L_IMG_HSGR_OFFSET);
	stereo_write(vsgr.RegValue, STEREO_L_IMG_VSGR_OFFSET);
	// STEREO_EYE_R
	stereo_write(sgr_en.RegValue, STEREO_R_IMG_FRAM_CFG_1_OFFSET);
	stereo_write(sgr_cnt.RegValue, STEREO_R_IMG_FRAM_CFG_2_OFFSET);
	stereo_write(hsgr.RegValue, STEREO_R_IMG_HSGR_OFFSET);
	stereo_write(vsgr.RegValue, STEREO_R_IMG_VSGR_OFFSET);
	// STEREO_TOF_C
	stereo_write(sgr_en.RegValue, STEREO_TOF_FRAM_CFG_1_OFFSET);
	stereo_write(sgr_cnt.RegValue, STEREO_TOF_FRAM_CFG_2_OFFSET);
	stereo_write(hsgr.RegValue, STEREO_TOF_HSGR_OFFSET);
	stereo_write(vsgr.RegValue, STEREO_TOF_VSGR_OFFSET);
}

void sunplus_stereo_apply_setting(struct v4l2_fh *fh)
{
	STEREO_FUNC_EN_0_RBUS fun_reg;
	STEREO_OUT_FMT_0_RBUS fmt_reg;
	struct stereo_video_fh *sfh = to_stereo_video_fh(fh);
	struct stereo_config *pCfg = &sfh->config;
	struct stereo_ioctl_param *param = &pCfg->param_tb;
	unsigned long flags;

	pr_debug("apply stereo config\n");

	sunplus_stereo_top_reset();

	/* 1. apply tuning params */
	if (sfh_last != sfh || g_stereo_status == SP_STEREO_INIT) {
		u32 i;

		/* apply tunning data */
		spin_lock_irqsave(&stereo_lock, flags);
		for(i = 0; i < param->size; i++)
			stereo_write(param->pdata[i], param->paddr[i]);

		spin_unlock_irqrestore(&stereo_lock, flags);

		sfh_last = sfh;
	}

	/* 2. apply the resolution setting */
	sunplus_stereo_update_window(sfh->out_format.fmt.pix.width, sfh->out_format.fmt.pix.height, sfh);
	/* 3. apply the tuning param */
	sunplus_stereo_update_roi(sfh);

	/* 4. apply function control setting */
	fun_reg.RegValue = stereo_read(STEREO_FUNC_EN_0_OFFSET);

	/* apply the tof setting to stereo hardware */
	fun_reg.f_tof_fusion_en = (pCfg->tof_mode == STEREO_W_TOF) ? 1: 0;
	// pr_info("reg tof_mode=%d\n", fun_reg.f_tof_fusion_en);

	/* apply the sgm setting to stereo hardware */
	fun_reg.f_sgm_8direction_en = (pCfg->sgm_mode == STEREO_8_DIR) ? 1: 0;
	// pr_info("sgm_mode=%d\n", pCfg->sgm_mode);

	/* apply output format setting */
	switch (pCfg->output_fmt) {
		case DISPARITY_W_THREHOLD:
		case DISPARITY_WO_THREHOLD:
			fun_reg.f_depth_out = 0;
		break;
		case DEPTH_W_THREHOLD:
		case DEPTH_WO_THREHOLD:
		default:
			fun_reg.f_depth_out = 1;
		break;
	}
	fun_reg.f_thresh_mode = 1;

	fmt_reg.f_FB_val = pCfg->calibration_data;
	fmt_reg.f_thresh_val = pCfg->thresh_val;

	/* apply output option setting */
	spin_lock_irqsave(&stereo_lock, flags);
	stereo_write(fun_reg.RegValue, STEREO_FUNC_EN_0_OFFSET);
	stereo_write(fmt_reg.RegValue, STEREO_OUT_FMT_0_OFFSET);
	spin_unlock_irqrestore(&stereo_lock, flags);

	// pr_info("fun_reg.RegValue=0x%x", fun_reg.RegValue);
}

inline s32 sunplus_stereo_DMA_address_update(enum ENUM_STEREO_BUF_TYPE type, dma_addr_t pa)
{
	//unsigned long flags;

	// spin_lock_irqsave(&stereo_lock, flags);
	switch(type)
	{
		case STEREO_EYE_L:
			/* apply the left eye PA to stereo DMA */
			stereo_write(LODWORD(pa), STEREO_LF_R_ADDR_LO_OFFSET);
			stereo_write(HIDWORD(pa), STEREO_LF_R_ADDR_HI_OFFSET);
			break;
		case STEREO_EYE_R:
			/* apply the right eye PA to stereo DMA */
			stereo_write(LODWORD(pa), STEREO_RF_R_ADDR_LO_OFFSET);
			stereo_write(HIDWORD(pa), STEREO_RF_R_ADDR_HI_OFFSET);
			break;
		case STEREO_TOF_C:
			/* apply the TOF C PA to stereo DMA */
			stereo_write(LODWORD(pa), STEREO_TOFC_R_ADDR_LO_OFFSET);
			stereo_write(HIDWORD(pa), STEREO_TOFC_R_ADDR_HI_OFFSET);
			break;
		case STEREO_TOF_D:
			/* apply the TOF D PA to stereo DMA */
			stereo_write(LODWORD(pa), STEREO_TOFD_R_ADDR_LO_OFFSET);
			stereo_write(HIDWORD(pa), STEREO_TOFD_R_ADDR_HI_OFFSET);
			break;
		case STEREO_OUT:
			/* apply the stereo out PA to stereo DMA */
			stereo_write(LODWORD(pa), STEREO_DEPTH_W_ADDR_LO_OFFSET);
			stereo_write(HIDWORD(pa), STEREO_DEPTH_W_ADDR_HI_OFFSET);
			break;
		default:
			// spin_unlock_irqrestore(&stereo_lock, flags);
			pr_err("unknown BUF TYPE\n");
			return -EINVAL;
	}
	// spin_unlock_irqrestore(&stereo_lock, flags);

	return 0;
}

s32 sunplus_stereo_dma_update(struct v4l2_fh *vfh)
{
	struct stereo_video_fh *sfh = to_stereo_video_fh(vfh);
	struct v4l2_m2m_ctx *m2m_ctx = sfh->vfh.m2m_ctx;
	struct vb2_v4l2_buffer *src_buf = v4l2_m2m_next_src_buf(m2m_ctx);
	struct vb2_v4l2_buffer *dst_buf = v4l2_m2m_next_dst_buf(m2m_ctx);
	dma_addr_t pa;
	unsigned long flags;
	enum ENUM_STEREO_BUF_TYPE type;
	int input_num = (sfh->config.tof_mode == STEREO_WO_TOF) ? 2 : 4;

	if (!src_buf && !dst_buf) {
		pr_err("No Stereo buffers available\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&stereo_lock, flags);

	for (type = 0; type < input_num; type++) {
		pa = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, type);
		sunplus_stereo_DMA_address_update(type, pa);
	}
	pa = vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0);
	sunplus_stereo_DMA_address_update(STEREO_OUT, pa);

	spin_unlock_irqrestore(&stereo_lock, flags);

	return 0;
}

void sunplus_stereo_start_stereo(void)
{
	unsigned long flags;

	pr_debug("Stereo HW trigger\n");

	/* let stereo out data frame start */
	spin_lock_irqsave(&stereo_lock, flags);
	stereo_write(1, STEREO_FRAME_START_OFFSET);
	g_stereo_status = SP_STEREO_RUN;
	spin_unlock_irqrestore(&stereo_lock, flags);
}

s32 sunplus_stereo_ISR_handler(void *data)
{
	unsigned long flags;
	u32 rest_num;

	// Check rest num
	rest_num = stereo_read(STEREO_SDP_ADDR_STATUS_OFFSET);

	/* DMA output finish handler */
	if (!rest_num) {
		// Clear IRQ
		if (!stereo_read(STEREO_SDP_FRM_STATUS_OFFSET)) {
			pr_debug("stereo device got false done isr\n");
			return 0;
		}

		spin_lock_irqsave(&stereo_lock, flags);
		g_stereo_status = SP_STEREO_IDLE;

		// wake_up(&stereo_process_wait);
		complete(&stereo_comp);
		spin_unlock_irqrestore(&stereo_lock, flags);
	}

	return 0;
}
