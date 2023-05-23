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

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include "linux/metadata_utility/m_srccap.h"
#include "mtk_srccap.h"
#include "mtk_srccap_memout.h"
#include "hwreg_srccap_memout.h"

#include "mtk_iommu_dtv_api.h"
#include "metadata_utility.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static int iommudd_ioctl(struct file *file, unsigned int req, void *arg)
{
	int ret = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_ioctl(file, req, (unsigned long)arg);
	set_fs(old_fs);

	return ret;
}

static int get_iova(int fd, unsigned long long *iova, unsigned int *size)
{
	if ((fd == 0) || (iova == NULL) || (size == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	struct file *file = NULL;
	struct mdiommu_ioc_data data;

	memset(&data, 0, sizeof(struct mdiommu_ioc_data));

	file = filp_open("/dev/iommu_mtkd", O_RDWR, 0);
	if (IS_ERR(file)) {
		SRCCAP_MSG_FATAL("corrupted file\n");
		return -ENOENT;
	}

	data.fd = fd;
	ret = iommudd_ioctl(file, MDIOMMU_IOC_GETIOVA, &data);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	*iova = data.addr;
	*size = data.len;

	ret = filp_close(file, NULL);
	if (ret < 0) {
		SRCCAP_MSG_FATAL("corrupted file\n");
		return -ENOENT;
	}

	return 0;
}

static int map_fd(int fd, void **va, u64 *size)
{
	if ((fd == 0) || (va == NULL) || (size == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	if (*va != NULL) {
		SRCCAP_MSG_ERROR("occupied va\n");
		return -EINVAL;
	}

	int ret = 0;
	struct dma_buf *db = NULL;
	unsigned long long iova = 0;

	db = dma_buf_get(fd);
	if (db == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	*size = db->size;

	ret = dma_buf_begin_cpu_access(db, DMA_BIDIRECTIONAL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		dma_buf_put(db);
		return ret;
	}

	*va = dma_buf_vmap(db);
	if ((*va == NULL) || (*va == -1)) {
		SRCCAP_MSG_ERROR("kernel address not valid\n");
		dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
		dma_buf_put(db);
		return -ENXIO;
	}

	dma_buf_put(db);
	SRCCAP_MSG_INFO("va=0x%llx, size=%llu\n", *va, *size);

	return 0;
}

static int unmap_fd(int fd, void *va)
{
	if ((fd == 0) || (va == NULL)) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	struct dma_buf *db = NULL;

	db = dma_buf_get(fd);
	if (db == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	dma_buf_vunmap(db, va);
	dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
	dma_buf_put(db);

	return 0;
}

static int get_metaheader(
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
	default:
		SRCCAP_MSG_ERROR("invalid metadata tag:%d\n", meta_tag);
		return -EINVAL;
	}

	return 0;
}

static int v4l2_source_to_meta_source(
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

static int dev_id_to_meta_memout_path(u16 dev_id, enum m_memout_path *meta_path)
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

static int hdmirx_format_to_ip_format(
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

static int get_ip_fmt(
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
		hdmirx_format_to_ip_format(hdmi_fmt, ip_fmt);
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
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
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
		break;
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		*ip_fmt = REG_SRCCAP_MEMOUT_FORMAT_YUV422;
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
	pr_err("[SRCCAP][%s]buf_ctrl_mode:%ld\n",
		__func__,
		mode);

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

	*min_bufs = 3; // to be fixed by min_bufs

	return 0;
}

static int _memout_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler,
		struct mtk_srccap_dev, memout_ctrl_handler);
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
#ifdef CONFIG_OPTEE
		struct v4l2_memout_sst_info *sstinfo =
			(struct v4l2_memout_sst_info *)(ctrl->p_new.p_u8);

		SRCCAP_MSG_DEBUG("V4L2_CID_MEMOUT_SECURE_SRC_TABLE\n");

		ret = mtk_memout_get_sst(srccap_dev, sstinfo);
#endif
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

	srccap_dev = container_of(ctrl->handler,
		struct mtk_srccap_dev, memout_ctrl_handler);
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
		if (ctrl->p_new.p == NULL) {
			SRCCAP_MSG_POINTER_CHECK();
			return -EINVAL;
		}
#ifdef CONFIG_OPTEE
		struct v4l2_memout_secure_info secureinfo =
			*(struct v4l2_memout_secure_info *)
			(ctrl->p_new.p);

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
		.name = "Srccap MemOut SECURE SOURCE TABLE",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_memout_sst_info)},
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
	if (meta_content == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = map_fd(meta_fd, &va, &size);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = get_metaheader(meta_tag, &header);
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
	unmap_fd(meta_fd, va);
	return ret;
}

int mtk_memout_write_metadata(
	int meta_fd,
	enum srccap_memout_metatag meta_tag,
	void *meta_content)
{
	if (meta_content == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = map_fd(meta_fd, &va, &size);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = get_metaheader(meta_tag, &header);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;

	meta_ret = attach_metadata(&buffer, header, meta_content);
	if (!meta_ret) {
		SRCCAP_MSG_ERROR("metadata not attached\n");
		ret = -EPERM;
		goto EXIT;
	}

EXIT:
	unmap_fd(meta_fd, va);
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

	ret = map_fd(meta_fd, &va, &size);
	if (ret) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto EXIT;
	}

	ret = get_metaheader(meta_tag, &header);
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
	unmap_fd(meta_fd, va);
	return ret;
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
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (format == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	int plane_cnt = 0;
	u8 dev_id = (u8)srccap_dev->dev_id;
	struct reg_srccap_memout_format_conversion fmt_conv;
	struct reg_srccap_memout_resolution res;
	bool pack = FALSE, argb = FALSE;
	u8 num_planes = 0, num_bufs = 0, access_mode = 0;
	u8 bitmode_alpha = 0, bitmode[SRCCAP_MAX_PLANE_NUM] = {0}, ce[SRCCAP_MAX_PLANE_NUM] = {0};
	u8 bitwidth[SRCCAP_MAX_PLANE_NUM] = {0};
	u32 hoffset = 0, vsize = 0, align = 0, pitch[SRCCAP_MAX_PLANE_NUM] = {0};

	memset(&fmt_conv, 0, sizeof(struct reg_srccap_memout_format_conversion));
	memset(&res, 0, sizeof(struct reg_srccap_memout_resolution));

	switch (format->fmt.pix_mp.pixelformat) {
	/* ------------------ RGB format ------------------ */
	/* 3 plane -- contiguous planes */
	case V4L2_PIX_FMT_RGB_8_8_8_CE:
		num_planes = 3;
		num_bufs = 1;
		pack = 0;
		fmt_conv.ipdma_fmt = REG_SRCCAP_MEMOUT_FORMAT_RGB;
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

	/* set format */
	/* TO BE FIXED: need input format from CFD */
	ret = get_ip_fmt(srccap_dev->srccap_info.src, srccap_dev->hdmirx_info.color_format,
		&fmt_conv.ip_fmt);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}
	fmt_conv.en = (fmt_conv.ip_fmt != fmt_conv.ipdma_fmt) ? TRUE : FALSE;
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

	/* set access mode */
	ret = drv_hwreg_srccap_memout_set_access(dev_id, access_mode, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* set bit mode */
	ret = drv_hwreg_srccap_memout_set_bit_mode(dev_id, bitmode, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* set compression */
	ret = drv_hwreg_srccap_memout_set_compression(dev_id, ce, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* set input resolution */
	res.hsize = format->fmt.pix_mp.width;
	res.vsize = format->fmt.pix_mp.height;

	ret = drv_hwreg_srccap_memout_set_input_resolution(dev_id, res, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* set line offset */
	hoffset = format->fmt.pix_mp.width;

	ret = drv_hwreg_srccap_memout_set_line_offset(
		dev_id, hoffset, TRUE, NULL, NULL); // unit: pixel
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* set frame pitch */
	vsize = format->fmt.pix_mp.height;
	align = srccap_dev->srccap_info.srccap_cap.bpw;
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

	ret = drv_hwreg_srccap_memout_set_frame_pitch(
		dev_id, pitch, TRUE, NULL, NULL); // unit: n bits, n = align
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	/* return buffer number */
	format->fmt.pix_mp.num_planes = num_bufs;

	/* save format info */
	srccap_dev->memout_info.fmt_mp = format->fmt.pix_mp;
	srccap_dev->memout_info.num_planes = num_planes;
	srccap_dev->memout_info.num_bufs = num_bufs;
	srccap_dev->memout_info.frame_pitch[0] = pitch[0] * align / SRCCAP_BYTE_SIZE; // unit: byte
	srccap_dev->memout_info.frame_pitch[1] = pitch[1] * align / SRCCAP_BYTE_SIZE; // unit: byte
	srccap_dev->memout_info.frame_pitch[2] = pitch[2] * align / SRCCAP_BYTE_SIZE; // unit: byte

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
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (vb == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -EINVAL;
	}

	int ret = 0;
	int plane_cnt = 0;
	u8 num_planes = srccap_dev->memout_info.num_planes;
	u32 align = srccap_dev->srccap_info.srccap_cap.bpw;
	u8 dev_id = (u8)srccap_dev->dev_id;
	u64 offset = 0;
	u64 addr[3] = {0};
	unsigned int size = 0;
	bool contiguous = FALSE;
	struct v4l2_memout_buf memout_buf[3] = {{0} };

	if (vb->memory == V4L2_MEMORY_USERPTR) {
		for (plane_cnt = 0; plane_cnt < vb->num_planes; ++plane_cnt) {
			if (!vb->planes[plane_cnt].m.userptr) {
				SRCCAP_MSG_POINTER_CHECK();
				return -EINVAL;
			}

			memout_buf[plane_cnt] =
				*(struct v4l2_memout_buf *)vb->planes[plane_cnt].m.userptr;
			ret = get_iova(memout_buf[plane_cnt].frame_fd, &addr[plane_cnt], &size);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}
		}
	}

	if (vb->memory == V4L2_MEMORY_DMABUF) {
		contiguous = srccap_dev->memout_info.num_bufs < num_planes;
		if (contiguous) {
			ret = get_iova(vb->planes[0].m.fd, &addr[0], &size);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				return ret;
			}

			/* calculate frame addresses for contiguous planes */
			offset = 0;
			for (plane_cnt = 0; plane_cnt < num_planes; plane_cnt++) {
				addr[plane_cnt] = addr[0] + offset;
				offset += srccap_dev->memout_info.frame_pitch[plane_cnt];
			}
		} else {
			for (plane_cnt = 0; plane_cnt < num_planes; ++plane_cnt) {
				if (vb->planes[plane_cnt].m.fd == 0)
					return -EINVAL;

				ret = get_iova(vb->planes[plane_cnt].m.fd, &addr[plane_cnt], &size);
				if (ret < 0) {
					SRCCAP_MSG_RETURN_CHECK(ret);
					return ret;
				}
			}
		}
	}

	/* set base address */
	for (plane_cnt = 0; plane_cnt < num_planes; plane_cnt++)
		addr[plane_cnt] = addr[plane_cnt] * SRCCAP_BYTE_SIZE / align; // unit: word
	ret = drv_hwreg_srccap_memout_set_base_addr(dev_id, addr, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
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
	struct v4l2_memout_buf memout_buf;
	struct meta_srccap_frame_info frame_info;

	memset(&memout_buf, 0, sizeof(struct v4l2_memout_buf));
	memset(&frame_info, 0, sizeof(struct meta_srccap_frame_info));

	/* prepare metadata info */
	if (buf->memory == V4L2_MEMORY_USERPTR)
		memout_buf = *(struct v4l2_memout_buf *)buf->m.planes[0].m.userptr;

	ret = v4l2_source_to_meta_source(srccap_dev->srccap_info.src, &frame_info.source);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = dev_id_to_meta_memout_path(srccap_dev->dev_id, &frame_info.path);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_get_w_index(srccap_dev->dev_id, &frame_info.w_index);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

#ifdef T32_HDMI_SIMULATION
	frame_info.width = srccap_dev->timingdetect_info.h_de;
	frame_info.height = srccap_dev->timingdetect_info.v_de;
	frame_info.frameratex1000 = srccap_dev->timingdetect_info.v_freqx1000;
	frame_info.interlaced = srccap_dev->timingdetect_info.interlaced;
	frame_info.h_total = srccap_dev->timingdetect_info.h_total;
	frame_info.v_total = srccap_dev->timingdetect_info.v_total;
#else
	frame_info.width = 1920; // TODO: obtain from timingdetect
	frame_info.height = 1080; // TODO: obtain from timingdetect
	frame_info.frameratex1000 = 60000; // TODO: obtain from timingdetect
	frame_info.interlaced = FALSE; // TODO: obtain from timingdetect
	frame_info.h_total = 2250; // TODO: obtain from timingdetect
	frame_info.v_total = 1125; // TODO: obtain from timingdetect
#endif
	frame_info.vrr_type = TYPE_NVRR; // TODO: obtain vrr_en from timingdetect

	/* write metadata */
	if (buf->memory == V4L2_MEMORY_USERPTR)
		mtk_memout_write_metadata(memout_buf.meta_fd,
			SRCCAP_MEMOUT_METATAG_FRAME_INFO, &frame_info);

	if (buf->memory == V4L2_MEMORY_DMABUF)
		mtk_memout_write_metadata(buf->m.planes[buf->length - 1].m.fd,
			SRCCAP_MEMOUT_METATAG_FRAME_INFO, &frame_info);

	/* get timestamp */
	SRCCAP_GET_TIMESTAMP(&buf->timestamp);

	return 0;
}

int mtk_memout_streamon(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_buf_type type)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u8 dev_id = (u8)srccap_dev->dev_id;

	ret = drv_hwreg_srccap_memout_set_frame_num(dev_id, 5, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	ret = drv_hwreg_srccap_memout_set_rw_diff(dev_id, 2, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

int mtk_memout_streamoff(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_buf_type type)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	return 0;
}

int mtk_srccap_register_memout_subdev(
	struct v4l2_device *srccap_dev,
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
		v4l2_err(srccap_dev, "failed to create memout ctrl handler\n");
		goto exit;
	}
	subdev_memout->ctrl_handler = memout_ctrl_handler;

	v4l2_subdev_init(subdev_memout, &memout_sd_ops);
	subdev_memout->internal_ops = &memout_sd_internal_ops;
	strlcpy(subdev_memout->name, "mtk-memout", sizeof(subdev_memout->name));

	ret = v4l2_device_register_subdev(srccap_dev, subdev_memout);
	if (ret) {
		v4l2_err(srccap_dev, "failed to register memout subdev\n");
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

