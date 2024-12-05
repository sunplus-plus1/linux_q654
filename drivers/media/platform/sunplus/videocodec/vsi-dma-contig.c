/*
 *    VSI V4L2 decoder entry.
 *
 *    Copyright (c) 2019, VeriSilicon Inc.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License, version 2, as
 *    published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License version 2 for more details.
 *
 *    You may obtain a copy of the GNU General Public License
 *    Version 2 at the following locations:
 *    https://opensource.org/licenses/gpl-2.0.php
 */

#include <linux/dma-buf.h>
#include <linux/module.h>
#include <linux/refcount.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>

#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-memops.h>
#include <linux/version.h>
#include "vsi-dma-priv.h"
#if defined(CONFIG_SOC_SP7350)
static bool remap;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)

MODULE_IMPORT_NS(DMA_BUF);

/*********************************************/
/*        scatterlist table functions        */
/*********************************************/

static unsigned long vb2_dc_get_contiguous_size(struct sg_table *sgt)
{
	struct scatterlist *s;
	dma_addr_t expected = sg_dma_address(sgt->sgl);
	unsigned int i;
	unsigned long size = 0;

	for_each_sgtable_dma_sg(sgt, s, i) {
		if (sg_dma_address(s) != expected)
			break;
		expected += sg_dma_len(s);
		size += sg_dma_len(s);
	}
	return size;
}

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static void *vb2_dc_cookie(struct vb2_buffer *vb, void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	return &buf->dma_addr;
}

/*
 * This function may fail if:
 *
 * - dma_buf_vmap() fails
 *   E.g. due to lack of virtual mapping address space, or due to
 *   dmabuf->ops misconfiguration.
 *
 * - dma_vmap_noncontiguous() fails
 *   For instance, when requested buffer size is larger than totalram_pages().
 *   Relevant for buffers that use non-coherent memory.
 *
 * - Queue DMA attrs have DMA_ATTR_NO_KERNEL_MAPPING set
 *   Relevant for buffers that use coherent memory.
 */
static void *vb2_dc_vaddr(struct vb2_buffer *vb, void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	if (buf->vaddr)
		return buf->vaddr;

	if (buf->db_attach) {
		struct iosys_map map;

		if (!dma_buf_vmap(buf->db_attach->dmabuf, &map))
			buf->vaddr = map.vaddr;

		return buf->vaddr;
	}

	if (buf->non_coherent_mem)
		buf->vaddr = dma_vmap_noncontiguous(buf->dev, buf->size,
						    buf->dma_sgt);
	return buf->vaddr;
}

static unsigned int vb2_dc_num_users(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	return refcount_read(&buf->refcount);
}

static void vb2_dc_prepare(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;
#if defined(CONFIG_SOC_SP7350)
	if (remap) {
		dma_sync_single_for_device(buf->dev, buf->dma_addr, buf->size, buf->dma_dir);
		return;
	}
#endif

	/* This takes care of DMABUF and user-enforced cache sync hint */
	if (buf->vb->skip_cache_sync_on_prepare)
		return;

	if (!buf->non_coherent_mem)
		return;

	/* Non-coherent MMAP only */
	if (buf->vaddr)
		flush_kernel_vmap_range(buf->vaddr, buf->size);

	/* For both USERPTR and non-coherent MMAP */
	dma_sync_sgtable_for_device(buf->dev, sgt, buf->dma_dir);
}

static void vb2_dc_finish(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

#if defined(CONFIG_SOC_SP7350)
	if (remap) {
		dma_sync_single_for_cpu(buf->dev, buf->dma_addr, buf->size, buf->dma_dir);
		return;
	}
#endif
	/* This takes care of DMABUF and user-enforced cache sync hint */
	if (buf->vb->skip_cache_sync_on_finish)
		return;

	if (!buf->non_coherent_mem)
		return;

	/* Non-coherent MMAP only */
	if (buf->vaddr)
		invalidate_kernel_vmap_range(buf->vaddr, buf->size);

	/* For both USERPTR and non-coherent MMAP */
	dma_sync_sgtable_for_cpu(buf->dev, sgt, buf->dma_dir);
}

/*********************************************/
/*        callbacks for MMAP buffers         */
/*********************************************/

static void vb2_dc_put(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	if (!refcount_dec_and_test(&buf->refcount))
		return;

	if (buf->non_coherent_mem) {
		if (buf->vaddr)
			dma_vunmap_noncontiguous(buf->dev, buf->vaddr);
		dma_free_noncontiguous(buf->dev, buf->size,
				       buf->dma_sgt, buf->dma_dir);
	} else {
		if (buf->sgt_base) {
			sg_free_table(buf->sgt_base);
			kfree(buf->sgt_base);
		}
		dma_free_attrs(buf->dev, buf->size, buf->cookie,
			       buf->dma_addr, buf->attrs);
	}
	put_device(buf->dev);
	kfree(buf);
}

static int vb2_dc_alloc_coherent(struct vb2_dc_buf *buf)
{
	struct vb2_queue *q = buf->vb->vb2_queue;

	buf->cookie = dma_alloc_attrs(buf->dev,
				      buf->size,
				      &buf->dma_addr,
				      GFP_KERNEL | q->gfp_flags,
				      buf->attrs);
	if (!buf->cookie)
		return -ENOMEM;

	if (q->dma_attrs & DMA_ATTR_NO_KERNEL_MAPPING)
		return 0;

	buf->vaddr = buf->cookie;
	return 0;
}

static int vb2_dc_alloc_non_coherent(struct vb2_dc_buf *buf)
{
	struct vb2_queue *q = buf->vb->vb2_queue;

	buf->dma_sgt = dma_alloc_noncontiguous(buf->dev,
					       buf->size,
					       buf->dma_dir,
					       GFP_KERNEL | q->gfp_flags,
					       buf->attrs);
	if (!buf->dma_sgt)
		return -ENOMEM;

	buf->dma_addr = sg_dma_address(buf->dma_sgt->sgl);

	/*
	 * For non-coherent buffers the kernel mapping is created on demand
	 * in vb2_dc_vaddr().
	 */
	return 0;
}

static void *vb2_dc_alloc(struct vb2_buffer *vb,
			  struct device *dev,
			  unsigned long size)
{
	struct vb2_dc_buf *buf;
	int ret;

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	// dev_info (dev, "2. size = %lx\n", size);

	buf->attrs = vb->vb2_queue->dma_attrs;
	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->vb = vb;
	buf->non_coherent_mem = vb->vb2_queue->non_coherent_mem;

	buf->size = size;
	/* Prevent the device from being released while the buffer is used */
	buf->dev = get_device(dev);

	if (buf->non_coherent_mem)
		ret = vb2_dc_alloc_non_coherent(buf);
	else
		ret = vb2_dc_alloc_coherent(buf);

	if (ret) {
		dev_err(dev, "dma alloc of size %lu failed\n", size);
		kfree(buf);
		return ERR_PTR(-ENOMEM);
	}

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = vb2_dc_put;
	buf->handler.arg = buf;

	refcount_set(&buf->refcount, 1);

	return buf;
}

static int vb2_dc_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct vb2_dc_buf *buf = buf_priv;
	int ret;

	if (!buf) {
		printk(KERN_ERR "No buffer to map\n");
		return -EINVAL;
	}
#if defined(CONFIG_SOC_SP7350)
	if (remap) {
		vm_flags_set(vma, VM_LOCKED);
		if (remap_pfn_range(vma, vma->vm_start,
				    buf->dma_addr >> PAGE_SHIFT,
				    vma->vm_end - vma->vm_start,
				    vma->vm_page_prot)) {
			pr_err("%s(): remap_pfn_range() failed\n", __func__);
			return -ENOBUFS;
		}
	}
	else
#endif
	{
		if (buf->non_coherent_mem)
			ret = dma_mmap_noncontiguous(buf->dev, vma, buf->size,
						     buf->dma_sgt);
		else
			ret = dma_mmap_attrs(buf->dev, vma, buf->cookie, buf->dma_addr,
					     buf->size, buf->attrs);
		if (ret) {
			pr_err("Remapping memory failed, error: %d\n", ret);
			return ret;
		}
	}

	vma->vm_flags		|= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data	= &buf->handler;
	vma->vm_ops		= &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	pr_debug("%s: mapped dma addr 0x%08lx at 0x%08lx, size %lu\n",
		 __func__, (unsigned long)buf->dma_addr, vma->vm_start,
		 buf->size);

	return 0;
}

/*********************************************/
/*         DMABUF ops for exporters          */
/*********************************************/

struct vb2_dc_attachment {
	struct sg_table sgt;
	enum dma_data_direction dma_dir;
};

static int vb2_dc_dmabuf_ops_attach(struct dma_buf *dbuf,
	struct dma_buf_attachment *dbuf_attach)
{
	struct vb2_dc_attachment *attach;
	unsigned int i;
	struct scatterlist *rd, *wr;
	struct sg_table *sgt;
	struct vb2_dc_buf *buf = dbuf->priv;
	int ret;

	attach = kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach)
		return -ENOMEM;

	sgt = &attach->sgt;
	/* Copy the buf->base_sgt scatter list to the attachment, as we can't
	 * map the same scatter list to multiple attachments at the same time.
	 */
	ret = sg_alloc_table(sgt, buf->sgt_base->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(attach);
		return -ENOMEM;
	}

	rd = buf->sgt_base->sgl;
	wr = sgt->sgl;
	for (i = 0; i < sgt->orig_nents; ++i) {
		sg_set_page(wr, sg_page(rd), rd->length, rd->offset);
		rd = sg_next(rd);
		wr = sg_next(wr);
	}

	attach->dma_dir = DMA_NONE;
	dbuf_attach->priv = attach;

	return 0;
}

static void vb2_dc_dmabuf_ops_detach(struct dma_buf *dbuf,
	struct dma_buf_attachment *db_attach)
{
	struct vb2_dc_attachment *attach = db_attach->priv;
	struct sg_table *sgt;

	if (!attach)
		return;

	sgt = &attach->sgt;

	/* release the scatterlist cache */
	if (attach->dma_dir != DMA_NONE)
		/*
		 * Cache sync can be skipped here, as the vb2_dc memory is
		 * allocated from device coherent memory, which means the
		 * memory locations do not require any explicit cache
		 * maintenance prior or after being used by the device.
		 */
		dma_unmap_sgtable(db_attach->dev, sgt, attach->dma_dir,
				  DMA_ATTR_SKIP_CPU_SYNC);
	sg_free_table(sgt);
	kfree(attach);
	db_attach->priv = NULL;
}

static struct sg_table *vb2_dc_dmabuf_ops_map(
	struct dma_buf_attachment *db_attach, enum dma_data_direction dma_dir)
{
	struct vb2_dc_attachment *attach = db_attach->priv;
	/* stealing dmabuf mutex to serialize map/unmap operations */
	struct mutex *lock = &db_attach->dmabuf->lock;
	struct sg_table *sgt;

	mutex_lock(lock);

	sgt = &attach->sgt;
	/* return previously mapped sg table */
	if (attach->dma_dir == dma_dir) {
		mutex_unlock(lock);
		return sgt;
	}

	/* release any previous cache */
	if (attach->dma_dir != DMA_NONE) {
		dma_unmap_sgtable(db_attach->dev, sgt, attach->dma_dir,
				  DMA_ATTR_SKIP_CPU_SYNC);
		attach->dma_dir = DMA_NONE;
	}

	/*
	 * mapping to the client with new direction, no cache sync
	 * required see comment in vb2_dc_dmabuf_ops_detach()
	 */
	if (dma_map_sgtable(db_attach->dev, sgt, dma_dir,
			    DMA_ATTR_SKIP_CPU_SYNC)) {
		pr_err("failed to map scatterlist\n");
		mutex_unlock(lock);
		return ERR_PTR(-EIO);
	}

	attach->dma_dir = dma_dir;

	mutex_unlock(lock);

	return sgt;
}

static void vb2_dc_dmabuf_ops_unmap(struct dma_buf_attachment *db_attach,
	struct sg_table *sgt, enum dma_data_direction dma_dir)
{
	/* nothing to be done here */
}

static void vb2_dc_dmabuf_ops_release(struct dma_buf *dbuf)
{
	/* drop reference obtained in vb2_dc_get_dmabuf */
	vb2_dc_put(dbuf->priv);
}

static int
vb2_dc_dmabuf_ops_begin_cpu_access(struct dma_buf *dbuf,
				   enum dma_data_direction direction)
{
	return 0;
}

static int
vb2_dc_dmabuf_ops_end_cpu_access(struct dma_buf *dbuf,
				 enum dma_data_direction direction)
{
	return 0;
}

static int vb2_dc_dmabuf_ops_vmap(struct dma_buf *dbuf, struct iosys_map *map)
{
	struct vb2_dc_buf *buf;
	void *vaddr;

	buf = dbuf->priv;
	vaddr = vb2_dc_vaddr(buf->vb, buf);
	if (!vaddr)
		return -EINVAL;

	iosys_map_set_vaddr(map, vaddr);

	return 0;
}

static int vb2_dc_dmabuf_ops_mmap(struct dma_buf *dbuf,
	struct vm_area_struct *vma)
{
	return vb2_dc_mmap(dbuf->priv, vma);
}

static const struct dma_buf_ops vb2_dc_dmabuf_ops = {
	.attach = vb2_dc_dmabuf_ops_attach,
	.detach = vb2_dc_dmabuf_ops_detach,
	.map_dma_buf = vb2_dc_dmabuf_ops_map,
	.unmap_dma_buf = vb2_dc_dmabuf_ops_unmap,
	.begin_cpu_access = vb2_dc_dmabuf_ops_begin_cpu_access,
	.end_cpu_access = vb2_dc_dmabuf_ops_end_cpu_access,
	.vmap = vb2_dc_dmabuf_ops_vmap,
	.mmap = vb2_dc_dmabuf_ops_mmap,
	.release = vb2_dc_dmabuf_ops_release,
};

static struct sg_table *vb2_dc_get_base_sgt(struct vb2_dc_buf *buf)
{
	int ret;
	struct sg_table *sgt;

	if (buf->non_coherent_mem)
		return buf->dma_sgt;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		dev_err(buf->dev, "failed to alloc sg table\n");
		return NULL;
	}

	ret = dma_get_sgtable_attrs(buf->dev, sgt, buf->cookie, buf->dma_addr,
		buf->size, buf->attrs);
	if (ret < 0) {
		dev_err(buf->dev, "failed to get scatterlist from DMA API\n");
		kfree(sgt);
		return NULL;
	}

	return sgt;
}

static struct dma_buf *vb2_dc_get_dmabuf(struct vb2_buffer *vb,
					 void *buf_priv,
					 unsigned long flags)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct dma_buf *dbuf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

	exp_info.ops = &vb2_dc_dmabuf_ops;
	exp_info.size = buf->size;
	exp_info.flags = flags;
	exp_info.priv = buf;

	if (!buf->sgt_base)
		buf->sgt_base = vb2_dc_get_base_sgt(buf);

	if (WARN_ON(!buf->sgt_base))
		return NULL;

	dbuf = dma_buf_export(&exp_info);
	if (IS_ERR(dbuf))
		return NULL;

	/* dmabuf keeps reference to vb2 buffer */
	refcount_inc(&buf->refcount);

	return dbuf;
}

/*********************************************/
/*       callbacks for USERPTR buffers       */
/*********************************************/

static void vb2_dc_put_userptr(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;
	int i;
	struct page **pages;

	if (sgt) {
		/*
		 * No need to sync to CPU, it's already synced to the CPU
		 * since the finish() memop will have been called before this.
		 */
		dma_unmap_sgtable(buf->dev, sgt, buf->dma_dir,
				  DMA_ATTR_SKIP_CPU_SYNC);
		pages = frame_vector_pages(buf->vec);
		/* sgt should exist only if vector contains pages... */
		BUG_ON(IS_ERR(pages));
		if (buf->dma_dir == DMA_FROM_DEVICE ||
		    buf->dma_dir == DMA_BIDIRECTIONAL)
			for (i = 0; i < frame_vector_count(buf->vec); i++)
				set_page_dirty_lock(pages[i]);
		sg_free_table(sgt);
		kfree(sgt);
	} else {
		dma_unmap_resource(buf->dev, buf->dma_addr, buf->size,
				   buf->dma_dir, 0);
	}
	vb2_destroy_framevec(buf->vec);
	kfree(buf);
}
#if 0
static int vsi_pin_user_pages_fast(unsigned long start, int nr_pages,
					unsigned int gup_flags, struct page **pages)
{
	unsigned long len, end;

	gup_flags |= FOLL_PIN;

	if (!test_bit(MMF_HAS_PINNED, &current->mm->flags))
		set_bit(MMF_HAS_PINNED, &current->mm->flags);

	if (!(gup_flags & FOLL_FAST_ONLY))
		might_lock_read(&current->mm->mmap_lock);

	start = untagged_addr(start) & PAGE_MASK;
	len = nr_pages << PAGE_SHIFT;
	if (check_add_overflow(start, len, &end)) {
		pr_err("overflow: %lx:%ld:%ld", start, len, end);
		return 0;
	}
	if (unlikely(!access_ok((void __user *)start, len))) {
		pr_err("cant access %lx:%ld", start, len);
		return -EFAULT;
	}
	return nr_pages;
}
#endif
static int vsi_get_vaddr_frames(unsigned long start, unsigned int nr_frames,
				     struct frame_vector *vec)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	int ret = 0;
	int err;
	unsigned int gup_flags = FOLL_FORCE | FOLL_WRITE;

	if (nr_frames == 0)
		return 0;

	if (WARN_ON_ONCE(nr_frames > vec->nr_allocated))
		nr_frames = vec->nr_allocated;

	start = untagged_addr(start);

	vma = find_vma_intersection(mm, start, start + 1);
	if (!vma) {
		pr_err("%s can't find vma, flag=%lx ", __func__, vma->vm_flags);
		ret = -EFAULT;
		goto out;
	}

	if (vma_is_fsdax(vma)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (!(vma->vm_flags & (VM_IO | VM_PFNMAP))) {
		vec->got_ref = true;
		vec->is_pfns = false;
		ret = pin_user_pages_unlocked(start, nr_frames, (struct page **)(vec->ptrs), gup_flags);
		pr_info("pin user page unlocked=%d", ret);
		goto out;
	}

	vec->got_ref = false;
	vec->is_pfns = true;
	do {
		unsigned long *nums = frame_vector_pfns(vec);
		while (ret < nr_frames && start + PAGE_SIZE <= vma->vm_end) {
			err = follow_pfn(vma, start, &nums[ret]);
			if (err) {
				pr_err("err=%d at %d page", err, ret);
				if (ret == 0)
					ret = err;
				goto out;
			}
			start += PAGE_SIZE;
			ret++;
		}
		if (ret >= nr_frames || start < vma->vm_end)
			break;
		vma = find_vma_intersection(mm, start, start + 1);
	} while (vma && vma->vm_flags & (VM_IO | VM_PFNMAP));
	pr_info("%s ret = %d", __func__, ret);
out:
	if (!ret)
		ret = -EFAULT;
	if (ret > 0)
		vec->nr_frames = ret;
	return ret;
}

static struct frame_vector *vsi_create_framevec(unsigned long start,
							 unsigned long length)
{
	int ret;
	unsigned long first, last;
	unsigned long nr;
	struct frame_vector *vec;

	pr_info("%s:%lx:%lx", __func__, start, length);
	first = start >> PAGE_SHIFT;
	last = (start + length - 1) >> PAGE_SHIFT;
	nr = last - first + 1;
	vec = frame_vector_create(nr);
	if (!vec)
		return ERR_PTR(-ENOMEM);
	ret = vsi_get_vaddr_frames(start & PAGE_MASK, nr, vec);
	if (ret < 0) {
		pr_err("%s:%d:%ld err=%d", __func__, __LINE__, nr, ret);
		goto out_destroy;
	}
	/* We accept only complete set of PFNs */
	if (ret != nr) {
		pr_err("%s:%d %d!=%ld", __func__, __LINE__, ret, nr);
		ret = -EFAULT;
		goto out_release;
	}
	return vec;
out_release:
	put_vaddr_frames(vec);
out_destroy:
	frame_vector_destroy(vec);
	return ERR_PTR(ret);
}


static void *vb2_dc_get_userptr(struct vb2_buffer *vb, struct device *dev,
				unsigned long vaddr, unsigned long size)
{
	struct vb2_dc_buf *buf;
	struct frame_vector *vec;
	unsigned int offset;
	int n_pages, i;
	int ret = 0;
	struct sg_table *sgt;
	unsigned long contig_size;
	unsigned long dma_align = dma_get_cache_alignment();

	/* Only cache aligned DMA transfers are reliable */
	if (!IS_ALIGNED(vaddr | size, dma_align)) {
		pr_debug("user data must be aligned to %lu bytes\n", dma_align);
		return ERR_PTR(-EINVAL);
	}

	if (!size) {
		pr_debug("size is zero\n");
		return ERR_PTR(-EINVAL);
	}

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->vb = vb;

	offset = lower_32_bits(offset_in_page(vaddr));
	vec = vsi_create_framevec(vaddr, size);
	if (IS_ERR(vec)) {
		ret = PTR_ERR(vec);
		pr_err("create_framevec fail err=%d", ret);
		goto fail_buf;
	}
	buf->vec = vec;
	n_pages = frame_vector_count(vec);
	ret = frame_vector_to_pages(vec);
	if (ret < 0) {
		unsigned long *nums = frame_vector_pfns(vec);
		pr_err("vec to page %d fail err=%d", n_pages, ret);

		/*
		 * Failed to convert to pages... Check the memory is physically
		 * contiguous and use direct mapping
		 */
		for (i = 1; i < n_pages; i++)
			if (nums[i-1] + 1 != nums[i])
				goto fail_pfnvec;
		buf->dma_addr = dma_map_resource(buf->dev,
				__pfn_to_phys(nums[0]), size, buf->dma_dir, 0);
		if (dma_mapping_error(buf->dev, buf->dma_addr)) {
			ret = -ENOMEM;
			goto fail_pfnvec;
		}
		goto out;
	}

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		pr_err("failed to allocate sg table\n");
		ret = -ENOMEM;
		goto fail_pfnvec;
	}

	ret = sg_alloc_table_from_pages(sgt, frame_vector_pages(vec), n_pages,
		offset, size, GFP_KERNEL);
	if (ret) {
		pr_err("failed to initialize sg table\n");
		goto fail_sgt;
	}

	/*
	 * No need to sync to the device, this will happen later when the
	 * prepare() memop is called.
	 */
	if (dma_map_sgtable(buf->dev, sgt, buf->dma_dir,
			    DMA_ATTR_SKIP_CPU_SYNC)) {
		pr_err("failed to map scatterlist\n");
		ret = -EIO;
		goto fail_sgt_init;
	}

	contig_size = vb2_dc_get_contiguous_size(sgt);
	if (contig_size < size) {
		pr_err("contiguous mapping is too small %lu/%lu\n",
			contig_size, size);
		ret = -EFAULT;
		goto fail_map_sg;
	}

	buf->dma_addr = sg_dma_address(sgt->sgl);
	buf->dma_sgt = sgt;
	buf->non_coherent_mem = 1;

out:
	buf->size = size;

	return buf;

fail_map_sg:
	dma_unmap_sgtable(buf->dev, sgt, buf->dma_dir, DMA_ATTR_SKIP_CPU_SYNC);

fail_sgt_init:
	sg_free_table(sgt);

fail_sgt:
	kfree(sgt);

fail_pfnvec:
	vb2_destroy_framevec(vec);

fail_buf:
	kfree(buf);

	return ERR_PTR(ret);
}

/*********************************************/
/*       callbacks for DMABUF buffers        */
/*********************************************/

static int vb2_dc_map_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;
	struct sg_table *sgt;
	unsigned long contig_size;

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to pin a non attached buffer\n");
		return -EINVAL;
	}

	if (WARN_ON(buf->dma_sgt)) {
		pr_err("dmabuf buffer is already pinned\n");
		return 0;
	}

	/* get the associated scatterlist for this buffer */
	sgt = dma_buf_map_attachment(buf->db_attach, buf->dma_dir);
	if (IS_ERR(sgt)) {
		pr_err("Error getting dmabuf scatterlist\n");
		return -EINVAL;
	}

	/* checking if dmabuf is big enough to store contiguous chunk */
	contig_size = vb2_dc_get_contiguous_size(sgt);
	if (contig_size < buf->size) {
		pr_err("contiguous chunk is too small %lu/%lu\n",
		       contig_size, buf->size);
		dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);
		return -EFAULT;
	}

	buf->dma_addr = sg_dma_address(sgt->sgl);
	buf->dma_sgt = sgt;
	buf->vaddr = NULL;

	return 0;
}

static void vb2_dc_unmap_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;
	struct sg_table *sgt = buf->dma_sgt;
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(buf->vaddr);

	if (WARN_ON(!buf->db_attach)) {
		pr_err("trying to unpin a not attached buffer\n");
		return;
	}

	if (WARN_ON(!sgt)) {
		pr_err("dmabuf buffer is already unpinned\n");
		return;
	}

	if (buf->vaddr) {
		dma_buf_vunmap(buf->db_attach->dmabuf, &map);
		buf->vaddr = NULL;
	}
	dma_buf_unmap_attachment(buf->db_attach, sgt, buf->dma_dir);

	buf->dma_addr = 0;
	buf->dma_sgt = NULL;
}

static void vb2_dc_detach_dmabuf(void *mem_priv)
{
	struct vb2_dc_buf *buf = mem_priv;

	/* if vb2 works correctly you should never detach mapped buffer */
	if (WARN_ON(buf->dma_addr))
		vb2_dc_unmap_dmabuf(buf);

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static void *vb2_dc_attach_dmabuf(struct vb2_buffer *vb, struct device *dev,
				  struct dma_buf *dbuf, unsigned long size)
{
	struct vb2_dc_buf *buf;
	struct dma_buf_attachment *dba;

	if (dbuf->size < size) {
		pr_err("dbuf size %ld < size %ld", dbuf->size, size);
		return ERR_PTR(-EFAULT);
	}

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	buf->vb = vb;

	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		pr_err("failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = vb->vb2_queue->dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

/*********************************************/
/*       DMA CONTIG exported functions       */
/*********************************************/

static const struct vb2_mem_ops vsi_dma_contig_memops = {
	.alloc		= vb2_dc_alloc,
	.put		= vb2_dc_put,
	.get_dmabuf	= vb2_dc_get_dmabuf,
	.cookie		= vb2_dc_cookie,
	.vaddr		= vb2_dc_vaddr,
	.mmap		= vb2_dc_mmap,
	.get_userptr	= vb2_dc_get_userptr,
	.put_userptr	= vb2_dc_put_userptr,
	.prepare	= vb2_dc_prepare,
	.finish		= vb2_dc_finish,
	.map_dmabuf	= vb2_dc_map_dmabuf,
	.unmap_dmabuf	= vb2_dc_unmap_dmabuf,
	.attach_dmabuf	= vb2_dc_attach_dmabuf,
	.detach_dmabuf	= vb2_dc_detach_dmabuf,
	.num_users	= vb2_dc_num_users,
};

const struct vb2_mem_ops *get_vsi_mmop(void)
{
	return &vsi_dma_contig_memops;
}

#else //KERNEL_VERSION >= (6, 1, 0)

#include <linux/dma-buf.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-memops.h>
//
static struct vb2_mem_ops vsi_dma_contig_memops;

/*********************************************/
/*        scatterlist table functions        */
/*********************************************/

static void vb2_dc_prepare(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	dma_sync_single_for_device(buf->dev, buf->dma_addr, buf->size, buf->dma_dir);
	return;
}

static void vb2_dc_finish(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	dma_sync_single_for_cpu(buf->dev, buf->dma_addr, buf->size, buf->dma_dir);
	return;
}

static int vb2_dc_mmap(void *buf_priv, struct vm_area_struct *vma)
{
	struct vb2_dc_buf *buf = buf_priv;
	int ret;

	if (!buf) {
		printk(KERN_ERR "No buffer to map\n");
		return -EINVAL;
	}

	vma->vm_flags |= VM_LOCKED;
	if (remap_pfn_range(vma, vma->vm_start,
			    buf->dma_addr >> PAGE_SHIFT,
			    vma->vm_end - vma->vm_start,
			    vma->vm_page_prot)) {
		pr_err("%s(): remap_pfn_range() failed\n", __func__);
		return -ENOBUFS;
	}

	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data	= &buf->handler;
	vma->vm_ops		= &vb2_common_vm_ops;

	vma->vm_ops->open(vma);

	pr_debug("%s: mapped dma addr 0x%08lx at 0x%08lx, size %lu\n",
		 __func__, (unsigned long)buf->dma_addr, vma->vm_start,
		 buf->size);

	return 0;
}

const struct vb2_mem_ops *get_vsi_mmop(void)
{
#if defined(CONFIG_SOC_SP7350)
	if(remap){
		memcpy(&vsi_dma_contig_memops, &vb2_dma_contig_memops, sizeof(vb2_dma_contig_memops));

		vsi_dma_contig_memops.mmap = vb2_dc_mmap;
		vsi_dma_contig_memops.prepare = vb2_dc_prepare;
		vsi_dma_contig_memops.finish = vb2_dc_finish;

		return &vsi_dma_contig_memops;
	}
#endif

	return &vb2_dma_contig_memops;
}
#endif //KERNEL_VERSION >= (6, 1, 0)




