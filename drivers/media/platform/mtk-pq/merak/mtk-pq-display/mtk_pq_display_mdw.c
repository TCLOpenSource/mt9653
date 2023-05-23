// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-----------------------------------------------------------------------------
//  Include Files
//-----------------------------------------------------------------------------

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/timekeeping.h>
#include <linux/delay.h>


#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <linux/videodev2.h>
#include <linux/dma-buf.h>
#include <uapi/linux/mtk-v4l2-pq.h>

#include "mtk_pq.h"
#include "mtk_iommu_dtv_api.h"
#include "metadata_utility.h"
#include "m_pqu_pq.h"
#include "pqu_msg.h"

//-----------------------------------------------------------------------------
//  Driver Compiler Options
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
#define POW2_ALIGN(val, align) (((val) + ((align) - 1)) & (~((align) - 1)))

//-----------------------------------------------------------------------------
//  Local Structurs
//-----------------------------------------------------------------------------
struct format_info {
	u32 v4l2_fmt;
	u32 meta_fmt;
	u8 plane_num;
	u8 buffer_num;
	u8 bpp[MAX_FPLANE_NUM];
	enum pqu_pq_output_path output_path;
};

//-----------------------------------------------------------------------------
//  Global Variables
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Local Variables
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Debug Functions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Local Functions
//-----------------------------------------------------------------------------
#define FMT_RGB565_BPP0 16
#define FMT_RGB444_6B_BPP0 18
#define FMT_RGB444_8B_BPP0 24
#define FMT_YUV444_6B_BPP0 18
#define FMT_YUV444_8B_BPP0 24
#define FMT_YUV422_6B_BPP0 12
#define FMT_YUV422_8B_BPP0 16
#define FMT_YUV420_6B_BPP0 9
#define FMT_YUV420_8B_BPP0 12
#define BPP_1ST 0
#define BPP_2ND 1
#define BPP_3RD 2
static int _get_fmt_info(u32 pixfmt_v4l2, struct format_info *fmt_info)
{
	if (WARN_ON(!fmt_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fmt_info=0x%x\n", fmt_info);
		return -EINVAL;
	}

	switch (pixfmt_v4l2) {
	/* ------------------ RGB format ------------------ */
	case V4L2_PIX_FMT_RGB565:
		fmt_info->meta_fmt = MEM_FMT_RGB565;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_RGB565_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_ARGB8888:
		fmt_info->meta_fmt = MEM_FMT_ARGB8888;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_ABGR8888:
		fmt_info->meta_fmt = MEM_FMT_ABGR8888;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_RGBA8888:
		fmt_info->meta_fmt = MEM_FMT_RGBA8888;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_BGRA8888:
		fmt_info->meta_fmt = MEM_FMT_BGRA8888;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_ARGB2101010:
		fmt_info->meta_fmt = MEM_FMT_ARGB2101010;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_ABGR2101010:
		fmt_info->meta_fmt = MEM_FMT_ABGR2101010;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_RGBA1010102:
		fmt_info->meta_fmt = MEM_FMT_RGBA1010102;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_BGRA1010102:
		fmt_info->meta_fmt = MEM_FMT_BGRA1010102;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 32;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	/* ------------------ YUV format ------------------ */
	/* 1 plane */
	case V4L2_PIX_FMT_YUV444_VYU_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV444_VYU_10B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 30;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_YUV444_VYU_8B:
		fmt_info->meta_fmt = MEM_FMT_YUV444_VYU_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 24;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_YVYU:
		fmt_info->meta_fmt = MEM_FMT_YUV422_YVYU_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = 16;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	/* 2 planes */
	case V4L2_PIX_FMT_YUV422_Y_VU_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_10B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 10;
		fmt_info->bpp[1] = 10;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_NV61:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 8;
		fmt_info->bpp[1] = 8;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_8B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_8B_CE;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 8;
		fmt_info->bpp[1] = 8;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV422_Y_VU_6B_CE;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 6;
		fmt_info->bpp[1] = 6;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_10B:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_10B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 10;
		fmt_info->bpp[1] = 5;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_NV21:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 8;
		fmt_info->bpp[1] = 4;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_8B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_8B_CE;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 8;
		fmt_info->bpp[1] = 4;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_6B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV420_Y_VU_6B_CE;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 2;
		fmt_info->bpp[0] = 6;
		fmt_info->bpp[1] = 3;
		fmt_info->bpp[2] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;
	/* 3 planes */
	case V4L2_PIX_FMT_YUV444_Y_U_V_8B_CE:
		fmt_info->meta_fmt = MEM_FMT_YUV444_Y_U_V_8B_CE;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 3;
		fmt_info->bpp[0] = 8;
		fmt_info->bpp[1] = 8;
		fmt_info->bpp[2] = 8;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_MDW;
		break;

//#if 1 //(MEMC_CONFIG == 1)
	case V4L2_PIX_FMT_RGB444_6B:
		fmt_info->meta_fmt = MEM_FMT_RGB444_6B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_RGB444_6B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#if (1)
	case V4L2_PIX_FMT_RGB444_8B:
		fmt_info->meta_fmt = MEM_FMT_RGB444_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_RGB444_8B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#endif
	case V4L2_PIX_FMT_YUV444_YUV_6B:
		fmt_info->meta_fmt = MEM_FMT_YUV444_YUV_6B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_YUV444_6B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#if (1)
	case V4L2_PIX_FMT_YUV444_YUV_8B:
		fmt_info->meta_fmt = MEM_FMT_YUV444_YUV_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_YUV444_8B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#endif
	case V4L2_PIX_FMT_YUV422_YUV_6B:
		fmt_info->meta_fmt = MEM_FMT_YUV422_YUV_6B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_YUV422_6B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#if (1)
	case V4L2_PIX_FMT_YUV422_YUV_8B:
		fmt_info->meta_fmt = MEM_FMT_YUV422_YUV_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_YUV422_8B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#endif
	case V4L2_PIX_FMT_YUV420_YUV_6B:
		fmt_info->meta_fmt = MEM_FMT_YUV420_YUV_6B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_YUV420_6B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#if (1)
	case V4L2_PIX_FMT_YUV420_YUV_8B:
		fmt_info->meta_fmt = MEM_FMT_YUV420_YUV_8B;
		fmt_info->buffer_num = 1;
		fmt_info->plane_num = 1;
		fmt_info->bpp[0] = FMT_YUV420_8B_BPP0;
		fmt_info->bpp[1] = 0;
		fmt_info->bpp[BPP_3RD] = 0;
		fmt_info->output_path = PQ_OUTPUT_PATH_WITH_BUF_FRC;
		break;
	#endif
//#endif
	/* TODO: multi buffer */
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown fmt = %d!!!\n", pixfmt_v4l2);
		return -EINVAL;

	}

	fmt_info->v4l2_fmt = pixfmt_v4l2;

	return 0;
}

//-----------------------------------------------------------------------------
//  Global Functions
//-----------------------------------------------------------------------------
int mtk_pq_display_mdw_read_dts(
	struct device_node *display_node,
	struct pq_display_mdw_device *mdw_dev)
{
	int ret = 0;
	u32 tmp_u32 = 0;
	const char *name;
	struct device_node *mdw_node;

	if (display_node == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "ptr np is NULL\n");
		return -EINVAL;
	}

	mdw_node = of_find_node_by_name(display_node, "mdw");

	// read mdw v align
	name = "mdw_v_align";
	ret = of_property_read_u32(mdw_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	mdw_dev->v_align = (u8)tmp_u32;

	// read mdw h align
	name = "mdw_h_align";
	ret = of_property_read_u32(mdw_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	mdw_dev->h_align = (u8)tmp_u32;

	return 0;

Fail:
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "read DTS failed, name = %s\n", name);
	return ret;
}

#if (1)//(MEMC_CONFIG == 1)
#define IP_VER_MACHLI 1
#define _8K_H 7680
#define _8K_V 4320
#define _8K_H_ALIGN 1024
#define FRAMES_CNT 10
#define MEDS_H 4096
#define MEDS_V 2160
#define MEDS_BPP 6

#define IP_VER_MANKS 2
#define _4K_H 3840
#define _4K_V 2160
#define _4K_H_ALIGN 1024
#define MEDS_H_4K 1920
#define MEDS_V_4K 1080
#define MEDS_BPP_4K 6
#define MV_BUFFER_SIZE 0x19999A

#define CPU_BASE_ADR 0x20000000

#define MTK_DRM_TV_FRC_COMPATIBLE_STR "mediatek,mediatek-frc"
int mtk_pq_display_frc_read_dts(
	struct pq_display_frc_device *frc_dev)
{
	int ret = 0;
	u32 tmp_u32 = 0;
	const char *name;
	struct device_node *frc_node;
	struct of_mmap_info_data of_mmap_info;

	if (frc_dev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "frc_dev is NULL\n");
		return -EINVAL;
	}

	frc_node = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_FRC_COMPATIBLE_STR);

	// read frc width
	name = "frc_width";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_width = (uint32_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_width=%d\n", frc_dev->frc_width);

	// read frc height
	name = "frc_height";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_height = (uint32_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_height=%d\n", frc_dev->frc_height);

	// read frc h align
	name = "frc_h_align";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_h_align = (uint16_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_h_align=%d\n", frc_dev->frc_h_align);

	// read frc frame cnt
	name = "frc_frame_cnt";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_frame_cnt = (uint8_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_frame_cnt=%d\n", frc_dev->frc_frame_cnt);

	name = "frc_meds_width";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_meds_width = (uint32_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_meds_width=%d\n", frc_dev->frc_meds_width);

	// read frc meds height
	name = "frc_meds_height";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_meds_height = (uint32_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_meds_height=%d\n", frc_dev->frc_meds_height);

	name = "frc_meds_bpp";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_meds_bpp = (uint8_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_meds_bpp=%d\n", frc_dev->frc_meds_bpp);

	name = "FRC_IP_VERSION";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_ip_version = (uint8_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_ip_version=%d\n", frc_dev->frc_ip_version);

	if (frc_dev->frc_ip_version == IP_VER_MACHLI) {
		name = "frc mvbff address";
		ret = of_mtk_get_reserved_memory_info_by_idx(frc_node, 0, &of_mmap_info);
		if (ret != 0x0)
			goto Fail;
		frc_dev->frc_mvbff_adr = (uint64_t)of_mmap_info.start_bus_address-CPU_BASE_ADR;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_mvbff_adr=%llx\n",
		frc_dev->frc_mvbff_adr);
	}

	name = "frc_mvbuff_size";
	ret = of_property_read_u32(frc_node, name, &tmp_u32);
	if (ret != 0x0)
		goto Fail;
	frc_dev->frc_mvbff_size = (uint32_t)tmp_u32;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "\nfrc_mvbuff_size=0x%x\n", frc_dev->frc_mvbff_size);

	return 0;

Fail:
	if (frc_dev->frc_ip_version == IP_VER_MANKS) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "read DTS failed, name = %s\n", name);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"so frc read DTS hard code:3840 2160 1920 1080\n");
		frc_dev->frc_width = _4K_H;
		frc_dev->frc_height = _4K_V;
		frc_dev->frc_h_align = _4K_H_ALIGN;
		frc_dev->frc_frame_cnt = FRAMES_CNT;
		frc_dev->frc_meds_width = MEDS_H_4K;
		frc_dev->frc_meds_height = MEDS_V_4K;
		frc_dev->frc_meds_bpp = MEDS_BPP_4K;
		frc_dev->frc_mvbff_size = MV_BUFFER_SIZE;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "read DTS failed, name = %s\n", name);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"so frc read DTS hard code:7680 4320 4096 2160\n");
		frc_dev->frc_width = _8K_H;
		frc_dev->frc_height = _8K_V;
		frc_dev->frc_h_align = _8K_H_ALIGN;
		frc_dev->frc_frame_cnt = FRAMES_CNT;
		frc_dev->frc_meds_width = MEDS_H;
		frc_dev->frc_meds_height = MEDS_V;
		frc_dev->frc_meds_bpp = MEDS_BPP;
		frc_dev->frc_mvbff_size = 0;
	}
	ret = 0;
	return ret;
}
#endif

int mtk_pq_display_mdw_queue_setup(struct vb2_queue *vq,
	unsigned int *num_buffers, unsigned int *num_planes,
	unsigned int sizes[], struct device *alloc_devs[])
{
	struct mtk_pq_ctx *pq_ctx = vb2_get_drv_priv(vq);
	struct mtk_pq_device *pq_dev = pq_ctx->pq_dev;
	struct pq_display_mdw_info *mdw = &pq_dev->display_info.mdw;
	int i = 0;
	int x = 1;

	/* video buffers + meta buffer */
	*num_planes = mdw->buffer_num + 1;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"[mdw] num_planes=%d\n", *num_planes);

	sizes[0] = 0;

	for (i = 0; i < mdw->plane_num; ++i) {
		if (mdw->buffer_num == 1) {
			/* contiguous buffer case, add all buffer size in one */
//			sizes[0] += mdw->plane_size[i];
			sizes[0] += x;
		} else {
			/* non contiguous buffer case */
//			sizes[i] = mdw->plane_size[i];
			sizes[i] = x;
		}
		alloc_devs[i] = pq_dev->dev;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"[mdw] size[%d]=%d, alloc_devs[%d]=%d\n", i, sizes[i], i, alloc_devs[i]);
	}

	/* give meta fd min size */
	sizes[*num_planes - 1] = 1;
	alloc_devs[*num_planes - 1] = pq_dev->dev;

	return 0;
}


int mtk_pq_display_mdw_queue_setup_info(struct mtk_pq_device *pq_dev,
	void *bufferctrl)
{
	struct v4l2_pq_g_buffer_info bufferInfo;
	struct pq_display_mdw_info *mdw = &pq_dev->display_info.mdw;
	int i = 0;

	memset(&bufferInfo, 0, sizeof(struct v4l2_pq_g_buffer_info));

	/* video buffers + meta buffer */
	bufferInfo.plane_num = mdw->buffer_num + 1;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"[mdw] num_planes=%d\n", bufferInfo.plane_num);

	bufferInfo.size[0] = 0;

	for (i = 0; i < mdw->plane_num; ++i) {
		if (mdw->buffer_num == 1) {
			/* contiguous buffer case, add all buffer size in one */
			bufferInfo.size[0] += mdw->plane_size[i];
		} else {
			/* non contiguous buffer case */
			bufferInfo.size[i] = mdw->plane_size[i];
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"[mdw] size[%d]=%d\n", i, bufferInfo.size[i]);
	}

	/* give meta fd min size */
	bufferInfo.size[bufferInfo.plane_num - 1] = 1;

	/* update buffer_num */
	if (mdw->output_path == PQ_OUTPUT_PATH_WITH_BUF_FRC)
		bufferInfo.buffer_num = 10;
	else
		bufferInfo.buffer_num = 2;

	/* update attri */
	if (mdw->output_path == PQ_OUTPUT_PATH_WITH_BUF_FRC)
		bufferInfo.buf_attri = V4L2_PQ_BUF_ATTRI_CONTINUOUS;
	else
		bufferInfo.buf_attri = V4L2_PQ_BUF_ATTRI_DISCRETE;

	/* update bufferctrl */
	memcpy(bufferctrl, &bufferInfo, sizeof(struct v4l2_pq_g_buffer_info));

	return 0;
}

int mtk_pq_display_mdw_set_vflip(struct pq_display_mdw_info *mdw_info, bool enable)
{
	if (WARN_ON(!mdw_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "ptr is NULL, mdw_info=0x%x\n", mdw_info);
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "pq display mdw set vflip = %d\n", enable);

	mdw_info->vflip = enable;

	return 0;
}

int mtk_pq_display_mdw_set_pix_format(
	struct v4l2_format *fmt,
	struct pq_display_mdw_info *mdw_info)
{
	return 0;
}
#define FRAMES_CNT_DIV 8
int mtk_pq_display_mdw_set_pix_format_mplane(
	struct mtk_pq_device *pq_dev,
	struct v4l2_format *fmt)
{
	struct format_info fmt_info;
	int ret = 0;

	struct pq_display_mdw_info *mdw_info = NULL;
	int i = 0;

	if (WARN_ON(!fmt) || WARN_ON(!pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fmt=0x%x, pq_dev=0x%x\n", fmt, pq_dev);
		return -EINVAL;
	}

	mdw_info = &pq_dev->display_info.mdw;

	memset(&fmt_info, 0, sizeof(fmt_info));
	memset(mdw_info->plane_size, 0, sizeof(mdw_info->plane_size));

	/* get pixel format info */
	ret = _get_fmt_info(
		fmt->fmt.pix_mp.pixelformat,
		&fmt_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_get_fmt_info fail, ret = %d\n", ret);
		return ret;
	}

	/* fill info to share mem */
	mdw_info->width = fmt->fmt.pix_mp.width;	//TODO: get from meta
	mdw_info->height = fmt->fmt.pix_mp.height;	//TODO: get from meta
	mdw_info->mem_fmt = fmt->fmt.pix_mp.pixelformat;
	mdw_info->plane_num = fmt_info.plane_num;
	mdw_info->buffer_num = fmt_info.buffer_num;

	/* update fmt to user */
	fmt->fmt.pix_mp.num_planes = mdw_info->buffer_num;

	/* save fmt  */
	memcpy(&pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp,
		&fmt->fmt.pix_mp, sizeof(struct v4l2_pix_format_mplane));

	return 0;
}

int mtk_pq_display_mdw_set_pix_format_mplane_info(
	struct mtk_pq_device *pq_dev,
	void *bufferctrl)
{

	struct v4l2_pq_s_buffer_info bufferInfo;

	memset(&bufferInfo, 0, sizeof(struct v4l2_pq_s_buffer_info));
	memcpy(&bufferInfo, bufferctrl, sizeof(struct v4l2_pq_s_buffer_info));

	struct format_info fmt_info;
	int ret = 0;
	uint32_t v_aligned = 0;
	uint32_t h_aligned_p = 0;
	uint32_t h_aligned_w[MAX_FPLANE_NUM];
	struct mtk_pq_platform_device *pq_pdev = NULL;
	struct display_device *disp_dev = NULL;
	struct pq_display_mdw_device *mdw_dev = NULL;
#if (1)//(MEMC_CONFIG == 1)
	struct pq_display_frc_device *frc_dev = NULL;
#endif
	struct pq_display_mdw_info *mdw_info = NULL;
	int i = 0;

	if (WARN_ON(!bufferctrl) || WARN_ON(!pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, bufferctrl=0x%x, pq_dev=0x%x\n", bufferctrl, pq_dev);
		return -EINVAL;
	}

	pq_pdev = dev_get_drvdata(pq_dev->dev);
	disp_dev = &pq_pdev->display_dev;
	mdw_dev = &disp_dev->mdw_dev;
#if (1)//(MEMC_CONFIG == 1)
	frc_dev  = &disp_dev->frc_dev;
#endif
	mdw_info = &pq_dev->display_info.mdw;

	memset(&fmt_info, 0, sizeof(fmt_info));
	memset(h_aligned_w, 0, sizeof(h_aligned_w));
	memset(mdw_info->plane_size, 0, sizeof(mdw_info->plane_size));

	pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.width = bufferInfo.width;
	pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.height = bufferInfo.height;

	/* get pixel format info */
	ret = _get_fmt_info(
		pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.pixelformat,
		&fmt_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_get_fmt_info fail, ret = %d\n", ret);
		return ret;
	}

	/* fill info to share mem */
	mdw_info->width
	= pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.width;	//TODO: get from meta
	mdw_info->height
		= pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.height;   //TODO: get from meta
	mdw_info->mem_fmt = pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.pixelformat;
	mdw_info->plane_num = fmt_info.plane_num;
	mdw_info->buffer_num = fmt_info.buffer_num;
	mdw_info->output_path = fmt_info.output_path;

	/* update fmt to user */
	pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.num_planes = mdw_info->buffer_num;

#if (1)//(MEMC_CONFIG == 1)
	if (fmt_info.output_path == PQ_OUTPUT_PATH_WITH_BUF_FRC) {
		for (i = 0; i < mdw_info->plane_num; ++i) {
			// h size align by word
			h_aligned_w[i] = POW2_ALIGN(
			disp_dev->frc_dev.frc_width, disp_dev->frc_dev.frc_h_align);
			// calc size in byte
			mdw_info->frc_ipm_offset = (h_aligned_w[i] * disp_dev->frc_dev.frc_height *
			fmt_info.bpp[i]) / FRAMES_CNT_DIV;// * disp_dev->frc_dev.frc_frame_cnt;

			/*meds*/
			mdw_info->frc_meds_offset = (disp_dev->frc_dev.frc_meds_width *
			disp_dev->frc_dev.frc_meds_height * disp_dev->frc_dev.frc_meds_bpp) /
			FRAMES_CNT_DIV; // * disp_dev->frc_dev.frc_frame_cnt;


			mdw_info->plane_size[i] = (mdw_info->frc_ipm_offset) +
									(mdw_info->frc_meds_offset)+
								(disp_dev->frc_dev.frc_mvbff_size);
		}
	mdw_info->frc_framecount = disp_dev->frc_dev.frc_frame_cnt;

	// frame offset need to transfer to word
	mdw_info->frame_offset[0] = mdw_info->plane_size[0] / disp_dev->bpw;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"[FRC]h_aligned_w[%d], frc_height[%d], bpp[%d], frc_frame_cnt[%d]\n",
		h_aligned_w[0], disp_dev->frc_dev.frc_height, fmt_info.bpp[0],
		disp_dev->frc_dev.frc_frame_cnt);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"[FRC]frc_meds_width[%d], frc_meds_height[%d], meds_bpp[%d], frc_mvbff_size[0x%x]\n",
		disp_dev->frc_dev.frc_meds_width, disp_dev->frc_dev.frc_meds_height,
		disp_dev->frc_dev.frc_meds_bpp, disp_dev->frc_dev.frc_mvbff_size);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"[FRC]h=%d, w=%d, h_align=%d, fmt=0x%x, bpw=%d, buffer_num=%d, plane_num=%d, frame_offset=0x%x frc_ipm_offset=0x%x frc_meds_offset=0x%x\n",
		mdw_info->height, mdw_info->width, disp_dev->frc_dev.frc_h_align,
		mdw_info->mem_fmt, disp_dev->bpw,
		mdw_info->buffer_num, mdw_info->plane_num, mdw_info->frame_offset,
		mdw_info->frc_ipm_offset, mdw_info->frc_meds_offset);
	} else
#endif
	{
	/* calculate frame offset */
	// v size align by pixel
	v_aligned = POW2_ALIGN(pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.height,
		mdw_dev->v_align);
	// h size align by pixel
	h_aligned_p = POW2_ALIGN(pq_dev->pix_format_info.v4l2_fmt_pix_mdw_mp.width,
		mdw_dev->h_align);
	mdw_info->line_offset = h_aligned_p;
	for (i = 0; i < mdw_info->plane_num; ++i) {
		// h size align by word
		h_aligned_w[i] = POW2_ALIGN(
			mdw_info->line_offset * fmt_info.bpp[i], disp_dev->bpw * 8);
		// calc size in byte
		mdw_info->plane_size[i] = (v_aligned * h_aligned_w[i]) / 8;
		// frame offset need to transfer to word
		mdw_info->frame_offset[i] = mdw_info->plane_size[i] / disp_dev->bpw;
	}

	//mdw_info->frame_offset = mdw_info->plane_size[0] / disp_dev->bpw;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"h=%d, w=%d, v_align=%d, h_align=%d, v_aligned=%d, h_aligned_p=%d, fmt=0x%x, bpw=%d, buffer_num=%d, plane_num=%d, frame_offset=0x%x\n",
		mdw_info->height, mdw_info->width, mdw_dev->v_align, mdw_dev->h_align, v_aligned,
		h_aligned_p, mdw_info->mem_fmt, disp_dev->bpw,
		mdw_info->buffer_num, mdw_info->plane_num, mdw_info->frame_offset);
	}

	for (i = 0; i < MAX_FPLANE_NUM; ++i) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"plane[%d]: h_aligned_w=%d, bpp=%d, plane_size=0x%x\n",
		i, h_aligned_w[i], fmt_info.bpp[i], mdw_info->plane_size[i]);
	}

	return 0;
}

int mtk_pq_display_mdw_streamoff(struct pq_display_mdw_info *mdw_info)
{
	if (WARN_ON(!mdw_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, mdw_info=0x%x\n", mdw_info);
		return -EINVAL;
	}

	memset(mdw_info->plane_size, 0, sizeof(mdw_info->plane_size));
	mdw_info->plane_num = 0;
	mdw_info->buffer_num = 0;
	mdw_info->output_mode = 0;
	mdw_info->mem_fmt = 0;
	mdw_info->width = 0;
	mdw_info->height = 0;
	mdw_info->vflip = 0;
	mdw_info->req_off = 0;
	mdw_info->line_offset = 0;
	mdw_info->frame_offset[0] = 0;
	mdw_info->frame_offset[1] = 0;
	mdw_info->frame_offset[MAX_FPLANE_NUM-1] = 0;

	return 0;
}

int mtk_pq_display_mdw_qbuf(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf)
{
	struct m_pq_display_mdw_ctrl meta_mdw;
	struct meta_pq_output_frame_info meta_pqdd_frm;
	struct meta_buffer meta_buf;
	struct vb2_buffer *vb = &pq_buf->vb.vb2_buf;
	int plane_num = 0;
	int i = 0;
	int ret = 0;
	struct dma_buf *meta_db = NULL;
	u32 size = 0;
	struct format_info fmt_info;
	uint64_t addr[MAX_FPLANE_NUM];
	uint64_t offset[MAX_FPLANE_NUM];
	struct mtk_pq_platform_device *pq_pdev = NULL;
	struct display_device *disp_dev = NULL;
	struct pq_display_mdw_device *mdw_dev = NULL;
	struct pq_display_mdw_info *mdw_info = NULL;
	struct pq_display_frc_device *frc_dev = NULL;
	u16 win_id = 0;

	if (WARN_ON(!pq_dev) || WARN_ON(!pq_buf)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x, vb=0x%x\n",
			pq_dev, pq_buf);
		return -EINVAL;
	}

	if (vb->memory != V4L2_MEMORY_DMABUF) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unsupport memory type %d\n", vb->memory);
		return -EINVAL;
	}

	memset(&meta_mdw, 0, sizeof(struct m_pq_display_mdw_ctrl));
	memset(&fmt_info, 0, sizeof(fmt_info));
	memset(addr, 0, sizeof(addr));
	memset(offset, 0, sizeof(offset));
	memset(&meta_buf, 0, sizeof(struct meta_buffer));

	pq_pdev = dev_get_drvdata(pq_dev->dev);
	disp_dev = &pq_pdev->display_dev;
	mdw_dev = &disp_dev->mdw_dev;
	mdw_info = &pq_dev->display_info.mdw;

	win_id = pq_dev->dev_indx;
	plane_num = vb->num_planes;
	frc_dev  = &disp_dev->frc_dev;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "cap, plane_num=%d\n", plane_num);

	if (mdw_info->plane_num > mdw_info->buffer_num) {
		/* contiguous fd: separate by plane size */
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"cap, frame_fd=%d\n", vb->planes[0].m.fd);

		for (i = 0; i < mdw_info->plane_num; ++i) {
			if ((i + 1) < mdw_info->plane_num)
				offset[i + 1] += mdw_info->plane_size[i];

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
				"cap, i=%d, offset=0x%llx\n", i, offset[i]);
		}
	} else {
		/* non contiguous fd: get addr individually */
		for (i = 0; i < mdw_info->buffer_num; ++i) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
				"cap, frame_fd[%d]=%d\n", i, vb->planes[i].m.fd);
		}
	}

	/* meta buffer handle */
	meta_buf.paddr = pq_buf->meta_buf.va;
	meta_buf.size = pq_buf->meta_buf.size;

	/* mapping pixfmt v4l2 -> meta & get format info */
	ret = _get_fmt_info(
		mdw_info->mem_fmt,
		&fmt_info);

	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_get_fmt_info fail, ret=%d\n", ret);
		return ret;
	}

	/* get meta info from share mem */
	memcpy(meta_mdw.plane_offset, offset, sizeof(meta_mdw.plane_offset));
	meta_mdw.mem_fmt = fmt_info.meta_fmt;
	meta_mdw.width = mdw_info->width;		// TODO: get REAL size from scaling
	meta_mdw.height = mdw_info->height;	// TODO: get REAL size from scaling
	meta_mdw.vflip = mdw_info->vflip;
	meta_mdw.line_offset = mdw_info->line_offset;
	memcpy(meta_mdw.frame_offset, mdw_info->frame_offset, sizeof(meta_mdw.frame_offset));
	//meta_mdw.frame_offset = mdw_info->frame_offset;
	meta_mdw.bpw = disp_dev->bpw;
	meta_mdw.frc_ipversion = frc_dev->frc_ip_version;
	if (mdw_info->output_mode == MTK_PQ_OUTPUT_MODE_BYPASS)
		meta_mdw.output_path = PQ_OUTPUT_PATH_WITHOUT_BUF;
	else
		meta_mdw.output_path = fmt_info.output_path;
#if (1)//(MEMC_CONFIG == 1)
	if (fmt_info.output_path == PQ_OUTPUT_PATH_WITH_BUF_FRC) {
		#if (1)
		meta_mdw.frc_mvbff_addr = disp_dev->frc_dev.frc_mvbff_adr;
		meta_mdw.frc_aid_enable = pq_dev->display_info.secure_info.svp_enable;
		#endif
		meta_mdw.frc_meds_addr = addr[0] + mdw_info->frc_ipm_offset;
	}
#endif


	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"%s %s0x%x, %s%d, %s%d, %s%d, %s%d, %s%d, %s0x%x, %s%d, %s0x%x, %s%d\n",
		"mdw w meta:",
		"fmt = ", meta_mdw.mem_fmt,
		"w = ", meta_mdw.width,
		"h = ", meta_mdw.height,
		"vflip = ", meta_mdw.vflip,
		"req_off = ", meta_mdw.req_off,
		"line_offset = ", meta_mdw.line_offset,
		"frame_offset[0] = ", meta_mdw.frame_offset[0],
		"frame_offset[1] = ", meta_mdw.frame_offset[1],
		"frame_offset[2] = ", meta_mdw.frame_offset[MAX_FPLANE_NUM-1],
		"output_path = ", meta_mdw.output_path,
		"vb = ", meta_mdw.vb,
		"bpw = ", meta_mdw.bpw);

	for (i = 0; i < MAX_FPLANE_NUM; ++i) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
			"plane_offset[%d] = 0x%llx\n", i, meta_mdw.plane_offset[i]);
	}

	/* set meta buf info */
	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_DISP_MDW_CTRL, &meta_mdw);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_write_metadata_addr fail, ret=%d\n", ret);
		return ret;
	}

	/* call CPU IF (window id)*/

	/* get info from metadata */
	memset(&meta_mdw, 0, sizeof(struct m_pq_display_mdw_ctrl));
	ret = mtk_pq_common_read_metadata_addr(&meta_buf,
		EN_PQ_METATAG_DISP_MDW_CTRL, &meta_mdw);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_DISP_MDW_CTRL Failed, ret = %d\n", ret);
		return ret;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_DISP_MDW_CTRL success\n");
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"%s %s0x%x, %s%d, %s%d, %s%d, %s%d, %s%d, %s%d, %s0x%x\n",
		"mdw w meta:",
		"fmt = ", meta_mdw.mem_fmt,
		"w = ", meta_mdw.width,
		"h = ", meta_mdw.height,
		"vflip = ", meta_mdw.vflip,
		"req_off = ", meta_mdw.req_off,
		"line_offset = ", meta_mdw.line_offset,
		"frame_offset[0] = ", meta_mdw.frame_offset[0],
		"frame_offset[1] = ", meta_mdw.frame_offset[1],
		"frame_offset[2] = ", meta_mdw.frame_offset[MAX_FPLANE_NUM-1],
		"vb = ", meta_mdw.vb);

	/* write meta pq->render */
	memset(&meta_pqdd_frm, 0, sizeof(struct meta_pq_output_frame_info));
	meta_pqdd_frm.width = meta_mdw.width;
	meta_pqdd_frm.height = meta_mdw.height;
	for (i = 0; i < MAX_FPLANE_NUM; ++i)
		meta_pqdd_frm.fd_offset[i] = offset[i];
	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_OUTPUT_FRAME_INFO, &meta_pqdd_frm);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_write_metadata_addr Failed, ret = %d\n", ret);
		return ret;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"%s %s%d, %s%d, %s0x%llx, %s0x%llx, %s0x%llx\n",
		"pqdd frm info w meta:",
		"w = ", meta_pqdd_frm.width,
		"h = ", meta_pqdd_frm.height,
		"fd_offset[0] = ", meta_pqdd_frm.fd_offset[0],
		"fd_offset[1] = ", meta_pqdd_frm.fd_offset[1],
		"fd_offset[2] = ", meta_pqdd_frm.fd_offset[2]);

	return 0;
}

void mtk_pq_display_mdw_qbuf_done(u8 win_id, __u64 meta_db_ptr)
{
	struct dma_buf *meta_db = (struct dma_buf *)meta_db_ptr;
	struct m_pq_display_mdw_ctrl meta_mdw;
	struct vb2_buffer *vb = NULL;
	struct v4l2_event ev;
	struct vb2_v4l2_buffer *vbuf = NULL;
	struct mtk_pq_ctx *ctx = NULL;
	struct mtk_pq_device *pq_dev = NULL;
	int ret = 0;

	memset(&ev, 0, sizeof(struct v4l2_event));

	/* read meta from dma_buf */
	ret = mtk_pq_common_read_metadata_db(
		meta_db, EN_PQ_METATAG_DISP_MDW_CTRL, &meta_mdw);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_read_metadata_db fail, ret=%d\n", ret);
	}

	/* get vb from meta */
	vb = (struct vb2_buffer *)meta_mdw.vb;

	/* delete meta tag */
	ret = mtk_pq_common_delete_metadata_db(meta_db, EN_PQ_METATAG_DISP_MDW_CTRL);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_delete_metadata_db fail, ret=%d\n", ret);
	}
}
