/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __UAPI_MTK_V4L2_VDEC_H__
#define __UAPI_MTK_V4L2_VDEC_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include "mtk-v4l2-vcodec.h"

enum v4l2_vdec_trick_mode {
	/* decode all frame */
	V4L2_VDEC_TRICK_MODE_ALL = 0,
	/* decode all except of non-reference frame */
	V4L2_VDEC_TRICK_MODE_IP,
	/* only decode I frame */
	V4L2_VDEC_TRICK_MODE_I
};

enum v4l2_vdec_ufo_mode {
	V4L2_VDEC_UFO_DEFAULT = 0,
	V4L2_VDEC_UFO_ON,
	V4L2_VDEC_UFO_OFF,
};

enum v4l2_vdec_aspect_ratio {
	V4L2_ASPECT_RATIO_NONE = 0,
	V4L2_ASPECT_RATIO_1_1,
	V4L2_ASPECT_RATIO_4_3,
	V4L2_ASPECT_RATIO_16_9,
	V4L2_ASPECT_RATIO_221_1,
	V4L2_ASPECT_RATIO_10_11,
	V4L2_ASPECT_RATIO_40_33,
	V4L2_ASPECT_RATIO_16_11,
	V4L2_ASPECT_RATIO_12_11,
	V4L2_ASPECT_RATIO_3_2,
	V4L2_ASPECT_RATIO_24_11,
	V4L2_ASPECT_RATIO_20_11,
	V4L2_ASPECT_RATIO_32_11,
	V4L2_ASPECT_RATIO_80_33,
	V4L2_ASPECT_RATIO_18_11,
	V4L2_ASPECT_RATIO_15_11,
	V4L2_ASPECT_RATIO_64_33,
	V4L2_ASPECT_RATIO_160_99,
	V4L2_ASPECT_RATIO_TRANSMIT,
	V4L2_ASPECT_RATIO_2_1,
	V4L2_ASPECT_RATIO_MAX
};

enum v4l2_vdec_bandwidth_type {
	V4L2_AV1_COMPRESS = 0,
	V4L2_AV1_NO_COMPRESS = 1,
	V4L2_HEVC_COMPRESS = 2,
	V4L2_HEVC_NO_COMPRESS = 3,
	V4L2_VSD = 4,
	V4L2_BW_COUNT = 5,
};

enum v4l2_vdec_subsample_mode {
	/*
	 * VDEC could generate subsample according to internal condition
	 * usually the condition is resolution > FHD
	 */
	V4L2_VDEC_SUBSAMPLE_DEFAULT = 0,
	/*
	 * VDEC should not generate subsample
	 * this setting will not affect buffer layout in order to support per-frame on/off
	 */
	V4L2_VDEC_SUBSAMPLE_OFF,
	/*
	 * VDEC should generate subsample
	 * this setting will not affect buffer layout in order to support per-frame on/off
	 */
	V4L2_VDEC_SUBSAMPLE_ON,
};

enum v4l2_vdec_dv_stream_highest_level {
	V4L2_VDEC_DV_STREAM_LEVEL_ID_UNSUPPORTED = 0,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_HD24,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_HD30,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_FHD24,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_FHD30,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_FHD60,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_UHD24,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_UHD30,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_UHD48,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_UHD60,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_UHD120,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_8K_UHD30,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_8K_UHD60,
	V4L2_VDEC_DV_STREAM_LEVEL_ID_8K_UHD120,
};

struct v4l2_vdec_dv_profile_level {
	int valid;
	int highest_level;    /* enum v4l2_vdec_dv_stream_highest_level */
} __attribute__((packed));

struct v4l2_vdec_color_desc {
	__u32 full_range;
	__u32 color_primaries;
	__u32 transform_character;
	__u32 matrix_coeffs;
	__u32 display_primaries_x[3];
	__u32 display_primaries_y[3];
	__u32 white_point_x;
	__u32 white_point_y;
	__u32 max_display_mastering_luminance;
	__u32 min_display_mastering_luminance;
	__u32 max_content_light_level;
	__u32 max_pic_light_level;
	__u32 is_hdr;
} __attribute__((packed));

struct v4l2_vdec_hdr10_info {
	__u8 matrix_coefficients;
	__u8 bits_per_channel;
	__u8 chroma_subsampling_horz;
	__u8 chroma_subsampling_vert;
	__u8 cb_subsampling_horz;
	__u8 cb_subsampling_vert;
	__u8 chroma_siting_horz;
	__u8 chroma_siting_vert;
	__u8 color_range;
	__u8 transfer_characteristics;
	__u8 colour_primaries;
	__u16 max_CLL;  /* CLL: Content Light Level */
	__u16 max_FALL; /* FALL: Frame Average Light Level */
	__u16 primaries[3][2];
	__u16 white_point[2];
	__u32 max_luminance;
	__u32 min_luminance;
} __attribute__((packed));

struct v4l2_vdec_hdr10plus_data {
	__u64 addr; /* user pointer */
	__u32 size;
};

struct v4l2_vdec_buffer_info {
	__u32 type; /* enum v4l2_vdec_buffer_type */
	__u64 addr;
	__u32 size;
} __attribute__((packed));

struct v4l2_vdec_resource_parameter {
	__u32 width;
	__u32 height;
	struct v4l2_fract frame_rate;
	__u32 priority; /* Smaller value means higher priority */
} __attribute__((packed));

struct v4l2_vdec_resource_metrics {
	__u32 core_used;  /* bit mask, if core-i is used, bit i is set */
	__u32 core_usage; /* unit is 1/1000 */
	__u8 gce;
} __attribute__((packed));

struct v4l2_vdec_bandwidth_info {
	/* unit is 1/1000 */
	/*[0] : av1 w/ compress, [1] : av1 w/o compress */
	/*[2] : hevc w/ compress, [3] : hevc w/o compress */
	/*[4] : vsd*/
	__u32 bandwidth_per_pixel[V4L2_BW_COUNT];
	__u8 compress;
	__u8 vsd;
} __attribute__((packed));

#define V4L2_PIX_FMT_RV30   v4l2_fourcc('R', 'V', '3', '0') /* RealVideo 8 */
#define V4L2_PIX_FMT_RV40   v4l2_fourcc('R', 'V', '4', '0') /* RealVideo 9/10 */
#define V4L2_PIX_FMT_AVS    v4l2_fourcc('A', 'V', 'S', '1') /* AVS */
#define V4L2_PIX_FMT_AVS2   v4l2_fourcc('A', 'V', 'S', '2') /* AVS2 */
#define V4L2_PIX_FMT_HEIF   v4l2_fourcc('H', 'E', 'I', 'F') /* HEIF */
#define V4L2_PIX_FMT_AV1    v4l2_fourcc('A', 'V', '1', '0') /* AV1 */
#define V4L2_PIX_FMT_S263   v4l2_fourcc('S', '2', '6', '3') /* Sorenson H.263(FLV1) */
#define V4L2_PIX_FMT_HEVCDV v4l2_fourcc('D', 'V', 'H', 'E') /* HEVCDV */
#define V4L2_PIX_FMT_SHVC   v4l2_fourcc('S', 'H', 'V', 'C') /* SHVC */
#define V4L2_PIX_FMT_AVIF   v4l2_fourcc('A', 'V', 'I', 'F') /* AVIF */
#define V4L2_PIX_FMT_AVCDV  v4l2_fourcc('D', 'V', 'A', 'V') /* AVCDV */
#define V4L2_PIX_FMT_H266   v4l2_fourcc('H', 'V', 'V', 'C') /* HVVC */
#define V4L2_PIX_FMT_AVS3   v4l2_fourcc('A', 'V', 'S', '3') /* AVS3 */
#define V4L2_PIX_FMT_MT21M    v4l2_fourcc('M', '2', '1', 'M') /* Mediatek yuv420 with meta data */

/* Mediatek control IDs */
#define V4L2_CID_MPEG_MTK_FRAME_INTERVAL (V4L2_CID_MPEG_MTK_BASE + 0)
#define V4L2_CID_MPEG_MTK_ERRORMB_MAP (V4L2_CID_MPEG_MTK_BASE + 1)
#define V4L2_CID_MPEG_MTK_DECODE_MODE (V4L2_CID_MPEG_MTK_BASE + 2)
#define V4L2_CID_MPEG_MTK_FRAME_SIZE (V4L2_CID_MPEG_MTK_BASE + 3)
#define V4L2_CID_MPEG_MTK_FIXED_MAX_FRAME_BUFFER (V4L2_CID_MPEG_MTK_BASE + 4)
#define V4L2_CID_MPEG_MTK_CRC_PATH (V4L2_CID_MPEG_MTK_BASE + 5)
#define V4L2_CID_MPEG_MTK_GOLDEN_PATH (V4L2_CID_MPEG_MTK_BASE + 6)
#define V4L2_CID_MPEG_MTK_ASPECT_RATIO (V4L2_CID_MPEG_MTK_BASE + 8)
#define V4L2_CID_MPEG_MTK_SET_WAIT_KEY_FRAME (V4L2_CID_MPEG_MTK_BASE + 9)
#define V4L2_CID_MPEG_MTK_SET_NAL_SIZE_LENGTH (V4L2_CID_MPEG_MTK_BASE + 10)
#define V4L2_CID_MPEG_MTK_SEC_DECODE (V4L2_CID_MPEG_MTK_BASE + 11)
#define V4L2_CID_MPEG_MTK_FIX_BUFFERS (V4L2_CID_MPEG_MTK_BASE + 12)
#define V4L2_CID_MPEG_MTK_FIX_BUFFERS_SVP (V4L2_CID_MPEG_MTK_BASE + 13)
#define V4L2_CID_MPEG_MTK_INTERLACING (V4L2_CID_MPEG_MTK_BASE + 14)
#define V4L2_CID_MPEG_MTK_CODEC_TYPE (V4L2_CID_MPEG_MTK_BASE + 15)
#define V4L2_CID_MPEG_MTK_OPERATING_RATE (V4L2_CID_MPEG_MTK_BASE + 16)
#define V4L2_CID_MPEG_MTK_SEC_ENCODE (V4L2_CID_MPEG_MTK_BASE + 17)
#define V4L2_CID_MPEG_MTK_QUEUED_FRAMEBUF_COUNT (V4L2_CID_MPEG_MTK_BASE + 18)

#define V4L2_CID_VDEC_DV_MAX_PROFILE_CNT (11)

/* Reference freed flags */
#define V4L2_BUF_FLAG_REF_FREED			0x00000200
/* Crop changed flags */
#define V4L2_BUF_FLAG_CROP_CHANGED		0x00008000
/* CSD data flags */
#define V4L2_BUF_FLAG_CSD			0x00200000
/* Need more bitstream flags
 * current bitstream needs to be combined with the next bitstream to
 * form a complete unit for decoder
 */
#define V4L2_BUF_FLAG_NEED_MORE_BS		0x80000000

/* Mediatek VDEC private event */
#define V4L2_EVENT_MTK_VDEC_START	(V4L2_EVENT_PRIVATE_START + 0x00003000)
#define V4L2_EVENT_MTK_VDEC_ERROR	(V4L2_EVENT_MTK_VDEC_START + 1)
#define V4L2_EVENT_MTK_VDEC_NOHEADER	(V4L2_EVENT_MTK_VDEC_START + 2)
#define V4L2_EVENT_MTK_VDEC_BITSTREAM_UNSUPPORT (V4L2_EVENT_MTK_VDEC_START + 3)

/* used for struct v4l2_event_src_change */
#define V4L2_EVENT_SRC_CH_FRAMERATE	(1 << 31)

/* used for struct v4l2_event_src_change */
#define V4L2_EVENT_SRC_CH_SCANTYPE	(1 << 30)

/* MTK additional set/get control */
#define V4L2_CTRL_CLASS_VDEC (V4L2_CTRL_CLASS_MPEG | 0x3000)

/* Mediatek control IDs */
#define V4L2_CID_VDEC_BASE (V4L2_CTRL_CLASS_VDEC)
#define V4L2_CID_VDEC_TRICK_MODE (V4L2_CID_VDEC_BASE + 1)
/* enum v4l2_vdec_trick_mode [set][get] */
#define V4L2_CID_VDEC_DV_SUPPORTED_PROFILE_LEVEL (V4L2_CID_VDEC_BASE + 2)
#define V4L2_CID_VDEC_UFO_MODE (V4L2_CID_VDEC_BASE + 3) /* [set] */
/* struct v4l2_vdec_dv_profile_level[V4L2_CID_VDEC_DV_MAX_PROFILE_CNT] */
#define V4L2_CID_VDEC_HDR10_INFO (V4L2_CID_VDEC_BASE + 4)
/* struct v4l2_vdec_hdr10_info [set] */
#define V4L2_CID_VDEC_NO_REORDER (V4L2_CID_VDEC_BASE + 5) /* [set] */
#define V4L2_CID_VDEC_SECURE_MODE (V4L2_CID_VDEC_BASE + 6) /* [set] */
#define V4L2_CID_VDEC_SECURE_PIPEID (V4L2_CID_VDEC_BASE + 7) /* [set] */
#define V4L2_CID_VDEC_COLOR_DESC (V4L2_CID_VDEC_BASE + 9)
/* struct v4l2_vdec_color_desc [get] */
#define V4L2_CID_VDEC_SUBSAMPLE_MODE (V4L2_CID_VDEC_BASE + 10)
/* enum v4l2_vdec_subsample_mode [set] */
#define V4L2_CID_VDEC_INCOMPLETE_BITSTREAM (V4L2_CID_VDEC_BASE + 11) /* [set] */
/* set this if input buffers do not end at frame boundaries */
#define V4L2_CID_VDEC_HDR10PLUS_DATA (V4L2_CID_VDEC_BASE + 13) /* [set] */
/* struct v4l2_vdec_hdr10plus_data */
#define V4L2_CID_VDEC_ACQUIRE_RESOURCE (V4L2_CID_VDEC_BASE + 14) /* [set] */
/* struct v4l2_vdec_resource_parameter */
#define V4L2_CID_VDEC_RESOURCE_METRICS (V4L2_CID_VDEC_BASE + 15) /* [get] */
/* struct v4l2_vdec_resource_metrics */
#define V4L2_CID_VDEC_SLICE_COUNT (V4L2_CID_VDEC_BASE + 16) /* [set] */
#define V4L2_CID_VDEC_DETECT_TIMESTAMP (V4L2_CID_VDEC_BASE + 17) /* [set] */
#define V4L2_CID_VDEC_VPEEK_MODE (V4L2_CID_VDEC_BASE + 18) /* [set] */
#define V4L2_CID_VDEC_PLUS_DROP_RATIO (V4L2_CID_VDEC_BASE + 19) /* [set] */
#define V4L2_CID_VDEC_CONTAINER_FRAMERATE (V4L2_CID_VDEC_BASE + 20) /* [set] */
/* Framerate base is 1000 */
#define V4L2_CID_VDEC_BANDWIDTH_INFO (V4L2_CID_VDEC_BASE + 21) /* [get] */
/* struct v4l2_vdec_bandwidth_info */
#endif /* #ifndef __UAPI_MTK_V4L2_VDEC_H__ */
