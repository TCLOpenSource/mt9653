// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>

#include <asm/barrier.h>

#include <soc/mediatek/mtktv-miup.h>
#include "mtk-miup.h"

/*********************************************************************
 *                         Private Structure                         *
 *********************************************************************/
static atomic_t init_done = ATOMIC_INIT(false);
static bool en_hit_irq = true;
static bool en_hit_panic = true;

u32 ob_black_client_cnt;
u32 *ob_black_client_list;

#define NULL_MEM_MAX_CNT (10)
static u32 null_mem_cnt;
static struct mtk_nullmem *null_mem;
static struct device_node *p_dev_node;
static struct platform_device *p_pdev;

/* gals ctrl offset, mst sw rst, mst slpprot en, slv sw rst, slv slpprot en , slpprot sw en*/
#define SLPPROT_ENABLE	1
#define SLPPROT_DISABLE	0
static u8 u8mt5896_slpprot[SLPPROT_MT5896_GALS][SLPPROT_MAX_CONTENT] = {
	{0x20,  0,   1,   2,   3, 0x44,  8,   9,  SLPPROT_ENABLE},//pm
};
static u8 u8mt5897_slpprot[SLPPROT_MT5897_GALS][SLPPROT_MAX_CONTENT] = {
	{0x20,  0,   1,   2,   3, 0x44,  8,   9,  SLPPROT_ENABLE},//pm
};
static u8 u8mt5876_slpprot[SLPPROT_MT5876_GALS][SLPPROT_MAX_CONTENT] = {
	{0x20,  0,   1,   2,   3, 0x84,  8,   9,  SLPPROT_ENABLE}, //pm
};
static u8 u8mt5879_slpprot[SLPPROT_MT5879_GALS][SLPPROT_MAX_CONTENT] = {
	{0x20,  0,   1,   2,   3, 0x84,  8,   9, SLPPROT_ENABLE},//pm
};

/*********************************************************************
 *                         Private Function                          *
 *********************************************************************/
static struct device *mtk_miup_get_miup_device(char *dev_node_name)
{
	struct device *dev = NULL;

	if (!p_dev_node)
		p_dev_node = of_find_compatible_node(NULL, NULL, dev_node_name);
	if (!p_dev_node) {
		MIUP_PRE("get node fail");
		goto RET_FUN;
	}
	if (!p_pdev)
		p_pdev = of_find_device_by_node(p_dev_node);
	if (!p_pdev) {
		MIUP_PRE("get pdevice fail");
		goto RET_FUN;
	}
	dev = &p_pdev->dev;
	if (!dev) {
		MIUP_PRE("get device fail");
		goto RET_FUN;
	}

RET_FUN:
	return dev;
}

static int mtk_miup_check_para(struct device *dev,
			struct mtk_miup_para *miup)
{
	u32 idx;
	struct mtk_miup *pdata = dev_get_drvdata(dev);

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	MIUP_DPRI(pdata->loglevel, dev, "protect attribute:%d", miup->attr);
	/* check attr value */
	if (miup->attr >= MTK_MIUP_ATTR_MAX) {
		MIUP_DPRW(pdata->loglevel, dev, "invalid attribute");
		return -EINVAL;
	}

	MIUP_DPRI(pdata->loglevel, dev, "protect block:%d", miup->prot_blk);
	/* check prot_blk index */
	if (miup->prot_blk >= pdata->max_prot_blk) {
		MIUP_DPRW(pdata->loglevel, dev, "invalid block");
		return -EINVAL;
	}

	/* clean protect check done */
	if (miup->attr == MTK_MIUP_CLEANPROT)
		return 0;

	MIUP_DPRI(pdata->loglevel, dev, "start_addr:%llX, addr_len:%llX",
		miup->start_addr, miup->addr_len);
	/* check address */
	if (miup->addr_len < PROT_ADDR_ALIGN ||
	((miup->start_addr & (PROT_ADDR_ALIGN - 1)) != 0) ||
	((miup->addr_len & (PROT_ADDR_ALIGN - 1)) != 0)) {
		MIUP_DPRW(pdata->loglevel, dev, "invalid address");
		return -EINVAL;
	}

	MIUP_DPRI(pdata->loglevel, dev, "cid_len:%d", miup->cid_len);
	/* check cid_len size */
	if (miup->cid_len > pdata->max_cid_number) {
		MIUP_DPRW(pdata->loglevel, dev, "invalid cid length");
		return -EINVAL;
	}
	/* check array cid */
	if (miup->cid_len != 0 && !miup->cid) {
		MIUP_DPRW(pdata->loglevel, dev, "null cid");
		return -EINVAL;
	}
	for (idx = 0; idx < miup->cid_len; idx++) {
		MIUP_DPRI(pdata->loglevel, dev, "cid[%d]:0x%llX", idx, miup->cid[idx]);
		if (miup->cid[idx] > PROT_CID_MAX_INDEX) {
			MIUP_DPRW(pdata->loglevel, dev, "invalid cid");
			return -EINVAL;
		}
	}

	return 0;
}

static int mtk_miup_clean_prot(struct device *dev,
			struct mtk_miup_para miup)
{
	u8 id_grp = 0;
	u32 idx, idx1, idxchk;
	u32 pblk = miup.prot_blk;
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	void __iomem *base = NULL;
	u16 *cids_en = NULL;

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}
	cids_en = kcalloc(pdata->max_cid_group, sizeof(u16), GFP_KERNEL);
	if (!cids_en)
		return -ENOMEM;
	memset(cids_en, 0, pdata->max_cid_group * sizeof(u16));
	miup_setbit(pblk, 0, base + REG_PROT_W_EN);
	miup_setbit(pblk, 0, base + REG_PROT_R_EN);
	miup_setbit(pblk, 0, base + REG_PROT_HIT_UNMASK);
	miup_setbit(pblk, 0, base + REG_PROT_ID_GP_SEL);
	writew_relaxed(0x0, base + PROT_ID_EN_OFFSET(pblk));
	writew_relaxed(0x0, base + PROT_SADDR_L_OFFSET(pblk));
	writew_relaxed(0x0, base + PROT_SADDR_H_OFFSET(pblk));
	writew_relaxed(0x0, base + PROT_EADDR_L_OFFSET(pblk));
	writew_relaxed(0x0, base + PROT_EADDR_H_OFFSET(pblk));

	/* set cid */
	/* choose cid group */
	id_grp = readb_relaxed(base + REG_PROT_ID_GP_SEL);
	MIUP_DPRI(pdata->loglevel, dev, "id group select status[%x]", id_grp);
	for (idx = 0; idx < pdata->max_prot_blk; idx++) {
		MIUP_DPRI(pdata->loglevel, dev, "cids_en_t[%x]",
		readw_relaxed(base + PROT_ID_EN_OFFSET(idx)));
		if (((id_grp >> idx) & BIT(0)) >= (pdata->max_cid_group))
			goto RET_FUN;

		idxchk = ((id_grp >> idx) & BIT(0));
		if (idxchk < pdata->max_cid_group) {
			cids_en[idxchk] |=
			readw_relaxed(base + PROT_ID_EN_OFFSET(idx));
		}
	}
	for (idx = 0; idx < pdata->max_cid_group; idx++)
		MIUP_DPRI(pdata->loglevel, dev, "group %d id enable:%u", idx, cids_en[idx]);
	for (idx = 0; idx < pdata->max_cid_group; idx++) {
		for (idx1 = 0; idx1 < pdata->max_cid_number; idx1++) {
			if ((cids_en[idx] & BIT(idx1)) == 0) {
				writew_relaxed(0x0,
				base + PROT_ID_OFFSET(idx, idx1));
				writew_relaxed(0xFFFF,
				base + PROT_ID_MASK_OFFSET(idx, idx1));
			}
		}
	}

RET_FUN:
	kfree(cids_en);
	return 0;
}

static u8 mtk_miup_find_dup(struct device *dev,
			u64 *permit_cid,
			u64 *cid,
			u32 cids_idx,
			u8 *find_idx)
{
	u8 ret = 0;
	u32 idx;
	struct mtk_miup *pdata = dev_get_drvdata(dev);

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	for (idx = 0; idx < pdata->max_cid_number; idx++) {
		if (permit_cid[idx] == cid[cids_idx]) {
			ret = 1;
			*find_idx = idx;
			break;
		}
	}
	return ret;
}

static int mtk_miup_match_cids(struct device *dev,
			struct mtk_miup_para miup,
			u64 *permit_cid,
			u16 *cids_en)
{
	int ret = 0;
	u32 idx;
	u8 find_idx = 0;
	u64 free_id[1] = {PROT_NULL_CID};
	struct mtk_miup *pdata = dev_get_drvdata(dev);

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	MIUP_DPRI(pdata->loglevel, dev, "set cid length: %d", miup.cid_len);
	for (idx = 0; idx < miup.cid_len; idx++) {
		MIUP_DPRI(pdata->loglevel, dev, "set cid: 0x%llX", miup.cid[idx]);
		if (miup.cid[idx] == PROT_NULL_CID)
			continue;
		if (mtk_miup_find_dup(dev, permit_cid,
				miup.cid, idx, &find_idx)) {
			*cids_en = (*cids_en | BIT(find_idx));
			MIUP_DPRI(pdata->loglevel, dev, "find 0x%llX in idx%d",
				miup.cid[idx], find_idx);
			continue;
		}

		if (mtk_miup_find_dup(dev, permit_cid,
				free_id, 0, &find_idx) > 0) {
			permit_cid[find_idx] = miup.cid[idx];
			*cids_en = (*cids_en | BIT(find_idx));
			MIUP_DPRI(pdata->loglevel, dev, "find space idx%d for 0x%llX",
				find_idx, miup.cid[idx]);
			continue;
		}
		ret = -1;
		MIUP_DPRI(pdata->loglevel, dev, "cannot find space for 0x%llX", miup.cid[idx]);
		break;
	}
	return ret;
}

static int mtk_miup_init_cid(struct device *dev,
			struct mtk_miup_para miup,
			s8 *cid_grp,
			u64 *permit_cid,
			u16 *cid_en)
{
	int ret = 0;
	u8 id_grp = 0;
	u32 idx, idx1, idxchk;
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	void __iomem *base = NULL;
	u16 *cids_en = NULL;
	u64 *crt_cid = NULL;

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	cids_en = kcalloc(pdata->max_cid_group, sizeof(u16), GFP_KERNEL);
	crt_cid = kcalloc(pdata->max_cid_number * pdata->max_cid_group,
				sizeof(u64), GFP_KERNEL);
	if (!crt_cid || !cids_en) {
		ret = -ENOMEM;
		goto RET_FUN;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		ret = -ENXIO;
		goto RET_FUN;
	}
	memset(cids_en, 0, pdata->max_cid_group * sizeof(u16));
	memset(crt_cid, 0, pdata->max_cid_number * pdata->max_cid_group * sizeof(u64));
	id_grp = readb_relaxed(base + REG_PROT_ID_GP_SEL);
	MIUP_DPRI(pdata->loglevel, dev, "id group select status[%x]", id_grp);
	for (idx = 0; idx < pdata->max_prot_blk; idx++) {
		if (idx == miup.prot_blk)
			continue;

		MIUP_DPRI(pdata->loglevel, dev, "cids_en_t[%x]",
		readw_relaxed(base + PROT_ID_EN_OFFSET(idx)));
		idxchk = ((id_grp >> idx) & BIT(0));
		if (idxchk < pdata->max_cid_group) {
			cids_en[idxchk] |=
			readw_relaxed(base + PROT_ID_EN_OFFSET(idx));
		}
	}
	for (idx = 0; idx < pdata->max_cid_group; idx++) {
		MIUP_DPRI(pdata->loglevel, dev, "cids enable group %d[%u]", idx, cids_en[idx]);
		for (idx1 = 0; idx1 < pdata->max_cid_number; idx1++) {
			idxchk = (idx * pdata->max_cid_number + idx1);
			if (idxchk >= (pdata->max_cid_number * pdata->max_cid_group))
				continue;
			crt_cid[idxchk] = 0;
			if ((cids_en[idx] & BIT(idx1)) == 0)
				continue;
			crt_cid[idxchk] =
			(u64)((readw_relaxed(base + PROT_ID_OFFSET(idx, idx1)) |
			(readw_relaxed(base +
			PROT_ID_MASK_OFFSET(idx, idx1)) << 16)) & 0xFFFFFFFF);
		}
	}

	/* for debug print */
	for (idx = 0; idx < pdata->max_cid_group; idx++) {
		MIUP_DPRI(pdata->loglevel, dev, "ID group[%d]", idx);
		for (idx1 = 0; idx1 < pdata->max_cid_number; idx1++) {
			MIUP_DPRI(pdata->loglevel, dev, "ID%d[0x%llX]",
			idx1, crt_cid[idx * pdata->max_cid_number + idx1]);
		}
	}

	/* check duplicate cids and free slack */
	for (idx = 0; idx < pdata->max_cid_group; idx++) {
		if (*cid_grp >= 0)
			break;
		memcpy(permit_cid, crt_cid + (idx * pdata->max_cid_number),
			sizeof(u64) * pdata->max_cid_number);

		*cid_grp = idx;
		*cid_en = 0;

		if (mtk_miup_match_cids(dev, miup, permit_cid, cid_en) < 0) {
			*cid_grp = -1;
			*cid_en = 0;
			MIUP_DPRI(pdata->loglevel, dev, "[can not find space in cid group%d]",
				idx);
		}
	}
	if (*cid_grp < 0) {
		MIUP_DPRW(pdata->loglevel, dev, "can not find enough cid space");
		ret = -ENOMEM;
		goto RET_FUN;
	}
	MIUP_DPRI(pdata->loglevel, dev, "use group%d for cid set", *cid_grp);

RET_FUN:
	kfree(crt_cid);
	kfree(cids_en);
	return ret;
}

static int mtk_miup_set_cid(struct device *dev,
			struct mtk_miup_para miup,
			s8 cid_grp,
			u64 *permit_cid,
			u16 cid_en)
{
	int ret = 0;
	u32 idx;
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	void __iomem *base = NULL;
	u32 pblk = miup.prot_blk;

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}
	if (cid_grp >= pdata->max_cid_group) {
		MIUP_DPRW(pdata->loglevel, dev, "set id group[%d], max_cid_group[%d]",
					cid_grp, pdata->max_cid_group);
		ret = -EINVAL;
		goto RET_FUN;
	}
	miup_setbit(pblk, (cid_grp & BIT(0)), base + REG_PROT_ID_GP_SEL);

	/* set ids */
	for (idx = 0; idx < pdata->max_cid_number; idx++) {
		MIUP_DPRI(pdata->loglevel, dev, "id[0x%llX]", permit_cid[idx]);
		writew_relaxed((u16)(permit_cid[idx] & 0xFFFF),
		base + PROT_ID_OFFSET(cid_grp, idx));
		writew_relaxed((u16)((permit_cid[idx] >> 16) & 0xFFFF),
		base + PROT_ID_MASK_OFFSET(cid_grp, idx));
	}

	/* set id enable */
	MIUP_DPRI(pdata->loglevel, dev, "id_en[0x%X]", cid_en);
	writew_relaxed(cid_en, base + PROT_ID_EN_OFFSET(pblk));

RET_FUN:
	return ret;
}

static int mtk_miup_set_addr(struct device *dev,
			struct mtk_miup_para miup)
{
	int ret = 0;
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	void __iomem *base = NULL;
	u32 pblk = miup.prot_blk;
	u64 saddr_align = miup.start_addr / PROT_ADDR_ALIGN;
	u64 eaddr_align = ((miup.start_addr + miup.addr_len) / PROT_ADDR_ALIGN);

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}

	if (pdata->end_addr_ver == END_ADDR_VER_1)
		eaddr_align -= 1;

	MIUP_DPRI(pdata->loglevel, dev, "end addr version:%u",
			pdata->end_addr_ver);
	MIUP_DPRI(pdata->loglevel, dev, "saddr:0x%llx, align:0x%llx",
			miup.start_addr, saddr_align);
	MIUP_DPRI(pdata->loglevel, dev, "addr_len:0x%llx align:0x%llx",
			miup.addr_len, eaddr_align);

	writew_relaxed((saddr_align & 0xFFFF),
	base + PROT_SADDR_L_OFFSET(pblk));
	writew_relaxed(((saddr_align >> 16) & 0xFFFF),
	base + PROT_SADDR_H_OFFSET(pblk));
	writew_relaxed((eaddr_align & 0xFFFF),
	base + PROT_EADDR_L_OFFSET(pblk));
	writew_relaxed(((eaddr_align >> 16) & 0xFFFF),
	base + PROT_EADDR_H_OFFSET(pblk));

	return ret;
}

static int mtk_miup_set_prot_en(struct device *dev,
			struct mtk_miup_para miup)
{
	int ret = 0;
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	void __iomem *base = NULL;
	u32 pblk = miup.prot_blk;

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}

	switch (miup.attr) {
	case MTK_MIUP_RPROT:
		miup_setbit(pblk, 1, base + REG_PROT_R_EN);
		miup_setbit(pblk, 0, base + REG_PROT_W_EN);
		break;
	case MTK_MIUP_WPROT:
		miup_setbit(pblk, 0, base + REG_PROT_R_EN);
		miup_setbit(pblk, 1, base + REG_PROT_W_EN);
		break;
	case MTK_MIUP_RWPROT:
		miup_setbit(pblk, 1, base + REG_PROT_R_EN);
		miup_setbit(pblk, 1, base + REG_PROT_W_EN);
		break;
	default:
		MIUP_DPRI(pdata->loglevel, dev, "set no protect");
		return -EINVAL;
	}
	miup_setbit(pblk, 0, base + REG_PROT_HIT_UNMASK);

	return ret;
}

static int mtk_miup_get_prot_attr(struct mtk_miup *pdata,
				u32 prot_type,
				u32 prot_blk,
				struct mtk_miup_para *miup)
{
	int ret = 0;
	bool w_en = false;
	bool r_en = false;
	void __iomem *base = NULL;
	int bmask = (prot_type == 1) ? BIT(12) : BIT(prot_blk);

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}
	w_en = (readw_relaxed(base + REG_PROT_W_EN) & bmask) ? true : false;
	r_en = (readw_relaxed(base + REG_PROT_R_EN) & bmask) ? true : false;

	if (w_en & r_en)
		miup->attr = MTK_MIUP_RWPROT;
	else if (w_en)
		miup->attr = MTK_MIUP_WPROT;
	else if (r_en)
		miup->attr = MTK_MIUP_RPROT;
	else
		ret = -1;

	return ret;
}

static int mtk_miup_get_prot_addr(struct mtk_miup *pdata,
				u32 prot_type,
				u32 prot_blk,
				struct mtk_miup_para *miup)
{
	int ret = 0;
	void __iomem *base = NULL;
	u32 saddr_reg = 0;
	u32 eaddr_reg = 0;
	u64 start_addr = 0x0;
	u64 end_addr = 0x0;

	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}
	if (prot_type == 1) {
		saddr_reg = REG_PROT_OUT_OF_AREA_START_L;
		eaddr_reg = REG_PROT_OUT_OF_AREA_END_L;
	} else {
		saddr_reg = PROT_SADDR_L_OFFSET(prot_blk);
		eaddr_reg = PROT_EADDR_L_OFFSET(prot_blk);
	}

	start_addr = (u64)(readw_relaxed(base + saddr_reg + 0x4) << 16);
	start_addr += (u64)(readw_relaxed(base + saddr_reg));
	start_addr *= PROT_ADDR_ALIGN;

	end_addr = (u64)(readw_relaxed(base + eaddr_reg + 0x4) << 16);
	end_addr += (u64)(readw_relaxed(base + eaddr_reg));
	if (pdata->end_addr_ver == END_ADDR_VER_1)
		end_addr += 1;
	end_addr *= PROT_ADDR_ALIGN;

	miup->start_addr = start_addr;
	miup->addr_len = end_addr - start_addr;

	return ret;
}

static int mtk_miup_get_permit_id(struct mtk_miup *pdata,
				u32 prot_type,
				u32 prot_blk,
				struct mtk_miup_para *miup)
{
	int ret = 0;
	u32 idx, idx2;
	u16 cids_en = 0;
	u32 cid_gp = 0;
	s32 *cids_idx = NULL;

	miup->cid_len = 0;
	if (prot_type == 1)
		goto RET_FUN;

	if (!pdata || !pdata->base) {
		MIUP_PRE("get device base failed");
		goto RET_FUN;
	}
	cids_idx = kcalloc(pdata->max_cid_number, sizeof(s32), GFP_KERNEL);
	if (!cids_idx) {
		ret = -ENOMEM;
		goto RET_FUN;
	}

	for (idx = 0; idx < pdata->max_cid_number; idx++)
		cids_idx[idx] = -1;

	if (readb_relaxed(pdata->base + REG_PROT_ID_GP_SEL) & BIT(prot_blk))
		cid_gp = 1;

	cids_en = readw_relaxed(pdata->base + PROT_ID_EN_OFFSET(prot_blk));
	if (cids_en == 0x0) {
		goto RET_FUN;
	}

	for (idx = 0; idx < pdata->max_cid_number; idx++) {
		if (cids_en & BIT(idx)) {
			cids_idx[miup->cid_len] = idx;
			miup->cid_len++;
		}
	}
	if (miup->cid_len == 0) {
		goto RET_FUN;
	}

	if (!miup->cid) {
		miup->cid_len = 0;
		ret = -ENOMEM;
		goto RET_FUN;
	}

	for (idx = 0, idx2 = 0; idx < miup->cid_len; idx++) {
		if (cids_idx[idx] == -1)
			break;
		miup->cid[idx2] =
		(u64)((readw_relaxed(pdata->base +
		PROT_ID_OFFSET(cid_gp, cids_idx[idx])) |
		(readw_relaxed(pdata->base +
		PROT_ID_MASK_OFFSET(cid_gp, cids_idx[idx])) << 16))
		& 0xFFFFFFFF);

		if (miup->cid[idx2] == 0)
			continue;
		else
			idx2++;
	}
	miup->cid_len = idx2;

RET_FUN:
	kfree(cids_idx);
	return ret;
}

int mtk_miup_get_all_protect(struct device *dev)
{
	int idx;
	void __iomem *base = NULL;
	struct mtk_miup *pdata = NULL;

	pdata = dev_get_drvdata(dev);
	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}

	for (idx = 0; idx < 128; idx++)
		pdata->miup_all[idx] = readw_relaxed(base + (idx << 2));

	return 0;
}
EXPORT_SYMBOL(mtk_miup_get_all_protect);

int mtk_miup_set_all_protect(struct device *dev)
{
	int idx;
	void __iomem *base = NULL;
	struct mtk_miup *pdata = NULL;

	pdata = dev_get_drvdata(dev);
	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}

	for (idx = 2; idx < 128; idx++)
		writew_relaxed(pdata->miup_all[idx], base + (idx << 2));

	/* enable miu protect after all miu protect setting */
	wmb();
	for (idx = 0; idx < 2; idx++)
		writew_relaxed(pdata->miup_all[idx], base + (idx << 2));

	return 0;
}
EXPORT_SYMBOL(mtk_miup_set_all_protect);

/*********************************************************************
 *                         Public Function                           *
 *********************************************************************/
/**
 * mtk_miup_set_protect - set miu protect.
 * @struct mtk_miup_para: Structure of set miu protect.
 */
int mtk_miup_set_protect(struct mtk_miup_para miup)
{
	int ret = 0;
	s8 cid_grp = -1;
	u16 cid_en = 0;
	u64 *permit_cid = NULL;
	void __iomem *base = NULL;
	struct device *dev = NULL;
	struct mtk_miup *pdata = NULL;

	dev = mtk_miup_get_miup_device("mediatek,mtk-miup");
	if (!dev) {
		MIUP_PRE("no device");
		return -ENODEV;
	}
	pdata = dev_get_drvdata(dev);
	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}
	permit_cid = kcalloc(pdata->max_cid_number, sizeof(u64), GFP_KERNEL);
	if (!permit_cid) {
		ret = -ENOMEM;
		goto RET_FUN;
	}

	/* check input value */
	ret = mtk_miup_check_para(dev, &miup);
	if (ret < 0)
		goto RET_FUN;

	/* clean protection only */
	if (miup.attr == MTK_MIUP_CLEANPROT) {
		mtk_miup_clean_prot(dev, miup);
		goto RET_FUN;
	}
	/* set ids */
	/* check id free slack and initial ids */
	ret = mtk_miup_init_cid(dev, miup, &cid_grp, permit_cid, &cid_en);
	if (ret < 0)
		goto RET_FUN;
	mtk_miup_clean_prot(dev, miup);
	/* wait clean protect first */
	/* set miu protect after original protect clean done */
	wmb();

	ret = mtk_miup_set_cid(dev, miup, cid_grp, permit_cid, cid_en);
	if (ret < 0)
		goto RET_FUN;

	/* set address */
	ret = mtk_miup_set_addr(dev, miup);
	if (ret < 0)
		goto RET_FUN;
	/* set protect enable */
	ret = mtk_miup_set_prot_en(dev, miup);
	if (ret < 0)
		goto RET_FUN;
	/* clean hit log */
	miup_setbit(0, 1, base + REG_PROT_HIT_FLAG);
	miup_setbit(0, 0, base + REG_PROT_HIT_FLAG);

RET_FUN:
	kfree(permit_cid);
	return ret;
}
EXPORT_SYMBOL(mtk_miup_set_protect);

/**
 * mtk_miup_get_protect - get miu protect.
 * @u32 prot_type: 0: normal protect, 1: out of memory protect.
 * @u32 prot_blk: Request which protect block info.
 * @struct mtk_miup_para: Structure of get miu protect.
 */
int mtk_miup_get_protect(u32 prot_type,
			u32 prot_blk,
			struct mtk_miup_para *miup)
{
	s32 ret = 0;
	struct device *dev = NULL;
	struct mtk_miup *pdata = NULL;
	void __iomem *base = NULL;

	dev = mtk_miup_get_miup_device("mediatek,mtk-miup");
	if (!dev) {
		MIUP_PRE("no device");
		ret = -ENODEV;
		goto RET_FUN;
	}
	pdata = dev_get_drvdata(dev);
	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return -ENXIO;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return -ENXIO;
	}
	if (prot_blk >= pdata->max_prot_blk) {
		MIUP_DPRW(pdata->loglevel, dev, "invalid block");
		ret = -EINVAL;
		goto RET_FUN;
	} else {
		miup->prot_blk = prot_blk;
	}

	/*get protect attribute*/
	if (mtk_miup_get_prot_attr(pdata, prot_type, prot_blk, miup) < 0) {
		MIUP_DPRI(pdata->loglevel, dev, "No protect");
		ret = -EINVAL;
		goto RET_FUN;
	}
	/*get protect address*/
	mtk_miup_get_prot_addr(pdata, prot_type, prot_blk, miup);
	/*get protect enable access client IDs*/
	mtk_miup_get_permit_id(pdata, prot_type, prot_blk, miup);

RET_FUN:
	return ret;
}
EXPORT_SYMBOL(mtk_miup_get_protect);

/**
 * mtk_miup_get_hit_status - get miu protect hit log.
 * @dev: device.
 * @struct mtk_miup_hitlog: Structure of get miu protect hit log.
 */
int mtk_miup_get_hit_status(struct mtk_miup_hitlog *hit_log)
{
	int ret = 1;
	struct device *dev = NULL;
	struct mtk_miup *pdata = NULL;
	void __iomem *base = NULL;
	void __iomem *hbase = NULL;

	dev = mtk_miup_get_miup_device("mediatek,mtk-miup");
	if (!dev) {
		MIUP_PRE("no device");
		ret = 0;
		goto RET_FUN;
	}
	pdata = dev_get_drvdata(dev);
	if (!pdata) {
		ret = 0;
		goto RET_FUN;
	}

	base = pdata->base;
	if (!base || !pdata->hit_log0_base || !pdata->hit_log1_base ||
	!pdata->hit_log2_base || !hit_log) {
		ret = 0;
		goto RET_FUN;
	}

	hit_log->attr = 0;
	if ((readw_relaxed(pdata->hit_log0_base + REG_PROT_W_HIT_LOG) &
		BIT(15))) {
		hbase = pdata->hit_log0_base;
		hit_log->attr = MTK_MIUP_WPROT;
	} else if ((readw_relaxed(pdata->hit_log0_base + REG_PROT_R_HIT_LOG) &
		BIT(15))) {
		hbase = pdata->hit_log0_base;
		hit_log->attr = MTK_MIUP_RPROT;
	} else if ((readw_relaxed(pdata->hit_log1_base + REG_PROT_W_HIT_LOG) &
		BIT(15))) {
		hbase = pdata->hit_log1_base;
		hit_log->attr = MTK_MIUP_WPROT;
	} else if ((readw_relaxed(pdata->hit_log1_base + REG_PROT_R_HIT_LOG) &
		BIT(15))) {
		hbase = pdata->hit_log1_base;
		hit_log->attr = MTK_MIUP_RPROT;
	} else if ((readw_relaxed(pdata->hit_log2_base + REG_PROT_W_HIT_LOG) &
		BIT(15))) {
		hbase = pdata->hit_log2_base;
		hit_log->attr = MTK_MIUP_WPROT;
	} else if ((readw_relaxed(pdata->hit_log2_base + REG_PROT_R_HIT_LOG) &
		BIT(15))) {
		hbase = pdata->hit_log2_base;
		hit_log->attr = MTK_MIUP_RPROT;
	} else {
		ret = 0;
		MIUP_DPRI(pdata->loglevel, dev, "no miu protect hit");
		goto RET_FUN;
	}

	hit_log->cid =
	readw_relaxed(hbase + ((hit_log->attr == MTK_MIUP_WPROT) ?
	REG_PROT_W_HIT_ID : REG_PROT_R_HIT_ID));
	if (hit_log->attr == MTK_MIUP_WPROT) {
		hit_log->hit_addr =
		(((u64)readw_relaxed(hbase + REG_PROT_W_HIT_ADDR_L) |
		((u64)readw_relaxed(hbase + REG_PROT_W_HIT_ADDR_H) << 16)) << 5);
	} else {
		hit_log->hit_addr =
		(((u64)readw_relaxed(hbase + REG_PROT_R_HIT_ADDR_L) |
		((u64)readw_relaxed(hbase + REG_PROT_R_HIT_ADDR_H) << 16)) << 5);
	}
	hit_log->hit_blk =
	(readb_relaxed(hbase + (hit_log->attr == MTK_MIUP_WPROT ?
	REG_PROT_W_HIT_LOG : REG_PROT_R_HIT_LOG)) & 0x1F);

	/* clean hit log */
	miup_setbit(0, 1, base + REG_PROT_HIT_FLAG);
	miup_setbit(0, 0, base + REG_PROT_HIT_FLAG);

RET_FUN:
	return ret;
}
EXPORT_SYMBOL(mtk_miup_get_hit_status);

static void gals_slpprot_off(void __iomem *base, int i, u8 *slpprot)
{
	u32 value, bit;

	/* un-protect sequence : slave -> master */
	value = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_CTRL_OFFSET];
	/* SLAVE */
	bit = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_SLV_EN];
	miup_setbit(bit, 0, base + value);
	/* MASTER */
	bit = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_MST_EN];
	miup_setbit(bit, 0, base + value);

}

static void gals_slpprot_on(void __iomem *base, int i, u8 *slpprot)
{
	u16 rdy = 0;
	u32 time_out = 0;
	u32 value, bit;

	/* protect sequence : master -> slave */

	/* MASTER */
	value = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_CTRL_OFFSET];
	bit = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_MST_EN];
	miup_setbit(bit, 1, base + value);

	/* check ready */
	value = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_RDY_OFFSET];
	bit = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_MST_RDY];
	while (rdy == 0 && ++time_out < GALS_RDY_TIMEOUT) {
		if (time_out ==  (GALS_RDY_TIMEOUT - 1))
			pr_emerg("gals[%d] rdy mst timeout\n", i);
		rdy = ((readw_relaxed(base + value) >> bit) & 0x1);
	}

	/* SLAVE */
	time_out = 0;
	rdy = 0;
	value = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_CTRL_OFFSET];
	bit = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_SLV_EN];
	miup_setbit(bit, 1, base + value);

	/* check ready */
	value = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_RDY_OFFSET];
	bit = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_SLV_RDY];
	while (rdy == 0 && ++time_out < GALS_RDY_TIMEOUT) {
		if (time_out ==  (GALS_RDY_TIMEOUT - 1))
			pr_emerg("gals[%d] rdy slv timeout\n", i);
		rdy = ((readw_relaxed(base + value) >> bit) & 0x1);
	}

}

static void mtk_gals_slpprot(struct device *dev, unsigned int flag)
{
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	void __iomem *base;
	u8 *slpprot;
	u32 gals_cnt = 0;
	u32 i, value, bit;

	base = ioremap(SMI_CTRL_BANK, SMI_CTRL_REG_SIZE);
	if (!base)
		return;

	switch (pdata->chipid) {
	case CHIP_MT5896:
		gals_cnt = SLPPROT_MT5896_GALS;
		slpprot = *u8mt5896_slpprot;
		break;
	case CHIP_MT5897:
		gals_cnt = SLPPROT_MT5897_GALS;
		slpprot = *u8mt5897_slpprot;
		break;
	case CHIP_MT5876:
		gals_cnt = SLPPROT_MT5876_GALS;
		slpprot = *u8mt5876_slpprot;
		break;
	case CHIP_MT5879:
		gals_cnt = SLPPROT_MT5879_GALS;
		slpprot = *u8mt5879_slpprot;
		break;
	default:
		goto RET_FUN;
	}

	if (!flag) {
		for (i = gals_cnt; i > 0; i--) {
			value = slpprot[(i - 1) * SLPPROT_MAX_CONTENT + GALS_SLPPROT_ENABLE];
			if (value == SLPPROT_DISABLE)
				continue;
			gals_slpprot_off(base, i-1, slpprot);
		}
		goto RET_FUN;
	}

	for (i = 0; i < gals_cnt; i++) {
		value = slpprot[i * SLPPROT_MAX_CONTENT + GALS_SLPPROT_ENABLE];
		if (value == SLPPROT_DISABLE)
			continue;
		gals_slpprot_on(base, i, slpprot);
	}

RET_FUN:
	iounmap(base);
}

static void mtk_miup_suspend_protect(struct device *dev)
{
	void __iomem *base = NULL;
	struct mtk_miup *pdata = NULL;
	struct mtk_miup_hitlog hit_log;
	struct mtk_miup_para suspend_miup;
	u64 dram_len;

	pdata = dev_get_drvdata(dev);
	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return;
	}

	pr_warn("miup suspend\n");

	suspend_miup.cid_len = pdata->cpu_cid_num;
	suspend_miup.cid = pdata->cpu_cid;
	suspend_miup.attr = MTK_MIUP_WPROT;
	suspend_miup.start_addr = 0x0;
	suspend_miup.prot_blk = PROT_BLK7;
	dram_len = ((u64)readw_relaxed((base + REG_PROT_OUT_OF_AREA_END_L)) |
		   ((u64)readw_relaxed((base + REG_PROT_OUT_OF_AREA_END_H)) << SHIFT_16BIT));
	if (pdata->end_addr_ver == END_ADDR_VER_1)
		suspend_miup.addr_len = ((dram_len + 1) * PROT_ADDR_ALIGN);
	else
		suspend_miup.addr_len = (dram_len * PROT_ADDR_ALIGN);
	mtk_miup_set_protect(suspend_miup);

	if (mtk_miup_get_hit_status(&hit_log)) {
		pr_emerg(
		"[MIUP][HIT][CID:0x%llX %s address: 0x%llX - 0x%llX block:%d\n",
		hit_log.cid, (hit_log.attr == 0) ? "read" : "write",
		hit_log.hit_addr, (hit_log.hit_addr + 0x1F), hit_log.hit_blk);
		WARN_ON(true);
	}
}

static void mtk_miup_resume_protect(struct device *dev)
{
	void __iomem *base = NULL;
	struct mtk_miup *pdata = NULL;
	struct mtk_miup_hitlog hit_log;
	struct mtk_miup_para resume_miup;

	pdata = dev_get_drvdata(dev);
	if (!pdata) {
		MIUP_PRE("no driver data\n");
		return;
	}
	base = pdata->base;
	if (!base) {
		MIUP_PRE("no device base\n");
		return;
	}

	pr_warn("miup resume\n");
	resume_miup.attr = MTK_MIUP_CLEANPROT;
	resume_miup.prot_blk = PROT_BLK7;
	resume_miup.start_addr = 0;
	resume_miup.addr_len = 0;
	resume_miup.cid_len = 0;
	resume_miup.cid = NULL;
	mtk_miup_set_protect(resume_miup);

	if (mtk_miup_get_hit_status(&hit_log)) {
		pr_emerg(
		"[MIUP][HIT][CID:0x%llX %s address: 0x%llX - 0x%llX block:%d\n",
		hit_log.cid, (hit_log.attr == 0) ? "read" : "write",
		hit_log.hit_addr, (hit_log.hit_addr + 0x1F), hit_log.hit_blk);
		WARN_ON(true);
	}
}

static void mtk_miup_get_black_client(struct device *dev,
				      struct mtk_miup *pdata)
{
	s32 err = 0;
	u32 idx = 0;
	u32 *get_cid;

	err = of_property_read_u32(dev->of_node,
			"overbound_black_client_cnt", &ob_black_client_cnt);
	if (err || !ob_black_client_cnt) {
		kfree(ob_black_client_list);
		return;
	}
	ob_black_client_list = kcalloc(ob_black_client_cnt, sizeof(u64), GFP_KERNEL);
	if (!ob_black_client_list) {
		ob_black_client_cnt = 0;
		return;
	}
	get_cid = kcalloc((ob_black_client_cnt * CLIENT_ID_USE_ARRAY_CNT),
			  sizeof(u32), GFP_KERNEL);
	if (!get_cid) {
		ob_black_client_cnt = 0;
		return;
	}
	err = of_property_read_u32_array(dev->of_node, "overbound_black_client",
			get_cid, (ob_black_client_cnt * CLIENT_ID_USE_ARRAY_CNT));
	if (err)
		goto RET_FUNC;

	for (idx = 0; idx < ob_black_client_cnt; idx++)
		ob_black_client_list[idx] =
		((u64)get_cid[(idx * CLIENT_ID_USE_ARRAY_CNT)] |
		((u64)get_cid[((idx * CLIENT_ID_USE_ARRAY_CNT) + 1)] << SHIFT_32BIT));

RET_FUNC:
	kfree(get_cid);
}

static void mtk_miup_get_client(struct device *dev,
				struct mtk_miup *pdata)
{
	s32 err = 0;
	u32 idx = 0;
	u32 *get_cid;

	err = of_property_read_u32(dev->of_node,
			"mediatek,client_cnt", &(pdata->kcid_num));
	if (err || !pdata->kcid_num) {
		MIUP_DPRW(pdata->loglevel, dev, "no kernel protect whitelist");
		pdata->kcid_num = 0;
		pdata->kcid = NULL;
	}

	if (pdata->kcid_num > 0) {
		pdata->kcid = kcalloc(pdata->kcid_num, sizeof(u64), GFP_KERNEL);
		get_cid = kcalloc((pdata->kcid_num * 2), sizeof(u32), GFP_KERNEL);
		err = of_property_read_u32_array(dev->of_node,
			"mediatek,client", get_cid, (pdata->kcid_num * 2));

		if (err) {
			MIUP_DPRW(pdata->loglevel, dev, "missing mediatek,client property");
			pdata->kcid_num = 0;
			kfree(pdata->kcid);
		}

		for (idx = 0; idx < pdata->kcid_num; idx++) {
			pdata->kcid[idx] = ((u64)get_cid[(idx * 2)] |
			((u64)get_cid[((idx * 2) + 1)] << 32));
		}
		kfree(get_cid);
	}

	err = of_property_read_u32(dev->of_node, "mediatek,cpu_client_cnt", &(pdata->cpu_cid_num));
	if (err || !pdata->cpu_cid_num) {
		MIUP_DPRE(pdata->loglevel, dev, "no cpu whitelist");
		pdata->cpu_cid_num = 0;
		pdata->cpu_cid = NULL;
	}

	if (pdata->cpu_cid_num > 0) {
		pdata->cpu_cid = kcalloc(pdata->cpu_cid_num, sizeof(u64), GFP_KERNEL);
		get_cid = kcalloc((pdata->cpu_cid_num * 2), sizeof(u32), GFP_KERNEL);
		err = of_property_read_u32_array(dev->of_node,
			"mediatek,cpu_client", get_cid, (pdata->cpu_cid_num * 2));

		if (err) {
			MIUP_DPRW(pdata->loglevel, dev, "missing mediatek,cpu_client property");
			pdata->cpu_cid_num = 0;
			kfree(pdata->cpu_cid);
		}

		for (idx = 0; idx < pdata->cpu_cid_num; idx++) {
			pdata->cpu_cid[idx] = ((u64)get_cid[(idx * 2)] |
			((u64)get_cid[((idx * 2) + 1)] << 32));
		}
		kfree(get_cid);
	}
}

static irqreturn_t mtk_miup_irq(int irq, void *id)
{
	struct mtk_miup *pdata = (struct mtk_miup *)id;
	struct mtk_miup_hitlog hit_log;
	u32 i;
	u16 cid, cid_en;
	u64 nullmem_staddr;

	if (!mtk_miup_get_hit_status(&hit_log))
		return IRQ_NONE;

	printk_ratelimited(KERN_EMERG
	"[MIUP][HIT][CID:0x%llX %s address: 0x%llX - 0x%llX block:%d\n",
	hit_log.cid, (hit_log.attr == 0) ? "read" : "write",
	hit_log.hit_addr, (hit_log.hit_addr + 0x1F), hit_log.hit_blk);

	for (i = 0; i < null_mem_cnt; i++) {
		// truncate 33bit
		nullmem_staddr = (null_mem[i].start_addr & TRUNCATE_33BIT);
		if (hit_log.hit_blk >= pdata->max_prot_blk &&
		   (hit_log.hit_addr >= nullmem_staddr &&
		    hit_log.hit_addr < (nullmem_staddr + null_mem[i].len))) {
			pr_warn_ratelimited("hit null mem\n");
			return IRQ_NONE;
		}
	}

	if (ob_black_client_cnt &&
	    hit_log.attr == 0 &&
	    hit_log.hit_blk > pdata->max_prot_blk) {
		for (i = 0; i < ob_black_client_cnt; i++) {
			cid = ob_black_client_list[i] & LOW_16BIT;
			cid_en = ((ob_black_client_list[i] >> SHIFT_16BIT) & LOW_16BIT);
			if ((hit_log.cid & cid_en) == (cid & cid_en)) {
				printk_ratelimited(KERN_EMERG
				"ID : 0x%llX SW patch by pass panic\n", hit_log.cid);
				return IRQ_HANDLED;
			}
		}
	}

	if (pdata->hit_panic)
		panic("MIUP hit panic\n");

	return IRQ_HANDLED;
}

static void mtk_miup_init_irq(struct platform_device *pdev,
			struct device *dev,
			struct mtk_miup *pdata)
{
	s32 err = 0;
	struct irq_desc *desc = NULL;

	pdata->irq = platform_get_irq(pdev, 0);
	if (pdata->irq < 0) {
		MIUP_DPRW(pdata->loglevel, dev, "no IRQ resource");
	} else {
		err = devm_request_irq(dev, (unsigned int)pdata->irq,
			mtk_miup_irq, IRQF_TRIGGER_HIGH | IRQF_SHARED | IRQF_ONESHOT,
			dev_name(dev), pdata);
		if (err) {
			MIUP_DPRW(pdata->loglevel, dev, "can not request IRQ");
			pdata->irq = -1;
		} else {
			if (!en_hit_irq) {
				desc = irq_to_desc((unsigned int)pdata->irq);
				if (desc && !irqd_irq_disabled(&desc->irq_data))
					disable_irq((unsigned int)pdata->irq);
			}
		}
	}
//	en_hit_panic = false;
	pdata->hit_panic = en_hit_panic;
}

static int mtk_miup_get_cpu_base(u32 *cpu_base)
{
	struct device_node *np = NULL;

	np = of_find_node_by_name(np, "memory_info");
	if (np)
		return of_property_read_u32(np, "cpu_emi0_base", cpu_base);

	return -1;
}

static void mtk_miup_init_nullmem(struct device *dev, struct mtk_miup *pdata)
{
	int i = 0;
	struct device_node *np = NULL;
	u32 tmp[MAX_REG_SIZE];
	u32 cpu_base;

	null_mem_cnt = 0;
	if (mtk_miup_get_cpu_base(&cpu_base) < 0)
		return;

	for (null_mem_cnt = 0; null_mem_cnt < NULL_MEM_MAX_CNT; null_mem_cnt++) {
		np = of_find_node_by_type(np, "mtk-nullmem");
		if (np == NULL)
			break;
	}
	if (null_mem_cnt == 0) {
		MIUP_DPRI(pdata->loglevel, dev, "no nullmem");
		return;
	}
	null_mem = kcalloc(null_mem_cnt, sizeof(struct mtk_nullmem), GFP_KERNEL);
	if (!null_mem)
		return;

	np = NULL;
	for (i = 0; i < null_mem_cnt; i++) {
		np = of_find_node_by_type(np, "mtk-nullmem");
		if (np == NULL)
			break;
		null_mem[i].start_addr = 0;
		null_mem[i].len = 0;
		if (of_property_read_u32_array(np, "reg", tmp, MAX_REG_SIZE))
			continue;

		null_mem[i].start_addr = ((((u64)tmp[START_ADDR_H] << SHIFT_32BIT) |
					    (u64)tmp[START_ADDR_L]) - cpu_base);
		null_mem[i].len = (((u64)tmp[LEN_ADDR_H] << SHIFT_32BIT) |
				    (u64)tmp[LEN_ADDR_L]);
	}

	for (i = 0; i < null_mem_cnt; i++)
		pr_emerg("[%d][%llx][%llx]\n", i, null_mem[i].start_addr, null_mem[i].len);
}

static int mtk_miup_probe(struct platform_device *pdev)
{
	int ret = 0;
	s32 err = 0;
	struct resource *res0 = NULL;
	struct resource *res1 = NULL;
	struct resource *res2 = NULL;
	struct resource *res3 = NULL;
	struct mtk_miup *pdata = NULL;
	struct device *dev = &pdev->dev;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ret = -ENOMEM;
		goto RET_FUN;
	}

	pdata->loglevel = MIUP_LOG_DEFAULT;
	pdata->dev = dev;
	res0 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	res2 = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	res3 = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!res0 || !res1 || !res2 || !res3) {
		ret = -EBUSY;
		goto RET_FUN;
	}
	pdata->base = devm_ioremap_resource(dev, res0);
	if (IS_ERR(pdata->base)) {
		ret = PTR_ERR(pdata->base);
		goto RET_FUN;
	}
	pdata->hit_log0_base = devm_ioremap_resource(dev, res1);
	if (IS_ERR(pdata->hit_log0_base)) {
		ret = PTR_ERR(pdata->hit_log0_base);
		goto RET_FUN;
	}
	pdata->hit_log1_base = devm_ioremap_resource(dev, res2);
	if (IS_ERR(pdata->hit_log1_base)) {
		ret = PTR_ERR(pdata->hit_log1_base);
		goto RET_FUN;
	}
	pdata->hit_log2_base = devm_ioremap_resource(dev, res3);
	if (IS_ERR(pdata->hit_log2_base)) {
		ret = PTR_ERR(pdata->hit_log2_base);
		goto RET_FUN;
	}

	err = of_property_read_u32(dev->of_node,
			"mediatek,max_prot_blk", &(pdata->max_prot_blk));
	if (err) {
		MIUP_DPRE(pdata->loglevel, dev, "missing mediatek,max_prot_blk property");
		ret = err;
		goto RET_FUN;
	}

	err = of_property_read_u32(dev->of_node,
			"mediatek,max_cid_group", &(pdata->max_cid_group));
	if (err) {
		MIUP_DPRE(pdata->loglevel, dev, "missing mediatek,max_cid_group property");
		ret = err;
		goto RET_FUN;
	}

	err = of_property_read_u32(dev->of_node,
			"mediatek,max_cid_number", &(pdata->max_cid_number));
	if (err) {
		MIUP_DPRE(pdata->loglevel, dev, "missing mediatek,max_cid_number property");
		ret = err;
		goto RET_FUN;
	}
	err = of_property_read_u32(dev->of_node,
			"mediatek,end_addr_ver", &(pdata->end_addr_ver));
	if (err) {
		MIUP_DPRE(pdata->loglevel, dev, "missing mediatek,end_addr_ver property");
		ret = err;
		goto RET_FUN;
	}

	err = of_property_read_u32(dev->of_node,
			"mediatek,chipid", &(pdata->chipid));
	if (err) {
		MIUP_DPRE(pdata->loglevel, dev, "missing mediatek,chipid");
		ret = err;
		goto RET_FUN;
	}

	mtk_miup_get_client(dev, pdata);
	mtk_miup_get_black_client(dev, pdata);
	pdata->irq = -1;
	platform_set_drvdata(pdev, pdata);
	mtk_miup_init_nullmem(dev, pdata);
	mtk_miup_init_irq(pdev, dev, pdata);

RET_FUN:
	if (!pdata)
		devm_kfree(dev, (void *)pdata);

	return ret;
}

static struct resource *mtk_miup_next_resource(struct resource *p, bool sibling_only)
{
	/* Caller wants to traverse through siblings only */
	if (sibling_only)
		return p->sibling;

	if (p->child)
		return p->child;
	while (!p->sibling && p->parent)
		p = p->parent;
	return p->sibling;
}

static int mtk_miup_select_check_id(struct device *dev, struct mtk_miup *pdata)
{
	if (!pdata || !pdata->base)
		return -ENXIO;

	miup_setbit(CHECK_ID_OFFSET, CHECK_AXID, pdata->base + REG_PROT_CHECK_ID_SELECT);
	return 0;
}

static int mtk_miup_do_kernel_protect(struct device *dev, struct mtk_miup *pdata)
{
	bool siblings_only = true;
	struct mtk_miup_para kcode_miup;
	struct resource *p;
	struct resource *cp;

	kcode_miup.cid_len = pdata->cpu_cid_num;
	kcode_miup.cid = pdata->cpu_cid;
	kcode_miup.attr = MTK_MIUP_WPROT;

	for (p = iomem_resource.child; p; p = mtk_miup_next_resource(p, siblings_only)) {
		if (p && !strcmp(p->name, "System RAM")) {
			for (cp = p->child; cp; cp = mtk_miup_next_resource(cp, siblings_only)) {
				if (cp && !strcmp(cp->name, "Kernel code")) {
					kcode_miup.start_addr =
					(u64)(align_st(cp->start) - PROT_BA_BASE);
					kcode_miup.addr_len =
					(u64)(align_ed(cp->end) - align_st(cp->start) + 1);
					kcode_miup.prot_blk = 7;
					mtk_miup_set_protect(kcode_miup);
					break;
				}
			}
		}
	}

	return 0;
}

static int mtk_miup_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_miup *pdata = NULL;
	int idx;

	pdata = dev_get_drvdata(dev);
	if (pdata->irq >= 0)
		devm_free_irq(dev, (unsigned int)pdata->irq, pdata);

	for (idx = 0; idx < pdata->kprot_num; idx++)
		kfree(pdata->kprot[idx].cid);

	kfree(pdata->kprot);
	kfree(pdata->kcid);
	devm_kfree(dev, (void *)pdata);

	MIUP_DPRI(pdata->loglevel, dev, "exit miu protect succeeded");

	return 0;
}

static int mtk_miup_suspend(struct device *dev)
{
	mtk_gals_slpprot(dev, 1);
	mtk_miup_get_all_protect(dev);
	mtk_miup_suspend_protect(dev);
	return 0;
}

static int mtk_miup_resume(struct device *dev)
{
	mtk_miup_resume_protect(dev);
	mtk_miup_set_all_protect(dev);
	mtk_gals_slpprot(dev, 0);

	return 0;
}

static void mtk_miup_shutdown(struct platform_device *pdev)
{
	mtk_gals_slpprot(&pdev->dev, 1);
}

static const struct dev_pm_ops mtk_miup_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(mtk_miup_suspend, mtk_miup_resume)
};

static const struct of_device_id mtk_miup_of_match[] = {
	{ .compatible = "mediatek,mtk-miup",},
	{/* sentinel */},
};

static struct platform_driver mtk_miup_platdrv = {
	.probe  = mtk_miup_probe,
	.remove = mtk_miup_remove,
	.shutdown = mtk_miup_shutdown,
	.driver = {
		.name	= "mtk-miup",
		.owner	= THIS_MODULE,
		.pm	= &mtk_miup_pm_ops,
		.of_match_table = mtk_miup_of_match,
	},
};

static int __init mtk_miup_init(void)
{
	int ret = 0;
	struct device *dev;
	struct mtk_miup *pdata = NULL;

	if (atomic_read(&init_done)) {
		MIUP_PRW("already init driver");
		return ret;
	}

	ret = platform_driver_register(&mtk_miup_platdrv);
	if (ret != 0) {
		MIUP_PRE("register miu protect driver failed");
		MIUP_PRI("init miu protect failed");
		return ret;
	}

	p_dev_node = NULL;
	p_pdev = NULL;
	dev = mtk_miup_get_miup_device("mediatek,mtk-miup");
	if (!dev) {
		MIUP_PRE("no device");
		return ret;
	}
	pdata = dev_get_drvdata(dev);

//set kernel memory
	ret = mtk_miup_select_check_id(dev, pdata);
	if (ret < 0) {
		MIUP_PRI("init miu protect select check id failed");
		return ret;
	}
	if (mtk_miup_do_kernel_protect(dev, pdata) < 0)
		MIUP_PRI("no kernel protect");

	atomic_set(&init_done, true);
	MIUP_PRI("init miu protect succeeded");
	return ret;
}

static void __exit mtk_miup_exit(void)
{
	platform_driver_unregister(&mtk_miup_platdrv);
}

module_init(mtk_miup_init);
module_exit(mtk_miup_exit);

module_param_named(irq, en_hit_irq, bool, 0660);
module_param_named(panic, en_hit_panic, bool, 0660);

MODULE_DESCRIPTION("MediaTek MIUP driver");
MODULE_AUTHOR("Max-CH.Tsai <Max-CH.Tsai@mediatek.com>");
MODULE_LICENSE("GPL");
