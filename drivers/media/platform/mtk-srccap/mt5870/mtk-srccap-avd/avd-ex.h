/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __AVD_EX_H__
#define __AVD_EX_H__

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <stdbool.h>

#define AVD_MSG_LEVEL_ERR    (0)
#define AVD_MSG_LEVEL_API    (1)
#define AVD_MSG_LEVEL_DEBUG  (2)
#define AVD_MSG_LEVEL_PARAM  (3)
#define AVD_MSG_LEVEL_ALL    (7)
#define AVD_LABEL "[AVD] "


#define AVD_ERR_MSG(x, args...) \
	do {\
		if (dbglevel >= AVD_MSG_LEVEL_ERR) \
			pr_err(x, ##args);\
	} while (0)

#define AVD_API_MSG(x, args...) \
	do {\
		if (dbglevel >= AVD_MSG_LEVEL_API) \
			pr_info(x, ##args);\
	} while (0)

#define AVD_DBG_MSG(x, args...) \
	do {\
		if (dbglevel >= AVD_MSG_LEVEL_DEBUG) \
			pr_debug(x, ##args);\
	} while (0)

#define AVD_PAM_MSG(x, args...) \
	do {\
		if (dbglevel >= AVD_MSG_LEVEL_PARAM) \
			pr_info(x, ##args);\
	} while (0)

extern unsigned int dbglevel;

#endif /*__AVD_EX_H__*/
