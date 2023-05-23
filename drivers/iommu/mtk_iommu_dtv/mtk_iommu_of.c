// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author: Benson liang <benson.liang@mediatek.com>
 */
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/types.h>
#include <linux/slab.h>

#include "mtk_iommu_of.h"

static struct device_node *mtk_iommu_buf_tag;
static LIST_HEAD(buftag_list);
static LIST_HEAD(lx_layout);

static int lx_add(uint64_t start, uint64_t length, struct list_head *head)
{
	struct list_head *pos;
	struct lx_range_node *pos_node = NULL;
	struct lx_range_node *node = NULL, *pre_node = NULL;

	if (length <= 0 || start <= 0)
		return -1;

	pos = head->next;
	while (pos != head) {
		pos_node = list_entry(pos, struct lx_range_node, list);
		if (pos_node->start > start) {
			if (pos_node->start < start + length) {
				pr_info("erro node : pre :0x%llx 0x%llx node 0x%llx 0x%llx\n",
					pos_node->start, pos_node->length,
					start, length);
				return -1;
			}
			if (start + length == pos_node->start) {
				pos_node->start = start;
				pos_node->length += length;
				if (pos->prev != head) {
					pre_node =
					    list_entry(pos->prev,
						struct lx_range_node, list);
					if (pre_node->start + pre_node->length
							== pos_node->start) {
						pos_node->start =
							pre_node->start;
						pos_node->length +=
							pre_node->length;
						list_del(pos->prev);
						kfree(pre_node);
					}
				}
				return 0;
			}
			break;
		}
		pos = pos->next;
	}

	if (pos->prev != head) {
		pre_node = list_entry(pos->prev, struct lx_range_node, list);
		if (pre_node->start + pre_node->length == start) {
			pre_node->length += length;
			return 0;
		}
		if (pre_node->start + pre_node->length > start)
			return -1;
	}

	node = kmalloc(sizeof(*node), GFP_KERNEL);
	if (node == NULL)
		return -1;

	node->start = start;
	node->length = length;
	list_add_tail(&(node->list), pos);

	return 0;
}

static int lx_remove(uint64_t start, uint64_t length, struct list_head *head)
{
	struct list_head *pos = NULL;
	struct lx_range_node *pos_node = NULL;
	uint64_t size;

	if (!start || !length || !head)
		return 0;
AGAIN:
	pos = head->next;
	while (pos != head) {
		pos_node = list_entry(pos, struct lx_range_node, list);
		if (start < pos_node->start) {
			if ((start + length) <= pos_node->start) {
				pos = pos->next;
				continue;
			} else if ((start + length) <
					(pos_node->start + pos_node->length)) {
				list_del(pos);
				size = pos_node->start + pos_node->length - (start + length);
				lx_add(start + length, size, head);
				kfree(pos_node);
				goto AGAIN;
			} else {// start + length >= pos_node->start + pos_node->length
				list_del(pos);
				kfree(pos_node);
				goto AGAIN;
			}
		} else if (start >= pos_node->start
				&& start < (pos_node->start + pos_node->length)) {
			list_del(pos);
			if ((start + length) < (pos_node->start + pos_node->length)) {
				if (start == pos_node->start) {
					size = pos_node->start + pos_node->length -
							(start + length);
					lx_add(start + length, size, head);
				} else {
					size = start - pos_node->start;
					lx_add(pos_node->start, size, head);
					size = pos_node->start + pos_node->length -
						(start + length);
					lx_add(start + length, size, head);
				}
			} else {//start + length >= pos_node->start+pos_node->length
				if (start > pos_node->start)
					lx_add(pos_node->start, start - pos_node->start, head);
			}
			kfree(pos_node);
			goto AGAIN;
		} else {
			pos = pos->next;
			continue;
		}
	}
	return 0;
}

static void free_lx_list(struct list_head *head)
{
	struct list_head *pos = NULL, *free_pos = NULL;
	struct lx_range_node *pos_node = NULL;

	pos = head->next;

	while (pos != head) {
		pos_node = list_entry(pos, struct lx_range_node, list);
		free_pos = pos;
		pos = pos->next;
		list_del(free_pos);
		kfree(pos_node);
	}
}

int mtk_iommu_get_memoryinfo(uint64_t *addr0, uint64_t *size0,
		uint64_t *addr1, uint64_t *size1)
{
	int i;
	struct device_node *node = NULL;
	struct device_node *np = NULL;
	__be32 *p = NULL;
	__be64 *p64 = NULL;
	uint32_t len = 0;
	uint64_t addr = 0, size = 0;
	struct property *pp;
	uint64_t dram_start = 0xFFFFFFFF, dram_end = 0;
	struct lx_range_node lx_mem[2] = {0};
	struct list_head *pos = NULL;
	struct lx_range_node *pos_node = NULL;

	if (!addr0 || !size0 || !addr1 || !size1)
		return -1;

	for (i = 0; i < 8; i++) {
		node = of_find_node_by_type(node, "memory");
		if (!node)
			break;
		np = node;
		p64 = (__be64 *)of_get_property(np, "reg", &len);
		if (!p64)
			continue;

		len = len / sizeof(__be32);
		if (len == 4) {
			addr = be64_to_cpup(p64);
			p64++;
			size = be64_to_cpup(p64);
			if (!size)
				continue;
			pr_info("%s,%d,addr = 0x%llx,size = 0x%llx!\n",
				__func__, __LINE__, addr, size);
			if (addr < dram_start)
				dram_start = addr;
			if (dram_end < addr + size)
				dram_end = addr + size;
		} else if (len == 2) {
			p = (__be32 *)p64;
			addr = be32_to_cpup(p);
			p++;
			size = be32_to_cpup(p);
			if (!size)
				continue;
			pr_info("%s,%d,addr = 0x%llx,size = 0x%llx!\n",
				__func__, __LINE__, addr, size);
			if (addr < dram_start)
				dram_start = addr;
			if (dram_end < addr + size)
				dram_end = addr + size;
		} else {
			pr_err("%s,%d,len = %d!\n",
				__func__, __LINE__, len);
			of_node_put(node);
			return -1;
		}
	}

	if (dram_start >= dram_end) {
		pr_err(
			"%s,%d,dram_start = 0x%llx,dram_end = 0x%llx,error!\n",
			__func__, __LINE__, dram_start, dram_end);
		return -1;
	}

	lx_add(dram_start, dram_end - dram_start, &lx_layout);

	node = of_find_node_by_name(NULL, "reserved-memory");
	if (!node) {
		pr_err("%s,%d,reserved memory no found!\n",
			   __func__, __LINE__);
		free_lx_list(&lx_layout);
		return -1;
	}
	np = node->child;
	if (!np) {
		pr_err("%s,%d,reserved child no found!\n",
			   __func__, __LINE__);
		of_node_put(node);
		free_lx_list(&lx_layout);
		return -1;
	}
	for (; np; np = np->sibling) {
		if (!np->name)
			break;

		of_node_get(np);

		pp = of_find_property(np, "no-map", NULL);
		if (!pp) {
			pp = of_find_property(np, "mediatek,mpu-nomap", NULL);
			if (!pp) {
				// case 1.
				// w/o [no-map] + w/o [mediatek,mpu-nomap] -> mpu map, skip
				of_node_put(np);
				continue;
			}
			// case 2.
			// w/o [no-map] + w/ [mediatek,mpu-nomap] -> mpu no map, going
			pr_info("%s: w/o [no-map] propperty but w/ [mediatek,mpu-nomap]\n",
					np->name);
		} else {
			pp = of_find_property(np, "mediatek,mpu-map", NULL);
			if (pp) {
				// case 3.
				// w/ [no-map] + w/ [mediatek,mpu-map] -> mpu  map, skip
				of_node_put(np);
				continue;
			}
			// case 4.
			// w/ [no-map] + w/o [mediatek,mpu-map] -> mpu no map, going
			pr_info("%s: w/ [no-map] propperty but w/o [mediatek,mpu-map]\n",
					np->name);
		}

		p64 = (__be64 *)of_get_property(np, "reg", &len);
		if (!p64) {
			of_node_put(np);
			continue;
		}

		len = len / sizeof(__be32);
		if (len == 4) {
			addr = be64_to_cpup(p64);
			p64++;
			size = be64_to_cpup(p64);
			if (!size)
				continue;
			pr_info("%s,%d: remove addr = 0x%llx,size = 0x%llx!\n",
				__func__, __LINE__, addr, size);
		} else if (len == 2) {
			p = (__be32 *)p64;
			addr = be32_to_cpup(p);
			p++;
			size = be32_to_cpup(p);
			if (!size)
				continue;
			pr_info("%s,%d: remove addr = 0x%llx,size = 0x%llx!\n",
				__func__, __LINE__, addr, size);
		} else {
			pr_err("%s,%d,len = %d!\n",
				__func__, __LINE__, len);
			of_node_put(np);
			of_node_put(node);
			return -1;
		}

		lx_remove(addr, size, &lx_layout);
		of_node_put(np);
	}

	pos = lx_layout.next;
	if (pos != &lx_layout) {
		pos_node = list_entry(pos, struct lx_range_node, list);
		lx_mem[0].start = pos_node->start;
		lx_mem[0].length = pos_node->length;
	}
	pos = pos->next;
	if (pos != &lx_layout) {
		pos_node = list_entry(pos, struct lx_range_node, list);
		lx_mem[1].start = pos_node->start;
		lx_mem[1].length = pos_node->length;
	}

	if (lx_mem[1].length && lx_mem[1].start) {
		np = node->child;
		if (!np)
			goto OUT;
		for (; np; np = np->sibling) {
			if (!np->name)
				break;

			of_node_get(np);
			pp = of_find_property(np, "no-map", NULL);
			if (pp) {
				pp = of_find_property(np, "mediatek,mpu-map", NULL);
				if (!pp) {
					// case 1.
					// w/ [no-map] + w/o [mediatek,mpu-map] -> mpu no map, skip
					of_node_put(np);
					continue;
				}
				// case 2.
				// w/ [no-map] + w/o [mediatek,mpu-map] -> mpu map, going
				pr_info("%s: w/ [no-map] propperty but w/o [mediatek,mpu-map]\n",
					np->name);
			} else {
				pp = of_find_property(np, "mediatek,mpu-nomap", NULL);
				if (pp) {
					// case 3.
					// w/o [no-map] + w/ [mediatek,mpu-nomap] ->
					// mpu no map, skip
					of_node_put(np);
					continue;
				}
				// case 4.
				// w/o [no-map] + w/o [mediatek,mpu-nomap] -> mpu map, going
				pr_info("%s: w/o [no-map] propperty but w/o [mediatek,nompu-map]\n",
					np->name);
			}

			p64 = (__be64 *) of_get_property(np, "reg", &len);
			if (!p64) {
				of_node_put(np);
				continue;
			}

			len = len / sizeof(__be32);
			if (len == 4) {
				addr = be64_to_cpup(p64);
				p64++;
				size = be64_to_cpup(p64);
			} else if (len == 2) {
				p = (__be32 *)p64;
				addr = be32_to_cpup(p);
				p++;
				size = be32_to_cpup(p);
			} else {
				of_node_put(np);
				continue;
			}
			if (lx_mem[1].start == addr) {
				pr_info("%s/%d: remove2 addr = 0x%llx,size = 0x%llx!\n",
					__func__, __LINE__, addr, size);
				lx_remove(addr, size, &lx_layout);
				break;
			}
			of_node_put(np);
		}

	}
OUT:
	of_node_put(node);
	pos = lx_layout.next;
	if (pos != &lx_layout) {
		pos_node = list_entry(pos, struct lx_range_node, list);
		lx_mem[0].start = pos_node->start;
		lx_mem[0].length = pos_node->length;
	}
	pos = pos->next;
	if (pos != &lx_layout) {
		pos_node = list_entry(pos, struct lx_range_node, list);
		lx_mem[1].start = pos_node->start;
		lx_mem[1].length = pos_node->length;
	}
	free_lx_list(&lx_layout);
	pr_info("%s,%d: mpu0 = 0x%llx,size = 0x%llx,mpu1 = 0x%llx,size = 0x%llx!\n",
		__func__, __LINE__,
		lx_mem[0].start, lx_mem[0].length,
		lx_mem[1].start, lx_mem[1].length);
	*addr0 = lx_mem[0].start;
	*size0 = lx_mem[0].length;
	*addr1 = lx_mem[1].start;
	*size1 = lx_mem[1].length;
	return 0;
}
int mtk_iommu_get_buftag_info(struct buf_tag_info *info)
{
	struct device_node *np = NULL;
	uint32_t len = 0;
	__be32 *p = NULL;
	__be64 *p64 = NULL;

	if (info == NULL)
		return -1;

	if (mtk_iommu_buf_tag == NULL) {
		mtk_iommu_buf_tag = of_find_node_by_name(NULL, "buf_tag");
		if (mtk_iommu_buf_tag == NULL) {
			pr_err("%s  %d,buf_tag no found!\n",
				   __func__, __LINE__);
			return -1;
		}
		if (mtk_iommu_buf_tag->child == NULL) {
			pr_err("%s  %d,buf_tag no found!\n",
				   __func__, __LINE__);
			of_node_put(mtk_iommu_buf_tag);
			return -1;
		}
	}

	np = mtk_iommu_buf_tag->child;
	for (; np; np = np->sibling) {
		p = NULL;
		p = (__be32 *) of_get_property(np, "id", &len);
		if (p) {
			if ((be32_to_cpup(p) == info->id) && of_node_get(np))
				break;
		}
	}
	if (np == NULL) {
		pr_err("%s  %d,buf_tag id =%d no found!\n",
			   __func__, __LINE__, info->id);
		return -1;
	}

	strncpy(info->name, np->name, MAX_NAME_SIZE);
	info->name[MAX_NAME_SIZE - 1] = '\0';

	p = (__be32 *) of_get_property(np, "heaptype", &len);
	if (p)
		info->heap_type = be32_to_cpup(p);

	p = NULL;
	p = (__be32 *) of_get_property(np, "miu", &len);
	if (p)
		info->miu = be32_to_cpup(p);

	p64 = NULL;
	p64 = (__be64 *) of_get_property(np, "max_size", &len);
	if (p64)
		info->maxsize = be64_to_cpup(p64);
	else
		info->maxsize = 0;


	p = NULL;
	p = (__be32 *) of_get_property(np, "normal_zone", &len);
	if (p)
		info->zone = be32_to_cpup(p);
	else
		info->zone = 0;

	of_node_put(np);
	return 0;
}

struct list_head *mtk_iommu_get_buftags(void)
{
	struct device_node *np = NULL;
	uint32_t len = 0;
	__be32 *p = NULL;
	__be64 *p64 = NULL;
	struct buf_tag_info *buf_info;

	if (!list_empty(&buftag_list))
		return &buftag_list;

	if (mtk_iommu_buf_tag == NULL) {
		mtk_iommu_buf_tag = of_find_node_by_name(NULL, "buf_tag");

		if (mtk_iommu_buf_tag == NULL)
			return NULL;
		if (mtk_iommu_buf_tag->child == NULL) {
			of_node_put(mtk_iommu_buf_tag);
			return NULL;
		}
	}

	np = mtk_iommu_buf_tag->child;
	for (; np; np = np->sibling) {
		if (!np->name)
			return NULL;

		of_node_get(np);

		buf_info = kzalloc(sizeof(struct buf_tag_info), GFP_KERNEL);
		if (!buf_info)
			return NULL;

		strncpy(buf_info->name, np->name, MAX_NAME_SIZE);
		buf_info->name[MAX_NAME_SIZE - 1] = '\0';

		p = (__be32 *) of_get_property(np, "id", &len);
		if (p != NULL)
			buf_info->id = be32_to_cpup(p);

		p = (__be32 *) of_get_property(np, "heaptype", &len);
		if (p != NULL)
			buf_info->heap_type = be32_to_cpup(p);

		p = NULL;
		p = (__be32 *) of_get_property(np, "miu", &len);
		if (p != NULL)
			buf_info->miu = be32_to_cpup(p);

		p64 = NULL;
		p64 = (__be64 *) of_get_property(np, "max_size", &len);
		if (p64 != NULL)
			buf_info->maxsize = be64_to_cpup(p64);

		list_add_tail(&(buf_info->list), &buftag_list);
		of_node_put(np);
	}
	return &buftag_list;
}
EXPORT_SYMBOL(mtk_iommu_get_buftags);

int mtk_iommu_get_reserved(struct list_head *head)
{
	struct device_node *np = NULL;
	uint32_t len = 0, id;
	__be32 *p = NULL;
	struct reserved_info *info;
	struct device_node *reserved_tag;
	int count = 0, i;
	struct list_head *list, *tmp;

	if (!head)
		return 0;
	reserved_tag = of_find_node_by_name(NULL, "iova_reserved");

	if (reserved_tag == NULL)
		return 0;

	if (reserved_tag->child == NULL) {
		of_node_put(reserved_tag);
		return 0;
	}


	np = reserved_tag->child;
	for (; np; np = np->sibling) {
		if (!np->name)
			return 0;

		of_node_get(np);

		info = kzalloc(sizeof(struct reserved_info), GFP_KERNEL);
		if (!info)
			goto out;
		strncpy(info->reservation.space_tag, np->name, MAX_NAME_SIZE);
		info->reservation.space_tag[MAX_NAME_SIZE - 1] = '\0';

		p = (__be32 *) of_get_property(np, "size", &len);
		if (p != NULL)
			info->reservation.size = be32_to_cpup(p);
		else {
			kfree(info);
			continue;
		}
		if (info->reservation.size == 0) {
			kfree(info);
			continue;
		}

		p = (__be32 *) of_get_property(np, "buf_tag_num", &len);
		if (p != NULL)
			info->reservation.buf_tag_num = be32_to_cpup(p);
		else {
			kfree(info);
			continue;
		}
		if (info->reservation.buf_tag_num > MAX_TAG_NUM) {
			pr_err("%s,%d, buf_tag_num=%d error\n", __func__,
				__LINE__, info->reservation.buf_tag_num);
			kfree(info);
			goto out;
		}
		if (info->reservation.buf_tag_num == 0) {
			kfree(info);
			continue;
		}
		p = (__be32 *) of_get_property(np, "buf_tag_id", &len);
		if (p == NULL) {
			kfree(info);
			continue;
		}

		for (i = 0; i < info->reservation.buf_tag_num; i++) {
			id = of_read_number(&p[i], 1);
			info->reservation.buf_tag_array[i] = id << BUFTAG_SHIFT;
		}
		pr_info(
			"%s,%d, buf_tag_num=%d ,name=%s,size =0x%x,id=%x,%x,%x,%x,%x,%x,%x,%x\n",
				__func__, __LINE__,
				info->reservation.buf_tag_num,
				info->reservation.space_tag,
				info->reservation.size,
				info->reservation.buf_tag_array[0],
				info->reservation.buf_tag_array[1],
				info->reservation.buf_tag_array[2],
				info->reservation.buf_tag_array[3],
				info->reservation.buf_tag_array[4],
				info->reservation.buf_tag_array[5],
				info->reservation.buf_tag_array[6],
				info->reservation.buf_tag_array[7]);
		count++;
		of_node_put(np);
		list_add_tail(&(info->list), head);
	}
	return count;
out:
	list_for_each_safe(list, tmp, head) {
		list_del_init(list);
		info = container_of(list, struct reserved_info, list);
		kfree(info);
	}
	return 0;
}
