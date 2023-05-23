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
#include "mtk_pq_display_manager.h"

#include "drvPQ.h"
#include "apiVDEC_EX.h"
#include "drvMVOP.h"



#define SYS_TIME_MAX					(0xffffffff)
#define MTK_PQ_FHD_MAX_H_SIZE			(1920)
#define MTK_PQ_FHD_MAX_V_SIZE			(1088)
#define MTK_PQ_VSD_100_PERCENT			(100)
#define MTK_PQ_VSD_90_PERCENT			(90)
#define MTK_PQ_VSD_50_PERCENT			(50)
#define MTK_PQ_VSD_25_PERCENT			(25)
#define MTK_PQ_VSD_125_PERCENT			(125)
#define MTK_PQ_VSD_XC_SCALING_DOWN		(0x2)
#define MTK_PQ_VSD_XC_FBL				(0x4)
#define MTK_PQ_VSD_2ND_BUFFER			(0x1)
#define MTK_PQ_DISPLAYCNT				(200)
#define MTK_PQ_VIDEODELAY_TIMEOUT		(200)
#define MTK_PQ_TIME_SEC2MSEC(x)			(x * 1000)
#define MTK_PQ_COMMON_SEMAPHORE			(V4L2_MTK_MAX_WINDOW)

static struct mtk_pq_window user_capt_win[V4L2_MTK_MAX_WINDOW];
static struct mtk_pq_window user_disp_win[V4L2_MTK_MAX_WINDOW];
static struct mtk_pq_window user_crop_win[V4L2_MTK_MAX_WINDOW];
static enum mtk_pq_aspect_ratio user_arc[V4L2_MTK_MAX_WINDOW];
static struct mtk_pq_frame_release_info
	release_info[V4L2_MTK_MAX_WINDOW];
static bool is_digital_cleared = TRUE;
static bool is_fired = FALSE;
static __u8 swds_index;
static bool is_even_field = FALSE;

static enum mtk_pq_field_change field_change_cnt =
	MTK_PQ_FIELD_CHANGE_DISABLE;

static bool ignore_cur_field = FALSE;
static enum mtk_pq_field_type pre_field_type = MTK_PQ_FIELD_TYPE_MAX;
static __u32 cur_histogram[V4L2_MTK_HISTOGRAM_INDEX * 2];
static __u32 pre_histogram[V4L2_MTK_HISTOGRAM_INDEX * 2];
static bool report[V4L2_MTK_MAX_WINDOW] = { FALSE };
static __u32 delay_time[V4L2_MTK_MAX_WINDOW];
static __s64 system_time[V4L2_MTK_MAX_WINDOW];
static __u64 pts_ms[V4L2_MTK_MAX_WINDOW];
static __u64 pts_us[V4L2_MTK_MAX_WINDOW];
static bool check_frame_disp[V4L2_MTK_MAX_WINDOW] = { FALSE };
static __u32 display_count[V4L2_MTK_MAX_WINDOW];

static __u64 task_cnt;
static __u64 total_time;
static enum mtk_pq_scan_type pre_scan_type = MTK_PQ_VIDEO_SCAN_TYPE_MAX;
static bool support_SPFLB = FALSE;
static EN_XC_HDR_TYPE pre_HDR_type = E_XC_HDR_TYPE_MAX;
static __u32 video_delay_time[MTK_PQ_QUEUE_DEPTH] = { 0 };

static struct mtk_pq_measure_time drv_delay_time[MEASURE_TIME_MAX_NUM];
static struct mtk_pq_measure_time hal_delay_time[MEASURE_TIME_MAX_NUM];
static enum mtk_pq_display_ctrl_type display_get_ctrl_type =
	MTK_PQ_DISPLAY_CTRL_TYPE_MAX;
static struct mtk_pq_window_info pre_get_win_info = { 0 };
static struct v4l2_rect pre_get_id_info = { 0 };
static struct mtk_pq_delay_info pre_get_delay_info = { 0 };

static wait_queue_head_t mvop_task;
static bool MVOP_TASK_WAKE_UP_FLAG = FALSE;

static struct mtk_pq_resource *display_resource[V4L2_MTK_MAX_WINDOW];

static int _display_get_resource(void *pResource, void **pPrivate);

#define DISPLAY_GET_RES_PRI struct mtk_pq_resource *resource = NULL;\
	_display_get_resource(display_resource[u32WindowID],\
		(void **)&resource)

static struct mtk_pq_common_resource *common_resource;
#define RES_DISPLAY resource->display_resource
#define RES_COMMON common_resource
static bool display_inited = FALSE;

static struct semaphore display_semaphore[V4L2_MTK_MAX_WINDOW + 1];

static void _display_obtain_semaphore(__u32 u32WindowID)
{
	down_interruptible(&display_semaphore[u32WindowID]);
}

static void _display_release_semaphore(__u32 u32WindowID)
{
	up(&display_semaphore[u32WindowID]);
}

static int _display_get_resource(void *pResource, void **pPrivate)
{
	if ((pResource == NULL) || (pPrivate == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	*pPrivate = pResource;

	return 0;
}

static __u32 _display_get_time(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static __u16 _display_get_pre_id(__u16 u16CurID)
{
	return ((u16CurID + MTK_PQ_QUEUE_DEPTH - 1) % MTK_PQ_QUEUE_DEPTH);
}

static __u16 _display_get_next_id(__u16 u16CurID)
{
	return ((u16CurID + 1) % MTK_PQ_QUEUE_DEPTH);
}

static __u16 _display_get_pre_w_ptr(__u32 u32WindowID)
{
	DISPLAY_GET_RES_PRI;
	return _display_get_pre_id(
		RES_DISPLAY.buffer_ptr.write_ptr);
}

static __u16 _display_get_next_w_ptr(__u32 u32WindowID)
{
	DISPLAY_GET_RES_PRI;
	return _display_get_next_id(
	    RES_DISPLAY.buffer_ptr.write_ptr);
}

static bool _display_is_ds_enable(void)
{
	XC_GET_DS_INFO stGetDSInfo;

	memset(&stGetDSInfo, 0, sizeof(XC_GET_DS_INFO));

	stGetDSInfo.u32ApiGetDSInfo_Version = API_GET_XCDS_INFO_VERSION;
	stGetDSInfo.u16ApiGetDSInfo_Length = sizeof(XC_GET_DS_INFO);
	MApi_XC_GetDSInfo(&stGetDSInfo, sizeof(XC_GET_DS_INFO), MVOP_WINDOW);

	return stGetDSInfo.bDynamicScalingEnable;
}

static bool _display_is_window_used(__u32 u32WindowID)
{
	DISPLAY_GET_RES_PRI;

	return RES_DISPLAY.stWindowInfo.u32WinIsused;
}

static bool _display_is_frame_valid(
		struct mtk_pq_frame_info *pstFrameFormat)
{
	if (pstFrameFormat == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	if ((pstFrameFormat->stFrames[0].u32Width == 0)
	    && (pstFrameFormat->stFrames[0].u32Height == 0)
	    && (pstFrameFormat->u32FrameRate == 0)) {
		return FALSE;
	} else {
		return TRUE;
	}
}

static __u32 _display_get_delay_time(__u32 u32StartTime)
{
	__u32 u32DelayTime = 0;
	__u32 u32CurrentTime = _display_get_time();

	if (u32CurrentTime >= u32StartTime)
		u32DelayTime = u32CurrentTime - u32StartTime;
	else
		u32DelayTime = SYS_TIME_MAX - u32StartTime + u32CurrentTime;

	return u32DelayTime;
}

static void _display_store_flip_time(__u16 u16WritePtr,
		__u32 u32WindowID, bool bSaveFlag)
{
	if (u32WindowID != 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"WindowID not support yet!\n");
		return;
	}

	DISPLAY_GET_RES_PRI;

	if (bSaveFlag) {
		RES_DISPLAY.u32FlipTime[u16WritePtr] = _display_get_time();
	} else {
		RES_DISPLAY.au32VideoFlipTime[u16WritePtr] =
			_display_get_time();
		RES_DISPLAY.abVideoFlip[u16WritePtr] = TRUE;
	}
}


static enum mtk_pq_scan_type _display_get_scan_type(__u8 u8ScanType)
{
	enum mtk_pq_scan_type eScanType = MTK_PQ_VIDEO_SCAN_TYPE_MAX;

	switch (u8ScanType) {
	case 0:
		eScanType = MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE;
		break;

	case 1:

	case 2:
		eScanType = MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME;
		break;

	case 3:
		eScanType = MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD;
		break;

	default:
		eScanType = MTK_PQ_VIDEO_SCAN_TYPE_MAX;
		break;
	}

	return eScanType;
}

static enum mtk_pq_video_data_format _display_get_format(
		enum mtk_pq_color_format eFormat)
{
	enum mtk_pq_video_data_format enVideoColorFormat =
			MTK_PQ_VIDEO_DATA_FMT_YUV422;

	switch (eFormat) {
	case MTK_PQ_COLOR_FORMAT_HW_HVD:

	case MTK_PQ_COLOR_FORMAT_HW_MVD:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_YUV420;
		break;

	case MTK_PQ_COLOR_FORMAT_SW_RGB565:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_RGB565;
		break;

	case MTK_PQ_COLOR_FORMAT_SW_ARGB8888:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_ARGB8888;
		break;

	case MTK_PQ_COLOR_FORMAT_YUYV:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_YUV422;
		break;

	case MTK_PQ_COLOR_FORMAT_10BIT_TILE:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_YUV420_H265_10BITS;
		break;

	case MTK_PQ_COLOR_FORMAT_SW_YUV420_SEMIPLANAR:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_YUV420_SEMI_PLANER;
		break;

	case MTK_PQ_COLOR_FORMAT_HW_EVD:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_YUV420_H265;
		break;

	case MTK_PQ_COLOR_FORMAT_HW_32x32:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_YUV420;
		break;

	default:
		enVideoColorFormat = MTK_PQ_VIDEO_DATA_FMT_MAX;
		break;
	}

	return enVideoColorFormat;
}

static __u64 _display_get_phy_bit_len_base(__u8 u8BitLenmiu,
			__u64 phyMFCBITLEN)
{
	__u64 phyBitlenBase = 0;

	if (u8BitLenmiu == 0) {
		phyBitlenBase = phyMFCBITLEN;
	} else if (u8BitLenmiu == 1) {
		//phyBitlenBase = phyMFCBITLEN | (__u64) HAL_MIU1_BASE;
		phyBitlenBase = (__u64) HAL_MIU1_BASE;
	} else {
		//phyBitlenBase = phyMFCBITLEN | (__u64) HAL_MIU2_BASE;
		phyBitlenBase = (__u64) HAL_MIU2_BASE;
	}

	return phyBitlenBase;
}

static bool _display_is_source_interlace(enum mtk_pq_scan_type eScanType)
{
	if ((eScanType == MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME)
	    || (eScanType == MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static bool _display_set_histogram(
			struct mtk_pq_frame_info *pstFrameFormat,
			__u8 u8WritePointer, bool b2ndField, __u32 u32WindowID)
{
	if (pstFrameFormat == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	bool bRet = FALSE;
	struct mtk_pq_hw_format *format0 =
		&(pstFrameFormat->stFrames[0].stHWFormat);
	struct mtk_pq_hw_format *format1 =
		&(pstFrameFormat->stFrames[1].stHWFormat);

	DISPLAY_GET_RES_PRI;

	if (format0->u8V7DataValid & 0x20) {
		if ((RES_DISPLAY.frame_info[u8WritePointer].enScanType ==
		     MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD) && b2ndField) {
			__u16 ct = 0;

			for (ct = 0; ct < V4L2_MTK_HISTOGRAM_INDEX; ct++) {
				RES_DISPLAY.u32Histogram[u8WritePointer][ct] =
				    format1->u16Histogram[ct];
			}
		} else {
			__u16 ct = 0;

			for (ct = 0; ct < V4L2_MTK_HISTOGRAM_INDEX; ct++) {
				RES_DISPLAY.u32Histogram[u8WritePointer][ct] =
				    format0->u16Histogram[ct];
			}
		}

		RES_DISPLAY.bHistogramValid = TRUE;
		bRet = TRUE;
	} else {
		static bool bLogFlag = FALSE;

		RES_DISPLAY.bHistogramValid = FALSE;

		if (!bLogFlag)
			bLogFlag = TRUE;

		bRet = FALSE;
	}

	return bRet;
}


static bool _display_is_keepbuffer_max(__u16 u16ReadPointer,
	__u16 u16WritePointer, enum mtk_pq_scan_type enScanType)
{
	__u16 u16MaxKeepBuffer = MTK_PQ_MAX_KEEP_BUFFER;

	if (u16ReadPointer > u16WritePointer)
		u16WritePointer += MTK_PQ_QUEUE_DEPTH;

	if (enScanType != MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE)
		u16MaxKeepBuffer *= 2;

	if ((u16WritePointer - u16ReadPointer) >= u16MaxKeepBuffer)
		return TRUE;

	return FALSE;
}

static bool _display_store_user_setwin_info(
		struct mtk_pq_vdec_frame_info *VDECFrame,
		struct mtk_pq_frame_info *pstFrameFormat,
					    __u32 u32WindowID)
{
	if ((pstFrameFormat == NULL) && (VDECFrame == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	__u16 u16WritePointer = RES_DISPLAY.buffer_ptr.write_ptr;
	struct mtk_pq_vdec_frame_info *pstFrmInfo =
		&(RES_DISPLAY.frame_info[u16WritePointer]);

	memcpy(&pstFrmInfo->stUserXCCaptWin,
	       &user_capt_win[u32WindowID], sizeof(struct mtk_pq_window));
	memcpy(&pstFrmInfo->stUserXCDispWin,
	       &user_disp_win[u32WindowID], sizeof(struct mtk_pq_window));
	memcpy(&pstFrmInfo->stUserXCCropWin,
	       &user_crop_win[u32WindowID], sizeof(struct mtk_pq_window));

	pstFrmInfo->enUserArc = user_arc[u32WindowID];

	if (!_display_is_source_interlace(VDECFrame->enScanType)
	    || (_display_is_source_interlace(VDECFrame->enScanType)
		&& (!VDECFrame->bIs2ndField))) {
		if (RES_DISPLAY.bUpdateSetWin
			&& (!RES_DISPLAY.bSetWinImmediately)) {
			RES_DISPLAY.bUpdateSetWin = FALSE;
			pstFrmInfo->bUserSetwinFlag = TRUE;
		}
	}

	return TRUE;
}

static bool _display_is_src_interlace(enum mtk_pq_scan_type eScanType)
{
	if (eScanType == MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD)
		return TRUE;
	else
		return FALSE;
}


static bool _display_vdec_frame_add_ref(struct mtk_pq_device *pq_dev,
		__u32 u32WindowID, __u16 u16BufID)
{
	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	if (RES_DISPLAY.frame_info[u16BufID].u32FrameIndex ==
			MTK_PQ_INVALID_FRAME_ID)
		return TRUE;

	VDEC_StreamId VdecStreamId;
	VDEC_EX_Result eResult;
	VDEC_EX_DispFrame VdecDispFrm;

	VdecStreamId.u32Version =
		RES_DISPLAY.frame_info[u16BufID].u32VDECStreamVersion;
	VdecStreamId.u32Id =
		RES_DISPLAY.frame_info[u16BufID].u32VDECStreamID;
	VdecDispFrm.u32Idx =
		RES_DISPLAY.frame_info[u16BufID].u32FrameIndex;
	VdecDispFrm.u32PriData =
		RES_DISPLAY.frame_info[u16BufID].u32PriData;

	eResult = MApi_VDEC_EX_DisplayFrame((VDEC_StreamId *) &VdecStreamId,
		&VdecDispFrm);

	if (_display_is_src_interlace
			(RES_DISPLAY.frame_info[u16BufID].enScanType)) {
		VDEC_EX_Result eResult;

		VdecStreamId.u32Version =
			RES_DISPLAY.frame_info[u16BufID].u32VDECStreamVersion;
		VdecStreamId.u32Id =
			RES_DISPLAY.frame_info[u16BufID].u32VDECStreamID;
		VdecDispFrm.u32Idx =
			RES_DISPLAY.frame_info[u16BufID].u32FrameIndex_2nd;
		VdecDispFrm.u32PriData =
			RES_DISPLAY.frame_info[u16BufID].u32PriData_2nd;

		eResult =
			MApi_VDEC_EX_DisplayFrame(
				(VDEC_StreamId *) &VdecStreamId,
				&VdecDispFrm);
	}

	RES_DISPLAY.u8LocalFrameRefCount[u16BufID]++;

	return TRUE;
}

static bool _display_video_flip_drop(struct mtk_pq_device *pq_dev,
				struct vb2_buffer *vb, __u32 u32WindowID,
				struct mtk_pq_frame_info *pstFrameFormat,
				struct mtk_pq_vdec_frame_info *stFrameInfo)
{
	if ((pq_dev == NULL) || (vb == NULL) ||
		(pstFrameFormat == NULL) || (stFrameInfo == NULL) ||
		is_digital_cleared) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

	if (pq_dev->display_info.bReleaseTaskCreated) {
		release_info[u32WindowID].u32VdecStreamId =
		    pstFrameFormat->u32VdecStreamId;
		release_info[u32WindowID].u32VdecStreamVersion =
		    pstFrameFormat->u32VdecStreamVersion;
		release_info[u32WindowID].u32WindowID = u32WindowID;
		release_info[u32WindowID].au32Idx[0] =
		    pstFrameFormat->stFrames[0].u32Idx;
		release_info[u32WindowID].au32Idx[1] =
		    pstFrameFormat->stFrames[1].u32Idx;
		release_info[u32WindowID].au32PriData[0] =
		    pstFrameFormat->stFrames[0].u32PriData;
		release_info[u32WindowID].au32PriData[1] =
		    pstFrameFormat->stFrames[1].u32PriData;
		release_info[u32WindowID].u64Pts =
		    pstFrameFormat->u64Pts;
		release_info[u32WindowID].u64PtsUs =
		    pstFrameFormat->u64PtsUs;
		release_info[u32WindowID].u32FrameNum =
		    pstFrameFormat->u32FrameNum;
		release_info[u32WindowID].u8ApplicationType =
		    pstFrameFormat->u8ApplicationType;

		struct v4l2_event ev;

		memset(&ev, 0, sizeof(struct v4l2_event));
		ev.type = V4L2_EVENT_MTK_PQ_INPUT_DONE;
		ev.u.data[0] = vb->index;
		v4l2_event_queue(&pq_dev->video_dev, &ev);
	}

	return TRUE;
}

static bool _display_store_vdec_info_to_queue(struct mtk_pq_device *pq_dev,
		struct vb2_buffer *vb, struct mtk_pq_vdec_frame_info *VDECFrame,
		struct mtk_pq_frame_info *pstFrameFormat,
		__u32 u32WindowID)
{
	if ((pq_dev == NULL) || (vb == NULL) ||
		(pstFrameFormat == NULL) || (VDECFrame == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	__u32 u32Window = VDECFrame->u32Window;
	__u16 u16PreWritePointer = _display_get_pre_w_ptr(u32Window);
	__u16 u16ReadPointer = RES_DISPLAY.buffer_ptr.read_ptr;
	__u16 u16WritePointer = RES_DISPLAY.buffer_ptr.write_ptr;
	__u16 u16TimeCnt = 0;

	struct mtk_pq_vdec_frame_info *pstFrmInfo =
		&(RES_DISPLAY.frame_info[u16WritePointer]);

	struct mtk_pq_vdec_frame_info *pstPreFrmInfo =
		&(RES_DISPLAY.frame_info[u16PreWritePointer]);

	if ((_display_is_keepbuffer_max
	     (u16ReadPointer, u16WritePointer, VDECFrame->enScanType))
	    && (VDECFrame->u8ContinuousTime == 1)) {
		_display_video_flip_drop(pq_dev, vb, u32WindowID,
			pstFrameFormat, VDECFrame);
		return FALSE;
	}

	if (pstFrmInfo->u32FrameIndex != MTK_PQ_INVALID_FRAME_ID) {
		while (pstFrmInfo->bValid) {
			_display_release_semaphore(u32WindowID);
			msleep_interruptible(1);
			_display_obtain_semaphore(u32WindowID);
			if (!RES_DISPLAY.bPlayFlag)
				return FALSE;

			u16TimeCnt += 1;

			if (u16TimeCnt > MTK_PQ_FLIP_TIMEOUT_THRESHOLD) {
				_display_video_flip_drop(pq_dev,
					vb, u32WindowID,
					pstFrameFormat, VDECFrame);
				return FALSE;
			}
		}
	}

	memcpy(&RES_DISPLAY.frame_info[u16WritePointer], VDECFrame,
	       sizeof(struct mtk_pq_vdec_frame_info));

	_display_store_user_setwin_info(VDECFrame, pstFrameFormat, u32WindowID);

	pstFrmInfo->vb = vb;

	pstFrmInfo->u32FrameIndex =
	    VDECFrame->u32FrameIndex;
	pstFrmInfo->u32VDECStreamID =
	    VDECFrame->u32VDECStreamID;
	pstFrmInfo->u32VDECStreamVersion =
	    VDECFrame->u32VDECStreamVersion;
	pstFrmInfo->u32PriData = VDECFrame->u32PriData;
	pstFrmInfo->u64Pts = VDECFrame->u64Pts;
	pstFrmInfo->u64PtsUs = VDECFrame->u64PtsUs;
	pstFrmInfo->u32FrameNum = VDECFrame->u32FrameNum;
	pstFrmInfo->enScanType = VDECFrame->enScanType;
	pstFrmInfo->bCheckFrameDisp =
	    VDECFrame->bCheckFrameDisp;

	pstFrmInfo->u32FrameIndex_2nd =
	    VDECFrame->u32FrameIndex_2nd;
	pstFrmInfo->u32PriData_2nd =
	    VDECFrame->u32PriData_2nd;
	pstFrmInfo->phySrcLumaAddrI =
	    VDECFrame->phySrcLumaAddrI;
	pstFrmInfo->phySrcChromaAddrI =
	    VDECFrame->phySrcChromaAddrI;

	_display_vdec_frame_add_ref(pq_dev, u32Window, u16WritePointer);

	pstFrmInfo->bValid = TRUE;

	pstFrmInfo->u16SrcWidth = VDECFrame->u16SrcWidth;
	pstFrmInfo->u16SrcHeight =
	    VDECFrame->u16SrcHeight;
	pstFrmInfo->u16CropLeft = VDECFrame->u16CropLeft;
	pstFrmInfo->u16CropRight =
	    VDECFrame->u16CropRight;
	pstFrmInfo->u16CropTop = VDECFrame->u16CropTop;
	pstFrmInfo->u16CropBottom =
	    VDECFrame->u16CropBottom;
	pstFrmInfo->u16SrcPitch = VDECFrame->u16SrcPitch;
	pstFrmInfo->phySrcLumaAddr =
	    VDECFrame->phySrcLumaAddr;
	pstFrmInfo->phySrcChromaAddr =
	    VDECFrame->phySrcChromaAddr;

	pstFrmInfo->u32FrameRate =
	    VDECFrame->u32FrameRate;
	pstFrmInfo->enFmt = VDECFrame->enFmt;
	pstFrmInfo->u32Window = VDECFrame->u32Window;
	pstFrmInfo->enCODEC = VDECFrame->enCODEC;
	pstFrmInfo->u83DMode = VDECFrame->u83DMode;
	pstFrmInfo->enFieldOrderType =
	    VDECFrame->enFieldOrderType;
	pstFrmInfo->enFieldType = VDECFrame->enFieldType;
	pstFrmInfo->enTileMode = VDECFrame->enTileMode;
	pstFrmInfo->u8ApplicationType =
	    VDECFrame->u8ApplicationType;
	pstFrmInfo->enFrameType = VDECFrame->enFrameType;
	pstFrmInfo->u8AspectRate =
	    VDECFrame->u8AspectRate;

	if (_display_is_src_interlace
	    (pstFrmInfo->enScanType)) {
		pstFrmInfo->u32FrameIndex_2nd =
		    VDECFrame->u32FrameIndex_2nd;
		pstFrmInfo->u32PriData_2nd =
		    VDECFrame->u32PriData_2nd;
		pstFrmInfo->phySrcLumaAddrI =
		    VDECFrame->phySrcLumaAddrI;
		pstFrmInfo->phySrcChromaAddrI =
		    VDECFrame->phySrcChromaAddrI;

		if (pstFrmInfo->enFieldOrderType ==
		    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP) {
			if (VDECFrame->bIs2ndField) {
				pstFrmInfo->enFieldType =
				    MTK_PQ_FIELD_TYPE_BOTTOM;
			} else {
				pstFrmInfo->enFieldType =
				    MTK_PQ_FIELD_TYPE_TOP;
			}
		} else {
			if (VDECFrame->bIs2ndField) {
				pstFrmInfo->enFieldType =
				    MTK_PQ_FIELD_TYPE_TOP;
			} else {
				pstFrmInfo->enFieldType =
				    MTK_PQ_FIELD_TYPE_BOTTOM;
			}
		}

		if (VDECFrame->bIs2ndField) {
			_display_set_histogram(pstFrameFormat,
			    u16WritePointer, TRUE, u32WindowID);

			RES_DISPLAY.u32QPMin[u16WritePointer] =
			    pstFrameFormat->stFrames[1].stHWFormat.u32QPMin;
			RES_DISPLAY.u32QPAvg[u16WritePointer] =
			    pstFrameFormat->stFrames[1].stHWFormat.u32QPAvg;
			RES_DISPLAY.u32QPMax[u16WritePointer] =
			    pstFrameFormat->stFrames[1].stHWFormat.u32QPMax;
		} else {
			_display_set_histogram(pstFrameFormat,
			    u16WritePointer, FALSE, u32WindowID);

			RES_DISPLAY.u32QPMin[u16WritePointer] =
			    pstFrameFormat->stFrames[0].stHWFormat.u32QPMin;
			RES_DISPLAY.u32QPAvg[u16WritePointer] =
			    pstFrameFormat->stFrames[0].stHWFormat.u32QPAvg;
			RES_DISPLAY.u32QPMax[u16WritePointer] =
			    pstFrameFormat->stFrames[0].stHWFormat.u32QPMax;
		}
	} else {
		_display_set_histogram(pstFrameFormat, u16WritePointer, FALSE,
				       u32WindowID);

		RES_DISPLAY.u32QPMin[u16WritePointer] =
		    pstFrameFormat->stFrames[0].stHWFormat.u32QPMin;
		RES_DISPLAY.u32QPAvg[u16WritePointer] =
		    pstFrameFormat->stFrames[0].stHWFormat.u32QPAvg;
		RES_DISPLAY.u32QPMax[u16WritePointer] =
		    pstFrameFormat->stFrames[0].stHWFormat.u32QPMax;
	}

	pstFrmInfo->stMFdecInfo.bMFDec_Enable =
	    VDECFrame->stMFdecInfo.bMFDec_Enable;

	if (u32Window == MVOP_WINDOW) {
		pstFrmInfo->stMFdecInfo.u8MFDec_Select = 0;
	} else {
		pstFrmInfo->stMFdecInfo.u8MFDec_Select =
		    VDECFrame->stMFdecInfo.u8MFDec_Select;
	}

	pstFrmInfo->stMFdecInfo.bUncompress_mode =
	    VDECFrame->stMFdecInfo.bUncompress_mode;
	pstFrmInfo->stMFdecInfo.bBypass_codec_mode =
	    VDECFrame->stMFdecInfo.bBypass_codec_mode;
	pstFrmInfo->stMFdecInfo.enMFDecVP9_mode =
	    VDECFrame->stMFdecInfo.enMFDecVP9_mode;
	pstFrmInfo->stMFdecInfo.u16StartX =
	    VDECFrame->stMFdecInfo.u16StartX;
	pstFrmInfo->stMFdecInfo.u16StartY =
	    VDECFrame->stMFdecInfo.u16StartY;
	pstFrmInfo->stMFdecInfo.phyBitlen_Base =
	    VDECFrame->stMFdecInfo.phyBitlen_Base;
	pstFrmInfo->stMFdecInfo.u16Bitlen_Pitch =
	    VDECFrame->stMFdecInfo.u16Bitlen_Pitch;

	pstFrmInfo->b10bitData = VDECFrame->b10bitData;
	pstFrmInfo->u16Src10bitPitch =
	    VDECFrame->u16Src10bitPitch;
	pstFrmInfo->phySrcLumaAddr_2bit =
	    VDECFrame->phySrcLumaAddr_2bit;
	pstFrmInfo->phySrcChromaAddr_2bit =
	    VDECFrame->phySrcChromaAddr_2bit;
	pstFrmInfo->u8LumaBitdepth =
	    VDECFrame->u8LumaBitdepth;

	pstFrmInfo->bIs2ndField = VDECFrame->bIs2ndField;
	pstFrmInfo->u8DiOutputRingBufferID =
	    VDECFrame->u8DiOutputRingBufferID;

	RES_DISPLAY.u32WindowFrameRate = VDECFrame->u32FrameRate / 1000;

	pstFrmInfo->phySrc2ndBufferLumaAddr =
	    VDECFrame->phySrc2ndBufferLumaAddr;
	pstFrmInfo->phySrc2ndBufferChromaAddr =
	    VDECFrame->phySrc2ndBufferChromaAddr;
	pstFrmInfo->u16Src2ndBufferPitch =
	    VDECFrame->u16Src2ndBufferPitch;
	pstFrmInfo->u8Src2ndBufferV7DataValid =
	    VDECFrame->u8Src2ndBufferV7DataValid;
	pstFrmInfo->u16Src2ndBufferWidth =
	    VDECFrame->u16Src2ndBufferWidth;
	pstFrmInfo->u16Src2ndBufferHeight =
	    VDECFrame->u16Src2ndBufferHeight;
	RES_DISPLAY.u32ReleaseState[u16WritePointer] = 0;
	pstFrmInfo->u8FieldCtrl = VDECFrame->u8FieldCtrl;

#if (API_XCDS_INFO_VERSION >= 2)
	pstFrmInfo->stHDRInfo.u32FrmInfoExtAvail =
	    VDECFrame->stHDRInfo.u32FrmInfoExtAvail;

	pstFrmInfo->stHDRInfo.stColorDescription.u8ColorPrimaries =
	    VDECFrame->stHDRInfo.stColorDescription.u8ColorPrimaries;
	pstFrmInfo->stHDRInfo.stColorDescription.u8MatrixCoefficients =
	    VDECFrame->stHDRInfo.stColorDescription.u8MatrixCoefficients;
	pstFrmInfo->stHDRInfo.stColorDescription.u8TransferCharacteristics =
	    VDECFrame->stHDRInfo.stColorDescription.u8TransferCharacteristics;

	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u32MaxLuminance =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u32MaxLuminance;
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u32MinLuminance =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u32MinLuminance;
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[0][0] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[0][0];
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[0][1] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[0][1];
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[1][0] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[1][0];
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[1][1] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[1][1];
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[2][0] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[2][0];
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[2][1] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16DisplayPrimaries[2][1];
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16WhitePoint[0] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16WhitePoint[0];
	pstFrmInfo->stHDRInfo.stMasterColorDisplay.u16WhitePoint[1] =
	    VDECFrame->stHDRInfo.stMasterColorDisplay.u16WhitePoint[1];

	pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u8CurrentIndex =
	    VDECFrame->stHDRInfo.stDolbyHDRInfo.u8CurrentIndex;
	pstFrmInfo->stHDRInfo.stDolbyHDRInfo.phyHDRRegAddr =
	    VDECFrame->stHDRInfo.stDolbyHDRInfo.phyHDRRegAddr;
	pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u32HDRRegSize =
	    VDECFrame->stHDRInfo.stDolbyHDRInfo.u32HDRRegSize;
	pstFrmInfo->stHDRInfo.stDolbyHDRInfo.phyHDRLutAddr =
	    VDECFrame->stHDRInfo.stDolbyHDRInfo.phyHDRLutAddr;
	pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u32HDRLutSize =
	    VDECFrame->stHDRInfo.stDolbyHDRInfo.u32HDRLutSize;
	pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u8DMEnable =
	    VDECFrame->stHDRInfo.stDolbyHDRInfo.u8DMEnable;
	pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u8CompEnable =
	    VDECFrame->stHDRInfo.stDolbyHDRInfo.u8CompEnable;
#endif

	RES_DISPLAY.u32AspectWidth = pstFrameFormat->u32AspectWidth;
	RES_DISPLAY.u32AspectHeight = pstFrameFormat->u32AspectHeight;

	if (pstFrameFormat->u32Version > 4)
		RES_DISPLAY.s32CPLX = pstFrameFormat->s32CPLX;

	if (pstFrameFormat->u32Version > 5) {
		pstFrmInfo->stMFdecInfo.bAV1PpuBypassMode =
		    pstFrameFormat->bAV1PpuBypassMode;
		pstFrmInfo->stMFdecInfo.bMonoChrome =
		    pstFrameFormat->bMonoChrome;
		pstFrmInfo->stMFdecInfo.bAV1_mode =
		    VDECFrame->stMFdecInfo.bAV1_mode;
	}

	pstFrmInfo->bReleaseBuf = TRUE;

	if (pstFrmInfo->enScanType !=
	    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
		pstFrmInfo->u8ContinuousTime =
		    VDECFrame->u8ContinuousTime;

		if (pstPreFrmInfo->u8ContinuousTime <
		    pstFrmInfo->u8ContinuousTime) {
			pstPreFrmInfo->bReleaseBuf = FALSE;
		}
	}

	_display_store_flip_time(u16WritePointer, u32WindowID, TRUE);

	RES_DISPLAY.buffer_ptr.write_ptr =
	    _display_get_next_w_ptr(u32Window);

	if (RES_DISPLAY.bFirstPlay) {
		RES_DISPLAY.bFirstPlay = FALSE;
		pre_scan_type =
		    _display_get_scan_type(pstFrameFormat->u8Interlace);
	}

	return TRUE;
}



static bool _display_skip_task(INPUT_SOURCE_TYPE_t enInputSourceType,
				__u32 u32WindowID)
{
	DISPLAY_GET_RES_PRI;
	bool bRet = FALSE;
	__u32 u32PreRPtr =
	    _display_get_pre_id(RES_DISPLAY.buffer_ptr.read_ptr);
	__u32 u32RPtr = RES_DISPLAY.buffer_ptr.read_ptr;

	struct mtk_pq_vdec_frame_info *pstFrmInfo =
		&(RES_DISPLAY.frame_info[u32RPtr]);

	struct mtk_pq_vdec_frame_info *pstPreFrmInfo =
		&(RES_DISPLAY.frame_info[u32PreRPtr]);

	if (RES_DISPLAY.buffer_ptr.read_ptr ==
	    RES_DISPLAY.buffer_ptr.write_ptr) {
		pstPreFrmInfo->u32TaskTrigCounts++;
		if ((pstPreFrmInfo->bIsFlipTrigFrame == TRUE)
		    && pstPreFrmInfo->u32TaskTrigCounts == 2) {
			return TRUE;
		}
	} else {
		pstFrmInfo->u32TaskTrigCounts++;
	}

	if (is_fired && (swds_index != (MApi_XC_R2BYTE(0x131FF2) & 0xFF)) &&
	    (_display_is_ds_enable() == TRUE) &&
	    IsSrcTypeStorage(enInputSourceType)) {
		bRet = TRUE;
	}

	return bRet;
}


static bool _display_is_frame_refcount_zero(__u32 u32WindowID, __u16 u16BufID)
{
	DISPLAY_GET_RES_PRI;

	if (RES_DISPLAY.u8LocalFrameRefCount[u16BufID] == 0)
		return TRUE;
	else
		return FALSE;
}

static bool _display_source_frame_release(struct mtk_pq_device *pq_dev,
		__u32 u32WindowID, __u16 u16BufID)
{
	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	struct vb2_buffer *vb = RES_DISPLAY.frame_info[u16BufID].vb;

	if (RES_DISPLAY.frame_info[u16BufID].u32FrameIndex ==
	    MTK_PQ_INVALID_FRAME_ID)
		return TRUE;

	if (RES_DISPLAY.u8LocalFrameRefCount[u16BufID] == 0)
		return TRUE;

	RES_DISPLAY.u8LocalFrameRefCount[u16BufID]--;

	if (RES_DISPLAY.u8LocalFrameRefCount[u16BufID] == 0) {
		VDEC_StreamId VdecStreamId;
		VDEC_EX_DispFrame VdecDispFrm;
		VDEC_EX_Result eResult;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

		VdecStreamId.u32Version =
			RES_DISPLAY.frame_info
			[u16BufID].u32VDECStreamVersion;

		VdecStreamId.u32Id =
			RES_DISPLAY.frame_info
			[u16BufID].u32VDECStreamID;

		VdecDispFrm.u32Idx =
			RES_DISPLAY.frame_info
			[u16BufID].u32FrameIndex;

		VdecDispFrm.u32PriData =
			RES_DISPLAY.frame_info
			[u16BufID].u32PriData;

		eResult = MApi_VDEC_EX_ReleaseFrame(
			&VdecStreamId, &VdecDispFrm);

		if (eResult == E_VDEC_EX_FAIL)
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Release Frame fail!\n");

		if (_display_is_src_interlace
		    (RES_DISPLAY.frame_info[u16BufID].enScanType)) {
			VDEC_EX_Result eResult;

			VdecStreamId.u32Version =
				RES_DISPLAY.frame_info
				[u16BufID].u32VDECStreamVersion;

			VdecStreamId.u32Id =
				RES_DISPLAY.frame_info
				[u16BufID].u32VDECStreamID;

			VdecDispFrm.u32Idx =
				RES_DISPLAY.frame_info
				[u16BufID].u32FrameIndex_2nd;

			VdecDispFrm.u32PriData =
				RES_DISPLAY.frame_info
				[u16BufID].u32PriData_2nd;

			eResult =
			    MApi_VDEC_EX_ReleaseFrame(&VdecStreamId,
						      &VdecDispFrm);

			if (eResult == E_VDEC_EX_FAIL) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				    "Release Frame fail!\n");
			}
		}

		release_info[u32WindowID].u32VdecStreamId =
		    RES_DISPLAY.frame_info[u16BufID].u32VDECStreamID;
		release_info[u32WindowID].u32VdecStreamVersion =
		    RES_DISPLAY.frame_info[u16BufID].u32VDECStreamVersion;
		release_info[u32WindowID].u32WindowID = u32WindowID;
		release_info[u32WindowID].au32Idx[0] =
		    RES_DISPLAY.frame_info[u16BufID].u32FrameIndex;
		release_info[u32WindowID].au32Idx[1] =
		    RES_DISPLAY.frame_info[u16BufID].u32FrameIndex_2nd;
		release_info[u32WindowID].au32PriData[0] =
		    RES_DISPLAY.frame_info[u16BufID].u32PriData;
		release_info[u32WindowID].au32PriData[1] =
		    RES_DISPLAY.frame_info[u16BufID].u32PriData_2nd;
		release_info[u32WindowID].u64Pts =
		    RES_DISPLAY.frame_info[u16BufID].u64Pts;
		release_info[u32WindowID].u64PtsUs =
		    RES_DISPLAY.frame_info[u16BufID].u64PtsUs;
		release_info[u32WindowID].u32FrameNum =
		    RES_DISPLAY.frame_info[u16BufID].u32FrameNum;
		release_info[u32WindowID].u8ApplicationType =
		    RES_DISPLAY.frame_info[u16BufID].u8ApplicationType;
		release_info[u32WindowID].u32DisplayCounts =
		    RES_DISPLAY.frame_info[u16BufID].u32DisplayCounts;

		if (_display_is_frame_refcount_zero(u32WindowID, u16BufID))
			RES_DISPLAY.frame_info[u16BufID].u32FrameIndex =
			    MTK_PQ_INVALID_FRAME_ID;

		RES_DISPLAY.frame_info[u16BufID].u32DisplayCounts = 0;

		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

		if (RES_DISPLAY.frame_info[u16BufID].bReleaseBuf
		    && pq_dev->display_info.bReleaseTaskCreated) {
			struct v4l2_event ev;

			memset(&ev, 0, sizeof(struct v4l2_event));
			ev.type = V4L2_EVENT_MTK_PQ_INPUT_DONE;
			ev.u.data[0] = vb->index;
			v4l2_event_queue(&pq_dev->video_dev, &ev);
		}
	}

	return TRUE;
}

static bool _display_release_buffer(struct mtk_pq_device *pq_dev,
			__u32 u32WindowID)
{
	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	bool bRet = TRUE;
	__u32 u32Index = 0;
	__u32 u32PreIndex = 0;
	__u32 u32PPreIndex = 0;
	__u8 u8FrameKeepNum = SOURCE_FRAME_KEEP_NUM;

	DISPLAY_GET_RES_PRI;
	struct mtk_pq_vdec_frame_info *pstFrmInfo = NULL;
	struct mtk_pq_vdec_frame_info *pstPreFrmInfo = NULL;
	struct mtk_pq_vdec_frame_info *pstPPreFrmInfo = NULL;

	for (u32Index = 0; u32Index < MTK_PQ_QUEUE_DEPTH; u32Index++) {
		u8FrameKeepNum = SOURCE_FRAME_KEEP_NUM;
		u32PreIndex = _display_get_pre_id(u32Index);
		u32PPreIndex = _display_get_pre_id(u32PreIndex);

		pstFrmInfo = &(RES_DISPLAY.frame_info[u32Index]);
		pstPreFrmInfo = &(RES_DISPLAY.frame_info[u32PreIndex]);
		pstPPreFrmInfo = &(RES_DISPLAY.frame_info[u32PPreIndex]);

		if (_display_is_source_interlace(pstFrmInfo->enScanType))
			u8FrameKeepNum += 1;

		if (RES_DISPLAY.u32ReleaseState[u32Index] >= u8FrameKeepNum) {
			if (pstPreFrmInfo->bValid && (!pstPPreFrmInfo->bValid
			    || (RES_DISPLAY.buffer_ptr.write_ptr ==
			    u32PreIndex))) {
				_display_source_frame_release(pq_dev,
				    u32WindowID, u32PreIndex);

				RES_DISPLAY.u32ReleaseState[u32PreIndex] = 0;
				pstPreFrmInfo->bValid = FALSE;
				pstPreFrmInfo->bIsFlipTrigFrame = FALSE;
				pstPreFrmInfo->u32TaskTrigCounts = 0;
			}

			_display_source_frame_release(pq_dev,
			    u32WindowID, u32Index);

			RES_DISPLAY.u32ReleaseState[u32Index] = 0;
			pstFrmInfo->bValid = FALSE;
			pstFrmInfo->bIsFlipTrigFrame = FALSE;
			pstFrmInfo->u32TaskTrigCounts = 0;
		}
	}

	return bRet;
}

static __u32 _display_frc_get_read_pointer(__u32 u32WindowID, bool bEvenField)
{
	__u32 u32CurReadPointer = 0;

	DISPLAY_GET_RES_PRI;

	__u16 u16RPtr = RES_DISPLAY.buffer_ptr.read_ptr;
	__u16 u16WPtr = RES_DISPLAY.buffer_ptr.write_ptr;

	if ((u16RPtr == u16WPtr) || bEvenField)
		u32CurReadPointer = _display_get_pre_id(u16RPtr);
	else
		u32CurReadPointer = u16RPtr;

	return u32CurReadPointer;
}

static __u32 _display_frc_get_input_rate(__u32 u32WindowID,
		__u32 u32CurReadPointer, enum mtk_pq_frc_ctrl eCtlType)
{
	__u32 u32InputFrmRate = 0;

	DISPLAY_GET_RES_PRI;
	struct mtk_pq_vdec_frame_info *pstFrmInfo =
	    &(RES_DISPLAY.frame_info[u32CurReadPointer]);

	if (eCtlType == MTK_PQ_FRC_REPEAT) {
		if (pstFrmInfo->enScanType ==
		    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
			u32InputFrmRate = pstFrmInfo->u32FrameRate;
		} else {
			u32InputFrmRate = pstFrmInfo->u32FrameRate * 2;
		}
	} else if (pstFrmInfo->enScanType ==
	    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
		u32InputFrmRate = RES_DISPLAY.u32InputFrmRate;
	} else {
		u32InputFrmRate = RES_DISPLAY.u32InputFrmRate * 2;
	}

	return u32InputFrmRate;
}

static enum mtk_pq_frc_ctrl _display_frc_condition(__u32 u32InputFrmRate,
					__u32 u32OutputFrmRate)
{
	enum mtk_pq_frc_ctrl eType = MTK_PQ_FRC_MAX;

	if (u32InputFrmRate > u32OutputFrmRate)
		eType = MTK_PQ_FRC_DROP;
	else if (u32OutputFrmRate > u32InputFrmRate)
		eType = MTK_PQ_FRC_REPEAT;
	else
		eType = MTK_PQ_FRC_NONE;

	return eType;
}

static bool _display_frc_check(__u32 u32WindowID,
		bool bEvenField, enum mtk_pq_frc_ctrl eCtlType)
{
	DISPLAY_GET_RES_PRI;

	if (u32WindowID >= V4L2_MTK_MAX_WINDOW)
		return FALSE;

	__u32 u32CurReadPointer =
	    _display_frc_get_read_pointer(u32WindowID, bEvenField);
	__u32 u32InputFrmRate =
	    _display_frc_get_input_rate(u32WindowID,
	    u32CurReadPointer, eCtlType);
	__u32 u32OutputFrmRate = RES_DISPLAY.u32OutputFrmRate;
	__u32 u32ACC = RES_DISPLAY.u32AccumuleOutputRate;
	__u8 u8Offest;
	enum mtk_pq_frc_ctrl eSelectType = MTK_PQ_FRC_MAX;

	if ((!RES_DISPLAY.frame_info[u32CurReadPointer].bValid) ||
	    (!RES_DISPLAY.bIsEnFRC))
		return FALSE;

	eSelectType = _display_frc_condition(u32InputFrmRate, u32OutputFrmRate);

	if ((eSelectType != eCtlType) || (u32InputFrmRate == 0) ||
	    (u32OutputFrmRate == 0))
		return FALSE;

	if (eSelectType == MTK_PQ_FRC_DROP) {
		u32InputFrmRate = RES_DISPLAY.u32OutputFrmRate;

		if (RES_DISPLAY.frame_info[u32CurReadPointer].enScanType ==
		     MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
			u32OutputFrmRate = RES_DISPLAY.u32InputFrmRate;
		} else {
			u32OutputFrmRate = RES_DISPLAY.u32InputFrmRate * 2;
		}
	}

	u32ACC = u32ACC + u32InputFrmRate;
	u8Offest = u32ACC / u32OutputFrmRate;
	u32ACC = u32ACC % u32OutputFrmRate;

	RES_DISPLAY.u32AccumuleOutputRate = u32ACC;

	if (u8Offest == 0)
		return TRUE;
	else
		return FALSE;
}

static bool _display_check_pixel_shift(__s8 s8HOffset, __s8 s8VOffset)
{
	bool bChange = FALSE;

	bChange = s8HOffset || s8VOffset;

	return bChange;
}

static bool _display_get_fd_mask_delay_count(__u32 u32WindowID,
				__u32 u32CurReadPointer)
{
	bool bRet = TRUE;
	ST_XC_APISTATUSNODELAY stDrvStatus;

	DISPLAY_GET_RES_PRI;

	bool bUserSetWinFlag =
	    RES_DISPLAY.frame_info[u32CurReadPointer].bUserSetwinFlag;
	memset(&stDrvStatus, 0, sizeof(ST_XC_APISTATUSNODELAY));

	if (((RES_DISPLAY.bUpdateSetWin) || bUserSetWinFlag)
	    || (RES_DISPLAY.bOneFieldMode)) {
		stDrvStatus.u32ApiStatusEx_Version =
		    API_XCSTATUS_NODELAY_VERSION;

		MApi_XC_GetStatusNodelay(&stDrvStatus, MAIN_WINDOW);

		RES_DISPLAY.u32RWDiff =
		    MApi_XC_GetWRBankMappingNum(u32WindowID) +
		    stDrvStatus.u8UCMRwDiff +
		    stDrvStatus.u8FRCRwDiff;

		if (RES_DISPLAY.u32RWDiff % 2)
			RES_DISPLAY.u32RWDiff = RES_DISPLAY.u32RWDiff + 1;

		if ((RES_DISPLAY.bUpdateSetWin) || bUserSetWinFlag)
			RES_DISPLAY.u32RWDiff = RES_DISPLAY.u32RWDiff - 1;
	} else {
		RES_DISPLAY.u32RWDiff = 0;
	}

	return bRet;
}

static bool _display_update_fd_mask_status(__u32 u32WindowID,
			    bool bPauseStage, bool bEnableMute,
			    __u32 u32CurReadPointer, bool *pbRepeatLastField)
{
	if (pbRepeatLastField == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	bool bUserSetWinFlag =
	    RES_DISPLAY.frame_info[u32CurReadPointer].bUserSetwinFlag;

	if (bEnableMute
	    || (MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE ==
		RES_DISPLAY.frame_info[u32CurReadPointer].enScanType)) {
		RES_DISPLAY.bFDMaskEnable = FALSE;
		*pbRepeatLastField = FALSE;
	} else {
		if (bPauseStage
		    && !(RES_DISPLAY.bUpdateSetWin
			 || bUserSetWinFlag)) {
			if ((RES_DISPLAY.u32RWDiff == 0) && (!is_even_field)) {
				RES_DISPLAY.bFDMaskEnable = TRUE;
				*pbRepeatLastField = FALSE;
			}

			if (RES_DISPLAY.u32RWDiff > 0)
				RES_DISPLAY.u32RWDiff--;
		} else {
			_display_get_fd_mask_delay_count(u32WindowID,
			    u32CurReadPointer);

			if ((!is_even_field) && (field_change_cnt !=
			    MTK_PQ_FIELD_CHANGE_FIRST_FIELD)) {
				RES_DISPLAY.bFDMaskEnable = FALSE;

				if (bPauseStage && (RES_DISPLAY.bUpdateSetWin
				    || bUserSetWinFlag)) {
					*pbRepeatLastField = TRUE;
					if (_display_is_ds_enable() == FALSE)
						RES_DISPLAY.bUpdateSetWin =
						    FALSE;
				} else {
					*pbRepeatLastField = FALSE;
				}
			}
		}
	}
	if ((bPauseStage == FALSE) && (_display_is_ds_enable() == FALSE))
		RES_DISPLAY.bUpdateSetWin = FALSE;

	return TRUE;
}

static bool _display_get_mvop_and_vdec_field_type(__u32 u32WindowID,
				  __u32 u32CurReadPointer,
				  enum mtk_pq_field_type *penMvopFieldType,
				  enum mtk_pq_field_type *penVdecFieldType,
				  bool bRepeatLastField, bool bPauseStageFBL)
{
	if ((penMvopFieldType == NULL) || (penVdecFieldType == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	bool bRet = TRUE;
	bool bMvopIsBot = TRUE;
	MVOP_Handle stHd = { E_MVOP_MODULE_MAIN };
	MVOP_Result eMvopRet = E_MVOP_FAIL;

	DISPLAY_GET_RES_PRI;

	eMvopRet =
	    MDrv_MVOP_GetCommand(&stHd, E_MVOP_CMD_GET_IS_BOT_FIELD,
	    &bMvopIsBot, sizeof(bMvopIsBot));

	if (eMvopRet == E_MVOP_OK) {
		if (bMvopIsBot)
			*penMvopFieldType = MTK_PQ_FIELD_TYPE_TOP;
		else
			*penMvopFieldType = MTK_PQ_FIELD_TYPE_BOTTOM;
	} else {
		*penMvopFieldType = MTK_PQ_FIELD_TYPE_MAX;
		bRet = FALSE;
	}

	struct mtk_pq_vdec_frame_info *pstFrmInfo =
	    &(RES_DISPLAY.frame_info[u32CurReadPointer]);

	if (pstFrmInfo->bIs2ndField) {
		if (pstFrmInfo->enFieldOrderType ==
		    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP)
			*penVdecFieldType = MTK_PQ_FIELD_TYPE_BOTTOM;
		else
			*penVdecFieldType = MTK_PQ_FIELD_TYPE_TOP;
	} else {
		if (pstFrmInfo->enFieldOrderType ==
		    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP)
			*penVdecFieldType = MTK_PQ_FIELD_TYPE_TOP;
		else
			*penVdecFieldType = MTK_PQ_FIELD_TYPE_BOTTOM;
	}

	if (pstFrmInfo->u8FieldCtrl || bRepeatLastField || bPauseStageFBL) {
		if (pstFrmInfo->enFieldOrderType ==
		    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP)
			*penVdecFieldType = MTK_PQ_FIELD_TYPE_TOP;
		else
			*penVdecFieldType = MTK_PQ_FIELD_TYPE_BOTTOM;
		RES_DISPLAY.bOneFieldMode = TRUE;
	} else {
		RES_DISPLAY.bOneFieldMode = FALSE;
	}

return bRet;
}

static bool _display_update_read_pointer_by_field_change(__u32 u32WindowID,
				 enum mtk_pq_field_type enVdecFieldType,
				 __u32 u32ReadPointer)
{
	DISPLAY_GET_RES_PRI;

	__u16 u16PreRPtr = _display_get_pre_id(u32ReadPointer);

	struct mtk_pq_vdec_frame_info *pstFrmInfo =
	    &(RES_DISPLAY.frame_info[u32ReadPointer]);

	struct mtk_pq_vdec_frame_info *pstPreFrmInfo =
	    &(RES_DISPLAY.frame_info[u16PreRPtr]);

	if ((pstFrmInfo->enFieldOrderType != pstPreFrmInfo->enFieldOrderType)
	    || (pstFrmInfo->enScanType != pstPreFrmInfo->enScanType)) {
		return FALSE;
	}

	if ((pre_field_type != MTK_PQ_FIELD_TYPE_MAX) &&
	    (pre_field_type == enVdecFieldType) &&
	    (RES_DISPLAY.bOneFieldMode == FALSE) &&
	    (_display_get_next_id(u32ReadPointer) !=
		RES_DISPLAY.buffer_ptr.write_ptr)) {
		return TRUE;
	}

	return FALSE;
}

static bool _display_set_bob_mode(__u32 u32WindowID,
		struct mtk_pq_bob_mode_info stBOBModeInfo)
{
	DISPLAY_GET_RES_PRI;

	if (stBOBModeInfo.bResetBobMode == TRUE) {
		MApi_XC_SetBOBMode(FALSE, MAIN_WINDOW);
		RES_DISPLAY.bXCEnableBob = FALSE;

		return TRUE;
	}

	if (RES_DISPLAY.frame_info
	    [stBOBModeInfo.u32CurReadPointer].u8FieldCtrl) {
		if (RES_DISPLAY.bXCEnableBob == FALSE) {
			MApi_XC_SetBOBMode(TRUE, MAIN_WINDOW);
			RES_DISPLAY.bXCEnableBob = TRUE;
		}
	} else {
		if (RES_DISPLAY.bXCEnableBob == TRUE &&
		    stBOBModeInfo.bPauseStage == FALSE) {
			MApi_XC_SetBOBMode(FALSE, MAIN_WINDOW);
			RES_DISPLAY.bXCEnableBob = FALSE;
		}
	}

	return TRUE;
}

static bool _display_enable_fd_mask(bool bEnable)
{
	MVOP_Handle stMvopHd = { E_MVOP_MODULE_MAIN };

	if (MDrv_MVOP_SetCommand(&stMvopHd, E_MVOP_CMD_SET_FDMASK,
	    &bEnable) != E_MVOP_OK)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "E_MVOP_CMD_SET_FDMASK failed!!\n");

	return TRUE;
}

static enum mtk_pq_field_order_type _display_invert_field_order(
		enum mtk_pq_field_order_type enFieldOrderType)
{
	enum mtk_pq_field_order_type enOrderType =
	    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX;

	if (enFieldOrderType == MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP)
		enOrderType = MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM;
	else if (enFieldOrderType == MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM)
		enOrderType = MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP;
	else
		enOrderType = MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX;

	return enOrderType;
}

static bool _display_is_need_update_read_pointer_by_fod(__u32 u32WindowID,
	__u16 u16BufferId, enum mtk_pq_field_type enVdecFieldType)
{
	bool bUpdate = FALSE;

	DISPLAY_GET_RES_PRI;

	if (RES_DISPLAY.bFieldOrderChange) {
		bUpdate =
		    (((MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM ==
		       RES_DISPLAY.frame_info[u16BufferId].enFieldOrderType)
		      && (enVdecFieldType == MTK_PQ_FIELD_TYPE_BOTTOM))
		     ||
		     ((MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP ==
		       RES_DISPLAY.frame_info[u16BufferId].enFieldOrderType)
		      && (enVdecFieldType == MTK_PQ_FIELD_TYPE_TOP)));
	}

	return bUpdate;
}

static __u32 _display_detect_field_order(__u32 u32WindowID,
				 __u32 u32CurPointer, bool bPauseState,
				 enum mtk_pq_field_type enFieldType,
				 bool *pbUpdateReadPointerByFod)
{
	__u32 u32CurReadPointer = u32CurPointer;
	__u32 fod_read_ptr = u32CurPointer;
	XC_FOD_INFO stFodInfo;
	ST_XC_APISTATUSNODELAY stDrvStatus;
	*pbUpdateReadPointerByFod = FALSE;
	DISPLAY_GET_RES_PRI;

	memset(&stFodInfo, 0, sizeof(stFodInfo));
	memset(&stDrvStatus, 0, sizeof(ST_XC_APISTATUSNODELAY));

	if (RES_DISPLAY.detect_order_type ==
	    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX) {
		RES_DISPLAY.detect_order_type =
		    RES_DISPLAY.frame_info[u32CurPointer].enFieldOrderType;
	} else if (RES_DISPLAY.enPreFieldOrderType !=
	    RES_DISPLAY.frame_info[u32CurPointer].enFieldOrderType) {
		RES_DISPLAY.detect_order_type =
		    _display_invert_field_order(RES_DISPLAY.detect_order_type);
	} else {
		//STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");
	}

	stFodInfo.u16Length = sizeof(XC_FOD_INFO);
	stFodInfo.u8Win = MAIN_WINDOW;

	if (RES_DISPLAY.detect_order_type ==
	    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP)
		stFodInfo.u8Bff = 0;
	else
		stFodInfo.u8Bff = 1;

	if ((bPauseState == FALSE) && (RES_DISPLAY.bOneFieldMode == FALSE)) {
		stDrvStatus.u32ApiStatusEx_Version =
		    API_XCSTATUS_NODELAY_VERSION;

		MApi_XC_GetStatusNodelay(&stDrvStatus, MAIN_WINDOW);

		stFodInfo.s32Width = (__s32) stDrvStatus.u32OPSizeH;
		stFodInfo.s32Height = (__s32) stDrvStatus.u32OPSizeV;
	} else {
		stFodInfo.s32Width = MTK_PQ_INIT_FOD_WIDTH;
		stFodInfo.s32Height = MTK_PQ_INIT_FOD_HEIGHT;
	}

	stFodInfo.bFieldOrderChange = FALSE;

	MApi_XC_HDR_Control(E_XC_HDR_CTRL_GET_FOD_INFO, (void *)&stFodInfo);

	if ((bPauseState == FALSE) && (RES_DISPLAY.bOneFieldMode == FALSE)) {
		if (stFodInfo.bFieldOrderChange) {
			fod_read_ptr =
			    (u32CurPointer + MTK_PQ_QUEUE_DEPTH -
			     SOURCE_FRAME_KEEP_NUM) % MTK_PQ_QUEUE_DEPTH;
			if (RES_DISPLAY.u32ReleaseState[fod_read_ptr] == 0) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
					   "Regard the result of fod for using a released buffer\n");
			} else {
				RES_DISPLAY.detect_order_type =
				    _display_invert_field_order(
				    RES_DISPLAY.detect_order_type);

				RES_DISPLAY.bFieldOrderChange =
				    (!RES_DISPLAY.bFieldOrderChange);
			}
		}
		if (_display_is_need_update_read_pointer_by_fod(
		    u32WindowID, u32CurPointer, enFieldType)) {
			*pbUpdateReadPointerByFod = TRUE;
			u32CurReadPointer = _display_get_pre_id(u32CurPointer);
			RES_DISPLAY.u32ReleaseState[u32CurReadPointer] = 0;
			u32CurReadPointer =
			    _display_get_pre_id(u32CurReadPointer);
		}
	}
	//save current field type.
	RES_DISPLAY.enPreFieldOrderType =
	    RES_DISPLAY.frame_info[u32CurReadPointer].enFieldOrderType;

	return u32CurReadPointer;
}

static bool _display_get_field_type_for_forcep(__u32 u32WindowID,
			    enum mtk_pq_field_type *penMvopFieldType,
			    enum mtk_pq_field_type *penMvopFieldTypeForceP)
{
	if (penMvopFieldTypeForceP == NULL || penMvopFieldType == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	bool bMvopIsBot = TRUE;
	MVOP_Handle stHd = { E_MVOP_MODULE_MAIN };

	DISPLAY_GET_RES_PRI;

	if (RES_DISPLAY.bIsEnForceP) {
		if (MDrv_MVOP_GetCommand
		    (&stHd, E_MVOP_CMD_GET_IS_BOT_FIELD, &bMvopIsBot,
		     sizeof(bMvopIsBot)) == E_MVOP_OK) {
			if (bMvopIsBot)
				*penMvopFieldType =
				    MTK_PQ_FIELD_TYPE_TOP;
			else
				*penMvopFieldType =
				    MTK_PQ_FIELD_TYPE_BOTTOM;
		} else {
			*penMvopFieldType = MTK_PQ_FIELD_TYPE_MAX;
		}

		if (*penMvopFieldTypeForceP == MTK_PQ_FIELD_TYPE_TOP)
			*penMvopFieldTypeForceP = MTK_PQ_FIELD_TYPE_BOTTOM;
		else
			*penMvopFieldTypeForceP = MTK_PQ_FIELD_TYPE_TOP;
	} else {
		*penMvopFieldType = MTK_PQ_FIELD_TYPE_MAX;
		*penMvopFieldTypeForceP = MTK_PQ_FIELD_TYPE_MAX;
	}

	return TRUE;
}

static bool _display_is_window_size_over_fhd(__u32 u32Width, __u32 u32Height)
{
	if ((u32Width > MTK_PQ_FHD_MAX_H_SIZE)
		|| (u32Height > MTK_PQ_FHD_MAX_V_SIZE))
		return TRUE;
	else
		return FALSE;
}

static bool _display_is_vdec_support_second_buffer(void)
{
	return TRUE;		//need get from DT----by sid.sun
}

static bool _display_set_scaling_down_condition(struct mtk_pq_window *ptCropWin,
				struct mtk_pq_window *ptDstWin)
{
	if ((ptCropWin == NULL) || (ptDstWin == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	__u32 u32WindowID = 0;

	DISPLAY_GET_RES_PRI;
	RES_DISPLAY.scaling_condition = 0;

	if (_display_is_window_size_over_fhd(ptCropWin->u32width,
	    ptCropWin->u32height)) {
		if (ptDstWin->u32width >= ptCropWin->u32width &&
		    ptDstWin->u32height >= ptCropWin->u32height) {
			RES_DISPLAY.scaling_condition |= MTK_PQ_VSD_XC_FBL;
		} else if ((ptDstWin->u32width >=
		    ((ptCropWin->u32width * MTK_PQ_VSD_90_PERCENT) /
		    MTK_PQ_VSD_100_PERCENT))
		    && (ptDstWin->u32height >=
		    ((ptCropWin->u32height * MTK_PQ_VSD_90_PERCENT) /
		    MTK_PQ_VSD_100_PERCENT))) {
			RES_DISPLAY.scaling_condition |=
			    MTK_PQ_VSD_XC_SCALING_DOWN;
		} else if ((ptDstWin->u32width >=
		    ((ptCropWin->u32width * MTK_PQ_VSD_50_PERCENT) /
		    MTK_PQ_VSD_100_PERCENT))
		    && (ptDstWin->u32height >=
		    ((ptCropWin->u32height * MTK_PQ_VSD_50_PERCENT) /
		    MTK_PQ_VSD_100_PERCENT))) {
			if (_display_is_vdec_support_second_buffer() == TRUE)
				RES_DISPLAY.scaling_condition |=
				    MTK_PQ_VSD_2ND_BUFFER;
			else
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
				    "VDEC no support 2nd buffer\n");
		} else {
			if (_display_is_vdec_support_second_buffer() == TRUE)
				RES_DISPLAY.scaling_condition |=
				    MTK_PQ_VSD_2ND_BUFFER;
			else
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
				    "VDEC no support 2nd buffer\n");
		}
	}

	return TRUE;
}

static bool _display_is_scaling_use_second_buffer(__u32 u32ScalingCondition)
{
	if (u32ScalingCondition & MTK_PQ_VSD_2ND_BUFFER)
		return TRUE;
	else
		return FALSE;
}

static void _display_get_mvop_drop_line_condition(__u32 u32WindowID,
			__u32 u32SrcHeight, __u32 u32DispHeight)
{
	DISPLAY_GET_RES_PRI;

	if (u32DispHeight < ((u32SrcHeight * MTK_PQ_VSD_125_PERCENT) /
	    (MTK_PQ_VSD_100_PERCENT * 10))) {
		RES_DISPLAY.enMVOPDropline = MTK_PQ_MVOP_DROP_LINE_1_8;
	} else if (u32DispHeight < ((u32SrcHeight * MTK_PQ_VSD_25_PERCENT) /
	    MTK_PQ_VSD_100_PERCENT)) {
		RES_DISPLAY.enMVOPDropline = MTK_PQ_MVOP_DROP_LINE_1_4;
	} else if (u32DispHeight < ((u32SrcHeight * MTK_PQ_VSD_50_PERCENT) /
	    MTK_PQ_VSD_100_PERCENT)) {
		RES_DISPLAY.enMVOPDropline = MTK_PQ_MVOP_DROP_LINE_1_2;
	} else {
		RES_DISPLAY.enMVOPDropline = MTK_PQ_MVOP_DROP_LINE_DISABLE;
	}
}

static bool _display_execute_swdr(__u8 u8CurID,
			  enum mtk_pq_field_type enFieldType,
			  enum mtk_pq_field_order_type enFieldOrderType,
			  __u32 u32WindowID)
{
	__u8 u8PreHistID = _display_get_pre_id(u8CurID);
	__u8 u8PrepreHistID = _display_get_pre_id(u8PreHistID);

	DISPLAY_GET_RES_PRI;

	if (RES_DISPLAY.bHistogramValid) {
		hal_delay_time[MEASURE_TIME_SWDR].u32StartTime =
		    _display_get_time();
		XC_SWDR_INPUT st_SWDRIn;

		memset(&st_SWDRIn, 0, sizeof(XC_SWDR_INPUT));
		st_SWDRIn.u32Version = SWDR_INPUT_STRUCTURE_VERSION;
		st_SWDRIn.u32Length = sizeof(XC_SWDR_INPUT);
		st_SWDRIn.eHist_type = DRV_HIST_TYPE_NULL;

		if (RES_DISPLAY.frame_info[u8CurID].enScanType ==
		    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
			st_SWDRIn.pHist_Curr =
			    RES_DISPLAY.u32Histogram[u8CurID];
			st_SWDRIn.pHist_Prev =
			    RES_DISPLAY.u32Histogram[u8PreHistID];
			st_SWDRIn.eHist_type = DRV_HIST_TYPE_PMode;
		} else if ((RES_DISPLAY.frame_info[u8CurID].enScanType ==
		    MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME) &&
		    (RES_DISPLAY.frame_info[u8PrepreHistID].enScanType ==
		    MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME)) {
			st_SWDRIn.pHist_Curr =
			    RES_DISPLAY.u32Histogram[u8CurID];
			st_SWDRIn.pHist_Prev =
			    RES_DISPLAY.u32Histogram[u8PrepreHistID];
			st_SWDRIn.eHist_type = DRV_HIST_TYPE_IFrameMode;
		} else if ((RES_DISPLAY.frame_info[u8CurID].enScanType ==
		    MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD) &&
		    (RES_DISPLAY.frame_info[u8PrepreHistID].enScanType ==
		    MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD)) {
			__u8 u8Bins = 0;

			memset(cur_histogram, 0, sizeof(cur_histogram));
			memset(pre_histogram, 0, sizeof(pre_histogram));
			if ((enFieldType == MTK_PQ_FIELD_TYPE_BOTTOM) &&
			    (enFieldOrderType ==
			    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM)) {
				for (u8Bins = 0; u8Bins <
				    V4L2_MTK_HISTOGRAM_INDEX * 2;
				    u8Bins++) {
					if (u8Bins <
					    V4L2_MTK_HISTOGRAM_INDEX) {
						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8CurID][u8Bins];
					} else {
						__u8 u8TopHistID =
						    _display_get_next_id(
						    u8CurID);

						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8TopHistID][(u8Bins -
						    V4L2_MTK_HISTOGRAM_INDEX)];
					}
				}

				for (u8Bins = 0; u8Bins <
				    V4L2_MTK_HISTOGRAM_INDEX * 2; u8Bins++) {
					if (u8Bins < V4L2_MTK_HISTOGRAM_INDEX) {
						pre_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8PrepreHistID][u8Bins];
					} else {
						__u8 u8TopHistID =
						    _display_get_next_id(
						    u8PrepreHistID);

						pre_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8TopHistID][(u8Bins -
						    V4L2_MTK_HISTOGRAM_INDEX)];
					}
				}
				st_SWDRIn.pHist_Curr = cur_histogram;
				st_SWDRIn.pHist_Prev = pre_histogram;
				st_SWDRIn.eHist_type =
				    DRV_HIST_TYPE_I1stFieldMode;
			} else if ((enFieldType ==
			    MTK_PQ_FIELD_TYPE_BOTTOM) &&
			    (enFieldOrderType ==
			    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP)) {
				for (u8Bins = 0; u8Bins <
				    V4L2_MTK_HISTOGRAM_INDEX * 2; u8Bins++) {
					if (u8Bins < V4L2_MTK_HISTOGRAM_INDEX) {
						__u8 u8TopHistID =
						    _display_get_pre_id(
						    u8CurID);

						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8TopHistID][u8Bins];
					} else {
						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8CurID][(u8Bins -
						    V4L2_MTK_HISTOGRAM_INDEX)];
					}
				}

				st_SWDRIn.pHist_Curr = cur_histogram;
				st_SWDRIn.pHist_Prev = NULL;
				st_SWDRIn.eHist_type =
				    DRV_HIST_TYPE_I2ndFieldMode;
			} else if ((enFieldType ==
			    MTK_PQ_FIELD_TYPE_TOP) &&
			    (enFieldOrderType ==
			    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM)) {
				for (u8Bins = 0; u8Bins <
				    V4L2_MTK_HISTOGRAM_INDEX * 2; u8Bins++) {
					if (u8Bins < V4L2_MTK_HISTOGRAM_INDEX) {
						__u8 u8BotHistID =
						    _display_get_pre_id(
						    u8CurID);

						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8BotHistID][u8Bins];
					} else {
						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8CurID][(u8Bins -
						    V4L2_MTK_HISTOGRAM_INDEX)];
					}
				}
				st_SWDRIn.pHist_Curr = cur_histogram;
				st_SWDRIn.pHist_Prev = NULL;
				st_SWDRIn.eHist_type =
				    DRV_HIST_TYPE_I2ndFieldMode;
			} else if ((enFieldType ==
			    MTK_PQ_FIELD_TYPE_TOP) &&
			    (enFieldOrderType ==
			    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP)) {
				for (u8Bins = 0; u8Bins <
				    (V4L2_MTK_HISTOGRAM_INDEX * 2); u8Bins++) {
					if (u8Bins < V4L2_MTK_HISTOGRAM_INDEX) {
						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8CurID][u8Bins];
					} else {
						__u8 u8BotHistID =
						    _display_get_next_id(
						    u8CurID);

						cur_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8BotHistID][(u8Bins -
						    V4L2_MTK_HISTOGRAM_INDEX)];
					}
				}

				for (u8Bins = 0; u8Bins <
				    (V4L2_MTK_HISTOGRAM_INDEX * 2); u8Bins++) {
					if (u8Bins < V4L2_MTK_HISTOGRAM_INDEX) {
						pre_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8PrepreHistID][u8Bins];
					} else {
						__u8 u8BotHistID =
						    _display_get_next_id(
						    u8PrepreHistID);

						pre_histogram[u8Bins] =
						    RES_DISPLAY.u32Histogram
						    [u8BotHistID][(u8Bins -
						    V4L2_MTK_HISTOGRAM_INDEX)];
					}
				}
				st_SWDRIn.pHist_Curr = cur_histogram;
				st_SWDRIn.pHist_Prev = pre_histogram;
				st_SWDRIn.eHist_type =
				    DRV_HIST_TYPE_I1stFieldMode;
			}
		}

		st_SWDRIn.u16Hist_size = V4L2_MTK_HISTOGRAM_INDEX * 2;

		if (MApi_XC_SetSWDRInputData) {
			MApi_XC_SetSWDRInputData(&st_SWDRIn, MAIN_WINDOW);
		} else {
			static bool bLogFlag = FALSE;

			if (!bLogFlag)
				bLogFlag = TRUE;
		}

		hal_delay_time[MEASURE_TIME_SWDR].u32DelayTime =
		    _display_get_delay_time(hal_delay_time
		    [MEASURE_TIME_SWDR].u32StartTime);
	} else {
		XC_SWDR_INPUT st_SWDRIn;

		memset(&st_SWDRIn, 0, sizeof(XC_SWDR_INPUT));
		st_SWDRIn.u32Version = SWDR_INPUT_STRUCTURE_VERSION;
		st_SWDRIn.u32Length = sizeof(XC_SWDR_INPUT);
		// use default curve
		st_SWDRIn.eHist_type = DRV_HIST_TYPE_NULL;

		if (MApi_XC_SetSWDRInputData) {
			MApi_XC_SetSWDRInputData(&st_SWDRIn, MAIN_WINDOW);
		} else {
			static bool bLogFlag = FALSE;

			if (!bLogFlag)
				bLogFlag = TRUE;
		}
	}

	return TRUE;
}

static EN_XC_HDR_COLOR_FORMAT _display_trans_xc_color_format(
			enum mtk_pq_video_data_format video_data_format)
{
	EN_XC_HDR_COLOR_FORMAT eXCColorFormat = E_XC_HDR_COLOR_MAX;

	switch (video_data_format) {
	case MTK_PQ_VIDEO_DATA_FMT_YUV420:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_H265:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_H265_10BITS:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_PLANER:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_SEMI_PLANER:
		eXCColorFormat = E_XC_HDR_COLOR_YUV420;
		break;

	case MTK_PQ_VIDEO_DATA_FMT_YUV422:

	case MTK_PQ_VIDEO_DATA_FMT_YC422:
		eXCColorFormat = E_XC_HDR_COLOR_YUV422;
		break;

	case MTK_PQ_VIDEO_DATA_FMT_RGB565:

	case MTK_PQ_VIDEO_DATA_FMT_ARGB8888:
		eXCColorFormat = E_XC_HDR_COLOR_RGB;
		break;

	default:
		eXCColorFormat = E_XC_HDR_COLOR_MAX;
		break;
	}

	return eXCColorFormat;
}

static bool _display_send_frame_info(__u8 u8CurID,
			    enum mtk_pq_field_type enFieldType,
			    enum mtk_pq_field_order_type enFieldOrderType,
			    __u32 u32WindowID)
{
	ST_XC_FRAME_INFO stFrameInfo;

	DISPLAY_GET_RES_PRI;

	memset(&stFrameInfo, 0, sizeof(ST_XC_FRAME_INFO));
	stFrameInfo.u32Version = FRAME_INFO_STRUCTURE_VERSION;
	stFrameInfo.u32Length = sizeof(ST_XC_FRAME_INFO);
	stFrameInfo.u8BotFlag = (enFieldType ==
	    MTK_PQ_FIELD_TYPE_BOTTOM) ? 1 : 0;
	stFrameInfo.u32QPAvg = RES_DISPLAY.u32QPAvg[u8CurID];
	stFrameInfo.u32QPMin = RES_DISPLAY.u32QPMin[u8CurID];
	stFrameInfo.u32QPMax = RES_DISPLAY.u32QPMax[u8CurID];
	stFrameInfo.u32CodecType =
	    (__u32) RES_DISPLAY.frame_info[u8CurID].enCODEC;
	stFrameInfo.u32FrameType =
	    (__u32) RES_DISPLAY.frame_info[u8CurID].enFrameType;

#if (VERSION_mtk_pq_frame_info > 4)
	stFrameInfo.s32CPLX = RES_DISPLAY.s32CPLX;
#endif

	if (RES_DISPLAY.frame_info[u8CurID].enScanType ==
	    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
		stFrameInfo.enScanType = DRV_HIST_TYPE_PMode;
	} else if (RES_DISPLAY.frame_info[u8CurID].enScanType ==
	    MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME) {
		stFrameInfo.enScanType = DRV_HIST_TYPE_IFrameMode;
	} else if (RES_DISPLAY.frame_info[u8CurID].enScanType ==
	    MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD) {
		if (enFieldType == MTK_PQ_FIELD_TYPE_BOTTOM) {
			if (enFieldOrderType ==
			    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM)
				stFrameInfo.enScanType =
				    DRV_HIST_TYPE_I1stFieldMode;
			else
				stFrameInfo.enScanType =
				    DRV_HIST_TYPE_I2ndFieldMode;
		} else {
			if (enFieldOrderType ==
			    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM)
				stFrameInfo.enScanType =
				    DRV_HIST_TYPE_I2ndFieldMode;
			else
				stFrameInfo.enScanType =
				    DRV_HIST_TYPE_I1stFieldMode;
		}
	} else {
		stFrameInfo.enScanType = DRV_HIST_TYPE_NULL;
	}

	stFrameInfo.enStreamColorFormat =
	    _display_trans_xc_color_format(
	    RES_DISPLAY.frame_info[u8CurID].enFmt);

	if (MApi_XC_SetFrameInfo) {
		MApi_XC_SetFrameInfo(&stFrameInfo, MAIN_WINDOW);
	} else {
		static bool _bLogFlag = FALSE;

		if (!_bLogFlag)
			_bLogFlag = TRUE;
	}

	return TRUE;
}

static __u16 _display_mvop_drop_line_update_pitch(__u16 u16Pitch,
				  enum mtk_pq_drop_line enMvopDropLine)
{
	__u16 u16TargetPitch = 0;

	switch (enMvopDropLine) {
	case MTK_PQ_MVOP_DROP_LINE_DISABLE:
		u16TargetPitch = u16Pitch;
		break;

	case MTK_PQ_MVOP_DROP_LINE_1_2:
		u16TargetPitch = u16Pitch;
		break;

	case MTK_PQ_MVOP_DROP_LINE_1_4:
		u16TargetPitch = u16Pitch * 2;
		break;

	case MTK_PQ_MVOP_DROP_LINE_1_8:
		u16TargetPitch = u16Pitch * 4;
		break;

	default:
		break;
	}

	return u16TargetPitch;
}

static __u32 _display_drop_line_update_h(__u32 u32Height,
				  enum mtk_pq_drop_line enMvopDropLine,
				  enum mtk_pq_update_height enUpdateType)
{
	__u32 u32TargetHeight = 0;
	__u8 u8UpdateRatio = 0;

	if (enUpdateType == MTK_PQ_UPDATE_HEIGHT_MVOP)
		u8UpdateRatio = 1;
	else
		u8UpdateRatio = 2;

	switch (enMvopDropLine) {
	case MTK_PQ_MVOP_DROP_LINE_DISABLE:
		u32TargetHeight = u32Height;
		break;

	case MTK_PQ_MVOP_DROP_LINE_1_2:
		u32TargetHeight = u32Height / (u8UpdateRatio);
		break;

	case MTK_PQ_MVOP_DROP_LINE_1_4:
		u32TargetHeight = u32Height / (u8UpdateRatio * 2);
		break;

	case MTK_PQ_MVOP_DROP_LINE_1_8:
		u32TargetHeight = u32Height / (u8UpdateRatio * 4);
		break;

	default:
		break;
	}

	return u32TargetHeight;
}

static bool _display_get_mvop_disp_size(__u32 u32WindowID,
				__u32 u32CurReadPointer,
				MVOP_DMSDisplaySize *pstMVOPDispSize)
{
	if (pstMVOPDispSize == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	MVOP_Timing stMVOPTiming;

	memset(&stMVOPTiming, 0, sizeof(MVOP_Timing));

	MDrv_MVOP_GetOutputTiming(&stMVOPTiming);

	DISPLAY_GET_RES_PRI;

	pstMVOPDispSize->u32ApiDMSSize_Version = API_MVOP_DMS_DISP_SIZE_VERSION;
	pstMVOPDispSize->u16ApiDMSSize_Length = sizeof(MVOP_DMSDisplaySize);
	pstMVOPDispSize->u16Width =
	    RES_DISPLAY.frame_info[u32CurReadPointer].u16SrcWidth;
	pstMVOPDispSize->u16Height =
	    RES_DISPLAY.frame_info[u32CurReadPointer].u16SrcHeight;

	if (_display_is_scaling_use_second_buffer(
	    RES_DISPLAY.scaling_condition)) {
		pstMVOPDispSize->u16Pitch[0] =
		    _display_mvop_drop_line_update_pitch(
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].u16Src2ndBufferPitch,
		    RES_DISPLAY.enMVOPDropline);

		pstMVOPDispSize->u16Width =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].u16Src2ndBufferWidth;

		pstMVOPDispSize->u16Height =
		    _display_drop_line_update_h(
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].u16Src2ndBufferHeight,
		    RES_DISPLAY.enMVOPDropline,
		    MTK_PQ_UPDATE_HEIGHT_MVOP);
	} else {
		pstMVOPDispSize->u16Pitch[0] =
		    RES_DISPLAY.frame_info[u32CurReadPointer].u16SrcPitch;
		if (RES_DISPLAY.bIsXCEnFBL == TRUE) {
			pstMVOPDispSize->u16Height =
			    _display_drop_line_update_h(
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].u16SrcHeight,
			    RES_DISPLAY.enMVOPDropline,
			    MTK_PQ_UPDATE_HEIGHT_MVOP);

			pstMVOPDispSize->u16Pitch[0] =
			    _display_mvop_drop_line_update_pitch(
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].u16SrcPitch,
			    RES_DISPLAY.enMVOPDropline);
		}
	}

	if (RES_DISPLAY.frame_info[u32CurReadPointer].b10bitData) {
		pstMVOPDispSize->u16Pitch[1] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].u16Src10bitPitch;
	}

	pstMVOPDispSize->U8DSIndex = 0xff;
	pstMVOPDispSize->bDualDV_EN = 0;
	pstMVOPDispSize->bHDup = stMVOPTiming.bHDuplicate;
	pstMVOPDispSize->bVDup = 0;

	return TRUE;
}

static MVOP_TileFormat _display_transfer_mvop_tile_mode(
				enum mtk_pq_tile_mode eTile)
{
	MVOP_TileFormat u32TileMode = E_MVOP_TILE_NONE;

	switch (eTile) {
	case MTK_PQ_VIDEO_TILE_MODE_16x32:
		u32TileMode = E_MVOP_TILE_16x32;
		break;

	case MTK_PQ_VIDEO_TILE_MODE_NONE:
		u32TileMode = E_MVOP_TILE_NONE;
		break;

	case MTK_PQ_VIDEO_TILE_MODE_32x16:
		u32TileMode = E_MVOP_TILE_32x16;
		break;

	case MTK_PQ_VIDEO_TILE_MODE_32x32:
		u32TileMode = E_MVOP_TILE_32x32;
		break;

	case MTK_PQ_VIDEO_TILE_MODE_4x2_COMPRESSION_MODE:
		u32TileMode = E_MVOP_TILE_COMP_4x2;
		break;

	case MTK_PQ_VIDEO_TILE_MODE_8x1_COMPRESSION_MODE:
		u32TileMode = E_MVOP_TILE_COMP_8x1;
		break;

	case MTK_PQ_VIDEO_TILE_MODE_TILE_32X16_82PACK:
		u32TileMode = E_MVOP_TILE_COMP_32x16_82PACK;
		break;

	default:
		break;
	}

	return u32TileMode;
}

static bool _display_is_422_mode(enum mtk_pq_video_data_format eFmt)
{
	bool bRet = FALSE;

	switch (eFmt) {
	case MTK_PQ_VIDEO_DATA_FMT_YUV420:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_H265:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_H265_10BITS:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_PLANER:

	case MTK_PQ_VIDEO_DATA_FMT_YUV420_SEMI_PLANER:
		bRet = FALSE;
		break;

	default:
		bRet = TRUE;
		break;
	}

	return bRet;
}

static bool _display_get_mvop_stream_info(__u32 u32WindowID,
				  __u32 u32CurReadPointer,
				  MVOP_DMSStreamINFO *pstMVOPStreamInfo)
{
	if (pstMVOPStreamInfo == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	pstMVOPStreamInfo->u32ApiDMSStream_Version =
	    API_MVOP_DMS_STREAM_VERSION;
	pstMVOPStreamInfo->u16ApiDMSStream_Length =
	    sizeof(MVOP_DMSStreamINFO);
	pstMVOPStreamInfo->eTileFormat =
	    _display_transfer_mvop_tile_mode(
	    RES_DISPLAY.frame_info[u32CurReadPointer].enTileMode);
	pstMVOPStreamInfo->bIs422Mode =
	    _display_is_422_mode(
	    RES_DISPLAY.frame_info[u32CurReadPointer].enFmt);
	pstMVOPStreamInfo->bIsDRAMCont =
	    (pstMVOPStreamInfo->bIs422Mode == TRUE) ? TRUE : FALSE;
	pstMVOPStreamInfo->bDDR4_REMAP = 0;
	pstMVOPStreamInfo->bDS_EN = _display_is_ds_enable();

	if (_display_is_scaling_use_second_buffer(
	    RES_DISPLAY.scaling_condition)) {
		pstMVOPStreamInfo->eTileFormat =
		    _display_transfer_mvop_tile_mode(
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].u8Src2ndBufferTileMode);
	}

	return TRUE;
}

static bool _display_get_first_view_addr_info(__u32 u32WindowID,
				    __u32 u32CurReadPointer,
				    MVOP_DMSDisplayADD *pstMvopDispAddr)
{
	if (pstMvopDispAddr == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	if (RES_DISPLAY.frame_info[u32CurReadPointer].b10bitData) {
		pstMvopDispAddr->u32LSB_FB_ADDR[0] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddr_2bit;
		pstMvopDispAddr->u32LSB_FB_ADDR[1] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddr_2bit;
		pstMvopDispAddr->u32LSB_FB_MIU[0] =
		    (RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddr_2bit >=
		     HAL_MIU1_BASE) ? 1 : 0;
		pstMvopDispAddr->u32LSB_FB_MIU[1] =
		    (RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddr_2bit >=
		     HAL_MIU1_BASE) ? 1 : 0;
		pstMvopDispAddr->u32BIT_DEPTH[0] = 10;
		pstMvopDispAddr->u32BIT_DEPTH[1] = 10;
	} else {
		pstMvopDispAddr->u32BIT_DEPTH[0] = 8;
		pstMvopDispAddr->u32BIT_DEPTH[1] = 8;

	}

	pstMvopDispAddr->u32MSB_FB_ADDR[0] =
	    RES_DISPLAY.frame_info[u32CurReadPointer].phySrcLumaAddr;
	pstMvopDispAddr->u32MSB_FB_ADDR[1] =
	    RES_DISPLAY.frame_info[u32CurReadPointer].phySrcChromaAddr;
	pstMvopDispAddr->u8DMSB_FB_MIU[0] =
	    (RES_DISPLAY.frame_info[u32CurReadPointer].phySrcLumaAddr >=
	    HAL_MIU1_BASE) ? 1 : 0;
	pstMvopDispAddr->u8DMSB_FB_MIU[1] =
	    (RES_DISPLAY.frame_info[u32CurReadPointer].phySrcChromaAddr >=
	     HAL_MIU1_BASE) ? 1 : 0;

	return TRUE;
}

static bool _display_get_right_view_addr_info(__u32 u32WindowID,
				    __u32 u32CurReadPointer,
				    MVOP_DMSDisplayADD *pstMvopDispAddr)
{
	if (pstMvopDispAddr == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	pstMvopDispAddr->bEN3D = TRUE;
	MVOP_Handle stHd = { E_MVOP_MODULE_MAIN };
	MVOP_DrvMirror enMvopMirror = E_VOPMIRROR_NONE;

	MDrv_MVOP_GetCommand(&stHd, E_MVOP_CMD_GET_MIRROR_MODE, &enMvopMirror,
			     sizeof(MVOP_DrvMirror));

	if ((RES_DISPLAY.frame_info[u32CurReadPointer].u83DMode == 1)
	    && ((enMvopMirror == E_VOPMIRROR_HVBOTH) ||
	    (enMvopMirror == E_VOPMIRROR_HORIZONTALL))) {
		pstMvopDispAddr->u32RViewFB[0] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddr;

		pstMvopDispAddr->u32RViewFB[1] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddr;

		pstMvopDispAddr->u32MSB_FB_ADDR[0] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddrI;

		pstMvopDispAddr->u32MSB_FB_ADDR[1] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddrI;

		pstMvopDispAddr->u8DMSB_FB_MIU[0] =
		    (RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddrI >=
		     HAL_MIU1_BASE) ? 1 : 0;

		pstMvopDispAddr->u8DMSB_FB_MIU[1] =
		    (RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddrI >=
		     HAL_MIU1_BASE) ? 1 : 0;
	} else {
		pstMvopDispAddr->u32RViewFB[0] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddrI;

		pstMvopDispAddr->u32RViewFB[1] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddrI;
	}

	return TRUE;
}

static bool _display_get_second_view_addr_info(__u32 u32WindowID,
				    __u32 u32CurReadPointer,
				    MVOP_DMSDisplayADD *pstMvopDispAddr)
{
	if (pstMvopDispAddr == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	if (RES_DISPLAY.frame_info[u32CurReadPointer].b10bitData) {
		pstMvopDispAddr->u32LSB_FB_ADDR[0] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddrI_2bit;

		pstMvopDispAddr->u32LSB_FB_ADDR[1] =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddrI_2bit;

		pstMvopDispAddr->u32LSB_FB_MIU[0] =
		    (RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcLumaAddrI_2bit >=
		     HAL_MIU1_BASE) ? 1 : 0;

		pstMvopDispAddr->u32LSB_FB_MIU[1] =
		    (RES_DISPLAY.frame_info
		    [u32CurReadPointer].phySrcChromaAddrI_2bit >=
		     HAL_MIU1_BASE) ? 1 : 0;
		pstMvopDispAddr->u32BIT_DEPTH[0] = 10;
		pstMvopDispAddr->u32BIT_DEPTH[1] = 10;
	} else {
		pstMvopDispAddr->u32BIT_DEPTH[0] = 8;
		pstMvopDispAddr->u32BIT_DEPTH[1] = 8;
	}
	pstMvopDispAddr->u32MSB_FB_ADDR[0] =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].phySrcLumaAddrI;

	pstMvopDispAddr->u32MSB_FB_ADDR[1] =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].phySrcChromaAddrI;

	pstMvopDispAddr->u8DMSB_FB_MIU[0] =
	    (RES_DISPLAY.frame_info
	    [u32CurReadPointer].phySrcLumaAddrI >= HAL_MIU1_BASE) ? 1 : 0;

	pstMvopDispAddr->u8DMSB_FB_MIU[1] =
	    (RES_DISPLAY.frame_info
	    [u32CurReadPointer].phySrcChromaAddrI >=
	     HAL_MIU1_BASE) ? 1 : 0;

	return TRUE;
}

static bool _display_get_mvop_disp_addr_info(__u32 u32WindowID,
				     __u32 u32CurReadPointer,
				     enum mtk_pq_scan_type enScanType,
				     enum mtk_pq_field_type enFieldType,
				     MVOP_DMSDisplayADD *pstMvopDispAddr)
{
	if (pstMvopDispAddr == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	bool bRet = FALSE;

	DISPLAY_GET_RES_PRI;

	// MVOP SetDispADDInfo
	pstMvopDispAddr->u32ApiDMSADD_Version = API_MVOP_DMS_DIPS_ADD_VERSION;
	pstMvopDispAddr->u16ApiDMSADD_Length = sizeof(MVOP_DMSDisplayADD);
	pstMvopDispAddr->u8FRAME_ID = 0;	// no use

	if (_display_is_scaling_use_second_buffer(
	    RES_DISPLAY.scaling_condition))
		RES_DISPLAY.frame_info
		    [u32CurReadPointer].stMFdecInfo.bMFDec_Enable = 0;

	pstMvopDispAddr->bMFDEC_EN =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.bMFDec_Enable;

	pstMvopDispAddr->u8MFDEC_ID =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.u8MFDec_Select;

	pstMvopDispAddr->u32UNCOMPRESS_MODE =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.bUncompress_mode;

	pstMvopDispAddr->u32BITLEN_FB_ADDR =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.phyBitlen_Base;

	if (RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.phyBitlen_Base >=
	    HAL_MIU1_BASE) {
		pstMvopDispAddr->u8BITLEN_FB_MIU = 1;
	} else {
		pstMvopDispAddr->u8BITLEN_FB_MIU =
		    RES_DISPLAY.frame_info
		    [u32CurReadPointer].stMFdecInfo.u8Bitlen_MiuSelect;
	}

	pstMvopDispAddr->u32BITLEN_FB_PITCH =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.u16Bitlen_Pitch;

	pstMvopDispAddr->u8FDMask = RES_DISPLAY.bFDMaskEnable;
	pstMvopDispAddr->bXCBOB_EN = FALSE;
	pstMvopDispAddr->bBLEN_SHT_MD =
	    (RES_DISPLAY.frame_info[u32CurReadPointer].enCODEC ==
	    MTK_PQ_VIDEO_CODEC_VP9) ? 1 : 0;

	switch (enScanType) {
	case MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE:
		_display_get_first_view_addr_info(u32WindowID,
		    u32CurReadPointer, pstMvopDispAddr);
		if (RES_DISPLAY.frame_info[u32CurReadPointer].enCODEC ==
		    MTK_PQ_VIDEO_CODEC_MVC) {
			_display_get_right_view_addr_info(u32WindowID,
			    u32CurReadPointer, pstMvopDispAddr);
		}

		pstMvopDispAddr->bOutputIMode = FALSE;
		if (_display_is_scaling_use_second_buffer(
		    RES_DISPLAY.scaling_condition)) {
			pstMvopDispAddr->u32MSB_FB_ADDR[0] =
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].phySrc2ndBufferLumaAddr;

			pstMvopDispAddr->u32MSB_FB_ADDR[1] =
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].phySrc2ndBufferChromaAddr;

			pstMvopDispAddr->u8DMSB_FB_MIU[0] =
			    (RES_DISPLAY.frame_info
			    [u32CurReadPointer].phySrc2ndBufferLumaAddr >=
			     HAL_MIU1_BASE) ? 1 : 0;

			pstMvopDispAddr->u8DMSB_FB_MIU[1] =
			    (RES_DISPLAY.frame_info
			    [u32CurReadPointer].phySrc2ndBufferChromaAddr >=
			    HAL_MIU1_BASE) ? 1 : 0;
			//stMVOPDispAddr.u32BIT_DEPTH[0] = 8;
			//stMVOPDispAddr.u32BIT_DEPTH[1] = 8;
		}

		if (RES_DISPLAY.bIsEnForceP) {
			pstMvopDispAddr->u8BotFlag = (enFieldType ==
			    MTK_PQ_FIELD_TYPE_BOTTOM) ? 1 : 0;
		}

		bRet = TRUE;
		break;

	case MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME:
		_display_get_first_view_addr_info(u32WindowID,
		    u32CurReadPointer, pstMvopDispAddr);

		pstMvopDispAddr->bOutputIMode = TRUE;
		pstMvopDispAddr->u8BotFlag = (enFieldType ==
		    MTK_PQ_FIELD_TYPE_BOTTOM) ? 1 : 0;
		bRet = TRUE;
		break;

	case MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD:
		if (enFieldType == MTK_PQ_FIELD_TYPE_TOP) {
			_display_get_first_view_addr_info(u32WindowID,
			    u32CurReadPointer, pstMvopDispAddr);
		} else {
			_display_get_second_view_addr_info(u32WindowID,
			    u32CurReadPointer, pstMvopDispAddr);
		}

		pstMvopDispAddr->bOutputIMode = TRUE;
		//MVOP u8BotFlag 1: Bottom, 0:Top
		pstMvopDispAddr->u8BotFlag = (enFieldType ==
		    MTK_PQ_FIELD_TYPE_BOTTOM) ? 1 : 0;
		bRet = TRUE;
		break;

	default:
		break;
	}

	//mvop drop line
	if ((RES_DISPLAY.enMVOPDropline != MTK_PQ_MVOP_DROP_LINE_DISABLE)
	    && (RES_DISPLAY.enMVOPDropline != MTK_PQ_MVOP_DROP_LINE_MAX_NUM)) {
		pstMvopDispAddr->bOutputIMode = TRUE;
	}
	//update mvop time stamp
	RES_DISPLAY.u64TimeStamp = _display_get_time();
	pstMvopDispAddr->bForceTimeStamp = RES_DISPLAY.bEnableMVOPTimeStamp;
	pstMvopDispAddr->u64TimeStamp = RES_DISPLAY.u64TimeStamp;

	//av1 vdec info
	pstMvopDispAddr->bAV1Mode =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.bAV1_mode;

	pstMvopDispAddr->bAllowIntrabc =
	    RES_DISPLAY.frame_info
	    [u32CurReadPointer].stMFdecInfo.bAV1PpuBypassMode;

	pstMvopDispAddr->bOneField = RES_DISPLAY.bOneFieldMode;

	return bRet;
}

static bool _display_update_mvop_setting(__u32 u32WindowID,
				 __u32 u32CurReadPointer,
				 enum mtk_pq_field_type enFieldType)
{
	MVOP_DMSStreamINFO stMVOPStreamInfo;
	MVOP_DMSDisplaySize stMVOPDispSize;
	MVOP_DMSCropINFO stMVOPCropInfo;
	MVOP_DMSDisplayADD stMVOPDispAddr;
	MVOP_Handle stHd = { E_MVOP_MODULE_MAIN };
	MVOP_Result enRet = E_MVOP_FAIL;

	DISPLAY_GET_RES_PRI;

	memset(&stMVOPStreamInfo, 0, sizeof(MVOP_DMSStreamINFO));
	memset(&stMVOPDispSize, 0, sizeof(MVOP_DMSDisplaySize));
	memset(&stMVOPCropInfo, 0, sizeof(MVOP_DMSCropINFO));
	memset(&stMVOPDispAddr, 0, sizeof(MVOP_DMSDisplayADD));

	// MVOP DispSizeInfo
	_display_get_mvop_disp_size(u32WindowID,
	    u32CurReadPointer, &stMVOPDispSize);

	enRet = MDrv_MVOP_DMS_SetDispSizeInfo(&stHd,
	    E_MVOP_MAIN_WIN, &stMVOPDispSize, NULL);

	if (enRet != E_MVOP_OK) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "MDrv_MVOP_DMS_SetDispSizeInfo failed! ret = %d\n", enRet);
	}
	// MVOP StreamInfo
	_display_get_mvop_stream_info(u32WindowID,
	    u32CurReadPointer, &stMVOPStreamInfo);

	enRet = MDrv_MVOP_DMS_SetStreamInfo(&stHd,
	    E_MVOP_MAIN_WIN, &stMVOPStreamInfo, NULL);

	if (enRet != E_MVOP_OK) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "MDrv_MVOP_DMS_SetStreamInfo failed! ret = %d\n", enRet);
	}

	_display_get_mvop_disp_addr_info(u32WindowID, u32CurReadPointer,
	     RES_DISPLAY.frame_info[u32CurReadPointer].enScanType,
	     enFieldType, &stMVOPDispAddr);

	enRet = MDrv_MVOP_DMS_SetDispADDInfo(&stHd,
	    E_MVOP_MAIN_WIN, &stMVOPDispAddr, NULL);

	if (enRet != E_MVOP_OK) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "MDrv_MVOP_DMS_SetDispADDInfo failed! ret = %d\n", enRet);
	}

	return TRUE;
}

static void _display_update_video_delay_time(struct mtk_pq_device *pq_dev,
				 __u32 u32WindowID,
				 __u32 u32CurReadPointer, bool bPauseStage,
				 enum mtk_pq_field_type enFieldType,
				 bool bIsRepeatFrame)
{
	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	DISPLAY_GET_RES_PRI;
	if ((RES_DISPLAY.frame_info[u32CurReadPointer].u32DisplayCounts <
	     MTK_PQ_DISPLAYCNT) && (!bIsRepeatFrame)) {
		RES_DISPLAY.frame_info[u32CurReadPointer].u32DisplayCounts++;
	}
	//Check event active
	if (pq_dev->display_info.bCallBackTaskCreated) {
		if (bPauseStage == FALSE) {
			RES_DISPLAY.u32DelayTime[u32CurReadPointer] =
			    _display_get_time() -
			    RES_DISPLAY.u32FlipTime[u32CurReadPointer];

			if ((RES_DISPLAY.frame_info
				[u32CurReadPointer].enScanType ==
			     MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME)
			    || (RES_DISPLAY.frame_info
			    [u32CurReadPointer].enScanType ==
				MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD)) {
				if (RES_DISPLAY.frame_info
				    [u32CurReadPointer].enFieldOrderType ==
				    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM) {
					report[u32WindowID] = TRUE;

					if (enFieldType ==
					    MTK_PQ_FIELD_TYPE_BOTTOM) {
						delay_time[u32WindowID] =
						    RES_DISPLAY.u32DelayTime
						    [u32CurReadPointer];
						MTK_PQ_GET_SYSTEM_TIME(
						    system_time
						    [u32WindowID]);
					} else if (enFieldType ==
					    MTK_PQ_FIELD_TYPE_TOP) {
						delay_time[u32WindowID] =
						    RES_DISPLAY.u32DelayTime
						    [_display_get_pre_id
						    (u32CurReadPointer)];
					} else {
						report[u32WindowID] = FALSE;
					}
				} else {
					report[u32WindowID] = TRUE;

					if (enFieldType ==
					    MTK_PQ_FIELD_TYPE_TOP) {
						delay_time[u32WindowID] =
						    RES_DISPLAY.u32DelayTime
						    [u32CurReadPointer];
						MTK_PQ_GET_SYSTEM_TIME(
						    system_time
						    [u32WindowID]);
					} else if (enFieldType ==
					    MTK_PQ_FIELD_TYPE_BOTTOM) {
						delay_time[u32WindowID] =
						    RES_DISPLAY.u32DelayTime
						    [_display_get_pre_id
						    (u32CurReadPointer)];
					} else {
						report[u32WindowID] = FALSE;
					}
				}
			} else {
				delay_time[u32WindowID] =
				    RES_DISPLAY.u32DelayTime[u32CurReadPointer];
				MTK_PQ_GET_SYSTEM_TIME(
				    system_time[u32WindowID]);
				report[u32WindowID] = TRUE;
			}

			pts_ms[u32WindowID] = RES_DISPLAY.frame_info
			    [u32CurReadPointer].u64Pts;
			pts_us[u32WindowID] = RES_DISPLAY.frame_info
			    [u32CurReadPointer].u64PtsUs;
			check_frame_disp[u32WindowID] =
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].bCheckFrameDisp;
		} else {
			//Report last states for Pause situation
			report[u32WindowID] = TRUE;
		}

		if (RES_DISPLAY.frame_info
		    [u32CurReadPointer].u32DisplayCounts >=
		    UNDERFLOW_X_REPEAT) {
			display_count[u32WindowID] =
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].u32DisplayCounts;
		} else {
			//For reducing underflow be trigged too sensitive
			display_count[u32WindowID] = 1;
		}

		if (pq_dev->display_info.bCallBackTaskCreated) {
			struct v4l2_event ev;

			memset(&ev, 0, sizeof(struct v4l2_event));
			ev.type = V4L2_EVENT_MTK_PQ_CALLBACK;
			v4l2_event_queue(&pq_dev->video_dev, &ev);
		}
	}
	//calculate video delay time for new api
	if (bPauseStage == FALSE) {
		__u32 u32Time = 0;

		video_delay_time[u32CurReadPointer] = _display_get_time();
		task_cnt += 1;

		if (RES_DISPLAY.abVideoFlip[u32CurReadPointer]) {
			if (video_delay_time[u32CurReadPointer] >=
			    RES_DISPLAY.au32VideoFlipTime[u32CurReadPointer]) {
				u32Time =
				    video_delay_time[u32CurReadPointer] -
				    RES_DISPLAY.au32VideoFlipTime
				    [u32CurReadPointer];
			} else {
				u32Time =
				    SYS_TIME_MAX - RES_DISPLAY.au32VideoFlipTime
				    [u32CurReadPointer] +
				    video_delay_time[u32CurReadPointer];
			}

			RES_DISPLAY.abVideoFlip[u32CurReadPointer] = FALSE;
		} else {
			if ((RES_DISPLAY.frame_info
			    [_display_get_pre_id(u32CurReadPointer)].bValid ==
			    TRUE)
			    && (RES_DISPLAY.frame_info
			    [u32CurReadPointer].bValid == TRUE)) {
				if (video_delay_time[u32CurReadPointer] >=
				    video_delay_time[_display_get_pre_id
							(u32CurReadPointer)]) {
					u32Time =
					    video_delay_time
					    [u32CurReadPointer] -
					    video_delay_time
					    [_display_get_pre_id
					    (u32CurReadPointer)];
				} else {
					u32Time =
					    SYS_TIME_MAX -
					    video_delay_time
					    [_display_get_pre_id
					    (u32CurReadPointer)] +
					    video_delay_time
					    [u32CurReadPointer];
				}
			}
		}

		total_time += u32Time;

		if (task_cnt == 0)
			return;

		RES_DISPLAY.u32VideoDelayTime = total_time / task_cnt;

		if (RES_DISPLAY.u32VideoDelayTime > MTK_PQ_VIDEODELAY_TIMEOUT)
			RES_DISPLAY.u32VideoDelayTime = 0;
	}
}

static bool _display_update_release_count_and_read_pointer(__u32 u32WindowID,
				   bool bPauseStage,
				   __u32 u32CurPointer,
				   __u32 u32ReadingPointer,
				   bool bRepeatLastField,
				   bool bIsRepeatFrame)
{
	__u8 u8Index = 0;

	DISPLAY_GET_RES_PRI;

	if (bIsRepeatFrame == FALSE) {
		if (bPauseStage == FALSE) {
			for (u8Index = 0; u8Index <
			    MTK_PQ_QUEUE_DEPTH; u8Index++) {
				if (RES_DISPLAY.u32ReleaseState[u8Index] >= 1)
					RES_DISPLAY.u32ReleaseState[u8Index]++;
			}

			RES_DISPLAY.u32ReleaseState[u32ReadingPointer]++;
		} else {
			if (RES_DISPLAY.frame_info
			    [u32CurPointer].enScanType !=
			    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
				if ((is_even_field == FALSE) &&
				    (bRepeatLastField == FALSE)) {
					u32ReadingPointer =
					    _display_get_next_id(
					    u32ReadingPointer);
				}

				is_even_field = !is_even_field;
			}
		}

		RES_DISPLAY.buffer_ptr.read_ptr =
		    _display_get_next_id(u32ReadingPointer);
	}
	if (ignore_cur_field == TRUE) {
		RES_DISPLAY.u32ReleaseState[_display_get_pre_id(
		    u32ReadingPointer)] += 2;
		ignore_cur_field = FALSE;
	}

	return TRUE;
}

static bool _display_is_first_field(__u32 u32WindowID, __u32 u32CurReadPointer)
{
	bool bRet = TRUE;
	bool bMvopIsBot = TRUE;
	MVOP_Handle stHd = { E_MVOP_MODULE_MAIN };
	MVOP_Result eMvopRet = E_MVOP_FAIL;

	DISPLAY_GET_RES_PRI;

	if (!_display_is_source_interlace(RES_DISPLAY.frame_info
	    [u32CurReadPointer].enScanType)) {
		bRet = TRUE;
	} else {
		eMvopRet =
		    MDrv_MVOP_GetCommand(&stHd, E_MVOP_CMD_GET_IS_BOT_FIELD,
		    &bMvopIsBot, sizeof(bMvopIsBot));

		if (RES_DISPLAY.frame_info
		    [u32CurReadPointer].enFieldOrderType ==
		    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP) {
			bRet = bMvopIsBot ? TRUE : FALSE;
		} else {
			bRet = bMvopIsBot ? FALSE : TRUE;
		}
	}

	return bRet;
}

static bool _display_is_need_per_frame_update_xc_setting(void)
{
	return RES_COMMON->bPerframeUpdateXCSetting;
}

static bool _display_is_support_dtv_ds(void)
{
	return TRUE;
}

static bool _display_is_dtv_ds_on(enum mtk_pq_input_source_type enInputSrcType)
{
	if (IS_SRC_DTV(enInputSrcType) &&
	    (RES_COMMON->bForceDTVDSOff == FALSE) &&
	    (_display_is_support_dtv_ds() == TRUE)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static void _display_update_user_set_window(__u32 u32WindowID,
			    __u32 u32ReadPointer,
			    bool bPauseState)
{
	DISPLAY_GET_RES_PRI;

	if (((RES_DISPLAY.bSetWinImmediately == TRUE) || (bPauseState == TRUE))
	    && (RES_DISPLAY.bUpdateSetWin == TRUE)) {
		RES_DISPLAY.bUpdateSetWin = FALSE;
		RES_DISPLAY.bRefreshWin = TRUE;

		memcpy(&RES_DISPLAY.stXCCaptWinInfo,
		    &user_capt_win[u32WindowID],
			sizeof(struct mtk_pq_window));
		memcpy(&RES_DISPLAY.stWindowInfo.stSetWinInfo.stDispWin,
		    &user_disp_win[u32WindowID],
			sizeof(struct mtk_pq_window));
		memcpy(&RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin,
		    &user_crop_win[u32WindowID],
			sizeof(struct mtk_pq_window));
	} else if ((bPauseState == FALSE)
	    && (RES_DISPLAY.frame_info
	    [u32ReadPointer].bUserSetwinFlag == TRUE)) {
		memcpy(&RES_DISPLAY.stXCCaptWinInfo, &RES_DISPLAY.frame_info
		    [u32ReadPointer].stUserXCCaptWin,
			sizeof(struct mtk_pq_window));
		memcpy(&RES_DISPLAY.stWindowInfo.stSetWinInfo.stDispWin,
		       &RES_DISPLAY.frame_info
		       [u32ReadPointer].stUserXCDispWin,
		       sizeof(struct mtk_pq_window));
		memcpy(&RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin,
		       &RES_DISPLAY.frame_info
		       [u32ReadPointer].stUserXCCropWin,
		       sizeof(struct mtk_pq_window));

		RES_DISPLAY.frame_info[u32ReadPointer].enUserArc =
		    user_arc[u32WindowID];
		RES_DISPLAY.frame_info[u32ReadPointer].bUserSetwinFlag =
		    FALSE;
		RES_DISPLAY.bRefreshWin = TRUE;
	}

}


static bool _display_check_ds_condition(__u32 u32WindowID,
				__u32 u32ReadPointer,
				__u16 pu16SrcWidth,
				__u16 pu16SrcHeight)
{
	bool bRet = FALSE;

	DISPLAY_GET_RES_PRI;

	if (((pu16SrcWidth != RES_DISPLAY.stXCCaptWinInfo.u32width
	    || pu16SrcHeight != RES_DISPLAY.stXCCaptWinInfo.u32height)
	    || (_display_is_source_interlace(pre_scan_type) !=
	    _display_is_source_interlace(RES_DISPLAY.frame_info
	     [u32ReadPointer].enScanType))) && !RES_DISPLAY.bRefreshWin) {
		bRet = TRUE;
	} else {
		bRet = FALSE;
	}

	return bRet;
}

static void _display_calculate_zoom_crop(struct mtk_pq_window *u32CurrentSize,
					 struct mtk_pq_window *u32FirstCapSize,
					 struct mtk_pq_window *u32FirstCropSize,
					 struct mtk_pq_window *pNewCropWin)
{
	if ((u32CurrentSize == NULL) || (u32FirstCapSize == NULL) ||
	    (u32FirstCropSize == NULL) || (pNewCropWin == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	// Care about u32width & u32height value to avoid divide by zero
	if (u32FirstCapSize->u32width != 0 &&
	    u32FirstCapSize->u32height != 0) {
		pNewCropWin->u32x =
		    ((u32FirstCropSize->u32x * u32CurrentSize->u32width) +
		     (u32FirstCapSize->u32width / 2)) /
		     u32FirstCapSize->u32width;

		pNewCropWin->u32y =
		    ((u32FirstCropSize->u32y * u32CurrentSize->u32height) +
		     (u32FirstCapSize->u32height / 2)) /
		     u32FirstCapSize->u32height;

		pNewCropWin->u32width =
		    ((u32FirstCropSize->u32width * u32CurrentSize->u32width) +
		     (u32FirstCapSize->u32width / 2)) /
		     u32FirstCapSize->u32width;

		pNewCropWin->u32height =
		    ((u32FirstCropSize->u32height * u32CurrentSize->u32height) +
		     (u32FirstCapSize->u32height / 2)) /
		     u32FirstCapSize->u32height;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "cap(%u,%u) is zero, can't calculate crop\n",
		    u32FirstCapSize->u32width, u32FirstCapSize->u32height);
	}
}

static void _display_adjust_window_arc(struct mtk_pq_window *pDispWin,
				       __u32 u32SrcARCWidth,
				       __u32 u32SrcARCHeight)
{
	if (pDispWin == NULL || u32SrcARCWidth == 0 || u32SrcARCHeight == 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}
	__u32 u32Temp;

	u32Temp = (pDispWin->u32height * u32SrcARCWidth) / u32SrcARCHeight;

	if (u32Temp <= pDispWin->u32width) {
		u32Temp = (u32Temp / 2) * 2;
		pDispWin->u32x += (pDispWin->u32width - u32Temp) / 2;
		pDispWin->u32width = u32Temp;
	} else {
		u32Temp = (pDispWin->u32width * u32SrcARCHeight) /
		    u32SrcARCWidth;
		u32Temp = (u32Temp / 2) * 2;
		pDispWin->u32y += (pDispWin->u32height - u32Temp) / 2;
		pDispWin->u32height = u32Temp;
	}
}

static void _display_calculate_aspect_ratio(__u32 u32WindowID,
				    struct mtk_pq_window *pCropWin,
				    struct mtk_pq_window *pDispWin,
				    enum mtk_pq_aspect_ratio enAR,
				    __u32 u32SrcARCWidth, __u32 u32SrcARCHeight)
{
	if ((pDispWin == NULL) || (pCropWin == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	__u32 u32Temp = 0;

	DISPLAY_GET_RES_PRI;

	switch (enAR) {
	case MTK_PQ_AR_DEFAULT:
		break;

	case MTK_PQ_AR_AUTO:
		{
			__u32 u32Ratio = 1000;
			__u32 u32CalWidth = 0;
			__u32 u32CalHeight = 0;

			if ((RES_DISPLAY.u32AspectWidth != 0) &&
			    (RES_DISPLAY.u32AspectHeight != 0)) {
				u32CalWidth = RES_DISPLAY.u32AspectWidth;
				u32CalHeight = RES_DISPLAY.u32AspectHeight;
			} else {
				u32CalWidth = pCropWin->u32width;
				u32CalHeight = pCropWin->u32height;
			}

			if (u32CalHeight == 0)
				break;

			u32Ratio = u32CalWidth * 1000 / u32CalHeight;
			u32Temp = (__u32) pDispWin->u32height * u32Ratio;

			if (u32Temp <= pDispWin->u32width * 1000) {
				u32Temp = (u32Temp / 2) * 2 / 1000;
				pDispWin->u32x +=
				    (pDispWin->u32width - u32Temp) / 2;
				pDispWin->u32width = u32Temp;
			} else {
				if (u32Ratio == 0)
					break;

				u32Temp = (__u32) pDispWin->u32width *
				    1000 * 1000 / u32Ratio;

				u32Temp = (u32Temp / 2) * 2 / 1000;

				pDispWin->u32y += (pDispWin->u32height -
				    u32Temp) / 2;

				pDispWin->u32height = u32Temp;
			}
		}
		break;

	case MTK_PQ_AR_16x9:
		u32Temp = (pDispWin->u32height * 16) / 9;

		if (u32Temp <= pDispWin->u32width) {
			pDispWin->u32x += (pDispWin->u32width - u32Temp) / 2;
			pDispWin->u32width = u32Temp;
		} else {	// H:V <= 16:9
			u32Temp = (pDispWin->u32width * 9) / 16;
			pDispWin->u32y += (pDispWin->u32height - u32Temp) / 2;
			pDispWin->u32height = u32Temp;
		}
		break;

	case MTK_PQ_AR_4x3:
		u32Temp = (pDispWin->u32height * 4) / 3;

		if (u32Temp <= pDispWin->u32width) {
			pDispWin->u32x += (pDispWin->u32width - u32Temp) / 2;
			pDispWin->u32width = u32Temp;
		} else {	// H:V <= 4:3
			u32Temp = (pDispWin->u32width * 3) / 4;
			pDispWin->u32y += (pDispWin->u32height - u32Temp) / 2;
			pDispWin->u32height = u32Temp;
		}
		break;

	case MTK_PQ_AR_14x9:
		u32Temp = ((__u32) pDispWin->u32height * 14) / 9;

		if (u32Temp <= pDispWin->u32width) {	// H:V >= 4:3
			pDispWin->u32x += (pDispWin->u32width - u32Temp) / 2;
			pDispWin->u32width = u32Temp;
		} else {	// H:V <= 14:9
			u32Temp = (pDispWin->u32width * 9) / 14;
			pDispWin->u32y += (pDispWin->u32height - u32Temp) / 2;
			pDispWin->u32height = u32Temp;
		}
		break;

	case MTK_PQ_AR_Zoom1:
		{
			__u32 u32Temp, u32Temp1, u32Temp2, u32Temp3;

			u32Temp = ((pCropWin->u32width * 50) / 1000);
			u32Temp1 = ((pCropWin->u32height * 60) / 1000);
			u32Temp &= ~0x1;
			u32Temp1 &= ~0x1;
			u32Temp2 = ((pCropWin->u32width * 50) / 1000);
			u32Temp3 = ((pCropWin->u32height * 60) / 1000);
			u32Temp2 &= ~0x1;
			u32Temp3 &= ~0x1;

			pCropWin->u32width = pCropWin->u32width -
			    (u32Temp + u32Temp2);
			pCropWin->u32height = pCropWin->u32height -
			    (u32Temp1 + u32Temp3);

			pCropWin->u32x = pCropWin->u32x + u32Temp;
			pCropWin->u32y = pCropWin->u32y + u32Temp1;
		}
		break;

	case MTK_PQ_AR_Zoom2:
		{
			__u32 u32Temp, u32Temp1, u32Temp2, u32Temp3;

			u32Temp = ((pCropWin->u32width * 100) / 1000);
			u32Temp1 = ((pCropWin->u32height * 120) / 1000);
			u32Temp &= ~0x1;
			u32Temp1 &= ~0x1;
			u32Temp2 = ((pCropWin->u32width * 100) / 1000);
			u32Temp3 = ((pCropWin->u32height * 120) / 1000);
			u32Temp2 &= ~0x1;
			u32Temp3 &= ~0x1;

			pCropWin->u32width = pCropWin->u32width -
			    (u32Temp + u32Temp2);
			pCropWin->u32height = pCropWin->u32height -
			    (u32Temp1 + u32Temp3);

			pCropWin->u32x = pCropWin->u32x + u32Temp;
			pCropWin->u32y = pCropWin->u32y + u32Temp1;
		}
		break;

	case MTK_PQ_AR_JustScan:
		if (pCropWin->u32width <= pDispWin->u32width
		    && pCropWin->u32height <= pDispWin->u32height) {

			pDispWin->u32x += (pDispWin->u32width -
			    pCropWin->u32width) / 2;

			pDispWin->u32width = pCropWin->u32width;

			pDispWin->u32y += (pDispWin->u32height -
			    pCropWin->u32height) / 2;

			pDispWin->u32height = pCropWin->u32height;
		}
		break;

	case MTK_PQ_AR_Panorama:

	case MTK_PQ_AR_DotByDot:
		break;

	case MTK_PQ_AR_Subtitle:
		u32Temp = (pDispWin->u32height * 2) / 25;
		pCropWin->u32y += u32Temp;
		pCropWin->u32height = pCropWin->u32height - u32Temp;
		break;

	case MTK_PQ_AR_Movie:
		u32Temp = (pDispWin->u32height * 2) / 25;
		pCropWin->u32y += u32Temp;
		pCropWin->u32height = pCropWin->u32height - 2 * u32Temp;
		break;

	case MTK_PQ_AR_Personal:
		u32Temp = (pDispWin->u32height * 3) / 50;
		pCropWin->u32y += u32Temp;
		pCropWin->u32height = pCropWin->u32height - 2 * u32Temp;
		break;

	case MTK_PQ_AR_4x3_LetterBox:
		u32Temp = (pCropWin->u32width * 3) / 4;

		if (u32Temp > pCropWin->u32height) {
			_display_adjust_window_arc(pDispWin,
			    u32SrcARCWidth, u32SrcARCHeight);
		} else {
			pCropWin->u32y += (pCropWin->u32height - u32Temp) / 2;
			pCropWin->u32height = u32Temp;
		}
		break;

	case MTK_PQ_AR_4x3_PanScan:
		u32Temp = (pCropWin->u32height * 4) / 3;

		if (u32Temp > pCropWin->u32width) {
			_display_adjust_window_arc(pDispWin,
			    u32SrcARCWidth, u32SrcARCHeight);
		} else {
			pCropWin->u32x += (pCropWin->u32width - u32Temp) / 2;
			pCropWin->u32width = u32Temp;
		}
		break;

	case MTK_PQ_AR_16x9_PillarBox:
		u32Temp = (pCropWin->u32height * 16) / 9;

		if (u32Temp > pCropWin->u32width) {
			_display_adjust_window_arc(pDispWin,
			    u32SrcARCWidth, u32SrcARCHeight);
		} else {
			pCropWin->u32x += (pCropWin->u32width - u32Temp) / 2;
			pCropWin->u32width = u32Temp;
		}
		break;

	case MTK_PQ_AR_16x9_PanScan:
		u32Temp = (pCropWin->u32width * 9) / 16;

		if (u32Temp > pCropWin->u32height) {
			_display_adjust_window_arc(pDispWin,
			    u32SrcARCWidth, u32SrcARCHeight);
		} else {
			pCropWin->u32y += (pCropWin->u32height - u32Temp) / 2;
			pCropWin->u32height = u32Temp;
		}
		break;

	default:
		_display_adjust_window_arc(pDispWin,
		    u32SrcARCWidth, u32SrcARCHeight);
		break;
	}
}

static void _display_regen_crop_window(__u32 u32WindowID,
				    __u32 u32ReadPointer, bool bPauseState,
				    __u16 *pu16SrcWidth, __u16 *pu16SrcHeight)
{
	if ((pu16SrcWidth == NULL) || (pu16SrcHeight == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	DISPLAY_GET_RES_PRI;
	struct mtk_pq_set_win_info *pstSetWinInfo =
	    &(RES_DISPLAY.stWindowInfo.stSetWinInfo);

	if (bPauseState != TRUE) {
		if (_display_check_ds_condition
		    (u32WindowID, u32ReadPointer,
		    *pu16SrcWidth, *pu16SrcHeight)) {
			RES_DISPLAY.stXCCaptWinInfo.u32width = *pu16SrcWidth;
			RES_DISPLAY.stXCCaptWinInfo.u32height = *pu16SrcHeight;
			pre_scan_type = RES_DISPLAY.frame_info
			    [u32ReadPointer].enScanType;

			memcpy(&pstSetWinInfo->stCropWin,
			    &RES_DISPLAY.stXCCaptWinInfo,
				sizeof(struct mtk_pq_window));
			memcpy(&pstSetWinInfo->stDispWin,
			    &RES_DISPLAY.frame_info
			    [u32ReadPointer].stUserXCDispWin,
			    sizeof(struct mtk_pq_window));

			_display_calculate_zoom_crop(
			     &RES_DISPLAY.stXCCaptWinInfo,
			     &RES_DISPLAY.frame_info
			     [u32ReadPointer].stUserXCCaptWin,
			     &RES_DISPLAY.frame_info
			     [u32ReadPointer].stUserXCCropWin,
			     &pstSetWinInfo->stCropWin);

			_display_calculate_aspect_ratio(u32WindowID,
			    &pstSetWinInfo->stCropWin,
			    &pstSetWinInfo->stDispWin,
			    pstSetWinInfo->enARC, 0, 0);

			*pu16SrcWidth =
			pstSetWinInfo->stCropWin.u32width;

			*pu16SrcHeight =
			pstSetWinInfo->stCropWin.u32height;

			RES_DISPLAY.bRefreshWin = TRUE;
		} else {
			if (RES_DISPLAY.bSetWinImmediately == TRUE) {
				RES_DISPLAY.bSetWinImmediately = FALSE;
				RES_DISPLAY.stXCCaptWinInfo.u32width =
				    *pu16SrcWidth;

				RES_DISPLAY.stXCCaptWinInfo.u32height =
				    *pu16SrcHeight;

				_display_calculate_zoom_crop(
				    &RES_DISPLAY.stXCCaptWinInfo,
				    &user_capt_win[u32WindowID],
				    &user_crop_win[u32WindowID],
				    &pstSetWinInfo->stCropWin);

				_display_calculate_aspect_ratio(u32WindowID,
				   &pstSetWinInfo->stCropWin,
				   &pstSetWinInfo->stDispWin,
				    user_arc[u32WindowID], 0, 0);

				*pu16SrcWidth =
				    pstSetWinInfo->stCropWin.u32width;
				*pu16SrcHeight =
				    pstSetWinInfo->stCropWin.u32height;
			} else {
				RES_DISPLAY.stXCCaptWinInfo.u32width =
				    *pu16SrcWidth;
				RES_DISPLAY.stXCCaptWinInfo.u32height =
				    *pu16SrcHeight;

				_display_calculate_zoom_crop(
				    &RES_DISPLAY.stXCCaptWinInfo,
				    &RES_DISPLAY.frame_info
				    [u32ReadPointer].stUserXCCaptWin,
				    &RES_DISPLAY.frame_info
				    [u32ReadPointer].stUserXCCropWin,
				    &pstSetWinInfo->stCropWin);

				_display_calculate_aspect_ratio(u32WindowID,
				    &pstSetWinInfo->stCropWin,
				    &pstSetWinInfo->stDispWin,
				    pstSetWinInfo->enARC, 0, 0);

				*pu16SrcWidth =
				    pstSetWinInfo->stCropWin.u32width;
				*pu16SrcHeight =
				    pstSetWinInfo->stCropWin.u32height;
			}
		}
	} else {
		RES_DISPLAY.stXCCaptWinInfo.u32width = *pu16SrcWidth;
		RES_DISPLAY.stXCCaptWinInfo.u32height = *pu16SrcHeight;

		_display_calculate_zoom_crop(&RES_DISPLAY.stXCCaptWinInfo,
					     &user_capt_win[u32WindowID],
					     &user_crop_win[u32WindowID],
					     &pstSetWinInfo->stCropWin);

		_display_calculate_aspect_ratio(u32WindowID,
						&pstSetWinInfo->stCropWin,
						&pstSetWinInfo->stDispWin,
						user_arc[u32WindowID], 0, 0);

		*pu16SrcWidth = pstSetWinInfo->stCropWin.u32width;
		*pu16SrcHeight = pstSetWinInfo->stCropWin.u32height;
	}
}

static bool _display_get_mvop_crop_info(__u32 u32WindowID,
			__u32 u32CurReadPointer,
			MVOP_DMSCropINFO *pstMVOPCropInfo)
{
	if (pstMVOPCropInfo == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	struct mtk_pq_vdec_frame_info *pstFrmInfo =
	    &(RES_DISPLAY.frame_info[u32CurReadPointer]);

	struct mtk_pq_window *pstCropWin =
	    &(RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin);

	pstMVOPCropInfo->u32ApiDMSCrop_Version = API_MVOP_DMS_CROP_VERSION;
	pstMVOPCropInfo->u16ApiDMSCrop_Length = sizeof(MVOP_DMSCropINFO);

	if (_display_is_scaling_use_second_buffer(
	    RES_DISPLAY.scaling_condition)) {
		if (RES_DISPLAY.bIsXCEnFBL) {
			pstMVOPCropInfo->stCropStart.u16Xpos =
			    pstCropWin->u32x +
			    (pstFrmInfo->u16CropLeft / 2);
			pstMVOPCropInfo->stCropStart.u16Ypos =
			    pstCropWin->u32y +
			    (pstFrmInfo->u16CropTop / 2);
			pstMVOPCropInfo->stCropSize.u16Width =
			    pstFrmInfo->u16Src2ndBufferWidth;
			pstMVOPCropInfo->stCropSize.u16Height =
			    pstFrmInfo->u16Src2ndBufferHeight;
		} else {
			pstMVOPCropInfo->stCropStart.u16Xpos =
			    (pstFrmInfo->u16CropLeft / 2);
			pstMVOPCropInfo->stCropStart.u16Ypos =
			    (pstFrmInfo->u16CropTop / 2);
			pstMVOPCropInfo->stCropSize.u16Width =
			    pstFrmInfo->u16Src2ndBufferWidth;
			pstMVOPCropInfo->stCropSize.u16Height =
			    pstFrmInfo->u16Src2ndBufferHeight;
		}
	} else {
		//if XC is FBL mode, need to do crop by MVOP.
		if (RES_DISPLAY.bIsXCEnFBL) {
			pstMVOPCropInfo->stCropStart.u16Xpos =
			    pstCropWin->u32x +
			    pstFrmInfo->u16CropLeft;
			pstMVOPCropInfo->stCropStart.u16Ypos =
			    pstCropWin->u32y +
			    pstFrmInfo->u16CropTop;
			pstMVOPCropInfo->stCropSize.u16Width =
			    pstCropWin->u32width;

			if (RES_DISPLAY.enMVOPDropline ==
			    MTK_PQ_MVOP_DROP_LINE_DISABLE) {
				pstMVOPCropInfo->stCropSize.u16Height =
				    pstCropWin->u32height;
			} else {
				pstMVOPCropInfo->stCropSize.u16Height =
				    pstFrmInfo->u16SrcHeight;
			}
		} else {
			pstMVOPCropInfo->stCropStart.u16Xpos =
			    pstFrmInfo->u16CropLeft;
			pstMVOPCropInfo->stCropStart.u16Ypos =
			    pstFrmInfo->u16CropTop;
			pstMVOPCropInfo->stCropSize.u16Width =
			    pstFrmInfo->u16SrcWidth -
			    pstFrmInfo->u16CropLeft -
			    pstFrmInfo->u16CropRight;
			pstMVOPCropInfo->stCropSize.u16Height =
			    pstFrmInfo->u16SrcHeight -
			    pstFrmInfo->u16CropTop -
			    pstFrmInfo->u16CropBottom;
		}
	}

	return TRUE;
}

static void _display_update_mvop_crop(__u32 u32WindowID,
				__u32 u32CurReadPointer)
{
	MVOP_DMSCropINFO stMVOPCropInfo;
	MVOP_Handle stHd = { E_MVOP_MODULE_MAIN };
	MVOP_HSMode enHskMode = E_MVOP_HS_DISABLE;
	MVOP_Result enRet = E_MVOP_FAIL;

	DISPLAY_GET_RES_PRI;

	memset(&stMVOPCropInfo, 0, sizeof(MVOP_DMSCropINFO));

	MDrv_MVOP_GetCommand(&stHd, E_MVOP_CMD_GET_HANDSHAKE_MODE,
	    &enHskMode, sizeof(enHskMode));

	if ((enHskMode == E_MVOP_HS_ENABLE) || (RES_DISPLAY.bIsXCEnFBL)) {
		_display_get_mvop_crop_info(u32WindowID,
		    u32CurReadPointer, &stMVOPCropInfo);

		enRet = MDrv_MVOP_DMS_SetCropInfo(&stHd, E_MVOP_MAIN_WIN,
		    &stMVOPCropInfo, NULL);

		if (enRet != E_MVOP_OK) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			    "MDrv_MVOP_DMS_SetCropInfo failed! ret = %d\n",
			    enRet);
		}
	}
}

static void _display_set_fast_forward_status(__u32 u32WindowID,
				__u32 u32CurReadPointer)
{
	DISPLAY_GET_RES_PRI;

	switch (RES_DISPLAY.fast_forward_status) {
	case MTK_PQ_NORMAL:
		if (RES_DISPLAY.frame_info
		    [u32CurReadPointer].u8FieldCtrl == TRUE)
			RES_DISPLAY.fast_forward_status = MTK_PQ_PQDS_LOADZERO;
		break;

	case MTK_PQ_PQDS_LOADZERO:
		RES_DISPLAY.fast_forward_status = MTK_PQ_PQDS_LOADZERO_DONE;
		RES_DISPLAY.u32PQFrameCnt = MAX_PQDS_FRAMECOUNT;
		break;

	case MTK_PQ_PQDS_LOADZERO_DONE:
		if (RES_DISPLAY.frame_info
		    [u32CurReadPointer].u8FieldCtrl == FALSE)
			RES_DISPLAY.fast_forward_status = MTK_PQ_RESUME;
		break;

	case MTK_PQ_RESUME:
		RES_DISPLAY.fast_forward_status = MTK_PQ_NORMAL;
		break;

	default:
		break;
	}
}

static EN_XC_HDR_TYPE _display_get_hdr_type(__u32 ext_avail)
{
	EN_XC_HDR_TYPE enRetHdrType = E_XC_HDR_TYPE_MAX;

	if ((ext_avail & MTK_PQ_HDR_METADATA_DOLBY_HDR) != 0)
		enRetHdrType = E_XC_HDR_TYPE_DOLBY;
	else if ((ext_avail & MTK_PQ_HDR_METADATA_TCH) != 0)
		enRetHdrType = E_XC_HDR_TYPE_TECHNICOLOR;
	else if ((ext_avail & MTK_PQ_HDR_METADATA_HDR10_PER_FRAME) != 0)
		enRetHdrType = E_XC_HDR_TYPE_HDR10_PLUS;
	else
		enRetHdrType = E_XC_HDR_TYPE_NONE;

	return enRetHdrType;
}

static bool _display_pq_ds_is_get_pq_setting(__u32 u32WindowID)
{
	DISPLAY_GET_RES_PRI;

	if ((RES_DISPLAY.bRefreshWin == TRUE)
	    || (RES_DISPLAY.fast_forward_status == MTK_PQ_PQDS_LOADZERO)
	    || (RES_DISPLAY.fast_forward_status == MTK_PQ_RESUME)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static bool _display_pq_ds_control(enum mtk_pq_pqds_action action)
{
	__u32 u32WindowID = MVOP_WINDOW;
	__u32 u32PQFrmCntTB[E_DS_XRULE_FRAME_COUNT_MAX] = { 0 };
	__u32 u32PQFrmCntIdx = 0;
	bool bRet = FALSE;
	XC_GET_DS_INFO stGetDSInfo;
	__u32 u32TBidx = 0;

	DISPLAY_GET_RES_PRI;

	if (RES_COMMON->bForceOffPQDS)
		return FALSE;

	switch (action) {
	case MTK_PQ_PQDS_GET_PQSETTING:
		memset(&stGetDSInfo, 0, sizeof(XC_GET_DS_INFO));
		stGetDSInfo.u32ApiGetDSInfo_Version = API_GET_XCDS_INFO_VERSION;
		stGetDSInfo.u16ApiGetDSInfo_Length = sizeof(XC_GET_DS_INFO);

		MApi_XC_GetDSInfo(&stGetDSInfo,
		    sizeof(XC_GET_DS_INFO), MVOP_WINDOW);

		for (u32TBidx = 0; u32TBidx <
		    E_DS_XRULE_FRAME_COUNT_MAX; u32TBidx++)
			u32PQFrmCntTB[u32TBidx] =
			    stGetDSInfo.u8DS_XRule_FrameCount[u32TBidx];

		u32PQFrmCntIdx = 0;
		RES_DISPLAY.u32PQFrameCnt = 0;
		bRet = TRUE;
		break;

	case MTK_PQ_PQDS_CHECKSTATUS:
		if (u32PQFrmCntTB[u32PQFrmCntIdx] == RES_DISPLAY.u32PQFrameCnt)
			bRet = TRUE;
		else
			bRet = FALSE;

		break;

	case MTK_PQ_PQDS_ENABLE_CHECK:
		if ((RES_DISPLAY.u32PQFrameCnt == u32PQFrmCntTB[u32PQFrmCntIdx])
		    && (RES_DISPLAY.u32PQFrameCnt <
		    u32PQFrmCntTB[u32PQFrmCntIdx + 1])) {
			u32PQFrmCntIdx++;
		}

		if (RES_DISPLAY.u32PQFrameCnt > u32PQFrmCntTB[u32PQFrmCntIdx])
			RES_DISPLAY.bPQDSEnable = TRUE;

		bRet = TRUE;
		break;

	case MTK_PQ_PQDS_UPDATEINDEX:
		if (u32PQFrmCntTB[u32PQFrmCntIdx] == RES_DISPLAY.u32PQFrameCnt)
			u32PQFrmCntIdx++;

		bRet = TRUE;
		break;

	default:
		break;
	}

	return bRet;
}

static void _display_update_setting(__u32 u32WindowID,
			__u32 u32CurReadPointer, bool bPauseState)
{
	bool bUpdate = FALSE;

	if (MApi_XC_SetDSInfo) {
		XC_DS_INFO stXCDSInfo;

		DISPLAY_GET_RES_PRI;

		struct mtk_pq_vdec_frame_info *pstFrmInfo =
		    &(RES_DISPLAY.frame_info[u32CurReadPointer]);

		memset(&stXCDSInfo, 0, sizeof(XC_DS_INFO));
		stXCDSInfo.u32ApiDSInfo_Version = API_XCDS_INFO_VERSION;
		stXCDSInfo.u16ApiDSInfo_Length = sizeof(XC_DS_INFO);
		stXCDSInfo.bUpdate_DS_CMD[MVOP_WINDOW] = TRUE;
		stXCDSInfo.bDynamicScalingEnable[MVOP_WINDOW] = TRUE;
		stXCDSInfo.bRepeatWindow[MVOP_WINDOW] = FALSE;
		stXCDSInfo.bEnable_ForceP[MVOP_WINDOW] =
			RES_DISPLAY.bIsEnForceP;
		stXCDSInfo.stHDRInfo.u8CurrentIndex = 0xFF;
		stXCDSInfo.enHDRType =
		    _display_get_hdr_type(
		    pstFrmInfo->stHDRInfo.u32FrmInfoExtAvail);
		stXCDSInfo.u32XCDSUpdateCondition = 0;
		stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] = E_PQ_DS_TYPE_MAX;

		if ((IS_HDR_PER_FRAME
		     (pstFrmInfo->stHDRInfo.u32FrmInfoExtAvail))) {
			stXCDSInfo.stHDRInfo.u32FrmInfoExtAvail =
			    pstFrmInfo->stHDRInfo.u32FrmInfoExtAvail;

			XC_DS_ColorDescription *pstdstColorDesc =
			    &(stXCDSInfo.stHDRInfo.stColorDescription);

			struct mtk_pq_color_description *pstsrcColorDesc =
			    &(pstFrmInfo->stHDRInfo.stColorDescription);

			pstdstColorDesc->u8ColorPrimaries =
			    pstsrcColorDesc->u8ColorPrimaries;
			pstdstColorDesc->u8MatrixCoefficients =
			    pstsrcColorDesc->u8MatrixCoefficients;
			pstdstColorDesc->u8TransferCharacteristics =
			    pstsrcColorDesc->u8TransferCharacteristics;

			XC_DS_MasterColorDisplay *pstdstMColorDisp =
			    &(stXCDSInfo.stHDRInfo.stMasterColorDisplay);

			struct mtk_pq_master_color_display *pstsrcMColorDisp =
			    &(pstFrmInfo->stHDRInfo.stMasterColorDisplay);

			pstdstMColorDisp->u32MaxLuminance =
			    pstsrcMColorDisp->u32MaxLuminance;

			pstdstMColorDisp->u32MinLuminance =
			    pstsrcMColorDisp->u32MinLuminance;

			pstdstMColorDisp->u16DisplayPrimaries[0][0] =
			    pstsrcMColorDisp->u16DisplayPrimaries[0][0];

			pstdstMColorDisp->u16DisplayPrimaries[0][1] =
			    pstsrcMColorDisp->u16DisplayPrimaries[0][1];

			pstdstMColorDisp->u16DisplayPrimaries[1][0] =
			    pstsrcMColorDisp->u16DisplayPrimaries[1][0];

			pstdstMColorDisp->u16DisplayPrimaries[1][1] =
			    pstsrcMColorDisp->u16DisplayPrimaries[1][1];

			pstdstMColorDisp->u16DisplayPrimaries[2][0] =
			    pstsrcMColorDisp->u16DisplayPrimaries[2][0];

			pstdstMColorDisp->u16DisplayPrimaries[2][1] =
			    pstsrcMColorDisp->u16DisplayPrimaries[2][1];

			pstdstMColorDisp->u16WhitePoint[0] =
			    pstsrcMColorDisp->u16WhitePoint[0];

			pstdstMColorDisp->u16WhitePoint[1] =
			    pstsrcMColorDisp->u16WhitePoint[1];

			stXCDSInfo.stHDRInfo.u8CurrentIndex =
			    pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u8CurrentIndex;
			stXCDSInfo.stHDRInfo.phyRegAddr =
			    pstFrmInfo->stHDRInfo.stDolbyHDRInfo.phyHDRRegAddr;
			stXCDSInfo.stHDRInfo.u32RegSize =
			    pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u32HDRRegSize;
			stXCDSInfo.stHDRInfo.phyLutAddr =
			    pstFrmInfo->stHDRInfo.stDolbyHDRInfo.phyHDRLutAddr;
			stXCDSInfo.stHDRInfo.u32LutSize =
			    pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u32HDRLutSize;
			stXCDSInfo.stHDRInfo.bDMEnable =
			    pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u8DMEnable;
			stXCDSInfo.stHDRInfo.bCompEnable =
			    pstFrmInfo->stHDRInfo.stDolbyHDRInfo.u8CompEnable;
			bUpdate = TRUE;
		}

		if (_display_pq_ds_is_get_pq_setting(u32WindowID) == TRUE)
			_display_pq_ds_control(MTK_PQ_PQDS_GET_PQSETTING);

		if (support_SPFLB) {
			if (RES_DISPLAY.bRefreshWin == TRUE) {
				stXCDSInfo.u32XCDSUpdateCondition |=
				    XC_DS_UPDATE_CONDITION_SPF;
				RES_DISPLAY.u32XCDSUpdateConditionPre |=
				    XC_DS_UPDATE_CONDITION_SPF;
				bUpdate = TRUE;
			}

			if (stXCDSInfo.enHDRType != pre_HDR_type) {
				stXCDSInfo.u32XCDSUpdateCondition |=
				    XC_DS_UPDATE_CONDITION_SPF;
				RES_DISPLAY.u32XCDSUpdateConditionPre |=
				    XC_DS_UPDATE_CONDITION_SPF;
				bUpdate = TRUE;
			}
		}

		if (_display_pq_ds_control(MTK_PQ_PQDS_CHECKSTATUS)
		    && (RES_DISPLAY.bPQDSEnable == TRUE)
		    && _display_is_first_field(u32WindowID,
		    u32CurReadPointer)) {
			stXCDSInfo.u8FrameCountPQDS[MVOP_WINDOW] =
			    RES_DISPLAY.u32PQFrameCnt;
			stXCDSInfo.bUpdate_PQDS_CMD[MVOP_WINDOW] = TRUE;
			stXCDSInfo.bInterlace[MVOP_WINDOW] =
			    _display_is_source_interlace(
			    pstFrmInfo->enScanType);
			stXCDSInfo.stCapWin[MVOP_WINDOW].x =
			    RES_DISPLAY.stXCCaptWinInfo.u32x;
			stXCDSInfo.stCapWin[MVOP_WINDOW].y =
			    RES_DISPLAY.stXCCaptWinInfo.u32y;
			stXCDSInfo.stCapWin[MVOP_WINDOW].width =
			    RES_DISPLAY.stXCCaptWinInfo.u32width;
			stXCDSInfo.stCapWin[MVOP_WINDOW].height =
			    RES_DISPLAY.stXCCaptWinInfo.u32height;

			if (RES_DISPLAY.u32PQFrameCnt == 0)
				stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] =
				    E_PQ_DS_GRULE_XRULE;
			else
				stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] =
				    E_PQ_DS_XRULE;

			if ((RES_DISPLAY.bRefreshWin == TRUE)
			    && (RES_DISPLAY.fast_forward_status ==
			    MTK_PQ_PQDS_LOADZERO_DONE)) {
				RES_DISPLAY.u32PQFrameCnt = MAX_PQDS_FRAMECOUNT;
			}

			stXCDSInfo.u32XCDSUpdateCondition |=
			    XC_DS_UPDATE_CONDITION_RESOLUTION_CHANGE;
			bUpdate = TRUE;

			RES_DISPLAY.u32PQFrameCntPre =
			    stXCDSInfo.u8FrameCountPQDS[MVOP_WINDOW];
			RES_DISPLAY.enPQDSTypePre =
			    stXCDSInfo.enPQDSTYPE[MVOP_WINDOW];
			RES_DISPLAY.u32XCDSUpdateConditionPre |=
			    stXCDSInfo.u32XCDSUpdateCondition;
		} else if (_display_pq_ds_control(MTK_PQ_PQDS_CHECKSTATUS)
			   && (RES_DISPLAY.bPQDSEnable == FALSE)
			   && _display_is_first_field(u32WindowID,
			   u32CurReadPointer)) {
			stXCDSInfo.bInterlace[MVOP_WINDOW] =
			    _display_is_source_interlace(
			    pstFrmInfo->enScanType);
			stXCDSInfo.stCapWin[MVOP_WINDOW].x =
			    RES_DISPLAY.stXCCaptWinInfo.u32x;
			stXCDSInfo.stCapWin[MVOP_WINDOW].y =
			    RES_DISPLAY.stXCCaptWinInfo.u32y;
			stXCDSInfo.stCapWin[MVOP_WINDOW].width =
			    RES_DISPLAY.stXCCaptWinInfo.u32width;
			stXCDSInfo.stCapWin[MVOP_WINDOW].height =
			    RES_DISPLAY.stXCCaptWinInfo.u32height;
			stXCDSInfo.u8FrameCountPQDS[MVOP_WINDOW] = 0;
			stXCDSInfo.bUpdate_PQDS_CMD[MVOP_WINDOW] = FALSE;

			if (RES_DISPLAY.u32PQFrameCnt == 0) {
				stXCDSInfo.bUpdate_PQDS_CMD[MVOP_WINDOW] = TRUE;
				stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] =
				    E_PQ_DS_GRULE;
				bUpdate = TRUE;
			}

			RES_DISPLAY.u32PQFrameCntPre =
			    stXCDSInfo.u8FrameCountPQDS[MVOP_WINDOW];
			RES_DISPLAY.enPQDSTypePre =
			    stXCDSInfo.enPQDSTYPE[MVOP_WINDOW];
			RES_DISPLAY.u32XCDSUpdateConditionPre = 0;
		} else if (_display_is_need_per_frame_update_xc_setting()
			   && (RES_DISPLAY.u32PQFrameCntPre <
			   MAX_PQDS_FRAMECOUNT)) {
			stXCDSInfo.u8FrameCountPQDS[MVOP_WINDOW] =
			    RES_DISPLAY.u32PQFrameCntPre;
			stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] =
			    RES_DISPLAY.enPQDSTypePre;
			stXCDSInfo.u32XCDSUpdateCondition |=
			    RES_DISPLAY.u32XCDSUpdateConditionPre;
			stXCDSInfo.bUpdate_PQDS_CMD[MVOP_WINDOW] = TRUE;
			stXCDSInfo.bInterlace[MVOP_WINDOW] =
			    _display_is_source_interlace(
			    pstFrmInfo->enScanType);
			stXCDSInfo.stCapWin[MVOP_WINDOW].x =
			    RES_DISPLAY.stXCCaptWinInfo.u32x;
			stXCDSInfo.stCapWin[MVOP_WINDOW].y =
			    RES_DISPLAY.stXCCaptWinInfo.u32y;
			stXCDSInfo.stCapWin[MVOP_WINDOW].width =
			    RES_DISPLAY.stXCCaptWinInfo.u32width;
			stXCDSInfo.stCapWin[MVOP_WINDOW].height =
			    RES_DISPLAY.stXCCaptWinInfo.u32height;
			bUpdate = TRUE;
		} else {
			stXCDSInfo.u8FrameCountPQDS[MVOP_WINDOW] = 0;
			stXCDSInfo.bUpdate_PQDS_CMD[MVOP_WINDOW] = FALSE;
			RES_DISPLAY.u32PQFrameCntPre =
			    stXCDSInfo.u8FrameCountPQDS[MVOP_WINDOW];
			RES_DISPLAY.enPQDSTypePre =
			    stXCDSInfo.enPQDSTYPE[MVOP_WINDOW];
			RES_DISPLAY.u32XCDSUpdateConditionPre =
			    stXCDSInfo.u32XCDSUpdateCondition;
		}

		if (_display_is_need_per_frame_update_xc_setting()
		    && !RES_DISPLAY.bRefreshWin) {
			stXCDSInfo.bRepeatWindow[MVOP_WINDOW] = TRUE;
			bUpdate = TRUE;
		}
		//Get SetWindow Or not
		stXCDSInfo.bIsNeedSetWindow = RES_DISPLAY.bRefreshWin;
		stXCDSInfo.u32DSBufferSize =
		    RES_DISPLAY.stDSInfo.u32DynScalingSize;

		if (support_SPFLB) {
			if ((stXCDSInfo.u32XCDSUpdateCondition &
			    XC_DS_UPDATE_CONDITION_SPF) == TRUE) {
				stXCDSInfo.bUpdate_PQDS_CMD[MVOP_WINDOW] =
				    TRUE;
				stXCDSInfo.bInterlace[MVOP_WINDOW] =
				    _display_is_source_interlace(
				    pstFrmInfo->enScanType);
				stXCDSInfo.stCapWin[MVOP_WINDOW].x =
				    RES_DISPLAY.stXCCaptWinInfo.u32x;
				stXCDSInfo.stCapWin[MVOP_WINDOW].y =
				    RES_DISPLAY.stXCCaptWinInfo.u32y;
				stXCDSInfo.stCapWin[MVOP_WINDOW].width =
				    RES_DISPLAY.stXCCaptWinInfo.u32width;
				stXCDSInfo.stCapWin[MVOP_WINDOW].height =
				    RES_DISPLAY.stXCCaptWinInfo.u32height;

				//any rule need OR Xrule.
				if (stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] !=
				    E_PQ_DS_XRULE)
					stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] =
					    E_PQ_DS_GRULE_XRULE;
				else
					stXCDSInfo.enPQDSTYPE[MVOP_WINDOW] =
					    E_PQ_DS_XRULE;

				RES_DISPLAY.enPQDSTypePre =
				    stXCDSInfo.enPQDSTYPE[MVOP_WINDOW];
			}
		}

		if (bUpdate == TRUE)
			MApi_XC_SetDSInfo(&stXCDSInfo,
			    sizeof(XC_DS_INFO), MVOP_WINDOW);

		//record previous
		pre_HDR_type = stXCDSInfo.enHDRType;
	}
}

static void _display_trans_pqwin_to_xcwin(MS_WINDOW_TYPE *pstXCWindow,
				      struct mtk_pq_window *pq_win)
{
	if ((pstXCWindow == NULL) || (pq_win == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	pstXCWindow->x = (pq_win->u32x);
	pstXCWindow->y = (pq_win->u32y);
	pstXCWindow->width = (pq_win->u32width);
	pstXCWindow->height = (pq_win->u32height);
}

static void _display_calculate_mirror(MS_WINDOW_TYPE *pstDispWin,
				      MS_WINDOW_TYPE *pstCropWin,
				      __u16 u16SrcWidth, __u16 u16SrcHeight)
{
	if ((pstDispWin == NULL) || (pstCropWin == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	MVOP_Handle stHdl = { E_MVOP_MODULE_MAIN };
	MVOP_DrvMirror enMirror = E_VOPMIRROR_NONE;

	MDrv_MVOP_GetCommand(&stHdl, E_MVOP_CMD_GET_MIRROR_MODE,
	    &enMirror, sizeof(MVOP_DrvMirror));
//fix me ---sid
//Calculate Disp. window
//if ((enMirror == E_VOPMIRROR_HVBOTH) ||
//    (enMirror == E_VOPMIRROR_HORIZONTALL)) {
//	if (g_IPanel.Width() > (pstDispWin->x + pstDispWin->width))
//		pstDispWin->x = g_IPanel.Width() -
//		    (pstDispWin->x + pstDispWin->width);
//		else
//			pstDispWin->x = 0;
//	}

//	if ((enMirror == E_VOPMIRROR_HVBOTH) ||
//	    (enMirror == E_VOPMIRROR_VERTICAL)) {
//		if (g_IPanel.Height() > (pstDispWin->y + pstDispWin->height))
//			pstDispWin->y = g_IPanel.Height() -
//			    (pstDispWin->y + pstDispWin->height);
//		else
//			pstDispWin->y = 0;
//	}

	if ((enMirror == E_VOPMIRROR_HVBOTH)
	    || (enMirror == E_VOPMIRROR_HORIZONTALL)) {
		if (u16SrcWidth > (pstCropWin->x + pstCropWin->width))
			pstCropWin->x = u16SrcWidth -
			    (pstCropWin->x + pstCropWin->width);
		else
			pstCropWin->x = 0;
	}

	if ((enMirror == E_VOPMIRROR_HVBOTH)
	    || (enMirror == E_VOPMIRROR_VERTICAL)) {
		if (u16SrcHeight > (pstCropWin->y + pstCropWin->height))
			pstCropWin->y = u16SrcHeight -
			    (pstCropWin->y + pstCropWin->height);
		else
			pstCropWin->y = 0;
	}
}

static bool _display_ds_set_window(__u32 u32ReadPoint,
			   __u16 u16SrcWidth, __u16 u16SrcHeight,
			   bool bPauseState)
{
	bool bIsXCGenTiming = FALSE;
	MVOP_Handle stHd = { E_MVOP_MODULE_MAIN };
	XC_SETWIN_INFO stXCSetWinInfo;
	XC_OUTPUTFRAME_Info stOutFrameInfo;
	MVOP_Timing mvop_timing;
	__u32 u32WindowID = MVOP_WINDOW;
	bool bResChange = FALSE;

	DISPLAY_GET_RES_PRI;

	memset(&stXCSetWinInfo, 0, sizeof(XC_SETWIN_INFO));
	memset(&stOutFrameInfo, 0, sizeof(XC_OUTPUTFRAME_Info));
	memset(&mvop_timing, 0, sizeof(MVOP_Timing));

	if (RES_DISPLAY.bRefreshWin) {
		RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32width =
		    u16SrcWidth;
		RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32height =
		    u16SrcHeight;
		bResChange = TRUE;
		RES_DISPLAY.bRefreshWin = FALSE;
	}

	stXCSetWinInfo.enInputSourceType =
	    mtk_display_transfer_input_source(RES_DISPLAY.enInputSrcType);

	if (MDrv_MVOP_GetOutputTiming(&mvop_timing) != E_MVOP_OK)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "MDrv_MVOP_GetOutputTiming failed!!\n");

	if (MDrv_MVOP_GetCommand(&stHd, E_MVOP_CMD_GET_IS_XC_GEN_TIMING,
	    &bIsXCGenTiming, sizeof(bIsXCGenTiming)) != E_MVOP_OK) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "E_MVOP_CMD_GET_IS_XC_GEN_TIMING failed!!\n");
	}

	stXCSetWinInfo.bInterlace =
	    _display_is_source_interlace(
	    RES_DISPLAY.frame_info[u32ReadPoint].enScanType);
	stXCSetWinInfo.bHDuplicate = FALSE;
	__u16 u16InputVFreq = 0;

	if (stXCSetWinInfo.bInterlace)
		u16InputVFreq = ((__u16) mvop_timing.u16ExpFrameRate * 2) / 100;
	else
		u16InputVFreq = (__u16) mvop_timing.u16ExpFrameRate / 100;

	if (RES_DISPLAY.bIsEnForceP)
		u16InputVFreq = RES_DISPLAY.u32OutputFrmRate / 100;
	else
		u16InputVFreq = u16InputVFreq;

	stXCSetWinInfo.u16InputVTotal = mvop_timing.u16V_TotalCount;
	stXCSetWinInfo.u16DefaultHtotal = mvop_timing.u16H_TotalCount;

	stOutFrameInfo.u16InVFreq = stXCSetWinInfo.u16InputVFreq;
	stOutFrameInfo.bInterlace = stXCSetWinInfo.bInterlace;

	//update cap/crop size
	if (RES_DISPLAY.bIsXCEnFBL) {
		//if XC is FBL mode, need to do crop by MVOP.
		//update capture size = crop size
		stXCSetWinInfo.stCapWin.width =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32width;
		stXCSetWinInfo.stCapWin.height =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32height;
		//update crop position, change to MVOP crop
		stXCSetWinInfo.stCropWin.x = 0;
		stXCSetWinInfo.stCropWin.y = 0;
		stXCSetWinInfo.stCropWin.width =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32width;
		stXCSetWinInfo.stCropWin.height =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32height;
	} else {
		stXCSetWinInfo.stCapWin.x = RES_DISPLAY.stXCCaptWinInfo.u32x;
		stXCSetWinInfo.stCapWin.y = RES_DISPLAY.stXCCaptWinInfo.u32y;
		stXCSetWinInfo.stCapWin.width =
		    RES_DISPLAY.stXCCaptWinInfo.u32width;
		stXCSetWinInfo.stCapWin.height =
		    RES_DISPLAY.stXCCaptWinInfo.u32height;

		stXCSetWinInfo.stCropWin.x =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32x;
		stXCSetWinInfo.stCropWin.y =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32y;
		stXCSetWinInfo.stCropWin.width =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32width;
		stXCSetWinInfo.stCropWin.height =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin.u32height;
	}

	_display_trans_pqwin_to_xcwin(&stXCSetWinInfo.stDispWin,
	    &RES_DISPLAY.stWindowInfo.stSetWinInfo.stDispWin);
	_display_calculate_mirror(&stXCSetWinInfo.stDispWin,
	    &stXCSetWinInfo.stCropWin, u16SrcWidth, u16SrcHeight);

	//-------------------------
	// customized pre scaling
	//-------------------------

	if (RES_DISPLAY.bIsXCEnFBL == TRUE) {
		//FBL case need to force H pre-scaling
		if (stXCSetWinInfo.stCapWin.width >
		    stXCSetWinInfo.stDispWin.width) {
			//customized Pre scaling
			stXCSetWinInfo.bPreHCusScaling = TRUE;
			stXCSetWinInfo.u16PreHCusScalingSrc =
			    stXCSetWinInfo.stCapWin.width;
			stXCSetWinInfo.u16PreHCusScalingDst =
			    stXCSetWinInfo.stDispWin.width;
			//customized Post scaling
			stXCSetWinInfo.bHCusScaling = TRUE;
			stXCSetWinInfo.u16HCusScalingSrc =
			    stXCSetWinInfo.stDispWin.width;
			stXCSetWinInfo.u16HCusScalingDst =
			    stXCSetWinInfo.stDispWin.width;
		} else {
			stXCSetWinInfo.bPreHCusScaling = FALSE;
			stXCSetWinInfo.u16PreHCusScalingSrc =
			    stXCSetWinInfo.stCapWin.width;
			stXCSetWinInfo.u16PreHCusScalingDst =
			    stXCSetWinInfo.stCapWin.width;
		}
	} else {
		stXCSetWinInfo.bPreHCusScaling = FALSE;
		stXCSetWinInfo.u16PreHCusScalingSrc =
		    stXCSetWinInfo.stCapWin.width;
		stXCSetWinInfo.u16PreHCusScalingDst =
		    stXCSetWinInfo.stCapWin.width;
	}

	stXCSetWinInfo.bPreVCusScaling = FALSE;
	stXCSetWinInfo.u16PreVCusScalingSrc = stXCSetWinInfo.stCapWin.height;
	stXCSetWinInfo.u16PreVCusScalingDst = stXCSetWinInfo.stCapWin.height;

	//customized Post scaling
	stXCSetWinInfo.bHCusScaling = FALSE;
	stXCSetWinInfo.bVCusScaling = FALSE;

	if (bResChange == TRUE) {
		hal_delay_time
		    [MEASURE_TIME_SETWINDOW_DS].u32StartTime =
		    _display_get_time();

		MApi_XC_SkipWaitVsync(MAIN_WINDOW, TRUE);
		MApi_XC_SetWindow(&stXCSetWinInfo,
		    sizeof(XC_SETWIN_INFO), MAIN_WINDOW);
		MApi_XC_SkipWaitVsync(MAIN_WINDOW, FALSE);

		hal_delay_time
		    [MEASURE_TIME_SETWINDOW_DS].u32DelayTime =
		    _display_get_delay_time(hal_delay_time
		    [MEASURE_TIME_SETWINDOW_DS].u32StartTime);
	} else {
		if (((bPauseState == FALSE))
		    || _display_is_need_per_frame_update_xc_setting()
		    || (_display_pq_ds_control(MTK_PQ_PQDS_CHECKSTATUS))) {
			MApi_SWDS_Fire(MAIN_WINDOW);
		}

		hal_delay_time
		    [MEASURE_TIME_SETWINDOW_DS].u32DelayTime = 0;
	}

	MApi_XC_OutputFrameCtrl(TRUE, &stOutFrameInfo, MAIN_WINDOW);

	swds_index = MApi_XC_GetSWDSIndex(MAIN_WINDOW);

	MApi_XC_Set_DSForceIndex(TRUE, swds_index, MAIN_WINDOW);

	is_fired = TRUE;

	return TRUE;
}

static void _display_check_isr_latency(__u32 u32WindowID,
				struct mtk_pq_measure_time *latency_time)
{
	if (latency_time == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	__u32 u32TimeLatency = 0;

	DISPLAY_GET_RES_PRI;

	if ((RES_DISPLAY.u32OutputFrmRate) == 0
	    || (RES_DISPLAY.bFirstPlay == TRUE)
	    || (latency_time->u32DelayTime == 0))
		return;

	//Calculate time latency
	if (latency_time->u32DelayTime >
	    (1000 * 1000 / RES_DISPLAY.u32OutputFrmRate)) {
		u32TimeLatency = latency_time->u32DelayTime -
		    (1000 * 1000 / RES_DISPLAY.u32OutputFrmRate);
	} else {
		u32TimeLatency = (1000 * 1000 / RES_DISPLAY.u32OutputFrmRate) -
		    latency_time->u32DelayTime;
	}

	//Count time latency
	if (u32TimeLatency >= 10 * MTK_PQ_NSEC_PER_USEC)
		latency_time->u32Count[MTK_PQ_UPPER_10_MS]++;
	else if (u32TimeLatency >= 8 * MTK_PQ_NSEC_PER_USEC)
		latency_time->u32Count[MTK_PQ_UPPER_8_MS]++;
	else if (u32TimeLatency >= 6 * MTK_PQ_NSEC_PER_USEC)
		latency_time->u32Count[MTK_PQ_UPPER_6_MS]++;
	else if (u32TimeLatency >= 4 * MTK_PQ_NSEC_PER_USEC)
		latency_time->u32Count[MTK_PQ_UPPER_4_MS]++;
}

irqreturn_t mtk_display_mvop_isr(int irq, void *prv)
{
	if (prv == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	struct device *dev = prv;
	__u32 u32WindowID = MVOP_WINDOW;
	MVOP_Handle stMvopHd = { E_MVOP_MODULE_MAIN };
	__u32 u32IntType;

	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	DISPLAY_GET_RES_PRI;

	MTK_PQ_GET_SYSTEM_TIME(RES_DISPLAY.s64MvopIsrTime);

	if (hal_delay_time[MEASURE_TIME_ISR].u32StartTime == 0) {
		hal_delay_time[MEASURE_TIME_ISR].u32StartTime =
		    _display_get_time();
	} else {
		hal_delay_time[MEASURE_TIME_ISR].u32DelayTime =
		    _display_get_delay_time(hal_delay_time
		    [MEASURE_TIME_ISR].u32StartTime);
		hal_delay_time[MEASURE_TIME_ISR].u32StartTime =
		    hal_delay_time[MEASURE_TIME_ISR].u32StartTime +
		    hal_delay_time[MEASURE_TIME_ISR].u32DelayTime;
	}

	u32IntType = E_MVOP_INT_3A_NONE_EX;
	if (MDrv_MVOP_SetCommand(&stMvopHd,
	    E_MVOP_CMD_SET_DMS_INT, &u32IntType) != 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "E_MVOP_CMD_SET_DMS_INT failed!!\n");
	}

	hal_delay_time[MEASURE_TIME_EVENT].u32StartTime =
	    hal_delay_time[MEASURE_TIME_ISR].u32StartTime;
	MVOP_TASK_WAKE_UP_FLAG = TRUE;
	wake_up(&mvop_task);

	u32IntType = E_MVOP_INT_DC_START;
	if (MDrv_MVOP_SetCommand(&stMvopHd,
	    E_MVOP_CMD_SET_DMS_INT, &u32IntType) != 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "E_MVOP_CMD_SET_DMS_INT failed!!\n");
	}

	return IRQ_HANDLED;

	//MsOS_EnableInterrupt(E_INT_IRQ_DC);
}

static int _display_set_mvop_task(void *data)
{
	if (data == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	struct mtk_pq_device *pq_dev = (struct mtk_pq_device *)data;
	__u32 u32WindowID = (__u32) pq_dev->dev_indx;
	ST_XC_APISTATUSNODELAY stDrvStatus;
	__u32 u32CurReadPointer = 0;
	bool bIsRepeatFrame = FALSE;
	bool bPauseStage = FALSE;
	__u16 u16SrcWidth = 0;
	__u16 u16SrcHeight = 0;
	bool bRepeatLastField = FALSE;
	bool bPauseStageFBL = FALSE;
	enum mtk_pq_field_type enMvopFieldType = MTK_PQ_FIELD_TYPE_MAX;
	enum mtk_pq_field_type enVdecFieldType = MTK_PQ_FIELD_TYPE_MAX;
	__u32 u32ReadPointer = 0;
	bool bUpdateReadPointerByFod = FALSE;
	__u32 u32PModePreReadPointer = 0;
	__u32 u32IModePreReadPointer = 0;

	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	DISPLAY_GET_RES_PRI;

	while (!kthread_should_stop()) {
		MVOP_TASK_WAKE_UP_FLAG = FALSE;
		wait_event(mvop_task, MVOP_TASK_WAKE_UP_FLAG
		    || kthread_should_stop());

		if (kthread_should_stop())
			break;

		hal_delay_time[MEASURE_TIME_EVENT].u32DelayTime =
		    _display_get_delay_time(hal_delay_time
		    [MEASURE_TIME_EVENT].u32StartTime);
		hal_delay_time[MEASURE_TIME_TASK].u32StartTime =
		    _display_get_time();
		_display_obtain_semaphore(u32WindowID);

		bPauseStage = FALSE;
		u16SrcWidth = 0;
		u16SrcHeight = 0;

		hal_delay_time
		    [MEASURE_TIME_RELEASE_BUF].u32StartTime =
		    _display_get_time();

		MApi_XC_GetStatusNodelay(&stDrvStatus, MAIN_WINDOW);

		if (_display_skip_task(stDrvStatus.enInputSourceType,
		    u32WindowID)) {
			_display_release_semaphore(u32WindowID);
			continue;
		}

		_display_release_buffer(pq_dev, u32WindowID);

		hal_delay_time
		    [MEASURE_TIME_RELEASE_BUF].u32DelayTime =
		    _display_get_delay_time(hal_delay_time
		    [MEASURE_TIME_RELEASE_BUF].u32StartTime);

		bIsRepeatFrame =
		    _display_frc_check(u32WindowID,
		    is_even_field, MTK_PQ_FRC_REPEAT);

		if ((RES_DISPLAY.buffer_ptr.read_ptr ==
		    RES_DISPLAY.buffer_ptr.write_ptr)
		    || is_even_field == TRUE) {
			bPauseStage = TRUE;
		}

		if (bPauseStage == FALSE) {
			u32CurReadPointer =
			    RES_DISPLAY.buffer_ptr.read_ptr;
		} else {
			if (RES_DISPLAY.frame_info
			    [u32CurReadPointer].enScanType ==
			    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
				u32CurReadPointer = _display_get_pre_id(
				    RES_DISPLAY.buffer_ptr.read_ptr);
			} else {
				u32CurReadPointer = _display_get_pre_id(
				    RES_DISPLAY.buffer_ptr.read_ptr);
				//odd field, repeat to the last 2 field
				if (is_even_field == FALSE)
					u32CurReadPointer = _display_get_pre_id(
					    u32CurReadPointer);
			}
		}

		RES_DISPLAY.bRefreshWin |= _display_check_pixel_shift(
		    stDrvStatus.s8HOffset, stDrvStatus.s8VOffset);

		if (RES_DISPLAY.frame_info
		    [u32CurReadPointer].bValid == FALSE) {
			is_even_field = FALSE;
			_display_release_semaphore(u32WindowID);
			_display_check_isr_latency(u32WindowID,
			    &hal_delay_time[MEASURE_TIME_ISR]);
			hal_delay_time
			    [MEASURE_TIME_TASK].u32DelayTime =
			    _display_get_delay_time(hal_delay_time
			    [MEASURE_TIME_TASK].u32StartTime);
			continue;
		}

		_display_update_fd_mask_status(u32WindowID, bPauseStage,
		    stDrvStatus.bBlackscreenEnabled,
		    u32CurReadPointer, &bRepeatLastField);

		if (bRepeatLastField && (is_even_field == FALSE))
			u32CurReadPointer = _display_get_next_id(
			    u32CurReadPointer);

		if (_display_is_source_interlace(RES_DISPLAY.frame_info
		    [u32CurReadPointer].enScanType)) {
			if (bPauseStage && RES_DISPLAY.bIsXCEnFBL)
				bPauseStageFBL = TRUE;
			else
				bPauseStageFBL = FALSE;

			_display_get_mvop_and_vdec_field_type(u32WindowID,
			    u32CurReadPointer, &enMvopFieldType,
			    &enVdecFieldType, bRepeatLastField,
			    bPauseStageFBL);

			if (_display_update_read_pointer_by_field_change
			    (u32WindowID, enVdecFieldType, u32CurReadPointer)) {
				u32CurReadPointer =
				    _display_get_next_id(u32CurReadPointer);
				ignore_cur_field = TRUE;
				enVdecFieldType =
				    (enVdecFieldType ==
				     MTK_PQ_FIELD_TYPE_TOP) ?
				    MTK_PQ_FIELD_TYPE_BOTTOM :
				    MTK_PQ_FIELD_TYPE_TOP;
			}

			u32ReadPointer = u32CurReadPointer;
			//set XC BOB mode
			struct mtk_pq_bob_mode_info stBOBModeInfo;

			memset(&stBOBModeInfo, 0,
			    sizeof(struct mtk_pq_bob_mode_info));
			stBOBModeInfo.u32WindowID = u32WindowID;
			stBOBModeInfo.u32CurReadPointer = u32CurReadPointer;
			stBOBModeInfo.bPauseStage = bPauseStage;
			_display_set_bob_mode(u32WindowID, stBOBModeInfo);

			if ((enMvopFieldType != enVdecFieldType)
			    && (RES_DISPLAY.bOneFieldMode == FALSE)) {
				if (_display_is_source_interlace
				    (RES_DISPLAY.frame_info
				    [_display_get_pre_id
				    (u32CurReadPointer)].enScanType)) {
					RES_DISPLAY.bFDMaskEnable = TRUE;
					field_change_cnt =
					    MTK_PQ_FIELD_CHANGE_FIRST_FIELD;
					_display_enable_fd_mask(TRUE);
				}
				_display_release_semaphore(u32WindowID);
				continue;
			} else {
				if (field_change_cnt !=
				    MTK_PQ_FIELD_CHANGE_DISABLE) {
					if (field_change_cnt >
					    MTK_PQ_FIELD_CHANGE_FIRST_FIELD)
						field_change_cnt =
						    MTK_PQ_FIELD_CHANGE_DISABLE;
					else
						field_change_cnt++;
				}
				u32CurReadPointer =
				    _display_detect_field_order(u32WindowID,
				    u32CurReadPointer,
				    bPauseStage, enVdecFieldType,
				    &bUpdateReadPointerByFod);
			}
		} else {
			is_even_field = FALSE;
			u32ReadPointer = u32CurReadPointer;

			if (!_display_get_field_type_for_forcep
			    (u32WindowID, &enMvopFieldType,
			    &enVdecFieldType)) {
				enMvopFieldType = enVdecFieldType =
				    MTK_PQ_FIELD_TYPE_MAX;
			}
		}

		if (RES_DISPLAY.bIsXCEnFBL == TRUE) {
			__u16 u16SrcHeight = 0;
			struct mtk_pq_window stVDECFrameSize;

			memset(&stVDECFrameSize, 0,
				sizeof(struct mtk_pq_window));
			stVDECFrameSize.u32x = 0;
			stVDECFrameSize.u32y = 0;
			stVDECFrameSize.u32width =
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].u16SrcWidth;

			stVDECFrameSize.u32height =
			    RES_DISPLAY.frame_info
			    [u32CurReadPointer].u16SrcHeight;

			if (((RES_DISPLAY.bSetWinImmediately == TRUE)
			    || (bPauseStage == TRUE))
			    && (RES_DISPLAY.bUpdateSetWin == TRUE)) {
				_display_set_scaling_down_condition(
				    &stVDECFrameSize,
				    &user_disp_win
				    [u32WindowID]);
			} else {
				_display_set_scaling_down_condition(
				    &stVDECFrameSize,
				    &RES_DISPLAY.frame_info
				    [u32CurReadPointer].stUserXCDispWin);
			}

			if (_display_is_scaling_use_second_buffer
			    (RES_DISPLAY.scaling_condition))
				u16SrcHeight = RES_DISPLAY.frame_info
				    [u32CurReadPointer].u16Src2ndBufferHeight;
			else
				u16SrcHeight = RES_DISPLAY.frame_info
				    [u32CurReadPointer].u16SrcHeight;


			if (((RES_DISPLAY.bSetWinImmediately == TRUE)
			    || (bPauseStage == TRUE))
			    && (RES_DISPLAY.bUpdateSetWin == TRUE)) {
				_display_get_mvop_drop_line_condition(
				    u32WindowID,
				    u16SrcHeight,
				    user_disp_win[u32WindowID].u32height);
			} else {
				_display_get_mvop_drop_line_condition(
				 u32WindowID, u16SrcHeight,
				 RES_DISPLAY.frame_info
				 [u32CurReadPointer].stUserXCDispWin.u32height);
			}
		}

		u32PModePreReadPointer =
		    _display_get_pre_id(u32CurReadPointer);
		u32IModePreReadPointer =
		    _display_get_pre_id(u32PModePreReadPointer);

		if (RES_DISPLAY.frame_info[u32CurReadPointer].enScanType ==
		    MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE) {
			if (bPauseStage == TRUE) {
				_display_execute_swdr(u32CurReadPointer,
				    MTK_PQ_FIELD_TYPE_NONE,
				    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX,
				    u32WindowID);

				_display_send_frame_info(u32CurReadPointer,
				    MTK_PQ_FIELD_TYPE_NONE,
				    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX,
				    u32WindowID);
			} else if (RES_DISPLAY.frame_info
			    [u32PModePreReadPointer].bValid == TRUE) {
				_display_execute_swdr(u32PModePreReadPointer,
				    MTK_PQ_FIELD_TYPE_NONE,
				    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX,
				    u32WindowID);

				_display_send_frame_info(u32PModePreReadPointer,
				    MTK_PQ_FIELD_TYPE_NONE,
				    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX,
				    u32WindowID);
			} else {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
				    "can't pass histogram! bPauseStage=%d, u32PModePreReadPointer=%d, PreFrameValid=%d\n",
				    bPauseStage, u32PModePreReadPointer,
				    RES_DISPLAY.frame_info
				    [u32PModePreReadPointer].bValid);
			}
		} else if (_display_is_source_interlace(
		    RES_DISPLAY.frame_info[u32CurReadPointer].enScanType)) {
			if (bPauseStage == TRUE) {
				_display_execute_swdr(u32CurReadPointer,
				    enVdecFieldType,
				    RES_DISPLAY.frame_info
				    [u32CurReadPointer].enFieldOrderType,
				    u32WindowID);

				_display_send_frame_info(u32CurReadPointer,
				    enVdecFieldType,
				    RES_DISPLAY.frame_info
				    [u32CurReadPointer].enFieldOrderType,
				    u32WindowID);

			} else if (RES_DISPLAY.frame_info
			    [u32IModePreReadPointer].bValid == TRUE) {
				_display_execute_swdr(u32IModePreReadPointer,
				    enVdecFieldType,
				     RES_DISPLAY.frame_info
				     [u32CurReadPointer].enFieldOrderType,
				     u32WindowID);

				_display_send_frame_info(u32IModePreReadPointer,
				    enVdecFieldType,
				    RES_DISPLAY.frame_info
				    [u32CurReadPointer].enFieldOrderType,
				    u32WindowID);
			} else {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
				    "can't pass histogram! bPauseStage=%d, u32IModePreReadPointer=%d, PreFrameValid=%d\n",
				    bPauseStage, u32IModePreReadPointer,
				    RES_DISPLAY.frame_info
				    [u32IModePreReadPointer].bValid);
			}
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				   "Undefine scanType, can't pass histogram\n");
		}

		//update MVOP setting frame by frame
		_display_update_mvop_setting(u32WindowID,
		    u32CurReadPointer, enVdecFieldType);

		//Update VideoDelay
		_display_update_video_delay_time(pq_dev, u32WindowID,
		    u32CurReadPointer, bPauseStage, enVdecFieldType,
		    bIsRepeatFrame);

		//update frame release count and read pointer
		_display_update_release_count_and_read_pointer(u32WindowID,
		    bPauseStage,
		    u32CurReadPointer,
		    u32ReadPointer,
		    bRepeatLastField,
		    bIsRepeatFrame);

		if (!RES_DISPLAY.bOneFieldMode)
			pre_field_type = enVdecFieldType;

		struct mtk_pq_vdec_frame_info *pstFrmInfo =
		    &(RES_DISPLAY.frame_info[u32CurReadPointer]);


		if (_display_is_first_field(u32WindowID, u32CurReadPointer)
		    || _display_is_need_per_frame_update_xc_setting()) {
			if (_display_is_ds_enable() == TRUE
			    && (IsSrcTypeStorage(stDrvStatus.enInputSourceType)
			    || _display_is_dtv_ds_on(
			    RES_DISPLAY.enInputSrcType))
			    && (!RES_DISPLAY.bFirstSetwindow)
			    && (!RES_DISPLAY.bFDMaskEnable)) {
				//check 2nd buffer VSD condition
				if (_display_is_scaling_use_second_buffer
				    (RES_DISPLAY.scaling_condition)) {
					u16SrcWidth =
					    pstFrmInfo->u16Src2ndBufferWidth;
					u16SrcHeight =
					    _display_drop_line_update_h(
					    pstFrmInfo->u16Src2ndBufferHeight,
					     RES_DISPLAY.enMVOPDropline,
					     MTK_PQ_UPDATE_HEIGHT_XC);
				} else {
					u16SrcWidth =
					    pstFrmInfo->u16SrcWidth -
					    pstFrmInfo->u16CropLeft -
					    pstFrmInfo->u16CropRight;

					u16SrcHeight =
					    pstFrmInfo->u16SrcHeight -
					    pstFrmInfo->u16CropTop -
					    pstFrmInfo->u16CropBottom;

					if (RES_DISPLAY.bIsXCEnFBL == TRUE) {
						u16SrcHeight =
						    _display_drop_line_update_h(
						    u16SrcHeight,
						    RES_DISPLAY.enMVOPDropline,
						    MTK_PQ_UPDATE_HEIGHT_XC);
					}
				}

				if (_display_is_first_field(u32WindowID,
				    u32CurReadPointer)) {
					_display_update_user_set_window(
					    u32WindowID,
					    u32CurReadPointer,
					    bPauseStage);

					_display_regen_crop_window(
					    u32WindowID,
					    u32CurReadPointer,
					    bPauseStage,
					    &u16SrcWidth,
					    &u16SrcHeight);

					_display_update_mvop_crop(
					    u32WindowID, u32CurReadPointer);

					_display_set_fast_forward_status(
					    u32WindowID, u32CurReadPointer);
				}

				hal_delay_time
				    [MEASURE_TIME_DOLBY_DS].u32StartTime =
				    _display_get_time();

				_display_update_setting(MVOP_WINDOW,
				    u32CurReadPointer, bPauseStage);

				hal_delay_time
				    [MEASURE_TIME_DOLBY_DS].u32DelayTime =
				    _display_get_delay_time(hal_delay_time
				    [MEASURE_TIME_DOLBY_DS].u32StartTime);

				_display_ds_set_window(u32CurReadPointer,
				    u16SrcWidth, u16SrcHeight, bPauseStage);

				if ((RES_DISPLAY.u32PQFrameCnt
				    < MAX_PQDS_FRAMECOUNT)
				    && _display_is_first_field(u32WindowID,
				    u32CurReadPointer)) {
					//check first set window done.
					if (RES_DISPLAY.bPQDSEnable == FALSE) {
						_display_pq_ds_control
						    (MTK_PQ_PQDS_ENABLE_CHECK);
					} else {
						_display_pq_ds_control
						    (MTK_PQ_PQDS_UPDATEINDEX);
					}

					RES_DISPLAY.u32PQFrameCnt++;
				}
			} else {
				_display_update_mvop_crop(u32WindowID,
				    u32CurReadPointer);
			}
		}

		_display_release_semaphore(u32WindowID);
		_display_check_isr_latency(u32WindowID,
		    &hal_delay_time[MEASURE_TIME_ISR]);

		hal_delay_time[MEASURE_TIME_TASK].u32DelayTime =
		    _display_get_delay_time(hal_delay_time
		    [MEASURE_TIME_TASK].u32StartTime);
	}

	return 0;
}

static void _display_clear_source_frame_ref(__u32 u32WindowID, __u16 u16BufID,
					    struct mtk_pq_device *pq_dev)
{
	if (pq_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	__u8 u8RefCounter = 0;

	DISPLAY_GET_RES_PRI;

	for (u8RefCounter = 0; u8RefCounter <
	    RES_DISPLAY.u8LocalFrameRefCount[u16BufID]; u8RefCounter++)
		_display_source_frame_release(pq_dev, u32WindowID, u16BufID);
}

static bool _display_video_flip_update_shm(__u32 u32WindowID,
			   struct mtk_pq_frame_info *pstFrameFormat,
			   struct mtk_pq_vdec_frame_info *stFrameInfo)
{
	if ((pstFrameFormat == NULL) || (stFrameInfo == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	_display_obtain_semaphore(u32WindowID);
	RES_DISPLAY.u32InputFrmRate = pstFrameFormat->u32FrameRate;
	_display_release_semaphore(u32WindowID);
	return TRUE;
}

static bool _display_video_flip_frame_info(__u32 u32WindowID,
			   struct mtk_pq_frame_info *pstFrameFormat,
			   struct mtk_pq_vdec_frame_info *stFrameInfo)
{
	if ((pstFrameFormat == NULL) || (stFrameInfo == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	stFrameInfo->u32Window = u32WindowID;

	stFrameInfo->u16SrcWidth = pstFrameFormat->stFrames[0].u32Width;

	stFrameInfo->u16SrcHeight = pstFrameFormat->stFrames[0].u32Height;

	stFrameInfo->u16SrcPitch =
	    pstFrameFormat->stFrames[0].stHWFormat.u32LumaPitch;

	stFrameInfo->phySrcLumaAddr =
	    pstFrameFormat->stFrames[0].stHWFormat.phyLumaAddr;

	stFrameInfo->phySrcChromaAddr =
	    pstFrameFormat->stFrames[0].stHWFormat.phyChromaAddr;

	stFrameInfo->enFrameType = pstFrameFormat->stFrames[0].enFrameType;

	stFrameInfo->u8AspectRate = pstFrameFormat->u8AspectRate;

	stFrameInfo->enFieldType = pstFrameFormat->stFrames[0].enFieldType;

	stFrameInfo->u16CropRight = pstFrameFormat->stFrames[0].u32CropRight;

	stFrameInfo->u16CropLeft = pstFrameFormat->stFrames[0].u32CropLeft;

	stFrameInfo->u16CropBottom = pstFrameFormat->stFrames[0].u32CropBottom;

	stFrameInfo->u16CropTop = pstFrameFormat->stFrames[0].u32CropTop;

	stFrameInfo->u32FrameRate = pstFrameFormat->u32FrameRate;

	stFrameInfo->enCODEC = pstFrameFormat->u32CodecType;

	stFrameInfo->enTileMode = pstFrameFormat->u32TileMode;

	stFrameInfo->enFmt =
	    _display_get_format(pstFrameFormat->color_format);

	stFrameInfo->enScanType =
	    _display_get_scan_type(pstFrameFormat->u8Interlace);

	if (pstFrameFormat->u8BottomFieldFirst == 0)
		stFrameInfo->enFieldOrderType =
		    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP;
	else
		stFrameInfo->enFieldOrderType =
		    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM;

	stFrameInfo->u83DMode = pstFrameFormat->u83DMode;

	stFrameInfo->u8ApplicationType = pstFrameFormat->u8ApplicationType;

	stFrameInfo->phySrcLumaAddrI =
	    pstFrameFormat->stFrames[1].stHWFormat.phyLumaAddr;

	stFrameInfo->phySrcChromaAddrI =
	    pstFrameFormat->stFrames[1].stHWFormat.phyChromaAddr;

	stFrameInfo->u32FrameIndex_2nd = pstFrameFormat->stFrames[1].u32Idx;

	stFrameInfo->u32PriData_2nd = pstFrameFormat->stFrames[1].u32PriData;

	stFrameInfo->u8FieldCtrl = pstFrameFormat->u8FieldCtrl;

	stFrameInfo->b10bitData =
	    (pstFrameFormat->color_format ==
	    MTK_PQ_COLOR_FORMAT_10BIT_TILE) ? TRUE : FALSE;

	if (stFrameInfo->b10bitData) {
		stFrameInfo->u16Src10bitPitch =
		    pstFrameFormat->stFrames[0].stHWFormat.u32LumaPitch2Bit;

		stFrameInfo->phySrcLumaAddr_2bit =
		    pstFrameFormat->stFrames[0].stHWFormat.phyLumaAddr2Bit;

		stFrameInfo->phySrcChromaAddr_2bit =
		    pstFrameFormat->stFrames[0].stHWFormat.phyChromaAddr2Bit;

		stFrameInfo->u8LumaBitdepth =
		    pstFrameFormat->stFrames[0].u8LumaBitdepth;
	}

	stFrameInfo->phySrc2ndBufferLumaAddr =
	    pstFrameFormat->stFrames[0].stHWFormat.phyLumaAddr_subsample;

	stFrameInfo->phySrc2ndBufferChromaAddr =
	    pstFrameFormat->stFrames[0].stHWFormat.phyChromaAddr_subsample;

	stFrameInfo->u16Src2ndBufferPitch =
	    pstFrameFormat->stFrames[0].stHWFormat.u16Pitch_subsample;

	stFrameInfo->u16Src2ndBufferWidth =
	    pstFrameFormat->stFrames[0].stHWFormat.u16Width_subsample;

	stFrameInfo->u16Src2ndBufferHeight =
	    pstFrameFormat->stFrames[0].stHWFormat.u16Height_subsample;

	stFrameInfo->u8Src2ndBufferTileMode =
	    pstFrameFormat->stFrames[0].stHWFormat.u8TileMode_subsample;
	//stFrameInfo->u8Src2ndBufferV7DataValid =
	//    pstFrameFormat->stFrames[0].stHWFormat.phyLumaAddr_subsample;

	return TRUE;
}

static bool _display_video_flip_frame_index(__u32 u32WindowID,
			    struct mtk_pq_frame_info *pstFrameFormat,
			    struct mtk_pq_vdec_frame_info *stFrameInfo)
{
	if ((pstFrameFormat == NULL) || (stFrameInfo == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	stFrameInfo->u32FrameIndex = pstFrameFormat->stFrames[0].u32Idx;
	stFrameInfo->u32VDECStreamID = pstFrameFormat->u32VdecStreamId;
	stFrameInfo->u32VDECStreamVersion =
	    pstFrameFormat->u32VdecStreamVersion;

	stFrameInfo->u32PriData = pstFrameFormat->stFrames[0].u32PriData;
	stFrameInfo->u64Pts = pstFrameFormat->u64Pts;
	stFrameInfo->u64PtsUs = pstFrameFormat->u64PtsUs;
	stFrameInfo->u32FrameNum = pstFrameFormat->u32FrameNum;
	stFrameInfo->bCheckFrameDisp = pstFrameFormat->bCheckFrameDisp;

	return TRUE;
}

static bool _display_video_flip_mf_codec_info(__u32 u32WindowID,
			      struct mtk_pq_frame_info *pstFrameFormat,
			      struct mtk_pq_vdec_frame_info *stFrameInfo)
{
	if ((pstFrameFormat == NULL) || (stFrameInfo == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	__u32 MFCodecInfo =
	    pstFrameFormat->stFrames[0].stHWFormat.u32MFCodecInfo;

	if ((MFCodecInfo & 0xFF) != 0 && (MFCodecInfo & 0xFF) != 0xFF) {
		stFrameInfo->stMFdecInfo.bMFDec_Enable = 1;
		stFrameInfo->stMFdecInfo.bBypass_codec_mode = 0;

		stFrameInfo->stMFdecInfo.u8MFDec_Select =
		    (MFCodecInfo >> 8) & 0x3;

		stFrameInfo->stMFdecInfo.bUncompress_mode =
		    (MFCodecInfo >> 28) & 0x1;

		if (((MFCodecInfo >> 29) & 0x1) == 0x01)
			stFrameInfo->stMFdecInfo.enMFDecVP9_mode =
			    MTK_PQ_DUMMY_VIDEO_VP9_MODE;
		else
			stFrameInfo->stMFdecInfo.enMFDecVP9_mode =
			    MTK_PQ_DUMMY_VIDEO_H26X_MODE;

		stFrameInfo->stMFdecInfo.u16Bitlen_Pitch =
		    (MFCodecInfo >> 16) & 0xFF;

		stFrameInfo->stMFdecInfo.bAV1_mode =
		    (MFCodecInfo >> 30) & 0x1;

		stFrameInfo->stMFdecInfo.u16StartX = stFrameInfo->u16CropLeft;
		stFrameInfo->stMFdecInfo.u16StartY = stFrameInfo->u16CropTop;

		__u8 u8BitLenmiu = 0;

		u8BitLenmiu = (MFCodecInfo >> 24) & 0xF;
		stFrameInfo->stMFdecInfo.u8Bitlen_MiuSelect = u8BitLenmiu;
		stFrameInfo->stMFdecInfo.phyBitlen_Base =
		    _display_get_phy_bit_len_base(u8BitLenmiu,
		    pstFrameFormat->stFrames[0].stHWFormat.phyMFCBITLEN);
	} else {
		stFrameInfo->stMFdecInfo.bMFDec_Enable = 0;
	}

	return TRUE;
}

static bool _display_video_flip_dolby_hdr_info(__u32 u32WindowID,
			       struct mtk_pq_frame_info *pstFrameFormat,
			       struct mtk_pq_vdec_frame_info *stFrameInfo)
{
	if ((pstFrameFormat == NULL) || (stFrameInfo == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	stFrameInfo->stHDRInfo.u32FrmInfoExtAvail =
	    pstFrameFormat->stHDRInfo.u32FrmInfoExtAvail;
#if (API_XCDS_INFO_VERSION >= 2)
	stFrameInfo->stHDRInfo.u32FrmInfoExtAvail =
	    pstFrameFormat->stHDRInfo.u32FrmInfoExtAvail;

	struct mtk_pq_color_description *pstdstColorDesc =
	    &(stFrameInfo->stHDRInfo.stColorDescription);

	struct mtk_pq_color_description *pstsrcColorDesc =
	    &(pstFrameFormat->stHDRInfo.stColorDescription);


	pstdstColorDesc->u8ColorPrimaries =
	    pstsrcColorDesc->u8ColorPrimaries;

	pstdstColorDesc->u8MatrixCoefficients =
	    pstsrcColorDesc->u8MatrixCoefficients;

	pstdstColorDesc->u8TransferCharacteristics =
	    pstsrcColorDesc->u8TransferCharacteristics;

	struct mtk_pq_master_color_display *pstdstMColorDisp =
	    &(stFrameInfo->stHDRInfo.stMasterColorDisplay);

	struct mtk_pq_master_color_display *pstsrcMColorDisp =
	    &(pstFrameFormat->stHDRInfo.stMasterColorDisplay);


	pstdstMColorDisp->u32MaxLuminance =
	    pstsrcMColorDisp->u32MaxLuminance;

	pstdstMColorDisp->u32MinLuminance =
	    pstsrcMColorDisp->u32MinLuminance;

	pstdstMColorDisp->u16DisplayPrimaries[0][0] =
	    pstsrcMColorDisp->u16DisplayPrimaries[0][0];

	pstdstMColorDisp->u16DisplayPrimaries[0][1] =
	    pstsrcMColorDisp->u16DisplayPrimaries[0][1];

	pstdstMColorDisp->u16DisplayPrimaries[1][0] =
	    pstsrcMColorDisp->u16DisplayPrimaries[1][0];

	pstdstMColorDisp->u16DisplayPrimaries[1][1] =
	    pstsrcMColorDisp->u16DisplayPrimaries[1][1];

	pstdstMColorDisp->u16DisplayPrimaries[2][0] =
	    pstsrcMColorDisp->u16DisplayPrimaries[2][0];

	pstdstMColorDisp->u16DisplayPrimaries[2][1] =
	    pstsrcMColorDisp->u16DisplayPrimaries[2][1];

	pstdstMColorDisp->u16WhitePoint[0] =
	    pstsrcMColorDisp->u16WhitePoint[0];

	pstdstMColorDisp->u16WhitePoint[1] =
	    pstsrcMColorDisp->u16WhitePoint[1];

	struct mtk_pq_dolby_hdr_info *pstdstDolbyInfo =
	    &(stFrameInfo->stHDRInfo.stDolbyHDRInfo);

	struct mtk_pq_dolby_hdr_info *pstsrcDolbyInfo =
	    &(pstFrameFormat->stHDRInfo.stDolbyHDRInfo);

	pstdstDolbyInfo->u8CurrentIndex = pstsrcDolbyInfo->u8CurrentIndex;
	pstdstDolbyInfo->phyHDRRegAddr = pstsrcDolbyInfo->phyHDRRegAddr;
	pstdstDolbyInfo->u32HDRRegSize = pstsrcDolbyInfo->u32HDRRegSize;
	pstdstDolbyInfo->phyHDRLutAddr = pstsrcDolbyInfo->phyHDRLutAddr;
	pstdstDolbyInfo->u32HDRLutSize = pstsrcDolbyInfo->u32HDRLutSize;
	pstdstDolbyInfo->u8DMEnable = pstsrcDolbyInfo->u8DMEnable;
	pstdstDolbyInfo->u8CompEnable = pstsrcDolbyInfo->u8CompEnable;
#endif

	return TRUE;
}

static bool _display_video_flip_store_frame_info(struct mtk_pq_device *pq_dev,
				struct vb2_buffer *vb, __u32 u32WindowID,
				struct mtk_pq_frame_info *pstFrameFormat,
				struct mtk_pq_vdec_frame_info *stFrameInfo)
{
	if ((pq_dev == NULL) || (vb == NULL)
	    || (pstFrameFormat == NULL) || (stFrameInfo == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	_display_obtain_semaphore(u32WindowID);

	stFrameInfo->u8ContinuousTime = 1;

	if (FALSE ==
	    _display_store_vdec_info_to_queue(pq_dev, vb,
		stFrameInfo, pstFrameFormat, u32WindowID)) {
		_display_release_semaphore(u32WindowID);
		return FALSE;
	}

	if (_display_is_source_interlace(stFrameInfo->enScanType)) {
		stFrameInfo->u8ContinuousTime += 1;
		stFrameInfo->bIs2ndField = TRUE;

		if (FALSE ==
		    _display_store_vdec_info_to_queue(pq_dev, vb,
			stFrameInfo, pstFrameFormat, u32WindowID)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			    "store info to queue fail.\n");
			_display_release_semaphore(u32WindowID);
			return FALSE;
		}

		if (pstFrameFormat->u8ToggleTime == 3) {
			stFrameInfo->u8ContinuousTime += 1;
			stFrameInfo->bIs2ndField = FALSE;

			if (FALSE ==
			    _display_store_vdec_info_to_queue(pq_dev, vb,
				stFrameInfo, pstFrameFormat, u32WindowID)) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				    "store info to queue fail.\n");
				_display_release_semaphore(u32WindowID);
				return FALSE;
			}
		}
	}
	_display_release_semaphore(u32WindowID);
	return TRUE;
}

static bool _display_is_support_flip_trigger(void)
{
	return TRUE;		//mark---sid
}

static void _display_send_trigger_event(__u32 u32WindowID)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	MVOP_TASK_WAKE_UP_FLAG = TRUE;
	wake_up(&mvop_task);
}

static bool _display_video_flip_trigger_event(__u32 u32WindowID)
{
	if (_display_is_support_flip_trigger() == FALSE
	    || RES_COMMON->bProcfsFlipTrigEvent == FALSE) {
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	_display_obtain_semaphore(u32WindowID);
	__u16 u16ReadPointer = RES_DISPLAY.buffer_ptr.read_ptr;
	__u16 u16PreWritePointer = _display_get_pre_w_ptr(u32WindowID);
	__u16 u16PreReadPointer = _display_get_pre_id(u16ReadPointer);

	if ((u16ReadPointer == u16PreWritePointer)
	    && (RES_DISPLAY.frame_info
	    [u16PreReadPointer].u32TaskTrigCounts >= 2)) {
		MVOP_Handle stHdl = { E_MVOP_MODULE_MAIN };
		MVOP_DMSFireOnTime fire_on_time;

		memset(&fire_on_time, 0, sizeof(MVOP_DMSFireOnTime));
		fire_on_time.u32NeedTime = MTK_PQ_EXPECTED_TASKPROCESSTIME;

		MDrv_MVOP_GetCommand(&stHdl,
		    E_MVOP_CMD_GET_IS_DMS_FIRE_IN_TIME, &fire_on_time,
		    sizeof(MVOP_DMSFireOnTime));

		if (fire_on_time.bIsOnTime) {
			RES_DISPLAY.frame_info
			    [u16PreWritePointer].bIsFlipTrigFrame = TRUE;
			_display_send_trigger_event(u32WindowID);
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			 "DISPLAY Send Flip Trigger Event, r/w=(%d, %d), u32Vcount=%d, u32NeedTime=%d\n",
			 RES_DISPLAY.buffer_ptr.read_ptr,
			 RES_DISPLAY.buffer_ptr.write_ptr,
			 fire_on_time.u32Vcount,
			 fire_on_time.u32NeedTime);
		}
	}
	_display_release_semaphore(u32WindowID);

	return TRUE;
}

static bool _display_video_flip(struct mtk_pq_device *pq_dev,
				struct vb2_buffer *vb, __u32 u32WindowID,
				struct mtk_pq_frame_info *pstFrameFormat)
{
	if ((pq_dev == NULL) || (vb == NULL) || (pstFrameFormat == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	DISPLAY_GET_RES_PRI;

	if (u32WindowID == 0) {
		if (drv_delay_time
		    [MEASURE_TIME_FLIP_INTERVAL].u32StartTime == 0) {
			drv_delay_time
			    [MEASURE_TIME_FLIP_INTERVAL].u32StartTime =
			    _display_get_time();
		} else {
			drv_delay_time
			    [MEASURE_TIME_FLIP_INTERVAL].u32DelayTime =
			    _display_get_delay_time(drv_delay_time
			    [MEASURE_TIME_FLIP_INTERVAL].u32StartTime);

			drv_delay_time
			    [MEASURE_TIME_FLIP_INTERVAL].u32StartTime =
			    drv_delay_time
			    [MEASURE_TIME_FLIP_INTERVAL].u32StartTime +
			    drv_delay_time
			    [MEASURE_TIME_FLIP_INTERVAL].u32DelayTime;

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			    "Flip interval = %d ms\n",
			    drv_delay_time
			    [MEASURE_TIME_FLIP_INTERVAL].u32DelayTime);
		}

		drv_delay_time[MEASURE_TIME_FLIP].u32StartTime =
		    _display_get_time();
	}

	bool bIsDropFrame = FALSE;
	struct mtk_pq_vdec_frame_info stFrameInfo;

	memset(&stFrameInfo, 0, sizeof(stFrameInfo));

	_display_video_flip_update_shm(u32WindowID,
	    pstFrameFormat, &stFrameInfo);

	_display_obtain_semaphore(u32WindowID);
	bIsDropFrame = _display_frc_check(u32WindowID, FALSE, MTK_PQ_FRC_DROP);
	_display_release_semaphore(u32WindowID);

	if ((u32WindowID == 0)
	    && (bIsDropFrame || (RES_DISPLAY.bIsSeqChange == TRUE)
	    || (is_digital_cleared == TRUE))) {
		_display_obtain_semaphore(u32WindowID);
		_display_video_flip_drop(pq_dev, vb, u32WindowID,
			pstFrameFormat, &stFrameInfo);
		_display_release_semaphore(u32WindowID);
	} else {
		_display_store_flip_time(
			RES_DISPLAY.buffer_ptr.write_ptr,
			u32WindowID, FALSE);

		_display_video_flip_frame_info(u32WindowID,
		    pstFrameFormat, &stFrameInfo);

		_display_video_flip_frame_index(u32WindowID,
		    pstFrameFormat, &stFrameInfo);

		_display_video_flip_mf_codec_info(u32WindowID,
		    pstFrameFormat, &stFrameInfo);

		_display_video_flip_dolby_hdr_info(u32WindowID,
		    pstFrameFormat, &stFrameInfo);

		_display_video_flip_store_frame_info(pq_dev, vb,
		    u32WindowID, pstFrameFormat, &stFrameInfo);

		_display_video_flip_trigger_event(u32WindowID);
	}

	drv_delay_time[MEASURE_TIME_FLIP].u32DelayTime =
	    _display_get_delay_time(drv_delay_time
	    [MEASURE_TIME_FLIP].u32StartTime);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	return TRUE;
}

static bool _display_set_ds_on_off(__u32 u32WindowID, bool bEnable)
{

	bool bRet = FALSE;
	XC_DynamicScaling_Info stXCDsInfo;
	XC_DS_INFO stDSInfo;

	DISPLAY_GET_RES_PRI;

	memset(&stXCDsInfo, 0, sizeof(XC_DynamicScaling_Info));
	memset(&stDSInfo, 0, sizeof(XC_DS_INFO));

	if (RES_DISPLAY.stDSInfo.u32DynScalingSize != 0) {
		//set ds mem size to xc shm if open ds
		if (bEnable == TRUE) {
			stDSInfo.u32ApiDSInfo_Version = API_XCDS_INFO_VERSION;
			stDSInfo.u16ApiDSInfo_Length = sizeof(XC_DS_INFO);
			stDSInfo.bUpdate_DS_CMD[MVOP_WINDOW] = FALSE;
			stDSInfo.bDynamicScalingEnable[MVOP_WINDOW] = FALSE;
			stDSInfo.u32DSBufferSize =
			    RES_DISPLAY.stDSInfo.u32DynScalingSize;

			stDSInfo.stHDRInfo.u8CurrentIndex = 0xFF;

			stDSInfo.bEnable_ForceP[MVOP_WINDOW] =
			    RES_DISPLAY.bIsEnForceP;

			MApi_XC_SetDSInfo(&stDSInfo,
			    sizeof(XC_DS_INFO), MVOP_WINDOW);
		}

		stXCDsInfo.u32DS_Info_BaseAddr =
		    RES_DISPLAY.stDSInfo.phyDynScalingAddr;

		stXCDsInfo.u8MIU_Select =
		    (RES_DISPLAY.stDSInfo.phyDynScalingAddr
		    >= HAL_MIU1_BASE) ? 1 : 0;

		stXCDsInfo.u8DS_Index_Depth = 32;

		if (bEnable) {
			stXCDsInfo.bOP_DS_On =
			    (RES_DISPLAY.bIsXCEnFBL) ? FALSE : TRUE;
			stXCDsInfo.bIPS_DS_On = FALSE;
			stXCDsInfo.bIPM_DS_On = TRUE;
		} else {
			stXCDsInfo.bOP_DS_On = FALSE;
			stXCDsInfo.bIPS_DS_On = FALSE;
			stXCDsInfo.bIPM_DS_On = FALSE;
		}

		bRet = MApi_XC_SetDynamicScaling(&stXCDsInfo,
		    sizeof(XC_DynamicScaling_Info), u32WindowID);

		if (!bRet)
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			    "[DS_Info]MApi_XC_SetDynamicScaling fail\n");
	}

	return bRet;
}



static bool _display_is_flow_type_correct(__u8 *pu8FlowType)
{
	if (pu8FlowType == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	*pu8FlowType = MTK_PQ_FLOW_TYPE;

	return TRUE;
}

static bool _display_is_start_check_sequence_change(__u16 u16SrcWidth,
			    __u16 u16SrcHeight,
			    __u32 u32FrameRate,
			    enum mtk_pq_scan_type enScanType)
{
	if ((u16SrcWidth == 0) || (u16SrcHeight == 0))
		return FALSE;

	if (u32FrameRate == 0)
		return FALSE;

	if (enScanType == MTK_PQ_VIDEO_SCAN_TYPE_MAX)
		return FALSE;

	return TRUE;
}

static bool _display_isneedseqchange(__u32 u32WindowID,
			struct mtk_pq_frame_info *pstFrameFormat)
{
	if (pstFrameFormat == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return FALSE;
	}

	__u32 u32CurWritePointer = 0;
	__u32 u32FrameRate = 0;
	__u16 u16SrcWidth = 0;
	__u16 u16SrcHeight = 0;
	__u8 u8AspectRate = 0;
	enum mtk_pq_scan_type enScanType = MTK_PQ_VIDEO_SCAN_TYPE_MAX;
	__u8 u8SeqChangeStatus = MTK_PQ_SEQ_CHANGE_NONE;
	__u8 u8SeqChangeAll =
	    (MTK_PQ_SEQ_CHANGE_TIMING | MTK_PQ_SEQ_CHANGE_FRAMERATE
	    | MTK_PQ_SEQ_CHANGE_IP | MTK_PQ_SEQ_CHANGE_ASPECTRATE);
	bool bReturn = FALSE;

	DISPLAY_GET_RES_PRI;

	u32CurWritePointer = _display_get_pre_id(
	    RES_DISPLAY.buffer_ptr.write_ptr);

	u32FrameRate =
	    RES_DISPLAY.frame_info[u32CurWritePointer].u32FrameRate;

	u16SrcWidth =
	    RES_DISPLAY.frame_info[u32CurWritePointer].u16SrcWidth;

	u16SrcHeight =
	    RES_DISPLAY.frame_info[u32CurWritePointer].u16SrcHeight;

	enScanType =
	    RES_DISPLAY.frame_info[u32CurWritePointer].enScanType;

	u8AspectRate =
	    RES_DISPLAY.frame_info[u32CurWritePointer].u8AspectRate;

	if ((RES_DISPLAY.bFirstPlay == FALSE)
	    && _display_is_start_check_sequence_change(u16SrcWidth,
	    u16SrcHeight, u32FrameRate, enScanType)
	    && (_display_is_frame_valid(pstFrameFormat) == TRUE)) {
		if (((u16SrcWidth != pstFrameFormat->stFrames[0].u32Width)
		    || (u16SrcHeight != pstFrameFormat->stFrames[0].u32Height))
		    && (_display_is_ds_enable() == FALSE))
			u8SeqChangeStatus =
			    u8SeqChangeStatus | MTK_PQ_SEQ_CHANGE_TIMING;

		if ((u32FrameRate != pstFrameFormat->u32FrameRate)
		    && (RES_DISPLAY.bIsEnFRC == FALSE))
			u8SeqChangeStatus =
			    u8SeqChangeStatus | MTK_PQ_SEQ_CHANGE_FRAMERATE;

		if ((enScanType !=
		    _display_get_scan_type(pstFrameFormat->u8Interlace))
		    && (RES_DISPLAY.bIsEnForceP == FALSE))
			u8SeqChangeStatus =
			    u8SeqChangeStatus | MTK_PQ_SEQ_CHANGE_IP;

		if ((u8AspectRate != pstFrameFormat->u8AspectRate)
		    && (_display_is_ds_enable() == FALSE))
			u8SeqChangeStatus =
			    u8SeqChangeStatus | MTK_PQ_SEQ_CHANGE_ASPECTRATE;

		if (u8SeqChangeStatus & u8SeqChangeAll) {
			bReturn = TRUE;
			RES_DISPLAY.bIsSeqChange = TRUE;
		} else {
			bReturn = FALSE;
		}
	} else {
		bReturn = FALSE;
	}

	return bReturn;
}

static int _display_create_window(__u32 u32WindowID)
{
	int ret = 0;

	DISPLAY_GET_RES_PRI;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");
	RES_DISPLAY.stWindowInfo.u32WinIsused = TRUE;

	return ret;
}

static int _display_set_window(__u32 u32WindowID,
			struct mtk_pq_set_window *pst_SetWindow_info)
{
	if (pst_SetWindow_info == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	int ret = 0;

	DISPLAY_GET_RES_PRI;
	if (_display_is_ds_enable() == FALSE) {
		memcpy(&RES_DISPLAY.stXCCaptWinInfo,
		    &pst_SetWindow_info->stCapWin,
			sizeof(struct mtk_pq_window));

		memcpy(&RES_DISPLAY.stWindowInfo.stSetWinInfo.stDispWin,
		       &pst_SetWindow_info->stOutputWin,
		       sizeof(struct mtk_pq_window));

		memcpy(&RES_DISPLAY.stWindowInfo.stSetWinInfo.stCropWin,
			&pst_SetWindow_info->stCropWin,
			sizeof(struct mtk_pq_window));

		RES_DISPLAY.stWindowInfo.stSetWinInfo.enARC =
		    pst_SetWindow_info->enARC;
	}
	memcpy(&user_disp_win[u32WindowID],
	    &pst_SetWindow_info->stUserDispWin, sizeof(struct mtk_pq_window));

	memcpy(&user_capt_win[u32WindowID],
	    &pst_SetWindow_info->stCapWin, sizeof(struct mtk_pq_window));

	memcpy(&user_crop_win[u32WindowID],
	    &pst_SetWindow_info->stCropWin, sizeof(struct mtk_pq_window));

	user_arc[u32WindowID] = pst_SetWindow_info->enARC;

	RES_DISPLAY.bSetWinImmediately = pst_SetWindow_info->bSetWinImmediately;

	if (RES_DISPLAY.bFirstSetwindow)
		RES_DISPLAY.bFirstSetwindow = FALSE;

	RES_DISPLAY.bUpdateSetWin = TRUE;
	//patch flow for T32 demo remove for M6
	MApi_XC_GenerateBlackVideo(0, 0);

	return ret;
}

static int _display_destroy_window(__u32 u32WindowID)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "WindowID = %d\n", u32WindowID);

	int ret = 0;

	DISPLAY_GET_RES_PRI;
	RES_DISPLAY.bUpdateSetWin = FALSE;
	RES_DISPLAY.bIsSeqChange = FALSE;
	RES_DISPLAY.stWindowInfo.u32WinIsused = FALSE;
	RES_DISPLAY.enInputSrcType = MTK_PQ_INPUTSRC_NONE;

	_display_set_ds_on_off(u32WindowID, FALSE);

	return ret;
}

static int _display_set_swfrc(__u32 u32WindowID, bool *enSWFRC)
{
	if (enSWFRC == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	int ret = 0;

	DISPLAY_GET_RES_PRI;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "enSWFRC = %d\n", *enSWFRC);

	RES_DISPLAY.bIsEnFRC = *enSWFRC;
	return ret;
}

static int _display_pre_get_win_id(struct v4l2_rect *disp_win)
{
	if (disp_win == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	memcpy(&pre_get_id_info, disp_win, sizeof(struct v4l2_rect));
	display_get_ctrl_type = MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWID;

	return 0;
}

static int _display_pre_get_win_info(
		struct mtk_pq_window_info *pq_win_info)
{
	if (pq_win_info == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	memcpy(&pre_get_win_info,
	    pq_win_info, sizeof(struct mtk_pq_window_info));

	display_get_ctrl_type = MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWINFO;

	return 0;
}

static int _display_pre_get_frame_release_info(void)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	display_get_ctrl_type = MTK_PQ_DISPLAY_CTRL_TYPE_FRAME_RELEASE_DATA;

	return 0;
}

static int _display_pre_get_callback_info(void)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	display_get_ctrl_type = MTK_PQ_DISPLAY_CTRL_TYPE_CALLBACK_DATA;

	return 0;
}

static int _display_pre_get_video_delay_time(
			struct mtk_pq_delay_info *pstVideoDelayInfo)
{
	if (pstVideoDelayInfo == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	memcpy(&pre_get_delay_info, pstVideoDelayInfo,
	    sizeof(struct mtk_pq_delay_info));

	display_get_ctrl_type = MTK_PQ_DISPLAY_CTRL_TYPE_CALLBACK_DATA;

	return 0;
}

static int _display_get_windowinfo(__u32 u32WindowID,
			struct mtk_pq_window_info *pq_win_info)
{
	if (pq_win_info == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	__u8 u8FlowType = 0;
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	_display_obtain_semaphore(u32WindowID);
	DISPLAY_GET_RES_PRI;
	if (_display_is_window_used(u32WindowID) == TRUE) {
		memcpy(pq_win_info, &pre_get_win_info,
		    sizeof(struct mtk_pq_window_info));

		pq_win_info->u32DeviceID =
		    RES_DISPLAY.stWindowInfo.u32DeviceID;

		pq_win_info->u32WinID =
		    RES_DISPLAY.stWindowInfo.u32WinID;

		pq_win_info->u32Layer =
		    RES_DISPLAY.stWindowInfo.u32Layer;

		pq_win_info->stZorderInfo =
		    RES_DISPLAY.stWindowInfo.stZorderInfo;

		pq_win_info->st3DInfo =
		    RES_DISPLAY.stWindowInfo.st3DInfo;

		pq_win_info->stHDRInfo =
		    RES_DISPLAY.stWindowInfo.stHDRInfo;

		pq_win_info->stSetCapInfo =
		    RES_DISPLAY.stWindowInfo.stSetCapInfo;

		pq_win_info->stSetWinInfo =
		    RES_DISPLAY.stWindowInfo.stSetWinInfo;

		if (pq_win_info->u32Version > 2) {
			//check seq change status
			pq_win_info->bIsNeedSeq =
			    _display_isneedseqchange(u32WindowID,
			    &pq_win_info->pstDispFrameFormat);

			if (pq_win_info->bIsNeedSeq)
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
				    "bIsNeedSeq = %d.\n",
				    pq_win_info->bIsNeedSeq);
		}

		if (pq_win_info->u32Version > 3) {
			_display_is_flow_type_correct(&u8FlowType);
			pq_win_info->display_flow_type = u8FlowType;
		}

		if (pq_win_info->u32Version > 4) {
			if (RES_DISPLAY.u32InputFrmRate > 0) {
				pq_win_info->display_delay_time =
				    ((MTK_PQ_TIME_SEC2MSEC(1) * 1000 +
				    (RES_DISPLAY.u32InputFrmRate / 2)) /
				    (RES_DISPLAY.u32InputFrmRate)) *
				    MTK_PQ_MAX_KEEP_BUFFER;
			} else {
				pq_win_info->display_delay_time = 0;
			}
		}

		if (pq_win_info->u32Version > 5) {
			pq_win_info->u32MvopFrameRate =
			    RES_DISPLAY.u32OutputFrmRate;

			pq_win_info->s64MvopIsrTime =
			    RES_DISPLAY.s64MvopIsrTime;
		}

		if (pq_win_info->u32Version > 6) {
			__u32 u32RPtr =
			    RES_DISPLAY.buffer_ptr.read_ptr;

			if (RES_DISPLAY.buffer_ptr.read_ptr ==
			    RES_DISPLAY.buffer_ptr.write_ptr) {
				u32RPtr =
				    _display_get_pre_id(
				    RES_DISPLAY.buffer_ptr.read_ptr);
			}
			pq_win_info->u64CurrentPTS =
			    RES_DISPLAY.frame_info[u32RPtr].u64Pts;
			pq_win_info->u64CurrentPTSUs =
			    RES_DISPLAY.frame_info[u32RPtr].u64PtsUs;
		}
	}
	_display_release_semaphore(u32WindowID);
	return ret;
}

static int _display_get_windowid(__u32 *u32GetWindowID)
{
	if (u32GetWindowID == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	__u32 u32WindowID = 0;
	struct mtk_pq_window *disp_win = NULL;

	for (u32WindowID = 0; u32WindowID <
	    V4L2_MTK_MAX_WINDOW; u32WindowID++) {
		_display_obtain_semaphore(u32WindowID);
		DISPLAY_GET_RES_PRI;
		disp_win = &(RES_DISPLAY.stWindowInfo.stSetWinInfo.stDispWin);
		if (_display_is_window_used(u32WindowID) == TRUE) {
			if (pre_get_id_info.left == disp_win->u32x
			    && pre_get_id_info.top == disp_win->u32y
			    && pre_get_id_info.width == disp_win->u32width
			    && pre_get_id_info.height == disp_win->u32height) {
				*u32GetWindowID = u32WindowID;
				_display_release_semaphore(u32WindowID);
				return 0;
			}
		}
		_display_release_semaphore(u32WindowID);
	}

	u32WindowID = 0xFF;
	*u32GetWindowID = u32WindowID;

	return 0;
}


static int _display_get_frame_release_info(__u32 u32WindowID,
			struct mtk_pq_frame_release_info *pstFrame_Release_Info)
{
	if (pstFrame_Release_Info == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	memcpy(pstFrame_Release_Info, &release_info[u32WindowID],
	       sizeof(struct mtk_pq_frame_release_info));

	return 0;
}

static int _display_get_callback_info(__u32 u32WindowID,
			 struct mtk_pq_delay_info *pstDelayInfo)
{
	if (pstDelayInfo == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	bool bReport = FALSE;
	int ret = 0;

	bReport = report[u32WindowID];
	if (bReport) {
		pstDelayInfo->u32WindowID = u32WindowID;
		pstDelayInfo->u32DelayTime = delay_time[u32WindowID];
		pstDelayInfo->u64Pts = pts_ms[u32WindowID];
		pstDelayInfo->u64PtsUs = pts_us[u32WindowID];
		pstDelayInfo->s64SystemTime = system_time[u32WindowID];
		pstDelayInfo->bSubtitleStamp = check_frame_disp[u32WindowID];
		pstDelayInfo->u32DisplayCnt = display_count[u32WindowID];
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Not report!\n");
		ret = -EACCES;
	}

	return ret;
}

static int _display_get_video_delay_time(__u32 u32WindowID,
			 struct mtk_pq_delay_info *pstVideoDelayInfo)
{
	if (pstVideoDelayInfo == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	int ret  = 0;

	memcpy(pstVideoDelayInfo, &pre_get_delay_info,
	    sizeof(struct mtk_pq_delay_info));

	if (pstVideoDelayInfo->enDelayType == MTK_PQ_GET_DISPLAY_DELAY) {
		DISPLAY_GET_RES_PRI;
		pstVideoDelayInfo->u32DelayTime = RES_DISPLAY.u32VideoDelayTime;
		pstVideoDelayInfo->u32WindowID = u32WindowID;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			   "cannot support getting this delay type. type is %d\n",
			   pstVideoDelayInfo->enDelayType);
		ret = -EINVAL;
	}

	return ret;
}

int mtk_display_get_data(__u32 u32WindowID, void *ctrl)
{
	if (ctrl == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	int ret = 0;
	struct mtk_pq_display_g_data display_data;

	memset(&display_data, 0, sizeof(struct mtk_pq_display_g_data));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	switch (display_get_ctrl_type) {
	case MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWINFO:
		ret = _display_get_windowinfo(u32WindowID,
		    &display_data.data.win_info);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWID:
		ret = _display_get_windowid(&display_data.data.u32WindowID);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_FRAME_RELEASE_DATA:
		ret =
		    _display_get_frame_release_info(u32WindowID,
			    &display_data.data.stFrame_Release_Info);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_CALLBACK_DATA:
		ret = _display_get_callback_info(u32WindowID,
		    &display_data.data.stVideoDelayInfo);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_VIDEO_DELAY_TIME:
		ret = _display_get_video_delay_time(u32WindowID,
		    &display_data.data.stVideoDelayInfo);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	if (!ret)
		memcpy(ctrl, (void *)(&display_data),
		    sizeof(struct mtk_pq_display_g_data));

	display_get_ctrl_type = MTK_PQ_DISPLAY_CTRL_TYPE_MAX;

	return ret;
}

int mtk_display_set_data(__u32 u32WindowID,
			struct v4l2_ext_control *ctrl)
{
	if (ctrl == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return -EINVAL;
	}

	struct mtk_pq_display_s_data display_ctrl;
	__u8 *pu8_data = NULL;
	int ret = 0;

	DISPLAY_GET_RES_PRI;

	memset(&display_ctrl, 0, sizeof(struct mtk_pq_display_s_data));
	memcpy(&display_ctrl, ctrl, sizeof(struct mtk_pq_display_s_data));

	if (display_ctrl.param_size) {
		pu8_data = kmalloc(display_ctrl.param_size, GFP_KERNEL);

		if (pu8_data == NULL)
			return -EFAULT;

		if (copy_from_user
		    ((void *)pu8_data, (__u8 __user *) display_ctrl.p.ctrl_data,
		    display_ctrl.param_size)) {
			kfree(pu8_data);
			return -EFAULT;
		}
	}
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
	    "sid.sun::ctrl_type = %d\n", display_ctrl.ctrl_type);

	switch (display_ctrl.ctrl_type) {
	case MTK_PQ_DISPLAY_CTRL_TYPE_CREATEWINDOW:
		ret = _display_create_window(u32WindowID);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_SETWINDOW:
		ret = _display_set_window(u32WindowID,
		    (struct mtk_pq_set_window *)pu8_data);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_DESTROYWINDOW:
		ret = _display_destroy_window(u32WindowID);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_SETENSWFRC:
		ret = _display_set_swfrc(u32WindowID, (bool *) pu8_data);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWID:
		ret = _display_pre_get_win_id((struct v4l2_rect *)pu8_data);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWINFO:
		ret = _display_pre_get_win_info(
		    (struct mtk_pq_window_info *)pu8_data);
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_FRAME_RELEASE_DATA:
		ret = _display_pre_get_frame_release_info();
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_CALLBACK_DATA:
		ret = _display_pre_get_callback_info();
		break;

	case MTK_PQ_DISPLAY_CTRL_TYPE_VIDEO_DELAY_TIME:
		ret = _display_pre_get_video_delay_time(
		    (struct mtk_pq_delay_info *) pu8_data);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	kfree(pu8_data);

	return ret;
}

static void _display_init_params(
		struct mtk_pq_resource *pq_resource)
{
	if (pq_resource == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Invalid Parameters!\n");
		return;
	}

	struct mtk_pq_display_resource *resource =
		&(pq_resource->display_resource);

	resource->bStartCountFRC = FALSE;
	resource->bWinUsed = FALSE;
	resource->bFirstPlay = FALSE;
	resource->bSetMute = FALSE;
	resource->u32WindowFrameRate = 0;
	resource->bRefreshWin = FALSE;
	resource->u32PQFrameCnt = 0;
	resource->u32PQFrameCntPre = 0;
	resource->enPQDSTypePre = E_PQ_DS_TYPE_MAX;
	resource->u32XCDSUpdateConditionPre = 0;
	resource->bEnable3D = FALSE;
	resource->bXCEnableBob = FALSE;
	resource->bFieldOrderChange = FALSE;

	resource->enPreFieldOrderType = MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX;
	resource->detect_order_type = MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX;

	resource->scaling_condition = 0;
	resource->bFDMaskEnable = FALSE;
	resource->u32RWDiff = 0;
	resource->bOneFieldMode = FALSE;
	resource->bFirstSetwindow = FALSE;
	resource->bPQDSEnable = FALSE;
	resource->bPlayFlag = FALSE;
	resource->u32InputFrmRate = 0;
	resource->u32OutputFrmRate = 0;
	resource->u32AccumuleOutputRate = 0;
	resource->bIsEnFRC = FALSE;
	resource->bTriggerMVOPOutputFRC = FALSE;
	resource->bIsEnForceP = FALSE;
	resource->bIsSeqChange = FALSE;
	resource->enMVOPDropline = MTK_PQ_MVOP_DROP_LINE_DISABLE;
	resource->bSetWinImmediately = FALSE;
	resource->fast_forward_status = MTK_PQ_NORMAL;
	resource->s64MvopIsrTime = 0;
	resource->bIsCreateEmptyDsScript = FALSE;
	resource->s32CPLX = -1;
	resource->bEnableMVOPTimeStamp = FALSE;
	resource->u64TimeStamp = 0;
	resource->api_state = MTK_PQ_INIT;
	resource->bNeedResetMEMC = FALSE;
	memset(resource->au8WinPattern, 0,
		sizeof(__u8) * V4L2_MTK_MAX_PATH_IPNUM);

	memset(resource->u32ReleaseState, 0,
	       sizeof(__u32) * MTK_PQ_QUEUE_DEPTH);

	memset(resource->frame_info, 0,
	       sizeof(struct mtk_pq_vdec_frame_info) * MTK_PQ_QUEUE_DEPTH);

	memset(&resource->buffer_ptr, 0,
	       sizeof(struct mtk_pq_buffer_ptr));

	memset(&resource->stWindowInfo, 0, sizeof(struct mtk_pq_window_info));

	memset(resource->u32FlipTime, 0,
	    sizeof(__u32) * MTK_PQ_QUEUE_DEPTH);

	memset(resource->u32DelayTime, 0,
	       sizeof(__u32) * MTK_PQ_QUEUE_DEPTH);

	memset(&resource->stDSInfo, 0, sizeof(struct mtk_pq_ds_info));
	memset(&resource->stUserSetwinInfo, 0,
			sizeof(struct mtk_pq_set_win_info));

}

int mtk_display_init(void)
{
	if (display_inited) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "DISPLAY has been inited already!\n");
		return 0;
	}

	int i;
	__u32 u32WindowID = 0;
	__u32 u32BufferID = 0;

	for (i = 0; i < V4L2_MTK_MAX_WINDOW; i++) {
		display_resource[i] = vmalloc(sizeof(struct mtk_pq_resource));
		_display_init_params(display_resource[i]);
		sema_init(&display_semaphore[i], 1);
	}
	common_resource =
	    vmalloc(sizeof(struct mtk_pq_common_resource));

	sema_init(&display_semaphore[MTK_PQ_COMMON_SEMAPHORE], 1);

	common_resource->bForceOffPQDS = FALSE;
	common_resource->bIsForcePDebugMode = FALSE;

	common_resource->enProcfsForceSwFrc =
	    MTK_PQ_FORCE_SWFRC_DEFAULT;

	common_resource->u32ProcfsForceSwFrcFrameRate = 0;
	common_resource->bProcfsFlipTrigEvent = TRUE;
	common_resource->bForceDTVDSOff = FALSE;
	common_resource->bPerframeUpdateXCSetting = TRUE;
	common_resource->bForceAllSourceDSOff = FALSE;

	for (u32WindowID = 0;
	    u32WindowID < V4L2_MTK_MAX_WINDOW; u32WindowID++) {
		DISPLAY_GET_RES_PRI;
		_display_obtain_semaphore(u32WindowID);
		for (u32BufferID = 0; u32BufferID <
		    MTK_PQ_QUEUE_DEPTH; u32BufferID++) {
			RES_DISPLAY.frame_info[u32BufferID].bValid = FALSE;

			RES_DISPLAY.frame_info[u32BufferID].u32FrameIndex =
			    MTK_PQ_INVALID_FRAME_ID;

			RES_DISPLAY.frame_info[u32BufferID].enScanType =
			    MTK_PQ_VIDEO_SCAN_TYPE_MAX;

			RES_DISPLAY.u8LocalFrameRefCount[u32BufferID] = 0;
		}
		_display_release_semaphore(u32WindowID);
	}

	init_waitqueue_head(&mvop_task);

	display_inited = TRUE;

	return 0;
}

int mtk_display_set_inputsource(__u32 u32WindowID, __u32 InputSrcType)
{
	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	DISPLAY_GET_RES_PRI;
	RES_DISPLAY.enInputSrcType = InputSrcType;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"InputSrcType = %d\n", InputSrcType);

	if (IS_INPUT_MVOP(RES_DISPLAY.enInputSrcType))
		RES_DISPLAY.bEnableMVOPTimeStamp = TRUE;
	else
		RES_DISPLAY.bEnableMVOPTimeStamp = FALSE;

	return 0;
}

int mtk_display_set_digitalsignalinfo(__u32 u32WindowID,
			struct mtk_pq_device *pq_dev)
{
	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	DISPLAY_GET_RES_PRI;
	RES_DISPLAY.bTriggerMVOPOutputFRC = TRUE;
	RES_DISPLAY.bFirstPlay = TRUE;
	RES_DISPLAY.bXCEnableBob = FALSE;
	RES_DISPLAY.bFieldOrderChange = FALSE;

	RES_DISPLAY.detect_order_type =
	    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX;

	RES_DISPLAY.enPreFieldOrderType =
	    MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX;

	RES_DISPLAY.bFDMaskEnable = FALSE;
	RES_DISPLAY.bFirstSetwindow = TRUE;
	RES_DISPLAY.bPQDSEnable = TRUE;
	RES_DISPLAY.bPlayFlag = TRUE;
	RES_DISPLAY.bRefreshWin = FALSE;
	RES_DISPLAY.bUpdateSetWin = FALSE;
	RES_DISPLAY.bSetWinImmediately = FALSE;
	RES_DISPLAY.fast_forward_status = MTK_PQ_NORMAL;
	RES_DISPLAY.scaling_condition = 0;
	RES_DISPLAY.u32AccumuleOutputRate = 0;
	RES_DISPLAY.u32VideoDelayTime = 0;

	pre_field_type = MTK_PQ_FIELD_TYPE_MAX;
	ignore_cur_field = FALSE;
	is_even_field = FALSE;
	is_fired = FALSE;
	delay_time[u32WindowID] = 0;
	pts_ms[u32WindowID] = 0;
	pts_us[u32WindowID] = 0;
	system_time[u32WindowID] = 0;
	report[u32WindowID] = FALSE;
	check_frame_disp[u32WindowID] = FALSE;
	display_count[u32WindowID] = 0;
	task_cnt = 0;
	total_time = 0;
	pre_HDR_type = E_XC_HDR_TYPE_MAX;

	__u32 u32Idex = 0;

	for (u32Idex = 0; u32Idex < MTK_PQ_QUEUE_DEPTH; u32Idex++) {
		RES_DISPLAY.u32FlipTime[u32Idex] = 0;
		RES_DISPLAY.u32DelayTime[u32Idex] = 0;
	}

	memset(RES_DISPLAY.au32VideoFlipTime, 0,
		sizeof(RES_DISPLAY.au32VideoFlipTime));

	memset(RES_DISPLAY.abVideoFlip, 0, sizeof(RES_DISPLAY.abVideoFlip));

	memset(&hal_delay_time, 0,
	    sizeof(struct mtk_pq_measure_time) * MEASURE_TIME_MAX_NUM);

	memset(video_delay_time, 0, sizeof(video_delay_time));


	__u32 u32tmpRet = 0;

	if (!(MApi_XC_GetChipCaps(E_XC_SUPPORT_SPF_LB_MODE,
	    &u32tmpRet, sizeof(__u32))))
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "GetChipCaps Fail !\n");

	support_SPFLB = ((u32tmpRet != 0) ? TRUE : FALSE);

	is_digital_cleared = FALSE;

	if (pq_dev->common_info.input_source == MTK_PQ_INPUTSRC_STORAGE ||
		pq_dev->common_info.input_source == MTK_PQ_INPUTSRC_DTV){

		/* fixme start */
		MDrv_MVOP_Init();
		MDrv_MVOP_Enable(TRUE);
		/* fixme end */
		MVOP_Handle stMvopHd = { E_MVOP_MODULE_MAIN };
		__u32 u32IntType = E_MVOP_INT_DC_START;

		if (MDrv_MVOP_SetCommand(&stMvopHd,
			E_MVOP_CMD_SET_DMS_INT, &u32IntType) != 0)
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"[DISPLAY]E_MVOP_CMD_SET_DMS_INT failed!\n");

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"DISPLAY: IRQ num. = %d\n", pq_dev->b2r_dev.irq);
		enable_irq(pq_dev->b2r_dev.irq);
	}

	return 0;
}

int mtk_display_clear_digitalsignalinfo(__u32 u32WindowID,
			struct mtk_pq_device *pq_dev)
{
	drv_delay_time[MEASURE_TIME_CLEAR_DIGITAL].u32StartTime =
	    _display_get_time();

	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	if ((u32WindowID == MVOP_WINDOW) && (_display_is_ds_enable() == TRUE))
		_display_set_ds_on_off(u32WindowID, FALSE);

	struct mtk_pq_bob_mode_info stBOBModeInfo;

	memset(&stBOBModeInfo, 0, sizeof(struct mtk_pq_bob_mode_info));
	stBOBModeInfo.bResetBobMode = TRUE;
	_display_set_bob_mode(u32WindowID, stBOBModeInfo);

	DISPLAY_GET_RES_PRI;
	if (pq_dev->common_info.input_source == MTK_PQ_INPUTSRC_STORAGE ||
		pq_dev->common_info.input_source == MTK_PQ_INPUTSRC_DTV){
		RES_DISPLAY.buffer_ptr.read_ptr = 0;
		RES_DISPLAY.buffer_ptr.write_ptr = 0;
		__u32 u32BufID = 0;

		for (u32BufID = 0; u32BufID < MTK_PQ_QUEUE_DEPTH; u32BufID++)
			_display_clear_source_frame_ref(u32WindowID,
				u32BufID, pq_dev);

		memset(RES_DISPLAY.frame_info, 0x0,
			sizeof(struct mtk_pq_vdec_frame_info) *
				MTK_PQ_QUEUE_DEPTH);

		for (u32BufID = 0; u32BufID < MTK_PQ_QUEUE_DEPTH; u32BufID++) {
			RES_DISPLAY.frame_info[u32BufID].u32FrameIndex =
				MTK_PQ_INVALID_FRAME_ID;

				RES_DISPLAY.frame_info[u32BufID].enScanType =
					MTK_PQ_VIDEO_SCAN_TYPE_MAX;

				RES_DISPLAY.frame_info[u32BufID].bValid = FALSE;
				RES_DISPLAY.u32ReleaseState[u32BufID] = 0;
			}

		RES_DISPLAY.bPlayFlag = FALSE;
		RES_DISPLAY.bIsSeqChange = FALSE;
		RES_DISPLAY.bIsXCEnFBL = FALSE;
		is_digital_cleared = TRUE;
		disable_irq(pq_dev->b2r_dev.irq);
	}
	struct v4l2_event ev;

	memset(&ev, 0, sizeof(struct v4l2_event));
	ev.type = V4L2_EVENT_MTK_PQ_STREAMOFF;
	v4l2_event_queue(&pq_dev->video_dev, &ev);

	return 0;
}

int mtk_display_qbuf(struct mtk_pq_device *pq_dev, struct vb2_buffer *vb)
{
	int ret = 0;
	__u32 u32WindowID = pq_dev->dev_indx;
	__u32 share_fd = vb->planes[0].m.fd;
	__u32 meta_fd = vb->planes[1].m.fd;
	void *meta_addr = NULL;
	struct mtk_pq_frame_info stFrameFormat;

	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Buffer Type = %d!\n", vb->type);

	if (vb->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"Skip Queue CAPTURE_MPLANE Buffer!\n");
		goto exit;
	}

	memset(&stFrameFormat, 0, sizeof(struct mtk_pq_frame_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"u32Index = %d, frame_fd = %d, meta_fd = %d\n",
		vb->index, share_fd, meta_fd);

	ret = mtk_pq_common_read_metadata(meta_fd,
		EN_PQ_METATAG_SH_FRM_INFO, (void *)&stFrameFormat);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_read_metadata failed!!\n");
		goto exit;
	}

	_display_video_flip(pq_dev, vb,
		u32WindowID, &stFrameFormat);

	return 0;

exit:
	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

	struct v4l2_event ev;

	memset(&ev, 0, sizeof(struct v4l2_event));

	if (V4L2_TYPE_IS_OUTPUT(vb->type))
		ev.type = V4L2_EVENT_MTK_PQ_INPUT_DONE;
	else
		ev.type = V4L2_EVENT_MTK_PQ_OUTPUT_DONE;

	ev.u.data[0] = vb->index;
	v4l2_event_queue(&pq_dev->video_dev, &ev);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"display qbuf failed!!Buffer Type = %d\n",
		vb->type);

	return -EFAULT;
}


int mtk_display_create_task(struct mtk_pq_device *pq_dev)
{
	if (pq_dev->dev_indx >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	if (pq_dev->display_info.mvop_task == NULL) {
		char name[20];
		char *buffer = "mvop_task";

		snprintf(name, sizeof(name), "%s%02d",
		    buffer, pq_dev->dev_indx);
		pq_dev->display_info.mvop_task =
		    kthread_run(_display_set_mvop_task, pq_dev, name);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "create task::%s\n", name);
	}

	return 0;
}

int mtk_display_destroy_task(struct mtk_pq_device *pq_dev)
{
	if (pq_dev->dev_indx >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	int ret = 0;

	if (pq_dev->display_info.mvop_task) {
		ret = kthread_stop(pq_dev->display_info.mvop_task);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Destroy task::mvop_task%02d\n",
			   pq_dev->dev_indx);
		pq_dev->display_info.mvop_task = NULL;
	}

	return ret;
}

int mtk_display_subscribe_event(__u32 event_type, __u16 *dev_id)
{
	struct mtk_pq_device *pq_dev = container_of(dev_id,
	    struct mtk_pq_device, dev_indx);

	if (pq_dev->dev_indx >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
	    "subscribe event type = %d\n", event_type);

	switch (event_type) {
	case V4L2_EVENT_MTK_PQ_INPUT_DONE:
		{
			pq_dev->display_info.bReleaseTaskCreated = TRUE;
			break;
		}
	case V4L2_EVENT_MTK_PQ_CALLBACK:
		{
			pq_dev->display_info.bCallBackTaskCreated = TRUE;
			break;
		}
	case V4L2_EVENT_MTK_PQ_STREAMOFF:
	case V4L2_EVENT_MTK_PQ_OUTPUT_DONE:
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

int mtk_display_unsubscribe_event(__u32 event_type, __u16 *dev_id)
{
	struct mtk_pq_device *pq_dev = container_of(dev_id,
	    struct mtk_pq_device, dev_indx);

	if (pq_dev->dev_indx >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
	    "unsubscribe event type = %d\n", event_type);

	switch (event_type) {
	case V4L2_EVENT_MTK_PQ_INPUT_DONE:
		{
			pq_dev->display_info.bReleaseTaskCreated = FALSE;
			break;
		}
	case V4L2_EVENT_MTK_PQ_CALLBACK:
		{
			pq_dev->display_info.bCallBackTaskCreated = FALSE;
			break;
		}
	case V4L2_EVENT_MTK_PQ_STREAMOFF:
	case V4L2_EVENT_MTK_PQ_OUTPUT_DONE:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int mtk_display_set_forcep(__u32 u32WindowID, bool enable)
{
	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	DISPLAY_GET_RES_PRI;
	RES_DISPLAY.bIsEnForceP = enable;

	return 0;
}

int mtk_display_set_xcenfbl(__u32 u32WindowID, bool enable)
{
	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	DISPLAY_GET_RES_PRI;
	RES_DISPLAY.bIsXCEnFBL = enable;

	return 0;
}

int mtk_display_set_outputfrmrate(__u32 u32WindowID, __u32 u32OutputFrmRate)
{
	if (u32WindowID >= V4L2_MTK_MAX_WINDOW) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		    "Invalid WindowID!!WindowID = %d\n", u32WindowID);
		return -EINVAL;
	}

	DISPLAY_GET_RES_PRI;
	RES_DISPLAY.u32OutputFrmRate = u32OutputFrmRate;

	return 0;
}

INPUT_SOURCE_TYPE_t mtk_display_transfer_input_source(
			enum mtk_pq_input_source_type enInputSrc)
{
	INPUT_SOURCE_TYPE_t enXCInputSrcType = INPUT_SOURCE_NONE;

	switch (enInputSrc) {
		//vga
	case MTK_PQ_INPUTSRC_VGA:
		enXCInputSrcType = INPUT_SOURCE_VGA;
		break;

		//tv
	case MTK_PQ_INPUTSRC_TV:
		enXCInputSrcType = INPUT_SOURCE_TV;
		break;

		//cvbs
	case MTK_PQ_INPUTSRC_CVBS:
		enXCInputSrcType = INPUT_SOURCE_CVBS;
		break;

	case MTK_PQ_INPUTSRC_CVBS2:
		enXCInputSrcType = INPUT_SOURCE_CVBS2;
		break;

	case MTK_PQ_INPUTSRC_CVBS3:
		enXCInputSrcType = INPUT_SOURCE_CVBS3;
		break;

	case MTK_PQ_INPUTSRC_CVBS4:
		enXCInputSrcType = INPUT_SOURCE_CVBS4;
		break;

	case MTK_PQ_INPUTSRC_CVBS5:
		enXCInputSrcType = INPUT_SOURCE_CVBS5;
		break;

	case MTK_PQ_INPUTSRC_CVBS6:
		enXCInputSrcType = INPUT_SOURCE_CVBS6;
		break;

	case MTK_PQ_INPUTSRC_CVBS7:
		enXCInputSrcType = INPUT_SOURCE_CVBS7;
		break;

	case MTK_PQ_INPUTSRC_CVBS8:
		enXCInputSrcType = INPUT_SOURCE_CVBS8;
		break;

	case MTK_PQ_INPUTSRC_CVBS_MAX:
		enXCInputSrcType = INPUT_SOURCE_CVBS_MAX;
		break;

		//svideo
	case MTK_PQ_INPUTSRC_SVIDEO:
		enXCInputSrcType = INPUT_SOURCE_SVIDEO;
		break;

	case MTK_PQ_INPUTSRC_SVIDEO2:
		enXCInputSrcType = INPUT_SOURCE_SVIDEO2;
		break;

	case MTK_PQ_INPUTSRC_SVIDEO3:
		enXCInputSrcType = INPUT_SOURCE_SVIDEO3;
		break;

	case MTK_PQ_INPUTSRC_SVIDEO4:
		enXCInputSrcType = INPUT_SOURCE_SVIDEO4;
		break;

	case MTK_PQ_INPUTSRC_SVIDEO_MAX:
		enXCInputSrcType = INPUT_SOURCE_SVIDEO_MAX;
		break;

		//ypbpr
	case MTK_PQ_INPUTSRC_YPBPR:
		enXCInputSrcType = INPUT_SOURCE_YPBPR;
		break;

	case MTK_PQ_INPUTSRC_YPBPR2:
		enXCInputSrcType = INPUT_SOURCE_YPBPR2;
		break;

	case MTK_PQ_INPUTSRC_YPBPR3:
		enXCInputSrcType = INPUT_SOURCE_YPBPR3;
		break;

	case MTK_PQ_INPUTSRC_YPBPR_MAX:
		enXCInputSrcType = INPUT_SOURCE_YPBPR_MAX;
		break;

		//scart
	case MTK_PQ_INPUTSRC_SCART:
		enXCInputSrcType = INPUT_SOURCE_SCART;
		break;

	case MTK_PQ_INPUTSRC_SCART2:
		enXCInputSrcType = INPUT_SOURCE_SCART2;
		break;

	case MTK_PQ_INPUTSRC_SCART_MAX:
		enXCInputSrcType = INPUT_SOURCE_SCART_MAX;
		break;

		//hdmi
	case MTK_PQ_INPUTSRC_HDMI:
		enXCInputSrcType = INPUT_SOURCE_HDMI;
		break;

	case MTK_PQ_INPUTSRC_HDMI2:
		enXCInputSrcType = INPUT_SOURCE_HDMI2;
		break;

	case MTK_PQ_INPUTSRC_HDMI3:
		enXCInputSrcType = INPUT_SOURCE_HDMI3;
		break;

	case MTK_PQ_INPUTSRC_HDMI4:
		enXCInputSrcType = INPUT_SOURCE_HDMI4;
		break;

	case MTK_PQ_INPUTSRC_HDMI_MAX:
		enXCInputSrcType = INPUT_SOURCE_HDMI_MAX;
		break;

		//dvi
	case MTK_PQ_INPUTSRC_DVI:
		enXCInputSrcType = INPUT_SOURCE_DVI;
		break;

	case MTK_PQ_INPUTSRC_DVI2:
		enXCInputSrcType = INPUT_SOURCE_DVI2;
		break;

	case MTK_PQ_INPUTSRC_DVI3:
		enXCInputSrcType = INPUT_SOURCE_DVI3;
		break;

	case MTK_PQ_INPUTSRC_DVI4:
		enXCInputSrcType = INPUT_SOURCE_DVI4;
		break;

	case MTK_PQ_INPUTSRC_DVI_MAX:
		enXCInputSrcType = INPUT_SOURCE_DVI_MAX;
		break;

		//dtv
	case MTK_PQ_INPUTSRC_DTV:
		enXCInputSrcType = INPUT_SOURCE_DTV;
		break;

	case MTK_PQ_INPUTSRC_DTV2:
		enXCInputSrcType = INPUT_SOURCE_DTV2;
		break;

	case MTK_PQ_INPUTSRC_DTV_MAX:
		enXCInputSrcType = INPUT_SOURCE_NONE;
		break;

		//storage
	case MTK_PQ_INPUTSRC_STORAGE:
		enXCInputSrcType = INPUT_SOURCE_STORAGE;
		break;

	case MTK_PQ_INPUTSRC_STORAGE2:
		enXCInputSrcType = INPUT_SOURCE_STORAGE2;
		break;

	case MTK_PQ_INPUTSRC_STORAGE_MAX:
		enXCInputSrcType = INPUT_SOURCE_NONE;
		break;

		//op capture
	case MTK_PQ_INPUTSRC_SCALER_OP:
		enXCInputSrcType = INPUT_SOURCE_SCALER_OP;
		break;

	default:
		enXCInputSrcType = INPUT_SOURCE_NONE;
		break;
	}

	return enXCInputSrcType;
}
