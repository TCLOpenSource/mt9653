/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_METABUF_H_
#define _MTK_TV_DRM_METABUF_H_

#include <linux/dma-buf.h>

//--------------------------------------------------------------------------------------------------
//  Defines
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Enum
//--------------------------------------------------------------------------------------------------
enum mtk_tv_drm_metabuf_type {
	MTK_TV_DRM_METABUF_TYPE_ION,
	MTK_TV_DRM_METABUF_TYPE_MMAP,
	MTK_TV_DRM_METABUF_TYPE_MAX
};

enum mtk_tv_drm_metabuf_mmap_type {
	MTK_TV_DRM_METABUF_MMAP_TYPE_GOP,
	MTK_TV_DRM_METABUF_MMAP_TYPE_DEMURA,
	MTK_TV_DRM_METABUF_MMAP_TYPE_PQU,
	MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_MATADATA,
	MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA,
	MTK_TV_DRM_METABUF_MMAP_TYPE_MAX
};

//--------------------------------------------------------------------------------------------------
//  Structures
//--------------------------------------------------------------------------------------------------
struct mtk_tv_drm_metabuf_ion_info {
	uint32_t ion_fd;
	struct dma_buf *dma_buf;
};

struct mtk_tv_drm_metabuf_mmap_info {
	enum mtk_tv_drm_metabuf_mmap_type mmap_type;
	uint64_t bus_addr;
	uint64_t phy_addr;
};

struct mtk_tv_drm_metabuf {
	void    *addr;
	uint64_t size;

	enum mtk_tv_drm_metabuf_type metabuf_type;
	union {
		struct mtk_tv_drm_metabuf_ion_info ion_info;
		struct mtk_tv_drm_metabuf_mmap_info mmap_info;
	};
};

//--------------------------------------------------------------------------------------------------
//  Global Functions
//--------------------------------------------------------------------------------------------------
int mtk_tv_drm_metabuf_alloc_by_mmap(
	struct mtk_tv_drm_metabuf *drm_metabuf,
	enum mtk_tv_drm_metabuf_mmap_type mmap_type);

int mtk_tv_drm_metabuf_alloc_by_ion(
	struct mtk_tv_drm_metabuf *drm_metabuf,
	int ion_fd);

int mtk_tv_drm_metabuf_free(
	struct mtk_tv_drm_metabuf *drm_metabuf);

#endif
