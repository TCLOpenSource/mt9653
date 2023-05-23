/* SPDX-License-Identifier: GPL-2.0*/
#ifndef __APUSYS_DTV_MEM_DMA_H__
#define __APUSYS_DTV_MEM_DMA_H__

int comm_dma_alloc(struct comm_mem_mgr *mem_mgr, struct comm_kmem *mem);
int comm_dma_free(struct comm_mem_mgr *mem_mgr, struct comm_kmem *mem);
int comm_dma_init(struct comm_mem_mgr *mem_mgr);
int comm_dma_destroy(struct comm_mem_mgr *mem_mgr);
#endif
