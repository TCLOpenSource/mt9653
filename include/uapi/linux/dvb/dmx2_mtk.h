/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef DMX2_MTK_H
#define DMX2_MTK_H

#include "dmx2.h"

/* mtk private event */
#define DMX2_EVENT_MTK_BASE		(DMX2_EVENT_PRIVATE_BASE + 0x10)
/* mtk private ctrl */
#define DMX2_CID_MTK_BASE		(DMX2_CID_PRIVATE_BASE + 0x10)

/* mtk capabilities */
#define DMX2_CAPS_VIDEO_FILTER_NUMBER	"video_filter_numb"
#define DMX2_CAPS_AUDIO_FILTER_NUMBER	"audio_filter_numb"
#define DMX2_CAPS_SECTION_FILTER_NUMBER	"section_filter_numb"
#define DMX2_CAPS_PCR_FILTER_NUMBER	"pcr_filter_numb"
#define DMX2_CAPS_SECTION_FILTER_SIZE	"section_filter_size"

enum dmx2_event_scramble_status {
	DMX2_UNKNOWN = 1 << 0,
	DMX2_NOT_SCRAMBLED = 1 << 1,
	DMX2_SCRAMBLED = 1 << 2,
};

enum dmx2_sc_index_type {
	DMX2_SC_MPEG_AVC_AVS,
	DMX2_SC_HEVC
};

/* General Key frame info for all video codec type */
enum dmx2_sc_index {
	/* Start Code is for a new I Frame */
	DMX2_I_FRAME = 1 << 0,
	/* Start Code is for a new P Frame */
	DMX2_P_FRAME = 1 << 1,
	/* Start Code is for a new B Frame */
	DMX2_B_FRAME = 1 << 2,
	/* Start Code is for a new Sequence */
	DMX2_SEQUENCE = 1 << 3,
	/* Start Code is for a new I Slice */
	DMX2_I_SLICE = 1 << 4,
	/* Start Code is for a new P Slice */
	DMX2_P_SLICE = 1 << 5,
	/* Start Code is for a new B Slice */
	DMX2_B_SLICE = 1 << 6,
	/* Start Code is for a new switch I Slice */
	DMX2_SI_SLICE = 1 << 7,
	/* Start Code is for a new switch P Slice */
	DMX2_SP_SLICE = 1 << 8,
};

/* Key frame info for HEVC codec type */
enum dmx2_sc_hevc_index {
	DMX2_SPS = 1 << 0,
	DMX2_AUD = 1 << 1,
	DMX2_SLICE_CE_BLA_W_LP = 1 << 2,
	DMX2_SLICE_BLA_W_RADL = 1 << 3,
	DMX2_SLICE_BLA_N_LP = 1 << 4,
	DMX2_SLICE_IDR_W_RADL = 1 << 5,
	DMX2_SLICE_IDR_N_LP = 1 << 6,
	DMX2_SLICE_TRAIL_CRA = 1 << 7,
};

enum dmx2_ts_index {
	DMX2_TS_ID_PAYLOAD_UNIT_START_INDICATOR = 1 << 0,
	DMX2_TS_ID_DISCONTINUITY_INDICATOR = 1 << 1,
};

enum dmx2_mmtp_index {
	DMX2_MMTP_ID_RANDOM_ACCESS_POINT = 1 << 0, /* RAP_FLAG */
	DMX2_MMTP_ID_MPT = 1 << 1,
	DMX2_MMTP_ID_VIDEO = 1 << 2,
	DMX2_MMTP_ID_AUDIO = 1 << 3,
};

enum dmx2_input_type {
	DMX2_INPUT_TYPE_DRAM,
	DMX2_INPUT_TYPE_INTERNAL_FRONTEND,
	DMX2_INPUT_TYPE_EXTERNAL_FRONTEND,
	DMX2_INPUT_TYPE_MAX
};

enum dmx2_port_type {
	DMX2_PORT_TYPE_PARALLEL,
	DMX2_PORT_TYPE_SERIAL,
	DMX2_PORT_TYPE_SERIAL_2BITS,
	DMX2_PORT_TYPE_MAX
};

#define dmx2_fourcc(a, b, c, d)\
	((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))
#define DMX2_CODEC_VIDEO_MPEG	dmx2_fourcc('M', 'P', 'E', 'G') /* MPEG-1/2/4 Multiplexed */
#define DMX2_CODEC_VIDEO_HEVC	dmx2_fourcc('H', 'E', 'V', 'C') /* HEVC aka H.265 */
#define DMX2_CODEC_VIDEO_H264	dmx2_fourcc('H', '2', '6', '4') /* H264  */

// ??? by key token or tuner hal
typedef enum {
	DVR2_ENCRYPT_TYPE_NONE,
	DVR2_ENCRYPT_TYPE_PAYLOAD,
	DVR2_ENCRYPT_TYPE_BLOCK
} dvr2_encrypt_type;

typedef enum {
	DVR2_OUT_TS,
	DVR2_OUT_TTS,   // timestamp tag(4byte) + TS (188)
	DVR2_OUT_TLV,
	DVR2_OUT_ALP,
} dvr2_format;

typedef enum {
	DVR2_TIMESTAMP_TYPE_REC_TIME,
	DVR2_TIMESTAMP_TYPE_PLAY_TIME,
	DVR2_TIMESTAMP_TYPE_PKT_TIME
} dvr2_timestamp_type;

struct dmx2_record_parser_params {
	union {
		__u32 ts_index;		// dmx2_ts_index
		__u32 mmtp_index;	// dmx2_mmtp_index
	} index;
	__u32 codec_type; // FourCC of the stream codec type.
	enum dmx2_sc_index_type sc_type;
	__u32 sc_index;     // dmx2_sc_index or dmx2_sc_hevc_index
	bool mpu_sn_to_pts;
};

struct dmx2_source_settings {
	enum dmx2_input_type input_type;
	enum dmx2_port_type port_type;
	__u8 port_num;
	bool with_CICAM;
};

struct dvr2_record_settings {
	dvr2_format format;
	bool rec_all_input;  // record all content from dmx input or not
	__u32       reserved[3];
};

struct dvr2_playback_settings {
	dvr2_format format;
	dvr2_encrypt_type encrypt_type;
	__u32       reserved[3];
};

struct dvr2_timeinfo {
	dvr2_timestamp_type type;
	__u64 timestamp;
};

//---get metadata----------------------------------------------------

struct dmx2_record_parser_ts_metadata {
	__u32 ts_index;	// dmx2_ts_index
	__u32 sc_index;	// dmx2_sc_index or dmx2_sc_hevc_index
	__u64 timestamp;
	__u32 first_mb_in_slice;
	__u64 byte_number;
	__u32 reserved[4];
};

struct dmx2_record_parser_mmtp_metadata {
	__u32 mmtp_index;
	__u32 sc_index;     // dmx2_sc_index or dmx2_sc_hevc_index
	__u64 timestamp;
	__u32 mpu_seq_numb;
	__u32 first_mb_in_slice;
	__u64 byte_number;
	__u32 reserved[4];
};

struct dmx2_temi_metadata {
	__u64 pts;
	__u8 descr_tag;
	__u8 descr_data[64]; // 64 is TBD
};


// proprietary filter subtype
#define DMX2_TS_PRIV_DATA_MTK_BASE		(DMX2_TS_DATA_PRIVATE_BASE+0)
/* private param: n/a */
#define DMX2_TS_PRIV_DATA_TEMI_AF		(DMX2_TS_PRIV_DATA_MTK_BASE+1)

#define DMX2_ALP_PRIV_DATA_MTK_BASE		(DMX2_ALP_DATA_PRIVATE_BASE+0)
/* private param: n/a */
#define DMX2_ALP_PRIV_DATA_PTP			(DMX2_ALP_PRIV_DATA_MTK_BASE+1)

// Proprietary event type
/* data = enum dmx2_event_scramble_status */
#define DMX2_EVENT_SCRAMBLING_STATUS		(DMX2_EVENT_MTK_BASE+0)
/* data = u32 */
#define DMX2_EVENT_IP_CID_CHANGE		(DMX2_EVENT_MTK_BASE+1)
/* data = n/a, reset after flush() */
#define DMX2_EVENT_STC_DATA_READY		(DMX2_EVENT_MTK_BASE+2)

//Proprietary control ID
/* Set: uint32 */
#define DMX2_CID_PRESET_IP_CID			(DMX2_CID_MTK_BASE+0)
/* Set: dmx2_record_parser_params, setup ts index, sc index to demux filter. */
#define DMX2_CID_RECORD_PARSER			(DMX2_CID_MTK_BASE+1)
/* Set: dvr2_record_settings , set to dvr fd only */
#define DMX2_CID_RECORD_SETTINGS		(DMX2_CID_MTK_BASE+2)
/* Set: dvr2_playback_settings , set to dvr fd only */
#define DMX2_CID_PLAYBACK_SETTING		(DMX2_CID_MTK_BASE+3)

// CID cmd only used in ref design
/*
 * Set: only for TEMI descriptor in TS AF.
 * not support the TEMI located in PES payload,
 * filter output with TS packet with TS header
 * only contained PES header or TEMI descriptor.
 */
#define DMX2_CID_RECORD_TIME_FILTER		(DMX2_CID_MTK_BASE+4)

/* Set: dmx2_source_settings */
#define DMX2_CID_SET_SOURCE			(DMX2_CID_MTK_BASE+5)

/* Set: struct dmx2_stc */
#define DMX2_CID_SET_STC			(DMX2_CID_MTK_BASE+6)

/* Get: uint32 */
#define DMX2_CID_GET_HW_FILTER_ID		(DMX2_CID_MTK_BASE+7)

/* Set: struct dvr2_timeinfo */
#define DMX2_CID_SET_TIME_FILTER		(DMX2_CID_MTK_BASE+8)

/* Get: struct dvr2_timeinfo */
#define DMX2_CID_GET_TIME_FILTER		(DMX2_CID_MTK_BASE+9)

#endif /* DMX2_MTK_H */
