// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joe Liu <joe.liu@mediatek.com>
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ion.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direction.h>
#include <linux/cma.h>

#include "cma.h"
#include "mtk-reserved-memory.h"
#include "mtk-cma.h"

int root_addr_cells = 2;
int root_size_cells = 2;

struct device *mtk_mmap_device;

struct mtkcma_id_to_device {
	unsigned int cma_id;
	struct device mtkcma_device;
};

struct mtkcma_id_to_device mtkcma_id_device_list[MAX_CMA_AREAS];
static unsigned int mtkcma_count;

static const struct of_device_id mtk_mmap_device_driver_of_ids[] = {
	{.compatible = "mediatek,tv-reserved-memory",},
	{},
};

u64 mtk_mem_next_cell(int s, const __be32 **cellp)
{
	const __be32 *p = *cellp;

	*cellp = p + s;
	return of_read_number(p, s);
}
EXPORT_SYMBOL(mtk_mem_next_cell);

/*
 * of_mtk_get_reserved_memory_info_by_idx
 * - Get the mmap info by the memory-region index of a device_node
 *
 * @np: device node which needs to query its reserved memory info
 * @idx: reserved memory idx
 * @of_mmap_info: Returned node info
 *
 * Use the given device_node and index to search the mmap_info device_node.
 * All property of this found child device node will be stored in of_mmap_info.
 */
int of_mtk_get_reserved_memory_info_by_idx(struct device_node *np, int idx,
	struct of_mmap_info_data *of_mmap_info)
{
	struct device_node *target_memory_np = NULL;
	uint32_t len = 0;
	__be32 *p, *endp = NULL;

	if (np == NULL) {
		pr_err("np is NULL\n");
		return -ENXIO;
	}

	if (of_mmap_info == NULL) {
		pr_err("np %s is @ 0x%lX, of_mmap_info is NULL\n",
			np->name, (unsigned long)np);
		return -ENXIO;
	}

	target_memory_np = of_parse_phandle(np, "memory-region", idx);
	if (!target_memory_np)
		return -ENODEV;

	p = (__be32 *)of_get_property(target_memory_np, "layer", &len);
	if (p != NULL) {
		of_mmap_info->layer = be32_to_cpup(p);
		p = NULL;
	}

	p = (__be32 *)of_get_property(target_memory_np, "miu", &len);
	if (p != NULL) {
		of_mmap_info->miu = be32_to_cpup(p);
		p = NULL;
	}

	p = (__be32 *)of_get_property(target_memory_np, "is_cache", &len);
	if (p != NULL) {
		of_mmap_info->is_cache = be32_to_cpup(p);
		p = NULL;
	}

	p = (__be32 *)of_get_property(target_memory_np, "cma_id", &len);
	if (p != NULL) {
		of_mmap_info->cma_id = be32_to_cpup(p);
		p = NULL;
	}

	p = (__be32 *)of_get_property(target_memory_np, "reg", &len);
	if (p != NULL) {
		endp = p + (len / sizeof(__be32));

		while ((endp - p) >=
			(root_addr_cells + root_size_cells)) {
			of_mmap_info->start_bus_address =
				mtk_mem_next_cell(root_addr_cells, (const __be32 **)&p);
			of_mmap_info->buffer_size =
				mtk_mem_next_cell(root_size_cells, (const __be32 **)&p);
		}
	}

	of_node_put(target_memory_np);
	return 0;
}
EXPORT_SYMBOL(of_mtk_get_reserved_memory_info_by_idx);

struct device *mtkcma_id_device_mapping(unsigned int mtkcma_id)
{
	int i;

	for (i = 0; i < mtkcma_count; i++) {
		if (mtkcma_id_device_list[i].cma_id == mtkcma_id)
			return &mtkcma_id_device_list[i].mtkcma_device;
	}

	return NULL;
}
EXPORT_SYMBOL(mtkcma_id_device_mapping);

int mtk_ion_alloc(size_t len, unsigned int heap_id_mask,
	unsigned int flags)
{
	int fd;
	struct dma_buf *dmabuf;

	dmabuf = ion_alloc(len, heap_id_mask, flags);
	if (IS_ERR(dmabuf))
		return PTR_ERR(dmabuf);

	fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	if (fd < 0)
		dma_buf_put(dmabuf);

	return fd;
}
EXPORT_SYMBOL(mtk_ion_alloc);

int ion_test(struct device *dev, unsigned long alloc_offset,
			unsigned long size, unsigned int flags)
{
	struct ion_heap_data *data;
	unsigned int i = 0;
	unsigned int cma_heap_id = -1;
	int ret = 0;
	int fd = -1;
	struct ion_allocation_data cma_alloc_data = {0};
	size_t heap_cnt = 0;
	size_t kmalloc_size = 0;

	heap_cnt = ion_query_heaps_kernel(NULL, heap_cnt);
	kmalloc_size = sizeof(struct ion_heap_data) * heap_cnt;
	data = (struct ion_heap_data *)
		kmalloc(kmalloc_size, GFP_KERNEL);
	if (!data) {
		pr_emerg("Function = %s, no data!\n", __PRETTY_FUNCTION__);
		return -ENOMEM;
	}
	heap_cnt = ion_query_heaps_kernel(data, heap_cnt);

	pr_info("\n\ncurrently, query heap cnt is %u\n\n", heap_cnt);

	for (i = 0; i < heap_cnt; i++) {
		pr_info("data[%d] name %s\n", i, data[i].name);
		ret = strncmp(data[i].name, dev_name(dev), MAX_HEAP_NAME);
		if (ret == 0) {
			cma_heap_id = data[i].heap_id;
			pr_info("    get cma_heap_id %u\n", cma_heap_id);
		}
	}
	if (cma_heap_id == -1) {
		pr_emerg("Function = %s, get cma_heap_id failed\n",
			__PRETTY_FUNCTION__);
		goto invalid_cma_heap_id;
	} else
		pr_info("get ion test cma_heap_id %d OK\n", cma_heap_id);

	cma_alloc_data.len = size;
	cma_alloc_data.heap_id_mask = 1 << cma_heap_id;
	fd = mtk_ion_alloc(size, 1 << cma_heap_id,
		((alloc_offset >> PAGE_SHIFT) << PAGE_SHIFT) | flags);

	pr_info("[ION] get fd %d\n", fd);

invalid_cma_heap_id:
	kfree(data);
	return fd;
}
EXPORT_SYMBOL(ion_test);

static int mtk_mmap_device_driver_probe(struct platform_device *pdev)
{
	int i;
	int ret = -1;
	struct device_node *device_node_with_cma = NULL;
	struct device *device_with_cma = NULL;
	__be32 *p = NULL;
	u32 pool_index = 0;
	uint32_t len = 0;

	if (strstr(dev_name(&pdev->dev), "mtk_utopia_cma_device")) {
		pr_info("do utopia setting\n");
		mtkcma_presetting_utopia(&pdev->dev, 0);

		/* check if cma exist, sometimes, mtkcma_presetting_utopia may fail */
		if (!pdev->dev.cma_area) {
			pr_emerg("cma_init for %s error\n", pdev->name);
			return 0;
		}

		/* set mtkcma_device */
		mtkcma_id_device_list[mtkcma_count].mtkcma_device = pdev->dev;

		p = (__be32 *)of_get_property(pdev->dev.of_node, "mtk_cma_id", &len);
		if (p != NULL)
			mtkcma_id_device_list[mtkcma_count].cma_id = be32_to_cpup(p);
		else
			pr_emerg("get mtk_cma_id for %s error\n", pdev->name);

		mtkcma_count++;

		pr_info("    platform_device name is %s\n",
				pdev->name);
		pr_info("    platform_device->device name is %s\n",
			dev_name(mtk_mmap_device));

		for (i = 0; i < mtkcma_count; i++) {
			pr_emerg("(%d)id%u, cma from 0x%lX, size 0x%lX\n",
			i, mtkcma_id_device_list[i].cma_id,
			(mtkcma_id_device_list[i].mtkcma_device.cma_area->base_pfn << PAGE_SHIFT),
			(mtkcma_id_device_list[i].mtkcma_device.cma_area->count * PAGE_SIZE));
		}
		return 0;
	}

	mtk_mmap_device = &pdev->dev;
	pr_info("\n\n[MTK-MEMORY] probe start !!\n");
	pr_info("    platform_device is @ 0x%lX\n",
				(unsigned long)pdev);

	if (pdev->name)
		pr_info("    platform_device name is %s\n",
			pdev->name);

	pr_info("    platform_device->device name is %s\n",
		dev_name(mtk_mmap_device));

	/* initialize nullmem heap */
	ret = mtk_nullmemory_heap_create();
	if (ret < 0) {
		pr_emerg("mtk nullmem failed to create heap\n");
		return ret;
	}

	pr_info("\n\nauto create mtkcma ion heap... start\n");
	for_each_node_with_property(device_node_with_cma, "cmas") {
		if (!of_device_is_available(device_node_with_cma)) {
			pr_emerg("device_node %s is not available\n",
				device_node_with_cma->name);
			continue;
		}

		device_with_cma = bus_find_device_by_of_node(&platform_bus_type,
			device_node_with_cma);
		if (!device_with_cma) {
			pr_emerg("device_node %s has no device\n",
				device_node_with_cma->name);
			continue;
		}

		p = (__be32 *)of_get_property(device_node_with_cma, "cmas", &len);
		if (p != NULL) {
			pool_index = be32_to_cpup(p);
			pr_info("for device_node %s, use pool_index %u\n",
				device_node_with_cma->name, pool_index);
		} else {
			pr_emerg("can not get cma pool index for device_node %s\n",
				device_node_with_cma->name);
			continue;
		}

		/* do mtkcma_presetting_v2() */
		if (!strstr(dev_name(device_with_cma), "mtk_utopia_cma_device"))
			mtkcma_presetting_v2(device_with_cma, (int)pool_index);

		/* due to of_find_device_by_node() */
		put_device(device_with_cma);

		/* due to for_each_node_with_property() */
		of_node_put(device_node_with_cma);
	}
	pr_info("auto create mtkcma ion heap... finish\n\n");
	return 0;
}

static int mtk_mmap_device_driver_remove(struct platform_device *pdev)
{
	pr_emerg("mtk_mmap_device_driver_exit\n");
	return 0;
}

static struct platform_driver mtk_mmap_device_driver = {
	.probe = mtk_mmap_device_driver_probe,
	.remove = mtk_mmap_device_driver_remove,
	.driver = {
		.name = "mtk-mmap-test",
		.of_match_table = mtk_mmap_device_driver_of_ids,
	}
};

static int __init mtk_mmap_device_init(void)
{
	mtkcma_count = 0;
	return platform_driver_register(&mtk_mmap_device_driver);
}

static void __exit mtk_mmap_device_exit(void)
{
	mtkcma_count = 0;
	platform_driver_unregister(&mtk_mmap_device_driver);
}

module_init(mtk_mmap_device_init);
module_exit(mtk_mmap_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mtk");
MODULE_DESCRIPTION("mtk mmap driver");
