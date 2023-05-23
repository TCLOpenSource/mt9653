// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-----------------------------------------------------------------------------
// Include Files
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
#include <linux/platform_device.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "mtk_srccap.h"

#include "metadata_utility.h"
#include "linux/metadata_utility/m_srccap.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
static int dv_meta_map_fd(int fd, void **va, u64 *size)
{
	struct dma_buf *db = NULL;

	if ((fd == 0) || (va == NULL) || (size == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (*va != NULL) {
		SRCCAP_MSG_ERROR("occupied va\n");
		return -EINVAL;
	}

	db = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(db)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	*size = db->size;

	*va = dma_buf_vmap(db);
	if (IS_ERR_OR_NULL(*va)) {
		SRCCAP_MSG_ERROR("kernel address not valid\n");
		dma_buf_put(db);
		return -ENXIO;
	}

	dma_buf_put(db);
	SRCCAP_MSG_INFO("va=0x%llx, size=%llu\n", *va, *size);

	return 0;
}

static int dv_meta_unmap_fd(int fd, void *va)
{
	struct dma_buf *db = NULL;

	if ((fd == 0) || (va == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	db = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(db)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	dma_buf_vunmap(db, va);
	dma_buf_put(db);

	return 0;
}

static int dv_meta_get_header(
	enum srccap_dv_meta_tag tag,
	struct meta_header *header)
{
	if (header == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (tag) {
	case SRCCAP_DV_META_TAG_DV_INFO:
		strncpy(header->tag, META_SRCCAP_DV_INFO_TAG, TAG_LENGTH);
		header->ver = META_SRCCAP_DV_INFO_VERSION;
		header->size = sizeof(struct meta_srccap_dv_info);
		break;
	default:
		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
			"invalid metadata tag: %d\n", tag);
		return -EINVAL;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------
int mtk_dv_meta_dqbuf(
	struct srccap_dv_dqbuf_in *in,
	struct srccap_dv_dqbuf_out *out)
{
	int ret = 0;
	struct meta_srccap_dv_info *meta_dv_info = NULL;
	u8 index;
	enum srccap_dv_descrb_interface interface = SRCCAP_DV_DESCRB_INTERFACE_NONE;
	uint32_t data_len = 0;
	uint32_t mem_size = 0;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "meta dequeue buffer\n");

	if ((in == NULL) || (out == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	meta_dv_info = kzalloc(sizeof(struct meta_srccap_dv_info), GFP_KERNEL);
	if (meta_dv_info == NULL) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	memset(meta_dv_info, 0, sizeof(struct meta_srccap_dv_info));

	meta_dv_info->common.path = in->dev->dev_id;
	meta_dv_info->common.dv_src_hw_version =
		in->dev->srccap_info.cap.u32DV_Srccap_HWVersion;
	meta_dv_info->common.hdmi_422_pack_en =
		in->dev->dv_info.descrb_info.common.hdmi_422_pack_en;

	interface = in->dev->dv_info.descrb_info.common.interface;
	if ((interface < SRCCAP_DV_DESCRB_INTERFACE_NONE)
		|| (interface >= SRCCAP_DV_DESCRB_INTERFACE_MAX))
		goto exit;

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "interface: %d\n", interface);

	/* set metadata */
	mem_size = sizeof(meta_dv_info->meta.data);
	switch (interface) {
	case SRCCAP_DV_DESCRB_INTERFACE_SINK_LED:
		index = (in->dev->dv_info.descrb_info.buf.index) ? (0) : (1);
		data_len = in->dev->dv_info.descrb_info.buf.data_length[index];
		/* check data length */
		if (data_len > mem_size) {
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
				"meta length %d > memory size %d\n",
				data_len, mem_size);
			ret = -ENOMEM;
			goto exit;
		}
		memcpy(meta_dv_info->meta.data,
			in->dev->dv_info.descrb_info.buf.data[index],
			data_len);
		meta_dv_info->meta.data_length = data_len;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_RGB:
	case SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_YUV:
		data_len = SRCCAP_DV_DESCRB_VSIF_SIZE;
		/* check data length */
		if (data_len > mem_size) {
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
				"meta length %d > memory size %d\n",
				data_len, mem_size);
			ret = -ENOMEM;
			goto exit;
		}
		memcpy(meta_dv_info->meta.data,
			in->dev->dv_info.descrb_info.pkt_info.vsif,
			data_len);
		meta_dv_info->meta.data_length = data_len;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_YUV:
	case SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_RGB:
		data_len = SRCCAP_DV_DESCRB_DRM_SIZE;
		/* check data length */
		if (data_len > mem_size) {
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
				"meta length %d > memory size %d\n",
				data_len, mem_size);
			ret = -ENOMEM;
			goto exit;
		}
		memcpy(meta_dv_info->meta.data,
			in->dev->dv_info.descrb_info.pkt_info.drm,
			data_len);
		meta_dv_info->meta.data_length = data_len;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_FORM_1:
	case SRCCAP_DV_DESCRB_INTERFACE_FORM_2_RGB:
	case SRCCAP_DV_DESCRB_INTERFACE_FORM_2_YUV:
		data_len = in->dev->dv_info.descrb_info.pkt_info.vsem_size;
		/* check data length */
		if (data_len > sizeof(in->dev->dv_info.descrb_info.pkt_info.vsem)) {
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
				"vsem length %d > vsem memory size %d\n",
				data_len,
				sizeof(in->dev->dv_info.descrb_info.pkt_info.vsem));
			ret = -EPERM;
			goto exit;
		}
		if (data_len > mem_size) {
			SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
				"meta length %d > memory size %d\n",
				data_len, mem_size);
			ret = -ENOMEM;
			goto exit;
		}
		memcpy(meta_dv_info->meta.data,
			in->dev->dv_info.descrb_info.pkt_info.vsem,
			data_len);
		meta_dv_info->meta.data_length = data_len;
		break;
	default:
		goto exit;
	}

	/* convert interface enum */
	switch (interface) {
	case SRCCAP_DV_DESCRB_INTERFACE_SINK_LED:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_SINK_LED;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_RGB:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_SOURCE_LED_RGB;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_SOURCE_LED_YUV:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_SOURCE_LED_YUV;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_RGB:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_DRM_SOURCE_LED_RGB;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_DRM_SOURCE_LED_YUV:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_DRM_SOURCE_LED_YUV;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_FORM_1:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_FORM_1;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_FORM_2_RGB:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_FORM_2_RGB;
		break;
	case SRCCAP_DV_DESCRB_INTERFACE_FORM_2_YUV:
		meta_dv_info->descrb.interface = M_DV_INTERFACE_FORM_2_YUV;
		break;
	default:
		goto exit;
	}

	/* convert dma status enum */
	switch (in->dev->dv_info.dma_info.dma_status) {
	case SRCCAP_DV_DMA_STATUS_DISABLE:
		meta_dv_info->dma.status = M_DV_DMA_STATUS_DISABLE;
		break;
	case SRCCAP_DV_DMA_STATUS_ENABLE_FB:
		meta_dv_info->dma.status = M_DV_DMA_STATUS_ENABLE_FB;
		break;
	case SRCCAP_DV_DMA_STATUS_ENABLE_FBL:
		meta_dv_info->dma.status = M_DV_DMA_STATUS_ENABLE_FBL;
		break;
	default:
		goto exit;
	}

	/* set dv dma information */
	meta_dv_info->dma.w_index = in->dev->dv_info.dma_info.w_index;
	meta_dv_info->dma.width = in->dev->dv_info.dma_info.mem_fmt.dma_width;
	meta_dv_info->dma.height = in->dev->dv_info.dma_info.mem_fmt.dma_height;

	ret = mtk_dv_meta_write(
		in->buf->m.planes[in->buf->length - 1].m.fd,
		SRCCAP_DV_META_TAG_DV_INFO,
		meta_dv_info);
	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_INFO, "meta dequeue buffer done\n");

exit:
	if (meta_dv_info != NULL)
		kfree(meta_dv_info);
	return ret;
}

int mtk_dv_meta_read(
	int fd,
	enum srccap_dv_meta_tag tag,
	void *content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (content == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = dv_meta_map_fd(fd, &va, &size);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = dv_meta_get_header(tag, &header);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;

	meta_ret = query_metadata_header(&buffer, &header);
	if (!meta_ret) {
		SRCCAP_MSG_ERROR("metadata header not found\n");
		ret = -EPERM;
		goto EXIT;
	}

	meta_ret = query_metadata_content(&buffer, &header, content);
	if (!meta_ret) {
		SRCCAP_MSG_ERROR("metadata content not found\n");
		ret = -EPERM;
		goto EXIT;
	}

EXIT:
	dv_meta_unmap_fd(fd, va);
	return ret;
}

int mtk_dv_meta_write(
	int fd,
	enum srccap_dv_meta_tag tag,
	void *content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (content == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = dv_meta_map_fd(fd, &va, &size);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = dv_meta_get_header(tag, &header);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;

	meta_ret = attach_metadata(&buffer, header, content);
	if (!meta_ret) {
		SRCCAP_MSG_ERROR("metadata not attached\n");
		ret = -EPERM;
		goto EXIT;
	}

EXIT:
	dv_meta_unmap_fd(fd, va);
	return ret;
}

int mtk_dv_meta_remove(
	int fd,
	enum srccap_dv_meta_tag tag)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = dv_meta_map_fd(fd, &va, &size);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = dv_meta_get_header(tag, &header);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;

	meta_ret = delete_metadata(&buffer, &header);
	if (!meta_ret) {
		SRCCAP_MSG_ERROR("metadata not deleted\n");
		ret = -EPERM;
		goto EXIT;
	}

EXIT:
	dv_meta_unmap_fd(fd, va);
	return ret;
}

//-----------------------------------------------------------------------------
// Debug Functions
//-----------------------------------------------------------------------------
