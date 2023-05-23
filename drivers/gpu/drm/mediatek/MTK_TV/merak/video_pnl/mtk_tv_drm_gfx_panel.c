// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include "mtk_tv_drm_gfx_panel.h"
#include "mtk_tv_drm_gop.h"


//-------------------------------------------------------------------------------------------------
// Prototype
//-------------------------------------------------------------------------------------------------
static int mtk_tv_drm_gfx_panel_disable(struct drm_panel *panel);
static int mtk_tv_drm_gfx_panel_unprepare(struct drm_panel *panel);
static int mtk_tv_drm_gfx_panel_prepare(struct drm_panel *panel);
static int mtk_tv_drm_gfx_panel_enable(struct drm_panel *panel);
static int mtk_tv_drm_gfx_panel_get_modes(struct drm_panel *panel);
static int mtk_tv_drm_gfx_panel_get_timings(struct drm_panel *panel,
						unsigned int num_timings,
						struct display_timing *timings);

static int mtk_tv_drm_gfx_panel_probe(struct platform_device *pdev);
static int mtk_tv_drm_gfx_panel_remove(struct platform_device *pdev);
static int mtk_tv_drm_gfx_panel_suspend(struct platform_device *pdev, pm_message_t state);
static int mtk_tv_drm_gfx_panel_resume(struct platform_device *pdev);
static int mtk_tv_drm_gfx_panel_bind(
	struct device *compDev,
	struct device *master,
	void *masterData);
static void mtk_tv_drm_gfx_panel_unbind(
	struct device *compDev,
	struct device *master,
	void *masterData);
#ifdef CONFIG_PM
static int gfx_runtime_suspend(struct device *dev);
static int gfx_runtime_resume(struct device *dev);
static int gfx_pm_runtime_force_suspend(struct device *dev);
static int gfx_pm_runtime_force_resume(struct device *dev);
#endif

//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------
static const struct dev_pm_ops gfx_pm_ops = {
	SET_RUNTIME_PM_OPS(gfx_runtime_suspend,
			   gfx_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(gfx_pm_runtime_force_suspend,
				gfx_pm_runtime_force_resume)
};

static const struct drm_panel_funcs gfxDrmPanelFuncs = {
	.disable = mtk_tv_drm_gfx_panel_disable,
	.unprepare = mtk_tv_drm_gfx_panel_unprepare,
	.prepare = mtk_tv_drm_gfx_panel_prepare,
	.enable = mtk_tv_drm_gfx_panel_enable,
	.get_modes = mtk_tv_drm_gfx_panel_get_modes,
	.get_timings = mtk_tv_drm_gfx_panel_get_timings,
};

static const struct of_device_id mtk_tv_drm_gfx_panel_of_match_table[] = {
	{ .compatible = "mtk,mt5896-gfx-panel",},
	{/* sentinel */}
};
MODULE_DEVICE_TABLE(of, mtk_tv_drm_gfx_panel_of_match_table);

struct platform_driver mtk_tv_drm_gfx_panel_driver = {
	.probe		= mtk_tv_drm_gfx_panel_probe,
	.remove		= mtk_tv_drm_gfx_panel_remove,
	.suspend	= mtk_tv_drm_gfx_panel_suspend,
	.resume		= mtk_tv_drm_gfx_panel_resume,
	.driver = {
			.name = GFX_NODE,
			.owner  = THIS_MODULE,
			.of_match_table = of_match_ptr(mtk_tv_drm_gfx_panel_of_match_table),
			.pm = &gfx_pm_ops,
			},
};

static const struct component_ops mtk_tv_drm_gfx_panel_component_ops = {
	.bind	= mtk_tv_drm_gfx_panel_bind,
	.unbind = mtk_tv_drm_gfx_panel_unbind,
};

//-------------------------------------------------------------------------------------------------
// Function
//-------------------------------------------------------------------------------------------------
void mtk_gfx_render_output_en(struct mtk_drm_panel_context *pStPanelCtx, bool bEn)
{
	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	if (pStPanelCtx->cfg.linkIF != E_LINK_NONE) {
		//gfx
		drv_hwreg_render_pnl_set_render_output_en_gfx(bEn);
	}
}

void _vb1_gfx_4k_setting(struct mtk_drm_panel_context *pStPanelCtx)
{
	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	if (pStPanelCtx->pnlInfo.vbo_byte == E_VBO_4BYTE_MODE)
		drv_hwreg_render_pnl_set_vby1_gfx_4k_argb8888_60HZ();
	else if (pStPanelCtx->pnlInfo.vbo_byte == E_VBO_5BYTE_MODE)
		drv_hwreg_render_pnl_set_vby1_gfx_4k_argb8101010_60HZ();
	else
		DRM_INFO("[%s][%d] GFX setting Not Support\n", __func__, __LINE__);
}

static void _mtk_gfx_vby1_setting(struct mtk_drm_panel_context *pStPanelCtx)
{
	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	if (pStPanelCtx->cfg.linkIF != E_LINK_NONE) {
		switch (pStPanelCtx->cfg.timing) {

		case E_4K2K_30HZ:
			drv_hwreg_render_pnl_set_vby1_gfx_4k_30HZ();
		break;

		case E_4K2K_60HZ:
			_vb1_gfx_4k_setting(pStPanelCtx);
		break;

		default:
			DRM_INFO("[%s][%d] GFX OUTPUT TIMING Not Support\n", __func__, __LINE__);
		break;
		}
	}
}

static void _mtk_gfx_mft_hbkproch_protect_init(struct mtk_drm_panel_context *pStPanelCtx)
{
	st_drv_hprotect_info stHprotectInfoTmp = {0};

	stHprotectInfoTmp.hsync_w =  pStPanelCtx->pnlInfo.hsync_w;
	stHprotectInfoTmp.de_hstart =  pStPanelCtx->pnlInfo.de_hstart;
	stHprotectInfoTmp.lane_numbers = pStPanelCtx->cfg.lanes;
	stHprotectInfoTmp.op_format =  pStPanelCtx->pnlInfo.op_format;

	drv_hwreg_render_pnl_gfx_hbkproch_protect_init(&stHprotectInfoTmp);
}

static void _init_mod_gfx_cfg(struct mtk_drm_panel_context *pStPanelCtx)
{
	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	init_mod_cfg(&pStPanelCtx->cfg);
}

static void _dump_mod_gfx_cfg(struct mtk_drm_panel_context *pStPanelCtx)
{
	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("================GFX_MOD_CFG================\n");
	dump_mod_cfg(&pStPanelCtx->cfg);
}


bool mtk_render_gfx_cfg_connector(struct mtk_drm_panel_context *pStPanelCtx)
{
	uint8_t out_clk_if = 0;
	st_drv_out_lane_order stPnlOutLaneOrder;

	if (pStPanelCtx == NULL) {
		DRM_INFO("[%s][%d]param is NULL, FAIL\n", __func__, __LINE__);
		return false;
	}
	memset(&stPnlOutLaneOrder, 0, sizeof(st_drv_out_lane_order));
	memcpy(&stPnlOutLaneOrder, &pStPanelCtx->out_lane_info, sizeof(st_drv_out_lane_order));

	_init_mod_gfx_cfg(pStPanelCtx);
	set_out_mod_cfg(&pStPanelCtx->pnlInfo, &pStPanelCtx->cfg);
	_dump_mod_gfx_cfg(pStPanelCtx);

	if (pStPanelCtx->cfg.linkIF != E_LINK_NONE) {
		if (pStPanelCtx->cfg.timing == E_4K2K_60HZ)
			out_clk_if = 8;
		else if (pStPanelCtx->cfg.timing == E_4K2K_30HZ)
			out_clk_if = 4;
		else
			out_clk_if = 0;

		if (pStPanelCtx->out_upd_type != E_NO_UPDATE_TYPE) {
			drv_hwreg_render_pnl_set_cfg_ckg_gfx(
					MOD_IN_IF_GFX,
					out_clk_if,
					(en_drv_pnl_output_mode)pStPanelCtx->cfg.timing,
					(en_drv_pnl_output_format)pStPanelCtx->cfg.format);
		}
	}

	if (pStPanelCtx->out_upd_type != E_NO_UPDATE_TYPE) {
		drv_hwreg_render_pnl_gfx_analog_setting(
			(en_drv_pnl_link_if)pStPanelCtx->cfg.linkIF,
			(en_drv_pnl_output_mode)pStPanelCtx->cfg.timing,
			&stPnlOutLaneOrder);
		_mtk_gfx_vby1_setting(pStPanelCtx);
		_mtk_gfx_mft_hbkproch_protect_init(pStPanelCtx);
		mtk_gfx_render_output_en(pStPanelCtx, true);
	}

	return true;
}
EXPORT_SYMBOL(mtk_render_gfx_cfg_connector);

static void dump_gfx_panel_dts_info(struct mtk_drm_panel_context *pStPanelCtx)
{
	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return;
	}

	DRM_INFO("================GFX Path================\n");
	dump_panel_info(&pStPanelCtx->pnlInfo);
	DRM_INFO("================================================\n");
}


int readDTB2GfxPanelPrivate(struct mtk_drm_panel_context *pStPanelCtx)
{
	int ret = 0;

	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	ret = _parse_panel_info(pStPanelCtx->dev->of_node,
			&pStPanelCtx->pnlInfo,
			&pStPanelCtx->panelInfoList);

	return ret;
}
EXPORT_SYMBOL(readDTB2GfxPanelPrivate);

static int mtk_gfx_panel_init(struct device *dev)
{
	int ret = 0;
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}
	pStPanelCtx = dev->driver_data;

	if (!pStPanelCtx) {
		DRM_ERROR("[%s, %d]: Panel context is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	//External BE related
	mtk_render_gfx_cfg_connector(pStPanelCtx);

	DRM_INFO("[%s][%d]gfx panel success!!\n", __func__, __LINE__);

	return ret;
}

static int mtk_tv_drm_gfx_panel_disable(struct drm_panel *panel)
{
	return 0;
}

static int mtk_tv_drm_gfx_panel_unprepare(struct drm_panel *panel)
{
	return 0;
}

static int mtk_tv_drm_gfx_panel_prepare(struct drm_panel *panel)
{
	struct mtk_drm_panel_context *pStPanelCtx = NULL;
	struct drm_crtc_state *drmCrtcState = NULL;
	int ret = 0;

	if (!panel) {
		DRM_ERROR("[%s, %d]: panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	drmCrtcState = panel->connector->state->crtc->state;

	pStPanelCtx = container_of(panel, struct mtk_drm_panel_context, drmPanel);

	if (drmCrtcState->mode_changed
		&& !(_isSameModeAndPanelInfo(&drmCrtcState->mode, &pStPanelCtx->pnlInfo))) {
		ret = _copyPanelInfo(drmCrtcState,
				&pStPanelCtx->pnlInfo,
				&pStPanelCtx->panelInfoList);

		if (ret) {
			DRM_ERROR("[%s, %d]: new panel mode does not set.\n",
				__func__, __LINE__);
		} else {
			pStPanelCtx->out_upd_type = E_MODE_RESET_TYPE;
			mtk_gfx_panel_init(panel->dev);
			mtk_gop_set_tgen(pStPanelCtx);
		}
	}
	return 0;
}

static int mtk_tv_drm_gfx_panel_enable(struct drm_panel *panel)
{
	return 0;
}

static int mtk_tv_drm_gfx_panel_get_modes(struct drm_panel *panel)
{
	struct mtk_drm_panel_context *pStPanelCtx = NULL;
	int modeCouct = 0;

	if (!panel) {
		DRM_ERROR("[%s, %d]: drm_panel is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	pStPanelCtx = container_of(panel, struct mtk_drm_panel_context, drmPanel);

	_panelAddModes(panel,
		&pStPanelCtx->panelInfoList,
		&modeCouct);

	pStPanelCtx->u8TimingsNum = modeCouct;

	return pStPanelCtx->u8TimingsNum;
}

static int mtk_tv_drm_gfx_panel_get_timings(struct drm_panel *panel,
							unsigned int num_timings,
							struct display_timing *timings)
{
	uint32_t u32NumTiming = 1;
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

	pStPanelCtx = container_of(panel, struct mtk_drm_panel_context, drmPanel);

	if (!timings) {
		DRM_WARN("\033[1;31m [%s][%d]pointer is NULL \033[0m\n",
							__func__, __LINE__);
		return u32NumTiming;
	}
	timings[0] = pStPanelCtx->pnlInfo.displayTiming;

	return u32NumTiming;
}

static int mtk_tv_drm_gfx_panel_bind(
	struct device *compDev,
	struct device *master,
	void *masterData)
{
	if (!compDev) {
		DRM_ERROR("[%s, %d]: compDev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	mtk_gfx_panel_init(compDev);
	return 0;
}

static void mtk_tv_drm_gfx_panel_unbind(
	struct device *compDev,
	struct device *master,
	void *masterData)
{

}

static int mtk_tv_drm_gfx_panel_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mtk_drm_panel_context *pStPanelCtx = NULL;
	const struct of_device_id *deviceId = NULL;

	if (!pdev) {
		DRM_ERROR("[%s, %d]: platform_device is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	deviceId = of_match_node(mtk_tv_drm_gfx_panel_of_match_table, pdev->dev.of_node);

	if (!deviceId) {
		DRM_WARN("of_match_node failed\n");
		return -EINVAL;
	}

	pStPanelCtx = devm_kzalloc(&pdev->dev, sizeof(struct mtk_drm_panel_context), GFP_KERNEL);
	INIT_LIST_HEAD(&pStPanelCtx->panelInfoList);
	pStPanelCtx->dev = &pdev->dev;
	dev_set_drvdata(pStPanelCtx->dev, pStPanelCtx);

	drm_panel_init(&pStPanelCtx->drmPanel);
	pStPanelCtx->drmPanel.dev = pStPanelCtx->dev;
	pStPanelCtx->drmPanel.funcs = &gfxDrmPanelFuncs;
	ret = drm_panel_add(&pStPanelCtx->drmPanel);
	ret = component_add(&pdev->dev, &mtk_tv_drm_gfx_panel_component_ops);

	if (ret) {
		DRM_ERROR("[%s, %d]: component_add fail\n",
			__func__, __LINE__);
	}

	/*read panel related data from dtb*/
	ret = readDTB2GfxPanelPrivate(pStPanelCtx);
	if (ret != 0x0) {
		DRM_INFO("[%s, %d]: readDTB2PanelPrivate  failed\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	return ret;
}

static int mtk_tv_drm_gfx_panel_remove(struct platform_device *pdev)
{
	struct mtk_drm_panel_context *pStPanelCtx = NULL;
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

	component_del(&pdev->dev, &mtk_tv_drm_gfx_panel_component_ops);

	return 0;
}

static int mtk_tv_drm_gfx_panel_suspend(struct platform_device *pdev, pm_message_t state)
{
	panel_common_disable();

	return 0;
}

static int mtk_tv_drm_gfx_panel_resume(struct platform_device *pdev)
{
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

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
	mtk_gfx_panel_init(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM
static int gfx_runtime_suspend(struct device *dev)
{
	panel_common_disable();

	return 0;
}

static int gfx_runtime_resume(struct device *dev)
{
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

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

	pStPanelCtx->out_upd_type = E_MODE_RESET_TYPE;
	panel_common_enable();
	mtk_gfx_panel_init(dev);

	return 0;
}

static int gfx_pm_runtime_force_suspend(struct device *dev)
{
	if (!dev) {
		DRM_ERROR("[%s, %d]: dev is NULL.\n",
			__func__, __LINE__);
		return -ENODEV;
	}

	panel_common_disable();

	return pm_runtime_force_suspend(dev);
}
static int gfx_pm_runtime_force_resume(struct device *dev)
{
	struct mtk_drm_panel_context *pStPanelCtx = NULL;

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

	pStPanelCtx->out_upd_type = E_MODE_RESET_TYPE;
	panel_common_enable();
	mtk_gfx_panel_init(dev);

	return pm_runtime_force_resume(dev);
}
#endif


MODULE_AUTHOR("Mediatek TV group");
MODULE_DESCRIPTION("Mediatek SoC DRM driver");
MODULE_LICENSE("GPL v2");
