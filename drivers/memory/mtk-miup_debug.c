// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/limits.h>

#include "mtk-miup.h"
#include <soc/mediatek/mtktv-miup.h>

int mtk_miup_req_kprot(u64 start_addr, u64 addr_len) __attribute__((weak));
int mtk_miup_rel_kprot(u64 start_addr, u64 addr_len) __attribute__((weak));
int mtk_miup_set_protect(struct mtk_miup_para miup) __attribute__((weak));
int mtk_miup_get_protect(
		u32 prot_type, u32 prot_blk,
		struct mtk_miup_para *miup)
		__attribute__((weak));
int mtk_miup_get_hit_status(
		struct mtk_miup_hitlog *hit_log)
		__attribute__((weak));


static void sort_block(s32 max_block, u64 *block_saddr, s8 *block_seq)
{
	s32 idx, idx1;
	u64 min_addr = U64_MAX;
	s8 min_idx = -1;

	for (idx = 0; idx < max_block; idx++) {
		min_addr = U64_MAX;
		min_idx = -1;
		for (idx1 = 0 ; idx1 < max_block; idx1++) {
			if (block_saddr[idx1] < min_addr) {
				min_addr = block_saddr[idx1];
				min_idx = idx1;
			}
		}
		if (min_idx == -1)
			continue;
		block_saddr[min_idx] = U64_MAX;
		block_seq[idx] = min_idx;
	}
}

static void get_protect(struct mtk_miup *pdata,
			struct mtk_miup_para *miup,
			s8 *block_seq,
			u64 *block_saddr)
{
	u32 idx = 0;

	for (idx = 0; idx < pdata->max_prot_blk; idx++) {
		block_seq[idx] = -1;
		block_saddr[idx] = U64_MAX;
		if (mtk_miup_get_protect(0, idx, &miup[idx]) < 0)
			continue;
		block_saddr[idx] = miup[idx].start_addr;
	}
	sort_block(pdata->max_prot_blk, block_saddr, block_seq);
}

static ssize_t protect_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, count = 0;
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	struct mtk_miup_para bstatus;
	struct mtk_miup_para *miup;
	struct mtk_miup_hitlog hit_log;
	int hitted = 0;
	u32 idx = 0;
	u32 idx1 = 0;
	int dram_size = 0;
	s8 *block_seq;
	u64 *block_saddr;

	bstatus.start_addr = 0;
	bstatus.addr_len = 0;
	dram_size = mtk_miup_get_protect(1, 0, &bstatus);

	miup = kcalloc(pdata->max_prot_blk, sizeof(struct mtk_miup_para), GFP_KERNEL);
	for (i = 0; i < pdata->max_prot_blk; i++)
		miup[i].cid = kcalloc(pdata->max_cid_number, sizeof(u64), GFP_KERNEL);
	block_seq = kcalloc(pdata->max_prot_blk, sizeof(s8), GFP_KERNEL);
	block_saddr = kcalloc(pdata->max_prot_blk, sizeof(s64), GFP_KERNEL);

	get_protect(pdata, miup, block_seq, block_saddr);
	hitted = mtk_miup_get_hit_status(&hit_log);

	count += scnprintf(buf + count, PAGE_SIZE, "\033[7mMemory Protect Unit\033[0m\n");
	if (dram_size >= 0) {
		count += scnprintf(buf + count, PAGE_SIZE,
		"\033[1;31;47m%*s\033[0m<-DRAM START:0x%llX->\n", 12, "", bstatus.start_addr);
	}
	for (idx = 0; idx < pdata->max_prot_blk; idx++) {
		if (block_seq[idx] == -1)
			continue;

		count += scnprintf(buf + count, PAGE_SIZE,
		"\033[1;31;47m \033[0m==========\033[1;31;47m \033[0m");

		count += scnprintf(buf + count, PAGE_SIZE,
		"<start addr:0x%llX>", miup[block_seq[idx]].start_addr);

		if (miup[block_seq[idx]].attr == MTK_MIUP_RPROT)
			count += scnprintf(buf + count, PAGE_SIZE, "read protect\n");
		else if (miup[block_seq[idx]].attr == MTK_MIUP_WPROT)
			count += scnprintf(buf + count, PAGE_SIZE, "write protect\n");
		else if (miup[block_seq[idx]].attr == MTK_MIUP_RWPROT)
			count += scnprintf(buf + count, PAGE_SIZE, "read/write protect\n");

		count += scnprintf(buf + count, PAGE_SIZE,
		"\033[1;31;47m \033[0m  block%d  \033[1;31;47m \033[0m", block_seq[idx]);
		if (miup[block_seq[idx]].cid_len != 0) {
			count += scnprintf(buf + count, PAGE_SIZE, "accessible ID:");
			for (idx1 = 0; idx1 < miup[block_seq[idx]].cid_len; idx1++) {
				if (idx1 == 8) {
					count += scnprintf(buf + count,
					PAGE_SIZE,
					"\n\033[1;31;47m \033[0m%*s", 10, "");
					count += scnprintf(buf + count,
					PAGE_SIZE, "\033[1;31;47m \033[0m");
				}
				count += scnprintf(buf + count, PAGE_SIZE, "0x%llX%s",
				miup[block_seq[idx]].cid[idx1],
				(idx1 == miup[block_seq[idx]].cid_len-1) ? "":",");
			}
		}
		if (hitted && hit_log.hit_blk == block_seq[idx]) {
			count += scnprintf(buf + count, PAGE_SIZE, "\n\033[1;31;47m \033[0m");
			count += scnprintf(buf + count, PAGE_SIZE,
			"  \033[31mHITTED\033[0m  \033[1;31;47m \033[0m ");
			count += scnprintf(buf + count, PAGE_SIZE, "\033[31mCID: 0x%llX %s",
			hit_log.cid, (hit_log.attr == 0) ? "read" : "write");
			count += scnprintf(buf + count, PAGE_SIZE,
			" address:0x%llX - 0x%llX\033[0m",
			hit_log.hit_addr, hit_log.hit_addr + 0x1F);
		}
		count += scnprintf(buf + count, PAGE_SIZE,
		"\n\033[1;31;47m \033[0m==========\033[1;31;47m \033[0m");
		count += scnprintf(buf + count, PAGE_SIZE, "<end addr:0x%llX>\n",
		(miup[block_seq[idx]].start_addr + miup[block_seq[idx]].addr_len - 1));
	}
	if (dram_size >= 0) {
		count += scnprintf(buf + count, PAGE_SIZE,
		"\033[1;31;47m%*s\033[0m<-DRAM END:0x%llX->\n",
		12, "", (bstatus.start_addr + bstatus.addr_len - 1));
	}

	if (hitted && hit_log.hit_blk > pdata->max_prot_blk) {
		count += scnprintf(buf + count, PAGE_SIZE,
		"\033[31mHITTED CID: 0x%llX %s overbound",
		hit_log.cid, (hit_log.attr == 0) ? "read" : "write");
		count += scnprintf(buf + count, PAGE_SIZE,
		" address:0x%llX - 0x%llX\033[0m\n",
		hit_log.hit_addr, hit_log.hit_addr + 0x1F);
	}

	for (i = 0; i < pdata->max_prot_blk; i++)
		kfree(miup[i].cid);
	kfree(miup);
	kfree(block_saddr);
	kfree(block_seq);
	return count;
}

static u32 check_match_char(char *buf, size_t count, char match_char)
{
	u32 ret = 0;
	u32 idx = 0;

	for (idx = 0; buf[idx]; idx++) {
		if (buf[idx] == match_char)
			ret++;
	}

	return ret;
}

static u32 get_client_id(char *buf, size_t count, u64 **cid)
{
	u32 cid_len = 0;
	u32 idx = 0;
	char *delim = ",";
	char *pch;

	cid_len = check_match_char(buf, count, *delim) + 1;
	*cid = kcalloc(cid_len, sizeof(u64), GFP_KERNEL);

	pch = strsep(&buf, delim);
	while (pch != NULL) {
		kstrtos64(pch, 0, &((*cid)[idx]));
		idx++;
		pch = strsep(&buf, delim);
	}

	return cid_len;
}

static ssize_t protect_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	char cid[256] = "";
	struct mtk_miup_para miup;

	miup.prot_blk = 0;
	miup.start_addr = 0x0;
	miup.addr_len = 0x0;
	miup.attr = 0;
	miup.cid_len = 0;
	miup.cid = NULL;

	if (sscanf(buf, "%11d %10u %20llx %20llx %255s", &miup.attr,
		&miup.prot_blk, &miup.start_addr,
		&miup.addr_len, cid) < 2)
		goto RETURN_FUNC;

	miup.cid_len = get_client_id(cid, strlen(cid), &miup.cid);
	mtk_miup_set_protect(miup);

RETURN_FUNC:
	kfree(miup.cid);
	return count;
}

static ssize_t interrupt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	struct irq_desc *desc;

	if (pdata->irq < 0)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "no IRQ resource");

	desc = irq_to_desc((unsigned int)pdata->irq);
	if (irqd_irq_disabled(&desc->irq_data))
		return scnprintf(buf, PAGE_SIZE, "%s\n", "miu protect interrupt disabled");
	else
		return scnprintf(buf, PAGE_SIZE, "%s\n", "miu protect interrupt enabled");
}

static ssize_t interrupt_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	s32 flag;
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	struct irq_desc *desc;

	if (pdata->irq < 0)
		goto RETURN_FUNC;

	desc = irq_to_desc((unsigned int)pdata->irq);
	if (kstrtos32(buf, 0, &flag) < 0)
		goto RETURN_FUNC;

	if (flag == 0) {
		if (!irqd_irq_disabled(&desc->irq_data))
			disable_irq((unsigned int)pdata->irq);
	} else if (flag == 1) {
		if (irqd_irq_disabled(&desc->irq_data))
			enable_irq((unsigned int)pdata->irq);
	}

RETURN_FUNC:
	return count;
}

static ssize_t hit_panic_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_miup *pdata = dev_get_drvdata(dev);

	if (pdata->hit_panic)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "MIUP hit with panic");

	return scnprintf(buf, PAGE_SIZE, "%s\n", "MIUP hit without panic");
}

static ssize_t hit_panic_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	s32 flag;
	struct mtk_miup *pdata = dev_get_drvdata(dev);

	if (kstrtos32(buf, 0, &flag) < 0)
		goto RETURN_FUNC;

	if (flag == 0)
		pdata->hit_panic = false;
	else if (flag == 1)
		pdata->hit_panic = true;

RETURN_FUNC:
	return count;
}

static ssize_t loglevel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_miup *pdata = dev_get_drvdata(dev);
	int count = 0;

	switch (pdata->loglevel) {
	case MIUP_LOG_NONE:
		count += scnprintf(buf, PAGE_SIZE, "%s", "no log");
		break;
	case MIUP_LOG_ERR:
		count += scnprintf(buf, PAGE_SIZE, "%s", "error log");
		break;
	case MIUP_LOG_WARN:
		count += scnprintf(buf, PAGE_SIZE, "%s", "warning log");
		break;
	case MIUP_LOG_INFO:
		count += scnprintf(buf, PAGE_SIZE, "%s", "info log");
		break;
	default:
		count += scnprintf(buf, PAGE_SIZE, "%s", "all log");
		break;
	}
	count += scnprintf(buf, PAGE_SIZE, "%s%d%s\n", ", log level(", pdata->hit_panic, ")");
	return count;
}

static ssize_t loglevel_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	u16 loglevel;
	struct mtk_miup *pdata = dev_get_drvdata(dev);

	if (kstrtou16(buf, 0, &loglevel) < 0)
		goto RETURN_FUNC;

	pdata->loglevel = loglevel;

RETURN_FUNC:
	return count;
}

static ssize_t rel_kprot_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	u64 start_addr, addr_len;

	if (sscanf(buf, "%20llx %20llx", &start_addr, &addr_len) != 2)
		goto RETURN_FUNC;

	mtk_miup_rel_kprot(start_addr, addr_len);

RETURN_FUNC:
	return count;
}

static ssize_t req_kprot_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	u64 start_addr, addr_len;

	if (sscanf(buf, "%20llx %20llx", &start_addr, &addr_len) != 2)
		goto RETURN_FUNC;

	mtk_miup_req_kprot(start_addr, addr_len);

RETURN_FUNC:
	return count;
}

static ssize_t help_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count = 0;

	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"[protect] : get protect status and set protect");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (r) : get protect status");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (w) : set protect (*: necessary)");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [*attr] [*block] [start address] [protect length] [cid]");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [attr] - protect type");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            0 : read protect only");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            1 : write protect only");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            2 : read and write protect");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            3 : clean protect");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [block] - protect block indx");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [start addr] - protect start physical address");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [protect length] - protect length");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [cid] - accessible cid, separate by comma");
	count += scnprintf(buf + count, PAGE_SIZE, "\n");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"[interrupt] : get protect interrupt status and set protect interrupt");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (r) : get interrupt status");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (w) : set interrupt");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [attr]");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            0 : disable protect interrupt");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            1 : enable protect interrupt");
	count += scnprintf(buf + count, PAGE_SIZE, "\n");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"[hit_panic] : get hit panic status and set hit panic");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (r) : get hit panic status");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (w) : set hit panic");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [attr]");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            0 : disable hit panic");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            1 : enable hit panic");
	count += scnprintf(buf + count, PAGE_SIZE, "\n");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"[loglevel] : set miup log level");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (r) : get log level");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"    (w) : set log level");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"        [attr]");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            0 : no log");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            1 : error log");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            2 : warning log");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            3 : information log");
	count += scnprintf(buf + count, PAGE_SIZE, "%s\n",
		"            4 : all log");

	return count;
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(protect);
static DEVICE_ATTR_RW(interrupt);
static DEVICE_ATTR_RW(hit_panic);
static DEVICE_ATTR_RW(loglevel);
static DEVICE_ATTR_WO(req_kprot);
static DEVICE_ATTR_WO(rel_kprot);

static struct attribute *miup_attr_list[] = {
	&dev_attr_protect.attr,
	&dev_attr_interrupt.attr,
	&dev_attr_hit_panic.attr,
	&dev_attr_loglevel.attr,
	&dev_attr_help.attr,
	&dev_attr_req_kprot.attr,
	&dev_attr_rel_kprot.attr,
	NULL,
};

static const struct attribute_group mtk_miup_attr_group = {
	.name = "mtk_dbg",
	.attrs = miup_attr_list,
};

static const struct attribute_group *mtk_miup_attr_groups[] = {
	&mtk_miup_attr_group,
	NULL,
};

static int __init mtk_miup_sysfs_init(void)
{
	struct device_node *dev_node;
	struct platform_device *pdev;
	struct device *dev;
	int err = 0;

	/* Get by User */
	/* get device by node */
	dev_node = of_find_compatible_node(NULL, NULL, "mediatek,mtk-miup");
	if (!dev_node) {
		pr_err("[%s][%d]get node fail\n", __func__, __LINE__);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(dev_node);
	if (!pdev) {
		pr_err("[%s][%d]get device fail\n", __func__, __LINE__);
		return -ENODEV;
	}
	dev = &pdev->dev;

	if (!dev)
		return -EINVAL;

	err = sysfs_create_groups(&dev->kobj, mtk_miup_attr_groups);
	if (err) {
		pr_err("%s create group fail\n", __func__);
		return err;
	}

	return 0;
}

static void __exit mtk_miup_sysfs_exit(void)
{
}

module_init(mtk_miup_sysfs_init);
module_exit(mtk_miup_sysfs_exit);

MODULE_LICENSE("GPL");
