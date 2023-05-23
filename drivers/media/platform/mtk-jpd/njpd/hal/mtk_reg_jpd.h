/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef _REG_NJPD_H_
#define _REG_NJPD_H_


//-------------------------------------------------------------------------------------------------
// Hardware Capability
//-------------------------------------------------------------------------------------------------
typedef volatile u32 REG16;

typedef struct {
	u32 jpd_bank_num, jpd_ext_bank_num;

	// jpd banks
	REG16 __iomem *bank_jpd;

	// jpd_ex banks
	REG16 __iomem *bank_jpd_ext;

	struct clk *clk_jpd;
	struct clk *sw_en_smi2jpd;
	struct clk *clk_njpd2jpd;

} mtk_jpd_riu_banks;

extern mtk_jpd_riu_banks *jpd_bank_offsets;


//-------------------------------------------------------------------------------------------------
// Macro and Define
//-------------------------------------------------------------------------------------------------
#define NJPD_MEM_SCWGIF_BASE		0x0000
#define NJPD_MEM_SYMIDX_BASE		0x0400
#define NJPD_MEM_QTBL_BASE		0x0800
#define NJPD_MEM_TBL_TOTAL_SIZE	0x1000
#define NJPD_MEM_PROGRESSIVE_INFO_SIZE	0xE000
#define NJPD_MEM_PROGRESSIVE_BUF_SIZE	0x50000

#define NJPD_REG_BASE					(jpd_bank_offsets->bank_jpd)
#define NJPD_EXT_REG_BASE				(jpd_bank_offsets->bank_jpd_ext)

// NJPD1_EXT register=======================================================================

#define BK_NJPD_EXT_GLOBAL_SETTING00				(NJPD_EXT_REG_BASE+0x00)
#define BK_NJPD_EXT_GLOBAL_SETTING01				(NJPD_EXT_REG_BASE+0x01)
#define BK_NJPD_EXT_GLOBAL_SETTING02				(NJPD_EXT_REG_BASE+0x02)


#define BK_NJPD_EXT_MIU_READ_START_8GADDR_0		(NJPD_EXT_REG_BASE+0x10)
#define BK_NJPD_EXT_MIU_READ_START_8GADDR_1		(NJPD_EXT_REG_BASE+0x11)
#define BK_NJPD_EXT_MIU_READ_START_8GADDR_2		(NJPD_EXT_REG_BASE+0x12)

#define BK_NJPD_EXT_MIU_READ_BUFFER0_START_8GADDR_0 (NJPD_EXT_REG_BASE+0x13)
#define BK_NJPD_EXT_MIU_READ_BUFFER0_START_8GADDR_1 (NJPD_EXT_REG_BASE+0x14)
#define BK_NJPD_EXT_MIU_READ_BUFFER0_START_8GADDR_2 (NJPD_EXT_REG_BASE+0x15)

#define BK_NJPD_EXT_MIU_READ_BUFFER0_END_8GADDR_0 (NJPD_EXT_REG_BASE+0x16)
#define BK_NJPD_EXT_MIU_READ_BUFFER0_END_8GADDR_1 (NJPD_EXT_REG_BASE+0x17)
#define BK_NJPD_EXT_MIU_READ_BUFFER0_END_8GADDR_2 (NJPD_EXT_REG_BASE+0x18)

#define BK_NJPD_EXT_MIU_READ_BUFFER1_START_8GADDR_0 (NJPD_EXT_REG_BASE+0x19)
#define BK_NJPD_EXT_MIU_READ_BUFFER1_START_8GADDR_1 (NJPD_EXT_REG_BASE+0x1a)
#define BK_NJPD_EXT_MIU_READ_BUFFER1_START_8GADDR_2 (NJPD_EXT_REG_BASE+0x1b)

#define BK_NJPD_EXT_MIU_READ_BUFFER1_END_8GADDR_0 (NJPD_EXT_REG_BASE+0x1c)
#define BK_NJPD_EXT_MIU_READ_BUFFER1_END_8GADDR_1 (NJPD_EXT_REG_BASE+0x1d)
#define BK_NJPD_EXT_MIU_READ_BUFFER1_END_8GADDR_2 (NJPD_EXT_REG_BASE+0x1e)

#define BK_NJPD_EXT_MIU_WRITE_START_8GADDR_0		(NJPD_EXT_REG_BASE+0x20)
#define BK_NJPD_EXT_MIU_WRITE_START_8GADDR_1		(NJPD_EXT_REG_BASE+0x21)
#define BK_NJPD_EXT_MIU_WRITE_START_8GADDR_2		(NJPD_EXT_REG_BASE+0x22)

#define BK_NJPD_EXT_MIU_HTABLE_START_8GADDR_0	(NJPD_EXT_REG_BASE+0x23)
#define BK_NJPD_EXT_MIU_HTABLE_START_8GADDR_1	(NJPD_EXT_REG_BASE+0x24)
#define BK_NJPD_EXT_MIU_HTABLE_START_8GADDR_2	(NJPD_EXT_REG_BASE+0x25)

#define BK_NJPD_EXT_MIU_GTABLE_START_8GADDR_0	(NJPD_EXT_REG_BASE+0x26)
#define BK_NJPD_EXT_MIU_GTABLE_START_8GADDR_1	(NJPD_EXT_REG_BASE+0x27)
#define BK_NJPD_EXT_MIU_GTABLE_START_8GADDR_2	(NJPD_EXT_REG_BASE+0x28)

#define BK_NJPD_EXT_MIU_QTABLE_START_8GADDR_0	(NJPD_EXT_REG_BASE+0x29)
#define BK_NJPD_EXT_MIU_QTABLE_START_8GADDR_1	(NJPD_EXT_REG_BASE+0x2a)
#define BK_NJPD_EXT_MIU_QTABLE_START_8GADDR_2	(NJPD_EXT_REG_BASE+0x2b)

#define BK_NJPD_EXT_MIU_WRITE_POINTER_8GADDR_0	(NJPD_EXT_REG_BASE+0x30)
#define BK_NJPD_EXT_MIU_WRITE_POINTER_8GADDR_1	(NJPD_EXT_REG_BASE+0x31)
#define BK_NJPD_EXT_MIU_WRITE_POINTER_8GADDR_2	(NJPD_EXT_REG_BASE+0x32)

#define BK_NJPD_EXT_MIU_READ_POINTER_8GADDR_0	(NJPD_EXT_REG_BASE+0x33)
#define BK_NJPD_EXT_MIU_READ_POINTER_8GADDR_1	(NJPD_EXT_REG_BASE+0x34)
#define BK_NJPD_EXT_MIU_READ_POINTER_8GADDR_2	(NJPD_EXT_REG_BASE+0x35)


#define BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_0	(NJPD_EXT_REG_BASE+0x36)
#define BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_1	(NJPD_EXT_REG_BASE+0x37)
#define BK_NJPD_EXT_WRITE_BUF_LBOUND_8GADDR_2	(NJPD_EXT_REG_BASE+0x38)

#define BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_0	(NJPD_EXT_REG_BASE+0x39)
#define BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_1	(NJPD_EXT_REG_BASE+0x3a)
#define BK_NJPD_EXT_WRITE_BUF_UBOUND_8GADDR_2	(NJPD_EXT_REG_BASE+0x3b)

#define BK_NJPD_EXT_8G_SPARE00					(NJPD_EXT_REG_BASE+0x70)
#define BK_NJPD_EXT_8G_SPARE01					(NJPD_EXT_REG_BASE+0x71)
#define BK_NJPD_EXT_8G_SPARE02					(NJPD_EXT_REG_BASE+0x72)
#define BK_NJPD_EXT_8G_SPARE03					(NJPD_EXT_REG_BASE+0x73)


#define NJPD0_EXT_8G_DRAM_ENABLE				NJPD_BIT(0)
#define NJPD0_EXT_8G_WRITE_OVER_8G_BOUND		NJPD_BIT(0)
#define NJPD0_EXT_8G_WRITE_BELOW_8G_BOUND		NJPD_BIT(1)
#define NJPD0_EXT_8G_WRITE_BOUND_ENABLE			NJPD_BIT(0)

// NJPD1 register=======================================================================

// Global Setting
#define BK_NJPD_GLOBAL_SETTING00				(NJPD_REG_BASE+0x00)
#define BK_NJPD_GLOBAL_SETTING01				(NJPD_REG_BASE+0x01)
#define BK_NJPD_GLOBAL_SETTING02				(NJPD_REG_BASE+0x02)

// Pitch Width
#define BK_NJPD_PITCH_WIDTH					(NJPD_REG_BASE+0x03)

// Restart Interval
#define BK_NJPD_RESTART_INTERVAL				(NJPD_REG_BASE+0x05)

// Image Size
#define BK_NJPD_IMG_HSIZE						(NJPD_REG_BASE+0x06)
#define BK_NJPD_IMG_VSIZE						(NJPD_REG_BASE+0x07)

// Write-one-clear
#define BK_NJPD_WRITE_ONE_CLEAR				(NJPD_REG_BASE+0x08)

// Region of Interest
#define BK_NJPD_ROI_H_START					(NJPD_REG_BASE+0x09)
#define BK_NJPD_ROI_V_START					(NJPD_REG_BASE+0x0a)
#define BK_NJPD_ROI_H_SIZE						(NJPD_REG_BASE+0x0b)
#define BK_NJPD_ROI_V_SIZE						(NJPD_REG_BASE+0x0c)

// Gated-Clock Control
#define BK_NJPD_GATED_CLOCK_CTRL				(NJPD_REG_BASE+0x0d)

// Miu Interface
#define BK_NJPD_MIU_READ_STATUS				(NJPD_REG_BASE+0x0e)
#define BK_NJPD_MIU_IBUFFER_CNT				(NJPD_REG_BASE+0x0f)
#define BK_NJPD_MIU_READ_START_ADDR_L			(NJPD_REG_BASE+0x10)
#define BK_NJPD_MIU_READ_START_ADDR_H			(NJPD_REG_BASE+0x11)
#define BK_NJPD_MIU_READ_BUFFER0_START_ADDR_L	(NJPD_REG_BASE+0x12)
#define BK_NJPD_MIU_READ_BUFFER0_START_ADDR_H	(NJPD_REG_BASE+0x13)
#define BK_NJPD_MIU_READ_BUFFER0_END_ADDR_L	(NJPD_REG_BASE+0x14)
#define BK_NJPD_MIU_READ_BUFFER0_END_ADDR_H	(NJPD_REG_BASE+0x15)
#define BK_NJPD_MIU_READ_BUFFER1_START_ADDR_L	(NJPD_REG_BASE+0x16)
#define BK_NJPD_MIU_READ_BUFFER1_START_ADDR_H	(NJPD_REG_BASE+0x17)
#define BK_NJPD_MIU_READ_BUFFER1_END_ADDR_L	(NJPD_REG_BASE+0x18)
#define BK_NJPD_MIU_READ_BUFFER1_END_ADDR_H	(NJPD_REG_BASE+0x19)
#define BK_NJPD_MIU_WRITE_START_ADDR_L			(NJPD_REG_BASE+0x1a)
#define BK_NJPD_MIU_WRITE_START_ADDR_H			(NJPD_REG_BASE+0x1b)
#define BK_NJPD_MIU_WRITE_POINTER_ADDR_L		(NJPD_REG_BASE+0x1c)
#define BK_NJPD_MIU_WRITE_POINTER_ADDR_H		(NJPD_REG_BASE+0x1d)
#define BK_NJPD_MIU_READ_POINTER_ADDR_L		(NJPD_REG_BASE+0x1e)
#define BK_NJPD_MIU_READ_POINTER_ADDR_H		(NJPD_REG_BASE+0x1f)
#define BK_NJPD_MIU_HTABLE_START_ADDR_L		(NJPD_REG_BASE+0x20)
#define BK_NJPD_MIU_HTABLE_START_ADDR_H		(NJPD_REG_BASE+0x21)
#define BK_NJPD_MIU_GTABLE_START_ADDR_L		(NJPD_REG_BASE+0x22)
#define BK_NJPD_MIU_GTABLE_START_ADDR_H		(NJPD_REG_BASE+0x23)
#define BK_NJPD_MIU_QTABLE_START_ADDR_L		(NJPD_REG_BASE+0x24)
#define BK_NJPD_MIU_QTABLE_START_ADDR_H		(NJPD_REG_BASE+0x25)
#define BK_NJPD_MIU_HTABLE_SIZE				(NJPD_REG_BASE+0x26)
#define BK_NJPD_SET_CHROMA_VALUE				(NJPD_REG_BASE+0x27)
#define BK_NJPD_IBUF_READ_LENGTH				(NJPD_REG_BASE+0x28)


// Interrupt
#define BK_NJPD_IRQ_CLEAR						(NJPD_REG_BASE+0x29)
#define BK_NJPD_IRQ_FORCE						(NJPD_REG_BASE+0x2a)
#define BK_NJPD_IRQ_MASK						(NJPD_REG_BASE+0x2b)
#define BK_NJPD_IRQ_FINAL_S					(NJPD_REG_BASE+0x2c)
#define BK_NJPD_IRQ_RAW_S						(NJPD_REG_BASE+0x2d)


// Debug
#define BK_NJPD_ROW_IDEX						(NJPD_REG_BASE+0x30)
#define BK_NJPD_COLUMN_IDEX					(NJPD_REG_BASE+0x31)
#define BK_NJPD_DEBUG_BUS_SELECT				(NJPD_REG_BASE+0x32)
#define BK_NJPD_DEBUG_BUS_H					(NJPD_REG_BASE+0x33)
#define BK_NJPD_DEBUG_BUS_L					(NJPD_REG_BASE+0x34)
#define BK_NJPD_IBUF_BYTE_COUNT_L				(NJPD_REG_BASE+0x35)
#define BK_NJPD_IBUF_BYTE_COUNT_H				(NJPD_REG_BASE+0x36)
#define BK_NJPD_VLD_BYTE_COUNT_L				(NJPD_REG_BASE+0x37)
#define BK_NJPD_VLD_BYTE_COUNT_H				(NJPD_REG_BASE+0x38)
#define BK_NJPD_VLD_DECODING_DATA_L			(NJPD_REG_BASE+0x39)
#define BK_NJPD_VLD_DECODING_DATA_H			(NJPD_REG_BASE+0x3a)
#define BK_NJPD_DEBUG_TRIG_CYCLE				(NJPD_REG_BASE+0x3b)
#define BK_NJPD_DEBUG_TRIG_MBX					(NJPD_REG_BASE+0x3c)
#define BK_NJPD_DEBUG_TRIG_MBY					(NJPD_REG_BASE+0x3d)
#define BK_NJPD_DEBUG_TRIGGER					(NJPD_REG_BASE+0x3e)


// BIST
#define BK_NJPD_BIST_FAIL						(NJPD_REG_BASE+0x3f)


// TBC RIU Interface
#define BK_NJPD_TBC							(NJPD_REG_BASE+0x40)
#define BK_NJPD_TBC_WDATA0						(NJPD_REG_BASE+0x41)
#define BK_NJPD_TBC_WDATA1						(NJPD_REG_BASE+0x42)
#define BK_NJPD_TBC_RDATA_L					(NJPD_REG_BASE+0x43)
#define BK_NJPD_TBC_RDATA_H					(NJPD_REG_BASE+0x44)


// Max Huffman Table Address
#define BK_NJPD_Y_MAX_HUFFTABLE_ADDRESS		(NJPD_REG_BASE+0x45)
#define BK_NJPD_CB_MAX_HUFFTABLE_ADDRESS		(NJPD_REG_BASE+0x46)
#define BK_NJPD_CR_MAX_HUFFTABLE_ADDRESS		(NJPD_REG_BASE+0x47)


// Spare
#define BK_NJPD_SPARE00						(NJPD_REG_BASE+0x48)
#define BK_NJPD_SPARE01						(NJPD_REG_BASE+0x49)
#define BK_NJPD_SPARE02						(NJPD_REG_BASE+0x4a)
#define BK_NJPD_SPARE03						(NJPD_REG_BASE+0x4b)
#define BK_NJPD_SPARE04						(NJPD_REG_BASE+0x4c)
#define BK_NJPD_SPARE05						(NJPD_REG_BASE+0x4d)
#define BK_NJPD_SPARE06						(NJPD_REG_BASE+0x4e)
#define BK_NJPD_SPARE07						(NJPD_REG_BASE+0x4f)

#define BK_NJPD_SPARE07						(NJPD_REG_BASE+0x4f)

#define BK_NJPD_MARB_SETTING_00				(NJPD_REG_BASE+0x50)
#define BK_NJPD_MARB_SETTING_01				(NJPD_REG_BASE+0x51)
#define BK_NJPD_MARB_SETTING_02				(NJPD_REG_BASE+0x52)
#define BK_NJPD_MARB_SETTING_03				(NJPD_REG_BASE+0x53)
#define BK_NJPD_MARB_SETTING_04				(NJPD_REG_BASE+0x54)
#define BK_NJPD_MARB_SETTING_05				(NJPD_REG_BASE+0x55)
#define BK_NJPD_MARB_SETTING_06				(NJPD_REG_BASE+0x56)
#define BK_NJPD_MARB_SETTING_07				(NJPD_REG_BASE+0x57)

#define BK_NJPD_MARB_UBOUND_0_L				(NJPD_REG_BASE+0x58)
#define BK_NJPD_MARB_UBOUND_0_H				(NJPD_REG_BASE+0x59)
#define BK_NJPD_MARB_LBOUND_0_L				(NJPD_REG_BASE+0x5a)
#define BK_NJPD_MARB_LBOUND_0_H				(NJPD_REG_BASE+0x5b)


#define BK_NJPD_CRC_MODE						(NJPD_REG_BASE+0x6d)

// Miu Arbiter
// TODO: MIU Arbiter registers


#define BK_NJPD_MARB_CRC_RESULT_0				(NJPD_REG_BASE+0x70)
#define BK_NJPD_MARB_CRC_RESULT_1				(NJPD_REG_BASE+0x71)
#define BK_NJPD_MARB_CRC_RESULT_2				(NJPD_REG_BASE+0x72)
#define BK_NJPD_MARB_CRC_RESULT_3				(NJPD_REG_BASE+0x73)
#define BK_NJPD_MARB_RESET						(NJPD_REG_BASE+0x74)
#define BK_NJPD_SW_MB_ROW_CNT					(NJPD_REG_BASE+0x75)
#define BK_NJPD_TOP_MARB_PORT_ENABLE			(NJPD_REG_BASE+0x76)

#define BK_NJPD_GENERAL(x)						(NJPD_REG_BASE+x)
#define BK_NJPD_EXT_GENERAL(x)					(NJPD_EXT_REG_BASE+x)


#endif				// _REG_NJPD_H_
