// SPDX-License-Identifier: GPL-2.0-only
/**
 * (C) Copyright 2019 Sunplus Technology. <http://www.sunplus.com/>
 *
 * Sunplus SD host controller v3.0
 *
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/clk.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include "../core/host.h"
#include "../core/core.h"
#include "sunplus_sd3.h"
#include <linux/iopoll.h>

enum loglevel {
	SPSDC_LOG_OFF,
	SPSDC_LOG_ERROR,
	SPSDC_LOG_WARNING,
	SPSDC_LOG_INFO,
	SPSDC_LOG_DEBUG,
	SPSDC_LOG_VERBOSE,
	SPSDC_LOG_MAX
};
static int loglevel = SPSDC_LOG_WARNING;

/**
 * we do not need `SPSDC_LOG_' prefix here, when specify @level.
 */
#define spsdc_pr(sd, level, fmt,  ...)	\
	do {	\
		if (unlikely(SPSDC_LOG_##level <= loglevel) && (sd == 0))	\
			pr_info("SPSD [" #level "] " fmt, ##__VA_ARGS__);	\
		if (unlikely(SPSDC_LOG_##level <= loglevel) && (sd == 2))	\
			pr_info("SPSDIO [" #level "] " fmt, ##__VA_ARGS__);	\
	} while (0)

/* Produces a mask of set bits covering a range of a 32-bit value */
static inline u32 bitfield_mask(u32 shift, u32 width)
{
	return ((1 << width) - 1) << shift;
}

/* Extract the value of a bitfield found within a given register value */
static inline u32 bitfield_extract(u32 reg_val, u32 shift, u32 width)
{
	return (reg_val & bitfield_mask(shift, width)) >> shift;
}

/* Replace the value of a bitfield found within a given register value */
static inline u32 bitfield_replace(u32 reg_val, u32 shift, u32 width, u32 val)
{
	u32 mask = bitfield_mask(shift, width);

	return (reg_val & ~mask) | (val << shift);
}
/* for register value with mask bits */
#define __bitfield_replace(value, shift, width, new_value)		\
	(bitfield_replace(value, shift, width, new_value) | bitfield_mask(shift+16, width))

static const u8 tuning_blk_pattern_4bit[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

#ifdef SPSDC_DEBUG
#define SPSDC_REG_SIZE (sizeof(struct spsdc_regs)) /* register address space size */
#define SPSDC_REG_GRPS (sizeof(struct spsdc_regs) / 128) /* we organize 32 registers as a group */
#define SPSDC_REG_CNT  (sizeof(struct spsdc_regs) / 4) /* total registers */

/**
 * dump a range of registers.
 * @host: host
 * @start_group: dump start from which group, base is 0
 * @start_reg: dump start from which register in @start_group
 * @len: how many registers to dump
 */
static void spsdc_dump_regs(struct spsdc_host *host, int start_group, int start_reg, int len)
{
	u32 *p = (u32 *)host->base;
	u32 *reg_end = p + SPSDC_REG_CNT;
	u32 *end;
	int groups;
	int i, j;

	if (start_group > SPSDC_REG_GRPS || start_reg > 31)
		return;
	p += start_group * 32 + start_reg;
	if (p > reg_end)
		return;
	end = p + len;
	groups = (len + 31) / 32;
	pr_info("groups = %d\n", groups);
	pr_info("### dump sd card controller registers start ###\n");
	for (i = 0; i < groups; i++) {
		for (j =  start_reg; j < 32 && p < end; j++) {
			pr_info("g%02d.%02d = 0x%08x\n", i+start_group, j, readl(p));
			p++;
		}
		start_reg = 0;
	}
	pr_info("### dump sd card controller registers end ###\n");
}
#endif

/**
 * wait for transaction done, return -1 if error.
 */
static inline int spsdc_wait_finish(struct spsdc_host *host)
{
	/* Wait for transaction finish */
	unsigned long timeout = jiffies + msecs_to_jiffies(5000);

	while (!time_after(jiffies, timeout)) {
		if (readl(&host->base->sd_state) & SPSDC_SDSTATE_FINISH)
			return 0;
		if (readl(&host->base->sd_state) & SPSDC_SDSTATE_ERROR)
			return -1;
	}
	return -1;
}

static inline int spsdc_wait_sdstatus(struct spsdc_host *host, unsigned int status_bit)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(5000);

	while (!time_after(jiffies, timeout)) {
		if (readl(&host->base->sd_status) & status_bit)
			return 0;
		if (readl(&host->base->sd_state) & SPSDC_SDSTATE_ERROR)
			return -1;
	}
	return -1;
}

#define spsdc_wait_rspbuf_full(host) spsdc_wait_sdstatus(host, SPSDC_SDSTATUS_RSP_BUF_FULL)
#define spsdc_wait_rxbuf_full(host) spsdc_wait_sdstatus(host, SPSDC_SDSTATUS_RX_DATA_BUF_FULL)
#define spsdc_wait_txbuf_empty(host) spsdc_wait_sdstatus(host, SPSDC_SDSTATUS_TX_DATA_BUF_EMPTY)

static inline __maybe_unused void spsdc_txdummy(struct spsdc_host *host, int count)
{
	u32 value;

	count &= 0x1ff;
	value = readl(&host->base->sd_config1);
	value = bitfield_replace(value, SPSDC_TX_DUMMY_NUM_w09, 9, count);
	writel(value, &host->base->sd_config1);
	value = readl(&host->base->sd_ctrl);
	value = bitfield_replace(value, SPSDC_sdctrl1_w01, 1, 1); /* trigger tx dummy */
	writel(value, &host->base->sd_ctrl);
}

static void spsdc_get_rsp(struct spsdc_host *host, struct mmc_command *cmd)
{
	u32 value0_3, value4_5;

	if (unlikely(!(cmd->flags & MMC_RSP_PRESENT)))
		return;
	if (unlikely(cmd->flags & MMC_RSP_136)) {
		if (spsdc_wait_rspbuf_full(host))
			return;
		value0_3 = readl(&host->base->sd_rspbuf0_3);
		value4_5 = readl(&host->base->sd_rspbuf4_5) & 0xffff;
		cmd->resp[0] = (value0_3 << 8) | (value4_5 >> 8);
		cmd->resp[1] = value4_5 << 24;
		value0_3 = readl(&host->base->sd_rspbuf0_3);
		value4_5 = readl(&host->base->sd_rspbuf4_5) & 0xffff;
		cmd->resp[1] |= value0_3 >> 8;
		cmd->resp[2] = value0_3 << 24;
		cmd->resp[2] |= value4_5 << 8;
		value0_3 = readl(&host->base->sd_rspbuf0_3);
		value4_5 = readl(&host->base->sd_rspbuf4_5) & 0xffff;
		cmd->resp[2] |= value0_3 >> 24;
		cmd->resp[3] = value0_3 << 8;
		cmd->resp[3] |= value4_5 >> 8;
	} else {
		if (spsdc_wait_rspbuf_full(host))
			return;
		value0_3 = readl(&host->base->sd_rspbuf0_3);
		value4_5 = readl(&host->base->sd_rspbuf4_5) & 0xffff;
		cmd->resp[0] = (value0_3 << 8) | (value4_5 >> 8);
		cmd->resp[1] = value4_5 << 24;
	}
}

static void spsdc_set_bus_clk(struct spsdc_host *host, int clk)
{
	unsigned int clkdiv;
	int f_min = host->mmc->f_min;
	int f_max = host->mmc->f_max;
	int soc_clk  = clk_get_rate(host->clk);
	u32 value = readl(&host->base->sd_config0);

	if (clk < f_min)
		clk = f_min;
	if (clk > f_max)
		clk = f_max;
	if(host->work_clk == clk) {
		spsdc_pr(host->mode, VERBOSE, "clk %d is set\n", clk);
		return;
	}
	if (host->soc_clk != soc_clk){
		clk_set_rate(host->clk, host->soc_clk);
		soc_clk  = clk_get_rate(host->clk);
	}

	clkdiv = (soc_clk/clk)-1;
	if ((soc_clk % clk) > (clk/10)) {
		clkdiv++;
	} 
	spsdc_pr(host->mode, INFO, "clk to %d,SYS_CLK %d,clkdiv %d real_clk %d\n", clk,
			soc_clk, clkdiv, (soc_clk / (clkdiv+1)));

	if (clkdiv > 0xfff) {
		spsdc_pr(host->mode, WARNING, "clock %d is too low to be set!\n", clk);
		clkdiv = 0xfff;
	}
	value = bitfield_replace(value, SPSDC_sdfqsel_w12, 12, clkdiv);
	writel(value, &host->base->sd_config0);
	host->work_clk = clk;

	/* In order to reduce the frequency of context switch,
	 * if it is high speed or upper, we do not use interrupt
	 * when send a command that without data transfering.
	 */
	if (clk > 25000000)
		host->use_int = 0;
	else
		host->use_int = 1;
}

static void spsdc_set_bus_timing(struct spsdc_host *host, unsigned int timing)
{
	u32 value = readl(&host->base->sd_config1);
	int clkdiv = readl(&host->base->sd_config0) >> SPSDC_sdfqsel_w12;
	int wr_delay = clkdiv < 7 ? 1 : 7;
	int rd_delay = clkdiv < 7 ? clkdiv+1 : 7;
	int hs_en = 1;
	union spmmc_reg_timing_config0 reg_timing;
	char *timing_name;

	if(host->timing == timing) {
		spsdc_pr(host->mode, VERBOSE, "timing %d is set\n", timing);
		return;
	}

	host->ddr_enabled = 0;
	reg_timing.bits.sd_wr_dat_dly_sel = wr_delay;
	reg_timing.bits.sd_wr_cmd_dly_sel = wr_delay;
	reg_timing.bits.sd_rd_dat_dly_sel = rd_delay;
	reg_timing.bits.sd_rd_rsp_dly_sel = rd_delay;
	reg_timing.bits.sd_rd_crc_dly_sel = rd_delay;

	switch (timing) {
	case MMC_TIMING_LEGACY:
		hs_en = 0;
		timing_name = "legacy";
		break;
	case MMC_TIMING_MMC_HS:
		timing_name = "mmc high-speed";
		break;
	case MMC_TIMING_SD_HS:
		reg_timing.val = 0x444110;
		timing_name = "sd high-speed";
		break;
	case MMC_TIMING_UHS_SDR50:
		if(host->mode == SPSDC_MODE_SDIO)
			reg_timing.val = host->val;
		else
			reg_timing.val = host->val;
		timing_name = "sd uhs SDR50";
		break;
	case MMC_TIMING_UHS_SDR104:
		if(host->mode == SPSDC_MODE_SDIO)
			reg_timing.val = host->val;	// 160M : 0x666330;
		else
			reg_timing.val = host->val;
		timing_name = "sd uhs SDR104";
		break;
	case MMC_TIMING_UHS_DDR50:
		host->ddr_enabled = 1;
		reg_timing.val = 0x444220;
		timing_name = "sd uhs DDR50";
		break;
	case MMC_TIMING_MMC_DDR52:
		host->ddr_enabled = 1;
		timing_name = "mmc DDR52";
		break;
	case MMC_TIMING_MMC_HS200:
		timing_name = "mmc HS200";
		break;
	default:
		timing_name = "invalid";
		hs_en = 0;
		break;
	}

	if (hs_en) {
		value = bitfield_replace(value, SPSDC_sdhigh_speed_en_w01, 1, 1); /* sd_high_speed_en */
		writel(value, &host->base->sd_config1);
		spsdc_pr(host->mode, VERBOSE, "sd_timing_config0: 0x%08x\n", reg_timing.val);
		writel(reg_timing.val, &host->base->sd_timing_config0);
	} else {
		value = bitfield_replace(value, SPSDC_sdhigh_speed_en_w01, 1, 0);
		writel(value, &host->base->sd_config1);
	}
	if (host->ddr_enabled) {
		value = readl(&host->base->sd_config0);
		value = bitfield_replace(value, SPSDC_sdddrmode_w01, 1, 1); /* sdddrmode */
		writel(value, &host->base->sd_config0);
	} else {
		value = readl(&host->base->sd_config0);
		value = bitfield_replace(value, SPSDC_sdddrmode_w01, 1, 0);
		writel(value, &host->base->sd_config0);
	}


	host->timing = timing;
	spsdc_pr(host->mode, INFO, "set bus timing to %s\n", timing_name);

}

static void spsdc_set_bus_width(struct spsdc_host *host, int width)
{
	u32 value = readl(&host->base->sd_config0);
	int bus_width;

	switch (width) {
	case MMC_BUS_WIDTH_8:
		value = bitfield_replace(value, SPSDC_sddatawd_w01, 1, 0);
		value = bitfield_replace(value, SPSDC_mmc8_en_w01, 1, 1);
		bus_width = 8;
		break;
	case MMC_BUS_WIDTH_4:
		value = bitfield_replace(value, SPSDC_sddatawd_w01, 1, 1);
		value = bitfield_replace(value, SPSDC_mmc8_en_w01, 1, 0);
		bus_width = 4;
		break;
	default:
		value = bitfield_replace(value, SPSDC_sddatawd_w01, 1, 0);
		value = bitfield_replace(value, SPSDC_mmc8_en_w01, 1, 0);
		bus_width = 1;
		break;
	};
	spsdc_pr(host->mode, INFO, "set bus width to %d bit(s)\n", bus_width);
	writel(value, &host->base->sd_config0);
}
/**
 * select the working mode of controller: sd/sdio/emmc
 */
static void spsdc_select_mode(struct spsdc_host *host)
{
	u32 value = readl(&host->base->sd_config0);

	/* set `sdmmcmode', as it will sample data at fall edge
	 * of SD bus clock if `sdmmcmode' is not set when
	 * `sd_high_speed_en' is not set, which is not compliant
	 * with SD specification
	 */
	value = bitfield_replace(value, SPSDC_sdmmcmode_w01, 1, 1);
	switch (host->mode) {
	case SPSDC_MODE_EMMC:
		value = bitfield_replace(value, SPSDC_sdiomode_w01, 1, 0);
		writel(value, &host->base->sd_config0);
		break;
	case SPSDC_MODE_SDIO:
		value = bitfield_replace(value, SPSDC_sdiomode_w01, 1, 1);
		writel(value, &host->base->sd_config0);
		value = readl(&host->base->sdio_ctrl);
		value = bitfield_replace(value, SPSDC_INT_MULTI_TRIG_w01, 1, 1); /* int_multi_trig */
		writel(value, &host->base->sdio_ctrl);
		break;
	case SPSDC_MODE_SD:
	default:
		value = bitfield_replace(value, SPSDC_sdiomode_w01, 1, 0);
		host->mode = SPSDC_MODE_SD;
		writel(value, &host->base->sd_config0);
		break;
	}
}

static void spsdc_sw_reset(struct spsdc_host *host)
{
	u32 value;

	spsdc_pr(host->mode, DEBUG, "sw reset\n");
	/* Must reset dma operation first, or it will
	 * be stuck on sd_state == 0x1c00 because of
	 * a controller software reset bug
	 */
	value = readl(&host->base->hw_dma_ctrl);
	value = bitfield_replace(value, SPSDC_dmaidle_w01, 1, 1);
	writel(value, &host->base->hw_dma_ctrl);
	value = bitfield_replace(value, SPSDC_dmaidle_w01, 1, 0);
	writel(value, &host->base->hw_dma_ctrl);
	value = readl(&host->base->hw_dma_ctrl);
	value = bitfield_replace(value, SPSDC_HW_DMA_RST_w01, 1, 1);
	writel(value, &host->base->hw_dma_ctrl);
	writel(0x7, &host->base->sd_rst);
	readl_poll_timeout_atomic(&host->base->sd_hw_state, value,
		!(value & BIT(6)), 1, SPMMC_TIMEOUT_US);
	spsdc_pr(host->mode, DEBUG, "sw reset done\n");

}

static void spsdc_prepare_cmd(struct spsdc_host *host, struct mmc_command *cmd)
{

	u32 value;

	value = ((cmd->opcode | 0x40) << 24) | (cmd->arg >> 8); /* add start bit, according to spec, command format */

	writel(value, &host->base->sd_cmdbuf0_3);
	writeb(cmd->arg & 0xff, &host->base->sd_cmdbuf4);

	/* disable interrupt if needed */
	value = readl(&host->base->sd_int);
	value = bitfield_replace(value, SPSDC_sd_cmp_clr_w01, 1, 1); /* sd_cmp_clr */
	if (likely(!host->use_int || cmd->flags & MMC_RSP_136))
		value = bitfield_replace(value, SPSDC_sdcmpen_w01, 1, 0); /* sdcmpen */
	else
		value = bitfield_replace(value, SPSDC_sdcmpen_w01, 1, 1);
	writel(value, &host->base->sd_int);

	value = readl(&host->base->sd_config0);
	value = bitfield_replace(value, SPSDC_trans_mode_w02, 2, 0); /* sd_trans_mode */
	value = bitfield_replace(value, SPSDC_sdcmddummy_w01, 1, 1); /* sdcmddummy */
	if (likely(cmd->flags & MMC_RSP_PRESENT)) {
		value = bitfield_replace(value, SPSDC_sdautorsp_w01, 1, 1); /* sdautorsp */
	} else {
		value = bitfield_replace(value, SPSDC_sdautorsp_w01, 1, 0);
		writel(value, &host->base->sd_config0);
		return;
	}
	/*
	 * Currently, host is not capable of checking R2's CRC7,
	 * thus, enable crc7 check only for 48 bit response commands
	 */
	if (likely(cmd->flags & MMC_RSP_CRC && !(cmd->flags & MMC_RSP_136)))
		value = bitfield_replace(value, SPSDC_sdrspchk_w01, 1, 1); /* sdrspchk_en */
	else
		value = bitfield_replace(value, SPSDC_sdrspchk_w01, 1, 0);

	if (unlikely(cmd->flags & MMC_RSP_136))
		value = bitfield_replace(value, SPSDC_sdrsptype_w01, 1, 1); /* sdrsptype */
	else
		value = bitfield_replace(value, SPSDC_sdrsptype_w01, 1, 0);
	writel(value, &host->base->sd_config0);
}

static void spsdc_prepare_data(struct spsdc_host *host, struct mmc_data *data)
{
	u32 value, srcdst;
	host->data = data;

	writel(data->blocks - 1, &host->base->sd_page_num);
	writel(data->blksz - 1, &host->base->sd_blocksize);
	value = readl(&host->base->sd_config0);
	if (data->flags & MMC_DATA_READ) {
		value = bitfield_replace(value, SPSDC_trans_mode_w02, 2, 2); /* sd_trans_mode */
		value = bitfield_replace(value, SPSDC_sdautorsp_w01, 1, 0); /* sdautorsp */
		value = bitfield_replace(value, SPSDC_sdcmddummy_w01, 1, 0); /* sdcmddummy */
		srcdst = readl(&host->base->card_mediatype_srcdst);
		srcdst = bitfield_replace(srcdst, SPSDC_dmasrc_w03, 7, SPSDC_DMA_READ);
		writel(srcdst, &host->base->card_mediatype_srcdst);
	} else {
		value = bitfield_replace(value, SPSDC_trans_mode_w02, 2, 1);
		srcdst = readl(&host->base->card_mediatype_srcdst);
		srcdst = bitfield_replace(srcdst, SPSDC_dmasrc_w03, 7, SPSDC_DMA_WRITE);
		writel(srcdst, &host->base->card_mediatype_srcdst);
	}

	/* to prevent of the responses of CMD18/25 being overrided by CMD12's,
	 * send CMD12 by ourself instead of by controller automatically
	 *
	 *	if ((cmd->opcode == MMC_READ_MULTIPLE_BLOCK) || (cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))
	 *	value = bitfield_replace(value, SPSDC_sd_len_mode_w01, 1, 0); // sd_len_mode
	 *	else
	 *	value = bitfield_replace(value, SPSDC_sd_len_mode_w01, 1, 1);
	 *
	 */
	value = bitfield_replace(value, SPSDC_sd_len_mode_w01, 1, 1);

	if (likely(host->dmapio_mode == SPSDC_DMA_MODE)) {
		struct scatterlist *sg;
		dma_addr_t dma_addr;
		unsigned int dma_size, sg_xlen;
		u32 *reg_addr;
		int dma_direction = data->flags & MMC_DATA_READ ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
		int i, count = 1;

		count = dma_map_sg(host->dev, data->sg, data->sg_len, dma_direction);
		if (unlikely(!count)) {
			spsdc_pr(host->mode, ERROR, "error occured at dma_mapp_sg: count = %d\n", count);
			data->error = -EINVAL;
			return;
		}

		if((host->ram >= 8) && (data->sg_len > SPSDC_HW_SEGS) ) {
			host->xfer_len = data->blocks * data->blksz;
			if (data->flags & MMC_DATA_WRITE) {
				sg_copy_to_buffer(data->sg, data->sg_len,
					host->buffer, host->xfer_len);
					/* Switch ownership to the DMA */
				dma_sync_single_for_device(host->dev,
							host->buf_phys_addr,
							host->xfer_len,
							mmc_get_dma_dir(data));
			}
			dma_addr = host->buf_phys_addr;
			dma_size = data->blocks - 1;
			//pr_info("c %d l %d s %d addr %x\n", data->sg_len,data->blocks*data->blksz,host->xfer_len,host->buf_phys_addr);
			writel(dma_addr, &host->base->dma_base_addr);
			writel(dma_size, &host->base->sdram_sector_0_size);
		} else {
			if(data->sg_len > SPSDC_HW_SEGS) {
			host->xfer_len = data->blocks * data->blksz;
			count = 8;
		}
			for_each_sg(data->sg, sg, count, i) {
				dma_addr = sg_dma_address(sg);
				sg_xlen = sg_dma_len(sg);
				dma_size = sg_xlen / data->blksz - 1;
				if (i == 0) {
					writel(dma_addr, &host->base->dma_base_addr);
					writel(dma_size, &host->base->sdram_sector_0_size);
					host->xfer_len -= sg_xlen;
				} else if ((i < 7) || (data->sg_len <= SPSDC_MAX_DMA_MEMORY_SECTORS)) {
					reg_addr = &host->base->sdram_sector_1_addr + (i - 1) * 2;
					writel(dma_addr, reg_addr);
					writel(dma_size, reg_addr + 1);
					host->xfer_len -= sg_xlen;
				} else {
					if (data->flags & MMC_DATA_WRITE) {
						sg_copy_to_buffer(sg, data->sg_len-7,
					  		host->buffer, host->xfer_len);
						/* Switch ownership to the DMA */
						dma_sync_single_for_device(host->dev,
							host->buf_phys_addr,
							host->xfer_len,
							mmc_get_dma_dir(data));
					}
					dma_addr = host->buf_phys_addr;
					dma_size = host->xfer_len / data->blksz - 1;
					writel(dma_addr, &host->base->sdram_sector_7_addr);
					writel(dma_size, &host->base->sdram_sector_7_size);
				}
			}
		}

		value = bitfield_replace(value, SPSDC_sdpiomode_w01, 1, 0); /* sdpiomode */
		writel(value, &host->base->sd_config0);
		/* enable interrupt if needed */
		if (!host->use_int && data->blksz * data->blocks > host->dma_int_threshold) {
			host->dma_use_int = 1;
			value = readl(&host->base->sd_int);
			value = bitfield_replace(value, SPSDC_sdcmpen_w01, 1, 1); /* sdcmpen */
			writel(value, &host->base->sd_int);
		}
	} else {
		value = bitfield_replace(value, SPSDC_sdpiomode_w01, 1, 1);
		value = bitfield_replace(value, SPSDC_rx4_en_w01, 1, 1); /* rx4_en */
		writel(value, &host->base->sd_config0);
	}
}

static inline void spsdc_trigger_transaction(struct spsdc_host *host)
{
	u32 value = readl(&host->base->sd_ctrl);

	value = bitfield_replace(value, SPSDC_sdctrl0_w01, 1, 1); /* trigger transaction */
	writel(value, &host->base->sd_ctrl);
}

static int __send_stop_cmd(struct spsdc_host *host, struct mmc_command *stop)
{
	u32 value;

	spsdc_prepare_cmd(host, stop);
	value = readl(&host->base->sd_int);
	value = bitfield_replace(value, SPSDC_sdcmpen_w01, 1, 0); /* sdcmpen */
	writel(value, &host->base->sd_int);
	spsdc_trigger_transaction(host);
	spsdc_get_rsp(host, stop);
	if (spsdc_wait_finish(host)) {
		value = readl(&host->base->sd_status);
		if (value & SPSDC_SDSTATUS_RSP_CRC7_ERROR)
			stop->error = -EILSEQ;
		else
			stop->error = -ETIMEDOUT;
		return -1;
	}
	return 0;
}

/*
 * check if error occured during transaction.
 * @host -  host
 * @mrq - the mrq
 * @return 0 if no error otherwise the error number.
 */
static int spsdc_check_error(struct spsdc_host *host, struct mmc_request *mrq)
{
	int ret = 0;
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	u32 value = readl(&host->base->sd_state);
	u32 crc_token = bitfield_extract(value, SPSDC_sdcrdcrc_w03, 3);
	union spmmc_reg_timing_config0 timing;
	int clkdiv = readl(&host->base->sd_config0) >> SPSDC_sdfqsel_w12;

	if (unlikely(value & SPSDC_SDSTATE_ERROR)) {
		spsdc_pr(host->mode, DEBUG, "%s cmd %d with data %08x error!\n", __func__, cmd->opcode, (unsigned int)(long)data);
		spsdc_pr(host->mode, VERBOSE, "%s sd_state: 0x%08x\n", __func__, value);
		value = readl(&host->base->sd_status);
		spsdc_pr(host->mode, VERBOSE, "%s sd_status: 0x%08x\n", __func__, value);

		if (host->tuning_info.enable_tuning) {
			timing.val = readl(&host->base->sd_timing_config0);
		}
                
		if (value & SPSDC_SDSTATUS_RSP_TIMEOUT) {
			spsdc_pr(host->mode, VERBOSE, "SPSDC_SDSTATUS_RSP_TIMEOUT\n");
			ret = (host->tuning_info.enable_tuning) ? -ETIMEDOUT : -1;
			timing.bits.sd_wr_cmd_dly_sel++;
		} else if (value & SPSDC_SDSTATUS_RSP_CRC7_ERROR) {
			spsdc_pr(host->mode, VERBOSE, "SPSDC_SDSTATUS_RSP_CRC7_ERROR\n");
			ret = (host->tuning_info.enable_tuning) ? -EILSEQ : -1;
			timing.bits.sd_rd_rsp_dly_sel++;
		} else if (data) {
			if ((value & SPSDC_SDSTATUS_STB_TIMEOUT)) {
				spsdc_pr(host->mode, VERBOSE, "SPSDC_SDSTATUS_STB_TIMEOUT\n");
				ret = (host->tuning_info.enable_tuning) ? -ETIMEDOUT : -1;
				timing.bits.sd_rd_dat_dly_sel++;
			} else if (value & SPSDC_SDSTATUS_RDATA_CRC16_ERROR) {
				spsdc_pr(host->mode, VERBOSE, "SPSDC_SDSTATUS_RDATA_CRC16_ERROR\n");
				ret = (host->tuning_info.enable_tuning) ? -EILSEQ : -1;
				timing.bits.sd_rd_dat_dly_sel++;
			} else if (value & SPSDC_SDSTATUS_CARD_CRC_CHECK_TIMEOUT) {
				spsdc_pr(host->mode, VERBOSE, "SPSDC_SDSTATUS_CARD_CRC_CHECK_TIMEOUT\n");
				ret = (host->tuning_info.enable_tuning) ? -ETIMEDOUT : -1;
				timing.bits.sd_rd_crc_dly_sel++;
			} else if (value & SPSDC_SDSTATUS_CRC_TOKEN_CHECK_ERROR) {
				spsdc_pr(host->mode, VERBOSE, "SPSDC_SDSTATUS_CRC_TOKEN_CHECK_ERROR\n");
				ret = (host->tuning_info.enable_tuning) ? -EILSEQ : -1;
				if (crc_token == 0x5)
					timing.bits.sd_wr_dat_dly_sel++;
				else
					timing.bits.sd_rd_crc_dly_sel++;
			}
			}
		cmd->error = ret;
		if (data) {
			data->error = ret;
			data->bytes_xfered = 0;
		}

		//if (!host->tuning_info.need_tuning) {
		if (!host->tuning_info.need_tuning && host->tuning_info.enable_tuning) {
			if (clkdiv >= 500)
				cmd->retries = 1; /* retry it */
			else
				cmd->retries = SPSDC_MAX_RETRIES; /* retry it */
		}
		spsdc_sw_reset(host);

		if (host->tuning_info.enable_tuning) {
			writel(timing.val, &host->base->sd_timing_config0);
		}

	} else if (data) {
		data->error = 0;
		data->bytes_xfered = data->blocks * data->blksz;
	}
	host->tuning_info.need_tuning = ret;
	return ret;
}

static void spsdc_xfer_data_pio(struct spsdc_host *host, struct mmc_data *data)
{
	u32 *buf; /* tx/rx 4 bytes one time in pio mode */
	int data_left = data->blocks * data->blksz;
	int consumed, remain;
	struct sg_mapping_iter *sg_miter = &host->sg_miter;
	unsigned int flags = 0;

	if (data->flags & MMC_DATA_WRITE)
		flags |= SG_MITER_FROM_SG;
	else
		flags |= SG_MITER_TO_SG;
	sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);
	while (data_left > 0) {
		consumed = 0;
		if (!sg_miter_next(sg_miter))
			break;
		buf = sg_miter->addr;
		remain = sg_miter->length;
		do {
			if (data->flags & MMC_DATA_WRITE) {
				if (spsdc_wait_txbuf_empty(host))
					goto done;
				writel(*buf, &host->base->sd_piodatatx);
			} else {
				if (spsdc_wait_rxbuf_full(host))
					goto done;
				*buf = readl(&host->base->sd_piodatarx);
			}
			buf++;
			consumed += 4; // enable rx4_en +=4  diaable +=2
			remain -= 4;
		} while (remain);
		sg_miter->consumed = consumed;
		data_left -= consumed;
	}
done:
	sg_miter_stop(sg_miter);
}

static void spsdc_controller_init(struct spsdc_host *host)
{
	u32 value;

	spsdc_sw_reset(host);
	value = readl(&host->base->card_mediatype_srcdst);
	value = bitfield_replace(value, SPSDC_MediaType_w03, 3, SPSDC_MEDIA_SD);
	writel(value, &host->base->card_mediatype_srcdst);
	host->signal_voltage = MMC_SIGNAL_VOLTAGE_330;

	/* Because we do not have a regulator to change the voltage at
	 * runtime, we can only rely on hardware circuit to ensure that
	 * the device pull up voltage is 1.8V(ex: wifi module AP6256) and
	 * use the macro `SPMMC_SDIO_1V8'to indicate that. Set signal
	 * voltage to 1.8V here.
	 */
	if ((host->mode == SPSDC_MODE_SDIO) && (host->vol_mode == SPSDC_1V8_MODE)) {
		value = readl(&host->base->sd_vol_ctrl);
		value = bitfield_replace(value, SPSDC_sw_set_vol_w01, 1, 1);
		writel(value, &host->base->sd_vol_ctrl);
		msleep(20);
		spsdc_txdummy(host, 400);
		host->signal_voltage = MMC_SIGNAL_VOLTAGE_180;
		spsdc_pr(host->mode, INFO, "use signal voltage 1.8V for SDIO\n");
	}
}

static void spsdc_set_power_mode(struct spsdc_host *host, struct mmc_ios *ios)
{
	if (host->power_state == ios->power_mode)
		return;

	switch (ios->power_mode) {
		/* power off->up->on */
	case MMC_POWER_ON:
		spsdc_pr(host->mode, DEBUG, "set SD_POWER_ON\n");
		spsdc_controller_init(host);
		pm_runtime_get_sync(host->mmc->parent);
		break;
	case MMC_POWER_UP:
		spsdc_pr(host->mode, DEBUG, "setSD_POWER_UP\n");
		break;
	case MMC_POWER_OFF:
		spsdc_pr(host->mode, DEBUG, "set SD_POWER_OFF\n");
		pm_runtime_put(host->mmc->parent);
		break;
	}
	host->power_state = ios->power_mode;
}

/**
 * 1. unmap scatterlist if needed;
 * 2. get response & check error conditions;
 * 3. unlock host->mrq_lock
 * 4. notify mmc layer the request is done
 */
static void spsdc_finish_request(struct spsdc_host *host, struct mmc_request *mrq)
{
	struct mmc_command *cmd;
	struct mmc_data *data;
	int i, count;
	struct scatterlist *sg;

	if (!mrq)
		return;

	cmd = mrq->cmd;
	data = mrq->data;

	spsdc_get_rsp(host, cmd);

	if (!(host->use_int || host->dma_use_int))
			spsdc_wait_finish(host);

	if (data && SPSDC_DMA_MODE == host->dmapio_mode) {
		int dma_direction = data->flags & MMC_DATA_READ ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
		if ((data->flags & MMC_DATA_READ) && (data->sg_len > SPSDC_HW_SEGS) ){
			if(host->ram >= 8) {
				dma_sync_single_for_cpu(host->dev,
					host->buf_phys_addr,
					host->xfer_len,
					DMA_FROM_DEVICE);
				sg_copy_from_buffer(data->sg, data->sg_len,
					host->buffer, host->xfer_len);
			} else {
				count = 8;
				for_each_sg(data->sg, sg, count, i) {
					if (i == 7) {
						dma_sync_single_for_cpu(host->dev,
						host->buf_phys_addr,
						host->xfer_len,
						DMA_FROM_DEVICE);
						sg_copy_from_buffer(sg, data->sg_len -7,
						   host->buffer, host->xfer_len);
					}
				}
			}
		}
		dma_unmap_sg(host->dev, data->sg, data->sg_len, dma_direction);
		host->dma_use_int = 0;
	}

	spsdc_check_error(host, mrq);
	if (mrq->stop) {
		if (__send_stop_cmd(host, mrq->stop))
			spsdc_sw_reset(host);
	}
	host->mrq = NULL;
	mutex_unlock(&host->mrq_lock);
	//	if(((host->mode == SPSDC_MODE_SD) && (cmd->opcode != 13) && (cmd->opcode != 18) && (cmd->opcode != 25)) && (cmd->opcode != 52) && (cmd->opcode != 53)){
	spsdc_pr(host->mode, VERBOSE, "request done > error:%d, cmd:%d, resp:0x%08x\n", cmd->error, cmd->opcode, cmd->resp[0]);
	//		}
	mmc_request_done(host->mmc, mrq);
}


/* Interrupt Service Routine */
irqreturn_t spsdc_irq(int irq, void *dev_id)
{
	struct spsdc_host *host = dev_id;
	u32 value = readl(&host->base->sd_int);
	unsigned int sdio_no_irq = 1;

	if ((value & SPSDC_SDINT_SDIO) && (value & SPSDC_SDINT_SDIOEN)){
		mmc_signal_sdio_irq(host->mmc);
		sdio_no_irq = 0;
	}

	if ((value & SPSDC_SDINT_SDCMP) &&
		(value & SPSDC_SDINT_SDCMPEN)) {
		value = bitfield_replace(value, SPSDC_sdcmpen_w01, 1, 0); /* disable sdcmmp */
		value = bitfield_replace(value, SPSDC_sd_cmp_w01, 1, 1); /* sd_cmp_clr */
		writel(value, &host->base->sd_int);
		/* we may need send stop command to stop data transaction,
		 * which is time consuming, so make use of tasklet to handle this.
		 */
		if (host->mrq && host->mrq->stop)
			tasklet_schedule(&host->tsklet_finish_req);
		else
			spsdc_finish_request(host, host->mrq);

		value = readl(&host->base->sd_int);
		if ((value & SPSDC_SDINT_SDIO) && (value & SPSDC_SDINT_SDIOEN) && (sdio_no_irq))
		mmc_signal_sdio_irq(host->mmc);

	}
	return IRQ_HANDLED;
}

static void spsdc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct spsdc_host *host = mmc_priv(mmc);
	struct mmc_data *data;
	struct mmc_command *cmd;
	int ret = 0;

	ret = mutex_lock_interruptible(&host->mrq_lock);
	host->mrq = mrq;
	data = mrq->data;
	cmd = mrq->cmd;

	//if(((host->mode == SPSDC_MODE_SD) && (cmd->opcode != 13) && (cmd->opcode != 18) && (cmd->opcode != 25)) && (cmd->opcode != 52) && (cmd->opcode != 53)){
	spsdc_pr(host->mode, VERBOSE, "%s > cmd:%d, arg:0x%08x, data len:%d\n", __func__,
		cmd->opcode, cmd->arg, data ? (data->blocks*data->blksz) : 0);
	//}

#ifdef HW_VOLTAGE_1V8
	u32 value;

	value = readl(&host->base->sd_vol_ctrl);
	value = bitfield_replace(value, SPSDC_vol_tmr_w02, 2, 3); /* 1ms timeout for 400K */

	if (cmd->opcode == 11)
		value = bitfield_replace(value, SPSDC_hw_set_vol_w01, 1, 1);
	else
		value = bitfield_replace(value, SPSDC_hw_set_vol_w01, 1, 0);

	//spsdc_pr(host->mode, WARNING, "base->sd_vol_ctrl!  0x%x\n",readl(&host->base->sd_vol_ctrl));
	writel(value, &host->base->sd_vol_ctrl);

#endif

	spsdc_prepare_cmd(host, cmd);

	/* we need manually read response R2. */
	if (unlikely(cmd->flags & MMC_RSP_136)) {
		spsdc_trigger_transaction(host);
		spsdc_get_rsp(host, cmd);
		spsdc_wait_finish(host);
		spsdc_check_error(host, mrq);
		host->mrq = NULL;
		spsdc_pr(host->mode, VERBOSE, "request done > error:%d, cmd:%d, resp:%08x %08x %08x %08x\n",
			 cmd->error, cmd->opcode, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
		mutex_unlock(&host->mrq_lock);
		mmc_request_done(host->mmc, mrq);
	} else {
		if (data)
			spsdc_prepare_data(host, data);

		if (unlikely(host->dmapio_mode == SPSDC_PIO_MODE && data)) {
			u32 value;
			/* pio data transfer do not use interrupt */
			value = readl(&host->base->sd_int);
			value = bitfield_replace(value, SPSDC_sdcmpen_w01, 1, 0); /* sdcmpen */
			writel(value, &host->base->sd_int);
			spsdc_trigger_transaction(host);
			spsdc_xfer_data_pio(host, data);
			spsdc_wait_finish(host);
			spsdc_finish_request(host, mrq);
		} else {
			if (!(host->use_int || host->dma_use_int)) {
				spsdc_trigger_transaction(host);
				spsdc_finish_request(host, mrq);
			} else {
				spsdc_trigger_transaction(host);
			}
		}
	}
}

static void spsdc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct spsdc_host *host = (struct spsdc_host *)mmc_priv(mmc);

	mutex_lock(&host->mrq_lock);
	spsdc_set_power_mode(host, ios);
	spsdc_set_bus_clk(host, ios->clock);
	spsdc_set_bus_timing(host, ios->timing);
	spsdc_set_bus_width(host, ios->bus_width);
	/* ensure mode is correct, because we might have hw reset the controller */
	spsdc_select_mode(host);
	mutex_unlock(&host->mrq_lock);

}

/**
 * Return values for the get_cd callback should be:
 *   0 for a absent card
 *   1 for a present card
 *   -ENOSYS when not supported (equal to NULL callback)
 *   or a negative errno value when something bad happened
 */
int spsdc_get_cd(struct mmc_host *mmc)
{
	int ret = 0;

	if (mmc_can_gpio_cd(mmc))
		ret = mmc_gpio_get_cd(mmc);
	else
		spsdc_pr(0,WARNING, "no gpio assigned for card detection\n");

	if (ret < 0) {
		spsdc_pr(0,ERROR, "Failed to get card presence status\n");
		ret = 0;
	}

    //return 1;  // for zebu test
	return ret;
}

#ifdef SPMMC_SUPPORT_VOLTAGE_1V8
static int spmmc_card_busy(struct mmc_host *mmc)
{
	struct spsdc_host *host = mmc_priv(mmc);

	if(host->mode == SPSDC_MODE_SD)
	spsdc_pr(host->mode, INFO, "card_busy! %d\n", !(readl(&host->base->sd_status) & SPSDC_SDSTATUS_DAT0_PIN_STATUS));
	return !(readl(&host->base->sd_status) & SPSDC_SDSTATUS_DAT0_PIN_STATUS);
}

static int spmmc_start_signal_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct spsdc_host *host = mmc_priv(mmc);
	u32 value;

	spsdc_pr(host->mode, INFO, "start_signal_voltage_switch: host->voltage %d ios->voltage %d!\n", host->signal_voltage, ios->signal_voltage);

	if (host->signal_voltage == ios->signal_voltage) {

		spsdc_txdummy(host, 400);
		return 0;

	}

	/* we do not support switch signal voltage for eMMC at runtime at present */
	if (host->mode == SPSDC_MODE_EMMC)
		return -EIO;

	if (ios->signal_voltage != MMC_SIGNAL_VOLTAGE_180) {
		spsdc_pr(host->mode, INFO, "can not switch voltage, only support 3.3v -> 1.8v switch!\n");
		return -EIO;
	}

#ifdef HW_VOLTAGE_1V8

	msleep(15);
	value = readl(&host->base->sd_ctrl);
	value = bitfield_replace(value, SPSDC_sdctrl1_w01, 1, 1); /* trigger tx dummy */
	writel(value, &host->base->sd_ctrl);
	msleep(1);

	/* mmc layer has guaranteed that CMD11 had issued to SD card at
	 * this time, so we can just continue to check the status.
	 */
	value = readl(&host->base->sd_vol_ctrl);
	for (i = 0 ; i <= 10 ;) {
		if (SPSDC_SWITCH_VOLTAGE_1V8_ERROR == (value & SPSDC_SWITCH_VOLTAGE_MASK) >> 4)
			return -EIO;
		if (SPSDC_SWITCH_VOLTAGE_1V8_TIMEOUT == (value & SPSDC_SWITCH_VOLTAGE_MASK) >> 4)
			return -EIO;
		if (SPSDC_SWITCH_VOLTAGE_1V8_FINISH == (value & SPSDC_SWITCH_VOLTAGE_MASK) >> 4)
			break;
		//if (value >> 4 == 0)
			i++;
		spsdc_pr(host->mode, INFO, "1V8 result %d\n", value >> 4);
	}

		spsdc_pr(host->mode, INFO, "1V8 result out %d\n", value >> 4);

#else


	value = readl(&host->base->sd_vol_ctrl);
	value = bitfield_replace(value, SPSDC_sw_set_vol_w01, 1, 1);
	writel(value, &host->base->sd_vol_ctrl);

	spsdc_pr(host->mode, INFO, "base->sd_vol_ctrl!  0x%x\n", readl(&host->base->sd_vol_ctrl));

	msleep(20);
	spsdc_txdummy(host, 400);
	msleep(1);

	#endif

	host->signal_voltage = ios->signal_voltage;
	return 0;
}
#endif /* ifdef SPMMC_SUPPORT_VOLTAGE_1V8 */

#ifdef SPMMC_SUPPORT_EXECUTE_TUNING

/**
 * the strategy is:
 * 1. if several continuous delays are acceptable, we choose a middle one;
 * 2. otherwise, we choose the first one.
 */
static int spmmc_find_best_delay(u32 valid, u32 valid_cnt, u32 *best)
{
	u32 cnt = 0, start = 0;
	u32 max_cnt = 0, max_start = 0;
	u32 i, find = 0;

	for (i = 0; i < 32; i++) {
		if (valid & (1 << i)) {
			if (cnt == 0)
				start = i;
			cnt++;
		} else {
			if (cnt >= valid_cnt) {
				if (cnt > max_cnt) {
					max_cnt = cnt;
					max_start = start;
				}
			}
			cnt = 0;
		}
	}

	if (max_cnt) {
		*best = max_start + (max_cnt / 2);
		find = 1;
	} else if (cnt > max_cnt && cnt > valid_cnt) {
		*best = start + (cnt / 2);
		find = 1;
	}

	return  (find) ? 0 : -1;
}

static void spsdc_dly_set(union spmmc_reg_timing_config0 *timing, u32 item, u32 dly)
{
	switch (item) {
	case RD_CRC_DLY:
		timing->bits.sd_rd_crc_dly_sel = dly & 0x07;
		break;
	case RD_DAT_DLY:
		timing->bits.sd_rd_dat_dly_sel = dly & 0x07;
		break;
	case RD_RSP_DLY:
		timing->bits.sd_rd_rsp_dly_sel = dly & 0x07;
		break;
	case WR_DAT_DLY:
		timing->bits.sd_wr_dat_dly_sel = dly & 0x07;
		break;
	case WR_CMD_DLY:
		timing->bits.sd_wr_cmd_dly_sel = dly & 0x07;
		break;
	default:
		timing->bits.sd_rd_crc_dly_sel = dly & 0x07;
		break;
	}
}

static int spsdc_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct spsdc_host *host = mmc_priv(mmc);
	const u8 *blk_pattern;
	u8 *blk_test;
	int blksz;
	u32 smpl_dly = 0, candidate_dly = 0;
	union spmmc_reg_timing_config0 timing;
	u32 value, item;

	blk_pattern = tuning_blk_pattern_4bit;
	blksz = sizeof(tuning_blk_pattern_4bit);
	blk_test = kmalloc(blksz, GFP_KERNEL);


	if (host->tuning_info.tuning_finish)
		return 0;
	if(host->mode == SPSDC_MODE_SD)
		return 0;

	if (!((mmc->ios.timing == MMC_TIMING_MMC_HS200) ||
	      (mmc->ios.timing == MMC_TIMING_UHS_SDR104) ||
	      (mmc->ios.timing == MMC_TIMING_UHS_SDR50)))
		return 0;

	spsdc_pr(host->mode, INFO, "%s\n", __func__);
	host->tuning_info.enable_tuning = 0;
	for (item = 0; item < 6; item++) {
		candidate_dly = 0;
		//for (dly = 0; dly < 0x20; dly++) {
		for (smpl_dly = 0; smpl_dly < 0x08; smpl_dly++) {
			struct mmc_request mrq = {NULL};
			struct mmc_command cmd = {0};
			struct mmc_command stop = {0};
			struct mmc_data data = {0};
			struct scatterlist sg;

			cmd.opcode = opcode;
			cmd.arg = 0;
			cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

			stop.opcode = MMC_STOP_TRANSMISSION;
			stop.arg = 0;
			stop.flags = MMC_RSP_R1B | MMC_CMD_AC;

			data.blksz = blksz;
			data.blocks = 1;
			data.flags = MMC_DATA_READ;
			data.sg = &sg;
			data.sg_len = 1;

			sg_init_one(&sg, blk_test, blksz);
			mrq.cmd = &cmd;
			mrq.stop = &stop;
			mrq.data = &data;
			host->mrq = &mrq;

			timing.val = readl(&host->base->sd_timing_config0);
			value = timing.val;
			spsdc_dly_set(&timing, item, smpl_dly);
			writel(timing.val, &host->base->sd_timing_config0);
			//msleep(1);
			mmc_wait_for_req(mmc, &mrq);
			if (!cmd.error && !data.error) {
				if (!memcmp(blk_pattern, blk_test, blksz))
					candidate_dly |= (1 << smpl_dly);
			} else {
				spsdc_pr(host->mode, DEBUG, "Tuning error: cmd.error:%d, data.error:%d\n",
				cmd.error, data.error);
			}
		}
		spsdc_pr(host->mode, DEBUG, "sampling delay candidates: %x\n", candidate_dly);
		if (candidate_dly) {
			spmmc_find_best_delay(candidate_dly, 3, &smpl_dly);
			//smpl_dly = __find_best_delay(candidate_dly);
			spsdc_pr(host->mode, DEBUG, "set sampling delay to: %d\n", smpl_dly);
	
			timing.val = readl(&host->base->sd_timing_config0);
			value = timing.val;
			spsdc_dly_set(&timing, item, smpl_dly);
			writel(timing.val, &host->base->sd_timing_config0);
			host->tuning_info.tuning_finish = 1;
		}
	}
	host->tuning_info.enable_tuning = 1;
	spsdc_pr(host->mode, DEBUG,"tuning: %#x old: %#x\n",  timing.val, value);
	pr_info("tuning: %#x old: %#x\n",  timing.val, value);
	timing.val = readl(&host->base->sd_timing_config0);
	host->tuning_info.tuning_finish = 1;

	return 0;
}
#endif

static void spsdc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct spsdc_host *host = mmc_priv(mmc);
	u32 value = readl(&host->base->sd_int);

	value = bitfield_replace(value, SPSDC_sdio_init_clr_w01, 1, 1); /* sdio_int_clr */
	if (enable)
		value = bitfield_replace(value, SPSDC_sdio_init_en_w01, 1, 1);
	else
		value = bitfield_replace(value, SPSDC_sdio_init_en_w01, 1, 0);
	writel(value, &host->base->sd_int);
}

static int spmmc_select_drive_strength(
	struct mmc_card *card, unsigned int max_dtr,
	int host_drv, int card_drv, int *drv_type)
{
	struct spsdc_host *host = mmc_priv(card->host);

	*drv_type = 0; /* Default driver */
	if (mmc_driver_type_mask(host->target_drv) & card_drv)
		*drv_type = host->target_drv;
	return *drv_type;
}

static const struct spsdc_compatible sp_sd_654_compat = {
	.source_clk = SPSDC_CLK_800M,
	.vol_mode = SPSDC_SWITCH_MODE,
	.delay_val = 0x777110,
};
static const struct spsdc_compatible sp_sd_654_loscr_compat = {
	.source_clk = SPSDC_CLK_360M,
	.vol_mode = SPSDC_SWITCH_MODE,
	.delay_val = 0x666330,
};
static const struct spsdc_compatible sp_sdio_1v8_654_compat = {
	.source_clk = SPSDC_CLK_800M,
	.vol_mode = SPSDC_1V8_MODE,
	.delay_val = 0x777110,
};

static const struct of_device_id spsdc_of_table[] = {
	{
		.compatible = "sunplus,sp7350-sd",
		.data = &sp_sd_654_compat,
	},
	{
		.compatible = "sunplus,sp7350-loscr-sd",
		.data = &sp_sd_654_loscr_compat,
	},
	{
		.compatible = "sunplus,sp7350-1v8-sdio",
		.data = &sp_sdio_1v8_654_compat,
	},
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(of, spsdc_of_table);

static const struct mmc_host_ops spsdc_ops = {
	.request = spsdc_request,
	.set_ios = spsdc_set_ios,
	.get_cd = spsdc_get_cd,
#ifdef SPMMC_SUPPORT_VOLTAGE_1V8
	.card_busy = spmmc_card_busy,
	.start_signal_voltage_switch = spmmc_start_signal_voltage_switch,
#endif
	.enable_sdio_irq = spsdc_enable_sdio_irq,
#ifdef SPMMC_SUPPORT_EXECUTE_TUNING
	.execute_tuning = spsdc_execute_tuning,
#endif
	.select_drive_strength = spmmc_select_drive_strength,
};

static void tsklet_func_finish_req(unsigned long data)
{
	struct spsdc_host *host = (struct spsdc_host *)data;

	spin_lock(&host->lock);
	spsdc_finish_request(host, host->mrq);
	spin_unlock(&host->lock);
}

static int spsdc_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mmc_host *mmc;
	struct resource *resource;
	struct spsdc_host *host;
	const struct spsdc_compatible *dev_mode;

	mmc = mmc_alloc_host(sizeof(*host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_free_host;
	}

	host = mmc_priv(mmc);
	if (!of_property_read_u32(pdev->dev.of_node, "sunplus.driver-type", &host->target_drv))
		host->target_drv = 0;	//0:TypeB 1:TypeA 2:TypeC 3:TypeD		

	if (device_property_read_bool(&pdev->dev, "sunplus.high-segs"))
		host->segs_no = SPSDC_MAX_SEGS;
	else
		host->segs_no = SPSDC_HW_SEGS;

	if (host->segs_no < SPSDC_HW_SEGS)
		host->segs_no = SPSDC_HW_SEGS;

	if (device_property_read_bool(&pdev->dev, "sunplus.high-mem"))
		host->ram = 8;
	else
		host->ram = 3;

	host->mmc = mmc;
	host->power_state = MMC_POWER_OFF;
	host->dma_int_threshold = 1024;
	host->dmapio_mode = SPSDC_DMA_MODE;
	//host->dmapio_mode = SPSDC_PIO_MODE;
	ret = mmc_of_parse(mmc);
	if (ret)
		goto probe_free_host;

	if (mmc->caps2 & MMC_CAP2_NO_SDIO)
		host->mode = SPSDC_MODE_SD;
	else
		host->mode = SPSDC_MODE_SDIO;

	host->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(host->clk)) {
		spsdc_pr(host->mode, ERROR, "Can not find clock source\n");
		ret = PTR_ERR(host->clk);
		goto probe_free_host;
	}

	host->rstc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(host->rstc)) {
		spsdc_pr(host->mode, ERROR, "Can not find reset controller\n");
		ret = PTR_ERR(host->rstc);
		goto probe_free_host;
	}

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(resource)) {
		spsdc_pr(host->mode, ERROR, "get sd register resource fail\n");
		ret = PTR_ERR(resource);
		goto probe_free_host;
	}

	if ((resource->end - resource->start + 1) < sizeof(*host->base)) {
		spsdc_pr(host->mode, ERROR, "register size is not right\n");
		ret = -EINVAL;
		goto probe_free_host;
	}

	host->base = devm_ioremap_resource(&pdev->dev, resource);
	if (IS_ERR((void *)host->base)) {
		spsdc_pr(host->mode, ERROR, "devm_ioremap_resource fail\n");
		ret = PTR_ERR((void *)host->base);
		goto probe_free_host;
	}

	host->irq = platform_get_irq(pdev, 0);
	if (host->irq <= 0) {
		spsdc_pr(host->mode, ERROR, "get sd irq resource fail\n");
		ret = -EINVAL;
		goto probe_free_host;
	}
	if (devm_request_irq(&pdev->dev, host->irq, spsdc_irq, IRQF_SHARED, dev_name(&pdev->dev), host)) {
		spsdc_pr(host->mode, ERROR, "Failed to request sd card interrupt.\n");
		ret = -ENOENT;
		goto probe_free_host;
	}
	spsdc_pr(host->mode, INFO, "spsdc driver probe, reg base:0x%08x, irq:%d\n", (unsigned int)(long)host->base, host->irq);

	host->dev = &pdev->dev;

	if(host->segs_no > SPSDC_HW_SEGS) {
		if(host->ram >= 8) {
			/*
			* When we just support one segment, we can get significant
			* speedups by the help of a bounce buffer to group scattered
			* reads/writes together.
			*/
			ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(33));
			if (ret)
				pr_warn("%s: Failed to set 32-bit DMA mask.\n", mmc_hostname(mmc));

			pdev->dev.coherent_dma_mask = DMA_BIT_MASK(33);
			pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
		}

		host->buffer = devm_kmalloc(&pdev->dev, SPSDC_MAX_BLK_CNT * 512, GFP_KERNEL | GFP_DMA);
		if (!host->buffer) {
			pr_err("%s: failed to allocate bytes for bounce buffer, falling back to single segments\n",
			mmc_hostname(mmc));
			/*
			* Exiting with zero here makes sure we proceed with
			* mmc->max_segs == 128.
			*/
			goto probe_free_host;
		}

		host->buf_phys_addr = dma_map_single(&pdev->dev,
					host->buffer,
					SPSDC_MAX_BLK_CNT * 512,
					DMA_BIDIRECTIONAL);

		ret = dma_mapping_error(&pdev->dev, host->buf_phys_addr);
		if (ret)
			goto probe_free_host;
	}

	ret = reset_control_assert(host->rstc);
	if (ret)
		goto probe_free_host;
	msleep(1);
	ret = reset_control_deassert(host->rstc);

	ret = clk_prepare_enable(host->clk);
	if (ret)
		goto probe_clk_unprepare;

	spin_lock_init(&host->lock);
	mutex_init(&host->mrq_lock);
	tasklet_init(&host->tsklet_finish_req, tsklet_func_finish_req, (unsigned long)host);
	dev_mode = of_device_get_match_data(&pdev->dev);
	host->vol_mode = dev_mode->vol_mode;
	spsdc_select_mode(host);
	host->soc_clk = dev_mode->source_clk;
	clk_set_rate(host->clk, host->soc_clk);
	host->work_clk = 0;
	host->timing = 10;
	if (!of_property_read_u32(pdev->dev.of_node, "delay-val", &host->val))
		host->val = host->val;
	else
		host->val = dev_mode->delay_val;

	mmc->ops = &spsdc_ops;
	mmc->f_min = SPSDC_MIN_CLK;
	mmc->ocr_avail |= MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	/* Host controller supports up to "SPSDC_MAX_DMA_MEMORY_SECTORS",
	 * a.k.a. max scattered memory segments per request
	 */
	mmc->max_segs = host->segs_no;
	//mmc->max_seg_size = SPSDC_MAX_BLK_CNT * 512;
	mmc->max_seg_size = SPSDC_MAX_BLK_CNT * 512;
	mmc->max_req_size = SPSDC_MAX_BLK_CNT * 512;
	mmc->max_blk_size = 512; /* Limited by the max value of dma_size & data_length, set it to 512 bytes for now */
	mmc->max_blk_count = SPSDC_MAX_BLK_CNT; /* Limited by sd_page_num */

	dev_set_drvdata(&pdev->dev, host);
	spsdc_controller_init(host);
	mmc_add_host(mmc);
	host->tuning_info.enable_tuning = 1;
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	return 0;

probe_clk_unprepare:
	spsdc_pr(host->mode, ERROR, "unable to enable controller clock\n");
	clk_unprepare(host->clk);
probe_free_host:
	if (mmc)
		mmc_free_host(mmc);

	return ret;
}

static int spsdc_drv_remove(struct platform_device *dev)
{
	struct spsdc_host *host = platform_get_drvdata(dev);

	spsdc_pr(host->mode, INFO, "%s\n", __func__);
	mmc_remove_host(host->mmc);
	clk_disable_unprepare(host->clk);
	reset_control_assert(host->rstc);
	pm_runtime_disable(&dev->dev);
	platform_set_drvdata(dev, NULL);
	mmc_free_host(host->mmc);

	return 0;
}


static int spsdc_drv_suspend(struct platform_device *dev, pm_message_t state)
{
	struct spsdc_host *host;

	host = platform_get_drvdata(dev);
	spsdc_pr(host->mode, INFO, "%s\n", __func__);
	mutex_lock(&host->mrq_lock); /* Make sure that no one is holding the controller */
	mutex_unlock(&host->mrq_lock);
	clk_disable_unprepare(host->clk);
	reset_control_assert(host->rstc);
	return 0;
}

static int spsdc_drv_resume(struct platform_device *dev)
{
	struct spsdc_host *host;

	host = platform_get_drvdata(dev);
	reset_control_deassert(host->rstc);
	clk_prepare_enable(host->clk);
	return 0;
}

#ifdef CONFIG_PM
#ifdef CONFIG_PM_SLEEP
static int spsdc_pm_suspend(struct device *dev)
{
	struct spsdc_host *host;

	host = dev_get_drvdata(dev);
	spsdc_pr(host->mode, INFO, "%s\n", __func__);
	pm_runtime_force_suspend(dev);
	clk_disable_unprepare(host->clk);	
	reset_control_assert(host->rstc);	
	return 0;
}

static int spsdc_pm_resume(struct device *dev)
{
	struct spsdc_host *host;
	int ret;
	
	host = dev_get_drvdata(dev);
	spsdc_pr(host->mode, INFO, "%s\n", __func__);	
	reset_control_deassert(host->rstc);
	ret = clk_prepare_enable(host->clk);
	if (ret)
		return ret;
	spsdc_controller_init(host);
	if (host->mmc->pm_flags & MMC_PM_KEEP_POWER){
		host->work_clk = 0;
		host->timing = 10;
		spsdc_set_ios(host->mmc, &host->mmc->ios);
		spmmc_start_signal_voltage_switch(host->mmc, &host->mmc->ios);
	}
	pm_runtime_force_resume(dev);
	return ret;
}
#endif /* ifdef CONFIG_PM_SLEEP */

#ifdef CONFIG_PM_RUNTIME_SD
static int spsdc_pm_runtime_suspend(struct device *dev)
{
	struct spsdc_host *host;

	host = dev_get_drvdata(dev);
	spsdc_pr(host->mode, INFO, "%s\n", __func__);
	if (__clk_is_enabled(host->clk))
		clk_disable_unprepare(host->clk);
	return 0;
}

static int spsdc_pm_runtime_resume(struct device *dev)
{
	struct spsdc_host *host;
	int ret = 0;

	host = dev_get_drvdata(dev);
	host->signal_voltage = 0;
	spsdc_pr(host->mode, INFO, "%s\n", __func__);
	if (!host->mmc)
		return -EINVAL;
	if (mmc_can_gpio_cd(host->mmc)) {
		ret = mmc_gpio_get_cd(host->mmc);
		if (!ret) {
			spsdc_pr(host->mode, DEBUG, "No card insert\n");
			return 0;
		}
	}
	return clk_prepare_enable(host->clk);
}
#endif /* ifdef CONFIG_PM_RUNTIME_SD */

static const struct dev_pm_ops spsdc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(spsdc_pm_suspend, spsdc_pm_resume)
#ifdef CONFIG_PM_RUNTIME_SD
	SET_RUNTIME_PM_OPS(spsdc_pm_runtime_suspend, spsdc_pm_runtime_resume, NULL)
#endif
};
#endif /* ifdef CONFIG_PM */


static struct platform_driver spsdc_driver = {
	.probe = spsdc_drv_probe,
	.remove = spsdc_drv_remove,
	.suspend = spsdc_drv_suspend,
	.resume = spsdc_drv_resume,
	.driver = {
		.name = "spsdc",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &spsdc_pm_ops,
#endif
		.of_match_table = spsdc_of_table,
	},
};
module_platform_driver(spsdc_driver);

MODULE_AUTHOR("lh.kuo <lh.kuo@sunplus.com>");
MODULE_DESCRIPTION("Sunplus SD/SDIO host controller v3.0 driver");
MODULE_LICENSE("GPL v2");
