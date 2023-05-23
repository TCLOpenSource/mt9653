/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __VBI_EX_H__
#define __VBI_EX_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <stdbool.h>

#define VBI_MSG_LEVEL_ERR    (0)
#define VBI_MSG_LEVEL_API    (1)
#define VBI_MSG_LEVEL_DEBUG  (2)
#define VBI_MSG_LEVEL_PARAM  (3)
#define VBI_MSG_LEVEL_ALL    (7)

#define VBI_ERR_MSG(x, args...) \
	do { if (dbglevel >= VBI_MSG_LEVEL_ERR) pr_err(x, ##args); } while (0)
#define VBI_API_MSG(x, args...) \
	do { if (dbglevel >= VBI_MSG_LEVEL_API) pr_err(x, ##args); } while (0)
#define VBI_DBG_MSG(x, args...) \
	do { if (dbglevel >= VBI_MSG_LEVEL_DEBUG) pr_err(x, ##args); } while (0)
#define VBI_PAM_MSG(x, args...) \
	do { if (dbglevel >= VBI_MSG_LEVEL_PARAM) pr_err(x, ##args); } while (0)

extern unsigned int dbglevel;

#endif /*__VBI_EX_H__*/
