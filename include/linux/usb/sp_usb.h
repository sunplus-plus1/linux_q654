/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __SP_USB_H
#define __SP_USB_H

#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/usb/otg.h>

#define RF_MASK_V(mask, val)			(((mask) << 16) | (val))
#define RF_MASK_V_SET(mask0, mask1)		(((mask0) << 16) | (mask1))
#define RF_MASK_V_CLR(mask)			(((mask) << 16) | 0)

#define USB_PORT0_ID				0
#define USB_PORT1_ID				1
#define USB_PORT_NUM				3

#define PORT0_ENABLED				BIT(0)
#define PORT1_ENABLED				BIT(1)
#define PORT2_ENABLED				BIT(2)

#define	VBUS_GPIO_CTRL_0			90
#define	VBUS_GPIO_CTRL_1			91

#define CDP_MODE_VALUE				0
#define DCP_MODE_VALUE				1
#define SDP_MODE_VALUE				2
#define UPHY1_OTP_DISC_LEVEL_OFFSET		5
#define OTP_DISC_LEVEL_TEMP			0x16
#define DISC_LEVEL_DEFAULT			0x0b
#define OTP_DISC_LEVEL_BIT			0x1f
#define GET_BC_MODE				0xff00
#define APHY_PROBE_CTRL				0x38

#define POWER_SAVING_SET			BIT(5)
#define ECO_PATH_SET				BIT(6)
#define	UPHY_DISC_0				BIT(2)
#define APHY_PROBE_CTRL_MASK			0x38

#define USB_RESET_OFFSET			0x5c
#define PIN_MUX_CTRL				0x8c

#define USBC_CTL_OFFSET				0x28
#define UPHY0_CTL0_OFFSET			0x48
#define UPHY0_CTL1_OFFSET			0x4c
#define UPHY0_CTL2_OFFSET			0x50
#define UPHY0_CTL3_OFFSET			0x54
#define UPHY1_CTL0_OFFSET			0x58
#define UPHY1_CTL1_OFFSET			0x5c
#define UPHY1_CTL2_OFFSET			0x60
#define UPHY1_CTL3_OFFSET			0x64

#define CLK_REG_OFFSET				0xc

#define	POWER_SAVING_OFFSET			0x4

#define DISC_LEVEL_OFFSET			0x1c
#define	ECO_PATH_OFFSET				0x24
#define	UPHY_DISC_OFFSET			0x28
#define BIT_TEST_OFFSET				0x10
#define CDP_REG_OFFSET				0x40
#define	DCP_REG_OFFSET				0x44
#define	UPHY_INTR_OFFSET			0x4c
#define APHY_PROBE_OFFSET			0x5c
#define CDP_OFFSET				0

#define	UPHY_DEBUG_SIGNAL_REG_OFFSET		0x30
#define UPHY_INTER_SIGNAL_REG_OFFSET		0xc

// MOON0
#define USBC0_RESET_OFFSET			0x18

// UPHY0
#define GLO_CTRL0_OFFSET			0x70
#define GLO_CTRL1_OFFSET			0x74
#define GLO_CTRL2_OFFSET			0x78

#define PORT_OWNERSHIP				0x00002000
#define CURRENT_CONNECT_STATUS			0x00000001
#define EHCI_CONNECT_STATUS_CHANGE		0x00000002
#define OHCI_CONNECT_STATUS_CHANGE		0x00010000

#define WAIT_TIME_AFTER_RESUME			25
#define ELAPSE_TIME_AFTER_SUSPEND		15000
#define SEND_SOF_TIME_BEFORE_SUSPEND		15000
#define SEND_SOF_TIME_BEFORE_SEND_IN_PACKET	15000

extern u32 bc_switch;
extern u32 cdp_cfg16_value;
extern u32 cdp_cfg17_value;
extern u32 dcp_cfg16_value;
extern u32 dcp_cfg17_value;
extern u32 sdp_cfg16_value;
extern u32 sdp_cfg17_value;

extern int uphy0_irq_num;
extern int uphy1_irq_num;
extern void __iomem *uphy0_base_addr;
extern void __iomem *uphy1_base_addr;
extern void __iomem *uphy0_res_moon4;
extern void __iomem *uphy1_res_moon4;

extern void __iomem *uphy0_regs;

extern u8 max_topo_level;
extern bool tid_test_flag;
extern u8 sp_port0_enabled;
extern u8 sp_port1_enabled;
extern uint accessory_port_id;
extern bool enum_rx_active_flag[USB_PORT_NUM];
extern struct semaphore enum_rx_active_reset_sem[USB_PORT_NUM];
extern struct timer_list hnp_polling_timer;

extern u8 otg0_vbus_off;
extern u8 otg1_vbus_off;

void phy0_otg_ctrl(void);
void phy1_otg_ctrl(void);

void udc_otg_ctrl(void);
void usb_switch(int device);
void detech_start(void);

void sp_accept_b_hnp_en_feature(struct usb_otg *otg);

#define	ENABLE_VBUS_POWER(port)
#define	DISABLE_VBUS_POWER(port)

#define UHPOWERCS_PORT		0x10
#define	UPHY_SUSP_EN		BIT(10)
#define UPHY_SUSP_CTRL		BIT(8)

static inline void uphy_force_disc(int en, int port)
{
	void __iomem *reg_addr;
	u32 uphy_val;

	if (port > USB_PORT1_ID)
		pr_err("-- err port num %d\n", port);

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;
	uphy_val = readl(reg_addr + UPHY_DISC_OFFSET);
	if (en)
		uphy_val |= UPHY_DISC_0;
	else
		uphy_val &= ~UPHY_DISC_0;

	writel(uphy_val, reg_addr + UPHY_DISC_OFFSET);
}

static inline int get_uphy_swing(int port)
{
	void __iomem *uphy_ctl2_addr = port ? (uphy1_res_moon4 + UPHY1_CTL2_OFFSET)
						: (uphy0_res_moon4 + UPHY0_CTL2_OFFSET);
	u32 val;

	val = readl(uphy_ctl2_addr);

	return (val >> 8) & 0xff;
}

static inline int set_uphy_swing(u32 swing, int port)
{
	void __iomem *uphy_ctl2_addr = port ? (uphy1_res_moon4 + UPHY1_CTL2_OFFSET)
						: (uphy0_res_moon4 + UPHY0_CTL2_OFFSET);

	writel(RF_MASK_V_CLR(0x3f << 8), uphy_ctl2_addr);
	writel(RF_MASK_V_SET((swing & 0x3f) << 8, (swing & 0x3f) << 8), uphy_ctl2_addr);
	writel(RF_MASK_V_SET(1 << 15, 1 << 15), uphy_ctl2_addr);

	return 0;
}

static inline int get_disconnect_level(int port)
{
	void __iomem *reg_addr;
	u32 val;

	if (port > USB_PORT1_ID)
		return -1;

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;
	val = readl(reg_addr + DISC_LEVEL_OFFSET);

	return val & 0x1f;
}

static inline int set_disconnect_level(u32 disc_level, int port)
{
	void __iomem *reg_addr;
	u32 val;

	if (port > USB_PORT1_ID)
		return -1;

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;
	val = readl(reg_addr + DISC_LEVEL_OFFSET);
	val = (val & ~0x1f) | disc_level;
	writel(val, reg_addr + DISC_LEVEL_OFFSET);

	return 0;
}

static inline void reinit_uphy(int port)
{
	void __iomem *reg_addr;
	u32 val;

	reg_addr = port ? uphy1_base_addr : uphy0_base_addr;

	val = readl(reg_addr + ECO_PATH_OFFSET);
	val &= ~(ECO_PATH_SET);
	writel(val, reg_addr + ECO_PATH_OFFSET);
	val = readl(reg_addr + POWER_SAVING_OFFSET);
	val &= ~(POWER_SAVING_SET);
	writel(val, reg_addr + POWER_SAVING_OFFSET);

	#ifdef CONFIG_USB_BC
	writel(0x19, reg_addr + CDP_REG_OFFSET);
	writel(0x92, reg_addr + DCP_REG_OFFSET);
	writel(0x17, reg_addr + UPHY_INTER_SIGNAL_REG_OFFSET);
	#endif
}
#endif	/* __SP_USB_H */

