/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/types.h>  //For GKI

#define EXT_CID_DIP_OFT                (0x400)
#define V4L2_CID_EXT_DIP_BASE          (V4L2_CID_PRIVATE_BASE+EXT_CID_DIP_OFT)
#define V4L2_CID_EXT_DIP_SET_MFDEC     (V4L2_CID_EXT_DIP_BASE+1)
#define V4L2_CID_EXT_DIP_SET_OP_MODE   (V4L2_CID_EXT_DIP_SET_MFDEC+1)
#define V4L2_CID_EXT_DIP_SET_BUF_MODE  (V4L2_CID_EXT_DIP_SET_OP_MODE+1)
#define V4L2_CID_EXT_DIP_METADATA_FD   (V4L2_CID_EXT_DIP_SET_BUF_MODE+1)
#define V4L2_CID_EXT_DIP_SET_ONEWIN    (V4L2_CID_EXT_DIP_METADATA_FD+1)
#define V4L2_CID_EXT_DIP_GET_HWCAP     (V4L2_CID_EXT_DIP_SET_ONEWIN+1)
#define V4L2_CID_EXT_DIP_SOURCE_INFO   (V4L2_CID_EXT_DIP_GET_HWCAP+1)
#define V4L2_CID_EXT_DIP_OUTPUT_INFO   (V4L2_CID_EXT_DIP_SOURCE_INFO+1)

#define V4L2_PIX_FMT_MT_YUV420_32_32   v4l2_fourcc('M', 'T', '3', '3') /* 12 Y/CbCr 4:2:0 32x32 */
#define V4L2_PIX_FMT_MT_YUV420_32_16   v4l2_fourcc('M', 'T', '3', '1') /* 12 Y/CbCr 4:2:0 32x16 */
#define V4L2_PIX_FMT_MT_YUV420_16_32   v4l2_fourcc('M', 'T', '1', '3') /* 12 Y/CbCr 4:2:0 16x32 */
#define V4L2_PIX_FMT_MT_MFDEC      v4l2_fourcc('M', 'M', 'F', 'D') /* Mediatek compressed MFDEC */

/* 32, LSB->B(8)->G(8)->R(8)->A(8)->MSB */
//#define V4L2_PIX_FMT_ABGR32  v4l2_fourcc('A', 'R', '2', '4')
/* 32bit, LSB->R(8)->G(8)->B(8)->A(8)->MSB */
#define V4L2_PIX_FMT_DIP_ABGR32_SWAP  v4l2_fourcc('A', 'R', '4', '2')
/* 32, LSB->A(8)->R(8)->G(8)->B(8)->MSB */
//#define V4L2_PIX_FMT_ARGB32  v4l2_fourcc('B', 'A', '2', '4')
/* 32bit, LSB->A(8)->B(8)->G(8)->R(8)->MSB */
#define V4L2_PIX_FMT_DIP_ARGB32_SWAP  v4l2_fourcc('B', 'A', '4', '2')
/* 32bit, MSB->A(2)->R(10)->G(10)->B(10)->LSB */
#define V4L2_PIX_FMT_DIP_ARGB2101010  v4l2_fourcc('M', 'R', '2', '1')
/* 24, LSB->B(8)->G(8)->R(8)->MSB */
//#define V4L2_PIX_FMT_BGR24   v4l2_fourcc('B', 'G', 'R', '3')
/* 16  LSB->R(5)-G(6)-B(5)->MSB */
//#define V4L2_PIX_FMT_RGB565X v4l2_fourcc('R', 'G', 'B', 'R')
/* 16  YUV 4:2:2, LSB->Y->U->Y->V->MSB */
//#define V4L2_PIX_FMT_YUYV    v4l2_fourcc('Y', 'U', 'Y', 'V')
/* 16  YUV 4:2:2, LSB->Y->V->Y->U->MSB */
//#define V4L2_PIX_FMT_YVYU    v4l2_fourcc('Y', 'V', 'Y', 'U')
/* 16  YUV 4:2:2, LSB->U->Y->V->Y->MSB */
//#define V4L2_PIX_FMT_UYVY    v4l2_fourcc('U', 'Y', 'V', 'Y')
/* 16  YUV 4:2:2, LSB->V->Y->U->Y->MSB */
//#define V4L2_PIX_FMT_VYUY    v4l2_fourcc('V', 'Y', 'U', 'Y')

/* YUV422 10bit, LSB->Y->U->Y->V->MSB */
#define V4L2_PIX_FMT_DIP_YUYV_10BIT    v4l2_fourcc('M', '1', 'Y', 'U')
/* YVU422 10bit, LSB->Y->V->Y->U->MSB */
#define V4L2_PIX_FMT_DIP_YVYU_10BIT    v4l2_fourcc('M', '1', 'Y', 'V')
/* YUV422 10bit, LSB->U->Y->V->Y->MSB */
#define V4L2_PIX_FMT_DIP_UYVY_10BIT    v4l2_fourcc('M', '1', 'U', 'Y')
/* YUV422 10bit, LSB->V->Y->U->Y->MSB */
#define V4L2_PIX_FMT_DIP_VYUY_10BIT    v4l2_fourcc('M', '1', 'V', 'Y')

/* 16  Y/CbCr 4:2:2, 2 plane, LSB->U->V->MSB */
//#define V4L2_PIX_FMT_NV16    v4l2_fourcc('N', 'V', '1', '6')
/* 16  Y/CrCb 4:2:2, 2 plane, LSB->V->U->MSB */
//#define V4L2_PIX_FMT_NV61    v4l2_fourcc('N', 'V', '6', '1')
/* YC422 8b  BLK_TILE, LSB->U->Y->V->Y->MSB */
#define V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE   v4l2_fourcc('M', '8', 'B', 'T')
/* YC422 10b BLK_TILE, LSB->U->Y->V->Y->MSB */
#define V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE  v4l2_fourcc('M', 'T', 'B', 'T')
/* 12  Y/CbCr 4:2:0, 2 plane, LSB->U->V->MSB */
//#define V4L2_PIX_FMT_NV12    v4l2_fourcc('N', 'V', '1', '2')
/* 12  Y/CrCb 4:2:0, 2 plane, LSB->V->U->MSB */
//#define V4L2_PIX_FMT_NV21    v4l2_fourcc('N', 'V', '2', '1')

/* MTK 8-bit block mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21S        v4l2_fourcc('M', '2', '1', 'S')
/* MTK 10-bit tile block mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21S10T     v4l2_fourcc('M', 'T', 'S', 'T')
/* MTK 10-bit raster block mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21S10R     v4l2_fourcc('M', 'T', 'S', 'R')
/* MTK 10-bit linear mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21S10L     v4l2_fourcc('M', 'T', 'S', 'L')
/* MTK 10-bit tile block jump mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21S10TJ    v4l2_fourcc('M', 'J', 'S', 'T')
/* MTK 10-bit raster block jump mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21S10RJ    v4l2_fourcc('M', 'J', 'S', 'R')

/* MTK 8-bit compressed block mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21CS       v4l2_fourcc('M', '2', 'C', 'S')
/* MTK 8-bit compressed block au offset mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21CSA      v4l2_fourcc('M', 'A', 'C', 'S')
/* MTK 10-bit tile compressed block mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21CS10T    v4l2_fourcc('M', 'C', 'S', 'T')
/* MTK 10-bit raster compressed block mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21CS10R    v4l2_fourcc('M', 'C', 'S', 'R')
/* MTK 10-bit tile compressed block jump mode, two contiguous planes */
#define V4L2_PIX_FMT_DIP_MT21CS10TJ   v4l2_fourcc('J', 'C', 'S', 'T')
/* MTK 10-bit raster compressed block jump mode, two cont. planes */
#define V4L2_PIX_FMT_DIP_MT21CS10RJ   v4l2_fourcc('J', 'C', 'S', 'R')

/* MTK VSD8x4, two contiguous planes */
#define V4L2_PIX_FMT_DIP_VSD8X4    v4l2_fourcc('M', 'V', '8', '4')
/* MTK VSD8x2, two contiguous planes */
#define V4L2_PIX_FMT_DIP_VSD8X2    v4l2_fourcc('M', 'V', '8', '2')

#define DIP_GET_HWCAP_VER           (1)
#define DIP_SOURCE_INFO_VER         (1)
#define DIP_OUTPUT_INFO_VER         (1)

/// Define source type for DIP
typedef enum {
	E_DIP_SOURCE_SRCCAP_MAIN = 0,
	E_DIP_SOURCE_SRCCAP_SUB,
	E_DIP_SOURCE_PQ_START,
	E_DIP_SOURCE_PQ_HDR,
	E_DIP_SOURCE_PQ_SHARPNESS,
	E_DIP_SOURCE_PQ_SCALING,
	E_DIP_SOURCE_PQ_CHROMA_COMPENSAT,
	E_DIP_SOURCE_PQ_BOOST,
	E_DIP_SOURCE_PQ_COLOR_MANAGER,
	E_DIP_SOURCE_STREAM_ALL_VIDEO,
	E_DIP_SOURCE_STREAM_ALL_VIDEO_OSDB,
	E_DIP_SOURCE_STREAM_POST,
	E_DIP_SOURCE_OSD,
	E_DIP_SOURCE_DIPR,
	E_DIP_SOURCE_DIPR_MFDEC,  //< DIP from MFDEC >
	E_DIP_SOURCE_B2R,
	E_DIP_SOURCE_MAX          //< capture point max >
} EN_DIP_SOURCE;

struct ST_DIP_METADATA {
	__u16 u16Enable;
	int meta_fd;
	EN_DIP_SOURCE enDIPSrc;
};

typedef enum {
	E_BUF_VA_MODE     = 0x00,
	E_BUF_PA_MODE,
	E_BUF_IOVA_MODE,
} EN_BUF_MODE;

typedef enum {
	E_DIP_MFEDC_TILE_16_32     = 0x00,
	E_DIP_MFEDC_TILE_32_16     = 0x01,
	E_DIP_MFEDC_TILE_32_32     = 0x02,
} EN_MFDEC_TILE_MODE;

typedef enum {
	E_XC_DIP_H26X_MODE     = 0x00,
	E_XC_DIP_VP9_MODE      = 0x01,
} EN_MFDEC_VP9_MODE;

struct ST_DIP_MFDEC_INFO {
	__u32 u32version;
	__u32 u32size;
	__u16 u16Enable;
	__u16 u16SelNum;
	__u16 u16HMirror;
	__u16 u16VMirror;
	EN_MFDEC_TILE_MODE enMFDecTileMode;
	__u16 u16Bypasse;
	__u16 u16VP9Mode;
	__u16 u16FBPitch;
	__u16 u16StartX;
	__u16 u16StartY;
	__u16 u16HSize;
	__u16 u16VSize;
	__u16 u16BitlenPitch;
	__u16 u16SimMode;
};

struct ST_DIP_BUF_MODE {
	__u32 u32version;
	__u32 u32size;
	__u32 u32BufType;
	__u32 u32BufMode;
};

typedef enum {
	E_DIP_MODE_NONE = 0x0,
	E_DIP_MODE_DI = 0x1,
	E_DIP_MODE_PDNR = 0x2,
} EN_DIP_MODE;

typedef enum {
	E_DIP_CAP_ADDR_ALIGN = 0x0,
	E_DIP_CAP_WIDTH_ALIGN,
	E_DIP_CAP_PITCH_ALIGN,
	E_DIP_CAP_IN_WIDTH_MAX,
	E_DIP_CAP_IN_WIDTH_MIN,
	E_DIP_CAP_OUT_WIDTH_MAX,
	E_DIP_CAP_OUT_WIDTH_MIN,
	E_DIP_CAP_CROP_W_ALIGN,
	E_DIP_CAP_DI_IN_WIDTH_ALIGN,
	E_DIP_CAP_DI_IN_WIDTH_MAX,
	E_DIP_CAP_DNR_IN_WIDTH_ALIGN,
	E_DIP_CAP_DNR_IN_WIDTH_MAX,
	E_DIP_CAP_VSD_IN_WIDTH_MAX,
	E_DIP_CAP_HVSP_IN_WIDTH_MAX,
	E_DIP_CAP_HMIR_WIDTH_MAX,
	E_DIP_CAP_RWSEP_WIDTH_ALIGN,
	E_DIP_CAP_RWSEP_IN_MAX,
	E_DIP_CAP_RWSEP_OUT_MAX,
	E_DIP_CAP_ROT_HV_ALIGN,
	E_DIP_CAP_FMT_BPP,
	E_DIP_CAP_INPUT_ALIGN,
	E_DIP_CAP_B2R_ADDR_ALIGN = 0x100,
	E_DIP_CAP_B2R_WIDTH_ALIGN,
	E_DIP_CAP_B2R_WIDTH_MAX,
} EN_DIP_CAP;

struct ST_DIP_MODE {
	__u32 u32Version;
	__u32 u32Length;
	EN_DIP_MODE enMode;
	void *pModeSettings;
};

struct ST_DIP_WINID_INFO {
	__u32 u32Version;
	__u32 u32Length;
	__u8 u8Enable;
	__u16 u16WinId;
};

struct ST_DIP_CAP_INFO {
	__u32 u32Version;
	__u32 u32Length;
	EN_DIP_CAP enDipCap;
	__u32 u32Fourcc;
	__u32 u32RetVal;
	__u32 u32RetValB;
};

typedef enum  {
	E_CLR_FMT_RGB = 0x00,
	E_CLR_FMT_YUV444,
	E_CLR_FMT_YUV422,
	E_CLR_FMT_YUV420,
} EN_CLR_FMT;

struct ST_DIP_SOURCE_INFO {
	__u32 u32Version;
	__u32 u32Length;
	EN_DIP_SOURCE enDIPSrc;
	__u32 u32Width;
	__u32 u32Height;
	EN_CLR_FMT enClrFmt;
	__u8 u8PxlEngine;
	__u8 u8IsValidFd;
	int MetaFd;
	__u32 u32PipeLineId;
	__u8 u8IsSecutiry;
	__u32 u32CropMaxWidth;
};

struct ST_DIP_OUTPUT_INFO {
	__u32 u32Version;
	__u32 u32Length;
	__u32 u32IsSecutiry;
	__u8 u8ContiBufCnt;
};
