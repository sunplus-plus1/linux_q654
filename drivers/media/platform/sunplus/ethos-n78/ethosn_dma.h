/*
 *
 * (C) COPYRIGHT 2018-2021 Arm Limited.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#ifndef _ETHOSN_DMA_H_
#define _ETHOSN_DMA_H_

#include <linux/dma-mapping.h>
#include <linux/types.h>

#define ETHOSN_REGION_MASK DMA_BIT_MASK(REGION_SHIFT)

#define ETHOSN_PROT_READ (1 << 0)
#define ETHOSN_PROT_WRITE (1 << 1)

struct device;
struct vm_area_struct;

/**
 * Streams identifier
 */
enum  ethosn_stream_id {
	ETHOSN_STREAM_FIRMWARE       = 0,
	ETHOSN_STREAM_WORKING_DATA   = 1,
	ETHOSN_STREAM_COMMAND_STREAM = 2,
	ETHOSN_STREAM_DMA            = 3,
};

/*
 * Used to save the result of dma_alloc calls for matching dma_free calls.
 * Also, iova_addr is used to set the buffer table for inferences.
 */
struct ethosn_dma_info {
	size_t     size;
	void       *cpu_addr;
	dma_addr_t iova_addr;
};

/**
 * struct ethosn_dma_allocator_ops - Allocator operations for DMA memory
 * @ops                Allocator operations
 * @dev                Device bound to the allocator
 */
struct ethosn_dma_allocator {
	const struct ethosn_dma_allocator_ops *ops;
	struct device                         *dev;
};

/**
 * struct ethosn_dma_allocator_ops - Allocator operations for DMA memory
 * @destroy:           Deinitialize the allocator and free private resources
 * @alloc:             Allocate DMA memory
 * @free               Free DMA memory allocated with alloc
 * @map                Map virtual addresses
 * @import             Attach and map the buffer into the device memory space
 * @release            Release and unmap the buffer from the device memory space
 * @unmap              Unmap virtual addresses
 * @sync_for_device    Transfer ownership of the memory buffer to the Ethos-N by
 *                     flushing the CPU cache
 * @sync_for_cpu       Transfer ownership of the memory buffer to the CPU by
 *                     invalidating the CPU cache
 * @mmap               Memory map the buffer into userspace
 * @get_addr_base      Get address base
 * @get_addr_size      Get address size
 */
struct ethosn_dma_allocator_ops {
	void                   (*destroy)(
		struct ethosn_dma_allocator *allocator);

	struct ethosn_dma_info *(*alloc)(struct ethosn_dma_allocator *allocator,
					 size_t size,
					 gfp_t gfp);
	int                    (*map)(struct ethosn_dma_allocator *allocator,
				      struct ethosn_dma_info *dma_info,
				      int prot,
				      enum ethosn_stream_id stream_id);
	struct ethosn_dma_info *(*import)(struct ethosn_dma_allocator *
					  allocator,
					  int fd,
					  size_t size);
	void (*release)(struct ethosn_dma_allocator *
			allocator,
			struct ethosn_dma_info *const
			dma_info);
	void (*unmap)(struct ethosn_dma_allocator *allocator,
		      struct ethosn_dma_info *dma_info,
		      enum ethosn_stream_id stream_id);
	void (*free)(struct ethosn_dma_allocator *allocator,
		     struct ethosn_dma_info *dma_info);
	void (*sync_for_device)(struct ethosn_dma_allocator *
				allocator,
				struct ethosn_dma_info *
				dma_info);
	void (*sync_for_cpu)(struct ethosn_dma_allocator *
			     allocator,
			     struct ethosn_dma_info *dma_info);
	int  (*mmap)(struct ethosn_dma_allocator *allocator,
		     struct vm_area_struct *const vma,
		     const struct ethosn_dma_info *const
		     dma_info);
	dma_addr_t      (*get_addr_base)(struct ethosn_dma_allocator *
					 allocator,
					 enum ethosn_stream_id stream_id);
	resource_size_t (*get_addr_size)(struct ethosn_dma_allocator *
					 allocator,
					 enum ethosn_stream_id stream_id);
};

/**
 * ethosn_dma_allocator_create() - Initializes a DMA memory allocator
 * @allocator: Allocator object
 * Return:
 *  Pointer to ethosn_dma_allocator struct representing the DMA memory allocator
 *  Or NULL or negative error code on failure
 */
struct ethosn_dma_allocator *ethosn_dma_allocator_create(struct device *dev);

/**
 * ethosn_dma_allocator_destroy() - Destroy the allocator and free all internal
 * resources.
 * @allocator: pointer to location of the allocator object to destory
 */
void ethosn_dma_allocator_destroy(struct ethosn_dma_allocator **allocator);

/**
 * ethosn_dma_alloc_and_map() - Allocate and map DMA memory
 * @allocator: Allocator object
 * @size: bytes of memory
 * @prot: read/write protection
 * @stream_id: Stream identifier
 * @gfp: GFP flags
 * @debug_tag: (optional) string to identify the allocation. This will be
 *   printed to the console if debug prints are enabled.
 *
 * Return:
 *  Pointer to ethosn_dma_info struct representing the allocation
 *  Or NULL or negative error code on failure
 */
struct ethosn_dma_info *ethosn_dma_alloc_and_map(
	struct ethosn_dma_allocator *allocator,
	size_t size,
	int prot,
	enum ethosn_stream_id stream_id,
	gfp_t gfp,
	const char *debug_tag);

/**
 * ethosn_dma_alloc() - Allocate DMA memory without mapping
 * @allocator: Allocator object
 * @size: bytes of memory
 * @gfp: GFP flags
 * @debug_tag: (optional) string to identify the allocation. This will be
 *   printed to the console if debug prints are enabled.
 *
 * Return:
 *  Pointer to ethosn_dma_info struct representing the allocation
 *  Or NULL or negative error code on failure
 */
struct ethosn_dma_info *ethosn_dma_alloc(struct ethosn_dma_allocator *allocator,
					 size_t size,
					 gfp_t gfp,
					 const char *debug_tag);

/**
 * ethosn_dma_map() - Map DMA memory
 * @allocator: Allocator object
 * @dma_info: Pointer to ethosn_dma_info struct representing the allocation
 * @prot: read/write protection
 * @stream_id: Stream identifier
 *
 * Return:
 *  0 or negative on failure
 */
int ethosn_dma_map(struct ethosn_dma_allocator *allocator,
		   struct ethosn_dma_info *dma_info,
		   int prot,
		   enum ethosn_stream_id stream_id);

/**
 * ethosn_dma_unmap() - Unmap DMA memory
 * @allocator: Allocator object
 * @dma_info: Allocation information
 * @stream_id: Stream identifier
 */
void ethosn_dma_unmap(struct ethosn_dma_allocator *allocator,
		      struct ethosn_dma_info *dma_info,
		      enum ethosn_stream_id stream_id);

/**
 * ethosn_dma_free() - Unmap and Free allocated DMA
 * @allocator: Allocator object
 * @dma_info: Allocation information
 * @stream_id: Stream identifier
 */
void ethosn_dma_unmap_and_free(struct ethosn_dma_allocator *allocator,
			       struct ethosn_dma_info *const dma_info,
			       enum ethosn_stream_id stream_id);

/**
 * ethosn_dma_free() - Free allocated DMA
 * @allocator: Allocator object
 * @dma_info: Allocation information
 */
void ethosn_dma_free(struct ethosn_dma_allocator *allocator,
		     struct ethosn_dma_info *dma_info);

/**
 * ethosn_dma_get_addr_base() - Get base address of a given stream
 * @allocator: Allocator object
 * @stream_id: Stream identifier
 *
 * Return:
 *  Base address or zero on failure
 */
dma_addr_t ethosn_dma_get_addr_base(struct ethosn_dma_allocator *allocator,
				    enum ethosn_stream_id stream_id);

/**
 * ethosn_dma_get_addr_size() - Get address space size of a given stream
 * @allocator: Allocator object
 * @stream_id: Stream identifier
 *
 * Return:
 *  Size of address space or zero on failure
 */
resource_size_t ethosn_dma_get_addr_size(struct ethosn_dma_allocator *allocator,
					 enum ethosn_stream_id stream_id);

/**
 * ethosn_dma_mmap() - Do MMAP of DMA allocated memory
 * @allocator: Allocator object
 * @vma: memory area
 * @dma_info: DMA allocation information
 *
 * Return:
 * * 0 - Success
 * * Negative error code
 */
int ethosn_dma_mmap(struct ethosn_dma_allocator *allocator,
		    struct vm_area_struct *const vma,
		    const struct ethosn_dma_info *const dma_info);

/**
 * ethosn_dma_sync_for_device() - Transfer ownership of the memory buffer to
 * the device. Flushes the CPU cache.
 * @allocator: Allocator object
 * @dma_info: DMA allocation information
 */
void ethosn_dma_sync_for_device(struct ethosn_dma_allocator *allocator,
				struct ethosn_dma_info *dma_info);

/**
 * ethosn_dma_sync_for_cpu() - Transfer ownership of the memory buffer to
 * the cpu. Invalidates the CPU cache.
 * @allocator: Allocator object
 * @dma_info: DMA allocation information
 */
void ethosn_dma_sync_for_cpu(struct ethosn_dma_allocator *allocator,
			     struct ethosn_dma_info *dma_info);

/**
 * ethosn_dma_import() - Import shared DMA buffer
 * @allocator: Allocator object
 * @fd: file descriptor for the shared DMA buffer
 * @size: size of the shared buffer
 *
 * Return:
 * * 0 - Success
 * * Negative error code
 */
struct ethosn_dma_info *ethosn_dma_import(
	struct ethosn_dma_allocator *allocator,
	int fd,
	size_t size);

/**
 * ethosn_dma_release() - Release shared DMA buffer
 * @allocator: Allocator object
 * @dma_info: Allocation information
 */
void ethosn_dma_release(struct ethosn_dma_allocator *allocator,
			struct ethosn_dma_info *const dma_info);

#endif /* _ETHOSN_DMA_H_ */
