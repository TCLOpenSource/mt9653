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
	struct buf_info buf;

	memset(&buf, 0, sizeof(buf));
	buf.size = kmem->size;
	buf.cache = kmem->cache;
	buf.pipe_id = kmem->pipe_id;
	buf.secure = kmem->secure;

	if (mdla_iommu_alloc(mgr->dev, &buf)) {
		comm_err("%s fail, size:%d cache:%d\n",
				__func__, buf.size, buf.cache);
		return -ENOMEM;
	}

	if ((kmem->pipe_id != 0xFFFFFFFF) && kmem->pipe_id) {
		if (mdla_iommu_auth(mgr->dev, &buf)) {
			comm_err("%s fail, pipe:%d iova:0x%llx size:%d\n",
					__func__, buf.pipe_id, buf.iova, buf.size);
		}
	}

	kmem->kva = buf.kva;
	kmem->iova = buf.iova;
	kmem->phy = buf.iova;
	kmem->fd = buf.fd;
	kmem->dmabuf = buf.dmabuf;
	kmem->sgt = buf.sgt;

	kmem->pid = task_pid_nr(current);
	kmem->tgid = current->tgid;
	kmem->status = 0;
	memcpy(kmem->comm, current->comm, 0x10);

	comm_drv_debug("%s: pipe:%x iova:%llx fd:%d size:%d cached:%x pid:%x\n",
		__func__, kmem->pipe_id, kmem->iova,
		 kmem->fd, kmem->size, kmem->cache, kmem->pid);

	return 0;
}

int plat_mem_free(struct comm_mem_mgr *mgr, struct comm_kmem *kmem, int freefd)
{
	struct buf_info buf;

	buf.size = kmem->size;
	buf.kva = kmem->kva;
	buf.iova = kmem->iova;
	buf.sgt = kmem->sgt;
	buf.dmabuf = kmem->dmabuf;
	buf.fd = kmem->fd;

	comm_drv_debug("%s: pipe:%x iova:%llx size:%d cached:%x pid:%x\n",
		__func__, kmem->pipe_id, kmem->iova,
		 kmem->size, kmem->cache, kmem->pid);

	if (mdla_iommu_free(mgr->dev, &buf, freefd) < 0) {
		comm_err("%s fail, iova:0x%llx size:%x\n",
				__func__, buf.iova, buf.size);
		return -EFAULT;
	}
	return 0;
}

#define CACHE_API_ENABLE
int plat_mem_cache_flush(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
#ifdef CACHE_API_ENABLE
	//if (mdla_iommu_cache_flush(mgr->dev, kmem->fd, kmem->size) < 0) {
	if (mdla_iommu_cache_flush(mgr->dev, kmem->fd, kmem->size, kmem->iova
, kmem->kva, kmem->sgt, kmem->cache) < 0) {
		comm_drv_debug("%s free failed!!\n", __func__);
		return -ENOMEM;
	}
#endif
	return 0;
}

int plat_mem_map_uva(struct comm_mem_mgr *mgr,
			struct vm_area_struct *vma,
			struct comm_kmem *kmem)
{
	struct buf_info buf;
	int ret;

	switch (kmem->mem_type) {
	case APU_COMM_MEM_VLM:
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
		ret = remap_pfn_range(vma, vma->vm_start,
					vma->vm_pgoff,
					vma->vm_end - vma->vm_start,
					vma->vm_page_prot);
		break;
	default:
		buf.fd = kmem->fd;
		buf.dmabuf = kmem->dmabuf;
		buf.size = kmem->size;
		buf.kva = kmem->kva;
		buf.iova = kmem->iova;
		buf.cache = kmem->cache;
		ret = mdla_iommu_map_uva(mgr->dev, vma, &buf);
		break;
	}

	return ret;
}

int plat_mem_import(struct comm_mem_mgr *mgr, struct comm_kmem *kmem) { return 0; };
int plat_mem_unimport(struct comm_mem_mgr *mgr, struct comm_kmem *kmem) { return 0; };

int plat_mem_map_iova(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	return 0;
}

int plat_mem_unmap_iova(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	return 0;
}

int plat_mem_map_kva(struct comm_mem_mgr *mgr, struct comm_kmem *kmem)
{
	return 0;
}

int plat_mem_unmap_kva(struct comm_mem_mgr *mgr, struct comm_kmem *kmem) { return 0; };
