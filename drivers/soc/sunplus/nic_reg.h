#ifndef __NIC_REG_H__
#define __NIC_REG_H__

typedef volatile struct st_nic_amba_master_arbiter_regs
{

	struct arb_ctrl_t
	{
		u32 arqos		: 4;
		u32 awqos		: 4;
		u32			: 4;
		u32 disable_arbiter	: 1;
		u32			: 3;
		u32 prio_level	: 2;
		u32 			: 2;
		u32 latency_weight	: 2;
		u32 			: 10;
	} arb_ctrl;

	struct arb_aw_bw_info_t
	{
		u32 alloc_bw		: 12;
		u32 			: 4;
		u32 max_latency	: 16;
	} arb_aw_bw_info;

	struct arb_ar_bw_info_t
	{
		u32 alloc_bw		: 12;
		u32 			: 4;
		u32 max_latency	: 16;
	} arb_ar_bw_info;

	struct arb_aw_issue_rate_t
	{
		u32 issue_cycle	: 12;
		u32 max_count	: 4;
		u32 rate_limit_en	: 1;
		u32			: 15;
	} arb_aw_issue_rate;

	struct arb_ar_issue_rate_t
	{
		u32 issue_cycle	: 12;
		u32 max_count	: 4;
		u32 rate_limit_en	: 1;
		u32			: 15;
	} arb_ar_issue_rate;

	u32 aw_total_data;
	u32 ar_total_data;

} nic_amba_master_arbiter_regs;

typedef volatile struct arbiter_version_reg_t
{
	u32 bug_version		: 8;
	u32 feature_version		: 4;
	u32 arch_version		: 4;
	u32 month			: 4;
	u32 era			: 12;
} arb_version_reg;

typedef volatile struct arbiter_global_reg_t
{
	u32 arb_period		: 12;
	u32 				: 20;
} arb_global_reg;

#endif	/* __NIC_REG_H__ */

