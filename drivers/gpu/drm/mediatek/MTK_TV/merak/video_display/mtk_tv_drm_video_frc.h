/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_TV_DRM_VIDEO_FRC_H_
#define _MTK_TV_DRM_VIDEO_FRC_H_

#include "ext_command_frc_if.h"
#include "mtk_tv_drm_video_display.h"

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define  MEMC_BLOCK_H_ALIGN 16
#define  MEMC_BLOCK_V_ALIGN 16


//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------
typedef enum MEMC_Version_Index {
	Version_Major                = 0x00,
	Version_Minor                = 0x01,
};

typedef enum Frc_command_list {
	//Get command
	FRC_MB_CMD_GET_SW_VERSION                       = 0x02,
	FRC_MB_CMD_GET_RWDIFF                           = 0x03,

	//Set command
	FRC_MB_CMD_INIT                                 = 0x04,
	FRC_MB_CMD_SET_TIMING                           = 0x05,
	FRC_MB_CMD_SET_INPUT_FRAME_SIZE                 = 0x06,
	FRC_MB_CMD_SET_OUTPUT_FRAME_SIZE                = 0x07,
	FRC_MB_CMD_SET_FPLL_LOCK_DONE                   = 0x08,
	FRC_MB_CMD_SET_FRC_MODE                         = 0x0F,
	FRC_MB_CMD_SET_MFC_MODE                         = 0x10,
	FRC_MB_CMD_SET_FRC_GAME_MODE                    = 0x11,
	FRC_MB_CMD_SET_MFC_DEMO_MODE	                = 0x12,
	FRC_MB_CMD_60_REALCINEMA_MODE                   = 0x13,
	FRC_MB_CMD_SET_MFC_DECODE_IDX_DIFF              = 0x20,
	FRC_MB_CMD_SET_MFC_FRC_DS_SETTINGS              = 0x21,
	FRC_MB_CMD_SET_MFC_CROPWIN_CHANGE               = 0x22,
	FRC_MB_CMD_MEMC_MASK_WINDOWS_H                  = 0x30,
	FRC_MB_CMD_MEMC_MASK_WINDOWS_V                  = 0x31,
	FRC_MB_CMD_MEMC_ACTIVE_WINDOWS_H                = 0x32,
	FRC_MB_CMD_MEMC_ACTIVE_WINDOWS_V                = 0x33,
	FRC_MB_CMD_FILM_CADENCE_CTRL                    = 0x34,
	FRC_MB_CMD_INIT_DONE                            = 0x35,
	FRC_MB_CMD_SET_SHARE_MEMORY_ADDR                = 0x36,
	FRC_MB_CMD_SET_QoS                            = 0x37,
	FRC_MB_CMD_GET_QoS                            = 0x38,

	//SONY Command
	FRC_MB_CMD_SET_MEMC_DEBUG_CADENCE               = 0x40,
	FRC_MB_CMD_SET_MEMC_DEBUG_OUTSR                 = 0x41,
	FRC_MB_CMD_SET_MEMC_DEBUG_FALLBACKW3            = 0x42,
	FRC_MB_CMD_SET_MEMC_DEBUG_FALLBACKW0            = 0x43,
	FRC_MB_CMD_SET_MEMC_DEBUG_MOTION                = 0x44,
	FRC_MB_CMD_SET_MEMC_DEBUG_SHOW_FILM_ERROR       = 0x45,
	FRC_MB_CMD_SET_MEMC_DEBUG_AUTOMODE              = 0x46,
	FRC_MB_CMD_SET_MEMC_DEBUG_AUTOMODE_FINAL        = 0x47,
	FRC_MB_CMD_SET_MEMC_DEBUG_OSDMASK               = 0x48,
	FRC_MB_CMD_SET_MEMC_DEBUG_HALO_REDUCTION        = 0x49,
	FRC_MB_CMD_SET_MEMC_DEBUG_SEARCH_RANGE          = 0x4A,
	FRC_MB_CMD_SET_MEMC_DEBUG_PERIODICAL            = 0x4B,
	FRC_MB_CMD_SET_MEMC_DEBUG_BADEDIT               = 0x4C,
	FRC_MB_CMD_SET_MEMC_DEBUG_MVSTICKING            = 0x4D,
	FRC_MB_CMD_SET_MEMC_DEBUG_SMALL_OBJECT          = 0x4E,
	FRC_MB_CMD_SET_MEMC_DEBUG_SPOTLIGHT             = 0x4F,
	FRC_MB_CMD_SET_MEMC_DEBUG_BOUNDARY_HALO         = 0x50,
	FRC_MB_CMD_SET_MEMC_DEBUG_POSITION_RATIO_TABLE  = 0x51,
	FRC_MB_CMD_SET_MEMC_DEBUG_IMPREMENTATION_FRAME  = 0x52,
	FRC_MB_CMD_SET_MEMC_RELPOS_RATIO_INDEX          = 0x53,

	//Send burst command
	FRC_MB_CMD_SET_MECM_LEVEL                       = 0x80,

	//Get command for MEMC alg LTP
	FRC_MB_CMD_GET_MECM_ALG_STATUS                  = 0xF0,
};

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------
struct mtk_frc_timinginfo_tbl {
	uint32_t input_freq_L;
	uint32_t input_freq_H;
	uint32_t frc_memc_latency;
	uint32_t frc_game_latency;
};

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
uint32_t mtk_video_display_frc_set_Qos(uint8_t QosMode);
uint32_t mtk_video_display_frc_get_Qos(uint8_t *pQosMode);

void mtk_video_display_frc_suspend(struct device *dev);
void mtk_video_display_frc_init(struct device *dev, struct mtk_video_context *pctx_video);

void mtk_video_display_set_frc_ins(unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct dma_buf *meta_db,
	struct drm_framebuffer *fb);

void mtk_video_display_set_frc_opm_path(struct mtk_plane_state *state,
						 bool enable);
void mtk_video_display_set_frc_property(
	struct video_plane_prop *plane_props,
	enum ext_video_plane_prop prop);
void mtk_video_display_get_frc_property(
	struct video_plane_prop *plane_props,
	enum ext_video_plane_prop prop);
void mtk_video_display_set_frc_base_adr(uint64_t adr);

void mtk_video_display_frc_set_pattern(
		struct mtk_tv_kms_context *pctx,
		enum mtk_video_frc_pattern sel);

void mtk_video_display_set_frc_freeze(
		struct mtk_video_context *pctx_video,
		bool enable);
#endif
