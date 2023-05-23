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

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "mtk_pq.h"
#include "mtk_pq_xc.h"
#include "mtk_pq_buffer.h"
#include "mtk_pq_svp.h"

#include "apiXC.h"
#include "drvPQ.h"
#include "mtk_apiACE.h"

#define PQ_DEV_NUM 2
/*main and sub,multi win shall enlarge it, TODO: this should put in DTS*/
#define FIELD_TO_INTERLACE(x) \
	(((x) == V4L2_FIELD_ANY || (x) == V4L2_FIELD_NONE) ? FALSE : TRUE)

static struct v4l2_ext_ctrl_ds_info _g_ds_info[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_rwbankauto_enable_info _g_rwba[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_scaling_enable_info _g_scaling[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_cap_win_info _g_cap_win[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_crop_win_info _g_crop_win[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_disp_win_info _g_disp_win[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_scaling_info _g_pre_scaling[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_scaling_info _g_post_scaling[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_status_info _g_status[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_caps_info _g_caps[PQ_DEV_NUM];
static struct v4l2_ext_ctrl_virtualbox_info _g_vb[PQ_DEV_NUM];


static int _mtk_xc_read_dts(struct v4l2_xc_info *xc_info)
{
	int ret;
	struct device_node *np;
	__u32 u32Tmp = 0;
	__u8 tmp_u8 = 0;
	const char *name;

	np = of_find_compatible_node(NULL, NULL, "mediatek,pq-xc");

	if (np != NULL) {
		// read window num
		name = "WINDOW_NUM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->window_num = u32Tmp;

		// read crystal clock
		name = "CRYSTAL_CLOCK";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->XTAL_Clock = u32Tmp;

		// read is share ground
		name = "IS_SHARE_GROUND";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->IsShareGround = u32Tmp;

		// read main frame buffer start address
		name = "MAIN_FB_START_ADDR";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Main_FB_Start_Addr = u32Tmp;

		// read main frame buffer start address
		name = "MAIN_FB_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Main_FB_Size = u32Tmp;

		// read sub frame buffer start address
		name = "SUB_FB_START_ADDR";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Sub_FB_Start_Addr = u32Tmp;

		// read sub frame buffer start address
		name = "SUB_FB_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Sub_FB_Size = u32Tmp;

		// read FRCM main frame buffer start address
		name = "MAIN_FRCM_FB_START_ADDR";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Main_FRCM_FB_Start_Addr = u32Tmp;

		// read FRCM main frame buffer start address
		name = "MAIN_FRCM_FB_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Main_FRCM_FB_Size = u32Tmp;

		// read FRCM sub frame buffer start address
		name = "SUB_FRCM_FB_START_ADDR";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Sub_FRCM_FB_Start_Addr = u32Tmp;

		// read FRCM sub frame buffer start address
		name = "SUB_FRCM_FB_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Sub_FRCM_FB_Size = u32Tmp;

		// read dual frame buffer start address
		name = "DUAL_FB_START_ADDR";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Dual_FB_Start_Addr = u32Tmp;

		// read dual frame buffer start address
		name = "DUAL_FB_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Dual_FB_Size = u32Tmp;

		// read auto download start addr
		name = "AUTO_DOWNLOAD_START_ADDR";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Auto_Download_Start_Addr = u32Tmp;

		// read auto download size
		name = "AUTO_DOWNLOAD_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Auto_Download_Size = u32Tmp;

		// read auto download xvycc size
		name = "AUTO_DOWNLOAD_XVYCC_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->Auto_Download_XVYCC_Size = u32Tmp;

		// read pq buffer tag
		name = "PQ_IOMMU_IDX_SCMI";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->pq_iommu_idx_scmi = u32Tmp;

		// read pq buffer tag
		name = "PQ_IOMMU_IDX_ZNR";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->pq_iommu_idx_znr = u32Tmp;

		// read pq buffer tag
		name = "PQ_IOMMU_IDX_UCM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->pq_iommu_idx_ucm = u32Tmp;

		// read pq buffer tag
		name = "PQ_IOMMU_IDX_ABF";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		xc_info->pq_iommu_idx_abf = u32Tmp;

		// init pq buffer table
		xc_info->pBufferTable = NULL;

		// read mdw v align
		name = "mdw_v_align";
		ret = of_property_read_u8(np, name, &tmp_u8);
		if (ret != 0x0)
			goto Fail;
		xc_info->mdw.v_align = tmp_u8;

		// read mdw h align
		name = "mdw_h_align";
		ret = of_property_read_u8(np, name, &tmp_u8);
		if (ret != 0x0)
			goto Fail;
		xc_info->mdw.h_align = tmp_u8;
	}

	return 0;

Fail:
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"[%s, %d]: read DTS failed, name = %s\n",
		__func__, __LINE__, name);
	return ret;
}

// todo: this function should put in script DD
static int _mtk_xc_config_autodownload(struct v4l2_xc_info *xc_info)
{
	XC_AUTODOWNLOAD_CONFIG_INFO stAdlCfg;

	memset(&stAdlCfg, 0, sizeof(XC_AUTODOWNLOAD_CONFIG_INFO));
	stAdlCfg.u32ConfigInfo_Version = AUTODOWNLOAD_CONFIG_INFO_VERSION;
	stAdlCfg.u16ConfigInfo_Length = sizeof(XC_AUTODOWNLOAD_CONFIG_INFO);
	stAdlCfg.enClient = E_XC_AUTODOWNLOAD_CLIENT_HDR;
	stAdlCfg.phyBaseAddr =  xc_info->Auto_Download_Start_Addr;
	stAdlCfg.u32Size = xc_info->Auto_Download_Size -
		xc_info->Auto_Download_XVYCC_Size;
	stAdlCfg.bEnable = TRUE;
	stAdlCfg.enMode = E_XC_AUTODOWNLOAD_TRIGGER_MODE;
	if (MApi_XC_AutoDownload_Config(&stAdlCfg) != TRUE)
		return -EPERM;

	// note: this can be remove when move to decompose stage
	memset(&stAdlCfg, 0, sizeof(XC_AUTODOWNLOAD_CONFIG_INFO));
	stAdlCfg.u32ConfigInfo_Version = AUTODOWNLOAD_CONFIG_INFO_VERSION;
	stAdlCfg.u16ConfigInfo_Length = sizeof(XC_AUTODOWNLOAD_CONFIG_INFO);
	stAdlCfg.enClient = E_XC_AUTODOWNLOAD_CLIENT_XVYCC;
	stAdlCfg.phyBaseAddr = xc_info->Auto_Download_Start_Addr +
		xc_info->Auto_Download_Size -
		xc_info->Auto_Download_XVYCC_Size;
	stAdlCfg.u32Size = xc_info->Auto_Download_XVYCC_Size;
	stAdlCfg.bEnable = TRUE;
	stAdlCfg.enMode = E_XC_AUTODOWNLOAD_TRIGGER_MODE;
	if (MApi_XC_AutoDownload_Config(&stAdlCfg) != TRUE)
		return -EPERM;

	return 0;
}

static int _mtk_3d_input_mode_sti2utopia(
	enum v4l2_ext_3d_input_mode en3DInputModeSti,
	E_XC_3D_INPUT_MODE *pen3dInputMode)
{
	int ret = 0;

	if (pen3dInputMode == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		ret = -EPERM;
	}

	switch (en3DInputModeSti)	{
	case V4L2_EXT_3D_INPUT_FRAME_PACKING:
		*pen3dInputMode = E_XC_3D_INPUT_FRAME_PACKING;
		break;
	case V4L2_EXT_3D_INPUT_FIELD_ALTERNATIVE:
		*pen3dInputMode = E_XC_3D_INPUT_FIELD_ALTERNATIVE;
		break;
	case V4L2_EXT_3D_INPUT_LINE_ALTERNATIVE:
		*pen3dInputMode = E_XC_3D_INPUT_LINE_ALTERNATIVE;
		break;
	case V4L2_EXT_3D_INPUT_L_DEPTH:
		*pen3dInputMode = E_XC_3D_INPUT_L_DEPTH;
		break;
	case V4L2_EXT_3D_INPUT_L_DEPTH_GRAPHICS_GRAPHICS_DEPTH:
		*pen3dInputMode =
			E_XC_3D_INPUT_L_DEPTH_GRAPHICS_GRAPHICS_DEPTH;
		break;
	case V4L2_EXT_3D_INPUT_TOP_BOTTOM:
		*pen3dInputMode = E_XC_3D_INPUT_TOP_BOTTOM;
		break;
	case V4L2_EXT_3D_INPUT_SIDE_BY_SIDE_HALF:
		*pen3dInputMode = E_XC_3D_INPUT_SIDE_BY_SIDE_HALF;
		break;
	case V4L2_EXT_3D_INPUT_CHECK_BOARD:
		*pen3dInputMode = E_XC_3D_INPUT_CHECK_BORAD;
		break;
	case V4L2_EXT_3D_INPUT_PIXEL_ALTERNATIVE:
		*pen3dInputMode = E_XC_3D_INPUT_PIXEL_ALTERNATIVE;
		break;
	case V4L2_EXT_3D_INPUT_FRAME_ALTERNATIVE:
		*pen3dInputMode = E_XC_3D_INPUT_FRAME_ALTERNATIVE;
		break;
	case V4L2_EXT_3D_INPUT_NORMAL_2D:
		*pen3dInputMode = E_XC_3D_INPUT_NORMAL_2D;
		break;
	case V4L2_EXT_3D_INPUT_MODE_NONE:
		*pen3dInputMode = E_XC_3D_INPUT_MODE_NONE;
		break;
	default:
		*pen3dInputMode = E_XC_3D_INPUT_MODE_NONE;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"3D input mode not matched.\n");
		break;
	}

	return ret;
}

static int _mtk_3d_output_mode_sti2utopia(
	enum v4l2_ext_3d_output_mode en3DOutputModeSti,
	E_XC_3D_OUTPUT_MODE *pen3dOutputMode)
{
	int ret = 0;

	if (en3DOutputModeSti == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		ret = -EPERM;
	}

	switch (en3DOutputModeSti) {
	case V4L2_EXT_3D_OUTPUT_LINE_ALTERNATIVE:
		*pen3dOutputMode = E_XC_3D_OUTPUT_LINE_ALTERNATIVE;
		break;
	case V4L2_EXT_3D_OUTPUT_TOP_BOTTOM:
		*pen3dOutputMode = E_XC_3D_OUTPUT_TOP_BOTTOM;
		break;
	case V4L2_EXT_3D_OUTPUT_SIDE_BY_SIDE_HALF:
		*pen3dOutputMode = E_XC_3D_OUTPUT_SIDE_BY_SIDE_HALF;
		break;
	case V4L2_EXT_3D_OUTPUT_FRAME_ALTERNATIVE:
		*pen3dOutputMode = E_XC_3D_OUTPUT_FRAME_ALTERNATIVE;
		break;
	case V4L2_EXT_3D_OUTPUT_FRAME_L:
		*pen3dOutputMode = E_XC_3D_OUTPUT_FRAME_L;
		break;
	case V4L2_EXT_3D_OUTPUT_FRAME_R:
		*pen3dOutputMode = E_XC_3D_OUTPUT_FRAME_R;
		break;
	case V4L2_EXT_3D_OUTPUT_FRAME_PACKING:
		*pen3dOutputMode = E_XC_3D_OUTPUT_FRAME_PACKING;
		break;
	case V4L2_EXT_3D_OUTPUT_FRAME_ALTERNATIVE_LLRR:
		*pen3dOutputMode = E_XC_3D_OUTPUT_FRAME_ALTERNATIVE_LLRR;
		break;
	case V4L2_EXT_3D_OUTPUT_MODE_NONE:
		*pen3dOutputMode = E_XC_3D_OUTPUT_MODE_NONE;
		break;
	default:
		*pen3dOutputMode = E_XC_3D_OUTPUT_MODE_NONE;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"3D output mode not matched.\n");
		break;
	}

	return ret;
}

static int _mtk_3d_input_mode_utopia2sti(
	enum v4l2_ext_3d_input_mode *pen3DInputModeSti,
	E_XC_3D_INPUT_MODE en3dInputMode)
{

	if (pen3DInputModeSti == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		return -EPERM;
	}

	switch (en3dInputMode) {
	case E_XC_3D_INPUT_FRAME_PACKING:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_FRAME_PACKING;
		break;
	case E_XC_3D_INPUT_FIELD_ALTERNATIVE:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_FIELD_ALTERNATIVE;
		break;
	case E_XC_3D_INPUT_LINE_ALTERNATIVE:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_LINE_ALTERNATIVE;
		break;
	case E_XC_3D_INPUT_L_DEPTH:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_L_DEPTH;
		break;
	case E_XC_3D_INPUT_L_DEPTH_GRAPHICS_GRAPHICS_DEPTH:
		*pen3DInputModeSti =
			V4L2_EXT_3D_INPUT_L_DEPTH_GRAPHICS_GRAPHICS_DEPTH;
		break;
	case E_XC_3D_INPUT_TOP_BOTTOM:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_TOP_BOTTOM;
		break;
	case E_XC_3D_INPUT_SIDE_BY_SIDE_HALF:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_SIDE_BY_SIDE_HALF;
		break;
	case E_XC_3D_INPUT_CHECK_BORAD:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_CHECK_BOARD;
		break;
	case E_XC_3D_INPUT_PIXEL_ALTERNATIVE:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_PIXEL_ALTERNATIVE;
		break;
	case E_XC_3D_INPUT_FRAME_ALTERNATIVE:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_FRAME_ALTERNATIVE;
		break;
	case E_XC_3D_INPUT_NORMAL_2D:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_NORMAL_2D;
		break;
	case E_XC_3D_INPUT_MODE_NONE:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_MODE_NONE;
		break;
	default:
		*pen3DInputModeSti = V4L2_EXT_3D_INPUT_MODE_NONE;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"3D input mode not matched.\n");
		break;
	}

	return 0;
}

static int _mtk_3d_output_mode_utopia2sti(
	enum v4l2_ext_3d_output_mode *pen3DOutputModeSti,
	E_XC_3D_OUTPUT_MODE en3dOutputMode)
{
	int ret = 0;

	if (en3dOutputMode == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		ret = -EPERM;
	}

	switch (en3dOutputMode) {
	case E_XC_3D_OUTPUT_LINE_ALTERNATIVE:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_LINE_ALTERNATIVE;
		break;
	case E_XC_3D_OUTPUT_TOP_BOTTOM:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_TOP_BOTTOM;
		break;
	case E_XC_3D_OUTPUT_SIDE_BY_SIDE_HALF:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_SIDE_BY_SIDE_HALF;
		break;
	case E_XC_3D_OUTPUT_FRAME_ALTERNATIVE:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_FRAME_ALTERNATIVE;
		break;
	case E_XC_3D_OUTPUT_FRAME_L:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_FRAME_L;
		break;
	case E_XC_3D_OUTPUT_FRAME_R:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_FRAME_R;
		break;
	case E_XC_3D_OUTPUT_FRAME_PACKING:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_FRAME_PACKING;
		break;
	case E_XC_3D_OUTPUT_FRAME_ALTERNATIVE_LLRR:
		*pen3DOutputModeSti =
			V4L2_EXT_3D_OUTPUT_FRAME_ALTERNATIVE_LLRR;
		break;
	case E_XC_3D_OUTPUT_MODE_NONE:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_MODE_NONE;
		break;
	default:
		*pen3DOutputModeSti = V4L2_EXT_3D_OUTPUT_MODE_NONE;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"3D output mode not matched.\n");
		break;
	}

	return ret;
}


static int _mtk_3d_set_mode(void *ctrl)
{
	int ret = 0;
	MS_BOOL bRet = FALSE;
	E_XC_3D_INPUT_MODE en3dInputMode = E_XC_3D_INPUT_MODE_NONE;
	E_XC_3D_OUTPUT_MODE en3dOutputMode = E_XC_3D_OUTPUT_MODE_NONE;
	E_XC_3D_PANEL_TYPE e3dPanelType = E_XC_3D_PANEL_NONE;
	struct v4l2_ext_ctrl_3d_mode set_3d_ext_info;

	memset(&set_3d_ext_info, 0, sizeof(struct v4l2_ext_ctrl_3d_mode));
	memcpy(&set_3d_ext_info, ctrl, sizeof(struct v4l2_ext_ctrl_3d_mode));

	ret = _mtk_3d_input_mode_sti2utopia(
		set_3d_ext_info.e3DInputMode, &en3dInputMode);
	if (ret != 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"%s convert input error.\n", __func__);
	}

	ret = _mtk_3d_output_mode_sti2utopia(
		set_3d_ext_info.e3DOutputMode, &en3dOutputMode);
	if (ret != 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"%s convert output error.\n", __func__);
	}

	bRet = MApi_XC_Set_3D_Mode(en3dInputMode, en3dOutputMode,
		e3dPanelType, MAIN_WINDOW);

	//FIXME for test
	bRet = TRUE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"set e3DInputMode=%d e3DOutputMode=%d\n",
		set_3d_ext_info.e3DInputMode, set_3d_ext_info.e3DOutputMode);

	ret = (bRet == TRUE) ? 0 : (-1);

	return ret;
}

static int _mtk_3d_get_mode(void *ctrl)
{
	int ret = 0;
	struct v4l2_ext_ctrl_3d_mode get_3d_ext_info;
	E_XC_3D_INPUT_MODE en3dInputMode = E_XC_3D_INPUT_MODE_NONE;
	E_XC_3D_OUTPUT_MODE en3dOutputMode = E_XC_3D_OUTPUT_MODE_NONE;

	memset(&get_3d_ext_info, 0, sizeof(struct v4l2_ext_ctrl_3d_mode));

	if (ctrl == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		ret = -EPERM;
	}
	// ....

	en3dInputMode = MApi_XC_Get_3D_Input_Mode(MAIN_WINDOW);
	en3dOutputMode = MApi_XC_Get_3D_Output_Mode();

	get_3d_ext_info.e3DInputMode =
		(enum v4l2_ext_3d_input_mode)en3dInputMode;
	get_3d_ext_info.e3DOutputMode =
		(enum v4l2_ext_3d_output_mode)en3dOutputMode;

	//FIXME for test
	get_3d_ext_info.e3DInputMode = V4L2_EXT_3D_INPUT_FIELD_ALTERNATIVE;
	get_3d_ext_info.e3DOutputMode = V4L2_EXT_3D_OUTPUT_LINE_ALTERNATIVE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"get e3DInputMode=%d e3DOutputMode=%d\n",
		get_3d_ext_info.e3DInputMode, get_3d_ext_info.e3DOutputMode);

	memcpy(ctrl, &get_3d_ext_info, sizeof(struct v4l2_ext_ctrl_3d_mode));
	return ret;
}

static int _mtk_3d_get_hw_version(__s32 *pValue)
{
	int ret = 0;
	MS_U16 u16HW3DVersion = 0;

	if (pValue == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		ret = -EPERM;
	}

	u16HW3DVersion = MApi_XC_Get_3D_HW_Version();

	//FIXME for test
	u16HW3DVersion = 1;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"get 3D HW version = %d\n", u16HW3DVersion);
	*pValue = (__s32)u16HW3DVersion;

	return ret;
}

static int _mtk_3d_get_lr_frame_exchged(__s32 *pValue)
{
	int ret = 0;
	MS_BOOL bLRFrameExchged = FALSE;

	if (pValue == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		ret = -EPERM;
	}

	bLRFrameExchged = MApi_XC_3D_Is_LR_Frame_Exchged(MAIN_WINDOW);

	//FIXME for test
	bLRFrameExchged = TRUE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"get 3D bLRFrameExchged = %d\n", bLRFrameExchged);
	*pValue = (__s32)bLRFrameExchged;

	return ret;
}

static int _mtk_3d_get_hshift(__s32 *pValue)
{
	int ret = 0;
	MS_U16 u16HShift3D = 0;

	if (pValue == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "INVALID PARA.\n");
		ret = -EPERM;
	}

	u16HShift3D = MApi_XC_Get_3D_HShift();

	//FIXME for test
	u16HShift3D = 2;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "get 3D hshift = %d\n", u16HShift3D);
	*pValue = (__s32)u16HShift3D;

	return ret;
}

static int _mtk_3d_set_hshift(__s32 s32Value)
{
	MS_BOOL bRet = FALSE;

	bRet = MApi_XC_Set_3D_HShift((MS_U16)s32Value);

	//FIXME for test
	bRet = TRUE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"set 3D hshift = %d bRet = %d\n", s32Value, bRet);

	return (bRet == TRUE) ? 0 : (-1);
}

static int _mtk_3d_set_lr_frame_exchged(__s32 s32Value)
{
	MS_BOOL bRet = FALSE;

	bRet = MApi_XC_Set_3D_LR_Frame_Exchg(MAIN_WINDOW);

	//FIXME for test
	bRet = TRUE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"set 3D lr frame exchged = %d bRet = %d\n", s32Value, bRet);

	return (bRet == TRUE) ? 0 : (-1);
}

static int _mtk_xc_set_init(struct v4l2_xc_info *xc_info)
{
	XC_INITDATA xc_init_data;

	memset(&xc_init_data, 0, sizeof(XC_INITDATA));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc init\n");

	// init pnl reg, should be remove in decompose stage
	MApi_PNL_IOMapBaseInit();

	xc_init_data.u32XC_version = XC_INITDATA_VERSION;
	xc_init_data.u32XTAL_Clock = xc_info->XTAL_Clock;
	xc_init_data.u32Main_FB_Start_Addr = xc_info->Main_FB_Start_Addr;
	xc_init_data.u32Main_FB_Size = xc_info->Main_FB_Size;
	xc_init_data.u32Sub_FB_Start_Addr = xc_info->Sub_FB_Start_Addr;
	xc_init_data.u32Sub_FB_Size = xc_info->Sub_FB_Size;
	xc_init_data.bCEC_Use_Interrupt = 0;
	xc_init_data.bIsShareGround = xc_info->IsShareGround;
	xc_init_data.bEnableIPAutoCoast = 0;
	xc_init_data.bMirror = 0;
	/*** panel info, should be remove in decompose stage ***/
	xc_init_data.stPanelInfo.u16HStart = 110;
	xc_init_data.stPanelInfo.u16VStart = 98;
	xc_init_data.stPanelInfo.u16Width = 3840;
	xc_init_data.stPanelInfo.u16Height = 2160;
	xc_init_data.stPanelInfo.u16HTotal = 4400;
	xc_init_data.stPanelInfo.u16VTotal = 2260;
	xc_init_data.stPanelInfo.u16DefaultVFreq = 600;
	xc_init_data.stPanelInfo.u8LPLL_Mode = 1;
	xc_init_data.stPanelInfo.enPnl_Out_Timing_Mode = 2;
	xc_init_data.stPanelInfo.u16DefaultHTotal = 4400;
	xc_init_data.stPanelInfo.u16DefaultVTotal = 2260;
	xc_init_data.stPanelInfo.u32MinSET = 1510632;
	xc_init_data.stPanelInfo.u32MaxSET = 1527016;
	xc_init_data.stPanelInfo.eLPLL_Type = 51;
	/*** panel info end ***/
	xc_init_data.bDLC_Histogram_From_VBlank = 0;
	xc_init_data.eScartIDPort_Sel = 0;
	xc_init_data.u32Main_FRCM_FB_Start_Addr =
		xc_info->Main_FRCM_FB_Start_Addr;
	xc_init_data.u32Main_FRCM_FB_Size = xc_info->Main_FRCM_FB_Size;
	xc_init_data.u32Sub_FRCM_FB_Start_Addr =
		xc_info->Sub_FRCM_FB_Start_Addr;
	xc_init_data.u32Sub_FRCM_FB_Size = xc_info->Sub_FRCM_FB_Size;
	xc_init_data.u32Dual_FB_Start_Addr = xc_info->Dual_FB_Start_Addr;
	xc_init_data.u32Dual_FB_Size = xc_info->Dual_FB_Size;

	if (MApi_XC_Init(&xc_init_data, sizeof(XC_INITDATA)) != TRUE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "xc exit fail!\n");
		return -EPERM;
	}

	XC_MUX_INPUTSRCTABLE mapping_tab[50];

	memset(mapping_tab, 0, sizeof(mapping_tab));
	mapping_tab[0].u32EnablePort = 2;
	mapping_tab[0].u32Port[0] = 1;
	mapping_tab[0].u32Port[1] = 10;
	mapping_tab[1].u32EnablePort = 1;
	mapping_tab[1].u32Port[0] = 32;
	mapping_tab[1].u32Port[1] = 0;
	mapping_tab[2].u32EnablePort = 1;
	mapping_tab[2].u32Port[0] = 30;
	mapping_tab[2].u32Port[1] = 0;
	mapping_tab[16].u32EnablePort = 1;
	mapping_tab[16].u32Port[0] = 1;
	mapping_tab[16].u32Port[1] = 0;
	mapping_tab[20].u32EnablePort = 0;
	mapping_tab[20].u32Port[0] = 2;
	mapping_tab[20].u32Port[1] = 31;
	mapping_tab[23].u32EnablePort = 1;
	mapping_tab[23].u32Port[0] = 80;
	mapping_tab[23].u32Port[1] = 0;
	mapping_tab[24].u32EnablePort = 1;
	mapping_tab[24].u32Port[0] = 81;
	mapping_tab[24].u32Port[1] = 0;
	mapping_tab[25].u32EnablePort = 1;
	mapping_tab[25].u32Port[0] = 82;
	mapping_tab[25].u32Port[1] = 0;
	mapping_tab[26].u32EnablePort = 1;
	mapping_tab[26].u32Port[0] = 83;
	mapping_tab[26].u32Port[1] = 0;
	mapping_tab[28].u32EnablePort = 1;
	mapping_tab[28].u32Port[0] = 100;
	mapping_tab[28].u32Port[1] = 0;
	mapping_tab[34].u32EnablePort = 1;
	mapping_tab[34].u32Port[0] = 100;
	mapping_tab[34].u32Port[1] = 0;
	mapping_tab[35].u32EnablePort = 1;
	mapping_tab[35].u32Port[0] = 100;
	mapping_tab[35].u32Port[1] = 0;
	mapping_tab[36].u32EnablePort = 1;
	mapping_tab[36].u32Port[0] = 100;
	mapping_tab[36].u32Port[1] = 0;
	mapping_tab[37].u32EnablePort = 1;
	mapping_tab[37].u32Port[0] = 101;
	mapping_tab[37].u32Port[1] = 0;
	mapping_tab[38].u32EnablePort = 1;
	mapping_tab[38].u32Port[0] = 101;
	mapping_tab[38].u32Port[1] = 0;
	mapping_tab[39].u32EnablePort = 1;
	mapping_tab[39].u32Port[0] = 102;
	mapping_tab[39].u32Port[1] = 0;
	mapping_tab[40].u32EnablePort = 1;
	mapping_tab[40].u32Port[0] = 110;
	mapping_tab[40].u32Port[1] = 0;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "MApi_XC_Mux_GetPortMappingMatrix\n");

	MApi_XC_Mux_GetPortMappingMatrix(mapping_tab, sizeof(mapping_tab));

	return 0;
}

static int _mtk_xc_set_misc(void *ctrl)
{
	struct v4l2_ext_ctrl_misc_info *v4l2_misc_info =
		(struct v4l2_ext_ctrl_misc_info *)ctrl;
	XC_INITMISC xc_init_misc;
	int ret;

	memset(&xc_init_misc, 0, sizeof(XC_INITMISC));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"misc_a=0x%x, misc_b=0x%x, misc_c=0x%x, misc_d=0x%x\n",
		v4l2_misc_info->misc_a, v4l2_misc_info->misc_b,
		v4l2_misc_info->misc_c, v4l2_misc_info->misc_d);

	xc_init_misc.u32MISC_A = v4l2_misc_info->misc_a;
	xc_init_misc.u32MISC_B = v4l2_misc_info->misc_b;
	xc_init_misc.u32MISC_C = v4l2_misc_info->misc_c;
	xc_init_misc.u32MISC_D = v4l2_misc_info->misc_d;

	ret = MApi_XC_Init_MISC(&xc_init_misc, sizeof(XC_INITMISC));

	if (ret != TRUE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "xc set misc fail!\n");
		return -EPERM;
	}

	return 0;
}

static int _mtk_xc_get_misc(void *ctrl)
{
	struct v4l2_ext_ctrl_misc_info *v4l2_misc_info =
		(struct v4l2_ext_ctrl_misc_info *)ctrl;
	XC_INITMISC xc_init_misc;
	int ret;

	memset(&xc_init_misc, 0, sizeof(XC_INITMISC));
	ret = MApi_XC_GetMISCStatus(&xc_init_misc);

	if (ret != TRUE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "xc get misc fail!\n");
		return -EPERM;
	}

	v4l2_misc_info->misc_a = xc_init_misc.u32MISC_A;
	v4l2_misc_info->misc_b = xc_init_misc.u32MISC_B;
	v4l2_misc_info->misc_c = xc_init_misc.u32MISC_C;
	v4l2_misc_info->misc_d = xc_init_misc.u32MISC_D;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"misc_a=0x%x, misc_b=0x%x, misc_c=0x%x, misc_d=0x%x\n",
		v4l2_misc_info->misc_a, v4l2_misc_info->misc_b,
		v4l2_misc_info->misc_c, v4l2_misc_info->misc_d);

	return 0;
}

static int _mtk_xc_set_exit(void)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc exit\n");

	if (MApi_XC_Exit() != TRUE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "xc exit fail!\n");
		return -EPERM;
	}

	return 0;
}

static int _mtk_xc_set_ds_info(struct mtk_pq_device *pq_dev, void *ctrl)
{
	XC_DS_INFO xc_ds_info;
	__u8 win = pq_dev->dev_indx;

	memcpy(&_g_ds_info[win], ctrl, sizeof(struct v4l2_ext_ctrl_ds_info));
	memset(&xc_ds_info, 0, sizeof(XC_DS_INFO));

	xc_ds_info.u32ApiDSInfo_Version = API_XCDS_INFO_VERSION;
	xc_ds_info.u16ApiDSInfo_Length = sizeof(XC_DS_INFO);
	xc_ds_info.u32MFCodecInfo = _g_ds_info[win].mfcodec_info;
	xc_ds_info.stHDRInfo.u8CurrentIndex = _g_ds_info[win].current_index;
	xc_ds_info.bUpdate_DS_CMD[win] = _g_ds_info[win].update_ds_cmd;
	xc_ds_info.bEnableDNR[win] = _g_ds_info[win].enable_dnr;
	xc_ds_info.u32DSBufferSize = 0; //check
	xc_ds_info.bEnable_ForceP[win] = _g_ds_info[win].enable_forcep;
	xc_ds_info.bDynamicScalingEnable[win] =
		_g_ds_info[win].dynamic_scaling_enable;
	xc_ds_info.u8FrameCountPQDS[win] = _g_ds_info[win].frame_count_pqds;
	xc_ds_info.enPQDSTYPE[win] = _g_ds_info[win].pqds_type;
	xc_ds_info.bUpdate_PQDS_CMD[win] = _g_ds_info[win].update_pqds_cmd;
	xc_ds_info.stCapWin[win].x = _g_cap_win[win].cap_win.left;
	xc_ds_info.stCapWin[win].y = _g_cap_win[win].cap_win.top;
	xc_ds_info.stCapWin[win].width = _g_cap_win[win].cap_win.width;
	xc_ds_info.stCapWin[win].height = _g_cap_win[win].cap_win.height;
	xc_ds_info.bInterlace[win] =
		FIELD_TO_INTERLACE(pq_dev->common_info.timing_in.interlance);
	xc_ds_info.bIsNeedSetWindow = _g_ds_info[win].is_need_set_window;
	xc_ds_info.enHDRType = _g_ds_info[win].hdr_type;
	xc_ds_info.u32XCDSUpdateCondition =
		_g_ds_info[win].xcds_update_condition;
	xc_ds_info.bRepeatWindow[win] = _g_ds_info[win].repeat_window;
	xc_ds_info.bControlMEMC[win] = _g_ds_info[win].control_memc;
	xc_ds_info.bForceMEMCOff[win] = _g_ds_info[win].force_memc_off;
	xc_ds_info.bResetMEMC[win] = _g_ds_info[win].reset_memc;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc set ds info\n");

	MApi_XC_SetDSInfo(&xc_ds_info, sizeof(XC_DS_INFO), win);

	return 0;
}

static int _mtk_xc_get_ds_info(__u8 win, void *ctrl)
{
	XC_GET_DS_INFO xc_ds_info;

	memset(&xc_ds_info, 0, sizeof(XC_GET_DS_INFO));
	xc_ds_info.u32ApiGetDSInfo_Version = API_GET_XCDS_INFO_VERSION;
	xc_ds_info.u16ApiGetDSInfo_Length = sizeof(XC_GET_DS_INFO);
	MApi_XC_GetDSInfo(&xc_ds_info, sizeof(XC_GET_DS_INFO), win);

	_g_ds_info[win].update_ds_cmd = xc_ds_info.bUpdate_DS_CMD;
	_g_ds_info[win].dynamic_scaling_enable =
		xc_ds_info.bDynamicScalingEnable;
	memcpy(_g_ds_info[win].ds_xrule_frame_count,
		xc_ds_info.u8DS_XRule_FrameCount, sizeof(__u8)*10);

	memcpy(ctrl, &_g_ds_info[win],
		sizeof(struct v4l2_ext_ctrl_ds_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc get ds info\n");

	return 0;
}

static int _mtk_xc_set_freeze(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_freeze_info *v4l2_freeze_info =
		(struct v4l2_ext_ctrl_freeze_info *)ctrl;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc set freeze=%d, win=%d\n",
		v4l2_freeze_info->enable, win);

	MApi_XC_FreezeImg(v4l2_freeze_info->enable, win);

	return 0;
}

static int _mtk_xc_get_freeze(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_freeze_info *v4l2_freeze_info =
		(struct v4l2_ext_ctrl_freeze_info *)ctrl;

	v4l2_freeze_info->enable = MApi_XC_IsFreezeImg(win);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc get freeze=%d, win=%d\n",
		v4l2_freeze_info->enable, win);

	return 0;
}

static int _mtk_xc_set_fb_mode(void *ctrl)
{
	struct v4l2_ext_ctrl_fb_mode_info *v4l2_fb_mode_info =
		(struct v4l2_ext_ctrl_fb_mode_info *)ctrl;

	if (v4l2_fb_mode_info->fb_level == V4L2_FB_LEVEL_FBL)
		MApi_XC_EnableFrameBufferLess(TRUE);
	else if (v4l2_fb_mode_info->fb_level == V4L2_FB_LEVEL_RFBL_DI)
		MApi_XC_EnableRequest_FrameBufferLess(TRUE);
	else if (v4l2_fb_mode_info->fb_level == V4L2_FB_LEVEL_FB) {
		MApi_XC_EnableFrameBufferLess(FALSE);
		MApi_XC_EnableRequest_FrameBufferLess(FALSE);
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unsupport FB level %d!\n",
			v4l2_fb_mode_info->fb_level);
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "set fb level = %d\n",
		v4l2_fb_mode_info->fb_level);

	return 0;
}

int mtk_xc_get_fb_mode(enum v4l2_fb_level *fb_level)
{
	bool bFBL, bRFBL;

	if (fb_level == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "fb_level = NULL!\n");
		return -EINVAL;
	}

	bFBL = MApi_XC_IsCurrentFrameBufferLessMode();
	bRFBL = MApi_XC_IsCurrentRequest_FrameBufferLessMode();

	if (bFBL)
		*fb_level = V4L2_FB_LEVEL_FBL;
	else if (bRFBL)
		*fb_level = V4L2_FB_LEVEL_RFBL_DI;
	else
		*fb_level = V4L2_FB_LEVEL_FB;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "get fb level = %d\n", *fb_level);

	return 0;
}

static int _mtk_xc_get_fb_mode(void *ctrl)
{
	struct v4l2_ext_ctrl_fb_mode_info *v4l2_fb_mode_info =
		(struct v4l2_ext_ctrl_fb_mode_info *)ctrl;

	return mtk_xc_get_fb_mode(&v4l2_fb_mode_info->fb_level);
}

static int _mtk_xc_set_rwbankauto_enable(__u8 win, void *ctrl)
{
	memcpy(&_g_rwba[win], ctrl,
		sizeof(struct v4l2_ext_ctrl_rwbankauto_enable_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc set rwbankauto enable = %d, win=%d\n",
		_g_rwba[win].enable, win);

	MApi_XC_EnableRWBankAuto(
		_g_rwba[win].enable, win);

	return 0;
}

static int _mtk_xc_get_rwbankauto_enable(__u8 win, void *ctrl)
{
	memcpy(ctrl, &_g_rwba[win],
		sizeof(struct v4l2_ext_ctrl_rwbankauto_enable_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "enable=%d, win=%d\n",
		_g_rwba[win].enable, win);

	return 0;
}

static int _mtk_xc_set_testpattern_enable(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_testpattern_enable_info *tp_enable_info =
		(struct v4l2_ext_ctrl_testpattern_enable_info *)ctrl;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc set testpattern mode=%d, win=%d\n",
		tp_enable_info->test_pattern_mode, win);
	switch (tp_enable_info->test_pattern_mode) {
	case V4L2_TEST_PATTERN_MODE_IP1:
	{
		XC_SET_IP1_TESTPATTERN_t xc_ip1_testpattern_info;

		memset(&xc_ip1_testpattern_info, 0,
			sizeof(XC_SET_IP1_TESTPATTERN_t));
		xc_ip1_testpattern_info.u16Enable =
			tp_enable_info->testpattern_ip1_info.enable;
		xc_ip1_testpattern_info.u32Pattern_type =
			tp_enable_info->testpattern_ip1_info.pattern_type;
		xc_ip1_testpattern_info.eWindow = win;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"ip1: enable=%d, patternType=0x%x, win=%d\n",
			xc_ip1_testpattern_info.u16Enable,
			xc_ip1_testpattern_info.u32Pattern_type, win);

		MApi_XC_GenerateTestPattern(E_XC_IP1_PATTERN_MODE,
			&xc_ip1_testpattern_info,
			sizeof(XC_SET_IP1_TESTPATTERN_t));

		break;
	}
	case V4L2_TEST_PATTERN_MODE_IP2:
	{
		XC_SET_IP2_TESTPATTERN_t xc_ip2_testpattern_info;

		memset(&xc_ip2_testpattern_info, 0,
			sizeof(XC_SET_IP2_TESTPATTERN_t));
		xc_ip2_testpattern_info.bEnable =
			tp_enable_info->testpattern_ip2_info.enable;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"ip2: enable=%d, win=%d\n",
			xc_ip2_testpattern_info.bEnable, win);

		MApi_XC_GenerateTestPattern(E_XC_IP2_PATTERN_MODE,
			&xc_ip2_testpattern_info,
			sizeof(XC_SET_IP2_TESTPATTERN_t));

		break;
	}
	case V4L2_TEST_PATTERN_MODE_OP:
	{
		XC_SET_OP_TESTPATTERN_t xc_op_testpattern_info;

		memset(&xc_op_testpattern_info, 0,
			sizeof(XC_SET_OP_TESTPATTERN_t));
		xc_op_testpattern_info.bMiuLineBuff =
			tp_enable_info->testpattern_op_info.miu_line_buff;
		xc_op_testpattern_info.bLineBuffHVSP =
			tp_enable_info->testpattern_op_info.line_buff_hvsp;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"op: miu_line_buff=%d, line_buff_hvsp=%d, win=%d\n",
			xc_op_testpattern_info.bMiuLineBuff,
			xc_op_testpattern_info.bLineBuffHVSP, win);

		MApi_XC_GenerateTestPattern(E_XC_OP_PATTERN_MODE,
			&xc_op_testpattern_info,
			sizeof(XC_SET_OP_TESTPATTERN_t));

		break;
	}
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown testpattern mode %d!\n",
			tp_enable_info->test_pattern_mode);
		return -EINVAL;
	}

	return 0;
}

static int _mtk_xc_set_scaling_enable(__u8 win, void *ctrl)
{
	E_XC_SCALING_TYPE xc_scaling_type;
	E_XC_VECTOR_TYPE xc_vector_type;

	memcpy(&_g_scaling[win], ctrl,
		sizeof(struct v4l2_ext_ctrl_scaling_enable_info));

	switch (_g_scaling[win].scaling_type) {
	case V4L2_SCALING_TYPE_PRE:
		xc_scaling_type = E_XC_PRE_SCALING;
		break;
	case V4L2_SCALING_TYPE_POST:
		xc_scaling_type = E_XC_POST_SCALING;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unknown scaling type %d!\n",
			_g_scaling[win].scaling_type);
		return -EINVAL;
	}

	switch (_g_scaling[win].vector_type) {
	case V4L2_VECTOR_TYPE_H:
		xc_vector_type = E_XC_H_VECTOR;
		break;
	case V4L2_VECTOR_TYPE_V:
		xc_vector_type = E_XC_V_VECTOR;
		break;
	case V4L2_VECTOR_TYPE_HV:
		xc_vector_type = E_XC_HV_VECTOR;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unknown vector type %d!\n",
			_g_scaling[win].vector_type);
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"enable=%d, scalingType=%d, vectorType=%d, win=%d\n",
		_g_scaling[win].enable,
		xc_scaling_type, xc_vector_type, win);

	MApi_XC_SetScaling(_g_scaling[win].enable,
		xc_scaling_type, xc_vector_type, win);

	return 0;
}

static int _mtk_xc_get_scaling_enable(__u8 win, void *ctrl)
{
	memcpy(ctrl, &_g_scaling[win],
		sizeof(struct v4l2_ext_ctrl_scaling_enable_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"enable=%d, scalingType=%d, vectorType=%d, win=%d\n",
		_g_scaling[win].enable,
		_g_scaling[win].scaling_type,
		_g_scaling[win].vector_type, win);

	return 0;
}

static int _mtk_xc_set_window(struct mtk_pq_device *pq_dev, __u8 win,
	enum set_window_type set_win_type, void *para, __u16 para_length)
{
	static XC_SETWIN_INFO xc_set_window_info = {0};
	static bool updated_cap_win = FALSE;
	static bool updated_crop_win = FALSE;
	static bool updated_disp_win = FALSE;
	static bool updated_pre_scaling = FALSE;
	static bool updated_post_scaling = FALSE;

	switch (set_win_type) {
	case SET_WINDOW_TYPE_CAP_WIN:
	{
		if (para_length != sizeof(MS_WINDOW_TYPE)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"inconsistance cap win size %d & %ld!\n",
				para_length, sizeof(MS_WINDOW_TYPE));
			return -EINVAL;
		}

		memcpy(&xc_set_window_info.stCapWin, para,
			sizeof(MS_WINDOW_TYPE));
		updated_cap_win = TRUE;
		break;
	}
	case SET_WINDOW_TYPE_CROP_WIN:
	{
		if (para_length != sizeof(MS_WINDOW_TYPE)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"inconsistance crop win size %d & %ld!\n",
				para_length, sizeof(MS_WINDOW_TYPE));
			return -EINVAL;
		}

		memcpy(&xc_set_window_info.stCropWin, para,
			sizeof(MS_WINDOW_TYPE));
		updated_crop_win = TRUE;
		break;
	}
	case SET_WINDOW_TYPE_DISP_WIN:
	{
		if (para_length != sizeof(MS_WINDOW_TYPE)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"inconsistance disp win size %d & %ld!\n",
				para_length, sizeof(MS_WINDOW_TYPE));
			return -EINVAL;
		}

		memcpy(&xc_set_window_info.stDispWin, para,
			sizeof(MS_WINDOW_TYPE));
		updated_disp_win = TRUE;
		break;
	}
	case SET_WINDOW_TYPE_PRE_SCALING:
	{
		if (para_length != sizeof(struct scaling_info)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"inconsistance pre-scaling size %d & %ld!\n",
				para_length, sizeof(struct scaling_info));
			return -EINVAL;
		}

		memcpy(&xc_set_window_info.bPreHCusScaling, para,
			sizeof(struct scaling_info));
		updated_pre_scaling = TRUE;
		break;
	}
	case SET_WINDOW_TYPE_POST_SCALING:
	{
		if (para_length != sizeof(struct scaling_info)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"inconsistance post-scaling size %d & %ld!\n",
				para_length, sizeof(struct scaling_info));
			return -EINVAL;
		}

		memcpy(&xc_set_window_info.bHCusScaling, para,
			sizeof(struct scaling_info));
		updated_post_scaling = TRUE;
		break;
	}
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown set window type %d!\n", set_win_type);
		return -EINVAL;
	}

	if (updated_cap_win && updated_crop_win && updated_disp_win &&
		updated_pre_scaling && updated_post_scaling) {
		xc_set_window_info.enInputSourceType =
			pq_dev->common_info.input_source;
		xc_set_window_info.bInterlace =
			FIELD_TO_INTERLACE(
			pq_dev->common_info.timing_in.interlance);
		xc_set_window_info.bHDuplicate =
			pq_dev->common_info.timing_in.h_duplicate;
		xc_set_window_info.u16InputVFreq =
			pq_dev->common_info.timing_in.v_freq / 100;
		xc_set_window_info.u16InputVTotal =
			pq_dev->common_info.timing_in.v_total;
		xc_set_window_info.u16DefaultHtotal =
			pq_dev->common_info.timing_in.h_total;
		xc_set_window_info.u8DefaultPhase = 0;
		xc_set_window_info.bDisplayNineLattice = 0;
		xc_set_window_info.u16DefaultPhase = 0;
		xc_set_window_info.bSetDispWinToReg = 0;

		updated_cap_win = FALSE;
		updated_crop_win = FALSE;
		updated_disp_win = FALSE;
		updated_pre_scaling = FALSE;
		updated_post_scaling = FALSE;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc set window\n");

		if (MApi_XC_SetWindow(&xc_set_window_info,
			sizeof(XC_SETWIN_INFO), win) != TRUE)
			return -EPERM;
	}

	return 0;
}

static int _mtk_xc_set_cap_win(struct mtk_pq_device *pq_dev, void *ctrl)
{
	__u8 win = pq_dev->dev_indx;
	MS_WINDOW_TYPE xc_cap_win_info;

	memcpy(&_g_cap_win[win], ctrl,
		sizeof(struct v4l2_ext_ctrl_cap_win_info));
	memset(&xc_cap_win_info, 0, sizeof(MS_WINDOW_TYPE));

	xc_cap_win_info.x = _g_cap_win[win].cap_win.left;
	xc_cap_win_info.y = _g_cap_win[win].cap_win.top;
	xc_cap_win_info.width = _g_cap_win[win].cap_win.width;
	xc_cap_win_info.height = _g_cap_win[win].cap_win.height;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc set cap win: (x,y,w,h)=(%d, %d, %d, %d), win=%d\n",
		xc_cap_win_info.x, xc_cap_win_info.y,
		xc_cap_win_info.width, xc_cap_win_info.height, win);

	return _mtk_xc_set_window(pq_dev, win, SET_WINDOW_TYPE_CAP_WIN,
		&xc_cap_win_info, sizeof(MS_WINDOW_TYPE));
}

static int _mtk_xc_get_cap_win(__u8 win, void *ctrl)
{
	memcpy(ctrl, &_g_cap_win[win],
		sizeof(struct v4l2_ext_ctrl_cap_win_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc get cap win: (x,y,w,h)=(%d, %d, %d, %d), win=%d\n",
		_g_cap_win[win].cap_win.left,
		_g_cap_win[win].cap_win.top,
		_g_cap_win[win].cap_win.width,
		_g_cap_win[win].cap_win.height, win);

	return 0;
}

static int _mtk_xc_set_crop_win(struct mtk_pq_device *pq_dev, void *ctrl)
{
	__u8 win = pq_dev->dev_indx;
	MS_WINDOW_TYPE xc_crop_win_info;

	memcpy(&_g_crop_win[win], ctrl,
		sizeof(struct v4l2_ext_ctrl_crop_win_info));
	memset(&xc_crop_win_info, 0, sizeof(MS_WINDOW_TYPE));

	xc_crop_win_info.x = _g_crop_win[win].crop_win.left;
	xc_crop_win_info.y = _g_crop_win[win].crop_win.top;
	xc_crop_win_info.width = _g_crop_win[win].crop_win.width;
	xc_crop_win_info.height = _g_crop_win[win].crop_win.height;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc set crop win: (x,y,w,h)=(%d, %d, %d, %d), win=%d\n",
		xc_crop_win_info.x, xc_crop_win_info.y,
		xc_crop_win_info.width, xc_crop_win_info.height, win);

	return _mtk_xc_set_window(pq_dev, win, SET_WINDOW_TYPE_CROP_WIN,
		&xc_crop_win_info, sizeof(MS_WINDOW_TYPE));
}

static int _mtk_xc_get_crop_win(__u8 win, void *ctrl)
{
	memcpy(ctrl, &_g_crop_win[win],
		sizeof(struct v4l2_ext_ctrl_crop_win_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc get crop win: (x,y,w,h)=(%d, %d, %d, %d), win=%d\n",
		_g_crop_win[win].crop_win.left,
		_g_crop_win[win].crop_win.top,
		_g_crop_win[win].crop_win.width,
		_g_crop_win[win].crop_win.height, win);

	return 0;
}

static int _mtk_xc_set_disp_win(struct mtk_pq_device *pq_dev, void *ctrl)
{
	__u8 win = pq_dev->dev_indx;
	MS_WINDOW_TYPE xc_disp_win_info;

	memcpy(&_g_disp_win[win], ctrl,
		sizeof(struct v4l2_ext_ctrl_disp_win_info));
	memset(&xc_disp_win_info, 0, sizeof(MS_WINDOW_TYPE));

	xc_disp_win_info.x = _g_disp_win[win].disp_win.left;
	xc_disp_win_info.y = _g_disp_win[win].disp_win.top;
	xc_disp_win_info.width = _g_disp_win[win].disp_win.width;
	xc_disp_win_info.height = _g_disp_win[win].disp_win.height;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc set disp win: (x,y,w,h)=(%d, %d, %d, %d), win=%d\n",
		xc_disp_win_info.x, xc_disp_win_info.y,
		xc_disp_win_info.width, xc_disp_win_info.height, win);

	return _mtk_xc_set_window(pq_dev, win, SET_WINDOW_TYPE_DISP_WIN,
		&xc_disp_win_info, sizeof(MS_WINDOW_TYPE));
}

static int _mtk_xc_get_disp_win(__u8 win, void *ctrl)
{
	memcpy(ctrl, &_g_disp_win[win],
		sizeof(struct v4l2_ext_ctrl_disp_win_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc get disp win: (x,y,w,h)=(%d, %d, %d, %d), win=%d\n",
		_g_disp_win[win].disp_win.left,
		_g_disp_win[win].disp_win.top,
		_g_disp_win[win].disp_win.width,
		_g_disp_win[win].disp_win.height, win);

	return 0;
}

static int _mtk_xc_set_pre_scaling(struct mtk_pq_device *pq_dev, void *ctrl)
{
	__u8 win = pq_dev->dev_indx;
	struct v4l2_ext_ctrl_scaling_info *v4l2_scaling_info =
		(struct v4l2_ext_ctrl_scaling_info *)ctrl;
	struct scaling_info xc_scaling_info;
	int ret;

	memset(&xc_scaling_info, 0, sizeof(struct scaling_info));

	xc_scaling_info.bHCusScaling = v4l2_scaling_info->enable_h_scaling;
	xc_scaling_info.u16HCusScalingSrc = v4l2_scaling_info->h_scaling_src;
	xc_scaling_info.u16HCusScalingDst = v4l2_scaling_info->h_scaling_dst;
	xc_scaling_info.bVCusScaling = v4l2_scaling_info->enable_v_scaling;
	xc_scaling_info.u16VCusScalingSrc = v4l2_scaling_info->v_scaling_src;
	xc_scaling_info.u16VCusScalingDst = v4l2_scaling_info->v_scaling_dst;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc set pre scaling: H(%d, %d->%d), V(%d, %d->%d), win=%d\n",
		xc_scaling_info.bHCusScaling,
		xc_scaling_info.u16HCusScalingSrc,
		xc_scaling_info.u16HCusScalingDst,
		xc_scaling_info.bVCusScaling,
		xc_scaling_info.u16VCusScalingSrc,
		xc_scaling_info.u16VCusScalingDst, win);

	ret = _mtk_xc_set_window(pq_dev, win, SET_WINDOW_TYPE_PRE_SCALING,
		&xc_scaling_info, sizeof(struct scaling_info));
	memcpy(&_g_pre_scaling[win], v4l2_scaling_info,
		sizeof(struct v4l2_ext_ctrl_scaling_info));

	return 0;
}

static int _mtk_xc_get_pre_scaling(__u8 win, void *ctrl)
{
	memcpy(ctrl, &_g_pre_scaling[win],
		sizeof(struct v4l2_ext_ctrl_scaling_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc get pre scaling: H(%d, %d->%d), V(%d, %d->%d), win=%d\n",
		_g_pre_scaling[win].enable_h_scaling,
		_g_pre_scaling[win].h_scaling_src,
		_g_pre_scaling[win].h_scaling_dst,
		_g_pre_scaling[win].enable_v_scaling,
		_g_pre_scaling[win].v_scaling_src,
		_g_pre_scaling[win].v_scaling_dst, win);

	return 0;
}

static int _mtk_xc_set_post_scaling(struct mtk_pq_device *pq_dev, void *ctrl)
{
	__u8 win = pq_dev->dev_indx;
	struct v4l2_ext_ctrl_scaling_info *v4l2_scaling_info =
		(struct v4l2_ext_ctrl_scaling_info *)ctrl;
	struct scaling_info xc_scaling_info;
	int ret;

	memset(&xc_scaling_info, 0, sizeof(struct scaling_info));

	xc_scaling_info.bHCusScaling = v4l2_scaling_info->enable_h_scaling;
	xc_scaling_info.u16HCusScalingSrc = v4l2_scaling_info->h_scaling_src;
	xc_scaling_info.u16HCusScalingDst = v4l2_scaling_info->h_scaling_dst;
	xc_scaling_info.bVCusScaling = v4l2_scaling_info->enable_v_scaling;
	xc_scaling_info.u16VCusScalingSrc = v4l2_scaling_info->v_scaling_src;
	xc_scaling_info.u16VCusScalingDst = v4l2_scaling_info->v_scaling_dst;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc set post scaling: H(%d, %d->%d), V(%d, %d->%d), win=%d\n",
		xc_scaling_info.bHCusScaling,
		xc_scaling_info.u16HCusScalingSrc,
		xc_scaling_info.u16HCusScalingDst,
		xc_scaling_info.bVCusScaling,
		xc_scaling_info.u16VCusScalingSrc,
		xc_scaling_info.u16VCusScalingDst, win);

	ret = _mtk_xc_set_window(pq_dev, win, SET_WINDOW_TYPE_POST_SCALING,
		&xc_scaling_info, sizeof(struct scaling_info));
	memcpy(&_g_post_scaling[win], v4l2_scaling_info,
		sizeof(struct v4l2_ext_ctrl_scaling_info));

	return 0;
}

static int _mtk_xc_get_post_scaling(__u8 win, void *ctrl)
{
	memcpy(ctrl, &_g_post_scaling[win],
		sizeof(struct v4l2_ext_ctrl_scaling_info));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc get post scaling: H(%d, %d->%d), V(%d, %d->%d), win=%d\n",
		_g_post_scaling[win].enable_h_scaling,
		_g_post_scaling[win].h_scaling_src,
		_g_post_scaling[win].h_scaling_dst,
		_g_post_scaling[win].enable_v_scaling,
		_g_post_scaling[win].v_scaling_src,
		_g_post_scaling[win].v_scaling_dst, win);

	return 0;
}

// this api only used for preparing _mtk_xc_get_status
static int _mtk_xc_set_status(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_status_info *v4l2_status_info =
		(struct v4l2_ext_ctrl_status_info *)ctrl;

	_g_status[win].report_pixel_info.pixel_rgb_stage =
		v4l2_status_info->report_pixel_info.pixel_rgb_stage;
	_g_status[win].report_pixel_info.rep_win_color =
		v4l2_status_info->report_pixel_info.rep_win_color;
	_g_status[win].report_pixel_info.x_start =
		v4l2_status_info->report_pixel_info.x_start;
	_g_status[win].report_pixel_info.x_end =
		v4l2_status_info->report_pixel_info.x_end;
	_g_status[win].report_pixel_info.y_start =
		v4l2_status_info->report_pixel_info.y_start;
	_g_status[win].report_pixel_info.y_end =
		v4l2_status_info->report_pixel_info.y_end;
	_g_status[win].report_pixel_info.show_rep_win =
		v4l2_status_info->report_pixel_info.show_rep_win;

	return 0;
}

static int _mtk_xc_get_status(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_status_info *status =
		(struct v4l2_ext_ctrl_status_info *)ctrl;
	XC_ApiStatusEx xc_status;
	MS_XC_REPORT_PIXELINFO xc_report_pixel;
	ST_XC_CFD_POINT_INFO xc_cfd_point;

	memset(&xc_status, 0, sizeof(XC_ApiStatusEx));
	memset(&xc_report_pixel, 0, sizeof(MS_XC_REPORT_PIXELINFO));
	memset(&xc_cfd_point, 0, sizeof(ST_XC_CFD_POINT_INFO));

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc set status\n");

	xc_status.u32ApiStatusEx_Version = API_STATUS_EX_VERSION;
	xc_status.u16ApiStatusEX_Length = sizeof(XC_ApiStatusEx);
	xc_cfd_point.u32Version = ST_XC_CFD_POINT_INFO_VERSION;
	xc_cfd_point.u32Length = sizeof(ST_XC_CFD_POINT_INFO);
	xc_status.pstCfdPointInfo = &xc_cfd_point;
	if (MApi_XC_GetStatusEx(&xc_status, win) != sizeof(XC_ApiStatusEx))
		return -EPERM;

	xc_report_pixel.u32ReportPixelInfo_Version =
		XC_REPORT_PIXELINFO_VERSION;
	xc_report_pixel.u16ReportPixelInfo_Length =
		sizeof(MS_XC_REPORT_PIXELINFO);
	xc_report_pixel.enStage =
		_g_status[win].report_pixel_info.pixel_rgb_stage;
	xc_report_pixel.u16RepWinColor =
		_g_status[win].report_pixel_info.rep_win_color;
	xc_report_pixel.u16XStart =
		_g_status[win].report_pixel_info.x_start;
	xc_report_pixel.u16XEnd =
		_g_status[win].report_pixel_info.x_end;
	xc_report_pixel.u16YStart =
		_g_status[win].report_pixel_info.y_start;
	xc_report_pixel.u16YEnd =
		_g_status[win].report_pixel_info.y_end;
	xc_report_pixel.bShowRepWin =
		_g_status[win].report_pixel_info.show_rep_win;
	if (MApi_XC_ReportPixelInfo(&xc_report_pixel) != TRUE)
		return -EPERM;

	status->h_size_after_prescaling = xc_status.u16H_SizeAfterPreScaling;
	status->v_size_after_prescaling = xc_status.u16V_SizeAfterPreScaling;
	status->scaled_crop_win.left = xc_status.ScaledCropWin.x;
	status->scaled_crop_win.top = xc_status.ScaledCropWin.y;
	status->scaled_crop_win.width = xc_status.ScaledCropWin.width;
	status->scaled_crop_win.height = xc_status.ScaledCropWin.height;
	status->v_length = xc_status.u16V_Length;
	status->bit_per_pixel = xc_status.u16BytePerWord;
	status->deinterlace_mode = xc_status.eDeInterlaceMode;
	status->linear_mode = xc_status.bLinearMode;
	status->fast_frame_lock = xc_status.bFastFrameLock;
	status->done_fpll = xc_status.bDoneFPLL;
	status->enable_fpll = xc_status.bEnableFPLL;
	status->fpll_lock = xc_status.bFPLL_LOCK;
	status->pq_set_hsd = xc_status.bPQSetHSD;
	status->default_phase = xc_status.u16DefaultPhase;
	status->is_hw_depth_adj_supported = xc_status.bIsHWDepthAdjSupported;
	status->is_2line_mode = xc_status.bIs2LineMode;
	status->is_pnl_yuv_output = xc_status.bIsPNLYUVOutput;
	status->hdmi_pixel_repetition = xc_status.u8HDMIPixelRepetition;
	status->fsc_enabled = xc_status.bFSCEnabled;
	status->frc_enabled = xc_status.bFRCEnabled;
	status->panel_interface_type = xc_status.u16PanelInterfaceType;
	status->is_hsk_mode = xc_status.bIsHskMode;
	status->enable_1pto2p_bypass = xc_status.bEnable1pTo2pBypass;
	status->format = xc_status.pstCfdPointInfo->u8Format;
	status->color_primaries = xc_status.pstCfdPointInfo->u8ColorPriamries;
	status->transfer_characterstics =
		xc_status.pstCfdPointInfo->u8TransferCharacterstics;
	status->matrix_coefficients =
		xc_status.pstCfdPointInfo->u8MatrixCoefficients;
	status->is_hfr_path_enable = xc_status.bIsHFRPathEnable;
	status->lock_point_auto = xc_status.u16LockPointAuto;
	status->is_120hz_output = xc_status.bIs120HzOutput;

	status->report_pixel_info.r_cr_min = xc_report_pixel.u16RCrMin;
	status->report_pixel_info.r_cr_max = xc_report_pixel.u16RCrMax;
	status->report_pixel_info.g_y_min = xc_report_pixel.u16GYMin;
	status->report_pixel_info.g_y_max = xc_report_pixel.u16GYMax;
	status->report_pixel_info.b_cb_min = xc_report_pixel.u16BCbMin;
	status->report_pixel_info.b_cb_max = xc_report_pixel.u16BCbMax;
	status->report_pixel_info.r_cr_sum = xc_report_pixel.u32RCrSum;
	status->report_pixel_info.g_y_sum = xc_report_pixel.u32GYSum;
	status->report_pixel_info.b_cb_sum = xc_report_pixel.u32BCbSum;

	return 0;
}

// this api only used for preparing _mtk_xc_get_caps
static int _mtk_xc_set_caps(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_caps_info *v4l2_caps_info =
		(struct v4l2_ext_ctrl_caps_info *)ctrl;

	_g_caps[win].cap_type = v4l2_caps_info->cap_type;

	switch (v4l2_caps_info->cap_type) {
	// return type = bool
	case V4L2_CAP_TYPE_IMMESWITCH:
	case V4L2_CAP_TYPE_DVI_AUTO_EQ:
	case V4L2_CAP_TYPE_FRC_INSIDE:
	case V4L2_CAP_TYPE_3D_FBL_CAPS:
	case V4L2_CAP_TYPE_HW_SEAMLESS_ZAPPING:
	case V4L2_CAP_TYPE_SUPPORT_DEVICE1:
	case V4L2_CAP_TYPE_SUPPORT_DETECT3D_IN3DMODE:
	case V4L2_CAP_TYPE_SUPPORT_FORCE_VSP_IN_DS_MODE:
	case V4L2_CAP_TYPE_SUPPORT_FRCM_MODE:
	case V4L2_CAP_TYPE_SUPPORT_INTERLACE_OUT:
	case V4L2_CAP_TYPE_SUPPORT_4K2K_WITH_PIP:
	case V4L2_CAP_TYPE_SUPPORT_4K2K_60P:
	case V4L2_CAP_TYPE_SUPPORT_PIP_PATCH_USING_SC1_MAIN_AS_SC0_SUB:
	case V4L2_CAP_TYPE_HW_4K2K_VIP_PEAKING_LIMITATION:
	case V4L2_CAP_TYPE_SUPPORT_3D_DS:
	case V4L2_CAP_TYPE_DIP_CHIP_SOURCESEL:
	case V4L2_CAP_TYPE_SCALING_LIMITATION:
	case V4L2_CAP_TYPE_SUPPORT_HDMI_SEAMLESS:
	case V4L2_CAP_TYPE_HDR_HW_SUPPORT_HANDSHAKE_MODE:
	case V4L2_CAP_TYPE_SUPPORT_HW_HSY:
	case V4L2_CAP_TYPE_SUPPORT_MOD_HMIRROR:
	case V4L2_CAP_TYPE_SUPPORT_ZNR:
	case V4L2_CAP_TYPE_SUPPORT_ABF:
	case V4L2_CAP_TYPE_SUPPORT_FRC_HSK_MODE:
	case V4L2_CAP_TYPE_SUPPORT_SPF_LB_MODE:
	case V4L2_CAP_TYPE_SUPPORT_HSE:
	case V4L2_CAP_TYPE_SUPPORT_HFR_PATH:
	case V4L2_CAP_TYPE_SUPPORT_PQ_DS_PLUS:
	case V4L2_CAP_TYPE_SUPPORT_SW_DS:
	case V4L2_CAP_TYPE_IS_SUPPORT_SUB_DISP:
		break;
	// return type = u32
	case V4L2_CAP_TYPE_2DTO3D_VERSION:
	case V4L2_CAP_TYPE_HW_SR_VERSION:
	case V4L2_CAP_TYPE_FB_CAPS_GET_FB_LEVEL:
	case V4L2_CAP_TYPE_FB_CAPS_GET_FB_LEVEL_EX:
		break;
	// return type = struct v4l2_dip_caps
	case V4L2_CAP_TYPE_DIP_CHIP_CAPS:
	case V4L2_CAP_TYPE_DIP_CHIP_WINDOWBUS:
		_g_caps[win].dip_caps.window =
			v4l2_caps_info->dip_caps.window;
		break;
	// return type = struct v4l2_autodownload_client_supported_caps
	case V4L2_CAP_TYPE_SUPPORT_AUTODOWNLOAD_CLIENT:
		_g_caps[win].adl_caps.client =
			v4l2_caps_info->adl_caps.client;
		break;
	// return type = struct v4l2_hdr_supported_caps
	case V4L2_CAP_TYPE_SUPPORT_HDR:
		_g_caps[win].hdr_caps.hdr_type =
			v4l2_caps_info->hdr_caps.hdr_type;
		break;
	// return type = struct v4l2_pqds_supported_caps
	case V4L2_CAP_TYPE_SUPPORT_PQ_DS:
		_g_caps[win].pqds_caps.pqds_type =
			v4l2_caps_info->pqds_caps.pqds_type;
		break;
	// return type = struct v4l2_fast_disp_supported_caps
	case V4L2_CAP_TYPE_HW_FAST_DISP:
		_g_caps[win].fd_caps.fd_type =
			v4l2_caps_info->fd_caps.fd_type;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unknown cap type %d!\n",
			v4l2_caps_info->cap_type);
		return -EINVAL;
	}

	return 0;
}

static int _mtk_xc_get_caps(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_caps_info *v4l2_caps_info =
		(struct v4l2_ext_ctrl_caps_info *)ctrl;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc get caps: type=%d\n",
		_g_caps[win].cap_type);

	v4l2_caps_info->cap_type = _g_caps[win].cap_type;
	switch (_g_caps[win].cap_type) {
	case V4L2_CAP_TYPE_IMMESWITCH:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_IMMESWITCH,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_DVI_AUTO_EQ:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_DVI_AUTO_EQ,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_FRC_INSIDE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_FRC_INSIDE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_DIP_CHIP_CAPS:
	{
		ST_XC_DIP_CHIPCAPS xc_dip_caps;

		xc_dip_caps.eWindow = v4l2_caps_info->dip_caps.window;
		if (MApi_XC_GetChipCaps(E_XC_DIP_CHIP_CAPS,
			(MS_U32 *)&xc_dip_caps,
			sizeof(ST_XC_DIP_CHIPCAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->dip_caps.dip_caps = xc_dip_caps.u32DipChipCaps;

		break;
	}
	case V4L2_CAP_TYPE_3D_FBL_CAPS:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_3D_FBL_CAPS,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_HW_SEAMLESS_ZAPPING:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_HW_SEAMLESS_ZAPPING,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_DEVICE1:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_DEVICE1,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_DETECT3D_IN3DMODE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_DETECT3D_IN3DMODE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_2DTO3D_VERSION:
	{
		MS_U32 u32;

		if (MApi_XC_GetChipCaps(E_XC_2DTO3D_VERSION,
			&u32, sizeof(MS_U32)) != TRUE)
			return -EPERM;

		v4l2_caps_info->u32 = u32;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_FORCE_VSP_IN_DS_MODE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_FORCE_VSP_IN_DS_MODE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_FRCM_MODE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_FRCM_MODE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_INTERLACE_OUT:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_INTERLACE_OUT,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_4K2K_WITH_PIP:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_4K2K_WITH_PIP,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_4K2K_60P:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_4K2K_60P,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_PIP_PATCH_USING_SC1_MAIN_AS_SC0_SUB:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(
			E_XC_SUPPORT_PIP_PATCH_USING_SC1_MAIN_AS_SC0_SUB,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_HW_4K2K_VIP_PEAKING_LIMITATION:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_HW_4K2K_VIP_PEAKING_LIMITATION,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_AUTODOWNLOAD_CLIENT:
	{
		XC_AUTODOWNLOAD_CLIENT_SUPPORTED_CAPS xc_adl_client_caps;

		xc_adl_client_caps.enClient =
			_g_caps[win].adl_caps.client;
		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_AUTODOWNLOAD_CLIENT,
			(MS_U32 *)&xc_adl_client_caps,
			sizeof(XC_AUTODOWNLOAD_CLIENT_SUPPORTED_CAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->adl_caps.supported =
			xc_adl_client_caps.bSupported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_HDR:
	{
		XC_HDR_SUPPORTED_CAPS xc_hdr_caps;

		xc_hdr_caps.enHDRType =
			_g_caps[win].hdr_caps.hdr_type;
		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_HDR,
			(MS_U32 *)&xc_hdr_caps,
			sizeof(XC_HDR_SUPPORTED_CAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->hdr_caps.supported =
			xc_hdr_caps.bSupported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_3D_DS:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_3D_DS,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_DIP_CHIP_SOURCESEL:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_DIP_CHIP_SOURCESEL,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_DIP_CHIP_WINDOWBUS:
	{
		ST_XC_DIP_CHIPCAPS xc_dip_caps;

		xc_dip_caps.eWindow = _g_caps[win].dip_caps.window;
		if (MApi_XC_GetChipCaps(E_XC_DIP_CHIP_WINDOWBUS,
			(MS_U32 *)&xc_dip_caps,
			sizeof(ST_XC_DIP_CHIPCAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->dip_caps.dip_caps = xc_dip_caps.u32DipChipCaps;

		break;
	}
	case V4L2_CAP_TYPE_SCALING_LIMITATION:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SCALING_LIMITATION,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_FB_CAPS_GET_FB_LEVEL:
	{
		XC_GET_FB_LEVEL xc_fb_level;

		if (MApi_XC_GetChipCaps(E_XC_FB_CAPS_GET_FB_LEVEL,
			(MS_U32 *)&xc_fb_level,
			sizeof(XC_GET_FB_LEVEL)) != TRUE)
			return -EPERM;

		v4l2_caps_info->u32 = xc_fb_level.eFBLevel;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_SW_DS:
	{
		XC_SWDS_SUPPORTED_CAPS xc_swds_caps;

		memset(&xc_swds_caps, 0, sizeof(XC_SWDS_SUPPORTED_CAPS));
		xc_swds_caps.eWindow = win;
		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_SW_DS,
			(MS_U32 *)&xc_swds_caps,
			sizeof(XC_SWDS_SUPPORTED_CAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = xc_swds_caps.bSupportSwDs;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_HDMI_SEAMLESS:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_HDMI_SEAMLESS,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_IS_SUPPORT_SUB_DISP:
	{
		XC_SUB_WINDOW_DISP_CAPS xc_sub_win_disp_cap;

		memset(&xc_sub_win_disp_cap, 0,
			sizeof(XC_SUB_WINDOW_DISP_CAPS));
		if (MApi_XC_GetChipCaps(E_XC_IS_SUPPORT_SUB_DISP,
			(MS_U32 *)&xc_sub_win_disp_cap,
			sizeof(XC_SUB_WINDOW_DISP_CAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = xc_sub_win_disp_cap.bSupported;

		break;
	}
	case V4L2_CAP_TYPE_HDR_HW_SUPPORT_HANDSHAKE_MODE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_HDR_HW_SUPPORT_HANDSHAKE_MODE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_PQ_DS:
	{
		XC_PQDS_SUPPORTED_CAPS xc_pqds_caps;

		xc_pqds_caps.enPQDSType =
			_g_caps[win].pqds_caps.pqds_type;
		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_PQ_DS,
			(MS_U32 *)&xc_pqds_caps,
			sizeof(XC_PQDS_SUPPORTED_CAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->pqds_caps.supported =
			xc_pqds_caps.bSupported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_HW_HSY:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_HW_HSY,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_HW_SR_VERSION:
	{
		MS_U32 u32;

		if (MApi_XC_GetChipCaps(E_XC_HW_SR_VERSION,
			&u32, sizeof(MS_U32)) != TRUE)
			return -EPERM;

		v4l2_caps_info->u32 = u32;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_MOD_HMIRROR:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_MOD_HMIRROR,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_HW_FAST_DISP:
	{
		XC_PQ_FAST_DISP_SUPPORTED_CAPS xc_fast_disp_caps;

		xc_fast_disp_caps.enPQFastDispType =
			_g_caps[win].fd_caps.fd_type;
		if (MApi_XC_GetChipCaps(E_XC_HW_FAST_DISP,
			(MS_U32 *)&xc_fast_disp_caps,
			sizeof(XC_PQ_FAST_DISP_SUPPORTED_CAPS)) != TRUE)
			return -EPERM;

		v4l2_caps_info->fd_caps.supported =
			xc_fast_disp_caps.bSupported;

		break;
	}
	case V4L2_CAP_TYPE_FB_CAPS_GET_FB_LEVEL_EX:
	{
		ST_XC_GET_FB_LEVEL_EX xc_fb_level_ex;

		xc_fb_level_ex.u32Version = ST_XC_GET_FBLEVEL_VERSION;
		xc_fb_level_ex.u32Length = sizeof(ST_XC_GET_FB_LEVEL_EX);
		if (MApi_XC_GetChipCaps(E_XC_FB_CAPS_GET_FB_LEVEL_EX,
			(MS_U32 *)&xc_fb_level_ex,
			sizeof(ST_XC_GET_FB_LEVEL_EX)) != TRUE)
			return -EPERM;

		v4l2_caps_info->u32 = xc_fb_level_ex.eFBLevel;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_ZNR:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_ZNR,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_ABF:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_ABF,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_FRC_HSK_MODE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_FRC_HSK_MODE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_SPF_LB_MODE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_SPF_LB_MODE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_HSE:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_HSE,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_HFR_PATH:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_HFR_PATH,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	case V4L2_CAP_TYPE_SUPPORT_PQ_DS_PLUS:
	{
		bool supported;

		if (MApi_XC_GetChipCaps(E_XC_SUPPORT_PQ_DS_PLUS,
			(MS_U32 *)&supported, sizeof(bool)) != TRUE)
			return -EPERM;

		v4l2_caps_info->support = supported;

		break;
	}
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "unknown cap type %d!\n",
			v4l2_caps_info->cap_type);
		return -EINVAL;
	}

	return 0;
}

static int _mtk_xc_get_frame_num_factor(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_frame_num_factor_info *v4l2_frame_num_factor_info =
		(struct v4l2_ext_ctrl_frame_num_factor_info *)ctrl;

	v4l2_frame_num_factor_info->frame_num_factor =
		MApi_XC_Get_FrameNumFactor(win);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc get frame num factor = %d\n",
		v4l2_frame_num_factor_info->frame_num_factor);

	return 0;
}

static int _mtk_xc_get_rw_bank(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_rw_bank_info *v4l2_rw_bank_info =
		(struct v4l2_ext_ctrl_rw_bank_info *)ctrl;

	v4l2_rw_bank_info->read_bank = MApi_XC_GetCurrentReadBank(win);
	v4l2_rw_bank_info->write_bank = MApi_XC_GetCurrentWriteBank(win);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc get rw bank: rb=%d, wb=%d\n",
		v4l2_rw_bank_info->read_bank, v4l2_rw_bank_info->write_bank);

	return 0;
}

// this api only used for preparing _mtk_xc_get_virtualbox
static int _mtk_xc_set_virtualbox(__u8 win, void *ctrl)
{
	struct v4l2_ext_ctrl_virtualbox_info *v4l2_virtualbox_info =
		(struct v4l2_ext_ctrl_virtualbox_info *)ctrl;

	_g_vb[win].max_drift_height =
		v4l2_virtualbox_info->max_drift_height;
	_g_vb[win].max_drift_width =
		v4l2_virtualbox_info->max_drift_width;

	return 0;
}

static int _mtk_xc_get_virtualbox(struct mtk_pq_device *pq_dev, void *ctrl)
{
	struct v4l2_ext_ctrl_virtualbox_info *v4l2_virtualbox_info =
		(struct v4l2_ext_ctrl_virtualbox_info *)ctrl;
	XC_VBOX_INFO xc_vbox_info;
	__u8 win = pq_dev->dev_indx;

	memset(&xc_vbox_info, 0, sizeof(XC_VBOX_INFO));
	xc_vbox_info.u32Version = ST_XC_VBOX_INFO_VERSION_PQDSPLUS;
	xc_vbox_info.u32Length = sizeof(XC_VBOX_INFO);
	xc_vbox_info.bInterface[win] =
		FIELD_TO_INTERLACE(pq_dev->common_info.timing_in.interlance);
	xc_vbox_info.u16Vfreq[win] = pq_dev->common_info.timing_in.v_freq;
	xc_vbox_info.u16SrcHSize[win] =
		_g_cap_win[win].cap_win.width;
	xc_vbox_info.u16SrcVSize[win] =
		_g_cap_win[win].cap_win.height;
	xc_vbox_info.enSourceType[win] = pq_dev->common_info.input_source;
	xc_vbox_info.u32MaxDriftHeight[win] =
		_g_vb[win].max_drift_height;
	xc_vbox_info.u32MaxDriftWidth[win] =
		_g_vb[win].max_drift_width;

	if (MApi_XC_Get_VirtualBox_Info(&xc_vbox_info) != TRUE)
		return -EPERM;

	v4l2_virtualbox_info->vbox_htotal = xc_vbox_info.u16VBox_Htotal[win];
	v4l2_virtualbox_info->vbox_vtotal = xc_vbox_info.u16VBox_Vtotal[win];

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"xc get virtualbox: vbox_htotal=%d, vbox_vtotal=%d\n",
		v4l2_virtualbox_info->vbox_htotal,
		v4l2_virtualbox_info->vbox_vtotal);

	return 0;
}

static int _mtk_xc_try_3dFormat(void *ctrl)
{
	struct v4l2_cus_try_3d_format cus_try_3d_mode;

	memset(&cus_try_3d_mode, 0, sizeof(struct v4l2_cus_try_3d_format));
	memcpy(&cus_try_3d_mode, ctrl, sizeof(struct v4l2_cus_try_3d_format));
	cus_try_3d_mode.bSupport3DFormat = MApi_XC_Is3DFormatSupported(
		cus_try_3d_mode.e3DInputMode, cus_try_3d_mode.e3DOutputMode);
	memcpy(ctrl, &cus_try_3d_mode, sizeof(struct v4l2_cus_try_3d_format));

	return 0;
}

static int _mtk_xc_set_mirror(__u8 win, enum set_mirror_type mirror_type,
	bool enable)
{
	int ret = 0;
	static bool h_mirror_update = FALSE;
	static bool v_mirror_update = FALSE;
	static bool h_mirror_enable = FALSE;
	static bool v_mirror_enable = FALSE;

	if (mirror_type == SET_MIRROR_TYPE_H) {
		h_mirror_update = TRUE;
		h_mirror_enable = enable;
	} else if (mirror_type == SET_MIRROR_TYPE_V) {
		v_mirror_update = TRUE;
		v_mirror_enable = enable;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown mirror type %d!\n", mirror_type);
		return -EINVAL;
	}

	if (h_mirror_update && v_mirror_update) {
		if (!h_mirror_enable && !v_mirror_enable)
			ret = MApi_XC_EnableMirrorModeEx(MIRROR_NORMAL, win);
		else if (h_mirror_enable && !v_mirror_enable)
			ret = MApi_XC_EnableMirrorModeEx(MIRROR_H_ONLY, win);
		else if (!h_mirror_enable && v_mirror_enable)
			ret = MApi_XC_EnableMirrorModeEx(MIRROR_V_ONLY, win);
		else
			ret = MApi_XC_EnableMirrorModeEx(MIRROR_HV, win);

		h_mirror_update = FALSE;
		v_mirror_update = FALSE;

		if (ret != TRUE) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"xc set mirror fail!\n");
			return -EPERM;
		}
	}

	return 0;
}

static int mtk_xc_memfmt_v4l22meta(u32 pixfmt_v4l2, u32 *memfmt_meta)
{
	if (WARN_ON(!memfmt_meta)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, memfmt_meta=0x%x\n", memfmt_meta);
		return -EINVAL;
	}

	switch (pixfmt_v4l2) {
	/* ------------------ RGB format ------------------ */
	/* 1 plane */
	case V4L2_PIX_FMT_RGB565:
		*memfmt_meta = MEM_FMT_RGB565;
		break;
	case V4L2_PIX_FMT_RGB888:
		*memfmt_meta = MEM_FMT_RGB888;
		break;
	case V4L2_PIX_FMT_RGB101010:
		*memfmt_meta = MEM_FMT_RGB101010;
		break;
	case V4L2_PIX_FMT_RGB121212:
		*memfmt_meta = MEM_FMT_RGB121212;
		break;
	case V4L2_PIX_FMT_ARGB8888:
		*memfmt_meta = MEM_FMT_ARGB8888;
		break;
	case V4L2_PIX_FMT_ABGR8888:
		*memfmt_meta = MEM_FMT_ABGR8888;
		break;
	case V4L2_PIX_FMT_RGBA8888:
		*memfmt_meta = MEM_FMT_RGBA8888;
		break;
	case V4L2_PIX_FMT_BGRA8888:
		*memfmt_meta = MEM_FMT_BGRA8888;
		break;
	case V4L2_PIX_FMT_ARGB2101010:
		*memfmt_meta = MEM_FMT_ARGB2101010;
		break;
	case V4L2_PIX_FMT_ABGR2101010:
		*memfmt_meta = MEM_FMT_ABGR2101010;
		break;
	case V4L2_PIX_FMT_RGBA1010102:
		*memfmt_meta = MEM_FMT_RGBA1010102;
		break;
	case V4L2_PIX_FMT_BGRA1010102:
		*memfmt_meta = MEM_FMT_BGRA1010102;
		break;
	/* 3 plane */
	/* ------------------ YUV format ------------------ */
	/* 1 plane */
	case V4L2_PIX_FMT_YUV444_VYU_10B:
		*memfmt_meta = MEM_FMT_YUV444_VYU_10B;
		break;
	case V4L2_PIX_FMT_YUV444_VYU_8B:
		*memfmt_meta = MEM_FMT_YUV444_VYU_8B;
		break;
	case V4L2_PIX_FMT_YVYU:
		*memfmt_meta = MEM_FMT_YUV422_YVYU_8B;
		break;
	/* 2 plane */
	case V4L2_PIX_FMT_YUV422_Y_UV_10B:
		*memfmt_meta = MEM_FMT_YUV422_Y_UV_10B;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_10B:
		*memfmt_meta = MEM_FMT_YUV422_Y_VU_10B;
		break;
	case V4L2_PIX_FMT_NV61:
		*memfmt_meta = MEM_FMT_YUV422_Y_VU_8B;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_8B_CE:
		*memfmt_meta = MEM_FMT_YUV422_Y_UV_8B_CE;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_8B_CE:
		*memfmt_meta = MEM_FMT_YUV422_Y_VU_8B_CE;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_8B_6B_CE:
		*memfmt_meta = MEM_FMT_YUV422_Y_UV_8B_6B_CE;
		break;
	case V4L2_PIX_FMT_YUV422_Y_UV_6B_CE:
		*memfmt_meta = MEM_FMT_YUV422_Y_UV_6B_CE;
		break;
	case V4L2_PIX_FMT_YUV422_Y_VU_6B_CE:
		*memfmt_meta = MEM_FMT_YUV422_Y_VU_6B_CE;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_10B:
		*memfmt_meta = MEM_FMT_YUV420_Y_UV_10B;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_10B:
		*memfmt_meta = MEM_FMT_YUV420_Y_VU_10B;
		break;
	case V4L2_PIX_FMT_NV21:
		*memfmt_meta = MEM_FMT_YUV420_Y_VU_8B;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_8B_CE:
		*memfmt_meta = MEM_FMT_YUV420_Y_UV_8B_CE;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_8B_CE:
		*memfmt_meta = MEM_FMT_YUV420_Y_VU_8B_CE;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_8B_6B_CE:
		*memfmt_meta = MEM_FMT_YUV420_Y_UV_8B_6B_CE;
		break;
	case V4L2_PIX_FMT_YUV420_Y_UV_6B_CE:
		*memfmt_meta = MEM_FMT_YUV420_Y_UV_6B_CE;
		break;
	case V4L2_PIX_FMT_YUV420_Y_VU_6B_CE:
		*memfmt_meta = MEM_FMT_YUV420_Y_VU_6B_CE;
		break;
	/* 3 plane */
	case V4L2_PIX_FMT_YUV444_Y_U_V_8B_CE:
		*memfmt_meta = MEM_FMT_YUV444_Y_U_V_8B_CE;
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown fmt = %d!!!\n", pixfmt_v4l2);
		return -EINVAL;

	}

	return 0;
}

int mtk_xc_set_hflip(__u8 win, bool enable)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc set hflip = %d\n", enable);

	return _mtk_xc_set_mirror(win, SET_MIRROR_TYPE_H, enable);
}

int mtk_xc_set_vflip(__u8 win, bool enable)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc set vflip = %d\n", enable);

	/* should remove in future */
	return _mtk_xc_set_mirror(win, SET_MIRROR_TYPE_V, enable);

	/* put vflip value to metadata */
}

int mtk_xc_get_hflip(__u8 win, bool *enable)
{
	MirrorMode_t xc_mirror_mode;

	xc_mirror_mode = MApi_XC_GetMirrorModeTypeEx(win);

	if (xc_mirror_mode == MIRROR_H_ONLY || xc_mirror_mode == MIRROR_HV)
		*enable = TRUE;
	else
		*enable = FALSE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc get hflip = %d\n", *enable);

	return 0;
}

int mtk_xc_get_vflip(__u8 win, bool *enable)
{
	MirrorMode_t xc_mirror_mode;

	xc_mirror_mode = MApi_XC_GetMirrorModeTypeEx(win);

	if (xc_mirror_mode == MIRROR_V_ONLY || xc_mirror_mode == MIRROR_HV)
		*enable = TRUE;
	else
		*enable = FALSE;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc get vflip = %d\n", *enable);

	return 0;
}

int mtk_xc_set_pix_format_in_mplane(
	struct v4l2_format *fmt,
	struct v4l2_xc_info *xc_info)
{
	if (WARN_ON(!fmt) || WARN_ON(!xc_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fmt=0x%x, xc_info=0x%x\n", fmt, xc_info);
		return -EINVAL;
	}

	/* MDW multi plane, save to share mem */
	xc_info->mdw.width = fmt->fmt.pix_mp.width;
	xc_info->mdw.height = fmt->fmt.pix_mp.height;
	xc_info->mdw.mem_fmt = fmt->fmt.pix_mp.pixelformat;
	xc_info->mdw.plane_num = fmt->fmt.pix_mp.num_planes;

	return 0;
}

int mtk_xc_set_pix_format_out_mplane(
	struct v4l2_format *fmt,
	struct v4l2_xc_info *xc_info)
{
	if (WARN_ON(!fmt) || WARN_ON(!xc_info)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, fmt=0x%x, xc_info=0x%x\n", fmt, xc_info);
		return -EINVAL;
	}

	/* IDR handle, save to share mem */
	xc_info->idr.width = fmt->fmt.pix_mp.width;
	xc_info->idr.height = fmt->fmt.pix_mp.height;
	xc_info->idr.mem_fmt = fmt->fmt.pix_mp.pixelformat;

	return 0;
}

int mtk_xc_cap_qbuf(struct mtk_pq_device *pq_dev, struct vb2_buffer *vb)
{
	int ret = 0;
	struct v4l2_xc_info *xc_info = NULL;
	struct pq_mdw_ctrl meta_mdw;
	//struct meta_buffer meta_buf;			/* meta related */
	//struct meta_header header;			/* meta related */
	struct v4l2_pq_buf mbuf[3] = {0};
	int plane_num = 0;
	int i = 0;

	if (WARN_ON(!pq_dev) || WARN_ON(!vb) || WARN_ON(!vb->planes)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x, vb=0x%x, vb->planes=0x%x\n",
			pq_dev, vb, vb->planes);
		return -EINVAL;
	}

	xc_info = &pq_dev->xc_info;

	memset(&meta_mdw, 0, sizeof(struct pq_mdw_ctrl));

	if (vb->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		if (WARN_ON(!vb->planes[0].m.userptr)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"ptr is NULL, vb->planes[0].m.userptr=0x%x\n",
				vb->planes[0].m.userptr);
			return -EINVAL;
		}

		/* MDW single plane */
		memcpy(&mbuf[0], vb->planes[0].m.userptr, sizeof(struct v4l2_pq_buf));

		plane_num = vb->num_planes;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"cap, i=%d, frame_fd=%d, meta_fd=%d, plane_num=%d\n",
			i, mbuf[i].frame_fd, mbuf[i].meta_fd, plane_num);
	} else {
		/* MDW multi plane */
		plane_num = vb->num_planes;

		for (i = 0; i < 3; ++i) {
			if (WARN_ON(!vb->planes[i].m.userptr)) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"ptr is NULL, vb->planes[%d].m.userptr=0x%x\n",
					i, vb->planes[i].m.userptr);
				return -EINVAL;
			}

			memcpy(&mbuf[i], vb->planes[i].m.userptr,
			       sizeof(struct v4l2_pq_buf));

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
				"cap_mp, i=%d, frame_fd=%d, meta_fd=%d, plane_num=%d\n",
				i, mbuf[i].frame_fd, mbuf[i].meta_fd, plane_num);
		}
	}

	/* get buf addr & size from fd */
	for (i = 0; i < plane_num; ++i) {
		ret = mtk_xc_get_buf_info(
			mbuf[i],
			&xc_info->mdw.frame_buf[i],
			&xc_info->mdw.meta_buf);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_xc_get_buf_info fail, ret=%d\n", ret);
			return ret;
		}
	}

	/* mapping pixfmt v4l2 -> meta */
	ret = mtk_xc_memfmt_v4l22meta(xc_info->mdw.mem_fmt, &meta_mdw.mem_fmt);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_xc_memfmt_v4l22meta fail, ret=%d\n", ret);
		return ret;
	}

	/* get meta info from share mem */
	memcpy(meta_mdw.frame_buf, xc_info->mdw.frame_buf,
		sizeof(meta_mdw.frame_buf));
	memcpy(&meta_mdw.meta_buf, &xc_info->mdw.meta_buf,
		sizeof(struct pq_buffer));
	meta_mdw.output_path = xc_info->mdw.output_path;	// TODO: cmap
	meta_mdw.width = xc_info->mdw.width;
	meta_mdw.height = xc_info->mdw.height;
	meta_mdw.v_align = xc_info->mdw.v_align;
	meta_mdw.h_align = xc_info->mdw.h_align;

#ifdef TODO_METADATA
	/* set meta buf info */
	meta_buf.paddr = xc_info->mdw.meta_buf.va;
	meta_buf.size = xc_info->mdw.meta_buf.size;

	strcpy(header.tag, "mdw_ctrl");
	header.ver = 0;
	header.size = sizeof(struct pq_mdw_ctrl);
	ret = attach_metadata(&meta_buf, header, &meta_mdw);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "attach_metadata fail, ret=%d\n", ret);
		return ret;
	}
#endif

	/* call CPU IF (window id)*/

	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

	return 0;
}

int mtk_xc_out_qbuf(struct mtk_pq_device *pq_dev, struct vb2_buffer *vb)
{
	int ret = 0;
	struct v4l2_xc_info *xc_info = NULL;
	struct pq_idr_ctrl meta_idr;
	//struct meta_buffer meta_buf;			/* meta related */
	//struct meta_header header;			/* meta related */
	struct v4l2_pq_buf mbuf[3] = {0};
	int plane_num = 0;
	int i = 0;

	if (WARN_ON(!pq_dev) || WARN_ON(!vb) || WARN_ON(!vb->planes)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x, vb=0x%x, vb->planes=0x%x\n",
			pq_dev, vb, vb->planes);
		return -EINVAL;
	}

	xc_info = &pq_dev->xc_info;

	memset(&meta_idr, 0, sizeof(struct pq_idr_ctrl));

	if (vb->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		if (WARN_ON(!vb->planes[0].m.userptr)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"ptr is NULL, vb->planes[0].m.userptr=0x%x\n",
				vb->planes[0].m.userptr);
			return -EINVAL;
		}
		memcpy(&mbuf[0], vb->planes[0].m.userptr, sizeof(struct v4l2_pq_buf));

		plane_num = vb->num_planes;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
			"out, i=%d, frame_fd=%d, meta_fd=%d, plane_num=%d\n",
			i, mbuf[i].frame_fd, mbuf[i].meta_fd, plane_num);
	} else { /* buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE */
		plane_num = vb->num_planes;

		for (i = 0; i < 3; ++i) {
			if (WARN_ON(!vb->planes[i].m.userptr)) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"ptr is NULL, vb->planes[%d].m.userptr=0x%x\n",
					i, vb->planes[i].m.userptr);
				return -EINVAL;
			}

			memcpy(&mbuf[i], vb->planes[i].m.userptr,
			       sizeof(struct v4l2_pq_buf));

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
				"out_mp, i=%d, frame_fd=%d, meta_fd=%d, plane_num=%d\n",
				i, mbuf[i].frame_fd, mbuf[i].meta_fd, plane_num);
		}
	}

	/* get buf addr & size from fd */
	for (i = 0; i < plane_num; ++i) {
		ret = mtk_xc_get_buf_info(
			mbuf[i],
			&xc_info->idr.frame_buf[i],
			&xc_info->idr.meta_buf);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_xc_get_buf_info fail, ret=%d\n", ret);
			return ret;
		}
	}

	/* mapping pixfmt v4l2 -> meta */
	ret = mtk_xc_memfmt_v4l22meta(xc_info->idr.mem_fmt, &meta_idr.mem_fmt);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_xc_memfmt_v4l22meta fail, ret=%d\n", ret);
		return ret;
	}

	/* get meta info from share mem */
	memcpy(meta_idr.frame_buf, xc_info->idr.frame_buf,
		sizeof(meta_idr.frame_buf));
	memcpy(&meta_idr.meta_buf, &xc_info->idr.meta_buf,
		sizeof(struct pq_buffer));
	meta_idr.width = xc_info->idr.width;
	meta_idr.height = xc_info->idr.height;
	meta_idr.index = xc_info->idr.index;
	meta_idr.crop = xc_info->idr.crop;

#ifdef TODO_METADATA
	/* set meta buf info */
	meta_buf.paddr = xc_info->idr.meta_buf.va;
	meta_buf.size = xc_info->idr.meta_buf.size;

	strcpy(header.tag, "idr_ctrl");
	header.ver = 0;
	header.size = sizeof(struct pq_idr_ctrl);
	ret = attach_metadata(&meta_buf, header, &meta_mdw);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "attach_metadata fail, ret=%d\n", ret);
		return ret;
	}
#endif

	/* call CPU IF (window id)*/

	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);

	return 0;
}

int mtk_xc_out_streamon(struct mtk_pq_device *pq_dev)
{
	if (WARN_ON(!pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x\n", pq_dev);
		return -EINVAL;
	}

	/* define default crop size if not set */
	if ((pq_dev->xc_info.idr.crop.c.left == 0)
		&& (pq_dev->xc_info.idr.crop.c.top == 0)
		&& (pq_dev->xc_info.idr.crop.c.width == 0)
		&& (pq_dev->xc_info.idr.crop.c.height == 0)) {
		pq_dev->xc_info.idr.crop.c.width = pq_dev->xc_info.idr.width;
		pq_dev->xc_info.idr.crop.c.height = pq_dev->xc_info.idr.height;
	}

	return 0;
}

int mtk_xc_out_streamoff(struct mtk_pq_device *pq_dev)
{
	if (WARN_ON(!pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x\n", pq_dev);
		return -EINVAL;
	}

	memset(&pq_dev->xc_info.idr, 0, sizeof(struct pq_idr_ctrl));

	return 0;
}

int mtk_xc_out_s_crop(struct mtk_pq_device *pq_dev, const struct v4l2_crop *crop)
{
	if (WARN_ON(!pq_dev) || WARN_ON(!crop)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x, crop=0x%x\n", pq_dev, crop);
		return -EINVAL;
	}

	pq_dev->xc_info.idr.crop = *crop;

	return 0;
}

int mtk_xc_out_g_crop(struct mtk_pq_device *pq_dev, struct v4l2_crop *crop)
{
	if (WARN_ON(!pq_dev) || WARN_ON(!crop)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"ptr is NULL, pq_dev=0x%x, crop=0x%x\n", pq_dev, crop);
		return -EINVAL;
	}

	*crop = pq_dev->xc_info.idr.crop;

	return 0;
}

static int _xc_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev = container_of(ctrl->handler,
		struct mtk_pq_device, xc_ctrl_handler);
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "id = %d\n", ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_3D_HW_VERSION:
		ret = _mtk_3d_get_hw_version(&(ctrl->val));
		break;
	case V4L2_CID_3D_LR_FRAME_EXCHG:
		ret = _mtk_3d_get_lr_frame_exchged(&(ctrl->val));
		break;
	case V4L2_CID_3D_HSHIFT:
		ret = _mtk_3d_get_hshift(&(ctrl->val));
		break;
	case V4L2_CID_3D_MODE:
		ret = _mtk_3d_get_mode(&(ctrl->val));
		break;
	case V4L2_CID_MISC:
		ret = _mtk_xc_get_misc((void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_CAP_WIN:
		ret = _mtk_xc_get_cap_win(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_CROP_WIN:
		ret = _mtk_xc_get_crop_win(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_DISP_WIN:
		ret = _mtk_xc_get_disp_win(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_PRE_SCALING:
		ret = _mtk_xc_get_pre_scaling(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_POST_SCALING:
		ret = _mtk_xc_get_post_scaling(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_DS_INFO:
		ret = _mtk_xc_get_ds_info(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_FREEZE:
		ret = _mtk_xc_get_freeze(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_FB_MODE:
		ret = _mtk_xc_get_fb_mode((void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_RWBANKAUTO_ENABLE:
		ret =  _mtk_xc_get_rwbankauto_enable(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_TESTPATTERN_ENABLE:	//only support set
		ret = -1;
		break;
	case V4L2_CID_SCALING_ENABLE:
		ret = _mtk_xc_get_scaling_enable(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_STATUS:
		ret = _mtk_xc_get_status(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_CAPS:
		ret = _mtk_xc_get_caps(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_FRAME_NUM_FACTOR:
		ret = _mtk_xc_get_frame_num_factor(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_RW_BANK:
		ret = _mtk_xc_get_rw_bank(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_VIRTUALBOX_INFO:
		ret = _mtk_xc_get_virtualbox(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _xc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev = container_of(ctrl->handler,
		struct mtk_pq_device, xc_ctrl_handler);
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "xc set ctrl: id=%d\n", ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_3D_LR_FRAME_EXCHG:
		ret = _mtk_3d_set_lr_frame_exchged(ctrl->val);
		break;
	case V4L2_CID_3D_HSHIFT:
		ret = _mtk_3d_set_hshift(ctrl->val);
		break;
	case V4L2_CID_3D_MODE:
		ret = _mtk_3d_set_mode((void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_MISC:
		ret = _mtk_xc_set_misc((void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_CAP_WIN:
		ret = _mtk_xc_set_cap_win(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_CROP_WIN:
		ret = _mtk_xc_set_crop_win(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_DISP_WIN:
		ret = _mtk_xc_set_disp_win(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_PRE_SCALING:
		ret = _mtk_xc_set_pre_scaling(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_POST_SCALING:
		ret = _mtk_xc_set_post_scaling(
			pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_DS_INFO:
		ret = _mtk_xc_set_ds_info(pq_dev, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_FREEZE:
		ret = _mtk_xc_set_freeze(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_FB_MODE:
		ret = _mtk_xc_set_fb_mode((void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_RWBANKAUTO_ENABLE:
		ret = _mtk_xc_set_rwbankauto_enable(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_TESTPATTERN_ENABLE:
		ret = _mtk_xc_set_testpattern_enable(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_SCALING_ENABLE:
		ret = _mtk_xc_set_scaling_enable(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_STATUS:
		ret = _mtk_xc_set_status(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_CAPS:
		ret = _mtk_xc_set_caps(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_VIRTUALBOX_INFO:
		ret = _mtk_xc_set_virtualbox(
			pq_dev->dev_indx, (void *)ctrl->p_new.p_u8);
		break;
	case V4L2_CID_PQ_BUFFER_REQ:
		ret = mtk_pq_buffer_allocate(pq_dev, ctrl->val);
		break;
	case V4L2_CID_PQ_SECURE_MODE:
#ifdef CONFIG_OPTEE
		ret = mtk_pq_set_secure_mode(pq_dev, ctrl->p_new.p_u8);
#endif
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "id = %d\n", ctrl->id);
		ret = -1;
		break;
	}

	return ret;
}

static int _xc_try_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = -1;

	struct mtk_pq_device *pq_dev = container_of(ctrl->handler,
		struct mtk_pq_device, xc_ctrl_handler);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "id = %d\n", ctrl->id);
	switch (ctrl->id) {
	case V4L2_CID_3D_FORMAT:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC, "id = %d\n", ctrl->id);
		ret = _mtk_xc_try_3dFormat((void *)ctrl->p_new.p);
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops xc_ctrl_ops = {
	.g_volatile_ctrl = _xc_g_ctrl,
	.s_ctrl = _xc_s_ctrl,
	.try_ctrl = _xc_try_ctrl,
};

static const struct v4l2_ctrl_config xc_ctrl[] = {
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_3D_HW_VERSION,
		.name = "3d hw version",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 0xff,
		.def = 0,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_3D_LR_FRAME_EXCHG,
		.name = "3d lr frame exchg",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = V4L2_3d_lr_frame_exchg_DISABLE,
		.max = V4L2_3d_lr_frame_exchg_ENABLE,
		.def = V4L2_3d_lr_frame_exchg_DISABLE,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_3D_FORMAT,
		.name = "try 3d format struct",
		.type = V4L2_CTRL_TYPE_U8,
		.min = 0,
		.max = 0xff,
		.def = 0,
		.step = 1,
		.dims = {sizeof(struct v4l2_cus_try_3d_format)},
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_3D_HSHIFT,
		.name = "3d hshift",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 0xff,
		.def = 0,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_3D_MODE,
		.name = "3d input&output mode",
		.type = V4L2_CTRL_TYPE_U8,
		.min = 0,
		.max = 0xff,
		.def = 0,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_3d_mode)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_MISC,
		.name = "xc misc",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_misc_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_CAP_WIN,
		.name = "xc cap window",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_cap_win_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_CROP_WIN,
		.name = "xc crop window",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_crop_win_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_DISP_WIN,
		.name = "xc disp window",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_disp_win_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_PRE_SCALING,
		.name = "xc pre-scaling",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_scaling_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_POST_SCALING,
		.name = "xc post-scaling",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_scaling_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_DS_INFO,
		.name = "xc ds info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_ds_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_FREEZE,
		.name = "xc freeze",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_freeze_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_FB_MODE,
		.name = "xc fb mode",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_fb_mode_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_RWBANKAUTO_ENABLE,
		.name = "xc rwbankauto enable",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_rwbankauto_enable_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_TESTPATTERN_ENABLE,
		.name = "xc testpattern enable",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_testpattern_enable_info)},
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_SCALING_ENABLE,
		.name = "xc scaling enable",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_scaling_enable_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_STATUS,
		.name = "xc status",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_status_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_CAPS,
		.name = "xc caps",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_caps_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_FRAME_NUM_FACTOR,
		.name = "xc frame num factor",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_frame_num_factor_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_RW_BANK,
		.name = "xc rw bank",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_rw_bank_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_VIRTUALBOX_INFO,
		.name = "xc virtualbox info",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_ext_ctrl_virtualbox_info)},
		.flags = V4L2_CTRL_FLAG_VOLATILE |
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &xc_ctrl_ops,
		.id = V4L2_CID_PQ_BUFFER_REQ,
		.name = "xc request pq buffer",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 0xFF,
		.def = 0,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
	 .ops = &xc_ctrl_ops,
	 .id = V4L2_CID_PQ_SECURE_MODE,
	 .name = "xc secure mode",
	 .type = V4L2_CTRL_TYPE_U8,
	 .def = 0,
	 .min = 0,
	 .max = 0xff,
	 .step = 1,
	 .flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	 .dims = {sizeof(u8)},
	 },

};

/* subdev operations */
static const struct v4l2_subdev_ops xc_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops xc_sd_internal_ops = {
};

//EVENT
static void _mtk_xc_queue_event_ipvs(SC_INT_SRC enIntNum, void *data)
{
	struct mtk_pq_device *pq_dev = (struct mtk_pq_device *)data;
	struct v4l2_event event;

	if (video_is_registered(&pq_dev->video_dev)) {
		event.type = V4L2_EVENT_VSYNC;
		v4l2_event_queue(&pq_dev->video_dev, &event);
	}
}

int mtk_xc_subscribe_event(__u32 event_type, __u16 *dev_id)
{
	struct mtk_pq_device *pq_dev =
		container_of(dev_id, struct mtk_pq_device, dev_indx);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"subscribe event type = %d\n", event_type);

	switch (event_type) {
	case V4L2_EVENT_VSYNC:
		MApi_XC_InterruptAttach(SC_INT_F2_IPVS_SW_TRIG,
			_mtk_xc_queue_event_ipvs, pq_dev);
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown event type = %d!\n", event_type);
		return -EINVAL;
	}

	return 0;
}

int mtk_xc_unsubscribe_event(__u32 event_type, __u16 *dev_id)
{
	struct mtk_pq_device *pq_dev =
		container_of(dev_id, struct mtk_pq_device, dev_indx);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_XC,
		"unsubscribe event type = %d\n", event_type);

	switch (event_type) {
	case V4L2_EVENT_VSYNC:
		MApi_XC_InterruptDeAttach(SC_INT_F2_IPVS_SW_TRIG,
			_mtk_xc_queue_event_ipvs, pq_dev);
		break;
	case V4L2_EVENT_ALL:
		MApi_XC_InterruptDeAttach(SC_INT_F2_IPVS_SW_TRIG,
			_mtk_xc_queue_event_ipvs, pq_dev);
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"unknown event type = %d!\n", event_type);
		return -EINVAL;
	}

	return 0;
}

int mtk_pq_register_xc_subdev(struct v4l2_device *pq_dev,
	struct v4l2_subdev *subdev_xc,
	struct v4l2_ctrl_handler *xc_ctrl_handler)
{
		int ret = 0;
		__u32 ctrl_count;
		__u32 ctrl_num = sizeof(xc_ctrl)/
				sizeof(struct v4l2_ctrl_config);
		struct mtk_pq_device *mtk_pq_dev =
			container_of(subdev_xc,
				struct mtk_pq_device, subdev_xc);

		v4l2_ctrl_handler_init(xc_ctrl_handler, ctrl_num);
		for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++)
			v4l2_ctrl_new_custom(xc_ctrl_handler,
				&xc_ctrl[ctrl_count], NULL);

		ret = xc_ctrl_handler->error;
		if (ret) {
			v4l2_err(pq_dev, "failed to create xc ctrl handler\n");
			goto exit;
		}
		subdev_xc->ctrl_handler = xc_ctrl_handler;

		v4l2_subdev_init(subdev_xc, &xc_sd_ops);
		subdev_xc->internal_ops = &xc_sd_internal_ops;
		strlcpy(subdev_xc->name, "mtk-xc", sizeof(subdev_xc->name));

		ret = v4l2_device_register_subdev(pq_dev, subdev_xc);
		if (ret) {
			v4l2_err(pq_dev, "failed to register xc subdev\n");
			goto exit;
		}

		//read dts
		ret = _mtk_xc_read_dts(&mtk_pq_dev->xc_info);
		if (ret)
			goto exit;

		//xc init
		ret = _mtk_xc_set_init(&mtk_pq_dev->xc_info);
		if (ret)
			goto exit;

		//config autodownload (should be move to script DD)
		ret = _mtk_xc_config_autodownload(&mtk_pq_dev->xc_info);
		if (ret)
			goto exit;

		ret = mtk_pq_buffer_win_init(&mtk_pq_dev->xc_info);
		if (ret)
			goto exit;

		return 0;

exit:
		v4l2_ctrl_handler_free(xc_ctrl_handler);
		return ret;
}

void mtk_pq_unregister_xc_subdev(struct v4l2_subdev *subdev_xc)
{
	struct mtk_pq_device *mtk_pq_dev =
		container_of(subdev_xc, struct mtk_pq_device, subdev_xc);

	_mtk_xc_set_exit();
	mtk_pq_buffer_win_exit(&mtk_pq_dev->xc_info);

	v4l2_ctrl_handler_free(subdev_xc->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_xc);
}

