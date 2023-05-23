/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2019 MediaTek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/


#ifndef _DRV_WS_TSF_H_
#define _DRV_WS_TSF_H_
//-------------------------------------------------------------------------------------------------
//  Include files
//-------------------------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/time.h>

//-------------------------------------------------------------------------------------------------
// Define & data type
//-------------------------------------------------------------------------------------------------
#define WS_TSF_IOC_MAGIC                        't'
#define IOCTL_WS_TSF_GET_LATCH_MONOTONIC_RAW      _IOWR(WS_TSF_IOC_MAGIC, 0UL, unsigned long long)
#define IOCTL_WS_TSF_SET_TIME_MEASURE_GPIO      _IOWR(WS_TSF_IOC_MAGIC, 1UL, int)
#define IOCTL_WS_TSF_GET_GPIO_TIME      _IOWR(WS_TSF_IOC_MAGIC, 2UL, WS_GPIO_Params_t)
#define IOCTL_WS_TSF_SET_INTERRUPT_TYPE           _IOWR(WS_TSF_IOC_MAGIC, 3UL, int)
#define WS_TSF_IOCTL_MAXNR                        4ul


#define MTK_WS_TSF_INFO(fmt, args...)    pr_info("[INFO][WS TSF] %s:%d "\
	fmt, __func__, __LINE__, ##args)
#define MTK_WS_TSF_WARN(fmt, args...)    pr_info("[WARN][WS TSF] %s:%d "\
	fmt, __func__, __LINE__, ##args)
#define MTK_WS_TSF_ERR(fmt, args...)     pr_err("[ERR][WS TSF] %s:%d "\
	fmt, __func__, __LINE__, ##args)
#if defined(MTK_WS_TSF_DEBUG)
#define MTK_WS_TSF_DBG(fmt, args...)     pr_info("[INFO][WS TSF] %s:%d "\
	fmt, __func__, __LINE__, ##args)
#else
#define MTK_WS_TSF_DBG(fmt, args...)
#endif

#define MTK_WS_TSF_NODE           "mediatek.ws_tsf"

struct MTK_WS_TSF_Params {
	unsigned int gpio;
	struct timespec ts;
};

enum
{
	TYPE_NONE,
	TYPE_WOW_INTERRUPT,
	TYPE_TSF_INTERRUPT,
};

#endif

