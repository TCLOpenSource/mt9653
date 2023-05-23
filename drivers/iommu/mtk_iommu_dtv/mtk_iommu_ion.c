// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/ion.h>

#include "mtk_iommu_ion.h"
#include "mtk_iommu_dtv.h"
#include "heaps/mtk_iommu_sys_heap.h"
#include "heaps/mtk_iommu_carveout_heap.h"
#include "heaps/mtk_iommu_ion_heap.h"
#include "heaps/mtk_iommu_cma_heap.h"

// iommu sys heap
static unsigned int mtk_sys_heapid = UINT_MAX;
// iommu cma heap
static unsigned int mtk_cma_heapid = UINT_MAX;
// iommu carveout heap
static unsigned int mtk_carveout_heapid = UINT_MAX;

static int _ion_heap_query(char *heapname, unsigned int *heapid)
{
	int i;
	size_t nr_heaps;
	struct ion_heap_data *data;
	unsigned int id = UINT_MAX;

	if (!heapname || !heapid) {
		pr_err("%s: invailed input\n", __func__);
		return -EINVAL;
	}

	nr_heaps = ion_query_heaps_kernel(NULL, 0);
	if (nr_heaps == 0)
		goto err;

	data = kcalloc(nr_heaps, sizeof(struct ion_heap_data), GFP_KERNEL);
	if (!data)
		goto err;

	if (ion_query_heaps_kernel(data, nr_heaps) == 0) {
		kfree(data);
		goto err;
	}

	for (i = 0; i < nr_heaps; i++) {
		if (strcmp(data[i].name, heapname) == 0) {
			id = data[i].heap_id;
			break;
		}
	}
	kfree(data);

	if (id == UINT_MAX)
		goto err;

	*heapid = id;

	pr_debug("%s: found [%s] is heap id %d/%d\n",
			__func__, heapname, i, (int)nr_heaps);

	return 0;
err:
	pr_err("%s: cannot find heap [%s]\n", __func__, heapname);
	return -ENOENT;
}

int mtk_iommu_ion_query_heap(void)
{
	int ret;

	ret = _ion_heap_query(MTK_ION_SYS_HEAP_NAME, &mtk_sys_heapid);
	if (ret)
		return -EINVAL;

	ret = _ion_heap_query(MTK_ION_CMA_HEAP_NAME, &mtk_cma_heapid);
	if (ret)
		return -EINVAL;

#if !IS_ENABLED(CONFIG_DMABUF_HEAPS_SYSTEM)
	ret = _ion_heap_query(MTK_ION_CARVEOUT_HEAP_NAME, &mtk_carveout_heapid);
	if (ret)
		return -EINVAL;
#endif
	return 0;
}

int mtk_iommu_get_id_flag(int heap_type, int miu, int zone_flag,
		bool secure, unsigned int *heap_mask, unsigned int *ion_flag,
		u32 size, const char *buf_tag, unsigned long attrs)
{
	unsigned int flag = 0;

	if (mtk_sys_heapid == UINT_MAX) {
		pr_crit("%s: mtk_sys_heap not support!\n", __func__);
		return -EINVAL;
	}

	switch (heap_type) {
	case HEAP_TYPE_IOMMU:
		*heap_mask = 1 << mtk_sys_heapid;
		break;
	case HEAP_TYPE_CMA_IOMMU:
		// TODO: cma heap
		*heap_mask = 1 << mtk_cma_heapid;
		if (miu == 0)
			flag |= IOMMUMD_FLAG_CMA0;
		else if (miu == 1)
			flag |= IOMMUMD_FLAG_CMA1;
		else {
			pr_err("%s: miu %d not support!\n", __func__, miu);
			WARN_ON(1);
			return -EINVAL;
		}
		break;
	case HEAP_TYPE_CARVEOUT:
		// open carveout heap
		if (mtk_carveout_heapid == UINT_MAX) {
			pr_err("%s: mtk_carveout_heap not support!\n", __func__);
			return -EINVAL;
		}
		*heap_mask = 1 << mtk_carveout_heapid;
		break;
	default:
		pr_err("%s: heap type %d not support!\n", __func__, heap_type);
		return -EINVAL;
	}

	if (strstr(buf_tag, "mali") != NULL)
		flag |= IOMMUMD_FLAG_ZEROPAGE;

	if (!(attrs & DMA_ATTR_WRITE_COMBINE))
		flag |= ION_FLAG_CACHED;

	if (attrs & IOMMUMD_FLAG_ZEROPAGE)
		flag |= IOMMUMD_FLAG_ZEROPAGE;

	if (!zone_flag)
		flag |= IOMMUMD_FLAG_DMAZONE;
	*ion_flag = flag;

	return 0;
}

int mtk_iommu_ion_alloc(size_t size, unsigned int heap_mask,
			unsigned int flags, struct dma_buf **db)
{
	// wrapper for dma-heap compatiable
	if (!size || !heap_mask)
		return -EINVAL;

	*db = ion_alloc(size, heap_mask, flags);
	if (IS_ERR(*db))
		return PTR_ERR(*db);

	return 0;
}

struct sg_table *get_ion_buffer_sgtable(struct dma_buf *db)
{
	struct ion_buffer *buffer;

	if (!db || !db->priv)
		return NULL;

	buffer = (struct ion_buffer *)db->priv;

	return buffer->sg_table;
}
