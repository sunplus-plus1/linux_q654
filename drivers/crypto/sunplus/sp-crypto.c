// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/types.h>
#include <linux/crypto.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>

#include "sp-crypto.h"
#include "sp-aes.h"
#include "sp-hash.h"
#include "sp-rsa.h"

#ifdef CONFIG_CRYPTO_DEV_SP_TEST
#include "sp-crypto-test.c"
#endif

static struct sp_crypto_dev sp_dd_tb[1];

#define OUT(fmt, args...) { p += sprintf(p, fmt, ##args); }

void dump_buf(u8 *buf, u32 len)
{
	static const char s[] = "       |       \n";
	char ss[52] = "";
	u32 i = 0, j;

	pr_info("buf:%px, len:%d\n", buf, len);
	while (i < len) {
		j = i & 0x0F;
		sprintf(ss + j * 3, "%02x%c", buf[i], s[j]);
		i++;
		if ((i & 0x0F) == 0) {
			pr_info("%s", ss);
			ss[0] = 0;
		}
	}
	if (i & 0x0F) {
		//strcat(ss, "\n");
		pr_info("%s", ss);
	}
}
EXPORT_SYMBOL(dump_buf);

/* hwcfg: enable/disable aes/hash hw
 * echo <aes:-1~2> [hash:-1~2] [print] > /sys/module/sp_crypto/parameters/hwcfg
 * -1: no change
 *  0: disable
 *  1: enable
 *  2: toggle
 */
static int hwcfg_set(const char *val, const struct kernel_param *kp)
{
	static int f_aes = 1, f_hash = 1; // initial status: aes & hash enabled
	int en_aes = -1, en_hash = -1, pr = 1;
	int ret;

	sscanf(val, "%d %d %d", &en_aes, &en_hash, &pr);
	if (en_aes >= 0) {
		if (en_aes > 1)
			en_aes = 1 - f_aes; // toggle
		ret = en_aes ? sp_aes_init() : sp_aes_finit();
		f_aes = en_aes;
		if (pr)
			pr_info("sp_aes %s: %d\n", en_aes ? "enable" : "disable", ret);
	}
	if (en_hash >= 0) {
		if (en_hash > 1)
			en_hash = 1 - f_hash; // toggle
		ret = en_hash ? sp_hash_init() : sp_hash_finit();
		f_hash = en_hash;
		if (pr)
			pr_info("sp_hash %s: %d\n", en_hash ? "enable" : "disable", ret);
	}

	return 0;
}

static const struct kernel_param_ops hwcfg_ops = {
	.set = hwcfg_set,
};
module_param_cb(hwcfg, &hwcfg_ops, NULL, 0600);

void *base_va;

irqreturn_t sp_crypto_irq(int irq, void *dev_id)
{
	struct sp_crypto_dev *dev = dev_id;
	struct sp_crypto_reg *reg = dev->reg;
	u32 secif = R(SECIF);
	u32 flag;

	W(SECIF, secif); // clear int
	//pr_info("<%04x>", secif);

	/* aes hash rsa may come at one irq */
	flag = secif & (AES_TRB_IF | AES_ERF_IF | AES_DMA_IF | AES_CMD_RD_IF);
	if (flag)
		sp_aes_irq(dev_id, flag);

	flag = secif & (HASH_TRB_IF | HASH_ERF_IF | HASH_DMA_IF | HASH_CMD_RD_IF);
	if (flag)
		sp_hash_irq(dev_id, flag);

	flag = secif & RSA_DMA_IF;
	if (flag)
		sp_rsa_irq(dev_id, flag);

	return IRQ_HANDLED;
}

/* alloc hw dev */
struct sp_crypto_dev *sp_crypto_alloc_dev(int type)
{
	return sp_dd_tb;
}

/* free hw dev */
void sp_crypto_free_dev(struct sp_crypto_dev *dev, u32 type)
{
}

static void sp_crypto_hw_init(struct sp_crypto_dev *dev)
{
	struct sp_crypto_reg *reg = dev->reg;

	W(SECIE, RSA_DMA_IE | AES_DMA_IE | HASH_DMA_IF);
	SP_CRYPTO_INF("SECIE: %08x\n", R(SECIE));
}

static int sp_crypto_init(void)
{
	int ret = 0;

	SP_CRYPTO_TRACE();
	ret = sp_hash_init();
	ERR_OUT(ret, goto out0, "sp_hash_init");
	ret = sp_aes_init();
	ERR_OUT(ret, goto out1, "sp_aes_init");
	ret = sp_rsa_init();
	ERR_OUT(ret, goto out2, "sp_rsa_init");

	return 0;
out2:
	sp_aes_finit();
out1:
	sp_hash_finit();
out0:
	return ret;

}

static void sp_crypto_exit(void)
{
	SP_CRYPTO_TRACE();
	sp_rsa_finit();
	sp_aes_finit();
	sp_hash_finit();
}

static int sp_crypto_probe(struct platform_device *pdev)
{
	struct sp_crypto_reg *reg;
	struct sp_crypto_dev *dev = sp_dd_tb;//platform_get_drvdata(pdev);
	int ret = 0;

	SP_CRYPTO_TRACE();
	dev->reg = devm_platform_ioremap_resource(pdev, 0);
	if (!dev->reg)
		return -ENXIO;

	dev->device = &pdev->dev;

	SP_CRYPTO_TRACE();
	dev->irq = platform_get_irq(pdev, 0);
	if (dev->irq < 0)
		return -ENODEV;

	/* reset */
	dev->rstc = devm_reset_control_get(&pdev->dev, NULL);
	ERR_OUT(dev->clk, goto out0, "get reset_control");
	ret = reset_control_deassert(dev->rstc);
	ERR_OUT(dev->clk, goto out0, "deassert reset_control");

	/* clock */
	dev->clk = devm_clk_get(&pdev->dev, NULL);
	ERR_OUT(dev->clk, goto out1, "get clk");
	ret = clk_prepare_enable(dev->clk);
	ERR_OUT(ret, goto out1, "enable clk");

	platform_set_drvdata(pdev, dev);
	reg = dev->reg;
	SP_CRYPTO_INF("SP_CRYPTO_ENGINE @ %px =================\n", reg);

	/////////////////////////////////////////////////////////////////////////////////

	dev->version = R(VERSION);
	SP_CRYPTO_INF("devid %d version %0x\n", dev->devid, dev->version);

	ret = devm_request_irq(&pdev->dev, dev->irq, sp_crypto_irq, IRQF_TRIGGER_HIGH, "sp_crypto", dev);
	ERR_OUT(ret, goto out2, "request_irq(%d)", dev->irq);

	sp_crypto_hw_init(dev);

	ret = sp_crypto_init();
	ERR_OUT(ret, goto out2, "sp_crypto_init");

	return 0;

out2:
	clk_disable_unprepare(dev->clk);
out1:
	reset_control_assert(dev->rstc);
out0:
	return ret;
}

static int sp_crypto_remove(struct platform_device *pdev)
{
	struct sp_crypto_dev *dev = platform_get_drvdata(pdev);

	SP_CRYPTO_TRACE();
	sp_crypto_exit();

	clk_disable_unprepare(dev->clk);
	reset_control_assert(dev->rstc);

	return 0;
}

#ifdef CONFIG_PM
#define REGS	35	// G123.0 ~ G124.2
static u32 regs[REGS];
static int sp_crypto_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct sp_crypto_dev *dev = platform_get_drvdata(pdev);
	void __iomem *reg = dev->reg;
	int i;

	SP_CRYPTO_TRACE();
	for (i = 0; i < REGS; i++) // save hw regs
		regs[i] = readl_relaxed(reg + i * 4);
	clk_disable_unprepare(dev->clk);
	reset_control_assert(dev->rstc);

	return 0;
}

static int sp_crypto_resume(struct platform_device *pdev)
{
	struct sp_crypto_dev *dev = platform_get_drvdata(pdev);
	void __iomem *reg = dev->reg;
	int i;

	SP_CRYPTO_TRACE();
	reset_control_deassert(dev->rstc);
	clk_prepare_enable(dev->clk);
	sp_crypto_hw_init(dev);
	for (i = 0; i < REGS; i++) // restore hw regs
		writel_relaxed(regs[i], reg + i * 4);

	return 0;
}
#endif

static const struct of_device_id sp_crypto_of_match[] = {
	{ .compatible = "sunplus,sp7350-crypto" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_crypto_of_match);

static struct platform_driver sp_crtpto_driver = {
	.probe		= sp_crypto_probe,
	.remove		= sp_crypto_remove,
#ifdef CONFIG_PM
	.suspend	= sp_crypto_suspend,
	.resume		= sp_crypto_resume,
#endif
	.driver		= {
		.name	= "sp_crypto",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_crypto_of_match),
	},
};

static int __init sp_crypto_module_init(void)
{
	return platform_driver_register(&sp_crtpto_driver);
}

static void __exit sp_crypto_module_exit(void)
{
	platform_driver_unregister(&sp_crtpto_driver);
}

module_init(sp_crypto_module_init);
module_exit(sp_crypto_module_exit);

MODULE_AUTHOR("Qin Jian <qinjian@sunmedia.com.cn>");
MODULE_DESCRIPTION("Sunplus SP7350 Crypto Engine Driver");
MODULE_LICENSE("GPL v2");
