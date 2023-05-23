/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __VBI_EX_H__
#define __VBI_EX_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <stdbool.h>

extern int log_level;

#define SRCCAP_VBI_MDBGIN_LABEL  "[SRCCAP][mdbgin_vbi]"
#define SRCCAP_VBI_MDBGERR_LABEL "[SRCCAP][mdbgerr_vbi]"
#define SRCCAP_VBI_MDBGDBG_LABEL "[SRCCAP][mdbgdbg_vbi]"
#define SRCCAP_VBI_MDBGWAR_LABEL "[SRCCAP][mdbgwar_vbi]"
#define SRCCAP_VBI_MDBGFTL_LABEL "[SRCCAP][mdbgftl_vbi]"

#define LOG_LEVEL_VBI_MASK    (0xFFFF)
#define LOG_LEVEL_VBI_NORMAL  (1)
#define LOG_LEVEL_VBI_FATAL   (2)
#define LOG_LEVEL_VBI_ERROR   (3)
#define LOG_LEVEL_VBI_WARNING (4)
#define LOG_LEVEL_VBI_INFO    (5)
#define LOG_LEVEL_VBI_DEBUG   (6)

#define SRCCAP_VBI_LOG(_dbgLevel_, _fmt, _args...) \
	do {\
		switch (_dbgLevel_) { \
		case LOG_LEVEL_VBI_FATAL: \
		if ((log_level & LOG_LEVEL_VBI_MASK) == LOG_LEVEL_VBI_NORMAL)  \
			pr_crit(SRCCAP_VBI_MDBGFTL_LABEL "[fatal] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_LEVEL_VBI_MASK) >= LOG_LEVEL_VBI_FATAL) \
			pr_emerg(SRCCAP_VBI_MDBGFTL_LABEL "[fatal] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_LEVEL_VBI_ERROR: \
		if ((log_level & LOG_LEVEL_VBI_MASK) == LOG_LEVEL_VBI_NORMAL)  \
			pr_err(SRCCAP_VBI_MDBGERR_LABEL "[error] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_LEVEL_VBI_MASK) >= LOG_LEVEL_VBI_ERROR) \
			pr_emerg(SRCCAP_VBI_MDBGERR_LABEL "[error] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_LEVEL_VBI_WARNING: \
		if ((log_level & LOG_LEVEL_VBI_MASK) == LOG_LEVEL_VBI_NORMAL)  \
			pr_warn(SRCCAP_VBI_MDBGWAR_LABEL "[warning] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_LEVEL_VBI_MASK) >= LOG_LEVEL_VBI_WARNING) \
			pr_emerg(SRCCAP_VBI_MDBGWAR_LABEL "[warning] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_LEVEL_VBI_INFO: \
		if ((log_level & LOG_LEVEL_VBI_MASK) == LOG_LEVEL_VBI_NORMAL)  \
			pr_info(SRCCAP_VBI_MDBGIN_LABEL "[info] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_LEVEL_VBI_MASK) >= LOG_LEVEL_VBI_INFO) \
			pr_emerg(SRCCAP_VBI_MDBGIN_LABEL "[info] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_LEVEL_VBI_DEBUG: \
		if ((log_level & LOG_LEVEL_VBI_MASK) == LOG_LEVEL_VBI_NORMAL)  \
			pr_debug(SRCCAP_VBI_MDBGDBG_LABEL "[debug] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_LEVEL_VBI_MASK) >= LOG_LEVEL_VBI_DEBUG) \
			pr_emerg(SRCCAP_VBI_MDBGDBG_LABEL "[debug] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		default: \
		break; \
		} \
	} while (0)

#define SRCCAP_VBI_MSG_DEBUG(format, args...) \
				SRCCAP_VBI_LOG(LOG_LEVEL_VBI_DEBUG, format, ##args)
#define SRCCAP_VBI_MSG_INFO(format, args...) \
				SRCCAP_VBI_LOG(LOG_LEVEL_VBI_INFO, format, ##args)
#define SRCCAP_VBI_MSG_WARNING(format, args...) \
				SRCCAP_VBI_LOG(LOG_LEVEL_VBI_WARNING, format, ##args)
#define SRCCAP_VBI_MSG_ERROR(format, args...) \
				SRCCAP_VBI_LOG(LOG_LEVEL_VBI_ERROR, format, ##args)
#define SRCCAP_VBI_MSG_FATAL(format, args...) \
				SRCCAP_VBI_LOG(LOG_LEVEL_VBI_FATAL, format, ##args)
#define SRCCAP_VBI_MSG_POINTER_CHECK() \
	do {\
		pr_err(SRCCAP_VBI_MDBGERR_LABEL"[%s,%d] pointer is NULL\n", \
		__func__, __LINE__); \
		dump_stack(); \
	} while (0)


#endif /*__VBI_EX_H__*/
