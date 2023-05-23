// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "mtk_iommu_ion.h"
#include "mtk_iommu_ion_heap.h"
#include "mtk_iommu_dtv_api.h"
#include "mtk_iommu_sys_heap.h"
#include "mtk_iommu_cma_heap.h"
#include "mtk_iommu_carveout_heap.h"

static int ion_iommu_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
			      unsigned long len, unsigned long flags)
{
	struct ion_iommu_heap *iommu_heap = to_iommu_heap(heap);
	struct device *dev = iommu_heap->dev;
	struct ion_iommu_buffer_info *info;
	unsigned long attrs = flags & (~0xFF);

	if (dma_get_mask(dev) < DMA_BIT_MASK(34)) {
		dev_err(dev, "%s,check IOMMU client fail, mask=0x%llx\n",
			__func__, dma_get_mask(dev));
		return -EINVAL;
	}

	info = kzalloc(sizeof(struct ion_iommu_buffer_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	if (!(flags & ION_FLAG_CACHED))
		attrs |= DMA_ATTR_WRITE_COMBINE;

	attrs |= DMA_ATTR_NO_KERNEL_MAPPING;

	info->cpu_addr = dma_alloc_attrs(dev, len, &(info->handle),
					GFP_HIGHUSER, attrs);

	if (!(info->handle || info->cpu_addr))
		goto err;

	if (dma_mapping_error(dev, info->handle) != 0)
		goto err;

	info->table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!info->table)
		goto free_mem;

	if (dma_get_sgtable_attrs(dev, info->table, info->cpu_addr,
			info->handle, len, attrs))
		goto free_table;
	info->len = len;
	info->magic = ION_IOMMU_MAGIC;
	info->attrs = attrs;
	/* keep this for memory release */
	buffer->priv_virt = info;
	buffer->sg_table = info->table;

	return 0;

free_table:
	kfree(info->table);
free_mem:
	dma_free_attrs(dev, len, info->cpu_addr, info->handle, attrs);
err:
	kfree(info);

	return -ENOMEM;
}

static void ion_iommu_free(struct ion_buffer *buffer)
{
	struct ion_iommu_heap *iommu_heap = to_iommu_heap(buffer->heap);
	struct device *dev = iommu_heap->dev;
	struct ion_iommu_buffer_info *info = buffer->priv_virt;

	if (!info->cpu_addr)
		info->cpu_addr = (void *)0xFFFFFFFF;

	/* release memory */
	dma_free_attrs(dev, buffer->size, info->cpu_addr,
					info->handle, info->attrs);
	/* release sg table */
	sg_free_table(info->table);
	kfree(info->table);
	kfree(info);
}

static struct ion_heap_ops ion_iommu_ops = {
	.allocate = ion_iommu_allocate,
	.free = ion_iommu_free,
};

static struct ion_iommu_heap iommu_heap = {
	.heap = {
		.ops = &ion_iommu_ops,
		.type = ION_HEAP_TYPE_DMA,
		.name = "ion_mtkdtv_iommu_heap",
	}
};

static int mtk_ion_iommu_probe(struct platform_device *pdev)
{
	int ret;

	iommu_heap.dev = &(pdev->dev);

	ret = mtk_sys_heap_init(&(pdev->dev));
	if (ret) {
		dev_err(&pdev->dev, "mtk iommu sys heap init failed\n");
		goto err;
	}

	ret = mtk_cma_heap_init(&(pdev->dev));
	if (ret) {
		dev_err(&pdev->dev, "mtk iommu cma heap init failed\n");
		goto err1;
	}

#if !IS_ENABLED(CONFIG_DMABUF_HEAPS_SYSTEM)
	ret = mtk_carveout_heap_init();
	if (ret) {
		dev_err(&pdev->dev, "mtk iommu carveout heap init failed\n");
		goto err2;
	}
#endif

	ret = mtk_iommu_ion_query_heap();
	if (ret) {
		dev_err(&pdev->dev, "cannot found iommu heap %d\n", ret);
		goto err3;
	}

	ret = ion_device_add_heap(&(iommu_heap.heap));
	if (ret) {
		dev_err(&pdev->dev, "mtk iommu add ion heap failed\n");
		goto err3;
	}

	return 0;

err3:
#if !IS_ENABLED(CONFIG_DMABUF_HEAPS_SYSTEM)
	mtk_carveout_heap_exit();
err2:
#endif
	mtk_cma_heap_exit();
err1:
	mtk_sys_heap_exit();
err:
	return ret;
}

static int mtk_ion_iommu_remove(struct platform_device *pdev)
{
	ion_device_remove_heap(&(iommu_heap.heap));
#if !IS_ENABLED(CONFIG_DMABUF_HEAPS_SYSTEM)
	mtk_carveout_heap_exit();
#endif
	mtk_cma_heap_exit();
	mtk_sys_heap_exit();
	return 0;
}

static const struct of_device_id mtk_ion_iommu_of_ids[] = {
	{.compatible = "mediatek,dtv-iommu-ion",},
	{}
};

static struct platform_driver mtk_ion_iommu_driver = {
	.probe = mtk_ion_iommu_probe,
	.remove = mtk_ion_iommu_remove,
	.driver = {
		   .name = "mtk-ion-iommu",
		   .of_match_table = mtk_ion_iommu_of_ids,
	}
};

int __init mtk_ion_iommu_init(void)
{
	return platform_driver_register(&mtk_ion_iommu_driver);
}

void __exit mtk_ion_iommu_deinit(void)
{
	platform_driver_unregister(&mtk_ion_iommu_driver);
}
