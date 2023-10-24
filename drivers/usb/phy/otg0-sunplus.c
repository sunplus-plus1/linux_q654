// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/sp_usb.h>
#include "otg-sunplus.h"

struct sp_otg *sp_otg0_host;
EXPORT_SYMBOL(sp_otg0_host);

static const struct of_device_id otg0_sunplus_dt_ids[] = {
	{ .compatible = "sunplus,sp7350-usb-otg" },
	{ }
};
MODULE_DEVICE_TABLE(of, otg0_sunplus_dt_ids);

#ifdef CONFIG_PM
struct dev_pm_ops const otg0_sunplus_pm_ops = {
	.suspend	= sp_otg_suspend,
	.resume		= sp_otg_resume,
};
#endif

static struct platform_driver sunplus_usb_otg0_driver = {
	.probe		= sp_otg_probe,
	.remove		= sp_otg_remove,
	.driver		= {
		.name	= "sunplus-usb-otg0",
		.of_match_table = otg0_sunplus_dt_ids,
#ifdef CONFIG_PM
		.pm = &otg0_sunplus_pm_ops,
#endif
	},
};

static int __init usb_otg0_sunplus_init(void)
{
	if (sp_port0_enabled & PORT0_ENABLED) {
		pr_notice("register sunplus_usb_otg0_driver\n");
		return platform_driver_register(&sunplus_usb_otg0_driver);
	}

	pr_notice("otg0 not enabled\n");

	return 0;
}
module_init(usb_otg0_sunplus_init);

static void __exit usb_otg0_sunplus_exit(void)
{
	if (sp_port0_enabled & PORT0_ENABLED) {
		pr_notice("unregister sunplus_usb_otg0_driver\n");
		platform_driver_unregister(&sunplus_usb_otg0_driver);
	} else {
		pr_notice("otg0 not enabled\n");
		return;
	}
}
module_exit(usb_otg0_sunplus_exit);

MODULE_AUTHOR("Vincent Shih <vincent.shih@sunplus.com>");
MODULE_DESCRIPTION("Sunplus USB OTG (port 0) driver");
MODULE_LICENSE("GPL v2");

