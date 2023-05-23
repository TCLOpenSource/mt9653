/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Joe Liu <joe.liu@mediatek.com>
 */

/*
 * SiP commands
 */
#define NULLMEM_MTK_SIP_RIU_ERROR_MASK_CTRL            BIT(0)
#define MTK_SIP_NULLMEM_CONTROL                0x8200000e

extern u64 mtk_mem_next_cell(int s, const __be32 **cellp);
extern int root_addr_cells;
extern int root_size_cells;
