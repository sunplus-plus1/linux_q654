/* SPDX-License-Identifier: GPL-2.0 */

#ifndef SPPCTL_GPIO_H
#define SPPCTL_GPIO_H

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

extern const char *const sppctlgpio_list_s[];
extern const size_t GPIS_list_size;

int sppctl_gpio_new(struct platform_device *pdev, void *platform_data);
int sppctl_gpio_del(struct platform_device *pdev, void *platform_data);

#define D_PIS(x) "GPIO" __stringify(x)

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

#endif // SPPCTL_GPIO_H
