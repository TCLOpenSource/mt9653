// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/dma-buf.h>
#include <drm/mtk_tv_drm.h>

#include "mtk_tv_drm_kms.h"
#include "mtk_tv_drm_metabuf.h"
#include "mtk_tv_drm_pqu_metadata.h"
#include "mtk_tv_drm_log.h"
#include "mtk-reserved-memory.h"
#include "m_pqu_render.h"

//--------------------------------------------------------------------------------------------------
//  Local Defines
//--------------------------------------------------------------------------------------------------
#define MTK_DRM_MODEL MTK_DRM_MODEL_METABUF
#define ERR(fmt, args...) DRM_ERROR("[%s][%d] " fmt, __func__, __LINE__, ##args)
#define LOG(fmt, args...) DRM_INFO("[%s][%d] " fmt, __func__, __LINE__, ##args)
#define BUSADDRESS_TO_IPADDRESS_OFFSET  0x20000000


#define MMAP_INDEX_GOP		0
#define MMAP_INDEX_DEMURA	1
#define MMAP_INDEX_PQU		2

//--------------------------------------------------------------------------------------------------
//  Local Enum and Structures
//--------------------------------------------------------------------------------------------------
struct pqubuf_layout {
	// PQU metadata: mtk_tv_drm_pqu_metadata.h
	struct drm_mtk_tv_pqu_metadata pqu_metadata[MTK_TV_DRM_PQU_METADATA_COUNT];
	// gamma data : M_pqu_render.h
	struct meta_render_pqu_pq_info gamma;
};

//--------------------------------------------------------------------------------------------------
//  Local Variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Global Functions
//--------------------------------------------------------------------------------------------------
int mtk_tv_drm_metabuf_alloc_by_mmap(
	struct mtk_tv_drm_metabuf *drm_metabuf,
	enum mtk_tv_drm_metabuf_mmap_type mmap_type)
{
	struct device_node *np = NULL;
	struct of_mmap_info_data mmap_data;
	void *virt;
	uint32_t index;
	uint64_t offset, size, bus, phys;
	int ret;

	if (mmap_type >= MTK_TV_DRM_METABUF_MMAP_TYPE_MAX || drm_metabuf == NULL) {
		ERR("Invalid input");
		return -EINVAL;
	}
	switch (mmap_type) {
	case MTK_TV_DRM_METABUF_MMAP_TYPE_GOP:
		index  = MMAP_INDEX_GOP;
		offset = size = 0; // use all mmap
		break;

	case MTK_TV_DRM_METABUF_MMAP_TYPE_DEMURA:
		index  = MMAP_INDEX_DEMURA;
		offset = size = 0; // use all mmap
		break;

	case MTK_TV_DRM_METABUF_MMAP_TYPE_PQU:
		index  = MMAP_INDEX_PQU;
		offset = size = 0; // use all mmap
		break;

	case MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_MATADATA:
		index  = MMAP_INDEX_PQU;
		offset = offsetof(struct pqubuf_layout, pqu_metadata);
		size   = sizeof(((struct pqubuf_layout *)0)->pqu_metadata);
		break;

	case MTK_TV_DRM_METABUF_MMAP_TYPE_PQU_GAMMA:
		index  = MMAP_INDEX_PQU;
		offset = offsetof(struct pqubuf_layout, gamma);
		size   = sizeof(((struct pqubuf_layout *)0)->gamma);
		break;

	default:
		ERR("Unknown metabuf type %d", drm_metabuf->metabuf_type);
		return -EINVAL;
	}
	LOG("mmap_type = %d",     mmap_type);
	LOG("index     = 0x%llx", index);
	LOG("size      = 0x%llx", size);
	LOG("offset    = 0x%llx", offset);

	np = of_find_compatible_node(NULL, NULL, MTK_DRM_TV_KMS_COMPATIBLE_STR);
	if (np == NULL) {
		ERR("find node %s failed", MTK_DRM_TV_KMS_COMPATIBLE_STR);
		return -EPERM;
	}
	ret = of_mtk_get_reserved_memory_info_by_idx(np, index, &mmap_data);
	if (ret) {
		ERR("get mmap index %d failed, ret = %d", index, ret);
		return -EPERM;
	}
	LOG("mmap size = 0x%llx", mmap_data.buffer_size);
	LOG("mmap bus  = 0x%llx", mmap_data.start_bus_address);

	if (size == 0 && offset == 0) {
		// @size is replaced by mmap size
		size = mmap_data.buffer_size;
	} else if (offset + size > mmap_data.buffer_size) {
		ERR("offset(%llu) + size(%llu) > limit(%llu)", offset, size, mmap_data.buffer_size);
		return -EINVAL;
	}
	bus  = mmap_data.start_bus_address + offset;
	phys = bus - BUSADDRESS_TO_IPADDRESS_OFFSET;
	virt = pfn_valid(__phys_to_pfn(bus)) ? __va(bus) : ioremap_wc(bus, size);
	if (IS_ERR_OR_NULL(virt)) {
		ERR("convert bus to virt fail, pfn: %d, bus: 0x%lx, size: 0x%lx",
			pfn_valid(__phys_to_pfn(bus)), bus, size);
		return -EPERM;
	}

	drm_metabuf->addr = virt;
	drm_metabuf->size = size;
	drm_metabuf->metabuf_type = MTK_TV_DRM_METABUF_TYPE_MMAP;
	drm_metabuf->mmap_info.mmap_type = mmap_type;
	drm_metabuf->mmap_info.bus_addr  = bus;
	drm_metabuf->mmap_info.phy_addr  = phys;
	LOG("meta addr = 0x%lx",  drm_metabuf->addr);
	LOG("meta size = 0x%lx",  drm_metabuf->size);
	LOG("meta type = %d",     drm_metabuf->mmap_info.mmap_type);
	LOG("meta bus  = 0x%llx", drm_metabuf->mmap_info.bus_addr);
	LOG("meta phys = 0x%llx", drm_metabuf->mmap_info.phy_addr);
	return 0;
}

int mtk_tv_drm_metabuf_alloc_by_ion(
	struct mtk_tv_drm_metabuf *drm_metabuf,
	int ion_fd)
{
	struct dma_buf *dma_buf = NULL;
	uint32_t size;
	void *addr;

	if (drm_metabuf == NULL || ion_fd <= 0) {
		ERR("Invalid input");
		return -EINVAL;
	}
	dma_buf = dma_buf_get(ion_fd);
	if (IS_ERR_OR_NULL(dma_buf)) {
		ERR("dma_buf_get fail, ret = 0x%lx", dma_buf);
		return -EPERM;
	}
	size = dma_buf->size;
	addr = dma_buf_vmap(dma_buf);
	if (IS_ERR_OR_NULL(addr)) {
		ERR("dma_buf_vmap fail, ret = 0x%lx", addr);
		dma_buf_put(dma_buf);
		return -EPERM;
	}

	drm_metabuf->addr = addr;
	drm_metabuf->size = size;
	drm_metabuf->metabuf_type = MTK_TV_DRM_METABUF_TYPE_ION;
	drm_metabuf->ion_info.ion_fd  = ion_fd;
	drm_metabuf->ion_info.dma_buf = dma_buf;
	LOG("addr    = 0x%lx", drm_metabuf->addr);
	LOG("size    = 0x%lx", drm_metabuf->size);
	LOG("ion_fd  = %d",    drm_metabuf->ion_info.ion_fd);
	LOG("dma_buf = 0x%lx", drm_metabuf->ion_info.dma_buf);
	return 0;
}

int mtk_tv_drm_metabuf_free(
	struct mtk_tv_drm_metabuf *drm_metabuf)
{
	if (drm_metabuf == NULL || drm_metabuf->addr == NULL) {
		ERR("Invalid input");
		return -EINVAL;
	}
	switch (drm_metabuf->metabuf_type) {
	case MTK_TV_DRM_METABUF_TYPE_ION:
		dma_buf_vunmap(drm_metabuf->ion_info.dma_buf, drm_metabuf->addr);
		dma_buf_put(drm_metabuf->ion_info.dma_buf);

		LOG("ion fd: %d, dma_buf: 0x%lx, addr: 0x%lx", drm_metabuf->ion_info.ion_fd,
			drm_metabuf->ion_info.dma_buf, drm_metabuf->addr);
		break;

	case MTK_TV_DRM_METABUF_TYPE_MMAP:
		if (!pfn_valid(__phys_to_pfn(drm_metabuf->mmap_info.bus_addr)))
			iounmap(drm_metabuf->addr);

		LOG("mmap idx: %d, bus: 0x%lx, addr: 0x%lx", drm_metabuf->mmap_info.mmap_type,
			drm_metabuf->mmap_info.bus_addr, drm_metabuf->addr);
		break;

	default:
		ERR("Unknown metabuf type %d", drm_metabuf->metabuf_type);
		return -EINVAL;
	}
	memset(drm_metabuf, 0, sizeof(struct mtk_tv_drm_metabuf));
	return 0;
}
