// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "mtk_tv_drm_video_panel.h"
#include "../mtk_tv_lpll_tbl.h"

static void _mtk_cfg_ckg_common(struct mtk_panel_context *pctx)
{
	if (pctx == NULL)
		return;

	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		//clock setting
		W2BYTEMSK(reg_ckg_lpll_syn_432, 0x0, 0x7);
		W2BYTEMSK(reg_ckg_lpll_syn_864, 0x0, 0x3);
		W2BYTEMSK(reg_ckg_xtal_24m_mod, 0x0, 0x3);
		W2BYTEMSK(reg_ckg_mod_d_odclk, 0x4, 0x3F);
	}

}

static void _ckg_sw_en_video(drm_mod_cfg *pcfg)
{
	if (pcfg == NULL)
		return;

	//general part
	//clock sw_en
	W2BYTEMSK(reg_sw_en_lpll_sync_4322fpll, 0x1, 0x1);
	W2BYTEMSK(reg_sw_en_lpll_sync_8642fpll, 0x1, 0x1);
	W2BYTEMSK(reg_sw_en_xtal_24m2fpll, 0x1, 0x1);
	W2BYTEMSK(reg_sw_en_lpll_syn_864_pipe2fpll, 0x1, 0x1);
	W2BYTEMSK(reg_sw_en_mod_d_odclk2mod_d, 0x1, 0x1);

	if (pcfg->timing == E_FHD_60HZ) {

		if (pcfg->linkIF == E_LINK_LVDS) {

			W2BYTEMSK(reg_sw_en_mod_d_odclk2lvds, 0x2, 0x2);

			W2BYTEMSK(reg_sw_en_v1_odclk2v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg12v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg22v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg32v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg42v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg52v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg62v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg72v1, 0x1, 0x1);

			W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2v1, 0x1, 0x1);

			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk2v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre02v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre12v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_final2v1, 0x1, 0x1);
		}

		if (pcfg->linkIF == E_LINK_VB1) {
			W2BYTEMSK(reg_sw_en_v1_odclk2v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg12v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg22v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg32v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg42v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg52v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg62v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg72v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg82v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_odclk_stg92v1, 0x1, 0x1);

			W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2vby1_v1, 0x2, 0x2);
			W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2v1, 0x1, 0x1);

			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk2v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre02v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre12v1, 0x1, 0x1);
			W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_final2v1, 0x1, 0x1);

		}

	} else if (pcfg->timing == E_FHD_120HZ) {
		W2BYTEMSK(reg_sw_en_v1_odclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg22v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg32v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg42v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg52v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg62v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg72v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg82v1, 0x0, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg92v1, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2vby1_v1, 0x2, 0x2);
		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2v1, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre02v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_final2v1, 0x1, 0x1);

	} else if (pcfg->timing == E_4K2K_60HZ) {
		W2BYTEMSK(reg_sw_en_v1_odclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg22v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg32v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg42v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg52v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg62v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg72v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg82v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg92v1, 0x1, 0x1);
		//sram share
		W2BYTEMSK(reg_sw_en_v2_odclk_stg32v2, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2vby1_v1, 0x2, 0x2);
		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_mod_v2_sr_wclk2v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre02v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_final2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk2v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_pre02v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_pre12v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_final2v2, 0x1, 0x1);

	} else if (pcfg->timing == E_4K2K_120HZ) {
		W2BYTEMSK(reg_sw_en_v1_odclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg22v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg32v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg42v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg52v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg62v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg72v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg82v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg92v1, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2vby1_v1, 0x2, 0x2);
		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2v1, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre02v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_final2v1, 0x1, 0x1);

	} else if (pcfg->timing == E_8K4K_60HZ && pcfg->format == E_OUTPUT_YUV422) {
		W2BYTEMSK(reg_sw_en_v1_odclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg22v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg32v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg42v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg52v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg62v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg72v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg82v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg92v1, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2vby1_v1, 0x2, 0x2);
		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2v1, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre02v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_final2v1, 0x1, 0x1);

	} else if (pcfg->timing == E_8K4K_60HZ) {
		W2BYTEMSK(reg_sw_en_v1_odclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg22v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg32v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg42v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg52v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg62v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg72v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg82v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_odclk_stg92v1, 0x1, 0x1);
		//sram share
		W2BYTEMSK(reg_sw_en_v2_odclk_stg32v2, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2vby1_v1, 0x2, 0x2);
		W2BYTEMSK(reg_sw_en_mod_v1_sr_wclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_mod_v2_sr_wclk2v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre02v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_pre12v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v1_mod_sr_rclk_final2v1, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk2v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_pre02v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_pre12v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_final2v2, 0x1, 0x1);

	} else {
		DRM_INFO("[%s][%d]Not SUPPORT OUTPUT Timing\n", __func__, __LINE__);
	}
}

static void _ckg_sw_en_deltav(drm_mod_cfg *pcfg)
{
	if (pcfg == NULL)
		return;

	if (pcfg->timing == E_4K2K_60HZ || pcfg->timing == E_4K2K_120HZ) {
		W2BYTEMSK(reg_sw_en_v2_odclk2v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_odclk_stg12v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_odclk_stg22v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_odclk_stg32v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_odclk_stg42v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_odclk_stg52v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_odclk_stg62v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_odclk_stg72v2, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_mod_v2_sr_wclk2vby1_v2, 0x2, 0x2);
		W2BYTEMSK(reg_sw_en_mod_v2_sr_wclk2v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk2v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_pre02v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_pre12v2, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_v2_mod_sr_rclk_final2v2, 0x1, 0x1);

	} else {
		DRM_INFO("[%s][%d]Not SUPPORT OUTPUT Timing\n", __func__, __LINE__);
	}


}

static void _ckg_sw_en_gfx(drm_mod_cfg *pcfg)
{
	if (pcfg == NULL)
		return;

	if (pcfg->timing == E_4K2K_30HZ ||
	    (pcfg->timing == E_4K2K_60HZ && pcfg->format == E_OUTPUT_RGBA8888) ||
	    (pcfg->timing == E_4K2K_60HZ && pcfg->format == E_OUTPUT_RGBA1010108)) {
		W2BYTEMSK(reg_sw_en_o_odclk2o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_o_odclk_stg12o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_o_odclk_stg22o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_o_odclk_stg32o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_o_odclk_stg42o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_o_odclk_stg52o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_o_odclk_stg62o, 0x1, 0x1);

		W2BYTEMSK(reg_sw_en_mod_o_sr_wclk2vby1_o, 0x2, 0x2);
		W2BYTEMSK(reg_sw_en_mod_o_sr_wclk2o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_mod_o_sr_rclk2o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_mod_o_sr_rclk_pre02o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_mod_o_sr_rclk_pre12o, 0x1, 0x1);
		W2BYTEMSK(reg_sw_en_mod_o_sr_rclk_final2o, 0x1, 0x1);

	} else {
		DRM_INFO("[%s][%d]Not SUPPORT OUTPUT Timing\n", __func__, __LINE__);
	}
}

static void _mtk_cfg_ckg_video(drm_mod_cfg cfg, uint8_t in, uint8_t out)
{
	if (in == 0 || out == 0)
		return;

	// MOD input is 4p
	if (in == 4) {
		switch (out) {
		case 2:
			if (cfg.linkIF == E_LINK_LVDS) {
				W2BYTEMSK(reg_ckg_v1_odclk, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg1, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg2, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg3, 0x2C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg4, 0x2C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg5, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg6, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg7, 0x1C, 0x3F);

				//serial top wclk
				W2BYTEMSK(reg_ckg_mod_v1_sr_wclk, 0xC, 0x1F);
				//serial top rclk
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre0, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre1, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_final, 0x0, 0x7);

			}
			if (cfg.linkIF == E_LINK_VB1) {
				W2BYTEMSK(reg_ckg_v1_odclk, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg1, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg2, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg3, 0x2C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg4, 0x2C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg5, 0x24, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg6, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg7, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg8, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg9, 0x1C, 0x3F);

				//serial top wclk
				W2BYTEMSK(reg_ckg_mod_v1_sr_wclk, 0xC, 0x1F);
				//serial top rclk
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre0, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre1, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_final, 0x0, 0x7);

			}
			break;
		case 4:
			W2BYTEMSK(reg_ckg_v1_odclk, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg1, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg2, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg3, 0x24, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg4, 0x24, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg5, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg6, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg7, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg8, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg9, 0x1C, 0x3F);

			//serial top wclk
			W2BYTEMSK(reg_ckg_mod_v1_sr_wclk, 0xC, 0x1F);
			//serial top rclk
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre0, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre1, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_final, 0x0, 0x7);
			break;
		case 8:
			W2BYTEMSK(reg_ckg_v1_odclk, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg1, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg2, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg3, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg4, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg5, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg6, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg7, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg8, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg9, 0x1C, 0x3F);

			//serial top wclk
			W2BYTEMSK(reg_ckg_mod_v1_sr_wclk, 0xC, 0x1F);
			//serial top rclk
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre0, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre1, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_final, 0x0, 0x7);

			break;
		case 16:
			if (cfg.timing == E_4K2K_120HZ) {
				W2BYTEMSK(reg_ckg_v1_odclk, 0x0C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg1, 0x0C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg2, 0x0C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg3, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg4, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg5, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg6, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg7, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg8, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg9, 0x1C, 0x3F);

				//serial top wclk
				W2BYTEMSK(reg_ckg_mod_v1_sr_wclk, 0xC, 0x1F);
				//serial top rclk
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre0, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre1, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_final, 0x0, 0x7);


			}
			if (cfg.timing == E_8K4K_120HZ && cfg.format == E_OUTPUT_YUV422) {
				W2BYTEMSK(reg_ckg_v1_odclk, 0x4, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg1, 0x4, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg2, 0xC, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg3, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg4, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg5, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg6, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg7, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg8, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_v1_odclk_stg9, 0x1C, 0x3F);

				//serial top wclk
				W2BYTEMSK(reg_ckg_mod_v1_sr_wclk, 0xC, 0x1F);
				//serial top rclk
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk, 0x4, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre0, 0x4, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre1, 0x4, 0x7);
				W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_final, 0x4, 0x7);

			}
			break;
		case 32:
			//clock setting
			W2BYTEMSK(reg_ckg_v1_odclk, 0x4, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg1, 0x4, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg2, 0x4, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg3, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg4, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg5, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg6, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg7, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg8, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v1_odclk_stg9, 0x1C, 0x3F);

			//SRAM share
			W2BYTEMSK(reg_ckg_v2_odclk_stg3, 0xC, 0x3F);

			//serial top wclk
			W2BYTEMSK(reg_ckg_mod_v1_sr_wclk, 0xC, 0x1F);
			W2BYTEMSK(reg_ckg_mod_v2_sr_wclk, 0xC, 0x1F);
			//serial top rclk
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre0, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_pre1, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v1_mod_sr_rclk_final, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_pre0, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_pre1, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_final, 0x0, 0x7);

			break;
		default:
			break;
		}

		_ckg_sw_en_video(&cfg);
	}


}

static void _mtk_cfg_ckg_deltav(drm_mod_cfg cfg, uint8_t in, uint8_t out)
{
	// ToDo: Add pctx->deltav_cfg as param.

	if (in == 0 || out == 0)
		return;

	if (in == 1) {
		switch (out) {
		case 16:	// 4K 120HZ case output
			W2BYTEMSK(reg_ckg_v2_odclk, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg1, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg2, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg3, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg4, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg5, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg6, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg7, 0x14, 0x3F);

			W2BYTEMSK(reg_ckg_mod_v2_sr_wclk, 0xC, 0x1F);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_pre0, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_pre1, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_final, 0x0, 0x7);
			break;
		case 8:	// 4K 60hz case
			W2BYTEMSK(reg_ckg_v2_odclk, 0x4, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg1, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg2, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg3, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg4, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg5, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg6, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_v2_odclk_stg7, 0x1C, 0x3F);

			W2BYTEMSK(reg_ckg_mod_v2_sr_wclk, 0xC, 0x1F);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_pre0, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_pre1, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_v2_mod_sr_rclk_final, 0x0, 0x7);
			break;
		default:
			break;
		}
		_ckg_sw_en_deltav(&cfg);
	}
}

static void _mtk_cfg_ckg_gfx(drm_mod_cfg cfg, uint8_t in, uint8_t out)
{
	if (in == 0 || out == 0)
		return;

	if (in == 1) {
		switch (out) {
		case 8:
			if (cfg.format == E_OUTPUT_RGBA8888) {
				W2BYTEMSK(reg_ckg_o_odclk, 0x4, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg1, 0xC, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg2, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg3, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg4, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg5, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg6, 0x1C, 0x3F);

				W2BYTEMSK(reg_ckg_mod_o_sr_wclk, 0xC, 0x1F);
				W2BYTEMSK(reg_ckg_mod_o_sr_rclk, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_mod_o_sr_rclk_pre0, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_mod_o_sr_rclk_pre1, 0x0, 0x7);
				W2BYTEMSK(reg_ckg_mod_o_sr_rclk_final, 0x0, 0x7);

			} else if (cfg.format == E_OUTPUT_RGBA1010108) {

				W2BYTEMSK(reg_ckg_o_odclk, 0x4, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg1, 0xC, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg2, 0x14, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg3, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg4, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg5, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_o_odclk_stg6, 0x1C, 0x3F);
				W2BYTEMSK(reg_ckg_mod_o_sr_wclk, 0xC, 0x1F);

				W2BYTEMSK(reg_ckg_mod_o_sr_rclk, 0x4, 0x7);
				W2BYTEMSK(reg_ckg_mod_o_sr_rclk_pre0, 0x4, 0x7);
				W2BYTEMSK(reg_ckg_mod_o_sr_rclk_pre1, 0x4, 0x7);
				W2BYTEMSK(reg_ckg_mod_o_sr_rclk_final, 0x4, 0x7);
			}
			break;
		case 4:
			W2BYTEMSK(reg_ckg_o_odclk, 0xC, 0x3F);
			W2BYTEMSK(reg_ckg_o_odclk_stg1, 0x14, 0x3F);
			W2BYTEMSK(reg_ckg_o_odclk_stg2, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_o_odclk_stg3, 0x24, 0x3F);
			W2BYTEMSK(reg_ckg_o_odclk_stg4, 0x24, 0x3F);
			W2BYTEMSK(reg_ckg_o_odclk_stg5, 0x1C, 0x3F);
			W2BYTEMSK(reg_ckg_o_odclk_stg6, 0x1C, 0x3F);

			W2BYTEMSK(reg_ckg_mod_o_sr_wclk, 0xC, 0x1F);
			W2BYTEMSK(reg_ckg_mod_o_sr_rclk, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_mod_o_sr_rclk_pre0, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_mod_o_sr_rclk_pre1, 0x0, 0x7);
			W2BYTEMSK(reg_ckg_mod_o_sr_rclk_final, 0x0, 0x7);

			break;
		default:
			break;
		}
		_ckg_sw_en_gfx(&cfg);
	}

}

static void _vby1_v_fhd_60HZ(void)
{
	W2BYTEMSK(reg_mod_2pto4p_en, 0x0, 0x2);
	W2BYTEMSK(reg_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_yuv_422_mode, 0x0, 0x8000);
	W2BYTEMSK(reg_mft_mode, 0x1, 0xF);
	W2BYTEMSK(reg_div_pix_mode, 0x200, 0x300);
	W2BYTEMSK(reg_hsize, 0x780, 0x3FFF);
	W2BYTEMSK(reg_div_len, 0x780, 0x3FFF);
	W2BYTEMSK(reg_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_vby1_8pto16p_en, 0x0, 0x1);
	W2BYTEMSK(reg_vby1_16pto32p_en, 0x0, 0x2);
	W2BYTEMSK(reg_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_format_map, 0x0, 0x1F);
	W2BYTEMSK(reg_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_fotmat_v1, 0x1, 0xF);
	W2BYTEMSK(reg_gcr_gank_clk_src_sel, 0x0, 0x1);

	W2BYTEMSK(reg_mft_read_hde_st, 0x10, 0xFFFF);
	W2BYTEMSK(reg_mft_vld_wrap_fix_htt, 0x0, 0x3F00);
}

static void _vby1_v_fhd_120HZ(void)
{
	W2BYTEMSK(reg_mod_2pto4p_en, 0x0, 0x2);
	W2BYTEMSK(reg_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_yuv_422_mode, 0x0, 0x8000);
	W2BYTEMSK(reg_mft_mode, 0x1, 0xF);
	W2BYTEMSK(reg_div_pix_mode, 0x100, 0x300);
	W2BYTEMSK(reg_hsize, 0x780, 0x3FFF);
	W2BYTEMSK(reg_div_len, 0x780, 0x3FFF);
	W2BYTEMSK(reg_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_vby1_8pto16p_en, 0x0, 0x1);
	W2BYTEMSK(reg_vby1_16pto32p_en, 0x0, 0x2);
	W2BYTEMSK(reg_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_format_map, 0x0, 0x1F);
	W2BYTEMSK(reg_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_fotmat_v1, 0x1, 0xF);
	W2BYTEMSK(reg_gcr_gank_clk_src_sel, 0x0, 0x1);

	W2BYTEMSK(reg_mft_read_hde_st, 0x10, 0xFFFF);
	W2BYTEMSK(reg_mft_vld_wrap_fix_htt, 0x0, 0x3F00);
}

static void _vby1_v_4k_60HZ(void)
{
	W2BYTEMSK(reg_mod_2pto4p_en, 0x0, 0x2);
	W2BYTEMSK(reg_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_yuv_422_mode, 0x0, 0x8000);
	W2BYTEMSK(reg_mft_mode, 0x1, 0xF);
	W2BYTEMSK(reg_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_hsize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_div_len, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_vby1_8pto16p_en, 0x0, 0x1);
	W2BYTEMSK(reg_vby1_16pto32p_en, 0x0, 0x2);
	W2BYTEMSK(reg_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_format_map, 0x0, 0x1F);
	W2BYTEMSK(reg_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_fotmat_v1, 0x1, 0xF);
	W2BYTEMSK(reg_gcr_gank_clk_src_sel, 0x0, 0x1);

	W2BYTEMSK(reg_mft_read_hde_st, 0x22, 0xFFFF);
	W2BYTEMSK(reg_mft_vld_wrap_fix_htt, 0x0, 0x3F00);
}

static void _vby1_v_4k_120HZ(void)
{
	W2BYTEMSK(reg_mod_2pto4p_en, 0x0, 0x2);
	W2BYTEMSK(reg_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_yuv_422_mode, 0x0, 0x8000);
	W2BYTEMSK(reg_mft_mode, 0x1, 0xF);	//TODO
	W2BYTEMSK(reg_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_hsize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_div_len, 0xF00, 0x3FFF);	//TODO
	W2BYTEMSK(reg_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_base1_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_vby1_8pto16p_en, 0x1, 0x1);
	W2BYTEMSK(reg_vby1_16pto32p_en, 0x0, 0x2);
	W2BYTEMSK(reg_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_format_map, 0x0, 0x1F);
	W2BYTEMSK(reg_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_fotmat_v1, 0x1, 0xF);
	W2BYTEMSK(reg_gcr_gank_clk_src_sel, 0x0, 0x1);

	W2BYTEMSK(reg_mft_read_hde_st, 0x10, 0xFFFF);
	W2BYTEMSK(reg_mft_vld_wrap_fix_htt, 0x100, 0x3F00);
}

static void _vby1_v_8k422_60HZ(void)
{
	W2BYTEMSK(reg_mod_2pto4p_en, 0x2, 0x2);
	W2BYTEMSK(reg_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_yuv_422_mode, 0x8000, 0x8000);
	W2BYTEMSK(reg_mft_mode, 0x1, 0xF);	//TODO
	W2BYTEMSK(reg_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_hsize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_div_len, 0xF00, 0x3FFF);	//TODO
	W2BYTEMSK(reg_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_base1_addr, 0x0, 0x3FFF); //TODO
	W2BYTEMSK(reg_vby1_8pto16p_en, 0x1, 0x1);
	W2BYTEMSK(reg_vby1_16pto32p_en, 0x0, 0x2);
	W2BYTEMSK(reg_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_format_map, 0x1, 0x1F);
	W2BYTEMSK(reg_vby1_byte_mode, 0x1000, 0x3000);
	W2BYTEMSK(reg_seri_fotmat_v1, 0x3, 0xF);
	W2BYTEMSK(reg_gcr_gank_clk_src_sel, 0x1, 0x1);

	W2BYTEMSK(reg_mft_read_hde_st, 0x10, 0xFFFF);
	W2BYTEMSK(reg_mft_vld_wrap_fix_htt, 0x100, 0x3F00);

}

static void _vby1_v_8k_60HZ(void)
{
	W2BYTEMSK(reg_mod_2pto4p_en, 0x0, 0x2);
	W2BYTEMSK(reg_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_yuv_422_mode, 0x0, 0x8000);
	W2BYTEMSK(reg_mft_mode, 0x1, 0xF);	//TODO
	W2BYTEMSK(reg_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_hsize, 0x1E00, 0x3FFF);
	W2BYTEMSK(reg_div_len, 0x1E00, 0x3FFF); //TODO
	W2BYTEMSK(reg_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_base1_addr, 0x0, 0x3FFF); //TODO
	W2BYTEMSK(reg_base2_addr, 0x0, 0x3FFF); //TODO
	W2BYTEMSK(reg_base3_addr, 0x0, 0x3FFF); //TODO
	W2BYTEMSK(reg_vby1_8pto16p_en, 0x1, 0x1);
	W2BYTEMSK(reg_vby1_16pto32p_en, 0x2, 0x2);
	W2BYTEMSK(reg_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_format_map, 0x0, 0x1F);
	W2BYTEMSK(reg_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_fotmat_v1, 0x1, 0xF);
	W2BYTEMSK(reg_gcr_gank_clk_src_sel, 0x0, 0x1);
	W2BYTEMSK(reg_sram_ext_en, 0x80, 0x80);

	W2BYTEMSK(reg_mft_read_hde_st, 0x10, 0xFFFF);
	W2BYTEMSK(reg_mft_vld_wrap_fix_htt, 0x300, 0x3F00);

}

static void _vby1_deltav_4k_60HZ(void)
{
	W2BYTEMSK(reg_v2_sel, 0x0, 0x8000);
	W2BYTEMSK(reg_v2_mod_1pto2p_en, 0x1, 0x1);
	W2BYTEMSK(reg_v2_mod_2pto4p_en, 0x2, 0x2);
	W2BYTEMSK(reg_v2_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_v2_mft_mode, 0x1, 0xF);
	W2BYTEMSK(reg_v2_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_v2_hsize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_v2_div_len, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_v2_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_v2_vby1_8pto16p_en, 0x0, 0x1);
	W2BYTEMSK(reg_v2_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_v2_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_v2_format_map, 0x0, 0x1F);
	W2BYTEMSK(reg_v2_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_format_v2, 0x10, 0xF0);
	W2BYTEMSK(reg_seri_v2_mode, 0x1, 0x1);
	W2BYTEMSK(reg_v2_gcr_bank_clk_src_sel, 0x0, 0x2);

	W2BYTEMSK(reg_v2_mft_read_hde_st, 0x22, 0xFFFF);
}

static void _vby1_deltav_4k_120HZ(void)
{
	W2BYTEMSK(reg_v2_sel, 0x8000, 0x8000);
	W2BYTEMSK(reg_v2_mod_1pto2p_en, 0x0, 0x1);
	W2BYTEMSK(reg_v2_mod_2pto4p_en, 0x0, 0x2);
	W2BYTEMSK(reg_v2_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_v2_mft_mode, 0x1, 0xF);	//TODO
	W2BYTEMSK(reg_v2_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_v2_hsize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_v2_div_len, 0xF00, 0x3FFF);	//TODO
	W2BYTEMSK(reg_v2_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_v2_base1_addr, 0x0, 0x3FFF);	//TODO
	W2BYTEMSK(reg_v2_vby1_8pto16p_en, 0x1, 0x1);
	W2BYTEMSK(reg_v2_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_v2_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_v2_format_map, 0x0, 0x1F);
	W2BYTEMSK(reg_v2_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_format_v2, 0x10, 0xF0);
	W2BYTEMSK(reg_seri_v2_mode, 0x1, 0x1);
	W2BYTEMSK(reg_v2_gcr_bank_clk_src_sel, 0x0, 0x2);

	W2BYTEMSK(reg_v2_mft_read_hde_st, 0x10, 0xFFFF);
}

static void _vby1_gfx_4k_30HZ(void)
{
	W2BYTEMSK(reg_o_mod_1pto2p_en, 0x1, 0x1);
	W2BYTEMSK(reg_o_mod_2pto4p_en, 0x2, 0x2);
	W2BYTEMSK(reg_o_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_o_mft_mode, 0x1, 0xF);
	W2BYTEMSK(reg_o_div_pix_mode, 0x1, 0x300);
	W2BYTEMSK(reg_o_hize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_o_div_len, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_o_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_o_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_o_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_o_format_map, 0x2, 0x1F);
	W2BYTEMSK(reg_o_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_format_osd, 0x100, 0xF00);
	W2BYTEMSK(reg_seri_osd_mode, 0x2, 0x2);
	W2BYTEMSK(reg_o_gcr_bank_clk_src_sel, 0x0, 0x4);

	W2BYTEMSK(reg_o_mft_read_hde_st, 0x22, 0xFFFF);
}

static void _vby1_gfx_4k_argb8888_60HZ(void)
{
	W2BYTEMSK(reg_o_mod_1pto2p_en, 0x1, 0x1);
	W2BYTEMSK(reg_o_mod_2pto4p_en, 0x2, 0x2);
	W2BYTEMSK(reg_o_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_o_mft_mode, 0x1, 0xF);
	W2BYTEMSK(reg_o_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_o_hize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_o_div_len, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_o_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_o_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_o_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_o_format_map, 0x2, 0x1F);
	W2BYTEMSK(reg_o_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_format_osd, 0x100, 0xF00);
	W2BYTEMSK(reg_seri_osd_mode, 0x2, 0x2);
	W2BYTEMSK(reg_o_gcr_bank_clk_src_sel, 0x0, 0x4);

	W2BYTEMSK(reg_o_mft_read_hde_st, 0x22, 0xFFFF);

}

static void _vby1_gfx_4k_argb8101010_60HZ(void)
{
	W2BYTEMSK(reg_o_mod_1pto2p_en, 0x1, 0x1);
	W2BYTEMSK(reg_o_mod_2pto4p_en, 0x2, 0x2);
	W2BYTEMSK(reg_o_mod_4pto8p_en, 0x4, 0x4);
	W2BYTEMSK(reg_o_mft_mode, 0x1, 0xF);
	W2BYTEMSK(reg_o_div_pix_mode, 0x0, 0x300);
	W2BYTEMSK(reg_o_hize, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_o_div_len, 0xF00, 0x3FFF);
	W2BYTEMSK(reg_o_base0_addr, 0x0, 0x3FFF);
	W2BYTEMSK(reg_o_vby1_hs_inv, 0x10, 0x10);
	W2BYTEMSK(reg_o_vby1_vs_inv, 0x40, 0x40);
	W2BYTEMSK(reg_o_format_map, 0x2, 0x1F);
	W2BYTEMSK(reg_o_vby1_byte_mode, 0x2000, 0x3000);
	W2BYTEMSK(reg_seri_format_osd, 0x300, 0xF00);
	W2BYTEMSK(reg_seri_osd_mode, 0x2, 0x2);
	W2BYTEMSK(reg_o_gcr_bank_clk_src_sel, 0x4, 0x4);

	W2BYTEMSK(reg_o_mft_read_hde_st, 0x22, 0xFFFF);
}


void _vb1_v_8k_setting(struct mtk_panel_context *pctx)
{
	if (pctx->v_cfg.format == E_OUTPUT_YUV422)
		_vby1_v_8k422_60HZ();
	else
		_vby1_v_8k_60HZ();
}

void _vb1_deltav_4k_setting(struct mtk_panel_context *pctx)
{
	if (pctx->deltav_cfg.timing == E_4K2K_60HZ)
		_vby1_deltav_4k_60HZ();
	else if (pctx->deltav_cfg.timing == E_4K2K_120HZ)
		_vby1_deltav_4k_120HZ();
	else
		DRM_INFO("[%s][%d] DeltaV OUTPUT TIMING Not Support\n", __func__, __LINE__);
}

void _vb1_gfx_4k_setting(struct mtk_panel_context *pctx)
{
	if (pctx->gfx_cfg.format == E_OUTPUT_RGBA8888)
		_vby1_gfx_4k_argb8888_60HZ();
	if (pctx->gfx_cfg.format == E_OUTPUT_RGBA1010108)
		_vby1_gfx_4k_argb8101010_60HZ();
}

void _vb1_seri_data_sel(struct mtk_panel_context *pctx)
{
	if (pctx->v_cfg.lanes == 32 && pctx->gfx_cfg.lanes == 8)
		W2BYTEMSK(reg_datax_sel, 0x1, 0xF);

	if (pctx->v_cfg.lanes == 16 &&
		pctx->deltav_cfg.lanes == 16 &&
		pctx->gfx_cfg.lanes == 8) {
		W2BYTEMSK(reg_datax_sel, 0x2, 0xF);
	}

	if (pctx->v_cfg.lanes == 64)
		W2BYTEMSK(reg_datax_sel, 0x3, 0xF);
}

static void _mtk_vby1_setting(struct mtk_panel_context *pctx)
{
	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		switch (pctx->v_cfg.timing) {

		case E_FHD_60HZ:
			_vby1_v_fhd_60HZ();
		break;
		case E_FHD_120HZ:
			_vby1_v_fhd_120HZ();
		break;
		case E_4K2K_60HZ:
			_vby1_v_4k_60HZ();
		break;
		case E_4K2K_120HZ:
			_vby1_v_4k_120HZ();
		break;
		case E_8K4K_60HZ:
			_vb1_v_8k_setting(pctx);
		break;
		default:
			DRM_INFO("[%s][%d] Video OUTPUT TIMING Not Support\n", __func__, __LINE__);
		break;
		}

		//ToDo:Add Fixed H-backporch code.
	}

	if (pctx->deltav_cfg.linkIF != E_LINK_NONE)
		_vb1_deltav_4k_setting(pctx);

	if (pctx->gfx_cfg.linkIF != E_LINK_NONE) {
		switch (pctx->gfx_cfg.timing) {

		case E_4K2K_30HZ:
			_vby1_gfx_4k_30HZ();
		break;

		case E_4K2K_60HZ:
			_vb1_gfx_4k_setting(pctx);
		break;

		default:
			DRM_INFO("[%s][%d] GFX OUTPUT TIMING Not Support\n", __func__, __LINE__);
		break;
		}
	}
	//serializer data sel
	if (pctx->v_cfg.linkIF == E_LINK_LVDS)
		W2BYTEMSK(reg_datax_sel, 0x0, 0xF);
	else
		_vb1_seri_data_sel(pctx);
}

static void _mtk_vby1_fifo_reset(void)
{
	W2BYTEMSK(reg_v1_sw_force_fix_trig, 0x4000, 0x4000);
	W2BYTEMSK(reg_v1_sw_force_fix_trig, 0x0, 0x4000);

	W2BYTEMSK(reg_v2_sw_force_fix_trig, 0x4000, 0x4000);
	W2BYTEMSK(reg_v2_sw_force_fix_trig, 0x0, 0x4000);

	W2BYTEMSK(reg_osd_sw_force_fix_trig, 0x4000, 0x4000);
	W2BYTEMSK(reg_osd_sw_force_fix_trig, 0x0, 0x4000);
}

void _mtk_vby1_lane_order(struct mtk_panel_context *pctx)
{
	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		if (pctx->v_cfg.timing == E_FHD_60HZ) {
			W2BYTEMSK(reg_vby1_16ch_cken_31_16, 0x0, 0x1000);
		} else if (pctx->v_cfg.timing == E_FHD_120HZ) {
			W2BYTEMSK(reg_vby1_16ch_cken_31_16, 0x0, 0x1000);
		} else if (pctx->v_cfg.timing == E_4K2K_60HZ) {
			W2BYTEMSK(reg_vby1_16ch_cken_31_16, 0x0, 0x1000);
		} else if (pctx->v_cfg.timing == E_4K2K_120HZ) {
			W2BYTEMSK(reg_vby1_16ch_cken_31_16, 0x0, 0x1000);
		} else if (pctx->v_cfg.timing == E_8K4K_60HZ
			   && pctx->v_cfg.format == E_OUTPUT_YUV422) {
			W2BYTEMSK(reg_vby1_16ch_cken_31_16, 0x0, 0x1000);
		} else if (pctx->v_cfg.timing == E_8K4K_60HZ) {
			W2BYTEMSK(reg_vby1_16ch_cken_31_16, 0x1000, 0x1000);
		} else {
			DRM_INFO("[%s][%d] OUTPUT TIMING Not Support\n", __func__, __LINE__);
		}
		/*
		 *  W2BYTEMSK(reg_vby1_swap_ch1_ch0, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch3_ch2, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch5_ch4, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch7_ch6, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch9_ch8, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch11_ch10, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch13_ch12, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch15_ch14, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch17_ch16, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch19_ch18, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch21_ch20, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch23_ch22, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch25_ch24, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch27_ch26, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch29_ch28, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_vby1_swap_ch31_ch30, 0x0, 0x3F3F);
		 */
	}

	if (pctx->deltav_cfg.linkIF != E_LINK_NONE) {

		/*
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch1_ch0, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch3_ch2, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch5_ch4, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch7_ch6, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch9_ch8, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch11_ch10, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch13_ch12, 0x0, 0x3F3F);
		 *  W2BYTEMSK(reg_v2_vby1_swap_ch15_ch14, 0x0, 0x3F3F);
		 */
	}

}

static void _mtk_pll_powerdown(bool bEn)
{
	if (bEn) {
		W2BYTEMSK(reg_gcr_en, 0x0, 0x8000);	//MOD power regulator
		W2BYTEMSK(reg_mpll_pd, 0x8, 0x8);	//MPLL power down
		//TODO: delay 500us.
		W2BYTEMSK(reg_mpll_clk_adc_vco_div2_pd, 0x10, 0x10);	//864M clk to LPLL
		W2BYTEMSK(reg_lpll1_pd, 0x20, 0x20);	//LPLL-VCO1 power down
		//W2BYTEMSK(reg_lpll2_pd, 0x20, 0x20); //LPLL-VCO2 power down
	} else {
		W2BYTEMSK(reg_gcr_en, 0x8000, 0x8000);	//MOD power regulator
		W2BYTEMSK(reg_mpll_pd, 0x0, 0x8);	//MPLL power down
		//TODO: delay 500us.
		W2BYTEMSK(reg_mpll_clk_adc_vco_div2_pd, 0x0, 0x10);	//864M clk to LPLL
		W2BYTEMSK(reg_lpll1_pd, 0x0, 0x20);	//LPLL-VCO1 power down
		//W2BYTEMSK(reg_lpll2_pd, 0x0, 0x20); //LPLL-VCO2 power down
	}
}

static E_PNL_SUPPORTED_LPLL_TYPE _get_link_idx(struct mtk_panel_context *pctx)
{
	E_PNL_SUPPORTED_LPLL_TYPE link_idx = E_PNL_SUPPORTED_LPLL_MAX;

	if (pctx->v_cfg.linkIF == E_LINK_LVDS) {

		link_idx = E_PNL_SUPPORTED_LPLL_LVDS_2ch_450to600MHz;
	} else if (pctx->v_cfg.linkIF == E_LINK_VB1) {

		if (pctx->v_cfg.timing == E_8K4K_60HZ && pctx->v_cfg.format == E_OUTPUT_YUV422) {
			if (pctx->gfx_cfg.format == E_OUTPUT_RGBA8888)
				link_idx =
				    E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_4BYTE_300to600MHz;
			else
				link_idx =
				    E_PNL_SUPPORTED_LPLL_Vx1_Video_5BYTE_OSD_5BYTE_300to600MHz;

		} else {
			if (pctx->gfx_cfg.format == E_OUTPUT_RGBA8888)
				link_idx =
				    E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_4BYTE_300to600MHz;
			else
				link_idx =
				    E_PNL_SUPPORTED_LPLL_Vx1_Video_4BYTE_OSD_5BYTE_300to600MHz;
		}

	} else {
		DRM_INFO("[%s][%d] Link type Not Support\n", __func__, __LINE__);
	}

	return link_idx;
}

static void _mtk_load_lpll_tbl(
	struct mtk_panel_context *pctx)
{
	uint8_t idx = 0;
	E_PNL_SUPPORTED_LPLL_TYPE tblidx = 0;

	tblidx = _get_link_idx(pctx);

	if (tblidx < E_PNL_SUPPORTED_LPLL_MAX) {
		for (idx = 0; idx < LPLL_REG_NUM; idx++) {

			W2BYTEMSK(LPLLSettingTBL[tblidx][idx].address,
				  LPLLSettingTBL[tblidx][idx].value,
				  LPLLSettingTBL[tblidx][idx].mask);
		}
	}
}

static void _mtk_tgen_init(
	struct mtk_panel_context *pctx)
{

	//init xc tgen

	W2BYTE(reg_tgen_ts_md, 0);	//legacy mode.

	W2BYTE(reg_htt, pctx->info.typ_htt);
	W2BYTE(reg_vtt, pctx->info.typ_vtt);

	W2BYTE(reg_hsync_st, 0);
	W2BYTE(reg_hsync_end, pctx->info.hsync_w - 1);

	W2BYTE(reg_hde_frame_st, pctx->info.de_hstart);
	W2BYTE(reg_hde_frame_end, pctx->info.de_hstart + pctx->info.de_width - 1);
	W2BYTE(reg_hde_frame_v_st, pctx->info.de_hstart);
	W2BYTE(reg_hde_frame_v_end, pctx->info.de_hstart + pctx->info.de_width - 1);
	W2BYTE(reg_hde_frame_g_st, pctx->info.de_hstart);
	W2BYTE(reg_hde_frame_g_end, pctx->info.de_hstart + pctx->info.de_width - 1);

	W2BYTE(reg_vsync_st, 0);
	W2BYTE(reg_vsync_end, pctx->info.vsync_w - 1);
	W2BYTE(reg_vde_frame_st, pctx->info.de_vstart);
	W2BYTE(reg_vde_frame_end, pctx->info.de_vstart + pctx->info.de_height - 1);
	W2BYTE(reg_vde_frame_v_st, pctx->info.de_vstart);
	W2BYTE(reg_vde_frame_v_end, pctx->info.de_vstart + pctx->info.de_height - 1);
	W2BYTE(reg_vde_frame_g_st, pctx->info.de_vstart);
	W2BYTE(reg_vde_frame_g_end, pctx->info.de_vstart + pctx->info.de_height - 1);

	W2BYTE(reg_m_delta, 0);
	W2BYTE(reg_protect_v_end, 0);
	W2BYTE(reg_panel_lower_bound, pctx->info.min_vtt);
	W2BYTE(reg_panel_upper_bound, pctx->info.max_vtt);
	W2BYTE(reg_even_vtt_vcnt_en, 1);
	W2BYTE(reg_tgen_sw_rst, 0);
	W2BYTE(reg_tgen_video_en, 1);

	W2BYTE(reg_hde_video0_st_1p, pctx->info.de_hstart);
	W2BYTE(reg_hde_video0_end_1p, pctx->info.de_hstart + pctx->info.de_width - 1);
	W2BYTE(reg_vde_video0_st_1p, pctx->info.de_vstart);
	W2BYTE(reg_vde_video0_end_1p, pctx->info.de_vstart + pctx->info.de_height - 1);


}

static void _mtk_odclk_setting(
	struct mtk_panel_context *pctx)
{

	uint64_t lpllset = 0;
	E_PNL_SUPPORTED_LPLL_TYPE tblidx = 0;
	uint8_t loop_gain = 0;
	uint8_t loop_div = 0;
	uint32_t odclk = 0;

	tblidx = _get_link_idx(pctx);
	loop_gain = u16LoopGain[tblidx];
	loop_div = u16LoopDiv[tblidx];
	odclk = pctx->info.typ_dclk;

	lpllset = ((uint64_t)LVDS_MPLL_CLOCK_MHZ * 524288 * 16 * loop_gain);
	do_div(lpllset, loop_div);
	do_div(lpllset, odclk);

	W4BYTE(reg_lpll_set, lpllset);
}

static void _vby1_v_8k_out_conf(
	struct mtk_panel_context *pctx,
	bool bEn)
{
	if (pctx->v_cfg.format == E_OUTPUT_YUV422) {
		W2BYTEMSK(reg_output_conf_ch07_ch00, bEn?0x5555:0x0, 0xFFFF);
		W2BYTEMSK(reg_output_conf_ch15_ch08, bEn?0x5555:0x0, 0xFFFF);
	} else {
		W2BYTEMSK(reg_output_conf_ch07_ch00, bEn?0x5555:0x0, 0xFFFF);
		W2BYTEMSK(reg_output_conf_ch15_ch08, bEn?0x5555:0x0, 0xFFFF);
		W2BYTEMSK(reg_output_conf_ch19_ch16, bEn?0x0055:0x0, 0xFFFF);
		W2BYTEMSK(reg_output_conf_ch27_ch20, bEn?0x5555:0x0, 0xFFFF);
		W2BYTEMSK(reg_output_conf_ch35_ch28, bEn?0x0055:0x0, 0xFFFF);
	}
}

static void _vby1_deltav_4k_out_conf(
	struct mtk_panel_context *pctx,
	bool bEn)
{
	if (pctx->deltav_cfg.timing == E_4K2K_60HZ) {
		W2BYTEMSK(reg_v2_output_conf_ch19_ch16, bEn?0x0055:0x0, 0xFFFF);
		W2BYTEMSK(reg_v2_output_conf_ch27_ch20, bEn?0x5555:0x0, 0xFFFF);
		W2BYTEMSK(reg_v2_output_conf_ch35_ch28, bEn?0x0055:0x0, 0xFFFF);

	} else if (pctx->deltav_cfg.timing == E_4K2K_120HZ) {
		W2BYTEMSK(reg_v2_output_conf_ch19_ch16, bEn?0x0055:0x0, 0xFFFF);
		W2BYTEMSK(reg_v2_output_conf_ch27_ch20, bEn?0x5555:0x0, 0xFFFF);
		W2BYTEMSK(reg_v2_output_conf_ch35_ch28, bEn?0x0055:0x0, 0xFFFF);
	} else {
		DRM_INFO("[%s][%d] OUTPUT TIMING Not Support\n", __func__, __LINE__);
	}

}


void mtk_render_output_en(
	struct mtk_panel_context *pctx,
	bool bEn)
{
	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		switch (pctx->v_cfg.timing) {
		case E_FHD_60HZ:
		{
			W2BYTEMSK(reg_output_conf_ch07_ch00, bEn?0x0440:0x0, 0xFFFF);
			W2BYTEMSK(reg_output_conf_ch15_ch08, 0x0, 0xFFFF);
		}
		break;
		case E_FHD_120HZ:
		{
			W2BYTEMSK(reg_output_conf_ch07_ch00, bEn?0x5440:0x0, 0xFFFF);
			W2BYTEMSK(reg_output_conf_ch15_ch08, 0x0, 0xFFFF);
		}
		break;
		case E_4K2K_60HZ:
		{
			W2BYTEMSK(reg_output_conf_ch07_ch00, bEn?0x5545:0x0, 0xFFFF);
			W2BYTEMSK(reg_output_conf_ch15_ch08, bEn?0x0010:0x0, 0xFFFF);
		}
		break;
		case E_4K2K_120HZ:
		{
			W2BYTEMSK(reg_output_conf_ch07_ch00, bEn?0x5555:0x0, 0xFFFF);
			W2BYTEMSK(reg_output_conf_ch15_ch08, bEn?0x5555:0x0, 0xFFFF);
		}
		break;
		case E_8K4K_60HZ:
		{
			_vby1_v_8k_out_conf(pctx, bEn);
		}
		break;
		default:
			DRM_INFO("[%s][%d] OUTPUT TIMING Not Support\n", __func__, __LINE__);
		break;
		}
	}

	if (pctx->deltav_cfg.linkIF != E_LINK_NONE)
		_vby1_deltav_4k_out_conf(pctx, bEn);

	if (pctx->gfx_cfg.linkIF != E_LINK_NONE) {
		W2BYTEMSK(reg_o_output_conf_ch35_ch28, bEn?0x5500:0x0, 0xFFFF);
		W2BYTEMSK(reg_o_output_conf_ch39_ch36, bEn?0x0055:0x0, 0xFFFF);
	}

}

bool mtk_render_cfg_connector(
	struct mtk_panel_context *pctx)
{
	uint8_t out_clk_if = 0;

	if (pctx == NULL) {
		DRM_INFO("[%s][%d]param is NULL, FAIL\n", __func__, __LINE__);
		return false;
	}
	//Determin timing of each connector
	switch (pctx->cus.disp_mode) {

	case E_VG_COMB_MODE:
		pctx->v_cfg.linkIF = pctx->info.linktype;
		pctx->v_cfg.lanes = pctx->op_cfg.ctrl_chs;
		pctx->v_cfg.div_sec = 1;
		if (pctx->v_cfg.linkIF == E_LINK_LVDS)
			pctx->v_cfg.timing = E_FHD_60HZ;
		else
			pctx->v_cfg.timing = E_8K4K_60HZ;

		pctx->v_cfg.format = pctx->cus.op_format;

		//how to know BE timing? (from DTS?)
		pctx->deltav_cfg.linkIF = E_LINK_NONE;
		pctx->deltav_cfg.lanes = 0;
		pctx->deltav_cfg.div_sec = 1;	//Fix
		pctx->deltav_cfg.timing = E_OUTPUT_MODE_NONE;
		pctx->deltav_cfg.format = pctx->cus.op_format;

		pctx->gfx_cfg.linkIF = E_LINK_NONE;
		pctx->gfx_cfg.lanes = 0;
		pctx->gfx_cfg.div_sec = 1;	//Fix
		pctx->gfx_cfg.timing = E_OUTPUT_MODE_NONE;
		pctx->gfx_cfg.format = pctx->cus.op_format;
		break;
	case E_VG_SEP_MODE:
	case E_VG_SEP_W_DELTAV_MODE:

		pctx->v_cfg.linkIF = pctx->info.linktype;
		pctx->v_cfg.lanes = pctx->op_cfg.ctrl_chs;
		pctx->v_cfg.div_sec = 1;
		pctx->v_cfg.timing = E_8K4K_60HZ;
		pctx->v_cfg.format = pctx->cus.op_format;

		//how to know BE timing? (from DTS?)
		pctx->deltav_cfg.linkIF = E_LINK_VB1;
		pctx->deltav_cfg.lanes = 16;	//FixMe.
		pctx->deltav_cfg.div_sec = 1;
		pctx->deltav_cfg.timing = E_4K2K_60HZ;
		pctx->deltav_cfg.format = E_OUTPUT_YUV444;

		pctx->gfx_cfg.linkIF = E_LINK_VB1;
		pctx->gfx_cfg.lanes = 8;
		pctx->gfx_cfg.div_sec = 1;
		pctx->gfx_cfg.timing = E_4K2K_60HZ;
		pctx->gfx_cfg.format = E_OUTPUT_RGBA8888;
		break;

	default:
		break;
	}

	_mtk_pll_powerdown(false);

	_mtk_load_lpll_tbl(pctx);

	_mtk_cfg_ckg_common(pctx);

	//determine output timing of each link (V / DeltaV /OSD)
	if (pctx->v_cfg.linkIF != E_LINK_NONE) {
		if (pctx->v_cfg.timing == E_8K4K_60HZ) {
			if (pctx->v_cfg.format == E_OUTPUT_YUV444)
				out_clk_if = 32;	// 8K60 YUV444, 32p
			else
				out_clk_if = 16;	//8K60 yuv422, 16p
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

		_mtk_cfg_ckg_video(pctx->v_cfg, MOD_IN_IF_VIDEO, out_clk_if);
	}

	if (pctx->deltav_cfg.linkIF != E_LINK_NONE) {

		if (pctx->deltav_cfg.timing == E_4K2K_120HZ)
			out_clk_if = 16;
		else if (pctx->deltav_cfg.timing == E_4K2K_60HZ)
			out_clk_if = 8;
		else
			out_clk_if = 0;

		_mtk_cfg_ckg_deltav(pctx->deltav_cfg, MOD_IN_IF_DeltaV, out_clk_if);
	}
	if (pctx->gfx_cfg.linkIF != E_LINK_NONE) {
		if (pctx->gfx_cfg.timing == E_4K2K_60HZ)
			out_clk_if = 8;
		else if (pctx->gfx_cfg.timing == E_4K2K_30HZ)
			out_clk_if = 4;
		else
			out_clk_if = 0;

		_mtk_cfg_ckg_gfx(pctx->gfx_cfg, MOD_IN_IF_GRAPH, out_clk_if);
	}

	_mtk_tgen_init(pctx);
	_mtk_odclk_setting(pctx);

	_mtk_vby1_setting(pctx);
	_mtk_vby1_lane_order(pctx);
	_mtk_vby1_fifo_reset();
	mtk_render_output_en(pctx, true);

	return true;
}

bool mtk_render_cfg_crtc(
	struct mtk_panel_context *pctx)
{
	//TODO:Refine
	return true;
}

static int _mtk_video_set_vttv_mode(
	struct mtk_tv_kms_context *pctx,
	uint64_t input_vfreq, bool bLowLatencyEn)
{
	int ret = 0, FRCType = 0;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	drm_en_vttv_source_type ivsSource = REF_PULSE, ovsSource = REF_PULSE;
	uint8_t ivs = 0, ovs = 0;
	uint16_t vttLowerBound = 500, windowNum = 0;
	uint64_t output_vfreq = 0, default_vfreq = 0, outputVTT = 0, phase_diff = 0;
	struct reg_info reg[REG_MAX_INDEX];
	struct hwregOut paramOut;

	//decide ivs/ovs source
	windowNum = pctx->video_priv.videoPlaneType_PlaneNum.MEMC_num +
		pctx->video_priv.videoPlaneType_PlaneNum.MGW_num +
		pctx->video_priv.videoPlaneType_PlaneNum.DMA1_num +
		pctx->video_priv.videoPlaneType_PlaneNum.DMA2_num;

	if (windowNum > 1) {
		ivsSource = REF_PULSE;
		ovsSource = REF_PULSE;
	} else {
		if (bLowLatencyEn) {
			ivsSource = RW_BK_UPD_OP2;
			ovsSource = RW_BK_UPD_DISP;
		} else {
			ivsSource = REF_PULSE;
			ovsSource = REF_PULSE;
		}
	}
	DRM_INFO("[%s][%d]windowNum: %d, ivsSource: %d, ovsSource: %d\n",
			__func__, __LINE__, windowNum, ivsSource, ovsSource);

	//find ivs/ovs
	for (FRCType = 0;
		FRCType < (sizeof(pctx_pnl->frc_info)/sizeof(struct drm_st_pnl_frc_table_info));
		FRCType++) {
		if (input_vfreq > pctx_pnl->frc_info[FRCType].u16LowerBound &&
			input_vfreq <= pctx_pnl->frc_info[FRCType].u16HigherBound) {
			ivs = pctx_pnl->frc_info[FRCType].u8FRC_in;
			ovs = pctx_pnl->frc_info[FRCType].u8FRC_out;
			break;
		}
	}

	//calc output vfreq
	if (ivs > 0) {
		output_vfreq = input_vfreq * ovs / ivs;
		pctx_pnl->outputTiming_info.u16OutputVfreq = output_vfreq;

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
			((uint64_t)pctx_pnl->info.typ_dclk*10000000) /
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
		pctx_pnl->outputTiming_info.u16OutputVTotal = outputVTT;

		DRM_INFO("[%s][%d] outputVTT: %td\n",
			__func__, __LINE__, (ptrdiff_t)outputVTT);
	} else {
		DRM_INFO("[%s][%d] calc output vtt fail, outputVTT: %td\n",
			__func__, __LINE__, (ptrdiff_t)outputVTT);
	}

	//set lock user phase diff
	if (bLowLatencyEn) {
		phase_diff = outputVTT*FRAMESYNC_VTTV_LOW_LATENCY_PHASE_DIFF/1000;

		DRM_INFO("[%s][%d] phase_diff: %td\n",
			__func__, __LINE__, (ptrdiff_t)phase_diff);
	} else {
		phase_diff = outputVTT*FRAMESYNC_VTTV_NORMAL_PHASE_DIFF/1000;

		DRM_INFO("[%s][%d] phase_diff: %td\n",
			__func__, __LINE__, (ptrdiff_t)phase_diff);
	}

	pctx_pnl->outputTiming_info.eFrameSyncMode = VIDEO_CRTC_FRAME_SYNC_VTTV;
	memset(&reg, 0, sizeof(struct reg_info)*REG_MAX_INDEX);
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;
	//h31[0] = 0
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		false,
		false);

	//BKA3A3_h16[4:0] = ivsSource
	//BKA3A3_h16[12:8] = ovsSource
	drv_hwreg_render_pnl_set_vttSourceSelect(
		&paramOut,
		false,
		ivsSource,
		ovsSource);

	//h20[15] = 1
	//h20[14:8] = 3
	//h20[7:0] = 3
	drv_hwreg_render_pnl_set_centralVttChange(
		&paramOut,
		false,
		true,
		FRAMESYNC_VTTV_FRAME_COUNT,
		FRAMESYNC_VTTV_THRESHOLD);

	//h22[15:3] = phase_diff
	//h23[15] = 1
	drv_hwreg_render_pnl_set_vttLockPhaseDiff(
		&paramOut,
		false,
		true,
		phase_diff);

	//h24[4] = 1
	drv_hwreg_render_pnl_set_vttLockKeepSequenceEnable(
		&paramOut,
		false,
		true);

	//h33[10] = 1
	//h30[15] = 1
	//h30[14:0] = 5
	drv_hwreg_render_pnl_set_vttLockDiffLimit(
		&paramOut,
		false,
		true,
		FRAMESYNC_VTTV_DIFF_LIMIT);

	//h33[11] = 1
	//h33[12] = 1
	//h36[15:0] = pctx_pnl->stPanelType.m_wPanelMaxVTotal
	//h37[15:0] = after y_trigger
	drv_hwreg_render_pnl_set_vttLockVttRange(
		&paramOut,
		false,
		true,
		pctx_pnl->info.max_vtt,
		vttLowerBound);

	//h33[13] = 1
	drv_hwreg_render_pnl_set_vttLockVsMaskMode(
		&paramOut,
		false,
		true);

	//h32[7:0] = ivs
	//h32[15:8] = ovs
	drv_hwreg_render_pnl_set_vttLockIvsOvs(
		&paramOut,
		false,
		ivs,
		ovs);

	//h3e[15:0] = 9
	drv_hwreg_render_pnl_set_vttLockLockedInLimit(
		&paramOut,
		false,
		FRAMESYNC_VTTV_LOCKED_LIMIT);

	//h31[13] = 1
	drv_hwreg_render_pnl_set_vttLockUdSwModeEnable(
		&paramOut,
		false,
		true);

	//h31[9] = 0
	drv_hwreg_render_pnl_set_vttLockIvsShiftEnable(
		&paramOut,
		false,
		false);

	//h31[8] = 1
	drv_hwreg_render_pnl_set_vttLockProtectEnable(
		&paramOut,
		false,
		true);

	//h31[0] = 1
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		false,
		true);

	//h24[12] = 0
	drv_hwreg_render_pnl_set_vttLockLastVttLatchEnable(
		&paramOut,
		false,
		false);

	return ret;
}

int _mtk_video_set_low_latency_mode(
	struct mtk_tv_kms_context *pctx,
	uint64_t bLowLatencyEn)
{
	int ret = 0;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	drm_en_vttv_source_type ivsSource = REF_PULSE, ovsSource = REF_PULSE;
	uint64_t outputVTT = 0, phase_diff = 0;
	uint16_t windowNum = 0;
	struct reg_info reg[REG_MAX_INDEX];
	struct hwregOut paramOut;

	memset(&reg, 0, sizeof(struct reg_info)*REG_MAX_INDEX);
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	//decide ivs/ovs source
	windowNum = pctx->video_priv.videoPlaneType_PlaneNum.MEMC_num +
		pctx->video_priv.videoPlaneType_PlaneNum.MGW_num +
		pctx->video_priv.videoPlaneType_PlaneNum.DMA1_num +
		pctx->video_priv.videoPlaneType_PlaneNum.DMA2_num;

	if (windowNum > 1) {
		ivsSource = REF_PULSE;
		ovsSource = REF_PULSE;
	} else {
		if (bLowLatencyEn) {
			ivsSource = RW_BK_UPD_OP2;
			ovsSource = RW_BK_UPD_DISP;
		} else {
			ivsSource = REF_PULSE;
			ovsSource = REF_PULSE;
		}
	}
	DRM_INFO("[%s][%d]windowNum: %d, ivsSource: %d, ovsSource: %d\n",
			__func__, __LINE__, windowNum, ivsSource, ovsSource);

	//get output VTT
	outputVTT = pctx_pnl->outputTiming_info.u16OutputVTotal;

	//set lock user phase diff
	if (bLowLatencyEn) {
		phase_diff = outputVTT*FRAMESYNC_VTTV_LOW_LATENCY_PHASE_DIFF/1000;

		DRM_INFO("[%s][%d] phase_diff: %td\n",
			__func__, __LINE__, (ptrdiff_t)phase_diff);
	} else {
		phase_diff = outputVTT*FRAMESYNC_VTTV_NORMAL_PHASE_DIFF/1000;

		DRM_INFO("[%s][%d] phase_diff: %td\n",
			__func__, __LINE__, (ptrdiff_t)phase_diff);
	}

	//h31[0] = 0
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		false,
		false);

	//BKA3A3_h16[4:0] = ivsSource
	//BKA3A3_h16[12:8] = ovsSource
	drv_hwreg_render_pnl_set_vttSourceSelect(
		&paramOut,
		false,
		ivsSource,
		ovsSource);

	//h22[15:3] = phase_diff
	drv_hwreg_render_pnl_set_vttLockPhaseDiff(
		&paramOut,
		false,
		true,
		phase_diff);

	//h31[0] = 1
	drv_hwreg_render_pnl_set_vttLockModeEnable(
		&paramOut,
		false,
		true);

	//h24[12] = 0
	drv_hwreg_render_pnl_set_vttLockLastVttLatchEnable(
		&paramOut,
		false,
		false);

	return 0;
}

static int _mtk_video_set_vrr_mode(
	struct mtk_tv_kms_context *pctx)
{
	int ret = 0;
	uint16_t rwdiff = 0, protectLine = 32;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct reg_info reg[REG_MAX_INDEX];
	struct hwregOut paramOut;

	memset(&reg, 0, sizeof(struct reg_info)*REG_MAX_INDEX);
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;
	pctx_pnl->outputTiming_info.eFrameSyncMode = VIDEO_CRTC_FRAME_SYNC_FRAMELOCK;

	//enable rwdiff auto-trace
	//BKA38B_40[11] = 1
	drv_hwreg_render_pnl_set_rwDiffTraceEnable(
		&paramOut,
		false,
		true);

	//BKA38B_44[4:0] = 1
	drv_hwreg_render_pnl_set_rwDiffTraceDepositCount(
		&paramOut,
		false,
		FRAMESYNC_VRR_DEPOSIT_COUNT);

	//Turn on Framelock 1:1
	//BKA3A2_01[0] = 1
	drv_hwreg_render_pnl_set_InsertRecord(
		&paramOut,
		false,
		true);

	//BKA3A2_11[0] = 1
	drv_hwreg_render_pnl_set_fakeInsertPEnable(
		&paramOut,
		false,
		true);

	//BKA3A2_10[3] = 1
	drv_hwreg_render_pnl_set_stateSwModeEnable(
		&paramOut,
		false,
		true);

	//BKA3A2_10[6:4] = 4
	drv_hwreg_render_pnl_set_stateSwMode(
		&paramOut,
		false,
		FRAMESYNC_VRR_STATE_SW_MODE);

	//BKA3A2_1e[0] = 1
	//BKA3A2_1e[1] = 1
	//BKA3A2_1e[2] = 1
	//BKA3A2_18[15:0] = pctx_pnl->cus.m_del
	//BKA3A2_19[15:0] = pctx_pnl->info.vrr_min_v
	//BKA3A2_1a[15:0] = pctx_pnl->info.vrr_max_v
	drv_hwreg_render_pnl_set_FrameLockVttRange(
		&paramOut,
		false,
		true,
		pctx_pnl->cus.m_del,
		pctx_pnl->info.vrr_min_v,
		pctx_pnl->info.vrr_max_v);

	//BKA3A2_23[15:0] = 2
	drv_hwreg_render_pnl_set_chaseTotalDiffRange(
		&paramOut,
		false,
		FRAMESYNC_VRR_CHASE_TOTAL_DIFF_RANGE);

	//BKA3A2_31[15:0] = 2
	drv_hwreg_render_pnl_set_chasePhaseTarget(
		&paramOut,
		false,
		FRAMESYNC_VRR_CHASE_PHASE_TARGET);

	//BKA3A2_32[15:0] = 2
	drv_hwreg_render_pnl_set_chasePhaseAlmost(
		&paramOut,
		false,
		FRAMESYNC_VRR_CHASE_PHASE_ALMOST);

	//BKA3A1_24[13] = 0
	drv_hwreg_render_pnl_set_vttLockLastVttLatchToggle(
		&paramOut,
		false,
		false);

	//BKA3A0_22[0] = 1
	drv_hwreg_render_pnl_set_framelockVcntEnableDb(
		&paramOut,
		false,
		true);

	//set rwdiff = 0

	//wbank protect on
	//BKA38B_4C[7:0] = protect line / 8
	drv_hwreg_render_pnl_set_rbankRefProtect(
		&paramOut,
		false,
		protectLine/FRAMESYNC_VRR_PROTECT_DIV);

	return 0;
}

static int _mtk_video_turn_off_framesync_mode(
	struct mtk_tv_kms_context *pctx)
{
	int ret = 0;
	struct mtk_panel_context *pctx_pnl = &pctx->panel_priv;
	struct reg_info reg[REG_MAX_INDEX];
	struct hwregOut paramOut;

	memset(&reg, 0, sizeof(struct reg_info)*REG_MAX_INDEX);
	memset(&paramOut, 0, sizeof(struct hwregOut));
	paramOut.reg = reg;

	if (pctx_pnl->outputTiming_info.eFrameSyncMode ==
		VIDEO_CRTC_FRAME_SYNC_FRAMELOCK) {
		//turn off vrr mode
		//BKA3A1_24[13] = 1
		drv_hwreg_render_pnl_set_vttLockLastVttLatchToggle(
			&paramOut,
			false,
			true);
		//BKA3A0_22[0] = 0
		drv_hwreg_render_pnl_set_framelockVcntEnableDb(
			&paramOut,
			false,
			false);
	} else if (pctx_pnl->outputTiming_info.eFrameSyncMode ==
		VIDEO_CRTC_FRAME_SYNC_VTTV) {
		//turn off vttv mode
		//BKA3A1_31[0] = 0
		drv_hwreg_render_pnl_set_vttLockModeEnable(
			&paramOut,
			false,
			false);
		//BKA3A1_24[12] = 1
		drv_hwreg_render_pnl_set_vttLockLastVttLatchEnable(
			&paramOut,
			false,
			true);
	}

	return 0;
}

int _mtk_video_set_customize_frc_table(
	struct mtk_panel_context *pctx_pnl,
	struct drm_st_pnl_frc_table_info *customizeFRCTableInfo)
{
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

static int _mtk_video_get_vttv_input_vfreq(
	struct mtk_tv_kms_context *pctx,
	uint64_t plane_id,
	uint64_t *input_vfreq)
{
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
	struct ext_crtc_prop_info *crtc_props = pctx->ext_crtc_properties;
	uint64_t input_vfreq = 0, framesync_mode = 0;
	bool bLowLatencyEn = false;

	framesync_mode = crtc_props->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE];

	if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_VTTV) {
		_mtk_video_get_vttv_input_vfreq(pctx,
			crtc_props->prop_val[E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID],
			&input_vfreq);
		DRM_INFO("[%s][%d] EXT_VPLANE_PROP_INPUT_VFREQ= %td\n",
			__func__, __LINE__, (ptrdiff_t)input_vfreq);
		bLowLatencyEn =
			(bool)crtc_props->prop_val[E_EXT_CRTC_PROP_LOW_LATENCY_MODE];
		_mtk_video_turn_off_framesync_mode(pctx);
		_mtk_video_set_vttv_mode(pctx, input_vfreq, bLowLatencyEn);

	} else if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_FRAMELOCK) {
		_mtk_video_turn_off_framesync_mode(pctx);
		_mtk_video_set_vrr_mode(pctx);
	}
	return 0;
}

