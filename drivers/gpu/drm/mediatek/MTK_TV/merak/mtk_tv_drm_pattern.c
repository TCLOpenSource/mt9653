// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <drm/mtk_tv_drm.h>
#include "mtk_tv_drm_pattern.h"
#include "mtk_tv_drm_kms.h"
#include "hwreg_render_pattern.h"
#include "mtk_tv_drm_log.h"

//-----------------------------------------------------------------------------
//  Local Defines
//-----------------------------------------------------------------------------
#define MTK_DRM_MODEL			MTK_DRM_MODEL_PATTERN
#define PATTERN_LOG_ERROR		BIT(0)
#define PATTERN_LOG_INFO		BIT(1)
#define PATTERN_LOG_FLOW		BIT(2)

#define PATTERN_LOG(_level, _fmt, _args...) { \
	if (_level & PATTERN_LOG_ERROR) \
		DRM_ERROR("[PATTERN ERROR]  [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	else if (_level & PATTERN_LOG_INFO) { \
		DRM_INFO("[PATTERN INFO]  [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	} \
	else if (_level & PATTERN_LOG_FLOW) { \
		DRM_WARN("[PATTERN FLOW]  [%s,%5d]"_fmt, \
		__func__, __LINE__, ##_args); \
	} \
}

#define ENTER()			PATTERN_LOG(PATTERN_LOG_INFO, ">>>>>ENTER>>>>>\n")
#define EXIT()			PATTERN_LOG(PATTERN_LOG_INFO, "<<<<<EXIT <<<<<\n")
#define LOG(fmt, args...)	PATTERN_LOG(PATTERN_LOG_INFO, fmt, ##args)
#define ERR(fmt, args...)	PATTERN_LOG(PATTERN_LOG_ERROR, fmt, ##args)

#define PATTERN_NUM		(11)
#define PATTERN_REG_NUM_MAX		(40)
#define SYSFS_MAX_BUF_COUNT	(0x1000)
#define INVALID_PATTERN_POSITION				(0xFF)
#define PATTERN_TEST

#define OPBLEND(i) \
		((i >= DRM_PATTERN_POSITION_MULTIWIN) && \
			(i <= DRM_PATTERN_POSITION_GFX)) \


//-----------------------------------------------------------------------------
//  Local Variables
//-----------------------------------------------------------------------------
static struct drm_pattern_env pattern_env[DRM_PATTERN_POSITION_MAX];
static unsigned short g_capability[DRM_PATTERN_POSITION_MAX];
static bool b_inited;
char *position_str[DRM_PATTERN_POSITION_MAX] = {
	" MultiWin ", "   TCon   ", "   GFX    ", " OP_DRAM  ", " IP_BLEND "};

char *type_str[PATTERN_NUM] = {
	"      Pure Color      ", "         Ramp         ", "     Pure Colorbar    ",
	"   Gradient Colorbar  ", " Black-White Colorbar ", "         Cross        ",
	"      PIP Window      ", "      Dot Matrix      ", "      Crosshatch      ",
	"        Random        ", "      Chessboard      "};

//-----------------------------------------------------------------------------
//  Local Functions
//-----------------------------------------------------------------------------
static int _mtk_drm_pattern_get_size(
	enum drm_pattern_position position,
	unsigned short *h_size, unsigned short *v_size)
{
	int ret = 0;

	ENTER();

	if ((h_size == NULL) || (v_size == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position, position = %u\n", position);
		return -EINVAL;
	}

	*h_size = pattern_env[position].h_size;
	*v_size = pattern_env[position].v_size;

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_get_capability(struct drm_pattern_capability *pattern_cap)
{
	int ret = 0;
	int position = 0;
	struct hwreg_render_pattern_capability hwreg_cap;

	ENTER();

	memset(&hwreg_cap, 0, sizeof(struct hwreg_render_pattern_capability));

	if (pattern_cap == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (!b_inited) {
		for (position = 0; position < DRM_PATTERN_POSITION_MAX; position++) {
			hwreg_cap.position = position;
			ret = drv_hwreg_render_pattern_get_capability(&hwreg_cap);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"drv_hwreg_render_pattern_get_capability fail, ret = %d!\n",
					ret);
				return ret;
			}

			g_capability[position] = hwreg_cap.pattern_type;

			PATTERN_LOG(PATTERN_LOG_INFO,
				"g_capability[%u] = 0x%x\n", position, g_capability[position]);
		}

		b_inited = true;
	}

	for (position = 0; position < DRM_PATTERN_POSITION_MAX; position++) {

		pattern_cap->pattern_type[position] = g_capability[position];

		PATTERN_LOG(PATTERN_LOG_INFO,
			"pattern_type[%u] = 0x%x\n", position, pattern_cap->pattern_type[position]);
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_check_color_range(
	struct drm_pattern_color *color)
{
	int ret = 0;

	ENTER();

	if (color == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if ((color->red & 0xFC00)
		|| (color->green & 0xFC00)
		|| (color->blue & 0xFC00)) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid RGB value(0x%x, 0x%x, 0x%x), should be within 10 bits!\n",
			color->red, color->green, color->blue);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_check_color_value(
	struct drm_pattern_color *color)
{
	int ret = 0;
	unsigned short max = 0;

	ENTER();

	if (color == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	max = max_t(unsigned short, color->red, max_t(unsigned short, color->green, color->blue));

	if ((color->red != 0) && (color->red != max)) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid RGB value(0x%x, 0x%x, 0x%x), the start value should be same or 0, and also the end value!\n",
			color->red, color->green, color->blue);
		return -EINVAL;
	}

	if ((color->green != 0) && (color->green != max)) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid RGB value(0x%x, 0x%x, 0x%x), the start value should be same or 0, and also the end value!\n",
			color->red, color->green, color->blue);
		return -EINVAL;
	}

	if ((color->blue != 0) && (color->blue != max)) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid RGB value(0x%x, 0x%x, 0x%x), the start value should be same or 0, and also the end value!\n",
			color->red, color->green, color->blue);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_check_gradient_color(
	struct drm_pattern_color *start, struct drm_pattern_color *end)
{
	int ret = 0;

	ENTER();

	if ((start == NULL) || (end == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_check_color_range(start);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_range fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_check_color_range(end);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_range fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_check_color_value(start);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_value fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_check_color_value(end);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_value fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	if (!(((start->red >= end->red)
		&& (start->green >= end->green)
		&& (start->blue >= end->blue))
		|| ((start->red <= end->red)
		&& (start->green <= end->green)
		&& (start->blue <= end->blue)))) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid RGB value(0x%x, 0x%x, 0x%x), (0x%x, 0x%x, 0x%x), the trend should be consistent!\n",
			start->red, start->green, start->blue,
			end->red, end->green, end->blue);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_check_level(
	enum drm_pattern_position position,
	unsigned short diff_level, bool b_vertical)
{
	int ret = 0;
	unsigned char count = 0;
	unsigned short temp = 0;
	unsigned short level = diff_level;
	unsigned short h_size = 0;
	unsigned short v_size = 0;

	ENTER();

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position, position = %u\n", position);
		return -EINVAL;
	}

	h_size = pattern_env[position].h_size;
	v_size = pattern_env[position].v_size;

	if (!(h_size & v_size)) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid h/v size, h = 0x%x, v = 0x%x\n", h_size, v_size);
		return -EINVAL;
	}

	if (diff_level < MIN_DIFF_LEVEL) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Diff level(0x%x) should be greater than 0x%x\n",
			diff_level, MIN_DIFF_LEVEL);
		return -EINVAL;
	}

	if (b_vertical) {
		if (diff_level > v_size) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Invalid level, level mast be smaller than v size, level = 0x%x, v = 0x%x\n",
				diff_level, v_size);
			return -EINVAL;
		}
	} else {
		if (diff_level > h_size) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Invalid level, level mast be smaller than h size, level = 0x%x, h = 0x%x\n",
				diff_level, h_size);
			return -EINVAL;
		}
	}

	for (count = 0; count < 16; count++) {
		temp = level & 0x1;

		if (temp) {
			if (count < 2) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"Diff level(0x%x) should be the nth power of 2\n",
					diff_level);
				return -EINVAL;
			}

			level = level >> 1;

			if (level) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"Diff level(0x%x) should be the nth power of 2\n",
					diff_level);
				return -EINVAL;
			}
			break;
		}
		level = level >> 1;
	}

	EXIT();
	return ret;
}

static bool _mtk_drm_pattern_get_value_from_string(char *buf, char *name,
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

static int _mtk_drm_pattern_fill_common_info(
	char *buf, unsigned int len,
	struct drm_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	find = _mtk_drm_pattern_get_value_from_string(buf, "enable", len, &value);
	if (!find) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Cmdline format error, enable should be set, please echo help!\n");
		return -EINVAL;
	}

	pattern_info->enable = value;

	if (pattern_info->enable) {
		ret = _mtk_drm_pattern_get_size(pattern_info->position, &h_size, &v_size);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"_mtk_drm_pattern_get_size fail, ret = %d!\n", ret);
			return ret;
		}

		pattern_info->h_size = h_size;
		pattern_info->v_size = v_size;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_fill_pure_color(char *buf,
	unsigned int len, struct drm_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	struct drm_pattern_pure_color *pure_color = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	pure_color = &pattern_info->m.pure_color;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_PURE_COLOR;

	find = _mtk_drm_pattern_get_value_from_string(buf, "r", len, &value);
	if (!find)
		pure_color->color.red = 1023;
	else
		pure_color->color.red = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "g", len, &value);
	if (!find)
		pure_color->color.green = 1023;
	else
		pure_color->color.green = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "b", len, &value);
	if (!find)
		pure_color->color.blue = 1023;
	else
		pure_color->color.blue = value;

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_fill_ramp(char *buf,
	unsigned int len, struct drm_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	struct drm_pattern_ramp *ramp = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ramp = &pattern_info->m.ramp;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_RAMP;

	_mtk_drm_pattern_get_value_from_string(buf, "r_st", len, &value);
	ramp->start.red = value;

	_mtk_drm_pattern_get_value_from_string(buf, "g_st", len, &value);
	ramp->start.green = value;

	_mtk_drm_pattern_get_value_from_string(buf, "b_st", len, &value);
	ramp->start.blue = value;

	_mtk_drm_pattern_get_value_from_string(buf, "vertical", len, &value);
	ramp->b_vertical = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "r_end", len, &value);
	if (!find)
		ramp->end.red = 1023;
	else
		ramp->end.red = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "g_end", len, &value);
	if (!find)
		ramp->end.green = 1023;
	else
		ramp->end.green = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "b_end", len, &value);
	if (!find)
		ramp->end.blue = 1023;
	else
		ramp->end.blue = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "level", len, &value);
	if (!find)
		ramp->diff_level = 4;
	else
		ramp->diff_level = value;

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_fill_pure_colorbar(char *buf,
	unsigned int len, struct drm_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct drm_pattern_pure_colorbar *pure_colorbar = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_get_size(pattern_info->position, &h_size, &v_size);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_get_size fail, ret = %d!\n", ret);
		return ret;
	}

	pure_colorbar = &pattern_info->m.pure_colorbar;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_PURE_COLORBAR;

	_mtk_drm_pattern_get_value_from_string(buf, "movable", len, &value);
	pure_colorbar->movable = value;

	_mtk_drm_pattern_get_value_from_string(buf, "speed", len, &value);
	pure_colorbar->shift_speed = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "width", len, &value);
	if (!find)
		pure_colorbar->colorbar_h_size = (h_size >> 3);
	else
		pure_colorbar->colorbar_h_size = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "height", len, &value);
	if (!find)
		pure_colorbar->colorbar_v_size = v_size;
	else
		pure_colorbar->colorbar_v_size = value;

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_fill_pip_window(char *buf,
	unsigned int len, struct drm_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct drm_pattern_pip_window *pip_window = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_get_size(pattern_info->position, &h_size, &v_size);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_get_size fail, ret = %d!\n", ret);
		return ret;
	}

	pip_window = &pattern_info->m.pip_window;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_PIP_WINDOW;

	find = _mtk_drm_pattern_get_value_from_string(buf, "r_st", len, &value);
	if (!find)
		pip_window->pip_color.red = 1023;
	else
		pip_window->pip_color.red = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "g_st", len, &value);
	if (!find)
		pip_window->pip_color.green = 1023;
	else
		pip_window->pip_color.green = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "b_st", len, &value);
	if (!find)
		pip_window->pip_color.blue = 1023;
	else
		pip_window->pip_color.blue = value;

	_mtk_drm_pattern_get_value_from_string(buf, "r_end", len, &value);
	pip_window->bg_color.red = value;

	_mtk_drm_pattern_get_value_from_string(buf, "g_end", len, &value);
	pip_window->bg_color.green = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "b_end", len, &value);
	if (!find)
		pip_window->bg_color.blue = 1023;
	else
		pip_window->bg_color.blue = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "x", len, &value);
	if (!find)
		pip_window->window.x = (h_size >> 2);
	else
		pip_window->window.x = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "y", len, &value);
	if (!find)
		pip_window->window.y = (v_size >> 2);
	else
		pip_window->window.y = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "width", len, &value);
	if (!find)
		pip_window->window.width = (h_size >> 1);
	else
		pip_window->window.width = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "height", len, &value);
	if (!find)
		pip_window->window.height = (v_size >> 1);
	else
		pip_window->window.height = value;

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_fill_crosshatch(char *buf,
	unsigned int len, struct drm_pattern_info *pattern_info)
{
	int ret = 0;
	int value = 0;
	bool find = false;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	struct drm_pattern_crosshatch *crosshatch = NULL;

	ENTER();

	if ((buf == NULL) || (pattern_info == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_get_size(pattern_info->position, &h_size, &v_size);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_get_size fail, ret = %d!\n", ret);
		return ret;
	}

	crosshatch = &pattern_info->m.crosshatch;
	pattern_info->pattern_type = MTK_PATTERN_FLAG_CROSSHATCH;

	find = _mtk_drm_pattern_get_value_from_string(buf, "r_st", len, &value);
	if (!find)
		crosshatch->line_color.red = 1023;
	else
		crosshatch->line_color.red = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "g_st", len, &value);
	if (!find)
		crosshatch->line_color.green = 1023;
	else
		crosshatch->line_color.green = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "b_st", len, &value);
	if (!find)
		crosshatch->line_color.blue = 1023;
	else
		crosshatch->line_color.blue = value;

	_mtk_drm_pattern_get_value_from_string(buf, "r_end", len, &value);
	crosshatch->bg_color.red = value;

	_mtk_drm_pattern_get_value_from_string(buf, "g_end", len, &value);
	crosshatch->bg_color.green = value;

	_mtk_drm_pattern_get_value_from_string(buf, "b_end", len, &value);
	crosshatch->bg_color.blue = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "h_line_interval", len, &value);
	if (!find)
		crosshatch->h_line_interval = 100;
	else
		crosshatch->h_line_interval = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "v_line_interval", len, &value);
	if (!find)
		crosshatch->v_line_interval = 100;
	else
		crosshatch->v_line_interval = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "h_line_width", len, &value);
	if (!find)
		crosshatch->h_line_width = 5;
	else
		crosshatch->h_line_width = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "v_line_width", len, &value);
	if (!find)
		crosshatch->v_line_width = 5;
	else
		crosshatch->v_line_width = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "h_position_st", len, &value);
	if (!find)
		crosshatch->h_position_st = 100;
	else
		crosshatch->h_position_st = value;

	find = _mtk_drm_pattern_get_value_from_string(buf, "v_position_st", len, &value);
	if (!find)
		crosshatch->v_position_st = 100;
	else
		crosshatch->v_position_st = value;

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_set_off(bool riu,
	enum hwreg_render_pattern_position point, struct hwregOut *pattern_reg)
{
	int ret = 0;

	if (point == DRM_PATTERN_POSITION_OPDRAM) {
		ret = drv_hwreg_render_pattern_opdram_set_off(riu);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"[drv_hwreg][%s %d] fail, position = %u, ret = %d!\n",
				__func__, __LINE__, point, ret);
			return -EINVAL;
		}
	} else if (point == DRM_PATTERN_POSITION_IPBLEND) {
		ret = drv_hwreg_render_pattern_ipblend_set_off(riu, pattern_reg);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"[drv_hwreg][%s %d] fail, position = %u, ret = %d!\n",
			__func__, __LINE__, point, ret);
			return -EINVAL;
		}
	} else if (OPBLEND(point)) {
		ret = drv_hwreg_render_pattern_opblend_set_off(riu, point, pattern_reg);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"[drv_hwreg][%s %d] fail, position = %u, ret = %d!\n",
				__func__, __LINE__, point, ret);
			return -EINVAL;
		}
	} else {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)!\n", point);
		return -EINVAL;
	}

	return ret;
}

static int _mtk_drm_pattern_set_pure_color(bool riu,
	struct drm_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;
	enum drm_pattern_position position;
	struct hwreg_render_pattern_pure_color data;
	struct drm_pattern_pure_color *pure_color = NULL;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	memset(&data, 0, sizeof(struct hwreg_render_pattern_pure_color));
	position = pattern_info->position;

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position, position = %u\n", position);
		return -EINVAL;
	}

	pure_color = &(pattern_info->m.pure_color);

	ret = _mtk_drm_pattern_check_color_range(&pure_color->color);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,	"[drv_hwreg][%s %d] fail, ret = %d!\n",
				__func__, __LINE__, ret);
		return -EINVAL;
	}

	if (position == DRM_PATTERN_POSITION_OPDRAM) {
		data.common.position = position;
		data.common.enable = true;
		data.color.red = pure_color->color.red;
		data.color.green = pure_color->color.green;
		data.color.blue = pure_color->color.blue;

		ret = drv_hwreg_render_pattern_set_opdram_pure_color(riu, &data, pattern_reg);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR,	"[drv_hwreg][%s %d] fail, ret = %d!\n",
					__func__, __LINE__, ret);
			return -EINVAL;
		}
	} else if (position == DRM_PATTERN_POSITION_IPBLEND) {
		ret = drv_hwreg_render_pattern_set_ipblend_pure_color(riu,
			pattern_reg);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR, "[drv_hwreg][%s %d] fail, ret = %d!\n",
					__func__, __LINE__, ret);
			return -EINVAL;
		}
	} else if (OPBLEND(position)) {
		data.common.position = position;
		data.common.enable = true;
		data.common.h_size = pattern_info->h_size;
		data.common.v_size = pattern_info->v_size;
		data.common.color_space = pattern_env[position].color_space;
		data.color.red = pure_color->color.red;
		data.color.green = pure_color->color.green;
		data.color.blue = pure_color->color.blue;

		ret = drv_hwreg_render_pattern_set_opblend_pure_color(riu, &data, pattern_reg);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR, "[drv_hwreg][%s %d] fail, ret = %d!\n",
					__func__, __LINE__, ret);
			return -EINVAL;
		}
	} else {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position, position = %u\n"
				, position);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_set_ramp(bool riu,
	struct drm_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;
	unsigned short color_st = 0;
	unsigned short color_end = 0;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	unsigned int diff_level = 0;
	enum drm_pattern_position position;
	struct hwreg_render_pattern_ramp data;
	struct drm_pattern_ramp *ramp = NULL;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	memset(&data, 0, sizeof(struct hwreg_render_pattern_ramp));
	position = pattern_info->position;

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position, position = %u\n", position);
		return -EINVAL;
	}

	h_size = pattern_env[position].h_size;
	v_size = pattern_env[position].v_size;

	if (!(h_size & v_size)) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid h/v size, h = 0x%x, v = 0x%x\n", h_size, v_size);
		return -EINVAL;
	}

	ramp = &(pattern_info->m.ramp);

	ret = _mtk_drm_pattern_check_level(position, ramp->diff_level, ramp->b_vertical);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_level fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_check_gradient_color(&ramp->start, &ramp->end);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_gradient_color fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	color_st = max_t(unsigned short, ramp->start.red,
		max_t(unsigned short, ramp->start.green, ramp->start.blue));

	color_end = max_t(unsigned short, ramp->end.red,
		max_t(unsigned short, ramp->end.green, ramp->end.blue));

	diff_level = ramp->diff_level;
	data.common.position = position;
	data.common.enable = true;
	data.common.h_size = pattern_info->h_size;
	data.common.v_size = pattern_info->v_size;
	data.common.color_space = pattern_env[position].color_space;
	data.start.red_integer = ramp->start.red;
	data.start.green_integer = ramp->start.green;
	data.start.blue_integer = ramp->start.blue;
	data.end.red_integer = ramp->end.red;
	data.end.green_integer = ramp->end.green;
	data.end.blue_integer = ramp->end.blue;
	data.b_vertical = ramp->b_vertical;

	if (ramp->b_vertical) {
		data.diff_h = h_size / 4;
		data.diff_v = (abs(color_st - color_end) << 13) / (diff_level - 1);
		data.ratio_h = 1;
		data.ratio_v = data.diff_v * diff_level / v_size;
	} else {
		data.diff_v = v_size / 4;
		data.diff_h = (abs(color_st - color_end) << 13) / (diff_level - 1);
		data.ratio_v = 1;
		data.ratio_h = data.diff_h * diff_level / h_size;
	}

	ret = drv_hwreg_render_pattern_set_ramp(riu, &data, pattern_reg);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"drv_hwreg_render_pattern_set_ramp fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_set_pure_colorbar(bool riu,
	struct drm_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;
	enum drm_pattern_position position;
	struct hwreg_render_pattern_pure_colorbar data;
	struct drm_pattern_pure_colorbar *pure_colorbar = NULL;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	memset(&data, 0, sizeof(struct hwreg_render_pattern_pure_colorbar));
	position = pattern_info->position;

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position,position = %u\n", position);
		return -EINVAL;
	}

	pure_colorbar = &(pattern_info->m.pure_colorbar);
	data.common.position = position;
	data.common.enable = true;
	data.common.color_space = pattern_env[position].color_space;
	data.diff_h = pure_colorbar->colorbar_h_size;
	data.diff_v = pure_colorbar->colorbar_v_size;
	data.movable = pure_colorbar->movable;

	if (pure_colorbar->movable)
		data.shift_speed = pure_colorbar->shift_speed;

	ret = drv_hwreg_render_pattern_set_pure_colorbar(riu, &data, pattern_reg);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"drv_hwreg_render_pattern_set_pure_colorbar fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_set_pip_window(bool riu,
	struct drm_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	enum drm_pattern_position position;
	struct hwreg_render_pattern_pip_window data;
	struct drm_pattern_window *window = NULL;
	struct drm_pattern_pip_window *pip_window = NULL;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	memset(&data, 0, sizeof(struct hwreg_render_pattern_pip_window));
	position = pattern_info->position;

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position,position = %u\n", position);
		return -EINVAL;
	}

	h_size = pattern_env[position].h_size;
	v_size = pattern_env[position].v_size;

	pip_window = &(pattern_info->m.pip_window);
	window = &(pip_window->window);

	data.common.position = position;
	data.common.enable = true;
	data.common.h_size = pattern_info->h_size;
	data.common.v_size = pattern_info->v_size;
	data.common.color_space = pattern_env[position].color_space;

	ret = _mtk_drm_pattern_check_color_range(&pip_window->pip_color);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_range fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_check_color_range(&pip_window->bg_color);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_range fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	data.pip.red_integer = pip_window->pip_color.red;
	data.pip.green_integer = pip_window->pip_color.green;
	data.pip.blue_integer = pip_window->pip_color.blue;
	data.bg.red_integer = pip_window->bg_color.red;
	data.bg.green_integer = pip_window->bg_color.green;
	data.bg.blue_integer = pip_window->bg_color.blue;

	if ((window->x >= h_size) || (window->x + window->width >= h_size)
		|| (window->y >= v_size) || (window->y + window->height >= v_size)
		|| (window->width == 0) || (window->height == 0)) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid pip window size(%u, %u, %u, %u), ret = %d!\n",
			window->x, window->y, window->width, window->height, ret);
		return -EINVAL;
	}

	data.window.h_start = window->x;
	data.window.h_end = window->x + window->width;
	data.window.v_start = window->y;
	data.window.v_end = window->y + window->height;

	ret = drv_hwreg_render_pattern_set_pip_window(riu, &data, pattern_reg);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"drv_hwreg_render_pattern_set_pip_window fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_set_crosshatch(bool riu,
	struct drm_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;
	enum drm_pattern_position position;
	struct hwreg_render_pattern_crosshatch data;
	struct drm_pattern_crosshatch *crosshatch = NULL;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	memset(&data, 0, sizeof(struct hwreg_render_pattern_crosshatch));
	position = pattern_info->position;

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"Invalid position,position = %u\n", position);
		return -EINVAL;
	}

	crosshatch = &(pattern_info->m.crosshatch);

	data.common.position = position;
	data.common.enable = true;
	data.common.h_size = pattern_info->h_size;
	data.common.v_size = pattern_info->v_size;
	data.common.color_space = pattern_env[position].color_space;

	ret = _mtk_drm_pattern_check_color_range(&crosshatch->line_color);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_range fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_check_color_range(&crosshatch->bg_color);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_check_color_range fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	data.line.red_integer = crosshatch->line_color.red;
	data.line.green_integer = crosshatch->line_color.green;
	data.line.blue_integer = crosshatch->line_color.blue;
	data.bg.red_integer = crosshatch->bg_color.red;
	data.bg.green_integer = crosshatch->bg_color.green;
	data.bg.blue_integer = crosshatch->bg_color.blue;

	data.h_line_interval = crosshatch->h_line_interval;
	data.v_line_interval = crosshatch->v_line_interval;
	data.h_position_st = crosshatch->h_position_st;
	data.h_line_width = crosshatch->h_line_width;
	data.v_position_st = crosshatch->v_position_st;
	data.v_line_width = crosshatch->v_line_width;

	ret = drv_hwreg_render_pattern_set_crosshatch(riu, &data, pattern_reg);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"drv_hwreg_render_pattern_set_crosshatch fail, ret = %d!\n", ret);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_set_random(bool riu,
	struct drm_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;
	struct hwreg_render_pattern_common_data data;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	ret = drv_hwreg_render_pattern_set_random(riu, pattern_reg);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"[drv_hwreg][%s %d] fail, ret = %d!\n",
			__func__, __LINE__, ret);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static int _mtk_drm_pattern_set_chessboard(bool riu,
	struct drm_pattern_info *pattern_info, struct hwregOut *pattern_reg)
{
	int ret = 0;

	ENTER();

	if ((pattern_info == NULL) || (pattern_reg == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	PATTERN_LOG(PATTERN_LOG_INFO,
		"Set Pattern %s\n", pattern_info->enable ? "ON" : "OFF");

	ret = drv_hwreg_render_pattern_set_chessboard(riu, pattern_reg);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"[drv_hwreg][%s %d] fail, ret = %d!\n",
			__func__, __LINE__, ret);
		return -EINVAL;
	}

	EXIT();
	return ret;
}

static ssize_t _mtk_drm_pattern_show_status(
	char *buf, ssize_t used_count, enum drm_pattern_position position,
	struct mtk_tv_kms_context *drm_ctx)
{
	int ret = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	unsigned short h_size = 0;
	unsigned short v_size = 0;

	ENTER();

	if (buf == NULL || drm_ctx == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return 0;
	}

	count = scnprintf(buf + total_count + used_count,
		SYSFS_MAX_BUF_COUNT - total_count - used_count,
		"DRM Pattern Status:\n");
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

	if (true) {
		if (position == drm_ctx->pattern_status.pattern_enable_position) {
			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"|   ON   |");
			total_count += count;

			count = scnprintf(buf + total_count + used_count,
				SYSFS_MAX_BUF_COUNT - total_count - used_count,
				"   %u  |", drm_ctx->pattern_status.pattern_type);
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

		ret = _mtk_drm_pattern_get_size(position, &h_size, &v_size);
		if (ret)
			PATTERN_LOG(PATTERN_LOG_ERROR, "Get DRM Pattern Size Fail!\n");

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

static ssize_t _mtk_drm_pattern_show_capbility(
	char *buf, ssize_t used_count, enum drm_pattern_position position)
{
	int ret = 0;
	int index = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	struct drm_pattern_capability cap;

	ENTER();

	if (buf == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return 0;
	}

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return 0;
	}

	memset(&cap, 0, sizeof(struct drm_pattern_capability));

	ret = _mtk_drm_pattern_get_capability(&cap);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Get DRM Pattern Capability Fail!\n");
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
		"\n|      %s     |", position_str[position], position);
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

static ssize_t _mtk_drm_pattern_show_help(
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

static ssize_t _mtk_drm_pattern_show_mapping_table(
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

static int _mtk_drm_pattern_set_info(struct drm_pattern_info *pattern_info)
{
	int ret = 0;
	bool riu = true;
	struct hwregOut pattern_reg;
	enum drm_pattern_position position;

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

	for (position = DRM_PATTERN_POSITION_OPDRAM;
		position < DRM_PATTERN_POSITION_MAX; position++) {
		ret = _mtk_drm_pattern_set_off(riu, position, &pattern_reg);

		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"_mtk_drm_pattern_set_off fail, position = %u, ret = %d!\n",
				position, ret);
			goto exit;
		}

		pattern_reg.regCount = 0;
	}

	if (pattern_info->enable) {
		if (pattern_info->position >= DRM_PATTERN_POSITION_MAX) {
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
		case MTK_PATTERN_FLAG_PURE_COLOR:
			ret = _mtk_drm_pattern_set_pure_color(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_set_pure_color fail, ret = %d!\n",
					ret);
				goto exit;
			}
			break;
		case MTK_PATTERN_FLAG_RAMP:
			ret = _mtk_drm_pattern_set_ramp(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_set_ramp fail, ret = %d!\n",
					ret);
				goto exit;
			}
			break;
		case MTK_PATTERN_FLAG_PURE_COLORBAR:
			ret = _mtk_drm_pattern_set_pure_colorbar(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_set_pure_colorbar fail, ret = %d!\n",
					ret);
				goto exit;
			}
			break;
		case MTK_PATTERN_FLAG_PIP_WINDOW:
			ret = _mtk_drm_pattern_set_pip_window(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_set_pip_window fail, ret = %d!\n",
					ret);
				goto exit;
			}
			break;
		case MTK_PATTERN_FLAG_CROSSHATCH:
			ret = _mtk_drm_pattern_set_crosshatch(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_set_crosshatch fail, ret = %d!\n",
					ret);
				goto exit;
			}
			break;
		case MTK_PATTERN_FLAG_RANDOM:
			ret = _mtk_drm_pattern_set_random(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_set_random fail, ret = %d!\n",
					ret);
				goto exit;
			}
			break;
		case MTK_PATTERN_FLAG_CHESSBOARD:
			ret = _mtk_drm_pattern_set_chessboard(riu,
				pattern_info, &pattern_reg);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_set_chessboard fail, ret = %d!\n",
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

int mtk_drm_pattern_set_env(
	enum drm_pattern_position position,
	struct drm_pattern_env *env)
{
	int ret = 0;

	ENTER();

	if (env == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= DRM_PATTERN_POSITION_MAX) {
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

ssize_t mtk_drm_pattern_show(char *buf,
	struct device *dev, enum drm_pattern_position position)
{
	int index;
	int ret = 0;
	ssize_t count = 0;
	ssize_t total_count = 0;
	struct drm_pattern_capability cap;
	struct mtk_tv_kms_context *drm_ctx = NULL;

	ENTER();

	if ((buf == NULL) || (dev == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return -EINVAL;
	}

	memset(&cap, 0, sizeof(struct drm_pattern_capability));

#ifdef PATTERN_TEST
	pattern_env[position].h_size = 7680;
	pattern_env[position].v_size = 4320;
#endif

	drm_ctx = dev_get_drvdata(dev);
	if (!drm_ctx) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "drm_ctx is NULL!\n");
		return -EINVAL;
	}

	ret = _mtk_drm_pattern_get_capability(&cap);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Get DRM Pattern Capability Fail!\n");
		return -EINVAL;
	}

	count = scnprintf(buf + total_count, SYSFS_MAX_BUF_COUNT - total_count,
		"Position: %s\n\n", position_str[position]);
	total_count += count;

	count = _mtk_drm_pattern_show_status(buf, total_count, position, drm_ctx);
	total_count += count;

	count = _mtk_drm_pattern_show_capbility(buf, total_count, position);
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
			count = _mtk_drm_pattern_show_help(buf, total_count, BIT(index));
			total_count += count;
		}
	}

	count = _mtk_drm_pattern_show_mapping_table(buf, total_count);
	total_count += count;

	EXIT();

	PATTERN_LOG(PATTERN_LOG_INFO, "count = %d!\n", total_count);

	return total_count;
}

int mtk_drm_pattern_store(const char *buf,
	struct device *dev, enum drm_pattern_position position)
{
	int ret = 0;
	int len = 0;
	int type = 0;
	bool find = false;
	char *cmd = NULL;
	struct drm_pattern_info pattern_info;
	struct mtk_tv_kms_context *drm_ctx = NULL;
	struct drm_pattern_capability cap;

	ENTER();

	if ((buf == NULL) || (dev == NULL)) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Pointer is NULL!\n");
		return -EINVAL;
	}

	if (position >= DRM_PATTERN_POSITION_MAX) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "Invalid position(%u)\n", position);
		return -EINVAL;
	}

	drm_ctx = dev_get_drvdata(dev);
	if (!drm_ctx) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "drm_ctx is NULL!\n");
		return -EINVAL;
	}

	cmd = vmalloc(sizeof(char) * 0x1000);
	if (cmd == NULL) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "vmalloc cmd fail!\n");
		return -EINVAL;
	}

	memset(cmd, 0, sizeof(char) * 0x1000);
	memset(&pattern_info, 0, sizeof(struct drm_pattern_info));
	memset(&cap, 0, sizeof(struct drm_pattern_capability));

#ifdef PATTERN_TEST
	pattern_env[position].h_size = 7680;
	pattern_env[position].v_size = 4320;
#endif

	len = strlen(buf);

	snprintf(cmd, len + 1, buf);

	pattern_info.position = position;

	ret = _mtk_drm_pattern_fill_common_info(cmd, len, &pattern_info);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_fill_common_info fail, ret = %d!\n", ret);
		goto exit;
	}

	if (!pattern_info.enable) {
		drm_ctx->pattern_status.pattern_enable_position = INVALID_PATTERN_POSITION;
	} else {
		ret = _mtk_drm_pattern_get_capability(&cap);
		if (ret) {
			PATTERN_LOG(PATTERN_LOG_ERROR, "Get DRM Pattern Capability Fail!\n");
			ret = -EINVAL;
			goto exit;
		}

		find = _mtk_drm_pattern_get_value_from_string(cmd, "type", len, &type);
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
		case 0:
			ret = _mtk_drm_pattern_fill_pure_color(cmd, len, &pattern_info);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_fill_pure_color fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 1:
			ret = _mtk_drm_pattern_fill_ramp(cmd, len, &pattern_info);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_fill_ramp fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 2:
			ret = _mtk_drm_pattern_fill_pure_colorbar(cmd, len, &pattern_info);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_fill_pure_colorbar fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 6:
			ret = _mtk_drm_pattern_fill_pip_window(cmd, len, &pattern_info);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_fill_pip_window fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 8:
			ret = _mtk_drm_pattern_fill_crosshatch(cmd, len, &pattern_info);
			if (ret) {
				PATTERN_LOG(PATTERN_LOG_ERROR,
					"_mtk_drm_pattern_fill_crosshatch fail, ret = %d!\n",
					ret);
				goto exit;
			}

			break;
		case 9:
			pattern_info.pattern_type = MTK_PATTERN_FLAG_RANDOM;
			break;
		case 10:
			pattern_info.pattern_type = MTK_PATTERN_FLAG_CHESSBOARD;
			break;
		default:
			PATTERN_LOG(PATTERN_LOG_ERROR,
				"Pattern Type::%d Not Support!\n", type);
			ret = -EINVAL;
			goto exit;
		}

		drm_ctx->pattern_status.pattern_enable_position = position;
		drm_ctx->pattern_status.pattern_type = type;
	}

	ret = _mtk_drm_pattern_set_info(&pattern_info);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR,
			"_mtk_drm_pattern_set_info fail, ret = %d!\n", ret);
		goto exit;
	}

exit:
	vfree(cmd);

	EXIT();
	return ret;
}

int mtk_drm_pattern_set_param(
	struct drm_pattern_status *pattern,
	struct drm_mtk_pattern_config *config)
{
	struct drm_pattern_info pattern_info;
	struct drm_pattern_capability cap;
	struct drm_pattern_env env;
	unsigned short h_size = 0;
	unsigned short v_size = 0;
	int ret = 0;

	ENTER();

	if ((pattern == NULL) || (config == NULL)) {
		ERR("Pointer is NULL!\n");
		return -EINVAL;
	}
	ret = _mtk_drm_pattern_get_capability(&cap);
	if (ret) {
		ERR("pattern get capability fail, ret = %d\n", ret);
		return ret;
	}
	ret = _mtk_drm_pattern_get_size(config->pattern_position, &h_size, &v_size);
	if (ret) {
		ERR("pattern get size fail, ret = %d!\n", ret);
		return ret;
	}
	if (h_size == 0 || v_size == 0) { // invalid H/V, set default value
		env.h_size = h_size = UHD_8K_W;
		env.v_size = v_size = UHD_8K_H;
		env.color_space = DRM_PATTERN_COLOR_SPACE_RGB;
		ret = mtk_drm_pattern_set_env(config->pattern_position, &env);
		if (ret) {
			ERR("pattern set env fail, ret = %d\n", ret);
			return ret;
		}
	}

	memset(&pattern_info, 0, sizeof(struct drm_pattern_info));
	if (config->pattern_type < MTK_PATTERN_MAX) {
		pattern->pattern_enable_position = config->pattern_position;
		pattern_info.position = config->pattern_position;
		pattern_info.enable = true;
		pattern_info.h_size = h_size;
		pattern_info.v_size = v_size;

		switch (config->pattern_type) {
		case MTK_PATTERN_PURE_COLOR: {
			struct drm_pattern_pure_color *pure_color = &pattern_info.m.pure_color;
			struct drm_mtk_pattern_pure_color *mtk_pure_color = &config->u.pure_color;

			pattern->pattern_type = MTK_PATTERN_BIT_INDEX_PURE_COLOR;
			pattern_info.pattern_type = MTK_PATTERN_FLAG_PURE_COLOR;
			pure_color->color.red   = mtk_pure_color->color.red;
			pure_color->color.green = mtk_pure_color->color.green;
			pure_color->color.blue  = mtk_pure_color->color.blue;
			LOG("MTK_PATTERN_PURE_COLOR");
			LOG("  red = %u", pure_color->color.red);
			LOG("  red = %u", pure_color->color.green);
			LOG("  red = %u", pure_color->color.blue);
			break;
		}
		case MTK_PATTERN_RAMP: {
			struct drm_pattern_ramp *ramp = &pattern_info.m.ramp;
			struct drm_mtk_pattern_ramp *mtk_ramp = &config->u.ramp;

			pattern->pattern_type = MTK_PATTERN_BIT_INDEX_RAMP;
			pattern_info.pattern_type = MTK_PATTERN_FLAG_RAMP;
			ramp->start.red   = mtk_ramp->color_begin.red;
			ramp->start.green = mtk_ramp->color_begin.green;
			ramp->start.blue  = mtk_ramp->color_begin.blue;
			ramp->end.red     = mtk_ramp->color_end.red;
			ramp->end.green   = mtk_ramp->color_end.green;
			ramp->end.blue    = mtk_ramp->color_end.blue;
			ramp->b_vertical  = mtk_ramp->vertical;
			ramp->diff_level  = mtk_ramp->level;
			LOG("MTK_PATTERN_RAMP");
			LOG("  start red   = %u", ramp->start.red);
			LOG("  start green = %u", ramp->start.green);
			LOG("  start blue  = %u", ramp->start.blue);
			LOG("  end red     = %u", ramp->end.red);
			LOG("  end green   = %u", ramp->end.green);
			LOG("  end blue    = %u", ramp->end.blue);
			LOG("  vertical    = %u", ramp->b_vertical);
			LOG("  level       = %u", ramp->diff_level);
			break;
		}
		case MTK_PATTERN_PIP_WINDOW: {
			struct drm_pattern_pip_window *pip_window = &pattern_info.m.pip_window;
			struct drm_mtk_pattern_pip_window *mtk_pip_window = &config->u.pip_window;

			pattern->pattern_type = MTK_PATTERN_BIT_INDEX_PIP_WINDOW;
			pattern_info.pattern_type = MTK_PATTERN_FLAG_PIP_WINDOW;
			pip_window->pip_color.red   = mtk_pip_window->window_color.red;
			pip_window->pip_color.green = mtk_pip_window->window_color.green;
			pip_window->pip_color.blue  = mtk_pip_window->window_color.blue;
			pip_window->bg_color.red    = mtk_pip_window->background_color.red;
			pip_window->bg_color.green  = mtk_pip_window->background_color.green;
			pip_window->bg_color.blue   = mtk_pip_window->background_color.blue;
			pip_window->window.x        = mtk_pip_window->window_x;
			pip_window->window.y        = mtk_pip_window->window_y;
			pip_window->window.width    = mtk_pip_window->window_width;
			pip_window->window.height   = mtk_pip_window->window_height;
			LOG("MTK_PATTERN_PIP_WINDOW");
			LOG("  pip red   = %u", pip_window->pip_color.red);
			LOG("  pip green = %u", pip_window->pip_color.green);
			LOG("  pip blue  = %u", pip_window->pip_color.blue);
			LOG("  bg red    = %u", pip_window->bg_color.red);
			LOG("  bg green  = %u", pip_window->bg_color.green);
			LOG("  bg blue   = %u", pip_window->bg_color.blue);
			LOG("  window x  = %u", pip_window->window.x);
			LOG("  window y  = %u", pip_window->window.y);
			LOG("  window w  = %u", pip_window->window.width);
			LOG("  window h  = %u", pip_window->window.height);
			break;
		}
		case MTK_PATTERN_CROSSHATCH: {
			struct drm_pattern_crosshatch *crosshatch = &pattern_info.m.crosshatch;
			struct drm_mtk_pattern_crosshatch *mtk_crosshatch = &config->u.crosshatch;

			pattern->pattern_type = MTK_PATTERN_BIT_INDEX_CROSSHATCH;
			pattern_info.pattern_type = MTK_PATTERN_FLAG_CROSSHATCH;
			crosshatch->line_color.red   = mtk_crosshatch->line_color.red;
			crosshatch->line_color.green = mtk_crosshatch->line_color.green;
			crosshatch->line_color.blue  = mtk_crosshatch->line_color.blue;
			crosshatch->bg_color.red     = mtk_crosshatch->background_color.red;
			crosshatch->bg_color.green   = mtk_crosshatch->background_color.green;
			crosshatch->bg_color.blue    = mtk_crosshatch->background_color.blue;
			crosshatch->h_position_st    = mtk_crosshatch->line_start_y;
			crosshatch->v_position_st    = mtk_crosshatch->line_start_x;
			crosshatch->h_line_width     = mtk_crosshatch->line_width_h;
			crosshatch->v_line_width     = mtk_crosshatch->line_width_v;
			crosshatch->h_line_interval  = mtk_crosshatch->line_interval_h;
			crosshatch->v_line_interval  = mtk_crosshatch->line_interval_v;
			LOG("MTK_PATTERN_CROSSHATCH");
			LOG("  line red   = %u", crosshatch->line_color.red);
			LOG("  line green = %u", crosshatch->line_color.green);
			LOG("  line blue  = %u", crosshatch->line_color.blue);
			LOG("  bg red     = %u", crosshatch->bg_color.red);
			LOG("  bg green   = %u", crosshatch->bg_color.green);
			LOG("  bg blue    = %u", crosshatch->bg_color.blue);
			LOG("  start y    = %u", crosshatch->h_position_st);
			LOG("  start x    = %u", crosshatch->v_position_st);
			LOG("  width h    = %u", crosshatch->h_line_width);
			LOG("  width v    = %u", crosshatch->v_line_width);
			LOG("  interval h = %u", crosshatch->h_line_interval);
			LOG("  interval v = %u", crosshatch->v_line_interval);
			break;
		}
		case MTK_PATTERN_RADOM: {
			pattern->pattern_type = MTK_PATTERN_BIT_INDEX_RANDOM;
			pattern_info.pattern_type = MTK_PATTERN_FLAG_RANDOM;
			LOG("MTK_PATTERN_RADOM");
			break;
		}
		case MTK_PATTERN_CHESSBOARD: {
			pattern->pattern_type = MTK_PATTERN_BIT_INDEX_CHESSBOARD;
			pattern_info.pattern_type = MTK_PATTERN_FLAG_CHESSBOARD;
			LOG("MTK_PATTERN_CHESSBOARD");
			break;
		}
		default:
			return -EINVAL;
		}
	} else {
		pattern->pattern_enable_position = INVALID_PATTERN_POSITION;
	}
	ret = _mtk_drm_pattern_set_info(&pattern_info);
	if (ret) {
		PATTERN_LOG(PATTERN_LOG_ERROR, "_mtk_drm_pattern_set_info fail, ret = %d!\n", ret);
		return ret;
	}
	EXIT();
	return ret;
}
