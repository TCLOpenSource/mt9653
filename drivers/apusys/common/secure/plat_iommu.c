// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/dma-buf.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/ion.h>
#include "mtk_iommu_dtv_api.h"
#include "comm_debug.h"
#include "plat_iommu.h"
#include "comm_mem_mgt.h"

#define MTK_IOMMU_DTV_IOVA_START (0x200000000ULL)  //8G
#define MTK_IOMMU_DTV_IOVA_END   (0x400000000ULL)  //16G
#define DMA_BIT_MASK_PARA 34

#define MDLA_BUFTAG_ID 40
#define BUFTAG_SHIFT		24
#define MDLA_BUFTAG_ATTR (MDLA_BUFTAG_ID << BUFTAG_SHIFT)

#define COMM_CACHE_FLUSH 5
#define COMM_CACHE_INVALIDATE 6

int mdla_iommu_cache_flush(struct device *dev, int fd, u32 size, u64 iova
, u64 kva, struct sg_table *table, unsigned int flush_invalidate_flag)
{
	if (!dev) {
		comm_drv_debug("dev init failed\n");
		goto out;
	}

	if (!table)
		goto out;

	if (flush_invalidate_flag == COMM_CACHE_FLUSH)
		// 1.3 flush IOMMU buffer after cpu access buffer in cache
		dma_sync_sg_for_device(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);
	if (flush_invalidate_flag == COMM_CACHE_INVALIDATE)
		// 1.4 flush IOMMU buffer after device access buffer for cpu
		dma_sync_sg_for_cpu(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);

	return 0;

out:
	return -EINVAL;
}

int mdla_iommu_map_uva(struct device *dev, struct vm_area_struct *vma, struct buf_info *buf)
{
	u64 addr;
	unsigned long size;
	unsigned long attrs = 0;
	u32 offset;

	addr = (u64)(vma->vm_pgoff) << PAGE_SHIFT;
	size = vma->vm_end - vma->vm_start;

	offset = (addr & UINT_MAX) - (buf->iova & UINT_MAX);
	vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP | VM_MIXEDMAP;

	vma->vm_pgoff = 0;
	if (buf->kva) {
		if (buf->cache == 0)
			attrs |= DMA_ATTR_WRITE_COMBINE;

		return dma_mmap_attrs(dev, vma, (void *)buf->kva + offset,
					buf->iova + offset, size, attrs);
	} else {
		//non-valid mapping range
		comm_drv_debug("mdla iova map uva  mmap fail! addr %llx size %lu!\n", addr, size);
		return -EINVAL;
	}

}

int mdla_iommu_alloc(struct device *dev, struct buf_info *buf)
{
	int ret = -ENOMEM;
	void *kva = NULL;
	struct sg_table *table = NULL;
	dma_addr_t dma_addr = 0;
	unsigned long attrs = 0;

	if (!dev) {
		comm_drv_debug("dev init failed\n");
		goto err_out;
	}

	if (dma_get_mask(dev) < DMA_BIT_MASK(DMA_BIT_MASK_PARA)) {
		comm_drv_debug("register IOMMU client fail, mask=0x%llx\n",
							dma_get_mask(dev));
		goto err_out;
	}

	if (!dma_supported(dev, 0))
		goto err_out;

	attrs = MDLA_BUFTAG_ATTR;

	if (buf->cache == 0)
		attrs |= DMA_ATTR_WRITE_COMBINE;

	kva = dma_alloc_attrs(dev, buf->size, &dma_addr, GFP_KERNEL, attrs);
	if (dma_mapping_error(dev, dma_addr) != 0) {
		comm_err("dma_alloc_attrs fail\n");
		goto err_out;
	}

	if (dma_addr < MTK_IOMMU_DTV_IOVA_START || dma_addr >= MTK_IOMMU_DTV_IOVA_END) {
		comm_err("check iova: 0x%llx range fail\n", dma_addr);
		goto err_free;
	}

	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		goto err_free;

	// 1.2 get IOMMU buffer sg_table table
	if (dma_get_sgtable_attrs(dev, table, kva, dma_addr, buf->size, 0)) {
		comm_drv_debug("get_sgtable_attrs fail\n");
		goto err_sg;
	}
	// 1.3 flush IOMMU buffer after cpu access buffer in cache
	dma_sync_sg_for_device(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);

	// 1.4 flush IOMMU buffer after device access buffer for cpu
	dma_sync_sg_for_cpu(dev, table->sgl, table->nents, DMA_BIDIRECTIONAL);


	buf->sgt = table;
	buf->kva = (u64)kva;
	buf->iova = (u64)dma_addr;
	buf->fd = (int)dma_addr;

	return 0;
err_sg:
	kfree(table);
err_free:
	dma_free_attrs(dev, buf->size, kva, dma_addr, MDLA_BUFTAG_ATTR);
err_out:
	return ret;
}

int mdla_iommu_free(struct device *dev, struct buf_info *buf, int closefd)
{
	if (buf->sgt) {
		sg_free_table(buf->sgt);
		kfree(buf->sgt);
	}

	dma_free_attrs(dev, buf->size, (void *)buf->kva,
			buf->iova, MDLA_BUFTAG_ATTR);

	return 0;
}

int mdla_iommu_auth(struct device *dev, struct buf_info *buf)
{
	int ret = 0;
	u32 pipelineID = 0;

	/* get device parameter */
	pipelineID = buf->pipe_id;
	comm_drv_debug("[%s] pipelineID:%x\n", __func__, pipelineID);

	if (pipelineID == 0xFFFFFFFF) {
	#ifdef ALLOC_PIPEID
		mtkd_iommu_allocpipelineid(&pipelineID);
		comm_drv_debug("Alloc pipelineID = %x\n", pipelineID);
		buf->pipe_id = pipelineID;
		if (pipelineID == 0xFFFFFFFF)
			return -EPERM;
	#else
		return -EPERM;
	#endif
	}

	/* authorize buf */
	ret = mtkd_iommu_buffer_authorize(MDLA_BUFTAG_ATTR,
					buf->iova, buf->size, pipelineID);
	if (ret) {
		comm_err("[%s] authorize fail, pipe:%d iova:%llx id:%d\n",
				__func__, pipelineID, buf->iova, buf->size);
		return -EPERM;
	}

	return ret;
}
