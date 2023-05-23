/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_XC_H
#define _MTK_PQ_XC_H

#include <linux/types.h>
#include <linux/videodev2.h>
#include "mtk_pq_svp.h"
#include "mtk_pq_buffer.h"

#define PQ_DEV_NUM 2   /*need to fix*/

/* ========================================================================== */
/* ------------ meta define, should move to meta dd .h in future ------------ */
/* ========================================================================== */
#define PQ_MDW_CTRL_META_TAG	"pq_mdw_ctrl"
#define PQ_IDR_CTRL_META_TAG	"pq_idr_ctrl"

enum pq_output_path {
	PQ_OUTPUT_PATH_WITHOUT_BUF,
	PQ_OUTPUT_PATH_WITH_BUF_MDW,
	PQ_OUTPUT_PATH_WITH_BUF_FRC,
	PQ_OUTPUT_PATH_MAX,
};

struct pq_mdw_ctrl {
	struct pq_buffer frame_buf[3];
	struct pq_buffer meta_buf;
	enum pq_output_path output_path;
	u32 mem_fmt;
	u8 plane_num;
	u32 width;
	u32 height;
	u8 v_align;
	u8 h_align;
};

struct pq_idr_ctrl {
	struct pq_buffer frame_buf[3];
	struct pq_buffer meta_buf;
	struct v4l2_crop crop;
	u32 mem_fmt;
	u32 width;
	u32 height;
	u16 index;	/* force read index */
};

/* ========================================================================== */
/* -------------------------- extend color format --------------------------- */
/* ========================================================================== */

#define fourcc_code(a, b, c, d)	((__u32)(a) | ((__u32)(b) << 8) | \
				((__u32)(c) << 16) | ((__u32)(d) << 24))

/* ========================================================================== */
/*   YUV fourcc character definition                                          */
/*                                                                            */
/*   a _ _ _  , a = Y(YUV) U(UVY) V(VYU) y(YVU) u(UYV) v(VUY)                 */
/*                                                                            */
/*   _ b _ _  , b = 4(444) 2(422) 0(420)                                      */
/*                                                                            */
/*   _ _ c _  , c = 6(YC6bit) 8(YC8bit) A(YC10bit) C(YC12bit)                 */
/*                  %(Y8bitC6bit)...                                          */
/*                                                                            */
/*   _ _ _ d  , d = C(compressed) P(planar) &(compressed & planar)            */
/* ========================================================================== */

/* packed YUV */
#define MEM_FMT_YUV444_VYU_10B		fourcc_code('V', '4', 'A', ' ')
#define MEM_FMT_YUV444_VYU_8B		fourcc_code('V', '4', '8', ' ')

#define MEM_FMT_YUV422_YUV_12B		fourcc_code('Y', '2', 'C', ' ')
#define MEM_FMT_YUV422_YUV_8B		fourcc_code('Y', '2', '8', ' ')
#define MEM_FMT_YUV422_YVYU_8B		fourcc_code('Y', 'V', 'Y', 'U')	//YVYU


/* 2 plane YUV */
#define MEM_FMT_YUV422_Y_UV_10B		fourcc_code('Y', '2', 'A', 'P')
#define MEM_FMT_YUV422_Y_VU_10B		fourcc_code('y', '2', 'A', 'P')
#define MEM_FMT_YUV422_Y_VU_8B		fourcc_code('y', '2', '8', 'P')	//NV61
#define MEM_FMT_YUV422_Y_UV_8B_CE	fourcc_code('Y', '2', '8', '&')
#define MEM_FMT_YUV422_Y_VU_8B_CE	fourcc_code('y', '2', '8', '&')
#define MEM_FMT_YUV422_Y_UV_8B_6B_CE	fourcc_code('Y', '2', '%', '&')
#define MEM_FMT_YUV422_Y_UV_6B_CE	fourcc_code('Y', '2', '6', '&')
#define MEM_FMT_YUV422_Y_VU_6B_CE	fourcc_code('y', '2', '6', '&')

#define MEM_FMT_YUV420_Y_UV_10B		fourcc_code('Y', '0', 'A', 'P')
#define MEM_FMT_YUV420_Y_VU_10B		fourcc_code('y', '0', 'A', 'P')
#define MEM_FMT_YUV420_Y_VU_8B		fourcc_code('y', '0', '8', 'P')	//NV21
#define MEM_FMT_YUV420_Y_UV_8B_CE	fourcc_code('Y', '0', '8', '&')
#define MEM_FMT_YUV420_Y_VU_8B_CE	fourcc_code('y', '0', '8', '&')
#define MEM_FMT_YUV420_Y_UV_8B_6B_CE	fourcc_code('Y', '0', '%', '&')
#define MEM_FMT_YUV420_Y_UV_6B_CE	fourcc_code('Y', '0', '6', '&')
#define MEM_FMT_YUV420_Y_VU_6B_CE	fourcc_code('y', '0', '6', '&')

/* 3 plane YUV */
#define MEM_FMT_YUV444_Y_U_V_8B_CE	fourcc_code('Y', '4', '8', '&')


/* ========================================================================== */
/*   RGB fourcc character definition                                          */
/*                                                                            */
/*   rule for 1 plane                                                         */
/*                                                                            */
/*   x y _ _  , xy = AR(ARGB) AB(ABGR) RA(RGBA) BA(BGRA)                      */
/*                                                                            */
/*   _ _ z w  , zw = 15(555) 16(565) 24(8888) 30(2101010)                     */
/*                                                                            */
/*   rule for 3 plane                                                         */
/*                                                                            */
/*   x _ _ _  , x = R(RGB) B(BGR) r(RGB compressed) b(BGR compressed)         */
/*                                                                            */
/*   _ y z w  , yzw = 888(8bit) AAA(10bit) CCC(12it)                          */
/* ========================================================================== */

/* 1 plane RGB */
#define MEM_FMT_RGB565			fourcc_code('R', 'G', '1', '6')
#define MEM_FMT_RGB888			fourcc_code('R', 'G', '2', '4')
#define MEM_FMT_RGB101010		fourcc_code('R', 'G', '3', '0')
#define MEM_FMT_RGB121212		fourcc_code('R', 'G', '3', '6')

#define MEM_FMT_ARGB8888		fourcc_code('A', 'R', '2', '4')
#define MEM_FMT_ABGR8888		fourcc_code('A', 'B', '2', '4')
#define MEM_FMT_RGBA8888		fourcc_code('R', 'A', '2', '4')
#define MEM_FMT_BGRA8888		fourcc_code('B', 'A', '2', '4')

#define MEM_FMT_ARGB2101010		fourcc_code('A', 'R', '3', '0')
#define MEM_FMT_ABGR2101010		fourcc_code('A', 'B', '3', '0')
#define MEM_FMT_RGBA1010102		fourcc_code('R', 'A', '3', '0')
#define MEM_FMT_BGRA1010102		fourcc_code('A', 'R', '3', '0')

/* 3 plane RGB */
#define MEM_FMT_RGB888_R_G_B_CE		v4l2_fourcc('r', '8', '8', '8')


/* enum*/
enum set_window_type {
	SET_WINDOW_TYPE_CAP_WIN,
	SET_WINDOW_TYPE_CROP_WIN,
	SET_WINDOW_TYPE_DISP_WIN,
	SET_WINDOW_TYPE_PRE_SCALING,
	SET_WINDOW_TYPE_POST_SCALING,
};

enum set_mirror_type {
	SET_MIRROR_TYPE_H,
	SET_MIRROR_TYPE_V,
};

/* struct*/
struct scaling_info {
	bool bHCusScaling;
	__u16  u16HCusScalingSrc;
	__u16  u16HCusScalingDst;
	bool bVCusScaling;
	__u16  u16VCusScalingSrc;
	__u16  u16VCusScalingDst;
};

struct v4l2_xc_info {
	__u8 window_num;

	// system related
	__u32 XTAL_Clock;                   ///<Crystal clock in Hz

	// This is related to chip package. ( Share Ground / Non-Share Ground )
	bool IsShareGround;

	// frame buffer related
	// scaler main window frame buffer start address
	// absolute without any alignment
	__u64 Main_FB_Start_Addr;
	// scaler main window frame buffer size, the unit is BYTES
	__u64 Main_FB_Size;
	// scaler sub window frame buffer start address
	// absolute without any alignment
	__u64 Sub_FB_Start_Addr;
	// scaler sub window frame buffer size, the unit is BYTES
	__u64 Sub_FB_Size;

	// frcm frame buffer related
	// scaler main window frcm frame buffer start address
	// absolute without any alignment
	__u64 Main_FRCM_FB_Start_Addr;
	// scaler main window frcm frame buffer size, the unit is BYTES
	__u64 Main_FRCM_FB_Size;
	// scaler sub window frcm frame buffer start address
	// absolute without any alignment
	__u64 Sub_FRCM_FB_Start_Addr;
	// scaler sub window frcm frame buffer size, the unit is BYTES
	__u64 Sub_FRCM_FB_Size;

	// XC Dual miu frame buffer related
	// scaler dual miu frame buffer start address
	// absolute without any alignment
	__u64 Dual_FB_Start_Addr;
	// scaler dual miu frame buffer size, the unit is BYTES
	__u64 Dual_FB_Size;

	// Autodownload buffer related
	__u64 Auto_Download_Start_Addr;
	__u64 Auto_Download_Size;
	// compatible for xc86, can be remove if move into script DD
	__u64 Auto_Download_XVYCC_Size;

	struct pq_buffer **pBufferTable;

#ifdef CONFIG_OPTEE
	struct pq_secure_info secure_info;
#endif

	__u8 pq_iommu_idx_scmi;
	__u8 pq_iommu_idx_znr;
	__u8 pq_iommu_idx_ucm;
	__u8 pq_iommu_idx_abf;

	enum pq_output_path output_path;

	struct pq_mdw_ctrl mdw;
	struct pq_idr_ctrl idr;
};

struct mtk_pq_device;

/* function */
int mtk_xc_get_fb_mode(enum v4l2_fb_level *fb_level);
int mtk_xc_set_hflip(__u8 win, bool enable);
int mtk_xc_set_vflip(__u8 win, bool enable);
int mtk_xc_get_hflip(__u8 win, bool *enable);
int mtk_xc_get_vflip(__u8 win, bool *enable);
int mtk_xc_set_pix_format_in_mplane(
	struct v4l2_format *fmt,
	struct v4l2_xc_info *xc_info);
int mtk_xc_set_pix_format_out_mplane(
	struct v4l2_format *fmt,
	struct v4l2_xc_info *xc_info);
int mtk_xc_cap_qbuf(struct mtk_pq_device *pq_dev, struct vb2_buffer *vb);
int mtk_xc_out_qbuf(struct mtk_pq_device *pq_dev, struct vb2_buffer *vb);
int mtk_xc_out_streamon(struct mtk_pq_device *pq_dev);
int mtk_xc_out_streamoff(struct mtk_pq_device *pq_dev);
int mtk_xc_out_s_crop(struct mtk_pq_device *pq_dev, const struct v4l2_crop *crop);
int mtk_xc_out_g_crop(struct mtk_pq_device *pq_dev, struct v4l2_crop *crop);
int mtk_xc_subscribe_event(__u32 event_type, __u16 *dev_id);
int mtk_xc_unsubscribe_event(__u32 event_type, __u16 *dev_id);
int mtk_pq_register_xc_subdev(struct v4l2_device *pq_dev,
	struct v4l2_subdev *subdev_xc,
	struct v4l2_ctrl_handler *xc_ctrl_handler);
void mtk_pq_unregister_xc_subdev(struct v4l2_subdev *subdev_xc);
#endif
