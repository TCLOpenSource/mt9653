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
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-dma-contig.h>

#include "mtk_pq.h"
#include "mtk_pq_common.h"
#include "mtk_pq_enhance.h"
#include "mtk_pq_xc.h"
#include "mtk_pq_display.h"
#include "mtk_pq_b2r.h"
#include "mtk_pq_buffer.h"
#include "mtk_iommu_dtv_api.h"
#include "mtk_pq_svp.h"

#include "apiXC.h"

#define PQ_V4L2_DEVICE_NAME		"v4l2-mtk-pq"
#define PQ_VIDEO_DRIVER_NAME		"mtk-pq-drv"
#define PQ_VIDEO_DEVICE_NAME		"mtk-pq-dev"
#define PQ_VIDEO_DEVICE_NUM_BASE	1
#define fh_to_ctx(f)	(container_of(f, struct mtk_pq_ctx, fh))

__u32 u32DbgLevel = STI_PQ_LOG_LEVEL_ALL &
	(~(STI_PQ_LOG_LEVEL_BUFFER));

static char *devID[PQ_WIN_MAX_NUM] = {"0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", "10", "11", "12", "13", "14", "15"};

//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
//static ssize_t mtk_pq_capability_show(struct device *dev,
	//struct device_attribute *attr, char *buf);
//static ssize_t mtk_pq_capability_store(struct device *dev,
	//struct device_attribute *attr, const char *buf, size_t count);

//-----------------------------------------------------------------------------
//  Local Functions
//-----------------------------------------------------------------------------
static int _mtk_pq_parse_dts(struct mtk_pq_caps *pqcaps,
	struct platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = &pdev->dev;
	struct device_node *bank_node;

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get capability node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "TCH_color",
		&pqcaps->u32TCH_color);
	if (ret) {
		PQ_MSG_ERROR("Failed to get TCH_color resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("TCH_color = %x\r\n", pqcaps->u32TCH_color);

	ret = of_property_read_u32(bank_node, "PreSharp",
		&pqcaps->u32PreSharp);
	if (ret) {
		PQ_MSG_ERROR("Failed to get PreSharp resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("PreSharp = %x\r\n", pqcaps->u32PreSharp);

	ret = of_property_read_u32(bank_node, "2D_Peaking",
		&pqcaps->u322D_Peaking);
	if (ret) {
		PQ_MSG_ERROR("Failed to get 2D_Peaking resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("2D_Peaking = %x\r\n", pqcaps->u322D_Peaking);

	ret = of_property_read_u32(bank_node, "LCE", &pqcaps->u32LCE);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LCE resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("LCE = %x\r\n", pqcaps->u32LCE);

	ret = of_property_read_u32(bank_node, "3D_LUT",
		&pqcaps->u323D_LUT);
	if (ret) {
		PQ_MSG_ERROR("Failed to get 3D_LUT resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("3D_LUT = %x\r\n", pqcaps->u323D_LUT);

	ret = of_property_read_u32(bank_node, "TS_Input",
		&pqcaps->u32TS_Input);
	if (ret) {
		PQ_MSG_ERROR("Failed to get TS_Input resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("TS_Input = %x\r\n", pqcaps->u32TS_Input);

	ret = of_property_read_u32(bank_node, "TS_Output",
		&pqcaps->u32TS_Output);
	if (ret) {
		PQ_MSG_ERROR("Failed to get TS_Output resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("TS_Output = %x\r\n", pqcaps->u32TS_Output);

	ret = of_property_read_u32(bank_node, "DMA_SCL",
		&pqcaps->u32DMA_SCL);
	if (ret) {
		PQ_MSG_ERROR("Failed to get DMA_SCL resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("DMA_SCL = %x\r\n", pqcaps->u32DMA_SCL);

	ret = of_property_read_u32(bank_node, "PQU", &pqcaps->u32PQU);
	if (ret) {
		PQ_MSG_ERROR("Failed to get PQU resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("PQU = %x\r\n", pqcaps->u32PQU);

	ret = of_property_read_u32(bank_node, "Window_Num",
		&pqcaps->u32Window_Num);
	if (ret) {
		PQ_MSG_ERROR("Failed to get Window_Num resource\r\n");
		return -EINVAL;
	}
	PQ_MSG_INFO("Window_Num = %x\r\n", pqcaps->u32Window_Num);

	return 0;
}

static ssize_t mtk_pq_capability_tch_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("TCH_color", pqdev->pqcaps.u32TCH_color);

	return ssize;
}

static ssize_t mtk_pq_capability_tch_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_presharp_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("PreSharp", pqdev->pqcaps.u32PreSharp);

	return ssize;
}

static ssize_t mtk_pq_capability_presharp_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_peaking_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("2D_Peaking", pqdev->pqcaps.u322D_Peaking);

	return ssize;
}

static ssize_t mtk_pq_capability_peaking_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_lce_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("LCE", pqdev->pqcaps.u32LCE);

	return ssize;
}

static ssize_t mtk_pq_capability_lce_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_3dlut_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("3D_LUT", pqdev->pqcaps.u323D_LUT);

	return ssize;
}

static ssize_t mtk_pq_capability_3dlut_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_tsinput_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("TS_Input", pqdev->pqcaps.u32TS_Input);

	return ssize;
}

static ssize_t mtk_pq_capability_tsinput_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_tsoutput_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("TS_Output", pqdev->pqcaps.u32TS_Output);

	return ssize;
}

static ssize_t mtk_pq_capability_tsoutput_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_dmascl_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("DMA_SCL", pqdev->pqcaps.u32DMA_SCL);

	return ssize;
}

static ssize_t mtk_pq_capability_dmascl_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_pqu_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("PQU", pqdev->pqcaps.u32PQU);

	return ssize;
}

static ssize_t mtk_pq_capability_pqu_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_winnum_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = 255;

	PQ_CAPABILITY_SHOW("Window_Num", pqdev->pqcaps.u32Window_Num);

	return ssize;
}

static ssize_t mtk_pq_capability_winnum_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static struct device_attribute mtk_pq_attr[] = {
	__ATTR(pq_capability_tchcolor, (00600),
		mtk_pq_capability_tch_show, mtk_pq_capability_tch_store),
	__ATTR(pq_capability_presharp, (00600),
		mtk_pq_capability_presharp_show,
		mtk_pq_capability_presharp_store),
	__ATTR(pq_capability_peaking, (00600),
		mtk_pq_capability_peaking_show,
		mtk_pq_capability_peaking_store),
	__ATTR(pq_capability_lce, (00600),
		mtk_pq_capability_lce_show, mtk_pq_capability_lce_store),
	__ATTR(pq_capability_3dlut, (00600),
		mtk_pq_capability_3dlut_show, mtk_pq_capability_3dlut_store),
	__ATTR(pq_capability_tsinput, (00600),
		mtk_pq_capability_tsinput_show,
		mtk_pq_capability_tsinput_store),
	__ATTR(pq_capability_tsoutput, (00600),
		mtk_pq_capability_tsoutput_show,
		mtk_pq_capability_tsoutput_store),
	__ATTR(pq_capability_dmascl, (00600),
		mtk_pq_capability_dmascl_show, mtk_pq_capability_dmascl_store),
	__ATTR(pq_capability_pqu, (00600),
		mtk_pq_capability_pqu_show, mtk_pq_capability_pqu_store),
	__ATTR(pq_capability_winnum, (00600),
		mtk_pq_capability_winnum_show, mtk_pq_capability_winnum_store),
};

void mtk_pq_CreateSysFS(struct device *pdv)
{
	int i = 0;

	PQ_MSG_INFO("Device_create_file initialized\n");

	for (i = 0; i < (sizeof(mtk_pq_attr) /
		sizeof(struct device_attribute)); i++) {
		if (device_create_file(pdv, &mtk_pq_attr[i]) != 0)
			PQ_MSG_INFO("Device_create_file_error\n");
		else
			PQ_MSG_INFO("Device_create_file_success\n");
	}
}

static int mtk_pq_querycap(struct file *file,
	void *priv, struct v4l2_capability *cap)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	strncpy(cap->driver, PQ_VIDEO_DRIVER_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, pq_dev->video_dev.name, sizeof(cap->card) - 1);
	cap->bus_info[0] = 0;
	cap->capabilities = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING |
		V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_CAPTURE|
		V4L2_CAP_DEVICE_CAPS;
	cap->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING |
		V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_CAPTURE;
	return 0;
}

static int mtk_pq_s_input(struct file *file, void *fh, unsigned int i)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_set_input(pq_dev, i);
}

static int mtk_pq_g_input(struct file *file, void *fh, unsigned int *i)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_get_input(pq_dev, i);
}

static int mtk_pq_s_fmt_vid_cap_mplane(struct file *file,
	void *fh, struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_set_fmt_cap_mplane(pq_dev, f);
}

static int mtk_pq_s_fmt_vid_out_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_set_fmt_out_mplane(pq_dev, f);
}

static int mtk_pq_g_fmt_vid_cap_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_get_fmt_cap_mplane(pq_dev, f);
}

static int mtk_pq_g_fmt_vid_out_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_get_fmt_out_mplane(pq_dev, f);
}

static void mtk_pq_s_patchflow(struct mtk_pq_device *pq_dev)
{
	//step 1. MApi_XC_Init
	XC_INITDATA XC_InitData;
	INPUT_SOURCE_TYPE_t stInputSrc;

	stInputSrc = mtk_display_transfer_input_source(pq_dev->common_info.input_source);

	memset((void *)&XC_InitData, 0, sizeof(XC_InitData));

	MApi_XC_Init(&XC_InitData, 0);

	//step 1.5 MApi_XC_GenerateBlackVideo
	MApi_XC_GenerateBlackVideo(1, 0);

	MS_S16 s16PathId;
	XC_MUX_PATH_INFO PathInfo;
	XC_MUX_PATH_INFO AllPathInfo[MAX_SYNC_DATA_PATH_SUPPORTED];
	MS_U8 u8Count = 0;

	memset(&PathInfo, 0, sizeof(XC_MUX_PATH_INFO));
	memset(AllPathInfo, 0, sizeof(XC_MUX_PATH_INFO)*MAX_SYNC_DATA_PATH_SUPPORTED);
	//step 2. MApi_XC_Mux_GetPathInfo
	u8Count = MApi_XC_Mux_GetPathInfo(AllPathInfo);

	if (u8Count > 0) {
		do {
			u8Count = u8Count - 1;

			if (AllPathInfo[u8Count].dest == OUTPUT_SCALER_MAIN_WINDOW) {
				if (AllPathInfo[u8Count].src == stInputSrc) {
					STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					    "eInputSrc %u was connected to OUTPUT_SCALER_MAIN_WINDOW.\n",
					    stInputSrc);
				} else {
					if (MApi_XC_Mux_DeletePath(AllPathInfo[u8Count].src,
					    OUTPUT_SCALER_MAIN_WINDOW) == -1)
						STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
						    " Delete path fail: src = %d, dest = %d\n",
						    AllPathInfo[u8Count].src,
						    OUTPUT_SCALER_MAIN_WINDOW);
					break;
				}
			}
		} while (u8Count > 0);
	}
	 PathInfo.Path_Type = PATH_TYPE_SYNCHRONOUS;
	 PathInfo.src = stInputSrc;
	 PathInfo.dest = OUTPUT_SCALER_MAIN_WINDOW;
	 PathInfo.SyncEventHandler = NULL;
	 PathInfo.DestOnOff_Event_Handler = NULL;
	 PathInfo.path_thread = NULL;
	 PathInfo.dest_periodic_handler = NULL;
	//step 3. MApi_XC_Mux_CreatePath
	 s16PathId = MApi_XC_Mux_CreatePath(&PathInfo, sizeof(XC_MUX_PATH_INFO));

	//step 4. MApi_XC_Mux_EnablePath
	 MApi_XC_Mux_EnablePath(s16PathId);

	//step 5. MApi_XC_SetInputSource
	 MApi_XC_SetInputSource(stInputSrc, 0);

	//step 6. MApi_XC_DisableInputSource
	 MApi_XC_DisableInputSource(DISABLE, MAIN_WINDOW);

	 XC_SetTiming_Info stXCTimingInfo;

	 memset(&stXCTimingInfo, 0, sizeof(XC_SetTiming_Info));
	 stXCTimingInfo.u32HighAccurateInputVFreq = 60000;
	 stXCTimingInfo.u16InputVFreq = 600;
	 stXCTimingInfo.u16InputVTotal = 2205;
	 stXCTimingInfo.bInterlace = 0;
	//step 7. MApi_XC_SetPanelTiming
	 MApi_XC_SetPanelTiming(&stXCTimingInfo, 0);
}

static int mtk_pq_streamon(struct file *file,
	void *fh, enum v4l2_buf_type type)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"stream on capture not support!\n");
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		mtk_display_create_task(pq_dev);
		mtk_display_set_digitalsignalinfo(
		    (__u32)pq_dev->dev_indx, pq_dev);
		mtk_pq_s_patchflow(pq_dev);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Stream on fail!Buffer Type = %d\n", type);
		return ret;
	}

	ret = v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"m2m stream on fail!Buffer Type = %d\n", type);
		return ret;
	}

	return ret;
}

static int mtk_pq_streamoff(struct file *file,
		void *fh, enum v4l2_buf_type type)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"stream off capture not support!\n");
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		mtk_display_clear_digitalsignalinfo(
		    (__u32)pq_dev->dev_indx, pq_dev);
		mtk_display_destroy_task(pq_dev);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Stream off fail!Buffer Type = %d\n", type);
		return ret;
	}

	ret = mtk_pq_common_stream_off(pq_dev);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Stream off fail!Buffer Type = %d\n", type);
		return ret;
	}

	ret = v4l2_m2m_streamoff(file, ctx->m2m_ctx, type);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Stream off fail!Buffer Type = %d\n", type);
		return ret;
	}

	return ret;
}

static int mtk_pq_subscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	struct mtk_pq_device *pq_dev = video_get_drvdata(fh->vdev);

	switch (sub->type) {
	case V4L2_EVENT_VSYNC:
		if (mtk_xc_subscribe_event(sub->type, &pq_dev->dev_indx)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to subscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_HDR_SEAMLESS_MUTE:
	case V4L2_EVENT_HDR_SEAMLESS_UNMUTE:
		if (mtk_pq_enhance_subscribe_event(pq_dev, sub->type)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to subscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_MTK_PQ_INPUT_DONE:
	case V4L2_EVENT_MTK_PQ_OUTPUT_DONE:
	case V4L2_EVENT_MTK_PQ_CALLBACK:
	case V4L2_EVENT_MTK_PQ_STREAMOFF:
		if (mtk_display_subscribe_event(sub->type, &pq_dev->dev_indx)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to subscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_subscribe(fh, sub, PQ_NEVENTS, NULL);
}

static int mtk_pq_unsubscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	struct mtk_pq_device *pq_dev = video_get_drvdata(fh->vdev);

	switch (sub->type) {
	case V4L2_EVENT_VSYNC:
		if (mtk_xc_unsubscribe_event(sub->type, &pq_dev->dev_indx)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to unsubscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_MTK_PQ_INPUT_DONE:
	case V4L2_EVENT_MTK_PQ_OUTPUT_DONE:
	case V4L2_EVENT_MTK_PQ_CALLBACK:
	case V4L2_EVENT_MTK_PQ_STREAMOFF:
		if (mtk_display_unsubscribe_event(
		    sub->type, &pq_dev->dev_indx)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to subscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_HDR_SEAMLESS_MUTE:
	case V4L2_EVENT_HDR_SEAMLESS_UNMUTE:
		if (mtk_pq_enhance_unsubscribe_event(pq_dev, sub->type)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to unsubscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_ALL:
		if (mtk_xc_unsubscribe_event(sub->type, &pq_dev->dev_indx)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to unsubscribe %d event\n", sub->type);
			return -EPERM;
		}
		if (mtk_pq_enhance_unsubscribe_event(pq_dev, sub->type)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to unsubscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_unsubscribe(fh, sub);
}

static int mtk_pq_reqbufs(struct file *file,
	void *fh, struct v4l2_requestbuffers *reqbufs)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", reqbufs->type);

	ret = v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Request buffer fail!Buffer Type = %d\n",
			reqbufs->type);

	return ret;
}

static int mtk_pq_querybuf(struct file *file,
	void *fh, struct v4l2_buffer *buffer)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", buffer->type);

	ret = v4l2_m2m_querybuf(file, ctx->m2m_ctx, buffer);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Query buffer fail!Buffer Type = %d\n",
			buffer->type);

	return ret;
}

static int mtk_pq_qbuf(struct file *file,
	void *fh, struct v4l2_buffer *buffer)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	struct mtk_pq_device *pq_dev = NULL;
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", buffer->type);

	pq_dev = video_get_drvdata(((struct v4l2_fh *)fh)->vdev);

#ifdef CONFIG_OPTEE
	/* config buffer security */
	switch (buffer->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		ret = mtk_pq_cfg_output_buf_sec(pq_dev, buffer);
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		ret = mtk_pq_cfg_capture_buf_sec(pq_dev, buffer);
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Unknown buf type : %u\n", buffer->type);
		return ret;
	}
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "config buf security fail!\n");
		return ret;
	}
#endif

	ret =  v4l2_m2m_qbuf(file, ctx->m2m_ctx, buffer);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Queue buffer fail! Buffer Type = %d, ret = %d\n",
			buffer->type, ret);

	return ret;
}

static int mtk_pq_dqbuf(struct file *file,
	void *fh, struct v4l2_buffer *buffer)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", buffer->type);

	ret = v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buffer);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Dequeue buffer fail! ret = %d, Buffer Type = %d\n",
			ret, buffer->type);

	return ret;
}

static int mtk_pq_s_crop(struct file *file, void *fh, const struct v4l2_crop *crop)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	switch (crop->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		ret = mtk_xc_out_s_crop(ctx->pq_dev, crop);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_xc_out_qbuf fail!\n");
			return ret;
		}
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid buffer type!\n");
		return -EINVAL;
	}

	return ret;
}

static int mtk_pq_g_crop(struct file *file, void *fh, struct v4l2_crop *crop)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	switch (crop->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		ret = mtk_xc_out_g_crop(ctx->pq_dev, crop);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_xc_out_qbuf fail!\n");
			return ret;
		}
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid buffer type!\n");
		return -EINVAL;
	}

	return ret;
}

/* v4l2_ioctl_ops */
static const struct v4l2_ioctl_ops mtk_pq_dec_ioctl_ops = {
	.vidioc_querycap		= mtk_pq_querycap,
	.vidioc_s_input			= mtk_pq_s_input,
	.vidioc_g_input			= mtk_pq_g_input,
	.vidioc_s_fmt_vid_cap_mplane	= mtk_pq_s_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_out_mplane	= mtk_pq_s_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap_mplane	= mtk_pq_g_fmt_vid_cap_mplane,
	.vidioc_g_fmt_vid_out_mplane	= mtk_pq_g_fmt_vid_out_mplane,
	.vidioc_streamon		= mtk_pq_streamon,
	.vidioc_streamoff		= mtk_pq_streamoff,
	.vidioc_subscribe_event		= mtk_pq_subscribe_event,
	.vidioc_unsubscribe_event	= mtk_pq_unsubscribe_event,
	.vidioc_reqbufs			= mtk_pq_reqbufs,
	.vidioc_querybuf		= mtk_pq_querybuf,
	.vidioc_qbuf			= mtk_pq_qbuf,
	.vidioc_dqbuf			= mtk_pq_dqbuf,
	//.vidioc_s_crop		= mtk_pq_s_crop,		//not support in 5.4
	//.vidioc_g_crop		= mtk_pq_g_crop,		//not support in 5.4
};

static int mtk_pq_queue_setup(struct vb2_queue *vq,
	unsigned int *num_buffers, unsigned int *num_planes,
	unsigned int sizes[], struct device *alloc_devs[])
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", vq->type);

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		*num_planes = 2;
		sizes[0] = 4;
		sizes[1] = 4;
		*num_buffers = 16;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "reset buffer count!\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		*num_planes = 2;
		sizes[0] = 4;
		sizes[1] = 4;
		*num_buffers = 16;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "reset buffer count!\n");
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Buffer type (%d) not support!\n", vq->type);
		return -EINVAL;
	}

	return 0;
}

static void mtk_pq_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct mtk_pq_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_pq_device *pq_dev = ctx->pq_dev;
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", vb->type);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, vbuf);

	mtk_display_qbuf(pq_dev, vb);
}

static void mtk_pq_buf_cleanup(struct vb2_buffer *vb)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", vb->type);
}
static int mtk_pq_start_streaming(struct vb2_queue *q,
	unsigned int count)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"Buffer Type = %d!\n", q->type);
	return 0;
}
static const struct vb2_ops mtk_pq_vb2_qops = {
	.queue_setup = mtk_pq_queue_setup,
	.buf_queue = mtk_pq_buf_queue,
	.buf_cleanup = mtk_pq_buf_cleanup,
	//.buf_prepare     = gsc_m2m_buf_prepare,
	.wait_prepare    = vb2_ops_wait_prepare,
	.wait_finish     = vb2_ops_wait_finish,
	//.stop_streaming  = gsc_m2m_stop_streaming,
	.start_streaming = mtk_pq_start_streaming,
};
static void *mtk_pq_vb2_get_userptr(struct device *dev,
	unsigned long vaddr, unsigned long size,
	enum dma_data_direction dma_dir)
{
	return NULL;
}
static void mtk_pq_vb2_put_userptr(void *buf_priv)
{

}
const struct vb2_mem_ops mtk_pq_vb2_mem_ops = {
	.get_userptr = mtk_pq_vb2_get_userptr,
	.put_userptr = mtk_pq_vb2_put_userptr,
};
static int queue_init(void *priv, struct vb2_queue *src_vq,
	struct vb2_queue *dst_vq)
{
	struct mtk_pq_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &mtk_pq_vb2_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->pq_dev->mutex;
	src_vq->dev = ctx->pq_dev->dev;
	src_vq->allow_zero_bytesused = 1;
	src_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(src_vq);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "vb2_queue_init fail!\n");
		return ret;
	}

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &mtk_pq_vb2_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->pq_dev->mutex;
	dst_vq->dev = ctx->pq_dev->dev;
	dst_vq->allow_zero_bytesused = 1;
	dst_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(dst_vq);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "vb2_queue_init fail!\n");
		return ret;
	}

	return 0;
}

static int mtk_pq_open(struct file *file)
{
	if (file == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}

	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct mtk_pq_ctx *pq_ctx = NULL;
	int ret = 0;
	int b2r_ret = -1, buffer_ret = -1, svp_ret = -1, enhance_ret = -1;
	struct pq_buffer pq_buf;

	mutex_lock(&pq_dev->mutex);

	if (pq_dev->bopened) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "device has been already opened!");
		ret = -EBUSY;
		goto ERR;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"%s!\n", __func__);

	pq_ctx = kzalloc(sizeof(struct mtk_pq_ctx), GFP_KERNEL);
	if (!pq_ctx) {
		ret = -ENOMEM;
		goto ERR;
	}

	pq_ctx->pq_dev = pq_dev;
	pq_ctx->m2m_ctx = v4l2_m2m_ctx_init(pq_dev->m2m.m2m_dev,
				pq_ctx, queue_init);

	v4l2_fh_init(&pq_ctx->fh, video_devdata(file));

	file->private_data = &pq_ctx->fh;
	v4l2_fh_add(&pq_ctx->fh);

	b2r_ret = mtk_pq_b2r_init(pq_dev);
	if (b2r_ret < 0) {
		ret = -1;
		pr_err("MTK PQ : b2r init failed\n");
		goto ERR;
	}

	buffer_ret = mtk_pq_buffer_buf_init(pq_dev);
	if (buffer_ret < 0) {
		ret = -1;
		pr_err("MTK PQ : pq buffer init failed\n");
		goto ERR;
	}

#ifdef CONFIG_OPTEE
	svp_ret = mtk_pq_svp_init(pq_dev);
	if (svp_ret < 0) {
		ret = -1;
		pr_err("MTK PQ : pq svp init failed\n");
		goto ERR;
	}
#endif

	enhance_ret = mtk_pq_enhance_open(pq_dev);
	if (enhance_ret) {
		ret = -1;
		v4l2_err(&pq_dev->v4l2_dev,
			"failed to open enhance subdev!\n");
		goto ERR;
	}

	pq_dev->bopened = TRUE;
	mutex_unlock(&pq_dev->mutex);
	return 0;

ERR:
	mutex_unlock(&pq_dev->mutex);
	return ret;
}

static int mtk_pq_release(struct file *file)
{
	if (file == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}

	int ret = 0;
	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct v4l2_fh *fh = (struct v4l2_fh *)file->private_data;
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int b2r_ret = -1;

	mutex_lock(&pq_dev->mutex);

	v4l2_m2m_ctx_release(ctx->m2m_ctx);

	b2r_ret = mtk_pq_b2r_exit();
	if (b2r_ret < 0) {
		ret = -1;
		pr_err("MTK PQ : b2r exit failed\n");
	}

#ifdef CONFIG_OPTEE
		mtk_pq_svp_exit(pq_dev);
#endif
	mtk_pq_buffer_buf_exit(pq_dev);
	mtk_pq_enhance_close(pq_dev);

	v4l2_info(&pq_dev->v4l2_dev, "MTK PQ : release() flag(%d)\n",
		file->f_flags);

	v4l2_fh_del(fh);
	v4l2_fh_exit(fh);
	kfree(fh);

	pq_dev->bopened = FALSE;
	mutex_unlock(&pq_dev->mutex);

	return ret;
}

/* v4l2 ops */
static const struct v4l2_file_operations mtk_pq_fops = {
	.owner              = THIS_MODULE,
	.open               = mtk_pq_open,
	.release            = mtk_pq_release,
	.poll               = v4l2_ctrl_poll,
	.unlocked_ioctl     = video_ioctl2,
};
static void mtk_m2m_device_run(void *priv)
{

}

static void mtk_m2m_job_abort(void *priv)
{
	struct mtk_pq_ctx *ctx = priv;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");
	v4l2_m2m_job_finish(ctx->pq_dev->m2m.m2m_dev, ctx->m2m_ctx);
}


// pq_init_subdevices - Initialize subdev structures and resources
// @pq_dev: pq device
// Return 0 on success or a negative error code on failure

static int pq_init_subdevices(struct mtk_pq_platform_device *pqdev)
{
	int ret = 0;
	int i = 0;

	ret = mtk_pq_b2r_subdev_init(pqdev->dev, &pqdev->b2r_dev);
	if (ret < 0) {
		PQ_MSG_ERROR("[B2R] get resource failed\n");
		ret = -1;
	}

	return ret;
}

static struct v4l2_m2m_ops mtk_pq_m2m_ops = {
	.device_run = mtk_m2m_device_run,
	.job_abort = mtk_m2m_job_abort,
};

static void _mtk_pq_unregister_v4l2dev(struct mtk_pq_device *pq_dev)
{
	mtk_pq_unregister_enhance_subdev(&pq_dev->subdev_enhance);
	mtk_pq_unregister_xc_subdev(&pq_dev->subdev_xc);
	mtk_pq_unregister_display_subdev(&pq_dev->subdev_display);
	mtk_pq_unregister_b2r_subdev(&pq_dev->subdev_b2r);
	mtk_pq_unregister_common_subdev(&pq_dev->subdev_common);
	v4l2_m2m_release(pq_dev->m2m.m2m_dev);
	video_unregister_device(&pq_dev->video_dev);
	v4l2_device_unregister(&pq_dev->v4l2_dev);
}

static int _mtk_pq_register_v4l2dev(struct mtk_pq_device *pq_dev,
	__u8 dev_id, struct mtk_pq_platform_device *pqdev)
{
	struct video_device *vdev;
	struct v4l2_device *v4l2_dev;
	unsigned int len;
	int ret;

	if ((!pq_dev) || (!pqdev))
		return -ENOMEM;

	spin_lock_init(&pq_dev->slock);
	mutex_init(&pq_dev->mutex);

	vdev = &pq_dev->video_dev;
	v4l2_dev = &pq_dev->v4l2_dev;

	pq_dev->dev = pqdev->dev;
	pq_dev->dev_indx = dev_id;

	//patch here
	pq_dev->b2r_dev.id = pqdev->b2r_dev.id;
	pq_dev->b2r_dev.irq = pqdev->b2r_dev.irq;

	snprintf(v4l2_dev->name, sizeof(v4l2_dev->name),
		"%s", PQ_V4L2_DEVICE_NAME);
	ret = v4l2_device_register(NULL, v4l2_dev);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register v4l2 device\n");
		goto exit;
	}

	pq_dev->m2m.m2m_dev = v4l2_m2m_init(&mtk_pq_m2m_ops);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to init m2m device\n");
		goto exit;
	}

	v4l2_ctrl_handler_init(&pq_dev->ctrl_handler, 0);
	v4l2_dev->ctrl_handler = &pq_dev->ctrl_handler;

	ret = mtk_pq_register_common_subdev(v4l2_dev, &pq_dev->subdev_common,
		&pq_dev->common_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register common sub dev\n");
		goto exit;
	}
	ret = mtk_pq_register_xc_subdev(v4l2_dev, &pq_dev->subdev_xc,
		&pq_dev->xc_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register xc sub dev\n");
		goto exit;
	}
	ret = mtk_pq_register_enhance_subdev(v4l2_dev, &pq_dev->subdev_enhance,
		&pq_dev->enhance_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register drv pq sub dev\n");
		goto exit;
	}
	ret = mtk_pq_register_display_subdev(v4l2_dev, &pq_dev->subdev_display,
		&pq_dev->display_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register display sub dev\n");
		goto exit;
	}
	ret = mtk_pq_register_b2r_subdev(v4l2_dev, &pq_dev->subdev_b2r,
		&pq_dev->b2r_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register b2r sub dev\n");
		goto exit;
	}

	vdev->fops = &mtk_pq_fops;
	vdev->ioctl_ops = &mtk_pq_dec_ioctl_ops;
	vdev->release = video_device_release;
	vdev->v4l2_dev = v4l2_dev;
	vdev->vfl_dir = VFL_DIR_M2M;
	//device name is "mtk-pq-dev" + "0"/"1"/...
	len = snprintf(vdev->name, sizeof(vdev->name), "%s",
		PQ_VIDEO_DEVICE_NAME);
	snprintf(vdev->name + len, sizeof(vdev->name) - len, "%s",
		devID[dev_id]);

	ret = video_register_device(vdev, VFL_TYPE_GRABBER,
		PQ_VIDEO_DEVICE_NUM_BASE + dev_id);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register video device!\n");
		goto exit;
	}

	video_set_drvdata(vdev, pq_dev);
	v4l2_info(v4l2_dev, "mtk-pq registered as /dev/video%d\n", vdev->num);

	return 0;

exit:
	_mtk_pq_unregister_v4l2dev(pq_dev);

	return -EPERM;
}

static int _mtk_pq_probe(struct platform_device *pdev)
{
	struct mtk_pq_device *pq_dev;
	struct mtk_pq_platform_device *pqdev;
	int ret;
	__u8 win_num;
	__u8 cnt;

	pqdev = devm_kzalloc(&pdev->dev,
		sizeof(struct mtk_pq_platform_device), GFP_KERNEL);
	if (!pqdev)
		return -ENOMEM;

	pqdev->dev = &pdev->dev;

	ret = _mtk_pq_parse_dts(&pqdev->pqcaps, pdev);
	if (ret != 0) {
		PQ_MSG_ERROR("Failed to parse pq cap dts\r\n");
		return ret;
	}

	ret = mtk_pq_enhance_parse_dts(&pqdev->pq_enhance, pdev);
	if (ret != 0) {
		PQ_MSG_ERROR("Failed to parse pq enhance dts\r\n");
		return ret;
	}

	ret = pq_init_subdevices(pqdev);

	win_num = pqdev->pqcaps.u32Window_Num;
	if (win_num > PQ_WIN_MAX_NUM) {
		PQ_MSG_ERROR("DTS error, can not create too many devices\r\n");
		win_num = PQ_WIN_MAX_NUM;
	}

	// 2 is patch here, should be win_num
	for (cnt = 0; cnt < 2; cnt++) {
		pq_dev = devm_kzalloc(&pdev->dev,
			sizeof(struct mtk_pq_device), GFP_KERNEL);
		pqdev->pq_devs[cnt] = pq_dev;
		if (!pq_dev)
			goto exit;

		/* for kernel 5.4.1 */
		pqdev->pq_devs[cnt]->video_dev.device_caps = V4L2_CAP_VIDEO_M2M_MPLANE |
			V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_CAPTURE;

		if (_mtk_pq_register_v4l2dev(pq_dev, cnt, pqdev)) {
			PQ_MSG_ERROR("Failed to register v4l2 dev\r\n");
			goto exit;
		}
	}

	// equal to dev_set_drvdata(&pdev->dev, pqdev);
	// you can get "pqdev" by platform_get_drvdata(pdev)
	// or dev_get_drvdata(&pdev->dev)
	platform_set_drvdata(pdev, pqdev);

	mtk_display_init();

	mtk_pq_CreateSysFS(&pdev->dev);

	return 0;

exit:
	// 2 is patch here, should be win_num
	for (cnt = 0; cnt < 2; cnt++) {
		pq_dev = pqdev->pq_devs[cnt];
		if (pq_dev)
			_mtk_pq_unregister_v4l2dev(pq_dev);
	}

	return -EPERM;
}

static int _mtk_pq_remove(struct platform_device *pdev)
{
	struct mtk_pq_platform_device *pqdev = platform_get_drvdata(pdev);
	struct mtk_pq_device *pq_dev;
	__u8 cnt;

	// 2 is patch here, should be win_num
	for (cnt = 0; cnt < 2; cnt++) {
		pq_dev = pqdev->pq_devs[cnt];
		if (pq_dev)
			_mtk_pq_unregister_v4l2dev(pq_dev);
	}

	return 0;
}

static int _mtk_pq_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	return 0;
}

static int _mtk_pq_resume(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mtk_pq_match[] = {
	{.compatible = "mediatek,pq",},
	{},
};

static struct platform_driver pq_pdrv = {
	.probe      = _mtk_pq_probe,
	.remove     = _mtk_pq_remove,
	.suspend    = _mtk_pq_suspend,
	.resume     = _mtk_pq_resume,
	.driver     = {
		.name = "mtk-pq",
		.of_match_table = mtk_pq_match,
		.owner = THIS_MODULE,
	},
};

static int __init _mtk_pq_init(void)
{
	platform_driver_register(&pq_pdrv);

	return 0;
}

static void __exit _mtk_pq_exit(void)
{
	platform_driver_unregister(&pq_pdrv);
}

module_init(_mtk_pq_init);
module_exit(_mtk_pq_exit);

MODULE_AUTHOR("Kevin Ren <kevin.ren@mediatek.com>");
MODULE_DESCRIPTION("mtk pq device driver");
MODULE_LICENSE("GPL v2");

