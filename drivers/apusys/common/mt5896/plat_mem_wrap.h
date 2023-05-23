/* SPDX-License-Identifier: GPL-2.0*/
#ifndef __APUSYS_PLAT_MEM_WRAP_H__
#define __APUSYS_PLAT_MEM_WRAP_H__

#include <linux/mm.h>
#include "comm_driver.h"
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#define IOMMU_ENABLE

int plat_mem_init(struct comm_mem_mgr *mgr);
int plat_mem_destroy(struct comm_mem_mgr *mgr);
int plat_mem_alloc(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_free(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_cache_flush(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_import(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_unimport(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_map_iova(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_unmap_iova(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_map_kva(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_unmap_kva(struct comm_mem_mgr *mgr, struct comm_kmem *kmem);
int plat_mem_map_uva(struct comm_mem_mgr *mgr, struct vm_area_struct *vma);
#endif
