/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef MTK_SRCCAP_ATRACE_H
#define MTK_SRCCAP_ATRACE_H

#if IS_ENABLED(CONFIG_MTK_TV_ATRACE)
#include <linux/mtk-tv-atrace.h>
#define SRCCAP_ATRACE_STR_SIZE 64
#define SRCCAP_ATRACE_ASYNC_BEGIN(name, cookie) \
	do {\
		if (atrace_enable_srccap == 1) \
			atrace_async_begin(name, cookie);\
	} while (0)
#define SRCCAP_ATRACE_ASYNC_END(name, cookie) \
	do {\
		if (atrace_enable_srccap == 1) \
			atrace_async_end(name, cookie);\
	} while (0)
#define SRCCAP_ATRACE_BEGIN(name) \
	do {\
		if (atrace_enable_srccap == 1) \
			atrace_begin(name);\
	} while (0)
#define SRCCAP_ATRACE_END(name) \
	do {\
		if (atrace_enable_srccap == 1) \
			atrace_end(name);\
	} while (0)
#define SRCCAP_ATRACE_INT(name, value) \
	do {\
		if (atrace_enable_srccap == 1) \
			atrace_int(name, value);\
	} while (0)
#define SRCCAP_ATRACE_INT_FORMAT(val, format, ...) \
	do {\
		if (atrace_enable_srccap == 1) { \
			char m_str[SRCCAP_ATRACE_STR_SIZE];\
			snprintf(m_str, SRCCAP_ATRACE_STR_SIZE, format, __VA_ARGS__);\
			atrace_int(m_str, val);\
		} \
	} while (0)
#else
#define SRCCAP_ATRACE_ASYNC_BEGIN(name, cookie)
#define SRCCAP_ATRACE_ASYNC_END(name, cookie)
#define SRCCAP_ATRACE_BEGIN(name)
#define SRCCAP_ATRACE_END(name)
#define SRCCAP_ATRACE_INT(name, value)
#define SRCCAP_ATRACE_INT_FORMAT(val, format, ...)
#endif

#endif  // MTK_SRCCAP_ATRACE_H

