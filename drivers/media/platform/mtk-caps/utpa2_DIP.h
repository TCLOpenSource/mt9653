/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _UTPA2_DIP_H
#define _UTPA2_DIP_H

#define MTK_DIP_SVC_INFO_VERSION (1) //Need to aligned with TEE driver header
#define IN_PLANE_MAX (4)

//utopia TEE header
typedef enum {
	E_DIP_REE_TO_TEE_CMD_BIND_PIPEID, //Param1: Pipeline id,Param2: Pipe id
	E_DIP_REE_TO_TEE_CMD_SDC_SETTING,
	E_DIP_REE_TO_TEE_CMD_MAX,
} EN_DIP_REE_TO_TEE_CMD_TYPE;

typedef enum {
	E_DIP0_ENG = 0,
	E_DIP1_ENG = 1,
	E_DIP2_ENG = 2,
	E_DIP_MAX_ENG,
} EN_DIP_ENG;

typedef enum {
	E_SVC_IN_ADDR_Y = 0x00,
	E_SVC_IN_ADDR_C,
	E_SVC_IN_ADDR_YBLN,
	E_SVC_IN_ADDR_CBLN,
	E_SVC_IN_ADDR_MAX,
} EN_SVC_IN_ADDR;

/// Define source type for DIP
typedef enum {
	E_DIP_SRC_SRCCAP_MAIN = 0,
	E_DIP_SRC_SRCCAP_SUB,
	E_DIP_SRC_PQ_START,
	E_DIP_SRC_PQ_HDR,
	E_DIP_SRC_PQ_SHARPNESS,
	E_DIP_SRC_PQ_SCALING,
	E_DIP_SRC_PQ_CHROMA_COMPENSAT,
	E_DIP_SRC_PQ_BOOST,
	E_DIP_SRC_PQ_COLOR_MANAGER,
	E_DIP_SRC_STREAM_ALL_VIDEO,
	E_DIP_SRC_STREAM_ALL_VIDEO_OSDB,
	E_DIP_SRC_STREAM_POST,
	E_DIP_SRC_OSD,
	E_DIP_SRC_DIPR,
	E_DIP_SRC_B2R,
	E_DIP_SRC_MAX          //< capture point max >
} EN_DIP_SRC;

typedef struct{
	u32 u32version;
	u32 u32length;
	EN_DIP_ENG enDIP;
	u8 u8Enable;
	EN_DIP_SRC enSrc;
	u32 u32OutWidth;
	u32 u32OutHeight;
	u32 u32CropWidth;
	u32 u32CropHeight;
	u8 u8InAddrshift;
	u32 u32PlaneCnt;
	u64 u64InputAddr[IN_PLANE_MAX];
	u32 u32BufCnt;
	u32 u32InputSize[IN_PLANE_MAX];
	u32 u32InputXStr;
	u32 u32InputYStr;
	u32 u32InputWidth;
	u32 u32InputHeight;
} ST_DIP_SVC_INFO;

#endif
