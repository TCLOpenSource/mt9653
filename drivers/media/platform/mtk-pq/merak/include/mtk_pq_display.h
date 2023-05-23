/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_DISPLAY_H
#define _MTK_PQ_DISPLAY_H
#include "mtk_pq_display_b2r.h"
#include "mtk_pq_display_mdw.h"
#include "mtk_pq_display_idr.h"
#include "mtk_pq_svp.h"
#include "mtk_pq_buffer.h"

//-----------------------------------------------------------------------------
//  Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  Macro and Define
//-----------------------------------------------------------------------------
#define PQ_ABF_BLK_MODE1	(1)
#define PQ_ABF_BLK_MODE2	(2)
#define PQ_ABF_BLK_MODE3	(3)
#define PQ_ABF_BLK_MODE4	(4)

#define PQ_ABF_BLK_SIZE1	(8)
#define PQ_ABF_BLK_SIZE2	(16)
#define PQ_ABF_BLK_SIZE3	(32)
#define PQ_ABF_BLK_SIZE4	(64)

#define PQ_ABF_CUTR_MODE1_SIZE	(960)
#define PQ_ABF_CUTR_MODE2_SIZE	(1920)
#define PQ_ABF_CUTR_MODE3_SIZE	(3840)

#define PQ_CTUR2_EXT_BIT	(2)

//-----------------------------------------------------------------------------
//  Enum
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  Structure
//-----------------------------------------------------------------------------
struct pq_display_aisr_dbg {
	bool bdbgModeEn;
	bool bAISREn;
	uint32_t u32Scale;
	struct v4l2_pq_aisr_active_win aisrActiveWin;
};

struct v4l2_display_info {
	struct task_struct *mvop_task;
	bool bReleaseTaskCreated;
	bool bCallBackTaskCreated;
	struct pq_display_mdw_info mdw;
	struct pq_display_idr_info idr;
	struct pq_buffer BufferTable[PQU_BUF_MAX];
#if IS_ENABLED(CONFIG_OPTEE)
	struct pq_secure_info secure_info;
#endif
	struct v4l2_pq_flow_control flowControl;
	struct v4l2_pq_aisr_active_win aisrActiveWin;
};

struct display_device {
	uint8_t bpw;		//byte per word
	uint8_t buf_iommu_offset;
	uint8_t pq_iommu_idx_scmi;
	uint8_t pq_iommu_idx_ucm;
	uint8_t pixel_align;
	uint16_t h_max_size;
	uint16_t v_max_size;
	struct pq_buffer **pReserveBufTbl;
	struct pq_display_mdw_device mdw_dev;
#if (1)//(MEMC_CONFIG == 1)
	struct pq_display_frc_device frc_dev;
#endif
	uint16_t znr_me_h_max_size;
	uint16_t znr_me_v_max_size;
};

enum AISR_DBG_TYPE {
	MTK_PQ_AISR_DBG_NONE,
	MTK_PQ_AISR_DBG_ENABLE,
	MTK_PQ_AISR_DBG_ACTIVE_WIN,
	MTK_PQ_AISR_DBG_MAX,
};
//-----------------------------------------------------------------------------
//  Function and Variable
//-----------------------------------------------------------------------------
int mtk_display_set_frame_metadata(struct mtk_pq_device *pq_dev, struct mtk_pq_buffer *pq_buf);
void mtk_display_abf_blk_mode(int width, uint16_t *ctur_mode, uint16_t *ctur2_mode);
int mtk_display_dynamic_ultra_init(struct mtk_pq_platform_device *pqdev);
int mtk_display_parse_dts(struct display_device *display_dev, struct mtk_pq_platform_device *pqdev);
int mtk_pq_register_display_subdev(
	struct v4l2_device *pq_dev,
	struct v4l2_subdev *subdev_display,
	struct v4l2_ctrl_handler *display_ctrl_handler);
void mtk_pq_unregister_display_subdev(
	struct v4l2_subdev *subdev_display);
int mtk_pq_aisr_dbg_show(struct device *dev, char *buf);
int mtk_pq_aisr_dbg_store(struct device *dev, const char *buf);

#endif
