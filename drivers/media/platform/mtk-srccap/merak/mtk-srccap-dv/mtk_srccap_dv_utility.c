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

#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "mtk_srccap.h"
#include "mtk_srccap_dv.h"

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define DV_MAX_CMD_LENGTH (0xFF)
#define DV_MAX_ARG_NUM (64)
#define DV_CMD_REMOVE_LEN (2)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
int g_dv_debug_level;
int g_dv_pr_level;

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------
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

	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"----------------dv information start----------------\n");
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"SRCCAP_DV_VERSION: %u\n", SRCCAP_DV_VERSION);
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"SRCCAP_DV_COMMON_VERSION: %u\n", SRCCAP_DV_COMMON_VERSION);
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"SRCCAP_DV_DESCRB_VERSION: %u\n", SRCCAP_DV_DESCRB_VERSION);
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"SRCCAP_DV_DMA_VERSION: %u\n", SRCCAP_DV_DMA_VERSION);
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"SRCCAP_DV_META_VERSION: %u\n", SRCCAP_DV_META_VERSION);
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"----------------dv information end----------------\n\n");
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"----------------dv debug commands help start----------------\n");
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"set_debug_level=debug_level\n");
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR,
		"----------------dv debug commands help end----------------\n");

	return  ret;
}

static int dv_debug_set_debug_level(const char *args[], int arg_num)
{
	int ret = 0;
	int dv_debug_level = 0;

	if (arg_num < 1)
		goto exit;

	ret = kstrtoint(args[0], 0, &dv_debug_level);
	if (ret < 0)
		goto exit;

	g_dv_debug_level = dv_debug_level;

exit:
	return ret;
}

static int dv_debug_set_pr_level(const char *args[], int arg_num)
{
	int ret = 0;
	int pr_level = 0;

	if (arg_num < 1)
		goto exit;

	ret = kstrtoint(args[0], 0, &pr_level);
	if (ret < 0)
		goto exit;

	g_dv_pr_level = pr_level;

exit:
	return ret;
}

//-----------------------------------------------------------------------------
// Global Functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Debug Functions
//-----------------------------------------------------------------------------
int mtk_dv_debug_show(
	struct device *dev,
	const char *buf)
{
	int ssize = 0;

	if ((dev == NULL) || (buf == NULL))
		return 0;

	dv_debug_print_help();

	return ssize;
}

int mtk_dv_debug_store(
	struct device *dev,
	const char *buf)
{
	int ret = 0;
	char cmd[DV_MAX_CMD_LENGTH];
	char *args[DV_MAX_ARG_NUM] = {NULL};
	int arg_num;

	if ((dev == NULL) || (buf == NULL)) {
		SRCCAP_DV_LOG_CHECK_POINTER(-EINVAL);
		goto exit;
	}

	strncpy(cmd, buf, DV_MAX_CMD_LENGTH);
	cmd[DV_MAX_CMD_LENGTH - 1] = '\0';

	arg_num = dv_parse_cmd_helper(cmd, args, DV_MAX_ARG_NUM);
	SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR, "cmd: %s, num: %d.\n", cmd, arg_num);

	if (strncmp(cmd, "set_debug_level", DV_MAX_CMD_LENGTH) == 0) {
		ret = dv_debug_set_debug_level((const char **)&args[1], arg_num);
	} else if (strncmp(cmd, "set_pr_level", DV_MAX_CMD_LENGTH) == 0) {
		ret = dv_debug_set_pr_level((const char **)&args[1], arg_num);
	} else {
		dv_debug_print_help();
		goto exit;
	}

	if (ret < 0) {
		SRCCAP_DV_LOG_CHECK_RETURN(ret, ret);
		goto exit;
	}

exit:
	return ret;
}

int mtk_dv_debug_checksum(
	__u8 *data,
	__u32 size,
	__u32 *sum)
{
	int ret = 0;
	int i = 0, s = 0;

	if ((data == NULL) || (sum == NULL))
		return -EINVAL;

	for (i = 0; i < size; i++)
		s += *(data + i);

	*sum = s;

	return ret;
}
