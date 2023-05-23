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
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include <linux/mtk-v4l2-srccap.h>
#include "sti_msos.h"
#include "mtk_srccap_avd_avd.h"
#include "mtk_srccap.h"
#include "show_param.h"
#include "avd-ex.h"

/* ============================================================================================== */
/* -------------------------------------- Global Type Definitions ------------------------------- */
/* ============================================================================================== */
static MS_PHY Comb3dBufAddr;
static MS_U32 Comb3dBufSize;
static MS_U32 Comb3dBufHeapId;
/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
enum V4L2_AVD_RESULT mtk_srccap_avd_init(struct mtk_srccap_dev *srccap_dev)
{
	struct v4l2_ext_avd_init_data stVdInitData;

	memset(&stVdInitData, 0, sizeof(struct v4l2_ext_avd_init_data));

	if (srccap_dev != NULL) {
		srccap_dev->avd_info.Comb3dBufAddr = 0;
		srccap_dev->avd_info.Comb3dBufHeapId = 0;
		srccap_dev->avd_info.Comb3dBufSize = 0;
		srccap_dev->avd_info.u32Comb3dAliment = 0;
		srccap_dev->avd_info.u32Comb3dSize = 0;
		srccap_dev->avd_info.enVdSignalStatus = 0; //NO SYNC
		srccap_dev->avd_info.stVDtiming.interlance = 0;
		srccap_dev->avd_info.stVDtiming.de_width = 0;
		srccap_dev->avd_info.stVDtiming.de_height = 0;
		srccap_dev->avd_info.stVDtiming.v_freq = 0;
	} else
		return -ENOMEM;

	stVdInitData.u32XTAL_Clock = 12000000UL;
	stVdInitData.eDemodType = V4L2_EXT_AVD_DEMODE_MSTAR_VIF;
	stVdInitData.eLoadCodeType = V4L2_EXT_AVD_LOAD_CODE_BYTE_WRITE;
	stVdInitData.u32VD_DSP_Code_Address = 0;
	stVdInitData.pu8VD_DSP_Code_Address = 0;
	stVdInitData.u32VD_DSP_Code_Len = 0;
	stVdInitData.u16VDDSPBinID = 0xFFFF;
	stVdInitData.bRFGainSel = 1;
	stVdInitData.u8RFGain = 0x80;
	stVdInitData.bAVGainSel = 1;
	stVdInitData.u8AVGain = 0x80;
	stVdInitData.u8SwingThreshold =  2;
	stVdInitData.u8VdDecInitializeExt = NULL;
	stVdInitData.u32COMB_3D_ADR = 0;
	stVdInitData.u32COMB_3D_LEN  = 0;
	stVdInitData.u32CMA_HEAP_ID = 0;

	mtk_avd_init(stVdInitData);

	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_InitHWinfo(struct mtk_srccap_dev *dev)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	struct AVD_HW_Info stAVDHWInfo;
	MS_U32 RegBaseAddr[4];
	MS_U32 RegBaseSize[4];
	MS_BOOL ret;

	dev->avd_info.pu8RiuBaseAddr = ioremap(0x1F000000, 0x400000);
	pr_info("[SRCCAP][%s][%d] : virtAVDBaseAddr = %lx\r\n",
		__func__, __LINE__, dev->avd_info.pu8RiuBaseAddr);

	dev->avd_info.u32DramBaseAddr = 0x21000000;
	pr_info("[SRCCAP][%s][%d] : dram_base_addr = %lx\r\n",
		__func__, __LINE__, dev->avd_info.u32DramBaseAddr);

	Comb3dBufAddr = dev->avd_info.Comb3dBufAddr;
	Comb3dBufSize = dev->avd_info.Comb3dBufSize;
	Comb3dBufHeapId = dev->avd_info.Comb3dBufHeapId;

	pr_info("[SRCCAP][%s][%d] : Comb3dBufAddr = %lx\r\n",
		__func__, __LINE__, Comb3dBufAddr);
	pr_info("[SRCCAP][%s][%d] : Comb3dBufSize = %lx\r\n",
			__func__, __LINE__, Comb3dBufSize);
	pr_info("[SRCCAP][%s][%d] : Comb3dBufHeapId = %lx\r\n",
			__func__, __LINE__, Comb3dBufHeapId);

	stAVDHWInfo.pu8RiuBaseAddr = dev->avd_info.pu8RiuBaseAddr;
	stAVDHWInfo.u32DramBaseAddr = dev->avd_info.u32DramBaseAddr;
	stAVDHWInfo.u32Comb3dAliment = dev->avd_info.u32Comb3dAliment;
	stAVDHWInfo.u32Comb3dSize = dev->avd_info.u32Comb3dSize;

	ret = Api_AVD_SetHWInfo(stAVDHWInfo);
	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);

	if (ret == TRUE)
		return V4L2_EXT_AVD_OK;
	else
		return V4L2_EXT_AVD_FAIL;
}

//VIDIOC_G_EXT_CTRLS ioctl API
enum V4L2_AVD_RESULT mtk_avd_hsync_edge(unsigned char *u8HsyncEdge)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	*u8HsyncEdge = Api_AVD_GetHsyncEdge();
#endif

#if (TEST_MODE) //For test
	*u8HsyncEdge = 0xDF;
#endif

	show_v4l2_ext_avd_hsync_edge(u8HsyncEdge);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_g_flag(unsigned int *u32AVDFlag)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	*u32AVDFlag = Api_AVD_GetFlag();
#endif

#if (TEST_MODE) //For test
	*u32AVDFlag = 0x12345678;
#endif

	show_v4l2_ext_avd_g_flag(u32AVDFlag);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_v_total(struct v4l2_timing *ptiming)
{
#ifdef DECOMPOSE
	unsigned int u32vtotal = 0;
#endif

	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	u32vtotal = Api_AVD_GetVTotal();
	ptiming->h_freq = 0;
	ptiming->h_total = 0;
	ptiming->v_freq = 0;
	ptiming->v_total = u32vtotal*1000;
#endif

#if (TEST_MODE) //For test
	ptiming->h_freq = 0;
	ptiming->h_total = 0;
	ptiming->v_freq = 0;
	ptiming->v_total = 5000;
#endif

	show_v4l2_ext_avd_timing(ptiming);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_noise_mag(unsigned char *u8NoiseMag)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	*u8NoiseMag  = Api_AVD_GetNoiseMag();
#endif

#if (TEST_MODE) //For test
	*u8NoiseMag = 0xFF;
#endif
	show_v4l2_ext_avd_noise_mag(u8NoiseMag);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_is_synclocked(unsigned char *u8lock)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	*u8lock = Api_AVD_IsSyncLocked();
#endif

#if (TEST_MODE) //For test
	*u8lock = 1;
#endif

	show_v4l2_ext_avd_is_synclocked(u8lock);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_scan_hsync_check(
			struct v4l2_ext_avd_scan_hsyc_check *pHsync)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	pHsync->u16ScanHsyncCheck =
	Api_AVD_Scan_HsyncCheck(pHsync->u8HtotalTolerance);
#endif

#if (TEST_MODE) //For test
	pHsync->u16ScanHsyncCheck = 0xE443;
#endif

	show_v4l2_ext_avd_scan_hsyc_check(pHsync);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_vertical_freq(struct v4l2_timing *ptiming)
{
#ifdef DECOMPOSE
	unsigned char u32vfreq = 0;
#endif
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	u32vfreq = Api_AVD_GetVerticalFreq();
	ptiming->h_freq = 0;
	ptiming->h_total = 0;
	ptiming->v_freq = u32vfreq*1000;
	ptiming->v_total = 0;
#endif

#if (TEST_MODE) //For test
	ptiming->h_freq = 0;
	ptiming->h_total = 0;
	ptiming->v_freq = 60000;
	ptiming->v_total = 0;
#endif

	show_v4l2_ext_avd_timing(ptiming);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_info(struct v4l2_ext_avd_info *pinfo)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	pinfo = (struct v4l2_ext_avd_info *)Api_AVD_GetInfo();
#endif

#if (TEST_MODE) //For test
	pinfo->einputsourcetype = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
	pinfo->u16CurVDStatus = 0xE443;
	pinfo->u64LastStandard = V4L2_STD_NTSC;
	pinfo->u64VideoSystem = V4L2_STD_NTSC;
	pinfo->u8AutoDetMode = 0;
	pinfo->u8AutoTuningIsProgress = 0;
#endif

	show_v4l2_ext_avd_info(pinfo);

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_is_lock_audio_carrier(bool *bLock)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);
#ifdef DECOMPOSE
	*bLock = Api_AVD_IsLockAudioCarrier();
#endif
	show_v4l2_ext_avd_is_lock_audio_carrier(bLock);
	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_alive_check(bool *bAlive)
{

	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);
#ifdef DECOMPOSE
	*bAlive = Api_AVD_AliveCheck();
#endif
	show_v4l2_ext_avd_alive_check(bAlive);
	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);

	return V4L2_EXT_AVD_OK;
}

//VIDIOC_S_EXT_CTRLS ioctl API
enum V4L2_AVD_RESULT mtk_avd_init(struct v4l2_ext_avd_init_data pParam)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);
	AVD_PAM_MSG("\t sizeof(init)={%ld}\n", sizeof(pParam));

	pParam.u32COMB_3D_ADR = Comb3dBufAddr;
	pParam.u32COMB_3D_LEN = Comb3dBufSize;
	pParam.u32CMA_HEAP_ID = Comb3dBufHeapId;

	show_v4l2_ext_avd_init_para(&pParam);
#ifdef DECOMPOSE

	if (Api_AVD_Init((struct VD_INITDATA *)&pParam, sizeof(pParam)) == 0) {
		AVD_ERR_MSG("[%s][%d]Fail\n", __func__, __LINE__);
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_exit(void)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	Api_AVD_Exit();
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_s_flag(unsigned int *u32VDPatchFlag)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

 #ifdef DECOMPOSE
	show_v4l2_ext_avd_s_flag(u32VDPatchFlag);
	Api_AVD_SetFlag(*u32VDPatchFlag);
 #endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_freerun_freq(
			enum v4l2_ext_avd_freerun_freq *pFreerunFreq)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	show_v4L2_ext_avd_freerun_freq(pFreerunFreq);
#ifdef DECOMPOSE
	Api_AVD_SetFreerunFreq(*pFreerunFreq);
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_still_image_param(
			struct v4l2_ext_avd_still_image_param *pImageParam)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	show_v4l2_ext_avd_still_image_param(pImageParam);
#ifdef DECOMPOSE
	Api_AVD_SetStillImageParam(*(struct AVD_Still_Image_Param *)(&pImageParam));
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_factory_para(
			struct v4l2_ext_avd_factory_data *pFactoryData)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	show_v4l2_ext_avd_factory_data(pFactoryData);
#ifdef DECOMPOSE
	Api_AVD_SetFactoryPara(pFactoryData->eFactoryPara,
					pFactoryData->u8Value);
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_3d_comb_speed(
			struct v4l2_ext_avd_3d_comb_speed *pspeed)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	show_v4l2_ext_avd_3d_comb_speed(pspeed);
#ifdef DECOMPOSE
	Api_AVD_Set3dCombSpeed(pspeed->u8COMB57, pspeed->u8COMB58);
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_3d_comb(bool *bCombEanble)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	show_v4l2_ext_avd_3d_comb(bCombEanble);

#ifdef DECOMPOSE
	Api_AVD_Set3dComb(*bCombEanble);
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_htt_user_md(unsigned short *u16Htt)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	show_v4l2_ext_avd_htt_user_md(u16Htt);
#ifdef DECOMPOSE
	Api_AVD_Set_Htt_UserMD(*u16Htt);
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_hsync_detect_detetion_for_tuning(
						bool *bTuningEanble)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

	show_v4l2_ext_avd_hsync_detect_detetion_for_tuning(bTuningEanble);
#ifdef DECOMPOSE
	Api_AVD_SetHsyncDetectionForTuning(*bTuningEanble);
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_channel_change(void)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	Api_AVD_SetChannelChange();
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_mcu_reset(void)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	if (Api_AVD_McuReset() == 0) {
		AVD_ERR_MSG("[%s][%d]Fail\n", __func__, __LINE__);
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_3d_comb_speed_up(void)
{
	AVD_API_MSG("[%s][%d]in\n", __func__, __LINE__);

#ifdef DECOMPOSE
	Api_AVD_3DCombSpeedup();
#endif

	AVD_API_MSG("[%s][%d]out\n", __func__, __LINE__);
	return V4L2_EXT_AVD_OK;
}
