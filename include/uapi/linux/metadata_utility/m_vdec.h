/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __UAPI_MTK_M_VDEC_H__
#define __UAPI_MTK_M_VDEC_H__

#include <linux/types.h>

enum tile_mode {
	tile_32x32,
	tile_16x32,
	tile_32x16,
	tile_4x2_compression,
	tile_32x16_82pack,
};

struct data_info {
	__u64 offset;
	__u32 size;
	__u32 reserve;
};

enum fmt_modifier_vsd {
	VSD_NONE = 0,
	VSD_8x2 = 1,
	VSD_8x4 = 2,
};

enum vdec_dolby_type {
	VDEC_DOLBY_NONE = 0,
	VDEC_DOLBY_RPU = 1,
	VDEC_DOLBY_SEMPT2094 = 2,
};

enum vdec_cuva_hdr_codec_type {
	VDEC_CUVA_HDR_HEVC = 0,
	VDEC_CUVA_HDR_AVS2 = 1,
	VDEC_CUVA_HDR_AVS3 = 2,
	VDEC_CUVA_HDR_VVC = 3,
};

enum vdec_codec_type {
	VDEC_TYPE_MPV = 0,
	VDEC_TYPE_MP4 = 1,
	VDEC_TYPE_H264 = 2,
	VDEC_TYPE_VC1 = 3,
	VDEC_TYPE_WMV = 4,
	VDEC_TYPE_MJPEG = 5,
	VDEC_TYPE_RV = 6,
	VDEC_TYPE_AVS = 7,
	VDEC_TYPE_VP6 = 8,
	VDEC_TYPE_VP8 = 9,
	VDEC_TYPE_RAW = 10,
	VDEC_TYPE_OGG = 11,
	VDEC_TYPE_JPG = 12,
	VDEC_TYPE_PNG = 13,
	VDEC_TYPE_H265 = 14,
	VDEC_TYPE_VP9 = 15,
	VDEC_TYPE_DV_H264 = 16,
	VDEC_TYPE_DV_H265 = 17,
	VDEC_TYPE_AVS2 = 18,
	VDEC_TYPE_SHVC = 19,
	VDEC_TYPE_AV1 = 20,
	VDEC_TYPE_H266 = 21,
	VDEC_TYPE_AVS3 = 22,
	VDEC_TYPE_MAX = 23,
};

struct fmt_modifier {
	union {
		struct {
			__u8 tile:1;
			__u8 raster:1;
			__u8 compress:1;
			__u8 jump:1;
			__u8 vsd_mode:3; /* enum fmt_modifier_vsd */
			__u8 vsd_ce_mode:1;
		};
		/* TODO: Reserve several bits in MSB for version control */
		__u64 padding;
	};
};

struct src_info {
	__u32 width;
	__u32 height;
	__u32 pitch;
	__u32 tile_mode; /* enum tile_mode */
	struct fmt_modifier modifier;
};

/* for struct vdec_dd_frm_info */
#define MTK_VDEC_DD_FRM_INFO_TAG	"MTK_VDEC_DD_FRMINFO"
/* for struct vdec_dd_compress_info */
#define MTK_VDEC_DD_COMPRESS_INFO_TAG	"MTK_VDEC_DD_COMPRESSINFO"
/* for struct vdec_dd_color_desc */
#define MTK_VDEC_DD_COLOR_DESC_TAG	"MTK_VDEC_DD_COLORDESC"
/* for struct vdec_dd_frm_statistic */
#define MTK_VDEC_DD_FRM_STATISTIC_TAG	"MTK_VDEC_DD_FRM_STATISTIC"
/* for struct vdec_dd_closed_caption */
#define MTK_VDEC_DD_CLOSED_CAPTION_TAG	"MTK_VDEC_DD_CLOSED_CAPTION"
/* for struct vdec_dd_svp_info */
#define MTK_VDEC_DD_SVP_INFO_TAG	"MTK_VDEC_DD_SVPINFO"
/* for struct vdec_dd_hdr10plus_desc */
#define MTK_VDEC_DD_HDR10PLUS_DESC_TAG	"MTK_VDEC_DD_HDR10PLUSDESC"
/* for struct vdec_dd_fb_hw_crc */
#define MTK_VDEC_DD_FB_HW_CRC_TAG	"MTK_VDEC_DD_FBHWCRC"
/* for struct vdec_dd_svp_buf */
#define MTK_VDEC_DD_SVP_BUF_TAG		"MTK_VDEC_DD_SVPBUF"
/* for struct vdec_dd_dolby_desc */
#define MTK_VDEC_DD_DOLBY_DESC_TAG	"MTK_VDEC_DD_DOLBYDESC"
/* for struct vdec_dd_film_grain_desc */
#define MTK_VDEC_DD_FG_DESC_TAG		"MTK_VDEC_DD_FGDESC"
// for struct vdec_dd_cuva_hdr_desc
#define MTK_VDEC_DD_CUVAHDR_DESC_TAG	"MTK_VDEC_DD_CUVAHDRDESC"
// for struct vdec_dd_sl_hdr_desc
#define MTK_VDEC_DD_SLHDR_DESC_TAG	"MTK_VDEC_DD_SLHDRDESC"
// for struct vdec_dd_dv_parsing
#define MTK_VDEC_DD_DV_PARSING_TAG	"MTK_VDEC_DD_DVPARSING"

#define AFD_MAX_VALUE 15 /* max value of active_format_description, according to wiki */
#define FPA_MAX_VALUE 8  /* max value of frame_packing_type */
			 /* from MPEG2 and H264, 3: side-by-side LR, 4: top-bottom, 8: 3D */

struct vdec_dd_frm_info {
	struct data_info luma_data_info[2];
	struct data_info chroma_data_info[2];
	struct data_info subsample_luma_data_info;
	struct data_info subsample_chroma_data_info;
	struct src_info frm_src_info[2];
	struct src_info subsample_frm_src_info;
	__u32 crop_left;
	__u32 crop_top;
	__u32 crop_width;
	__u32 crop_height;
	__u32 sar_width;
	__u32 sar_height;
	__u32 vsd_crop_left;
	__u32 vsd_crop_top;
	__u32 vsd_crop_width;
	__u32 vsd_crop_height;
	/* not necessary valid since framerate info in only optional in bitstream */
	__u32 framerate; /* unit is 1/1000 frame per second */
	__u32 counter; /* 0 based index */
	__u8 bitdepth;
	__u8 top_first;
	__u8 repeat_first_field;
	__u8 chroma_location_info_present;
	__u32 chroma_location_type_top;
	__u32 chroma_location_type_bottom;
	__u8 active_format_description;
	__u8 overscan_info_present;
	__u8 overscan_appropriate;
	__u8 frame_packing_type;
	__u8 content_interpretation_type;
	__u32 codec_bit_count;
	__u8 scaling_list_intra[64];
	__u8 scaling_list_inter[64];
	__u32 user_data_size;
	__u8 user_data[128];
	__u32 display_horizontal_size;
	__u32 display_vertical_size;
	__u8 monochrome;
	__u8 is_filmmaker;
	enum vdec_codec_type codec_type;
	__u32 toggle_time;
	__u8 ecosystem_ide;
} __attribute__((packed));

/*  Flags for 'flag' field */
#define VDEC_COMPRESS_FLAG_UNCOMPRESS		(1 << 0)
#define VDEC_COMPRESS_FLAG_VP9			(1 << 1)
#define VDEC_COMPRESS_FLAG_AV1			(1 << 2)
#define VDEC_COMPRESS_FLAG_AV1_PPU_BYPASS	(1 << 3)

struct vdec_dd_compress_info {
	struct data_info luma_data_info;
	struct data_info chroma_data_info;
	__u32 pitch;
	__u32 flag;
	__u8 engine_id;
} __attribute__((packed));

struct vdec_dd_color_desc {
	__u32 max_luminance;
	__u32 min_luminance;
	__u16 primaries[3][2];
	__u16 white_point[2];
	__u16 max_content_light_level;
	__u16 max_pic_average_light_level;
	__u8 colour_primaries;
	__u8 transfer_characteristics;
	__u8 matrix_coefficients;
	__u8 video_full_range;
	__u8 is_hdr;
};

struct vdec_dd_frm_statistic {
	__u32 histogram[32];
	__u32 cplx;
	__s32 min_qp;
	__s32 max_qp;
	__s32 pps_qp;
	__s32 slice_qp;
	__s32 mb_qp;
	__u16 err_mb_rate;
	__u8 statistic_enable; /* histogram and cplx enable */
} __attribute__((packed));

struct vdec_dd_closed_caption {
	__u64 timestamp; /* unit is us */
	__u32 data_size;
	__u8 data[256];
	__u8 top_first;
	__u8 frame_type;
	__u16 temproal_ref;
	__u8 u8CcPtkCnt;
	__u32 u32CcPtkOffset[8];
} __attribute__((packed));

enum vdec_aid {
	vdec_aid_ns = 0,	/* non secure */
	vdec_aid_sdc,		/* secure downscaled capture */
	vdec_aid_s,		/* secure */
	vdec_aid_csp,		/* content service provider */
};

struct vdec_dd_svp_info {
	__u8 aid;
	__u32 pipeline_id;
} __attribute__((packed));

struct vdec_dd_hdr10plus_desc {
	__u8 itu_t_t35_country_code;
	__u16 terminal_provider_code;
	__u16 terminal_provider_oriented_code;
	__u8 application_identifier;
	__u8 application_version;
	__u8 num_windows;
	__u32 targeted_system_display_maximum_luminance;
	__u8 targeted_system_display_actual_peak_luminance_flag;
	__u32 maxscl[3];
	__u32 average_maxrgb;
	__u8 num_distribution_maxrgb_percentiles;
	__u8 distribution_maxrgb_percentages[9];
	__u32 distribution_maxrgb_percentiles[9];
	__u16 fraction_bright_pixels;
	__u8 mastering_display_actual_peak_luminance_flag;
	__u8 tone_mapping_flag;
	__u16 knee_point_x;
	__u16 knee_point_y;
	__u8 num_bezier_curve_anchors;
	__u16 bezier_curve_anchors[9];
	__u8 color_saturation_mapping_flag;
};

struct vdec_dd_film_grain_desc {
	__u8 apply_grain;
	__u16 grain_seed;
	__u8 update_grain;
	__u8 film_grain_params_ref_idx;
	__u8 num_y_points;
	__u8 point_y_value[14];
	__u8 point_y_scaling[14];
	__u8 chroma_scaling_from_luma;
	__u8 num_cb_points;
	__u8 point_cb_value[10];
	__u8 point_cb_scaling[10];
	__u8 num_cr_points;
	__u8 point_cr_value[10];
	__u8 point_cr_scaling[10];
	__u8 grain_scaling;
	__u8 ar_coeff_lag;
	__s32 ar_coeffs_y[24];
	__s32 ar_coeffs_cb[25];
	__s32 ar_coeffs_cr[25];
	__u8 ar_coeff_shift;
	__u8 grain_scale_shift;
	__u8 cb_mult;
	__u8 cb_luma_mult;
	__u16 cb_offset;
	__u8 cr_mult;
	__u8 cr_luma_mult;
	__u16 cr_offset;
	__u8 overlap_flag;
	__u8 clip_to_restricted_range;
	__u8 mc_identity;
	__u8 bit_depth;
	__u8 monochrome;

};

#define METADATA_MAX_CUVA_SIZE  256
#define METADATA_MAX_TCH_SIZE   256

struct vdec_dd_cuva_hdr_desc {
	enum vdec_cuva_hdr_codec_type cuva_hdr_codec_type;
	__u32 data_size;
	__u8 cuva_metadata[METADATA_MAX_CUVA_SIZE];
};

struct vdec_dd_sl_hdr_desc {
	__u32 data_size;
	__u8 sl_hdr_metadata[METADATA_MAX_TCH_SIZE];
};

struct vdec_dd_fb_hw_crc {
	__u32 luma_crc[12];
	__u32 chroma_crc[12];
} __attribute__((packed));

struct vdec_dd_svp_buf {
	__u8 authorized;
} __attribute__((packed));

#define MTK_VDEC_DD_DOLBY_DESC_VERSION (0)
struct vdec_dd_dolby_desc {
	enum vdec_dolby_type dolby_type;
	__u32 data_size;
	union {
		__u8 rpu_data[1024];
		__u8 sei_2094_data[256];
	};
};

struct vdec_dd_dolby_desc_parsing {
	enum vdec_dolby_type dolby_type;
	__u32 dm_data_size;
	__u8 dm_data[1024];
	__u32 comp_data_size;
	__u8 comp_data[2048];
};
#endif /* #ifndef __UAPI_MTK_M_VDEC_H__ */
