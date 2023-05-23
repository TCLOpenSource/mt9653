// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include "mtk_tv_drm_panel_common.h"

static bool isPanelCommonEnable = FALSE;
//-------------------------------------------------------------------------------------------------
// Prototype
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------

#define VB1_16_LANE (16)
//-------------------------------------------------------------------------------------------------
// Function
//-------------------------------------------------------------------------------------------------
__u32 get_dts_u32(struct device_node *np, const char *name)
{
	int u32Tmp = 0x0;
	int ret = 0;

	if (!np) {
		DRM_ERROR("[%s, %d]: device_node is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	if (!name) {
		DRM_ERROR("[%s, %d]: name is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

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
	int ret = 0;

	if (!np) {
		DRM_ERROR("[%s, %d]: device_node is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	if (!name) {
		DRM_ERROR("[%s, %d]: name is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	ret = of_property_read_u32_index(np, name, index, &u32Tmp);
	if (ret != 0x0) {
		DRM_INFO("[%s, %d]: read u32 index failed, name = %s\n",
			__func__, __LINE__, name);
	}
	return u32Tmp;
}

void panel_common_enable(void)
{
	if (isPanelCommonEnable == FALSE) {

		struct device_node *np = NULL;
		uint32_t clk_version = 0;

		np = of_find_node_by_name(NULL, PNL_LPLL_NODE);
		if (np != NULL) {
			uint32_t u32RegAddr = 0x0;
			uint32_t u32Size = 0x0;
			void __iomem *pVaddr = NULL;

			u32RegAddr = get_dts_u32_index(np, PNL_LPLL_REG, 1);
			u32Size = get_dts_u32_index(np, PNL_LPLL_REG, 3);
			pVaddr = ioremap(u32RegAddr, u32Size);
			if (pVaddr != NULL)
				drv_hwreg_common_setRIUaddr(
					(u32RegAddr - 0x1c000000),
					u32Size,
					(uint64_t)pVaddr);
		} else {
			DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_MOD_NODE);
		}

		np = of_find_node_by_name(NULL, PNL_MOD_NODE);
		if (np != NULL) {
			uint32_t u32RegAddr = 0x0;
			uint32_t u32Size = 0x0;
			void __iomem *pVaddr = NULL;

			u32RegAddr = get_dts_u32_index(np, PNL_MOD_REG, 1);
			u32Size = get_dts_u32_index(np, PNL_MOD_REG, 3);
			pVaddr = ioremap(u32RegAddr, u32Size);
			if (pVaddr != NULL)
				drv_hwreg_common_setRIUaddr(
					(u32RegAddr - 0x1c000000),
					u32Size,
					(uint64_t)pVaddr);
		} else {
			DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_MOD_NODE);
		}

		//PATCH need(no one sets)
		drv_hwreg_common_setRIUaddr(0x002E4000, 0x200,
					(uint64_t)ioremap(0x1c2E4000, 0x200));
		drv_hwreg_common_setRIUaddr(0x00204000, 0x2000,
					(uint64_t)ioremap(0x1c204000, 0x2000));
		drv_hwreg_common_setRIUaddr(0x00206000, 0x2000,
					(uint64_t)ioremap(0x1c206000, 0x2000));

		drv_hwreg_render_pnl_init_clk();

		//check clock version
		np = of_find_compatible_node(NULL, NULL, "MTK-drm-tv-kms");
		clk_version = get_dts_u32(np, "Clk_Version");
		if (clk_version == 1)
			drv_hwreg_render_pnl_init_clk_v2();
	}
	isPanelCommonEnable = TRUE;
}

void panel_common_disable(void)
{
	isPanelCommonEnable = FALSE;
}

void init_mod_cfg(drm_mod_cfg *pCfg)
{
	if (!pCfg) {
		DRM_ERROR("[%s, %d]: drm_mod_cfg is NULL.\n",
			__func__, __LINE__);
		return;
	}

	pCfg->linkIF = E_LINK_NONE;
	pCfg->lanes = 0;
	pCfg->div_sec = 0;
	pCfg->timing = E_OUTPUT_MODE_NONE;
	pCfg->format = E_PNL_OUTPUT_FORMAT_NUM;
}

__u32 setDisplayTiming(drm_st_pnl_info *pStPnlInfo)
{
	if (!pStPnlInfo) {
		DRM_ERROR("[%s, %d]: Panel INFO is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	if (pStPnlInfo->linkIF != E_LINK_NONE) {
		uint32_t u32HdeEnd = 0;
		uint32_t u32VdeEnd = 0;
		uint32_t u32Hbackporch = 0;
		uint32_t u32Vbackporch = 0;

		u32HdeEnd = pStPnlInfo->de_hstart + pStPnlInfo->de_width;
		u32VdeEnd = pStPnlInfo->de_vstart + pStPnlInfo->de_height;

		u32Hbackporch = pStPnlInfo->de_hstart -
					(pStPnlInfo->hsync_st + pStPnlInfo->hsync_w);
		u32Vbackporch = pStPnlInfo->de_vstart -
					(pStPnlInfo->vsync_st + pStPnlInfo->vsync_w);

		pStPnlInfo->displayTiming.pixelclock.max = pStPnlInfo->max_dclk;
		pStPnlInfo->displayTiming.pixelclock.typ = pStPnlInfo->typ_dclk;
		pStPnlInfo->displayTiming.pixelclock.min = pStPnlInfo->min_dclk;


		pStPnlInfo->displayTiming.hactive.max = pStPnlInfo->de_width;
		pStPnlInfo->displayTiming.hactive.typ = pStPnlInfo->de_width;
		pStPnlInfo->displayTiming.hactive.min = pStPnlInfo->de_width;


		pStPnlInfo->displayTiming.hfront_porch.max = pStPnlInfo->max_htt - u32HdeEnd;
		pStPnlInfo->displayTiming.hfront_porch.typ = pStPnlInfo->typ_htt - u32HdeEnd;
		pStPnlInfo->displayTiming.hfront_porch.min = pStPnlInfo->min_htt - u32HdeEnd;

		pStPnlInfo->displayTiming.hback_porch.max = u32Hbackporch;
		pStPnlInfo->displayTiming.hback_porch.typ = u32Hbackporch;
		pStPnlInfo->displayTiming.hback_porch.min = u32Hbackporch;

		pStPnlInfo->displayTiming.hsync_len.max = pStPnlInfo->hsync_w;
		pStPnlInfo->displayTiming.hsync_len.typ = pStPnlInfo->hsync_w;
		pStPnlInfo->displayTiming.hsync_len.min = pStPnlInfo->hsync_w;

		pStPnlInfo->displayTiming.vactive.max = pStPnlInfo->de_height;
		pStPnlInfo->displayTiming.vactive.typ = pStPnlInfo->de_height;
		pStPnlInfo->displayTiming.vactive.min = pStPnlInfo->de_height;

		pStPnlInfo->displayTiming.vfront_porch.max = pStPnlInfo->max_vtt - u32VdeEnd;
		pStPnlInfo->displayTiming.vfront_porch.typ = pStPnlInfo->typ_vtt - u32VdeEnd;
		pStPnlInfo->displayTiming.vfront_porch.min = pStPnlInfo->min_vtt - u32VdeEnd;

		pStPnlInfo->displayTiming.vback_porch.max = u32Vbackporch;
		pStPnlInfo->displayTiming.vback_porch.typ = u32Vbackporch;
		pStPnlInfo->displayTiming.vback_porch.min = u32Vbackporch;

		pStPnlInfo->displayTiming.vsync_len.max = pStPnlInfo->vsync_w;
		pStPnlInfo->displayTiming.vsync_len.typ = pStPnlInfo->vsync_w;
		pStPnlInfo->displayTiming.vsync_len.min = pStPnlInfo->vsync_w;

		//pStPnlInfo->displayTiming.flags if need
	}
	return 0;
}

void set_out_mod_cfg(drm_st_pnl_info *src_info,
						drm_mod_cfg *dst_cfg)
{
	uint16_t vfreq = 0;

	if (!src_info) {
		DRM_ERROR("[%s, %d]: Panel INFO is NULL.\n",
			__func__, __LINE__);
		return;
	}

	if (!dst_cfg) {
		DRM_ERROR("[%s, %d]: drm_mod_cfg is NULL.\n",
			__func__, __LINE__);
		return;
	}

	if (src_info->linkIF != E_LINK_NONE) {

		dst_cfg->linkIF =  src_info->linkIF;
		dst_cfg->div_sec = src_info->div_section;
		dst_cfg->format = src_info->op_format;

		if (src_info->typ_htt != 0 && src_info->typ_vtt != 0)
			vfreq = src_info->typ_dclk / src_info->typ_htt / src_info->typ_vtt;

		if (IS_OUT_8K4K(src_info->de_width, src_info->de_height)) {
			if (IS_VFREQ_60HZ_GROUP(vfreq))
				dst_cfg->timing = E_8K4K_60HZ;

			if (IS_VFREQ_120HZ_GROUP(vfreq))
				dst_cfg->timing = E_8K4K_120HZ;

			//set lane number
			if (dst_cfg->format == E_OUTPUT_RGB ||
					dst_cfg->format == E_OUTPUT_YUV444)
				dst_cfg->lanes = 32;

			if (dst_cfg->format == E_OUTPUT_YUV422)
				dst_cfg->lanes = 16;
		}

		if (IS_OUT_4K2K(src_info->de_width, src_info->de_height)) {
			if (IS_VFREQ_60HZ_GROUP(vfreq)) {
				dst_cfg->timing = E_4K2K_60HZ;
				dst_cfg->lanes = 8;
			}
			if (IS_VFREQ_120HZ_GROUP(vfreq)) {
				dst_cfg->timing = E_4K2K_120HZ;
				dst_cfg->lanes = 16;
			}
			if (IS_VFREQ_144HZ_GROUP(vfreq)) {
				dst_cfg->timing = E_4K2K_144HZ;
				dst_cfg->lanes = VB1_16_LANE;
			}
		}
		if (IS_OUT_2K1K(src_info->de_width, src_info->de_height)) {
			if (IS_VFREQ_60HZ_GROUP(vfreq)) {
				dst_cfg->timing = E_FHD_60HZ;
				dst_cfg->lanes = 2;
			}
			if (IS_VFREQ_120HZ_GROUP(vfreq)) {
				dst_cfg->timing = E_FHD_120HZ;
				dst_cfg->lanes = 4;
			}
		}
	}
}

void dump_panel_info(drm_st_pnl_info *pStPnlInfo)
{
	if (!pStPnlInfo) {
		DRM_ERROR("[%s, %d]: Panel INFO is NULL.\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("panel name = %s\n", pStPnlInfo->pnlname);
	DRM_INFO("linkIF (0:None, 1:VB1, 2:LVDS) = %d\n", pStPnlInfo->linkIF);
	DRM_INFO("VBO byte mode = %d\n", pStPnlInfo->vbo_byte);
	DRM_INFO("div_section = %d\n", pStPnlInfo->div_section);
	DRM_INFO("Panel OnTiming1 = %d\n", pStPnlInfo->ontiming_1);
	DRM_INFO("Panel OnTiming2 = %d\n", pStPnlInfo->ontiming_2);
	DRM_INFO("Panel OffTiming1 = %d\n", pStPnlInfo->offtiming_1);
	DRM_INFO("Panel OffTiming2 = %d\n", pStPnlInfo->offtiming_2);
	DRM_INFO("Hsync Start = %d\n", pStPnlInfo->hsync_st);
	DRM_INFO("HSync Width = %d\n", pStPnlInfo->hsync_w);
	DRM_INFO("Hsync Polarity = %d\n", pStPnlInfo->hsync_pol);
	DRM_INFO("Vsync Start = %d\n", pStPnlInfo->vsync_st);
	DRM_INFO("VSync Width = %d\n", pStPnlInfo->vsync_w);
	DRM_INFO("VSync Polarity = %d\n", pStPnlInfo->vsync_pol);
	DRM_INFO("Panel HStart = %d\n", pStPnlInfo->de_hstart);
	DRM_INFO("Panel VStart = %d\n", pStPnlInfo->de_vstart);
	DRM_INFO("Panel Width = %d\n", pStPnlInfo->de_width);
	DRM_INFO("Panel Height = %d\n", pStPnlInfo->de_height);
	DRM_INFO("Panel MaxHTotal = %d\n", pStPnlInfo->max_htt);
	DRM_INFO("Panel HTotal = %d\n", pStPnlInfo->typ_htt);
	DRM_INFO("Panel MinHTotal = %d\n", pStPnlInfo->min_htt);
	DRM_INFO("Panel MaxVTotal = %d\n", pStPnlInfo->max_vtt);
	DRM_INFO("Panel VTotal = %d\n", pStPnlInfo->typ_vtt);
	DRM_INFO("Panel MinVTotal = %d\n", pStPnlInfo->min_vtt);
	DRM_INFO("Panel MaxDCLK = %d\n", pStPnlInfo->max_dclk);
	DRM_INFO("Panel DCLK = %d\n", pStPnlInfo->typ_dclk);
	DRM_INFO("Panel MinDCLK = %d\n", pStPnlInfo->min_dclk);
	DRM_INFO("Output Format = %d\n", pStPnlInfo->op_format);
	DRM_INFO("Content Output Format = %d\n", pStPnlInfo->cop_format);
	DRM_INFO("graphic_mixer1_outmode = %d\n", pStPnlInfo->graphic_mixer1_out_prealpha);
	DRM_INFO("graphic_mixer2_outmode = %d\n", pStPnlInfo->graphic_mixer2_out_prealpha);
}

const char *_print_linktype(drm_pnl_link_if linkIF)
{
	switch (linkIF) {
	case E_LINK_NONE:	return "No Link";
	case E_LINK_VB1:	return "VByOne";
	case E_LINK_LVDS:	return "LVDS";
	case E_LINK_VB1_TO_HDMITX:	return "VB1_TO_HDMITX";
	case E_LINK_MAX:
	default:
		return "NOT Supported link";
	}
}


const char *_print_format(drm_en_pnl_output_format format)
{
	switch (format) {
	case E_OUTPUT_RGB:	return "RGB";
	case E_OUTPUT_YUV444:	return "YUV444";
	case E_OUTPUT_YUV422:	return "YUV422";
	case E_OUTPUT_YUV420:	return "YUV420";
	case E_OUTPUT_ARGB8101010:	return "ARGB8101010";
	case E_OUTPUT_ARGB8888_W_DITHER:	return "ARGB8888 (w Dither)";
	case E_OUTPUT_ARGB8888_W_ROUND: return "ARGB8888 (w Round)";
	case E_OUTPUT_ARGB8888_MODE0:   return "MOD ARGB8888 (mode 0)";
	case E_OUTPUT_ARGB8888_MODE1:   return "MOD ARGB8888 (mode 1)";
	case E_PNL_OUTPUT_FORMAT_NUM:
	default:
		return "NOT Supported Format";
	}
}


const char *_print_timing(drm_output_mode mode)
{
	switch (mode) {
	case E_OUTPUT_MODE_NONE:	return "No Timing";
	case E_8K4K_120HZ:	return "8K4K 120HZ";
	case E_8K4K_60HZ:	return "8K4K 60HZ";
	case E_8K4K_30HZ:	return "8K4K 30HZ";
	case E_4K2K_120HZ:	return "4K2K 120HZ";
	case E_4K2K_144HZ:	return "4K2K 144HZ";
	case E_4K2K_60HZ:	return "4K2K 60HZ";
	case E_4K2K_30HZ:	return "4K2K 30HZ";
	case E_FHD_120HZ:	return "FHD 120HZ";
	case E_FHD_60HZ:	return "FHD 60HZ";
	case E_OUTPUT_MODE_MAX:
	default:
		return "NOT Supported Timing";
	}
}


void dump_mod_cfg(drm_mod_cfg *pCfg)
{
	if (!pCfg) {
		DRM_ERROR("[%s, %d]: drm_mod_cfg is NULL.\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("link IF = %s\n", _print_linktype(pCfg->linkIF));
	DRM_INFO("div_sec = %d\n", pCfg->div_sec);
	DRM_INFO("lanes = %d\n", pCfg->lanes);
	DRM_INFO("format = %s\n", _print_format(pCfg->format));
	DRM_INFO("timing = %s\n", _print_timing(pCfg->timing));
}

int _parse_panel_info(struct device_node *np,
	drm_st_pnl_info *currentPanelInfo,
	struct list_head *panelInfoList)
{
	int ret = 0, i = 0, count = 0;
	struct device_node *infoNp = NULL;
	const char *name = NULL;
	const char *pnlname = NULL;
	const char *pnlInfoName = NULL;
	drm_st_pnl_info *info = NULL;

	if (np != NULL && of_device_is_available(np)) {
		count = of_property_count_strings(np, "panel_info_name_list");

		for (i = 0; i < count; i++) {
			info = kzalloc(sizeof(drm_st_pnl_info), GFP_KERNEL);
			if (!info) {
				ret = -ENOMEM;
				return ret;
			}
			of_property_read_string_index(np, "panel_info_name_list", i, &pnlInfoName);
			infoNp = of_find_node_by_name(np, pnlInfoName);
			if (infoNp != NULL && of_device_is_available(infoNp)) {
				name = NAME_TAG;
				ret = of_property_read_string(infoNp, name, &pnlname);
				if (ret != 0x0) {
					DRM_INFO("[%s, %d]: read string failed, name = %s\n",
						__func__, __LINE__, name);
					return ret;
				}
				memset(info->pnlname, 0, DRM_NAME_MAX_NUM);
				strncpy(info->pnlname, pnlname, DRM_NAME_MAX_NUM - 1);
				info->linkIF = get_dts_u32(infoNp, LINK_TYPE_TAG);
				info->vbo_byte = get_dts_u32(infoNp, VBO_BYTE_TAG);
				info->div_section = get_dts_u32(infoNp, DIV_SECTION_TAG);
				info->ontiming_1 = get_dts_u32(infoNp, ON_TIMING_1_TAG);
				info->ontiming_2 = get_dts_u32(infoNp, ON_TIMING_2_TAG);
				info->offtiming_1 = get_dts_u32(infoNp, OFF_TIMING_1_TAG);
				info->offtiming_2 = get_dts_u32(infoNp, OFF_TIMING_2_TAG);
				info->hsync_st = get_dts_u32(infoNp, HSYNC_ST_TAG);
				info->hsync_w = get_dts_u32(infoNp, HSYNC_WIDTH_TAG);
				info->hsync_pol = get_dts_u32(infoNp, HSYNC_POL_TAG);
				info->vsync_st = get_dts_u32(infoNp, VSYNC_ST_TAG);
				info->vsync_w = get_dts_u32(infoNp, VSYNC_WIDTH_TAG);
				info->vsync_pol = get_dts_u32(infoNp, VSYNC_POL_TAG);
				info->de_hstart = get_dts_u32(infoNp, HSTART_TAG);
				info->de_vstart = get_dts_u32(infoNp, VSTART_TAG);
				info->de_width = get_dts_u32(infoNp, WIDTH_TAG);
				info->de_height = get_dts_u32(infoNp, HEIGHT_TAG);
				info->max_htt = get_dts_u32(infoNp, MAX_HTOTAL_TAG);
				info->typ_htt = get_dts_u32(infoNp, HTOTAL_TAG);
				info->min_htt = get_dts_u32(infoNp, MIN_HTOTAL_TAG);
				info->max_vtt = get_dts_u32(infoNp, MAX_VTOTAL_TAG);
				info->typ_vtt = get_dts_u32(infoNp, VTOTAL_TAG);
				info->min_vtt = get_dts_u32(infoNp, MIN_VTOTAL_TAG);
				info->max_dclk = get_dts_u32(infoNp, MAX_DCLK_TAG);
				info->typ_dclk = get_dts_u32(infoNp, DCLK_TAG);
				info->min_dclk = get_dts_u32(infoNp, MIN_DCLK_TAG);
				info->op_format = get_dts_u32(infoNp, OUTPUT_FMT_TAG);
				info->cop_format = get_dts_u32(infoNp, COUTPUT_FMT_TAG);

				info->vrr_en = get_dts_u32(infoNp, VRR_EN_TAG);
				info->vrr_max_v = get_dts_u32(infoNp, VRR_MAX_TAG);
				info->vrr_min_v = get_dts_u32(infoNp, VRR_MIN_TAG);

				info->graphic_mixer1_out_prealpha = get_dts_u32(infoNp,
									GRAPHIC_MIXER1_OUTMODE_TAG);
				info->graphic_mixer2_out_prealpha = get_dts_u32(infoNp,
									GRAPHIC_MIXER2_OUTMODE_TAG);

			} else {
				info->linkIF = E_LINK_NONE;
				DRM_INFO("Video path is not valid\n");
			}
			setDisplayTiming(info);
			list_add_tail(&info->list, panelInfoList);

			dump_panel_info(info);
		}
	}

	info = NULL;
	info = list_first_entry(panelInfoList, drm_st_pnl_info, list);
	if (!info) {
		DRM_INFO("[%s, %d]: panel info list is NULL\n",
						__func__, __LINE__);
	} else {
		memcpy(currentPanelInfo, info, sizeof(drm_st_pnl_info));
	}

	return ret;
}

bool _isSameModeAndPanelInfo(struct drm_display_mode *mode,
		drm_st_pnl_info *pPanelInfo)
{
	bool isSameFlag = false;
	__u32 hsync_w = 0, vsync_w = 0, de_hstart = 0, de_vstart = 0;
	__u32 hfront_porch = 0, vfront_proch = 0;

	hfront_porch = mode->hsync_start - mode->hdisplay;
	vfront_proch = mode->vsync_start - mode->vdisplay;
	hsync_w = mode->hsync_end - mode->hsync_start;
	vsync_w = mode->vsync_end - mode->vsync_start;
	de_hstart = mode->htotal - mode->hdisplay - hfront_porch;
	de_vstart = mode->vtotal - mode->vdisplay - vfront_proch;

	if (((pPanelInfo->typ_dclk/1000) == mode->clock)
		&& (pPanelInfo->de_width == mode->hdisplay)
		&& (pPanelInfo->typ_htt == mode->htotal)
		&& (pPanelInfo->de_height == mode->vdisplay)
		&& (pPanelInfo->typ_vtt == mode->vtotal)
		&& (pPanelInfo->hsync_w == hsync_w)
		&& (pPanelInfo->vsync_w == vsync_w)
		&& (pPanelInfo->de_hstart == de_hstart)
		&& (pPanelInfo->de_vstart == de_vstart)) {

		isSameFlag = true;
	}
	return isSameFlag;
}

int _copyPanelInfo(struct drm_crtc_state *crtc_state,
	drm_st_pnl_info *pCurrentInfo,
	struct list_head *pInfoList)
{
	drm_st_pnl_info *tmpInfo = NULL;
	int flag = 0;

	list_for_each_entry(tmpInfo, pInfoList, list) {

		if (_isSameModeAndPanelInfo(&crtc_state->mode, tmpInfo)) {
			memcpy(pCurrentInfo, tmpInfo, sizeof(drm_st_pnl_info));
			flag = 1;
			break;
		}

	}

	dump_panel_info(tmpInfo);

	if (!flag) {
		DRM_ERROR("[%s, %d]: panel info does not match.\n",
			__func__, __LINE__);
		return -1;
	} else
		return 0;

}

int _panelAddModes(struct drm_panel *panel,
	struct list_head *pInfoList,
	int *modeCouct)
{
	struct videomode _videomode = {0};
	const struct display_timing *displayTiming = NULL;
	drm_st_pnl_info *tmpInfo = NULL;
	struct drm_device *drmDevice = NULL;
	struct drm_connector *drmConnector = NULL;
	struct drm_display_mode *mode = NULL;

	drmDevice = panel->drm;
	drmConnector = panel->connector;

	list_for_each_entry(tmpInfo, pInfoList, list) {

		displayTiming = &tmpInfo->displayTiming;
		if (displayTiming != NULL) {
			videomode_from_timing(displayTiming, &_videomode);
		} else {
			DRM_WARN("displayTiming NULL\n");
			return 0;
		}
		mode = drm_mode_create(drmDevice);

		if (!mode) {
			DRM_WARN("failed to create a new display mode.\n");

		} else {

			drm_display_mode_from_videomode(&_videomode, mode);

			mode->type = 0;
			mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;

			mode->width_mm = displayTiming->hactive.typ;
			mode->height_mm = displayTiming->vactive.typ;


			drm_mode_probed_add(drmConnector, mode);
			(*modeCouct)++;

			if (displayTiming->hactive.typ > drmConnector->display_info.width_mm) {
				drmConnector->display_info.width_mm = displayTiming->hactive.typ;
				drmConnector->display_info.height_mm = displayTiming->vactive.typ;
			}
		}
	}
	return 0;
}
