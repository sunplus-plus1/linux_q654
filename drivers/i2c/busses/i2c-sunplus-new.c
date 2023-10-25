// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Sunplus Inc.
 * Author: Li-hao Kuo <lhjeff911@gmail.com>
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>

#define SP_I2C_STD_FREQ				100
#define SP_I2C_FAST_FREQ			400
#define SP_I2C_SLEEP_TIMEOUT			200
#define SP_I2C_SCL_DELAY			1
#define SP_CLK_SOURCE_FREQ			27000
#define SP_I2C_EMP_THOLD			4

#define SP_I2C_BURST_RDATA_BYTES		16
#define SP_I2C_BURST_READ			4
#define SP_I2C_BURST_RDATA_FLG			(BIT(31) | BIT(15))
#define SP_I2C_BURST_RDATA_ALL_FLG		GENMASK(31, 0)

#define SP_I2C_CTL0_REG				0x0000
//control0
#define SP_I2C_CTL0_FREQ_MASK			GENMASK(26, 24)
#define SP_I2C_CTL0_PREFETCH			BIT(18)
#define SP_I2C_CTL0_RESTART_EN			BIT(17)
#define SP_I2C_CTL0_SUBADDR_EN			BIT(16)
#define SP_I2C_CTL0_SW_RESET			BIT(15)
#define SP_I2C_CTL0_SLAVE_ADDR_MASK		GENMASK(7, 1)

#define SP_I2C_CTL1_REG				0x0004
//control1
#define SP_I2C_CTL1_ALL_CLR			GENMASK(9, 0)
#define SP_I2C_CTL1_EMP_CLR			BIT(9)
#define SP_I2C_CTL1_SCL_HOLD_TOO_LONG_CLR	BIT(8)
#define SP_I2C_CTL1_SCL_WAIT_CLR		BIT(7)
#define SP_I2C_CTL1_EMP_THOLD_CLR		BIT(6)
#define SP_I2C_CTL1_DATA_NACK_CLR		BIT(5)
#define SP_I2C_CTL1_ADDR_NACK_CLR		BIT(4)
#define SP_I2C_CTL1_BUSY_CLR			BIT(3)
#define SP_I2C_CTL1_CLKERR_CLR			BIT(2)
#define SP_I2C_CTL1_DONE_CLR			BIT(1)
#define SPI2C_CTL1_SIFBUSY_CLR			BIT(0)

#define SP_I2C_CTL2_REG				0x0008
//control2
#define SP_I2C_CTL2_FREQ_CUSTOM_MASK		GENMASK(10, 0)  // 11 bit
#define SP_I2C_CTL2_SCL_DELAY_MASK		GENMASK(25, 24)
#define SP_I2C_CTL2_SDA_HALF_EN			BIT(31)

#define SP_I2C_INT_REG				0x001c
#define SP_I2C_INT_EN0_REG			0x0020
#define SP_I2C_MOD_REG				0x0024
#define SP_I2C_CTL6_REG				0x0030
#define SP_I2C_INT_EN1_REG			0x0034
#define SP_I2C_STATUS3_REG			0x0038
#define SP_I2C_STATUS4_REG			0x0040
#define SP_I2C_INT_EN2_REG			0x0040
#define SP_I2C_CTL7_REG				0x0044
//control7
#define SP_I2C_CTL7_RD_MASK			GENMASK(31, 16)
#define SP_I2C_CTL7_WR_MASK			GENMASK(15, 0)  //bit[31:16]
//i2c master mode
#define SP_I2C_MODE_MASK			GENMASK(2, 0)  //bit[31:16]
#define SP_I2C_MODE_DMA_MODE			BIT(2)
#define SP_I2C_MODE_MANUAL_MODE			BIT(1)
#define SP_I2C_MODE_MANUAL_TRIG			BIT(0)

#define SP_I2C_DATA0_REG			0x0060

#define SP_I2C_DMA_CONF_REG			0x0084
#define SP_I2C_DMA_LEN_REG			0x0088
#define SP_I2C_DMA_ADDR_REG			0x008c
//dma config
#define SP_I2C_DMA_CFG_DMA_GO			BIT(8)
#define SP_I2C_DMA_CFG_NON_BUF_MODE		BIT(2)
#define SP_I2C_DMA_CFG_SAME_SLAVE		BIT(1)
#define SP_I2C_DMA_CFG_DMA_MODE			BIT(0)

#define SP_I2C_DMA_FLAG_REG			0x0094
//dma interrupt flag
#define SP_I2C_DMA_INT_LEN0_FLG			BIT(6)
#define SP_I2C_DMA_INT_THOLD_FLG		BIT(5)
#define SP_I2C_DMA_INT_IP_TMO_FLG		BIT(4)
#define SP_I2C_DMA_INT_GDMA_TMO_FLG		BIT(3)
#define SP_I2C_DMA_INT_WB_EN_ERR_FLG		BIT(2)
#define SP_I2C_DMA_INT_WCNT_ERR_FLG		BIT(1)
#define SP_I2C_DMA_INT_DMA_DONE_FLG		BIT(0)

#define SP_I2C_DMA_INT_EN_REG			0x0098
//dma interrupt enable
#define SP_I2C_DMA_EN_LENGTH0_INT		BIT(6)
#define SP_I2C_DMA_EN_THOLD_IN			BIT(5)
#define SP_I2C_DMA_EN_IP_TMO_INT		BIT(4)
#define SP_I2C_DMA_EN_GDMA_TMO_INT		BIT(3)
#define SP_I2C_DMA_EN_WB_EN_ERR_INT		BIT(2)
#define SP_I2C_DMA_EN_WCNT_ERR_INT		BIT(1)
#define SP_I2C_DMA_EN_DMA_DONE_INT		BIT(0)
//interrupt
#define SP_I2C_INT_RINC_INDEX			GENMASK(20, 18) //bit[20:18]
#define SP_I2C_INT_WINC_INDEX			GENMASK(17, 15) //bit[17:15]
#define SP_I2C_INT_SCL_HOLD_TOO_LONG_FLG	BIT(11)
#define SP_I2C_INT_WFIFO_EN			BIT(10)
#define SP_I2C_INT_FULL_FLG			BIT(9)
#define SP_I2C_INT_EMP_FLG			BIT(8)
#define SP_I2C_INT_SCL_WAIT_FLG			BIT(7)
#define SP_I2C_INT_EMPTY_THOLD_FLG		BIT(6)
#define SP_I2C_INT_DATA_NACK_FLG		BIT(5)
#define SP_I2C_INT_ADDR_NACK_FLG		BIT(4)
#define SP_I2C_INT_BUSY_FLG			BIT(3)
#define SP_I2C_INT_CLKERR_FLG			BIT(2)
#define SP_I2C_INT_DONE_FLG			BIT(1)
#define SP_I2C_INT_SIFBUSY_FLG			BIT(0)
//interrupt enable0
#define SP_I2C_EN0_SCL_HOLD_TOO_LONG_INT	BIT(13)
#define SP_I2C_EN0_NACK_INT			BIT(12)
#define SP_I2C_EN0_CTL_EMP_THOLD		GENMASK(11, 9)
#define SP_I2C_EN0_EMP_INT			BIT(8)
#define SP_I2C_EN0_SCL_WAIT_INT			BIT(7)
#define SP_I2C_EN0_EMP_THOLD_INT		BIT(6)
#define SP_I2C_EN0_DATA_NACK_INT		BIT(5)
#define SP_I2C_EN0_ADDR_NACK_INT		BIT(4)
#define SP_I2C_EN0_BUSY_INT			BIT(3)
#define SP_I2C_EN0_CLKERR_INT			BIT(2)
#define SP_I2C_EN0_DONE_INT			BIT(1)
#define SP_I2C_EN0_SIFBUSY_INT			BIT(0)

#define SP_I2C_RESET(id, val)			(((1 << 16) | (val)) << (id))
#define SP_I2C_CLKEN(id, val)			(((1 << 16) | (val)) << (id))
#define SP_I2C_GCLKEN(id, val)			(((1 << 16) | (val)) << (id))

#define SP_I2C_POWER_CLKEN3			0x0010
#define SP_I2C_POWER_GCLKEN3			0x0038
#define SP_I2C_POWER_RESET3			0x0060

enum sp_state_e_ {
	SPI2C_SUCCESS = 0,	/* successful */
	SPI2C_STATE_WR = 1,	/* i2c is write */
	SPI2C_STATE_RD = 2,	/* i2c is read */
	SPI2C_STATE_IDLE = 3,	/* i2c is idle */
	SPI2C_STATE_DMA_WR = 4,	/* i2c is dma write */
	SPI2C_STATE_DMA_RD = 5,	/* i2c is dma read */
};

enum sp_i2c_switch_e_ {
	SP_I2C_DMA_POW_NO_SW = 0,
	SP_I2C_DMA_POW_SW = 1,
};

enum sp_i2c_xfer_mode {
	I2C_PIO_MODE = 0,
	I2C_DMA_MODE = 1,
};

struct sp_i2c_cmd {
	unsigned int xfer_mode;
	unsigned int freq;
	unsigned int slv_addr;
	unsigned int restart_en;
	unsigned int xfer_cnt;
	unsigned int restart_write_cnt;
	unsigned char *write_data;
	unsigned char *read_data;
	dma_addr_t dma_w_addr;
	dma_addr_t dma_r_addr;
};

struct sp_i2c_irq_event {
	enum sp_state_e_ rw_state;
	unsigned int burst_cnt;
	unsigned int data_total_len;
	unsigned char busy;
	unsigned char dma_done;
	unsigned char active_done;
	unsigned char ret;
	unsigned char *data_buf;
	unsigned int int_flg;
	unsigned int int_dma_flg;
	unsigned int overflow_flg;	
};

enum sp_i2c_dma_mode {
	I2C_DMA_WRITE_MODE,
	I2C_DMA_READ_MODE,
};

enum sp_i2c_mode {
	I2C_WRITE_MODE,
	I2C_READ_MODE,
	I2C_RESTART_MODE,
};

struct i2c_compatible {
	int mode; /* dma power switch*/
};

struct sp_i2c_dev {
	struct device *dev;
	struct i2c_adapter adap;
	struct sp_i2c_cmd spi2c_cmd;
	struct sp_i2c_irq_event spi2c_irq;
	struct clk *clk;
	struct reset_control *rstc;
	unsigned int mode;
	unsigned int i2c_clk_freq;
	int irq;
	void __iomem *i2c_regs;
	void __iomem *i2c_power_regs;
	wait_queue_head_t wait;
};

static unsigned int sp_i2cm_get_int_flag(void __iomem *sr)
{
	return readl(sr + SP_I2C_INT_REG);
}

static void sp_i2cm_status_clear(void __iomem *sr, u32 flag)
{
	u32 ctl1;

	ctl1 = readl(sr + SP_I2C_CTL1_REG);
	ctl1 |= flag;
	writel(ctl1, sr + SP_I2C_CTL1_REG);

	ctl1 = readl(sr + SP_I2C_CTL1_REG);
	ctl1 &= (~flag);
	writel(ctl1, sr + SP_I2C_CTL1_REG);
}

static void sp_i2cm_reset(void __iomem *sr)
{
	u32 ctl0;

	ctl0 = readl(sr + SP_I2C_CTL0_REG);
	ctl0 |= SP_I2C_CTL0_SW_RESET;
	writel(ctl0, sr + SP_I2C_CTL0_REG);

	udelay(2);
}

static unsigned int sp_i2cm_over_flag_get(void __iomem *sr)
{
	return readl(sr + SP_I2C_STATUS4_REG);
}

static void sp_i2cm_data_get(void __iomem *sr, unsigned int index, void *rxdata)
{
	unsigned int *rdata = rxdata;

	*rdata = readl(sr + SP_I2C_DATA0_REG + (index * 4));
}

static void sp_i2cm_addr_freq_set(void __iomem *sr, unsigned int freq, unsigned int addr)
{
	unsigned int div;
	u32 ctl0, ctl2;

	div = SP_CLK_SOURCE_FREQ / freq;
	div -= 1;
	if (SP_CLK_SOURCE_FREQ % freq != 0)
		div += 1;

	if (div > SP_I2C_CTL2_FREQ_CUSTOM_MASK)
		div = SP_I2C_CTL2_FREQ_CUSTOM_MASK;

	ctl0 = readl(sr + SP_I2C_CTL0_REG);
	ctl0 &= ~SP_I2C_CTL0_FREQ_MASK;
	ctl0 &= ~SP_I2C_CTL0_SLAVE_ADDR_MASK;
	ctl0 |= FIELD_PREP(SP_I2C_CTL0_SLAVE_ADDR_MASK, addr);
	writel(ctl0, sr + SP_I2C_CTL0_REG);

	ctl2 = readl(sr + SP_I2C_CTL2_REG);
	ctl2 &= ~SP_I2C_CTL2_FREQ_CUSTOM_MASK;
	ctl2 |= FIELD_PREP(SP_I2C_CTL2_FREQ_CUSTOM_MASK, div);
	writel(ctl2, sr + SP_I2C_CTL2_REG);
}

static void sp_i2cm_scl_delay_set(void __iomem *sr, unsigned int delay)
{
	u32 ctl2;

	ctl2 = readl(sr + SP_I2C_CTL2_REG);
	ctl2 &= ~SP_I2C_CTL2_SCL_DELAY_MASK;
	ctl2 |= FIELD_PREP(SP_I2C_CTL2_SCL_DELAY_MASK, delay);
	ctl2 &= ~SP_I2C_CTL2_SDA_HALF_EN;
	writel(ctl2, sr + SP_I2C_CTL2_REG);
}

static void sp_i2cm_trans_cnt_set(void __iomem *sr, unsigned int write_cnt,
				  unsigned int read_cnt)
{
	u32 ctl7 = 0;

	ctl7 = FIELD_PREP(SP_I2C_CTL7_RD_MASK, read_cnt) |
		FIELD_PREP(SP_I2C_CTL7_WR_MASK, write_cnt);
	writel(ctl7, sr + SP_I2C_CTL7_REG);
}

static void sp_i2cm_active_mode_set(void __iomem *sr, enum sp_i2c_xfer_mode mode)
{
	u32 val;

	val = readl(sr + SP_I2C_MOD_REG);
	val &= ~SP_I2C_MODE_MASK;
	if(mode == I2C_DMA_MODE)
		val |= (SP_I2C_MODE_MANUAL_MODE | SP_I2C_MODE_DMA_MODE);

	writel(val, sr + SP_I2C_MOD_REG);
}

static void sp_i2cm_data_tx(void __iomem *sr, void *wdata, unsigned int cnt)
{
	unsigned int *wrdata = wdata;
	unsigned int i;

	for (i = 0 ; i < cnt ; i++)
		writel(wrdata[i], sr + SP_I2C_DATA0_REG + (i * 4));
}

static void sp_i2cm_rw_mode_set(void __iomem *sr, enum sp_i2c_mode rw_mode)
{
	u32 ctl0;

	ctl0 = readl(sr + SP_I2C_CTL0_REG);
	switch (rw_mode) {
	default:
	case I2C_WRITE_MODE:
		ctl0 &= ~(SP_I2C_CTL0_PREFETCH |
			  SP_I2C_CTL0_RESTART_EN | SP_I2C_CTL0_SUBADDR_EN);
		break;
	case I2C_READ_MODE:
		ctl0 &= (~(SP_I2C_CTL0_RESTART_EN | SP_I2C_CTL0_SUBADDR_EN));
		ctl0 |= SP_I2C_CTL0_PREFETCH;
		break;
	case I2C_RESTART_MODE:
		ctl0 |= (SP_I2C_CTL0_PREFETCH |
			 SP_I2C_CTL0_RESTART_EN | SP_I2C_CTL0_SUBADDR_EN);
		break;
	}
	writel(ctl0, sr + SP_I2C_CTL0_REG);
}

static void sp_i2cm_int_set(void __iomem *sr, u32 int0, u32 rdata_en, u32 overflow_en)
{
	if(int0 != 0)
		writel(int0, sr + SP_I2C_INT_EN0_REG);
	if(rdata_en != 0)
		writel(rdata_en, sr + SP_I2C_INT_EN1_REG);
	if(overflow_en != 0)
		writel(overflow_en, sr + SP_I2C_INT_EN2_REG);
}

static void sp_i2cm_enable(unsigned int device_id, void *membase)
{
	writel(SP_I2C_CLKEN(device_id, 1),  membase + SP_I2C_POWER_CLKEN3);
	writel(SP_I2C_GCLKEN(device_id, 0), membase + SP_I2C_POWER_GCLKEN3);
	writel(SP_I2C_RESET(device_id, 0), membase + SP_I2C_POWER_RESET3);
}

static void sp_i2cm_manual_trigger(void __iomem *sr)
{
	u32 val;

	val = readl(sr + SP_I2C_MOD_REG);
	val |= SP_I2C_MODE_MANUAL_TRIG;
	writel(val, sr + SP_I2C_MOD_REG);
}

static unsigned int sp_i2cm_get_dma_int_flag(void __iomem *sr_dma)
{
	return readl(sr_dma + SP_I2C_DMA_FLAG_REG);
}

static void sp_i2cm_dma_int_flag_clear(void __iomem *sr_dma, unsigned int flag)
{
	u32 val;

	val = readl(sr_dma + SP_I2C_DMA_FLAG_REG);
	val |= flag;
	writel(val, sr_dma + SP_I2C_DMA_FLAG_REG);
}

static void sp_i2cm_dma_addr_set(void __iomem *sr_dma, dma_addr_t addr)
{
	writel(addr, sr_dma + SP_I2C_DMA_ADDR_REG);
}

static void sp_i2cm_dma_len_set(void __iomem *sr_dma, unsigned int len)
{
	len &= (0xFFFF);  //only support 16 bit
	writel(len, sr_dma + SP_I2C_DMA_LEN_REG);
}

static void sp_i2cm_dma_rw_mode_set(void __iomem *sr_dma, enum sp_i2c_dma_mode rw_mode)
{
	u32 val;

	val = readl(sr_dma + SP_I2C_DMA_CONF_REG);
	switch (rw_mode) {
	default:
	case I2C_DMA_WRITE_MODE:
		val |= SP_I2C_DMA_CFG_DMA_MODE;
		break;
	case I2C_DMA_READ_MODE:
		val &= (~SP_I2C_DMA_CFG_DMA_MODE);
		break;
	}
	writel(val, sr_dma + SP_I2C_DMA_CONF_REG);
}

static void sp_i2cm_dma_int_en_set(void __iomem *sr_dma, unsigned int dma_int)
{
	writel(dma_int, sr_dma + SP_I2C_DMA_INT_EN_REG);
}

static void sp_i2cm_dma_go_set(void __iomem *sr_dma)
{
	u32 val;

	val = readl(sr_dma + SP_I2C_DMA_CONF_REG);
	val |= SP_I2C_DMA_CFG_DMA_GO;
	writel(val, sr_dma + SP_I2C_DMA_CONF_REG);
}

static void _sp_i2cm_intflag_check(struct sp_i2c_dev *spi2c)
{
	void __iomem *sr = spi2c->i2c_regs;
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;

	spi2c_irq->int_flg = sp_i2cm_get_int_flag(sr);

	if (spi2c_irq->int_flg & SP_I2C_INT_DONE_FLG)
		spi2c_irq->active_done = 1;
	else
		spi2c_irq->active_done = 0;

	sp_i2cm_status_clear(sr, SP_I2C_CTL1_ALL_CLR);
	spi2c_irq->overflow_flg = sp_i2cm_over_flag_get(sr);
}

static unsigned int _sp_i2cm_err_check(struct sp_i2c_dev *spi2c)
{
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;
	unsigned int int_flg = spi2c_irq->int_dma_flg;
	unsigned int ret = 0;

	if ((int_flg & SP_I2C_INT_ADDR_NACK_FLG) || (int_flg & SP_I2C_INT_DATA_NACK_FLG)) {
		if (spi2c_irq->rw_state == SPI2C_STATE_DMA_WR)
			dev_dbg(spi2c->dev, "DMA write NACK!\n");
		else if (spi2c_irq->rw_state == SPI2C_STATE_DMA_RD)
			dev_dbg(spi2c->dev, "DMA read NACK!!\n");
		else {
			if (int_flg & SP_I2C_INT_ADDR_NACK_FLG)
				dev_dbg(spi2c->dev, " Addr NACK!\n");
			if (int_flg & SP_I2C_INT_DATA_NACK_FLG)
				dev_dbg(spi2c->dev, " Data NACK!\n");
		}
		ret = -ENXIO;
	} else if (spi2c_irq->int_flg & SP_I2C_INT_EMPTY_THOLD_FLG) {
		dev_dbg(spi2c->dev, " empty hold !\n");
		ret =  -EINVAL;
	} else if (spi2c_irq->int_flg & SP_I2C_INT_EMP_FLG) {
		dev_dbg(spi2c->dev, " empty !\n");
		ret =  -EINVAL;
	} else if (spi2c_irq->int_flg & SP_I2C_INT_SCL_HOLD_TOO_LONG_FLG) {
		dev_dbg(spi2c->dev, " hold too long !\n");
		ret =  -EINVAL;
	} else if (spi2c_irq->overflow_flg) {
		dev_dbg(spi2c->dev, " overflow !\n");
		ret =  -EINVAL;
	}
	if(ret != 0) {
		spi2c_irq->dma_done = 1;
		spi2c_irq->active_done = 1;
		wake_up(&spi2c->wait);
	}
	return  ret;
}

static void _sp_i2cm_dma_intflag_check(struct sp_i2c_dev *spi2c)
{
	void __iomem *sr_dma = spi2c->i2c_regs;
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;

	spi2c_irq->int_dma_flg = sp_i2cm_get_dma_int_flag(sr_dma);

	if (spi2c_irq->int_dma_flg & SP_I2C_DMA_INT_DMA_DONE_FLG)
		spi2c_irq->dma_done = 1;
	else
		spi2c_irq->dma_done = 0;
	sp_i2cm_dma_int_flag_clear(sr_dma, 0x7F);  //write 1 to clear
}

static unsigned int _sp_i2cm_dma_err_check(struct sp_i2c_dev *spi2c)
{
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;
	unsigned int int_flg = spi2c_irq->int_dma_flg;
	unsigned int ret = 0;

	if (int_flg & SP_I2C_DMA_INT_WCNT_ERR_FLG) {
		dev_dbg(spi2c->dev, "DMA write cnt err!\n");
		ret = -EINVAL;
	}else if (int_flg & SP_I2C_DMA_INT_WB_EN_ERR_FLG) {
		dev_dbg(spi2c->dev, "DMA WB en err!\n");
		ret = -EINVAL;
	}else if (int_flg & SP_I2C_DMA_INT_GDMA_TMO_FLG) {
		dev_dbg(spi2c->dev, "DMA GDMA_TMO!\n");
		ret = -EINVAL;
	}else if (int_flg & SP_I2C_DMA_INT_IP_TMO_FLG) {
		dev_dbg(spi2c->dev, "DMA IP_TMO!\n");
		ret = -EINVAL;
	}else if (int_flg & SP_I2C_DMA_INT_LEN0_FLG) {
		dev_dbg(spi2c->dev, "INT_LEN0!\n");
		ret = -EINVAL;
	}
	if(ret != 0) {
		spi2c_irq->dma_done = 1;
		spi2c_irq->active_done = 1;
		wake_up(&spi2c->wait);
	}
	return  ret;
}

static irqreturn_t _sp_i2cm_irqevent_handler(int irq, void *args)
{
	struct sp_i2c_dev *spi2c = args;
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;
	void __iomem *sr = spi2c->i2c_regs;
	unsigned char r_data[SP_I2C_BURST_RDATA_BYTES] = {0};
	int i = 0;

	if (spi2c_irq->rw_state >= SPI2C_STATE_DMA_WR) {
	_sp_i2cm_dma_intflag_check(spi2c);
	if (spi2c_irq->dma_done) {
		spi2c_irq->ret = SPI2C_SUCCESS;
		if(spi2c_irq->rw_state == SPI2C_STATE_DMA_RD){
			wake_up(&spi2c->wait);
			return IRQ_HANDLED;
		}
	}else {
		spi2c_irq->ret = _sp_i2cm_dma_err_check(spi2c);
		if(spi2c_irq->ret != 0)
			return IRQ_HANDLED;
	}
	}

	_sp_i2cm_intflag_check(spi2c);
	if (spi2c_irq->active_done) {
		if(spi2c_irq->rw_state == SPI2C_STATE_RD) {
			for (i = 0; i < spi2c_irq->burst_cnt; i++) {
				sp_i2cm_data_get(sr, i, &r_data[i*4]);
			}
			for (i = 0; i < spi2c_irq->data_total_len; i++) {
				spi2c_irq->data_buf[i] = r_data[i];
			}
		}
		if(spi2c_irq->rw_state != SPI2C_STATE_DMA_RD) {
			spi2c_irq->ret = SPI2C_SUCCESS;
			wake_up(&spi2c->wait);
		}
	} else 
		spi2c_irq->ret = _sp_i2cm_err_check(spi2c);

	return IRQ_HANDLED;
}

static int sp_i2cm_read(struct sp_i2c_cmd *spi2c_cmd, struct sp_i2c_dev *spi2c)
{
	void __iomem *sr = spi2c->i2c_regs;
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;
	unsigned int int0 = 0, int1 = 0, int2 = 0;
	unsigned int write_cnt = 0;
	unsigned int read_cnt = 0;
	int ret = SPI2C_SUCCESS;

	if (spi2c_irq->busy) {
		dev_err(spi2c->dev, "IO error !!\n");
		return -ENXIO;
	}

	//pr_info("sp_i2cm_read %d\n",spi2c_cmd->xfer_cnt);
	sp_i2cm_reset(sr);
	memset(spi2c_irq, 0, sizeof(*spi2c_irq));
	spi2c_irq->busy = 1;

	read_cnt = spi2c_cmd->xfer_cnt;

	if (spi2c_cmd->restart_en) {
		if (write_cnt > 32) {
			spi2c_irq->busy = 0;
			dev_err(spi2c->dev,
				"I2C write count is invalid !! write count=%d\n", write_cnt);
			return -EINVAL;
		}
		write_cnt = spi2c_cmd->restart_write_cnt;
	}

	spi2c_irq->burst_cnt = read_cnt / SP_I2C_BURST_READ;
	if(read_cnt % SP_I2C_BURST_READ)
		spi2c_irq->burst_cnt++;

	int0 = (SP_I2C_EN0_SCL_HOLD_TOO_LONG_INT | SP_I2C_EN0_EMP_INT | SP_I2C_EN0_DATA_NACK_INT
		| SP_I2C_EN0_ADDR_NACK_INT | SP_I2C_EN0_DONE_INT);

	spi2c_irq->rw_state = SPI2C_STATE_RD;
	spi2c_irq->data_total_len = read_cnt;
	spi2c_irq->data_buf = spi2c_cmd->read_data;

	sp_i2cm_addr_freq_set(sr, spi2c_cmd->freq, spi2c_cmd->slv_addr);
	sp_i2cm_scl_delay_set(sr, SP_I2C_SCL_DELAY);
	sp_i2cm_trans_cnt_set(sr, write_cnt, read_cnt);
	sp_i2cm_active_mode_set(sr, spi2c_cmd->xfer_mode);

	if (spi2c_cmd->restart_en) {
		write_cnt = spi2c_cmd->restart_write_cnt / 4;
		if (spi2c_cmd->restart_write_cnt % 4)
			write_cnt += 1;

		sp_i2cm_data_tx(sr, spi2c_cmd->write_data, write_cnt);
		sp_i2cm_rw_mode_set(sr, I2C_RESTART_MODE);
	} else {
		sp_i2cm_rw_mode_set(sr, I2C_READ_MODE);
	}

	sp_i2cm_int_set(sr, int0, int1, int2);
	sp_i2cm_manual_trigger(sr);	//start send data

	ret = wait_event_timeout(spi2c->wait, spi2c_irq->active_done,
				 (SP_I2C_SLEEP_TIMEOUT * HZ) / 500);
	if (ret == 0) {
		dev_err(spi2c->dev, "I2C read timeout !!\n");
		ret = -ETIMEDOUT;
	} else {
		ret = spi2c_irq->ret;
	}
	sp_i2cm_reset(sr);
	spi2c_irq->rw_state = SPI2C_STATE_IDLE;
	spi2c_irq->busy = 0;

	return ret;
}

static int sp_i2cm_write(struct sp_i2c_cmd *spi2c_cmd, struct sp_i2c_dev *spi2c)
{
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;
	void __iomem *sr = spi2c->i2c_regs;
	unsigned int write_cnt = 0;
	unsigned char w_data[SP_I2C_BURST_RDATA_BYTES] = {0};
	unsigned int int0 = 0;
	int ret = SPI2C_SUCCESS;
	int i = 0;

	if (spi2c_irq->busy) {
		dev_err(spi2c->dev, "IO error !!\n");
		return -ENXIO;
	}

	//pr_info("sp_i2cm_wr %d\n",spi2c_cmd->xfer_cnt);
	sp_i2cm_reset(sr);
	memset(spi2c_irq, 0, sizeof(*spi2c_irq));
	spi2c_irq->busy = 1;
	write_cnt = spi2c_cmd->xfer_cnt;

	spi2c_irq->burst_cnt = write_cnt  / SP_I2C_BURST_READ;
	if (write_cnt % SP_I2C_BURST_READ)
		spi2c_irq->burst_cnt += 1;
	for(i = 0; i < write_cnt; i++){
		w_data[i] = spi2c_cmd->write_data[i];
	}

	int0 = (SP_I2C_EN0_SCL_HOLD_TOO_LONG_INT | SP_I2C_EN0_EMP_INT | SP_I2C_EN0_DATA_NACK_INT
		| SP_I2C_EN0_ADDR_NACK_INT | SP_I2C_EN0_DONE_INT);
	//if (spi2c_irq->burst_cnt)
	//	int0 |= SP_I2C_EN0_EMP_THOLD_INT;

	spi2c_irq->rw_state = SPI2C_STATE_WR;
	spi2c_irq->data_total_len = write_cnt;
	spi2c_irq->data_buf = spi2c_cmd->write_data;

	sp_i2cm_addr_freq_set(sr, spi2c_cmd->freq, spi2c_cmd->slv_addr);
	sp_i2cm_scl_delay_set(sr, SP_I2C_SCL_DELAY);
	sp_i2cm_trans_cnt_set(sr, write_cnt, 0);
	sp_i2cm_active_mode_set(sr, spi2c_cmd->xfer_mode);
	sp_i2cm_rw_mode_set(sr, I2C_WRITE_MODE);
	sp_i2cm_data_tx(sr, &w_data[0], spi2c_irq->burst_cnt);
	sp_i2cm_int_set(sr, int0, 0, 0);

	sp_i2cm_manual_trigger(sr);	//start send data

	ret = wait_event_timeout(spi2c->wait, spi2c_irq->active_done,
				 (SP_I2C_SLEEP_TIMEOUT * HZ) / 500);
	if (ret == 0) {
		dev_err(spi2c->dev, "I2C write timeout !!\n");
		ret = -ETIMEDOUT;
	} else {
		ret = spi2c_irq->ret;
	}
	sp_i2cm_reset(sr);
	spi2c_irq->rw_state = SPI2C_STATE_IDLE;
	spi2c_irq->busy = 0;

	return ret;
}

static int sp_i2cm_dma_write(struct sp_i2c_cmd *spi2c_cmd, struct sp_i2c_dev *spi2c, struct i2c_msg *msgs)
{
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;
	void __iomem *sr = spi2c->i2c_regs;
	unsigned int int0 = 0;
	unsigned int dma_int = 0;
	int ret = SPI2C_SUCCESS;

	if (spi2c_irq->busy) {
		dev_err(spi2c->dev, "IO error !!\n");
		return -ENXIO;
	}

	if (spi2c->mode == SP_I2C_DMA_POW_SW && spi2c->i2c_power_regs != 0)
		sp_i2cm_enable(0, spi2c->i2c_power_regs);

	sp_i2cm_reset(sr);
	memset(spi2c_irq, 0, sizeof(*spi2c_irq));
	spi2c_irq->busy = 1;

	if (spi2c_cmd->xfer_cnt > 0xFFFF) {
		spi2c_irq->busy = 0;
		dev_err(spi2c->dev, "write count = %d is invalid!\n", spi2c_cmd->xfer_cnt);
		return -EINVAL;
	}

	spi2c_irq->rw_state = SPI2C_STATE_DMA_WR;

	int0 = (SP_I2C_EN0_SCL_HOLD_TOO_LONG_INT | SP_I2C_EN0_EMP_INT
		| SP_I2C_EN0_DATA_NACK_INT | SP_I2C_EN0_ADDR_NACK_INT | SP_I2C_EN0_DONE_INT);
	dma_int = SP_I2C_DMA_EN_DMA_DONE_INT;

	sp_i2cm_addr_freq_set(sr, spi2c_cmd->freq, spi2c_cmd->slv_addr);
	sp_i2cm_scl_delay_set(sr, SP_I2C_SCL_DELAY);
	sp_i2cm_active_mode_set(sr, spi2c_cmd->xfer_mode);
	sp_i2cm_rw_mode_set(sr, I2C_WRITE_MODE);
	sp_i2cm_int_set(sr, int0, 0, 0);

	sp_i2cm_dma_addr_set(sr, spi2c_cmd->dma_w_addr);
	sp_i2cm_dma_len_set(sr, spi2c_cmd->xfer_cnt);
	sp_i2cm_dma_rw_mode_set(sr, I2C_DMA_READ_MODE);
	sp_i2cm_dma_int_en_set(sr, dma_int);
	sp_i2cm_dma_go_set(sr);

	ret = wait_event_timeout(spi2c->wait, spi2c_irq->active_done,
				 (SP_I2C_SLEEP_TIMEOUT * HZ) / 200);
	if (ret == 0) {
		dev_err(spi2c->dev, "I2C DMA write timeout !!\n");
		ret = -ETIMEDOUT;
	} else {
		ret = spi2c_irq->ret;
	}
	sp_i2cm_status_clear(sr, 0xFFFFFFFF);

	spi2c_irq->rw_state = SPI2C_STATE_IDLE;
	spi2c_irq->busy = 0;

	sp_i2cm_reset(sr);
	return ret;
}

static int sp_i2cm_dma_read(struct sp_i2c_cmd *spi2c_cmd, struct sp_i2c_dev *spi2c, struct i2c_msg *msgs)
{
	struct sp_i2c_irq_event *spi2c_irq = &spi2c->spi2c_irq;
	void __iomem *sr = spi2c->i2c_regs;
	unsigned int int0 = 0, int1 = 0, int2 = 0;
	unsigned int write_cnt = 0;
	unsigned int read_cnt = 0;
	unsigned int dma_int = 0;
	int ret = SPI2C_SUCCESS;

	if (spi2c_irq->busy) {
		dev_err(spi2c->dev, "IO error !!\n");
		return -ENXIO;
	}

	if (spi2c->mode == SP_I2C_DMA_POW_SW && spi2c->i2c_power_regs != 0)
		sp_i2cm_enable(0, spi2c->i2c_power_regs);

	sp_i2cm_reset(sr);
	memset(spi2c_irq, 0, sizeof(*spi2c_irq));
	spi2c_irq->busy = 1;

	write_cnt = spi2c_cmd->restart_write_cnt;
	read_cnt = spi2c_cmd->xfer_cnt;

	if (spi2c_cmd->restart_en) {
		if (write_cnt > 32) {
			spi2c_irq->busy = 0;
			dev_err(spi2c->dev,
				"I2C write count is invalid !! write count=%d\n", write_cnt);
			return -EINVAL;
		}
	}
	if (read_cnt > 0xFFFF  || read_cnt == 0) {
		spi2c_irq->busy = 0;
		dev_err(spi2c->dev,
			"I2C read count is invalid !! read count=%d\n", read_cnt);
		return -EINVAL;
	}

	int0 = (SP_I2C_EN0_SCL_HOLD_TOO_LONG_INT | SP_I2C_EN0_EMP_INT | SP_I2C_EN0_DATA_NACK_INT
		| SP_I2C_EN0_ADDR_NACK_INT | SP_I2C_EN0_DONE_INT);
	dma_int = SP_I2C_DMA_EN_DMA_DONE_INT;

	spi2c_irq->rw_state = SPI2C_STATE_DMA_RD;
	spi2c_irq->data_total_len = read_cnt;

	sp_i2cm_addr_freq_set(sr, spi2c_cmd->freq, spi2c_cmd->slv_addr);
	sp_i2cm_scl_delay_set(sr, SP_I2C_SCL_DELAY);

	if (spi2c_cmd->restart_en) {
		sp_i2cm_active_mode_set(sr, I2C_PIO_MODE);
		sp_i2cm_rw_mode_set(sr, I2C_RESTART_MODE);
		sp_i2cm_trans_cnt_set(sr, write_cnt, read_cnt);
		write_cnt = spi2c_cmd->restart_write_cnt  / 4;
		if (spi2c_cmd->restart_write_cnt % 4)
			write_cnt += 1;

		sp_i2cm_data_tx(sr, spi2c_cmd->write_data, write_cnt);

	} else {
		sp_i2cm_active_mode_set(sr, spi2c_cmd->xfer_mode);
		sp_i2cm_rw_mode_set(sr, I2C_READ_MODE);
	}
	sp_i2cm_int_set(sr, int0, int1, int2);

	sp_i2cm_dma_addr_set(sr, spi2c_cmd->dma_r_addr);
	sp_i2cm_dma_len_set(sr, spi2c_cmd->xfer_cnt);
	sp_i2cm_dma_rw_mode_set(sr, I2C_DMA_WRITE_MODE);
	sp_i2cm_dma_int_en_set(sr, dma_int);
	sp_i2cm_dma_go_set(sr);

	if (spi2c_cmd->restart_en)
		sp_i2cm_manual_trigger(sr); //start send data

	ret = wait_event_timeout(spi2c->wait, spi2c_irq->dma_done,
				 (SP_I2C_SLEEP_TIMEOUT * HZ) / 200);
	if (ret == 0) {
		dev_err(spi2c->dev, "I2C DMA read timeout !!\n");
		ret = -ETIMEDOUT;
	} else {
		ret = spi2c_irq->ret;
	}
	sp_i2cm_status_clear(sr, 0xFFFFFFFF);

	//copy data from virtual addr to spi2c_cmd->read_data
	spi2c_irq->rw_state = SPI2C_STATE_IDLE;
	spi2c_irq->busy = 0;

	sp_i2cm_reset(sr);
	return ret;
}

static int sp_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct sp_i2c_dev *spi2c = adap->algo_data;
	struct sp_i2c_cmd *spi2c_cmd = &spi2c->spi2c_cmd;
	unsigned char restart_w_data[32] = {0};
	unsigned int  restart_write_cnt = 0;
	unsigned int  restart_en = 0;
	u8 *r_buf;
	u8 *w_buf;
	int ret = SPI2C_SUCCESS;
	int i,j;

	ret = pm_runtime_get_sync(spi2c->dev);

	if (num == 0)
		return -EINVAL;

	memset(spi2c_cmd, 0, sizeof(*spi2c_cmd));

	if (spi2c_cmd->freq > SP_I2C_FAST_FREQ)
		spi2c_cmd->freq = SP_I2C_FAST_FREQ;
	else
		spi2c_cmd->freq = spi2c->i2c_clk_freq / 1000;

	for (i = 0; i < num; i++) {
		if (msgs[i].flags & I2C_M_TEN)
			return -EINVAL;
		r_buf = NULL;
		w_buf = NULL;
		spi2c_cmd->xfer_mode = I2C_PIO_MODE;

		spi2c_cmd->slv_addr = msgs[i].addr;
		if (msgs[i].flags & I2C_M_NOSTART) {
			restart_write_cnt = msgs[i].len;
			for (j = 0; j < restart_write_cnt; j++)
				restart_w_data[j] = msgs[i].buf[j];

			restart_en = 1;
			continue;
		}
		//pr_info(" xfer len %d\n",msgs[i].len);
		spi2c_cmd->xfer_cnt  = msgs[i].len;
		if (msgs[i].flags & I2C_M_RD) {
			if (restart_en == 1) {
				spi2c_cmd->restart_write_cnt = restart_write_cnt;
				spi2c_cmd->write_data = restart_w_data;
				restart_en = 0;
				spi2c_cmd->restart_en = 1;
			}

			if(spi2c_cmd->xfer_cnt >= 16) {
				r_buf = i2c_get_dma_safe_msg_buf(&msgs[i], 16);
				if (r_buf) {
					spi2c_cmd->dma_r_addr = dma_map_single(spi2c->dev, r_buf,
									       spi2c_cmd->xfer_cnt, DMA_FROM_DEVICE);
					if (dma_mapping_error(spi2c->dev, spi2c_cmd->dma_r_addr))
						i2c_put_dma_safe_msg_buf(r_buf, &msgs[i], false);
					else
						spi2c_cmd->xfer_mode = I2C_DMA_MODE;
				}
			}

			if (spi2c_cmd->xfer_mode == I2C_PIO_MODE) {
				spi2c_cmd->read_data = msgs[i].buf;
				ret = sp_i2cm_read(spi2c_cmd, spi2c);
			} else {
				ret = sp_i2cm_dma_read(spi2c_cmd, spi2c, &msgs[i]);
				dma_unmap_single(spi2c->dev, spi2c_cmd->dma_r_addr,
						 spi2c_cmd->xfer_cnt, DMA_FROM_DEVICE);
				i2c_put_dma_safe_msg_buf(r_buf, &msgs[i], true);
			}
		} else {
			if(spi2c_cmd->xfer_cnt >= 16) {
				w_buf = i2c_get_dma_safe_msg_buf(&msgs[i], 16);
				if (w_buf) {
					spi2c_cmd->dma_w_addr = dma_map_single(spi2c->dev, w_buf,
									       spi2c_cmd->xfer_cnt, DMA_TO_DEVICE);
					if (dma_mapping_error(spi2c->dev, spi2c_cmd->dma_w_addr))
						i2c_put_dma_safe_msg_buf(w_buf, &msgs[i], false);
					else
						spi2c_cmd->xfer_mode = I2C_DMA_MODE;
				}
			}
			if (spi2c_cmd->xfer_mode == I2C_PIO_MODE) {
				spi2c_cmd->write_data = msgs[i].buf;
				ret = sp_i2cm_write(spi2c_cmd, spi2c);
			} else {
				ret = sp_i2cm_dma_write(spi2c_cmd, spi2c, &msgs[i]);
				dma_unmap_single(spi2c->dev, spi2c_cmd->dma_w_addr,
						 spi2c_cmd->xfer_cnt, DMA_TO_DEVICE);
				i2c_put_dma_safe_msg_buf(w_buf, &msgs[i], true);
			}
		}
		if (ret != SPI2C_SUCCESS)
			return -EIO;
	}
	pm_runtime_put(spi2c->dev);
	return num;
}

static u32 sp_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm sp_algorithm = {
	.master_xfer	= sp_master_xfer,
	.functionality	= sp_functionality,
};

static const struct i2c_compatible i2c_645_compat = {
	.mode = SP_I2C_DMA_POW_NO_SW,
};

static const struct of_device_id sp_i2c_of_match[] = {
	{	.compatible = "sunplus,q645-i2cm",
		.data = &i2c_645_compat, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_i2c_of_match);

static void sp_i2c_disable_unprepare(void *data)
{
	clk_disable_unprepare(data);
}

static void sp_i2c_reset_control_assert(void *data)
{
	reset_control_assert(data);
}

static int sp_i2c_probe(struct platform_device *pdev)
{
	struct sp_i2c_dev *spi2c;
	struct i2c_adapter *p_adap;
	int ret = SPI2C_SUCCESS;
	struct resource *res;
	struct device *dev = &pdev->dev;
	const struct i2c_compatible *dev_mode;

	if (pdev->dev.of_node)
		pdev->id = of_alias_get_id(pdev->dev.of_node, "i2c");

	spi2c = devm_kzalloc(&pdev->dev, sizeof(*spi2c), GFP_KERNEL);
	if (!spi2c)
		return -ENOMEM;

	if (!of_property_read_u32(pdev->dev.of_node, "clock-frequency", &spi2c->i2c_clk_freq))
		spi2c->i2c_clk_freq = SP_I2C_STD_FREQ * 1000;

	dev_mode = of_device_get_match_data(&pdev->dev);
	spi2c->mode = dev_mode->mode;
	spi2c->dev = &pdev->dev;

	spi2c->i2c_regs = devm_platform_ioremap_resource_byname(pdev, "i2cm");
	if (IS_ERR(spi2c->i2c_regs))
		return dev_err_probe(&pdev->dev, PTR_ERR(spi2c->i2c_regs), "regs get fail\n");

	if (spi2c->mode == SP_I2C_DMA_POW_SW) {
		//spi2c->i2c_power_regs = devm_platform_ioremap_resource_byname(pdev, "i2cdmapower");
		//if (IS_ERR(spi2c->i2c_power_regs))
		//	spi2c->i2c_power_regs = 0;

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "i2cdmapower");
		if (IS_ERR(res))
			dev_err_probe(&pdev->dev, PTR_ERR(res), "resource get fail\n");

		spi2c->i2c_power_regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (IS_ERR(spi2c->i2c_power_regs)){
			spi2c->i2c_power_regs = 0;
			dev_err_probe(&pdev->dev, PTR_ERR(spi2c->i2c_power_regs), "power regs get fail\\n");
		}
	}

	spi2c->irq = platform_get_irq(pdev, 0);
	if (!spi2c->irq) {
		dev_err(&pdev->dev, "IRQ missing or invalid\n");
		return -EINVAL;
	}

	ret = devm_request_irq(&pdev->dev, spi2c->irq, _sp_i2cm_irqevent_handler,
			       IRQF_TRIGGER_HIGH, pdev->name, spi2c);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq %i\n", spi2c->irq);
		return ret;
	}

	spi2c->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(spi2c->clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(spi2c->clk), "err get clock\n");

	spi2c->rstc = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(spi2c->rstc))
		return dev_err_probe(&pdev->dev, PTR_ERR(spi2c->rstc), "err get reset\n");

	ret = clk_prepare_enable(spi2c->clk);
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "failed to enable clk\n");

	ret = devm_add_action_or_reset(dev, sp_i2c_disable_unprepare, spi2c->clk);
	if (ret)
		return ret;

	ret = reset_control_deassert(spi2c->rstc);
	if (ret)
		dev_err_probe(&pdev->dev, ret, "failed to deassert reset\n");

	ret = devm_add_action_or_reset(dev, sp_i2c_reset_control_assert, spi2c->rstc);
	if (ret)
		return ret;

	init_waitqueue_head(&spi2c->wait);

	p_adap = &spi2c->adap;
	sprintf(p_adap->name, "%s%d", "sunplus-i2cm", pdev->id);
	p_adap->algo = &sp_algorithm;
	p_adap->algo_data = spi2c;
	p_adap->nr = pdev->id;
	p_adap->class = 0;
	p_adap->retries = 5;
	p_adap->dev.parent = &pdev->dev;
	p_adap->dev.of_node = pdev->dev.of_node;

	ret = i2c_add_numbered_adapter(p_adap);
	sp_i2cm_reset(spi2c->i2c_regs);
	platform_set_drvdata(pdev, spi2c);

	pm_runtime_set_autosuspend_delay(&pdev->dev, 5000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	return ret;
}

static int sp_i2c_remove(struct platform_device *pdev)
{
	struct sp_i2c_dev *spi2c = platform_get_drvdata(pdev);
	struct i2c_adapter *p_adap = &spi2c->adap;

	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	i2c_del_adapter(p_adap);

	return 0;
}

static int __maybe_unused sp_i2c_suspend(struct device *dev)
{
	struct sp_i2c_dev *spi2c = dev_get_drvdata(dev);

	reset_control_assert(spi2c->rstc);
	return 0;
}

static int __maybe_unused sp_i2c_resume(struct device *dev)
{
	struct sp_i2c_dev *spi2c = dev_get_drvdata(dev);

	reset_control_deassert(spi2c->rstc);
	return 0;
}

static int sp_i2c_runtime_suspend(struct device *dev)
{
	struct sp_i2c_dev *spi2c = dev_get_drvdata(dev);

	reset_control_assert(spi2c->rstc);
	return 0;
}

static int sp_i2c_runtime_resume(struct device *dev)
{
	struct sp_i2c_dev *spi2c = dev_get_drvdata(dev);

	reset_control_deassert(spi2c->rstc);   //release reset
	clk_prepare_enable(spi2c->clk);        //enable clken and disable gclken
	return 0;
}

static const struct dev_pm_ops sp7021_i2c_pm_ops = {
	SET_RUNTIME_PM_OPS(sp_i2c_runtime_suspend,
			   sp_i2c_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(sp_i2c_suspend, sp_i2c_resume)

};

#define sp_i2c_pm_ops  (&sp7021_i2c_pm_ops)

static struct platform_driver sp_i2c_driver = {
	.probe		= sp_i2c_probe,
	.remove		= sp_i2c_remove,
	.driver		= {
		.name		= "sunplus-i2cm",
		.of_match_table = sp_i2c_of_match,
		.pm     = sp_i2c_pm_ops,
	},
};

static int __init sp_i2c_adap_init(void)
{
	return platform_driver_register(&sp_i2c_driver);
}
module_init(sp_i2c_adap_init);

static void __exit sp_i2c_adap_exit(void)
{
	platform_driver_unregister(&sp_i2c_driver);
}
module_exit(sp_i2c_adap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Li-hao Kuo <lhjeff911@gmail.com>");
MODULE_DESCRIPTION("Sunplus I2C Master Driver");
