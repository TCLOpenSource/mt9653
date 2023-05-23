// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include "mtk_pq.h"
#include "mtk_pq_pattern.h"
#include "hwreg_pq_display_pattern.h"

//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
#define ENTER()	(STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN, ">>>>>ENTER>>>>>\n"))
#define EXIT()		(STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN, "<<<<<EXIT <<<<<\n"))

#define PATTERN_NUM		(9)
#define SYSFS_MAX_BUF_COUNT	(0x1000)
#define DEFAULT_H_SIZE		(64)
#define DEFAULT_V_SIZE		(32)
//-----------------------------------------------------------------------------
//  Local Variables
//-----------------------------------------------------------------------------
static struct pq_pattern_size pattern_size[PQ_WIN_MAX_NUM][PQ_PATTERN_POSITION_MAX];
static unsigned short g_capability[PQ_PATTERN_POSITION_MAX];
static bool b_inited;
static char character[37][8] = {
	{0x00, 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},	//A
	{0x00, 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},	//B
	{0x00, 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},	//C
	{0x00, 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},	//D
	{0x00, 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},	//E
	{0x00, 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},	//F
	{0x00, 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},	//G
	{0x00, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},	//H
	{0x00, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},	//I
	{0x00, 0x0F, 0x01, 0x01, 0x01, 0x01, 0x11, 0x0E},	//J
	{0x00, 0x11, 0x11, 0x12, 0x1C, 0x12, 0x11, 0x11},	//K
	{0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},	//L
	{0x00, 0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11},	//M
	{0x00, 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},	//N
	{0x00, 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},	//O
	{0x00, 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},	//P
	{0x00, 0x0E, 0x11, 0x11, 0x11, 0x15, 0x13, 0x0F},	//Q
	{0x00, 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x11},	//R
	{0x00, 0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E},	//S
	{0x00, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},	//T
	{0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},	//U
	{0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},	//V
	{0x00, 0x11, 0x11, 0x11, 0x11, 0x15, 0x1B, 0x11},	//W
	{0x00, 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},	//X
	{0x00, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},	//Y
	{0x00, 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},	//Z
	{0x00, 0x0E, 0x11, 0x19, 0x15, 0x13, 0x11, 0x0E},	//zero
	{0x00, 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},	//one
	{0x00, 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},	//two
	{0x00, 0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},	//three
	{0x00, 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},	//four
	{0x00, 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E},	//five
	{0x00, 0x0E, 0x11, 0x10, 0x1E, 0x11, 0x11, 0x0E},	//six
	{0x00, 0x1F, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04},	//seven
	{0x00, 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},	//eight
	{0x00, 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x11, 0x0E},	//nine
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},	//space
};
char *position_str[PQ_PATTERN_POSITION_MAX] = {
	"  IP2   ", " NR_DNR ", "NR_IPMR ", " NR_OPM ",
	" NR_DI  ", "   DI   ", " SRS_IN ", " SRS_OUT",
	"  VOP   ", "IP2_POST", "SCIP_DV ", "SCDV_DV ",
	"B2R_DMA ", "B2R_LIT1", "B2R_PRE ", "B2R_POST"};

char *type_str[PATTERN_NUM] = {
	"      Pure Color      ", "         Ramp         ", "     Pure Colorbar    ",
	"   Gradient Colorbar  ", " Black-White Colorbar ", "         Cross        ",
	"      PIP Window      ", "      Dot Matrix      ", "      Crosshatch      "};

//-----------------------------------------------------------------------------
//  Local Functions
//-----------------------------------------------------------------------------
static int _mtk_pq_pattern_get_size(
	unsigned char win_id, enum pq_pattern_position position,
	unsigned short *h_size, unsigned short *v_size)
{
	int ret = 0;

	ENTER();

	if ((h_size == NULL) || (v_size == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if ((win_id >= PQ_WIN_MAX_NUM) || (position >= PQ_PATTERN_POSITION_MAX)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid win_id or position, win_id = %u, position = %u\n",
			win_id, position);
		*h_size = 0;
		*v_size = 0;
		return -EINVAL;
	}

	if (pattern_size[win_id][position].h_size == 0)
		*h_size = DEFAULT_H_SIZE;//fix pattern_size
	else
		*h_size = pattern_size[win_id][position].h_size;

	if (pattern_size[win_id][position].v_size == 0)
		*v_size = DEFAULT_V_SIZE;//fix pattern_size
	else
		*v_size = pattern_size[win_id][position].v_size;

	EXIT();
	return ret;
}

static bool _mtk_pq_pattern_get_value_from_string(char *buf, char *name,
	unsigned int len, int *value)
{
	bool find = false;
	char *string, *tmp_name, *cmd, *tmp_value;

	ENTER();

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
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN,
				"name = %s, value = %d\n", name, *value);
			return find;
		}
	}

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN,
		"name(%s) was not found!\n", name);

	EXIT();
	return find;
}

static int _mtk_pq_pattern_get_capability(
	struct pq_pattern_capability *pattern_cap)
{
	int ret = 0;
	int position = 0;
	struct hwreg_pq_pattern_capability hwreg_cap;

	ENTER();

	memset(&hwreg_cap, 0, sizeof(struct hwreg_pq_pattern_capability));

	if (pattern_cap == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (!b_inited) {
		for (position = 0; position < PQ_PATTERN_POSITION_MAX; position++) {
			hwreg_cap.position = position;
			ret = drv_hwreg_pq_pattern_get_capability(&hwreg_cap);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"drv_hwreg_pq_pattern_get_capability fail, ret = %d!\n",
					ret);
				return ret;
			}
			g_capability[position] = hwreg_cap.pattern_type;
		}
		b_inited = true;
	}

	for (position = 0; position < PQ_PATTERN_POSITION_MAX; position++) {

		pattern_cap->pattern_type[position] = g_capability[position];

		STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN,
			"pattern_type[%u] = 0x%x\n", position, pattern_cap->pattern_type[position]);
	}

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_common_info(
	char *buf, unsigned int len,
	struct mtk_pq_platform_device *pqdev,
	struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int window = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct mtk_pq_device *pq_dev = NULL;

	ENTER();

	if ((buf == NULL) || (pqdev == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	find = _mtk_pq_pattern_get_value_from_string(buf, "window", len, &value);
	if (!find) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Cmdline format error, window should be set, please echo help!\n");
		return -EINVAL;
	}

	window = value;

	if ((window < 0) || (window >=  pqdev->usable_win)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid window ID::%d!\n", window);
		return -EINVAL;
	}

	pq_dev = pqdev->pq_devs[window];
	if (!pq_dev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_dev is NULL!\n");
		return -EINVAL;
	}

	pattern_info->win_id = window;

	find = _mtk_pq_pattern_get_value_from_string(buf, "enable", len, &value);
	if (!find) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Cmdline format error, enable should be set, please echo help!\n");
		return -EINVAL;
	}

	pattern_info->enable = value;

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_pure_color(char *buf,
	unsigned int len, struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	struct pq_pattern_pure_color *pure_color = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	pure_color = &pattern_info->m.pure_color;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_PURE_COLOR;

	_mtk_pq_pattern_get_value_from_string(buf, "ts_enable", len, &value);
	pattern_info->enable_ts = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "r", len, &value);
	if (!find)
		pure_color->color.red = 1023;
	else
		pure_color->color.red = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "g", len, &value);
	if (!find)
		pure_color->color.green = 1023;
	else
		pure_color->color.green = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "b", len, &value);
	if (!find)
		pure_color->color.blue = 1023;
	else
		pure_color->color.blue = value;

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_ramp(char *buf,
	unsigned int len, struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	struct pq_pattern_ramp *ramp = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ramp = &pattern_info->m.ramp;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_RAMP;

	_mtk_pq_pattern_get_value_from_string(buf, "r_st", len, &value);
	ramp->start.red = value;

	_mtk_pq_pattern_get_value_from_string(buf, "g_st", len, &value);
	ramp->start.green = value;

	_mtk_pq_pattern_get_value_from_string(buf, "b_st", len, &value);
	ramp->start.blue = value;

	_mtk_pq_pattern_get_value_from_string(buf, "vertical", len, &value);
	ramp->b_vertical = value;

	_mtk_pq_pattern_get_value_from_string(buf, "ts_enable", len, &value);
	pattern_info->enable_ts = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "r_end", len, &value);
	if (!find)
		ramp->end.red = 1023;
	else
		ramp->end.red = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "g_end", len, &value);
	if (!find)
		ramp->end.green = 1023;
	else
		ramp->end.green = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "b_end", len, &value);
	if (!find)
		ramp->end.blue = 1023;
	else
		ramp->end.blue = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "level", len, &value);
	if (!find)
		ramp->diff_level = 4;
	else
		ramp->diff_level = value;

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_pure_colorbar(char *buf,
	unsigned int len, struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct pq_pattern_pure_colorbar *pure_colorbar = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_pq_pattern_get_size(pattern_info->win_id,
		pattern_info->position, &h_size, &v_size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_mtk_pq_pattern_get_size fail, ret = %d!\n", ret);
		return ret;
	}

	pure_colorbar = &pattern_info->m.pure_colorbar;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_PURE_COLORBAR;

	_mtk_pq_pattern_get_value_from_string(buf, "movable", len, &value);
	pure_colorbar->movable = value;

	_mtk_pq_pattern_get_value_from_string(buf, "speed", len, &value);
	pure_colorbar->shift_speed = value;

	_mtk_pq_pattern_get_value_from_string(buf, "ts_enable", len, &value);
	pattern_info->enable_ts = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "width", len, &value);
	if (!find)
		pure_colorbar->colorbar_h_size = DEFAULT_H_SIZE;
	else
		pure_colorbar->colorbar_h_size = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "height", len, &value);
	if (!find)
		pure_colorbar->colorbar_v_size = DEFAULT_V_SIZE;
	else
		pure_colorbar->colorbar_v_size = value;

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_gradient_colorbar(char *buf,
	unsigned int len, struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	struct pq_pattern_gradient_colorbar *gradient_colorbar = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	gradient_colorbar = &pattern_info->m.gradient_colorbar;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_GRADIENT_COLORBAR;

	_mtk_pq_pattern_get_value_from_string(buf, "r_st", len, &value);
	gradient_colorbar->start.red = value;

	_mtk_pq_pattern_get_value_from_string(buf, "g_st", len, &value);
	gradient_colorbar->start.green = value;

	_mtk_pq_pattern_get_value_from_string(buf, "b_st", len, &value);
	gradient_colorbar->start.blue = value;

	_mtk_pq_pattern_get_value_from_string(buf, "color_1", len, &value);
	gradient_colorbar->color_1st = value;

	_mtk_pq_pattern_get_value_from_string(buf, "vertical", len, &value);
	gradient_colorbar->b_vertical = value;

	_mtk_pq_pattern_get_value_from_string(buf, "ts_enable", len, &value);
	pattern_info->enable_ts = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "r_end", len, &value);
	if (!find)
		gradient_colorbar->end.red = 1023;
	else
		gradient_colorbar->end.red = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "g_end", len, &value);
	if (!find)
		gradient_colorbar->end.green = 1023;
	else
		gradient_colorbar->end.green = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "b_end", len, &value);
	if (!find)
		gradient_colorbar->end.blue = 1023;
	else
		gradient_colorbar->end.blue = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "color_2", len, &value);
	if (!find)
		gradient_colorbar->color_2nd = 1;
	else
		gradient_colorbar->color_2nd = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "color_3", len, &value);
	if (!find)
		gradient_colorbar->color_3rd = 2;
	else
		gradient_colorbar->color_3rd = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "color_4", len, &value);
	if (!find)
		gradient_colorbar->color_4th = 3;
	else
		gradient_colorbar->color_4th = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "level", len, &value);
	if (!find)
		gradient_colorbar->diff_level = 4;
	else
		gradient_colorbar->diff_level = value;

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_black_white_colorbar(char *buf,
	unsigned int len, struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct pq_pattern_black_white_colorbar *black_white_colorbar = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_pq_pattern_get_size(pattern_info->win_id,
		pattern_info->position, &h_size, &v_size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_mtk_pq_pattern_get_size fail, ret = %d!\n", ret);
		return ret;
	}

	black_white_colorbar = &pattern_info->m.black_white_colorbar;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_BLACK_WHITE_COLORBAR;

	_mtk_pq_pattern_get_value_from_string(buf, "r_st", len, &value);
	black_white_colorbar->first_color.red = value;

	_mtk_pq_pattern_get_value_from_string(buf, "g_st", len, &value);
	black_white_colorbar->first_color.green = value;

	_mtk_pq_pattern_get_value_from_string(buf, "b_st", len, &value);
	black_white_colorbar->first_color.blue = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "r_end", len, &value);
	if (!find)
		black_white_colorbar->second_color.red = 1023;
	else
		black_white_colorbar->second_color.red = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "g_end", len, &value);
	if (!find)
		black_white_colorbar->second_color.green = 1023;
	else
		black_white_colorbar->second_color.green = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "b_end", len, &value);
	if (!find)
		black_white_colorbar->second_color.blue = 1023;
	else
		black_white_colorbar->second_color.blue = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "width", len, &value);
	if (!find)
		black_white_colorbar->colorbar_h_size = (h_size >> 3);
	else
		black_white_colorbar->colorbar_h_size = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "height", len, &value);
	if (!find)
		black_white_colorbar->colorbar_v_size = v_size;
	else
		black_white_colorbar->colorbar_v_size = value;

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_cross(char *buf,
	unsigned int len, struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct pq_pattern_cross *cross = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_pq_pattern_get_size(pattern_info->win_id,
		pattern_info->position, &h_size, &v_size);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_mtk_pq_pattern_get_size fail, ret = %d!\n", ret);
		return ret;
	}

	cross = &pattern_info->m.cross;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_CROSS;

	_mtk_pq_pattern_get_value_from_string(buf, "g_st", len, &value);
	cross->cross_color.green = value;

	_mtk_pq_pattern_get_value_from_string(buf, "b_st", len, &value);
	cross->cross_color.blue = value;

	_mtk_pq_pattern_get_value_from_string(buf, "g_end", len, &value);
	cross->border_color.green = value;

	_mtk_pq_pattern_get_value_from_string(buf, "b_end", len, &value);
	cross->border_color.blue = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "r_st", len, &value);
	if (!find)
		cross->cross_color.red = 1023;
	else
		cross->cross_color.red = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "r_end", len, &value);
	if (!find)
		cross->border_color.red = 1023;
	else
		cross->border_color.red = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "position_h", len, &value);
	if (!find)
		cross->h_position = (h_size >> 1);
	else
		cross->h_position = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "position_v", len, &value);
	if (!find)
		cross->v_position = (v_size >> 1);
	else
		cross->v_position = value;

	EXIT();
	return ret;
}

static unsigned char _mtk_pq_pattern_swap_bits(unsigned char data)
{
	int index = 0;
	unsigned char target = 0;

	ENTER();

	for (index = 0; index < 8; index++)
		target |= ((data >> index) & 1) << (7 - index);

	EXIT();
	return target;
}

static int _mtk_pq_pattern_update_matrix(
	char s, unsigned char index, struct pq_pattern_dot_matrix *dot_matrix)
{
	int ret = 0;
	int count = 0;
	unsigned char addr = 0;
	unsigned char temp = 0;
	unsigned char table_index = 0;
	bool height = false;

	ENTER();

	if (dot_matrix == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (index >= MATRIX_MAX_CHAR) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid Index(%u), should smaller than %u!\n",
			index, MATRIX_MAX_CHAR);
		return -EINVAL;
	}

	addr = index / 2;
	height = index % 2;

	if ((s >= 'A') && (s <= 'Z')) {
		table_index = s - 'A';
	} else if ((s >= '0') && (s <= '9')) {
		table_index = s - '0' + 26;
	} else if (s == ' ') {
		table_index = 36;
	} else {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"String only support uppercase letter, number and space!\n");
		return -EINVAL;
	}

	for (count = 0; count < 8; count++) {
		temp = _mtk_pq_pattern_swap_bits(character[table_index][count]);

		if (height)
			dot_matrix->matrix_data[addr] |= (temp << 8);
		else
			dot_matrix->matrix_data[addr] |= temp;

		addr += (MATRIX_MAX_CHAR >> 1);
	}

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_matrix(const char *string,
	struct pq_pattern_dot_matrix *dot_matrix)
{
	int ret = 0;
	unsigned char index = 0;
	char s;
	char content[MATRIX_MAX_CHAR + 1];

	ENTER();

	memset(content, 0, sizeof(content));

	if ((string == NULL) || (dot_matrix == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	snprintf(content, sizeof(content), string);

	for (index = 0; index < MATRIX_MAX_CHAR; index++) {
		s = content[index];

		if (!s)
			break;

		ret = _mtk_pq_pattern_update_matrix(s, index, dot_matrix);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"_mtk_pq_pattern_update_matrix fail, ret = %d!\n", ret);
			return ret;
		}
	}

	EXIT();
	return ret;
}

static int _mtk_pq_pattern_fill_dot_matrix(char *buf,
	unsigned int len, struct pq_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	char string[MATRIX_MAX_CHAR + 1];
	struct pq_pattern_dot_matrix *dot_matrix = NULL;
	bool find = false;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	dot_matrix = &pattern_info->m.dot_matrix;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_DOT_MATRIX;

	_mtk_pq_pattern_get_value_from_string(buf, "ts_enable", len, &value);
	pattern_info->enable_ts = value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "h_st", len, &value);
	dot_matrix->h_pos = (!find) ? 0 : value;

	find = _mtk_pq_pattern_get_value_from_string(buf, "v_st", len, &value);
	dot_matrix->v_pos = (!find) ? 0 : value;

	scnprintf(string, MATRIX_MAX_CHAR + 1,
		"     THIS IS A TEST STRING      ");

	ret = _mtk_pq_pattern_fill_matrix(string, dot_matrix);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_mtk_pq_pattern_fill_matrix fail, ret = %d!\n", ret);
		return ret;
	}

	EXIT();
	return ret;
}

static ssize_t _mtk_pq_pattern_show_status(
	char *buf, ssize_t used_count, enum pq_pattern_position position,
	struct mtk_pq_platform_device *pqdev)
{
	int ret = 0;
	bool opened = false;
	ssize_t count = 0;
	ssize_t total_count = 0;
	unsigned char win_id = 0;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct mtk_pq_device *pq_dev = NULL;

	ENTER();

	if (buf == NULL || pqdev == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	if (position >= PQ_PATTERN_POSITION_MAX) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid position(%u)\n", position);
		return 0;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"PQ Pattern Status:\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" ---------------------------------------\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"| Window | Enable | Type |  Size(h, v)  |\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" ---------------------------------------\n");
	total_count += count;

	for (win_id = 0; win_id < pqdev->usable_win; win_id++) {
		pq_dev = pqdev->pq_devs[win_id];
		if (!pq_dev) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pq_dev is NULL!\n");
			continue;
		}

		if (pq_dev->stream_on_ref) {
			opened = true;
			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"|   %02u   |", win_id);
			total_count += count;
			if (position == pq_dev->pattern_status.pattern_enable_position) {
				count = scnprintf(buf + total_count + used_count,
					SYSFS_MAX_BUF_COUNT - total_count - used_count,
					"   ON   |");
				total_count += count;

				count = scnprintf(buf + total_count + used_count,
					SYSFS_MAX_BUF_COUNT - total_count - used_count,
					"   %u  |", pq_dev->pattern_status.pattern_type);
				total_count += count;
			} else {
				count = scnprintf(buf + total_count + used_count,
					SYSFS_MAX_BUF_COUNT - total_count - used_count,
					"   OFF  |");
				total_count += count;

				count = scnprintf(buf + total_count + used_count,
					SYSFS_MAX_BUF_COUNT - total_count - used_count,
					" NULL |");
				total_count += count;
			}

			ret = _mtk_pq_pattern_get_size(win_id, position, &h_size, &v_size);
			if (ret)
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Get PQ Pattern Size Fail!\n");

			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				" (%04u, %04u) |\n", h_size, v_size);
			total_count += count;

			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				" ----------------------------------------\n");
			total_count += count;
		}
	}

	if (!opened) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"No Window Opened!\n");
		total_count += count;
	}

	EXIT();
	return total_count;
}

static ssize_t _mtk_pq_pattern_show_capbility(
	char *buf, ssize_t used_count, enum pq_pattern_position position)
{
	int ret = 0;
	int index = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	struct pq_pattern_capability cap;

	ENTER();

	if (buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	if (position >= PQ_PATTERN_POSITION_MAX) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid position(%u)\n", position);
		return 0;
	}

	memset(&cap, 0, sizeof(struct pq_pattern_capability));

	ret = _mtk_pq_pattern_get_capability(&cap);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Get PQ Pattern Capability Fail!\n");
		return 0;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\nPattery Capability Table:\n");
	total_count += count;

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		" ---------------");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"-------");
		total_count += count;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\n| Position\\Type |");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"  %2u  |", index);
		total_count += count;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"\n ---------------");
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
		"\n ---------------");
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

static ssize_t _mtk_pq_pattern_show_help(
	char *buf, ssize_t used_count, unsigned int pattern_type)
{
	ssize_t count = 0;
	ssize_t total_count = 0;

	ENTER();

	if (buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
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

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"ts_enable: enable time sharing mode.[0, 1]\n");
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

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"ts_enable: enable time sharing mode.[0, 1]\n");
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

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"ts_enable: enable time sharing mode.[0, 1]\n");
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
			"h_st: h start position\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"v_st: v start position\n");
		total_count += count;

		count = scnprintf(buf + total_count + used_count,
			SYSFS_MAX_BUF_COUNT - total_count - used_count,
			"ts_enable: enable time sharing mode.[0, 1]\n");
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
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"Invalid pattern type = 0x%x!\n", pattern_type);
		break;
	}

	EXIT();
	return total_count;
}

static ssize_t _mtk_pq_pattern_show_mapping_table(
	char *buf, ssize_t used_count)
{
	int index = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;

	ENTER();

	if (buf == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
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

int mtk_pq_pattern_set_size_info(struct pq_pattern_size_info *size_info)
{
	int ret = 0;
	unsigned char win_id = 0;
	unsigned short position = 0;

	//ENTER();

	if (size_info == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	win_id = size_info->win_id;

	for (position = 0; position < PQ_PATTERN_POSITION_MAX; position++) {
		pattern_size[win_id][position].h_size = size_info->h_size[position];
		pattern_size[win_id][position].v_size = size_info->v_size[position];
		//STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN,
			//"win_id = %u, position = %u, h_size = %u, v_size = %u!\n",
			//size_info->win_id, position,
			//size_info->h_size[position], size_info->v_size[position]);
	}

	//EXIT();
	return ret;
}

ssize_t mtk_pq_pattern_show(char *buf,
	struct device *dev, enum pq_pattern_position position)
{
	int index;
	int ret = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	struct mtk_pq_platform_device *pqdev = NULL;
	struct pq_pattern_capability cap;

	ENTER();

	if ((buf == NULL) || (dev == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= PQ_PATTERN_POSITION_MAX) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid position(%u)\n", position);
		return -EINVAL;
	}

	memset(&cap, 0, sizeof(struct pq_pattern_capability));

	ret = _mtk_pq_pattern_get_capability(&cap);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Get PQ Pattern Capability Fail!\n");
		return -EINVAL;
	}

	pqdev = dev_get_drvdata(dev);
	if (!pqdev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pqdev is NULL!\n");
		return -EINVAL;
	}

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"Position: %s\n\n", position_str[position]);
	total_count += count;

	count = _mtk_pq_pattern_show_status(buf, total_count, position, pqdev);
	total_count += count;

	count = _mtk_pq_pattern_show_capbility(buf, total_count, position);
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"\nCommon Cmd:\n");
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"enable: echo window=0 enable=1 type=1 > pattern node\n");
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"disable: echo window=0 enable=0 > pattern node\n");
	total_count += count;

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"\nOptional Cmd:\n");
	total_count += count;

	for (index = 0; index < PATTERN_NUM; index++) {
		if (((cap.pattern_type[position] >> index) & 1)) {
			count = _mtk_pq_pattern_show_help(buf, total_count, BIT(index));
			total_count += count;
		}
	}

	count = _mtk_pq_pattern_show_mapping_table(buf, total_count);
	total_count += count;

	EXIT();

	STI_PQ_LOG(STI_PQ_LOG_LEVEL_PATTERN, "count = %d!\n", total_count);

	return total_count;
}

int mtk_pq_pattern_store(const char *buf,
	struct device *dev, enum pq_pattern_position position)
{
	int ret = 0;
	int len = 0;
	int type = 0;
	bool find = false;
	char *cmd = NULL;
	struct pq_pattern_info pattern_info;
	struct mtk_pq_device *pq_dev = NULL;
	struct mtk_pq_platform_device *pqdev = NULL;
	struct pq_pattern_capability cap;

	ENTER();

	if ((buf == NULL) || (dev == NULL)) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= PQ_PATTERN_POSITION_MAX) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "Invalid position(%u)\n", position);
		return -EINVAL;
	}

	pqdev = dev_get_drvdata(dev);
	if (!pqdev) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "pqdev is NULL!\n");
		return -EINVAL;
	}

	cmd = vmalloc(sizeof(char) * 0x1000);
	if (cmd == NULL) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR, "vmalloc cmd fail!\n");
		return -EINVAL;
	}

	memset(cmd, 0, sizeof(char) * 0x1000);
	memset(&pattern_info, 0, sizeof(struct pq_pattern_info));
	memset(&cap, 0, sizeof(struct pq_pattern_capability));

	len = strlen(buf);

	snprintf(cmd, len + 1, buf);

	pattern_info.position = position;

	ret = _mtk_pq_pattern_fill_common_info(cmd, len, pqdev, &pattern_info);
	if (ret) {
		STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
			"_mtk_pq_pattern_fill_common_info fail, ret = %d!\n", ret);
		goto exit;
	}

	pq_dev = pqdev->pq_devs[pattern_info.win_id];

	if (!pattern_info.enable) {
		pq_dev->pattern_status.pattern_enable_position = INVALID_PATTERN_POSITION;
	} else {
		ret = _mtk_pq_pattern_get_capability(&cap);
		if (ret) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Get PQ Pattern Capability Fail!\n");
			goto exit;
		}

		find = _mtk_pq_pattern_get_value_from_string(cmd, "type", len, &type);
		if (!find) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Cmdline format error, type should be set, please echo help!\n");
			ret = -EINVAL;
			goto exit;
		}

		if (!((cap.pattern_type[position] >> type) & 1)) {
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Pattern type not support, position = %d, type = %d!\n",
				position, type);
			ret = -EINVAL;
			goto exit;
		}

		switch (type) {
		case 0:
			ret = _mtk_pq_pattern_fill_pure_color(cmd, len, &pattern_info);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_pattern_fill_pure_color fail, ret = %d!\n", ret);
				goto exit;
			}

			break;
		case 1:
			ret = _mtk_pq_pattern_fill_ramp(cmd, len, &pattern_info);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_pattern_fill_ramp fail, ret = %d!\n", ret);
				goto exit;
			}

			break;
		case 2:
			ret = _mtk_pq_pattern_fill_pure_colorbar(cmd, len, &pattern_info);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_pattern_fill_pure_colorbar fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 3:
			ret = _mtk_pq_pattern_fill_gradient_colorbar(cmd, len, &pattern_info);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_pattern_fill_gradient_colorbar fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 4:
			ret = _mtk_pq_pattern_fill_black_white_colorbar(cmd, len, &pattern_info);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_pattern_fill_black_white_colorbar fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 5:
			ret = _mtk_pq_pattern_fill_cross(cmd, len, &pattern_info);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_pattern_fill_cross fail, ret = %d!\n", ret);
				goto exit;
			}

			break;
		case 7:
			ret = _mtk_pq_pattern_fill_dot_matrix(cmd, len, &pattern_info);
			if (ret) {
				STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
					"_mtk_pq_pattern_fill_dot_matrix fail, ret = %d!\n", ret);
				goto exit;
			}

			break;
		default:
			STI_PQ_LOG(STI_PQ_LOG_LEVEL_ERROR,
				"Pattern Type::%d Not Support!\n", type);
			ret = -EINVAL;
			goto exit;
		}

		pq_dev->pattern_status.pattern_enable_position = position;
		pq_dev->pattern_status.pattern_type = type;
	}

	memcpy(&pq_dev->pattern_info, &pattern_info, sizeof(struct pq_pattern_info));
	pq_dev->trigger_pattern = true;

exit:
	vfree(cmd);

	if (ret == -ENOSR)
		ret = 0;

	EXIT();
	return ret;
}


