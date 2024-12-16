#ifndef VSI_DMA_PRIV_H
#define VSI_DMA_PRIV_H

#include <media/videobuf2-memops.h>

struct vb2_dc_buf {
	struct device			*dev;
	void				*vaddr;
	unsigned long			size;
	void				*cookie;
	dma_addr_t			dma_addr;
	unsigned long			attrs;
	enum dma_data_direction		dma_dir;
	struct sg_table			*dma_sgt;
	struct frame_vector		*vec;

	/* MMAP related */
	struct vb2_vmarea_handler	handler;
	refcount_t			refcount;
	struct sg_table			*sgt_base;

	/* DMABUF related */
	struct dma_buf_attachment	*db_attach;

	struct vb2_buffer		*vb;
	bool				non_coherent_mem;

	bool remap;
};

static inline void dma_export_buf_release(__s32 exp_fd)
{
	struct dma_buf *d_buf;
	struct vb2_dc_buf *vb_buf;

	if(exp_fd){
		d_buf = dma_buf_get(exp_fd);
		vb_buf = d_buf->priv;

		vb_buf->handler.put(vb_buf->handler.arg);
	}
}

#endif	//VSI_DMA_PRIV_H