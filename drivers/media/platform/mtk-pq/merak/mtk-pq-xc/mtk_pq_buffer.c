// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Kevin Ren <kevin.ren@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "mtk_pq.h"

#define SYSFS_MAX_BUF_COUNT        (0x100)
#define PQU_BIT_WIDTH_IDK          (36)
#define IDK_ENABLE_POSITION        (14)

static uint16_t _mtk_pq_transfer_ip_val(hwmap_ip_sw hwreg_ip, uint16_t hwmap_src_type)
{
	uint16_t ipIdx = 0;
	uint16_t levelIdx = 0;
	uint16_t ipLevel = 0;

	ipIdx = drv_hwreg_common_config_mapping_ip(hwreg_ip);
	levelIdx = drv_hwreg_common_config_get_table_level(hwmap_src_type, ipIdx, 0);
	ipLevel = drv_hwreg_common_config_mapping_ip_level(hwreg_ip, levelIdx);

	return ipLevel;
}

static int _mtk_pq_cal_buf_size(struct pq_buffer *ppq_buf, enum pqu_buffer_type buf_type,
			struct meta_pq_stream_info stream)
{
	uint16_t ctur_mode = 1, ctur2_mode = 1;

	if (ppq_buf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	switch (buf_type) {
	case PQU_BUF_SCMI:
	case PQU_BUF_UCM:
		ppq_buf->size_ch[PQU_BUF_CH_0] =
			(((stream.width + (ADDRESS_ALIGN - 1)) / ADDRESS_ALIGN)
			* ADDRESS_ALIGN * stream.height * ppq_buf->frame_num
			* ppq_buf->bit[PQU_BUF_CH_0]) / BIT_PER_BYTE;
		ppq_buf->size_ch[PQU_BUF_CH_1] =
			(((stream.width + (ADDRESS_ALIGN - 1)) / ADDRESS_ALIGN)
			* ADDRESS_ALIGN * stream.height * ppq_buf->frame_num
			* ppq_buf->bit[PQU_BUF_CH_1]) / BIT_PER_BYTE;
		ppq_buf->size_ch[PQU_BUF_CH_2] =
			(((stream.width + (ADDRESS_ALIGN - 1)) / ADDRESS_ALIGN)
			* ADDRESS_ALIGN * stream.height * ppq_buf->frame_num
			* ppq_buf->bit[PQU_BUF_CH_2]) / BIT_PER_BYTE;
		break;
	case PQU_BUF_ZNR:
		ppq_buf->size_ch[PQU_BUF_CH_0] =
				(stream.width / 32) * (stream.height / 32) *
				((ppq_buf->bit[PQU_BUF_CH_0] + (BIT_PER_BYTE - 1)) / BIT_PER_BYTE)
				* ppq_buf->frame_num;
		ppq_buf->size_ch[PQU_BUF_CH_1] =
				(stream.width / 16) * (stream.height / 16) *
				((ppq_buf->bit[PQU_BUF_CH_1] + (BIT_PER_BYTE - 1)) / BIT_PER_BYTE)
				* ppq_buf->frame_num;
		ppq_buf->size_ch[PQU_BUF_CH_2] = 0;
		break;
	case PQU_BUF_ABF:
		mtk_display_abf_blk_mode(stream.width, &ctur_mode, &ctur2_mode);

		ppq_buf->size_ch[PQU_BUF_CH_0] =
				(stream.width / ctur_mode) * (stream.height / ctur_mode) *
				((ppq_buf->bit[PQU_BUF_CH_0] + BIT_PER_BYTE - 1) / BIT_PER_BYTE)
				* ppq_buf->frame_num;
		ppq_buf->size_ch[PQU_BUF_CH_1] =
				(stream.width / ctur2_mode) *
				(stream.height / ctur2_mode) *
				((ppq_buf->bit[PQU_BUF_CH_1] + BIT_PER_BYTE - 1) / BIT_PER_BYTE)
				* ppq_buf->frame_num;
		ppq_buf->size_ch[PQU_BUF_CH_2] =
				((stream.width / ctur2_mode) + PQ_CTUR2_EXT_BIT) *
				((stream.height / ctur2_mode) + PQ_CTUR2_EXT_BIT) *
				((ppq_buf->bit[PQU_BUF_CH_2] + BIT_PER_BYTE - 1) / BIT_PER_BYTE)
				* ppq_buf->frame_num;
		break;
	case PQU_BUF_MCDI:
		ppq_buf->size_ch[PQU_BUF_CH_0] = MCDI_BUFFER_SIZE;
		ppq_buf->size_ch[PQU_BUF_CH_1] = MCDI_BUFFER_SIZE;
		ppq_buf->size_ch[PQU_BUF_CH_2] = 0;
		break;
	default:
		break;
	}

	return 0;
}

static int _mtk_pq_cut_reserved_memory(struct mtk_pq_platform_device *pqdev)
{
	enum pqu_buffer_type buf_idx = PQU_BUF_SCMI;
	int iommu_idx = 0, ret = 0, win = 0, buf_win_size = 0;
	struct device_node *np, *sub_np;
	const char *name;
	uint32_t mmap_size = 0;
	uint64_t mmap_addr = 0;
	struct pq_buffer **ppResBuf = NULL;
	struct device *property_dev = NULL;
	struct of_mmap_info_data of_mmap_info;

	if (pqdev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	ppResBuf = pqdev->display_dev.pReserveBufTbl;

	if (ppResBuf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	property_dev = pqdev->dev;

	for (buf_idx = PQU_BUF_SCMI; buf_idx < PQU_BUF_MAX; buf_idx++) {
		iommu_idx = mtk_pq_buf_get_iommu_idx(pqdev, buf_idx);

		if (iommu_idx == 0) {
			switch (buf_idx) {
			case PQU_BUF_ZNR:
				memset(&of_mmap_info, 0, sizeof(struct of_mmap_info_data));

				of_mtk_get_reserved_memory_info_by_idx(property_dev->of_node,
							MMAP_ZNR_INDEX, &of_mmap_info);

				mmap_addr = of_mmap_info.start_bus_address
							- BUSADDRESS_TO_IPADDRESS_OFFSET;
				mmap_size = of_mmap_info.buffer_size;
				break;
			case PQU_BUF_ABF:
				memset(&of_mmap_info, 0, sizeof(struct of_mmap_info_data));

				of_mtk_get_reserved_memory_info_by_idx(property_dev->of_node,
							MMAP_ABF_INDEX, &of_mmap_info);

				mmap_addr = of_mmap_info.start_bus_address
							- BUSADDRESS_TO_IPADDRESS_OFFSET;
				mmap_size = of_mmap_info.buffer_size;
				break;
			case PQU_BUF_MCDI:
				memset(&of_mmap_info, 0, sizeof(struct of_mmap_info_data));

				of_mtk_get_reserved_memory_info_by_idx(property_dev->of_node,
							MMAP_MCDI_INDEX, &of_mmap_info);

				mmap_addr = of_mmap_info.start_bus_address
							- BUSADDRESS_TO_IPADDRESS_OFFSET;
				mmap_size = of_mmap_info.buffer_size;
				break;
			default:
				break;
			}

			buf_win_size = mmap_size / pqdev->usable_win;

			for (win = 0; win < pqdev->usable_win; win++) {
				if (ppResBuf[win] == NULL) {
					STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"revserve table[%d] is not initialized\n", win);
					return -EPERM;
				}

				ppResBuf[win][buf_idx].size = buf_win_size;
				ppResBuf[win][buf_idx].addr = mmap_addr + (win * buf_win_size);
				ppResBuf[win][buf_idx].valid = true;

				STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
					"ppResBuf[%d][%d].size : 0x%llx\n",
					win, buf_idx, ppResBuf[win][buf_idx].size);

				STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
					"ppResBuf[%d][%d].addr : 0x%llx\n",
					win, buf_idx, ppResBuf[win][buf_idx].addr);
			}
		}
	}

	return 0;
}

static int _mtk_pq_get_reserved_memory(struct mtk_pq_device *pq_dev,
		enum pqu_buffer_type buf_type, unsigned long long *iova, uint32_t size)
{
	int ret = 0, win = 0;
	struct mtk_pq_platform_device *pqdev = NULL;
	struct pq_buffer **ppResBuf = NULL;

	if (iova == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	win = pq_dev->dev_indx;
	pqdev = dev_get_drvdata(pq_dev->dev);

	if (pqdev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	ppResBuf = pqdev->display_dev.pReserveBufTbl;

	if (ppResBuf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (ppResBuf[win] == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (ppResBuf[win][buf_type].size < size) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"mmap buf isn't enough : 0x%llx < 0x%llx\n",
		ppResBuf[win][buf_type].size, size);
		return -EPERM;
	}

	if (ppResBuf[win][buf_type].valid == false) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"mmap buf isn't in mmap : %d\n", buf_type);
		return -EPERM;
	}

	*iova = ppResBuf[win][buf_type].addr;

	return ret;
}

static bool _mtk_pq_get_idk_status(void)
{
#ifdef DOLBY_IDK_DUMP_ENABLE
	mm_segment_t cur_mm_seg;
	struct file *verfile = NULL;
	loff_t pos;
	char *pu8EnableFlag = NULL;
	int u32ReadCount = 0;

	// check if Dolby IDK is enabled
	pu8EnableFlag = vmalloc(sizeof(char) * SYSFS_MAX_BUF_COUNT);
	if (pu8EnableFlag == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "cannot malloc to read idk_dump node\n");
		return false;
	}

	cur_mm_seg = get_fs();
	set_fs(KERNEL_DS);
	verfile = filp_open("/sys/devices/platform/mtk-pq/mtk_dbg/mtk_pq_idkdump",
		O_RDONLY, 0);

	if (IS_ERR_OR_NULL(verfile)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "cannot open idk_dump node\n");
		set_fs(cur_mm_seg);
		vfree(pu8EnableFlag);
		return false;
	}

	pos = vfs_llseek(verfile, 0, SEEK_SET);
	u32ReadCount = kernel_read(verfile, pu8EnableFlag, SYSFS_MAX_BUF_COUNT, &pos);
	set_fs(cur_mm_seg);

	if (u32ReadCount < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "open idk_dump node error %d\n", u32ReadCount);
		vfree(pu8EnableFlag);
		filp_close(verfile, NULL);
		return false;
	}

	// check string "enable_status:%d\n", gu16IDKEnable);
	if (pu8EnableFlag[IDK_ENABLE_POSITION] == '1') {
		vfree(pu8EnableFlag);
		filp_close(verfile, NULL);
		return true;
	}

	vfree(pu8EnableFlag);
	filp_close(verfile, NULL);
	return false;
#else
	/* Dolby IDK not enabled */
	return false;
#endif
}

static int _mtk_pq_get_hwmap_info(struct pq_buffer *ppq_buf, enum pqu_buffer_type buf_type,
		struct meta_pq_stream_info stream)
{
	uint16_t src_type = 0, transition_type = 0;
	uint32_t label[1] = {0};
	hwmap_condition cond;

	if (ppq_buf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	src_type = stream.scenario_idx;
	transition_type = drv_hwreg_common_mapping_transition(src_type);

	switch (buf_type) {
	case PQU_BUF_SCMI:
		ppq_buf->frame_num = _mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_frame_num, src_type);
		ppq_buf->bit[PQU_BUF_CH_0] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_ch0_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_ch0_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_1] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_ch1_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_ch1_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_2] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_ch2_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_ch2_bit, transition_type));
		ppq_buf->frame_diff =
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_scmi_frame_diff, src_type);

		if (_mtk_pq_get_idk_status()) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "PQU 36 bit mode due to IDK dump\n");
			ppq_buf->bit[PQU_BUF_CH_0] = PQU_BIT_WIDTH_IDK;
			ppq_buf->bit[PQU_BUF_CH_1] = 0;
			ppq_buf->bit[PQU_BUF_CH_2] = 0;
		}

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "scmi frame num : %d, scmi frame diff : %d\n"
			, ppq_buf->frame_num, ppq_buf->frame_diff);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "scmi bit[0]:%d, bit[1]:%d, bit[2]:%d\n",
		ppq_buf->bit[PQU_BUF_CH_0], ppq_buf->bit[PQU_BUF_CH_1], ppq_buf->bit[PQU_BUF_CH_2]);
		break;
	case PQU_BUF_ZNR:
		ppq_buf->frame_num = _mtk_pq_transfer_ip_val(hwmap_ip_sw_znr_frame_num, src_type);
		ppq_buf->bit[PQU_BUF_CH_0] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_znr_ch0_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_znr_ch0_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_1] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_znr_ch1_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_znr_ch1_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_2] = 0;
		ppq_buf->frame_diff = 0;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "znr frame num : %d, znr frame diff : %d\n"
			, ppq_buf->frame_num, ppq_buf->frame_diff);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "znr bit[0]:%d, bit[1]:%d, bit[2]:%d\n",
		ppq_buf->bit[PQU_BUF_CH_0], ppq_buf->bit[PQU_BUF_CH_1], ppq_buf->bit[PQU_BUF_CH_2]);
		break;
	case PQU_BUF_UCM:
		ppq_buf->frame_num = _mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_frame_num, src_type);
		ppq_buf->bit[PQU_BUF_CH_0] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_ch0_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_ch0_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_1] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_ch1_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_ch1_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_2] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_ch2_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_ch2_bit, transition_type));
		ppq_buf->frame_diff =
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_ucm_frame_diff, src_type);

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "ucm frame num : %d, ucm frame diff : %d\n"
			, ppq_buf->frame_num, ppq_buf->frame_diff);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "ucm bit[0]:%d, bit[1]:%d, bit[2]:%d\n",
		ppq_buf->bit[PQU_BUF_CH_0], ppq_buf->bit[PQU_BUF_CH_1], ppq_buf->bit[PQU_BUF_CH_2]);
		break;
	case PQU_BUF_ABF:
		ppq_buf->frame_num = _mtk_pq_transfer_ip_val(hwmap_ip_sw_abf_frame_num, src_type);
		ppq_buf->bit[PQU_BUF_CH_0] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_abf_ch0_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_abf_ch0_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_1] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_abf_ch1_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_abf_ch1_bit, transition_type));
		ppq_buf->bit[PQU_BUF_CH_2] = MAX(
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_abf_ch2_bit, src_type),
			_mtk_pq_transfer_ip_val(hwmap_ip_sw_abf_ch2_bit, transition_type));
		ppq_buf->frame_diff = 0;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "abf frame num : %d, abf frame diff : %d\n"
			, ppq_buf->frame_num, ppq_buf->frame_diff);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "abf bit[0]:%d, bit[1]:%d, bit[2]:%d\n",
		ppq_buf->bit[PQU_BUF_CH_0], ppq_buf->bit[PQU_BUF_CH_1], ppq_buf->bit[PQU_BUF_CH_2]);
		break;
	case PQU_BUF_MCDI:
		ppq_buf->frame_num = _mtk_pq_transfer_ip_val(hwmap_ip_sw_mcdi_frame_num, src_type);
		ppq_buf->bit[PQU_BUF_CH_0] = 0;
		ppq_buf->bit[PQU_BUF_CH_1] = 0;
		ppq_buf->bit[PQU_BUF_CH_2] = 0;
		ppq_buf->frame_diff = 0;

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "mcdi frame num : %d, mcdi frame diff : %d\n"
			, ppq_buf->frame_num, ppq_buf->frame_diff);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "mcdi bit[0]:%d, bit[1]:%d, bit[2]:%d\n",
		ppq_buf->bit[PQU_BUF_CH_0], ppq_buf->bit[PQU_BUF_CH_1], ppq_buf->bit[PQU_BUF_CH_2]);
		break;
	default:
		break;
	}

	return 0;
}

static int _mtk_pq_allocate_buf(struct mtk_pq_device *pq_dev, enum pqu_buffer_type buf_type,
			struct pq_buffer *ppq_buf, struct meta_pq_stream_info stream_pq)
{
	struct mtk_pq_platform_device *pqdev = NULL;
	unsigned int buf_iommu_idx = 0;
	unsigned long iommu_attrs = 0;
	unsigned long long iova = 0, va = 0;
	int ret = 0;
	uint32_t size = 0;
	struct device *dev = NULL;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (ppq_buf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	dev = pq_dev->dev;
	pqdev = dev_get_drvdata(pq_dev->dev);
	buf_iommu_idx = mtk_pq_buf_get_iommu_idx(pqdev, buf_type);

	if (dma_get_mask(dev) < DMA_BIT_MASK(34)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "IOMMU isn't registered\n");
		return -1;
	}

	if (!dma_supported(dev, 0)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"IOMMU is not supported\n");
		return -1;
	}

	ret = _mtk_pq_cal_buf_size(ppq_buf, buf_type, stream_pq);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_mtk_pq_cal_buf_size Failed, ret = %d\n", ret);
		return ret;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"%s%x, %s%lx, %s%lx, %s%lx\n",
		"buf_type : ", buf_type,
		"size_ch[PQU_BUF_CH_0] : ", ppq_buf->size_ch[PQU_BUF_CH_0],
		"size_ch[PQU_BUF_CH_1] : ", ppq_buf->size_ch[PQU_BUF_CH_1],
		"size_ch[PQU_BUF_CH_2] : ", ppq_buf->size_ch[PQU_BUF_CH_2]);

	size = ppq_buf->size_ch[PQU_BUF_CH_0] + ppq_buf->size_ch[PQU_BUF_CH_1]
							+ ppq_buf->size_ch[PQU_BUF_CH_2];
	if (buf_iommu_idx != 0) {
		// set buf tag
		iommu_attrs |= buf_iommu_idx;

		// if mapping uncache
		iommu_attrs |= DMA_ATTR_WRITE_COMBINE;

		if (size == 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"PQ buffer alloc size = 0\n");
			return -1;
		}

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"buf_type : %d, buf_iommu_idx : %x, size = %llx\n",
			buf_type, buf_iommu_idx, size);

		va = (unsigned long long)dma_alloc_attrs(dev,
				size,
				&iova,
				GFP_KERNEL,
				iommu_attrs);
		ret = dma_mapping_error(dev, iova);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
				"dma_alloc_attrs fail\n");
			return ret;
		}
	} else {
		ret = _mtk_pq_get_reserved_memory(pq_dev, buf_type, &iova, size);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"_mtk_pq_get_reserved_memory Failed, ret = %d\n", ret);
			return ret;
		}
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"buf = {va : %llx, addr : %llx, size = %llx}\n", va, iova, size);

	ppq_buf->va = va;
	ppq_buf->addr = iova;
	ppq_buf->size = size;

	return ret;
}

static int _mtk_pq_release_buf(struct mtk_pq_device *pq_dev,
	enum pqu_buffer_type buf_type, struct pq_buffer *ppq_buf)
{
	struct device *dev = NULL;
	struct mtk_pq_platform_device *pqdev = NULL;
	unsigned int buf_iommu_idx = 0;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (ppq_buf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	dev = pq_dev->dev;
	pqdev = dev_get_drvdata(pq_dev->dev);
	buf_iommu_idx = mtk_pq_buf_get_iommu_idx(pqdev, buf_type);

	/* clear valid flag first to avoid racing condition */
	ppq_buf->valid = false;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"buf_type : %d, buf_iommu_idx, size : %llx : %x\n",
		buf_type, buf_iommu_idx, ppq_buf->size);

	if (buf_iommu_idx != 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"va : %llx, addr : %llx\n", ppq_buf->va, ppq_buf->addr);

		dma_free_attrs(dev, ppq_buf->size, (void *)ppq_buf->va,
			ppq_buf->addr, buf_iommu_idx);
	}

	return 0;
}

int mtk_pq_buf_get_iommu_idx(struct mtk_pq_platform_device *pqdev, enum pqu_buffer_type buf_type)
{
	int idx = 0;

	if (pqdev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	switch (buf_type) {
	case PQU_BUF_SCMI:
		idx = pqdev->display_dev.pq_iommu_idx_scmi;
		break;
	case PQU_BUF_UCM:
		idx = pqdev->display_dev.pq_iommu_idx_ucm;
		break;
	default:
		/* buffer type does not support iommu */
		break;
	}

	return (idx << pqdev->display_dev.buf_iommu_offset);
}

int mtk_pq_buffer_reserve_buf_init(struct mtk_pq_platform_device *pqdev)
{
	uint8_t idx = 0;
	struct pq_buffer **ppResBuf = NULL;

	if (pqdev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (pqdev->display_dev.pReserveBufTbl != NULL)
		mtk_pq_buffer_reserve_buf_exit(pqdev);

	pqdev->display_dev.pReserveBufTbl
		= (struct pq_buffer **)malloc(pqdev->usable_win * sizeof(struct pq_buffer *));

	ppResBuf = pqdev->display_dev.pReserveBufTbl;

	if (ppResBuf != NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"pq reserve buffer allocate succeesful : %llx\n", ppResBuf);

		memset(ppResBuf, 0, pqdev->usable_win * sizeof(struct pq_buffer *));

		for (idx = 0; idx < pqdev->usable_win; idx++) {
			ppResBuf[idx] =
				(struct pq_buffer *)malloc(PQU_BUF_MAX * sizeof(struct pq_buffer));

			STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
				"pq[%d] reserve buffer allocate succeesful : %llx\n",
				idx, ppResBuf[idx]);

			if (ppResBuf[idx] == NULL) {
				mtk_pq_buffer_reserve_buf_exit(pqdev);

				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"pq[%d] reserve buffer allocate failed!\n", idx);
				return -EPERM;
			}
		}

		_mtk_pq_cut_reserved_memory(pqdev);

	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"pq reserve buffer allocate failed!\n");
		return -EPERM;
	}

	return 0;
}

int mtk_pq_buffer_reserve_buf_exit(struct mtk_pq_platform_device *pqdev)
{
	uint8_t idx = 0;
	struct pq_buffer **ppResBuf = NULL;

	if (pqdev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	ppResBuf = pqdev->display_dev.pReserveBufTbl;

	if (ppResBuf != NULL) {
		for (idx = 0; idx < pqdev->usable_win; idx++) {
			if (ppResBuf[idx] != NULL) {
				free(ppResBuf[idx]);
				ppResBuf[idx] = NULL;
			}
		}

		free(ppResBuf);
		ppResBuf = NULL;
	}

	return 0;
}

int mtk_pq_buffer_buf_init(struct mtk_pq_device *pq_dev)
{
	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	memset(pq_dev->display_info.BufferTable, 0,
			PQU_BUF_MAX*sizeof(struct pq_buffer));

	return 0;
}

int mtk_pq_buffer_buf_exit(struct mtk_pq_device *pq_dev)
{
	enum pqu_buffer_type idx = PQU_BUF_SCMI;
	struct pq_buffer *ppq_buf = NULL;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	for (idx = PQU_BUF_SCMI; idx < PQU_BUF_MAX; idx++) {
		ppq_buf = &(pq_dev->display_info.BufferTable[idx]);

		if (ppq_buf->valid == true)
			_mtk_pq_release_buf(pq_dev, idx, ppq_buf);
	}

	memset(&(pq_dev->display_info.BufferTable), 0, PQU_BUF_MAX*sizeof(struct pq_buffer));

	return 0;
}

int mtk_pq_buffer_get_hwmap_info(struct mtk_pq_device *pq_dev)
{
	enum pqu_buffer_type buf_idx = PQU_BUF_SCMI;
	struct pq_buffer *pBufferTable = NULL;
	struct meta_pq_stream_info stream_pq;
	int ret = 0;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	pBufferTable = pq_dev->display_info.BufferTable;

	memset(&stream_pq, 0, sizeof(struct meta_pq_stream_info));

	/* read meta from stream metadata */
	ret = mtk_pq_common_read_metadata(pq_dev->stream_meta.fd,
		EN_PQ_METATAG_STREAM_INFO, &stream_pq);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_read_metadata Failed, ret = %d\n", ret);
		return ret;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "stream.width: 0x%llx, stream.height: 0x%llx\n",
						stream_pq.width, stream_pq.height);

	for (buf_idx = PQU_BUF_SCMI; buf_idx < PQU_BUF_MAX; buf_idx++) {
		struct pq_buffer *ppq_buf = &(pBufferTable[buf_idx]);

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "buf_idx, %d\n", buf_idx);

		ret = _mtk_pq_get_hwmap_info(ppq_buf, buf_idx, stream_pq);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"_mtk_pq_get_hwmap_info Failed, ret = %d\n", ret);
			return ret;
		}
	}

	return ret;
}

int mtk_pq_buffer_allocate(struct mtk_pq_device *pq_dev)
{
	enum pqu_buffer_type buf_idx = PQU_BUF_SCMI;
	enum pqu_buffer_channel ch_idx = PQU_BUF_CH_0;
	struct pq_buffer *pBufferTable = NULL;
	struct meta_pq_stream_info stream_pq;
	int ret = 0;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	pBufferTable = pq_dev->display_info.BufferTable;

	memset(&stream_pq, 0, sizeof(struct meta_pq_stream_info));

	/* read meta from stream metadata */
	ret = mtk_pq_common_read_metadata(pq_dev->stream_meta.fd,
		EN_PQ_METATAG_STREAM_INFO, &stream_pq);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_read_metadata Failed, ret = %d\n", ret);
		return ret;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "stream.width: 0x%llx, stream.height: 0x%llx\n",
						stream_pq.width, stream_pq.height);

	for (buf_idx = PQU_BUF_SCMI; buf_idx < PQU_BUF_MAX; buf_idx++) {
		struct pq_buffer *ppq_buf = &(pBufferTable[buf_idx]);

		if (ppq_buf->frame_num != 0) {
			if (ppq_buf->valid == true)
				_mtk_pq_release_buf(pq_dev, buf_idx, ppq_buf);

			ret = _mtk_pq_allocate_buf(pq_dev, buf_idx, ppq_buf, stream_pq);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_allocate_buf Failed, ret = %d\n", ret);
				return ret;
			}

			for (ch_idx = PQU_BUF_CH_0; ch_idx < PQU_BUF_CH_MAX; ch_idx++) {
				if (ch_idx == PQU_BUF_CH_0)
					ppq_buf->addr_ch[ch_idx] = ppq_buf->addr;
				else
					ppq_buf->addr_ch[ch_idx] =
						ppq_buf->addr_ch[ch_idx-1] +
						ppq_buf->size_ch[ch_idx-1];
			}

			if ((buf_idx == PQU_BUF_SCMI) || (buf_idx == PQU_BUF_UCM))
				ppq_buf->sec_buf = true;

			ppq_buf->valid = true;
		} else {
			if (ppq_buf->valid == true)
				_mtk_pq_release_buf(pq_dev, buf_idx, ppq_buf);
		}
	}

	return ret;
}

int mtk_pq_buffer_release(struct mtk_pq_device *pq_dev)
{
	enum pqu_buffer_type buf_idx = PQU_BUF_SCMI;
	struct pq_buffer *pBufferTable = NULL;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	pBufferTable = pq_dev->display_info.BufferTable;

	for (buf_idx = PQU_BUF_SCMI; buf_idx < PQU_BUF_MAX; buf_idx++) {
		struct pq_buffer *ppq_buf = &(pBufferTable[buf_idx]);

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER, "buf_idx, %d\n", buf_idx);

		if (ppq_buf->valid == true) {
			if ((buf_idx == PQU_BUF_SCMI) || (buf_idx == PQU_BUF_UCM))
				ppq_buf->sec_buf = false;

			_mtk_pq_release_buf(pq_dev, buf_idx, ppq_buf);
		}
	}

	return 0;
}

int mtk_get_pq_buffer(struct mtk_pq_device *pq_dev, enum pqu_buffer_type buftype,
	struct pq_buffer *ppq_buf)
{
	int ret = 1;
	struct pq_buffer *pBufferTable = NULL;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (ppq_buf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	pBufferTable = pq_dev->display_info.BufferTable;

	if (buftype >= PQU_BUF_MAX) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"buftype is not valid : %d\n", buftype);
		return -EPERM;
	}

	ppq_buf->valid = pBufferTable[buftype].valid;
	ppq_buf->size = pBufferTable[buftype].size;
	ppq_buf->frame_num = pBufferTable[buftype].frame_num;
	ppq_buf->frame_diff = pBufferTable[buftype].frame_diff;
	ppq_buf->va = pBufferTable[buftype].va;
	ppq_buf->addr = pBufferTable[buftype].addr;
	memcpy(ppq_buf->addr_ch, pBufferTable[buftype].addr_ch, sizeof(__u64) * PQU_BUF_CH_MAX);
	memcpy(ppq_buf->size_ch, pBufferTable[buftype].size_ch, sizeof(__u32) * PQU_BUF_CH_MAX);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"valid : %d, size : %llx\n", ppq_buf->valid, ppq_buf->size);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
		"va : %llx, addr : %llx\n", ppq_buf->va, ppq_buf->addr);

	return ret;
}

