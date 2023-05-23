// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Kevin Ren <kevin.ren@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "mtk_pq.h"
#include "mtk_iommu_dtv_api.h"

#define PQ_DEV_NUM 2   //need to fix

int mtk_pq_buf_get_iommu_idx(struct mtk_pq_device *pq_dev,
	enum pq_buffer_type buf_type)
{
	int idx = 0;

	switch (buf_type) {
	case PQ_BUF_SCMI:
		idx = pq_dev->xc_info.pq_iommu_idx_scmi;
		break;
	case PQ_BUF_ZNR:
		idx = pq_dev->xc_info.pq_iommu_idx_znr;
		break;
	case PQ_BUF_UCM:
		idx = pq_dev->xc_info.pq_iommu_idx_ucm;
		break;
	case PQ_BUF_ABF:
		idx = pq_dev->xc_info.pq_iommu_idx_abf;
		break;
	default:
		break;
	}

	return (idx << 24);
}

static int _mtk_pq_allocate_buf(struct mtk_pq_device *pq_dev,
	enum pq_buffer_type buf_type, __u32 size, struct pq_buffer *ppq_buf)
{
	unsigned int buf_iommu_idx = mtk_pq_buf_get_iommu_idx(pq_dev, buf_type);
	unsigned long iommu_attrs = 0;
	unsigned long long iova[PQ_BUF_ATTR_MAX] = {0};
	unsigned long long va[PQ_BUF_ATTR_MAX] = {0};
	unsigned int idx = 0;
	struct device *dev = pq_dev->dev;

	if (ppq_buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_buf is NULL\n");
		return -1;
	}
/*	need iommu fix
 *	if (dma_get_mask(dev) < DMA_BIT_MASK(34)) {
 *		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
 *			"IOMMU is not registered\n");
 *		return -1;
 *	}
 */
	//check iommu client
	if (!dma_supported(dev, 0)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"IOMMU is not supported\n");
		return -1;
	}

	// set buf tag
	iommu_attrs |= buf_iommu_idx;

	// if mapping uncache
	iommu_attrs |= DMA_ATTR_WRITE_COMBINE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"buf_type : %d, buf_iommu_idx : %x, size = %llx\n",
		buf_type, buf_iommu_idx, size);

	// allocate buffer for every attribute (secure / non-secure for now)
	for (idx = 0; idx < PQ_BUF_ATTR_MAX; idx++) {
		va[idx] = dma_alloc_attrs(dev, size, &iova[idx], GFP_KERNEL, iommu_attrs);
		if (dma_mapping_error(dev, iova[idx]) != 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
				"dma_alloc_attrs fail, buf_attr_idx : %u\n", idx);
			return -1;
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"buf[%u] = {va : %llx, addr : %llx}\n", idx, va[idx], iova[idx]);
	}

	memcpy(ppq_buf->va, va, sizeof(unsigned long long) * PQ_BUF_ATTR_MAX);
	memcpy(ppq_buf->addr, iova, sizeof(unsigned long long) * PQ_BUF_ATTR_MAX);
	ppq_buf->size = size;
	ppq_buf->valid = true;

	return 0;
}

static int _mtk_pq_release_buf(struct mtk_pq_device *pq_dev,
	enum pq_buffer_type buf_type, struct pq_buffer *ppq_buf)
{
	struct device *dev = pq_dev->dev;
	unsigned int buf_iommu_idx = mtk_pq_buf_get_iommu_idx(pq_dev, buf_type);
	unsigned int idx = 0;

	if (ppq_buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_buf is NULL\n");
		return -EPERM;
	}

	// clear valid flag first to avoid racing condition
	ppq_buf->valid = false;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"buf_type : %d, buf_iommu_idx, size : %llx : %x\n",
		buf_type, buf_iommu_idx, ppq_buf->size);

	for (idx = 0; idx < PQ_BUF_ATTR_MAX; idx++) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"idx : %u, va : %llx, addr : %llx\n",
			idx, ppq_buf->va[idx], ppq_buf->addr[idx]);

		dma_free_attrs(dev, ppq_buf->size, ppq_buf->va[idx],
			ppq_buf->addr[idx], buf_iommu_idx);
	}

	memset(ppq_buf, 0, sizeof(struct pq_buffer));
	return 1;
}

int mtk_pq_buffer_win_init(struct v4l2_xc_info *xc_info)
{
	__u8 max_win = xc_info->window_num;

	if (xc_info->pBufferTable != NULL) {
		free(xc_info->pBufferTable);
		xc_info->pBufferTable = NULL;
	}

	xc_info->pBufferTable =
		(struct pq_buffer **)malloc(max_win*sizeof(struct pq_buffer *));

	if (xc_info->pBufferTable != NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"pq buffer window allocate succeesful : %llx\n",
			xc_info->pBufferTable);

		memset(xc_info->pBufferTable, 0,
			max_win * sizeof(struct pq_buffer *));
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"pq buffer window allocate failed!\n");
		return -EPERM;
	}

	return 0;
}

void mtk_pq_buffer_win_exit(struct v4l2_xc_info *xc_info)
{
	if (xc_info->pBufferTable != NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"pq buffer window free : %llx\n",
			xc_info->pBufferTable);
		free(xc_info->pBufferTable);
		xc_info->pBufferTable = NULL;
	}
}

int mtk_pq_buffer_buf_init(struct mtk_pq_device *pq_dev)
{
	__u8 win = pq_dev->dev_indx; //need to fix

	if (pq_dev->xc_info.pBufferTable[win] != NULL) {
		free(pq_dev->xc_info.pBufferTable[win]);
		pq_dev->xc_info.pBufferTable[win] = NULL;
	}

	pq_dev->xc_info.pBufferTable[win] =
		(struct pq_buffer *)malloc(PQ_BUF_MAX*sizeof(struct pq_buffer));

	if (pq_dev->xc_info.pBufferTable[win] != NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"pq buffer allocate succeesful : %llx\n",
			pq_dev->xc_info.pBufferTable[win]);

		memset(pq_dev->xc_info.pBufferTable[win], 0,
			PQ_BUF_MAX*sizeof(struct pq_buffer));
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"pq buffer allocate failed!\n");
		return -EPERM;
	}

	return 0;
}

void mtk_pq_buffer_buf_exit(struct mtk_pq_device *pq_dev)
{
	__u8 win = pq_dev->dev_indx; //need to fix
	enum pq_buffer_type idx = PQ_BUF_SCMI;
	struct pq_buffer *ppq_buf = NULL;
	struct pq_buffer *pBufferTable = pq_dev->xc_info.pBufferTable[win];


	if (pBufferTable != NULL) {
		for (idx = PQ_BUF_SCMI; idx < PQ_BUF_MAX; idx++) {
			ppq_buf = &(pBufferTable[idx]);

			if (ppq_buf->valid == true)
				_mtk_pq_release_buf(pq_dev, idx, ppq_buf);
		}

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"pq buffer free : %llx\n", pBufferTable);

		free(pBufferTable);
		pq_dev->xc_info.pBufferTable[win] = NULL;
	}
}

int mtk_pq_buffer_allocate(struct mtk_pq_device *pq_dev, __s32 BufEn)
{
	__u8	win = pq_dev->dev_indx; //need to fix
	enum pq_buffer_type buf_idx = PQ_BUF_SCMI;
	bool	SCMIEn = BufEn & V4L2_PQ_BUFFER_SCMI,
		ZNREn = BufEn & V4L2_PQ_BUFFER_ZNR,
		UCMEn = BufEn & V4L2_PQ_BUFFER_UCM,
		ABFEn = BufEn & V4L2_PQ_BUFFER_ABF;
	struct pq_buffer *pBufferTable = pq_dev->xc_info.pBufferTable[win];

	if (win >= PQ_DEV_NUM) { //need to fix
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"win num is not valid : %d\n", win);
		return -EPERM;
	}

	if (pBufferTable == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"buftable[%d] is not initialized\n", win);
		return -EPERM;
	}

	for (buf_idx = PQ_BUF_SCMI; buf_idx < PQ_BUF_MAX; buf_idx++) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "win : %d, buf_idx, %d\n",
			win, buf_idx);
		bool BufEn = false;
		struct pq_buffer *ppq_buf = &(pBufferTable[buf_idx]);

		switch (buf_idx) {
		case PQ_BUF_SCMI:
			BufEn = SCMIEn;
			break;
		case PQ_BUF_ZNR:
			BufEn = ZNREn;
			break;
		case PQ_BUF_UCM:
			BufEn = UCMEn;
			break;
		case PQ_BUF_ABF:
			BufEn = ABFEn;
			break;
		default:
			break;
		}

		if (BufEn) {
			__u32 size = 0x1000; //need qmap parsing

			if (ppq_buf->valid == true)
				_mtk_pq_release_buf(pq_dev, buf_idx, ppq_buf);

			_mtk_pq_allocate_buf(pq_dev, buf_idx, size, ppq_buf);

		} else {
			if (ppq_buf->valid == true)
				_mtk_pq_release_buf(pq_dev, buf_idx, ppq_buf);
		}
	}

	return 0;
}

int mtk_get_pq_buffer(struct mtk_pq_device *pq_dev, enum pq_buffer_type buftype,
	struct pq_buffer *ppq_buf)
{
	__u8 win = pq_dev->dev_indx; //need to fix
	unsigned int idx = 0;
	int ret = 1;
	struct pq_buffer *pBufferTable = pq_dev->xc_info.pBufferTable[win];

	if (win >= PQ_DEV_NUM) { /*need to fix*/
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"win num is not valid : %d\n", win);
		return -EPERM;
	}

	if (pBufferTable == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"buftable[%d] is not initialized\n", win);
		return -EPERM;
	}

	if (buftype >= PQ_BUF_MAX) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"buftype is not valid : %d\n", buftype);
		return -EPERM;
	}

	if (ppq_buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"pq_buf is NULL\n");
		return -EPERM;
	}

	ppq_buf->valid = pBufferTable[buftype].valid;
	ppq_buf->size = pBufferTable[buftype].size;
	memcpy(ppq_buf->va, pBufferTable[buftype].va, sizeof(__u64) * PQ_BUF_ATTR_MAX);
	memcpy(ppq_buf->addr, pBufferTable[buftype].addr, sizeof(__u64) * PQ_BUF_ATTR_MAX);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"valid : %d, size : %llx\n", ppq_buf->valid, ppq_buf->size);

	for (idx = 0; idx < PQ_BUF_ATTR_MAX; idx++) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"va[%d] : %llx, addr[%d] : %llx\n",
			idx, ppq_buf->va[idx], ppq_buf->addr[idx]);
	}

	return ret;
}

int mtk_xc_get_buf_info(
	struct v4l2_pq_buf buf,
	struct pq_buffer *fbuf,
	struct pq_buffer *mbuf)
{
	int ret = 0;
	unsigned long long iova = 0;
	unsigned int size = 0;
	u64 va = 0;

	if (WARN_ON(!fbuf) || WARN_ON(!mbuf)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fbuf=0x%x, mbuf=0x%x\n", fbuf, mbuf);
		return -EINVAL;
	}

	/* get frame iova & size from frame_fd */
	ret = mtkd_iommu_getiova(buf.frame_fd, &iova, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtkd_iommu_getiova fail, ret=%d\n", ret);
		return ret;
	}

	fbuf->addr[PQ_BUF_ATTR_NS] = iova;
	fbuf->size = size;
	fbuf->va[PQ_BUF_ATTR_NS] = 0;
	fbuf->valid = true;


	/* get meta va from meta_fd */
	/* static int _mtk_pq_map_metadata(int meta_fd, struct v4l2_pq_buffer_md *md) */
	//ret = _mtk_pq_map_metadata(mbuf[0].meta_fd, va);

	/* get meta iova & size from meta_fd */
	ret = mtkd_iommu_getiova(buf.meta_fd, &iova, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtkd_iommu_getiova fail, ret=%d\n", ret);
		return ret;
	}

	mbuf->addr[PQ_BUF_ATTR_NS] = iova;
	mbuf->size = size;
	mbuf->va[PQ_BUF_ATTR_NS] = va;
	mbuf->valid = true;

	return 0;
}

