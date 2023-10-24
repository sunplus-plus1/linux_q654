/* SPDX-License-Identifier: GPL-2.0 */
/*
 * GPIO Driver for Sunplus/Tibbo SP7021 controller
 * Copyright (C) 2020 Sunplus Tech./Tibbo Tech.
 * Author: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef SPPCTL_GPIO_H
#define SPPCTL_GPIO_H

#define SPPCTL_GPIO_IRQS 8

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/gpio/driver.h>
#include <linux/stringify.h>

#include "sppctl.h"

#ifndef SPPCTL_H

#define MNAME "sp7350_gpio"
#define M_NAM "SP7350 GPIO"
#define M_LIC "GPL v2"
#define M_AUT1 "Dvorkin Dmitry <dvorkin@tibbo.com>"
#define M_AUT2 "Wells Lu <wells.lu@sunplus.com>"

#define M_ORG "Sunplus/Tibbo Tech."
#define M_CPR "(C) 2020"

#define KINF(pd, fmt, args...) \
	do { \
		if ((pd) != NULL) \
			dev_info((pd), fmt, ##args); \
		else \
			pr_info(MNAME ": " fmt, ##args); \
	} while (0)
#define KERR(pd, fmt, args...) \
	do { \
		if ((pd) != NULL) \
			dev_info((pd), fmt, ##args); \
		else \
			pr_err(MNAME ": " fmt, ##args); \
	} while (0)
#ifdef CONFIG_DEBUG_GPIO
#define KDBG(pd, fmt, args...) \
	do { \
		if ((pd) != NULL) \
			dev_info((pd), fmt, ##args); \
		else \
			pr_debug(MNAME ": " fmt, ##args); \
	} while (0)
#else
#define KDBG(pd, fmt, args...)
#endif

#endif // SPPCTL_H

struct sppctlgpio_chip_t {
	spinlock_t lock;
	struct gpio_chip chip;
	void __iomem *base0;   // MASTER, OE, OUT, IN (, I_INV, O_INV, OD)
	void __iomem *base2;   // GPIO_FIRST
	int irq[SPPCTL_GPIO_IRQS];
	int irq_pin[SPPCTL_GPIO_IRQS];
#if defined(SUPPORT_GPIO_AO_INT)
	void __iomem *baseA;   // GPIO_AO_INT
	uint32_t gpio_ao_int_prescale;
	uint32_t gpio_ao_int_debounce;
	int gpio_ao_int_pins[32];
#endif
};

extern const char * const sppctlgpio_list_s[];
extern const size_t GPIS_listSZ;

int sppctl_gpio_new(struct platform_device *_pd, void *_datap);
int sppctl_gpio_del(struct platform_device *_pd, void *_datap);

#define D_PIS(x) "GPIO" __stringify(x)

// FIRST: MUX=0, GPIO=1
enum muxF_MG_t {
	muxF_M = 0,
	muxF_G = 1,
	muxFKEEP = 2,
};
// MASTER: IOP=0,GPIO=1
enum muxM_IG_t {
	muxM_I = 0,
	muxM_G = 1,
	muxMKEEP = 2,
};

#endif // SPPCTL_GPIO_H
