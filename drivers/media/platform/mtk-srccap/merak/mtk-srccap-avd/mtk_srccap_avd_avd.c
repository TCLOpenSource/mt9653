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
static v4l2_std_id _MappingVdStandardToV4l2std(
			enum AVD_VideoStandardType  eVdStandardType);
static enum v4l2_srccap_input_source _MappingVdSourceToV4l2Source(
			enum AVD_InputSourceType  eVdStandardType);

static v4l2_std_id _MappingVdStandardToV4l2std(
			enum AVD_VideoStandardType  eVdStandardType)
{
	v4l2_std_id std_id;

	switch (eVdStandardType) {
	case E_VIDEOSTANDARD_PAL_BGHI:
		std_id = V4L2_STD_PAL;
		break;
	case E_VIDEOSTANDARD_NTSC_M:
		std_id = V4L2_STD_NTSC;
		break;
	case E_VIDEOSTANDARD_SECAM:
		std_id = V4L2_STD_SECAM;
		break;
	case E_VIDEOSTANDARD_NTSC_44:
		std_id = V4L2_STD_NTSC_443;
		break;
	case E_VIDEOSTANDARD_PAL_M:
		std_id = V4L2_STD_PAL_M;
		break;
	case E_VIDEOSTANDARD_PAL_N:
		std_id = V4L2_STD_PAL_N;
		break;
	case E_VIDEOSTANDARD_PAL_60:
		std_id = V4L2_STD_PAL_60;
		break;
	case E_VIDEOSTANDARD_NOTSTANDARD:
		std_id = V4L2_STD_UNKNOWN;
		break;
	case E_VIDEOSTANDARD_AUTO:
		std_id = V4L2_STD_ALL;
		break;
	default:
		std_id = V4L2_STD_UNKNOWN;
		break;
	}

	return std_id;
}

static enum v4l2_srccap_input_source  _MappingVdSourceToV4l2Source(
	enum AVD_InputSourceType eVdStandardType)
{
	enum v4l2_srccap_input_source eSrc = V4L2_SRCCAP_INPUT_SOURCE_NUM;

	switch (eVdStandardType) {
	case E_INPUT_SOURCE_ATV:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_ATV;
		break;
	case E_INPUT_SOURCE_CVBS1:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
		break;
	case E_INPUT_SOURCE_CVBS2:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS2;
		break;
	case E_INPUT_SOURCE_CVBS3:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_CVBS3;
		break;
	case E_INPUT_SOURCE_SVIDEO1:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
		break;
	case E_INPUT_SOURCE_SVIDEO2:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2;
		break;
	case E_INPUT_SOURCE_SCART1:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_SCART;
		break;
	case E_INPUT_SOURCE_SCART2:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_SCART2;
		break;
	default:
		eSrc = V4L2_SRCCAP_INPUT_SOURCE_NUM;
		break;
	}
	return eSrc;
}

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
enum V4L2_AVD_RESULT mtk_srccap_avd_init(struct mtk_srccap_dev *srccap_dev)
{

	struct v4l2_ext_avd_init_data stVdInitData;
	static bool IsAVDinit = FALSE;
	memset(&stVdInitData, 0, sizeof(struct v4l2_ext_avd_init_data));

	if (IsAVDinit) {
		return V4L2_EXT_AVD_OK;
	}

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
		srccap_dev->avd_info.u64DetectTvSystem = V4L2_STD_UNKNOWN;
		srccap_dev->avd_info.u64ForceTvSystem = V4L2_STD_UNKNOWN;
		srccap_dev->avd_info.eStableTvsystem = V4L2_EXT_AVD_STD_NOTSTANDARD;
		srccap_dev->avd_info.eInputSource = V4L2_SRCCAP_INPUT_SOURCE_NONE;
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
	IsAVDinit = TRUE;

	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_InitHWinfo(struct mtk_srccap_dev *dev)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	struct AVD_HW_Info stAVDHWInfo;
	MS_U32 RegBaseAddr[4];
	MS_U32 RegBaseSize[4];
	MS_BOOL ret;
	memset(&stAVDHWInfo, 0, sizeof(struct AVD_HW_Info));

	Comb3dBufAddr = dev->avd_info.Comb3dBufAddr;
	Comb3dBufSize = dev->avd_info.Comb3dBufSize;
	Comb3dBufHeapId = dev->avd_info.Comb3dBufHeapId;

	SRCCAP_AVD_MSG_INFO("Comb3dBufAddr = 0x%llX\n", Comb3dBufAddr);
	SRCCAP_AVD_MSG_INFO("Comb3dBufSize = 0x%llX\n", Comb3dBufSize);
	SRCCAP_AVD_MSG_INFO("Comb3dBufHeapId = 0x%llX\n", Comb3dBufHeapId);

	stAVDHWInfo.u32Comb3dAliment = dev->avd_info.u32Comb3dAliment;
	stAVDHWInfo.u32Comb3dSize = dev->avd_info.u32Comb3dSize;

	SRCCAP_AVD_MSG_INFO("Comb3dAliment = %d\n", stAVDHWInfo.u32Comb3dAliment);
	SRCCAP_AVD_MSG_INFO("Comb3dSize = %d\n", stAVDHWInfo.u32Comb3dSize);


	ret = Api_AVD_SetHWInfo(stAVDHWInfo);
	SRCCAP_AVD_MSG_INFO("out\n");

	if (ret == TRUE)
		return V4L2_EXT_AVD_OK;
	else
		return V4L2_EXT_AVD_FAIL;
}

int mtk_avd_streamoff(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	kfree(srccap_dev->avd_info.pqRminfo.p.node_array);
	srccap_dev->avd_info.pqRminfo.p.node_array = NULL;
	memset(&srccap_dev->avd_info.pqRminfo, 0, sizeof(struct v4l2_srccap_pqmap_rm_info));
	return 0;
}

//VIDIOC_G_EXT_CTRLS ioctl API
enum V4L2_AVD_RESULT mtk_avd_hsync_edge(unsigned char *u8HsyncEdge)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");

#ifdef DECOMPOSE
	if (u8HsyncEdge == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	*u8HsyncEdge = Api_AVD_GetHsyncEdge();
#endif

#if (TEST_MODE) //For test
	*u8HsyncEdge = 0xDF;
#endif

	show_v4l2_ext_avd_hsync_edge(u8HsyncEdge);

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_g_flag(unsigned int *u32AVDFlag)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	if (u32AVDFlag == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	*u32AVDFlag = Api_AVD_GetFlag();
#endif

#if (TEST_MODE) //For test
	*u32AVDFlag = 0x12345678;
#endif

	show_v4l2_ext_avd_g_flag(u32AVDFlag);

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_v_total(struct v4l2_timing *ptiming)
{
#ifdef DECOMPOSE
	unsigned int u32vtotal = 0;
#endif

	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	if (ptiming == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
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

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_noise_mag(unsigned char *u8NoiseMag)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	if (u8NoiseMag == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	*u8NoiseMag  = Api_AVD_GetNoiseMag();
#endif

#if (TEST_MODE) //For test
	*u8NoiseMag = 0xFF;
#endif
	show_v4l2_ext_avd_noise_mag(u8NoiseMag);

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_is_synclocked(unsigned char *u8lock)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");

#ifdef DECOMPOSE
	if (u8lock == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	*u8lock = Api_AVD_IsSyncLocked();
#endif

#if (TEST_MODE) //For test
	*u8lock = 1;
#endif

	show_v4l2_ext_avd_is_synclocked(u8lock);

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_vertical_freq(struct v4l2_timing *ptiming)
{
#ifdef DECOMPOSE
	unsigned char u32vfreq = 0;
#endif
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	if (ptiming == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
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

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_info(struct v4l2_ext_avd_info *pinfo)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");


#ifdef DECOMPOSE
	if (pinfo == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	const struct AVD_Info *stInfo;

	stInfo = Api_AVD_GetInfo();
	pinfo->einputsourcetype = _MappingVdSourceToV4l2Source(stInfo->eVDInputSource);
	pinfo->u16CurVDStatus = stInfo->u16CurVDStatus;
	pinfo->u64LastStandard = _MappingVdStandardToV4l2std(stInfo->eLastStandard);
	pinfo->u64VideoSystem = _MappingVdStandardToV4l2std(stInfo->eVideoSystem);
	pinfo->u8AutoDetMode = stInfo->u8AutoDetMode;
	pinfo->u8AutoTuningIsProgress = stInfo->u8AutoTuningIsProgress;
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

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_setting_info(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_ext_avd_info *pinfo)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");
	enum V4L2_AVD_RESULT ret = V4L2_EXT_AVD_FAIL;
	u16 *node_array = NULL;
	u16 u16arraysize = 0;
	u8 index = 0;

	if (srccap_dev == NULL || pinfo == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return V4L2_EXT_AVD_FAIL;
	}

	if (srccap_dev->avd_info.bIsPassPQ == TRUE) {
		SRCCAP_AVD_MSG_INFO("PyPASS VDPQMAP\n");
		return V4L2_EXT_AVD_OK;
	}

	u16arraysize = pinfo->stRminfo.array_size;

	if (u16arraysize == 0) {
		SRCCAP_AVD_MSG_ERROR("Array size is 0!\n");
		return V4L2_EXT_AVD_FAIL;
	}

	node_array = kcalloc(u16arraysize, sizeof(u16), GFP_KERNEL);

	if (!node_array) {
		SRCCAP_AVD_MSG_ERROR("kcalloc failed!\n");
		return V4L2_EXT_AVD_FAIL;
	}

	if (copy_from_user((void *)node_array,
		(u16 __user *)pinfo->stRminfo.p.node_array,
		sizeof(u16) * u16arraysize)) {
		SRCCAP_AVD_MSG_ERROR("copy_from_user fail\n");
		ret = V4L2_EXT_AVD_FAIL;
		goto exit;
	}

	srccap_dev->avd_info.pqRminfo.array_size = pinfo->stRminfo.array_size;
	kfree(srccap_dev->avd_info.pqRminfo.p.node_array);

	srccap_dev->avd_info.pqRminfo.p.node_array = kcalloc(u16arraysize, sizeof(u16), GFP_KERNEL);

	if (!srccap_dev->avd_info.pqRminfo.p.node_array) {
		SRCCAP_MSG_ERROR("srccap pqRminfo node array kcalloc failed!\n");
		ret = V4L2_EXT_AVD_FAIL;
		goto exit;
	}

	memcpy(srccap_dev->avd_info.pqRminfo.p.node_array,
		node_array, sizeof(u16) * u16arraysize);

	SRCCAP_AVD_MSG_INFO("array_size:(%u)\n",
		srccap_dev->avd_info.pqRminfo.array_size);

	for (index = 0; index < u16arraysize; index++)
		SRCCAP_AVD_MSG_INFO("Node[%d]:%u\n",
		index, srccap_dev->avd_info.pqRminfo.p.node_array[index]);

	ret = mtk_srccap_common_load_vd_pqmap(srccap_dev);

	if (ret < 0) {
		SRCCAP_MSG_ERROR("srccap load vd pqmap failed!\n");
		ret = V4L2_EXT_AVD_FAIL;
		goto exit;
	}

	ret = V4L2_EXT_AVD_OK;
exit:
	kfree(node_array);

	SRCCAP_AVD_MSG_DEBUG("out\n");
	return ret;
}
enum V4L2_AVD_RESULT mtk_avd_is_lock_audio_carrier(bool *bLock)
{
	SRCCAP_AVD_MSG_DEBUG("in\n");
#ifdef DECOMPOSE
	*bLock = Api_AVD_IsLockAudioCarrier();
#endif
	show_v4l2_ext_avd_is_lock_audio_carrier(bLock);
	SRCCAP_AVD_MSG_DEBUG("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_alive_check(bool *bAlive)
{

	SRCCAP_AVD_MSG_INFO("in\n");
#ifdef DECOMPOSE
	if (bAlive == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	*bAlive = Api_AVD_AliveCheck();
#endif
	show_v4l2_ext_avd_alive_check(bAlive);
	SRCCAP_AVD_MSG_INFO("out\n");

	return V4L2_EXT_AVD_OK;
}

//VIDIOC_S_EXT_CTRLS ioctl API
enum V4L2_AVD_RESULT mtk_avd_init(struct v4l2_ext_avd_init_data pParam)
{
	SRCCAP_AVD_MSG_INFO("in\n");
	SRCCAP_AVD_MSG_INFO("\t sizeof(init)={%ld}\n", sizeof(pParam));

	pParam.u32COMB_3D_ADR = Comb3dBufAddr;
	pParam.u32COMB_3D_LEN = Comb3dBufSize;
	pParam.u32CMA_HEAP_ID = Comb3dBufHeapId;

	show_v4l2_ext_avd_init_para(&pParam);
#ifdef DECOMPOSE
	if (Api_AVD_Init((struct VD_INITDATA *)&pParam, sizeof(pParam)) == 0) {
		SRCCAP_AVD_MSG_ERROR("\t sizeof(init)={%ld}\n", sizeof(pParam));
		show_v4l2_ext_avd_init_para(&pParam);
		SRCCAP_AVD_MSG_ERROR("Fail(-1)\n");
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_exit(void)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	Api_AVD_Exit();
#endif

	SRCCAP_AVD_MSG_INFO("\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_s_flag(unsigned int *u32VDPatchFlag)
{
	SRCCAP_AVD_MSG_INFO("in\n");

 #ifdef DECOMPOSE
	if (u32VDPatchFlag == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	show_v4l2_ext_avd_s_flag(u32VDPatchFlag);
	Api_AVD_SetFlag(*u32VDPatchFlag);
 #endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_freerun_freq(
			enum v4l2_ext_avd_freerun_freq *pFreerunFreq)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	show_v4L2_ext_avd_freerun_freq(pFreerunFreq);
#ifdef DECOMPOSE
	if (pFreerunFreq == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	Api_AVD_SetFreerunFreq(*pFreerunFreq);
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_still_image_param(
			struct v4l2_ext_avd_still_image_param *pImageParam)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	show_v4l2_ext_avd_still_image_param(pImageParam);
#ifdef DECOMPOSE
	if (pImageParam == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	Api_AVD_SetStillImageParam(*(struct AVD_Still_Image_Param *)(&pImageParam));
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_factory_para(
			struct v4l2_ext_avd_factory_data *pFactoryData)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	show_v4l2_ext_avd_factory_data(pFactoryData);
#ifdef DECOMPOSE
	if (pFactoryData == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	Api_AVD_SetFactoryPara(pFactoryData->eFactoryPara,
					pFactoryData->u8Value);
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_3d_comb_speed(
			struct v4l2_ext_avd_3d_comb_speed *pspeed)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	show_v4l2_ext_avd_3d_comb_speed(pspeed);
#ifdef DECOMPOSE
	if (pspeed == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	Api_AVD_Set3dCombSpeed(pspeed->u8COMB57, pspeed->u8COMB58);
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_3d_comb(bool *bCombEanble)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	show_v4l2_ext_avd_3d_comb(bCombEanble);

#ifdef DECOMPOSE
	if (bCombEanble == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	Api_AVD_Set3dComb(*bCombEanble);
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_htt_user_md(unsigned short *u16Htt)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	show_v4l2_ext_avd_htt_user_md(u16Htt);
#ifdef DECOMPOSE
	if (u16Htt == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	Api_AVD_Set_Htt_UserMD(*u16Htt);
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_hsync_detect_detetion_for_tuning(
						bool *bTuningEanble)
{
	SRCCAP_AVD_MSG_INFO("in\n");

	show_v4l2_ext_avd_hsync_detect_detetion_for_tuning(bTuningEanble);
#ifdef DECOMPOSE
	if (bTuningEanble == NULL) {
		SRCCAP_AVD_MSG_POINTER_CHECK();
		return -EINVAL;
	}
	Api_AVD_SetHsyncDetectionForTuning(*bTuningEanble);
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_channel_change(void)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	Api_AVD_SetChannelChange();
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}
enum V4L2_AVD_RESULT mtk_avd_mcu_reset(void)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	if (Api_AVD_McuReset() == 0) {
		SRCCAP_AVD_MSG_ERROR("Fail(-1)\n");
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_dsp_reset(void)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	if (Api_AVD_DspReset() == 0) {
		SRCCAP_AVD_MSG_ERROR("Fail(-1)\n");
		return V4L2_EXT_AVD_FAIL;
	}
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

enum V4L2_AVD_RESULT mtk_avd_3d_comb_speed_up(void)
{
	SRCCAP_AVD_MSG_INFO("in\n");

#ifdef DECOMPOSE
	Api_AVD_3DCombSpeedup();
#endif

	SRCCAP_AVD_MSG_INFO("out\n");
	return V4L2_EXT_AVD_OK;
}

