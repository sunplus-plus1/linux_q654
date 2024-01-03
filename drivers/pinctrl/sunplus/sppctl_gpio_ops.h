/* SPDX-License-Identifier: GPL-2.0 */

#ifndef SPPCTL_GPIO_OPS_H
#define SPPCTL_GPIO_OPS_H

#include "sppctl_gpio.h"

/**
 * enum vol_ms_group - the groups of voltage mode select for DVIO.
 * @G_MX_MS_TOP_0 includes pins from G_MX21 to G_MX27.
 * @G_MX_MS_TOP_1 includes pins G_MX20, and those from G_MX28 to G_MX37.
 * @AO_MX_MS_TOP_0 includes pins from A0_MX0 to AO_MX9.
 * @AO_MX_MS_TOP_1 includes pins from A0_MX10 to AO_MX19.
 * @AO_MX_MS_TOP_2 includes pins from A0_MX20 to AO_MX29.
 */

enum vol_ms_group {
	G_MX_MS_TOP_0,
	G_MX_MS_TOP_1,
	AO_MX_MS_TOP_0,
	AO_MX_MS_TOP_1,
	AO_MX_MS_TOP_2,
};

// who is first: GPIO(1) | MUX(0)
int sppctl_gpio_first_get(struct gpio_chip *chip, unsigned int selector);

// who is master: GPIO(1) | IOP(0)
int sppctl_gpio_master_get(struct gpio_chip *chip, unsigned int selector);

// set MASTER and FIRST
void sppctl_gpio_first_master_set(struct gpio_chip *chip, unsigned int selector,
				  enum MUX_FIRST_MG_t first_sel,
				  enum MUX_MASTER_IG_t master_sel);

// is inv: INVERTED(1) | NORMAL(0)
int sppctl_gpio_is_inverted(struct gpio_chip *chip, unsigned int selector);
// set (I|O)inv
void sppctl_gpio_input_invert_set(struct gpio_chip *chip, unsigned int selector,
				  unsigned int value);
void sppctl_gpio_output_invert_set(struct gpio_chip *chip,
				   unsigned int selector, unsigned int value);

// set driving strength in uA
int sppctl_gpio_drive_strength_set(struct gpio_chip *chip,
				   unsigned int selector, int value);

//voltage mode select
int sppctl_gpio_voltage_mode_select_set(struct gpio_chip *chip,
					enum vol_ms_group ms_group,
					unsigned int value);

// enable/disable schmitt trigger 0:disable 1:enable
int sppctl_gpio_schmitt_trigger_set(struct gpio_chip *chip,
				    unsigned int selector, int value);

//enable/disable slew rate control 0:disable 1:enable
int sppctl_gpio_slew_rate_control_set(struct gpio_chip *chip,
				      unsigned int selector,
				      unsigned int value);

/* pull-up */
int sppctl_gpio_pull_up(struct gpio_chip *chip, unsigned int selector);

/* pull-down */
int sppctl_gpio_pull_down(struct gpio_chip *chip, unsigned int selector);

/* strongly pull-up; for GPIO only */
int sppctl_gpio_strong_pull_up(struct gpio_chip *chip, unsigned int selector);

/* high-Z; */
int sppctl_gpio_high_impedance(struct gpio_chip *chip, unsigned int selector);

/* bias disable */
int sppctl_gpio_bias_disable(struct gpio_chip *chip, unsigned int selector);

/* input enable or disable */
int sppctl_gpio_input_enable_set(struct gpio_chip *chip, unsigned int selector,
				 int value);

// enable/disable output: 0:disable, 1:enable
int sppctl_gpio_output_enable_set(struct gpio_chip *chip, unsigned int selector,
				  int value);

// is open-drain: YES(1) | NON(0)
int sppctl_gpio_is_open_drain_mode(struct gpio_chip *chip,
				   unsigned int selector);
void sppctl_gpio_open_drain_mode_set(struct gpio_chip *chip,
				     unsigned int selector, unsigned int value);

int sppctl_gpio_request(struct gpio_chip *chip, unsigned int selector);
void sppctl_gpio_free(struct gpio_chip *chip, unsigned int selector);

// get dir: 0=out, 1=in, -E =err (-EINVAL for ex): OE inverted on ret
int sppctl_gpio_get_direction(struct gpio_chip *chip, unsigned int selector);

// set to input: 0:ok: OE=0
int sppctl_gpio_direction_input(struct gpio_chip *chip, unsigned int selector);

// set to output: 0:ok: OE=1,O=value
int sppctl_gpio_direction_output(struct gpio_chip *chip, unsigned int selector,
				 int value);

// get value for signal: 0=low | 1=high | -err
int sppctl_gpio_get_value(struct gpio_chip *chip, unsigned int selector);

// OUT only: can't call set on IN pin: protected by gpio_chip layer
void sppctl_gpio_set_value(struct gpio_chip *chip, unsigned int selector,
			   int value);

// FIX: test in-depth
int sppctl_gpio_set_config(struct gpio_chip *chip, unsigned int selector,
			   unsigned long config);

#ifdef CONFIG_DEBUG_FS
void sppctl_gpio_dbg_show(struct seq_file *seq, struct gpio_chip *chip);
#else
#define sppctl_gpio_dbg_show NULL
#endif

#ifdef CONFIG_OF_GPIO
//int sppctlgpio_xlate(struct gpio_chip *chip, const struct of_phandle_args *arg,
// u32 *num_args);
#endif

int sppctl_gpio_to_irq(struct gpio_chip *chip, unsigned int offset);

void sppctl_gpio_unmux_irq(struct gpio_chip *chip, unsigned int selector);

#endif // SPPCTL_GPIO_OPS_H
