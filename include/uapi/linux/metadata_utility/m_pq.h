/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __UAPI_MTK_M_PQ_H
#define __UAPI_MTK_M_PQ_H

#include <linux/types.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#endif

/* ============================================================== */
/* ------------------------ Metadata Tag ------------------------- */
/* ============================================================== */
#define PQ_DISP_SVP_META_TAG                "pq_svp"
#define META_PQ_OUTPUT_FRAME_INFO_TAG       "pq_dd_frm_info"
#define MTK_PQ_SH_FRM_INFO_TAG              "MTK_PQ_SH_FRMINFO"
#define META_PQ_DISPLAY_FLOW_INFO_TAG       "MTK_PQ_DISPLAY_FLOW_INFO"
#define META_PQ_DISPLAY_IDR_CTRL_META_TAG   "MTK_PQ_DISPLAY_IDR_CTRL"
#define META_PQ_STREAM_INTERNAL_INFO_TAG    "MTK_PQ_DD_STREAM_INFO"
#define PQ_DISP_STREAM_META_TAG             "stream_info"
#define META_PQ_INPUT_QUEUE_EXT_INFO_TAG    "MTK_PQ_INPUT_QUEUE_EXT_INFO"
#define META_PQ_OUTPUT_QUEUE_EXT_INFO_TAG   "MTK_PQ_OUT_QUEUE_EXT_INFO"
#define META_PQ_FRAMESYNC_INFO_TAG          "MTK_PQ_FRAME_SYNC_INFO"
#define META_PQ_OUTPUT_RENDER_INFO_TAG      "MTK_PQ_OUTPUT_RENDER_INFO"
#define META_PQ_DISPLAY_WM_INFO_TAG       "MTK_PQ_DISPLAY_WM_INFO"
#define META_PQ_BBD_INFO_TAG			"MTK_PQ_BBD_INFO"

#define M_PQ_CFD_INPUT_FORMAT_META_TAG      "m_pq_cfd_input_format"
#define M_PQ_CFD_OUTPUT_FORMAT_META_TAG     "m_pq_cfd_output_format"
#define META_PQ_PQMAP_RULE_TAG              "m_pq_pqmap_rule_settings"
#define META_PQ_PQPARAM_TAG                 "m_pq_pqparam_info"
#define META_PQ_PQPARAM_VTS_TAG             "m_pq_pqparam_pqu_info"
#define META_DISP_NON_LINEAR_SCALING_TAG    "m_disp_non_linear_scaling"
#define META_PQ_DV_INFO_TAG                 "m_pq_dv_info"

/* ============================================================== */
/* ---------------------- Metadata ENUM ----------------------- */
/* ============================================================== */
enum meta_pq_source {
	meta_pq_source_ipdma = 0,
	meta_pq_source_b2r,
	meta_pq_source_max,
};

enum meta_pq_mode {
	meta_pq_mode_ts = 0,
	meta_pq_mode_legacy,
	meta_pq_mode_max,
};
enum meta_pq_scene {
	meta_pq_scene_normal = 0,
	meta_pq_scene_game,
	meta_pq_scene_pc,
	meta_pq_scene_memc,
	meta_pq_scene_pip,
	meta_pq_scene_max,
};
enum meta_pq_framerate {
	meta_pq_framerate_24 = 0,
	meta_pq_framerate_30,
	meta_pq_framerate_50,
	meta_pq_framerate_60,
	meta_pq_framerate_120,
	meta_pq_framerate_max,
};
enum meta_pq_quality {
	meta_pq_quality_full = 0,
	meta_pq_quality_lite,
	meta_pq_quality_zfd,
	meta_pq_quality_max,
};

enum meta_pq_idr_input_path {
	META_PQ_PATH_IPDMA_0 = 0,
	META_PQ_PATH_IPDMA_1,
	META_PQ_PATH_IPDMA_MAX,
};

enum meta_pq_colorformat {
	meta_pq_colorformat_rgb = 0,
	meta_pq_colorformat_yuv,
	meta_pq_colorformat_max,
};

enum meta_pq_input_source {
	META_PQ_INPUTSRC_NONE = 0,      ///<NULL input
	META_PQ_INPUTSRC_VGA,           ///<1   VGA input
	META_PQ_INPUTSRC_TV,            ///<2   TV input

	META_PQ_INPUTSRC_CVBS,          ///<3   AV 1
	META_PQ_INPUTSRC_CVBS2,         ///<4   AV 2
	META_PQ_INPUTSRC_CVBS3,         ///<5   AV 3
	META_PQ_INPUTSRC_CVBS4,         ///<6   AV 4
	META_PQ_INPUTSRC_CVBS5,         ///<7   AV 5
	META_PQ_INPUTSRC_CVBS6,         ///<8   AV 6
	META_PQ_INPUTSRC_CVBS7,         ///<9   AV 7
	META_PQ_INPUTSRC_CVBS8,         ///<10   AV 8
	META_PQ_INPUTSRC_CVBS_MAX,      ///<11 AV max

	META_PQ_INPUTSRC_SVIDEO,        ///<12 S-video 1
	META_PQ_INPUTSRC_SVIDEO2,       ///<13 S-video 2
	META_PQ_INPUTSRC_SVIDEO3,       ///<14 S-video 3
	META_PQ_INPUTSRC_SVIDEO4,       ///<15 S-video 4
	META_PQ_INPUTSRC_SVIDEO_MAX,    ///<16 S-video max

	META_PQ_INPUTSRC_YPBPR,         ///<17 Component 1
	META_PQ_INPUTSRC_YPBPR2,        ///<18 Component 2
	META_PQ_INPUTSRC_YPBPR3,        ///<19 Component 3
	META_PQ_INPUTSRC_YPBPR_MAX,     ///<20 Component max

	META_PQ_INPUTSRC_SCART,         ///<21 Scart 1
	META_PQ_INPUTSRC_SCART2,        ///<22 Scart 2
	META_PQ_INPUTSRC_SCART_MAX,     ///<23 Scart max

	META_PQ_INPUTSRC_HDMI,          ///<24 HDMI 1
	META_PQ_INPUTSRC_HDMI2,         ///<25 HDMI 2
	META_PQ_INPUTSRC_HDMI3,         ///<26 HDMI 3
	META_PQ_INPUTSRC_HDMI4,         ///<27 HDMI 4
	META_PQ_INPUTSRC_HDMI_MAX,      ///<28 HDMI max

	META_PQ_INPUTSRC_DVI,           ///<29 DVI 1
	META_PQ_INPUTSRC_DVI2,          ///<30 DVI 2
	META_PQ_INPUTSRC_DVI3,          ///<31 DVI 2
	META_PQ_INPUTSRC_DVI4,          ///<32 DVI 4
	META_PQ_INPUTSRC_DVI_MAX,       ///<33 DVI max

	META_PQ_INPUTSRC_DTV,           ///<34 DTV
	META_PQ_INPUTSRC_DTV2,          ///<35 DTV2
	META_PQ_INPUTSRC_DTV_MAX,       ///<36 DTV max

	// Application source
	META_PQ_INPUTSRC_STORAGE,       ///<37 Storage
	META_PQ_INPUTSRC_STORAGE2,      ///<38 Storage2
	META_PQ_INPUTSRC_STORAGE_MAX,   ///<39 Storage max

	// Support OP capture
	META_PQ_INPUTSRC_SCALER_OP,     ///<40 scaler OP

	META_PQ_INPUTSRC_NUM,           ///<number of the source

};

enum meta_ip_window {
	meta_ip_capture = 0,
	meta_ip_ip2_in,
	meta_ip_ip2_out,
	meta_ip_scmi_in,
	meta_ip_scmi_out,
	meta_ip_aisr_in,
	meta_ip_aisr_out,
	meta_ip_hvsp_in,
	meta_ip_hvsp_out,
	meta_ip_display,
	meta_ip_max,
};

enum meta_dip_cap_point {
	meta_dip_cap_pqin = 0,
	meta_dip_cap_iphdr,
	meta_dip_cap_prespf,
	meta_dip_cap_hvsp,
	meta_dip_cap_srs,
	meta_dip_cap_vip,
	meta_dip_cap_mdw,
	meta_dip_cap_max,
};

enum meta_pq_pqmap_type {
	META_PQMAP_MAIN,
	META_PQMAP_MAIN_EX,
	META_PQMAP_MAX
};

enum EN_PQ_CFD_DATA_FORMAT {
	E_PQ_CFD_DATA_FORMAT_RGB = 0x00,
	E_PQ_CFD_DATA_FORMAT_YUV422 = 0x01,
	E_PQ_CFD_DATA_FORMAT_YUV444 = 0x02,
	E_PQ_CFD_DATA_FORMAT_YUV420 = 0x03,
	E_PQ_CFD_DATA_FORMAT_RESERVED_START,
};

enum EN_PQ_CFD_COLOR_RANGE {
	E_PQ_CFD_COLOR_RANGE_LIMIT = 0x0,
	E_PQ_CFD_COLOR_RANGE_FULL = 0x1,
	E_PQ_CFD_COLOR_RANGE_RESERVED_START,
};

enum EN_PQ_CFD_HDR_MODE {
	E_PQ_CFD_HDR_MODE_SDR = 0x0,
	E_PQ_CFD_HDR_MODE_DOLBY = 0x1,  // Dolby
	E_PQ_CFD_HDR_MODE_HDR10 = 0x2,  // open HDR
	E_PQ_CFD_HDR_MODE_HLG = 0x3,    // Hybrid log gamma
	E_PQ_CFD_HDR_MODE_TCH = 0x4,    // TCH
	E_PQ_CFD_HDR_MODE_HDR10PLUS = 0x5,  // HDR10plus
	E_PQ_CFD_HDR_MODE_CONTROL_FLAG_RESET = 0x10,    // flag reset
	E_PQ_CFD_HDR_MODE_RESERVED_START,
};

enum EN_PQ_CFD_INPUT_SOURCE {
	E_PQ_CFD_INPUT_SOURCE_VGA = 0x00,
	E_PQ_CFD_INPUT_SOURCE_TV = 0x01,
	E_PQ_CFD_INPUT_SOURCE_CVBS = 0x02,
	E_PQ_CFD_INPUT_SOURCE_SVIDEO = 0x03,
	E_PQ_CFD_INPUT_SOURCE_YPBPR = 0x04,
	E_PQ_CFD_INPUT_SOURCE_SCART = 0x05,
	E_PQ_CFD_INPUT_SOURCE_HDMI = 0x06,
	E_PQ_CFD_INPUT_SOURCE_DTV = 0x07,
	E_PQ_CFD_INPUT_SOURCE_DVI = 0x08,
	E_PQ_CFD_INPUT_SOURCE_STORAGE = 0x09,
	E_PQ_CFD_INPUT_SOURCE_KTV = 0x0A,
	E_PQ_CFD_INPUT_SOURCE_JPEG = 0x0B,
	E_PQ_CFD_INPUT_SOURCE_RX = 0x0C,
	E_PQ_CFD_INPUT_SOURCE_RESERVED_START,
	E_PQ_CFD_INPUT_SOURCE_NONE = E_PQ_CFD_INPUT_SOURCE_RESERVED_START,
	E_PQ_CFD_INPUT_SOURCE_GENERAL = 0xFF,
};

enum EN_PQ_CFD_OUTPUT_SOURCE {
	//include VDEC series
	E_PQ_CFD_OUTPUT_SOURCE_MM = 0x00,
	E_PQ_CFD_OUTPUT_SOURCE_HDMI = 0x01,
	E_PQ_CFD_OUTPUT_SOURCE_ANALOG = 0x02,
	E_PQ_CFD_OUTPUT_SOURCE_PANEL = 0x03,
	E_PQ_CFD_OUTPUT_SOURCE_ULSA = 0x04,
	E_PQ_CFD_OUTPUT_SOURCE_GENERAL = 0x05,
	E_PQ_CFD_OUTPUT_SOURCE_RESERVED_START,
};

enum EN_PQ_CFD_COLOR_FORMAT {
	E_PQ_CFD_COLOR_FORMAT_RGB_NOTSPECIFIED      = 0x0, //means RGB, but no specific colorspace
	E_PQ_CFD_COLOR_FORMAT_RGB_BT601_625         = 0x1,
	E_PQ_CFD_COLOR_FORMAT_RGB_BT601_525         = 0x2,
	E_PQ_CFD_COLOR_FORMAT_RGB_BT709             = 0x3,
	E_PQ_CFD_COLOR_FORMAT_RGB_BT2020            = 0x4,
	E_PQ_CFD_COLOR_FORMAT_SRGB                  = 0x5,
	E_PQ_CFD_COLOR_FORMAT_ADOBE_RGB             = 0x6,
	E_PQ_CFD_COLOR_FORMAT_YUV_NOTSPECIFIED      = 0x7, //means RGB, but no specific colorspace
	E_PQ_CFD_COLOR_FORMAT_YUV_BT601_625         = 0x8,
	E_PQ_CFD_COLOR_FORMAT_YUV_BT601_525         = 0x9,
	E_PQ_CFD_COLOR_FORMAT_YUV_BT709             = 0xA,
	E_PQ_CFD_COLOR_FORMAT_YUV_BT2020_NCL        = 0xB,
	E_PQ_CFD_COLOR_FORMAT_YUV_BT2020_CL         = 0xC,
	E_PQ_CFD_COLOR_FORMAT_XVYCC_601             = 0xD,
	E_PQ_CFD_COLOR_FORMAT_XVYCC_709             = 0xE,
	E_PQ_CFD_COLOR_FORMAT_SYCC601               = 0xF,
	E_PQ_CFD_COLOR_FORMAT_ADOBE_YCC601          = 0x10,
	E_PQ_CFD_COLOR_FORMAT_DOLBY_HDR_TEMP        = 0x11,
	E_PQ_CFD_COLOR_FORMAT_SYCC709               = 0x12,
	E_PQ_CFD_COLOR_FORMAT_DCIP3_THEATER         = 0x13,
	E_PQ_CFD_COLOR_FORMAT_DCIP3_D65             = 0x14,
	E_PQ_CFD_COLOR_FORMAT_ITURBT_BT2100_ICTCP   = 0x15,
	E_PQ_CFD_COLOR_FORMAT_RESERVED_START,
};

//color space
enum EN_PQ_CFD_CFIO_CP {
	E_PQ_CFD_CFIO_CP_RESERVED0 = 0x0,   //means RGB, but no specific colorspace
	E_PQ_CFD_CFIO_CP_BT709_SRGB_SYCC = 0x1,
	E_PQ_CFD_CFIO_CP_UNSPECIFIED = 0x2,
	E_PQ_CFD_CFIO_CP_RESERVED3 = 0x3,
	E_PQ_CFD_CFIO_CP_BT470_6 = 0x4,
	E_PQ_CFD_CFIO_CP_BT601625 = 0x5,
	E_PQ_CFD_CFIO_CP_BT601525_SMPTE170M = 0x6,
	E_PQ_CFD_CFIO_CP_SMPTE240M = 0x7,
	E_PQ_CFD_CFIO_CP_GENERIC_FILM = 0x8,
	E_PQ_CFD_CFIO_CP_BT2020 = 0x9,
	E_PQ_CFD_CFIO_CP_CIEXYZ = 0xA,
	E_PQ_CFD_CFIO_CP_DCIP3_THEATER = 0xB,
	E_PQ_CFD_CFIO_CP_DCIP3_D65 = 0xC,
	// 13-21: reserved
	E_PQ_CFD_CFIO_CP_EBU3213 = 0x16,
	// 23-127: reserved
	E_PQ_CFD_CFIO_CP_ADOBERGB = 0x80,
	E_PQ_CFD_CFIO_CP_PANEL = 0x81,
	E_PQ_CFD_CFIO_CP_SOURCE = 0x82,
	E_PQ_CFD_CFIO_CP_RESERVED_START,
};

//Transfer characteristics
enum EN_PQ_CFD_CFIO_TR {
	E_PQ_CFD_CFIO_TR_RESERVED0 = 0x0,   //means RGB, but no specific colorspace
	E_PQ_CFD_CFIO_TR_BT709 = 0x1,
	E_PQ_CFD_CFIO_TR_UNSPECIFIED = 0x2,
	E_PQ_CFD_CFIO_TR_RESERVED3 = 0x3,
	E_PQ_CFD_CFIO_TR_GAMMA2P2 = 0x4,
	E_PQ_CFD_CFIO_TR_GAMMA2P8 = 0x5,
	E_PQ_CFD_CFIO_TR_BT601525_601625 = 0x6,
	E_PQ_CFD_CFIO_TR_SMPTE240M = 0x7,
	E_PQ_CFD_CFIO_TR_LINEAR = 0x8,
	E_PQ_CFD_CFIO_TR_LOG0 = 0x9,
	E_PQ_CFD_CFIO_TR_LOG1 = 0xA,
	E_PQ_CFD_CFIO_TR_XVYCC = 0xB,
	E_PQ_CFD_CFIO_TR_BT1361 = 0xC,
	E_PQ_CFD_CFIO_TR_SRGB_SYCC = 0xD,
	E_PQ_CFD_CFIO_TR_BT2020NCL = 0xE,
	E_PQ_CFD_CFIO_TR_BT2020CL = 0xF,
	E_PQ_CFD_CFIO_TR_SMPTE2084 = 0x10,
	E_PQ_CFD_CFIO_TR_SMPTE428 = 0x11,
	E_PQ_CFD_CFIO_TR_HLG = 0x12,
	E_PQ_CFD_CFIO_TR_BT1886 = 0x13,
	E_PQ_CFD_CFIO_TR_DOLBYMETA = 0x14,
	E_PQ_CFD_CFIO_TR_ADOBERGB = 0x15,
	E_PQ_CFD_CFIO_TR_GAMMA2P6 = 0x16,
	E_PQ_CFD_CFIO_TR_RESERVED_START,
};

//Matrix coefficient
enum EN_PQ_CFD_CFIO_MC {
	E_PQ_CFD_CFIO_MC_IDENTITY = 0x0,    //means RGB, but no specific colorspace
	E_PQ_CFD_CFIO_MC_BT709_XVYCC709 = 0x1,
	E_PQ_CFD_CFIO_MC_UNSPECIFIED = 0x2,
	E_PQ_CFD_CFIO_MC_RESERVED = 0x3,
	E_PQ_CFD_CFIO_MC_USFCCT47 = 0x4,
	E_PQ_CFD_CFIO_MC_BT601625_XVYCC601_SYCC = 0x5,
	E_PQ_CFD_CFIO_MC_BT601525_SMPTE170M = 0x6,
	E_PQ_CFD_CFIO_MC_SMPTE240M = 0x7,
	E_PQ_CFD_CFIO_MC_YCGCO = 0x8,
	E_PQ_CFD_CFIO_MC_BT2020NCL = 0x9,
	E_PQ_CFD_CFIO_MC_BT2020CL = 0xA,
	E_PQ_CFD_CFIO_MC_YDZDX = 0xB,
	E_PQ_CFD_CFIO_MC_CD_NCL = 0xC,
	E_PQ_CFD_CFIO_MC_CD_CL = 0xD,
	E_PQ_CFD_CFIO_MC_ICTCP = 0xE,
	E_PQ_CFD_CFIO_MC_RESERVED_START,
};

/* ============================================================== */
/* ---------------------- Metadata Struct ----------------------- */
/* ============================================================== */
struct meta_pq_window {
	__u16 x;
	__u16 y;
	__u16 width;
	__u16 height;
	__u16 w_align;
	__u16 h_align;
};

struct meta_dip_window {
	uint16_t width;
	uint16_t height;
	uint8_t color_fmt; /* refer EN_PQ_CFD_DATA_FORMAT */
	uint8_t p_engine;
};

struct meta_pq_display_rect {
	uint32_t left;
	uint32_t top;
	uint32_t width;
	uint32_t height;
};

struct meta_pq_wm_config {
	bool bWm_en;
};

struct meta_pq_wm_result {
	__u32 u32Version;
	__u32 u32Length;
	__u16 u16Wm_En;
	__u16 u16Wm_decoded[30];
	__u8 u8Wm_DetectStatus;
};
/* ============================================================== */
/* ---------------------- Metadata Content ----------------------- */
/* ============================================================== */
#define META_PQ_DISP_SVP_VERSION (1)
struct meta_pq_disp_svp {
	__u8 aid;
	__u32 pipelineid;
};

/*
 * Version 1: Init structure
 */
#define META_PQ_OUTPUT_FRAME_INFO_VERSION (1)
struct meta_pq_output_frame_info {
	uint32_t width;
	uint32_t height;
	struct meta_pq_window capture;
	struct meta_pq_window crop;
	struct meta_pq_window display;
	bool nonlinear;
	uint64_t fd_offset[3];
	int32_t pq_frame_id;
};

#define META_PQ_FRAMESYNC_INFO_VERSION (1)
struct meta_pq_framesync_info {
	enum meta_pq_input_source input_source;
};

#define META_PQ_OUTPUT_RENDER_INFO_VERSION (1)
struct meta_pq_output_render_info {
	struct meta_pq_window capture;
	struct meta_pq_window crop;
	struct meta_pq_window displayRange;
	struct meta_pq_window displayArea;
	struct meta_pq_window displayWin;
	__u8 u8DotByDotType;
	__u8 u8SignalStable;
	__u32 u32RefreshRate;
	__u8 u8Aid;
	__u32 u32Pipelineid;
	__u8 u8MuteAction;
	__u8 u8PqMuteAction;
	__u64 u64Pts;
	__u32 u32OriginalInputFps;
};

#define META_PQ_STREAM_INTERNAL_INFO_VERSION (1)
struct meta_pq_stream_internal_info {
	uint64_t file;
	uint64_t pq_dev;
};

#define META_PQ_STREAM_DEBUG_INFO_VERSION (1)
struct meta_pq_stream_debug_info {
	bool cmdq_timeout;
};

#define META_PQ_STREAM_INFO_VERSION (1)
struct meta_pq_stream_info {
	int width;
	int height;
	bool interlace;
	bool adaptive_stream;
	bool low_latency;
	enum meta_pq_source pq_source;
	enum meta_pq_mode pq_mode;
	enum meta_pq_scene pq_scene;
	enum meta_pq_framerate pq_framerate;
	enum meta_pq_quality pq_quality;
	enum meta_pq_colorformat pq_colorformat;
	bool stub;
	enum meta_pq_idr_input_path pq_idr_input_path;
	__u8 scenario_idx;
};

#define META_PQ_DISPLAY_FLOW_INFO_VERSION (1)
struct meta_pq_display_flow_info {
	struct meta_pq_window content;
	struct meta_pq_window capture;
	struct meta_pq_window crop;
	struct meta_pq_window display;
	struct meta_pq_window displayArea;
	struct meta_pq_window displayRange;

	struct meta_pq_window ip_win[meta_ip_max];
	bool aisr_enable;

	/* output data */
	uint8_t win_id;
	struct meta_dip_window dip_win[meta_dip_cap_max];
	struct meta_pq_window outcrop;
	struct meta_pq_window outdisplay;
	struct meta_pq_wm_config wm_config;
};

#define META_PQ_INPUT_QUEUE_EXT_INFO_VERSION (1)
struct meta_pq_input_queue_ext_info {
	__u64 u64Pts;
	__u64 u64UniIdx;
	__u64 u64ExtUniIdx;
	__u64 u64TimeStamp;
	__u64 u64RenderTimeNs;
	__u8 u8BufferValid;
	__u32 u32BufferSlot;
	__u32 u32GenerationIndex;
	__u8 u8RepeatStatus;
	__u8 u8FrcMode;
	__u8 u8Interlace;
	__u32 u32InputFps;
	__u32 u32OriginalInputFps;
	bool bEOS;
	__u8 u8MuteAction;
	__u8 u8SignalStable;
	__u8 u8DotByDotType;
	__u32 u32RefreshRate;
	__u32 u32QueueInputIndex;
};

#define META_PQ_OUTPUT_QUEUE_EXT_INFO_VERSION (1)
struct meta_pq_output_queue_ext_info {
	__u64 u64Pts;
	__u64 u64UniIdx;
	__u64 u64ExtUniIdx;
	__u64 u64TimeStamp;
	__u64 u64RenderTimeNs;
	__u8 u8BufferValid;
	__u32 u32BufferSlot;
	__u32 u32GenerationIndex;
	__u8 u8RepeatStatus;
	__u8 u8FrcMode;
	__u8 u8Interlace;
	__u32 u32InputFps;
	__u32 u32OriginalInputFps;
	bool bEOS;
	__u8 u8MuteAction;
	__u8 u8SignalStable;
	__u8 u8DotByDotType;
	__u32 u32RefreshRate;
	__u32 u32QueueInputIndex;
	__u32 u32QueueOutputIndex;
	struct meta_pq_wm_result wm_result;
};

#define META_PQ_DISPLAY_IDR_CTRL_VERSION (1)
struct meta_pq_display_idr_ctrl {
	struct meta_pq_display_rect crop;
	uint32_t mem_fmt;
	uint32_t width;
	uint32_t height;
	uint16_t index;		/* force read index */
	uint64_t vb;		// ptr of struct vb2_buffer, use in callback
	enum meta_pq_idr_input_path path;
	bool bypass_ipdma;
	bool v_flip;
	uint8_t aid;		// access id
};

#define M_PQ_CFD_INPUT_FORMAT_VERSION (0)
struct m_pq_cfd_input_format {
	uint8_t source; //refer enum EN_PQ_CFD_INPUT_SOURCE

	uint8_t data_format; //refer enum EN_PQ_CFD_DATA_FORMAT
	uint8_t bit_depth; //6/8/10/12
	uint8_t data_range; //refer enum EN_PQ_CFD_COLOR_RANGE
	uint8_t colour_primaries; //refer enum EN_PQ_CFD_CFIO_CP
	uint8_t transfer_characteristics; //refer enum EN_PQ_CFD_CFIO_TR
	uint8_t matrix_coefficients; //refer enum EN_PQ_CFD_CFIO_MC

	uint8_t hdr_mode; //refer enum EN_PQ_CFD_HDR_MODE
	uint32_t u32SamplingMode;
} __attribute__((packed));

#define M_PQ_CFD_OUTPUT_FORMAT_VERSION (0)
struct m_pq_cfd_output_format {
	uint8_t source; //refer enum EN_PQ_CFD_OUTPUT_SOURCE

	uint8_t data_format; //refer enum EN_PQ_CFD_DATA_FORMAT
	uint8_t bit_depth; //6/8/10/12
	uint8_t data_range; //refer enum EN_PQ_CFD_COLOR_RANGE
	uint8_t colour_primaries; //refer enum EN_PQ_CFD_CFIO_CP
	uint8_t transfer_characteristics; //refer enum EN_PQ_CFD_CFIO_TR
	uint8_t matrix_coefficients; //refer enum EN_PQ_CFD_CFIO_MC
} __attribute__((packed));

#define META_PQ_PQMAP_RULE_VERSION (1)
#define META_PQ_PQMAP_NODE_SIZE (1024)

struct meta_pq_pqmap_rule_settings {
	uint16_t au16Nodes[META_PQ_PQMAP_NODE_SIZE];
	uint16_t au16NodesNum[META_PQMAP_MAX];
};

#define META_PQ_PQPARAM_VERSION (0)
struct meta_pq_pqparam {
	__u8  data[1024*3];
};

#define META_DISP_NON_LINEAR_SCALING_VERSION (0)
struct meta_disp_non_linear_scaling {
	__u8 u8HNL_En;
	__u16 u16SourceRatio;
	__u16 u16TargetRatio;
};

#define META_PQ_DISPLAY_WM_INFO_VERSION (1)
struct meta_pq_display_wm_info {
	struct meta_pq_wm_config wm_config;
	struct meta_pq_wm_result wm_result;
};

/* refer to m_pq_dv_info */
#define META_PQ_DV_INFO_VERSION (2)
#define META_PQ_DV_PYR_NUM      (7)
/* refer to struct m_pq_dv_pyr */
struct meta_pq_dv_pyr {
	bool  valid;
	__u8  frame_num;
	__u8  rw_diff;
	__u32 frame_pitch[META_PQ_DV_PYR_NUM];
	__u64 addr[META_PQ_DV_PYR_NUM];
	__u32 width[META_PQ_DV_PYR_NUM];
	__u32 height[META_PQ_DV_PYR_NUM];
};

/* refer to struct ST_DV_AMBIENT_INFO */
struct meta_pq_dv_ambient {
	__u32 u32Version;
	__u16 u16Length;

	bool bIsModeValid;
	__u32 u32Mode;
	bool bIsFrontLuxValid;
	signed long long s64FrontLux;
	bool  bIsRearLumValid;
	signed long long s64RearLum;
	bool bIsWhiteXYValid;
	__u32 u32WhiteX;
	__u32 u32WhiteY;

	bool bIsLsEnabledValid;
	bool bLsEnabled;
};

/* refer to struct m_pq_dv_pr_ctrl */
struct meta_pq_dv_pr_ctrl {
	bool en;
	__u32 fe_in_width;  /* input width of front end */
	__u32 fe_in_height; /* input height of front end */
	__u32 c1_in_width;  /* input width of core1 */
	__u32 c1_in_height; /* input height of core1 */
};

/* struct m_pq_dv_info */
struct meta_pq_dv_info {
	struct meta_pq_dv_pyr pyr;
	struct meta_pq_dv_pr_ctrl pr_ctrl;
	struct meta_pq_dv_ambient ambient;
};

#define META_PQ_BBD_INFO_VERSION (1)
struct meta_pq_bbd_info {
	__u8 u8Validity;
	__u16 u16LeftOuterPos;
	__u16 u16RightOuterPos;
	__u16 u16LeftInnerPos;
	__u16 u16LeftInnerConf;
	__u16 u16RightInnerPos;
	__u16 u16RightInnerConf;
};
#endif
