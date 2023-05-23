// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_pqu_metadata.h"
#include "mtk_tv_drm_metabuf.h"
#include "mtk_tv_drm_log.h"
#include "pqu_msg.h"
#include "ext_command_render_if.h"

//--------------------------------------------------------------------------------------------------
//  Local Defines
//--------------------------------------------------------------------------------------------------
#define pqu_metadata_to_kms(x) container_of(x, struct mtk_tv_kms_context, pqu_metadata_priv)
#define MTK_DRM_MODEL MTK_DRM_MODEL_PQU_METADATA
#define ERR(fmt, args...) DRM_ERROR("[%s][%d] " fmt, __func__, __LINE__, ##args)
#define LOG(fmt, args...) DRM_INFO("[%s][%d] " fmt, __func__, __LINE__, ##args)

#define PQU_ENDIAN_CONVERT_ENABLE       (0) // is the endian difference between host and pqu
#if PQU_ENDIAN_CONVERT_ENABLE
#define host_to_pqu_16(v)               bswap_16(v)
#define host_to_pqu_32(v)               bswap_32(v)
#define host_to_pqu_64(v)               bswap_64(v)
#else
#define host_to_pqu_16(v)               (v)
#define host_to_pqu_32(v)               (v)
#define host_to_pqu_64(v)               (v)
#endif
#define pqu_to_host_16(v)               host_to_pqu_16(v)
#define pqu_to_host_32(v)               host_to_pqu_32(v)
#define pqu_to_host_64(v)               host_to_pqu_64(v)

//--------------------------------------------------------------------------------------------------
//  Local Enum and Structures
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local Variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local Functions
//--------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_tv_drm_pqu_metadata_init(
	struct mtk_pqu_metadata_context *context)
{
	int i;

	if (!context) {
		ERR("Invalid input");
		return -ENXIO;
	}
	if (context->init == true) {
		ERR("context has already inited");
		return 0;
	}

	memset(&context->pqu_metabuf, 0, sizeof(struct mtk_tv_drm_metabuf));
	if (mtk_tv_drm_metabuf_alloc_by_mmap(
			&context->pqu_metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_MATADATA)) {
		ERR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_MATADATA);
		return -EPERM;
	}

	for (i = 0; i < MTK_TV_DRM_PQU_METADATA_COUNT; ++i) {
		context->pqu_metadata_ptr[i] = context->pqu_metabuf.addr +
			(sizeof(struct drm_mtk_tv_pqu_metadata) * i);
	}

	LOG("msg_sharemem.phys  = %llu", context->pqu_metabuf.mmap_info.phy_addr);
	LOG("msg_sharemem.size  = %lu",  sizeof(struct drm_mtk_tv_pqu_metadata));
	LOG("msg_sharemem.count = %lu",  MTK_TV_DRM_PQU_METADATA_COUNT);
	if (bPquEnable) {
		struct pqu_render_be_sharemem_info_param pqu_sharemem;

		LOG("setup PQU be shared memory info");
		memset(&pqu_sharemem, 0, sizeof(struct pqu_render_be_sharemem_info_param));
		pqu_sharemem.phys = context->pqu_metabuf.mmap_info.phy_addr;
		pqu_sharemem.size = sizeof(struct drm_mtk_tv_pqu_metadata);
		pqu_sharemem.count = MTK_TV_DRM_PQU_METADATA_COUNT;
		pqu_render_be_sharemem_info(
			(const struct pqu_render_be_sharemem_info_param *)&pqu_sharemem, NULL);
	} else {
		struct msg_render_be_sharemem_info msg_sharemem;

		LOG("setup MSG be shared memory info");
		memset(&msg_sharemem, 0, sizeof(struct msg_render_be_sharemem_info));
		msg_sharemem.phys = context->pqu_metabuf.mmap_info.phy_addr;
		msg_sharemem.size = sizeof(struct drm_mtk_tv_pqu_metadata);
		msg_sharemem.count = MTK_TV_DRM_PQU_METADATA_COUNT;
		pqu_msg_send(PQU_MSG_SEND_RENDER_BE_SHAREMEM_INFO, &msg_sharemem);
	}

	context->init = true;
	return 0;
}

int mtk_tv_drm_pqu_metadata_deinit(
	struct mtk_pqu_metadata_context *context)
{
	int ret = 0;

	if (!context) {
		ERR("Invalid input, context = %lx", context);
		return -EINVAL;
	}
	if (context->init == false) {
		ERR("render context has not been inited");
		return 0;
	}
	if (mtk_tv_drm_metabuf_free(&context->pqu_metabuf)) {
		ERR("free mmap fail");
		ret = -EPERM;
	}

	// clear PQU info
	if (bPquEnable) {
		struct pqu_render_be_sharemem_info_param pqu_sharemem;

		LOG("clean PQU be shared memory info");
		memset(&pqu_sharemem, 0, sizeof(struct pqu_render_be_sharemem_info_param));
		pqu_render_be_sharemem_info(
			(const struct pqu_render_be_sharemem_info_param *)&pqu_sharemem, NULL);
	} else {
		struct msg_render_be_sharemem_info msg_sharemem;

		LOG("clean MSG be shared memory info");
		memset(&msg_sharemem, 0, sizeof(struct msg_render_be_sharemem_info));
		pqu_msg_send(PQU_MSG_SEND_RENDER_BE_SHAREMEM_INFO, &msg_sharemem);
	}

	memset(context, 0, sizeof(struct mtk_pqu_metadata_context));
	LOG("deinit render context succsessed !!!");
	return ret;
}

int mtk_tv_drm_pqu_metadata_set_attr(
	struct mtk_pqu_metadata_context *context,
	enum mtk_pqu_metadata_attr attr,
	void *value)
{
	bool update_be_setting = false;
	int ret = 0;

	if (!context || !value) {
		ERR("Invalid input");
		return -EINVAL;
	}
	if (context->init == false) {
		ERR("render context has not been inited");
		return -EINVAL;
	}

	switch (attr) {
	case MTK_PQU_METADATA_ATTR_GLOBAL_FRAME_ID:
		context->pqu_metadata.global_frame_id = *(uint32_t *)value;
		// LOG("Update global_frame_id = %u", context->pqu_metadata.global_frame_id);
		break;

	case MTK_PQU_METADATA_ATTR_RENDER_MODE:
		context->render_setting.render_mode = *(uint32_t *)value;
		LOG("Update render_mode = %u", context->render_setting.render_mode);
		update_be_setting = true;
		break;

	default:
		ERR("Unknown attr %d", attr);
		ret = -EINVAL;
	}

	if (update_be_setting) {
		if (bPquEnable) {
			struct pqu_render_be_setting_param pqu_be_setting;

			memset(&pqu_be_setting, 0, sizeof(struct pqu_render_be_setting_param));
			pqu_be_setting.render_mode = context->render_setting.render_mode;
			pqu_render_be_setting(
				(const struct pqu_render_be_setting_param *)&pqu_be_setting, NULL);
		} else {
			struct msg_render_be_setting msg_be_setting;

			memset(&msg_be_setting, 0, sizeof(struct msg_render_be_setting));
			msg_be_setting.render_mode = context->render_setting.render_mode;
			pqu_msg_send(PQU_MSG_SEND_RENDER_BE_SETTING, &msg_be_setting);
		}
	}
	return ret;
}

int mtk_tv_drm_pqu_metadata_add_window_setting(
	struct mtk_pqu_metadata_context *context,
	struct drm_mtk_tv_pqu_window_setting *window_setting)
{
	struct drm_mtk_tv_pqu_metadata *pqu_metadata;

	if (!context || !window_setting) {
		ERR("Invalid input");
		return -EINVAL;
	}
	if (context->init == false) {
		ERR("render context has not been inited");
		return -EINVAL;
	}
	pqu_metadata = &context->pqu_metadata;
	if (pqu_metadata->window_num >= MTK_TV_DRM_WINDOW_NUM_MAX) {
		ERR("Too many window, MAX %d", MTK_TV_DRM_WINDOW_NUM_MAX);
		return -EINVAL;
	}
	LOG("window   = %u", pqu_metadata->window_num);
	LOG("mute     = %u", window_setting->mute);
	LOG("pq_id    = %u", window_setting->pq_id);
	LOG("frame_id = %u", window_setting->frame_id);
	LOG("window_x = %u", window_setting->window_x);
	LOG("window_y = %u", window_setting->window_y);
	LOG("window_z = %u", window_setting->window_z);
	LOG("window_w = %u", window_setting->window_width);
	LOG("window_h = %u", window_setting->window_height);
	memcpy(&pqu_metadata->window_setting[pqu_metadata->window_num++], window_setting,
		sizeof(struct drm_mtk_tv_pqu_window_setting));
	return 0;
}

int mtk_tv_drm_pqu_metadata_fire_window_setting(
	struct mtk_pqu_metadata_context *context)
{
	struct mtk_tv_kms_context *kms = pqu_metadata_to_kms(context);
	struct mtk_video_context *pctx_video = &kms->video_priv;
	uint16_t reg_num = pctx_video->reg_num;
	struct hwregDummyValueIn dummy_reg_in;
	struct hwregOut dummy_reg_out;
	struct sm_ml_add_info ml_cmd;
	struct reg_info reg[reg_num];

	if (!context) {
		ERR("Invalid input");
		return -EINVAL;
	}
	if (context->init == false) {
		ERR("render context has not been inited");
		return -EINVAL;
	}

	memset(&ml_cmd, 0, sizeof(struct sm_ml_add_info));
	memset(&dummy_reg_in, 0, sizeof(struct hwregDummyValueIn));
	memset(&dummy_reg_out, 0, sizeof(struct hwregOut));

	// if pqurv55 enable, convert endian
	if (bPquEnable) {
		struct drm_mtk_tv_pqu_metadata *pqu_metadata = &context->pqu_metadata;
		struct drm_mtk_tv_pqu_window_setting *setting = NULL;
		int i;

		for (i = 0; i < pqu_metadata->window_num; ++i) {
			setting = &pqu_metadata->window_setting[i];
			setting->mute          = host_to_pqu_32(setting->mute);
			setting->pq_id         = host_to_pqu_32(setting->pq_id);
			setting->frame_id      = host_to_pqu_32(setting->frame_id);
			setting->window_x      = host_to_pqu_32(setting->window_x);
			setting->window_y      = host_to_pqu_32(setting->window_y);
			setting->window_z      = host_to_pqu_32(setting->window_z);
			setting->window_width  = host_to_pqu_32(setting->window_width);
			setting->window_height = host_to_pqu_32(setting->window_height);
		}
		pqu_metadata->window_num      = host_to_pqu_32(pqu_metadata->window_num);
		pqu_metadata->global_frame_id = host_to_pqu_32(pqu_metadata->global_frame_id);
	}

	// copy current pqu_metadata to correct shared memory and reset it
	memcpy(context->pqu_metadata_ptr[context->pqu_metadata_ptr_index],
		&context->pqu_metadata, sizeof(struct drm_mtk_tv_pqu_metadata));
	memset(&context->pqu_metadata, 0, sizeof(struct drm_mtk_tv_pqu_metadata));

	// get dummy reg
	dummy_reg_in.RIU = 0;
	dummy_reg_in.dummy_type = DUMMY_TYPE_PQU_METADATA_INDEX;
	dummy_reg_in.dummy_value = host_to_pqu_16(context->pqu_metadata_ptr_index);
	dummy_reg_out.reg = reg;
	drv_hwreg_render_display_set_dummy_value(dummy_reg_in, &dummy_reg_out);

	// send dummy reg
	if (!dummy_reg_in.RIU && dummy_reg_out.regCount) {
		ml_cmd.cmd_cnt = dummy_reg_out.regCount;
		ml_cmd.mem_index = pctx_video->disp_ml.memindex;
		ml_cmd.reg = (struct sm_reg *)&reg;
		sm_ml_add_cmd(pctx_video->disp_ml.fd, &ml_cmd);
		pctx_video->disp_ml.regcount += dummy_reg_out.regCount;
	}
	LOG("set dummy reg to %d", context->pqu_metadata_ptr_index);

	// update shared memory index
	context->pqu_metadata_ptr_index =
		(context->pqu_metadata_ptr_index + 1) % MTK_TV_DRM_PQU_METADATA_COUNT;

	return 0;
}

int mtk_tv_drm_pqu_metadata_get_copy_ioctl(
	struct drm_device *drm,
	void *data,
	struct drm_file *file_priv)
{
	struct mtk_tv_drm_metabuf pqu_metabuf;
	struct drm_mtk_tv_pqu_metadata pqu_metadata;
	uint16_t pqu_meta_index = MTK_TV_DRM_PQU_METADATA_COUNT;
	int ret = 0;

	if (data == NULL) {
		ERR("Invalid input");
		return -EINVAL;
	}

	// get pqu metadata index
	drv_hwreg_render_display_get_dummy_value(DUMMY_TYPE_PQU_METADATA_INDEX, &pqu_meta_index);
	// if pqurv55 enable, convert endian
	if (bPquEnable)
		pqu_meta_index = pqu_to_host_16(pqu_meta_index);
	if (pqu_meta_index >= MTK_TV_DRM_PQU_METADATA_COUNT) {
		ERR("get meta index fail (%u)", pqu_meta_index);
		return -EPERM;
	}

	// get metabuf of pqu metadata mmap
	memset(&pqu_metabuf, 0, sizeof(struct mtk_tv_drm_metabuf));
	if (mtk_tv_drm_metabuf_alloc_by_mmap(
		&pqu_metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_MATADATA)) {
		ERR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_MATADATA);
		return -EPERM;
	}

	// copy current mmap pqu metadata to local buffer
	memcpy(&pqu_metadata, &((struct drm_mtk_tv_pqu_metadata *)pqu_metabuf.addr)[pqu_meta_index],
		sizeof(struct drm_mtk_tv_pqu_metadata));
	// if pqurv55 enable, convert endian
	if (bPquEnable) {
		struct drm_mtk_tv_pqu_window_setting *setting = NULL;
		int i;

		pqu_metadata.window_num      = pqu_to_host_32(pqu_metadata.window_num);
		pqu_metadata.global_frame_id = pqu_to_host_32(pqu_metadata.global_frame_id);
		if (pqu_metadata.window_num > MTK_TV_DRM_WINDOW_NUM_MAX) {
			ERR("get window number fail (%u)", pqu_metadata.window_num);
			ret = -EPERM;
			goto EXIT;
		}
		for (i = 0; i < pqu_metadata.window_num; ++i) {
			setting = &pqu_metadata.window_setting[i];
			setting->mute          = pqu_to_host_32(setting->mute);
			setting->pq_id         = pqu_to_host_32(setting->pq_id);
			setting->frame_id      = pqu_to_host_32(setting->frame_id);
			setting->window_x      = pqu_to_host_32(setting->window_x);
			setting->window_y      = pqu_to_host_32(setting->window_y);
			setting->window_z      = pqu_to_host_32(setting->window_z);
			setting->window_width  = pqu_to_host_32(setting->window_width);
			setting->window_height = pqu_to_host_32(setting->window_height);
		}
	}
	// copy local pqu metadata buffer to output buffer
	memcpy(data, &pqu_metadata, sizeof(struct drm_mtk_tv_pqu_metadata));
EXIT:
	// free metabuf
	if (mtk_tv_drm_metabuf_free(&pqu_metabuf)) {
		ERR("free mmap fail");
		ret = -EPERM;
	}
	return ret;
}
