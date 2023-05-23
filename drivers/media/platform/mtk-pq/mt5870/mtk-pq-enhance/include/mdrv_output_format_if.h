/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MDRV_OUTPUT_FORMAT_H_
#define _MDRV_OUTPUT_FORMAT_H_

#ifdef _MDRV_OUTPUT_FORMAT_C_
#ifndef INTERFACE
#define INTERFACE
#endif
#else
#ifndef INTERFACE
#define INTERFACE extern
#endif
#endif

#include <linux/types.h>

enum EN_REG_TYPE {
	E_REG_IP,
	E_REG_OP,
	E_REG_VOP,
	E_REG_FO,
	E_REG_MAX,
};

/// Output version of PQ module
enum EN_PQ_CLIENT_TYPE {
	E_PQ_CLIENT_HDR10PLUS,
	E_PQ_CLIENT_GCE,

	E_PQ_CLIENT_MAX,
};

struct STU_Register_Table {
	__u32 u32Depth;
	__u32 *pu32Address;
	__u16 *pu16Value;
	__u16 *pu16Mask;
	__u16 *pu16Client;
};

struct STU_Autodownload_Table {
	__u16 u16client;
	__u8 *pu8Data;
	__u32 u32Size;
};

struct ST_Register_Table {
	__u32 u32Version;
	__u16 u16Length;

	__u32 u32MaxSize;
	__u32 u32UsedSize;
	__u32 *pu32Address;
	__u16 *pu16Value;
	__u16 *pu16Mask;
	__u16 *pu16Client;
};

#undef INTERFACE
#endif		// _MDRV_OUTPUT_FORMAT_H_

