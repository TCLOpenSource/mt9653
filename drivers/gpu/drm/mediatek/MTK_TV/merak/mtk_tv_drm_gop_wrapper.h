/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_UTP_WRAPPER_
#define _MTK_TV_DRM_UTP_WRAPPER_

#define GOP_FB_INVALID	(0xFF)
#define GOP_CONSTANT_ALPHA_DEF	(0xFF)
#define MAX_GOP_PLANE_NUM (8)

#define GOP_HMIRROR_STATE_CHANGE (0)
#define GOP_VMIRROR_STATE_CHANGE (1)
#define GOP_HSTRETCH_STATE_CHANGE (2)
#define GOP_VSTRETCH_STATE_CHANGE (3)
#define GOP_ALPHA_MODE_STATE_CHANGE (4)
#define GOP_AID_TYPE_STATE_CHANGE (5)

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
	/// Color format A2RGB10
	UTP_E_GOP_COLOR_A2RGB10,
	/// Color format A2BGR10
	UTP_E_GOP_COLOR_A2BGR10,
	/// Color format RGB10A2
	UTP_E_GOP_COLOR_RGB10A2,
	/// Color format BGR10A2
	UTP_E_GOP_COLOR_BGR10A2,
	/// Color format XRGB2101010
	UTP_E_GOP_COLOR_X2RGB10,
	/// Color format XBGR2101010
	UTP_E_GOP_COLOR_X2BGR10,
	/// Color format RGBX1010102
	UTP_E_GOP_COLOR_RGB10X2,
	/// Color format BGRX1010102
	UTP_E_GOP_COLOR_BGR10X2,
	// Color format RGB888.
	UTP_E_GOP_COLOR_RGB888,
	/// Invalid color format.
	UTP_E_GOP_COLOR_INVALID
} UTP_EN_GOP_COLOR_TYPE;

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
	E_GOP_WARP_DST_OP,
	E_GOP_WARP_DST_VG_SEPARATE,
	E_GOP_WARP_DST_INVALID,
} EN_GOP_WRAP_DST_PATH;

typedef struct _UTP_GOP_INIT_INFO {
	__u8 gop_idx;
	__u32 panel_width;
	__u32 panel_height;
	__u32 panel_hstart;
	__u8 ignore_level;
	EN_GOP_WRAP_DST_PATH eGopDstPath;
} UTP_GOP_INIT_INFO;

typedef enum {
	UTP_E_GOP_CAP_AFBC_SUPPORT,
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

typedef enum {
	E_GOP_WARP_PATTERN_MOVE_TO_RIGHT,
	E_GOP_WARP_PATTERN_MOVE_TO_LEFT,
} EN_GOP_WRAP_TESTPATTERN_MOVE_DIR;

typedef enum {
	E_GOP_WARP_OUT_A8RGB10,
	E_GOP_WARP_OUT_ARGB8888_DITHER,
	E_GOP_WARP_OUT_ARGB8888_ROUND,
} EN_GOP_WRAP_OUTPUT_MODE;

typedef enum {
	E_GOP_WRAP_IGNORE_NONE = 0,
	E_GOP_WRAP_IGNORE_NORMAL,
	E_GOP_WRAP_IGNORE_SWRST,
	E_GOP_WRAP_IGNORE_ALL,
} EN_GOP_WRAP_IGNORE_TYPE;

typedef enum {
	E_GOP_WRAP_VIDEO_OSDB0_OSDB1,
	E_GOP_WRAP_OSDB0_VIDEO_OSDB1,
	E_GOP_WRAP_OSDB0_OSDB1_VIDEO,
} EN_GOP_WRAP_VIDEO_ORDER_TYPE;

typedef struct _UTP_GOP_SET_ENABLE {
	__u8 gop_idx;
	__u8 bEn;
	__u8 bIsCursor;
} UTP_GOP_SET_ENABLE;

typedef struct _UTP_GOP_SET_BLENDING {
	__u8 bEnPixelAlpha;
	__u8 u8Coef;
} UTP_GOP_SET_BLENDING;

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
	UTP_EN_GOP_COLOR_TYPE fmt_type;
	__u8 u8zpos;
	__u8 afbc_buffer;
	__u32 GopChangeStatus;
	UTP_EN_GOP_TYPE eGopType;
	__u8 bypass_path;
	__u8 gop_ml_res_idx;
	UTP_GOP_SET_BLENDING stGopAlphaMode;
	__u64 gop_aid_type;
} UTP_GOP_SET_WINDOW;

typedef struct _UTP_GOP_QUERY_FEATURE {
	__u8 gop_idx;
	UTP_EN_GOP_CAPS feature;
	__u64 value;
} UTP_GOP_QUERY_FEATURE;

typedef struct _MTK_GOP_TESTPATTERN {
	__u8  PatEnable;
	__u8  HwAutoMode;
	__u32 DisWidth;
	__u32 DisHeight;
	__u32 ColorbarW;
	__u32 ColorbarH;
	__u8  EnColorbarMove;
	EN_GOP_WRAP_TESTPATTERN_MOVE_DIR MoveDir;
	__u16 MoveSpeed;
} MTK_GOP_TESTPATTERN;

typedef struct _MTK_GOP_TGEN_INFO {
	__u16  Tgen_hs_st;
	__u16  Tgen_hs_end;
	__u16  Tgen_hfde_st;
	__u16  Tgen_hfde_end;
	__u16  Tgen_htt;
	__u16  Tgen_vs_st;
	__u16  Tgen_vs_end;
	__u16  Tgen_vfde_st;
	__u16  Tgen_vfde_end;
	__u16  Tgen_vtt;
	EN_GOP_WRAP_OUTPUT_MODE GOPOutMode;
} MTK_GOP_TGEN_INFO;


typedef struct _MTK_GOP_ML_INFO {
	int fd;
} MTK_GOP_ML_INFO;

typedef struct _MTK_GOP_ADL_INFO {
	int fd;
	__u32 adl_bufsize;
	__u64 __user *u64_adl_buf;
} MTK_GOP_ADL_INFO;

typedef struct _MTK_GOP_PQ_CFD_ML_INFO {
		__u32 u32_cfd_ml_bufsize;
		__u64 __user *u64_cfd_ml_buf;
		__u32 u32_pq_ml_bufsize;
		__u64 __user *u64_pq_ml_buf;
} MTK_GOP_PQ_CFD_ML_INFO;

typedef struct _MTK_GOP_PQ_INFO {
	void *pCtx;
	void *pSwReg;
	void *pMetadata;
	void *pHwReport;
} MTK_GOP_PQ_INFO;

typedef struct _MTK_GOP_GETROI {
	__u8 bRetTotal;
	__u16 gopIdx;
	__u32 Threshold;
	__u32 MainVRate;
	__u64 RetCount;
} MTK_GOP_GETROI;

int utp_gop_InitByGop(UTP_GOP_INIT_INFO *utp_init_info, MTK_GOP_ML_INFO *pGopMlInfo);
int utp_gop_set_enable(UTP_GOP_SET_ENABLE *en_info, __u8 gop_ml_resIdx);
int utp_gop_SetWindow(UTP_GOP_SET_WINDOW *p_win);
int utp_gop_intr_ctrl(UTP_GOP_SET_ENABLE *pSetEnable);
int utp_gop_is_enable(UTP_GOP_SET_ENABLE *en_info);
int utp_gop_query_chip_prop(UTP_GOP_QUERY_FEATURE *pFeatureEnum);
int utp_gop_power_state(__u32 State);
int gop_wrapper_set_testpattern(MTK_GOP_TESTPATTERN *pGopTestPatternInfo);
int gop_wrapper_get_CRC(__u32 *CRCvalue);
int gop_wrapper_print_status(__u8 GOPNum);
int gop_wrapper_set_Tgen(MTK_GOP_TGEN_INFO *pGOPTgenInfo);
int gop_wrapper_set_TgenFreeRun(__u8 Tgen_VGSyncEn, __u8 u8MlResIdx);
int gop_wrapper_set_MixerOutMode(bool Mixer1_OutPreAlpha, bool Mixer2_OutPreAlpha);
int gop_wrapper_set_BypassMode(__u8 u8GOP, bool bEnable);
int gop_wrapper_set_PQCtxInfo(__u8 u8GOP, MTK_GOP_PQ_INFO *pGopPQCtx);
int gop_wrapper_set_Adlnfo(__u8 u8GOP, MTK_GOP_ADL_INFO *pGopAdlInfo);
int gop_wrapper_set_PqCfdMlInfo(__u8 u8GOP, MTK_GOP_PQ_CFD_ML_INFO *pGopPqCfdMl);
int gop_wrapper_ml_fire(__u8 u8MlResIdx);
int gop_wrapper_ml_get_mem_info(__u8 u8MlResIdx);
int gop_wrapper_setLayer(__u32 GopIdx, __u32 MlResIdx);
int gop_wrapper_get_ROI(MTK_GOP_GETROI *pRoiInfo);
int gop_wrapper_set_vg_order(EN_GOP_WRAP_VIDEO_ORDER_TYPE eVGOrder, __u8 u8MlResIdx);

#endif				// _MTK_TV_DRM_UTP_WRAPPER_
