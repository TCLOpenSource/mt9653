// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "mtk_tv_drm_video_panel.h"
#include "../mtk_tv_lpll_tbl.h"
#include "../mtk_tv_drm_log.h"
#include "../mtk_tv_drm_kms.h"
#include "hwreg_render_video_pnl.h"
#include "hwreg_render_common_trigger_gen.h"
#include "common/hwreg_common_riu.h"
#include "pixelmonitor.h"
#include "drv_scriptmgt.h"
#include "mtk_tv_drm_common.h"


#define MTK_DRM_MODEL MTK_DRM_MODEL_PANEL
#define CONNECTID_MAIN (0)
#define CONNECTID_GFX (1)
#define CONNECTID_DELTA (2)
#define VFRQRATIO_10 (10)
#define VFRQRATIO_1000 (1000)
#define MOD_TOP_CLK (594)
#define MOD_TOP_CLK_720MHZ (720)

#define IS_INPUT_B2R(x)     ((x == META_PQ_INPUTSRC_DTV) || (x == META_PQ_INPUTSRC_STORAGE) || \
				(x == META_PQ_INPUTSRC_STORAGE))
#define IS_INPUT_SRCCAP(x)     ((x == META_PQ_INPUTSRC_HDMI) || (x == META_PQ_INPUTSRC_VGA) || \
				(x == META_PQ_INPUTSRC_TV) || (x == META_PQ_INPUTSRC_YPBPR) || \
				(x == META_PQ_INPUTSRC_CVBS))

#define LANE_NUM_16 (16)
//chip version
#define LPLL_VER_2 (2)
#define MOD_VER_2 (2)
//-------------------------------------------------------------------------------------------------
// Prototype
//-------------------------------------------------------------------------------------------------
static int mtk_tv_drm_panel_disable(struct drm_panel *panel);
static int mtk_tv_drm_panel_unprepare(struct drm_panel *panel);
static int mtk_tv_drm_panel_prepare(struct drm_panel *panel);
static int mtk_tv_drm_panel_enable(struct drm_panel *panel);
static int mtk_tv_drm_panel_get_modes(struct drm_panel *panel);
static int mtk_tv_drm_panel_get_timings(struct drm_panel *panel,
						unsigned int num_timings,
						struct display_timing *timings);

static int mtk_tv_drm_panel_probe(struct platform_device *pdev);
static int mtk_tv_drm_panel_remove(struct platform_device *pdev);
static int mtk_tv_drm_panel_suspend(struct platform_device *pdev, pm_message_t state);
static int mtk_tv_drm_panel_resume(struct platform_device *pdev);
static int mtk_tv_drm_panel_bind(
	struct device *compDev,
	struct device *master,
	void *masterData);
static void mtk_tv_drm_panel_unbind(
	struct device *compDev,
	struct device *master,
	void *masterData);
#ifdef CONFIG_PM
static int panel_runtime_suspend(struct device *dev);
static int panel_runtime_resume(struct device *dev);
static int panel_pm_runtime_force_suspend(struct device *dev);
static int panel_pm_runtime_force_resume(struct device *dev);
#endif
__u64 u64PreTypDclk;
drm_en_pnl_output_format ePreOutputFormat = E_PNL_OUTPUT_FORMAT_NUM;
drm_en_vbo_bytemode ePreByteMode = E_VBO_MODE_MAX;

//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------
static const struct dev_pm_ops panel_pm_ops = {
	SET_RUNTIME_PM_OPS(panel_runtime_suspend,
			   panel_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(panel_pm_runtime_force_suspend,
				panel_pm_runtime_force_resume)
};

static const struct drm_panel_funcs drmPanelFuncs = {
	.disable = mtk_tv_drm_panel_disable,
	.unprepare = mtk_tv_drm_panel_unprepare,
	.prepare = mtk_tv_drm_panel_prepare,
	.enable = mtk_tv_drm_panel_enable,
	.get_modes = mtk_tv_drm_panel_get_modes,
	.get_timings = mtk_tv_drm_panel_get_timings,
};

static const struct of_device_id mtk_tv_drm_panel_of_match_table[] = {
	{ .compatible = "mtk,mt5896-panel",},
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(of, mtk_tv_drm_panel_of_match_table);

struct platform_driver mtk_tv_drm_panel_driver = {
	.probe		= mtk_tv_drm_panel_probe,
	.remove		= mtk_tv_drm_panel_remove,
	.suspend	= mtk_tv_drm_panel_suspend,
	.resume		= mtk_tv_drm_panel_resume,
	.driver = {
			.name = "video_out",
			.owner  = THIS_MODULE,
			.of_match_table = of_match_ptr(mtk_tv_drm_panel_of_match_table),
			.pm = &panel_pm_ops,
			},
};

static const struct component_ops mtk_tv_drm_panel_component_ops = {
	.bind	= mtk_tv_drm_panel_bind,
	.unbind = mtk_tv_drm_panel_unbind,
};

struct mtk_lpll_table_info {
	E_PNL_SUPPORTED_LPLL_TYPE table_start;
	E_PNL_SUPPORTED_LPLL_TYPE table_end;
	uint16_t panel_num;
	uint16_t lpll_reg_num;
	uint16_t mpll_reg_num;
	uint16_t moda_reg_num;
};

struct mtk_lpll_type {
	E_PNL_SUPPORTED_LPLL_TYPE lpll_type;
	E_PNL_SUPPORTED_LPLL_TYPE lpll_tbl_idx;
};

//-------------------------------------------------------------------------------------------------
// Function
//-------------------------------------------------------------------------------------------------

//bring up start
static void _get_table_info(struct mtk_panel_context *pctx, struct mtk_lpll_table_info *tbl_info)
{
	uint32_t lpll_version = 0;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
	} else {
		lpll_version = pctx->hw_info.lpll_hw_version;

		if (lpll_version == LPLL_VER_2) {
			tbl_info->table_start = E_PNL_SUPPORTED_LPLL_LVDS_1ch_450to600MHz_VER2;
			tbl_info->table_end =
			E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_5BYTE_Xtal_mode_0to583_2MHz_VER2;
			tbl_info->panel_num = PNL_NUM_VER2;
			tbl_info->lpll_reg_num = LPLL_REG_NUM_VER2;
			tbl_info->mpll_reg_num = MPLL_REG_NUM_VER2;
			tbl_info->moda_reg_num = MODA_REG_NUM_VER2;
		} else {
			tbl_info->table_start = E_PNL_SUPPORTED_LPLL_LVDS_1ch_450to600MHz;
			tbl_info->table_end =
			E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_5BYTE_300to300MHz;
			tbl_info->panel_num = PNL_NUM_VER1;
			tbl_info->lpll_reg_num = LPLL_REG_NUM;
			tbl_info->mpll_reg_num = 0;
			tbl_info->moda_reg_num = 0;
		}
	}
}

static struct mtk_lpll_type _get_link_idx(struct mtk_panel_context *pctx)
{
	uint32_t lpll_version = 0;
	struct mtk_lpll_type lpll_name;

	lpll_name.lpll_tbl_idx = E_PNL_SUPPORTED_LPLL_MAX;
	lpll_name.lpll_type = E_PNL_SUPPORTED_LPLL_MAX;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		lpll_name.lpll_tbl_idx = E_PNL_SUPPORTED_LPLL_MAX;
		lpll_name.lpll_type = E_PNL_SUPPORTED_LPLL_MAX;
		return lpll_name;
	}

	lpll_version = pctx->hw_info.lpll_hw_version;
	if (lpll_version == LPLL_VER_2) {
		struct mtk_lpll_table_info lpll_tbl_info;

		_get_table_info(pctx, &lpll_tbl_info);
		if (pctx->v_cfg.linkIF == E_LINK_LVDS) {
			lpll_name.lpll_tbl_idx =
				E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz_VER2 -
				lpll_tbl_info.table_start;

			lpll_name.lpll_type =  E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz_VER2;

		} else if (pctx->v_cfg.linkIF == E_LINK_VB1) {
			if (pctx->v_cfg.timing == E_4K2K_144HZ) {

				lpll_name.lpll_tbl_idx =
				E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_340to720MHz_VER2 -
				lpll_tbl_info.table_start;

				lpll_name.lpll_type =
				E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_340to720MHz_VER2;
			} else {

				lpll_name.lpll_tbl_idx =
				E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz_VER2 -
				lpll_tbl_info.table_start;

				lpll_name.lpll_type =
				E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz_VER2;
			}
		} else {
			DRM_INFO("[%s][%d] Link type Not Support\n", __func__, __LINE__);
		}
	} else {
		if (pctx->v_cfg.linkIF == E_LINK_LVDS) {
			lpll_name.lpll_tbl_idx = E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz;
			lpll_name.lpll_type =  E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz;
		} else if (pctx->v_cfg.linkIF == E_LINK_VB1) {
			lpll_name.lpll_tbl_idx =
				E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz;

			lpll_name.lpll_type =
				E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz;
		} else {
			DRM_INFO("[%s][%d] Link type Not Support\n", __func__, __LINE__);
		}
	}
	return lpll_name;
}

static void _mtk_load_lpll_tbl(struct mtk_panel_context *pctx)
{
	uint32_t version = 0;
	uint16_t idx = 0;
	uint16_t panelNum = 0;
	E_PNL_SUPPORTED_LPLL_TYPE tblidx = 0;
	struct mtk_lpll_table_info lpll_tbl_info;
	struct mtk_lpll_type lpll_name;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	version = pctx->hw_info.lpll_hw_version;

	lpll_name = _get_link_idx(pctx);
	tblidx = lpll_name.lpll_tbl_idx;

	_get_table_info(pctx, &lpll_tbl_info);
	panelNum = lpll_tbl_info.panel_num;

	DRM_INFO("[%s][%d] lpll_tbl_ver = %d\n", __func__, __LINE__, version);
	DRM_INFO("[%s][%d] lpll_tbl_idx = %d\n", __func__, __LINE__, tblidx);
	DRM_INFO("[%s][%d] lpll_tbl_panelNum = %d\n", __func__, __LINE__, panelNum);

	drv_hwreg_render_pnl_load_lpll_tbl(version, tblidx, panelNum);
}

static void _mtk_load_mpll_tbl(struct mtk_panel_context *pctx)
{
	uint32_t version = 0;
	uint16_t idx = 0;
	uint16_t regNum = 0;
	uint16_t panelNum = 0;
	E_PNL_SUPPORTED_LPLL_TYPE tblidx = 0;
	struct mtk_lpll_table_info lpll_tbl_info;
	struct mtk_lpll_type lpll_name;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	version = pctx->hw_info.lpll_hw_version;

	lpll_name = _get_link_idx(pctx);
	tblidx = lpll_name.lpll_tbl_idx;

	_get_table_info(pctx, &lpll_tbl_info);
	regNum = lpll_tbl_info.mpll_reg_num;
	panelNum = lpll_tbl_info.panel_num;

	DRM_INFO("[%s][%d] mpll_tbl_ver = %d\n", __func__, __LINE__, version);
	DRM_INFO("[%s][%d] mpll_tbl_idx = %d\n", __func__, __LINE__, tblidx);
	DRM_INFO("[%s][%d] mpll_tbl_regNum = %d\n", __func__, __LINE__, regNum);
	DRM_INFO("[%s][%d] mpll_tbl_panelNum = %d\n", __func__, __LINE__, panelNum);

	drv_hwreg_render_pnl_load_mpll_tbl(version, tblidx, regNum, panelNum);
}

static void _mtk_load_moda_tbl(struct mtk_panel_context *pctx)
{
	uint32_t version = 0;
	uint16_t idx = 0;
	uint16_t regNum = 0;
	uint16_t panelNum = 0;
	E_PNL_SUPPORTED_LPLL_TYPE tblidx = 0;
	struct mtk_lpll_table_info lpll_tbl_info;
	struct mtk_lpll_type lpll_name;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	version = pctx->hw_info.lpll_hw_version;

	lpll_name = _get_link_idx(pctx);
	tblidx = lpll_name.lpll_tbl_idx;

	_get_table_info(pctx, &lpll_tbl_info);
	regNum = lpll_tbl_info.moda_reg_num;
	panelNum = lpll_tbl_info.panel_num;

	DRM_INFO("[%s][%d] moda_tbl_ver = %d\n", __func__, __LINE__, version);
	DRM_INFO("[%s][%d] moda_tbl_idx = %d\n", __func__, __LINE__, tblidx);
	DRM_INFO("[%s][%d] moda_tbl_regNum = %d\n", __func__, __LINE__, regNum);
	DRM_INFO("[%s][%d] moda_tbl_panelNum = %d\n", __func__, __LINE__, panelNum);

	drv_hwreg_render_pnl_load_moda_tbl(version, tblidx, regNum, panelNum);
}

static void _mtk_odclk_setting(struct mtk_panel_context *pctx)
{
	uint32_t version = 0;
	uint32_t odclk = 0;
	E_PNL_SUPPORTED_LPLL_TYPE tblidx = 0;
	E_PNL_SUPPORTED_LPLL_TYPE lpll_enum = 0;
	struct mtk_lpll_type lpll_name;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	version = pctx->hw_info.lpll_hw_version;

	lpll_name = _get_link_idx(pctx);
	tblidx = lpll_name.lpll_tbl_idx;
	lpll_enum = lpll_name.lpll_type;

	DRM_INFO("[%s][%d] odclk_tbl_ver = %d\n", __func__, __LINE__, version);
	DRM_INFO("[%s][%d] odclk_tbl_idx = %d\n", __func__, __LINE__, tblidx);
	DRM_INFO("[%s][%d] odclk_tbl_enum = %d\n", __func__, __LINE__, lpll_enum);

	odclk = MOD_TOP_CLK;
	if (version == LPLL_VER_2) {
		if ((lpll_enum ==
			E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_3BYTE_340to720MHz_VER2) ||
			(lpll_enum ==
			E_PNL_SUPPORTED_LPLL_Vx1_Video_3BYTE_OSD_4BYTE_340to720MHz_VER2) ||
			(lpll_enum ==
			E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_3BYTE_340to720MHz_VER2) ||
			(lpll_enum ==
			E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_4BYTE_340to720MHz_VER2)) {
			odclk = MOD_TOP_CLK_720MHZ;
		}
	}

	drv_hwreg_render_pnl_set_odclk(version, tblidx, odclk);
}

//bring up end

static void _mtk_moda_path_setting(struct mtk_panel_context *pctx)
{
	uint32_t version = 0;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}
	version = pctx->hw_info.mod_hw_version;
	if (version == MOD_VER_2) {
		bool path1EN = false, path2EN = false;

		if (pctx->v_cfg.linkIF == E_LINK_VB1) {
			if ((pctx->info.vbo_byte == E_VBO_5BYTE_MODE) &&
					(pctx->ext_grpah_combo_info.graph_vbo_byte_mode ==
						E_VBO_5BYTE_MODE)) {
				path1EN = false;
			} else {
				path1EN = true;
			}

			if ((pctx->info.vbo_byte == E_VBO_5BYTE_MODE) ||
					(pctx->ext_grpah_combo_info.graph_vbo_byte_mode ==
						E_VBO_5BYTE_MODE)) {
				path2EN = true;
			} else {
				path2EN = false;
			}

		} else {
			path1EN = true;
			path2EN = false;
		}
		drv_hwreg_render_pnl_analog_path_setting(path1EN, path2EN);
	}

}

static struct mtk_drm_panel_context *_get_gfx_panel_ctx(void)
{
	struct device_node *deviceNodePanel = NULL;
	struct drm_panel *pDrmPanel = NULL;
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

	deviceNodePanel = of_find_node_by_name(NULL, GFX_NODE);
	if (deviceNodePanel != NULL && of_device_is_available(deviceNodePanel)) {
		pDrmPanel = of_drm_find_panel(deviceNodePanel);
		pStPanelCtx = container_of(pDrmPanel, struct mtk_drm_panel_context, drmPanel);
	}
	return pStPanelCtx;
}

static struct mtk_drm_panel_context *_get_extdev_panel_ctx(void)
{
	struct device_node *deviceNodePanel = NULL;
	struct drm_panel *pDrmPanel = NULL;
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

	deviceNodePanel = of_find_node_by_name(NULL, EXTDEV_NODE);
	if (deviceNodePanel != NULL && of_device_is_available(deviceNodePanel)) {
		pDrmPanel = of_drm_find_panel(deviceNodePanel);
		pStPanelCtx = container_of(pDrmPanel, struct mtk_drm_panel_context, drmPanel);
	}
	return pStPanelCtx;
}

static void _mtk_cfg_ckg_common(struct mtk_panel_context *pctx)
{
	if (pctx == NULL)
		return;

	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		//clock setting
		drv_hwreg_render_pnl_set_cfg_ckg_common();
	}
}

void _vb1_set_mft(struct mtk_panel_context *pctx)
{
	#define H_DIVIDER_FOR_YUV422 2
	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	if (pctx->v_cfg.format == E_OUTPUT_YUV422) {
		drv_hwreg_render_pnl_set_vby1_mft(
			(en_drv_pnl_link_if)pctx->v_cfg.linkIF,
			pctx->v_cfg.div_sec,
			pctx->v_cfg.lanes,
			(pctx->info.de_width)/H_DIVIDER_FOR_YUV422);
	} else {
		drv_hwreg_render_pnl_set_vby1_mft(
			(en_drv_pnl_link_if)pctx->v_cfg.linkIF,
			pctx->v_cfg.div_sec,
			pctx->v_cfg.lanes,
			pctx->info.de_width);
	}
	#undef H_DIVIDER_FOR_YUV422
}

void _vb1_v_8k_setting(struct mtk_panel_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	if (pctx->v_cfg.format == E_OUTPUT_YUV422)
		drv_hwreg_render_pnl_set_vby1_v_8k422_60HZ();
	else
		drv_hwreg_render_pnl_set_vby1_v_8k_60HZ();
}

void _vb1_seri_data_sel(struct mtk_panel_context *pctx)
{
	__u8 u8GfxLanes = 0;
	__u8 u8ExtdevLanes = 0;
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	pStPanelCtx = _get_gfx_panel_ctx();
	if (pStPanelCtx != NULL)
		u8GfxLanes = pStPanelCtx->cfg.lanes;

	pStPanelCtx = _get_extdev_panel_ctx();
	if (pStPanelCtx != NULL)
		u8ExtdevLanes = pStPanelCtx->cfg.lanes;


	if (pctx->v_cfg.linkIF == E_LINK_LVDS)
		drv_hwreg_render_pnl_set_vby1_seri_data_sel(0x0);

	if (pctx->v_cfg.lanes == 32)
		drv_hwreg_render_pnl_set_vby1_seri_data_sel(0x1);

	if (pctx->v_cfg.lanes == 16 ||
		(u8ExtdevLanes == 16 &&
		u8GfxLanes == 8)) {
		drv_hwreg_render_pnl_set_vby1_seri_data_sel(0x2);
	}

	if (pctx->v_cfg.lanes == 64)
		drv_hwreg_render_pnl_set_vby1_seri_data_sel(0x3);
}

drm_en_vbo_bytemode _vb1_set_byte_mode_decision(struct mtk_panel_context *pctx)
{
	uint32_t mod_version = 0;
	drm_en_vbo_bytemode ret = E_VBO_NO_LINK;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return ret;
	}

	mod_version = pctx->hw_info.mod_hw_version;

	if (mod_version == MOD_VER_2) {
		ret = pctx->info.vbo_byte;
	} else {
		if ((pctx->v_cfg.timing == E_8K4K_60HZ) &&
				(pctx->v_cfg.format == E_OUTPUT_YUV422)) {
			ret = E_VBO_5BYTE_MODE;
		} else {
			ret = E_VBO_4BYTE_MODE;
		}
	}
	return ret;
}

void _vb1_set_byte_mode(struct mtk_panel_context *pctx)
{
	drm_en_vbo_bytemode ret = _vb1_set_byte_mode_decision(pctx);

	drv_hwreg_render_pnl_set_vby1_byte_mode((en_drv_pnl_vbo_bytemode)ret);
}

static void _mtk_vby1_setting(struct mtk_panel_context *pctx)
{
	uint32_t mod_version = 0;
	uint8_t output_format = 0;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	mod_version = pctx->hw_info.mod_hw_version;
	output_format = (uint8_t)pctx->v_cfg.format;

	DRM_INFO("[%s][%d] pctx->v_cfg.linkIF= %d pctx->v_cfg.timing =%d\n",
			__func__, __LINE__, pctx->v_cfg.linkIF, pctx->v_cfg.timing);

	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		switch (pctx->v_cfg.timing) {

		case E_FHD_60HZ:
			if (pctx->v_cfg.linkIF == E_LINK_LVDS)
				drv_hwreg_render_pnl_set_lvds_v_fhd_60HZ(mod_version);
			else
				drv_hwreg_render_pnl_set_vby1_v_fhd_60HZ(mod_version);
		break;
		case E_FHD_120HZ:
			drv_hwreg_render_pnl_set_vby1_v_fhd_120HZ(mod_version);
		break;
		case E_4K2K_60HZ:
			drv_hwreg_render_pnl_set_vby1_v_4k_60HZ(mod_version);
		break;
		case E_4K2K_120HZ:
		case E_4K2K_144HZ:
			drv_hwreg_render_pnl_set_vby1_v_4k_120HZ(mod_version);
		break;
		case E_8K4K_60HZ:
			_vb1_v_8k_setting(pctx);
		break;
		default:
			DRM_INFO("[%s][%d] Video OUTPUT TIMING Not Support\n", __func__, __LINE__);
		break;
		}

		//ToDo:Add Fixed H-backporch code.

		//VBY1 byte-mode
		_vb1_set_byte_mode(pctx);
	}

	DRM_INFO("[%s][%d]pctx->v_cfg.linkIF = %d\n", __func__, __LINE__, pctx->v_cfg.linkIF);

	_vb1_set_mft(pctx);
	drv_hwreg_render_pnl_set_sctcon_misc(output_format);

	//serializer data sel
	if (pctx->v_cfg.linkIF == E_LINK_LVDS)
		drv_hwreg_render_pnl_set_vby1_seri_data_sel(0x0);
	else
		_vb1_seri_data_sel(pctx);
}

void _mtk_vby1_lane_order(struct mtk_panel_context *pctx)
{
	uint32_t hwversion = 0;
	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}
	st_drv_out_lane_order stPnlOutLaneOrder;

	memset(&stPnlOutLaneOrder, 0, sizeof(st_drv_out_lane_order));
	memcpy(&stPnlOutLaneOrder, &pctx->out_lane_info, sizeof(st_drv_out_lane_order));

	hwversion = pctx->hw_info.mod_hw_version;

	if (pctx->v_cfg.linkIF == E_LINK_VB1) {
		drv_hwreg_render_pnl_set_vby1_lane_order(
			(en_drv_pnl_link_if)pctx->v_cfg.linkIF,
			(en_drv_pnl_output_mode)pctx->v_cfg.timing,
			(en_drv_pnl_output_format)pctx->v_cfg.format,
			pctx->v_cfg.div_sec,
			pctx->v_cfg.lanes,
			&stPnlOutLaneOrder);
	}

	if (pctx->v_cfg.linkIF == E_LINK_LVDS) {
		drv_hwreg_render_pnl_set_lvds_lane_order(hwversion, 0x0);
	}
}

void mtk_render_output_en(struct mtk_panel_context *pctx, bool bEn)
{
	uint32_t hwversion = 0;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	st_drv_out_lane_order stPnlOutLaneOrder;

	memset(&stPnlOutLaneOrder, 0, sizeof(st_drv_out_lane_order));
	memcpy(&stPnlOutLaneOrder, &pctx->out_lane_info, sizeof(st_drv_out_lane_order));

	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		hwversion = pctx->hw_info.mod_hw_version;
	    //video
		drv_hwreg_render_pnl_set_render_output_en_video(
					hwversion,
					bEn,
					(en_drv_pnl_output_mode)pctx->v_cfg.timing,
					(en_drv_pnl_output_format)pctx->v_cfg.format,
					(en_drv_pnl_link_if)pctx->v_cfg.linkIF,
					&stPnlOutLaneOrder);
	}
}

int mtk_render_set_gop_pattern_en(bool bEn)
{
	return drv_hwreg_render_pnl_set_gop_pattern(bEn);
}

int mtk_render_set_multiwin_pattern_en(bool bEn)
{
	return drv_hwreg_render_pnl_set_multiwin_pattern(bEn);
}

int mtk_render_set_tcon_pattern_en(bool bEn)
{
	return drv_hwreg_render_pnl_set_tcon_pattern(bEn);
}


int mtk_render_set_output_hmirror_enable(struct mtk_panel_context *pctx)
{
	int Ret = -EINVAL;
	bool hmirror_en = false;
	uint32_t hwversion = 0;


	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return Ret;
	}

	hmirror_en = pctx->cus.hmirror_en;
	hwversion = pctx->hw_info.mod_hw_version;

	drv_hwreg_render_pnl_set_vby1_mft_hmirror_video(hmirror_en);
	if (hwversion != MOD_VER_2)
		drv_hwreg_render_pnl_set_vby1_mft_hmirror_deltav(hmirror_en);
	drv_hwreg_render_pnl_set_vby1_mft_hmirror_gfx(hmirror_en);

	if (hwversion == MOD_VER_2) {
		if (drv_hwreg_render_pnl_get_vby1_mft_hmirror_video() != hmirror_en ||
			drv_hwreg_render_pnl_get_vby1_mft_hmirror_gfx() != hmirror_en) {
			return -EINVAL;
		}
	} else {
		if (drv_hwreg_render_pnl_get_vby1_mft_hmirror_video() != hmirror_en ||
			drv_hwreg_render_pnl_get_vby1_mft_hmirror_deltav() != hmirror_en ||
			drv_hwreg_render_pnl_get_vby1_mft_hmirror_gfx() != hmirror_en) {
			return -EINVAL;
		}
	}

	DRM_INFO("[%s, %d]: Mirror mode set done\n", __func__, __LINE__);

	return 0;
}


static void _mtk_tgen_init(struct mtk_panel_context *pctx, uint8_t out_clk_if)
{
	st_drv_pnl_info stPnlInfoTmp = {0};
	uint32_t hwversion = 0;

	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	hwversion = pctx->hw_info.tgen_hw_version;

	stPnlInfoTmp.version = pctx->info.version;
	stPnlInfoTmp.length = pctx->info.length;

	strncpy(stPnlInfoTmp.pnlname, pctx->info.pnlname, DRV_PNL_NAME_MAX_NUM-1);

	stPnlInfoTmp.linkIf = (en_drv_pnl_link_if)pctx->info.linkIF;
	stPnlInfoTmp.div_section = pctx->info.div_section;

	stPnlInfoTmp.ontiming_1 = pctx->info.ontiming_1;
	stPnlInfoTmp.ontiming_2 = pctx->info.ontiming_2;
	stPnlInfoTmp.offtiming_1 = pctx->info.offtiming_1;
	stPnlInfoTmp.offtiming_2 = pctx->info.offtiming_2;

	stPnlInfoTmp.hsync_st = pctx->info.hsync_st;
	stPnlInfoTmp.hsync_w = pctx->info.hsync_w;
	stPnlInfoTmp.hsync_pol = pctx->info.hsync_pol;

	stPnlInfoTmp.vsync_st = pctx->info.vsync_st;
	stPnlInfoTmp.vsync_w = pctx->info.vsync_w;
	stPnlInfoTmp.vsync_pol = pctx->info.vsync_pol;

	stPnlInfoTmp.de_hstart = pctx->info.de_hstart;
	stPnlInfoTmp.de_vstart = pctx->info.de_vstart;
	stPnlInfoTmp.de_width = pctx->info.de_width;
	stPnlInfoTmp.de_height = pctx->info.de_height;

	stPnlInfoTmp.max_htt = pctx->info.max_htt;
	stPnlInfoTmp.typ_htt = pctx->info.typ_htt;
	stPnlInfoTmp.min_htt = pctx->info.min_htt;

	stPnlInfoTmp.max_vtt = pctx->info.max_vtt;
	stPnlInfoTmp.typ_vtt = pctx->info.typ_vtt;
	stPnlInfoTmp.min_vtt = pctx->info.min_vtt;

	stPnlInfoTmp.max_dclk = pctx->info.max_dclk;
	stPnlInfoTmp.typ_dclk = pctx->info.typ_dclk;
	stPnlInfoTmp.min_dclk = pctx->info.min_dclk;

	stPnlInfoTmp.op_format = (en_drv_pnl_output_format)pctx->info.op_format;

	stPnlInfoTmp.vrr_en = pctx->info.vrr_en;
	stPnlInfoTmp.vrr_max_v = pctx->info.vrr_max_v;
	stPnlInfoTmp.vrr_min_v = pctx->info.vrr_min_v;

	stPnlInfoTmp.lane_num = out_clk_if;


	drv_hwreg_render_pnl_tgen_init(hwversion, &stPnlInfoTmp);
}
static void _mtk_mft_hbkproch_protect_init(struct mtk_panel_context *pctx)
{
	st_drv_hprotect_info stHprotectInfoTmp = {0};

	stHprotectInfoTmp.hsync_w =  pctx->info.hsync_w;
	stHprotectInfoTmp.de_hstart =  pctx->info.de_hstart;
	stHprotectInfoTmp.lane_numbers = pctx->v_cfg.lanes;
	stHprotectInfoTmp.op_format = pctx->info.op_format;

	drv_hwreg_render_pnl_hbkproch_protect_init(&stHprotectInfoTmp);

}
static void _dump_mod_cfg(struct mtk_panel_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("================V_MOD_CFG================\n");
	dump_mod_cfg(&pctx->v_cfg);
}

static void _init_mod_cfg(struct mtk_panel_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	init_mod_cfg(&pctx->v_cfg);
}

int mtk_tv_drm_check_out_mod_cfg(struct mtk_panel_context *pctx_pnl,
						drm_parse_dts_info *data)
{
	drm_mod_cfg out_cfg = {0};

	if (pctx_pnl == NULL) {
		DRM_INFO("[%s] param is NULL\n", __func__);
		return -EINVAL;
	}

	memset(&out_cfg, 0, sizeof(drm_mod_cfg));

	set_out_mod_cfg(&data->src_info, &out_cfg);

	DRM_INFO("[%s] calculated out_cfg.lanes is %d target = %d\n"
			, __func__, out_cfg.lanes, data->out_cfg.lanes);
	DRM_INFO("[%s] calculated out_cfg.timing is %d target = %d\n"
			, __func__, out_cfg.timing, data->out_cfg.timing);

	if (out_cfg.lanes == data->out_cfg.lanes &&
		out_cfg.timing == data->out_cfg.timing) {

		return 0;
	} else {

		return -1;
	}

}

int mtk_tv_drm_check_out_lane_cfg(struct mtk_panel_context *pctx_pnl)
{
	uint32_t sup_lanes = 0;
	uint32_t def_layout = 0;
	uint32_t hw_read_layout = 0;
	uint8_t index = 0;
	int ret = 0;

	if (pctx_pnl == NULL) {
		DRM_INFO("[%s] param is NULL\n", __func__);
		return -EINVAL;
	}
	sup_lanes = pctx_pnl->out_lane_info.sup_lanes;

	for (index = 0; index < sup_lanes; index++) {
		def_layout = pctx_pnl->out_lane_info.def_layout[index];
		hw_read_layout = drv_hwreg_render_pnl_get_vby1_swap(index);
		if (def_layout != hw_read_layout) {
			ret = -1;
			break;
		}
	}
	return ret;
}

int mtk_tv_drm_check_video_hbproch_protect(drm_parse_dts_info *data)
{
	st_drv_hprotect_info tmpVideoHbkprotectInfo;
	st_drv_hprotect_info tmpDeltaHbkprotectInfo;
	st_drv_hprotect_info tmpGfxHbkprotectInfo;
	int ret = 0;

	tmpVideoHbkprotectInfo.hsync_w = data->video_hprotect_info.hsync_w;
	tmpVideoHbkprotectInfo.de_hstart = data->video_hprotect_info.de_hstart;
	tmpVideoHbkprotectInfo.lane_numbers = data->video_hprotect_info.lane_numbers;
	tmpVideoHbkprotectInfo.op_format = data->video_hprotect_info.op_format;

	tmpDeltaHbkprotectInfo.hsync_w = data->delta_hprotect_info.hsync_w;
	tmpDeltaHbkprotectInfo.de_hstart = data->delta_hprotect_info.de_hstart;
	tmpDeltaHbkprotectInfo.lane_numbers = data->delta_hprotect_info.lane_numbers;
	tmpDeltaHbkprotectInfo.op_format = data->delta_hprotect_info.op_format;

	tmpGfxHbkprotectInfo.hsync_w = data->gfx_hprotect_info.hsync_w;
	tmpGfxHbkprotectInfo.de_hstart = data->gfx_hprotect_info.de_hstart;
	tmpGfxHbkprotectInfo.lane_numbers = data->gfx_hprotect_info.lane_numbers;
	tmpGfxHbkprotectInfo.op_format = data->gfx_hprotect_info.op_format;

	drv_hwreg_render_pnl_hbkproch_protect_init(&tmpVideoHbkprotectInfo);
	drv_hwreg_render_pnl_delta_hbkproch_protect_init(&tmpDeltaHbkprotectInfo);
	drv_hwreg_render_pnl_gfx_hbkproch_protect_init(&tmpGfxHbkprotectInfo);

	return ret;
}

int mtk_render_set_tx_mute(struct mtk_panel_context *pctx_pnl,
							drm_st_tx_mute_info tx_mute_info)
{
	#define PNL_V1 0
	#define PNL_V2 1
	#define PNL_OSD 2
	int Ret = -EINVAL;
	bool bReadBack = 0;
	uint32_t hw_version = 0;

	if (pctx_pnl == NULL) {
		DRM_INFO("[%s] param is NULL\n", __func__);
		return -EINVAL;
	}

	hw_version = pctx_pnl->hw_info.tgen_hw_version;

	if (tx_mute_info.u8ConnectorID == PNL_V1) {
		//video
		Ret = drv_hwreg_render_pnl_set_tx_mute_en_video(tx_mute_info.bEnable);
		bReadBack = drv_hwreg_render_pnl_get_tx_mute_en_video();
		DRM_INFO("%s V1:%d\n", __func__, bReadBack);
	}

	if (hw_version == MOD_VER_2) {
		// not support
		Ret = 0;
	} else {
		if (tx_mute_info.u8ConnectorID == PNL_V2) {
			//deltav
			Ret = drv_hwreg_render_pnl_set_tx_mute_en_deltav(tx_mute_info.bEnable);
			bReadBack = drv_hwreg_render_pnl_get_tx_mute_en_deltav();
			DRM_INFO("%s V2:%d\n", __func__, bReadBack);
		}
	}

	if (tx_mute_info.u8ConnectorID == PNL_OSD) {
		//gfx
		Ret = drv_hwreg_render_pnl_set_tx_mute_en_gfx(tx_mute_info.bEnable);
		bReadBack = drv_hwreg_render_pnl_get_tx_mute_en_gfx();
		DRM_INFO("%s OSD:%d\n", __func__, bReadBack);
	}

	if ((hw_version != MOD_VER_2) || (tx_mute_info.u8ConnectorID != PNL_V2)) {
		if (tx_mute_info.bEnable != bReadBack) {
			DRM_ERROR("[%s, %d]: TX mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}
	}

	#undef PNL_V1
	#undef PNL_V2
	#undef PNL_OSD

	return Ret;
}

int mtk_render_set_tx_mute_common(struct mtk_panel_context *pctx_pnl,
	drm_st_tx_mute_info tx_mute_info,
	uint8_t u8connector_id)
{
	#define PNL_V1 0
	#define PNL_V2 1
	#define PNL_OSD 2
	int Ret = -EINVAL;
	bool bReadBack = 0;
	uint32_t hw_version = 0;

	if (pctx_pnl == NULL) {
		DRM_INFO("[%s] param is NULL\n", __func__);
		return -EINVAL;
	}

	hw_version = pctx_pnl->hw_info.tgen_hw_version;

	if (u8connector_id == MTK_DRM_CONNECTOR_TYPE_VIDEO) {
		//video
		Ret = drv_hwreg_render_pnl_set_tx_mute_en_video(tx_mute_info.bEnable);
		bReadBack = drv_hwreg_render_pnl_get_tx_mute_en_video();
		DRM_INFO("%s V1:%d\n", __func__, bReadBack);
	}

	if (hw_version == MOD_VER_2) {
		// not support
		Ret = 0;
	} else {
		if (u8connector_id == MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO) {
			//deltav
			Ret = drv_hwreg_render_pnl_set_tx_mute_en_deltav(tx_mute_info.bEnable);
			bReadBack = drv_hwreg_render_pnl_get_tx_mute_en_deltav();
			DRM_INFO("%s V2:%d\n", __func__, bReadBack);
		}
	}

	if (u8connector_id == MTK_DRM_CONNECTOR_TYPE_GRAPHIC) {
		//gfx
		Ret = drv_hwreg_render_pnl_set_tx_mute_en_gfx(tx_mute_info.bEnable);
		bReadBack = drv_hwreg_render_pnl_get_tx_mute_en_gfx();
		DRM_INFO("%s OSD:%d\n", __func__, bReadBack);
	}

	if ((hw_version != MOD_VER_2) ||
		(u8connector_id != MTK_DRM_CONNECTOR_TYPE_EXT_VIDEO)) {
		if (tx_mute_info.bEnable != bReadBack) {
			DRM_ERROR("[%s, %d]: TX mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}
	}

	#undef PNL_V1
	#undef PNL_V2
	#undef PNL_OSD

	return Ret;
}

int mtk_render_set_pixel_mute_video(drm_st_pixel_mute_info pixel_mute_info)
{
	int Ret = -EINVAL;
	uint16_t u16ReadBack = 0;

	Ret = drv_hwreg_render_pnl_set_pixel_mute_en_color_video(
		pixel_mute_info.bEnable,
		pixel_mute_info.u32Red,
		pixel_mute_info.u32Green,
		pixel_mute_info.u32Blue);

	if (pixel_mute_info.bEnable == 0) {

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_red_video();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_green_video();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_blue_video();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}
	} else {

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_en_video();
		if (u16ReadBack != pixel_mute_info.bEnable) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_red_video();
		if (u16ReadBack != pixel_mute_info.u32Red) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_green_video();
		if (u16ReadBack != pixel_mute_info.u32Green) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_blue_video();
		if (u16ReadBack != pixel_mute_info.u32Blue) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}
	}
	return Ret;
}

int mtk_render_set_pixel_mute_deltav(drm_st_pixel_mute_info pixel_mute_info)
{
	int Ret = -EINVAL;
	uint16_t u16ReadBack = 0;

	Ret = drv_hwreg_render_pnl_set_pixel_mute_en_color_deltav(
		pixel_mute_info.bEnable,
		pixel_mute_info.u32Red,
		pixel_mute_info.u32Green,
		pixel_mute_info.u32Blue);

	if (pixel_mute_info.bEnable == 0) {

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_red_deltav();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_green_deltav();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_blue_deltav();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}
	} else {

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_en_deltav();
		if (u16ReadBack != pixel_mute_info.bEnable) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_red_deltav();
		if (u16ReadBack != pixel_mute_info.u32Red) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_green_deltav();
		if (u16ReadBack != pixel_mute_info.u32Green) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_blue_deltav();
		if (u16ReadBack != pixel_mute_info.u32Blue) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

	}
	return Ret;
}

int mtk_render_set_pixel_mute_gfx(drm_st_pixel_mute_info pixel_mute_info)
{
	int Ret = -EINVAL;
	uint16_t u16ReadBack = 0;

	Ret = drv_hwreg_render_pnl_set_pixel_mute_en_color_gfx(
		pixel_mute_info.bEnable,
		pixel_mute_info.u32Red,
		pixel_mute_info.u32Green,
		pixel_mute_info.u32Blue);

	if (pixel_mute_info.bEnable == 0) {

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_red_gfx();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_green_gfx();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_blue_gfx();
		if (u16ReadBack != 0) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}
	} else {

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_en_gfx();
		if (u16ReadBack != pixel_mute_info.bEnable) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_red_gfx();
		if (u16ReadBack != pixel_mute_info.u32Red) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_green_gfx();
		if (u16ReadBack != pixel_mute_info.u32Green) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

		u16ReadBack = drv_hwreg_render_pnl_get_pixel_mute_blue_gfx();
		if (u16ReadBack != pixel_mute_info.u32Blue) {
			DRM_ERROR("[%s, %d]: Pixel mute is incorrect.\n",
				__func__, __LINE__);
			Ret = -EINVAL;
		}

	}
	return Ret;
}

int mtk_render_set_backlight_mute(struct mtk_panel_context *pctx,
	drm_st_backlight_mute_info backlight_mute_info)
{
	int Ret = -EINVAL;
	bool bReadBack = 0;

	if (backlight_mute_info.bEnable == true)
		gpiod_set_value(pctx->gpio_backlight, false);
	else
		gpiod_set_value(pctx->gpio_backlight, true);

	if (gpiod_get_value(pctx->gpio_backlight) == (!backlight_mute_info.bEnable)) {
		DRM_INFO("[%s][%d]BL mute pass\n", __func__, __LINE__);
		Ret = 0;
	}
	return Ret;
}

bool mtk_render_cfg_connector(struct mtk_panel_context *pctx)
{
	uint8_t out_clk_if = 0;
	uint32_t hw_version = 0;
	st_drv_out_lane_order stPnlOutLaneOrder;

	if (pctx == NULL) {
		DRM_INFO("[%s][%d]param is NULL, FAIL\n", __func__, __LINE__);
		return false;
	}

	hw_version = pctx->hw_info.mod_hw_version;
	memset(&stPnlOutLaneOrder, 0, sizeof(st_drv_out_lane_order));
	memcpy(&stPnlOutLaneOrder, &pctx->out_lane_info, sizeof(st_drv_out_lane_order));

	_init_mod_cfg(pctx);
	set_out_mod_cfg(&pctx->info, &pctx->v_cfg);
	_dump_mod_cfg(pctx);

	//for haps only case.
#ifdef CONFIG_MSTAR_ARM_BD_FPGA
	pctx->v_cfg.linkIF = E_LINK_LVDS;
	pctx->v_cfg.timing = E_FHD_60HZ;
	pctx->v_cfg.format = E_OUTPUT_RGB;
	pctx->v_cfg.lanes = 2;
#endif

	if (pctx->out_upd_type == E_MODE_RESET_TYPE) {
		drv_hwreg_render_pnl_analog_setting(
			hw_version,
			(en_drv_pnl_link_if)pctx->v_cfg.linkIF,
			(en_drv_pnl_output_mode)pctx->v_cfg.timing,
			&stPnlOutLaneOrder,
			(en_drv_pnl_output_format)pctx->v_cfg.format);

		_mtk_load_mpll_tbl(pctx);//load mpll table after enable power regulator

		drv_hwreg_render_pnl_set_vby1_pll_powerdown(false);

		_mtk_cfg_ckg_common(pctx);

		_mtk_load_lpll_tbl(pctx);

		_mtk_load_moda_tbl(pctx);
		_mtk_moda_path_setting(pctx);
	}

	//determine output timing of each link (V / DeltaV /OSD)
	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		if (pctx->v_cfg.timing == E_8K4K_60HZ) {
			if ((pctx->v_cfg.format == E_OUTPUT_YUV444)
				|| (pctx->v_cfg.format == E_OUTPUT_RGB)) {
				out_clk_if = 32;	// 8K60 YUV444, 32p
			}
			else
				out_clk_if = 16;	//8K60 yuv422, 16p
		} else if (pctx->v_cfg.timing == E_4K2K_144HZ) {
			out_clk_if = LANE_NUM_16;
		} else if (pctx->v_cfg.timing == E_4K2K_120HZ) {
			out_clk_if = 16;
		} else if (pctx->v_cfg.timing == E_4K2K_60HZ) {
			out_clk_if = 8;
		} else if (pctx->v_cfg.timing == E_FHD_120HZ) {
			out_clk_if = 4;
		} else if (pctx->v_cfg.timing == E_FHD_60HZ) {
			out_clk_if = 2;	//LVDS 2p mode.
		} else
			out_clk_if = 0;

		//_mtk_cfg_ckg_video(pctx->v_cfg, MOD_IN_IF_VIDEO, out_clk_if);

		if (pctx->out_upd_type == E_MODE_RESET_TYPE) {
			drm_en_vbo_bytemode vbo_bytemode = _vb1_set_byte_mode_decision(pctx);
			drv_hwreg_render_pnl_set_cfg_ckg_video(
						hw_version,
						(en_drv_pnl_vbo_bytemode)vbo_bytemode,
						MOD_IN_IF_VIDEO,
						out_clk_if,
						(en_drv_pnl_link_if)pctx->v_cfg.linkIF,
						(en_drv_pnl_output_mode)pctx->v_cfg.timing,
						(en_drv_pnl_output_format)pctx->v_cfg.format);
		}
	}

	if (pctx->out_upd_type == E_MODE_RESET_TYPE) {
		uint32_t mod_version = pctx->hw_info.mod_hw_version;
		uint16_t biasConVal = 0;
		uint16_t biasConOffsetVal = 0;
		_mtk_odclk_setting(pctx);
		_mtk_vby1_setting(pctx);
		_mtk_vby1_lane_order(pctx);
		drv_hwreg_render_pnl_set_vby1_fifo_reset();

		drv_hwreg_render_pnl_disp_odclk_init();
		drv_hwreg_render_pnl_set_swing_rcon((en_drv_pnl_link_if)pctx->v_cfg.linkIF);
		drv_hwreg_render_pnl_set_swing_biascon(
			(en_drv_pnl_link_if)pctx->v_cfg.linkIF,
			mod_version,
			biasConVal,
			biasConOffsetVal);

		drv_hwreg_render_pnl_set_pre_emphasis_enable();
		mtk_render_set_output_hmirror_enable(pctx);

	}

	if (pctx->out_upd_type != E_NO_UPDATE_TYPE) {
		_mtk_mft_hbkproch_protect_init(pctx);
		_mtk_tgen_init(pctx, out_clk_if);
	}

	if (pctx->out_upd_type == E_MODE_RESET_TYPE)
		mtk_render_output_en(pctx, true);

       //set default vfreq
	if (pctx->info.typ_htt != 0 && pctx->info.typ_vtt != 0) {
		pctx->outputTiming_info.u32OutputVfreq =
			(pctx->info.typ_dclk / pctx->info.typ_htt / pctx->info.typ_vtt)
			*VFRQRATIO_1000;//ex:60000 = 60.000hz
	}

	pctx->u8controlbit_gfid = 0;//initial control bit gfid

	return true;
}
EXPORT_SYMBOL(mtk_render_cfg_connector);

bool mtk_render_cfg_crtc(struct mtk_panel_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return false;
	}

	//TODO:Refine
	return true;
}

static void dump_panel_dts_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("================Video Path================\n");
	dump_panel_info(&pctx_pnl->info);
	DRM_INFO("================================================\n");
}

static void dump_dts_info(struct mtk_panel_context *pctx_pnl)
{
	int FRCType = 0;

	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	dump_panel_dts_info(pctx_pnl);

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
	DRM_INFO("u32PeriodPWM = %d\n", pctx_pnl->cus.period_pwm);
	DRM_INFO("u32DutyPWM = %d\n", pctx_pnl->cus.duty_pwm);
	DRM_INFO("u16DivPWM = %d\n", pctx_pnl->cus.div_pwm);
	DRM_INFO("bPolPWM = %d\n", pctx_pnl->cus.pol_pwm);
	DRM_INFO("u16MaxPWMvalue = %d\n", pctx_pnl->cus.max_pwm_val);
	DRM_INFO("u16MinPWMvalue = %d\n", pctx_pnl->cus.min_pwm_val);
	DRM_INFO("u16PWMPort = %d\n", pctx_pnl->cus.pwmport);

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

	DRM_INFO("Color_Format = %d\n", pctx_pnl->color_info.format);
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

static int _parse_cus_setting_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	struct device_node *np;
	const char *tcon_pth;
	const char *pgamma_pth;
	int ret = 0;

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
		pctx_pnl->cus.dtsi_file_type = get_dts_u32(np, DTSI_FILE_TYPE);
		pctx_pnl->cus.dtsi_file_ver = get_dts_u32(np, DTSI_FILE_VER);
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
		pctx_pnl->cus.period_pwm = get_dts_u32(np, PERIOD_PWM_TAG);
		pctx_pnl->cus.duty_pwm = get_dts_u32(np, DUTY_PWM_TAG);
		pctx_pnl->cus.div_pwm = get_dts_u32(np, DIV_PWM_TAG);
		pctx_pnl->cus.pol_pwm = get_dts_u32(np, POL_PWM_TAG);
		pctx_pnl->cus.max_pwm_val = get_dts_u32(np, MAX_PWM_VALUE_TAG);
		pctx_pnl->cus.min_pwm_val = get_dts_u32(np, MIN_PWM_VALUE_TAG);
		pctx_pnl->cus.pwmport = get_dts_u32(np, PWM_PORT_TAG);

		pctx_pnl->cus.hmirror_en = (pctx_pnl->cus.mirror_mode == E_PNL_MIRROR_H) ||
			(pctx_pnl->cus.mirror_mode == E_PNL_MIRROR_V_H);
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_CUS_SETTING_NODE);
	}

	return ret;
}

static int _parse_swing_level_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	int ret = 0;
	struct device_node *np;
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
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_OUTPUT_SWING_NODE);
	}

	return ret;
}

static int _parse_pe_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	int ret = 0;
	struct device_node *np;
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
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_OUTPUT_PE_NODE);
	}

	return ret;
}

static int _parse_lane_order_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	int ret = 0;
	struct device_node *np;
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
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_LANE_ORDER_NODE);
	}

	return ret;
}

static int _parse_output_lane_order_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	int ret = 0;
	int index;
	struct device_node *np;

	np = of_find_node_by_name(NULL, PNL_PKG_LANE_ORDER_NODE);
	if (np != NULL) {
		pctx_pnl->out_lane_info.sup_lanes = get_dts_u32(np, SUPPORT_LANES);

		for (index = 0; index < pctx_pnl->out_lane_info.sup_lanes; index++) {
			pctx_pnl->out_lane_info.def_layout[index] =
				get_dts_u32_index(np, LAYOUT_ORDER,	index);
			}
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_PKG_LANE_ORDER_NODE);
		ret = -ENODEV;
	}

	np = of_find_node_by_name(NULL, PNL_USR_LANE_ORDER_NODE);
	if (np != NULL) {
		pctx_pnl->out_lane_info.usr_defined = get_dts_u32(np, USR_DEFINE);
		pctx_pnl->out_lane_info.ctrl_lanes = get_dts_u32(np, CTRL_LANES);
		for (index = 0; index < pctx_pnl->out_lane_info.ctrl_lanes; index++) {
			pctx_pnl->out_lane_info.lane_order[index] =
				get_dts_u32_index(np, LANEORDER, index);
		}
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_USR_LANE_ORDER_NODE);
		ret = -ENODEV;
	}
	return ret;
}

static int _parse_op_cfg_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	int ret = 0;
	struct device_node *np;
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

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_OUTPUT_CONFIG_NODE);
	}

	return ret;
}

static int _parse_panel_color_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	int ret = 0;
	struct device_node *np;
	np = of_find_node_by_name(NULL, PNL_COLOR_INFO_NODE);
	if (np != NULL) {

		pctx_pnl->color_info.format = get_dts_u32(np, COLOR_FORMAT_TAG);
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

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_COLOR_INFO_NODE);
	}

	return ret;
}

static int _parse_frc_tbl_info(struct mtk_panel_context *pctx_pnl)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	struct device_node *np;
	int ret = 0, FRClen = 0, FRCType = 0;

	//Get FRC table info
	np = of_find_node_by_name(NULL, PNL_FRC_TABLE_NODE);
	if (np != NULL) {
		DRM_INFO("[%s, %d]: find node success, name = %s\n",
			__func__, __LINE__, PNL_FRC_TABLE_NODE);

		//ret = of_get_property(np, FRC_TABLE_TAG, &FRClen);
		if (of_get_property(np, FRC_TABLE_TAG, &FRClen) == NULL) {
			DRM_INFO("[%s, %d]: read string failed, name = %s\n",
				__func__, __LINE__, FRC_TABLE_TAG);
			return -1;
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
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_FRC_TABLE_NODE);
		ret = -ENODATA;
	}

	return ret;

}

static int _parse_hw_info(struct mtk_panel_context *pctx_pnl)
{
	int ret = 0;
	struct device_node *np;

	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	memset(&pctx_pnl->hw_info, 0, sizeof(drm_st_hw_info));
	np = of_find_node_by_name(NULL, PNL_HW_INFO_NODE);
	if (np != NULL) {
		pctx_pnl->hw_info.tgen_hw_version = get_dts_u32(np, TGEN_VERSION);
		pctx_pnl->hw_info.lpll_hw_version = get_dts_u32(np, LPLL_VERSION);
		pctx_pnl->hw_info.mod_hw_version = get_dts_u32(np, MOD_VERSION);
		pctx_pnl->hw_info.rcon_enable = get_dts_u32(np, RCON_ENABLE);
		pctx_pnl->hw_info.rcon_max = get_dts_u32(np, RCON_MAX);
		pctx_pnl->hw_info.rcon_min = get_dts_u32(np, RCON_MIN);
		pctx_pnl->hw_info.rcon_value = get_dts_u32(np, RCON_VALUE);

		pctx_pnl->hw_info.biascon_single_max =
			get_dts_u32(np, BIASCON_SINGLE_MAX);

		pctx_pnl->hw_info.biascon_single_min =
			get_dts_u32(np, BIASCON_SINGLE_MIN);

		pctx_pnl->hw_info.biascon_single_value =
			get_dts_u32(np, BIASCON_SINGLE_VALUE);

		pctx_pnl->hw_info.biascon_double_max =
			get_dts_u32(np, BIASCON_BIASCON_DOUBLE_MAX);

		pctx_pnl->hw_info.biascon_double_min =
			get_dts_u32(np, BIASCON_BIASCON_DOUBLE_MIN);

		pctx_pnl->hw_info.biascon_double_value =
			get_dts_u32(np, BIASCON_BIASCON_DOUBLE_VALUE);

		pctx_pnl->hw_info.rint_enable = get_dts_u32(np, RINT_ENABLE);
		pctx_pnl->hw_info.rint_max = get_dts_u32(np, RINT_MAX);
		pctx_pnl->hw_info.rint_min = get_dts_u32(np, RINT_MIN);
		pctx_pnl->hw_info.rint_value = get_dts_u32(np, RINT_VALUE);
	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_HW_INFO_NODE);
	}

	return ret;
}

static int _parse_ext_graph_combo_info(struct mtk_panel_context *pctx_pnl)
{
	int ret = 0;
	struct device_node *np;

	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	memset(&pctx_pnl->ext_grpah_combo_info, 0, sizeof(drm_st_ext_graph_combo_info));
	np = of_find_node_by_name(NULL, PNL_COMBO_INFO_NODE);
	if (np != NULL) {

		pctx_pnl->ext_grpah_combo_info.graph_vbo_byte_mode =
			get_dts_u32(np, GRAPH_VBO_BYTE_MODE);

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
					__func__, __LINE__, PNL_COMBO_INFO_NODE);
	}

	return ret;
}

int readDTB2PanelPrivate(struct mtk_panel_context *pctx_pnl)
{
	int ret = 0;

	if (pctx_pnl == NULL)
		return -ENODEV;

	//Get panel info
	ret |= _parse_panel_info(pctx_pnl->dev->of_node,
		&pctx_pnl->info,
		&pctx_pnl->panelInfoList);

	//Get Board ini related info
	ret |= _parse_cus_setting_info(pctx_pnl);

	//Get MMAP related info
	ret |= _parse_swing_level_info(pctx_pnl);

	ret |= _parse_pe_info(pctx_pnl);

	ret |= _parse_lane_order_info(pctx_pnl);

	ret |= _parse_op_cfg_info(pctx_pnl);

	ret |= _parse_panel_color_info(pctx_pnl);

	ret |= _parse_frc_tbl_info(pctx_pnl);

	ret |= _parse_output_lane_order_info(pctx_pnl);

	ret |= _parse_hw_info(pctx_pnl);

	ret |= _parse_ext_graph_combo_info(pctx_pnl);

	dump_dts_info(pctx_pnl);

	//Patch for GOP sets TGEN of GFX
	{
		struct mtk_drm_panel_context *pStPanelCtx = NULL;

		pStPanelCtx = _get_gfx_panel_ctx();
		if (pStPanelCtx != NULL) {
			memcpy(&pStPanelCtx->out_lane_info, &pctx_pnl->out_lane_info,
					sizeof(drm_st_out_lane_order));
			memcpy(&pctx_pnl->gfx_info, &pStPanelCtx->pnlInfo, sizeof(drm_st_pnl_info));
		}
	}

	//Patch for _mtk_render_check_vbo_ctrlbit_feature sets TGEN of EXTDEV
	{
		struct mtk_drm_panel_context *pStPanelCtx = NULL;

		pStPanelCtx = _get_extdev_panel_ctx();
		if (pStPanelCtx != NULL) {
			memcpy(&pStPanelCtx->out_lane_info, &pctx_pnl->out_lane_info,
					sizeof(drm_st_out_lane_order));
			memcpy(&pctx_pnl->extdev_info, &pStPanelCtx->pnlInfo,
					sizeof(drm_st_pnl_info));
		}
	}

	return ret;
}
EXPORT_SYMBOL(readDTB2PanelPrivate);

static void _mtk_render_map_timing(
	drm_output_mode timing,
	struct pxm_size *size)
{
	if (size == NULL) {
		DRM_ERROR("[drm][pxm] size is NULL!\n");
		return;
	}

	switch (timing) {
	case E_8K4K_120HZ:
	case E_8K4K_60HZ:
	case E_8K4K_30HZ:
		size->h_size = 7680;
		size->v_size = 4320;
		break;
	case E_4K2K_144HZ:
	case E_4K2K_120HZ:
	case E_4K2K_60HZ:
	case E_4K2K_30HZ:
		size->h_size = 3840;
		size->v_size = 2160;
		break;
	case E_FHD_120HZ:
	case E_FHD_60HZ:
		size->h_size = 1920;
		size->v_size = 1080;
		break;
	default:
		size->h_size = 0;
		size->v_size = 0;
		DRM_INFO("[drm][pxm] timing(%d) is invalid!\n", timing);
		break;
	}
}

static void _mtk_render_set_pxm_size(struct mtk_panel_context *pctx)
{
	enum pxm_return ret = E_PXM_RET_FAIL;
	struct pxm_size_info info;
	struct mtk_drm_panel_context *pStPanelCtx = NULL;
	drm_output_mode extdevOutputMode = 0;

	memset(&info, 0, sizeof(struct pxm_size_info));

	if (pctx == NULL) {
		DRM_ERROR("[drm][pxm] pctx is NULL!\n");
		return;
	}

	info.win_id = 0;  // tcon not support time sharing, win id set 0

	info.point = E_PXM_POINT_TCON_AFTER_OSD;
	_mtk_render_map_timing(pctx->v_cfg.timing, &info.size);
	ret = pxm_set_size(&info);
	if (ret != E_PXM_RET_OK) {
		DRM_ERROR("[drm][pxm] set pxm size fail(%d)!\n", ret);
		return;
	}

	info.point = E_PXM_POINT_TCON_BEFORE_MOD;
	_mtk_render_map_timing(pctx->v_cfg.timing, &info.size);
	ret = pxm_set_size(&info);
	if (ret != E_PXM_RET_OK) {
		DRM_ERROR("[drm][pxm] set pxm size fail(%d)!\n", ret);
		return;
	}

	pStPanelCtx = _get_extdev_panel_ctx();
	if (pStPanelCtx != NULL)
		extdevOutputMode = pStPanelCtx->cfg.timing;

	info.point = E_PXM_POINT_TCON_DELTA_PATH;
	_mtk_render_map_timing(extdevOutputMode, &info.size);
	ret = pxm_set_size(&info);
	if (ret != E_PXM_RET_OK)
		DRM_ERROR("[drm][pxm] set pxm size fail(%d)!\n", ret);

}

static int mtk_panel_init(struct device *dev)
{
	struct mtk_panel_context *pctx_pnl = NULL;

	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	pctx_pnl = dev->driver_data;

	u64PreTypDclk = pctx_pnl->info.typ_dclk;
	ePreOutputFormat = pctx_pnl->info.op_format;
	ePreByteMode = pctx_pnl->info.vbo_byte;

	//External BE related
	if (mtk_render_cfg_connector(pctx_pnl) == false) {
		DRM_INFO("[%s][%d] mtk_plane_init failed!!\n", __func__, __LINE__);
		return -ENODEV;
	}

	//2. Config CRTC setting
	//update gamma lut size
	if (mtk_render_cfg_crtc(pctx_pnl) == false) {
		DRM_INFO("[%s][%d] mtk_plane_init failed!!\n", __func__, __LINE__);
		return -ENODEV;
	}

	//update gamma lut size
	//drm_mode_crtc_set_gamma_size(&mtkCrtc->base, MTK_GAMMA_LUT_SIZE);
	//set pixel monitor size
	_mtk_render_set_pxm_size(pctx_pnl);

	DRM_INFO("[%s][%d] success!!\n", __func__, __LINE__);
	return 0;

}

static int _mtk_video_panel_add_framesyncML(
	struct mtk_tv_kms_context *pctx,
	struct reg_info *reg,
	uint32_t RegCount)
{
	if (!pctx || !reg) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx, reg=0x%llx\n",
			__func__, __LINE__, pctx, reg);
		return -ENODEV;
	}

	struct sm_ml_add_info cmd_info;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	int ret = 0;

	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	switch (pctx_pnl->outputTiming_info.eFrameSyncState) {
	case E_PNL_FRAMESYNC_STATE_PROP_IN:
		//3.add cmd
		cmd_info.cmd_cnt = RegCount;
		cmd_info.mem_index = pctx_video->disp_ml.memindex;
		cmd_info.reg = (struct sm_reg *)reg;
		sm_ml_add_cmd(pctx_video->disp_ml.fd, &cmd_info);
		pctx_video->disp_ml.regcount = pctx_video->disp_ml.regcount + RegCount;
		break;
	case E_PNL_FRAMESYNC_STATE_IRQ_IN:
		//3.add cmd
		cmd_info.cmd_cnt = RegCount;
		cmd_info.mem_index = pctx_video->disp_ml.irq_memindex;
		cmd_info.reg = (struct sm_reg *)reg;
		sm_ml_add_cmd(pctx_video->disp_ml.irq_fd, &cmd_info);
		pctx_video->disp_ml.irq_regcount = pctx_video->disp_ml.irq_regcount + RegCount;
		break;
	default:
		DRM_ERROR("[%s, %d] framesync_state = %d\n",
			__func__, __LINE__, pctx_pnl->outputTiming_info.eFrameSyncState);
		ret = -1;
		break;
	}

	return ret;
}

static int _mtk_video_panel_set_input_source_select(
	struct mtk_tv_kms_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t plane_id = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
	struct drm_mode_object *obj = drm_mode_object_find(pctx->drm_dev,
		NULL, plane_id, DRM_MODE_OBJECT_ANY);
	struct drm_plane *plane = obj_to_plane(obj);
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx_plane =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx_plane->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct mtk_video_plane_ctx *plane_ctx =
		(pctx_video->plane_ctx + plane_inx);
	int ret = 0;
	struct meta_pq_framesync_info framesyncInfo;
	struct meta_pq_display_idr_ctrl IdrCtrlInfo;
	enum meta_pq_input_source input_source = META_PQ_INPUTSRC_NUM;
	enum meta_pq_idr_input_path idr_path = META_PQ_PATH_IPDMA_MAX;
	enum drm_hwreg_triggen_src_sel src = RENDER_COMMON_TRIGEN_INPUT_MAX;

	bool bRIU = false;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	struct hwregTrigGenInputSrcSelIn paramIn;

	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;

	memset(&framesyncInfo, 0, sizeof(struct meta_pq_framesync_info));
	memset(&IdrCtrlInfo, 0, sizeof(struct meta_pq_display_idr_ctrl));

	// get framesync info
	ret = mtk_render_common_read_metadata(plane_ctx->meta_db,
		RENDER_METATAG_FRAMESYNC_INFO, (void *)&framesyncInfo);
	if (ret) {
		DRM_INFO("[%s][%d] metadata do not has framesync info tag\n",
			__func__, __LINE__);
	} else {
		DRM_INFO("[%s][%d] meta framesync info: input_source: %d\n",
			__func__, __LINE__, framesyncInfo.input_source);
		input_source = framesyncInfo.input_source;
	}

	// get idr crtl info
	ret = mtk_render_common_read_metadata(plane_ctx->meta_db,
		RENDER_METATAG_IDR_CTRL_INFO, (void *)&IdrCtrlInfo);
	if (ret) {
		DRM_INFO("[%s][%d] metadata do not has idr frame info tag\n",
			__func__, __LINE__);
	} else {
		DRM_INFO("[%s][%d] meta idr frame info: m_memout_path: %d\n",
			__func__, __LINE__, IdrCtrlInfo.path);
		idr_path = IdrCtrlInfo.path;
	}

	//get input source type
	DRM_INFO("[%s][%d] input_source: %d, idr_path: %d\n",
		__func__, __LINE__, input_source, idr_path);

	//decide input source select
	if (IS_INPUT_B2R(input_source)) {
		src = RENDER_COMMON_TRIGEN_INPUT_B2R0;
	} else if (IS_INPUT_SRCCAP(input_source)) {
		switch (idr_path) {
		case META_PQ_PATH_IPDMA_0:
			src = RENDER_COMMON_TRIGEN_INPUT_IP0;
			break;
		case META_PQ_PATH_IPDMA_1:
			src = RENDER_COMMON_TRIGEN_INPUT_IP1;
			break;
		default:
			DRM_ERROR("[%s][%d] invalid source!! input_source:%d, idr_path:%d\n",
				__func__, __LINE__, input_source, idr_path);
		}
	}

	//save or use inuput_src
	if (pctx->b2r_src_stubmode) {
		pctx_pnl->outputTiming_info.input_src = RENDER_COMMON_TRIGEN_INPUT_B2R0;
		src = RENDER_COMMON_TRIGEN_INPUT_B2R0;
	} else {
		if (src == RENDER_COMMON_TRIGEN_INPUT_MAX)
			src = pctx_pnl->outputTiming_info.input_src;
		else
			pctx_pnl->outputTiming_info.input_src = src;
	}

	memset(&reg, 0, sizeof(reg));
	memset(&paramIn, 0, sizeof(struct hwregTrigGenInputSrcSelIn));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramIn.RIU = 0;
	paramIn.domain = RENDER_COMMON_TRIGEN_DOMAIN_TGEN;
	paramIn.src = src;

	paramOut.reg = reg;

	DRM_INFO("[%s][%d][paramIn] RIU:%d domain:%d, src:%d\n",
		__func__, __LINE__,
		paramIn.RIU, paramIn.domain, paramIn.src);

	//BKA3A6_h21[1:0] = src_select
	drv_hwreg_render_common_triggen_set_inputSrcSel(
		paramIn, &paramOut);

	if ((bRIU == false) && (paramOut.regCount != 0))
		_mtk_video_panel_add_framesyncML(pctx, reg, paramOut.regCount);

	return ret;
}

static int _mtk_video_get_vplane_buf_mode(
	struct mtk_tv_kms_context *pctx,
	enum drm_mtk_vplane_buf_mode *buf_mode)
{
	if (!pctx || !buf_mode) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx, buf_mode=0x%llx\n",
			__func__, __LINE__, pctx, buf_mode);
		return -ENODEV;
	}

	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t plane_id = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
	struct drm_mode_object *obj = drm_mode_object_find(pctx->drm_dev,
		NULL, plane_id, DRM_MODE_OBJECT_ANY);
	struct drm_plane *plane = obj_to_plane(obj);
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx_plane =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx_plane->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);

	*buf_mode = plane_props->prop_val[EXT_VPLANE_PROP_BUF_MODE];

	return 0;
}

static int _mtk_video_panel_set_vttv_phase_diff(
	struct mtk_tv_kms_context *pctx,
	uint64_t *phase_diff)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	int ret = 0;
	enum drm_mtk_vplane_buf_mode vplane_buf_mode = MTK_VPLANE_BUF_MODE_MAX;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t plane_id = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
	struct drm_mode_object *obj = drm_mode_object_find(pctx->drm_dev,
		NULL, plane_id, DRM_MODE_OBJECT_ANY);
	struct drm_plane *plane = obj_to_plane(obj);

	ret = _mtk_video_get_vplane_buf_mode(pctx, &vplane_buf_mode);
	if (ret) {
		DRM_ERROR("[%s, %d]: get vplane buf mode error\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	//set lock user phase diff
	if ((vplane_buf_mode == MTK_VPLANE_BUF_MODE_HW) &&
		((uint16_t)plane->state->crtc_w == pctx_pnl->info.de_width &&
		(uint16_t)plane->state->crtc_h == pctx_pnl->info.de_height)) {
		if ((pctx_pnl->outputTiming_info.u8FRC_in == 1 &&
			pctx_pnl->outputTiming_info.u8FRC_out == 1) &&
			(pctx_pnl->info.de_width == UHD_8K_W &&
			pctx_pnl->info.de_height == UHD_8K_H)) {
			*phase_diff = pctx_pnl->outputTiming_info.u16OutputVTotal*
				FRAMESYNC_VTTV_SHORTEST_LOW_LATENCY_PHASE_DIFF/
				FRAMESYNC_VTTV_PHASE_DIFF_DIV_1000;
		} else {
			*phase_diff = pctx_pnl->outputTiming_info.u16OutputVTotal*
				FRAMESYNC_VTTV_LOW_LATENCY_PHASE_DIFF/
				FRAMESYNC_VTTV_PHASE_DIFF_DIV_1000;
		}
	} else {
		*phase_diff = pctx_pnl->outputTiming_info.u16OutputVTotal*
			FRAMESYNC_VTTV_NORMAL_PHASE_DIFF/
			FRAMESYNC_VTTV_PHASE_DIFF_DIV_1000;
	}
	DRM_INFO("[%s][%d] phase_diff: %td\n",
			__func__, __LINE__, (ptrdiff_t)*phase_diff);

	return ret;
}

static uint64_t _mtk_video_adjust_input_freq_stubMode(
	struct mtk_tv_kms_context *pctx,
	struct mtk_panel_context *pctx_pnl,
	uint64_t input_vfreq)
{
	//in stub mode
	//change the value of input frame rate to max output frame rate
	//to meet the ivs:ovs = 1:1
	if (pctx->stub == true)
		if ((pctx_pnl->info.typ_htt*pctx_pnl->info.typ_vtt) != 0)
			input_vfreq = ((uint64_t)pctx_pnl->info.typ_dclk*VFRQRATIO_1000) /
				((uint32_t)(pctx_pnl->info.typ_htt*pctx_pnl->info.typ_vtt));
	return input_vfreq;
}

static uint16_t _mtk_video_adjust_output_vtt_ans_stubMode(
	struct mtk_tv_kms_context *pctx,
	struct mtk_panel_context *pctx_pnl,
	uint16_t output_vtt)
{
	if (pctx->stub == true) {
		if ((pctx_pnl->outputTiming_info.u16OutputVTotal != 0) &&
			(pctx_pnl->outputTiming_info.u16OutputVTotal == pctx_pnl->info.typ_vtt))
			output_vtt = 1;// right result
		else
			output_vtt = 0;// wrong result

		DRM_INFO("[%s][%d] stub result log : %td %td\n",
				__func__, __LINE__,
				(ptrdiff_t)pctx_pnl->outputTiming_info.u16OutputVTotal,
				(ptrdiff_t)pctx_pnl->info.typ_vtt);
	}
	return output_vtt;
}

static int _mtk_video_set_vttv_mode(
	struct mtk_tv_kms_context *pctx,
	uint64_t input_vfreq, bool bLowLatencyEn)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	int ret = 0, FRCType = 0;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	drm_en_vttv_source_type ivsSource = REF_PULSE, ovsSource = REF_PULSE;
	uint8_t ivs = 0, ovs = 0;
	uint64_t output_vfreq = 0, default_vfreq = 0, outputVTT = 0, phase_diff = 0;
	uint64_t frc_table_upperbound = 0, frc_table_lowerbound = 0;
	bool bRIU = false;
	uint16_t RegCount = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	enum drm_mtk_vplane_buf_mode vplane_buf_mode = MTK_VPLANE_BUF_MODE_MAX;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t plane_id = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
	struct drm_mode_object *obj = drm_mode_object_find(pctx->drm_dev,
		NULL, plane_id, DRM_MODE_OBJECT_ANY);
	struct drm_plane *plane = obj_to_plane(obj);

	DRM_INFO("[%s][%d]input_vfreq: %d, bLowLatencyEn: %d\n",
			__func__, __LINE__, input_vfreq, bLowLatencyEn);

	//decide ivs/ovs source
	_mtk_video_get_vplane_buf_mode(pctx, &vplane_buf_mode);
	switch (vplane_buf_mode) {
	case MTK_VPLANE_BUF_MODE_SW:
		ivsSource = REF_PULSE;
		ovsSource = REF_PULSE;
		break;
	case MTK_VPLANE_BUF_MODE_HW:
		ivsSource = RW_BK_UPD_OP2;
		ovsSource = RW_BK_UPD_DISP;
		break;
	default:
		DRM_ERROR("[%s][%d] invalid video buffer mode!!\n",
			__func__, __LINE__);
	}
	DRM_INFO("[%s][%d]vplane_buf_mode: %d, ivsSource: %d, ovsSource: %d\n",
			__func__, __LINE__, vplane_buf_mode, ivsSource, ovsSource);

	//find ivs/ovs
	ivs = pctx_pnl->frcRatio_info.u8FRC_in;
	ovs = pctx_pnl->frcRatio_info.u8FRC_out;

	DRM_INFO("[%s][%d]ivs: %d, ovs: %d\n",
			__func__, __LINE__, ivs, ovs);

	if (ivs == 0) {
		for (FRCType = 0;
			FRCType < (sizeof(pctx_pnl->frc_info)/
				sizeof(struct drm_st_pnl_frc_table_info));
			FRCType++) {
			// VFreq in FRC table is x10 base.
			frc_table_upperbound = (uint64_t)VFRQRATIO_1000/VFRQRATIO_10
				*pctx_pnl->frc_info[FRCType].u16HigherBound;
			frc_table_lowerbound = (uint64_t)VFRQRATIO_1000/VFRQRATIO_10
				*pctx_pnl->frc_info[FRCType].u16LowerBound;
			if (input_vfreq > frc_table_lowerbound &&
				input_vfreq <= frc_table_upperbound) {
				ivs = pctx_pnl->frc_info[FRCType].u8FRC_in;
				ovs = pctx_pnl->frc_info[FRCType].u8FRC_out;
				break;
			}
		}
	}

	//calc output vfreq
	if (ivs > 0) {
		output_vfreq = input_vfreq * ovs / ivs;
		pctx_pnl->outputTiming_info.u32OutputVfreq = output_vfreq;
		pctx_pnl->outputTiming_info.u8FRC_in = ivs;
		pctx_pnl->outputTiming_info.u8FRC_out = ovs;

		DRM_INFO("[%s][%d] ivs: %td, ovs: %td, output_vfreq: %td\n",
			__func__, __LINE__, (ptrdiff_t)ivs, (ptrdiff_t)ovs,
			(ptrdiff_t)output_vfreq);
	} else {
		DRM_INFO("[%s][%d]calc output vfreq fail, ivs: %td, ovs: %td\n",
			__func__, __LINE__, (ptrdiff_t)ivs, (ptrdiff_t)ovs);

		DRM_INFO("[%s][%d]calc output vfreq fail, output_vfreq: %td\n",
			__func__, __LINE__, (ptrdiff_t)output_vfreq);
	}

	//calc default vfreq
	if (pctx_pnl->info.typ_htt*pctx_pnl->info.typ_vtt > 0) {
		default_vfreq =
			((uint64_t)pctx_pnl->info.typ_dclk*VFRQRATIO_1000) /
			((uint32_t)(pctx_pnl->info.typ_htt*pctx_pnl->info.typ_vtt));

		DRM_INFO("[%s][%d]typ_htt:%td, typ_vtt:%td, typ_dclk:%td, default_vfreq:%td\n",
			__func__, __LINE__,
			(ptrdiff_t)pctx_pnl->info.typ_htt,
			(ptrdiff_t)pctx_pnl->info.typ_vtt,
			(ptrdiff_t)pctx_pnl->info.typ_dclk,
			(ptrdiff_t)default_vfreq);
	} else {
		DRM_INFO("[%s][%d]calc default vfreq fail,typ_htt:%td, typ_vtt:%td\n",
			__func__, __LINE__,
			(ptrdiff_t)pctx_pnl->info.typ_htt,
			(ptrdiff_t)pctx_pnl->info.typ_vtt);

		DRM_INFO("[%s][%d]calc default vfreq fail,typ_dclk:%td, default_vfreq: %td\n",
			__func__, __LINE__,
			(ptrdiff_t)pctx_pnl->info.typ_dclk,
			(ptrdiff_t)default_vfreq);
	}

	//calc output vtt
	if (output_vfreq > 0) {
		outputVTT = (pctx_pnl->info.typ_vtt*default_vfreq) / output_vfreq;
		DRM_INFO("[%s][%d] cal outputVTT: %d\n",
			__func__, __LINE__, outputVTT);
		if ((pctx_pnl->info.min_vtt < outputVTT)
			&& (pctx_pnl->info.max_vtt > outputVTT)) {
			DRM_INFO("[%s][%d] in tolerance range, outputVTT: %td\n",
				__func__, __LINE__, (ptrdiff_t)outputVTT);
		} else {
			pctx_pnl->outputTiming_info.u16OutputVTotal = pctx_pnl->info.typ_vtt;
			pctx_pnl->outputTiming_info.u32OutputVfreq = default_vfreq;
	DRM_INFO("[%s][%d] out of tolerance range free run with u16OutputVTotal: %td\n",
	__func__, __LINE__, (ptrdiff_t)pctx_pnl->outputTiming_info.u16OutputVTotal);

			return -EPERM;
		}
		pctx_pnl->outputTiming_info.u16OutputVTotal = outputVTT;
	} else {
		DRM_INFO("[%s][%d] calc output vtt fail, outputVTT: %td\n",
			__func__, __LINE__, (ptrdiff_t)outputVTT);
	}

	//adjust output vtt in stub mode
	outputVTT = _mtk_video_adjust_output_vtt_ans_stubMode(
			pctx, pctx_pnl, pctx_pnl->outputTiming_info.u16OutputVTotal);

	//set lock user phase diff
	ret = _mtk_video_panel_set_vttv_phase_diff(pctx, &phase_diff);
	if (ret) {
		DRM_ERROR("[%s, %d]: set vttv phase diff error\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	// we cannot use VTTV in B2R mode at E1
	if (pctx_pnl->hw_info.tgen_hw_version == 1) {
		if ((pctx_pnl->outputTiming_info.input_src == RENDER_COMMON_TRIGEN_INPUT_B2R0
			|| pctx_pnl->outputTiming_info.input_src == RENDER_COMMON_TRIGEN_INPUT_B2R1)
			&& (pctx->stub == false))
			return -EPERM;
	}

	pctx_pnl->outputTiming_info.eFrameSyncMode = VIDEO_CRTC_FRAME_SYNC_VTTV;

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;

	//h31[0] = 0
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A3_h16[4:0] = ivsSource
	//BKA3A3_h16[12:8] = ovsSource
	drv_hwreg_render_pnl_set_vttSourceSelect(
		&paramOut,
		bRIU,
		ivsSource,
		ovsSource);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h20[15] = 1
	//h20[14:8] = 1
	//h20[7:0] = 3
	drv_hwreg_render_pnl_set_centralVttChange(
		&paramOut,
		bRIU,
		true,
		FRAMESYNC_VTTV_FRAME_COUNT,
		FRAMESYNC_VTTV_THRESHOLD);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h22[15:3] = phase_diff
	//h23[15] = 1
	drv_hwreg_render_pnl_set_vttLockPhaseDiff(
		&paramOut,
		bRIU,
		1,
		phase_diff);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h24[4] = 1
	drv_hwreg_render_pnl_set_vttLockKeepSequenceEnable(
		&paramOut,
		bRIU,
		1,
		FRAMESYNC_VTTV_LOCKKEEPSEQUENCETH);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h33[10] = 1
	//h30[15] = 1
	//h30[14:0] = m_delta
	drv_hwreg_render_pnl_set_vttLockDiffLimit(
		&paramOut,
		bRIU,
		true,
		FRAMESYNC_VTTV_DIFF_LIMIT);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//VTT uper bound/lower bound is control by Tgen panel only
	//disable VTTV uper bound /lower bound
	//h33[11] = 0
	//h33[12] = 0
	//h36[15:0] = 0xffff
	//h37[15:0] = 0
	drv_hwreg_render_pnl_set_vttLockVttRange(
		&paramOut,
		bRIU,
		false,
		FRAMESYNC_VTTV_LOCK_VTT_UPPER,
		FRAMESYNC_VTTV_LOCK_VTT_LOWER);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h33[13] = 1
	drv_hwreg_render_pnl_set_vttLockVsMaskMode(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h32[7:0] = ivs
	//h32[15:8] = ovs
	drv_hwreg_render_pnl_set_vttLockIvsOvs(
		&paramOut,
		bRIU,
		ivs,
		ovs);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h3e[15:0] = 9
	drv_hwreg_render_pnl_set_vttLockLockedInLimit(
		&paramOut,
		bRIU,
		FRAMESYNC_VTTV_LOCKED_LIMIT);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[13] = 1
	drv_hwreg_render_pnl_set_vttLockUdSwModeEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[9] = 0
	drv_hwreg_render_pnl_set_vttLockIvsShiftEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[8] = 1
	drv_hwreg_render_pnl_set_vttLockProtectEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h23[15:0] = output VTT
		drv_hwreg_render_pnl_set_vtt(
			&paramOut,
			bRIU,
			outputVTT);

	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[0] = 1
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h24[12] = 0
	drv_hwreg_render_pnl_set_vttLockLastVttLatchEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	if ((bRIU == false) && (RegCount != 0))
		_mtk_video_panel_add_framesyncML(pctx, reg, RegCount);

	return ret;
}

int _mtk_video_set_low_latency_mode(
	struct mtk_tv_kms_context *pctx,
	uint64_t bLowLatencyEn)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	int ret = 0;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	drm_en_vttv_source_type ivsSource = REF_PULSE, ovsSource = REF_PULSE;
	uint64_t outputVTT = 0, phase_diff = 0;
	bool bRIU = false;
	uint16_t RegCount = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	enum drm_mtk_vplane_buf_mode vplane_buf_mode = MTK_VPLANE_BUF_MODE_MAX;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t plane_id = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
	struct drm_mode_object *obj = drm_mode_object_find(pctx->drm_dev,
		NULL, plane_id, DRM_MODE_OBJECT_ANY);
	struct drm_plane *plane = obj_to_plane(obj);

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));
	paramOut.reg = reg;

	//decide ivs/ovs source
	_mtk_video_get_vplane_buf_mode(pctx, &vplane_buf_mode);
	switch (vplane_buf_mode) {
	case MTK_VPLANE_BUF_MODE_SW:
		ivsSource = REF_PULSE;
		ovsSource = REF_PULSE;
		break;
	case MTK_VPLANE_BUF_MODE_HW:
		ivsSource = RW_BK_UPD_OP2;
		ovsSource = RW_BK_UPD_DISP;
		break;
	default:
		DRM_ERROR("[%s][%d] invalid video buffer mode!!\n",
			__func__, __LINE__);
	}
	DRM_INFO("[%s][%d]vplane_buf_mode: %d, ivsSource: %d, ovsSource: %d\n",
			__func__, __LINE__, vplane_buf_mode, ivsSource, ovsSource);

	//get output VTT
	outputVTT = pctx_pnl->outputTiming_info.u16OutputVTotal;

	//set lock user phase diff
	ret = _mtk_video_panel_set_vttv_phase_diff(pctx, &phase_diff);
	if (ret) {
		DRM_ERROR("[%s, %d]: set vttv phase diff error\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	//h31[0] = 0
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A3_h16[4:0] = ivsSource
	//BKA3A3_h16[12:8] = ovsSource
	drv_hwreg_render_pnl_set_vttSourceSelect(
		&paramOut,
		bRIU,
		ivsSource,
		ovsSource);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h22[15:3] = phase_diff
	drv_hwreg_render_pnl_set_vttLockPhaseDiff(
		&paramOut,
		bRIU,
		true,
		phase_diff);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[0] = 1
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h24[12] = 0
	drv_hwreg_render_pnl_set_vttLockLastVttLatchEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	if ((bRIU == false) && (RegCount != 0))
		_mtk_video_panel_add_framesyncML(pctx, reg, RegCount);

	return 0;
}

static int _mtk_video_set_vrr_mode(
	struct mtk_tv_kms_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	int ret = 0;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	bool bRIU = false;
	uint16_t RegCount = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;
	pctx_pnl->outputTiming_info.eFrameSyncMode = VIDEO_CRTC_FRAME_SYNC_FRAMELOCK;

	//enable rwdiff auto-trace
	//BKA38B_40[11] = 1
	drv_hwreg_render_pnl_set_rwDiffTraceEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA38B_44[4:0] = 1
	drv_hwreg_render_pnl_set_rwDiffTraceDepositCount(
		&paramOut,
		bRIU,
		FRAMESYNC_VRR_DEPOSIT_COUNT);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//Turn on Framelock 1:1
	//BKA3A2_01[1] = 1
	drv_hwreg_render_pnl_set_forceInsertBypass(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_01[0] = 1
	drv_hwreg_render_pnl_set_InsertRecord(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_11[0] = 1
	drv_hwreg_render_pnl_set_fakeInsertPEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_10[3] = 1
	drv_hwreg_render_pnl_set_stateSwModeEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_10[6:4] = 4
	drv_hwreg_render_pnl_set_stateSwMode(
		&paramOut,
		bRIU,
		FRAMESYNC_VRR_STATE_SW_MODE);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_1e[0] = 1
	//BKA3A2_1e[1] = 1
	//BKA3A2_1e[2] = 1
	//BKA3A2_18[15:0] = 0
	//BKA3A2_19[15:0] = pctx_pnl->info.vrr_min_v
	//BKA3A2_1a[15:0] = pctx_pnl->info.vrr_max_v
	drv_hwreg_render_pnl_set_FrameLockVttRange(
		&paramOut,
		bRIU,
		true,
		0,
		pctx_pnl->info.vrr_min_v,
		pctx_pnl->info.vrr_max_v);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_23[15:0] = 2
	drv_hwreg_render_pnl_set_chaseTotalDiffRange(
		&paramOut,
		bRIU,
		FRAMESYNC_VRR_CHASE_TOTAL_DIFF_RANGE);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_31[15:0] = 2
	drv_hwreg_render_pnl_set_chasePhaseTarget(
		&paramOut,
		bRIU,
		FRAMESYNC_VRR_CHASE_PHASE_TARGET);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_32[15:0] = 2
	drv_hwreg_render_pnl_set_chasePhaseAlmost(
		&paramOut,
		bRIU,
		FRAMESYNC_VRR_CHASE_PHASE_ALMOST);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_10[15] = 1
	drv_hwreg_render_pnl_set_statusReset(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A1_24[13] = 0
	drv_hwreg_render_pnl_set_vttLockLastVttLatchToggle(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A0_22[0] = 1
	drv_hwreg_render_pnl_set_framelockVcntEnableDb(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A2_10[15] = 0
	drv_hwreg_render_pnl_set_statusReset(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	if ((bRIU == false) && (RegCount != 0))
		_mtk_video_panel_add_framesyncML(pctx, reg, RegCount);

	return 0;
}

static int _mtk_video_set_freerun_vfreq(
	struct mtk_tv_kms_context *pctx,
	uint64_t input_vfreq, uint64_t output_vfreq)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}
	int ret = 0, FRCType = 0;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	drm_en_vttv_source_type ivsSource = REF_PULSE, ovsSource = REF_PULSE;
	uint8_t ivs = 0, ovs = 0;
	uint64_t default_vfreq = 0, outputVTT = 0, phase_diff = 0;
	bool bRIU = false;
	uint16_t RegCount = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	enum drm_mtk_vplane_buf_mode vplane_buf_mode = MTK_VPLANE_BUF_MODE_MAX;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t plane_id = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
	struct drm_mode_object *obj = drm_mode_object_find(pctx->drm_dev,
		NULL, plane_id, DRM_MODE_OBJECT_ANY);
	struct drm_plane *plane = obj_to_plane(obj);

	//decide ivs/ovs source
	ivsSource = REF_PULSE;
	ovsSource = REF_PULSE;
	DRM_INFO("[%s][%d] ivsSource: %d, ovsSource: %d\n",
			__func__, __LINE__, ivsSource, ovsSource);

	DRM_INFO("[%s][%d] input_vfreq: %d, output_vfreq: %d\n",
			__func__, __LINE__, input_vfreq, output_vfreq);

	//set output vfreq
	if (output_vfreq != 0)
		pctx_pnl->outputTiming_info.u32OutputVfreq = output_vfreq;

	DRM_INFO("[%s][%d] ivs: %td, ovs: %td, output_vfreq: %td\n",
		__func__, __LINE__, (ptrdiff_t)ivs, (ptrdiff_t)ovs,
		(ptrdiff_t)output_vfreq);

	//calc default vfreq
	if (pctx_pnl->info.typ_htt*pctx_pnl->info.typ_vtt > 0) {
		default_vfreq =
			((uint64_t)pctx_pnl->info.typ_dclk*VFRQRATIO_1000) /
			((uint32_t)(pctx_pnl->info.typ_htt*pctx_pnl->info.typ_vtt));

		DRM_INFO("[%s][%d]typ_htt:%td, typ_vtt:%td, typ_dclk:%td, default_vfreq:%td\n",
			__func__, __LINE__,
			(ptrdiff_t)pctx_pnl->info.typ_htt,
			(ptrdiff_t)pctx_pnl->info.typ_vtt,
			(ptrdiff_t)pctx_pnl->info.typ_dclk,
			(ptrdiff_t)default_vfreq);
	} else {
		DRM_INFO("[%s][%d]calc default vfreq fail,typ_htt:%td, typ_vtt:%td\n",
			__func__, __LINE__,
			(ptrdiff_t)pctx_pnl->info.typ_htt,
			(ptrdiff_t)pctx_pnl->info.typ_vtt);

		DRM_INFO("[%s][%d]calc default vfreq fail,typ_dclk:%td, default_vfreq: %td\n",
			__func__, __LINE__,
			(ptrdiff_t)pctx_pnl->info.typ_dclk,
			(ptrdiff_t)default_vfreq);
	}

	//calc output vtt
	if (output_vfreq > 0) {
		outputVTT = (pctx_pnl->info.typ_vtt*default_vfreq) / output_vfreq;
		DRM_INFO("[%s][%d] input_vfreq: %d, output_vfreq: %d\n",
			__func__, __LINE__, input_vfreq, output_vfreq);
		if ((pctx_pnl->info.min_vtt < outputVTT)
			&& (pctx_pnl->info.max_vtt > outputVTT)) {
			DRM_INFO("[%s][%d] in tolerance range, outputVTT: %td\n",
				__func__, __LINE__, (ptrdiff_t)outputVTT);
		} else {
			outputVTT = pctx_pnl->info.typ_vtt;
			DRM_INFO("[%s][%d] out of tolerance range free run with outputVTT: %td\n",
				__func__, __LINE__, (ptrdiff_t)outputVTT);
		}
		pctx_pnl->outputTiming_info.u16OutputVTotal = outputVTT;
		DRM_INFO("[%s][%d] pctx_pnl->outputTiming_info.u16OutputVTotal: %d\n",
			__func__, __LINE__, pctx_pnl->outputTiming_info.u16OutputVTotal);
	} else {
		pctx_pnl->outputTiming_info.u16OutputVTotal = pctx_pnl->info.typ_vtt;
		DRM_INFO("[%s][%d] calc output vtt fail, u16OutputVTotal: %td\n",
			__func__, __LINE__, (ptrdiff_t)pctx_pnl->outputTiming_info.u16OutputVTotal);
	}

	//set lock user phase diff
	phase_diff = outputVTT*FRAMESYNC_VTTV_NORMAL_PHASE_DIFF/
		FRAMESYNC_VTTV_PHASE_DIFF_DIV_1000;

	DRM_INFO("[%s][%d] phase_diff: %td\n",
		__func__, __LINE__, (ptrdiff_t)phase_diff);

	//adjust output vtt in stub mode
	outputVTT = _mtk_video_adjust_output_vtt_ans_stubMode(
			pctx, pctx_pnl, pctx_pnl->outputTiming_info.u16OutputVTotal);

	pctx_pnl->outputTiming_info.eFrameSyncMode = VIDEO_CRTC_FRAME_SYNC_FREERUN;
	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

	paramOut.reg = reg;

	//h31[0] = 0
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//BKA3A3_h16[4:0] = ivsSource
	//BKA3A3_h16[12:8] = ovsSource
	drv_hwreg_render_pnl_set_vttSourceSelect(
		&paramOut,
		bRIU,
		ivsSource,
		ovsSource);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h20[15] = 0
	//h20[14:8] = 0
	//h20[7:0] = 0
	drv_hwreg_render_pnl_set_centralVttChange(
		&paramOut,
		bRIU,
		false,
		0,
		0);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h22[15:3] = phase_diff
	//h23[15] = 1
	drv_hwreg_render_pnl_set_vttLockPhaseDiff(
		&paramOut,
		bRIU,
		1,
		phase_diff);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h24[4] = 0
	//h24[3:0] = LockKeepSequenceTh
	drv_hwreg_render_pnl_set_vttLockKeepSequenceEnable(
		&paramOut,
		bRIU,
		0,
		FRAMESYNC_RECOV_LOCKKEEPSEQUENCETH);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h33[10] = 1
	//h30[15] = 1
	//h30[14:0] = 0
	drv_hwreg_render_pnl_set_vttLockDiffLimit(
		&paramOut,
		bRIU,
		true,
		FRAMESYNC_RECOV_DIFF_LIMIT);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//VTT uper bound/lower bound is control by Tgen panel only
	//disable VTTV uper bound /lower bound
	//h33[11] = 0
	//h33[12] = 0
	//h36[15:0] = pctx_pnl->stPanelType.m_wPanelMaxVTotal
	//h37[15:0] = after y_trigger
	drv_hwreg_render_pnl_set_vttLockVttRange(
		&paramOut,
		bRIU,
		false,
		0xffff,
		0);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h33[13] = 0
	drv_hwreg_render_pnl_set_vttLockVsMaskMode(
		&paramOut,
		bRIU,
		0);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h32[7:0] = ivs
	//h32[15:8] = ovs
	drv_hwreg_render_pnl_set_vttLockIvsOvs(
		&paramOut,
		bRIU,
		0,
		0);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h3e[15:0] = 9
	drv_hwreg_render_pnl_set_vttLockLockedInLimit(
		&paramOut,
		bRIU,
		FRAMESYNC_VTTV_LOCKED_LIMIT);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[13] = 1
	drv_hwreg_render_pnl_set_vttLockUdSwModeEnable(
		&paramOut,
		bRIU,
		true);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[9] = 0
	drv_hwreg_render_pnl_set_vttLockIvsShiftEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[8] = 0
	drv_hwreg_render_pnl_set_vttLockProtectEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h23[15:0] = output VTT
		drv_hwreg_render_pnl_set_vtt(
			&paramOut,
			bRIU,
			outputVTT);

	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h31[0] = 0
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		bRIU,
		false);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	//h24[12] = 1
	drv_hwreg_render_pnl_set_vttLockLastVttLatchEnable(
		&paramOut,
		bRIU,
		1);
	RegCount = RegCount + paramOut.regCount;
	paramOut.reg = reg + RegCount;

	if ((bRIU == false) && (RegCount != 0))
		_mtk_video_panel_add_framesyncML(pctx, reg, RegCount);

	return ret;
}


static int _mtk_video_turn_off_framesync_mode(
	struct mtk_tv_kms_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}
	int ret = 0;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	bool bRIU = false;
	uint16_t RegCount = 0;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));
	paramOut.reg = reg;

	if (pctx_pnl->outputTiming_info.eFrameSyncMode ==
		VIDEO_CRTC_FRAME_SYNC_FRAMELOCK) {
		//turn off vrr mode
		//BKA3A1_24[13] = 1
		drv_hwreg_render_pnl_set_vttLockLastVttLatchToggle(
			&paramOut,
			bRIU,
			true);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
		//BKA3A0_22[0] = 0
		drv_hwreg_render_pnl_set_framelockVcntEnableDb(
			&paramOut,
			bRIU,
			false);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
	} else if (pctx_pnl->outputTiming_info.eFrameSyncMode ==
		VIDEO_CRTC_FRAME_SYNC_VTTV) {
		//turn off vttv mode
		//h20[15] = 0
		//h20[14:8] = 0
		//h20[7:0] = 0
		drv_hwreg_render_pnl_set_centralVttChange(
			&paramOut,
			bRIU,
			false,
			0,
			0);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
		//BKA3A1_31[0] = 0
		drv_hwreg_render_pnl_set_vttLockModeEnable(
			&paramOut,
			bRIU,
			false);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
		//BKA3A1_24[12] = 1
		drv_hwreg_render_pnl_set_vttLockLastVttLatchEnable(
			&paramOut,
			bRIU,
			true);
		RegCount = RegCount + paramOut.regCount;
		paramOut.reg = reg + RegCount;
	}

	if ((bRIU == false) && (RegCount != 0))
		_mtk_video_panel_add_framesyncML(pctx, reg, RegCount);

	pctx_pnl->outputTiming_info.eFrameSyncMode = VIDEO_CRTC_FRAME_SYNC_FREERUN;

	return 0;
}

int _mtk_video_set_customize_frc_table(
	struct mtk_panel_context *pctx_pnl,
	struct drm_st_pnl_frc_table_info *customizeFRCTableInfo)
{
	if (!pctx_pnl || !customizeFRCTableInfo) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx, customizeFRCTableInfo=0x%llx\n",
			__func__, __LINE__, pctx_pnl, customizeFRCTableInfo);
		return -ENODEV;
	}

	int ret = 0, FRCType = 0;

	memset(pctx_pnl->frc_info,
		0,
		sizeof(struct drm_st_pnl_frc_table_info)*PNL_FRC_TABLE_MAX_INDEX);
	memcpy(pctx_pnl->frc_info,
		customizeFRCTableInfo,
		sizeof(struct drm_st_pnl_frc_table_info)*PNL_FRC_TABLE_MAX_INDEX);

	//FRC table
	for (FRCType = 0;
		FRCType < (sizeof(pctx_pnl->frc_info)/sizeof(struct drm_st_pnl_frc_table_info));
		FRCType++) {
		DRM_INFO("[%s][%d] index = %d, u16LowerBound = %d\n",
			__func__, __LINE__,
			FRCType,
			pctx_pnl->frc_info[FRCType].u16LowerBound);
		DRM_INFO("[%s][%d] index = %d, u16HigherBound = %d\n",
			__func__, __LINE__,
			FRCType,
			pctx_pnl->frc_info[FRCType].u16HigherBound);
		DRM_INFO("[%s][%d] index = %d, u8FRC_in = %d\n",
			__func__, __LINE__,
			FRCType,
			pctx_pnl->frc_info[FRCType].u8FRC_in);
		DRM_INFO("[%s][%d] index = %d, u8FRC_out = %d\n",
			__func__, __LINE__,
			FRCType,
			pctx_pnl->frc_info[FRCType].u8FRC_out);
	}
	return 0;
}

int _mtk_video_get_customize_frc_table(
	struct mtk_panel_context *pctx_pnl,
	void *CurrentFRCTableInfo)
{
	if (!pctx_pnl || !CurrentFRCTableInfo) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx, CurrentFRCTableInfo=0x%llx\n",
			__func__, __LINE__, pctx_pnl, CurrentFRCTableInfo);
		return -ENODEV;
	}

	int ret = 0, FRCType = 0;

	memset(CurrentFRCTableInfo,
		0,
		sizeof(struct drm_st_pnl_frc_table_info)*PNL_FRC_TABLE_MAX_INDEX);
	memcpy(CurrentFRCTableInfo,
		pctx_pnl->frc_info,
		sizeof(struct drm_st_pnl_frc_table_info)*PNL_FRC_TABLE_MAX_INDEX);

	//FRC table
	for (FRCType = 0; FRCType < PNL_FRC_TABLE_MAX_INDEX; FRCType++) {
		DRM_INFO("[%s][%d] index = %d, u16LowerBound = %d\n",
			__func__, __LINE__,
			FRCType,
			((struct drm_st_pnl_frc_table_info *)
				CurrentFRCTableInfo)[FRCType].u16LowerBound);
		DRM_INFO("[%s][%d] index = %d, u16HigherBound = %d\n",
			__func__, __LINE__,
			FRCType,
			((struct drm_st_pnl_frc_table_info *)
				CurrentFRCTableInfo)[FRCType].u16HigherBound);
		DRM_INFO("[%s][%d] index = %d, u8FRC_in = %d\n",
			__func__, __LINE__,
			FRCType,
			((struct drm_st_pnl_frc_table_info *)
				CurrentFRCTableInfo)[FRCType].u8FRC_in);
		DRM_INFO("[%s][%d] index = %d, u8FRC_out = %d\n",
			__func__, __LINE__,
			FRCType,
			((struct drm_st_pnl_frc_table_info *)
				CurrentFRCTableInfo)[FRCType].u8FRC_out);
	}
	return 0;
}

int mtk_video_get_vttv_input_vfreq(
	struct mtk_tv_kms_context *pctx,
	uint64_t plane_id,
	uint64_t *input_vfreq)
{
	if (!pctx || !input_vfreq) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx, input_vfreq=0x%llx\n",
			__func__, __LINE__, pctx, input_vfreq);
		return -ENODEV;
	}

	struct drm_mode_object *obj = drm_mode_object_find(pctx->drm_dev,
		NULL, plane_id, DRM_MODE_OBJECT_ANY);
	struct drm_plane *plane = obj_to_plane(obj);
	struct mtk_drm_plane *mplane = to_mtk_plane(plane);
	struct mtk_tv_kms_context *pctx_plane =
		(struct mtk_tv_kms_context *)mplane->crtc_private;
	struct mtk_video_context *pctx_video = &pctx_plane->video_priv;
	unsigned int plane_inx = mplane->video_index;
	struct video_plane_prop *plane_props =
		(pctx_video->video_plane_properties + plane_inx);

	*input_vfreq = plane_props->prop_val[EXT_VPLANE_PROP_INPUT_VFREQ];

	return 0;
}

int _mtk_video_set_framesync_mode(
	struct mtk_tv_kms_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	int ret = 0;
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	uint64_t input_vfreq = 0, framesync_mode = 0, output_vfreq = 0;
	uint64_t framesync_plane_id = 0;
	bool bLowLatencyEn = false;
	struct drm_mode_object *obj = NULL;

	pctx_pnl->outputTiming_info.AutoForceFreeRun = false;
	framesync_mode = crtc_props->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE];
	framesync_plane_id = crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID];
	obj = drm_mode_object_find(pctx->drm_dev, NULL, framesync_plane_id,
		DRM_MODE_OBJECT_ANY);

	if ((!pctx->stub) && (mtk_video_panel_get_framesync_mode_en(pctx) == true) &&
		(IS_FRAMESYNC(framesync_mode) == true)
		|| (pctx_pnl->outputTiming_info.ForceFreeRun == true)) {
		framesync_mode = VIDEO_CRTC_FRAME_SYNC_FREERUN;
	}

	DRM_INFO("[%s][%d] framesync_mode = %d, framesync_state = %d\n",
		__func__, __LINE__, framesync_mode,
		pctx_pnl->outputTiming_info.eFrameSyncState);

	if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_VTTV) {
		if (!obj) {
			DRM_ERROR("[%s, %d]: null ptr! obj=0x%llx\n",
				__func__, __LINE__, obj);
			return -ENODEV;
		}
		mtk_video_get_vttv_input_vfreq(pctx,
			crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID],
			&input_vfreq);
		DRM_INFO("[%s][%d] EXT_VPLANE_PROP_INPUT_VFREQ= %td\n",
			__func__, __LINE__, (ptrdiff_t)input_vfreq);

		bLowLatencyEn =
			(bool)crtc_props->prop_val[E_EXT_CRTC_PROP_LOW_LATENCY_MODE];
		_mtk_video_turn_off_framesync_mode(pctx);
		_mtk_video_panel_set_input_source_select(pctx);
		ret = _mtk_video_set_vttv_mode(pctx, input_vfreq, bLowLatencyEn);
		if (ret == -EPERM) {
			pctx_pnl->outputTiming_info.AutoForceFreeRun = true;
			output_vfreq = pctx_pnl->outputTiming_info.u32OutputVfreq;
			ret = _mtk_video_set_freerun_vfreq(pctx, input_vfreq, output_vfreq);
		}

	} else if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_FRAMELOCK) {
		if (!obj) {
			DRM_ERROR("[%s, %d]: null ptr! obj=0x%llx\n",
				__func__, __LINE__, obj);
			return -ENODEV;
		}
		_mtk_video_turn_off_framesync_mode(pctx);
		_mtk_video_panel_set_input_source_select(pctx);
		_mtk_video_set_vrr_mode(pctx);
	} else if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_FREERUN) {
		output_vfreq =
			crtc_props->prop_val[E_EXT_CRTC_PROP_FREERUN_VFREQ];
		DRM_INFO("[%s][%d] VIDEO_CRTC_FRAME_SYNC_FREERUN mode\n",
			__func__, __LINE__);
		DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_FREERUN_VFREQ= %td\n",
			__func__, __LINE__, (ptrdiff_t)output_vfreq);
		_mtk_video_turn_off_framesync_mode(pctx);
		_mtk_video_set_freerun_vfreq(pctx, input_vfreq, output_vfreq);
	}
	return ret;
}

int mtk_get_framesync_locked_flag(
	struct mtk_tv_kms_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	bool bRIU = true, locked_total = false, locked_phase = false;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	if (pctx->stub) {
		pctx_pnl->outputTiming_info.locked_flag = true;
		return 0;
	}

	switch (pctx_pnl->outputTiming_info.eFrameSyncMode) {
	case VIDEO_CRTC_FRAME_SYNC_VTTV:
		drv_hwreg_render_pnl_get_vttvLockedFlag(&paramOut, bRIU);
		pctx_pnl->outputTiming_info.locked_flag = paramOut.reg[0].val;
		break;
	case VIDEO_CRTC_FRAME_SYNC_FRAMELOCK:
		drv_hwreg_render_pnl_get_chaseTotalStatus(&paramOut, bRIU);
		locked_total = paramOut.reg[0].val;
		drv_hwreg_render_pnl_get_chasePhaseStatus(&paramOut, bRIU);
		locked_phase = paramOut.reg[0].val;
		pctx_pnl->outputTiming_info.locked_flag = (locked_total && locked_phase);
		break;
	default:
		pctx_pnl->outputTiming_info.locked_flag = false;
		break;
	}

	return 0;
}

bool mtk_video_panel_get_framesync_mode_en(
	struct mtk_tv_kms_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	bool bRIU = true, framesync_en = false;
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	uint16_t reg_num = pctx_video->reg_num;
	struct reg_info reg[reg_num];
	struct hwregOut paramOut;

	memset(&reg, 0, sizeof(reg));
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	switch (pctx_pnl->outputTiming_info.eFrameSyncMode) {
	case VIDEO_CRTC_FRAME_SYNC_VTTV:
		drv_hwreg_render_pnl_get_vttLockModeEnable(&paramOut, bRIU);
		framesync_en = paramOut.reg[0].val;
		break;
	case VIDEO_CRTC_FRAME_SYNC_FRAMELOCK:
		drv_hwreg_render_pnl_get_framelockVcntEnableDb(&paramOut, bRIU);
		framesync_en = paramOut.reg[0].val;
		break;
	default:
		framesync_en = false;
		break;
	}

	return framesync_en;
}

int mtk_video_panel_update_framesync_state(
	struct mtk_tv_kms_context *pctx)
{
	if (!pctx) {
		DRM_ERROR("[%s, %d]: null ptr! pctx=0x%llx\n",
			__func__, __LINE__, pctx);
		return -ENODEV;
	}

	switch (pctx->panel_priv.outputTiming_info.eFrameSyncState) {
	case E_PNL_FRAMESYNC_STATE_PROP_IN:
		pctx->panel_priv.outputTiming_info.eFrameSyncState =
			E_PNL_FRAMESYNC_STATE_PROP_FIRE;
		break;
	case E_PNL_FRAMESYNC_STATE_IRQ_IN:
		pctx->panel_priv.outputTiming_info.eFrameSyncState =
			E_PNL_FRAMESYNC_STATE_IRQ_FIRE;
		break;
	default:
		DRM_INFO("[%s, %d]: FrameSyncState=%d\n",
			__func__, __LINE__,
			pctx->panel_priv.outputTiming_info.eFrameSyncState);
		break;
	}

	return 0;
}

int _mtk_render_check_vbo_ctrlbit_feature(struct mtk_panel_context *pctx_pnl,
		uint8_t u8connectid, en_drv_vbo_ctrlbit_feature enfeature)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	uint32_t retValue = 0;
	uint8_t gfid_g_cnt = 0;
	const uint8_t gfid_ratio = 2;
	uint32_t u32OutputVfreq = 0;

	u32OutputVfreq = pctx_pnl->outputTiming_info.u32OutputVfreq/VFRQRATIO_1000;
	switch (enfeature) {
	case E_DRV_VBO_CTRLBIT_GLOBAL_FRAMEID:
		if (u8connectid == CONNECTID_MAIN)
			retValue = pctx_pnl->u8controlbit_gfid;
		else if (u8connectid == CONNECTID_DELTA)
			retValue = pctx_pnl->u8controlbit_gfid;
		else if (u8connectid == CONNECTID_GFX) {
			if (IS_VFREQ_120HZ_GROUP(u32OutputVfreq))
				retValue = pctx_pnl->u8controlbit_gfid+gfid_ratio;
			else
				retValue = pctx_pnl->u8controlbit_gfid;
		}
		break;
	case E_DRV_VBO_CTRLBIT_HTOTAL:
		if (u8connectid == CONNECTID_MAIN)
			retValue = pctx_pnl->info.typ_htt;
		else if (u8connectid == CONNECTID_DELTA)
			retValue = pctx_pnl->extdev_info.typ_htt;
		else if (u8connectid == CONNECTID_GFX)
			retValue = pctx_pnl->gfx_info.typ_htt;

		break;
	case E_DRV_VBO_CTRLBIT_VTOTAL:
		if (u8connectid == CONNECTID_MAIN)
			retValue = pctx_pnl->info.typ_vtt;
		else if (u8connectid == CONNECTID_DELTA)
			retValue = pctx_pnl->extdev_info.typ_vtt;
		else if (u8connectid == CONNECTID_GFX)
			retValue = pctx_pnl->gfx_info.typ_vtt;

		break;
	case E_DRV_VBO_CTRLBIT_HSTART_POS:
		if (u8connectid == CONNECTID_MAIN)
			retValue = pctx_pnl->info.de_hstart;
		else if (u8connectid == CONNECTID_DELTA)
			retValue = pctx_pnl->extdev_info.de_hstart;
		else if (u8connectid == CONNECTID_GFX)
			retValue = pctx_pnl->gfx_info.de_hstart;

		break;
	case E_DRV_VBO_CTRLBIT_VSTART_POS:
		if (u8connectid == CONNECTID_MAIN)
			retValue = pctx_pnl->info.de_vstart;
		else if (u8connectid == CONNECTID_DELTA)
			retValue = pctx_pnl->extdev_info.de_vstart;
		else if (u8connectid == CONNECTID_GFX)
			retValue = pctx_pnl->gfx_info.de_vstart;

		break;
	default:
		break;
	}

	return retValue;

}
int mtk_render_set_vbo_ctrlbit(
	struct mtk_panel_context *pctx_pnl,
	struct drm_st_ctrlbits *pctrlbits)
{
	if (!pctx_pnl) {
		DRM_ERROR("[%s, %d]: null ptr! pctx_pnl=0x%llx\n",
			__func__, __LINE__, pctx_pnl);
		return -ENODEV;
	}

	uint8_t ctrlbitlaneNum = 0;
	uint16_t ctrlbitIndex = 0;

	st_drv_ctrlbits stPnlCtrlbits = {0};

	stPnlCtrlbits.u8ConnectorID = pctrlbits->u8ConnectorID;
	stPnlCtrlbits.u8CopyType = pctrlbits->u8CopyType;

	for (ctrlbitIndex = 0; ctrlbitIndex < CTRLBIT_MAX_NUM; ctrlbitIndex++) {
		stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].bEnable =
				pctrlbits->ctrlbit_item[ctrlbitIndex].bEnable;
		stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].u8LaneID =
				pctrlbits->ctrlbit_item[ctrlbitIndex].u8LaneID;
		stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].u8Lsb =
				pctrlbits->ctrlbit_item[ctrlbitIndex].u8Lsb;
		stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].u8Msb =
				pctrlbits->ctrlbit_item[ctrlbitIndex].u8Msb;
		stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].enFeature =
				(en_drv_vbo_ctrlbit_feature)pctrlbits
				->ctrlbit_item[ctrlbitIndex].enFeature;

		if ((stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].enFeature !=
			E_DRV_VBO_CTRLBIT_NO_FEATURE) &&
			(stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].enFeature !=
			E_DRV_VBO_CTRLBIT_NUM)) {
			stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].u32Value =
				_mtk_render_check_vbo_ctrlbit_feature
				(pctx_pnl, stPnlCtrlbits.u8ConnectorID,
				stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].enFeature);
		} else {
			stPnlCtrlbits.ctrlbit_item[ctrlbitIndex].u32Value =
				pctrlbits->ctrlbit_item[ctrlbitIndex].u32Value;
		}
	}

	return drv_hwreg_render_pnl_set_vbo_ctrlbit(&stPnlCtrlbits);
}

int mtk_render_set_swing_vreg(struct drm_st_out_swing_level *stdrmSwing)
{
	if (!stdrmSwing) {
		DRM_ERROR("[%s, %d]: stdrmSwing is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	uint16_t SwingIndex = 0;
	st_out_swing_level stSwing = {0};

	stSwing.usr_swing_level = stdrmSwing->usr_swing_level;
	stSwing.common_swing = stdrmSwing->common_swing;
	stSwing.ctrl_lanes = stdrmSwing->ctrl_lanes;

	if ((stSwing.usr_swing_level != 0) && (stSwing.ctrl_lanes <= DRV_VBO_MAX_NUM)) {
		for (SwingIndex = 0 ; SwingIndex < stSwing.ctrl_lanes ; SwingIndex++)
			stSwing.swing_level[SwingIndex] = stdrmSwing->swing_level[SwingIndex];
	} else {
		DRM_ERROR("[%s, %d]: usr_swing_level setting not crrect def=%d lan=%d\n",
			__func__, __LINE__, stSwing.usr_swing_level, stSwing.ctrl_lanes);
		return -ENODEV;
	}
	drv_hwreg_render_pnl_set_swing_vreg(&stSwing);

	return 0;
}

int mtk_render_get_swing_vreg(struct drm_st_out_swing_level *stdrmSwing)
{
	if (!stdrmSwing) {
		DRM_ERROR("[%s, %d]: stdrmSwing is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	uint16_t SwingIndex = 0;
	st_out_swing_level stSwing = {0};

	drv_hwreg_render_pnl_get_swing_vreg(&stSwing);

	for (SwingIndex = 0 ; SwingIndex < DRV_VBO_MAX_NUM ; SwingIndex++)
		stdrmSwing->swing_level[SwingIndex] = stSwing.swing_level[SwingIndex];

	return 0;
}

int mtk_render_set_pre_emphasis(struct drm_st_out_pe_level *stdrmPE)
{
	if (!stdrmPE) {
		DRM_ERROR("[%s, %d]: stdrmPE is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	uint16_t PEIndex = 0;
	st_out_pe_level stPE = {0};

	stPE.usr_pe_level = stdrmPE->usr_pe_level;
	stPE.common_pe = stdrmPE->common_pe;
	stPE.ctrl_lanes = stdrmPE->ctrl_lanes;

	if ((stPE.usr_pe_level != 0) && (stPE.ctrl_lanes <= DRV_VBO_MAX_NUM)) {
		for (PEIndex = 0 ; PEIndex < stPE.ctrl_lanes ; PEIndex++)
			stPE.pe_level[PEIndex] = stdrmPE->pe_level[PEIndex];
	} else {
		DRM_ERROR("[%s, %d]: usr_pe_level setting not crrect def=%d lan=%d\n",
			__func__, __LINE__, stPE.usr_pe_level, stPE.ctrl_lanes);
		return -ENODEV;
	}

	drv_hwreg_render_pnl_set_pre_emphasis_enable();
	drv_hwreg_render_pnl_set_pre_emphasis(&stPE);

	return 0;
}

int mtk_render_get_pre_emphasis(struct drm_st_out_pe_level *stdrmPE)
{
	if (!stdrmPE) {
		DRM_ERROR("[%s, %d]: stdrmPE is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	uint16_t PEIndex = 0;
	st_out_pe_level stPE = {0};

	drv_hwreg_render_pnl_get_pre_emphasis(&stPE);

	for (PEIndex = 0 ; PEIndex < DRV_VBO_MAX_NUM ; PEIndex++)
		stdrmPE->pe_level[PEIndex] = stPE.pe_level[PEIndex];

	return 0;
}

int mtk_render_set_ssc(struct drm_st_out_ssc *stdrmSSC)
{
	if (!stdrmSSC) {
		DRM_ERROR("[%s, %d]: stdrmSSC is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	st_out_ssc_ctrl stSSC = {0};

	stSSC.ssc_en = stdrmSSC->ssc_en;
	stSSC.ssc_modulation = stdrmSSC->ssc_modulation;
	stSSC.ssc_deviation = stdrmSSC->ssc_deviation;

	drv_hwreg_render_pnl_set_ssc(&stSSC);

	return 0;
}

int mtk_render_get_ssc(struct drm_st_out_ssc *stdrmSSC)
{
	if (!stdrmSSC) {
		DRM_ERROR("[%s, %d]: stdrmSSC is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	st_out_ssc_ctrl stSSC = {0};

	drv_hwreg_render_pnl_get_ssc(&stSSC);

	stdrmSSC->ssc_en = stSSC.ssc_en;
	stdrmSSC->ssc_modulation = stSSC.ssc_modulation;
	stdrmSSC->ssc_deviation = stSSC.ssc_deviation;

	return 0;
}

int mtk_render_get_mod_status(struct drm_st_mod_status *stdrmMod)
{
	if (!stdrmMod) {
		DRM_ERROR("[%s, %d]: stdrmMod is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	st_drv_pnl_mod_status stdrvMod = {0};

	drv_hwreg_render_pnl_get_mod_status(&stdrvMod);

	stdrmMod->vby1_locked = stdrvMod.vby1_locked;
	stdrmMod->vby1_unlockcnt = stdrvMod.vby1_unlockcnt;
	stdrmMod->vby1_htpdn = stdrvMod.vby1_htpdn;
	stdrmMod->vby1_lockn = stdrvMod.vby1_lockn;
	stdrmMod->vby1_htpdn = stdrvMod.vby1_htpdn;

	stdrmMod->outconfig.outconfig_ch0_ch7 = stdrvMod.outconfig.outconfig_ch0_ch7;
	stdrmMod->outconfig.outconfig_ch8_ch15 = stdrvMod.outconfig.outconfig_ch8_ch15;
	stdrmMod->outconfig.outconfig_ch16_ch19 = stdrvMod.outconfig.outconfig_ch16_ch19;

	stdrmMod->freeswap.freeswap_ch0_ch1 = stdrvMod.freeswap.freeswap_ch0_ch1;
	stdrmMod->freeswap.freeswap_ch2_ch3 = stdrvMod.freeswap.freeswap_ch2_ch3;
	stdrmMod->freeswap.freeswap_ch4_ch5 = stdrvMod.freeswap.freeswap_ch4_ch5;
	stdrmMod->freeswap.freeswap_ch6_ch7 = stdrvMod.freeswap.freeswap_ch6_ch7;
	stdrmMod->freeswap.freeswap_ch8_ch9 = stdrvMod.freeswap.freeswap_ch8_ch9;
	stdrmMod->freeswap.freeswap_ch10_ch11 = stdrvMod.freeswap.freeswap_ch10_ch11;
	stdrmMod->freeswap.freeswap_ch12_ch13 = stdrvMod.freeswap.freeswap_ch12_ch13;
	stdrmMod->freeswap.freeswap_ch14_ch15 = stdrvMod.freeswap.freeswap_ch14_ch15;
	stdrmMod->freeswap.freeswap_ch16_ch17 = stdrvMod.freeswap.freeswap_ch16_ch17;
	stdrmMod->freeswap.freeswap_ch18_ch19 = stdrvMod.freeswap.freeswap_ch18_ch19;
	stdrmMod->freeswap.freeswap_ch20_ch21 = stdrvMod.freeswap.freeswap_ch20_ch21;
	stdrmMod->freeswap.freeswap_ch22_ch23 = stdrvMod.freeswap.freeswap_ch22_ch23;
	stdrmMod->freeswap.freeswap_ch24_ch25 = stdrvMod.freeswap.freeswap_ch24_ch25;
	stdrmMod->freeswap.freeswap_ch26_ch27 = stdrvMod.freeswap.freeswap_ch26_ch27;
	stdrmMod->freeswap.freeswap_ch28_ch29 = stdrvMod.freeswap.freeswap_ch28_ch29;
	stdrmMod->freeswap.freeswap_ch30_ch31 = stdrvMod.freeswap.freeswap_ch30_ch31;

	return 0;
}

static int mtk_tv_drm_panel_disable(struct drm_panel *panel)
{
	//If needed, control backlight in panel_enable
	struct mtk_panel_context *pStPanelCtx = NULL;

	if (!panel) {
		DRM_ERROR("[%s, %d]: panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pStPanelCtx = container_of(panel, struct mtk_panel_context, drmPanel);

	//control backlight in panel enable/disable depend on pm mode/ use case.
	gpiod_set_value(pStPanelCtx->gpio_backlight, 0);

	return 0;
}

static int mtk_tv_drm_panel_unprepare(struct drm_panel *panel)
{
	struct mtk_panel_context *pStPanelCtx = NULL;

	if (!panel) {
		DRM_ERROR("[%s, %d]: panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pStPanelCtx = container_of(panel, struct mtk_panel_context, drmPanel);

	if (gpiod_get_value(pStPanelCtx->gpio_vcc) != 1)
		gpiod_set_value(pStPanelCtx->gpio_vcc, 1);
	else
		DRM_INFO("[%s]Panel VCC is already enabled\n", __func__);

	return 0;
}

int mtk_tv_drm_panel_checkDTSPara(drm_st_pnl_info *pStPnlInfo)
{
	if (!pStPnlInfo) {
		DRM_ERROR("[%s, %d]: pStPnlInfo is NULL.\n",
			__func__, __LINE__);

		return -ENODEV;
	}

	// HTT < Hde_start + Hde
	if (pStPnlInfo->typ_htt <= pStPnlInfo->de_hstart + pStPnlInfo->de_width) {
		DRM_ERROR("[%s, %d]: HTT < Hde_start + Hde\n",
			__func__, __LINE__);
		return -EPERM;
	}

	// Hde_start < Hsync_s + Hsync_w
	if (pStPnlInfo->de_hstart < pStPnlInfo->hsync_st + pStPnlInfo->hsync_w) {
		DRM_ERROR("[%s, %d]: Hde_start < Hsync_s + Hsync_w\n",
			__func__, __LINE__);
		return -EPERM;
	}

    // VTT < Vde_start + Vde
	if (pStPnlInfo->typ_vtt <= pStPnlInfo->de_vstart + pStPnlInfo->de_height) {
		DRM_ERROR("[%s, %d]:  VTT < Vde_start + Vde\n",
			__func__, __LINE__);
		return -EPERM;
	}

	// Vde_start < Vsync_s + Vsync_w
	if (pStPnlInfo->de_vstart < pStPnlInfo->vsync_st + pStPnlInfo->vsync_w) {
		DRM_ERROR("[%s, %d]: Vde_start < Vsync_s + Vsync_w\n",
			__func__, __LINE__);
		return -EPERM;
	}

	// VTT_max < Vtt_typ
	if (pStPnlInfo->max_vtt < pStPnlInfo->typ_vtt) {
		DRM_ERROR("[%s, %d]: VTT_max < Vtt_typ\n",
			__func__, __LINE__);
		return -EPERM;
	}

	// VTT_min > Vtt_typ
	if (pStPnlInfo->min_vtt > pStPnlInfo->typ_vtt) {
		DRM_ERROR("[%s, %d]: VTT_min > Vtt_typ\n",
			__func__, __LINE__);
		return -EPERM;
	}

    // VTT_min < Vde + Vde_start
	if (pStPnlInfo->min_vtt < pStPnlInfo->de_vstart
		+ pStPnlInfo->de_height) {
		DRM_ERROR("[%s, %d]: VTT_min < Vde + Vde_start\n",
			__func__, __LINE__);
		return -EPERM;
	}

	return 0;
}

static int mtk_tv_drm_panel_get_fix_ModeID(struct drm_panel *panel)
{
	struct mtk_panel_context *pStPanelCtx = NULL;
	struct drm_device *kms_drm_dev = NULL;
	uint16_t kms_vtt = 0, panel_vtt = 0;
	struct mtk_tv_drm_connector *mtk_connector = NULL;
	struct drm_connector *drm_con = NULL;
	struct mtk_tv_kms_context *pctx = NULL;

	if (!panel) {
		DRM_ERROR("[%s, %d]: panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	drm_con = panel->connector;
	mtk_connector = to_mtk_tv_connector(drm_con);
	pctx = (struct mtk_tv_kms_context *)mtk_connector->connector_private;

	return pctx->panel_priv.disable_ModeID_change;
}


static int mtk_tv_drm_panel_update_kms_pnl_info(struct drm_panel *panel)
{
	struct mtk_panel_context *pStPanelCtx = NULL;
	struct mtk_tv_kms_context *pctx = NULL;
	struct drm_device *kms_drm_dev = NULL;
	uint16_t kms_vtt = 0, panel_vtt = 0;
	struct mtk_tv_drm_connector *mtk_connector = NULL;
	struct drm_connector *drm_con = NULL;

	if (!panel) {
		DRM_ERROR("[%s, %d]: panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	pStPanelCtx = container_of(panel, struct mtk_panel_context, drmPanel);

	mtk_tv_drm_panel_checkDTSPara(&pStPanelCtx->info);

	kms_drm_dev = panel->drm;

	drm_con = panel->connector;
	mtk_connector = to_mtk_tv_connector(drm_con);
	pctx = (struct mtk_tv_kms_context *)mtk_connector->connector_private;
	memcpy(&pctx->panel_priv.info, &pStPanelCtx->info, sizeof(drm_st_pnl_info));
	return 0;
}

static int mtk_tv_drm_panel_prepare(struct drm_panel *panel)
{
	struct mtk_panel_context *pStPanelCtx = NULL;
	struct drm_crtc_state *drmCrtcState = NULL;
	struct mtk_tv_kms_context *pctx = NULL;
	int ret = 0;

	if (!panel) {
		DRM_ERROR("[%s, %d]: panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	panel_common_enable();

	drmCrtcState = panel->connector->state->crtc->state;

	pStPanelCtx = container_of(panel, struct mtk_panel_context, drmPanel);

	if (drmCrtcState->mode_changed
		&& !(_isSameModeAndPanelInfo(&drmCrtcState->mode, &pStPanelCtx->info))) {

		ret = _copyPanelInfo(drmCrtcState, &pStPanelCtx->info, &pStPanelCtx->panelInfoList);

		if (ret) {
			DRM_ERROR("[%s, %d]: new panel mode does not set.\n",
				__func__, __LINE__);
		} else {
			mtk_tv_drm_panel_update_kms_pnl_info(panel);

			if (mtk_tv_drm_panel_get_fix_ModeID(panel)) {
				DRM_ERROR("[%s, %d]: fix_ModeID.\n",
				__func__, __LINE__);
				return 0;
				}

			if ((pStPanelCtx->info.typ_dclk != u64PreTypDclk) ||
				(pStPanelCtx->info.op_format != ePreOutputFormat) ||
				(pStPanelCtx->info.vbo_byte != ePreByteMode))
				pStPanelCtx->out_upd_type = E_MODE_RESET_TYPE;
			else
				pStPanelCtx->out_upd_type = E_MODE_CHANGE_TYPE;

			mtk_panel_init(panel->dev);
			pStPanelCtx->out_upd_type = E_NO_UPDATE_TYPE;

			//check VCC status first, then enable VCC
			if (gpiod_get_value(pStPanelCtx->gpio_vcc) != 0)
				gpiod_set_value(pStPanelCtx->gpio_vcc, 0);
			else
				DRM_INFO("[%s]Panel VCC is already enabled\n", __func__);
		}
	}
	return 0;
}

static int mtk_tv_drm_panel_enable(struct drm_panel *panel)
{
	struct mtk_panel_context *pStPanelCtx = NULL;

	if (!panel) {
		DRM_ERROR("[%s, %d]: panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pStPanelCtx = container_of(panel, struct mtk_panel_context, drmPanel);

	//control backlight in panel enable/disable depend on pm mode/ use case.
	gpiod_set_value(pStPanelCtx->gpio_backlight, 1);

	return 0;
}

static int mtk_tv_drm_panel_get_modes(struct drm_panel *panel)
{
	struct mtk_panel_context *pStPanelCtx = NULL;
	int modeCouct = 0;

	if (!panel) {
		DRM_ERROR("[%s, %d]: drm_panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pStPanelCtx = container_of(panel, struct mtk_panel_context, drmPanel);

	_panelAddModes(panel,
		&pStPanelCtx->panelInfoList,
		&modeCouct);

	pStPanelCtx->u8TimingsNum = modeCouct;

	return pStPanelCtx->u8TimingsNum;
}

static int mtk_tv_drm_panel_get_timings(struct drm_panel *panel,
							unsigned int num_timings,
							struct display_timing *timings)
{
	uint32_t u32NumTiming = 1;
	struct mtk_panel_context *pStPanelCtx = NULL;

	pStPanelCtx = container_of(panel, struct mtk_panel_context, drmPanel);

	if (!timings) {
		DRM_WARN("\033[1;31m [%s][%d]pointer is NULL \033[0m\n",
							__func__, __LINE__);
		return u32NumTiming;
	}
	timings[0] = pStPanelCtx->info.displayTiming;
	return u32NumTiming;
}

int mtk_tv_drm_video_get_outputinfo_ioctl(
	struct drm_device *drm_dev,
	void *data,
	struct drm_file *file_priv)
{
	if (!drm_dev || !data || !file_priv)
		return -EINVAL;

	struct drm_mtk_tv_dip_capture_info *outputinfo = (struct drm_mtk_tv_dip_capture_info *)data;
	struct mtk_tv_kms_context *pctx = NULL;
	struct drm_crtc *crtc = NULL;
	struct mtk_tv_drm_crtc *mtk_tv_crtc = NULL;
	struct mtk_panel_context *pctx_pnl = NULL;
	struct mtk_video_context *pctx_video = NULL;
	struct mtk_video_plane_ctx *plane_ctx = NULL;
	int i = 0;

	drm_for_each_crtc(crtc, drm_dev) {
		mtk_tv_crtc = to_mtk_tv_crtc(crtc);
		break;
	}
	pctx = (struct mtk_tv_kms_context *)mtk_tv_crtc->crtc_private;
	pctx_video = &pctx->video_priv;
	pctx_pnl = &pctx->panel_priv;

	outputinfo->video.p_engine = pctx->render_p_engine;
	outputinfo->video.color_fmt = pctx_pnl->v_cfg.format;
	outputinfo->video_osdb.p_engine = pctx->render_p_engine;
	outputinfo->video_osdb.color_fmt = pctx_pnl->v_cfg.format;
	outputinfo->video_post.p_engine = pctx->render_p_engine;
	outputinfo->video_post.color_fmt = pctx_pnl->v_cfg.format;
	return 0;
}

static int mtk_tv_drm_panel_bind(
	struct device *compDev,
	struct device *master,
	void *masterData)
{
	if (!compDev) {
		DRM_ERROR("[%s, %d]: compDev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	mtk_panel_init(compDev);
	return 0;
}

static void mtk_tv_drm_panel_unbind(
	struct device *compDev,
	struct device *master,
	void *masterData)
{

}

static int mtk_tv_drm_panel_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mtk_panel_context *pctx_pnl = NULL;
	const struct of_device_id *deviceId = NULL;

	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	deviceId = of_match_node(mtk_tv_drm_panel_of_match_table, pdev->dev.of_node);

	if (!deviceId) {
		DRM_WARN("of_match_node failed\n");
		return -EINVAL;
	}

	pctx_pnl = devm_kzalloc(&pdev->dev, sizeof(struct mtk_panel_context), GFP_KERNEL);
	INIT_LIST_HEAD(&pctx_pnl->panelInfoList);
	pctx_pnl->dev = &pdev->dev;
	dev_set_drvdata(pctx_pnl->dev, pctx_pnl);

	drm_panel_init(&pctx_pnl->drmPanel);
	pctx_pnl->drmPanel.dev = pctx_pnl->dev;
	pctx_pnl->drmPanel.funcs = &drmPanelFuncs;
	ret = drm_panel_add(&pctx_pnl->drmPanel);
	ret = component_add(&pdev->dev, &mtk_tv_drm_panel_component_ops);

	if (ret) {
		DRM_ERROR("[%s, %d]: component_add fail\n",
			__func__, __LINE__);
	}

	/*read panel related data from dtb*/
	ret = readDTB2PanelPrivate(pctx_pnl);
	if (ret != 0x0) {
		DRM_INFO("[%s, %d]: readDTB2PanelPrivate  failed\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pctx_pnl->gpio_vcc = devm_gpiod_get(&pdev->dev, "vcc", GPIOD_OUT_LOW);
	pctx_pnl->gpio_backlight = devm_gpiod_get(&pdev->dev, "backlight", GPIOD_OUT_LOW);
	if (IS_ERR(pctx_pnl->gpio_vcc))
		DRM_ERROR("get gpio_vcc gpio desc failed\n");

	if (IS_ERR(pctx_pnl->gpio_backlight))
		DRM_ERROR("get gpio_backlight gpio desc failed\n");

	return ret;
}

static int mtk_tv_drm_panel_remove(struct platform_device *pdev)
{
	struct mtk_panel_context *pStPanelCtx = NULL;
	struct list_head *tmpInfoNode = NULL, *n;
	drm_st_pnl_info *tmpInfo = NULL;

	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	//free link list of drm_st_pnl_info
	pStPanelCtx = pdev->dev.driver_data;
	list_for_each_safe(tmpInfoNode, n, &pStPanelCtx->panelInfoList) {
		tmpInfo = list_entry(tmpInfoNode, drm_st_pnl_info, list);
		list_del(&tmpInfo->list);
		kfree(tmpInfo);
		tmpInfo = NULL;
	}

	component_del(&pdev->dev, &mtk_tv_drm_panel_component_ops);

	return 0;
}

static int mtk_tv_drm_panel_suspend(struct platform_device *pdev, pm_message_t state)
{
	panel_common_disable();

	return 0;
}

static int mtk_tv_drm_panel_resume(struct platform_device *pdev)
{
	struct mtk_panel_context *pStPanelCtx = NULL;

	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	pStPanelCtx = pdev->dev.driver_data;

	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: panel context is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pStPanelCtx->out_upd_type = E_MODE_RESET_TYPE;
	panel_common_enable();
	mtk_panel_init(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM
static int panel_resume_debugtestpattern(struct device *dev)
{
	#define PATTERN_RED_VAL (0xFFF)
	struct mtk_panel_context *pStPanelCtx = NULL;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_connector *mtk_connector = NULL;
	struct drm_connector *drm_con = NULL;
	drm_st_pixel_mute_info pixel_mute_info = {false};
	bool bTconPatternEn = false;
	bool bGopPatternEn = false;
	bool bMultiwinPatternEn = false;

	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pStPanelCtx = dev->driver_data;

	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: panel context is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	drm_con = pStPanelCtx->drmPanel.connector;
	mtk_connector = to_mtk_tv_connector(drm_con);
	pctx = (struct mtk_tv_kms_context *)mtk_connector->connector_private;

	switch (pctx->eResumePatternselect) {
	case E_RESUME_PATTERN_SEL_DEFAULT:
		break;

	case E_RESUME_PATTERN_SEL_MOD:
		{
			pixel_mute_info.bEnable = true;
			pixel_mute_info.u32Red = PATTERN_RED_VAL;
			pixel_mute_info.u32Green = false;
			pixel_mute_info.u32Blue = false;
		}
	break;

	case E_RESUME_PATTERN_SEL_TCON:
		{
			bTconPatternEn = true;
		}
		break;

	case E_RESUME_PATTERN_SEL_GOP:
		{
			bGopPatternEn = true;
		}
		break;

	case E_RESUME_PATTERN_SEL_MULTIWIN:
		{
			bMultiwinPatternEn = true;
		}
		break;

	default:
		break;
	}

	mtk_render_set_pixel_mute_video(pixel_mute_info);
	mtk_render_set_tcon_pattern_en(bTconPatternEn);
	mtk_render_set_gop_pattern_en(bGopPatternEn);
	mtk_render_set_multiwin_pattern_en(bMultiwinPatternEn);

	#undef PATTERN_RED_VAL
	return 0;
}

static int panel_runtime_suspend(struct device *dev)
{
	panel_common_disable();

	return 0;
}

static int panel_runtime_resume(struct device *dev)
{
	struct mtk_panel_context *pStPanelCtx = NULL;

	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	pStPanelCtx = dev->driver_data;

	pStPanelCtx->out_upd_type = E_MODE_RESET_TYPE;
	panel_common_enable();
	mtk_panel_init(dev);

	return 0;
}

static int panel_pm_runtime_force_suspend(struct device *dev)
{
	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	panel_common_disable();

	return pm_runtime_force_suspend(dev);
}
static int panel_pm_runtime_force_resume(struct device *dev)
{
	struct mtk_panel_context *pStPanelCtx = NULL;
	struct mtk_tv_kms_context *pctx = NULL;
	struct mtk_tv_drm_connector *mtk_connector = NULL;
	struct drm_connector *drm_con = NULL;

	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	pStPanelCtx = dev->driver_data;

	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: panel context is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	drm_con = pStPanelCtx->drmPanel.connector;
		mtk_connector = to_mtk_tv_connector(drm_con);
		pctx = (struct mtk_tv_kms_context *)mtk_connector->connector_private;


	pStPanelCtx->out_upd_type = E_MODE_RESET_TYPE;
	panel_common_enable();

	panel_resume_debugtestpattern(dev);

	mtk_panel_init(dev);

	return pm_runtime_force_resume(dev);
}
#endif

MODULE_AUTHOR("Mediatek TV group");
MODULE_DESCRIPTION("Mediatek SoC DRM driver");
MODULE_LICENSE("GPL v2");
