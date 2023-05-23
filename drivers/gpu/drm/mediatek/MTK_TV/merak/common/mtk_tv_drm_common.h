/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_COMMON_H_
#define _MTK_TV_DRM_COMMON_H_

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/mtk_tv_drm.h>
#include <drm/drm_print.h>
#include <linux/videodev2.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <uapi/linux/metadata_utility/m_render.h>
#include <uapi/linux/metadata_utility/m_pq.h>

#include "mtk_iommu_dtv_api.h"
#include "metadata_utility.h"
#include "mtk_tv_drm_crtc.h"

enum mtk_render_common_metatag {
	RENDER_METATAG_FRAME_INFO = 0,
	RENDER_METATAG_SVP_INFO,
	RENDER_METATAG_FRAMESYNC_INFO,
	RENDER_METATAG_IDR_CTRL_INFO,
	RENDER_METATAG_DV_OUTPUT_FRAME_INFO,
	RENDER_METATAG_PQDD_DISPLAY_INFO,
	RENDER_METATAG_PQDD_OUTPUT_FRAME_INFO,
	RENDER_METATAG_FRC_SCALING_INFO,
	RENDER_METATAG_PQU_DISPLAY_INFO,
	RENDER_METATAG_MAX,
};

/* function*/
int mtk_render_common_read_metadata(struct dma_buf *meta_db,
	enum mtk_render_common_metatag meta_tag, void *meta_content);
int mtk_render_common_write_metadata(struct dma_buf *meta_db,
	enum mtk_render_common_metatag meta_tag, void *meta_content);
int mtk_render_common_delete_metadata(struct dma_buf *meta_db,
	enum mtk_render_common_metatag meta_tag);
int mtk_render_common_set_stub_mode(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_common_stub *stubInfo);
int mtk_render_common_write_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_render_common_metatag meta_tag, void *meta_content);
int mtk_render_common_read_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_render_common_metatag meta_tag, void *meta_content);

#endif
