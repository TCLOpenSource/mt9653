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
#include <linux/timekeeping.h>
#include <linux/genalloc.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include "pqu_msg.h"
#include "ext_command_video_if.h"

#include "mtk_pq.h"
#include "mtk_pq_enhance.h"
#include "mtk_pq_common.h"
#include "mtk_pq_b2r.h"
#include "mtk_pq_display.h"
#include "mtk_pq_pattern.h"
#include "mtk_pq_common_irq.h"
#include "mtk_pq_svp.h"
#include "mtk_pq_buffer.h"

#include "apiXC.h"
#include "drvPQ.h"
#include "mtk_iommu_dtv_api.h"

#include "hwreg_common.h"
#include "hwreg_pq_common_trigger_gen.h"
#include "hwreg_pq_display_pattern.h"
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
#include <linux/mtk-tv-atrace.h>
#endif

#define REG_SIZE	(10)	// TODO: need to set from DD (source: DTS)
#define PQU_TIME_OUT_MS		(10000)
#define ALLOC_ORDER         (10)
#define NODE_ID             (-1)
#define TRIG_SW_HTT			(200)
#define TRIG_VCNT_UPD_MSK_RNG		(2)
#define TRIG_VS_DLY_H			(0x1)
#define TRIG_VS_DLY_V			(0x6)
#define TRIG_DMA_R_DLY_H		(0x1)
#define TRIG_DMA_R_DLY_V		(0x7)
#define TRIG_STR_DLY_V			(0x7)
#define PATTERN_REG_NUM_MAX		(400)// 3 * 127 + 6 + 2 = 389
#define PTN_H_SIZE	(64)
#define PTN_V_SIZE	(32)

static struct gen_pool *pool;
static struct m_pq_common_debug_info g_stream_debug_info;

static void mtk_pq_common_dqbuf_cb(
			int error_cause,
			pid_t thread_id,
			uint32_t user_param,
			void *param);

static void mtk_pq_common_stream_off_cb(
			int error_cause,
			pid_t thread_id,
			uint32_t user_param,
			void *param);

static int create_pool(unsigned long addr, size_t size);
static int alloc_pool(unsigned long *paddr, size_t size);
static void free_pool(unsigned long addr, size_t size);

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
#define PQ_ATRACE_STR_SIZE 64
#define PQ_ATRACE_ASYNC_BEGIN(name, cookie) \
	do {\
		if (atrace_enable_pq == 1) \
			atrace_async_begin(name, cookie);\
	} while (0)
#define PQ_ATRACE_ASYNC_END(name, cookie) \
	do {\
		if (atrace_enable_pq == 1) \
			atrace_async_end(name, cookie);\
	} while (0)
#define PQ_ATRACE_BEGIN(name) \
	do {\
		if (atrace_enable_pq == 1) \
			atrace_begin(name);\
	} while (0)
#define PQ_ATRACE_END(name) \
	do {\
		if (atrace_enable_pq == 1) \
			atrace_end(name);\
	} while (0)
#define PQ_ATRACE_INT(name, value) \
	do {\
		if (atrace_enable_pq == 1) \
			atrace_int(name, value);\
	} while (0)
#define PQ_ATRACE_INT_FORMAT(val, format, ...) \
	do {\
		if (atrace_enable_pq == 1) { \
			char m_str[PQ_ATRACE_STR_SIZE];\
			snprintf(m_str, PQ_ATRACE_STR_SIZE, format, __VA_ARGS__);\
			atrace_int(m_str, val);\
		} \
	} while (0)

#else
#define PQ_ATRACE_ASYNC_BEGIN(name, cookie)
#define PQ_ATRACE_ASYNC_END(name, cookie)
#define PQ_ATRACE_BEGIN(name)
#define PQ_ATRACE_END(name)
#define PQ_ATRACE_INT(name, value)
#define PQ_ATRACE_INT_FORMAT(val, format, ...)
#endif

static __u64 _mtk_pq_common_get_ms_time(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int _mtk_pq_common_map_fd(int fd, void **va, u64 *size)
{
	int ret = 0;
	struct dma_buf *db = NULL;

	if (size == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, fd=(%d), size is NULL?(%s)\n",
			fd, (size == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	db = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(db)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_get fail, db=0x%x\n", db);
		return -EPERM;
	}

	*size = db->size;

	*va = dma_buf_vmap(db);

	if (!*va || (*va == -1)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_vmap fail\n");
		dma_buf_put(db);
		return -EPERM;
	}

	dma_buf_put(db);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "va=0x%llx, size=%llu\n", *va, *size);
	return ret;
}

static int _mtk_pq_common_map_db(struct dma_buf *db, void **va, u64 *size)
{
	int ret = 0;
	unsigned long long iova = 0;

	if ((size == NULL) || (IS_ERR_OR_NULL(db))) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid ptr, db=0x%x, size=0x%x\n", db, size);
		return -EINVAL;
	}

	*size = db->size;

	*va = dma_buf_vmap(db);
	if (!*va || (*va == -1)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_vmap fail\n");
		return -EPERM;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "va=0x%llx, size=%llu\n", *va, *size);
	return ret;
}

static int _mtk_pq_common_unmap_fd(int fd, void *va)
{
	struct dma_buf *db = NULL;

	if (va == NULL) {
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
	dma_buf_put(db);
	return 0;
}

static int _mtk_pq_common_unmap_db(struct dma_buf *db, void *va)
{
	if ((va == NULL) || (IS_ERR_OR_NULL(db))) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid ptr, db=0x%x, va=0x%x\n", db, va);
		return -EINVAL;
	}

	dma_buf_vunmap(db, va);

	return 0;
}

static enum pqu_idr_input_path _mtk_pq_common_set_pqu_idr_input_path_meta(
					enum meta_pq_idr_input_path pq_idr_input_path)
{
	switch (pq_idr_input_path) {
	case META_PQ_PATH_IPDMA_0:
		return PQU_PATH_IPDMA_0;
	case META_PQ_PATH_IPDMA_1:
		return PQU_PATH_IPDMA_1;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unknown enum :%d\n", pq_idr_input_path);
	}
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
#if IS_ENABLED(CONFIG_OPTEE)
		meta_header->size = sizeof(struct vdec_dd_svp_info);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_VDEC_DD_SVP_INFO_TAG, TAG_LENGTH);
#endif
		break;
	case EN_PQ_METATAG_SRCCAP_SVP_INFO:
#if IS_ENABLED(CONFIG_OPTEE)
		meta_header->size = sizeof(struct meta_srccap_svp_info);
		meta_header->ver = META_SRCCAP_SVP_INFO_VERSION;
		strncpy(meta_header->tag, META_SRCCAP_SVP_INFO_TAG, TAG_LENGTH);
#endif
		break;
	case EN_PQ_METATAG_SVP_INFO:
		meta_header->size = sizeof(struct meta_pq_disp_svp);
		meta_header->ver = META_PQ_DISP_SVP_VERSION;
		strncpy(meta_header->tag, PQ_DISP_SVP_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_SH_FRM_INFO:
		meta_header->size = sizeof(struct mtk_pq_frame_info);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_PQ_SH_FRM_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_DISP_MDW_CTRL:
		meta_header->size = sizeof(struct m_pq_display_mdw_ctrl);
		meta_header->ver = M_PQ_DISPLAY_MDW_CTRL_VERSION;
		strncpy(meta_header->tag, M_PQ_DISPLAY_MDW_CTRL_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_DISP_IDR_CTRL:
		meta_header->size = sizeof(struct m_pq_display_idr_ctrl);
		meta_header->ver = M_PQ_DISPLAY_IDR_CTRL_VERSION;
		strncpy(meta_header->tag, M_PQ_DISPLAY_IDR_CTRL_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_DISP_B2R_CTRL:
		meta_header->size = sizeof(struct m_pq_display_b2r_ctrl);
		meta_header->ver = M_PQ_DISPLAY_B2R_CTRL_VERSION;
		strncpy(meta_header->tag, M_PQ_DISPLAY_B2R_CTRL_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_DISP_FLOW_INFO:
		meta_header->size = sizeof(struct m_pq_display_flow_ctrl);
		meta_header->ver = M_PQ_DISPLAY_FLOW_CTRL_VERSION;
		strncpy(meta_header->tag, M_PQ_DISPLAY_FLOW_CTRL_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_DISP_WM_INFO:
		meta_header->size = sizeof(struct meta_pq_display_wm_info);
		meta_header->ver = M_PQ_DISPLAY_WM_VERSION;
		strncpy(meta_header->tag, M_PQ_DISPLAY_WM_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_OUTPUT_FRAME_INFO:
		meta_header->size = sizeof(struct meta_pq_output_frame_info);
		meta_header->ver = META_PQ_OUTPUT_FRAME_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_OUTPUT_FRAME_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_SRCCAP_FRAME_INFO:
		meta_header->size = sizeof(struct meta_srccap_frame_info);
		meta_header->ver = META_SRCCAP_FRAME_INFO_VERSION;
		strncpy(meta_header->tag, META_SRCCAP_FRAME_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_STREAM_INTERNAL_INFO:
		meta_header->size = sizeof(struct meta_pq_stream_internal_info);
		meta_header->ver = META_PQ_STREAM_INTERNAL_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_STREAM_INTERNAL_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_DEBUG_INFO:
		meta_header->size = sizeof(struct m_pq_common_debug_info);
		meta_header->ver = M_PQ_COMMON_DEBUG_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_COMMON_DEBUG_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_STREAM_INFO:
		meta_header->size = sizeof(struct m_pq_common_stream_info);
		meta_header->ver = M_PQ_COMMON_STREAM_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_COMMON_STREAM_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_STREAM_INFO:
		meta_header->size = sizeof(struct meta_pq_stream_info);
		meta_header->ver = META_PQ_STREAM_INFO_VERSION;
		strncpy(meta_header->tag, PQ_DISP_STREAM_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_PATTERN_INFO:
		meta_header->size = sizeof(struct pq_pattern_info);
		meta_header->ver = M_PQ_PATTERN_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_PATTERN_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_PATTERN_SIZE_INFO:
		meta_header->size = sizeof(struct pq_pattern_size_info);
		meta_header->ver = M_PQ_PATTERN_SIZE_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_PATTERN_SIZE_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_BBD_INFO:
		meta_header->size = sizeof(struct meta_pq_bbd_info);
		meta_header->ver = META_PQ_BBD_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_BBD_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_BBD_INFO:
		meta_header->size = sizeof(struct m_pq_bbd_info);
		meta_header->ver = M_PQ_BBD_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_BBD_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_DISPLAY_FLOW_INFO:
		meta_header->size = sizeof(struct meta_pq_display_flow_info);
		meta_header->ver = META_PQ_DISPLAY_FLOW_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_DISPLAY_FLOW_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_DISPLAY_WM_INFO:
		meta_header->size = sizeof(struct meta_pq_display_wm_info);
		meta_header->ver = META_PQ_DISPLAY_WM_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_DISPLAY_WM_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_INPUT_QUEUE_EXT_INFO:
		meta_header->size = sizeof(struct meta_pq_input_queue_ext_info);
		meta_header->ver = META_PQ_INPUT_QUEUE_EXT_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_INPUT_QUEUE_EXT_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO:
		meta_header->size = sizeof(struct meta_pq_output_queue_ext_info);
		meta_header->ver = META_PQ_OUTPUT_QUEUE_EXT_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_OUTPUT_QUEUE_EXT_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_QUEUE_EXT_INFO:
		meta_header->size = sizeof(struct m_pq_queue_ext_info);
		meta_header->ver = M_PQ_QUEUE_EXT_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_QUEUE_EXT_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_OUTPUT_DISP_IDR_CTRL:
		meta_header->size = sizeof(struct meta_pq_display_idr_ctrl);
		meta_header->ver = META_PQ_DISPLAY_IDR_CTRL_VERSION;
		strncpy(meta_header->tag, META_PQ_DISPLAY_IDR_CTRL_META_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_DV_DEBUG:
		meta_header->size = sizeof(struct m_pq_dv_debug);
		meta_header->ver = M_PQ_DV_DEBUG_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_DV_DEBUG_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQPARAM_TAG:
		meta_header->size = sizeof(struct meta_pq_pqparam);
		meta_header->ver = META_PQ_PQPARAM_VERSION;
		strncpy(meta_header->tag, META_PQ_PQPARAM_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_PQPARAM_TAG:
		meta_header->size = sizeof(struct m_pq_pqparam_meta);
		meta_header->ver = M_PQ_PQPARAM_VERSION;
		strlcpy(meta_header->tag, M_PQ_PQPARAM_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_DV_INFO:
		meta_header->size = sizeof(struct meta_pq_dv_info);
		meta_header->ver = META_PQ_DV_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_DV_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_SRCCAP_DV_HDMI_INFO:
		meta_header->size = sizeof(struct meta_srccap_dv_info);
		meta_header->ver = META_SRCCAP_DV_INFO_VERSION;
		strncpy(meta_header->tag, META_SRCCAP_DV_INFO_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_VDEC_DV_DESCRB_INFO:
		meta_header->size = sizeof(struct vdec_dd_dolby_desc);
		meta_header->ver = MTK_VDEC_DD_DOLBY_DESC_VERSION;
		strncpy(meta_header->tag, MTK_VDEC_DD_DOLBY_DESC_TAG, TAG_LENGTH);
		break;
	case EN_PQ_METATAG_PQU_THERMAL_INFO:
		meta_header->size = sizeof(struct m_pq_thermal);
		meta_header->ver = M_PQ_THERMAL_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_THERMAL_INFO_TAG, TAG_LENGTH);
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

int mtk_pq_common_read_metadata_db(struct dma_buf *meta_db,
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

	ret = _mtk_pq_common_map_db(meta_db, &va, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "trans db to va fail\n");
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
	meta_ret = query_metadata_content(&buffer, &header, meta_content);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "query metadata content fail\n");
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_pq_common_unmap_db(meta_db, va);
	return ret;
}

int mtk_pq_common_read_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_pq_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	struct meta_header header;

	if (meta_content == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, meta_content=NULL\n");
		return -EPERM;
	}

	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_pq_common_get_metaheader(meta_tag, &header);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get meta header fail\n");
		ret = -EPERM;
		return ret;
	}

	meta_ret = query_metadata_content(meta_buf, &header, meta_content);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "query metadata content fail\n");
		ret = -EPERM;
		return ret;
	}

	return ret;
}

int mtk_pq_common_write_metadata_db(struct dma_buf *meta_db,
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

	ret = _mtk_pq_common_map_db(meta_db, &va, &size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "trans db to va fail\n");
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
	_mtk_pq_common_unmap_db(meta_db, va);
	return ret;
}

int mtk_pq_common_write_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_pq_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	struct meta_header header;

	if (meta_content == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid input, meta_content=NULL\n");
		return -EPERM;
	}

	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_pq_common_get_metaheader(meta_tag, &header);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get meta header fail\n");
		ret = -EPERM;
		return ret;
	}

	meta_ret = attach_metadata(meta_buf, header, meta_content);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "attach metadata fail\n");
		ret = -EPERM;
		return ret;
	}

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

int mtk_pq_common_delete_metadata_db(struct dma_buf *meta_db,
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

	ret = _mtk_pq_common_map_db(meta_db, &va, &size);
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
	_mtk_pq_common_unmap_db(meta_db, va);
	return ret;
}

int mtk_pq_common_delete_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_pq_common_metatag meta_tag)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	struct meta_header header;

	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_pq_common_get_metaheader(meta_tag, &header);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get meta header fail\n");
		ret = -EPERM;
		return ret;
	}

	meta_ret = delete_metadata(meta_buf, &header);
	if (!meta_ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "delete metadata fail\n");
		ret = -EPERM;
		return ret;
	}

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

	if (mtk_pq_enhance_set_pix_format_out_mplane(pq_dev, &fmt->fmt.pix_mp)) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: pq enhance set pix fmt failed!\n", __func__);
		goto ERR;
	}

	/* note: this function will change fmt->fmt.pix_mp.num_planes */
	ret = mtk_pq_display_mdw_set_pix_format_mplane(pq_dev, fmt);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_display_mdw_set_pix_format_mplane failed, ret=%d\n", ret);
		goto ERR;
	}

	memcpy(&pq_dev->common_info.format_cap, fmt,
		sizeof(struct v4l2_format));

ERR:
	return ret;
}

int mtk_pq_common_set_fmt_out_mplane(
	struct mtk_pq_device *pq_dev,
	struct v4l2_format *fmt)
{
	int ret = 0;
	u32 source = 0;

	if ((!pq_dev) || (!fmt))
		return -ENOMEM;

	source = pq_dev->common_info.input_source;

	if (mtk_pq_enhance_set_pix_format_in_mplane(pq_dev, &fmt->fmt.pix_mp)) {
		ret = -EPERM;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: hdr set pix fmt failed!\n", __func__);
		goto ERR;
	}
	if (IS_INPUT_SRCCAP(source)) {
		/* note: this function will change fmt->fmt.pix_mp.num_planes */
		ret = mtk_pq_display_idr_set_pix_format_mplane(fmt, pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"%s: mtk_pq_display_idr_set_pix_format failed!\n", __func__);
			return ret;
		}
	} else if (IS_INPUT_B2R(source)) {
		if (mtk_pq_b2r_set_pix_format_in_mplane(fmt, &pq_dev->b2r_info) < 0) {
			ret = -EPERM;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"%s: b2r set pix fmt failed!\n", __func__);
			goto ERR;
		}
	} else {
		ret = -EINVAL;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"%s: unknown source %d!\n", __func__, source);
		goto ERR;
	}

	memcpy(&pq_dev->common_info.format_out, fmt,
		sizeof(struct v4l2_format));

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
	struct mtk_pq_device *pq_dev, void *ctrl)
{
	bool enable = false;
	__u8 input_source = 0;
	__u8 win = 0;

	if (!pq_dev || !ctrl) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}
	input_source = pq_dev->common_info.input_source;
	win = pq_dev->dev_indx;

	enable = *(bool *)ctrl;

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
	}

	return 0;
}

static int _mtk_pq_common_set_vflip(
	struct mtk_pq_device *pq_dev, void *ctrl)
{
	bool enable = false;

	if (!pq_dev || !ctrl) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}
	enable = *(bool *)ctrl;

	pq_dev->common_info.v_flip = enable;

	return 0;
}

static int _mtk_pq_common_set_forcep(
	struct mtk_pq_device *pq_dev, bool enable)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;

	/* set b2r*/
	mtk_pq_b2r_set_forcep(win, enable, &pq_dev->b2r_info);

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

	mtk_b2r_set_ext_timing(pq_dev->dev, win, ctrl, &pq_dev->b2r_info);

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

static int _mtk_pq_common_set_input_mode(
	struct mtk_pq_device *pq_dev, u8 *val)
{
	int ret = 0;
	struct mtk_pq_platform_device *pqdev = NULL;
	enum mtk_pq_input_mode mode = (enum mtk_pq_input_mode)*val;

	if (!pq_dev)
		return -ENOMEM;

	pqdev = dev_get_drvdata(pq_dev->dev);

	pq_dev->common_info.input_mode = mode;
	pq_dev->display_info.idr.input_mode = mode;

	// temp: hardcode sw mode
	if (mode == MTK_PQ_INPUT_MODE_HW) {
		pq_dev->common_info.input_mode = MTK_PQ_INPUT_MODE_SW;
		pq_dev->display_info.idr.input_mode = MTK_PQ_INPUT_MODE_SW;
	}


	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "SwMode_Support=%d, input_mode=%d\n",
		pqdev->pqcaps.u32Idr_SwMode_Support, mode);

	return ret;
}

static int _mtk_pq_common_set_output_mode(
	struct mtk_pq_device *pq_dev, u8 *val)
{
	int ret = 0;
	enum mtk_pq_output_mode mode = (enum mtk_pq_output_mode)*val;

	if (!pq_dev)
		return -ENOMEM;

	pq_dev->common_info.output_mode = mode;
	pq_dev->display_info.mdw.output_mode = mode;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "output_mode=%d\n", mode);

	return ret;
}

static int _mtk_pq_common_set_sw_usr_mode(
	enum pq_common_triggen_domain domain,
	bool sw_user_mode_h,
	bool sw_user_mode_v)
{
	int ret = 0;
	struct pq_common_triggen_sw_user_mode paramInSwUserMode;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramInSwUserMode, 0, sizeof(paramInSwUserMode));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	//set sw user mode
	paramInSwUserMode.domain = domain;
	paramInSwUserMode.sw_user_mode_h = sw_user_mode_h;
	paramInSwUserMode.sw_user_mode_v = sw_user_mode_v;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swUserMode(paramInSwUserMode, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swUserMode failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_all_sw_usr_mode(bool sw_mode)
{
	int ret = 0;

	/* set all trigger gen sw user */
	ret = _mtk_pq_common_set_sw_usr_mode(PQ_COMMON_TRIGEN_DOMAIN_B2R, sw_mode, sw_mode);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_set_sw_usr_mode(B2R, T, T) fail, ret = %d\n", ret);
		return ret;
	}
	ret = _mtk_pq_common_set_sw_usr_mode(PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1, sw_mode, sw_mode);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_set_sw_usr_mode(B2R, T, T) fail, ret = %d\n", ret);
		return ret;
	}
	ret = _mtk_pq_common_set_sw_usr_mode(PQ_COMMON_TRIGEN_DOMAIN_IP, sw_mode, sw_mode);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_set_sw_usr_mode(IP, T, T) fail, ret = %d\n", ret);
		return ret;
	}
	ret = _mtk_pq_common_set_sw_usr_mode(PQ_COMMON_TRIGEN_DOMAIN_OP1, sw_mode, sw_mode);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_set_sw_usr_mode(OP1, T, T) fail, ret = %d\n", ret);
		return ret;
	}
	ret = _mtk_pq_common_set_sw_usr_mode(PQ_COMMON_TRIGEN_DOMAIN_OP2, sw_mode, sw_mode);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_set_sw_usr_mode(OP2, T, T) fail, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_output_dqbuf_meta(uint8_t win_id, struct meta_buffer meta_buf)
{
	int ret = 0;
	enum pqu_dip_cap_point idx = pqu_dip_cap_pqin;
	struct meta_pq_display_flow_info meta_disp_info;
	struct m_pq_display_flow_ctrl pqu_disp_info;
	struct pq_pattern_size_info size_info;
	struct meta_pq_bbd_info meta_bbd_info;
	struct m_pq_bbd_info pqu_bbd_info;

	memset(&size_info, 0, sizeof(struct pq_pattern_size_info));
	memset(&meta_disp_info, 0, sizeof(struct meta_pq_display_flow_info));
	memset(&pqu_disp_info, 0, sizeof(struct m_pq_display_flow_ctrl));
	memset(&meta_bbd_info, 0, sizeof(struct meta_pq_bbd_info));
	memset(&pqu_bbd_info, 0, sizeof(struct m_pq_bbd_info));

	/* fill display flow info start */
	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_DISPLAY_FLOW_INFO, (void *)&meta_disp_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_DISPLAY_FLOW_INFO fail,ret = %d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_DISPLAY_FLOW_INFO success\n");
	}

	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_PQU_DISP_FLOW_INFO, (void *)&pqu_disp_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_PQU_DISP_FLOW_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_PQU_DISP_FLOW_INFO success\n");
	}

	meta_disp_info.win_id = win_id;
	for (idx = pqu_dip_cap_pqin; idx < pqu_dip_cap_max; idx++)
		memcpy(&(meta_disp_info.dip_win[idx]), &(pqu_disp_info.dip_win[idx]),
				sizeof(struct dip_window_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"meta_pq_display_flow_info : %s%d, %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d)\n",
	"win_id = ", meta_disp_info.win_id,
	"pqu_dip_cap_pqin = ", meta_disp_info.dip_win[pqu_dip_cap_pqin].width,
		meta_disp_info.dip_win[pqu_dip_cap_pqin].height,
		meta_disp_info.dip_win[pqu_dip_cap_pqin].color_fmt,
		meta_disp_info.dip_win[pqu_dip_cap_pqin].p_engine,
	"pqu_dip_cap_iphdr = ", meta_disp_info.dip_win[pqu_dip_cap_iphdr].width,
		meta_disp_info.dip_win[pqu_dip_cap_iphdr].height,
		meta_disp_info.dip_win[pqu_dip_cap_iphdr].color_fmt,
		meta_disp_info.dip_win[pqu_dip_cap_iphdr].p_engine,
	"pqu_dip_cap_prespf = ", meta_disp_info.dip_win[pqu_dip_cap_prespf].width,
		meta_disp_info.dip_win[pqu_dip_cap_prespf].height,
		meta_disp_info.dip_win[pqu_dip_cap_prespf].color_fmt,
		meta_disp_info.dip_win[pqu_dip_cap_prespf].p_engine);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"meta_pq_display_flow_info : %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d)\n",
	"pqu_dip_cap_srs = ", meta_disp_info.dip_win[pqu_dip_cap_srs].width,
		meta_disp_info.dip_win[pqu_dip_cap_srs].height,
		meta_disp_info.dip_win[pqu_dip_cap_srs].color_fmt,
		meta_disp_info.dip_win[pqu_dip_cap_srs].p_engine,
	"pqu_dip_cap_vip = ", meta_disp_info.dip_win[pqu_dip_cap_vip].width,
		meta_disp_info.dip_win[pqu_dip_cap_vip].height,
		meta_disp_info.dip_win[pqu_dip_cap_vip].color_fmt,
		meta_disp_info.dip_win[pqu_dip_cap_vip].p_engine,
	"pqu_dip_cap_mdw = ", meta_disp_info.dip_win[pqu_dip_cap_mdw].width,
		meta_disp_info.dip_win[pqu_dip_cap_mdw].height,
		meta_disp_info.dip_win[pqu_dip_cap_mdw].color_fmt,
		meta_disp_info.dip_win[pqu_dip_cap_mdw].p_engine);

	ret = mtk_pq_common_write_metadata_addr(&meta_buf,
		EN_PQ_METATAG_DISPLAY_FLOW_INFO, (void *)&meta_disp_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_DISPLAY_FLOW_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_DISPLAY_FLOW_INFO success\n");
	}
	/* fill display flow info end */

	/* fill bbd info start */
	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_PQU_BBD_INFO, (void *)&pqu_bbd_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_PQU_BBD_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_PQU_BBD_INFO success\n");
	}

	memcpy(&meta_bbd_info, &pqu_bbd_info, sizeof(struct meta_pq_bbd_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"meta_pq_bbd_info : %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d\n",
	"u8Validity = ", meta_bbd_info.u8Validity,
	"u16LeftOuterPos = ", meta_bbd_info.u16LeftOuterPos,
	"u16RightOuterPos = ", meta_bbd_info.u16RightOuterPos,
	"u16LeftInnerPos = ", meta_bbd_info.u16LeftInnerPos,
	"u16LeftInnerConf = ", meta_bbd_info.u16LeftInnerConf,
	"u16RightInnerPos = ", meta_bbd_info.u16RightInnerPos,
	"u16RightInnerConf = ", meta_bbd_info.u16RightInnerConf);

	ret = mtk_pq_common_write_metadata_addr(&meta_buf,
		EN_PQ_METATAG_BBD_INFO, (void *)&meta_bbd_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_BBD_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_BBD_INFO success\n");
	}
	/* fill bbd info end */

#ifdef PQU_PATTERN_TEST
	/* fill test pattern info start */
	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_PQU_PATTERN_SIZE_INFO, (void *)&size_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_PQU_PATTERN_SIZE_INFO failed, ret = %d\n"
				, ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_PQU_PATTERN_SIZE_INFO success\n");
	}

	ret = mtk_pq_pattern_set_size_info(&size_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_pattern_set_size_info failed, ret = %d\n", ret);
		goto exit;
	}
	/* fill test pattern info end */
#endif

exit:
	return ret;
}

static int _mtk_pq_common_set_capture_dqbuf_meta(uint8_t win_id, struct meta_buffer meta_buf)
{
	int ret = 0;
	enum pqu_dip_cap_point idx = pqu_dip_cap_pqin;
	static struct m_pq_display_flow_ctrl pqu_disp_info;
	static struct meta_pq_output_queue_ext_info pq_outputq_ext_info;
	struct meta_pq_display_wm_info pq_wm_meta;
	static struct m_pq_queue_ext_info pqu_queue_ext_info;
	static struct meta_pq_display_flow_info meta_disp_info;
	static struct m_pq_display_idr_ctrl pqu_disp_idr_crtl;
	struct meta_pq_display_idr_ctrl meta_disp_idr_crtl;
	struct meta_pq_output_frame_info meta_pqdd_frm;

	memset(&pqu_disp_info, 0, sizeof(struct m_pq_display_flow_ctrl));
	memset(&pq_outputq_ext_info, 0, sizeof(struct meta_pq_output_queue_ext_info));
	memset(&pq_wm_meta, 0, sizeof(struct meta_pq_display_wm_info));
	memset(&pqu_queue_ext_info, 0, sizeof(struct m_pq_queue_ext_info));
	memset(&meta_disp_info, 0, sizeof(struct meta_pq_display_flow_info));
	memset(&pqu_disp_idr_crtl, 0, sizeof(struct m_pq_display_idr_ctrl));
	memset(&meta_disp_idr_crtl, 0, sizeof(struct meta_pq_display_idr_ctrl));
	memset(&meta_pqdd_frm, 0, sizeof(struct meta_pq_output_frame_info));

	/* fill queue ext info start */
	ret = mtk_pq_common_read_metadata_addr(
		&meta_buf, EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO, &pq_outputq_ext_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO failed, ret = %d\n"
				, ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO success\n");
	}

	ret = mtk_pq_common_read_metadata_addr(
		&meta_buf, EN_PQ_METATAG_PQU_QUEUE_EXT_INFO, &pqu_queue_ext_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_PQU_QUEUE_EXT_INFO failed, ret = %d\n"
				, ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_PQU_QUEUE_EXT_INFO success\n");
	}

	ret = mtk_pq_common_read_metadata_addr(
		&meta_buf, EN_PQ_METATAG_PQU_DISP_WM_INFO, &pq_wm_meta);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_PQU_DISP_WM_INFO failed, ret = %d\n"
				, ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_PQU_DISP_WM_INFO success\n");
	}

	pq_outputq_ext_info.u64Pts = pqu_queue_ext_info.u64Pts;
	pq_outputq_ext_info.u64UniIdx = pqu_queue_ext_info.u64UniIdx;
	pq_outputq_ext_info.u64ExtUniIdx = pqu_queue_ext_info.u64ExtUniIdx;
	pq_outputq_ext_info.u64TimeStamp = pqu_queue_ext_info.u64TimeStamp;
	pq_outputq_ext_info.u64RenderTimeNs = pqu_queue_ext_info.u64RenderTimeNs;
	pq_outputq_ext_info.u8BufferValid = pqu_queue_ext_info.u8BufferValid;
	pq_outputq_ext_info.u32BufferSlot = pqu_queue_ext_info.u32BufferSlot;
	pq_outputq_ext_info.u32GenerationIndex = pqu_queue_ext_info.u32GenerationIndex;
	pq_outputq_ext_info.u8RepeatStatus = pqu_queue_ext_info.u8RepeatStatus;
	pq_outputq_ext_info.u8FrcMode = pqu_queue_ext_info.u8FrcMode;
	pq_outputq_ext_info.u8Interlace = pqu_queue_ext_info.u8Interlace;
	pq_outputq_ext_info.u32InputFps = pqu_queue_ext_info.u32InputFps;
	pq_outputq_ext_info.u32OriginalInputFps = pqu_queue_ext_info.u32OriginalInputFps;
	pq_outputq_ext_info.bEOS = pqu_queue_ext_info.bEOS;
	pq_outputq_ext_info.u8MuteAction = pqu_queue_ext_info.u8MuteAction;
	pq_outputq_ext_info.u8SignalStable = pqu_queue_ext_info.u8SignalStable;
	pq_outputq_ext_info.u8DotByDotType = pqu_queue_ext_info.u8DotByDotType;
	pq_outputq_ext_info.u32RefreshRate = pqu_queue_ext_info.u32RefreshRate;
	pq_outputq_ext_info.u32QueueInputIndex = pqu_queue_ext_info.u32QueueInputIndex;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"meta_pq_output_queue_ext_info : %s%llu, %s%llu, %s%llu, %s%llu, %s%llu, %s%u, %s%u, %s%u, %s%u, %s%u, %s%u, %s%u, %s%d, %s%u, %s%u, %s%u, %s%u, %s%u, %s%u\n",
	"u64Pts = ", pq_outputq_ext_info.u64Pts,
	"u64UniIdx = ", pq_outputq_ext_info.u64UniIdx,
	"u64ExtUniIdx = ", pq_outputq_ext_info.u64ExtUniIdx,
	"u64TimeStamp = ", pq_outputq_ext_info.u64TimeStamp,
	"u64RenderTimeNs = ", pq_outputq_ext_info.u64RenderTimeNs,
	"u8BufferValid = ", pq_outputq_ext_info.u8BufferValid,
	"u32GenerationIndex = ", pq_outputq_ext_info.u32GenerationIndex,
	"u8RepeatStatus = ", pq_outputq_ext_info.u8RepeatStatus,
	"u8FrcMode = ", pq_outputq_ext_info.u8FrcMode,
	"u8Interlace = ", pq_outputq_ext_info.u8Interlace,
	"u32InputFps = ", pq_outputq_ext_info.u32InputFps,
	"u32OriginalInputFps = ", pq_outputq_ext_info.u32OriginalInputFps,
	"bEOS = ", pq_outputq_ext_info.bEOS,
	"u8MuteAction = ", pq_outputq_ext_info.u8MuteAction,
	"u8SignalStable = ", pq_outputq_ext_info.u8SignalStable,
	"u8DotByDotType = ", pq_outputq_ext_info.u8DotByDotType,
	"u32RefreshRate = ", pq_outputq_ext_info.u32RefreshRate,
	"u32QueueOutputIndex = ", pq_outputq_ext_info.u32QueueOutputIndex,
	"u32QueueInputIndex = ", pq_outputq_ext_info.u32QueueInputIndex);

	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO, &pq_outputq_ext_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO failed, ret = %d\n"
				, ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_OUTPUT_QUEUE_EXT_INFO success\n");
	}
	/* fill queue ext info end */

	/* fill queue ext info to output info start */
	ret = mtk_pq_common_read_metadata_addr(
		&meta_buf, EN_PQ_METATAG_OUTPUT_FRAME_INFO, &meta_pqdd_frm);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_write_metadata_addr Failed, ret = %d\n", ret);
		return ret;
	}
	meta_pqdd_frm.pq_frame_id = pqu_queue_ext_info.int32FrameId;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "pqu_queue_ext_info : %s%d\n",
		"pq_frame_id = ", meta_pqdd_frm.pq_frame_id);

	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_OUTPUT_FRAME_INFO, &meta_pqdd_frm);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_write_metadata_addr Failed, ret = %d\n", ret);
		return ret;
	}
	/* fill queue ext info to output info end */

	/* fill display flow info start */
	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_PQU_DISP_FLOW_INFO, (void *)&pqu_disp_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_PQU_DISP_FLOW_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_PQU_DISP_FLOW_INFO success\n");
	}

	memcpy(&(meta_disp_info.content), &(pqu_disp_info.content),
				sizeof(struct meta_pq_window));
	memcpy(&(meta_disp_info.capture), &(pqu_disp_info.cap_win),
				sizeof(struct meta_pq_window));
	memcpy(&(meta_disp_info.crop), &(pqu_disp_info.crop_win),
				sizeof(struct meta_pq_window));
	memcpy(&(meta_disp_info.display), &(pqu_disp_info.disp_win),
				sizeof(struct meta_pq_window));
	memcpy(&(meta_disp_info.displayArea), &(pqu_disp_info.displayArea),
				sizeof(struct meta_pq_window));
	memcpy(&(meta_disp_info.displayRange), &(pqu_disp_info.displayRange),
				sizeof(struct meta_pq_window));

	meta_disp_info.win_id = win_id;
	for (idx = pqu_dip_cap_pqin; idx < pqu_dip_cap_max; idx++)
		memcpy(&(meta_disp_info.dip_win[idx]), &(pqu_disp_info.dip_win[idx]),
				sizeof(struct dip_window_info));
	memcpy(&(meta_disp_info.outcrop), &(pqu_disp_info.ip_win[pqu_ip_scmi_out]),
				sizeof(struct meta_pq_window));
	memcpy(&(meta_disp_info.outdisplay), &(pqu_disp_info.ip_win[pqu_ip_hvsp_out]),
				sizeof(struct meta_pq_window));
	memcpy(&(meta_disp_info.wm_config), &(pqu_disp_info.wm_config),
				sizeof(struct meta_pq_wm_config));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"meta_pq_display_flow_info : %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d)\n",
	"content = ", meta_disp_info.content.x, meta_disp_info.content.y,
		meta_disp_info.content.width, meta_disp_info.content.height,
	"capture = ", meta_disp_info.capture.x, meta_disp_info.capture.y,
		meta_disp_info.capture.width, meta_disp_info.capture.height,
	"crop = ", meta_disp_info.crop.x, meta_disp_info.crop.y,
		meta_disp_info.crop.width, meta_disp_info.crop.height,
	"display = ", meta_disp_info.display.x, meta_disp_info.display.y,
		meta_disp_info.display.width, meta_disp_info.display.height,
	"displayArea = ", meta_disp_info.displayArea.x, meta_disp_info.displayArea.y,
		meta_disp_info.displayArea.width, meta_disp_info.displayArea.height,
	"displayRange = ", meta_disp_info.displayRange.x, meta_disp_info.displayRange.y,
		meta_disp_info.displayRange.width, meta_disp_info.displayRange.height);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"meta_pq_display_flow_info : %s(%d,%d,%d,%d), %s(%d,%d,%d,%d)\n",
	"outcrop = ", meta_disp_info.outcrop.x, meta_disp_info.outcrop.y,
		meta_disp_info.outcrop.width, meta_disp_info.outcrop.height,
	"outdisplay = ", meta_disp_info.outdisplay.x, meta_disp_info.outdisplay.y,
		meta_disp_info.outdisplay.width, meta_disp_info.outdisplay.height);

	ret = mtk_pq_common_write_metadata_addr(&meta_buf,
		EN_PQ_METATAG_DISPLAY_FLOW_INFO, (void *)&meta_disp_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_DISPLAY_FLOW_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_DISPLAY_FLOW_INFO success\n");
	}

	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_DISPLAY_WM_INFO, &pq_wm_meta);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_DISPLAY_WM_INFO failed, ret = %d\n"
				, ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_DISPLAY_WM_INFO success\n");
	}
	/* fill display flow info end */

	/* fill display idr ctrl start */
	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_DISP_IDR_CTRL, (void *)&pqu_disp_idr_crtl);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_DISP_IDR_CTRL fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_DISP_IDR_CTRL success\n");
	}

	meta_disp_idr_crtl.mem_fmt = pqu_disp_idr_crtl.mem_fmt;
	meta_disp_idr_crtl.width = pqu_disp_idr_crtl.width;
	meta_disp_idr_crtl.height = pqu_disp_idr_crtl.height;
	meta_disp_idr_crtl.index = pqu_disp_idr_crtl.index;
	meta_disp_idr_crtl.crop.left = pqu_disp_idr_crtl.crop.left;
	meta_disp_idr_crtl.crop.top = pqu_disp_idr_crtl.crop.top;
	meta_disp_idr_crtl.crop.width = pqu_disp_idr_crtl.crop.width;
	meta_disp_idr_crtl.crop.height = pqu_disp_idr_crtl.crop.height;
	if (pqu_disp_idr_crtl.path == PQU_PATH_IPDMA_0)
		meta_disp_idr_crtl.path = META_PQ_PATH_IPDMA_0;
	else if (pqu_disp_idr_crtl.path == PQU_PATH_IPDMA_1)
		meta_disp_idr_crtl.path = META_PQ_PATH_IPDMA_1;
	else
		meta_disp_idr_crtl.path = META_PQ_PATH_IPDMA_MAX;
	meta_disp_idr_crtl.bypass_ipdma = pqu_disp_idr_crtl.bypass_ipdma;
	meta_disp_idr_crtl.v_flip = pqu_disp_idr_crtl.v_flip;
	meta_disp_idr_crtl.aid = pqu_disp_idr_crtl.aid;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"meta_disp_idr_crtl : %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d\n",
	"mem_fmt = ", meta_disp_idr_crtl.mem_fmt,
	"width = ", meta_disp_idr_crtl.width,
	"height = ", meta_disp_idr_crtl.height,
	"index = ", meta_disp_idr_crtl.index,
	"crop.left = ", meta_disp_idr_crtl.crop.left,
	"crop.top = ", meta_disp_idr_crtl.crop.top,
	"crop.width = ", meta_disp_idr_crtl.crop.width,
	"crop.height = ", meta_disp_idr_crtl.crop.height,
	"path = ", meta_disp_idr_crtl.path,
	"bypass_ipdma = ", meta_disp_idr_crtl.bypass_ipdma,
	"v_flip = ", meta_disp_idr_crtl.v_flip,
	"aid = ", meta_disp_idr_crtl.aid);

	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_OUTPUT_DISP_IDR_CTRL, &meta_disp_idr_crtl);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_OUTPUT_DISP_IDR_CTRL failed, ret = %d\n"
				, ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_OUTPUT_DISP_IDR_CTRL success\n");
	}
	/* fill display idr ctrl end */
exit:
	return ret;
}

static int _mtk_pq_common_set_flow_control(struct mtk_pq_device *pq_dev, void *flowctrl)
{
	int ret = 0;
	__u8 win = pq_dev->dev_indx;
	struct v4l2_pq_flow_control flowcontrol;

	memset(&flowcontrol, 0, sizeof(struct v4l2_pq_flow_control));
	memcpy(&flowcontrol, flowctrl, sizeof(struct v4l2_pq_flow_control));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "[FC] flow control enable=%d\n", flowcontrol.enable);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "[FC] flow control factor=%d\n", flowcontrol.factor);

	pq_dev->display_info.flowControl.enable = flowcontrol.enable;
	pq_dev->display_info.flowControl.factor = flowcontrol.factor;

	return 0;
}

static int _mtk_pq_common_set_aisr_active_win(struct mtk_pq_device *pq_dev, void *win)
{
	struct v4l2_pq_aisr_active_win active_win;

	memset(&active_win, 0, sizeof(struct v4l2_pq_aisr_active_win));
	memcpy(&active_win, win, sizeof(struct v4l2_pq_aisr_active_win));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "[AISR] Active win enable=%d\n", active_win.bEnable);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "[AISR] Active win x=%u, y=%u, w=%u, h=%u\n",
				active_win.x, active_win.y, active_win.width, active_win.height);

	pq_dev->display_info.aisrActiveWin.bEnable = active_win.bEnable;
	pq_dev->display_info.aisrActiveWin.x = active_win.x;
	pq_dev->display_info.aisrActiveWin.y = active_win.y;
	pq_dev->display_info.aisrActiveWin.width = active_win.width;
	pq_dev->display_info.aisrActiveWin.height = active_win.height;

	return 0;
}

static int _mtk_pq_common_set_pix_format_info(struct mtk_pq_device *pq_dev, void *bufferctrl)
{
	struct v4l2_pq_s_buffer_info bufferInfo;
	u32 source = 0;
	int ret = 0;

	if ((!pq_dev) || (!bufferctrl))
		return -ENOMEM;

	source = pq_dev->common_info.input_source;

	memset(&bufferInfo, 0, sizeof(struct v4l2_pq_s_buffer_info));
	memcpy(&bufferInfo, bufferctrl, sizeof(struct v4l2_pq_s_buffer_info));

	switch (bufferInfo.type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (IS_INPUT_SRCCAP(source)) {
			ret = mtk_pq_display_idr_set_pix_format_mplane_info(pq_dev, bufferctrl);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					   "%s: idr_set_pix_format failed!\n",
					   __func__);
				return ret;
			}
		} else if (IS_INPUT_B2R(source)) {
			ret = mtk_pq_display_b2r_set_pix_format_mplane_info(pq_dev, bufferctrl);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					   "%s: b2r_set_pix_format failed!\n",
					   __func__);
				return ret;
			}
		} else {
			ret = -EINVAL;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				   "%s: unknown source %d!\n", __func__, source);
			goto ERR;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		mtk_pq_display_mdw_set_pix_format_mplane_info(pq_dev,
			bufferctrl);
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid buffer type %d!\n", bufferInfo.type);
		return -EINVAL;
	}

ERR:
	return ret;
}

static int _mtk_pq_common_set_pq_memory_index(struct mtk_pq_device *pq_dev, u8 *val)
{
	int ret = 0;
	struct mtk_pq_platform_device *pqdev = NULL;

	if (!pq_dev)
		return -ENOMEM;

	pqdev = dev_get_drvdata(pq_dev->dev);
	if (!pqdev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}
	pqdev->pq_memory_index = (uint8_t)*val;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "set pq memory index = %d\n",
	pqdev->pq_memory_index);

	return ret;
}

static int _mtk_pq_common_set_stage_pattern(struct mtk_pq_device *pq_dev, void *ctrl)
{
	int ret = 0;
	struct hwregOut pattern_reg;
	struct stage_pattern_info stage_pat_info;
	struct hwreg_pq_pattern_pure_color data;
	struct hwreg_pq_pattern_pure_colorbar data1;
	enum pq_pattern_position position;

	memset(&pattern_reg, 0, sizeof(struct hwregOut));
	pattern_reg.reg = vmalloc(sizeof(struct reg_info) * PATTERN_REG_NUM_MAX);
	if (pattern_reg.reg == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN, "vmalloc pattern reg fail!\n");
		return -EINVAL;
	}
	pattern_reg.regCount = 0;
	memset(&stage_pat_info, 0, sizeof(struct stage_pattern_info));
	memcpy(&stage_pat_info, ctrl, sizeof(struct stage_pattern_info));
	position = stage_pat_info.position;

	switch (position) {
	case PQ_PATTERN_POSITION_VOP:
		memset(&data, 0, sizeof(struct hwreg_pq_pattern_pure_color));
		data.common.position = (enum hwreg_pq_pattern_position)position;
		data.common.enable = stage_pat_info.enable;
		data.common.color_space = stage_pat_info.color_space;
		data.color.red = stage_pat_info.color.red;
		data.color.green = stage_pat_info.color.green;
		data.color.blue = stage_pat_info.color.blue;

		ret = drv_hwreg_pq_pattern_set_pure_color(true, &data, &pattern_reg);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN,
				"set VOP pattern fail, ret = %d!\n", ret);
			return -EINVAL;
		}
		break;
	case PQ_PATTERN_POSITION_NR_OPM:
	case PQ_PATTERN_POSITION_SRS_OUT:
		memset(&data1, 0, sizeof(struct hwreg_pq_pattern_pure_colorbar));
		data1.common.position = (enum hwreg_pq_pattern_position)position;
		data1.common.enable = stage_pat_info.enable;
		data1.common.color_space = stage_pat_info.color_space;
		data1.diff_h = PTN_H_SIZE;
		data1.diff_v = PTN_V_SIZE;

		ret = drv_hwreg_pq_pattern_set_pure_colorbar(true, &data1, &pattern_reg);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN,
				"set NR_OPM/SRS_OUT pattern fail, ret = %d!\n", ret);
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
		goto ERR;
	}

ERR:
	return ret;
}

int mtk_pq_common_get_dma_buf(struct mtk_pq_device *pq_dev,
	struct mtk_pq_dma_buf *buf)
{
	struct scatterlist *sg = NULL;

	if (!pq_dev || !buf || !buf->dev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (buf->fd < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid dma fd::%d!\n", buf->fd);
		return -EINVAL;
	}

	buf->db = dma_buf_get(buf->fd);
	if (IS_ERR_OR_NULL(buf->db)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_get fail!\n");
		return -EINVAL;
	}

	buf->size = buf->db->size;

	buf->va = dma_buf_vmap(buf->db);
	if (buf->va == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_vmap fail\n");
		goto vmap_fail;
	}

	buf->attach = dma_buf_attach(buf->db, buf->dev);
	if (IS_ERR(buf->attach)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_attach fail!\n");
		goto attach_fail;
	}

	buf->sgt = dma_buf_map_attachment(buf->attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(buf->sgt)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_map_attachment fail!\n");
		goto map_attachment_fail;
	}

	sg = buf->sgt->sgl;
	if (sg == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "sgl is NULL!\n");
		goto dms_address_fail;
	}

	buf->pa = page_to_phys(sg_page(sg)) + sg->offset;

	buf->iova = sg_dma_address(sg);
	if (buf->iova < 0x200000000) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Error iova=0x%llx\n", buf->iova);
		goto dms_address_fail;
	}

	if (bPquEnable) {
		unsigned long pa = 0;

		if (alloc_pool(&pa, buf->size)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "alloc_pool fail\n");
			return -ENOMEM;
		}
		buf->pa = pa;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"fd = %d, va = 0x%llx, pa = 0x%llx, iova=0x%llx, size=0x%llx\n",
		buf->fd, buf->va, buf->pa, buf->iova, buf->size);

	return 0;

dms_address_fail:
	dma_buf_unmap_attachment(buf->attach, buf->sgt, DMA_BIDIRECTIONAL);
map_attachment_fail:
	dma_buf_detach(buf->db, buf->attach);
attach_fail:
	dma_buf_vunmap(buf->db, buf->va);
vmap_fail:
	dma_buf_put(buf->db);

	return -EINVAL;
}

int mtk_pq_common_put_dma_buf(struct mtk_pq_dma_buf *buf)
{
	if ((!buf) || (buf->va == NULL) || !(buf->db && buf->attach && buf->sgt)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid params!\n");
		return -EINVAL;
	}

	dma_buf_unmap_attachment(buf->attach, buf->sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(buf->db, buf->attach);
	dma_buf_vunmap(buf->db, buf->va);
	dma_buf_put(buf->db);

	if (bPquEnable)
		gen_pool_free(pool, buf->pa, buf->size);

	buf->va = NULL;
	buf->pa = NULL;
	buf->iova = 0;
	buf->size = 0;
	buf->db = NULL;
	buf->attach = NULL;
	buf->sgt = NULL;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "puf dma buf (fd = %d) success!\n", buf->fd);

	return 0;
}

static int create_pool(unsigned long addr, size_t size)
{
	if (pool != NULL)
		return 0;

	pool = gen_pool_create(ALLOC_ORDER, NODE_ID);
	if (pool == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Failed to gen_pool_create\n");
		return -1;
	}

	return gen_pool_add(pool, addr, size, -1);
}

static int alloc_pool(unsigned long *paddr, size_t size)
{
	if (paddr == NULL || pool == NULL)
		return -1;

	*paddr = gen_pool_alloc(pool, size);

	return 0;
}

static void free_pool(unsigned long addr, size_t size)
{
	gen_pool_free(pool, addr, size);
}

int mtk_pq_common_open(struct mtk_pq_device *pq_dev,
	struct msg_new_win_info *msg_info)
{
	int ret = 0;
	struct msg_new_win_done_info reply_info = {0};
	struct callback_info dequeue_frame_callback_info = {0};

	if (!pq_dev || !msg_info) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	msg_info->win_id = (__u8)pq_dev->dev_indx;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "enter common open, dev_indx=%u!\n", pq_dev->dev_indx);


	if (bPquEnable) {
		ret = pqu_video_new_win(msg_info, &reply_info);
		dequeue_frame_callback_info.callback = mtk_pq_common_dqbuf_cb;
		if (pqu_video_dequeue_frame(&dequeue_frame_callback_info))
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pqu_video_dequeue_frame fail\n");

		if (create_pool(pq_dev->qbuf_meta_pool_addr, pq_dev->qbuf_meta_pool_size))
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "gen pool fail!\n");
	} else {
		pqu_msg_send(PQU_MSG_SEND_NEW_WIN, (void *)msg_info);
	}

	return ret;
}

int mtk_pq_common_stream_on(struct file *file, int type,
	struct mtk_pq_device *pq_dev,
	struct msg_stream_on_info *msg_info)
{
	int ret = 0;
	struct meta_pq_stream_info stream;
	struct meta_pq_stream_internal_info internal_info;
	struct pq_buffer pq_buf;
	enum pqu_buffer_type buf_idx;
	enum pqu_buffer_channel buf_ch;
	struct meta_buffer meta_buf;
	struct meta_buffer pqu_meta_buf;
	struct m_pq_common_stream_info stream_info;
	struct m_pq_common_debug_info stream_debug_info;
	struct mtk_pq_platform_device *pqdev = NULL;
	struct callback_info stream_on_callback_info = {0};
	__u8 input_source = 0;

	if (!file || !pq_dev || !msg_info) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	memset(&meta_buf, 0, sizeof(struct meta_buffer));
	memset(&pqu_meta_buf, 0, sizeof(struct meta_buffer));
	memset(&stream, 0, sizeof(struct meta_pq_stream_info));
	memset(&internal_info, 0, sizeof(struct meta_pq_stream_internal_info));
	memset(&stream_info, 0, sizeof(struct m_pq_common_stream_info));
	memset(&stream_debug_info, 0, sizeof(struct m_pq_common_debug_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"enter common stream on, dev_indx=%u, buffer type = %d!\n",
		pq_dev->dev_indx, type);

	pqdev = dev_get_drvdata(pq_dev->dev);
	input_source = pq_dev->common_info.input_source;

	if (!pq_dev->stream_on_ref) {
		ret = mtk_pq_common_get_dma_buf(pq_dev, &pq_dev->stream_meta);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "get stream meta buf fail\n");
			return ret;
		}
	}

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		meta_buf.paddr = (unsigned char *)pq_dev->stream_meta.va;
		meta_buf.size = (unsigned int)pq_dev->stream_meta.size;
		pqu_meta_buf.paddr =  (unsigned char *)pq_dev->pqu_stream_meta.va;
		pqu_meta_buf.size = (unsigned int)pq_dev->pqu_stream_meta.size;

		internal_info.file = (u64)file;
		internal_info.pq_dev = (u64)pq_dev;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"%s%llx, %s%llx\n",
			"file = 0x", internal_info.file,
			"pq_dev = 0x", internal_info.pq_dev);

		ret = mtk_pq_common_write_metadata_addr(&pqu_meta_buf,
			EN_PQ_METATAG_STREAM_INTERNAL_INFO, (void *)&internal_info);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk-pq write EN_PQ_METATAG_STREAM_INTERNAL_INFO Failed, ret = %d\n"
					, ret);
			goto put_dma_buf;
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
				"mtk-pq write EN_PQ_METATAG_STREAM_INTERNAL_INFO success\n");
		}

		/* read meta from stream metadata */
		ret = mtk_pq_common_read_metadata_addr(&meta_buf,
			EN_PQ_METATAG_STREAM_INFO, &stream);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk-pq read EN_PQ_METATAG_STREAM_INFO Failed, ret = %d\n", ret);
			goto put_dma_buf;
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
				"mtk-pq read EN_PQ_METATAG_STREAM_INFO success\n");
		}

		stream_info.input_source    = input_source;//pq_dev->common_info.input_source;
		stream_info.width           = stream.width;
		stream_info.height          = stream.height;
		stream_info.interlace       = stream.interlace;
		stream_info.adaptive_stream = stream.adaptive_stream;
		stream_info.low_latency     = stream.low_latency;
		stream_info.v_flip          = pq_dev->common_info.v_flip;
		stream_info.source          = (enum pqu_source)stream.pq_source;
		stream_info.mode            = (enum pqu_mode)stream.pq_mode;
		stream_info.scene           = (enum pqu_scene)stream.pq_scene;
		stream_info.framerate       = (enum pqu_framerate)stream.pq_framerate;
		stream_info.quality         = (enum pqu_quality)stream.pq_quality;
		stream_info.colorformat     = (enum pqu_colorformat)stream.pq_colorformat;
		stream_info.aisr_support    = ((bool)pqdev->pqcaps.u32AISR_Support);
		stream_info.phase_ip2       = pqdev->pqcaps.u32Phase_IP2;
		stream_info.aisr_version    = pqdev->pqcaps.u32AISR_Version;
		stream_info.cfd_pq_version	= pqdev->pqcaps.u32CFD_PQ_Version;
		stream_info.stream_b2r.b2r_version    = pqdev->b2r_dev.b2r_ver;
		stream_info.stream_b2r.h_max_size    = pqdev->b2r_dev.h_max_size;
		stream_info.stream_b2r.v_max_size    = pqdev->b2r_dev.v_max_size;
		stream_info.mdw_version    = pqdev->pqcaps.u32MDW_Version;
		stream_info.scenario_idx    = stream.scenario_idx;

		stream_info.ip_diff = 0;
		stream_info.stub = stream.stub;

		stream_info.idr_input_mode = pq_dev->display_info.idr.input_mode;

		stream_info.idr_input_path = _mtk_pq_common_set_pqu_idr_input_path_meta(
								stream.pq_idr_input_path);
		stream_info.idr_sw_mode_supported = pqdev->pqcaps.u32Idr_SwMode_Support;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"idr_input_mode:%d, idr_input_path:%d\n",
			stream_info.idr_input_mode, stream_info.idr_input_path);

		/* meta get pq buf info */
		for (buf_idx = PQU_BUF_SCMI; buf_idx < PQU_BUF_MAX; buf_idx++) {
			mtk_get_pq_buffer(pq_dev, buf_idx, &pq_buf);

			if (pq_buf.valid == true) {
				for (buf_ch = PQU_BUF_CH_0; buf_ch < PQU_BUF_CH_MAX; buf_ch++) {
					stream_info.pq_buf[buf_idx].addr[buf_ch] =
									pq_buf.addr_ch[buf_ch];
					stream_info.pq_buf[buf_idx].size[buf_ch] =
									pq_buf.size_ch[buf_ch];
					stream_info.pq_buf[buf_idx].frame_num =
									pq_buf.frame_num;
				}

				if (buf_idx == PQU_BUF_SCMI) {
					stream_info.opf_diff += pq_buf.frame_diff;
					stream_info.opb_diff += pq_buf.frame_diff;
					stream_info.op2_diff += pq_buf.frame_diff;
					stream_info.scmi_enable = true;
				} else if (buf_idx == PQU_BUF_UCM) {
					stream_info.opb_diff += pq_buf.frame_diff;
					stream_info.op2_diff += pq_buf.frame_diff;
					stream_info.ucm_enable = true;
				}
			} else {
				/* clear plane info */
				memset(&(stream_info.pq_buf[buf_idx]),
							0, sizeof(struct buffer_info));
			}
		}

		pq_dev->common_info.op2_diff = stream_info.op2_diff;
		stream_info.pixel_align = pqdev->display_dev.pixel_align;
		stream_info.h_max_size = pqdev->display_dev.h_max_size;
		stream_info.v_max_size = pqdev->display_dev.v_max_size;
		stream_info.ED_clock = pqdev->pqcaps.u32XC_ED_TARGET_CLK;
		stream_info.FN_clock = pqdev->pqcaps.u32XC_FN_TARGET_CLK;
		stream_info.FS_clock = pqdev->pqcaps.u32XC_FS_TARGET_CLK;
		stream_info.FD_clock = pqdev->pqcaps.u32XC_FD_TARGET_CLK;
		stream_info.NREnginePhase = pqdev->pqcaps.u32NREnginePhase;
		stream_info.MDWEnginePhase = pqdev->pqcaps.u32MDWEnginePhase;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"[FC] FN_clock = %ld, FS_clock = %ld, NREnginePhase = %d, MDWEnginePhase = %d",
			stream_info.FN_clock, stream_info.FS_clock,
			stream_info.NREnginePhase, stream_info.MDWEnginePhase);

		stream_info.znr_me_h_max_size = pqdev->display_dev.znr_me_h_max_size;
		stream_info.znr_me_v_max_size = pqdev->display_dev.znr_me_v_max_size;

		ret = mtk_pq_common_write_metadata_addr(&pqu_meta_buf,
			EN_PQ_METATAG_PQU_STREAM_INFO, (void *)&stream_info);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk-pq write EN_PQ_METATAG_PQU_STREAM_INFO Failed, ret=%d\n", ret);
			goto put_dma_buf;
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
				"mtk-pq write EN_PQ_METATAG_PQU_STREAM_INFO success\n");
		}

		stream_debug_info.cmdq_timeout = false;

		ret = mtk_pq_common_write_metadata_addr(&pqu_meta_buf,
			EN_PQ_METATAG_PQU_DEBUG_INFO, (void *)&stream_debug_info);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk-pq write EN_PQ_METATAG_PQU_DEBUG_INFO Failed, ret=%d\n", ret);
			goto put_dma_buf;
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
				"mtk-pq write EN_PQ_METATAG_PQU_DEBUG_INFO success\n");
		}
	}

	msg_info->win_id = pq_dev->dev_indx;
	msg_info->stream_meta_va = (u64)(pq_dev->pqu_stream_meta.va);
	msg_info->stream_meta_pa = (u64)(pq_dev->pqu_stream_meta.pa);
	msg_info->stream_meta_size = (u32)(pq_dev->pqu_stream_meta.size);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"meta_va = 0x%llx, meta_iova = 0x%lx, meta_size = 0x%x!\n",
		msg_info->stream_meta_va,
		msg_info->stream_meta_pa,
		msg_info->stream_meta_size);

	/* send new win cmd to pqu */
	if (bPquEnable) {
		ret = pqu_video_stream_on(msg_info, &stream_on_callback_info);
	} else
		pqu_msg_send(PQU_MSG_SEND_STREAM_ON, (void *)msg_info);

	pq_dev->stream_on_ref++;

	if (V4L2_TYPE_IS_OUTPUT(type))
		pq_dev->event_flag &= ~PQU_INPUT_STREAM_OFF_DONE;
	else
		pq_dev->event_flag &= ~PQU_OUTPUT_STREAM_OFF_DONE;

	return ret;

put_dma_buf:
	if (pq_dev->stream_on_ref == 0)
		mtk_pq_common_put_dma_buf(&pq_dev->stream_meta);

	return ret;
}

int mtk_pq_common_stream_off(struct mtk_pq_device *pq_dev,
	int type, struct msg_stream_off_info *msg_info)
{
	int ret = 0;
	unsigned long timeout = 0;
	struct v4l2_event ev;
	struct mtk_pq_ctx *pq_ctx = NULL;
	struct v4l2_m2m_buffer *src_buf = NULL;
	struct v4l2_m2m_buffer *dst_buf = NULL;
	struct mtk_pq_buffer *pq_src_buf = NULL;
	struct mtk_pq_buffer *pq_dst_buf = NULL;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"enter common stream off, dev_indx=%u, buffer type = %d!\n",
		pq_dev->dev_indx, type);

	if (!pq_dev || !msg_info) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	memset(&ev, 0, sizeof(struct v4l2_event));

	msg_info->win_id = pq_dev->dev_indx;

	pq_ctx = pq_dev->m2m.ctx;

	pq_ctx->state = MTK_STATE_ABORT;

	if (V4L2_TYPE_IS_OUTPUT(type)) {
		//dequeue input buffer in rdy_queue
		ev.type = V4L2_EVENT_MTK_PQ_INPUT_DONE;
		v4l2_m2m_for_each_src_buf(pq_ctx->m2m_ctx, src_buf) {
			if (src_buf == NULL) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Input Buffer Is NULL!\n");
				break;
			}

			pq_src_buf = (struct mtk_pq_buffer *)src_buf;

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
				"[input] frame fd = %d, meta fd = %d, index = %d!\n",
				pq_src_buf->frame_buf.fd, pq_src_buf->meta_buf.fd,
				pq_src_buf->vb.vb2_buf.index);

			vb2_buffer_done(&src_buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
			ev.u.data[0] = src_buf->vb.vb2_buf.index;
			v4l2_event_queue(&(pq_dev->video_dev), &ev);
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Send DeQueue Input Buffer Event!\n");
		}
		INIT_LIST_HEAD(&pq_ctx->m2m_ctx->out_q_ctx.rdy_queue);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Clear Input Ready Queue Done!\n");
	} else {
		//dequeue output buffer in rdy_queue
		ev.type = V4L2_EVENT_MTK_PQ_OUTPUT_DONE;
		v4l2_m2m_for_each_dst_buf(pq_ctx->m2m_ctx, dst_buf) {
			if (dst_buf == NULL) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Output Buffer Is NULL!\n");
				break;
			}

			pq_dst_buf = (struct mtk_pq_buffer *)dst_buf;

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
				"[output] frame fd = %d, meta fd = %d, index = %d!\n",
				pq_dst_buf->frame_buf.fd, pq_dst_buf->meta_buf.fd,
				pq_dst_buf->vb.vb2_buf.index);

			vb2_buffer_done(&dst_buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
			ev.u.data[0] = dst_buf->vb.vb2_buf.index;
			v4l2_event_queue(&(pq_dev->video_dev), &ev);
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Send DeQueue Output Buffer Event!\n");
		}
		INIT_LIST_HEAD(&pq_ctx->m2m_ctx->cap_q_ctx.rdy_queue);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Clear Output Ready Queue Done!\n");
	}

	if (bPquEnable) {
		struct callback_info stream_off_callback_info = {0};

		stream_off_callback_info.callback = mtk_pq_common_stream_off_cb;
		ret = pqu_video_stream_off(msg_info, &stream_off_callback_info);
		if (ret != 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pqu_video_stream_off fail(%d)!\n", ret);
			return -EINVAL;
		}
	} else {
		pqu_msg_send(PQU_MSG_SEND_STREAM_OFF, (void *)msg_info);
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Waiting for PQU stream off done!\n");

	timeout = PQU_TIME_OUT_MS;

	if (V4L2_TYPE_IS_OUTPUT(type)) {
		ret = wait_event_timeout(pq_dev->wait,
			pq_dev->event_flag & PQU_INPUT_STREAM_OFF_DONE, timeout);
		if (ret == 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Wait PQU input stream off time out(%ums)!\n", PQU_TIME_OUT_MS);
			return -ETIMEDOUT;
		}

		pq_dev->event_flag &= ~PQU_INPUT_STREAM_OFF_DONE;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "PQU input stream off done!\n");
	} else {
		ret = wait_event_timeout(pq_dev->wait,
			pq_dev->event_flag & PQU_OUTPUT_STREAM_OFF_DONE, timeout);
		if (ret == 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Wait PQU output stream off time out(%ums)!\n", PQU_TIME_OUT_MS);
			return -ETIMEDOUT;
		}

		pq_dev->event_flag &= ~PQU_OUTPUT_STREAM_OFF_DONE;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "PQU output stream off done!\n");
	}

	return 0;
}

static int _mtk_pq_common_stream_off_cb_memctrl(
	struct mtk_pq_device *pq_dev, int buf_type)
{
	int ret = 0;

	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Received NULL pointer!\n");
		return -EINVAL;
	}

	if (buf_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		//before release buffer, set all pq trigger gen to sw mode
		ret = _mtk_pq_common_set_all_sw_usr_mode(true);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_common_set_all_sw_usr_mode fail, Buffer Type=%d, ret=%d\n",
				buf_type, ret);
			return ret;
		}

#if IS_ENABLED(CONFIG_OPTEE)
		/* svp out stream off process start */
		ret = mtk_pq_svp_out_streamoff(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_svp_out_streamoff fail! ret=%d\n", ret);
			return ret;
		}
		/* svp out stream off process end */
#endif

		ret = mtk_pq_buffer_release(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq buf release fail!\n");
			return ret;
		}
	} else {
#if IS_ENABLED(CONFIG_OPTEE)
		/* svp cap stream off process start */
		ret = mtk_pq_svp_cap_streamoff(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_svp_cap_streamoff fail! ret=%d\n", ret);
			return ret;
		}
		/* svp cap stream off process end */
#endif
	}

	return ret;
}

/* pqu stream off cb */
static void mtk_pq_common_stream_off_cb(
			int error_cause,
			pid_t thread_id,
			uint32_t user_param,
			void *param)
{
	int ret = 0;
	int type;
	struct msg_stream_off_done_info *msg_info = NULL;
	struct mtk_pq_device *pq_dev = NULL;
	struct file *file = NULL;
	struct meta_pq_stream_internal_info internal_info;
	struct meta_buffer meta_buf;
	struct m_pq_common_debug_info stream_debug_info;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "enter stream off cb!\n");

	if (!param) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}

	memset(&internal_info, 0, sizeof(struct meta_pq_stream_internal_info));
	memset(&meta_buf, 0, sizeof(struct meta_buffer));
	memset(&stream_debug_info, 0, sizeof(struct m_pq_common_debug_info));

	msg_info = (struct msg_stream_off_done_info *)param;

	if (msg_info->win_id >= pqdev->usable_win) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid Window ID::%d!\n", msg_info->win_id);
		return;
	}

	pq_dev = pqdev->pq_devs[msg_info->win_id];
	if (!pq_dev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_dev in NULL!\n");
		return;
	}

	if (!pq_dev->ref_count) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Window (%d) is not open!\n", msg_info->win_id);
		return;
	}

	if (bPquEnable) {
		meta_buf.paddr = (unsigned char *)pq_dev->pqu_stream_meta.va;
		meta_buf.size = (unsigned int)pq_dev->pqu_stream_meta.size;
		if (msg_info->stream_meta_pa != pq_dev->pqu_stream_meta.pa) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"(stream_meta_pa=0x%llx) != (pqu_stream_meta.pa=0x%llx)\n",
				msg_info->stream_meta_pa, pq_dev->pqu_stream_meta.pa);
		}
	} else {
		meta_buf.paddr = (unsigned char *)msg_info->stream_meta_va;
		meta_buf.size = (unsigned int)msg_info->stream_meta_size;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"meta_va = 0x%llx, meta_iova = 0x%lx, meta_size = 0x%x!\n",
		msg_info->stream_meta_va,
		msg_info->stream_meta_pa,
		msg_info->stream_meta_size);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "meta_buf.paddr = 0x%llx, meta_buf.size = 0x%x!\n",
		meta_buf.paddr, meta_buf.size);

	if (msg_info->stream_type == PQU_MSG_BUF_TYPE_INPUT)
		type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	else
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "buffer type = %d!\n", type);

	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_PQU_DEBUG_INFO, (void *)&stream_debug_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_PQU_DEBUG_INFO Failed, ret = %d\n", ret);
		return;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"%s%llx\n",
		"cmdq_timeout = 0x", stream_debug_info.cmdq_timeout);

	g_stream_debug_info.cmdq_timeout = stream_debug_info.cmdq_timeout;

	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_STREAM_INTERNAL_INFO, (void *)&internal_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_STREAM_INTERNAL_INFO Failed, ret = %d\n", ret);
		return;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_STREAM_INTERNAL_INFO success\n");
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"%s%llx, %s%llx\n",
		"file = 0x", internal_info.file,
		"pq_dev = 0x", internal_info.pq_dev);

	file = (struct file *)(internal_info.file);
	pq_dev = (struct mtk_pq_device *)(internal_info.pq_dev);

	ret = v4l2_m2m_streamoff(file, pq_dev->m2m.ctx->m2m_ctx, type);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Stream off fail!Buffer Type = %d, ret=%d\n", type, ret);
		return;
	}

	if (pq_dev->stream_on_ref > 0)
		pq_dev->stream_on_ref--;
	if (pq_dev->stream_on_ref == 0) {
		mtk_pq_common_put_dma_buf(&pq_dev->stream_meta);
	}

	ret = _mtk_pq_common_stream_off_cb_memctrl(pq_dev, type);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_mtk_pq_common_stream_off_cb_memctrl fail, Buffer Type=%d, ret=%d\n",
			type, ret);
		return;
	}

	if (V4L2_TYPE_IS_OUTPUT(type))
		pq_dev->event_flag |= PQU_INPUT_STREAM_OFF_DONE;
	else
		pq_dev->event_flag |= PQU_OUTPUT_STREAM_OFF_DONE;

	wake_up(&(pq_dev->wait));
}

int mtk_pq_common_qbuf(struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf,
	struct msg_queue_info *msg_info)
{
	int ret = 0;
	struct meta_buffer meta_buf;
	struct callback_info queue_frame_callback_info = {0};

	memset(&meta_buf, 0, sizeof(struct meta_buffer));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"enter common qbuf, dev_indx=%u, buffer type = %d!\n",
		pq_dev->dev_indx, pq_buf->vb.vb2_buf.type);

	if (!pq_dev || !pq_buf || !msg_info) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	meta_buf.paddr = (unsigned char *)pq_buf->meta_buf.va;
	meta_buf.size = (unsigned int)pq_buf->meta_buf.size;

	msg_info->win_id = (u8)(pq_dev->dev_indx);
	msg_info->frame_iova = (u64)pq_buf->frame_buf.iova;
	msg_info->meta_va = (u64)pq_buf->meta_buf.va;
	msg_info->meta_size = (u32)pq_buf->meta_buf.size;
	msg_info->priv = (u64)pq_buf;
	msg_info->extra_frame_num =
		(V4L2_TYPE_IS_OUTPUT(pq_buf->vb.vb2_buf.type)) ?
		(pq_dev->dv_win_info.common.extra_frame_num) : 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"meta_va=0x%llx,meta_size=0x%x,priv=0x%llx,frame_iova=0x%lx,extra=%d\n",
		msg_info->meta_va,
		msg_info->meta_size,
		msg_info->priv,
		msg_info->frame_iova,
		msg_info->extra_frame_num);

#ifdef PQU_PATTERN_TEST
	if (pq_dev->trigger_pattern) {
		ret = mtk_pq_common_write_metadata_addr(&meta_buf,
			EN_PQ_METATAG_PQU_PATTERN_INFO, (void *)&pq_dev->pattern_info);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN,
				"mtk-pq write EN_PQ_METATAG_PQU_PATTERN_INFO Failed,ret=%d\n", ret);
			//return ret;
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
				"mtk-pq write EN_PQ_METATAG_PQU_PATTERN_INFO success\n");
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN, "Write Pattern Metada Success!\n");
		pq_dev->trigger_pattern = false;
	}
#endif
	dma_buf_end_cpu_access(pq_buf->meta_buf.db, DMA_BIDIRECTIONAL);
	if (bPquEnable) {
		unsigned long pa = 0;
		void *remap_va = NULL;

		if (alloc_pool(&pa, msg_info->meta_size)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "alloc_pool fail\n");
			return -ENOMEM;
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "alloc_pool pa = 0x%llx\n", pa);

		remap_va = ioremap_nocache(pa, msg_info->meta_size);
		if (remap_va == NULL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "ioremap_nocache fail\n");
			return -EIO;
		}

		memcpy_toio(remap_va, msg_info->meta_va, msg_info->meta_size);
		iounmap(remap_va);
		msg_info->meta_pa = (u64)pa;
		ret = pqu_video_queue_frame(msg_info, &queue_frame_callback_info);
	} else {
		pqu_msg_send(PQU_MSG_SEND_QUEUE, (void *)msg_info);
	}
	pq_buf->queue_time = _mtk_pq_common_get_ms_time();

	return ret;
}

/* pqu dqbuf cb */
static void mtk_pq_common_dqbuf_cb(
			int error_cause,
			pid_t thread_id,
			uint32_t user_param,
			void *param)
{
	int ret = 0;
	int type = 0;
	struct msg_dqueue_info *msg_info = (struct msg_dqueue_info *)param;
	enum mtk_pq_input_source_type input_source;
	struct meta_buffer meta_buf;
	struct v4l2_event ev;
	struct mtk_pq_ctx *ctx = NULL;
	struct mtk_pq_device *pq_dev = NULL;
	struct mtk_pq_buffer *pq_buf = NULL;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "enter dqbuf cb!\n");

	if (!param) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}

	memset(&meta_buf, 0, sizeof(struct meta_buffer));
	memset(&ev, 0, sizeof(struct v4l2_event));

	if (msg_info->win_id >= pqdev->usable_win) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid Window ID::%d!\n", msg_info->win_id);
		return;
	}

	pq_dev = pqdev->pq_devs[msg_info->win_id];
	if (!pq_dev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_dev in NULL!\n");
		return;
	}

	if (!pq_dev->ref_count) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Window (%d) is not open!\n", msg_info->win_id);
		return;
	}

	if (bPquEnable) {
		void *da = ioremap_nocache(msg_info->meta_va, msg_info->meta_size);

		if (da) {
			memcpy_fromio(msg_info->meta_host_va, da, msg_info->meta_size);
			iounmap(da);
			meta_buf.paddr = (unsigned char *)msg_info->meta_host_va;
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "ioremap_nocache fail\n");
			return;
		}
		free_pool(msg_info->meta_va, msg_info->meta_size);
	} else {
		meta_buf.paddr = (unsigned char *)msg_info->meta_va;
	}
	meta_buf.size = (unsigned int)msg_info->meta_size;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"meta_va = 0x%llx, meta_size = 0x%x, priv = 0x%llx !\n",
		msg_info->meta_va,
		msg_info->meta_size,
		msg_info->priv);

	pq_buf = (struct mtk_pq_buffer *)(msg_info->priv);
	if ((!pq_buf) || (!pq_buf->meta_buf.db)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_buf is NULL\n");
		return;
	}

	pq_buf->dequeue_time = _mtk_pq_common_get_ms_time();
	pq_buf->process_time = pq_buf->dequeue_time - pq_buf->queue_time;

	input_source = pq_dev->common_info.input_source;

	ret = dma_buf_begin_cpu_access(pq_buf->meta_buf.db, DMA_BIDIRECTIONAL);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "dma_buf_begin_cpu_access fail\n");
		goto exit;
	}

	if (msg_info->frame_type == PQU_MSG_BUF_TYPE_INPUT) {
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
		PQ_ATRACE_INT_FORMAT(pq_buf->process_time,
			"pq_dd_inq_proc_time_%d", msg_info->win_id);
		PQ_ATRACE_INT_FORMAT(pq_buf->vb.vb2_buf.index,
			"7-E: pq_dqbuf_in_idx_%d", msg_info->win_id);
#endif
		type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		ev.type = V4L2_EVENT_MTK_PQ_INPUT_DONE;
	} else if (msg_info->frame_type == PQU_MSG_BUF_TYPE_OUTPUT) {
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
		PQ_ATRACE_INT_FORMAT(pq_buf->process_time,
			"pq_dd_outq_proc_time_%d", msg_info->win_id);
		PQ_ATRACE_INT_FORMAT(pq_buf->vb.vb2_buf.index,
			"26-E: pq_dqbuf_out_idx_%d", msg_info->win_id);
#endif
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		ev.type = V4L2_EVENT_MTK_PQ_OUTPUT_DONE;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"error frame type::%d!\n",
				msg_info->frame_type);
		goto exit;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "buffer type = %d!\n", pq_buf->vb.vb2_buf.type);

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		ret = _mtk_pq_common_set_output_dqbuf_meta(msg_info->win_id, meta_buf);
		if (ret) {
			goto exit;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		ret = _mtk_pq_common_set_capture_dqbuf_meta(msg_info->win_id, meta_buf);
		if (ret) {
			goto exit;
		}
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid buffer type!\n");
		goto exit;
	}
exit:
	vb2_buffer_done(&pq_buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
	ev.u.data[0] = pq_buf->vb.vb2_buf.index;
	v4l2_event_queue(&(pq_dev->video_dev), &ev);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "queue buffer done event, buffer type = %d!\n", type);
#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	switch (ev.type) {
	case V4L2_EVENT_MTK_PQ_INPUT_DONE:
		PQ_ATRACE_INT_FORMAT(pq_buf->vb.vb2_buf.index,
			"8-S: pq_dqbuf_event_in_idx_%d", msg_info->win_id);
		break;

	case V4L2_EVENT_MTK_PQ_OUTPUT_DONE:
		PQ_ATRACE_INT_FORMAT(pq_buf->vb.vb2_buf.index,
			"27-S: pq_dqbuf_event_out_idx_%d", msg_info->win_id);
		break;
	default:
		break;
	}
#endif
}

int mtk_pq_common_close(struct mtk_pq_device *pq_dev,
	struct msg_destroy_win_info *msg_info)
{
	int ret = 0;
	unsigned int timeout = PQU_TIME_OUT_MS;
	struct msg_destroy_win_done_info reply_info = {0};

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"enter common close, dev_indx=%u!\n", pq_dev->dev_indx);

	if (!pq_dev || !msg_info) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	msg_info->win_id = pq_dev->dev_indx;

	if (bPquEnable) {
		ret = pqu_video_destroy_win(msg_info, &reply_info);
	} else {
		pqu_msg_send(PQU_MSG_SEND_DESTROY_WIN, (void *)msg_info);

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Waiting for PQU close win done!\n");

		ret = wait_event_timeout(pq_dev->wait,
			pq_dev->event_flag & PQU_CLOSE_WIN_DONE, timeout);
		if (ret == 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Wait PQU close win time out(%ums)!\n", PQU_TIME_OUT_MS);
			return -ETIMEDOUT;
		}

		pq_dev->event_flag &= ~PQU_CLOSE_WIN_DONE;
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "PQU close win done!\n");

	return 0;
}

static void mtk_pq_common_close_cb(
			int error_cause,
			pid_t thread_id,
			uint32_t user_param,
			void *param)
{
	struct msg_destroy_win_done_info *msg_info = NULL;
	struct mtk_pq_device *pq_dev = NULL;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "enter close win cb!\n");

	if (!param) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}

	msg_info = (struct msg_destroy_win_done_info *)param;

	if (msg_info->win_id >= pqdev->usable_win) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid Window ID::%d!\n", msg_info->win_id);
		return;
	}

	pq_dev = pqdev->pq_devs[msg_info->win_id];
	if (!pq_dev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_dev in NULL!\n");
		return;
	}

	if (!pq_dev->ref_count) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Window (%d) is already closed!\n", msg_info->win_id);
		return;
	}

	pq_dev->event_flag |= PQU_CLOSE_WIN_DONE;
	wake_up(&(pq_dev->wait));
}

int mtk_pq_common_config(struct msg_config_info *msg_info, bool is_pqu)
{
	int ret = 0;

	if (!msg_info) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "enter common config!\n");

	if (is_pqu) {
		ret = pqu_video_update_config(msg_info);
	} else {
		pqu_msg_send(PQU_MSG_SEND_CONFIG, (void *)msg_info);
	}

	return ret;
}

static int _mtk_pq_common_set_stream_metadata(struct mtk_pq_device *pq_dev, void *ctrl)
{
	int ret = 0;

	pq_dev->stream_meta.fd = ((struct v4l2_pq_stream_meta_info *)ctrl)->meta_fd;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "stream meta fd = %d\n",
		pq_dev->stream_meta.fd);

	mtk_pq_buffer_get_hwmap_info(pq_dev);

	return ret;
}

static int _mtk_pq_common_get_stream_metadata(struct mtk_pq_device *pq_dev, void *ctrl)
{
	int ret = 0;

	((struct v4l2_pq_stream_meta_info *)ctrl)->meta_fd = pq_dev->stream_meta.fd;

	return ret;
}

static int _common_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev;
	int ret = 0;
	u32 source = 0;

	if (!ctrl)
		return -ENOMEM;
	pq_dev = container_of(ctrl->handler, struct mtk_pq_device,
		common_ctrl_handler);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "%s: ctrl id = 0x%x.\n",
		__func__, ctrl->id);

	source = pq_dev->common_info.input_source;

	switch (ctrl->id) {
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
	case V4L2_CID_SET_STREAMMETA_DATA:
		ret = _mtk_pq_common_get_stream_metadata(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_STREAM_DEBUG:
		ret = 0;
		memcpy(ctrl->p_new.p, &g_stream_debug_info, sizeof(struct m_pq_common_debug_info));
		break;
	case V4L2_CID_PQ_G_BUFFER_OUT_INFO:
		if (IS_INPUT_SRCCAP(source))
			ret = mtk_pq_display_idr_queue_setup_info(pq_dev, ctrl->p_new.p);
		else if (IS_INPUT_B2R(source))
			ret = mtk_pq_display_b2r_queue_setup_info(pq_dev, ctrl->p_new.p);
		else
			ret = -EINVAL;
		break;
	case V4L2_CID_PQ_G_BUFFER_CAP_INFO:
		ret = mtk_pq_display_mdw_queue_setup_info(pq_dev, ctrl->p_new.p);
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
	int ret = 0;

	if (!ctrl)
		return -ENOMEM;
	pq_dev = container_of(ctrl->handler, struct mtk_pq_device,
		common_ctrl_handler);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "%s: ctrl id = 0x%x.\n",
		__func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		ret = _mtk_pq_common_set_hflip(pq_dev, ctrl->p_new.p_u8);
		break;
	case V4L2_CID_VFLIP:
		ret = _mtk_pq_common_set_vflip(pq_dev, ctrl->p_new.p_u8);
		break;
	case V4L2_CID_FORCE_P_MODE:
		ret = _mtk_pq_common_set_forcep(pq_dev, (bool)ctrl->cur.val);
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
	case V4L2_CID_SET_STREAMMETA_DATA:
		ret = _mtk_pq_common_set_stream_metadata(pq_dev, ctrl->p_new.p);
		break;
	case V4L2_CID_SET_INPUT_MODE:
		ret = _mtk_pq_common_set_input_mode(pq_dev, ctrl->p_new.p_u8);
		break;
	case V4L2_CID_SET_OUTPUT_MODE:
		ret = _mtk_pq_common_set_output_mode(pq_dev, ctrl->p_new.p_u8);
		break;
	case V4L2_CID_SET_FLOW_CONTROL:
		ret = _mtk_pq_common_set_flow_control(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_SET_PQ_MEMORY_INDEX:
		ret = _mtk_pq_common_set_pq_memory_index(pq_dev, ctrl->p_new.p_u8);
		break;
	case V4L2_CID_SET_AISR_ACTIVE_WIN:
		ret = _mtk_pq_common_set_aisr_active_win(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_PQ_S_BUFFER_INFO:
		ret = _mtk_pq_common_set_pix_format_info(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_SET_STAGE_PATTERN:
		ret = _mtk_pq_common_set_stage_pattern(pq_dev, (void *)ctrl->p_new.p_u8);
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
		.id = V4L2_CID_HFLIP,
		.name = "hflip enable",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 1,
		.step = 1,
		.dims = {sizeof(u8)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_VFLIP,
		.name = "vflip enable",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 1,
		.step = 1,
		.dims = {sizeof(u8)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SET_STREAMMETA_DATA,
		.name = "Stream metadata",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_stream_meta_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SET_INPUT_MODE,
		.name = "set pq output mode",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = MTK_PQ_INPUT_MODE_MAX,
		.step = 1,
		.dims = {sizeof(u8)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SET_OUTPUT_MODE,
		.name = "set pq output mode",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = MTK_PQ_OUTPUT_MODE_MAX,
		.step = 1,
		.dims = {sizeof(u8)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SET_FLOW_CONTROL,
		.name = "set flow control",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_flow_control)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SET_PQ_MEMORY_INDEX,
		.name = "set pq memory index",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(u8)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SET_AISR_ACTIVE_WIN,
		.name = "set aisr active win",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_aisr_active_win)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_STREAM_DEBUG,
		.name = "Stream debug",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_stream_debug)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_PQ_S_BUFFER_INFO,
		.name = "set buffer info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_s_buffer_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_PQ_G_BUFFER_OUT_INFO,
		.name = "get buffer info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_g_buffer_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_PQ_G_BUFFER_CAP_INFO,
		.name = "get buffer info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_pq_g_buffer_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &common_ctrl_ops,
		.id = V4L2_CID_SET_STAGE_PATTERN,
		.name = "set stage pattern",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct stage_pattern_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
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

	if (!bPquEnable) {
		/* register stream off done event */
		pqu_msg_register_notify_func(PQU_MSG_REPLY_STREAM_OFF, mtk_pq_common_stream_off_cb);
		/* register dqbuf ready event */
		pqu_msg_register_notify_func(PQU_MSG_REPLY_DQUEUE, mtk_pq_common_dqbuf_cb);
		/* register close window done event */
		pqu_msg_register_notify_func(PQU_MSG_REPLY_DESTROY_WIN, mtk_pq_common_close_cb);
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

static int _mtk_pq_common_set_input_source_select(bool stub)
{
	int ret = 0;
	struct pq_common_triggen_input_src_sel paramIn;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramIn, 0, sizeof(struct pq_common_triggen_input_src_sel));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	out.reg = reg;

	//set input source select
	paramIn.domain = PQ_COMMON_TRIGEN_DOMAIN_B2R;
	paramIn.src = PQ_COMMON_TRIGEN_INPUT_TGEN;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_inputSrcSel(paramIn, true, &out, stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_inputSrcSel failed!\n",
			__func__);
		return ret;
	}

	paramIn.domain = PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1;
	paramIn.src = PQ_COMMON_TRIGEN_INPUT_TGEN;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_inputSrcSel(paramIn, true, &out, stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_inputSrcSel failed!\n",
			__func__);
		return ret;
	}

	paramIn.domain = PQ_COMMON_TRIGEN_DOMAIN_IP;
	paramIn.src = PQ_COMMON_TRIGEN_INPUT_TGEN;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_inputSrcSel(paramIn, true, &out, stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_inputSrcSel failed!\n",
			__func__);
		return ret;
	}

	paramIn.domain = PQ_COMMON_TRIGEN_DOMAIN_OP1;
	paramIn.src = PQ_COMMON_TRIGEN_INPUT_TGEN;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_inputSrcSel(paramIn, true, &out, stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_inputSrcSel failed!\n",
			__func__);
		return ret;
	}

	paramIn.domain = PQ_COMMON_TRIGEN_DOMAIN_OP2;
	paramIn.src = PQ_COMMON_TRIGEN_INPUT_TGEN;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_inputSrcSel(paramIn, true, &out, stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_inputSrcSel failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_htt_size(void)
{
	int ret = 0;
	struct pq_common_triggen_sw_htt_size paramInSwHtt;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramInSwHtt, 0, sizeof(struct pq_common_triggen_sw_htt_size));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	out.reg = reg;

	//set sw htt
	paramInSwHtt.domain = PQ_COMMON_TRIGEN_DOMAIN_B2R;
	paramInSwHtt.sw_htt = TRIG_SW_HTT;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swHttSize(paramInSwHtt, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swHttSize failed!\n",
			__func__);
		return ret;
	}

	paramInSwHtt.domain = PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1;
	paramInSwHtt.sw_htt = TRIG_SW_HTT;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swHttSize(paramInSwHtt, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swHttSize failed!\n",
			__func__);
		return ret;
	}

	paramInSwHtt.domain = PQ_COMMON_TRIGEN_DOMAIN_IP;
	paramInSwHtt.sw_htt = TRIG_SW_HTT;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swHttSize(paramInSwHtt, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swHttSize failed!\n",
			__func__);
		return ret;
	}

	paramInSwHtt.domain = PQ_COMMON_TRIGEN_DOMAIN_OP1;
	paramInSwHtt.sw_htt = TRIG_SW_HTT;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swHttSize(paramInSwHtt, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swHttSize failed!\n",
			__func__);
		return ret;
	}

	paramInSwHtt.domain = PQ_COMMON_TRIGEN_DOMAIN_OP2;
	paramInSwHtt.sw_htt = TRIG_SW_HTT;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swHttSize(paramInSwHtt, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swHttSize failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_sw_user_mode(void)
{
	int ret = 0;
	struct pq_common_triggen_sw_user_mode paramInSwUserMode;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramInSwUserMode, 0, sizeof(struct pq_common_triggen_sw_user_mode));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	out.reg = reg;

	//set sw user mode
	paramInSwUserMode.domain = PQ_COMMON_TRIGEN_DOMAIN_B2R;
	paramInSwUserMode.sw_user_mode_h = true;
	paramInSwUserMode.sw_user_mode_v = true;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swUserMode(paramInSwUserMode, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swUserMode failed!\n",
			__func__);
		return ret;
	}

	paramInSwUserMode.domain = PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1;
	paramInSwUserMode.sw_user_mode_h = true;
	paramInSwUserMode.sw_user_mode_v = true;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swUserMode(paramInSwUserMode, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swUserMode failed!\n",
			__func__);
		return ret;
	}

	paramInSwUserMode.domain = PQ_COMMON_TRIGEN_DOMAIN_IP;
	paramInSwUserMode.sw_user_mode_h = true;
	paramInSwUserMode.sw_user_mode_v = true;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swUserMode(paramInSwUserMode, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swUserMode failed!\n",
			__func__);
		return ret;
	}

	paramInSwUserMode.domain = PQ_COMMON_TRIGEN_DOMAIN_OP1;
	paramInSwUserMode.sw_user_mode_h = true;
	paramInSwUserMode.sw_user_mode_v = true;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swUserMode(paramInSwUserMode, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swUserMode failed!\n",
			__func__);
		return ret;
	}

	paramInSwUserMode.domain = PQ_COMMON_TRIGEN_DOMAIN_OP2;
	paramInSwUserMode.sw_user_mode_h = true;
	paramInSwUserMode.sw_user_mode_v = true;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_swUserMode(paramInSwUserMode, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_swUserMode failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_vcount_keep_enable(void)
{
	int ret = 0;
	struct pq_common_triggen_vcnt_keep_enable paramInVcntKeepEn;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramInVcntKeepEn, 0, sizeof(struct pq_common_triggen_vcnt_keep_enable));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	out.reg = reg;

	//set vcnt keep enable
	paramInVcntKeepEn.domain = PQ_COMMON_TRIGEN_DOMAIN_B2R;
	paramInVcntKeepEn.vcnt_keep_en = true;
	paramInVcntKeepEn.vcnt_upd_mask_range = TRIG_VCNT_UPD_MSK_RNG;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_vcntKeepEnable(paramInVcntKeepEn, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vcntKeepEnable failed!\n",
			__func__);
		return ret;
	}

	paramInVcntKeepEn.domain = PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1;
	paramInVcntKeepEn.vcnt_keep_en = true;
	paramInVcntKeepEn.vcnt_upd_mask_range = TRIG_VCNT_UPD_MSK_RNG;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_vcntKeepEnable(paramInVcntKeepEn, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vcntKeepEnable failed!\n",
			__func__);
		return ret;
	}

	paramInVcntKeepEn.domain = PQ_COMMON_TRIGEN_DOMAIN_IP;
	paramInVcntKeepEn.vcnt_keep_en = true;
	paramInVcntKeepEn.vcnt_upd_mask_range = TRIG_VCNT_UPD_MSK_RNG;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_vcntKeepEnable(paramInVcntKeepEn, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vcntKeepEnable failed!\n",
			__func__);
		return ret;
	}

	paramInVcntKeepEn.domain = PQ_COMMON_TRIGEN_DOMAIN_OP1;
	paramInVcntKeepEn.vcnt_keep_en = true;
	paramInVcntKeepEn.vcnt_upd_mask_range = TRIG_VCNT_UPD_MSK_RNG;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_vcntKeepEnable(paramInVcntKeepEn, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vcntKeepEnable failed!\n",
			__func__);
		return ret;
	}

	paramInVcntKeepEn.domain = PQ_COMMON_TRIGEN_DOMAIN_OP2;
	paramInVcntKeepEn.vcnt_keep_en = true;
	paramInVcntKeepEn.vcnt_upd_mask_range = TRIG_VCNT_UPD_MSK_RNG;

	/* call hwreg to get addr/mask */
	ret = drv_hwreg_pq_common_triggen_set_vcntKeepEnable(paramInVcntKeepEn, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vcntKeepEnable failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_vsync_delay_trigger(void)
{
	int ret = 0;
	struct pq_common_triggen_vs_dly paramInVsDelay;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramInVsDelay, 0, sizeof(struct pq_common_triggen_vs_dly));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	out.reg = reg;

	paramInVsDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_B2R;
	paramInVsDelay.vs_dly_h = TRIG_VS_DLY_H;
	paramInVsDelay.vs_dly_v = TRIG_VS_DLY_V;
	paramInVsDelay.vs_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set vsync delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_vsyncTrig(paramInVsDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vsyncTrig failed!\n",
			__func__);
		return ret;
	}

	paramInVsDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1;
	paramInVsDelay.vs_dly_h = TRIG_VS_DLY_H;
	paramInVsDelay.vs_dly_v = TRIG_VS_DLY_V;
	paramInVsDelay.vs_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set vsync delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_vsyncTrig(paramInVsDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vsyncTrig failed!\n",
			__func__);
		return ret;
	}

	paramInVsDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_IP;
	paramInVsDelay.vs_dly_h = TRIG_VS_DLY_H;
	paramInVsDelay.vs_dly_v = TRIG_VS_DLY_V;
	paramInVsDelay.vs_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set vsync delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_vsyncTrig(paramInVsDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vsyncTrig failed!\n",
			__func__);
		return ret;
	}

	paramInVsDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_OP1;
	paramInVsDelay.vs_dly_h = TRIG_VS_DLY_H;
	paramInVsDelay.vs_dly_v = TRIG_VS_DLY_V;
	paramInVsDelay.vs_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set vsync delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_vsyncTrig(paramInVsDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vsyncTrig failed!\n",
			__func__);
		return ret;
	}

	paramInVsDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_OP2;
	paramInVsDelay.vs_dly_h = TRIG_VS_DLY_H;
	paramInVsDelay.vs_dly_v = TRIG_VS_DLY_V;
	paramInVsDelay.vs_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set vsync delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_vsyncTrig(paramInVsDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_vsyncTrig failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_dma_rd_trigger(void)
{
	int ret = 0;
	struct pq_common_triggen_dma_r_dly paramInDmaRDelay;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramInDmaRDelay, 0, sizeof(struct pq_common_triggen_dma_r_dly));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	out.reg = reg;

	paramInDmaRDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_B2R;
	paramInDmaRDelay.dma_r_dly_h = TRIG_DMA_R_DLY_H;
	paramInDmaRDelay.dma_r_dly_v = TRIG_DMA_R_DLY_V;
	paramInDmaRDelay.dmar_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set dma_rd trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_dmaRdTrig(paramInDmaRDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_dmaRdTrig failed!\n",
			__func__);
		return ret;
	}

	paramInDmaRDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1;
	paramInDmaRDelay.dma_r_dly_h = TRIG_DMA_R_DLY_H;
	paramInDmaRDelay.dma_r_dly_v = TRIG_DMA_R_DLY_V;
	paramInDmaRDelay.dmar_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set dma_rd trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_dmaRdTrig(paramInDmaRDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_dmaRdTrig failed!\n",
			__func__);
		return ret;
	}

	paramInDmaRDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_IP;
	paramInDmaRDelay.dma_r_dly_h = TRIG_DMA_R_DLY_H;
	paramInDmaRDelay.dma_r_dly_v = TRIG_DMA_R_DLY_V;
	paramInDmaRDelay.dmar_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set dma_rd trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_dmaRdTrig(paramInDmaRDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_dmaRdTrig failed!\n",
			__func__);
		return ret;
	}

	paramInDmaRDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_OP1;
	paramInDmaRDelay.dma_r_dly_h = TRIG_DMA_R_DLY_H;
	paramInDmaRDelay.dma_r_dly_v = TRIG_DMA_R_DLY_V;
	paramInDmaRDelay.dmar_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set dma_rd trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_dmaRdTrig(paramInDmaRDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_dmaRdTrig failed!\n",
			__func__);
		return ret;
	}

	paramInDmaRDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_OP2;
	paramInDmaRDelay.dma_r_dly_h = TRIG_DMA_R_DLY_H;
	paramInDmaRDelay.dma_r_dly_v = TRIG_DMA_R_DLY_V;
	paramInDmaRDelay.dmar_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set dma_rd trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_dmaRdTrig(paramInDmaRDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_dmaRdTrig failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

static int _mtk_pq_common_set_str_delay_trigger(void)
{
	int ret = 0;
	struct pq_common_triggen_str_dly paramInStrDelay;
	struct reg_info reg[REG_SIZE];
	struct hwregOut out;

	memset(&paramInStrDelay, 0, sizeof(struct pq_common_triggen_str_dly));
	memset(reg, 0, sizeof(reg));
	memset(&out, 0, sizeof(out));

	out.reg = reg;

	paramInStrDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_B2R;
	paramInStrDelay.str_dly_v = TRIG_STR_DLY_V;
	paramInStrDelay.str_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set str delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_strTrig(paramInStrDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_strTrig failed!\n",
			__func__);
		return ret;
	}

	paramInStrDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_B2RLITE1;
	paramInStrDelay.str_dly_v = TRIG_STR_DLY_V;
	paramInStrDelay.str_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set str delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_strTrig(paramInStrDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_strTrig failed!\n",
			__func__);
		return ret;
	}

	paramInStrDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_IP;
	paramInStrDelay.str_dly_v = TRIG_STR_DLY_V;
	paramInStrDelay.str_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set str delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_strTrig(paramInStrDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_strTrig failed!\n",
			__func__);
		return ret;
	}

	paramInStrDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_OP1;
	paramInStrDelay.str_dly_v = TRIG_STR_DLY_V;
	paramInStrDelay.str_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set str delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_strTrig(paramInStrDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_strTrig failed!\n",
			__func__);
		return ret;
	}

	paramInStrDelay.domain = PQ_COMMON_TRIGEN_DOMAIN_OP2;
	paramInStrDelay.str_dly_v = TRIG_STR_DLY_V;
	paramInStrDelay.str_trig_sel = PQ_COMMON_TRIGEN_TRIG_SEL_VS;

	/* set str delay trigger gen */
	ret = drv_hwreg_pq_common_triggen_set_strTrig(paramInStrDelay, true, &out);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: drv_hwreg_pq_common_triggen_set_strTrig failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

int mtk_pq_common_trigger_gen_init(bool stub)
{
	int ret = 0;

	ret = _mtk_pq_common_set_input_source_select(stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: _mtk_pq_common_set_input_source_select failed!\n",
			__func__);
		return ret;
	}

	if (stub)
		return 0;

	ret = _mtk_pq_common_set_htt_size();
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: _mtk_pq_common_set_htt_size failed!\n",
			__func__);
		return ret;
	}

	ret = _mtk_pq_common_set_sw_user_mode();
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: _mtk_pq_common_set_sw_user_mode failed!\n",
			__func__);
		return ret;
	}

	ret = _mtk_pq_common_set_vcount_keep_enable();
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: _mtk_pq_common_set_vcount_keep_enable failed!\n",
			__func__);
		return ret;
	}

	/* If DS/ML can't finish before vsync, need to adjust following triggers*/
	ret = _mtk_pq_common_set_vsync_delay_trigger();
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: _mtk_pq_common_set_vsync_delay_trigger failed!\n",
			__func__);
		return ret;
	}

	ret = _mtk_pq_common_set_dma_rd_trigger();
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: _mtk_pq_common_set_dma_rd_trigger failed!\n",
			__func__);
		return ret;
	}

	ret = _mtk_pq_common_set_str_delay_trigger();
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: _mtk_pq_common_set_str_delay_trigger failed!\n",
			__func__);
		return ret;
	}

	return 0;
}

int mtk_pq_common_suspend(bool stub)
{
	int ret = 0;

	ret = mtk_pq_common_irq_suspend(stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: mtk_pq_common_irq_resume failed!\n",
			__func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_pq_common_suspend);

int mtk_pq_common_resume(bool stub, uint32_t u32IRQ_Version)
{
	int ret = 0;

	ret = mtk_pq_common_trigger_gen_init(stub);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: mtk_pq_common_trigger_gen_init failed!\n",
			__func__);
		return ret;
	}

	ret = mtk_pq_common_irq_resume(stub, u32IRQ_Version);
	if (ret < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: mtk_pq_common_irq_resume failed!\n",
			__func__);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_pq_common_resume);
