#include "nic_reg.h"

struct sp_nic_t {
	struct miscdevice dev;
	struct mutex write_lock;
	void __iomem *nic_main_regs;
	void __iomem *nic_pai_regs;
	void __iomem *nic_paii_regs;
	struct timer_list	nic_timer;
};
static struct sp_nic_t *nic_main;

#define NIC_ARBITER_REG_BASE	nic_main->nic_main_regs


/* master modules */
typedef enum master_module_list_t
{
	/*nic main*/
	CA55_M0,
	CSDBG_M1,
	CSETR_M2,
	NPU_MA,
	AXI_DMA_M0,
	AXI_DMA_M1,
	CBDMA0_MA,
	CPIOR0_MA,
	CPIOR1_MA,
	SEC_AES,
	SEC_HASH,
	SEC_RAS,
	USB30C0_MA,
	USBC0_MA,
	CARD0_MA,
	CARD1_MA,
	CARD2_MA,
	SPI_NOR_MA,
	NBS_MA,
	GMAC_MA,
	DUMMY0_MA,
	UART2AXI_MA,
	VI23_CSIIW0_MA,
	VI23_CSIIW1_MA,
	VI23_CSIIW2_MA,
	VI23_CSIIW3_MA,
    /*nic pai*/
	SLAM_MA,
	VC_8000E_MA,
	VCDNANO_MA,
	/*nic paii*/
	IMGREAD0_MA,
	DISP_OSD0_MA,
	DISP_OSD1_MA,
	DISP_OSD2_MA,
	DISP_OSD3_MA,
	VI0_CSIIW0_MA,
	VI0_CSIIW1_MA,
	VI1_CSIIW0_MA,
	VI1_CSIIW1_MA,
	VI4_CSIIW0_MA,
	VI4_CSIIW1_MA,
	VI5_CSIIW0_MA,
	VI5_CSIIW1_MA,
	VI5_CSIIW2_MA,
	VI5_CSIIW3_MA,
	DUMMY1_MA,
	MASTER_MAX_CNT
} master_module_list;

typedef enum nic_cap_type_t
{
	CAP_READ,
	CAP_WRITE
} nic_cap_type;

void hal_nic_disable_arbiter(u8 master_id);
void hal_nic_enable_arbiter(u8 master_id);
u16 hal_nic_get_arbiter_period(u8 master_id);
u16 hal_nic_get_bw(u8 master_id, nic_cap_type cap_type);
u32 hal_nic_get_captured_data(u8 master_id, nic_cap_type cap_type);
u8 hal_nic_get_default_qos(u8 master_id, nic_cap_type cap_type);
u16 hal_nic_get_max_latency(u8 master_id, nic_cap_type cap_type);
void hal_nic_get_priority_weight(u8 master_id, u8 *prio, u8 *weight);
void hal_nic_get_rate_limit(u8 master_id, nic_cap_type cap_type, u8 *enable, u8 *max_count, u16 *issue_cycle);
void hal_nic_reset_all_captured_data(u32 value);
void hal_nic_reset_captured_data(u8 master_id, nic_cap_type cap_type);
void hal_nic_set_arbiter_period(u16 period);
void hal_nic_set_bw_latency(u8 master_id, u16 alloc_bw, u16 max_latency);
void hal_nic_set_default_qos(u8 master_id, u8 qos, nic_cap_type cap_type);
void hal_nic_set_priority_weight(u8 master_id, u8 prio, u8 weight);
void hal_nic_set_rate_limit(u8 master_id, u8 enable, u8 max_count, u16 issue_cycle);

/**************************************************************************/



u16 hal_nic_get_arbiter_period(u8 master_id)
{
	arb_global_reg *arb_global_para;

	if(master_id < SLAM_MA)
		arb_global_para = (arb_global_reg *)(nic_main->nic_main_regs + 4);
	else if(master_id < IMGREAD0_MA)
		arb_global_para = (arb_global_reg *)(nic_main->nic_pai_regs + 4);
	else 
		arb_global_para = (arb_global_reg *)(nic_main->nic_paii_regs + 4);
	
	return (arb_global_para->arb_period);
}

void hal_nic_set_arbiter_period(u16 period)
{
	arb_global_reg *arb_global_para;

	arb_global_para = (arb_global_reg *)(nic_main->nic_main_regs + 4);
	arb_global_para->arb_period = (period & 0xfff);
	arb_global_para = (arb_global_reg *)(nic_main->nic_pai_regs + 4);
	arb_global_para->arb_period = (period & 0xfff);
	arb_global_para = (arb_global_reg *)(nic_main->nic_paii_regs + 4);
	arb_global_para->arb_period = (period & 0xfff);
}

void hal_nic_reset_all_captured_data(u32 value)
{
	volatile u32 *arbiter_version;

	arbiter_version = (volatile u32 *)(nic_main->nic_main_regs);
	*arbiter_version = value;
	
	arbiter_version = (volatile u32 *)(nic_main->nic_pai_regs);
	*arbiter_version = value;

	arbiter_version = (volatile u32 *)(nic_main->nic_paii_regs);
	*arbiter_version = value;

}

void hal_nic_reset_captured_data(u8 master_id, nic_cap_type cap_type)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	if (cap_type == CAP_READ)
		arbiter_base_register->ar_total_data = 0;
	else
		arbiter_base_register->aw_total_data = 0;
}

u32 hal_nic_get_captured_data(u8 master_id, nic_cap_type cap_type)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;
	u32 data_width;

	if (master_id >= MASTER_MAX_CNT)
		return 0;
	
	switch (master_id)
	{
		case SPI_NOR_MA:
		case NBS_MA:
		case GMAC_MA:
			data_width = 32;
			break;
		case CA55_M0:
		case CSETR_M2:
		case NPU_MA:
		case AXI_DMA_M0:
		case AXI_DMA_M1:	
		case CBDMA0_MA:
		case CPIOR0_MA:
		case CPIOR1_MA:
		case DUMMY0_MA:
		case SLAM_MA:
		case DUMMY1_MA:
			data_width = 128;
			break;
		default:
			data_width = 64;
			break;
	}

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	#if 0
	if (cap_type == CAP_READ)
		return (arbiter_base_register->ar_total_data * data_width / 8);
	else
		return (arbiter_base_register->aw_total_data * data_width / 8);
	#else
	
	if (cap_type == CAP_READ)
		return (arbiter_base_register->ar_total_data);
	else
		return (arbiter_base_register->aw_total_data); 
	#endif
}

void hal_nic_disable_arbiter(u8 master_id)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	arbiter_base_register->arb_ctrl.disable_arbiter = 1;
}

void hal_nic_enable_arbiter(u8 master_id)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	arbiter_base_register->arb_ctrl.disable_arbiter = 0;
}

void hal_nic_set_priority_weight(u8 master_id, u8 prio, u8 weight)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	arbiter_base_register->arb_ctrl.prio_level = (prio & 0x03);
	arbiter_base_register->arb_ctrl.latency_weight = (weight & 0x03);
}

void hal_nic_get_priority_weight(u8 master_id, u8 *prio, u8 *weight)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	*prio = arbiter_base_register->arb_ctrl.prio_level;
	*weight = arbiter_base_register->arb_ctrl.latency_weight;
}

u8 hal_nic_get_default_qos(u8 master_id, nic_cap_type cap_type)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return 0;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	

	if (cap_type == CAP_READ)
		return arbiter_base_register->arb_ctrl.arqos;
	else
		return arbiter_base_register->arb_ctrl.awqos;
}

void hal_nic_set_default_qos(u8 master_id, u8 qos, nic_cap_type cap_type)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	if (cap_type == CAP_READ)
		arbiter_base_register->arb_ctrl.arqos = (qos & 0xf);
	else
		arbiter_base_register->arb_ctrl.awqos = (qos & 0xf);
}

void hal_nic_get_rate_limit(u8 master_id, nic_cap_type cap_type, u8 *enable, u8 *max_count, u16 *issue_cycle)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	if (cap_type == CAP_READ) {
		*enable = arbiter_base_register->arb_ar_issue_rate.rate_limit_en;
		*max_count = arbiter_base_register->arb_ar_issue_rate.max_count;
		*issue_cycle = arbiter_base_register->arb_ar_issue_rate.issue_cycle;
	} else {
		*enable = arbiter_base_register->arb_aw_issue_rate.rate_limit_en;
		*max_count = arbiter_base_register->arb_aw_issue_rate.max_count;
		*issue_cycle = arbiter_base_register->arb_aw_issue_rate.issue_cycle;
	}
}

void hal_nic_set_rate_limit(u8 master_id, u8 enable, u8 max_count, u16 issue_cycle)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	arbiter_base_register->arb_aw_issue_rate.rate_limit_en = enable;
	arbiter_base_register->arb_aw_issue_rate.max_count = (max_count & 0xf);
	arbiter_base_register->arb_aw_issue_rate.issue_cycle = (issue_cycle & 0xfff);

	arbiter_base_register->arb_ar_issue_rate.rate_limit_en = enable;
	arbiter_base_register->arb_ar_issue_rate.max_count = (max_count & 0x0f);
	arbiter_base_register->arb_ar_issue_rate.issue_cycle = (issue_cycle & 0xfff);
}

void hal_nic_set_bw_latency(u8 master_id, u16 alloc_bw, u16 max_latency)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	arbiter_base_register->arb_aw_bw_info.alloc_bw = (alloc_bw & 0xfff);
	arbiter_base_register->arb_aw_bw_info.max_latency = max_latency;

	arbiter_base_register->arb_ar_bw_info.alloc_bw = (alloc_bw & 0xfff);
	arbiter_base_register->arb_ar_bw_info.max_latency = max_latency;
}

u16 hal_nic_get_bw(u8 master_id, nic_cap_type cap_type)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return 0;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	if (cap_type == CAP_READ)
		return (arbiter_base_register->arb_ar_bw_info.alloc_bw);
	else
		return (arbiter_base_register->arb_aw_bw_info.alloc_bw);
}

u16 hal_nic_get_max_latency(u8 master_id, nic_cap_type cap_type)
{
	nic_amba_master_arbiter_regs *arbiter_base_register;

	if (master_id >= MASTER_MAX_CNT)
		return 0;

	if(master_id < SLAM_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_main_regs + 16 + master_id * 28);
	else if(master_id < IMGREAD0_MA)
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_pai_regs + 16 + master_id * 28);
	else 
		arbiter_base_register = (nic_amba_master_arbiter_regs *)(nic_main->nic_paii_regs + 16 + master_id * 28);
	
	if (cap_type == CAP_READ)
		return (arbiter_base_register->arb_ar_bw_info.max_latency);
	else
		return (arbiter_base_register->arb_aw_bw_info.max_latency);
}

