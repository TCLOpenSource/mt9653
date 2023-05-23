// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joe Liu <joe.liu@mediatek.com>
 */

#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-direct.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/buffer_head.h>
#include <linux/dma-buf.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/bitmap.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/ion.h>
#include <linux/arm-smccc.h>

#include "mtk-cma.h"
#include "cma.h"
#include "mtk-null-memory.h"

#define NULL_MEMORY_SIZE 0x10000000
#define NULL_MEMORY_CNT 1

struct mtk_nullmemory_heap *nullmemory_heap[NULL_MEMORY_CNT];

#define to_mtk_nullmemory_heap(x) container_of(x, struct mtk_nullmemory_heap, heap)

#define nullmem_mtk_smc(cmd, val, res) \
	arm_smccc_smc(MTK_SIP_NULLMEM_CONTROL, cmd, val, 0, 0, 0, 0, 0, &(res))

#define nullmem_mtk_riu_ctrl(res, set_bit) \
	nullmem_mtk_smc(NULLMEM_MTK_SIP_RIU_ERROR_MASK_CTRL, set_bit, res)

static bool is_nullheap_created;

struct mtk_nullmemory_heap {
	unsigned long start_bus_address;
	unsigned long buffer_size;
	unsigned long page_cnt;
	struct ion_heap heap;
	unsigned long *bitmap;
	spinlock_t lock;
	int add_heap_result;
	unsigned long remain_size;
};

bool fake_bitmap_clear(struct mtk_nullmemory_heap *fake_heap, const struct page *pages,
			unsigned int count)
{
	unsigned long bus_address;
	unsigned long start_bus_address;
	unsigned long end_bus_address;
	unsigned long bitmap_no;

	start_bus_address = fake_heap->start_bus_address;
	end_bus_address = fake_heap->start_bus_address + fake_heap->buffer_size;
	bus_address = page_to_phys(pages);

	if (bus_address < start_bus_address)
		return false;

	if (bus_address >= end_bus_address)
		return false;

	VM_BUG_ON(bus_address + (count << PAGE_SHIFT) > end_bus_address);

	bitmap_no = (bus_address - start_bus_address) >> PAGE_SHIFT;

	spin_lock(&fake_heap->lock);
	bitmap_clear(fake_heap->bitmap, bitmap_no, count);
	spin_unlock(&fake_heap->lock);
	return true;
}

static void update_error_response_mask(struct mtk_nullmemory_heap *fake_heap)
{
	struct arm_smccc_res res;

	if (fake_heap->buffer_size > fake_heap->remain_size) {
		// set mask bit to be 1
		nullmem_mtk_riu_ctrl(res, 1);
	} else if (fake_heap->buffer_size == fake_heap->remain_size) {
		// set mask bit to be 0
		nullmem_mtk_riu_ctrl(res, 0);
	} else {
		pr_emerg("un-known case, panic\n");
		panic("nullmem: update error response mask error\n");
	}
}

static int mtk_nullmemory_allocate(struct ion_heap *heap,
			struct ion_buffer *buffer,
			unsigned long len, unsigned long flags)
{
	struct mtk_nullmemory_heap *fake_heap = to_mtk_nullmemory_heap(heap);
	struct sg_table *table;
	struct page *pages;
	int ret;
	unsigned long size = PAGE_ALIGN(len);
	unsigned long nr_pages = size >> PAGE_SHIFT;
	unsigned long pageno;

	spin_lock(&fake_heap->lock);
	if (size > fake_heap->remain_size) {
		pr_emerg("Function = %s, no enough nullmem size\n",
				__PRETTY_FUNCTION__);
		spin_unlock(&fake_heap->lock);
		return -ENOMEM;
	}

	pageno = bitmap_find_next_zero_area_off(fake_heap->bitmap, fake_heap->page_cnt,
		0, nr_pages, 0, 0);

	if (pageno >= fake_heap->page_cnt) {
		spin_unlock(&fake_heap->lock);
		pr_emerg("Function = %s, no page\n",
				__PRETTY_FUNCTION__);
		return -ENOMEM;
	}
	bitmap_set(fake_heap->bitmap, pageno, nr_pages);
	spin_unlock(&fake_heap->lock);

	pages = pfn_to_page(__phys_to_pfn(fake_heap->start_bus_address) + pageno);

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto err;

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto free_mem;

	sg_set_page(table->sgl, pages, size, 0);

	buffer->priv_virt = pages;
	buffer->sg_table = table;

	spin_lock(&fake_heap->lock);
	fake_heap->remain_size -= size;
	update_error_response_mask(fake_heap);
	spin_unlock(&fake_heap->lock);

	return 0;

free_mem:
	kfree(table);

err:
	fake_bitmap_clear(fake_heap, pages, nr_pages);
	return -ENOMEM;
}

static void mtk_nullmemory_free(struct ion_buffer *buffer)
{
	struct mtk_nullmemory_heap *fake_heap = to_mtk_nullmemory_heap(buffer->heap);
	struct page *pages = buffer->priv_virt;
	unsigned long nr_pages = PAGE_ALIGN(buffer->size) >> PAGE_SHIFT;

	fake_bitmap_clear(fake_heap, pages, nr_pages);
	/* release sg table */
	sg_free_table(buffer->sg_table);
	kfree(buffer->sg_table);

	spin_lock(&fake_heap->lock);
	fake_heap->remain_size += nr_pages << PAGE_SHIFT;
	update_error_response_mask(fake_heap);
	spin_unlock(&fake_heap->lock);
}

static struct ion_heap_ops mtk_nullmemory_ops = {
	.allocate = mtk_nullmemory_allocate,
	.free = mtk_nullmemory_free,
};

int mtk_nullmemory_heap_create(void)
{
	struct ion_heap *heap;
	int i;
	unsigned long start_bus_address;
	unsigned long buffer_size;
	int ret;
	struct device_node *np = NULL;
	__be32 *p, *endp = NULL;
	uint32_t len = 0;

	/* we assume that each nullmemory is 256MB */
	int bitmap_size = BITS_TO_LONGS(NULL_MEMORY_SIZE >> PAGE_SHIFT) * sizeof(long);

	if (is_nullheap_created)
		return 0;

	for (i = 0; i < NULL_MEMORY_CNT; i++) {
		np = of_find_node_by_type(np, "mtk-nullmem");
		if (np == NULL) {
			pr_err("[%d] not find device node\033[m\n", i);
			return 0;
		}

		p = (__be32 *)of_get_property(np, "reg", &len);
		if (p != NULL) {
			endp = p + (len / sizeof(__be32));

			while ((endp - p) >=
				(root_addr_cells + root_size_cells)) {
				start_bus_address =
					mtk_mem_next_cell(root_addr_cells, (const __be32 **)&p);
				buffer_size =
					mtk_mem_next_cell(root_size_cells, (const __be32 **)&p);
			}
		} else {
			pr_err("can not find memory info reg, name is %s\n",
				np->name);
			of_node_put(np);
			ret = -EINVAL;
			goto nullmem_error_case;
		}
		of_node_put(np);
		nullmemory_heap[i] = kzalloc(sizeof(struct mtk_nullmemory_heap), GFP_KERNEL);

		if (IS_ERR(nullmemory_heap[i])) {
			pr_emerg("Function = %s, no nullmemory_heap\n", __PRETTY_FUNCTION__);
			ret = PTR_ERR(-ENOMEM);
			goto nullmem_error_case;
		}

		nullmemory_heap[i]->add_heap_result = -1;
		nullmemory_heap[i]->start_bus_address = start_bus_address;
		nullmemory_heap[i]->buffer_size = buffer_size;
		nullmemory_heap[i]->remain_size = buffer_size;
		nullmemory_heap[i]->bitmap = kzalloc(bitmap_size, GFP_KERNEL);
		nullmemory_heap[i]->page_cnt = NULL_MEMORY_SIZE >> PAGE_SHIFT;
		spin_lock_init(&nullmemory_heap[i]->lock);

		if (!nullmemory_heap[i]->bitmap) {
			pr_emerg("Function = %s, no bitmap\n", __PRETTY_FUNCTION__);
			ret = -ENOMEM;
			goto nullmem_error_case;
		}

		nullmemory_heap[i]->heap.ops = &mtk_nullmemory_ops;
		nullmemory_heap[i]->heap.type = ION_HEAP_TYPE_CUSTOM;

		heap = &nullmemory_heap[i]->heap;

		heap->name = np->name;
		pr_emerg("add null memory ion_heap with name %s\n",
			heap->name);
		pr_emerg("    range from 0x%lX to 0x%lX\n\n\n",
			start_bus_address, (start_bus_address + buffer_size));

		nullmemory_heap[i]->add_heap_result = ion_device_add_heap(heap);
	}

	is_nullheap_created = true;
	return 0;

nullmem_error_case:
	pr_emerg("fail, do nullmem_error_case\n");
	for (i = 0; i < NULL_MEMORY_CNT; i++) {
		if (nullmemory_heap[i]) {
			kfree(nullmemory_heap[i]->bitmap);

			if (nullmemory_heap[i]->add_heap_result == 0)
				ion_device_remove_heap(&nullmemory_heap[i]->heap);
			kfree(nullmemory_heap[i]);
		}
	}
	return ret;
}
EXPORT_SYMBOL(mtk_nullmemory_heap_create);
