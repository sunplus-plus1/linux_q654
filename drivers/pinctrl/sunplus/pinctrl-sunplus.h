/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __PINCTRL_SUNPLUS_H
#define __PINCTRL_SUNPLUS_H

#define MNAME "sppctl"
#define M_LIC "GPL v2"
#define M_AUT "Yubo Leng <yb.leng@sunmedia.com.cn>"
#define M_NAM "SP7350 PinCtl"
#define M_ORG "Sunplus Tech."
#define M_CPR "(C) 2023"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/sysfs.h>
#include <linux/printk.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <dt-bindings/pinctrl/sppctl-sp7350.h>

//#define CONFIG_PINCTRL_SPPCTL_DEBUG 1

#define SPPCTL_MAX_NAM 64
#define SPPCTL_MAX_BUF PAGE_SIZE

#define SPPCTL_MUXABLE_MIN 8
#define SPPCTL_MUXABLE_MAX 71

#define KINF(pd, fmt, args...)                           \
	do {                                             \
		if ((pd) != NULL)                        \
			dev_info((pd), fmt, ##args);     \
		else                                     \
			pr_info(MNAME ": " fmt, ##args); \
	} while (0)
#define KERR(pd, fmt, args...)                          \
	do {                                            \
		if ((pd) != NULL)                       \
			dev_info((pd), fmt, ##args);    \
		else                                    \
			pr_err(MNAME ": " fmt, ##args); \
	} while (0)
#ifdef CONFIG_PINCTRL_SPPCTL_DEBUG
#define KDBG(pd, fmt, args...)                            \
	do {                                              \
		if ((pd) != NULL)                         \
			dev_info((pd), fmt, ##args);      \
		else                                      \
			pr_debug(MNAME ": " fmt, ##args); \
	} while (0)
#else
#define KDBG(pd, fmt, args...)
#endif

#define D_PIS(x) "GPIO" __stringify(x)

#define EGRP(n, v, p)                                                       \
	{                                                                   \
		.name = n, .gval = (v), .pins = (p), .pnum = ARRAY_SIZE(p), \
	}
#define FNCE(n, r, o, bo, bl, g)                                         \
	{                                                                \
		.name = n, .freg = r, .roff = o, .boff = bo, .blen = bl, \
		.grps = (g), .gnum = ARRAY_SIZE(g),                      \
	}

#define FNCN(n, r, o, bo, bl)                                            \
	{                                                                \
		.name = n, .freg = r, .roff = o, .boff = bo, .blen = bl, \
		.grps = NULL, .gnum = 0,                                 \
	}

// FIRST: MUX=0, GPIO=1
enum MUX_FIRST_MG_t {
	MUX_FIRST_M = 0,
	MUX_FIRST_G = 1,
	MUX_FIRST_KEEP = 2,
};

// MASTER: IOP=0,GPIO=1
enum MUX_MASTER_IG_t {
	MUX_MASTER_I = 0,
	MUX_MASTER_G = 1,
	MUX_MASTER_KEEP = 2,
};

enum F_OFF_t {
	F_OFF_0, // nowhere
	F_OFF_M, // in mux registers
	F_OFF_G, // mux group registers
	F_OFF_I, // in iop registers
};

struct sppctl_pdata_t {
	u8 debug;
	void *sysfs_sdp;
	void __iomem *gpioxt_regs_base; // MASTER , OE , OUT , IN
	void __iomem *first_regs_base; // GPIO_FIRST
	void __iomem *padctl1_regs_base; // PAD CTRL1
	void __iomem *padctl2_regs_base; // PAD CTRL2
	void __iomem *moon1_regs_base; // PIN-GROUP
#if defined(SUPPORT_GPIO_AO_INT)
	void __iomem *gpio_ao_int_regs_base; // GPIO_AO_INT
#endif
	// pinctrl-related
	struct pinctrl_desc pdesc;
	struct pinctrl_dev *pcdp;
	struct pinctrl_gpio_range gpio_range;
	struct sppctlgpio_chip_t *gpiod;
};

struct sppctl_reg_t {
	u16 v; // value part
	u16 m; // mask part
};

struct sppctlgrp_t {
	const char *const name;
	const u8 gval; // value for register
	const unsigned *const pins; // list of pins
	const unsigned int pnum; // number of pins
};

struct func_t {
	const char *const name;
	const enum F_OFF_t freg; // function register type
	const u8 roff; // register offset
	const u8 boff; // bit offset
	const u8 blen; // number of bits
	const struct sppctlgrp_t *const grps; // list of groups
	const unsigned int gnum; // number of groups
	const char *grps_sa[12]; // array of pointers to func's grps names
};

struct grp2fp_map_t {
	u16 f_idx; // function index
	u16 g_idx; // pins/group index inside function
};

struct sppctlgpio_chip_t {
	spinlock_t lock; //spin lock
	struct gpio_chip chip;

	void __iomem
		*gpioxt_regs_base; // MASTER, OE, OUT, IN (, I_INV, O_INV, OD)
	void __iomem *first_regs_base; // GPIO_FIRST
	void __iomem *padctl1_regs_base; // PAD CTRL1
	void __iomem *padctl2_regs_base; // PAD CTRL2
#if defined(SUPPORT_GPIO_AO_INT)
	void __iomem *gpio_ao_int_regs_base; // GPIO_AO_INT
	u32 gpio_ao_int_prescale;
	u32 gpio_ao_int_debounce;
	int gpio_ao_int_pins[32];
#endif
};

int sppctl_pinctrl_init(struct platform_device *pdev);
void sppctl_pinctrl_clean(struct platform_device *pdev);
void sppctl_gmx_set(struct sppctl_pdata_t *pdata, u8 reg_offset, u8 bit_offset,
		    u8 bit_nums, u8 bit_value);
u8 sppctl_gmx_get(struct sppctl_pdata_t *pdata, u8 reg_offset, u8 bit_offset,
		  u8 bit_nums);
void sppctl_pin_set(struct sppctl_pdata_t *pdata, u8 pin_selector,
		    u8 func_selector);
u8 sppctl_fun_get(struct sppctl_pdata_t *pdata, u8 func_selector);

extern const char *const sppctlgpio_list_s[];
extern const size_t GPIS_list_size;
extern struct func_t list_funcs[];
extern const size_t list_func_nums;

#endif // __PINCTRL_SUNPLUS_H
