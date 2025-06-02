// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#ifndef ISP_UTIL_H
#define ISP_UTIL_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include "isp_reg.h"

enum ISP_ID {
	CONTROL = 0,
	FUNC_EN,
	HDR,
	FCURVE,
	DPC,
	GE, // 5
	MLSC,
	RLSC,
	BNR,
	WB,
	WDR, // 10
	DM,
	CCM,
	GAMMA,
	CST,
	LCE, // 15
	CNR,
	YCURVE,
	SP, // SHARPEN
	NR3D,
	SCALER, // 20
	AWB,
	AE,
	ISP_SETTINGS_END
};

struct Settings {
	u8 dirty;
	u32 offset;
	u32 value;
} __attribute__((aligned(8)));

struct FrameSetting {
	enum ISP_ID id;
	size_t num_settings;
	struct Settings settings[0];
};

enum BUF_STATUS {
	BUF_STATUS_NONE = 0,
	BUF_STATUS_HAL = 1,
	BUF_STATUS_HAL_DONE = 2,
	BUF_STATUS_DOING = 3,
	BUF_STATUS_DONE = 4,
	BUF_STATUS_END = 5,
};

struct ModuleRest {
	u32 reset_all : 1;
	u32 reset_fcurve : 1;
	u32 reset_wdr : 1;
	u32 reset_lce : 1;
	u32 reset_cnr : 1;
	u32 reset_3dnr : 1;
};

struct SettingHeader {
	u32 version;
	enum BUF_STATUS status;
	u64 statis_id;
	u32 input_sequence;
	struct ModuleRest reset_module;
	u8 regQ_idx;
	int mlsc_fd;
} __attribute__((aligned(8)));

struct SettingMmaping {
	struct SettingHeader header;
	struct FrameSetting module[0];
};

#define ISP_REG_BUFF_NUM (1)
// 1063 registers *(4 bytes + 4(offset/val)) = 8504 bytes
#define ISP_REGQ_SIZE (17 * 1024)
#define REG_MAX_OFFSET (0x1098)

#define WRITE_REG(hw_base_addr, reg, val, mask)                    \
	{                                                          \
		writel((readl((hw_base_addr) + (reg)) & ~(mask)) | \
			       ((val) & (mask)),                   \
		       (hw_base_addr) + (reg));                    \
	}

// mp version
#define READ_REG(hw_base_addr, reg, val)                  \
	{                                                 \
		(*(val)) = readl((hw_base_addr) + (reg)); \
	}

void isp_video_set_reg_base(void __iomem *isp_base, void __iomem *isp_base_2, int en);
void __iomem *isp_video_get_reg_base(void);
void __iomem *isp_video_next_reg_buff(void);
int isp_video_get_reg_buff_idx(void);
long isp_video_write_reg_from_buff(u8 *ker_rq_addr);
void isp_video_print_reg_buff(u8 *usr_reg_addr, size_t buf_size, size_t group_size);

#endif