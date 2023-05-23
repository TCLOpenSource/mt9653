// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/dma-buf.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <uapi/linux/mtk_iommu_dtv_api.h>
#include <uapi/linux/metadata_utility/m_vdec.h>
#include <uapi/linux/mtk_dip.h>
#include <asm-generic/div64.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>

#include "mtk_dip_priv.h"
#include "mtk_dip_svc.h"
#include "metadata_utility.h"
#include "mtk_iommu_dtv_api.h"
#include "mtk-efuse.h"

#define DIP_ERR(args...)	pr_err("[DIP ERR]" args)
#define DIP_WARNING(args...)	pr_warn("[DIP WARNING]" args)
#define DIP_INFO(args...)	pr_info("[DIP INFO]" args)
#define DIP_DBG_API(args...)	\
do { \
	switch (dev->variant->u16DIPIdx) { \
	case DIP0_WIN_IDX: \
		if (gu32Dip0LogLevel & LOG_LEVEL_API) \
			pr_crit("[DIP0 DBG]" args); \
		break; \
	case DIP1_WIN_IDX: \
		if (gu32Dip1LogLevel & LOG_LEVEL_API) \
			pr_crit("[DIP1 DBG]" args); \
		break; \
	case DIP2_WIN_IDX: \
		if (gu32Dip2LogLevel & LOG_LEVEL_API) \
			pr_crit("[DIP2 DBG]" args); \
		break; \
	default: \
		break; \
	} \
} while (0)

#define DIP_DBG_BUF(args...)	\
do { \
	switch (dev->variant->u16DIPIdx) { \
	case DIP0_WIN_IDX: \
		if (gu32Dip0LogLevel & LOG_LEVEL_BUF) \
			pr_crit("[DIP0 DBG]" args); \
		break; \
	case DIP1_WIN_IDX: \
		if (gu32Dip1LogLevel & LOG_LEVEL_BUF) \
			pr_crit("[DIP1 DBG]" args); \
		break; \
	case DIP2_WIN_IDX: \
		if (gu32Dip2LogLevel & LOG_LEVEL_BUF) \
			pr_crit("[DIP2 DBG]" args); \
		break; \
	default: \
		break; \
	} \
} while (0)

#define DIP_DBG_QUEUE(args...)	\
do { \
	switch (dev->variant->u16DIPIdx) { \
	case DIP0_WIN_IDX: \
		if (gu32Dip0LogLevel & LOG_LEVEL_QUEUE) \
			pr_crit("[DIP0 DBG]" args); \
		break; \
	case DIP1_WIN_IDX: \
		if (gu32Dip1LogLevel & LOG_LEVEL_QUEUE) \
			pr_crit("[DIP1 DBG]" args); \
		break; \
	case DIP2_WIN_IDX: \
		if (gu32Dip2LogLevel & LOG_LEVEL_QUEUE) \
			pr_crit("[DIP2 DBG]" args); \
		break; \
	default: \
		break; \
	} \
} while (0)

#define DIP_DBG_IOCTLTIME(args...)	\
do { \
	switch (dev->variant->u16DIPIdx) { \
	case DIP0_WIN_IDX: \
		if (gu32Dip0LogLevel & LOG_LEVEL_IOCTLTIME) \
			pr_crit("[DIP0 DBG]" args); \
		break; \
	case DIP1_WIN_IDX: \
		if (gu32Dip1LogLevel & LOG_LEVEL_IOCTLTIME) \
			pr_crit("[DIP1 DBG]" args); \
		break; \
	case DIP2_WIN_IDX: \
		if (gu32Dip2LogLevel & LOG_LEVEL_IOCTLTIME) \
			pr_crit("[DIP2 DBG]" args); \
		break; \
	default: \
		break; \
	} \
} while (0)

#define DIP0_WIN_IDX    (0)
#define DIP1_WIN_IDX    (1)
#define DIP2_WIN_IDX    (2)

#define DMA_INPUT     (1)
#define DMA_OUTPUT    (2)

#define BUF_VA_MODE      (0)
#define BUF_PA_MODE      (1)
#define BUF_IOVA_MODE    (2)

#define DEVICE_CAPS  (V4L2_CAP_VIDEO_CAPTURE_MPLANE|\
						V4L2_CAP_VIDEO_OUTPUT_MPLANE|\
						V4L2_CAP_VIDEO_M2M_MPLANE|\
						V4L2_CAP_STREAMING)

#define ATTR_MOD     (0600)

#define fh2ctx(__fh) container_of(__fh, struct dip_ctx, fh)

#define SPT_FMT_CNT(x) ARRAY_SIZE(x)

#define DIP_CAPABILITY_SHOW(DIPItem, DIPItemCap)\
{\
	if (ssize > u8Size) {\
		DIP_ERR("Failed to get" DIPItem "\n");\
	} else {\
		ssize += snprintf(buf + ssize, u8Size - ssize, "%d\n", DIPItemCap);\
	} \
}

#define ALIGN_UPTO_32(x)  ((((x) + 31) >> 5) << 5)

static struct ST_DIP_DINR_BUF gstDiNrBufInfo = {0};

static u32 gu32Dip0LogLevel = LOG_LEVEL_OFF;
static u32 gu32Dip1LogLevel = LOG_LEVEL_OFF;
static u32 gu32Dip2LogLevel = LOG_LEVEL_OFF;

struct mtk_vb2_buf {
	enum dma_data_direction   dma_dir;
	unsigned long     size;
	unsigned long     paddr;
	atomic_t          refcount;
};

struct dip_cap_dev gDIPcap_capability[3];

static char *b2r_clocks[E_B2R_CLK_MAX] = {
	"top_subdc",
	"top_smi",
	"top_mfdec",
};

static struct dip_fmt stDip0FmtList[] = {
	{
		.name = "ABGR32",
		.fourcc = V4L2_PIX_FMT_ABGR32,
	},
	{
		.name = "ARGB32",
		.fourcc = V4L2_PIX_FMT_ARGB32,
	},
	{
		.name = "ABGR32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ABGR32_SWAP,
	},
	{
		.name = "ARGB32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB32_SWAP,
	},
	{
		.name = "ARGB2101010",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB2101010,
	},
	{
		.name = "BGR24",
		.fourcc = V4L2_PIX_FMT_BGR24,
	},
	{
		.name = "RGB565X",
		.fourcc = V4L2_PIX_FMT_RGB565X,
	},
	{
		.name = "YUYV",
		.fourcc = V4L2_PIX_FMT_YUYV,
	},
	{
		.name = "VYUY",
		.fourcc = V4L2_PIX_FMT_VYUY,
	},
	{
		.name = "YVYU",
		.fourcc = V4L2_PIX_FMT_YVYU,
	},
	{
		.name = "UYVY",
		.fourcc = V4L2_PIX_FMT_UYVY,
	},
	{
		.name = "YUYV_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YUYV_10BIT,
	},
	{
		.name = "YVYU_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YVYU_10BIT,
	},
	{
		.name = "UYVY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_UYVY_10BIT,
	},
	{
		.name = "VYUY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_VYUY_10BIT,
	},
	{
		.name = "NV16",
		.fourcc = V4L2_PIX_FMT_NV16,
	},
	{
		.name = "NV61",
		.fourcc = V4L2_PIX_FMT_NV61,
	},
	{
		.name = "8B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE,
	},
	{
		.name = "10B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE,
	},
	{
		.name = "NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
	},
	{
		.name = "NV21",
		.fourcc = V4L2_PIX_FMT_NV21,
	},
	{
		.name = "MT21S10L",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10L,
	},
};

static struct dip_fmt stDip1FmtList[] = {
	{
		.name = "ABGR32",
		.fourcc = V4L2_PIX_FMT_ABGR32,
	},
	{
		.name = "ARGB32",
		.fourcc = V4L2_PIX_FMT_ARGB32,
	},
	{
		.name = "ABGR32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ABGR32_SWAP,
	},
	{
		.name = "ARGB32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB32_SWAP,
	},
	{
		.name = "ARGB2101010",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB2101010,
	},
	{
		.name = "BGR24",
		.fourcc = V4L2_PIX_FMT_BGR24,
	},
	{
		.name = "RGB565X",
		.fourcc = V4L2_PIX_FMT_RGB565X,
	},
	{
		.name = "YUYV",
		.fourcc = V4L2_PIX_FMT_YUYV,
	},
	{
		.name = "VYUY",
		.fourcc = V4L2_PIX_FMT_VYUY,
	},
	{
		.name = "YVYU",
		.fourcc = V4L2_PIX_FMT_YVYU,
	},
	{
		.name = "UYVY",
		.fourcc = V4L2_PIX_FMT_UYVY,
	},
	{
		.name = "YUYV_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YUYV_10BIT,
	},
	{
		.name = "YVYU_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YVYU_10BIT,
	},
	{
		.name = "UYVY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_UYVY_10BIT,
	},
	{
		.name = "VYUY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_VYUY_10BIT,
	},
	{
		.name = "NV16",
		.fourcc = V4L2_PIX_FMT_NV16,
	},
	{
		.name = "NV61",
		.fourcc = V4L2_PIX_FMT_NV61,
	},
	{
		.name = "8B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE,
	},
	{
		.name = "10B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE,
	},
	{
		.name = "NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
	},
	{
		.name = "NV21",
		.fourcc = V4L2_PIX_FMT_NV21,
	},
	{
		.name = "MT21S10L",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10L,
	},
	{
		.name = "MT21S",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S,
	},
	{
		.name = "MT21S10T",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10T,
	},
	{
		.name = "MT21S10R",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10R,
	},
};

static struct dip_fmt stDip2FmtList[] = {
	{
		.name = "ABGR32",
		.fourcc = V4L2_PIX_FMT_ABGR32,
	},
	{
		.name = "ARGB32",
		.fourcc = V4L2_PIX_FMT_ARGB32,
	},
	{
		.name = "ABGR32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ABGR32_SWAP,
	},
	{
		.name = "ARGB32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB32_SWAP,
	},
	{
		.name = "ARGB2101010",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB2101010,
	},
	{
		.name = "BGR24",
		.fourcc = V4L2_PIX_FMT_BGR24,
	},
	{
		.name = "RGB565X",
		.fourcc = V4L2_PIX_FMT_RGB565X,
	},
	{
		.name = "YUYV",
		.fourcc = V4L2_PIX_FMT_YUYV,
	},
	{
		.name = "VYUY",
		.fourcc = V4L2_PIX_FMT_VYUY,
	},
	{
		.name = "YVYU",
		.fourcc = V4L2_PIX_FMT_YVYU,
	},
	{
		.name = "UYVY",
		.fourcc = V4L2_PIX_FMT_UYVY,
	},
	{
		.name = "YUYV_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YUYV_10BIT,
	},
	{
		.name = "YVYU_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YVYU_10BIT,
	},
	{
		.name = "UYVY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_UYVY_10BIT,
	},
	{
		.name = "VYUY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_VYUY_10BIT,
	},
	{
		.name = "NV16",
		.fourcc = V4L2_PIX_FMT_NV16,
	},
	{
		.name = "NV61",
		.fourcc = V4L2_PIX_FMT_NV61,
	},
	{
		.name = "8B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE,
	},
	{
		.name = "10B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE,
	},
	{
		.name = "NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
	},
	{
		.name = "NV21",
		.fourcc = V4L2_PIX_FMT_NV21,
	},
	{
		.name = "MT21S10L",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10L,
	},
};

static struct dip_fmt stDiprFmtList[] = {
	{
		.name = "ABGR32",
		.fourcc = V4L2_PIX_FMT_ABGR32,
	},
	{
		.name = "ARGB32",
		.fourcc = V4L2_PIX_FMT_ARGB32,
	},
	{
		.name = "ABGR32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ABGR32_SWAP,
	},
	{
		.name = "ARGB32_SWAP",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB32_SWAP,
	},
	{
		.name = "ARGB2101010",
		.fourcc = V4L2_PIX_FMT_DIP_ARGB2101010,
	},
	{
		.name = "BGR24",
		.fourcc = V4L2_PIX_FMT_BGR24,
	},
	{
		.name = "RGB565X",
		.fourcc = V4L2_PIX_FMT_RGB565X,
	},
	{
		.name = "YUYV",
		.fourcc = V4L2_PIX_FMT_YUYV,
	},
	{
		.name = "VYUY",
		.fourcc = V4L2_PIX_FMT_VYUY,
	},
	{
		.name = "YVYU",
		.fourcc = V4L2_PIX_FMT_YVYU,
	},
	{
		.name = "UYVY",
		.fourcc = V4L2_PIX_FMT_UYVY,
	},
	{
		.name = "YUYV_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YUYV_10BIT,
	},
	{
		.name = "YVYU_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_YVYU_10BIT,
	},
	{
		.name = "UYVY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_UYVY_10BIT,
	},
	{
		.name = "VYUY_10BIT",
		.fourcc = V4L2_PIX_FMT_DIP_VYUY_10BIT,
	},
	{
		.name = "NV16",
		.fourcc = V4L2_PIX_FMT_NV16,
	},
	{
		.name = "NV61",
		.fourcc = V4L2_PIX_FMT_NV61,
	},
	{
		.name = "8B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE,
	},
	{
		.name = "10B_BLK_TILE",
		.fourcc = V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE,
	},
	{
		.name = "NV12",
		.fourcc = V4L2_PIX_FMT_NV12,
	},
	{
		.name = "NV21",
		.fourcc = V4L2_PIX_FMT_NV21,
	},
	{
		.name = "MT21S10L",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10L,
	},
	{
		.name = "MT21S",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S,
	},
	{
		.name = "MT21S10T",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10T,
	},
	{
		.name = "MT21S10R",
		.fourcc = V4L2_PIX_FMT_DIP_MT21S10R,
	},
	{
		.name = "MT21C",
		.fourcc = V4L2_PIX_FMT_MT21C,
	},
	{
		.name = "MTKVSD8x4",
		.fourcc = V4L2_PIX_FMT_DIP_VSD8X4,
	},
	{
		.name = "MTKVSD8x2",
		.fourcc = V4L2_PIX_FMT_DIP_VSD8X2,
	},
};

static struct dip_frame def_frame = {
	.width      = DIP_DEFAULT_WIDTH,
	.height     = DIP_DEFAULT_HEIGHT,
	.c_width    = DIP_DEFAULT_WIDTH,
	.c_height   = DIP_DEFAULT_HEIGHT,
	.o_width    = 0,
	.o_height   = 0,
	.fmt        = &stDiprFmtList[0],
	.u32OutWidth      = DIP_DEFAULT_WIDTH,
	.u32OutHeight     = DIP_DEFAULT_HEIGHT,
	.colorspace = V4L2_COLORSPACE_REC709,
};

static int _FindMappingDIP(struct dip_dev *dev, EN_DIP_IDX *penDIPIdx)
{
	int ret = 0;

	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX)
		*penDIPIdx = E_DIP_IDX0;
	else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX)
		*penDIPIdx = E_DIP_IDX1;
	else if (dev->variant->u16DIPIdx == DIP2_WIN_IDX)
		*penDIPIdx = E_DIP_IDX2;
	else {
		ret = -ENODEV;
		v4l2_err(&dev->v4l2_dev, "%s,%d, Invalid DIP number\n",
			__func__, __LINE__);
	}

	return ret;
}

static bool _GetDIPWFmtAlignUnit(u32 fourcc, u16 *pu16SizeAlign, u16 *pu16PitchAlign)
{
	u16 u16SizeAlign = 0;
	u16 u16PitchAlign = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_DIP_ABGR32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB2101010:
		u16SizeAlign = ARGB32_HSIZE_ALIGN;
		u16PitchAlign = ARGB32_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_BGR24:
		u16SizeAlign = RGB24_HSIZE_ALIGN;
		u16PitchAlign = RGB24_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
		u16SizeAlign = RGB16_HSIZE_ALIGN;
		u16PitchAlign = RGB16_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
		u16SizeAlign = YUV422_10B_HSIZE_ALIGN;
		u16PitchAlign = YUV422_10B_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		u16SizeAlign = YUV422_SEP_HSIZE_ALIGN;
		u16PitchAlign = YUV422_SEP_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
		u16SizeAlign = YUV422_8BLKTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV422_8BLKTILE_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
		u16SizeAlign = YUV422_10BLKTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV422_10BLKTILE_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		u16SizeAlign = YUV420_LINEAR_HSIZE_ALIGN;
		u16PitchAlign = YUV420_LINEAR_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_MT21S10L:
		u16SizeAlign = YUV420_10BLINEAR_HSIZE_ALIGN;
		u16PitchAlign = YUV420_10BLINEAR_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_MT21S:
		u16SizeAlign = YUV420_8BTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV420_W_8BTILE_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		u16SizeAlign = YUV420_10BTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV420_W_10BTILE_PITCH_ALIGN;
		break;
	default:
		break;
	}
	*pu16SizeAlign = u16SizeAlign;
	*pu16PitchAlign = u16PitchAlign;

	return true;
}

static bool _GetDIPFmtAlignUnit(u32 fourcc, u16 *pu16SizeAlign, u16 *pu16PitchAlign)
{
	u16 u16SizeAlign = 0;
	u16 u16PitchAlign = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_DIP_ABGR32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB2101010:
		u16SizeAlign = ARGB32_HSIZE_ALIGN;
		u16PitchAlign = ARGB32_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_BGR24:
		u16SizeAlign = RGB24_HSIZE_ALIGN;
		u16PitchAlign = RGB24_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
		u16SizeAlign = RGB16_HSIZE_ALIGN;
		u16PitchAlign = RGB16_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
		u16SizeAlign = YUV422_10B_HSIZE_ALIGN;
		u16PitchAlign = YUV422_10B_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		u16SizeAlign = YUV422_SEP_HSIZE_ALIGN;
		u16PitchAlign = YUV422_SEP_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
		u16SizeAlign = YUV422_8BLKTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV422_8BLKTILE_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
		u16SizeAlign = YUV422_10BLKTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV422_10BLKTILE_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		u16SizeAlign = YUV420_LINEAR_HSIZE_ALIGN;
		u16PitchAlign = YUV420_LINEAR_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_MT21S10L:
		u16SizeAlign = YUV420_10BLINEAR_HSIZE_ALIGN;
		u16PitchAlign = YUV420_10BLINEAR_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_MT21S:
		u16SizeAlign = YUV420_8BTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV420_R_8BTILE_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		u16SizeAlign = YUV420_10BTILE_HSIZE_ALIGN;
		u16PitchAlign = YUV420_R_10BTILE_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_MT21C:
		u16SizeAlign = MT21C_HSIZE_ALIGN;
		u16PitchAlign = MT21C_PITCH_ALIGN;
		break;
	case V4L2_PIX_FMT_DIP_VSD8X4:
	case V4L2_PIX_FMT_DIP_VSD8X2:
		u16SizeAlign = VSD_HSIZE_ALIGN;
		u16PitchAlign = VSD_PITCH_ALIGN;
		break;
	default:
		break;
	}
	*pu16SizeAlign = u16SizeAlign;
	*pu16PitchAlign = u16PitchAlign;

	return true;
}

static u32 _GetDIPFmtPlaneCnt(u32 fourcc)
{
	u32 u32NPlanes = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_DIP_ABGR32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB2101010:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
	case V4L2_PIX_FMT_MT21C:
		u32NPlanes = 1;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_DIP_MT21S10L:
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
	case V4L2_PIX_FMT_DIP_VSD8X4:
	case V4L2_PIX_FMT_DIP_VSD8X2:
		u32NPlanes = 2;
		break;
	default:
		u32NPlanes = 1;
		break;
	}

	return u32NPlanes;
}

static u32 _GetDIPWFmtMaxWidth(struct dip_ctx *ctx, u32 fourcc, EN_DIP_IDX enDIPIdx)
{
	struct dip_dev *dev = ctx->dev;
	u32 u32MaxWidth = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_DIP_ABGR32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB2101010:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		u32MaxWidth = DIP_GNRL_FMT_WIDTH_MAX;
		break;
	case V4L2_PIX_FMT_DIP_MT21S10L:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		if (enDIPIdx == E_DIP_IDX0)
			u32MaxWidth = DIP0_420LINEAR_MAX_WIDTH;
		else if (enDIPIdx == E_DIP_IDX1)
			u32MaxWidth = DIP1_420LINEAR_MAX_WIDTH;
		else if (enDIPIdx == E_DIP_IDX2)
			u32MaxWidth = DIP2_420LINEAR_MAX_WIDTH;
		else
			u32MaxWidth = 0;
		break;
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		u32MaxWidth = dev->dipcap_dev.TileWidthMax;
		break;
	default:
		u32MaxWidth = 0;
		break;
	}

	return u32MaxWidth;
}

static u32 _GetDIPRFmtMaxWidth(struct dip_ctx *ctx, u32 fourcc)
{
	struct dip_dev *dev = ctx->dev;
	u32 u32MaxWidth = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_DIP_ABGR32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB2101010:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		u32MaxWidth = DIP_GNRL_FMT_WIDTH_MAX;
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_DIP_MT21S10L:
		u32MaxWidth = DIPR_420LINEAR_MAX_WIDTH;
		break;
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		u32MaxWidth = dev->dipcap_dev.TileWidthMax;
		break;
	case V4L2_PIX_FMT_MT21C:
	case V4L2_PIX_FMT_DIP_VSD8X4:
	case V4L2_PIX_FMT_DIP_VSD8X2:
		u32MaxWidth = DIP_B2R_WIDTH_MAX;
		break;
	default:
		u32MaxWidth = 0;
		break;
	}

	return u32MaxWidth;
}

static u32 _GetDIPDiDnrMaxWidth(EN_DIP_IDX enDIPIdx)
{
	u32 u32MaxWidth = 0;

	if (enDIPIdx == E_DIP_IDX0)
		u32MaxWidth = DIP0_DI_IN_WIDTH_MAX;
	else if (enDIPIdx == E_DIP_IDX1)
		u32MaxWidth = DIP1_DI_IN_WIDTH_MAX;
	else if (enDIPIdx == E_DIP_IDX2)
		u32MaxWidth = DIP2_DI_IN_WIDTH_MAX;
	else
		u32MaxWidth = 0;

	return u32MaxWidth;
}

static u16 _GetDIPDiDnrWidthAlign(EN_DIP_IDX enDIPIdx)
{
	u32 u32Align = 0;

	if (enDIPIdx == E_DIP_IDX0)
		u32Align = DIP0_DI_IN_WIDTH_ALIGN;
	else if (enDIPIdx == E_DIP_IDX1)
		u32Align = DIP1_DI_IN_WIDTH_ALIGN;
	else if (enDIPIdx == E_DIP_IDX2)
		u32Align = DIP2_DI_IN_WIDTH_ALIGN;
	else
		u32Align = 0;

	return u32Align;
}

static u32 _GetDIPHvspMaxWidth(EN_DIP_IDX enDIPIdx)
{
	u32 u32MaxWidth = 0;

	if (enDIPIdx == E_DIP_IDX0)
		u32MaxWidth = DIP0_HVSP_IN_WIDTH_MAX;
	else if (enDIPIdx == E_DIP_IDX1)
		u32MaxWidth = DIP1_HVSP_IN_WIDTH_MAX;
	else if (enDIPIdx == E_DIP_IDX2)
		u32MaxWidth = DIP2_HVSP_IN_WIDTH_MAX;
	else
		u32MaxWidth = 0;

	return u32MaxWidth;
}

static bool _IsSupportRWSepFmt(struct dip_ctx *ctx, u32 fourcc)
{
	struct dip_dev *dev = ctx->dev;
	bool bRet = false;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_DIP_ABGR32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB2101010:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		bRet = true;
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_DIP_MT21S10L:
		if (dev->dipcap_dev.u32RWSepVer == DIP_RWSEP_V0_VERSION)
			bRet = false;
		else
			bRet = true;
		break;
	case V4L2_PIX_FMT_MT21C:
	default:
		bRet = false;
		break;
	}

	return bRet;
}

bool _IsYUV420TileFormat(MS_U32 u32fourcc)
{
	bool bRet = false;

	switch (u32fourcc) {
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		bRet = true;
		break;
	default:
		bRet = false;
		break;
	}

	return bRet;
}

static u32 _GetDIPRWSepWidthAlign(struct dip_ctx *ctx, u32 fourcc)
{
	u32 u32RetAlign = 0;
	u16 u16SizeAlign = 0;
	u16 u16PitchAlign = 0;

	if (_IsSupportRWSepFmt(ctx, fourcc) == true) {
		if (_IsYUV420TileFormat(fourcc) == true) {
			u32RetAlign = DIP_RWSEP_420TILE_ALIGN;
		} else {
			_GetDIPFmtAlignUnit(fourcc, &u16SizeAlign, &u16PitchAlign);
			u32RetAlign = u16SizeAlign;
		}
	} else {
		u32RetAlign = 0;
	}

	return u32RetAlign;
}

static bool _GetFmtBpp(u32 fourcc, u32 *pu32Dividend, u32 *pu32Divisor)
{
	u16 u16Dividend = 0;
	u16 u16Divisor = 0;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_DIP_ABGR32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB32_SWAP:
	case V4L2_PIX_FMT_DIP_ARGB2101010:
		u16Dividend = BPP_4BYTE;
		u16Divisor = BPP_NO_MOD;
		break;
	case V4L2_PIX_FMT_BGR24:
		u16Dividend = BPP_3BYTE;
		u16Divisor = BPP_NO_MOD;
		break;
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
		u16Dividend = BPP_2BYTE;
		u16Divisor = BPP_NO_MOD;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_MT21C:
	case V4L2_PIX_FMT_DIP_VSD8X4:
	case V4L2_PIX_FMT_DIP_VSD8X2:
		u16Dividend = BPP_1BYTE;
		u16Divisor = BPP_NO_MOD;
		break;
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
		u16Dividend = BPP_YUV422_10B_DIV;
		u16Divisor = BPP_YUV422_10B_MOD;
		break;
	case V4L2_PIX_FMT_DIP_MT21S10L:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		u16Dividend = BPP_YUV420_10B_DIV;
		u16Divisor = BPP_YUV420_10B_MOD;
		break;
	default:
		break;
	}
	*pu32Dividend = u16Dividend;
	*pu32Divisor = u16Divisor;

	return true;
}

u32 CalFramePixel(u32 fourcc, u32 size)
{
	u32 u32PixelCnt = 0;
	u32 u32Dividend = 0;
	u32 u32Divisor = 0;

	_GetFmtBpp(fourcc, &u32Dividend, &u32Divisor);
	if ((u32Dividend == 0) || (u32Divisor == 0))
		return 0;

	u32PixelCnt = (size*u32Divisor)/u32Dividend;

	return u32PixelCnt;
}

u32 MapPixelToByte(u32 fourcc, u32 u32Pixel)
{
	u32 u32Byte = 0;
	u32 u32Dividend = 0;
	u32 u32Divisor = 0;

	_GetFmtBpp(fourcc, &u32Dividend, &u32Divisor);
	if ((u32Dividend == 0) || (u32Divisor == 0))
		return 0;

	u32Byte = (u32Pixel*u32Dividend)/u32Divisor;

	return u32Byte;
}

static bool _IsCompressFormat(u32 fourcc)
{
	bool bRet = false;

	switch (fourcc) {
	case V4L2_PIX_FMT_MT21C:
	case DIP_PIX_FMT_DIP_MT21CS:
	case DIP_PIX_FMT_DIP_MT21CS10T:
	case DIP_PIX_FMT_DIP_MT21CS10R:
	case DIP_PIX_FMT_DIP_MT21CS10TJ:
	case DIP_PIX_FMT_DIP_MT21CS10RJ:
		bRet = true;
		break;
	default:
		bRet = false;
		break;
	}

	return bRet;
};

static bool _IsYUVFormat(u32 u32fourcc)
{
	bool bRet = false;

	switch (u32fourcc) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
	case V4L2_PIX_FMT_DIP_MT21S10L:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
	case V4L2_PIX_FMT_MT21C:
	case V4L2_PIX_FMT_DIP_MT21CS:
	case V4L2_PIX_FMT_DIP_MT21CS10T:
	case V4L2_PIX_FMT_DIP_MT21CS10R:
	case V4L2_PIX_FMT_DIP_MT21CS10TJ:
	case V4L2_PIX_FMT_DIP_MT21CS10RJ:
		bRet = TRUE;
		break;
	default:
		bRet = FALSE;
		break;
	}

	return bRet;
}

enum v4l2_ycbcr_encoding _QueryYcbcrEnc(enum v4l2_colorspace colorspace,
	enum v4l2_ycbcr_encoding YCbCrEnc)
{
	enum v4l2_ycbcr_encoding RetYCbCrEnc = V4L2_YCBCR_ENC_DEFAULT;

	if (YCbCrEnc == V4L2_YCBCR_ENC_DEFAULT)
		RetYCbCrEnc = V4L2_MAP_YCBCR_ENC_DEFAULT(colorspace);
	else
		RetYCbCrEnc = YCbCrEnc;

	return RetYCbCrEnc;
}

enum v4l2_quantization _LookUpQuantization(enum v4l2_colorspace colorspace,
	enum v4l2_ycbcr_encoding ycbcr_enc,
	enum v4l2_quantization quantization,
	u8 is_rgb)
{
	enum v4l2_quantization RetQuan = V4L2_QUANTIZATION_DEFAULT;

	if (quantization == V4L2_QUANTIZATION_DEFAULT) {
		RetQuan = V4L2_MAP_QUANTIZATION_DEFAULT(is_rgb,
				colorspace, ycbcr_enc);
	} else {
		RetQuan = quantization;
	}

	return RetQuan;
}

EN_DIP_CSC_ENC _GetDIPCscEnc(enum v4l2_ycbcr_encoding SrcYCbCrEnc,
	enum v4l2_ycbcr_encoding DstYCbCrEnc)
{
	EN_DIP_CSC_ENC enCscEnc = E_DIPCSC_ENC_BT709;
	int Ret = 0;

	switch (SrcYCbCrEnc) {
	case V4L2_YCBCR_ENC_709:
	case V4L2_YCBCR_ENC_XV709:
		if ((DstYCbCrEnc == V4L2_YCBCR_ENC_709) ||
			(DstYCbCrEnc == V4L2_YCBCR_ENC_XV709)) {
			enCscEnc = E_DIPCSC_ENC_BT709;
		} else {
			Ret = (-1);
		}
		break;
	case V4L2_YCBCR_ENC_BT2020:
	case V4L2_YCBCR_ENC_BT2020_CONST_LUM:
		if ((DstYCbCrEnc == V4L2_YCBCR_ENC_BT2020) ||
			(DstYCbCrEnc == V4L2_YCBCR_ENC_BT2020_CONST_LUM)) {
			enCscEnc = E_DIPCSC_ENC_BT2020;
		} else {
			Ret = (-1);
		}
		break;

	case V4L2_YCBCR_ENC_601:
	case V4L2_YCBCR_ENC_XV601:
		if ((DstYCbCrEnc == V4L2_YCBCR_ENC_601) ||
			(DstYCbCrEnc == V4L2_YCBCR_ENC_XV601)) {
			enCscEnc = E_DIPCSC_ENC_BT601;
		} else {
			Ret = (-1);
		}
		break;
	default:
		Ret = (-1);
		break;
	}

	if (Ret != 0)
		enCscEnc = E_DIPCSC_ENC_MAX;

	return enCscEnc;
}

EN_DIP_CSC_RANGE _CalDIPCscRange(enum v4l2_quantization SrcQuan,
		enum v4l2_quantization DstQuan)
{
	EN_DIP_CSC_RANGE enCscRg = E_DIPCSC_RG_F2F;

	if (SrcQuan == V4L2_QUANTIZATION_FULL_RANGE) {
		if (DstQuan == V4L2_QUANTIZATION_FULL_RANGE)
			enCscRg = E_DIPCSC_RG_F2F;
		else
			enCscRg = E_DIPCSC_RG_F2L;
	} else {
		if (DstQuan == V4L2_QUANTIZATION_FULL_RANGE)
			enCscRg = E_DIPCSC_RG_L2F;
		else
			enCscRg = E_DIPCSC_RG_L2L;
	}

	return enCscRg;
}

EN_DIP_CSC_RANGE _GetDIPCscRange(enum v4l2_ycbcr_encoding SrcYCbCrEnc,
	enum v4l2_ycbcr_encoding DstYCbCrEnc,
	enum v4l2_quantization SrcQuan,
	enum v4l2_quantization DstQuan)
{
	EN_DIP_CSC_RANGE enCscRg = E_DIPCSC_RG_F2F;
	int Ret = 0;

	switch (SrcYCbCrEnc) {
	case V4L2_YCBCR_ENC_709:
		if (DstYCbCrEnc == V4L2_YCBCR_ENC_709) {
			enCscRg = _CalDIPCscRange(SrcQuan, DstQuan);
		} else if (DstYCbCrEnc == V4L2_YCBCR_ENC_XV709) {
			if (DstQuan == V4L2_QUANTIZATION_FULL_RANGE)
				enCscRg = E_DIPCSC_RG_L2F;
			else
				enCscRg = E_DIPCSC_RG_L2L;
		} else {
			Ret = (-1);
		}
		break;
	case V4L2_YCBCR_ENC_XV709:
		if (DstYCbCrEnc == V4L2_YCBCR_ENC_709) {
			if (DstQuan == V4L2_QUANTIZATION_FULL_RANGE)
				enCscRg = E_DIPCSC_RG_L2F;
			else
				enCscRg = E_DIPCSC_RG_L2L;
		} else if (DstYCbCrEnc == V4L2_YCBCR_ENC_XV709) {
			enCscRg = E_DIPCSC_RG_L2L;
		} else {
			Ret = (-1);
		}
		break;
	case V4L2_YCBCR_ENC_BT2020:
	case V4L2_YCBCR_ENC_BT2020_CONST_LUM:
		if ((DstYCbCrEnc == V4L2_YCBCR_ENC_BT2020) ||
			(DstYCbCrEnc == V4L2_YCBCR_ENC_BT2020_CONST_LUM)) {
			enCscRg = _CalDIPCscRange(SrcQuan, DstQuan);
		} else {
			Ret = (-1);
		}
		break;
	case V4L2_YCBCR_ENC_601:
		if (DstYCbCrEnc == V4L2_YCBCR_ENC_601) {
			enCscRg = _CalDIPCscRange(SrcQuan, DstQuan);
		} else if (DstYCbCrEnc == V4L2_YCBCR_ENC_XV601) {
			if (DstQuan == V4L2_QUANTIZATION_FULL_RANGE)
				enCscRg = E_DIPCSC_RG_L2F;
			else
				enCscRg = E_DIPCSC_RG_L2L;
		} else {
			Ret = (-1);
		}
		break;
	case V4L2_YCBCR_ENC_XV601:
		if (DstYCbCrEnc == V4L2_YCBCR_ENC_601) {
			if (DstQuan == V4L2_QUANTIZATION_FULL_RANGE)
				enCscRg = E_DIPCSC_RG_L2F;
			else
				enCscRg = E_DIPCSC_RG_L2L;
		} else if (DstYCbCrEnc == V4L2_YCBCR_ENC_XV601) {
			enCscRg = E_DIPCSC_RG_L2L;
		} else {
			Ret = (-1);
		}
		break;
	default:
		Ret = (-1);
		break;
	}

	if (Ret != 0)
		enCscRg = E_DIPCSC_RG_MAX;

	return enCscRg;
}

int _GetDIPCscMapping(struct dip_frame SrcFrm,
	struct ST_SRC_INFO *pstSrcInfo,
	struct dip_frame *pDstFrm,
	struct ST_CSC *pstCSC)
{
	enum v4l2_ycbcr_encoding SrcYCbCrEnc = V4L2_YCBCR_ENC_DEFAULT;
	enum v4l2_ycbcr_encoding DstYCbCrEnc = V4L2_YCBCR_ENC_DEFAULT;
	enum v4l2_quantization SrcQuan = V4L2_QUANTIZATION_DEFAULT;
	enum v4l2_quantization DstQuan = V4L2_QUANTIZATION_DEFAULT;
	EN_DIP_CSC_ENC enCscEnc = E_DIPCSC_ENC_BT709;
	EN_DIP_CSC_RANGE enCscRg = E_DIPCSC_RG_F2F;
	u8 IsRGB = 0;
	bool bDstIsYUV = false;

	SrcYCbCrEnc = _QueryYcbcrEnc(SrcFrm.colorspace, SrcFrm.ycbcr_enc);
	DstYCbCrEnc = _QueryYcbcrEnc(pDstFrm->colorspace, pDstFrm->ycbcr_enc);

	if (pstSrcInfo->enSrcClr == E_SRC_COLOR_RGB)
		IsRGB = 1;
	else
		IsRGB = 0;
	SrcQuan = _LookUpQuantization(SrcFrm.colorspace, SrcYCbCrEnc,
			SrcFrm.quantization, IsRGB);

	if (_IsYUVFormat(pDstFrm->fmt->fourcc) == true)
		IsRGB = 0;
	else
		IsRGB = 1;
	DstQuan = _LookUpQuantization(pDstFrm->colorspace, DstYCbCrEnc,
			pDstFrm->quantization, IsRGB);

	enCscEnc = _GetDIPCscEnc(SrcYCbCrEnc, DstYCbCrEnc);
	if (enCscEnc != E_DIPCSC_ENC_MAX)
		pstCSC->enCscEnc = enCscEnc;
	else
		return (-1);

	enCscRg = _GetDIPCscRange(SrcYCbCrEnc, DstYCbCrEnc, SrcQuan, DstQuan);
	if (enCscRg != E_DIPCSC_RG_MAX)
		pstCSC->enCscRg = enCscRg;
	else
		return (-1);

	bDstIsYUV = _IsYUVFormat(pDstFrm->fmt->fourcc);
	if (pstSrcInfo->enSrcClr == E_SRC_COLOR_RGB) {
		if (bDstIsYUV == true) {
			pstCSC->bEnaR2Y = true;
			pstCSC->bEnaY2R = false;
		} else {
			pstCSC->bEnaR2Y = false;
			pstCSC->bEnaY2R = false;
		}
	} else {
		if (bDstIsYUV == true) {
			pstCSC->bEnaR2Y = false;
			pstCSC->bEnaY2R = false;
		} else {
			pstCSC->bEnaR2Y = false;
			pstCSC->bEnaY2R = true;
		}
	}

	return 0;
}

static void _GetB2RAddrAlign(u32 fourcc, __u32 *pu32RetVal, __u32 *pu32RetValB)
{
	u16 u16LumaFB = 0;
	u16 u16ChromaFB = 0;
	u16 u16LumaBL = 0;
	u16 u16ChromaBL = 0;

	if (_IsCompressFormat(fourcc) == true) {
		u16LumaFB = B2R_LUMA_FB_CMPS_ADR_AL;
		u16ChromaFB = B2R_CHRO_FB_CMPS_ADR_AL;
		u16LumaBL = B2R_LUMA_BL_CMPS_ADR_AL;
		u16ChromaBL = B2R_CHRO_BL_CMPS_ADR_AL;
	} else {
		u16LumaFB = B2R_LUMA_FB_CMPS_ADR_AL;
		u16ChromaFB = B2R_CHRO_FB_CMPS_ADR_AL;
	}
	*pu32RetVal = u16ChromaFB << ChromaFB_BITSHIFT | u16LumaFB;
	*pu32RetValB = u16ChromaBL << ChromaBL_BITSHIFT | u16LumaBL;
}

static int _IsAlign(u32 u32Size, u16 u16AlignUnit)
{
	if (u16AlignUnit == 0) {
		DIP_ERR("[%s]Error:u16AlignUnit is zero\n", __func__);
		return (-1);
	}
	if ((u32Size % u16AlignUnit) == 0)
		return 0;
	else
		return (-1);
}

static int _AlignAdjust(u32 *pu32Width, u16 u16SizeAlign)
{
	u32 u32Width = *pu32Width;
	u32 u16Remainder = 0;

	if (u16SizeAlign == 0) {
		DIP_ERR("[%s]Error:u16SizeAlign is zero\n", __func__);
		return (-1);
	}
	u16Remainder = u32Width%u16SizeAlign;
	if (u16Remainder == 0)
		*pu32Width = u32Width;
	else
		*pu32Width = u32Width + u16SizeAlign - u16Remainder;

	return 0;
}

static bool _PitchConvert(u32 fourcc, u16 *pu16Pitch)
{
	u16 u16ConvertPitch = *pu16Pitch;
	u32 u32Dividend = 0;
	u32 u32Divisor = 0;

	_GetFmtBpp(fourcc, &u32Dividend, &u32Divisor);
	if ((u32Dividend == 0) || (u32Divisor == 0))
		return false;

	//bpp = u32Dividend/u32Divisor;
	u16ConvertPitch = (u16ConvertPitch*u32Divisor)/u32Dividend;

	*pu16Pitch = u16ConvertPitch;

	return true;
}

static bool _IsM2mCase(EN_DIP_SOURCE enDIPSrc)
{
	if ((enDIPSrc == E_DIP_SOURCE_DIPR)
			|| (enDIPSrc == E_DIP_SOURCE_DIPR_MFDEC)
			|| (enDIPSrc == E_DIP_SOURCE_B2R)) {
		return true;
	} else {
		return false;
	}
};

static bool _IsCapCase(EN_DIP_SOURCE enDIPSrc)
{
	if (_IsM2mCase(enDIPSrc) == true)
		return false;
	else
		return true;
};

static bool _IsSdcCapCase(EN_DIP_SOURCE enDIPSrc)
{
	if ((enDIPSrc == E_DIP_SOURCE_SRCCAP_MAIN)
		|| (enDIPSrc == E_DIP_SOURCE_SRCCAP_SUB)
		|| (enDIPSrc == E_DIP_SOURCE_PQ_START)
		|| (enDIPSrc == E_DIP_SOURCE_PQ_HDR)
		|| (enDIPSrc == E_DIP_SOURCE_PQ_SHARPNESS)
		|| (enDIPSrc == E_DIP_SOURCE_PQ_SCALING)
		|| (enDIPSrc == E_DIP_SOURCE_PQ_CHROMA_COMPENSAT)
		|| (enDIPSrc == E_DIP_SOURCE_PQ_BOOST)
		|| (enDIPSrc == E_DIP_SOURCE_PQ_COLOR_MANAGER)) {
		return true;
	} else {
		return false;
	}
};

static bool _IsSrcBufType(enum v4l2_buf_type type)
{
	if ((type == V4L2_BUF_TYPE_VIDEO_OUTPUT) || (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE))
		return true;
	else
		return false;
};

static bool _IsDstBufType(enum v4l2_buf_type type)
{
	if ((type == V4L2_BUF_TYPE_VIDEO_CAPTURE) || (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE))
		return true;
	else
		return false;
};

bool IsYUVFormat(MS_U32 u32fourcc)
{
	bool bRet = false;

	switch (u32fourcc) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_DIP_MT21S:
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
	case V4L2_PIX_FMT_DIP_MT21S10L:
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
		bRet = true;
		break;
	default:
		bRet = false;
		break;
	}

	return bRet;
}

bool IsYUV422Format(MS_U32 u32fourcc)
{
	bool bRet = false;

	switch (u32fourcc) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
	case V4L2_PIX_FMT_DIP_YUYV_10BIT:
	case V4L2_PIX_FMT_DIP_YVYU_10BIT:
	case V4L2_PIX_FMT_DIP_UYVY_10BIT:
	case V4L2_PIX_FMT_DIP_VYUY_10BIT:
		bRet = true;
		break;
	default:
		bRet = false;
		break;
	}

	return bRet;
}

static bool _FmtIsYuv10bTile(__u32 u32fourcc)
{
	bool ret = false;

	switch (u32fourcc) {
	case V4L2_PIX_FMT_DIP_MT21S10T:
	case V4L2_PIX_FMT_DIP_MT21S10R:
	case V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

static bool _ValidBufType(enum v4l2_buf_type type)
{
	if ((_IsSrcBufType(type) == true) || (_IsDstBufType(type) == true))
		return true;
	else
		return false;
};

static bool _GetSourceSelect(struct dip_ctx *ctx, EN_DIP_SRC_FROM *penSrcFrom)
{
	bool bRet = true;
	EN_DIP_SOURCE enCapSrc = E_DIP_SOURCE_SRCCAP_MAIN;

	if (ctx == NULL)
		return -EINVAL;

	enCapSrc = ctx->enSource;

	if (enCapSrc == E_DIP_SOURCE_SRCCAP_MAIN)
		*penSrcFrom = E_DIP_SRC_FROM_SRCCAP_MAIN;
	else if (enCapSrc == E_DIP_SOURCE_SRCCAP_SUB)
		*penSrcFrom = E_DIP_SRC_FROM_SRCCAP_SUB;
	else if (enCapSrc == E_DIP_SOURCE_PQ_START)
		*penSrcFrom = E_DIP_SRC_FROM_PQ_START;
	else if (enCapSrc == E_DIP_SOURCE_PQ_HDR)
		*penSrcFrom = E_DIP_SRC_FROM_PQ_HDR;
	else if (enCapSrc == E_DIP_SOURCE_PQ_SHARPNESS)
		*penSrcFrom = E_DIP_SRC_FROM_PQ_SHARPNESS;
	else if (enCapSrc == E_DIP_SOURCE_PQ_SCALING)
		*penSrcFrom = E_DIP_SRC_FROM_PQ_SCALING;
	else if (enCapSrc == E_DIP_SOURCE_PQ_CHROMA_COMPENSAT)
		*penSrcFrom = E_DIP_SRC_FROM_PQ_CHROMA_COMPENSAT;
	else if (enCapSrc == E_DIP_SOURCE_PQ_BOOST)
		*penSrcFrom = E_DIP_SRC_FROM_PQ_BOOST;
	else if (enCapSrc == E_DIP_SOURCE_PQ_COLOR_MANAGER)
		*penSrcFrom = E_DIP_SRC_FROM_PQ_COLOR_MANAGER;
	else if (enCapSrc == E_DIP_SOURCE_STREAM_ALL_VIDEO)
		*penSrcFrom = E_DIP_SRC_FROM_STREAM_ALL_VIDEO;
	else if (enCapSrc == E_DIP_SOURCE_STREAM_ALL_VIDEO_OSDB)
		*penSrcFrom = E_DIP_SRC_FROM_STREAM_ALL_VIDEO_OSDB;
	else if (enCapSrc == E_DIP_SOURCE_STREAM_POST)
		*penSrcFrom = E_DIP_SRC_FROM_STREAM_POST;
	else if (enCapSrc == E_DIP_SOURCE_OSD)
		*penSrcFrom = E_DIP_SRC_FROM_OSD;
	else if (enCapSrc == E_DIP_SOURCE_DIPR) {
		if ((_IsCompressFormat(ctx->in.fmt->fourcc) == true)
			|| (ctx->u8WorkingAid == E_AID_SDC)) {
			*penSrcFrom = E_DIP_SRC_FROM_B2R;
		} else {
			*penSrcFrom = E_DIP_SRC_FROM_DIPR;
		}
	} else if (enCapSrc == E_DIP_SOURCE_DIPR_MFDEC)
		*penSrcFrom = E_DIP_SRC_FROM_DIPR;
	else if (enCapSrc == E_DIP_SOURCE_B2R)
		*penSrcFrom = E_DIP_SRC_FROM_B2R;
	else
		return false;

	return bRet;
};

static bool _GetSourceInfo(struct dip_ctx *ctx, EN_DIP_SRC_FROM enSrcFrom,
	struct ST_SRC_INFO *pstSrcInfo)
{
	u32 u32fourcc = 0;
	bool bRet = true;
	bool bIsYUV = false;
	bool bIsYUV422 = false;

	if (enSrcFrom == E_DIP_SRC_FROM_DIPR) {
		u32fourcc = ctx->in.fmt->fourcc;
		if ((_IsCompressFormat(u32fourcc) == true)
			|| (ctx->u8WorkingAid == E_AID_SDC)) {
			pstSrcInfo->u8PixelNum = ctx->dev->dipcap_dev.u32B2rPixelNum;
			pstSrcInfo->enSrcClr = E_SRC_COLOR_YUV444;
		} else {
			bIsYUV = IsYUVFormat(u32fourcc);
			bIsYUV422 = IsYUV422Format(u32fourcc);
			pstSrcInfo->u8PixelNum = ctx->dev->dipcap_dev.u32DiprPixelNum;
			if (bIsYUV == TRUE) {
				if (bIsYUV422 == TRUE)
					pstSrcInfo->enSrcClr = E_SRC_COLOR_YUV422;
				else
					pstSrcInfo->enSrcClr = E_SRC_COLOR_YUV420;
			} else
				pstSrcInfo->enSrcClr = E_SRC_COLOR_RGB;
		}
		pstSrcInfo->u32Width = ctx->in.width;
		pstSrcInfo->u32Height = ctx->in.height;
		pstSrcInfo->u32DisplayWidth = ctx->in.width;
		pstSrcInfo->u32DisplayHeight = ctx->in.height;
	} else if (enSrcFrom == E_DIP_SRC_FROM_B2R) {
		pstSrcInfo->u8PixelNum = ctx->dev->dipcap_dev.u32B2rPixelNum;
		pstSrcInfo->enSrcClr = E_SRC_COLOR_YUV444;
		pstSrcInfo->u32Width = ctx->in.width;
		pstSrcInfo->u32Height = ctx->in.height;
		pstSrcInfo->u32DisplayWidth = ctx->in.width;
		pstSrcInfo->u32DisplayHeight = ctx->in.height;
	} else {
		pstSrcInfo->u8PixelNum = ctx->stSrcInfo.u8PixelNum;
		pstSrcInfo->enSrcClr = ctx->stSrcInfo.enSrcClr;
		pstSrcInfo->u32Width = ctx->stSrcInfo.u32Width;
		pstSrcInfo->u32Height = ctx->stSrcInfo.u32Height;
		pstSrcInfo->u32DisplayWidth = ctx->stSrcInfo.u32DisplayWidth;
		pstSrcInfo->u32DisplayHeight = ctx->stSrcInfo.u32DisplayHeight;
	}

	return bRet;
}

static int _ClkMappingIndex(u64 u64ClockRate, int *pParentClkIdx)
{
	int ret = 0;

	if (u64ClockRate == DIP_B2R_CLK_720MHZ)
		*pParentClkIdx = E_TOP_SUBDC_CLK_720MHZ;
	else if (u64ClockRate == DIP_B2R_CLK_630MHZ)
		*pParentClkIdx = E_TOP_SUBDC_CLK_630MHZ;
	else if (u64ClockRate == DIP_B2R_CLK_360MHZ)
		*pParentClkIdx = E_TOP_SUBDC_CLK_360MHZ;
	else if (u64ClockRate == DIP_B2R_CLK_312MHZ)
		*pParentClkIdx = E_TOP_SUBDC_CLK_312MHZ;
	else if (u64ClockRate == DIP_B2R_CLK_156MHZ)
		*pParentClkIdx = E_TOP_SUBDC_CLK_156MHZ;
	else if (u64ClockRate == DIP_B2R_CLK_144MHZ)
		*pParentClkIdx = E_TOP_SUBDC_CLK_144MHZ;
	else
		ret = (-1);

	return ret;
}

static int _SetB2rClk(struct dip_dev *dev, bool bEnable, u64 u64ClockRate)
{
	struct clk_hw *MuxClkHw;
	struct clk_hw *ParentClkGHw;
	int ret = 0;
	int ParentClkIdx = E_TOP_SUBDC_CLK_720MHZ;
	int Idx = 0;

	if (bEnable == true) {
		for (Idx = 0; Idx < E_B2R_CLK_MAX; Idx++) {
			ret = clk_enable(dev->B2rClk[Idx]);
			if (ret) {
				DIP_ERR("[%s]Enable clock failed %d\n", __func__, ret);
				return ret;
			}
		}
	} else {
		clk_disable(dev->B2rClk[E_B2R_CLK_TOP_SUBDC]);
	}

	_ClkMappingIndex(u64ClockRate, &ParentClkIdx);
	//get change_parent clk
	MuxClkHw = __clk_get_hw(dev->B2rClk[E_B2R_CLK_TOP_SUBDC]);
	ParentClkGHw = clk_hw_get_parent_by_index(MuxClkHw, ParentClkIdx);
	if (IS_ERR(ParentClkGHw)) {
		DIP_ERR("[%s]failed to get parent_clk_hw\n", __func__);
		return (-1);
	}

	//set_parent clk
	ret = clk_set_parent(dev->B2rClk[E_B2R_CLK_TOP_SUBDC], ParentClkGHw->clk);
	if (ret) {
		DIP_ERR("[%s]failed to set parent clock\n", __func__);
		return (-1);
	}

	return 0;
}

static bool _SetDIPClkParent(struct dip_dev *dev, EN_DIP_SRC_FROM enSrcFrom, bool bEnable)
{
	struct clk_hw *MuxClkHw;
	struct clk_hw *ParentClkGHw;
	struct clk *scip_dip_clk;
	int ret = 0;
	int ParentClkIdx = 0;

	if (enSrcFrom != E_DIP_SRC_FROM_DIPR) {
		if (dev == NULL)
			return false;
		scip_dip_clk = dev->scip_dip_clk;
		if (bEnable == true) {
			ret = clk_enable(scip_dip_clk);
			if (ret) {
				DIP_ERR("[%s]Enable clock failed %d\n", __func__, ret);
				return false;
			}
		} else {
			clk_disable(scip_dip_clk);
		}

		switch (enSrcFrom) {
		case E_DIP_SRC_FROM_SRCCAP_MAIN:
			ParentClkIdx = 0;
			break;
		case E_DIP_SRC_FROM_SRCCAP_SUB:
			ParentClkIdx = 1;
			break;
		case E_DIP_SRC_FROM_PQ_START:
		case E_DIP_SRC_FROM_PQ_HDR:
		case E_DIP_SRC_FROM_PQ_SHARPNESS:
			ParentClkIdx = 2;
			break;
		case E_DIP_SRC_FROM_PQ_SCALING:
		case E_DIP_SRC_FROM_PQ_CHROMA_COMPENSAT:
		case E_DIP_SRC_FROM_PQ_BOOST:
		case E_DIP_SRC_FROM_PQ_COLOR_MANAGER:
			ParentClkIdx = 3;
			break;
		case E_DIP_SRC_FROM_STREAM_ALL_VIDEO:
		case E_DIP_SRC_FROM_STREAM_ALL_VIDEO_OSDB:
		case E_DIP_SRC_FROM_STREAM_POST:
			ParentClkIdx = 5;
			break;
		case E_DIP_SRC_FROM_OSD:
			ParentClkIdx = 4;
			break;
		case E_DIP_SRC_FROM_B2R:
			ParentClkIdx = 6;
			break;
		default:
			return false;
		}

		//get change_parent clk
		MuxClkHw = __clk_get_hw(scip_dip_clk);
		ParentClkGHw = clk_hw_get_parent_by_index(MuxClkHw, ParentClkIdx);
		if (IS_ERR(ParentClkGHw) || (ParentClkGHw == NULL)) {
			DIP_ERR("[%s]failed to get parent_clk_hw\n", __func__);
			return false;
		}

		//set_parent clk
		ret = clk_set_parent(scip_dip_clk, ParentClkGHw->clk);
		if (ret) {
			DIP_ERR("[%s]failed to set parent clock\n", __func__);
			return false;
		}
	}

	return true;
}

static void b2r_clk_put(struct platform_device *pdev, struct dip_dev *dev)
{
	int i;

	for (i = 0; i < E_B2R_CLK_MAX; i++) {
		dev_err(&pdev->dev, "[%s]clock Index:%d\n", __func__, i);
		if (IS_ERR(dev->B2rClk[i])) {
			dev_err(&pdev->dev, "[%s]IS_ERR Idx:%d\n", __func__, i);
			continue;
		}
		clk_unprepare(dev->B2rClk[i]);
		clk_put(dev->B2rClk[i]);
		dev->B2rClk[i] = ERR_PTR(-EINVAL);
	}
}

static int b2r_clk_get(struct platform_device *pdev, struct dip_dev *dev)
{
	int i, ret;

	for (i = 0; i < E_B2R_CLK_MAX; i++) {
		dev->B2rClk[i] = devm_clk_get(&pdev->dev, b2r_clocks[i]);
		if (IS_ERR(dev->B2rClk[i])) {
			dev_err(&pdev->dev, "[%s]failed devm_clk_get Idx:%d\n", __func__, i);
			ret = PTR_ERR(dev->B2rClk[i]);
			goto b2rClkErr;
		}
		ret = clk_prepare(dev->B2rClk[i]);
		if (ret < 0) {
			dev_err(&pdev->dev, "[%s]failed clk_prepare Idx:%d\n", __func__, i);
			clk_put(dev->B2rClk[i]);
			dev->B2rClk[i] = ERR_PTR(-EINVAL);
			goto b2rClkErr;
		}
	}
	return 0;
b2rClkErr:
	b2r_clk_put(pdev, dev);
	return -ENXIO;
}

static int _SetIoBase(struct platform_device *pdev, MS_U8 u8IdxDts, MS_U8 u8IdxHwreg)
{
	int ret = 0;
	struct resource *res;
	void __iomem *ioDipBase;
	MS_U64 u64V_Addr = 0;

	dev_info(&pdev->dev, "u8IdxDts:%d, u8IdxHwreg:%d\n",
			u8IdxDts, u8IdxHwreg);

	if (KDrv_DIP_CheckIOBaseInit(u8IdxHwreg) == FALSE) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, u8IdxDts);
		if (!res) {
			dev_err(&pdev->dev, "failed get IORESOURCE_MEM u8IdxDts %d\n", u8IdxDts);
			ret = -ENXIO;
		} else {
			dev_info(&pdev->dev, "MEM u8IdxDts:%d, Size:0x%llx\n",
					u8IdxDts, resource_size(res));
			ioDipBase = devm_ioremap_resource(&pdev->dev, res);
			if (IS_ERR(ioDipBase)) {
				dev_err(&pdev->dev, "failed devm_ioremap_resource\n");
				ret = PTR_ERR(ioDipBase);
			} else {
				u64V_Addr = (MS_U64)ioDipBase;
				KDrv_DIP_SetIOBase(u8IdxHwreg,
						res->start, resource_size(res), u64V_Addr);
			}
		}
	}

	return ret;
}

static struct dip_fmt *_FindDipCapMpFmt(struct dip_dev *dev, struct v4l2_format *f)
{
	u32 u32FmtCnt = 0;
	unsigned int i;

	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX) {
		u32FmtCnt = SPT_FMT_CNT(stDip0FmtList);
		for (i = 0; i < u32FmtCnt; i++) {
			if (stDip0FmtList[i].fourcc == f->fmt.pix_mp.pixelformat)
				return &stDip0FmtList[i];
		}
	} else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX) {
		u32FmtCnt = SPT_FMT_CNT(stDip1FmtList);
		for (i = 0; i < u32FmtCnt; i++) {
			if (stDip1FmtList[i].fourcc == f->fmt.pix_mp.pixelformat)
				return &stDip1FmtList[i];
		}
	} else if (dev->variant->u16DIPIdx == DIP2_WIN_IDX) {
		u32FmtCnt = SPT_FMT_CNT(stDip2FmtList);
		for (i = 0; i < u32FmtCnt; i++) {
			if (stDip2FmtList[i].fourcc == f->fmt.pix_mp.pixelformat)
				return &stDip2FmtList[i];
		}
	}

	return NULL;
}

static struct dip_fmt *_FindDipOutMpFmt(struct v4l2_format *f)
{
	unsigned int i;

	for (i = 0; i < SPT_FMT_CNT(stDiprFmtList); i++) {
		if (stDiprFmtList[i].fourcc == f->fmt.pix_mp.pixelformat)
			return &stDiprFmtList[i];
	}
	return NULL;
}

static struct dip_frame *_GetDIPFrame(struct dip_ctx *ctx,
						enum v4l2_buf_type type)
{
	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		return &ctx->in;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		return &ctx->out;
	default:
		return ERR_PTR(-EINVAL);
	}
}

static int _ValidCrop(struct file *file, void *priv, struct v4l2_selection *pstSel)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *f;

	f = _GetDIPFrame(ctx, pstSel->type);
	if (IS_ERR(f))
		return PTR_ERR(f);

	if (pstSel->r.top < 0 || pstSel->r.left < 0) {
		v4l2_err(&dev->v4l2_dev,
			"doesn't support negative values for top & left\n");
		return -EINVAL;
	}

	return 0;
}

static int _dip_common_map_fd(int fd, void **va, u64 *size)
{
	int ret = 0;
	struct dma_buf *db = NULL;

	if ((size == NULL) || (fd == 0)) {
		DIP_ERR("Invalid input, fd=(%d), size is NULL?(%s)\n",
			fd, (size == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	if (*va != NULL) {
		kfree(*va);
		*va = NULL;
	}

	db = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(db)) {
		DIP_ERR("dma_buf_get fail, db=%p\n", db);
		return -EPERM;
	}

	*size = db->size;

	ret = dma_buf_begin_cpu_access(db, DMA_BIDIRECTIONAL);
	if (ret < 0) {
		DIP_ERR("dma_buf_begin_cpu_access fail\n");
		dma_buf_put(db);
		return -EPERM;
	}

	*va = dma_buf_vmap(db);

	if (*va == NULL) {
		dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
		DIP_ERR("dma_buf_vmap fail\n");
		dma_buf_put(db);
		return -EPERM;
	}

	dma_buf_put(db);

	return ret;
}

static int _dip_common_unmap_fd(int fd, void *va)
{
	struct dma_buf *db = NULL;

	if ((va == NULL) || (fd == 0)) {
		DIP_ERR("Invalid input, fd=(%d), va is NULL?(%s)\n",
			fd, (va == NULL)?"TRUE":"FALSE");
		return -EPERM;
	}

	db = dma_buf_get(fd);
	if (db == NULL) {
		DIP_ERR("dma_buf_get fail\n");
		return -1;
	}

	dma_buf_vunmap(db, va);
	dma_buf_end_cpu_access(db, DMA_BIDIRECTIONAL);
	dma_buf_put(db);

	return 0;
}

static int _dip_common_get_metaheader(
	enum EN_DIP_MTDT_TAG meta_tag,
	struct meta_header *meta_header)
{
	int ret = 0;

	if (meta_header == NULL) {
		DIP_ERR("Invalid input, meta_header == NULL\n");
		return -EPERM;
	}

	switch (meta_tag) {
	case E_DIP_MTDT_VDEC_FRM_TAG:
		meta_header->size = sizeof(struct vdec_dd_frm_info);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_VDEC_DD_FRM_INFO_TAG, TAG_LENGTH);
		break;
	case E_DIP_MTDT_VDEC_COMPRESS_TAG:
		meta_header->size = sizeof(struct vdec_dd_compress_info);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_VDEC_DD_COMPRESS_INFO_TAG, TAG_LENGTH);
		break;
	case E_DIP_MTDT_VDEC_COLOR_DESC_TAG:
		meta_header->size = sizeof(struct vdec_dd_color_desc);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_VDEC_DD_COLOR_DESC_TAG, TAG_LENGTH);
		break;
	case E_DIP_MTDT_VDEC_SVP_TAG:
		meta_header->size = sizeof(struct vdec_dd_svp_info);
		meta_header->ver = 0;
		strncpy(meta_header->tag, MTK_VDEC_DD_SVP_INFO_TAG, TAG_LENGTH);
		break;
	default:
		DIP_ERR("Invalid metadata tag:(%d)\n", meta_tag);
		ret = -EPERM;
		break;
	}

	return ret;
}

int dip_common_read_metadata(int meta_fd,
	enum EN_DIP_MTDT_TAG meta_tag, void *meta_content)
{
	int ret = 0;
	unsigned char meta_ret = 0;
	void *va = NULL;
	u64 size = 0;
	struct meta_buffer buffer;
	struct meta_header header;

	if (meta_content == NULL) {
		DIP_ERR("Invalid input, meta_content=NULL\n");
		return -EPERM;
	}

	memset(&buffer, 0, sizeof(struct meta_buffer));
	memset(&header, 0, sizeof(struct meta_header));

	ret = _dip_common_map_fd(meta_fd, &va, &size);
	if (ret) {
		DIP_ERR("trans fd to va fail\n");
		ret = -EPERM;
		goto out;
	}

	ret = _dip_common_get_metaheader(meta_tag, &header);
	if (ret) {
		DIP_ERR("get meta header fail\n");
		ret = -EPERM;
		goto out;
	}

	buffer.paddr = (unsigned char *)va;
	buffer.size = (unsigned int)size;
	meta_ret = query_metadata_header(&buffer, &header);
	if (!meta_ret) {
		DIP_ERR("query metadata header fail\n");
		ret = -EPERM;
		goto out;
	}

	meta_ret = query_metadata_content(&buffer, &header, meta_content);
	if (!meta_ret) {
		DIP_ERR("query metadata content fail\n");
		ret = -EPERM;
		goto out;
	}

out:
	_dip_common_unmap_fd(meta_fd, va);
	return ret;
}

int _GetCapPtType(EN_DIP_SOURCE enDIPSrc, EN_CAPPT_TYPE *penCappt)
{
	EN_CAPPT_TYPE enCappt = E_CAPPT_VDEC;
	int ret = 0;

	switch (enDIPSrc) {
	case E_DIP_SOURCE_DIPR:
	case E_DIP_SOURCE_B2R:
		enCappt = E_CAPPT_VDEC;
		break;
	case E_DIP_SOURCE_SRCCAP_MAIN:
	case E_DIP_SOURCE_SRCCAP_SUB:
		enCappt = E_CAPPT_SRC;
		break;
	case E_DIP_SOURCE_PQ_START:
	case E_DIP_SOURCE_PQ_HDR:
	case E_DIP_SOURCE_PQ_SHARPNESS:
	case E_DIP_SOURCE_PQ_SCALING:
	case E_DIP_SOURCE_PQ_CHROMA_COMPENSAT:
	case E_DIP_SOURCE_PQ_BOOST:
	case E_DIP_SOURCE_PQ_COLOR_MANAGER:
		enCappt = E_CAPPT_PQ;
		break;
	case E_DIP_SOURCE_STREAM_ALL_VIDEO:
	case E_DIP_SOURCE_STREAM_ALL_VIDEO_OSDB:
	case E_DIP_SOURCE_STREAM_POST:
		enCappt = E_CAPPT_RNDR;
		break;
	case E_DIP_SOURCE_OSD:
		enCappt = E_CAPPT_OSD;
		break;
	default:
		enCappt = E_CAPPT_VDEC;
		ret = (-1);
		break;
	}
	*penCappt = enCappt;

	return ret;
}

static int _GetVdecMtdtFormat(struct vdec_dd_frm_info *pvdec_frm_md, u32 *pu32fourcc)
{
	struct fmt_modifier *pmodifier = NULL;

	if (pvdec_frm_md == NULL)
		return -EINVAL;

	if (pu32fourcc == NULL)
		return -EINVAL;

	pmodifier = &(pvdec_frm_md->frm_src_info[0].modifier);

	if (pmodifier->compress == 1) {
		if (pvdec_frm_md->bitdepth == 10) {
			if (pmodifier->jump == 1) {
				if (pmodifier->tile == 1)
					*pu32fourcc = DIP_PIX_FMT_DIP_MT21CS10TJ;
				else
					*pu32fourcc = DIP_PIX_FMT_DIP_MT21CS10RJ;
			} else {
				if (pmodifier->tile == 1)
					*pu32fourcc = DIP_PIX_FMT_DIP_MT21CS10T;
				else
					*pu32fourcc = DIP_PIX_FMT_DIP_MT21CS10R;
			}
		} else {
			*pu32fourcc = DIP_PIX_FMT_DIP_MT21CS;
		}
	} else {
		if (pvdec_frm_md->bitdepth == 10) {
			if (pmodifier->tile == 1)
				*pu32fourcc = DIP_PIX_FMT_DIP_MT21S10T;
			else
				*pu32fourcc = DIP_PIX_FMT_DIP_MT21S10R;
		} else {
			*pu32fourcc = DIP_PIX_FMT_DIP_MT21S;
		}
	}

	return 0;
}

static int GetVdecMetadataInfo(struct dip_ctx *ctx, struct ST_DIP_METADATA *pstMetaData)
{
	struct vdec_dd_frm_info vdec_frm_md;
	struct vdec_dd_compress_info vdec_cprs_md;
	struct vdec_dd_svp_info vdec_svp;
	struct ST_DIP_B2R_MTDT *pstB2rMtdt = &(ctx->stB2rMtdt);
	int ret = 0;
	int MetaFd = 0;

	MetaFd = pstMetaData->meta_fd;
	ret = dip_common_read_metadata(MetaFd, E_DIP_MTDT_VDEC_COMPRESS_TAG,
			&vdec_cprs_md);
	if (ret)
		return ret;
	if ((vdec_cprs_md.flag & VDEC_COMPRESS_FLAG_UNCOMPRESS) == 0) {
		ret = dip_common_read_metadata(MetaFd, E_DIP_MTDT_VDEC_FRM_TAG,
				&vdec_frm_md);
		if (ret)
			return ret;
		ret = _GetVdecMtdtFormat(&vdec_frm_md, &(pstB2rMtdt->u32fourcc));
		if (ret)
			return ret;
		pstB2rMtdt->u32BitlenPitch = vdec_frm_md.frm_src_info[0].pitch;
		pstB2rMtdt->u64LumaFbOft = vdec_frm_md.luma_data_info[0].offset;
		pstB2rMtdt->u32LumaFbSz = vdec_frm_md.luma_data_info[0].size;
		pstB2rMtdt->u64ChromaFbOft = vdec_frm_md.chroma_data_info[0].offset;
		pstB2rMtdt->u32ChromaFbSz = vdec_frm_md.chroma_data_info[0].size;
		pstB2rMtdt->u64LumaBitlenOft = vdec_cprs_md.luma_data_info.offset;
		pstB2rMtdt->u32LumaBitlenSz = vdec_cprs_md.luma_data_info.size;
		pstB2rMtdt->u64ChromaBitlenOft =
			vdec_cprs_md.chroma_data_info.offset;
		pstB2rMtdt->u32ChromaBitlenSz =
			vdec_cprs_md.chroma_data_info.size;
		pstB2rMtdt->u32Enable = 1;
	} else {
		pstB2rMtdt->u32Enable = 0;
	}
	ret = dip_common_read_metadata(MetaFd, E_DIP_MTDT_VDEC_SVP_TAG, &vdec_svp);
	if (ret) {
		ctx->u8aid = 0;
		ctx->u32pipeline_id = 0;
		return ret;
	}
	ctx->u8aid = vdec_svp.aid;
	ctx->u32pipeline_id = vdec_svp.pipeline_id;

	return ret;
}

static int GetMetadataInfo(struct dip_ctx *ctx, struct ST_DIP_METADATA *pstMetaData)
{
	struct dip_dev *dev = ctx->dev;
	EN_CAPPT_TYPE enCappt = E_CAPPT_VDEC;
	int ret = 0;

	if (pstMetaData->u16Enable) {
		ret = _GetCapPtType(ctx->enSource, &enCappt);
		if (ret)
			return ret;
		if (enCappt == E_CAPPT_VDEC) {
			ret = GetVdecMetadataInfo(ctx, pstMetaData);
		} else {
			v4l2_err(&dev->v4l2_dev, "%s,Not Support enSource:%d metadata\n",
				__func__, ctx->enSource);
		}
	} else {
		ctx->u8aid = 0;
		ctx->u32pipeline_id = 0;
	}

	return ret;
}

int _DipB2rChkSize(u32 u32Width, u32 u32Height)
{
	if ((u32Width < DIP_B2R_WIDTH_ALIGN)
		|| (u32Height == 0)) {
		return -EINVAL;
	}

	return 0;
}

int _DipB2rChkTiming(struct ST_B2R_INFO *pstB2rInfo)
{
	if ((pstB2rInfo->u32Fps < DIP_B2R_MIN_FPS)
		|| (pstB2rInfo->u32DeWidth < DIP_B2R_WIDTH_ALIGN)
		|| (pstB2rInfo->u32DeHeight == 0)) {
		return -EINVAL;
	}

	return 0;
}

int _DipB2rGetClk(struct ST_B2R_INFO *pstB2rInfo)
{
	u64 clock_rate_est;
	u64 htt_est;
	u64 vtt_est;
	u32 u32B2RPixelNum = 0;
	int ret = 0;

	ret = _DipB2rChkTiming(pstB2rInfo);
	if (ret) {
		DIP_ERR("[%s]Check B2R Timing Fail\n", __func__);
		return ret;
	}

	u32B2RPixelNum = pstB2rInfo->u32B2rPixelNum;
	if (u32B2RPixelNum == 0) {
		DIP_ERR("[%s]Invalid B2R Engine Pixel\n", __func__);
		return (-1);
	}

	htt_est = ALIGN_UPTO_32(pstB2rInfo->u32DeWidth/u32B2RPixelNum) + DIP_B2R_HBLK_SZ;
	vtt_est = ALIGN_UPTO_32(pstB2rInfo->u32DeHeight) + DIP_B2R_VBLK_SZ;
	clock_rate_est = (htt_est * vtt_est * pstB2rInfo->u32Fps);

	if (clock_rate_est < DIP_B2R_CLK_144MHZ) {
		pstB2rInfo->u64ClockRate = DIP_B2R_CLK_144MHZ;
	} else if (clock_rate_est < DIP_B2R_CLK_156MHZ) {
		pstB2rInfo->u64ClockRate = DIP_B2R_CLK_156MHZ;
	} else if (clock_rate_est < DIP_B2R_CLK_312MHZ) {
		pstB2rInfo->u64ClockRate = DIP_B2R_CLK_312MHZ;
	} else if (clock_rate_est < DIP_B2R_CLK_360MHZ) {
		pstB2rInfo->u64ClockRate = DIP_B2R_CLK_360MHZ;
	} else if (clock_rate_est < DIP_B2R_CLK_630MHZ) {
		pstB2rInfo->u64ClockRate = DIP_B2R_CLK_630MHZ;
	} else if (clock_rate_est <= DIP_B2R_CLK_720MHZ) {
		pstB2rInfo->u64ClockRate = DIP_B2R_CLK_720MHZ;
	} else {
		pstB2rInfo->u64ClockRate = DIP_B2R_CLK_720MHZ;
		DIP_ERR("[%s]Request clock over highest clock\n", __func__);
		return -EINVAL;
	}

	return 0;
}

int _DipB2rGenTiming(struct ST_B2R_INFO *pstB2rInfo,
	struct ST_B2R_TIMING *pstB2rTiming)
{
	u64 u64Htt = 0;
	u64 u64Vtt = 0;
	u32 u32B2RPixelNum = 0;
	u16 u16VBlankSz = 0;
	int ret = 0;

	ret = _DipB2rChkTiming(pstB2rInfo);
	if (ret) {
		DIP_ERR("[%s]Check B2R Timing Fail\n", __func__);
		return ret;
	}

	u32B2RPixelNum = pstB2rInfo->u32B2rPixelNum;
	if (u32B2RPixelNum == 0) {
		DIP_ERR("[%s]Invalid B2R Engine Pixel\n", __func__);
		return (-1);
	}

	u64Vtt = ALIGN_UPTO_32(pstB2rInfo->u32DeHeight) + DIP_B2R_VBLK_SZ;
	u64Htt = pstB2rInfo->u64ClockRate;
	do_div(u64Htt, pstB2rInfo->u32Fps);
	do_div(u64Htt, u64Vtt);

	pstB2rTiming->V_TotalCount = u64Vtt;
	if (pstB2rInfo->u32B2rHttPixelUnit)
		pstB2rTiming->H_TotalCount = u64Htt * u32B2RPixelNum;
	else
		pstB2rTiming->H_TotalCount = u64Htt;
	pstB2rTiming->VBlank0_Start = DIP_B2R_VBLK_STR;
	u16VBlankSz = pstB2rTiming->V_TotalCount - pstB2rInfo->u32DeHeight;
	pstB2rTiming->VBlank0_End = u16VBlankSz - pstB2rTiming->VBlank0_Start;
	pstB2rTiming->TopField_VS = pstB2rTiming->VBlank0_Start
		+ (u16VBlankSz >> 1);
	pstB2rTiming->TopField_Start = pstB2rTiming->TopField_VS;
	pstB2rTiming->HActive_Start = (pstB2rTiming->H_TotalCount
		- (ALIGN_UPTO_32(pstB2rInfo->u32DeWidth) / u32B2RPixelNum));
	pstB2rTiming->HImg_Start = pstB2rTiming->HActive_Start;
	pstB2rTiming->VImg_Start0 = pstB2rTiming->VBlank0_End;

	pstB2rTiming->VBlank1_Start = pstB2rTiming->VBlank0_Start;
	pstB2rTiming->VBlank1_End = pstB2rTiming->VBlank0_End;
	pstB2rTiming->BottomField_Start = pstB2rTiming->TopField_Start;
	pstB2rTiming->BottomField_VS = pstB2rTiming->TopField_VS;
	pstB2rTiming->VImg_Start1 = pstB2rTiming->VImg_Start0;

	return 0;
}

static unsigned long mtk_get_addr(struct vb2_buffer *vb, unsigned int plane_no)
{
	struct vb2_queue *vq = NULL;
	struct dip_ctx *ctx = NULL;
	struct dip_dev *dev = NULL;
	unsigned long *addr = NULL;

	if (vb == NULL) {
		DIP_ERR("%s, vb is NULL\n", __func__);
		return 0;
	}

	vq = vb->vb2_queue;
	ctx = vq->drv_priv;
	dev = ctx->dev;

	addr = vb2_plane_cookie(vb, plane_no);
	if (addr == NULL)
		return INVALID_ADDR;

	return *addr;
}

int _DipDevRunSetSrcAddr(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *src_vb = NULL;
	struct dip_fmt *Srcfmt = NULL;
	struct ST_B2R_ADDR stB2rAddr;
	struct ST_DIP_B2R_MTDT *pstB2rMtdt = &(ctx->stB2rMtdt);
	struct ST_DIP_MFDEC *pstMfdecSet = &(ctx->stMfdecSet);
	u64 pu64SrcPhyAddr[MAX_PLANE_NUM] = {0};
	u32 u32Idx = 0;
	u32 u32NPlanes = 0;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	src_vb = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	if (src_vb == NULL)
		return (-1);

	Srcfmt = ctx->in.fmt;
	u32NPlanes = _GetDIPFmtPlaneCnt(Srcfmt->fourcc);
	if (u32NPlanes > MAX_PLANE_NUM) {
		v4l2_err(&dev->v4l2_dev, "%s,%d,NPlanes(%d) too much\n",
				__func__, __LINE__, u32NPlanes);
		return (-1);
	}
	for (u32Idx = 0; u32Idx < u32NPlanes ; u32Idx++) {
		pu64SrcPhyAddr[u32Idx] = mtk_get_addr(&(src_vb->vb2_buf), u32Idx);
		if (pu64SrcPhyAddr[u32Idx] == INVALID_ADDR) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, address[%d] invalid\n",
				__func__, __LINE__, u32Idx);
			return -EINVAL;
		}
		if (ctx->u32SrcBufMode == E_BUF_IOVA_MODE)
			pu64SrcPhyAddr[u32Idx] += IOVA_START_ADDR;
		DIP_DBG_BUF("%s, Addr[%d]:0x%llx\n",
					__func__, u32Idx, pu64SrcPhyAddr[u32Idx]);
	}

	if (ctx->enSource == E_DIP_SOURCE_DIPR) {
		if (_IsCompressFormat(Srcfmt->fourcc) == true) {
			if (pstB2rMtdt->u32Enable == 0) {
				v4l2_err(&dev->v4l2_dev, "%s,%d,Invalid Metadata\n",
					__func__, __LINE__);
				return (-1);
			}
			stB2rAddr.u64LumaFb =
				pu64SrcPhyAddr[0] + pstB2rMtdt->u64LumaFbOft;
			stB2rAddr.u64ChromaFb =
				pu64SrcPhyAddr[0] + pstB2rMtdt->u64ChromaFbOft;
			stB2rAddr.u64LumaBlnFb =
				pu64SrcPhyAddr[0] + pstB2rMtdt->u64LumaBitlenOft;
			stB2rAddr.u64ChromaBlnFb =
				pu64SrcPhyAddr[0] + pstB2rMtdt->u64ChromaBitlenOft;
			stB2rAddr.u8Addrshift = B2R_CMPS_ADDR_OFT;
			KDrv_DIP_B2R_SetAddr(&stB2rAddr);
		} else {
			KDrv_DIP_SetSrcAddr(u32NPlanes, pu64SrcPhyAddr, enDIPIdx);
		}
	} else if (ctx->enSource == E_DIP_SOURCE_B2R) {
		stB2rAddr.u64LumaFb = pu64SrcPhyAddr[0];
		stB2rAddr.u64ChromaFb = pu64SrcPhyAddr[1];
		if (_IsCompressFormat(Srcfmt->fourcc) == true) {
			stB2rAddr.u64LumaBlnFb = pu64SrcPhyAddr[2];
			stB2rAddr.u64ChromaBlnFb = pu64SrcPhyAddr[3];
			stB2rAddr.u8Addrshift = B2R_CMPS_ADDR_OFT;
		} else {
			stB2rAddr.u8Addrshift = B2R_UNCMPS_ADDR_OFT;
		}
		KDrv_DIP_B2R_SetAddr(&stB2rAddr);
		if (ctx->stDiNrInfo.u32OpMode == E_DIP_MODE_DI) {
			if ((src_vb->vb2_buf.index % MOD2) == 0)
				KDrv_DIP_B2R_SetFiled(E_B2R_FIELD_TOP);
			else
				KDrv_DIP_B2R_SetFiled(E_B2R_FIELD_BOTTOM);
		}
	} else {
		pstMfdecSet->u64BitlenBase = pu64SrcPhyAddr[0];
		if (pstMfdecSet->u16SimMode != 0) {
			pstMfdecSet->u64LumaBase = pu64SrcPhyAddr[1];
			pstMfdecSet->u64ChromaBase = pu64SrcPhyAddr[2];
		}
		KDrv_DIP_SetMFDEC(pstMfdecSet, enDIPIdx);
	}

	return 0;
}

int _DipDevRunSetDstAddr(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *dst_vb = NULL;
	struct dip_fmt *Dstfmt = NULL;
	u64 pu64DstPhyAddr[MAX_PLANE_NUM] = {0};
	u32 u32Idx = 0;
	u32 u32NPlanes = 0;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	dst_vb = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	if (dst_vb == NULL) {
		ctx->u32M2mStat = ST_OFF;
		return (-1);
	}

	Dstfmt = ctx->out.fmt;
	u32NPlanes = _GetDIPFmtPlaneCnt(Dstfmt->fourcc);
	if (u32NPlanes > MAX_PLANE_NUM) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, NPlanes(%d) too much\n",
			__func__, __LINE__, u32NPlanes);
		return (-1);
	}
	for (u32Idx = 0; u32Idx < u32NPlanes ; u32Idx++) {
		pu64DstPhyAddr[u32Idx] = mtk_get_addr(&(dst_vb->vb2_buf), u32Idx);
		if (pu64DstPhyAddr[u32Idx] == INVALID_ADDR) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, address[%d] invalid\n",
				__func__, __LINE__, u32Idx);
			return -EINVAL;
		}
		if (ctx->u32DstBufMode == E_BUF_IOVA_MODE)
			pu64DstPhyAddr[u32Idx] += IOVA_START_ADDR;
		DIP_DBG_BUF("%s, Addr[%d]:0x%llx\n",
					__func__, u32Idx, pu64DstPhyAddr[u32Idx]);
	}
	KDrv_DIP_SetDstAddr(u32NPlanes, pu64DstPhyAddr, enDIPIdx);

	return 0;
}

int _DipDevRunFire(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	struct dip_fmt *Srcfmt = NULL;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	Srcfmt = ctx->in.fmt;
	if (ctx->enSource == E_DIP_SOURCE_DIPR) {
		if ((_IsCompressFormat(Srcfmt->fourcc) == true)
			|| (ctx->u8WorkingAid == E_AID_SDC)) {
			if (dev->dipcap_dev.u32B2R) {
				KDrv_DIP_B2R_Ena(true);
				KDrv_DIP_Trigger(enDIPIdx);
			} else {
				v4l2_err(&dev->v4l2_dev, "%s,%d, Not Support B2R\n",
					__func__, __LINE__);
				ret = (-1);
			}
		} else {
			KDrv_DIP_DIPR_Trigger(enDIPIdx);
		}
	} else if (ctx->enSource == E_DIP_SOURCE_B2R) {
		if (dev->dipcap_dev.u32B2R) {
			if ((ctx->u32CapStat & ST_CONT) == 0)
				KDrv_DIP_Trigger(enDIPIdx);
			KDrv_DIP_B2R_Ena(true);
		} else {
			v4l2_err(&dev->v4l2_dev, "%s,%d, Not Support B2R\n",
				__func__, __LINE__);
			ret = (-1);
		}
	}

	return 0;
}

int _DipAllocateBuffer(struct dip_ctx *ctx,
		unsigned int buf_tag,
		dma_addr_t *piova,
		size_t size)
{
	struct dip_dev *dipdev = ctx->dev;
	struct device *dev = dipdev->dev;
	int ret = 0;
	unsigned long dma_attrs = 0;
	void *cookie;

	if (buf_tag == 0) {
		v4l2_err(&(dipdev->v4l2_dev), "Invalid buf tag\n");
		return -1;
	}

	if (dma_get_mask(dev) < DMA_BIT_MASK(34)) {
		v4l2_err(&(dipdev->v4l2_dev), "IOMMU isn't registered\n");
		return -1;
	}

	if (!dma_supported(dev, 0)) {
		v4l2_err(&(dipdev->v4l2_dev), "IOMMU is not supported\n");
		return -1;
	}

	dma_attrs = buf_tag << IOMMU_BUF_TAG_SHIFT;
	dma_attrs |= DMA_ATTR_WRITE_COMBINE;
	cookie = dma_alloc_attrs(dev, size,
					piova, GFP_KERNEL,
					dma_attrs);
	if (!cookie) {
		v4l2_err(&(dipdev->v4l2_dev), "failed to allocate %zx byte dma buffer", size);
		return (-1);
	}
	gstDiNrBufInfo.DmaAddr = *piova;
	gstDiNrBufInfo.u32Size = size;
	gstDiNrBufInfo.dma_attrs = dma_attrs;
	gstDiNrBufInfo.virtAddr = cookie;
	ret = dma_mapping_error(dev, *piova);
	if (ret) {
		v4l2_err(&(dipdev->v4l2_dev), "dma_alloc_attrs fail\n");
		return ret;
	}

	return ret;
}

int _DipSetSource(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *DipFrm;
	struct ST_DIP_B2R_MTDT *pstB2rMtdt = &(ctx->stB2rMtdt);
	struct ST_B2R_TIMING stB2rTiming = {0};
	struct ST_B2R_INFO stB2rInfo = {0};
	struct vb2_v4l2_buffer *src_vb = NULL;
	struct ST_DIPR_SRC_WIN stDiprSrcWin = {0};
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;
	u32 u32MaxWidth = 0;
	u16 u16SizeAlign = 0;
	u16 u16PitchAlign = 0;
	u16 u16CovertPitch = 0;

	DipFrm = _GetDIPFrame(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	if (IS_ERR(DipFrm))
		return PTR_ERR(DipFrm);

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	DIP_DBG_API("%s, AID Type:%d\n", __func__, ctx->u8aid);

	_GetDIPFmtAlignUnit(DipFrm->fmt->fourcc, &u16SizeAlign, &u16PitchAlign);
	ret = _IsAlign(DipFrm->width, u16SizeAlign);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "[%s,%d]Width(%u) Not Align(%u)\n",
			__func__, __LINE__, DipFrm->width, u16SizeAlign);
		_AlignAdjust(&(DipFrm->width), u16SizeAlign);
	}

	u16PitchAlign = MapPixelToByte(DipFrm->fmt->fourcc, u16PitchAlign);
	ret = _IsAlign(DipFrm->u32BytesPerLine[0], u16PitchAlign);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "[%s,%d]Pitch(%u) Not Align(%u)\n",
			__func__, __LINE__, DipFrm->u32BytesPerLine[0], u16PitchAlign);
		_AlignAdjust(&(DipFrm->u32BytesPerLine[0]), u16PitchAlign);
	}
	u16CovertPitch = DipFrm->u32BytesPerLine[0];
	_PitchConvert(DipFrm->fmt->fourcc, &u16CovertPitch);

	u32MaxWidth = _GetDIPRFmtMaxWidth(ctx, DipFrm->fmt->fourcc);
	if (DipFrm->width > u32MaxWidth) {
		if ((dev->dipcap_dev.u32RWSep == 0) ||
				(_IsSupportRWSepFmt(ctx, DipFrm->fmt->fourcc) == false)) {
			v4l2_err(&dev->v4l2_dev, "%s, %d, Out Of Spec Width(%d), Input Max Width(%d)",
				__func__, __LINE__, DipFrm->width, u32MaxWidth);
			return (-1);
		}
	}

	src_vb = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);

	if ((_IsCompressFormat(DipFrm->fmt->fourcc) == true)
		|| (ctx->enSource == E_DIP_SOURCE_B2R)
		|| (ctx->u8WorkingAid == E_AID_SDC)) {
		if (dev->dipcap_dev.u32B2R) {
			ret = _DipB2rChkSize(DipFrm->width, DipFrm->height);
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s, %d, InValid B2R Size",
						__func__, __LINE__);
				return (-1);
			}
			KDrv_DIP_B2R_Init();
			/* check input parameters */
			stB2rInfo.u32DeWidth = DipFrm->width;
			stB2rInfo.u32DeHeight = DipFrm->height;
			stB2rInfo.u32B2rPixelNum = dev->dipcap_dev.u32B2rPixelNum;
			stB2rInfo.u32B2rHttPixelUnit = dev->dipcap_dev.u32B2rHttPixelUnit;
			stB2rInfo.u32Fps = DIP_B2R_DEF_FPS;
			ret = _DipB2rGetClk(&stB2rInfo);
			if (ret)
				return ret;
			ret = _SetB2rClk(dev, true, stB2rInfo.u64ClockRate);
			if (ret)
				return ret;
			ret = _DipB2rGenTiming(&stB2rInfo, &stB2rTiming);
			if (ret)
				return ret;
			KDrv_DIP_B2R_SetTiming(&stB2rTiming);
			if (_IsCompressFormat(DipFrm->fmt->fourcc) == true) {
				if (pstB2rMtdt->u32Enable == 0) {
					v4l2_err(&dev->v4l2_dev, "%s, %d, InValid Metadata data ",
							__func__, __LINE__);
					return (-1);
				}
				KDrv_DIP_B2R_SetFmt(pstB2rMtdt->u32fourcc);
				KDrv_DIP_B2R_SetFiled(E_B2R_FIELD_PRO);
				KDrv_DIP_B2R_SetSize(DipFrm->width, DipFrm->height,
						pstB2rMtdt->u32BitlenPitch);
			} else {
				KDrv_DIP_B2R_SetFmt(DipFrm->fmt->fourcc);
				if (ctx->stDiNrInfo.u32OpMode == E_DIP_MODE_DI)
					KDrv_DIP_B2R_SetFiled(E_B2R_FIELD_TOP);
				else
					KDrv_DIP_B2R_SetFiled(E_B2R_FIELD_PRO);
				KDrv_DIP_B2R_SetSize(DipFrm->width,
						DipFrm->height,
						u16CovertPitch);
			}
			KDrv_DIP_SetAID(ctx->u8aid, E_HW_B2R, enDIPIdx);
		} else {
			v4l2_err(&dev->v4l2_dev, "%s, %d, Not Support B2R",
					__func__, __LINE__);
			return (-1);
		}
	} else {
		memset(&stDiprSrcWin, 0, sizeof(stDiprSrcWin));
		stDiprSrcWin.u16Width = DipFrm->width;
		stDiprSrcWin.u16Height = DipFrm->height;
		stDiprSrcWin.u16Pitch = u16CovertPitch;
		stDiprSrcWin.u32fourcc = DipFrm->fmt->fourcc;
		KDrv_DIP_DIPR_SetSrcWin(&stDiprSrcWin, enDIPIdx);
		KDrv_DIP_SetAID(ctx->u8aid, E_HW_DIPR, enDIPIdx);
	}

	return 0;
}

int _DipDiNrSetting(struct dip_ctx *ctx, unsigned int type)
{
	struct dip_dev *dev = ctx->dev;
	struct ST_DIP_DINR *pstDiNrInfo = &(ctx->stDiNrInfo);
	struct ST_OPMODE stOpMode = {0};
	struct dip_frame *DipFrm;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;

	DipFrm = _GetDIPFrame(ctx, type);
	if (IS_ERR(DipFrm))
		return PTR_ERR(DipFrm);

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	if (ctx->stDiNrInfo.u32OpMode) {
		stOpMode.enOpMode = ctx->stDiNrInfo.u32OpMode;
		stOpMode.u16Width = pstDiNrInfo->u32InputWidth;
		stOpMode.u16Height = pstDiNrInfo->u32InputHeight;
		stOpMode.u64Addr = gstDiNrBufInfo.DmaAddr;
		KDrv_DIP_SetOpMode(&stOpMode, enDIPIdx);
	} else {
		stOpMode.enOpMode = E_DIP_NORMAL;
		KDrv_DIP_SetOpMode(&stOpMode, enDIPIdx);
	}

	return 0;
}

int _DipScalingSetting(struct dip_ctx *ctx, unsigned int type)
{
	struct dip_dev *dev = ctx->dev;
	struct ST_SCALING *pstScalingDown = &(ctx->stScalingDown);
	struct ST_SCALING *pstScalingHVSP = &(ctx->stScalingHVSP);
	struct ST_RWSEP_INFO *pstRwSep = &(ctx->stRwSep);
	struct dip_frame *DipFrm;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;

	DipFrm = _GetDIPFrame(ctx, type);
	if (IS_ERR(DipFrm))
		return PTR_ERR(DipFrm);

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	KDrv_DIP_SetScaling(pstScalingDown, enDIPIdx);
	KDrv_DIP_SetHVSP(pstScalingHVSP, enDIPIdx);
	KDrv_DIP_SetRWSeparate(pstRwSep, enDIPIdx);

	return 0;
}

static int _DipSetOutputSize(struct dip_ctx *ctx,
	u32 u32OutWidth,
	u32 u32OutHeight,
	u32 u16CovertPitch)
{
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *pDstFrm = NULL;
	struct ST_RES_WIN stResWin = {0};
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;
	u32 u32NPlanes = 0;
	u32 u32Idx = 0;
	u32 u32BufSzNeed = 0;
	u32 u32PerPlaneHeight = 0;
	u32 u32WriteHeight = 0;

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	pDstFrm = _GetDIPFrame(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	if (IS_ERR(pDstFrm)) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, Invalid Frame Type\n",
				__func__, __LINE__);
		return (-1);
	}

	if (ctx->stScalingHVSP.u8HScalingEna == 1) {
		if (u32OutWidth > ctx->stScalingHVSP.u32OutputWidth) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, output width%d over scaling out%d\n",
					__func__, __LINE__,
					u32OutWidth,
					ctx->stScalingHVSP.u32OutputWidth);
			u32OutWidth = ctx->stScalingHVSP.u32OutputWidth;
			ret = (-EINVAL);
		}
	}

	if (ctx->stScalingHVSP.u8VScalingEna == 1) {
		if (u32OutHeight > ctx->stScalingHVSP.u32OutputHeight) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, output height%d over scaling out%d\n",
					__func__, __LINE__,
					u32OutHeight,
					ctx->stScalingHVSP.u32OutputHeight);
			u32OutHeight = ctx->stScalingHVSP.u32OutputHeight;
			ret = (-EINVAL);
		}
	}

	u32NPlanes = _GetDIPFmtPlaneCnt(pDstFrm->fmt->fourcc);
	u32WriteHeight = u32OutHeight;
	for (u32Idx = 0; u32Idx < u32NPlanes; u32Idx++) {
		if (u32Idx == 0)
			u32BufSzNeed = u16CovertPitch*u32OutHeight;
		else
			u32BufSzNeed = (u16CovertPitch*u32OutHeight)>>1;
		if (u32BufSzNeed > pDstFrm->size[u32Idx]) {
			v4l2_err(&dev->v4l2_dev, "[%s,%d]Plane:%d, Output Size(0x%x) is not Enough(0x%x)\n",
				__func__, __LINE__, u32Idx, pDstFrm->size[u32Idx], u32BufSzNeed);
			u32PerPlaneHeight = pDstFrm->size[u32Idx]/u16CovertPitch;
			if (_IsYUV420TileFormat(pDstFrm->fmt->fourcc) == true) {
				u32PerPlaneHeight = u32PerPlaneHeight/YUV420TL_OUTBUF_HEIGHT_ALIGN;
				u32PerPlaneHeight = u32PerPlaneHeight*YUV420TL_OUTBUF_HEIGHT_ALIGN;
			}
			if (u32WriteHeight > u32PerPlaneHeight) {
				v4l2_err(&dev->v4l2_dev, "[%s,%d]Plane:%d, Adjust Output Height:%d to %d\n",
					__func__, __LINE__,
					u32Idx, u32WriteHeight, u32PerPlaneHeight);
				u32WriteHeight = u32PerPlaneHeight;
			}
		}
	}

	stResWin.u32Width = u32OutWidth;
	stResWin.u32Height = u32WriteHeight;
	stResWin.u32Pitch = u16CovertPitch;
	KDrv_DIP_SetWriteProperty(&stResWin, pDstFrm->fmt->fourcc, enDIPIdx);

	return ret;
}

int _DipWorkCapFire(struct dip_ctx *ctx, unsigned int type)
{
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *pDstFrm = _GetDIPFrame(ctx, type);
	struct dip_fmt *pDstfmt = pDstFrm->fmt;
	struct vb2_v4l2_buffer *dst_vb = NULL;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;
	u64 pu64DstPhyAddr[MAX_PLANE_NUM] = {0};
	u32 u32Idx = 0;
	u32 u32NPlanes = 0;
	u32 u32PixelCnt = 0;

	if (IS_ERR(pDstFrm))
		return PTR_ERR(pDstFrm);

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	//set address
	dst_vb = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	u32NPlanes = _GetDIPFmtPlaneCnt(pDstfmt->fourcc);
	if (u32NPlanes > MAX_PLANE_NUM) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, NPlanes(%d) too much\n",
			__func__, __LINE__, u32NPlanes);
		return -1;
	}
	for (u32Idx = 0; u32Idx < u32NPlanes ; u32Idx++) {
		pu64DstPhyAddr[u32Idx] = mtk_get_addr(&(dst_vb->vb2_buf), u32Idx);
		if (pu64DstPhyAddr[u32Idx] == INVALID_ADDR) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, address[%d] invalid\n",
				__func__, __LINE__, u32Idx);
			return -EINVAL;
		}
		if (ctx->u32DstBufMode == E_BUF_IOVA_MODE)
			pu64DstPhyAddr[u32Idx] += IOVA_START_ADDR;
		DIP_DBG_BUF("%s, Addr[%d]:0x%llx\n",
					__func__, u32Idx, pu64DstPhyAddr[u32Idx]);
	}
	KDrv_DIP_SetDstAddr(u32NPlanes, pu64DstPhyAddr, enDIPIdx);
	if ((ctx->u32CapStat & ST_CONT) == 0) {
		KDrv_DIP_Cont_Ena(false, enDIPIdx);
		KDrv_DIP_FrameCnt(1, 0, enDIPIdx);
		KDrv_DIP_Ena(true, enDIPIdx);
		KDrv_DIP_Trigger(enDIPIdx);
	} else {
		if ((ctx->u32InFps == ctx->u32OutFps) ||
			(ctx->u32InFps == 0) ||
			(ctx->u32OutFps == 0))
			KDrv_DIP_FrameRateCtrl(false, ctx->u32InFps,
						ctx->u32OutFps, enDIPIdx);
		else
			KDrv_DIP_FrameRateCtrl(true, ctx->u32InFps,
						ctx->u32OutFps, enDIPIdx);
		KDrv_DIP_Cont_Ena(true, enDIPIdx);
		u32PixelCnt = CalFramePixel(pDstfmt->fourcc, pDstFrm->size[0]);
		KDrv_DIP_FrameCnt(ctx->u8ContiBufCnt, u32PixelCnt, enDIPIdx);
		KDrv_DIP_Ena(true, enDIPIdx);
	}

	return 0;
}

int _DipSecureCheck(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	EN_AID enAid = E_AID_NS;
	int ret = 0;

	if (ctx->u8aid == E_AID_NS) {
		if (ctx->u32OutputSecutiry == 0)
			enAid = E_AID_NS;
		else
			enAid = E_AID_S;
	} else if (ctx->u8aid == E_AID_S) {
		enAid = E_AID_S;
		if (ctx->u32OutputSecutiry == 0) {
			if (dev->DebugMode == 0)
				ret = -EPERM;
		}
	} else if (ctx->u8aid == E_AID_SDC) {
		if (ctx->u32OutputSecutiry == 0) {
			if (dev->DebugMode == 1) {
				enAid = E_AID_S;
			} else {
				if (dev->SDCSupport) {
					enAid = E_AID_SDC;
				} else {
					v4l2_err(&dev->v4l2_dev, "Not Support Secure Capture\n");
					enAid = E_AID_S;
					ret = -EPERM;
				}
			}
		} else {
			enAid = E_AID_S;
		}
	}
	ctx->u8WorkingAid = enAid;

	return ret;
}

int _DipTeeSdcCapture(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	int ret = 0;

	DIP_DBG_API("%s, SDC Capture Case\n", __func__);

	if (dev->SDCSupport == 0) {
		v4l2_err(&dev->v4l2_dev, "%s, Not Support secure capture\n", __func__);
		return -EPERM;
	}

	ret = mtk_dip_svc_init(ctx);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "mtk_dip_svc_init Fail\n");
		return -EPERM;
	}
	ret = mtk_dip_svc_enable(ctx, 1);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "mtk_dip_svc_enable Fail\n");
		return -EPERM;
	}

	return ret;
}

int _DipTeeSdcMem2Mem(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *src_vb = NULL;
	struct dip_fmt *Srcfmt = NULL;
	struct ST_DIP_B2R_MTDT *pstB2rMtdt = &(ctx->stB2rMtdt);
	u64 u64SrcPhyAddr = 0;
	u32 u32Idx = 0;
	u32 u32NPlanes = 0;
	int ret = 0;

	DIP_DBG_API("%s, SDC Memory to Memory Case\n", __func__);

	if (dev->SDCSupport == 0) {
		v4l2_err(&dev->v4l2_dev, "%s, Not Support secure capture\n", __func__);
		return -EPERM;
	}

	ret = mtk_dip_svc_init(ctx);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "mtk_dip_svc_init Fail\n");
		return -EPERM;
	}
	src_vb = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	if (src_vb == NULL) {
		v4l2_err(&dev->v4l2_dev, "%s, v4l2_m2m_next_src_buf Fail\n", __func__);
		return (-1);
	}

	Srcfmt = ctx->in.fmt;
	if (_IsCompressFormat(Srcfmt->fourcc) == true) {
		if (pstB2rMtdt->u32Enable == 0) {
			v4l2_err(&dev->v4l2_dev, "%s,%d,Invalid Metadata\n",
				__func__, __LINE__);
			return (-1);
		}
		u64SrcPhyAddr = mtk_get_addr(&(src_vb->vb2_buf), 0);
		if (u64SrcPhyAddr == INVALID_ADDR) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, address invalid\n",
				__func__, __LINE__);
			return -EINVAL;
		}
		if (ctx->u32SrcBufMode == E_BUF_IOVA_MODE)
			u64SrcPhyAddr += IOVA_START_ADDR;
		DIP_DBG_BUF("%s, compress base Addr:0x%llx\n", __func__, u64SrcPhyAddr);
		ctx->secure_info.u64InputAddr[E_SVC_IN_ADDR_Y] =
			u64SrcPhyAddr + pstB2rMtdt->u64LumaFbOft;
		ctx->secure_info.u64InputAddr[E_SVC_IN_ADDR_C] =
			u64SrcPhyAddr + pstB2rMtdt->u64ChromaFbOft;
		ctx->secure_info.u64InputAddr[E_SVC_IN_ADDR_YBLN] =
			u64SrcPhyAddr + pstB2rMtdt->u64LumaBitlenOft;
		ctx->secure_info.u64InputAddr[E_SVC_IN_ADDR_CBLN] =
			u64SrcPhyAddr + pstB2rMtdt->u64ChromaBitlenOft;
		ctx->secure_info.u32PlaneCnt = E_SVC_IN_ADDR_MAX;
		ctx->secure_info.u32BufCnt = 1;
		ctx->secure_info.u32InputSize[0] =
			pstB2rMtdt->u64ChromaBitlenOft - pstB2rMtdt->u64LumaFbOft +
			pstB2rMtdt->u32ChromaBitlenSz;
		ctx->secure_info.u8InAddrshift = B2R_CMPS_ADDR_OFT;
	} else {
		u32NPlanes = _GetDIPFmtPlaneCnt(Srcfmt->fourcc);
		if (u32NPlanes > MAX_PLANE_NUM) {
			v4l2_err(&dev->v4l2_dev, "%s,%d,NPlanes(%d) too much\n",
					__func__, __LINE__, u32NPlanes);
			return (-1);
		}
		ctx->secure_info.u32PlaneCnt = u32NPlanes;
		ctx->secure_info.u32BufCnt = u32NPlanes;
		for (u32Idx = 0; u32Idx < MAX_PLANE_NUM ; u32Idx++) {
			if (u32Idx < u32NPlanes) {
				u64SrcPhyAddr = mtk_get_addr(&(src_vb->vb2_buf), u32Idx);
				if (u64SrcPhyAddr == INVALID_ADDR) {
					v4l2_err(&dev->v4l2_dev, "%s,%d, address[%d] invalid\n",
						__func__, __LINE__, u32Idx);
					return -EINVAL;
				}
				if (ctx->u32SrcBufMode == E_BUF_IOVA_MODE)
					u64SrcPhyAddr += IOVA_START_ADDR;
			} else {
				u64SrcPhyAddr = 0;
			}
			ctx->secure_info.u64InputAddr[u32Idx] = u64SrcPhyAddr;
			ctx->secure_info.u32InputSize[u32Idx] = ctx->out.size[u32Idx];
			DIP_DBG_BUF("%s, Addr[%d]:0x%llx, size[%d]:0x%x\n",
				__func__, u32Idx, ctx->secure_info.u64InputAddr[u32Idx],
				u32Idx, ctx->secure_info.u32InputSize[u32Idx]);
		}
		ctx->secure_info.u8InAddrshift = B2R_UNCMPS_ADDR_OFT;
	}
	ret = mtk_dip_svc_enable(ctx, 1);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "mtk_dip_svc_enable Fail\n");
		return -EPERM;
	}

	return ret;
}

int _SetCapTsSetting(struct dip_ctx *ctx, EN_DIP_SRC_FROM enSrcFrom, EN_DIP_IDX enDIPIdx)
{
	EN_CAPPT_TYPE enCappt = E_CAPPT_VDEC;
	int ret = 0;

	ret = _GetCapPtType(ctx->enSource, &enCappt);
	if (ret == 0)
		return ret;

	if (enCappt == E_CAPPT_PQ) {
		KDrv_DIP_SetWinId(ctx->stWinid.u8Enable, ctx->stWinid.u16WinId, enDIPIdx);
		KDrv_DIP_SetSrcHblanking(DIP_PQ_HBLANKING, enSrcFrom);
	} else {
		KDrv_DIP_SetWinId(0, 0, enDIPIdx);
	}

	return ret;
}

int _DipReeCapture(struct dip_ctx *ctx, unsigned int type)
{
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *pDstFrm = _GetDIPFrame(ctx, type);
	struct dip_fmt *pDstfmt = pDstFrm->fmt;
	struct dip_fmt *pSrcfmt = ctx->in.fmt;
	struct ST_SRC_INFO stSrcInfo = {0};
	struct ST_CROP_WIN stCropWin = {0};
	struct ST_MUX_INFO stMuxInfo = {0};
	struct ST_CSC stCSC = {0};
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	EN_DIP_SRC_FROM enSrcFrom = E_DIP_SRC_FROM_MAX;
	int ret = 0;
	u32 u32MaxWidth = 0;
	u32 u32OutWidth = 0;
	u32 u32OutHeight = 0;
	u16 u16SizeAlign = 0;
	u16 u16PitchAlign = 0;
	u16 u16CovertPitch = 0;
	bool bRet = false;

	if (IS_ERR(pDstFrm))
		return PTR_ERR(pDstFrm);

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	KDrv_DIP_Init(enDIPIdx);

	_GetSourceSelect(ctx, &enSrcFrom);
	_GetSourceInfo(ctx, enSrcFrom, &stSrcInfo);
	KDrv_DIP_SetSourceInfo(&stSrcInfo, enSrcFrom, enDIPIdx);

	ret = _SetCapTsSetting(ctx, enSrcFrom, enDIPIdx);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s, SetCapTsSetting Fail\n", __func__);
		return ret;
	}

	bRet = _SetDIPClkParent(dev, enSrcFrom, true);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "_SetDIPClkParent Fail\n");
		return (-1);
	}
	stMuxInfo.enSrcFrom = enSrcFrom;
	stMuxInfo.u32DipPixelNum = dev->dipcap_dev.u32DipPixelNum;
	stMuxInfo.u32FrontPack = dev->dipcap_dev.u32FrontPack;
	KDrv_DIP_SetMux(&stMuxInfo, enDIPIdx);

	KDrv_DIP_SetAlpha(ctx->alpha, enDIPIdx);

	KDrv_DIP_SetMirror(ctx->HMirror, ctx->VMirror, enDIPIdx);

	stCropWin.u32XStart = pDstFrm->o_width;
	stCropWin.u32YStart = pDstFrm->o_height;
	stCropWin.u32CropWidth = pDstFrm->c_width;
	stCropWin.u32CropHeight = pDstFrm->c_height;
	KDrv_DIP_SetCrop(&stCropWin, enDIPIdx);

	ret = _DipDiNrSetting(ctx, type);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, Error in DI setting\n",
				__func__, __LINE__);
	}

	ret = _DipScalingSetting(ctx, type);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, Error in Scaling setting\n",
				__func__, __LINE__);
	}

	if (dev->dipcap_dev.u32Rotate) {
		if (ctx->u32Rotation == ROT90_ANGLE) {
			KDrv_DIP_SetRotation(E_DIP_ROT_90, enDIPIdx);
			u32OutWidth = pDstFrm->u32OutHeight;
			u32OutHeight = pDstFrm->u32OutWidth;
		} else if (ctx->u32Rotation == ROT270_ANGLE) {
			KDrv_DIP_SetRotation(E_DIP_ROT_270, enDIPIdx);
			u32OutWidth = pDstFrm->u32OutHeight;
			u32OutHeight = pDstFrm->u32OutWidth;
		} else {
			KDrv_DIP_SetRotation(E_DIP_ROT_0, enDIPIdx);
			u32OutWidth = pDstFrm->u32OutWidth;
			u32OutHeight = pDstFrm->u32OutHeight;
		}
	} else {
		u32OutWidth = pDstFrm->u32OutWidth;
		u32OutHeight = pDstFrm->u32OutHeight;
	}

	_GetDIPWFmtAlignUnit(pDstfmt->fourcc, &u16SizeAlign, &u16PitchAlign);

	ret = _IsAlign(u32OutWidth, u16SizeAlign);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "[%s,%d]Width(%u) Not Align(%u)\n",
			__func__, __LINE__, u32OutWidth, u16SizeAlign);
		_AlignAdjust(&u32OutWidth, u16SizeAlign);
	}

	u16PitchAlign = MapPixelToByte(pDstfmt->fourcc, u16PitchAlign);
	ret = _IsAlign(pDstFrm->u32BytesPerLine[0], u16PitchAlign);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "[%s,%d]Pitch(%u) Not Align(%u)\n",
			__func__, __LINE__, pDstFrm->u32BytesPerLine[0], u16PitchAlign);
		_AlignAdjust(&(pDstFrm->u32BytesPerLine[0]), u16PitchAlign);
	}
	u16CovertPitch = pDstFrm->u32BytesPerLine[0];
	_PitchConvert(pDstfmt->fourcc, &u16CovertPitch);

	u32MaxWidth = _GetDIPWFmtMaxWidth(ctx, pDstfmt->fourcc, enDIPIdx);
	if (u32OutWidth > u32MaxWidth) {
		if ((dev->dipcap_dev.u32RWSep == 0) ||
			(_IsSupportRWSepFmt(ctx, pDstfmt->fourcc) == false)) {
			v4l2_err(&dev->v4l2_dev, "%s, %d, Out Of Spec Width(%d), Output Max Width(%d)",
				__func__, __LINE__, u32OutWidth, u32MaxWidth);
			return -EINVAL;
		}
	}
	ret = _DipSetOutputSize(ctx, u32OutWidth, u32OutHeight, u16CovertPitch);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "[%s,%d]Invalid Output Size:%d, %d, %d\n",
			__func__, __LINE__, u32OutWidth, u32OutHeight, u16CovertPitch);
		return -EINVAL;
	}

	ret = _GetDIPCscMapping(ctx->in, &stSrcInfo, pDstFrm, &stCSC);
	if (ret)
		return -EINVAL;
	KDrv_DIP_SetCSC(&stCSC, enDIPIdx);

	if ((_IsCapCase(ctx->enSource)) == true) {
		_DipWorkCapFire(ctx, type);
	} else {
		if ((_IsCompressFormat(pSrcfmt->fourcc) == true)
			|| (ctx->u8WorkingAid == E_AID_SDC))
			KDrv_DIP_Cont_Ena(false, enDIPIdx);
		else
			KDrv_DIP_Cont_Ena(true, enDIPIdx);
		KDrv_DIP_FrameRateCtrl(false, 1, 1, enDIPIdx);
		KDrv_DIP_FrameCnt(1, 0, enDIPIdx);
		KDrv_DIP_Ena(true, enDIPIdx);
	}

	return 0;
}

int _DipWorkSetting(struct dip_ctx *ctx, unsigned int type)
{
	struct dip_dev *dev = ctx->dev;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	if (ctx->u8WorkingAid > ctx->u8aid)
		KDrv_DIP_SetAID(ctx->u8WorkingAid, E_HW_DIPW, enDIPIdx);

	ret = _DipReeCapture(ctx, type);
	if (ret)
		return ret;

	return 0;
}

static int dip_queue_setup(struct vb2_queue *vq,
				unsigned int *nbuffers, unsigned int *nplanes,
				unsigned int sizes[], struct device *alloc_devs[])
{
	struct dip_ctx *ctx = vb2_get_drv_priv(vq);
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *f = _GetDIPFrame(ctx, vq->type);
	int idx;
	u32 u32NPlanes = 0;

	if (IS_ERR(f)) {
		v4l2_err(&dev->v4l2_dev, "[%s]dip_frame IS_ERR\n", __func__);
		return PTR_ERR(f);
	}

	u32NPlanes = _GetDIPFmtPlaneCnt(f->fmt->fourcc);
	if (u32NPlanes == 0) {
		v4l2_err(&dev->v4l2_dev, "[%s]format 0x%x plane count is zero\n",
			__func__, f->fmt->fourcc);
		return -EINVAL;
	}
	*nplanes = u32NPlanes;

	for (idx = 0; idx < *nplanes; idx++) {
		sizes[idx] = f->size[idx];
		DIP_DBG_BUF("%s, idx:%d, Sizes:0x%x, Plane:%d\n",
			__func__, idx, sizes[idx], *nplanes);
		if (sizes[idx] == 0) {
			v4l2_err(&dev->v4l2_dev, "[%s]plane[%d] size is zero\n", __func__, idx);
			return -EINVAL;
		}
	}

	return 0;
}

static int dip_buf_prepare(struct vb2_buffer *vb)
{
	struct dip_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *f = _GetDIPFrame(ctx, vb->vb2_queue->type);
	unsigned int idx = 0;
	u32 u32NPlanes = 0;

	if (IS_ERR(f)) {
		v4l2_err(&dev->v4l2_dev, "[%s]dip_frame IS_ERR\n", __func__);
		return PTR_ERR(f);
	}

	u32NPlanes = _GetDIPFmtPlaneCnt(f->fmt->fourcc);

	for (idx = 0 ; idx < u32NPlanes; idx++) {
		DIP_DBG_BUF("%s, Idx:%d, Fsize:0x%x, Plane:%d\n",
			__func__, idx, f->size[idx], u32NPlanes);
		vb2_set_plane_payload(vb, idx, f->size[idx]);
	}
	return 0;
}

static void dip_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct dip_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

int _DipPreCalDiScalUpDnFromDipr(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;
	struct ST_SCALING *pstScalingDown = &(ctx->stScalingDown);
	struct ST_SCALING *pstScalingHVSP = &(ctx->stScalingHVSP);
	struct ST_DIP_DINR *pstDiNrInfo = &(ctx->stDiNrInfo);
	struct ST_RWSEP_INFO *pstRwSep = &(ctx->stRwSep);
	struct dip_frame *pDipFrm = &(ctx->out);
	u32 u32MaxInWidth = 0;
	u32 u32MaxOutWidth = 0;
	bool bHVSPScalingDown = false;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return ret;

	//HSD
	pstScalingDown->u32InputWidth = pDipFrm->c_width;
	pstScalingDown->u32OutputWidth = pDipFrm->c_width;
	pstScalingDown->u8HScalingEna = 0;

	//DI
	pstDiNrInfo->u32InputWidth = pstScalingDown->u32OutputWidth;
	pstDiNrInfo->u32OutputWidth = pstScalingDown->u32OutputWidth;
	pstDiNrInfo->u32InputHeight = pDipFrm->c_height;
	pstDiNrInfo->u32OutputHeight = pDipFrm->c_height;

	//VSD
	pstScalingDown->u32InputHeight = pstDiNrInfo->u32OutputHeight;
	pstScalingDown->u32OutputHeight = pstDiNrInfo->u32OutputHeight;
	pstScalingDown->u8VScalingEna = 0;

	//HSP
	pstScalingHVSP->u32InputWidth = pstDiNrInfo->u32InputWidth;
	pstScalingHVSP->u32OutputWidth = pDipFrm->u32SclOutWidth;
	if (pstScalingHVSP->u32InputWidth != pstScalingHVSP->u32OutputWidth)
		pstScalingHVSP->u8HScalingEna = 1;
	else
		pstScalingHVSP->u8HScalingEna = 0;
	if (pstScalingHVSP->u32InputWidth > pstScalingHVSP->u32OutputWidth)
		bHVSPScalingDown = true;
	else
		bHVSPScalingDown = false;

	//VSP
	pstScalingHVSP->u32InputHeight = pstScalingDown->u32OutputHeight;
	pstScalingHVSP->u32OutputHeight = pDipFrm->u32SclOutHeight;
	if (pstScalingHVSP->u32InputHeight != pstScalingHVSP->u32OutputHeight)
		pstScalingHVSP->u8VScalingEna = 1;
	else
		pstScalingHVSP->u8VScalingEna = 0;

	// RWSEP
	u32MaxInWidth = _GetDIPRFmtMaxWidth(ctx, ctx->in.fmt->fourcc);
	u32MaxOutWidth = _GetDIPWFmtMaxWidth(ctx, ctx->out.fmt->fourcc, enDIPIdx);
	if ((ctx->in.width > u32MaxInWidth) || (ctx->out.width > u32MaxOutWidth)) {
		pstRwSep->bEable = true;
		pstRwSep->bHVSPScalingDown = bHVSPScalingDown;
		pstRwSep->u16Times = DIP_RWSEP_TIMES;
	} else {
		pstRwSep->bEable = false;
	}

	return 0;
}

int _DipPreCalDiScalUpDnFromSrc(struct dip_ctx *ctx)
{
	struct ST_SCALING *pstScalingDown = &(ctx->stScalingDown);
	struct ST_SCALING *pstScalingHVSP = &(ctx->stScalingHVSP);
	struct ST_DIP_DINR *pstDiNrInfo = &(ctx->stDiNrInfo);
	struct ST_RWSEP_INFO *pstRwSep = &(ctx->stRwSep);
	struct dip_frame *pDipFrm = &(ctx->out);
	MS_U32 u32LimitWidth = 0;

	//HSD
	pstScalingDown->u32InputWidth = pDipFrm->c_width;
	if (ctx->stDiNrInfo.u32OpMode) {
		if (DIP1_HVSP_IN_WIDTH_MAX > DIP1_DI_IN_WIDTH_MAX)
			u32LimitWidth = DIP1_DI_IN_WIDTH_MAX;
		else
			u32LimitWidth = DIP1_HVSP_IN_WIDTH_MAX;
	} else {
		if ((pDipFrm->c_width == pDipFrm->u32SclOutWidth) &&
			(pDipFrm->c_height == pDipFrm->u32SclOutHeight)) {
			u32LimitWidth = pDipFrm->c_width;
		} else {
			u32LimitWidth = DIP1_HVSP_IN_WIDTH_MAX;
		}
	}
	if (pDipFrm->c_width > u32LimitWidth)
		pstScalingDown->u32OutputWidth = u32LimitWidth;
	else
		pstScalingDown->u32OutputWidth = pDipFrm->c_width;
	if (pstScalingDown->u32InputWidth != pstScalingDown->u32OutputWidth)
		pstScalingDown->u8HScalingEna = 1;
	else
		pstScalingDown->u8HScalingEna = 0;

	//DI
	pstDiNrInfo->u32InputWidth = pstScalingDown->u32OutputWidth;
	pstDiNrInfo->u32OutputWidth = pstScalingDown->u32OutputWidth;
	pstDiNrInfo->u32InputHeight = pDipFrm->c_height;
	if (ctx->stDiNrInfo.u32OpMode == E_DIP_MODE_DI)
		pstDiNrInfo->u32OutputHeight = pDipFrm->c_height*2;
	else
		pstDiNrInfo->u32OutputHeight = pDipFrm->c_height;

	//VSD
	pstScalingDown->u32InputHeight = pstDiNrInfo->u32OutputHeight;
	pstScalingDown->u32OutputHeight = pstDiNrInfo->u32OutputHeight;
	pstScalingDown->u8VScalingEna = 0;

	//HSP
	pstScalingHVSP->u32InputWidth = pstDiNrInfo->u32OutputWidth;
	pstScalingHVSP->u32OutputWidth = pDipFrm->u32SclOutWidth;
	if (pstScalingHVSP->u32InputWidth != pstScalingHVSP->u32OutputWidth)
		pstScalingHVSP->u8HScalingEna = 1;
	else
		pstScalingHVSP->u8HScalingEna = 0;

	//VSP
	pstScalingHVSP->u32InputHeight = pstScalingDown->u32OutputHeight;
	pstScalingHVSP->u32OutputHeight = pDipFrm->u32SclOutHeight;
	if (pstScalingHVSP->u32InputHeight != pstScalingHVSP->u32OutputHeight)
		pstScalingHVSP->u8VScalingEna = 1;
	else
		pstScalingHVSP->u8VScalingEna = 0;

	// RWSEP
	pstRwSep->bEable = false;

	return 0;
}

int _DipPreCalScalDown(struct dip_ctx *ctx)
{
	struct ST_SCALING *pstScalingDown = &(ctx->stScalingDown);
	struct ST_SCALING *pstScalingHVSP = &(ctx->stScalingHVSP);
	struct ST_DIP_DINR *pstDiNrInfo = &(ctx->stDiNrInfo);
	struct ST_RWSEP_INFO *pstRwSep = &(ctx->stRwSep);
	struct dip_frame *pDipFrm = &(ctx->out);

	//HSD
	pstScalingDown->u32InputWidth = pDipFrm->c_width;
	pstScalingDown->u32OutputWidth = pDipFrm->u32SclOutWidth;
	if (pstScalingDown->u32InputWidth != pstScalingDown->u32OutputWidth)
		pstScalingDown->u8HScalingEna = 1;
	else
		pstScalingDown->u8HScalingEna = 0;

	//DI
	pstDiNrInfo->u32InputWidth = pstScalingDown->u32OutputWidth;
	pstDiNrInfo->u32OutputWidth = pstScalingDown->u32OutputWidth;
	pstDiNrInfo->u32InputHeight = pDipFrm->c_height;
	pstDiNrInfo->u32OutputHeight = pDipFrm->c_height;

	//VSD
	pstScalingDown->u32InputHeight = pDipFrm->c_height;
	pstScalingDown->u32OutputHeight = pDipFrm->u32SclOutHeight;
	if (pstScalingDown->u32InputHeight != pstScalingDown->u32OutputHeight)
		pstScalingDown->u8VScalingEna = 1;
	else
		pstScalingDown->u8VScalingEna = 0;

	//HSP
	pstScalingHVSP->u32InputWidth = pstScalingDown->u32OutputWidth;
	pstScalingHVSP->u32OutputWidth = pstScalingDown->u32OutputWidth;
	if (pstScalingHVSP->u32InputWidth != pstScalingHVSP->u32OutputWidth)
		pstScalingHVSP->u8HScalingEna = 1;
	else
		pstScalingHVSP->u8HScalingEna = 0;

	//VSP
	pstScalingHVSP->u32InputHeight = pstScalingDown->u32OutputHeight;
	pstScalingHVSP->u32OutputHeight = pstScalingDown->u32OutputHeight;
	if (pstScalingHVSP->u32InputHeight != pstScalingHVSP->u32OutputHeight)
		pstScalingHVSP->u8VScalingEna = 1;
	else
		pstScalingHVSP->u8VScalingEna = 0;

	// RWSEP
	pstRwSep->bEable = false;

	return 0;
}

int _DipPreCalSetting(struct dip_ctx *ctx)
{
	struct dip_dev *dev = ctx->dev;

	if ((dev->dipcap_dev.u32USCL) &&
			(dev->dipcap_dev.u32DSCL) &&
			(dev->dipcap_dev.u323DDI) &&
			(dev->dipcap_dev.u32RWSep)) {
		if ((ctx->enSource == E_DIP_SOURCE_DIPR) &&
				(_IsCompressFormat(ctx->in.fmt->fourcc) == false)) {
			_DipPreCalDiScalUpDnFromDipr(ctx);
		} else {
			_DipPreCalDiScalUpDnFromSrc(ctx);
		}
	} else if (dev->dipcap_dev.u32DSCL) {
		_DipPreCalScalDown(ctx);
	}

	return 0;
}

static int dip_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct dip_ctx *ctx = vb2_get_drv_priv(vq);
	struct dip_dev *dev = ctx->dev;
	unsigned long flags = 0;
	int ret = 0;
	dma_addr_t iova = 0;
	size_t DiBufSize = 0;

	if (_IsDstBufType(vq->type) == true) {
		_DipPreCalSetting(ctx);
		if (ctx->stDiNrInfo.u32OpMode) {
			if (gstDiNrBufInfo.DmaAddr == 0) {
				if (ctx->stDiNrInfo.u32OpMode == E_DIP_MODE_DI) {
					DiBufSize = (ctx->stDiNrInfo.u32InputWidth)*
						(ctx->stDiNrInfo.u32InputHeight)*
						DIP_DIDNR_BPP*
						DIP_DI_CNT;
				} else {
					DiBufSize = (ctx->stDiNrInfo.u32InputWidth)*
						(ctx->stDiNrInfo.u32InputHeight)*
						DIP_DIDNR_BPP*
						DIP_DNR_CNT;
				}
				ret = _DipAllocateBuffer(ctx,
						dev->di_buf_tag,
						&iova,
						DiBufSize);
				if (ret) {
					v4l2_err(&dev->v4l2_dev, "Allocate DI Buffer Fail\n");
					return -EPERM;
				}
			}
		}

		ret = _DipSecureCheck(ctx);
		if (ret) {
			v4l2_err(&dev->v4l2_dev, "Check Dip Secure Fail\n");
			return ret;
		}
		if ((_IsCapCase(ctx->enSource)) == true) {
			if (ctx->u8WorkingAid == E_AID_SDC) {
				if (_IsSdcCapCase(ctx->enSource) == true) {
					ret = _DipTeeSdcCapture(ctx);
					if (ret) {
						v4l2_err(&dev->v4l2_dev,
							"%s, Secure Capture Fail\n", __func__);
						return ret;
					}
				} else {
					v4l2_err(&dev->v4l2_dev,
						"%s, enSource:%d, Not Support secure capture\n",
						__func__, ctx->enSource);
					ctx->u8WorkingAid = E_AID_S;
					return -EPERM;
				}
			}
		}

		spin_lock_irqsave(&ctx->dev->ctrl_lock, flags);
		if ((_IsM2mCase(ctx->enSource)) == true) {
			ret = _DipSetSource(ctx);
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,%d, Error in Src Setting\n",
					__func__, __LINE__);
			}
		}
		ret = _DipWorkSetting(ctx, vq->type);
		if (ret) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, Error in Capture Setting\n",
				__func__, __LINE__);
		}
		spin_unlock_irqrestore(&ctx->dev->ctrl_lock, flags);
	}

	return ret;
}

static void dip_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct dip_ctx *ctx = vb2_get_drv_priv(vq);
	struct dip_dev *dev = ctx->dev;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	unsigned long flags;
	EN_DIP_SRC_FROM enSrcFrom = E_DIP_SRC_FROM_MAX;

	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX)
		enDIPIdx = E_DIP_IDX0;
	else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX)
		enDIPIdx = E_DIP_IDX1;
	else if (dev->variant->u16DIPIdx == DIP2_WIN_IDX)
		enDIPIdx = E_DIP_IDX2;

	spin_lock_irqsave(&ctx->dev->ctrl_lock, flags);
	/* stop hardware streaming */
	if (_IsDstBufType(vq->type) == true) {
		KDrv_DIP_Ena(false, enDIPIdx);
		_GetSourceSelect(ctx, &enSrcFrom);
		_SetDIPClkParent(dev, enSrcFrom, false);
	}
	if (_IsSrcBufType(vq->type) == true) {
		if ((dev->dipcap_dev.u32B2R) &&
			((_IsCompressFormat(ctx->in.fmt->fourcc) == true)
			|| (ctx->enSource == E_DIP_SOURCE_B2R))) {
			KDrv_DIP_B2R_Ena(false);
			_SetB2rClk(dev, false, 0);
		}
	}
	spin_unlock_irqrestore(&ctx->dev->ctrl_lock, flags);

	if (ctx->stDiNrInfo.u32OpMode) {
		if (gstDiNrBufInfo.DmaAddr != 0) {
			dma_free_attrs(dev->dev,
					gstDiNrBufInfo.u32Size,
					gstDiNrBufInfo.virtAddr,
					gstDiNrBufInfo.DmaAddr,
					gstDiNrBufInfo.dma_attrs);
			gstDiNrBufInfo.DmaAddr = 0;
			gstDiNrBufInfo.virtAddr = NULL;
		}
	}

	if (_IsDstBufType(vq->type) == true) {
		if (ctx->u8WorkingAid == E_AID_SDC)
			mtk_dip_svc_exit(ctx);
	}
}

static const struct vb2_ops dip_qops = {
	.queue_setup     = dip_queue_setup,
	.buf_prepare     = dip_buf_prepare,
	.buf_queue       = dip_buf_queue,
	.start_streaming = dip_vb2_start_streaming,
	.stop_streaming  = dip_vb2_stop_streaming,
};

static void *mtk_vb2_cookie(void *buf_priv)
{
	struct mtk_vb2_buf *buf = buf_priv;

	return &buf->paddr;
}

static void *mtk_vb2_get_userptr(struct device *dev, unsigned long vaddr,
			unsigned long size, enum dma_data_direction dma_dir)
{
	struct mtk_vb2_buf *buf;
	struct dip_dev *Dipdev = dev_get_drvdata(dev);
	struct dip_ctx *ctx = Dipdev->ctx;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->dma_dir = dma_dir;
	if (dma_dir == DMA_INPUT) {
		if (ctx->u32SrcBufMode == E_BUF_PA_MODE) {
			buf->paddr = vaddr;
		} else {
			buf->paddr = vaddr;
			//fixme
		}
	}
	if (dma_dir == DMA_OUTPUT) {
		if (ctx->u32DstBufMode == E_BUF_PA_MODE) {
			buf->paddr = vaddr;
		} else {
			buf->paddr = vaddr;
			//fixme
		}
	}
	buf->size = size;

	return buf;
}

static void mtk_vb2_put_userptr(void *buf_priv)
{
	struct mtk_vb2_buf *buf = buf_priv;

	kfree(buf);
}

const struct vb2_mem_ops mtk_vb2_alloc_memops = {
	.get_userptr    = mtk_vb2_get_userptr,
	.put_userptr    = mtk_vb2_put_userptr,
	.cookie         = mtk_vb2_cookie,
};

static int queue_init(void *priv, struct vb2_queue *src_vq,
						struct vb2_queue *dst_vq)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	int ret = 0;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_USERPTR;
	src_vq->drv_priv = ctx;
	src_vq->ops = &dip_qops;
	src_vq->mem_ops = &mtk_vb2_alloc_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->dev->mutex;
	src_vq->min_buffers_needed = 1;
	src_vq->dev = ctx->dev->v4l2_dev.dev;
	ret = vb2_queue_init(src_vq);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "[%s,%d], src_vq Init Fail, ret =%d\n",
				__func__, __LINE__, ret);
		return ret;
	}

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_USERPTR;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &dip_qops;
	dst_vq->mem_ops = &mtk_vb2_alloc_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->dev->mutex;
	dst_vq->min_buffers_needed = 1;
	dst_vq->dev = ctx->dev->v4l2_dev.dev;
	ret = vb2_queue_init(dst_vq);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "[%s,%d], dst_vq Init Fail, ret =%d\n",
				__func__, __LINE__, ret);
		return ret;
	}

	return ret;
}

int _UpdateSourceInfo(struct dip_ctx *ctx, struct ST_DIP_SOURCE_INFO *pstSrcInfo)
{
	struct dip_dev *dev = ctx->dev;
	struct ST_DIP_METADATA stMetaData;
	struct mdseal_ioc_data sealdata;
	int ret = 0;
	unsigned int idx = 0;

	DIP_DBG_API("Source Width:%d, Height:%d\n", pstSrcInfo->u32Width, pstSrcInfo->u32Height);

	if ((pstSrcInfo->u32Version == DIP_SOURCE_INFO_VER) &&
		(pstSrcInfo->u32Length == sizeof(struct ST_DIP_SOURCE_INFO))) {
		//General Iinfo
		ctx->stSrcInfo.u8PixelNum = pstSrcInfo->u8PxlEngine;
		ctx->stSrcInfo.enSrcClr = (EN_DIP_SRC_COLOR)pstSrcInfo->enClrFmt;
		ctx->stSrcInfo.u32Width = pstSrcInfo->u32Width;
		ctx->stSrcInfo.u32Height = pstSrcInfo->u32Height;
		ctx->stSrcInfo.u32DisplayWidth = pstSrcInfo->u32Width;
		ctx->stSrcInfo.u32DisplayHeight = pstSrcInfo->u32Height;
		//Secure Info
		ctx->u32pipeline_id = pstSrcInfo->u32PipeLineId;
		if (pstSrcInfo->u8IsValidFd) {
			stMetaData.u16Enable = pstSrcInfo->u8IsValidFd;
			stMetaData.meta_fd = pstSrcInfo->MetaFd;
			stMetaData.enDIPSrc = pstSrcInfo->enDIPSrc;
			GetMetadataInfo(ctx, &stMetaData);
		} else if (pstSrcInfo->u32PipeLineId) {
			sealdata.pipelineID = pstSrcInfo->u32PipeLineId;
			ret = mtkd_seal_getaidtype(&sealdata);
			if (ret == 0) {
				ctx->u8aid = sealdata.enType;
			} else {
				v4l2_err(&dev->v4l2_dev, "[%s][Error]Get AID type fail\n",
					__func__);
				ctx->u8aid = E_AID_NS;
			}
		} else {
			if (pstSrcInfo->u8IsSecutiry)
				ctx->u8aid = E_AID_S;
			else
				ctx->u8aid = E_AID_NS;
		}
		//Max Crop Width
		if ((_IsCapCase(ctx->enSource)) == true) {
			if (ctx->stSrcInfo.u8PixelNum > dev->dipcap_dev.u32DipPixelNum)
				idx = (ctx->stSrcInfo.u8PixelNum)/(dev->dipcap_dev.u32DipPixelNum);
			else
				idx = 1;
			pstSrcInfo->u32CropMaxWidth = ctx->stSrcInfo.u32Width/idx;
		} else {
			pstSrcInfo->u32CropMaxWidth = ctx->stSrcInfo.u32Width;
		}
		DIP_DBG_API("%s, Crop Input Max Width:%d\n", __func__, pstSrcInfo->u32CropMaxWidth);
	} else {
		v4l2_err(&dev->v4l2_dev, "%s, Version and Size Mismatch\n",
			__func__);
		ret = (-EINVAL);
	}

	return ret;
}

int dip_SetSourceInfo(struct dip_ctx *ctx, void __user *ptr, __u32 size)
{
	struct dip_dev *dev = ctx->dev;
	struct ST_DIP_SOURCE_INFO stSrcInfo;
	int ret = 0;

	if (ptr == NULL) {
		v4l2_err(&dev->v4l2_dev, "%s, NULL pointer\n", __func__);
		return -EINVAL;
	}

	memset(&stSrcInfo, 0, sizeof(struct ST_DIP_SOURCE_INFO));
	ret = copy_from_user(&stSrcInfo, (void *)ptr, size);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s, error copying data\n",
				__func__);
		return -EFAULT;
	}
	ret = _UpdateSourceInfo(ctx, &stSrcInfo);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s, _UpdateSourceInfo error\n", __func__);
		return -EFAULT;
	}
	ret = copy_to_user((void *)ptr, &stSrcInfo, size);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s, error copy data to user\n",
				__func__);
		return -EFAULT;
	}

	return ret;
}

int dip_SetOutputInfo(struct dip_ctx *ctx, void __user *ptr, __u32 size)
{
	struct dip_dev *dev = ctx->dev;
	struct ST_DIP_OUTPUT_INFO stOutInfo;
	int ret = 0;

	if (ptr == NULL) {
		v4l2_err(&dev->v4l2_dev, "%s, NULL pointer\n", __func__);
		return -EINVAL;
	}

	memset(&stOutInfo, 0, sizeof(struct ST_DIP_OUTPUT_INFO));
	ret = copy_from_user(&stOutInfo, (void *)ptr, size);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s, error copying data\n", __func__);
		return -EFAULT;
	}

	if ((stOutInfo.u32Version == DIP_OUTPUT_INFO_VER) &&
	(stOutInfo.u32Length == sizeof(struct ST_DIP_OUTPUT_INFO))) {
		ctx->u32OutputSecutiry = stOutInfo.u32IsSecutiry;
		ctx->u8ContiBufCnt = stOutInfo.u8ContiBufCnt;
	} else {
		v4l2_err(&dev->v4l2_dev, "%s, Version and Size Mismatch\n", __func__);
		ret = (-EINVAL);
	}

	return ret;
}

int _dip_get_hwcap(struct dip_ctx *ctx, struct ST_DIP_CAP_INFO *pstHwCap)
{
	struct dip_dev *dev = ctx->dev;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	int ret = 0;
	u16 u16SizeAlign = 0;
	u16 u16PitchAlign = 0;
	u32 u32MaxWidth = 0;

	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX)
		enDIPIdx = E_DIP_IDX0;
	else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX)
		enDIPIdx = E_DIP_IDX1;
	else if (dev->variant->u16DIPIdx == DIP2_WIN_IDX)
		enDIPIdx = E_DIP_IDX2;
	else
		return (-1);

	if ((pstHwCap->u32Version == DIP_GET_HWCAP_VER) &&
		(pstHwCap->u32Length == sizeof(struct ST_DIP_CAP_INFO))) {
		switch (pstHwCap->enDipCap) {
		case E_DIP_CAP_ADDR_ALIGN:
			pstHwCap->u32RetVal = DIP_ADDR_ALIGN;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_WIDTH_ALIGN:
			_GetDIPWFmtAlignUnit(pstHwCap->u32Fourcc, &u16SizeAlign, &u16PitchAlign);
			pstHwCap->u32RetVal = u16SizeAlign;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_PITCH_ALIGN:
			_GetDIPWFmtAlignUnit(pstHwCap->u32Fourcc, &u16SizeAlign, &u16PitchAlign);
			pstHwCap->u32RetVal = u16PitchAlign;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_IN_WIDTH_MAX:
			pstHwCap->u32RetVal = _GetDIPRFmtMaxWidth(ctx, pstHwCap->u32Fourcc);
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_IN_WIDTH_MIN:
			pstHwCap->u32RetVal = DIP_IN_WIDTH_MIN;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_OUT_WIDTH_MAX:
			pstHwCap->u32RetVal =
				_GetDIPWFmtMaxWidth(ctx, pstHwCap->u32Fourcc, enDIPIdx);
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_OUT_WIDTH_MIN:
			pstHwCap->u32RetVal = DIP_OUT_WIDTH_MIN;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_CROP_W_ALIGN:
			pstHwCap->u32RetVal = dev->dipcap_dev.u32CropAlign;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_DI_IN_WIDTH_MAX:
			pstHwCap->u32RetVal = _GetDIPDiDnrMaxWidth(enDIPIdx);
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_DI_IN_WIDTH_ALIGN:
			pstHwCap->u32RetVal = _GetDIPDiDnrWidthAlign(enDIPIdx);
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_DNR_IN_WIDTH_MAX:
			pstHwCap->u32RetVal = _GetDIPDiDnrMaxWidth(enDIPIdx);
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_DNR_IN_WIDTH_ALIGN:
			u32MaxWidth = _GetDIPDiDnrWidthAlign(enDIPIdx);
			pstHwCap->u32RetVal = u32MaxWidth;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_VSD_IN_WIDTH_MAX:
			pstHwCap->u32RetVal = dev->dipcap_dev.u32VsdInW;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_HVSP_IN_WIDTH_MAX:
			pstHwCap->u32RetVal = _GetDIPHvspMaxWidth(enDIPIdx);
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_HMIR_WIDTH_MAX:
			pstHwCap->u32RetVal = dev->dipcap_dev.u32HMirSz;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_RWSEP_WIDTH_ALIGN:
			pstHwCap->u32RetVal = _GetDIPRWSepWidthAlign(ctx, pstHwCap->u32Fourcc);
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_RWSEP_IN_MAX:
			pstHwCap->u32RetVal = DIP_RWSEP_IN_WIDTH_MAX;
			if (dev->dipcap_dev.u32RWSepVer == DIP_RWSEP_V0_VERSION)
				pstHwCap->u32RetValB = DIP_RWSEP_V0_IN_HEIGHT_MAX;
			else
				pstHwCap->u32RetValB = DIP_RWSEP_V1_IN_HEIGHT_MAX;
			break;
		case E_DIP_CAP_RWSEP_OUT_MAX:
			pstHwCap->u32RetVal = DIP_RWSEP_OUT_WIDTH_MAX;
			pstHwCap->u32RetValB = DIP_RWSEP_OUT_HEIGHT_MAX;
			break;
		case E_DIP_CAP_ROT_HV_ALIGN:
			pstHwCap->u32RetVal = DIP_ROT_HV_ALIGN;
			pstHwCap->u32RetValB = DIP_ROT_HV_ALIGN;
			break;
		case E_DIP_CAP_FMT_BPP:
			_GetFmtBpp(pstHwCap->u32Fourcc,
					&(pstHwCap->u32RetVal), &(pstHwCap->u32RetValB));
			break;
		case E_DIP_CAP_INPUT_ALIGN:
			_GetDIPFmtAlignUnit(pstHwCap->u32Fourcc, &u16SizeAlign, &u16PitchAlign);
			pstHwCap->u32RetVal = u16SizeAlign;
			pstHwCap->u32RetValB = u16PitchAlign;
			break;
		case E_DIP_CAP_B2R_ADDR_ALIGN:
			_GetB2RAddrAlign(pstHwCap->u32Fourcc,
					&(pstHwCap->u32RetVal), &(pstHwCap->u32RetValB));
			break;
		case E_DIP_CAP_B2R_WIDTH_ALIGN:
			pstHwCap->u32RetVal = DIP_B2R_WIDTH_ALIGN;
			pstHwCap->u32RetValB = 0;
			break;
		case E_DIP_CAP_B2R_WIDTH_MAX:
			pstHwCap->u32RetVal = DIP_B2R_WIDTH_MAX;
			pstHwCap->u32RetValB = 0;
			break;
		default:
			ret = (-1);
			break;
		}
	} else {
		ret = (-1);
	}

	return ret;
}

static int _Check420Tile10bSupportIova(struct dip_ctx *ctx)
{
	struct dip_dev *dev = NULL;
	int ret = 0;
	bool bIs420tile10b = false;
	u32 u32fourcc = 0;
	struct vb2_v4l2_buffer *vb = NULL;
	u64 u64Addr = 0;

	if (ctx == NULL) {
		v4l2_err(&dev->v4l2_dev, "%s, ctx is NULL\n", __func__);
		return (-EPERM);
	}
	dev = ctx->dev;

	if (ctx->enSource == E_DIP_SOURCE_DIPR) {
		u32fourcc = ctx->in.fmt->fourcc;
		bIs420tile10b = _FmtIsYuv10bTile(u32fourcc);
		vb = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
		u64Addr = mtk_get_addr(&(vb->vb2_buf), 0);
		if (u64Addr == INVALID_ADDR) {
			v4l2_err(&dev->v4l2_dev, "%s,%d, address invalid\n",
				__func__, __LINE__);
			return -EINVAL;
		}
		if ((bIs420tile10b == true) &&
			((ctx->u32SrcBufMode == E_BUF_IOVA_MODE) ||
			(u64Addr >= IOVA_START_ADDR))) {
			if (dev->dipcap_dev.u32YUV10bTileIova == 0) {
				v4l2_err(&dev->v4l2_dev, "Input format YUV420 10b tile Not Support IOVA\n");
				ret = (-EPERM);
			}
		}
	}
	u32fourcc = ctx->out.fmt->fourcc;
	bIs420tile10b = _FmtIsYuv10bTile(u32fourcc);
	vb = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	u64Addr = mtk_get_addr(&(vb->vb2_buf), 0);
	if (u64Addr == INVALID_ADDR) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, address invalid\n",
			__func__, __LINE__);
		return -EINVAL;
	}
	if ((bIs420tile10b == true) &&
		((ctx->u32DstBufMode == E_BUF_IOVA_MODE) ||
		(u64Addr >= IOVA_START_ADDR))) {
		if (dev->dipcap_dev.u32YUV10bTileIova == 0) {
			v4l2_err(&dev->v4l2_dev, "Output format YUV420 10b tile Not Support IOVA\n");
			ret = (-EPERM);
		}
	}

	return ret;
}

static int _CheckSupportRotate(struct dip_ctx *ctx)
{
	struct dip_dev *dev = NULL;
	int ret = 0;
	u32 u32Infourcc = 0;
	u32 u32Outfourcc = 0;

	if (ctx == NULL) {
		v4l2_err(&dev->v4l2_dev, "%s, ctx is NULL\n", __func__);
		return (-EPERM);
	}
	dev = ctx->dev;

	if (ctx->u32Rotation) {
		if (dev->dipcap_dev.u32Rotate) {
			u32Infourcc = ctx->in.fmt->fourcc;
			u32Outfourcc = ctx->out.fmt->fourcc;
			if (u32Infourcc == V4L2_PIX_FMT_DIP_YUV422_8B_BLK_TILE) {
				if (!((u32Outfourcc == V4L2_PIX_FMT_YUYV) ||
					(u32Outfourcc == V4L2_PIX_FMT_YVYU) ||
					(u32Outfourcc == V4L2_PIX_FMT_UYVY) ||
					(u32Outfourcc == V4L2_PIX_FMT_VYUY))) {
					v4l2_err(&dev->v4l2_dev,
						"%s, Invalid Rotate Output Format:0x%x\n",
						__func__, u32Outfourcc);
					ret = (-EPERM);
				}
			} else if (u32Infourcc == V4L2_PIX_FMT_DIP_YUV422_10B_BLK_TILE) {
				if (u32Outfourcc != V4L2_PIX_FMT_DIP_MT21S10T) {
					v4l2_err(&dev->v4l2_dev,
						"%s, Invalid Rotate Output Format:0x%x\n",
						__func__, u32Outfourcc);
					ret = (-EPERM);
				}
			} else {
				v4l2_err(&dev->v4l2_dev,
					"%s, Invalid Rotate input format:0x%x\n",
					__func__, u32Infourcc);
				ret = (-EPERM);
			}
		} else {
			v4l2_err(&dev->v4l2_dev, "%s, Not Support Rotate\n", __func__);
			ret = (-EPERM);
		}
	}

	return ret;
}

static int _CheckHWSupport(struct dip_ctx *ctx)
{
	int ret = 0;

	ret |= _Check420Tile10bSupportIova(ctx);

	ret |= _CheckSupportRotate(ctx);

	return ret;
}

static irqreturn_t dip_isr(int irq, void *prv)
{
	struct dip_dev *dev = NULL;
	struct dip_ctx *ctx = NULL;
	struct vb2_v4l2_buffer *src, *dst;
	struct dip_fmt *Dstfmt = NULL;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;
	u16 u16IrqSts = 0;
	u64 pu64DstPhyAddr[MAX_PLANE_NUM] = {0};
	u32 u32Idx = 0;
	u32 u32NPlanes = 0;
	int ret = 0;

	if (prv == NULL)
		return IRQ_NONE;
	dev = prv;
	ctx = dev->ctx;
	if (ctx == NULL)
		return IRQ_NONE;

	ret = _FindMappingDIP(dev, &enDIPIdx);
	if (ret)
		return IRQ_NONE;

	KDrv_DIP_GetIrqStatus(&u16IrqSts, enDIPIdx);
	KDrv_DIP_ClearIrq(u16IrqSts, enDIPIdx);
	if (u16IrqSts != 0) {
		if (ctx->fh.m2m_ctx->cap_q_ctx.num_rdy == 0) {
			DIP_DBG_QUEUE("%s,%d, rdy buffer is empty\n",
				__func__, __LINE__);
			return IRQ_NONE;
		}
		// Set Dst done buffer
		dst = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (dst == NULL) {
			ctx->u32CapStat = ST_OFF;
			v4l2_err(&dev->v4l2_dev, "%s,%d, NULL Dst Buffer\n",
				__func__, __LINE__);
			return IRQ_HANDLED;
		}
		DIP_DBG_QUEUE("%s,%d, rdy:%d, u16IrqSts:0x%x, index:%d\n",
			__func__, __LINE__,
			ctx->fh.m2m_ctx->cap_q_ctx.num_rdy, u16IrqSts,
			dst->vb2_buf.index);

		v4l2_m2m_buf_done(dst, VB2_BUF_STATE_DONE);
		if ((_IsM2mCase(ctx->enSource)) == true) {
			// Set Src done buffer
			src = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
			if (src == NULL) {
				ctx->u32M2mStat = ST_OFF;
				v4l2_err(&dev->v4l2_dev, "%s,%d, NULL Src Buffer\n",
					__func__, __LINE__);
				return IRQ_HANDLED;
			}
			v4l2_m2m_buf_done(src, VB2_BUF_STATE_DONE);
			v4l2_m2m_job_finish(dev->m2m_dev, ctx->fh.m2m_ctx);

			//Check next buffer
			src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
			dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
			if ((src == NULL) || (dst == NULL)) {
				ctx->u32M2mStat = ST_OFF;
				return IRQ_HANDLED;
			}
			//check StreamOff
			if (ctx->u32M2mStat & ST_ABORT)
				ctx->u32M2mStat = ST_OFF;
		} else if ((_IsCapCase(ctx->enSource)) == true) {
			if ((ctx->u32CapStat & ST_CONT) == 0) {
				dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
				if (dst == NULL)
					return IRQ_HANDLED;
				Dstfmt = ctx->out.fmt;
				u32NPlanes = _GetDIPFmtPlaneCnt(Dstfmt->fourcc);
				if (u32NPlanes > MAX_PLANE_NUM)
					return IRQ_HANDLED;
				for (u32Idx = 0 ; u32Idx < u32NPlanes ; u32Idx++) {
					pu64DstPhyAddr[u32Idx] = mtk_get_addr(&(dst->vb2_buf),
						u32Idx);
					if (pu64DstPhyAddr[u32Idx] == INVALID_ADDR) {
						v4l2_err(&dev->v4l2_dev, "%s,%d, addr[%d] invalid\n",
							__func__, __LINE__, u32Idx);
						return -EINVAL;
					}
					if (ctx->u32DstBufMode == E_BUF_IOVA_MODE)
						pu64DstPhyAddr[u32Idx] += IOVA_START_ADDR;
					DIP_DBG_BUF("%s, Addr[%d]:0x%llx\n",
						__func__, u32Idx, pu64DstPhyAddr[u32Idx]);
				}
				KDrv_DIP_SetDstAddr(u32NPlanes, pu64DstPhyAddr, enDIPIdx);
				KDrv_DIP_Trigger(enDIPIdx);
			}
		}
		return IRQ_HANDLED;
	} else {
		return IRQ_NONE;
	}
}

int dip_request_irq(struct dip_dev *dev, u8 u8Enable)
{
	struct dip_ctx *ctx = dev->ctx;
	int ret = 0;

	DIP_DBG_QUEUE("%s, Enable:%d\n", __func__, u8Enable);

	if (u8Enable) {
		if (ctx->u8ReqIrq == 0) {
			ret = devm_request_irq(dev->dev, dev->irq, dip_isr,
				IRQF_SHARED, dev->v4l2_dev.name, dev);
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s, failed to request IRQ %d",
					__func__, dev->irq);
				return ret;
			}
			ctx->u8ReqIrq = 1;
		}
	} else {
		if (ctx->u8ReqIrq) {
			devm_free_irq(dev->dev, dev->irq, dev);
			ctx->u8ReqIrq = 0;
		}
	}

	return ret;
}

static int dip_ioctl_s_ctrl(struct file *file, void *priv, struct v4l2_control *ctrl)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;

	DIP_DBG_API("%s, Id:0x%x, value:%d\n", __func__, ctrl->id, ctrl->value);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		ctx->HMirror = ctrl->value;
		break;
	case V4L2_CID_VFLIP:
		ctx->VMirror = ctrl->value;
		break;
	case V4L2_CID_ALPHA_COMPONENT:
		ctx->alpha = ctrl->value;
		break;
	case V4L2_CID_ROTATE:
		if (ctrl->value == 0) {
			ctx->u32Rotation = ctrl->value;
		} else {
			if ((ctrl->value == ROT90_ANGLE) &&
				(dev->dipcap_dev.u32Rotate & ROT90_SYS_BIT)) {
				ctx->u32Rotation = ctrl->value;
			} else if ((ctrl->value == ROT270_ANGLE) &&
				(dev->dipcap_dev.u32Rotate & ROT270_SYS_BIT)) {
				ctx->u32Rotation = ctrl->value;
			} else {
				ctx->u32Rotation = ctrl->value;
				v4l2_err(&dev->v4l2_dev, "Not Support Rotate %d", ctrl->value);
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

static int dip_ioctl_querycap(struct file *file, void *priv,
	struct v4l2_capability *cap)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;

	DIP_DBG_API("%s\n", __func__);

	strncpy(cap->driver, DIP_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, DIP_NAME, sizeof(cap->card) - 1);
	cap->bus_info[0] = 0;
	cap->device_caps = DEVICE_CAPS;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int dip_ioctl_s_input(struct file *file, void *priv, unsigned int i)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;

	DIP_DBG_API("%s, Input:%d\n", __func__, i);

	if (i >= E_DIP_SOURCE_MAX) {
		v4l2_err(&dev->v4l2_dev, "%s, %d, Invalid Input enSource %d",
			__func__, __LINE__, ctx->enSource);
		return -EINVAL;
	}

	ctx->enSource = (EN_DIP_SOURCE)i;
	ctx->u32CapStat = (ctx->u32CapStat & (~ST_CONT));

	return 0;
}

static int dip_ioctl_streamon(struct file *file, void *priv, enum v4l2_buf_type type)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	struct vb2_queue *vq;
	int ret = 0;

	DIP_DBG_API("%s, Type:%d\n", __func__, type);

	if (_ValidBufType(type) == false) {
		v4l2_err(&dev->v4l2_dev, "%s, %d, Invalid Buffer Type %d",
			__func__, __LINE__, type);
		return -EINVAL;
	}

	dev->ctx = ctx;
	if (_IsDstBufType(type) == true) {
		ret = _CheckHWSupport(ctx);
		if (ret) {
			v4l2_err(&dev->v4l2_dev, "[%s]HW Not Support\n", __func__);
			return ret;
		}
		ret = dip_request_irq(dev, 1);
		if (ret) {
			v4l2_err(&dev->v4l2_dev, "failed to request IRQ:%d\n", dev->irq);
			return ret;
		}
	}
	if ((_IsM2mCase(ctx->enSource)) == true) {
		ret = v4l2_m2m_ioctl_streamon(file, priv, type);
	} else {
		vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, type);
		ret = vb2_streamon(vq, type);
	}

	return ret;
}

static int dip_ioctl_streamoff(struct file *file, void *priv, enum v4l2_buf_type type)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	struct vb2_queue *vq;
	int ret = 0;
	EN_DIP_IDX enDIPIdx = E_DIP_IDX0;

	DIP_DBG_API("%s, Type:%d\n", __func__, type);

	if (_ValidBufType(type) == false) {
		v4l2_err(&dev->v4l2_dev, "%s, %d, Invalid Buffer Type %d",
			__func__, __LINE__, type);
		return -EINVAL;
	}

	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX)
		enDIPIdx = E_DIP_IDX0;
	else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX)
		enDIPIdx = E_DIP_IDX1;
	else if (dev->variant->u16DIPIdx == DIP2_WIN_IDX)
		enDIPIdx = E_DIP_IDX2;

	if (_IsDstBufType(type) == true) {
		ret = dip_request_irq(dev, 0);
		if (ret) {
			v4l2_err(&dev->v4l2_dev, "failed to free IRQ:%d\n", dev->irq);
			return ret;
		}
	}
	if ((_IsM2mCase(ctx->enSource)) == true) {
		ret = v4l2_m2m_ioctl_streamoff(file, priv, type);
		ctx->u32M2mStat |= ST_ABORT;
	} else {
		vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, type);
		if (vb2_is_busy(vq)) {
			ctx->u32CapStat |= ST_ABORT;
			v4l2_info(&dev->v4l2_dev, "%s,%d, type %d, queue is busy\n",
					__func__, __LINE__, type);
		}
		ctx->u32CapStat = ST_OFF;
		ret = vb2_streamoff(vq, type);
	}

	return ret;
}

static int dip_ioctl_s_ext_ctrls(struct file *file, void *fh, struct v4l2_ext_controls *ctrls)
{
	struct dip_ctx *ctx = fh2ctx(file->private_data);
	struct dip_dev *dev = ctx->dev;
	struct v4l2_ext_control *ctrl = NULL;
	struct ST_DIP_MFDEC *pstMfdecSet = &(ctx->stMfdecSet);
	struct ST_DIP_MFDEC_INFO stMfdInfo;
	struct ST_DIP_BUF_MODE stBufMode;
	struct ST_DIP_METADATA stMetaData;
	struct ST_DIP_WINID_INFO stWinIdInfo;
	int i = 0;
	int ret = 0;
	u32 u32OpMode = 0;

	if (ctrls == NULL) {
		v4l2_err(&dev->v4l2_dev, "%s, ctrls is NULL\n", __func__);
		return -EINVAL;
	}

	DIP_DBG_API("%s, Count:%d\n", __func__, ctrls->count);

	for (i = 0; i < ctrls->count; i++) {
		ctrl = &ctrls->controls[i];
		DIP_DBG_API("%s, Id:0x%x\n", __func__, ctrl->id);
		switch (ctrl->id) {
		case V4L2_CID_EXT_DIP_SET_MFDEC:
			ret = copy_from_user(&stMfdInfo, (void *)ctrl->ptr,
							sizeof(struct ST_DIP_MFDEC_INFO));
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, error copying data\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			pstMfdecSet->u16Ena = stMfdInfo.u16Enable;
			pstMfdecSet->u16Sel = stMfdInfo.u16SelNum;
			pstMfdecSet->u16FBPitch = stMfdInfo.u16FBPitch;
			pstMfdecSet->u16BitlenPitch = stMfdInfo.u16BitlenPitch;
			pstMfdecSet->u16StartX = stMfdInfo.u16StartX;
			pstMfdecSet->u16StartY = stMfdInfo.u16StartY;
			pstMfdecSet->u16HSize = stMfdInfo.u16HSize;
			pstMfdecSet->u16VSize = stMfdInfo.u16VSize;
			pstMfdecSet->u16VP9Mode = stMfdInfo.u16VP9Mode;
			pstMfdecSet->u16HMir = stMfdInfo.u16HMirror;
			pstMfdecSet->u16VMir = stMfdInfo.u16VMirror;
			if (stMfdInfo.enMFDecTileMode == E_DIP_MFEDC_TILE_16_32)
				pstMfdecSet->enTileFmt = E_TILE_FMT_16_32;
			else if (stMfdInfo.enMFDecTileMode == E_DIP_MFEDC_TILE_32_32)
				pstMfdecSet->enTileFmt = E_TILE_FMT_32_32;
			else
				pstMfdecSet->enTileFmt = E_TILE_FMT_32_16;
			pstMfdecSet->enTileFmt = (EN_TILE_FMT)stMfdInfo.enMFDecTileMode;
			pstMfdecSet->u16Bypass = stMfdInfo.u16Bypasse;
			pstMfdecSet->u16SimMode = stMfdInfo.u16SimMode;
			break;
		case V4L2_CID_EXT_DIP_SET_OP_MODE:
			ret = copy_from_user(&u32OpMode, (void *)ctrl->ptr, sizeof(u32));
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, error copying data\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			if (dev->dipcap_dev.u323DDI) {
				ctx->stDiNrInfo.u32OpMode = u32OpMode;
				if (ctx->stDiNrInfo.u32OpMode == 0) {
					if ((gstDiNrBufInfo.DmaAddr != 0) &&
							(gstDiNrBufInfo.virtAddr != NULL)) {
						dma_free_attrs(dev->dev,
								gstDiNrBufInfo.u32Size,
								gstDiNrBufInfo.virtAddr,
								gstDiNrBufInfo.DmaAddr,
								gstDiNrBufInfo.dma_attrs);
						gstDiNrBufInfo.DmaAddr = 0;
						gstDiNrBufInfo.virtAddr = NULL;
					}
				}
			}
			break;
		case V4L2_CID_EXT_DIP_SET_BUF_MODE:
			ret = copy_from_user(&stBufMode, (void *)ctrl->ptr, sizeof(stBufMode));
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, error copying data\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			if (_IsSrcBufType(stBufMode.u32BufType) == true)
				ctx->u32SrcBufMode = stBufMode.u32BufMode;
			if (_IsDstBufType(stBufMode.u32BufType) == true)
				ctx->u32DstBufMode = stBufMode.u32BufMode;
			break;
		case V4L2_CID_EXT_DIP_METADATA_FD:
			memset(&stMetaData, 0, sizeof(struct ST_DIP_METADATA));
			ret = copy_from_user(&stMetaData, (void *)ctrl->ptr, sizeof(stMetaData));
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, error copying data\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			ret = GetMetadataInfo(ctx, &stMetaData);
			break;
		case V4L2_CID_EXT_DIP_SET_ONEWIN:
			memset(&stWinIdInfo, 0, sizeof(struct ST_DIP_WINID_INFO));
			ret = copy_from_user(&stWinIdInfo, (void *)ctrl->ptr, sizeof(stWinIdInfo));
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, error copying data\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			ctx->stWinid.u8Enable = stWinIdInfo.u8Enable;
			ctx->stWinid.u16WinId = stWinIdInfo.u16WinId;
			break;
		case V4L2_CID_EXT_DIP_SOURCE_INFO:
			ret = dip_SetSourceInfo(ctx, ctrl->ptr, sizeof(struct ST_DIP_SOURCE_INFO));
			break;
		case V4L2_CID_EXT_DIP_OUTPUT_INFO:
			ret = dip_SetOutputInfo(ctx, ctrl->ptr, ctrl->size);
			break;
		default:
			v4l2_err(&dev->v4l2_dev, "%s,%d, id=0x%x\n", __func__, __LINE__, ctrl->id);
			ret = (-EINVAL);
			break;
		}
	}

	return ret;
}

static int dip_ioctl_g_ext_ctrls(struct file *file, void *fh, struct v4l2_ext_controls *ctrls)
{
	struct dip_ctx *ctx = fh2ctx(file->private_data);
	struct dip_dev *dev = ctx->dev;
	struct v4l2_ext_control *ctrl = NULL;
	struct ST_DIP_CAP_INFO stHwCap;
	int ret = 0;
	int i = 0;

	if (ctrls == NULL) {
		v4l2_err(&dev->v4l2_dev, "%s, ctrls is NULL\n", __func__);
		return -EINVAL;
	}

	DIP_DBG_API("%s, Count:%d\n", __func__, ctrls->count);

	for (i = 0; i < ctrls->count; i++) {
		ctrl = &ctrls->controls[i];
		DIP_DBG_API("%s, Id:0x%x\n", __func__, ctrl->id);
		switch (ctrl->id) {
		case V4L2_CID_EXT_DIP_GET_HWCAP:
			memset(&stHwCap, 0, sizeof(struct ST_DIP_CAP_INFO));
			ret = copy_from_user(&stHwCap, (void *)ctrl->ptr, sizeof(stHwCap));
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, error copying data\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			ret = _dip_get_hwcap(ctx, &stHwCap);
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, get invalid parameter\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			ret = copy_to_user((void *)ctrl->ptr, &stHwCap, sizeof(stHwCap));
			if (ret) {
				v4l2_err(&dev->v4l2_dev, "%s,id=0x%x, error copy data to user\n",
						__func__, ctrl->id);
				return -EFAULT;
			}
			break;
		default:
			v4l2_err(&dev->v4l2_dev, "%s,%d, id=0x%x\n", __func__, __LINE__, ctrl->id);
			break;
		}
	}

	return ret;
}

static int dip_ioctl_enum_cap_mp_fmt(struct file *file, void *prv, struct v4l2_fmtdesc *f)
{
	struct dip_ctx *ctx = fh2ctx(file->private_data);
	struct dip_dev *dev = ctx->dev;
	struct dip_fmt *fmt;
	u32 u32FmtCnt = 0;

	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX) {
		u32FmtCnt = SPT_FMT_CNT(stDip0FmtList);
		if (f->index < u32FmtCnt)
			fmt = &stDip0FmtList[f->index];
		else
			return -EINVAL;
	} else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX) {
		u32FmtCnt = SPT_FMT_CNT(stDip1FmtList);
		if (f->index < u32FmtCnt)
			fmt = &stDip1FmtList[f->index];
		else
			return -EINVAL;
	} else if (dev->variant->u16DIPIdx == DIP2_WIN_IDX) {
		u32FmtCnt = SPT_FMT_CNT(stDip2FmtList);
		if (f->index < u32FmtCnt)
			fmt = &stDip2FmtList[f->index];
		else
			return -EINVAL;
	} else {
		return -EINVAL;
	}

	f->pixelformat = fmt->fourcc;
	strncpy(f->description, fmt->name, sizeof(f->description) - 1);

	DIP_DBG_API("%s, Name:%s\n", __func__, fmt->name);

	return 0;
}

static int dip_ioctl_enum_out_mp_fmt(struct file *file, void *prv, struct v4l2_fmtdesc *f)
{
	struct dip_ctx *ctx = fh2ctx(file->private_data);
	struct dip_dev *dev = ctx->dev;
	struct dip_fmt *fmt;

	if (f->index >= SPT_FMT_CNT(stDiprFmtList))
		return -EINVAL;
	fmt = &stDiprFmtList[f->index];

	if ((fmt->fourcc == V4L2_PIX_FMT_DIP_VSD8X4) ||
		(fmt->fourcc == V4L2_PIX_FMT_DIP_VSD8X2)) {
		if (dev->dipcap_dev.u32B2R == 0)
			return -EINVAL;
	}

	f->pixelformat = fmt->fourcc;
	strncpy(f->description, fmt->name, sizeof(f->description) - 1);

	DIP_DBG_API("%s, Name:%s\n", __func__, fmt->name);

	return 0;
}

static int dip_ioctl_g_fmt_mp(struct file *file, void *prv, struct v4l2_format *f)
{
	struct dip_ctx *ctx = prv;
	struct dip_dev *dev = ctx->dev;
	struct vb2_queue *vq;
	struct dip_frame *frm;
	unsigned int idx = 0;
	u32 u32NPlanes = 0;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (!vq)
		return -EINVAL;
	frm = _GetDIPFrame(ctx, f->type);
	if (IS_ERR(frm))
		return PTR_ERR(frm);

	f->fmt.pix.width        = frm->width;
	f->fmt.pix.height       = frm->height;
	f->fmt.pix.field        = V4L2_FIELD_NONE;
	f->fmt.pix.pixelformat      = frm->fmt->fourcc;

	u32NPlanes = _GetDIPFmtPlaneCnt(frm->fmt->fourcc);
	for (idx = 0; idx < u32NPlanes; idx++) {
		f->fmt.pix_mp.plane_fmt[idx].sizeimage = frm->size[idx];
		f->fmt.pix_mp.plane_fmt[idx].bytesperline = frm->u32BytesPerLine[idx];
	}

	f->fmt.pix_mp.colorspace = frm->colorspace;
	f->fmt.pix_mp.ycbcr_enc = frm->ycbcr_enc;
	f->fmt.pix_mp.quantization = frm->quantization;

	DIP_DBG_API("%s, Type:%d, Width:%d, Height:%d, Format:0x%x, Plane:%d\n",
		__func__, f->type,
		f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.num_planes);

	return 0;
}

static int dip_try_cap_mp_fmt(struct file *file, void *prv, struct v4l2_format *f)
{
	struct dip_ctx *ctx = prv;
	struct dip_dev *dev = ctx->dev;
	struct dip_fmt *fmt;
	enum v4l2_field *field;

	DIP_DBG_API("%s, Type:%d, Width:%d, Height:%d, Format:0x%x, Plane:%d\n",
		__func__, f->type,
		f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.num_planes);

	fmt = _FindDipCapMpFmt(dev, f);
	if (!fmt)
		return -EINVAL;

	field = &f->fmt.pix_mp.field;
	if (*field == V4L2_FIELD_ANY)
		*field = V4L2_FIELD_NONE;
	else if (*field != V4L2_FIELD_NONE)
		return -EINVAL;

	if (f->fmt.pix_mp.width < 1)
		f->fmt.pix_mp.width = 1;
	if (f->fmt.pix_mp.height < 1)
		f->fmt.pix_mp.height = 1;

	return 0;
}

static int dip_try_out_mp_fmt(struct file *file, void *prv, struct v4l2_format *f)
{
	struct dip_ctx *ctx = prv;
	struct dip_dev *dev = ctx->dev;
	struct dip_fmt *fmt;
	enum v4l2_field *field;

	DIP_DBG_API("%s, Type:%d, Width:%d, Height:%d, Format:0x%x, Plane:%d\n",
		__func__, f->type,
		f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.num_planes);

	fmt = _FindDipOutMpFmt(f);
	if (!fmt)
		return -EINVAL;

	field = &f->fmt.pix_mp.field;
	if (*field == V4L2_FIELD_ANY)
		*field = V4L2_FIELD_NONE;
	else if (*field != V4L2_FIELD_NONE)
		return -EINVAL;

	if (f->fmt.pix_mp.width < 1)
		f->fmt.pix_mp.width = 1;
	if (f->fmt.pix_mp.height < 1)
		f->fmt.pix_mp.height = 1;

	return 0;
}

static int dip_ioctl_s_fmt_cap_mplane(struct file *file, void *prv, struct v4l2_format *f)
{
	struct dip_ctx *ctx = prv;
	struct dip_dev *dev = ctx->dev;
	struct vb2_queue *vq;
	struct dip_frame *frm;
	struct dip_fmt *fmt;
	int ret = 0;
	unsigned int idx = 0;
	u32 u32NPlanes = 0;

	DIP_DBG_API("%s, Type:%d, Width:%d, Height:%d, Format:0x%x, Plane:%d\n",
		__func__, f->type,
		f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.num_planes);

	ret = dip_try_cap_mp_fmt(file, prv, f);
	if (ret)
		return ret;
	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vb2_is_busy(vq)) {
		v4l2_err(&dev->v4l2_dev, "queue (%d) bust\n", f->type);
		return -EBUSY;
	}
	frm = _GetDIPFrame(ctx, f->type);
	if (IS_ERR(frm))
		return PTR_ERR(frm);
	fmt = _FindDipCapMpFmt(dev, f);
	if (!fmt)
		return -EINVAL;

	frm->width  = f->fmt.pix_mp.width;
	frm->height = f->fmt.pix_mp.height;

	frm->u32OutWidth  = frm->width;
	frm->u32OutHeight = frm->height;
	frm->fmt    = fmt;
	u32NPlanes = _GetDIPFmtPlaneCnt(fmt->fourcc);
	for (idx = 0; idx < u32NPlanes; idx++) {
		frm->u32BytesPerLine[idx]  = f->fmt.pix_mp.plane_fmt[idx].bytesperline;
		frm->size[idx] = f->fmt.pix_mp.plane_fmt[idx].sizeimage;
		DIP_DBG_API("%s, plane[%d], BytesPerLine:0x%x, size:0x%x\n",
			__func__, idx, frm->u32BytesPerLine[idx], frm->size[idx]);
	}

	if ((_IsCapCase(ctx->enSource)) == true) {
		if (ctx->stSrcInfo.u8PixelNum > dev->dipcap_dev.u32DipPixelNum)
			idx = (ctx->stSrcInfo.u8PixelNum)/(dev->dipcap_dev.u32DipPixelNum);
		else
			idx = 1;
		f->fmt.pix_mp.width = (ctx->stSrcInfo.u32Width)/idx;
		DIP_DBG_API("%s, Crop Input Max Width:%d\n", __func__, f->fmt.pix_mp.width);
	}

	if (f->fmt.pix_mp.colorspace == V4L2_COLORSPACE_DEFAULT)
		frm->colorspace = def_frame.colorspace;
	else
		frm->colorspace = f->fmt.pix_mp.colorspace;
	frm->ycbcr_enc = f->fmt.pix_mp.ycbcr_enc;
	frm->quantization = f->fmt.pix_mp.quantization;

	return 0;
}

static int dip_ioctl_s_fmt_out_mplane(struct file *file, void *prv, struct v4l2_format *f)
{
	struct dip_ctx *ctx = prv;
	struct dip_dev *dev = ctx->dev;
	struct vb2_queue *vq;
	struct dip_frame *frm;
	struct dip_fmt *fmt;
	int ret = 0;
	unsigned int idx = 0;
	u32 u32NPlanes = 0;

	DIP_DBG_API("%s, Type:%d, Width:%d, Height:%d, Format:0x%x, Plane:%d\n",
		__func__, f->type,
		f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.num_planes);

	ret = dip_try_out_mp_fmt(file, prv, f);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s, %d, No Support Foramt, type:%d, fourc:0x%x",
			__func__, __LINE__, f->type, f->fmt.pix_mp.pixelformat);
		return ret;
	}

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vb2_is_busy(vq)) {
		v4l2_err(&dev->v4l2_dev, "queue (%d) bust\n", f->type);
		return -EBUSY;
	}
	frm = _GetDIPFrame(ctx, f->type);
	if (IS_ERR(frm))
		return PTR_ERR(frm);
	fmt = _FindDipOutMpFmt(f);
	if (!fmt)
		return -EINVAL;

	frm->width  = f->fmt.pix_mp.width;
	frm->height = f->fmt.pix_mp.height;

	frm->u32OutWidth  = frm->width;
	frm->u32OutHeight = frm->height;
	frm->fmt    = fmt;
	u32NPlanes = _GetDIPFmtPlaneCnt(fmt->fourcc);
	for (idx = 0; idx < u32NPlanes; idx++) {
		frm->u32BytesPerLine[idx]  = f->fmt.pix_mp.plane_fmt[idx].bytesperline;
		frm->size[idx] = f->fmt.pix_mp.plane_fmt[idx].sizeimage;
		DIP_DBG_API("%s, plane[%d], BytesPerLine:0x%x, size:0x%x\n",
			__func__, idx, frm->u32BytesPerLine[idx], frm->size[idx]);
	}

	if (f->fmt.pix_mp.colorspace == V4L2_COLORSPACE_DEFAULT)
		frm->colorspace = def_frame.colorspace;
	else
		frm->colorspace = f->fmt.pix_mp.colorspace;
	frm->ycbcr_enc = f->fmt.pix_mp.ycbcr_enc;
	frm->quantization = f->fmt.pix_mp.quantization;

	return 0;
}

static int dip_ioctl_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *rb)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;

	DIP_DBG_API("%s, Type:%d, Count:%d\n", __func__, rb->type, rb->count);

	return v4l2_m2m_ioctl_reqbufs(file, priv, rb);
}

static int dip_ioctl_querybuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;

	DIP_DBG_API("%s\n", __func__);

	return v4l2_m2m_ioctl_querybuf(file, priv, buf);
}

static int dip_ioctl_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	int ret = 0;

	DIP_DBG_API("%s, Type:%d, Index:%d, rdy:%d, Start\n", __func__,
		buf->type, buf->index, ctx->fh.m2m_ctx->cap_q_ctx.num_rdy);

	ret = v4l2_m2m_ioctl_qbuf(file, priv, buf);

	DIP_DBG_API("%s, Type:%d, Index:%d, rdy:%d, End\n", __func__,
		buf->type, buf->index, ctx->fh.m2m_ctx->cap_q_ctx.num_rdy);

	return ret;
}

static int dip_ioctl_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	int ret = 0;

	DIP_DBG_API("%s, Type:%d, rdy:%d, Start\n", __func__,
		buf->type, ctx->fh.m2m_ctx->cap_q_ctx.num_rdy);

	ret = v4l2_m2m_ioctl_dqbuf(file, priv, buf);

	DIP_DBG_API("%s, Type:%d, Index:%d, rdy:%d, End\n", __func__,
		buf->type, buf->index, ctx->fh.m2m_ctx->cap_q_ctx.num_rdy);

	return ret;
}

static int dip_ioctl_g_selection(struct file *file, void *priv, struct v4l2_selection *pstSel)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *f;

	f = _GetDIPFrame(ctx, pstSel->type);
	if (IS_ERR(f))
		return PTR_ERR(f);

	if (pstSel->target == V4L2_SEL_TGT_CROP) {
		pstSel->r.left = f->o_width;
		pstSel->r.top = f->o_height;
		pstSel->r.width = f->c_width;
		pstSel->r.height = f->c_height;
	} else if (pstSel->target == V4L2_SEL_TGT_COMPOSE) {
		pstSel->r.left = 0;
		pstSel->r.top = 0;
		pstSel->r.width = f->u32SclOutWidth;
		pstSel->r.height = f->u32SclOutHeight;
	} else {
		v4l2_err(&dev->v4l2_dev,
			"doesn't support target:0x%X\n", pstSel->target);
		return -EINVAL;
	}

	DIP_DBG_API("%s, Target:%d, Left:%d, Top:%d, Width:%d, Height:%d\n",
		__func__, pstSel->target,
		pstSel->r.left, pstSel->r.top,
		pstSel->r.width, pstSel->r.height);

	return 0;
}

static int dip_ioctl_s_selection(struct file *file, void *priv, struct v4l2_selection *pstSel)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;
	struct dip_frame *f;
	int ret;

	DIP_DBG_API("%s, Target:0x%x, Left:%d, Top:%d, Width:%d, Height:%d\n",
		__func__, pstSel->target,
		pstSel->r.left, pstSel->r.top,
		pstSel->r.width, pstSel->r.height);

	f = _GetDIPFrame(ctx, pstSel->type);
	if (IS_ERR(f))
		return PTR_ERR(f);

	if (pstSel->target == V4L2_SEL_TGT_CROP) {
		ret = _ValidCrop(file, priv, pstSel);
		if (ret)
			return ret;
		f->o_width  = pstSel->r.left;
		f->o_height = pstSel->r.top;
		f->c_width  = pstSel->r.width;
		f->c_height = pstSel->r.height;
	} else if (pstSel->target == V4L2_SEL_TGT_COMPOSE) {
		f->u32SclOutWidth  = pstSel->r.width;
		f->u32SclOutHeight = pstSel->r.height;
	} else {
		v4l2_err(&dev->v4l2_dev,
			"doesn't support target = 0x%X\n", pstSel->target);
		return -EINVAL;
	}

	return 0;
}

static int dip_ioctl_s_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *a)
{
	struct dip_ctx *ctx = priv;
	struct dip_dev *dev = ctx->dev;

	DIP_DBG_API("%s, Type:%d, InFps:%d, OutFps:%d\n",
		__func__, a->type,
		a->parm.output.timeperframe.denominator,
		a->parm.output.timeperframe.numerator);

	if (_IsDstBufType(a->type) == true) {
		if (a->parm.output.timeperframe.numerator != 0) {
			ctx->u32CapStat |= ST_CONT;
			ctx->u32InFps = a->parm.output.timeperframe.denominator;
			ctx->u32OutFps = a->parm.output.timeperframe.numerator;
		} else {
			ctx->u32CapStat = (ctx->u32CapStat & (~ST_CONT));
			ctx->u32InFps = a->parm.output.timeperframe.denominator;
			ctx->u32OutFps = a->parm.output.timeperframe.numerator;
		}
	}

	return 0;
}

static void dip_job_abort(void *prv)
{
	struct dip_ctx *ctx = prv;
	struct dip_dev *dev = ctx->dev;

	if (ctx->u32M2mStat == ST_OFF) /* No job currently running */
		return;
	wait_event_timeout(dev->irq_queue,
		ctx->u32M2mStat == ST_OFF,
		msecs_to_jiffies(DIP_TIMEOUT));
}

static void dip_device_run(void *prv)
{
	struct dip_ctx *ctx = prv;
	struct dip_dev *dev = ctx->dev;
	unsigned long flags;
	int ret = 0;

	if (ctx->u8WorkingAid == E_AID_SDC)
		ret = _DipTeeSdcMem2Mem(ctx);

	spin_lock_irqsave(&ctx->dev->ctrl_lock, flags);

	if (ctx->u8WorkingAid != E_AID_SDC)
		ret = _DipDevRunSetSrcAddr(ctx);

	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, Error in Src buffer\n",
			__func__, __LINE__);
		ctx->u32M2mStat = ST_OFF;
	}

	ret = _DipDevRunSetDstAddr(ctx);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, Error in Dst buffer\n",
			__func__, __LINE__);
		ctx->u32M2mStat = ST_OFF;
	}

	ret = _DipDevRunFire(ctx);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "%s,%d, Error in Fire\n",
			__func__, __LINE__);
		ctx->u32M2mStat = ST_OFF;
	}

	spin_unlock_irqrestore(&dev->ctrl_lock, flags);
}

static ssize_t dip_capability0_3ddi_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("3DDI", gDIPcap_capability[0].u323DDI);

	return ssize;
}

static ssize_t dip_capability0_dnr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DNR", gDIPcap_capability[0].u32DNR);

	return ssize;
}

static ssize_t dip_capability0_mfdec_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("MFDEC", gDIPcap_capability[0].u32MFDEC);

	return ssize;
}

static ssize_t dip_capability0_dscl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DSCL", gDIPcap_capability[0].u32DSCL);

	return ssize;
}

static ssize_t dip_capability0_uscl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("USCL", gDIPcap_capability[0].u32USCL);

	return ssize;
}

static ssize_t dip_capability0_rotate_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("Rotate", gDIPcap_capability[0].u32Rotate);

	return ssize;
}

static ssize_t dip_capability0_hdr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("HDR", gDIPcap_capability[0].u32HDR);

	return ssize;
}

static ssize_t dip_capability0_letterbox_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("LetterBox", gDIPcap_capability[0].u32LetterBox);

	return ssize;
}

static ssize_t dip_capability0_rwsep_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("RWSep", gDIPcap_capability[0].u32RWSep);

	return ssize;
}

static ssize_t dip_capability0_b2r_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("B2R", gDIPcap_capability[0].u32B2R);

	return ssize;
}

static ssize_t dip_capability0_dipr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DIPR", gDIPcap_capability[0].u32DIPR);

	return ssize;
}

static ssize_t dip_capability0_diprblending_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DiprBlending", gDIPcap_capability[0].u32DiprBlending);

	return ssize;
}

static ssize_t dip_capability0_SDCCap_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = SYS_SIZE;

	DIP_CAPABILITY_SHOW("SDCCap", gDIPcap_capability[DIP0_WIN_IDX].u32SDCCap);

	return ssize;
}

static ssize_t dip_capability0_42010bTileIova_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = SYS_SIZE;

	DIP_CAPABILITY_SHOW("YUV42010bitTileIova",
		gDIPcap_capability[DIP0_WIN_IDX].u32YUV10bTileIova);

	return ssize;
}

static ssize_t dip0_crc_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	MS_U8 u8Enable = 0;
	MS_U8 u8FrmCnt = 0;

	KDrv_DIP_IsEnableCRC(&u8Enable, &u8FrmCnt, E_DIP_IDX0);
	ssize = snprintf(buf, SYS_SIZE, "%d\n", u8Enable);
	if ((ssize < 0) || (ssize > SYS_SIZE)) {
		DIP_ERR("%s Failed to get CRC enable status\n", __func__);
		return (-EPERM);
	}

	return ssize;
}

static ssize_t dip0_crc_enable_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	MS_U8 u8FrmCnt = DIP_CRC_FRMCNT;

	if (strncmp("on", buf, 2) == 0) {
		DIP_INFO("[%s]Enable DIP CRC\n", __func__);
		KDrv_DIP_EnableCRC(true, u8FrmCnt, E_DIP_IDX0);
	} else if (strncmp("off", buf, 3) == 0) {
		DIP_INFO("[%s]Disable DIP CRC\n", __func__);
		KDrv_DIP_EnableCRC(false, u8FrmCnt, E_DIP_IDX0);
	}

	return count;
}

static ssize_t dip0_crc_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	MS_U32 u32Val = 0;

	KDrv_DIP_GetCrcValue(&u32Val, E_DIP_IDX0);
	DIP_INFO("[%s]Get DIP0 CRC, u32Val:0x%x\n", __func__, u32Val);
	ssize = snprintf(buf, SYS_SIZE, "%d\n", u32Val);
	if ((ssize < 0) || (ssize > SYS_SIZE)) {
		DIP_ERR("%s Failed to get CRC Value\n", __func__);
		return (-EPERM);
	}

	return ssize;
}

static ssize_t dip0_debug_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;

	KDrv_DIP_ShowDevStatus(E_DIP_IDX0);

	return ssize;
}

static ssize_t dip0_show_loglevel(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	uint16_t u16Size = SYS_SIZE;

	ssize +=
		scnprintf(buf + ssize, u16Size - ssize, "%u\n", gu32Dip0LogLevel);

	return ssize;
}

static ssize_t dip0_store_loglevel(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);

	if (err)
		return err;

	gu32Dip0LogLevel = val;

	return count;
}

static ssize_t dip_capability1_3ddi_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("3DDI", gDIPcap_capability[1].u323DDI);

	return ssize;
}

static ssize_t dip_capability1_dnr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DNR", gDIPcap_capability[1].u32DNR);

	return ssize;
}

static ssize_t dip_capability1_mfdec_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("MFDEC", gDIPcap_capability[1].u32MFDEC);

	return ssize;
}

static ssize_t dip_capability1_dscl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DSCL", gDIPcap_capability[1].u32DSCL);

	return ssize;
}

static ssize_t dip_capability1_uscl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("USCL", gDIPcap_capability[1].u32USCL);

	return ssize;
}

static ssize_t dip_capability1_rotate_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("Rotate", gDIPcap_capability[1].u32Rotate);

	return ssize;
}

static ssize_t dip_capability1_hdr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("HDR", gDIPcap_capability[1].u32HDR);

	return ssize;
}

static ssize_t dip_capability1_letterbox_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("LetterBox", gDIPcap_capability[1].u32LetterBox);

	return ssize;
}

static ssize_t dip_capability1_b2r_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("B2R", gDIPcap_capability[1].u32B2R);

	return ssize;
}

static ssize_t dip_capability1_rwsep_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("RWSep", gDIPcap_capability[1].u32RWSep);

	return ssize;
}

static ssize_t dip_capability1_dipr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DIPR", gDIPcap_capability[1].u32DIPR);

	return ssize;
}

static ssize_t dip_capability1_diprblending_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DiprBlending", gDIPcap_capability[1].u32DiprBlending);

	return ssize;
}

static ssize_t dip_capability1_SDCCap_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = SYS_SIZE;

	DIP_CAPABILITY_SHOW("SDCCap", gDIPcap_capability[DIP1_WIN_IDX].u32SDCCap);

	return ssize;
}

static ssize_t dip_capability1_42010bTileIova_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = SYS_SIZE;

	DIP_CAPABILITY_SHOW("YUV42010bitTileIova",
		gDIPcap_capability[DIP1_WIN_IDX].u32YUV10bTileIova);

	return ssize;
}


static ssize_t dip1_crc_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	MS_U8 u8Enable = 0;
	MS_U8 u8FrmCnt = 0;

	KDrv_DIP_IsEnableCRC(&u8Enable, &u8FrmCnt, E_DIP_IDX1);
	ssize = snprintf(buf, 255, "%d\n", u8Enable);
	if ((ssize < 0) || (ssize > SYS_SIZE)) {
		DIP_ERR("%s Failed to get CRC enable status\n", __func__);
		return (-EPERM);
	}

	return ssize;
}

static ssize_t dip1_crc_enable_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	MS_U8 u8FrmCnt = DIP_CRC_FRMCNT;

	if (strncmp("on", buf, 2) == 0) {
		DIP_INFO("[%s]Enable DIP CRC\n", __func__);
		KDrv_DIP_EnableCRC(true, u8FrmCnt, E_DIP_IDX1);
	} else if (strncmp("off", buf, 3) == 0) {
		DIP_INFO("[%s]Disable DIP CRC\n", __func__);
		KDrv_DIP_EnableCRC(false, u8FrmCnt, E_DIP_IDX1);
	}

	return count;
}

static ssize_t dip1_crc_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	MS_U32 u32Val = 0;

	KDrv_DIP_GetCrcValue(&u32Val, E_DIP_IDX1);
	DIP_INFO("[%s]Get DIP1 CRC, u32Val:0x%x\n", __func__, u32Val);
	ssize = snprintf(buf, SYS_SIZE, "%d\n", u32Val);
	if ((ssize < 0) || (ssize > SYS_SIZE)) {
		DIP_ERR("%s Failed to get CRC Value\n", __func__);
		return (-EPERM);
	}

	return ssize;
}

static ssize_t dip1_debug_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;

	KDrv_DIP_ShowDevStatus(E_DIP_IDX1);

	return ssize;
}

static ssize_t dip1_show_loglevel(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	uint16_t u16Size = SYS_SIZE;

	ssize +=
		scnprintf(buf + ssize, u16Size - ssize, "%u\n", gu32Dip1LogLevel);

	return ssize;
}

static ssize_t dip1_store_loglevel(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);

	if (err)
		return err;

	gu32Dip1LogLevel = val;

	return count;
}

static ssize_t dip_capability2_dscl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DSCL", gDIPcap_capability[2].u32DSCL);

	return ssize;
}

static ssize_t dip_capability2_uscl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("USCL", gDIPcap_capability[2].u32USCL);

	return ssize;
}

static ssize_t dip_capability2_rotate_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("Rotate", gDIPcap_capability[2].u32Rotate);

	return ssize;
}

static ssize_t dip_capability2_rwsep_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("RWSep", gDIPcap_capability[2].u32RWSep);

	return ssize;
}

static ssize_t dip_capability2_b2r_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("B2R", gDIPcap_capability[2].u32B2R);

	return ssize;
}

static ssize_t dip_capability2_mfdec_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("MFDEC", gDIPcap_capability[2].u32MFDEC);

	return ssize;
}

static ssize_t dip_capability2_3ddi_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("3DDI", gDIPcap_capability[2].u323DDI);

	return ssize;
}

static ssize_t dip_capability2_dnr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DNR", gDIPcap_capability[2].u32DNR);

	return ssize;
}

static ssize_t dip_capability2_hdr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("HDR", gDIPcap_capability[2].u32HDR);

	return ssize;
}

static ssize_t dip_capability2_dipr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DIPR", gDIPcap_capability[2].u32DIPR);

	return ssize;
}

static ssize_t dip_capability2_letterbox_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("LetterBox", gDIPcap_capability[2].u32LetterBox);

	return ssize;
}

static ssize_t dip_capability2_diprblending_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = 255;

	DIP_CAPABILITY_SHOW("DiprBlending", gDIPcap_capability[2].u32DiprBlending);

	return ssize;
}

static ssize_t dip_capability2_SDCCap_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = SYS_SIZE;

	DIP_CAPABILITY_SHOW("SDCCap", gDIPcap_capability[DIP2_WIN_IDX].u32SDCCap);

	return ssize;
}

static ssize_t dip_capability2_42010bTileIova_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	__u8 u8Size = SYS_SIZE;

	DIP_CAPABILITY_SHOW("YUV42010bitTileIova",
		gDIPcap_capability[DIP2_WIN_IDX].u32YUV10bTileIova);

	return ssize;
}

static ssize_t dip2_crc_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	MS_U8 u8Enable = 0;
	MS_U8 u8FrmCnt = 0;

	KDrv_DIP_IsEnableCRC(&u8Enable, &u8FrmCnt, E_DIP_IDX2);
	ssize = snprintf(buf, 255, "%d\n", u8Enable);
	if ((ssize < 0) || (ssize > SYS_SIZE)) {
		DIP_ERR("%s Failed to get CRC enable status\n", __func__);
		return (-EPERM);
	}

	return ssize;
}

static ssize_t dip2_crc_enable_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	MS_U8 u8FrmCnt = DIP_CRC_FRMCNT;

	if (strncmp("on", buf, 2) == 0) {
		DIP_INFO("[%s]Enable DIP CRC\n", __func__);
		KDrv_DIP_EnableCRC(true, u8FrmCnt, E_DIP_IDX2);
	} else if (strncmp("off", buf, 3) == 0) {
		DIP_INFO("[%s]Disable DIP CRC\n", __func__);
		KDrv_DIP_EnableCRC(false, u8FrmCnt, E_DIP_IDX2);
	}

	return count;
}

static ssize_t dip2_crc_result_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	MS_U32 u32Val = 0;

	KDrv_DIP_GetCrcValue(&u32Val, E_DIP_IDX2);
	DIP_INFO("[%s]Get DIP2 CRC, u32Val:0x%x\n", __func__, u32Val);
	ssize = snprintf(buf, SYS_SIZE, "%d\n", u32Val);
	if ((ssize < 0) || (ssize > SYS_SIZE)) {
		DIP_ERR("%s Failed to get CRC Value\n", __func__);
		return (-EPERM);
	}

	return ssize;
}

static ssize_t dip2_debug_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ssize = 0;

	KDrv_DIP_ShowDevStatus(E_DIP_IDX2);

	return ssize;
}

static ssize_t dip2_show_loglevel(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ssize = 0;
	uint16_t u16Size = SYS_SIZE;

	ssize +=
		scnprintf(buf + ssize, u16Size - ssize, "%u\n", gu32Dip2LogLevel);

	return ssize;
}

static ssize_t dip2_store_loglevel(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);

	if (err)
		return err;

	gu32Dip2LogLevel = val;

	return count;
}

static ssize_t dip_capability_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static struct device_attribute dip_cap_attr0[] = {
	__ATTR(dip_capability_dscl, ATTR_MOD, dip_capability0_dscl_show,
			dip_capability_store),
	__ATTR(dip_capability_uscl, ATTR_MOD, dip_capability0_uscl_show,
			dip_capability_store),
	__ATTR(dip_capability_rotate, ATTR_MOD, dip_capability0_rotate_show,
			dip_capability_store),
	__ATTR(dip_capability_rwsep, ATTR_MOD, dip_capability0_rwsep_show,
			dip_capability_store),
	__ATTR(dip_capability_b2r, ATTR_MOD, dip_capability0_b2r_show,
			dip_capability_store),
	__ATTR(dip_capability_mfdec, ATTR_MOD, dip_capability0_mfdec_show,
			dip_capability_store),
	__ATTR(dip_capability_3ddi, ATTR_MOD, dip_capability0_3ddi_show,
			dip_capability_store),
	__ATTR(dip_capability_dnr, ATTR_MOD, dip_capability0_dnr_show,
			dip_capability_store),
	__ATTR(dip_capability_hdr, ATTR_MOD, dip_capability0_hdr_show,
			dip_capability_store),
	__ATTR(dip_capability_letterbox, ATTR_MOD, dip_capability0_letterbox_show,
			dip_capability_store),
	__ATTR(dip_capability_dipr, ATTR_MOD, dip_capability0_dipr_show,
			dip_capability_store),
	__ATTR(dip_capability_diprblending, ATTR_MOD, dip_capability0_diprblending_show,
			dip_capability_store),
	__ATTR(dip_capability_SDCCap, ATTR_MOD, dip_capability0_SDCCap_show,
			dip_capability_store),
	__ATTR(dip_capability_YUV42010bitTileIova, ATTR_MOD, dip_capability0_42010bTileIova_show,
			dip_capability_store),
	__ATTR(dip_crc_enable, ATTR_MOD, dip0_crc_enable_show,
			dip0_crc_enable_store),
	__ATTR(dip_crc_result, ATTR_MOD, dip0_crc_result_show,
			dip_capability_store),
	__ATTR(dip_debug, ATTR_MOD, dip0_debug_show,
			dip_capability_store),
	__ATTR(dip_loglevel, ATTR_MOD, dip0_show_loglevel,
			dip0_store_loglevel),
};

static struct device_attribute dip_cap_attr1[] = {
	__ATTR(dip_capability_dscl, ATTR_MOD, dip_capability1_dscl_show,
			dip_capability_store),
	__ATTR(dip_capability_uscl, ATTR_MOD, dip_capability1_uscl_show,
			dip_capability_store),
	__ATTR(dip_capability_rotate, ATTR_MOD, dip_capability1_rotate_show,
			dip_capability_store),
	__ATTR(dip_capability_rwsep, ATTR_MOD, dip_capability1_rwsep_show,
			dip_capability_store),
	__ATTR(dip_capability_b2r, ATTR_MOD, dip_capability1_b2r_show,
			dip_capability_store),
	__ATTR(dip_capability_mfdec, ATTR_MOD, dip_capability1_mfdec_show,
			dip_capability_store),
	__ATTR(dip_capability_3ddi, ATTR_MOD, dip_capability1_3ddi_show,
			dip_capability_store),
	__ATTR(dip_capability_dnr, ATTR_MOD, dip_capability1_dnr_show,
			dip_capability_store),
	__ATTR(dip_capability_hdr, ATTR_MOD, dip_capability1_hdr_show,
			dip_capability_store),
	__ATTR(dip_capability_letterbox, ATTR_MOD, dip_capability1_letterbox_show,
			dip_capability_store),
	__ATTR(dip_capability_dipr, ATTR_MOD, dip_capability1_dipr_show,
			dip_capability_store),
	__ATTR(dip_capability_diprblending, ATTR_MOD, dip_capability1_diprblending_show,
			dip_capability_store),
	__ATTR(dip_capability_SDCCap, ATTR_MOD, dip_capability1_SDCCap_show,
			dip_capability_store),
	__ATTR(dip_capability_YUV42010bitTileIova, ATTR_MOD, dip_capability1_42010bTileIova_show,
			dip_capability_store),
	__ATTR(dip_crc_enable, ATTR_MOD, dip1_crc_enable_show,
			dip1_crc_enable_store),
	__ATTR(dip_crc_result, ATTR_MOD, dip1_crc_result_show,
			dip_capability_store),
	__ATTR(dip_debug, ATTR_MOD, dip1_debug_show,
			dip_capability_store),
	__ATTR(dip_loglevel, ATTR_MOD, dip1_show_loglevel,
			dip1_store_loglevel),
};

static struct device_attribute dip_cap_attr2[] = {
	__ATTR(dip_capability_dscl, ATTR_MOD, dip_capability2_dscl_show,
			dip_capability_store),
	__ATTR(dip_capability_uscl, ATTR_MOD, dip_capability2_uscl_show,
			dip_capability_store),
	__ATTR(dip_capability_rotate, ATTR_MOD, dip_capability2_rotate_show,
			dip_capability_store),
	__ATTR(dip_capability_rwsep, ATTR_MOD, dip_capability2_rwsep_show,
			dip_capability_store),
	__ATTR(dip_capability_b2r, ATTR_MOD, dip_capability2_b2r_show,
			dip_capability_store),
	__ATTR(dip_capability_mfdec, ATTR_MOD, dip_capability2_mfdec_show,
			dip_capability_store),
	__ATTR(dip_capability_3ddi, ATTR_MOD, dip_capability2_3ddi_show,
			dip_capability_store),
	__ATTR(dip_capability_dnr, ATTR_MOD, dip_capability2_dnr_show,
			dip_capability_store),
	__ATTR(dip_capability_hdr, ATTR_MOD, dip_capability2_hdr_show,
			dip_capability_store),
	__ATTR(dip_capability_letterbox, ATTR_MOD, dip_capability2_letterbox_show,
			dip_capability_store),
	__ATTR(dip_capability_dipr, ATTR_MOD, dip_capability2_dipr_show,
			dip_capability_store),
	__ATTR(dip_capability_diprblending, ATTR_MOD, dip_capability2_diprblending_show,
			dip_capability_store),
	__ATTR(dip_capability_SDCCap, ATTR_MOD, dip_capability2_SDCCap_show,
			dip_capability_store),
	__ATTR(dip_capability_YUV42010bitTileIova, ATTR_MOD, dip_capability2_42010bTileIova_show,
			dip_capability_store),
	__ATTR(dip_crc_enable, ATTR_MOD, dip2_crc_enable_show,
			dip2_crc_enable_store),
	__ATTR(dip_crc_result, ATTR_MOD, dip2_crc_result_show,
			dip_capability_store),
	__ATTR(dip_debug, ATTR_MOD, dip2_debug_show,
			dip_capability_store),
	__ATTR(dip_loglevel, ATTR_MOD, dip2_show_loglevel,
			dip2_store_loglevel),
};

static void dip_get_capability(struct dip_dev *dip_dev, struct platform_device *pdev)
{
	if (dip_dev->variant->u16DIPIdx == 0) {
		memset(&gDIPcap_capability[0], 0, sizeof(struct dip_cap_dev));
		gDIPcap_capability[0].u32DSCL = dip_dev->dipcap_dev.u32DSCL;
		gDIPcap_capability[0].u32USCL = dip_dev->dipcap_dev.u32USCL;
		gDIPcap_capability[0].u32Rotate = dip_dev->dipcap_dev.u32Rotate;
		gDIPcap_capability[0].u32RWSep = dip_dev->dipcap_dev.u32RWSep;
		gDIPcap_capability[0].u32B2R = dip_dev->dipcap_dev.u32B2R;
		gDIPcap_capability[0].u32MFDEC = dip_dev->dipcap_dev.u32MFDEC;
		gDIPcap_capability[0].u323DDI = dip_dev->dipcap_dev.u323DDI;
		gDIPcap_capability[0].u32DNR = dip_dev->dipcap_dev.u32DNR;
		gDIPcap_capability[0].u32HDR = dip_dev->dipcap_dev.u32HDR;
		gDIPcap_capability[0].u32LetterBox = dip_dev->dipcap_dev.u32LetterBox;
		gDIPcap_capability[0].u32DIPR = dip_dev->dipcap_dev.u32DIPR;
		gDIPcap_capability[0].u32DiprBlending = dip_dev->dipcap_dev.u32DiprBlending;
		gDIPcap_capability[DIP0_WIN_IDX].u32SDCCap = dip_dev->SDCSupport;
		gDIPcap_capability[DIP0_WIN_IDX].u32YUV10bTileIova =
			dip_dev->dipcap_dev.u32YUV10bTileIova;
	} else if (dip_dev->variant->u16DIPIdx == 1) {
		memset(&gDIPcap_capability[1], 0, sizeof(struct dip_cap_dev));
		gDIPcap_capability[1].u32DSCL = dip_dev->dipcap_dev.u32DSCL;
		gDIPcap_capability[1].u32USCL = dip_dev->dipcap_dev.u32USCL;
		gDIPcap_capability[1].u32Rotate = dip_dev->dipcap_dev.u32Rotate;
		gDIPcap_capability[1].u32RWSep = dip_dev->dipcap_dev.u32RWSep;
		gDIPcap_capability[1].u32B2R = dip_dev->dipcap_dev.u32B2R;
		gDIPcap_capability[1].u32MFDEC = dip_dev->dipcap_dev.u32MFDEC;
		gDIPcap_capability[1].u323DDI = dip_dev->dipcap_dev.u323DDI;
		gDIPcap_capability[1].u32DNR = dip_dev->dipcap_dev.u32DNR;
		gDIPcap_capability[1].u32HDR = dip_dev->dipcap_dev.u32HDR;
		gDIPcap_capability[1].u32LetterBox = dip_dev->dipcap_dev.u32LetterBox;
		gDIPcap_capability[1].u32DIPR = dip_dev->dipcap_dev.u32DIPR;
		gDIPcap_capability[1].u32DiprBlending = dip_dev->dipcap_dev.u32DiprBlending;
		gDIPcap_capability[DIP1_WIN_IDX].u32SDCCap = dip_dev->SDCSupport;
		gDIPcap_capability[DIP1_WIN_IDX].u32YUV10bTileIova =
			dip_dev->dipcap_dev.u32YUV10bTileIova;
	} else if (dip_dev->variant->u16DIPIdx == 2) {
		memset(&gDIPcap_capability[2], 0, sizeof(struct dip_cap_dev));
		gDIPcap_capability[2].u32DSCL = dip_dev->dipcap_dev.u32DSCL;
		gDIPcap_capability[2].u32USCL = dip_dev->dipcap_dev.u32USCL;
		gDIPcap_capability[2].u32Rotate = dip_dev->dipcap_dev.u32Rotate;
		gDIPcap_capability[2].u32B2R = dip_dev->dipcap_dev.u32B2R;
		gDIPcap_capability[2].u32MFDEC = dip_dev->dipcap_dev.u32MFDEC;
		gDIPcap_capability[2].u323DDI = dip_dev->dipcap_dev.u323DDI;
		gDIPcap_capability[2].u32DNR = dip_dev->dipcap_dev.u32DNR;
		gDIPcap_capability[2].u32HDR = dip_dev->dipcap_dev.u32HDR;
		gDIPcap_capability[2].u32LetterBox = dip_dev->dipcap_dev.u32LetterBox;
		gDIPcap_capability[2].u32DIPR = dip_dev->dipcap_dev.u32DIPR;
		gDIPcap_capability[2].u32DiprBlending = dip_dev->dipcap_dev.u32DiprBlending;
		gDIPcap_capability[DIP2_WIN_IDX].u32SDCCap = dip_dev->SDCSupport;
		gDIPcap_capability[DIP2_WIN_IDX].u32YUV10bTileIova =
			dip_dev->dipcap_dev.u32YUV10bTileIova;
	} else {
		dev_err(&pdev->dev, "Failed get capability\r\n");
	}
}

static int dip_read_dts_u32(struct device *property_dev,
				struct device_node *node, char *s, u32 *val)
{
	int ret = 0;

	if (node == NULL) {
		DIP_ERR("[%s,%d]node pointer is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (s == NULL) {
		DIP_ERR("[%s,%d]string pointer is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (val == NULL) {
		DIP_ERR("[%s,%d]val pointer is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	ret = of_property_read_u32(node, s, val);
	if (ret != 0) {
		DIP_ERR("[%s,%d]Failed to get %s\n", __func__, __LINE__, s);
		return -ENOENT;
	}

	return ret;
}

static int dip_parse_cap_dts(struct dip_dev *dip_dev, struct platform_device *pdev)
{
	struct device *property_dev = &pdev->dev;
	struct device_node *bank_node;
	int ret = 0;

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		dev_err(&pdev->dev, "Failed to get banks node\r\n");
		return -EINVAL;
	}

	ret |= dip_read_dts_u32(property_dev, bank_node, "DSCL",
					&(dip_dev->dipcap_dev.u32DSCL));

	ret |= dip_read_dts_u32(property_dev, bank_node, "USCL",
					&(dip_dev->dipcap_dev.u32USCL));

	ret |= dip_read_dts_u32(property_dev, bank_node, "Rotate",
					&(dip_dev->dipcap_dev.u32Rotate));

	ret |= dip_read_dts_u32(property_dev, bank_node, "RWSep",
					&(dip_dev->dipcap_dev.u32RWSep));

	ret |= dip_read_dts_u32(property_dev, bank_node, "B2R",
					&(dip_dev->dipcap_dev.u32B2R));

	ret |= dip_read_dts_u32(property_dev, bank_node, "MFDEC",
					&(dip_dev->dipcap_dev.u32MFDEC));

	ret |= dip_read_dts_u32(property_dev, bank_node, "3DDI",
					&(dip_dev->dipcap_dev.u323DDI));

	ret |= dip_read_dts_u32(property_dev, bank_node, "DNR",
					&(dip_dev->dipcap_dev.u32DNR));

	ret |= dip_read_dts_u32(property_dev, bank_node, "HDR",
					&(dip_dev->dipcap_dev.u32HDR));

	ret |= dip_read_dts_u32(property_dev, bank_node, "LetterBox",
					&(dip_dev->dipcap_dev.u32LetterBox));

	ret |= dip_read_dts_u32(property_dev, bank_node, "DIPR",
					&(dip_dev->dipcap_dev.u32DIPR));

	ret |= dip_read_dts_u32(property_dev, bank_node, "DiprBlending",
					&(dip_dev->dipcap_dev.u32DiprBlending));

	ret |= dip_read_dts_u32(property_dev, bank_node, "DiprPixelNum",
					&(dip_dev->dipcap_dev.u32DiprPixelNum));

	ret |= dip_read_dts_u32(property_dev, bank_node, "B2rPixelNum",
					&(dip_dev->dipcap_dev.u32B2rPixelNum));

	ret |= dip_read_dts_u32(property_dev, bank_node, "DipPixelNum",
					&(dip_dev->dipcap_dev.u32DipPixelNum));

	ret |= dip_read_dts_u32(property_dev, bank_node, "FrontPack",
					&(dip_dev->dipcap_dev.u32FrontPack));

	ret |= dip_read_dts_u32(property_dev, bank_node, "CropAlign",
					&(dip_dev->dipcap_dev.u32CropAlign));

	ret |= dip_read_dts_u32(property_dev, bank_node, "HMirrorSz",
					&(dip_dev->dipcap_dev.u32HMirSz));

	ret |= dip_read_dts_u32(property_dev, bank_node, "RWSepVer",
					&(dip_dev->dipcap_dev.u32RWSepVer));

	ret |= dip_read_dts_u32(property_dev, bank_node, "VsdInW",
					&(dip_dev->dipcap_dev.u32VsdInW));

	ret |= dip_read_dts_u32(property_dev, bank_node, "B2rHttPixelUnit",
					&(dip_dev->dipcap_dev.u32B2rHttPixelUnit));

	ret |= dip_read_dts_u32(property_dev, bank_node, "fixed_dd_index",
					&(dip_dev->dipcap_dev.u32MinorNo));

	ret |= dip_read_dts_u32(property_dev, bank_node, "TileWidthMax",
					&(dip_dev->dipcap_dev.TileWidthMax));

	ret |= dip_read_dts_u32(property_dev, bank_node, "SDCCap",
					&(dip_dev->dipcap_dev.u32SDCCap));

	ret |= dip_read_dts_u32(property_dev, bank_node, "yuv10bTileIova",
					&(dip_dev->dipcap_dev.u32YUV10bTileIova));

	return ret;
}

static int dip_parse_iommu_dts(struct dip_dev *dip_dev, struct platform_device *pdev)
{
	struct device *property_dev = &pdev->dev;
	struct device_node *bank_node;

	bank_node = of_find_node_by_name(property_dev->of_node, "iommu_buf_tag");

	if (bank_node == NULL) {
		dev_err(&pdev->dev, "Failed to get banks node\r\n");
		return -EINVAL;
	}

	if (of_property_read_u32(bank_node, "di_buf", &(dip_dev->di_buf_tag))) {
		dev_err(&pdev->dev, "Failed to get iommu buf tag\n");
		return -EINVAL;
	}

	return 0;
}

static int dip_set_ioBase(struct dip_dev *dev, struct platform_device *pdev)
{
	int ret = 0;
	MS_U8 u8IdxDts = 0;
	MS_U8 u8IdxHwreg = 0;

	//DIPW
	u8IdxDts = E_DTS_DIP0;
	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX)
		u8IdxHwreg = E_HWREG_DIP0;
	else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX)
		u8IdxHwreg = E_HWREG_DIP1;
	else
		u8IdxHwreg = E_HWREG_DIP2;
	ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
	if (ret != 0)
		return ret;

	//DIPR
	u8IdxDts = E_DTS_DIPR;
	u8IdxHwreg = E_HWREG_DIPR;
	ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
	if (ret != 0)
		return ret;

	//DIP_TOP
	u8IdxDts = E_DTS_DIPTOP;
	u8IdxHwreg = E_HWREG_DIPTOP;
	ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
	if (ret != 0)
		return ret;

	if (dev->dipcap_dev.u323DDI == 1) {
		//DDI1 bank
		u8IdxDts = E_DTS_DDI1;
		u8IdxHwreg = E_HWREG_DDI1;
		ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
		if (ret != 0)
			return ret;

		//DDNR bank
		u8IdxDts = E_DTS_DDNR;
		u8IdxHwreg = E_HWREG_DDNR;
		ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
		if (ret != 0)
			return ret;
	}

	if (dev->dipcap_dev.u32USCL == 1) {
		//HVSP bank
		u8IdxDts = E_DTS_HVSP;
		u8IdxHwreg = E_HWREG_HVSP;
		ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
		if (ret != 0)
			return ret;
	}

	if (dev->dipcap_dev.u32B2R == 1) {
		//B2R+UFO clk
		ret = b2r_clk_get(pdev, dev);
		if (ret) {
			dev_err(&pdev->dev, "failed to get b2r clock\n");
			return ret;
		}

		//UFO bank
		u8IdxDts = E_DTS_UFO;
		u8IdxHwreg = E_HWREG_UFO;
		ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
		if (ret != 0)
			return ret;

		//B2R bank
		u8IdxDts = E_DTS_B2R;
		u8IdxHwreg = E_HWREG_B2R;
		ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
		if (ret != 0)
			return ret;
	}

	//AID bank
	u8IdxDts = E_DTS_AID;
	u8IdxHwreg = E_HWREG_AID;
	ret = _SetIoBase(pdev, u8IdxDts, u8IdxHwreg);
	if (ret != 0)
		return ret;

	return 0;
}

static void dip_set_UdmaLevel(struct dip_dev *dev)
{
	struct ST_UDMA_LV stUdmaLv = {0};

	stUdmaLv.u8PreultraMask = 0;
	stUdmaLv.u8UltraMask = 0;
	stUdmaLv.u8UrgentMask = 0;

	if (dev->variant->u16DIPIdx == DIP0_WIN_IDX) {
		stUdmaLv.u8PreultraLv = DIPW0_PREULTRA_VAL;
		stUdmaLv.u8UltraLv = DIPW0_ULTRA_VAL;
		stUdmaLv.u8UrgentLv = DIPW0_URGENT_VAL;
		KDrv_DIP_SetUDMALevel(&stUdmaLv, E_UDMA_DIPW0);
	} else if (dev->variant->u16DIPIdx == DIP1_WIN_IDX) {
		stUdmaLv.u8PreultraLv = DIPW1_PREULTRA_VAL;
		stUdmaLv.u8UltraLv = DIPW1_ULTRA_VAL;
		stUdmaLv.u8UrgentLv = DIPW1_URGENT_VAL;
		KDrv_DIP_SetUDMALevel(&stUdmaLv, E_UDMA_DIPW1);
	} else if (dev->variant->u16DIPIdx == DIP2_WIN_IDX) {
		stUdmaLv.u8PreultraLv = DIPW2_PREULTRA_VAL;
		stUdmaLv.u8UltraLv = DIPW2_ULTRA_VAL;
		stUdmaLv.u8UrgentLv = DIPW2_URGENT_VAL;
		KDrv_DIP_SetUDMALevel(&stUdmaLv, E_UDMA_DIPW2);
	}

	if (dev->dipcap_dev.u32DIPR) {
		stUdmaLv.u8PreultraLv = DIPR_PREULTRA_VAL;
		stUdmaLv.u8UltraLv = DIPR_ULTRA_VAL;
		stUdmaLv.u8UrgentLv = DIPR_URGENT_VAL;
		KDrv_DIP_SetUDMALevel(&stUdmaLv, E_UDMA_DIPR);
	}

	if (dev->dipcap_dev.u323DDI) {
		stUdmaLv.u8PreultraLv = DDIW_PREULTRA_VAL;
		stUdmaLv.u8UltraLv = DDIW_ULTRA_VAL;
		stUdmaLv.u8UrgentLv = DDIW_URGENT_VAL;
		KDrv_DIP_SetUDMALevel(&stUdmaLv, E_UDMA_DDIW);
		stUdmaLv.u8PreultraLv = DDIR_PREULTRA_VAL;
		stUdmaLv.u8UltraLv = DDIR_ULTRA_VAL;
		stUdmaLv.u8UrgentLv = DDIR_URGENT_VAL;
		KDrv_DIP_SetUDMALevel(&stUdmaLv, E_UDMA_DDIR);
	}
}

int dip_get_efuse(struct dip_dev *dev)
{
	int Ret = 0;
	unsigned char GetVal = 0;

	Ret = mtk_efuse_get_did_pci(&GetVal);

	if (Ret) {
		v4l2_err(&dev->v4l2_dev, "[%s]call mtk_efuse_get_did_pci Fail\n",
			__func__);
		dev->DebugMode = 0;
	} else {
		v4l2_info(&dev->v4l2_dev, "[%s]efusePCI:%d\n", __func__, GetVal);
		if (GetVal == EFUSE_DIP_PCI_TRUE)
			dev->DebugMode = 0;
		else
			dev->DebugMode = 1;
	}

	Ret = mtk_efuse_get_did_dsc(&GetVal);
	if (Ret) {
		v4l2_err(&dev->v4l2_dev, "[%s]call mtk_efuse_get_did_dsc Fail\n",
			__func__);
		dev->SDCSupport = 0;
	} else {
		v4l2_info(&dev->v4l2_dev, "[%s]efuseSDC:%d, SDCCap:%d\n", __func__,
			GetVal, dev->dipcap_dev.u32SDCCap);
		if ((GetVal == EFUSE_DIP_SDC_SUPPORT) && (dev->dipcap_dev.u32SDCCap))
			dev->SDCSupport = 1;
		else
			dev->SDCSupport = 0;
	}
	v4l2_info(&dev->v4l2_dev, "[%s]DebugMode:%d, SDCSupport:%d\n",
		__func__, dev->DebugMode, dev->SDCSupport);

	return 0;
}
void dip_create_sysfs(struct dip_dev *dip_dev, struct platform_device *pdev)
{
	int i = 0;

	dev_info(&pdev->dev, "Device_create_file initialized u16DIPIdx:%d\n",
					dip_dev->variant->u16DIPIdx);

	if (dip_dev->variant->u16DIPIdx == 0) {
		for (i = 0; i < (sizeof(dip_cap_attr0) / sizeof(struct device_attribute)); i++) {
			if (device_create_file(dip_dev->dev, &dip_cap_attr0[i]) != 0)
				dev_err(&pdev->dev, "Device_create_file_error\n");
		}
	} else if (dip_dev->variant->u16DIPIdx == 1) {
		for (i = 0; i < (sizeof(dip_cap_attr1) / sizeof(struct device_attribute)); i++) {
			if (device_create_file(dip_dev->dev, &dip_cap_attr1[i]) != 0)
				dev_err(&pdev->dev, "Device_create_file_error\n");
		}
	} else if (dip_dev->variant->u16DIPIdx == 2) {
		for (i = 0; i < (sizeof(dip_cap_attr2) / sizeof(struct device_attribute)); i++) {
			if (device_create_file(dip_dev->dev, &dip_cap_attr2[i]) != 0)
				dev_err(&pdev->dev, "Device_create_file_error\n");
		}
	} else {
		dev_err(&pdev->dev, "Failed get capability\r\n");
	}
}

void dip_remove_sysfs(struct dip_dev *dip_dev, struct platform_device *pdev)
{
	u8 u8Idx = 0;
	u8 u8loopSz = 0;

	dev_info(&pdev->dev, "remove DIP:%d sysfs\n", dip_dev->variant->u16DIPIdx);

	if (dip_dev->variant->u16DIPIdx == DIP0_WIN_IDX) {
		u8loopSz = sizeof(dip_cap_attr0)/sizeof(struct device_attribute);
		for (u8Idx = 0; u8Idx < u8loopSz; u8Idx++)
			device_remove_file(dip_dev->dev, &dip_cap_attr0[u8Idx]);
	} else if (dip_dev->variant->u16DIPIdx == DIP1_WIN_IDX) {
		u8loopSz = sizeof(dip_cap_attr1)/sizeof(struct device_attribute);
		for (u8Idx = 0; u8Idx < u8loopSz; u8Idx++)
			device_remove_file(dip_dev->dev, &dip_cap_attr1[u8Idx]);
	} else if (dip_dev->variant->u16DIPIdx == DIP2_WIN_IDX) {
		u8loopSz = sizeof(dip_cap_attr2)/sizeof(struct device_attribute);
		for (u8Idx = 0; u8Idx < u8loopSz; u8Idx++)
			device_remove_file(dip_dev->dev, &dip_cap_attr2[u8Idx]);
	} else {
		dev_err(&pdev->dev, "invalid dip number:%d\n", dip_dev->variant->u16DIPIdx);
	}
}

static int dip_open(struct file *file)
{
	struct dip_dev *dev = video_drvdata(file);
	struct dip_ctx *ctx = NULL;
	int ret = 0;

	DIP_DBG_API("%s, instance opened\n", __func__);

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dev = dev;
	/* Set default DIPFormats */
	ctx->in = def_frame;
	ctx->out = def_frame;

	if (mutex_lock_interruptible(&dev->mutex)) {
		kfree(ctx);
		return -ERESTARTSYS;
	}
	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(dev->m2m_dev, ctx, &queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		mutex_unlock(&dev->mutex);
		kfree(ctx);
		return ret;
	}
	v4l2_fh_init(&ctx->fh, video_devdata(file));
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	dev->ctx = ctx;

	ctx->u32SrcBufMode = E_BUF_PA_MODE;
	ctx->u32DstBufMode = E_BUF_PA_MODE;

	mutex_unlock(&dev->mutex);

	return 0;
}

static int dip_release(struct file *file)
{
	struct dip_dev *dev = video_drvdata(file);
	struct dip_ctx *ctx = fh2ctx(file->private_data);

	DIP_DBG_API("%s, instance closed\n", __func__);

	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	kfree(ctx);

	return 0;
}

static long dip_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	struct dip_dev *dev = video_drvdata(file);
	struct timespec64 Time;
	long ret = 0;
	u64 start = 0, end = 0;

	if ((cmd == VIDIOC_QBUF) || (cmd == VIDIOC_DQBUF)) {
		ktime_get_real_ts64(&Time);
		start = (Time.tv_sec*S_TO_US) + (Time.tv_nsec/NS_TO_US);
	}

	ret = video_ioctl2(file, cmd, arg);

	if ((cmd == VIDIOC_QBUF) || (cmd == VIDIOC_DQBUF)) {
		ktime_get_real_ts64(&Time);
		end = (Time.tv_sec*S_TO_US) + (Time.tv_nsec/NS_TO_US);
		DIP_DBG_IOCTLTIME("%s, Cmd:0x%X, time:%llu us\n", __func__, cmd, end-start);
	}

	return ret;
}

static const struct v4l2_file_operations dip_fops = {
	.owner      = THIS_MODULE,
	.open       = dip_open,
	.release    = dip_release,
	.poll       = v4l2_m2m_fop_poll,
	.unlocked_ioctl = dip_ioctl,
	.mmap       = v4l2_m2m_fop_mmap,
};

static const struct v4l2_ioctl_ops dip_ioctl_ops = {
	.vidioc_querycap    = dip_ioctl_querycap,

	.vidioc_s_input     = dip_ioctl_s_input,

	.vidioc_enum_fmt_vid_cap    = dip_ioctl_enum_cap_mp_fmt,
	.vidioc_enum_fmt_vid_out    = dip_ioctl_enum_out_mp_fmt,

	.vidioc_g_fmt_vid_cap_mplane = dip_ioctl_g_fmt_mp,
	.vidioc_g_fmt_vid_out_mplane = dip_ioctl_g_fmt_mp,

	.vidioc_s_fmt_vid_cap       = dip_ioctl_s_fmt_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane = dip_ioctl_s_fmt_cap_mplane,
	.vidioc_s_fmt_vid_out       = dip_ioctl_s_fmt_out_mplane,
	.vidioc_s_fmt_vid_out_mplane = dip_ioctl_s_fmt_out_mplane,

	.vidioc_reqbufs         = dip_ioctl_reqbufs,
	.vidioc_querybuf        = dip_ioctl_querybuf,
	.vidioc_qbuf            = dip_ioctl_qbuf,
	.vidioc_dqbuf           = dip_ioctl_dqbuf,
	.vidioc_create_bufs     = v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf     = v4l2_m2m_ioctl_prepare_buf,

	.vidioc_streamon        = dip_ioctl_streamon,
	.vidioc_streamoff       = dip_ioctl_streamoff,
	.vidioc_s_parm          = dip_ioctl_s_parm,

	.vidioc_s_ctrl          = dip_ioctl_s_ctrl,
	.vidioc_s_ext_ctrls     = dip_ioctl_s_ext_ctrls,
	.vidioc_g_ext_ctrls     = dip_ioctl_g_ext_ctrls,

	.vidioc_g_selection     = dip_ioctl_g_selection,
	.vidioc_s_selection     = dip_ioctl_s_selection,
};

static struct video_device dip_videodev = {
	.name       = DIP_NAME,
	.fops       = &dip_fops,
	.ioctl_ops  = &dip_ioctl_ops,
	.minor      = -1,
	.release    = video_device_release,
	.vfl_dir    = VFL_DIR_M2M,
};

static struct v4l2_m2m_ops dip_m2m_ops = {
	.device_run = dip_device_run,
	.job_abort  = dip_job_abort,
};

static const struct of_device_id mtk_dip_match[];

static int dip_probe(struct platform_device *pdev)
{
	struct dip_dev *dev;
	struct video_device *vfd;
	const struct of_device_id *of_id;
	int ret = 0;
	int loopIndex = 0;
	unsigned int irq_no;
	int nr = 0;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->ctrl_lock);
	mutex_init(&dev->mutex);
	atomic_set(&dev->num_inst, 0);
	init_waitqueue_head(&dev->irq_queue);

	/* get the interrupt */
	irq_no = platform_get_irq(pdev, 0);
	if (irq_no < 0) {
		dev_err(&pdev->dev, "no irq defined\n");
		return -EINVAL;
	}
	dev->irq = irq_no;

	of_id = of_match_node(mtk_dip_match, pdev->dev.of_node);
	if (!of_id) {
		ret = -ENODEV;
		return ret;
	}
	dev->variant = (struct dip_variant *)of_id->data;

	ret = dip_parse_cap_dts(dev, pdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to get capability\n");
		return ret;
	}

	if (dev->dipcap_dev.u323DDI) {
		ret = dip_parse_iommu_dts(dev, pdev);
		if (ret) {
			dev_err(&pdev->dev, "failed to get iommu tag\n");
			return ret;
		}
	}
	//get clcok
	dev->scip_dip_clk = devm_clk_get(&pdev->dev, "scip_dip");
	if (IS_ERR(dev->scip_dip_clk)) {
		dev_err(&pdev->dev, "failed to get scip_dip clock\n");
		return -ENODEV;
	}

	//prepare clk
	ret = clk_prepare(dev->scip_dip_clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to prepare_enable scip_dip_clk\n");
		goto put_scip_dip_clk;
	}

	ret = dip_set_ioBase(dev, pdev);
	if (ret)
		goto unprep_scip_dip_clk;

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret)
		goto unprep_scip_dip_clk;
	vfd = video_device_alloc();
	if (!vfd) {
		v4l2_err(&dev->v4l2_dev, "Failed to allocate video device\n");
		ret = -ENOMEM;
		goto unreg_v4l2_dev;
	}
	*vfd = dip_videodev;
	vfd->lock = &dev->mutex;
	vfd->v4l2_dev = &dev->v4l2_dev;
	dev->dev = &pdev->dev;
	vfd->device_caps = DEVICE_CAPS;
	if (dev->dipcap_dev.u32MinorNo == FRAMEWK_ASSIGN_NO)
		nr = (-1);
	else
		nr = dev->dipcap_dev.u32MinorNo;
	ret = video_register_device(vfd, VFL_TYPE_GRABBER, nr);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "Failed to register video device\n");
		goto rel_vdev;
	}
	video_set_drvdata(vfd, dev);
	snprintf(vfd->name, sizeof(vfd->name), "%s", dip_videodev.name);
	dev->vfd = vfd;
	v4l2_info(&dev->v4l2_dev, "device registered as /dev/video%d\n", vfd->num);
	platform_set_drvdata(pdev, dev);
	dev->m2m_dev = v4l2_m2m_init(&dip_m2m_ops);
	if (IS_ERR(dev->m2m_dev)) {
		v4l2_err(&dev->v4l2_dev, "Failed to init mem2mem device\n");
		ret = PTR_ERR(dev->m2m_dev);
		goto unreg_video_dev;
	}
	for (loopIndex = 0; loopIndex < MAX_PLANE_NUM; loopIndex++)
		def_frame.u32BytesPerLine[loopIndex] = 0;

	dip_create_sysfs(dev, pdev);

	dip_set_UdmaLevel(dev);

	dip_get_efuse(dev);

	dip_get_capability(dev, pdev);

	return 0;

unreg_video_dev:
	video_unregister_device(dev->vfd);
rel_vdev:
	video_device_release(vfd);
unreg_v4l2_dev:
	v4l2_device_unregister(&dev->v4l2_dev);
unprep_scip_dip_clk:
	clk_unprepare(dev->scip_dip_clk);
put_scip_dip_clk:
	clk_put(dev->scip_dip_clk);

	return ret;
}

static int dip_remove(struct platform_device *pdev)
{
	struct dip_dev *dev = platform_get_drvdata(pdev);

	v4l2_info(&dev->v4l2_dev, "Removing " DIP_NAME);
	v4l2_m2m_release(dev->m2m_dev);
	video_unregister_device(dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);
	dip_remove_sysfs(dev, pdev);
//  vb2_dma_contig_clear_max_seg_size(&pdev->dev);
	clk_unprepare(dev->scip_dip_clk);
	clk_put(dev->scip_dip_clk);
	b2r_clk_put(pdev, dev);

	return 0;
}

/* the First DIP HW Info */
static struct dip_variant mtk_drvdata_dip0 = {
	.u16DIPIdx = DIP0_WIN_IDX,
};

/* the Second DIP HW Info  */
static struct dip_variant mtk_drvdata_dip1 = {
	.u16DIPIdx = DIP1_WIN_IDX,
};

/* the 3rd DIP HW Info  */
static struct dip_variant mtk_drvdata_dip2 = {
	.u16DIPIdx = DIP2_WIN_IDX,
};

static const struct of_device_id mtk_dip_match[] = {
	{
		.compatible = "MTK,DIP0",
		.data = &mtk_drvdata_dip0,
	}, {
		.compatible = "MTK,DIP1",
		.data = &mtk_drvdata_dip1,
	}, {
		.compatible = "mediatek,mt5896-dip0",
		.data = &mtk_drvdata_dip0,
	}, {
		.compatible = "mediatek,mt5896-dip1",
		.data = &mtk_drvdata_dip1,
	}, {
		.compatible = "mediatek,mt5896-dip2",
		.data = &mtk_drvdata_dip2,
	}, {
		.compatible = "mediatek,mtk-dtv-dip0",
		.data = &mtk_drvdata_dip0,
	}, {
		.compatible = "mediatek,mtk-dtv-dip1",
		.data = &mtk_drvdata_dip1,
	}, {
		.compatible = "mediatek,mtk-dtv-dip2",
		.data = &mtk_drvdata_dip2,
	},
	{},
};

static struct platform_driver dip_pdrv = {
	.probe      = dip_probe,
	.remove     = dip_remove,
	.driver     = {
		.name = DIP_NAME,
		.of_match_table = mtk_dip_match,
	},
};
module_platform_driver(dip_pdrv);

MODULE_DEVICE_TABLE(of, mtk_dip_match);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MTK Video Capture Driver");
