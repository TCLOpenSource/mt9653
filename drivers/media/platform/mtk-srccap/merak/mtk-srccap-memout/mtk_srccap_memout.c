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
#include <linux/delay.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <asm-generic/div64.h>

#include "linux/metadata_utility/m_srccap.h"
#include "mtk_srccap.h"
#include "mtk_srccap_memout.h"
#include "hwreg_srccap_memout.h"

#include "mtk_iommu_dtv_api.h"
#include "metadata_utility.h"

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
#include "mtk_srccap_atrace.h"
static int buf_cnt[SRCCAP_DEV_NUM] = {0};
#endif
#include "mapi_pq_cfd_if.h"
#include "drv_scriptmgt.h"


#define FMT_NUM        (4)
unsigned int fmt_map[FMT_NUM] = {    //refer to fmt_map_dv/mi_pq_cfd_fmt_map in mi_pq_apply_cfd.c
	V4L2_PIX_FMT_RGB_8_8_8_CE,
	V4L2_PIX_FMT_YUV422_Y_UV_8B_CE,
	V4L2_PIX_FMT_YUV444_Y_U_V_8B_CE,
	V4L2_PIX_FMT_YUV420_Y_UV_8B_CE};

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int memout_map_fd(int fd, void **va, u64 *size)
{
	struct dma_buf *db = NULL;

	if ((va == NULL) || (size == NULL)) {
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

	return 0;
}

static int memout_unmap_fd(int fd, void *va)
{
	struct dma_buf *db = NULL;

	if (va == NULL) {
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

static int memout_get_metaheader(
	enum srccap_memout_metatag meta_tag,
	struct meta_header *meta_header)
{
	if (meta_header == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (meta_tag) {
	case SRCCAP_MEMOUT_METATAG_FRAME_INFO:
		strncpy(meta_header->tag, META_SRCCAP_FRAME_INFO_TAG, TAG_LENGTH);
		meta_header->ver = META_SRCCAP_FRAME_INFO_VERSION;
		meta_header->size = sizeof(struct meta_srccap_frame_info);
		break;
	case SRCCAP_MEMOUT_METATAG_SVP_INFO:
		strncpy(meta_header->tag, META_SRCCAP_SVP_INFO_TAG, TAG_LENGTH);
		meta_header->ver = META_SRCCAP_SVP_INFO_VERSION;
		meta_header->size = sizeof(struct meta_srccap_svp_info);
		break;
	case SRCCAP_MEMOUT_METATAG_COLOR_INFO:
		strncpy(meta_header->tag, META_SRCCAP_COLOR_INFO_TAG, TAG_LENGTH);
		meta_header->ver = META_SRCCAP_COLOR_INFO_VERSION;
		meta_header->size = sizeof(struct meta_srccap_color_info);
		break;
	case SRCCAP_MEMOUT_METATAG_HDR_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDR_PKT_TAG, TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDR_PKT_VERSION;
		meta_header->size = sizeof(struct meta_srccap_hdr_pkt);
		break;
	case SRCCAP_MEMOUT_METATAG_HDR_VSIF_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDR_VSIF_PKT_TAG, TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDR_VSIF_PKT_VERSION;
		meta_header->size = sizeof(struct meta_srccap_hdr_vsif_pkt);
		break;
	case SRCCAP_MEMOUT_METATAG_HDR_EMP_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDR_EMP_PKT_TAG, TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDR_EMP_PKT_VERSION;
		meta_header->size = sizeof(struct meta_srccap_hdr_emp_pkt);
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_AVI_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_AVI_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_AVI_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_avi_packet_info) *
				    META_SRCCAP_HDMIRX_AVI_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_GCP_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_GCP_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_GCP_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_gc_packet_info) *
				META_SRCCAP_HDMIRX_GCP_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_SPD_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_SPD_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_SPD_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_packet_info) *
				    META_SRCCAP_HDMIRX_SPD_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_VSIF_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_VSIF_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_VSIF_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_vsif_packet_info) *
				    META_SRCCAP_HDMIRX_VSIF_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_HDR_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_HDR_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_HDR_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_vsif_packet_info) *
				    META_SRCCAP_HDMIRX_HDR_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_AUI_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_AUI_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_AUI_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_packet_info) *
				    META_SRCCAP_HDMIRX_AUI_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_MPG_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_MPG_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_MPG_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_packet_info) *
				    META_SRCCAP_HDMIRX_MPG_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_VS_EMP_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_VS_EMP_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_VS_EMP_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_emp_packet_info) *
				    META_SRCCAP_HDMIRX_VS_EMP_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_DSC_EMP_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_DSC_EMP_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_DSC_EMP_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_emp_packet_info) *
				    META_SRCCAP_HDMIRX_DSC_EMP_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_VRR_EMP_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_VRR_EMP_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_VRR_EMP_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_emp_packet_info) *
				    META_SRCCAP_HDMIRX_VRR_EMP_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_HDMIRX_DHDR_EMP_PKT:
		strncpy(meta_header->tag, META_SRCCAP_HDMIRX_DHDR_EMP_PKT_TAG,
			TAG_LENGTH);
		meta_header->ver = META_SRCCAP_HDMIRX_DHDR_EMP_PKT_VERSION;
		meta_header->size =
			1 + sizeof(struct hdmi_emp_packet_info) *
				    META_SRCCAP_HDMIRX_DHDR_EMP_PKT_MAX_SIZE;
		break;
	case SRCCAP_MEMOUT_METATAG_DIP_POINT_INFO:
		strncpy(meta_header->tag, META_SRCCAP_DIP_POINT_INFO_TAG, TAG_LENGTH);
		meta_header->ver = META_SRCCAP_DIP_POINT_INFO_VERSION;
		meta_header->size = sizeof(struct meta_srccap_dip_point_info);
		break;
	default:
		SRCCAP_MSG_ERROR("invalid metadata tag:%d\n", meta_tag);
		return -EINVAL;
	}

	return 0;
}

static int memout_map_source_meta(
	enum v4l2_srccap_input_source v4l2_src,
	enum m_input_source *meta_src)
{
	if (meta_src == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (v4l2_src) {
	case V4L2_SRCCAP_INPUT_SOURCE_NONE:
		*meta_src = SOURCE_NONE;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
		*meta_src = SOURCE_HDMI;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
		*meta_src = SOURCE_HDMI2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
		*meta_src = SOURCE_HDMI3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		*meta_src = SOURCE_HDMI4;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
		*meta_src = SOURCE_CVBS;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
		*meta_src = SOURCE_CVBS2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
		*meta_src = SOURCE_CVBS3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
		*meta_src = SOURCE_CVBS4;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
		*meta_src = SOURCE_CVBS5;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
		*meta_src = SOURCE_CVBS6;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
		*meta_src = SOURCE_CVBS7;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		*meta_src = SOURCE_CVBS8;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
		*meta_src = SOURCE_SVIDEO;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
		*meta_src = SOURCE_SVIDEO2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
		*meta_src = SOURCE_SVIDEO3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		*meta_src = SOURCE_SVIDEO4;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
		*meta_src = SOURCE_YPBPR;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
		*meta_src = SOURCE_YPBPR2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		*meta_src = SOURCE_YPBPR3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
		*meta_src = SOURCE_VGA;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
		*meta_src = SOURCE_VGA2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		*meta_src = SOURCE_VGA3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		*meta_src = SOURCE_ATV;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
		*meta_src = SOURCE_SCART;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		*meta_src = SOURCE_SCART2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
		*meta_src = SOURCE_DVI;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
		*meta_src = SOURCE_DVI2;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
		*meta_src = SOURCE_DVI3;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		*meta_src = SOURCE_DVI4;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int memout_map_path_meta(
	u16 dev_id,
	enum m_memout_path *meta_path)
{
	if (meta_path == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (dev_id) {
	case 0:
		*meta_path = PATH_IPDMA_0;
		break;
	case 1:
		*meta_path = PATH_IPDMA_1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int memout_map_ip_format(
	MS_HDMI_COLOR_FORMAT hdmi_fmt,
	enum reg_srccap_memout_format *ip_fmt)
{
	if (ip_fmt == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (hdmi_fmt) {
	case MS_HDMI_COLOR_RGB:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		break;
	case MS_HDMI_COLOR_YUV_422:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		break;
	case MS_HDMI_COLOR_YUV_444:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		break;
	case MS_HDMI_COLOR_YUV_420:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV420;
		break;
	default:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		break;
	}

	return 0;
}

static int memout_get_ip_fmt(
	enum v4l2_srccap_input_source src,
	MS_HDMI_COLOR_FORMAT hdmi_fmt,
	enum reg_srccap_memout_format *ip_fmt)
{
	if (ip_fmt == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
		*ip_fmt =
			(hdmi_fmt == MS_HDMI_COLOR_RGB) ?
			REG_SRCCAP_MEMOUT_FORMAT_RGB : REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int memout_set_frame_capturing_parameters(
	u8 dev_id,
	struct reg_srccap_memout_format_conversion fmt_conv,
	bool pack,
	bool argb,
	u8 bitmode[SRCCAP_MAX_PLANE_NUM],
	u8 ce[SRCCAP_MAX_PLANE_NUM],
	struct reg_srccap_memout_resolution res,
	u32 hoffset,
	u32 pitch[SRCCAP_MAX_PLANE_NUM])
{
	int ret = 0;

	ret = drv_hwreg_srccap_memout_set_format(dev_id, fmt_conv, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_pack(dev_id, pack, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_argb(dev_id, argb, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_bit_mode(dev_id, bitmode, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_compression(dev_id, ce, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_input_resolution(dev_id, res, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_line_offset(
		dev_id, hoffset, TRUE, NULL, NULL); // unit: pixel
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_frame_pitch(
		dev_id, pitch, TRUE, NULL, NULL); // unit: n bits, n = align
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int memout_init_buffer_queue(struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
	INIT_LIST_HEAD(&srccap_dev->memout_info.frame_q.inputq_head);
	INIT_LIST_HEAD(&srccap_dev->memout_info.frame_q.outputq_head);
	INIT_LIST_HEAD(&srccap_dev->memout_info.frame_q.processingq_head);
	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);

	return 0;
}

static void memout_deinit_buffer_queue(struct mtk_srccap_dev *srccap_dev)
{
	struct list_head *list_ptr = NULL, *tmp_ptr = NULL;
	struct srccap_memout_buffer_entry *entry = NULL;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);

	if ((srccap_dev->memout_info.frame_q.inputq_head.prev == NULL)
		|| (srccap_dev->memout_info.frame_q.inputq_head.next == NULL))
		SRCCAP_MSG_POINTER_CHECK();
	else
		list_for_each_safe(list_ptr, tmp_ptr,
				&srccap_dev->memout_info.frame_q.inputq_head) {
			entry = list_entry(list_ptr, struct srccap_memout_buffer_entry, list);
			list_del(&entry->list);
			kfree(entry);
		}

	if ((srccap_dev->memout_info.frame_q.outputq_head.prev == NULL)
		|| (srccap_dev->memout_info.frame_q.outputq_head.next == NULL))
		SRCCAP_MSG_POINTER_CHECK();
	else
		list_for_each_safe(list_ptr, tmp_ptr,
				&srccap_dev->memout_info.frame_q.outputq_head) {
			entry = list_entry(list_ptr, struct srccap_memout_buffer_entry, list);
			list_del(&entry->list);
			kfree(entry);
		}

	if ((srccap_dev->memout_info.frame_q.processingq_head.prev == NULL)
		|| (srccap_dev->memout_info.frame_q.processingq_head.next == NULL))
		SRCCAP_MSG_POINTER_CHECK();
	else
		list_for_each_safe(list_ptr, tmp_ptr,
				&srccap_dev->memout_info.frame_q.processingq_head) {
			entry = list_entry(list_ptr, struct srccap_memout_buffer_entry, list);
			list_del(&entry->list);
			kfree(entry);
		}
	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
}

static void memout_add_next_entry(
	struct list_head *head,
	struct srccap_memout_buffer_entry *entry)
{
	if ((head == NULL) || (entry == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	list_add_tail(&entry->list, head);
}

static void memout_add_last_entry(
	struct list_head *head,
	struct srccap_memout_buffer_entry *entry)
{
	if ((head == NULL) || (entry == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	list_add(&entry->list, head);
}

static struct srccap_memout_buffer_entry *memout_delete_last_entry(
	struct list_head *head)
{
	struct srccap_memout_buffer_entry *entry = NULL;

	if ((head == NULL) || (head->next == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return NULL;
	}

	if (head->next == head) {
		SRCCAP_MSG_ERROR("No entries in queue!\n");
		return NULL;
	}

	entry = list_first_entry(head, struct srccap_memout_buffer_entry, list);
	list_del(head->next);

	return entry;
}

static int memout_count_buffer_in_queue(
	struct list_head *head)
{
	struct list_head *list_ptr = NULL, *tmp_ptr = NULL;
	int count = 0;

	if (head == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return 0;
	}

	list_for_each_safe(list_ptr, tmp_ptr, head) {
		count++;
	}

	return count;
}

static bool memout_check_frame_or_field_appropriate_for_write(
	struct mtk_srccap_dev *srccap_dev,
	u8 src_field)
{
	int ret = 0;
	bool appropriate = FALSE, interlaced = FALSE, enough_buffer = FALSE, write_en = FALSE;
	u8 mode = 0;
	struct list_head *input_q = NULL;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return FALSE;
	}

	input_q = &srccap_dev->memout_info.frame_q.inputq_head;
	interlaced = srccap_dev->timingdetect_info.data.interlaced;

	if (interlaced) {
		enough_buffer = (memout_count_buffer_in_queue(input_q)
			>= SRCCAP_MEMOUT_MIN_BUF_FOR_CAPTURING_FIELDS);
		ret = drv_hwreg_srccap_memout_get_access(srccap_dev->dev_id, &mode);
		if (ret < 0)
			mode = 0;
		write_en = (mode > 0);

		appropriate = ((enough_buffer && (src_field == 0))
			|| (write_en && (src_field == 1)));
	} else
		appropriate = !list_empty(input_q);

	SRCCAP_MSG_DEBUG("%s:%d, %s:%u, %s:%d, %s:%d, %s:%u\n",
		"inputq_buf_cnt", memout_count_buffer_in_queue(input_q),
		"mode", mode,
		"enough_buffer", enough_buffer,
		"write_en", write_en,
		"src_field", src_field);

	return appropriate;
}

static int memout_check_next_entry_stage(
	struct list_head *head,
	enum srccap_memout_buffer_stage *stage)
{
	struct srccap_memout_buffer_entry *entry = NULL;

	if ((head == NULL) || (head->next == NULL) || (stage == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (head->prev == head) {
		SRCCAP_MSG_ERROR("No entries in queue!\n");
		return -EINVAL;
	}

	entry = list_last_entry(head, struct srccap_memout_buffer_entry, list);
	if (entry == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENOMEM;
	}

	*stage = entry->stage;

	return 0;
}

static int memout_prepare_dmabuf(
	struct mtk_srccap_dev *srccap_dev,
	struct vb2_buffer *vb,
	u64 addr[])
{
	int ret = 0;
	int plane_cnt = 0;
	u8 frame_num = 0;
	u8 num_bufs = 0;
	u8 num_planes = 0;
	u32 align = 0;
	u64 offset = 0;
	unsigned int size = 0;
	bool contiguous = FALSE;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if ((vb == NULL) || (addr == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	switch (srccap_dev->memout_info.buf_ctrl_mode) {
	case V4L2_MEMOUT_SWMODE:
#ifdef M6L_SW_MODE_EN
		if (srccap_dev->srccap_info.cap.hw_version == 1) /* index control */
#endif
			frame_num = SRCCAP_MEMOUT_FRAME_NUM;
#ifdef M6L_SW_MODE_EN
		else /* address control */
			frame_num = 1;
#endif
		break;
	case V4L2_MEMOUT_HWMODE:
		frame_num = SRCCAP_MEMOUT_FRAME_NUM;
		break;
	case V4L2_MEMOUT_BYPASSMODE:
		return 0;
	default:
		SRCCAP_MSG_ERROR("Invalid buffer control mode\n");
		return -EINVAL;
	}

	num_bufs = srccap_dev->memout_info.num_bufs;
	num_planes = srccap_dev->memout_info.num_planes;
	align = srccap_dev->srccap_info.cap.bpw;

	contiguous = num_bufs < num_planes;
	if (contiguous) {
		ret = mtkd_iommu_getiova(vb->planes[0].m.fd, &addr[0], &size);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		/* calculate frame addresses for contiguous planes */
		offset = 0;
		for (plane_cnt = 0; plane_cnt < num_planes; plane_cnt++) {
			addr[plane_cnt] = addr[0] + offset;
			offset += srccap_dev->memout_info.frame_pitch[plane_cnt] * frame_num;
		}
	} else {
		for (plane_cnt = 0; plane_cnt < num_planes; ++plane_cnt) {
			ret = mtkd_iommu_getiova(vb->planes[plane_cnt].m.fd,
				&addr[plane_cnt], &size);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}
		}
	}

	/* prepare base address */
	for (plane_cnt = 0; plane_cnt < num_planes; plane_cnt++) {
		addr[plane_cnt] *= SRCCAP_BYTE_SIZE;
		do_div(addr[plane_cnt], align); // unit: word
	}

	return 0;
}

static int memout_check_reg_occupied(
	struct reg_info reg[],
	u16 count,
	bool *occupied)
{
	int index = 0;

	if ((reg == NULL) || (occupied == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	for (index = 0; index < count; index++) {
		if ((reg[index].addr == 0) || (reg[index].mask == 0))
			break;
	}

	*occupied = (index == count);

	return 0;
}

static int memout_write_ml_commands(
	struct reg_info reg[],
	u16 count,
	struct srccap_ml_script_info *ml_script_info)
{
	int ret = 0;
	enum sm_return_type ml_ret = 0;
	bool reg_occupied = FALSE;
	struct sm_ml_direct_write_mem_info ml_direct_write_info;

	if (ml_script_info == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&ml_direct_write_info, 0, sizeof(struct sm_ml_direct_write_mem_info));

	ret = memout_check_reg_occupied(reg, count, &reg_occupied);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (reg_occupied == FALSE) {
		SRCCAP_MSG_ERROR("No registers to be written\n");
		return -EINVAL;
	}

	ml_direct_write_info.reg = (struct sm_reg *)reg;
	ml_direct_write_info.cmd_cnt = count;
	ml_direct_write_info.va_address = ml_script_info->start_addr
			+ (ml_script_info->total_cmd_count  * SRCCAP_ML_CMD_SIZE);
	ml_direct_write_info.va_max_address = ml_script_info->max_addr;
	ml_direct_write_info.add_32_support = FALSE;

	ml_ret = sm_ml_write_cmd(&ml_direct_write_info);
	if (ml_ret != E_SM_RET_OK)
		SRCCAP_MSG_ERROR("sm_ml_write_cmd fail, ret:%d\n", ml_ret);

	ml_script_info->total_cmd_count += count;

	return 0;
}

static int memout_fire_ml_commands(
	struct reg_info reg[],
	u16 count,
	int ml_fd,
	u8 ml_mem_index,
	struct srccap_ml_script_info *ml_script_info)
{
	int ret = 0;
	enum sm_return_type ml_ret = 0;
	bool reg_occupied = FALSE;
	struct sm_ml_fire_info ml_fire_info;

	if (ml_script_info == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&ml_fire_info, 0, sizeof(struct sm_ml_fire_info));

	ret = memout_check_reg_occupied(reg, count, &reg_occupied);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (reg_occupied == FALSE) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return -EINVAL;
	}

	ml_fire_info.cmd_cnt = ml_script_info->total_cmd_count;
	ml_fire_info.external = FALSE;
	ml_fire_info.mem_index = ml_mem_index;
	ml_ret = sm_ml_fire(ml_fd, &ml_fire_info);
	if (ml_ret != E_SM_RET_OK)
		SRCCAP_MSG_ERROR("sm_ml_fire fail, ret:%d\n", ml_ret);

	return 0;
}

static void memout_set_sw_mode_settings(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_memout_buffer_entry *entry)
{
	int ret = 0;
	enum sm_return_type ml_ret = E_SM_RET_FAIL;
	u8 ml_mem_index = 0;
	struct reg_info reg[SRCCAP_MEMOUT_REG_NUM] = {{0}};
	u16 count = 0;
	struct srccap_ml_script_info ml_script_info;
	struct sm_ml_info ml_info;
	struct reg_srccap_memout_force_w_setting force_w_setting;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	if (entry == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	if (!srccap_dev->streaming) {
		SRCCAP_MSG_ERROR("Not streaming\n");
		return;
	}

	memset(&ml_script_info, 0, sizeof(struct srccap_ml_script_info));
	memset(&ml_info, 0, sizeof(struct sm_ml_info));
	memset(&force_w_setting, 0, sizeof(struct reg_srccap_memout_force_w_setting));

	ml_ret = sm_ml_get_mem_index(srccap_dev->srccap_info.ml_info.fd_memout,
		&ml_mem_index, FALSE);
	if (ml_ret != E_SM_RET_OK) {
		SRCCAP_MSG_DEBUG("sm_ml_get_mem_index fail, ml_ret:%d\n", ml_ret);
		return;
	}

	ml_script_info.instance = srccap_dev->srccap_info.ml_info.fd_memout;
	ml_script_info.mem_index = ml_mem_index;

	ml_ret = sm_ml_get_info(ml_script_info.instance, ml_script_info.mem_index, &ml_info);
	if (ml_ret != E_SM_RET_OK) {
		SRCCAP_MSG_ERROR("sm_ml_get_info fail\n");
		return;
	}

	ml_script_info.start_addr = (void *)ml_info.start_va;
	ml_script_info.max_addr = (void *)((u8 *)ml_info.start_va + ml_info.mem_size);
	ml_script_info.mem_size = ml_info.mem_size;

	/* set base address */
	count = 0;
	ret = drv_hwreg_srccap_memout_set_base_addr(srccap_dev->dev_id,
		entry->addr, FALSE, reg, &count);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return;
	}

	ret = memout_write_ml_commands(reg, count, &ml_script_info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return;
	}

	ml_script_info.memout_memory_addr_cmd_count += count;

	/* set w index if sw mode not supported by hw */
#ifdef M6L_SW_MODE_EN
	if (srccap_dev->srccap_info.cap.hw_version == 1) {
#endif
		count = 0;
		force_w_setting.index = entry->w_index;
		force_w_setting.mode = 0;
		force_w_setting.en = TRUE;
		ret = drv_hwreg_srccap_memout_set_force_write(srccap_dev->dev_id,
			force_w_setting, FALSE, reg, &count);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return;
		}

		ret = memout_write_ml_commands(reg, count, &ml_script_info);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return;
		}

		ml_script_info.memout_memory_addr_cmd_count += count;
#ifdef M6L_SW_MODE_EN
	}
#endif

	/* set access */
	count = 0;
	ret = drv_hwreg_srccap_memout_set_access(srccap_dev->dev_id,
		srccap_dev->memout_info.access_mode, FALSE, reg, &count);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return;
	}

	ret = memout_write_ml_commands(reg, count, &ml_script_info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return;
	}

	ml_script_info.memout_memory_addr_cmd_count += count;

	ret = memout_fire_ml_commands(reg, count,
		srccap_dev->srccap_info.ml_info.fd_memout, ml_mem_index, &ml_script_info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return;
	}
}

static int memout_set_hw_mode_settings(
	struct mtk_srccap_dev *srccap_dev,
	u64 addr[SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM])
{
	int ret = 0;

	ret = drv_hwreg_srccap_memout_set_base_addr(srccap_dev->dev_id, addr, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_access(srccap_dev->dev_id,
		srccap_dev->memout_info.access_mode, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int memout_disable_access(
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	enum sm_return_type ml_ret = E_SM_RET_FAIL;
	u8 ml_mem_index = 0;
	struct reg_info reg[SRCCAP_MEMOUT_REG_NUM] = {{0}};
	u16 count = 0;
	struct srccap_ml_script_info ml_script_info;
	struct sm_ml_info ml_info;
	struct reg_srccap_memout_force_w_setting force_w_setting;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	memset(&ml_script_info, 0, sizeof(struct srccap_ml_script_info));
	memset(&ml_info, 0, sizeof(struct sm_ml_info));
	memset(&force_w_setting, 0, sizeof(struct reg_srccap_memout_force_w_setting));

	ml_ret = sm_ml_get_mem_index(srccap_dev->srccap_info.ml_info.fd_memout,
		&ml_mem_index, FALSE);
	if (ml_ret != E_SM_RET_OK) {
		SRCCAP_MSG_DEBUG("sm_ml_get_mem_index fail, ml_ret:%d\n", ml_ret);
		return -EPERM;
	}

	ml_script_info.instance = srccap_dev->srccap_info.ml_info.fd_memout;
	ml_script_info.mem_index = ml_mem_index;

	ml_ret = sm_ml_get_info(ml_script_info.instance, ml_script_info.mem_index, &ml_info);
	if (ml_ret != E_SM_RET_OK) {
		SRCCAP_MSG_ERROR("sm_ml_get_info fail\n");
		return -EPERM;
	}

	ml_script_info.start_addr = (void *)ml_info.start_va;
	ml_script_info.max_addr = (void *)((u8 *)ml_info.start_va + ml_info.mem_size);
	ml_script_info.mem_size = ml_info.mem_size;

	count = 0;
	/* set access */
	ret = drv_hwreg_srccap_memout_set_access(srccap_dev->dev_id, 0, FALSE, reg, &count);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = memout_write_ml_commands(reg, count, &ml_script_info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ml_script_info.memout_memory_addr_cmd_count += count;

	/* set w index if sw mode not supported by hw */
	if (srccap_dev->srccap_info.cap.hw_version == 1) {
		count = 0;
		/* switch index to a number that is never used so field type is not overwritten */
		force_w_setting.index = SRCCAP_MEMOUT_MAX_W_INDEX;
		force_w_setting.mode = 0;
		force_w_setting.en = TRUE;
		ret = drv_hwreg_srccap_memout_set_force_write(srccap_dev->dev_id,
			force_w_setting, FALSE, reg, &count);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		ret = memout_write_ml_commands(reg, count, &ml_script_info);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		ml_script_info.memout_memory_addr_cmd_count += count;
	}

	ret = memout_fire_ml_commands(reg, count,
		srccap_dev->srccap_info.ml_info.fd_memout, ml_mem_index, &ml_script_info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int memout_convert_timestamp_to_us(u32 clk, u32 timestamp, u64 *timestamp_in_us)
{
	if (timestamp_in_us == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	*timestamp_in_us = (u64)timestamp * SRCCAP_USEC_PER_SEC;
	do_div(*timestamp_in_us, clk);

	return 0;
}

static int memout_process_timestamp(
	struct mtk_srccap_dev *srccap_dev,
	u32 *hw_timestamp,
	u64 *timestamp_in_us)
{
	int ret = 0;
	u32 timestamp_report = 0;
	u64 timestamp_us = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* HDMI can get timestamp from hw but others can not */
	if ((srccap_dev->srccap_info.src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI) &&
		(srccap_dev->srccap_info.src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4)) {
		ret = drv_hwreg_srccap_memout_get_timestamp(srccap_dev->dev_id, &timestamp_report);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		ret = memout_convert_timestamp_to_us(srccap_dev->srccap_info.cap.xtal_clk,
			timestamp_report, &timestamp_us);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		*hw_timestamp = timestamp_report;
		*timestamp_in_us = timestamp_us;

		SRCCAP_MSG_DEBUG("%s:0x%x, %s:0x%lx\n",
			"hw_timestamp", *hw_timestamp,
			"timestamp_in_us", *timestamp_in_us);
	}

	return ret;
}

static int memout_get_cfd_color_format(u8 data_format, enum m_color_format *color_format)
{
	int ret = 0;

	switch (data_format) {
	case E_PQ_CFD_DATA_FORMAT_RGB:
		*color_format = FORMAT_RGB;
		break;
	case E_PQ_CFD_DATA_FORMAT_YUV420:
		*color_format = FORMAT_YUV420;
		break;
	case E_PQ_CFD_DATA_FORMAT_YUV422:
		*color_format = FORMAT_YUV422;
		break;
	case E_PQ_CFD_DATA_FORMAT_YUV444:
		*color_format = FORMAT_YUV444;
		break;
	default:
		SRCCAP_MSG_ERROR("unknoewn cfd data format\n");
		ret = -1;
		break;
	}
	return ret;

}

static int memout_get_dip_point_info(
	struct mtk_srccap_dev *srccap_dev,
	struct meta_srccap_dip_point_info *dip_point_info)
{
	int ret = 0;

	dip_point_info->width = srccap_dev->dscl_info.dscl_size.input.width;
	dip_point_info->height = srccap_dev->dscl_info.dscl_size.input.height;
	dip_point_info->crop_align = srccap_dev->srccap_info.cap.crop_align;

	ret = memout_get_cfd_color_format(
		srccap_dev->common_info.color_info.dip_point.data_format,
		&dip_point_info->color_format);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	SRCCAP_MSG_DEBUG("dip point info w:%d, h:%d, crop align:%d, format:%d, cfd format:%d\n",
		dip_point_info->width,
		dip_point_info->height,
		dip_point_info->crop_align,
		dip_point_info->color_format,
		srccap_dev->common_info.color_info.dip_point.data_format);

	return 0;
}

static int memout_get_field_info(struct mtk_srccap_dev *srccap_dev, u8 w_index)
{
	int ret = 0;
	u8 field_report = 0;

	if (srccap_dev == NULL) {

		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = drv_hwreg_srccap_memout_get_field(srccap_dev->dev_id, w_index, &field_report);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	srccap_dev->memout_info.field_info = field_report;
	SRCCAP_MSG_DEBUG("%s:%x\n",
			"field_info", field_report);

	return ret;
}

/* ============================================================================================== */
/* --------------------------------------- v4l2_ctrl_ops ---------------------------------------- */
/* ============================================================================================== */
static int mtk_memout_set_vflip(
	struct mtk_srccap_dev *srccap_dev,
	bool vflip)
{
	int ret = 0;
	u8 dev_id = (u8)srccap_dev->dev_id;

	pr_err("[SRCCAP][%s]vflip:%ld\n",
		__func__,
		vflip);

	/* set vflip */
	ret = drv_hwreg_srccap_memout_set_vflip(dev_id, vflip, TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int mtk_memout_set_buf_ctrl_mode(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_memout_buf_ctrl_mode mode)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	SRCCAP_MSG_INFO("buf_ctrl_mode:%d\n", mode);

	srccap_dev->memout_info.buf_ctrl_mode = mode;

	return 0;
}

static int mtk_memout_get_min_bufs_for_capture(
	struct mtk_srccap_dev *srccap_dev,
	s32 *min_bufs)
{
	if (min_bufs == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	*min_bufs = SRCCAP_MEMOUT_FRAME_NUM;

	return 0;
}

static int mtk_memout_get_timestamp(struct mtk_srccap_dev *srccap_dev,
	struct v4l2_memout_timeinfo *timeinfo)
{
	int ret = 0;
	u32 hw_timestamp_val = 0;
	u64 timestamp_in_us = 0;
	struct v4l2_memout_timeval ktime;

	memset(&ktime, 0, sizeof(struct v4l2_memout_timeval));

	SRCCAP_GET_TIMESTAMP(&ktime);
	timeinfo->ktimes.tv_sec = ktime.tv_sec;
	timeinfo->ktimes.tv_usec = ktime.tv_usec;

	ret = memout_process_timestamp(srccap_dev, &hw_timestamp_val, &timestamp_in_us);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	timeinfo->timestamp = timestamp_in_us;

	return ret;
}

static int _mtk_memout_get_hdmi_port_by_src(enum v4l2_srccap_input_source src,
	int *hdmi_port)
{
	if ((src >= V4L2_SRCCAP_INPUT_SOURCE_DVI) && (src <= V4L2_SRCCAP_INPUT_SOURCE_DVI4))
		*hdmi_port = src - V4L2_SRCCAP_INPUT_SOURCE_DVI;
	else if ((src >= V4L2_SRCCAP_INPUT_SOURCE_HDMI) && (src <= V4L2_SRCCAP_INPUT_SOURCE_HDMI4))
		*hdmi_port = src - V4L2_SRCCAP_INPUT_SOURCE_HDMI;
	else
		return -EINVAL;

	return 0;
}

static int mtk_memout_updata_hdmi_packet_info(struct mtk_srccap_dev *srccap_dev,
	struct meta_srccap_hdr_pkt *hdr_pkt,
	struct meta_srccap_hdr_vsif_pkt *hdr_vsif_pkt)
{
	int ret = 0;
	int u32IEEEOUI = 0;
	int hdmi_port = -1;
	enum v4l2_srccap_input_source src;
	struct hdmi_vsif_packet_info vsif_data;

	memset(&vsif_data, 0, sizeof(struct hdmi_vsif_packet_info));

	src = srccap_dev->srccap_info.src;

	if (_mtk_memout_get_hdmi_port_by_src(src, &hdmi_port)) {
		SRCCAP_MSG_DEBUG("Not HDMI source!!!\n");
	} else {
		if (mtk_srccap_common_is_Cfd_stub_mode()) {
			/*hdr_pkt*/
			ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_HDRIF(hdr_pkt);
			if (ret == 0)
				SRCCAP_MSG_ERROR("failed to mtk_srccap_hdmirx_pkt_get_HDRIF\n");

			/*vsif_pkt*/
			ret = mtk_srccap_common_get_cfd_test_HDMI_pkt_VSIF(&vsif_data);
			if (ret == 0)
				SRCCAP_MSG_ERROR("failed to mtk_srccap_hdmirx_pkt_get_VSIF\n");
		} else {
			/*hdr_pkt*/
			ret = mtk_srccap_hdmirx_pkt_get_HDRIF(src,
				(struct hdmi_hdr_packet_info *)hdr_pkt,
				sizeof(struct meta_srccap_hdr_pkt));
			if (ret == 0)
				SRCCAP_MSG_ERROR("failed to mtk_srccap_hdmirx_pkt_get_HDRIF\n");
				/*hdr_vsif_pkt*/
				ret = mtk_srccap_hdmirx_pkt_get_VSIF(src,
				&vsif_data, sizeof(struct hdmi_vsif_packet_info));
				if (ret == 0)
					SRCCAP_MSG_ERROR(
					"failed to mtk_srccap_hdmirx_pkt_get_VSIF\n");
		}


		// HB0
		u32IEEEOUI = ((vsif_data.au8ieee[0]) | (vsif_data.au8ieee[1] << 8)
				| (vsif_data.au8ieee[2] << 16));

		switch (u32IEEEOUI) {
		case SRCCAP_HDMI_VSIF_PACKET_IEEE_OUI_HRD10_PLUS:
			srccap_dev->common_info.color_info.hdr_type
				= 0x5;//E_PQ_CFD_HDR_MODE_HDR10PLUS;
			hdr_vsif_pkt->u16Version = 0;
			hdr_vsif_pkt->u16Size = sizeof(struct meta_srccap_hdr_vsif_pkt);
			hdr_vsif_pkt->bValid = TRUE;
			hdr_vsif_pkt->u8VSIFTypeCode =
				(SRCCAP_HDMI_VS_PACKET_TYPE) & 0x7F; //(0x81) & 0x7F
			hdr_vsif_pkt->u8VSIFVersion = vsif_data.u8version;
			hdr_vsif_pkt->u8Length = vsif_data.u5length & 0x1F;
			hdr_vsif_pkt->u32IEEECode = u32IEEEOUI;
			//PB4
			hdr_vsif_pkt->u8ApplicationVersion = (vsif_data.au8payload[0] & 0xC0) >> 6;
			hdr_vsif_pkt->u8TargetSystemDisplayMaxLuminance =
				(vsif_data.au8payload[0] & 0x3E) >> 1;
			hdr_vsif_pkt->u8AverageMaxRGB = vsif_data.au8payload[1];
			hdr_vsif_pkt->au8DistributionValues[0] = vsif_data.au8payload[2];
			hdr_vsif_pkt->au8DistributionValues[1] = vsif_data.au8payload[3];
			hdr_vsif_pkt->au8DistributionValues[2] = vsif_data.au8payload[4];
			hdr_vsif_pkt->au8DistributionValues[3] = vsif_data.au8payload[5];
			hdr_vsif_pkt->au8DistributionValues[4] = vsif_data.au8payload[6];
			hdr_vsif_pkt->au8DistributionValues[5] = vsif_data.au8payload[7];
			hdr_vsif_pkt->au8DistributionValues[6] = vsif_data.au8payload[8];
			hdr_vsif_pkt->au8DistributionValues[7] = vsif_data.au8payload[9];
			hdr_vsif_pkt->au8DistributionValues[8] = vsif_data.au8payload[10];
			hdr_vsif_pkt->u16KneePointX =
				(((vsif_data.au8payload[12] & 0xFC) >> 2)
				| ((vsif_data.au8payload[11] & 0x0F) << 6));
			hdr_vsif_pkt->u16KneePointY =
				(vsif_data.au8payload[13] & 0xFF)
				| ((vsif_data.au8payload[12] & 0x03) << 8);
			hdr_vsif_pkt->u8NumBezierCurveAnchors =
				(vsif_data.au8payload[11] & 0xF0) >> 4;
			hdr_vsif_pkt->au8BezierCurveAnchors[0] = vsif_data.au8payload[14];
			hdr_vsif_pkt->au8BezierCurveAnchors[1] = vsif_data.au8payload[15];
			hdr_vsif_pkt->au8BezierCurveAnchors[2] = vsif_data.au8payload[16];
			hdr_vsif_pkt->au8BezierCurveAnchors[3] = vsif_data.au8payload[17];
			hdr_vsif_pkt->au8BezierCurveAnchors[4] = vsif_data.au8payload[18];
			hdr_vsif_pkt->au8BezierCurveAnchors[5] = vsif_data.au8payload[19];
			hdr_vsif_pkt->au8BezierCurveAnchors[6] = vsif_data.au8payload[20];
			hdr_vsif_pkt->au8BezierCurveAnchors[7] = vsif_data.au8payload[21];
			hdr_vsif_pkt->au8BezierCurveAnchors[8] = vsif_data.au8payload[22];
			hdr_vsif_pkt->u8GraphicsOverlayFlag =
				(vsif_data.au8payload[23] & 0x80) >> 7;
			hdr_vsif_pkt->u8NoDelayFlag = (vsif_data.au8payload[23] & 0x40) >> 6;

			break;
		default:
			break;
		}
	}

	return 0;
}

static int _memout_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, memout_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_MIN_BUFFERS_FOR_CAPTURE:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_MIN_BUFFERS_FOR_CAPTURE\n");

		ret = mtk_memout_get_min_bufs_for_capture(
			srccap_dev, &ctrl->val);
		break;
	}
	case V4L2_CID_MEMOUT_SECURE_SRC_TABLE:
	{

#if IS_ENABLED(CONFIG_OPTEE)
		struct v4l2_memout_sst_info *sstinfo =
			(struct v4l2_memout_sst_info *)(ctrl->p_new.p);

#endif
		if (ctrl->p_new.p == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			return -EINVAL;
		}

#if IS_ENABLED(CONFIG_OPTEE)
		SRCCAP_MSG_DEBUG("V4L2_CID_MEMOUT_SECURE_SRC_TABLE\n");

		ret = mtk_memout_get_sst(srccap_dev, sstinfo);
#endif
		break;
	}
	case V4L2_CID_MEMOUT_TIME_STAMP:
	{
		if (ctrl->p_new.p == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			return -EINVAL;
		}

		u32 hw_timestamp = 0;
		struct v4l2_memout_timeinfo *timeinfo =
			(struct v4l2_memout_timeinfo *)(ctrl->p_new.p);

		SRCCAP_MSG_DEBUG("V4L2_CID_MEMOUT_TIME_STAMP\n");

		ret = mtk_memout_get_timestamp(srccap_dev, timeinfo);
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int _memout_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, memout_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
	{
		bool vflip = (bool)(ctrl->val);

		SRCCAP_MSG_DEBUG("V4L2_CID_VFLIP\n");

		ret = mtk_memout_set_vflip(srccap_dev, vflip);
		break;
	}
	case V4L2_CID_MEMOUT_BUF_CTRL_MODE:
	{
		enum v4l2_memout_buf_ctrl_mode buf_ctrl_mode =
			(enum v4l2_memout_buf_ctrl_mode)(ctrl->val);

		SRCCAP_MSG_DEBUG("V4L2_CID_MEMOUT_BUF_CTRL_MODE\n");

		ret = mtk_memout_set_buf_ctrl_mode(srccap_dev, buf_ctrl_mode);
		break;
	}
	case V4L2_CID_MEMOUT_SECURE_MODE:
	{
#if IS_ENABLED(CONFIG_OPTEE)
		struct v4l2_memout_secure_info secureinfo;

		if (ctrl->p_new.p == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			return -EINVAL;
		}

		secureinfo = *(struct v4l2_memout_secure_info *)(ctrl->p_new.p);

		SRCCAP_MSG_DEBUG("V4L2_CID_MEMOUT_SECURE_MODE\n");

		ret = mtk_memout_set_secure_mode(srccap_dev, &secureinfo);
#endif
		break;
	}
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops memout_ctrl_ops = {
	.g_volatile_ctrl = _memout_g_ctrl,
	.s_ctrl = _memout_s_ctrl,
};

static const struct v4l2_ctrl_config memout_ctrl[] = {
	{
		.ops = &memout_ctrl_ops,
		.id = V4L2_CID_VFLIP,
		.name = "Srccap MemOut V Flip",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = 0,
		.max = 1,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &memout_ctrl_ops,
		.id = V4L2_CID_MEMOUT_BUF_CTRL_MODE,
		.name = "Srccap MemOut Buffer Ctrl Mode",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &memout_ctrl_ops,
		.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
		.name = "Srccap MemOut Minimum Buffer Number",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &memout_ctrl_ops,
		.id = V4L2_CID_MEMOUT_SECURE_MODE,
		.name = "Srccap MemOut Security",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_memout_secure_info)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &memout_ctrl_ops,
		.id = V4L2_CID_MEMOUT_SECURE_SRC_TABLE,
		.name = "Srccap MemOut Get SST",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_memout_sst_info)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &memout_ctrl_ops,
		.id = V4L2_CID_MEMOUT_TIME_STAMP,
		.name = "Srccap MemOut Get TimeStamp",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_memout_timeinfo)},
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE
			| V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops memout_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops memout_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_memout_read_metadata(
	int meta_fd,
	enum srccap_memout_metatag meta_tag,
	void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (meta_content == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = memout_map_fd(meta_fd, &va, &size);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = memout_get_metaheader(meta_tag, &header);
	if (ret) {
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

	meta_ret = query_metadata_content(&buffer, &header, meta_content);
	if (!meta_ret) {
		SRCCAP_MSG_ERROR("metadata content not found\n");
		ret = -EPERM;
		goto EXIT;
	}

EXIT:
	memout_unmap_fd(meta_fd, va);
	return ret;
}

int mtk_memout_write_metadata(
	int meta_fd,
	enum srccap_memout_metatag meta_tag,
	void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (meta_content == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = memout_map_fd(meta_fd, &va, &size);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = memout_get_metaheader(meta_tag, &header);
	if (ret) {
		//SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;

	if (meta_tag == SRCCAP_MEMOUT_METATAG_HDMIRX_VSIF_PKT) {
		struct meta_srccap_hdmi_pkt *meta_hdmi_pkt =
			(struct meta_srccap_hdmi_pkt *)meta_content;
		header.size = 1 + sizeof(struct hdmi_vsif_packet_info) *
			meta_hdmi_pkt->u8Size;
	}
	if ((meta_tag == SRCCAP_MEMOUT_METATAG_HDMIRX_VS_EMP_PKT) ||
		(meta_tag == SRCCAP_MEMOUT_METATAG_HDMIRX_DSC_EMP_PKT) ||
		(meta_tag == SRCCAP_MEMOUT_METATAG_HDMIRX_DHDR_EMP_PKT)) {
		struct meta_srccap_hdmi_pkt *meta_hdmi_pkt =
			(struct meta_srccap_hdmi_pkt *)meta_content;
		header.size = 1 + sizeof(struct hdmi_emp_packet_info) *
			meta_hdmi_pkt->u8Size;
	}

	meta_ret = attach_metadata(&buffer, header, meta_content);
	if (!meta_ret) {
		SRCCAP_MSG_ERROR("metadata not attached\n");
		ret = -EPERM;
		goto EXIT;
	}

EXIT:
	memout_unmap_fd(meta_fd, va);
	return ret;
}

int mtk_memout_remove_metadata(
	int meta_fd,
	enum srccap_memout_metatag meta_tag)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = memout_map_fd(meta_fd, &va, &size);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = memout_get_metaheader(meta_tag, &header);
	if (ret) {
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
	memout_unmap_fd(meta_fd, va);
	return ret;
}

void mtk_memout_process_buffer(
	struct mtk_srccap_dev *srccap_dev,
	u8 src_field)
{
	int ret = 0;
	int plane_cnt = 0;
	bool empty_inputq = FALSE, empty_processingq = FALSE;
	struct srccap_memout_buffer_entry *entry = NULL;
	struct reg_srccap_memout_force_w_setting force_w_setting;
	struct timeval start_time;
	struct timeval end_time;
	u64 time_diff = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	if (atrace_enable_srccap == 1) {
		SRCCAP_ATRACE_BEGIN("mtk_memout_process_buffer");
		SRCCAP_ATRACE_INT_FORMAT(srccap_dev->dev_id,
			"S: [start][srccap]process_buf_%u", srccap_dev->dev_id);
	}
#endif

	memset(&start_time, 0, sizeof(struct timeval));
	memset(&end_time, 0, sizeof(struct timeval));
	memset(&force_w_setting, 0, sizeof(struct reg_srccap_memout_force_w_setting));

	SRCCAP_GET_TIMESTAMP(&start_time);

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
	/* move every completed frame from processing queue to output queue */
	/* stop after reaching last frame(new) or out of frames */
	empty_processingq = list_empty(&srccap_dev->memout_info.frame_q.processingq_head);
	while (!empty_processingq) {
		entry =  memout_delete_last_entry(
			&srccap_dev->memout_info.frame_q.processingq_head);
		if (entry == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
			return;
		}

		if (entry->stage == SRCCAP_MEMOUT_BUFFER_STAGE_NEW) {
			entry->stage = SRCCAP_MEMOUT_BUFFER_STAGE_CAPTURING;

			SRCCAP_MSG_DEBUG("[proq->proq](%u): vb:%p vb_index:%d stage:%d",
				srccap_dev->dev_id,
				entry->vb, entry->vb->index, entry->stage);
			for (plane_cnt = 0;
				plane_cnt < SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM; plane_cnt++)
				SRCCAP_MSG_DEBUG(" addr%d=%d",
					plane_cnt, entry->addr[plane_cnt]);
			SRCCAP_MSG_DEBUG(" index:%d field:%u\n",
				entry->w_index, entry->field);

			memout_add_last_entry(
				&srccap_dev->memout_info.frame_q.processingq_head,
				entry);

			break;
		} else if (entry->stage == SRCCAP_MEMOUT_BUFFER_STAGE_CAPTURING) {
			entry->stage = SRCCAP_MEMOUT_BUFFER_STAGE_DONE;

			SRCCAP_MSG_DEBUG("[proq->outq](%u): vb:%p vb_index:%d stage:%d",
				srccap_dev->dev_id,
				entry->vb, entry->vb->index, entry->stage);
			for (plane_cnt = 0;
				plane_cnt < SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM; plane_cnt++)
				SRCCAP_MSG_DEBUG(" addr%d=%d",
					plane_cnt, entry->addr[plane_cnt]);
			SRCCAP_MSG_DEBUG(" index:%d field:%u\n",
				entry->w_index, entry->field);

			memout_add_next_entry(
				&srccap_dev->memout_info.frame_q.outputq_head,
				entry);

			vb2_buffer_done(entry->vb, VB2_BUF_STATE_DONE);
		} else { /* entry->stage == SRCCAP_MEMOUT_BUFFER_STAGE_DONE */
			SRCCAP_MSG_ERROR("Buffer already done!\n");

			SRCCAP_MSG_DEBUG("[proq->outq](%u): vb:%p vb_index:%d stage:%d",
				srccap_dev->dev_id,
				entry->vb, entry->vb->index, entry->stage);
			for (plane_cnt = 0;
				plane_cnt < SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM; plane_cnt++)
				SRCCAP_MSG_DEBUG(" addr%d=%d",
					plane_cnt, entry->addr[plane_cnt]);
			SRCCAP_MSG_DEBUG(" index:%d\n", entry->w_index);

			memout_add_next_entry(
				&srccap_dev->memout_info.frame_q.outputq_head,
				entry);

			vb2_buffer_done(entry->vb, VB2_BUF_STATE_DONE);
		}

		empty_processingq =
			list_empty(&srccap_dev->memout_info.frame_q.processingq_head);
	}

	/* move a frame or two fields from input queue to processing queue */
	if (memout_check_frame_or_field_appropriate_for_write(srccap_dev, src_field)) {
		entry =  memout_delete_last_entry(
			&srccap_dev->memout_info.frame_q.inputq_head);
		if (entry == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
			return;
		}

		entry->field = (src_field == 0) ?
			SRCCAP_MEMOUT_TOP_FIELD : SRCCAP_MEMOUT_BOTTOM_FIELD;

		SRCCAP_MSG_DEBUG(
			"[inq->proq](%u): vb:%p vb_index:%d stage:%d",
			srccap_dev->dev_id,
			entry->vb, entry->vb->index, entry->stage);
		for (plane_cnt = 0;
			plane_cnt < SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM; plane_cnt++)
			SRCCAP_MSG_DEBUG(" addr%d=%d", plane_cnt, entry->addr[plane_cnt]);
		SRCCAP_MSG_DEBUG(" index:%d field:%u\n", entry->w_index, entry->field);

		if (entry->vb != NULL)
			memout_set_sw_mode_settings(srccap_dev, entry);

		memout_add_next_entry(
			&srccap_dev->memout_info.frame_q.processingq_head,
			entry);
	} else {
		if (memout_disable_access(srccap_dev))
			SRCCAP_MSG_ERROR("access off\n");
	}
	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);

	SRCCAP_GET_TIMESTAMP(&end_time);

	time_diff = ((u64)end_time.tv_sec - (u64)start_time.tv_sec) * 1000000UL
		+ ((u64)end_time.tv_usec - (u64)start_time.tv_usec);
	if (time_diff >= SRCCAP_MEMOUT_MAX_THREAD_PROCESSING_TIME)
		SRCCAP_MSG_ERROR("process timeout!\n");

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	if (atrace_enable_srccap == 1) {
		SRCCAP_ATRACE_END("mtk_memout_process_buffer");
		SRCCAP_ATRACE_INT_FORMAT(srccap_dev->dev_id,
			"E: [end][srccap]process_buf_%u", srccap_dev->dev_id);
	}
#endif
}

int mtk_memout_s_fmt_vid_cap(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (format == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	return 0;
}

int mtk_memout_s_fmt_vid_cap_mplane(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format)
{
	int ret = 0;
	int plane_cnt = 0;
	u8 dev_id = 0;
	struct reg_srccap_memout_format_conversion fmt_conv;
	struct reg_srccap_memout_resolution res;
	bool pack = FALSE, argb = FALSE;
	u8 num_planes = 0, num_bufs = 0, access_mode = 0;
	u8 bitmode_alpha = 0, bitmode[SRCCAP_MAX_PLANE_NUM] = { 0 },
	   ce[SRCCAP_MAX_PLANE_NUM] = { 0 };
	u8 bitwidth[SRCCAP_MAX_PLANE_NUM] = { 0 };
	u32 hoffset = 0, vsize = 0, align = 0, pitch[SRCCAP_MAX_PLANE_NUM] = { 0 };
	enum srccap_dv_descrb_color_format color_format = SRCCAP_DV_DESCRB_COLOR_FORMAT_UNKNOWN;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (format == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	dev_id = (u8)srccap_dev->dev_id;
	memset(&fmt_conv, 0, sizeof(struct reg_srccap_memout_format_conversion));
	memset(&res, 0, sizeof(struct reg_srccap_memout_resolution));

	color_format = srccap_dev->dv_info.descrb_info.pkt_info.color_format;

	if (color_format >= SRCCAP_DV_DESCRB_COLOR_FORMAT_RESERVED)
		color_format = SRCCAP_DV_DESCRB_COLOR_FORMAT_RGB;
	format->fmt.pix_mp.pixelformat = fmt_map[color_format];

	switch (format->fmt.pix_mp.pixelformat) {
	/* ------------------ RGB format ------------------ */
	/* 3 plane -- contiguous planes */
	case V4L2_PIX_FMT_RGB_8_8_8_CE:
		num_planes = 3;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 1;
		ce[1] = 1;
		ce[2] = 1;
		break;
	/* 1 plane */
	case V4L2_PIX_FMT_RGB565:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 5;
		bitmode[1] = 6;
		bitmode[2] = 5;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_RGB888:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_RGB101010:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 10;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_RGB121212:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 12;
		bitmode[1] = 12;
		bitmode[2] = 12;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_ARGB8888:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_ARGB;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 8;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_ABGR8888:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_ABGR;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 8;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_RGBA8888:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGBA;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 8;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_BGRA8888:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_BGRA;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 8;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_ARGB2101010:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_ARGB;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 2;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 10;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_ABGR2101010:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_ABGR;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 2;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 10;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_RGBA1010102:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGBA;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 2;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 10;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_BGRA1010102:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_BGRA;
		srccap_dev->common_info.color_info.out.data_format = 0;
		argb = TRUE;
		access_mode = 0x7;
		bitmode_alpha = 2;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 10;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	/* ------------------ YUV format ------------------ */
	/* 3 plane -- contiguous planes */
	case V4L2_PIX_FMT_YUV444_Y_U_V_12B:
		num_planes = 3;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 12;
		bitmode[1] = 12;
		bitmode[2] = 12;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV444_Y_U_V_10B:
		num_planes = 3;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		srccap_dev->common_info.color_info.out.data_format = 2;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 10;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV444_Y_U_V_8B:
		num_planes = 3;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		srccap_dev->common_info.color_info.out.data_format = 2;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV444_Y_U_V_8B_CE:
		num_planes = 3;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		srccap_dev->common_info.color_info.out.data_format = 2;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 8;
		ce[0] = 1;
		ce[1] = 1;
		ce[2] = 1;
		break;
	/* 3 plane -- uncontiguous planes */
	/* 2 plane -- contiguous planes */
	case V4L2_PIX_FMT_YUV422_Y_UV_12B:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 12;
		bitmode[1] = 12;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_10B:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_NV16:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_8B_CE:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 0;
		ce[0] = 1;
		ce[1] = 1;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_8B_6B_CE:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 6;
		bitmode[2] = 0;
		ce[0] = 1;
		ce[1] = 2;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_6B_CE:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 6;
		bitmode[1] = 6;
		bitmode[2] = 0;
		ce[0] = 2;
		ce[1] = 2;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_10B:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV420;
		srccap_dev->common_info.color_info.out.data_format = 3;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_NV12:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV420;
		srccap_dev->common_info.color_info.out.data_format = 3;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_8B_CE:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV420;
		srccap_dev->common_info.color_info.out.data_format = 3;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 8;
		bitmode[2] = 0;
		ce[0] = 1;
		ce[1] = 1;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_8B_6B_CE:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV420;
		srccap_dev->common_info.color_info.out.data_format = 3;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 8;
		bitmode[1] = 6;
		bitmode[2] = 0;
		ce[0] = 1;
		ce[1] = 2;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_6B_CE:
		num_planes = 2;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV420;
		srccap_dev->common_info.color_info.out.data_format = 3;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 6;
		bitmode[1] = 6;
		bitmode[2] = 0;
		ce[0] = 2;
		ce[1] = 2;
		ce[2] = 0;
		break;
	/* 1 plane */
	case V4L2_PIX_FMT_YUV444_YUV_12B:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		srccap_dev->common_info.color_info.out.data_format = 2;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 12;
		bitmode[1] = 12;
		bitmode[2] = 12;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV444_YUV_10B:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV444;
		srccap_dev->common_info.color_info.out.data_format = 2;
		argb = FALSE;
		access_mode = 0x7;
		bitmode_alpha = 0;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 10;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV422_YUV_12B:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 12;
		bitmode[1] = 12;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV422_YUV_10B:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		srccap_dev->common_info.color_info.out.data_format = 1;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	case V4L2_PIX_FMT_YUV420_YUV_10B:
		num_planes = 1;
		num_bufs = 1;
		pack = 1;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV420;
		srccap_dev->common_info.color_info.out.data_format = 3;
		argb = FALSE;
		access_mode = 0x6;
		bitmode_alpha = 0;
		bitmode[0] = 10;
		bitmode[1] = 10;
		bitmode[2] = 0;
		ce[0] = 0;
		ce[1] = 0;
		ce[2] = 0;
		break;
	default:
		SRCCAP_MSG_ERROR("Invalid memory format\n");
		return -EINVAL;
	}

	/* prepare format */
	ret = memout_get_ip_fmt(srccap_dev->srccap_info.src, srccap_dev->hdmirx_info.color_format,
		&fmt_conv.ip_fmt);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_common_updata_color_info(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (srccap_dev->memout_info.buf_ctrl_mode != V4L2_MEMOUT_BYPASSMODE) {
		ret = mtk_common_set_cfd_setting(srccap_dev);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	fmt_conv.en = (fmt_conv.ip_fmt != fmt_conv.ipdma_fmt) ? TRUE : FALSE;

	/* prepare input resolution */
	res.hsize = format->fmt.pix_mp.width;
	res.vsize = (srccap_dev->timingdetect_info.data.interlaced) ?
		(format->fmt.pix_mp.height / SRCCAP_INTERLACE_HEIGHT_DIVISOR) :
		format->fmt.pix_mp.height;

	/* prepare line offset */
	hoffset = format->fmt.pix_mp.width;

	/* prepare frame pitch */
	vsize = (srccap_dev->timingdetect_info.data.interlaced) ?
		(format->fmt.pix_mp.height / SRCCAP_INTERLACE_HEIGHT_DIVISOR) :
		format->fmt.pix_mp.height;
	align = srccap_dev->srccap_info.cap.bpw;
	if (pack) {
		bitwidth[0] = bitmode[0] + bitmode[1] + bitmode[2];
		if (argb)
			bitwidth[0] += bitmode_alpha;
	} else {
		for (plane_cnt = 0; plane_cnt < SRCCAP_MAX_PLANE_NUM; plane_cnt++)
			bitwidth[plane_cnt] = bitmode[plane_cnt];
	}

	for (plane_cnt = 0; plane_cnt < SRCCAP_MAX_PLANE_NUM; plane_cnt++)
		pitch[plane_cnt] = SRCCAP_MEMOUT_ALIGN((hoffset * bitwidth[plane_cnt]), align)
			* vsize / align;

	/* set parameters for frame capturing */
	if ((srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_SWMODE) ||
		(srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_HWMODE))
		ret = memout_set_frame_capturing_parameters(dev_id, fmt_conv, pack, argb, bitmode,
			ce, res, hoffset, pitch);
	else /* srccap_dev->memout_info.buf_ctrl_mode == V4L2_MEMOUT_BYPASSMODE */
		SRCCAP_MSG_INFO("Bypass mode\n");

	/* return buffer number */
	format->fmt.pix_mp.num_planes = num_bufs;

	/* save format info */
	srccap_dev->memout_info.fmt_mp = format->fmt.pix_mp;
	srccap_dev->memout_info.num_planes = num_planes;
	srccap_dev->memout_info.num_bufs = num_bufs;
	srccap_dev->memout_info.frame_pitch[0] = pitch[0] * align / SRCCAP_BYTE_SIZE; // unit: byte
	srccap_dev->memout_info.frame_pitch[1] = pitch[1] * align / SRCCAP_BYTE_SIZE; // unit: byte
	srccap_dev->memout_info.frame_pitch[2] = pitch[2] * align / SRCCAP_BYTE_SIZE; // unit: byte
	srccap_dev->memout_info.access_mode = access_mode;
	srccap_dev->memout_info.hoffset = hoffset;

	return 0;
}

int mtk_memout_g_fmt_vid_cap(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (format == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	return 0;
}

int mtk_memout_g_fmt_vid_cap_mplane(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_format *format)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (format == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* load format info */
	format->fmt.pix_mp = srccap_dev->memout_info.fmt_mp;

	return 0;
}

int mtk_memout_reqbufs(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_requestbuffers *req_buf)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	return 0;
}

int mtk_memout_qbuf(
	struct mtk_srccap_dev *srccap_dev,
	struct vb2_buffer *vb)
{
	int ret = 0;
	int plane_cnt = 0;
	u8 dev_id = 0;
	u64 addr[SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM] = { 0 };
	bool empty_inputq_n_processingq = FALSE;
	bool empty_processingq = FALSE;
	struct srccap_memout_buffer_entry *entry = NULL;
	static u8 q_index[SRCCAP_DEV_NUM];
	enum srccap_memout_buffer_stage next_entry_stage = SRCCAP_MEMOUT_BUFFER_STAGE_NEW;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (vb == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	dev_id = (u8)srccap_dev->dev_id;

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	if (atrace_enable_srccap == 1) {
		SRCCAP_ATRACE_BEGIN("mtk_memout_qbuf");
		SRCCAP_ATRACE_INT_FORMAT(vb->index,  "40-S: [start][srccap]qbuf_%u", dev_id);
		SRCCAP_ATRACE_INT_FORMAT(buf_cnt[dev_id],  "buf_cnt_%u", dev_id);
	}
#endif

	switch (vb->memory) {
	case V4L2_MEMORY_DMABUF:
		ret = memout_prepare_dmabuf(srccap_dev, vb, addr);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("Invalid memory type\n");
		return -EINVAL;
	}

	switch (srccap_dev->memout_info.buf_ctrl_mode) {
	case V4L2_MEMOUT_SWMODE:
		mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
		entry = (struct srccap_memout_buffer_entry *)
			kzalloc(sizeof(struct srccap_memout_buffer_entry), GFP_KERNEL);
		if (entry == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
			return -ENOMEM;
		}
		entry->vb = vb;
		entry->stage = SRCCAP_MEMOUT_BUFFER_STAGE_NEW;
		memcpy(&entry->addr, &addr, (sizeof(u64) * SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM));
		entry->w_index = q_index[dev_id]; // internal index counter
		q_index[dev_id] = (q_index[dev_id] + 1) % SRCCAP_MEMOUT_FRAME_NUM;

		SRCCAP_MSG_DEBUG("[->inq](%u): vb:%p vb_index:%d stage:%d",
			dev_id,
			entry->vb, entry->vb->index, entry->stage);
		for (plane_cnt = 0; plane_cnt < SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM; plane_cnt++)
			SRCCAP_MSG_DEBUG(" addr%d=%d", plane_cnt, entry->addr[plane_cnt]);
		SRCCAP_MSG_DEBUG(" index:%d\n", entry->w_index);

		memout_add_next_entry(&srccap_dev->memout_info.frame_q.inputq_head, entry);
		mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);

		break;
	case V4L2_MEMOUT_HWMODE:
		ret = memout_set_hw_mode_settings(srccap_dev, addr);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

		break;
	case V4L2_MEMOUT_BYPASSMODE:
		ret = drv_hwreg_srccap_memout_set_access(dev_id, 0, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

		break;
	default:
		SRCCAP_MSG_ERROR("Invalid buffer control mode\n");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	if (atrace_enable_srccap == 1) {
		buf_cnt[dev_id]++;
		SRCCAP_ATRACE_END("mtk_memout_qbuf");
		SRCCAP_ATRACE_INT_FORMAT(vb->index,  "40-E: [end][srccap]qbuf_%u", dev_id);
		SRCCAP_ATRACE_INT_FORMAT(buf_cnt[dev_id],  "buf_cnt_%u", dev_id);
	}
#endif

	return 0;
}

static int
_mtk_memout_write_hdmirxdata(struct mtk_srccap_dev *srccap_dev,
			     struct v4l2_buffer *buf,
			     enum srccap_memout_metatag meta_tag,
			     struct meta_srccap_hdmi_pkt *pmeta_hdmi_pkt)
{
	int ret = 0;

	if (pmeta_hdmi_pkt->u8Size >= 0) {
		ret = mtk_memout_write_metadata(
			buf->m.planes[buf->length - 1].m.fd, meta_tag,
			pmeta_hdmi_pkt);
		if (ret < 0) {
			//SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	return 0;
}

static int _mtk_memout_fill_hdmirxdata(struct mtk_srccap_dev *srccap_dev,
				       struct v4l2_buffer *buf)
{
	int ret = 0;
	struct meta_srccap_hdmi_pkt meta_hdmi_pkt;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (srccap_dev->srccap_info.src) {
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		ret = 0;
		break;
	default:
		ret = ENXIO;
		break;
	}

	if (ret == ENXIO) {
		SRCCAP_MSG_ERROR("[%s][%d]\r\n", __func__, __LINE__);
		return -ENXIO;
	}

	if (buf == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf->m.planes[0].m.userptr == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf->memory == V4L2_MEMORY_DMABUF) {
		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_avi_packet_info) *
			       META_SRCCAP_HDMIRX_AVI_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_AVI(
			srccap_dev->srccap_info.src,
			(struct hdmi_avi_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_avi_packet_info) *
				META_SRCCAP_HDMIRX_AVI_PKT_MAX_SIZE);

		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "AVI packet", meta_hdmi_pkt.u8Size);

		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf, SRCCAP_MEMOUT_METATAG_HDMIRX_AVI_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
			sizeof(struct hdmi_gc_packet_info) *
			META_SRCCAP_HDMIRX_GCP_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_GCP(
			srccap_dev->srccap_info.src,
			(struct hdmi_gc_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_gc_packet_info) *
				META_SRCCAP_HDMIRX_GCP_PKT_MAX_SIZE);

		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
			"HMDI port", srccap_dev->srccap_info.src,
			"GCP packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf, SRCCAP_MEMOUT_METATAG_HDMIRX_GCP_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			//SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_packet_info) *
			       META_SRCCAP_HDMIRX_SPD_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_gnl(
			srccap_dev->srccap_info.src,
			V4L2_EXT_HDMI_PKT_SPD,
			(struct hdmi_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_packet_info) *
				META_SRCCAP_HDMIRX_SPD_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "SPD packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf, SRCCAP_MEMOUT_METATAG_HDMIRX_SPD_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_vsif_packet_info) *
			       META_SRCCAP_HDMIRX_VSIF_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_VSIF(
			srccap_dev->srccap_info.src,
			(struct hdmi_vsif_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_vsif_packet_info) *
				META_SRCCAP_HDMIRX_VSIF_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "VSIF packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf, SRCCAP_MEMOUT_METATAG_HDMIRX_VSIF_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct meta_srccap_hdr_pkt) *
			       META_SRCCAP_HDMIRX_HDR_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_HDRIF(
			srccap_dev->srccap_info.src,
			(struct hdmi_hdr_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_hdr_packet_info) *
				META_SRCCAP_HDMIRX_HDR_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "HDR_VSIF packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf, SRCCAP_MEMOUT_METATAG_HDMIRX_HDR_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_packet_info) *
			       META_SRCCAP_HDMIRX_AUI_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_gnl(
			srccap_dev->srccap_info.src,
			V4L2_EXT_HDMI_PKT_AUI,
			(struct hdmi_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_packet_info) *
				META_SRCCAP_HDMIRX_AUI_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "AUI packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf, SRCCAP_MEMOUT_METATAG_HDMIRX_AUI_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_packet_info) *
			       META_SRCCAP_HDMIRX_MPG_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_gnl(
			srccap_dev->srccap_info.src,
			V4L2_EXT_HDMI_PKT_MPEG,
			(struct hdmi_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_packet_info) *
				META_SRCCAP_HDMIRX_MPG_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "MPG packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf, SRCCAP_MEMOUT_METATAG_HDMIRX_MPG_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_emp_packet_info) *
			       META_SRCCAP_HDMIRX_VS_EMP_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_VS_EMP(
			srccap_dev->srccap_info.src,
			(struct hdmi_emp_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_emp_packet_info) *
				META_SRCCAP_HDMIRX_VS_EMP_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "VS_EMP packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf,
			SRCCAP_MEMOUT_METATAG_HDMIRX_VS_EMP_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_emp_packet_info) *
			       META_SRCCAP_HDMIRX_DSC_EMP_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_DSC_EMP(
			srccap_dev->srccap_info.src,
			(struct hdmi_emp_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_emp_packet_info) *
				META_SRCCAP_HDMIRX_DSC_EMP_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "DSC_EMP packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf,
			SRCCAP_MEMOUT_METATAG_HDMIRX_DSC_EMP_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_emp_packet_info) *
			       META_SRCCAP_HDMIRX_DHDR_EMP_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_Dynamic_HDR_EMP(
			srccap_dev->srccap_info.src,
			(struct hdmi_emp_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_emp_packet_info) *
			META_SRCCAP_HDMIRX_DHDR_EMP_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "Dynamic_HDR_EMP packet",
				 meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf,
			SRCCAP_MEMOUT_METATAG_HDMIRX_DHDR_EMP_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		memset(meta_hdmi_pkt.u8Data, 0,
		       sizeof(struct hdmi_emp_packet_info) *
			       META_SRCCAP_HDMIRX_VRR_EMP_PKT_MAX_SIZE);

		meta_hdmi_pkt.u8Size = mtk_srccap_hdmirx_pkt_get_VRR_EMP(
			srccap_dev->srccap_info.src,
			(struct hdmi_emp_packet_info *)meta_hdmi_pkt.u8Data,
			sizeof(struct hdmi_emp_packet_info) *
				META_SRCCAP_HDMIRX_VRR_EMP_PKT_MAX_SIZE);
		SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d  %s=%d\n",
				 "HMDI port", srccap_dev->srccap_info.src,
				 "VRR EMP packet", meta_hdmi_pkt.u8Size);
		ret = _mtk_memout_write_hdmirxdata(
			srccap_dev, buf,
			SRCCAP_MEMOUT_METATAG_HDMIRX_VRR_EMP_PKT,
			&meta_hdmi_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	return 0;
}

int mtk_memout_dqbuf(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_buffer *buf)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (buf->m.planes[0].m.userptr == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	int plane_cnt = 0;
	u32 hw_timestamp = 0;
	u64 timestamp_in_us = 0;
	u16 scaled_width = 0, scaled_height = 0;
	struct meta_srccap_frame_info frame_info;

	struct meta_srccap_color_info color_info;
	static u32 frame_id[SRCCAP_DEV_NUM] = {0};
	struct meta_srccap_hdr_pkt hdr_pkt;
	struct meta_srccap_hdr_vsif_pkt hdr_vsif_pkt;
	u16 dev_id = srccap_dev->dev_id;
	struct meta_srccap_dip_point_info dip_point_info;
	struct srccap_memout_buffer_entry *entry = NULL;

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	if (atrace_enable_srccap == 1) {
		SRCCAP_ATRACE_BEGIN("mtk_memout_dqbuf");
		SRCCAP_ATRACE_INT_FORMAT(buf->index,  "42-S: [start][srccap]dqbuf_%u", dev_id);
		SRCCAP_ATRACE_INT_FORMAT(buf_cnt[dev_id],  "buf_cnt_%u", dev_id);
	}
#endif

	memset(&frame_info, 0, sizeof(struct meta_srccap_frame_info));
	memset(&color_info, 0, sizeof(struct meta_srccap_color_info));
	memset(&hdr_pkt, 0, sizeof(struct meta_srccap_hdr_pkt));
	memset(&hdr_vsif_pkt, 0, sizeof(struct meta_srccap_hdr_vsif_pkt));
	memset(&dip_point_info, 0, sizeof(struct meta_srccap_dip_point_info));

	if (srccap_dev->memout_info.frame_id == 0)
		frame_id[srccap_dev->dev_id] = 0;

	frame_info.frame_id = frame_id[srccap_dev->dev_id]++;
	srccap_dev->memout_info.frame_id = frame_id[srccap_dev->dev_id];

	/* prepare metadata info */
	ret = memout_map_source_meta(srccap_dev->srccap_info.src, &frame_info.source);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = memout_map_path_meta(srccap_dev->dev_id, &frame_info.path);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_get_w_index(srccap_dev->dev_id, &frame_info.w_index);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

 #ifndef CONFIG_MSTAR_ARM_BD_FPGA
	ret = mtk_common_updata_frame_color_info(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
#endif

	/* get timestamp */
	SRCCAP_GET_TIMESTAMP(&buf->timestamp);

	ret = memout_process_timestamp(srccap_dev, &hw_timestamp, &timestamp_in_us);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

 #ifndef CONFIG_MSTAR_ARM_BD_FPGA
	ret = mtk_memout_updata_hdmi_packet_info(srccap_dev, &hdr_pkt, &hdr_vsif_pkt);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
#endif

	frame_info.x = srccap_dev->timingdetect_info.data.h_start;
	frame_info.y = srccap_dev->timingdetect_info.data.v_start;
	frame_info.width = srccap_dev->timingdetect_info.data.h_de;
	frame_info.height = srccap_dev->timingdetect_info.data.v_de;
	frame_info.frameratex1000 = srccap_dev->timingdetect_info.data.v_freqx1000;
	frame_info.interlaced = srccap_dev->timingdetect_info.data.interlaced;
	frame_info.h_total = srccap_dev->timingdetect_info.data.h_total;
	frame_info.v_total = srccap_dev->timingdetect_info.data.v_total;
	frame_info.ktimes.tv_sec = buf->timestamp.tv_sec;
	frame_info.ktimes.tv_usec = buf->timestamp.tv_usec;
	frame_info.timestamp = timestamp_in_us;
	frame_info.vrr_type = srccap_dev->timingdetect_info.data.vrr_type;
	frame_info.refresh_rate = srccap_dev->timingdetect_info.data.refresh_rate;

	scaled_width = (srccap_dev->dscl_info.scaled_size_meta.width == 0) ?
		srccap_dev->dscl_info.dscl_size.input.width :
		srccap_dev->dscl_info.scaled_size_meta.width;
	frame_info.scaled_width = scaled_width;

	scaled_height = (srccap_dev->dscl_info.scaled_size_meta.height == 0) ?
		srccap_dev->dscl_info.dscl_size.input.height :
		srccap_dev->dscl_info.scaled_size_meta.height;
	frame_info.scaled_height = scaled_height;

	switch (srccap_dev->memout_info.buf_ctrl_mode) {
	case V4L2_MEMOUT_SWMODE:
		mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
		entry =  memout_delete_last_entry(
			&srccap_dev->memout_info.frame_q.outputq_head);
		if (entry == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);
			return -ENOMEM;
		}

		SRCCAP_MSG_DEBUG("[outq->](%u): vb:%p vb_index:%d stage:%d",
			srccap_dev->dev_id,
			entry->vb, entry->vb->index, entry->stage);
		for (plane_cnt = 0; plane_cnt < SRCCAP_MEMOUT_MAX_BUF_PLANE_NUM; plane_cnt++)
			SRCCAP_MSG_DEBUG(" addr%d=%d", plane_cnt, entry->addr[plane_cnt]);
		SRCCAP_MSG_DEBUG(" index:%d field:%u\n", entry->w_index, entry->field);

#ifdef M6L_SW_MODE_EN
		if (srccap_dev->srccap_info.cap.hw_version == 1)
#endif
			frame_info.w_index = entry->w_index;
#ifdef M6L_SW_MODE_EN
		else
			frame_info.w_index = 0;
#endif

		frame_info.field = entry->field;

		kfree(entry);
		mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_buffer_q);

		break;
	case V4L2_MEMOUT_HWMODE:
		frame_info.w_index = (srccap_dev->memout_info.first_frame == true) ?
			(frame_info.w_index + SRCCAP_MEMOUT_FRAME_NUM -
			SRCCAP_MEMOUT_ONE_INDEX) % SRCCAP_MEMOUT_FRAME_NUM :
			(frame_info.w_index + SRCCAP_MEMOUT_FRAME_NUM -
			SRCCAP_MEMOUT_TWO_INDEX) % SRCCAP_MEMOUT_FRAME_NUM;

		/* get field info */
		memout_get_field_info(srccap_dev, frame_info.w_index);
		frame_info.field = srccap_dev->memout_info.field_info;

		break;
	case V4L2_MEMOUT_BYPASSMODE:
	default:
		frame_info.w_index = 0;

		break;
	}

	SRCCAP_MSG_DEBUG("first_frame:%d, frame_info.w_index:%u\n",
		srccap_dev->memout_info.first_frame,
		frame_info.w_index);

	/* DV */
	frame_info.dv_crc_mute = srccap_dev->dv_info.descrb_info.buf.neg_ctrl;
	frame_info.dv_game_mode = srccap_dev->dv_info.descrb_info.common.dv_game_mode;

	srccap_dev->memout_info.first_frame = false;
	srccap_dev->dscl_info.scaled_size_meta.width = srccap_dev->dscl_info.scaled_size_ml.width;
	srccap_dev->dscl_info.scaled_size_meta.height = srccap_dev->dscl_info.scaled_size_ml.height;

	srccap_dev->dscl_info.scaled_size_ml.width = srccap_dev->dscl_info.dscl_size.output.width;
	srccap_dev->dscl_info.scaled_size_ml.height = srccap_dev->dscl_info.dscl_size.output.height;
	frame_info.frame_pitch = srccap_dev->memout_info.frame_pitch[0];
	frame_info.hoffset = srccap_dev->memout_info.hoffset;

	ret = memout_get_dip_point_info(srccap_dev, &dip_point_info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = mtk_common_get_cfd_metadata(srccap_dev, &color_info);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	SRCCAP_MSG_DEBUG("dscl menuload size(%d, %d), tmp meta size:(%d, %d)\n",
		srccap_dev->dscl_info.scaled_size_ml.width,
		srccap_dev->dscl_info.scaled_size_ml.height,
		srccap_dev->dscl_info.scaled_size_meta.width,
		srccap_dev->dscl_info.scaled_size_meta.height);

	/* write metadata */
	switch (buf->memory) {
	case V4L2_MEMORY_DMABUF:
		ret = mtk_memout_write_metadata(buf->m.planes[buf->length - 1].m.fd,
			SRCCAP_MEMOUT_METATAG_FRAME_INFO, &frame_info);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		ret = mtk_memout_write_metadata(buf->m.planes[buf->length - 1].m.fd,
			SRCCAP_MEMOUT_METATAG_COLOR_INFO, &color_info);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		//fix me
		ret = mtk_memout_write_metadata(buf->m.planes[buf->length - 1].m.fd,
			SRCCAP_MEMOUT_METATAG_HDR_PKT, &hdr_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		ret = mtk_memout_write_metadata(buf->m.planes[buf->length - 1].m.fd,
			SRCCAP_MEMOUT_METATAG_HDR_VSIF_PKT, &hdr_vsif_pkt);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		ret = mtk_memout_write_metadata(buf->m.planes[buf->length - 1].m.fd,
			SRCCAP_MEMOUT_METATAG_DIP_POINT_INFO, &dip_point_info);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("Invalid memory type\n");
		return -EINVAL;
	}

	SRCCAP_MSG_DEBUG("[CTRL_FLOW]metadata: %s=%d, %s=%d, %s=%d, %s=%u, %s=%d, %s=%u, %s=%u\n",
		"source", frame_info.source,
		"path", frame_info.path,
		"bypass_ipdma", frame_info.bypass_ipdma,
		"w_index", frame_info.w_index,
		"format", frame_info.format,
		"x", frame_info.x,
		"y", frame_info.y);
	SRCCAP_MSG_DEBUG("%s=%u, %s=%u, %s=%u, %s=%d, %s=%u, %s=%u, %s=%u, %s=%lu, %s=%d, %s=%d\n",
		"width", frame_info.width,
		"height", frame_info.height,
		"frameratex1000", frame_info.frameratex1000,
		"interlaced", frame_info.interlaced,
		"h_total", frame_info.h_total,
		"v_total", frame_info.v_total,
		"pixels_aligned", frame_info.pixels_aligned,
		"timestamp", frame_info.timestamp,
		"vrr_type", frame_info.vrr_type,
		"refresh_rate", frame_info.refresh_rate);

	SRCCAP_MSG_DEBUG("%s=%u, %s=%u, %s=%u.%u\n",
		"scaled_width", frame_info.scaled_width,
		"scaled_height", frame_info.scaled_height,
		"ktimes", frame_info.ktimes.tv_sec, frame_info.ktimes.tv_usec);

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	_mtk_memout_fill_hdmirxdata(srccap_dev, buf);
#endif

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
	if (atrace_enable_srccap == 1) {
		if (buf_cnt[dev_id] > 0)
			buf_cnt[dev_id]--;
		SRCCAP_ATRACE_END("mtk_memout_dqbuf");
		SRCCAP_ATRACE_INT_FORMAT(buf->index,  "42-E: [end][srccap]dqbuf_%u", dev_id);
		SRCCAP_ATRACE_INT_FORMAT(buf_cnt[dev_id],  "buf_cnt_%u", dev_id);
	}
#endif

	return 0;
}

int mtk_memout_streamon(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_buf_type type)
{
	int ret = 0;
	u8 dev_id = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = memout_init_buffer_queue(srccap_dev);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_frame_num(dev_id,
		SRCCAP_MEMOUT_FRAME_NUM, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_rw_diff(dev_id, SRCCAP_MEMOUT_RW_DIFF, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	srccap_dev->memout_info.first_frame = true;

	return 0;
}

int mtk_memout_streamoff(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_buf_type type)
{
	int ret = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	srccap_dev->memout_info.frame_id = 0;
	srccap_dev->memout_info.first_frame = true;

	memout_deinit_buffer_queue(srccap_dev);

	/* disable access at the end of streamoff to prevent memory hit */
	mdelay((SRCCAP_MEMOUT_24FPS_FRAME_TIME) +
			(SRCCAP_MEMOUT_24FPS_FRAME_TIME));

	ret = drv_hwreg_srccap_memout_set_access(srccap_dev->dev_id, 0, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_reset_ipdma(srccap_dev->dev_id, TRUE, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_reset_ipdma(srccap_dev->dev_id, FALSE, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

void mtk_memout_vsync_isr(
	void *param)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	spinlock_t *spinlock_buffer_handling_task = NULL;
	unsigned long flags = 0;

	if (param == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	srccap_dev = (struct mtk_srccap_dev *)param;

	spinlock_buffer_handling_task =
		&srccap_dev->srccap_info.spinlock_list.spinlock_buffer_handling_task;

	spin_lock_irqsave(spinlock_buffer_handling_task, flags);
	if (srccap_dev->srccap_info.buffer_handling_task != NULL) {
		srccap_dev->srccap_info.waitq_list.buffer_done = 1;
		wake_up_interruptible(&srccap_dev->srccap_info.waitq_list.waitq_buffer);
	}
	spin_unlock_irqrestore(spinlock_buffer_handling_task, flags);
}

int mtk_srccap_register_memout_subdev(
	struct v4l2_device *v4l2_dev,
	struct v4l2_subdev *subdev_memout,
	struct v4l2_ctrl_handler *memout_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(memout_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(memout_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(memout_ctrl_handler,
			&memout_ctrl[ctrl_count], NULL);
	}

	ret = memout_ctrl_handler->error;
	if (ret) {
		SRCCAP_MSG_ERROR("failed to create memout ctrl handler\n");
		goto exit;
	}
	subdev_memout->ctrl_handler = memout_ctrl_handler;

	v4l2_subdev_init(subdev_memout, &memout_sd_ops);
	subdev_memout->internal_ops = &memout_sd_internal_ops;
	strlcpy(subdev_memout->name, "mtk-memout", sizeof(subdev_memout->name));

	ret = v4l2_device_register_subdev(v4l2_dev, subdev_memout);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to register memout subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(memout_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_memout_subdev(struct v4l2_subdev *subdev_memout)
{
	v4l2_ctrl_handler_free(subdev_memout->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_memout);
}

void mtk_memout_set_capture_size(
	struct mtk_srccap_dev *srccap_dev,
	struct srccap_ml_script_info *ml_script_info,
	struct v4l2_area_size *capture_size)
{
	int ret = 0;
	u16 count = 0;
	u32 width = 0, height = 0;
	struct reg_info reg[SRCCAP_MEMOUT_REG_NUM] = {{0}};
	bool stub = 0;
	struct sm_ml_direct_write_mem_info ml_direct_write_info;
	struct v4l2_area_size size;

	if ((srccap_dev == NULL) || (ml_script_info == NULL) || (capture_size == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	memset(&ml_direct_write_info, 0, sizeof(struct sm_ml_direct_write_mem_info));

		SRCCAP_MSG_DEBUG("capture_size:(%u, %u), count:%d\n",
			capture_size->width,
			capture_size->height,
			count);

	width = capture_size->width;
	height = (srccap_dev->timingdetect_info.data.interlaced == TRUE) ?
		capture_size->height / SRCCAP_INTERLACE_HEIGHT_DIVISOR :
		capture_size->height;

	ret = drv_hwreg_srccap_memout_set_ipdmaw_size(srccap_dev->dev_id,
					false,
					width,
					height,
					reg, &count);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

	ret = drv_hwreg_common_get_stub(&stub);
	if (ret < 0)
		SRCCAP_MSG_RETURN_CHECK(ret);

	ml_script_info->memout_capture_size_cmd_count = count;

	if (stub == 0) {
		//3.add cmd
		ml_direct_write_info.reg = (struct sm_reg *)&reg;
		ml_direct_write_info.cmd_cnt = count;
		ml_direct_write_info.va_address = ml_script_info->start_addr
				+ (ml_script_info->total_cmd_count  * SRCCAP_ML_CMD_SIZE);
		ml_direct_write_info.va_max_address = ml_script_info->max_addr;
		ml_direct_write_info.add_32_support = FALSE;

		SRCCAP_MSG_DEBUG("ml_write_info: cmd_count: auto_gen:%d, dscl:%d, ipdmaw:%d\n",
			ml_script_info->auto_gen_cmd_count,
			ml_script_info->dscl_cmd_count,
			count);
		SRCCAP_MSG_DEBUG("ml_write_info: va_address:0x%x,  va_max_address:0x%x\n",
			ml_direct_write_info.va_address,
			ml_direct_write_info.va_max_address);

		ret = sm_ml_write_cmd(&ml_direct_write_info);
		if (ret != E_SM_RET_OK)
			SRCCAP_MSG_ERROR("sm_ml_write_cmd fail, ret:%d\n", ret);
	}
	ml_script_info->total_cmd_count = ml_script_info->total_cmd_count
					+ ml_script_info->memout_capture_size_cmd_count;
}

