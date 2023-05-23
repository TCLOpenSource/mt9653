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
#include <linux/types.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <uapi/linux/mtk-v4l2-pq.h>

#include "mtk_pq.h"
#include "mtk_pq_buffer.h"
#include "mtk_pq_svp.h"
#include "mtk_pq_display_b2r.h"
#include "mtk_pq_display_mdw.h"
#include "mtk_pq_display_idr.h"

#include "pqu_msg.h"
#include "m_pqu_pq.h"
#include "apiXC.h"

#include "hwreg_pq_display_ip2.h"

//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------

#define DYNAMIC_ULTRA_REG_NUM (200)
/*main and sub,multi win shall enlarge it, TODO: this should put in DTS*/
#define FIELD_TO_INTERLACE(x) \
	(((x) == V4L2_FIELD_ANY || (x) == V4L2_FIELD_NONE) ? FALSE : TRUE)

struct pq_display_aisr_dbg _gAisrdbg;

static int _mtk_display_cal_rwdiff(__s32 *rwdiff, struct mtk_pq_device *pq_dev)
{
	int diff = 0;
	struct pq_buffer pq_buf;
	enum pqu_buffer_type buf_idx;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	memset(&pq_buf, 0, sizeof(struct pq_buffer));

	for (buf_idx = PQU_BUF_SCMI; buf_idx < PQU_BUF_MAX; buf_idx++) {
		mtk_get_pq_buffer(pq_dev, buf_idx, &pq_buf);

		if (pq_buf.frame_num != 0)
			diff += pq_buf.frame_diff;
	}

	*rwdiff = diff;

	return 0;
}

static int _display_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev = NULL;
	struct mtk_pq_platform_device *pqdev = NULL;

	if (ctrl == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid Parameters!\n");
		return -EINVAL;
	}

	pq_dev = container_of(ctrl->handler,
	    struct mtk_pq_device, display_ctrl_handler);
	pqdev = dev_get_drvdata(pq_dev->dev);

	if (pq_dev->dev_indx >= pqdev->usable_win) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_GET_DISPLAY_DATA:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Is empty now\n");
		break;
	case V4L2_CID_GET_RW_DIFF:
		ret = _mtk_display_cal_rwdiff(&(ctrl->val), pq_dev);
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "V4L2_CID_GET_RW_DIFF\n");
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int _display_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_pq_device *pq_dev = NULL;
	struct mtk_pq_platform_device *pqdev = NULL;

	if (ctrl == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid Parameters!\n");
		return -EINVAL;
	}

	pq_dev = container_of(ctrl->handler,
	    struct mtk_pq_device, display_ctrl_handler);
	pqdev = dev_get_drvdata(pq_dev->dev);

	if (pq_dev->dev_indx >= pqdev->usable_win) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
		    "Invalid WindowID!!WindowID = %d\n", pq_dev->dev_indx);
		return -EINVAL;
	}

	int ret = 0;

	switch (ctrl->id) {
	case V4L2_CID_SET_DISPLAY_DATA:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "Is empty now\n");
		break;
	case V4L2_CID_PQ_SECURE_MODE:
#if IS_ENABLED(CONFIG_OPTEE)
		ret = mtk_pq_svp_set_sec_md(pq_dev, ctrl->p_new.p_u8);
#endif
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static bool _mtk_pq_get_dbg_value_from_string(char *buf, char *name,
	unsigned int len, __u32 *value)
{
	bool find = false;
	char *string, *tmp_name, *cmd, *tmp_value;

	if ((buf == NULL) || (name == NULL) || (value == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return false;
	}

	*value = 0;
	cmd = buf;

	for (string = buf; string < (buf + len); ) {
		strsep(&cmd, " =\n\r");
		for (string += strlen(string); string < (buf + len) && !*string;)
			string++;
	}

	for (string = buf; string < (buf + len); ) {
		tmp_name = string;
		for (string += strlen(string); string < (buf + len) && !*string;)
			string++;

		tmp_value = string;
		for (string += strlen(string); string < (buf + len) && !*string;)
			string++;

		if (strncmp(tmp_name, name, strlen(name) + 1) == 0) {
			if (kstrtoint(tmp_value, 0, value)) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
						"kstrtoint fail!\n");
				return find;
			}
			find = true;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
					"name = %s, value = %d\n", name, *value);
			return find;
		}
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"name(%s) was not found!\n", name);

	return find;
}

static bool _mtk_pq_get_aisr_dbg_type(char *buf, char *name,
	unsigned int len)
{
	bool find = false;
	char *string, *tmp_name, *cmd, *tmp_value;

	if ((buf == NULL) || (name == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return false;
	}

	cmd = buf;

	for (string = buf; string < (buf + len); ) {
		strsep(&cmd, " =\n\r");
		for (string += strlen(string); string < (buf + len) && !*string;)
			string++;
	}

	for (string = buf; string < (buf + len); ) {
		tmp_name = string;
		for (string += strlen(string); string < (buf + len) && !*string;)
			string++;

		tmp_value = string;
		for (string += strlen(string); string < (buf + len) && !*string;)
			string++;

		if (strncmp(tmp_name, name, strlen(name) + 1) == 0) {
			find = true;
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "type = %s\n", name);
			return find;
		}
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"name(%s) was not found!\n", name);

	return find;
}

static int _mtk_pq_get_aisr_dbg_mode(char *buf,
	unsigned int len, enum AISR_DBG_TYPE *type)
{
	bool find = false;
	char *cmd;

	if ((buf == NULL) || (type == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return false;
	}

	*type = MTK_PQ_AISR_DBG_NONE;
	cmd = buf;

	find = _mtk_pq_get_aisr_dbg_type(cmd, "aisr_en", len);
	if (find) {
		*type = MTK_PQ_AISR_DBG_ENABLE;
		return 0;
	}

	find = _mtk_pq_get_aisr_dbg_type(cmd, "active_win", len);
	if (find) {
		*type = MTK_PQ_AISR_DBG_ACTIVE_WIN;
		return 0;
	}

	return -EINVAL;
}

static int _mtk_pq_set_aisr_dbg(
	struct mtk_pq_device *pq_dev,
	struct m_pq_display_flow_ctrl *pqu_meta)
{
	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (pqu_meta == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (_gAisrdbg.bdbgModeEn) {
		if (!_gAisrdbg.bAISREn && pqu_meta->aisr_en) {
			memcpy(&(pqu_meta->ip_win[pqu_ip_aisr_out]),
				&(pqu_meta->ip_win[pqu_ip_aisr_in]),
				sizeof(struct window_info));
			memcpy(&(pqu_meta->ip_win[pqu_ip_hvsp_in]),
				&(pqu_meta->ip_win[pqu_ip_aisr_out]),
				sizeof(struct window_info));
			pqu_meta->aisr_en = 0;
		} else if (_gAisrdbg.bAISREn && pqu_meta->aisr_en) {
			if (_gAisrdbg.u32Scale > 0) {
				pqu_meta->ip_win[pqu_ip_aisr_out].height =
					pqu_meta->ip_win[pqu_ip_aisr_in].height *
						_gAisrdbg.u32Scale;
				pqu_meta->ip_win[pqu_ip_aisr_out].width =
					pqu_meta->ip_win[pqu_ip_aisr_in].width *
						_gAisrdbg.u32Scale;
				memcpy(&(pqu_meta->ip_win[pqu_ip_hvsp_in]),
					&(pqu_meta->ip_win[pqu_ip_aisr_out]),
					sizeof(struct window_info));
			}
		}

		if ((pqu_meta->ip_win[pqu_ip_display].width != 0) &&
			(pqu_meta->ip_win[pqu_ip_display].height != 0)) {
			pqu_meta->bAisrActiveWinEn =
				_gAisrdbg.aisrActiveWin.bEnable;
			pqu_meta->aisr_active_win_info.x =
				(pqu_meta->ip_win[pqu_ip_aisr_out].width *
					_gAisrdbg.aisrActiveWin.x) /
					pqu_meta->ip_win[pqu_ip_display].width;
			pqu_meta->aisr_active_win_info.y =
				(pqu_meta->ip_win[pqu_ip_aisr_out].height *
					_gAisrdbg.aisrActiveWin.y) /
					pqu_meta->ip_win[pqu_ip_display].height;
			pqu_meta->aisr_active_win_info.width =
				(pqu_meta->ip_win[pqu_ip_aisr_out].width *
					_gAisrdbg.aisrActiveWin.width) /
					pqu_meta->ip_win[pqu_ip_display].width;
			pqu_meta->aisr_active_win_info.height =
				(pqu_meta->ip_win[pqu_ip_aisr_out].height *
					_gAisrdbg.aisrActiveWin.height) /
					pqu_meta->ip_win[pqu_ip_display].height;
		}
	}
	return 0;
}

static const struct v4l2_ctrl_ops display_ctrl_ops = {
	.g_volatile_ctrl = _display_g_ctrl,
	.s_ctrl = _display_s_ctrl,
};

static const struct v4l2_ctrl_config display_ctrl[] = {
	{
		.ops = &display_ctrl_ops,
		.id = V4L2_CID_SET_DISPLAY_DATA,
		.name = "set display manager data",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct mtk_pq_display_s_data)},
	},
	{
		.ops = &display_ctrl_ops,
		.id = V4L2_CID_GET_DISPLAY_DATA,
		.name = "get display manager data",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(struct mtk_pq_display_g_data)},
	},
	{
		.ops = &display_ctrl_ops,
		.id = V4L2_CID_GET_RW_DIFF,
		.name = "get display rw diff",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &display_ctrl_ops,
		.id = V4L2_CID_PQ_SECURE_MODE,
		.name = "xc secure mode",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.dims = {sizeof(u8)},
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops display_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops display_sd_internal_ops = {
};

//-----------------------------------------------------------------------------
//  FUNCTION
//-----------------------------------------------------------------------------
int mtk_display_set_frame_metadata(struct mtk_pq_device *pq_dev,
	struct mtk_pq_buffer *pq_buf)
{
	struct m_pq_display_flow_ctrl pqu_meta;
	struct meta_pq_display_flow_info pq_meta;
	struct meta_pq_input_queue_ext_info pq_inputq_ext_info;
	struct m_pq_queue_ext_info pqu_queue_ext_info;
	struct mtk_pq_thermal thermal_info;
	int ret = 0;
	struct meta_buffer meta_buf;
	__u8 input_source = 0;

	if (pq_dev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	if (pq_buf == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	memset(&pqu_meta, 0, sizeof(struct m_pq_display_flow_ctrl));
	memset(&pq_meta, 0, sizeof(struct meta_pq_display_flow_info));
	memset(&pq_inputq_ext_info, 0, sizeof(struct meta_pq_input_queue_ext_info));
	memset(&pqu_queue_ext_info, 0, sizeof(struct m_pq_queue_ext_info));
	memset(&meta_buf, 0, sizeof(struct meta_buffer));
	memset(&thermal_info, 0, sizeof(struct mtk_pq_thermal));

	/* meta buffer handle */
	meta_buf.paddr = pq_buf->meta_buf.va;
	meta_buf.size = pq_buf->meta_buf.size;

	ret = mtk_pq_common_read_metadata_addr(
		&meta_buf, EN_PQ_METATAG_DISPLAY_FLOW_INFO, &pq_meta);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_DISPLAY_FLOW_INFO Failed, ret = %d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_DISPLAY_FLOW_INFO success\n");
	}

	/* meta get window info */
	memcpy(&(pqu_meta.content), &(pq_meta.content),
				sizeof(struct meta_pq_window));
	memcpy(&(pqu_meta.cap_win), &(pq_meta.capture),
				sizeof(struct meta_pq_window));
	memcpy(&(pqu_meta.crop_win), &(pq_meta.crop),
				sizeof(struct meta_pq_window));
	memcpy(&(pqu_meta.disp_win), &(pq_meta.display),
				sizeof(struct meta_pq_window));
	memcpy(&(pqu_meta.displayArea), &(pq_meta.displayArea),
				sizeof(struct meta_pq_window));
	memcpy(&(pqu_meta.displayRange), &(pq_meta.displayRange),
				sizeof(struct meta_pq_window));
	memcpy(&(pqu_meta.wm_config), &(pq_meta.wm_config),
				sizeof(struct m_pq_wm_config));
	memcpy(&(pqu_meta.ip_win), &(pq_meta.ip_win),
				sizeof(struct meta_pq_window)*meta_ip_max);

	pqu_meta.flowControlEn = pq_dev->display_info.flowControl.enable;
	pqu_meta.flowControlFactor = pq_dev->display_info.flowControl.factor;

	pqu_meta.aisr_en = pq_meta.aisr_enable;
	pqu_meta.bAisrActiveWinEn = pq_dev->display_info.aisrActiveWin.bEnable;
	pqu_meta.aisr_active_win_info.x = pq_dev->display_info.aisrActiveWin.x;
	pqu_meta.aisr_active_win_info.y = pq_dev->display_info.aisrActiveWin.y;
	pqu_meta.aisr_active_win_info.width = pq_dev->display_info.aisrActiveWin.width;
	pqu_meta.aisr_active_win_info.height = pq_dev->display_info.aisrActiveWin.height;

	_mtk_pq_set_aisr_dbg(pq_dev, &pqu_meta);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"m_pq_display_flow_ctrl : %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d), %s(%d,%d,%d,%d)\n",
	"content = ", pqu_meta.content.x, pqu_meta.content.y,
		pqu_meta.content.width, pqu_meta.content.height,
	"cap_win = ", pqu_meta.cap_win.x, pqu_meta.cap_win.y,
		pqu_meta.cap_win.width, pqu_meta.cap_win.height,
	"crop_win = ", pqu_meta.crop_win.x, pqu_meta.crop_win.y,
		pqu_meta.crop_win.width, pqu_meta.crop_win.height,
	"disp_win = ", pqu_meta.disp_win.x, pqu_meta.disp_win.y,
		pqu_meta.disp_win.width, pqu_meta.disp_win.height,
	"displayArea = ", pqu_meta.displayArea.x, pqu_meta.displayArea.y,
		pqu_meta.displayArea.width, pqu_meta.displayArea.height,
	"displayRange = ", pqu_meta.displayRange.x, pqu_meta.displayRange.y,
		pqu_meta.displayRange.width, pqu_meta.displayRange.height);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"m_pq_display_get_ip_size : %s(%d,%d), %s(%d,%d), %s(%d,%d), %s(%d,%d), %s(%d,%d), %s(%d,%d)\n",
	"scmi out = ", pqu_meta.ip_win[pqu_ip_scmi_out].width,
		pqu_meta.ip_win[pqu_ip_scmi_out].height,
	"aisr_in = ", pqu_meta.ip_win[pqu_ip_aisr_in].width,
		pqu_meta.ip_win[pqu_ip_aisr_in].height,
	"aisr_out = ", pqu_meta.ip_win[pqu_ip_aisr_out].width,
		pqu_meta.ip_win[pqu_ip_aisr_out].height,
	"hvsp_in = ", pqu_meta.ip_win[pqu_ip_hvsp_in].width,
		pqu_meta.ip_win[pqu_ip_hvsp_in].height,
	"hvsp_out = ", pqu_meta.ip_win[pqu_ip_hvsp_out].width,
		pqu_meta.ip_win[pqu_ip_hvsp_out].height,
	"display = ", pqu_meta.ip_win[pqu_ip_display].width,
		pqu_meta.ip_win[pqu_ip_display].height);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"[FC]Flow control enable:%d factor:%d\n",
		pqu_meta.flowControlEn,
		pqu_meta.flowControlFactor);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"[AISR]Active win enable:%u x:%u, y:%u, w:%u, h:%u\n",
		pqu_meta.bAisrActiveWinEn,
		pqu_meta.aisr_active_win_info.x,
		pqu_meta.aisr_active_win_info.y,
		pqu_meta.aisr_active_win_info.width,
		pqu_meta.aisr_active_win_info.height);

	input_source = pq_dev->common_info.input_source;
	if (IS_INPUT_B2R(input_source)) {
		pqu_meta.InputHTT = pq_dev->b2r_info.timing_out.H_TotalCount;
		pqu_meta.InputVTT = pq_dev->b2r_info.timing_out.V_TotalCount;
	} else {
		pqu_meta.InputHTT = pq_dev->display_info.idr.inputHTT;
		pqu_meta.InputVTT = pq_dev->display_info.idr.inputVTT;
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
		"input_source = %d, InputHTT = %ld, InputVTT = %ld\n",
		input_source, pqu_meta.InputHTT, pqu_meta.InputVTT);

	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_PQU_DISP_FLOW_INFO, &pqu_meta);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_PQU_DISP_FLOW_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_PQU_DISP_FLOW_INFO success\n");
	}

	ret = mtk_pq_common_read_metadata_addr(
		&meta_buf, EN_PQ_METATAG_INPUT_QUEUE_EXT_INFO, &pq_inputq_ext_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq read EN_PQ_METATAG_INPUT_QUEUE_EXT_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq read EN_PQ_METATAG_INPUT_QUEUE_EXT_INFO success\n");
	}

	pqu_queue_ext_info.u64Pts = pq_inputq_ext_info.u64Pts;
	pqu_queue_ext_info.u64UniIdx = pq_inputq_ext_info.u64UniIdx;
	pqu_queue_ext_info.u64ExtUniIdx = pq_inputq_ext_info.u64ExtUniIdx;
	pqu_queue_ext_info.u64TimeStamp = pq_inputq_ext_info.u64TimeStamp;
	pqu_queue_ext_info.u64RenderTimeNs = pq_inputq_ext_info.u64RenderTimeNs;
	pqu_queue_ext_info.u8BufferValid = pq_inputq_ext_info.u8BufferValid;
	pqu_queue_ext_info.u32BufferSlot = pq_inputq_ext_info.u32BufferSlot;
	pqu_queue_ext_info.u32GenerationIndex = pq_inputq_ext_info.u32GenerationIndex;
	pqu_queue_ext_info.u8RepeatStatus = pq_inputq_ext_info.u8RepeatStatus;
	pqu_queue_ext_info.u8FrcMode = pq_inputq_ext_info.u8FrcMode;
	pqu_queue_ext_info.u8Interlace = pq_inputq_ext_info.u8Interlace;
	pqu_queue_ext_info.u32InputFps = pq_inputq_ext_info.u32InputFps;
	pqu_queue_ext_info.u32OriginalInputFps = pq_inputq_ext_info.u32OriginalInputFps;
	pqu_queue_ext_info.bEOS = pq_inputq_ext_info.bEOS;
	pqu_queue_ext_info.u8MuteAction = pq_inputq_ext_info.u8MuteAction;
	pqu_queue_ext_info.u8SignalStable = pq_inputq_ext_info.u8SignalStable;
	pqu_queue_ext_info.u8DotByDotType = pq_inputq_ext_info.u8DotByDotType;
	pqu_queue_ext_info.u32RefreshRate = pq_inputq_ext_info.u32RefreshRate;
	pqu_queue_ext_info.u32QueueInputIndex = pq_inputq_ext_info.u32QueueInputIndex;

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
	"m_pq_queue_ext_info : %s%llu, %s%llu, %s%llu, %s%llu, %s%llu, %s%u, %s%u, %s%u, %s%u, %s%u, %s%u, %s%u, %s%d, %s%u, %s%u, %s%u, %s%u, %s%u\n",
	"u64Pts = ", pqu_queue_ext_info.u64Pts,
	"u64UniIdx = ", pqu_queue_ext_info.u64UniIdx,
	"u64ExtUniIdx = ", pqu_queue_ext_info.u64ExtUniIdx,
	"u64TimeStamp = ", pqu_queue_ext_info.u64TimeStamp,
	"u64RenderTimeNs = ", pqu_queue_ext_info.u64RenderTimeNs,
	"u8BufferValid = ", pqu_queue_ext_info.u8BufferValid,
	"u32GenerationIndex = ", pqu_queue_ext_info.u32GenerationIndex,
	"u8RepeatStatus = ", pqu_queue_ext_info.u8RepeatStatus,
	"u8FrcMode = ", pqu_queue_ext_info.u8FrcMode,
	"u8Interlace = ", pqu_queue_ext_info.u8Interlace,
	"u32InputFps = ", pqu_queue_ext_info.u32InputFps,
	"u32OriginalInputFps = ", pqu_queue_ext_info.u32OriginalInputFps,
	"bEOS = ", pqu_queue_ext_info.bEOS,
	"u8MuteAction = ", pqu_queue_ext_info.u8MuteAction,
	"u8SignalStable = ", pqu_queue_ext_info.u8SignalStable,
	"u8DotByDotType = ", pqu_queue_ext_info.u8DotByDotType,
	"u32RefreshRate = ", pqu_queue_ext_info.u32RefreshRate,
	"u32QueueOutputIndex = ", pqu_queue_ext_info.u32QueueInputIndex);

	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_PQU_QUEUE_EXT_INFO, &pqu_queue_ext_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_PQU_QUEUE_EXT_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_PQU_QUEUE_EXT_INFO success\n");
	}

	mtk_pq_cdev_get_thermal_info(&thermal_info);
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "Thermal disable aisr=%u\n",
		thermal_info.thermal_disable_aisr);

	ret = mtk_pq_common_write_metadata_addr(
		&meta_buf, EN_PQ_METATAG_PQU_THERMAL_INFO, &thermal_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"mtk-pq write EN_PQ_METATAG_PQU_THERMAL_INFO fail, ret=%d\n", ret);
		goto exit;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG,
			"mtk-pq write EN_PQ_METATAG_PQU_THERMAL_INFO success\n");
	}

exit:
	return ret;
}

int mtk_display_abf_blk_mapping(uint16_t mode)
{
	switch (mode) {
	case PQ_ABF_BLK_MODE1:
		return PQ_ABF_BLK_SIZE1;
	case PQ_ABF_BLK_MODE2:
		return PQ_ABF_BLK_SIZE2;
	case PQ_ABF_BLK_MODE3:
		return PQ_ABF_BLK_SIZE3;
	case PQ_ABF_BLK_MODE4:
		return PQ_ABF_BLK_SIZE4;
	default:
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "abf block mode error\n");
		return PQ_ABF_BLK_SIZE4;
	}
}

void mtk_display_abf_blk_mode(int width, uint16_t *ctur_mode, uint16_t *ctur2_mode)
{
	if (width <= PQ_ABF_CUTR_MODE1_SIZE) {
		*ctur_mode = mtk_display_abf_blk_mapping(PQ_ABF_BLK_MODE2);
		*ctur2_mode = mtk_display_abf_blk_mapping(PQ_ABF_BLK_MODE1);
	} else if (width <= PQ_ABF_CUTR_MODE2_SIZE) {
		*ctur_mode = mtk_display_abf_blk_mapping(PQ_ABF_BLK_MODE3);
		*ctur2_mode = mtk_display_abf_blk_mapping(PQ_ABF_BLK_MODE2);
	} else {
		*ctur_mode = mtk_display_abf_blk_mapping(PQ_ABF_BLK_MODE4);
		*ctur2_mode = mtk_display_abf_blk_mapping(PQ_ABF_BLK_MODE3);
	}
}

int mtk_display_dynamic_ultra_init(struct mtk_pq_platform_device *pqdev)
{
	struct hwregOut hwOut;
	struct reg_info *regTbl;

	if (pqdev == NULL) {
		PQ_CHECK_NULLPTR();
		return -EPERM;
	}

	regTbl = vmalloc(sizeof(struct reg_info) * DYNAMIC_ULTRA_REG_NUM);
	if (!regTbl) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "vmalloc fail\n");
		return -EPERM;
	}

	memset(&hwOut, 0, sizeof(struct hwregOut));
	memset(regTbl, 0, sizeof(struct reg_info)*DYNAMIC_ULTRA_REG_NUM);

	hwOut.reg = regTbl;

	drv_hwreg_pq_display_dynamic_ultra_init(1, &hwOut);

	if (regTbl != NULL)
		vfree(regTbl);

	return 0;
}

int mtk_display_parse_dts(struct display_device *display_dev, struct mtk_pq_platform_device *pqdev)
{
	int ret = 0;
	struct device_node *np;
	struct device *property_dev = pqdev->dev;
	__u32 u32Tmp = 0;
	const char *name;

	if (property_dev->of_node)
		of_node_get(property_dev->of_node);

	np = of_find_node_by_name(property_dev->of_node, "display");

	if (np != NULL) {
		// read byte per word
		name = "byte_per_word";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->bpw = (u8)u32Tmp;

		name = "BUF_IOMMU_OFFSET";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->buf_iommu_offset = u32Tmp;

		// read pq buffer tag
		name = "PQ_IOMMU_IDX_SCMI";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->pq_iommu_idx_scmi = u32Tmp;

		// read pq buffer tag
		name = "PQ_IOMMU_IDX_UCM";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->pq_iommu_idx_ucm = u32Tmp;

		// read usable window count
		name = "USABLE_WIN";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		pqdev->usable_win = u32Tmp;

		// read h pixel alignment
		name = "PQ_H_PIXEL_ALIGN";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->pixel_align = u32Tmp;

		// read h pixel alignment
		name = "PQ_H_MAX_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->h_max_size = u32Tmp;

		// read h pixel alignment
		name = "PQ_V_MAX_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->v_max_size = u32Tmp;

		// read znr me h max size
		name = "PQ_ZNR_ME_H_MAX_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->znr_me_h_max_size = u32Tmp;

		// read znr me v max size
		name = "PQ_ZNR_ME_V_MAX_SIZE";
		ret = of_property_read_u32(np, name, &u32Tmp);
		if (ret != 0x0)
			goto Fail;
		display_dev->znr_me_v_max_size = u32Tmp;

		// read mdw dts
		ret = mtk_pq_display_mdw_read_dts(np, &display_dev->mdw_dev);
		if (ret != 0x0)
			goto Fail;
#if (1)//(MEMC_CONFIG == 1)
		ret = mtk_pq_display_frc_read_dts(&display_dev->frc_dev);
		if (ret != 0x0)
			goto Fail;
#endif
	}

	of_node_put(np);

	return 0;

Fail:
	if (np != NULL)
		of_node_put(np);

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY,
		"[%s, %d]: read DTS failed, name = %s\n",
		__func__, __LINE__, name);
	return ret;
}

int mtk_pq_register_display_subdev(struct v4l2_device *pq_dev,
			struct v4l2_subdev *subdev_display,
			struct v4l2_ctrl_handler *display_ctrl_handler)
{
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DISPLAY, "sid.sun::note!\n");

	int ret = 0;
	__u32 ctrl_count;
	__u32 ctrl_num = sizeof(display_ctrl) / sizeof(struct v4l2_ctrl_config);
	struct mtk_pq_device *mtk_pq_dev = container_of(
		subdev_display, struct mtk_pq_device, subdev_display);

	v4l2_ctrl_handler_init(display_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++)
		v4l2_ctrl_new_custom(display_ctrl_handler,
		    &display_ctrl[ctrl_count], NULL);

	ret = display_ctrl_handler->error;
	if (ret) {
		v4l2_err(pq_dev, "failed to create display ctrl handler\n");
		goto exit;
	}
	subdev_display->ctrl_handler = display_ctrl_handler;

	v4l2_subdev_init(subdev_display, &display_sd_ops);
	subdev_display->internal_ops = &display_sd_internal_ops;
	strlcpy(subdev_display->name, "mtk-display",
	    sizeof(subdev_display->name));

	ret = v4l2_device_register_subdev(pq_dev, subdev_display);
	if (ret) {
		v4l2_err(pq_dev, "failed to register display subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(display_ctrl_handler);
	return ret;
}

void mtk_pq_unregister_display_subdev(struct v4l2_subdev *subdev_display)
{
	v4l2_ctrl_handler_free(subdev_display->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_display);
}

int mtk_pq_aisr_dbg_show(struct device *dev, char *buf)
{
	struct mtk_pq_platform_device *pqdev = NULL;
	__u8 u8MaxSize = MAX_SYSFS_SIZE;
	int count = 0;

	if ((dev == NULL) || (buf == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	pqdev = dev_get_drvdata(dev);

	count += snprintf(buf + count, u8MaxSize - count,
		"AISR_Support=%d\n", pqdev->pqcaps.u32AISR_Support);
	count += snprintf(buf + count, u8MaxSize - count,
		"AISR_Version=%d\n", pqdev->pqcaps.u32AISR_Version);

	return count;
}

int mtk_pq_aisr_dbg_store(struct device *dev, const char *buf)
{
	int ret = 0;
	int len = 0;
	__u32 dbg_en = 0;
	__u32 win_id = 0, enable = 0, x = 0, y = 0, width = 0, height = 0;
	__u32 scale = 1;
	enum AISR_DBG_TYPE dbg_mode = MTK_PQ_AISR_DBG_NONE;
	bool find = false;
	char *cmd = NULL;
	struct mtk_pq_device *pq_dev = NULL;
	struct mtk_pq_platform_device *pqdev = NULL;

	if ((buf == NULL) || (dev == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	pqdev = dev_get_drvdata(dev);
	if (!pqdev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pqdev is NULL!\n");
		return -EINVAL;
	}

	if (!pqdev->pqcaps.u32AISR_Support) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "AISR not support!\n");
		return 0;
	}

	cmd = vmalloc(sizeof(char) * 0x1000);
	if (cmd == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "vmalloc cmd fail!\n");
		return -EINVAL;
	}

	memset(cmd, 0, sizeof(char) * 0x1000);

	len = strlen(buf);

	snprintf(cmd, len + 1, "%s", buf);

	find = _mtk_pq_get_dbg_value_from_string(cmd, "dbg_en", len, &dbg_en);
	if (!find) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Cmdline format error, type should be set, please echo help!\n");
		ret = -EINVAL;
		goto exit;
	}

	find = _mtk_pq_get_dbg_value_from_string(cmd, "window", len, &win_id);
	if (!find) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Cmdline format error, type should be set, please echo help!\n");
		ret = -EINVAL;
		goto exit;
	}

	pq_dev = pqdev->pq_devs[win_id];

	if (dbg_en) {
		_mtk_pq_get_aisr_dbg_mode(cmd, len, &dbg_mode);

		switch (dbg_mode) {
		case MTK_PQ_AISR_DBG_ENABLE:
			find = _mtk_pq_get_dbg_value_from_string(cmd, "aisr_en", len, &enable);
			if (!find) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"get enable fail, dbg_mode=scaling, find = %d!\n", find);
				goto exit;
			}
			_gAisrdbg.bAISREn = enable;

			if (_gAisrdbg.bAISREn) {
				find = _mtk_pq_get_dbg_value_from_string(cmd, "scale", len, &scale);
				if (!find) {
					_gAisrdbg.u32Scale = 1;
					STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
						"get scale fail, dbg_mode=scaling, find = %d!\n",
							find);
					goto exit;
				}
				_gAisrdbg.u32Scale = scale;
			}
			break;
		case MTK_PQ_AISR_DBG_ACTIVE_WIN:
			find = _mtk_pq_get_dbg_value_from_string(cmd, "active_win", len, &enable);
			if (!find) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"get enable fail, dbg_mode=active, find = %d!\n", find);
				goto exit;
			}
			_gAisrdbg.bAISREn = TRUE;
			_gAisrdbg.aisrActiveWin.bEnable = enable;
			if (!enable)
				goto exit;

			find = _mtk_pq_get_dbg_value_from_string(cmd, "x", len, &x);
			if (!find) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"get x fail, dbg_mode=active, find = %d!\n", find);
				goto exit;
			}

			find = _mtk_pq_get_dbg_value_from_string(cmd, "y", len, &y);
			if (!find) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"get y fail, dbg_mode=active, find = %d!\n", find);
				goto exit;
			}

			find = _mtk_pq_get_dbg_value_from_string(cmd, "w", len, &width);
			if (!find) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"get w fail, dbg_mode=active, find = %d!\n", find);
				goto exit;
			}

			find = _mtk_pq_get_dbg_value_from_string(cmd, "h", len, &height);
			if (!find) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"get h fail, dbg_mode=active, find = %d!\n", find);
				goto exit;
			}

			_gAisrdbg.aisrActiveWin.x = x;
			_gAisrdbg.aisrActiveWin.y = y;
			_gAisrdbg.aisrActiveWin.width = width;
			_gAisrdbg.aisrActiveWin.height = height;
			break;
		default:
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"Not Support dbg mode=%d!\n", dbg_mode);
			ret = -EINVAL;
			goto exit;
		}
	}

	_gAisrdbg.bdbgModeEn = dbg_en;
	STI_PQ_LOG(STI_PQ_LOG_LEVEL_DBG, "dbg_en=%u dbg_mode=%d\n", dbg_en, dbg_mode);

exit:
	vfree(cmd);

	return ret;
}
