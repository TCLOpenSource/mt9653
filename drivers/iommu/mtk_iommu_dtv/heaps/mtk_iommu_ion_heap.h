/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */


#ifndef _LINUX_ION_IOMMU_HEAP_H
#define _LINUX_ION_IOMMU_HEAP_H

#define to_iommu_heap(x) container_of(x, struct ion_iommu_heap, heap)
#define ION_IOMMU_MAGIC 0xabc66

struct ion_iommu_heap {
	struct ion_heap heap;
	struct device *dev;
};

struct ion_iommu_buffer_info {
	int magic;
	unsigned long attrs;
	void *cpu_addr;
	dma_addr_t handle;
	unsigned long len;
	struct sg_table *table;
};

int __init mtk_ion_iommu_init(void);
void __exit mtk_ion_iommu_deinit(void);

#endif	//_LINUX_ION_IOMMU_HEAP_H
