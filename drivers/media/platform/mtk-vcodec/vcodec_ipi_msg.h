/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _VCODEC_IPI_MSG_H_
#define _VCODEC_IPI_MSG_H_

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

enum mtk_venc_hw_id {
	MTK_VENC_CORE_0 = 0,
	MTK_VENC_CORE_1 = 1,
	MTK_VENC_HW_NUM = 2,
};

enum mtk_vdec_hw_id {
	MTK_VDEC_CORE = 0, // keep the original one
	MTK_VDEC_CORE_0 = 0,
	MTK_VDEC_CORE_1 = 1,
	MTK_VDEC_LAT = 2,
	MTK_VDEC_LAT1 = 3,
	MTK_VDEC_HW_NUM = 4,
};

enum mtk_fmt_type {
	MTK_FMT_DEC = 0,
	MTK_FMT_ENC = 1,
	MTK_FMT_FRAME = 2,
};

enum mtk_frame_type {
	MTK_FRAME_NONE = 0,
	MTK_FRAME_I = 1,
	MTK_FRAME_P = 2,
	MTK_FRAME_B = 3,
};

enum mtk_bandwidth_type {
	MTK_AV1_COMPRESS = 0,
	MTK_AV1_NO_COMPRESS = 1,
	MTK_HEVC_COMPRESS = 2,
	MTK_HEVC_NO_COMPRESS = 3,
	MTK_VSD = 4,
	MTK_BW_COUNT = 5,
};

// smaller value means higher priority
enum vdec_priority {
	VDEC_PRIORITY_REAL_TIME = 0,
	VDEC_PRIORITY_NON_REAL_TIME1,
	VDEC_PRIORITY_NON_REAL_TIME2,
	VDEC_PRIORITY_NON_REAL_TIME3,
	VDEC_PRIORITY_NUM,
};

/**
 * struct mtk_video_fmt - Structure used to store information about pixelformats
 */
struct mtk_video_fmt {
	__u32	fourcc;
	__u32	type;   /* enum mtk_fmt_type */
	__u32	num_planes;
};

/**
 * struct mtk_codec_framesizes - Structure used to store information about
 *							framesizes
 */
struct mtk_codec_framesizes {
	__u32	fourcc;
	__u32	profile;
	__u32	level;
	struct	v4l2_frmsize_stepwise	stepwise;
};

/**
 * struct mtk_video_frame_frameintervals - Structure used to store information about
 *							frameintervals
 * fourcc/width/height are input parameters
 * stepwise is output parameter
 */
struct mtk_video_frame_frameintervals {
	__u32   fourcc;
	__u32   width;
	__u32   height;
	struct v4l2_frmival_stepwise stepwise;
};

struct mtk_color_desc {
	__u32   colour_description;
	__u32	color_primaries;
	__u32	transform_character;
	__u32	matrix_coeffs;
	__u32	display_primaries_x[3];
	__u32	display_primaries_y[3];
	__u32	white_point_x;
	__u32	white_point_y;
	__u32	max_display_mastering_luminance;
	__u32	min_display_mastering_luminance;
	__u32	max_content_light_level;
	__u32	max_pic_light_level;
	__u32   is_hdr;
	__u32   full_range;
};

struct mtk_video_quality {
	__u32   video_qp_i_max;
	__u32   video_qp_i_min;
	__u32   video_qp_p_max;
	__u32   video_qp_p_min;
};

struct mtk_hdr_dynamic_info {
	__u32    max_sc_lR;
		// u(17); Max R Nits *10; in the range of 0x00000-0x186A0
	__u32    max_sc_lG;
		// u(17); Max G Nits *10; in the range of 0x00000-0x186A0
	__u32    max_sc_lB;
		// u(17); Max B Nits *10; in the range of 0x00000-0x186A0
	__u32    avg_max_rgb;
		// u(17); Average maxRGB Nits *10; in 0x00000-0x186A0
	__u32    distribution_values[9];
		/* u(17)
		 * 0=1% percentile maxRGB Nits *10
		 * 1=Maximum Nits of 99YF *10
		 * 2=Average Nits of DPY100F
		 * 3=25% percentile maxRGB Nits *10
		 * 4=50% percentile maxRGB Nits *10
		 * 5=75% percentile maxRGB Nits *10
		 * 6=90% percentile maxRGB Nits *10
		 * 7=95% percentile maxRGB Nits *10
		 * 8=99.95% percentile maxRGB Nits *10
		 */
};

/**
 * struct vdec_pic_info  - picture size information
 * @pic_w: picture width
 * @pic_h: picture height
 * @buf_w   : picture buffer width (codec aligned up from pic_w)
 * @buf_h   : picture buffer heiht (codec aligned up from pic_h)
 * @fb_sz: frame buffer size
 * @bitdepth: Sequence bitdepth
 * @layout_mode: mediatek frame layout mode
 * @fourcc: frame buffer color format
 * @field: enum v4l2_field, field type of this sequence
 * E.g. suppose picture size is 176x144,
 *      buffer size will be aligned to 176x160.
 */
struct vdec_pic_info {
	__u32 pic_w;
	__u32 pic_h;
	__u32 buf_w;
	__u32 buf_h;
	__u32 fb_sz[VIDEO_MAX_PLANES];
	__u32 bitdepth;
	__u32 layout_mode;
	__u32 fourcc;
	__u32 field;
};

/**
 * struct vdec_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @vdec_changed_info  : some changed flags
 * @bs_va		: Input bit-stream buffer kernel virtual address
 * @bs_dma		: Input bit-stream buffer dma address
 * @bs_fd               : Input bit-stream buffer dmabuf fd
 * @fb_dma		: Y frame buffer dma address
 * @fb_fd             : Y frame buffer dmabuf fd
 * @fb_length		: frame buffer length
 * @vdec_bs_va		: VDEC bitstream buffer struct virtual address
 * @vdec_fb_va		: VDEC frame buffer struct virtual address
 * @fb_num_planes	: frame buffer plane count
 * @timestamp		: frame timestamp in ns
 */
struct vdec_dec_info {
	__u32 dpb_sz;
	__u32 vdec_changed_info;
#ifdef TV_INTEGRATION
	__u64 bs_va;
#endif
	__u64 bs_dma;
	__u64 bs_fd;
	__u64 fb_dma[VIDEO_MAX_PLANES];
	__u64 fb_va[VIDEO_MAX_PLANES];
	__u64 fb_fd[VIDEO_MAX_PLANES];
	__u32 fb_length[VIDEO_MAX_PLANES];
	__u64 vdec_bs_va;
	__u64 vdec_fb_va;
	__u32 fb_num_planes;
	__u32 index;
	__u32 wait_key_frame;
	__u32 error_map;
	__u64 timestamp;
	__u32 queued_frame_buf_count;
};

#define HDR10_PLUS_MAX_SIZE              (128)

struct hdr10plus_info {
	__u8 data[HDR10_PLUS_MAX_SIZE];
	__u32 size;
};

struct vdec_resource_info {
	__u32 usage; /* unit is 1/1000 */
	bool hw_used[MTK_VDEC_HW_NUM]; /* index MTK_VDEC_LAT is not used for now */
	bool gce;
	enum vdec_priority priority;
};

struct vdec_bandwidth_info {
	/* unit is 1/1000 */
	/*[0] : av1 compress, [1] : av1 no compress */
	/*[2] : hevc compress, [3] : hevc no compress */
	/*[4] : vsd*/
	__u32 bandwidth_per_pixel[MTK_BW_COUNT];
	bool compress;
	bool vsd;
};
#endif
