/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _MTK_IOMMU_MIXED_MMA_H_
#define _MTK_IOMMU_MIXED_MMA_H_

struct mma_callback {
	/**
	 * @alloc:
	 *
	 * This is called by __mtk_iommu_alloc_attrs() to make mma side (if exist)
	 * to record this buffer allocation.
	 *
	 * @param dmabuf: external dmabuf
	 * @param dmabuf_id: unique dmabuf serial number
	 *
	 * This callback is optional.
	 */
	int (*alloc)(struct dma_buf *db, unsigned int db_id);

	/**
	 * @free:
	 *
	 * This is called by __mtk_iommu_free_attrs() to make mma side (if exist)
	 * to remove this buffer record.
	 *
	 * @param dmabuf: external dmabuf
	 * @param dmabuf_id: unique dmabuf serial number
	 * @param iova: iova
	 *
	 * This callback is optional.
	 */
	int (*free)(struct dma_buf *db, unsigned int db_id, u64 iova);

	/**
	 * @map_iova:
	 *
	 * This is called when iommu driver got iova, we need to make mma side (if exist)
	 * to add iova information into buffer record.
	 *
	 * @param dmabuf: external dmabuf
	 * @param dmabuf_id: unique dmabuf serial number
	 * @param iova: iova
	 *
	 * This callback is optional.
	 */
	int (*map_iova)(struct dma_buf *db, unsigned int db_id, u64 iova);
};

/*
 *-----------------------------------------------------------------
 * mma_callback_register
 * register callback
 * @param  mma_callback      \b IN: callback struct
 * @return 0 : succeed
 *     other : fail
 *-----------------------------------------------------------------
 */
int mma_callback_register(struct mma_callback *mma_cb);
#endif
