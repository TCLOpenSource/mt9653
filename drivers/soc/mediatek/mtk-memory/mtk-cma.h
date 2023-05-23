/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joe Liu <joe.liu@mediatek.com>
 */

#include <linux/cma.h>

#define IOMMU_CMA_ALLOC_BIT 31

extern struct device *cma_get_heap_dev(unsigned int heap_id);
extern int mtk_miup_rel_kprot(u64 start_addr, u64 addr_len);
extern int mtk_miup_req_kprot(u64 start_addr, u64 addr_len);

extern struct device *mtk_mmap_device;
extern u32 mtkcma_cpu_bus_base;
extern int ion_test(struct device *dev,	unsigned long alloc_offset,
	unsigned long size, unsigned int flags);
extern int mtk_ion_alloc(size_t len, unsigned int heap_id_mask,
	unsigned int flags);

struct page *dma_alloc_at_from_contiguous(struct cma *cma, int count,
				unsigned int align, phys_addr_t at_addr);
unsigned long ion_get_mtkcma_buffer_info(int share_fd);
void show_cma_info(struct cma *show_cma);
int mtkcma_presetting(struct device *dev, int pool_index);
int mtkcma_presetting_v2(struct device *dev, int pool_index);
int mtkcma_presetting_utopia(struct device *dev, int pool_index);
