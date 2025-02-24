// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/rawnand.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/of.h>
#include "sp_bch.h"
#include "sp_spinand.h"

/**************************************************************************
 *                             M A C R O S                                *
 **************************************************************************/
#define NAND_DEVICE_NAME  "sp_spinand.0"

/**************************************************************************
 *                        F U N C T I O N S                               *
 **************************************************************************/
/*static*/ void dump_spi_regs(struct sp_spinand_info *info)
{
	u32 *p = (u32 *)info->regs;
	int i;
	static const char * const reg_name[] = {
		"spi_ctrl",
		"spi_timing",
		"spi_page_addr",
		"spi_data",
		"spi_status",
		"spi_auto_cfg",
		"spi_cfg0",
		"spi_cfg1",
		"spi_cfg2",
		"spi_data_64",
		"spi_buf_addr",
		"spi_statu_2",
		"spi_err_status",
		"mem_data_addr",
		"mem_parity_addr",
		"spi_col_addr",
		"spi_bch",
		"spi_intr_msk",
		"spi_intr_sts",
		"spi_page_size",
	};

	for (i = 0; i < ARRAY_SIZE(reg_name); i++, p++)
		SPINAND_LOGI("%s = 0x%08X\n", reg_name[i], readl(p));
}

static int get_iomode_cfg(u32 io_mode, u32 d1oen)
{
	int cfg = -1;

	if (io_mode == SPINAND_1BIT_MODE) {
		cfg = SPINAND_CMD_BITMODE(1)
			| SPINAND_CMD_DQ(1)
			| SPINAND_ADDR_BITMODE(1)
			| SPINAND_ADDR_DQ(1)
			| SPINAND_DATA_BITMODE(1)
			| SPINAND_DATAOUT_DQ(1)
			| SPINAND_DATAIN_DQ(2);
	} else if (io_mode == SPINAND_2BIT_MODE) {
		cfg = SPINAND_CMD_BITMODE(1)
			| SPINAND_CMD_DQ(1)
			| SPINAND_ADDR_BITMODE(1)
			| SPINAND_ADDR_DQ(1)
			| SPINAND_DATA_BITMODE(2);
	} else if (io_mode == SPINAND_4BIT_MODE) {
		cfg = SPINAND_CMD_BITMODE(1)
			| SPINAND_CMD_DQ(1)
			| SPINAND_ADDR_BITMODE(1)
			| SPINAND_ADDR_DQ(d1oen? 2:1)		// 1->2. Solve D1 delay issue for write
			| SPINAND_DATA_BITMODE(3);
	} else if (io_mode == SPINAND_DUAL_MODE) {
		cfg = SPINAND_CMD_BITMODE(1)
			| SPINAND_ADDR_BITMODE(2)
			| SPINAND_DATA_BITMODE(2);
	} else if (io_mode == SPINAND_QUAD_MODE) {
		cfg = SPINAND_CMD_BITMODE(1)
			| SPINAND_ADDR_BITMODE(3)
			| SPINAND_DATA_BITMODE(3);
	}

	return cfg;
}

static int get_iomode_readcmd(u32 io_mode)
{
	int cmd = -1;

	if (io_mode == SPINAND_1BIT_MODE)
		cmd = SPINAND_CMD_PAGEREAD;
	else if (io_mode == SPINAND_2BIT_MODE)
		cmd = SPINAND_CMD_PAGEREAD_X2;
	else if (io_mode == SPINAND_4BIT_MODE)
		cmd = SPINAND_CMD_PAGEREAD_X4;
	else if (io_mode == SPINAND_DUAL_MODE)
		cmd = SPINAND_CMD_PAGEREAD_DUAL;
	else if (io_mode == SPINAND_QUAD_MODE)
		cmd = SPINAND_CMD_PAGEREAD_QUAD;

	return cmd;
}

static int get_iomode_writecmd(u32 io_mode)
{
	int cmd = -1;

	if (io_mode == SPINAND_1BIT_MODE)
		cmd = SPINAND_CMD_PROGLOAD;
	else if (io_mode == SPINAND_4BIT_MODE)
		cmd = SPINAND_CMD_PROGLOAD_X4;

	return cmd;
}

static int wait_spi_idle(struct sp_spinand_info *info)
{
	struct sp_spinand_regs *regs = info->regs;
	unsigned long timeout;
	int ret = -1;

	timeout = jiffies + msecs_to_jiffies(CONFIG_SPINAND_TIMEOUT);
	do {
		if (!(readl(&regs->spi_ctrl) & SPINAND_BUSY_MASK)) {
			ret = 0;
			break;
		}
	} while (time_before(jiffies, timeout));

	if (ret < 0) {
		SPINAND_LOGE("%s timeout\n", __func__);
		//dump_spi_regs(info);
	}

	return ret;
}

int spi_nand_trigger_and_wait_dma(struct sp_spinand_info *info)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value;
	int ret;

	value = ~SPINAND_DMA_DONE_MASK;
	writel(value, &regs->spi_intr_msk);

	value = readl(&regs->spi_intr_sts);
	writel(value, &regs->spi_intr_sts);

	info->busy = 1;

	value = readl(&regs->spi_auto_cfg);
	value |= SPINAND_DMA_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	ret = wait_event_timeout(info->wq, !info->busy, HZ/10);
	if (!ret) {
		if (info->busy) {
			SPINAND_LOGE("wait dma done timeout!\n");
			//dump_spi_regs(info);
			ret = -ETIME;
		}
	} else {
		ret = 0;
	}

	/* disable all interrupt */
	writel(0xff, &regs->spi_intr_msk);

	return ret;
}

int spi_nand_trigger_and_wait_pio(struct sp_spinand_info *info)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value;
	int ret;

	value = ~SPINAND_PIO_DONE_MASK;
	writel(value, &regs->spi_intr_msk);

	value = readl(&regs->spi_intr_sts);
	writel(value, &regs->spi_intr_sts);

	info->busy = 1;

	value = readl(&regs->spi_auto_cfg)
		| SPINAND_USR_CMD_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	ret = wait_event_timeout(info->wq, !info->busy, HZ/10);
	if (!ret) {
		if (info->busy) {
			SPINAND_LOGE("wait pio done timeout!\n");
			//dump_spi_regs(info);
			ret = -ETIME;
		}
	} else {
		ret = 0;
	}

	/* disable all interrupt */
	writel(0xff, &regs->spi_intr_msk);

	return ret;
}

int spi_nand_getfeatures(struct sp_spinand_info *info, u32 addr)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value = 0;

	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_USR_CMD(SPINAND_CMD_GETFEATURES)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_DATASZ(1)
		| SPINAND_READ_MODE
		| SPINAND_USRCMD_ADDRSZ(1);
	writel(value, &regs->spi_ctrl);

	writel(addr, &regs->spi_page_addr);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE;
	writel(value, &regs->spi_cfg[0]);

	value = SPINAND_CMD_BITMODE(1)
		| SPINAND_CMD_DQ(1)
		| SPINAND_ADDR_BITMODE(1)
		| SPINAND_ADDR_DQ(1)
		| SPINAND_DATA_BITMODE(1)
		| SPINAND_DATAIN_DQ(2);
	writel(value, &regs->spi_cfg[1]);

	value = SPINAND_USR_CMD_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	wait_spi_idle(info);

	return (readl(&regs->spi_data) & 0xFF);
}

void spi_nand_setfeatures(struct sp_spinand_info *info, u32 addr, u32 data)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value = 0;

	value = SPINAND_SEL_CHIP_A
		| SPINAND_AUTOWEL_EN
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_USR_CMD(SPINAND_CMD_SETFEATURES)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_DATASZ(1)
		| SPINAND_WRITE_MODE
		| SPINAND_USRCMD_ADDRSZ(1);
	writel(value, &regs->spi_ctrl);

	writel(addr, &regs->spi_page_addr);

	writel(data, &regs->spi_data);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE;
	writel(value, &regs->spi_cfg[0]);

	value = SPINAND_CMD_BITMODE(1)
		| SPINAND_CMD_DQ(1)
		| SPINAND_ADDR_BITMODE(1)
		| SPINAND_ADDR_DQ(1)
		| SPINAND_DATA_BITMODE(1)
		| SPINAND_DATAOUT_DQ(1);
	writel(value, &regs->spi_cfg[1]);

	value = SPINAND_USR_CMD_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	wait_spi_idle(info);
}

int spi_nand_reset(struct sp_spinand_info *info)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value = 0;

	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(7)
		| SPINAND_USR_CMD(SPINAND_CMD_RESET)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_DATASZ(0)
		| SPINAND_WRITE_MODE
		| SPINAND_USRCMD_ADDRSZ(0);
	writel(value, &regs->spi_ctrl);

	value = SPINAND_CS_DISACTIVE_CYC(CONFIG_SPINAND_CS_DISACTIVE_CYC)
		| SPINAND_READ_TIMING(info->rts);
	writel(value, &regs->spi_timing);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE;
	writel(value, &regs->spi_cfg[0]);

	value = SPINAND_CMD_BITMODE(1)
		| SPINAND_CMD_DQ(1)
		| SPINAND_ADDR_BITMODE(0)
		| SPINAND_ADDR_DQ(0)
		| SPINAND_DATA_BITMODE(0)
		| SPINAND_DATAIN_DQ(0)
		| SPINAND_DATAOUT_DQ(0);
	writel(value, &regs->spi_cfg[1]);

	/* disable all interrupt */
	writel(0xff, &regs->spi_intr_msk);

	value = SPINAND_USR_CMD_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	/* some devices can't support any cmd directly after reset cmd */
	/* so need this delay. */
	mdelay(1);

	wait_spi_idle(info);

	value = SPINAND_CHECK_OIP_EN
		| SPINAND_USR_CMD_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	return wait_spi_idle(info);
}

void spi_nand_readid(struct sp_spinand_info *info, u32 addr, u8 *data)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value = 0;

	/*read 3 byte cycle same to 8388 */
	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_USR_CMD(SPINAND_CMD_READID)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_DATASZ(3)
		| SPINAND_READ_MODE
		| SPINAND_USRCMD_ADDRSZ(1);
	writel(value, &regs->spi_ctrl);

	writel(addr, &regs->spi_page_addr);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE;
	writel(value, &regs->spi_cfg[0]);

	value = SPINAND_CMD_BITMODE(1)
		| SPINAND_CMD_DQ(1)
		| SPINAND_ADDR_BITMODE(1)
		| SPINAND_ADDR_DQ(1)
		| SPINAND_DATA_BITMODE(1)
		| SPINAND_DATAIN_DQ(2)
		| SPINAND_DATAOUT_DQ(0);
	writel(value, &regs->spi_cfg[1]);

	value = SPINAND_USR_CMD_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	wait_spi_idle(info);

	value = readl(&regs->spi_data);

	value &= 0xffff;

	*(u32 *)data = value;
}

static int spi_nand_select_die(struct sp_spinand_info *info, u32 id)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value;

	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_USR_CMD(SPINAND_CMD_DIE_SELECT)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_DATASZ(0)
		| SPINAND_WRITE_MODE
		| SPINAND_USRCMD_ADDRSZ(1);
	writel(value, &regs->spi_ctrl);

	writel(id, &regs->spi_page_addr);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE;
	writel(value, &regs->spi_cfg[0]);

	value = SPINAND_CMD_BITMODE(1)
		| SPINAND_CMD_DQ(1)
		| SPINAND_ADDR_BITMODE(1)
		| SPINAND_ADDR_DQ(1);
	writel(value, &regs->spi_cfg[1]);

	value = SPINAND_USR_CMD_TRIGGER;
	writel(value, &regs->spi_auto_cfg);

	return wait_spi_idle(info);
}

static int spi_nand_wait_dev_idle(struct sp_spinand_info *info)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(CONFIG_SPINAND_TIMEOUT);
	int status;

	do {
		status = spi_nand_getfeatures(info, DEVICE_STATUS_ADDR);
		if ((status & DEVICE_STATUS_OIP_MSK) == 0)
			break;
		cond_resched();
	} while (time_before(jiffies, timeout));
	return status;
}

int spi_nand_blkerase(struct sp_spinand_info *info, u32 row)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 value = 0;
	int ret = 0;

	value = SPINAND_SEL_CHIP_A
		| SPINAND_AUTOWEL_EN
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_USR_CMD(SPINAND_CMD_BLKERASE)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_DATASZ(0)
		| SPINAND_WRITE_MODE
		| SPINAND_USRCMD_ADDRSZ(3);
	writel(value, &regs->spi_ctrl);

	writel(row, &regs->spi_page_addr);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE;
	writel(value, &regs->spi_cfg[0]);

	value = SPINAND_CMD_BITMODE(1)
		| SPINAND_CMD_DQ(1)
		| SPINAND_ADDR_BITMODE(1)
		| SPINAND_ADDR_DQ(1)
		| SPINAND_DATA_BITMODE(1)
		| SPINAND_DATAIN_DQ(2)
		| SPINAND_DATAOUT_DQ(1);
	writel(value, &regs->spi_cfg[1]);

	value = SPINAND_AUTO_RDSR_EN;
	writel(value, &regs->spi_auto_cfg);

	ret = spi_nand_trigger_and_wait_pio(info);

	if (!ret) {
		value = readl(&regs->spi_status);
		ret = (value&ERASE_STATUS) ? (-EIO) : 0;
	}
	return ret;
}

int spi_nand_read_by_dma(struct sp_spinand_info *info, u32 io_mode,
				u32 row, u32 col, u8 *buf, u32 size)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 plane_sel_mode = info->plane_sel_mode;
	u32 page_size = info->mtd->writesize;
	int cmd = get_iomode_readcmd(io_mode);
	int cfg = get_iomode_cfg(io_mode, 0);
	u32 value = 0;

	if (cmd < 0 || cfg < 0)
		return -1;

	while (readl(&regs->spi_auto_cfg) & SPINAND_DMA_OWNER_MASK) {
		// Do nothing
	};

	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_ADDRSZ(2);
	writel(value, &regs->spi_ctrl);

	writel(row, &regs->spi_page_addr);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE
		| SPINAND_DATA_LEN(size);
	writel(value, &regs->spi_cfg[0]);

	value = cfg | SPINAND_DUMMY_CYCLES(8);
	writel(value, &regs->spi_cfg[1]);

	if ((plane_sel_mode & 0x1) != 0) {
		u32 pagemark = (plane_sel_mode>>2)&0xfff;
		u32 colmark = (plane_sel_mode>>16)&0xffff;

		col |= ((row & pagemark) != 0) ? colmark : 0;
		page_size += ((row & pagemark) != 0) ? colmark : 0;
	}
	writel(col, &regs->spi_col_addr);

	value = SPINAND_SPARE_SIZE(info->mtd->oobsize)
		| SPINAND_PAGE_SIZE((page_size >> 10) - 1);
	writel(value, &regs->spi_page_size);

	writel((u32)((ulong)buf), &regs->mem_data_addr);

	value = SPINAND_USR_READCACHE_CMD(cmd)
		| SPINAND_USR_READCACHE_EN;
	writel(value, &regs->spi_auto_cfg);

	return spi_nand_trigger_and_wait_dma(info);
}

int spi_nand_write_by_dma(struct sp_spinand_info *info, u32 io_mode,
				u32 row, u32 col, u8 *buf, u32 size)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 plane_sel_mode = info->plane_sel_mode;
	u32 page_size = info->mtd->writesize;
	int cmd = get_iomode_writecmd(io_mode);
	int cfg = get_iomode_cfg(io_mode, 1);
	u32 value = 0;
	int ret;

	if (cmd < 0 || cfg < 0)
		return -1;

	while (readl(&regs->spi_auto_cfg) & SPINAND_DMA_OWNER_MASK) {
		// Do nothing
	};

	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_CTRL_EN
		| SPINAND_WRITE_MODE
		| SPINAND_USRCMD_ADDRSZ(2);
	writel(value, &regs->spi_ctrl);

	writel(row, &regs->spi_page_addr);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE
		| SPINAND_DATA_LEN(size);
	writel(value, &regs->spi_cfg[0]);

	writel(cfg, &regs->spi_cfg[1]);

	if ((plane_sel_mode & 0x1) != 0) {
		u32 pagemark = (plane_sel_mode>>2)&0xfff;
		u32 colmark = (plane_sel_mode>>16)&0xffff;

		col |= (row & pagemark) ? colmark : 0;
		page_size += (row & pagemark) ? colmark : 0;
	}
	writel(col, &regs->spi_col_addr);

	value = SPINAND_SPARE_SIZE(info->mtd->oobsize)
		| SPINAND_PAGE_SIZE((page_size >> 10) - 1);
	writel(value, &regs->spi_page_size);

	writel((u32)((ulong)buf), &regs->mem_data_addr);

	value = SPINAND_USR_PRGMLOAD_CMD(cmd)
		| SPINAND_USR_PRGMLOAD_EN
		| SPINAND_AUTOWEL_BF_PRGMLOAD;
	writel(value, &regs->spi_auto_cfg);

	ret = spi_nand_trigger_and_wait_dma(info);

	if (!ret) {
		value = readl(&regs->spi_status);
		ret = (value&0x08) ? (-EIO) : 0;
	}

	return ret;
}

int spi_nand_pageread_autobch(struct sp_spinand_info *info, u32 io_mode,
				u32 row, u8 *buf)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 plane_sel_mode = info->plane_sel_mode;
	u32 page_size = info->mtd->writesize;
	int cmd = get_iomode_readcmd(io_mode);
	int cfg = get_iomode_cfg(io_mode, 0);
	u32 value = 0;
	int ret;

	if (cmd < 0 || cfg < 0)
		return -1;

	while (readl(&regs->spi_auto_cfg) & SPINAND_DMA_OWNER_MASK) {
		// Do nothing
	};

	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_CTRL_EN
		| SPINAND_USRCMD_ADDRSZ(2);
	writel(value, &regs->spi_ctrl);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE
		| SPINAND_DATA_LEN(info->mtd->writesize);
	writel(value, &regs->spi_cfg[0]);

	value = cfg | SPINAND_DUMMY_CYCLES(8);
	writel(value, &regs->spi_cfg[1]);

	writel(row, &regs->spi_page_addr);

	value = 0;
	if ((plane_sel_mode & 0x1) != 0) {
		u32 pagemark = (plane_sel_mode>>2)&0xfff;
		u32 colmark = (plane_sel_mode>>16)&0xffff;

		value |= ((row & pagemark) != 0) ? colmark : 0;
		page_size += ((row & pagemark) != 0) ? colmark : 0;
	}
	writel(value, &regs->spi_col_addr);

	writel(page_size, &regs->device_parity_addr);

	value = SPINAND_SPARE_SIZE(info->mtd->oobsize)
		| SPINAND_PAGE_SIZE((page_size >> 10) - 1);
	writel(value, &regs->spi_page_size);

	writel((u32)((ulong)buf), &regs->mem_data_addr);
	writel((u32)((ulong)buf+info->mtd->writesize), &regs->mem_parity_addr);

	value = SPINAND_BCH_DATA_LEN(info->parity_sector_size)
		| SPINAND_BCH_DECSRC(info->bch_dec_src)
		| SPINAND_BCH_BLOCKS(info->nand.ecc.steps - 1)
		| SPINAND_BCH_AUTO_EN;
	value |= (info->parity_sector_size & 31) ?
			SPINAND_BCH_ALIGN_16B : SPINAND_BCH_ALIGN_32B;
	value |= (info->nand.ecc.size == 1024) ?
			SPINAND_BCH_1K_MODE : SPINAND_BCH_512B_MODE;
	writel(value, &regs->spi_bch);

	value = SPINAND_USR_READCACHE_CMD(cmd)
		| SPINAND_USR_READCACHE_EN;
	writel(value, &regs->spi_auto_cfg);

	sp_autobch_config(info->mtd, buf, buf+info->mtd->writesize, 0, info->bch_dec_src);

	ret = spi_nand_trigger_and_wait_dma(info);

	if (!ret)
		ret = sp_autobch_result(info->mtd);

	writel(0, &regs->spi_bch); /* close auto bch */

	return ret;
}

int spi_nand_pagewrite_autobch(struct sp_spinand_info *info, u32 io_mode,
				u32 row, u8 *buf)
{
	struct sp_spinand_regs *regs = info->regs;
	u32 plane_sel_mode = info->plane_sel_mode;
	u32 page_size = info->mtd->writesize;
	int cmd = get_iomode_writecmd(io_mode);
	int cfg = get_iomode_cfg(io_mode, 1);
	u32 value = 0;
	int ret = 0;

	if (cmd < 0 || cfg < 0)
		return -1;

	while (readl(&regs->spi_auto_cfg) & SPINAND_DMA_OWNER_MASK) {
		// Do nothing
	};

	value = SPINAND_SEL_CHIP_A
		| SPINAND_SCK_DIV(info->spi_clk_div)
		| SPINAND_USR_CMD(SPINAND_CMD_PROGEXEC)
		| SPINAND_CTRL_EN
		| SPINAND_WRITE_MODE
		| SPINAND_USRCMD_ADDRSZ(2);
	writel(value, &regs->spi_ctrl);

	value = SPINAND_LITTLE_ENDIAN
		| SPINAND_TRS_MODE
		| SPINAND_DATA_LEN(info->mtd->writesize);
	writel(value, &regs->spi_cfg[0]);

	value = cfg;
	writel(value, &regs->spi_cfg[1]);

	writel(row, &regs->spi_page_addr);

	value = 0;
	if ((plane_sel_mode & 0x1) != 0) {
		u32 pagemark = (plane_sel_mode>>2)&0xfff;
		u32 colmark = (plane_sel_mode>>16)&0xffff;

		value |= ((row & pagemark) != 0) ? colmark : 0;
		page_size += ((row & pagemark) != 0) ? colmark : 0;
	}
	writel(value, &regs->spi_col_addr);

	writel(page_size, &regs->device_parity_addr);

	value = SPINAND_SPARE_SIZE(info->mtd->oobsize)
		| SPINAND_PAGE_SIZE((page_size >> 10) - 1);
	writel(value, &regs->spi_page_size);

	writel((u32)((ulong)buf), &regs->mem_data_addr);
	writel((u32)((ulong)buf+info->mtd->writesize), &regs->mem_parity_addr);

	value = SPINAND_BCH_DATA_LEN(info->parity_sector_size)
		| SPINAND_BCH_DECSRC(info->bch_dec_src)
		| SPINAND_BCH_BLOCKS(info->nand.ecc.steps - 1)
		| SPINAND_BCH_AUTO_EN;
	value |= (info->parity_sector_size & 31) ?
			SPINAND_BCH_ALIGN_16B : SPINAND_BCH_ALIGN_32B;
	value |= (info->nand.ecc.size == 1024) ?
			SPINAND_BCH_1K_MODE : SPINAND_BCH_512B_MODE;

	writel(value, &regs->spi_bch);

	value = SPINAND_USR_PRGMLOAD_CMD(cmd)
		| SPINAND_USR_PRGMLOAD_EN
		| SPINAND_AUTOWEL_BF_PRGMLOAD;
	writel(value, &regs->spi_auto_cfg);

	sp_autobch_config(info->mtd, buf, buf+info->mtd->writesize, 1, info->bch_dec_src);

	ret = spi_nand_trigger_and_wait_dma(info);

	writel(0, &regs->spi_bch);

	if (!ret) {
		value = spi_nand_getfeatures(info, DEVICE_STATUS_ADDR);
		if ((value & DEVICE_STATUS_OIP_MSK) != 0) {
			spi_nand_wait_dev_idle(info);
			ret = -1;
		} else {
			ret = (value & DEVICE_STATUS_PFAIL_MSK) ? (-1) : 0;
		}
	}

	if (!ret)
		ret = sp_autobch_result(info->mtd);

	return ret;
}

static irqreturn_t spi_nand_irq(int irq, void *dev)
{
	struct sp_spinand_info *info = dev;
	struct sp_spinand_regs *regs = info->regs;
	u32 value;

	/* clear intrrupt flag */
	value = readl(&regs->spi_intr_sts);
	writel(value, &regs->spi_intr_sts);

	info->busy = 0;
	wake_up(&info->wq);

	return IRQ_HANDLED;
}

static int sp_spinand_read_raw(struct sp_spinand_info *info,
				u32 row, u32 col, u32 size)
{
	int ret = -1;
	u8 io = info->read_bitmode;
	u8 trsmode = info->raw_trs_mode;
	//u8 *va = info->buff.virt + info->buff.idx;
	u8 *pa = (u8 *)info->buff.phys + info->buff.idx;

	if (trsmode == SPINAND_TRS_DMA)
		ret = spi_nand_read_by_dma(info, io, row, col, pa, size);
	//else if (trsmode == SPINAND_TRS_PIO_AUTO)
		//ret = spi_nand_read_by_pio_auto(info, io, row, col, va, size);
	//else if (trsmode == SPINAND_TRS_PIO)
		//ret = spi_nand_read_by_pio(info, io, row, col, va, size);

	return ret;
}

static int sp_spinand_write_raw(struct sp_spinand_info *info,
				u32 row, u32 col, u32 size)
{
	int ret = -1;
	u8 io = info->write_bitmode;
	u8 trsmode = info->raw_trs_mode;
	//u8 *va = info->buff.virt + info->buff.idx;
	u8 *pa = (u8 *)info->buff.phys + info->buff.idx;

	if (trsmode == SPINAND_TRS_DMA)
		ret = spi_nand_write_by_dma(info, io, row, col, pa, size);
	//else if (trsmode == SPINAND_TRS_PIO_AUTO)
		//ret = spi_nand_write_by_pio_auto(info, io, row, col, va, size);
	//else if (trsmode == SPINAND_TRS_PIO)
		//ret = spi_nand_write_by_pio(info, io, row, col, va, size);

	return ret;
}

/*
 * nand_select_chip - control CE line
 * @mtd:    MTD device structure
 * @chipnr: chipnumber to select, -1 for deselect
 */
static void sp_spinand_select_chip(struct nand_chip *chip, int chipnr)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;

	if ((info->chip_num > 0) && (info->cur_chip != chipnr) && (chipnr >= 0)) {
		info->cur_chip = chipnr;
		spi_nand_select_die(info, chipnr);
	}
}

static int sp_spinand_waitrdy(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;
	int ret;

	ret = wait_spi_idle(info);

	return ret;
}

static void sp_spinand_read_buf(struct nand_chip *chip, u8 *buf, int len)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;
	u32 value;
	u32 size;

	if (info->buff.idx == 0) {
		switch (info->cmd) {
		case NAND_CMD_READOOB:
		case NAND_CMD_READ0: {
			size = info->mtd->writesize + info->mtd->oobsize - info->col;
			sp_spinand_read_raw(info, info->row, info->col, size);
			break;
		}
		case NAND_CMD_READID:
			spi_nand_readid(info, info->col, info->buff.virt);
			break;
		case NAND_CMD_STATUS:
			value = spi_nand_getfeatures(info, DEVICE_STATUS_ADDR);
			*(u32 *)info->buff.virt  = (value&0x0c) ? 0x01 : 0x00;
			*(u32 *)info->buff.virt |= (value&0x01) ? 0x00 : 0x40;
			if (!(info->dev_protection & PROTECT_STATUS))
				*(u32 *)info->buff.virt |= 0X80;
			memcpy(buf, info->buff.virt + info->buff.idx, len);
			return;
		default:
			break;
		}
	}

	memcpy(buf, info->buff.virt + info->buff.idx, len);
	info->buff.idx += len;
}

static u8 sp_spinand_read_byte(struct nand_chip *chip)
{
	u8 ret = 0;

	sp_spinand_read_buf(chip, &ret, 1);
	return ret;
}

static void sp_spinand_write_buf(struct nand_chip *chip, const u8 *buf, int len)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;

	memcpy(info->buff.virt + info->buff.idx, buf, len);
	info->buff.idx += len;
}

static int sp_spinand_op_parser(struct nand_chip *chip,
				const struct nand_operation *op)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;
	const struct nand_op_instr *instr;
	unsigned int i, j;
	u32 cmd_idx;
	u32 col_addr, col_size, row_addr;

	/* Initialize the command array */
	cmd_idx = 0;
	info->nnandcmd = 0;
	memset(info->nandcmd, 0, sizeof(info->nandcmd));

	for(i = 0; i < op->ninstrs; i++) {
		instr = &op->instrs[i];
		switch (instr->type) {
		case NAND_OP_CMD_INSTR:
			if (instr->ctx.cmd.opcode == NAND_CMD_READSTART) {
				/* Skip the read-start and next wait-for-ready instructions. */
				i++;
				break;
			}

			cmd_idx = info->nnandcmd;
			info->nandcmd[cmd_idx].opcode = instr->ctx.cmd.opcode;
			info->nnandcmd++;
			break;
		case NAND_OP_ADDR_INSTR:
			col_addr = 0;
			row_addr = 0;
			if (info->nandcmd[cmd_idx].opcode == NAND_CMD_ERASE1) {
				col_size = 0;
			} else if ((info->nandcmd[cmd_idx].opcode == NAND_CMD_READ0) ||
					   (info->nandcmd[cmd_idx].opcode == NAND_CMD_SEQIN)) {
				col_size = 2;
			} else {
				col_size = 1;
			}

			for (j = 0; j < instr->ctx.addr.naddrs; j++) {
				if (j < col_size)
					col_addr += instr->ctx.addr.addrs[j]<<(8*j);
				else
					row_addr += instr->ctx.addr.addrs[j]<<(8*(j-col_size));
			}
			info->nandcmd[cmd_idx].col = col_addr;
			info->nandcmd[cmd_idx].row = row_addr;
			break;
		case NAND_OP_DATA_IN_INSTR:
			memcpy(&info->nandcmd[cmd_idx].data, &instr->ctx.data,
					sizeof(struct nand_op_data_instr));
			break;
		case NAND_OP_DATA_OUT_INSTR:
			cmd_idx = info->nnandcmd;
			info->nandcmd[cmd_idx].opcode = 0x101;	/* Do the write-buf instruction */
			info->nnandcmd++;

			memcpy(&info->nandcmd[cmd_idx].data, &instr->ctx.data,
					sizeof(struct nand_op_data_instr));
			break;
		case NAND_OP_WAITRDY_INSTR:
			cmd_idx = info->nnandcmd;
			info->nandcmd[cmd_idx].opcode = 0x100;	/* Do the wait-for-ready instruction */
			info->nnandcmd++;
			break;
		}
	}

	return 0;
}

static int sp_spinand_exec_instrs(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;
	unsigned int i, j;
	int ret = 0;

	/* Reset previous command */
	info->pre_cmd = 0;

	for (i = 0; i < info->nnandcmd; i++) {
		info->cmd = info->nandcmd[i].opcode;

		switch(info->nandcmd[i].opcode) {
		case NAND_CMD_READ0:
		case NAND_CMD_READ1:
		case NAND_CMD_READOOB:
			info->row = info->nandcmd[i].row;
			info->col = info->nandcmd[i].col;
			info->buff.idx = 0;
			sp_spinand_read_buf(chip, info->nandcmd[i].data.buf.in,
					info->nandcmd[i].data.len);
			break;
		case NAND_CMD_PAGEPROG:
			if (info->buff.idx) {
				u32 size = info->buff.idx;
			
				info->buff.idx = 0;
				sp_spinand_write_raw(info, info->row, info->col, size);
			}
			break;
		case NAND_CMD_ERASE1:
			info->row = info->nandcmd[i].row;
			info->col = 0;
			info->row &= ~(info->mtd->erasesize / info->mtd->writesize - 1);
			ret = spi_nand_blkerase(info, info->row);
			break;
		case NAND_CMD_STATUS:
		case NAND_CMD_READID:
			info->row = info->nandcmd[i].row;
			info->col = info->nandcmd[i].col;
			info->buff.idx = 0;
			for (j = 0; j < info->nandcmd[i].data.len; j++) {
				u8 *id = info->nandcmd[i].data.buf.in;

				id[j] = sp_spinand_read_byte(chip);
			}
			break;
		case NAND_CMD_SEQIN:
			info->buff.idx = 0;
			info->col = info->nandcmd[i].col;
			info->row = info->nandcmd[i].row;
			break;
		case NAND_CMD_ERASE2:
			break;
		case NAND_CMD_RESET:			
			ret = spi_nand_reset(info);
			break;
		case 0x100: /* Do the wait-for-ready instruction */
			ret = sp_spinand_waitrdy(chip);
			break;
		case 0x101: /* Do the write-buf instruction */
			sp_spinand_write_buf(chip, info->nandcmd[i].data.buf.out,
					info->nandcmd[i].data.len);
			break;
		default:
			SPINAND_LOGE("Unsupported command!\n");
			break;
		}

		/* Store previous command */
		info->pre_cmd = info->cmd;
	}

	return ret;
}

static int sp_spinand_exec_op(struct nand_chip *chip,
				const struct nand_operation *op,
				bool check_only)
{
	if (check_only)
		return 0;

	sp_spinand_select_chip(chip, op->cs);
	sp_spinand_op_parser(chip, op);
	return sp_spinand_exec_instrs(chip);
}

static int sp_spinand_read_page(struct nand_chip *chip, u8 *buf,
				int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;
	void *data_va = info->buff.virt;
	void *oob_va = info->buff.virt + mtd->writesize;
	dma_addr_t data_pa = info->buff.phys;
	dma_addr_t oob_pa = info->buff.phys + mtd->writesize;
	int ret;

	sp_spinand_select_chip(chip, chip->cur_cs);

	info->row = page;
	info->buff.idx = 0;
	if (info->trs_mode == SPINAND_TRS_DMA_AUTOBCH) {
		ret = spi_nand_pageread_autobch(info,
			info->read_bitmode, page, (u8 *)data_pa);
	} else {
		ret = sp_spinand_read_raw(info,
			info->row, 0, mtd->writesize+mtd->oobsize);
		if (ret == 0)
			ret = sp_bch_decode(mtd, data_pa, oob_pa);
	}

	if (ret < 0)
		return ret;

	memcpy(buf, data_va, mtd->writesize);
	if (oob_required)
		memcpy(chip->oob_poi, oob_va, mtd->oobsize);

	return 0;
}

static int sp_spinand_write_page(struct nand_chip *chip, const u8 *buf,
				int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;
	u8 *data_va = info->buff.virt;
	u8 *oob_va = info->buff.virt + mtd->writesize;
	dma_addr_t data_pa = info->buff.phys;
	dma_addr_t oob_pa = info->buff.phys + mtd->writesize;
	int ret;

	sp_spinand_select_chip(chip, chip->cur_cs);

	info->row = page;
	info->buff.idx = 0;
	memcpy(data_va, buf, mtd->writesize);
	memcpy(oob_va, chip->oob_poi, mtd->oobsize);
	if (info->trs_mode == SPINAND_TRS_DMA_AUTOBCH) {
		ret = spi_nand_pagewrite_autobch(info,
			info->write_bitmode, page, (u8 *)data_pa);
	} else {
		sp_bch_encode(mtd, data_pa, oob_pa);
		ret = sp_spinand_write_raw(info,
			info->row, 0, mtd->writesize+mtd->oobsize);
	}

	return ret;
}

static int sp_spinand_feature_cfg(struct sp_spinand_info *info)
{
	int i;
	u32 value;

	for (i = 0; i < nanddev_ntargets(&info->nand.base); i++) {
		sp_spinand_select_chip(&info->nand, i);
		if (info->drv_options & SPINAND_OPT_ECCEN_IN_F90_4) {
			value = spi_nand_getfeatures(info, 0x90);
			value &= ~0x01;
			spi_nand_setfeatures(info, 0x90, value);
		}

		value = spi_nand_getfeatures(info, DEVICE_FEATURE_ADDR);
		value &= ~0x10; 	/* disable internal ECC */
		if (info->drv_options & SPINAND_OPT_HAS_BUF_BIT)
			value |= 0x08;	/* use buffer read mode */
		if (info->drv_options & SPINAND_OPT_HAS_CONTI_RD)
			value &= ~0x01; /* disable continuous read mode */
		if (info->drv_options & SPINAND_OPT_HAS_QE_BIT)
			value |= 0x01;	/* enable quad io */
		spi_nand_setfeatures(info, DEVICE_FEATURE_ADDR, value);

		/* close write protection */
		spi_nand_setfeatures(info, DEVICE_PROTECTION_ADDR, 0x0);
		info->dev_protection = spi_nand_getfeatures(info, DEVICE_PROTECTION_ADDR);
	}

	return 0;
}

static int sunplus_parse_cfg_partitions(struct mtd_info *master,
					const struct mtd_partition **pparts,
					struct mtd_part_parser_data *data)
{
	int ret = 0;
	struct mtd_partition *sunplus_parts;

	sunplus_parts = kzalloc(sizeof(*sunplus_parts) * 2, GFP_KERNEL);
	if (!sunplus_parts)
		return -ENOMEM;
	sunplus_parts[0].name = "Linux";
	sunplus_parts[0].offset = 0;
	sunplus_parts[0].size = master->size>>1;
	sunplus_parts[0].mask_flags = MTD_NO_ERASE;

	sunplus_parts[1].name = "User";
	sunplus_parts[1].offset = sunplus_parts[0].offset+sunplus_parts[0].size;
	sunplus_parts[1].size = master->size>>1;
	sunplus_parts[1].mask_flags = MTD_WRITEABLE;

	*pparts = sunplus_parts;
	ret = 2;
	return ret;
};

static struct mtd_part_parser sunplus_nand_parser = {
	.parse_fn = sunplus_parse_cfg_partitions,
	.name = "sunplus_part",
};

static int sp_spinand_attach_chip(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct sp_spinand_info *info = (struct sp_spinand_info *)mtd->priv;

	/* Configure the feature registers of SPI-NAND devices */
	sp_spinand_feature_cfg(info);

	if (sp_bch_init(info->mtd, &info->parity_sector_size) < 0) {
		SPINAND_LOGE("sp_bch_init fail\n");
		return  -ENXIO;
	}

	return 0;
}

static const struct nand_controller_ops sp_spinand_controller_ops = {
	.attach_chip = sp_spinand_attach_chip,
	.exec_op = sp_spinand_exec_op,
};

static int sp_spinand_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;
	u32 value;
	u32 id;
	u32 rts;
	u32 clk_freq, max_freq;
	u8 mfr_id, dev_id;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct resource *res_mem;
	struct clk *clk;
	struct sp_spinand_info *info;
	static const char * const part_types[] = {
	#ifdef CONFIG_MTD_CMDLINE_PARTS
		"cmdlinepart",
	#else
		"sunplus_part",
	#endif
		NULL,
	};

	SPINAND_LOGI("%s in\n", __func__);

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		SPINAND_LOGE("Failed to allocate memory!\n");
		return -ENOMEM;
	}

	init_waitqueue_head(&info->wq);
	platform_set_drvdata(pdev, info);

	res_mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "nand");
	if (!res_mem) {
		SPINAND_LOGE("Failed to get memory resource!\n");
		ret = -ENXIO;
		goto err1;
	}

	info->regs = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(info->regs)) {
		SPINAND_LOGE("memory remap fail!\n");
		ret = PTR_ERR(info->regs);
		info->regs = NULL;
		goto err1;
	}

	info->irq = platform_get_irq_byname(pdev, "int_nand");
	if (info->irq < 0) {
		SPINAND_LOGE("failed to get irq resource\n");
		ret = info->irq;
		goto err1;
	}

	info->rstc = devm_reset_control_get(&pdev->dev, "rst_nand");
	if (IS_ERR(info->rstc)) {
		SPINAND_LOGE("Failed to get reset control\n");
		goto err1;
	}

	ret = reset_control_deassert(info->rstc);
	if (ret) {
		SPINAND_LOGE("Failed to deassert reset control\n");
		goto err1;
	}

	clk = devm_clk_get(&pdev->dev, "clk_nand");
	if (!IS_ERR(clk)) {
		ret = clk_prepare(clk);
		if (ret) {
			SPINAND_LOGE("clk_prepare fail\n");
			goto err1;
		}
		ret = clk_enable(clk);
		if (ret) {
			SPINAND_LOGE("clk_enable fail\n");
			clk_unprepare(clk);
			goto err1;
		}
		info->clk = clk;
	}

	clk_freq = clk_get_rate(clk);
	if (!clk_freq) {
		SPINAND_LOGE("Failed to get clock rate\n");
		ret = -ENXIO;
		goto err1;
	}
	SPINAND_LOGI("source clock frequency: %d Hz\n", clk_freq);

	info->buff.size = CONFIG_SPINAND_BUF_SZ;
	#ifdef CONFIG_SPINAND_USE_SRAM
	info->buff.phys = (dma_addr_t)CONFIG_SPINAND_SRAM_ADDR;
	info->buff.virt = ioremap(info->buff.phys, info->buff.size);
	#else
	info->buff.virt = dma_alloc_coherent(&pdev->dev, info->buff.size,
		&info->buff.phys, GFP_KERNEL);
	#endif

	if (!info->buff.virt) {
		SPINAND_LOGE("alloc memory(size=0x%x) fail\n", info->buff.size);
		ret = -ENOMEM;
		goto err1;
	}

	ret = devm_request_irq(dev, info->irq, spi_nand_irq,
					IRQF_SHARED, "sp_spinand", info);
	if (ret) {
		SPINAND_LOGE("failed to request IRQ: %d\n", ret);
		goto err1;
	}

	ret = of_property_read_u32(node, "spi-max-frequency", &max_freq);
	if (ret < 0) {
		max_freq = 50000000;    // Hz
		SPINAND_LOGW("default SPI max frequency: %d Hz\n", max_freq);
	}

	if (max_freq > 100000000) {
		clk_set_rate(info->clk, 614000000);

		clk_freq = clk_get_rate(clk);
		if (!clk_freq) {
			SPINAND_LOGE("Failed to get clock rate\n");
			ret = -ENXIO;
			goto err1;
		}
		SPINAND_LOGI("source clock frequency: %d Hz\n", clk_freq);
	}

    // Calculate SPI interface clock frequency
	// SCK_MODE : 1    2    3    4    5     6     7
	// Freq Div : 1/2  1/4  1/6  1/8  1/16  1/24  1/32
	for (i = 1; i < 8; i++) {
		if (i < 5)
			value = i * 2;
		else
			value = (i - 5) * 8 + 16;

		if ((clk_freq/value) <= max_freq)
			break;

		if (i == 7) {
			SPINAND_LOGW("touched min division setting: %d\n", i);
			break;
		}
	}
	info->spi_clk_div = i;
	SPINAND_LOGI("SPI clock frequency: %d Hz\n", (clk_freq/value));

	ret = of_property_read_u32(node, "read-timing-selection", &rts);
	if (ret < 0) {
		rts = CONFIG_SPINAND_READ_TIMING_SEL;
		SPINAND_LOGW("default read timing sel: %d\n", max_freq);
	}
	info->rts = (u8)rts;
	SPINAND_LOGI("read timing selection: %d\n", info->rts);

	if (spi_nand_reset(info) < 0) {
		SPINAND_LOGE("reset device fail\n");
		ret = -ENXIO;
		goto err1;
	}

	ret = sp_bch_probe(pdev);
	if (ret) {
		SPINAND_LOGE("failed to probe BCH: %d\n", ret);
		goto err1;
	}

	/* Initialize NAND controller structure */
	nand_controller_init(&info->controller);
	info->controller.ops = &sp_spinand_controller_ops;

	info->dev = dev;
	info->mtd = &info->nand.base.mtd;
	info->mtd->priv = info;
	info->mtd->name = NAND_DEVICE_NAME;//dev_name(dev);
	info->mtd->owner = THIS_MODULE;
	info->nand.options = NAND_NO_SUBPAGE_WRITE;
	info->nand.controller = &info->controller;
	info->nand.ecc.read_page = sp_spinand_read_page;
	info->nand.ecc.write_page = sp_spinand_write_page;
	info->nand.ecc.engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
	info->nand.bbt_options = NAND_BBT_USE_FLASH|NAND_BBT_NO_OOB;
	nand_set_flash_node(&info->nand, dev->of_node);

	spi_nand_readid(info, 0, (u8 *)&id);

	/* Lookup table to get the driver options */
	mfr_id = id & 0x000000ff;
	dev_id = (id & 0x0000ff00)>>8;

	for (i = 0; sp_spinand_opt[i].name != NULL; i++) {
		if ((sp_spinand_opt[i].mfr_id == mfr_id) && (sp_spinand_opt[i].dev_id == dev_id)) {
			info->drv_options = sp_spinand_opt[i].options;
			break;
		}
	}

	/* Lookup table to get the NAND information */
	for (i = 0; sp_spinand_ids[i].name != NULL; i++) {
		if ((sp_spinand_ids[i].mfr_id == mfr_id) && (sp_spinand_ids[i].dev_id == dev_id)) {
			info->type = &sp_spinand_ids[i];
			break;
		}
	}

	if (info->drv_options & SPINAND_OPT_HAS_TWO_PLANE)
		info->plane_sel_mode = (0x1000<<16) | (0x40<<2) | 3;
	else
		info->plane_sel_mode = 0;

	if ((info->type->pagesize+info->type->oobsize) > info->buff.size) {
		SPINAND_LOGE("device page size is larger than buffer!\n");
		goto err1;
	}

	info->bch_dec_src = CONFIG_SPINAND_AUTOBCH_DECSRC;
	info->trs_mode = CONFIG_SPINAND_TRSMODE;
	info->raw_trs_mode = CONFIG_SPINAND_TRSMODE_RAW;

	info->read_bitmode = CONFIG_SPINAND_READ_BITMODE;
	if ((info->read_bitmode == SPINAND_4BIT_MODE) &&
		(info->drv_options & SPINAND_OPT_NO_4BIT_READ))
		info->read_bitmode = SPINAND_2BIT_MODE;
	if ((info->read_bitmode == SPINAND_2BIT_MODE) &&
		(info->drv_options & SPINAND_OPT_NO_2BIT_READ))
		info->read_bitmode = SPINAND_1BIT_MODE;

	info->write_bitmode = CONFIG_SPINAND_WRITE_BITMODE;
	if (info->drv_options & SPINAND_OPT_NO_4BIT_PROGRAM)
		info->write_bitmode = SPINAND_1BIT_MODE;

	info->chip_num = SPINAND_OPT_GET_DIENUM(info->drv_options);
	info->cur_chip = -1;

	ret = nand_scan_with_ids(&info->nand, info->chip_num, sp_spinand_ids);
	if (ret) {
		SPINAND_LOGE("nand_scan_with_ids failed\n");
		ret = -ENXIO;
		goto err1;
	}

	register_mtd_parser(&sunplus_nand_parser);
	ret = mtd_device_parse_register(info->mtd, part_types, NULL, NULL, 0);
	if (ret) {
		SPINAND_LOGE("mtd_device_parse_register fail!\n");
		ret = -ENXIO;
		goto err1;
	}

	SPINAND_LOGI("====Sunplus SPI-NAND Driver====\n");
	SPINAND_LOGI("==spi nand driver info==\n");
	SPINAND_LOGI("regs = 0x%p@0x%08llx, size = %lld\n",
		info->regs, res_mem->start, res_mem->end-res_mem->start);
	SPINAND_LOGI("buffer = 0x%p@0x%08llx, size = %d\n",
		info->buff.virt, info->buff.phys, info->buff.size);
	SPINAND_LOGI("irq = %d\n", info->irq);
	SPINAND_LOGI("==spi nand device info==\n");
	SPINAND_LOGI("device name : %s\n", info->mtd->name);
	SPINAND_LOGI("device id   : 0x%08x\n", id);
	SPINAND_LOGI("options     : 0x%08x\n", info->nand.options);
	SPINAND_LOGI("drv options : 0x%08x\n", info->drv_options);
	SPINAND_LOGI("chip number : %d\n", info->chip_num);
	SPINAND_LOGI("block size  : %d\n", info->mtd->erasesize);
	SPINAND_LOGI("page size   : %d\n", info->mtd->writesize);
	SPINAND_LOGI("oob size    : %d\n", info->mtd->oobsize);
	SPINAND_LOGI("oob avail   : %d\n", info->mtd->oobavail);
	SPINAND_LOGI("ecc size    : %d\n", info->nand.ecc.size);
	SPINAND_LOGI("ecc strength: %d\n", info->nand.ecc.strength);
	SPINAND_LOGI("ecc steps   : %d\n", info->nand.ecc.steps);
	SPINAND_LOGI("ecc options : 0x%08x\n", info->nand.ecc.options);

	return ret;

err1:
	if (info->clk) {
		clk_disable(info->clk);
		clk_unprepare(info->clk);
	}

	if (info->regs)
		iounmap(info->regs);

	if (info->buff.virt) {
		#ifdef CONFIG_SPINAND_USE_SRAM
		iounmap(info->buff.virt);
		#else
		dma_free_coherent(NULL, info->buff.size,
			info->buff.virt, info->buff.phys);
		#endif
	}

	if (info->irq > 0)
		free_irq(info->irq, info);

	devm_kfree(dev, (void *)info);

	return ret;
}

static int sp_spinand_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct sp_spinand_info *info = platform_get_drvdata(pdev);

	wake_up(&info->wq);

	mtd_device_unregister(info->mtd);
	deregister_mtd_parser(&sunplus_nand_parser);

	if (info->clk) {
		clk_disable(info->clk);
		clk_unprepare(info->clk);
	}

	if (info->regs)
		iounmap(info->regs);

	if (info->buff.virt) {
		#ifdef CONFIG_SPINAND_USE_SRAM
		iounmap(info->buff.virt);
		#else
		dma_free_coherent(NULL, info->buff.size,
			info->buff.virt, info->buff.phys);
		#endif
	}

	if (info->irq > 0)
		free_irq(info->irq, info);

	devm_kfree(&pdev->dev, (void *)info);

	ret = sp_bch_remove(pdev);

	return ret;
}

static int sp_spinand_suspend(struct device *dev)
{
	struct sp_spinand_info *info = dev_get_drvdata(dev);
	int ret;

	clk_disable_unprepare(info->clk);

	ret = reset_control_assert(info->rstc);
	if (ret) {
		SPINAND_LOGE("Failed to assert reset control\n");
		return ret;
	}

	ret = sp_bch_suspend(dev);
	if (ret) 
		return ret;

	return 0;

}

static int sp_spinand_resume(struct device *dev)
{
	struct sp_spinand_info *info = dev_get_drvdata(dev);
	int ret;

	ret = reset_control_deassert(info->rstc);
	if (ret) {
		SPINAND_LOGE("Failed to deassert reset control\n");
		return ret;
	}

	ret = clk_prepare_enable(info->clk);
	if (ret < 0) {
		SPINAND_LOGE("Failed to enable clock");
		return ret;
	}

	/* Configure the feature registers of SPI-NAND devices */
	sp_spinand_feature_cfg(info);

	ret = sp_bch_resume(dev);
	if (ret) 
		return ret;

	return 0;
}

static const struct dev_pm_ops sp_spinand_pm_ops = {
	.suspend	= sp_spinand_suspend,
	.resume		= sp_spinand_resume,
};

static const struct of_device_id sunplus_nand_of_match[] = {
	{ .compatible = "sunplus,sp7350-spi-nand" },
	{},
};
MODULE_DEVICE_TABLE(of, sunplus_nand_of_match);

static struct platform_driver sp_spinand_driver = {
	.probe = sp_spinand_probe,
	.remove = sp_spinand_remove,
	.driver = {
		.name = "sunplus,spinand",
		.owner = THIS_MODULE,
		.of_match_table = sunplus_nand_of_match,
		.pm = &sp_spinand_pm_ops,
	},
};

static int __init sp_spinand_module_init(void)
{
	platform_driver_register(&sp_spinand_driver);
	return 0;
}

static void __exit sp_spinand_module_exit(void)
{
	platform_driver_unregister(&sp_spinand_driver);
}

module_init(sp_spinand_module_init);
module_exit(sp_spinand_module_exit);

MODULE_AUTHOR("Cheng Chung Ho <cc.ho@sunplus.com>");
MODULE_DESCRIPTION("Sunplus SPI-NAND flash controller driver");
MODULE_LICENSE("GPL v2");
