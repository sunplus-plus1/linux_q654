// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/io.h>

#include "sppctl_gpio_ops.h"
#include "sppctl_gpio.h"

__attribute((unused)) static irqreturn_t gpio_int_0(int irq, void *data)
{
	pr_info("register gpio int0 trigger\n");
	return IRQ_HANDLED;
}

int sppctl_gpio_new(struct platform_device *pdev, void *platform_data)
{
	struct sppctl_pdata_t *pdata;
	struct sppctlgpio_chip_t *pc;
	struct gpio_chip *gchip;
	struct device_node *npi;
	struct device_node *np;
	int err = 0;
	int i = 0;

	pdata = (struct sppctl_pdata_t *)platform_data;

	np = pdev->dev.of_node;
	if (!np) {
		KERR(&pdev->dev, "invalid devicetree node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(np)) {
		KERR(&pdev->dev, "devicetree status is not available\n");
		return -ENODEV;
	}

	// print_device_tree_node(np, 0);
	for_each_child_of_node(np, npi) {
		if (of_find_property(npi, "gpio-controller", NULL)) {
			i = 1;
			break;
		}
	}

	if (of_find_property(np, "gpio-controller", NULL))
		i = 1;
	if (i == 0) {
		KERR(&pdev->dev, "is not gpio-controller\n");
		return -ENODEV;
	}

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;
	gchip = &pc->chip;

	pc->gpioxt_regs_base = pdata->gpioxt_regs_base;
	pc->first_regs_base = pdata->first_regs_base;
	pc->padctl1_regs_base = pdata->padctl1_regs_base;
	pc->padctl2_regs_base = pdata->padctl2_regs_base;
#if defined(SUPPORT_GPIO_AO_INT)
	pc->gpio_ao_int_regs_base = pdata->gpio_ao_int_regs_base;
#endif
	pdata->gpiod = pc;

#if defined(SUPPORT_GPIO_AO_INT)
	if (of_property_read_u32(np, "sunplus,ao-pin-prescale",
				 &pc->gpio_ao_int_prescale) == 0) {
		writel(pc->gpio_ao_int_prescale & 0xfffff,
		       pc->gpio_ao_int_regs_base); // PRESCALE
	}

	if (of_property_read_u32(np, "sunplus,ao-pin-debounce",
				 &pc->gpio_ao_int_debounce) == 0) {
		writel(pc->gpio_ao_int_prescale & 0xff,
		       pc->gpio_ao_int_regs_base + 4); // DEB_TIME
	}

	for (i = 0; i < 32; i++)
		pc->gpio_ao_int_pins[i] = -1;
#endif

	gchip->label = MNAME;
	gchip->parent = &pdev->dev;
	gchip->owner = THIS_MODULE;
	gchip->request = sppctl_gpio_request;
	gchip->free = sppctl_gpio_free;
	gchip->get_direction = sppctl_gpio_get_direction;
	gchip->direction_input = sppctl_gpio_direction_input;
	gchip->direction_output = sppctl_gpio_direction_output;
	gchip->get = sppctl_gpio_get_value;
	gchip->set = sppctl_gpio_set_value;
	gchip->set_config = sppctl_gpio_set_config;
	gchip->dbg_show = sppctl_gpio_dbg_show;
	gchip->base = 0; // it is main platform GPIO controller
	gchip->ngpio = GPIS_list_size;
	gchip->names = sppctlgpio_list_s;
	gchip->can_sleep = 0;
#if defined(CONFIG_OF_GPIO)
	gchip->of_node = np;
#endif
	gchip->to_irq = sppctl_gpio_to_irq;

	pdata->gpio_range.npins = gchip->ngpio;
	pdata->gpio_range.base = gchip->base;
	pdata->gpio_range.name = gchip->label;
	pdata->gpio_range.gc = gchip;

	// FIXME: can't set pc globally
	err = devm_gpiochip_add_data(&pdev->dev, gchip, pc);
	if (err < 0) {
		KERR(&pdev->dev, "gpiochip add failed\n");
		return err;
	}

	spin_lock_init(&pc->lock);

	return 0;
}

int sppctl_gpio_del(struct platform_device *pdev, void *platform_data)
{
	//struct sppctlgpio_chip_t *cp;

	// FIXME: can't use globally now
	//cp = platform_get_drvdata(pdev);
	//if (cp == NULL)
	//	return -ENODEV;
	//gpiochip_remove(&(cp->chip));
	// FIX: remove spinlock_t ?
	return 0;
}
