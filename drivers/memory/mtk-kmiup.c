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
#include <asm/barrier.h>
#include <soc/mediatek/mtktv-miup.h>

#include "mtk-miup.h"

#define kprot_num (8)
static u8 kprot_stat;
static u8 prot_stat;
static struct device_node *p_dev_node;
static struct platform_device *p_pdev;
static DEFINE_MUTEX(miup_region_op_lock);

int mtk_miup_set_protect(struct mtk_miup_para miup) __attribute__((weak));
int mtk_miup_get_protect(u32 prot_type, u32 prot_blk,
		struct mtk_miup_para *miup) __attribute__((weak));

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
	struct mtk_miup *pdata = NULL;

	pdata = dev_get_drvdata(dev);
	MIUP_DPRI(pdata->loglevel, dev, "start_addr:%llX, addr_len:%llX",
		miup->start_addr, miup->addr_len);
	/* check address */
	if (miup->addr_len < PROT_ADDR_ALIGN ||
	((miup->start_addr & (PROT_ADDR_ALIGN - 1)) != 0) ||
	((miup->addr_len & (PROT_ADDR_ALIGN - 1)) != 0)) {
		MIUP_DPRW(pdata->loglevel, dev, "invalid address");
		return -EINVAL;
	}

	return 0;
}

static int mtk_miup_check_kprot(struct mtk_miup *pdata,
				struct mtk_miup_para *miup)
{
	u32 idx;

	if (miup->cid_len != pdata->kcid_num)
		return -1;

	for (idx = 0; idx < pdata->kcid_num; idx++) {
		if (miup->cid[idx] != pdata->kcid[idx])
			return -1;
	}
	return 0;
}

static void mtk_miup_check_prot_status(struct device *dev)
{
	int i, j;
	struct mtk_miup *pdata = NULL;
	struct mtk_miup_para miup;

	pdata = dev_get_drvdata(dev);
	miup.cid = kcalloc(pdata->max_cid_number, sizeof(u64), GFP_KERNEL);
	if (!miup.cid) {
		pr_err("wrong memory\n");
		return;
	}

	miup.cid_len = 0;
	prot_stat = 0;
	kprot_stat = 0;
	for (i = 0; i < pdata->max_prot_blk; i++) {
		if (mtk_miup_get_protect(0, i, &miup) < 0)
			goto NEXT_BLK;
		prot_stat |= (1 << i);

		if (mtk_miup_check_kprot(pdata, &miup) < 0)
			goto NEXT_BLK;
		kprot_stat |= (1 << i);
NEXT_BLK:
		for (j = 0; j < pdata->max_cid_number; j++)
			miup.cid[j] = 0x0;
		miup.cid_len = 0;
	}
	kfree(miup.cid);
}

static int mtk_miup_check_prot_status_with_sort(struct device *dev,
						int *sort_blk,
						struct mtk_miup_para *miup_list)
{
	int i, sort_cnt = 0, sort_to, sort_tmp;
	struct mtk_miup *pdata = NULL;

	pdata = dev_get_drvdata(dev);

	prot_stat = 0;
	kprot_stat = 0;
	for (i = 0; i < pdata->max_prot_blk; i++) {
		miup_list[i].cid_len = 0;
		if (mtk_miup_get_protect(0, i, &miup_list[i]) < 0)
			continue;
		prot_stat |= (1 << i);

		if (mtk_miup_check_kprot(pdata, &miup_list[i]) < 0)
			continue;
		kprot_stat |= (1 << i);
		sort_blk[sort_cnt] = i;
		sort_cnt++;
	}
	if (sort_cnt == 0)
		return sort_cnt;

	sort_to = sort_cnt - 1;
	while (sort_to > 0) {
		for (i = 0; i < sort_to; i++) {
			if (miup_list[sort_blk[i]].start_addr >
			    miup_list[sort_blk[i + 1]].start_addr) {
				sort_tmp = sort_blk[i + 1];
				sort_blk[i + 1] = sort_blk[i];
				sort_blk[i] = sort_tmp;
			}
		}
		sort_to--;
	}
	return sort_cnt;
}

static int mtk_miup_find_empty_prot_blk(unsigned int max_prot_blk)
{
	int i;

	for (i = 0; i < max_prot_blk; i++) {
		if (!((prot_stat >> i) & 0x1))
			return i;
	}

	return -1;
}

static int mtk_miup_split_kprot(struct device *dev,
				struct mtk_miup_para *req_miup)
{
	int i, j, ret = 0;
	struct mtk_miup *pdata = NULL;
	struct mtk_miup_para miup;
	u64 req_start, req_end, now_start, now_end;
	int empty_blk;

	pdata = dev_get_drvdata(dev);
	if (pdata->kcid_num == 0) {
		MIUP_DPRW(pdata->loglevel, dev, "no kernel protect\n");
		return ret;
	}
	mtk_miup_check_prot_status(dev);
	MIUP_DPRW(pdata->loglevel, dev, "prot[%X], kprot[%X]\n", prot_stat, kprot_stat);
	if (kprot_stat == 0) {
		MIUP_DPRW(pdata->loglevel, dev, "no kernel protect\n");
		return ret;
	}
	req_start = req_miup->start_addr;
	req_end = req_miup->start_addr + req_miup->addr_len;
	MIUP_DPRI(pdata->loglevel, dev, "req_st[%llX]req_en[%llX]\n", req_start, req_end);
	miup.cid_len = 0;
	miup.cid = kcalloc(pdata->max_cid_number, sizeof(u64), GFP_KERNEL);
	if (!miup.cid)
		return -ENOMEM;
	for (j = 0; j < pdata->max_cid_number; j++)
		miup.cid[j] = 0x0;
	for (i = 0; i < pdata->max_prot_blk; i++) {
		if (!((kprot_stat >> i) & 0x1))
			continue;
		if (mtk_miup_get_protect(0, i, &miup) < 0)
			goto NEXT_BLK;
		now_start = miup.start_addr;
		now_end = miup.start_addr + miup.addr_len;
		MIUP_DPRI(pdata->loglevel, dev, "blk_st[%llX]blk_en[%llX]\n", now_start, now_end);
		if (req_start < now_start) {
			if (req_end <= now_start) {
				MIUP_DPRI(pdata->loglevel, dev, "no change\n");
				goto NEXT_BLK;
			}
			if (req_end >= now_end) {
				miup.attr = MTK_MIUP_CLEANPROT;
				mtk_miup_set_protect(miup);
				kprot_stat &= (u8)(~BIT(i));
				prot_stat &= (u8)(~BIT(i));
				MIUP_DPRI(pdata->loglevel, dev, "clean protect\n");
				goto NEXT_BLK;
			}
			miup.start_addr = req_end;
			miup.addr_len = (now_end - req_end);
			mtk_miup_set_protect(miup);
			MIUP_DPRI(pdata->loglevel, dev, "change protect\n");
		} else if (req_start == now_start) {
			if (req_end < now_end) {
				miup.start_addr = req_end;
				miup.addr_len = (now_end - req_end);
				mtk_miup_set_protect(miup);
				MIUP_DPRI(pdata->loglevel, dev, "change protect\n");
				goto NEXT_BLK;
			}
			miup.attr = MTK_MIUP_CLEANPROT;
			mtk_miup_set_protect(miup);
			kprot_stat &= (u8)(~BIT(i));
			prot_stat &= (u8)(~BIT(i));
			MIUP_DPRI(pdata->loglevel, dev, "clean protect\n");
		} else if (req_start < now_end) {
			if (req_end < now_end) {
				empty_blk = mtk_miup_find_empty_prot_blk(pdata->max_prot_blk);
				if (empty_blk < 0) {
					pr_emerg("NO MORE PROTECT BLOCK\n");
					ret = -EPERM;
					goto RET_FUN;
				}
				miup.start_addr = req_end;
				miup.addr_len = (now_end - req_end);
				miup.prot_blk = (u32)empty_blk;
				mtk_miup_set_protect(miup);
				kprot_stat |= (u8)(BIT(i));
				prot_stat |= (u8)(BIT(i));

				miup.start_addr = now_start;
				miup.addr_len = (req_start - now_start);
				miup.prot_blk = (u32)i;
				mtk_miup_set_protect(miup);
				MIUP_DPRI(pdata->loglevel, dev, "split protect\n");
			}
			miup.addr_len = (req_start - now_start);
			mtk_miup_set_protect(miup);
			MIUP_DPRI(pdata->loglevel, dev, "change protect\n");
		} else {
			MIUP_DPRI(pdata->loglevel, dev, "no change\n");
			goto NEXT_BLK;
		}
NEXT_BLK:
		miup.cid_len = 0;
		for (j = 0; j < pdata->max_cid_number; j++)
			miup.cid[j] = 0x0;
	}

RET_FUN:
	kfree(miup.cid);
	return ret;
}

static int mtk_miup_do_protect(struct device *dev,
			       struct mtk_miup_para *req_miup)
{
	struct mtk_miup *pdata = NULL;
	struct mtk_miup_para miup;
	int empty_blk;

	pdata = dev_get_drvdata(dev);
	empty_blk = mtk_miup_find_empty_prot_blk(pdata->max_prot_blk);
	if (empty_blk < 0) {
		pr_emerg("NO MORE PROTECT BLOCK\n");
		return -EPERM;
	}
	miup.prot_blk = empty_blk;
	miup.start_addr = req_miup->start_addr;
	miup.addr_len = req_miup->addr_len;
	miup.attr = req_miup->attr;
	miup.cid = pdata->kcid;
	miup.cid_len = pdata->kcid_num;
	mtk_miup_set_protect(miup);
	kprot_stat |= (u8)(BIT(empty_blk));
	prot_stat |= (u8)(BIT(empty_blk));

	return empty_blk;
}

static int mtk_miup_do_merge(struct device *dev,
			     struct mtk_miup_para *req_miup,
			     struct mtk_miup *pdata)
{
	int i, sort_cnt;
	int sort_blk[kprot_num];
	struct mtk_miup_para miup_list[kprot_num];
	struct mtk_miup_para new_miup;
	u64 front_st, front_end, end_st, end_end;
	bool go_new_protect = true;

	while (go_new_protect) {
		go_new_protect = false;
		for (i = 0; i < pdata->max_prot_blk; i++) {
			sort_blk[i] = -1;
			miup_list[i].cid = kcalloc(pdata->max_cid_number, sizeof(u64), GFP_KERNEL);
		}
		sort_cnt = mtk_miup_check_prot_status_with_sort(dev, &sort_blk[0], &miup_list[0]);
		if (sort_cnt == 0)
			goto CONT_WHILE_LOOP;

		for (i = 0; i < (sort_cnt - 1); i++) {
			front_st = miup_list[sort_blk[i]].start_addr;
			front_end = miup_list[sort_blk[i]].start_addr +
				    miup_list[sort_blk[i]].addr_len;
			end_st = miup_list[sort_blk[i + 1]].start_addr;
			end_end = miup_list[sort_blk[i + 1]].start_addr +
				  miup_list[sort_blk[i + 1]].addr_len;
			if (end_st > front_end)
				continue;

			new_miup.attr = miup_list[sort_blk[i]].attr;
			new_miup.prot_blk = sort_blk[i];
			new_miup.start_addr = front_st;
			new_miup.addr_len = (end_end > front_end) ?
					    (end_end - front_st) : (front_end - front_st);
			new_miup.cid = pdata->kcid;
			new_miup.cid_len = pdata->kcid_num;
			mtk_miup_set_protect(new_miup);
			new_miup.attr = MTK_MIUP_CLEANPROT;
			new_miup.prot_blk = sort_blk[i + 1];
			mtk_miup_set_protect(new_miup);
			go_new_protect = true;
			break;
		}

CONT_WHILE_LOOP:
		for (i = 0; i < pdata->max_prot_blk; i++)
			kfree(miup_list[i].cid);
	}

	return 0;
}

static int mtk_miup_merge_kprot(struct device *dev,
				struct mtk_miup_para *req_miup)
{
	int ret = 0, empty_blk;
	struct mtk_miup *pdata = NULL;

	pdata = dev_get_drvdata(dev);
	if (pdata->kcid_num == 0) {
		MIUP_DPRW(pdata->loglevel, dev, "no kernel protect client list\n");
		return ret;
	}
	mtk_miup_check_prot_status(dev);
	MIUP_DPRW(pdata->loglevel, dev, "prot[%X], kprot[%X]\n", prot_stat, kprot_stat);
	if (kprot_stat == 0) {
		MIUP_DPRW(pdata->loglevel, dev, "no kernel protect\n");
		return ret;
	}
	empty_blk = mtk_miup_do_protect(dev, req_miup);
	if (empty_blk < 0) {
		MIUP_DPRE(pdata->loglevel, dev, "no more protect block\n");
		return -EBUSY;
	}

	mtk_miup_do_merge(dev, req_miup, pdata);
	return ret;
}

/*********************************************************************
 *                         Public Function                           *
 *********************************************************************/
/*
 * call for a region to be miup again
 * start_addr/addr_len should be PROT_ADDR_ALIGN aligned
 * May sleep, do not use in atomic context
 */
int mtk_miup_rel_kprot(u64 start_addr,
			u64 addr_len)
{
	int ret = 0;
	struct mtk_miup_para miup;
	struct device *dev = NULL;

	dev = mtk_miup_get_miup_device("mediatek,mtk-miup");
	if (!dev) {
		MIUP_PRE("no device");
		return -ENODEV;
	}

	miup.prot_blk = 0;
	miup.attr = MTK_MIUP_WPROT;
	miup.start_addr = start_addr;
	miup.addr_len = addr_len;
	miup.cid_len = 0;
	miup.cid = NULL;

	/* check input value */
	ret = mtk_miup_check_para(dev, &miup);
	if (ret < 0)
		goto RET_FUN;

	mutex_lock(&miup_region_op_lock);
	ret = mtk_miup_merge_kprot(dev, &miup);
	mutex_unlock(&miup_region_op_lock);

RET_FUN:
	if (ret < 0)
		pr_emerg("(%s)Fail:0x%llx 0x%llx\n", __func__, start_addr, addr_len);

	return 0;
}
EXPORT_SYMBOL(mtk_miup_rel_kprot);

/*
 * call for a region to be non-miup
 * start_addr/addr_len should be PROT_ADDR_ALIGN aligned
 * May sleep, do not use in atomic context
 */
int mtk_miup_req_kprot(u64 start_addr,
			u64 addr_len)
{
	int ret = 0;
	struct mtk_miup_para miup;
	struct device *dev = NULL;

	dev = mtk_miup_get_miup_device("mediatek,mtk-miup");
	if (!dev) {
		MIUP_PRE("no device");
		return -ENODEV;
	}

	miup.prot_blk = 0;
	miup.attr = MTK_MIUP_WPROT;
	miup.start_addr = start_addr;
	miup.addr_len = addr_len;
	miup.cid_len = 0;
	miup.cid = NULL;

	/* check input value */
	ret = mtk_miup_check_para(dev, &miup);
	if (ret < 0)
		goto RET_FUN;

	mutex_lock(&miup_region_op_lock);
	ret = mtk_miup_split_kprot(dev, &miup);
	mutex_unlock(&miup_region_op_lock);

RET_FUN:
	if (ret < 0)
		pr_emerg("(%s)Fail:0x%llx 0x%llx\n", __func__, start_addr, addr_len);

	return ret;
}
EXPORT_SYMBOL(mtk_miup_req_kprot);

static int mtk_miup_get_cpu_base(u32 *cpu_base)
{
	struct device_node *np = NULL;

	np = of_find_node_by_name(np, "memory_info");
	if (np)
		return of_property_read_u32(np, "cpu_emi0_base", cpu_base);

	return -1;
}

//static void mtk_miup_set_cma(void)
//{
//	struct device_node *np = NULL;
//	u32 tmp[MAX_REG_SIZE];
//	u64 start_addr;
//	u64 addr_len;
//	u32 cpu_base;

//	if (mtk_miup_get_cpu_base(&cpu_base) < 0)
//		return;

//	np = of_find_compatible_node(np, NULL, "shared-dma-pool");
//	while (np) {
//		if (!of_find_property(np, "heap_name", NULL))
//			goto NEXT_CMA;
//		if (of_property_read_u32_array(np, "reg", tmp, MAX_REG_SIZE))
//			goto NEXT_CMA;
//		start_addr = ((((u64)tmp[START_ADDR_H] << SHIFT_32BIT) |
//				(u64)tmp[START_ADDR_L]) - cpu_base);
//		addr_len = (((u64)tmp[LEN_ADDR_H] << SHIFT_32BIT) |
//			     (u64)tmp[LEN_ADDR_L]);
//		mtk_miup_req_kprot(start_addr, addr_len);
//NEXT_CMA:
//		np = of_find_compatible_node(np, NULL, "shared-dma-pool");
//	}
//}

static void mtk_miup_set_local_dimming_patch(void)
{
	struct device_node *np = NULL;
	u32 tmp[MAX_REG_SIZE];
	u64 start_addr;
	u64 addr_len;
	u32 cpu_base;

	if (mtk_miup_get_cpu_base(&cpu_base) < 0)
		return;
	np = of_find_node_by_name(np, "reserved_local_dimming");
	if (np) {
		if (of_property_read_u32_array(np, "reg", tmp, MAX_REG_SIZE))
			return;
		start_addr = ((((u64)tmp[START_ADDR_H] << SHIFT_32BIT) |
				(u64)tmp[START_ADDR_L]) - cpu_base);
		addr_len = (((u64)tmp[LEN_ADDR_H] << SHIFT_32BIT) |
			     (u64)tmp[LEN_ADDR_L]);
		mtk_miup_req_kprot(start_addr, addr_len);
	}
}

static struct platform_driver mtk_kernel_protect_driver = {
	.driver = {
		.name	= "mtk-kmiup",
		.owner	= THIS_MODULE,
	},
};

static int __init mtk_kmiup_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_kernel_protect_driver);
	if (ret != 0) {
		MIUP_PRW("register kernel protect driver failed");
		return ret;
	}

	p_dev_node = NULL;
	p_pdev = NULL;
//	mtk_miup_set_cma();
	mtk_miup_set_local_dimming_patch();
	return ret;
}

module_init(mtk_kmiup_init);

MODULE_DESCRIPTION("MediaTek kernerl MIUP driver");
MODULE_AUTHOR("Max-CH.Tsai <Max-CH.Tsai@mediatek.com>");
MODULE_LICENSE("GPL");
