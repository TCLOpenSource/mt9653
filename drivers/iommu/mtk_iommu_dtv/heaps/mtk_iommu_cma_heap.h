/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */
#ifndef _LINUX_MTK_IOMMU_CMA_HEAP_H
#define _LINUX_MTK_IOMMU_CMA_HEAP_H

#define MTK_ION_CMA_HEAP_NAME "mtkdtv_cma_heap"
#define MAX_IOMMU_CMA_AREAS 4

void sys_cma_free(struct ion_buffer *buffer);
int sys_cma_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
			unsigned long size, unsigned long flags);
int mtk_cma_heap_init(struct device *dev);
void mtk_cma_heap_exit(void);

#endif
