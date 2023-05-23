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
#include "mtk_tv_drm_crtc.h"
#include "mtk_tv_drm_demura.h"
#include "mtk_tv_drm_log.h"
#include "pqu_msg.h"
#include "ext_command_render_if.h"

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define MTK_DRM_MODEL MTK_DRM_MODEL_DEMURA
#define ERR(fmt, args...) DRM_ERROR("[%s][%d] " fmt, __func__, __LINE__, ##args)
#define LOG(fmt, args...) DRM_INFO("[%s][%d] " fmt, __func__, __LINE__, ##args)

//-------------------------------------------------------------------------------------------------
//  Local Enum and Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
int mtk_tv_drm_demura_init(
	struct mtk_demura_context *demura)
{
	struct mtk_tv_drm_metabuf metabuf = {0};
	void *va = NULL;
	uint64_t pa = 0;
	uint32_t len = 0;
	int ret = 0;
	int i;

	if (!demura) {
		ERR("Invalid input");
		return -EINVAL;
	}
	if (demura->init == true) {
		ERR("demura is already inited");
		return 0;
	}

	if (mtk_tv_drm_metabuf_alloc_by_mmap(&metabuf, MTK_TV_DRM_METABUF_MMAP_TYPE_DEMURA)) {
		ERR("get mmap %d fail", MTK_TV_DRM_METABUF_MMAP_TYPE_DEMURA);
		return -EPERM;
	}
	len = metabuf.size / MTK_DEMURA_SLOT_NUM_MAX;
	va  = metabuf.addr;
	pa  = metabuf.mmap_info.phy_addr;

	memset(demura, 0, sizeof(struct mtk_demura_context));
	memcpy(&demura->metabuf, &metabuf, sizeof(struct mtk_tv_drm_metabuf));
	for (i = 0; i < MTK_DEMURA_SLOT_NUM_MAX; ++i) {
		demura->config_slot[i].bin_va = va;
		demura->config_slot[i].bin_pa = pa;
		va += len;
		pa += len;
	}
	demura->bin_max_size = len;
	demura->init = true;

	LOG("mmap bus address  = 0x%llx", demura->metabuf.mmap_info.bus_addr);
	LOG("mmap phys address = 0x%llx", demura->metabuf.mmap_info.phy_addr);
	LOG("mmap virt address = 0x%llx", demura->metabuf.addr);
	LOG("mmap size         = 0x%llx", demura->metabuf.size);
	LOG("demura slot num   = 0x%lx",  MTK_DEMURA_SLOT_NUM_MAX);
	LOG("demura bin size   = 0x%llx", demura->bin_max_size);
	return 0;
}

int mtk_tv_drm_demura_deinit(
	struct mtk_demura_context *demura)
{
	if (!demura) {
		ERR("Invalid input");
		return -ENXIO;
	}
	if (demura->init == false) {
		LOG("render demura is not inited");
		return 0;
	}
	mtk_tv_drm_metabuf_free(&demura->metabuf);
	memset(demura, 0, sizeof(struct mtk_demura_context));
	return 0;
}

int mtk_tv_drm_demura_set_config(
	struct mtk_demura_context *demura,
	struct drm_mtk_demura_config *config)
{
	struct mtk_demura_slot *slot = NULL;
	struct msg_render_demura_info info;
	struct pqu_render_demura_param param;
	int index = 0;
	int i;

	if (!demura || !config) {
		ERR("Invalid input");
		return -EINVAL;
	}

	if (demura->init == false) {
		ERR("render demura is not inited");
		return -EINVAL;
	}

	if (config->binary_size >= demura->bin_max_size) {
		ERR("demura binary too large, input(0x%llx) >= max(0x%llx)",
			config->binary_size, demura->bin_max_size);
		return -EINVAL;
	}

	index = demura->slot_idx;
	slot = &demura->config_slot[index];
	slot->bin_len = config->binary_size;
	slot->disable = config->disable;
	slot->id      = config->id;
	memcpy(slot->bin_va, config->binary_data, config->binary_size);
	demura->slot_idx = (demura->slot_idx + 1) % MTK_DEMURA_SLOT_NUM_MAX;
	LOG("slot index = %d", index);
	LOG("bin size   = 0x%llx", slot->bin_len);
	LOG("bin va     = 0x%llx", slot->bin_va);
	LOG("bin pa     = 0x%llx", slot->bin_pa);
	LOG("disable    = 0x%llx", slot->disable);
	LOG("id         = 0x%llx", slot->id);

	if (bPquEnable) { // RV55
		memset(&param, 0, sizeof(struct pqu_render_demura_param));
		param.binary_pa  = slot->bin_pa;
		param.binary_len = slot->bin_len;
		param.disable    = slot->disable;
		param.hw_index   = slot->id;
		pqu_render_demura((const struct pqu_render_demura_param *)&param, NULL);
	} else { // PQU
		memset(&info, 0, sizeof(struct msg_render_demura_info));
		info.binary_pa  = slot->bin_pa;
		info.binary_len = slot->bin_len;
		info.disable    = slot->disable;
		info.hw_index   = slot->id;
		pqu_msg_send(PQU_MSG_SEND_DEMURA, (void *)&info);
	}
	LOG("pqu done\n");
	return 0;
}
