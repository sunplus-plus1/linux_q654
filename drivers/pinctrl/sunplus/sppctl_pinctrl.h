/* SPDX-License-Identifier: GPL-2.0 */

#ifndef SPPCTL_PINCTRL_H
#define SPPCTL_PINCTRL_H

#include "sppctl.h"

int sppctl_pinctrl_init(struct platform_device *pdev);
void sppctl_pinctrl_clean(struct platform_device *pdev);

#define D(x, y) ((x) * 8 + (y))

extern const struct pinctrl_pin_desc sppctlpins_all[];
extern const size_t sppctlpins_all_nums;
extern const unsigned int sppctlpins_group[];

#endif // SPPCTL_PINCTRL_H
