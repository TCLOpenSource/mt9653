/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_AVD_H
#define MTK_SRCCAP_AVD_H

#include <linux/videodev2.h>
#include <linux/types.h>
#include "mtk_srccap.h"
#include "mtk_srccap_avd.h"
#include "mtk_srccap_avd_avd.h"
#include "mtk_srccap_avd_mux.h"
#include "mtk_srccap_avd_colorsystem.h"

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
//for avd decompose
#define DECOMPOSE

#ifdef DECOMPOSE
#include "drvAVD.h"
#endif

#define TEST_MODE         0  // 1: for TEST
#define PLAY_MODE         0
#define SCAN_MODE         1

//check avd status
#define _AVD_DATA_VALID            0x0001UL
#define _AVD_VSYNC_TYPE_PAL        0x1000UL
#define _AVD_IS_HSYNC_LOCK         0x4000UL

// Check signal status
//Cvbs Play mode
#define _AVD_CVBS_PLAY_STABLE_CHECK_COUNT  (20)
#define _AVD_CVBS_PLAY_HSYNC_POLLING_TIME  (20)
#define _AVD_CVBS_PLAY_CHECK_COUNT         (10)
#define _AVD_CVBS_PLAY_TIME_OUT (600)
//Atv Play mode
#define _AVD_ATV_PLAY_STABLE_CHECK_COUNT  (20)
#define _AVD_ATV_PLAY_HSYNC_POLLING_TIME  (20)
#define _AVD_ATV_PLAY_CHECK_COUNT         (5)
#define _AVD_ATV_PLAY_TIME_OUT (600)
//Atv Scan mode
#define _AVD_ATV_SCAN_STABLE_CHECK_COUNT	(20)
#define _AVD_ATV_SCAN_HSYNC_POLLING_TIME	(20)
#define _AVD_ATV_SCAN_CHECK_COUNT			(5)
#define _AVD_ATV_SCAN_TIME_OUT (600)

//Sampling info
#define AVD_SAMPLING_INFO_NUM (7)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */
#define AVD_STATUS_LOCK (_AVD_DATA_VALID | _AVD_IS_HSYNC_LOCK)
#define IsAVDLOCK(Status) ((Status & AVD_STATUS_LOCK) == AVD_STATUS_LOCK)
#define IsVSYNCTYPE(Status) ((Status & _AVD_VSYNC_TYPE_PAL) == _AVD_VSYNC_TYPE_PAL)
#define FIND_STAGE_BY_KEY(preStage, cntStage) ((preStage == cntStage) ?\
	((cntStage == TRUE) ? V4L2_SIGNAL_KEEP_TRUE : V4L2_SIGNAL_KEEP_FALSE) :\
	((cntStage == TRUE) ? V4L2_SIGNAL_CHANGE_TRUE : V4L2_SIGNAL_CHANGE_FALSE))

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum V4L2_AVD_RESULT {
	V4L2_EXT_AVD_FAIL = -1,
	V4L2_EXT_AVD_OK = 0,
	V4L2_EXT_AVD_NOT_SUPPORT,
	V4L2_EXT_AVD_NOT_IMPLEMENT,
};

enum V4L2_AVD_SIGNAL_STAGE {
	V4L2_SIGNAL_CHANGE_FALSE = 0,
	V4L2_SIGNAL_CHANGE_TRUE,
	V4L2_SIGNAL_KEEP_TRUE,
	V4L2_SIGNAL_KEEP_FALSE,
};

enum V4L2_AVD_MEM_TYPE {
	V4L2_MEMORY_TYPE_MMAP = 0,
	V4L2_MEMORY_TYPE_CMA,
};

/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct srccap_avd_vd_sampling {
	enum v4l2_ext_avd_videostandardtype videotype;
	u16 v_start;
	u16 h_start;
	u16 h_de;
	u16 v_de;
	u16 h_total;
	u32 v_freqx1000;
};

struct stAvdCount {
	u8 u8StableCheckCount;
	u8 u8PollingTime;
	u8 u8CheckCount;
	u32 u8Timeout;
};
struct std_descr {
	v4l2_std_id std;
	const char *descr;
};

struct srccap_avd_clock {
	struct clk *cmbai2vd;
	struct clk *cmbao2vd;
	struct clk *cmbbi2vd;
	struct clk *cmbbo2vd;
	struct clk *mcu_mail02vd;
	struct clk *mcu_mail12vd;
	struct clk *smi2mcu_m2mcu;
	struct clk *smi2vd;
	struct clk *vd2x2vd;
	struct clk *vd_32fsc2vd;
	struct clk *vd2vd;
	struct clk *xtal_12m2vd;
	struct clk *xtal_12m2mcu_m2riu;
	struct clk *xtal_12m2mcu_m2mcu;
	struct clk *clk_vd_atv_input;
	struct clk *clk_vd_cvbs_input;
	struct clk *clk_buf_sel;
};

struct avd_sensibility {
	u8 init;
	u32 normal_detect_win_before_lock;
	u32 noamrl_detect_win_after_lock;
	u32 normal_cntr_fail_before_lock;
	u32 normal_cntr_sync_before_lock;
	u32 normal_cntr_sync_after_lock;
	u32 scan_detect_win_before_lock;
	u32 scan_detect_win_after_lock;
	u32 scan_cntr_fail_before_lock;
	u32 scan_cntr_sync_before_lock;
	u32 scan_cntr_sync_after_lock;
	u8 scan_hsync_check_count;
};

struct avd_patchflag {
	u8 init;
	u32 init_patch_flag;
};

struct avd_factory {
	u8 init;
	u32 fix_gain;
	u32 fine_gain;
	u32 color_kill_high_bound;
	u32 color_kill_low_bound;
};

struct srccap_avd_cus_setting {
	struct avd_sensibility sensibility;
	struct avd_patchflag patchflag;
	struct avd_factory factory;
	enum V4L2_AVD_MEM_TYPE memtype;
};
struct srccap_avd_info {
	enum v4l2_timingdetect_status enVdSignalStatus;
	enum V4L2_AVD_SIGNAL_STAGE enSignalStage;
	enum v4l2_ext_avd_videostandardtype eStableTvsystem;
	enum v4l2_srccap_input_source eInputSource;
	struct v4l2_timing stVDtiming;
	struct srccap_avd_clock stclock;
	struct srccap_avd_vd_sampling *vdsampling_table;
	struct v4l2_srccap_pqmap_rm_info pqRminfo;
	struct srccap_avd_cus_setting cus_setting;
	u16 vdsampling_table_entry_count;
	v4l2_std_id u64DetectTvSystem;
	v4l2_std_id u64ForceTvSystem;
	v4l2_std_id region_type;
	u64 Comb3dBufAddr;
	u32 Comb3dBufSize;
	u32 Comb3dBufHeapId;
	u32 u32Comb3dAliment;
	u32 u32Comb3dSize;
	u8 *pu8RiuBaseAddr;
	u32 u32DramBaseAddr;
	u64 *pIoremap;
	u32 regCount;
	u8 reg_info_offset;
	u8 reg_pa_idx;
	u8 reg_pa_size_idx;
	u8 en_count;
	bool bIsATVScanMode;
	bool bStrAtvVdDet;
	bool bIsVifLock;
	bool bIsTimeOut;
	bool bForce;
	bool bIsMonitorTaskWorking;
	bool bIsPassPQ;
	bool bIsPassColorsys;
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_srccap_register_avd_subdev(
	struct v4l2_device *srccap_dev,
	struct v4l2_subdev *subdev_avd,
	struct v4l2_ctrl_handler *avd_ctrl_handler);
void mtk_srccap_unregister_avd_subdev(
	struct v4l2_subdev *subdev_avd);

int mtk_avd_eanble_clock(struct mtk_srccap_dev *srccap_dev);
int mtk_avd_disable_clock(struct mtk_srccap_dev *srccap_dev);
int mtk_avd_SetClockSource(struct mtk_srccap_dev *srccap_dev);
int mtk_avd_SetPowerState(struct mtk_srccap_dev *srccap_dev,
	EN_POWER_MODE u16PowerState);
int mtk_avd_store(struct device *dev, const char *buf);
int mtk_avd_init_customer_setting(struct mtk_srccap_dev *srccap_dev,
	struct v4l2_ext_avd_init_data *init_data);
#endif  // MTK_SRCCAP_AVD_H
