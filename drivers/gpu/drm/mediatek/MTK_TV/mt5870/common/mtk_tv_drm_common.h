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

enum mtk_render_common_metatag {
	RD_METATAG_FRM_INFO = 0,
	RD_METATAG_SVP_INFO,
	RD_METATAG_MAX,
};

/* function*/
int mtk_render_common_read_metadata(int meta_fd,
	enum mtk_render_common_metatag meta_tag, void *meta_content);
int mtk_render_common_write_metadata(int meta_fd,
	enum mtk_render_common_metatag meta_tag, void *meta_content);
int mtk_render_common_delete_metadata(int meta_fd,
	enum mtk_render_common_metatag meta_tag);

#endif
