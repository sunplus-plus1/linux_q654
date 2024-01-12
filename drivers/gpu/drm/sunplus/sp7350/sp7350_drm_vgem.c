/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Sunplus SP7350 SoC DRM vGEM
 *
 * Author: dx.jiang<dx.jiang@sunmedia.com.cn>
 */

#include <linux/dma-buf.h>
#include <linux/shmem_fs.h>
#include <linux/vmalloc.h>
#include <drm/drm_prime.h>

#include "sp7350_drm_vgem.h"

static struct sp7350_vgem_object *__sp7350_vgem_create(struct drm_device *dev,
						 u64 size)
{
	struct sp7350_vgem_object *obj;
	int ret;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return ERR_PTR(-ENOMEM);

	size = roundup(size, PAGE_SIZE);
	ret = drm_gem_object_init(dev, &obj->gem, size);
	if (ret) {
		kfree(obj);
		return ERR_PTR(ret);
	}

	mutex_init(&obj->pages_lock);

	return obj;
}

void sp7350_vgem_free_object(struct drm_gem_object *obj)
{
	struct sp7350_vgem_object *gem = container_of(obj, struct sp7350_vgem_object,
						   gem);

	WARN_ON(gem->pages);
	WARN_ON(gem->vaddr);

	mutex_destroy(&gem->pages_lock);
	drm_gem_object_release(obj);
	kfree(gem);
}

vm_fault_t sp7350_vgem_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct sp7350_vgem_object *obj = vma->vm_private_data;
	unsigned long vaddr = vmf->address;
	pgoff_t page_offset;
	loff_t num_pages;
	vm_fault_t ret = VM_FAULT_SIGBUS;

	page_offset = (vaddr - vma->vm_start) >> PAGE_SHIFT;
	num_pages = DIV_ROUND_UP(obj->gem.size, PAGE_SIZE);

	if (page_offset > num_pages)
		return VM_FAULT_SIGBUS;

	mutex_lock(&obj->pages_lock);
	if (obj->pages) {
		get_page(obj->pages[page_offset]);
		vmf->page = obj->pages[page_offset];
		ret = 0;
	}
	mutex_unlock(&obj->pages_lock);
	if (ret) {
		struct page *page;
		struct address_space *mapping;

		mapping = file_inode(obj->gem.filp)->i_mapping;
		page = shmem_read_mapping_page(mapping, page_offset);

		if (!IS_ERR(page)) {
			vmf->page = page;
			ret = 0;
		} else {
			switch (PTR_ERR(page)) {
			case -ENOSPC:
			case -ENOMEM:
				ret = VM_FAULT_OOM;
				break;
			case -EBUSY:
				ret = VM_FAULT_RETRY;
				break;
			case -EFAULT:
			case -EINVAL:
				ret = VM_FAULT_SIGBUS;
				break;
			default:
				WARN_ON(PTR_ERR(page));
				ret = VM_FAULT_SIGBUS;
				break;
			}
		}
	}
	return ret;
}

static struct drm_gem_object *sp7350_vgem_create(struct drm_device *dev,
					      struct drm_file *file,
					      u32 *handle,
					      u64 size)
{
	struct sp7350_vgem_object *obj;
	int ret;

	if (!file || !dev || !handle)
		return ERR_PTR(-EINVAL);

	obj = __sp7350_vgem_create(dev, size);
	if (IS_ERR(obj))
		return ERR_CAST(obj);

	ret = drm_gem_handle_create(file, &obj->gem, handle);
	if (ret)
		return ERR_PTR(ret);

	return &obj->gem;
}

int sp7350_vgem_dumb_create(struct drm_file *file, struct drm_device *dev,
		     struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *gem_obj;
	u64 pitch, size;

	if (!args || !dev || !file)
		return -EINVAL;

	pitch = args->width * DIV_ROUND_UP(args->bpp, 8);
	size = pitch * args->height;

	if (!size)
		return -EINVAL;

	gem_obj = sp7350_vgem_create(dev, file, &args->handle, size);
	if (IS_ERR(gem_obj))
		return PTR_ERR(gem_obj);

	args->size = gem_obj->size;
	args->pitch = pitch;

	drm_gem_object_put(gem_obj);

	DRM_DEBUG_DRIVER("Created object of size %lld\n", size);

	return 0;
}

static struct page **_get_pages(struct sp7350_vgem_object *vgem_obj)
{
	struct drm_gem_object *gem_obj = &vgem_obj->gem;

	if (!vgem_obj->pages) {
		struct page **pages = drm_gem_get_pages(gem_obj);

		if (IS_ERR(pages))
			return pages;

		if (cmpxchg(&vgem_obj->pages, NULL, pages))
			drm_gem_put_pages(gem_obj, pages, false, true);
	}

	return vgem_obj->pages;
}

void sp7350_vgem_vunmap(struct drm_gem_object *obj)
{
	struct sp7350_vgem_object *vgem_obj = drm_gem_to_vgem(obj);

	mutex_lock(&vgem_obj->pages_lock);
	if (vgem_obj->vmap_count < 1) {
		WARN_ON(vgem_obj->vaddr);
		WARN_ON(vgem_obj->pages);
		mutex_unlock(&vgem_obj->pages_lock);
		return;
	}

	vgem_obj->vmap_count--;

	if (vgem_obj->vmap_count == 0) {
		vunmap(vgem_obj->vaddr);
		vgem_obj->vaddr = NULL;
		drm_gem_put_pages(obj, vgem_obj->pages, false, true);
		vgem_obj->pages = NULL;
	}

	mutex_unlock(&vgem_obj->pages_lock);
}

int sp7350_vgem_vmap(struct drm_gem_object *obj)
{
	struct sp7350_vgem_object *vgem_obj = drm_gem_to_vgem(obj);
	int ret = 0;

	mutex_lock(&vgem_obj->pages_lock);

	if (!vgem_obj->vaddr) {
		unsigned int n_pages = obj->size >> PAGE_SHIFT;
		struct page **pages = _get_pages(vgem_obj);

		if (IS_ERR(pages)) {
			ret = PTR_ERR(pages);
			goto out;
		}

		vgem_obj->vaddr = vmap(pages, n_pages, VM_MAP, PAGE_KERNEL);
		if (!vgem_obj->vaddr)
			goto err_vmap;
	}

	vgem_obj->vmap_count++;
	goto out;

err_vmap:
	ret = -ENOMEM;
	drm_gem_put_pages(obj, vgem_obj->pages, false, true);
	vgem_obj->pages = NULL;
out:
	mutex_unlock(&vgem_obj->pages_lock);
	return ret;
}

struct drm_gem_object *
sp7350_vgem_prime_import_sg_table(struct drm_device *dev,
			   struct dma_buf_attachment *attach,
			   struct sg_table *sg)
{
	struct sp7350_vgem_object *obj;
	int npages;

	obj = __sp7350_vgem_create(dev, attach->dmabuf->size);
	if (IS_ERR(obj))
		return ERR_CAST(obj);

	npages = PAGE_ALIGN(attach->dmabuf->size) / PAGE_SIZE;
	DRM_DEBUG_PRIME("Importing %d pages\n", npages);

	obj->pages = kvmalloc_array(npages, sizeof(struct page *), GFP_KERNEL);
	if (!obj->pages) {
		sp7350_vgem_free_object(&obj->gem);
		return ERR_PTR(-ENOMEM);
	}

	drm_prime_sg_to_page_addr_arrays(sg, obj->pages, NULL, npages);
	return &obj->gem;
}
