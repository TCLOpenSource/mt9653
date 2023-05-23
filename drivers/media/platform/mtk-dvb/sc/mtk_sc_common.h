/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef MHAL_SC_COMMON_H
#define MHAL_SC_COMMON_H
//#include "mtk_sc_drv.h"
#include <linux/io.h>
#include <linux/bitops.h>

// hw capability
#define PID_FILTER_NUM			(128)
#define SEC_FILTER_NUM			(128)
#define PCR_FILTER_NUM			(2)
#define STC_NUM					(2)
#define VFIFO_NUM				(2)
#define AFIFO_NUM				(4)
#define PVR_NUM					(2)
#define VQ_NUM					(3)
#define SVQ_NUM					(3)
#define LIVEIN_NUM				(3)
#define MERGE_STREAM_NUM		(8)
#define FILEIN_NUM				(1)
#define FILEIN_CMDQ_NUM			(16)
#define MMFI_NUM				(1)
#define MMFI_FILTER_NUM			(6)
#define MMFI_CMDQ_NUM			(8)
#define FIQ_NUM					(1)
#define TSO_NUM					(1)
//==================================
#define TSIF_0					(0)
#define TSIF_FI					(1)
#define TSIF_1					(2)

#define VFIFO_0					(0)
#define VFIFO_1					(1)

#define AFIFO_0					(0)
#define AFIFO_1					(1)
#define AFIFO_2					(2)
#define AFIFO_3					(3)

#define PVR_1					(0)
#define PVR_2					(1)

#define STC_0					(0)
#define STC_1					(1)

#define PCR_0					(0)
#define PCR_1					(1)

#define TSO_TSIF_1				(0)
#define TSO_TSIF_5				(1)
#define TSO_TSIF_6				(2)

#define MIU_BUS					(4)
#define FQ_PACKET_UNIT_LENGTH	(192)
#define VQ_PACKET_UNIT_LENGTH	(208)

#define MIU0_BUS_BASE			(0x20000000)	// MIU0 Low 256MB
#define MIU1_BUS_BASE			(0xA0000000)	// MIU1 Low 256MB
#define IOVA_START_ADDR			(0x200000000)



#define PA_to_BA(pa)			(pa + MIU0_BUS_BASE)
#define BA_to_PA(ba)			(ba - MIU0_BUS_BASE)
#define HAS_BITS(val, bits)		((val) & (bits))
#define SET_BITS(val, bits)		((val) |= (bits))
#define CLEAR_BITS(val, bits)	((val) &= (~(bits)))

typedef enum {
	E_SC_RST_TO_IO_HW_NOT_SUPPORT = 0x01,
	E_SC_RST_TO_IO_HW_TIMER_SHARE,
	E_SC_RST_TO_IO_HW_TIMER_IND,
	E_SC_RST_TO_IO_HW_INVALID
} SC_RST_TO_IO_HW;

typedef struct {
	SC_RST_TO_IO_HW		  eRstToIoHwFeature;
	u8				 bExtCwtFixed;
} SC_HW_FEATURE;

//typedef volatile u32 REG16;
typedef volatile u8 REG16;

bool send_mcu_cmd(REG16 __iomem *base, u32 cmd, u32 *pmsg0, u32 *pmsg1);
void indirect_write(REG16 __iomem *base, u32 OR_addr, u32 value);
u32 indirect_read(REG16 __iomem *base, u32 OR_addr);
void reg32_write(REG16 __iomem *base, u32 offset, u32 value);
u32 reg32_read(REG16 __iomem *base, u32 offset);
void reg16_write(REG16 __iomem *base, u32 offset, u16 value);
u16 reg16_read(REG16 __iomem *base, u32 offset);
void reg16_set_bit(REG16 __iomem *base, u32 offset, u8 nr);
void reg16_clear_bit(REG16 __iomem *base, u32 offset, u8 nr);
bool reg16_test_bit(REG16 __iomem *base, u32 offset, u8 nr);
void reg16_set_mask(REG16 __iomem *base, u32 offset, u16 mask);
void reg16_clear_mask(REG16 __iomem *base, u32 offset, u16 mask);
void reg16_mask_write(REG16 __iomem *base, u32 offset, u16 mask, u16 value);
u16 reg16_mask_read(REG16 __iomem *base, u32 offset, u16 mask, u16 shift);

#endif				/* MHAL_CI_COMMON_H */
