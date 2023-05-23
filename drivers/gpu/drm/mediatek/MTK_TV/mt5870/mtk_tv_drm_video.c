// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_plane.h"
#include "mtk_tv_drm_gem.h"
#include "mtk_tv_drm_drv.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_video.h"
#include "mtk_tv_drm_encoder.h"
#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_video_panel.h"
#include "mtk_tv_drm_common.h"

#include "apiXC.h"
#include "chip_int.h"

__u32 get_dts_u32(struct device_node *np, const char *name)
{
	int u32Tmp = 0x0;
	int ret;

	ret = of_property_read_u32(np, name, &u32Tmp);
	if (ret != 0x0) {
		DRM_INFO("[%s, %d]: read u32 failed, name = %s\n",
			__func__, __LINE__, name);
	}

	return u32Tmp;
}

__u32 get_dts_u32_index(struct device_node *np, const char *name, int index)
{
	int u32Tmp = 0x0;
	int ret;

	ret = of_property_read_u32_index(np, name, index, &u32Tmp);
	if (ret != 0x0) {
		DRM_INFO("[%s, %d]: read u32 index failed, name = %s\n",
			__func__, __LINE__, name);
	}

	return u32Tmp;
}

static inline int64_t positive2negative(int64_t val)
{
	return (~(val)+1);
}

static void dump_dts_info(struct mtk_panel_context *pctx_pnl)
{
	int FRCType = 0;

	DRM_INFO("m_PanelName = %s\n", pctx_pnl->info.pnlname);
	DRM_INFO("m_ePanelLinkType = %d\n", pctx_pnl->info.linktype);
	DRM_INFO("m_bPanelSwapPort = %d\n", pctx_pnl->info.swap_port);
	DRM_INFO("m_bPanelSwapOdd_ML = %d\n", pctx_pnl->info.swapodd_ml);
	DRM_INFO("m_bPanelSwapEven_ML = %d\n", pctx_pnl->info.swapeven_ml);
	DRM_INFO("m_bPanelSwapOdd_RB = %d\n", pctx_pnl->info.swapodd_rb);
	DRM_INFO("m_bPanelSwapEven_RB = %d\n", pctx_pnl->info.swapeven_rb);
	DRM_INFO("m_bPanelSwapLVDS_POL = %d\n", pctx_pnl->info.swaplvds_pol);
	DRM_INFO("m_bPanelSwapLVDS_CH = %d\n", pctx_pnl->info.swaplvds_ch);
	DRM_INFO("m_bPanelPDP10BIT = %d\n", pctx_pnl->info.pdp_10bit);
	DRM_INFO("m_bPanelLVDS_TI_MODE = %d\n", pctx_pnl->info.lvds_ti_mode);
	DRM_INFO("m_ucPanelDCLKDelay = %d\n", pctx_pnl->info.dclk_dely);
	DRM_INFO("m_bPanelInvDCLK = %d\n", pctx_pnl->info.inv_dclk);
	DRM_INFO("m_bPanelInvDE = %d\n", pctx_pnl->info.inv_de);
	DRM_INFO("m_bPanelInvHSync = %d\n", pctx_pnl->info.inv_hsync);
	DRM_INFO("m_bPanelInvVSync = %d\n", pctx_pnl->info.inv_vsync);
	DRM_INFO("m_wPanelOnTiming1 = %d\n", pctx_pnl->info.ontiming_1);
	DRM_INFO("m_wPanelOnTiming2 = %d\n", pctx_pnl->info.ontiming_2);
	DRM_INFO("m_wPanelOffTiming1 = %d\n", pctx_pnl->info.offtiming_1);
	DRM_INFO("m_wPanelOffTiming2 = %d\n", pctx_pnl->info.offtiming_2);
	DRM_INFO("m_ucPanelHSyncWidth = %d\n", pctx_pnl->info.hsync_w);
	DRM_INFO("m_ucPanelHSyncBackPorch = %d\n", pctx_pnl->info.hbp);
	DRM_INFO("m_ucPanelVSyncWidth = %d\n", pctx_pnl->info.vsync_w);
	DRM_INFO("m_ucPanelVBackPorch = %d\n", pctx_pnl->info.vbp);
	DRM_INFO("m_wPanelHStart = %d\n", pctx_pnl->info.de_hstart);
	DRM_INFO("m_wPanelVStart = %d\n", pctx_pnl->info.de_vstart);
	DRM_INFO("m_wPanelWidth = %d\n", pctx_pnl->info.de_width);
	DRM_INFO("m_wPanelHeight = %d\n", pctx_pnl->info.de_height);
	DRM_INFO("m_wPanelMaxHTotal = %d\n", pctx_pnl->info.max_htt);
	DRM_INFO("m_wPanelHTotal = %d\n", pctx_pnl->info.typ_htt);
	DRM_INFO("m_wPanelMinHTotal = %d\n", pctx_pnl->info.min_htt);
	DRM_INFO("m_wPanelMaxVTotal = %d\n", pctx_pnl->info.max_vtt);
	DRM_INFO("m_wPanelVTotal = %d\n", pctx_pnl->info.typ_vtt);
	DRM_INFO("m_wPanelMinVTotal = %d\n", pctx_pnl->info.min_vtt);
	DRM_INFO("m_dwPanelMaxDCLK = %d\n", pctx_pnl->info.max_dclk);
	DRM_INFO("m_dwPanelDCLK = %d\n", pctx_pnl->info.typ_dclk);
	DRM_INFO("m_dwPanelMinDCLK = %d\n", pctx_pnl->info.min_dclk);
	DRM_INFO("m_ucTiBitMode = %d\n", pctx_pnl->info.ti_bitmode);
	DRM_INFO("m_ucOutputFormatBitMode = %d\n", pctx_pnl->info.op_bitmode);
	DRM_INFO("m_u16LVDSTxSwapValue = %d\n", pctx_pnl->info.lvds_txswap);
	DRM_INFO("m_bPanelSwapOdd_RG = %d\n", pctx_pnl->info.swapodd_rg);
	DRM_INFO("m_bPanelSwapEven_RG = %d\n", pctx_pnl->info.swapeven_rg);
	DRM_INFO("m_bPanelSwapOdd_GB = %d\n", pctx_pnl->info.swapodd_gb);
	DRM_INFO("m_bPanelSwapEven_GB = %d\n", pctx_pnl->info.swapeven_gb);
	DRM_INFO("m_bPanelNoiseDith = %d\n", pctx_pnl->info.noise_dith);
	DRM_INFO("u32PeriodPWM = %d\n", pctx_pnl->info.period_pwm);
	DRM_INFO("u32DutyPWM = %d\n", pctx_pnl->info.duty_pwm);
	DRM_INFO("u16DivPWM = %d\n", pctx_pnl->info.div_pwm);
	DRM_INFO("bPolPWM = %d\n", pctx_pnl->info.pol_pwm);
	DRM_INFO("u16MaxPWMvalue = %d\n", pctx_pnl->info.max_pwm_val);
	DRM_INFO("u16TypPWMvalue = %d\n", pctx_pnl->info.typ_pwm_val);
	DRM_INFO("u16MinPWMvalue = %d\n", pctx_pnl->info.min_pwm_val);
	DRM_INFO("u16PWMPort = %d\n", pctx_pnl->info.pwmport);
	DRM_INFO("bVRR_en = %d\n", pctx_pnl->info.vrr_en);
	DRM_INFO("u16VRRMaxRate = %d\n", pctx_pnl->info.vrr_max_v);
	DRM_INFO("u16VRRMinRate = %d\n", pctx_pnl->info.vrr_min_v);

	//pnl_cus_setting
	DRM_INFO("PanelGammaBinPath = %s\n", pctx_pnl->cus.pgamma_path);
	DRM_INFO("TCON bin path = %s\n", pctx_pnl->cus.tcon_path);
	DRM_INFO("bPanelGammaEnable = %d\n", pctx_pnl->cus.pgamma_en);
	DRM_INFO("bTCONBinEnable = %d\n", pctx_pnl->cus.tcon_en);
	DRM_INFO("enPanelMirrorMode = %d\n", pctx_pnl->cus.mirror_mode);
	DRM_INFO("enPanelOutputFormat = %d\n", pctx_pnl->cus.op_format);
	DRM_INFO("bSSCEnable = %d\n", pctx_pnl->cus.ssc_en);
	DRM_INFO("u16SSC_Step = %d\n", pctx_pnl->cus.ssc_step);
	DRM_INFO("u16SSC_Span = %d\n", pctx_pnl->cus.ssc_span);
	DRM_INFO("bOverDriveEnable = %d\n", pctx_pnl->cus.od_en);
	DRM_INFO("u64OverDrive_Buf_Addr = %llx\n", pctx_pnl->cus.od_buf_addr);
	DRM_INFO("u32OverDrive_Buf_Size = %lu\n", pctx_pnl->cus.od_buf_size);
	DRM_INFO("u16GPIO_Backlight = %d\n", pctx_pnl->cus.gpio_bl);
	DRM_INFO("u16GPIO_VCC = %d\n", pctx_pnl->cus.gpio_vcc);
	DRM_INFO("u16M_delta = %d\n", pctx_pnl->cus.m_del);

	DRM_INFO("u16CustomerColorPrimaries = %d\n", pctx_pnl->cus.cus_color_prim);
	DRM_INFO("u16SourceWx = %d\n", pctx_pnl->cus.src_wx);
	DRM_INFO("u16SourceWy = %d\n", pctx_pnl->cus.src_wy);
	DRM_INFO("enVideoGFXDispMode = %d\n", pctx_pnl->cus.disp_mode);
	DRM_INFO("u16OSD_Height = %d\n", pctx_pnl->cus.osd_height);
	DRM_INFO("u16OSD_Width = %d\n", pctx_pnl->cus.osd_width);
	DRM_INFO("u16OSD_HsyncStart = %d\n", pctx_pnl->cus.osd_hs_st);
	DRM_INFO("u16OSD_HsyncEnd = %d\n", pctx_pnl->cus.osd_hs_end);
	DRM_INFO("u16OSD_HDEStart = %d\n", pctx_pnl->cus.osd_hde_st);
	DRM_INFO("u16OSD_HDEEnd = %d\n", pctx_pnl->cus.osd_hde_end);
	DRM_INFO("u16OSD_HTotal = %d\n", pctx_pnl->cus.osd_htt);
	DRM_INFO("u16OSD_VsyncStart = %d\n", pctx_pnl->cus.osd_vs_st);
	DRM_INFO("u16OSD_VsyncEnd = %d\n", pctx_pnl->cus.osd_vs_end);
	DRM_INFO("u16OSD_VDEStart = %d\n", pctx_pnl->cus.osd_vde_st);
	DRM_INFO("u16OSD_VDEEnd = %d\n", pctx_pnl->cus.osd_vde_end);
	DRM_INFO("u16OSD_VTotal = %d\n", pctx_pnl->cus.osd_vtt);

	//swing level control
	DRM_INFO("bSwingCtrlEn = %d\n", pctx_pnl->swing.en);
	DRM_INFO("u16SwingCtrlChs = %d\n", pctx_pnl->swing.ctrl_chs);
	DRM_INFO("u32Swing_Ch3_0 = %d\n", pctx_pnl->swing.ch3_0);
	DRM_INFO("u32Swing_Ch7_4 = %d\n", pctx_pnl->swing.ch7_4);
	DRM_INFO("u32Swing_Ch11_8 = %d\n", pctx_pnl->swing.ch11_8);
	DRM_INFO("u32Swing_Ch15_12 = %d\n", pctx_pnl->swing.ch15_12);
	DRM_INFO("u32Swing_Ch19_16 = %d\n", pctx_pnl->swing.ch19_16);
	DRM_INFO("u32Swing_Ch23_20 = %d\n", pctx_pnl->swing.ch23_20);
	DRM_INFO("u32Swing_Ch27_24 = %d\n", pctx_pnl->swing.ch27_24);
	DRM_INFO("u32Swing_Ch31_28 = %d\n", pctx_pnl->swing.ch31_28);

	// PE control
	DRM_INFO("bPECtrlEn = %d\n", pctx_pnl->pe.en);
	DRM_INFO("u16PECtrlChs = %d\n", pctx_pnl->pe.ctrl_chs);
	DRM_INFO("u32PE_Ch3_0 = %d\n", pctx_pnl->pe.ch3_0);
	DRM_INFO("u32PE_Ch7_4 = %d\n", pctx_pnl->pe.ch7_4);
	DRM_INFO("u32PE_Ch11_8 = %d\n", pctx_pnl->pe.ch11_8);
	DRM_INFO("u32PE_Ch15_12 = %d\n", pctx_pnl->pe.ch15_12);
	DRM_INFO("u32PE_Ch19_16 = %d\n", pctx_pnl->pe.ch19_16);
	DRM_INFO("u32PE_Ch23_20 = %d\n", pctx_pnl->pe.ch23_20);
	DRM_INFO("u32PE_Ch27_24 = %d\n", pctx_pnl->pe.ch27_24);
	DRM_INFO("u32PE_Ch31_28 = %d\n", pctx_pnl->pe.ch31_28);

	//lane order control
	//DRM_INFO("u16LaneOrder_Ch3_0 = %d\n", pctx_pnl->lane_order.ch3_0);
	//DRM_INFO("u16LaneOrder_Ch7_4 = %d\n", pctx_pnl->lane_order.ch7_4);
	//DRM_INFO("u16LaneOrder_Ch11_8 = %d\n", pctx_pnl->lane_order.ch11_8);
	//DRM_INFO("u16LaneOrder_Ch15_12 = %d\n", pctx_pnl->lane_order.ch15_12);
	//DRM_INFO("u16LaneOrder_Ch19_16 = %d\n", pctx_pnl->lane_order.ch19_16);
	//DRM_INFO("u16LaneOrder_Ch23_20 = %d\n", pctx_pnl->lane_order.ch23_20);
	//DRM_INFO("u16LaneOrder_Ch27_24 = %d\n", pctx_pnl->lane_order.ch27_24);
	//DRM_INFO("u16LaneOrder_Ch31_28 = %d\n", pctx_pnl->lane_order.ch31_28);

	//output config
	DRM_INFO("u16OutputCfg_Ch3_0 = %d\n", pctx_pnl->op_cfg.ch3_0);
	DRM_INFO("u16OutputCfg_Ch7_4 = %d\n", pctx_pnl->op_cfg.ch7_4);
	DRM_INFO("u16OutputCfg_Ch11_8 = %d\n", pctx_pnl->op_cfg.ch11_8);
	DRM_INFO("u16OutputCfg_Ch15_12 = %d\n", pctx_pnl->op_cfg.ch15_12);
	DRM_INFO("u16OutputCfg_Ch19_16 = %d\n", pctx_pnl->op_cfg.ch19_16);
	DRM_INFO("u16OutputCfg_Ch23_20 = %d\n", pctx_pnl->op_cfg.ch23_20);
	DRM_INFO("u16OutputCfg_Ch27_24 = %d\n", pctx_pnl->op_cfg.ch27_24);
	DRM_INFO("u16OutputCfg_Ch31_28 = %d\n", pctx_pnl->op_cfg.ch31_28);

	DRM_INFO("Color_Space = %d\n", pctx_pnl->color_info.space);
	DRM_INFO("Color_Format = %d\n", pctx_pnl->color_info.format);
	DRM_INFO("Color_Range = %d\n", pctx_pnl->color_info.range);
	DRM_INFO("Rx = %d\n", pctx_pnl->color_info.rx);
	DRM_INFO("Ry = %d\n", pctx_pnl->color_info.ry);
	DRM_INFO("Gx = %d\n", pctx_pnl->color_info.gx);
	DRM_INFO("Gy = %d\n", pctx_pnl->color_info.gy);
	DRM_INFO("Bx = %d\n", pctx_pnl->color_info.bx);
	DRM_INFO("By = %d\n", pctx_pnl->color_info.by);
	DRM_INFO("Wx = %d\n", pctx_pnl->color_info.wx);
	DRM_INFO("Wy = %d\n", pctx_pnl->color_info.wy);
	DRM_INFO("MaxLuminance = %d\n", pctx_pnl->color_info.maxlum);
	DRM_INFO("MedLuminance = %d\n", pctx_pnl->color_info.medlum);
	DRM_INFO("MinLuminance = %d\n", pctx_pnl->color_info.minlum);
	DRM_INFO("LinearRGB = %d\n", pctx_pnl->color_info.linear_rgb);
	DRM_INFO("HDRPanelNits = %d\n", pctx_pnl->color_info.hdrnits);

	//FRC table
	for (FRCType = 0;
		FRCType < (sizeof(pctx_pnl->frc_info)/sizeof(struct drm_st_pnl_frc_table_info));
		FRCType++) {
		DRM_INFO("index = %d, u16LowerBound = %d\n",
			FRCType,
			pctx_pnl->frc_info[FRCType].u16LowerBound);
		DRM_INFO("index = %d, u16HigherBound = %d\n",
			FRCType,
			pctx_pnl->frc_info[FRCType].u16HigherBound);
		DRM_INFO("index = %d, u8FRC_in = %d\n",
			FRCType,
			pctx_pnl->frc_info[FRCType].u8FRC_in);
		DRM_INFO("index = %d, u8FRC_out = %d\n",
			FRCType,
			pctx_pnl->frc_info[FRCType].u8FRC_out);
	}
}



int readDTB2PanelPrivate(struct mtk_panel_context *pctx_pnl)
{
	int ret, FRClen = 0, FRCType = 0;
	struct device_node *np;
	int u32Tmp = 0xFF;
	const char *name;
	//uint8_t szPanelName[MTK_DRM_NAME_MAX_NUM];
	const char *pnlname;
	const char *tcon_pth;
	const char *pgamma_pth;

	//Get panel info
	np = of_find_node_by_name(NULL, PNL_INFO_NODE);
	if (np != NULL) {
		name = NAME_TAG;
		ret = of_property_read_string(np, name, &pnlname);
		if (ret != 0x0) {
			DRM_INFO("[%s, %d]: read string failed, name = %s\n",
				__func__, __LINE__, name);
			return ret;
		}
		memset(pctx_pnl->info.pnlname, 0, DRM_NAME_MAX_NUM);
		strncpy(pctx_pnl->info.pnlname, pnlname, DRM_NAME_MAX_NUM - 1);

		pctx_pnl->info.linktype = get_dts_u32(np, LINK_TYPE_TAG);
		pctx_pnl->info.swap_port = get_dts_u32(np, SWAP_PORT_TAG);
		pctx_pnl->info.swapodd_ml = get_dts_u32(np, SWAP_ODD_TAG);
		pctx_pnl->info.swapeven_ml = get_dts_u32(np, SWAP_EVEN_TAG);
		pctx_pnl->info.swapodd_rb = get_dts_u32(np, SWAP_ODD_RB_TAG);
		pctx_pnl->info.swapeven_rb = get_dts_u32(np, SWAP_EVEN_RB_TAG);
		pctx_pnl->info.swaplvds_pol = get_dts_u32(np, SWAP_LVDS_POL_TAG);
		pctx_pnl->info.swaplvds_ch = get_dts_u32(np, SWAP_LVDS_CH_TAG);
		pctx_pnl->info.pdp_10bit = get_dts_u32(np, PDP_10BIT_TAG);
		pctx_pnl->info.lvds_ti_mode = get_dts_u32(np, LVDS_TI_MODE_TAG);
		pctx_pnl->info.dclk_dely = get_dts_u32(np, DCLK_DELAY_TAG);
		pctx_pnl->info.inv_dclk = get_dts_u32(np, INV_DCLK_TAG);
		pctx_pnl->info.inv_de = get_dts_u32(np, INV_DE_TAG);
		pctx_pnl->info.inv_hsync = get_dts_u32(np, INV_HSYNC_TAG);
		pctx_pnl->info.inv_vsync = get_dts_u32(np, INV_VSYNC_TAG);
		pctx_pnl->info.ontiming_1 = get_dts_u32(np, ON_TIMING_1_TAG);
		pctx_pnl->info.ontiming_2 = get_dts_u32(np, ON_TIMING_2_TAG);
		pctx_pnl->info.offtiming_1 = get_dts_u32(np, OFF_TIMING_1_TAG);
		pctx_pnl->info.offtiming_2 = get_dts_u32(np, OFF_TIMING_2_TAG);
		pctx_pnl->info.hsync_w = get_dts_u32(np, HSYNC_WIDTH_TAG);
		pctx_pnl->info.hbp = get_dts_u32(np, HSYNC_BACK_PORCH_TAG);
		pctx_pnl->info.vsync_w = get_dts_u32(np, VSYNC_WIDTH_TAG);
		pctx_pnl->info.vbp = get_dts_u32(np, VSYNC_BACK_PORCH_TAG);
		pctx_pnl->info.de_hstart = get_dts_u32(np, HSTART_TAG);
		pctx_pnl->info.de_vstart = get_dts_u32(np, VSTART_TAG);
		pctx_pnl->info.de_width = get_dts_u32(np, WIDTH_TAG);
		pctx_pnl->info.de_height = get_dts_u32(np, HEIGHT_TAG);
		pctx_pnl->info.max_htt = get_dts_u32(np, MAX_HTOTAL_TAG);
		pctx_pnl->info.typ_htt = get_dts_u32(np, HTOTAL_TAG);
		pctx_pnl->info.min_htt = get_dts_u32(np, MIN_HTOTAL_TAG);
		pctx_pnl->info.max_vtt = get_dts_u32(np, MAX_VTOTAL_TAG);
		pctx_pnl->info.typ_vtt = get_dts_u32(np, VTOTAL_TAG);
		pctx_pnl->info.min_vtt = get_dts_u32(np, MIN_VTOTAL_TAG);
		pctx_pnl->info.max_dclk = get_dts_u32(np, MAX_DCLK_TAG);
		pctx_pnl->info.typ_dclk = get_dts_u32(np, DCLK_TAG);
		pctx_pnl->info.min_dclk = get_dts_u32(np, MIN_DCLK_TAG);
		pctx_pnl->info.ti_bitmode = get_dts_u32(np, TI_BIT_MODE_TAG);
		pctx_pnl->info.op_bitmode = get_dts_u32(np, OUTPUT_BIT_MODE_TAG);
		pctx_pnl->info.lvds_txswap = get_dts_u32(np, LVDS_TX_SWAP_TAG);
		pctx_pnl->info.swapodd_rg = get_dts_u32(np, SWAP_ODD_RG_TAG);
		pctx_pnl->info.swapeven_rg = get_dts_u32(np, SWAP_EVEN_RG_TAG);
		pctx_pnl->info.swapodd_gb = get_dts_u32(np, SWAP_ODD_GB_TAG);
		pctx_pnl->info.swapeven_gb = get_dts_u32(np, SWAP_EVEN_GB_TAG);
		pctx_pnl->info.noise_dith = get_dts_u32(np, NOISE_DITH_TAG);

		pctx_pnl->info.period_pwm = get_dts_u32(np, PERIOD_PWM_TAG);
		pctx_pnl->info.duty_pwm = get_dts_u32(np, DUTY_PWM_TAG);
		pctx_pnl->info.div_pwm = get_dts_u32(np, DIV_PWM_TAG);
		pctx_pnl->info.pol_pwm = get_dts_u32(np, POL_PWM_TAG);
		pctx_pnl->info.max_pwm_val = get_dts_u32(np, MAX_PWM_VALUE_TAG);
		pctx_pnl->info.min_pwm_val = get_dts_u32(np, MIN_PWM_VALUE_TAG);
		pctx_pnl->info.pwmport = get_dts_u32(np, PWM_PORT_TAG);
		pctx_pnl->info.vrr_en = get_dts_u32(np, VRR_EN_TAG);
		pctx_pnl->info.vrr_max_v = get_dts_u32(np, VRR_MAX_TAG);
		pctx_pnl->info.vrr_min_v = get_dts_u32(np, VRR_MIN_TAG);

	}

	//Get Board ini related info
	np = of_find_node_by_name(NULL, PNL_CUS_SETTING_NODE);
	if (np != NULL) {

		ret = of_property_read_string(np, GAMMA_BIN_TAG, &pgamma_pth);
		if (ret != 0x0) {
			DRM_INFO("[%s, %d]: read string failed, name = %s\n",
				__func__, __LINE__, GAMMA_BIN_TAG);
			return ret;
		}
		memset(pctx_pnl->cus.pgamma_path, 0, FILE_PATH_LEN);
		strncpy(pctx_pnl->cus.pgamma_path, pgamma_pth, FILE_PATH_LEN - 1);

		ret = of_property_read_string(np, GAMMA_BIN_TAG, &tcon_pth);
		if (ret != 0x0) {
			DRM_INFO("[%s, %d]: read string failed, name = %s\n",
				__func__, __LINE__, GAMMA_BIN_TAG);
			return ret;
		}
		memset(pctx_pnl->cus.tcon_path, 0, FILE_PATH_LEN);
		strncpy(pctx_pnl->cus.tcon_path, tcon_pth, FILE_PATH_LEN - 1);

		pctx_pnl->cus.pgamma_en = get_dts_u32(np, GAMMA_EN_TAG);
		pctx_pnl->cus.tcon_en = get_dts_u32(np, TCON_EN_TAG);
		pctx_pnl->cus.mirror_mode = get_dts_u32(np, MIRROR_TAG);
		pctx_pnl->cus.op_format = get_dts_u32(np, OUTPUT_FORMAT_TAG);
		pctx_pnl->cus.ssc_en = get_dts_u32(np, SSC_EN_TAG);
		pctx_pnl->cus.ssc_step = get_dts_u32(np, SSC_STEP_TAG);
		pctx_pnl->cus.ssc_span = get_dts_u32(np, SSC_SPAN_TAG);
		pctx_pnl->cus.od_en = get_dts_u32(np, OD_EN_TAG);
		pctx_pnl->cus.od_buf_addr = get_dts_u32(np, OD_ADDR_TAG);
		pctx_pnl->cus.od_buf_size = get_dts_u32(np, OD_BUF_SIZE_TAG);
		pctx_pnl->cus.gpio_bl = get_dts_u32(np, GPIO_BL_TAG);
		pctx_pnl->cus.gpio_vcc = get_dts_u32(np, GPIO_VCC_TAG);
		pctx_pnl->cus.m_del = get_dts_u32(np, M_DELTA_TAG);
		pctx_pnl->cus.cus_color_prim = get_dts_u32(np, COLOR_PRIMA_TAG);
		pctx_pnl->cus.src_wx = get_dts_u32(np, SOURCE_WX_TAG);
		pctx_pnl->cus.src_wy = get_dts_u32(np, SOURCE_WY_TAG);
		pctx_pnl->cus.disp_mode = get_dts_u32(np, VG_DISP_TAG);
		pctx_pnl->cus.osd_height = get_dts_u32(np, OSD_HEIGHT_TAG);
		pctx_pnl->cus.osd_width = get_dts_u32(np, OSD_WIDTH_TAG);
		pctx_pnl->cus.osd_hs_st = get_dts_u32(np, OSD_HS_START_TAG);
		pctx_pnl->cus.osd_hs_end = get_dts_u32(np, OSD_HS_END_TAG);
		pctx_pnl->cus.osd_hde_st = get_dts_u32(np, OSD_HDE_START_TAG);
		pctx_pnl->cus.osd_hde_end = get_dts_u32(np, OSD_HDE_END_TAG);
		pctx_pnl->cus.osd_htt = get_dts_u32(np, OSD_HTOTAL_TAG);
		pctx_pnl->cus.osd_vs_st = get_dts_u32(np, OSD_VS_START_TAG);
		pctx_pnl->cus.osd_vs_end = get_dts_u32(np, OSD_VS_END_TAG);
		pctx_pnl->cus.osd_vde_st = get_dts_u32(np, OSD_VDE_START_TAG);
		pctx_pnl->cus.osd_vde_end = get_dts_u32(np, OSD_VDE_END_TAG);
		pctx_pnl->cus.osd_vtt = get_dts_u32(np, OSD_VTOTAL_TAG);
	}

	//Get MMAP related info
	np = of_find_node_by_name(NULL, PNL_OUTPUT_SWING_NODE);
	if (np != NULL) {
		pctx_pnl->swing.en = get_dts_u32(np, SWING_EN_TAG);
		pctx_pnl->swing.ctrl_chs = get_dts_u32(np, SWING_CHS_TAG);
		pctx_pnl->swing.ch3_0 = get_dts_u32(np, SWING_CH3_0_TAG);
		pctx_pnl->swing.ch7_4 = get_dts_u32(np, SWING_CH7_4_TAG);
		pctx_pnl->swing.ch11_8 = get_dts_u32(np, SWING_CH11_8_TAG);
		pctx_pnl->swing.ch15_12 = get_dts_u32(np, SWING_CH15_12_TAG);
		pctx_pnl->swing.ch19_16 = get_dts_u32(np, SWING_CH19_16_TAG);
		pctx_pnl->swing.ch23_20 = get_dts_u32(np, SWING_CH23_20_TAG);
		pctx_pnl->swing.ch27_24 = get_dts_u32(np, SWING_CH27_24_TAG);
		pctx_pnl->swing.ch31_28 = get_dts_u32(np, SWING_CH31_28_TAG);
	}

	np = of_find_node_by_name(NULL, PNL_OUTPUT_PE_NODE);
	if (np != NULL) {

		pctx_pnl->pe.en = get_dts_u32(np, PE_EN_TAG);
		pctx_pnl->pe.ctrl_chs = get_dts_u32(np, PE_CHS_TAG);
		pctx_pnl->pe.ch3_0 = get_dts_u32(np, PE_CH3_0_TAG);
		pctx_pnl->pe.ch7_4 = get_dts_u32(np, PE_CH7_4_TAG);
		pctx_pnl->pe.ch11_8 = get_dts_u32(np, PE_CH11_8_TAG);
		pctx_pnl->pe.ch15_12 = get_dts_u32(np, PE_CH15_12_TAG);
		pctx_pnl->pe.ch19_16 = get_dts_u32(np, PE_CH19_16_TAG);
		pctx_pnl->pe.ch23_20 = get_dts_u32(np, PE_CH23_20_TAG);
		pctx_pnl->pe.ch27_24 = get_dts_u32(np, PE_CH27_24_TAG);
		pctx_pnl->pe.ch31_28 = get_dts_u32(np, PE_CH31_28_TAG);
	}

	np = of_find_node_by_name(NULL, PNL_LANE_ORDER_NODE);
	if (np != NULL) {

		pctx_pnl->lane_order.ch3_0 = get_dts_u32(np, LANE_CH3_0_TAG);
		pctx_pnl->lane_order.ch7_4 = get_dts_u32(np, LANE_CH7_4_TAG);
		pctx_pnl->lane_order.ch11_8 = get_dts_u32(np, LANE_CH11_8_TAG);
		pctx_pnl->lane_order.ch15_12 = get_dts_u32(np, LANE_CH15_12_TAG);
		pctx_pnl->lane_order.ch19_16 = get_dts_u32(np, LANE_CH19_16_TAG);
		pctx_pnl->lane_order.ch23_20 = get_dts_u32(np, LANE_CH23_20_TAG);
		pctx_pnl->lane_order.ch27_24 = get_dts_u32(np, LANE_CH27_24_TAG);
		pctx_pnl->lane_order.ch31_28 = get_dts_u32(np, LANE_CH31_28_TAG);
	}

	np = of_find_node_by_name(NULL, PNL_OUTPUT_CONFIG_NODE);
	if (np != NULL) {

		pctx_pnl->op_cfg.ch3_0 = get_dts_u32(np, OUTCFG_CH3_0_TAG);
		pctx_pnl->op_cfg.ch7_4 = get_dts_u32(np, OUTCFG_CH7_4_TAG);
		pctx_pnl->op_cfg.ch11_8 = get_dts_u32(np, OUTCFG_CH11_8_TAG);
		pctx_pnl->op_cfg.ch15_12 = get_dts_u32(np, OUTCFG_CH15_12_TAG);
		pctx_pnl->op_cfg.ch19_16 = get_dts_u32(np, OUTCFG_CH19_16_TAG);
		pctx_pnl->op_cfg.ch23_20 = get_dts_u32(np, OUTCFG_CH23_20_TAG);
		pctx_pnl->op_cfg.ch27_24 = get_dts_u32(np, OUTCFG_CH27_24_TAG);
		pctx_pnl->op_cfg.ch31_28 = get_dts_u32(np, OUTCFG_CH31_28_TAG);

	}

	np = of_find_node_by_name(NULL, PNL_COLOR_INFO_NODE);
	if (np != NULL) {

		pctx_pnl->color_info.space = get_dts_u32(np, COLOR_SPACE_TAG);
		pctx_pnl->color_info.format = get_dts_u32(np, COLOR_FORMAT_TAG);
		pctx_pnl->color_info.range = get_dts_u32(np, COLOR_RANGE_TAG);
		pctx_pnl->color_info.rx = get_dts_u32(np, RX_TAG);
		pctx_pnl->color_info.ry = get_dts_u32(np, RY_TAG);
		pctx_pnl->color_info.gx = get_dts_u32(np, GX_TAG);
		pctx_pnl->color_info.gy = get_dts_u32(np, GY_TAG);
		pctx_pnl->color_info.bx = get_dts_u32(np, BX_TAG);
		pctx_pnl->color_info.by = get_dts_u32(np, BY_TAG);
		pctx_pnl->color_info.wx = get_dts_u32(np, WX_TAG);
		pctx_pnl->color_info.wy = get_dts_u32(np, WY_TAG);
		pctx_pnl->color_info.maxlum = get_dts_u32(np, MAX_LUMI_TAG);
		pctx_pnl->color_info.medlum = get_dts_u32(np, MED_LUMI_TAG);
		pctx_pnl->color_info.minlum = get_dts_u32(np, MIN_LUMI_TAG);
		pctx_pnl->color_info.linear_rgb = get_dts_u32(np, LINEAR_RGB_TAG);
		pctx_pnl->color_info.hdrnits = get_dts_u32(np, HDR_NITS_TAG);

	}

	//Get FRC table info
	np = of_find_node_by_name(NULL, PNL_FRC_TABLE_NODE);
	if (np != NULL) {
		DRM_INFO("[%s, %d]: find node success, name = %s\n",
			__func__, __LINE__, PNL_FRC_TABLE_NODE);

		ret = of_get_property(np, FRC_TABLE_TAG, &FRClen);
		if (ret == 0x0) {
			DRM_INFO("[%s, %d]: read string failed, name = %s\n",
				__func__, __LINE__, FRC_TABLE_TAG);
			return ret;
		}
		for (FRCType = 0;
			FRCType < (FRClen / (sizeof(__u32) * PNL_FRC_TABLE_ARGS_NUM));
			FRCType++) {

			DRM_INFO("[%s, %d]: get_dts_u32_index, FRCType = %d\n",
				__func__, __LINE__, FRCType);
			pctx_pnl->frc_info[FRCType].u16LowerBound =
				get_dts_u32_index(np,
				FRC_TABLE_TAG,
				FRCType*PNL_FRC_TABLE_ARGS_NUM + 0);
			pctx_pnl->frc_info[FRCType].u16HigherBound =
				get_dts_u32_index(np,
				FRC_TABLE_TAG,
				FRCType*PNL_FRC_TABLE_ARGS_NUM + 1);
			pctx_pnl->frc_info[FRCType].u8FRC_in =
				get_dts_u32_index(np,
				FRC_TABLE_TAG,
				FRCType*PNL_FRC_TABLE_ARGS_NUM + 2);
			pctx_pnl->frc_info[FRCType].u8FRC_out =
				get_dts_u32_index(np,
				FRC_TABLE_TAG,
				FRCType*PNL_FRC_TABLE_ARGS_NUM + 3);
		}
	}

	dump_dts_info(pctx_pnl);

	return 0;
}

static bool mtk_video_is_ext_prop_updated(
	struct video_plane_prop *plane_props,
	enum ext_video_plane_prop prop)
{
	bool update = FALSE;

	if (plane_props->prop_update[prop])
		update = TRUE;

	DRM_INFO("[%s][%d] ext_prop:%d update:%d\n", __func__, __LINE__,
		prop, update);

	return update;
}

static int _mtk_video_set_pixelshift_values(
	int64_t pixelshift_h_val,
	int64_t pixelshift_v_val)
{
	return 0;
}

static int _mtk_video_set_pixelshift_features(
	uint64_t pixelshift_h_range,
	uint64_t pixelshift_v_range,
	uint64_t pixelshift_osd_attached,
	uint64_t pixelshift_type)
{
	return 0;
}

static int _mtk_video_get_pixelshift_Value(
	int64_t *ppixelshift_Hval,
	int64_t *ppixelshift_Vval)
{
	return 0;
}

static int _mtk_video_get_memc_chip_caps(int64_t *pMemcChipCaps)
{
	return 0;
}

static int _mtk_video_get_memc_status(void *pMemcStatus)
{
	return 0;
}

static int _mtk_video_send_memc_cmd(void *pMemcCmd)
{
	return 0;
}

static int _mtk_video_set_memc_misc_type(uint64_t u64MiscType)
{
	return 0;
}

static MS_BOOL _mtk_video_SetPanelConfig(struct mtk_panel_context *pctx_panel)
{
	return TRUE;
	//yu-jen will remove this function later
}

static MS_BOOL _mtk_video_SetPanelFpllCus(struct mtk_panel_context *pctx_pnl)
{
	if (pctx_pnl->stCusLpllSetting.u16PhaseLimit) {
		DRM_INFO("LPLL debug E_FPLL_FLAG_PHASELIMIT = 0x%x\n",
			pctx_pnl->stCusLpllSetting.u16PhaseLimit);
		//MApi_XC_FPLLCustomerMode(E_FPLL_MODE_ENABLE,
		//E_FPLL_FLAG_PHASELIMIT,
		//pctx_pnl->stCusLpllSetting.u16PhaseLimit);
	}
	if (pctx_pnl->stCusLpllSetting.u32D5D6D7) {
		DRM_INFO("LPLL debug E_FPLL_FLAG_D5D6D7 = 0x%x\n",
			pctx_pnl->stCusLpllSetting.u32D5D6D7);
		//MApi_XC_FPLLCustomerMode(E_FPLL_MODE_ENABLE,
		//E_FPLL_FLAG_D5D6D7,pctx_pnl->stCusLpllSetting.u32D5D6D7);
	}
	if (pctx_pnl->stCusLpllSetting.u8Igain) {
		DRM_INFO("LPLL debug E_FPLL_FLAG_IGAIN = 0x%x\n",
			pctx_pnl->stCusLpllSetting.u8Igain);
		//MApi_XC_FPLLCustomerMode(E_FPLL_MODE_ENABLE,
		//E_FPLL_FLAG_IGAIN, pctx_pnl->stCusLpllSetting.u8Igain);
	}
	if (pctx_pnl->stCusLpllSetting.u8Pgain) {
		DRM_INFO("LPLL debug E_FPLL_FLAG_PGAIN = 0x%x\n",
			pctx_pnl->stCusLpllSetting.u8Pgain);
		//MApi_XC_FPLLCustomerMode(E_FPLL_MODE_ENABLE,
		//E_FPLL_FLAG_PGAIN, pctx_pnl->stCusLpllSetting.u8Pgain);
	}
	if (pctx_pnl->stCusLpllSetting.u16VttUpperBound) {
		DRM_INFO("LPLL debug E_FPLL_FLAG_VTT_Upperbound = 0x%x\n",
			pctx_pnl->stCusLpllSetting.u16VttUpperBound);
		//MApi_XC_FPLLCustomerMode(E_FPLL_MODE_ENABLE,
		//E_FPLL_FLAG_VTT_Upperbound,
		//pctx_pnl->stCusLpllSetting.u16VttUpperBound);
	}
	if (pctx_pnl->stCusLpllSetting.u16VttLowerBound) {
		DRM_INFO("LPLL debug E_FPLL_FLAG_VTT_Lowerbound = 0x%x\n",
			pctx_pnl->stCusLpllSetting.u16VttLowerBound);
		//MApi_XC_FPLLCustomerMode(
		//E_FPLL_MODE_ENABLE, E_FPLL_FLAG_VTT_Lowerbound,
		//pctx_pnl->stCusLpllSetting.u16VttLowerBound);
	}
	if (pctx_pnl->stCusLpllSetting.u8DiffLimit) {
		DRM_INFO("LPLL debug E_FPLL_FLAG_VTT_DiffLimit = 0x%x\n",
			pctx_pnl->stCusLpllSetting.u8DiffLimit);
		//MApi_XC_FPLLCustomerMode(
		//E_FPLL_MODE_ENABLE, E_FPLL_FLAG_VTT_DiffLimit,
		//pctx_pnl->stCusLpllSetting.u8DiffLimit);
	}
	return TRUE;
}
/* ------- above is static function -------*/

int mtk_video_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val, int prop_index)
{
	int ret = 0;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct drm_property_blob *property_blob = NULL;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t framesync_mode = 0;

	//static XC_SetTiming_Info stTimingInfo = {0};

	switch (prop_index) {
	case E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID:
		//stTimingInfo.u32HighAccurateInputVFreq = (uint32_t)val;
		//stTimingInfo.u16InputVFreq = (uint32_t)val /100;
		DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		framesync_mode = crtc_props->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE];
		if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_VTTV)
			_mtk_video_set_framesync_mode(pctx);
		break;

	case E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE:
		DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		_mtk_video_set_framesync_mode(pctx);
		break;

	case E_EXT_CRTC_PROP_LOW_LATENCY_MODE:
		DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_LOW_LATENCY_MODE= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		framesync_mode = crtc_props->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE];
		if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_VTTV)
			_mtk_video_set_low_latency_mode(pctx, val);
		break;

	case E_EXT_CRTC_PROP_CUSTOMIZE_FRC_TABLE:
		DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_CUSTOMIZE_FRC_TABLE= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob != NULL) {
			struct drm_st_pnl_frc_table_info cusFRCTableInfo[PNL_FRC_TABLE_MAX_INDEX];

			memset(&cusFRCTableInfo,
				0,
				sizeof(struct drm_st_pnl_frc_table_info)*PNL_FRC_TABLE_MAX_INDEX);
			memcpy(&cusFRCTableInfo,
				property_blob->data,
				property_blob->length);

			_mtk_video_set_customize_frc_table(
				&pctx->panel_priv,
				cusFRCTableInfo);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;

	case E_EXT_CRTC_PROP_PANEL_TIMING:
		DRM_INFO("[%s][%d] PANEL_TIMING= 0x%tx\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob != NULL) {
			drm_st_mtk_tv_timing_info stTimingInfo;

			memset(&stTimingInfo,
				0,
				sizeof(drm_st_mtk_tv_timing_info));
			memcpy(&stTimingInfo,
				property_blob->data,
				sizeof(drm_st_mtk_tv_timing_info));
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;

	case E_EXT_CRTC_PROP_SET_FREERUN_TIMING:
		DRM_INFO("[%s][%d] SET_FREERUN_TIMING= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		//MApi_XC_SetFreeRunTiming();
		break;

	case E_EXT_CRTC_PROP_FORCE_FREERUN:
		DRM_INFO("[%s][%d] FORCE_FREERUN= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		//MApi_SC_ForceFreerun(val);
		break;

	case E_EXT_CRTC_PROP_FREERUN_VFREQ:
		DRM_INFO("[%s][%d] FREERUN_VFREQ= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		//MApi_SC_SetFreerunVFreq(val);
		break;

	case E_EXT_CRTC_PROP_VIDEO_LATENCY:
		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob != NULL) {
			drm_st_mtk_tv_video_latency_info stVideoLatencyInfo;

			memset(&stVideoLatencyInfo,
				0,
				sizeof(drm_st_mtk_tv_video_latency_info));

			memcpy(&stVideoLatencyInfo,
				property_blob->data,
				sizeof(drm_st_mtk_tv_video_latency_info));

			DRM_INFO("[%s][%d] VIDEO_LATENCY = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);

			//Update blob id memory
			memcpy(property_blob->data,
				&stVideoLatencyInfo,
				sizeof(drm_st_mtk_tv_video_latency_info));
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACHED:
		DRM_INFO("[%s][%d] set pixelshift osd attached= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_TYPE:
		DRM_INFO("[%s][%d] set pixelshift type= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		if (val == VIDEO_CRTC_PIXELSHIFT_DO_JUSTSCAN) {
			pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_HRANGE]->values[1]
				= PIXELSHIFT_TYPE_JUSTSCAN_H_MAX;
			pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_VRANGE]->values[1]
				= PIXELSHIFT_TYPE_JUSTSCAN_V_MAX;
		} else if (val == VIDEO_CRTC_PIXELSHIFT_DO_OVERSCAN) {
			pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_HRANGE]->values[1]
				= PIXELSHIFT_TYPE_OVERSCAN_H_MAX;
			pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_VRANGE]->values[1]
				= PIXELSHIFT_TYPE_OVERSCAN_V_MAX;
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_HRANGE:
		DRM_INFO("[%s][%d] set pixelshift H range= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);

		pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_H]->values[0] =
			I642U64(positive2negative(val));
		pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_H]->values[1] =
			val;
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_VRANGE:
		DRM_INFO("[%s][%d] set pixelshift V range= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_V]->values[0]
			= I642U64(positive2negative(val));
		pctx->drm_crtc_prop[E_EXT_CRTC_PROP_PIXELSHIFT_V]->values[1]
			= val;
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_H:
		DRM_INFO("[%s][%d] set pixelshift H value= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_V:
		DRM_INFO("[%s][%d] set pixelshift V value= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CRTC_PROP_MEMC_PLANE_ID:
		DRM_INFO("[%s][%d] set memc plane id= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CRTC_PROP_MEMC_LEVEL:
		//if(property_blob == NULL)
		{
			/*Get the blob property form blob id*/
			property_blob = drm_property_lookup_blob(property->dev, val);
			if (property_blob != NULL)
				ret = _mtk_video_send_memc_cmd(property_blob->data);

			DRM_INFO("[%s][%d] set memc level blob_id= %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CRTC_PROP_MEMC_GAME_MODE:
		//if(property_blob == NULL)
		{
			property_blob = drm_property_lookup_blob(property->dev, val);
			if (property_blob != NULL)
				ret = _mtk_video_send_memc_cmd(property_blob->data);

			DRM_INFO("[%s][%d] set memc glame blob_id= %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CRTC_PROP_MEMC_FIRSTFRAME_READY:
		//if(property_blob == NULL)
		{
			property_blob = drm_property_lookup_blob(property->dev, val);
			if (property_blob != NULL)
				ret = _mtk_video_send_memc_cmd(property_blob->data);

			DRM_INFO("[%s][%d] set firstframe ready blob_id= %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CRTC_PROP_MEMC_DECODE_IDX_DIFF:
		//if(property_blob == NULL)
		{
			property_blob = drm_property_lookup_blob(property->dev, val);
			if (property_blob != NULL)
				ret = _mtk_video_send_memc_cmd(property_blob->data);

			DRM_INFO("[%s][%d] set decode idx diff blob id = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CRTC_PROP_MEMC_MISC_A_TYPE:
		DRM_INFO("[%s][%d] set memc misc_a_type value= %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		ret = _mtk_video_set_memc_misc_type(val);
		break;

	case E_EXT_CRTC_PROP_MAX:
		DRM_INFO("[%s][%d] invalid property!!\n",
			__func__, __LINE__);
		break;

	default:
		break;
	}

	return ret;
}

int mtk_video_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index)
{
	int ret = 0;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	struct drm_property_blob *property_blob = NULL;
	int64_t int64Value = 0;

	switch (prop_index) {
	case E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID:
		if (val != NULL) {
			*val = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
			DRM_INFO("[%s][%d] get framesync plane id = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE:
		if (val != NULL) {
			*val = crtc_props->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE];
			DRM_INFO("[%s][%d] get frame sync mode = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_LOW_LATENCY_MODE:
		if (val != NULL) {
			*val = crtc_props->prop_val[E_EXT_CRTC_PROP_LOW_LATENCY_MODE];
			DRM_INFO("[%s][%d] get low latency mode status = %td\n",
				__func__, __LINE__, (ptrdiff_t)*val);
		}

		break;
	case E_EXT_CRTC_PROP_CUSTOMIZE_FRC_TABLE:
		if ((property_blob == NULL) && (val != NULL)) {
			property_blob = drm_property_lookup_blob(property->dev, *val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown FRC table status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			} else {
				ret = _mtk_video_get_customize_frc_table(
					&pctx->panel_priv,
					property_blob->data);
				DRM_INFO("[%s][%d] get FRC table status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			}
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_H:
		ret = _mtk_video_get_pixelshift_Value(&int64Value, NULL);
		if (val != NULL) {
			*val = (uint64_t)int64Value;

			//FIXME for test
			if (*val == 0)
				*val = crtc_props->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_H];

			//DRM_INFO("[%s][%d] get pixelshiftf h value = %td\n",
				//__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_PIXELSHIFT_V:
		ret = _mtk_video_get_pixelshift_Value(NULL, &int64Value);
		if (val != NULL) {
			*val = (uint64_t)int64Value;

			//FIXME for test
			if (*val == 0)
				*val = crtc_props->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_V];

			//DRM_INFO("[%s][%d] get pixelshiftf v value = %td\n",
				//__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_MEMC_CHIP_CAPS:
		ret = _mtk_video_get_memc_chip_caps(val);
		if (val != NULL) {
			//DRM_INFO("[%s][%d] get memc chip caps = %td\n",
				//__func__, __LINE__, (ptrdiff_t)*val);
		}
		break;
	case E_EXT_CRTC_PROP_MEMC_GET_STATUS:
		if ((property_blob == NULL) && (val != NULL)) {
			property_blob = drm_property_lookup_blob(property->dev, *val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			} else {
				ret = _mtk_video_get_memc_status(property_blob->data);
				//DRM_INFO("[%s][%d] status = %td!!\n",
					//__func__, __LINE__, (ptrdiff_t)*val);
			}
		}
		break;
	case E_EXT_CRTC_PROP_MAX:
		DRM_INFO("[%s][%d] invalid property!!\n", __func__, __LINE__);
		break;
	default:
		//DRM_INFO("[DRM][VIDEO][%s][%d]default\n", __func__, __LINE__);
		break;
	}

	return ret;
}

/*Connector*/
int mtk_video_atomic_set_connector_property(
	struct mtk_tv_drm_connector *connector,
	struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t val)
{
	int ret = -EINVAL;
	int i;
	struct drm_property_blob *property_blob;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)connector->connector_private;
	struct ext_connector_prop_info *connector_props = pctx->ext_connector_properties;

	/*Get the blob property form blob id*/
	if (property->flags  & DRM_MODE_PROP_BLOB)
		property_blob = drm_property_lookup_blob(property->dev, val);

	for (i = 0; i < E_EXT_CONNECTOR_PROP_MAX; i++) {
		if (property == pctx->drm_connector_prop[i]) {
			connector_props->prop_val[i] = val;
			ret = 0x0;
			break;
		}
	}

	switch (i) {
	case E_EXT_CONNECTOR_PROP_PNL_DBG_LEVEL:
		//MApi_PNL_SetDbgLevel(val);
		DRM_INFO("[%s][%d] PNL_DBG_LEVEL = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_OUTPUT:
		//MApi_PNL_SetOutput(val);
		DRM_INFO("[%s][%d] PNL_OUTPUT = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SWING_LEVEL:
		//MApi_PNL_Control_Out_Swing(val);
		DRM_INFO("[%s][%d] PNL_SWING_LEVEL = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_FORCE_PANEL_DCLK:
		//MApi_PNL_ForceSetPanelDCLK(val , TRUE);
		DRM_INFO("[%s][%d] PNL_FORCE_PANEL_DCLK = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_PNL_FORCE_PANEL_HSTART:
		//MApi_PNL_ForceSetPanelHStart(val , TRUE);
		DRM_INFO("[%s][%d] PNL_PNL_FORCE_PANEL_HSTART = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_PNL_OUTPUT_PATTERN:
		if (property_blob != NULL) {
			drm_st_pnl_output_pattern stOutputPattern;

			memset(&stOutputPattern, 0, sizeof(drm_st_pnl_output_pattern));
			property_blob = drm_property_lookup_blob(property->dev, val);
			memcpy(&stOutputPattern,
				property_blob->data,
				sizeof(drm_st_pnl_output_pattern));

			DRM_INFO("[%s][%d] PNL_OUTPUT_PATTERN = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}

		break;
	case E_EXT_CONNECTOR_PROP_PNL_MIRROR_STATUS:
		if (property_blob != NULL) {
			drm_st_pnl_mirror_info stMirrorInfo;

			memset(&stMirrorInfo, 0, sizeof(drm_st_pnl_mirror_info));
			property_blob = drm_property_lookup_blob(property->dev, val);
			memcpy(&stMirrorInfo, property_blob->data, sizeof(drm_st_pnl_mirror_info));

			//MApi_PNL_GetMirrorStatus(&stMirrorInfo);

			//Update blob id memory
			memcpy(property_blob->data, &stMirrorInfo, sizeof(drm_st_pnl_mirror_info));
			DRM_INFO("[%s][%d] PNL_MIRROR_STATUS = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SSC_EN:
		//MApi_PNL_SetSSC_En(val);
		DRM_INFO("[%s][%d] PNL_SSC_EN = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SSC_FMODULATION:
		//MApi_PNL_SetSSC_Fmodulation(val);
		DRM_INFO("[%s][%d] PNL_SSC_FMODULATION = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SSC_RDEVIATION:
		//MApi_PNL_SetSSC_Rdeviation(val);
		DRM_INFO("[[%s][%d] PNL_SSC_RDEVIATION = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_OVERDRIVER_ENABLE:
		//MApi_PNL_OverDriver_Enable(val);
		DRM_INFO("[%s][%d] PNL_OVERDRIVER_ENABLE = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		break;
	case E_EXT_CONNECTOR_PROP_PNL_PANEL_SETTING:
		if (property_blob != NULL) {
			drm_st_pnl_ini_para stPnlSetting;

			memset(&stPnlSetting, 0, sizeof(drm_st_pnl_ini_para));

			property_blob = drm_property_lookup_blob(property->dev, val);
			memcpy(&stPnlSetting, property_blob->data, sizeof(drm_st_pnl_ini_para));

			DRM_INFO("[%s][%d] PNL_PANEL_SETTING = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CONNECTOR_PROP_MAX:
		DRM_INFO("[%s][%d] invalid property!!\n",
			__func__, __LINE__);
		break;
	default:
		break;
	}

	return ret;
}

int mtk_video_atomic_get_connector_property(
	struct mtk_tv_drm_connector *connector,
	const struct drm_connector_state *state,
	struct drm_property *property,
	uint64_t *val)
{
	int ret = -EINVAL;
	int i;
	drm_pnl_info stPanelInfo;
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)connector->connector_private;
	struct ext_connector_prop_info *connector_props = pctx->ext_connector_properties;

	memset(&stPanelInfo, 0, sizeof(drm_pnl_info));

	for (i = 0; i < E_EXT_CONNECTOR_PROP_MAX; i++) {
		if (property == pctx->drm_connector_prop[i]) {
			*val = connector_props->prop_val[i];
			ret = 0x0;
			break;
		}
	}

	switch (i) {
	case E_EXT_CONNECTOR_PROP_PNL_PANEL_INFO:
		//stPanelInfo.u16PnlHstart = MAPI_PNL_GetPNLHstart_U2();
		//stPanelInfo.u16PnlVstart = MAPI_PNL_GetPNLVstart_U2();
		//stPanelInfo.u16PnlWidth = MAPI_PNL_GetPNLWidth_U2();
		//stPanelInfo.u16PnlHeight = MAPI_PNL_GetPNLHeight_U2();
		//stPanelInfo.u16PnlHtotal = MAPI_PNL_GetPNLHtotal_U2();
		//stPanelInfo.u16PnlVtotal = MAPI_PNL_GetPNLVtotal_U2();
		//stPanelInfo.u16PnlHsyncWidth = MAPI_PNL_GetPNLHsyncWidth_U2();
		//stPanelInfo.u16PnlHsyncBackPorch = MAPI_PNL_GetPNLHsyncBackPorch_U2();
		//stPanelInfo.u16PnlVsyncBackPorch = MAPI_PNL_GetPNLVsyncBackPorch_U2();
		//stPanelInfo.u16PnlDefVfreq = MApi_PNL_GetDefVFreq();
		//stPanelInfo.u16PnlLPLLMode = MApi_PNL_GetLPLLMode();
		//stPanelInfo.u16PnlLPLLType = MApi_PNL_GetLPLLType_U2();
		//stPanelInfo.u16PnlMinSET = MApi_PNL_GetMinSET_U2();
		//stPanelInfo.u16PnlMaxSET = MApi_PNL_GetMaxSET_U2();
		break;
	case E_EXT_CONNECTOR_PROP_MAX:
		DRM_INFO("[%s][%d] invalid property!!\n",
			__func__, __LINE__);
		break;
	default:
		break;
	}

	return ret;
}

void mtk_video_atomic_crtc_flush(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *old_crtc_state)
{
	struct mtk_tv_kms_context *pctx =
		(struct mtk_tv_kms_context *)
		crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	struct ext_crtc_prop_info *crtc_prop = pctx->ext_crtc_properties;
	int64_t pixelshift_h_val =
		crtc_prop->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_H];
	int64_t pixelshift_v_val =
		crtc_prop->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_V];
	uint64_t pixelshift_osd_attached =
		crtc_prop->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACHED];
	uint64_t pixelshift_type =
		crtc_prop->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_TYPE];
	uint64_t pixelshift_h_range =
		crtc_prop->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_HRANGE];
	uint64_t pixelshift_v_range =
		crtc_prop->prop_val[E_EXT_CRTC_PROP_PIXELSHIFT_VRANGE];

	//pixelshift feature
	if (crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_TYPE] &&
	   crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACHED] &&
	   crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_HRANGE] &&
	   crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_VRANGE]) {
		_mtk_video_set_pixelshift_features(pixelshift_h_range,
			pixelshift_v_range,
			pixelshift_osd_attached,
			pixelshift_type);
		crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_TYPE] = 0;
		crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_OSD_ATTACHED] = 0;
		crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_HRANGE] = 0;
		crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_VRANGE] = 0;
	}

	//pixelshift value
	if (crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_H] &&
	   crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_V]) {
		_mtk_video_set_pixelshift_values(pixelshift_h_val, pixelshift_v_val);
		crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_H] = 0;
		crtc_prop->prop_update[E_EXT_CRTC_PROP_PIXELSHIFT_V] = 0;
	}

}

int mtk_panel_init(struct device *dev)
{
	int ret;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct mtk_tv_drm_crtc *crtc = &pctx->crtc;
	drm_st_pnl_ini_para stPnlIniDclk;
	char szDefaultTconBinPath[] = "/vendor/tvconfig/config/tcon/TCON20.bin";

	memset(&stPnlIniDclk, 0x00, sizeof(drm_st_pnl_ini_para));

	/*read panel related data from dtb*/
	ret = readDTB2PanelPrivate(&pctx->panel_priv);
	if (ret != 0x0) {
		DRM_INFO("[%s, %d]: readDTB2PanelPrivate  failed\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	/*set panel output config*/
	if (_mtk_video_SetPanelConfig(&pctx->panel_priv) != TRUE) {
		DRM_INFO("[%s, %d]: Set panel output config fail!\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	/*Init cus fpll setting*/
	_mtk_video_SetPanelFpllCus(&pctx->panel_priv);

	//Reset the right PANEL_MAX_DCLK, PANEL_DCLK, PANEL_MIN_DCLK
	stPnlIniDclk.u32Version = ST_PNL_PARA_VERSION;
	stPnlIniDclk.u32PanelMaxDCLK = (__u32)pctx_pnl->stPanelType.m_dwPanelMaxDCLK;
	stPnlIniDclk.u32PanelDCLK = (__u32)pctx_pnl->stPanelType.m_dwPanelDCLK;
	stPnlIniDclk.u32PanelMinDCLK = (__u32)pctx_pnl->stPanelType.m_dwPanelMinDCLK;


	if (pctx_pnl->stODControl.u8ODEnable == TRUE) {
		drm_st_pnl_od_setting stPnlOdSetting;

		memset(&stPnlOdSetting, 0, sizeof(drm_st_pnl_od_setting));
		if (pctx_pnl->stODControl.u32OD_MSB_Size != 0) {
			stPnlOdSetting.u32OD_MSB_Addr = pctx_pnl->stODControl.u32OD_MSB_Addr;
			stPnlOdSetting.u32OD_MSB_Size = pctx_pnl->stODControl.u32OD_MSB_Size;
			stPnlOdSetting.pu16ODTbl = NULL;
			stPnlOdSetting.u32ODTbl_Size = NULL;
		}
	}

	//External BE related
	mtk_render_cfg_connector(pctx_pnl);

	//2. Config CRTC setting
	//update gamma lut size
	mtk_render_cfg_crtc(pctx_pnl);

	//update gamma lut size
	drm_mode_crtc_set_gamma_size(&crtc->base, MTK_GAMMA_LUT_SIZE);

	if (ret != 0x0) {
		DRM_INFO("mtk_plane_init failed.\n");
		goto ERR;
	}
	DRM_INFO("[%s][%d] success!!\n", __func__, __LINE__);

	return 0;

ERR:
	return ret;
}

void mtk_video_panel_gamma_setting(
	struct drm_crtc *crtc,
	uint16_t *r,
	uint16_t *g,
	uint16_t *b,
	uint32_t size)
{
	struct drm_property_blob *property_blob;
	drm_st_pnl_gammatbl stGammaTbl;
	uint16_t u16Blobid = 0;

	memset(&stGammaTbl, 0, sizeof(drm_st_pnl_gammatbl));

	u16Blobid = *r;

	property_blob = drm_property_lookup_blob(crtc->dev, u16Blobid);
	memcpy(&stGammaTbl, property_blob->data, sizeof(drm_st_pnl_gammatbl));
}
