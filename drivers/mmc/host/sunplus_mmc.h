/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * SD host controller v3.0
 */
#ifndef __SPMMC_H__
#define __SPMMC_H__

#include <linux/types.h>
#include <linux/spinlock_types.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>

#define MEASUREMENT_SIGNAL //timing measurement
//#define SPMMC_SOFTPAD
#define SPMMC_DMA_ALLOC //Enable:software workaround for sector=128, Disable: HW sector=8
#define SPMMC_HIGH_MEM //Enable:for DRAM size 4GB/8GB, Disable: for DRAM size 3.75GB

#define SPMMC_SUPPORT_VOLTAGE_1V8
#define SPMMC_EMMC_VCCQ_1V8
#define HS400

#define SPMMC_MIN_CLK	400000
#define SPMMC_MAX_CLK	400000000

#ifdef SPMMC_DMA_ALLOC
#define SPMMC_SOFTWAVE_MAX_SECTORS 128
#define SPMMC_MAX_BLK_COUNT 2560 /*2560*512=1310720Bytes, 1 sector size=1.3MB*/
#else
#define SPMMC_MAX_BLK_COUNT 65536 /*65536*512=33554432Bytes, 1 sector size=3.3MB*/
#endif
#define SPMMC_MAX_TUNABLE_DLY 7
#define SPMMC_SYS_CLK	360000000
#define __rsvd_regs(l) __append_suffix(l, __COUNTER__)
#define __append_suffix(l, s) _append_suffix(l, s)
#define _append_suffix(l, s) (reserved##s[l])

struct spmmc_regs {
#define SPMMC_MEDIA_NONE 0
#define SPMMC_MEDIA_SD 6
#define SPMMC_MEDIA_MS 7
	u32 card_mediatype_srcdst;
	u32 card_cpu_page_cnt;
	u32 sdram_sector_0_size;
	u32 dma_base_addr;
	u32 hw_dma_ctrl;
	u32 card_gclk_disable;
#define SPMMC_MAX_DMA_MEMORY_SECTORS 8 /* support up to 8 fragmented memory blocks */
	u32 sdram_sector_1_addr;
	u32 sdram_sector_1_size;
	u32 sdram_sector_2_addr;
	u32 sdram_sector_2_size;
	u32 sdram_sector_3_addr;
	u32 sdram_sector_3_size;
	u32 sdram_sector_4_addr;
	u32 sdram_sector_4_size;
	u32 sdram_sector_5_addr;
	u32 sdram_sector_5_size;
	u32 sdram_sector_6_addr;
	u32 sdram_sector_6_size;
	u32 sdram_sector_7_addr;
	u32 sdram_sector_7_size;
	u32 sdram_sector_cnt;
	u32 dma_hw_page_addr0;
	u32 dma_hw_page_addr1;
	u32 dma_hw_page_addr2;
	u32 dma_hw_page_addr3;
	u32 dma_hw_page_num0;
	u32 dma_hw_page_num1;
	u32 dma_hw_page_num2;
	u32 dma_hw_page_num3;
	u32 dma_hw_wait_num;
	u32 dma_hw_delay_num;
	u32 dma_debug;

	u32 boot_ctrl;
#define SPMMC_SWITCH_VOLTAGE_1V8_FINISH		1
#define SPMMC_SWITCH_VOLTAGE_1V8_ERROR		2
#define SPMMC_SWITCH_VOLTAGE_1V8_TIMEOUT	3
	u32 sd_vol_ctrl;
#define SPMMC_SDINT_SDCMPEN	BIT(0)
#define SPMMC_SDINT_SDCMP	BIT(1)
#define SPMMC_SDINT_SDIOEN	BIT(3)
#define SPMMC_SDINT_SDIO	BIT(4)
	u32 sd_int;
	u32 sd_page_num;
#define SPMMC_MODE_SDIO	2
#define SPMMC_MODE_EMMC	1
#define SPMMC_MODE_SD	0
	u32 sd_config0;
	u32 sdio_ctrl;
	u32 sd_rst;
	u32 sd_ctrl;
#define SPMMC_SDSTATUS_DUMMY_READY			BIT(0)
#define SPMMC_SDSTATUS_RSP_BUF_FULL			BIT(1)
#define SPMMC_SDSTATUS_TX_DATA_BUF_EMPTY		BIT(2)
#define SPMMC_SDSTATUS_RX_DATA_BUF_FULL			BIT(3)
#define SPMMC_SDSTATUS_CMD_PIN_STATUS			BIT(4)
#define SPMMC_SDSTATUS_DAT0_PIN_STATUS			BIT(5)
#define SPMMC_SDSTATUS_RSP_TIMEOUT			BIT(6)
#define SPMMC_SDSTATUS_CARD_CRC_CHECK_TIMEOUT		BIT(7)
#define SPMMC_SDSTATUS_STB_TIMEOUT			BIT(8)
#define SPMMC_SDSTATUS_RSP_CRC7_ERROR			BIT(9)
#define SPMMC_SDSTATUS_CRC_TOKEN_CHECK_ERROR		BIT(10)
#define SPMMC_SDSTATUS_RDATA_CRC16_ERROR		BIT(11)
#define SPMMC_SDSTATUS_SUSPEND_STATE_READY		BIT(12)
#define SPMMC_SDSTATUS_BUSY_CYCLE			BIT(13)
#define SPMMC_SDSTATUS_DAT1_PIN_STATUS			BIT(14)
#define SPMMC_SDSTATUS_SD_SENSE_STATUS			BIT(15)
#define SPMMC_SDSTATUS_BOOT_ACK_TIMEOUT			BIT(16)
#define SPMMC_SDSTATUS_BOOT_DATA_TIMEOUT		BIT(17)
#define SPMMC_SDSTATUS_BOOT_ACK_ERROR			BIT(18)
	u32 sd_status;
#define SPMMC_SDSTATE_IDLE	(0x0)
#define SPMMC_SDSTATE_TXDUMMY	(0x1)
#define SPMMC_SDSTATE_TXCMD	(0x2)
#define SPMMC_SDSTATE_RXRSP	(0x3)
#define SPMMC_SDSTATE_TXDATA	(0x4)
#define SPMMC_SDSTATE_RXCRC	(0x5)
#define SPMMC_SDSTATE_RXDATA	(0x6)
#define SPMMC_SDSTATE_MASK	(0x7)
#define SPMMC_SDSTATE_BADCRC	(0x5)
#define SPMMC_SDSTATE_ERROR	BIT(13)
#define SPMMC_SDSTATE_FINISH	BIT(14)
	u32 sd_state;
	u32 sd_hw_state;
	u32 sd_blocksize;
	u32 sd_config1;
	u32 sd_timing_config0;
	u32 sd_rx_data_tmr;
	u32 sd_piodatatx;
	u32 sd_piodatarx;
	u32 sd_cmdbuf0_3;
	u32 sd_cmdbuf4;
	u32 sd_rspbuf0_3;
	u32 sd_rspbuf4_5;
	u32 __rsvd_regs(32);
};

#ifdef MEASUREMENT_SIGNAL
union spmmc_reg_timing_config0 {
	u32 val;

	struct {
		u32 sd_clk_dly_sel	: 3;
		u32 resv1		: 1;
		u32 sd_wr_dat_dly_sel	: 3;
		u32 resv2		: 1;
		u32 sd_wr_cmd_dly_sel	: 3;
		u32 resv3		: 1;
		u32 sd_rd_rsp_dly_sel	: 3;
		u32 resv4		: 1;
		u32 sd_rd_dat_dly_sel	: 3;
		u32 resv5		: 1;
		u32 sd_rd_crc_dly_sel	: 3;
		u32 resv6		: 1;
	} bits;
};

union spmmc_reg_config0 {
	u32 val;

	struct {
		u32 sdpiomode		: 1;
		u32 sdddrmode		: 1;
		u32 sd_len_mode		: 1;
		u32 ddr_rx_first_hcyc	: 1;
		u32 sd_trans_mode	: 2;
		u32 sdautorsp		: 1;
		u32 sdcmddummy		: 1;
		u32 sdrspchk_en		: 1;
		u32 sdiomode		: 1;
		u32 sdmmcmode		: 1;
		u32 sddatawd		: 1;
		u32 sdrsptmren		: 1;
		u32 sdcrctmren		: 1;
		u32 rx4_en		: 1;
		u32 sdrsptype		: 1;
		u32 detect_tmr		: 2;
		u32 mmc8_en		: 1;
		u32 selci		: 1;
		u32 sdfqsel		: 12;
	} bits;
};

union spmmc_softpad_config {
	u32 val;
	struct {
		u32 clk_level	: 4;
		u32 data_level	: 2;
		u32 resv1		: 2;
		u32 resv2	: 2;
		u32 resv3		: 2;
	} bits;
};

struct spmmc_tuning_info {
	int enable_tuning;
	int need_tuning;
#ifdef SPMMC_SOFTPAD
	int tuning_finish;
	int softpad_tuning;
#endif
#define SPMMC_MAX_RETRIES (8 * 8)
	int retried; /* how many times has been retried */
	u32 rd_crc_dly:3;
	u32 rd_dat_dly:3;
	u32 rd_rsp_dly:3;
	u32 wr_cmd_dly:3;
	u32 wr_dat_dly:3;
	u32 clk_dly:3;
};

struct pad_ctl_regs {
	unsigned int pull_down_enable[4];  // 101.0 - 101.3
	unsigned int driving_selector0[4]; // 101.4 - 101.7
	unsigned int driving_selector1[4]; // 101.8 - 101.11
	unsigned int driving_selector2[4]; // 101.12 - 101.15
	unsigned int driving_selector3[4]; // 101.16 - 101.19
	unsigned int reserved_20[2];       // 101.20 - 101.21
	unsigned int sd_config[1];         // 101.22
	unsigned int sdio_config[1];       // 101.23
	unsigned int reserved_24[1];       // 101.24
	unsigned int gpio_first[4];        // 101.25
	unsigned int reserved_29[3];       // 101.29 - 101.31
};

struct pad_ctl2_regs {
	unsigned int emmc_sftpad_ctl[3];	// 102.21 - 102.23
};
#else
struct spmmc_tuning_info {
	int enable_tuning;
	int need_tuning;
#define SPMMC_MAX_RETRIES (8 * 8)
	int retried; /* how many times has been retried */
	u32 rd_crc_dly:3;
	u32 rd_dat_dly:3;
	u32 rd_rsp_dly:3;
	u32 wr_cmd_dly:3;
	u32 wr_dat_dly:3;
	u32 clk_dly:3;
};
#endif

struct spmmc_host {
	struct spmmc_regs *base;
#ifdef MEASUREMENT_SIGNAL
	struct pad_ctl_regs *pad_base;
	struct pad_ctl2_regs *pad_ctl2_base;
#endif

	struct clk *clk;
	struct reset_control *rstc;
	int mode; /* SD/SDIO/eMMC */
#ifdef MEASUREMENT_SIGNAL
	u32 mmc_no;
	u32 driving;
#endif
	spinlock_t lock; /* controller lock */
	struct mutex mrq_lock;
	/* tasklet used to handle error then finish the request */
	struct tasklet_struct tsklet_finish_req;
	struct mmc_host *mmc;
	struct mmc_request *mrq; /* current mrq */

	int irq;
	int use_int; /* should raise irq when done? */
	int power_state; /* current power state: off/up/on */
	int ddr_enabled;
	int signal_voltage;

#define SPMMC_DMA_MODE 0
#define SPMMC_PIO_MODE 1
	int dmapio_mode;
	/* for purpose of reducing context switch, only when transfer data that*/
	/* length is greater than `dma_int_threshold' should use interrupt */
	int dma_int_threshold;
	struct sg_mapping_iter sg_miter; /* for pio mode to access sglist */
	int dma_use_int; /* should raise irq when dma done */
	struct spmmc_tuning_info tuning_info;

#ifdef SPMMC_DMA_ALLOC
	struct device		*dev;
	struct mmc_data	*data;
	unsigned int		*buffer;
	unsigned int		buf_size;
	unsigned int		xfer_len;
	dma_addr_t		buf_phys_addr;
	dma_addr_t		buf_addr;
#endif
};

#endif /* #ifndef __SPMMC_H__ */
