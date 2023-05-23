/* SPDX-License-Identifier: GPL-2.0*/
#ifndef __APUSYS_PLAT_IOMMU_H__
#define __APUSYS_PLAT_IOMMU_H__

#include <linux/platform_device.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "comm_mem_mgt.h"
#include "plat_mem_wrap.h"

struct buf_info {
	u32	size;
	u64	kva;
	u64	iova;
	u32	type;
	pid_t	pid;
	int	fd;
	u32	cache;
	u32	pipe_id;
	u32	secure;
	struct dma_buf *dmabuf;
	struct sg_table *sgt;
};

int mdla_iommu_alloc(struct device *dev, struct buf_info *buf);
int mdla_iommu_free(struct device *dev, struct buf_info *buf, int closefd);
int mdla_iommu_auth(struct device *dev, struct buf_info *buf);
int mdla_iommu_map_uva(struct device *dev, struct vm_area_struct *vma, struct buf_info *buf);
int mdla_iommu_cache_flush(struct device *dev, int fd, u32 size, u64 iova
, u64 kva, struct sg_table *table, unsigned int flush_invalidate_flag);
#endif
