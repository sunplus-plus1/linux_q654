// SPDX-License-Identifier: GPL-2.0-only

#include <linux/extcon-provider.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_platform.h>
#include <linux/clk.h>

#define USB_GPIO_DEBOUNCE_MS	20	/* ms */

struct u2phy_regs {
	unsigned int cfg[32];		       // 149.0
};

struct u3phy_regs {
	unsigned int cfg[32];		       // 149.0
};

struct u3blkdev_regs {
	unsigned int cfg[32];		       // 149.0
};

struct usb3_phy {
	struct device		*dev;
	void __iomem		*u3phy_base_addr;
	void __iomem		*u3_portsc_addr;
	void __iomem		*u3_blkdev_addr;
	struct clk		*u3_clk;
	struct clk		*u3phy_clk;
	struct reset_control	*u3phy_rst;
	int			irq;
	wait_queue_head_t	wq;
	int			busy;
	struct delayed_work	typecdir;
	int			dir;
	struct gpio_desc	*gpiodir;
};

struct usb_extcon_info {
	struct device		*dev;
	struct extcon_dev	*edev;
	void __iomem		*u2phy_base_addr;
	struct gpio_desc	*id_gpiod;
	struct gpio_desc	*vbus_gpiod;
	int			id_irq;
	int			vbus_irq;

	unsigned long		debounce_jiffies;
	struct delayed_work	wq_detcable;
	struct usb3_phy		*spphydata;
	/* Usb3 vbus eco solution */
	int			chip_version;
	struct clk		*u2phy_clk;
};

static const unsigned int usb_extcon_cable[] = {
	EXTCON_USB,
	EXTCON_USB_HOST,
	EXTCON_NONE,
};

/*
 * "USB" = VBUS and "USB-HOST" = !ID, so we have:
 * Both "USB" and "USB-HOST" can't be set as active at the
 * same time so if "USB-HOST" is active (i.e. ID is 0)  we keep "USB" inactive
 * even if VBUS is on.
 *
 *  State              |    ID   |   VBUS
 * ----------------------------------------
 *  [1] USB            |    H    |    H
 *  [2] none           |    H    |    L
 *  [3] USB-HOST       |    L    |    H
 *  [4] USB-HOST       |    L    |    L
 *
 * In case we have only one of these signals:
 * - VBUS only - we want to distinguish between [1] and [2], so ID is always 1.
 * - ID only - we want to distinguish between [1] and [4], so VBUS = ID.
 */
static int pre_id = 2;
static int pre_u3linkstate = 0xff;
static void usb_extcon_detect_cable(struct work_struct *work)
{
	int id, vbus, dir, u3linkstate;
	struct usb_extcon_info *info = container_of(to_delayed_work(work),
						    struct usb_extcon_info,
						    wq_detcable);
	struct u2phy_regs *phy_reg;

	phy_reg = (struct u2phy_regs *)info->u2phy_base_addr;
	/* check ID and VBUS and update cable state */
	id = info->id_gpiod ?
		gpiod_get_value_cansleep(info->id_gpiod) : 0;
	vbus = info->vbus_gpiod ?
		gpiod_get_value_cansleep(info->vbus_gpiod) : id;
	/* workaround for vbus issue */
	if (info->chip_version == 0xa30) {
		if (id) {
			struct u3blkdev_regs *exconblkdev_reg = info->spphydata->u3_blkdev_addr;

			u3linkstate = (readl(&exconblkdev_reg->cfg[3]) & 0x3c0000) >> 18;
			if (u3linkstate != pre_u3linkstate) {
				if (u3linkstate == 0x4) {
					int temp_reg = readl(&exconblkdev_reg->cfg[1]);

					temp_reg &= ~0x1e0;
					writel(temp_reg, &exconblkdev_reg->cfg[1]);
					temp_reg |= (0x6 << 5);
					writel(temp_reg, &exconblkdev_reg->cfg[1]);
				}
				pre_u3linkstate = u3linkstate;
			}
		}
	}

	if (id != pre_id) {
		dir = info->spphydata->gpiodir ? gpiod_get_value(info->spphydata->gpiodir) : 0;
		if (info->spphydata->dir == dir) {
			pre_id = id;
			//printk("@@@usb_extcon_detect_cable id 0x%x vbus 0x%x\n", id, vbus);
			/* at first we clean states which are no longer active */
			if (id)
				extcon_set_state_sync(info->edev, EXTCON_USB_HOST, false);
			if (!vbus)
				extcon_set_state_sync(info->edev, EXTCON_USB, false);

			if (!id) {
				/* Usb3 vbus eco solution */
				phy_reg->cfg[29] |= (3 << 30);
				extcon_set_state_sync(info->edev, EXTCON_USB_HOST, true);
			} else {
				if (vbus) {
					/* Usb3 vbus eco solution */
					phy_reg->cfg[29] |= (1 << 31);
					phy_reg->cfg[29] &= ~(1 << 30);
					extcon_set_state_sync(info->edev, EXTCON_USB, true);
				}
			}
		} else {
			schedule_delayed_work(&info->wq_detcable, msecs_to_jiffies(10));
			return;
		}
	}
	schedule_delayed_work(&info->wq_detcable, msecs_to_jiffies(150));
}

static int usb_extcon_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct usb_extcon_info *info;
	int ret;
	struct resource *u2phy_res_mem;
	struct device_node *phynp = of_find_compatible_node(NULL, NULL, "sunplus,usb3-phy");
	struct platform_device *spphypdev = of_find_device_by_node(phynp);
	void __iomem *stamp;

	if (!np)
		return -EINVAL;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = dev;
	info->id_gpiod = devm_gpiod_get_optional(&pdev->dev, "id", GPIOD_IN);
	info->vbus_gpiod = devm_gpiod_get_optional(&pdev->dev, "vbus", GPIOD_IN);

	if (!info->id_gpiod && !info->vbus_gpiod)
		pr_info("[USB3EXTCON] no id gpios\n");

	/* Usb3 vbus eco solution */
	stamp = ioremap(0xf8800000, 1);
	info->chip_version = readl(stamp);
	iounmap(stamp);

	u2phy_res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->u2phy_base_addr = devm_ioremap(&pdev->dev, u2phy_res_mem->start,
					     resource_size(u2phy_res_mem));
	if (IS_ERR(info->u2phy_base_addr))
		return PTR_ERR(info->u2phy_base_addr);

	/*enable u2 phy system clock*/
	info->u2phy_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(info->u2phy_clk)) {
		dev_err(dev, "not found clk source\n");
		return PTR_ERR(info->u2phy_clk);
	}
	clk_prepare_enable(info->u2phy_clk);

	info->edev = devm_extcon_dev_allocate(dev, usb_extcon_cable);
	if (IS_ERR(info->edev)) {
		dev_err(dev, "failed to allocate extcon device\n");
		return -ENOMEM;
	}

	if (!phynp) {
		dev_err(dev, "!!!no phy\n");
	} else {
		info->spphydata = dev_get_drvdata(&spphypdev->dev);
		if (!info->spphydata)
			dev_err(dev, "!!!no phy data\n");
	}
	/* Usb3 vbus eco solution end*/
	ret = devm_extcon_dev_register(dev, info->edev);
	if (ret < 0) {
		dev_err(dev, "failed to register extcon device\n");
		return ret;
	}

	if (info->id_gpiod)
		ret = gpiod_set_debounce(info->id_gpiod,
					 USB_GPIO_DEBOUNCE_MS * 1000);
	if (!ret && info->vbus_gpiod)
		ret = gpiod_set_debounce(info->vbus_gpiod,
					 USB_GPIO_DEBOUNCE_MS * 1000);

	if (ret < 0)
		info->debounce_jiffies = msecs_to_jiffies(USB_GPIO_DEBOUNCE_MS);

	INIT_DELAYED_WORK(&info->wq_detcable, usb_extcon_detect_cable);

	platform_set_drvdata(pdev, info);
	device_set_wakeup_capable(&pdev->dev, true);
	/* Perform initial detection */
	usb_extcon_detect_cable(&info->wq_detcable.work);
	//pr_info("%s end\n", __func__);
	return 0;
}

static int usb_extcon_remove(struct platform_device *pdev)
{
	struct usb_extcon_info *info = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&info->wq_detcable);
	device_init_wakeup(&pdev->dev, false);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int usb_extcon_suspend(struct device *dev)
{
	struct usb_extcon_info *info = dev_get_drvdata(dev);
	int ret = 0;

	if (device_may_wakeup(dev)) {
		if (info->id_gpiod) {
			ret = enable_irq_wake(info->id_irq);
			if (ret)
				return ret;
		}
		if (info->vbus_gpiod) {
			ret = enable_irq_wake(info->vbus_irq);
			if (ret) {
				if (info->id_gpiod)
					disable_irq_wake(info->id_irq);

				return ret;
			}
		}
	}

	/*
	 * We don't want to process any IRQs after this point
	 * as GPIOs used behind I2C subsystem might not be
	 * accessible until resume completes. So disable IRQ.
	 */
	if (info->id_gpiod)
		disable_irq(info->id_irq);
	if (info->vbus_gpiod)
		disable_irq(info->vbus_irq);

	if (!device_may_wakeup(dev))
		pinctrl_pm_select_sleep_state(dev);

	return ret;
}

static int usb_extcon_resume(struct device *dev)
{
	struct usb_extcon_info *info = dev_get_drvdata(dev);
	int ret = 0;

	if (!device_may_wakeup(dev))
		pinctrl_pm_select_default_state(dev);

	if (device_may_wakeup(dev)) {
		if (info->id_gpiod) {
			ret = disable_irq_wake(info->id_irq);
			if (ret)
				return ret;
		}
		if (info->vbus_gpiod) {
			ret = disable_irq_wake(info->vbus_irq);
			if (ret) {
				if (info->id_gpiod)
					enable_irq_wake(info->id_irq);

				return ret;
			}
		}
	}

	if (info->id_gpiod)
		enable_irq(info->id_irq);
	if (info->vbus_gpiod)
		enable_irq(info->vbus_irq);

	queue_delayed_work(system_power_efficient_wq,
			   &info->wq_detcable, 0);

	return ret;
}
#endif

static SIMPLE_DEV_PM_OPS(usb_extcon_pm_ops,
			 usb_extcon_suspend, usb_extcon_resume);

static const struct of_device_id usb_extcon_dt_match[] = {
	{ .compatible = "linux,extcon-usb-gpio", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, usb_extcon_dt_match);

static const struct platform_device_id usb_extcon_platform_ids[] = {
	{ .name = "extcon-usb-gpio", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(platform, usb_extcon_platform_ids);

static struct platform_driver usb_extcon_driver = {
	.probe		= usb_extcon_probe,
	.remove		= usb_extcon_remove,
	.driver		= {
		.name	= "extcon-usb-gpio",
		.pm	= &usb_extcon_pm_ops,
		.of_match_table = usb_extcon_dt_match,
	},
	.id_table = usb_extcon_platform_ids,
};

module_platform_driver(usb_extcon_driver);

MODULE_AUTHOR("ChingChouHuang <chingchouhuang@sunplus.com>");
MODULE_DESCRIPTION("Sunplus USB GPIO extcon driver");
MODULE_LICENSE("GPL v2");
