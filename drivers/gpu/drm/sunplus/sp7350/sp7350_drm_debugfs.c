// SPDX-License-Identifier: GPL-2.0+
/*
 * Sunplus SP7350 SoC DRM debugfs
 *
 * Author: hammer.hsieh<hammer.hsieh@sunplus.com>
 */

#include <linux/seq_file.h>
#include <linux/circ_buf.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>

#include "sp7350_drm_drv.h"
#include "sp7350_drm_regs.h"

struct sp7350_debugfs_info_entry {
	struct list_head link;
	struct drm_info_list info;
};

/**
 * Called at drm_dev_register() time on each of the minors registered
 * by the DRM device, to attach the debugfs files.
 */
void
sp7350_debugfs_init(struct drm_minor *minor)
{
	struct sp7350_dev *sp_dev = to_sp7350_dev(minor->dev);
	struct sp7350_debugfs_info_entry *entry;

	debugfs_create_bool("debug_test", 0644,
			    minor->debugfs_root, &sp_dev->debug_test_enabled);

	list_for_each_entry(entry, &sp_dev->debugfs_list, link) {
		drm_debugfs_create_files(&entry->info, 1,
					 minor->debugfs_root, minor);
	}
}

static int sp7350_debugfs_regset32(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct debugfs_regset32 *regset = node->info_ent->data;
	struct drm_printer p = drm_seq_file_printer(m);

	drm_print_regset32(&p, regset);

	return 0;
}

/**
 * Registers a debugfs file with a callback function for a sp7350 component.
 *
 * This is like drm_debugfs_create_files(), but that can only be
 * called a given DRM minor, while the various SP7350 components want to
 * register their debugfs files during the component bind process.  We
 * track the request and delay it to be called on each minor during
 * sp7350_debugfs_init().
 */
void sp7350_debugfs_add_file(struct drm_device *dev,
			  const char *name,
			  int (*show)(struct seq_file*, void*),
			  void *data)
{
	struct sp7350_dev *sp_dev = to_sp7350_dev(dev);

	struct sp7350_debugfs_info_entry *entry =
		devm_kzalloc(dev->dev, sizeof(*entry), GFP_KERNEL);

	if (!entry)
		return;

	entry->info.name = name;
	entry->info.show = show;
	entry->info.data = data;

	list_add(&entry->link, &sp_dev->debugfs_list);
}

void sp7350_debugfs_add_regset32(struct drm_device *drm,
			      const char *name,
			      struct debugfs_regset32 *regset)
{
	sp7350_debugfs_add_file(drm, name, sp7350_debugfs_regset32, regset);
}
