/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */


#ifndef _DRV_NJPD_H_
#define _DRV_NJPD_H_

#include "mtk_njpeg_def.h"
#include "mtk_reg_jpd.h"
#include "mtk_hal_jpd.h"


#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------
// Macro and Define
//-------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// @brief \b NJPD_DRV_VERSION : NJPEG Version
//------------------------------------------------------------------------------

#define NJPD_BIT(_bit_)    (1<<(_bit_))
#define NJPD_BITMASK(_bits_) (NJPD_BIT(((1) ? _bits_)+1)-NJPD_BIT(((0) ? _bits_)))

/*****************Config Flag*********************/

// detail for reg: BK_NJPD_GLOBAL_SETTING00 (0x00)
#define NJPD_Y_HSF1                    NJPD_BIT(0)
#define NJPD_Y_HSF2                    NJPD_BIT(1)
#define NJPD_Y_VSF2                    NJPD_BIT(2)
#define NJPD_UV                        NJPD_BIT(3)
#define NJPD_SWRST                     NJPD_BIT(4)
#define NJPD_RST_EN                    NJPD_BIT(5)
#define NJPD_ROI_EN                    NJPD_BIT(6)
#define NJPD_SUVQ                      NJPD_BIT(7)
#define NJPD_SUVH                      NJPD_BIT(8)
#define NJPD_YC_SWAP                   NJPD_BIT(10)
#define NJPD_UV_SWAP                   NJPD_BIT(11)


// detail for reg: BK_NJPD_GLOBAL_SETTING01 (0x01)
#define NJPD_DOWN_SCALE                (NJPD_BIT(0) | NJPD_BIT(1))
#define NJPD_SVLD                      NJPD_BIT(6)
#define NJPD_UV_7BIT                   NJPD_BIT(8)
#define NJPD_UV_MSB                    NJPD_BIT(9)
#define NJPD_GTABLE_RST                NJPD_BIT(10)
#define NJPD_HTABLE_RELOAD_EN          NJPD_BIT(11)
#define NJPD_GTABLE_RELOAD_EN          NJPD_BIT(12)
#define NJPD_QTABLE_RELOAD_EN          NJPD_BIT(13)

// detail for reg: BK_NJPD_GLOBAL_SETTING02 (0x02)
#define NJPD_TBC_MODE                  NJPD_BIT(1)
#define NJPD_FFD9_EN                   NJPD_BIT(2)
#define NJPD_LITTLE_ENDIAN             NJPD_BIT(8)
#define NJPD_REMOVE_0xFF00             NJPD_BIT(9)
#define NJPD_REMOVE_0xFFFF             NJPD_BIT(10)
#define NJPD_HUFF_TABLE_ERROR          NJPD_BIT(11)
#define NJPD_HUFF_DATA_LOSS_ERROR      NJPD_BIT(12)
#define NJPD_BITSTREAM_LE              NJPD_BIT(13)
#define NJPD_SRAM_SD_EN                NJPD_BIT(15)


// detail for reg: BK_NJPD_WRITE_ONE_CLEAR (0x08)
#define NJPD_DECODE_ENABLE             NJPD_BIT(0)
#define NJPD_TABLE_LOADING_START       NJPD_BIT(1)
#define NJPD_MIU_PARK                  NJPD_BIT(2)
#define NJPD_MRC0_VALID                NJPD_BIT(3)
#define NJPD_MRC1_VALID                NJPD_BIT(4)
#define NJPD_MRC_LAST                  NJPD_BIT(5)
#define NJPD_TBC_EN                    NJPD_BIT(6)
#define NJPD_TBC_DONE_CLR              NJPD_BIT(7)
#define NJPD_CLEAR_CRC                 NJPD_BIT(8)
#define NJPD_HANDSHAKE_SW_WOC          NJPD_BIT(9)


// detail for reg: BK_NJPD_MIU_READ_STATUS (0x0e)
#define NJPD_MIU_MRC0_STATUS           NJPD_BIT(0)
#define NJPD_MIU_MRC1_STATUS           NJPD_BIT(1)
#define NJPD_MIU_HTABLE_RDY            NJPD_BIT(2)
#define NJPD_MIU_GTABLE_RDY            NJPD_BIT(3)
#define NJPD_MIU_QTABLE_RDY            NJPD_BIT(4)
#define NJPD_MIU_PACK_SHIFT            8
#define NJPD_MIU_MIU_PACK            \
	(NJPD_BIT(NJPD_MIU_PACK_SHIFT) | NJPD_BIT(NJPD_MIU_PACK_SHIFT+1))

// detail for reg: BK_NJPD1_IBUF_READ_LENGTH (0x28)
#define NJPD_MIU_SEL_SHIFT            10
#define NJPD_MIU_SEL_1_SHIFT            11
#define NJPD_MIU_SEL                   NJPD_BIT(NJPD_MIU_SEL_SHIFT)
#define NJPD_MIU_SEL_1                 NJPD_BIT(NJPD_MIU_SEL_1_SHIFT)

// detail for reg: BK_NJPD_IRQ_CLEAR (0x29)
//[0] Decode done event flag
//[1] Mini-code error event flag
//[2] Inverse scan error event flag
//[3] Restart marker error event flag
//[4] Restart marker index disorder error event flag
//[5] End image error event flag
//[6] Read buffer0 empty event flag
//[7] Read buffer1 empty event flag
//[8] MIU write protect event flag
//[9] Data lose error event flag
//[10] iBuf table load done flag
//[11] Huffman table error

#define NJPD_EVENT_DECODE_DONE         NJPD_BIT(0)
#define NJPD_EVENT_MINICODE_ERR        NJPD_BIT(1)
#define NJPD_EVENT_INV_SCAN_ERR        NJPD_BIT(2)
#define NJPD_EVENT_RES_MARKER_ERR      NJPD_BIT(3)
#define NJPD_EVENT_RMID_ERR            NJPD_BIT(4)
#define NJPD_EVENT_END_IMAGE_ERR       NJPD_BIT(5)
#define NJPD_EVENT_MRC0_EMPTY          NJPD_BIT(6)
#define NJPD_EVENT_MRC1_EMPTY          NJPD_BIT(7)
#define NJPD_EVENT_WRITE_PROTECT       NJPD_BIT(8)
#define NJPD_EVENT_DATA_LOSS_ERR       NJPD_BIT(9)
#define NJPD_EVENT_IBUF_LOAD_DONE      NJPD_BIT(10)
#define NJPD_EVENT_HUFF_TABLE_ERR      NJPD_BIT(11)
#define NJPD_EVENT_ALL	(NJPD_EVENT_DECODE_DONE | NJPD_EVENT_MINICODE_ERR | \
			NJPD_EVENT_INV_SCAN_ERR | NJPD_EVENT_RES_MARKER_ERR | \
			NJPD_EVENT_RMID_ERR |    NJPD_EVENT_END_IMAGE_ERR | \
			NJPD_EVENT_MRC0_EMPTY | NJPD_EVENT_MRC1_EMPTY |    \
			NJPD_EVENT_WRITE_PROTECT | NJPD_EVENT_DATA_LOSS_ERR |    \
			NJPD_EVENT_IBUF_LOAD_DONE | NJPD_EVENT_HUFF_TABLE_ERR)

#define NJPD_EVENT_ERROR	(NJPD_EVENT_MINICODE_ERR | NJPD_EVENT_INV_SCAN_ERR | \
			NJPD_EVENT_RES_MARKER_ERR | NJPD_EVENT_RMID_ERR | \
			NJPD_EVENT_END_IMAGE_ERR | NJPD_EVENT_DATA_LOSS_ERR |\
			NJPD_EVENT_HUFF_TABLE_ERR)


// detail for reg: BK_NJPD_TBC (0x40)
#define NJPD_JPD_TBC_RW     NJPD_BIT(0)
#define NJPD_JPD_TBC_SEL    (NJPD_BIT(1) | NJPD_BIT(2))
#define NJPD_JPD_TBC_TABLE_READ    NJPD_BIT(4)
#define NJPD_JPD_TBC_ADR    (NJPD_BIT(8) | NJPD_BIT(9) | NJPD_BIT(10) \
	| NJPD_BIT(11) | NJPD_BIT(12) | NJPD_BIT(13) | NJPD_BIT(14) | NJPD_BIT(15))


// detail for reg: BK_NJPD2_MARB_SETTING_06 (0x56)
#define NJPD_JPD_MARB_MRPRIORITY_SW    (NJPD_BIT(0) | NJPD_BIT(1))

// detail for reg: BK_NJPD2_MARB_SETTING_07 (0x57)
#define NJPD_JPD_MARB_BURST_SPLIT      (NJPD_BIT(12) | NJPD_BIT(13))


// detail for reg: BK_NJPD_MARB_LBOUND_0_H (0x5b)
#define NJPD_MARB_MIU_BOUND_EN_0        NJPD_BIT(13)



// detail for reg: BK_NJPD_TOP_MARB_PORT_ENABLE (0x76)
#define NJPD_TOP_MARB_P0_ENABLE                    NJPD_BIT(0)
#define NJPD_TOP_MARB_P1_ENABLE                    NJPD_BIT(1)
#define NJPD_TOP_MARB_P2_ENABLE                    NJPD_BIT(2)
#define NJPD_TOP_MARB_P0_W_BYPASS_ENABLE           NJPD_BIT(4)
#define NJPD_TOP_MARB_P1_W_BYPASS_ENABLE           NJPD_BIT(5)
#define NJPD_TOP_MARB_P2_W_BYPASS_ENABLE           NJPD_BIT(6)
#define NJPD_TOP_MARB_P0_R_BYPASS_ENABLE           NJPD_BIT(7)
#define NJPD_TOP_MARB_P1_R_BYPASS_ENABLE           NJPD_BIT(0)	// second byte
#define NJPD_TOP_MARB_P2_R_BYPASS_ENABLE           NJPD_BIT(1)	// second byte

//-------------------------------------------------------------------------------------------------
// Type and Structure
//-------------------------------------------------------------------------------------------------

	typedef enum {
		E_NJPD_EVENT_DEC_NONE = 0x00,
		E_NJPD_EVENT_DEC_DONE = 0x01,
		E_NJPD_EVENT_MINICODE_ERR = 0x02,
		E_NJPD_EVENT_INV_SCAN_ERR = 0x04,
		E_NJPD_EVENT_RES_MARKER_ERR = 0x08,
		E_NJPD_EVENT_RMID_ERR = 0x10,
		E_NJPD_EVENT_END_IMAGE_ERR = 0x20,
		E_NJPD_EVENT_MRC0_EMPTY = 0x40,
		E_NJPD_EVENT_MRC1_EMPTY = 0x80,
		E_NJPD_EVENT_WRITE_PROTECT = 0x100,
		E_NJPD_EVENT_DATA_LOSS_ERR = 0x200,
		E_NJPD_EVENT_IBUF_LOAD_DONE = 0x400,
		E_NJPD_EVENT_HUFF_TABLE_ERR = 0x800
	} NJPD_Event;


// NJPD Downscale Ratio
// Bellows are 1, 1/2, 1/4 and 1/8 in order
	typedef enum {
		E_NJPD_DOWNSCALE_ORG = 0x00,
		E_NJPD_DOWNSCALE_HALF = 0x01,
		E_NJPD_DOWNSCALE_FOURTH = 0x02,
		E_NJPD_DOWNSCALE_EIGHTH = 0x03
	} NJPD_DownScale;


// NJPD debug level enum
	typedef enum {
		E_NJPD_DEBUG_DRV_ERR = 0x0,
		E_NJPD_DEBUG_DRV_MSG = 0x02
	} NJPD_DrvDbgLevel;

	typedef enum {
		E_NJPD_FAILED = 0,
		E_NJPD_OK = 1,
		E_NJPD_INVALID_PARAM = 2
	} NJPD_Return;

	typedef enum {
		E_SCALING_ORG = 0,
		E_SCALING_1_2 = 1,
		E_SCALING_1_4 = 2,
		E_SCALING_1_8 = 3
	} NJPD_SCALING_DOWN_FACTOR;



// NJPD Interrupt Register Function
	typedef void (*NJPD_IsrFuncCb) (void);

//------------------------------------------------------------------------------
// Structure for buffer
	typedef struct {
		__u64 u64MRCBufAddr;	// MRC buffer address
		__u32 u32MRCBufSize;	// MRC buffer size
		__u32 u32MRCBufOffset;	//  relative to MRC start address
		__u64 u64MWCBufAddr;	// MWC buffer address
		__u16 u16MWCBufLineNum;	// MWC Line number
		//__u8 bProgressive;
	} NJPD_BufCfg;

//------------------------------------------------------------------------------
// Structure for NJPD capability
	typedef struct {
		__u8 bBaseline;
		__u8 bProgressive;
		__u8 bMJPEG;
	} NJPD_Cap;
//------------------------------------------------------------------------------
// Structure for NJPD Status
	typedef struct {
		__u32 u32CurMRCAddr;
		__u16 u16CurVidx;
		__u16 u16CurRow;
		__u16 u16CurCol;
		__u8 bIsBusy;
		__u8 bIsrEnable;
	} NJPD_Status;

//-------------------------------------------------------------------------------------------------
// Function and Variable
//-------------------------------------------------------------------------------------------------
	void MDrv_NJPD_SetRIUBank(mtk_jpd_riu_banks *riu_banks);
	__u8 MDrv_NJPD_InitBuf(NJPD_BufCfg in);
	__u8 MDrv_NJPD_SetReadBuffer(__u64 u32BufAddr, __u32 u32BufSize, __u8 index);
	void MDrv_NJPD_Rst(void);
	__u32 MDrv_NJPD_GetMWCStartAddr(void);
	__u32 MDrv_NJPD_GetWritePtrAddr(void);
	void MDrv_NJPD_SetPicDimension(__u16 u16Width, __u16 u16Height);
	__u16 MDrv_NJPD_GetEventFlag(void);
	void MDrv_NJPD_SetEventFlag(__u16 u16Value);
	void MDrv_NJPD_SetROI(__u16 start_x, __u16 start_y, __u16 width, __u16 height);
	void MDrv_NJPD_PowerOn(void);
	void MDrv_NJPD_PowerOff(void);
	void MDrv_NJPD_SetRSTIntv(__u16 u16Value);
	__u16 MDrv_NJPD_GetCurVidx(void);

	void MDrv_NJPD_SetDbgLevel(__u8 u8DbgLevel);
	NJPD_Status *MDrv_NJPD_GetStatus(void);

	__u8 MDrv_NJPD_EnableISR(NJPD_IsrFuncCb IsrCb);
	__u8 MDrv_NJPD_DisableISR(void);

	__u32 MDrv_NJPD_GetCurMRCAddr(void);
	__u32 MDrv_NJPD_GetCurMWCAddr(void);
	__u16 MDrv_NJPD_GetCurRow(void);
	__u16 MDrv_NJPD_GetCurCol(void);

	void MDrv_NJPD_SetAutoProtect(__u8 enable);
	void MDrv_NJPD_SetWPENEndAddr(__u64 u32ByteOffset);
	void MDrv_NJPD_SetWPENStartAddr(__u64 u32ByteOffset);

	void MDrv_NJPD_SetSpare(__u16 u16Value);
	__u16 MDrv_NJPD_GetSpare(void);
	void MDrv_NJPD_SetMRC_Valid(__u8 u8Index);

	void MDrv_NJPD_DecodeEnable(void);
	void MDrv_NJPD_TableLoadingStart(void);
	void MDrv_NJPD_ReadLastBuffer(void);
	void MDrv_NJPD_SetScalingDownFactor(NJPD_SCALING_DOWN_FACTOR eScalingFactor);

	void MDrv_NJPD_GTable_Rst(__u8 bEnable);
	void MDrv_NJPD_HTable_Reload_Enable(__u8 bEnable);
	void MDrv_NJPD_GTable_Reload_Enable(__u8 bEnable);
	void MDrv_NJPD_QTable_Reload_Enable(__u8 bEnable);
	void MDrv_NJPD_SetSoftwareVLD(__u8 bEnable);
	void MDrv_NJPD_SetDifferentQTable(__u8 bEnable);
	void MDrv_NJPD_SetDifferentHTable(__u8 bEnable);
	void MDrv_NJPD_Set_GlobalSetting00(__u16 u16Value);
	__u16 MDrv_NJPD_Get_GlobalSetting00(void);
	void MDrv_NJPD_SetHTableStartAddr(__u64 u32ByteOffset);
	void MDrv_NJPD_SetHTableSize(__u16 u6Size);
	void MDrv_NJPD_SetQTableStartAddr(__u64 u32ByteOffset);
	void MDrv_NJPD_SetGTableStartAddr(__u64 u32ByteOffset);
	void MDrv_NJPD_Debug(void);
	void MDrv_NJPD_EnablePowerSaving(void);
	void MDrv_NJPD_SetOutputFormat(__u8 bRst, NJPD_OutputFormat eOutputFormat);
	void MDrv_NJPD_SetIBufReadLength(__u8 u8Min, __u8 u8Max);

#ifdef __cplusplus
}
#endif
#endif				// _DRV_NJPD_H_
