/* SPDX-License-Identifier: GPL-2.0 */

#ifndef SPPCTL_SYSFS_H
#define SPPCTL_SYSFS_H

#include "sppctl.h"

void sppctl_sysfs_init(struct platform_device *pdev);
void sppctl_sysfs_clean(struct platform_device *pdev);

#endif // SPPCTL_SYSFS_H
