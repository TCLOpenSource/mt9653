// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-----------------------------------------------------------------------------
//  Include Files
//-----------------------------------------------------------------------------

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
#include <uapi/linux/mtk-v4l2-pq.h>

#include "mtk_pq.h"

//-----------------------------------------------------------------------------
//  Driver Compiler Options
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
#define PQ_IDR_FIELD_DEFAULT (0xFF)

//-----------------------------------------------------------------------------
//  Local Structurs
//-----------------------------------------------------------------------------
struct format_info {
	u32 v4l2_fmt;
	u32 meta_fmt;
	u8 yuv_mode;
	u8 plane_num;
	u8 buffer_num;
};

//-----------------------------------------------------------------------------
//  Global Variables
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Local Variables
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Debug Functions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Local Functions
//-----------------------------------------------------------------------------
static bool _is_yuv_422(u32 meta_fmt)
{
	if ((meta_fmt == MEM_FMT_YUV422_Y_UV_12B) ||
		(meta_fmt == MEM_FMT_YUV422_Y_UV_10B) ||
		(meta_fmt == MEM_FMT_YUV422_Y_VU_10B) ||
		(meta_fmt == MEM_FMT_YUV422_Y_UV_8B) ||
		(meta_fmt == MEM_FMT_YUV422_Y_VU_8B) ||
		(meta_fmt == MEM_FMT_YUV422_Y_UV_8B_CE) ||
		(meta_fmt == MEM_FMT_YUV422_Y_VU_8B_CE) ||
		(meta_fmt == MEM_FMT_YUV422_Y_UV_8B_6B_CE) ||
		(meta_fmt == MEM_FMT_YUV422_Y_UV_6B_CE) ||
		(meta_fmt == MEM_FMT_YUV422_Y_VU_6B_CE) ||
		(meta_fmt == MEM_FMT_YUV422_YVYU_8B))
		return TRUE;
	else
		return FALSE;
}

static bool _is_yuv_420(u32 meta_fmt)
{
	if ((meta_fmt == MEM_FMT_YUV420_Y_UV_10B) ||
		(meta_fmt == MEM_FMT_YUV420_Y_VU_10B) ||
		(meta_fmt == MEM_FMT_YUV420_Y_UV_8B) ||
		(meta_fmt == MEM_FMT_YUV420_Y_VU_8B) ||
		(meta_fmt == MEM_FMT_YUV420_Y_UV_8B_CE) ||
		(meta_fmt == MEM_FMT_YUV420_Y_VU_8B_CE) ||
		(meta_fmt == MEM_FMT_YUV420_Y_UV_8B_6B_CE) ||
		(meta_fmt == MEM_FMT_YUV420_Y_UV_6B_CE) ||
		(meta_fmt == MEM_FMT_YUV420_Y_VU_6B_CE))
		return TRUE;
	else
		return FALSE;
}

static int _get_fmt_info(u32 pixfmt_v4l2, struct format_info *fmt_info)
{
	if (WARN_ON(!fmt_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fmt_info=0x%x\n", fmt_info);
		return -EINVAL;
	}

	switch (pixfmt_v4l2) {
	/* ------------------ RGB format ------------------ */
	/* 1 plane */
	case V4L2_PIX_FMT_RGB565:
		fmt_info->meta_fmt = MEM_FMT_RGB565;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_RGB888:
		fmt_info->meta_fmt = MEM_FMT_RGB888;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_RGB101010:
		fmt_info->meta_fmt = MEM_FMT_RGB101010;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_RGB121212:
		fmt_info->meta_fmt = MEM_FMT_RGB121212;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_ARGB8888:
		fmt_info->meta_fmt = MEM_FMT_ARGB8888;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_ABGR8888:
		fmt_info->meta_fmt = MEM_FMT_ABGR8888;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_RGBA8888:
		fmt_info->meta_fmt = MEM_FMT_RGBA8888;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_BGRA8888:
		fmt_info->meta_fmt = MEM_FMT_BGRA8888;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_ARGB2101010:
		fmt_info->meta_fmt = MEM_FMT_ARGB2101010;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_ABGR2101010:
		fmt_info->meta_fmt = MEM_FMT_ABGR2101010;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_RGBA1010102:
		fmt_info->meta_fmt = MEM_FMT_RGBA1010102;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	case V4L2_PIX_FMT_BGRA1010102:
		fmt_info->meta_fmt = MEM_FMT_BGRA1010102;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	/* 3 plane */
	case V4L2_PIX_FMT_RGB_8_8_8_CE:
		fmt_info->meta_fmt = MEM_FMT_RGB_8_8_8_CE;
		fmt_info->plane_num = 3;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 0;
		break;
	/* ------------------ YUV format ------------------ */
	/* 1 plane */
	case V4L2_PIX_FMT_YUV444_VYU_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV444_VYU_10B;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV444_VYU_8B:
		fmt_info->meta_fmt = MEM_FMT_YUV444_VYU_8B;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YVYU:
		fmt_info->meta_fmt = MEM_FMT_YUV422_YVYU_8B;
		fmt_info->plane_num = 1;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	/* 2 plane */
	case V4L2_PIX_FMT_YUV422_Y_UV_12B:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_UV_12B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_UV_10B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_10B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_NV16:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_UV_8B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_NV61:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_8B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_8B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_UV_8B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_8B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_8B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_8B_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_UV_8B_6B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_UV_6B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_6B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_UV_10B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_10B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_NV12:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_UV_8B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_NV21:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_8B;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_8B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_UV_8B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_8B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_8B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_8B_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_UV_8B_6B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_UV_6B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_6B_CE;
		fmt_info->plane_num = 2;
		fmt_info->buffer_num = 1;
		fmt_info->yuv_mode = 1;
		break;
	/* TODO: multi buffer */
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown fmt = %d!!!\n", pixfmt_v4l2);
		return -EINVAL;

	}

	fmt_info->v4l2_fmt = pixfmt_v4l2;

	return 0;
}

static int _decide_duplicate_out(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf,
	struct meta_srccap_frame_info meta_srccap,
	struct m_pq_display_idr_duplicate_out *duplicate_out)
{
	struct mtk_pq_platform_device *pq_pdev = NULL;
	struct pq_display_idr_info *idr_info = NULL;
	bool same_frame = FALSE, same_field = FALSE;

	if (WARN_ON(!pq_dev) || WARN_ON(!pq_buf)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x, pq_buf=0x%x\n",
			pq_dev, pq_buf);
		return -ENXIO;
	}

	if (duplicate_out == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "duplicate_out is NULL!\n");
		return -EINVAL;
	}

	pq_pdev = dev_get_drvdata(pq_dev->dev);
	idr_info = &pq_dev->display_info.idr;

	if ((idr_info->input_mode == MTK_PQ_INPUT_MODE_SW) ||
		(idr_info->input_mode == MTK_PQ_INPUT_MODE_BYPASS))
		duplicate_out->user_config = TRUE;
	else
		duplicate_out->user_config = FALSE;

	if (!idr_info->first_frame) {
		if (idr_info->input_mode == MTK_PQ_INPUT_MODE_SW) {
			if (pq_pdev->pqcaps.u32Idr_SwMode_Support)
				same_frame =
					(pq_buf->vb.vb2_buf.planes[0].m.fd == idr_info->frame_fd);
			else
				same_frame = (meta_srccap.w_index == idr_info->index);

			if (meta_srccap.interlaced)
				same_field = (meta_srccap.field == idr_info->field);
			else
				same_field = FALSE;

			if (same_frame || same_field)
				duplicate_out->duplicate = TRUE;
			else
				duplicate_out->duplicate = FALSE;
		}
	} else
		idr_info->first_frame = FALSE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"idr duplicate info: fd:(%d,%d) w_index:(%u,%u) field:(%u,%u) duplicate(%d)\n",
		pq_buf->vb.vb2_buf.planes[0].m.fd, idr_info->frame_fd,
		meta_srccap.w_index, idr_info->index,
		meta_srccap.field, idr_info->field,
		duplicate_out->duplicate);

	duplicate_out->duplicate = FALSE;

	return 0;
}

//-----------------------------------------------------------------------------
//  Global Functions
//-----------------------------------------------------------------------------
int mtk_pq_display_idr_queue_setup(struct vb2_queue *vq,
	unsigned int *num_buffers, unsigned int *num_planes,
	unsigned int sizes[], struct device *alloc_devs[])
{
	struct mtk_pq_ctx *pq_ctx = vb2_get_drv_priv(vq);
	struct mtk_pq_device *pq_dev = pq_ctx->pq_dev;
	struct pq_display_idr_info *idr = &pq_dev->display_info.idr;
	int i = 0;

	/* video buffers + meta buffer */
	*num_planes = idr->buffer_num + 1;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"[idr] num_planes=%d\n", *num_planes);

	for (i = 0; i < idr->plane_num; ++i) {
		sizes[i] = 1;	// TODO
		alloc_devs[i] = pq_dev->dev;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"out size[%d]=%d, alloc_devs[%d]=%d\n", i, sizes[i], i, alloc_devs[i]);
	}

	/* give meta fd min size */
	sizes[*num_planes - 1] = 1;
	alloc_devs[*num_planes - 1] = pq_dev->dev;

	return 0;
}

int mtk_pq_display_idr_queue_setup_info(struct mtk_pq_device *pq_dev,
	void *bufferctrl)
{
	struct v4l2_pq_g_buffer_info bufferInfo;
	struct pq_display_idr_info *idr = &pq_dev->display_info.idr;
	int i = 0;

	memset(&bufferInfo, 0, sizeof(struct v4l2_pq_g_buffer_info));

	/* video buffers + meta buffer */
	bufferInfo.plane_num = idr->buffer_num + 1;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"[idr] plane_num=%d\n", bufferInfo.plane_num);

	for (i = 0; i < idr->plane_num; ++i) {
		bufferInfo.size[i] = 1;	// TODO

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"out size[%d]=%d\n", i, bufferInfo.size[i]);
	}

	/* give meta fd min size */
	bufferInfo.size[bufferInfo.plane_num - 1] = 1;

	/* update buffer_num */
	bufferInfo.buffer_num = pq_dev->pix_format_info.v4l2_fmt_pix_idr_mp.num_planes;

	/* update attri */
	if (idr->buffer_num == 1)
		bufferInfo.buf_attri = V4L2_PQ_BUF_ATTRI_CONTINUOUS;
	else
		bufferInfo.buf_attri = V4L2_PQ_BUF_ATTRI_DISCRETE;

	/* update bufferctrl */
	memcpy(bufferctrl, &bufferInfo, sizeof(struct v4l2_pq_g_buffer_info));

	return 0;
}


int mtk_pq_display_idr_set_pix_format(
	struct v4l2_format *fmt,
	struct pq_display_idr_info *idr_info)
{
	if (WARN_ON(!fmt) || WARN_ON(!idr_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fmt=0x%x, idr_info=0x%x\n", fmt, idr_info);
		return -EINVAL;
	}

	/* IDR handle, save to share mem */
	idr_info->width = fmt->fmt.pix.width;
	idr_info->height = fmt->fmt.pix.height;
	idr_info->mem_fmt = fmt->fmt.pix.pixelformat;
	idr_info->plane_num = 1;

	return 0;
}

int mtk_pq_display_idr_set_pix_format_mplane(
	struct v4l2_format *fmt,
	struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	struct format_info fmt_info;

	if (WARN_ON(!fmt) || WARN_ON(!pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fmt=0x%x, idr_info=0x%x\n", fmt, pq_dev);
		return -EINVAL;
	}

	memset(&fmt_info, 0, sizeof(fmt_info));

	/* IDR handle, save to share mem */
	pq_dev->display_info.idr.width = fmt->fmt.pix_mp.width;
	pq_dev->display_info.idr.height = fmt->fmt.pix_mp.height;
	pq_dev->display_info.idr.mem_fmt = fmt->fmt.pix_mp.pixelformat;

	/* get fmt info */
	ret = _get_fmt_info(
		fmt->fmt.pix_mp.pixelformat,
		&fmt_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_get_fmt_info fail, ret = %d\n", ret);
		return ret;
	}

	pq_dev->display_info.idr.plane_num = fmt_info.plane_num;
	pq_dev->display_info.idr.buffer_num = fmt_info.buffer_num;

	/* update fmt to user */
	fmt->fmt.pix_mp.num_planes = pq_dev->display_info.idr.buffer_num;

	/* save fmt  */
	memcpy(&pq_dev->pix_format_info.v4l2_fmt_pix_idr_mp,
		&fmt->fmt.pix_mp, sizeof(struct v4l2_pix_format_mplane));

	return 0;
}

int mtk_pq_display_idr_set_pix_format_mplane_info(
	struct mtk_pq_device *pq_dev,
	void *bufferctrl)
{
	struct v4l2_pq_s_buffer_info bufferInfo;

	memset(&bufferInfo, 0, sizeof(struct v4l2_pq_s_buffer_info));
	memcpy(&bufferInfo, bufferctrl, sizeof(struct v4l2_pq_s_buffer_info));

	int ret = 0;
	struct format_info fmt_info;

	if (WARN_ON(!bufferctrl) || WARN_ON(!pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, bufferctrl=0x%x, pq_dev=0x%x\n", bufferctrl, pq_dev);
		return -EINVAL;
	}

	memset(&fmt_info, 0, sizeof(fmt_info));

	/* IDR handle, save to share mem */
	pq_dev->pix_format_info.v4l2_fmt_pix_idr_mp.width = bufferInfo.width;
	pq_dev->pix_format_info.v4l2_fmt_pix_idr_mp.height = bufferInfo.height;

	/* get fmt info */
	ret = _get_fmt_info(
		pq_dev->pix_format_info.v4l2_fmt_pix_idr_mp.pixelformat,
		&fmt_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_get_fmt_info fail, ret = %d\n", ret);
		return ret;
	}

	pq_dev->display_info.idr.width = bufferInfo.width;
	pq_dev->display_info.idr.height = bufferInfo.height;
	pq_dev->display_info.idr.mem_fmt
		= pq_dev->pix_format_info.v4l2_fmt_pix_idr_mp.pixelformat;
	pq_dev->display_info.idr.plane_num = fmt_info.plane_num;
	pq_dev->display_info.idr.buffer_num = fmt_info.buffer_num;

	/* update fmt to user */
	pq_dev->pix_format_info.v4l2_fmt_pix_idr_mp.num_planes
		= pq_dev->display_info.idr.buffer_num;

	return 0;
}

int mtk_pq_display_idr_streamon(struct pq_display_idr_info *idr_info)
{
	if (WARN_ON(!idr_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, idr_info=0x%x\n", idr_info);
		return -EINVAL;
	}

	/* define default crop size if not set */
	if ((idr_info->crop.left == 0)
		&& (idr_info->crop.top == 0)
		&& (idr_info->crop.width == 0)
		&& (idr_info->crop.height == 0)) {
		idr_info->crop.width = idr_info->width;
		idr_info->crop.height = idr_info->height;
	}

	/* update initial value*/
	idr_info->first_frame = TRUE;

	return 0;
}

int mtk_pq_display_idr_streamoff(struct pq_display_idr_info *idr_info)
{
	if (WARN_ON(!idr_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, idr_info=0x%x\n", idr_info);
		return -EINVAL;
	}

	memset(idr_info->fbuf, 0, sizeof(idr_info->fbuf));
	memset(&idr_info->crop, 0, sizeof(idr_info->crop));
	idr_info->mem_fmt = 0;
	idr_info->width = 0;
	idr_info->height = 0;
	idr_info->index = 0;
	idr_info->plane_num = 0;
	idr_info->buffer_num = 0;

	return 0;
}

int mtk_pq_display_idr_s_crop(
	struct pq_display_idr_info *idr_info,
	const struct v4l2_selection *s)
{
	if (WARN_ON(!idr_info) || WARN_ON(!s)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, idr_info=0x%x, selection=0x%x\n", idr_info, s);
		return -EINVAL;
	}

	idr_info->crop.left = s->r.left;
	idr_info->crop.top = s->r.top;
	idr_info->crop.width = s->r.width;
	idr_info->crop.height = s->r.height;

	return 0;
}

int mtk_pq_display_idr_g_crop(
	struct pq_display_idr_info *idr_info,
	struct v4l2_selection *s)
{
	if (WARN_ON(!idr_info) || WARN_ON(!s)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, idr_info=0x%x, selection=0x%x\n", idr_info, s);
		return -EINVAL;
	}

	s->r.left = idr_info->crop.left;
	s->r.top = idr_info->crop.top;
	s->r.width = idr_info->crop.width;
	s->r.height = idr_info->crop.height;

	return 0;
}

int mtk_pq_display_idr_qbuf(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf)
{
	int ret = 0;
	struct mtk_pq_platform_device *pq_pdev = NULL;
	struct meta_srccap_frame_info meta_srccap;
	struct m_pq_display_idr_ctrl meta_idr;
	struct pq_buffer meta_buf_info;
	struct dma_buf *meta_db = NULL;
	struct format_info fmt_info;
	struct pq_display_idr_info *idr_info = NULL;
	struct meta_buffer meta_buf;
	u16 win_id = 0;
	s32 crop_left = 0, crop_top = 0;
	u32 crop_width = 0, crop_height = 0;

	if (WARN_ON(!pq_dev) || WARN_ON(!pq_buf)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x, pq_buf=0x%x\n",
			pq_dev, pq_buf);
		return -EINVAL;
	}

	pq_pdev = dev_get_drvdata(pq_dev->dev);
	idr_info = &pq_dev->display_info.idr;
	win_id = pq_dev->dev_indx;

	memset(&meta_srccap, 0, sizeof(meta_srccap));
	memset(&meta_idr, 0, sizeof(meta_idr));
	memset(&fmt_info, 0, sizeof(fmt_info));
	memset(&meta_buf, 0, sizeof(struct meta_buffer));

	/* meta buffer handle */
	meta_buf.paddr = pq_buf->meta_buf.va;
	meta_buf.size = pq_buf->meta_buf.size;

	/* read meta from srccap */
	memset(&meta_srccap, 0, sizeof(struct meta_srccap_frame_info));
	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_SRCCAP_FRAME_INFO, &meta_srccap);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_SRCCAP_FRAME_INFO Failed, ret = %d\n", ret);
		return ret;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"mtk-pq read EN_PQ_METATAG_SRCCAP_FRAME_INFO success\n");

	/* mapping pixfmt v4l2 -> meta & get format info */
	ret = _get_fmt_info(idr_info->mem_fmt, &fmt_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_get_fmt_info fail, ret=%d\n", ret);
		return ret;
	}

	/* get meta info from share mem */
	memcpy(meta_idr.fbuf, idr_info->fbuf,
		sizeof(meta_idr.fbuf));
	meta_idr.mem_fmt = fmt_info.meta_fmt;
	meta_idr.yuv_mode = fmt_info.yuv_mode;
	meta_idr.width = idr_info->width;
	meta_idr.height = idr_info->height;
	meta_idr.index = meta_srccap.w_index;

	crop_left = idr_info->crop.left;
	crop_top = idr_info->crop.top;

	if ((idr_info->crop.width > meta_srccap.scaled_width) || (idr_info->crop.width == 0))
		crop_width = meta_srccap.scaled_width;
	else
		crop_width = idr_info->crop.width;

	if ((idr_info->crop.height > meta_srccap.scaled_height) || (idr_info->crop.height == 0))
		crop_height = meta_srccap.scaled_height;
	else
		crop_height = idr_info->crop.height;

	crop_height = PQ_ALIGN(crop_height, PQ_Y_ALIGN);

	if (meta_srccap.interlaced == TRUE) {
		crop_top /= PQ_DIVIDE_BASE_2;
		crop_height /= PQ_DIVIDE_BASE_2;
	}

	// x needs to align 2 for yuv 422/420
	if (_is_yuv_422(fmt_info.meta_fmt) || _is_yuv_420(fmt_info.meta_fmt))
		meta_idr.crop.left = PQ_ALIGN(crop_left, PQ_X_ALIGN);
	else
		meta_idr.crop.left = crop_left;

	// y needs to align 2 for yuv 420
	if (_is_yuv_420(fmt_info.meta_fmt))
		meta_idr.crop.top = PQ_ALIGN(crop_top, PQ_Y_ALIGN);
	else
		meta_idr.crop.top = crop_top;

	if (_is_yuv_422(fmt_info.meta_fmt) || _is_yuv_420(fmt_info.meta_fmt))
		crop_width = PQ_ALIGN(crop_width, PQ_X_ALIGN);

	meta_idr.crop.width = PQ_ALIGN(crop_width, pq_pdev->display_dev.pixel_align);

	//Temp for align pq
	//if (_is_yuv_420(fmt_info.meta_fmt))
		meta_idr.crop.height = PQ_ALIGN(crop_height, PQ_Y_ALIGN);
	//else
	//	meta_idr.crop.height = idr_info->crop.height;

	meta_idr.path = meta_srccap.path;
	meta_idr.bypass_ipdma = meta_srccap.bypass_ipdma;
	meta_idr.v_flip = false;	//TODO
#if IS_ENABLED(CONFIG_OPTEE)
	meta_idr.aid = pq_dev->display_info.secure_info.aid;
#endif
	meta_idr.buf_ctrl_mode = pq_dev->display_info.idr.input_mode;
	meta_idr.frame_pitch = meta_srccap.frame_pitch;
	meta_idr.line_offset = meta_srccap.hoffset;
	meta_idr.sw_mode_supported = pq_pdev->pqcaps.u32Idr_SwMode_Support;

	ret = _decide_duplicate_out(pq_dev, pq_buf, meta_srccap, &meta_idr.duplicate_out);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_get_fmt_info fail, ret=%d\n", ret);
		return ret;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"%s %s0x%x, %s%d, %s%d, %s%d, %s[%d,%d,%d,%d], %s%d, %s%d, %s%d, %s%u, %s%u, %s%u, %s%u, %s%d, %s%d\n",
		"idr w meta:",
		"fmt = ", meta_idr.mem_fmt,
		"w = ", meta_idr.width,
		"h = ", meta_idr.height,
		"index = ", meta_idr.index,
		"crop = ", meta_idr.crop.left, meta_idr.crop.top,
		meta_idr.crop.width, meta_idr.crop.height,
		"path = ", meta_idr.path,
		"bypass_ipdma = ", meta_idr.bypass_ipdma,
		"vflip = ", meta_idr.v_flip,
		"aid = ", meta_idr.aid,
		"buf_ctrl_mode = ", meta_idr.buf_ctrl_mode,
		"frame_pitch = ", meta_idr.frame_pitch,
		"line_offset = ", meta_idr.line_offset,
		"duplicate_out.user_config = ", meta_idr.duplicate_out.user_config,
		"duplicate_out.duplicate = ", meta_idr.duplicate_out.duplicate);

	/* set meta buf info */
	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_DISP_IDR_CTRL, &meta_idr);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"EN_PQ_METATAG_DISP_IDR_CTRL fail, ret=%d\n", ret);
		return ret;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "mtk-pq write EN_PQ_METATAG_DISP_IDR_CTRL success\n");

	/*store shm from meta*/
	/*for video calc use*/
	pq_dev->display_info.idr.inputHTT = meta_srccap.h_total;
	pq_dev->display_info.idr.inputVTT = meta_srccap.v_total;

	idr_info->index = meta_srccap.w_index;
	idr_info->frame_fd = pq_buf->vb.vb2_buf.planes[0].m.fd;
	idr_info->field = meta_srccap.field;

	return 0;
}

void mtk_pq_display_idr_qbuf_done(u8 win_id, __u64 meta_db_ptr)
{
	struct dma_buf *meta_db = (struct dma_buf *)meta_db_ptr;
	struct m_pq_display_idr_ctrl meta_idr;
	struct vb2_buffer *vb = NULL;
	struct v4l2_event ev;
	struct vb2_v4l2_buffer *vbuf = NULL;
	struct mtk_pq_ctx *ctx = NULL;
	struct mtk_pq_device *pq_dev = NULL;
	int ret = 0;

	memset(&ev, 0, sizeof(struct v4l2_event));

	/* read meta from dma_buf */
	ret = mtk_pq_common_read_metadata_db(
		meta_db, EN_PQ_METATAG_DISP_IDR_CTRL, &meta_idr);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_read_metadata_db fail, ret=%d\n", ret);
	}

	/* get vb from meta */
	vb = (struct vb2_buffer *)meta_idr.vb;

	/* delete meta tag */
	ret = mtk_pq_common_delete_metadata_db(meta_db, EN_PQ_METATAG_DISP_IDR_CTRL);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_delete_metadata_db fail, ret=%d\n", ret);
	}

}
