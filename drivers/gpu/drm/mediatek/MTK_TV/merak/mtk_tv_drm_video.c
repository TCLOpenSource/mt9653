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
#include "mtk_tv_drm_common.h"
#include "drv_scriptmgt.h"
#include "mtk_tv_drm_demura.h"
#include "mtk_tv_drm_log.h"

#define MTK_DRM_MODEL MTK_DRM_MODEL_VIDEO
#define VFRQRATIO_1000 (1000)
#define MAX_WINDOW_NUM (16)
#define SW_DELAY (20) // for LTP test

static void _mtk_video_display_set_BackgroundRGB(
	struct mtk_tv_kms_context *pctx,
	void *buffer_addr)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct drm_mtk_bg_color *bg_color = (struct drm_mtk_bg_color *)buffer_addr;
	struct reg_info reg[reg_num];
	struct hwregPostFillBgARGBIn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	int i;

	if (buffer_addr == NULL) {
		DRM_ERROR("[%s][%d] set background color fail\n", __func__, __LINE__);
		return;
	}

	DRM_INFO("[%s][%d][paramIn]background RGB:[%llx, %llx, %llx]\n", __func__, __LINE__,
		bg_color->red, bg_color->green, bg_color->blue);

	for (i = MTK_VPLANE_TYPE_MEMC; i < MTK_VPLANE_TYPE_MAX; ++i) {
		memset(reg, 0, sizeof(reg));
		memset(&paramIn, 0, sizeof(struct hwregPostFillBgARGBIn));
		memset(&paramOut, 0, sizeof(struct hwregOut));
		memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

		paramIn.RIU = 0;
		paramIn.planeType = i;
		paramIn.bgARGB.R = bg_color->red;
		paramIn.bgARGB.G = bg_color->green;
		paramIn.bgARGB.B = bg_color->blue;
		paramOut.reg = reg;

		drv_hwreg_render_display_set_postFillBgRGB(paramIn, &paramOut);
		if ((!paramIn.RIU) && (paramOut.regCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = paramOut.regCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->disp_ml.regcount =
				pctx_video->disp_ml.regcount + paramOut.regCount;
		}
		DRM_INFO("[%s][%d]: plane_type: %d, RIU %d, regCount %d", __func__, __LINE__,
			i, paramIn.RIU, paramOut.regCount);
	}
}

static void _mtk_video_crtc_set_windowRGB(
	struct mtk_tv_kms_context *pctx,
	void *buffer_addr,
	uint32_t buffer_size)
{
	struct mtk_video_context *pctx_video = &pctx->video_priv;
	struct video_plane_prop *plane_props = pctx_video->video_plane_properties;
	uint16_t reg_num = pctx_video->reg_num;
	uint8_t memindex = pctx_video->disp_ml.memindex;
	int fd = pctx_video->disp_ml.fd;
	struct drm_mtk_mute_color *mute_color = (struct drm_mtk_mute_color *)buffer_addr;
	struct reg_info reg[reg_num];
	struct hwregPostFillWindowARGBIn paramIn;
	struct hwregOut paramOut;
	struct sm_ml_add_info cmd_info;
	int i;

	DRM_INFO("[%s][%d][paramIn]window RGB:[%llx, %llx, %llx]\n", __func__, __LINE__,
		mute_color->red, mute_color->green, mute_color->blue);

	for (i = MTK_VPLANE_TYPE_MEMC; i < MTK_VPLANE_TYPE_MAX; ++i) {
		memset(reg, 0, sizeof(reg));
		memset(&paramIn, 0, sizeof(struct hwregPostFillWindowARGBIn));
		memset(&paramOut, 0, sizeof(struct hwregOut));
		memset(&cmd_info, 0, sizeof(struct sm_ml_add_info));

		paramIn.RIU = 0;
		paramIn.planeType = i;
		paramIn.windowARGB.R = mute_color->red;
		paramIn.windowARGB.G = mute_color->green;
		paramIn.windowARGB.B = mute_color->blue;
		paramOut.reg = reg;

		drv_hwreg_render_display_set_postFillWindowRGB(paramIn, &paramOut);
		if ((!paramIn.RIU) && (paramOut.regCount != 0)) {
			//3.add cmd
			cmd_info.cmd_cnt = paramOut.regCount;
			cmd_info.mem_index = memindex;
			cmd_info.reg = (struct sm_reg *)reg;
			sm_ml_add_cmd(fd, &cmd_info);
			pctx_video->disp_ml.regcount =
				pctx_video->disp_ml.regcount + paramOut.regCount;
		}
		DRM_INFO("[%s][%d]: plane_type: %d, RIU %d, regCount %d", __func__, __LINE__,
			i, paramIn.RIU, paramOut.regCount);
	}
}

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

	//static XC_SetTiming_Info stTimingInfo = {0};

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
	case E_EXT_CRTC_PROP_FRAMESYNC_FRC_RATIO:
		if ((property_blob == NULL) && (val != NULL)) {
			property_blob = drm_property_lookup_blob(property->dev, *val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown FRC ratio status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			} else {
				memset(property_blob->data,
					0,
					sizeof(struct drm_st_pnl_frc_ratio_info));
				memcpy(property_blob->data,
					&pctx->panel_priv.frcRatio_info,
					sizeof(struct drm_st_pnl_frc_ratio_info));
				DRM_INFO("[%s][%d] get FRC ratio status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			}
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
	case E_EXT_CRTC_PROP_DEMURA_CONFIG:
		DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_DEMURA_CUC, %td!!\n",
			__func__, __LINE__, (ptrdiff_t)*val);
		break;
	case E_EXT_CRTC_PROP_PATTERN_GENERATE:
		DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_PATTERN_GENERATE, %td!!\n",
			__func__, __LINE__, (ptrdiff_t)*val);
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

void mtk_video_update_crtc(
	struct mtk_tv_drm_crtc *crtc)
{
	int i = 0;
	struct mtk_tv_kms_context *pctx = NULL;
	struct ext_crtc_prop_info *crtc_prop = NULL;
	struct drm_property_blob *property_blob = NULL;
	uint64_t framesync_mode = 0;

	if (!crtc) {
		DRM_ERROR("[%s, %d]: null ptr! crtc=0x%llx\n", __func__, __LINE__, crtc);
		return;
	}

	pctx = (struct mtk_tv_kms_context *)crtc->plane[MTK_DRM_PLANE_TYPE_VIDEO]->crtc_private;
	crtc_prop = pctx->ext_crtc_properties;

	for (i = 0; i < E_EXT_CRTC_PROP_MAX; i++) {
		if (crtc_prop->prop_update[i] == 0) {
			// no update, no work
			continue;
		}
		crtc_prop->prop_update[i] = 0;
		switch (i) {
		case E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID:
			//stTimingInfo.u32HighAccurateInputVFreq = (uint32_t)val;
			//stTimingInfo.u16InputVFreq = (uint32_t)val /100;
			DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_FRAMESYNC_PLANE_ID= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			framesync_mode =
				crtc_prop->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE];
			if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_VTTV) {
				pctx->panel_priv.outputTiming_info.eFrameSyncState =
					E_PNL_FRAMESYNC_STATE_PROP_IN;
				_mtk_video_set_framesync_mode(pctx);
			}
			break;

		case E_EXT_CRTC_PROP_FRAMESYNC_FRC_RATIO:
			DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_FRAMESYNC_FRC_RATIO= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			property_blob = drm_property_lookup_blob(
				pctx->drm_dev, crtc_prop->prop_val[i]);
			if (property_blob != NULL) {
				memset(&pctx->panel_priv.frcRatio_info,
					0,
					sizeof(struct drm_st_pnl_frc_ratio_info));
				memcpy(&pctx->panel_priv.frcRatio_info,
					property_blob->data,
					property_blob->length);

				DRM_INFO("[%s][%d] frc_in: %d, frc_out: %d\n",
					__func__, __LINE__,
					pctx->panel_priv.frcRatio_info.u8FRC_in,
					pctx->panel_priv.frcRatio_info.u8FRC_out);
			} else {
				DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
					__func__, __LINE__,
					(ptrdiff_t)crtc_prop->prop_val[i]);
			}
			break;

		case E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE:
			DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			pctx->panel_priv.outputTiming_info.eFrameSyncState =
				E_PNL_FRAMESYNC_STATE_PROP_IN;
			_mtk_video_set_framesync_mode(pctx);
			break;

		case E_EXT_CRTC_PROP_LOW_LATENCY_MODE:
			DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_LOW_LATENCY_MODE= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			framesync_mode =
				crtc_prop->prop_val[E_EXT_CRTC_PROP_SET_FRAMESYNC_MODE];
			if (framesync_mode == VIDEO_CRTC_FRAME_SYNC_VTTV) {
				pctx->panel_priv.outputTiming_info.eFrameSyncState =
					E_PNL_FRAMESYNC_STATE_PROP_IN;
				_mtk_video_set_low_latency_mode(
					pctx,
					crtc_prop->prop_val[i]);
			}
			break;

		case E_EXT_CRTC_PROP_CUSTOMIZE_FRC_TABLE:
			DRM_INFO("[%s][%d] E_EXT_CRTC_PROP_CUSTOMIZE_FRC_TABLE= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			property_blob = drm_property_lookup_blob(
				pctx->drm_dev, crtc_prop->prop_val[i]);
			if (property_blob != NULL) {
				struct drm_st_pnl_frc_table_info
					cusFRCTableInfo[PNL_FRC_TABLE_MAX_INDEX];

				memset(&cusFRCTableInfo,
					0,
					sizeof(struct drm_st_pnl_frc_table_info)*
					PNL_FRC_TABLE_MAX_INDEX);
				memcpy(&cusFRCTableInfo,
					property_blob->data,
					property_blob->length);

				_mtk_video_set_customize_frc_table(
					&pctx->panel_priv,
					cusFRCTableInfo);
			} else {
				DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
					__func__, __LINE__,
					(ptrdiff_t)crtc_prop->prop_val[i]);
			}
			break;

		case E_EXT_CRTC_PROP_PANEL_TIMING:
			DRM_INFO("[%s][%d] PANEL_TIMING= 0x%tx\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			property_blob = drm_property_lookup_blob(
				pctx->drm_dev, crtc_prop->prop_val[i]);
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
					__func__, __LINE__,
					(ptrdiff_t)crtc_prop->prop_val[i]);
			}
			break;

		case E_EXT_CRTC_PROP_SET_FREERUN_TIMING:
			DRM_INFO("[%s][%d] SET_FREERUN_TIMING= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			//MApi_XC_SetFreeRunTiming();
			break;

		case E_EXT_CRTC_PROP_FORCE_FREERUN:
			DRM_INFO("[%s][%d] FORCE_FREERUN= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			//MApi_SC_ForceFreerun(val);
			break;

		case E_EXT_CRTC_PROP_FREERUN_VFREQ:
			DRM_INFO("[%s][%d] FREERUN_VFREQ= %td\n",
				__func__, __LINE__, (ptrdiff_t)crtc_prop->prop_val[i]);
			//MApi_SC_SetFreerunVFreq(val);
			break;

		case E_EXT_CRTC_PROP_VIDEO_LATENCY:
			property_blob = drm_property_lookup_blob(
				pctx->drm_dev, crtc_prop->prop_val[i]);
			if (property_blob != NULL) {
				drm_st_mtk_tv_video_latency_info stVideoLatencyInfo;

				memset(&stVideoLatencyInfo,
					0,
					sizeof(drm_st_mtk_tv_video_latency_info));

				memcpy(&stVideoLatencyInfo,
					property_blob->data,
					sizeof(drm_st_mtk_tv_video_latency_info));

				DRM_INFO("[%s][%d] VIDEO_LATENCY = %td\n",
					__func__, __LINE__,
					(ptrdiff_t)crtc_prop->prop_val[i]);

				//Update blob id memory
				memcpy(property_blob->data,
					&stVideoLatencyInfo,
					sizeof(drm_st_mtk_tv_video_latency_info));
			} else {
				DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
					__func__, __LINE__,
					(ptrdiff_t)crtc_prop->prop_val[i]);
			}
			break;

		case E_EXT_CRTC_PROP_DEMURA_CONFIG:
			DRM_INFO("[%s][%d] DEMURA_CONFIG", __func__, __LINE__);
			if (crtc_prop->prop_val[i] != 0) {
				mtk_tv_drm_demura_set_config(&pctx->demura_priv,
				(struct drm_mtk_demura_config *)(crtc_prop->prop_val[i]));
			}
			break;

		case E_EXT_CRTC_PROP_PATTERN_GENERATE:
			DRM_INFO("[%s][%d] PATTERN_GENERATE", __func__, __LINE__);
			if (crtc_prop->prop_val[i] != 0) {
				mtk_drm_pattern_set_param(&pctx->pattern_status,
				(struct drm_mtk_pattern_config *)(crtc_prop->prop_val[i]));
			}
			break;

		case E_EXT_CRTC_PROP_MUTE_COLOR:
			if (crtc_prop->prop_val[i] != 0) {
				_mtk_video_crtc_set_windowRGB(pctx,
					(void *)crtc_prop->prop_val[i],
					sizeof(struct drm_mtk_mute_color));
			}
			break;
		case E_EXT_CRTC_PROP_BACKGROUND_COLOR:
			_mtk_video_display_set_BackgroundRGB(pctx, (void *)crtc_prop->prop_val[i]);
			break;

		default:
			break;
		}
	}
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
	case E_EXT_CONNECTOR_PROP_PARSE_OUT_INFO_FROM_DTS:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_PROP_PARSE_OUT_INFO_FROM_DTS = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob == NULL) {
			DRM_INFO("[%s][%d] unknown output info status = %td!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			drm_parse_dts_info param;
			drm_parse_dts_info *drmParseDtsInfo
					= ((drm_parse_dts_info *)(property_blob->data));
			drm_en_pnl_output_model output_model_cal;

			memset(&param, 0, sizeof(drm_parse_dts_info));
			//memcpy(&param, property_blob->data, sizeof(drm_parse_dts_info));

			param.src_info.linkIF = drmParseDtsInfo->src_info.linkIF;
			param.src_info.div_section = drmParseDtsInfo->src_info.div_section;
			param.src_info.hsync_st = drmParseDtsInfo->src_info.hsync_st;
			param.src_info.hsync_w = drmParseDtsInfo->src_info.hsync_w;
			param.src_info.vsync_st = drmParseDtsInfo->src_info.vsync_st;
			param.src_info.vsync_w = drmParseDtsInfo->src_info.vsync_w;

			param.src_info.de_hstart = drmParseDtsInfo->src_info.de_hstart;
			param.src_info.de_vstart = drmParseDtsInfo->src_info.de_vstart;
			param.src_info.de_width = drmParseDtsInfo->src_info.de_width;
			param.src_info.de_height = drmParseDtsInfo->src_info.de_height;
			param.src_info.typ_htt = drmParseDtsInfo->src_info.typ_htt;

			param.src_info.typ_vtt = drmParseDtsInfo->src_info.typ_vtt;
			param.src_info.typ_dclk = drmParseDtsInfo->src_info.typ_dclk;

			param.src_info.op_format = drmParseDtsInfo->src_info.op_format;

			param.out_cfg.linkIF = drmParseDtsInfo->out_cfg.linkIF;
			param.out_cfg.lanes = drmParseDtsInfo->out_cfg.lanes;
			param.out_cfg.div_sec = drmParseDtsInfo->out_cfg.div_sec;
			param.out_cfg.timing = drmParseDtsInfo->out_cfg.timing;
			param.out_cfg.format = drmParseDtsInfo->out_cfg.format;

			param.v_path_en = drmParseDtsInfo->v_path_en;
			param.extdev_path_en = drmParseDtsInfo->extdev_path_en;
			param.gfx_path_en = drmParseDtsInfo->gfx_path_en;
			param.output_model_ref = drmParseDtsInfo->output_model_ref;

			param.video_hprotect_info.hsync_w =
				drmParseDtsInfo->video_hprotect_info.hsync_w;
			param.video_hprotect_info.de_hstart =
				drmParseDtsInfo->video_hprotect_info.de_hstart;
			param.video_hprotect_info.lane_numbers =
				drmParseDtsInfo->video_hprotect_info.lane_numbers;
			param.delta_hprotect_info.hsync_w =
				drmParseDtsInfo->delta_hprotect_info.hsync_w;
			param.delta_hprotect_info.de_hstart =
				drmParseDtsInfo->delta_hprotect_info.de_hstart;
			param.delta_hprotect_info.lane_numbers =
				drmParseDtsInfo->delta_hprotect_info.lane_numbers;
			param.gfx_hprotect_info.hsync_w =
				drmParseDtsInfo->gfx_hprotect_info.hsync_w;
			param.gfx_hprotect_info.de_hstart =
				drmParseDtsInfo->gfx_hprotect_info.de_hstart;
			param.gfx_hprotect_info.lane_numbers =
				drmParseDtsInfo->gfx_hprotect_info.lane_numbers;

			ret |= mtk_tv_drm_check_out_mod_cfg(&pctx->panel_priv, &param);
			ret |= mtk_tv_drm_check_out_lane_cfg(&pctx->panel_priv);
			ret |= mtk_tv_drm_check_video_hbproch_protect(&param);

			output_model_cal = mtk_tv_drm_set_output_model(param.v_path_en,
				param.extdev_path_en, param.gfx_path_en);

			DRM_INFO("[%s][%d] param.output_model_ref = %d!!\n",
				__func__, __LINE__, param.output_model_ref);

			if (output_model_cal == param.output_model_ref)
				ret |= 0;
			else
				ret |= -1;

			DRM_INFO("[%s][%d] set out mod cfg return = %td!!\n",
				__func__, __LINE__, ret);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_VBO_CTRLBIT:
		if ((property_blob != NULL) && (val != NULL)) {
			struct drm_st_ctrlbits stCtrlbits;

			memset(&stCtrlbits, 0, sizeof(struct drm_st_ctrlbits));
			property_blob = drm_property_lookup_blob(property->dev, val);
			memcpy(&stCtrlbits, property_blob->data, sizeof(struct drm_st_ctrlbits));

			mtk_render_set_vbo_ctrlbit(&pctx->panel_priv, &stCtrlbits);

			DRM_INFO("[%s][%d] PNL_VBO_CTRLBIT = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_VBO_TX_MUTE_EN:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_PROP_PNL_VBO_TX_MUTE_EN = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);

		if (property_blob == NULL) {
			DRM_INFO("[%s][%d] unknown tx_mute_info status = %td!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			drm_st_tx_mute_info param;

			memset(&param, 0, sizeof(drm_st_tx_mute_info));
			memcpy(&param, property_blob->data, sizeof(drm_st_tx_mute_info));

			ret = mtk_render_set_tx_mute(&pctx->panel_priv, param);

			DRM_INFO("[%s][%d] set TX mute return = %td!!\n",
				__func__, __LINE__, ret);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SWING_VREG:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_PROP_PNL_SWING_VREG = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
		property_blob = drm_property_lookup_blob(property->dev, val);

		if (property_blob != NULL) {
			struct drm_st_out_swing_level stSwing;

			memset(&stSwing, 0, sizeof(struct drm_st_out_swing_level));
			memcpy(&stSwing, property_blob->data,
				sizeof(struct drm_st_out_swing_level));

			mtk_render_set_swing_vreg(&stSwing);

			DRM_INFO("[%s][%d] PNL_SWING_VREG = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_PRE_EMPHASIS:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_PROP_PNL_PRE_EMPHASIS = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);
			property_blob = drm_property_lookup_blob(property->dev, val);

		if (property_blob != NULL) {
			struct drm_st_out_pe_level stPE;

			memset(&stPE, 0, sizeof(struct drm_st_out_pe_level));
			memcpy(&stPE, property_blob->data, sizeof(struct drm_st_out_pe_level));

			mtk_render_set_pre_emphasis(&stPE);

			DRM_INFO("[%s][%d] PNL_SE = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SSC:
		if (property_blob != NULL) {
			struct drm_st_out_ssc stSSC;

			memset(&stSSC, 0, sizeof(struct drm_st_out_ssc));
			property_blob = drm_property_lookup_blob(property->dev, val);
			memcpy(&stSSC, property_blob->data, sizeof(struct drm_st_out_ssc));

			mtk_render_set_ssc(&stSSC);

			DRM_INFO("[%s][%d] PNL_SSC = %td\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			DRM_INFO("[%s][%d]blob id= 0x%tx, blob is NULL!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_CHECK_DTS_OUTPUT_TIMING:
		DRM_INFO("[%s][%d] E_EXT_CONNECTOR_PROP_PNL_CHECK_DTS_OUTPUT_TIMING = %td\n",
			__func__, __LINE__, (ptrdiff_t)val);

		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob == NULL) {
			DRM_INFO("[%s][%d] unknown output info status = %td!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			drm_st_pnl_info *pStPnlInfo
					= ((drm_st_pnl_info *)(property_blob->data));

			ret = mtk_tv_drm_panel_checkDTSPara(pStPnlInfo);

			DRM_INFO("[%s][%d] mtk_tv_drm_panel_checkDTSPara return = %td!!\n",
				__func__, __LINE__, ret);
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_FRAMESYNC_MLOAD:
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
	struct drm_property_blob *property_blob = NULL;

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
	case E_EXT_CONNECTOR_PROP_PNL_VBO_CTRLBIT:
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SWING_VREG:
		if ((property_blob == NULL) && (val != NULL)) {
			property_blob = drm_property_lookup_blob(property->dev, *val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown swing vreg status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			} else {
				struct drm_st_out_swing_level stSwing;

				memset(&stSwing, 0, sizeof(struct drm_st_out_swing_level));
				memcpy(&stSwing, property_blob->data,
					sizeof(struct drm_st_out_swing_level));
				mtk_render_get_swing_vreg(&stSwing);

				DRM_INFO("[%s][%d] get swing verg status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			}
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_PRE_EMPHASIS:
		if ((property_blob == NULL) && (val != NULL)) {
			property_blob = drm_property_lookup_blob(property->dev, *val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown pre emphasis status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			} else {
				struct drm_st_out_pe_level stPE;

				memset(&stPE, 0, sizeof(struct drm_st_out_pe_level));
				memcpy(&stPE, property_blob->data,
					sizeof(struct drm_st_out_pe_level));
				mtk_render_get_pre_emphasis(&stPE);

				DRM_INFO("[%s][%d] get pre emphasis status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			}
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_SSC:
		if ((property_blob == NULL) && (val != NULL)) {
			property_blob = drm_property_lookup_blob(property->dev, *val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown ssc status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			} else {
				struct drm_st_out_ssc stSSC;

				memset(&stSSC, 0, sizeof(struct drm_st_out_ssc));
				memcpy(&stSSC, property_blob->data,
					sizeof(struct drm_st_out_ssc));
				mtk_render_get_ssc(&stSSC);

				DRM_INFO("[%s][%d] get ssc status = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)*val);
			}
		}
		break;
	case E_EXT_CONNECTOR_PROP_PNL_OUT_MODEL:
		*val = pctx->out_model;
		break;
	case E_EXT_CONNECTOR_PROP_PNL_FRAMESYNC_MLOAD:
		 *val = pctx->panel_priv.outputTiming_info.eFrameSyncState;
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

drm_en_pnl_output_model mtk_tv_drm_set_output_model(bool v_path_en,
						bool extdev_path_en, bool gfx_path_en)
{
	drm_en_pnl_output_model ret = E_PNL_OUT_MODEL_MAX;

	if (v_path_en) {
		if (extdev_path_en && gfx_path_en)
			ret = E_PNL_OUT_MODEL_VG_SEPARATED_W_EXTDEV;

		if (!extdev_path_en && gfx_path_en)
			ret = E_PNL_OUT_MODEL_VG_SEPARATED;

		if (!extdev_path_en && !gfx_path_en)
			ret = E_PNL_OUT_MODEL_VG_BLENDED;
	} else {
		ret = E_PNL_OUT_MODEL_ONLY_EXTDEV;
	}

	return ret;
}

void set_panel_context(struct mtk_tv_kms_context *pctx)
{
	struct device_node *np = NULL;
	bool v_path_en = false;
	bool extdev_path_en = false;
	bool gfx_path_en = false;
	uint8_t out_model_idx = E_EXT_CONNECTOR_PROP_PNL_OUT_MODEL;

	np = of_find_node_by_name(NULL, "video_out");
	if (np != NULL && of_device_is_available(np)) {
		struct mtk_panel_context *pStPanelCtx = NULL;
		struct drm_panel *pDrmPanel = NULL;

		pDrmPanel = of_drm_find_panel(np);
		pStPanelCtx = container_of(pDrmPanel, struct mtk_panel_context, drmPanel);
		memcpy(&pctx->panel_priv, pStPanelCtx, sizeof(struct mtk_panel_context));

		v_path_en = true;
	}

	np = of_find_node_by_name(NULL, "ext_video_out");
	if (np != NULL && of_device_is_available(np))
		extdev_path_en = true;

	np = of_find_node_by_name(NULL, "graphic_out");
	if (np != NULL && of_device_is_available(np))
		gfx_path_en = true;

	pctx->out_model = mtk_tv_drm_set_output_model(v_path_en, extdev_path_en, gfx_path_en);

	pctx->ext_connector_properties->prop_val[out_model_idx] = pctx->out_model;

	drm_mode_crtc_set_gamma_size(&pctx->crtc[MTK_DRM_CRTC_TYPE_VIDEO].base, MTK_GAMMA_LUT_SIZE);

	DRM_INFO("Output Model is %d\n", pctx->out_model);
}

int mtk_tv_drm_video_get_delay_ioctl(struct drm_device *drm_dev,
	void *data, struct drm_file *file_priv)
{
	if (!drm_dev || !data || !file_priv)
		return -EINVAL;

	struct drm_mtk_tv_video_delay *delay = (struct drm_mtk_tv_video_delay *)data;
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

	if (pctx->stub)
		delay->sw_delay_ms = SW_DELAY;
	else
		delay->sw_delay_ms = pctx->swdelay;

	delay->memc_delay_ms = pctx_video->frc_latency; //memc latency
	delay->memc_rw_diff  = pctx_video->frc_rwdiff;

	for (i = 0; i < MAX_WINDOW_NUM; i++) {
		plane_ctx = pctx_video->plane_ctx + i;
		delay->mdgw_delay_ms[i] =
			(VFRQRATIO_1000/pctx_pnl->outputTiming_info.u32OutputVfreq)*
			plane_ctx->rwdiff;
	}
	return 0;
}

