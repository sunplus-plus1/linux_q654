// SPDX-License-Identifier: GPL-2.0-only
/*
 * Timer Driver
 *
 */

#include <linux/cdev.h>
#include <linux/compat.h>
#include <linux/clk.h>
#include <linux/err.h>		/* For IS_ERR macros */
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/idr.h>		/* For ida_* macros */
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>	/* For printk/panic/... */
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>	/* For mutexes */
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>		/* For memory functions */
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm-generic/siginfo.h>
#include <linux/init.h>
#include <asm/signal.h>

static DEFINE_IDA(timer_ida);

#define TRACE(s) printk("### %s:%d %s\n", __FUNCTION__, __LINE__, s)

//#define CONFIG_SP_TIMER_DEBUG
#ifdef CONFIG_SP_TIMER_DEBUG
	#define TAG "Timer: "
	#define sp_timer_dbg(fmt, ...) printk(KERN_INFO TAG fmt, ##__VA_ARGS__);
#else
	#define sp_timer_dbg(fmt, ...) do {} while (0)
#endif

#define DEVICE_NAME		"sunplus-timer"

#define	TIMER_IOCTL_BASE 	'T'

#define	TMRIOC_SETOPTIONS	_IOR(TIMER_IOCTL_BASE, 0, int)
#define	TMRIOC_SETTIME		_IOWR(TIMER_IOCTL_BASE, 1, int)
#define	TMRIOC_GETTIMELEFT	_IOR(TIMER_IOCTL_BASE, 2, int)
#define	TMRIOC_GETINFO		_IOR(TIMER_IOCTL_BASE, 3, int)

/* Ioctrl set options */
#define TMRIOS_DISABLECARD	1
#define	TMRIOS_ENABLECARD	2
#define	TMRIOS_SELMODE		4
#define	TMRIOS_SELSRC		8

#define TIMER_DEVICES		32

#define sp_timer_groups NULL  //TODO refer watchdog

/* Timer register offset */
#define TIMER_CTRL		0x00
#define TIMER_CNT		0x04
#define TIMER_RELOAD		0x08

/* The register width of timer 0,1,2 in each STC group */
#define REG_WIDTH		0x0C

/* Timer counter start/stop */
#define TIMER_START		BIT(11)
#define TIMER_STOP		~BIT(11)

/* Timer count mode selection: oneshot/repeat */
#define SP_TM_OS		0
#define SP_TM_RPT		1

#define TIMER_REPEAT		BIT(13)
#define TIMER_ONE_SHOT		~BIT(13)

/* Timer clock source selection: oneshot/repeat */
#define SP_TM_SYS		0
#define SP_TM_STC		1
#define SP_TM_RTC		2
#define SP_TM_WRAP		3

#define TIMER_SRC_MSK		GENMASK(15,14)
#define TIMER_SRC(x)		((x) << 14)

static struct mutex sp_timer_sem;
static dev_t sp_timer_major;

struct sp_timer_priv {
	struct device dev;	/* internal device */
	struct cdev cdev;
	struct mutex lock;
	struct device *parent;	/* pdev->dev */
	struct device_node *parent_np;
	unsigned long status;		/* Internal status bits */

	void __iomem *base;
	int irqn;

	u8 mode;		/* 0:onshot 1:reload */
	u8 src;			/* 0:sysclk 1:stc 2:rtc 3:wrap */
	u32 freq;
	u32 rel_cnt;

	u8 tmr_id; //TODO: Need modify
	u8 stc_id;

	struct fasync_struct *async;

#define _SP_TIME_DEV_OPEN		0	/* Opened ? */
#define _SP_TIME_ACTIVE			1	/* Active ? */
};

static inline bool sp_timer_active(struct sp_timer_priv *priv)
{
	return test_bit(_SP_TIME_ACTIVE, &priv->status);
}

/* Used for ioctl SETOPTIONS*/
static int sp_timer_stop(struct sp_timer_priv *priv)
{
	void __iomem *base = priv->base;
	u32 val;

	sp_timer_dbg("### %s(%d) \n", __FUNCTION__, __LINE__);

	val = readl_relaxed(base + TIMER_CTRL);
	val &= TIMER_STOP;
	writel(val, base + TIMER_CTRL);

	clear_bit(_SP_TIME_ACTIVE, &priv->status);

	return 0;
}

static int sp_timer_start(struct sp_timer_priv *priv)
{
	void __iomem *base = priv->base;
	u32 val;

	sp_timer_dbg("### %s(%d) \n", __FUNCTION__, __LINE__);

	val = readl_relaxed(base + TIMER_CTRL);
	val |= TIMER_START;
	writel(val, base + TIMER_CTRL);

	set_bit(_SP_TIME_ACTIVE, &priv->status);

	return 0;
}

static int sp_timer_get_timeleft(struct sp_timer_priv *priv, u32 *val)
{
	void __iomem *base = priv->base;

	//sp_timer_dbg("### %s(%d) \n", __FUNCTION__, __LINE__);

	*val = readl_relaxed(base + TIMER_CNT);

	return 0;
}

static int sp_timer_sel_mode(struct sp_timer_priv *priv)
{
	void __iomem *base = priv->base;
	u8 mode = priv->mode;
	u32 val;

	val = readl_relaxed(base + TIMER_CTRL);
	if(mode == SP_TM_OS)
		val &= TIMER_ONE_SHOT;
	else
		val |= TIMER_REPEAT;
	writel(val, base + TIMER_CTRL);

	return 0;
}

static int sp_timer_sel_src(struct sp_timer_priv *priv)
{
	void __iomem *base = priv->base;
	u8 src = priv->src;
	u32 val;

	if (src > SP_TM_WRAP) {
		printk("Invaild sourcce selection\n");
		return -1;
	}

	val = readl_relaxed(base + TIMER_CTRL);
	/* Clear the clk src field bit[15:14] */
	val &= ~TIMER_SRC_MSK;
	val |= TIMER_SRC(src);
	writel(val, base + TIMER_CTRL);

	return 0;
}

static int sp_timer_set_cnt(struct sp_timer_priv *priv, u32 cnt)
{
	void __iomem *base = priv->base;
	u8 mode = priv->mode;
	u32 val;

	if (sp_timer_active(priv)) {
		val = readl_relaxed(base + TIMER_CTRL);
		val &= TIMER_STOP;
		writel(val, base +  TIMER_CTRL);
	}

	writel(cnt, base + TIMER_CNT);
	if(mode == SP_TM_RPT)
		writel(cnt, base + TIMER_RELOAD);

	if (sp_timer_active(priv)) {
		val = readl_relaxed(base + TIMER_CTRL);
		val |= TIMER_START;
		writel(val, base + TIMER_CTRL);
	}

	priv->rel_cnt = cnt;

	return 0;
}

static int sp_timer_hw_init(struct sp_timer_priv *priv)
{
	sp_timer_sel_src(priv);
	sp_timer_sel_mode(priv);
	sp_timer_set_cnt(priv, priv->rel_cnt);

	return 0;
}

static long sp_timer_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	struct sp_timer_priv *priv = file->private_data;
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	unsigned int val;
	int err;

	mutex_lock(&priv->lock);

	//sp_timer_dbg("1### %s(%d) \n", __FUNCTION__, __LINE__);

	switch (cmd) {
	case TMRIOC_SETOPTIONS:
		if (get_user(val, p))
			err = -EFAULT;
		if (val & TMRIOS_DISABLECARD)
			err = sp_timer_stop(priv);
		else if (val & TMRIOS_ENABLECARD)
			err = sp_timer_start(priv);
		else if (val & TMRIOS_SELMODE)
			err = sp_timer_sel_mode(priv);
		else if (val & TMRIOS_SELSRC)
			err = sp_timer_sel_src(priv);
		break;
	case TMRIOC_SETTIME:
		if (get_user(val, p)) {
			err = -EFAULT;
			break;
		}
		err = sp_timer_set_cnt(priv, val);
		if (err < 0)
			break;
		fallthrough;
	case TMRIOC_GETTIMELEFT:
		err = sp_timer_get_timeleft(priv, &val);
		if (err < 0)
			break;
		err = put_user(val, p);
		break;
	case TMRIOC_GETINFO:
		printk("STC%dTIMER%d:sel_src=%d freq=%d, mode=%d, rel_time=%d",
			priv->stc_id,
			priv->tmr_id,
			priv->src,
			priv->freq,
			priv->mode,
			priv->rel_cnt);
		break;
	default:
		err = -ENOTTY;
		break;
	}

	mutex_unlock(&priv->lock);

	return err;
}

static int sp_timer_open(struct inode *inode, struct file *file)
{
	struct sp_timer_priv *priv;

	priv = container_of(inode->i_cdev, struct sp_timer_priv,
				       cdev);

	/* The timer is single open! */
	if (test_and_set_bit(_SP_TIME_DEV_OPEN, &priv->status))
		return -EBUSY;

	sp_timer_start(priv);

	file->private_data = priv;

	if (!sp_timer_active(priv))
		get_device(&priv->dev);

	return 0;
}

static int sp_timer_release(struct inode *inode, struct file *file)
{
	struct sp_timer_priv *priv = file->private_data;

	/* make sure that /dev/time{x} can be re-opened */
	clear_bit(_SP_TIME_DEV_OPEN, &priv->status);

	if (!sp_timer_active(priv))
		put_device(&priv->dev);

	return 0;
}

static int sp_timer_fasync(int fd, struct file *file, int mode)
{
	struct sp_timer_priv *priv = file->private_data;

	/* Register the user space pid */
	return fasync_helper(fd, file, mode, &priv->async);
}

static void sp_timer_priv_release(struct device *dev)
{
	struct sp_timer_priv *priv;

	priv = container_of(dev, struct sp_timer_priv, dev);

	kfree(priv);
}

static const struct file_operations sp_timer_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= sp_timer_ioctl,
	.compat_ioctl	= compat_ptr_ioctl,
	.open		= sp_timer_open,
	.release	= sp_timer_release,
	.fasync		= sp_timer_fasync,
};

static struct class sp_timer_class = {
	.name =		"timer",
	.owner =	THIS_MODULE,
	.dev_groups =	sp_timer_groups,
};

/*
 *	timer_cdev_register: register timer character device
 *	@priv: timer device
 *
 *	Register a timer character device including handling the legacy
 *	/dev/timer node. /dev/timer is actually a miscdevice and
 *	thus we set it up like that.
 */
static int timer_cdev_register(struct sp_timer_priv *priv)
{
	int err;

	mutex_init(&priv->lock);

	device_initialize(&priv->dev);
	priv->dev.devt = MKDEV(MAJOR(sp_timer_major), priv->tmr_id);
	priv->dev.class = &sp_timer_class;
	priv->dev.parent = priv->parent;
	//priv->dev.groups = priv->groups;
	priv->dev.release = sp_timer_priv_release;
	dev_set_drvdata(&priv->dev, priv);
	dev_set_name(&priv->dev, "timer%d", priv->tmr_id);

	/* Fill in the data structures */
	cdev_init(&priv->cdev, &sp_timer_fops);

	/* Add the device */
	err = cdev_device_add(&priv->cdev, &priv->dev);
	if (err) {
		pr_err("timer%d unable to add device %d:%d\n",
			priv->tmr_id,  MAJOR(sp_timer_major), priv->tmr_id);
		return err;
	}
	priv->cdev.owner = THIS_MODULE;

	//device_create(&sp_timer_class, priv->parent, priv->dev.devt, NULL, "%s%d", "timer", priv->id);

	return 0;
}

static void timer_cdev_unregister(struct sp_timer_priv *priv)
{
	cdev_device_del(&priv->cdev, &priv->dev);

	mutex_lock(&priv->lock);

	priv = NULL;

	mutex_unlock(&priv->lock);

	put_device(&priv->dev);
}

static void devm_timer_cdev_unregister(struct device *dev, void *res)
{
	timer_cdev_unregister(*(struct sp_timer_priv **)res);
}

int devm_timer_cdev_register(struct device *dev,
				struct sp_timer_priv *priv)
{
	struct sp_timer_priv **rcpriv;
	int ret;

	rcpriv = devres_alloc(devm_timer_cdev_unregister, sizeof(*rcpriv),
			     GFP_KERNEL);
	if (!rcpriv)
		return -ENOMEM;

	ret = timer_cdev_register(priv);
	if (!ret) {
		*rcpriv = priv;
		devres_add(dev, rcpriv);
	} else {
		devres_free(rcpriv);
	}

	return ret;
}

static irqreturn_t sp_timer_isr(int irq, void *arg)
{
	struct sp_timer_priv *priv = arg;

	//printk("k\n");

	kill_fasync(&priv->async, SIGIO, POLL_IN);

	return IRQ_HANDLED;
}

static int sp_timer_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sp_timer_priv *priv;
	int ret;
	int irq;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

	priv->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	irq = platform_get_irq_optional(pdev, 0);
	if (irq > 0) {
		ret = devm_request_irq(dev, irq, sp_timer_isr, 0, dev_name(&pdev->dev), priv);
		if (ret)
			return ret;
	} else {
		return -EPROBE_DEFER;
	}

	priv->irqn = irq;
	priv->parent_np = of_get_parent(dev->of_node);
	priv->parent = dev;

	/* Timers hw have no pre-scaler function, temporarily only consider the STC as the source */
	priv->src = SP_TM_STC;
	ret = of_property_read_u32(priv->parent_np, "stc-freq", &priv->freq);
	if (ret) {
		dev_err(dev, "Failed to get STC frequency (%d)\n", ret);
		return ret;
	}

	ret = of_property_read_u8(dev->of_node, "timer-mode", &priv->mode);
	if (ret) {
		dev_err(dev, "Failed to get timer mode (%d)\n", ret);
 		return ret;
	}

	ret = of_alias_get_id(dev->of_node, "timer");
	if (ret >= 0)
		priv->tmr_id = ida_simple_get(&timer_ida, ret, ret + 1, GFP_KERNEL);
	sp_timer_dbg("%s(%d) ret %d ida %d\n", __FUNCTION__, __LINE__, ret, priv->tmr_id);

	priv->stc_id = of_alias_get_id(priv->parent_np, "stc");

	/* Default value */
	priv->rel_cnt = 100000; //1s

	sp_timer_hw_init(priv);
	printk("STC%dTIMER%d:irqn=%d, sel_src=%d freq=%d, mode=%d, rel_time=%d",
			priv->stc_id,
			priv->tmr_id,
			priv->irqn,
			priv->src,
			priv->freq,
			priv->mode,
			priv->rel_cnt);
#if 0// xtdebug
	sp_timer_start(&priv->priv);
	u32 *stc_av4 = ioremap(0xf8801300, 0x80);
	int i;
	sp_timer_dbg("Dump STC_AV4 Group REG: \n");
	sp_timer_dbg("G38.3  stc_divisor 0x%x\n", *(stc_av4 + 3));
	sp_timer_dbg("G38.9  timer0_ctl 0x%x\n", *(stc_av4 + 9));
	sp_timer_dbg("G38.10 timer0_cnt 0x%x\n", *(stc_av4 + 10));
	sp_timer_dbg("G38.11 timer0_rel 0x%x\n", *(stc_av4 + 11));
	//sp_timer_dbg("Dump STC_AV4 0xf8801300\n");
	//for (i = 0; i < 32; i++) {
		//sp_timer_dbg("%d 0x%x\n", i, *(stc_av4 + i));
	//}
#endif
	return devm_timer_cdev_register(dev, priv);
}

#ifdef CONFIG_PM_SLEEP
static u32 regs[4];
static int sp_timer_suspend(struct device *dev)
{
	struct sp_timer_priv *priv = dev_get_drvdata(dev);
	void __iomem *base = priv->base;
	int i;

	sp_timer_dbg(">>>>>> [DEBUG] TIMER suspend <<<<<<\n");

	if (sp_timer_active(priv))
		sp_timer_stop(priv);

	/* Save the reg val */
	for(i = 0; i < 3; i++)
		regs[i] = readl_relaxed(base + i * 0x4);

	return 0;
}

static int sp_timer_resume(struct device *dev)
{
	struct sp_timer_priv *priv = dev_get_drvdata(dev);
	void __iomem *base = priv->base;
	int i;

	sp_timer_dbg(">>>>>> [DEBUG] TIMER resume <<<<<<\n");

	/* Restore the reg val */
	for(i = 0; i < 3; i++)
		writel(regs[i], base + i * 0x4);

	if (sp_timer_active(priv))
		sp_timer_start(priv);

	return 0;
}
#endif

static const struct of_device_id sp_timer_of_match[] = {
	{.compatible = "sunplus,sp7350-timer", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_timer_of_match);

static const struct dev_pm_ops sp_timer_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sp_timer_suspend,
				sp_timer_resume)
};

static struct platform_driver sp_timer_driver = {
	.probe = sp_timer_probe,
	.driver = {
		.name		= DEVICE_NAME,
		.pm		= &sp_timer_pm_ops,
		.of_match_table = sp_timer_of_match,
	},
};

//module_platform_driver(sp_timer_driver);

static int __init timer_module_init(void)
{
	int err;

	//sp_timer_class = class_create("sunplus_timer");
	err = class_register(&sp_timer_class);
	if (err < 0) {
		pr_err("Failed to register timer class, err %d\n", err);
		goto err_register;
	}

	mutex_init(&sp_timer_sem);

	err = alloc_chrdev_region(&sp_timer_major, 0, TIMER_DEVICES, DEVICE_NAME);
	if (err < 0) {
		pr_err("Failed to alloc timer cdev region, err %d\n", err);
		goto err_alloc;
	}

	err = platform_driver_register(&sp_timer_driver);
	if (err)
		goto err_platform;

	return 0;

err_platform:
	unregister_chrdev_region(sp_timer_major, TIMER_DEVICES);
err_alloc:
	class_unregister(&sp_timer_class);
err_register:
	return err;
}

static void __exit timer_module_exit(void)
{
	class_unregister(&sp_timer_class);

	//platform_driver_unregister(&hwicap_platform_driver);

	unregister_chrdev_region(sp_timer_major, TIMER_DEVICES);
}

module_init(timer_module_init);
module_exit(timer_module_exit);

MODULE_AUTHOR("Xiantao Hu <xt.hu@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus Watchdog Timer Driver");
MODULE_LICENSE("GPL v2");
