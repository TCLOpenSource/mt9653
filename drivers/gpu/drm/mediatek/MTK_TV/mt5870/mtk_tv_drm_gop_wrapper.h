/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_UTP_WRAPPER_
#define _MTK_TV_DRM_UTP_WRAPPER_

#define GOP_CSC_STATE_CHANGE (0)

typedef enum {
	/// Color format ARGB8888.
	UTP_E_GOP_COLOR_ARGB8888,
	/// Color format ABGR8888.
	UTP_E_GOP_COLOR_ABGR8888,
	/// Color format RGBA8888.
	UTP_E_GOP_COLOR_RGBA8888,
	/// Color format BGRA8888.
	UTP_E_GOP_COLOR_BGRA8888,
	/// Color format BGRA8888.
	UTP_E_GOP_COLOR_XRGB8888,	// mode 1
	/// Color format RGB565.
	UTP_E_GOP_COLOR_RGB565,
	/// Color format ARGB4444.
	UTP_E_GOP_COLOR_ARGB4444,
	/// Color format ARGB1555.
	UTP_E_GOP_COLOR_ARGB1555,
	/// Color format YUV422.
	UTP_E_GOP_COLOR_YUV422,
	/// Color format RGBA5551.
	UTP_E_GOP_COLOR_RGBA5551,
	/// Color format RGBA4444.
	UTP_E_GOP_COLOR_RGBA4444,
	// Color format BGR565.
	UTP_E_GOP_COLOR_BGR565,
	// Color format ABGR4444.
	UTP_E_GOP_COLOR_ABGR4444,
	// Color format ABGR1555.
	UTP_E_GOP_COLOR_ABGR1555,
	// Color format BGRA5551.
	UTP_E_GOP_COLOR_BGRA5551,
	// Color format BGRA4444.
	UTP_E_GOP_COLOR_BGRA4444,
	/// Color format ABGR8888.
	UTP_E_GOP_COLOR_XBGR8888,	// mode 1
	/// Color format UYVY.
	UTP_E_GOP_COLOR_UYVY,
	/// Invalid color format.
	UTP_E_GOP_COLOR_INVALID
} UTP_EN_GOP_COLOR_TYPE;

typedef enum {
	UTP_E_GOP_COLOR_YCBCR_BT601,
	UTP_E_GOP_COLOR_YCBCR_BT709,
	UTP_E_GOP_COLOR_YCBCR_BT2020,
	UTP_E_GOP_COLOR_RGB_BT601,
	UTP_E_GOP_COLOR_RGB_BT709,
	UTP_E_GOP_COLOR_RGB_BT2020,
	UTP_E_GOP_COLOR_SRGB,
	UTP_E_GOP_COLOR_RGB_DCIP3_D65,
	UTP_E_GOP_COLOR_SPACE_MAX,
} UTP_EN_GOP_CS;

typedef enum {
	UTP_E_GOP_COLOR_FORMAT_RGB,
	UTP_E_GOP_COLOR_FORMAT_YUV422,
	UTP_E_GOP_COLOR_FORMAT_YUV444,
	UTP_E_GOP_COLOR_FORMAT_YUV420,
	UTP_E_GOP_COLOR_FORMAT_MAX,
} UTP_EN_GOP_DATA_FMT;

typedef enum {
	UTP_E_GOP_COLOR_YCBCR_LIMITED_RANGE,
	UTP_E_GOP_COLOR_YCBCR_FULL_RANGE,
	UTP_E_GOP_COLOR_RGB_LIMITED_RANGE,
	UTP_E_GOP_COLOR_RGB_FULL_RANGE,
	UTP_E_GOP_COLOR_COLOR_RANGE_MAX,
} UTP_EN_GOP_CR;

typedef enum {
	UTP_E_GOP_HSTRCH_6TAPE_LINEAR,
	UTP_E_GOP_HSTRCH_DUPLICATE,
	UTP_E_GOP_HSTRCH_NEW4TAP,
	UTP_E_GOP_HSTRCH_INVALID
} UTP_EN_GOP_STRETCH_HMODE;

typedef enum {
	UTP_E_GOP_VSTRCH_2TAP,
	UTP_E_GOP_VSTRCH_DUPLICATE,
	UTP_E_GOP_VSTRCH_BILINEAR,
	UTP_E_GOP_VSTRCH_4TAP,
	UTP_E_GOP_VSTRCH_INVALID
} UTP_EN_GOP_STRETCH_VMODE;

typedef enum {
	UTP_E_GOP_IGNORE_NONE = 0,
	UTP_E_GOP_IGNORE_NORMAL,
	UTP_E_GOP_IGNORE_ALL,
} UTP_EN_GOP_IGNORE_TYPE;

typedef struct _UTP_GOP_INIT_INFO {
	__u8 gop_idx;
	__u32 panel_width;
	__u32 panel_height;
	__u32 panel_hstart;
	__u8 ignore_level;
} UTP_GOP_INIT_INFO;

typedef enum {
	UTP_E_GOP_CAP_WORD_UNIT,
	UTP_E_GOP_CAP_GWIN_NUM,
	UTP_E_GOP_CAP_RESERVED,
	UTP_E_GOP_CAP_CONSALPHA_VALIDBITS,
	UTP_E_GOP_CAP_PALETTE,
	UTP_E_GOP_CAP_DWIN,
	UTP_E_GOP_CAP_GOP_MUX,
	UTP_E_GOP_CAP_PIXELMODE_SUPPORT,
	UTP_E_GOP_CAP_STRETCH,
	UTP_E_GOP_CAP_TLBMODE_SUPPORT,
	UTP_E_GOP_CAP_AFBC_SUPPORT,
	UTP_E_GOP_CAP_BNKFORCEWRITE,
	UTP_E_GOP_CAP_SUPPORT_H4TAP_256PHASE,
	UTP_E_GOP_CAP_SUPPORT_V4TAP_256PHASE,
	UTP_E_GOP_CAP_SUPPORT_MIXER_SDR2HDR,
	UTP_E_GOP_CAP_SUPPORT_H8V4TAP,
	UTP_E_GOP_CAP_PALETTE_SHARESRAM_INFO,
	UTP_E_GOP_CAP_BRIGHTNESS_INFO,
	UTP_E_GOP_CAP_SUPPORT_V_SCALING_BILINEAR,
	UTP_E_GOP_CAP_SUPPORT_CSC,
} UTP_EN_GOP_CAPS;

typedef enum {
	UTP_E_GOP_POWER_SUSPEND,
	UTP_E_GOP_POWER_RESUME,
} UTP_EN_GOP_POWER_STATE;

typedef enum {
	UTP_E_GOP_OVERLAY,
	UTP_E_GOP_PRIMARY,
	UTP_E_GOP_CURSOR,
} UTP_EN_GOP_TYPE;

typedef struct _UTP_GOP_SET_ENABLE {
	__u8 gop_idx;
	__u8 bEn;
} UTP_GOP_SET_ENABLE;

typedef struct _UTP_GOP_SET_BLENDING {
	__u8 gop_idx;
	__u8 bEn;
	__u8 u8Coef;
} UTP_GOP_SET_BLENDING;

typedef struct _UTP_GOP_CSC_PARAM {
	__u16 u16Hue;
	__u16 u16Saturation;
	__u16 u16Contrast;
	__u16 u16Brightness;
	__u16 u16RGBGain[3];
	UTP_EN_GOP_CS eInputCS;
	UTP_EN_GOP_DATA_FMT eInputCFMT;
	UTP_EN_GOP_CR eInputCR;
	UTP_EN_GOP_CS eOutputCS;
	UTP_EN_GOP_DATA_FMT eOutputCFMT;
	UTP_EN_GOP_CR eOutputCR;
} UTP_GOP_CSC_PARAM;

typedef struct _UTP_GOP_PNL_INFO {
	__u16 u16Display_primaries_x[3];
	__u16 u16Display_primaries_y[3];
	__u16 u16White_point_x;
	__u16 u16White_point_y;
} UTP_GOP_PNL_INFO;

typedef struct _UTP_GOP_SET_WINDOW {
	__u8 gop_idx;
	__u8 win_enable;
	__u16 stretchWinX;
	__u16 stretchWinY;
	__u16 stretchWinDstW;
	__u16 stretchWinDstH;
	__u16 pic_x_start;
	__u16 pic_y_start;
	__u16 pic_x_end;
	__u16 pic_y_end;
	__u8 hmirror;
	__u8 vmirror;
	UTP_EN_GOP_STRETCH_HMODE hstretch_mode;
	UTP_EN_GOP_STRETCH_VMODE vstretch_mode;
	__u16 fb_width;
	__u16 fb_height;
	__u16 fb_pitch;
	__u64 fb_addr;
	__u8 wait_vsync;
	__u8 cropEn;
	__u16 cropWinXStart;
	__u16 cropWinXEnd;
	__u16 cropWinYStart;
	__u16 cropWinYEnd;
	UTP_EN_GOP_COLOR_TYPE fmt_type;
	__u8 u8zpos;
	__u8 afbc_buffer;
	UTP_GOP_CSC_PARAM *pGOPCSCParam;
	__u32 GopChangeStatus;
	UTP_EN_GOP_TYPE eGopType;
} UTP_GOP_SET_WINDOW;

typedef struct _UTP_GOP_QUERY_FEATURE {
	__u8 gop_idx;
	UTP_EN_GOP_CAPS feature;
	__u64 value;
} UTP_GOP_QUERY_FEATURE;

int utp_gop_InitByGop(UTP_GOP_INIT_INFO *utp_init_info);
int utp_gop_set_enable(UTP_GOP_SET_ENABLE *en_info);
int utp_gop_set_blending(UTP_GOP_SET_BLENDING *p_blending);
int utp_gop_SetWindow(UTP_GOP_SET_WINDOW *p_win);
int utp_gop_intr_ctrl(UTP_GOP_SET_ENABLE *pSetEnable);
int utp_gop_is_enable(UTP_GOP_SET_ENABLE *en_info);
int utp_gop_query_chip_prop(UTP_GOP_QUERY_FEATURE *pFeatureEnum);
int utp_gop_power_state(UTP_EN_GOP_POWER_STATE enPowerState);
int utp_gop_SetCscTuning(__u8 u8GOPNum, UTP_GOP_CSC_PARAM  *pCSCParamWrp);

#endif				// _MTK_TV_DRM_UTP_WRAPPER_
