/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joe Liu <joe.liu@mediatek.com>
 */

struct of_mmap_info_data {
	u32 layer;
	u32 miu;
	u32 is_cache;
	u32 cma_id;
	u64 start_bus_address;
	u64 buffer_size;
};

extern int mtk_nullmemory_heap_create(void);
extern int mtk_mmap_test_init(struct device *mtk_test_mmap_device);

struct device *mtkcma_id_device_mapping(unsigned int mtkcma_id);
int of_mtk_get_reserved_memory_info_by_idx(struct device_node *np, int idx,
			struct of_mmap_info_data *of_mmap_info);
