// SPDX-License-Identifier: GPL-2.0
#include <linux/slab.h>
#include <linux/version.h>
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
#include <linux/dma-direct.h>
//#else
//#include <linux/dma-mapping.h>
//#endif
#include <asm/mman.h>
#include "comm_driver.h"
#include "comm_debug.h"
#include "comm_mem_mgt.h"
#include "comm_mem_dma.h"

int comm_dma_alloc(struct comm_mem_mgr *mem_mgr, struct comm_kmem *mem)
{
	dma_addr_t dma_addr = 0;

	mem->kva = (uint64_t) dma_alloc_coherent(mem_mgr->dev,
		mem->size, &dma_addr, GFP_KERNEL);

	mem->iova = dma_to_phys(mem_mgr->dev, dma_addr);

	comm_drv_debug("iova: %08llx kva: %08llx\n", mem->iova, mem->kva);

	return (mem->kva) ? 0 : -ENOMEM;
}

int comm_dma_free(struct comm_mem_mgr *mem_mgr, struct comm_kmem *mem)
{
	dma_free_attrs(mem_mgr->dev, mem->size,
		(void *)(uintptr_t) mem->kva, mem->iova, 0);
	comm_drv_debug("Done\n");
	return 0;
}

int comm_dma_init(struct comm_mem_mgr *mem_mgr)
{
	/* check init */
	if (mem_mgr->is_init) {
		comm_drv_debug("apusys memory mgr is already inited\n");
		return -EALREADY;
	}

	/* init */
	mutex_init(&mem_mgr->list_mtx);
	INIT_LIST_HEAD(&mem_mgr->list);

	mem_mgr->is_init = 1;

	comm_drv_debug("done\n");

	return 0;
}

int comm_dma_destroy(struct comm_mem_mgr *mem_mgr)
{
	int ret = 0;

	if (!mem_mgr->is_init) {
		comm_drv_debug("apusys memory mgr is not init, can't destroy\n");
		return -EALREADY;
	}

	mem_mgr->is_init = 0;
	comm_drv_debug("done\n");

	return ret;
}
