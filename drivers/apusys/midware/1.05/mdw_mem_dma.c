// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/dma-direct.h>
//#if 0
//#include <ion.h>
//#include <mtk/ion_drv.h>
//#include <mtk/mtk_ion.h>

//#include <linux/dma-mapping.h>
//#endif
#include <asm/mman.h>

#include "mdw_cmn.h"
#include "apusys_drv.h"
//#include "memory_mgt.h"
//#include "memory_dma.h"

//#define IOMMU_PATH
//static int global_fd;
#define MELW_M7642_PLATFORM
//#define MANDY_PLATFORM
//#include "mdla_plat_fixed_mem.h"
//extern struct device *mdlactl_device;
#include <linux/dma-mapping.h>

#include <linux/dma-buf.h>
#include <linux/slab.h>

#define mdla_err_debug mdw_mem_debug
#define mdla_info_debug mdw_mem_debug
//#include "memory_dma.h"
#include "comm_driver.h"
//#include "mma_api.h"
//#define OLD_ALLOC_PATH

#include "mdw_mem_cmn.h"

/* ion mem allocator */
struct mdw_mem_dma_ma {
	struct mdw_mem_ops ops;
};

static struct mdw_mem_dma_ma dma_ma;

int dma_mem_map_kva(struct apusys_kmem *mem)
{
	//void *buffer = NULL;
	//struct dma_buf *db = NULL;
	//struct mdla_iova_alloc_node buf;
	//int ret = 0;
	//struct mma_buf_handle *handle = NULL;
	struct comm_kmem localmem;

	/* check argument */
	if (mem == NULL) {
		mdw_drv_err("invalid argument\n");
		return -EINVAL;
	}

	mdw_mem_debug(" map kva mem->fd =%x\n", mem->fd);
	localmem.iova = (unsigned long long)mem->fd | 0x200000000;
	localmem.size = mem->size;

	localmem.mem_type = APU_COMM_MEM_DRAM_DMA;

	//comm_util_get_cb()->map_kva(&localmem);
	if (comm_util_get_cb()->map_kva(&localmem)) {
		mdw_mem_debug("%s: failed!!\n", __func__);
		return -EFAULT;
	}
	mdw_mem_debug("map_kva %llx\n", localmem.kva);

	mem->iova_size = localmem.iova_size;
	mem->kva = localmem.kva;

	mdw_mem_debug("mem(%d/0x%llx/0x%x/0x%x/%d/0x%x/0x%llx/0x%llx)\n",
			mem->fd, mem->uva, mem->iova,
			mem->iova + mem->iova_size - 1,
			mem->size, mem->iova_size,
			mem->khandle, mem->kva);
	//dma_buf_put(db);
	return 0;

//free_import:
//	return ret;
}

int dma_mem_map_iova(struct apusys_kmem *mem)
{
	//int ret = 0;
	//struct dma_buf *db = NULL;
	//struct mdla_iova_alloc_node buf;
	//struct mma_buf_handle *handle = NULL;
	//u64 addr = 0;
	struct comm_kmem localmem;

	localmem.iova = (unsigned long long)mem->fd | 0x200000000;

	localmem.mem_type = APU_COMM_MEM_DRAM_DMA;

	//comm_util_get_cb()->map_iova(&localmem);
	if (comm_util_get_cb()->map_iova(&localmem)) {
		mdw_mem_debug("%s: failed!!\n", __func__);
		return -EFAULT;
	}

	mem->size = localmem.size;
	mem->iova_size = localmem.iova_size;
	//mem->iova = localmem.iova;
	mem->iova = localmem.iova & 0xFFFFFFFF;


	mdw_mem_debug("mem(%d/0x%llx/0x%x/0x%x/%d/0x%x/0x%llx/0x%llx)\n",
			mem->fd, mem->uva, mem->iova,
			mem->iova + mem->iova_size - 1,
			mem->size, mem->iova_size,
			mem->khandle, mem->kva);

	//dma_buf_put(db);

	return 0;
//free_import:
//	return ret;
}
int dma_mem_unmap_iova(struct apusys_kmem *mem)
{
	int ret = 0;

	mdw_mem_debug("mem(%d/0x%llx/0x%x/0x%x/%d/0x%x/0x%llx/0x%llx)\n",
			mem->fd, mem->uva, mem->iova,
			mem->iova + mem->iova_size - 1,
			mem->size, mem->iova_size,
			mem->khandle, mem->kva);

	return ret;
}

int dma_mem_unmap_kva(struct apusys_kmem *mem)
{
	//struct dma_buf *dma_buf = NULL;
	//struct mma_buf_handle *dma_buf = NULL;
	int ret = 0;
	//u32 size;

	mdw_mem_debug("mem(%d/0x%llx/0x%x/0x%x/%d/0x%x/0x%llx/0x%llx)\n",
			mem->fd, mem->uva, mem->iova,
			mem->iova + mem->iova_size - 1,
			mem->size, mem->iova_size,
			mem->khandle, mem->kva);

	return ret;

}

//static void mdla_iommu_open(void)
//{
//	//mma_open();
//}

//static void mdla_iommu_release(void)
//{
//	//mma_release();
//}


int dma_mem_alloc(struct apusys_kmem *mem)
{
	//int ret = 0;
	void *va = NULL;
	//struct dma_buf *db = NULL;
	__u64 pa = 0;
	//int fd = NULL;
	//dma_addr_t dma_addr = 0;
	struct comm_kmem localmem;

	memset(&localmem, 0, sizeof(localmem));
	localmem.mem_type = APU_COMM_MEM_DRAM_DMA;
	localmem.size = mem->size;
	localmem.cache = mem->cache;
	//localmem.cache = 0;//force non cacheable allocate

	mdw_mem_debug("enter apusys dma mem alloc\n");
	if (comm_util_get_cb()->alloc(&localmem)) {
		mdw_mem_debug("%s: failed!!\n", __func__);
		return -EFAULT;
	}
	mdw_mem_debug("after alloc(&localmem) at  dma mem alloc\n");
#ifdef OLD_ALLOC_PATH
#ifdef MANDY_PLATFORM
	mem->kva = (uint64_t)apusys_fixed_mem_alloc(mem->size, &pa);
	if (!(mem->kva)) {
		mdw_mem_debug("MDLA: apusys_fixed_mem_alloc failed!!\n");
		return -ENOMEM;
	}
	mem->iova = pa;
#else
	mem->kva = (uint64_t) dma_alloc_coherent(mem_mgr->dev,
		mem->size, &dma_addr, GFP_KERNEL | GFP_DMA);
	if (!(mem->kva)) {
		mdw_mem_debug("MDLA: dma_alloc_coherent failed!!\n");
		return -ENOMEM;
	}

	mem->iova = dma_to_phys(mem_mgr->dev, dma_addr);
#endif

#ifdef MELW_M7642_PLATFORM
	mem->iova -= 0x20000000;
#endif

	mem->fd = ++global_fd;
#endif
	mem->fd = (int)localmem.iova;
	mem->kva = localmem.kva;
	//mem->iova = localmem.iova;
	mem->iova = localmem.iova & 0xFFFFFFFF;
	va = (void *)mem->kva;
	pa = mem->iova;
	//ret = iommu_alloc_buf_register(mem->fd, va, (dma_addr_t *)pa, mem->size, db);
	//if(ret < 0)
		//goto err_buf_register;

	mdw_mem_debug("dma path iommu_alloc_buf_register finish");
	mdw_mem_debug("mem->fd=%x vaddr=%lx phy_addr =%llx  mem->size =%x\n"
, mem->fd, (unsigned long)va, pa, mem->size);

	return (mem->kva) ? 0 : -ENOMEM;

//err_buf_register:
	//iommu_alloc_buf_unregister(va);
	//return -ENOMEM;
}

int dma_mem_free(struct apusys_kmem *mem)
{
	void *virt;
	struct comm_kmem localmem;

	virt = (void *)mem->kva;
	//iommu_alloc_buf_unregister(virt);

	memset(&localmem, 0, sizeof(localmem));
	localmem.mem_type = APU_COMM_MEM_DRAM_DMA;
	localmem.kva = mem->kva;
	localmem.iova = mem->iova | 0x200000000;
	localmem.size = mem->size;

	if (comm_util_get_cb()->free(&localmem, mem->release_flag)) {
		mdw_mem_debug("%s: failed!!\n", __func__);
		return -EFAULT;
	}

#ifdef OLD_ALLOC_PATH
#ifdef MANDY_PLATFORM
	apusys_fixed_mem_free((void *)mem->kva, mem->size);
#else
	dma_free_attrs(mem_mgr->dev, mem->size,
		(void *) mem->kva, mem->iova, 0);
#endif
#endif
	mdw_mem_debug("Done\n");
	return 0;
}

int dma_mem_import(struct apusys_kmem *mem)
{
	int ret = 0;

	if (dma_mem_map_iova(mem)) {
		ret = -ENOMEM;
		goto free_import;
	}

free_import:
	return ret;

}

#define CACHE_FLUSH 5
#define CACHE_INVALIDATE 6
int dma_mem_flush(struct apusys_kmem *mem)
{
	void *virt;
	struct comm_kmem localmem;

	virt = (void *)mem->kva;
	//iommu_alloc_buf_unregister(virt);

	memset(&localmem, 0, sizeof(localmem));
	localmem.mem_type = APU_COMM_MEM_DRAM_DMA;
	//localmem.kva = mem->kva;
	localmem.iova = (unsigned long long)mem->fd;
	localmem.fd = mem->fd;
	localmem.cache = CACHE_FLUSH;
	//mdw_mem_debug("apusys dma mem flush  localmem.fd = %lx\n", localmem.fd);
	mdw_mem_debug("apusys cache flsuh test  dma mem flush  localmem.fd = %x\n", localmem.fd);
	localmem.size = mem->size;

	if (comm_util_get_cb()->cache_flush(&localmem)) {
		mdw_mem_debug("%s: failed!!\n", __func__);
		return -EFAULT;
	}

	mdw_mem_debug("Done\n");
	return 0;
}

static int mdw_mem_dma_invalidate(struct apusys_kmem *mem)
{
	void *virt;
	struct comm_kmem localmem;

	virt = (void *)mem->kva;
	//iommu_alloc_buf_unregister(virt);

	localmem.mem_type = APU_COMM_MEM_DRAM_DMA;
	//localmem.kva = mem->kva;
	localmem.iova = (unsigned long long)mem->fd;
	localmem.fd = mem->fd;
	localmem.cache = CACHE_INVALIDATE;
	//mdw_mem_debug("apusys dma mem flush  localmem.fd = %lx\n", localmem.fd);
	mdw_mem_debug("apusys cache invlidate dma mem localmem.fd = %x\n", localmem.fd);
	localmem.size = mem->size;

	if (comm_util_get_cb()->cache_flush(&localmem)) {
		mdw_mem_debug("%s: failed!!\n", __func__);
		return -EFAULT;
	}

	mdw_mem_debug("Done\n");
	return 0;
}

void dma_mem_destroy(void)
{
	//int ret = 0;

//	if (!mem_mgr->is_init) {
//		mdw_drv_debug("apusys memory mgr is not init, can't destroy\n");
//		return -EALREADY;
//	}

//	mem_mgr->is_init = 0;
	mdw_mem_debug("done\n");

	//return ret;
}

struct mdw_mem_ops *dma_mem_init(void)
{

	memset(&dma_ma, 0, sizeof(dma_ma));

	/* create ion client */
	//ion_ma.client = ion_client_create(g_ion_device, "apusys midware");
	//if (IS_ERR_OR_NULL(ion_ma.client))
		//return NULL;

	dma_ma.ops.alloc = dma_mem_alloc;
	dma_ma.ops.free = dma_mem_free;
	dma_ma.ops.flush = dma_mem_flush;
	dma_ma.ops.invalidate = mdw_mem_dma_invalidate;
	dma_ma.ops.map_kva = dma_mem_map_kva;
	dma_ma.ops.unmap_kva = dma_mem_unmap_kva;
	dma_ma.ops.map_iova = dma_mem_map_iova;
	dma_ma.ops.unmap_iova = dma_mem_unmap_iova;
	dma_ma.ops.destroy = dma_mem_destroy;

	return &dma_ma.ops;


#ifdef OLD_ALLOC_PATH
#ifdef MANDY_PLATFORM
	//apusys_fixed_mem_init(0x99200000,(0x99200000-0x20000000),0x6e00000);

	//for me_ll_o platform
	apusys_fixed_mem_init(0x4c100000, (0x4c100000-0x20000000), 0x2000000);
#endif
#endif
	mdw_mem_debug("done\n");

	return 0;
}
