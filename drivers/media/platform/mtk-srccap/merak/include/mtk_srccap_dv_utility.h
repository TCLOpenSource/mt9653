/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_DV_UTILITY_H
#define MTK_SRCCAP_DV_UTILITY_H

//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Driver Capability
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macros and Defines
//-----------------------------------------------------------------------------
#define SRCCAP_DV_LABEL "[SRCCAP][DV]"

#define SRCCAP_DV_BIT(_bit_) (1 << (_bit_))

#define SRCCAP_DV_DBG_LEVEL_ERR       SRCCAP_DV_BIT(0)
#define SRCCAP_DV_DBG_LEVEL_INFO      SRCCAP_DV_BIT(1)
#define SRCCAP_DV_DBG_LEVEL_DEBUG     SRCCAP_DV_BIT(2)
#define SRCCAP_DV_DBG_LEVEL_HDMI_DESC SRCCAP_DV_BIT(3)
#define SRCCAP_DV_DBG_LEVEL_HDMI_PKT  SRCCAP_DV_BIT(4)
#define SRCCAP_DV_DBG_LEVEL_PR        SRCCAP_DV_BIT(5)

#define SRCCAP_DV_LOG_TRACE(_dbg_level_, _fmt, _args...) \
{ \
	if (((_dbg_level_) & g_dv_debug_level) != 0) { \
		if ((_dbg_level_) & SRCCAP_DV_DBG_LEVEL_ERR) { \
			pr_err(SRCCAP_DV_LABEL "[Error][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & SRCCAP_DV_DBG_LEVEL_INFO) { \
			pr_err(SRCCAP_DV_LABEL "[Info][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & SRCCAP_DV_DBG_LEVEL_DEBUG) { \
			pr_err(SRCCAP_DV_LABEL "[Debug][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & SRCCAP_DV_DBG_LEVEL_HDMI_DESC) { \
			pr_err(SRCCAP_DV_LABEL "[HDMI descramble][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & SRCCAP_DV_DBG_LEVEL_HDMI_PKT) { \
			pr_err(SRCCAP_DV_LABEL "[HDMI packet][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else if ((_dbg_level_) & SRCCAP_DV_DBG_LEVEL_PR) { \
			pr_err(SRCCAP_DV_LABEL "[PR][%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} else { \
			pr_err(SRCCAP_DV_LABEL "[%s,%5d] "_fmt, \
				__func__, __LINE__, ##_args); \
		} \
	} \
}

#define SRCCAP_DV_LOG_CHECK_POINTER(errno) \
	do { \
		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR, "pointer is NULL!\n");\
		ret = (errno); \
	} while (0)

#define SRCCAP_DV_LOG_CHECK_RETURN(result, errno) \
	do { \
		SRCCAP_DV_LOG_TRACE(SRCCAP_DV_DBG_LEVEL_ERR, "return is %d\n", (result));\
		ret = (errno); \
	} while (0)

#define SRCCAP_DV_PR_LEVEL_DEFAULT      SRCCAP_DV_BIT(0)
#define SRCCAP_DV_PR_LEVEL_DISABLE      SRCCAP_DV_BIT(1)
#define SRCCAP_DV_PR_LEVEL_PRE_SD_OFF   SRCCAP_DV_BIT(2)
#define SRCCAP_DV_PR_LEVEL_PRE_SD_FORCE SRCCAP_DV_BIT(3)

//-----------------------------------------------------------------------------
// Enums and Structures
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables and Functions
//-----------------------------------------------------------------------------
extern int g_dv_debug_level;
extern int g_dv_pr_level;

int mtk_dv_debug_show(
	struct device *dev,
	const char *buf);

int mtk_dv_debug_store(
	struct device *dev,
	const char *buf);

int mtk_dv_debug_checksum(
	__u8 *data,
	__u32 size,
	__u32 *sum);

#endif  // MTK_SRCCAP_DV_UTILITY_H
