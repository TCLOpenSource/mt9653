/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_CRCCAP_PATTERN_H
#define MTK_CRCCAP_PATTERN_H
//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#ifndef BIT
#define BIT(_bit_)								(1 << (_bit_))
#endif

#ifndef MTK_PATTERN_FLAG_PURE_COLOR
#define MTK_PATTERN_FLAG_PURE_COLOR				(BIT(0))
#endif

#ifndef MTK_PATTERN_FLAG_RAMP
#define MTK_PATTERN_FLAG_RAMP					(BIT(1))
#endif

#ifndef MTK_PATTERN_FLAG_PURE_COLORBAR
#define MTK_PATTERN_FLAG_PURE_COLORBAR			(BIT(2))
#endif

#ifndef MTK_PATTERN_FLAG_GRADIENT_COLORBAR
#define MTK_PATTERN_FLAG_GRADIENT_COLORBAR		(BIT(3))
#endif

#ifndef MTK_PATTERN_FLAG_BLACK_WHITE_COLORBAR
#define MTK_PATTERN_FLAG_BLACK_WHITE_COLORBAR	(BIT(4))
#endif

#ifndef MTK_PATTERN_FLAG_CROSS
#define MTK_PATTERN_FLAG_CROSS					(BIT(5))
#endif

#ifndef MTK_PATTERN_FLAG_PIP_WINDOW
#define MTK_PATTERN_FLAG_PIP_WINDOW				(BIT(6))
#endif

#ifndef MTK_PATTERN_FLAG_DOT_MATRIX
#define MTK_PATTERN_FLAG_DOT_MATRIX				(BIT(7))
#endif

#ifndef MTK_PATTERN_FLAG_CROSSHATCH
#define MTK_PATTERN_FLAG_CROSSHATCH				(BIT(8))
#endif

//-------------------------------------------------------------------------------------------------
//  enums
//-------------------------------------------------------------------------------------------------
enum srccap_pattern_position {
	SRCCAP_PATTERN_POSITION_IP1,
	SRCCAP_PATTERN_POSITION_PRE_IP2_0,
	SRCCAP_PATTERN_POSITION_PRE_IP2_1,
	SRCCAP_PATTERN_POSITION_PRE_IP2_0_POST,
	SRCCAP_PATTERN_POSITION_PRE_IP2_1_POST,
	SRCCAP_PATTERN_POSITION_MAX,
};

enum srccap_pattern_color_space {
	SRCCAP_PATTERN_COLOR_SPACE_RGB,
	SRCCAP_PATTERN_COLOR_SPACE_YUV_FULL,
	SRCCAP_PATTERN_COLOR_SPACE_YUV_LIMITED,
	SRCCAP_PATTERN_COLOR_SPACE_MAX,
};

//-------------------------------------------------------------------------------------------------
//  structs
//-------------------------------------------------------------------------------------------------
struct srccap_pattern_status {
	unsigned char pattern_enable_position;
	unsigned char pattern_type;
};

struct srccap_pattern_env {
	unsigned short h_size;
	unsigned short v_size;
	enum srccap_pattern_color_space color_space;
};

struct srccap_pattern_pure_colorbar {
	unsigned int colorbar_h_size;
	unsigned int colorbar_v_size;
	unsigned short shift_speed;
	bool movable;
};

struct srccap_pattern_info {
	enum srccap_pattern_position position;
	unsigned short pattern_type;
	union {
		struct srccap_pattern_pure_colorbar pure_colorbar;
	} m;
	bool enable;
};

struct srccap_pattern_capability {
	unsigned short pattern_type[SRCCAP_PATTERN_POSITION_MAX];
};

//-------------------------------------------------------------------------------------------------
//  function
//-------------------------------------------------------------------------------------------------
int mtk_srccap_pattern_set_env(enum srccap_pattern_position position,
	struct srccap_pattern_env *env);

ssize_t mtk_srccap_pattern_show(char *buf,
	struct device *dev, enum srccap_pattern_position position);

int mtk_srccap_pattern_store(const char *buf,
		struct device *dev, enum srccap_pattern_position position);

#endif
