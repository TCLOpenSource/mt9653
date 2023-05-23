/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_DISPLAY_MANAGER_H
#define _MTK_PQ_DISPLAY_MANAGER_H
#include <linux/types.h>
#include <linux/timekeeping.h>
#include <uapi/linux/mtk-v4l2-pq.h>

#include "mtk_pq.h"
#include "UFO.h"
#include "apiXC.h"

//----------------------------------------------------------------------------
//  Driver Capability
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//  Macro and Define
//----------------------------------------------------------------------------

#define MTK_PQ_QUEUE_DEPTH          (16)
#define MTK_PQ_MAX_KEEP_BUFFER            (3)
#define MTK_PQ_INVALID_FRAME_ID           (0xFF)
#define MTK_PQ_FLIP_TIMEOUT_THRESHOLD      (200)
#define SOURCE_FRAME_KEEP_NUM (2)
#define MAX_PQDS_FRAMECOUNT (20)
#define MTK_PQ_INIT_FOD_WIDTH (1)
#define MTK_PQ_INIT_FOD_HEIGHT (1)
#define MTK_PQ_FLOW_TYPE (1)
#define UNDERFLOW_X_REPEAT (2)

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
#define MTK_PQ_EXPECTED_TASKPROCESSTIME (5000)

#define IS_SRC_STORAGE(x)    (((x) == MTK_PQ_INPUTSRC_STORAGE) ||\
				((x) == MTK_PQ_INPUTSRC_STORAGE2))
#define IS_SRC_DTV(x)        (((x) == MTK_PQ_INPUTSRC_DTV) ||\
				((x) == MTK_PQ_INPUTSRC_DTV2))
#define IS_SRC_HDMI(x)       (((x) >= MTK_PQ_INPUTSRC_HDMI) &&\
				((x) < MTK_PQ_INPUTSRC_HDMI_MAX))
#define IS_INPUT_MVOP(x)     (IS_SRC_DTV(x) || IS_SRC_STORAGE(x))
#define MVOP_WINDOW 0

#define IS_HDR_PER_FRAME(x) (x & (MTK_PQ_HDR_METADATA_DOLBY_HDR +\
				MTK_PQ_HDR_METADATA_TCH +\
				MTK_PQ_HDR_METADATA_HDR10_PER_FRAME))
#define MTK_PQ_NSEC_PER_USEC (1)

#define MTK_PQ_GET_SYSTEM_TIME(x) do { \
	struct timespec stSystemTime; \
	getrawmonotonic(&stSystemTime); \
	x = (__s64)stSystemTime.tv_sec * 1000000000 + \
		(__s64)stSystemTime.tv_nsec; \
	} while (0)

//----------------------------------------------------------------------------
//  enums
//----------------------------------------------------------------------------

enum mtk_pq_video_codec {
	MTK_PQ_VIDEO_CODEC_UNKNOWN,
	MTK_PQ_VIDEO_CODEC_MPEG2,
	MTK_PQ_VIDEO_CODEC_H263,
	MTK_PQ_VIDEO_CODEC_MPEG4,
	MTK_PQ_VIDEO_CODEC_DIVX311,
	MTK_PQ_VIDEO_CODEC_DIVX412,
	MTK_PQ_VIDEO_CODEC_FLV,
	MTK_PQ_VIDEO_CODEC_VC1_AVD,
	MTK_PQ_VIDEO_CODEC_VC1_MAIN,
	MTK_PQ_VIDEO_CODEC_RV8,
	MTK_PQ_VIDEO_CODEC_RV9,
	MTK_PQ_VIDEO_CODEC_H264,
	MTK_PQ_VIDEO_CODEC_AVS,
	MTK_PQ_VIDEO_CODEC_MJPEG,
	MTK_PQ_VIDEO_CODEC_MVC,
	MTK_PQ_VIDEO_CODEC_VP8,
	MTK_PQ_VIDEO_CODEC_HEVC,
	MTK_PQ_VIDEO_CODEC_VP9,
	MTK_PQ_VIDEO_CODEC_HEVC_DV,
	MTK_PQ_VIDEO_CODEC_H264_DV,
	MTK_PQ_VIDEO_CODEC_NUM
};

enum mtk_pq_pqds_action {
	MTK_PQ_PQDS_GET_PQSETTING,
	MTK_PQ_PQDS_SET_PQSETTING,
	MTK_PQ_PQDS_CHECKSTATUS,
	MTK_PQ_PQDS_ENABLE_CHECK,
	MTK_PQ_PQDS_UPDATEINDEX,
};

enum mtk_pq_frc_ctrl {
	MTK_PQ_FRC_NONE = 0,
	MTK_PQ_FRC_DROP,
	MTK_PQ_FRC_REPEAT,
	MTK_PQ_FRC_MAX,
};

enum mtk_pq_field_order_type {
	MTK_PQ_VIDEO_FIELD_ORDER_TYPE_BOTTOM = 0,
	MTK_PQ_VIDEO_FIELD_ORDER_TYPE_TOP,
	MTK_PQ_VIDEO_FIELD_ORDER_TYPE_MAX,
};

enum mtk_pq_tile_mode {
	MTK_PQ_VIDEO_TILE_MODE_NONE = 0,
	MTK_PQ_VIDEO_TILE_MODE_16x16,
	MTK_PQ_VIDEO_TILE_MODE_16x32,
	MTK_PQ_VIDEO_TILE_MODE_32x16,
	MTK_PQ_VIDEO_TILE_MODE_32x32,
	MTK_PQ_VIDEO_TILE_MODE_4x2_COMPRESSION_MODE,
	MTK_PQ_VIDEO_TILE_MODE_8x1_COMPRESSION_MODE,
	MTK_PQ_VIDEO_TILE_MODE_TILE_32X16_82PACK,
	MTK_PQ_VIDEO_TILE_MODE_MAX,
};

enum mtk_pq_mfdec_vp9_mode {
	MTK_PQ_DUMMY_VIDEO_H26X_MODE = 0x00,
	MTK_PQ_DUMMY_VIDEO_VP9_MODE = 0x01,
};

enum mtk_pq_ds_manaul_state {
	MTK_PQ_DS_MANAUL_ON = 0,
	MTK_PQ_DS_MANAUL_OFF,
	MTK_PQ_DS_MAX,
};

enum mtk_pq_api_state {
	MTK_PQ_INIT = 0,
	MTK_PQ_CREATE_WINDOW,
	MTK_PQ_SETDIGITAL,
	MTK_PQ_SETWINDOW,
	MTK_PQ_CLEARDIGITAL_DESTROY_WINDOW,
};

enum mtk_pq_force_swfrc {
	MTK_PQ_FORCE_SWFRC_DEFAULT = 0,
	MTK_PQ_FORCE_SWFRC_ON,
	MTK_PQ_FORCE_SWFRC_OFF,
};

enum mtk_pq_measure_time_type {
	MEASURE_TIME_ISR = 0,
	MEASURE_TIME_RELEASE_BUF,
	MEASURE_TIME_SETWINDOW_DS,
	MEASURE_TIME_TASK,
	MEASURE_TIME_DOLBY_DS,
	MEASURE_TIME_SWDR,
	MEASURE_TIME_EVENT,
	MEASURE_TIME_CREATE_WIN,
	MEASURE_TIME_SET_WIN,
	MEASURE_TIME_DESTROY_WIN,
	MEASURE_TIME_SET_DIGITAL,
	MEASURE_TIME_CLEAR_DIGITAL,
	MEASURE_TIME_FLIP,
	MEASURE_TIME_FLIP_INTERVAL,
	MEASURE_TIME_IP_ISR,
	MEASURE_TIME_IP_EVENT,
	MEASURE_TIME_IP_TASK,
	MEASURE_TIME_MAX_NUM,
};

enum mtk_pq_measure_time_offset {
	MTK_PQ_UPPER_4_MS = 0,
	MTK_PQ_UPPER_6_MS,
	MTK_PQ_UPPER_8_MS,
	MTK_PQ_UPPER_10_MS,
	MTK_PQ_UPPER_MAX,
};

enum mtk_pq_seq_change_type {
	MTK_PQ_SEQ_CHANGE_NONE = 0,
	MTK_PQ_SEQ_CHANGE_TIMING = 0x01,
	MTK_PQ_SEQ_CHANGE_FRAMERATE = 0x02,
	MTK_PQ_SEQ_CHANGE_IP = 0x04,
	MTK_PQ_SEQ_CHANGE_ASPECTRATE = 0x08,
	MTK_PQ_SEQ_CHANGE_MAX = 0xFF,
};

enum mtk_pq_fastforward_status {
	MTK_PQ_NORMAL,
	MTK_PQ_PQDS_LOADZERO,
	MTK_PQ_PQDS_LOADZERO_DONE,
	MTK_PQ_RESUME,
};

enum mtk_pq_field_change {
	MTK_PQ_FIELD_CHANGE_DISABLE = 0,
	MTK_PQ_FIELD_CHANGE_FIRST_FIELD,
	MTK_PQ_FIELD_CHANGE_SECOND_FIELD,
	MTK_PQ_FIELD_CHANGE_MAX_NUM,
};

enum mtk_pq_hdr_metadata_type {
	MTK_PQ_HDR_METATYPE_MPEG_VUI = (0x1 << 0),
	MTK_PQ_HDR_METADATA_MPEG_SEI_MASTERING_COLOR_VOLUME = (0x1 << 1),
	MTK_PQ_HDR_METADATA_DOLBY_HDR = (0x1 << 2),
	MTK_PQ_HDR_METADATA_TCH = (0x1 << 3),
	MTK_PQ_HDR_METADATA_HDR10_PER_FRAME = (0x1 << 4),
	MTK_PQ_HDR_METATYPE_CONTENT_LIGHT = (0x1 << 5),
	MTK_PQ_HDR_METATYPE_DYNAMIC = (0x1 << 6),
};

//----------------------------------------------------------------------------
//  structs
//----------------------------------------------------------------------------

struct mtk_pq_mfdec_htlb_info {
	__u32 u32version;
	__u32 u32size;
	__u64 phyHTLBEntriesAddr;
	__u8 u8HTLBEntriesSize;
	__u8 u8HTLBTableId;
	void *pHTLBInfo;
#ifndef __aarch64__
	__u32 dummy;
#endif
};

struct mtk_pq_mfdec_info {
	bool bMFDec_Enable;
	__u8 u8MFDec_Select;
	bool bHMirror;
	bool bVMirror;
	bool bUncompress_mode;
	bool bBypass_codec_mode;
	enum mtk_pq_mfdec_vp9_mode enMFDecVP9_mode;
	__u16 u16StartX;
	__u16 u16StartY;
	__u64 phyBitlen_Base;
	__u16 u16Bitlen_Pitch;
	__u8 u8Bitlen_MiuSelect;
	struct mtk_pq_mfdec_htlb_info stMFDec_HTLB_Info;
	void *pMFDecInfo;
#ifndef __aarch64__
	__u32 dummy;
#endif
	bool bAV1_mode;
	bool bAV1PpuBypassMode;
	bool bMonoChrome;
};

struct mtk_pq_vdec_frame_info {
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
	enum mtk_pq_video_data_format enFmt;
	__u32 u32Window;
	enum mtk_pq_video_codec enCODEC;
	enum mtk_pq_scan_type enScanType;
	enum mtk_pq_field_order_type enFieldOrderType;
	enum mtk_pq_field_type enFieldType;
	enum mtk_pq_field_type enFieldType_2nd;
	enum mtk_pq_tile_mode enTileMode;
	struct mtk_pq_mfdec_info stMFdecInfo;
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

	struct mtk_pq_vdec_hdr_info stHDRInfo;

	__u8 u8FieldCtrl;

	enum mtk_pq_frame_type enFrameType;

	__u32 u32DisplayCounts;

	__u32 u32TaskTrigCounts;

	bool bUserSetwinFlag;
	struct mtk_pq_window stUserXCCaptWin;
	struct mtk_pq_window stUserXCDispWin;
	struct mtk_pq_window stUserXCCropWin;
	enum mtk_pq_aspect_ratio enUserArc;
	__u64 u64PtsUs;
	struct vb2_buffer *vb;
};

struct mtk_pq_buffer_ptr {
	__u16 write_ptr;
	__u16 read_ptr;
};

struct mtk_pq_ds_info {
	__u64 phyDynScalingAddr;
	__u32 u32DynScalingSize;
	bool bEnableMIUSel;
	__u8 u8DynScalingDepth;
	enum mtk_pq_ds_manaul_state enDsManualState;
};


struct mtk_pq_secure_resource {
	bool bSecurityEnabled[V4L2_MTK_MAX_WINDOW];
	__u32 u32PipeID[V4L2_MTK_MAX_WINDOW];
};

struct mtk_pq_bob_mode_info {
	__u32 u32WindowID;
	__u32 u32CurReadPointer;
	bool bPauseStage;
	bool bResetBobMode;
};

struct mtk_pq_display_resource {
	struct mtk_pq_window_info stWindowInfo;
	struct mtk_pq_buffer_ptr buffer_ptr;
	__u32 u32WindowFrameRate;
	__u32 scaling_condition;
	struct mtk_pq_window stXCCaptWinInfo;
	bool bStartCountFRC;
	struct mtk_pq_vdec_frame_info frame_info[MTK_PQ_QUEUE_DEPTH];
	__u8 u8LocalFrameRefCount[MTK_PQ_QUEUE_DEPTH];
	bool bWinUsed;
	__u32 u32ReleaseState[MTK_PQ_QUEUE_DEPTH];
	bool bFirstPlay;
	bool bSetMute;
	__u32 u32MuteCounter;
	__u8 au8WinPattern[V4L2_MTK_MAX_PATH_IPNUM];
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
	enum mtk_pq_field_order_type detect_order_type;

	bool bHistogramValid;
	__u32 u32Histogram[MTK_PQ_QUEUE_DEPTH][V4L2_MTK_HISTOGRAM_INDEX];

	__u32 u32QPMin[MTK_PQ_QUEUE_DEPTH];	// Min of quantization parameter
	__u32 u32QPAvg[MTK_PQ_QUEUE_DEPTH];	// Avg of quantization parameter
	__u32 u32QPMax[MTK_PQ_QUEUE_DEPTH];	// Max of quantization parameter
	enum mtk_pq_frame_type enFrameType;	// Frame type (I/P/B mode)
	__u32 u32FlipTime[MTK_PQ_QUEUE_DEPTH];
	__u32 u32DelayTime[MTK_PQ_QUEUE_DEPTH];

	enum mtk_pq_field_order_type enPreFieldOrderType;
	bool bFDMaskEnable;
	enum mtk_pq_input_source_type enInputSrcType;

	bool bFirstSetwindow;
	bool bPQDSEnable;
	struct mtk_pq_ds_info stDSInfo;

	bool bPlayFlag;

	__u32 u32RWDiff;
	bool bOneFieldMode;

	__u32 u32AspectWidth;
	__u32 u32AspectHeight;
	__u8 u8AspectRate;

	__u32 au32VideoFlipTime[MTK_PQ_QUEUE_DEPTH];
	bool abVideoFlip[MTK_PQ_QUEUE_DEPTH];
	__u32 u32VideoDelayTime;
	__u32 u32PipeID[V4L2_MTK_MAX_WINDOW];

	bool bIsSeqChange;

	enum mtk_pq_drop_line enMVOPDropline;

	bool bSetWinImmediately;

	enum mtk_pq_fastforward_status fast_forward_status;

	__s64 s64MvopIsrTime;

	bool bIsCreateEmptyDsScript;
	__s32 s32CPLX;
	bool bEnableMVOPTimeStamp;
	__u64 u64TimeStamp;

	enum mtk_pq_api_state api_state;

	__u32 u32PQFrameCntPre;
	EN_PQ_DS_TYPE enPQDSTypePre;
	__u32 u32XCDSUpdateConditionPre;

	struct mtk_pq_set_win_info stUserSetwinInfo;

	bool bNeedResetMEMC;
};

struct mtk_pq_resource {
	struct mtk_pq_display_resource display_resource;
	struct mtk_pq_secure_resource secure_resource;
};

struct mtk_pq_common_resource {
	bool bDummy;
	__u8 u8PathPattern[MTK_PQ_INPUTSRC_NUM][5];
	bool bDrvInit;
	__u8 u8CreateWindowNum;
	struct mtk_pq_window stOutputLayer;
	__u16 u16DebugLevel;
	bool bForceOffPQDS;
	__u8 u8CfdVersion;
	bool bIsForcePDebugMode;
	enum mtk_pq_force_swfrc enProcfsForceSwFrc;
	__u32 u32ProcfsForceSwFrcFrameRate;
	bool bProcfsFlipTrigEvent;
	bool bForceDTVDSOff;
	bool bPerframeUpdateXCSetting;
	bool bForceAllSourceDSOff;
};

struct mtk_pq_measure_time {
	__u32 u32StartTime;
	__u32 u32DelayTime;
	__u32 u32Count[MTK_PQ_UPPER_MAX];
};

int mtk_display_get_data(__u32 u32WindowID, void *ctrl);
int mtk_display_set_data(__u32 u32WindowID,
			struct v4l2_ext_control *ctrl);

int mtk_display_init(void);
int mtk_display_set_inputsource(__u32 u32WindowID, __u32 InputSrcType);

int mtk_display_set_digitalsignalinfo(__u32 u32WindowID,
	struct mtk_pq_device *pq_dev);

int mtk_display_clear_digitalsignalinfo(__u32 u32WindowID,
	struct mtk_pq_device *pq_dev);

int mtk_display_qbuf(struct mtk_pq_device *pq_dev, struct vb2_buffer *vb);
int mtk_display_create_task(struct mtk_pq_device *pq_dev);
int mtk_display_destroy_task(struct mtk_pq_device *pq_dev);
int mtk_display_subscribe_event(__u32 event_type, __u16 *dev_id);
int mtk_display_unsubscribe_event(__u32 event_type, __u16 *dev_id);
int mtk_display_set_forcep(__u32 u32WindowID, bool enable);
int mtk_display_set_xcenfbl(__u32 u32WindowID, bool enable);
int mtk_display_set_outputfrmrate(__u32 u32WindowID, __u32 u32OutputFrmRate);

INPUT_SOURCE_TYPE_t mtk_display_transfer_input_source(
	enum mtk_pq_input_source_type enInputSrc);

irqreturn_t mtk_display_mvop_isr(int irq, void *prv);

#endif
