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
#define GET_REG8_ADDR(x, y)               (x+(y)*2-((y)&1))
#define GET_REG16_ADDR(x, y)             (x+(y)*4)
#define GET_REG_OFFSET(x, y)              ((x)*0x200+(y)*0x4)
#define RIU_BASE_ADDR                        riu_base
#define VBDMA_BASE_ADDR                   vbdma_base
// TODO: apply base address from device tree
#define REG_ADDR_BASE_VBDMA          GET_REG8_ADDR(RIU_BASE_ADDR, vbdma_base)
#define REG_ADDR_BASE_MAILBOX       GET_REG8_ADDR(RIU_BASE_ADDR, 0x303000)
#define VBDMA_SET_CH_REG(x)              (GET_REG16_ADDR(REG_ADDR_BASE_VBDMA, x))
#define MAILBOX_SET_CH_REG(x)              (GET_REG16_ADDR(REG_ADDR_BASE_MAILBOX, x))
#define MAILBOX_REG_CTRL           MAILBOX_SET_CH_REG(0x00)
#define VBDMA_REG_CTRL(ch)                VBDMA_SET_CH_REG(0x00 + ch*0x10)
#define VBDMA_REG_STATUS(ch)            VBDMA_SET_CH_REG(0x01 + ch*0x10)
#define VBDMA_REG_DEV_SEL(ch)          VBDMA_SET_CH_REG(0x02 + ch*0x10)
#define VBDMA_REG_MISC(ch)                VBDMA_SET_CH_REG(0x03 + ch*0x10)
#define VBDMA_REG_SRC_ADDR_L(ch)    VBDMA_SET_CH_REG(0x04 + ch*0x10)
#define VBDMA_REG_SRC_ADDR_H(ch)   VBDMA_SET_CH_REG(0x05 + ch*0x10)
#define VBDMA_REG_DST_ADDR_L(ch)    VBDMA_SET_CH_REG(0x06 + ch*0x10)
#define VBDMA_REG_DST_ADDR_H(ch)   VBDMA_SET_CH_REG(0x07 + ch*0x10)
#define VBDMA_REG_SIZE_L(ch)             VBDMA_SET_CH_REG(0x08 + ch*0x10)
#define VBDMA_REG_SIZE_H(ch)             VBDMA_SET_CH_REG(0x09 + ch*0x10)
#define VBDMA_REG_CMD0_L(ch)            VBDMA_SET_CH_REG(0x0A + ch*0x10)
#define VBDMA_REG_CMD0_H(ch)           VBDMA_SET_CH_REG(0x0B + ch*0x10)
#define VBDMA_REG_CMD1_L(ch)            VBDMA_SET_CH_REG(0x0C + ch*0x10)
#define VBDMA_REG_CMD1_H(ch)           VBDMA_SET_CH_REG(0x0D + ch*0x10)
#define VBDMA_REG_CMD2_L(ch)            VBDMA_SET_CH_REG(0x0E + ch*0x10)
#define VBDMA_REG_CMD2_H(ch)           VBDMA_SET_CH_REG(0x0F + ch*0x10)

//---------------------------------------------
// definition for VBDMA_REG_CH0_CTRL/VBDMA_REG_CH1_CTRL
//---------------------------------------------
#define VBDMA_CH_TRIGGER                  BIT(0)
#define VBDMA_CH_STOP                        BIT(4)

//---------------------------------------------
// definition for REG_VBDMA_CH0_STATUS/REG_VBDMA_CH1_STATUS
//---------------------------------------------
#define VBDMA_CH_QUEUED                  BIT(0)
#define VBDMA_CH_BUSY                       BIT(1)
#define VBDMA_CH_INT                          BIT(2)
#define VBDMA_CH_DONE                      BIT(3)
#define VBDMA_CH_RESULT                   BIT(4)
#define VBDMA_CH_CLEAR_STATUS       (VBDMA_CH_INT|VBDMA_CH_DONE|VBDMA_CH_RESULT)

//---------------------------------------------
// definition for REG_VBDMA_CH0_MISC/REG_VBDMA_CH1_MISC
//---------------------------------------------
#define VBDMA_CH_ADDR_DECDIR         BIT(0)
#define VBDMA_CH_DONE_INT_EN         BIT(1)
#define VBDMA_CH_CRC_REFLECTION    BIT(4)
#define VBDMA_CH_MOBF_EN                BIT(5)
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

struct vbdma_data {
	u32 reg_addr;
	u32 reg_length;
};
//------------------------------------------------
// Extern Functions
//------------------------------------------------

// TODO: api prototype need to fix
bool vbdma_copy(bool ch, u32 src, u32 dst, u32 len, vbdma_mode mode);
u32 vbdma_crc32(bool ch, u32 src, u32 len, vbdma_dev dev);
bool vbdma_pattern_search(bool ch, u32 src,
		u32 len, u32 pattern, vbdma_dev dev);
void vbdma_get_riu_base(u8 *riu_map_addr, u32 vbdma_base_addr);
#endif

