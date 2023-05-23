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
//#include "mtk_tv_drm_utpa_wrapper.h"
#include "mtk_tv_drm_drv.h"
#include "mtk_tv_drm_connector.h"
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_ld.h"
#include "mtk_tv_drm_encoder.h"
//#include "mtk_tv_drm_osdp_plane.h"
#include "mtk_tv_drm_kms.h"

#include "apiLDM.h"
#include "drvLDM_sti.h"

//#define MTK_DRM_LD_COMPATIBLE_STR "MTK-drm-ld"
#define MTK_DRM_DTS_LD_SUPPORT_TAG "LDM_SUPPORT"
#define MTK_DRM_DTS_LD_MMAP_ADDR_TAG "LDM_MMAP_ADDR"
#define MTK_DRM_DTS_LD_VERSION_TAG "MX_LDM_VERSION"

#define MTK_DRM_DTS_MMAP_INFO_LD_MMAP_ADDR_TAG "MI_DISPOUT_LOCAL_DIMMING"
#define MTK_DRM_DTS_MMAP_INFO_LD_MMAP_ADDR_REG_TAG "reg"




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

	pctx = (struct mtk_tv_kms_context *)crtc->crtc_private;
	crtc_props = pctx->ext_crtc_properties;
	pctx_ld = &pctx->ld_priv;

	if (pctx_ld->u8LDMSupport != E_LDM_SUPPORT) {
		DRM_INFO("[DRM][LDM][%s][%d]LDM unsupport, support = %td\n",
		__func__, __LINE__,
		(ptrdiff_t)pctx_ld->u8LDMSupport);

		return 0;

	} else {
		switch (prop_index) {
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
		case E_EXT_CRTC_PROP_MAX:
		DRM_INFO(
			"[DRM][LDM][%s][%d] invalid property!!\n",
			__func__, __LINE__);
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


	if (pctx_ld->u8LDMSupport != E_LDM_SUPPORT) {
		DRM_INFO("[DRM][LDM][%s][%d]LDM unsupport, support = %td\n",
		__func__, __LINE__,
		(ptrdiff_t)pctx_ld->u8LDMSupport);

		return 0;

	} else {

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
	}
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
	ret = of_property_read_u32_array(np, name, u32TmpArray, 4);
	if (ret != 0x0) {
		kfree(u32TmpArray);
		return ret;
	}
	for (i = 0; i < 4; i++)
		DRM_INFO("u32TmpArray[%d]  %x \r\n",
			i, u32TmpArray[i]);


	pctx->u64LDM_phyAddress = (__u64)(u32TmpArray[1]-0x20000000);

	kfree(u32TmpArray);

	}


	return 0;
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

	DRM_INFO("[%s][%d] success!!\n", __func__, __LINE__);



	if (pctx_ld->u8LDMSupport == E_LDM_SUPPORT) {
		DRM_INFO("LDM support TRUNK \r\n");

		//ret = MApi_ld_Init(pctx_ld->u64LDM_phyAddress);

	} else {
		DRM_INFO("LDM Unsupport \r\n");
	}

	return 0;
}



