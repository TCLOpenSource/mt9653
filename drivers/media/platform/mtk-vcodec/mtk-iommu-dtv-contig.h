/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_IOMMU_DTV_CONTIG_H
#define _MTK_IOMMU_DTV_CONTIG_H

#include <media/videobuf2-v4l2.h>
#include <linux/dma-mapping.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-memops.h>

static inline dma_addr_t
mtk_iommu_dtv_contig_plane_dma_addr(struct vb2_buffer *vb, unsigned int plane_no)
{
	dma_addr_t *addr = vb2_plane_cookie(vb, plane_no);

	return addr != NULL ? *addr : 0;
}

extern const struct vb2_mem_ops mtk_iommu_dtv_contig_memops;

#endif
