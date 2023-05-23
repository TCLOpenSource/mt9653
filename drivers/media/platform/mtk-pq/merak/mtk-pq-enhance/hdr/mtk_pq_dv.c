// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include "pqu_msg.h"

#include "mtk_pq.h"
#include "mtk_pq_dv.h"
#include "mtk_pq_dv_version.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define DV_DMA_ALIGN(val, align) (((val) + ((align) - 1)) & (~((align) - 1)))

#define DV_PYR_CHANNEL_SIZE      (10)
#define DV_PYR_FRAME_NUM         (3)
#define DV_PYR_RW_DIFF           (1)
#define DV_PYR_0_IDX             (0)
#define DV_PYR_0_WIDTH           (1024)
#define DV_PYR_0_HEIGHT          (576)
#define DV_PYR_1_IDX             (1)
#define DV_PYR_1_WIDTH           (512)
#define DV_PYR_1_HEIGHT          (288)
#define DV_PYR_2_IDX             (2)
#define DV_PYR_2_WIDTH           (256)
#define DV_PYR_2_HEIGHT          (144)
#define DV_PYR_3_IDX             (3)
#define DV_PYR_3_WIDTH           (128)
#define DV_PYR_3_HEIGHT          (72)
#define DV_PYR_4_IDX             (4)
#define DV_PYR_4_WIDTH           (64)
#define DV_PYR_4_HEIGHT          (36)
#define DV_PYR_5_IDX             (5)
#define DV_PYR_5_WIDTH           (32)
#define DV_PYR_5_HEIGHT          (18)
#define DV_PYR_6_IDX             (6)
#define DV_PYR_6_WIDTH           (16)
#define DV_PYR_6_HEIGHT          (9)
#define DV_BIT_PER_WORD          (256)
#define DV_BIT_PER_BYTE          (8)
#define DV_MAX_CMD_LENGTH        (0xFF)
#define DV_MAX_ARG_NUM           (64)
#define DV_CMD_REMOVE_LEN        (2)

#define Fld(wid, shft, ac)  (((uint32_t)wid<<16)|(shft<<8)|ac)
#define AC_FULLB0  (1)
#define AC_FULLB1  (2)
#define AC_FULLB2  (3)
#define AC_FULLB3  (4)
#define AC_FULLW10 (5)
#define AC_FULLW21 (6)
#define AC_FULLW32 (7)
#define AC_FULLDW  (8)
#define AC_MSKB0   (9)
#define AC_MSKB1   (10)
#define AC_MSKB2   (11)
#define AC_MSKB3   (12)
#define AC_MSKW10  (13)
#define AC_MSKW21  (14)
#define AC_MSKW32  (15)
#define AC_MSKDW   (16)

#define MASK_FULLDW  (0xFFFFFFFF)
#define MASK_FULLW32 (0xFFFF0000)
#define MASK_FULLW21 (0x00FFFF00)
#define MASK_FULLW10 (0x0000FFFF)
#define MASK_FULLB3  (0xFF000000)
#define MASK_FULLB2  (0x00FF0000)
#define MASK_FULLB1  (0x0000FF00)
#define MASK_FULLB0  (0x000000FF)

#define MASK_X32_BASE_BANK (0xFFF000)
#define MASK_X32_BANK      (0xF00)
#define MASK_X32_ADDR      (0xFF)
#define MASK_X16_ADDR      (0xFF)
#define MUL_X32_ADDR_32TO8 (4)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
int g_mtk_pq_dv_debug_level;

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
static unsigned int dv_get_bits_set(unsigned int v)
{
	// c accumulates the total bits set in v
	unsigned int c;

	for (c = 0; v; c++) {
		// clear the least significant bit set
		v &= v - 1;
	}

	return c;
}

static unsigned int dv_get_ac(uint32_t mask)
{
	bool MaskB0 = FALSE, MaskB1 = FALSE, MaskB2 = FALSE, MaskB3 = FALSE;

	switch (mask) {
	case MASK_FULLDW:
		return AC_FULLDW;
	case MASK_FULLW32:
		return AC_FULLW32;
	case MASK_FULLW21:
		return AC_FULLW21;
	case MASK_FULLW10:
		return AC_FULLW10;
	case MASK_FULLB3:
		return AC_FULLB3;
	case MASK_FULLB2:
		return AC_FULLB2;
	case MASK_FULLB1:
		return AC_FULLB1;
	case MASK_FULLB0:
		return AC_FULLB0;
	}

	MaskB0 = !!(mask & MASK_FULLB0);
	MaskB1 = !!(mask & MASK_FULLB1);
	MaskB2 = !!(mask & MASK_FULLB2);
	MaskB3 = !!(mask & MASK_FULLB3);

	if (MaskB3 && MaskB2 && MaskB1 && MaskB0)
		return AC_MSKDW;
	else if (MaskB3 && MaskB2)
		return AC_MSKW32;
	else if (MaskB2 && MaskB1)
		return AC_MSKW21;
	else if (MaskB1 && MaskB0)
		return AC_MSKW10;
	else if (MaskB3)
		return AC_MSKB3;
	else if (MaskB2)
		return AC_MSKB2;
	else if (MaskB1)
		return AC_MSKB1;
	else if (MaskB0)
		return AC_MSKB0;

	return 0;
}

static bool dv_is_X32_bank(u32 addr)
{
	u32 addr_masked = 0;

	addr_masked = addr & MASK_X32_BASE_BANK;

	switch (addr_masked) {
	case 0xAF0000: /* DV core 1  */
	case 0xAF1000: /* DV core 1b */
	case 0xAE0000: /* DV core 2  */
	case 0xAE8000: /* DV core 2b */
		return TRUE;
	default:
		return FALSE;
	}

	return FALSE;
}

static int dv_parse_cmd_helper(char *buf, char *sep[], int max_cnt)
{
	char delim[] = " =,\n\r";
	char **b = &buf;
	char *token = NULL;
	int cnt = 0;

	while (cnt < max_cnt) {
		token = strsep(b, delim);
		if (token == NULL)
			break;
		sep[cnt++] = token;
	}

	// Exclude command and new line symbol
	return (cnt - DV_CMD_REMOVE_LEN);
}

static int dv_debug_print_help(void)
{
	int ret = 0;

	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		"----------------dv information start----------------\n");
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		"DV_PQ_VERSION: %u\n", DV_PQ_VERSION);
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		"----------------dv information end----------------\n\n");

	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		"----------------debug commands help start----------------\n");
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		" set_debug_level=debug_level_pq,debug_level_pqu,debug_level_3pty\n");
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		" force_reg=en,addr,val,mask,addr2,val2,mask2,addr3,val3,mask3\n");
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		" force_viewmode=en,view_id\n");
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		"               =1,1\n");
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		"----------------debug commands help end------------------\n");

	return  ret;
}

static int dv_debug_set_debug_level(
	const char *args[],
	int arg_num,
	struct mtk_pq_dv_debug *dv_debug)
{
	int ret = 0;
	int arg_idx;
	u32 val;

	if (dv_debug == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (arg_num < 3)
		return -EINVAL;

	arg_idx = 0;
	ret = kstrtoint(args[arg_idx++], 0, &val);
	if (ret < 0)
		goto exit;
	g_mtk_pq_dv_debug_level = val;

	ret = kstrtoint(args[arg_idx++], 0, &val);
	if (ret < 0)
		goto exit;
	dv_debug->debug_level.debug_level_pqu = val;

	ret = kstrtoint(args[arg_idx++], 0, &val);
	if (ret < 0)
		goto exit;
	dv_debug->debug_level.debug_level_3pty = val;

	dv_debug->debug_level.en = TRUE;

exit:
	return  ret;
}

static int dv_debug_force_reg(
	const char *args[],
	int arg_num,
	struct mtk_pq_dv_debug *dv_debug)
{
	int ret = 0;
	int arg_idx;
	int reg_idx;
	u32 addr;
	u32 val;
	u32 mask;
	u32 mask_width, LSB, AC;

	if (dv_debug == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (arg_num < 1)
		return -EINVAL;

	arg_idx = 0;
	ret = kstrtouint(args[arg_idx++], 0, &val);
	if (ret < 0)
		goto exit;
	dv_debug->force_reg.en = !!val;

	dv_debug->force_reg.reg_num = 0;
	while (arg_idx + 2 < arg_num) {
		ret = kstrtouint(args[arg_idx++], 0, &addr);
		ret |= kstrtouint(args[arg_idx++], 0, &val);
		ret |= kstrtouint(args[arg_idx++], 0, &mask);
		if (ret < 0)
			continue;

		reg_idx = dv_debug->force_reg.reg_num;
		if (reg_idx >= MTK_PQ_DV_DBG_MAX_SET_REG_NUM)
			break;

		if (dv_is_X32_bank(addr)) {
			/* the input addr is XYZPQR, XYZ is basebank, like AF0, AF1, AE0, AE8. */
			/* X is bank number. YZ is address in 32bit format*/
			dv_debug->force_reg.regs[reg_idx].addr = ((addr&MASK_X32_BASE_BANK)<<1)
							+ (addr&MASK_X32_BANK)
							+ (addr&MASK_X32_ADDR)*MUL_X32_ADDR_32TO8;
			dv_debug->force_reg.regs[reg_idx].val = val;
			dv_debug->force_reg.regs[reg_idx].mask = MASK_FULLDW;
		} else {
			// get mask width [15:8] gives 8. mask must be continuous
			mask_width = dv_get_bits_set(mask);

			// get LSB
			LSB = ffs(mask) - 1;

			// get AC
			AC = dv_get_ac(mask);

			if (AC == 0)
				continue;

			dv_debug->force_reg.regs[reg_idx].addr =
				((addr + (addr & MASK_X16_ADDR)) << 1);
			dv_debug->force_reg.regs[reg_idx].val = (val >> LSB);
			dv_debug->force_reg.regs[reg_idx].mask = Fld(mask_width, LSB, AC);
		}
		dv_debug->force_reg.reg_num++;
	}

exit:
	return  ret;
}

static int dv_debug_force_viewmode(
	const char *args[],
	int arg_num,
	struct mtk_pq_dv_debug *dv_debug)
{
	int ret = 0;
	int arg_idx;
	u32 en;
	u32 val;

	if (dv_debug == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (arg_num < 2)
		return -EINVAL;

	arg_idx = 0;

	/* arg 0: en */
	ret = kstrtouint(args[arg_idx++], 0, &en);
	if (ret < 0)
		goto exit;
	dv_debug->force_ctrl.view_mode.en = !!en;

	/* arg 1: view_id */
	ret = kstrtouint(args[arg_idx++], 0, &val);
	if (ret < 0)
		goto exit;
	dv_debug->force_ctrl.view_mode.view_id = (uint8_t)val;

exit:
	return  ret;
}

static int dv_debug_force_pr(
	const char *args[],
	int arg_num,
	struct mtk_pq_dv_debug *dv_debug)
{
	int ret = 0;
	int arg_idx = 0;
	int extra_frame_num_valid = 0;
	int extra_frame_num = 0;
	int pr_en_valid = 0;
	int pr_en = 0;

	if (dv_debug == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (arg_num < 1)
		return -EINVAL;

	arg_idx = 0;
	ret = kstrtoint(args[arg_idx++], 0, &extra_frame_num_valid);
	if (ret < 0)
		goto exit;

	ret = kstrtoint(args[arg_idx++], 0, &extra_frame_num);
	if (ret < 0)
		goto exit;

	ret = kstrtoint(args[arg_idx++], 0, &pr_en_valid);
	if (ret < 0)
		goto exit;

	ret = kstrtoint(args[arg_idx++], 0, &pr_en);
	if (ret < 0)
		goto exit;

	dv_debug->force_pr.extra_frame_num_valid = !!extra_frame_num_valid;
	dv_debug->force_pr.extra_frame_num = extra_frame_num;
	dv_debug->force_pr.pr_en_valid = !!pr_en_valid;
	dv_debug->force_pr.pr_en = pr_en;

exit:
	return  ret;
}

/* dolby debug process for qbuf */
static int dv_debug_qbuf(
	struct mtk_pq_device *pq_dev,
	struct meta_pq_dv_info *meta_dv_pq,
	struct m_pq_dv_debug *meta_dv_debug)
{
	int ret = 0;
	struct mtk_pq_platform_device *pqdev = NULL;
	struct mtk_pq_dv_debug *dv_debug = NULL;

	if ((pq_dev == NULL) ||
		(meta_dv_pq == NULL) ||
		(meta_dv_debug == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	pqdev = dev_get_drvdata(pq_dev->dev);
	dv_debug = &pqdev->dv_ctrl.debug;

	/* prepare dolby debug info into */
	if (dv_debug->debug_level.en) {
		meta_dv_debug->debug_level.valid = TRUE;
		meta_dv_debug->debug_level.debug_level_pqu =
			dv_debug->debug_level.debug_level_pqu;
		meta_dv_debug->debug_level.debug_level_3pty =
			dv_debug->debug_level.debug_level_3pty;
	}

	if (dv_debug->force_reg.en) {
		meta_dv_debug->set_reg.valid = TRUE;
		meta_dv_debug->set_reg.reg_num = dv_debug->force_reg.reg_num;
		memcpy(meta_dv_debug->set_reg.regs,
			dv_debug->force_reg.regs,
			sizeof(meta_dv_debug->set_reg.regs));
	}

	if (dv_debug->force_ctrl.view_mode.en) {
		meta_dv_debug->force_ctrl.view_mode.en = TRUE;
		meta_dv_debug->force_ctrl.view_mode.view_id =
			dv_debug->force_ctrl.view_mode.view_id;
	}

	if (dv_debug->force_pr.extra_frame_num_valid)
		pq_dev->dv_win_info.common.extra_frame_num =
			dv_debug->force_pr.extra_frame_num;

	if (dv_debug->force_pr.pr_en_valid)
		meta_dv_pq->pr_ctrl.en = dv_debug->force_pr.pr_en;

exit:
	return ret;
}

static int dv_allocate_pyr_buf(struct mtk_pq_device *pq_dev)
{
	int ret = 0, i = 0;
	struct mtk_pq_platform_device *pqdev = NULL;
	u64 offset = 0;

	if (pq_dev == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if (pq_dev->dv_win_info.pyr_buf.valid) {
		MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
			"pyr buffer exist\n");
		goto exit;
	}

	if (pq_dev->dv_win_info.pyr_buf.size == 0) {
		MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
			"pyr buffer size = 0\n");
		goto exit;
	}

	pqdev = dev_get_drvdata(pq_dev->dev);

	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
		"mmap addr: %llx, mmap size: %x, required size: %x\n",
		pqdev->dv_ctrl.pyr_ctrl.mmap_addr,
		pqdev->dv_ctrl.pyr_ctrl.mmap_size,
		pq_dev->dv_win_info.pyr_buf.size);

	/* NOT an error if buffer allocation fail */
	if (pq_dev->dv_win_info.pyr_buf.size >
		pqdev->dv_ctrl.pyr_ctrl.mmap_size) {
		MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
			"mmap size not enough\n");
		goto exit;
	}

	/* NOT an error if buffer allocation fail */
	if (pqdev->dv_ctrl.pyr_ctrl.mmap_addr == 0) {
		MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
			"mmap addr = 0\n");
		goto exit;
	}

	/* allocate buffer */
	pq_dev->dv_win_info.pyr_buf.addr =
		(pqdev->dv_ctrl.pyr_ctrl.mmap_addr -
		 BUSADDRESS_TO_IPADDRESS_OFFSET);

	offset = 0;
	for (i = 0; i < MTK_PQ_DV_PYR_NUM; i++) {
		/* unit: n bit align */
		pq_dev->dv_win_info.pyr_buf.pyr_addr[i] =
			(pq_dev->dv_win_info.pyr_buf.addr + offset) *
			DV_BIT_PER_BYTE / DV_BIT_PER_WORD;

		/* unit: byte */
		offset += pq_dev->dv_win_info.pyr_buf.pyr_size[i];

		MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
			"pyramid: %d, addr: %llx, pitch: %x, size: %x\n",
			i,
			pq_dev->dv_win_info.pyr_buf.pyr_addr[i],
			pq_dev->dv_win_info.pyr_buf.frame_pitch[i],
			pq_dev->dv_win_info.pyr_buf.pyr_size[i]);
	}

	/* set dolby window info */
	pq_dev->dv_win_info.pyr_buf.valid = TRUE;

	/* set dolby global control */
	pqdev->dv_ctrl.pyr_ctrl.available = FALSE;

exit:
	return ret;
}

static int dv_free_pyr_buf(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	struct mtk_pq_platform_device *pqdev = NULL;

	if (pq_dev == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	pqdev = dev_get_drvdata(pq_dev->dev);

	if (pq_dev->dv_win_info.pyr_buf.valid) {
		/* set dolby window info */
		pq_dev->dv_win_info.pyr_buf.valid = FALSE;

		/* set dolby global control */
		pqdev->dv_ctrl.pyr_ctrl.available = TRUE;
	}

exit:
	return ret;
}

static int dv_try_allocate_pyr_buf(struct mtk_pq_device *pq_dev)
{
	int ret = 0;
	struct mtk_pq_platform_device *pqdev = NULL;

	if (pq_dev == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	pqdev = dev_get_drvdata(pq_dev->dev);
	if ((pqdev->dv_ctrl.pyr_ctrl.available) &&
		(!pq_dev->dv_win_info.pyr_buf.valid) &&
		(pq_dev->dv_win_info.pr_ctrl.is_dolby) &&
		(pq_dev->dv_win_info.pr_ctrl.width > 0) &&
		(pq_dev->dv_win_info.pr_ctrl.height > 0))
		ret = dv_allocate_pyr_buf(pq_dev);

exit:
	return ret;
}

static int dv_set_pr(struct mtk_pq_device *pq_dev)
{
	int ret = 0;

	if (pq_dev == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	pq_dev->dv_win_info.pr_ctrl.en = FALSE;

	if ((pq_dev->dv_win_info.pyr_buf.valid) &&
		(pq_dev->dv_win_info.pr_ctrl.is_dolby) &&
		(pq_dev->dv_win_info.pr_ctrl.is_dolby ==
		 pq_dev->dv_win_info.pr_ctrl.pre_is_dolby))
		pq_dev->dv_win_info.pr_ctrl.en = TRUE;

exit:
	return ret;
}

static int dv_set_extra_frame_num(struct mtk_pq_device *pq_dev)
{
	int ret = 0;

	if (pq_dev == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	if ((pq_dev->dv_win_info.pr_ctrl.en) ||
		(pq_dev->dv_win_info.common.extra_frame_num == 1))
		pq_dev->dv_win_info.common.extra_frame_num = 1;

exit:
	return ret;
}

static int dv_get_pr_info(struct mtk_pq_device *pq_dev, struct meta_buffer *meta_buf)
{
	int ret = 0;
	int meta_ret = 0;
	struct meta_srccap_dv_info *meta_dv_hdmi_info = NULL;
	struct vdec_dd_dolby_desc *meta_dv_vdec_info = NULL;
	struct mtk_pq_frame_info *meta_frame_info = NULL;

	if ((pq_dev == NULL) || (meta_buf == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	meta_dv_hdmi_info = kzalloc(sizeof(struct meta_srccap_dv_info), GFP_KERNEL);
	if (meta_dv_hdmi_info == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	meta_dv_vdec_info = kzalloc(sizeof(struct vdec_dd_dolby_desc), GFP_KERNEL);
	if (meta_dv_vdec_info == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	meta_frame_info = kzalloc(sizeof(struct mtk_pq_frame_info), GFP_KERNEL);
	if (meta_frame_info == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	/* get dolby info in hdmi case */
	meta_ret = mtk_pq_common_read_metadata_addr(meta_buf,
			EN_PQ_METATAG_SRCCAP_DV_HDMI_INFO, (void *)meta_dv_hdmi_info);
	if (meta_ret >= 0) {
		/* enum srccap_dv_descrb_interface */
		if ((meta_dv_hdmi_info->descrb.interface > M_DV_INTERFACE_NONE) &&
			(meta_dv_hdmi_info->descrb.interface < M_DV_INTERFACE_MAX))
			pq_dev->dv_win_info.pr_ctrl.is_dolby = TRUE;

		if (meta_dv_hdmi_info->dma.status == M_DV_DMA_STATUS_ENABLE_FB) {
			pq_dev->dv_win_info.pr_ctrl.width =
				meta_dv_hdmi_info->dma.width;
			pq_dev->dv_win_info.pr_ctrl.height =
				meta_dv_hdmi_info->dma.height;
		}
	}

	/* get dolby info in ott case */
	meta_ret = mtk_pq_common_read_metadata_addr(meta_buf,
			EN_PQ_METATAG_VDEC_DV_DESCRB_INFO, (void *)meta_dv_vdec_info);
	meta_ret |= mtk_pq_common_read_metadata_addr(meta_buf,
			EN_PQ_METATAG_SH_FRM_INFO, (void *)meta_frame_info);
	if (meta_ret >= 0) {
		/* enum vdec_dolby_type */
		if (meta_dv_vdec_info->dolby_type != VDEC_DOLBY_NONE)
			pq_dev->dv_win_info.pr_ctrl.is_dolby = TRUE;

		if (meta_frame_info->submodifier.vsd_mode != 0) {
			pq_dev->dv_win_info.pr_ctrl.width =
				meta_frame_info->stSubFrame.u32Width;
			pq_dev->dv_win_info.pr_ctrl.height =
				meta_frame_info->stSubFrame.u32Height;
		} else if (meta_frame_info->modifier.compress == 0) {
			pq_dev->dv_win_info.pr_ctrl.width =
				meta_frame_info->stFrames[0].u32Width;
			pq_dev->dv_win_info.pr_ctrl.height =
				meta_frame_info->stFrames[0].u32Height;
		} else {
			pq_dev->dv_win_info.pr_ctrl.width = 0;
			pq_dev->dv_win_info.pr_ctrl.height = 0;
			MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
				"vsd mode of sub1 = none and compress of main = 1\n");
		}
	}

exit:
	if (meta_dv_hdmi_info != NULL)
		kfree(meta_dv_hdmi_info);

	if (meta_dv_vdec_info != NULL)
		kfree(meta_dv_vdec_info);

	if (meta_frame_info != NULL)
		kfree(meta_frame_info);

	return ret;
}

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------
int mtk_pq_dv_ctrl_init(
	struct mtk_pq_dv_ctrl_init_in *in,
	struct mtk_pq_dv_ctrl_init_out *out)
{
	int ret = 0;

	if ((in == NULL) || (out == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	/* set dolby global control */
	in->pqdev->dv_ctrl.pyr_ctrl.mmap_size = in->mmap_size;
	in->pqdev->dv_ctrl.pyr_ctrl.mmap_addr = in->mmap_addr;
	in->pqdev->dv_ctrl.pyr_ctrl.available = TRUE;

	memset(&in->pqdev->dv_ctrl.debug, 0, sizeof(struct mtk_pq_dv_debug));

	g_mtk_pq_dv_debug_level = MTK_PQ_DV_DBG_LEVEL_ERR;

exit:
	return ret;
}

int mtk_pq_dv_win_init(
	struct mtk_pq_dv_win_init_in *in,
	struct mtk_pq_dv_win_init_out *out)
{
	int ret = 0;

	if ((in == NULL) || (out == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	/* set dolby window info */
	in->dev->dv_win_info.pr_ctrl.is_dolby = FALSE;
	in->dev->dv_win_info.pr_ctrl.pre_is_dolby = FALSE;
	in->dev->dv_win_info.pr_ctrl.width = 0;
	in->dev->dv_win_info.pr_ctrl.height = 0;
	in->dev->dv_win_info.pyr_buf.valid = FALSE;

exit:
	return ret;
}

int mtk_pq_dv_streamon(
	struct mtk_pq_dv_streamon_in *in,
	struct mtk_pq_dv_streamon_out *out)
{
	int ret = 0;
	int i = 0;
	struct mtk_pq_dv_pyr_buf *pyr_buf = NULL;

	if ((in == NULL) || (out == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	pyr_buf = &(in->dev->dv_win_info.pyr_buf);

	/* set dolby window info */
	in->dev->dv_win_info.common.extra_frame_num = 0;
	in->dev->dv_win_info.pr_ctrl.is_dolby = FALSE;
	in->dev->dv_win_info.pr_ctrl.pre_is_dolby = FALSE;
	in->dev->dv_win_info.pr_ctrl.width = 0;
	in->dev->dv_win_info.pr_ctrl.height = 0;
	pyr_buf->valid = FALSE;

	/* calculate size, unit: n bit align */
	pyr_buf->frame_num = DV_PYR_FRAME_NUM;
	pyr_buf->rw_diff = DV_PYR_RW_DIFF;
	pyr_buf->frame_pitch[DV_PYR_0_IDX] =
		DV_PYR_0_HEIGHT
		* DV_DMA_ALIGN((DV_PYR_0_WIDTH * DV_PYR_CHANNEL_SIZE), DV_BIT_PER_WORD)
		/ DV_BIT_PER_WORD;
	pyr_buf->pyr_width[DV_PYR_0_IDX] = DV_PYR_0_WIDTH;
	pyr_buf->pyr_height[DV_PYR_0_IDX] = DV_PYR_0_HEIGHT;
	pyr_buf->frame_pitch[DV_PYR_1_IDX] =
		DV_PYR_1_HEIGHT
		* DV_DMA_ALIGN((DV_PYR_1_WIDTH * DV_PYR_CHANNEL_SIZE), DV_BIT_PER_WORD)
		/ DV_BIT_PER_WORD;
	pyr_buf->pyr_width[DV_PYR_1_IDX] = DV_PYR_1_WIDTH;
	pyr_buf->pyr_height[DV_PYR_1_IDX] = DV_PYR_1_HEIGHT;
	pyr_buf->frame_pitch[DV_PYR_2_IDX] =
		DV_PYR_2_HEIGHT
		* DV_DMA_ALIGN((DV_PYR_2_WIDTH * DV_PYR_CHANNEL_SIZE), DV_BIT_PER_WORD)
		/ DV_BIT_PER_WORD;
	pyr_buf->pyr_width[DV_PYR_2_IDX] = DV_PYR_2_WIDTH;
	pyr_buf->pyr_height[DV_PYR_2_IDX] = DV_PYR_2_HEIGHT;
	pyr_buf->frame_pitch[DV_PYR_3_IDX] =
		DV_PYR_3_HEIGHT
		* DV_DMA_ALIGN((DV_PYR_3_WIDTH * DV_PYR_CHANNEL_SIZE), DV_BIT_PER_WORD)
		/ DV_BIT_PER_WORD;
	pyr_buf->pyr_width[DV_PYR_3_IDX] = DV_PYR_3_WIDTH;
	pyr_buf->pyr_height[DV_PYR_3_IDX] = DV_PYR_3_HEIGHT;
	pyr_buf->frame_pitch[DV_PYR_4_IDX] =
		DV_PYR_4_HEIGHT
		* DV_DMA_ALIGN((DV_PYR_4_WIDTH * DV_PYR_CHANNEL_SIZE), DV_BIT_PER_WORD)
		/ DV_BIT_PER_WORD;
	pyr_buf->pyr_width[DV_PYR_4_IDX] = DV_PYR_4_WIDTH;
	pyr_buf->pyr_height[DV_PYR_4_IDX] = DV_PYR_4_HEIGHT;
	pyr_buf->frame_pitch[DV_PYR_5_IDX] =
		DV_PYR_5_HEIGHT
		* DV_DMA_ALIGN((DV_PYR_5_WIDTH * DV_PYR_CHANNEL_SIZE), DV_BIT_PER_WORD)
		/ DV_BIT_PER_WORD;
	pyr_buf->pyr_width[DV_PYR_5_IDX] = DV_PYR_5_WIDTH;
	pyr_buf->pyr_height[DV_PYR_5_IDX] = DV_PYR_5_HEIGHT;
	pyr_buf->frame_pitch[DV_PYR_6_IDX] =
		DV_PYR_6_HEIGHT
		* DV_DMA_ALIGN((DV_PYR_6_WIDTH * DV_PYR_CHANNEL_SIZE), DV_BIT_PER_WORD)
		/ DV_BIT_PER_WORD;
	pyr_buf->pyr_width[DV_PYR_6_IDX] = DV_PYR_6_WIDTH;
	pyr_buf->pyr_height[DV_PYR_6_IDX] = DV_PYR_6_HEIGHT;
	pyr_buf->size = 0;
	for (i = 0; i < MTK_PQ_DV_PYR_NUM; i++) {
		/* unit: byte */
		pyr_buf->pyr_size[i] =
			(pyr_buf->frame_pitch[i] * DV_BIT_PER_WORD / DV_BIT_PER_BYTE) *
			pyr_buf->frame_num;
		pyr_buf->size += pyr_buf->pyr_size[i];
	}

exit:
	return ret;
}

int mtk_pq_dv_streamoff(
	struct mtk_pq_dv_streamoff_in *in,
	struct mtk_pq_dv_streamoff_out *out)
{
	int ret = 0;

	if ((in == NULL) || (out == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	ret = dv_free_pyr_buf(in->dev);
	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	return ret;
}

int mtk_pq_dv_set_ambient(void *ctrl, struct mtk_pq_device *pq_dev)
{
	int ret = 0;

	if ((pq_dev == NULL) || (ctrl == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	memcpy(&(pq_dev->dv_win_info.ambient), ctrl, sizeof(struct meta_pq_dv_ambient));

	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_DEBUG,
		"front sensor lux = %lld, mode = %d\n",
		pq_dev->dv_win_info.ambient.s64FrontLux, pq_dev->dv_win_info.ambient.u32Mode);

exit:
	return ret;
}

int mtk_pq_dv_qbuf(
	struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf)
{
	int ret = 0, fd = 0, i = 0;
	struct mtk_pq_platform_device *pqdev = NULL;
	struct mtk_pq_dv_debug *dv_debug = NULL;
	struct mtk_pq_dv_pyr_buf *dv_pyr_buf = NULL;
	struct meta_pq_dv_info meta_dv_pq;
	struct m_pq_dv_debug *meta_dv_debug = NULL;
	struct meta_buffer meta_buf;

	if ((pq_dev == NULL) || (pq_buf == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	meta_dv_debug = kzalloc(sizeof(struct m_pq_dv_debug), GFP_KERNEL);
	if (meta_dv_debug == NULL) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	memset(&meta_dv_pq, 0, sizeof(struct meta_pq_dv_info));
	memset(&meta_buf, 0, sizeof(struct meta_buffer));

	pqdev = dev_get_drvdata(pq_dev->dev);
	dv_debug = &pqdev->dv_ctrl.debug;

	fd = pq_buf->vb.planes[pq_buf->vb.vb2_buf.num_planes - 1].m.fd;
	dv_pyr_buf = &(pq_dev->dv_win_info.pyr_buf);

	pq_dev->dv_win_info.pr_ctrl.en = FALSE;
	pq_dev->dv_win_info.pr_ctrl.pre_is_dolby =
		pq_dev->dv_win_info.pr_ctrl.is_dolby;
	pq_dev->dv_win_info.pr_ctrl.is_dolby = FALSE;
	pq_dev->dv_win_info.pr_ctrl.width = 0;
	pq_dev->dv_win_info.pr_ctrl.height = 0;

	meta_buf.paddr = pq_buf->meta_buf.va;
	meta_buf.size = pq_buf->meta_buf.size;

	/* get pr info */
	ret = dv_get_pr_info(pq_dev, &meta_buf);
	if (ret) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	/* try to allocate pyramid buffer */
	ret = dv_try_allocate_pyr_buf(pq_dev);
	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	/* determine pr status */
	ret = dv_set_pr(pq_dev);
	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	/* set extra frame number */
	ret = dv_set_extra_frame_num(pq_dev);
	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	/* prepare dolby pq info */
	meta_dv_pq.pyr.valid = dv_pyr_buf->valid;
	meta_dv_pq.pyr.frame_num = dv_pyr_buf->frame_num;
	meta_dv_pq.pyr.rw_diff = dv_pyr_buf->rw_diff;
	for (i = 0; i < MTK_PQ_DV_PYR_NUM; i++) {
		meta_dv_pq.pyr.frame_pitch[i] = dv_pyr_buf->frame_pitch[i];
		meta_dv_pq.pyr.addr[i]        = dv_pyr_buf->pyr_addr[i];
		meta_dv_pq.pyr.width[i]       = dv_pyr_buf->pyr_width[i];
		meta_dv_pq.pyr.height[i]      = dv_pyr_buf->pyr_height[i];
	}
	meta_dv_pq.pr_ctrl.en = pq_dev->dv_win_info.pr_ctrl.en;
	meta_dv_pq.pr_ctrl.fe_in_width = pq_dev->dv_win_info.pr_ctrl.width;
	meta_dv_pq.pr_ctrl.fe_in_height = pq_dev->dv_win_info.pr_ctrl.height;

	/* prepare ambient info of dolby Adv. SDK light sense */
	memcpy(&(meta_dv_pq.ambient), &(pq_dev->dv_win_info.ambient),
		sizeof(struct meta_pq_dv_ambient));
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_DEBUG,
		"front sensor lux = %lld, mode = %d\n",
		meta_dv_pq.ambient.s64FrontLux, meta_dv_pq.ambient.u32Mode);

	/* dolby debug process */
	ret = dv_debug_qbuf(pq_dev, &meta_dv_pq, meta_dv_debug);
	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	/* write dolby debug info into metadata */
	ret = mtk_pq_common_write_metadata_addr(&meta_buf,
			EN_PQ_METATAG_DV_DEBUG, meta_dv_debug);
	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

	/* write dolby pq info into metadata */
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_PR,
		"%s%d, %s%d, %s%u, %s%u, %s%d\n",
		"pyramid valid: ", pq_dev->dv_win_info.pyr_buf.valid,
		"pr enable: ", pq_dev->dv_win_info.pr_ctrl.en,
		"width: ", pq_dev->dv_win_info.pr_ctrl.width,
		"height: ", pq_dev->dv_win_info.pr_ctrl.height,
		"extra frame number: ", pq_dev->dv_win_info.common.extra_frame_num);

	ret = mtk_pq_common_write_metadata_addr(&meta_buf, EN_PQ_METATAG_DV_INFO, &meta_dv_pq);
	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	if (meta_dv_debug != NULL)
		kfree(meta_dv_debug);

	return ret;
}

int mtk_pq_dv_show(struct device *dev, const char *buf)
{
	int ssize = 0;

	if ((dev == NULL) || (buf == NULL))
		return 0;

	dv_debug_print_help();

	return ssize;
}

int mtk_pq_dv_store(struct device *dev, const char *buf)
{
	int ret = 0;
	char cmd[DV_MAX_CMD_LENGTH];
	char *args[DV_MAX_ARG_NUM] = {NULL};
	int arg_num;
	struct mtk_pq_platform_device *pqdev = NULL;
	struct mtk_pq_dv_debug *dv_debug = NULL;

	if ((dev == NULL) || (buf == NULL)) {
		MTK_PQ_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	pqdev = dev_get_drvdata(dev);
	dv_debug = &pqdev->dv_ctrl.debug;

	strncpy(cmd, buf, DV_MAX_CMD_LENGTH);
	cmd[DV_MAX_CMD_LENGTH - 1] = '\0';

	arg_num = dv_parse_cmd_helper(cmd, args, DV_MAX_ARG_NUM);
	MTK_PQ_DV_LOG_TRACE(MTK_PQ_DV_DBG_LEVEL_ERR,
		"cmd: %s, num: %d\n", cmd, arg_num);

	if (strncmp(cmd, "set_debug_level", DV_MAX_CMD_LENGTH) == 0) {
		ret = dv_debug_set_debug_level((const char **)&args[1], arg_num, dv_debug);
	} else if (strncmp(cmd, "force_reg", DV_MAX_CMD_LENGTH) == 0) {
		ret = dv_debug_force_reg((const char **)&args[1], arg_num, dv_debug);
	} else if (strncmp(cmd, "force_viewmode", DV_MAX_CMD_LENGTH) == 0) {
		ret = dv_debug_force_viewmode((const char **)&args[1], arg_num, dv_debug);
	} else if (strncmp(cmd, "force_pr", DV_MAX_CMD_LENGTH) == 0) {
		ret = dv_debug_force_pr((const char **)&args[1], arg_num, dv_debug);
	} else {
		ret = dv_debug_print_help();
		goto exit;
	}

	if (ret < 0) {
		MTK_PQ_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	return ret;
}

//-----------------------------------------------------------------------------
// Debug Functions
//-----------------------------------------------------------------------------
