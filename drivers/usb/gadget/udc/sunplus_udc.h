/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
 *
 * linux/kernel/drivers/usb/gadget/sunplus_udc.h
 * Sunplus on-chip full/high speed USB device controllers
 *
 * Copyright (C) 2004-2007 Herbert Pötzl - Arnaud Patard
 *	Additional cleanups by Ben Dooks <ben-linux@fluff.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _SUNPLUS_UDC_H
#define _SUNPLUS_UDC_H

#include <linux/interrupt.h>
#include <linux/usb/gadget.h>

//#define CONFIG_BOOT_ON_ZEBU

#define TRANS_MODE			DMA_MAP
#define DMA_MODE			0			/* if all DRAMs locate within 4GB */
#define DMA_MAP				1

#define CACHE_LINE_SIZE			64
#define USB_PORT_NUM			3

/* uphy0 */
#define GLO_CTRL1_OFFSET		0x74
#define CLK120_27_SEL			BIT(19)
#define GLO_CTRL2_OFFSET		0x78
#define PLL_PD_SEL			BIT(7)
#define PLL_PD				BIT(3)

/* moon3 */
#define M3_SCFG_22			0x58

/* moon4 */
#define M4_SCFG_10			0x28

#define MO1_USBC0_USB0_TYPE		BIT(2)
#define MASK_MO1_USBC0_USB0_TYPE	BIT(2 + 16)
#define MO1_USBC0_USB0_SEL		BIT(1)
#define MASK_MO1_USBC0_USB0_SEL		BIT(1 + 16)
#define MO1_USBC0_USB0_CTRL		BIT(0)
#define MASK_MO1_USBC0_USB0_CTRL	BIT(0 + 16)
#define USB_HOST_MODE			(MO1_USBC0_USB0_SEL | MO1_USBC0_USB0_CTRL)
#define USB_DEVICE_MODE			MO1_USBC0_USB0_CTRL
#define MASK_USB_HOST_DEVICE_MODE	(MASK_MO1_USBC0_USB0_TYPE | MASK_MO1_USBC0_USB0_SEL | \
									MASK_MO1_USBC0_USB0_CTRL)

/* run speeed & max ep condig config */
#define UDC_FULL_SPEED			0x1
#define UDC_HIGH_SPEED			0x3

#define UDC_MAX_ENDPOINT_NUM		9
#define EP0				0
#define EP1				1
#define EP2				2
#define EP3				3
#define EP4				4
#define EP5				5
#define EP6				6
#define EP7				7
#define EP8				8

#define AHB_USBD_BASE0			0x9c102800
#define AHB_USBD_END0			0x9c102b00
#define AHB_USBD_BASE1			0x9c103800
#define AHB_USBD_END1			0x9c103b00

#define IRQ_USB_DEV_PORT0		182
#define IRQ_USB_DEV_PORT1		186

#define EP_FIFO_SIZE			64
#define BULK_MPS_SIZE			512
#define INT_MPS_SIZE			64
#define ISO_MPS_SIZE			1024

#define UDC_EP_TYPE_CTRL		0
#define UDC_EP_TYPE_ISOC		1
#define UDC_EP_TYPE_BULK		2
#define UDC_EP_TYPE_INTR		3

/* event ring config */
#define ENTRY_SIZE			(16)
#define TR_COUNT			(64)			/* there is no limitation */
#define EVENT_SEG_COUNT			1			/* max = 2^ERST_Max. ERST_Max = 3 */

#define EVENT_RING_SIZE			(16 * TR_COUNT)
#define EVENT_RING_COUNT		(TR_COUNT)
#define TRANSFER_RING_SIZE		(16 * TR_COUNT)
#define TRANSFER_RING_COUNT		(TR_COUNT)

/* sw desc define  */
#define NOT_AUTO_SETINT			0x0
#define AUTO_SET_CONF			0x1			/* setConfiguration auto response */
#define AUTO_SET_INF			0x2			/* set interface auto response */
#define AUTO_SET_ADDR			0x4			/* set address auto response */
#define AUTO_REQ_ERR			0x8			/* request error auto response */
#define AUTO_RESPONSE			0xff

#define FRAME_TRANSFER_NUM_0		0x0			/* number of ISO/INT transfer */
								/* 1 micro frame */
#define FRAME_TRANSFER_NUM_1		0x1
#define FRAME_TRANSFER_NUM_2		0x2
#define FRAME_TRANSFER_NUM_3		0x3

/* alignment length define */
#define ALIGN_64_BYTE			BIT(6)
#define ALIGN_4_BYTE			ALIGN_64_BYTE		// (1 << 2)
#define ALIGN_8_BYTE			ALIGN_64_BYTE		// (1 << 3)
#define ALIGN_16_BYTE			ALIGN_64_BYTE		// (1 << 4)
#define ALIGN_32_BYTE			ALIGN_64_BYTE		// (1 << 5)

#define HAL_UDC_ENDPOINT_NUMBER_MASK	0xF
#define EP_NUM(ep_addr)			((ep_addr) & HAL_UDC_ENDPOINT_NUMBER_MASK)
#define EP_DIR(ep_addr)			(((ep_addr) >> 7) & 0x1)
#define SHIFT_LEFT_BIT4(x)		((u32)(x) >> 4)

/* regester define*/
#define DEVC_INTR_ENABLE		BIT(1)			/* DEVC_IMAN */
#define DEVC_INTR_PENDING		BIT(0)			/* DEVC_IMAN */

#define IMOD				(0)			/* DEVC_IMOD Interrupt moderation */
#define EHB				BIT(3)			/* DEVC_ERDP */
#define DESI				(0x7)			/* DEVC_ERDP */
#define VBUS_DIS			BIT(16)			/* DEVC_CTRL */
#define VBUS				BIT(16)			/* DEVC_STS */
#define EINT				BIT(1)			/* DEVC_STS */
#define VBUS_CI				BIT(0)			/* DEVC_STS */
#define CLEAR_INT_VBUS			(0x3)			/* DEVC_STS */

#define EP_EN				BIT(0)
#define RDP_EN				BIT(1)			/* reload dequeue pointer */
#define EP_STALL			BIT(2)
#define UDC_RUN				BIT(31)			/* DEV_RS */

#define DRIVE_RESUME			BIT(10)			/* GL_CS */

#define ADDR_VLD			BIT(23)
#define DEV_ADDR			(0x7F)
#define FRNUM				(0x3FFF)

#define UPHY0_CLKEN			BIT(5)
#define USBC0_CLKEN			BIT(4)
#define UPHY0_RESET			BIT(5)
#define USBC0_RESET			BIT(4)

/* transfer ring normal TRB */
#define TRB_C				BIT(0)			/* entry3 */
#define TRB_ISP				BIT(2)
#define TRB_IOC				BIT(5)
#define TRB_IDT				BIT(6)
#define TRB_BEI				BIT(9)
#define TRB_DIR				BIT(16)
#define TRB_FID(x)			(((x) >> 20) & 0x000007FF)	/* ISO */
#define TRB_SIA				BIT(31)			/* ISO */

/* transfer ring Link TRB */
#define	LTRB_PTR(x)			(((x) >> 4) & 0x0FFFFFFF)	/* entry0 */
#define LTRB_C				BIT(0)			/* entry3 */
#define LTRB_TC				BIT(5)
#define LTRB_IOC			BIT(5)
#define LTRB_TYPE(x)			(((x) & 0xfc00) >> 10)

/* Device controller event TRB */
#define ETRB_CC(x)			(((x) >> 24) & 0xFF)	/* entry2 */
#define ETRB_C(x)			((x) & (1 << 0))
#define ETRB_TYPE(x)			(((x) >> 10) & 0x3F)

/* Setup stage event TRB */
#define ETRB_SDL(x)			(x)			/* entry0 : wValue, bRequest */
								/*	       bmRequestType */
#define ETRB_SDH(x)			(x)			/* entry1 : wLength, wIndex */
#define	ETRB_ENUM(x)			(((x) >> 16) & 0xF)	/* entry3 */

/* Transfer event TRB */
#define	ETRB_TRBP(x)			(x)			/* entry0 */
#define	ETRB_LEN(x)			((x) & 0xFFFFFF)	/* entry2 */
#define	ETRB_EID(x)			(((x) >> 16) & 0x1F)	/* entry3 */

/* SOF event TRB */
#define ETRB_FNUM(x)			((x) & 0x1FFFF)		/* entry2 */
#define	ETRB_SLEN(x)			(((x) >> 11) & 0x1FFF)

#define ERDP_MASK			0xFFFFFFF0

/* TRB type */
#define NORMAL_TRB			1
#define SETUP_TRB			2
#define LINK_TRB			6
#define TRANS_EVENT_TRB			32
#define DEV_EVENT_TRB			48
#define SOF_EVENT_TRB			49

/* completion codes */
#define INVALID_TRB			0
#define SUCCESS_TRB			1
#define DATA_BUF_ERR			2
#define BABBLE_ERROR			3
#define TRANS_ERR			4
#define TRB_ERR				5
#define RESOURCE_ERR			7
#define SHORT_PACKET			13
#define EVENT_RING_FULL			21
#define UDC_STOPED			26
#define UDC_RESET			192
#define UDC_SUSPEND			193
#define UDC_RESUME			194

#define	DMA_ADDR_INVALID		(~(dma_addr_t)0)

/* USB device power state */
enum udc_power_state {
	UDC_POWER_OFF = 0,		/* USB Device power on */
	UDC_POWER_LOW,			/* USB Device low power */
	UDC_POWER_FULL,			/* USB Device power off */
};

/* base TRB data struct */
struct trb_data {
	u32 entry0;
	u32 entry1;
	u32 entry2;
	u32 entry3;
};

struct udc_ring {
	struct trb_data *trb_va;	/* ring start pointer */
	dma_addr_t trb_pa;		/* ring phy address */
	struct trb_data *end_trb_va;	/* ring end trb address*/
	dma_addr_t end_trb_pa;		/* ring end trb phy address */
	u16 num_mem;			/* number of ring members */
};

/* device descriptor */
struct sp_desc {
	u32 entry0;
	u32 entry1;
	u32 entry2;
	u32 entry3;
};

/**********************************************
 * struct info
 **********************************************/
/* Transfer TRB data struct(normal TRB) */
struct normal_trb {
	u32 ptr;			/* Data buffer pointer */
	u32 rsvd1;			/* Reserved shall always be 0 */
	u32 tlen:17;			/* TRB transfer length */
	u32 rsvd2:15;			/* Reserved for future use */
	u32 cycbit:1;			/* Cycle bit */
	u32 rsvd3:1;			/* Reserved for future use */
	u32 isp:1;			/* Interrupt on short packet */
	u32 rsvd4:2;			/* Reserved for future use */
	u32 ioc:1;			/* Interrupt on completion */
	u32 idt:1;			/* Inmediate data */
	u32 rsvd5:2;			/* Reserved for future use */
	u32 bei:1;			/* Block event interrupt */
	u32 type:6;			/* TRB type */
	u32 dir:1;			/* OUT/IN transfer */
	u32 rsvd6:3;			/* Reserved */
	u32 fid:11;			/* Frame ID */
	u32 sia:1;			/* Start ISOchronous ASAP */
};

/* Transfer TRB data struct(link TRB) */
struct link_trb {
	u32 rsvd1:4;			/* Reserved shall always be 0 */
	u32 ptr:28;			/* Ring segment pointer */
	u32 rsvd2;			/* Reserved shall always be 0 */
	u32 rfu1;			/* Reserved for future use */
	u32 cycbit:1;			/* Cycle bit */
	u32 togbit:1;			/* Toggle bit */
	u32 rfu2:3;			/* Reserved for future use */
	u32 rfu3:5;			/* Reserved for future use */
	u32 type:6;			/* Rrb type */
	u32 rfu4:16;			/* Reserved for future use */
};

/* Event Ring segment table entry */
struct segment_table {
	u32 rsvd0:6;			/* Reserved shall always be 0 */
	u32 rsba:26;			/* Ring segment base address */
	u32 rsvd1;			/* Reserved for future use */
	u32 rssz:16;			/* Reserved for future use */
	u32 rsvd2:16;			/* Reserved for future use */
	u32 rsvd3;			/* Reserved for future use */
};

/* event TRB (Transfer) */
struct transfer_event_trb {
	u32 trbp;			/* The pointer of the TRB which generated this event */
	u32 rsvd1:32;			/* Reserved for future use */
	u32 len:24;			/* TRB transfer length */
	u32 cc:8;			/* Completion code */
	u32 cycbit:1;			/* Cycle bit */
	u32 rsvd2:9;			/* Reserved for future use */
	u32 type:6;			/* TRB type */
	u32 eid:5;			/* Endpoint ID */
	u32 rsvd3:11;			/* Reserved for future use */
};

/* event TRB (device) */
struct device_event_trb {
	u32 rfu1;			/* Reserved for future use */
	u32 rfu2;			/* Reserved for future use */
	u32 rfu3:24;			/* Reserved for future use */
	u32 cc:8;			/* Completion code */
	u32 cycbit:1;			/* Cycle bit */
	u32 rfu4:9;			/* Reserved for future use */
	u32 type:6;			/* TRB type */
	u32 rfu5:16;			/* Reserved for future use */
};

/* event TRB (setup) */
struct setup_trb_t {
	u32 sdl;			/* Low word of the setup data */
	u32 sdh;			/* High word of the setup data */
	u32 len:17;			/* Transfer length,always 8 */
	u32 rdu1:7;		/* Reserved for future use */
	u32 cc:8;				/* Completion code */
	u32 cycbit:1;			/* Cycle bit */
	u32 rfu:9;			/* Reserved for future use */
	u32 type:6;			/* TRB type */
	u32 epnum:4;			/* Endpoint number */
	u32 rfu2:12;			/* Reserved for future use */
};

/* Endpoint 0 descriptor data struct */
struct endpoint0_desc {
	u32 cfgs:8;		/* Device configure setting */
	u32 cfgm:8;		/* Device configure mask */
	u32 speed:4;		/* Device speed */
	u32 rsvd1:12;		/* Reserved for future use */
	u32 sofic:3;		/* SOF interrupt control */
	u32 rsvd2:1;		/* Reserved for future use */
	u32 aset:4;		/* Auto setup */
	u32 rsvd3:24;		/* Reserved for future use */
	u32 rsvd4:32;		/* Reserved for future use */
	u32 dcs:1;		/* De-queue cycle bit */
	u32 rsvd5:3;		/* Reserved,shall always be 0 */
	u32 dptr:28;		/* TR de-queue pointer */
};

/* Endpoint number 1~15 descriptor data struct */
struct endpointn_desc {
	u32 ifs:8;		/* Interface setting */
	u32 ifm:8;		/* Interface mask */
	u32 alts:8;		/* Alternated setting */
	u32 altm:8;		/* Alternated setting mask */
	u32 num:4;		/* Endpoint number */
	u32 type:2;		/* Endpoint type */
	u32 rsvd1:26;		/* Reserved for future use */
	u32 mps:16;		/* Maximum packet size of endpoint */
	u32 rsvd2:14;		/* Reserved for future use */
	u32 mult:2;		/* Ror high speed device */
	u32 dcs:1;		/* De-queue cycle bit state */
	u32 rsvd3:3;		/* Reserved,shall always be 0 */
	u32 dptr:28;		/* TR de-queue pointer */
};

/* USB Device regiester */
struct udc_reg {
	/* Group0 */
	u32 DEVC_VER;
	u32 DEVC_MMR;
	u32 DEVC_REV0[2];
	u32 DEVC_PARAM;
	u32 DEVC_REV1[3];
	u32 GL_CS;
	u32 DEVC_REV2[23];

	/* Group1 */
	u32 DEVC_IMAN;
	u32 DEVC_IMOD;
	u32 DEVC_ERSTSZ;
	u32 DEVC_REV3[1];
	u32 DEVC_ERSTBA;
	u32 DEVC_REV4[1];
	u32 DEVC_ERDP;
	u32 DEVC_REV5[1];
	u32 DEVC_ADDR;
	u32 DEVC_REV6[1];
	u32 DEVC_CTRL;
	u32 DEVC_STS;
	u32 DEVC_REV7[18];
	u32 DEVC_DTOGL;
	u32 DEVC_DTOGH;

	/* Group2 */
	u32 DEVC_CS;
	u32 EP0_CS;
	u32 EPN_CS[30];

	/* Group3 */
	u32 MAC_FSM;
	u32 DEV_EP0_FSM;
	u32 EPN_FSM[30];

	/* Group4 */
	u32 USBD_CFG;
	u32 USBD_INF;
	u32 EPN_INF[30];

	/* Group5 */
	u32 USBD_FRNUM_ADDR;
	u32 USBD_DEBUG_PP;
	#define PP_PORT_IDLE   0x0
	#define PP_FULL_SPEED  0x10
	#define PP_HIGH_SPEED  0x18
	//uint32_t USBD_REV8[30];
};

struct sp_request {
	struct list_head	queue;			/* ep's requests */
	struct usb_request	req;
	struct trb_data		*transfer_trb;		/* pointer transfer trb*/
};

/* USB Device endpoint struct */
struct udc_endpoint {
	bool			is_in;			/* Endpoint direction */
	#define			ENDPOINT_HALT	(1)
	#define			ENDPOINT_READY	(0)
	u16		status;			/* Endpoint status */
	u8			num;			/* Endpoint number 0~15 */
	u8			type;			/* Endpoint type 0~3 */
	u8			*transfer_buff;		/* Pointer to transfer buffer */
	dma_addr_t		transfer_buff_pa;
	u32			transfer_len;		/* transfer length */
	u16			maxpacket;		/* Endpoint Max packet size */

	u8			bEndpointAddress;
	u8			bmAttributes;

	struct usb_ep		ep;
	struct sp_udc		*dev;
	struct usb_gadget	*gadget;
	struct udc_ring		ep_transfer_ring;	/* One transfer ring per ep */
	struct trb_data		*ep_trb_ring_dq;	/* transfer ring dequeue */
	spinlock_t		lock;
	struct list_head	queue;
};

extern void __iomem	*uphy0_regs;
void __iomem		*moon3_reg;
void __iomem		*moon4_reg;

struct sp_udc {
	enum usb_dr_mode		dr_mode;
	/* auto set flag, If this flag is true, zero packet will not be sent */
	bool				aset_flag;
	struct phy			*uphy[USB_PORT_NUM];
	struct reset_control		*rstc;
	struct clk			*clock;
	int				irq_num;
	struct usb_phy			*usb_phy;
	int				port_num;
	u32				frame_num;
	bool				bus_reset_finish;
	bool				def_run_full_speed;		/* default high speed */
	struct timer_list		sof_polling_timer;
	struct timer_list		before_sof_polling_timer;
	bool				vbus_active;
	int				usb_test_mode;			/* USB IF device test */

	spinlock_t			lock;				/* */
	struct tasklet_struct		event_task;
	struct usb_gadget		gadget;
	struct usb_gadget_driver	*driver;
	struct device			*dev;

	struct udc_reg			*reg;				/* UDC Register  */
	struct sp_desc			*ep_desc;			/* ep description pointer */
	dma_addr_t			ep_desc_pa;			/* ep desc phy address */
	u8				event_ccs;			/* Consumer Cycle state */
	u8				current_event_ring_seg;		/* current ER seg index */
	u8				event_ring_seg_total;		/* Total number of ER seg */
	struct segment_table		*event_seg_table;		/* evnet seg */
	dma_addr_t			event_seg_table_pa;
	struct udc_ring			*event_ring;			/* all seg ER pointer */
	dma_addr_t			event_ring_pa;			/* ER pointer phy address */
	struct trb_data			*event_ring_dq;			/* event ring dequeue */
	struct udc_endpoint		ep_data[UDC_MAX_ENDPOINT_NUM];	/* endpoint data struct */
	struct usb_otg_caps		*otg_caps;
};

int32_t hal_udc_init(struct sp_udc *udc);
int32_t hal_udc_deinit(struct sp_udc *udc);
int32_t hal_udc_device_connect(struct sp_udc *udc);
int32_t hal_udc_device_disconnect(struct sp_udc *udc);
int32_t hal_udc_endpoint_stall(struct sp_udc *udc, u8 ep_addr, bool stall);
int32_t hal_udc_power_control(struct sp_udc *udc, enum udc_power_state power_state);
int32_t hal_udc_endpoint_transfer(struct sp_udc	*udc, struct sp_request *req, u8 ep_addr,
				  u8 *data, dma_addr_t data_pa, uint32_t length, uint32_t zero);
int32_t hal_udc_endpoint_unconfigure(struct sp_udc *udc, u8 ep_addr);
int32_t hal_udc_endpoint_configure(struct sp_udc *udc, u8 ep_addr, u8 ep_type,
				   uint16_t ep_max_packet_size);
#endif
