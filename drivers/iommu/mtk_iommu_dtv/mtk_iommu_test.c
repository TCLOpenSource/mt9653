// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/dma-buf.h>

#include "mtk_iommu_dtv.h"
#include "mtk_iommu_of.h"
#include "mtk_iommu_ion.h"
#include "mtk_iommu_common.h"
#include "mtk_iommu_tee_interface.h"
#include "heaps/mtk_iommu_ion_heap.h"

static int _get_ion_heapid(char *heapname)
{
	int i;
	size_t nr_heaps;
	struct ion_heap_data *data;
	unsigned int id = UINT_MAX;

	if (!heapname)
		return -EINVAL;

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

	pr_debug("%s: found [%s] is heap id %d/%d\n",
			__func__, heapname, i, (int)nr_heaps);

	return id;
err:
	pr_err("%s: cannot find heap [%s]\n", __func__, heapname);
	return -ENOENT;
}

int mtk_iommu_test_tee(void)
{
	int ret;
	struct dma_buf *db;
	int heap_id;
	struct ion_buffer *buffer;
	struct sg_table *table;
	size_t size = 10 * 1024 * 1024;
	struct buf_tag_info tag_info = { };

	tag_info.id = 1;	// vdec_fb
	ret = mtk_iommu_get_buftag_info(&tag_info);
	if (ret)
		return ret;

	heap_id = _get_ion_heapid("ion_system_heap");
	if (heap_id >> 5) {
		pr_err("%s: heap id must be 0 to 31\n", __func__);
		return -EINVAL;
	}

	db = ion_alloc(size, 1 << heap_id, ION_FLAG_CACHED);
	if (IS_ERR(db)) {
		pr_err("%s: alloc %zu byte failed\n", __func__, size);
		return -ENOMEM;
	}

	buffer = (struct ion_buffer *)db->priv;
	if (!buffer) {
		pr_err("%s: cannot find heap buffer\n", __func__);
		ret = -ENODEV;
		goto out;
	}
	table = buffer->sg_table;
	if (!table) {
		pr_err("%s: cannot find sgtable\n", __func__);
		ret = -ENODEV;
		goto out;
	}

	ret = mtk_iommu_tee_test(table, tag_info.name);
	if (ret)
		pr_err("%s: tee test fail ret = 0x%x\n", __func__, ret);
	else
		pr_info("%s: tee test success!\n", __func__);

out:
	dma_buf_put(db);
	return ret;
}
