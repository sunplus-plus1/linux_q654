// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/uaccess.h>

#include "isp_dbg.h"

#define DBG_LOG (false)

#define ISP_FILE_NAME_SIZE (200)

#define ISP_SW0_SW_TMP0_DBG_EN_MASK (0x1)

#define ISP_SW0_SW_TMP0_FUNC_0_MASK (0x10)
#define ISP_SW0_SW_TMP0_FUNC_1_MASK (0x20)
#define ISP_SW0_SW_TMP0_FUNC_2_MASK (0x40)

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 47)
static int isp_reg_atoi(char *buff, u32 *reg, u32 *val);

static void isp_dump_virt(void *virt_addr, unsigned long buff_size, struct file *filp);
static void isp_dump_sram(void __iomem *isp_virt_base, unsigned long buff_size, struct file *filp);

static void isp_dump_mlsc(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt);
static void isp_dump_fcurve(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt);
static void isp_dump_wdr(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt);
static void isp_dump_lce(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt);
static void isp_dump_cnr(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt);
static void isp_dump_3dnr_yuv(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt);
static void isp_dump_3dnr_mot(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt);

static void isp_reg_from_file(void __iomem *isp_base);
#endif

void isp_dbg_set_clk_func(enum ISP_CLK_GATE_STATUS status)
{
	static enum ISP_CLK_GATE_STATUS last_status = ISP_CLK_FUNC_NONE;

	if (ISP_CLK_FUNC_DIS == status)	{
		if (ISP_CLK_FUNC_DIS != last_status)
		{
			last_status = ISP_CLK_FUNC_DIS;
			isp_video_set_clk_func_status(ISP_CLK_FUNC_DIS);
		}
	} else if (ISP_CLK_FUNC_EN == status) {
		if (ISP_CLK_FUNC_EN != last_status)
		{
			last_status = ISP_CLK_FUNC_EN;
			isp_video_set_clk_func_status(ISP_CLK_FUNC_EN);
		}
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 47)
static void isp_dump_virt(void *virt_addr, unsigned long buff_size, struct file *filp)
{
	u8 *isp_virt_base = (u8 *)virt_addr;

	if (filp == NULL) {
		printk("(isp_dbg) filp is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_virt_base == NULL) {
		printk("(isp_dbg) isp_virt_base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (buff_size == 0) {
		printk("(isp_dbg) buff size is 0 %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (DBG_LOG)
		printk("(isp_dbg) dump mem buff_size=0x%lx %s(%d) \n", buff_size, __func__, __LINE__);

	if (!IS_ERR(filp) && isp_virt_base != NULL) {
		loff_t pos = 0;
		ssize_t w_size = kernel_write(filp, (char *)isp_virt_base, buff_size, &pos);
		if (w_size != buff_size)
			printk("(isp_dbg) dump mem write file error %s(%d) \n", __func__, __LINE__);
	}
}

static void isp_dump_sram(void __iomem *isp_virt_base, unsigned long buff_size, struct file *filp)
{
	if (filp == NULL) {
		printk("filp is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_virt_base == NULL) {
		printk("isp_virt_base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (buff_size == 0) {
		printk("buff size is 0 %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (DBG_LOG)
		printk("dump sram buff_size=0x%lx  %s(%d) \n", buff_size, __func__, __LINE__);

	if (!IS_ERR(filp) && isp_virt_base != NULL) {
		u64 i = 0;
		u64 totalNum = buff_size >> 2; // buff_size * 8 / 32
		loff_t pos = 0;

		for (i = 0; i < totalNum; i++) {
			u32 data = readl(isp_virt_base + i * 4);
			ssize_t w_size = kernel_write(filp, (char *)(&data), sizeof(u32), &pos);

			if (w_size != sizeof(u32))
				printk("dump mem write file error %s(%d) \n", __func__, __LINE__);
		}
	}
}

static void isp_dump_mlsc(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 func_en = 0;
	dma_addr_t statis_mem_base = 0;

	if (last_hdl == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (folder_name == NULL)
		return;

	READ_REG(isp_base, ISP_FUNC_EN_0, &func_en);
	if ((func_en & FUNC_EN_0_MLSC_EN_MASK) == 0x0)
		return;

	READ_REG(isp_base, ISP_DMA_8, &statis_mem_base);
	if (statis_mem_base != 0x0) {
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/in_%d_%d_mlsc.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump mlsc in %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);

		if (IS_ERR(filp)) {
			printk("(isp_dbg) create file %s error, exiting...\n", filename);
		} else {
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			isp_dump_virt(last_hdl->hw_buff.mlsc_virt, MLSC_BUFF_MAX_SIZE, filp);

			set_fs(old_fs);
		}

		if (filp != NULL) {
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

static void isp_dump_fcurve(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 func_en = 0;
	dma_addr_t statis_mem_base = 0;

	if (last_hdl == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (folder_name == NULL)
		return;

	READ_REG(isp_base, ISP_FUNC_EN_0, &func_en);
	if ((func_en & FUNC_EN_0_FCURVE_SUBOUT_EN_MASK) == 0x0)
		return;

	// fcurve in
	READ_REG(isp_base, ISP_DMA_4, &statis_mem_base);
	if (statis_mem_base != 0x0) {
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/in_%d_%d_fcurve.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump fcurve in %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp)) {
			printk("(isp_dbg) create file %s error, exiting...\n", filename);
		} else {
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.fcurve_buff.sram_en == 1) {
				if (last_hdl->hw_buff.fcurve_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.fcurve_buff.virt_addr, FCURVE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_sram(last_hdl->hw_buff.fcurve_buff.virt_addr + last_hdl->hw_buff.fcurve_buff.max_size, FCURVE_BUFF_MAX_SIZE, filp);
			} else {
				if (last_hdl->hw_buff.fcurve_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.fcurve_buff.virt_addr, FCURVE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_virt(last_hdl->hw_buff.fcurve_buff.virt_addr + last_hdl->hw_buff.fcurve_buff.max_size, FCURVE_BUFF_MAX_SIZE, filp);
			}

			set_fs(old_fs);
		}
		if (filp != NULL) {
			filp_close(filp, NULL);
			filp = NULL;
		}
	}

	// fcurve out
	READ_REG(isp_base, ISP_DMA_6, &statis_mem_base);
	if (statis_mem_base != 0x0) {
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/out_%d_%d_fcurve.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump fcurve out %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp)) {
			printk("create file %s error, exiting...\n", filename);
		} else {
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.fcurve_buff.sram_en == 1) {
				if (last_hdl->hw_buff.fcurve_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.fcurve_buff.virt_addr + last_hdl->hw_buff.fcurve_buff.max_size, FCURVE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_sram(last_hdl->hw_buff.fcurve_buff.virt_addr, FCURVE_BUFF_MAX_SIZE, filp);
			} else {
				if (last_hdl->hw_buff.fcurve_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.fcurve_buff.virt_addr + last_hdl->hw_buff.fcurve_buff.max_size, FCURVE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_virt(last_hdl->hw_buff.fcurve_buff.virt_addr, FCURVE_BUFF_MAX_SIZE, filp);
			}

			set_fs(old_fs);
		}

		if (filp != NULL) {
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

static void isp_dump_wdr(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 func_en = 0;
	dma_addr_t statis_mem_base = 0;

	if (last_hdl == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (folder_name == NULL)
		return;

	READ_REG(isp_base, ISP_FUNC_EN_1, &func_en);
	if ((func_en & FUNC_EN_1_WDR_SUBOUT_EN_MASK) == 0x0)
		return;

	// wdr in
	READ_REG(isp_base, ISP_DMA_10, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/in_%d_%d_wdr.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump wdr in %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("(isp_dbg) create file %s error, exiting...\n", "wdr_in.raw");
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.wdr_buff.sram_en == 1)
			{
				if (last_hdl->hw_buff.wdr_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.wdr_buff.virt_addr, WDR_BUFF_MAX_SIZE, filp);
				else
					isp_dump_sram(last_hdl->hw_buff.wdr_buff.virt_addr + last_hdl->hw_buff.wdr_buff.max_size, WDR_BUFF_MAX_SIZE, filp);
			}
			else
			{
				if (last_hdl->hw_buff.wdr_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.wdr_buff.virt_addr, WDR_BUFF_MAX_SIZE, filp);
				else
					isp_dump_virt(last_hdl->hw_buff.wdr_buff.virt_addr + last_hdl->hw_buff.wdr_buff.max_size, WDR_BUFF_MAX_SIZE, filp);
			}

			set_fs(old_fs);
		}
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}

	// wdr out
	READ_REG(isp_base, ISP_DMA_12, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/out_%d_%d_wdr.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump wdr out %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.wdr_buff.sram_en == 1)
			{
				if (last_hdl->hw_buff.wdr_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.wdr_buff.virt_addr + last_hdl->hw_buff.wdr_buff.max_size, WDR_BUFF_MAX_SIZE, filp);
				else
					isp_dump_sram(last_hdl->hw_buff.wdr_buff.virt_addr, WDR_BUFF_MAX_SIZE, filp);
			}
			else
			{
				if (last_hdl->hw_buff.wdr_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.wdr_buff.virt_addr + last_hdl->hw_buff.wdr_buff.max_size, WDR_BUFF_MAX_SIZE, filp);
				else
					isp_dump_virt(last_hdl->hw_buff.wdr_buff.virt_addr, WDR_BUFF_MAX_SIZE, filp);
			}

			set_fs(old_fs);
		}
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

static void isp_dump_lce(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 func_en = 0;
	dma_addr_t statis_mem_base = 0;

	if (last_hdl == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (folder_name == NULL)
		return;

	READ_REG(isp_base, ISP_FUNC_EN_1, &func_en);
	if ((func_en & FUNC_EN_1_LCE_SUBOUT_ENABLE_MASK) == 0x0)
		return;

	// lce in
	READ_REG(isp_base, ISP_DMA_14, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/in_%d_%d_lce.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump lce in %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("(isp_dbg) create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.lce_buff.sram_en == 1)
			{
				if (last_hdl->hw_buff.lce_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.lce_buff.virt_addr, LCE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_sram(last_hdl->hw_buff.lce_buff.virt_addr + last_hdl->hw_buff.lce_buff.max_size, LCE_BUFF_MAX_SIZE, filp);
			}
			else
			{
				if (last_hdl->hw_buff.lce_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.lce_buff.virt_addr, LCE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_virt(last_hdl->hw_buff.lce_buff.virt_addr + last_hdl->hw_buff.lce_buff.max_size, LCE_BUFF_MAX_SIZE, filp);
			}

			set_fs(old_fs);
		}
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}

	// lce out
	READ_REG(isp_base, ISP_DMA_16, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/out_%d_%d_lce.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump lce out %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.lce_buff.sram_en == 1)
			{
				if (last_hdl->hw_buff.lce_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.lce_buff.virt_addr + last_hdl->hw_buff.lce_buff.max_size, LCE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_sram(last_hdl->hw_buff.lce_buff.virt_addr, LCE_BUFF_MAX_SIZE, filp);
			}
			else
			{
				if (last_hdl->hw_buff.lce_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.lce_buff.virt_addr + last_hdl->hw_buff.lce_buff.max_size, LCE_BUFF_MAX_SIZE, filp);
				else
					isp_dump_virt(last_hdl->hw_buff.lce_buff.virt_addr, LCE_BUFF_MAX_SIZE, filp);
			}

			set_fs(old_fs);
		}
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

static void isp_dump_cnr(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 func_en = 0;
	u32 width = 0;
	u32 height = 0;
	dma_addr_t statis_mem_base = 0;

	if (last_hdl == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (folder_name == NULL)
		return;

	READ_REG(isp_base, ISP_FUNC_EN_1, &func_en);
	if ((func_en & FUNC_EN_1_CNR_SUBOUT_ENABLE_MASK) == 0x0)
		return;

	READ_REG(isp_base, ISP_CONTROL_0, &width);
	height = (width & CONTROL_0_HEIGHT_MASK) >> CONTROL_0_HEIGHT_LSB;
	width = (width & CONTROL_0_WIDTH_MASK);

	// cnr in
	READ_REG(isp_base, ISP_DMA_18, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/in_%d_%d_cnr.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump cnr in %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("(isp_dbg) create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.cnr_buff.sram_en == 1)
			{
				if (last_hdl->hw_buff.cnr_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.cnr_buff.virt_addr, CNR_SIZE_FORMULA(width, height), filp);
				else
					isp_dump_sram(last_hdl->hw_buff.cnr_buff.virt_addr + last_hdl->hw_buff.cnr_buff.max_size, CNR_SIZE_FORMULA(width, height), filp);
			}
			else
			{
				if (last_hdl->hw_buff.cnr_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.cnr_buff.virt_addr, CNR_SIZE_FORMULA(width, height), filp);
				else
					isp_dump_virt(last_hdl->hw_buff.cnr_buff.virt_addr + last_hdl->hw_buff.cnr_buff.max_size, CNR_SIZE_FORMULA(width, height), filp);
			}

			set_fs(old_fs);
		}
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}

	// cnr out
	READ_REG(isp_base, ISP_DMA_20, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/out_%d_%d_cnr.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump cnr out %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			if (last_hdl->hw_buff.cnr_buff.sram_en == 1)
			{
				if (last_hdl->hw_buff.cnr_buff.buff_idx == 0)
					isp_dump_sram(last_hdl->hw_buff.cnr_buff.virt_addr + last_hdl->hw_buff.cnr_buff.max_size, CNR_SIZE_FORMULA(width, height), filp);
				else
					isp_dump_sram(last_hdl->hw_buff.cnr_buff.virt_addr, CNR_SIZE_FORMULA(width, height), filp);
			}
			else
			{
				if (last_hdl->hw_buff.cnr_buff.buff_idx == 0)
					isp_dump_virt(last_hdl->hw_buff.cnr_buff.virt_addr + last_hdl->hw_buff.cnr_buff.max_size, CNR_SIZE_FORMULA(width, height), filp);
				else
					isp_dump_virt(last_hdl->hw_buff.cnr_buff.virt_addr, CNR_SIZE_FORMULA(width, height), filp);
			}

			set_fs(old_fs);
		}
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

static void isp_dump_3dnr_yuv(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 func_en = 0;
	u32 width = 0;
	u32 height = 0;
	u8 sep_on = 0;
	u32 sc_format = 0; // 0:444, 1:420
	dma_addr_t statis_mem_base = 0;

	struct file *filp_combo_in = NULL;
	mm_segment_t old_fs_combo_in;
	struct file *filp_combo_out = NULL;
	mm_segment_t old_fs_combo_out;

	char filename_combo_in[ISP_FILE_NAME_SIZE];
	char filename_combo_out[ISP_FILE_NAME_SIZE];

	loff_t pos_combo_in = 0;
	loff_t pos_combo_out = 0;

	if (last_hdl == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (folder_name == NULL)
		return;

	sprintf(filename_combo_in, "%s/in_%d_%d_3dnr_yuv.raw", folder_name, last_hdl->regQ_id, frame_cnt);
	sprintf(filename_combo_out, "%s/out_%d_%d_3dnr_yuv.raw", folder_name, last_hdl->regQ_id, frame_cnt);
	filp_combo_in = filp_open(filename_combo_in, O_RDWR | O_CREAT, 0644);
	filp_combo_out = filp_open(filename_combo_out, O_RDWR | O_CREAT, 0644);

	READ_REG(isp_base, ISP_DNR_INOUT_FORMAT_CTRL, &func_en);
	if ((func_en & DNR_INOUT_FORMAT_CTRL_NR3D_YUV_REF_OUT_EN_MASK) == 0x0)
		return;

	// dump 3ndr input
	READ_REG(isp_base, ISP_CONTROL_0, &width);
	height = (width & CONTROL_0_HEIGHT_MASK) >> CONTROL_0_HEIGHT_LSB;
	width = (width & CONTROL_0_WIDTH_MASK);

	READ_REG(isp_base, ISP_DNR_INOUT_FORMAT_CTRL, &sc_format)
	switch (sc_format & DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_IN_MASK)
	{
	case 3: // yuv444p
	case 4: // yuv420p
	case 5: // yuv422p
		sep_on = 1;
		break;
	default:
		sep_on = 0;
		break;
	}

	// 3dnr y in
	READ_REG(isp_base, ISP_DMA_22, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/in_%d_%d_3dnr_y.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump 3dnr y in %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("(isp_dbg) create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			isp_dump_virt(last_hdl->hw_buff.d3nr_y_buff.virt_addr, D3NR_Y_SIZE_FORMULA(width, height), filp);

			set_fs(old_fs);
			if (filp != NULL)
			{
				filp_close(filp, NULL);
				filp = NULL;
			}

			// combo in
			old_fs_combo_in = get_fs();
			set_fs(KERNEL_DS);
			if (!IS_ERR(filp_combo_in))
			{
				ssize_t w_size = kernel_write(filp_combo_in, (char *)last_hdl->hw_buff.d3nr_y_buff.virt_addr, (size_t)width * height, &pos_combo_in);
				if (w_size != (size_t)width * height)
					printk("(isp_dbg) dump 3dnr y in combo mem write file error %s(%d) \n", __func__, __LINE__);
			}
			set_fs(old_fs_combo_in);
		}
	}

	if (sep_on == 0)
	{ // 3dnr uv in
		READ_REG(isp_base, ISP_DMA_26, &statis_mem_base);
		if (statis_mem_base != 0x0)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/in_%d_%d_3dnr_uv.raw", folder_name, last_hdl->regQ_id, frame_cnt);

			if (DBG_LOG)
				printk("(isp_dbg) dump 3dnr uv in %s(%d)\n", __func__, __LINE__);

			filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
			if (IS_ERR(filp))
			{
				printk("(isp_dbg) create file %s error, exiting...\n", filename);
			}
			else
			{
				old_fs = get_fs();
				set_fs(KERNEL_DS);

				isp_dump_virt(last_hdl->hw_buff.d3nr_uv_buff.virt_addr, D3NR_UV_SIZE_FORMULA(width, height), filp);

				set_fs(old_fs);
				if (filp != NULL)
				{
					filp_close(filp, NULL);
					filp = NULL;
				}

				// combo in
				old_fs_combo_in = get_fs();
				set_fs(KERNEL_DS);
				if (!IS_ERR(filp_combo_in))
				{
					ssize_t w_size = kernel_write(filp_combo_in, (char *)last_hdl->hw_buff.d3nr_uv_buff.virt_addr, (size_t)width * height * 2, &pos_combo_in);
					if (w_size != (size_t)width * height * 2)
						printk("(isp_dbg) dump 3dnr uv in combo mem write file error %s(%d) \n", __func__, __LINE__);
				}
				set_fs(old_fs_combo_in);
			}
		}
	}
	else
	{ // 3dnr in u
		READ_REG(isp_base, ISP_DMA_49, &statis_mem_base);
		if (statis_mem_base != 0x0)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/in_%d_%d_3dnr_u.raw", folder_name, last_hdl->regQ_id, frame_cnt);

			if (DBG_LOG)
				printk("(isp_dbg) dump 3dnr u in %s(%d)\n", __func__, __LINE__);

			filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
			if (IS_ERR(filp))
			{
				printk("(isp_dbg) create file %s error, exiting...\n", filename);
			}
			else
			{
				old_fs = get_fs();
				set_fs(KERNEL_DS);

				isp_dump_virt(last_hdl->hw_buff.d3nr_uv_buff.virt_addr, D3NR_U_SIZE_FORMULA(width, height), filp);

				set_fs(old_fs);
				if (filp != NULL)
				{
					filp_close(filp, NULL);
					filp = NULL;
				}
				// combo in
				old_fs_combo_in = get_fs();
				set_fs(KERNEL_DS);
				if (!IS_ERR(filp_combo_in))
				{
					ssize_t w_size = kernel_write(filp_combo_in, (char *)last_hdl->hw_buff.d3nr_uv_buff.virt_addr, (size_t)width * height, &pos_combo_in);
					if (w_size != (size_t)width * height)
						printk("(isp_dbg) dump 3dnr u in combo mem write file error %s(%d) \n", __func__, __LINE__);
				}
				set_fs(old_fs_combo_in);
			}
		}

		// 3dnr in v
		READ_REG(isp_base, ISP_DMA_50, &statis_mem_base);
		if (statis_mem_base != 0x0)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/in_%d_%d_3dnr_v.raw", folder_name, last_hdl->regQ_id, frame_cnt);

			if (DBG_LOG)
				printk("(isp_dbg) dump 3dnr v in %s(%d)\n", __func__, __LINE__);

			filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
			if (IS_ERR(filp))
			{
				printk("(isp_dbg) create file %s error, exiting...\n", filename);
			}
			else
			{
				old_fs = get_fs();
				set_fs(KERNEL_DS);

				isp_dump_virt(last_hdl->hw_buff.d3nr_v_buff.virt_addr, D3NR_V_SIZE_FORMULA(width, height), filp);

				set_fs(old_fs);
				if (filp != NULL)
				{
					filp_close(filp, NULL);
					filp = NULL;
				}

				// combo in
				old_fs_combo_in = get_fs();
				set_fs(KERNEL_DS);
				if (!IS_ERR(filp_combo_in))
				{
					ssize_t w_size = kernel_write(filp_combo_in, (char *)last_hdl->hw_buff.d3nr_v_buff.virt_addr, (size_t)width * height, &pos_combo_in);
					if (w_size != (size_t)width * height)
						printk("(isp_dbg) dump 3dnr v in combo mem write file error %s(%d) \n", __func__, __LINE__);
				}
				set_fs(old_fs_combo_in);
			}
		}
	}

	// dump 3dnr output
	sc_format = (sc_format & DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_OUT_MASK) >> DNR_INOUT_FORMAT_CTRL_NR3D_YUV_FORMAT_OUT_LSB;

	switch (sc_format)
	{
	case 3: // yuv444p
	case 4: // yuv420p
	case 5: // yuv422p
		sep_on = 1;
		break;
	default:
		sep_on = 0;
		break;
	}

	// 3dnr out
	READ_REG(isp_base, ISP_DMA_24, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/out_%d_%d_3dnr_y.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump 3dnr y out %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("(isp_dbg) create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			isp_dump_virt(last_hdl->hw_buff.d3nr_y_buff.virt_addr_out, D3NR_Y_SIZE_FORMULA(width, height), filp);

			set_fs(old_fs);
			if (filp != NULL)
			{
				filp_close(filp, NULL);
				filp = NULL;
			}

			// combo out
			old_fs_combo_out = get_fs();
			set_fs(KERNEL_DS);
			if (!IS_ERR(filp_combo_out))
			{
				ssize_t w_size = kernel_write(filp_combo_out, (char *)last_hdl->hw_buff.d3nr_y_buff.virt_addr_out, (size_t)width * height, &pos_combo_out);
				if (w_size != (size_t)width * height)
					printk("(isp_dbg) dump 3dnr y out combo mem write file error %s(%d) \n", __func__, __LINE__);
			}
			set_fs(old_fs_combo_out);
		}
	}

	if (sep_on == 0)
	{ // 3dnr out uv
		READ_REG(isp_base, ISP_DMA_28, &statis_mem_base);
		if (statis_mem_base != 0x0)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_%d_%d_3dnr_uv.raw", folder_name, last_hdl->regQ_id, frame_cnt);

			if (DBG_LOG)
				printk("(isp_dbg) dump 3dnr uv out %s(%d)\n", __func__, __LINE__);

			filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
			if (IS_ERR(filp))
			{
				printk("create file %s error, exiting...\n", filename);
			}
			else
			{
				old_fs = get_fs();
				set_fs(KERNEL_DS);

				isp_dump_virt(last_hdl->hw_buff.d3nr_uv_buff.virt_addr_out, D3NR_UV_SIZE_FORMULA(width, height), filp);

				set_fs(old_fs);
				if (filp != NULL)
				{
					filp_close(filp, NULL);
					filp = NULL;
				}

				// combo out
				old_fs_combo_out = get_fs();
				set_fs(KERNEL_DS);
				if (!IS_ERR(filp_combo_out))
				{
					ssize_t w_size = kernel_write(filp_combo_out, (char *)last_hdl->hw_buff.d3nr_uv_buff.virt_addr_out, (size_t)width * height * 2, &pos_combo_out);
					if (w_size != (size_t)width * height * 2)
						printk("(isp_dbg) dump 3dnr uv out combo mem write file error %s(%d) \n", __func__, __LINE__);
				}
				set_fs(old_fs_combo_out);
			}
		}
	}
	else
	{ // 3dnr out u
		READ_REG(isp_base, ISP_DMA_51, &statis_mem_base);
		if (statis_mem_base != 0x0)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_%d_%d_3dnr_u.raw", folder_name, last_hdl->regQ_id, frame_cnt);

			if (DBG_LOG)
				printk("(isp_dbg) dump 3dnr u out %s(%d)\n", __func__, __LINE__);

			filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
			if (IS_ERR(filp))
			{
				printk("create file %s error, exiting...\n", filename);
			}
			else
			{
				old_fs = get_fs();
				set_fs(KERNEL_DS);

				isp_dump_virt(last_hdl->hw_buff.d3nr_uv_buff.virt_addr_out, D3NR_U_SIZE_FORMULA(width, height), filp);

				set_fs(old_fs);
				if (filp != NULL)
				{
					filp_close(filp, NULL);
					filp = NULL;
				}

				// combo out
				old_fs_combo_out = get_fs();
				set_fs(KERNEL_DS);
				if (!IS_ERR(filp_combo_out))
				{
					ssize_t w_size = kernel_write(filp_combo_out, (char *)last_hdl->hw_buff.d3nr_uv_buff.virt_addr_out, (size_t)width * height, &pos_combo_out);
					if (w_size != (size_t)width * height)
						printk("(isp_dbg) dump 3dnr u out combo mem write file error %s(%d) \n", __func__, __LINE__);
				}
				set_fs(old_fs_combo_out);
			}
		}

		// 3dnr out v
		READ_REG(isp_base, ISP_DMA_52, &statis_mem_base);
		if (statis_mem_base != 0x0)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_%d_%d_3dnr_v.raw", folder_name, last_hdl->regQ_id, frame_cnt);

			if (DBG_LOG)
				printk("(isp_dbg) dump 3dnr v out %s(%d)\n", __func__, __LINE__);

			filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
			if (IS_ERR(filp))
			{
				printk("create file %s error, exiting...\n", filename);
			}
			else
			{
				old_fs = get_fs();
				set_fs(KERNEL_DS);

				isp_dump_virt(last_hdl->hw_buff.d3nr_v_buff.virt_addr_out, D3NR_V_SIZE_FORMULA(width, height), filp);

				set_fs(old_fs);
				if (filp != NULL)
				{
					filp_close(filp, NULL);
					filp = NULL;
				}

				// combo out
				old_fs_combo_out = get_fs();
				set_fs(KERNEL_DS);
				if (!IS_ERR(filp_combo_out))
				{
					ssize_t w_size = kernel_write(filp_combo_out, (char *)last_hdl->hw_buff.d3nr_v_buff.virt_addr_out, (size_t)width * height, &pos_combo_out);
					if (w_size != (size_t)width * height)
						printk("(isp_dbg) dump 3dnr v out combo mem write file error %s(%d) \n", __func__, __LINE__);
				}
				set_fs(old_fs_combo_out);
			}
		}
	}

	if (IS_ERR(filp_combo_in))
	{
		printk("(isp_dbg) create file %s error, exiting...\n", filename_combo_in);
	}
	else
	{
		if (filp_combo_in != NULL)
		{
			filp_close(filp_combo_in, NULL);
			filp_combo_in = NULL;
		}
	}

	if (IS_ERR(filp_combo_out))
	{
		printk("(isp_dbg) create file %s error, exiting...\n", filename_combo_out);
	}
	else
	{
		if (filp_combo_out != NULL)
		{
			filp_close(filp_combo_out, NULL);
			filp_combo_out = NULL;
		}
	}
}

static void isp_dump_3dnr_mot(void __iomem *isp_base, char *folder_name, struct isp_video_fh *last_hdl, u32 frame_cnt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 func_en = 0;
	u32 width = 0;
	u32 height = 0;
	dma_addr_t statis_mem_base = 0;

	if (last_hdl == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (folder_name == NULL)
		return;

	READ_REG(isp_base, ISP_DNR_INOUT_FORMAT_CTRL, &func_en);
	if ((func_en & DNR_INOUT_FORMAT_CTRL_NR3D_MOTION_REF_OUT_EN_MASK) == 0x0)
		return;

	READ_REG(isp_base, ISP_CONTROL_0, &width);
	height = (width & CONTROL_0_HEIGHT_MASK) >> CONTROL_0_HEIGHT_LSB;
	width = (width & CONTROL_0_WIDTH_MASK);

	READ_REG(isp_base, ISP_DMA_44, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/in_%d_%d_3dnr_mot.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump 3dnr mot in %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("(isp_dbg) create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			isp_dump_virt(last_hdl->hw_buff.d3nr_mot_buff.virt_addr, D3NR_MOT_SIZE_FORMULA(width, height), filp);

			set_fs(old_fs);
			if (filp != NULL)
			{
				filp_close(filp, NULL);
				filp = NULL;
			}
		}
	}

	// 3dnr out
	READ_REG(isp_base, ISP_DMA_46, &statis_mem_base);
	if (statis_mem_base != 0x0)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/out_%d_%d_3dnr_mot.raw", folder_name, last_hdl->regQ_id, frame_cnt);

		if (DBG_LOG)
			printk("(isp_dbg) dump 3dnr mot out %s(%d)\n", __func__, __LINE__);

		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(filp))
		{
			printk("create file %s error, exiting...\n", filename);
		}
		else
		{
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			isp_dump_virt(last_hdl->hw_buff.d3nr_mot_buff.virt_addr_out, D3NR_MOT_SIZE_FORMULA(width, height), filp);

			set_fs(old_fs);
			if (filp != NULL)
			{
				filp_close(filp, NULL);
				filp = NULL;
			}
		}
	}
}

static void isp_dump_input(void __iomem *isp_base, char *file_name, void *input_virt)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 width = 0;
	u32 height = 0;
	size_t img_size = 0; // bytes
	u32 in_format = 0;

	if (file_name == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (input_virt == NULL)
		return;

	if (DBG_LOG)
		printk("(isp_dbg) dump input frame %s(%d)\n", __func__, __LINE__);

	filp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(filp))
	{
		printk("(isp_dbg) create file %s error, exiting...\n", file_name);
	}
	else
	{
		if (DBG_LOG)
			printk("(isp_dbg) dump input frame name: %s   %s(%d) \n", file_name, __func__, __LINE__);

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		READ_REG(isp_base, ISP_CONTROL_0, &width);
		height = (width & CONTROL_0_HEIGHT_MASK) >> CONTROL_0_HEIGHT_LSB;
		width = (width & CONTROL_0_WIDTH_MASK);

		READ_REG(isp_base, ISP_IN_FORMAT_CTRL, &in_format);
		switch (in_format)
		{
		case 0:
		case 1:
			// pack
			img_size = (size_t)width * height * 5 / 4;
			break;
		default:
			// unpack
			img_size = (size_t)width * height * 2;
			break;
		}

		isp_dump_virt(input_virt, img_size, filp);

		set_fs(old_fs);
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

static void isp_dump_output(char *file_name, void *input_virt, size_t img_size)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;

	if (file_name == NULL)
		return;

	if (input_virt == NULL)
		return;

	if (DBG_LOG)
		printk("(isp_dbg) dump output frame %s(%d)\n", __func__, __LINE__);

	filp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(filp))
	{
		printk("(isp_dbg) dump output file %s error, exiting...\n", file_name);
	}
	else
	{
		if (DBG_LOG)
			printk("(isp_dbg) dump output file name: %s   %s(%d) \n", file_name, __func__, __LINE__);

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		isp_dump_virt(input_virt, img_size, filp);

		set_fs(old_fs);
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

void isp_dbg_dump_output_frame(void __iomem *isp_base, struct isp_video_fh *vfh, struct cap_buff *cap_buf, u32 frame_cnt)
{
	char *folder_name = isp_dbgfs_get_dump_folder();
	void *y_virt = NULL;
	void *u_virt = NULL;
	void *v_virt = NULL;

	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (cap_buf == NULL)
	{
		printk("(isp_dbg) cap_buf is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (vfh == NULL)
	{
		printk("(isp_dbg) handle ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_dbgfs_target_idx_exist(vfh->regQ_id) == 0)
		return;

	if (folder_name != NULL)
	{
		u32 width = 0;
		u32 height = 0;
		u32 out_format = 0;
		size_t img_size = 0;
		char filename[ISP_FILE_NAME_SIZE];
		struct file *filp = NULL;
		mm_segment_t old_fs;
		loff_t pos = 0;

		if (isp_base == NULL)
		{
			printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
			return;
		}

		if (cap_buf == NULL)
		{
			printk("(isp_dbg) cap_buf is NULL %s(%d) \n", __func__, __LINE__);
			return;
		}

		sprintf(filename, "%s/out_yuv_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
		filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
		old_fs = get_fs();
		set_fs(KERNEL_DS);

		y_virt = cap_buf->y_out_virt;
		u_virt = cap_buf->u_out_virt;
		v_virt = cap_buf->v_out_virt;

		// dump out y
		READ_REG(isp_base, ISP_SCALER_1, &width);
		height = (width & SCALER_1_DST_HTO_MASK) >> SCALER_1_DST_HTO_LSB;
		width = (width & SCALER_1_DST_WDO_MASK);
		img_size = (size_t)width * height;
		if (y_virt != NULL)
		{
			if (!IS_ERR(filp))
			{
				ssize_t w_size = kernel_write(filp, (char *)y_virt, img_size, &pos);
				if (w_size != img_size)
					printk("(isp_dbg) dump y mem write file error %s(%d) \n", __func__, __LINE__);
			}
		}
		// dump output u/v
		READ_REG(isp_base, ISP_OUT_FORMAT_CTRL, &out_format);

		switch (out_format)
		{
		case 0:
			// 444
			img_size = (size_t)width * height * 2;
			break;
		case 1:
			// 420
			img_size = (size_t)width * height / 2;
			break;
		case 2:
			// 422
			img_size = (size_t)width * height;
			break;
		case 3:
			// 444p
			img_size = (size_t)width * height;
			break;
		case 4:
			// 420p
			img_size = (size_t)width * height / 4;
			break;
		case 5:
			// 422p
			img_size = (size_t)width * height / 2;
			break;
		default:
			img_size = 0;
			break;
		}

		if (u_virt != NULL)
		{
			if (!IS_ERR(filp))
			{
				ssize_t w_size = kernel_write(filp, (char *)u_virt, img_size, &pos);
				if (w_size != img_size)
					printk("(isp_dbg) dump u mem write file error %s(%d) \n", __func__, __LINE__);
			}
		}
		if (out_format >= 3 && v_virt != NULL)
		{
			if (!IS_ERR(filp))
			{
				ssize_t w_size = kernel_write(filp, (char *)v_virt, img_size, &pos);
				if (w_size != img_size)
					printk("(isp_dbg) dump v mem write file error %s(%d) \n", __func__, __LINE__);
			}
		}

		set_fs(old_fs);
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

void isp_dbg_dump_input_frame(void __iomem *isp_base, struct isp_video_fh *vfh, struct out_buff *out_buf, u32 frame_cnt)
{
	char *folder_name = isp_dbgfs_get_dump_folder();
	void *input_virt_1 = NULL;
	void *input_virt_2 = NULL;

	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (out_buf == NULL)
	{
		printk("(isp_dbg) out_buf is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (vfh == NULL)
	{
		printk("(isp_dbg) handle ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_dbgfs_target_idx_exist(vfh->regQ_id) == 0)
		return;

	input_virt_1 = out_buf->input_hdr1_virt;
	input_virt_2 = out_buf->input_hdr2_virt;

	if (folder_name != NULL)
	{
		if (input_virt_1 != NULL)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/in_hdr0_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
			isp_dump_input(isp_base, filename, input_virt_1);
		}
		if (input_virt_2 != NULL)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/in_hdr1_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
			isp_dump_input(isp_base, filename, input_virt_2);
		}
	}
}

void isp_dbg_dump_statis(void __iomem *isp_base, struct isp_video_fh *hdl, u32 frame_cnt)
{
	char *folder_name = isp_dbgfs_get_dump_folder();
	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}
	if (hdl == NULL)
	{
		printk("(isp_dbg) handle ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_dbgfs_target_idx_exist(hdl->regQ_id) == 0)
		return;

	if (folder_name != NULL)
	{
		isp_dump_mlsc(isp_base, folder_name, hdl, frame_cnt);
		isp_dump_fcurve(isp_base, folder_name, hdl, frame_cnt);
		isp_dump_wdr(isp_base, folder_name, hdl, frame_cnt);
		isp_dump_lce(isp_base, folder_name, hdl, frame_cnt);
		isp_dump_cnr(isp_base, folder_name, hdl, frame_cnt);
	}
}

void isp_dbg_dump_3dnr(void __iomem *isp_base, struct isp_video_fh *hdl, u32 frame_cnt)
{
	char *folder_name = isp_dbgfs_get_dump_folder();
	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}
	if (hdl == NULL)
	{
		printk("(isp_dbg) handle ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_dbgfs_target_idx_exist(hdl->regQ_id) == 0)
		return;

	if (folder_name != NULL)
	{
		isp_dump_3dnr_yuv(isp_base, folder_name, hdl, frame_cnt);
		isp_dump_3dnr_mot(isp_base, folder_name, hdl, frame_cnt);
	}
}

void isp_dbg_dump_3a(void __iomem *isp_base, struct isp_video_fh *vfh, struct cap_buff *cap_buff_ptr, u32 frame_cnt)
{
	char *folder_name = isp_dbgfs_get_dump_folder();
	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}
	if (cap_buff_ptr == NULL)
	{
		printk("(isp_dbg) cap_buff ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}
	if (vfh == NULL)
	{
		printk("(isp_dbg) handle ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_dbgfs_target_idx_exist(vfh->regQ_id) == 0)
		return;

	if (folder_name != NULL)
	{
		u32 func_en = 0;
		READ_REG(isp_base, ISP_FUNC_EN_2, &func_en);
		if ((func_en & FUNC_EN_2_AWB_EN_MASK) == FUNC_EN_2_AWB_EN_MASK)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_awb_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
			isp_dump_output(filename, cap_buff_ptr->awb_virt, cap_buff_ptr->awb_out_size);
		}
		if ((func_en & FUNC_EN_2_AE_EN_MASK) == FUNC_EN_2_AE_EN_MASK)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_ae1_ba_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
			isp_dump_output(filename, cap_buff_ptr->ae_ba_virt, cap_buff_ptr->ae_ba_size);
		}
		if ((func_en & FUNC_EN_2_AE_HIST_EN_MASK) == FUNC_EN_2_AE_HIST_EN_MASK)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_ae1_hist_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
			isp_dump_output(filename, cap_buff_ptr->ae_hist_virt, cap_buff_ptr->ae_hist_size);
		}
		if ((func_en & FUNC_EN_2_AE_EN2_MASK) == FUNC_EN_2_AE_EN2_MASK)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_ae2_ba_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
			isp_dump_output(filename, cap_buff_ptr->ae2_ba_virt, cap_buff_ptr->ae2_ba_size);
		}
		if ((func_en & FUNC_EN_2_AE_HIST_EN2_MASK) == FUNC_EN_2_AE_HIST_EN2_MASK)
		{
			char filename[ISP_FILE_NAME_SIZE];
			sprintf(filename, "%s/out_ae2_hist_%d_%d.raw", folder_name, vfh->regQ_id, frame_cnt);
			isp_dump_output(filename, cap_buff_ptr->ae2_hist_virt, cap_buff_ptr->ae2_hist_size);
		}
	}
}

void isp_dbg_dump_reg(char *filename, void __iomem *isp_base)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;
	enum ISP_CLK_GATE_STATUS cur_status = isp_video_get_clk_func_status();

	if (filename == NULL)
		return;

	if (isp_base == NULL)
		return;

	isp_dbg_set_clk_func(ISP_CLK_FUNC_DIS);

	filp = filp_open(filename, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(filp))
	{
		printk("create file %s error, exiting...\n", filename);
	}
	else
	{
		u16 idx = 0;

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		for (idx = 0; idx <= REG_MAX_OFFSET; idx += 4)
		{
			char reg_string[17];
			u32 reg_val = 0;
			ssize_t w_size = 0;

			READ_REG(isp_base, idx, &reg_val);

			sprintf(reg_string, "%08x%08x\n", idx, reg_val);

			w_size = kernel_write(filp, (char *)reg_string, 17, &pos);
			if (w_size < 0)
			{
				printk("dump reg write file error %s(%d) \n", __func__, __LINE__);
				break;
			}
		}

		set_fs(old_fs);
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}

	isp_dbg_set_clk_func(cur_status);
}

void isp_dbg_dump_frame_reg(void __iomem *isp_base, struct isp_video_fh *vfh, u32 frame_cnt)
{
	char *folder_name = isp_dbgfs_get_dump_folder();
	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (folder_name != NULL)
	{
		char filename[ISP_FILE_NAME_SIZE];
		sprintf(filename, "%s/reg_%d_%d.txt", folder_name, vfh->regQ_id, frame_cnt);
		isp_dbg_dump_reg(filename, isp_base);
	}
}

void isp_dbg_dump_last_reg(void)
{
	void __iomem *isp_base = isp_video_get_reg_base();
	char *folder_name = isp_dbgfs_get_dump_folder();
	char filename[100];

	if (folder_name == NULL)
		return;

	if (isp_base == NULL)
		return;

	sprintf(filename, "%s/reg_out.txt", folder_name);
	isp_dbg_dump_reg(filename, isp_base);
}

// === hard code section ===

static void isp_write_sram(void __iomem *isp_virt_base, unsigned long buff_size, struct file *filp)
{
	if (filp == NULL)
	{
		printk("(isp_dbg) filp is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_virt_base == NULL)
	{
		printk("(isp_dbg) isp_virt_base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (buff_size == 0)
	{
		printk("(isp_dbg) buff size is 0 %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (DBG_LOG)
		printk("(isp_dbg) write sram buff_size=0x%lx  %s(%d) \n", buff_size, __func__, __LINE__);

	if (!IS_ERR(filp) && isp_virt_base != NULL)
	{
		u64 i = 0, j = 0;
		u32 final_data = 0;
		u64 totalNum = buff_size >> 2;			 // buff_size * 8 / 32;
		u64 diffNum = buff_size - (totalNum << 2); // (buff_size * 8 - totalNum * 32)/8;
		loff_t pos = 0;

		for (i = 0; i < totalNum; i++)
		{
			u32 data = 0;
			ssize_t w_size = kernel_read(filp, (char *)(&data), sizeof(u32), &pos);
			if (w_size != sizeof(u32))
			{
				printk("dump mem write file error %s(%d) \n", __func__, __LINE__);
				break;
			}

			writel(data, isp_virt_base + i * 4);
		}

		for (j = 0; j < diffNum; j++)
		{
			u8 data = 0;
			ssize_t w_size = kernel_read(filp, (char *)(&data), sizeof(u8), &pos);

			if (w_size != sizeof(u8))
			{
				printk("dump mem write file error %s(%d) \n", __func__, __LINE__);
				break;
			}

			final_data += (data << (j * 8));
		}

		writel(final_data, isp_virt_base + i * 4);
	}
}

static void isp_write_virt(void *isp_virt_base, unsigned long buff_size, struct file *filp)
{
	if (filp == NULL)
	{
		printk("(isp_dbg) filp is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (buff_size == 0)
	{
		printk("(isp_dbg) buff size is 0 %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_virt_base == NULL)
	{
		printk("(isp_dbg) isp_virt_base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (DBG_LOG)
	{
		struct inode *inode;
		loff_t size;

		// Get the inode associated with the file
		inode = file_inode(filp);
		// Get the file size from the inode
		size = i_size_read(inode);

		printk("(isp_dbg) write mem buff_size=0x%lx %s(%d) \n", buff_size, __func__, __LINE__);
		printk("(isp_dbg) write mem file_size=0x%llx %s(%d) \n", size, __func__, __LINE__);
	}

	if (!IS_ERR(filp) && isp_virt_base != NULL)
	{
		loff_t pos = 0;
		ssize_t w_size = kernel_read(filp, (char *)isp_virt_base, buff_size, &pos);
		if (DBG_LOG && (w_size != buff_size))
			printk("(isp_dbg) write mem file error %s(%d) \n", __func__, __LINE__);
	}
}

static void isp_write_input(void __iomem *isp_base, void *in_addr, char *filename)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;
	u32 width = 0;
	u32 height = 0;
	size_t img_size = 0; // bytes
	u32 in_format = 0;

	if (filename == NULL)
		return;

	if (isp_base == NULL)
		return;

	if (in_addr == NULL)
		return;

	filp = filp_open(filename, O_RDWR, 0644);
	if (IS_ERR(filp))
	{
		printk("(isp_dbg) read input file %s error, exiting...\n", filename);
	}
	else
	{
		if (DBG_LOG)
			printk("(isp_dbg) hardcode input file name: %s   %s(%d) \n", filename, __func__, __LINE__);

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		READ_REG(isp_base, ISP_CONTROL_0, &width);
		height = (width & CONTROL_0_HEIGHT_MASK) >> CONTROL_0_HEIGHT_LSB;
		width = (width & CONTROL_0_WIDTH_MASK);

		READ_REG(isp_base, ISP_IN_FORMAT_CTRL, &in_format);
		switch (in_format)
		{
		case 0:
		case 1:
			// pack
			img_size = (size_t)width * height * 5 / 4;
			break;
		default:
			// unpack
			img_size = (size_t)width * height * 2;
			break;
		}

		if (DBG_LOG)
		{
			printk("(isp_dbg) hardcode input width=%d   %s(%d) \n", width, __func__, __LINE__);
			printk("(isp_dbg) hardcode input height=%d  %s(%d) \n", height, __func__, __LINE__);
			printk("(isp_dbg) hardcode input img_size=0x%lx  %s(%d) \n", img_size, __func__, __LINE__);
		}

		isp_write_virt(in_addr, img_size, filp);

		set_fs(old_fs);
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

void isp_dbg_hardcode_input(void __iomem *isp_base, struct isp_video_fh *vfh, void *hdr0_virt, void *hdr1_virt)
{
	char *filename = isp_dbgfs_get_input_filename();

	if (vfh == NULL)
	{
		printk("(isp_dbg) vfh ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_dbgfs_target_idx_exist(vfh->regQ_id))
	{
		if ((hdr0_virt != NULL) && (filename != NULL))
			isp_write_input(isp_base, hdr0_virt, filename);

		filename = isp_dbgfs_get_input_2_filename();
		if ((hdr1_virt != NULL) && (filename != NULL))
			isp_write_input(isp_base, hdr1_virt, filename);
	}
}

static void isp_write_statis(void *isp_dma_virt, void __iomem *isp_sram_virt, size_t in_size, char *filename, u8 sram_en)
{
	struct file *filp = NULL;
	mm_segment_t old_fs;

	if (filename == NULL)
		return;

	if ((sram_en == 0) && (isp_dma_virt == NULL))
		return;

	if ((sram_en == 1) && (isp_sram_virt == NULL))
		return;

	if (in_size == 0)
		return;

	filp = filp_open(filename, O_RDWR, 0644);
	if (IS_ERR(filp))
	{
		printk("(isp_dbg) statis input file %s error, exiting...\n", filename);
	}
	else
	{
		if (DBG_LOG)
			printk("(isp_dbg) hardcode statis input file name:%s   %s(%d) \n", filename, __func__, __LINE__);

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		if ((sram_en == 1) && (isp_sram_virt != NULL))
			isp_write_sram(isp_sram_virt, in_size, filp);
		else if ((sram_en == 0) && (isp_dma_virt != NULL))
			isp_write_virt(isp_dma_virt, in_size, filp);

		set_fs(old_fs);
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

void isp_dbg_hardcode_statis(void __iomem *isp_base, struct isp_video_fh *handle)
{
	u32 in_addr = 0;
	u32 width = 0;
	u32 height = 0;
	char *filename = isp_dbgfs_get_mlsc_filename();

	if (handle == NULL)
	{
		printk("(isp_dbg) handle ptr is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_base == NULL)
	{
		printk("(isp_dbg) reg base is NULL %s(%d) \n", __func__, __LINE__);
		return;
	}

	if (isp_dbgfs_target_idx_exist(handle->regQ_id) == 0)
		return;

	READ_REG(isp_base, ISP_CONTROL_0, &width);
	height = (width & CONTROL_0_HEIGHT_MASK) >> CONTROL_0_HEIGHT_LSB;
	width = (width & CONTROL_0_WIDTH_MASK);

	// mlsc
	filename = isp_dbgfs_get_mlsc_filename();
	if (filename != NULL)
	{
		void *in_addr = handle->hw_buff.mlsc_virt;
		isp_write_statis(in_addr, NULL, MLSC_BUFF_MAX_SIZE, filename, 0);
	}

	// fcurve
	filename = isp_dbgfs_get_fcurve_filename();
	if (filename != NULL)
	{
		if (handle->hw_buff.fcurve_buff.sram_en == 0)
		{
			void *in_addr = NULL;
			if (handle->hw_buff.fcurve_buff.buff_idx == 0)
				in_addr = handle->hw_buff.fcurve_buff.virt_addr;
			else
				in_addr = handle->hw_buff.fcurve_buff.virt_addr + handle->hw_buff.fcurve_buff.max_size;
			isp_write_statis(in_addr, NULL, FCURVE_BUFF_MAX_SIZE,
							 filename, handle->hw_buff.fcurve_buff.sram_en);
		}
		else
		{
			void __iomem *isp_sram_virt = NULL;
			if (handle->hw_buff.fcurve_buff.buff_idx == 0)
				isp_sram_virt = handle->hw_buff.fcurve_buff.virt_addr;
			else
				isp_sram_virt = handle->hw_buff.fcurve_buff.virt_addr + handle->hw_buff.fcurve_buff.max_size;

			isp_write_statis(NULL, isp_sram_virt, FCURVE_BUFF_MAX_SIZE,
							 filename, handle->hw_buff.fcurve_buff.sram_en);
		}
	}

	// wdr
	filename = isp_dbgfs_get_wdr_filename();
	if (filename != NULL)
	{
		if (handle->hw_buff.wdr_buff.sram_en == 0)
		{
			void *in_addr = NULL;
			if (handle->hw_buff.wdr_buff.buff_idx == 0)
				in_addr = handle->hw_buff.wdr_buff.virt_addr;
			else
				in_addr = handle->hw_buff.wdr_buff.virt_addr + handle->hw_buff.wdr_buff.max_size;
			isp_write_statis(in_addr, NULL, WDR_BUFF_MAX_SIZE,
							 filename, handle->hw_buff.wdr_buff.sram_en);
		}
		else
		{
			void __iomem *isp_sram_virt = NULL;
			if (handle->hw_buff.wdr_buff.buff_idx == 0)
				isp_sram_virt = handle->hw_buff.wdr_buff.virt_addr;
			else
				isp_sram_virt = handle->hw_buff.wdr_buff.virt_addr + handle->hw_buff.wdr_buff.max_size;
			isp_write_statis(NULL, isp_sram_virt, WDR_BUFF_MAX_SIZE,
							 filename, handle->hw_buff.wdr_buff.sram_en);
		}
	}

	// lce
	filename = isp_dbgfs_get_lce_filename();
	if (filename != NULL)
	{
		if (handle->hw_buff.lce_buff.sram_en == 0)
		{
			void *in_addr = NULL;
			if (handle->hw_buff.lce_buff.buff_idx == 0)
				in_addr = handle->hw_buff.lce_buff.virt_addr;
			else
				in_addr = handle->hw_buff.lce_buff.virt_addr + handle->hw_buff.lce_buff.max_size;
			isp_write_statis(in_addr, NULL, LCE_BUFF_MAX_SIZE,
							 filename, handle->hw_buff.lce_buff.sram_en);
		}
		else
		{
			void __iomem *isp_sram_virt = NULL;
			if (handle->hw_buff.lce_buff.buff_idx == 0)
				isp_sram_virt = handle->hw_buff.lce_buff.virt_addr;
			else
				isp_sram_virt = handle->hw_buff.lce_buff.virt_addr + handle->hw_buff.lce_buff.max_size;
			isp_write_statis(NULL, isp_sram_virt, LCE_BUFF_MAX_SIZE,
							 filename, handle->hw_buff.lce_buff.sram_en);
		}
	}

	// cnr
	filename = isp_dbgfs_get_cnr_filename();
	if (filename != NULL)
	{
		void *in_addr = NULL;
		if (handle->hw_buff.cnr_buff.buff_idx == 0)
			in_addr = handle->hw_buff.cnr_buff.virt_addr;
		else
			in_addr = handle->hw_buff.cnr_buff.virt_addr + handle->hw_buff.cnr_buff.max_size;
		isp_write_statis(in_addr, NULL, CNR_SIZE_FORMULA(width, height),
						 filename, handle->hw_buff.cnr_buff.sram_en);
	}

	// 3dnr y
	filename = isp_dbgfs_get_3dnr_y_filename();
	if (filename != NULL)
	{
		isp_write_statis(handle->hw_buff.d3nr_y_buff.virt_addr, NULL,
						 D3NR_Y_SIZE_FORMULA(width, height),
						 filename, handle->hw_buff.d3nr_y_buff.sram_en);
	}

	// 3dnr u
	filename = isp_dbgfs_get_3dnr_uv_filename();
	if (filename != NULL)
	{
		READ_REG(isp_base, ISP_DMA_26, &in_addr);
		if (in_addr != 0)
			isp_write_statis(handle->hw_buff.d3nr_uv_buff.virt_addr, NULL,
							 D3NR_UV_SIZE_FORMULA(width, height),
							 filename, handle->hw_buff.d3nr_uv_buff.sram_en);
		else
			isp_write_statis(handle->hw_buff.d3nr_uv_buff.virt_addr, NULL,
							 D3NR_U_SIZE_FORMULA(width, height),
							 filename, handle->hw_buff.d3nr_uv_buff.sram_en);
	}

	// 3dnr v
	filename = isp_dbgfs_get_3dnr_v_filename();
	if (filename != NULL)
	{
		isp_write_statis(handle->hw_buff.d3nr_v_buff.virt_addr, NULL,
						 D3NR_V_SIZE_FORMULA(width, height),
						 filename, handle->hw_buff.d3nr_v_buff.sram_en);
	}

	// 3dnr mot
	filename = isp_dbgfs_get_3dnr_mot_filename();
	if (filename != NULL)
	{
		isp_write_statis(handle->hw_buff.d3nr_mot_buff.virt_addr, NULL,
						 D3NR_MOT_SIZE_FORMULA(width, height),
						 filename, handle->hw_buff.d3nr_mot_buff.sram_en);
	}
}

static int isp_reg_atoi(char *buff, u32 *reg, u32 *val)
{
	int idx = 0;
	char *val_char = buff + 8;

	if (buff == NULL)
		return -1;

	if (strncmp(val_char, "deaddead", 8) == 0)
		return -1;
	if (strncmp(val_char, "DEADDEAD", 8) == 0)
		return -1;

	for (idx = 0; idx < 8; idx++)
	{
		if (buff[idx] >= '0' && buff[idx] <= '9')
			*reg = *reg * 16 + buff[idx] - '0';
		else if (buff[idx] >= 'a' && buff[idx] <= 'f')
			*reg = *reg * 16 + buff[idx] - 'a' + 10;
		else if (buff[idx] >= 'A' && buff[idx] <= 'F')
			*reg = *reg * 16 + buff[idx] - 'A' + 10;
		else
		{
			printk("(isp_dbg) buff=%s idx=%d data=%c \n", buff, idx, buff[idx]);
			return -1;
		}
	}

	for (idx = 8; idx < 16; idx++)
	{
		if (buff[idx] >= '0' && buff[idx] <= '9')
			*val = *val * 16 + buff[idx] - '0';
		else if (buff[idx] >= 'a' && buff[idx] <= 'f')
			*val = *val * 16 + buff[idx] - 'a' + 10;
		else if (buff[idx] >= 'A' && buff[idx] <= 'F')
			*val = *val * 16 + buff[idx] - 'A' + 10;
		else
		{
			printk("(isp_dbg) buff=%s idx=%d data=%c \n", buff, idx, buff[idx]);
			return -1;
		}
	}

	return 0;
}

static void isp_reg_from_file(void __iomem *isp_base)
{
	char *filename = isp_dbgfs_get_reg_filename();
	struct file *filp = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;

	if (filename == NULL)
		return;

	filp = filp_open(filename, O_RDWR, 0644);

	if (IS_ERR(filp))
	{
		printk("(isp_dbg) read reg file %s error, exiting...\n", filename);
	}
	else
	{
		u32 regloop = 0;
		char buf[17];
		struct inode *inode;
		loff_t size;

		if (DBG_LOG)
			printk("(isp_dbg) hardcode reg file name %s  %s(%d) \n", filename, __func__, __LINE__);

		// Get the inode associated with the file
		inode = file_inode(filp);
		// Get the file size from the inode
		size = i_size_read(inode);
		regloop = size / 17;

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		while (regloop > 0)
		{
			u32 reg = 0;
			u32 val = 0;

			if (kernel_read(filp, buf, 17, &pos) != 17)
			{
				printk("(isp_dbg) Failed to read file %s contents, pos=%lld \n", filename, pos);
				filp_close(filp, NULL);
				return;
			}

			if (isp_reg_atoi(buf, &reg, &val) != -1)
			{
				// printk("(isp_dbg) buff=%s reg=0x%x val=0x%x in\n", buf, reg, val);
				if (ISP_SW_RESET_EN == reg)
					continue;
				if (ISP_CR_REGION_VLD == reg)
					continue;
				if (ISP_INTERRUPT_EN == reg)
					continue;
				WRITE_REG(isp_base, reg, val, 0xFFFFFFFF);
			}
			regloop--;
		}

		set_fs(old_fs);
		if (filp != NULL)
		{
			filp_close(filp, NULL);
			filp = NULL;
		}
	}
}

void isp_dbg_hardcode_reg(void __iomem *isp_base, struct isp_video_fh *vfh)
{
	u32 dbg_ctrl = 0;

	if (isp_base == NULL)
		return;

	READ_REG(isp_base, ISP_ISP_SW0, &dbg_ctrl)

	if ((dbg_ctrl & ISP_SW0_SW_TMP0_DBG_EN_MASK) == ISP_SW0_SW_TMP0_DBG_EN_MASK)
	{
		// hard code func_en 0
		if ((dbg_ctrl & ISP_SW0_SW_TMP0_FUNC_0_MASK) == ISP_SW0_SW_TMP0_FUNC_0_MASK)
		{
			u32 hardcode_func = 0;
			READ_REG(isp_base, ISP_ISP_SW1, &hardcode_func)

			WRITE_REG(isp_base, ISP_FUNC_EN_0, hardcode_func, 0xFFFFFFFF)

			if (DBG_LOG)
				printk("(isp_dbg) hardcode func0 = 0x%x  %s(%d)\n", hardcode_func, __func__, __LINE__);
		}

		// hard code func_en 1
		if ((dbg_ctrl & ISP_SW0_SW_TMP0_FUNC_1_MASK) == ISP_SW0_SW_TMP0_FUNC_1_MASK)
		{
			u32 hardcode_func = 0;
			READ_REG(isp_base, ISP_ISP_SW2, &hardcode_func)

			WRITE_REG(isp_base, ISP_FUNC_EN_1, hardcode_func, 0xFFFFFFFF)

			if (DBG_LOG)
				printk("(isp_dbg) hardcode func1 = 0x%x  %s(%d)\n", hardcode_func, __func__, __LINE__);
		}

		// hard code func_en 2
		if ((dbg_ctrl & ISP_SW0_SW_TMP0_FUNC_2_MASK) == ISP_SW0_SW_TMP0_FUNC_2_MASK)
		{
			u32 hardcode_func = 0;
			READ_REG(isp_base, ISP_ISP_SW3, &hardcode_func)

			WRITE_REG(isp_base, ISP_FUNC_EN_2, hardcode_func, 0xFFFFFFFF)

			if (DBG_LOG)
				printk("(isp_dbg) hardcode func2 = 0x%x  %s(%d)\n", hardcode_func, __func__, __LINE__);
		}
	}

	if (isp_dbgfs_target_idx_exist(vfh->regQ_id))
		isp_reg_from_file(isp_base);
}
#endif
