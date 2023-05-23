// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/dma-buf.h>
#include <linux/module.h>
#include <linux/refcount.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <media/videobuf2-memops.h>

static int debug;
module_param(debug, int, 0644);

#define dprintk(level, fmt, args...)					\
	do {								\
		if (debug >= level)					\
			pr_info("mtk-iommu-dtv-contig: " fmt, ##args);	\
	} while (0)

struct vb2_dc_buf {
	struct device *dev;
	void *vaddr;
	unsigned long size;
	void *cookie;
	dma_addr_t dma_addr;
	unsigned long attrs;
	enum dma_data_direction dma_dir;
	struct sg_table *dma_sgt;
	struct frame_vector *vec;

	/* MMAP related */
	struct vb2_vmarea_handler handler;
	refcount_t refcount;
	struct sg_table *sgt_base;

	/* DMABUF related */
	struct dma_buf_attachment *db_attach;
};

/*********************************************/
/*        scatterlist table functions        */
/*********************************************/

static unsigned long vb2_dc_get_contiguous_size(struct sg_table *sgt)
{
	struct scatterlist *s;
	dma_addr_t expected = sg_dma_address(sgt->sgl);
	unsigned int i;
	unsigned long size = 0;

	for_each_sg(sgt->sgl, s, sgt->nents, i) {
		if (sg_dma_address(s) != expected)
			break;
		expected = sg_dma_address(s) + sg_dma_len(s);
		size += sg_dma_len(s);
	}
	return size;
}

/*********************************************/
/*         callbacks for all buffers         */
/*********************************************/

static void *vb2_dc_cookie(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	return &buf->dma_addr;
}

static void *vb2_dc_vaddr(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;

	if (!buf->vaddr && buf->db_attach)
		buf->vaddr = dma_buf_vmap(buf->db_attach->dmabuf);

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

	/* DMABUF exporter will flush the cache for us */
	if (!sgt || buf->db_attach)
		return;

	dma_sync_sg_for_device(buf->dev, sgt->sgl, sgt->orig_nents, buf->dma_dir);
}

static void vb2_dc_finish(void *buf_priv)
{
	struct vb2_dc_buf *buf = buf_priv;
	struct sg_table *sgt = buf->dma_sgt;

	/* DMABUF exporter will flush the cache for us */
	if (!sgt || buf->db_attach)
		return;

	dma_sync_sg_for_cpu(buf->dev, sgt->sgl, sgt->orig_nents, buf->dma_dir);
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
		dprintk(0, "trying to pin a non attached buffer\n");
		return -EINVAL;
	}

	if (WARN_ON(buf->dma_sgt)) {
		dprintk(0, "dmabuf buffer is already pinned\n");
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
		pr_err("contiguous chunk is too small %lu/%lu b\n", contig_size, buf->size);
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

	if (WARN_ON(!buf->db_attach)) {
		dprintk(0, "trying to unpin a not attached buffer\n");
		return;
	}

	if (WARN_ON(!sgt)) {
		dprintk(0, "dmabuf buffer is already unpinned\n");
		return;
	}

	if (buf->vaddr) {
		dma_buf_vunmap(buf->db_attach->dmabuf, buf->vaddr);
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

	/* Meta fd doesn't need vb2_dc_map_dmabuf/vb2_dc_unmap_dmabuf for SHBG now */
	/* so add vunmap here */
	if (buf->vaddr) {
		dma_buf_vunmap(buf->db_attach->dmabuf, buf->vaddr);
		buf->vaddr = NULL;
	}

	/* detach this attachment */
	dma_buf_detach(buf->db_attach->dmabuf, buf->db_attach);
	kfree(buf);
}

static void *vb2_dc_attach_dmabuf(struct device *dev, struct dma_buf *dbuf,
				  unsigned long size, enum dma_data_direction dma_dir)
{
	struct vb2_dc_buf *buf;
	struct dma_buf_attachment *dba;

	if (dbuf->size < size)
		return ERR_PTR(-EFAULT);

	if (WARN_ON(!dev))
		return ERR_PTR(-EINVAL);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dev = dev;
	/* create attachment for the dmabuf with the user device */
	dba = dma_buf_attach(dbuf, buf->dev);
	if (IS_ERR(dba)) {
		dprintk(0, "failed to attach dmabuf\n");
		kfree(buf);
		return dba;
	}

	buf->dma_dir = dma_dir;
	buf->size = size;
	buf->db_attach = dba;

	return buf;
}

/*********************************************/
/*       DMA CONTIG exported functions       */
/*********************************************/

const struct vb2_mem_ops mtk_iommu_dtv_contig_memops = {
	.cookie = vb2_dc_cookie,
	.vaddr = vb2_dc_vaddr,
	.prepare = vb2_dc_prepare,
	.finish = vb2_dc_finish,
	.map_dmabuf = vb2_dc_map_dmabuf,
	.unmap_dmabuf = vb2_dc_unmap_dmabuf,
	.attach_dmabuf = vb2_dc_attach_dmabuf,
	.detach_dmabuf = vb2_dc_detach_dmabuf,
	.num_users = vb2_dc_num_users,
};
EXPORT_SYMBOL(mtk_iommu_dtv_contig_memops);
