/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_PQD_H
#define _MTK_PQ_PQD_H


//INFO OF enhance SUB DEV
struct st_pqd_info {
	__u8  pqd_input_source;
};

//-----------------------------------------------------------------------------
//  Function and Variable
//-----------------------------------------------------------------------------
//set ctrl
int mtk_pqd_LoadPQSetting(struct st_pqd_info *pqd_info,
	__s32 load_enable);
int mtk_pqd_SetPointToPoint(struct st_pqd_info *pqd_info,
	__s32 is_enable);
int mtk_pqd_LoadPTPTable(struct st_pqd_info *pqd_info,
	__s32 ptp_type);
int mtk_pqd_ACE_3DClonePQMap(
	struct st_pqd_info *pqd_info, __s32 weave_type);
int mtk_pqd_Set3DCloneforPIP(struct st_pqd_info *pqd_info,
	__s32 pip_enable);
int mtk_pqd_Set3DCloneforLBL(struct st_pqd_info *pqd_info,
	__s32 lbl_enable);
int mtk_pqd_SetGameMode(struct st_pqd_info *pqd_info,
	__s32 game_enable);
int mtk_pqd_SetBOBMode(struct st_pqd_info *pqd_info,
	__s32 bob_enable);
int mtk_pqd_SetBypassMode(struct st_pqd_info *pqd_info,
	__s32 bypass_enable);
int mtk_pqd_DemoCloneWindow(struct st_pqd_info *pqd_info,
	__s32 mode);
int mtk_pqd_ReduceBWforOSD(struct st_pqd_info *pqd_info,
	__s32 osd_on);
int mtk_pqd_EnableHDRMode(struct st_pqd_info *pqd_info,
	__s32 grule_level_index);
int mtk_pqd_PQExit(struct st_pqd_info *pqd_info,
	__s32 exit_enable);
int mtk_pqd_ACE_SetRBChannelRange(
	struct st_pqd_info *pqd_info, __s32 range_enable);
int mtk_pqd_SetFilmMode(struct st_pqd_info *pqd_info,
	__s32 film_mode);
int mtk_pqd_SetNRMode(struct st_pqd_info *pqd_info,
	__s32 nr_mode);
int mtk_pqd_SetMPEGNRMode(
	struct st_pqd_info *pqd_info, __s32 mpeg_nr_mode);
int mtk_pqd_SetXvyccMode(struct st_pqd_info *pqd_info,
	__s32 xvycc_mode);
int mtk_pqd_SetDLCMode(struct st_pqd_info *pqd_info,
	__s32 dlc_mode);
int mtk_pqd_SetSWDRMode(struct st_pqd_info *pqd_info,
	__s32 swdr_mode);
int mtk_pqd_SetDynamicContrastMode(
	struct st_pqd_info *pqd_info, __s32 contrast_type);
int mtk_pqd_ACE_SetContrast(struct st_pqd_info *pqd_info,
	__s32 contrast_value);
int mtk_pqd_ACE_SetBrightness(struct st_pqd_info *pqd_info,
	__s32 brightness_value);
int mtk_pqd_ACE_SetHue(struct st_pqd_info *pqd_info,
	__s32 hue_value);
int mtk_pqd_ACE_SetSaturation(struct st_pqd_info *pqd_info,
	__s32 saturation_value);
int mtk_pqd_ACE_SetSharpness(struct st_pqd_info *pqd_info,
	__s32 sharpness_value);

//set ext ctrl
int mtk_pqd_SetDbgLevel(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetPQInit(struct st_pqd_info *pqd_info, void *ctrl);
int mtk_pqd_MADIForceMotion(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetRFBLMode(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetGruleStatus(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetUCDMode(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetHSBBypass(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetVersion(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetHSYGrule(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetFBLStatus(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetHistogram(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetIPHistogram(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetNRSettings(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetDPUMode(struct st_pqd_info *pqd_info, void *ctrl);
int mtk_pqd_ACE_SetCCMInfo(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_SetColorTuner(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_SetHSYSetting(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_SetHSYRange(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_SetManualLumaCurve(
	struct st_pqd_info *pqd_info, void *ctrl);
int mtk_pqd_ACE_SetStretchSettings(
	struct st_pqd_info *pqd_info, void *ctrl);
int mtk_pqd_ACE_SetPQCmd(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_SetPostColorTemp(
	struct st_pqd_info *pqd_info, void *ctrl);
int mtk_pqd_ACE_SetMWE(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetSWDRInfo(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GenRGBPattern(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetBlueStretch(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_Set_H_NonlinearScaling(
	struct st_pqd_info *pqd_info, void *ctrl);
int mtk_pqd_SetMCDIMode(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetChromaInfo(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetPQGamma(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetVideoDelayTime(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_DLCInit(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_SetDLCCaptureRange(
	struct st_pqd_info *pqd_info, void *ctrl);
int mtk_pqd_SetPQMapInfo(struct st_pqd_info *pqd_info, void *ctrl);

//get ctrl
int mtk_pqd_GetPointToPoint(
	struct st_pqd_info *pqd_info, __s32 *ptp_enable);
int mtk_pqd_GetGameModeStatus(
	struct st_pqd_info *pqd_info, __s32 *game_enable);
int mtk_pqd_GetDualViewState(
	struct st_pqd_info *pqd_info, __s32 *dual_view_state);
int mtk_pqd_GetZeroFrameMode(struct st_pqd_info *pqd_info,
	__s32 *zero_frame_mode);

//get ext ctrl
int mtk_pqd_GetRFBLMode(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetDPUStatus(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ReadPQCmd(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetSWDRInfo(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetVideoDelayTime(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetPQCaps(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetPQVersion(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetHSYGrule(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetFBLStatus(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_GetIPHistogram(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_GetHSYSetting(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_GetHSYRange(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_GetLumaInfo(struct st_pqd_info *pqd_info,
	void *ctrl);
int mtk_pqd_ACE_GetChromaInfo(struct st_pqd_info *pqd_info,
	void *ctrl);

//functions
int mtk_pqd_Open(struct st_pqd_info *pqd_info);
int mtk_pqd_Close(struct st_pqd_info *pqd_info);
int mtk_pqd_SetInputSource(struct st_pqd_info *pqd_info,
	__u8 u8Input);
int mtk_pqd_SetPixFmtIn(struct st_pqd_info *pqd_info,
	struct v4l2_pix_format_mplane *pix);
int mtk_pqd_SetPixFmtOut(struct st_pqd_info *pqd_info,
	struct v4l2_pix_format_mplane *pix);
int mtk_pqd_StreamOn(struct st_pqd_info *pqd_info);
int mtk_pqd_StreamOff(struct st_pqd_info *pqd_info);


#endif

