/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_BUFFER_H
#define _MTK_PQ_BUFFER_H

#include <linux/types.h>
#include <linux/videodev2.h>

#include "m_pqu_pq.h"
#include "mtk-reserved-memory.h"

#define free(buf) kfree(buf)
#define malloc(size) kzalloc((size), GFP_KERNEL)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define BIT_PER_BYTE	8
#define BYTE_PER_WORD	32
#define ADDRESS_ALIGN	(BYTE_PER_WORD * BIT_PER_BYTE)
#define BUSADDRESS_TO_IPADDRESS_OFFSET	0x20000000
#define MCDI_BUFFER_SIZE		0x100000
#define DV_CONFIG_BIN_OFFSET		(0x400)
#define DV_CONFIG_BIN_DEFAULT_SIZE	(10000)
#define MMAP_MCDI_INDEX             0
#define MMAP_ABF_INDEX              1
#define MMAP_ZNR_INDEX              2
#define MMAP_PQMAP_INDEX            3
#define MMAP_CONTROLMAP_INDEX       4
#define MMAP_DV_PYR_INDEX           5
#define MMAP_DV_CONFIG_INDEX        6
#define MMAP_STREAM_METADATA_INDEX  7
#define MMAP_METADATA_INDEX         8
#define MMAP_PQPARAM_INDEX          9
#define MMAP_DDBUF_MAX              10

struct pq_buffer {
	bool valid;
	uint32_t size;
	uint64_t addr;	// by HW
	uint64_t va;	// by CPU
	uint64_t addr_ch[PQU_BUF_CH_MAX];
	uint32_t size_ch[PQU_BUF_CH_MAX];
	uint8_t bit[PQU_BUF_CH_MAX];
	uint8_t frame_num;
	uint8_t frame_diff;
	bool sec_buf;
};

struct mtk_pq_platform_device;

/* function */
int mtk_pq_buf_get_iommu_idx(struct mtk_pq_platform_device *pqdev, enum pqu_buffer_type buf_type);
int mtk_pq_buffer_reserve_buf_init(struct mtk_pq_platform_device *pqdev);
int mtk_pq_buffer_reserve_buf_exit(struct mtk_pq_platform_device *pqdev);
int mtk_pq_buffer_buf_init(struct mtk_pq_device *pq_dev);
int mtk_pq_buffer_buf_exit(struct mtk_pq_device *pq_dev);
int mtk_pq_buffer_get_hwmap_info(struct mtk_pq_device *pq_dev);
int mtk_pq_buffer_allocate(struct mtk_pq_device *pq_dev);
int mtk_pq_buffer_release(struct mtk_pq_device *pq_dev);
int mtk_get_pq_buffer(struct mtk_pq_device *pq_dev,
	enum pqu_buffer_type buftype, struct pq_buffer *pq_buf);
#endif
