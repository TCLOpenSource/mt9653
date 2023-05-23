// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include "comm_mem_mgt.h"
#include "comm_debug.h"
#include "plat_iommu.h"

int plat_mem_init(struct comm_mem_mgr *mgr)
{
	return 0;
}

int plat_mem_destroy(struct comm_mem_mgr *mgr)
{
	return 0;
}

int plat_mem_alloc(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	kmem->kva = mdla_iommu_alloc(mgr->dev, kmem->size,
				&kmem->iova, &kmem->fd, kmem->cache);
	if (!kmem->kva) {
		comm_drv_debug("%s alloc failed!!\n", __func__);
		return -ENOMEM;
	}

	comm_drv_debug("%s: kva: 0x%llx iova %llx fd %x size %d cached %x\n",
		__func__, kmem->kva, kmem->iova, kmem->fd, kmem->size, kmem->cache);

	return 0;
}

int plat_mem_free(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	//fill iova info for list comapre
	struct mdla_iova_alloc_node buf;

	if (mdla_iova_buf_get_by_virt((void *)kmem->kva, &buf)) {
		kmem->iova = buf.buf_phy;
		comm_drv_debug("plat mem free iova: %llx kva: %llx\n", kmem->iova, kmem->kva);
	}

	if (mdla_iova_free(mgr->dev, (void *)kmem->kva, kmem->size) < 0) {
		comm_drv_debug("%s free failed!!\n", __func__);
		return -ENOMEM;
	}
	return 0;
}

int plat_mem_cache_flush(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	if (mdla_iova_cache_flush(mgr->dev, kmem->fd, kmem->size) < 0) {
		comm_drv_debug("%s free failed!!\n", __func__);
		return -ENOMEM;
	}
	return 0;
}

int plat_mem_map_uva(struct comm_mem_mgr *mgr, struct vm_area_struct *vma)
{
	return mdla_iova_map_uva(mgr->dev, vma);
}

int plat_mem_import(struct comm_mem_mgr *mgr, struct comm_kmem *kmem) { return 0; };
int plat_mem_unimport(struct comm_mem_mgr *mgr, struct comm_kmem *kmem) { return 0; };

int plat_mem_map_iova(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{

	kmem->iova = mdla_iova_fd_to_mva(kmem->fd);

	kmem->size = mdla_iova_phy_to_size(kmem->iova);
	kmem->iova_size	= kmem->size;

	return (kmem->iova)?0:(-EINVAL);
}

int plat_mem_unmap_iova(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	return 0;
}

int plat_mem_map_kva(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	unsigned long long iova;

	kmem->kva = mdla_iova_fd_to_kva(kmem->fd);

	kmem->iova = mdla_iova_fd_to_mva(kmem->fd);
	kmem->size = mdla_iova_phy_to_size(kmem->iova);
	kmem->iova_size	= kmem->size;


	return (kmem->kva)?0:(-EINVAL);
}

int plat_mem_unmap_kva(struct comm_mem_mgr *mgr, struct comm_kmem *kmem) { return 0; };
