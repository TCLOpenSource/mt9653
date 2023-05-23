/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
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
#define audio_emerg(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_EMERG) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define audio_alert(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_ALERT) \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define audio_crit(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_CRIT)  \
					pr_emerg(fmt, ##__VA_ARGS__); \
					} while (0)
#define audio_err(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_ERR)  \
					pr_err(fmt, ##__VA_ARGS__); \
					} while (0)
#define audio_warn(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_WARNING) \
					pr_err(fmt, ##__VA_ARGS__); \
					} while (0)
#define audio_notice(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_NOTICE) \
					pr_err(fmt, ##__VA_ARGS__); \
					} while (0)
#define audio_info(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_INFO) \
					pr_err(fmt, ##__VA_ARGS__); \
					} while (0)
#define audio_debug(fmt, ...)		do {if (mtk_log_level >= LOGLEVEL_DEBUG) \
					pr_err(fmt, ##__VA_ARGS__); \
					} while (0)
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------

#endif /* _MTK_DBG_H */
