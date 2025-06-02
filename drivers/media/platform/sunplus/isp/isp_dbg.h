// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#ifndef ISP_DBG_H
#define ISP_DBG_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include "isp.h"
#include "isp_debugfs.h"
#include "isp_reg.h"

void isp_dbg_set_clk_func(enum ISP_CLK_GATE_STATUS status);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 47)
void isp_dbg_hardcode_reg(void __iomem *isp_base, struct isp_video_fh *vfh);
void isp_dbg_hardcode_input(void __iomem *isp_base, struct isp_video_fh *vfh, void *hdr0_virt, void *hdr1_virt);
void isp_dbg_hardcode_statis(void __iomem *isp_base, struct isp_video_fh *vfh);

void isp_dbg_dump_last_reg(void);
void isp_dbg_dump_frame_reg(void __iomem *isp_base, struct isp_video_fh *vfh, u32 frame_cnt);
void isp_dbg_dump_statis(void __iomem *isp_base, struct isp_video_fh *vfh, u32 frame_cnt);
void isp_dbg_dump_3dnr(void __iomem *isp_base, struct isp_video_fh *vfh, u32 frame_cnt);
void isp_dbg_dump_3a(void __iomem *isp_base, struct isp_video_fh *vfh, struct cap_buff *cap_buff_ptr, u32 frame_cnt);
void isp_dbg_dump_input_frame(void __iomem *isp_base, struct isp_video_fh *vfh, struct out_buff *out_buf, u32 frame_cnt);
void isp_dbg_dump_output_frame(void __iomem *isp_base, struct isp_video_fh *vfh, struct cap_buff *cap_buf, u32 frame_cnt);
#endif

#endif /* ISP_DBG_H */
