// SPDX-License-Identifier: GPL-2.0
/*
 * Remote Processor driver
 *
 * Based on origin Zynq Remote Processor driver
 *
 * Copyright (C) 2012 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2012 PetaLogix
 *
 * Based on origin OMAP Remote Processor driver
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/reset.h>
#include <linux/firmware.h>
#include <linux/elf.h>
#include <linux/suspend.h>
#include <linux/workqueue.h>
#include <linux/mailbox_client.h>

#include "remoteproc_internal.h"
#include "remoteproc_elf_helpers.h"
#include <linux/kobject.h>

#define MAX_NUM_VRINGS 2
#define NOTIFYID_ANY (-1)
/* Maximum on chip memories used by the driver*/
#define MAX_ON_CHIP_MEMS        32

#define MAILBOX_CM4_TO_CA55_SUSPEND      0xaabb1234
#define FIRMWARE_WARMBOOT_NAME           "warmboot"

/* Structure for IPIs */
struct ipi_info {
	u32 notifyid;
	bool pending;
};

/**
 * struct sp_rproc_data - sunplus rproc private data
 * @rproc: pointer to remoteproc instance
 * @ipis: interrupt processor interrupts statistics
 * @fw_mems: list of firmware memories
 * @mbox0to2: mbox register for cpu0(local) to cpu2(remote)
 * @mbox2to0: mbox register for cpu2(remote) to cpu0(local)
 * @boot: boot register to boot remote
 */
struct sp_rproc_pdata {
	struct rproc *rproc;
	struct ipi_info ipis[MAX_NUM_VRINGS];
	struct list_head fw_mems;
	u32 __iomem *mbox0to2;
	u32 __iomem *boot;
	u64 bootaddr;
	u32 __iomem *mbox2to0; // read to clear intr
	struct reset_control *rstc; // FIXME: RST_A926 not worked
	struct work_struct suspend_work;
	void __iomem *va[5]; //max num_mems
#ifdef CONFIG_SUNPLUS_MBOX_TEST
	struct mbox_client cl;
	struct mbox_chan *chan;
#endif
};

static bool autoboot __read_mostly;

/* Store rproc for IPI handler */
static struct rproc *rproc;
static struct work_struct workqueue;

#ifdef CONFIG_SUNPLUS_MBOX_TEST
#define MBOX_DATA_SIZE	20

static void mbox_rx_callback(struct mbox_client *cl, void *data)
{
	// for (int i = 0; i < MBOX_DATA_SIZE; i++)
	// 	dev_info(cl->dev, "RX[%02d] %08x\n", i, ((u32 *)data)[i]);
}

static void mbox_tx_test(u32 arg)
{
	struct sp_rproc_pdata *local = rproc->priv;
	struct mbox_client *cl = &local->cl;
	u32 msg[MBOX_DATA_SIZE];
	int i, ret;

	for (i = 0; i < MBOX_DATA_SIZE; i++) {
		msg[i] = arg + i;
		//dev_info(cl->dev, "TX[%02d] %08x\n", i, msg[i]);
	}
	ret = mbox_send_message(local->chan, &msg);
	if (ret < 0)
		dev_err(cl->dev, "mbox_send_message returned %d\n", ret);
}
#endif

static int remoteproc_power_event(struct notifier_block *this, unsigned long event,void *ptr)
{
	char *envp[] = {
		"DEVICE=wlan0",
		NULL
	};

    switch (event)
	{
        case PM_POST_SUSPEND:
			kobject_uevent_env(&rproc->dev.kobj, KOBJ_ONLINE,envp);
			break;
        case PM_SUSPEND_PREPARE:
             break;
        default:
            return NOTIFY_DONE;
      }
	return NOTIFY_DONE;
}

static struct notifier_block remoteproc_power_notifier = {
        .notifier_call = remoteproc_power_event,
};

void suspend_work_func(struct work_struct *work)
{
	char *envp[] = {
		"DEVICE=wlan0",
		NULL
	};
	kobject_uevent_env(&rproc->dev.kobj, KOBJ_OFFLINE,envp);
}

static void sp7350_request_firmware_callback(const struct firmware *fw, void *context)
{
	struct rproc *rproc = context;
	rproc_elf_load_segments(rproc,fw);
	release_firmware(fw);
}

static int sp7350_load_warmboot_firmware(struct rproc *rproc)
{
	int ret;

	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
				      FIRMWARE_WARMBOOT_NAME, &rproc->dev, GFP_KERNEL,
				      rproc, sp7350_request_firmware_callback);
	if (ret < 0)
		dev_err(&rproc->dev, "request_firmware_nowait err: %d\n", ret);
	return ret;
}

static int sp_rproc_load(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	struct sp_rproc_pdata *local = rproc->priv;
	struct elf32_hdr *ehdr;
	struct elf32_phdr *phdr;
	int i, ret = 0;
	const u8 *elf_data = fw->data;

	ehdr = (struct elf32_hdr *)elf_data;
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		u32 da = phdr->p_paddr;
		u32 memsz = phdr->p_memsz;
		u32 filesz = phdr->p_filesz;
		u32 offset = phdr->p_offset;
		void *ptr;
		if (phdr->p_type != PT_LOAD)
			continue;
		dev_dbg(dev, "phdr: type %d memsz 0x%x filesz 0x%x\n",
			phdr->p_type, memsz, filesz);
		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%x memsz 0x%x\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}
		if (offset + filesz > fw->size) {
			dev_err(dev, "truncated fw: need 0x%x avail 0x%zx\n",
				offset + filesz, fw->size);
			ret = -EINVAL;
			break;
		}

		/* grab the kernel address for this device address */
		if (da > local->bootaddr)
			ptr = rproc_da_to_va(rproc, da, memsz);
		else
			ptr = rproc_da_to_va(rproc, local->bootaddr + (da & 0xFFFFF), memsz);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%llx mem 0x%x\n", local->bootaddr, memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (phdr->p_filesz)
			memcpy(ptr, elf_data + offset, phdr->p_filesz);
	}
	return ret;
}

static void handle_event(struct work_struct *work)
{
	struct sp_rproc_pdata *local = rproc->priv;

	if (rproc_vq_interrupt(local->rproc, local->ipis[0].notifyid) ==
				IRQ_NONE)
		dev_dbg(rproc->dev.parent, "no message found in vqid 0\n");
}

static void ipi_kick(void)
{
	dev_dbg(rproc->dev.parent, "KICK Linux because of pending message\n");
	schedule_work(&workqueue);
}

#define RPROC_TRIGGER \
do { \
	writel(local->ipis[i].notifyid, local->mbox0to2); \
} while (0)

static void kick_pending_ipi(struct rproc *rproc)
{
	struct sp_rproc_pdata *local = rproc->priv;
	int i;

	for (i = 0; i < MAX_NUM_VRINGS; i++) {
		/* Send swirq to firmware */
		if (local->ipis[i].pending) {
			RPROC_TRIGGER;
			local->ipis[i].pending = false;
		}
	}
}

static int sp_rproc_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct sp_rproc_pdata *local = rproc->priv;

	dev_dbg(dev, "%s\n", __func__);
	INIT_WORK(&workqueue, handle_event);

	/* Trigger pending kicks */
	kick_pending_ipi(rproc);

	// set remote start addr to boot register,
	writel(0xFFFF0000|(local->bootaddr >> 16), local->boot);
	reset_control_deassert(local->rstc);
	/* for (echo mem) into deepsleep. load warmboot firmware to dram first. */
	sp7350_load_warmboot_firmware(rproc);
	INIT_WORK(&local->suspend_work,suspend_work_func);

	return 0;
}

/* kick a firmware */
static void sp_rproc_kick(struct rproc *rproc, int vqid)
{
	struct device *dev = rproc->dev.parent;
	struct sp_rproc_pdata *local = rproc->priv;
	struct rproc_vdev *rvdev, *rvtmp;
	int i;

	dev_dbg(dev, "KICK Firmware to start send messages vqid %d\n", vqid);

	list_for_each_entry_safe(rvdev, rvtmp, &rproc->rvdevs, node) {
		for (i = 0; i < MAX_NUM_VRINGS; i++) {
			struct rproc_vring *rvring = &rvdev->vring[i];

			/* Send swirq to firmware */
			if (rvring->notifyid == vqid) {
				local->ipis[i].notifyid = vqid;
				/* As we do not turn off CPU1 until start,
				 * we delay firmware kick
				 */
				if (rproc->state == RPROC_RUNNING)
					RPROC_TRIGGER;
				else
					local->ipis[i].pending = true;
			}
		}
	}
}

/* power off the remote processor */
static int sp_rproc_stop(struct rproc *rproc)
{
	struct sp_rproc_pdata *local = rproc->priv;
	struct device *dev = rproc->dev.parent;

	dev_dbg(dev, "%s\n", __func__);
#ifdef CONFIG_SUNPLUS_MBOX_TEST
	mbox_tx_test(0x00000000);
	mbox_tx_test(0xdeadc0de);
#endif
	reset_control_assert(local->rstc);
#ifdef CONFIG_ARM_SP7350_CPUFREQ
	{ // FIXME: force unlock hwspin locked by CM4
		extern void sp7350_dvfs_unlock(void);
		sp7350_dvfs_unlock();
	}
#endif

	return 0;
}

static int sp_parse_fw(struct rproc *rproc, const struct firmware *fw)
{
	struct sp_rproc_pdata *local = rproc->priv;
	void __iomem **va = local->va;
	int num_mems, i, ret;
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct rproc_mem_entry *mem;

	num_mems = of_count_phandle_with_args(np, "memory-region", NULL);
	if (num_mems <= 0)
		return 0;

	for (i = 0; i < num_mems; i++) {
		struct device_node *node;
		struct reserved_mem *rmem;

		node = of_parse_phandle(np, "memory-region", i);
		rmem = of_reserved_mem_lookup(node);
		if (!rmem) {
			dev_err(dev, "unable to acquire memory-region\n");
			return -EINVAL;
		}

		if (strstr(node->name, "vdev"))
			mem = rproc_mem_entry_init(dev, NULL,
					(dma_addr_t)rmem->base,
					rmem->size, rmem->base,
					NULL, NULL,
					node->name);
		else
			mem = rproc_of_resm_mem_entry_init(dev, i,
					rmem->size,
					rmem->base,
					node->name);
		if (!mem) {
			dev_err(dev, "unable to initialize memory-region %s\n", node->name);
			return -ENOMEM;
		}

		if (!strstr(node->name, "buffer")) {
			if (va[i])
				devm_iounmap(dev, va[i]);
			if (strstr(node->name, "cm4runaddr")) {
				local->bootaddr = rmem->base;
				va[i] = mem->va = devm_ioremap(dev, rmem->base, rmem->size);
			} else {
				va[i] = mem->va = devm_ioremap_wc(dev, rmem->base, rmem->size);
			}
			if (!mem->va)
				return -ENOMEM;
			//printk("!!![%s] %08x:%08x -> %px\n", node->name, rmem->base, rmem->size, mem->va);
		}

		rproc_add_carveout(rproc, mem);
	}

	ret = rproc_elf_load_rsc_table(rproc, fw);
	if (ret == -EINVAL)
		ret = 0;
	return ret;
}

static struct rproc_ops sp_rproc_ops = {
	.start		= sp_rproc_start,
	.stop		= sp_rproc_stop,
	.parse_fw	= sp_parse_fw,
	.find_loaded_rsc_table = rproc_elf_find_loaded_rsc_table,
	.get_boot_addr	= rproc_elf_get_boot_addr,
	.kick		= sp_rproc_kick,
	.load		= sp_rproc_load,
};

static irqreturn_t sp_remoteproc_interrupt(int irq, void *dev_id)
{
	struct sp_rproc_pdata *local = rproc->priv;

	if(readl(local->mbox2to0) == MAILBOX_CM4_TO_CA55_SUSPEND) { // read to clear intr
		dev_info(&rproc->dev, "Ca55 in suspend start \n");
		schedule_work(&local->suspend_work);
	} else {
		ipi_kick();
	}

	return IRQ_HANDLED;
}

static int sp_remoteproc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct sp_rproc_pdata *local;
	struct device *dev = &pdev->dev;

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(dev, "missing mbox irq\n");
		return ret;
	}

	ret = devm_request_irq(dev, ret, sp_remoteproc_interrupt,
							IRQF_TRIGGER_NONE, dev_name(dev), NULL);
	if (ret) {
		dev_err(dev, "request rproc irq failed: %d\n", ret);
		return ret;
	}

	rproc = rproc_alloc(dev, dev_name(dev),
			    &sp_rproc_ops, NULL,
		sizeof(struct sp_rproc_pdata));
	if (!rproc) {
		dev_err(dev, "rproc allocation failed\n");
		return -ENOMEM;
	}
	local = rproc->priv;
	local->rproc = rproc;

	platform_set_drvdata(pdev, rproc);

#ifdef CONFIG_SUNPLUS_MBOX_TEST
	local->cl.dev = dev;
	local->cl.rx_callback = mbox_rx_callback;
	local->cl.tx_block = true;
	local->cl.tx_tout = 5000; // timeout 5000 ms

	local->chan = mbox_request_channel(&local->cl, 0);
	if (IS_ERR(local->chan)) {
		int ret = PTR_ERR(local->chan);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "Failed to get mbox channel: %d\n", ret);
		return ret;
	}
#endif

	ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "dma_set_coherent_mask: %d\n", ret);
		goto probe_failed;
	}

	local->mbox0to2 = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR((void *)local->mbox0to2)) {
		ret = PTR_ERR((void *)local->mbox0to2);
		dev_err(dev, "ioremap mbox0to2 reg failed: %d\n", ret);
		goto probe_failed;
	}

	local->boot = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR((void *)local->boot)) {
		ret = PTR_ERR((void *)local->boot);
		dev_err(dev, "ioremap boot reg failed: %d\n", ret);
		goto probe_failed;
	}

	local->mbox2to0 = devm_platform_ioremap_resource(pdev, 2);
	if (IS_ERR((void *)local->mbox2to0)) {
		ret = PTR_ERR((void *)local->mbox2to0);
		dev_err(dev, "ioremap mbox2to0 reg failed: %d\n", ret);
		goto probe_failed;
	}

	local->rstc = devm_reset_control_get(dev, NULL);
	if (IS_ERR(local->rstc)) {
		ret = PTR_ERR(local->rstc);
		dev_err(dev, "missing reset controller\n");
		goto probe_failed;
	}

	rproc->auto_boot = autoboot;

	ret = rproc_add(local->rproc);
	if (ret) {
		dev_err(dev, "rproc registration failed: %d\n", ret);
		goto probe_failed;
	}
	ret = register_pm_notifier(&remoteproc_power_notifier);
	if (ret) {
		dev_err(dev, "pm registration failed: %d\n", ret);
		goto probe_failed;
	}
	return 0;

probe_failed:
	rproc_free(rproc);
	return ret;
}

static int sp_remoteproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	dev_info(dev, "%s\n", __func__);

	rproc_del(rproc);
	of_reserved_mem_device_release(dev);
	rproc_free(rproc);

	return 0;
}

/* Match table for OF platform binding */
static const struct of_device_id sp_remoteproc_match[] = {
	{ .compatible = "sunplus,sp-rproc", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, sp_remoteproc_match);

static struct platform_driver sp_remoteproc_driver = {
	.probe = sp_remoteproc_probe,
	.remove = sp_remoteproc_remove,
	.driver = {
		.name = "sp_remoteproc",
		.of_match_table = sp_remoteproc_match,
	},
};
module_platform_driver(sp_remoteproc_driver);

module_param_named(autoboot, autoboot, bool, 0444);
MODULE_PARM_DESC(autoboot,
		 "enable | disable autoboot. (default: false)");

MODULE_AUTHOR("qinjian@sunmedia.com.cn");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sunplus remote processor control driver");
