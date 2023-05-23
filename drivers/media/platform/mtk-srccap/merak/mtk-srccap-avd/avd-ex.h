/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __AVD_EX_H__
#define __AVD_EX_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <stdbool.h>

#define SRCCAP_AVD_MDBGIN_LABEL "[SRCCAP][mdbgin_avd]"
#define SRCCA_AVD_MDBGERR_LABEL "[SRCCAP][mdbgerr_avd]"
#define SRCCA_AVD_MDBGDBG_LABEL "[SRCCAP][mdbgdbg_avd]"
#define SRCCA_AVD_MDBGWAR_LABEL "[SRCCAP][mdbgwar_avd]"
#define SRCCA_AVD_MDBGFTL_LABEL "[SRCCAP][mdbgftl_avd]"

#define LOG_AVD_LEVEL_MASK    (0xffff)
#define LOG_AVD_LEVEL_NORMAL  (1)
#define LOG_AVD_LEVEL_FATAL   (2)
#define LOG_AVD_LEVEL_ERROR   (3)
#define LOG_AVD_LEVEL_WARNING (4)
#define LOG_AVD_LEVEL_INFO    (5)
#define LOG_AVD_LEVEL_DEBUG   (6)

extern int log_level;
#define SRCCAP_AVD_LOG(_dbgLevel_, _fmt, _args...) \
	do { \
		switch (_dbgLevel_) { \
		case LOG_AVD_LEVEL_FATAL: \
		if ((log_level & LOG_AVD_LEVEL_MASK) == LOG_AVD_LEVEL_NORMAL)  \
			pr_crit(SRCCA_AVD_MDBGFTL_LABEL "[fatal] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_AVD_LEVEL_MASK) >= LOG_AVD_LEVEL_FATAL) \
			pr_emerg(SRCCA_AVD_MDBGFTL_LABEL "[fatal] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_AVD_LEVEL_ERROR: \
		if ((log_level & LOG_AVD_LEVEL_MASK) == LOG_AVD_LEVEL_NORMAL)  \
			pr_err(SRCCA_AVD_MDBGERR_LABEL "[error] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_AVD_LEVEL_MASK) >= LOG_AVD_LEVEL_ERROR) \
			pr_emerg(SRCCA_AVD_MDBGERR_LABEL "[error] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_AVD_LEVEL_WARNING: \
		if ((log_level & LOG_AVD_LEVEL_MASK) == LOG_AVD_LEVEL_NORMAL)  \
			pr_warn(SRCCA_AVD_MDBGWAR_LABEL "[warning] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_AVD_LEVEL_MASK) >= LOG_AVD_LEVEL_WARNING) \
			pr_emerg(SRCCA_AVD_MDBGWAR_LABEL "[warning] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_AVD_LEVEL_INFO: \
		if ((log_level & LOG_AVD_LEVEL_MASK) == LOG_AVD_LEVEL_NORMAL)  \
			pr_info(SRCCAP_AVD_MDBGIN_LABEL "[info] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_AVD_LEVEL_MASK) >= LOG_AVD_LEVEL_INFO) \
			pr_emerg(SRCCAP_AVD_MDBGIN_LABEL "[info] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		case LOG_AVD_LEVEL_DEBUG: \
		if ((log_level & LOG_AVD_LEVEL_MASK) == LOG_AVD_LEVEL_NORMAL)  \
			pr_debug(SRCCA_AVD_MDBGDBG_LABEL "[debug] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		else if ((log_level & LOG_AVD_LEVEL_MASK) >= LOG_AVD_LEVEL_DEBUG) \
			pr_emerg(SRCCA_AVD_MDBGDBG_LABEL "[debug] [%s,%5d]"_fmt, \
			__func__, __LINE__, ##_args); \
		break; \
		default: \
		break; \
		} \
	} while (0)

#define SRCCAP_AVD_MSG_DEBUG(format, args...) \
				SRCCAP_AVD_LOG(LOG_AVD_LEVEL_DEBUG, format, ##args)
#define SRCCAP_AVD_MSG_INFO(format, args...) \
				SRCCAP_AVD_LOG(LOG_AVD_LEVEL_INFO, format, ##args)
#define SRCCAP_AVD_MSG_WARNING(format, args...) \
				SRCCAP_AVD_LOG(LOG_AVD_LEVEL_WARNING, format, ##args)
#define SRCCAP_AVD_MSG_ERROR(format, args...) \
				SRCCAP_AVD_LOG(LOG_AVD_LEVEL_ERROR, format, ##args)
#define SRCCAP_AVD_MSG_FATAL(format, args...) \
				SRCCAP_AVD_LOG(LOG_AVD_LEVEL_FATAL, format, ##args)
#define SRCCAP_AVD_MSG_POINTER_CHECK() \
			do { \
				pr_err(SRCCA_AVD_MDBGERR_LABEL"[%s,%d] pointer is NULL\n", \
				__func__, __LINE__); \
				dump_stack(); \
			} while (0)

#endif /*__AVD_EX_H__*/
