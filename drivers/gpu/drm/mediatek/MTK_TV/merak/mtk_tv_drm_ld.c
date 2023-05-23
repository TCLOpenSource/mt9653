// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/mtk_tv_drm.h>
#include <linux/pinctrl/consumer.h>
#include "mtk_tv_drm_plane.h"
//#include "mtk_tv_drm_utpa_wrapper.h"
#include "mtk_tv_drm_drv.h"
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_ld.h"
#include "mtk_tv_drm_encoder.h"
//#include "mtk_tv_drm_osdp_plane.h"
#include "mtk_tv_drm_kms.h"

#include "ext_command_render_if.h"

#include "pqu_msg.h"
#include "apiLDM.h"
#include "drvLDM_sti.h"

//#define MTK_DRM_LD_COMPATIBLE_STR "MTK-drm-ld"
#define MTK_DRM_DTS_LD_SUPPORT_TAG "LDM_SUPPORT"
#define MTK_DRM_DTS_LD_MMAP_ADDR_TAG "LDM_MMAP_ADDR"
#define MTK_DRM_DTS_LD_VERSION_TAG "MX_LDM_VERSION"

#define MTK_DRM_DTS_MMAP_INFO_LD_MMAP_ADDR_TAG "MI_DISPOUT_LOCAL_DIMMING"
#define MTK_DRM_DTS_MMAP_INFO_LD_MMAP_ADDR_REG_TAG "reg"

#define BACKLIGHT 100


static struct pqu_render_ldm_param ldm_param;
static struct pqu_render_ldm_reply_param ldm_reply_param;




__u32 get_dts_u32_ldm(struct device_node *np, const char *name)
{
	__u32 u32Tmp = 0x0;
	int ret;

	ret = of_property_read_u32(np, name, &u32Tmp);
	if (ret != 0x0) {

		DRM_INFO(
		"[%s, %d]: LDM of_property_read_u32 failed, name = %s \r\n",
		__func__, __LINE__,
		name);
	}

	return u32Tmp;
}

__u32 get_dts_array_u32_ldm(struct device_node *np,
	const char *name,
	__u32 *ret_array,
	int ret_array_length)
{
	int ret, i;
	__u32 *u32TmpArray = NULL;

	u32TmpArray = kmalloc_array(ret_array_length, sizeof(__u32), GFP_KERNEL);
	ret = of_property_read_u32_array(np, name, u32TmpArray, ret_array_length);
	if (ret != 0x0) {
		kfree(u32TmpArray);
		return ret;
	}
	for (i = 0; i < ret_array_length; i++)
		*(ret_array + i) = u32TmpArray[i];
	kfree(u32TmpArray);
	return 0;
}

static void dump_dts_ld_info(struct mtk_ld_context *pctx)
{
	int i;

	DRM_INFO("================LDM INFO================\n");
	DRM_INFO("u16PanelWidth = %d\n", pctx->ld_panel_info.u16PanelWidth);
	DRM_INFO("u16PanelHeight = %d\n", pctx->ld_panel_info.u16PanelHeight);
	DRM_INFO("eLEDType = %d\n", pctx->ld_panel_info.eLEDType);
	DRM_INFO("u8LDFWidth = %d\n", pctx->ld_panel_info.u8LDFWidth);
	DRM_INFO("u8LDFHeight = %d\n", pctx->ld_panel_info.u8LDFHeight);
	DRM_INFO("u8LEDWidth = %d\n", pctx->ld_panel_info.u8LEDWidth);
	DRM_INFO("u8LEDHeight = %d\n", pctx->ld_panel_info.u8LEDHeight);
	DRM_INFO("u8LSFWidth = %d\n", pctx->ld_panel_info.u8LSFWidth);
	DRM_INFO("u8LSFHeight = %d\n", pctx->ld_panel_info.u8LSFHeight);
	DRM_INFO("u8Edge2D_Local_h = %d\n", pctx->ld_panel_info.u8Edge2D_Local_h);
	DRM_INFO("u8Edge2D_Local_v = %d\n", pctx->ld_panel_info.u8Edge2D_Local_v);
	DRM_INFO("u8PanelHz = %d\n", pctx->ld_panel_info.u8PanelHz);


	DRM_INFO("u32MarqueeDelay = %d\n", pctx->ld_misc_info.u32MarqueeDelay);
	DRM_INFO("bLDEn = %d\n", pctx->ld_misc_info.bLDEn);
	DRM_INFO("bOLEDEn = %d\n", pctx->ld_misc_info.bOLEDEn);
	DRM_INFO("bOLED_LSP_En = %d\n", pctx->ld_misc_info.bOLED_LSP_En);
	DRM_INFO("bOLED_GSP_En = %d\n", pctx->ld_misc_info.bOLED_GSP_En);
	DRM_INFO("bOLED_HBP_En = %d\n", pctx->ld_misc_info.bOLED_HBP_En);
	DRM_INFO("bOLED_CRP_En = %d\n", pctx->ld_misc_info.bOLED_CRP_En);
	DRM_INFO("bOLED_LSPHDR_En = %d\n", pctx->ld_misc_info.bOLED_LSPHDR_En);
	DRM_INFO("u8SPIBits = %d\n", pctx->ld_misc_info.u8SPIBits);
	DRM_INFO("u8ClkHz = %d\n", pctx->ld_misc_info.u8ClkHz);
	DRM_INFO("u8MirrorPanel = %d\n", pctx->ld_misc_info.u8MirrorPanel);


	DRM_INFO("u8MspiChanel = %d\n", pctx->ld_mspi_info.u8MspiChanel);
	DRM_INFO("u8MspiMode = %d\n", pctx->ld_mspi_info.u8MspiMode);
	DRM_INFO("u8TrStart = %d\n", pctx->ld_mspi_info.u8TrStart);
	DRM_INFO("u8TrEnd = %d\n", pctx->ld_mspi_info.u8TrEnd);
	DRM_INFO("u8TB = %d\n", pctx->ld_mspi_info.u8TB);
	DRM_INFO("u8TRW = %d\n", pctx->ld_mspi_info.u8TRW);
	DRM_INFO("u32MspiClk = %d\n", pctx->ld_mspi_info.u32MspiClk);
	DRM_INFO("BClkPolarity = %d\n", pctx->ld_mspi_info.BClkPolarity);
	DRM_INFO("BClkPhase = %d\n", pctx->ld_mspi_info.BClkPhase);
	DRM_INFO("u32MAXClk = %d\n", pctx->ld_mspi_info.u32MAXClk);
	DRM_INFO("u8MspiBuffSizes = 0x%x\n", pctx->ld_mspi_info.u8MspiBuffSizes);

	for (i = 0; i < W_BIT_CONFIG_NUM; i++)
		DRM_INFO("u8WBitConfig[%d]  0x%x \r\n",
	i, pctx->ld_mspi_info.u8WBitConfig[i]);

	for (i = 0; i < R_BIT_CONFIG_NUM; i++)
		DRM_INFO("u8RBitConfig[%d]  0x%x \r\n",
		i, pctx->ld_mspi_info.u8RBitConfig[i]);


	DRM_INFO("u8LDMATrimode = 0x%x\n", pctx->ld_dma_info.u8LDMATrimode);
	DRM_INFO("u8LDMACheckSumMode = 0x%x\n", pctx->ld_dma_info.u8LDMACheckSumMode);
	DRM_INFO("u8cmdlength = 0x%x\n", pctx->ld_dma_info.u8cmdlength);
	DRM_INFO("u8BLWidth = 0x%x\n", pctx->ld_dma_info.u8BLWidth);
	DRM_INFO("u8BLHeight = 0x%x\n", pctx->ld_dma_info.u8BLHeight);
	DRM_INFO("u16LedNum = 0x%x\n", pctx->ld_dma_info.u16LedNum);
	DRM_INFO("u16DMAPackLength = %d\n", pctx->ld_dma_info.u16DMAPackLength);
	DRM_INFO("u8DataInvert = 0x%x\n", pctx->ld_dma_info.u8DataInvert);
	DRM_INFO("u8ExtDataLength = 0x%x\n", pctx->ld_dma_info.u8ExtDataLength);
	DRM_INFO("u16BDACNum = %d\n", pctx->ld_dma_info.u16BDACNum);
	DRM_INFO("u8BDACcmdlength = 0x%x\n", pctx->ld_dma_info.u8BDACcmdlength);

	for (i = 0; i < MSPI_HEAD_NUM; i++)
		DRM_INFO("u16MspiHead[%d]  0x%x \r\n",
		i, pctx->ld_dma_info.u16MspiHead[i]);

	for (i = 0; i < DMA_DELAY_NUM; i++)
		DRM_INFO("u16DMADelay[%d]  0x%x \r\n",
		i, pctx->ld_dma_info.u16DMADelay[i]);

	for (i = 0; i < EXT_DATA_NUM; i++)
		DRM_INFO("u8ExtData[%d]  0x%x \r\n",
		i, pctx->ld_dma_info.u8ExtData[i]);

	for (i = 0; i < BDAC_HEAD_NUM; i++)
		DRM_INFO("u16BDACHead[%d]  0x%x \r\n",
		i, pctx->ld_dma_info.u16BDACHead[i]);

	DRM_INFO("u16Rsense = %d\n", pctx->ld_led_device_info.u16Rsense);
	DRM_INFO("u16VDAC_MBR_OFF = 0x%x\n", pctx->ld_led_device_info.u16VDAC_MBR_OFF);
	DRM_INFO("u16VDAC_MBR_NO = 0x%x\n", pctx->ld_led_device_info.u16VDAC_MBR_NO);
	DRM_INFO("u16BDAC_High = 0x%x\n", pctx->ld_led_device_info.u16BDAC_High);
	DRM_INFO("u16BDAC_Low = 0x%x\n", pctx->ld_led_device_info.u16BDAC_Low);
	DRM_INFO("u16SPINUM = %d\n", pctx->ld_led_device_info.u16SPINUM);
	DRM_INFO("bAS3824 = %d\n", pctx->ld_led_device_info.bAS3824);
	DRM_INFO("bNT50585 = %d\n", pctx->ld_led_device_info.bNT50585);
	DRM_INFO("bIW7039 = %d\n", pctx->ld_led_device_info.bIW7039);
	DRM_INFO("bMCUswmode = %d\n", pctx->ld_led_device_info.bMCUswmode);
	DRM_INFO("u8SPIDutybit = %d\n", pctx->ld_led_device_info.u8SPIDutybit);

	DRM_INFO("================LDM INFO END================\n");
}

static int _mtk_ldm_select_pins_to_func(struct mtk_tv_kms_context *pctx, uint8_t val)
{
	struct pinctrl *pinctrl;
	struct pinctrl_state *default_state;
	int ret = 0;

	pinctrl = devm_pinctrl_get(pctx->dev);
	if (IS_ERR(pinctrl)) {
		pinctrl = NULL;
		DRM_INFO("[DRM][LDM][%s][%d] Cannot find pinctrl!\n",
		__func__, __LINE__);
		return 1;
	}
	if (pinctrl) {
		if (val == 1)
			default_state = pinctrl_lookup_state(pinctrl, "fun_ldc_tmon_pmux");
		else
			default_state = pinctrl_lookup_state(pinctrl, "fun_ldc_tmon_gpio_pmux");
		if (IS_ERR(default_state)) {
			default_state = NULL;
			DRM_INFO("[DRM][LDM][%s][%d] Cannot find pinctrl_state!\n",
			__func__, __LINE__);
			return 1;
		}
		if (default_state)
			ret = pinctrl_select_state(pinctrl, default_state);
	}

	return ret;
}

int mtk_ld_atomic_set_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t val,
	int prop_index)
{
	// int ret = 0;
	int ret_api = 0;
	struct drm_property_blob *property_blob;
	struct mtk_tv_kms_context *pctx;
	struct ext_crtc_prop_info *crtc_props;
	struct mtk_ld_context *pctx_ld;
	struct hwregSWSetCtrlIn stApiSWSetCtrl;

	pctx = (struct mtk_tv_kms_context *)crtc->crtc_private;
	crtc_props = pctx->ext_crtc_properties;
	pctx_ld = &pctx->ld_priv;



	if (pctx_ld->u8LDMSupport == E_LDM_SUPPORT_TRUNK) {
		switch (prop_index) {
		case E_EXT_CRTC_PROP_LDM_SW_REG_CTRL:
		{
		memset(
		&stApiSWSetCtrl,
		0,
		sizeof(struct hwregSWSetCtrlIn));

		property_blob = drm_property_lookup_blob(property->dev, val);
		if (property_blob == NULL) {
			DRM_INFO("[%s][%d] unknown SW_REG_CTRL = %td!!\n",
				__func__, __LINE__, (ptrdiff_t)val);
		} else {
			memcpy(
			&stApiSWSetCtrl,
			property_blob->data,
			sizeof(struct hwregSWSetCtrlIn));
		if (stApiSWSetCtrl.enLDMSwSetCtrlType == E_LDM_SW_SET_LD_SUPPORT) {
			pctx_ld->u8LDMSupport = stApiSWSetCtrl.u8Value;
			ret_api = MApi_ld_Init_SetLDC(stApiSWSetCtrl.u8Value);
		} else {
			ret_api = MApi_ld_Init_SetSwSetCtrl(&stApiSWSetCtrl);
		}
		}
		break;
		}
		case E_EXT_CRTC_PROP_LDM_STATUS:
		{
		DRM_INFO("[DRM][LDM][%s][%d] set LDM_STATUSe = %td\n",
			__func__, __LINE__,
			(ptrdiff_t)crtc_props->prop_val[prop_index]);
			break;
		}
		case E_EXT_CRTC_PROP_LDM_LUMA:
		DRM_INFO("[DRM][LDM][%s][%d] set LDM_LUMA = %td\n",
			__func__, __LINE__,
			(ptrdiff_t)crtc_props->prop_val[prop_index]);
			break;
		case E_EXT_CRTC_PROP_LDM_ENABLE:
		{

		DRM_INFO("[DRM][LDM][%s][%d] set LDM_ENABLE or Disable = %td\n",
			__func__, __LINE__,
			(ptrdiff_t)crtc_props->prop_val[prop_index]);
////remove-start trunk ld need refactor
//		if (crtc_props->prop_val[prop_index]) {
//			ret_api = MApi_LDM_Enable();
//			DRM_INFO("[DRM][LDM] set LDM_ENABLE  %d\n", ret_api);
//		} else {
//			ret_api = MApi_LDM_Disable(
//				(__u8)crtc_props->prop_val[prop_index-1]);
//
//			DRM_INFO("[DRM][LDM] set LDM_Disable ,luma=%tx\n",
//				(ptrdiff_t)crtc_props->prop_val[prop_index-1]);
//		}
////remove-end trunk ld need refactor

			break;
		}
		case E_EXT_CRTC_PROP_LDM_STRENGTH:
		{
		DRM_INFO("[DRM][LDM][%s][%d] set LDM_STRENGTH = %td\n",
			__func__, __LINE__,
			(ptrdiff_t)crtc_props->prop_val[prop_index]);
////remove-start trunk ld need refactor
//		ret_api = MApi_LDM_SetStrength(
//			(__u8)crtc_props->prop_val[prop_index]);
////remove-end trunk ld need refactor

			break;
		}
		case E_EXT_CRTC_PROP_LDM_DATA:
		{
////remove-start trunk ld need refactor
//		ST_LDM_GET_DATA stApiLDMData;
//
//
//		memset(
//			&stApiLDMData,
//			0,
//			sizeof(ST_LDM_GET_DATA));
//
//		property_blob = drm_property_lookup_blob(property->dev, val);
//
//		memcpy(
//			&stApiLDMData,
//			property_blob->data,
//			sizeof(ST_LDM_GET_DATA));
//
//		ret_api =  MApi_LDM_GetData(&stApiLDMData);
//
//		memcpy(
//			property_blob->data,
//			&stApiLDMData,
//			sizeof(ST_LDM_GET_DATA));
//
//		DRM_INFO(
//			"[DRM][LDM][%s][%d] set LDM_DATA = %d  %tx	%d %td\n",
//			__func__, __LINE__,
//			stApiLDMData.enDataType,
//			(ptrdiff_t)stApiLDMData.phyAddr,
//			ret_api,
//			(ptrdiff_t)val);
////remove-end trunk ld need refactor

			break;
		}
		case E_EXT_CRTC_PROP_LDM_DEMO_PATTERN:
		{
////remove-start trunk ld need refactor
//		ret_api = MApi_LDM_SetDemoPattern(
//			(__u32)crtc_props->prop_val[prop_index]);
//
//		DRM_INFO(
//			"[DRM][LDM][%s][%d] set DEMO_PATTERN = %td\n",
//			__func__, __LINE__,
//			(ptrdiff_t)crtc_props->prop_val[prop_index]);
////remove-start trunk ld need refactor

			break;
		}
		case E_EXT_CRTC_PROP_LDM_INIT:
		{
			DRM_INFO("[DRM][LDM][%s][%d] set LDM_INIT = %td\n",
			__func__, __LINE__,
			(ptrdiff_t)crtc_props->prop_val[prop_index]);
			ret_api = MApi_ld_Init(pctx_ld->u64LDM_phyAddress, pctx);
			//ret_api = MApi_ld_Init_SetLDC(pctx_ld->u8LDMSupport);
			DRM_INFO("[DRM][LDM] set LDM_INIT  %d\n", ret_api);
			break;
		}
		default:
			break;
		}

		return ret_api;

	} else if (pctx_ld->u8LDMSupport == E_LDM_SUPPORT_CUS) {
		switch (prop_index) {
		case E_EXT_CRTC_PROP_LDM_SW_REG_CTRL:
			{

			memset(
				&stApiSWSetCtrl,
				0,
				sizeof(struct hwregSWSetCtrlIn));

			property_blob = drm_property_lookup_blob(property->dev, val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown SW_REG_CTRL = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)val);
			} else {
				memcpy(
				&stApiSWSetCtrl,
				property_blob->data,
				sizeof(struct hwregSWSetCtrlIn));
			if (stApiSWSetCtrl.enLDMSwSetCtrlType == E_LDM_SW_SET_LD_SUPPORT) {
				pctx_ld->u8LDMSupport = stApiSWSetCtrl.u8Value;
				ret_api = MApi_ld_Init_SetLDC(stApiSWSetCtrl.u8Value);
			} else if (stApiSWSetCtrl.enLDMSwSetCtrlType == E_LDM_SW_SET_CUS_PROFILE) {
				ret_api = MApi_ld_Init_UpdateProfile(
							pctx_ld->u64LDM_phyAddress,
							pctx);
			} else if (stApiSWSetCtrl.enLDMSwSetCtrlType == E_LDM_SW_SET_TMON_DEBUG) {
				ret_api = _mtk_ldm_select_pins_to_func(
							pctx,
							stApiSWSetCtrl.u8Value
							);

			} else {
				ret_api = MApi_ld_Init_SetSwSetCtrl(&stApiSWSetCtrl);
			}
			}
			break;
		}
		case E_EXT_CRTC_PROP_LDM_STRENGTH:
		case E_EXT_CRTC_PROP_LDM_AUTO_LD:
		case E_EXT_CRTC_PROP_LDM_XTendedRange:
		{
			ldm_param.u8ld_support_type = pctx_ld->u8LDMSupport;
			ldm_param.u8auto_ld = crtc_props->prop_val[E_EXT_CRTC_PROP_LDM_AUTO_LD];
			ldm_param.u8xtended_range =
				crtc_props->prop_val[E_EXT_CRTC_PROP_LDM_XTendedRange];
			ldm_param.u8backlight = crtc_props->prop_val[E_EXT_CRTC_PROP_LDM_STRENGTH];

			//ret_api = pqu_render_ldm(&ldm_param, &ldm_reply_param);
			DRM_INFO(
			"[DRM][LDM][%s][%d] UI reply %d %x !!\n",
			__func__,
			__LINE__,
			prop_index,
			ldm_reply_param.ret);
			break;

		}
		case E_EXT_CRTC_PROP_MAX:
			DRM_INFO(
			"[DRM][LDM][%s][%d] invalid property!!\n",
			__func__, __LINE__);
			break;
		default:
			break;


		}

		return ret_api;

	} else {

		switch (prop_index) {
		case E_EXT_CRTC_PROP_LDM_SW_REG_CTRL:
			{
			memset(
				&stApiSWSetCtrl,
				0,
				sizeof(struct hwregSWSetCtrlIn));

			property_blob = drm_property_lookup_blob(property->dev, val);
			if (property_blob == NULL) {
				DRM_INFO("[%s][%d] unknown SW_REG_CTRL = %td!!\n",
					__func__, __LINE__, (ptrdiff_t)val);
			} else {
				memcpy(
				&stApiSWSetCtrl,
				property_blob->data,
				sizeof(struct hwregSWSetCtrlIn));
			if (stApiSWSetCtrl.enLDMSwSetCtrlType == E_LDM_SW_SET_LD_SUPPORT) {
				pctx_ld->u8LDMSupport = stApiSWSetCtrl.u8Value;
				ret_api = MApi_ld_Init_SetLDC(stApiSWSetCtrl.u8Value);
			} else if (stApiSWSetCtrl.enLDMSwSetCtrlType == E_LDM_SW_SET_CUS_PROFILE) {
				ret_api = MApi_ld_Init_UpdateProfile(
							pctx_ld->u64LDM_phyAddress,
							pctx);
			} else {
				ret_api = MApi_ld_Init_SetSwSetCtrl(&stApiSWSetCtrl);
			}
			}
			break;
		}
		case E_EXT_CRTC_PROP_LDM_AUTO_LD:
			break;
		case E_EXT_CRTC_PROP_LDM_XTendedRange:
			break;

		default:
			break;


		}

		return ret_api;

	}
}

int mtk_ld_atomic_get_crtc_property(
	struct mtk_tv_drm_crtc *crtc,
	const struct drm_crtc_state *state,
	struct drm_property *property,
	uint64_t *val,
	int prop_index)
{
	struct mtk_tv_kms_context *pctx;
	struct ext_crtc_prop_info *crtc_props;
	int ret = 0;
	struct mtk_ld_context *pctx_ld;

	pctx = (struct mtk_tv_kms_context *)crtc->crtc_private;
	crtc_props = pctx->ext_crtc_properties;
	pctx_ld = &pctx->ld_priv;


	if (pctx_ld->u8LDMSupport == E_LDM_SUPPORT_TRUNK) {


		switch (prop_index) {
		case E_EXT_CRTC_PROP_LDM_STATUS:
		{
////remove-start trunk ld need refactor
//		crtc_props->prop_val[prop_index] = MApi_LDM_GetStatus();
//
//		DRM_INFO(
//			"[DRM][LDM][%s][%d] get LDM_STATUS = %td\n",
//			__func__, __LINE__,
//			(ptrdiff_t)crtc_props->prop_val[prop_index]);
////remove-end trunk ld need refactor

		break;
		}
		case E_EXT_CRTC_PROP_MAX:
		DRM_INFO("[DRM][LDM][%s][%d] invalid property!!\n",
			__func__, __LINE__);
		break;
		default:
		break;
		}

		return ret;
	} else if (pctx_ld->u8LDMSupport == E_LDM_SUPPORT_CUS) {

		return ret;

	} else {

		return 0;
	}

}


int _parse_ldm_led_device_info(struct mtk_ld_context *pctx)
{
	int ret = 0, i = 0;
	struct device_node *np;
	//const char *name;
	//__u32 *u32TmpArray = NULL;


	//Get ldm LED DEVICE info
	np = of_find_node_by_name(NULL, LDM_LED_DEVICE_INFO_NODE);
	if (np != NULL && of_device_is_available(np)) {

		pctx->ld_led_device_info.u16Rsense = get_dts_u32_ldm(np, RSENSE_TAG);
		pctx->ld_led_device_info.u16VDAC_MBR_OFF = get_dts_u32_ldm(np, VDAC_MBR_OFF_TAG);
		pctx->ld_led_device_info.u16VDAC_MBR_NO = get_dts_u32_ldm(np, VDAC_MBR_ON_TAG);
		pctx->ld_led_device_info.u16BDAC_High = get_dts_u32_ldm(np, BDAC_HIGH_TAG);
		pctx->ld_led_device_info.u16BDAC_Low = get_dts_u32_ldm(np, BDAC_LOW_TAG);
		pctx->ld_led_device_info.u16SPINUM = get_dts_u32_ldm(np, SPI_NUM_TAG);
		pctx->ld_led_device_info.bAS3824 = get_dts_u32_ldm(np, AS3824_TAG);
		pctx->ld_led_device_info.bNT50585 = get_dts_u32_ldm(np, NT50585_TAG);
		pctx->ld_led_device_info.bIW7039 = get_dts_u32_ldm(np, IW_7039_TAG);
		pctx->ld_led_device_info.bMCUswmode = get_dts_u32_ldm(np, MCU_SW_MODE_TAG);
		pctx->ld_led_device_info.u8SPIDutybit = get_dts_u32_ldm(np, SPI_DUTY_BIT_TAG);

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
		__func__, __LINE__, LDM_LED_DEVICE_INFO_NODE);
		ret = -ENODATA;
	}

	return ret;
}


int _parse_ldm_dma_info(struct mtk_ld_context *pctx)
{
	int ret = 0, i = 0;
	struct device_node *np;
	const char *name;
	__u32 *u32TmpArray = NULL;


	//Get ldm DMA info
	np = of_find_node_by_name(NULL, LDM_DMA_INFO_NODE);
	if (np != NULL && of_device_is_available(np)) {

		pctx->ld_dma_info.u8LDMAchanel = get_dts_u32_ldm(np, LDMA_CHANEL_TAG);
		pctx->ld_dma_info.u8LDMATrimode = get_dts_u32_ldm(np, LDMA_TRIMODE_TAG);
		pctx->ld_dma_info.u8LDMACheckSumMode = get_dts_u32_ldm(np, LDMA_CHECK_SUM_MODE_TAG);
		pctx->ld_dma_info.u8cmdlength = get_dts_u32_ldm(np, CMD_LENGTH_TAG);
		pctx->ld_dma_info.u8BLWidth = get_dts_u32_ldm(np, BL_WIDTH_TAG);
		pctx->ld_dma_info.u8BLHeight = get_dts_u32_ldm(np, BL_HEIGHT_TAG);
		pctx->ld_dma_info.u16LedNum = get_dts_u32_ldm(np, LED_NUM_TAG);
		pctx->ld_dma_info.u8DataPackMode = get_dts_u32_ldm(np, DATA_PACK_MODE_TAG);
		pctx->ld_dma_info.u16DMAPackLength = get_dts_u32_ldm(np, DMA_PACK_LENGTH_TAG);
		pctx->ld_dma_info.u8DataInvert = get_dts_u32_ldm(np, DATA_INVERT_TAG);
		pctx->ld_dma_info.u32DMABaseOffset = get_dts_u32_ldm(np, DMA_BASE_OFFSET_TAG);
		pctx->ld_dma_info.u8ExtDataLength = get_dts_u32_ldm(np, EXT_DATA_LENGTH_TAG);
		pctx->ld_dma_info.u16BDACNum = get_dts_u32_ldm(np, BDAC_NUM_TAG);
		pctx->ld_dma_info.u8BDACcmdlength = get_dts_u32_ldm(np, BDAC_CMD_LENGTH_TAG);


		name = MSPI_HEAD_TAG;
		u32TmpArray = kmalloc(sizeof(__u32) * MSPI_HEAD_NUM, GFP_KERNEL);
		ret = get_dts_array_u32_ldm(np, name, u32TmpArray, MSPI_HEAD_NUM);
		for (i = 0; i < MSPI_HEAD_NUM; i++)
			pctx->ld_dma_info.u16MspiHead[i] = u32TmpArray[i];
		kfree(u32TmpArray);

		name = DMA_DELAY_TAG;
		u32TmpArray = kmalloc(sizeof(__u32) * DMA_DELAY_NUM, GFP_KERNEL);
		ret = get_dts_array_u32_ldm(np, name, u32TmpArray, DMA_DELAY_NUM);
		for (i = 0; i < DMA_DELAY_NUM; i++)
			pctx->ld_dma_info.u16DMADelay[i] = u32TmpArray[i];
		kfree(u32TmpArray);

		name = EXT_DATA_TAG;
		u32TmpArray = kmalloc(sizeof(__u32) * EXT_DATA_NUM, GFP_KERNEL);
		ret = get_dts_array_u32_ldm(np, name, u32TmpArray, EXT_DATA_NUM);
		for (i = 0; i < EXT_DATA_NUM; i++)
			pctx->ld_dma_info.u8ExtData[i] = u32TmpArray[i];
		kfree(u32TmpArray);

		name = BDAC_HEAD_TAG;
		u32TmpArray = kmalloc(sizeof(__u32) * BDAC_HEAD_NUM, GFP_KERNEL);
		ret = get_dts_array_u32_ldm(np, name, u32TmpArray, BDAC_HEAD_NUM);
		for (i = 0; i < BDAC_HEAD_NUM; i++)
			pctx->ld_dma_info.u16BDACHead[i] = u32TmpArray[i];
		kfree(u32TmpArray);

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
		__func__, __LINE__, LDM_DMA_INFO_NODE);
		ret = -ENODATA;

	}

	return ret;
}


int _parse_ldm_mspi_info(struct mtk_ld_context *pctx)
{
	int ret = 0;
	struct device_node *np;
	const char *name;
	int i;
	__u32 *u32TmpArray = NULL;


	//Get ldm MSPI info
	np = of_find_node_by_name(NULL, LDM_MSPI_INFO_NODE);
	if (np != NULL && of_device_is_available(np)) {

		pctx->ld_mspi_info.u8MspiChanel = get_dts_u32_ldm(np, MSPI_CHANEL_TAG);
		pctx->ld_mspi_info.u8MspiMode = get_dts_u32_ldm(np, MSPI_MODE_TAG);
		pctx->ld_mspi_info.u8TrStart = get_dts_u32_ldm(np, TR_START_TAG);
		pctx->ld_mspi_info.u8TrEnd = get_dts_u32_ldm(np, TR_END_TAG);
		pctx->ld_mspi_info.u8TB = get_dts_u32_ldm(np, TB_TAG);
		pctx->ld_mspi_info.u8TRW = get_dts_u32_ldm(np, TRW_TAG);
		pctx->ld_mspi_info.u32MspiClk = get_dts_u32_ldm(np, MSPI_CLK_TAG);
		pctx->ld_mspi_info.BClkPolarity = get_dts_u32_ldm(np, BCLKPOLARITY_TAG);
		pctx->ld_mspi_info.BClkPhase = get_dts_u32_ldm(np, BCLKPHASE_TAG);
		pctx->ld_mspi_info.u32MAXClk = get_dts_u32_ldm(np, MAX_CLK_TAG);
		pctx->ld_mspi_info.u8MspiBuffSizes = get_dts_u32_ldm(np, MSPI_BUFFSIZES_TAG);

		name = W_BIT_CONFIG_TAG;
		u32TmpArray = kmalloc(sizeof(__u32) * W_BIT_CONFIG_NUM, GFP_KERNEL);
		ret = get_dts_array_u32_ldm(np, name, u32TmpArray, W_BIT_CONFIG_NUM);
		for (i = 0; i < W_BIT_CONFIG_NUM; i++)
			pctx->ld_mspi_info.u8WBitConfig[i] = u32TmpArray[i];
		kfree(u32TmpArray);

		name = R_BIT_CONFIG_TAG;
		u32TmpArray = kmalloc(sizeof(__u32) * R_BIT_CONFIG_NUM, GFP_KERNEL);
		ret = get_dts_array_u32_ldm(np, name, u32TmpArray, R_BIT_CONFIG_NUM);
		for (i = 0; i < R_BIT_CONFIG_NUM; i++)
			pctx->ld_mspi_info.u8RBitConfig[i] = u32TmpArray[i];
		kfree(u32TmpArray);

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
		__func__, __LINE__, LDM_MSPI_INFO_NODE);
		ret = -ENODATA;

	}

	return ret;
}


int _parse_ldm_misc_info(struct mtk_ld_context *pctx)
{
	int ret = 0;
	struct device_node *np;
	const char *name;
	//int i;
	//__u32 u32Tmp = 0x0;
	//__u32 *u32TmpArray = NULL;


	//Get ldm Misc info
	np = of_find_node_by_name(NULL, LDM_MISC_INFO_NODE);
	if (np != NULL && of_device_is_available(np)) {

		pctx->ld_misc_info.u32MarqueeDelay = get_dts_u32_ldm(np, MARQUEE_DELAY_TAG);
		pctx->ld_misc_info.bLDEn = get_dts_u32_ldm(np, LDEN_TAG);
		pctx->ld_misc_info.bOLEDEn = get_dts_u32_ldm(np, OLED_EN_TAG);
		pctx->ld_misc_info.bOLED_LSP_En = get_dts_u32_ldm(np, OLED_LSP_EN_TAG);
		pctx->ld_misc_info.bOLED_GSP_En = get_dts_u32_ldm(np, OLED_GSP_EN_TAG);
		pctx->ld_misc_info.bOLED_HBP_En = get_dts_u32_ldm(np, OLED_HBP_EN_TAG);
		pctx->ld_misc_info.bOLED_CRP_En = get_dts_u32_ldm(np, OLED_CRP_EN_TAG);
		pctx->ld_misc_info.bOLED_LSPHDR_En = get_dts_u32_ldm(np, OLED_LSPHDR_EN_TAG);
		pctx->ld_misc_info.u8SPIBits = get_dts_u32_ldm(np, SPIBITS_TAG);
		pctx->ld_misc_info.u8ClkHz = get_dts_u32_ldm(np, CLKHZ_TAG);
		pctx->ld_misc_info.u8MirrorPanel = get_dts_u32_ldm(np, MIRROR_PANEL_TAG);

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
		__func__, __LINE__, LDM_MISC_INFO_NODE);
		ret = -ENODATA;

	}

	return ret;
}

int _parse_ldm_panel_info(struct mtk_ld_context *pctx)
{
	int ret = 0;
	struct device_node *np;
	const char *name;
	//int i;
	//__u32 *u32TmpArray = NULL;

	//Get ldm Panel info
	np = of_find_node_by_name(NULL, LDM_PANEL_INFO_NODE);
	if (np != NULL && of_device_is_available(np)) {

		pctx->ld_panel_info.u16PanelWidth = get_dts_u32_ldm(np, PANEL_WIDTH_TAG);
		pctx->ld_panel_info.u16PanelHeight = get_dts_u32_ldm(np, PANEL_HEIGHT_TAG);
		pctx->ld_panel_info.eLEDType = get_dts_u32_ldm(np, LED_TYPE_TAG);
		pctx->ld_panel_info.u8LDFWidth = get_dts_u32_ldm(np, LDF_WIDTH_TAG);
		pctx->ld_panel_info.u8LDFHeight = get_dts_u32_ldm(np, LDF_HEIGHT_TAG);
		pctx->ld_panel_info.u8LEDWidth = get_dts_u32_ldm(np, LED_WIDTH_TAG);
		pctx->ld_panel_info.u8LEDHeight = get_dts_u32_ldm(np, LED_HEIGHT_TAG);
		pctx->ld_panel_info.u8LSFWidth = get_dts_u32_ldm(np, LSF_WIDTH_TAG);
		pctx->ld_panel_info.u8LSFHeight = get_dts_u32_ldm(np, LSF_HEIGHT_TAG);
		pctx->ld_panel_info.u8Edge2D_Local_h = get_dts_u32_ldm(np, EDGE2D_LOCAL_H_TAG);
		pctx->ld_panel_info.u8Edge2D_Local_v = get_dts_u32_ldm(np, EDGE2D_LOCAL_V_TAG);
		pctx->ld_panel_info.u8PanelHz = get_dts_u32_ldm(np, PANEL_TAG);

	} else {
		DRM_INFO("[%s, %d]: find node failed, name = %s\n",
		__func__, __LINE__, LDM_PANEL_INFO_NODE);
		ret = -ENODATA;
	}

	return ret;
}


int readDTB2LDMPrivate(struct mtk_ld_context *pctx)
{
	int ret;
	struct device_node *np;
	__u32 u32Tmp = 0x0;
	const char *name;
	//int *tmpArray = NULL, i;
	__u32 *u32TmpArray = NULL;
	int i;

	np = of_find_compatible_node(NULL, NULL, LDM_COMPATIBLE_STR);

	if (np != NULL) {
		name = MTK_DRM_DTS_LD_SUPPORT_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {

			DRM_INFO(
				"[%s, %d]: LDM of_property_read_u8 failed, name = %s \r\n",
				__func__, __LINE__,
				name);
			return ret;
		}
		pctx->u8LDMSupport = u32Tmp;

		DRM_INFO(
			"LDM_support=%x %x \r\n",
			u32Tmp, pctx->u8LDMSupport);

		name = MTK_DRM_DTS_LD_MMAP_ADDR_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);

		if (ret != 0x0) {

			DRM_INFO(
				"[%s, %d]: of_property_read_u32 failed, name = %s \r\n",
				__func__, __LINE__,
				name);
			return ret;
		}

		pctx->u64LDM_phyAddress = (__u64)u32Tmp;
		DRM_INFO("LDM_phyaddress=%x  %tx \r\n",
			u32Tmp, (ptrdiff_t)pctx->u64LDM_phyAddress);

		name = MTK_DRM_DTS_LD_VERSION_TAG;
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0) {

			DRM_INFO(
				"[%s, %d]: LDM of_property_read_u8 failed, name = %s \r\n",
				__func__, __LINE__,
				name);
			return ret;
		}
		pctx->u8LDM_Version = u32Tmp;
		DRM_INFO(
			"LDM_Version=%x %x \r\n",
			u32Tmp, pctx->u8LDM_Version);
	}

	np = of_find_node_by_name(NULL, MTK_DRM_DTS_MMAP_INFO_LD_MMAP_ADDR_TAG);
	if (np != NULL) {
		DRM_INFO("LDM mmap info node find success \r\n");
		name = MTK_DRM_DTS_MMAP_INFO_LD_MMAP_ADDR_REG_TAG;

	u32TmpArray = kmalloc(sizeof(__u32) * 4, GFP_KERNEL);

	if (u32TmpArray == NULL) {
		DRM_ERROR("[LDM][%s, %d]: malloc failed \r\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto ERROR;
	}

	ret = of_property_read_u32_array(np, name, u32TmpArray, 4);
	if (ret != 0x0) {
		kfree(u32TmpArray);
		return ret;
	}
	for (i = 0; i < 4; i++)
		DRM_INFO("u32TmpArray[%d]  %x \r\n",
			i, u32TmpArray[i]);


	pctx->u64LDM_phyAddress = (__u64)(u32TmpArray[1]-0x20000000);
	DRM_INFO("From mmap LDM_phyaddress=%x  %tx \r\n",
		u32Tmp, (ptrdiff_t)pctx->u64LDM_phyAddress);

	kfree(u32TmpArray);

	}

	_parse_ldm_panel_info(pctx);
	_parse_ldm_misc_info(pctx);
	_parse_ldm_mspi_info(pctx);
	_parse_ldm_dma_info(pctx);
	_parse_ldm_led_device_info(pctx);
	dump_dts_ld_info(pctx);

	return 0;

ERROR:
	kfree(u32TmpArray);

	return ret;
}


int mtk_ldm_init(struct device *dev)
{

	int ret;
	struct mtk_tv_kms_context *pctx = dev_get_drvdata(dev);
	struct mtk_ld_context *pctx_ld = &pctx->ld_priv;


	ret = readDTB2LDMPrivate(pctx_ld);
	if (ret != 0x0) {

		DRM_INFO(
			"[%s, %d]: readDTB2LDMPrivate failed\n",
			__func__, __LINE__);

		return -ENODEV;
	}

	//ret = MApi_ld_Init_SetLDC(pctx_ld->u8LDMSupport);
	ldm_param.u8ld_support_type = pctx_ld->u8LDMSupport;
	ldm_param.u8ld_version = pctx_ld->u8LDM_Version;
	ldm_param.u8auto_ld = 0;
	ldm_param.u8xtended_range = 0;
	ldm_param.u8backlight = BACKLIGHT;

	pqu_msg_send(PQU_MSG_SEND_LDM, &ldm_param);

	DRM_INFO(
	"[DRM][LDM][%s][%d] ld_version_reply %d %x !!\n",
	__func__,
	__LINE__,
	ret,
	ldm_reply_param.ret);

/*   ret = pqu_render_ldm(&ldm_param, &ldm_reply_param);*/
	DRM_INFO(
	"[DRM][LDM][%s][%d] ld_version_reply1 %d %x !!\n",
	__func__,
	__LINE__,
	ret,
	ldm_reply_param.ret);


	if (pctx_ld->u8LDMSupport == E_LDM_SUPPORT_TRUNK) {
		DRM_INFO("LDM support TRUNK \r\n");
		ret = MApi_ld_Init(pctx_ld->u64LDM_phyAddress, pctx);
		//ret = MApi_ld_Init_SetLDC(pctx_ld->u8LDMSupport);

	} else if (pctx_ld->u8LDMSupport == E_LDM_SUPPORT_CUS) {
		DRM_INFO("LDM support CUS \r\n");
/*
		ret = MApi_ld_Init_SetLDC(pctx_ld->u8LDMSupport);
		if (ret != 0x0) {
			DRM_ERROR("[LDM][%s, %d]: setLDC failed.\n", __func__,
				  __LINE__);
			return ret;
		}

		ret = MApi_ld_Init_UpdateProfile(pctx_ld->u64LDM_phyAddress, pctx);
		if (ret != 0x0) {
			DRM_ERROR("[LDM][%s, %d]: UpdateProfile failed.\n", __func__,
				  __LINE__);
			return ret;
		}

		ret = _mtk_ldm_select_pins_to_func(pctx, 1);
		if (ret != 0x0) {
			DRM_ERROR("[LDM][%s, %d]: SetTmon_pmux failed.\n", __func__,
				  __LINE__);
			return ret;
		}
*/

	} else {
		//ret = MApi_ld_Init_SetLDC(pctx_ld->u8LDMSupport);

	}


	return 0;
}



