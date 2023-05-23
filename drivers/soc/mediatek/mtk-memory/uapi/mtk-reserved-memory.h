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
