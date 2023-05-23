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
#include <linux/videodev2.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include "mtk_pq.h"
#include "mtk_pq_xc.h"
#include "mtk_pq_enhance.h"
#include "mtk_pq_common.h"
#include "mtk_pq_b2r.h"
#include "mtk_pq_display_manager.h"

#include "apiXC.h"
#include "drvPQ.h"
#include "mtk_iommu_dtv_api.h"
#include "metadata_utility.h"

static int _mtk_pq_common_map_fd(int fd, void **va, u64 *size)
{
	int ret = 0;
	struct dma_buf *db = NULL;

	if ((size == NULL) || (fd == 0)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, fd=(%d), size is NULL?(%s)\n",
			fd, (size == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	if (*va != NULL) {
		free(*va);
		*va = NULL;
	}

	db = dma_buf_get(fd);
	if (db == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_get fail\n");
		return -EPERM;
	}

	*size = db->size;

	ret = dma_buf_begin_cpu_access(db, DMA_BIDIRECTIONAL);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_begin_cpu_access fail\n");
		dma_buf_put(db);
		return -EPERM;
	}

	*va = dma_buf_vmap(db);

	if (!*va || (*va == -1)) {
		dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_vmap fail\n");
		dma_buf_put(db);
		return -EPERM;
	}

	dma_buf_put(db);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "va=0x%llx, size=%llu\n", *va, *size);
	return ret;
}

static int _mtk_pq_common_unmap_fd(int fd, void *va)
{
	struct dma_buf *db = NULL;

	if ((va == NULL) || (fd == 0)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, fd=(%d), va is NULL?(%s)\n",
			fd, (va == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	db = dma_buf_get(fd);
	if (db == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_get fail\n");
		return -1;
	}

	dma_buf_vunmap(db, va);
	dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
	dma_buf_put(db);
	return 0;
}

static int _mtk_pq_common_get_metaheader(
	enum mtk_pq_common_metatag meta_tag,
	struct meta_header *meta_header)
{
	int ret = 0;

	if (meta_header == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, meta_header == NULL\n");
		return -EPERM;
	}

	switch (meta_tag) {
	case EN_PQ_METATAG_VDEC_SVP_INFO:
		meta_header->size = sizeof(struct vdec_dd_svp_info);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_VDEC_DD_SVP_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_SRCCAP_SVP_INFO:
		meta_header->size = sizeof(struct meta_srccap_svp_info);
		meta_header->ver = META_SRCCAP_SVP_INFO_VERSION;
		strncpy(meta_header->tag, META_SRCCAP_SVP_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQ_SVP_INFO:
		meta_header->size = sizeof(struct meta_pq_disp_svp);
		meta_header->ver = META_PQ_DISP_SVP_VERSION;
		strncpy(meta_header->tag, PQ_DISP_SVP_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_SH_FRM_INFO:
		meta_header->size = sizeof(struct mtk_pq_frame_info);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_PQ_SH_FRM_INFO_TAG, TAG_LENGTH);
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid metadata tag:(%d)\n", meta_tag);
		ret = -EPERM;
		break;
	}
	return ret;
}

int mtk_pq_common_read_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (meta_content == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, meta_content=NULL\n");
		return -EPERM;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_pq_common_map_fd(meta_fd, &va, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "trans fd to va fail\n");
		ret = -EPERM;
		goto out;
	}

	ret = _mtk_pq_common_get_metaheader(meta_tag, &header);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get meta header fail\n");
		ret = -EPERM;
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = query_metadata_header(&buffer, &header);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "query metadata header fail\n");
		ret = -EPERM;
		goto out;
	}

	meta_ret = query_metadata_content(&buffer, &header, meta_content);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "query metadata content fail\n");
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_pq_common_unmap_fd(meta_fd, va);
	return ret;
}

int mtk_pq_common_write_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (meta_content == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, meta_content=NULL\n");
		return -EPERM;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_pq_common_map_fd(meta_fd, &va, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "trans fd to va fail\n");
		ret = -EPERM;
		goto out;
	}

	ret = _mtk_pq_common_get_metaheader(meta_tag, &header);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get meta header fail\n");
		ret = -EPERM;
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = attach_metadata(&buffer, header, meta_content);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "attach metadata fail\n");
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_pq_common_unmap_fd(meta_fd, va);
	return ret;
}

int mtk_pq_common_delete_metadata(int meta_fd,
	enum mtk_pq_common_metatag meta_tag)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_pq_common_map_fd(meta_fd, &va, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "trans fd to va fail\n");
		ret = -EPERM;
		goto out;
	}

	ret = _mtk_pq_common_get_metaheader(meta_tag, &header);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get meta header fail\n");
		ret = -EPERM;
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = delete_metadata(&buffer, &header);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "delete metadata fail\n");
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_pq_common_unmap_fd(meta_fd, va);
	return ret;
}

int mtk_pq_common_set_input(struct mtk_pq_device *pq_dev,
	unsigned int input_index)
{
	int ret = 0;
	__u32 input_type;

	if (!pq_dev)
		return -ENOMEM;
	pq_dev->common_info.input_source = input_index;

	input_type = mtk_display_transfer_input_source(input_index);

	if (mtk_display_set_inputsource((__u32)pq_dev->dev_indx,
	    input_index)) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: display set input failed!\n", __func__);
		goto ERR;
	}
	if (mtk_pq_enhance_set_input_source(pq_dev, input_index)) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s:pq enhance set input failed!\n", __func__);
		goto ERR;
	}

ERR:
	return ret;
}

int mtk_pq_common_get_input(struct mtk_pq_device *pq_dev,
	unsigned int *p_input_index)
{
	if ((!pq_dev) || (!p_input_index))
		return -ENOMEM;
	*p_input_index = pq_dev->common_info.input_source;

	return 0;
}

int mtk_pq_common_set_fmt_cap_mplane(
	struct mtk_pq_device *pq_dev,
	struct v4l2_format *fmt)
{
	int ret = 0;

	if ((!pq_dev) || (!fmt))
		return -ENOMEM;

	memcpy(&pq_dev->common_info.format_cap, fmt,
		sizeof(struct v4l2_format));

	if (mtk_xc_set_pix_format_out_mplane(fmt, &pq_dev->xc_info)) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "%s: xc set pix fmt failed!\n", __func__);
		goto ERR;
	}

	if (mtk_pq_enhance_set_pix_format_out_mplane(pq_dev, &fmt->fmt.pix_mp)) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: hdr set pix fmt failed!\n", __func__);
		goto ERR;
	}

ERR:
	return ret;
}

int mtk_pq_common_set_fmt_out_mplane(
	struct mtk_pq_device *pq_dev,
	struct v4l2_format *fmt)
{
	int ret = 0;
	if ((!pq_dev) || (!fmt))
		return -ENOMEM;

	memcpy(&pq_dev->common_info.format_out, fmt,
		sizeof(struct v4l2_format));

	if (mtk_pq_enhance_set_pix_format_in_mplane(pq_dev, &fmt->fmt.pix_mp)) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: hdr set pix fmt failed!\n", __func__);
		goto ERR;
	}
	if (mtk_pq_b2r_set_pix_format_in_mplane(fmt, &pq_dev->b2r_info) < 0) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: b2r set pix fmt failed!\n", __func__);
		goto ERR;
	}
	ret = mtk_xc_set_pix_format_in_mplane(fmt, &pq_dev->xc_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: xc set pix fmt mp failed!\n", __func__);
		goto ERR;
	}

ERR:
	return ret;
}

int mtk_pq_common_get_fmt_cap_mplane(
	struct mtk_pq_device *pq_dev,
	struct v4l2_format *fmt)
{
	if ((!pq_dev) || (!fmt))
		return -ENOMEM;

	memcpy(&(fmt->fmt.pix_mp), &pq_dev->common_info.format_cap.fmt.pix_mp,
		sizeof(struct v4l2_pix_format_mplane));

	return 0;
}

int mtk_pq_common_get_fmt_out_mplane(
	struct mtk_pq_device *pq_dev,
	struct v4l2_format *fmt)
{
	memcpy(&(fmt->fmt.pix_mp), &pq_dev->common_info.format_out.fmt.pix_mp,
		sizeof(struct v4l2_pix_format_mplane));

	return 0;
}

int mtk_pq_common_stream_off(struct mtk_pq_device *pq_dev)
{
	return 0;
}

/* standard FLIP will return in v4l2 framework, _mtk_pq_common_get_hflip/vflip
 * not use so far. If need these 2 api,
 * set .flag = V4L2_CTRL_FLAG_VOLATILE |
 *V4L2_CTRL_FLAG_EXECUTE_ON_WRITE by customer
 */
static int _mtk_pq_common_get_hflip(
	struct mtk_pq_device *pq_dev, bool *enable)
{
	int ret = 0;
	__u8 input_source = pq_dev->common_info.input_source;
	__u8 win = pq_dev->dev_indx;
	enum v4l2_fb_level fb_level;

	if (mtk_xc_get_fb_mode(&fb_level)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "run out of memory!\n");
		return -ENOMEM;
	}

	if (input_source == PQ_INPUT_SOURCE_DTV ||
		input_source == PQ_INPUT_SOURCE_STORAGE) {
		/* b2r mirror*/
		if (mtk_pq_b2r_get_flip(pq_dev->dev_indx, enable,
				&pq_dev->b2r_info, B2R_HFLIP) < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_b2r_get_flip failed\n");
			return -EPERM;
		}
	} else {
		/* xc mirror*/
		ret = mtk_xc_get_hflip(win, enable);
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON,
		"get hflip = %d, input source = %d\n", *enable, input_source);

	return ret;
}

static int _mtk_pq_common_get_vflip(
	struct mtk_pq_device *pq_dev, bool *enable)
{
	int ret = 0;
	__u8 input_source = pq_dev->common_info.input_source;
	__u8 win = pq_dev->dev_indx;
	enum v4l2_fb_level fb_level;

	if (mtk_xc_get_fb_mode(&fb_level)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "run out of memory!\n");
		return -ENOMEM;
	}

	if (input_source == PQ_INPUT_SOURCE_DTV ||
		input_source == PQ_INPUT_SOURCE_STORAGE) {
		/* b2r mirror*/
		if (mtk_pq_b2r_get_flip(pq_dev->dev_indx, enable,
			&pq_dev->b2r_info, B2R_VFLIP) < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_b2r_get_flip failed\n");
			return -EPERM;
		}
	} else {
		// xc mirror
		ret = mtk_xc_get_vflip(win, enable);
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON,
		"get vflip = %d, input source = %d\n", *enable, input_source);

	return ret;
}

static int _mtk_pq_common_get_delay_time(
	struct mtk_pq_device *pq_dev, void *ctrl)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;
	struct v4l2_pq_delaytime_info delaytime_info;
	__u16 b2r_delay = 0;
	__u16 display_delay = 0;
	__u16 pq_delay = 0;

	memset(&delaytime_info, 0, sizeof(struct v4l2_pq_delaytime_info));
	if (mtk_pq_b2r_get_delaytime(win, &b2r_delay, &pq_dev->b2r_info)) {
		ret = -EPERM;
		goto ERR;
	}

	delaytime_info.delaytime = b2r_delay + display_delay + pq_delay;
	memcpy(ctrl, &delaytime_info, sizeof(struct v4l2_pq_delaytime_info));

ERR:
	return ret;
}

static int _mtk_pq_common_set_input_ext_info(
	struct mtk_pq_device *pq_dev, void *ctrl)
{
	return 0;
}

static int _mtk_pq_common_set_output_ext_info(
	struct mtk_pq_device *pq_dev, void *ctrl)
{
	return 0;
}

static int _mtk_pq_common_set_hflip(
	struct mtk_pq_device *pq_dev, bool enable)
{
	int ret = 0;
	__u8 input_source = pq_dev->common_info.input_source;
	__u8 win = pq_dev->dev_indx;
	enum v4l2_fb_level fb_level;

	if (mtk_xc_get_fb_mode(&fb_level)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "run out of memory!\n");
		return -ENOMEM;
	}

	if (input_source == PQ_INPUT_SOURCE_DTV ||
		input_source == PQ_INPUT_SOURCE_STORAGE) {
		/* b2r mirror*/
		if (mtk_pq_b2r_set_flip(pq_dev->dev_indx, enable,
			&pq_dev->b2r_info, B2R_HFLIP) < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_b2r_set_flip failed\n");
			return -EPERM;
		}
	} else {
		/* xc mirror*/
		ret = mtk_xc_set_hflip(win, enable);
	}

	return ret;
}

static int _mtk_pq_common_set_vflip(
	struct mtk_pq_device *pq_dev, bool enable)
{
	int ret = 0;
	__u8 input_source = pq_dev->common_info.input_source;
	__u8 win = pq_dev->dev_indx;
	enum v4l2_fb_level fb_level;

	if (mtk_xc_get_fb_mode(&fb_level)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "run out of memory!\n");
		return -ENOMEM;
	}

	if (input_source == PQ_INPUT_SOURCE_DTV ||
	input_source == PQ_INPUT_SOURCE_STORAGE) {
		/* b2r mirror*/
		if (mtk_pq_b2r_set_flip(pq_dev->dev_indx, enable,
			&pq_dev->b2r_info, B2R_VFLIP) < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_b2r_set_flip failed\n");
			return -EPERM;
		}
	} else {
		/* xc mirror*/
		ret = mtk_xc_set_vflip(win, enable);
	}

	return ret;
}

static int _mtk_pq_common_set_forcep(
	struct mtk_pq_device *pq_dev, bool enable)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;

	/* set b2r*/
	mtk_pq_b2r_set_forcep(win, enable, &pq_dev->b2r_info);
	mtk_display_set_forcep((__u32)win, enable);

	return ret;
}

static int _mtk_pq_common_set_delay_time(struct mtk_pq_device *pq_dev,
	void *ctrl)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;
	struct v4l2_pq_delaytime_info delaytime_info;

	memset(&delaytime_info, 0, sizeof(struct v4l2_pq_delaytime_info));
	memcpy(&delaytime_info, ctrl, sizeof(struct v4l2_pq_delaytime_info));

	if (mtk_pq_b2r_set_delaytime(win, delaytime_info.fps,
		&pq_dev->b2r_info)) {
		ret = -EPERM;
		goto ERR;
	}

ERR:
	return ret;
}

static int _mtk_pq_common_set_input_timing(struct mtk_pq_device *pq_dev,
	void *ctrl)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;
	struct v4l2_timing input_timing_info;

	memcpy(&pq_dev->common_info.timing_in, ctrl,
		sizeof(struct v4l2_timing));

	mtk_b2r_set_ext_timing(win, ctrl, &pq_dev->b2r_info);

	return ret;
}

static int _mtk_pq_common_get_input_timing(struct mtk_pq_device *pq_dev, void *ctrl)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;
	struct v4l2_timing input_timing_info;

	memcpy(ctrl, &pq_dev->common_info.timing_in, sizeof(struct v4l2_timing));

	return ret;
}

static int _mtk_pq_common_set_gen_pattern(struct mtk_pq_device *pq_dev, void *ctrl)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;
	struct v4l2_pq_gen_pattern gen_pat_info;

	memset(&gen_pat_info, 0, sizeof(struct v4l2_pq_gen_pattern));
	memcpy(&gen_pat_info, ctrl, sizeof(struct v4l2_pq_gen_pattern));

	switch (gen_pat_info.pat_type) {
	case V4L2_EXT_PQ_B2R_PAT_FRAMECOLOR:
		mtk_pq_b2r_set_pattern(win, gen_pat_info.pat_type,
			&gen_pat_info.pat_para.b2r_pat_info, &pq_dev->b2r_info);
		break;
	default:
		ret = -EINVAL;
		goto ERR;
	}

ERR:
	return ret;
}

static int _common_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev;
	int ret;

	if (!ctrl)
		return -ENOMEM;
	pq_dev = container_of(ctrl->handler, struct mtk_pq_device,
		common_ctrl_handler);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "%s: ctrl id = %d.\n",
		__func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		ret = _mtk_pq_common_get_hflip(pq_dev, (bool *)&ctrl->val);
		break;
	case V4L2_CID_VFLIP:
		ret = _mtk_pq_common_get_vflip(pq_dev, (bool *)&ctrl->val);
		break;
	case V4L2_CID_DELAY_TIME:
		ret = _mtk_pq_common_get_delay_time(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_INPUT_TIMING:
		ret = _mtk_pq_common_get_input_timing(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_INPUT_EXT_INFO:
	case V4L2_CID_OUTPUT_EXT_INFO:
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _common_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev;
	int ret;

	if (!ctrl)
		return -ENOMEM;
	pq_dev = container_of(ctrl->handler, struct mtk_pq_device,
		common_ctrl_handler);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "%s: ctrl id = %d.\n",
		__func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		ret = _mtk_pq_common_set_hflip(pq_dev, (bool)ctrl->val);
		break;
	case V4L2_CID_VFLIP:
		ret = _mtk_pq_common_set_vflip(pq_dev, (bool)ctrl->val);
		break;
	case V4L2_CID_FORCE_P_MODE:
		ret = _mtk_pq_common_set_forcep(pq_dev, (bool)ctrl->val);
		break;
	case V4L2_CID_DELAY_TIME:
		ret = _mtk_pq_common_set_delay_time(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_INPUT_TIMING:
		ret = _mtk_pq_common_set_input_timing(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_INPUT_EXT_INFO:
		ret = _mtk_pq_common_set_input_ext_info(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_OUTPUT_EXT_INFO:
		ret = _mtk_pq_common_set_output_ext_info(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_GEN_PATTERN:
		ret = _mtk_pq_common_set_gen_pattern(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops common_ctrl_ops = {
	.g_volatile_ctrl = _common_g_ctrl,
	.s_ctrl = _common_s_ctrl,
};

static const struct v4l2_ctrl_config common_ctrl[] = {
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_INPUT_EXT_INFO,
		.name = "input ext info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_input_ext_info)},
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_OUTPUT_EXT_INFO,
		.name = "output ext info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_output_ext_info)},
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_FORCE_P_MODE,
		.name = "force p mode enable",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = 0,
		.max = 1,
		.step = 1,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_DELAY_TIME,
		.name = "delay time",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_delaytime_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_INPUT_TIMING,
		.name = "input timing",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_timing)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_GEN_PATTERN,
		.name = "generate pattern",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_gen_pattern)},
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops common_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops common_sd_internal_ops = {
};


int mtk_pq_register_common_subdev(
	struct v4l2_device *pq_dev,
	struct v4l2_subdev *subdev_common,
	struct v4l2_ctrl_handler *common_ctrl_handler)
{
	int ret = 0;
	__u32 count;
	__u32 ctrl_num = sizeof(common_ctrl)/sizeof(struct v4l2_ctrl_config);

	if ((!pq_dev) || (!subdev_common) || (!common_ctrl_handler))
		return -ENOMEM;

	v4l2_ctrl_handler_init(common_ctrl_handler, ctrl_num);
	for (count = 0; count < ctrl_num; count++)
		v4l2_ctrl_new_custom(common_ctrl_handler,
			&common_ctrl[count], NULL);

	v4l2_ctrl_new_std(common_ctrl_handler, &common_ctrl_ops, V4L2_CID_HFLIP,
		0, 1, 1, 0);
	v4l2_ctrl_new_std(common_ctrl_handler, &common_ctrl_ops, V4L2_CID_VFLIP,
		0, 1, 1, 0);

	ret = common_ctrl_handler->error;
	if (ret) {
		v4l2_err(pq_dev, "failed to create common ctrl handler\n");
		goto exit;
	}
	subdev_common->ctrl_handler = common_ctrl_handler;

	v4l2_subdev_init(subdev_common, &common_sd_ops);
	subdev_common->internal_ops = &common_sd_internal_ops;
	strlcpy(subdev_common->name, "mtk-common", sizeof(subdev_common->name));

	ret = v4l2_device_register_subdev(pq_dev, subdev_common);
	if (ret) {
		v4l2_err(pq_dev, "failed to register common subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(common_ctrl_handler);
	return ret;
}

void mtk_pq_unregister_common_subdev(
	struct v4l2_subdev *subdev_common)
{
	if (subdev_common) {
		v4l2_ctrl_handler_free(subdev_common->ctrl_handler);
		v4l2_device_unregister_subdev(subdev_common);
	}
}

