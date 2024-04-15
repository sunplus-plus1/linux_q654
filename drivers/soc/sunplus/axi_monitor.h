/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AXI_MONITOR_H__
#define __AXI_MONITOR_H__


void axi_mon_interrupt_control_mask(int enable);

// for test
//void axi_mon_test_init();

void axi_mon_unexcept_access_test(void __iomem *axi_mon_regs, void __iomem *axi_id4_regs, void __iomem *axi_id45_regs);
void axi_mon_timeout_test(void __iomem *axi_mon_regs);
//void axi_mon_bw_test();


#endif // __AXI_MONITOR_H__
