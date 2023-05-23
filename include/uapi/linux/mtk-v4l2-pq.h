/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __UAPI_MTK_V4L2_PQ_H__
#define __UAPI_MTK_V4L2_PQ_H__

#include <linux/types.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#endif
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
//---------------------------------------------------------------------------
//----------------------
//  Macro and Define
//---------------------------------------------------------------------------
//----------------------
//COMMON

//pq-enhance:HDR

//pq-enhance:PQD
#define V4L2_HISTOGRAM_SATOFHUE_WIN_NUM (6)
#define V4L2_HISTOGRAM_BIN_NUM_MAX      (128)
#define V4L2_HSY_CFD_MAX_HUE_INDEX      (14)     //maximum hue index
#define V4L2_PQ_BIN_VERSION_LEN         (16)
#define V4L2_PQ_FILE_PATH_LENGTH        (255)
#define V4L2_NR_CONTROL_STEP_NUM        (8)
#define V4L2_DLC_HISTOGRAM_LIMIT_CURVE_ARRARY_NUM  (17)
#define V4L2_NUM_PQ_GAMMA_LUT_ENTRY     (256)
#define V4L2_PQ_GAMMA_CHANNEL           (3)
#define V4L2_ST_XC_PQ_GAMMA_IN_VERSION  (3)

//DISPLAY
#define V4L2_MTK_HISTOGRAM_INDEX		(32)
#define V4L2_MTK_MAX_PATH_IPNUM			(5)

// Dolby Vision config
#define V4L2_DV_MAX_MODE_NUM	        (10)
#define V4L2_DV_MAX_MODE_LENGTH         (32)

//B2R
/* MTK 8-bit compressed block mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21CS       v4l2_fourcc('M', '2', 'C', 'S')
/* MTK 8-bit block mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21S        v4l2_fourcc('M', '2', '1', 'S')
/* MTK 10-bit tile block mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21S10T     v4l2_fourcc('M', 'T', 'S', 'T')
/* MTK 10-bit raster block mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21S10R     v4l2_fourcc('M', 'T', 'S', 'R')
/* MTK 10-bit tile compressed block mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21CS10T    v4l2_fourcc('M', 'C', 'S', 'T')
/* MTK 10-bit raster compressed block mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21CS10R    v4l2_fourcc('M', 'C', 'S', 'R')
/* MTK 8-bit compressed block au offset mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21CSA      v4l2_fourcc('M', 'A', 'C', 'S')
/* MTK 10-bit tile block jump mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21S10TJ    v4l2_fourcc('M', 'J', 'S', 'T')
/* MTK 10-bit raster block jump mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21S10RJ    v4l2_fourcc('M', 'J', 'S', 'R')
/* MTK 10-bit tile compressed block jump mode, two contiguous planes */
#define V4L2_PIX_FMT_MT21CS10TJ   v4l2_fourcc('J', 'C', 'S', 'T')
/* MTK 10-bit raster compressed block jump mode, two cont. planes */
#define V4L2_PIX_FMT_MT21CS10RJ   v4l2_fourcc('J', 'C', 'S', 'R')

//XC

/************************************************
 *******************CTRL CIDs*********************
 ************************************************
 */

//COMMON
#define V4L2_CID_INPUT_EXT_INFO                 (V4L2_CID_LASTP1+0x1)
//fixme remove
#define V4L2_CID_OUTPUT_EXT_INFO                (V4L2_CID_LASTP1+0x2)
//fixme remove
#define V4L2_CID_FORCE_P_MODE                   (V4L2_CID_LASTP1+0x3)
#define V4L2_CID_DELAY_TIME                     (V4L2_CID_LASTP1+0x4)
#define V4L2_CID_INPUT_TIMING                   (V4L2_CID_LASTP1+0x5)
#define V4L2_CID_GEN_PATTERN                    (V4L2_CID_LASTP1+0x6)
//pq-enhance:HDR base:0x100
#define V4L2_CID_HDR_BASE                       (V4L2_CID_LASTP1+0x100)
#define V4L2_CID_DOLBY_3DLUT                    (V4L2_CID_LASTP1+0x101)
#define V4L2_CID_DOLBY_HDR_MODE                 (V4L2_CID_LASTP1+0x102)
#define V4L2_CID_CUS_COLOR_RANGE                (V4L2_CID_LASTP1+0x103)
#define V4L2_CID_HDR_ENABLE_MODE                (V4L2_CID_LASTP1+0x104)
#define V4L2_CID_TMO_MODE                       (V4L2_CID_LASTP1+0x105)
#define V4L2_CID_CFD_CUSTOMER_SETTING           (V4L2_CID_LASTP1+0x106)
#define V4L2_CID_HDR_ATTR                       (V4L2_CID_LASTP1+0x107)
#define V4L2_CID_HDR_SEAMLESS_STATUS            (V4L2_CID_LASTP1+0x108)
#define V4L2_CID_HDR_TYPE                       (V4L2_CID_LASTP1+0x109)
#define V4L2_CID_HDR_CUS_COLORI_METRY           (V4L2_CID_LASTP1+0x10a)
#define V4L2_CID_HDR_DOLBY_GD_INFO              (V4L2_CID_LASTP1+0x10b)
#define V4L2_CID_DOLBY_BIN_DATA                 (V4L2_CID_LASTP1+0x10c)
#define V4L2_CID_DOLBY_BIN_BUFFER               (V4L2_CID_LASTP1+0x10d)
#define V4L2_CID_DOLBY_LS_DATA                  (V4L2_CID_LASTP1+0x10e)

//pq-enhance:PQD base:0x200
#define V4L2_CID_PQ_BASE                        (V4L2_CID_LASTP1+0x200)
#define V4L2_CID_PQ_DBGLEVEL                    (V4L2_CID_LASTP1+0x201)
#define V4L2_CID_PQ_INIT                        (V4L2_CID_LASTP1+0x202)
#define V4L2_CID_PQ_SETTING                     (V4L2_CID_LASTP1+0x203)
#define V4L2_CID_MADI_MOTION                    (V4L2_CID_LASTP1+0x204)
#define V4L2_CID_RFBL_MODE                      (V4L2_CID_LASTP1+0x205)
#define V4L2_CID_DOT_BY_DOT                     (V4L2_CID_LASTP1+0x206)
#define V4L2_CID_PQ_TABLE_STATUS                (V4L2_CID_LASTP1+0x207)
#define V4L2_CID_UCD_MODE                       (V4L2_CID_LASTP1+0x208)
#define V4L2_CID_HSB_BYPASS                     (V4L2_CID_LASTP1+0x209)
#define V4L2_CID_PICTUREMODE                    (V4L2_CID_LASTP1+0x20a)
#define V4L2_CID_LOAD_PTP                       (V4L2_CID_LASTP1+0x20b)
#define V4L2_CID_NR_SETTINGS                    (V4L2_CID_LASTP1+0x20c)
#define V4L2_CID_DPU_MODULE_SETTINGS            (V4L2_CID_LASTP1+0x20d)
#define V4L2_CID_CCM                            (V4L2_CID_LASTP1+0x20e)
#define V4L2_CID_COLOR_TUNER                    (V4L2_CID_LASTP1+0x20f)
#define V4L2_CID_HSY                            (V4L2_CID_LASTP1+0x210)
#define V4L2_CID_MANUAL_LUMA_CURVE              (V4L2_CID_LASTP1+0x211)
#define V4L2_CID_BLACK_WHITE_STRETCH            (V4L2_CID_LASTP1+0x212)
#define V4L2_CID_PQ_CMD                         (V4L2_CID_LASTP1+0x213)
#define V4L2_CID_COLORTEMP                      (V4L2_CID_LASTP1+0x214)
#define V4L2_CID_3D_CLONE_PQMAP                 (V4L2_CID_LASTP1+0x215)
#define V4L2_CID_SWDR_INFO                      (V4L2_CID_LASTP1+0x216)
#define V4L2_CID_DUAL_VIEW_STATUS               (V4L2_CID_LASTP1+0x217)
#define V4L2_CID_RGB_PATTERN                    (V4L2_CID_LASTP1+0x218)
#define V4L2_CID_BLUE_STRETCH                   (V4L2_CID_LASTP1+0x219)
#define V4L2_CID_H_NONLINEARSCALING             (V4L2_CID_LASTP1+0x21a)
#define V4L2_CID_MCDI_MODE                      (V4L2_CID_LASTP1+0x21b)
#define V4L2_CID_DEALYTIME                      (V4L2_CID_LASTP1+0x21c)
#define V4L2_CID_PQ_CAPS                        (V4L2_CID_LASTP1+0x21d)
#define V4L2_CID_PQ_VERSION                     (V4L2_CID_LASTP1+0x21e)
#define V4L2_CID_ZERO_FRAME_MODE                (V4L2_CID_LASTP1+0x21f)
#define V4L2_CID_EVALUATE_FBL                   (V4L2_CID_LASTP1+0x220)
#define V4L2_CID_HISTOGRAM                      (V4L2_CID_LASTP1+0x221)
#define V4L2_CID_HSY_RANGE                      (V4L2_CID_LASTP1+0x222)
#define V4L2_CID_LUMA_CURVE                     (V4L2_CID_LASTP1+0x223)
#define V4L2_CID_CHROMA                         (V4L2_CID_LASTP1+0x224)
#define V4L2_CID_3D_CLONE_PIP                   (V4L2_CID_LASTP1+0x225)
#define V4L2_CID_3D_CLONE_LBL                   (V4L2_CID_LASTP1+0x226)
#define V4L2_CID_GAME_MODE                      (V4L2_CID_LASTP1+0x227)
#define V4L2_CID_BOB_MODE                       (V4L2_CID_LASTP1+0x228)
#define V4L2_CID_BYPASS_MODE                    (V4L2_CID_LASTP1+0x229)
#define V4L2_CID_DMEO_CLONE                     (V4L2_CID_LASTP1+0x22a)
#define V4L2_CID_REDUCE_BW                      (V4L2_CID_LASTP1+0x22b)
#define V4L2_CID_HDR_LEVEL                      (V4L2_CID_LASTP1+0x22c)
#define V4L2_CID_MWE                            (V4L2_CID_LASTP1+0x22d)
#define V4L2_CID_PQ_EXIT                        (V4L2_CID_LASTP1+0x22e)
#define V4L2_CID_RB_CHANNEL                     (V4L2_CID_LASTP1+0x22f)
#define V4L2_CID_FILM_MODE                      (V4L2_CID_LASTP1+0x230)
#define V4L2_CID_NR_MODE                        (V4L2_CID_LASTP1+0x231)
#define V4L2_CID_MEPG_NR_MODE                   (V4L2_CID_LASTP1+0x232)
#define V4L2_CID_XVYCC_MODE                     (V4L2_CID_LASTP1+0x233)
#define V4L2_CID_DLC_MODE                       (V4L2_CID_LASTP1+0x234)
#define V4L2_CID_SWDR_MODE                      (V4L2_CID_LASTP1+0x235)
#define V4L2_CID_DYNAMIC_CONTRAST_ENABLE        (V4L2_CID_LASTP1+0x236)
#define V4L2_CID_PQ_GAMMA                       (V4L2_CID_LASTP1+0x237)
#define V4L2_CID_DLC_INIT                       (V4L2_CID_LASTP1+0x238)
#define V4L2_CID_DLC_HISTO_RANGE                (V4L2_CID_LASTP1+0x239)
#define V4L2_CID_PQMAP_INFO                     (V4L2_CID_LASTP1+0x23a)
#define V4L2_CID_PQ_METADATA                    (V4L2_CID_LASTP1+0x23b)
#define V4L2_CID_PQPARAM                        (V4L2_CID_LASTP1+0x23c)
#define V4L2_CID_SET_PQ_MEMORY_INDEX            (V4L2_CID_LASTP1+0x23d)

//3D base:0x300
#define V4L2_CID_3D_BASE                        (V4L2_CID_LASTP1+0x300)
#define V4L2_CID_3D_HW_VERSION                  (V4L2_CID_LASTP1+0x301)
#define V4L2_CID_3D_LR_FRAME_EXCHG              (V4L2_CID_LASTP1+0x302)
#define V4L2_CID_3D_FORMAT                      (V4L2_CID_LASTP1+0x303)
#define V4L2_CID_3D_HSHIFT                      (V4L2_CID_LASTP1+0x304)
#define V4L2_CID_3D_MODE                        (V4L2_CID_LASTP1+0x305)

//DISPLAY base:0x400
#define V4L2_CID_DISPLAY_BASE                   (V4L2_CID_LASTP1+0x400)
#define V4L2_CID_SET_DISPLAY_DATA               (V4L2_CID_LASTP1+0x401)
#define V4L2_CID_GET_DISPLAY_DATA               (V4L2_CID_LASTP1+0x402)
#define V4L2_CID_GET_QUEUE_INFO                 (V4L2_CID_LASTP1+0x404)
#define V4L2_CID_SET_STREAMMETA_DATA            (V4L2_CID_LASTP1+0x405)
#define V4L2_CID_SET_INPUT_MODE			(V4L2_CID_LASTP1+0x406)
#define V4L2_CID_SET_OUTPUT_MODE		(V4L2_CID_LASTP1+0x407)
#define V4L2_CID_GET_RW_DIFF			(V4L2_CID_LASTP1+0x408)
#define V4L2_CID_SET_FLOW_CONTROL		(V4L2_CID_LASTP1+0x409)
#define V4L2_CID_SET_AISR_ACTIVE_WIN		(V4L2_CID_LASTP1+0x40a)
#define V4L2_CID_STREAM_DEBUG               (V4L2_CID_LASTP1+0x40b)
#define V4L2_CID_SET_STAGE_PATTERN		(V4L2_CID_LASTP1+0x40c)

//B2R base:0x500
#define V4L2_CID_B2R_BASE                        (V4L2_CID_LASTP1+0x500)
#define V4L2_CID_B2R_TIMING                      (V4L2_CID_LASTP1+0x501)
#define V4L2_CID_B2R_GLOBAL_TIMING               (V4L2_CID_LASTP1+0x502)
#define V4L2_CID_B2R_GET_CAP                     (V4L2_CID_LASTP1+0x503)
//XC base:0x600
#define V4L2_CID_PQ_BUFFER_REQ                   (V4L2_CID_LASTP1+0x612)
#define V4L2_CID_PQ_SECURE_MODE                  (V4L2_CID_LASTP1+0x613)
#define V4L2_CID_PQ_TRY_QUEUE			(V4L2_CID_LASTP1+0x614)
#define V4L2_CID_PQ_S_BUFFER_INFO		(V4L2_CID_LASTP1+0x615)
#define V4L2_CID_PQ_G_BUFFER_OUT_INFO		(V4L2_CID_LASTP1+0x616)
#define V4L2_CID_PQ_G_BUFFER_CAP_INFO		(V4L2_CID_LASTP1+0x617)

/************************************************
 *********************EVENTs**********************
 ************************************************
 */

//COMMON

//pq-enhance:HDR
#define V4L2_EVENT_HDR_SEAMLESS_MUTE    (V4L2_EVENT_PRIVATE_START | 0x1)
#define V4L2_EVENT_HDR_SEAMLESS_UNMUTE  (V4L2_EVENT_PRIVATE_START | 0x2)
//pq-enhance:PQD

//DISPLAY
#define V4L2_EVENT_MTK_PQ_INPUT_DONE    (V4L2_EVENT_PRIVATE_START | 0x10)
#define V4L2_EVENT_MTK_PQ_OUTPUT_DONE   (V4L2_EVENT_PRIVATE_START | 0x11)
#define V4L2_EVENT_MTK_PQ_CALLBACK      (V4L2_EVENT_PRIVATE_START | 0x12)
#define V4L2_EVENT_MTK_PQ_STREAMOFF     (V4L2_EVENT_PRIVATE_START | 0x13)
#define V4L2_EVENT_MTK_PQ_INPUT_STREAMOFF     (V4L2_EVENT_PRIVATE_START | 0x14)
#define V4L2_EVENT_MTK_PQ_OUTPUT_STREAMOFF    (V4L2_EVENT_PRIVATE_START | 0x15)

//B2R

//XC

/*
 * note: following define is used to identify T32 or M6 flow.
 * Should be remove in future.
 *
 * If need to execute M6 flow, need to add following settings:
 * VIDIOC_QBUF:		v4l2_buffer.flags |= V4L2_BUF_FLAG_TEMP_M6
 *
 * If need to execute T32 flow, need to add following settings:
 * VIDIOC_QBUF:		v4l2_buffer.flags |= V4L2_BUF_FLAG_TEMP_T32
 */
#define V4L2_BUF_FLAG_TEMP_M6		(0x08000000)

/* ========================================================================== */
/* -------------------------- extend color format --------------------------- */
/* ========================================================================== */

/* ============================================================================================== */
/*   RGB fourcc character definition                                                              */
/*                                                                                                */
/*   x y _ _  , xy = AR(ARGB)   AB(ABGR)    RA(RGBA)    BA(BGRA)    RG(RGB)    BG(BGR)            */
/*                   ar(AGB CE) ab(ABGR CE) ra(RGBA CE) ba(BGRA CE) rg(RGB CE) bg(BGR CE)         */
/*                   AR(ARGB)   AB(ABGR)    RA(RGBA)    BA(BGRA)                                  */
/*                   ar(AGB CE) ab(ABGR CE) ra(RGBA CE) ba(BGRA CE)                           */
/*                                                                                                */
/*   _ _ z _  , z = 8(888/8888) A(101010/2101010) C(121212)  4(444/444)                 */
/*                                                                                                */
/*   _ _ _ w  , w = P(planar) ' '(none)  C(6bit-compressed) D(8bit-compressed)           */
/* ============================================================================================== */

/* 3 plane contiguous RGB */
#define V4L2_PIX_FMT_RGB_8_8_8_CE		v4l2_fourcc('r', 'g', '8', 'P')

/* 1 plane RGB */
#define V4L2_PIX_FMT_RGB888			v4l2_fourcc('R', 'G', '8', ' ')
#define V4L2_PIX_FMT_RGB101010			v4l2_fourcc('R', 'G', 'A', ' ')
#define V4L2_PIX_FMT_RGB121212			v4l2_fourcc('R', 'G', 'C', ' ')

#define V4L2_PIX_FMT_ARGB8888			v4l2_fourcc('A', 'R', '8', ' ')
#define V4L2_PIX_FMT_ABGR8888			v4l2_fourcc('A', 'B', '8', ' ')
#define V4L2_PIX_FMT_RGBA8888			v4l2_fourcc('R', 'A', '8', ' ')
#define V4L2_PIX_FMT_BGRA8888			v4l2_fourcc('B', 'A', '8', ' ')

#define V4L2_PIX_FMT_ARGB2101010		v4l2_fourcc('A', 'R', 'A', ' ')
#define V4L2_PIX_FMT_ABGR2101010		v4l2_fourcc('A', 'B', 'A', ' ')
#define V4L2_PIX_FMT_RGBA1010102		v4l2_fourcc('R', 'A', 'A', ' ')
#define V4L2_PIX_FMT_BGRA1010102		v4l2_fourcc('B', 'A', 'A', ' ')

/*MEMC*/
#define V4L2_PIX_FMT_RGB444_6B			v4l2_fourcc('R', 'G', '4', 'C')
#define V4L2_PIX_FMT_RGB444_8B			v4l2_fourcc('R', 'G', '4', 'D')


/* ============================================================================================== */
/*   YUV fourcc character definition                                                              */
/*                                                                                                */
/*   a _ _ _  , a = Y(YUV) U(UVY) V(VYU) y(YVU) u(UYV) v(VUY)                                     */
/*                                                                                                */
/*   _ b _ _  , b = 4(444) 2(422) 0(420)                                                          */
/*                                                                                                */
/*   _ _ c _  , c = 6(YC6bit) 8(YC8bit) A(YC10bit) C(YC12bit) %(Y8bitC6bit)...                    */
/*                                                                                                */
/*   _ _ _ d  , d = C(compressed) P(planar) &(compressed & planar) ' '(none)  D(8bit-compressed)  */
/* ============================================================================================== */

/* 1 plane YUV */
#define V4L2_PIX_FMT_YUV444_VYU_10B		v4l2_fourcc('V', '4', 'A', ' ')
#define V4L2_PIX_FMT_YUV444_VYU_8B		v4l2_fourcc('V', '4', '8', ' ')

/* 2 plane YUV */
#define V4L2_PIX_FMT_YUV422_Y_UV_12B		v4l2_fourcc('Y', '2', 'C', 'P')
#define V4L2_PIX_FMT_YUV422_Y_UV_10B		v4l2_fourcc('Y', '2', 'A', 'P')
#define V4L2_PIX_FMT_YUV422_Y_VU_10B		v4l2_fourcc('y', '2', 'A', 'P')
#define V4L2_PIX_FMT_YUV422_Y_UV_8B_CE		v4l2_fourcc('Y', '2', '8', '&')
#define V4L2_PIX_FMT_YUV422_Y_VU_8B_CE		v4l2_fourcc('y', '2', '8', '&')
#define V4L2_PIX_FMT_YUV422_Y_UV_8B_6B_CE	v4l2_fourcc('Y', '2', '%', '&')
#define V4L2_PIX_FMT_YUV422_Y_UV_6B_CE		v4l2_fourcc('Y', '2', '6', '&')
#define V4L2_PIX_FMT_YUV422_Y_VU_6B_CE		v4l2_fourcc('y', '2', '6', '&')

#define V4L2_PIX_FMT_YUV420_Y_UV_10B		v4l2_fourcc('Y', '0', 'A', 'P')
#define V4L2_PIX_FMT_YUV420_Y_VU_10B		v4l2_fourcc('y', '0', 'A', 'P')
#define V4L2_PIX_FMT_YUV420_Y_UV_8B_CE		v4l2_fourcc('Y', '0', '8', '&')
#define V4L2_PIX_FMT_YUV420_Y_VU_8B_CE		v4l2_fourcc('y', '0', '8', '&')
#define V4L2_PIX_FMT_YUV420_Y_UV_8B_6B_CE	v4l2_fourcc('Y', '0', '%', '&')
#define V4L2_PIX_FMT_YUV420_Y_UV_6B_CE		v4l2_fourcc('Y', '0', '6', '&')
#define V4L2_PIX_FMT_YUV420_Y_VU_6B_CE		v4l2_fourcc('y', '0', '6', '&')

/* 3 plane YUV */
#define V4L2_PIX_FMT_YUV444_Y_U_V_8B_CE		v4l2_fourcc('Y', '4', '8', '&')

/*MEMC*/
#define V4L2_PIX_FMT_YUV444_YUV_6B		v4l2_fourcc('Y', '4', 'C', 'C')
#define V4L2_PIX_FMT_YUV444_YUV_8B		v4l2_fourcc('Y', '4', 'C', 'D')
#define V4L2_PIX_FMT_YUV422_YUV_6B		v4l2_fourcc('Y', '2', '8', 'C')
#define V4L2_PIX_FMT_YUV422_YUV_8B		v4l2_fourcc('Y', '2', '8', 'D')
#define V4L2_PIX_FMT_YUV420_YUV_6B		v4l2_fourcc('Y', '0', '6', 'C')
#define V4L2_PIX_FMT_YUV420_YUV_8B		v4l2_fourcc('Y', '0', '6', 'D')

/************************************************
 *********************ENUMs***********************
 ************************************************
 */

//pq-enhance:HDR
enum v4l2_dolby_view_mode {
	V4L2_DOLBY_VIEW_MODE_VIVID,
	V4L2_DOLBY_VIEW_MODE_BRIGHTNESS,
	V4L2_DOLBY_VIEW_MODE_DARK,
	V4L2_DOLBY_VIEW_MODE_USER,
};

enum v4l2_hdr_enable_mode {
	V4L2_HDR_ENABLE_MODE_AUTO,
	V4L2_HDR_ENABLE_MODE_DISABLE,
};

enum v4l2_tmo_level_type {
	V4L2_TMO_LEVEL_TYPE_LOW,
	V4L2_TMO_LEVEL_TYPE_MID,
	V4L2_TMO_LEVEL_TYPE_HIGH,
};

enum v4l2_hdr_type {
	V4L2_HDR_TYPE_NONE,
	V4L2_HDR_TYPE_DOLBY,
	V4L2_HDR_TYPE_OPEN,
	V4L2_HDR_TYPE_TCH,
	V4L2_HDR_TYPE_HLG,
	V4L2_HDR_TYPE_HDR10_PLUS,
};

enum v4l2_cus_color_range_type {
	V4L2_CUS_COLOR_RANGE_TYPE_YUV_LIMIT,       //force limit
	V4L2_CUS_COLOR_RANGE_TYPE_YUV_FULL,        //force full
	V4L2_CUS_COLOR_RANGE_TYPE_RGB_LIMIT,       //force limit
	V4L2_CUS_COLOR_RANGE_TYPE_RGB_FULL,        //force full
	V4L2_CUS_COLOR_RANGE_TYPE_AUTO,      //according to input quantization
	V4L2_CUS_COLOR_RANGE_TYPE_ADJ,
	//according to ultral black and white level
};

enum v4l2_hdr_output_tr_type {
	V4L2_HDR_OUTPUT_TR_BT1886 = 0,
	V4L2_HDR_OUTPUT_TR_PQ = 1,
	V4L2_HDR_OUTPUT_TR_LINEAR = 2,
};

enum v4l2_cus_ip_type {
	V4L2_CUS_IP_RANGE_COVERT,
	V4L2_CUS_IP_IPCSC,
	V4L2_CUS_IP_HISTOGRAM_LOCATION,
	V4L2_CUS_IP_PRE_RANGE_COVERT,
	V4L2_CUS_IP_PRE_CSC,
	V4L2_CUS_IP_EOTF,
	V4L2_CUS_IP_PRE_GAMUT,
	V4L2_CUS_IP_OOTF,
	V4L2_CUS_IP_RGB3DLUT,
	V4L2_CUS_IP_OETF,
	V4L2_CUS_IP_POST_CSC,
	V4L2_CUS_IP_TMO,
	V4L2_CUS_IP_POST_RANGE_COVERT,
	V4L2_CUS_IP_POST_CSC_SDR,
	V4L2_CUS_IP_POST_GAMUT,
	V4L2_CUS_IP_POST_DEGAMMA,
	V4L2_CUS_IP_POST_RGB3D,
	V4L2_CUS_IP_POST_GAMMA,
	V4L2_CUS_IP_PRE_RANGE_COVERT_SDR,
	V4L2_CUS_IP_POST_RANGE_COVERT_SDR,
	V4L2_CUS_IP_DLC_RANGE,
	V4L2_CUS_IP_VIP_OUT_RANGE,
	V4L2_CUS_IP_RGB3DLUT_INTERVAL,
	V4L2_CUS_IP_MAX,
};

//pq-enhance:PQD
enum v4l2_ext_dtv_type {
	V4L2_EXT_DTV_TYPE_H264,
	V4L2_EXT_DTV_TYPE_MPEG2,
	V4L2_EXT_DTV_TYPE_IFRAME,
};

enum v4l2_ext_mm_type {
	V4L2_EXT_MM_TYPE_PHOTO,
	V4L2_EXT_MM_TYPE_MOVIE,
	V4L2_EXT_MM_TYPE_AMAZON,
	V4L2_EXT_MM_TYPE_NETFLIX,
};

enum v4l2_ext_video_standard {
	V4L2_EXT_VIDEO_STANDARD_PAL_BGHI,
	V4L2_EXT_VIDEO_STANDARD_NTSC_M,
	V4L2_EXT_VIDEO_STANDARD_SECAM,
	V4L2_EXT_VIDEO_STANDARD_NTSC_44,
	V4L2_EXT_VIDEO_STANDARD_PAL_M,
	V4L2_EXT_VIDEO_STANDARD_PAL_N,
	V4L2_EXT_VIDEO_STANDARD_PAL_60,
	V4L2_EXT_VIDEO_STANDARD_NOTSTANDARD,
	V4L2_EXT_VIDEO_STANDARD_AUTO,
};

//DISPLAY
enum mtk_pq_3d_input_mode {
	MTK_PQ_3D_INPUT_FRAME_PACKING            = 0x00, //0000
	MTK_PQ_3D_INPUT_FIELD_ALTERNATIVE        = 0x01, //0001
	MTK_PQ_3D_INPUT_LINE_ALTERNATIVE         = 0x02, //0010
	MTK_PQ_3D_INPUT_SIDE_BY_SIDE_FULL        = 0x03, //0011
	MTK_PQ_3D_INPUT_L_DEPTH                  = 0x04, //0100
	MTK_PQ_3D_INPUT_L_DEPTH_GRAPHICS_DEPTH   = 0x05, //0101
	MTK_PQ_3D_INPUT_TOP_BOTTOM               = 0x06, //0110
	MTK_PQ_3D_INPUT_SIDE_BY_SIDE_HALF        = 0x08, //1000
	MTK_PQ_3D_INPUT_CHECK_BOARD              = 0x09, //1001
	MTK_PQ_3D_INPUT_MODE_USER                = 0x10,
	MTK_PQ_3D_INPUT_MODE_NONE                = MTK_PQ_3D_INPUT_MODE_USER,
	MTK_PQ_3D_INPUT_FRAME_ALTERNATIVE,
	MTK_PQ_3D_INPUT_SIDE_BY_SIDE_HALF_INTERLACE,
	MTK_PQ_3D_INPUT_FRAME_PACKING_OPT,
	MTK_PQ_3D_INPUT_TOP_BOTTOM_OPT,
	MTK_PQ_3D_INPUT_NORMAL_2D,
	MTK_PQ_3D_INPUT_NORMAL_2D_INTERLACE,
	MTK_PQ_3D_INPUT_NORMAL_2D_INTERLACE_PTP,
	MTK_PQ_3D_INPUT_SIDE_BY_SIDE_HALF_INTERLACE_OPT,
	MTK_PQ_3D_INPUT_NORMAL_2D_HW,                //for hw 2D to 3D use
	MTK_PQ_3D_INPUT_PIXEL_ALTERNATIVE,
	MTK_PQ_3D_INPUT_MAX,
};

enum mtk_pq_3d_output_mode {
	MTK_PQ_3D_OUTPUT_MODE_NONE,
	MTK_PQ_3D_OUTPUT_LINE_ALTERNATIVE,
	MTK_PQ_3D_OUTPUT_TOP_BOTTOM,
	MTK_PQ_3D_OUTPUT_SIDE_BY_SIDE_HALF,
	MTK_PQ_3D_OUTPUT_FRAME_ALTERNATIVE,
	MTK_PQ_3D_OUTPUT_FRAME_L,
	MTK_PQ_3D_OUTPUT_FRAME_R,
	MTK_PQ_3D_OUTPUT_FRAME_ALTERNATIVE_NOFRC,
	MTK_PQ_3D_OUTPUT_CHECKBOARD_HW,                  //for hw 2d to 3d use
	MTK_PQ_3D_OUTPUT_LINE_ALTERNATIVE_HW,            //for hw 2d to 3d use
	MTK_PQ_3D_OUTPUT_PIXEL_ALTERNATIVE_HW,           //for hw 2d to 3d use
	MTK_PQ_3D_OUTPUT_FRAME_L_HW,                     //for hw 2d to 3d use
	MTK_PQ_3D_OUTPUT_FRAME_R_HW,                     //for hw 2d to 3d use
	MTK_PQ_3D_OUTPUT_FRAME_ALTERNATIVE_HW,           //for hw 2d to 3d use
	MTK_PQ_3D_OUTPUT_TOP_BOTTOM_HW,
	MTK_PQ_3D_OUTPUT_SIDE_BY_SIDE_HALF_HW,
	MTK_PQ_3D_OUTPUT_FRAME_PACKING,
	MTK_PQ_3D_OUTPUT_FRAME_ALTERNATIVE_LLRR,         //for 4k0.5k@240 3D
	MTK_PQ_3D_OUTPUT_FRAME_ALTERNATIVE_LLRR_HW,
	MTK_PQ_3D_OUTPUT_MAX,
};

enum mtk_pq_3d_panel_type {
	MTK_PQ_3D_PANEL_NONE,
	MTK_PQ_3D_PANEL_SHUTTER,
	MTK_PQ_3D_PANEL_PELLICLE,
	MTK_PQ_3D_PANEL_4K1K_SHUTTER,
	MTK_PQ_3D_PANEL_MAX,
};

enum mtk_pq_hdr_type {
	MTK_PQ_HDR_NONE,
	MTK_PQ_HDR_SWDR,
	MTK_PQ_HDR_HDR10,
	MTK_PQ_HDR_HLG,
	MTK_PQ_HDR_TCH,
	MTK_PQ_HDR_DOLBY,
	MTK_PQ_HDR_MAX,
};

enum mtk_pq_capt_color_format {
	MTK_PQ_CAPTURE_COLOR_FORMAT_16X32TILE,//YUV420 16(w)x32(h) tiled format
	MTK_PQ_CAPTURE_COLOR_FORMAT_32X16TILE,//YUV420 32(w)x16(h) tiled format
	MTK_PQ_CAPTURE_COLOR_FORMAT_32X32TILE,//YUV420 32(w)x32(h) tiled format
	MTK_PQ_CAPTURE_COLOR_FORMAT_YUYV,     //YUV422 YUYV
	MTK_PQ_CAPTURE_COLOR_FORMAT_YVYU,     //YUV422 YVYU
	MTK_PQ_CAPTURE_COLOR_FORMAT_UYVY,     //YUV422 UYVY
	MTK_PQ_CAPTURE_COLOR_FORMAT_VYUY,     //YUV422 VYUY
	MTK_PQ_CAPTURE_COLOR_FORMAT_NV12,     //YUV420 SemiPlanar
	MTK_PQ_CAPTURE_COLOR_FORMAT_NV21,     //YUV420 SemiPlanar
	MTK_PQ_CAPTURE_COLOR_FORMAT_MAX,
};

enum mtk_pq_capt_type {
	MTK_PQ_CAPTURE_INDIVIDUAL_WINDOW,
	MTK_PQ_CAPTURE_ALL_SCREEN,
	MTK_PQ_CAPTURE_ALL_SCREEN_WITHOUT_OSD,
	MTK_PQ_CAPTURE_MAX,
};

enum mtk_pq_capt_aspect_ratio {
	MTK_PQ_CAPTURE_DAR_DEFAULT = 0,
	MTK_PQ_CAPTURE_DAR_16x9,
	MTK_PQ_CAPTURE_DAR_4x3,
	MTK_PQ_CAPTURE_DAR_MAX,
};

enum mtk_pq_aspect_ratio {
	MTK_PQ_AR_DEFAULT = 0,
	MTK_PQ_AR_16x9,
	MTK_PQ_AR_4x3,
	MTK_PQ_AR_AUTO,
	MTK_PQ_AR_Panorama,
	MTK_PQ_AR_JustScan,
	MTK_PQ_AR_Zoom1,
	MTK_PQ_AR_Zoom2,
	MTK_PQ_AR_14x9,
	MTK_PQ_AR_DotByDot,
	MTK_PQ_AR_Subtitle,
	MTK_PQ_AR_Movie,
	MTK_PQ_AR_Personal,
	MTK_PQ_AR_4x3_PanScan,
	MTK_PQ_AR_4x3_LetterBox,
	MTK_PQ_AR_16x9_PillarBox,
	MTK_PQ_AR_16x9_PanScan,
	MTK_PQ_AR_4x3_Combind,
	MTK_PQ_AR_16x9_Combind,
	MTK_PQ_AR_Zoom_2x,
	MTK_PQ_AR_Zoom_3x,
	MTK_PQ_AR_Zoom_4x,
	MTK_PQ_AR_CUS = 0x20,
	MTK_PQ_AR_MAX = 0x40,
};

enum mtk_pq_frame_type {
	MTK_PQ_FRAME_TYPE_I,
	MTK_PQ_FRAME_TYPE_P,
	MTK_PQ_FRAME_TYPE_B,
	MTK_PQ_FRAME_TYPE_OTHER,
	MTK_PQ_FRAME_TYPE_MAX,
};

enum mtk_pq_field_type {
	MTK_PQ_FIELD_TYPE_NONE,
	MTK_PQ_FIELD_TYPE_TOP,
	MTK_PQ_FIELD_TYPE_BOTTOM,
	MTK_PQ_FIELD_TYPE_BOTH,
	MTK_PQ_FIELD_TYPE_MAX,
};

enum mtk_pq_view_type {
	MTK_PQ_VIEW_TYPE_CENTER,
	MTK_PQ_VIEW_TYPE_LEFT,
	MTK_PQ_VIEW_TYPE_RIGHT,
	MTK_PQ_VIEW_TYPE_TOP,
	MTK_PQ_VIEW_TYPE_BOTTOM,
	MTK_PQ_VIEW_TYPE_MAX,
};

enum mtk_pq_color_format {
	MTK_PQ_COLOR_FORMAT_HW_HVD,//YUV420 HVD tiled format
	MTK_PQ_COLOR_FORMAT_HW_MVD,//YUV420 MVD tiled format
	MTK_PQ_COLOR_FORMAT_SW_YUV420_PLANAR,//YUV420 Planar
	MTK_PQ_COLOR_FORMAT_SW_RGB565,//RGB565
	MTK_PQ_COLOR_FORMAT_SW_ARGB8888,//ARGB8888
	MTK_PQ_COLOR_FORMAT_YUYV,//YUV422 YUYV
	MTK_PQ_COLOR_FORMAT_SW_RGB888,//RGB888
	MTK_PQ_COLOR_FORMAT_10BIT_TILE,//YUV420 tiled 10 bits mode
	MTK_PQ_COLOR_FORMAT_SW_YUV420_SEMIPLANAR,//YUV420 SemiPlanar
	MTK_PQ_COLOR_FORMAT_YUYV_CSC_BIT601,
	//YUV422 YUYV from RGB2YUV bit601 mode
	MTK_PQ_COLOR_FORMAT_YUYV_CSC_255,//YUV422 YUYV from RGB2YUV 0~255 mode
	MTK_PQ_COLOR_FORMAT_HW_EVD,//YUV420 EVD tiled format
	MTK_PQ_COLOR_FORMAT_HW_32x32,//YUV420 32x32 tiled format (32x32)
	MTK_PQ_COLOR_FORMAT_MAX,
};

enum mtk_pq_frame_ext_type {
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_10BIT,
	// in MVC case it is L view 2 bit
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_INTERLACE = 1,
	// interlace bottom 8bit will share the same enum value
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_DOLBY_EL = 1,
	// with dolby enhance layer 8bit
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_10BIT_INTERLACE = 2,
	// interlace bottom 2bit will share the same enum
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_10BIT_DOLBY_EL = 2,
	// value with dolby enhance layer 2bit
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_10BIT_MVC,
	// R view 2 bit
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_INTERLACE_MVC,
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_10BIT_INTERLACE_MVC = 5,
	// MVC interlace R-View 2bit will share the
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_DOLBY_META = 5,
	// same enum with dolby meta data
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_MFCBITLEN,
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_MFCBITLEN_MVC,
	MTK_PQ_DISP_FRM_INFO_EXT_TYPE_MAX,
};

enum mtk_pq_display_ctrl_type {
	//MTK_PQ_DISPLAY_CTRL_TYPE_INIT,
	MTK_PQ_DISPLAY_CTRL_TYPE_CREATEWINDOW = 0x1,
	MTK_PQ_DISPLAY_CTRL_TYPE_SETWINDOW,
	MTK_PQ_DISPLAY_CTRL_TYPE_DESTROYWINDOW,
	MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWINFO,
	MTK_PQ_DISPLAY_CTRL_TYPE_GETWINDOWID,//maybe not necessary
	//MTK_PQ_DISPLAY_CTRL_TYPE_SETDROPLINE,//will set before use
	//MTK_PQ_DISPLAY_CTRL_TYPE_SETFORCEP,//use V4L2_CID_DS_INFO
	//MTK_PQ_DISPLAY_CTRL_TYPE_SETXCENFBL,//use V4L2_CID_FB_MODE
	MTK_PQ_DISPLAY_CTRL_TYPE_SETENSWFRC,
	//MTK_PQ_DISPLAY_CTRL_TYPE_SETSCALINGCONDITION,//will set before use
	MTK_PQ_DISPLAY_CTRL_TYPE_FRAME_RELEASE_DATA,
	MTK_PQ_DISPLAY_CTRL_TYPE_CALLBACK_DATA,
	MTK_PQ_DISPLAY_CTRL_TYPE_VIDEO_DELAY_TIME,
	MTK_PQ_DISPLAY_CTRL_TYPE_MAX,
};

enum mtk_pq_drop_line {
	MTK_PQ_MVOP_DROP_LINE_DISABLE = 0,
	MTK_PQ_MVOP_DROP_LINE_1_2,
	MTK_PQ_MVOP_DROP_LINE_1_4,
	MTK_PQ_MVOP_DROP_LINE_1_8,
	MTK_PQ_MVOP_DROP_LINE_MAX_NUM,
};

enum mtk_pq_update_height {
	MTK_PQ_UPDATE_HEIGHT_MVOP = 0,
	MTK_PQ_UPDATE_HEIGHT_XC,
	MTK_PQ_UPDATE_HEIGHT_XC_MAX_NUM,
};

enum mtk_pq_input_source_type {
	MTK_PQ_INPUTSRC_NONE = 0,      ///<NULL input
	MTK_PQ_INPUTSRC_VGA,           ///<1   VGA input
	MTK_PQ_INPUTSRC_TV,            ///<2   TV input

	MTK_PQ_INPUTSRC_CVBS,          ///<3   AV 1
	MTK_PQ_INPUTSRC_CVBS2,         ///<4   AV 2
	MTK_PQ_INPUTSRC_CVBS3,         ///<5   AV 3
	MTK_PQ_INPUTSRC_CVBS4,         ///<6   AV 4
	MTK_PQ_INPUTSRC_CVBS5,         ///<7   AV 5
	MTK_PQ_INPUTSRC_CVBS6,         ///<8   AV 6
	MTK_PQ_INPUTSRC_CVBS7,         ///<9   AV 7
	MTK_PQ_INPUTSRC_CVBS8,         ///<10   AV 8
	MTK_PQ_INPUTSRC_CVBS_MAX,      ///<11 AV max

	MTK_PQ_INPUTSRC_SVIDEO,        ///<12 S-video 1
	MTK_PQ_INPUTSRC_SVIDEO2,       ///<13 S-video 2
	MTK_PQ_INPUTSRC_SVIDEO3,       ///<14 S-video 3
	MTK_PQ_INPUTSRC_SVIDEO4,       ///<15 S-video 4
	MTK_PQ_INPUTSRC_SVIDEO_MAX,    ///<16 S-video max

	MTK_PQ_INPUTSRC_YPBPR,         ///<17 Component 1
	MTK_PQ_INPUTSRC_YPBPR2,        ///<18 Component 2
	MTK_PQ_INPUTSRC_YPBPR3,        ///<19 Component 3
	MTK_PQ_INPUTSRC_YPBPR_MAX,     ///<20 Component max

	MTK_PQ_INPUTSRC_SCART,         ///<21 Scart 1
	MTK_PQ_INPUTSRC_SCART2,        ///<22 Scart 2
	MTK_PQ_INPUTSRC_SCART_MAX,     ///<23 Scart max

	MTK_PQ_INPUTSRC_HDMI,          ///<24 HDMI 1
	MTK_PQ_INPUTSRC_HDMI2,         ///<25 HDMI 2
	MTK_PQ_INPUTSRC_HDMI3,         ///<26 HDMI 3
	MTK_PQ_INPUTSRC_HDMI4,         ///<27 HDMI 4
	MTK_PQ_INPUTSRC_HDMI_MAX,      ///<28 HDMI max

	MTK_PQ_INPUTSRC_DVI,           ///<29 DVI 1
	MTK_PQ_INPUTSRC_DVI2,          ///<30 DVI 2
	MTK_PQ_INPUTSRC_DVI3,          ///<31 DVI 2
	MTK_PQ_INPUTSRC_DVI4,          ///<32 DVI 4
	MTK_PQ_INPUTSRC_DVI_MAX,       ///<33 DVI max

	MTK_PQ_INPUTSRC_DTV,           ///<34 DTV
	MTK_PQ_INPUTSRC_DTV2,          ///<35 DTV2
	MTK_PQ_INPUTSRC_DTV_MAX,       ///<36 DTV max

	// Application source
	MTK_PQ_INPUTSRC_STORAGE,       ///<37 Storage
	MTK_PQ_INPUTSRC_STORAGE2,      ///<38 Storage2
	MTK_PQ_INPUTSRC_STORAGE_MAX,   ///<39 Storage max

	// Support OP capture
	MTK_PQ_INPUTSRC_SCALER_OP,     ///<40 scaler OP

	MTK_PQ_INPUTSRC_NUM,           ///<number of the source

};

enum mtk_pq_video_data_format {
	MTK_PQ_VIDEO_DATA_FMT_YUV422 = 0,    /// YCrYCb.
	MTK_PQ_VIDEO_DATA_FMT_RGB565,        /// RGB domain
	MTK_PQ_VIDEO_DATA_FMT_ARGB8888,      /// RGB domain
	MTK_PQ_VIDEO_DATA_FMT_YUV420,        /// YUV420 HVD tile
	MTK_PQ_VIDEO_DATA_FMT_YC422,          /// YC separate 422
	MTK_PQ_VIDEO_DATA_FMT_YUV420_H265,    /// YUV420 H265 tile
	MTK_PQ_VIDEO_DATA_FMT_YUV420_H265_10BITS,/// YUV420 H265_10bits tile
	MTK_PQ_VIDEO_DATA_FMT_YUV420_PLANER, /// YUV420 planer
	MTK_PQ_VIDEO_DATA_FMT_YUV420_SEMI_PLANER,/// YUV420 semi planer
	MTK_PQ_VIDEO_DATA_FMT_MAX
};

enum mtk_pq_order_type {
	MTK_PQ_VIDEO_ORDER_TYPE_BOTTOM = 0,
	MTK_PQ_VIDEO_ORDER_TYPE_TOP,
	MTK_PQ_VIDEO_ORDER_TYPE_MAX,
};

enum mtk_pq_scan_type {
	MTK_PQ_VIDEO_SCAN_TYPE_PROGRESSIVE = 0,
	MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FRAME,
	MTK_PQ_VIDEO_SCAN_TYPE_INTERLACE_FIELD,
	MTK_PQ_VIDEO_SCAN_TYPE_MAX,
};

enum mtk_pq_rotate_type {
	MTK_PQ_ROTATE_TYPE_0 = 0,
	MTK_PQ_ROTATE_TYPE_90,
	MTK_PQ_ROTATE_TYPE_180,
	MTK_PQ_ROTATE_TYPE_270,
	MTK_PQ_ROTATE_TYPE_MAX,
};

enum mtk_pq_get_delay_type {
	MTK_PQ_GET_DISPLAY_DELAY = 0,
	MTK_PQ_GET_DELAY_MAX,
};

enum mtk_pq_input_mode {
	MTK_PQ_INPUT_MODE_BYPASS,
	MTK_PQ_INPUT_MODE_SW,
	MTK_PQ_INPUT_MODE_HW,
	MTK_PQ_INPUT_MODE_MAX,
};

enum mtk_pq_output_mode {
	MTK_PQ_OUTPUT_MODE_BYPASS,
	MTK_PQ_OUTPUT_MODE_SW,
	MTK_PQ_OUTPUT_MODE_HW,
	MTK_PQ_OUTPUT_MODE_MAX,
};

enum mtk_pq_fmt_modifier_vsd {
	PQ_VSD_NONE = 0,
	PQ_VSD_8x2 = 1,
	PQ_VSD_8x4 = 2,
};

//B2R
enum v4l2_ext_sync_status {
	V4L2_EXT_VSYNC_POR_BIT = 0x1,
	V4L2_EXT_HSYNC_POR_BIT = 0x2,
	V4L2_EXT_HSYNC_LOSS_BIT = 0x4,
	V4L2_EXT_VSYNC_LOSS_BIT = 0x8,
	V4L2_EXT_USER_MODE_BIT = 0x10,
	V4L2_EXT_SYNC_LOSS = 0xC,
};

//XC
enum v4l2_init_misc_a {
	V4L2_INIT_MISC_A_NULL = 0,
	V4L2_INIT_MISC_A_IMMESWITCH = 1,
	V4L2_INIT_MISC_A_IMMESWITCH_DVI_POWERSAVING = 2,
	V4L2_INIT_MISC_A_DVI_AUTO_EQ = 4,
	V4L2_INIT_MISC_A_FRC_INSIDE = 8,
	V4L2_INIT_MISC_A_OSD_TO_HSLVDS = 0x10,
	V4L2_INIT_MISC_A_FRC_INSIDE_60HZ = 0x20,
	V4L2_INIT_MISC_A_FRC_INSIDE_240HZ = 0x40,
	V4L2_INIT_MISC_A_FRC_INSIDE_4K1K_120HZ = 0x80,
	V4L2_INIT_MISC_A_ADC_AUTO_SCART_SV = 0x100,
	V4L2_INIT_MISC_A_FRC_INSIDE_KEEP_OP_4K2K = 0x200,
	V4L2_INIT_MISC_A_FRC_INSIDE_4K_HALFK_240HZ = 0x400,
	V4L2_INIT_MISC_A_FCLK_DYNAMIC_CONTROL = 0x800,
	V4L2_INIT_MISC_A_SKIP_UC_DOWNLOAD = 0x1000,
	V4L2_INIT_MISC_A_SKIP_VIP_PEAKING_CONTROL = 0x2000,
	V4L2_INIT_MISC_A_FAST_GET_VFREQ = 0x4000,
	V4L2_INIT_MISC_A_LEGACY_MODE = 0x8000,
	V4L2_INIT_MISC_A_SAVE_MEM_MODE = 0x10000,
	V4L2_INIT_MISC_A_FRC_DUAL_MIU  = 0x20000,
	V4L2_INIT_MISC_A_IS_ANDROID = 0x40000,
	V4L2_INIT_MISC_A_ENABLE_CVBSOX_ADCX = 0x80000,
	V4L2_INIT_MISC_A_IPMUX_HDR_MODE = 0x100000,
	V4L2_INIT_MISC_A_SWITCH_MVOP_TO_XC_PATH   = 0x200000,
	V4L2_INIT_MISC_A_FRC_SPREAD = 0x400000,
};

enum v4l2_init_misc_b {
	V4L2_INIT_MISC_B_NULL = 0,
	V4L2_INIT_MISC_B_PQ_SKIP_PCMODE_NEWFLOW = 1,
	V4L2_INIT_MISC_B_SKIP_SR = 2,
	V4L2_INIT_MISC_B_HDMITX_ENABLE = 4,
	V4L2_INIT_MISC_B_DRAM_DDR4_MODE = 8,
	V4L2_INIT_MISC_B_HDMITX_FREERUN_UCNR_ENABLE = 0x10,
	V4L2_INIT_MISC_B_DISABLE_SUPPORT_4K2K = 0x20,
	V4L2_INIT_MISC_B_DISABLE_FSS = 0x40,
};

enum v4l2_init_misc_c {
	V4L2_INIT_MISC_C_NULL = 0,
	V4L2_INIT_MISC_C_MM_PHOTO_SPC = 1,
	V4L2_INIT_MISC_C_NO_DELAY_ML = 2,
};

enum v4l2_init_misc_d {
	V4L2_INIT_MISC_D_NULL = 0,
};

enum v4l2_scaling_type {
	V4L2_SCALING_TYPE_PRE,
	V4L2_SCALING_TYPE_POST,
	V4L2_SCALING_TYPE_MAX,
};

enum v4l2_vector_type {
	V4L2_VECTOR_TYPE_H,
	V4L2_VECTOR_TYPE_V,
	V4L2_VECTOR_TYPE_HV,
	V4L2_VECTOR_TYPE_MAX,
};

enum v4l2_pq_ds_type {
	V4L2_PQ_DS_TYPE_GRULE,
	V4L2_PQ_DS_TYPE_XRULE,
	V4L2_PQ_DS_TYPE_GRULE_XRULE,
	V4L2_PQ_DS_TYPE_MAX,
};

enum v4l2_fb_level {
	V4L2_FB_LEVEL_FB,                      //  frame buff mode //FB
	V4L2_FB_LEVEL_FBL,                     // same as fbl,not use miu //FBL
	V4L2_FB_LEVEL_RFBL_DI,                 // use miu to deinterlace //RFBL
	V4L2_FB_LEVEL_FB_VSD,                  // FB + VSD (vdec 2nd buffer)
	V4L2_FB_LEVEL_FBL_VSD,                 // FBL + VSD (vdec 2nd buffer)
	V4L2_FB_LEVEL_RFBL_VSD,                //RFBL + VSD (vdec 2nd buffer)
	V4L2_FB_LEVEL_FB_MVOPDROPLINE,         //FB + mvop drop line
	V4L2_FB_LEVEL_FBL_MVOPDROPLINE,        //FBL + mvop drop line
	V4L2_FB_LEVEL_RFBL_MVOPDROPLINE,       //RFBL + mvop drop line
	V4L2_FB_LEVEL_NUM,                     // number
};

enum v4l2_deinterlace_mode {

	V4L2_DEINTERLACE_MODE_OFF = 0,
	///<deinterlace mode off
	V4L2_DEINTERLACE_MODE_2DDI_BOB,
	///<deinterlace mode: BOB
	V4L2_DEINTERLACE_MODE_2DDI_AVG,
	///<deinterlace mode: AVG
	V4L2_DEINTERLACE_MODE_3DDI_HISTORY,
	///<deinterlace mode: HISTORY // 24 bit
	V4L2_DEINTERLACE_MODE_3DDI,
	///<deinterlace mode:3DDI// 16 bit
	V4L2_DEINTERLACE_MODE_MAX,
};

/// Image store format in XC
enum v4l2_image_store_fmt {
	V4L2_IMAGE_STORE_FMT_444_24BIT,    ///< (8+8+8)   Y Cb Cr   / B G R
	V4L2_IMAGE_STORE_FMT_422_16BIT,    ///< (8+8)     Y Cb Y Cr / G B G R
	V4L2_IMAGE_STORE_FMT_422_24BIT,
	///< (10+10+4) Y Cb Y Cr / G B G R --
	//Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 - C5 C4 C3 C2 C1 C0 Y9 Y8
	//- xx xx xx xx C9 C8 C7 C6
	V4L2_IMAGE_STORE_FMT_MAX,
};

enum v4l2_test_pattern_mode {
	V4L2_TEST_PATTERN_MODE_IP1,               // ip test Pattern
	V4L2_TEST_PATTERN_MODE_OP,              // op test Pattern
	V4L2_TEST_PATTERN_MODE_IP2,           // ip2 test Pattern
	V4L2_TEST_PATTERN_MODE_MAX,
};

enum v4l2_pixel_rgb_stage {
	V4L2_PIXEL_RGB_STAGE_AFTER_DLC,
	V4L2_PIXEL_RGB_STAGE_PRE_GAMMA,
	V4L2_PIXEL_RGB_STAGE_AFTER_OSD,
	V4L2_PIXEL_RGB_STAGE_AFTER_DLC_YCBCR,
	V4L2_PIXEL_RGB_STAGE_MAX,
};

enum v4l2_cap_type {
	V4L2_CAP_TYPE_IMMESWITCH,
	///< return true if H/W support HDMI immeswitch
	V4L2_CAP_TYPE_DVI_AUTO_EQ,
	///< return true if driver support Auto EQ.
	V4L2_CAP_TYPE_FRC_INSIDE,
	///< return true if scaler driver support FRC (MFC) function.
	V4L2_CAP_TYPE_DIP_CHIP_CAPS,
	///< return dip chip caps.
	V4L2_CAP_TYPE_3D_FBL_CAPS,
	///< return true if chip support fbl 3d.attention:
	//fbl 3d only support sbs to lbl and sbs to sbs
	V4L2_CAP_TYPE_HW_SEAMLESS_ZAPPING,
	///< return true if H/W support seamless zapping
	V4L2_CAP_TYPE_SUPPORT_DEVICE1,
	///< return true if H/W support scaler device1
	V4L2_CAP_TYPE_SUPPORT_DETECT3D_IN3DMODE,
	///< return true if H/W support detecting 3d when already in 3d mode
	V4L2_CAP_TYPE_2DTO3D_VERSION,
	///< return value 2D-to-3D version
	V4L2_CAP_TYPE_SUPPORT_FORCE_VSP_IN_DS_MODE,
	///< return if H/W support force post-Vscalin-down in DS mode
	V4L2_CAP_TYPE_SUPPORT_FRCM_MODE,
	///< return if H/W support frcm
	V4L2_CAP_TYPE_SUPPORT_INTERLACE_OUT,
	///< return if H/W supports interlace output timing
	V4L2_CAP_TYPE_SUPPORT_4K2K_WITH_PIP,
	///< return if H/W supports output is 4k2k and open pip
	V4L2_CAP_TYPE_SUPPORT_4K2K_60P,
	///< return if H/W supports output is 4k2k_60P timing
	V4L2_CAP_TYPE_DIP_CHIP_SOURCESEL,
	///< return if H/W supports output is 4k2k_60P timing
	V4L2_CAP_TYPE_SUPPORT_PIP_PATCH_USING_SC1_MAIN_AS_SC0_SUB,
	///<return if sc1 support PIP
	V4L2_CAP_TYPE_HW_4K2K_VIP_PEAKING_LIMITATION,
	///return if H/W has vip peaking limitation
	V4L2_CAP_TYPE_SUPPORT_AUTODOWNLOAD_CLIENT,
	///return whether the client is supported, refer to
	//XC_AUTODOWNLOAD_CLIENT_SUPPORTED_CAPS
	V4L2_CAP_TYPE_SUPPORT_HDR,
	///return whether HDR is supported, refer to XC_HDR_SUPPORTED_CAPS
	V4L2_CAP_TYPE_SUPPORT_3D_DS,
	///return whether 3D DS is supported
	V4L2_CAP_TYPE_DIP_CHIP_WINDOWBUS,
	///< return DIP window bus length
	V4L2_CAP_TYPE_SCALING_LIMITATION,
	///return whether scaling is supported
	V4L2_CAP_TYPE_FB_CAPS_GET_FB_LEVEL,
	//get fb caps
	V4L2_CAP_TYPE_SUPPORT_SW_DS,
	///return true if support sw ds
	V4L2_CAP_TYPE_SUPPORT_HDMI_SEAMLESS,
	/// return true if support hdmi seamless
	V4L2_CAP_TYPE_IS_SUPPORT_SUB_DISP,
	///return true if support sub window display size
	V4L2_CAP_TYPE_HDR_HW_SUPPORT_HANDSHAKE_MODE,
	// return true if hdr hw support handshake mode
	V4L2_CAP_TYPE_SUPPORT_PQ_DS,
	///return true if support PQ ds
	V4L2_CAP_TYPE_SUPPORT_HW_HSY,
	/// return true if support HW HSY
	V4L2_CAP_TYPE_HW_SR_VERSION,
	///< return value SuperResolution version
	V4L2_CAP_TYPE_SUPPORT_MOD_HMIRROR,
	///return true if support MOD H Mirror
	V4L2_CAP_TYPE_HW_FAST_DISP,
	///return true if HW support Fast Display
	V4L2_CAP_TYPE_FB_CAPS_GET_FB_LEVEL_EX,
	//get fb caps EX
	V4L2_CAP_TYPE_SUPPORT_ZNR,/// return true if XC support ZNR
	V4L2_CAP_TYPE_SUPPORT_ABF,/// return true if XC support ABF
	V4L2_CAP_TYPE_SUPPORT_FRC_HSK_MODE,
	/// return true if support FBL post scaling
	V4L2_CAP_TYPE_SUPPORT_SPF_LB_MODE,
	/// return true if support SPF linebuffer
	V4L2_CAP_TYPE_SUPPORT_HSE,/// return true if support HSE
	V4L2_CAP_TYPE_SUPPORT_HFR_PATH,
	/// return true if support HFR path
	V4L2_CAP_TYPE_SUPPORT_PQ_DS_PLUS,
	/// return true if support PQDS+
	V4L2_CAP_TYPE_SUPPORT_HVSP_DS,
	/// return TRUE if support HVSP DS
	V4L2_CAP_TYPE_SUPPORT_DECIMAL_CROP,
	/// return true if support DecimalCrop
	V4L2_CAP_TYPE_MAX_WINDOW_NUM,
	/// return max window number that XC driver
	//supports such as MAIN/SUB window
	V4L2_CAP_TYPE_DIGITAL_DDCRAM_NUM,
	/// return how many Digital DDCRam that use
	//can use without external EDID
	//EEPROM
	V4L2_CAP_TYPE_MAX_FRAME_NUM_IN_MEM,
	/// return maximal number of frames (Progressive mode)
	//supported by scaler
	//simultaneously
	V4L2_CAP_TYPE_MAX_FIELD_NUM_IN_MEM,
	/// return maximal number of fields (Interlace mode)
	//supported by scaler
	//simultaneously
};

enum v4l2_dip_win {
	V4L2_DIP_WIN_WINDOW = 0,         ///< DIP window
	V4L2_DIP_WIN_DWIN0_WINDOW = 1,
	V4L2_DIP_WIN_DWIN1_WINDOW = 2,
	V4L2_DIP_WIN_MAX,          /// The max support window
};

/* autodownload client */
enum v4l2_adl_client {
	V4L2_ADL_CLIENT_HDR,
	V4L2_ADL_CLIENT_OP2GAMMA,
	V4L2_ADL_CLIENT_FRCOP2GAMMA,
	V4L2_ADL_CLIENT_XVYCC,
	V4L2_ADL_CLIENT_ODTABLE1,
	V4L2_ADL_CLIENT_ODTABLE2,
	V4L2_ADL_CLIENT_ODTABLE3,
	V4L2_ADL_CLIENT_DEMURA,
	V4L2_ADL_CLIENT_OP2LUT,
	V4L2_ADL_CLIENT_T3D_0,        /// t3d of sc0
	V4L2_ADL_CLIENT_T3D_1,        /// t3d of sc1
	V4L2_ADL_CLIENT_FRCSPTPOPM,
	V4L2_ADL_CLIENT_FOOPM,
	V4L2_ADL_CLIENT_HVSP,
	V4L2_ADL_CLIENT_HVSP_DIP,
	V4L2_ADL_CLIENT_SCRAMBLE,
	V4L2_ADL_CLIENT_OSD_HDR,
	V4L2_ADL_CLIENT_OSD_THDR,
	V4L2_ADL_CLIENT_ADL2RIU,
	V4L2_ADL_CLIENT_HSY,
	V4L2_ADL_CLIENT_PQ_GAMMA,
	V4L2_ADL_CLIENT_PANEL_GAMMA,
	V4L2_ADL_CLIENT_DLC_256,
	V4L2_ADL_CLIENT_ICC_IHC,
	V4L2_ADL_CLIENT_3DLUT,
	V4L2_ADL_CLIENT_SC2VIP,
	V4L2_ADL_CLIENT_RGBWGEN2,
	V4L2_ADL_CLIENT_OD,
	V4L2_ADL_CLIENT_PCID,
	V4L2_ADL_CLIENT_MAX,
};

enum v4l2_fast_disp_type {
	V4L2_FAST_DISP_TYPE_ZEROPQ,
	V4L2_FAST_DISP_TYPE_MAX,
};


//3D
enum v4l2_ext_3d_input_mode {
	V4L2_EXT_3D_INPUT_FRAME_PACKING                     = 0x00, //0000
	V4L2_EXT_3D_INPUT_FIELD_ALTERNATIVE                 = 0x01, //0001
	V4L2_EXT_3D_INPUT_LINE_ALTERNATIVE                  = 0x02, //0010
	V4L2_EXT_3D_INPUT_L_DEPTH                           = 0x04, //0100
	V4L2_EXT_3D_INPUT_L_DEPTH_GRAPHICS_GRAPHICS_DEPTH   = 0x05, //0101
	V4L2_EXT_3D_INPUT_TOP_BOTTOM                        = 0x06, //0110
	V4L2_EXT_3D_INPUT_SIDE_BY_SIDE_HALF                 = 0x08, //1000
	V4L2_EXT_3D_INPUT_CHECK_BOARD                       = 0x09, //1001
	V4L2_EXT_3D_INPUT_PIXEL_ALTERNATIVE                 = 0x1A,
	V4L2_EXT_3D_INPUT_FRAME_ALTERNATIVE                 = 0x11,
	V4L2_EXT_3D_INPUT_NORMAL_2D                         = 0x15,
	V4L2_EXT_3D_INPUT_MODE_NONE                         = 0x10,
};

enum v4l2_ext_3d_output_mode {
	V4L2_EXT_3D_OUTPUT_MODE_NONE,
	V4L2_EXT_3D_OUTPUT_LINE_ALTERNATIVE,
	V4L2_EXT_3D_OUTPUT_TOP_BOTTOM,
	V4L2_EXT_3D_OUTPUT_SIDE_BY_SIDE_HALF,
	V4L2_EXT_3D_OUTPUT_FRAME_ALTERNATIVE,
	//25-->50,30-->60,24-->48,50-->100,60-->120----FRC 1:2
	V4L2_EXT_3D_OUTPUT_FRAME_L,
	V4L2_EXT_3D_OUTPUT_FRAME_R,
	V4L2_EXT_3D_OUTPUT_FRAME_PACKING,
	V4L2_EXT_3D_OUTPUT_FRAME_ALTERNATIVE_LLRR,//for 4k0.5k@240 3D
};

enum v4l2_3d_lr_frame_exchg {
	V4L2_3d_lr_frame_exchg_DISABLE,
	V4L2_3d_lr_frame_exchg_ENABLE,
};

/*COMMON*/
enum v4l2_ext_info_type {
	V4L2_EXT_INFO_TYPE_TIMING,
	V4L2_EXT_INFO_TYPE_3D,
	V4L2_EXT_INFO_TYPE_WINDOW,
	V4L2_EXT_INFO_TYPE_ATV,
	V4L2_EXT_INFO_TYPE_DTV,
	V4L2_EXT_INFO_TYPE_HDMI,
	V4L2_EXT_INFO_TYPE_DVI,
	V4L2_EXT_INFO_TYPE_VGA,
	V4L2_EXT_INFO_TYPE_COMP,
	V4L2_EXT_INFO_TYPE_AV,
	V4L2_EXT_INFO_TYPE_SV,
	V4L2_EXT_INFO_TYPE_SCART,
	V4L2_EXT_INFO_TYPE_MM,

	V4L2_EXT_INFO_TYPE_COLOR_OUTPUT,
};

enum v4l2_delay_time_type {
	V4L2_EXT_DELAY_DISPLAYWITHPQ       = 0x00,
	V4L2_EXT_DELAY_FIRSTFRAMEOUT       = 0x01,
	V4L2_EXT_DELAY_FIRSTFRAMEOUT_AVG   = 0x02,
	V4L2_EXT_DELAY_MAX,
};

enum v4l2_mfc_level {
	V4L2_EXT_MFC_LEVEL_OFF,
	V4L2_EXT_MFC_LEVEL_ON,
	V4L2_EXT_MFC_LEVEL_BYPASS,
};

enum v4l2_pq_buffer_type {
	V4L2_PQ_BUFFER_SCMI     = 0x01, //bit0
	V4L2_PQ_BUFFER_ZNR      = 0x02, //bit1
	V4L2_PQ_BUFFER_UCM      = 0x04, //bit2
	V4L2_PQ_BUFFER_ABF      = 0x08, //bit3
};

enum v4l2_ext_pq_pattern {
	V4L2_EXT_PQ_B2R_PAT_FRAMECOLOR,
};

/************************************************
 ********************STRUCTs**********************
 ************************************************
 */

struct v4l2_pq_buf {
	// frame fd
	int frame_fd;
	// metadata fd
	int meta_fd;
};

//pq-enhance:HDR
struct v4l2_dolby_3dlut {
	__u32 size;
	union{
		__u64 addr;
		__u8 *pu8_data;
	} p;
} __attribute__((__packed__));

struct v4l2_sei_info {
	__u16 display_primaries_x[3];
	__u16 display_primaries_y[3];
	__u16 white_point_x;
	__u16 white_point_y;
	__u16 max_display_mastering_luminance;
	__u16 min_display_mastering_luminance;
};

struct v4l2_cll_info {
	__u16 max_content_light_level;
	__u16 max_frame_average_Light_Level;
};

struct v4l2_cus_colori_metry {
	bool b_customer_color_primaries;
	__u16 white_point_x;
	__u16 white_point_y;
};

struct v4l2_dolby_gd_info {
	bool b_global_dimming;
	__s8 mm_delay_frame;
	__s8 hdmi_delay_frame;
};

struct v4l2_cus_color_range {
	__u8 cus_type;  //enum v4l2_cus_color_range_type
	__u8 ultral_black_level;        //0~64
	__u8 ultral_white_level;         //0~64
};

struct v4l2_seamless_status {
	bool b_hdr_changed;
	bool b_mute_done;
	bool b_cfd_updated;
	bool b_reset_flag;
};

struct v4l2_hdr_attr_info {
	__u8 hdr_type;   //enum v4l2_hdr_type
	bool b_output_max_luminace;
	__u16 output_max_luminace;
	bool b_output_tr;
	__u8 output_tr;   //enum v4l2_hdr_output_tr_type
};

struct v4l2_cus_ip_setting {
	__u8 ip;  //enum v4l2_cus_ip_type
	union {
		__u64 addr;
		__u8 *pu8_param;
	} p;
	__u32 param_size;
} __attribute__((__packed__));

struct v4l2_color_out_ext_info {
	__u16 display_primaries_x[3];
	__u16 display_primaries_y[3];
	__u16 white_point_x;
	__u16 white_point_y;
	__u16 max_luminance;
	__u16 med_luminance;
	__u16 min_luminance;
	bool b_linear_rgb;
};
//pq-enhance:PQ
struct v4l2_vd_ext_info {
	bool b_scart_rgb;
	__u8 std_type; //define as enum v4l2_ext_video_standard
	bool b_vif_in;
};

enum v4l2_pq_dbg_event {
	V4L2_PQ_DBG_PQ,
	V4L2_PQ_DBG_ACE,
	V4L2_PQ_DBG_MAX,
};

struct v4l2_pq_dbg_info {
	enum v4l2_pq_dbg_event DBGEvent;
	__u16 DBGLevel;
} __attribute__((__packed__));

enum v4l2_pq_madi_force_motion_event {
	V4L2_PQ_MADI_FORCE_MOTION,
	V4L2_PQ_MADI_FORCE_MOTION_Y,
	V4L2_PQ_MADI_FORCE_MOTION_C,
	V4L2_PQ_MADI_FORCE_MOTION_MAX,
};

struct v4l2_pq_madi_force_motion {
	enum v4l2_pq_madi_force_motion_event MotionEvent;
	bool      EnableY;
	__u16     DataY;
	bool      EnableC;
	__u16     DataC;
} __attribute__((__packed__));

struct v4l2_pq_rfbl_info {
	bool     Enable;
	bool     Film;
	__u8     MADiType;
} __attribute__((__packed__));

struct v4l2_pq_table_status {
	__u16  TableType;
	__u16  TableStatus;
} __attribute__((__packed__));

enum v4l2_pq_ucd_level {
	V4L2_E_PQ_UCD_OFF,
	V4L2_E_PQ_UCD_LOW,
	V4L2_E_PQ_UCD_MID,
	V4L2_E_PQ_UCD_HIGH,
	V4L2_E_PQ_UCD_MAX,
};

struct v4l2_pq_ucd_level_info {
	__u32 Version;
	enum v4l2_pq_ucd_level Level_Index;
} __attribute__((__packed__));

struct v4l2_pq_hsb_bypass {
	__u32   Version;
	__u8    HsbIdx;
	bool    Bypass;
} __attribute__((__packed__));

enum v4l2_pq_picture_mode {
	V4L2_E_PQ_PICTUREMODE_AUTO,
	V4L2_E_PQ_PICTUREMODE_GAME,
	V4L2_E_PQ_PICTUREMODE_LIGHTNESS,
	V4L2_E_PQ_PICTUREMODE_NATURAL,
	V4L2_E_PQ_PICTUREMODE_SOFT,
	V4L2_E_PQ_PICTUREMODE_SPORTS,

	V4L2_E_PQ_PICTUREMODE_STANDARD,
	V4L2_E_PQ_PICTUREMODE_USER,
	V4L2_E_PQ_PICTUREMODE_VIVID,
	V4L2_E_PQ_PICTUREMODE_PC,
	V4L2_E_PQ_PICTUREMODE_AI_PQ,
	V4L2_E_PQ_PICTUREMODE_OFF,
	V4L2_E_PQ_PICTUREMODE_MAX,
};

enum v4l2_pq_hsy_grule_ip {
	V4L2_E_PQ_HSY_GRULE_IP_PICTUREMODE,
	V4L2_E_PQ_HSY_GRULE_IP_MAX,
};

enum v4l2_pq_hsy_grule_event {
	V4L2_E_HSY_GRULE_SET,
	V4L2_E_HSY_GRULE_GET,
	V4L2_E_HSY_GRULE_MAX,
};

struct v4l2_pq_hsy_grule_info {
	enum v4l2_pq_hsy_grule_event HsyGruleEvent;
	enum v4l2_pq_hsy_grule_ip HsyGruleIP;
	enum v4l2_pq_picture_mode PictureMode;
} __attribute__((__packed__));

enum v4l2_pq_ptp_type {
	V4L2_E_PQ_PTP_PTP,
	V4L2_E_PQ_PTP_NUM,
};

struct v4l2_pq_nr_settings {
	__u32  Version;
	__u8   Ctrl;
	__u8   Ctrl2;
	bool   ControlFlag;
	__u8   NRStrength;
	__u8   DNRLut[V4L2_NR_CONTROL_STEP_NUM];
} __attribute__((packed));

enum v4l2_pq_dpu_event {
	V4L2_E_PQ_DPU_SET,
	V4L2_E_PQ_DPU_GET,
	V4L2_E_PQ_DPU_MAX,
};

enum v4l2_pq_dpu_mod_type {
	V4L2_E_PQ_DPU_MOD_SET_NONE = 0,
	V4L2_E_PQ_DPU_MOD_SET_BYPASS,
	V4L2_E_PQ_DPU_MOD_SET_RESUME,
	V4L2_E_PQ_DPU_MOD_SET_TRIGGER_ALL,
	V4L2_E_PQ_DPU_MOD_SET_MAX,
};

enum v4l2_pq_dpu_mod_status {
	V4L2_E_PQ_DPU_MOD_STATUS_NONE = 0,
	V4L2_E_PQ_DPU_MOD_STATUS_BYPASS,
	V4L2_E_PQ_DPU_MOD_STATUS_AVAIL,
	V4L2_E_PQ_DPU_MOD_STATUS_MAX,
};

struct v4l2_pq_dpu_info {
	__u32   Version;
	enum v4l2_pq_dpu_event DpuEvent;
	enum v4l2_pq_dpu_mod_type DpuModType;
	enum v4l2_pq_dpu_mod_status DpuModStatus;
	__u64   DpuMod;
} __attribute__((__packed__));

enum v4l2_pq_ccm_event {
	V4L2_E_PQ_CCM_SET_BYPASSMATRIX,
	V4L2_E_PQ_CCM_SELECT_Y2RMATRIX,
	V4L2_E_PQ_CCM_SET_CORRECTIONTABLE,
	V4L2_E_PQ_CCM_CORRECTIONTABLE,
	V4L2_E_PQ_CCM_MAX,
};

struct v4l2_pq_ccm_info {
	enum v4l2_pq_ccm_event CCMEvent;
	__u8  Matrix;
	__s16 UserYUVtoRGBMatrix[3][3];
	__u16 ColorMatrix[3][3];
	__s16 ColorCorrectionMatrix[32];
	bool EnableBypss;
} __attribute__((__packed__));

enum v4l2_pq_color_tuner_event {
	V4L2_E_PQ_COLOR_TUNER_IHC,
	V4L2_E_PQ_COLOR_TUNER_ICC,
	V4L2_E_PQ_COLOR_TUNER_IBC,
	V4L2_E_PQ_COLOR_TUNER_MAX,
};

enum v4l2_pq_ihc_type {
	V4L2_E_PQ_IHC_COLOR_R,
	V4L2_E_PQ_IHC_COLOR_G,
	V4L2_E_PQ_IHC_COLOR_B,
	V4L2_E_PQ_IHC_COLOR_C,
	V4L2_E_PQ_IHC_COLOR_M,
	V4L2_E_PQ_IHC_COLOR_Y,
	V4L2_E_PQ_IHC_COLOR_F,
	V4L2_E_PQ_IHC_USER_COLOR1 = V4L2_E_PQ_IHC_COLOR_R,
	V4L2_E_PQ_IHC_USER_COLOR2 = V4L2_E_PQ_IHC_COLOR_G,
	V4L2_E_PQ_IHC_USER_COLOR3 = V4L2_E_PQ_IHC_COLOR_B,
	V4L2_E_PQ_IHC_USER_COLOR4 = V4L2_E_PQ_IHC_COLOR_C,
	V4L2_E_PQ_IHC_USER_COLOR5 = V4L2_E_PQ_IHC_COLOR_M,
	V4L2_E_PQ_IHC_USER_COLOR6 = V4L2_E_PQ_IHC_COLOR_Y,
	V4L2_E_PQ_IHC_USER_COLOR7 = V4L2_E_PQ_IHC_COLOR_F,
	V4L2_E_PQ_IHC_USER_COLOR8,
	V4L2_E_PQ_IHC_USER_COLOR9,
	V4L2_E_PQ_IHC_USER_COLOR10,
	V4L2_E_PQ_IHC_USER_COLOR11,
	V4L2_E_PQ_IHC_USER_COLOR12,
	V4L2_E_PQ_IHC_USER_COLOR13,
	V4L2_E_PQ_IHC_USER_COLOR14,
	V4L2_E_PQ_IHC_USER_COLOR15,
	V4L2_E_PQ_IHC_USER_COLOR0,
	V4L2_E_PQ_IHC_COLOR_MAX,
};

enum v4l2_pq_icc_type {
	V4L2_E_PQ_ICC_COLOR_R,
	V4L2_E_PQ_ICC_COLOR_G,
	V4L2_E_PQ_ICC_COLOR_B,
	V4L2_E_PQ_ICC_COLOR_C,
	V4L2_E_PQ_ICC_COLOR_M,
	V4L2_E_PQ_ICC_COLOR_Y,
	V4L2_E_PQ_ICC_COLOR_F,
	V4L2_E_PQ_ICC_USER_COLOR1 = V4L2_E_PQ_ICC_COLOR_R,
	V4L2_E_PQ_ICC_USER_COLOR2 = V4L2_E_PQ_ICC_COLOR_G,
	V4L2_E_PQ_ICC_USER_COLOR3 = V4L2_E_PQ_ICC_COLOR_B,
	V4L2_E_PQ_ICC_USER_COLOR4 = V4L2_E_PQ_ICC_COLOR_C,
	V4L2_E_PQ_ICC_USER_COLOR5 = V4L2_E_PQ_ICC_COLOR_M,
	V4L2_E_PQ_ICC_USER_COLOR6 = V4L2_E_PQ_ICC_COLOR_Y,
	V4L2_E_PQ_ICC_USER_COLOR7 = V4L2_E_PQ_ICC_COLOR_F,
	V4L2_E_PQ_ICC_USER_COLOR8,
	V4L2_E_PQ_ICC_USER_COLOR9,
	V4L2_E_PQ_ICC_USER_COLOR10,
	V4L2_E_PQ_ICC_USER_COLOR11,
	V4L2_E_PQ_ICC_USER_COLOR12,
	V4L2_E_PQ_ICC_USER_COLOR13,
	V4L2_E_PQ_ICC_USER_COLOR14,
	V4L2_E_PQ_ICC_USER_COLOR15,
	V4L2_E_PQ_ICC_USER_COLOR0,
	V4L2_E_PQ_ICC_COLOR_MAX,
};

enum v4l2_pq_ibc_type {
	V4L2_E_PQ_IBC_COLOR_R,
	V4L2_E_PQ_IBC_COLOR_G,
	V4L2_E_PQ_IBC_COLOR_B,
	V4L2_E_PQ_IBC_COLOR_C,
	V4L2_E_PQ_IBC_COLOR_M,
	V4L2_E_PQ_IBC_COLOR_Y,
	V4L2_E_PQ_IBC_COLOR_F,
	V4L2_E_PQ_IBC_USER_COLOR1 = V4L2_E_PQ_IBC_COLOR_R,
	V4L2_E_PQ_IBC_USER_COLOR2 = V4L2_E_PQ_IBC_COLOR_G,
	V4L2_E_PQ_IBC_USER_COLOR3 = V4L2_E_PQ_IBC_COLOR_B,
	V4L2_E_PQ_IBC_USER_COLOR4 = V4L2_E_PQ_IBC_COLOR_C,
	V4L2_E_PQ_IBC_USER_COLOR5 = V4L2_E_PQ_IBC_COLOR_M,
	V4L2_E_PQ_IBC_USER_COLOR6 = V4L2_E_PQ_IBC_COLOR_Y,
	V4L2_E_PQ_IBC_USER_COLOR7 = V4L2_E_PQ_IBC_COLOR_F,
	V4L2_E_PQ_IBC_USER_COLOR8,
	V4L2_E_PQ_IBC_USER_COLOR9,
	V4L2_E_PQ_IBC_USER_COLOR10,
	V4L2_E_PQ_IBC_USER_COLOR11,
	V4L2_E_PQ_IBC_USER_COLOR12,
	V4L2_E_PQ_IBC_USER_COLOR13,
	V4L2_E_PQ_IBC_USER_COLOR14,
	V4L2_E_PQ_IBC_USER_COLOR15,
	V4L2_E_PQ_IBC_USER_COLOR0,
	V4L2_E_PQ_IBC_COLOR_MAX,
};

struct v4l2_pq_color_tuner_info {
	enum v4l2_pq_color_tuner_event ColorTunerEvent;
	enum v4l2_pq_ihc_type IHCType;
	enum v4l2_pq_icc_type ICCType;
	enum v4l2_pq_ibc_type IBCType;
	__u8 Val;
} __attribute__((__packed__));

enum v4l2_pq_hsy_event {
	V4L2_E_PQ_HSY_SET,
	V4L2_E_PQ_HSY_GET,
	V4L2_E_PQ_HSY_MAX,
};

enum v4l2_pq_hsy_index {
	V4L2_E_PQ_HSY_HUE,
	V4L2_E_PQ_HSY_SAT,
	V4L2_E_PQ_HSY_LUMA,
	V4L2_E_PQ_HSY_NUM,
};

struct v4l2_pq_hsy_info {
	enum v4l2_pq_hsy_event HSYEvent;
	__u32  Version;
	enum v4l2_pq_hsy_index HSYIdx;
	__s32  Degresslut[V4L2_HSY_CFD_MAX_HUE_INDEX];
} __attribute__((packed));

struct v4l2_pq_luma_curve_info {
	__u32    Version;
	bool     Enable;
	__u16    PairNum;
	__u32   *pIndexLut;
#if !defined(__aarch64__)
	void    *pDummy0;
#endif
	__u32   *pOutputLut;
#if !defined(__aarch64__)
	void    *pDummy1;
#endif
	__u16    ManualBlendFactor;
} __attribute__((__packed__));

struct v4l2_pq_stretch_info {
	__u32 Version;
	__u8  u0p8BlackStretchRatio;
	__u8  u0p8WhiteStretchRatio;
	__u16 u4p12BlackStretchGain;
	__u16 u4p12WhiteStretchGain;
} __attribute__((__packed__));

enum v4l2_pq_cmd_event {
	V4L2_E_PQ_CMD_SET,
	V4L2_E_PQ_CMD_GET,
	V4L2_E_PQ_CDM_MAX,
};

struct v4l2_pq_cmd_info {
	enum v4l2_pq_cmd_event CmdEvent;
	__u32 Version;
	__u32 CmdID;
	__u16 Data;
} __attribute__((__packed__));

struct v4l2_pq_color_temp_info {
	__u16 cRedOffset;
	__u16 cGreenOffset;
	__u16 cBlueOffset;
	__u16 cRedColor;
	__u16 cGreenColor;
	__u16 cBlueColor;
	__u16 cRedScaleValue;
	__u16 cGreenScaleValue;
	__u16 cBlueScaleValue;
} __attribute__((__packed__));

enum v4l2_pq_mwe_mode {
	V4L2_PQ_MWE_MODE_OFF,
	V4L2_PQ_MWE_MODE_H_SPLIT,
	V4L2_PQ_MWE_MODE_MOVE,
	V4L2_PQ_MWE_MODE_ZOOM,
	V4L2_PQ_MWE_MODE_H_SCAN,
	V4L2_PQ_MWE_MODE_H_SPLIT_LEFT,
	V4L2_PQ_MWE_MODE_MAX,
};

enum v4l2_pq_mwe_event {
	V4L2_PQ_MWE_SET_MODE,
	V4L2_PQ_MWE_SET_ON_OFF,
	V4L2_PQ_MWE_SET_MAX,
};

struct v4l2_pq_mwe_info {
	enum v4l2_pq_mwe_event MWEEvent;
	enum v4l2_pq_mwe_mode MWEMode;
	bool bMWEEnable;
} __attribute__((__packed__));

enum v4l2_pq_weave_type {
	V4L2_E_PQ_WEAVETYPE_NONE        = 0x00,
	V4L2_E_PQ_WEAVETYPE_H           = 0x01,
	V4L2_E_PQ_WEAVETYPE_V           = 0x02,
	V4L2_E_PQ_WEAVETYPE_DUALVIEW    = 0x04,
	V4L2_E_PQ_WEAVETYPE_MAX,
};

enum v4l2_pq_swdr_event {
	V4L2_E_PQ_SWDR_SET,
	V4L2_E_PQ_SWDR_GET,
	V4L2_E_PQ_SWDR_MAX,
};

struct v4l2_pq_swdr_info {
	enum v4l2_pq_swdr_event SWDREvent;
	__u32  Version;
	bool   DRE_En;
	bool   DRE_SWDR_En;
	__u8   DRE_DR_Set_BasicStrength;
	__u8   DRE_SWDR_Set_BasicStrength;
	__u16  DRE_SWDR_Set_StrengthDk;
	__u16  DRE_SWDR_Set_StrengthBr;
	__u8   DRE_SWDR_Set_SceneProtectLevel;
	__u8   DRE_Set_TemporalFilterLevel;
	__u8   DRE_Set_ColorCorrectLevel;
	__u16 *pData;
#if !defined(__aarch64__)
	void   *pDummy;
#endif
	__u16  NumofData;
	__u16  DataIndex;
} __attribute__((__packed__));

enum v4l2_pq_hist_type {
	V4L2_E_PQ_HIST_TYPE_PMode                 = 0x0,
	V4L2_E_PQ_HIST_TYPE_IFrameMode            = 0x1,
	V4L2_E_PQ_HIST_TYPE_I1stFieldMode         = 0x2,
	V4L2_E_PQ_HIST_TYPE_I2ndFieldMode         = 0x3,
	V4L2_E_PQ_HIST_TYPE_NULL                  = 0xFF,
};

struct v4l2_pq_rgb_pattern_info {
	__u32  Version;
	bool   Enable;
	__u16  u0p16R;
	__u16  u0p16G;
	__u16  u0p16B;
} __attribute__((__packed__));

struct v4l2_pq_blue_stretch {
	__u32 Version;
	__u8  Strength;
	__u8  SatLevel;
	bool  Enable;
} __attribute__((__packed__));

struct v4l2_pq_h_nonlinear_scaling_info {
	__u32 Version;
	__u8  HNL_En;
	__u32 HNL_Target_Ratio_h;
	__u32 HNL_Target_Ratio_h_start;
	__u16 HNL_Width0;
	__u16 HNL_Width1;
	__u8  ParamFromVREn;
} __attribute__((__packed__));

enum v4l2_pq_mcdi_type {
	V4L2_PQ_MCDI_TYPE_ME1,
	V4L2_PQ_MCDI_TYPE_ME2,
	V4L2_PQ_MCDI_TYPE_BOTH,
	V4L2_PQ_MCDI_TYPE_MAX,
};

struct v4l2_pq_mcdi_info {
	enum v4l2_pq_mcdi_type MCDIType;
	bool bMCDIEnable;
} __attribute__((__packed__));

enum V4L2_PQ_DELAY_TIME_TYPE {
	V4L2_E_PQ_DELAY_DISPLAYWITHPQ   = 0x00,
	V4L2_E_PQ_DELAY_FIRSTFRAMEOUT  = 0x01,
	V4L2_E_PQ_DELAY_FIRSTFRAMEOUT_AVG = 0x02,
	V4L2_E_PQ_DELAY_MAX,
};

struct v4l2_pq_video_delaytime_info {
	enum V4L2_PQ_DELAY_TIME_TYPE DelayType;
	__u16 delaytime;
} __attribute__((__packed__));

struct v4l2_pq_cap_info {
	bool bPIP_Supported;
	bool b3DVideo_Supported;
	bool b4K2KPIP_Supported;
	bool bPQBin_Enable;
	bool bUFSCDLC_Enable;
	bool bPQBinHSY_Enable;
	bool bPQBinTMO_Enable;
} __attribute__((__packed__));

enum v4l2_pq_get_bin_version_event {
	V4L2_E_PQ_GET_BIN_VERSION,
	V4L2_E_PQ_GET_BIN_VERSION_BY_TYPE,
	V4L2_E_PQ_GET_BIN_MAX,
};

enum v4l2_pq_get_bin_version_event_type {
	V4L2_E_PQ_GET_BIN_TYPE_STD,
	V4L2_E_PQ_GET_BIN_TYPE_EXT,
	V4L2_E_PQ_GET_BIN_TYPE_CUSTOMER,
	V4L2_E_PQ_GET_BIN_TYPE_UFSC,
	V4L2_E_PQ_GET_BIN_TYPE_TMO,
	V4L2_E_PQ_GET_BIN_TYPE_HSY,
	V4L2_E_PQ_GET_BIN_TYPE_MAX,
};

struct v4l2_pq_get_bin_version {
	enum v4l2_pq_get_bin_version_event BinEvent;
	__u8 version[V4L2_PQ_BIN_VERSION_LEN];
	enum v4l2_pq_get_bin_version_event_type BINType;
} __attribute__((__packed__));

enum v4l2_pq_init_event {
	V4L2_PQ_INIT_PQ,
	V4L2_PQ_INIT_BIN_PATH,
	V4L2_PQ_INIT_BW_TABLE,
	V4L2_PQ_INIT_ACE,
	V4L2_PQ_INIT_MAX,
};

struct v4l2_pq_bin_info {
	/// ID
	__u8  PQID;
	/// Physical address
	__u32 PQBin_PhyAddr;//FIXME
} __attribute__((__packed__));

struct v4l2_pq_init_pq {
	__u16      OSD_hsize;
	__u8       PQBinCnt;
	__u8       PQTextBinCnt;
	__u8       PQBinCustomerCnt;
	__u8       PQBinUFSCCnt;
	struct v4l2_pq_bin_info PQBinInfo[V4L2_E_PQ_GET_BIN_TYPE_MAX];
	struct v4l2_pq_bin_info PQTextBinInfo;
	__u8       PQBinCFCnt;
	__u8       PQBinTMOCnt;
} __attribute__((packed));

enum v4l2_pq_bin_path {
	V4L2_E_PQ_BIN_PATH_CUSTOMER,
	V4L2_E_PQ_BIN_PATH_DEFAULT,
	V4L2_E_PQ_BIN_PATH_INI,
	V4L2_E_PQ_BIN_PATH_BANDWIDTH,
	V4L2_E_PQ_BIN_PATH_MAX,
};

struct v4l2_pq_binfile_path {
	enum v4l2_pq_bin_path PqBinPath;
	__u8      size;
	char      pPQBinFilePath[V4L2_PQ_FILE_PATH_LENGTH];
} __attribute__((__packed__));

struct v4l2_pq_init_ace_info {
	__s16  ColorCorrectionMatrix[32];
	__s16  RGB[3][3];
	__u16  MWEHstart;
	__u16  MWEVstart;
	__u16  MWEWidth;
	__u16  MWEHeight;
	__u16  MWE_Disp_Hstart;
	__u16  MWE_Disp_Vstart;
	__u16  MWE_Disp_Width;
	__u16  MWE_Disp_Height;
	bool   MWE_Enable;
} __attribute__((__packed__));

struct v4l2_pq_init_info {
	enum v4l2_pq_init_event InitEvent;
	union{
		struct v4l2_pq_init_pq           init_pq;
		struct v4l2_pq_binfile_path      binfile_path;
		struct v4l2_pq_init_ace_info     init_ace_info;
	} init;
} __attribute__((packed));

struct v4l2_pq_mode_info {
	/// is FBL or not
	bool   bFBL;
	/// is interlace mode or not
	bool   bInterlace;
	/// input Horizontal size
	__u16  input_hsize;
	/// input Vertical size
	__u16  input_vsize;
	/// input Vertical total
	__u16  input_vtotal;
	/// input Vertical frequency
	__u16  input_vfreq;
	/// output Vertical frequency
	__u16  output_vfreq;
	/// Display Horizontal size
	__u16  display_hsize;
	/// Display Vertical size
	__u16  display_vsize;
	/// Crop Horizontal size
	__u16  cropwin_hsize;
	/// Crop Vertical size
	__u16  cropwin_vsize;
} __attribute__((__packed__));

struct v4l2_pq_fbl_status {
	bool bFblEnable;
	bool bDsEnable;
	struct v4l2_pq_mode_info PQModeInfo;
} __attribute__((__packed__));

enum v4l2_pq_histogram_type {
	V4L2_PQ_E_HISTOGRAM_HUE_IP,
	V4L2_PQ_E_HISTOGRAM_SATURATION_IP,
	V4L2_PQ_E_HISTOGRAM_LUMA_IP,
	V4L2_PQ_E_HISTOGRAM_RGBMAX_IP,
	V4L2_PQ_E_HISTOGRAM_MAX,
};

struct v4l2_pq_ip_histogram_info {
	__u32   Version;
	enum v4l2_pq_histogram_type HistogramType;
	__u32   HistogramBinNum;
	__u32   *HistogramValue;
#if !defined(__aarch64__)
	void    *pDummy0;
#endif
} __attribute__((__packed__));

struct v4l2_pq_hsy_range_info {
	__u32 Version;
	enum v4l2_pq_hsy_index HSYIdx;
	__s32 MinValue;
	__s32 MaxValue;
	__u8 NumofRegion;
} __attribute__((__packed__));

struct v4l2_pq_luma_info {
	__u8  IpMaximumPixel;
	__u8  IpMinimumPixel;
	__s32 Yavg;
} __attribute__((__packed__));

enum v4l2_pq_chroma_info_control {
	V4L2_E_PQ_CHROMA_SET_WINDOW = 0,
	V4L2_E_PQ_CHROMA_GET_HIST,
	V4L2_E_PQ_CHROMA_MAX,
};

struct v4l2_pq_chroma_info {
	__u32 Version;
	enum v4l2_pq_chroma_info_control ChormaInfoCtl;
	__u8  SatbyHue_HueStart_Window0;
	__u8  SatbyHue_HueRange_Window0;
	__u8  SatbyHue_HueStart_Window1;
	__u8  SatbyHue_HueRange_Window1;
	__u8  SatbyHue_HueStart_Window2;
	__u8  SatbyHue_HueRange_Window2;
	__u8  SatbyHue_HueStart_Window3;
	__u8  SatbyHue_HueRange_Window3;
	__u8  SatbyHue_HueStart_Window4;
	__u8  SatbyHue_HueRange_Window4;
	__u8  SatbyHue_HueStart_Window5;
	__u8  SatbyHue_HueRange_Window5;
	bool  HistValid;
	__u32 *SatOfHueHist0;
#if !defined(__aarch64__)
	void    *pDummy0;
#endif
	__u32 *SatOfHueHist1;
#if !defined(__aarch64__)
	void    *pDummy1;
#endif
	__u32 *SatOfHueHist2;
#if !defined(__aarch64__)
	void    *pDummy2;
#endif
	__u32 *SatOfHueHist3;
#if !defined(__aarch64__)
	void    *pDummy3;
#endif
	__u32 *SatOfHueHist4;
#if !defined(__aarch64__)
	void    *pDummy4;
#endif
	__u32 *SatOfHueHist5;
#if !defined(__aarch64__)
	void    *pDummy5;
#endif
	__u8  SatOfHueHistSize[V4L2_HISTOGRAM_SATOFHUE_WIN_NUM];
} __attribute__((__packed__));

enum v4l2_pq_film_mode_type {
	V4L2_E_PQ_FilmMode_MIN,
	V4L2_E_PQ_FilmMode_OFF = V4L2_E_PQ_FilmMode_MIN,
	V4L2_E_PQ_FilmMode_ON,
	V4L2_E_PQ_FilmMode_DEFAULT,
	V4L2_E_PQ_FilmMode_NUM,
};

enum v4l2_pq_3d_nr_type {
	V4L2_E_PQ_3D_NR_MIN,
	V4L2_E_PQ_3D_NR_OFF = V4L2_E_PQ_3D_NR_MIN,
	V4L2_E_PQ_3D_NR_LOW,
	V4L2_E_PQ_3D_NR_MID,
	V4L2_E_PQ_3D_NR_HIGH,
	V4L2_E_PQ_3D_NR_AUTO,
	V4L2_E_PQ_3D_NR_AUTO_LOW_L,
	V4L2_E_PQ_3D_NR_AUTO_LOW_M,
	V4L2_E_PQ_3D_NR_AUTO_LOW_H,
	V4L2_E_PQ_3D_NR_AUTO_MID_L,
	V4L2_E_PQ_3D_NR_AUTO_MID_M,
	V4L2_E_PQ_3D_NR_AUTO_MID_H,
	V4L2_E_PQ_3D_NR_AUTO_HIGH_L,
	V4L2_E_PQ_3D_NR_AUTO_HIGH_M,
	V4L2_E_PQ_3D_NR_AUTO_HIGH_H,
	V4L2_E_PQ_3D_NR_DEFAULT,
	V4L2_E_PQ_3D_NR_NUM,
};

enum v4l2_pq_mpeg_nr_type {
	V4L2_E_PQ_MPEG_NR_MIN,
	V4L2_E_PQ_MPEG_NR_OFF = V4L2_E_PQ_MPEG_NR_MIN,
	V4L2_E_PQ_MPEG_NR_LOW,
	V4L2_E_PQ_MPEG_NR_MID,
	V4L2_E_PQ_MPEG_NR_HIGH,
	V4L2_E_PQ_MPEG_NR_AUTO,
	V4L2_E_PQ_MPEG_NR_DEFAULT,
	V4L2_E_PQ_MPEG_NR_NUM,
};

enum v4l2_pq_xvycc_type {
	V4L2_E_PQ_XVYCC_NORMAL,
	V4L2_E_PQ_XVYCC_ON_XVYCC,
	V4L2_E_PQ_XVYCC_ON_SRGB,
	V4L2_E_PQ_XVYCC_NUM,
};

enum v4l2_pq_dlc_level {
	V4L2_E_PQ_DLC_DEFAULT,
	V4L2_E_PQ_DLC_LOW,
	V4L2_E_PQ_DLC_MID,
	V4L2_E_PQ_DLC_HIGH,
	V4L2_E_PQ_DLC_MAX,
};

enum v4l2_pq_swdr_level {
	V4L2_E_PQ_SWDR_OFF,
	V4L2_E_PQ_SWDR_LOW,
	V4L2_E_PQ_SWDR_MID,
	V4L2_E_PQ_SWDR_HIGH,
	V4L2_E_PQ_SWDR_DLC_OFF,
};

enum v4l2_pq_dynamic_contrast_level {
	V4L2_E_PQ_DynContr_MIN,
	V4L2_E_PQ_DynContr_OFF = V4L2_E_PQ_DynContr_MIN,
	V4L2_E_PQ_DynContr_ON,
	V4L2_E_PQ_DynContr_DEFAULT,
	V4L2_E_PQ_DynContr_NUM,
};

enum v4l2_pq_gamma_event {
	V4L2_PQ_SET_GAMMA_ON_OFF,
	V4L2_PQ_SET_GAMMA_TABLE,
	V4L2_PQ_SET_GAMMA_CURVE,
	V4L2_PQ_SET_GAMMA_MAX,
};

enum v4l2_pq_buffer_attribute {
	V4L2_PQ_BUF_ATTRI_CONTINUOUS = 0,
	V4L2_PQ_BUF_ATTRI_DISCRETE,
	V4L2_PQ_BUF_ATTRI_ATTRIBUTE_MAX,
};

struct v4l2_pq_gain_offset {
	__u16 gain;
	__u16 offset;
} __attribute__((packed));

struct v4l2_pq_gamma_in {
	__u32 Version;
	bool bInit;
	struct v4l2_pq_gain_offset GainOff1[V4L2_PQ_GAMMA_CHANNEL];
	struct v4l2_pq_gain_offset GainOff2[V4L2_PQ_GAMMA_CHANNEL];
	bool One_Drv3;
	bool Enable;
	__u32 u20p12MaxBase[V4L2_PQ_GAMMA_CHANNEL];
	__u32 u20p12Lut[V4L2_NUM_PQ_GAMMA_LUT_ENTRY*V4L2_PQ_GAMMA_CHANNEL];
} __attribute__((packed));

struct v4l2_pq_gamma_info {
	enum v4l2_pq_gamma_event GammaEvent;
	bool EnableGamma;
	bool LoadGammaTable;
	struct v4l2_pq_gamma_in GammaIn;
} __attribute__((packed));

struct v4l2_dlc_mf_init_info_ex {
	__u32 Version;
	__u8 ucLumaCurve[16];
	__u8 ucLumaCurve2_a[16];
	__u8 ucLumaCurve2_b[16];
	__u8 ucLumaCurve2[16];
	__u8 L_L_U;
	__u8 L_L_D;
	__u8 L_H_U;
	__u8 L_H_D;
	__u8 S_L_U;
	__u8 S_L_D;
	__u8 S_H_U;
	__u8 S_H_D;
	__u8 ucCGCCGain_offset;
	__u8 ucCGCChroma_GainLimitH;
	__u8 ucCGCChroma_GainLimitL;
	__u8 ucCGCYCslope;
	__u8 ucCGCYth;
	__u8 ucDlcPureImageMode;
	__u8 ucDlcLevelLimit;
	__u8 ucDlcAvgDelta;
	__u8 ucDlcAvgDeltaStill;
	__u8 ucDlcFastAlphaBlending;
	__u8 ucDlcSlowEvent;
	__u8 ucDlcTimeOut;
	__u8 ucDlcFlickAlphaStart;
	__u8 ucDlcYAvgThresholdH;
	__u8 ucDlcYAvgThresholdL;
	__u8 ucDlcBLEPoint;
	__u8 ucDlcWLEPoint;
	__u8 bCGCCGainCtrl : 1;
	__u8 bEnableBLE : 1;
	__u8 bEnableWLE : 1;
	__u8 ucDlcYAvgThresholdM;
	__u8 ucDlcCurveMode;
	__u8 ucDlcCurveModeMixAlpha;
	__u8 ucDlcAlgorithmMode;
	__u8 DlcHistogramLimitCurve[V4L2_DLC_HISTOGRAM_LIMIT_CURVE_ARRARY_NUM];
	__u8 ucDlcSepPointH;
	__u8 ucDlcSepPointL;
	__u16 uwDlcBleStartPointTH;
	__u16 uwDlcBleEndPointTH;
	__u8 ucDlcCurveDiff_L_TH;
	__u8 ucDlcCurveDiff_H_TH;
	__u16 uwDlcBLESlopPoint_1;
	__u16 uwDlcBLESlopPoint_2;
	__u16 uwDlcBLESlopPoint_3;
	__u16 uwDlcBLESlopPoint_4;
	__u16 uwDlcBLESlopPoint_5;
	__u16 uwDlcDark_BLE_Slop_Min;
	__u8 ucDlcCurveDiffCoringTH;
	__u8 ucDlcAlphaBlendingMin;
	__u8 ucDlcAlphaBlendingMax;
	__u8 ucDlcFlicker_alpha;
	__u8 ucDlcYAVG_L_TH;
	__u8 ucDlcYAVG_H_TH;
	__u8 ucDlcDiffBase_L;
	__u8 ucDlcDiffBase_M;
	__u8 ucDlcDiffBase_H;
	__u8 LMaxThreshold;
	__u8 LMinThreshold;
	__u8 LMaxCorrection;
	__u8 LMinCorrection;
	__u8 RMaxThreshold;
	__u8 RMinThreshold;
	__u8 RMaxCorrection;
	__u8 RMinCorrection;
	__u8 AllowLoseContrast;
} __attribute__((packed));

struct v4l2_pq_dlc_init_info {
	struct v4l2_dlc_mf_init_info_ex MFInitInfoEx;
	__u16 CurveHStart;
	__u16 CurveHEnd;
	__u16 CurveVStart;
	__u16 CurveVEnd;
} __attribute__((packed));

struct v4l2_pq_dlc_capture_range_info {
	__u16 wHStart;
	__u16 wHEnd;
	__u16 wVStart;
	__u16 wVEnd;
} __attribute__((packed));

//DISPLAY
struct mtk_pq_zorder_info {
	__u32 u32Version;      /// mtk_pq_zorder_info version
	__u32 u32Length;       /// sizeof(mtk_pq_zorder_info)

	__u32 u32Zorder;
} __attribute__((__packed__));

struct mtk_pq_3d_info {
	__u32 u32Version;      /// mtk_pq_3d_info version
	__u32 u32Length;       /// sizeof(mtk_pq_3d_info)

	enum mtk_pq_3d_input_mode en3DInputMode;
	enum mtk_pq_3d_output_mode en3DOutputMode;
	enum mtk_pq_3d_panel_type en3DPanelType;
} __attribute__((__packed__));

struct mtk_pq_hdr_info {
	__u32 u32Version;      /// mtk_pq_hdr_info version
	__u32 u32Length;       /// sizeof(mtk_pq_hdr_info)

	enum mtk_pq_hdr_type enHDRType;
} __attribute__((__packed__));


struct mtk_pq_set_capt_info {
	__u32 u32Version;      /// mtk_pq_set_capt_info version
	__u32 u32Length;       /// sizeof(mtk_pq_set_capt_info)

	__u64 phyAddr;
	__u64 phySize;

	__u32 u32Enable;
	__u32 u32Visible;
	__u32 u32FrameRate;
	///< If desired frameRate is 30fps,
	//u32FrameRate value should be set 30000
	__u32 u32Width;
	__u32 u32Height;
	enum mtk_pq_capt_color_format enColorFormat;
	__u32 u32BufferLessMode;
	///< Type 0: with ring buffer, Others:
	//Enable IMI capture mode to save dram cost
	enum mtk_pq_capt_type enCaptureType;
	enum mtk_pq_capt_aspect_ratio enCaptureDARC;
} __attribute__((__packed__));

struct mtk_pq_window {
	__u32 u32x;
	__u32 u32y;
	__u32 u32width;
	__u32 u32height;
} __attribute__((__packed__));

struct mtk_pq_frame_release_info {
	__u32 u32Version;      /// mtk_pq_frame_release_info version
	__u32 u32Length;       /// sizeof(mtk_pq_frame_release_info)

	__u32 au32Idx[2];
	__u32 au32PriData[2];
	__u32 u32VdecStreamVersion;
	__u32 u32VdecStreamId;
	__u32 u32WindowID;
	__u32 u32FrameNum;
	__u64 u64Pts;   /// pts info(default usage), unit depends on caller
	__u8 u8ApplicationType;
	__u32 u32DisplayCounts;
	__u64 u64PtsUs;        /// use this pts when your unit is us
} __attribute__((__packed__));

struct mtk_pq_delay_info {
	__u32 u32Version;
	/// mtk_pq_delay_info version
	__u32 u32Length;
	/// sizeof(mtk_pq_delay_info)

	__u32 u32DelayTime;
	__u32 u32WindowID;
	enum mtk_pq_get_delay_type enDelayType;
	__u64 u64Pts;
	/// pts info(default usage), unit depends on caller
	__s64 s64SystemTime;
	void *pParam;
	bool bSubtitleStamp;
	__u32 u32DisplayCnt;
	/// report the times of current display frame counts
	__u64 u64PtsUs;
	/// use this pts when your unit is us
} __attribute__((__packed__));

typedef unsigned char (*func_mtk_pq_video_delay) (struct mtk_pq_delay_info);

struct mtk_pq_callback_info {
	__u32 u32Version;      /// mtk_pq_callback_info version
	__u32 u32Length;       /// sizeof(mtk_pq_callback_info)

	/// Function Pointer
	func_mtk_pq_video_delay pfnFunVideoDelay;
	void *pParam;
} __attribute__((__packed__));

struct mtk_pq_set_win_info {
	__u32 u32Version;      /// mtk_pq_set_win_info version
	__u32 u32Length;       /// sizeof(mtk_pq_set_win_info)

	// window aspect ratio
	enum mtk_pq_aspect_ratio enARC;

	//-------------
	// Input
	//-------------
	__u32 u32InputSourceType;      ///<Input source

	//-------------
	// Window
	//-------------
	struct mtk_pq_window stCapWin;        ///<Capture window
	struct mtk_pq_window stDispWin;       ///<Display window
	struct mtk_pq_window stCropWin;       ///<Crop window

	//-------------
	// Timing
	//-------------
	__u32  u32Interlace;             ///<Interlaced or Progressive
	__u32  u32HDuplicate;
	///<flag for vop horizontal duplicate,
	//for MVD, YPbPr, indicate input double sampled or not
	__u32  u32InputVFreq;
	///<Input V Frequency, VFreqx10, for calculate output panel timing
	__u32  u32InputVTotal;
	///<Input Vertical total, for calculate output panel timing
	__u32  u32DefaultHtotal;
	///<Default Htotal for VGA/YPbPr input
	__u32  u32DefaultPhase;
	///<Default Phase for VGA/YPbPr input

	//-------------------------
	// customized post scaling
	//-------------------------
	__u32  u32HCusScaling;
	///<assign post H customized scaling instead of using XC scaling
	__u32  u32HCusScalingSrc;
	///<post H customized scaling src width
	__u32  u32HCusScalingDst;
	///<post H customized scaling dst width
	__u32  u32VCusScaling;
	///<assign post V manuel scaling instead of using XC scaling
	__u32  u32VCusScalingSrc;
	///<post V customized scaling src height
	__u32  u32VCusScalingDst;
	///<post V customized scaling dst height

	//--------------
	// 9 lattice
	//--------------
	__u32  u32DisplayNineLattice;
	///<used to indicate where to display in panel and where to put in
	//frame buffer

	//-------------------------
	// customized pre scaling
	//-------------------------
	__u32  u32PreHCusScaling;
	///<assign pre H customized scaling instead of using XC scaling
	__u32  u32PreHCusScalingSrc;
	///<pre H customized scaling src width
	__u32  u32PreHCusScalingDst;
	///<pre H customized scaling dst width
	__u32  u32PreVCusScaling;
	///<assign pre V manuel scaling instead of using XC scaling
	__u32  u32PreVCusScalingSrc;
	///<pre V customized scaling src height
	__u32  u32PreVCusScalingDst;
	///<pre V customized scaling dst height

	//-------------------------
	// the output window info
	//-------------------------
	__u32 u32OnOutputLayer;
	/// enable output layer to panel timing
	struct mtk_pq_window stOutputWin;
	/// the output window
	bool bIsResChange;
	/// resolution change flag
	bool bSetWinImmediately;
	bool bForceSetWin;
	///it need to re-run XC set window for reset DS case
} __attribute__((__packed__));

struct mtk_pq_hw_format {
	__u32 u32Version;          /// struct mtk_pq_hw_format version
	__u32 u32Length;           /// sizeof(struct mtk_pq_hw_format)
	__u64 phyLumaAddr;
	__u64 phyChromaAddr;
	__u64 phyLumaAddr2Bit;
	__u64 phyChromaAddr2Bit;
	__u32 u32LumaPitch;
	__u32 u32ChromaPitch;
	__u32 u32LumaPitch2Bit;
	__u32 u32ChromaPitch2Bit;
	__u32 u32MFCodecInfo;
	__u64 phyMFCBITLEN;
	__u8  u8V7DataValid;
	__u16 u16Width_subsample;
	__u16 u16Height_subsample;
	__u64 phyLumaAddr_subsample;
	__u64 phyChromaAddr_subsample;
	__u16 u16Pitch_subsample;
	__u8  u8TileMode_subsample;
	__u64 u32HTLBTableAddr;
	__u8  u8HTLBPageSizes;
	__u8  u8HTLBChromaEntriesSize;
	__u64 u32HTLBChromaEntriesAddr;
	__u16 u16MaxContentLightLevel;
	__u16 u16MaxPicAverageLightLevel;
	__u64 u64NumUnitsInTick;
	__u64 u64TimeScale;
	__u16 u16Histogram[V4L2_MTK_HISTOGRAM_INDEX];
	__u32 u32QPMin; // Min of quantization parameter
	__u32 u32QPAvg; // Avg of quantization parameter
	__u32 u32QPMax; // Max of quantization parameter
} __attribute__((__packed__));

struct mtk_pq_frame_format {
	__u32 u32Version; /// struct mtk_pq_frame_format version
	__u32 u32Length;  /// sizeof(struct mtk_pq_frame_format)
	enum mtk_pq_frame_type enFrameType;
	enum mtk_pq_field_type enFieldType;
	enum mtk_pq_view_type enViewType;
	__u32 u32X;
	__u32 u32Y;
	__u32 u32Width;
	__u32 u32Height;
	__u32 u32CropLeft;
	__u32 u32CropRight;
	__u32 u32CropTop;
	__u32 u32CropBottom;
	struct mtk_pq_hw_format stHWFormat;
	__u32 u32Idx;
	__u32 u32PriData;
	__u8 u8LumaBitdepth;
	__u8 u8ChromaBitdepth;
} __attribute__((__packed__));

struct mtk_pq_color_description {
	__u32 u32Version;          /// mtk_pq_color_description version
	__u32 u32Length;           /// sizeof(mtk_pq_color_description)
	//color_description: indicates the
	//chromaticity/opto-electronic coordinates of the source primaries
	__u8 u8ColorPrimaries;
	__u8 u8TransferCharacteristics;
	// matrix coefficients in deriving YUV signal from RGB
	__u8 u8MatrixCoefficients;
} __attribute__((__packed__));

struct mtk_pq_master_color_display {
	__u32 u32Version;       /// struct mtk_pq_master_color_display version
	__u32 u32Length;        /// sizeof(struct mtk_pq_master_color_display)
	//mastering color display: color volumne of a display
	__u32 u32MaxLuminance;
	__u32 u32MinLuminance;
	__u16 u16DisplayPrimaries[3][2];
	__u16 u16WhitePoint[2];
} __attribute__((__packed__));

struct mtk_pq_dolby_hdr_info {
	__u32 u32Version;       /// mtk_pq_dolby_hdr_info version
	__u32 u32Length;        /// sizeof(mtk_pq_dolby_hdr_info)
	// bit[0:1] 0: Disable 1:Single layer 2: Dual layer,
	//bit[2] 0:Base Layer 1:Enhance Layer
	__u8  u8DVMode;
	__u64 phyHDRMetadataAddr;
	__u32 u32HDRMetadataSize;
	__u64 phyHDRRegAddr;
	__u32 u32HDRRegSize;
	__u64 phyHDRLutAddr;
	__u32 u32HDRLutSize;
	__u8  u8DMEnable;
	__u8  u8CompEnable;
	__u8  u8CurrentIndex;
} __attribute__((__packed__));

struct mtk_pq_vdec_hdr_info {
	__u32 u32Version;      /// struct mtk_pq_vdec_hdr_info version
	__u32 u32Length;       /// sizeof(struct mtk_pq_vdec_hdr_info)

	// bit[0]: MS_ColorDescription present or valid, bit[1]:
	//MS_MasterColorDisplay present or valid
	__u32 u32FrmInfoExtAvail;
	// //color_description: indicates the chromaticity/opto-electronic
	//coordinates of the source primaries
	struct mtk_pq_color_description   stColorDescription;
	// mastering color display: color volumne of a display
	struct mtk_pq_master_color_display stMasterColorDisplay;
	struct mtk_pq_dolby_hdr_info stDolbyHDRInfo;
} __attribute__((__packed__));

struct mtk_pq_ext_frame_info {
	__u32 u32LumaAddrExt[MTK_PQ_DISP_FRM_INFO_EXT_TYPE_MAX];
	__u32 u32ChromaAddrExt[MTK_PQ_DISP_FRM_INFO_EXT_TYPE_MAX];
	__u16 u16Width;      // the width of second frame
	__u16 u16Height;     // the height of second frame
	__u16 u16Pitch[2];   // the pitch of second frame
} __attribute__((__packed__));

struct mtk_pq_fmt_modifier {
	bool tile;
	bool raster;
	bool compress;
	bool jump;
	enum mtk_pq_fmt_modifier_vsd vsd_mode;
	bool vsd_ce_mode;
} __attribute__((__packed__));

struct mtk_pq_vdec_offset {
	__u64 luma_0_offset;
	__u32 luma_0_size;
	__u64 luma_1_offset;
	__u32 luma_1_size;
	__u64 chroma_0_offset;
	__u32 chroma_0_size;
	__u64 chroma_1_offset;
	__u32 chroma_1_size;
	__u64 luma_blen_offset;
	__u32 luma_blen_size;
	__u64 chroma_blen_offset;
	__u32 chroma_blen_size;
	__u64 sub_luma_0_offset;    // for DoVi use
	__u32 sub_luma_0_size;      // for DoVi use
	__u64 sub_chroma_0_offset;  // for DoVi use
	__u32 sub_chroma_0_size;    // for DoVi use
} __attribute__((__packed__));

struct mtk_pq_frame_info {
	__u32 u32Version;             /* struct mtk_pq_frame_info version*/
	__u32 u32Length;              /* sizeof(struct mtk_pq_frame_info)*/
	__u32 u32OverlayID;           /*not use----sid*/
	struct mtk_pq_frame_format stFrames[2];
	struct mtk_pq_frame_format stSubFrame;  // for DoVi use
	enum mtk_pq_color_format color_format;
	enum mtk_pq_order_type order_type;
	enum mtk_pq_scan_type scan_type;
	enum mtk_pq_field_type field_type;
	enum mtk_pq_rotate_type rotate_type;
	__u32 u32FrameNum;
	__u64 u64Pts;
	__u32 u32CodecType;
	__u32 u32FrameRate;
	__u32 u32AspectWidth;
	__u32 u32AspectHeight;
	__u32 u32VdecStreamVersion;
	__u32 u32VdecStreamId;
	__u32 u32UniqueId;    /*not use----sid*/
	__u8 u8AspectRate;
	/*frame_info[u16CurrentWritePointer].*/
	/*u8AspectRate*/
	__u8 u8Interlace;
	__u8 u8FrcMode;//not use----sid
	__u8 u83DMode;//frame_info[u16CurrentWritePointer].u83DMode
	__u8 u8BottomFieldFirst;
	//frame_info[u16CurrentWritePointer].enFieldOrderType
	__u8 u8FreezeThisFrame;  //not use----sid
	__u8 u8ToggleTime;
	__u8 u8MCUMode;          //not use----sid
	__u8 u8FieldCtrl;
	// control one field mode, always top or bot when FF or FR
	__u8 u8ApplicationType;
	//release frame buffer only---sid
	__u8 u83DLayout;           //not use ----sid
	__u8 u8ColorInXVYCC;   //not use ----sid
	__u8 u8LowLatencyMode;     //not use----sid
	__u8 u8VdecComplexity;         //not use----sid
	__u8 u8HTLBTableId;                //not use----sid
	__u8 u8HTLBEntriesSize;           //not use ----sid
	__u8 u8AFD;                //not use----sid
	struct mtk_pq_vdec_hdr_info stHDRInfo;
	struct mtk_pq_ext_frame_info stDispFrmInfoExt; /*not use----sid*/
	__u16 u16MIUBandwidth;     /*not use----sid*/
	__u16 u16Bitrate;/*not use----sid*/
	__u32 u32TileMode;
	__u32 u32SubTileMode;   // for DoVi use
	__u64 phyHTLBEntriesAddr;/*not use----sid*/
	__u64 phyVsyncBridgeAddr;/*not use----sid*/
	__u64 phyVsyncBridgeExtAddr; /*not use----sid*/
	__u8 u8CustomerScantype; /*to set forcep(user only)----sid*/
	__u32 u32ForceFrameRate; /*force frame rate value, 0 is disable, */
	/*other is enable and use it.    Version 4*/
	__s32 s32CPLX;           /*vdec CPLX info   Version 5*/
	bool bAV1PpuBypassMode;  /*av1 vdec info     Version 6*/
	bool bMonoChrome;        /*av1 vdec info     Version 6*/
	bool bCheckFrameDisp;
	/*need to check the frame already display.           Version 7*/
	__u32 u32MaxDriftHeight;
	/*Set CSP streaming maximum floating height.         Version 8*/
	__u32 u32MaxDriftWidth;
	/*Set CSP streaming maximum floating width.          Version 8*/
	__u64 u64PtsUs;           //use this pts when your unit is us
	bool bFilmGrain;     //not use----sid
	struct mtk_pq_fmt_modifier modifier;
	struct mtk_pq_fmt_modifier submodifier; // for DoVi use
	struct mtk_pq_vdec_offset vdec_offset;
	struct v4l2_pq_buf buf;	//frame fd & metadata fd
} __attribute__((__packed__));

struct mtk_pq_window_info {
	__u32 u32Version;      // mtk_pq_window_info version
	__u32 u32Length;       // sizeof(mtk_pq_window_info)

	__u32 u32DeviceID;
	__u32 u32WinID;
	__u32 u32Layer;        // WIN_LAYER eLayer
	__u32 u32WinIsused;    // Is used

	struct mtk_pq_zorder_info stZorderInfo;
	struct mtk_pq_3d_info st3DInfo;
	struct mtk_pq_hdr_info stHDRInfo;
	struct mtk_pq_set_capt_info stSetCapInfo;
	struct mtk_pq_set_win_info stSetWinInfo;
	struct mtk_pq_frame_info pstDispFrameFormat;
	bool bIsNeedSeq;     /*the status for seq change info*/

	__u8 display_flow_type;
	__u32 display_delay_time;
	/*It's delay time the last buffer is shown.(ms)*/
	__u32 u32MvopFrameRate;
	__s64 s64MvopIsrTime;
	__u64 u64CurrentPTS;
	/* pts info(default usage), unit depends on caller*/
	__u64 u64CurrentPTSUs;
	/* use this pts when your unit is us*/
} __attribute__((__packed__));

struct mtk_pq_set_window {
	struct v4l2_rect stOutputWin;
	struct v4l2_rect stUserDispWin;
	struct v4l2_rect stCapWin;
	struct v4l2_rect stCropWin;
	enum mtk_pq_aspect_ratio enARC;
	bool bSetWinImmediately;
} __attribute__((__packed__));

struct mtk_pq_display_g_data {
	union {
		struct mtk_pq_window_info win_info;
		struct mtk_pq_frame_release_info stFrame_Release_Info;
		struct mtk_pq_delay_info stVideoDelayInfo;
		__u32 u32WindowID;
	} data;
} __attribute__((__packed__));

struct mtk_pq_display_s_data {
	enum mtk_pq_display_ctrl_type ctrl_type;
	union {
		__u64 addr;
		__u8 *ctrl_data;
	} p;
	__u32 param_size;
} __attribute__((__packed__));

//B2R


//XC
struct v4l2_ext_ctrl_misc_info {
	__u32 st_version;
	__u32 st_length;
	__u32 misc_a;       //reference enum v4l2_init_misc_a
	__u32 misc_b;       //reference enum v4l2_init_misc_b
	__u32 misc_c;       //reference enum v4l2_init_misc_c
	__u32 misc_d;       //reference enum v4l2_init_misc_d
};

struct v4l2_ext_ctrl_cap_win_info {
	__u32 st_version;
	__u32 st_length;
	struct v4l2_rect cap_win;
};

struct v4l2_ext_ctrl_crop_win_info {
	__u32 st_version;
	__u32 st_length;
	struct v4l2_rect crop_win;
	__u32 decimal_crop_scale;
	/*scale value of decimal crop win. ex:1000 means decimal places are 3*/
	struct v4l2_rect decimal_crop_win;
	/*decimal point with multiply decimal_crop_scale*/
};

struct v4l2_ext_ctrl_disp_win_info {
	__u32 st_version;
	__u32 st_length;
	struct v4l2_rect disp_win;
};

struct v4l2_ext_ctrl_scaling_info {
	__u32 st_version;
	__u32 st_length;
	bool enable_h_scaling;
	/*assign H customized scaling instead of using XC scaling*/
	__u16 h_scaling_src;
	/*H customized scaling src width*/
	__u16 h_scaling_dst;
	/*H customized scaling dst width*/
	bool enable_v_scaling;
	/*assign V manuel scaling instead of using XC scaling*/
	__u16 v_scaling_src;
	/*V customized scaling src height*/
	__u16 v_scaling_dst;
	/*V customized scaling dst height*/
};

struct v4l2_ext_ctrl_ds_info {
	__u32 st_version;
	__u32 st_length;
	__u32 mfcodec_info;
	__u8 current_index;
	bool update_ds_cmd;
	bool enable_dnr;
	bool enable_forcep;
	bool dynamic_scaling_enable;
	__u8 frame_count_pqds;
	enum v4l2_pq_ds_type pqds_type;
	bool update_pqds_cmd;
	bool is_need_set_window;
	enum v4l2_hdr_type hdr_type;
	__u32 xcds_update_condition;
	bool repeat_window;
	bool control_memc;
	bool force_memc_off;
	bool reset_memc;
	__u8 ds_xrule_frame_count[10];
};


struct v4l2_ext_ctrl_freeze_info {
	__u32 st_version;
	__u32 st_length;
	bool enable;
};

struct v4l2_ext_ctrl_fb_mode_info {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_fb_level fb_level;
};

struct v4l2_ext_ctrl_rwbankauto_enable_info {
	__u32 st_version;
	__u32 st_length;
	bool enable;
};

struct v4l2_testpattern_ip1_info {
	__u32 st_version;
	__u32 st_length;
	bool enable;
	__u32 pattern_type;
};

struct v4l2_testpattern_ip2_info {
	__u32 st_version;
	__u32 st_length;
	bool enable;
};

struct v4l2_testpattern_op_info {
	__u32 st_version;
	__u32 st_length;
	bool miu_line_buff;
	bool line_buff_hvsp;
};

struct v4l2_ext_ctrl_testpattern_enable_info {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_test_pattern_mode test_pattern_mode;
	union {
		struct v4l2_testpattern_ip1_info testpattern_ip1_info;
		struct v4l2_testpattern_ip2_info testpattern_ip2_info;
		struct v4l2_testpattern_op_info testpattern_op_info;
	};
};

struct v4l2_ext_ctrl_scaling_enable_info {
	__u32 st_version;
	__u32 st_length;
	bool enable;
	enum v4l2_scaling_type scaling_type;
	enum v4l2_vector_type vector_type;
};

struct v4l2_report_pixel_info {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_pixel_rgb_stage pixel_rgb_stage;
	__u16 rep_win_color;
	__u16 x_start;
	__u16 x_end;
	__u16 y_start;
	__u16 y_end;
	__u16 r_cr_min;
	__u16 r_cr_max;
	__u16 g_y_min;
	__u16 g_y_max;
	__u16 b_cb_min;
	__u16 b_cb_max;
	__u32 r_cr_sum;
	__u32 g_y_sum;
	__u32 b_cb_sum;
	bool show_rep_win;
};

struct v4l2_ext_ctrl_status_info {
	__u32 st_version;
	__u32 st_length;
	__u16 h_size_after_prescaling;
	__u16 v_size_after_prescaling;
	struct v4l2_rect scaled_crop_win;
	__u16 v_length;
	__u8 bit_per_pixel;
	enum v4l2_deinterlace_mode deinterlace_mode;
	bool linear_mode;
	bool fast_frame_lock;
	bool done_fpll;
	bool enable_fpll;
	bool fpll_lock;
	bool pq_set_hsd;
	__u16 default_phase;
	bool is_hw_depth_adj_supported;
	bool is_2line_mode;
	bool is_pnl_yuv_output;
	__u8 hdmi_pixel_repetition;
	bool fsc_enabled;
	bool frc_enabled;
	__u16 panel_interface_type;
	bool is_hsk_mode;
	bool enable_1pto2p_bypass;
	__u8 format;
	__u8 color_primaries;
	__u8 transfer_characterstics;
	__u8 matrix_coefficients;
	bool is_hfr_path_enable;
	__u16 lock_point_auto;
	bool is_120hz_output;
	struct v4l2_report_pixel_info report_pixel_info;
};

struct v4l2_dip_caps {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_dip_win window;
	__u32 dip_caps;
};

/* autodownload client supported caps */
struct v4l2_adl_caps {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_adl_client client;
	bool supported;
};

/* hdr supported caps */
struct v4l2_hdr_caps {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_hdr_type hdr_type;
	bool supported;
};

/* pqds supported caps */
struct v4l2_pqds_caps {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_pq_ds_type pqds_type;
	bool supported;
};

/* fast disp supported caps */
struct v4l2_fd_caps {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_fast_disp_type fd_type;
	bool supported;
};

struct v4l2_ext_ctrl_caps_info {
	__u32 st_version;
	__u32 st_length;
	enum v4l2_cap_type cap_type;
	union {
		bool support;
		struct v4l2_dip_caps dip_caps;
		__u32 u32;
		/* autodownload client supported caps */
		struct v4l2_adl_caps adl_caps;
		/* hdr supported caps */
		struct v4l2_hdr_caps hdr_caps;
		/* pqds supported caps */
		struct v4l2_pqds_caps pqds_caps;
		/* fast disp supported caps */
		struct v4l2_fd_caps fd_caps;
	};
};

struct v4l2_ext_ctrl_frame_num_factor_info {
	__u32 st_version;
	__u32 st_length;
	__u8 frame_num_factor;
};

struct v4l2_ext_ctrl_rw_bank_info {
	__u32 st_version;
	__u32 st_length;
	__u16 read_bank;
	__u16 write_bank;
};

//3D
struct v4l2_cus_try_3d_format {
	enum v4l2_ext_3d_input_mode e3DInputMode;
	enum v4l2_ext_3d_output_mode e3DOutputMode;
	bool bSupport3DFormat;
};

struct v4l2_ext_ctrl_3d_mode {
	enum v4l2_ext_3d_input_mode e3DInputMode;
	enum v4l2_ext_3d_output_mode e3DOutputMode;
};

/*COMMON*/
struct v4l2_atv_ext_info {
	bool b_vd_info_valid;
	struct v4l2_vd_ext_info ext_vd_info;
};

struct v4l2_dtv_ext_info {
	bool b_use_static_metadata;
	bool b_sei_valid;
	struct v4l2_sei_info sei_info;
	bool b_cll_valid;
	struct v4l2_cll_info cll_info;

	bool b_codec_type_valid;
	__u8 codec_type; //define as enum v4l2_ext_dtv_type
};

struct v4l2_dvi_ext_info {
	bool b_pc_mode_valid;
	bool b_pc_mode;
};

struct v4l2_vga_ext_info {

};

struct v4l2_comp_ext_info {

};

struct v4l2_av_ext_info {
	bool b_vd_info_valid;
	struct v4l2_vd_ext_info ext_vd_info;
};

struct v4l2_sv_ext_info {
	bool b_vd_info_valid;
	struct v4l2_vd_ext_info ext_vd_info;
};

struct v4l2_scart_ext_info {
	bool b_vd_info_valid;
	struct v4l2_vd_ext_info ext_vd_info;
};

struct v4l2_mm_ext_info {
	bool b_use_static_metadata;
	bool b_sei_valid;
	struct v4l2_sei_info sei_info;
	bool b_cll_valid;
	struct v4l2_cll_info cll_info;

	bool b_content_type_valid;
	__u8 content_type;  //define as enum v4l2_ext_mm_type
};

struct v4l2_hdmi_ext_info {
	bool b_hdr_info_frame_valid;
	__u8 EOTF;
	__u8 SMDID;
	struct v4l2_sei_info sei_info;
	struct v4l2_cll_info cll_info;

	bool b_pc_mode_valid;
	bool b_pc_mode;
	bool bit_depth_valid;
	__u8 bit_depth;
};

enum v4l2_pq_b2r_disp_type {
	V4L2_E_PQ_B2R_DISP_TYPE_NONE,
	V4L2_E_PQ_B2R_DISP_TYPE_TS,
	V4L2_E_PQ_B2R_DISP_TYPE_LGY,
};

struct v4l2_timing {
	__u32 h_total;
	__u32 v_total;
	__u32 h_freq;
	__u32 v_freq;
	__u32 de_x;
	__u32 de_y;
	__u32 de_width;
	__u32 de_height;
	enum v4l2_field interlance;
	enum v4l2_ext_sync_status sync_status;
	/*bitwise value, define as v4l2_ext_sync_status*/
	__u8 h_duplicate;
	enum v4l2_pq_b2r_disp_type disp_type;
};

struct v4l2_input_ext_info {
	__u32 type;    /*enum v4l2_ext_info_type*/
	union {
		struct v4l2_timing timing_ext_info;
		__u32 ThirdD_type; /*enum v4l2_ext_3d_input_mode*/
		struct v4l2_atv_ext_info atv_ext_info;
		struct v4l2_dtv_ext_info dtv_ext_info;
		struct v4l2_hdmi_ext_info hdmi_ext_info;
		struct v4l2_dvi_ext_info dvi_ext_info;
		struct v4l2_vga_ext_info vga_ext_info;
		struct v4l2_comp_ext_info comp_ext_info;
		struct v4l2_av_ext_info av_ext_info;
		struct v4l2_sv_ext_info sv_ext_info;
		struct v4l2_scart_ext_info scart_ext_info;
		struct v4l2_mm_ext_info mm_ext_info;
	} info;
};

struct v4l2_output_ext_info {
	__u32 type;      /* enum v4l2_ext_info_type */
	union {
		/*...*/
		__u32 ThirdD_type; /*define as v4l2_ext_3d_output_mode*/
		struct v4l2_color_out_ext_info color_out_ext_info;
	} info;
};

struct v4l2_pq_delay_info {
	__u32 length;
	enum v4l2_mfc_level mfc_level;
};

struct v4l2_pq_delaytime_info {
	enum v4l2_delay_time_type delay_type;
	struct v4l2_pq_delay_info delay_info;
	__u32 fps;
	__u16 delaytime;
};

struct v4l2_pqmap_info {
	__u32 fd;
	__u32 u32MainPimLen;
	__u32 u32MainRmLen;
	__u32 u32MainExPimLen;
	__u32 u32MainExRmLen;
} __attribute__((__packed__));

struct v4l2_pq_pqparam {
	__u32 win_id;
	__s32 fd;
};

struct v4l2_b2r_pattern {
	bool enable;
	__u16 ydata;
	__u16 cbdata;
	__u16 crdata;
};

struct v4l2_pq_gen_pattern {
	enum v4l2_ext_pq_pattern pat_type;
	__u32 length;
	union {
		struct v4l2_b2r_pattern b2r_pat_info;
	} pat_para;
};

struct v4l2_pq_stream_meta_info {
	__u32 meta_fd;
	__u32 type;//enum v4l2_buf_type
} __attribute__((__packed__));

struct v4l2_pq_flow_control {
	bool enable;
	__u16 factor;
};

struct v4l2_pq_aisr_active_win {
	bool bEnable;
	__u16 x;
	__u16 y;
	__u16 width;
	__u16 height;
};

struct v4l2_pq_try_queue {
	__u32 width;
	__u32 height;
};

struct v4l2_pq_s_buffer_info {
	__u32 type;
	__u32 width;
	__u32 height;
};

struct v4l2_pq_g_buffer_info {
	__u32 size[3];
	__u8 plane_num;
	__u8 buffer_num;
	enum v4l2_pq_buffer_attribute buf_attri;
};

struct v4l2_pq_stream_debug {
	bool cmdq_timeout;
};

struct v4l2_pq_metadata {
	__u32 fd;
	uint64_t meta_pa;
	uint32_t meta_size;
	uint8_t win_id;
	bool enable;
};

// refer to struct st_dv_config_bin_info
typedef struct v4l2_PQ_dv_config_bin_info_s {
	__u64 pa;
	__u64 va;
	__u32 size;
	__u32 en;
} v4l2_PQ_dv_config_bin_info_t;

// refer to struct st_dv_config_mode_info
typedef struct v4l2_PQ_dv_config_mode_info_s {
	__u32 en;
	__u32 num;
	__u8  name[V4L2_DV_MAX_MODE_NUM][V4L2_DV_MAX_MODE_LENGTH + 1];
} v4l2_PQ_dv_config_mode_info_t;

// refer to struct st_dv_config_info
typedef struct v4l2_PQ_dv_config_info_s {
	v4l2_PQ_dv_config_bin_info_t bin_info;
	v4l2_PQ_dv_config_mode_info_t mode_info;
} v4l2_PQ_dv_config_info_t;

struct v4l2_b2r_cap {
	__u32 h_max_420;
	__u32 h_max_422;
};
//===============================================================================
enum v4l2_stage_pattern_color_space {
	v4l2_COLOR_SPACE_RGB,
	v4l2_COLOR_SPACE_YUV_FULL,
	v4l2_COLOR_SPACE_YUV_LIMITED,
	v4l2_COLOR_SPACE_MAX,
};

enum v4l2_pq_pattern_position {
	v4l2_PQ_PATTERN_POSITION_IP2,
	v4l2_PQ_PATTERN_POSITION_NR_DNR,
	v4l2_PQ_PATTERN_POSITION_NR_IPMR,
	v4l2_PQ_PATTERN_POSITION_NR_OPM,
	v4l2_PQ_PATTERN_POSITION_NR_DI,
	v4l2_PQ_PATTERN_POSITION_DI,
	v4l2_PQ_PATTERN_POSITION_SRS_IN,
	v4l2_PQ_PATTERN_POSITION_SRS_OUT,
	v4l2_PQ_PATTERN_POSITION_VOP,
	v4l2_PQ_PATTERN_POSITION_IP2_POST,
	v4l2_PQ_PATTERN_POSITION_SCIP_DV,
	v4l2_PQ_PATTERN_POSITION_SCDV_DV,
	v4l2_PQ_PATTERN_POSITION_B2R_DMA,
	v4l2_PQ_PATTERN_POSITION_B2R_LITE1_DMA,
	v4l2_PQ_PATTERN_POSITION_B2R_PRE,
	v4l2_PQ_PATTERN_POSITION_B2R_POST,
	v4l2_PQ_PATTERN_POSITION_MAX,
};

struct v4l2_pq_pattern_color {
	unsigned short red;		//10 bits
	unsigned short green;	//10 bits
	unsigned short blue;	//10 bits
};


struct v4l2_stage_pattern_info {
	bool enable;
	enum v4l2_pq_pattern_position position;
	enum v4l2_stage_pattern_color_space color_space;
	struct v4l2_pq_pattern_color color;
};




#endif
