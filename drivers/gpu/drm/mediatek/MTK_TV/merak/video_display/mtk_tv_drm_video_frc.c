// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/math64.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/dma-buf.h>

#include <drm/mtk_tv_drm.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>

#include "../mtk_tv_drm_plane.h"
#include "../mtk_tv_drm_gem.h"
#include "../mtk_tv_drm_kms.h"
#include "mtk_tv_drm_common.h"
#include "mtk_tv_drm_log.h"
#include "drv_scriptmgt.h"
#if (1) //(MEMC_CONFIG == 1)
#include "hwreg_render_video_frc.h"
#endif
#include "hwreg_render_video_display.h"
#include "mtk_tv_drm_video_frc.h"
#include "m_pqu_pq.h"

#define free kfree
#define malloc(size) kmalloc((size), GFP_KERNEL)

#define FRC_PQ_BUFFER_SIZE   0xC00000 /*12MB*/

#define FRC_IP_VERSION_1       (0x01)
#define FRC_IP_VERSION_2       (0x02)


#define LOCK                    (1)
#define MEMC_RW_DIFF_VAL        (4)
#define VFRQRATIO_10            (10)
#define VFRQRATIO_1000          (1000)
#define SIZERATIO_16            (16)

#define FRC_MODE_CNT            (2)
#define MEMC_MODE_CNT           (4)
#define MEMC_LEVLE_MAX_CNT      (39)
#define DEMO_MODE_CNT           (2)

#define DATA_D0                 (0)
#define DATA_D1                 (1)
#define DATA_D2                 (2)
#define DATA_D3                 (3)

//CMD DEFAULT VALUE
#define DEFAULT_VALUE           (0x00)
#define DEFAULT_MEMC_MODE       (0x01)
#define DEFAULT_FRC_MODE        (0x00)
#define DEFAULT_DEMO_MODE       (0x00)
#define DEFAULT_PARTITION_MODE  (0x00)
#define DEFAULT_CADENCE_CTRL    (0xFF)
#define DEFAULT_FPLL_LOCK       (0xFF)
#define DEFAULT_PRE_VALUE       (0xFF)

#define LIMIT_MEMC_LEVEL        (0xFF)

#define MEMC_LEVEL_OFF          (0x00)
#define MEMC_LEVEL_LOW          (0x07)
#define MEMC_LEVEL_MIDDLE       (0x0E)
#define MEMC_LEVEL_HIGH         (0x15)

#define INIT_FREQ               (0x3C) // 60Hz
#define INIT_H_SIZE             (0x1E00) // 7680
#define INIT_V_SIZE             (0x10E0) // 4320

#define INIT_VALUE_FAIL         (0x00)
#define INIT_VALUE_PASS         (0xFF)

#define DEFAULT_CMD_STATUS      (0xFF)


#define NO_UPDATE               (0)
#define UPDATE                  (1)

//For LTP
#define CMD_STATUS             (0x000FFFFFFFF)
#define INPUT_SIZE_CMD_STATUS  (0x00000000001)
#define OUTPUT_SIZE_CMD_STATUS (0x00000000002)
#define FPLL_LOCK_CMD_STATUS   (0x00000000004)
#define TIMING_CMD_STATUS      (0x00000000008)
#define FRC_MODE_CMD_STATUS    (0x00000000010)
#define MFC_MODE_CMD_STATUS    (0x00000000020)
#define MFC_LEVEL_CMD_STATUS   (0x00000000040)
#define MFC_DEMO_CMD_STATUS    (0x00000000080)
#define MFC_ALG_STATUS         (0x00000000100)
#define MFC_PW_STATUS         (0x00000000200)

#define MEMC_STATUS            (0x00700000000)
#define MEMC_ON_STATUS         (0x00100000000)
#define MEMC_OFF_STATUS        (0x00200000000)
#define MEMC_AUTO_STATUS       (0x00400000000)

#define DEMO_STATUS            (0x07800000000)
#define DEMO_ON_ALL_STATUS     (0x00800000000)
#define DEMO_ON_LEFT_STATUS    (0x01000000000)
#define DEMO_ON_RIGHT_STATUS   (0x02000000000)
#define DEMO_OFF_STATUS        (0x04000000000)

#define MEMC_LV_STATUS         (0x78000000000)
#define MEMC_LV_OFF_STATUS     (0x08000000000)
#define MEMC_LV_LOW_STATUS     (0x10000000000)
#define MEMC_LV_MID_STATUS     (0x20000000000)
#define MEMC_LV_HIGH_STATUS    (0x40000000000)
//----------------------------------------------
#define MTK_DRM_MODEL MTK_DRM_MODEL_DISPLAY

//FRC mode setting
#define MEMC_MODE_OFF     (0)
#define MEMC_MODE_ON      (1)
#define MEMC_MODE_AUTO    (2)

//Demo mode setting
#define DEMO_MODE_OFF            (0)
#define DEMO_MODE_ON_LEFT_RIGHT  (1)
#define DEMO_MODE_ON_TOP_DOWN    (2)

//Demo PARTITION setting
#define DEMO_PARTITION_LEFT_OR_TOP      (0)
#define DEMO_PARTITION_RIGHT_OR_DOWN    (1)

//DS ALIGNMENT
#define DS_ALIGNMENT    (4)

#define _24HZ 24000
#define _25HZ 25000
#define _30HZ 30000
#define _48HZ 48000
#define _50HZ 50000
#define _60HZ 60000
#define _0HZ 0
#define _100HZ 100000
#define _tHZ 500

#define _60HZ_LATENCY 0x40
#define _60HZ_LOW_LATENCY 0x10

#define _120HZ_LATENCY 0x35
#define _120HZ_LOW_LATENCY 0x05

struct mtk_frc_timinginfo_tbl frc_timing_60Hz_table[] = {
		{_24HZ-_tHZ, _24HZ+_tHZ, _60HZ_LATENCY, _60HZ_LOW_LATENCY},//24Hz
		{_25HZ-_tHZ, _25HZ+_tHZ, _60HZ_LATENCY, _60HZ_LOW_LATENCY},//25Hz
		{_30HZ-_tHZ, _30HZ+_tHZ, _60HZ_LATENCY, _60HZ_LOW_LATENCY},//30Hz
		{_48HZ-_tHZ, _48HZ+_tHZ, _60HZ_LATENCY, _60HZ_LOW_LATENCY},//48Hz
		{_50HZ-_tHZ, _50HZ+_tHZ, _60HZ_LATENCY, _60HZ_LOW_LATENCY},//50Hz
		{_60HZ-_tHZ, _60HZ+_tHZ, _60HZ_LATENCY, _60HZ_LOW_LATENCY},//60Hz
		{          _0HZ,         _100HZ, _60HZ_LATENCY, _60HZ_LOW_LATENCY} //Others
};

struct mtk_frc_timinginfo_tbl frc_timing_120Hz_table[] = {
		{_24HZ-_tHZ, _24HZ+_tHZ, _120HZ_LATENCY, _120HZ_LOW_LATENCY},//24Hz
		{_25HZ-_tHZ, _25HZ+_tHZ, _120HZ_LATENCY, _120HZ_LOW_LATENCY},//25Hz
		{_30HZ-_tHZ, _30HZ+_tHZ, _120HZ_LATENCY, _120HZ_LOW_LATENCY},//30Hz
		{_48HZ-_tHZ, _48HZ+_tHZ, _120HZ_LATENCY, _120HZ_LOW_LATENCY},//48Hz
		{_50HZ-_tHZ, _50HZ+_tHZ, _120HZ_LATENCY, _120HZ_LOW_LATENCY},//50Hz
		{_60HZ-_tHZ, _60HZ+_tHZ, _120HZ_LATENCY, _120HZ_LOW_LATENCY},//60Hz
		{          _0HZ,         _100HZ, _120HZ_LATENCY, _120HZ_LOW_LATENCY} //Others
};
#define _FREQ_100HZ 100



static uint64_t memc_init_value_for_rv55 = DEFAULT_VALUE;
static uint64_t memc_rv55_info = DEFAULT_VALUE;

//FRC_Mode = MFCMode,FRC_Mode
static uint8_t FRCMode[FRC_MODE_CNT] = {DEFAULT_MEMC_MODE, DEFAULT_FRC_MODE};

//MFCMode = MFCMode,DeJudderLevel,DeBlurLevel,FRC_Mode
//static uint8_t MFCMode[MEMC_MODE_CNT]= {0x01,0x00,0x00,0x00};

//For LTP test
//MFClevel 0~17 = Off, 18~39 = High
static uint8_t LTP_MFCLevel_OFF[MEMC_LEVLE_MAX_CNT] = {
MEMC_LEVEL_OFF, MEMC_LEVEL_OFF, MEMC_LEVEL_OFF,
MEMC_LEVEL_OFF, MEMC_LEVEL_OFF, MEMC_LEVEL_OFF,
MEMC_LEVEL_OFF, MEMC_LEVEL_OFF, MEMC_LEVEL_OFF,
MEMC_LEVEL_OFF, MEMC_LEVEL_OFF, MEMC_LEVEL_OFF,
MEMC_LEVEL_OFF, MEMC_LEVEL_OFF, MEMC_LEVEL_OFF,
MEMC_LEVEL_OFF, MEMC_LEVEL_OFF, MEMC_LEVEL_OFF,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH
};

//MFClevel 0~17 = Low, 18~39 = High
static uint8_t LTP_MFCLevel_Low[MEMC_LEVLE_MAX_CNT] = {
MEMC_LEVEL_LOW, MEMC_LEVEL_LOW, MEMC_LEVEL_LOW,
MEMC_LEVEL_LOW, MEMC_LEVEL_LOW, MEMC_LEVEL_LOW,
MEMC_LEVEL_LOW, MEMC_LEVEL_LOW, MEMC_LEVEL_LOW,
MEMC_LEVEL_LOW, MEMC_LEVEL_LOW, MEMC_LEVEL_LOW,
MEMC_LEVEL_LOW, MEMC_LEVEL_LOW, MEMC_LEVEL_LOW,
MEMC_LEVEL_LOW, MEMC_LEVEL_LOW, MEMC_LEVEL_LOW,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH
};

//MFClevel 0~17 = Middle,18~39 = High,
static uint8_t LTP_MFCLevel_Mid[MEMC_LEVLE_MAX_CNT] = {
MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE,
MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE,
MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE,
MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE,
MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE,
MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE, MEMC_LEVEL_MIDDLE,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH
};

//MFClevel 0~39 = High
static uint8_t LTP_MFCLevel_High[MEMC_LEVLE_MAX_CNT] = {
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH
};
//---------------------------------------------------

//MFClevel 0~39 = default:21
static uint8_t MFCLevel[MEMC_LEVLE_MAX_CNT] = {
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH,
MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH, MEMC_LEVEL_HIGH
};

//DEMO_Mode = DEMO_MODE,PARTITION_MODE
static uint8_t DEMOMode[FRC_MODE_CNT] = {DEFAULT_DEMO_MODE, DEFAULT_PARTITION_MODE};

enum DS_SYNC {
	DS_SRC0_SYNC,
	DS_SRC1_SYNC,
	DS_IP_SYNC,
	DS_OPF_SYNC,
	DS_OPB_SYNC,
	DS_OP2_SYNC,
	DS_DISP_SYNC,
	DS_SYNC_MAX,
};

static bool _mtk_video_display_frc_is_variable_updated(
	uint64_t oldVar,
	uint64_t newVar)
{
	bool update = NO_UPDATE;

	if (newVar != oldVar)
		update = UPDATE;

	return update;
}

static void _mtk_video_display_set_init_done_for0tv55(void)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};

	//FRC_MB_CMD_INIT_DONE
	send.CmdID = FRC_MB_CMD_INIT_DONE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);
}

static void _mtk_video_display_set_init_value_for_rv55(void)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};

	memc_init_value_for_rv55 = INIT_VALUE_FAIL;
	//FRC_MB_CMD_INIT
	send.CmdID = FRC_MB_CMD_INIT;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

    //FRC_MB_CMD_SET_TIMING
	send.CmdID = FRC_MB_CMD_SET_TIMING;
	send.D1    = (uint32_t)INIT_FREQ;  //60Hz
	send.D2    = (uint32_t)INIT_FREQ;  //60Hz
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_INPUT_FRAME_SIZE
	send.CmdID = FRC_MB_CMD_SET_INPUT_FRAME_SIZE;
	send.D1    = (uint32_t)INIT_H_SIZE;  // 7680
	send.D2    = (uint32_t)INIT_V_SIZE;  // 4320
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_OUTPUT_FRAME_SIZE
	send.CmdID = FRC_MB_CMD_SET_OUTPUT_FRAME_SIZE;
	send.D1    = (uint32_t)INIT_H_SIZE;  // 7680
	send.D2    = (uint32_t)INIT_V_SIZE;  // 4320
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	#if (0)
    //FRC_MB_CMD_SET_FPLL_LOCK_DONE
	send.CmdID = FRC_MB_CMD_SET_FPLL_LOCK_DONE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);
	#endif

    //FRC_MB_CMD_SET_FRC_MODE
	send.CmdID = FRC_MB_CMD_SET_FRC_MODE;
	send.D1    = DEFAULT_MEMC_MODE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	#if (0)
	//FRC_MB_CMD_SET_MFC_MODE
	send.CmdID = FRC_MB_CMD_SET_MFC_MODE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_FRC_GAME_MODE
	send.CmdID = FRC_MB_CMD_SET_FRC_GAME_MODE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_MFC_DEMO_MODE
	send.CmdID = FRC_MB_CMD_SET_MFC_DEMO_MODE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_60_REALCINEMA_MODE
	send.CmdID = FRC_MB_CMD_60_REALCINEMA_MODE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_MFC_DECODE_IDX_DIFF
	send.CmdID = FRC_MB_CMD_SET_MFC_DECODE_IDX_DIFF;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_MFC_FRC_DS_SETTINGS
	send.CmdID = FRC_MB_CMD_SET_MFC_FRC_DS_SETTINGS;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_MFC_CROPWIN_CHANGE
	send.CmdID = FRC_MB_CMD_SET_MFC_CROPWIN_CHANGE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_MEMC_MASK_WINDOWS_H
	send.CmdID = FRC_MB_CMD_MEMC_MASK_WINDOWS_H;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_MEMC_MASK_WINDOWS_V
	send.CmdID = FRC_MB_CMD_MEMC_MASK_WINDOWS_V;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_MEMC_ACTIVE_WINDOWS_H
	send.CmdID = FRC_MB_CMD_MEMC_ACTIVE_WINDOWS_H;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_MEMC_ACTIVE_WINDOWS_V
	send.CmdID = FRC_MB_CMD_MEMC_ACTIVE_WINDOWS_V;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);
	#endif

	//FRC_MB_CMD_FILM_CADENCE_CTRL
	send.CmdID = FRC_MB_CMD_FILM_CADENCE_CTRL;
	send.D1    = (uint32_t)DEFAULT_CADENCE_CTRL;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	//FRC_MB_CMD_SET_SHARE_MEMORY_ADDR
	send.CmdID = FRC_MB_CMD_SET_SHARE_MEMORY_ADDR;
	send.D1    = (uint32_t)DEFAULT_VALUE;  //Need Get share memory address
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);

	#if (0)
	//FRC_MB_CMD_INIT_DONE
	send.CmdID = FRC_MB_CMD_INIT_DONE;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	pqu_send_cmd(&send, &callback_info);
	#endif
	memc_init_value_for_rv55 = INIT_VALUE_PASS;
}

//set memc input size
static void _mtk_video_display_set_input_frame_size(struct mtk_tv_kms_context *pctx,
				struct mtk_plane_state *state)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};

	static uint32_t pre_width  = DEFAULT_VALUE;
	static uint32_t pre_height = DEFAULT_VALUE;
	static uint32_t cmdstatus  = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;

	if ((force == TRUE) ||
		(_mtk_video_display_frc_is_variable_updated(
				pre_width, (state->base.src_w>>SIZERATIO_16))) ||
		(_mtk_video_display_frc_is_variable_updated(
				pre_height, (state->base.src_h>>SIZERATIO_16)))
	){
		DRM_INFO("[%s,%d] pre_width = 0x%x, pre_height = 0x%x\n",
			__func__, __LINE__, pre_width, pre_height);
		send.CmdID = FRC_MB_CMD_SET_INPUT_FRAME_SIZE;
		send.D1    = (state->base.src_w>>SIZERATIO_16);
		send.D2    = (state->base.src_h>>SIZERATIO_16);
		send.D3    = DEFAULT_VALUE;
		send.D4    = DEFAULT_VALUE;
		cmdstatus = pqu_send_cmd(&send, &callback_info);
		pre_width  = (state->base.src_w>>SIZERATIO_16);
		pre_height = (state->base.src_h>>SIZERATIO_16);
		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			__func__, __LINE__, send.D1, send.D2, send.D3, send.D4);
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|INPUT_SIZE_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~INPUT_SIZE_CMD_STATUS));
	}
}

//set memc output size
static void _mtk_video_display_set_output_frame_size(struct mtk_tv_kms_context *pctx,
				struct mtk_plane_state *state)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};

	static uint32_t pre_width  = DEFAULT_VALUE;
	static uint32_t pre_height = DEFAULT_VALUE;
	static uint32_t cmdstatus  = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;

	if ((force == TRUE) ||
		(_mtk_video_display_frc_is_variable_updated(
				pre_width, state->base.crtc_w)) ||
		(_mtk_video_display_frc_is_variable_updated(
				pre_height, state->base.crtc_h))
	){
		DRM_INFO("[%s,%d] pre_width = 0x%x, pre_height = 0x%x\n",
			__func__, __LINE__, pre_width, pre_height);
		send.CmdID = FRC_MB_CMD_SET_MFC_FRC_DS_SETTINGS;
		send.D1    = state->base.crtc_w;
		send.D2    = state->base.crtc_h;
		send.D3    = DEFAULT_VALUE;
		send.D4    = DEFAULT_VALUE;
		cmdstatus  = pqu_send_cmd(&send, &callback_info);

		pre_width  = state->base.crtc_w;
		pre_height = state->base.crtc_h;
		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			__func__, __LINE__, send.D1, send.D2, send.D3, send.D4);
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|OUTPUT_SIZE_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~OUTPUT_SIZE_CMD_STATUS));
	}
}

//set lpll lock done
static void _mtk_video_display_set_fpll_lock_done(struct mtk_tv_kms_context *pctx)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;

	static bool fpll_lock     = DEFAULT_VALUE;
	static bool pre_fpll_lock = DEFAULT_FPLL_LOCK;
	static uint32_t cmdstatus = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub) {
		force = TRUE;
		pctx_pnl->outputTiming_info.locked_flag = TRUE;
	}
	fpll_lock = pctx_pnl->outputTiming_info.locked_flag;

	if ((force == TRUE) ||
		(_mtk_video_display_frc_is_variable_updated(
				pre_fpll_lock, fpll_lock))
	){
		DRM_INFO("[%s,%d] pre_fpll_lock = 0x%x\n",
			__func__, __LINE__, pre_fpll_lock);
		send.CmdID = FRC_MB_CMD_SET_FPLL_LOCK_DONE;
		send.D1    = (uint32_t)fpll_lock;
		send.D2    = DEFAULT_VALUE;
		send.D3    = DEFAULT_VALUE;
		send.D4    = DEFAULT_VALUE;
		cmdstatus = pqu_send_cmd(&send, &callback_info);
		pre_fpll_lock = fpll_lock;
		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			__func__, __LINE__, send.D1, send.D2, send.D3, send.D4);
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|FPLL_LOCK_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~FPLL_LOCK_CMD_STATUS));
	}
}

//set timing
static void _mtk_video_display_set_timing(unsigned int plane_inx, struct mtk_tv_kms_context *pctx)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);

	static uint32_t input_freq      = DEFAULT_VALUE;
	static uint32_t pre_input_freq  = DEFAULT_VALUE;
	static uint32_t output_freq     = DEFAULT_VALUE;
	static uint32_t pre_output_freq = DEFAULT_VALUE;
	static uint32_t cmdstatus       = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;

	input_freq  = div64_u64((plane_props->prop_val[EXT_VPLANE_PROP_INPUT_VFREQ] +
		_tHZ), VFRQRATIO_1000); // +500 for round
	output_freq = ((pctx_pnl->outputTiming_info.u32OutputVfreq +
		_tHZ)/VFRQRATIO_1000); // +500 for round

	if ((force == TRUE) ||
		(_mtk_video_display_frc_is_variable_updated(
				pre_input_freq, input_freq)) ||
		(_mtk_video_display_frc_is_variable_updated(
				pre_output_freq, output_freq))
	){
		DRM_INFO("[%s,%d] pre_input_freq = 0x%x, pre_output_freq = 0x%x\n",
			__func__, __LINE__, pre_input_freq, pre_output_freq);
		send.CmdID = FRC_MB_CMD_SET_TIMING;
		send.D1    = (uint32_t)input_freq;
		send.D2    = (uint32_t)output_freq;
		send.D3    = DEFAULT_VALUE;
		send.D4    = DEFAULT_VALUE;
		cmdstatus  = pqu_send_cmd(&send, &callback_info);
		pre_input_freq  = input_freq;
		pre_output_freq = output_freq;
		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			__func__, __LINE__, send.D1, send.D2, send.D3, send.D4);
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|TIMING_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~TIMING_CMD_STATUS));
	}
}

struct mtk_frc_timinginfo_tbl *_mtk_video_frc_find_in_timing_mappinginfo(uint32_t output_freq,
	uint32_t input_freq)
{
	uint16_t tablenum;
	struct mtk_frc_timinginfo_tbl *pinfotable = NULL;
	uint16_t index;

	if (output_freq > _FREQ_100HZ) {
		tablenum = (sizeof(frc_timing_120Hz_table) /
			sizeof(struct mtk_frc_timinginfo_tbl));
		pinfotable = frc_timing_120Hz_table;
	} else {
		tablenum = (sizeof(frc_timing_60Hz_table) /
			sizeof(struct mtk_frc_timinginfo_tbl));
		pinfotable = frc_timing_60Hz_table;
	}

	for (index = 0; index < tablenum; index++) {
		if ((input_freq > pinfotable->input_freq_L) &&
			(input_freq < pinfotable->input_freq_H))
			break;
		pinfotable++;
	}
	return pinfotable;
}


#define _MSB(x) (x>>4)
#define _LSB(x) (x&0x0F)
#define _HEX2DEC(x) ((_MSB(x)*10)+_LSB(x))
#define _DIV10 10
#define _1_SECOND 1000000ul
void _mtk_video_frc_get_latency(unsigned int plane_inx, struct mtk_tv_kms_context *pctx,
	bool enable)
{
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);
	struct mtk_frc_timinginfo_tbl *pinfotable = NULL;

	static uint32_t input_freq      = DEFAULT_VALUE;
	static uint32_t pre_input_freq  = DEFAULT_VALUE;
	static uint32_t output_freq     = DEFAULT_VALUE;
	bool force = FALSE;
	bool pre_enable = FALSE;
	uint8_t u8latency;

	input_freq  = plane_props->prop_val[EXT_VPLANE_PROP_INPUT_VFREQ];
	output_freq = (pctx_pnl->outputTiming_info.u32OutputVfreq/VFRQRATIO_1000);

	if ((force == TRUE) || (pre_enable != enable) ||
			(_mtk_video_display_frc_is_variable_updated(
					pre_input_freq, input_freq))) {
		pinfotable = _mtk_video_frc_find_in_timing_mappinginfo(output_freq, input_freq);
		u8latency = pinfotable->frc_memc_latency;
		u8latency = _HEX2DEC(u8latency);
		if (enable) {
			if (input_freq)
				pctx_video->frc_latency = _1_SECOND/input_freq*u8latency/_DIV10;
			else
				pctx_video->frc_latency = 0;
		} else {
			pctx_video->frc_latency = 0;
		}
		DRM_INFO("[%s,%d] frc_latency= %d\n",
					__func__, __LINE__, pctx_video->frc_latency);
		pre_input_freq  = input_freq;
		pre_enable = enable;
		pctx_video->frc_rwdiff = MEMC_RW_DIFF_VAL;
	}
}

uint32_t mtk_video_display_frc_set_Qos(uint8_t QosMode)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};
	//FRC_MB_CMD_INIT_DONE
	send.CmdID = FRC_MB_CMD_SET_QoS;
	send.D1    = QosMode;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;

	DRM_INFO("[%s,%d] [%x]D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
		__func__, __LINE__, send.CmdID, send.D1, send.D2, send.D3, send.D4);

	return pqu_send_cmd(&send, &callback_info);
}

static uint32_t _mtk_video_display_frc_get_Qos(uint8_t *pQosMode)
{
	struct pqu_frc_get_cmd_param send = {DEFAULT_VALUE};
	struct pqu_frc_get_cmd_reply_param reply = {DEFAULT_VALUE};

	static uint32_t cmdstatus = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	send.CmdID = FRC_MB_CMD_GET_QoS;
	send.D1    = DEFAULT_VALUE;
	send.D2    = DEFAULT_VALUE;
	send.D3    = DEFAULT_VALUE;
	send.D4    = DEFAULT_VALUE;
	cmdstatus = pqu_get_cmd(&send, &reply);

	DRM_INFO("[%s,%d] [%x]D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
		__func__, __LINE__, send.CmdID, reply.D1, reply.D2, reply.D3, reply.D4);
	*pQosMode = reply.D1;
	return cmdstatus;
}

uint32_t mtk_video_display_frc_get_Qos(uint8_t *pQosMode)
{
	return _mtk_video_display_frc_get_Qos(pQosMode);
}

static void _mtk_video_display_update_Qos(struct mtk_tv_kms_context *pctx)
{
	uint8_t PreQosMode, QosMode;
	static uint32_t cmdstatus   = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;

	if (force == TRUE) {
		PreQosMode = true;
		mtk_video_display_frc_set_Qos(PreQosMode);
		mtk_video_display_frc_get_Qos(&QosMode);
		if (PreQosMode == QosMode)
			memc_rv55_info = (memc_rv55_info|MFC_PW_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~MFC_PW_STATUS));

	}
}


// Parsing and update memc mode
static void _mtk_video_display_parsing_and_update_memc_mode(char *memc_mode)
{
	char Mode_off[]  = "Off;";
	char Mode_on[]   = "On;";
	char Mode_auto[] = "Auto;";


	if (memc_mode == NULL) {
		DRM_ERROR("[%s %d] memc_mode = NULL\n", __func__, __LINE__);
		return;
	}

	memc_mode += strlen("MEMC_ONOFF:");
	if (strncmp(Mode_off, memc_mode, strlen(Mode_off)) == 0) {
		FRCMode[DATA_D0] = MEMC_MODE_OFF;
		DRM_INFO("[%s,%d] FRCMode[0]= 0x%x\n", __func__, __LINE__, FRCMode[DATA_D0]);
		memc_rv55_info = (memc_rv55_info|MEMC_OFF_STATUS);
	} else if (strncmp(Mode_on, memc_mode, strlen(Mode_on)) == 0) {
		FRCMode[DATA_D0] = MEMC_MODE_ON;
		DRM_INFO("[%s,%d] FRCMode[0]= 0x%x\n", __func__, __LINE__, FRCMode[DATA_D0]);
		memc_rv55_info = (memc_rv55_info|MEMC_ON_STATUS);
	} else if (strncmp(Mode_auto, memc_mode, strlen(Mode_auto)) == 0) {
		FRCMode[DATA_D0] = MEMC_MODE_AUTO;
		DRM_INFO("[%s,%d] FRCMode[0]= 0x%x\n", __func__, __LINE__, FRCMode[DATA_D0]);
		memc_rv55_info = (memc_rv55_info|MEMC_AUTO_STATUS);
	} else {
		DRM_ERROR("[%s %d] unsupport exception memc mode\n", __func__, __LINE__);
		memc_rv55_info = (memc_rv55_info&(~(MEMC_STATUS)));
	}
}

// Parsing and update memc demo mode
static void _mtk_video_display_parsing_and_update_demo_mode(
			char *memc_demo,
			char *demo_partition)
{
	char Demo_off[]  = "Off;";
	char Demo_on[]   = "On;";
	char partition_left[]  = "Left;";
	char partition_right[] = "Right;";
	char partition_top[]   = "Top;";
	char partition_dwon[]  = "Down;";
	char partition_all[]  = "All;";

	if (memc_demo == NULL) {
		DRM_ERROR("[%s %d] memc_demo = NULL\n", __func__, __LINE__);
		return;
	}

	if (demo_partition == NULL) {
		DRM_ERROR("[%s %d] demo_partition = NULL\n", __func__, __LINE__);
		return;
	}

	memc_demo += strlen("MJC_Demo:");
	demo_partition += strlen("MJC_Demo_Partition:");
	if (strncmp(Demo_off, memc_demo, strlen(Demo_off)) == 0) {
		DEMOMode[DATA_D0] = MEMC_MODE_OFF;
		DRM_INFO("[%s,%d] DEMOMode[0]= 0x%x\n", __func__, __LINE__, DEMOMode[DATA_D0]);
		memc_rv55_info = (memc_rv55_info|DEMO_OFF_STATUS);
	} else if ((strncmp(Demo_on, memc_demo, strlen(Demo_on)) == 0) &&
			(strncmp(partition_left, demo_partition, strlen(partition_left)) == 0)
	 ){
		DEMOMode[DATA_D0] = DEMO_MODE_ON_LEFT_RIGHT;
		DEMOMode[DATA_D1] = DEMO_PARTITION_LEFT_OR_TOP;
		DRM_INFO("[%s,%d] DEMOMode[0]= 0x%x,DEMOMode[1]= 0x%x\n", __func__, __LINE__,
			DEMOMode[DATA_D0], DEMOMode[DATA_D1]);
		memc_rv55_info = (memc_rv55_info|DEMO_ON_LEFT_STATUS);
	} else if ((strncmp(Demo_on, memc_demo, strlen(Demo_on)) == 0) &&
			(strncmp(partition_right, demo_partition, strlen(partition_right)) == 0)
	 ){
		DEMOMode[DATA_D0] = DEMO_MODE_ON_LEFT_RIGHT;
		DEMOMode[DATA_D1] = DEMO_PARTITION_RIGHT_OR_DOWN;
		DRM_INFO("[%s,%d] DEMOMode[0]= 0x%x,DEMOMode[1]= 0x%x\n", __func__, __LINE__,
			DEMOMode[DATA_D0], DEMOMode[DATA_D1]);
		memc_rv55_info = (memc_rv55_info|DEMO_ON_RIGHT_STATUS);
	} else if ((strncmp(Demo_on, memc_demo, strlen(Demo_on)) == 0) &&
			(strncmp(partition_top, demo_partition, strlen(partition_top)) == 0)
	 ){
		DEMOMode[DATA_D0] = DEMO_MODE_ON_TOP_DOWN;
		DEMOMode[DATA_D1] = DEMO_PARTITION_LEFT_OR_TOP;
		DRM_INFO("[%s,%d] DEMOMode[0]= 0x%x,DEMOMode[1]= 0x%x\n", __func__, __LINE__,
			DEMOMode[DATA_D0], DEMOMode[DATA_D1]);
	} else if ((strncmp(Demo_on, memc_demo, strlen(Demo_on)) == 0) &&
			(strncmp(partition_dwon, demo_partition, strlen(partition_dwon)) == 0)
	 ){
		DEMOMode[DATA_D0] = DEMO_MODE_ON_TOP_DOWN;
		DEMOMode[DATA_D1] = DEMO_PARTITION_RIGHT_OR_DOWN;
		DRM_INFO("[%s,%d] DEMOMode[0]= 0x%x,DEMOMode[1]= 0x%x\n", __func__, __LINE__,
			DEMOMode[DATA_D0], DEMOMode[DATA_D1]);
	} else if ((strncmp(Demo_on, memc_demo, strlen(Demo_on)) == 0) &&
			(strncmp(partition_all, demo_partition, strlen(partition_all)) == 0)
	 ){
		DEMOMode[DATA_D0] = MEMC_MODE_OFF;
		DRM_INFO("[%s,%d] DEMOMode[0]= 0x%x\n", __func__, __LINE__, DEMOMode[DATA_D0]);
		memc_rv55_info = (memc_rv55_info|DEMO_ON_ALL_STATUS);
	} else {
		DRM_ERROR("[%s %d] unsupport exception demo mode\n",	__func__, __LINE__);
		memc_rv55_info = (memc_rv55_info&(~(DEMO_STATUS)));
	}
}

// Parsing and update memc level
static void _mtk_video_display_parsing_and_update_memc_level(char *memc_level)
{
	char memc_level_start[]  = "[";
	char memc_level_comma[]  = ",";
	char memc_level_end[]    = "];";
	char *p_one_level_end = NULL;
	unsigned long ul_one_memc_level = 0;
	uint8_t memc_level_value = 0;
	uint8_t data_index = 0;

	if (memc_level == NULL) {
		DRM_ERROR("[%s %d] memc_level = NULL\n", __func__, __LINE__);
		return;
	}

	memc_level += strlen("MEMC_Level:");
	if ((*memc_level) == memc_level_start[DATA_D0]) {
		memc_level += strlen(memc_level_start);
		for (data_index = 0; data_index < MEMC_LEVLE_MAX_CNT; data_index++) {
			ul_one_memc_level = strtoul(memc_level, &p_one_level_end, 0);

			if (ul_one_memc_level < LIMIT_MEMC_LEVEL)
				memc_level_value = (uint8_t)(ul_one_memc_level);
			else
				DRM_ERROR("[%s %d] unsupport exception memc_level_value\n",
				__func__, __LINE__);

			MFCLevel[data_index] = memc_level_value;
			DRM_INFO("[%s,%d] MFCLevel[%d]= 0x%x\n", __func__, __LINE__,
				data_index, MFCLevel[data_index]);
			if (strncmp(memc_level_end, p_one_level_end,
								strlen(memc_level_end)) == 0) {
				break;
			} else if ((*p_one_level_end) == memc_level_comma[DATA_D0]) {
				memc_level = p_one_level_end;
				memc_level += strlen(memc_level_comma);
			} else {
				DRM_ERROR("[%s %d] unsupport exception memc_level\n",
									__func__, __LINE__);
			}
		}
	} else {
		DRM_ERROR("[%s %d] unsupport exception memc_level\n",
			__func__, __LINE__);
	}

	//For LTP
	if (memcmp(MFCLevel, LTP_MFCLevel_OFF, sizeof(MFCLevel)) == 0)
		memc_rv55_info = (memc_rv55_info|MEMC_LV_OFF_STATUS);
	else if (memcmp(MFCLevel, LTP_MFCLevel_Low, sizeof(MFCLevel)) == 0)
		memc_rv55_info = (memc_rv55_info|MEMC_LV_LOW_STATUS);
	else if (memcmp(MFCLevel, LTP_MFCLevel_Mid, sizeof(MFCLevel)) == 0)
		memc_rv55_info = (memc_rv55_info|MEMC_LV_MID_STATUS);
	else if (memcmp(MFCLevel, LTP_MFCLevel_High, sizeof(MFCLevel)) == 0)
		memc_rv55_info = (memc_rv55_info|MEMC_LV_HIGH_STATUS);
	else
		memc_rv55_info = (memc_rv55_info&(~MEMC_LV_STATUS));

}


//Parsing json file
static void _mtk_video_display_parsing_json_file(unsigned int plane_inx,
							struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);

	char *p_memc_onoff = NULL;
	char *p_memc_demo = NULL;
	char *p_memc_demo_Partition = NULL;
	char *p_memc_level = NULL;

	if (plane_ctx->pq_string == NULL) {
		DRM_ERROR("[%s %d] plane_ctx->pq_string = NULL\n", __func__, __LINE__);
	} else {
		DRM_INFO("[%s,%d]pq_string:%s\n", __func__, __LINE__, plane_ctx->pq_string);
		// Parsing and update memc mode
		p_memc_onoff = strstr(plane_ctx->pq_string, "MEMC_ONOFF:");
		if (p_memc_onoff == NULL) {
			DRM_ERROR("\n[%s,%d]p_memc_onoff is NULL", __func__, __LINE__);
			return;
		}

		_mtk_video_display_parsing_and_update_memc_mode(p_memc_onoff);

		// Parsing and update memc demo mode
		p_memc_demo = strstr(plane_ctx->pq_string, "MJC_Demo:");
		if (p_memc_demo == NULL) {
			DRM_ERROR("\n[%s,%d]p_memc_demo is NULL", __func__, __LINE__);
			return;
		}

		p_memc_demo_Partition = strstr(plane_ctx->pq_string, "MJC_Demo_Partition:");
		if (p_memc_demo_Partition == NULL) {
			DRM_ERROR("\n[%s,%d]p_memc_demo_Partition is NULL", __func__, __LINE__);
			return;
		}
		_mtk_video_display_parsing_and_update_demo_mode(p_memc_demo, p_memc_demo_Partition);

		// Parsing and update memc level
		p_memc_level = strstr(plane_ctx->pq_string, "MEMC_Level:");
		if (p_memc_level == NULL) {
			DRM_ERROR("\n[%s,%d]p_memc_level is NULL", __func__, __LINE__);
			return;
		}

		_mtk_video_display_parsing_and_update_memc_level(p_memc_level);
	}
}

//set frc mode
static void _mtk_video_display_set_frc_mode(struct mtk_tv_kms_context *pctx,
						uint8_t *u8Data)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};

	static uint8_t pre_MFCMode  = DEFAULT_PRE_VALUE;
	static uint8_t pre_FRC_Mode = DEFAULT_PRE_VALUE;
	static uint32_t cmdstatus   = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;

	if ((force == TRUE) ||
		(_mtk_video_display_frc_is_variable_updated(
					pre_MFCMode, u8Data[DATA_D0])) ||
		(_mtk_video_display_frc_is_variable_updated(
					pre_FRC_Mode, u8Data[DATA_D1]))
	){
		DRM_INFO("[%s,%d] pre_MFCMode = 0x%x, pre_FRC_Mode = 0x%x\n",
			__func__, __LINE__, pre_MFCMode, pre_FRC_Mode);
		send.CmdID = FRC_MB_CMD_SET_FRC_MODE;
		send.D1    = (uint32_t)u8Data[DATA_D0];
		send.D2    = (uint32_t)u8Data[DATA_D1];
		send.D3    = (uint32_t)DEFAULT_VALUE;
		send.D4    = (uint32_t)DEFAULT_VALUE;
		cmdstatus  = pqu_send_cmd(&send, &callback_info);
		pre_MFCMode  = u8Data[DATA_D0];
		pre_FRC_Mode = u8Data[DATA_D1];
		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			__func__, __LINE__, send.D1, send.D2, send.D3, send.D4);
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|FRC_MODE_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~FRC_MODE_CMD_STATUS));
	}
}

#if (0)
//set mfc mode
static void _mtk_video_display_set_mfc_mode(uint8_t *u8Data)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};

	static uint8_t pre_MFCMode       = DEFAULT_VALUE;
	static uint8_t pre_DeJudderLevel = DEFAULT_VALUE;
	static uint8_t pre_DeBlurLevel   = DEFAULT_VALUE;
	static uint8_t pre_FRC_Mode      = DEFAULT_VALUE;
	static uint32_t cmdstatus        = DEFAULT_CMD_STATUS;

	if ((_mtk_video_display_frc_is_variable_updated(
					pre_MFCMode, u8Data[DATA_D0])) ||
		(_mtk_video_display_frc_is_variable_updated(
					pre_DeJudderLevel, u8Data[DATA_D1])) ||
		(_mtk_video_display_frc_is_variable_updated(
					pre_DeBlurLevel, u8Data[DATA_D2])) ||
		(_mtk_video_display_frc_is_variable_updated(
					pre_FRC_Mode, u8Data[DATA_D3]))
	){
		DRM_INFO("[%s,%d] Host send CPUIF to FRC RV55\n", __func__, __LINE__);
		send.CmdID = FRC_MB_CMD_SET_MFC_MODE;
		send.D1    = (uint32_t)u8Data[DATA_D0];
		send.D2    = (uint32_t)u8Data[DATA_D1];
		send.D3    = (uint32_t)u8Data[DATA_D2];
		send.D4    = (uint32_t)u8Data[DATA_D3];
		cmdstatus = pqu_send_cmd(&send, &callback_info);
		pre_MFCMode       = u8Data[DATA_D0];
		pre_DeJudderLevel = u8Data[DATA_D1];
		pre_DeBlurLevel   = u8Data[DATA_D2];
		pre_FRC_Mode      = u8Data[DATA_D3];
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|MFC_MODE_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~MFC_MODE_CMD_STATUS));
	}
}
#endif

//set mfc level
static void _mtk_video_display_set_memc_level_update(struct mtk_tv_kms_context *pctx,
							uint8_t *u8Data)
{
	struct pqu_frc_send_burstcmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info      = {DEFAULT_VALUE};
	uint8_t data_size = 0;
	static uint8_t pre_MFCLeve[MEMC_LEVLE_MAX_CNT] = {DEFAULT_PRE_VALUE};
	static uint32_t cmdstatus = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;

	if ((force == TRUE) ||
		(memcmp(pre_MFCLeve, u8Data, sizeof(pre_MFCLeve)) != 0)) {
		send.CmdID = FRC_MB_CMD_SET_MECM_LEVEL;
		for (data_size = 0; data_size < MEMC_LEVLE_MAX_CNT; data_size++) {
			send.Data[data_size] = u8Data[data_size];
			pre_MFCLeve[data_size] = u8Data[data_size];
			DRM_INFO("[%s,%d] send.Data[%d] = %d\n",
				__func__, __LINE__, data_size, send.Data[data_size]);
		}
		cmdstatus = pqu_send_burst_cmd(&send, &callback_info);
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|MFC_LEVEL_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~MFC_LEVEL_CMD_STATUS));
	}
}
#define RV55_magic_status 0x55667788
static void _mtk_video_display_get_mfc_alg_status(struct mtk_tv_kms_context *pctx)
{
	struct pqu_frc_get_cmd_param send = {DEFAULT_VALUE};
	struct pqu_frc_get_cmd_reply_param reply = {DEFAULT_VALUE};



	static uint32_t cmdstatus = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;
	if (force == TRUE) {
		send.CmdID = FRC_MB_CMD_GET_MECM_ALG_STATUS;
		send.D1    = DEFAULT_VALUE;
		send.D2    = DEFAULT_VALUE;
		send.D3    = DEFAULT_VALUE;
		send.D4    = DEFAULT_VALUE;
		cmdstatus = pqu_get_cmd(&send, &reply);

		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			__func__, __LINE__, reply.D1, reply.D2, reply.D3, reply.D4);

		if (reply.D1 == RV55_magic_status)
			memc_rv55_info = (memc_rv55_info|MFC_ALG_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~MFC_ALG_STATUS));
	}
}

//set demo mode
static void _mtk_video_display_set_demo_mode(struct mtk_tv_kms_context *pctx,
							uint8_t *u8Data)
{
	struct pqu_frc_send_cmd_param send = {DEFAULT_VALUE};
	struct callback_info callback_info = {DEFAULT_VALUE};

	static uint8_t pre_DemoMode  = DEFAULT_PRE_VALUE;
	static uint8_t pre_Partition = DEFAULT_PRE_VALUE;
	static uint32_t cmdstatus = DEFAULT_CMD_STATUS;
	bool force = FALSE;

	if (pctx->stub)
		force = TRUE;

	if ((force == TRUE) ||
		(_mtk_video_display_frc_is_variable_updated(
					pre_DemoMode, u8Data[DATA_D0])) ||
		(_mtk_video_display_frc_is_variable_updated(
					pre_Partition, u8Data[DATA_D1]))
	){
		DRM_INFO("[%s,%d] pre_DemoMode = 0x%x, pre_Partition = 0x%x\n",
			__func__, __LINE__, pre_DemoMode, pre_Partition);
		send.CmdID = FRC_MB_CMD_SET_MFC_DEMO_MODE;
		send.D1    = (uint32_t)u8Data[DATA_D0];
		send.D2    = (uint32_t)u8Data[DATA_D1];
		send.D3    = DEFAULT_VALUE;
		send.D4    = DEFAULT_VALUE;
		cmdstatus = pqu_send_cmd(&send, &callback_info);
		pre_DemoMode  = u8Data[DATA_D0];
		pre_Partition = u8Data[DATA_D1];
		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			__func__, __LINE__, send.D1, send.D2, send.D3, send.D4);
		if (cmdstatus == 0)
			memc_rv55_info = (memc_rv55_info|MFC_DEMO_CMD_STATUS);
		else
			memc_rv55_info = (memc_rv55_info&(~MFC_DEMO_CMD_STATUS));
	}
}

//init clock
static void _mtk_video_display_set_frc_clock(struct mtk_video_context *pctx_video,
							bool enable)
{
#if (1) //(MEMC_CONFIG == 1)

	struct hwregFrcClockIn paramFrcClockIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];

	memset(&paramFrcClockIn, 0, sizeof(struct hwregFrcClockIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(reg, 0, sizeof(reg));

	paramOut.reg = reg;

	paramFrcClockIn.RIU = 1;
	paramFrcClockIn.bEnable = enable;

	drv_hwreg_render_frc_set_clock(
		paramFrcClockIn, &paramOut);
#endif
}

static int _mtk_video_display_get_frc_memory_size(unsigned int plane_inx,
							struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);

	plane_ctx->memc_pq_size = FRC_PQ_BUFFER_SIZE;

	return 0;
}
#define DMA_ALLOC_ATT_MASK 34
static int _mtk_video_display_alloc_frc_memory(unsigned int plane_inx,
							struct mtk_tv_kms_context *pctx)
{
	int ret;

	/* check pointer */
	//CHECK_POINTER(pctx, -1);
		if	(pctx == NULL) {
			DRM_INFO("[%s,%d], pointer is NULL or Error\n", __func__, __LINE__);
			return -1;	}

	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	struct mtk_tv_drm_private *pri = pctx->drm_dev->dev_private;
	__u64 va = 0;
	__u64 iova = 0;
	unsigned long iommu_attrs = 0;
	unsigned int buf_iommu_idx = 0;


#if (1) //(MEMC_CONFIG == 1)
	if (dma_get_mask(pri->dev) < DMA_BIT_MASK(DMA_ALLOC_ATT_MASK)) {
		DRM_ERROR("[%s %d]dma_alloc_attrs MASK<\n",
				__func__, __LINE__);
	}

	if (!dma_supported(pri->dev, 0)) {
		DRM_ERROR("[%s %d]dma_supported: Not\n",
				__func__, __LINE__);
	}

	// set buf tag
	buf_iommu_idx = (pctx->drmcap_dev.u8iommu_idx_memc_pq
					<< pctx->drmcap_dev.u8buf_iommu_offset);
	iommu_attrs |= buf_iommu_idx;
	if (iommu_attrs != 0) {
		// if mapping uncache
		iommu_attrs |= DMA_ATTR_WRITE_COMBINE;

		DRM_INFO("[%s %d]alloc_att iova: ", __func__, __LINE__);
		DRM_INFO("size=0x%llx att=0x%llx, dev=0x%llx drm_dev=0x%llx dev=0x%llx\n",
		plane_ctx->memc_pq_size, iommu_attrs, pctx->dev, pctx->drm_dev, pri->dev);

		va = dma_alloc_attrs(pri->dev,
			plane_ctx->memc_pq_size,
			&iova,
			GFP_KERNEL,
			iommu_attrs);
		ret = dma_mapping_error(pri->dev, iova);
		if (ret) {
			DRM_ERROR("[%s %d]dma_alloc_attrs fail, buf_attr_idx : %u\n",
				__func__, __LINE__, buf_iommu_idx);
			return ret;
		}
		plane_ctx->memc_pq_va = va;
		plane_ctx->memc_pq_iova = iova;
	DRM_INFO("[%s %d]dma_alloc_attrs iova: 0x%llx\n",
				__func__, __LINE__, iova);
	} else {
	/*get reserved meory*/
	}
#endif
	return 0;
}

//release frc_pq buf and clear frc_pq buf addr register
//
static void _mtk_video_display_release_frc_memory(unsigned int plane_inx,
							struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	struct mtk_tv_drm_private *pri = pctx->drm_dev->dev_private;
	unsigned int buf_iommu_idx = 0;

#if (1) //(MEMC_CONFIG == 1)

	buf_iommu_idx = (pctx->drmcap_dev.u8iommu_idx_memc_pq
					<< pctx->drmcap_dev.u8buf_iommu_offset);

	/*release buf*/
	dma_free_attrs(pri->dev, plane_ctx->memc_pq_size, (void *)plane_ctx->memc_pq_va,
					plane_ctx->memc_pq_iova, buf_iommu_idx);

	plane_ctx->memc_pq_va = -1;
	plane_ctx->memc_pq_iova = -1;
#endif
}

#define FRC_PQ_ADDR_MASK 0xFFFF
static bool _mtk_video_display_set_frc_memory_addr(unsigned int plane_inx,
							struct mtk_tv_kms_context *pctx)
{
#if (0) //(MEMC_CONFIG == 1)

	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);

	struct hwregFrcBaseAddrIn paramBaseAddrIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	uint16_t byte_per_word = pctx_video->byte_per_word;

	memset(&paramBaseAddrIn, 0, sizeof(struct hwregFrcBaseAddrIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(reg, 0, sizeof(reg));

	paramOut.reg = reg;

	paramBaseAddrIn.RIU = 1;
	if (plane_ctx->memc_pq_iova != -1)
		paramBaseAddrIn.frc_pq_addr =  plane_ctx->memc_pq_iova / byte_per_word;
	else
		paramBaseAddrIn.frc_pq_addr = FRC_PQ_ADDR_MASK;

	paramBaseAddrIn.frc_pq_size = plane_ctx->memc_pq_size;

	drv_hwreg_render_frc_set_baseAddr(
		paramBaseAddrIn, &paramOut);
#endif
	return true;
}

#define SHIFT_16B(x) ((x) >> 16)

static int _mtk_video_display_set_frc_3d_table(unsigned int plane_inx,
						struct mtk_tv_kms_context *pctx)
{
#if (1) //(MEMC_CONFIG == 1)
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct hwregFrc3DTableIn param3DTableIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	//struct reg_info reg[REG_NUM_MAX];
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);

	memset(&param3DTableIn, 0, sizeof(struct hwregFrc3DTableIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return -1;
	}

	param3DTableIn.RIU = 1;
	param3DTableIn.s_u16Vsize = SHIFT_16B(plane_ctx->src_h);
	param3DTableIn.s_u16Hsize = SHIFT_16B(plane_ctx->src_w);

	drv_hwreg_render_frc_set_3DTable(
		param3DTableIn, &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);
#endif
	return 0;
}

static void _mtk_video_display_set_frc_dscl(void)
{

}

static void _mtk_video_display_unmask_frc(struct mtk_video_context *pctx_video,
						bool enable)
{
#if (1) //(MEMC_CONFIG == 1)

	struct hwregFrcUnmaskIn paramUnmaskIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	//struct reg_info reg[REG_NUM_MAX];

	memset(&paramUnmaskIn, 0, sizeof(struct hwregFrcUnmaskIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}

	paramUnmaskIn.RIU = 1;
	paramUnmaskIn.unmask = enable;

	drv_hwreg_render_frc_unmask(
		paramUnmaskIn, &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);
#endif
}

static void _mtk_video_display_set_frc_blendOut(struct mtk_video_context *pctx_video, bool enable)
{
#if (0) //(MEMC_CONFIG == 1)

	struct hwregBlendOutIn paramBlendOutIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];

	memset(&paramBlendOutIn, 0, sizeof(struct hwregBlendOutIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(reg, 0, sizeof(reg));

	paramOut.reg = reg;

	paramBlendOutIn.RIU = 1;
	paramBlendOutIn.blackEn = enable;
	paramBlendOutIn.snowEn = enable;
	paramBlendOutIn.Hsize = 0;

	drv_hwreg_render_display_set_blendOut(
		paramBlendOutIn, &paramOut);
#endif
}

static void _mtk_video_display_set_frc_vop2memcMuxSelect(
	struct mtk_video_context *pctx_video,
	bool muxSel)
{
	struct hwregVop2memcMuxSelectIn paramVop2memcMuxSelectIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];

	memset(&paramVop2memcMuxSelectIn, 0, sizeof(struct hwregVop2memcMuxSelectIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(reg, 0, sizeof(reg));

	paramOut.reg = reg;
	paramVop2memcMuxSelectIn.muxSel = muxSel;
	paramVop2memcMuxSelectIn.RIU = 1;

	drv_hwreg_render_display_set_vop2memcMuxSelect(
		paramVop2memcMuxSelectIn, &paramOut);
}

#define _8K_HSIZE 7680
#define _8K_VSIZE 4320
#define _4K_HSIZE 3840
#define _4K_VSIZE 2160

void _mtk_video_display_frc_set_vs_trig(
		struct mtk_tv_kms_context *pctx,
		bool bEn)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];

	struct hwregFrcPatIn paramFrcPatIn;
	struct hwregOut paramOut;

	memset(&paramFrcPatIn, 0, sizeof(struct hwregFrcPatIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(reg, 0, sizeof(reg));

	paramOut.reg = reg;

	paramFrcPatIn.RIU = 1;
	paramFrcPatIn.bEn = bEn;
	if (pctx_video->frc_ip_version == FRC_IP_VERSION_2) {
		paramFrcPatIn.Hsize = _4K_HSIZE;
		paramFrcPatIn.Vsize = _4K_VSIZE;
	} else {
		paramFrcPatIn.Hsize = _8K_HSIZE;
		paramFrcPatIn.Vsize = _8K_VSIZE;
	}

	drv_hwreg_render_frc_set_vs_sw_trig(
		paramFrcPatIn, &paramOut);

}

void _mtk_video_display_frc_vs_trig_ctl(struct mtk_tv_kms_context *pctx)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;

		if ((pctx_video->frc_pattern_sel == MEMC_PAT_IPM_DYNAMIC) ||
		((pctx_video->frc_pattern_sel == MEMC_PAT_IPM_STATIC) &&
		(pctx_video->frc_pattern_trig_cnt))
		) {
			_mtk_video_display_frc_set_vs_trig(pctx, true);
			if (pctx_video->frc_pattern_trig_cnt)
				pctx_video->frc_pattern_trig_cnt--;
		}
}

void _mtk_video_display_frc_set_ipm_pattern(
		struct mtk_video_context *pctx_video,
		bool bEn)
{
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];

	struct hwregFrcPatIn paramFrcPatIn;
	struct hwregOut paramOut;

	memset(&paramFrcPatIn, 0, sizeof(struct hwregFrcPatIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(reg, 0, sizeof(reg));

	paramOut.reg = reg;

	paramFrcPatIn.RIU = 1;
	paramFrcPatIn.bEn = bEn;
	if (pctx_video->frc_ip_version == FRC_IP_VERSION_2) {
		paramFrcPatIn.Hsize = _4K_HSIZE;
		paramFrcPatIn.Vsize = _4K_VSIZE;
	} else {
		paramFrcPatIn.Hsize = _8K_HSIZE;
		paramFrcPatIn.Vsize = _8K_VSIZE;
	}

	drv_hwreg_render_frc_set_ipmpattern(
	paramFrcPatIn, &paramOut);

}

void _mtk_video_display_frc_set_opm_pattern(
		struct mtk_video_context *pctx_video,
		bool bEn)
{
	uint16_t reg_num = pctx_video->reg_num;
	//struct reg_info reg[REG_NUM_MAX];

	struct hwregFrcPatIn paramFrcPatIn;
	struct hwregOut paramOut;

	memset(&paramFrcPatIn, 0, sizeof(struct hwregFrcPatIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}

	paramFrcPatIn.RIU = 1;
	paramFrcPatIn.bEn = bEn;
	if (pctx_video->frc_ip_version == FRC_IP_VERSION_2) {
		paramFrcPatIn.Hsize = _4K_HSIZE;
		paramFrcPatIn.Vsize = _4K_VSIZE;
	} else {
		paramFrcPatIn.Hsize = _8K_HSIZE;
		paramFrcPatIn.Vsize = _8K_VSIZE;
	}

	drv_hwreg_render_frc_set_opmpattern(
	paramFrcPatIn, &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);
}

#define MOD_MTK_MASK 0xFF
#define DRM_FORMAT_MOD_MASK 0xF
#define SHIFT_56 56
static int _mtk_video_get_format_yuv(
	struct mtk_video_format_info video_format_info,
	enum E_DRV_IPM_MEMORY_CONFIG *eMemoryFormat)
{
	if (video_format_info.modifier_arrange == (DRM_FORMAT_MOD_MTK_YUV444_VYU & MOD_MTK_MASK)) {
		if (video_format_info.modifier_10bit)
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_MAX;
		else if (video_format_info.modifier_6bit)
			//YUV444 6b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_444_6B;
		else
			//YUV444 8b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_444_8B;
	}
	DRM_INFO("%s %d memc format=%d\n", __func__, __LINE__, *eMemoryFormat);

	return 0;
}
#define MASK_FF 0xFF
#define BIT_COMP  0x100
#define BIT_6BIT  0x200
#define BIT_10BIT 0x400
#define SHIFT_COMP 8
#define SHIFT_6BIT 9
#define SHIFT_10BIT 10

static int _mtk_video_display_get_frc_fb_memory_format(struct drm_framebuffer *fb,
	enum E_DRV_IPM_MEMORY_CONFIG *eMemoryFormat)
{
	struct mtk_video_format_info video_format_info;
	struct drm_format_name_buf format_name;
	uint64_t modifier = 0;

	//CHECK_POINTER(fb, NULL);
	if	(fb == NULL)	{
		DRM_INFO("[%s,%d], pointer is NULL or Error\n", __func__, __LINE__);
		return -1;	}
	//CHECK_POINTER(eMemoryFormat, NULL);
	if	(eMemoryFormat == NULL)	{
		DRM_INFO("[%s,%d], pointer is NULL or Error\n", __func__, __LINE__);
		return -1;	}

	memset(&video_format_info, 0, sizeof(struct mtk_video_format_info));

	//Get bpp for plane 0
	video_format_info.fourcc = fb->format->format;
	if (((fb->modifier >> SHIFT_56) & DRM_FORMAT_MOD_MASK) == DRM_FORMAT_MOD_VENDOR_MTK) {
		video_format_info.modifier = 1;
		modifier	= fb->modifier & 0x00ffffffffffffffULL;
		video_format_info.modifier_arrange = modifier & MASK_FF;
		video_format_info.modifier_compressed = (modifier & BIT_COMP) >> SHIFT_COMP;
		video_format_info.modifier_6bit = (modifier & BIT_6BIT) >> SHIFT_6BIT;
		video_format_info.modifier_10bit = (modifier & BIT_10BIT) >> SHIFT_10BIT;
	}

	DRM_INFO("[%s][%d] %s%s %s(0x%08x) %s%d %s0x%x %s%d %s%d %s%d\n",
		__func__, __LINE__,
		"fourcc:", drm_get_format_name(fb->format->format, &format_name),
		"fourcc:", video_format_info.fourcc,
		"modifier:", video_format_info.modifier,
		"modifier_arrange:", video_format_info.modifier_arrange,
		"modifier_compressed:", video_format_info.modifier_compressed,
		"modifier_6bit:", video_format_info.modifier_6bit,
		"modifier_10bit:", video_format_info.modifier_10bit);

	switch (video_format_info.fourcc) {
	case DRM_FORMAT_YVYU:
		_mtk_video_get_format_yuv(video_format_info, eMemoryFormat);
		break;
	case DRM_FORMAT_YUV444:
		if (video_format_info.modifier &&
			   video_format_info.modifier_6bit &&
			   video_format_info.modifier_compressed)
			//YUV444 6b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_444_6B;
		else if (video_format_info.modifier && video_format_info.modifier_compressed)
			//YUV444 8b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_444_8B;
		else
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_MAX;

		DRM_INFO("%s %d memc format=%d\n", __func__, __LINE__, *eMemoryFormat);
		break;
	case DRM_FORMAT_NV61:
		if (video_format_info.modifier &&
			   video_format_info.modifier_6bit &&
			   video_format_info.modifier_compressed)
			//YUV422 6b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_422_6B;
		else if (video_format_info.modifier && video_format_info.modifier_compressed)
			//YUV422 8b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_422_8B;
		else
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_MAX;

		DRM_INFO("%s %d memc format=%d\n", __func__, __LINE__, *eMemoryFormat);
		break;
	case DRM_FORMAT_NV21:
		if (video_format_info.modifier &&
			   video_format_info.modifier_6bit &&
			   video_format_info.modifier_compressed)
			//YUV420 6b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_420_6B;
		else if (video_format_info.modifier && video_format_info.modifier_compressed)
			//YUV420 8b ce
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_420_8B;
		else
			*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_MAX;

		DRM_INFO("%s %d memc format=%d\n", __func__, __LINE__, *eMemoryFormat);
		break;
	default:
		*eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_MAX;
		break;
	}
	return 0;
}

void _mtk_video_frc_sw_reset(struct mtk_video_context *pctx_video, bool bEnable)
{
	struct hwregFrcSwReset paramInitIn;
	struct hwregOut paramOut;
	struct reg_info *regTbl;
	uint16_t reg_num = pctx_video->reg_num;

	regTbl = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (regTbl == NULL) {
		DRM_ERROR("[%s,%5d] NULL POINTER\n", __func__, __LINE__);
		return;
	}

	memset(&paramInitIn, 0, sizeof(struct hwregFrcSwReset));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(regTbl, 0, sizeof(struct reg_info)*REG_NUM_MAX);

	paramOut.reg = regTbl;
	paramInitIn.RIU = 1;
	paramInitIn.bEnable = bEnable;

	//frc sw reset
	drv_hwreg_render_frc_sw_reset(
		paramInitIn, &paramOut);
}

void _mtk_video_frc_init_reg_1(struct mtk_video_context *pctx_video)
{

	struct hwregFrcInitIn paramInitIn;
	struct hwregFrctriggenIn paramTriggergenIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	//struct reg_info reg[REG_NUM_MAX];

	memset(&paramInitIn, 0, sizeof(struct hwregFrcInitIn));
	memset(&paramTriggergenIn, 0, sizeof(struct hwregFrctriggenIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}

	paramInitIn.RIU = 1;

	//init frc
	drv_hwreg_render_frc_init_1(
		paramInitIn, &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);
}

void _mtk_video_frc_init_reg_2(struct mtk_video_context *pctx_video)
{

	struct hwregFrcInitIn paramInitIn;
	struct hwregFrctriggenIn paramTriggergenIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	//struct reg_info reg[REG_NUM_MAX];

	memset(&paramInitIn, 0, sizeof(struct hwregFrcInitIn));
	memset(&paramTriggergenIn, 0, sizeof(struct hwregFrctriggenIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}

	paramInitIn.RIU = 1;

	//init frc
	drv_hwreg_render_frc_init_2(
		paramInitIn, &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);
}

void _mtk_video_frc_init_reg(struct mtk_video_context *pctx_video)
{
#if (1) //(MEMC_CONFIG == 1)
	struct hwregFrcInitIn paramInitIn;
	struct hwregFrctriggenIn paramTriggergenIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	//struct reg_info reg[REG_NUM_MAX];
	uint16_t reg_size;

	memset(&paramInitIn, 0, sizeof(struct hwregFrcInitIn));
	memset(&paramTriggergenIn, 0, sizeof(struct hwregFrctriggenIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	reg_size = sizeof(struct reg_info)*REG_NUM_MAX;
	paramOut.reg = malloc(reg_size);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}

	paramInitIn.RIU = 1;

	//init frc
	drv_hwreg_render_frc_init(
		paramInitIn, &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);

	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramTriggergenIn.RIU = 1;
	paramOut.reg = malloc(reg_size);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}
	//init trigger gen
	drv_hwreg_render_frc_trig_gen(
		paramTriggergenIn, &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);
#endif
}

uint64_t g_FrcBaseAdr;
void mtk_video_display_set_frc_base_adr(uint64_t adr)
{
	g_FrcBaseAdr = adr;
}

void _mtk_set_frc_base_adr(struct mtk_video_context *pctx_video)
{
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	//struct reg_info reg[REG_NUM_MAX];
	struct hwregFrcBaseAddrIn FrcPamaIn;


	memset(&FrcPamaIn, 0, sizeof(struct hwregFrcBaseAddrIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));

	paramOut.reg = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}

	FrcPamaIn.RIU = 1;
	FrcPamaIn.frc_pq_addr = g_FrcBaseAdr;

	drv_hwreg_render_frc_set_baseAddr(FrcPamaIn,  &paramOut);

	if (paramOut.reg)
		free(paramOut.reg);
}

#define NODE_VR_BASE 0x1C000000
void _mtk_video_frc_set_RiuAdr(uint64_t adr, uint64_t adr_size)
{
	drv_hwreg_common_setRIUaddr(adr, adr_size,
				(uint64_t)ioremap(adr+NODE_VR_BASE, adr_size));
}

#define REG_BK_SIZE 0x200
#define BK_8001 0x1000200
#define BK_1030 0x206000
#define BK_A4F0 0x149E000
#define BK_840A 0x1081400
#define BK_8426 0x1084C00
#define BK_8429 0x1085200
#define BK_8432 0x1086400
#define BK_8433 0x1086600
#define BK_843A 0x1087400
#define BK_8440 0x1088000
#define BK_8442 0x1088400
#define BK_8446 0x1088C00
#define BK_8450 0x108A000
#define BK_8452 0x108A400
#define BK_8453 0x108A600
#define BK_8461 0x108C200
#define BK_84B4 0x1096800
#define BK_84D4 0x109A800
#define BK_84F4 0x109E800
#define BK_8502 0x10A0400
#define BK_8503 0x10A0600
#define BK_8504 0x10A0800
#define BK_851B 0x10A3600
#define BK_851E 0x10A3C00
#define BK_8532 0x10A6400
#define BK_8533 0x10A6600

#define BK_8619 0x10C3200
#define BK_861B 0x10C3600
#define BK_861C 0x10C3800
#define BK_861F 0x10C3E00
#define BK_8620 0x10C4000
#define BK_8621 0x10C4200
#define BK_8622 0x10C4400
#define BK_8623 0x10C4600
#define BK_8624 0x10C4800
#define BK_8625 0x10C4A00
#define BK_8626 0x10C4C00
#define BK_8627 0x10C4E00
#define BK_8628 0x10C5000
#define BK_8629 0x10C5200
#define BK_862A 0x10C5400
#define BK_862B 0x10C5600
#define BK_862C 0x10C5800
#define BK_862D 0x10C5A00
#define BK_862E 0x10C5C00
#define BK_8630 0x10C6000
#define BK_8636 0x10C6C00
#define BK_8637 0x10C6E00
#define BK_8638 0x10C7000
#define BK_863A 0x10C7400
#define BK_8650 0x10CA000

#define BK_A36B 0x146D600
#define BK_A36C 0x146D800
#define BK_A38D 0x1471A00
#define BK_A391 0x1472200
#define BK_A3A0 0x1474000
#define BK_A4F0 0x149E000

#define BK_102A 0x205400
#define BK_103A 0x207400
#define BK_103C 0x207A00

//FRC IP Version 2
#define BK_8416 0x1082C00
#define BK_8417 0x1082E00
#define BK_8418 0x1083000
#define BK_842C 0x1085800
#define BK_8456 0x108AC00
#define BK_8457 0x108AE00
#define BK_8548 0x10A9000
#define BK_854C 0x10A9800
#define BK_8550 0x10AA000
#define BK_8554 0x10AA800
#define BK_8558 0x10AB000
#define BK_8568 0x10AD000



void _mtk_video_frc_init_reg_bank_node_init(uint8_t u8frc_ipversion)
{
	_mtk_video_frc_set_RiuAdr(BK_8001, REG_BK_SIZE);//840A
	_mtk_video_frc_set_RiuAdr(BK_840A, REG_BK_SIZE);//840A
	_mtk_video_frc_set_RiuAdr(BK_8426, REG_BK_SIZE);//8426
	_mtk_video_frc_set_RiuAdr(BK_8429, REG_BK_SIZE);//8429
	_mtk_video_frc_set_RiuAdr(BK_842C, REG_BK_SIZE);//842C
	_mtk_video_frc_set_RiuAdr(BK_8432, REG_BK_SIZE);//8432
	_mtk_video_frc_set_RiuAdr(BK_8433, REG_BK_SIZE);//8433
	_mtk_video_frc_set_RiuAdr(BK_843A, REG_BK_SIZE);//843A
	_mtk_video_frc_set_RiuAdr(BK_8440, REG_BK_SIZE);//8440
	_mtk_video_frc_set_RiuAdr(BK_8442, REG_BK_SIZE);//8442
	_mtk_video_frc_set_RiuAdr(BK_8446, REG_BK_SIZE);//8446
	_mtk_video_frc_set_RiuAdr(BK_8450, REG_BK_SIZE);//8450
	_mtk_video_frc_set_RiuAdr(BK_8452, REG_BK_SIZE);//8452
	_mtk_video_frc_set_RiuAdr(BK_8453, REG_BK_SIZE);//8453
	_mtk_video_frc_set_RiuAdr(BK_8461, REG_BK_SIZE);//8461
	_mtk_video_frc_set_RiuAdr(BK_84B4, REG_BK_SIZE);//84B4
	_mtk_video_frc_set_RiuAdr(BK_84D4, REG_BK_SIZE);//84D4
	_mtk_video_frc_set_RiuAdr(BK_84F4, REG_BK_SIZE);//84F4
	_mtk_video_frc_set_RiuAdr(BK_8502, REG_BK_SIZE);//8502
	_mtk_video_frc_set_RiuAdr(BK_8503, REG_BK_SIZE);//8503
	_mtk_video_frc_set_RiuAdr(BK_8504, REG_BK_SIZE);//8504
	_mtk_video_frc_set_RiuAdr(BK_851B, REG_BK_SIZE);//851B
	_mtk_video_frc_set_RiuAdr(BK_851E, REG_BK_SIZE);//851E
	_mtk_video_frc_set_RiuAdr(BK_8532, REG_BK_SIZE);//8532
	_mtk_video_frc_set_RiuAdr(BK_8533, REG_BK_SIZE);//8533

	_mtk_video_frc_set_RiuAdr(BK_8619, REG_BK_SIZE);//8619
	_mtk_video_frc_set_RiuAdr(BK_861B, REG_BK_SIZE);//861B
	_mtk_video_frc_set_RiuAdr(BK_861C, REG_BK_SIZE);//861C
	_mtk_video_frc_set_RiuAdr(BK_861F, REG_BK_SIZE);//861F
	_mtk_video_frc_set_RiuAdr(BK_8620, REG_BK_SIZE);//8620
	_mtk_video_frc_set_RiuAdr(BK_8621, REG_BK_SIZE);//8621
	_mtk_video_frc_set_RiuAdr(BK_8622, REG_BK_SIZE);//8622
	_mtk_video_frc_set_RiuAdr(BK_8623, REG_BK_SIZE);//8623
	_mtk_video_frc_set_RiuAdr(BK_8624, REG_BK_SIZE);//8624
	_mtk_video_frc_set_RiuAdr(BK_8625, REG_BK_SIZE);//8625
	_mtk_video_frc_set_RiuAdr(BK_8626, REG_BK_SIZE);//8626
	_mtk_video_frc_set_RiuAdr(BK_8627, REG_BK_SIZE);//8627
	_mtk_video_frc_set_RiuAdr(BK_8628, REG_BK_SIZE);//8628
	_mtk_video_frc_set_RiuAdr(BK_8629, REG_BK_SIZE);//8629
	_mtk_video_frc_set_RiuAdr(BK_862A, REG_BK_SIZE);//862A
	_mtk_video_frc_set_RiuAdr(BK_862B, REG_BK_SIZE);//862B
	_mtk_video_frc_set_RiuAdr(BK_862C, REG_BK_SIZE);//862C
	_mtk_video_frc_set_RiuAdr(BK_862D, REG_BK_SIZE);//862D
	_mtk_video_frc_set_RiuAdr(BK_862E, REG_BK_SIZE);//862E
	_mtk_video_frc_set_RiuAdr(BK_8630, REG_BK_SIZE);//8630
	_mtk_video_frc_set_RiuAdr(BK_8636, REG_BK_SIZE);//8636
	_mtk_video_frc_set_RiuAdr(BK_8637, REG_BK_SIZE);//8637
	_mtk_video_frc_set_RiuAdr(BK_8638, REG_BK_SIZE);//8638
	_mtk_video_frc_set_RiuAdr(BK_863A, REG_BK_SIZE);//863A
	_mtk_video_frc_set_RiuAdr(BK_8650, REG_BK_SIZE);//8650
	if (u8frc_ipversion == FRC_IP_VERSION_2) {
		_mtk_video_frc_set_RiuAdr(BK_8416, REG_BK_SIZE);//8416
		_mtk_video_frc_set_RiuAdr(BK_8417, REG_BK_SIZE);//8417
		_mtk_video_frc_set_RiuAdr(BK_8418, REG_BK_SIZE);//8418
		_mtk_video_frc_set_RiuAdr(BK_842C, REG_BK_SIZE);//842C
		_mtk_video_frc_set_RiuAdr(BK_8456, REG_BK_SIZE);//8456
		_mtk_video_frc_set_RiuAdr(BK_8457, REG_BK_SIZE);//8457
		_mtk_video_frc_set_RiuAdr(BK_8548, REG_BK_SIZE);//8548
		_mtk_video_frc_set_RiuAdr(BK_854C, REG_BK_SIZE);//854C
		_mtk_video_frc_set_RiuAdr(BK_8550, REG_BK_SIZE);//8550
		_mtk_video_frc_set_RiuAdr(BK_8554, REG_BK_SIZE);//8554
		_mtk_video_frc_set_RiuAdr(BK_8558, REG_BK_SIZE);//8558
		_mtk_video_frc_set_RiuAdr(BK_8568, REG_BK_SIZE);//8568
		_mtk_video_frc_set_RiuAdr(BK_A3A0, REG_BK_SIZE);//A3A0
	}
}

void _mtk_video_frc_init_reg_bank_node_loop(void)
{
	_mtk_video_frc_set_RiuAdr(BK_A4F0, REG_BK_SIZE);//A4F0

	_mtk_video_frc_set_RiuAdr(BK_A36B, REG_BK_SIZE);//A36B
	_mtk_video_frc_set_RiuAdr(BK_A36C, REG_BK_SIZE);//A36C
	_mtk_video_frc_set_RiuAdr(BK_A38D, REG_BK_SIZE);//A38D
	_mtk_video_frc_set_RiuAdr(BK_A391, REG_BK_SIZE);//A391
	_mtk_video_frc_set_RiuAdr(BK_A4F0, REG_BK_SIZE);//A4F0

}

#define MTK_DRM_TV_FRC_COMPATIBLE_STR "mediatek,mediatek-frc"

int readDTB2FRCPrivate(struct mtk_video_context *pctx_video)
{
	int ret = 0;
	struct device_node *np;
	__u32 u32Tmp = 0x0;
	const char *name;

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_FRC_COMPATIBLE_STR);

	if (np != NULL) {
		// read frc ip version
		name = "FRC_IP_VERSION";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {
			DRM_ERROR("[%s, %d] Get DTS failed, name = %s \r\n",
				__func__, __LINE__, name);
			return ret;
		}
		pctx_video->frc_ip_version = u32Tmp;
	}

	return ret;
}

static int _mtk_video_display_frc_set_mux_clock(struct device *dev,
			int clk_index, const char *dev_clk_name, bool bExistParent)
{
	int ret = 0;
	struct clk *dev_clk;
	struct clk_hw *clk_hw;
	struct clk_hw *parent_clk_hw;

	dev_clk = devm_clk_get(dev, dev_clk_name);

	if (IS_ERR_OR_NULL(dev_clk)) {
		DRM_ERROR("[FRC][%s, %d]: failed to get %s\n", __func__, __LINE__, dev_clk_name);
		return -ENODEV;
	}
	DRM_INFO("[FRC][%s, %d]clk_name = %s\n", __func__, __LINE__, __clk_get_name(dev_clk));

	if (bExistParent == true) {
		clk_hw = __clk_get_hw(dev_clk);
		parent_clk_hw = clk_hw_get_parent_by_index(clk_hw, clk_index);
		if (IS_ERR_OR_NULL(parent_clk_hw)) {
			DRM_ERROR("[FRC][%s, %d]failed to get parent_clk_hw\n", __func__, __LINE__);
			return -ENODEV;
		}

		//set_parent clk
		ret = clk_set_parent(dev_clk, parent_clk_hw->clk);
		if (ret) {
			DRM_ERROR("[FRC][%s, %d]failed to change clk_index [%d]\n",
				__func__, __LINE__, clk_index);
			return -ENODEV;
		}
	}

	//prepare and enable clk
	ret = clk_prepare_enable(dev_clk);

	if (ret) {
		DRM_ERROR("[FRC][%s, %d]failed to enable clk_name = %s\n",
				__func__, __LINE__,  __clk_get_name(dev_clk));
		return -ENODEV;
	}

	return ret;
}

static int _mtk_video_display_frc_set_dis_clock(struct device *dev,
			const char *dev_clk_name)
{
	int ret = 0;
	struct clk *dev_clk;

	dev_clk = devm_clk_get(dev, dev_clk_name);

	if (IS_ERR_OR_NULL(dev_clk)) {
		DRM_ERROR("[FRC][%s, %d]: failed to get %s\n", __func__, __LINE__, dev_clk_name);
		return -ENODEV;
	}

	DRM_INFO("[FRC][%s, %d]clk_name = %s\n", __func__, __LINE__, __clk_get_name(dev_clk));

	//disable clk
	clk_disable_unprepare(dev_clk);

	devm_clk_put(dev, dev_clk);

	return ret;
}

#define CK_NUM_MAX     (40)
#define DIS_CK_NUM_MAX (40)

#define MUX_CK_NUM_MAX (4)

static const char *ck_name[CK_NUM_MAX] = {
	"MEMC_EN_FRC_FCLK_2X2DEFLICKER",
	"MEMC_EN_FRC_FCLK_2X2FDCLK_2X",
	"MEMC_EN_FRC_FCLK_2X2FRCIOPM",
	"MEMC_EN_FRC_FCLK_2X2FRCIOPMRV55",
	"MEMC_EN_FRC_FCLK_2X2MI_PH3",
	"MEMC_EN_FRC_FCLK_2X2MI_PH4",
	"MEMC_EN_FRC_FCLK_2X2OUT_STAGE",
	"MEMC_EN_FRC_FCLK_2X2SNR",
	"MEMC_EN_FRC_FCLK_2XPLUS2FRCIOPMRV55",
	"MEMC_EN_FRC_FCLK_2XPLUS2SCIP",
	"MEMC_EN_FRC_FCLK_2XPLUS2SCMW",
	"MEMC_EN_FRC_FCLK2FDCLK",
	"MEMC_EN_FRC_FCLK2FRCIOPM",
	"MEMC_EN_FRC_FCLK2FRCIOPMRV55",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_ALL",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_MLB_SRAM",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_0",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_1",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_2",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_3",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_4",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_5",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_6",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_7",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_GMV",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_ALL",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_L",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_R",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_SRAM_L",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_SRAM_R",
	"MEMC_EN_FRC_FCLK2ME",
	"MEMC_EN_FRC_FCLK2ME_4X4",
	"MEMC_EN_FRC_FCLK2ME_PH1",
	"MEMC_EN_FRC_FCLK2ME_PH2",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_8",
	"MEMC_EN_XTAL_24M2FRCIOPM",
	"MEMC_EN_XTAL_24M2FRCIOPMRV55",
	"MEMC_EN_XTAL_24M2FRCMEHALO",
	"MEMC_EN_XTAL_24M2FRCMEMLB",
	"MEMC_EN_XTAL_24M2FRCMI"
};

static const char *mux_ck_name[MUX_CK_NUM_MAX] = {
	"MEMC_FRC_FCLK_2X_INT_CK",
	"MEMC_FRC_FCLK_INT_CK",
	"TOPGEN_FRC_FCLK_2X_CK",
	"TOPGEN_FRC_FCLK_2XPLUS_CK"
};

static const char *dis_ck_name[DIS_CK_NUM_MAX] = {
	"MEMC_EN_FRC_FCLK_2X2DEFLICKER",
	"MEMC_EN_FRC_FCLK_2X2FDCLK_2X",
	"MEMC_EN_FRC_FCLK_2X2FRCIOPM",
	"MEMC_EN_FRC_FCLK_2X2FRCIOPMRV55",
	"MEMC_EN_FRC_FCLK_2X2MI_PH3",
	"MEMC_EN_FRC_FCLK_2X2MI_PH4",
	"MEMC_EN_FRC_FCLK_2X2OUT_STAGE",
	"MEMC_EN_FRC_FCLK_2X2SNR",
	"MEMC_EN_FRC_FCLK_2XPLUS2FRCIOPMRV55",
	"MEMC_EN_FRC_FCLK_2XPLUS2SCIP",
	"MEMC_EN_FRC_FCLK_2XPLUS2SCMW",
	"MEMC_EN_FRC_FCLK2FDCLK",
	"MEMC_EN_FRC_FCLK2FRCIOPM",
	"MEMC_EN_FRC_FCLK2FRCIOPMRV55",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_ALL",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_MLB_SRAM",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_0",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_1",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_2",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_3",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_4",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_5",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_6",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_7",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_GMV",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_ALL",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_L",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_R",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_SRAM_L",
	"MEMC_EN_FRC_FCLK2FRCMI_CLK_MLB_MI_SRAM_R",
	"MEMC_EN_FRC_FCLK2ME",
	"MEMC_EN_FRC_FCLK2ME_4X4",
	"MEMC_EN_FRC_FCLK2ME_PH1",
	"MEMC_EN_FRC_FCLK2ME_PH2",
	"MEMC_EN_FRC_FCLK2FRCMEMLB_CLK_SAD_8",
	"MEMC_EN_XTAL_24M2FRCIOPM",
	"MEMC_EN_XTAL_24M2FRCIOPMRV55",
	"MEMC_EN_XTAL_24M2FRCMEHALO",
	"MEMC_EN_XTAL_24M2FRCMEMLB",
	"MEMC_EN_XTAL_24M2FRCMI"
};

//REG_01F0_CKGEN01_REG_CKG_FRC_FCLK_2X_FRCIOPMRV55
//0:frc_fclk_2x_ck, 1:frc_fclk_ck,
//REG_01E8_CKGEN01_REG_CKG_FRC_FCLK_FRCMI
//0:frc_fclk_ck, 1:frc_fclk_2x_ck,
//REG_01F0_CKGEN01_REG_CKG_FRC_FCLK_2X
//0:xcsrpll_vcod1_630m_ck, 1:sys2pll_vcod4_360m_ck, 2:mpll_vcod3_288m_ck, 3:mpll_vcod4_216m_ck
//REG_01F8_CKGEN01_REG_CKG_FRC_FCLK_2XPLUS
//0:xcpll_vcod1_720m_ck, 1:DUMMY_MIMI, 2:sys2pll_vcod4_360m_ck, 3:mpll_vcod3_288m_ck


static int mux_ck_val[MUX_CK_NUM_MAX] = {
	1,
	1,
	1,
	0
};

static int mtk_video_display_frc_init_clock(struct device *dev,
	struct mtk_video_context *pctx_video)
{
	uint8_t ck_num = 0;
	uint8_t mux_ck_num = 0;

	for (ck_num = 0; ck_num < CK_NUM_MAX; ck_num++) {
		if (_mtk_video_display_frc_set_mux_clock(dev, 0, ck_name[ck_num], FALSE))
			return -ENODEV;
	}

	//FRC mux clock setting
	for (mux_ck_num = 0; mux_ck_num < MUX_CK_NUM_MAX; mux_ck_num++) {
		if (_mtk_video_display_frc_set_mux_clock(dev,
			mux_ck_val[mux_ck_num], mux_ck_name[mux_ck_num], TRUE))
			return -ENODEV;
	}

	return 0;
}

static int mtk_video_display_frc_disable_clock(struct device *dev)
{
	uint8_t ck_num = 0;
	uint8_t mux_ck_num = 0;

	for (ck_num = 0; ck_num < DIS_CK_NUM_MAX; ck_num++) {
		if (_mtk_video_display_frc_set_dis_clock(dev, dis_ck_name[ck_num]))
			return -ENODEV;
	}

	return 0;
}

void mtk_video_display_frc_suspend(struct device *dev)
{
	int ret;

	ret = mtk_video_display_frc_disable_clock(dev);

}

void mtk_video_display_frc_init(struct device *dev, struct mtk_video_context *pctx_video)
{
	int ret;

	ret = readDTB2FRCPrivate(pctx_video);

	if (ret != 0) {
		DRM_ERROR("[%s, %d] Read DTB failed.\n",
			__func__, __LINE__);
	}
	DRM_INFO("\nfrc_ip_version=%x\n", pctx_video->frc_ip_version);

	ret = drv_hwreg_render_frc_ipversion(pctx_video->frc_ip_version);

	if (ret != 0) {
		DRM_ERROR("[%s, %d] Set frc ipversion failed.\n",
			__func__, __LINE__);
	}

	//init RV55;
	#if (0)
		_mtk_video_display_set_init_value_for_rv55();
	#else
		memc_init_value_for_rv55 = INIT_VALUE_PASS;
	#endif
	_mtk_video_frc_init_reg_bank_node_init(pctx_video->frc_ip_version);

	//hal_hwreg would !(true) = 0 to set Rst, patch for Machili
	//8429_01[0] = 0 is Rst, 8429_01[0] = 1 is not Rst
	_mtk_video_frc_sw_reset(pctx_video, true);

	ret = mtk_video_display_frc_init_clock(dev, pctx_video);

	if (ret != 0) {
		DRM_ERROR("[%s, %d] Frc_init_clock failed.\n",
			__func__, __LINE__);
	}

	pctx_video->bIsmemc_1stInit = true;
	pctx_video->bIspre_memc_en_Status = false;
	pctx_video->memc_InitState = MEMC_INIT_CHECKRV55;
	pctx_video->frc_latency = 0;
	pctx_video->frc_pattern_sel = MEMC_PAT_OFF;
	pctx_video->frc_pattern_trig_cnt = 0;

	_mtk_video_frc_init_reg_1(pctx_video);
	_mtk_video_frc_init_reg_2(pctx_video);
	_mtk_video_frc_init_reg(pctx_video);
}

void mtk_video_display_set_frc_ins(unsigned int plane_inx,
	struct mtk_video_context *pctx_video,
	struct dma_buf *meta_db,
	struct drm_framebuffer *fb)
{
	static uint16_t preHvspOutWinWAlign[MAX_WINDOW_NUM] = {0};
	struct m_pq_display_frc_scaling pqu_display_frc_scaling;
	struct m_pq_display_flow_ctrl pqu_display_flow_info;

	//struct reg_info reg[REG_NUM_MAX];
	struct hwregFrcInsIn paramFrcIn;
	struct hwregOut paramOut;
	struct sm_ds_add_info ds_info;
	struct sm_ds_skip_info ds_skip_info;
	int32_t ds_instance;
	uint32_t temp_value, used_depth;
	struct mtk_video_plane_ctx *plane_ctx = (pctx_video->plane_ctx + plane_inx);

	memset(&paramFrcIn, 0, sizeof(struct hwregFrcInsIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&ds_info, 0, sizeof(struct sm_ds_add_info));

	// 1.get the frc scaling from pqu_metadata
	memset(&pqu_display_frc_scaling, 0, sizeof(struct m_pq_display_frc_scaling));
	if (mtk_render_common_read_metadata(meta_db,
		RENDER_METATAG_FRC_SCALING_INFO, &pqu_display_frc_scaling)) {
		DRM_ERROR("[%s][%d] Read metadata RENDER_METATAG_FRC_SCALING_INFO fail\n",
			__func__, __LINE__);
			return;
	}
	DRM_INFO("[%s][%d] Read metadata ds_instance[%d], ds_index[%d], ds_depth[%d]\n",
		__func__, __LINE__, pqu_display_frc_scaling.ds_instance,
		pqu_display_frc_scaling.ds_index, pqu_display_frc_scaling.ds_depth);

	// 2.get the disp fllow from pqu_metadata
	memset(&pqu_display_flow_info, 0, sizeof(struct m_pq_display_flow_ctrl));
	if (mtk_render_common_read_metadata(meta_db,
		RENDER_METATAG_PQU_DISPLAY_INFO, &pqu_display_flow_info)) {
		DRM_ERROR("[%s][%d] Read metadata RENDER_METATAG_PQU_DISPLAY_INFO fail\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("[%s][%d] Read PQU ip_hvsp_out metadata width[%d], height[%d]\n algin[%d/%d]",
		__func__, __LINE__, pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].width,
		pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].height,
		pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].w_align,
		pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].h_align);

	// 3.set RIU
	paramOut.reg = malloc(sizeof(struct reg_info)*REG_NUM_MAX);
	if (paramOut.reg)
		memset(paramOut.reg, 0, sizeof(struct reg_info)*REG_NUM_MAX);
	else {
		DRM_ERROR("\n[%s,%d]malloc fail", __func__, __LINE__);
		return;
	}
	paramFrcIn.RIU = 0; // Using DS
	//paramFrcIn.hvspOut_width = pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].width;
	//paramFrcIn.hvspOut_height = pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].height;
	//paramFrcIn.hvspOut_w_align = pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].w_align;
	//paramFrcIn.hvspOut_h_align = pqu_display_flow_info.ip_win[pqu_ip_hvsp_out].h_align;
	paramFrcIn.hvspOut_width = pqu_display_frc_scaling.hvspout_win.width;
	paramFrcIn.hvspOut_height = pqu_display_frc_scaling.hvspout_win.height;
	paramFrcIn.hvspOut_w_align = pqu_display_frc_scaling.hvspout_win.w_align;
	paramFrcIn.hvspOut_h_align = pqu_display_frc_scaling.hvspout_win.h_align;
	paramFrcIn.buffer_h = fb->width;
	paramFrcIn.buffer_v = fb->height;
	if ((paramFrcIn.hvspOut_w_align > fb->width) ||
		(paramFrcIn.hvspOut_h_align > fb->height)) {
		DRM_ERROR("[%s][%d] hvspOut_w_align[%d] >= buffer_h[%d] shouldn't blk_ins!\n",
			__func__, __LINE__, paramFrcIn.hvspOut_w_align, fb->width);
		DRM_ERROR("[%s][%d] hvspOut_h_align[%d] >= buffer_v[%d] shouldn't blk_ins!\n",
			__func__, __LINE__, paramFrcIn.hvspOut_h_align, fb->height);

		if (paramOut.reg)
			free(paramOut.reg);
		return;
	}
	if ((plane_ctx->old_memc_hvspout_w_align == paramFrcIn.hvspOut_w_align) &&
		(plane_ctx->old_memc_hvspout_h_align == paramFrcIn.hvspOut_h_align)) {
		DRM_INFO("[%s][%d] old_memc_hvspout_w[%d] == curHvspOut_w_align[%d] return!\n",
			__func__, __LINE__, plane_ctx->old_memc_hvspout_w_align,
			paramFrcIn.hvspOut_w_align);
		DRM_INFO("[%s][%d] old_memc_hvspout_h[%d] == curHvspOut_w_align[%d] return!\n",
			__func__, __LINE__, plane_ctx->old_memc_hvspout_h_align,
			paramFrcIn.hvspOut_h_align);

		if (paramOut.reg)
			free(paramOut.reg);
		return;
	}
	//printk("old_w[%d], old_h[%d], w[%d], h[%d], w_a[%d], h_a[%d], fb_w[%d], fb_h[%d]\n",
		//plane_ctx->old_memc_hvspout_w_align, plane_ctx->old_memc_hvspout_h_align,
		//paramFrcIn.hvspOut_width, paramFrcIn.hvspOut_height,
		//paramFrcIn.hvspOut_w_align, paramFrcIn.hvspOut_h_align,
		//fb->width, fb->height);
	plane_ctx->old_memc_hvspout_w_align = paramFrcIn.hvspOut_w_align;
	plane_ctx->old_memc_hvspout_h_align = paramFrcIn.hvspOut_h_align;

	drv_hwreg_render_frc_set_ins(paramFrcIn, &paramOut);

	//4.add DS cmd
	if ((!paramFrcIn.RIU) && (paramOut.regCount != 0)) {
		//DS cmd
		ds_instance = pqu_display_frc_scaling.ds_instance;
		ds_info.reg = (struct sm_reg *)(paramOut.reg);
		ds_info.reg_count = paramOut.regCount;
		ds_info.mem_index = pqu_display_frc_scaling.ds_index;
		ds_info.sync_id = DS_DISP_SYNC;
		sm_ds_add_cmd(ds_instance, &ds_info);
		//Skip cmd
		temp_value = paramOut.regCount / DS_ALIGNMENT;
		used_depth = ((temp_value) ? (temp_value + 1) : (temp_value));
		ds_skip_info.mem_index = pqu_display_frc_scaling.ds_index;
		ds_skip_info.sync_id = DS_DISP_SYNC;
		ds_skip_info.skip_depth = pqu_display_frc_scaling.ds_depth - used_depth;
		DRM_INFO("[%s][%d] ds_info skip_depth[%d], temp_value[%d], reg_count[%d]\n",
			__func__, __LINE__, ds_skip_info.skip_depth, temp_value,
			paramOut.regCount);
		sm_ds_add_skip_cmd(ds_instance, &ds_skip_info);
	}

	if (paramOut.reg)
		free(paramOut.reg);
}

//set MEMC_OPM path
void mtk_video_display_set_frc_opm_path(struct mtk_plane_state *state,
						 bool enable)
{
	struct drm_plane *plane = state->base.plane;
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct mtk_video_plane_ctx *plane_ctx = (pctx_video->plane_ctx + plane_inx);
	struct drm_framebuffer *fb = state->base.fb;
	enum E_DRV_IPM_MEMORY_CONFIG eMemoryFormat = E_DRV_IPM_MEMORY_CONFIG_MAX;


	if (connect_frc_rv55_is_ok()) {

		if (pctx_video->memc_InitState == MEMC_INIT_CHECKRV55) {
			pctx_video->memc_InitState = MEMC_INIT_SENDDONE;
			_mtk_video_display_set_init_value_for_rv55();
		}
	}

	//set info for rv55
	if (enable && (pctx_video->memc_InitState != MEMC_INIT_CHECKRV55)) {
		//set info for rv55
		//Parsing_json_file
		_mtk_video_display_parsing_json_file(plane_inx, pctx);

		//set memc input size
		_mtk_video_display_set_input_frame_size(pctx, state);

		//set memc output size
		_mtk_video_display_set_output_frame_size(pctx, state);

		//set memc crop win change
		DRM_INFO("[%s,%d] D1 = 0x%x, D2 = 0x%x, D3 = 0x%x, D4 = 0x%x\n",
			"FRC_MB_CMD_SET_MFC_FRC_CROPWIN_CHANGE", __LINE__,
			state->base.crtc_w, state->base.crtc_h, DEFAULT_VALUE, DEFAULT_VALUE);

		//set timing
		_mtk_video_display_set_timing(plane_inx, pctx);

		//set lpll lock done
		_mtk_video_display_set_fpll_lock_done(pctx);

		//set frc mode
		_mtk_video_display_set_frc_mode(pctx, FRCMode);

		//Qos
		_mtk_video_display_update_Qos(pctx);


		//set memc mode
		//_mtk_video_display_set_mfc_mode(MFCMode);

		//set memc level
		_mtk_video_display_set_memc_level_update(pctx, MFCLevel);
		//set demo mode
		_mtk_video_display_set_demo_mode(pctx, DEMOMode);

		_mtk_video_display_get_mfc_alg_status(pctx);

		_mtk_set_frc_base_adr(pctx_video);
		_mtk_video_display_frc_vs_trig_ctl(pctx);

	}

	_mtk_video_frc_get_latency(plane_inx, pctx, enable);

	//the following setting is only 1 time for MEMC status change
	if (pctx_video->bIspre_memc_en_Status == enable)
		return;

	DRM_INFO("[%s][%d] plane %d: enable %u\n",
		__func__, __LINE__, plane_inx, enable);

	if (enable) {

		if (pctx_video->bIsmemc_1stInit) {
			pctx_video->bIsmemc_1stInit = false;
			_mtk_video_frc_init_reg_bank_node_loop();
		}

		//init clock
		_mtk_video_display_set_frc_clock(pctx_video, enable);


		//get frc_pq buf size
		_mtk_video_display_get_frc_memory_size(plane_inx, pctx);

		//alloc frc_pq buf
		_mtk_video_display_alloc_frc_memory(plane_inx, pctx);

		//set frc_pq buf baseaddr
		_mtk_video_display_set_frc_memory_addr(plane_inx, pctx);

		// check fb memory format settins
		_mtk_video_display_get_frc_fb_memory_format(fb, &eMemoryFormat);

		//load 3D Qmap
		_mtk_video_display_set_frc_3d_table(plane_inx, pctx);

		//set DSCL
		_mtk_video_display_set_frc_dscl();

		//unmask frc_pq
		_mtk_video_display_unmask_frc(pctx_video, enable);

		//Patch for vop2memcMuxSelect
		_mtk_video_display_set_frc_vop2memcMuxSelect(pctx_video, enable);

		//blending out
		_mtk_video_display_set_frc_blendOut(pctx_video, enable);
		if (pctx_video->memc_InitState == MEMC_INIT_SENDDONE) {
			_mtk_video_display_set_init_done_for0tv55();
			pctx_video->memc_InitState = MEMC_INIt_FINISH;
		}

	} else {
		//mask frc_pq
		_mtk_video_display_unmask_frc(pctx_video, enable);

		//release frc_pq buf and clear frc_pq buf addr register
		_mtk_video_display_release_frc_memory(plane_inx, pctx);

		/*clean address register*/
		_mtk_video_display_set_frc_memory_addr(plane_inx, pctx);

		//close clock
		//_mtk_video_display_set_frc_clock(pctx_video, enable);
	}
	pctx_video->bIspre_memc_en_Status = enable;
}

void mtk_video_display_set_frc_property(
	struct video_plane_prop *plane_props,
	enum ext_video_plane_prop prop)
{
	switch (prop) {
	case EXT_VPLANE_PROP_MEMC_LEVEL:
		break;
	case EXT_VPLANE_PROP_MEMC_GAME_MODE:
		break;
	case EXT_VPLANE_PROP_MEMC_PATTERN:
		break;
	case EXT_VPLANE_PROP_MEMC_MISC_TYPE:
		break;
	case EXT_VPLANE_PROP_MEMC_DECODE_IDX_DIFF:
		break;
	default:
		break;
	}

	DRM_INFO("[%s][%d] memc ext_prop:[%d]val= %d old_val= %d\n", __func__, __LINE__,
		prop, plane_props->prop_val[prop], plane_props->old_prop_val[prop]);

}

void mtk_video_display_get_frc_property(
	struct video_plane_prop *plane_props,
	enum ext_video_plane_prop prop)
{
	switch (prop) {
	case EXT_VPLANE_PROP_MEMC_STATUS:
		break;
	case EXT_VPLANE_PROP_MEMC_TRIG_GEN:
		plane_props->prop_val[prop] = drv_hwreg_render_frc_get_trig_gen();
		break;
	case EXT_VPLANE_PROP_MEMC_RV55_INFO:
		plane_props->prop_val[prop] = memc_rv55_info;
		memc_rv55_info = (memc_rv55_info&
			(~(MEMC_STATUS|DEMO_STATUS|MEMC_LV_STATUS|CMD_STATUS)));
		break;
	case EXT_VPLANE_PROP_MEMC_INIT_VALUE_FOR_RV55:
		plane_props->prop_val[prop] = memc_init_value_for_rv55;
		break;
	default:
		break;
	}

	DRM_INFO("[%s][%d] memc ext_prop:[%d]val= %llu old_val= %llu\n", __func__, __LINE__,
		prop, plane_props->prop_val[prop], plane_props->old_prop_val[prop]);

}

#define _PAT_TRIG_CNT 10
void mtk_video_display_frc_set_pattern(
		struct mtk_tv_kms_context *pctx,
		enum mtk_video_frc_pattern sel)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;


	pctx_video->frc_pattern_sel = sel;

	switch (pctx_video->frc_pattern_sel) {
	case MEMC_PAT_OFF:
		_mtk_video_display_frc_set_ipm_pattern(pctx_video, false);
		_mtk_video_display_frc_set_opm_pattern(pctx_video, false);
		break;
	case MEMC_PAT_OPM:
		_mtk_video_display_frc_set_opm_pattern(pctx_video, true);
		break;
	case MEMC_PAT_IPM_DYNAMIC:
		_mtk_video_display_frc_set_ipm_pattern(pctx_video, true);
		break;
	case MEMC_PAT_IPM_STATIC:
		_mtk_video_display_frc_set_ipm_pattern(pctx_video, true);
		pctx_video->frc_pattern_trig_cnt = _PAT_TRIG_CNT;
		break;
	default:
		break;
	}
}

void mtk_video_display_set_frc_freeze(
		struct mtk_video_context *pctx_video,
		bool enable)
{

	struct hwregFrcClockIn paramFrcClockIn;
	struct hwregOut paramOut;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];

	memset(&paramFrcClockIn, 0, sizeof(struct hwregFrcClockIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(reg, 0, sizeof(reg));

	paramOut.reg = reg;

	paramFrcClockIn.RIU = 1;
	paramFrcClockIn.bEnable = enable;

	drv_hwreg_render_frc_set_freeze(
		paramFrcClockIn, &paramOut);

}
