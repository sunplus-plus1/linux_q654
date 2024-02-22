/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM fbdev
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <linux/console.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_atomic_helper.h>

#include <drm/drm_crtc.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_crtc_helper.h>

#include "sp7350_drm_fbdev.h"
#include "../../drm_internal.h"

static int sp7350_drm_fbdev_fb_mmap(struct fb_info *info,
			struct vm_area_struct *vma)
{
	struct drm_fb_helper *fb_helper = info->par;

	if (fb_helper->dev->driver->gem_prime_mmap)
		return fb_helper->dev->driver->gem_prime_mmap(fb_helper->buffer->gem, vma);
	else
		return -ENODEV;
}

static void pan_set(struct drm_fb_helper *fb_helper, int x, int y)
{
	struct drm_mode_set *mode_set;

	mutex_lock(&fb_helper->client.modeset_mutex);
	drm_client_for_each_modeset(mode_set, &fb_helper->client) {
		mode_set->x = x;
		mode_set->y = y;
	}
	mutex_unlock(&fb_helper->client.modeset_mutex);
}

static int update_output_state(struct drm_atomic_state *state,
			       struct drm_mode_set *set)
{
	struct drm_device *dev = set->crtc->dev;
	struct drm_crtc *crtc;
	struct drm_crtc_state *new_crtc_state;
	struct drm_connector *connector;
	struct drm_connector_state *new_conn_state;
	int ret, i;

	ret = drm_modeset_lock(&dev->mode_config.connection_mutex,
			       state->acquire_ctx);
	if (ret)
		return ret;

	/* First disable all connectors on the target crtc. */
	ret = drm_atomic_add_affected_connectors(state, set->crtc);
	if (ret)
		return ret;

	for_each_new_connector_in_state(state, connector, new_conn_state, i) {
		if (new_conn_state->crtc == set->crtc) {
			ret = drm_atomic_set_crtc_for_connector(new_conn_state,
								NULL);
			if (ret)
				return ret;

			/* Make sure legacy setCrtc always re-trains */
			new_conn_state->link_status = DRM_LINK_STATUS_GOOD;
		}
	}

	/* Then set all connectors from set->connectors on the target crtc */
	for (i = 0; i < set->num_connectors; i++) {
		new_conn_state = drm_atomic_get_connector_state(state,
								set->connectors[i]);
		if (IS_ERR(new_conn_state))
			return PTR_ERR(new_conn_state);

		ret = drm_atomic_set_crtc_for_connector(new_conn_state,
							set->crtc);
		if (ret)
			return ret;
	}

	for_each_new_crtc_in_state(state, crtc, new_crtc_state, i) {
		/*
		 * Don't update ->enable for the CRTC in the set_config request,
		 * since a mismatch would indicate a bug in the upper layers.
		 * The actual modeset code later on will catch any
		 * inconsistencies here.
		 */
		if (crtc == set->crtc)
			continue;

		if (!new_crtc_state->connector_mask) {
			ret = drm_atomic_set_mode_prop_for_crtc(new_crtc_state,
								NULL);
			if (ret < 0)
				return ret;

			new_crtc_state->active = false;
		}
	}

	return 0;
}

/* refer to drm_client_modeset_commit_atomic */
static int _drm_client_modeset_commit_atomic(struct drm_client_dev *client, bool active, bool check)
{
	struct drm_device *dev = client->dev;
	struct drm_plane *plane;
	struct drm_atomic_state *state;
	struct drm_modeset_acquire_ctx ctx;
	struct drm_mode_set *mode_set;
	int ret;

	drm_modeset_acquire_init(&ctx, 0);

	state = drm_atomic_state_alloc(dev);
	if (!state) {
		ret = -ENOMEM;
		goto out_ctx;
	}

	state->acquire_ctx = &ctx;
retry:
	drm_client_for_each_modeset(mode_set, client) {
		unsigned int rotation;
		struct drm_crtc_state *crtc_state;
		struct drm_plane_state *plane_state;
		int hdisplay, vdisplay;

		#if 1
		drm_for_each_plane(plane, dev) {
			/* set the first overlay plane(osd0) to /dev/fb0 */
			if (plane->type == DRM_PLANE_TYPE_OVERLAY) {
				DRM_DEBUG("%s[%d]overlay plane index:%d\n", __func__, __LINE__, plane->index);
				if (plane->index == 1) {  /* for OSD0 */
					plane_state = drm_atomic_get_plane_state(state, plane);
					break;
				}
			}
		}
		#else
		/* default set primary plane to /dev/fb0 */
		plane_state = drm_atomic_get_plane_state(state, mode_set->crtc->primary);
		#endif
		if (IS_ERR(plane_state))
			goto out_state;

		if (drm_client_rotation(mode_set, &rotation)) {
			/* Cannot fail as we've already gotten the plane state above */
			plane_state->rotation = rotation;
		}

		//ret = __drm_atomic_helper_set_config(mode_set, state);
		crtc_state = drm_atomic_get_crtc_state(state, mode_set->crtc);
		if (IS_ERR(crtc_state))
			goto out_state;

		if (!mode_set->mode) {
			WARN_ON(mode_set->fb);
			WARN_ON(mode_set->num_connectors);

			ret = drm_atomic_set_mode_for_crtc(crtc_state, NULL);
			if (ret != 0)
				goto out_state;

			crtc_state->active = false;

			ret = drm_atomic_set_crtc_for_plane(plane_state, NULL);
			if (ret != 0)
				goto out_state;

			drm_atomic_set_fb_for_plane(plane_state, NULL);

			ret = update_output_state(state, mode_set);
			goto out_state;
		}

		WARN_ON(!mode_set->fb);
		WARN_ON(!mode_set->num_connectors);

		ret = drm_atomic_set_mode_for_crtc(crtc_state, mode_set->mode);
		if (ret != 0)
			goto out_state;

		crtc_state->active = true;

		ret = drm_atomic_set_crtc_for_plane(plane_state, mode_set->crtc);
		if (ret != 0)
			goto out_state;

		drm_mode_get_hv_timing(mode_set->mode, &hdisplay, &vdisplay);

		drm_atomic_set_fb_for_plane(plane_state, mode_set->fb);
		plane_state->crtc_x = 0;
		plane_state->crtc_y = 0;
		plane_state->crtc_w = hdisplay;
		plane_state->crtc_h = vdisplay;
		plane_state->src_x = mode_set->x << 16;
		plane_state->src_y = mode_set->y << 16;
		if (drm_rotation_90_or_270(plane_state->rotation)) {
			plane_state->src_w = vdisplay << 16;
			plane_state->src_h = hdisplay << 16;
		} else {
			plane_state->src_w = hdisplay << 16;
			plane_state->src_h = vdisplay << 16;
		}
		ret = update_output_state(state, mode_set);
		/*********__drm_atomic_helper_set_config end********/

		if (ret != 0)
			goto out_state;

		/*
		 * __drm_atomic_helper_set_config() sets active when a
		 * mode is set, unconditionally clear it if we force DPMS off
		 */
		if (!active) {
			struct drm_crtc *crtc = mode_set->crtc;
			struct drm_crtc_state *crtc_state = drm_atomic_get_new_crtc_state(state, crtc);

			crtc_state->active = false;
		}
	}

	if (check)
		ret = drm_atomic_check_only(state);
	else
		ret = drm_atomic_commit(state);

out_state:
	if (ret == -EDEADLK)
		goto backoff;

	drm_atomic_state_put(state);
out_ctx:
	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return ret;

backoff:
	drm_atomic_state_clear(state);
	drm_modeset_backoff(&ctx);

	goto retry;
}

/**
 * drm_fb_helper_set_par - implementation for &fb_ops.fb_set_par
 * @info: fbdev registered by the helper
 *
 * This will let fbcon do the mode init and is called at initialization time by
 * the fbdev core when registering the driver, and later on through the hotplug
 * callback.
 */
int sp7350_drm_fb_helper_set_par(struct fb_info *info)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct fb_var_screeninfo *var = &info->var;
	bool force;

	if (oops_in_progress)
		return -EBUSY;

	if (var->pixclock != 0) {
		drm_err(fb_helper->dev, "PIXEL CLOCK SET\n");
		return -EINVAL;
	}

	/*
	 * Normally we want to make sure that a kms master takes precedence over
	 * fbdev, to avoid fbdev flickering and occasionally stealing the
	 * display status. But Xorg first sets the vt back to text mode using
	 * the KDSET IOCTL with KD_TEXT, and only after that drops the master
	 * status when exiting.
	 *
	 * In the past this was caught by drm_fb_helper_lastclose(), but on
	 * modern systems where logind always keeps a drm fd open to orchestrate
	 * the vt switching, this doesn't work.
	 *
	 * To not break the userspace ABI we have this special case here, which
	 * is only used for the above case. Everything else uses the normal
	 * commit function, which ensures that we never steal the display from
	 * an active drm master.
	 */
	force = var->activate & FB_ACTIVATE_KD_TEXT;

	//__drm_fb_helper_restore_fbdev_mode_unlocked(fb_helper, force);
	{
		bool do_delayed;
		int ret;

		//if (!drm_fbdev_emulation || !fb_helper)
		//	return -ENODEV;

		if (READ_ONCE(fb_helper->deferred_setup))
			return 0;

		mutex_lock(&fb_helper->lock);
		if (force) {
			/*
			 * Yes this is the _locked version which expects the master lock
			 * to be held. But for forced restores we're intentionally
			 * racing here, see drm_fb_helper_set_par().
			 */
			//ret = drm_client_modeset_commit_locked(&fb_helper->client);
			{
				struct drm_client_dev *client = &fb_helper->client;

				mutex_lock(&client->modeset_mutex);
				ret = _drm_client_modeset_commit_atomic(client, true, false);
				mutex_unlock(&client->modeset_mutex);
			}
		} else {
			//ret = drm_client_modeset_commit(&fb_helper->client);
			{
				struct drm_client_dev *client = &fb_helper->client;

				if (!drm_master_internal_acquire(client->dev))
					return -EBUSY;

				mutex_lock(&client->modeset_mutex);
				ret = _drm_client_modeset_commit_atomic(client, true, false);
				mutex_unlock(&client->modeset_mutex);

				drm_master_internal_release(client->dev);
			}
		}

		do_delayed = fb_helper->delayed_hotplug;
		if (do_delayed)
			fb_helper->delayed_hotplug = false;
		mutex_unlock(&fb_helper->lock);

		if (do_delayed)
			drm_fb_helper_hotplug_event(fb_helper);
	}

	return 0;
}

/**
 * drm_fb_helper_pan_display - implementation for &fb_ops.fb_pan_display
 * @var: updated screen information
 * @info: fbdev registered by the helper
 */
int sp7350_drm_fbdev_pan_display(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct drm_fb_helper *fb_helper = info->par;
	struct drm_device *dev = fb_helper->dev;
	int ret;

	if (oops_in_progress)
		return -EBUSY;

	mutex_lock(&fb_helper->lock);
	if (!drm_master_internal_acquire(dev)) {
		ret = -EBUSY;
		goto unlock;
	}

	//ret = pan_display_atomic(var, info);
	pan_set(fb_helper, var->xoffset, var->yoffset);
	//ret = drm_client_modeset_commit_locked(&fb_helper->client);
	mutex_lock(&fb_helper->client.modeset_mutex);
	ret = _drm_client_modeset_commit_atomic(&fb_helper->client, true, false);
	mutex_unlock(&fb_helper->client.modeset_mutex);
	if (!ret) {
		info->var.xoffset = var->xoffset;
		info->var.yoffset = var->yoffset;
	} else
		pan_set(fb_helper, info->var.xoffset, info->var.yoffset);


	drm_master_internal_release(dev);
unlock:
	mutex_unlock(&fb_helper->lock);

	return ret;
}

static const struct fb_ops sp7350_drm_fbdev_fb_ops = {
	.owner		= THIS_MODULE,
	DRM_FB_HELPER_DEFAULT_OPS,
	.fb_mmap        = sp7350_drm_fbdev_fb_mmap,
	.fb_fillrect    = drm_fb_helper_cfb_fillrect,
	.fb_copyarea    = drm_fb_helper_cfb_copyarea,
	.fb_imageblit   = drm_fb_helper_cfb_imageblit,
	.fb_set_par     = sp7350_drm_fb_helper_set_par,
	.fb_pan_display = sp7350_drm_fbdev_pan_display,
};

static int sp7350_drm_fbdev_create(struct drm_fb_helper *fb_helper,
				    struct drm_fb_helper_surface_size *sizes)
{
	struct drm_client_dev *client = &fb_helper->client;
	struct drm_device *dev = fb_helper->dev;
	struct drm_client_buffer *buffer;
	struct drm_framebuffer *fb;
	struct fb_info *fbi;
	u32 format;
	void *vaddr;

	drm_info(dev, "surface width(%d), height(%d) and bpp(%d)\n",
		    sizes->surface_width, sizes->surface_height,
		    sizes->surface_bpp);

	format = drm_mode_legacy_fb_format(sizes->surface_bpp, sizes->surface_depth);
	buffer = drm_client_framebuffer_create(client, sizes->surface_width,
					       sizes->surface_height, format);
	if (IS_ERR(buffer))
		return PTR_ERR(buffer);

	fb_helper->buffer = buffer;
	fb_helper->fb = buffer->fb;
	fb = buffer->fb;

	fbi = drm_fb_helper_alloc_fbi(fb_helper);
	if (IS_ERR(fbi))
		return PTR_ERR(fbi);

	fbi->fbops = &sp7350_drm_fbdev_fb_ops;
	fbi->screen_size = fb->height * fb->pitches[0];
	fbi->fix.smem_len = fbi->screen_size;

	drm_fb_helper_fill_info(fbi, fb_helper, sizes);

	/* buffer is mapped for HW framebuffer */
	vaddr = drm_client_buffer_vmap(fb_helper->buffer);
	if (IS_ERR(vaddr))
		return PTR_ERR(vaddr);

	fbi->screen_buffer = vaddr;
	/* Shamelessly leak the physical address to user-space */
#if IS_ENABLED(CONFIG_DRM_FBDEV_LEAK_PHYS_SMEM)
	if (fbi->fix.smem_start == 0)
		fbi->fix.smem_start =
			page_to_phys(virt_to_page(fbi->screen_buffer));
#endif

	return 0;
}

static const struct drm_fb_helper_funcs sp7350_drm_fb_helper_funcs = {
	.fb_probe =	sp7350_drm_fbdev_create,
};

#if 1
static void sp7350_drm_fbdev_cleanup(struct drm_fb_helper *fb_helper)
{
	struct fb_info *fbi = fb_helper->fbdev;
	void *shadow = NULL;

	if (!fb_helper->dev)
		return;

	if (fbi && fbi->fbdefio) {
		fb_deferred_io_cleanup(fbi);
		shadow = fbi->screen_buffer;
	}

	drm_fb_helper_fini(fb_helper);

	vfree(shadow);

	drm_client_framebuffer_delete(fb_helper->buffer);
}

static void sp7350_drm_fbdev_release(struct drm_fb_helper *fb_helper)
{
	sp7350_drm_fbdev_cleanup(fb_helper);
	drm_client_release(&fb_helper->client);
	kfree(fb_helper);
}

static void sp7350_drm_fbdev_client_unregister(struct drm_client_dev *client)
{
	struct drm_fb_helper *fb_helper = drm_fb_helper_from_client(client);

	if (fb_helper->fbdev)
		/* drm_fbdev_fb_destroy() takes care of cleanup */
		drm_fb_helper_unregister_fbi(fb_helper);
	else
		sp7350_drm_fbdev_release(fb_helper);
}

static int sp7350_drm_fbdev_client_restore(struct drm_client_dev *client)
{
	drm_fb_helper_lastclose(client->dev);

	return 0;
}

static int sp7350_drm_fbdev_client_hotplug(struct drm_client_dev *client)
{
	struct drm_fb_helper *fb_helper = drm_fb_helper_from_client(client);
	struct drm_device *dev = client->dev;
	int ret;

	/* Setup is not retried if it has failed */
	if (!fb_helper->dev && fb_helper->funcs)
		return 0;

	if (dev->fb_helper)
		return drm_fb_helper_hotplug_event(dev->fb_helper);

	if (!dev->mode_config.num_connector) {
		drm_dbg_kms(dev, "No connectors found, will not create framebuffer!\n");
		return 0;
	}

	drm_fb_helper_prepare(dev, fb_helper, &sp7350_drm_fb_helper_funcs);

	ret = drm_fb_helper_init(dev, fb_helper);
	if (ret)
		goto err;

	if (!drm_drv_uses_atomic_modeset(dev))
		drm_helper_disable_unused_functions(dev);

	ret = drm_fb_helper_initial_config(fb_helper, fb_helper->preferred_bpp);
	if (ret)
		goto err_cleanup;

	return 0;

err_cleanup:
	sp7350_drm_fbdev_cleanup(fb_helper);
err:
	fb_helper->dev = NULL;
	fb_helper->fbdev = NULL;

	drm_err(dev, "fbdev: Failed to setup generic emulation (ret=%d)\n", ret);

	return ret;
}

static const struct drm_client_funcs sp7350_drm_fbdev_client_funcs = {
	.owner		= THIS_MODULE,
	.unregister	= sp7350_drm_fbdev_client_unregister,
	.restore	= sp7350_drm_fbdev_client_restore,
	.hotplug	= sp7350_drm_fbdev_client_hotplug,
};
#endif

int sp7350_drm_fbdev_init(struct drm_device *dev,
			     unsigned int preferred_bpp)
{
	struct drm_fb_helper *helper;
	int ret;

	if (!dev->mode_config.num_crtc)
		return 0;

	helper = kzalloc(sizeof(*helper), GFP_KERNEL);
	if (!helper) {
		drm_err(dev, "Failed to allocate fb_helper\n");
		return -1;
	}

#if 1
	ret = drm_client_init(dev, &helper->client, "fbdev", &sp7350_drm_fbdev_client_funcs);
	if (ret) {
		kfree(helper);
		drm_err(dev, "Failed to register client: %d\n", ret);
		return -1;
	}

	if (!preferred_bpp)
		preferred_bpp = dev->mode_config.preferred_depth;
	if (!preferred_bpp)
		preferred_bpp = 32;
	helper->preferred_bpp = preferred_bpp;
	//helper->preferred_bpp = 16;

	ret = sp7350_drm_fbdev_client_hotplug(&helper->client);
	if (ret)
		drm_dbg_kms(dev, "client hotplug ret=%d\n", ret);

	drm_client_register(&helper->client);
#else
	drm_fb_helper_prepare(dev, helper, &sp7350_drm_fb_helper_funcs);

	ret = drm_fb_helper_init(dev, helper);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev,
			      "failed to initialize drm fb helper.\n");
		return ret;
	}

	ret = drm_fb_helper_initial_config(helper, preferred_bpp);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev,
			      "failed to set up hw configuration. ret=%d\n", ret);
		goto err_setup;
	}
	return 0;

err_setup:
	drm_fb_helper_fini(helper);

#endif
	return ret;
}

#if 0
void sp7350_drm_fbdev_fini(struct drm_device *dev)
{
	struct drm_framebuffer *fb;

	/* release drm framebuffer and real buffer */
	if (dev->fb_helper->fb && dev->fb_helper->fb->funcs) {
		fb = dev->fb_helper->fb;
		if (fb)
			drm_framebuffer_remove(fb);
	}

	drm_fb_helper_unregister_fbi(dev->fb_helper);

	drm_fb_helper_fini(dev->fb_helper);
}
#endif
