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
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <asm/mman.h>
#include "comm_util.h"
#include "comm_debug.h"
#include "comm_mem_mgt.h"
#include "comm_mem_dma.h"
#include "plat_mem_wrap.h"

static struct comm_mem_mgr g_mem_mgr;

struct mem_record {
	struct comm_kmem kmem;
	struct list_head m_list;
};

//----------------------------------------------
static void comm_print_kmem(const char *str, struct comm_kmem *mem)
{
	if (mem == NULL)
		return;

	comm_drv_debug("%s: mr(0x%llx/%d/0x%llx/%d/0x%llx)\n",
		str,
		mem->uva,
		mem->size,
		mem->iova,
		mem->fd,
		mem->kva);
}

struct apu_iommu_heap {
	char *name;
	void *data;
};

static int plat_dev_mem_init(struct platform_device *pdev)
{
	struct mem_record *mr = NULL;
	struct comm_kmem *mem;
	int mem_idx;
	struct device *dev = &pdev->dev;
	struct resource *res;
	void *gsm_cpu_virt;
	u32 gsm_size, gsm_cpu_mva, gsm_mdla_mva;

	dev_info(dev, "Device Tree (GSM) Probing\n");

	mem_idx = 3;
	res = platform_get_resource(pdev, IORESOURCE_MEM, mem_idx++);

	if (!res) {
		dev_err(dev, "GSM: get mdla_mva fail\n");
		return -ENODEV;
	}

	gsm_size = res->end - res->start + 1;
	gsm_cpu_virt = ioremap_nocache(res->start, gsm_size);

	if (!gsm_cpu_virt) {
		dev_info(dev, "%s ioremap fail, cpu_pa: %llx size %x\n",
			__func__, (u64)res->start, gsm_size);

		return -EIO;
	}

	gsm_cpu_mva = res->start;
	dev_info(dev, "GSM: cpu_mva: %x, cpu_virt: %p\n",
			(u32)gsm_cpu_mva, gsm_cpu_virt);

	res = platform_get_resource(pdev, IORESOURCE_MEM, mem_idx);

	if (!res) {
		dev_err(dev, "GSM: get mdla_mva fail, idx = %d\n", mem_idx);
		iounmap(gsm_cpu_virt);
		return -ENODEV;
	}

	gsm_mdla_mva = res->start;

	dev_info(dev, "GSM: mdla_mva: %x, cpu_virt: %p\n",
			(u32)gsm_mdla_mva, gsm_cpu_virt);

	mr = vmalloc(sizeof(struct mem_record));
	if (mr != NULL) {
		mem = &mr->kmem;
		memset(mem, 0, sizeof(*mem));
		mem->iova = gsm_mdla_mva;
		mem->kva = (u64)gsm_cpu_virt;
		mem->phy = gsm_cpu_mva;
		mem->size = gsm_size;
		mem->mem_type = APU_COMM_MEM_VLM;

		INIT_LIST_HEAD(&mr->m_list);
		mutex_lock(&g_mem_mgr.list_mtx);
		list_add_tail(&mr->m_list,
			&g_mem_mgr.list);
		mutex_unlock(&g_mem_mgr.list_mtx);
		comm_print_kmem("insert", mem);
	} else {
		dev_err(dev, "GSM: insert to list fail.\n");
		return -ENOMEM;
	}

	return 0;
}

int plat_dev_init(struct platform_device *pdev)
{
	struct platform_device *mdla_pdev;
	struct device *dev = &pdev->dev;
	struct device_node *np;

	g_mem_mgr.pdev = pdev;
	g_mem_mgr.dev = &pdev->dev;

	np = comm_get_device_node(NULL);
	if (!np) {
		dev_err(dev, "of_node: heap node not found.\n");
		goto out;
	}

	mdla_pdev = of_find_device_by_node(np);
	if (!mdla_pdev) {
		dev_info(dev, "mdla_pdev not found\n");
		goto put;
	}

	dev_info(dev, "mdla_pdev at %p\n", mdla_pdev);

	plat_dev_mem_init(mdla_pdev);

	return 0;

put:
	of_node_put(np);
out:
	return -EFAULT;
}

int plat_dev_uninit(void)
{
	if (g_mem_mgr.pdev)
		platform_device_put(g_mem_mgr.pdev);

	g_mem_mgr.pdev = NULL;
	g_mem_mgr.dev = NULL;

	return 0;
}

int comm_mem_vlm_info(struct comm_kmem *mem)
{
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;
	int ret = -EFAULT;

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		if (mr->kmem.mem_type == APU_COMM_MEM_VLM) {
			memcpy(mem, &mr->kmem, sizeof(*mem));
			ret = 0;
			break;
		}
	}
	mutex_unlock(&g_mem_mgr.list_mtx);

	return ret;
}

int comm_mem_alloc(struct comm_kmem *mem)
{
	int ret = 0;
	struct mem_record *mr = NULL;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		ret = plat_mem_alloc(&g_mem_mgr, mem);
		break;
	case APU_COMM_MEM_VLM:
	default:
		comm_err("invalid argument\n");
		ret = -EINVAL;
		break;
	}
	if (!ret) {
		mr = vmalloc(sizeof(struct mem_record));
		if (mr != NULL) {
			comm_print_kmem("alloc", mem);
			memcpy(&mr->kmem, mem,
				sizeof(struct comm_kmem));

			INIT_LIST_HEAD(&mr->m_list);
			mutex_lock(&g_mem_mgr.list_mtx);
			list_add_tail(&mr->m_list,
				&g_mem_mgr.list);
			mutex_unlock(&g_mem_mgr.list_mtx);
		}
	}

	return ret;
}

int comm_mem_free(struct comm_kmem *mem, u32 closefd)
{
	int ret = -EFAULT;
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		mutex_lock(&g_mem_mgr.list_mtx);
		list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
			mr = list_entry(list_ptr, struct mem_record, m_list);
			if (mr->kmem.iova == mem->iova &&
				mr->kmem.size == mem->size) {
				ret = plat_mem_free(&g_mem_mgr, &mr->kmem, closefd);
				comm_print_kmem("free", &mr->kmem);
				list_del(&mr->m_list);
				vfree(mr);
				break;
			}
		}
		mutex_unlock(&g_mem_mgr.list_mtx);

		break;
	case APU_COMM_MEM_VLM:
	default:
		comm_err("invalid argument\n");
		break;
	}

	if (ret)
		comm_err("[error] %s: iova:%llx, fd:%d, close:%u\n",
				__func__, mem->iova, mem->fd, closefd);

	return ret;
}

int comm_mem_cache_flush(struct comm_kmem *mem)
{
	int ret = 0;
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		//ret = plat_mem_cache_flush(&g_mem_mgr, mem);
		mutex_lock(&g_mem_mgr.list_mtx);
		list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
			mr = list_entry(list_ptr, struct mem_record, m_list);
			if (mr->kmem.fd == mem->fd &&
				mem->size <= mr->kmem.size && mr->kmem.cache >= 1) {
				mem->iova = mr->kmem.iova;
				mem->kva = mr->kmem.kva;
				mem->sgt = mr->kmem.sgt;
				ret = plat_mem_cache_flush(&g_mem_mgr, mem);
				//ret = plat_mem_free(&g_mem_mgr, &mr->kmem, closefd);
				comm_print_kmem("flush", &mr->kmem);
				//list_del(&mr->m_list);
				//vfree(mr);
				break;
			}
		}
		mutex_unlock(&g_mem_mgr.list_mtx);
		break;
	case APU_COMM_MEM_VLM:
	default:
		comm_drv_debug("invalid argument\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

int comm_mem_import(struct comm_kmem *mem)
{
	int ret = 0;
	struct mem_record *mr = NULL;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		ret = plat_mem_import(&g_mem_mgr, mem);
		break;
	default:
		comm_drv_debug("invalid argument\n");
		ret = -EINVAL;
		break;
	}

	if (!ret) {
		mr = vmalloc(sizeof(struct mem_record));
		if (mr != NULL) {
			comm_print_kmem("import", mem);
			memcpy(&mr->kmem, mem,
				sizeof(struct comm_kmem));

			INIT_LIST_HEAD(&mr->m_list);
			mutex_lock(&g_mem_mgr.list_mtx);
			list_add_tail(&mr->m_list,
				&g_mem_mgr.list);
			mutex_unlock(&g_mem_mgr.list_mtx);
		}
	}
	return ret;
}

int comm_mem_unimport(struct comm_kmem *mem)
{
	int ret = 0;
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		ret = plat_mem_unimport(&g_mem_mgr, mem);
		break;
	default:
		comm_drv_debug("invalid argument\n");
		ret = -EINVAL;
		break;
	}

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		if (mr->kmem.iova == mem->iova) {
			comm_print_kmem("unimport", mem);
			list_del(&mr->m_list);
			vfree(mr);
			break;
		}
	}
	mutex_unlock(&g_mem_mgr.list_mtx);

	return ret;
}

#ifdef COMM_RELEASE
int comm_mem_release(struct comm_kmem *mem)
{
	int ret = 0;

	switch (mem->property) {
	case APU_COMM_MEM_PROP_ALLOC:
		ret = comm_mem_free(mem);
		break;
	case APU_COMM_MEM_PROP_IMPORT:
		ret = comm_mem_unimport(mem);
		break;
	default:
		comm_drv_debug("invalid argument\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

int comm_mem_flush(struct comm_kmem *mem)
{
	int ret = 0;

	mdw_lne_debug();

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		ret = ion_wrp_mem_flush(&g_mem_mgr, mem);
		break;
	case APU_COMM_MEM_DRAM_DMA:
		ret = -EINVAL;
		break;
	default:
		comm_drv_debug("invalid argument\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}
EXPORT_SYMBOL(comm_mem_flush);

int comm_mem_invalidate(struct comm_kmem *mem)
{
	int ret = 0;

	mdw_lne_debug();

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		ret = ion_wrp_mem_invalidate(&g_mem_mgr, mem);
		break;
	case APU_COMM_MEM_DRAM_DMA:
		ret = -EINVAL;
		break;
	default:
		comm_drv_debug("invalid argument\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}
EXPORT_SYMBOL(apusys_mem_invalidate);
#endif

int comm_mem_init(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	g_mem_mgr.dev = dev;

	INIT_LIST_HEAD(&g_mem_mgr.list);
	mutex_init(&g_mem_mgr.list_mtx);

	ret = plat_mem_init(&g_mem_mgr);

	plat_dev_init(pdev);

	return ret;

}

int comm_mem_destroy(void)
{
	int ret = 0;

	plat_dev_uninit();
	ret = plat_mem_destroy(&g_mem_mgr);

	return ret;
}

#ifdef COMM_CTL
int comm_mem_ctl(struct apusys_mem_ctl *ctl_data, struct comm_kmem *mem)
{
	int ret = 0;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		ret = -EINVAL;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}
#endif

int comm_mem_map_iova(struct comm_kmem *mem)
{
	int ret = -EFAULT;
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
		break;
	case APU_COMM_MEM_DRAM_DMA:
		mutex_lock(&g_mem_mgr.list_mtx);
		list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
			mr = list_entry(list_ptr, struct mem_record, m_list);
			if (mem->iova == mr->kmem.iova) {
				mem->iova = mr->kmem.iova;
				mem->kva = mr->kmem.kva;
				mem->size = mr->kmem.size;
				mem->iova_size = mr->kmem.size;
				mem->status = mr->kmem.status;
				ret = 0;
				break;
			}
		}
		mutex_unlock(&g_mem_mgr.list_mtx);

		break;
	default:
		break;
	}

	return ret;
}

int comm_mem_map_kva(struct comm_kmem *mem)
{
	int ret = -EFAULT;
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;
	u64 iova;
	u64 buf_ofs;
	u32 size;
	u64 auto_ofs;

	iova = mem->iova;
	size = mem->size;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
		break;
	case APU_COMM_MEM_DRAM_DMA:
		iova |= IOVA_START_ADDR;
		break;
	default:
		break;
	}

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		if ((mem->mem_type == APU_COMM_MEM_AUTO) &&
			(mr->kmem.mem_type != APU_COMM_MEM_VLM))
			auto_ofs = IOVA_START_ADDR;
		else
			auto_ofs = 0;

		if ((iova + auto_ofs) >= mr->kmem.iova &&
			size <= mr->kmem.size &&
			(iova + auto_ofs) + size <= mr->kmem.iova + mr->kmem.size) {
			buf_ofs = (iova + auto_ofs) - mr->kmem.iova;
			mem->iova = mr->kmem.iova + buf_ofs;
			mem->kva = mr->kmem.kva + buf_ofs;
			mem->size = mem->size ?
				size : (mr->kmem.size - (u32)buf_ofs);
			mem->iova_size = mr->kmem.size;
			mem->status = mr->kmem.status;
			ret = 0;
			break;
		}
	}
	mutex_unlock(&g_mem_mgr.list_mtx);

	return ret;
}

int comm_mem_unmap_iova(struct comm_kmem *mem)
{
	int ret = 0;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int comm_mem_unmap_kva(struct comm_kmem *mem)
{
	int ret = 0;

	switch (mem->mem_type) {
	case APU_COMM_MEM_DRAM_ION:
	case APU_COMM_MEM_DRAM_ION_AOSP:
		// [TODO] ADD for ION PATH
	case APU_COMM_MEM_DRAM_DMA:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int comm_mem_map_uva(struct vm_area_struct *vma)
{
	int found = 0;
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;
	unsigned long size, align_size;
	u64 phy;
	u64 addr;
	int ret = -EINVAL;

	phy = (vma->vm_pgoff) << PAGE_SHIFT;
	size = vma->vm_end - vma->vm_start;

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		addr = (mr->kmem.mem_type == APU_COMM_MEM_VLM) ?
					phy : phy | IOVA_START_ADDR;
		align_size = ALIGN(mr->kmem.size, PAGE_SIZE);
		if (addr >= mr->kmem.phy &&
			size <= align_size &&
			addr + size <= mr->kmem.phy + align_size) {
			found = 1;
			ret = plat_mem_map_uva(&g_mem_mgr, vma, &mr->kmem);
			break;
		}
	}

	if (!found)
		comm_err("%s: addr:%llx size:%lu not valid\n", __func__, phy, size);

	mutex_unlock(&g_mem_mgr.list_mtx);

	return ret;
}

unsigned int comm_mem_get_support(void)
{
	unsigned int mem_support = 0;

#ifdef APU_COMM_OPTIONS_MEM_ION
	mem_support |= (1UL << APU_COMM_MEM_DRAM_ION);
#endif

#ifdef APU_COMM_OPTIONS_MEM_DMA
	mem_support |= (1UL << APUSYS_MEM_DRAM_DMA);
#endif

	return mem_support;
}

void comm_mem_suspend(void)
{
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		if (mr->kmem.secure)
			mr->kmem.status |= (1 << COMM_MEM_OBSOLETE);
	}
	mutex_unlock(&g_mem_mgr.list_mtx);
}

static int comm_mem_query_by_iova(struct comm_kmem *mem)
{
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;
	u64 iova;
	int ret = -EFAULT;

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		if ((mem->mem_type == APU_COMM_MEM_AUTO) &&
			(mr->kmem.mem_type != APU_COMM_MEM_VLM))
			iova = mem->iova + IOVA_START_ADDR;
		else
			iova = mem->iova;

		if (iova >= mr->kmem.iova &&
			mem->size <= mr->kmem.size &&
			iova < mr->kmem.iova + mr->kmem.size) {
			memcpy(mem, &mr->kmem, sizeof(*mem));
			ret = 0;
			break;
		}
	}
	mutex_unlock(&g_mem_mgr.list_mtx);

	return ret;
}

static int comm_mem_query_by_kva(struct comm_kmem *mem)
{
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;
	int ret = -EFAULT;

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		if (mr->kmem.kva <= mem->kva &&
			mem->size <= mr->kmem.size &&
			mr->kmem.kva + mr->kmem.size > mem->kva) {
			memcpy(mem, &mr->kmem, sizeof(*mem));
			ret = 0;
			break;
		}
	}
	mutex_unlock(&g_mem_mgr.list_mtx);

	return ret;
}

static int comm_mem_query_by_fd(struct comm_kmem *mem)
{
	struct mem_record *mr = NULL;
	struct list_head *tmp = NULL, *list_ptr = NULL;
	int ret = -EFAULT;

	mutex_lock(&g_mem_mgr.list_mtx);
	list_for_each_safe(list_ptr, tmp, &g_mem_mgr.list) {
		mr = list_entry(list_ptr, struct mem_record, m_list);
		if (mem->fd == mr->kmem.fd) {
			memcpy(mem, &mr->kmem, sizeof(*mem));
			ret = 0;
			break;
		}
	}
	mutex_unlock(&g_mem_mgr.list_mtx);

	return ret;
}

int comm_mem_query_mem(struct comm_kmem *mem, u32 query_type)
{
	int ret = -EFAULT;

	switch (query_type) {
	case COMM_QUERY_BY_FD:
		ret = comm_mem_query_by_fd(mem);
		break;
	case COMM_QUERY_BY_IOVA:
		ret = comm_mem_query_by_iova(mem);
		break;
	case COMM_QUERY_BY_KVA:
		ret = comm_mem_query_by_kva(mem);
		break;
	default:
		break;
	}

	return ret;
}
