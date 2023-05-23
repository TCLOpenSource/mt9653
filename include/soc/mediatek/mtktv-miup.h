/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MediaTek DTV MIUP Interface driver
 *
 * Copyright (C) 2019 MediaTek Inc.
 * Author: Max Tsai <max-ch.tsai@mediatek.com>
 */

#ifndef __LINUX_MEMORY_MIUP_H__
#define __LINUX_MEMORY_MIUP_H__

enum {
	MTK_MIUP_RPROT = 0,
	MTK_MIUP_WPROT,
	MTK_MIUP_RWPROT,
	MTK_MIUP_CLEANPROT,
	MTK_MIUP_ATTR_MAX,
};

struct mtk_miup_hitlog {
	u64	cid;
	u64	hit_addr;
	u32	hit_blk;
	/* 0: read; 1: write; 2: read/write */
	u32	attr;
};

struct mtk_miup_para {
	u32	prot_blk;
	u64	start_addr;
	u64	addr_len;
	u32	cid_len;
	u64	*cid;
	s32	attr;
};

//int mtk_miup_set_protect(struct mtk_miup_para miup);
//int mtk_miup_get_protect(u32 prot_type, u32 prot_blk,
//	struct mtk_miup_para *miup);
//int mtk_miup_get_hit_log(struct mtk_miup_hitlog *hit_log);

#endif /* __LINUX_MEMORY_MIUP_H__ */
