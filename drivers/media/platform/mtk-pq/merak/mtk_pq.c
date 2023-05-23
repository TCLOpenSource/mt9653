// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>
#include <media/videobuf2-dma-sg.h>
#include <linux/io.h>

#include "mtk_pq.h"
#include "mtk_iommu_dtv_api.h"
#include "mtk_pq_common_irq.h"
#include "mtk_pq_svp.h"
#include "mtk_pq_buffer.h"
#include "mtk_pq_hdr.h"
#include "mtk_pq_thermal.h"

#include "apiXC.h"
#include "hwreg_common_bin.h"
#include "pqu_msg.h"

#define PQ_V4L2_DEVICE_NAME		"v4l2-mtk-pq"
#define PQ_VIDEO_DRIVER_NAME		"mtk-pq-drv"
#define PQ_VIDEO_DEVICE_NAME		"mtk-pq-dev"
#define PQ_VIDEO_DEVICE_NAME_LENGTH	20
#define fh_to_ctx(f)	(container_of(f, struct mtk_pq_ctx, fh))
#define ALIGN_DOWNTO_16(x)  ((x >> 4) << 4)
#define STI_PQ_LOG_OFF 0

__u32 u32DbgLevel = STI_PQ_LOG_OFF;
struct mtk_pq_platform_device *pqdev;
bool bPquEnable;
__u32 atrace_enable_pq;

static char *devID[PQ_WIN_MAX_NUM] = {"0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", "10", "11", "12", "13", "14", "15"};

MS_PQBin_Header_Info stPQBinHeaderInfo;
struct msg_config_info config_info;

static const struct of_device_id mtk_pq_match[] = {
	{.compatible = "mediatek,pq",},
	{},
};

//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
//static ssize_t mtk_pq_capability_show(struct device *dev,
	//struct device_attribute *attr, char *buf);
//static ssize_t mtk_pq_capability_store(struct device *dev,
	//struct device_attribute *attr, const char *buf, size_t count);

//-----------------------------------------------------------------------------
//  Local Functions
//-----------------------------------------------------------------------------
static int _mtk_pq_get_memory_info(struct device *dev, int pool_index,
	__u64 *pbus_address, __u32 *pbus_size)
{
	int ret = 0;
	struct of_mmap_info_data of_mmap_info;

	if (dev == NULL || pbus_address == NULL || pbus_size == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "NULL pointer\n");
		return -EINVAL;
	}

	memset(&of_mmap_info, 0, sizeof(struct of_mmap_info_data));
	of_mtk_get_reserved_memory_info_by_idx(dev->of_node, pool_index, &of_mmap_info);
	*pbus_address = of_mmap_info.start_bus_address;
	*pbus_size = of_mmap_info.buffer_size;

	if (*pbus_address == 0 || *pbus_size == 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid bus address\n");
		return -EINVAL;
	}
	return 0;
}

static __u8 *_mtk_pq_config_open_bin(__u8 *pName, __u64 *filesize, bool is_share)
{
	long lfilezize;
	__u32 ret;
	const char *filename = NULL;
	__u8 *buffer = NULL;
	struct file *pFile = NULL;
	mm_segment_t cur_mm_seg;
	loff_t pos;

	filename = (const char *)pName;
	pFile = filp_open(filename, O_RDONLY, 0);

	if (pFile == NULL)
		return NULL;

	vfs_llseek(pFile, 0, SEEK_END);
	lfilezize = pFile->f_pos;

	*filesize = lfilezize;
	if (lfilezize <= 0) {
		filp_close(pFile, NULL);
		return NULL;
	}

	pos = vfs_llseek(pFile, 0, SEEK_SET);

	if (is_share) {
		if (!pqdev) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
			goto exit;
		}
		if (pqdev->hwmap_config.va == NULL) {
			ret = _mtk_pq_get_memory_info(pqdev->dev, MMAP_CONTROLMAP_INDEX,
						&pqdev->hwmap_config.pa, &pqdev->hwmap_config.size);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"Failed to _mtk_pq_get_memory_info(index=%d)\n",
					MMAP_CONTROLMAP_INDEX);
				goto exit;
			}
			pqdev->hwmap_config.va =
				(__u64)memremap(pqdev->hwmap_config.pa, pqdev->hwmap_config.size,
						MEMREMAP_WC);
			if (pqdev->hwmap_config.va == NULL) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"Failed to memremap(0x%llx, 0x%x)\n",
					pqdev->hwmap_config.pa, pqdev->hwmap_config.size);
				goto exit;
			}
		}
		buffer = pqdev->hwmap_config.va;
	} else {
		buffer = vmalloc(lfilezize);
	}

	if (buffer == NULL) {
		goto exit;
	}
	cur_mm_seg = get_fs();
	set_fs(KERNEL_DS);
	ret = kernel_read(pFile, (char *)buffer, lfilezize, &pos);
	set_fs(cur_mm_seg);
	if (ret != (__u32)lfilezize) {
		if (!is_share)
			vfree(buffer);
		filp_close(pFile, NULL);
		return NULL;
	}

exit:
	filp_close(pFile, NULL);

	return buffer;
}

static int _mtk_pq_read_bin_file(__u8 *pName, __u8 *buffer, __u64 *filesize)
{
	int ret = 0;
	const char *filename;
	struct file *pFile = NULL;
	long lfilezize;
	loff_t pos;
	mm_segment_t cur_mm_seg;
	ssize_t read_size = 0;

	/* check input */
	if (pName == NULL || buffer == NULL || filesize == NULL) {
		ret = -EINVAL;
		goto exit;
	}

	/* open file */
	filename = (const char *)pName;
	pFile = filp_open(filename, O_RDONLY, 0);
	if (pFile == NULL) {
		ret = -ENOENT;
		goto exit;
	}

	/* file size */
	vfs_llseek(pFile, 0, SEEK_END);
	lfilezize = pFile->f_pos;
	if (lfilezize <= 0) {
		ret = -EPERM;
		goto exit;
	}

	/* read file */
	pos = vfs_llseek(pFile, 0, SEEK_SET);
	cur_mm_seg = get_fs();
	set_fs(KERNEL_DS);
	read_size = kernel_read(pFile, (char *)buffer, lfilezize, &pos);
	set_fs(cur_mm_seg);
	if (read_size != (__u32)lfilezize) {
		ret = -EIO;
		goto exit;
	}

	*filesize = lfilezize;
	ret = 0;

exit:
	if (pFile)
		filp_close(pFile, NULL);

	return ret;
}

static v4l2_PQ_dv_config_info_t *_mtk_pq_init_dolby_config(void)
{
	int ret = 0;
	v4l2_PQ_dv_config_info_t *dv_config_info = NULL;

	/* get dolby config memory */
	if (!pqdev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}
	ret = _mtk_pq_get_memory_info(pqdev->dev, MMAP_DV_CONFIG_INDEX,
					&pqdev->dv_config.pa, &pqdev->dv_config.size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"Failed to _mtk_pq_get_memory_info(index=%d)\n", MMAP_DV_CONFIG_INDEX);
		return -EINVAL;
	}
	pqdev->dv_config.va =
		(__u64)memremap(pqdev->dv_config.pa, pqdev->dv_config.size, MEMREMAP_WB);
	if (pqdev->dv_config.va == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Failed to memremap(0x%llx, 0x%x)\n",
			pqdev->dv_config.pa, pqdev->dv_config.size);
		return -EINVAL;
	}

	dv_config_info = (v4l2_PQ_dv_config_info_t *)pqdev->dv_config.va;
	memset(dv_config_info, 0, sizeof(v4l2_PQ_dv_config_info_t));

	dv_config_info->bin_info.pa = (__u64)pqdev->dv_config.pa + DV_CONFIG_BIN_OFFSET;
	dv_config_info->bin_info.va = (__u64)pqdev->dv_config.va + DV_CONFIG_BIN_OFFSET;

	/* default dolby bin */
	dv_config_info->bin_info.size = DV_CONFIG_BIN_DEFAULT_SIZE;
	dv_config_info->bin_info.en = true;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "dolby bin: va=0x%llx, pa=0x%llx, size=%ld\n",
		(__u64)dv_config_info->bin_info.va,
		(__u64)dv_config_info->bin_info.pa,
		(__u64)dv_config_info->bin_info.size);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "dolby bin: en=%d (bin_info = %d)\n",
		dv_config_info->bin_info.en,
		sizeof(dv_config_info->bin_info));
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON,
		"dolby picture mode: en=%d, num=%d (mode_info size = %d)\n",
		dv_config_info->mode_info.en,
		dv_config_info->mode_info.num,
		sizeof(dv_config_info->mode_info));

	return dv_config_info;
}

static int _mtk_pq_config_open(struct mtk_pq_platform_device *pdev)
{
	int ret = 0;
	struct device_node *bank_node;
	const char const config_ini_path[PQ_FILE_PATH_LENGTH] = "/vendor/firmware/HWMAP.bin";
	const char const config_default_path[PQ_FILE_PATH_LENGTH] = "/config/pq/HWMAP.bin";
	char config_bin_path[PQ_FILE_PATH_LENGTH] = {0};
	__u64 filesize = 0;
	struct file *pFile = NULL;
	struct device_node *np;
	__u16 idx = 0;
	v4l2_PQ_dv_config_info_t *dv_config_info = NULL;

	//irq
	np = of_find_matching_node(NULL, mtk_pq_match);
	if (np != NULL) {
		idx = of_irq_get(np, 1);
		if (idx < 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "of_irq_get failed\n");
			return -EINVAL;
		}
	}
	config_info.idx = idx;
	config_info.idx_en = true;

	// open config file
	pFile = (struct file *)filp_open(config_ini_path, O_RDONLY, 0);
	if (IS_ERR(pFile)) {
		pFile = (struct file *)filp_open(config_default_path, O_RDONLY, 0);
		strncpy(config_bin_path, config_default_path, PQ_FILE_PATH_LENGTH);
	} else {
		strncpy(config_bin_path, config_ini_path, PQ_FILE_PATH_LENGTH);
	}

	if (IS_ERR(pFile)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "%s is %ld\n", config_bin_path, PTR_ERR(pFile));
		pFile = NULL;
	} else {
		filp_close(pFile, NULL);
		pFile = NULL;
		//load common
		pdev->hwmap_config.va = (__u64)_mtk_pq_config_open_bin(
								(__u8 *)config_bin_path,
								&filesize,
								true);
		stPQBinHeaderInfo.u32BinStartAddress = (size_t)pdev->hwmap_config.va;

		// send msg to pqu
		config_info.config = stPQBinHeaderInfo.u32BinStartAddress;
		config_info.config_en = true;
	}

	/* init dolby config */
	dv_config_info = _mtk_pq_init_dolby_config();

	/* send msg to pqu */
	config_info.dvconfig = (__u64)dv_config_info;
	config_info.dvconfigsize = sizeof(v4l2_PQ_dv_config_info_t);
	config_info.dvconfig_en = true;
	config_info.dvconfig = pdev->dv_config.va;

	//irq version
	config_info.u32IRQ_Version = pdev->pqcaps.u32IRQ_Version;

	//config table version
	config_info.u32Config_Version = pdev->pqcaps.u32Config_Version;

	//config table version
	config_info.u32DV_PQ_Version = pdev->pqcaps.u32DV_PQ_Version;

	config_info.u16HSY_PQ_Version = (u16)pdev->pqcaps.u32HSY_Version;

	mtk_pq_common_config(&config_info, false);

	return ret;
}

static int _mtk_pq_set_parent_clock(struct mtk_pq_platform_device *pdev, int clk_index,
					char *dev_clk_name, bool enable)
{
	int ret = 0;
	struct clk *dev_clk;
	struct clk_hw *clk_hw;
	struct clk_hw *parent_clk_hw;

	dev_clk = devm_clk_get(pdev->dev, dev_clk_name);
	if (IS_ERR(dev_clk)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "failed to get %s\n", dev_clk_name);
		return -ENODEV;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "clk_name = %s\n", __clk_get_name(dev_clk));

	//the way 2 : get change_parent clk
	clk_hw = __clk_get_hw(dev_clk);
	parent_clk_hw = clk_hw_get_parent_by_index(clk_hw, clk_index);
	if (IS_ERR(parent_clk_hw)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "failed to get parent_clk_hw\n");
		return -ENODEV;
	}

	//set_parent clk
	ret = clk_set_parent(dev_clk, parent_clk_hw->clk);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "failed to change clk_index [%d]\n", clk_index);
		return -ENODEV;
	}

	if (enable == true) {
		//prepare and enable clk
		ret = clk_prepare_enable(dev_clk);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"failed to enable clk_index [%d]\n", clk_index);
			return -ENODEV;
		}
	} else
		clk_disable_unprepare(dev_clk);

	devm_clk_put(pdev->dev, dev_clk);

	return ret;
}

static int _mtk_pq_set_clock(struct mtk_pq_platform_device *pdev, bool enable)
{
	int ret = 0;

	int clk_xc_ed_index = 1;
	//"0:xtal_vcod1_24m_ck","1:xcpll_vcod1_720m_xc_ed_buf_ck","2:xcpll_vcod1_720m_ck"
	int clk_xc_fn_index = 1;
	//"0:xtal_vcod1_24m_ck","1:xcpll_vcod1_720m_xc_fn_buf_ck","2:xcpll_vcod1_720m_ck"
	int clk_xc_fs_index = 1;
	//"0:xtal_vcod1_24m_ck","1:xcpll_vcod1_720m_xc_fs_buf_ck","2:xcpll_vcod1_720m_ck"
	int clk_xc_fd_index = 1;
	//"0:xtal_vcod1_24m_ck","1:xcpll_vcod1_720m_xc_fd_buf_ck","2:xcpll_vcod1_720m_ck"
	int clk_xc_srs_index = 1;
	//"0:xtal_vcod1_24m_ck","1:sys2pll_vcod3_480m_ck"
	int clk_xc_slow_index = 1;
	//"0:xtal_vcod1_24m_ck","1:sys2pll_vcod4_360m_ck"
	int clk_sr_sram_index = 1;
	//"0:xc_srs_ck","1:xc_fn_ck","2:xc_630m_ck"

	_mtk_pq_set_parent_clock(pdev, clk_xc_ed_index, "clk_xc_ed", enable);
	_mtk_pq_set_parent_clock(pdev, clk_xc_fn_index, "clk_xc_fn", enable);
	_mtk_pq_set_parent_clock(pdev, clk_xc_fs_index, "clk_xc_fs", enable);
	_mtk_pq_set_parent_clock(pdev, clk_xc_fd_index, "clk_xc_fd", enable);
	_mtk_pq_set_parent_clock(pdev, clk_xc_srs_index, "clk_xc_srs", enable);
	_mtk_pq_set_parent_clock(pdev, clk_xc_slow_index, "clk_xc_slow", enable);

	if ((pdev->pqcaps.u32AISR_Support) && (pdev->pqcaps.u32AISR_Version == 0))
		_mtk_pq_set_parent_clock(pdev, clk_sr_sram_index, "clk_xc_sr_sram", enable);

	return ret;
}

static int _mtk_pq_parse_dts_clock_target(struct mtk_pq_platform_device *pdev)
{

	int ret = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	struct mtk_pq_caps *pqcaps = &pdev->pqcaps;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "clock-target");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get clock-target node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "XC_ED_TARGET_CLK",
		&pqcaps->u32XC_ED_TARGET_CLK);
	if (ret) {
		PQ_MSG_ERROR("Failed to get XC_ED_TARGET_CLK resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "XC_FN_TARGET_CLK",
		&pqcaps->u32XC_FN_TARGET_CLK);
	if (ret) {
		PQ_MSG_ERROR("Failed to get XC_FN_TARGET_CLK resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "XC_FS_TARGET_CLK",
		&pqcaps->u32XC_FS_TARGET_CLK);
	if (ret) {
		PQ_MSG_ERROR("Failed to get XC_FS_TARGET_CLK resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "XC_FD_TARGET_CLK",
		&pqcaps->u32XC_FD_TARGET_CLK);
	if (ret) {
		PQ_MSG_ERROR("Failed to get XC_FD_TARGET_CLK resource\r\n");
		goto Fail;
	}

	of_node_put(bank_node);

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;
}

static int _mtk_pq_parse_dts_capbability_misc(struct mtk_pq_platform_device *pdev)
{

	int ret = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	struct mtk_pq_caps *pqcaps = &pdev->pqcaps;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get capability node\r\n");
		return -EINVAL;
	}

	ret |= of_property_read_u32(bank_node, "TCH_color",
		&pqcaps->u32TCH_color);
	PQ_CAPABILITY_CHECK_RET(ret, "TCH_color");
	PQ_MSG_INFO("TCH_color = %x\r\n", pqcaps->u32TCH_color);

	ret |= of_property_read_u32(bank_node, "PreSharp",
		&pqcaps->u32PreSharp);
	PQ_CAPABILITY_CHECK_RET(ret, "PreSharp");
	PQ_MSG_INFO("PreSharp = %x\r\n", pqcaps->u32PreSharp);

	ret |= of_property_read_u32(bank_node, "2D_Peaking",
		&pqcaps->u322D_Peaking);
	PQ_CAPABILITY_CHECK_RET(ret, "2D_Peaking");
	PQ_MSG_INFO("2D_Peaking = %x\r\n", pqcaps->u322D_Peaking);

	ret |= of_property_read_u32(bank_node, "LCE", &pqcaps->u32LCE);
	PQ_CAPABILITY_CHECK_RET(ret, "LCE");
	PQ_MSG_INFO("LCE = %x\r\n", pqcaps->u32LCE);

	ret |= of_property_read_u32(bank_node, "3D_LUT",
		&pqcaps->u323D_LUT);
	PQ_CAPABILITY_CHECK_RET(ret, "3D_LUT");
	PQ_MSG_INFO("3D_LUT = %x\r\n", pqcaps->u323D_LUT);

	ret |= of_property_read_u32(bank_node, "DMA_SCL",
		&pqcaps->u32DMA_SCL);
	PQ_CAPABILITY_CHECK_RET(ret, "DMA_SCL");
	PQ_MSG_INFO("DMA_SCL = %x\r\n", pqcaps->u32DMA_SCL);

	ret |= of_property_read_u32(bank_node, "PQU", &pqcaps->u32PQU);
	PQ_CAPABILITY_CHECK_RET(ret, "PQU");
	PQ_MSG_INFO("PQU = %x\r\n", pqcaps->u32PQU);

	ret = of_property_read_u32(bank_node, "Window_Num",
		&pqcaps->u32Window_Num);
	PQ_CAPABILITY_CHECK_RET(ret, "Window_Num");
	PQ_MSG_INFO("Window_Num = %x\r\n", pqcaps->u32Window_Num);

	ret |= of_property_read_u32(bank_node, "SRS_SUPPORT",
		&pqcaps->u32SRS_Support);
	PQ_CAPABILITY_CHECK_RET(ret, "SRS_SUPPORT");
	PQ_MSG_INFO("SRS_SUPPORT = %x\r\n", pqcaps->u32SRS_Support);

	ret |= of_property_read_u32(bank_node, "AISR_SUPPORT",
		&pqcaps->u32AISR_Support);
	PQ_CAPABILITY_CHECK_RET(ret, "AISR_SUPPORT");
	PQ_MSG_INFO("AISR_SUPPORT = %x\r\n", pqcaps->u32AISR_Support);

	ret |= of_property_read_u32(bank_node, "Phase_IP2",
		&pqcaps->u32Phase_IP2);
	PQ_CAPABILITY_CHECK_RET(ret, "Phase_IP2");
	PQ_MSG_INFO("Phase_IP2 = %x\r\n", pqcaps->u32Phase_IP2);

	ret |= of_property_read_u32(bank_node, "AISR_VERSION",
		&pqcaps->u32AISR_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "AISR_VERSION");
	PQ_MSG_INFO("AISR_VERSION = %x\r\n", pqcaps->u32AISR_Version);

	ret |= of_property_read_u32(bank_node, "HSY_SUPPORT",
		&pqcaps->u32HSY_Support);
	PQ_CAPABILITY_CHECK_RET(ret, "HSY_SUPPORT");
	PQ_MSG_INFO("HSY_SUPPORT = %x\r\n", pqcaps->u32HSY_Support);

	ret |= of_property_read_u32(bank_node, "IRQ_Version",
		&pqcaps->u32IRQ_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "IRQ_Version");
	PQ_MSG_INFO("IRQ_Version = %x\r\n", pqcaps->u32IRQ_Version);

	ret |= of_property_read_u32(bank_node, "CFD_PQ_HWVersion",
		&pqcaps->u32CFD_PQ_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "CFD_PQ_HWVersion");
	PQ_MSG_INFO("CFD_PQ_HWVersion = %x\r\n", pqcaps->u32CFD_PQ_Version);

	ret |= of_property_read_u32(bank_node, "MDW_VERSION",
		&pqcaps->u32MDW_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "MDW_VERSION");
	PQ_MSG_INFO("MDW_VERSION = %x\r\n", pqcaps->u32MDW_Version);

	ret |= of_property_read_u32(bank_node, "Config_Version",
		&pqcaps->u32Config_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "Config_Version");
	PQ_MSG_INFO("Config_Version = %x\r\n", pqcaps->u32Config_Version);

	ret |= of_property_read_u32(bank_node, "IDR_SWMODE_SUPPORT",
		&pqcaps->u32Idr_SwMode_Support);
	PQ_CAPABILITY_CHECK_RET(ret, "IDR_SWMODE_SUPPORT");
	PQ_MSG_INFO("IDR_SWMODE_SUPPORT = %x\r\n", pqcaps->u32Idr_SwMode_Support);

	ret |= of_property_read_u32(bank_node, "DV_PQ_Version",
		&pqcaps->u32DV_PQ_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "DV_PQ_Version");
	PQ_MSG_INFO("DV_PQ_Version = %x\r\n", pqcaps->u32DV_PQ_Version);

	ret |= of_property_read_u32(bank_node, "HSY_VERSION",
		&pqcaps->u32HSY_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "HSY_VERSION");
	PQ_MSG_INFO("HSY_VERSION = %x\r\n", pqcaps->u32HSY_Version);

	ret |= of_property_read_u32(bank_node, "QMAP_VERSION",
		&pqcaps->u32Qmap_Version);
	PQ_CAPABILITY_CHECK_RET(ret, "QMAP_VERSION");
	PQ_MSG_INFO("QMAP_VERSION = %x\r\n", pqcaps->u32Qmap_Version);

	if (ret)
		return -EINVAL;

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;

}

static int _mtk_pq_parse_dts_reg(struct mtk_pq_platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	struct mtk_pq_caps *pqcaps = &pdev->pqcaps;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get capability node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "FULL_PQ_I_Register",
		&pqcaps->u32FULL_PQ_I_Register);
	if (ret) {
		PQ_MSG_ERROR("Failed to get FULL_PQ_I_Register resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "FULL_PQ_P_Register",
		&pqcaps->u32FULL_PQ_P_Register);
	if (ret) {
		PQ_MSG_ERROR("Failed to get FULL_PQ_P_Register resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LITE_PQ_I_Register",
		&pqcaps->u32LITE_PQ_I_Register);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LITE_PQ_I_Register resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LITE_PQ_P_Register",
		&pqcaps->u32LITE_PQ_P_Register);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LITE_PQ_P_Register resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "ZFD_PQ_I_Register",
		&pqcaps->u32ZFD_PQ_I_Register);
	if (ret) {
		PQ_MSG_ERROR("Failed to get ZFD_PQ_I_Register resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "ZFD_PQ_P_Register",
		&pqcaps->u32ZFD_PQ_P_Register);
	if (ret) {
		PQ_MSG_ERROR("Failed to get ZFD_PQ_P_Register resource\r\n");
		goto Fail;
	}

	of_node_put(bank_node);

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;
}

static int _mtk_pq_parse_dts_performance(struct mtk_pq_platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	struct mtk_pq_caps *pqcaps = &pdev->pqcaps;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get capability node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "FULL_PQ_I_Performance",
		&pqcaps->u32FULL_PQ_I_Performance);
	if (ret) {
		PQ_MSG_ERROR("Failed to get FULL_PQ_I_Performance resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "FULL_PQ_P_Performance",
		&pqcaps->u32FULL_PQ_P_Performance);
	if (ret) {
		PQ_MSG_ERROR("Failed to get FULL_PQ_P_Performance resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LITE_PQ_I_Performance",
		&pqcaps->u32LITE_PQ_I_Performance);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LITE_PQ_I_Performance resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LITE_PQ_P_Performance",
		&pqcaps->u32LITE_PQ_P_Performance);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LITE_PQ_P_Performance resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "ZFD_PQ_I_Performance",
		&pqcaps->u32ZFD_PQ_I_Performance);
	if (ret) {
		PQ_MSG_ERROR("Failed to get ZFD_PQ_I_Performance resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "ZFD_PQ_P_Performance",
		&pqcaps->u32ZFD_PQ_P_Performance);
	if (ret) {
		PQ_MSG_ERROR("Failed to get ZFD_PQ_P_Performance resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "MEMC",
		&pqcaps->u32Memc);
	if (ret) {
		PQ_MSG_ERROR("Failed to get MEMC resource\r\n");
		goto Fail;
	}
	PQ_MSG_INFO("MEMC = %x\r\n", pqcaps->u32Memc);

	of_node_put(bank_node);

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;
}

static int _mtk_pq_parse_dts_dscl(struct mtk_pq_platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	struct mtk_pq_caps *pqcaps = &pdev->pqcaps;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get capability node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "DSCL_MM",
		&pqcaps->u32DSCL_MM);
	if (ret) {
		PQ_MSG_ERROR("Failed to get DSCL_MM resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "DSCL_IPDMA",
		&pqcaps->u32DSCL_IPDMA);
	if (ret) {
		PQ_MSG_ERROR("Failed to get DSCL_IPDMA resource\r\n");
		goto Fail;
	}

	of_node_put(bank_node);

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;
}

static int _mtk_pq_parse_dts_linedelay(struct mtk_pq_platform_device *pdev)
{
	int ret = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	struct mtk_pq_caps *pqcaps = &pdev->pqcaps;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get capability node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "LineDelay_MTK",
		&pqcaps->u32LineDelay_MTK);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LineDelay_MTK resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LineDelay_SRS_8K",
		&pqcaps->u32LineDelay_SRS_8K);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LineDelay_SRS_8K resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LineDelay_SRS_4K",
		&pqcaps->u32LineDelay_SRS_4K);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LineDelay_SRS_4K resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LineDelay_SRS_FHD",
		&pqcaps->u32LineDelay_SRS_FHD);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LineDelay_SRS_FHD resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "LineDelay_SRS_Other",
		&pqcaps->u32LineDelay_SRS_Other);
	if (ret) {
		PQ_MSG_ERROR("Failed to get LineDelay_SRS_Other resource\r\n");
		goto Fail;
	}

	of_node_put(bank_node);

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;
}

static int _mtk_pq_parse_dts_phase(struct mtk_pq_platform_device *pdev)
{

	int ret = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	struct mtk_pq_caps *pqcaps = &pdev->pqcaps;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "capability");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get capability node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "Phase_I_Input",
		&pqcaps->u32Phase_I_Input);
	if (ret) {
		PQ_MSG_ERROR("Failed to get Phase_I_Input resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "Phase_I_Output",
		&pqcaps->u32Phase_I_Output);
	if (ret) {
		PQ_MSG_ERROR("Failed to get Phase_I_Output resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "Phase_P_Input",
		&pqcaps->u32Phase_P_Input);
	if (ret) {
		PQ_MSG_ERROR("Failed to get Phase_P_Input resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "Phase_P_Output",
		&pqcaps->u32Phase_P_Output);
	if (ret) {
		PQ_MSG_ERROR("Failed to get Phase_P_Output resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "NREnginePhase",
		&pqcaps->u32NREnginePhase);
	if (ret) {
		PQ_MSG_ERROR("Failed to get NREnginePhase resource\r\n");
		goto Fail;
	}

	ret = of_property_read_u32(bank_node, "MDWEnginePhase",
		&pqcaps->u32MDWEnginePhase);
	if (ret) {
		PQ_MSG_ERROR("Failed to get MDWEnginePhase resource\r\n");
		goto Fail;
	}

	of_node_put(bank_node);

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;
}

static int _mtk_pq_parse_dts_cus_id(struct mtk_pq_platform_device *pdev)
{

	int ret = 0, idx = 0, len = 0;
	__u32 u32Tmp = 0;
	struct device *property_dev = pdev->dev;
	struct device_node *bank_node;
	char tmp_dev[PQ_VIDEO_DEVICE_NAME_LENGTH] = {'\0'};

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	bank_node = of_find_node_by_name(property_dev->of_node, "cus-dev-id");

	if (bank_node == NULL) {
		PQ_MSG_ERROR("Failed to get cus-dev-id node\r\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(bank_node, "enable", &u32Tmp);
	if (ret) {
		PQ_MSG_ERROR("Failed to get cus_dev resource\r\n");
		goto Fail;
	}
	pdev->cus_dev = u32Tmp;

	for (idx = 0; idx < PQ_WIN_MAX_NUM; idx++) {
		len = snprintf(tmp_dev, sizeof(tmp_dev), "%s", PQ_VIDEO_DEVICE_NAME);
		snprintf(tmp_dev + len, sizeof(tmp_dev) - len, "%s", devID[idx]);

		ret = of_property_read_u32(bank_node, tmp_dev, &u32Tmp);
		if (ret) {
			PQ_MSG_ERROR("Failed to get %s resource\r\n", tmp_dev);
			goto Fail;
		}
		pdev->cus_id[idx] = u32Tmp;
		PQ_MSG_INFO("pdev->cus_id[%d] = %d\r\n", idx, pdev->cus_id[idx]);
	}

	of_node_put(bank_node);

	return 0;

Fail:
	if (bank_node != NULL)
		of_node_put(bank_node);

	return ret;
}

static int _mtk_pq_parse_dts(struct mtk_pq_platform_device *pqdev)
{
	int ret = 0;

	ret = _mtk_pq_parse_dts_clock_target(pqdev);
	if (ret == -EINVAL)
		return ret;

	ret = _mtk_pq_parse_dts_capbability_misc(pqdev);
	if (ret == -EINVAL)
		return ret;

	ret = _mtk_pq_parse_dts_reg(pqdev);
	if (ret == -EINVAL)
		return ret;

	ret = _mtk_pq_parse_dts_performance(pqdev);
	if (ret == -EINVAL)
		return ret;

	ret = _mtk_pq_parse_dts_dscl(pqdev);
	if (ret == -EINVAL)
		return ret;

	ret = _mtk_pq_parse_dts_linedelay(pqdev);
	if (ret == -EINVAL)
		return ret;

	ret = _mtk_pq_parse_dts_phase(pqdev);
	if (ret == -EINVAL)
		return ret;

	ret = _mtk_pq_parse_dts_cus_id(pqdev);
	if (ret == -EINVAL)
		return ret;

	return 0;
}

static int _mtk_pq_dv_init(struct mtk_pq_platform_device *pqdev)
{
	int ret = 0;
	struct mtk_pq_dv_ctrl_init_in dv_ctrl_in;
	struct mtk_pq_dv_ctrl_init_out dv_ctrl_out;
	struct mtk_pq_dv_win_init_in dv_win_in;
	struct mtk_pq_dv_win_init_out dv_win_out;
	struct of_mmap_info_data of_mmap_info = {0};
	__u8 win_num = 0;
	__u8 cnt = 0;
	struct mtk_pq_device *pq_dev = NULL;

	/* initialize Dolby control */
	memset(&dv_ctrl_in, 0, sizeof(struct mtk_pq_dv_ctrl_init_in));
	memset(&dv_ctrl_out, 0, sizeof(struct mtk_pq_dv_ctrl_init_out));
	memset(&of_mmap_info, 0, sizeof(struct of_mmap_info_data));

	dv_ctrl_in.pqdev = pqdev;
	dv_ctrl_in.mmap_size = 0;
	dv_ctrl_in.mmap_addr = 0;

	/* get dolby dma address from MMAP */
	ret = of_mtk_get_reserved_memory_info_by_idx(
		pqdev->dev->of_node, MMAP_DV_PYR_INDEX, &of_mmap_info);
	if (ret < 0)
		PQ_MSG_ERROR("mmap return %d\n", ret);
	else {
		dv_ctrl_in.mmap_size = of_mmap_info.buffer_size;
		dv_ctrl_in.mmap_addr = of_mmap_info.start_bus_address;
	}

	ret = mtk_pq_dv_ctrl_init(&dv_ctrl_in, &dv_ctrl_out);
	if (ret < 0) {
		PQ_MSG_ERROR("mtk_pq_dv_ctrl_init return %d\n", ret);
		goto exit;
	}

	/* initialize Dolby window info */
	win_num = pqdev->usable_win;
	if (win_num > PQ_WIN_MAX_NUM)
		win_num = PQ_WIN_MAX_NUM;

	for (cnt = 0; cnt < win_num; cnt++) {
		pq_dev = pqdev->pq_devs[cnt];
		if (!pq_dev)
			goto exit;

		memset(&dv_win_in, 0, sizeof(struct mtk_pq_dv_win_init_in));
		memset(&dv_win_out, 0, sizeof(struct mtk_pq_dv_win_init_out));

		dv_win_in.dev = pq_dev;

		ret = mtk_pq_dv_win_init(&dv_win_in, &dv_win_out);
		if (ret < 0) {
			PQ_MSG_ERROR("mtk_pq_dv_win_init return %d\n", ret);
			goto exit;
		}
	}

exit:
	return ret;
}

static ssize_t help_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	return snprintf(buf, PAGE_SIZE, "Debug Help:\n"
				"- u32DbgLevel <RW>: To control debug log level.\n"
				"		STI_PQ_LOG_OFF			0x00\n"
				"		STI_PQ_LOG_LEVEL_ERROR		0x01\n"
				"		STI_PQ_LOG_LEVEL_COMMON		0x02\n"
				"		STI_PQ_LOG_LEVEL_ENHANCE	0x04\n"
				"		STI_PQ_LOG_LEVEL_B2R		0x08\n"
				"		STI_PQ_LOG_LEVEL_DISPLAY	0x10\n"
				"		STI_PQ_LOG_LEVEL_XC		0x20\n"
				"		STI_PQ_LOG_LEVEL_3D		0x40\n"
				"		STI_PQ_LOG_LEVEL_BUFFER		0x80\n"
				"		STI_PQ_LOG_LEVEL_SVP		0x100\n"
				"		STI_PQ_LOG_LEVEL_IRQ		0x200\n"
				"		STI_PQ_LOG_LEVEL_PATTERN	0x400\n"
				"		STI_PQ_LOG_LEVEL_DBG		0x800\n"
				"		STI_PQ_LOG_LEVEL_ALL		0xFFF\n");
}

static ssize_t log_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "u32DbgLevel = 0x%x\n", u32DbgLevel);
}

static ssize_t log_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	u32DbgLevel = val;
	return count;
}

static ssize_t atrace_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "atrace_enable_pq = 0x%x\n", atrace_enable_pq);
}

static ssize_t atrace_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int err;

	err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	atrace_enable_pq = val;
	return count;
}


static ssize_t mtk_pq_capability_tch_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("TCH_color", pqdev->pqcaps.u32TCH_color);

	return ssize;
}

static ssize_t mtk_pq_clock_target_xc_ed_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("XC_ED_TARGET_CLK", pqdev->pqcaps.u32XC_ED_TARGET_CLK);

	return ssize;
}

static ssize_t mtk_pq_clock_target_xc_ed_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_clock_target_xc_fn_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("XC_FN_TARGET_CLK", pqdev->pqcaps.u32XC_FN_TARGET_CLK);

	return ssize;
}

static ssize_t mtk_pq_clock_target_xc_fn_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_clock_target_xc_fs_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("XC_FS_TARGET_CLK", pqdev->pqcaps.u32XC_FS_TARGET_CLK);

	return ssize;
}

static ssize_t mtk_pq_clock_target_xc_fs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_clock_target_xc_fd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("XC_FD_TARGET_CLK", pqdev->pqcaps.u32XC_FD_TARGET_CLK);

	return ssize;
}

static ssize_t mtk_pq_clock_target_xc_fd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_tch_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_presharp_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("PreSharp", pqdev->pqcaps.u32PreSharp);

	return ssize;
}

static ssize_t mtk_pq_capability_presharp_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_peaking_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("2D_Peaking", pqdev->pqcaps.u322D_Peaking);

	return ssize;
}

static ssize_t mtk_pq_capability_peaking_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_lce_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LCE", pqdev->pqcaps.u32LCE);

	return ssize;
}

static ssize_t mtk_pq_capability_lce_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_3dlut_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("3D_LUT", pqdev->pqcaps.u323D_LUT);

	return ssize;
}

static ssize_t mtk_pq_capability_3dlut_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_dmascl_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("DMA_SCL", pqdev->pqcaps.u32DMA_SCL);

	return ssize;
}

static ssize_t mtk_pq_capability_dmascl_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_pqu_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("PQU", pqdev->pqcaps.u32PQU);

	return ssize;
}

static ssize_t mtk_pq_capability_pqu_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_winnum_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("Window_Num", pqdev->pqcaps.u32Window_Num);

	return ssize;
}

static ssize_t mtk_pq_capability_winnum_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_srs_support_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("SRS_SUPPORT", pqdev->pqcaps.u32SRS_Support);

	return ssize;
}

static ssize_t mtk_pq_capability_srs_support_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_aisr_support_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("AISR_SUPPORT", pqdev->pqcaps.u32AISR_Support);

	return ssize;
}

static ssize_t mtk_pq_capability_hsy_support_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("HSY_SUPPORT", pqdev->pqcaps.u32HSY_Support);

	return ssize;
}

static ssize_t mtk_pq_capability_hsy_version_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int count = 0;
	__u8 u8MaxSize = MAX_SYSFS_SIZE;

	count += snprintf(buf + count, u8MaxSize - count,
		"HSY_Version=%d\n", pqdev->pqcaps.u32HSY_Version);

	return count;
}

static ssize_t mtk_pq_capability_qmap_version_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int count = 0;
	__u8 u8MaxSize = MAX_SYSFS_SIZE;

	count += snprintf(buf + count, u8MaxSize - count,
		"Qmap_Version=%d\n", pqdev->pqcaps.u32Qmap_Version);

	return count;
}
static ssize_t pattern_ip2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_IP2;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_ip2_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_IP2;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_nr_dnr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_DNR;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_nr_dnr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_DNR;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_nr_ipmr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_IPMR;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_nr_ipmr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_IPMR;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}
static ssize_t pattern_nr_opm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_OPM;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_nr_opm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_OPM;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}
static ssize_t pattern_nr_di_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_DI;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_nr_di_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_NR_DI;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}
static ssize_t pattern_di_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_DI;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_di_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_DI;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}
static ssize_t pattern_srs_in_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SRS_IN;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_srs_in_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SRS_IN;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_srs_out_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SRS_OUT;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_srs_out_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SRS_OUT;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_vop_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_VOP;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_vop_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_VOP;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_ip2_post_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_IP2_POST;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_ip2_post_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_IP2_POST;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_scip_dv_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SCIP_DV;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_scip_dv_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SCIP_DV;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_scdv_dv_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SCDV_DV;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_scdv_dv_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_SCDV_DV;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_b2r_dma_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_DMA;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_b2r_dma_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_DMA;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_b2r_lite1_dma_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_LITE1_DMA;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_b2r_lite1_dma_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_LITE1_DMA;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_b2r_pre_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_PRE;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_b2r_pre_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_PRE;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t pattern_b2r_post_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_POST;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = mtk_pq_pattern_show(buf, dev, position);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t pattern_b2r_post_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	enum pq_pattern_position position = PQ_PATTERN_POSITION_B2R_POST;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_pattern_store(buf, dev, position);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_store failed!\n");
		return ret;
	}

	return count;
}

static ssize_t dv_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_dv_show(dev, buf);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_dv_show return %d\n", ret);
		return ret;
	}

	return ret;
}

static ssize_t dv_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_dv_store(dev, buf);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_dv_store return %d\n", ret);
		return ret;
	}

	return count;
}

static ssize_t mtk_pq_capability_h_max_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("H_MAX_SIZE", pqdev->display_dev.h_max_size);

	return ssize;
}

static ssize_t mtk_pq_capability_v_max_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("V_MAX_SIZE", pqdev->display_dev.v_max_size);

	return ssize;
}

static ssize_t mtk_pq_capability_fullPQ_iReg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("FULL_PQ_I_Register", pqdev->pqcaps.u32FULL_PQ_I_Register);

	return ssize;
}

static ssize_t mtk_pq_capability_fullPQ_iReg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_fullPQ_pReg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("FULL_PQ_P_Register", pqdev->pqcaps.u32FULL_PQ_P_Register);

	return ssize;
}

static ssize_t mtk_pq_capability_fullPQ_pReg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_litePQ_iReg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LITE_PQ_I_Register", pqdev->pqcaps.u32LITE_PQ_I_Register);

	return ssize;
}

static ssize_t mtk_pq_capability_litePQ_iReg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_litePQ_pReg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LITE_PQ_P_Register", pqdev->pqcaps.u32LITE_PQ_P_Register);

	return ssize;
}

static ssize_t mtk_pq_capability_litePQ_pReg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_zfdPQ_iReg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("ZFD_PQ_I_Register", pqdev->pqcaps.u32ZFD_PQ_I_Register);

	return ssize;
}

static ssize_t mtk_pq_capability_zfdPQ_iReg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_zfdPQ_pReg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("ZFD_PQ_P_Register", pqdev->pqcaps.u32ZFD_PQ_P_Register);

	return ssize;
}

static ssize_t mtk_pq_capability_zfdPQ_pReg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_fullPQ_iPerform_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("FULL_PQ_I_Performance", pqdev->pqcaps.u32FULL_PQ_I_Performance);

	return ssize;
}

static ssize_t mtk_pq_capability_fullPQ_iPerform_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_fullPQ_pPerform_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("FULL_PQ_P_Performance", pqdev->pqcaps.u32FULL_PQ_P_Performance);

	return ssize;
}

static ssize_t mtk_pq_capability_fullPQ_pPerform_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_litePQ_iPerform_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LITE_PQ_I_Performance", pqdev->pqcaps.u32LITE_PQ_I_Performance);

	return ssize;
}

static ssize_t mtk_pq_capability_litePQ_iPerform_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_litePQ_pPerform_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LITE_PQ_P_Performance", pqdev->pqcaps.u32LITE_PQ_P_Performance);

	return ssize;
}

static ssize_t mtk_pq_capability_litePQ_pPerform_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}


static ssize_t mtk_pq_capability_zfdPQ_iPerform_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("ZFD_PQ_I_Performance", pqdev->pqcaps.u32ZFD_PQ_I_Performance);

	return ssize;
}

static ssize_t mtk_pq_capability_zfdPQ_iPerform_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_zfdPQ_pPerform_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("ZFD_PQ_P_Performance", pqdev->pqcaps.u32ZFD_PQ_P_Performance);

	return ssize;
}

static ssize_t mtk_pq_capability_zfdPQ_pPerform_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_dscl_mm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("DSCL_MM", pqdev->pqcaps.u32DSCL_MM);

	return ssize;
}

static ssize_t mtk_pq_capability_dscl_mm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_dscl_ipdma_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("DSCL_IPDMA", pqdev->pqcaps.u32DSCL_IPDMA);

	return ssize;
}

static ssize_t mtk_pq_capability_dscl_ipdma_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_linedelay_mtk_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LineDelay_MTK", pqdev->pqcaps.u32LineDelay_MTK);

	return ssize;
}

static ssize_t mtk_pq_capability_linedelay_mtk_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_linedelay_srs_8k_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LineDelay_SRS_8K", pqdev->pqcaps.u32LineDelay_SRS_8K);

	return ssize;
}

static ssize_t mtk_pq_capability_linedelay_srs_8k_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_linedelay_srs_4k_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LineDelay_SRS_4K", pqdev->pqcaps.u32LineDelay_SRS_4K);

	return ssize;
}

static ssize_t mtk_pq_capability_linedelay_srs_4k_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_linedelay_srs_fhd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LineDelay_SRS_FHD", pqdev->pqcaps.u32LineDelay_SRS_FHD);

	return ssize;
}

static ssize_t mtk_pq_capability_linedelay_srs_fhd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_linedelay_srs_other_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("LineDelay_SRS_Other", pqdev->pqcaps.u32LineDelay_SRS_Other);

	return ssize;
}

static ssize_t mtk_pq_capability_linedelay_srs_other_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_phase_i_input_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("Phase_I_Input", pqdev->pqcaps.u32Phase_I_Input);

	return ssize;
}

static ssize_t mtk_pq_capability_phase_i_input_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_phase_i_output_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("Phase_I_Output", pqdev->pqcaps.u32Phase_I_Output);

	return ssize;
}

static ssize_t mtk_pq_capability_phase_i_output_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_phase_p_input_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("Phase_P_Input", pqdev->pqcaps.u32Phase_P_Input);

	return ssize;
}

static ssize_t mtk_pq_capability_phase_p_input_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_capability_phase_p_output_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MAX_SYSFS_SIZE;

	PQ_CAPABILITY_SHOW("Phase_P_Output", pqdev->pqcaps.u32Phase_P_Output);

	return ssize;
}

static ssize_t mtk_pq_capability_phase_p_output_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

#define MEMC_SHOW_SIZE 255
static ssize_t mtk_pq_capability_memc_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int ssize = 0;
	__u8 u8Size = MEMC_SHOW_SIZE;

	PQ_CAPABILITY_SHOW("MEMC", pqdev->pqcaps.u32Memc);

	return ssize;
}

static ssize_t mtk_pq_capability_memc_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t mtk_pq_dev_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mtk_pq_platform_device *pqdev = dev_get_drvdata(dev);
	int i, ssize, splitLen;
	char tmp[MAX_DEV_NAME_LEN] = { 0 };
	char devName[] = "/dev/video";
	char split[] = "\n";

	if ((buf == NULL) || (pqdev->usable_win > MAX_DEV_NAME_LEN))
		return 0;

	splitLen = strlen(split) + 1;
	ssize = pqdev->usable_win / 10 + 2;
	snprintf(tmp, ssize, "%d\n", pqdev->usable_win);
	strncat(buf, tmp, strlen(tmp));
	strncat(buf, split, splitLen);
	ssize = strlen(devName) + 1;
	for (i = 0; i < pqdev->usable_win; ++i) {
		snprintf(tmp, ssize + ((i+1) / 10) + 1, "%s%d\n",
			devName, pqdev->pqcaps.u32Device_register_Num[i]);
		strncat(buf, tmp, strlen(tmp));
		strncat(buf, split, splitLen);
	}
	ssize = strlen(buf) + 1;
	pr_err("[***%s%d***] buf:%s, ssize:%d", __func__, __LINE__, buf, ssize);
	return ssize;
}

static ssize_t mtk_pq_idkdump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = _mtk_pq_idkdump_show(dev, buf);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_pattern_show failed!\n");
		return count;
	}

	return count;
}

static ssize_t mtk_pq_idkdump_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_pq_idkdump_store(dev, buf);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "_mtk_pq_idkdump_store failed!!\n");
		return ret;
	}

	return count;
}

static ssize_t mtk_pq_aisr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	count = mtk_pq_aisr_dbg_show(dev, buf);
	if (count < 0) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_aisr_dbg_show failed!!\n");
		return count;
	}

	return count;
}

static ssize_t mtk_pq_aisr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = mtk_pq_aisr_dbg_store(dev, buf);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_aisr_dbg_store failed!!\n");
		return ret;
	}

	return count;
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(log_level);
static DEVICE_ATTR_RW(atrace_enable);
static DEVICE_ATTR_RW(mtk_pq_clock_target_xc_ed);
static DEVICE_ATTR_RW(mtk_pq_clock_target_xc_fn);
static DEVICE_ATTR_RW(mtk_pq_clock_target_xc_fs);
static DEVICE_ATTR_RW(mtk_pq_clock_target_xc_fd);
static DEVICE_ATTR_RW(mtk_pq_capability_tch);
static DEVICE_ATTR_RW(mtk_pq_capability_presharp);
static DEVICE_ATTR_RW(mtk_pq_capability_peaking);
static DEVICE_ATTR_RW(mtk_pq_capability_lce);
static DEVICE_ATTR_RW(mtk_pq_capability_3dlut);
static DEVICE_ATTR_RW(mtk_pq_capability_dmascl);
static DEVICE_ATTR_RW(mtk_pq_capability_pqu);
static DEVICE_ATTR_RW(mtk_pq_capability_winnum);
static DEVICE_ATTR_RW(mtk_pq_capability_memc);
static DEVICE_ATTR_RW(mtk_pq_capability_srs_support);
static DEVICE_ATTR_RW(pattern_ip2);
static DEVICE_ATTR_RW(pattern_nr_dnr);
static DEVICE_ATTR_RW(pattern_nr_ipmr);
static DEVICE_ATTR_RW(pattern_nr_opm);
static DEVICE_ATTR_RW(pattern_nr_di);
static DEVICE_ATTR_RW(pattern_di);
static DEVICE_ATTR_RW(pattern_srs_in);
static DEVICE_ATTR_RW(pattern_srs_out);
static DEVICE_ATTR_RW(pattern_vop);
static DEVICE_ATTR_RW(pattern_ip2_post);
static DEVICE_ATTR_RW(pattern_scip_dv);
static DEVICE_ATTR_RW(pattern_scdv_dv);
static DEVICE_ATTR_RW(pattern_b2r_dma);
static DEVICE_ATTR_RW(pattern_b2r_lite1_dma);
static DEVICE_ATTR_RW(pattern_b2r_pre);
static DEVICE_ATTR_RW(pattern_b2r_post);
static DEVICE_ATTR_RW(dv);
static DEVICE_ATTR_RO(mtk_pq_capability_h_max);
static DEVICE_ATTR_RO(mtk_pq_capability_v_max);
static DEVICE_ATTR_RW(mtk_pq_capability_fullPQ_iReg);
static DEVICE_ATTR_RW(mtk_pq_capability_fullPQ_pReg);
static DEVICE_ATTR_RW(mtk_pq_capability_litePQ_iReg);
static DEVICE_ATTR_RW(mtk_pq_capability_litePQ_pReg);
static DEVICE_ATTR_RW(mtk_pq_capability_zfdPQ_iReg);
static DEVICE_ATTR_RW(mtk_pq_capability_zfdPQ_pReg);
static DEVICE_ATTR_RW(mtk_pq_capability_fullPQ_iPerform);
static DEVICE_ATTR_RW(mtk_pq_capability_fullPQ_pPerform);
static DEVICE_ATTR_RW(mtk_pq_capability_litePQ_iPerform);
static DEVICE_ATTR_RW(mtk_pq_capability_litePQ_pPerform);
static DEVICE_ATTR_RW(mtk_pq_capability_zfdPQ_iPerform);
static DEVICE_ATTR_RW(mtk_pq_capability_zfdPQ_pPerform);
static DEVICE_ATTR_RW(mtk_pq_capability_dscl_mm);
static DEVICE_ATTR_RW(mtk_pq_capability_dscl_ipdma);
static DEVICE_ATTR_RW(mtk_pq_capability_linedelay_mtk);
static DEVICE_ATTR_RW(mtk_pq_capability_linedelay_srs_8k);
static DEVICE_ATTR_RW(mtk_pq_capability_linedelay_srs_4k);
static DEVICE_ATTR_RW(mtk_pq_capability_linedelay_srs_fhd);
static DEVICE_ATTR_RW(mtk_pq_capability_linedelay_srs_other);
static DEVICE_ATTR_RW(mtk_pq_capability_phase_i_input);
static DEVICE_ATTR_RW(mtk_pq_capability_phase_i_output);
static DEVICE_ATTR_RW(mtk_pq_capability_phase_p_input);
static DEVICE_ATTR_RW(mtk_pq_capability_phase_p_output);
static DEVICE_ATTR_RO(mtk_pq_dev);
static DEVICE_ATTR_RW(mtk_pq_idkdump);
static DEVICE_ATTR_RO(mtk_pq_capability_hsy_support);
static DEVICE_ATTR_RW(mtk_pq_aisr);
static DEVICE_ATTR_RO(mtk_pq_capability_aisr_support);
static DEVICE_ATTR_RO(mtk_pq_capability_hsy_version);
static DEVICE_ATTR_RO(mtk_pq_capability_qmap_version);

static struct attribute *mtk_pq_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	&dev_attr_atrace_enable.attr,
	&dev_attr_mtk_pq_clock_target_xc_ed.attr,
	&dev_attr_mtk_pq_clock_target_xc_fn.attr,
	&dev_attr_mtk_pq_clock_target_xc_fs.attr,
	&dev_attr_mtk_pq_clock_target_xc_fd.attr,
	&dev_attr_mtk_pq_capability_tch.attr,
	&dev_attr_mtk_pq_capability_presharp.attr,
	&dev_attr_mtk_pq_capability_peaking.attr,
	&dev_attr_mtk_pq_capability_lce.attr,
	&dev_attr_mtk_pq_capability_3dlut.attr,
	&dev_attr_mtk_pq_capability_dmascl.attr,
	&dev_attr_mtk_pq_capability_pqu.attr,
	&dev_attr_mtk_pq_capability_winnum.attr,
	&dev_attr_mtk_pq_capability_memc.attr,
	&dev_attr_mtk_pq_capability_srs_support.attr,
	&dev_attr_pattern_ip2.attr,
	&dev_attr_pattern_nr_dnr.attr,
	&dev_attr_pattern_nr_ipmr.attr,
	&dev_attr_pattern_nr_opm.attr,
	&dev_attr_pattern_nr_di.attr,
	&dev_attr_pattern_di.attr,
	&dev_attr_pattern_srs_in.attr,
	&dev_attr_pattern_srs_out.attr,
	&dev_attr_pattern_vop.attr,
	&dev_attr_pattern_ip2_post.attr,
	&dev_attr_pattern_scip_dv.attr,
	&dev_attr_pattern_scdv_dv.attr,
	&dev_attr_pattern_b2r_dma.attr,
	&dev_attr_pattern_b2r_lite1_dma.attr,
	&dev_attr_pattern_b2r_pre.attr,
	&dev_attr_pattern_b2r_post.attr,
	&dev_attr_dv.attr,
	&dev_attr_mtk_pq_capability_h_max.attr,
	&dev_attr_mtk_pq_capability_v_max.attr,
	&dev_attr_mtk_pq_capability_fullPQ_iReg.attr,
	&dev_attr_mtk_pq_capability_fullPQ_pReg.attr,
	&dev_attr_mtk_pq_capability_litePQ_iReg.attr,
	&dev_attr_mtk_pq_capability_litePQ_pReg.attr,
	&dev_attr_mtk_pq_capability_zfdPQ_iReg.attr,
	&dev_attr_mtk_pq_capability_zfdPQ_pReg.attr,
	&dev_attr_mtk_pq_capability_fullPQ_iPerform.attr,
	&dev_attr_mtk_pq_capability_fullPQ_pPerform.attr,
	&dev_attr_mtk_pq_capability_litePQ_iPerform.attr,
	&dev_attr_mtk_pq_capability_litePQ_pPerform.attr,
	&dev_attr_mtk_pq_capability_zfdPQ_iPerform.attr,
	&dev_attr_mtk_pq_capability_zfdPQ_pPerform.attr,
	&dev_attr_mtk_pq_capability_dscl_mm.attr,
	&dev_attr_mtk_pq_capability_dscl_ipdma.attr,
	&dev_attr_mtk_pq_capability_linedelay_mtk.attr,
	&dev_attr_mtk_pq_capability_linedelay_srs_8k.attr,
	&dev_attr_mtk_pq_capability_linedelay_srs_4k.attr,
	&dev_attr_mtk_pq_capability_linedelay_srs_fhd.attr,
	&dev_attr_mtk_pq_capability_linedelay_srs_other.attr,
	&dev_attr_mtk_pq_capability_phase_i_input.attr,
	&dev_attr_mtk_pq_capability_phase_i_output.attr,
	&dev_attr_mtk_pq_capability_phase_p_input.attr,
	&dev_attr_mtk_pq_capability_phase_p_output.attr,
	&dev_attr_mtk_pq_dev.attr,
	&dev_attr_mtk_pq_idkdump.attr,
	&dev_attr_mtk_pq_capability_hsy_support.attr,
	&dev_attr_mtk_pq_aisr.attr,
	&dev_attr_mtk_pq_capability_aisr_support.attr,
	&dev_attr_mtk_pq_capability_hsy_version.attr,
	&dev_attr_mtk_pq_capability_qmap_version.attr,
	NULL,
};

static const struct attribute_group mtk_pq_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_pq_attrs
};

static void mtk_pq_CreateSysFS(struct device *pdv)
{
	int ret = 0;
	PQ_MSG_INFO("Device_create_file initialized\n");

	ret = sysfs_create_group(&pdv->kobj, &mtk_pq_attr_group);
	if (ret)
		dev_err(pdv,
		"[%d]Fail to create sysfs files: %d\n",
		__LINE__, ret);
}

static void mtk_pq_RemoveSysFS(struct device *pdv)
{
	dev_info(pdv, "Remove device attribute files");
	sysfs_remove_group(&pdv->kobj, &mtk_pq_attr_group);
}

static int mtk_pq_querycap(struct file *file,
	void *priv, struct v4l2_capability *cap)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	strncpy(cap->driver, PQ_VIDEO_DRIVER_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, pq_dev->video_dev.name, sizeof(cap->card) - 1);
	cap->bus_info[0] = 0;
	cap->capabilities = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING |
		V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_CAPTURE|
		V4L2_CAP_DEVICE_CAPS;
	cap->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING |
		V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_CAPTURE;
	return 0;
}

static int mtk_pq_s_input(struct file *file, void *fh, unsigned int i)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_set_input(pq_dev, i);
}

static int mtk_pq_g_input(struct file *file, void *fh, unsigned int *i)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_get_input(pq_dev, i);
}

static int mtk_pq_s_fmt_vid_cap_mplane(struct file *file,
	void *fh, struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_set_fmt_cap_mplane(pq_dev, f);
}

static int mtk_pq_s_fmt_vid_out_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_set_fmt_out_mplane(pq_dev, f);
}

static int mtk_pq_g_fmt_vid_cap_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_get_fmt_cap_mplane(pq_dev, f);
}

static int mtk_pq_g_fmt_vid_out_mplane(
	struct file *file,
	void *fh,
	struct v4l2_format *f)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	return mtk_pq_common_get_fmt_out_mplane(pq_dev, f);
}

static int mtk_pq_streamon(struct file *file,
	void *fh, enum v4l2_buf_type type)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	struct msg_stream_on_info msg_info;
	struct mtk_pq_dv_streamon_in dv_in;
	struct mtk_pq_dv_streamon_out dv_out;
	u32 source = pq_dev->common_info.input_source;
	int ret = 0;

	memset(&msg_info, 0, sizeof(struct msg_stream_on_info));

	if (!pq_dev->ref_count) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Window is not open!\n");
		ret = -EINVAL;
		return ret;
	}

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		msg_info.stream_type = PQU_MSG_BUF_TYPE_OUTPUT;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		msg_info.stream_type = PQU_MSG_BUF_TYPE_INPUT;
		if (IS_INPUT_SRCCAP(source)) {
			ret = mtk_pq_display_idr_streamon(&pq_dev->display_info.idr);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"Stream on fail!Buffer Type = %d, ret=%d\n", type, ret);
				return ret;
			}
		} else if (IS_INPUT_B2R(source)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "stream on b2r source!\n");
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"%s: unknown source %d!\n", __func__, source);
			return -EINVAL;
		}

		ret = mtk_pq_buffer_allocate(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_buffer_allocate fail! ret=%d\n", ret);
			return ret;
		}

#if IS_ENABLED(CONFIG_OPTEE)
		/* svp stream on process start */
		ret = mtk_pq_svp_out_streamon(pq_dev);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_svp_out_streamon fail! ret=%d\n", ret);
			return ret;
		}
		/* svp stream on process end */
#endif

		/* dolby stream on process start */
		memset(&dv_in, 0, sizeof(struct mtk_pq_dv_streamon_in));
		memset(&dv_out, 0, sizeof(struct mtk_pq_dv_streamon_out));
		dv_in.dev = pq_dev;
		ret = mtk_pq_dv_streamon(&dv_in, &dv_out);
		if (ret)
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_dv_streamon return %d\n", ret);
		/* dolby stream on process end */
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid Buffer Type: %d\n", type);
		return -EINVAL;
	}

	ret = v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"m2m stream on fail!Buffer Type = %d\n", type);
		return ret;
	}

	ret = mtk_pq_common_stream_on(file, type, pq_dev, &msg_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_stream_on fail!\n");
		return ret;
	}

	if (V4L2_TYPE_IS_OUTPUT(type))
		pq_dev->input_stream.streaming = TRUE;
	else
		pq_dev->output_stream.streaming = TRUE;

	pq_dev->common_info.diff_count = 0;

	ctx->state = MTK_STATE_INIT;

	return ret;
}

static int mtk_pq_streamoff(struct file *file,
		void *fh, enum v4l2_buf_type type)
{
	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	struct msg_stream_off_info msg_info;
	struct v4l2_event ev;
	struct mtk_pq_dv_streamoff_in dv_in;
	struct mtk_pq_dv_streamoff_out dv_out;
	u32 source = pq_dev->common_info.input_source;
	int ret = 0;

	memset(&msg_info, 0, sizeof(struct msg_stream_off_info));
	memset(&ev, 0, sizeof(struct v4l2_event));

	if (!pq_dev->ref_count) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Window is not open!\n");
		ret = -EINVAL;
		return ret;
	}

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE &&
		pq_dev->input_stream.streaming == FALSE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "input_stream is not opened\n");
		return -EINVAL;
	}

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
		pq_dev->output_stream.streaming == FALSE) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "output_stream is not opened\n");
		return -EINVAL;
	}

	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		msg_info.stream_type = PQU_MSG_BUF_TYPE_OUTPUT;
		ret = mtk_pq_display_mdw_streamoff(&pq_dev->display_info.mdw);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Stream off fail!Buffer Type = %d, ret=%d\n", type, ret);
			return ret;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		msg_info.stream_type = PQU_MSG_BUF_TYPE_INPUT;
		if (IS_INPUT_SRCCAP(source)) {
			ret = mtk_pq_display_idr_streamoff(&pq_dev->display_info.idr);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"Stream off fail!Buffer Type = %d, ret=%d\n", type, ret);
				return ret;
			}
		} else if (IS_INPUT_B2R(source)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "stream off b2r source!\n");
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"%s: unknown source %d!\n", __func__, source);
			return -EINVAL;
		}

		/* dolby stream off process start */
		memset(&dv_in, 0, sizeof(struct mtk_pq_dv_streamoff_in));
		memset(&dv_out, 0, sizeof(struct mtk_pq_dv_streamoff_out));
		dv_in.dev = pq_dev;
		ret = mtk_pq_dv_streamoff(&dv_in, &dv_out);
		if (ret)
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_dv_streamoff return %d\n", ret);
		/* dolby stream off process end */
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid Buffer Type: %d\n", type);
		return -EINVAL;
	}

	/* prepare msg to pqu */
	ret = mtk_pq_common_stream_off(pq_dev, type, &msg_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_stream_off fail!\n");
		return ret;
	}

	if (V4L2_TYPE_IS_OUTPUT(type)) {
		pq_dev->input_stream.streaming = FALSE;
		ev.type = V4L2_EVENT_MTK_PQ_INPUT_STREAMOFF;

		v4l2_event_queue(&pq_dev->video_dev, &ev);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "queue input stream off event!\n");
	} else {
		pq_dev->output_stream.streaming = FALSE;
		ev.type = V4L2_EVENT_MTK_PQ_OUTPUT_STREAMOFF;

		v4l2_event_queue(&pq_dev->video_dev, &ev);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "queue output stream off event!\n");
	}

	if ((!pq_dev->input_stream.streaming) && (!pq_dev->output_stream.streaming)) {
		ev.type = V4L2_EVENT_MTK_PQ_STREAMOFF;
		v4l2_event_queue(&pq_dev->video_dev, &ev);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "queue stream off event!\n");
	}

	pq_dev->common_info.diff_count = 0;

	return ret;
}

static int mtk_pq_subscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	struct mtk_pq_device *pq_dev = video_get_drvdata(fh->vdev);

	switch (sub->type) {
	case V4L2_EVENT_HDR_SEAMLESS_MUTE:
	case V4L2_EVENT_HDR_SEAMLESS_UNMUTE:
		if (mtk_pq_enhance_subscribe_event(pq_dev, sub->type)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to subscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_MTK_PQ_INPUT_DONE:
	case V4L2_EVENT_MTK_PQ_OUTPUT_DONE:
	case V4L2_EVENT_MTK_PQ_CALLBACK:
	case V4L2_EVENT_MTK_PQ_STREAMOFF:
	case V4L2_EVENT_MTK_PQ_INPUT_STREAMOFF:
	case V4L2_EVENT_MTK_PQ_OUTPUT_STREAMOFF:
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_subscribe(fh, sub, PQ_NEVENTS, NULL);
}

static int mtk_pq_unsubscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	struct mtk_pq_device *pq_dev = video_get_drvdata(fh->vdev);

	switch (sub->type) {
	case V4L2_EVENT_MTK_PQ_INPUT_DONE:
	case V4L2_EVENT_MTK_PQ_OUTPUT_DONE:
	case V4L2_EVENT_MTK_PQ_CALLBACK:
	case V4L2_EVENT_MTK_PQ_STREAMOFF:
	case V4L2_EVENT_MTK_PQ_INPUT_STREAMOFF:
	case V4L2_EVENT_MTK_PQ_OUTPUT_STREAMOFF:
		break;
	case V4L2_EVENT_HDR_SEAMLESS_MUTE:
	case V4L2_EVENT_HDR_SEAMLESS_UNMUTE:
		if (mtk_pq_enhance_unsubscribe_event(pq_dev, sub->type)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to unsubscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	case V4L2_EVENT_ALL:
		if (mtk_pq_enhance_unsubscribe_event(pq_dev, sub->type)) {
			v4l2_err(&pq_dev->v4l2_dev,
				"failed to unsubscribe %d event\n", sub->type);
			return -EPERM;
		}
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_unsubscribe(fh, sub);
}

static int mtk_pq_reqbufs(struct file *file,
	void *fh, struct v4l2_requestbuffers *reqbufs)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Buffer Type = %d!\n", reqbufs->type);

	ret = v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Request buffer fail!Buffer Type = %d, ret=%d\n",
			reqbufs->type, ret);

	return ret;
}

static int mtk_pq_querybuf(struct file *file,
	void *fh, struct v4l2_buffer *buffer)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Buffer Type = %d!\n", buffer->type);

	ret = v4l2_m2m_querybuf(file, ctx->m2m_ctx, buffer);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Query buffer fail!Buffer Type = %d\n",
			buffer->type);

	return ret;
}

static int mtk_pq_qbuf(struct file *file,
	void *fh, struct v4l2_buffer *buffer)
{
	int ret = 0;
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Buffer Type = %d!\n", buffer->type);

	if (!pq_dev->ref_count) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Window is not open!\n");
		ret = -EINVAL;
		return ret;
	}

#if IS_ENABLED(CONFIG_OPTEE)
	if (V4L2_TYPE_IS_OUTPUT(buffer->type)) {
		ret = mtk_pq_svp_cfg_outbuf_sec(pq_dev, buffer);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_svp_cfg_outbuf_sec fail!\n");
			return ret;
		}
	}
#endif

	ret =  v4l2_m2m_qbuf(file, ctx->m2m_ctx, buffer);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Queue buffer fail! Buffer Type = %d, ret = %d\n",
			buffer->type, ret);
		return ret;
	}

	return ret;
}

static int mtk_pq_dqbuf(struct file *file,
	void *fh, struct v4l2_buffer *buffer)
{
	int ret = 0;
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	struct mtk_pq_device *pq_dev = video_drvdata(file);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Buffer Type = %d!\n", buffer->type);

	if (!pq_dev->ref_count) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Window is not open!\n");
		ret = -EINVAL;
		return ret;
	}

	if (V4L2_TYPE_IS_OUTPUT(buffer->type)) {
		if (pq_dev->input_stream.streaming == FALSE)
			return -ENODATA;
	} else {
		if (pq_dev->output_stream.streaming == FALSE)
			return -ENODATA;
	}

	ret = v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buffer);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Dequeue buffer fail!Buffer Type = %d\n",
			buffer->type);
		return ret;
	}

	return ret;
}

static int mtk_pq_s_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	if (V4L2_TYPE_IS_OUTPUT(s->type)) {
		switch (s->target) {
		case V4L2_SEL_TGT_CROP:
			ret = mtk_pq_display_idr_s_crop(&ctx->pq_dev->display_info.idr, s);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"mtk_pq_display_idr_s_crop fail!\n");
				return ret;
			}
			break;
		default:
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Unsupported target type!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int mtk_pq_g_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	int ret = 0;

	if (V4L2_TYPE_IS_OUTPUT(s->type)) {
		switch (s->target) {
		case V4L2_SEL_TGT_CROP:
			ret = mtk_pq_display_idr_g_crop(&ctx->pq_dev->display_info.idr, s);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"mtk_pq_display_idr_g_crop fail!\n");
				return ret;
			}
			break;
		default:
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Unsupported target type!\n");
			return -EINVAL;
		}
	}

	return 0;
}

/* v4l2_ioctl_ops */
static const struct v4l2_ioctl_ops mtk_pq_dec_ioctl_ops = {
	.vidioc_querycap		= mtk_pq_querycap,
	.vidioc_s_input			= mtk_pq_s_input,
	.vidioc_g_input			= mtk_pq_g_input,
	.vidioc_s_fmt_vid_cap_mplane	= mtk_pq_s_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_out_mplane	= mtk_pq_s_fmt_vid_out_mplane,
	.vidioc_g_fmt_vid_cap_mplane	= mtk_pq_g_fmt_vid_cap_mplane,
	.vidioc_g_fmt_vid_out_mplane	= mtk_pq_g_fmt_vid_out_mplane,
	.vidioc_streamon		= mtk_pq_streamon,
	.vidioc_streamoff		= mtk_pq_streamoff,
	.vidioc_subscribe_event		= mtk_pq_subscribe_event,
	.vidioc_unsubscribe_event	= mtk_pq_unsubscribe_event,
	.vidioc_reqbufs			= mtk_pq_reqbufs,
	.vidioc_querybuf		= mtk_pq_querybuf,
	.vidioc_qbuf			= mtk_pq_qbuf,
	.vidioc_dqbuf			= mtk_pq_dqbuf,
	.vidioc_s_selection		= mtk_pq_s_selection,
	.vidioc_g_selection		= mtk_pq_g_selection,
};

static int _mtk_pq_queue_setup(struct vb2_queue *vq,
	unsigned int *num_buffers, unsigned int *num_planes,
	unsigned int sizes[], struct device *alloc_devs[])
{
	struct mtk_pq_ctx *ctx = vb2_get_drv_priv(vq);
	struct mtk_pq_device *pq_dev = ctx->pq_dev;
	u32 source = pq_dev->common_info.input_source;
	int ret = 0;

	switch (vq->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (IS_INPUT_SRCCAP(source)) {
			ret = mtk_pq_display_idr_queue_setup(
				vq, num_buffers, num_planes, sizes, alloc_devs);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_queue_setup_out fail, ret=%d!\n", ret);
				return ret;
			}
		} else if (IS_INPUT_B2R(source)) {
			// TODO
			ret = mtk_pq_display_b2r_queue_setup(
				vq, num_buffers, num_planes, sizes, alloc_devs);
			*num_planes = 2;
			sizes[0] = 4;
			sizes[1] = 4;
			*num_buffers = 32;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "reset buffer count!\n");
		} else {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"%s: unknown source %d!\n", __func__, source);
			return -EINVAL;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		ret = mtk_pq_display_mdw_queue_setup(
			vq, num_buffers, num_planes, sizes, alloc_devs);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"_mtk_pq_queue_setup_cap fail, ret=%d!\n", ret);
			return ret;
		}
		break;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid buffer type %d!\n", vq->type);
		return -EINVAL;
	}

	return 0;
}

static int _mtk_pq_buf_prepare(struct vb2_buffer *vb)
{
	int ret = 0;
	struct mtk_pq_buffer *pq_buf = NULL;
	struct mtk_pq_ctx *ctx = NULL;
	struct mtk_pq_device *pq_dev = NULL;
	struct scatterlist *sg = NULL;

	if ((!vb) || (!vb->vb2_queue)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Buffer Type = %d!\n", vb->type);

	ctx = vb2_get_drv_priv(vb->vb2_queue);
	if ((!ctx) || (!ctx->pq_dev)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	pq_dev = ctx->pq_dev;

	if (vb->memory != VB2_MEMORY_DMABUF) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid Memory Type!\n");
		return -EINVAL;
	}

	pq_buf = container_of(container_of(vb, struct vb2_v4l2_buffer, vb2_buf),
		struct mtk_pq_buffer, vb);
	if (pq_buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	pq_buf->meta_buf.fd = vb->planes[vb->num_planes - 1].m.fd;
	pq_buf->meta_buf.va = vb2_plane_vaddr(vb, (vb->num_planes - 1));
	pq_buf->meta_buf.db = dma_buf_get(pq_buf->meta_buf.fd);
	if (pq_buf->meta_buf.db == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	pq_buf->meta_buf.size = pq_buf->meta_buf.db->size;
	pq_buf->meta_buf.sgt = vb2_plane_cookie(vb, (vb->num_planes - 1));

	sg = pq_buf->meta_buf.sgt->sgl;
	pq_buf->meta_buf.pa = page_to_phys(sg_page(sg)) + sg->offset - pq_dev->memory_bus_base;

	pq_buf->meta_buf.iova = sg_dma_address(sg);
	if (pq_buf->meta_buf.iova < 0x200000000) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Error iova=0x%llx\n", pq_buf->meta_buf.iova);
		dma_buf_put(pq_buf->meta_buf.db);
	}

	pq_buf->frame_buf.fd = vb->planes[0].m.fd;
	pq_buf->frame_buf.va = vb2_plane_vaddr(vb, 0);
	pq_buf->frame_buf.db = dma_buf_get(pq_buf->frame_buf.fd);
	if (pq_buf->frame_buf.db == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		dma_buf_put(pq_buf->meta_buf.db);
		return -EINVAL;
	}

	pq_buf->frame_buf.size = pq_buf->frame_buf.db->size;
	pq_buf->frame_buf.sgt = vb2_plane_cookie(vb, 0);

	sg = pq_buf->frame_buf.sgt->sgl;
	pq_buf->frame_buf.pa = page_to_phys(sg_page(sg)) + sg->offset - pq_dev->memory_bus_base;

	pq_buf->frame_buf.iova = sg_dma_address(sg);
	if (pq_buf->frame_buf.iova < 0x200000000) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Error iova=0x%llx\n", pq_buf->frame_buf.iova);
		dma_buf_put(pq_buf->meta_buf.db);
		dma_buf_put(pq_buf->frame_buf.db);
		return -EINVAL;
	}

	return 0;
}

static void _mtk_pq_buf_queue(struct vb2_buffer *vb)
{
	int ret = 0;
	struct vb2_v4l2_buffer *vb2_v4l2 = NULL;
	struct mtk_pq_ctx *ctx = NULL;

	if (!vb) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}
	ctx = vb2_get_drv_priv(vb->vb2_queue);
	if (!ctx) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}

	vb2_v4l2 = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, vb2_v4l2);
}

static void _mtk_pq_buf_finish(struct vb2_buffer *vb)
{
	struct mtk_pq_buffer *pq_buf = NULL;

	if (vb == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Buffer Type = %d!\n", vb->type);

	pq_buf = container_of(container_of(vb, struct vb2_v4l2_buffer, vb2_buf),
		struct mtk_pq_buffer, vb);
	if (pq_buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return;
	}

	dma_buf_put(pq_buf->meta_buf.db);
	dma_buf_put(pq_buf->frame_buf.db);

	return;
}

static const struct vb2_ops mtk_pq_vb2_qops = {
	.queue_setup	= _mtk_pq_queue_setup,
	.buf_queue	= _mtk_pq_buf_queue,
	.buf_prepare	= _mtk_pq_buf_prepare,
	.buf_finish	= _mtk_pq_buf_finish,
	.wait_prepare	= vb2_ops_wait_prepare,
	.wait_finish	= vb2_ops_wait_finish,
};

static int queue_init(void *priv, struct vb2_queue *src_vq,
	struct vb2_queue *dst_vq)
{
	struct mtk_pq_ctx *ctx = priv;
	int ret = 0;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &mtk_pq_vb2_qops;
	src_vq->mem_ops = &vb2_dma_sg_memops;
	src_vq->buf_struct_size = sizeof(struct mtk_pq_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->pq_dev->mutex;
	src_vq->dev = ctx->pq_dev->dev;
	src_vq->allow_zero_bytesused = 1;
	src_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(src_vq);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "vb2_queue_init fail!\n");
		return ret;
	}

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &mtk_pq_vb2_qops;
	dst_vq->mem_ops = &vb2_dma_sg_memops;
	dst_vq->buf_struct_size = sizeof(struct mtk_pq_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->pq_dev->mutex;
	dst_vq->dev = ctx->pq_dev->dev;
	dst_vq->allow_zero_bytesused = 1;
	dst_vq->min_buffers_needed = 1;

	ret = vb2_queue_init(dst_vq);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "vb2_queue_init fail!\n");
		return ret;
	}

	return 0;
}

static int mtk_pq_open(struct file *file)
{
	if (file == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}

	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct mtk_pq_ctx *pq_ctx = NULL;
	int ret = 0;
	int b2r_ret = -1, buffer_ret = -1, svp_ret = -1, enhance_ret = -1;
	struct pq_buffer pq_buf;
	struct msg_new_win_info msg_info;

	mutex_lock(&pq_dev->mutex);

	if (pq_dev->ref_count) {
		pq_dev->ref_count++;
		file->private_data = &pq_dev->m2m.ctx->fh;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Increase Window (%d) Reference count: %d!\n",
			pq_dev->dev_indx, pq_dev->ref_count);
		goto unlock;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "pq open\n");

	memset(&msg_info, 0, sizeof(struct msg_new_win_info));

	pq_ctx = kzalloc(sizeof(struct mtk_pq_ctx), GFP_KERNEL);
	if (!pq_ctx) {
		ret = -ENOMEM;
		goto unlock;
	}

	pq_ctx->pq_dev = pq_dev;
	pq_ctx->m2m_ctx = v4l2_m2m_ctx_init(pq_dev->m2m.m2m_dev,
				pq_ctx, queue_init);

	pq_dev->m2m.ctx = pq_ctx;

	v4l2_fh_init(&pq_ctx->fh, video_devdata(file));
	pq_ctx->fh.m2m_ctx = pq_ctx->m2m_ctx;
	file->private_data = &pq_ctx->fh;
	v4l2_fh_add(&pq_ctx->fh);

	b2r_ret = mtk_pq_b2r_init(pq_dev);
	if (b2r_ret < 0) {
		ret = -1;
		pr_err("MTK PQ : b2r init failed\n");
		goto unlock;
	}

	buffer_ret = mtk_pq_buffer_buf_init(pq_dev);
	if (buffer_ret < 0) {
		ret = -1;
		pr_err("MTK PQ : pq buffer init failed\n");
		goto unlock;
	}

#if IS_ENABLED(CONFIG_OPTEE)
	svp_ret = mtk_pq_svp_init(pq_dev);
	if (svp_ret < 0) {
		ret = -1;
		pr_err("MTK PQ : pq svp init failed\n");
		goto unlock;
	}
#endif

	enhance_ret = mtk_pq_enhance_open(pq_dev);
	if (enhance_ret) {
		ret = -1;
		v4l2_err(&pq_dev->v4l2_dev,
			"failed to open enhance subdev!\n");
		goto unlock;
	}

	if (pq_dev->qbuf_meta_pool_addr == 0 || pq_dev->qbuf_meta_pool_size == 0) {
		ret = _mtk_pq_get_memory_info(pqdev->dev, MMAP_METADATA_INDEX,
				&pq_dev->qbuf_meta_pool_addr, &pq_dev->qbuf_meta_pool_size);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Failed to _mtk_pq_get_memory_info(index=%d)\n", MMAP_METADATA_INDEX);
			goto unlock;
		}
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"qbuf_meta_pool_addr = 0x%llx qbuf_meta_pool_size = 0x%x\n",
			pq_dev->qbuf_meta_pool_addr, pq_dev->qbuf_meta_pool_size);
	}

	if (bPquEnable && config_info.config) {
		config_info.config = pqdev->hwmap_config.pa;
		config_info.dvconfig = pqdev->dv_config.pa;
		ret = mtk_pq_common_config(&config_info, true);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_common_config fail!\n");
			goto unlock;
		}
		config_info.config = 0;
	}

	ret = mtk_pq_common_open(pq_dev, &msg_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_open fail!\n");
		goto unlock;
	}

	pq_dev->ref_count = 1;
	pq_dev->input_stream.streaming = FALSE;
	pq_dev->output_stream.streaming = FALSE;

unlock:
	mutex_unlock(&pq_dev->mutex);
	return ret;
}

static int mtk_pq_release(struct file *file)
{
	if (file == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}

	int ret = 0;
	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct v4l2_fh *fh = (struct v4l2_fh *)file->private_data;
	struct mtk_pq_ctx *ctx = fh_to_ctx(fh);
	struct msg_destroy_win_info msg_info;

	memset(&msg_info, 0, sizeof(struct msg_destroy_win_info));

	mutex_lock(&pq_dev->mutex);

	if (pq_dev->ref_count > 1) {
		pq_dev->ref_count--;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Decrease Window (%d) Reference count: %d!\n",
			pq_dev->dev_indx, pq_dev->ref_count);
		goto unlock;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "pq close\n");

	ret = mtk_pq_common_close(pq_dev, &msg_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_common_close fail!\n");
		goto unlock;
	}

	v4l2_m2m_ctx_release(ctx->m2m_ctx);

	ret = mtk_pq_b2r_exit(pq_dev);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_b2r_exit fail!\n");
		goto unlock;
	}

#if IS_ENABLED(CONFIG_OPTEE)
	mtk_pq_svp_exit(pq_dev);
#endif
	ret = mtk_pq_buffer_buf_exit(pq_dev);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_buffer_buf_exit fail!\n");
		goto unlock;
	}

	ret = mtk_pq_enhance_close(pq_dev);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk_pq_enhance_close fail!\n");
		goto unlock;
	}

	v4l2_fh_del(fh);
	v4l2_fh_exit(fh);
	kfree(fh);

	pq_dev->ref_count = 0;
	pq_dev->input_stream.streaming = FALSE;
	pq_dev->output_stream.streaming = FALSE;

unlock:
	mutex_unlock(&pq_dev->mutex);

	return ret;
}

static int mmap_userdev_mmap(struct file *file,
	struct vm_area_struct *vma)
{
	// test_1: mtk_ltp_mmap_map_user_va_rw
	int ret;
	u64 start_bus_pfn;
	struct mtk_pq_device *pq_dev = video_drvdata(file);
	struct mtk_pq_platform_device *pqdev = NULL;

	if (!pq_dev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}

	pqdev = dev_get_drvdata(pq_dev->dev);
	if (!pqdev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}

	// step_1: get mmap info
	if (mtk_hdr_GetDVBinMmpStatus()) {
		if (pqdev->dv_config.pa == 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid DV CFG PA\n");
			return -EINVAL;
		}
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"mmap index: %d\n", pqdev->pq_memory_index);
		if (pqdev->DDBuf[pqdev->pq_memory_index].pa == 0) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid PQMAP PA\n");
			return -EINVAL;
		}
	}

	// step_2: mmap to user_va
	if (mtk_hdr_GetDVBinMmpStatus()) {
		start_bus_pfn = pqdev->dv_config.pa >> PAGE_SHIFT;
	} else {
		start_bus_pfn = pqdev->DDBuf[pqdev->pq_memory_index].pa >> PAGE_SHIFT;
	}

	if (io_remap_pfn_range(vma, vma->vm_start,
		start_bus_pfn, (vma->vm_end - vma->vm_start),
		vma->vm_page_prot)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mmap user space va failed\n");
		ret = -EAGAIN;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_BUFFER,
			"mmap user space va: 0x%lX ~ 0x%lX\n", vma->vm_start, vma->vm_end);
		ret = 0;
	}

	return ret;
}

/* v4l2 ops */
static const struct v4l2_file_operations mtk_pq_fops = {
	.owner              = THIS_MODULE,
	.open               = mtk_pq_open,
	.release            = mtk_pq_release,
	.poll               = v4l2_ctrl_poll,
	.unlocked_ioctl     = video_ioctl2,
	.mmap               = mmap_userdev_mmap,
};
static void mtk_m2m_device_run(void *priv)
{
	int ret = 0;
	u32 source = 0;
	struct mtk_pq_ctx *pq_ctx = NULL;
	struct mtk_pq_device *pq_dev = NULL;
	struct vb2_v4l2_buffer *src_buf = NULL;
	struct vb2_v4l2_buffer *dst_buf = NULL;
	struct mtk_pq_buffer *pq_src_buf = NULL;
	struct mtk_pq_buffer *pq_dst_buf = NULL;
	struct msg_queue_info msg_info;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "enter device run!\n");

	if (!priv) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Priv Pointer is NULL!\n");
		return;
	}

	memset(&msg_info, 0, sizeof(struct msg_queue_info));

	pq_ctx = priv;
	pq_dev = pq_ctx->pq_dev;

	if (pq_ctx->state == MTK_STATE_ABORT) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "job was cancelled!\n");
		goto job_finish;
	}

	source = pq_dev->common_info.input_source;

	//input buffer
	src_buf = v4l2_m2m_src_buf_remove(pq_ctx->m2m_ctx);
	if (src_buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Input Buffer is NULL!\n");
		goto job_finish;
	}

	pq_src_buf = container_of(src_buf, struct mtk_pq_buffer, vb);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"[input] frame fd = %d, meta fd = %d, index = %d!\n",
		pq_src_buf->frame_buf.fd, pq_src_buf->meta_buf.fd,
		pq_src_buf->vb.vb2_buf.index);

	if (IS_INPUT_SRCCAP(source)) {
		ret = mtk_pq_display_idr_qbuf(pq_dev, pq_src_buf);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_display_idr_qbuf fail!\n");
			goto job_finish;
		}
	} else if (IS_INPUT_B2R(source)) {
		ret = mtk_pq_display_b2r_qbuf(pq_dev, pq_src_buf);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_display_b2r_qbuf fail!\n");
			goto job_finish;
		}
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"%s: unknown source %d!\n", __func__, source);
		goto job_finish;
	}

	ret = mtk_display_set_frame_metadata(pq_dev, pq_src_buf);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk_display_set_frame_metadata fail!\n");
	}

	ret = mtk_pq_dv_qbuf(pq_dev, pq_src_buf);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_dv_qbuf fail!");

	msg_info.frame_type = PQU_MSG_BUF_TYPE_INPUT;
	ret = mtk_pq_common_qbuf(pq_dev, pq_src_buf, &msg_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Common Queue Input Buffer Fail!\n");
		goto job_finish;
	}

	//output buffer
	if (pq_dev->common_info.diff_count >= pq_dev->common_info.op2_diff) {
		dst_buf = v4l2_m2m_dst_buf_remove(pq_ctx->m2m_ctx);
		if (src_buf == NULL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Output Buffer is NULL!\n");
			goto job_finish;
		}

		pq_dst_buf = container_of(dst_buf, struct mtk_pq_buffer, vb);

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"[output] frame fd = %d, meta fd = %d, index = %d!\n",
			pq_dst_buf->frame_buf.fd, pq_dst_buf->meta_buf.fd,
			pq_dst_buf->vb.vb2_buf.index);

#if IS_ENABLED(CONFIG_OPTEE)
		ret = mtk_pq_svp_cfg_capbuf_sec(pq_dev, pq_dst_buf);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "mtk_pq_cfg_capture_buf_sec fail!\n");
			goto job_finish;
		}
#endif

		ret = mtk_pq_display_mdw_qbuf(pq_dev, pq_dst_buf);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"mtk_pq_display_mdw_qbuf fail!\n");
			goto job_finish;
		}

		msg_info.frame_type = PQU_MSG_BUF_TYPE_OUTPUT;
		ret = mtk_pq_common_qbuf(pq_dev, pq_dst_buf, &msg_info);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Common Queue Output Buffer Fail!\n");
			goto job_finish;
		}
	} else {
		pq_dev->common_info.diff_count++;
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "diff_count = %d, diff = %d!\n",
			pq_dev->common_info.diff_count, pq_dev->common_info.op2_diff);
	}

job_finish:
	v4l2_m2m_job_finish(pq_dev->m2m.m2m_dev, pq_ctx->m2m_ctx);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "exit device run!\n");

	return;
}

static void mtk_m2m_job_abort(void *priv)
{
	struct mtk_pq_ctx *ctx = NULL;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "enter job abort!\n");

	if (!priv) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Priv Pointer is NULL!\n");
		return;
	}

	ctx = priv;

	ctx->state = MTK_STATE_ABORT;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "exit job abort!\n");

	return;
}

// pq_init_subdevices - Initialize subdev structures and resources
// @pq_dev: pq device
// Return 0 on success or a negative error code on failure

static int pq_init_subdevices(struct mtk_pq_platform_device *pqdev)
{
	int ret = 0;
	int i = 0;

	ret = mtk_pq_b2r_subdev_init(pqdev->dev, &pqdev->b2r_dev);
	if (ret < 0) {
		PQ_MSG_ERROR("[B2R] get resource failed\n");
		ret = -1;
	}

	return ret;
}

static struct v4l2_m2m_ops mtk_pq_m2m_ops = {
	.device_run = mtk_m2m_device_run,
	.job_abort = mtk_m2m_job_abort,
};

static void _mtk_pq_unregister_v4l2dev(struct mtk_pq_device *pq_dev)
{
	mtk_pq_unregister_enhance_subdev(&pq_dev->subdev_enhance);
	mtk_pq_unregister_display_subdev(&pq_dev->subdev_display);
	mtk_pq_unregister_b2r_subdev(&pq_dev->subdev_b2r);
	mtk_pq_unregister_common_subdev(&pq_dev->subdev_common);
	v4l2_m2m_release(pq_dev->m2m.m2m_dev);
	video_unregister_device(&pq_dev->video_dev);
	v4l2_device_unregister(&pq_dev->v4l2_dev);
}

static int _mtk_pq_register_v4l2dev(struct mtk_pq_device *pq_dev,
	__u8 dev_id, struct mtk_pq_platform_device *pqdev)
{
	struct video_device *vdev;
	struct v4l2_device *v4l2_dev;
	unsigned int len;
	int ret = 0, id = 0;

	if ((!pq_dev) || (!pqdev))
		return -ENOMEM;

	spin_lock_init(&pq_dev->slock);
	mutex_init(&pq_dev->mutex);

	vdev = &pq_dev->video_dev;
	v4l2_dev = &pq_dev->v4l2_dev;

	pq_dev->dev = pqdev->dev;
	pq_dev->dev_indx = dev_id;

	//patch here
	pq_dev->b2r_dev.id = pqdev->b2r_dev.id;
	pq_dev->b2r_dev.irq = pqdev->b2r_dev.irq;

	snprintf(v4l2_dev->name, sizeof(v4l2_dev->name),
		"%s", PQ_V4L2_DEVICE_NAME);
	ret = v4l2_device_register(NULL, v4l2_dev);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register v4l2 device\n");
		goto exit;
	}

	pq_dev->m2m.m2m_dev = v4l2_m2m_init(&mtk_pq_m2m_ops);
	if (IS_ERR(pq_dev->m2m.m2m_dev)) {
		ret = PTR_ERR(pq_dev->m2m.m2m_dev);
		v4l2_err(v4l2_dev, "failed to init m2m device\n");
		goto exit;
	}

	v4l2_ctrl_handler_init(&pq_dev->ctrl_handler, 0);
	v4l2_dev->ctrl_handler = &pq_dev->ctrl_handler;

	ret = mtk_pq_register_common_subdev(v4l2_dev, &pq_dev->subdev_common,
		&pq_dev->common_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register common sub dev\n");
		goto exit;
	}
	ret = mtk_pq_register_enhance_subdev(v4l2_dev, &pq_dev->subdev_enhance,
		&pq_dev->enhance_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register drv pq sub dev\n");
		goto exit;
	}
	ret = mtk_pq_register_display_subdev(v4l2_dev, &pq_dev->subdev_display,
		&pq_dev->display_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register display sub dev\n");
		goto exit;
	}
	ret = mtk_pq_register_b2r_subdev(v4l2_dev, &pq_dev->subdev_b2r,
		&pq_dev->b2r_ctrl_handler);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register b2r sub dev\n");
		goto exit;
	}

	vdev->fops = &mtk_pq_fops;
	vdev->ioctl_ops = &mtk_pq_dec_ioctl_ops;
	vdev->release = video_device_release;
	vdev->v4l2_dev = v4l2_dev;
	vdev->vfl_dir = VFL_DIR_M2M;
	//device name is "mtk-pq-dev" + "0"/"1"/...
	len = snprintf(vdev->name, sizeof(vdev->name), "%s",
		PQ_VIDEO_DEVICE_NAME);
	snprintf(vdev->name + len, sizeof(vdev->name) - len, "%s",
		devID[dev_id]);

	if (pqdev->cus_dev)
		id = pqdev->cus_id[dev_id];
	else
		id = -1;

	ret = video_register_device(vdev, VFL_TYPE_GRABBER, id);
	if (ret) {
		v4l2_err(v4l2_dev, "failed to register video device!\n");
		goto exit;
	}

	video_set_drvdata(vdev, pq_dev);
	v4l2_info(v4l2_dev, "mtk-pq registered as /dev/video%d\n", vdev->num);
	pqdev->pqcaps.u32Device_register_Num[dev_id] = vdev->num;
	return 0;

exit:
	_mtk_pq_unregister_v4l2dev(pq_dev);

	return -EPERM;
}

static int _mtk_pq_probe(struct platform_device *pdev)
{
	boottime_print("MTK PQ insmod [begin]\n");
	struct mtk_pq_device *pq_dev;
	int ret = 0;
	__u8 win_num;
	__u8 cnt;
	__u64 stream_pa = 0;
	__u32 stream_size = 0;
	__u32 stream_offset = 0;

	pqdev = devm_kzalloc(&pdev->dev,
		sizeof(struct mtk_pq_platform_device), GFP_KERNEL);
	if (!pqdev)
		return -ENOMEM;

	pqdev->display_dev.pReserveBufTbl = NULL;

	pqdev->dev = &pdev->dev;

	ret = _mtk_pq_parse_dts(pqdev);
	if (ret) {
		PQ_MSG_ERROR("Failed to parse pq cap dts\r\n");
		return ret;
	}

	ret = _mtk_pq_set_clock(pqdev, true);
	if (ret) {
		PQ_MSG_ERROR("Failed to init clock\r\n");
		return ret;
	}

	ret = mtk_pq_enhance_parse_dts(&pqdev->pq_enhance, pdev);
	if (ret) {
		PQ_MSG_ERROR("Failed to parse pq enhance dts\r\n");
		return ret;
	}

	ret = mtk_display_parse_dts(&pqdev->display_dev, pqdev);
	if (ret) {
		PQ_MSG_ERROR("Failed to parse display dts\r\n");
		return ret;
	}

	ret = mtk_b2r_parse_dts(&pqdev->b2r_dev, pdev);
	if (ret) {
		PQ_MSG_ERROR("Failed to parse b2r dts\r\n");
		return ret;
	}

	ret = _mtk_pq_config_open(pqdev);
	if (ret) {
		PQ_MSG_ERROR("Failed to parse pq config dts\r\n");
		return ret;
	}

	ret = mtk_pq_buffer_reserve_buf_init(pqdev);
	if (ret) {
		PQ_MSG_ERROR("Failed to mtk_pq_buffer_reserve_buf_init\r\n");
		return ret;
	}

	ret = pq_init_subdevices(pqdev);
	ret = mtk_pq_common_irq_init(pqdev);
	if (ret != 0) {
		PQ_MSG_ERROR("Failed to init irq\r\n");
		return ret;
	}

	ret = mtk_display_dynamic_ultra_init(pqdev);
	if (ret != 0) {
		PQ_MSG_ERROR("Failed to init dynamic_ultra\r\n");
		return ret;
	}

	// prepare pqmap dedicated buffer
	ret = _mtk_pq_get_memory_info(pqdev->dev, MMAP_PQMAP_INDEX,
		&pqdev->DDBuf[MMAP_PQMAP_INDEX].pa, &pqdev->DDBuf[MMAP_PQMAP_INDEX].size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"Failed to _mtk_pq_get_memory_info(index=%d)\n", MMAP_PQMAP_INDEX);
		return ret;
	}
	pqdev->DDBuf[MMAP_PQMAP_INDEX].va =
		(__u64)memremap(pqdev->DDBuf[MMAP_PQMAP_INDEX].pa,
		pqdev->DDBuf[MMAP_PQMAP_INDEX].size, MEMREMAP_WB);
	if (pqdev->DDBuf[MMAP_PQMAP_INDEX].va == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Failed to memremap(0x%llx, 0x%x)\n",
			pqdev->DDBuf[MMAP_PQMAP_INDEX].pa, pqdev->DDBuf[MMAP_PQMAP_INDEX].size);
		return -ENOMEM;
	}

	// prepare pqparam dedicated buffer
	ret = _mtk_pq_get_memory_info(pqdev->dev, MMAP_PQPARAM_INDEX,
		&pqdev->DDBuf[MMAP_PQPARAM_INDEX].pa, &pqdev->DDBuf[MMAP_PQPARAM_INDEX].size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"Failed to _mtk_pq_get_memory_info(index=%d)\n", MMAP_PQPARAM_INDEX);
		return ret;
	}
	pqdev->DDBuf[MMAP_PQPARAM_INDEX].va =
		(__u64)memremap(pqdev->DDBuf[MMAP_PQPARAM_INDEX].pa,
		pqdev->DDBuf[MMAP_PQPARAM_INDEX].size, MEMREMAP_WB);
	if (pqdev->DDBuf[MMAP_PQPARAM_INDEX].va == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Failed to memremap(0x%llx, 0x%x)\n",
			pqdev->DDBuf[MMAP_PQPARAM_INDEX].pa, pqdev->DDBuf[MMAP_PQPARAM_INDEX].size);
		return -ENOMEM;
	}

	ret = mtk_pq_cdev_probe(pdev);
	if (ret != 0)
		PQ_MSG_ERROR("Failed to cdev_probe\r\n");

	win_num = pqdev->usable_win;
	if (win_num > PQ_WIN_MAX_NUM) {
		PQ_MSG_ERROR("DTS error, can not create too many devices\r\n");
		win_num = PQ_WIN_MAX_NUM;
	}

	ret = _mtk_pq_get_memory_info(pqdev->dev, MMAP_STREAM_METADATA_INDEX,
					&stream_pa, &stream_size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		"Failed to _mtk_pq_get_memory_info(index=%d)\n", MMAP_STREAM_METADATA_INDEX);
		return ret;
	}
	stream_offset = stream_size / win_num;
	stream_offset = ALIGN_DOWNTO_16(stream_offset);

	for (cnt = 0; cnt < win_num; cnt++) {
		pq_dev = devm_kzalloc(&pdev->dev,
			sizeof(struct mtk_pq_device), GFP_KERNEL);
		pqdev->pq_devs[cnt] = pq_dev;
		if (!pq_dev)
			goto exit;

		/* for kernel 5.4.1 */
		pqdev->pq_devs[cnt]->video_dev.device_caps = V4L2_CAP_VIDEO_M2M_MPLANE |
			V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_CAPTURE;

		if (_mtk_pq_register_v4l2dev(pq_dev, cnt, pqdev)) {
			PQ_MSG_ERROR("Failed to register v4l2 dev\r\n");
			goto exit;
		}

		pq_dev->stream_meta.dev = pq_dev->dev;
		pq_dev->memory_bus_base = 0x20000000;//fix me
		pq_dev->pqu_stream_meta.pa = stream_pa + stream_offset*cnt;
		pq_dev->pqu_stream_meta.size = stream_offset;
		pq_dev->pqu_stream_meta.va = (__u64)memremap(pq_dev->pqu_stream_meta.pa,
						pq_dev->pqu_stream_meta.size, MEMREMAP_WC);
		if (pq_dev->pqu_stream_meta.va == NULL) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "memremap fail, cnt=%d\n", cnt);
			goto exit;
		}
		pq_dev->display_info.idr.input_mode = MTK_PQ_INPUT_MODE_HW;
		memcpy(&pq_dev->b2r_dev, &pqdev->b2r_dev,
			sizeof(struct b2r_device));
		init_waitqueue_head(&(pq_dev->wait));
	}

	/* dolby init process start */
	ret = _mtk_pq_dv_init(pqdev);
	if (ret < 0)
		PQ_MSG_ERROR("_mtk_pq_dv_init return %d\n", ret);
	/* dolby init process end */

	// equal to dev_set_drvdata(&pdev->dev, pqdev);
	// you can get "pqdev" by platform_get_drvdata(pdev)
	// or dev_get_drvdata(&pdev->dev)
	platform_set_drvdata(pdev, pqdev);

	mtk_pq_common_trigger_gen_init(false);

	mtk_pq_CreateSysFS(&pdev->dev);

	boottime_print("MTK PQ insmod [end]\n");

	return 0;

exit:
	for (cnt = 0; cnt < win_num; cnt++) {
		pq_dev = pqdev->pq_devs[cnt];
		if (pq_dev) {
			_mtk_pq_unregister_v4l2dev(pq_dev);
			if (pq_dev->pqu_stream_meta.va != NULL) {
				memunmap((void *)pq_dev->pqu_stream_meta.va);
				memset(&pq_dev->pqu_stream_meta, 0,
						sizeof(struct mtk_pq_remap_buf));
			}
		}
	}
	if (pqdev->DDBuf[MMAP_PQMAP_INDEX].va != NULL) {
		memunmap((void *)pqdev->DDBuf[MMAP_PQMAP_INDEX].va);
		memset((void *)pqdev->DDBuf[MMAP_PQMAP_INDEX].va,
			0, sizeof(struct mtk_pq_remap_buf));
	}
	if (pqdev->DDBuf[MMAP_PQPARAM_INDEX].va != NULL) {
		memunmap((void *)pqdev->DDBuf[MMAP_PQPARAM_INDEX].va);
		memset((void *)pqdev->DDBuf[MMAP_PQPARAM_INDEX].va,
			0, sizeof(struct mtk_pq_remap_buf));
	}

	return -EPERM;
}

static int _mtk_pq_remove(struct platform_device *pdev)
{
	struct mtk_pq_platform_device *pqdev = platform_get_drvdata(pdev);
	struct mtk_pq_device *pq_dev = NULL;
	__u8 win_num = 0;
	__u8 cnt = 0;

	if (!pqdev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!");
		return -EINVAL;
	}

	win_num = pqdev->usable_win;

	mtk_pq_buffer_reserve_buf_exit(pqdev);

	mtk_pq_cdev_remove(pdev);

	mtk_pq_RemoveSysFS(&pdev->dev);
	for (cnt = 0; cnt < win_num; cnt++) {
		pq_dev = pqdev->pq_devs[cnt];
		if (pq_dev) {
			_mtk_pq_unregister_v4l2dev(pq_dev);

			if (pq_dev->pqu_stream_meta.va != NULL) {
				memunmap((void *)pq_dev->pqu_stream_meta.va);
				memset(&pq_dev->pqu_stream_meta, 0,
						sizeof(struct mtk_pq_remap_buf));
			}
		}
	}
	if (pqdev->hwmap_config.va != NULL) {
		memunmap((void *)pqdev->hwmap_config.va);
		memset((void *)pqdev->hwmap_config.va, 0, sizeof(struct mtk_pq_remap_buf));
	}
	if (pqdev->dv_config.va != NULL) {
		memunmap((void *)pqdev->dv_config.va);
		memset((void *)pqdev->dv_config.va, 0, sizeof(struct mtk_pq_remap_buf));
	}
	if (pqdev->DDBuf[MMAP_PQMAP_INDEX].va != NULL) {
		memunmap((void *)pqdev->DDBuf[MMAP_PQMAP_INDEX].va);
		memset((void *)pqdev->DDBuf[MMAP_PQMAP_INDEX].va,
			0, sizeof(struct mtk_pq_remap_buf));
	}
	if (pqdev->DDBuf[MMAP_PQPARAM_INDEX].va != NULL) {
		memunmap((void *)pqdev->DDBuf[MMAP_PQPARAM_INDEX].va);
		memset((void *)pqdev->DDBuf[MMAP_PQPARAM_INDEX].va,
			0, sizeof(struct mtk_pq_remap_buf));
	}

	return 0;
}

static int _mtk_pq_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	mtk_pq_common_irq_suspend(false);

	return 0;
}

static int _mtk_pq_resume(struct platform_device *pdev)
{
	mtk_pq_common_irq_resume(false, pqdev->pqcaps.u32IRQ_Version);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pq_pm_runtime_force_suspend(struct device *dev)
{
	int ret = 0;
	struct mtk_pq_platform_device *pdev = dev_get_drvdata(dev);

	ret = mtk_pq_common_suspend(false);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Failed to suspend pq\r\n");

	_mtk_pq_set_clock(pdev, false);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Failed to disable clock\r\n");

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "%s\n", __func__);

	return pm_runtime_force_suspend(dev);
}

static int pq_pm_runtime_force_resume(struct device *dev)
{
	int ret = 0;

	struct mtk_pq_platform_device *pdev = dev_get_drvdata(dev);

	ret = _mtk_pq_set_clock(pdev, true);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Failed to enable clock\r\n");

	ret = _mtk_pq_config_open(pdev);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Failed to parse pq config dts\r\n");

	ret = mtk_pq_common_resume(false, pdev->pqcaps.u32IRQ_Version);
	if (ret)
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Failed to resume pq\r\n");

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_COMMON, "%s\n", __func__);

	return pm_runtime_force_resume(dev);
}

static const struct dev_pm_ops pq_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
		pq_pm_runtime_force_suspend,
		pq_pm_runtime_force_resume
	)
};
#endif

static struct platform_driver pq_pdrv = {
	.probe      = _mtk_pq_probe,
	.remove     = _mtk_pq_remove,
	.suspend    = _mtk_pq_suspend,
	.resume     = _mtk_pq_resume,
	.driver     = {
		.name = "mtk-pq",
		.of_match_table = mtk_pq_match,
		.owner = THIS_MODULE,
#ifdef CONFIG_PM_SLEEP
		.pm = &pq_pm_ops,
#endif
	},
};

static int __init _mtk_pq_init(void)
{
	boottime_print("MTK PQ init [begin]\n");
	platform_driver_register(&pq_pdrv);
	boottime_print("MTK PQ init [end]\n");
	return 0;
}

static void __exit _mtk_pq_exit(void)
{
	platform_driver_unregister(&pq_pdrv);
}

module_param_named(pqu_enable, bPquEnable, bool, 0660);
module_init(_mtk_pq_init);
module_exit(_mtk_pq_exit);

MODULE_AUTHOR("Kevin Ren <kevin.ren@mediatek.com>");
MODULE_DESCRIPTION("mtk pq device driver");
MODULE_LICENSE("GPL v2");

