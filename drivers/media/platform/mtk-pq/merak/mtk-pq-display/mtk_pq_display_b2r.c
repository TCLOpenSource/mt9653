// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
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
#include <linux/timekeeping.h>
#include <linux/delay.h>


#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <linux/videodev2.h>
#include <linux/dma-buf.h>
#include "metadata_utility.h"

#include "mtk_pq.h"
#include "mtk_pq_display_b2r.h"

#include "drvPQ.h"
#include "apiVDEC_EX.h"
#include "drvMVOP.h"

int mtk_pq_display_b2r_qbuf(struct mtk_pq_device *pq_dev, struct mtk_pq_buffer *pq_buf)
{
	int ret = 0;
	struct meta_buffer meta_buf;
	struct mtk_pq_frame_info *pstFrameFormat = NULL;
	struct m_pq_display_b2r_ctrl *b2r_info = NULL;

	pstFrameFormat = kzalloc(sizeof(struct mtk_pq_frame_info), GFP_KERNEL);
	if (pstFrameFormat == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "malloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	b2r_info = kzalloc(sizeof(struct m_pq_display_b2r_ctrl), GFP_KERNEL);
	if (b2r_info == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "malloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	memset(&meta_buf, 0, sizeof(struct meta_buffer));

	/* meta buffer handle */
	meta_buf.paddr = pq_buf->meta_buf.va;
	meta_buf.size = pq_buf->meta_buf.size;

	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_SH_FRM_INFO, (void *)pstFrameFormat);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_SH_FRM_INFO Failed, ret = %d\n", ret);
		return ret;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_SH_FRM_INFO success\n");
	}

	/* set main b2r info */
	b2r_info->main.luma_fb_offset = pstFrameFormat->vdec_offset.luma_0_offset;
	b2r_info->main.chroma_fb_offset = pstFrameFormat->vdec_offset.chroma_0_offset;
	b2r_info->main.luma_blen_offset = pstFrameFormat->vdec_offset.luma_blen_offset;
	b2r_info->main.chroma_blen_offset = pstFrameFormat->vdec_offset.chroma_blen_offset;
	b2r_info->main.x = (__u16)pstFrameFormat->stFrames[0].u32X;
	b2r_info->main.y = (__u16)pstFrameFormat->stFrames[0].u32Y;
	b2r_info->main.width = (__u16)pstFrameFormat->stFrames[0].u32Width;
	b2r_info->main.height = (__u16)pstFrameFormat->stFrames[0].u32Height;
	b2r_info->main.bitdepth = (__u16)pstFrameFormat->stFrames[0].u8LumaBitdepth;
	b2r_info->main.pitch = (__u16)pstFrameFormat->stFrames[0].stHWFormat.u32LumaPitch;
	b2r_info->main.modifier.tile = pstFrameFormat->modifier.tile;
	b2r_info->main.modifier.raster = pstFrameFormat->modifier.raster;
	b2r_info->main.modifier.compress = pstFrameFormat->modifier.compress;
	b2r_info->main.modifier.jump = pstFrameFormat->modifier.jump;
	b2r_info->main.modifier.vsd_mode = pstFrameFormat->modifier.vsd_mode;
	b2r_info->main.modifier.vsd_ce_mode = pstFrameFormat->modifier.vsd_ce_mode;
#if IS_ENABLED(CONFIG_OPTEE)
	b2r_info->main.aid = pq_dev->display_info.secure_info.aid;
#endif
	b2r_info->main.order_type = pstFrameFormat->order_type;
	b2r_info->main.scan_type = pstFrameFormat->scan_type;
	b2r_info->main.field_type = pstFrameFormat->field_type;
	b2r_info->main.rotate_type = pstFrameFormat->rotate_type;
	if (pstFrameFormat->color_format == MTK_PQ_COLOR_FORMAT_YUYV)
		b2r_info->main.color_fmt_422 = TRUE;

	/* set sub1 b2r info */
	b2r_info->sub1.luma_fb_offset = pstFrameFormat->vdec_offset.sub_luma_0_offset;
	b2r_info->sub1.chroma_fb_offset = pstFrameFormat->vdec_offset.sub_chroma_0_offset;
	b2r_info->sub1.luma_blen_offset = pstFrameFormat->vdec_offset.luma_blen_offset;
	b2r_info->sub1.chroma_blen_offset = pstFrameFormat->vdec_offset.chroma_blen_offset;
	b2r_info->sub1.x = (__u16)pstFrameFormat->stSubFrame.u32X;
	b2r_info->sub1.y = (__u16)pstFrameFormat->stSubFrame.u32Y;
	b2r_info->sub1.width = (__u16)pstFrameFormat->stSubFrame.u32Width;
	b2r_info->sub1.height = (__u16)pstFrameFormat->stSubFrame.u32Height;
	b2r_info->sub1.bitdepth = (__u16)pstFrameFormat->stSubFrame.u8LumaBitdepth;
	b2r_info->sub1.pitch = (__u16)pstFrameFormat->stSubFrame.stHWFormat.u32LumaPitch;
	b2r_info->sub1.modifier.tile = pstFrameFormat->submodifier.tile;
	b2r_info->sub1.modifier.raster = pstFrameFormat->submodifier.raster;
	b2r_info->sub1.modifier.compress = pstFrameFormat->submodifier.compress;
	b2r_info->sub1.modifier.jump = pstFrameFormat->submodifier.jump;
	b2r_info->sub1.modifier.vsd_mode = pstFrameFormat->submodifier.vsd_mode;
	b2r_info->sub1.modifier.vsd_ce_mode = pstFrameFormat->submodifier.vsd_ce_mode;
#if IS_ENABLED(CONFIG_OPTEE)
	b2r_info->sub1.aid = pq_dev->display_info.secure_info.aid;
#endif
	b2r_info->sub1.order_type = pstFrameFormat->order_type;
	b2r_info->sub1.scan_type = pstFrameFormat->scan_type;
	b2r_info->sub1.field_type = pstFrameFormat->field_type;
	b2r_info->sub1.rotate_type = pstFrameFormat->rotate_type;
	if (pstFrameFormat->color_format == MTK_PQ_COLOR_FORMAT_YUYV)
		b2r_info->sub1.color_fmt_422 = TRUE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"%s%llx, %s%llx, %s%llx, %s%llx, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d\n",
		"b2r_info->main.luma_fb_offset = 0x", b2r_info->main.luma_fb_offset,
		"b2r_info->main.chroma_fb_offset = 0x", b2r_info->main.chroma_fb_offset,
		"b2r_info->main.luma_blen_offset = 0x", b2r_info->main.luma_blen_offset,
		"b2r_info->main.chroma_blen_offset = 0x", b2r_info->main.chroma_blen_offset,
		"b2r_info->main.x = ", b2r_info->main.x,
		"b2r_info->main.y = ", b2r_info->main.y,
		"b2r_info->main.width = ", b2r_info->main.width,
		"b2r_info->main.height = ", b2r_info->main.height,
		"b2r_info->main.pitch = ", b2r_info->main.pitch,
		"b2r_info->main.bitdepth = ", b2r_info->main.bitdepth);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"%s%d, %s%d, %s%d, %s%d, %s%d, %s%d\n",
		"b2r_info->main.modifier.tile = ", b2r_info->main.modifier.tile,
		"b2r_info->main.modifier.raster = ", b2r_info->main.modifier.raster,
		"b2r_info->main.modifier.compress = ", b2r_info->main.modifier.compress,
		"b2r_info->main.modifier.jump = ", b2r_info->main.modifier.jump,
		"b2r_info->main.modifier.vsd_mode = ", b2r_info->main.modifier.vsd_mode,
		"b2r_info->main.modifier.vsd_ce_mode = ", b2r_info->main.modifier.vsd_ce_mode);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"%s%u, %s%d, %s%d, %s%d, %s%d, %s%d\n",
		"b2r_info->main.aid = ", b2r_info->main.aid,
		"b2r_info->main.order_type = ", b2r_info->main.order_type,
		"b2r_info->main.scan_type = ", b2r_info->main.scan_type,
		"b2r_info->main.field_type = ", b2r_info->main.field_type,
		"b2r_info->main.rotate_type = ", b2r_info->main.rotate_type,
		"b2r_info->main.color_fmt_422 = ", b2r_info->main.color_fmt_422);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"%s%llx, %s%llx, %s%llx, %s%llx, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d\n",
		"b2r_info->sub1.luma_fb_offset = 0x", b2r_info->sub1.luma_fb_offset,
		"b2r_info->sub1.chroma_fb_offset = 0x", b2r_info->sub1.chroma_fb_offset,
		"b2r_info->sub1.luma_blen_offset = 0x", b2r_info->sub1.luma_blen_offset,
		"b2r_info->sub1.chroma_blen_offset = 0x", b2r_info->sub1.chroma_blen_offset,
		"b2r_info->sub1.x = ", b2r_info->sub1.x,
		"b2r_info->sub1.y = ", b2r_info->sub1.y,
		"b2r_info->sub1.width = ", b2r_info->sub1.width,
		"b2r_info->sub1.height = ", b2r_info->sub1.height,
		"b2r_info->sub1.pitch = ", b2r_info->sub1.pitch,
		"b2r_info->sub1.bitdepth = ", b2r_info->sub1.bitdepth);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"%s%d, %s%d, %s%d, %s%d, %s%d, %s%d\n",
		"b2r_info->sub1.modifier.tile = ", b2r_info->sub1.modifier.tile,
		"b2r_info->sub1.modifier.raster = ", b2r_info->sub1.modifier.raster,
		"b2r_info->sub1.modifier.compress = ", b2r_info->sub1.modifier.compress,
		"b2r_info->sub1.modifier.jump = ", b2r_info->sub1.modifier.jump,
		"b2r_info->sub1.modifier.vsd_mode = ", b2r_info->sub1.modifier.vsd_mode,
		"b2r_info->sub1.modifier.vsd_ce_mode = ", b2r_info->sub1.modifier.vsd_ce_mode);

	ret = mtk_pq_common_write_metadata_addr(&meta_buf,
		EN_PQ_METATAG_DISP_B2R_CTRL, (void *)b2r_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_DISP_B2R_CTRL Failed, ret = %d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_DISP_B2R_CTRL success\n");
	}

exit:
	if (pstFrameFormat != NULL) {
		kfree(pstFrameFormat);
		pstFrameFormat = NULL;
	}

	if (b2r_info != NULL) {
		kfree(b2r_info);
		b2r_info = NULL;
	}

	return ret;
}
