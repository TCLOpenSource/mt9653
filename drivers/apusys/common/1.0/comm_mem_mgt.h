/* SPDX-License-Identifier: GPL-2.0*/
#ifndef __APUSYS_COMM_MEM_MGT_H__
#define __APUSYS_COMM_MEM_MGT_H__

#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/dma-buf.h>

struct comm_mem_mgr {
	struct list_head list;
	struct mutex list_mtx;
	struct platform_device *pdev;
	struct device *dev;
	uint8_t is_init;
};

enum {
	APU_COMM_MEM_DRAM_ION,
	APU_COMM_MEM_DRAM_DMA,
	APU_COMM_MEM_VLM,
	APU_COMM_MEM_DRAM_ION_AOSP,
	APU_COMM_MEM_DRAM_ION_CACHED,
	APU_COMM_MEM_DRAM_DMA_CACHED,
	APU_COMM_MEM_AUTO,
	APU_COMM_MEM_MAX,
};

enum {
	COMM_MEM_OBSOLETE,
};

enum {
	COMM_CACHE_OP_NONE,
	COMM_CACHE_OP_SYNC,
	COMM_CACHE_OP_FLUSH,
	COMM_CACHE_OP_INVALIDATE,
	COMM_CACHE_OP_MAX,
};

enum {
	COMM_QUERY_BY_FD,
	COMM_QUERY_BY_IOVA,
	COMM_QUERY_BY_KVA,
};

struct comm_kmem {
	unsigned long long uva;
	unsigned long long kva;
	unsigned long long phy;
	unsigned long long iova;
	unsigned int size;
	unsigned int iova_size;

	unsigned int align;
	unsigned int cache;

	int mem_type;
	int fd;
	unsigned long long khandle;
	int property;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;

	struct dma_buf *dmabuf;

	pid_t pid;
	pid_t tgid;
	char comm[0x10];

	/* security authorize */
	unsigned int secure;
	unsigned int pipe_id;
	unsigned int status;
};

int comm_mem_init(struct platform_device *pdev);
int comm_mem_destroy(void);
#ifdef COMM_CTL
int comm_mem_ctl(struct apusys_mem_ctl *ctl_data, struct comm_kmem *mem);
#endif
int comm_mem_alloc(struct comm_kmem *mem);
int comm_mem_free(struct comm_kmem *mem, u32 closefd);
int comm_mem_cache_flush(struct comm_kmem *mem);
int comm_mem_import(struct comm_kmem *mem);
int comm_mem_unimport(struct comm_kmem *mem);
int comm_mem_map_iova(struct comm_kmem *mem);
int comm_mem_map_kva(struct comm_kmem *mem);
int comm_mem_unmap_iova(struct comm_kmem *mem);
int comm_mem_unmap_kva(struct comm_kmem *mem);
int comm_mem_map_uva(struct vm_area_struct *vma);
int comm_mem_vlm_info(struct comm_kmem *mem);
void comm_mem_suspend(void);
int comm_mem_query_mem(struct comm_kmem *mem, u32 query_type);

#endif
