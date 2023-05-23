/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

// TODO: temp solution, it should be fixed to linux coding style next iteration
#ifndef _VBDMA_H
#define _VBDMA_H

//----------------------------------------------
// Defines
//----------------------------------------------

//----------------------------------------------
// Macros
//----------------------------------------------

//----------------------------------------------
// Type and Structure Declaration
//----------------------------------------------

// TODO: enum prototype need to fix
typedef enum {
	E_VBDMA_CPtoHK,
	E_VBDMA_HKtoCP,
	E_VBDMA_CPtoCP,
	E_VBDMA_HKtoHK,
} vbdma_mode;

// TODO: enum prototype need to fix
typedef enum {
	E_VBDMA_DEV_MIU,
	E_VBDMA_DEV_IMI
} vbdma_dev;

//------------------------------------------------
// Extern Functions
//------------------------------------------------

// TODO: api prototype need to fix
//bool vbdma_copy(bool ch, u32 src, u32 dst, u32 len, vbdma_mode mode);
u32 vbdma_crc32(bool ch, u32 src, u32 src_H, u32 len, vbdma_dev dev);
//bool vbdma_pattern_search(bool ch, u32 src,
//		u32 len, u32 pattern, vbdma_dev dev);
void vbdma_get_base(void __iomem *vbdma_base_addr);
#endif

