// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "mtk_tv_drm_common.h"

static int _mtk_render_common_map_fd(int fd, void **va, u64 *size)
{
	int ret = 0;
	struct dma_buf *db = NULL;
	unsigned long long iova = 0;

	if ((size == NULL) || (fd == 0)) {
		DRM_INFO("[%s][%d] Invalid input, fd=(%d), size is NULL?(%s)\n",
			__func__, __LINE__, fd, (size == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	if (*va != NULL) {
		kfree(*va);
		*va = NULL;
	}

	db = dma_buf_get(fd);
	if (db == NULL) {
		DRM_INFO("[%s][%d] dma_buf_get fail\n",
			__func__, __LINE__);
		return -EPERM;
	}

	*size = db->size;

	ret = dma_buf_begin_cpu_access(db, DMA_BIDIRECTIONAL);
	if (ret < 0) {
		DRM_INFO("[%s][%d] dma_buf_begin_cpu_access fail\n",
			__func__, __LINE__);
		dma_buf_put(db);
		return -EPERM;
	}
	*va = dma_buf_vmap(db);
	if (!*va || (*va == -1)) {
		dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
		DRM_INFO("[%s][%d] dma_buf_vmap fail\n",
			__func__, __LINE__);
		dma_buf_put(db);
		return -EPERM;
	}

	dma_buf_put(db);
	DRM_INFO("[%s][%d] va=0x%llx, size=%llu\n",
			__func__, __LINE__, *va, *size);
	return ret;
}

static int _mtk_render_common_unmap_fd(int fd, void *va)
{
	struct dma_buf *db = NULL;

	if ((va == NULL) || (fd == 0)) {
		DRM_INFO("[%s][%d] Invalid input, fd=(%d), va is NULL?(%s)\n",
			__func__, __LINE__, fd, (va == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	db = dma_buf_get(fd);
	if (db == NULL) {
		DRM_INFO("[%s][%d] dma_buf_get fail\n",
			__func__, __LINE__);
		return -1;
	}

	dma_buf_vunmap(db, va);
	dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
	dma_buf_put(db);
	return 0;
}

static int _mtk_render_common_get_metaheader(
	enum mtk_render_common_metatag meta_tag,
	struct meta_header *meta_header)
{
	int ret = 0;

	if (meta_header == NULL) {
		DRM_INFO("[%s][%d] Invalid input, meta_header == NULL\n",
			__func__, __LINE__);
		return -EPERM;
	}

	switch (meta_tag) {
	case RD_METATAG_FRM_INFO:
		meta_header->size = sizeof(struct m_rd_display_frm_info);
		meta_header->ver = M_RD_DISP_FRM_INFO_VERSION;
		strncpy(meta_header->tag, M_RD_DISP_FRM_INFO_TAG, TAG_LENGTH);
		break;
	case RD_METATAG_SVP_INFO:
		meta_header->size = sizeof(struct meta_pq_disp_svp);
		meta_header->ver = META_PQ_DISP_SVP_VERSION;
		strncpy(meta_header->tag, PQ_DISP_SVP_META_TAG, TAG_LENGTH);
		break;
	default:
		DRM_INFO("[%s][%d] Invalid metadata tag:(%d)\n",
			__func__, __LINE__, meta_tag);
		ret = -EPERM;
		break;
	}
	return ret;
}

int mtk_render_common_read_metadata(int meta_fd,
	enum mtk_render_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (meta_content == NULL) {
		DRM_INFO("[%s][%d] Invalid input, meta_content=NULL\n",
			__func__, __LINE__);
		return -EPERM;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_render_common_map_fd(meta_fd, &va, &size);
	if (ret) {
		DRM_INFO("[%s][%d] trans fd to va fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_INFO("[%s][%d] get meta header fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = query_metadata_header(&buffer, &header);
	if (!meta_ret) {
		DRM_INFO("[%s][%d] query metadata header fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

	meta_ret = query_metadata_content(&buffer, &header, meta_content);
	if (!meta_ret) {
		DRM_INFO("[%s][%d] query metadata content fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_render_common_unmap_fd(meta_fd, va);
	return ret;
}

int mtk_render_common_write_metadata(int meta_fd,
	enum mtk_render_common_metatag meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (meta_content == NULL) {
		DRM_INFO("[%s][%d] Invalid input, meta_content=NULL\n",
			__func__, __LINE__);
		return -EPERM;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _mtk_render_common_map_fd(meta_fd, &va, &size);
	if (ret) {
		DRM_INFO("[%s][%d] trans fd to va fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_INFO("[%s][%d] get meta header fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = attach_metadata(&buffer, header, meta_content);
	if (!meta_ret) {
		DRM_INFO("[%s][%d] attach metadata fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_render_common_unmap_fd(meta_fd, va);
	return ret;
}

int mtk_render_common_delete_metadata(int meta_fd,
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

	ret = _mtk_render_common_map_fd(meta_fd, &va, &size);
	if (ret) {
		DRM_INFO("[%s][%d] trans fd to va fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

	ret = _mtk_render_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DRM_INFO("[%s][%d] get meta header fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = delete_metadata(&buffer, &header);
	if (!meta_ret) {
		DRM_INFO("[%s][%d] delete metadata fail\n",
			__func__, __LINE__);
		ret = -EPERM;
		goto out;
	}

out:
	_mtk_render_common_unmap_fd(meta_fd, va);
	return ret;
}

