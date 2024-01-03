// SPDX-License-Identifier: GPL-2.0

#include "../core.h"
#include "../pinctrl-utils.h"
#include "../devicetree.h"
#include "sppctl_pinctrl.h"
#include "sppctl_gpio_ops.h"

char const **unq_grps;
size_t unique_groups_nums;
struct grp2fp_map_t *g2fp_maps;

#define PIN_CONFIG_INPUT_INVERT (PIN_CONFIG_END + 1)
#define PIN_CONFIG_OUTPUT_INVERT (PIN_CONFIG_END + 2)
#define PIN_CONFIG_SLEW_RATE_CTRL (PIN_CONFIG_END + 3)
#define PIN_CONFIG_BIAS_STRONG_PULL_UP (PIN_CONFIG_END + 4)

#ifdef CONFIG_GENERIC_PINCONF
static const struct pinconf_generic_params sppctl_dt_params[] = {
	{ "sunplus,input-invert-enable", PIN_CONFIG_INPUT_INVERT, 1 },
	{ "sunplus,output-invert-enable", PIN_CONFIG_OUTPUT_INVERT, 1 },
	{ "sunplus,input-invert-disable", PIN_CONFIG_INPUT_INVERT, 0 },
	{ "sunplus,output-invert-disable", PIN_CONFIG_OUTPUT_INVERT, 0 },
	{ "sunplus,slew-rate-control-disable", PIN_CONFIG_SLEW_RATE_CTRL, 0 },
	{ "sunplus,slew-rate-control-enable", PIN_CONFIG_SLEW_RATE_CTRL, 1 },
	{ "sunplus,bias-strong-pull-up", PIN_CONFIG_BIAS_STRONG_PULL_UP, 1 },

};

#ifdef CONFIG_DEBUG_FS
static const struct pin_config_item sppctl_conf_items[] = {
	PCONFDUMP(PIN_CONFIG_INPUT_INVERT, "input invert enabled", NULL, false),
	PCONFDUMP(PIN_CONFIG_OUTPUT_INVERT, "output invert enabled", NULL,
		  false),
	PCONFDUMP(PIN_CONFIG_SLEW_RATE_CTRL, "slew rate control enabled", NULL,
		  false),
	PCONFDUMP(PIN_CONFIG_BIAS_STRONG_PULL_UP, "bias strong pull up", NULL,
		  false),
};
#endif

#endif

const char *sppctl_get_function_name_by_selector(unsigned int selector)
{
	return list_funcs[selector].name;
}

struct func_t *sppctl_get_function_by_selector(unsigned int selector)
{
	return &list_funcs[selector];
}

int sppctl_get_function_count(void)
{
	return list_func_nums;
}

struct grp2fp_map_t *sppctl_get_group_by_name(const char *name)
{
	int i;

	for (i = 0; i < unique_groups_nums; i++) {
		if (!strcmp(unq_grps[i], name))
			return &g2fp_maps[i];
	}

	return NULL;
}

struct grp2fp_map_t *sppctl_get_group_by_selector(unsigned int group_selector,
						  unsigned int func_selector)
{
	struct func_t *func;
	int i, j;

	if (group_selector > GPIS_list_size - 1) {
		return &g2fp_maps[group_selector];
	} else if (func_selector == 0) { /* function:GPIO */
		return &g2fp_maps[group_selector];
	}

	/* group:GPIO0 ~GPIO105 */
	func = sppctl_get_function_by_selector(func_selector);

	for (i = 0; i < func->gnum; i++) {
		for (j = 0; j < func->grps[i].pnum; j++) {
			if (group_selector == func->grps[i].pins[j])
				return sppctl_get_group_by_name(
					func->grps[i].name);
		}
	}

	return NULL;
}

int sppctl_pin_function_association_query(unsigned int pin_selector,
					  unsigned int func_selector)
{
	int i, j;
	struct func_t *func = sppctl_get_function_by_selector(func_selector);

	for (i = 0; i < func->gnum; i++) {
		for (j = 0; j < func->grps[i].pnum; j++) {
			if (pin_selector == func->grps[i].pins[j])
				return 0;
		}
	}

	return -EEXIST;
}

int sppctl_group_function_association_query(unsigned int group_sel,
					    unsigned int func_sel)
{
	struct grp2fp_map_t *group_map =
		sppctl_get_group_by_selector(group_sel, func_sel);

	if (group_map->f_idx == func_sel)
		return 0;

	return -EEXIST;
}

static int sppctl_pinconf_get(struct pinctrl_dev *pctldev,
			      unsigned int pin_selector, unsigned long *config)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	unsigned int param;
	unsigned int arg = 0;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;
	param = pinconf_to_config_param(*config);

	KDBG(pctldev->dev, "%s(%d)\n", __func__, pin_selector);
	switch (param) {
	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
		if (!sppctl_gpio_is_open_drain_mode(chip, pin_selector))
			return -EINVAL;
		break;

	case PIN_CONFIG_OUTPUT:
		if (!sppctl_gpio_first_get(chip, pin_selector))
			return -EINVAL;
		if (!sppctl_gpio_master_get(chip, pin_selector))
			return -EINVAL;
		if (sppctl_gpio_get_direction(chip, pin_selector) != 0)
			return -EINVAL;
		arg = sppctl_gpio_get_value(chip, pin_selector);
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		break;

	default:
		//KINF(pctldev->dev, "%s(%d) skipping:x%X\n", __func__, pin_selector, param);
		return -ENOTSUPP;
	}
	*config = pinconf_to_config_packed(param, arg);

	return 0;
}

static int sppctl_pinconf_set(struct pinctrl_dev *pctldev,
			      unsigned int pin_selector, unsigned long *config,
			      unsigned int num_configs)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	u8 param;
	u32 arg;
	int i;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;

	KDBG(pctldev->dev, "%s(%d,%ld,%d)\n", __func__, pin_selector, *config,
	     num_configs);

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(config[i]);
		arg = pinconf_to_config_argument(config[i]);

		KDBG(pctldev->dev, "(%s:%d)GPIO[%d], arg:0x%x,param:0x%x)\n",
		     __func__, __LINE__, pin_selector, arg, param);
		switch (param) {
		case PIN_CONFIG_OUTPUT_ENABLE:
			KDBG(pctldev->dev, "GPIO[%d]:output %s\n", pin_selector,
			     arg == 1 ? "enable" : "disable");
			sppctl_gpio_output_enable_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_OUTPUT:
			KDBG(pctldev->dev, "GPIO[%d]:output %s\n", pin_selector,
			     arg == 1 ? "high" : "low");
			sppctl_gpio_direction_output(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_OUTPUT_INVERT:
			KDBG(pctldev->dev, "GPIO[%d]:%s output\n", pin_selector,
			     arg == 1 ? "invert" : "normalize");
			sppctl_gpio_output_invert_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_DRIVE_OPEN_DRAIN:
			KDBG(pctldev->dev, "GPIO[%d]:open drain\n",
			     pin_selector);
			sppctl_gpio_open_drain_mode_set(chip, pin_selector, 1);
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			KDBG(pctldev->dev, "GPIO[%d]:drive strength %dmA\n",
			     pin_selector, arg);
			sppctl_gpio_drive_strength_set(chip, pin_selector,
						       arg * 1000);
			break;
		case PIN_CONFIG_DRIVE_STRENGTH_UA:
			KDBG(pctldev->dev, "GPIO[%d]:drive strength %duA\n",
			     pin_selector, arg);
			sppctl_gpio_drive_strength_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_INPUT_ENABLE:
			KDBG(pctldev->dev, "GPIO[%d]:input %s\n", pin_selector,
			     arg == 0 ? "disable" : "enable");
			sppctl_gpio_input_enable_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_INPUT_INVERT:
			KDBG(pctldev->dev, "GPIO[%d]:%s input\n", pin_selector,
			     arg == 1 ? "invert" : "normalize");
			sppctl_gpio_input_invert_set(chip, pin_selector, arg);
			break;
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			KDBG(pctldev->dev, "GPIO[%d]:%s schmitt trigger\n",
			     pin_selector, arg == 0 ? "disable" : "enable");
			sppctl_gpio_schmitt_trigger_set(chip, pin_selector,
							arg);
			break;
		case PIN_CONFIG_SLEW_RATE_CTRL:
			KDBG(pctldev->dev, "GPIO[%d]:%s slew rate control\n",
			     pin_selector, arg == 0 ? "disable" : "enable");
			sppctl_gpio_slew_rate_control_set(chip, pin_selector,
							  arg);
			break;
		case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
			KDBG(pctldev->dev, "GPIO[%d]:high-Z\n", pin_selector);
			sppctl_gpio_high_impedance(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			KDBG(pctldev->dev, "GPIO[%d]:pull up\n", pin_selector);
			sppctl_gpio_pull_up(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_STRONG_PULL_UP:
			KDBG(pctldev->dev, "GPIO[%d]:strong pull up\n",
			     pin_selector);
			sppctl_gpio_strong_pull_up(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			KDBG(pctldev->dev, "GPIO[%d]:pull down\n",
			     pin_selector);
			sppctl_gpio_pull_down(chip, pin_selector);
			break;
		case PIN_CONFIG_BIAS_DISABLE:
			KDBG(pctldev->dev, "GPIO[%d]:bias disable\n",
			     pin_selector);
			sppctl_gpio_bias_disable(chip, pin_selector);
			break;
		default:
			KERR(pctldev->dev,
			     "GPIO[%d]:param:0x%x is not supported\n",
			     pin_selector, param);
			break;
		}
		// FIXME: add pullup/pulldown, irq enable/disable
	}

	return 0;
}

static int sppctl_pinconf_group_get(struct pinctrl_dev *pctldev,
				    unsigned int group_selector,
				    unsigned long *config)
{
	KINF(pctldev->dev, "%s(%d)\n", __func__, group_selector);

	return 0;
}

static int sppctl_pinconf_group_set(struct pinctrl_dev *pctldev,
				    unsigned int group_selector,
				    unsigned long *config,
				    unsigned int num_configs)
{
	unsigned int pin;
	int i;
	int j;
	int k;

	for (i = 0; i < list_func_nums; i++) {
		struct func_t func = list_funcs[i];

		for (j = 0; j < func.gnum; j++) {
			struct sppctlgrp_t group = func.grps[j];

			if (strcmp(group.name, unq_grps[group_selector]) == 0) {
				for (k = 0; k < group.pnum; k++) {
					pin = group.pins[k];

					KINF(pctldev->dev,
					     "(%s:%d)grp%d.name[%s]pin[%d]\n",
					     __func__, __LINE__, group_selector,
					     group.name, pin);
					sppctl_pinconf_set(pctldev, pin, config,
							   num_configs);
				}
			}
		}
	}

	return 0;
}

#ifdef CONFIG_DEBUG_FS
static void sppctl_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				    struct seq_file *seq, unsigned int offset)
{
	// KINF(pctldev->dev, "%s(%d)\n", __func__, offset);
	seq_printf(seq, " %s", dev_name(pctldev->dev));
}

static void sppctl_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
					  struct seq_file *seq,
					  unsigned int group_selector)
{
	// group: freescale/pinctrl-imx.c, 448
	// KINF(pctldev->dev, "%s(%d)\n", __func__, group_selector);
}

static void sppctl_pinconf_config_dbg_show(struct pinctrl_dev *pctldev,
					   struct seq_file *seq,
					   unsigned long config)
{
	// KINF(pctldev->dev, "%s(%ld)\n", __func__, config);
}
#else
#define sppctl_pinconf_dbg_show NULL
#define sppctl_pinconf_group_dbg_show NULL
#define sppctl_pinconf_config_dbg_show NULL
#endif

static int sppctl_pinmux_request(struct pinctrl_dev *pctldev,
				 unsigned int selector)
{
	//KDBG(pctldev->dev, "%s(%d)\n", __func__, selector);
	return 0;
}

static int sppctl_pinmux_free(struct pinctrl_dev *pctldev,
			      unsigned int selector)
{
	//KDBG(pctldev->dev, "%s(%d)\n", __func__, selector);
	return 0;
}

static int sppctl_pinmux_get_functions_count(struct pinctrl_dev *pctldev)
{
	return list_func_nums;
}

static const char *sppctl_pinmux_get_function_name(struct pinctrl_dev *pctldev,
						   unsigned int selector)
{
	return list_funcs[selector].name;
}

int sppctl_pinmux_get_function_groups(struct pinctrl_dev *pctldev,
				      unsigned int selector,
				      const char *const **groups,
				      unsigned int *num_groups)
{
#ifndef DISABLE_CONFLICT_CODE_WITH_GENERIC_USAGE
	struct func_t *func = &list_funcs[selector];

	*num_groups = 0;
	switch (func->freg) {
	case F_OFF_I:
	case F_OFF_0: // gen GPIO/IOP: all groups = all pins
		*num_groups = GPIS_list_size;
		*groups = sppctlgpio_list_s;
		break;

	case F_OFF_M: // pin-mux
		*num_groups = pinmux_list_size;
		*groups = sppctlpmux_list_s;
		break;

	case F_OFF_G: // pin-group
		if (!func->grps)
			break;
		*num_groups = f->gnum;
		*groups = (const char *const *)func->grps_sa;
		break;

	default:
		KERR(pctldev->dev, "%s(fid:%d) unknown fOFF %d\n", __func__,
		     selector, func->freg);
		break;
	}
#else
	*num_groups = unique_groups_nums;
	*groups = unq_grps;
#endif
	KDBG(pctldev->dev, "%s(fid:%d) %d\n", __func__, selector, *num_groups);
	return 0;
}

#ifndef DISABLE_CONFLICT_CODE_WITH_GENERIC_USAGE
static int sppctl_pinmux_set_mux(struct pinctrl_dev *pctldev,
				 unsigned int func_selector,
				 unsigned int group_selector)
{
	struct sppctl_pdata_t *pctrl;
	struct grp2fp_map_t g2fpm;
	unsigned int pin_selector;
	struct gpio_chip *chip;
	struct func_t *func;
	u8 reg_value;
	int i = -1;
	int j = -1;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;
	func = &list_funcs[func_selector];
	g2fpm = g2fp_maps[group_selector];

	KDBG(pctldev->dev, "%s(fun:%d,grp:%d)\n", __func__, func_selector,
	     group_selector);
	switch (func->freg) {
	case F_OFF_0: // GPIO. detouch from all funcs - ?
		sppctl_gpio_first_master_set(chip, group_selector, MUX_FIRST_G,
					     MUX_MASTER_G);
		for (i = 0; i < list_func_nums; i++) {
			if (list_funcs[i].freg != F_OFF_M)
				continue;
			j++;
			if (sppctl_fun_get(pctrl, j) != group_selector)
				continue;
			sppctl_pin_set(pctrl, 0, j);
		}
		break;

	case F_OFF_M: // MUX :
		sppctl_gpio_first_master_set(chip, group_selector, MUX_FIRST_M,
					     MUX_MASTER_KEEP);
		pin_selector = (group_selector == 0 ? group_selector :
							    group_selector - 7);
		sppctl_pin_set(pctrl, pin_selector,
			       func_selector - 2); // pin, fun FIXME
		break;

	case F_OFF_G: // GROUP
		for (i = 0; i < func->grps[g2fpm.g_idx].pnum; i++)
			sppctl_gpio_first_master_set(
				chip, func->grps[g2fpm.g_idx].pins[i],
				MUX_FIRST_M, MUX_MASTER_KEEP);
		reg_value = func->grps[g2fpm.g_idx].gval;
		sppctl_gmx_set(pctrl, func->roff, func->boff, func->blen,
			       reg_value);
		break;

	case F_OFF_I: // IOP
		sppctl_gpio_first_master_set(chip, group_selector, MUX_FIRST_G,
					     MUX_MASTER_I);
		break;

	default:
		KERR(pctldev->dev, "%s(func_selector:%d) unknown fOFF %d\n",
		     __func__, func_selector, func->freg);
		break;
	}

	return 0;
}
#else
static int sppctl_pinmux_set_mux(struct pinctrl_dev *pctldev,
				 unsigned int func_selector,
				 unsigned int group_selector)
{
	struct sppctl_pdata_t *pctrl;
	struct grp2fp_map_t *g2fpm;
	struct func_t *owner_func;
	struct gpio_chip *chip;
	struct func_t *func;
	unsigned int pin;
	u8 reg_value;
	int i = -1;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	func = sppctl_get_function_by_selector(func_selector);
	g2fpm = sppctl_get_group_by_selector(group_selector, func_selector);
	chip = &pctrl->gpiod->chip;

	//KDBG(pctldev->dev, "[%s:%d]function:%d, group:%d\n",
	//	__func__, __LINE__, func_selector, group_selector);

	if (!strcmp(func->name, "GPIO")) {
		if (group_selector < GPIS_list_size) {
			pin = group_selector;

			KDBG(pctldev->dev,
			     "[%s:%d]set function[%s] on pin[%d]\n", __func__,
			     __LINE__, func->name, pin);
			sppctl_gpio_first_master_set(chip, pin, MUX_FIRST_G,
						     MUX_MASTER_G);
		} else {
			owner_func =
				sppctl_get_function_by_selector(g2fpm->f_idx);

			for (i = 0; i < owner_func->grps[g2fpm->g_idx].pnum;
			     i++) {
				pin = owner_func->grps[g2fpm->g_idx].pins[i];

				KDBG(pctldev->dev,
				     "[%s:%d]set function[%s] on pin[%d]\n",
				     __func__, __LINE__, func->name, pin);
				sppctl_gpio_first_master_set(
					chip, pin, MUX_FIRST_G, MUX_MASTER_G);
			}
		}
	} else {
		if (group_selector < GPIS_list_size) {
			if (!sppctl_pin_function_association_query(
				    group_selector, func_selector)) {
				pin = group_selector;

				KDBG(pctldev->dev,
				     "[%s:%d]set function[%s] on pin[%d]\n",
				     __func__, __LINE__, func->name, pin);
				sppctl_gpio_first_master_set(chip, pin,
							     MUX_FIRST_M,
							     MUX_MASTER_KEEP);
				reg_value = func->grps[g2fpm->g_idx].gval;
				sppctl_gmx_set(pctrl, func->roff, func->boff,
					       func->blen, reg_value);
			} else {
				KERR(pctldev->dev,
				     "invalid pin[%d] for function \"%s\"\n",
				     group_selector, func->name);
			}
		} else if (!sppctl_group_function_association_query(
				   group_selector, func_selector)) {
			for (i = 0; i < func->grps[g2fpm->g_idx].pnum; i++) {
				pin = func->grps[g2fpm->g_idx].pins[i];

				KDBG(pctldev->dev,
				     "[%s:%d]set function[%s] on pin[%d]\n",
				     __func__, __LINE__, func->name, pin);
				sppctl_gpio_first_master_set(chip, pin,
							     MUX_FIRST_M,
							     MUX_MASTER_KEEP);
			}
			reg_value = func->grps[g2fpm->g_idx].gval;
			sppctl_gmx_set(pctrl, func->roff, func->boff,
				       func->blen, reg_value);
		} else {
			KERR(pctldev->dev,
			     "invalid group \"%s\" for function \"%s\"\n",
			     unq_grps[group_selector], func->name);
		}
	}

	return 0;
}
#endif

static int sppctl_pinmux_gpio_request_enable(struct pinctrl_dev *pctldev,
					     struct pinctrl_gpio_range *range,
					     unsigned int pin_selector)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	struct pin_desc *pdesc;
	int g_f;
	int g_m;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;

	KDBG(pctldev->dev, "%s(%d)\n", __func__, pin_selector);
	g_f = sppctl_gpio_first_get(chip, pin_selector);
	g_m = sppctl_gpio_master_get(chip, pin_selector);
	if (g_f == MUX_FIRST_G && g_m == MUX_MASTER_G)
		return 0;

	pdesc = pin_desc_get(pctldev, pin_selector);
	// in non-gpio state: is it claimed already?
	if (pdesc->mux_owner)
		return -EACCES;

	sppctl_gpio_first_master_set(chip, pin_selector, MUX_FIRST_G,
				     MUX_MASTER_G);
	return 0;
}

static void sppctl_pinmux_gpio_disable_free(struct pinctrl_dev *pctldev,
					    struct pinctrl_gpio_range *range,
					    unsigned int pin_selector)
{
	sppctl_gpio_unmux_irq(range->gc, pin_selector);
}

int sppctl_pinmux_gpio_set_direction(struct pinctrl_dev *pctldev,
				     struct pinctrl_gpio_range *range,
				     unsigned int pin_selector,
				     bool direction_input)
{
	KDBG(pctldev->dev, "%s(%d,%d)\n", __func__, pin_selector,
	     direction_input);
	return 0;
}

// all groups
static int sppctl_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	return unique_groups_nums;
}

static const char *sppctl_pinctrl_get_group_name(struct pinctrl_dev *pctldev,
						 unsigned int selector)
{
	return unq_grps[selector];
}

static int sppctl_pinctrl_get_group_pins(struct pinctrl_dev *pctldev,
					 unsigned int selector,
					 const unsigned int **pins,
					 unsigned int *num_pins)
{
#if defined(SUPPORT_GPIO_AO_INT)
	struct sppctl_pdata_t *pctrl = pinctrl_dev_get_drvdata(pctldev);
	struct sppctlgpio_chip_t *pc = pctrl->gpiod;
	int i;
#endif
	struct grp2fp_map_t g2fpm = g2fp_maps[selector];
	struct func_t *func = &list_funcs[g2fpm.f_idx];

	KDBG(pctldev->dev, "grp-pins g:%d f_idx:%d,g_idx:%d freg:%d...\n",
	     selector, g2fpm.f_idx, g2fpm.g_idx, func->freg);
	*num_pins = 0;

	// MUX | GPIO | IOP: 1 pin -> 1 group
	if (func->freg != F_OFF_G) {
		*num_pins = 1;
		*pins = &sppctlpins_group[selector];
		return 0;
	}

	// IOP (several pins at once in a group)
	if (!func->grps)
		return 0;
	if (func->gnum < 1)
		return 0;
	*num_pins = func->grps[g2fpm.g_idx].pnum;
	*pins = func->grps[g2fpm.g_idx].pins;

#if defined(SUPPORT_GPIO_AO_INT)
	if (selector == 265 || selector == 266) { // GPIO_AO_INT0
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i] = (*pins)[i];
	}
	if (selector == 267 || selector == 268) { // GPIO_AO_INT1
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i + 8] = (*pins)[i];
	}
	if (selector == 269 || selector == 270) { // GPIO_AO_INT2
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i + 16] = (*pins)[i];
	}
	if (selector == 271 || selector == 272) { // GPIO_AO_INT3
		for (i = 0; i < *num_pins; i++)
			pc->gpio_ao_int_pins[i + 24] = (*pins)[i];
	}
#endif

	return 0;
}

// /sys/kernel/debug/pinctrl/sppctl/pins add: gpio_first and ctrl_sel
#ifdef CONFIG_DEBUG_FS
static void sppctl_pinctrl_pin_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *seq,
					unsigned int selector)
{
	struct sppctl_pdata_t *pctrl;
	struct gpio_chip *chip;
	const char *tmpp;
	u8 g_f, g_m;

	pctrl = pinctrl_dev_get_drvdata(pctldev);
	chip = &pctrl->gpiod->chip;

	seq_printf(seq, "%s", dev_name(pctldev->dev));
	g_f = sppctl_gpio_first_get(chip, selector);
	g_m = sppctl_gpio_master_get(chip, selector);

	tmpp = "?";
	if (g_f && g_m)
		tmpp = "GPIO";
	if (g_f && !g_m)
		tmpp = " IOP";
	if (!g_f)
		tmpp = " MUX";
	seq_printf(seq, " %s", tmpp);
}
#else
#define sppctl_ops_show NULL
#endif

static unsigned long
sppctl_pinconf_param_2_generic_pinconf_param(unsigned char param)
{
	unsigned long config = PIN_CONFIG_MAX;

	switch (param) {
	case SPPCTL_PCTL_L_OUT:
		config = PIN_CONF_PACKED(PIN_CONFIG_OUTPUT, 0);
		break;
	case SPPCTL_PCTL_L_OU1:
		config = PIN_CONF_PACKED(PIN_CONFIG_OUTPUT, 1);
		break;
	case SPPCTL_PCTL_L_INV:
		config = PIN_CONF_PACKED(PIN_CONFIG_INPUT_INVERT, 0);
		break;
	case SPPCTL_PCTL_L_ONV:
		config = PIN_CONF_PACKED(PIN_CONFIG_OUTPUT_INVERT, 0);
		break;
	case SPPCTL_PCTL_L_ODR:
		config = PIN_CONF_PACKED(PIN_CONFIG_DRIVE_OPEN_DRAIN, 0);
		break;
	default:
		break;
	}

	return config;
}

static int sppctl_pinctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
					 struct device_node *np_config,
					 struct pinctrl_map **map,
					 unsigned int *num_maps)
{
	struct pinctrl_map *generic_map = NULL;
	unsigned int num_generic_maps = 0;
#if defined(SUPPORT_GPIO_AO_INT)
	struct sppctlgpio_chip_t *pc;
	int mask, reg1, reg2;
	int ao_size = 0;
	const __be32 *ao_list;
	int ao_nm;
#endif
	struct sppctl_pdata_t *pctrl;
	struct device_node *parent;
	struct func_t *func = NULL;
	unsigned long *configs;
	const __be32 *list;
#ifndef DISABLE_CONFLICT_CODE_WITH_GENERIC_USAGE
	struct property *prop;
	const char *s_f;
	const char *s_g;
#endif
	int size = 0;
	int ret = 0;
	int nmG = 0;
	int i = 0;
	u32 dt_pin;
	u32 dt_fun;
	u8 p_p;
	u8 p_g;
	u8 p_f;
	u8 p_l;

	list = of_get_property(np_config, "sunplus,pins", &size);

	pctrl = pinctrl_dev_get_drvdata(pctldev);

#if defined(SUPPORT_GPIO_AO_INT)
	pc = pctrl->gpiod;
	ao_list = of_get_property(np_config, "sunplus,ao-pins", &ao_size);
	ao_nm = ao_size / sizeof(*ao_list);
#endif

#ifndef DISABLE_CONFLICT_CODE_WITH_GENERIC_USAGE
	nmG = of_property_count_strings(np_config, "groups");

	//print_device_tree_node(np_config, 0);
	if (nmG <= 0)
		nmG = 0;
#endif
	*num_maps = size / sizeof(*list);

	// Check if out of range or invalid?
	for (i = 0; i < (*num_maps); i++) {
		dt_pin = be32_to_cpu(list[i]);
		p_p = SPPCTL_PCTLD_P(dt_pin);
		p_g = SPPCTL_PCTLD_G(dt_pin);

		if (p_p >= sppctlpins_all_nums || p_g == SPPCTL_PCTL_G_PMUX) {
			KDBG(pctldev->dev,
			     "Invalid \'sunplus,pins\' property at index %d (0x%08x)\n",
			     i, dt_pin);
			return -EINVAL;
		}
	}

#if defined(SUPPORT_GPIO_AO_INT)
	// Check if out of range?
	for (i = 0; i < ao_nm; i++) {
		dt_pin = be32_to_cpu(ao_list[i]);
		if (SPPCTL_AOPIN_PIN(dt_pin) >= 32) {
			KDBG(pctldev->dev,
			     "Invalid \'sunplus,ao_pins\' property at index %d (0x%08x)\n",
			     i, dt_pin);
			return -EINVAL;
		}
	}
#endif

	ret = pinconf_generic_dt_node_to_map_all(
		pctldev, np_config, &generic_map, &num_generic_maps);
	if (ret < 0) {
		KERR(pctldev->dev, "L:%d;Parse generic pinconfig error on %d\n",
		     __LINE__, ret);
		return ret;
	}

	*map = kcalloc(*num_maps + nmG + num_generic_maps, sizeof(**map),
		       GFP_KERNEL);
	if (!(*map))
		return -ENOMEM;

	parent = of_get_parent(np_config);
	for (i = 0; i < (*num_maps); i++) {
		dt_pin = be32_to_cpu(list[i]);
		p_p = SPPCTL_PCTLD_P(dt_pin);
		p_g = SPPCTL_PCTLD_G(dt_pin);
		p_f = SPPCTL_PCTLD_F(dt_pin);
		p_l = SPPCTL_PCTLD_L(dt_pin);
		(*map)[i].name = parent->name;
		KDBG(pctldev->dev, "map [%d]=%08x p=%d g=%d f=%d l=%d\n", i,
		     dt_pin, p_p, p_g, p_f, p_l);

		if (p_g == SPPCTL_PCTL_G_GPIO) {
			// look into parse_dt_cfg(),
			(*map)[i].type = PIN_MAP_TYPE_CONFIGS_PIN;
			(*map)[i].data.configs.num_configs = 1;
			(*map)[i].data.configs.group_or_pin =
				pin_get_name(pctldev, p_p);
			configs = kcalloc(1, sizeof(*configs), GFP_KERNEL);
			if (!configs)
				goto sppctl_n2map_err;
			*configs = sppctl_pinconf_param_2_generic_pinconf_param(
				p_l);
			(*map)[i].data.configs.configs = configs;

			KDBG(pctldev->dev, "%s(%d) = x%X\n",
			     (*map)[i].data.configs.group_or_pin, p_p, p_l);
		} else if (p_g == SPPCTL_PCTL_G_IOPP) {
			(*map)[i].type = PIN_MAP_TYPE_CONFIGS_PIN;
			(*map)[i].data.configs.num_configs = 1;
			(*map)[i].data.configs.group_or_pin =
				pin_get_name(pctldev, p_p);
			configs = kcalloc(1, sizeof(*configs), GFP_KERNEL);
			if (!configs)
				goto sppctl_n2map_err;
			*configs = sppctl_pinconf_param_2_generic_pinconf_param(
				PIN_CONFIG_MAX);
			(*map)[i].data.configs.configs = configs;

			KDBG(pctldev->dev, "%s(%d) = x%X\n",
			     (*map)[i].data.configs.group_or_pin, p_p, p_l);
		} else {
			(*map)[i].type = PIN_MAP_TYPE_MUX_GROUP;
			(*map)[i].data.mux.function = list_funcs[p_f].name;
			(*map)[i].data.mux.group = pin_get_name(pctldev, p_p);

			KDBG(pctldev->dev, "f->p: %s(%d)->%s(%d)\n",
			     (*map)[i].data.mux.function, p_f,
			     (*map)[i].data.mux.group, p_p);
		}
	}

	for (i = 0; i < num_generic_maps; i++, (*num_maps)++) {
		struct pinctrl_map pmap = generic_map[i];

		if (pmap.type == PIN_MAP_TYPE_MUX_GROUP) {
			(*map)[*num_maps].type = pmap.type;
			(*map)[*num_maps].data.mux.group = pmap.data.mux.group;
			(*map)[*num_maps].data.mux.function =
				pmap.data.mux.function;
		} else {
			(*map)[*num_maps].type = pmap.type;
			(*map)[*num_maps].data.configs.group_or_pin =
				pmap.data.configs.group_or_pin;
			(*map)[*num_maps].data.configs.configs =
				pmap.data.configs.configs;
			(*map)[*num_maps].data.configs.num_configs =
				pmap.data.configs.num_configs;
		}
	}

#if defined(SUPPORT_GPIO_AO_INT)
	for (i = 0; i < ao_nm; i++) {
		dt_pin = be32_to_cpu(ao_list[i]);
		p_p = SPPCTL_AOPIN_PIN(dt_pin);
		p_l = SPPCTL_AOPIN_FLG(dt_pin);

		mask = 1 << p_p;
		reg1 = readl(pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE
		if (p_l & (SPPCTL_AOPIN_OUT0 | SPPCTL_AOPIN_OUT1)) {
			reg2 = readl(pc->gpio_ao_int_regs_base +
				     0x14); // GPIO_O
			if (p_l & SPPCTL_AOPIN_OUT1)
				reg2 |= mask;
			else
				reg2 &= ~mask;
			writel(reg2,
			       pc->gpio_ao_int_regs_base + 0x14); // GPIO_O

			reg1 |= mask;
		} else {
			reg1 &= ~mask;
		}
		writel(reg1, pc->gpio_ao_int_regs_base + 0x18); // GPIO_OE

		reg1 = readl(pc->gpio_ao_int_regs_base + 0x08); // DEB_EN
		if (p_l & SPPCTL_AOPIN_DEB)
			reg1 |= mask;
		else
			reg1 &= ~mask;
		writel(reg1, pc->gpio_ao_int_regs_base + 0x08); // DEB_EN
	}
#endif

#ifndef DISABLE_CONFLICT_CODE_WITH_GENERIC_USAGE
	// handle pin-group function
	if (nmG > 0 &&
	    of_property_read_string(np_config, "function", &s_f) == 0) {
		KDBG(pctldev->dev, "found func: %s\n", s_f);
		of_property_for_each_string(np_config, "groups", prop, s_g) {
			KDBG(pctldev->dev, " %s: %s\n", s_f, s_g);
			(*map)[*num_maps].type = PIN_MAP_TYPE_MUX_GROUP;
			(*map)[*num_maps].data.mux.function = s_f;
			(*map)[*num_maps].data.mux.group = s_g;
			KDBG(pctldev->dev, "f->g: %s->%s\n",
			     (*map)[*num_maps].data.mux.function,
			     (*map)[*num_maps].data.mux.group);
			(*num_maps)++;
		}
	}
#endif
	// handle zero function
	list = of_get_property(np_config, "sunplus,zerofunc", &size);
	if (list) {
		for (i = 0; i < size / sizeof(*list); i++) {
			dt_fun = be32_to_cpu(list[i]);
			if (dt_fun >= list_func_nums) {
				KERR(pctldev->dev,
				     "zero func %d out of range\n", dt_fun);
				continue;
			}

			func = &list_funcs[dt_fun];
			switch (func->freg) {
			case F_OFF_M:
				KDBG(pctldev->dev, "zero func: %d (%s)\n",
				     dt_fun, func->name);
				sppctl_pin_set(pctrl, 0, dt_fun - 2);
				break;

			case F_OFF_G:
				KDBG(pctldev->dev, "zero group: %d (%s)\n",
				     dt_fun, func->name);
				sppctl_gmx_set(pctrl, func->roff, func->boff,
					       func->blen, 0);
				break;

			default:
				KERR(pctldev->dev,
				     "wrong zero group: %d (%s)\n", dt_fun,
				     func->name);
				break;
			}
		}
	}

	of_node_put(parent);
	KDBG(pctldev->dev, "%d pins or functions are mapped!\n", *num_maps);
	return 0;

sppctl_n2map_err:
	for (i = 0; i < (*num_maps); i++)
		if (((*map)[i].type == PIN_MAP_TYPE_CONFIGS_PIN) &&
		    (*map)[i].data.configs.configs)
			kfree((*map)[i].data.configs.configs);
	kfree(*map);
	of_node_put(parent);
	return -ENOMEM;
}

static void sppctl_pinctrl_dt_free_map(struct pinctrl_dev *pctldev,
				       struct pinctrl_map *map,
				       unsigned int num_maps)
{
	//KINF(pctldev->dev, "%s(%d)\n", __func__, num_maps);
	// FIXME: test
	pinctrl_utils_free_map(pctldev, map, num_maps);
}

// creates unq_grps[] uniq group names array char *
// sets unique_groups_nums
// creates XXX[group_idx]{func_idx, pins_idx}
void group_groups(struct platform_device *pdev)
{
	int i;
	int k;
	int j;

	// fill array of all groups
	unq_grps = NULL;
	unique_groups_nums = GPIS_list_size;

	// calc unique group names array size
	for (i = 0; i < list_func_nums; i++) {
		if (list_funcs[i].freg != F_OFF_G)
			continue;
		unique_groups_nums += list_funcs[i].gnum;
	}

	// fill up unique group names array
	unq_grps = devm_kzalloc(&pdev->dev,
				(unique_groups_nums + 1) * sizeof(char *),
				GFP_KERNEL);
	g2fp_maps = devm_kzalloc(&pdev->dev,
				 (unique_groups_nums + 1) *
					 sizeof(struct grp2fp_map_t),
				 GFP_KERNEL);

	// groups == pins
	j = 0;
	for (i = 0; i < GPIS_list_size; i++) {
		unq_grps[i] = sppctlgpio_list_s[i];
		g2fp_maps[i].f_idx = 0;
		g2fp_maps[i].g_idx = i;
	}
	j = GPIS_list_size;

	// +IOP groups
	for (i = 0; i < list_func_nums; i++) {
		if (list_funcs[i].freg != F_OFF_G)
			continue;

		for (k = 0; k < list_funcs[i].gnum; k++) {
			list_funcs[i].grps_sa[k] =
				(char *)list_funcs[i].grps[k].name;
			unq_grps[j] = list_funcs[i].grps[k].name;
			g2fp_maps[j].f_idx = i;
			g2fp_maps[j].g_idx = k;
			j++;
		}
	}
	KINF(&pdev->dev, "funcs: %zd unq_grps: %zd\n", list_func_nums,
	     unique_groups_nums);
}

static struct pinconf_ops sppctl_pconf_ops = {
	.is_generic = true,
	.pin_config_get = sppctl_pinconf_get,
	.pin_config_set = sppctl_pinconf_set,
	.pin_config_group_get = sppctl_pinconf_group_get,
	.pin_config_group_set = sppctl_pinconf_group_set,
	.pin_config_dbg_show = sppctl_pinconf_dbg_show,
	.pin_config_group_dbg_show = sppctl_pinconf_group_dbg_show,
	.pin_config_config_dbg_show = sppctl_pinconf_config_dbg_show,
};

static const struct pinmux_ops sppctl_pinmux_ops = {
	.request = sppctl_pinmux_request,
	.free = sppctl_pinmux_free,
	.get_functions_count = sppctl_pinmux_get_functions_count,
	.get_function_name = sppctl_pinmux_get_function_name,
	.get_function_groups = sppctl_pinmux_get_function_groups,
	.set_mux = sppctl_pinmux_set_mux,
	.gpio_request_enable = sppctl_pinmux_gpio_request_enable,
	.gpio_disable_free = sppctl_pinmux_gpio_disable_free,
	.gpio_set_direction = sppctl_pinmux_gpio_set_direction,
	.strict = 1
};

static const struct pinctrl_ops sppctl_pctl_ops = {
	.get_groups_count = sppctl_pinctrl_get_groups_count,
	.get_group_name = sppctl_pinctrl_get_group_name,
	.get_group_pins = sppctl_pinctrl_get_group_pins,
#ifdef CONFIG_DEBUG_FS
	.pin_dbg_show = sppctl_pinctrl_pin_dbg_show,
#endif
	.dt_node_to_map = sppctl_pinctrl_dt_node_to_map,
	.dt_free_map = sppctl_pinctrl_dt_free_map,
};

// ---------- main (exported) functions
int sppctl_pinctrl_init(struct platform_device *pdev)
{
	struct device_node *np_config;
	struct sppctl_pdata_t *pdata;
	struct device *dev;
	int err;

	dev = &pdev->dev;
	pdata = (struct sppctl_pdata_t *)pdev->dev.platform_data;
	np_config = of_node_get(dev->of_node);

	// init pdesc
	pdata->pdesc.owner = THIS_MODULE;
	pdata->pdesc.name = dev_name(&pdev->dev);
	pdata->pdesc.pins = &sppctlpins_all[0];
	pdata->pdesc.npins = sppctlpins_all_nums;
	pdata->pdesc.pctlops = &sppctl_pctl_ops;
	pdata->pdesc.confops = &sppctl_pconf_ops;
	pdata->pdesc.pmxops = &sppctl_pinmux_ops;

#ifdef CONFIG_GENERIC_PINCONF
	pdata->pdesc.custom_params = sppctl_dt_params;
	pdata->pdesc.num_custom_params = ARRAY_SIZE(sppctl_dt_params);
#endif

#ifdef CONFIG_DEBUG_FS
	pdata->pdesc.custom_conf_items = sppctl_conf_items;
#endif

	group_groups(pdev);

	err = devm_pinctrl_register_and_init(&pdev->dev, &pdata->pdesc, pdata,
					     &pdata->pcdp);
	if (err) {
		KERR(&pdev->dev, "Failed to register\n");
		of_node_put(np_config);
		return err;
	}

	pinctrl_enable(pdata->pcdp);
	return 0;
}

void sppctl_pinctrl_clean(struct platform_device *pdev)
{
	struct sppctl_pdata_t *pdata;

	pdata = (struct sppctl_pdata_t *)pdev->dev.platform_data;

	devm_pinctrl_unregister(&pdev->dev, pdata->pcdp);
}
