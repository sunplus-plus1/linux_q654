/*!
 * vicore_stereo_utility.h - stereo utility tool box
 * @file vicore_stereo_utility.h
 * @brief stereo utility tool box
 * @author Saxen Ko <saxen.ko@vicorelogic.com>
 * @version 1.0
 * @copyright  Copyright (C) 2022 Vicorelogic
 * @note
 * Copyright (C) 2022 Vicorelogic
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifndef VICORE_STEREO_UTILITY_H
#define VICORE_STEREO_UTILITY_H

#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/atomic.h>

#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-event.h>

#define VCL_STEREO_TIMEOUT 2000

#ifndef HIDWORD
#define LODWORD(_qw)    ((u32)(_qw))
#define HIDWORD(_qw)    ((u32)(((_qw) >> 32) & 0xffffffff))
#endif

/*!
 * @def stereo_read
 * @brief stereo register read MARCO
 * @param reg:register index
 */
#define stereo_read(reg) readl(stereo_base + reg)
/*!
 * @def stereo_write
 * @brief stereo register write MARCO
 * @param reg: register index
 */
#define stereo_write(val, reg) writel(val, stereo_base + reg)

/*!
 * @def IOCTL command ID
 * @brief stereo command ID for ioctl
 */
#define VCL_STEREO_IOCTL_S_TUNE_DATA (BASE_VIDIOC_PRIVATE)
// #define VCL_STEREO_IOCTL_G_TUNE_DATA (BASE_VIDIOC_PRIVATE + 1)
#define VICORE_CID_CUSTOM_BASE		(V4L2_CID_USER_BASE | 0xf000)
#define VICORE_CID_CALIBRATION_DATA	(VICORE_CID_CUSTOM_BASE + 1)
#define VICORE_CID_TOF_MODE			(VICORE_CID_CUSTOM_BASE + 2)
#define VICORE_CID_SGM_MODE			(VICORE_CID_CUSTOM_BASE + 3)
#define VICORE_CID_OUTPUT_FMT		(VICORE_CID_CUSTOM_BASE + 4)
#define VICORE_CID_OUTPUT_THRESHOLD	(VICORE_CID_CUSTOM_BASE + 5)

/* STEREO_HAL_INIT structure and definitions */
/*!
 * @def STEREO_INIT_PASS
 * @brief Stereo init all pass
 */
#define STEREO_INIT_PASS		0x00
/*!
 * @def STEREO_INIT_CALIBRATION_ERROR
 * @brief There is one calibration error on stereo init
 */
#define STEREO_INIT_CALIBRATION_ERROR		0x01
/*!
 * @def STEREO_CONFIG_TOF_ERROR
 * @brief There is one tof error on stereo configuration
 */
#define STEREO_CONFIG_TOF_ERROR		0x02
/*!
 * @def STEREO_CONFIG_SGM_ERROR
 * @brief There is one sgm error on stereo configuration
 */
#define STEREO_CONFIG_SGM_ERROR		0x04
/*!
 * @def STEREO_CONFIG_OUT_FMT_ERROR
 * @brief There is one out format error on stereo configuration
 */
#define STEREO_CONFIG_OUT_FMT_ERROR		0x08
/*!
 * @def STEREO_CONFIG_OUT_OPTION_ERROR
 * @brief There is one out option error on stereo configuration
 */
#define STEREO_CONFIG_OUT_OPTION_ERROR		0x10
/*!
 * @def STEREO_CONFIG_RES_ERROR
 * @brief There is one resolution error on stereo configuration
 */
#define STEREO_CONFIG_RES_ERROR		0x20
/*!
 * @def STEREO_CONFIG_RECT_ERROR
 * @brief There is one rectangle error on stereo configuration
 */
#define STEREO_CONFIG_RECT_ERROR		0x40
/*!
 * @def STEREO_INIT_OTHER_ERROR
 * @brief There is one other error on stereo init
 */
#define STEREO_INIT_OTHER_ERROR		0x80

/* Stereo STEREO BUF TYPE */
/*!
 * @enum ENUM_STEREO_BUF_TYPE
 * @brief The enum of Stereo buffer types
 * @param STEREO_OUT: Stereo out
 * @param STEREO_TOF_C: TOF C data input
 * @param STEREO_TOF_D: TOF D data input
 * @param STEREO_EYE_L: Left eye data input
 * @param STEREO_EYE_R: Right eye data input
 */
enum ENUM_STEREO_BUF_TYPE {
	STEREO_EYE_L,
	STEREO_EYE_R,
	STEREO_TOF_C,
	STEREO_TOF_D,
	STEREO_OUT,
	STEREO_BUF_TYPE_NUM,
	STEREO_BUF_IN_NUM = (STEREO_BUF_TYPE_NUM - 1),
};

/* Stereo STEREO Control Status */
/*!
 * @enum STEREO_CTRL_STATE
 * @brief The enum of Stereo device status
 * @param VCL_STEREO_INIT: Stereo default
 * @param VCL_STEREO_OUT_TRIG: trig output start
 * @param VCL_STEREO_OUT_DONE: output processing done
 */
enum STEREO_CTRL_STATE{
	VCL_STEREO_INIT,
	VCL_STEREO_RUN,
	VCL_STEREO_IDLE
};

/*!
 * @enum TOF_MODE
 * @brief The enum of TOF option
 * @param STEREO_WO_TOF: without TOF input
 * @param STEREO_W_TOF: with TOF input
 */
enum ENUM_TOF_MODE {
	/*! without TOF input */
	STEREO_WO_TOF,
	/*! with TOF input */
	STEREO_W_TOF,
};

/*!
 * @enum SGM_MODE
 * @brief The enum of SGM_MODE option
 * @param STEREO_4_DIR: SGM 4 direction
 * @param STEREO_8_DIR: SGM 8 direction
 */
enum ENUM_SGM_MODE {
	/*! SGM 4 direction */
	STEREO_4_DIR,
	/*! SGM 8 direction */
	STEREO_8_DIR,
};

/*!
 * @enum OUTPUT_FMT
 * @brief The enum of output format
 * @param CONFIDENCE_DISPARITY: 7 bits: Confidence, 7 + 2 bits: Disparity
 * @param DISPARITY: 7 bits: Zero, 7 + 2 bits: Disparity
 * @param CONFIDENCE_DEPTH: 3 bits: Confidence, 9 + 4 bits: Depth
 * @param DEPTH: 3 bits: Zeros, 9 + 4 bits: Depth
 */
enum ENUM_OUTPUT_FMT {
	/*! Disparity without threshold */
	DISPARITY_WO_THREHOLD,
	/*! Disparity with threshold */
	DISPARITY_W_THREHOLD,
	/*! Depth without threshold */
	DEPTH_WO_THREHOLD,
	/*! Depth with threshold */
	DEPTH_W_THREHOLD,
};

/*!
 * @struct stereo_ioctl_param
 * @brief The structure of stereo ioctl params
 * @param size:total size of params
 * @param paddr:array of phyiscal address
 * @param pdata:array of data
 */
struct stereo_ioctl_param {
	uint32_t size;
	uint32_t *paddr;
	uint32_t *pdata;
};

/*!
 * @struct stereo_config
 * @brief The structure of stereo IOCTL init
 * @param calibration:calibration data form application
 * @param tof_mode:TOF setting from application
 * @param sgm_mode:SGM setting from application
 * @param output_fmt:output format setting from application
 * @param res:resolution setting from application
 * @param rect:rectangle setting from application
 * @param status: error status
 */
struct stereo_config {
	/*! calibration data form application */
	u16 calibration_data;
	/*! TOF setting from application */
	enum ENUM_TOF_MODE tof_mode;
	/*! SGM setting from application */
	enum ENUM_SGM_MODE sgm_mode;
	/*! output format setting from application */
	enum ENUM_OUTPUT_FMT output_fmt;
	/* Output data threshold */
	u8 thresh_val;
	/*! Image output resolution */
	struct v4l2_area res;
	/*! Image input resolution */
	struct v4l2_area res_input;
	/*! crop enable */
	u32 crop_en;
	/*! ROI Rectangle */
	//struct v4l2_rect crop[STEREO_BUF_TYPE_NUM];
	struct v4l2_rect crop;
	/*! tuning params*/
	struct stereo_ioctl_param param_tb;
	/*! status*/
	s32 status;
};

/*
	Variable
 */
extern spinlock_t stereo_lock;
extern void __iomem *stereo_base;

/*! stereo sgm_8dir_available */
extern bool sgm_8dir_available;

/*! stereo FWRR read buffer physical address */
extern phys_addr_t stereo_fwrr_r_mem_phys;

/*! stereo FWRR read buffer visual address */
extern void *stereo_fwrr_r_mem_visu;

/*! stereo FWRR write buffer physical address */
extern phys_addr_t stereo_fwrr_w_mem_phys;

/*! stereo FWRR write buffer visual address*/
extern void *stereo_fwrr_w_mem_visu;

/*! stereo RWFR read buffer physical address */
extern phys_addr_t stereo_rwfr_r_mem_phys;

/*! stereo RWFR read buffer visual address*/
extern void *stereo_rwfr_r_mem_visu;

/*! stereo RWFR write buffer physical address */
extern phys_addr_t stereo_rwfr_w_mem_phys;

/*! stereo RWFR write buffer visual address */
extern void* stereo_rwfr_w_mem_visu;

/*! wait queue */
// extern wait_queue_head_t stereo_process_wait; // wake on stereo process done
extern struct completion stereo_comp;

/* stereo device number */
extern bool g_stereo_is_running;

/* control flow status */
extern enum STEREO_CTRL_STATE g_stereo_status;

/* Input/output buffer list */
extern struct list_head vcl_stereo_buf[][STEREO_BUF_TYPE_NUM];

/*! save last running param */
extern struct stereo_video_fh *sfh_last;

/* Functions */
extern s32 vicore_stereo_driver_init(void);
extern void vicore_stereo_apply_setting(struct v4l2_fh *fh);
extern void vicore_stereo_start_stereo(void);
extern s32 vicore_stereo_ISR_handler(void *data);
extern s32 vicore_stereo_power_on(void *data);
extern s32 vicore_stereo_power_off(void *data);
extern void vicore_stereo_clk_gating(struct clk *stereo_clk, bool isGating);

/* vcore stereo utility APIs */
extern void vicore_stereo_swreset_top(void);
extern void vcore_stereo_top_reset(void);
extern void vicore_stereo_intr_enable(u32 on);
extern void vicore_stereo_reset_rdma(enum ENUM_STEREO_BUF_TYPE type);
extern s32 vicore_stereo_dma_update(struct v4l2_fh *vfh);
extern void vicore_stereo_clr_done(void);
extern int vicore_stereo_out_hdlr(void);
extern int vicore_stereo_in_hdlr(void);
extern void vicore_stereo_init_sgm8dir(void);

#endif /* VICORE_STEREO_UTILITY_H */
