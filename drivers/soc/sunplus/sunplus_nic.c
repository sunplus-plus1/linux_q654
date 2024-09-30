// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/firmware.h>
#include <asm/irq.h>

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>

#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/reset.h>

#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>

#include "nic_reg.h"
#include "hal_nic.h"

enum NIC_Status_e {
	NIC_SUCCESS,                /* successful */
	NIC_ERR,
};

#define NIC_MAIN_REG  "nic_main"
#define NIC_PAI_REG  "nic_pai"
#define NIC_PAII_REG  "nic_paii"
#define DEVICE_NAME	  "sunplus,sp7350-nic"

struct master_port_info {
	char c_hw_id;
	char c_data_width;
	char c_name[16];
};

struct master_port_info PORT_INFO[MASTER_MAX_CNT] = {
	/*nic main*/
	{ 0, 4, "CA55_M0"},
	{ 1, 3, "CSDBG_M1"},
	{ 2, 4, "CSETR_M2"},
	{ 3, 4, "NPU_MA"},
	{ 4, 4, "AXI_DMA_M0"},
	{ 5, 4, "AXI_DMA_M1"},
	{ 6, 4, "CBDMA0_MA"},
	{ 7, 4, "CPIOR0_MA"},
	{ 8, 4, "CPIOR1_MA"},
	{ 9, 3, "SEC_AES"},
	{10, 3, "SEC_HASH"},
	{11, 3, "SEC_RAS"},
	{12, 3, "USB30C0_MA"},
	{13, 3, "USBC0_MA"},
	{14, 3, "CARD0_MA"},
	{15, 3, "CARD1_MA"},
	{16, 3, "CARD2_MA"},
	{17, 2, "SPI_NOR_MA"},
	{18, 2, "NBS_MA"},
	{19, 2, "GMAC_MA"},
	{20, 4, "DUMMY0_MA"},
	{21, 3, "UART2AXI_MA"},
	{22, 3, "VI23_CSIIW0_MA"},
	{23, 3, "VI23_CSIIW1_MA"},
	{24, 3, "VI23_CSIIW2_MA"},
	{25, 3, "VI23_CSIIW3_MA"},
	/*nic pai*/
	{26, 4, "SLAM_MA"},
	{27, 3, "VC_8000E_MA"},
	{28, 3, "VCDNANO_MA"},
	/*nic paii*/
	{29, 3, "IMGREAD0_MA"},
	{30, 3, "DISP_OSD0_MA"},
	{31, 3, "DISP_OSD1_MA"},
	{32, 3, "DISP_OSD2_MA"},
	{33, 3, "DISP_OSD3_MA"},
	{34, 3, "VI0_CSIIW0_MA"},
	{35, 3, "VI0_CSIIW1_MA"},
	{36, 3, "VI1_CSIIW0_MA"},
	{37, 3, "VI1_CSIIW1_MA"},
	{38, 3, "VI4_CSIIW0_MA"},
	{39, 3, "VI4_CSIIW1_MA"},
	{40, 3, "VI5_CSIIW0_MA"},
	{41, 3, "VI5_CSIIW1_MA"},
	{42, 3, "VI5_CSIIW2_MA"},
	{43, 3, "VI5_CSIIW3_MA"},
	{44, 4, "DUMMY1_MA"},
};

/* ---------------------------------------------------------------------------------------------- */
#define NIC_FUNC_DEBUG
#define NIC_KDBG_INFO
#define NIC_KDBG_ERR

#ifdef NIC_FUNC_DEBUG
	#define FUNC_DEBUG()    pr_info("[NIC]: %s(%d)\n", __func__, __LINE__)
#else
	#define FUNC_DEBUG()
#endif

#ifdef NIC_KDBG_INFO
#define DBG_INFO(fmt, args ...)	pr_info("K_NIC: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef NIC_KDBG_ERR
#define DBG_ERR(fmt, args ...)	pr_err("K_NIC: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif

struct sunplus_nic {
	struct clk *axiclk;
	struct reset_control *rstc;
};

struct sunplus_nic sp_nic;

static struct timer_list nic_timer;
unsigned char counter = 10;
u64 select_ports = 0x000000100001;

void nic_timer_callback(struct timer_list *timer)
{
	u8 i;
	u64 r_value, w_value;
	u64 r_value_byte, w_value_byte;
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	DBG_INFO("Port             Read(MB/s)  Write(MB/s)  Read(B/s)  Write(B/s)  Time: %lld.%09ld\n", (long long)ts.tv_sec, ts.tv_nsec);
	DBG_INFO("===============================================================\n");
	for (i = 0; i < MASTER_MAX_CNT; i++) {
		if (((select_ports >> i) & 0x01) == 0x01) {
			r_value = hal_nic_get_captured_data(i, CAP_READ);
			r_value = r_value << PORT_INFO[i].c_data_width;
			r_value = r_value * 10 / counter;
			r_value_byte = r_value;
			r_value = r_value >> 20;

			w_value = hal_nic_get_captured_data(i, CAP_WRITE);
			w_value = w_value << PORT_INFO[i].c_data_width;
			w_value = w_value * 10 / counter;
			w_value_byte = w_value;
			w_value = w_value >> 20;
			DBG_INFO("%-16s  %10lld  %10lld  %10lld  %10lld\n", PORT_INFO[i].c_name, r_value, w_value, r_value_byte, w_value_byte);
		}
	}
	DBG_INFO("===============================================================\n\n\n");
	hal_nic_reset_all_captured_data(0);
	mod_timer(&nic_timer, jiffies + msecs_to_jiffies(counter * 100));
}

static int __init nic_module_init(void)
{
	timer_setup(&nic_timer, nic_timer_callback, 0);
	mod_timer(&nic_timer, jiffies + msecs_to_jiffies(counter * 100));

	return 0;
}

static void __exit nic_module_exit(void)
{
	del_timer(&nic_timer);
}

static int _sp_nic_get_register_base(struct platform_device *pdev, unsigned long long *membase, const char *res_name)
{
	struct resource *r;
	void __iomem *p;

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if (!r) {
		DBG_ERR("[NIC] platform_get_resource_byname fail\n");
		return -ENODEV;
	}

	p = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(p)) {
		DBG_ERR("[NIC] ioremap fail\n");
		return PTR_ERR(p);
	}

	*membase = (unsigned long long)p;
	return NIC_SUCCESS;
}

static int _sp_nic_get_resources(struct platform_device *pdev, struct sp_nic_t *pstSpNicInfo)
{
	int ret;
	unsigned long long membase = 0;

	FUNC_DEBUG();
	ret = _sp_nic_get_register_base(pdev, &membase, NIC_MAIN_REG);
	if (ret) {
		DBG_ERR("[NIC] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpNicInfo->nic_main_regs = (void __iomem *)membase;

	ret = _sp_nic_get_register_base(pdev, &membase, NIC_PAI_REG);
	if (ret) {
		DBG_ERR("[NIC] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpNicInfo->nic_pai_regs = (void __iomem *)membase;

	ret = _sp_nic_get_register_base(pdev, &membase, NIC_PAII_REG);
	if (ret) {
		DBG_ERR("[NIC] %s (%d) ret = %d\n", __func__, __LINE__, ret);
		return ret;
	}
	pstSpNicInfo->nic_paii_regs = (void __iomem *)membase;
	return NIC_SUCCESS;
}

static int sp_nic_start(struct sp_nic_t *base)
{
	FUNC_DEBUG();
	return NIC_SUCCESS;
}

static ssize_t bw_monitor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char ret = 0;

	return ret;
}

static ssize_t bw_monitor_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	long val;
	ssize_t status;

	status = kstrtol(buf, 0, &val);
	if (status)
		return status;
	counter = val;
	if (counter == 0) {
		DBG_INFO("stop bw_monitor\n");
		nic_module_exit();
	} else {	
		DBG_INFO("set sample time %10lld ms\n", (counter * 100));
		nic_module_init();
	}
	return ret;
}

static ssize_t mon_port_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char ret = 0;

	return ret;
}

static ssize_t mon_port_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	long val;
	ssize_t status;

	status = kstrtol(buf, 0, &val);
	if (status)
		return status;

	select_ports |= (0x01 << val);
	DBG_INFO("ulSelectPorts x%x\n", select_ports);

	return ret;
}

static ssize_t bw_help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char ret = 0;
	u8 i;

	DBG_INFO("Usage:\n");
	DBG_INFO("echo <time> > bw_monitor                 : set sample time\n");
	DBG_INFO("                                           <time>  : in units of 100 ms\n");
	DBG_INFO("                                              0    : stop bw monitor\n");
	DBG_INFO("echo <hwid> > mon_port                   : monitor bandwidth of specified ports\n");
	DBG_INFO("                                           <hwid>  : specified ports\n");
	for (i = 0; i < MASTER_MAX_CNT; i++)
		DBG_INFO("                                           <%2d>    : %-16s\n", PORT_INFO[i].c_hw_id, PORT_INFO[i].c_name);

	return ret;
}

static ssize_t bw_help_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;

	return ret;
}

static DEVICE_ATTR_RW(bw_monitor);
static DEVICE_ATTR_RW(mon_port);
static DEVICE_ATTR_RW(bw_help);

static int sp_nic_platform_driver_probe(struct platform_device *pdev)
{
	int ret = -ENXIO;

	FUNC_DEBUG();
	nic_main = devm_kzalloc(&pdev->dev, sizeof(struct sp_nic_t), GFP_KERNEL);
	if (!nic_main) {
		DBG_INFO("sp_axi_t malloc fail\n");
		ret	= -ENOMEM;
		goto fail_kmalloc;
	}
	/* init */
	mutex_init(&nic_main->write_lock);
	/* register device */
	nic_main->dev.name  = "sp_nic";
	nic_main->dev.minor = MISC_DYNAMIC_MINOR;
	ret = misc_register(&nic_main->dev);
	if (ret != 0) {
		DBG_INFO("sp_nic device register fail\n");
		goto fail_regdev;
	}

	ret = _sp_nic_get_resources(pdev, nic_main);
	ret = sp_nic_start(nic_main);
	if (ret != 0) {
		DBG_ERR("[NIC] sp nic init err=%d\n", ret);
		return ret;
	}

	device_create_file(&pdev->dev, &dev_attr_bw_monitor);
	device_create_file(&pdev->dev, &dev_attr_mon_port);
	device_create_file(&pdev->dev, &dev_attr_bw_help);
fail_regdev:
	mutex_destroy(&nic_main->write_lock);
fail_kmalloc:
	return ret;
}

static int sp_nic_platform_driver_remove(struct platform_device *pdev)
{
	FUNC_DEBUG();
	reset_control_assert(sp_nic.rstc);
	return 0;
}

static int sp_nic_platform_driver_suspend(struct platform_device *pdev, pm_message_t state)
{
	FUNC_DEBUG();
	return 0;
}

static int sp_nic_platform_driver_resume(struct platform_device *pdev)
{
	FUNC_DEBUG();
	return 0;
}

static const struct of_device_id sp_nic_of_match[] = {
	{ .compatible = "sunplus,sp7350-nic", },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, sp_nic_of_match);

static struct platform_driver sp_nic_platform_driver = {
	.probe		= sp_nic_platform_driver_probe,
	.remove		= sp_nic_platform_driver_remove,
	.suspend	= sp_nic_platform_driver_suspend,
	.resume		= sp_nic_platform_driver_resume,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_nic_of_match),
	}
};

module_platform_driver(sp_nic_platform_driver);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus NIC Driver");
MODULE_LICENSE("GPL");
