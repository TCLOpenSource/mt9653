/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Kevin Ren <kevin.ren@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DRV_DMS_H_
#define _DRV_DMS_H_

#include <linux/timekeeping.h>
#include <linux/videodev2.h>
#include <uapi/linux/mtk-v4l2-pq.h>

#include "UFO.h"

#include "apiXC.h"

//----------------------------------------------------------------------------
//  Driver Capability
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//  Macro and Define
//----------------------------------------------------------------------------

#define DMS_BUFFER_QUEUEDEPTH          (8)
#define DMS_MAX_KEEP_BUFFER            (3)
#define DMS_INVALID_FRAME_ID           (0xFF)
#define VERSION_ST_DMS_DISPFRAMEFORMAT      (10)
#define DMS_FLIP_TIMEOUT_THRESHOLD      (200)
#define SOURCE_FRAME_KEEP_NUM (2)
#define MAX_PQDS_FRAMECOUNT (20)
#define DMS_INIT_FOD_WIDTH (1)
#define DMS_INIT_FOD_HEIGHT (1)
#define DMS_FLOW_TYPE (1)
#define UNDERFLOW_X_REPEAT (2)
#define DMS_MAIN_WINDOW_FLOW_VERSION (0)

#define CFD_PLUS_VERSION           (1)
#define FRAME_RATE16          (16000)
#define FRAME_RATE21          (21000)
#define FRAME_RATE24_5        (24500)
#define FRAME_RATE26          (26000)
#define FRAME_RATE40          (40000)
#define FRAME_RATE47          (47000)
#define FRAME_RATE49          (49000)
#define FRAME_RATE51          (51000)
#define HAL_MIU1_BASE         0xFFFFFFFFFFFFFFFFUL
#define HAL_MIU2_BASE         0xFFFFFFFFFFFFFFFFUL
// no MIU2 BASE  fix me ---sid
#define TICK_PER_ONE_MS     (1)
#define MSOS_WAIT_FOREVER   (0xffffff00/TICK_PER_ONE_MS)
#define DMS_EXPECTED_TASKPROCESSTIME (5000)


#define DMS_SWDRHISTOGRAM_INDEX             (32)

#define IS_SRC_STORAGE(x)    (((x) == E_DMS_INPUTSRC_STORAGE) ||\
				((x) == E_DMS_INPUTSRC_STORAGE2))
#define IS_SRC_DTV(x)        (((x) == E_DMS_INPUTSRC_DTV) ||\
				((x) == E_DMS_INPUTSRC_DTV2))
#define IS_SRC_HDMI(x)       (((x) >= E_DMS_INPUTSRC_HDMI) &&\
				((x) < E_DMS_INPUTSRC_HDMI_MAX))
#define IS_INPUT_MVOP(x)     (IS_SRC_DTV(x) || IS_SRC_STORAGE(x))
#define MVOP_WINDOW 0

#define IS_HDR_PER_FRAME(x) (x & (E_DMS_HDR_METADATA_DOLBY_HDR +\
				E_DMS_HDR_METADATA_TCH +\
				E_DMS_HDR_METADATA_HDR10_PER_FRAME))
#define DMS_NSEC_PER_USEC (1)

#define DMS_GET_STANDARD_TIME(x) getrawmonotonic(&x)

#define DMS_GET_SYSTEM_TIME(x) do { \
	struct timespec stSystemTime; \
	DMS_GET_STANDARD_TIME(stSystemTime); \
	x = (__s64)stSystemTime.tv_sec * 1000000000 + \
		(__s64)stSystemTime.tv_nsec; \
	} while (0)

//----------------------------------------------------------------------------
//  enums
//----------------------------------------------------------------------------

enum E_DMS_VIDEO_CODEC {
	E_DMS_VIDEO_CODEC_UNKNOWN,
	E_DMS_VIDEO_CODEC_MPEG2,
	E_DMS_VIDEO_CODEC_H263,
	E_DMS_VIDEO_CODEC_MPEG4,
	E_DMS_VIDEO_CODEC_DIVX311,
	E_DMS_VIDEO_CODEC_DIVX412,
	E_DMS_VIDEO_CODEC_FLV,
	E_DMS_VIDEO_CODEC_VC1_AVD,
	E_DMS_VIDEO_CODEC_VC1_MAIN,
	E_DMS_VIDEO_CODEC_RV8,
	E_DMS_VIDEO_CODEC_RV9,
	E_DMS_VIDEO_CODEC_H264,
	E_DMS_VIDEO_CODEC_AVS,
	E_DMS_VIDEO_CODEC_MJPEG,
	E_DMS_VIDEO_CODEC_MVC,
	E_DMS_VIDEO_CODEC_VP8,
	E_DMS_VIDEO_CODEC_HEVC,
	E_DMS_VIDEO_CODEC_VP9,
	E_DMS_VIDEO_CODEC_HEVC_DV,
	E_DMS_VIDEO_CODEC_H264_DV,
	E_DMS_VIDEO_CODEC_NUM
};

enum EN_DMS_PQDS_ACTION {
	E_DMS_PQDS_GET_PQSETTING,
	E_DMS_PQDS_SET_PQSETTING,
	E_DMS_PQDS_CHECKSTATUS,
	E_DMS_PQDS_ENABLE_CHECK,
	E_DMS_PQDS_UPDATEINDEX,
};

enum E_DMS_FRC_CONTROL {
	E_DMS_FRC_NONE = 0,
	E_DMS_FRC_DROP,
	E_DMS_FRC_REPEAT,
	E_DMS_FRC_MAX,
};

enum E_DMS_VIDEO_FIELD_ORDER_TYPE {
	E_DMS_VIDEO_FIELD_ORDER_TYPE_BOTTOM = 0,
	E_DMS_VIDEO_FIELD_ORDER_TYPE_TOP,
	E_DMS_VIDEO_FIELD_ORDER_TYPE_MAX,
};

enum E_DMS_FIELD_TYPE {
	E_DMS_VIDEO_FIELD_TYPE_NONE = 0,
	E_DMS_VIDEO_FIELD_TYPE_TOP,
	E_DMS_VIDEO_FIELD_TYPE_BOTTOM,
	E_DMS_VIDEO_FIELD_TYPE_BOTH,
	E_DMS_VIDEO_FIELD_TYPE_MAX,
};

enum E_DMS_TILE_MODE {
	E_DMS_VIDEO_TILE_MODE_NONE = 0,
	E_DMS_VIDEO_TILE_MODE_16x16,
	E_DMS_VIDEO_TILE_MODE_16x32,
	E_DMS_VIDEO_TILE_MODE_32x16,
	E_DMS_VIDEO_TILE_MODE_32x32,
	E_DMS_VIDEO_TILE_MODE_4x2_COMPRESSION_MODE,
	E_DMS_VIDEO_TILE_MODE_8x1_COMPRESSION_MODE,
	E_DMS_VIDEO_TILE_MODE_TILE_32X16_82PACK,
	E_DMS_VIDEO_TILE_MODE_MAX,
};

enum E_DMS_MFDEC_VP9_MODE {
	E_DMS_DUMMY_VIDEO_H26X_MODE = 0x00,
	E_DMS_DUMMY_VIDEO_VP9_MODE = 0x01,
};


enum EN_DMS_DS_MANAUL_STATE {
	E_DMS_DS_MANAUL_ON = 0,
	E_DMS_DS_MANAUL_OFF,
	E_DMS_DS_MAX,
};

enum EN_DMS_API_STATE {
	E_DMS_INIT = 0,
	E_DMS_CREATE_WINDOW,
	E_DMS_SETDIGITAL,
	E_DMS_SETWINDOW,
	E_DMS_CLEARDIGITAL_DESTROY_WINDOW,
};

enum EN_DMS_FORCE_SWFRC {
	E_DMS_FORCE_SWFRC_DEFAULT = 0,
	E_DMS_FORCE_SWFRC_ON,
	E_DMS_FORCE_SWFRC_OFF,
};

enum E_DMS_MEASURE_TIME {
	E_DMS_MEASURE_TIME_ISR = 0,
	E_DMS_MEASURE_TIME_RELEASE_BUF,
	E_DMS_MEASURE_TIME_SETWINDOW_DS,
	E_DMS_MEASURE_TIME_TASK,
	E_DMS_MEASURE_TIME_DOLBY_DS,
	E_DMS_MEASURE_TIME_SWDR,
	E_DMS_MEASURE_TIME_EVENT,
	E_DMS_MEASURE_TIME_CREATE_WIN,
	E_DMS_MEASURE_TIME_SET_WIN,
	E_DMS_MEASURE_TIME_DESTROY_WIN,
	E_DMS_MEASURE_TIME_SET_DIGITAL,
	E_DMS_MEASURE_TIME_CLEAR_DIGITAL,
	E_DMS_MEASURE_TIME_FLIP,
	E_DMS_MEASURE_TIME_FLIP_INTERVAL,
	E_DMS_MEASURE_TIME_IP_ISR,
	E_DMS_MEASURE_TIME_IP_EVENT,
	E_DMS_MEASURE_TIME_IP_TASK,
	E_DMS_MEASURE_TIME_MAX_NUM,
};

enum E_DMS_MEASURE_TIME_OFFEST {
	E_DMS_UPPER_4_MS = 0,
	E_DMS_UPPER_6_MS,
	E_DMS_UPPER_8_MS,
	E_DMS_UPPER_10_MS,
	E_DMS_UPPER_MAX,
};

enum EN_DMS_SEQ_CHANGE_TYPE {
	E_DMS_SEQ_CHANGE_NONE = 0,
	E_DMS_SEQ_CHANGE_TIMING = 0x01,
	E_DMS_SEQ_CHANGE_FRAMERATE = 0x02,
	E_DMS_SEQ_CHANGE_IP = 0x04,
	E_DMS_SEQ_CHANGE_ASPECTRATE = 0x08,
	E_DMS_SEQ_CHANGE_MAX = 0xFF,
};

enum EN_DMS_FASTFORWARE_STATUS {
	EN_DMS_NORMAL,
	EN_DMS_PQDS_LOADZERO,
	EN_DMS_PQDS_LOADZERO_DONE,
	EN_DMS_RESUME,
};

enum EN_DMS_FIELD_CHANGE {
	EN_DMS_FIELD_CHANGE_DISABLE = 0,
	EN_DMS_FIELD_CHANGE_FIRST_FIELD,
	EN_DMS_FIELD_CHANGE_SECOND_FIELD,
	EN_DMS_FIELD_CHANGE_MAX_NUM,
};

enum EN_DMS_HDRMetaType {
	E_DMS_HDR_METATYPE_MPEG_VUI = (0x1 << 0),
	E_DMS_HDR_METADATA_MPEG_SEI_MASTERING_COLOR_VOLUME = (0x1 << 1),
	E_DMS_HDR_METADATA_DOLBY_HDR = (0x1 << 2),
	E_DMS_HDR_METADATA_TCH = (0x1 << 3),
	E_DMS_HDR_METADATA_HDR10_PER_FRAME = (0x1 << 4),
	E_DMS_HDR_METATYPE_CONTENT_LIGHT = (0x1 << 5),
	E_DMS_HDR_METATYPE_DYNAMIC = (0x1 << 6),
};

//----------------------------------------------------------------------------
//  structs
//----------------------------------------------------------------------------

struct DMS_MFDEC_VER_CRL {
	__u32 u32version;
	__u32 u32size;
};

struct DMS_MFDEC_HTLB_INFO {
	struct DMS_MFDEC_VER_CRL stMFDec_VerCtl;
	__u64 phyHTLBEntriesAddr;
	__u8 u8HTLBEntriesSize;
	__u8 u8HTLBTableId;
	void *pHTLBInfo;
#ifndef __aarch64__
	__u32 dummy;
#endif
};

struct DMS_MFDEC_INFO {
	bool bMFDec_Enable;
	__u8 u8MFDec_Select;
	bool bHMirror;
	bool bVMirror;
	bool bUncompress_mode;
	bool bBypass_codec_mode;
	enum E_DMS_MFDEC_VP9_MODE enMFDecVP9_mode;
	__u16 u16StartX;
	__u16 u16StartY;
	__u64 phyBitlen_Base;
	__u16 u16Bitlen_Pitch;
	__u8 u8Bitlen_MiuSelect;
	struct DMS_MFDEC_HTLB_INFO stMFDec_HTLB_Info;
	void *pMFDecInfo;
#ifndef __aarch64__
	__u32 dummy;
#endif
	bool bAV1_mode;
	bool bAV1PpuBypassMode;
	bool bMonoChrome;
};

struct ST_DMS_VDECFRAME_INFO {
	bool bValid;
	__u16 u16SrcWidth;
	__u16 u16SrcHeight;
	__u16 u16CropRight;
	__u16 u16CropLeft;
	__u16 u16CropBottom;
	__u16 u16CropTop;
	__u16 u16SrcPitch;
	__u64 phySrcLumaAddr;
	__u64 phySrcChromaAddr;
	__u64 phySrcLumaAddr_2bit;
	__u64 phySrcChromaAddr_2bit;
	__u64 phySrcLumaAddrI;
	__u64 phySrcLumaAddrI_2bit;
	__u64 phySrcChromaAddrI;
	__u64 phySrcChromaAddrI_2bit;
	__u16 u16Src10bitPitch;
	bool b10bitData;
	__u8 u8LumaBitdepth;
	__u32 u32FrameRate;
	enum E_DMS_VIDEO_DATA_FMT enFmt;
	__u32 u32Window;
	enum E_DMS_VIDEO_CODEC enCODEC;
	enum E_DMS_VIDEO_SCAN_TYPE enScanType;
	enum E_DMS_VIDEO_FIELD_ORDER_TYPE enFieldOrderType;
	enum E_DMS_FIELD_TYPE enFieldType;
	enum E_DMS_FIELD_TYPE enFieldType_2nd;
	enum E_DMS_TILE_MODE enTileMode;
	struct DMS_MFDEC_INFO stMFdecInfo;
	__u32 u32FrameIndex;
	__u32 u32FrameIndex_2nd;
	__u32 u32VDECStreamID;
	__u32 u32VDECStreamVersion;
	__u32 u32PriData;
	__u32 u32PriData_2nd;
	__u32 u32FrameNum;
	__u64 u64Pts;
	bool bCheckFrameDisp;
	__u8 u8ApplicationType;
	__u8 u8AspectRate;
	__u64 phySrc2ndBufferLumaAddr;
	__u64 phySrc2ndBufferChromaAddr;
	__u16 u16Src2ndBufferPitch;
	__u8 u8Src2ndBufferTileMode;
	__u8 u8Src2ndBufferV7DataValid;
	__u16 u16Src2ndBufferWidth;
	__u16 u16Src2ndBufferHeight;
	__u8 u83DMode;

	bool bIs2ndField;
	__u8 u8DiOutputRingBufferID;
	__u8 u8ContinuousTime;
	bool bReleaseBuf;
	bool bIsFlipTrigFrame;

	struct ST_DMS_HDR_FRAME_INFO stHDRInfo;

	__u8 u8FieldCtrl;

	enum EN_DMS_FRAMETYPE enFrameType;

	__u32 u32DisplayCounts;

	__u32 u32TaskTrigCounts;

	bool bUserSetwinFlag;
	struct ST_DMS_WINDOW stUserXCCaptWin;
	struct ST_DMS_WINDOW stUserXCDispWin;
	struct ST_DMS_WINDOW stUserXCCropWin;
	enum EN_DMS_ASPECT_RATIO enUserArc;
	__u64 u64PtsUs;
	struct vb2_buffer *vb;
};

struct ST_DMS_BufferWriteReadPointer {
	__u16 u16WritePointer;
	__u16 u16ReadPointer;
};

struct ST_DMS_DS_INFO {
	__u64 phyDynScalingAddr;
	__u32 u32DynScalingSize;
	bool bEnableMIUSel;
	__u8 u8DynScalingDepth;
	enum EN_DMS_DS_MANAUL_STATE enDsManualState;
};


struct ST_DRV_DMS_SECURE {
	bool bSecurityEnabled[DMS_MAX_WINDOW_NUM];
	__u32 u32PipeID[DMS_MAX_WINDOW_NUM];
};

struct ST_DRV_DMS_BOB_MODE_INFO {
	__u32 u32WindowID;
	__u32 u32CurReadPointer;
	bool bPauseStage;
	bool bResetBobMode;
};

struct ST_DRV_DMS {
	struct ST_DMS_WINDOW_INFO stWindowInfo;
	struct ST_DMS_BufferWriteReadPointer stBufferRWPointer;
	__u32 u32WindowFrameRate;
	__u32 u32DMS_ScalingCondition;
	struct ST_DMS_WINDOW stXCCaptWinInfo;
	bool bStartCountFRC;
	struct ST_DMS_VDECFRAME_INFO stFrameBufferInfo[DMS_BUFFER_QUEUEDEPTH];
	__u8 u8LocalFrameRefCount[DMS_BUFFER_QUEUEDEPTH];
	bool bWinUsed;
	__u32 u32ReleaseState[DMS_BUFFER_QUEUEDEPTH];
	bool bFirstPlay;
	bool bSetMute;
	__u32 u32MuteCounter;
	__u8 au8WinPattern[DMS_MAX_PATH_IPNUM];
	bool bRefreshWin;
	bool bUpdateSetWin;
	__u32 u32InputFrmRate;
	__u32 u32OutputFrmRate;
	__u32 u32AccumuleOutputRate;
	bool bIsEnFRC;
	bool bIsXCEnFBL;
	bool bIsEnForceP;
	bool bTriggerMVOPOutputFRC;
	bool bEnable3D;
	__u32 u32PQFrameCnt;
	bool bXCEnableBob;

	bool bFieldOrderChange;
	enum E_DMS_VIDEO_FIELD_ORDER_TYPE enDetectFieldOrderType;

	bool bHistogramValid;
	__u32 u32Histogram[DMS_BUFFER_QUEUEDEPTH][DMS_SWDRHISTOGRAM_INDEX];

	__u32 u32QPMin[DMS_BUFFER_QUEUEDEPTH];	// Min of quantization parameter
	__u32 u32QPAvg[DMS_BUFFER_QUEUEDEPTH];	// Avg of quantization parameter
	__u32 u32QPMax[DMS_BUFFER_QUEUEDEPTH];	// Max of quantization parameter
	enum EN_DMS_FRAMETYPE enFrameType;	// Frame type (I/P/B mode)
	__u32 u32FlipTime[DMS_BUFFER_QUEUEDEPTH];
	__u32 u32DelayTime[DMS_BUFFER_QUEUEDEPTH];

	enum E_DMS_VIDEO_FIELD_ORDER_TYPE enPreFieldOrderType;
	bool bFDMaskEnable;
	enum E_DMS_INPUTSRC_TYPE enInputSrcType;

	bool bFirstSetwindow;
	bool bPQDSEnable;
	struct ST_DMS_DS_INFO stDSInfo;

	bool bPlayFlag;

	__u32 u32RWDiff;
	bool bOneFieldMode;

	__u32 u32AspectWidth;
	__u32 u32AspectHeight;
	__u8 u8AspectRate;

	__u32 au32VideoFlipTime[DMS_BUFFER_QUEUEDEPTH];
	bool abVideoFlip[DMS_BUFFER_QUEUEDEPTH];
	__u32 u32VideoDelayTime;
	__u32 u32PipeID[DMS_MAX_WINDOW_NUM];

	bool bIsSeqChange;

	enum E_DMS_MVOP_DROP_LINE enMVOPDropline;

	bool bSetWinImmediately;

	enum EN_DMS_FASTFORWARE_STATUS enDMSFastForwardStatus;

	__s64 s64MvopIsrTime;

	bool bIsCreateEmptyDsScript;
	__s32 s32CPLX;
	bool bEnableMVOPTimeStamp;
	__u64 u64TimeStamp;

	enum EN_DMS_API_STATE enDmsApiState;

	__u32 u32PQFrameCntPre;
	EN_PQ_DS_TYPE enPQDSTypePre;
	__u32 u32XCDSUpdateConditionPre;

	struct ST_DMS_SETWIN_INFO stUserSetwinInfo;

	bool bNeedResetMEMC;
};

struct DMS_RESOURCE_PRIVATE {
	struct ST_DRV_DMS stDrvDMS;
	struct ST_DRV_DMS_SECURE stDrvDMSSecure;
};

struct DMS_COMMON_RESOURCE_PRIVATE {
	bool bDummy;
	__u8 u8PathPattern[E_DMS_INPUTSRC_NUM][5];
	bool bDrvInit;
	__u8 u8CreateWindowNum;
	struct ST_DMS_WINDOW stOutputLayer;
	__u16 u16DebugLevel;
	bool bForceOffPQDS;
	__u8 u8CfdVersion;
	bool bIsForcePDebugMode;
	enum EN_DMS_FORCE_SWFRC enProcfsForceSwFrc;
	__u32 u32ProcfsForceSwFrcFrameRate;
	bool bProcfsFlipTrigEvent;
	bool bForceDTVDSOff;
	bool bPerframeUpdateXCSetting;
	bool bForceAllSourceDSOff;
};

struct ST_DRV_DMS_TIME {
	__u32 u32StartTime;
	__u32 u32DelayTime;
	__u32 u32Count[E_DMS_UPPER_MAX];
};

#endif
