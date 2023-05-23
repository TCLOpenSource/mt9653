/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _WRAP_G2D_H_
#define _WRAP_G2D_H_

#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>

#define G2D_LABEL "[G2D] "
#define LOG_LEVEL_NORMAL  (1)
#define LOG_LEVEL_FATAL   (2)
#define LOG_LEVEL_ERROR   (3)
#define LOG_LEVEL_WARNING (4)
#define LOG_LEVEL_INFO    (5)
#define LOG_LEVEL_DEBUG   (6)

#define G2D_LOG(_dbgLevel_, _fmt, _args...) \
do { \
	switch (_dbgLevel_) { \
	case LOG_LEVEL_FATAL: \
	if (log_level == LOG_LEVEL_NORMAL)  \
		pr_crit(G2D_LABEL "[fatal] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	else if (log_level >= LOG_LEVEL_FATAL) \
		pr_emerg(G2D_LABEL "[fatal] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_ERROR: \
	if (log_level == LOG_LEVEL_NORMAL)  \
		pr_err(G2D_LABEL "[error] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	else if (log_level >= LOG_LEVEL_ERROR) \
		pr_emerg(G2D_LABEL "[error] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_WARNING: \
	if (log_level == LOG_LEVEL_NORMAL)  \
		pr_warn(G2D_LABEL "[warning] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	else if (log_level >= LOG_LEVEL_WARNING) \
		pr_emerg(G2D_LABEL "[warning] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_INFO: \
	if (log_level == LOG_LEVEL_NORMAL)  \
		pr_info(G2D_LABEL "[info] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	else if (log_level >= LOG_LEVEL_INFO) \
		pr_emerg(G2D_LABEL "[info] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	case LOG_LEVEL_DEBUG: \
	if (log_level == LOG_LEVEL_NORMAL)  \
		pr_debug(G2D_LABEL "[debug] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	else if (log_level >= LOG_LEVEL_DEBUG) \
		pr_emerg(G2D_LABEL "[debug] [%s,%d] "_fmt, \
		__func__, __LINE__, ##_args); \
	break; \
	default: \
	break; \
	} \
} while (0)

#define G2D_DEBUG(format, args...) \
				G2D_LOG(LOG_LEVEL_DEBUG, format, ##args)
#define G2D_INFO(format, args...) \
				G2D_LOG(LOG_LEVEL_INFO, format, ##args)
#define G2D_WARNING(format, args...) \
				G2D_LOG(LOG_LEVEL_WARNING, format, ##args)
#define G2D_ERROR(format, args...) \
				G2D_LOG(LOG_LEVEL_ERROR, format, ##args)
#define G2D_FATAL(format, args...) \
				G2D_LOG(LOG_LEVEL_FATAL, format, ##args)

extern __u32 log_level;


int g2d_utp_init(void);
int g2d_resource(u32 type);
int g2d_wait_done(struct v4l2_ext_control *ext_ctrl);
int g2d_get_clip(struct v4l2_selection *s);
int g2d_set_clip(struct v4l2_selection *s);
int g2d_render(struct v4l2_ext_control *ext_ctrl);
int g2d_get_render_info(struct v4l2_ext_control *ext_ctrl);
int g2d_set_ext_ctrl(struct v4l2_ext_control *ext_ctrl);
int g2d_set_mirror(u32 bHMirror, u32 bVMirror);
int g2d_set_rotate(u32 u32Rotate);
int g2d_set_powerstate(u32 state);
int g2d_get_crc(u32 *CRCvalue);
void dump_g2d_status(void);

#endif
