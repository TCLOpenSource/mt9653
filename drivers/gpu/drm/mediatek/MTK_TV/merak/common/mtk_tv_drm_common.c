// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "mtk_tv_drm_common.h"
#include "mtk_tv_drm_log.h"
#include "hwreg_common.h"
#include "hwreg_render_stub.h"
#include "mtk_tv_drm_kms.h"
#include "m_pqu_render.h"
#include "m_pqu_pq.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_COMMON

static int _mtk_render_common_map_db(struct dma_buf *db, void **va, u64 *size)
{
	int ret = 0;
	unsigned long long iova = 0;

	if ((db == NULL) || (va == NULL) || (size == NULL)) {
		DRM_ERROR("[%s][%d] Invalid input, db=0x%llx, va=0x%llx, size=0x%llx\n",
			__func__, __LINE__, db, va, size);
		return -EINVAL;
	}

	if (*va != NULL) {
		kfree(*va);
		*va = NULL;
	}

	*size = db->size;

	*va = dma_buf_vmap(db);
	if (IS_ERR_OR_NULL(*va)) {
		if (IS_ERR(*va)) {
			DRM_ERROR("[%s][%d] dma_buf_vmap fail, db=%p, errno = %d\n",
				__func__, __LINE__, db, PTR_ERR(*va));
			return PTR_ERR(*va);
		}

		DRM_ERROR("[%s][%d] dma_buf_vmap fail, db=%p, null ptr!\n",
			__func__, __LINE__, db);
		return -EINVAL;
	}

	DRM_INFO("[%s][%d] va=0x%llx, size=%llu\n",
			__func__, __LINE__, *va, *size);
	return ret;
}

static int _mtk_render_common_unmap_db(struct dma_buf *db, void *va)
{
	if ((db == NULL) || (va == NULL)) {
		DRM_ERROR("[%s][%d] Invalid input, db=0x%llx, va=0x%llx\n",
			__func__, __LINE__, db, va);
		return -EINVAL;
	}

	dma_buf_vunmap(db, va);

	return 0;
}

static int _mtk_render_common_get_metaheader(
	enum mtk_render_common_metatag meta_tag,
	struct meta_header *meta_header)
{
	int ret = 0;

	if (meta_header == NULL) {
		DRM_ERROR("[%s][%d] Invalid input, meta_header is NULL, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		return -EINVAL;
	}

	switch (meta_tag) {
	case RENDER_METATAG_FRAME_INFO:
		DRM_INFO("[%s][%d] Now this metadata tag:(%d) is unused\n",
			__func__, __LINE__, meta_tag);
		ret = -EINVAL;
		break;
	case RENDER_METATAG_SVP_INFO:
		meta_header->size = sizeof(struct meta_pq_disp_svp);
		meta_header->ver = META_PQ_DISP_SVP_VERSION;
		strncpy(meta_header->tag, PQ_DISP_SVP_META_TAG, TAG_LENGTH);
		break;
	case RENDER_METATAG_FRAMESYNC_INFO:
		meta_header->size = sizeof(struct meta_pq_framesync_info);
		meta_header->ver = META_PQ_FRAMESYNC_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_FRAMESYNC_INFO_TAG, TAG_LENGTH);
		break;
	case RENDER_METATAG_IDR_CTRL_INFO:
		meta_header->size = sizeof(struct meta_pq_display_idr_ctrl);
		meta_header->ver = META_PQ_DISPLAY_IDR_CTRL_VERSION;
		strncpy(meta_header->tag, META_PQ_DISPLAY_IDR_CTRL_META_TAG, TAG_LENGTH);
		break;
	case RENDER_METATAG_DV_OUTPUT_FRAME_INFO:
		meta_header->size = sizeof(struct m_pq_dv_out_frame_info);
		meta_header->ver = M_PQ_DV_OUTPUT_FRAME_INFO_VERSION;
		strncpy(meta_header->tag, M_PQ_DV_OUTPUT_FRAME_INFO_TAG, TAG_LENGTH);
		break;
	case RENDER_METATAG_PQDD_DISPLAY_INFO:
		meta_header->size = sizeof(struct meta_pq_display_flow_info);
		meta_header->ver = META_PQ_DISPLAY_FLOW_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_DISPLAY_FLOW_INFO_TAG, TAG_LENGTH);
		break;
	case RENDER_METATAG_PQDD_OUTPUT_FRAME_INFO:
		meta_header->size = sizeof(struct meta_pq_output_frame_info);
		meta_header->ver = META_PQ_OUTPUT_FRAME_INFO_VERSION;
		strncpy(meta_header->tag, META_PQ_OUTPUT_FRAME_INFO_TAG, TAG_LENGTH);
		break;
	case RENDER_METATAG_FRC_SCALING_INFO:
		meta_header->size = sizeof(struct m_pq_display_frc_scaling);
		meta_header->ver = M_PQ_DISPLAY_FRC_SCALING_VERSION;
		strncpy(meta_header->tag, M_PQ_DISPLAY_FRC_SCALING_TAG, TAG_LENGTH);
		break;
	case RENDER_METATAG_PQU_DISPLAY_INFO:
		meta_header->size = sizeof(struct m_pq_display_flow_ctrl);
		meta_header->ver = M_PQ_DISPLAY_FLOW_CTRL_VERSION;
		strncpy(meta_header->tag, M_PQ_DISPLAY_FLOW_CTRL_META_TAG, TAG_LENGTH);
		break;
	default:
		DRM_INFO("[%s][%d] Invalid metadata tag:(%d)\n",
			__func__, __LINE__, meta_tag);
		ret = -EINVAL;
		break;
	}
	return ret;
}

int mtk_render_common_read_metadata(struct dma_buf *meta_db,
	enum mtk_render_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	DRM_INFO("[%s][%d] read db=0x%llx meta_tag:%d\n", __func__, __LINE__, meta_db, meta_tag);

	if (meta_content == NULL) {
		DRM_ERROR("[%s][%d] Invalid input, meta_content is NULL, db=0x%llx, meta_tag:%d\n",
			__func__, __LINE__, meta_db, meta_tag);
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_render_common_map_db(meta_db, &va, &size);
	if (ret) {
		DRM_ERROR("[%s][%d] trans fd to va fail, db=0x%llx, meta_tag:%d\n",
			__func__, __LINE__, meta_db, meta_tag);
		goto out;
	}

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_ERROR("[%s][%d] get meta header fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = query_metadata_header(&buffer, &header);
	if (!meta_ret) {
		DRM_ERROR("[%s][%d] query metadata header fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		ret = -EPERM;
		goto out;
	}

	meta_ret = query_metadata_content(&buffer, &header, meta_content);
	if (!meta_ret) {
		DRM_ERROR("[%s][%d] query metadata content fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_render_common_unmap_db(meta_db, va);
	return ret;
}

int mtk_render_common_write_metadata(struct dma_buf *meta_db,
	enum mtk_render_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	DRM_INFO("[%s][%d] write db=0x%llx meta_tag:%d\n", __func__, __LINE__, meta_db, meta_tag);

	if (meta_content == NULL) {
		DRM_ERROR("[%s][%d] Invalid input, meta_content is NULL, db=0x%llx, meta_tag:%d\n",
			__func__, __LINE__, meta_db, meta_tag);
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_render_common_map_db(meta_db, &va, &size);
	if (ret) {
		DRM_ERROR("[%s][%d] trans fd to va fail, db=0x%llx, meta_tag:%d\n",
			__func__, __LINE__, meta_db, meta_tag);
		goto out;
	}

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_ERROR("[%s][%d] get meta header fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = attach_metadata(&buffer, header, meta_content);
	if (!meta_ret) {
		DRM_ERROR("[%s][%d] attach metadata fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_render_common_unmap_db(meta_db, va);
	return ret;
}

int mtk_render_common_delete_metadata(struct dma_buf *meta_db,
	enum mtk_render_common_metatag meta_tag)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_render_common_map_db(meta_db, &va, &size);
	if (ret) {
		DRM_ERROR("[%s][%d] trans fd to va fail, db=0x%llx, meta_tag:%d\n",
			__func__, __LINE__, meta_db, meta_tag);
		goto out;
	}

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_ERROR("[%s][%d] get meta header fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = delete_metadata(&buffer, &header);
	if (!meta_ret) {
		DRM_INFO("[%s][%d] delete metadata fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_render_common_unmap_db(meta_db, va);
	return ret;
}

int mtk_render_common_write_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_render_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	struct meta_header header;

	if (meta_content == NULL) {
		DRM_ERROR("[%s][%d] Invalid input, meta_content is NULL, meta_tag:%d\n\n",
			__func__, __LINE__, meta_tag);
		ret =  -EINVAL;
		goto out;
	}

	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_ERROR("[%s][%d] get meta header fail, meta_tag:%d\n\n",
			__func__, __LINE__, meta_tag);
		ret = -EINVAL;
		goto out;
	}

	meta_ret = attach_metadata(meta_buf, header, meta_content);
	if (!meta_ret) {
		DRM_ERROR("[%s][%d] attach metadata fail, meta_tag:%d\n\n",
			__func__, __LINE__, meta_tag);
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}

int mtk_render_common_read_metadata_addr(struct meta_buffer *meta_buf,
	enum mtk_render_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	struct meta_header header;

	if (meta_content == NULL) {
		DRM_ERROR("[%s][%d] Invalid input, meta_content is NULL, meta_tag:%d\n\n",
			__func__, __LINE__, meta_tag);
		ret =  -EINVAL;
		goto out;
	}

	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_ERROR("[%s][%d] get meta header fail, meta_tag:%d\n\n",
			__func__, __LINE__, meta_tag);
		ret = -EINVAL;
		goto out;
	}

	meta_ret = query_metadata_header(meta_buf, &header);
	if (!meta_ret) {
		DRM_ERROR("[%s][%d] query metadata header fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		ret = -EINVAL;
		goto out;
	}

	meta_ret = query_metadata_content(meta_buf, &header, meta_content);
	if (!meta_ret) {
		DRM_ERROR("[%s][%d] query metadata content fail, meta_tag:%d\n",
			__func__, __LINE__, meta_tag);
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}

int mtk_render_common_set_stub_mode(struct mtk_tv_drm_crtc *crtc,
	struct drm_mtk_tv_common_stub *stubInfo)
{
	int ret = 0;
	struct mtk_tv_kms_context *pctx = NULL;

	if (!crtc || !stubInfo) {
		DRM_ERROR("[%s][%d] null ptr, crtc=%p, stubInfo=%p\n",
			__func__, __LINE__, crtc, stubInfo);
		return -EINVAL;
	}

	DRM_INFO("[%s,%d] stub = %d\n", __func__, __LINE__, stubInfo->stub);
	ret = drv_hwreg_common_set_stub(stubInfo->stub);
	if (ret < 0) {
		DRM_ERROR("[%s][%d] set stub fail\n",
			__func__, __LINE__);
		return ret;
	}

	ret = drv_hwreg_render_stub_set_mode(stubInfo->stub);
	if (ret < 0) {
		DRM_ERROR("[%s][%d] set render stub fail\n", __func__, __LINE__);
		return ret;
	}

	pctx = (struct mtk_tv_kms_context *)crtc->crtc_private;
	pctx->stub = stubInfo->stub;
	pctx->pnlgamma_stubmode = stubInfo->pnlgamma_stubmode;

	return ret;
}
