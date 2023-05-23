/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtk_dbg.h  --  Mediatek debug header
 *
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef _MTK_DBG_H
#define _MTK_DBG_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define voc_emerg(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_EMERG) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_alert(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_ALERT) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_crit(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_CRIT)  \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_err(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_ERR)  \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_warn(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_WARNING) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_notice(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_NOTICE) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_info(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_INFO) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_debug(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_DEBUG) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_emerg(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_EMERG) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_alert(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_ALERT) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_crit(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_CRIT) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_err(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_ERR) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_warn(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_WARNING) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_notice(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_NOTICE) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_info(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_INFO) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
#define voc_dev_dbg(dev, fmt, ...)	do {if (mtk_log_level >= LOGLEVEL_DEBUG) \
					dev_emerg(dev, fmt, ##__VA_ARGS__); \
					} while (0)
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------

#endif /* _MTK_DBG_H */
