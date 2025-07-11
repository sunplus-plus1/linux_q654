/* Copyright (C)  Sunplus, Inc - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include <linux/clk.h>
#include <linux/hashtable.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_PM
#include <linux/pm_runtime.h>
#endif
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/clocksource.h>
#include <linux/cdev.h>
#include <linux/of_address.h>
#include "include/sunplus_tsp_reg.h"
#include "include/timer.h"

#define TSP_USE_CLK
#define TSP_USE_RESET
//#define TSP_USE_REGULATOR

#define tsp_dbg(d, fmt, arg...)         dev_dbg(d.pdev_dev, fmt, ##arg)
#define tsp_info(d, fmt, arg...)        dev_info(d.pdev_dev, fmt, ##arg)
#define tsp_warn(d, fmt, arg...)        dev_warn(d.pdev_dev, fmt, ##arg)
#define tsp_err(d, fmt, arg...)         dev_err(d.pdev_dev, fmt, ##arg)

#define TSP_NAME "sunplus_timer"
#define MAX_TSP_DEVICE 10
#define TSP_PM_TIMER (60 * 1000)

struct tsp_node {
	int index;
	u32 *timestamp_buffers;
	dma_addr_t tsp_phys;
	struct hlist_node node;
};

#define MAX_HLIST_TBL 16
struct tsp {
	int id;
	u32 num_buffers;
	struct tsp_node addr_tbl[MAX_HLIST_TBL];
	struct hlist_head tsphlist;
};

struct tsp_request {
	int id;
	uint64_t tsp;
	uint32_t tsp_buffers_num;
	uint64_t tsp_buffers[64];
};

#define TSP_IOC_MAGIC 'T'
#define TSP_GET_TSP _IOWR(TSP_IOC_MAGIC, 0, struct tsp_request)
#define TSP_GET_TSP_FRAME _IOWR(TSP_IOC_MAGIC, 1, struct tsp_request)	

struct sunplus_timer {
	int dev_major;
	struct device *pdev_dev;
	struct class *class;
	struct cdev cdev;
	struct timer_list tsp_timer;
	struct resource r;
#ifdef TSP_USE_CLK
	struct clk *clk_gate;		// VCL4
	struct clk *vcl_clk_gate;	// VCL
	struct clk *vcl5_clk_gate;	// VCL5
#endif
#ifdef TSP_USE_RESET
	struct reset_control *vcl_rst;
	struct reset_control *timer_rst;
	struct reset_control *vcl5_rst;
#endif
#ifdef TSP_USE_REGULATOR
	struct regulator *regulator;
#endif
	dev_t devno;
	spinlock_t timer_lock;
	u32 callback_timer;
	u32 tsp_ctrl0_val;
	s32 mult;
	s32 shift;
	u64 tsp_time_s;
	u64 raw_time_s;
	bool clk_funcEn;
	bool clk_lock;
	bool in_suspend;
	bool disable_gpi_timer;
	DECLARE_BITMAP(enabled_dev, 16);
};

static struct sunplus_timer timer;
static void __iomem *timer_base;
static void __iomem *sram_addr[MAX_TSP_DEVICE];
static u32 sram_phys[MAX_TSP_DEVICE];
static struct device *dev;

static struct tsp *tsp_device[TSP_ID_MAX];
static u32 tsp_freq = 0;

u32 get_tsp_buffer_idx(struct hlist_head *h, u32 num_buffers, dma_addr_t phys);

#ifdef TSP_USE_CLK
inline void sunplus_timer_clk_gating(struct clk *timer_clk, bool isGating)
{
	if (timer_clk) {
		if (!timer.clk_funcEn) return;

		if (isGating) {
			clk_disable(timer_clk);
		} else {
			clk_enable(timer_clk);
		}
	}
}
#endif

static inline u32 tsp_global_count(void)
{
	return readl(timer_base + SUNPLUS_TSP_GLB_0);
}

ssize_t timer_read(struct file *file, char __user *buf, size_t len,
		   loff_t *offset)
{
	u32 val;

	val = tsp_global_count();
	put_user(val, (u32 *)buf);

	return 0;
}

ssize_t timer_write(struct file *file, const char __user *buf, size_t len,
		    loff_t *offset)
{
	return 0;
}

static u64 timer_get_tsp(int id)
{
	struct tsp *tsp;
	u32 addr, offset;
	u64 timestamp;
	int idx;

	if (id >= TSP_ID_MAX)
		return 0;

	tsp = tsp_device[id];
	if (tsp == NULL) {
		return 0;
	}

	if (id >= TSP_MIPI0 && id <= TSP_MIPI5 ) {
		offset = SUNPLUS_VSYNC0_STATUS_0;
		offset += id * 60;
		addr = readl(timer_base + offset);
	} else {
		offset = SUNPLUS_IMU0_STATUS_0;
		offset += (id - TSP_IMU0) * 20;
		addr = readl(timer_base + offset);
	}

	idx = get_tsp_buffer_idx(&tsp_device[id]->tsphlist, tsp_device[id]->num_buffers, addr);
	timestamp = tsp_device[id]->addr_tbl[idx].timestamp_buffers[0];

	return timestamp;
}

struct timecounter sunplus_tc;
struct timecounter sunplus_mipi0_tc;
struct timecounter sunplus_mipi1_tc;
struct timecounter sunplus_mipi2_tc;
struct timecounter sunplus_mipi3_tc;
struct timecounter sunplus_mipi4_tc;
struct timecounter sunplus_mipi5_tc;
struct timecounter sunplus_imu0_tc;
struct timecounter sunplus_imu1_tc;
struct timecounter sunplus_imu2_tc;
struct timecounter sunplus_imu3_tc;

static void sunplus_nsec_update(struct timecounter *tc, u64 cycle, u64 mask)
{
	u64 cycle_delta;
	u64 ns_offset;

	cycle_delta = (cycle - tc->cycle_last) & mask;
	ns_offset = cyclecounter_cyc2ns(tc->cc, cycle_delta, tc->mask, &tc->frac);
	tc->nsec += ns_offset;
}

static void sunplus_mipi_cycle_update(u64 cycle)
{
	sunplus_tc.cycle_last = sunplus_mipi0_tc.cycle_last = sunplus_mipi1_tc.cycle_last = \
	sunplus_mipi2_tc.cycle_last = sunplus_mipi3_tc.cycle_last = sunplus_mipi4_tc.cycle_last = \
	sunplus_mipi5_tc.cycle_last = cycle;
}

static void sunplus_imu_cycle_update(u64 cycle)
{
        sunplus_imu0_tc.cycle_last = sunplus_imu1_tc.cycle_last = \
        sunplus_imu2_tc.cycle_last = sunplus_imu3_tc.cycle_last = cycle;
}

static void sunplus_timecounter_update(struct timecounter *tc, bool skip_imu_update)
{
	u64 cycle_now;
	u64 mask;

	mask = tc->cc->mask;
	cycle_now = tsp_global_count();

	sunplus_nsec_update(&sunplus_tc, cycle_now, mask);
	sunplus_nsec_update(&sunplus_mipi0_tc, cycle_now, mask);
	sunplus_nsec_update(&sunplus_mipi1_tc, cycle_now, mask);
	sunplus_nsec_update(&sunplus_mipi2_tc, cycle_now, mask);
	sunplus_nsec_update(&sunplus_mipi3_tc, cycle_now, mask);
	sunplus_nsec_update(&sunplus_mipi4_tc, cycle_now, mask);
	sunplus_nsec_update(&sunplus_mipi5_tc, cycle_now, mask);
	sunplus_mipi_cycle_update(cycle_now);

	if (!skip_imu_update) {
	    sunplus_nsec_update(&sunplus_imu0_tc, cycle_now, mask);
	    sunplus_nsec_update(&sunplus_imu1_tc, cycle_now, mask);
	    sunplus_nsec_update(&sunplus_imu2_tc, cycle_now, mask);
	    sunplus_nsec_update(&sunplus_imu3_tc, cycle_now, mask);
	    sunplus_imu_cycle_update(cycle_now);
	}
}

static void sunplus_tsp_timer_callback(struct timer_list *data)
{
	struct sunplus_timer *tsp_pdev = from_timer(tsp_pdev, data, tsp_timer);
	unsigned long flags;

	spin_lock_irqsave(&tsp_pdev->timer_lock, flags);

	sunplus_timecounter_update(&sunplus_tc, timer.disable_gpi_timer);

	spin_unlock_irqrestore(&tsp_pdev->timer_lock, flags);
	mod_timer(&tsp_pdev->tsp_timer, jiffies + msecs_to_jiffies(tsp_pdev->callback_timer));
}

static void sunplus_sync_system_time(void)
{
	timer.raw_time_s = ktime_get_raw_ns();
	timer.tsp_time_s = timecounter_read(&sunplus_tc);
}

u64 sunplus_tsp_read(int id)
{
	unsigned long flags;
	u64 timestamp = 0;

	spin_lock_irqsave(&timer.timer_lock, flags);

	if (timer.in_suspend) {
		pr_err("%s, tsp not work now\n", __func__);
		timestamp = 0;
		goto sunplus_tsp_read_exit;
	}

	switch (id) {
	case TSP_MIPI0:
		timestamp = timecounter_read(&sunplus_mipi0_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "MIPI%d realtime:%lld\n", id, timestamp);
		break;
	case TSP_MIPI1:
		timestamp = timecounter_read(&sunplus_mipi1_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "MIPI%d realtime:%lld\n", id, timestamp);
		break;
	case TSP_MIPI2:
		timestamp = timecounter_read(&sunplus_mipi2_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "MIPI%d realtime:%lld\n", id, timestamp);
		break;
	case TSP_MIPI3:
		timestamp = timecounter_read(&sunplus_mipi3_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "MIPI%d realtime:%lld\n", id, timestamp);
		break;
	case TSP_MIPI4:
		timestamp = timecounter_read(&sunplus_mipi4_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "MIPI%d realtime:%lld\n", id, timestamp);
		break;
	case TSP_MIPI5:
		timestamp = timecounter_read(&sunplus_mipi5_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "MIPI%d realtime:%lld\n", id, timestamp);
		break;
	case TSP_IMU0:
		timestamp = timecounter_read(&sunplus_imu0_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "IMU%d realtime:%lld\n", id - TSP_IMU0, timestamp);
		break;
	case TSP_IMU1:
		timestamp = timecounter_read(&sunplus_imu1_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "IMU%d realtime:%lld\n", id - TSP_IMU0, timestamp);
		break;
	case TSP_IMU2:
		timestamp = timecounter_read(&sunplus_imu2_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "IMU%d realtime:%lld\n", id - TSP_IMU0, timestamp);
		break;
	case TSP_IMU3:
		timestamp = timecounter_read(&sunplus_imu3_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
                tsp_dbg(timer, "IMU%d realtime:%lld\n", id - TSP_IMU0, timestamp);
		break;
	case TSP_GLOBAL:
		timestamp = timecounter_read(&sunplus_tc);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
		tsp_dbg(timer, "GLOBAL realtime:%lld\n", timestamp);
		break;
	default:
		break;
	}

sunplus_tsp_read_exit:
	spin_unlock_irqrestore(&timer.timer_lock, flags);

	return timestamp;
}
EXPORT_SYMBOL(sunplus_tsp_read);


static u64 timecounter_read_delta_ext(struct timecounter *tc, u64 cycle)
{
	u64 cycle_now, cycle_delta;
	u64 ns_offset;

	/* read cycle counter: */
	cycle_now = cycle;

	/* calculate the delta since the last timecounter_read_delta(): */
	cycle_delta = (cycle_now - tc->cycle_last) & tc->cc->mask;

	/* convert to nanoseconds: */
	ns_offset = cyclecounter_cyc2ns(tc->cc, cycle_delta,
					tc->mask, &tc->frac);
	if (cycle_now <= tc->cycle_last) {
		tsp_err(timer, "error %s cycle_now:%llx, tc->cycle_last:%llx, cycle_delta:%llx, tc->cc->mask:%llx, ns_offset:%lld\n", 
			__FUNCTION__, cycle_now, tc->cycle_last, cycle_delta, tc->cc->mask, ns_offset);
	}
	/* update time stamp of timecounter_read_delta() call: */
	tc->cycle_last = cycle_now;

	return ns_offset;
}

u64 timecounter_read_ext(struct timecounter *tc, u64 cycle)
{
	u64 nsec;

	/* increment time by nanoseconds since last call */
	nsec = timecounter_read_delta_ext(tc, cycle);
	nsec += tc->nsec;
	tc->nsec = nsec;

	return nsec;
}

u64 tsp_to_realtime(int id, u64 cycle)
{
	unsigned long flags;
	u64 timestamp = 0;

	spin_lock_irqsave(&timer.timer_lock, flags);

	switch (id) {
	case TSP_IMU0:
		timestamp = timecounter_read_ext(&sunplus_imu0_tc, cycle);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
		tsp_dbg(timer, "IMU%d cyc:%lld realtime:%lld\n", id - TSP_IMU0, cycle, timestamp);
		break;
	case TSP_IMU1:
			timestamp = timecounter_read_ext(&sunplus_imu1_tc, cycle);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
		tsp_dbg(timer, "IMU%d cyc:%lld realtime:%lld\n", id - TSP_IMU0, cycle, timestamp);
			break;
	case TSP_IMU2:
			timestamp = timecounter_read_ext(&sunplus_imu2_tc, cycle);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
		tsp_dbg(timer, "IMU%d cyc:%lld realtime:%lld\n", id - TSP_IMU0, cycle, timestamp);
			break;
	case TSP_IMU3:
			timestamp = timecounter_read_ext(&sunplus_imu3_tc, cycle);
		timestamp = (timestamp - timer.tsp_time_s) + timer.raw_time_s;
		tsp_dbg(timer, "IMU%d cyc:%lld realtime:%lld\n", id - TSP_IMU0, cycle, timestamp);
			break;
	}

	spin_unlock_irqrestore(&timer.timer_lock, flags);

	return timestamp;
}
EXPORT_SYMBOL(tsp_to_realtime);

static ssize_t sunplus_tsp_debugfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int id = TSP_MIPI0;
	u64 real_time;
	u64 monotonic_raw_time;
	u64 tsp_time;

	real_time = ktime_get_real_ns();
	monotonic_raw_time = ktime_get_raw_ns();

	for (id = TSP_MIPI0; id <= TSP_MIPI5; ++id ) {
		tsp_time = sunplus_tsp_read(id);
		tsp_info(timer, "MIPI%d tsp_time:%lld (ns)\n", id, tsp_time);
		tsp_info(timer, "MIPI%d real_time:%lld (ns)\n", id, real_time - (monotonic_raw_time - tsp_time));
	}

	for (id = TSP_IMU0; id <= TSP_IMU3; ++id ) {
		tsp_time = sunplus_tsp_read(id);
                tsp_info(timer, "GPI%d tsp_time:%lld (ns)\n", (id - TSP_IMU0), sunplus_tsp_read(id));
		tsp_info(timer, "GPI%d real_time:%lld (ns)\n", (id - TSP_IMU0), real_time - (monotonic_raw_time - tsp_time));
        }

	tsp_time = sunplus_tsp_read(TSP_GLOBAL);
	tsp_info(timer, "GLOBAL tsp_time:%lld (ns)\n", tsp_time);
	tsp_info(timer, "GLOBAL real_time:%lld (ns)\n", real_time - (monotonic_raw_time - tsp_time));

	return 0;
}
static DEVICE_ATTR_RO(sunplus_tsp_debugfs);

static ssize_t clk_funcEn_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "clk_funcEn = %d\n", timer.clk_funcEn);
}

static ssize_t clk_funcEn_store(struct device *device, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	unsigned int value;
	unsigned long flags;

	value = simple_strtoul(buf, NULL, 0);
	if (value != 0 && value != 1)
		return -EINVAL;

	spin_lock_irqsave(&timer.timer_lock, flags);
	timer.clk_funcEn = value;
	spin_unlock_irqrestore(&timer.timer_lock, flags);

	return count;
}
static DEVICE_ATTR_RW(clk_funcEn);

static ssize_t clk_lock_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "clk_lock = %d\n", timer.clk_lock);
}

static ssize_t clk_lock_store(struct device *device, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	unsigned int value;
	unsigned long flags;

	value = simple_strtoul(buf, NULL, 0);
	if (value != 0 && value != 1)
		return -EINVAL;

	spin_lock_irqsave(&timer.timer_lock, flags);
	timer.clk_lock = value;

#ifdef TSP_USE_CLK
	sunplus_timer_clk_gating(timer.clk_gate, timer.clk_lock);
#endif

	spin_unlock_irqrestore(&timer.timer_lock, flags);

	return count;
}
static DEVICE_ATTR_RW(clk_lock);

int sunplus_tsp_sysfs_files(struct device *dev)
{
	int error;

	error = device_create_file(dev, &dev_attr_sunplus_tsp_debugfs);
	if (error)
		goto out;

	error = device_create_file(dev, &dev_attr_clk_funcEn);
	if (error)
		goto out;

	error = device_create_file(dev, &dev_attr_clk_lock);
	if (error)
		goto out;

	return 0;

out:
	return error;
}

static long timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tsp_request req;
	int index;

	if (copy_from_user(&req, (void __user *)arg, _IOC_SIZE(cmd)))
		return -EFAULT;

	switch (cmd) {
	case TSP_GET_TSP: {
		index = req.id;
		req.tsp = sunplus_tsp_read(index);
	} break;
	default:
		break;
	}

	if (_IOC_DIR(cmd) & _IOC_READ) {
		if (copy_to_user((void __user *)arg, &req, _IOC_SIZE(cmd)))
			return -EFAULT;
	}

	return 0;
}

u64 tsp_cycle_counter(int id)
{
	u64 cycle_now = timer_get_tsp(id);
	u64 cycle_global = tsp_global_count();
	u64 cycle_last = 0;
	u64 cycle_delta = 0;

	switch (id) {
	case TSP_MIPI0:
		cycle_last = sunplus_mipi0_tc.cycle_last;
		tsp_dbg(timer, "MIPI%d cyc_now:%lld cyc_last:%lld global:%lld\n", id, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_MIPI1:
		cycle_last = sunplus_mipi1_tc.cycle_last;
		tsp_dbg(timer, "MIPI%d cyc_now:%lld cyc_last:%lld global:%lld\n", id, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_MIPI2:
		cycle_last = sunplus_mipi2_tc.cycle_last;
		tsp_dbg(timer, "MIPI%d cyc_now:%lld cyc_last:%lld global:%lld\n", id, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_MIPI3:
		cycle_last = sunplus_mipi3_tc.cycle_last;
		tsp_dbg(timer, "MIPI%d cyc_now:%lld cyc_last:%lld global:%lld\n", id, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_MIPI4:
		cycle_last = sunplus_mipi4_tc.cycle_last;
		tsp_dbg(timer, "MIPI%d cyc_now:%lld cyc_last:%lld global:%lld\n", id, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_MIPI5:
		cycle_last = sunplus_mipi5_tc.cycle_last;
		tsp_dbg(timer, "MIPI%d cyc_now:%lld cyc_last:%lld global:%lld\n", id, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_IMU0:
		cycle_last = sunplus_imu0_tc.cycle_last;
		tsp_dbg(timer, "IMU%d cyc_now:%lld cyc_last:%lld global:%lld\n", id - TSP_IMU0, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_IMU1:
		cycle_last = sunplus_imu1_tc.cycle_last;
		tsp_dbg(timer, "IMU%d cyc_now:%lld cyc_last:%lld global:%lld\n", id - TSP_IMU0, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_IMU2:
		cycle_last = sunplus_imu2_tc.cycle_last;
		tsp_dbg(timer, "IMU%d cyc_now:%lld cyc_last:%lld global:%lld\n", id - TSP_IMU0, cycle_now, cycle_last, cycle_global);
		break;
	case TSP_IMU3:
		cycle_last = sunplus_imu3_tc.cycle_last;
		tsp_dbg(timer, "IMU%d cyc_now:%lld cyc_last:%lld global:%lld\n", id - TSP_IMU0, cycle_now, cycle_last, cycle_global);
		break;
	}

	if (cycle_now <= cycle_last){
		cycle_delta = cycle_global;
	} else {
		if (cycle_now <= cycle_global) {
			cycle_delta = cycle_now;
		} else {
			cycle_delta = cycle_global;
		}
	}

	return cycle_delta;
}

static const struct file_operations sunplus_timer_fops = {
	.owner = THIS_MODULE,
	.read = timer_read,
	.write = timer_write,
	.unlocked_ioctl = timer_ioctl,
};

static void timer_set_imu_addr(int index, dma_addr_t addr, u32 num_buffers)
{
	u32 offset = SUNPLUS_IMU0_CTRL0 + (index * 20);

	writel(num_buffers, timer_base + offset);
	offset += 4;
	writel(addr, timer_base + offset);
}

static void timer_set_mipi_buffernum(int index, u32 num_buffers)
{
	u32 offset = SUNPLUS_VSYNC0_CTRL0 + (index * 60);

	writel(num_buffers, timer_base + offset);
}

static void timer_set_mipi_addr(int index, int addr_n, dma_addr_t addr)
{
	u32 offset = SUNPLUS_VSYNC0_ADDR1_LO + (index * 60) + (addr_n * 8);

	writel(addr, timer_base + offset);
}

u32 get_tsp_buffer_idx(struct hlist_head *h, u32 num_buffers, dma_addr_t phys)
{
	struct tsp_node* pos;
	int idx;

	hlist_for_each_entry(pos, h, node) {
		if (pos->tsp_phys == phys) {
			idx = (pos->index == 0) ? (num_buffers - 1) : (pos->index - 1) % num_buffers;
			return idx;
		}
	}

	return 0;
}

static int timer_mem_address(int index)
{
	int i = 0;

	if (index >= TSP_MIPI0 && index <= TSP_MIPI5 ) {
		timer_set_mipi_buffernum(index, tsp_device[index]->num_buffers);
		for (i = 0; i < tsp_device[index]->num_buffers ; ++i)
			timer_set_mipi_addr(index, i, tsp_device[index]->addr_tbl[i].tsp_phys);
	} else
		timer_set_imu_addr(index - TSP_IMU0, tsp_device[index]->addr_tbl[0].tsp_phys, tsp_device[index]->num_buffers);

	return 0;
}

static int timer_allocate_buffer(int index, u32 num_buffers)
{
	struct tsp *t;
	int i;

	t = kzalloc(sizeof(struct tsp), GFP_KERNEL);
	if (!t)
		return -ENOMEM;

	t->num_buffers = num_buffers;
	INIT_HLIST_HEAD(&t->tsphlist);

	for (i = 0; i < num_buffers ; i++) {
		t->addr_tbl[i].index = i;
		t->addr_tbl[i].tsp_phys = sram_phys[index] + (i * 16);
		t->addr_tbl[i].timestamp_buffers = sram_addr[index] + (i * 16);
		hlist_add_head(&t->addr_tbl[i].node, &t->tsphlist);
	}

	tsp_device[index] = t;
	timer_mem_address(index);

	return 0;
}

static int timer_enable(int index, u32 bit, int num_buffers)
{
	u32 v;
	int r;

	r = timer_allocate_buffer(index, num_buffers);
	if (r)
		return r;

	v = readl(timer_base + SUNPLUS_TSP_CTRL_0);
	v |= (1 << bit);
	writel(v, timer_base + SUNPLUS_TSP_CTRL_0);

	bitmap_set(timer.enabled_dev, index, 1);

	return 0;
}

static u64 sunplus_cc_read(const struct cyclecounter *cc)
{
	return tsp_global_count();
}

static struct cyclecounter sunplus_cc = {
	.read = sunplus_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_mipi0_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_MIPI0, timer.enabled_dev))
		return tsp_cycle_counter(TSP_MIPI0);

	return 0;
}

static struct cyclecounter sunplus_mipi0_cc = {
	.read = sunplus_mipi0_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_mipi1_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_MIPI1, timer.enabled_dev))
		return tsp_cycle_counter(TSP_MIPI1);

	return 0;
}

static struct cyclecounter sunplus_mipi1_cc = {
	.read = sunplus_mipi1_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_mipi2_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_MIPI2, timer.enabled_dev))
		return tsp_cycle_counter(TSP_MIPI2);

	return 0;
}

static struct cyclecounter sunplus_mipi2_cc = {
	.read = sunplus_mipi2_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_mipi3_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_MIPI3, timer.enabled_dev))
		return tsp_cycle_counter(TSP_MIPI3);

	return 0;
}

static struct cyclecounter sunplus_mipi3_cc = {
	.read = sunplus_mipi3_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_mipi4_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_MIPI4, timer.enabled_dev))
		return tsp_cycle_counter(TSP_MIPI4);

	return 0;
}

static struct cyclecounter sunplus_mipi4_cc = {
	.read = sunplus_mipi4_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_mipi5_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_MIPI5, timer.enabled_dev))
		return tsp_cycle_counter(TSP_MIPI5);

	return 0;
}

static struct cyclecounter sunplus_mipi5_cc = {
	.read = sunplus_mipi5_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_imu0_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_IMU0, timer.enabled_dev))
		return tsp_cycle_counter(TSP_IMU0);

	return 0;
}

static struct cyclecounter sunplus_imu0_cc = {
	.read = sunplus_imu0_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_imu1_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_IMU1, timer.enabled_dev))
		return tsp_cycle_counter(TSP_IMU1);

	return 0;
}

static struct cyclecounter sunplus_imu1_cc = {
	.read = sunplus_imu1_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_imu2_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_IMU2, timer.enabled_dev))
		return tsp_cycle_counter(TSP_IMU2);

	return 0;
}

static struct cyclecounter sunplus_imu2_cc = {
	.read = sunplus_imu2_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static u64 sunplus_imu3_cc_read(const struct cyclecounter *cc)
{
	if (test_bit(TSP_IMU3, timer.enabled_dev))
		return tsp_cycle_counter(TSP_IMU3);

	return 0;
}

static struct cyclecounter sunplus_imu3_cc = {
	.read = sunplus_imu3_cc_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 8,
};

static int timer_init(struct device_node *np_dev)
{
	struct device_node *np;
	const __be32 *prop;
	u32 v, num_buffers;
	int i, num_tsp_device;
	int index, len, tsp_timer, ret = -EINVAL;
	int mult, shift;
	int j;

	spin_lock_init(&timer.timer_lock);

#ifdef TSP_USE_CLK
	timer.vcl_clk_gate = of_clk_get_by_name(np_dev, "vcl_clk");
	if (IS_ERR(timer.vcl_clk_gate)) {
		tsp_err(timer, "%s Failed to get vcl clock gate\n", __func__);
		timer.vcl_clk_gate = NULL;
	} else {
		ret = clk_prepare_enable(timer.vcl_clk_gate);
		if (ret)
			tsp_err(timer , "%s prepare vcl clock failed\n", __func__);
		else
			tsp_dbg(timer , "%s prepare vcl clock \n", __func__);
	}

	timer.vcl5_clk_gate = of_clk_get_by_name(np_dev, "vcl5_clk");
	if (IS_ERR(timer.vcl5_clk_gate)) {
		tsp_err(timer , "%s Failed to get vcl5 clock gate\n", __func__);
		timer.vcl5_clk_gate = NULL;
	} else {
		ret = clk_prepare_enable(timer.vcl5_clk_gate);
		if (ret)
			tsp_err(timer , "%s prepare vcl5 clock failed\n", __func__);
		else
			tsp_dbg(timer , "%s prepare vcl5 clock \n", __func__);
	}

	timer.clk_gate = of_clk_get_by_name(np_dev, "tsp_clk");
	if (IS_ERR(timer.clk_gate)) {
		tsp_err(timer , "%s Failed to get clock gate\n", __func__);
		timer.clk_gate = NULL;
	} else {
		ret = clk_prepare_enable(timer.clk_gate);
		if (ret)
			tsp_err(timer , "%s prepare clock failed\n", __func__);
		else
			tsp_dbg(timer , "%s prepare clock \n", __func__);
	}
#endif

	np = of_parse_phandle(np_dev, "memory-region", 0);
	if (!np) {
		tsp_err(timer, "No %s specified\n", "memory-region");
		goto error;
	}

	ret = of_address_to_resource(np, 0, &timer.r);
	if (ret) {
		tsp_err(timer, "No memory address assigned to the region\n");
		goto error;
	}

	if (!request_mem_region(timer.r.start, resource_size(&timer.r), np->full_name)) {
		ret = -ENOMEM;
		of_node_put(np);
		goto error;
	}

	timer_base = ioremap(timer.r.start, resource_size(&timer.r));
	WARN_ON(!timer_base);

	prop = of_get_property(np_dev, "num_tsp_device", &len);
	if (!prop) {
		ret = -EINVAL;
		goto error;
	}

	num_tsp_device = be32_to_cpup(prop);
	for (i = 0; i < num_tsp_device; i++) {
		np = of_parse_phandle(np_dev, "tsp_device", i);
		if (!np) {
			tsp_err(timer, "tsp_device incorrectly detected");
			goto error;
		}

		tsp_dbg(timer, "Node: %s\n", np->name);
		of_property_read_u32(np, "id", &index);
		tsp_dbg(timer, "index: %d\n", index);
		of_property_read_u32(np, "tsp_ctrl_0", &v);
		tsp_dbg(timer, "tsp_ctrl_0: %d\n", v);
		of_property_read_u32(np, "num_buffers", &num_buffers);
		tsp_dbg(timer, "num_buffers: %d\n", num_buffers);
		of_property_read_u32(np, "sram_phy", &sram_phys[i]);

		sram_addr[i] = ioremap(sram_phys[i], sizeof(u32) * 4 * num_buffers);
		
		for (j = 0; j < num_buffers; ++j ) {
			writel(0, sram_addr[i] + (j * 16));
		}

		timer_enable(index, v, num_buffers);
	}

	prop = of_get_property(np_dev, "tsp_freq", &len);
	if (!prop) {
		ret = -EINVAL;
		goto error;
	}

	tsp_freq = be32_to_cpup(prop);

	prop = of_get_property(np_dev, "tsp_timer", &tsp_timer);
	if (!prop) {
		ret = -EINVAL;
		goto error;
	}
	timer.callback_timer = be32_to_cpup(prop);

	if (of_property_read_bool(np_dev, "disable_gpi_timer")) {
	    timer.disable_gpi_timer = true;
	} else {
            timer.disable_gpi_timer = false;
	}

	timer.mult = timer.shift = (-1);
	prop = of_get_property(np_dev, "tsp_mult", &mult);
	if (!prop) {
		ret = -EINVAL;
		goto error;
	}
	timer.mult = be32_to_cpup(prop);

	prop = of_get_property(np_dev, "tsp_shift", &shift);
	if (!prop) {
		ret = -EINVAL;
		goto error;
	}
	timer.shift = be32_to_cpup(prop);

#if 0
#ifdef TSP_USE_CLK
	timer.vcl_clk_gate = of_clk_get_by_name(np_dev, "vcl_clk");
	if (IS_ERR(timer.vcl_clk_gate)) {
		tsp_err(timer , "%s Failed to get vcl clock gate\n", __func__);
		timer.vcl_clk_gate = NULL;
	} else {
		ret = clk_prepare_enable(timer.vcl_clk_gate);
		if (ret)
			tsp_err(timer , "%s prepare vcl clock failed\n", __func__);
		else
			tsp_dbg(timer , "%s prepare vcl clock \n", __func__);
	}

	timer.vcl5_clk_gate = of_clk_get_by_name(np_dev, "vcl5_clk");
	if (IS_ERR(timer.vcl5_clk_gate)) {
		tsp_err(timer, "%s Failed to get vcl5 clock gate\n", __func__);
		timer.vcl5_clk_gate = NULL;
	} else {
		ret = clk_prepare_enable(timer.vcl5_clk_gate);
		if (ret)
			tsp_err(timer , "%s prepare vcl5 clock failed\n", __func__);
		else
			tsp_dbg(timer , "%s prepare vcl5 clock \n", __func__);
	}

	timer.clk_gate = of_clk_get_by_name(np_dev, "tsp_clk");
	if (IS_ERR(timer.clk_gate)) {
		tsp_err(timer, "%s Failed to get clock gate\n", __func__);
		timer.clk_gate = NULL;
	} else {
		ret = clk_prepare_enable(timer.clk_gate);
		if (ret)
			tsp_err(timer , "%s prepare clock failed\n", __func__);
		else
			tsp_dbg(timer , "%s prepare clock \n", __func__);
	}
#endif
#endif

#ifdef TSP_USE_RESET
	timer.vcl_rst = of_reset_control_get_shared(np_dev, "vcl_reset");
	if (IS_ERR(timer.vcl_rst)){
		tsp_err(timer, "%s, vcl_reset get of_reset_control_get_shared() fail\n", __func__);
		timer.vcl_rst = NULL;
	} else
		reset_control_deassert(timer.vcl_rst);

	timer.vcl5_rst = of_reset_control_get_shared(np_dev, "vcl5_reset");
	if (IS_ERR(timer.vcl5_rst)){
		tsp_err(timer, "%s, vcl5_reset get of_reset_control_get_shared() fail\n", __func__);
		timer.vcl5_rst = NULL;
	} else
		reset_control_deassert(timer.vcl5_rst);

	timer.timer_rst = of_reset_control_get_shared(np_dev, "timer_reset");
	if (IS_ERR(timer.timer_rst)){
		tsp_err(timer, "%s, timer_reset get of_reset_control_get_shared() fail\n", __func__);
		timer.timer_rst = NULL;
	} else
		reset_control_deassert(timer.timer_rst);
#endif

	return 0;
error:
	return ret;
}

static int sunplus_timer_cdev(void)
{
	int ret;

	ret = alloc_chrdev_region(&timer.devno, 0, 1, TSP_NAME);
	if (ret)
		goto out;

	timer.class = class_create(TSP_NAME);
	timer.dev_major = MAJOR(timer.devno);

	cdev_init(&timer.cdev, &sunplus_timer_fops);
	ret = cdev_add(&timer.cdev, timer.devno, 1);
	if (ret)
		goto out;

	dev = device_create(timer.class, NULL, timer.devno, NULL, TSP_NAME);
	if (IS_ERR(dev)) {
		return PTR_ERR(dev);
	}

	ret = sunplus_tsp_sysfs_files(dev);
	if (ret)
		goto out;

	dev_set_drvdata(dev, &timer);

	return 0;
out:
	return ret;
}

static int timer_member_init(void)
{
	u32 mult, shift;

	/* determine tsp freq */
	clocks_calc_mult_shift(&mult, &shift, tsp_freq, NSEC_PER_SEC, 0);

	if (timer.mult != (-1) && timer.shift != (-1)) {
		mult = timer.mult;
		shift = timer.shift;
	}

	sunplus_cc.mult = sunplus_mipi0_cc.mult = sunplus_mipi1_cc.mult =
		sunplus_mipi2_cc.mult = sunplus_mipi3_cc.mult =
		sunplus_mipi4_cc.mult = sunplus_mipi5_cc.mult =
		sunplus_imu0_cc.mult = sunplus_imu1_cc.mult =
		sunplus_imu2_cc.mult = sunplus_imu3_cc.mult = mult;
	sunplus_cc.shift = sunplus_mipi0_cc.shift = sunplus_mipi1_cc.shift =
		sunplus_mipi2_cc.shift = sunplus_mipi3_cc.shift =
		sunplus_mipi4_cc.shift = sunplus_mipi5_cc.shift =
		sunplus_imu0_cc.shift = sunplus_imu1_cc.shift =
		sunplus_imu2_cc.shift = sunplus_imu3_cc.shift = shift;

	/* pass 0 since tsp is reset */
	timecounter_init(&sunplus_tc, &sunplus_cc, 0);
	timecounter_init(&sunplus_mipi0_tc, &sunplus_mipi0_cc, 0);
	timecounter_init(&sunplus_mipi1_tc, &sunplus_mipi1_cc, 0);
	timecounter_init(&sunplus_mipi2_tc, &sunplus_mipi2_cc, 0);
	timecounter_init(&sunplus_mipi3_tc, &sunplus_mipi3_cc, 0);
	timecounter_init(&sunplus_mipi4_tc, &sunplus_mipi4_cc, 0);
	timecounter_init(&sunplus_mipi5_tc, &sunplus_mipi5_cc, 0);
	timecounter_init(&sunplus_imu0_tc, &sunplus_imu0_cc, 0);
	timecounter_init(&sunplus_imu1_tc, &sunplus_imu1_cc, 0);
	timecounter_init(&sunplus_imu2_tc, &sunplus_imu2_cc, 0);
	timecounter_init(&sunplus_imu3_tc, &sunplus_imu3_cc, 0);

	/* get system time as reference time */
	sunplus_sync_system_time();
	timer.tsp_ctrl0_val  = readl(timer_base + SUNPLUS_TSP_CTRL_0);

	timer.clk_funcEn = true; /* sysfs debug */
	timer.clk_lock = false;

	return 0;
}

static int sunplus_timer_probe(struct platform_device *pdev)
{
	struct device_node *np_dev = pdev->dev.of_node;
	int ret = -EINVAL;
	//u32 mult, shift;

	timer.pdev_dev = &pdev->dev;

	ret = timer_init(np_dev);
	if (ret)
		goto error;

	ret = sunplus_timer_cdev();
	if (ret)
		goto error;

#ifdef TSP_USE_REGULATOR
	timer.regulator = devm_regulator_get_optional(&pdev->dev, "ext_buck1_0v8");
	if (IS_ERR(timer.regulator)) {
		tsp_err(timer.pdev_dev, "%s, regulator_get() fail\n", __func__);
		timer.regulator = NULL;
	} else
		regulator_enable(timer.regulator);
#endif

	/* tsp_sw_rst */
	writel(1, timer_base + SUNPLUS_TSP_CTRL_1);

	/* tsp timer init */
	timer_setup(&timer.tsp_timer, sunplus_tsp_timer_callback, 0);
	mod_timer(&timer.tsp_timer, jiffies + msecs_to_jiffies(timer.callback_timer));

	timer_member_init();
	timer.in_suspend = false;
	tsp_info(timer, "%s, done\n", __func__);

	return 0;
error:
	return ret;
}

static int sunplus_timer_remove(struct platform_device *pdev)
{
	del_timer(&timer.tsp_timer);

#ifdef TSP_USE_REGULATOR
	if (timer.regulator)
		regulator_disable(timer.regulator);
#endif

	device_remove_file(dev, &dev_attr_sunplus_tsp_debugfs);
	device_remove_file(dev, &dev_attr_clk_funcEn);
	device_remove_file(dev, &dev_attr_clk_lock);
	device_destroy(timer.class, timer.devno);
	class_destroy(timer.class);

	//iounmap memory
	iounmap(timer_base);

	//free memory region
	release_mem_region(timer.r.start, resource_size(&timer.r));

#ifdef TSP_USE_CLK
	if (timer.clk_gate)
		clk_disable_unprepare(timer.clk_gate);

	if (timer.vcl_clk_gate)
		clk_disable_unprepare(timer.vcl_clk_gate);

	if (timer.vcl5_clk_gate)
		clk_disable_unprepare(timer.vcl5_clk_gate);
#endif

	return 0;
}

#ifdef CONFIG_PM
static int sunplus_timer_recover_setting(void)
{
	int index = 0;

	writel(timer.tsp_ctrl0_val, timer_base + SUNPLUS_TSP_CTRL_0);
	sunplus_mipi_cycle_update(0);
	sunplus_imu_cycle_update(0);
	sunplus_tc.nsec = sunplus_mipi0_tc.nsec = sunplus_mipi1_tc.nsec = \
	sunplus_mipi2_tc.nsec = sunplus_mipi3_tc.nsec = sunplus_mipi4_tc.nsec = \
	sunplus_mipi5_tc.nsec = sunplus_imu0_tc.nsec = sunplus_imu1_tc.nsec = \
	sunplus_imu2_tc.nsec = sunplus_imu3_tc.nsec = ktime_get_raw_ns() - timer.raw_time_s;

	for (index = 0; index < MAX_TSP_DEVICE; ++index) {
		timer_mem_address(index);
	}

	return 0;
}

static int sunplus_suspend_common(void)
{
	writel(timer.tsp_ctrl0_val & 0xFFFFFC00, timer_base + SUNPLUS_TSP_CTRL_0);
#ifdef TSP_USE_RESET
	if (timer.timer_rst)
		reset_control_assert(timer.timer_rst);

	if (timer.vcl5_rst)
		reset_control_assert(timer.vcl5_rst);

	if (timer.vcl_rst)
		reset_control_assert(timer.vcl_rst);
#endif
#ifdef TSP_USE_CLK
	sunplus_timer_clk_gating(timer.clk_gate, true);
	sunplus_timer_clk_gating(timer.vcl5_clk_gate, true);
	sunplus_timer_clk_gating(timer.vcl_clk_gate, true);
#endif
#ifdef TSP_USE_REGULATOR
	if (timer.regulator)
		regulator_disable(timer.regulator);
#endif
	timer.in_suspend = true;

	return 0 ;
}

static int sunplus_timer_suspend_ops(struct device *dev)
{
	unsigned long flags;

	tsp_info(timer, "%s \n", __func__);
	spin_lock_irqsave(&timer.timer_lock, flags);
	sunplus_suspend_common();
	spin_unlock_irqrestore(&timer.timer_lock, flags);

	return 0;
}

static int sunplus_timer_resume_ops(struct device *dev)
{
	unsigned long flags;

	tsp_info(timer, "%s \n", __func__);
	spin_lock_irqsave(&timer.timer_lock, flags);
#ifdef TSP_USE_REGULATOR
	if (timer.regulator)
		regulator_enable(timer.regulator);
#endif
#ifdef TSP_USE_CLK
	sunplus_timer_clk_gating(timer.vcl_clk_gate, false);
	sunplus_timer_clk_gating(timer.vcl5_clk_gate, false);
	sunplus_timer_clk_gating(timer.clk_gate, false);
#endif
#ifdef TSP_USE_RESET
	if (timer.vcl_rst)
		reset_control_deassert(timer.vcl_rst);

	if (timer.vcl5_rst)
		reset_control_deassert(timer.vcl5_rst);

	if (timer.timer_rst)
		reset_control_deassert(timer.timer_rst);
#endif

	writel(1, timer_base + SUNPLUS_TSP_CTRL_1); /* tsp_sw_rst */

	sunplus_timer_recover_setting();
	timer.in_suspend = false;

	mod_timer(&timer.tsp_timer, jiffies + msecs_to_jiffies(timer.callback_timer));
	spin_unlock_irqrestore(&timer.timer_lock, flags);

	return 0;
}

static const struct dev_pm_ops sunplus_timer_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(sunplus_timer_suspend_ops, sunplus_timer_resume_ops)
};
#endif

static const struct of_device_id sunplus_timer_ids[] = {
	{
		.compatible = "sunplus,sp7350-tsp",
	},
	{}
};
MODULE_DEVICE_TABLE(of, sunplus_timer_ids);

static struct platform_driver sunplus_timer_driver = {
	.driver = {
		.name = TSP_NAME,
		.of_match_table = sunplus_timer_ids,
#ifdef CONFIG_PM
		.pm = &sunplus_timer_pm,
#endif
	},
	.probe = sunplus_timer_probe,
	.remove = sunplus_timer_remove,
};

module_platform_driver(sunplus_timer_driver);

MODULE_AUTHOR("Sunplus Corporation");
MODULE_DESCRIPTION("Sunplus TimeStamp driver");
MODULE_LICENSE("GPL");

