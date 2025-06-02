// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for ISP
 *
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include "isp.h"
#include "isp_dbg.h"
#include "isp_debugfs.h"

struct kobject *isp_debug_kobj;
const char isp_debug_group_name[] = "isp_debug";
static char *reg_file_name = NULL;
static char *input_file_name = NULL;
static char *input_2_file_name = NULL;

static char *mlsc_file_name = NULL;
static char *fcurve_file_name = NULL;
static char *wdr_file_name = NULL;
static char *lce_file_name = NULL;
static char *cnr_file_name = NULL;
static char *d3nr_y_file_name = NULL;
static char *d3nr_uv_file_name = NULL;
static char *d3nr_v_file_name = NULL;
static char *d3nr_mot_file_name = NULL;

static char *dump_folder = NULL;
static u8 dump_statis = 0;
static u8 dump_3dnr = 0;
static u8 dump_input = 0;
static u8 dump_output = 0;
static u8 dump_3a = 0;
static u8 dump_reg = 0;

static struct list_head target_list;
struct isp_dbg_info {
	u8 regQ_id;
	struct list_head head;
};

static void isp_debugfs_clean_all(void);

static int isp_stoi(const char *buf, size_t n)
{
	int target_idx = 0;
	int loop = 0;
	while (loop < n - 1) {
		if ((buf[loop] - '0' >= 0) && (buf[loop] - '0' <= 9)) {
			target_idx = target_idx * 10 + buf[loop] - '0';
		} else {
			printk("(isp_dbg) wrong data = %s\n", buf);
			return -1;
		}
		loop++;
	}
	return target_idx;
}

static void isp_debugfs_travel_list(void)
{
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct isp_dbg_info *t_entry = NULL;
	list_for_each_safe(listptr, listptr_next, &target_list) {
		t_entry = list_entry(listptr, struct isp_dbg_info, head);
		printk("(isp_dbg) target idx = %d\n", t_entry->regQ_id);
	}
}

int isp_dbgfs_target_idx_exist(u8 regQ_idx)
{
	// return value: 1: found, 0: not found

	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct isp_dbg_info *t_entry = NULL;

	if (list_empty(&target_list)) {
		// user did not set target idx, so apply on all regQ_idx
		return 1;
	}

	list_for_each_safe(listptr, listptr_next, &target_list) {
		t_entry = list_entry(listptr, struct isp_dbg_info, head);
		if (t_entry->regQ_id == regQ_idx) {
			return 1;
		}
	}
	return 0;
}

static ssize_t isp_del_idx_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	isp_debugfs_travel_list();
	return 0;
}

static ssize_t isp_del_idx_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t n)
{
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct isp_dbg_info *entry = NULL;
	int target_idx = isp_stoi(buf, n);
	if (target_idx == -1)
		goto err_end;

	list_for_each_safe(listptr, listptr_next, &target_list) {
		entry = list_entry(listptr, struct isp_dbg_info, head);
		if (entry->regQ_id == (u8)target_idx) {
			list_del(listptr);
			kfree(entry);
			break;
		}
	}

err_end:
	return n;
}

static ssize_t isp_add_idx_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	isp_debugfs_travel_list();
	return 0;
}

static ssize_t isp_add_idx_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t n)
{
	int target_idx = 0;
	struct isp_dbg_info *entry =
		kzalloc(sizeof(struct isp_dbg_info), GFP_KERNEL);

	if (entry == NULL) {
		printk("(isp_dbg) %s: isp_dbg_info entry alloc failed \n",
		       __func__);
		goto err_end;
	}

	target_idx = isp_stoi(buf, n);
	if (target_idx == -1)
		goto err_end;

	entry->regQ_id = (u8)target_idx;
	list_add_tail(&entry->head, &target_list);
	printk("(isp_dbg) add target idx = %d\n", entry->regQ_id);

err_end:
	return n;
}

static ssize_t isp_debug_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	// show queue status
	isp_video_monitor_q_info();

	return 0;
}

static ssize_t isp_debug_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t n)
{
	return n;
}

static ssize_t isp_clk_gate_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	if (isp_video_get_clk_func_status() == ISP_CLK_FUNC_EN)
		printk("(isp_dbg) isp clk gate flow on\n");
	else
		printk("(isp_dbg) isp clk gate flow off\n");
	return 0;
}

static ssize_t isp_clk_gate_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	printk("(isp_dbg) user clk flow setting: %s\n", buf);

	if (strncmp(buf, "0", 1) == 0) {
		isp_dbg_set_clk_func(ISP_CLK_FUNC_DIS);
	} else if (strncmp(buf, "1", 1) == 0) {
		isp_dbg_set_clk_func(ISP_CLK_FUNC_EN);
	}

	if (ISP_CLK_FUNC_EN == isp_video_get_clk_func_status())
		printk("(isp_dbg) isp clk gate flow on\n");
	else
		printk("(isp_dbg) isp clk gate flow off\n");
	return n;
}

static ssize_t isp_dump_log_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	printk("(isp_dbg) === isp version %d === %s(%d) \n", ISP_VERSION,
	       __func__, __LINE__);
	printk("(isp_dbg) === isp dump log: %d === %s(%d) \n",
	       isp_video_get_dump_log_status(), __func__, __LINE__);

	return 0;
}

static ssize_t isp_dump_log_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	printk("(isp_dbg) user dump log setting: %s\n", buf);
	if (strncmp(buf, "0", 1) == 0)
		isp_video_set_dump_log_status(0);
	else
		isp_video_set_dump_log_status(1);
	printk("(isp_dbg) isp dump log: %d \n",
	       isp_video_get_dump_log_status());
	return n;
}

static ssize_t isp_hard_reg_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	if (reg_file_name == NULL)
		printk("(isp_dbg) === no hardcode reg filename ===\n");
	else
		printk("(isp_dbg) === hardcode reg filename = %s ===\n",
		       reg_file_name);

	return 0;
}

static ssize_t isp_hard_reg_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	printk("(isp_dbg) user reg in, filename: %s (string_size: %ld) \n", buf,
	       n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (reg_file_name != NULL) {
				kfree(reg_file_name);
				reg_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (reg_file_name != NULL) {
				kfree(reg_file_name);
				reg_file_name = NULL;
			}

			reg_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(reg_file_name, buf, n);
			reg_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_reg_filename(void)
{
	return reg_file_name;
}

static ssize_t isp_hard_input_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	if (input_file_name == NULL)
		printk("(isp_dbg) === no hardcode input filename ===\n");
	else
		printk("(isp_dbg) === hardcode input filename = %s ===\n",
		       input_file_name);

	return 0;
}

static ssize_t isp_hard_input_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t n)
{
	printk("(isp_dbg) user input in, filename: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (input_file_name != NULL) {
				kfree(input_file_name);
				input_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (input_file_name != NULL) {
				kfree(input_file_name);
				input_file_name = NULL;
			}

			input_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(input_file_name, buf, n);
			input_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_input_filename(void)
{
	return input_file_name;
}

static ssize_t isp_hard_input_2_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	if (input_2_file_name == NULL)
		printk("(isp_dbg) === no hardcode input2 filename ===\n");
	else
		printk("(isp_dbg) === hardcode input2 filename = %s ===\n",
		       input_2_file_name);

	return 0;
}

static ssize_t isp_hard_input_2_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t n)
{
	printk("(isp_dbg) user input_2 in, filename: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (input_2_file_name != NULL) {
				kfree(input_2_file_name);
				input_2_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (input_2_file_name != NULL) {
				kfree(input_2_file_name);
				input_2_file_name = NULL;
			}

			input_2_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(input_2_file_name, buf, n);
			input_2_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_input_2_filename(void)
{
	return input_2_file_name;
}

static ssize_t isp_hard_mlsc_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	if (mlsc_file_name == NULL)
		printk("(isp_dbg) === no hardcode mlsc filename ===\n");
	else
		printk("(isp_dbg) === hardcode mlsc filename = %s ===\n",
		       mlsc_file_name);

	return 0;
}

static ssize_t isp_hard_mlsc_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t n)
{
	printk("(isp_dbg) user mlsc in, filename: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (mlsc_file_name != NULL) {
				kfree(mlsc_file_name);
				mlsc_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (mlsc_file_name != NULL) {
				kfree(mlsc_file_name);
				mlsc_file_name = NULL;
			}

			mlsc_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(mlsc_file_name, buf, n);
			mlsc_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_mlsc_filename(void)
{
	return mlsc_file_name;
}

static ssize_t isp_hard_fcurve_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	if (mlsc_file_name == NULL)
		printk("(isp_dbg) === no hardcode fcurve filename ===\n");
	else
		printk("(isp_dbg) === hardcode fcurve filename = %s ===\n",
		       fcurve_file_name);

	return 0;
}

static ssize_t isp_hard_fcurve_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t n)
{
	printk("(isp_dbg) user fcurve in, filename: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (fcurve_file_name != NULL) {
				kfree(fcurve_file_name);
				fcurve_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (fcurve_file_name != NULL) {
				kfree(fcurve_file_name);
				fcurve_file_name = NULL;
			}

			fcurve_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(fcurve_file_name, buf, n);
			fcurve_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_fcurve_filename(void)
{
	return fcurve_file_name;
}

static ssize_t isp_hard_wdr_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	if (wdr_file_name == NULL)
		printk("(isp_dbg) === no hardcode wdr filename ===\n");
	else
		printk("(isp_dbg) === hardcode wdr filename = %s ===\n",
		       wdr_file_name);

	return 0;
}

static ssize_t isp_hard_wdr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	printk("(isp_dbg) user wdr in, filename: %s (string_size: %ld) \n", buf,
	       n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (wdr_file_name != NULL) {
				kfree(wdr_file_name);
				wdr_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (wdr_file_name != NULL) {
				kfree(wdr_file_name);
				wdr_file_name = NULL;
			}

			wdr_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(wdr_file_name, buf, n);
			wdr_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_wdr_filename(void)
{
	return wdr_file_name;
}

static ssize_t isp_hard_lce_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	if (lce_file_name == NULL)
		printk("(isp_dbg) === no hardcode lce filename ===\n");
	else
		printk("(isp_dbg) === hardcode lce filename = %s ===\n",
		       lce_file_name);

	return 0;
}

static ssize_t isp_hard_lce_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	printk("(isp_dbg) user lce in, filename: %s (string_size: %ld) \n", buf,
	       n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (lce_file_name != NULL) {
				kfree(lce_file_name);
				lce_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (lce_file_name != NULL) {
				kfree(lce_file_name);
				lce_file_name = NULL;
			}

			lce_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(lce_file_name, buf, n);
			lce_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_lce_filename(void)
{
	return lce_file_name;
}

static ssize_t isp_hard_cnr_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	if (cnr_file_name == NULL)
		printk("(isp_dbg) === no hardcode cnr filename ===\n");
	else
		printk("(isp_dbg) === hardcode cnr filename = %s ===\n",
		       cnr_file_name);

	return 0;
}

static ssize_t isp_hard_cnr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	printk("(isp_dbg) user cnr in, filename: %s (string_size: %ld) \n", buf,
	       n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (cnr_file_name != NULL) {
				kfree(cnr_file_name);
				cnr_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (cnr_file_name != NULL) {
				kfree(cnr_file_name);
				cnr_file_name = NULL;
			}

			cnr_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(cnr_file_name, buf, n);
			cnr_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_cnr_filename(void)
{
	return cnr_file_name;
}

static ssize_t isp_hard_3dnr_y_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	if (d3nr_y_file_name == NULL)
		printk("(isp_dbg) === no hardcode 3dnr_y filename ===\n");
	else
		printk("(isp_dbg) === hardcode 3dnr_y filename = %s ===\n",
		       d3nr_y_file_name);

	return 0;
}

static ssize_t isp_hard_3dnr_y_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t n)
{
	printk("(isp_dbg) user 3dnr_y in, filename: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (d3nr_y_file_name != NULL) {
				kfree(d3nr_y_file_name);
				d3nr_y_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (d3nr_y_file_name != NULL) {
				kfree(d3nr_y_file_name);
				d3nr_y_file_name = NULL;
			}

			d3nr_y_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(d3nr_y_file_name, buf, n);
			d3nr_y_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_3dnr_y_filename(void)
{
	return d3nr_y_file_name;
}

static ssize_t isp_hard_3dnr_uv_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	if (d3nr_uv_file_name == NULL)
		printk("(isp_dbg) === no hardcode 3dnr_uv filename ===\n");
	else
		printk("(isp_dbg) === hardcode 3dnr_uv filename = %s ===\n",
		       d3nr_uv_file_name);

	return 0;
}

static ssize_t isp_hard_3dnr_uv_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t n)
{
	printk("(isp_dbg) user 3dnr_uv in, filename: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (d3nr_uv_file_name != NULL) {
				kfree(d3nr_uv_file_name);
				d3nr_uv_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (d3nr_uv_file_name != NULL) {
				kfree(d3nr_uv_file_name);
				d3nr_uv_file_name = NULL;
			}

			d3nr_uv_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(d3nr_uv_file_name, buf, n);
			d3nr_uv_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_3dnr_uv_filename(void)
{
	return d3nr_uv_file_name;
}

static ssize_t isp_hard_3dnr_v_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	if (d3nr_v_file_name == NULL)
		printk("(isp_dbg) === no hardcode 3dnr_v filename ===\n");
	else
		printk("(isp_dbg) === hardcode 3dnr_v filename = %s ===\n",
		       d3nr_v_file_name);

	return 0;
}

static ssize_t isp_hard_3dnr_v_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t n)
{
	printk("(isp_dbg) user 3dnr_v in, filename: %s (string_size: %ld)\n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (d3nr_v_file_name != NULL) {
				kfree(d3nr_v_file_name);
				d3nr_v_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (d3nr_v_file_name != NULL) {
				kfree(d3nr_v_file_name);
				d3nr_v_file_name = NULL;
			}

			d3nr_v_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(d3nr_v_file_name, buf, n);
			d3nr_v_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_3dnr_v_filename(void)
{
	return d3nr_v_file_name;
}

static ssize_t isp_hard_3dnr_mot_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	if (d3nr_mot_file_name == NULL)
		printk("(isp_dbg) === no hardcode 3dnr_mot filename ===\n");
	else
		printk("(isp_dbg) === hardcode 3dnr_mot filename = %s ===\n",
		       d3nr_mot_file_name);

	return 0;
}

static ssize_t isp_hard_3dnr_mot_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t n)
{
	printk("(isp_dbg) user 3dnr_mot in, filename: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (d3nr_mot_file_name != NULL) {
				kfree(d3nr_mot_file_name);
				d3nr_mot_file_name = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (d3nr_mot_file_name != NULL) {
				kfree(d3nr_mot_file_name);
				d3nr_mot_file_name = NULL;
			}

			d3nr_mot_file_name = kzalloc(n, GFP_KERNEL);
			strncpy(d3nr_mot_file_name, buf, n);
			d3nr_mot_file_name[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_3dnr_mot_filename(void)
{
	return d3nr_mot_file_name;
}

//======dump======

static ssize_t isp_dump_folder_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	if (dump_folder == NULL)
		printk("(isp_dbg) === no dump folder ===\n");
	else
		printk("(isp_dbg) === dump folder path : %s ===\n",
		       dump_folder);

	return 0;
}

static ssize_t isp_dump_folder_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t n)
{
	printk("(isp_dbg) user dump folder, path name: %s (string_size: %ld) \n",
	       buf, n);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			if (dump_folder != NULL) {
				kfree(dump_folder);
				dump_folder = NULL;
			}
		} else if (buf != NULL && strcmp(buf, "") != 0) {
			if (dump_folder != NULL) {
				kfree(dump_folder);
				dump_folder = NULL;
			}

			dump_folder = kzalloc(n, GFP_KERNEL);
			strncpy(dump_folder, buf, n);
			dump_folder[n - 1] = '\0';
		}
	}
	return n;
}

char *isp_dbgfs_get_dump_folder(void)
{
	return dump_folder;
}

static ssize_t isp_dump_reg_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	printk("(isp_dbg) user dump reg : %s \n", buf);
	if (dump_folder == NULL)
		printk("(isp_dbg) user dump reg failed, no dump folder\n");
	else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 47)
		printk("(isp_dbg) user dump reg start \n");
		isp_dbg_dump_last_reg();
		printk("(isp_dbg) user dump reg end \n");
#endif
	}
	return 0;
}

static ssize_t isp_dump_reg_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	printk("(isp_dbg) user dump reg : %s \n", buf);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			dump_reg = 0;
			printk("(isp_dbg) dump reg(per frame) off  %s(%d)\n",
			       __func__, __LINE__);
		} else if (buf != NULL && strncmp(buf, "1", 1) == 0) {
			dump_reg = 1;
			printk("(isp_dbg) dump reg(per frame) on  %s(%d)\n",
			       __func__, __LINE__);
		}
	}
	return n;
}

u8 isp_dbgfs_get_dump_reg(void)
{
	return dump_reg;
}

static ssize_t isp_dump_statis_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t isp_dump_statis_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t n)
{
	printk("(isp_dbg) user dump statis : %s \n", buf);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			dump_statis = 0;
			printk("(isp_dbg) dump statis off  %s(%d)\n", __func__,
			       __LINE__);
		} else if (buf != NULL && strncmp(buf, "1", 1) == 0) {
			dump_statis = 1;
			printk("(isp_dbg) dump statis on  %s(%d)\n", __func__,
			       __LINE__);
		}
	}
	return n;
}

u8 isp_dbgfs_get_dump_statis(void)
{
	return dump_statis;
}

static ssize_t isp_dump_3dnr_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t isp_dump_3dnr_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t n)
{
	printk("(isp_dbg) user dump 3dnr : %s \n", buf);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			dump_3dnr = 0;
			printk("(isp_dbg) dump 3dnr off  %s(%d)\n", __func__,
			       __LINE__);
		} else if (buf != NULL && strncmp(buf, "1", 1) == 0) {
			dump_3dnr = 1;
			printk("(isp_dbg) dump 3dnr on  %s(%d)\n", __func__,
			       __LINE__);
		}
	}
	return n;
}

u8 isp_dbgfs_get_dump_3dnr(void)
{
	return dump_3dnr;
}

static ssize_t isp_dump_3a_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t isp_dump_3a_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t n)
{
	printk("(isp_dbg) user dump 3a : %s \n", buf);

	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			dump_3a = 0;
			printk("(isp_dbg) dump 3a off  %s(%d)\n", __func__,
			       __LINE__);
		} else if (buf != NULL && strncmp(buf, "1", 1) == 0) {
			dump_3a = 1;
			printk("(isp_dbg) dump 3a on  %s(%d)\n", __func__,
			       __LINE__);
		}
	}
	return n;
}

u8 isp_dbgfs_get_dump_3a(void)
{
	return dump_3a;
}

static ssize_t isp_dump_output_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t isp_dump_output_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t n)
{
	printk("(isp_dbg) user dump output frame : %s \n", buf);
	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			dump_output = 0;
			printk("(isp_dbg) dump output frame off  %s(%d)\n",
			       __func__, __LINE__);
		} else if (buf != NULL && strncmp(buf, "1", 1) == 0) {
			dump_output = 1;
			printk("(isp_dbg) dump output frame on  %s(%d)\n",
			       __func__, __LINE__);
		}
	}
	return n;
}

u8 isp_dbgfs_get_dump_output_frame(void)
{
	return dump_output;
}

static ssize_t isp_dump_input_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t isp_dump_input_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t n)
{
	printk("(isp_dbg) user dump input frame : %s \n", buf);
	if (n > 1) {
		if (buf != NULL && strncmp(buf, "0", 1) == 0) {
			dump_input = 0;
			printk("(isp_dbg) dump input frame off  %s(%d)\n",
			       __func__, __LINE__);
		} else if (buf != NULL && strncmp(buf, "1", 1) == 0) {
			dump_input = 1;
			printk("(isp_dbg) dump input frame on  %s(%d)\n",
			       __func__, __LINE__);
		}
	}
	return n;
}

u8 isp_dbgfs_get_dump_input_frame(void)
{
	return dump_input;
}

static ssize_t isp_list_all_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	printk("(isp_dbg) === list all ===   \n");

	printk("(isp_dbg) === isp version %d === \n", ISP_VERSION);
	printk("(isp_dbg) dump log : %d \n", isp_video_get_dump_log_status());
	printk("(isp_dbg) dump folder: %s \n",
	       (dump_folder == NULL) ? "none" : dump_folder);

	printk("(isp_dbg) list target idx below\n");
	isp_debugfs_travel_list();

	printk("(isp_dbg) dump input frame: %s \n",
	       (dump_input == 0) ? "off" : "on");
	printk("(isp_dbg) dump output frame: %s \n",
	       (dump_output == 0) ? "off" : "on");
	printk("(isp_dbg) dump statis: %s \n",
	       (dump_statis == 0) ? "off" : "on");
	printk("(isp_dbg) dump 3dnr: %s \n", (dump_3dnr == 0) ? "off" : "on");
	printk("(isp_dbg) dump 3a: %s \n", (dump_3a == 0) ? "off" : "on");

	if (ISP_CLK_FUNC_EN == isp_video_get_clk_func_status())
		printk("(isp_dbg) isp clk gate flow: on\n");
	else
		printk("(isp_dbg) isp clk gate flow: off\n");

	printk("(isp_dbg) hardcode reg file: %s \n",
	       (reg_file_name == NULL) ? "none" : reg_file_name);

	printk("(isp_dbg) hardcode input raw 1: %s \n",
	       (input_file_name == NULL) ? "none" : input_file_name);
	printk("(isp_dbg) hardcode input raw 2: %s \n",
	       (input_2_file_name == NULL) ? "none" : input_2_file_name);

	printk("(isp_dbg) hardcode mlsc: %s \n",
	       (mlsc_file_name == NULL) ? "none" : mlsc_file_name);
	printk("(isp_dbg) hardcode fcurve: %s \n",
	       (fcurve_file_name == NULL) ? "none" : fcurve_file_name);
	printk("(isp_dbg) hardcode wdr: %s \n",
	       (wdr_file_name == NULL) ? "none" : wdr_file_name);
	printk("(isp_dbg) hardcode lce: %s \n",
	       (lce_file_name == NULL) ? "none" : lce_file_name);
	printk("(isp_dbg) hardcode cnr: %s \n",
	       (cnr_file_name == NULL) ? "none" : cnr_file_name);

	printk("(isp_dbg) hardcode 3dnr y: %s \n",
	       (d3nr_y_file_name == NULL) ? "none" : d3nr_y_file_name);
	printk("(isp_dbg) hardcode 3dnr uv: %s \n",
	       (d3nr_uv_file_name == NULL) ? "none" : d3nr_uv_file_name);
	printk("(isp_dbg) hardcode 3dnr v: %s \n",
	       (d3nr_v_file_name == NULL) ? "none" : d3nr_v_file_name);
	printk("(isp_dbg) hardcode 3dnr mot: %s \n",
	       (d3nr_mot_file_name == NULL) ? "none" : d3nr_mot_file_name);

	return 0;
}

static ssize_t isp_list_all_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t n)
{
	return n;
}

static void isp_debugfs_clean_all(void)
{
	struct list_head *listptr = NULL, *listptr_next = NULL;
	struct isp_dbg_info *entry = NULL;

	// del all target idx list
	list_for_each_safe(listptr, listptr_next, &target_list) {
		entry = list_entry(listptr, struct isp_dbg_info, head);
		list_del(listptr);
		kfree(entry);
	}

	// disable log
	isp_video_set_dump_log_status(0);

	// disable dump flag
	dump_statis = 0;
	dump_3dnr = 0;
	dump_input = 0;
	dump_output = 0;
	dump_3a = 0;

	// clean dump ctrl
	if (dump_folder != NULL) {
		kfree(dump_folder);
		dump_folder = NULL;
	}

	// clean hardcode filename
	if (reg_file_name != NULL) {
		kfree(reg_file_name);
		reg_file_name = NULL;
	}
	if (input_file_name != NULL) {
		kfree(input_file_name);
		input_file_name = NULL;
	}
	if (input_2_file_name != NULL) {
		kfree(input_2_file_name);
		input_2_file_name = NULL;
	}
	if (mlsc_file_name != NULL) {
		kfree(mlsc_file_name);
		mlsc_file_name = NULL;
	}
	if (fcurve_file_name != NULL) {
		kfree(fcurve_file_name);
		fcurve_file_name = NULL;
	}
	if (wdr_file_name != NULL) {
		kfree(wdr_file_name);
		wdr_file_name = NULL;
	}
	if (lce_file_name != NULL) {
		kfree(lce_file_name);
		lce_file_name = NULL;
	}
	if (cnr_file_name != NULL) {
		kfree(cnr_file_name);
		cnr_file_name = NULL;
	}
	if (d3nr_y_file_name != NULL) {
		kfree(d3nr_y_file_name);
		d3nr_y_file_name = NULL;
	}
	if (d3nr_uv_file_name != NULL) {
		kfree(d3nr_uv_file_name);
		d3nr_uv_file_name = NULL;
	}
	if (d3nr_v_file_name != NULL) {
		kfree(d3nr_v_file_name);
		d3nr_v_file_name = NULL;
	}
	if (d3nr_mot_file_name != NULL) {
		kfree(d3nr_mot_file_name);
		d3nr_mot_file_name = NULL;
	}
}

static ssize_t isp_clean_all_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t isp_clean_all_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t n)
{
	isp_debugfs_clean_all();
	return n;
}

static DEVICE_ATTR_RW(isp_debug);
static DEVICE_ATTR_RW(isp_clk_gate);
static DEVICE_ATTR_RW(isp_dump_log);
static DEVICE_ATTR_RW(isp_hard_reg);
static DEVICE_ATTR_RW(isp_hard_input);
static DEVICE_ATTR_RW(isp_hard_input_2);
static DEVICE_ATTR_RW(isp_hard_mlsc);
static DEVICE_ATTR_RW(isp_hard_fcurve);
static DEVICE_ATTR_RW(isp_hard_wdr);
static DEVICE_ATTR_RW(isp_hard_lce);
static DEVICE_ATTR_RW(isp_hard_cnr);
static DEVICE_ATTR_RW(isp_hard_3dnr_y);
static DEVICE_ATTR_RW(isp_hard_3dnr_uv);
static DEVICE_ATTR_RW(isp_hard_3dnr_v);
static DEVICE_ATTR_RW(isp_hard_3dnr_mot);
static DEVICE_ATTR_RW(isp_dump_folder);
static DEVICE_ATTR_RW(isp_dump_input);
static DEVICE_ATTR_RW(isp_dump_output);
static DEVICE_ATTR_RW(isp_dump_statis);
static DEVICE_ATTR_RW(isp_dump_3dnr);
static DEVICE_ATTR_RW(isp_dump_3a);
static DEVICE_ATTR_RW(isp_dump_reg);
static DEVICE_ATTR_RW(isp_list_all);
static DEVICE_ATTR_RW(isp_clean_all);
static DEVICE_ATTR_RW(isp_add_idx);
static DEVICE_ATTR_RW(isp_del_idx);

static struct attribute *isp_attrs[] = {
	&dev_attr_isp_debug.attr,
	&dev_attr_isp_clk_gate.attr,
	&dev_attr_isp_dump_log.attr,
	&dev_attr_isp_hard_reg.attr,
	&dev_attr_isp_hard_input.attr,
	&dev_attr_isp_hard_input_2.attr,
	&dev_attr_isp_hard_mlsc.attr,
	&dev_attr_isp_hard_fcurve.attr,
	&dev_attr_isp_hard_wdr.attr,
	&dev_attr_isp_hard_lce.attr,
	&dev_attr_isp_hard_cnr.attr,
	&dev_attr_isp_hard_3dnr_y.attr,
	&dev_attr_isp_hard_3dnr_uv.attr,
	&dev_attr_isp_hard_3dnr_v.attr,
	&dev_attr_isp_hard_3dnr_mot.attr,
	&dev_attr_isp_dump_folder.attr,
	&dev_attr_isp_dump_input.attr,
	&dev_attr_isp_dump_output.attr,
	&dev_attr_isp_dump_statis.attr,
	&dev_attr_isp_dump_3dnr.attr,
	&dev_attr_isp_dump_3a.attr,
	&dev_attr_isp_dump_reg.attr,
	&dev_attr_isp_list_all.attr,
	&dev_attr_isp_clean_all.attr,
	&dev_attr_isp_add_idx.attr,
	&dev_attr_isp_del_idx.attr,
	NULL,
};

static const struct attribute_group isp_attr_group = {
	.name = isp_debug_group_name,
	.attrs = isp_attrs,
};

int isp_debugfs_init(struct device *dev)
{
	int rc;

	isp_debug_kobj = kobject_create_and_add("sp", NULL);
	if (!isp_debug_kobj) {
		return -ENOMEM;
	}

	rc = sysfs_create_group(isp_debug_kobj, &isp_attr_group);
	if (rc) {
		kobject_put(isp_debug_kobj);
	}

	INIT_LIST_HEAD(&target_list);

	return 0;
}

void isp_debugfs_remove(struct device *dev)
{
	isp_debugfs_clean_all();

	sysfs_remove_group(isp_debug_kobj, &isp_attr_group);
	kobject_put(isp_debug_kobj);
}
