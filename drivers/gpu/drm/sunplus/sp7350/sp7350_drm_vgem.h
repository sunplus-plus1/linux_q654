/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM vGEM
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */


#ifndef __SUNPLUS_SP7350_DRM_VGEM_H__
#define __SUNPLUS_SP7350_DRM_VGEM_H__

#include <drm/drm.h>
#include <drm/drm_gem.h>


struct sp7350_vgem_object {
	struct drm_gem_object gem;
	struct mutex pages_lock; /* Page lock used in page fault handler */
	struct page **pages;
	unsigned int vmap_count;
	void *vaddr;
};

#define drm_gem_to_vgem(target)\
	container_of(target, struct sp7350_vgem_object, gem)


/* Gem stuff */
vm_fault_t sp7350_vgem_fault(struct vm_fault *vmf);

int sp7350_vgem_dumb_create(struct drm_file *file, struct drm_device *dev,
		     struct drm_mode_create_dumb *args);

void sp7350_vgem_free_object(struct drm_gem_object *obj);

int sp7350_vgem_vmap(struct drm_gem_object *obj);

void sp7350_vgem_vunmap(struct drm_gem_object *obj);


/* Prime */
struct drm_gem_object *
sp7350_vgem_prime_import_sg_table(struct drm_device *dev,
			   struct dma_buf_attachment *attach,
			   struct sg_table *sg);

#endif /* __SUNPLUS_SP7350_DRM_VGEM_H__ */
