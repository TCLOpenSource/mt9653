/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_TVDAC_H_
#define _MTK_TV_DRM_TVDAC_H_

#include <linux/dma-buf.h>

#define INVALID_NUM		(0xff)
#define ADCTBL_CLK_SET_NUM	(50)
#define ADCTBL_CLK_SWEN_NUM	(50)


#ifndef BIT
#define BIT(_bit_)	(1 << (_bit_))
#endif

#ifndef BMASK
#define BMASK(_bits_)	(BIT(((1)?_bits_)+1)-BIT(((0)?_bits_)))
#endif

struct mtk_tvdac_context {
	struct device *dev;
	int status;
	int reserved;
};

int mtk_tv_drm_set_cvbso_mode_ioctl(
	struct drm_device *drm_dev,
	void *data,
	struct drm_file *file_priv);
int mtk_tvdac_init(struct device *dev);
int mtk_tvdac_deinit(struct device *dev);
int mtk_tvdac_cvbso_suspend(struct platform_device *pdev);
int mtk_tvdac_cvbso_resume(struct platform_device *pdev);

#endif
