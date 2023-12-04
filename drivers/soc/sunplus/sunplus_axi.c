// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/firmware.h>
#include <asm/irq.h>

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include "sunplus_axi.h"
#include "axi_ioctl.h"

#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/reset.h>

/*AXI Monitor Validation*/
#include "axi_monitor.h"
#include "hal_axi_monitor_reg.h"
#include "hal_axi_monitor_sub_reg.h"
#include "regmap_q628.h"
#include "reg_axi.h"


/*for AXI monitor*/
#define AXI_MONITOR_TOP_REG_NAME  "axi_mon"
#define AXI_IP_00_REG_NAME		  "axi_0"
#define AXI_IP_01_REG_NAME		  "axi_1"
#define AXI_IP_02_REG_NAME		  "axi_2"
#define AXI_IP_03_REG_NAME		  "axi_3"
#define AXI_IP_04_REG_NAME        "axi_4"
#define AXI_IP_05_REG_NAME        "axi_5"
#define AXI_IP_06_REG_NAME	      "axi_6"
#define AXI_IP_07_REG_NAME		  "axi_7"
#define AXI_IP_08_REG_NAME		  "axi_8"
#define AXI_IP_09_REG_NAME        "axi_9"
#define AXI_IP_10_REG_NAME        "axi_10"
#define AXI_IP_11_REG_NAME		  "axi_11"
#define AXI_IP_12_REG_NAME		  "axi_12"
#define AXI_IP_13_REG_NAME		  "axi_13"
#define AXI_IP_14_REG_NAME        "axi_14"
#define AXI_IP_15_REG_NAME        "axi_15"
#define AXI_IP_16_REG_NAME	      "axi_16"
#define AXI_IP_17_REG_NAME		  "axi_17"
#define AXI_IP_18_REG_NAME		  "axi_18"
#define AXI_IP_19_REG_NAME        "axi_19"
#define AXI_IP_20_REG_NAME        "axi_20"
#define AXI_IP_21_REG_NAME        "axi_21"
#define AXI_IP_22_REG_NAME        "axi_22"
#define AXI_IP_23_REG_NAME        "axi_23"
#define AXI_IP_24_REG_NAME        "axi_24"
#define AXI_IP_25_REG_NAME        "axi_25"
#define AXI_IP_26_REG_NAME        "axi_26"
#define AXI_MONITOR_PAI_REG_NAME  "axi_27"
#define AXI_IP_28_REG_NAME        "axi_28"
#define AXI_IP_29_REG_NAME        "axi_29"
#define AXI_IP_30_REG_NAME        "axi_30"
#define AXI_IP_31_REG_NAME        "axi_31"
#define DEVICE_NAME			"sunplus,sp7350-axi"

/*Device ID*/
#define CA55		0
#define CSDBG_M1	1
#define CSETR_M2    2
#define NPU_MA		3
#define AXI_DMA_M0	4
#define CBDMA0_MA	5
#define CPIOR0_MA	6
#define CPIOR1_MA	7
#define SEC_AES		8
#define SEC_HASH	9
#define SEC_RSA		10
#define USB30C0_MA	11
#define USBC0_MA	12
#define CARD0_MA	13
#define CARD1_MA	14
#define CARD2_MA	15
#define GMAC_MA		16
#define VI23_CSIIW0_MA	17
#define VI23_CSIIW1_MA	18
#define VI23_CSIIW2_MA	19
#define VI23_CSIIW3_MA	20
#define RB_SL		21
#define VCL_SL		22
#define TZC_SL0		23
#define CPIOR_SL	24
#define TZC_SL2		25
#define TZC_SL3		26
#define VCL_MA		28
#define VCE_MA		29
#define VCD_MA		30
#define TZC_SL1		31
#define valid_id 1
#define invalid_id 0

struct sp_axi_t {
	struct miscdevice dev;
	struct mutex write_lock;
	//void __iomem *iop_regs;
	//void __iomem *moon0_regs;
	//void __iomem *qctl_regs;
	//void __iomem *pmc_regs;
	//void __iomem *rtc_regs;
	/*for AXI monitor*/
	void __iomem *axi_mon_regs;
	void __iomem *axi_id0_regs;
	void __iomem *axi_id1_regs;
	void __iomem *axi_id2_regs;
	void __iomem *axi_id3_regs;
	void __iomem *axi_id4_regs;
	void __iomem *axi_id5_regs;
	void __iomem *axi_id6_regs;
	void __iomem *axi_id7_regs;
	void __iomem *axi_id8_regs;
	void __iomem *axi_id9_regs;
	void __iomem *axi_id10_regs;
	void __iomem *axi_id11_regs;
	void __iomem *axi_id12_regs;
	void __iomem *axi_id13_regs;
	void __iomem *axi_id14_regs;
	void __iomem *axi_id15_regs;
	void __iomem *axi_id16_regs;
	void __iomem *axi_id17_regs;
	void __iomem *axi_id18_regs;
	void __iomem *axi_id19_regs;
	void __iomem *axi_id20_regs;
	void __iomem *axi_id21_regs;
	void __iomem *axi_id22_regs;
	void __iomem *axi_id23_regs;
	void __iomem *axi_id24_regs;
	void __iomem *axi_id25_regs;
	void __iomem *axi_id26_regs;
	void __iomem *axi_id27_regs;
	void __iomem *axi_id28_regs;
	void __iomem *axi_id29_regs;
	void __iomem *axi_id30_regs;
	void __iomem *axi_id31_regs;
	void __iomem *axi_cbdma_regs;
	void __iomem *current_id_regs;
	int irq;
};
static struct sp_axi_t *axi_monitor;

/*AXI Monitor Validation end*/
/* ---------------------------------------------------------------------------------------------- */
#define AXI_FUNC_DEBUG
#define AXI_KDBG_INFO
#define AXI_KDBG_ERR

#ifdef AXI_FUNC_DEBUG
	#define FUNC_DEBUG()    pr_info("[AXI]: %s(%d)\n", __func__, __LINE__)
#else
	#define FUNC_DEBUG()
#endif

#ifdef AXI_KDBG_INFO
#define DBG_INFO(fmt, args ...)	pr_info("K_AXI: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef AXI_KDBG_ERR
#define DBG_ERR(fmt, args ...)	pr_err("K_AXI: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif

unsigned char AxiDeviceID;
struct sunplus_axi {
	struct clk *axiclk;
	struct reset_control *rstc;
};

struct sunplus_axi sp_axi;

#define CBDMA0_SRAM_ADDRESS (0x9E800000) // 40KB
#define CBDMA1_SRAM_ADDRESS (0x9E820000) // 4KB
#define CBDMA_TEST_SOURCE      ((void *) 0x9EA00000)
#define CBDMA_TEST_DESTINATION ((void *) 0x9EA01000)
#define CBDMA_TEST_SIZE       0x1000

void Get_Monitor_Event(void __iomem *axi_id_regs)
{
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;

	DBG_INFO("current_id_regs=%p\n", axi_id_regs);
	DBG_INFO("axi_id ip monitor: 0x%X\n", readl(&axi_id->sub_ip_monitor));
	DBG_INFO("axi_id event infomation: 0x%X\n", readl(&axi_id->sub_event));
}

void Event_Monitor_Clear(void __iomem *axi_mon_regs, void __iomem *axi_id_regs)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;

	writel(0x0001, &axi->axi_control);
	writel(0x00000000, &axi_id->sub_ip_monitor);
}


void Get_current_id(unsigned char device_id)
{
	/*Device ID*/
	switch (device_id) {
	case CA55:
		DBG_INFO("CA55\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id0_regs;
		break;
	case CSDBG_M1:
		DBG_INFO("CSDBG_M1\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id1_regs;
		break;
	case CSETR_M2:
		DBG_INFO("CSETR_M2\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id2_regs;
		break;
	case NPU_MA:
		DBG_INFO("NPU_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id3_regs;
		break;
	case AXI_DMA_M0:
		DBG_INFO("AXI_DMA_M0\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id4_regs;
		break;
	case CBDMA0_MA:
		DBG_INFO("CBDMA0_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id5_regs;
		break;
	case CPIOR0_MA:
		DBG_INFO("CPIOR0_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id6_regs;
		break;
	case CPIOR1_MA:
		DBG_INFO("CPIOR1_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id7_regs;
		break;
	case SEC_AES:
		DBG_INFO("SEC_AES\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id8_regs;
		break;
	case SEC_HASH:
		DBG_INFO("SEC_HASH\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id9_regs;
		break;
	case SEC_RSA:
		DBG_INFO("SEC_RSA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id10_regs;
		break;
	case USB30C0_MA:
		DBG_INFO("USB30C0_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id11_regs;
		break;
	case USBC0_MA:
		DBG_INFO("USBC0_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id12_regs;
		break;
	case CARD0_MA:
		DBG_INFO("CARD0_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id13_regs;
		break;
	case CARD1_MA:
		DBG_INFO("CARD1_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id14_regs;
		break;
	case CARD2_MA:
		DBG_INFO("CARD2_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id15_regs;
		break;
	case GMAC_MA:
		DBG_INFO("GMAC_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id16_regs;
		break;
	case VI23_CSIIW0_MA:
		DBG_INFO("VI23_CSIIW0_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id17_regs;
		break;
	case VI23_CSIIW1_MA:
		DBG_INFO("VI23_CSIIW1_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id18_regs;
		break;
	case VI23_CSIIW2_MA:
		DBG_INFO("VI23_CSIIW2_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id19_regs;
		break;
	case VI23_CSIIW3_MA:
		DBG_INFO("VI23_CSIIW3_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id20_regs;
		break;
	case RB_SL:
		DBG_INFO("RB_SL\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id21_regs;
		break;
	case VCL_SL:
		DBG_INFO("VCL_SL\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id22_regs;
		break;
	case TZC_SL0:
		DBG_INFO("TZC_SL0\n");

		DBG_INFO("axi_monitor->axi_id23_regs =0x%px\n", axi_monitor->axi_id23_regs );
		axi_monitor->current_id_regs = axi_monitor->axi_id23_regs;

		DBG_INFO("axi_monitor->current_id_regs =0x%px\n", axi_monitor->current_id_regs );
		break;
	case CPIOR_SL:
		DBG_INFO("CPIOR_SL\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id24_regs;
		break;
	case TZC_SL2:
		DBG_INFO("TZC_SL2\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id25_regs;
		break;
	case TZC_SL3:
		DBG_INFO("TZC_SL3\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id26_regs;
		break;
	case VCL_MA:
		DBG_INFO("VCL_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id28_regs;
		break;
	case VCE_MA:
		DBG_INFO("VCE_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id29_regs;
		break;
	case VCD_MA:
		DBG_INFO("VCD_MA\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id30_regs;
		break;
	case TZC_SL1:
		DBG_INFO("TZC_SL1\n");
		axi_monitor->current_id_regs = axi_monitor->axi_id31_regs;
		break;
	}
}

static int Check_current_id(unsigned char device_id)
{
	switch (device_id) {
	case CA55:
	case CSDBG_M1:
	case CSETR_M2:
	case NPU_MA:
	case AXI_DMA_M0:
	case CBDMA0_MA:
	case CPIOR0_MA:
	case CPIOR1_MA:
	case SEC_AES:
	case SEC_HASH:
	case SEC_RSA:
	case USB30C0_MA:
	case USBC0_MA:
	case CARD0_MA:
	case CARD1_MA:
	case CARD2_MA:
	case GMAC_MA:
	case VI23_CSIIW0_MA:
	case VI23_CSIIW1_MA:
	case VI23_CSIIW2_MA:
	case VI23_CSIIW3_MA:
	case RB_SL:
	case VCL_SL:
	case TZC_SL0:
	case CPIOR_SL:
	case TZC_SL2:
	case TZC_SL3:
	case VCL_MA:
	case VCE_MA:
	case VCD_MA:
	case TZC_SL1:
		DBG_INFO("valid_id");
		return valid_id;
	default:
		DBG_INFO("invalid_id");
		return invalid_id;
	}
}

static irqreturn_t axi_irq_handler(int irq, void *data)
{
	DBG_INFO("[AXI] %s\n", __func__);
	DBG_INFO("AXI device_id=%d\n", AxiDeviceID);
	Get_current_id(AxiDeviceID);
	Get_Monitor_Event(axi_monitor->current_id_regs);
	Event_Monitor_Clear(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs);
	return IRQ_HANDLED;
}



void axi_mon_special_data(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;

	DBG_INFO("axi=0x%p\n", axi);
	DBG_INFO("axi_id=0x%p\n", axi_id);

	writel(data, &axi->axi_special_data);

	//bit8:latency_mon_start = 1;bit4=bw_mon_start = 1; bit0=event_clear = 1
	writel(0x00000111, &axi->axi_control);

	//bit20: timeout=0, bit16:unexpect_r_access=0, bit12:unexpect_w_access=0,
	//bit8: special_r_data=1, bit4: special_w_data=1, bit0: monitor enable=1.
	writel(0x00000111, &axi_id->sub_ip_monitor);
}

void axi_mon_unexcept_access_sAddr(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;

	DBG_INFO("axi=0x%p\n", axi);
	DBG_INFO("axi_id=0x%p\n", axi_id);

	writel((data>>16), &axi->axi_valid_start_add);
	DBG_INFO("unexpect_access_sAddr=0x%x\n", (data>>16));
	writel((data&0xFFFF), &axi->axi_valid_end_add);
	DBG_INFO("unexpect_access_eAddr=0x%x\n", (data&0xFFFF));
	//bit8:latency_mon_start = 1;bit4=bw_mon_start = 1; bit0=event_clear = 1
	writel(0x00000111, &axi->axi_control);
	//unexpect access
	//bit20: timeout=0, bit16:unexpect_r_access=1, bit12:unexpect_w_access=1,
	//bit8: special_r_data=0, bit4: special_w_data=0, bit0: monitor enable=1.
	writel(0x00011001, &axi_id->sub_ip_monitor);
}

void axi_mon_unexcept_access_eAddr(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;

	DBG_INFO("axi_mon_regs=%p\n", axi_mon_regs);
	writel(data, &axi->axi_valid_end_add);
	//bit8:latency_mon_start = 1;bit4=bw_mon_start = 1; bit0=event_clear = 1
	writel(0x00000111, &axi->axi_control);
	//unexpect access
	//bit20: timeout=0, bit16:unexpect_r_access=1, bit12:unexpect_w_access=1,
	//bit8: special_r_data=0, bit4: special_w_data=0, bit0: monitor enable=1.
	writel(0x00011001, &axi_id->sub_ip_monitor);
}

void axi_mon_timeout(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;

	DBG_INFO("axi_mon_regs=0x%p\n", axi);
	DBG_INFO("axi_id=0x%p\n", axi_id);

	// about 4.95ns, configure Timeout cycle
	writel(data, &axi->axi_time_out);

	// about 83ms
	//BW update period = 0x4, BW Monitor Start=1
	writel(0x00004010, &axi->axi_control);

	//timeout
	//bit20: timeout=1, bit16:unexpect_r_access=0, bit12:unexpect_w_access=0,
	//bit8: special_r_data=0, bit4: special_w_data=0, bit0: monitor enable=1.
	writel(0x00100001, &axi_id->sub_ip_monitor);
}

void axi_mon_BW_Monitor(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;

	DBG_INFO("axi_mon_regs=0x%px\n", axi);
	DBG_INFO("axi_id=0x%px\n", axi_id);

	//data = BW update period, BW Monitor Start=1, Latency Monitor Start =1
	writel(0x00000111 | (data << 12), &axi->axi_control);
	//IP Monitor Enable = 1
	writel(0x00000001, &axi_id->sub_ip_monitor);
}

void axi_mon_BW_Value(void __iomem *axi_id_regs)
{
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;
	unsigned long long temp;

	DBG_INFO("axi_id=0x%px\n", axi_id);
	temp = readl(&axi_id->sub_ip_monitor);
	DBG_INFO("sub_ip_monitor=0x%llx\n", temp);
	temp = readl(&axi_id->sub_event);
	DBG_INFO("sub_event=0x%llx\n", temp);
	temp = readl(&axi_id->sub_bw);
	DBG_INFO("sub_bw=0x%llx\n", temp);
	temp = readl(&axi_id->sub_wcomd_count);
	DBG_INFO("sub_wcomd_count=0x%llx\n", temp);
	temp = readl(&axi_id->axi_wcomd_execute_cycle_time);
	DBG_INFO("axi_wcomd_execute_cycle_time=0x%llx\n", temp);
	temp = readl(&axi_id->axi_rcomd_count);
	DBG_INFO("axi_rcomd_count=0x%llx\n", temp);
	temp = readl(&axi_id->axi_rcomd_execute_cycle_time);
	DBG_INFO("axi_rcomd_execute_cycle_time=0x%llx\n", temp);
}

static ssize_t device_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	DBG_INFO("[AXI] %s\n", __func__);
	return len;
}

static ssize_t device_id_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	long val;
	ssize_t status;

	DBG_INFO("[AXI] %s\n", __func__);
	status = kstrtol(buf, 0, &val);	//Get device_id
	if (status)
		return status;
	AxiDeviceID = val;
	if (Check_current_id(AxiDeviceID) == valid_id)
		DBG_INFO("AXI device_id=%d\n", AxiDeviceID);
	else
		DBG_INFO("INVALID DEVICE ID\n");
	return ret;
}


static ssize_t special_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	DBG_INFO("[AXI] %s\n", __func__);
	return len;
}

static ssize_t special_data_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	unsigned int special_data;
	long val;
	ssize_t status;

	DBG_INFO("[AXI] %s\n", __func__);
	status = kstrtol(buf, 0, &val);	//Get special_data
	if (status)
		return status;
	special_data = val;
	DBG_INFO("AXI device_id=%d\n", AxiDeviceID);
	DBG_INFO("special_data=0x%x\n", special_data);
	if (Check_current_id(AxiDeviceID) == valid_id) {
		Get_current_id(AxiDeviceID);
		axi_mon_special_data(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs, special_data);
	} else
		DBG_INFO("INVALID DEVICE ID\n");

	return ret;
}

static ssize_t unexpect_access_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	DBG_INFO("[AXI] %s\n", __func__);
	return len;
}

static ssize_t unexpect_access_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	unsigned int unexpect_access;
	long val;
	ssize_t status;

	DBG_INFO("[AXI] %s\n", __func__);
	status = kstrtol(buf, 0, &val); //Get unexpect_access
	if (status)
		return status;
	unexpect_access = val;
	DBG_INFO("AXI device_id=%d\n", AxiDeviceID);
	if (Check_current_id(AxiDeviceID) == valid_id) {
		Get_current_id(AxiDeviceID);
		axi_mon_unexcept_access_sAddr(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs, unexpect_access);
	} else
		DBG_INFO("INVALID DEVICE ID\n");

	return ret;
}

static ssize_t time_out_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	DBG_INFO("[AXI] %s\n", __func__);
	return len;
}

static ssize_t time_out_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	unsigned int Timeout_cycle;
	long val;
	ssize_t status;

	DBG_INFO("[AXI] %s\n", __func__);
	status = kstrtol(buf, 0, &val); //Get Timeout cycle
	if (status)
		return status;
	Timeout_cycle = val;
	DBG_INFO("AXI device_id=%d\n", AxiDeviceID);
	DBG_INFO("Timeout_cycle=0x%x\n", Timeout_cycle);
	if (Check_current_id(AxiDeviceID) == valid_id) {
		Get_current_id(AxiDeviceID);
		axi_mon_timeout(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs, Timeout_cycle);
	} else
		DBG_INFO("INVALID DEVICE ID\n");

	return ret;
}

static ssize_t bw_monitor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;

	DBG_INFO("[AXI] %s\n", __func__);
	axi_mon_BW_Value(axi_monitor->current_id_regs);
	return len;
}


static ssize_t bw_monitor_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	unsigned int BW_update_period;
	long val;
	ssize_t status;

	DBG_INFO("[AXI] %s\n", __func__);
	status = kstrtol(buf, 0, &val); //Get BW update period
	if (status)
		return status;
	BW_update_period = val;
	DBG_INFO("AXI device_id=%d\n", AxiDeviceID);
	DBG_INFO("BW_update_period=0x%x\n", BW_update_period);
	if (Check_current_id(AxiDeviceID) == valid_id) {
		Get_current_id(AxiDeviceID);
		axi_mon_BW_Monitor(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs, BW_update_period);
	} else {
		DBG_INFO("INVALID DEVICE ID\n");
	}
	return ret;
}

static DEVICE_ATTR_RW(device_id);
static DEVICE_ATTR_RW(special_data);
static DEVICE_ATTR_RW(unexpect_access);
//static DEVICE_ATTR_RW(unexpect_access_eAddr);
static DEVICE_ATTR_RW(time_out);
static DEVICE_ATTR_RW(bw_monitor);

/* ---------------------------------------------------------------------------------------------- */

/**************************************************************************/
/*			                         G L O B A L    D A T A				                         */
/**************************************************************************/

static int sp_axi_open(struct inode *inode, struct file *pfile)
{
	DBG_INFO("Sunplus AXI module open\n");
	return 0;
}

static int sp_axi_release(struct inode *inode, struct file *pfile)
{
	DBG_INFO("Sunplus AXI module release\n");
	return 0;
}

static long sp_axi_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	return ret;
}

static const struct file_operations sp_axi_fops = {
	.owner          = THIS_MODULE,
	.open           = sp_axi_open,
	.release        = sp_axi_release,
	.unlocked_ioctl = sp_axi_ioctl,
};

static int _sp_axi_get_register_base(struct platform_device *pdev, unsigned long long *membase, const char *res_name)
{
	struct resource *r;
	void __iomem *p;

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if (r == NULL) {
		DBG_ERR("[AXI] platform_get_resource_byname fail\n");
		return -ENODEV;
	}

	p = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(p)) {
		DBG_ERR("[AXI] ioremap fail\n");
		return PTR_ERR(p);
	}

	*membase = (unsigned long long)p;
	return IOP_SUCCESS;
}

static int _sp_axi_get_resources(struct platform_device *pdev, struct sp_axi_t *pstSpIOPInfo)
{
	int ret;

	unsigned long long membase = 0;

	FUNC_DEBUG();

	/*for AXI monitor*/
	ret = _sp_axi_get_register_base(pdev, &membase, AXI_MONITOR_TOP_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_mon_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_00_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id0_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_01_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id1_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_02_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id2_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_03_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id3_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_04_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id4_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_05_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id5_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_06_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id6_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_07_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id7_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_08_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id8_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_09_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id9_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_10_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id10_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_11_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id11_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_12_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id12_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_13_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id13_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_14_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id14_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_15_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id15_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_16_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id16_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_17_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id17_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_18_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id18_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_19_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id19_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_20_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id20_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_21_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id21_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_22_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id22_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_23_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id23_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_24_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id24_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_25_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id25_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_26_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id26_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_MONITOR_PAI_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id26_regs = (void __iomem *)membase;

	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_28_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id28_regs = (void __iomem *)membase;
	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_29_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id29_regs = (void __iomem *)membase;
	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_30_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id30_regs = (void __iomem *)membase;
	ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_31_REG_NAME);
	if (ret) {
		DBG_ERR("[AXI] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpIOPInfo->axi_id31_regs = (void __iomem *)membase;

	return IOP_SUCCESS;
}


static int sp_axi_start(struct sp_axi_t *iopbase)
{

	FUNC_DEBUG();
	return IOP_SUCCESS;
}


static int sp_axi_platform_driver_probe(struct platform_device *pdev)
{
	int ret = -ENXIO;
	int err, irq;

	AxiDeviceID = 0;
	FUNC_DEBUG();
	axi_monitor = devm_kzalloc(&pdev->dev, sizeof(struct sp_axi_t), GFP_KERNEL);
	if (axi_monitor == NULL) {
		DBG_INFO("sp_iop_t malloc fail\n");
		ret	= -ENOMEM;
		goto fail_kmalloc;
	}
	/* init */
	mutex_init(&axi_monitor->write_lock);
	/* register device */
	axi_monitor->dev.name  = "sp_axi";
	axi_monitor->dev.minor = MISC_DYNAMIC_MINOR;
	axi_monitor->dev.fops  = &sp_axi_fops;
	ret = misc_register(&axi_monitor->dev);
	if (ret != 0) {
		DBG_INFO("sp_iop device register fail\n");
		goto fail_regdev;
	}

	ret = _sp_axi_get_resources(pdev, axi_monitor);

	ret = sp_axi_start(axi_monitor);
	if (ret != 0) {
		DBG_ERR("[AXI] sp axi init err=%d\n", ret);
		return ret;
	}

	// request irq
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		DBG_INFO("platform_get_irq failed\n");
		return -ENODEV;
	}

	err = devm_request_irq(&pdev->dev, irq, axi_irq_handler, IRQF_TRIGGER_HIGH, "axi irq", pdev);
	if (err) {
		DBG_INFO("devm_request_irq failed: %d\n", err);
		return err;
	}

	device_create_file(&pdev->dev, &dev_attr_device_id);
	device_create_file(&pdev->dev, &dev_attr_special_data);
	device_create_file(&pdev->dev, &dev_attr_unexpect_access);
	//device_create_file(&pdev->dev, &dev_attr_unexpect_access_eAddr);
	device_create_file(&pdev->dev, &dev_attr_time_out);
	device_create_file(&pdev->dev, &dev_attr_bw_monitor);

fail_regdev:
	mutex_destroy(&axi_monitor->write_lock);
fail_kmalloc:
	return ret;
}

static int sp_axi_platform_driver_remove(struct platform_device *pdev)
{
	//struct sunplus_axi *axi = platform_get_drvdata(pdev);
	FUNC_DEBUG();
	reset_control_assert(sp_axi.rstc);
	//rtc_device_unregister(axi);
	return 0;
}

static int sp_axi_platform_driver_suspend(struct platform_device *pdev, pm_message_t state)
{
	FUNC_DEBUG();
	return 0;
}

static int sp_axi_platform_driver_resume(struct platform_device *pdev)
{
	FUNC_DEBUG();
	return 0;
}

static const struct of_device_id sp_axi_of_match[] = {
	{ .compatible = "sunplus,sp7350-axi", },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, sp_axi_of_match);

static struct platform_driver sp_axi_platform_driver = {
	.probe		= sp_axi_platform_driver_probe,
	.remove		= sp_axi_platform_driver_remove,
	.suspend	= sp_axi_platform_driver_suspend,
	.resume		= sp_axi_platform_driver_resume,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_axi_of_match),
	}
};

module_platform_driver(sp_axi_platform_driver);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus AXI Driver");
MODULE_LICENSE("GPL");


