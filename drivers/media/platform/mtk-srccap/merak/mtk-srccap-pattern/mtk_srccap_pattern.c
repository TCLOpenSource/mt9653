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
#include <stdbool.h>
//#include <linux/vmalloc.h>

#include "mtk_srccap.h"
#include "mtk_srccap_pattern.h"
#include "hwreg_srccap_pattern.h"

//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
#define PATTERN_LOG_ERROR		BIT(0)
#define PATTERN_LOG_INFO		BIT(1)
#define PATTERN_LOG_FLOW		BIT(2)

static unsigned char debug = 7;
#define PATTERN_LOG(_level, _fmt, _args...) { \
	if ((_level & debug) != 0) { \
		if (_level & PATTERN_LOG_ERROR) \
			pr_err("[PATTERN ERROR]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if (_level & PATTERN_LOG_INFO) { \
			pr_err("[PATTERN INFO]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
		else if (_level & PATTERN_LOG_FLOW) { \
			pr_err("[PATTERN FLOW]  [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		} \
	} \
}

#define ENTER()		(PATTERN_LOG(PATTERN_LOG_FLOW, ">>>>>ENTER>>>>>\n"))
#define EXIT()		(PATTERN_LOG(PATTERN_LOG_FLOW, "<<<<<EXIT <<<<<\n"))

#define PATTERN_NUM		(9)
#define PATTERN_REG_NUM_MAX		(20)
#define SYSFS_MAX_BUF_COUNT	(0x1000)
#define INVALID_PATTERN_POSITION				(0xFF)
#define PATTERN_TEST

//-----------------------------------------------------------------------------
//  Local Variables
//-----------------------------------------------------------------------------
static struct srccap_pattern_env pattern_env[SRCCAP_PATTERN_POSITION_MAX];
static unsigned short g_capability[SRCCAP_PATTERN_POSITION_MAX];
static bool b_inited;
char *position_str[SRCCAP_PATTERN_POSITION_MAX] = {
	"      IP1     ", "  PRE_IP2_0   ", "  PRE_IP2_1   ",
	"PRE_IP2_0_POST", "PRE_IP2_1_POST"};

char *type_str[PATTERN_NUM] = {
	"      Pure Color      ", "         Ramp         ", "     Pure Colorbar    ",
	"   Gradient Colorbar  ", " Black-White Colorbar ", "         Cross        ",
	"      PIP Window      ", "      Dot Matrix      ", "      Crosshatch      "};

//-----------------------------------------------------------------------------
//  Local Functions
//-----------------------------------------------------------------------------
static int _mtk_srccap_pattern_get_size(enum srccap_pattern_position position,
	unsigned short *h_size, unsigned short *v_size)
{
	int ret = 0;

	ENTER();

	if ((h_size == NULL) || (v_size == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= SRCCAP_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position, position = %u\n", position);
		return -EINVAL;
	}

	*h_size = pattern_env[position].h_size;
	*v_size = pattern_env[position].v_size;

	EXIT();
	return ret;
}

static int _mtk_srccap_pattern_get_capability(
	struct srccap_pattern_capability *pattern_cap)
{
	int ret = 0;
	int position = 0;
	struct hwreg_srccap_pattern_capability hwreg_cap;

	ENTER();

	memset(&hwreg_cap, 0, sizeof(struct hwreg_srccap_pattern_capability));

	if (pattern_cap == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (!b_inited) {
		for (position = 0; position < (int)SRCCAP_PATTERN_POSITION_MAX; position++) {
			hwreg_cap.position = (enum hwreg_srccap_pattern_position)position;
			ret = drv_hwreg_srccap_pattern_get_capability(&hwreg_cap);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"drv_hwreg_srccap_pattern_get_capability fail, ret = %d!\n",
					ret);
				return ret;
			}

			g_capability[position] = hwreg_cap.pattern_type;

			PATTERN_LOG(PATTERN_LOG_INFO,
				"g_capability[%u] = 0x%x\n", position, g_capability[position]);
		}

		b_inited = true;
	}

	for (position = 0; position < SRCCAP_PATTERN_POSITION_MAX; position++) {

		pattern_cap->pattern_type[position] = g_capability[position];

		PATTERN_LOG(PATTERN_LOG_INFO,
			"pattern_type[%u] = 0x%x\n",
			position, pattern_cap->pattern_type[position]);
	}

	EXIT();
	return ret;
}

static bool _mtk_srccap_pattern_get_value_from_string(char *buf, char *name,
	unsigned int len, int *value)
{
	bool find = false;
	char *string, *tmp_name, *cmd, *tmp_value;

	ENTER();

	if ((buf == NULL) || (name == NULL) || (value == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
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
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"kstrtoint fail!\n");
				return find;
			}
			find = true;
			PATTERN_LOG(PATTERN_LOG_INFO,
				"name = %s, value = %d\n", name, *value);
			return find;
		}
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"name(%s) was not found!\n", name);

	EXIT();
	return find;
}

static int _mtk_srccap_pattern_fill_common_info(
	char *buf, unsigned int len,
	struct srccap_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	find = _mtk_srccap_pattern_get_value_from_string(buf, "enable", len, &value);
	if (!find) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Cmdline format error, enable should be set, please echo help!\n");
		return -EINVAL;
	}

	pattern_info->enable = value;

	EXIT();
	return ret;
}

static int _mtk_srccap_pattern_fill_pure_colorbar(char *buf,
	unsigned int len, struct srccap_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct srccap_pattern_pure_colorbar *pure_colorbar = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_srccap_pattern_get_size(pattern_info->position, &h_size, &v_size);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_srccap_pattern_get_size fail, ret = %d!\n", ret);
		return ret;
	}

	pure_colorbar = &pattern_info->m.pure_colorbar;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_PURE_COLORBAR;

	_mtk_srccap_pattern_get_value_from_string(buf, "movable", len, &value);
	pure_colorbar->movable = value;

	_mtk_srccap_pattern_get_value_from_string(buf, "speed", len, &value);
	pure_colorbar->shift_speed = value;

	find = _mtk_srccap_pattern_get_value_from_string(buf, "width", len, &value);
	if (!find)
		pure_colorbar->colorbar_h_size = (h_size >> 3);
	else
		pure_colorbar->colorbar_h_size = value;

	find = _mtk_srccap_pattern_get_value_from_string(buf, "height", len, &value);
	if (!find)
		pure_colorbar->colorbar_v_size = v_size;
	else
		pure_colorbar->colorbar_v_size = value;

	EXIT();
	return ret;
}

static int _mtk_srccap_pattern_set_pure_colorbar(bool riu,
	struct srccap_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;
	enum srccap_pattern_position position;
	struct hwreg_srccap_pattern_pure_colorbar data;
	struct srccap_pattern_pure_colorbar *pure_colorbar = NULL;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	memset(&data, 0, sizeof(struct hwreg_srccap_pattern_pure_colorbar));
	position = pattern_info->position;

	if (position >= SRCCAP_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position,position = %u\n", position);
		return -EINVAL;
	}

	pure_colorbar = &(pattern_info->m.pure_colorbar);
	data.common.position = (enum hwreg_srccap_pattern_position)position;
	data.common.enable = true;
	data.common.color_space = pattern_env[position].color_space;
	data.diff_h = pure_colorbar->colorbar_h_size;
	data.diff_v = pure_colorbar->colorbar_v_size;
	data.movable = pure_colorbar->movable;

	if (pure_colorbar->movable)
		data.shift_speed = pure_colorbar->shift_speed;

	ret = drv_hwreg_srccap_pattern_set_pure_colorbar(riu, &data, pattern_reg);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"drv_hwreg_srccap_pattern_set_pure_colorbar fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static ssize_t _mtk_srccap_pattern_show_status(
	char *buf, ssize_t used_count, enum srccap_pattern_position position,
	struct mtk_srccap_dev *srccap_dev)
{
	int ret = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	unsigned short h_size = 0;
	unsigned short v_size = 0;

	ENTER();

	if (buf == NULL || srccap_dev == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	if (position >= SRCCAP_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return 0;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"SRCCAP Pattern Status:\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" ------------------------------\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"| Enable | Type |  Size(h, v)  |\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" ------------------------------\n");
	total_count += count;

	if (srccap_dev->streaming) {
		if (position == srccap_dev->pattern_status.pattern_enable_position) {
			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"|   ON   |");
			total_count += count;

			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"   %u  |", srccap_dev->pattern_status.pattern_type);
			total_count += count;
		} else {
			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"|   OFF  |");
			total_count += count;

			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				" NULL |");
			total_count += count;
		}

		ret = _mtk_srccap_pattern_get_size(position, &h_size, &v_size);
		if (ret)
			PATTERN_LOG(PATTERN_LOG_ERROR, "Get SRCCAP Pattern Size Fail!\n");

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			" (%04u, %04u) |\n", h_size, v_size);
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			" ------------------------------\n");
		total_count += count;
	} else {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"Window Is Not Opened!\n");
		total_count += count;
	}

	EXIT();
	return total_count;
}

static ssize_t _mtk_srccap_pattern_show_capbility(
	char *buf, ssize_t used_count, enum srccap_pattern_position position)
{
	int ret = 0;
	int index = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	struct srccap_pattern_capability cap;

	ENTER();

	if (buf == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	if (position >= SRCCAP_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return 0;
	}

	memset(&cap, 0, sizeof(struct srccap_pattern_capability));

	ret = _mtk_srccap_pattern_get_capability(&cap);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Get SRCCAP Pattern Capability Fail!\n");
		return 0;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\nPattery Capability Table:\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" ---------------------");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"-------");
		total_count += count;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\n|    Position\\Type    |");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"  %2u  |", index);
		total_count += count;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\n ---------------------");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"-------");
		total_count += count;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\n|    %s   |", position_str[position]);
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		if (((cap.pattern_type[position] >> index) & 1)) {
			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"   O  |");
			total_count += count;
		} else {
			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"   X  |");
			total_count += count;
		}
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\n ---------------------");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"-------");
		total_count += count;
	}

	EXIT();
	return total_count;
}

static ssize_t _mtk_srccap_pattern_show_help(
	char *buf, ssize_t used_count, unsigned int pattern_type)
{
	ssize_t count = 0;
	ssize_t total_count = 0;

	ENTER();

	if (buf == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	switch (pattern_type) {
	case MTK_PATTERN_FLAG_PURE_COLOR:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nPure Color Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r: red color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g: green color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b: blue color value(10 bit)\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_RAMP:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nRame Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_st: red start color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_st: green start color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_st: blue start color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_end: red end color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_end: green end color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_end: blue end color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"vertical: ramp is vertical or not.[0, 1]\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"level: color granularity, should be the nth power of 2\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_PURE_COLORBAR:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nPure Colorbar Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"movable: colorbar is movable or not.[0, 1]\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"speed: move a pixel after x frame\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"width: colorbar width\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"height: colorbar height\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_GRADIENT_COLORBAR:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nGradient Colorbar Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_st: red start color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_st: green start color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_st: blue start color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_end: red end color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_end: green end color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_end: blue end color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"vertical: ramp is vertical or not.[0, 1]\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"level: color granularity, should be the nth power of 2\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"color_1: color of colorbar 1.[0, 3]\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"color_2: color of colorbar 2.[0, 3]\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"color_3: color of colorbar 3.[0, 3]\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"color_4: color of colorbar 4.[0, 3]\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_BLACK_WHITE_COLORBAR:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nBlack White Colorbar Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_st: red first color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_st: green first color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_st: blue first color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_end: red second color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_end: green second color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_end: blue second color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"width: colorbar width\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"height: colorbar height\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_CROSS:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nCross Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_st: red cross color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_st: green cross color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_st: blue cross color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_end: red border color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_end: green border color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_end: blue border color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"position_h: corss center h position\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"position_v: corss center v position\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_PIP_WINDOW:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nPIP Window Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_st: red PIP color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_st: green PIP color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_st: blue PIP color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_end: red background color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_end: green background color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_end: blue background color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"x: PIP window h start position\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"y: PIP window v start position\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"width: PIP window width.\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"height: PIP window height\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_DOT_MATRIX:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nDot Matrix Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"No parameter need to set\n");
		total_count += count;
		break;
	case MTK_PATTERN_FLAG_CROSSHATCH:
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"\nCrosshatch Cmd:\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_st: red line color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_st: green line color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_st: blue line color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"r_end: red background color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"g_end: green background color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"b_end: blue background color value(10 bit)\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"h_line_interval: h line interval\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"v_line_interval: v line interval\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"h_line_width: h line width\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"v_line_width: v line width\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"h_position_st: h start position\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"v_position_st: v start position\n");
		total_count += count;
		break;
	default:
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid pattern type = 0x%x!\n", pattern_type);
		break;
	}

	EXIT();
	return total_count;
}

static ssize_t _mtk_srccap_pattern_show_mapping_table(
	char *buf, ssize_t used_count)
{
	int index = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;

	ENTER();

	if (buf == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\n\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"Pattern Type Mapping Table:\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" -----------------------------\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"| Index|     Pattern Type     |\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" -----------------------------\n");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"|  %2u  |%s|\n", index, type_str[index]);
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			" -----------------------------\n");
		total_count += count;
	}

	EXIT();
	return total_count;
}

static int _mtk_srccap_pattern_set_info(
	struct srccap_pattern_info *pattern_info)
{
	int ret = 0;
	bool riu = true;
	struct hwregOut pattern_reg;
	enum srccap_pattern_position position;

	ENTER();

	memset(&pattern_reg, 0, sizeof(struct hwregOut));

	if (pattern_info == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO, "riu : %s!\n", riu ? "ON" : "OFF");

	pattern_reg.reg = vmalloc(sizeof(struct reg_info) * PATTERN_REG_NUM_MAX);
	if (pattern_reg.reg == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "vmalloc pattern reg fail!\n");
		return -EINVAL;
	}

	for (position = SRCCAP_PATTERN_POSITION_IP1;
		position < SRCCAP_PATTERN_POSITION_MAX; position++) {
		ret = drv_hwreg_srccap_pattern_set_off(riu,
					(enum hwreg_srccap_pattern_position)position, &pattern_reg);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"drv_hwreg_srccap_pattern_set_off fail, position = %u, ret = %d!\n",
				position, ret);
			goto exit;
		}

		pattern_reg.regCount = 0;
	}

	if (pattern_info->enable) {
		if (pattern_info->position >= SRCCAP_PATTERN_POSITION_MAX) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Invalid position, position = %u\n", pattern_info->position);
			ret = -EINVAL;
			goto exit;
		}

		if (!(pattern_info->pattern_type & g_capability[pattern_info->position])) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"pattern type not support, please check it!\n");
			ret = -EINVAL;
			goto exit;
		}

		switch (pattern_info->pattern_type) {
		case MTK_PATTERN_FLAG_PURE_COLORBAR:
			ret = _mtk_srccap_pattern_set_pure_colorbar(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_srccap_pattern_set_pure_colorbar fail, ret = %d!\n",
					ret);
				goto exit;
			}
			break;
		default:
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Pattern type not support, pattern_type = 0x%x!\n",
				pattern_info->pattern_type);
			ret = -EINVAL;
			goto exit;
		}
	} else
		PATTERN_LOG(PATTERN_LOG_INFO, "Set Pattern OFF\n");

exit:
	vfree(pattern_reg.reg);

	EXIT();
	return ret;
}

int mtk_srccap_pattern_set_env(
	enum srccap_pattern_position position,
	struct srccap_pattern_env *env)
{
	int ret = 0;

	ENTER();

	if (env == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= SRCCAP_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position, position = %u\n", position);
		return -EINVAL;
	}

	pattern_env[position].h_size = env->h_size;
	pattern_env[position].v_size = env->v_size;
	pattern_env[position].color_space = env->color_space;

	PATTERN_LOG(PATTERN_LOG_INFO, "h = 0x%x, v = 0x%x, color space = %u\n",
		env->h_size, env->v_size, env->color_space);

	EXIT();
	return ret;
}

ssize_t mtk_srccap_pattern_show(char *buf,
	struct device *dev, enum srccap_pattern_position position)
{
	int index;
	int ret = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	struct srccap_pattern_capability cap;
	struct mtk_srccap_dev *srccap_dev = NULL;

	ENTER();

	if ((buf == NULL) || (dev == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= SRCCAP_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return -EINVAL;
	}

	memset(&cap, 0, sizeof(struct srccap_pattern_capability));

#ifdef PATTERN_TEST
	pattern_env[position].h_size = 3840;
	pattern_env[position].v_size = 2160;
#endif

	srccap_dev = dev_get_drvdata(dev);
	if (!srccap_dev) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "srccap_dev is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_srccap_pattern_get_capability(&cap);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Get SRCCAP Pattern Capability Fail!\n");
		return -EINVAL;
	}

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"Position: %s\n\n", position_str[position]);
	total_count += count;

	count = _mtk_srccap_pattern_show_status(buf, total_count, position, srccap_dev);
	total_count += count;

	count = _mtk_srccap_pattern_show_capbility(buf, total_count, position);
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"\nCommon Cmd:\n");
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"enable: echo enable=1 type=1 > pattern node\n");
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"disable: echo enable=0 > pattern node\n");
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"\nOptional Cmd:\n");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		if (((cap.pattern_type[position] >> index) & 1)) {
			count = _mtk_srccap_pattern_show_help(buf, total_count, BIT(index));
			total_count += count;
		}
	}

	count = _mtk_srccap_pattern_show_mapping_table(buf, total_count);
	total_count += count;

	EXIT();

	PATTERN_LOG(PATTERN_LOG_INFO, "count = %d!\n", total_count);

	return total_count;
}

int mtk_srccap_pattern_store(const char *buf,
	struct device *dev, enum srccap_pattern_position position)
{
	int ret = 0;
	int len = 0;
	int type = 0;
	bool find = false;
	char *cmd = NULL;
	struct srccap_pattern_info pattern_info;
	struct mtk_srccap_dev *srccap_dev = NULL;
	struct srccap_pattern_capability cap;

	ENTER();

	if ((buf == NULL) || (dev == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= SRCCAP_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return -EINVAL;
	}

	srccap_dev = dev_get_drvdata(dev);
	if (!srccap_dev) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "srccap_dev is NULL!\n");
		return -EINVAL;
	}

	cmd = vmalloc(sizeof(char) * 0x1000);
	if (cmd == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "vmalloc cmd fail!\n");
		return -EINVAL;
	}

	memset(cmd, 0, sizeof(char) * 0x1000);
	memset(&pattern_info, 0, sizeof(struct srccap_pattern_info));
	memset(&cap, 0, sizeof(struct srccap_pattern_capability));

#ifdef PATTERN_TEST
	pattern_env[position].h_size = 3840;
	pattern_env[position].v_size = 2160;
#endif

	len = strlen(buf);

	snprintf(cmd, len + 1, buf);

	ret = _mtk_srccap_pattern_fill_common_info(cmd, len, &pattern_info);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_srccap_pattern_fill_common_info fail, ret = %d!\n", ret);
		goto exit;
	}

	if (!pattern_info.enable) {
		srccap_dev->pattern_status.pattern_enable_position = INVALID_PATTERN_POSITION;
	} else {
		pattern_info.position = position;

		ret = _mtk_srccap_pattern_get_capability(&cap);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR, "Get SRCCAP Pattern Capability Fail!\n");
			goto exit;
		}

		find = _mtk_srccap_pattern_get_value_from_string(cmd, "type", len, &type);
		if (!find) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Cmdline format error, type should be set, please echo help!\n");
			ret = -EINVAL;
			goto exit;
		}

		if (!((cap.pattern_type[position] >> type) & 1)) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Pattern type not support, position = %d, type = %d!\n",
				position, type);
			ret = -EINVAL;
			goto exit;
		}

		switch (type) {
		case 2:
			ret = _mtk_srccap_pattern_fill_pure_colorbar(cmd, len, &pattern_info);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_srccap_pattern_fill_pure_colorbar fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		default:
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Pattern Type::%d Not Support!\n", type);
			ret = -EINVAL;
			goto exit;
		}

		srccap_dev->pattern_status.pattern_enable_position = position;
		srccap_dev->pattern_status.pattern_type = type;
	}

	ret = _mtk_srccap_pattern_set_info(&pattern_info);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_srccap_pattern_set_info fail, ret = %d!\n", ret);
		goto exit;
	}

exit:
	vfree(cmd);

	EXIT();
	return ret;
}

