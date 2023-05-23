/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * mtk_dbg.h  --  Mediatek debug header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef _MTK_VOC_DBG_H
#define _MTK_VOC_DBG_H
#define VOC_DEBUG_MODE 0
#if VOC_DEBUG_MODE
#define NONE            "\033[m"
#define RED             "\033[0;32;31m"
#define LIGHT_RED       "\033[1;31m"
#define GREEN           "\033[0;32;32m"
#define LIGHT_GREEN     "\033[1;32m"
#define BLUE            "\033[0;32;34m"
#define LIGHT_BLUE      "\033[1;34m"
#define DARY_GRAY       "\033[1;30m"
#define CYAN            "\033[0;36m"
#define LIGHT_CYAN      "\033[1;36m"
#define PURPLE          "\033[0;35m"
#define LIGHT_PURPLE    "\033[1;35m"
#define BROWN           "\033[0;33m"
#define YELLOW          "\033[1;33m"
#define LIGHT_GRAY      "\033[0;37m"
#define WHITE           "\033[1;37m"

#define LEVEL_ERROR     LIGHT_RED"[VOC ERROR]"NONE
#define LEVEL_INFO      LIGHT_GREEN"[VOC INFO]"NONE
#define LEVEL_DEBUG     LIGHT_BLUE"[VOC DEBUG]"NONE
#define LEVEL_MSG       LIGHT_PURPLE"[VOC MSG]"NONE
#else
#define LEVEL_ERROR
#define LEVEL_INFO
#define LEVEL_DEBUG
#define LEVEL_MSG
#endif
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define voc_emerg(fmt, ...)     do {if (mtk_log_level >= LOGLEVEL_EMERG) \
					pr_emerg(LEVEL_ERROR fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_alert(fmt, ...)     do {if (mtk_log_level >= LOGLEVEL_ALERT) \
					pr_emerg(LEVEL_ERROR fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_crit(fmt, ...)      do {if (mtk_log_level >= LOGLEVEL_CRIT)  \
					pr_emerg(LEVEL_ERROR fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_err(fmt, ...)       do {if (mtk_log_level >= LOGLEVEL_ERR)  \
					pr_emerg(LEVEL_ERROR fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_warn(fmt, ...)      do {if (mtk_log_level >= LOGLEVEL_WARNING) \
					pr_emerg(LEVEL_INFO fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_notice(fmt, ...)        do {if (mtk_log_level >= LOGLEVEL_NOTICE) \
					pr_emerg(LEVEL_INFO fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_info(fmt, ...)      do {if (mtk_log_level >= LOGLEVEL_INFO) \
					pr_emerg(LEVEL_INFO fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_debug(fmt, ...)     do {if (mtk_log_level >= LOGLEVEL_DEBUG) \
					pr_emerg(LEVEL_DEBUG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_emerg(dev, fmt, ...)    do {if (mtk_log_level >= LOGLEVEL_EMERG) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_alert(dev, fmt, ...)    do {if (mtk_log_level >= LOGLEVEL_ALERT) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_crit(dev, fmt, ...) do {if (mtk_log_level >= LOGLEVEL_CRIT) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_err(dev, fmt, ...)  do {if (mtk_log_level >= LOGLEVEL_ERR) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_warn(dev, fmt, ...) do {if (mtk_log_level >= LOGLEVEL_WARNING) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_notice(dev, fmt, ...)   do {if (mtk_log_level >= LOGLEVEL_NOTICE) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_info(dev, fmt, ...) do {if (mtk_log_level >= LOGLEVEL_INFO) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_dbg(dev, fmt, ...)  do {if (mtk_log_level >= LOGLEVEL_DEBUG) \
					dev_emerg(dev, LEVEL_MSG fmt, ##__VA_ARGS__); \
					} while (0)
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
extern unsigned long mtk_log_level;
#endif /* _MTK_VOC_DBG_H */

