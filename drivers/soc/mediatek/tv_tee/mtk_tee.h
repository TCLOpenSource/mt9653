/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
 *
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef __MTK_TEE_H__
#define __MTK_TEE_H__

#include <linux/tee_drv.h>

#define TEE_XTEST_SUPPLICANT_CNT_IOC	_IOWR('t', 200, uint32_t)
#define TEE_XTEST_CHECK_SMP_IOC		_IOWR('t', 201, uint32_t)

#endif				/* __MTK_TEE_H__ */
