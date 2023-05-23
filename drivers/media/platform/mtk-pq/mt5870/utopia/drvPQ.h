/* SPDX-License-Identifier: GPL-2.0 */
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020-2024 MediaTek Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MediaTek Inc. and be kept in strict confidence
// ("MediaTek Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MediaTek Confidential
// Information is unlawful and strictly prohibited. MediaTek hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _DRVPQ_H_
#define _DRVPQ_H_

enum PQ_BIN_TYPE {
	PQ_BIN_STD_MAIN,
	PQ_BIN_STD_SUB,
	PQ_BIN_EXT_MAIN,
	PQ_BIN_EXT_SUB,
	PQ_BIN_CUSTOMER_MAIN,
	PQ_BIN_CUSTOMER_SUB,
	PQ_BIN_UFSC,
	PQ_BIN_CF_MAIN,
	PQ_BIN_CF_SUB,
	PQ_BIN_TMO,
	PQ_BIN_HSY,
	MAX_PQ_BIN_NUM,
};

enum PQ_TEXT_BIN_TYPE {
	PQ_TEXT_BIN_STD_MAIN,
	PQ_TEXT_BIN_STD_SUB,
	MAX_PQ_TEXT_BIN_NUM,
};

struct PQ_Bin_Info {
	/// ID
	__u8 u8PQID;
	/// Virtual address
	void *pPQBin_AddrVirt;
	/// Physical address
	size_t PQBin_PhyAddr;
} __packed;

struct MS_PQ_Init_Info {
	/// DDR2
	bool bDDR2;
	///MIU0 mem size
	__u32 u32miu0em_size;
	///MIU1 mem size
	__u32 u32miu1em_size;
	/// DDR Frequency
	__u32 u32DDRFreq;
	/// Bus width
	__u8 u8BusWidth;
	/// Panel width
	__u16 u16PnlWidth;
	/// Panel height
	__u16 u16PnlHeight;
	/// Panel Vtotal
	__u16 u16Pnl_vtotal;
	/// OSD Hsize
	__u16 u16OSD_hsize;
	/// Bin count
	__u8 u8PQBinCnt;
	/// Text Bin count
	__u8 u8PQTextBinCnt;
	/// Customer Bin count
	__u8 u8PQBinCustomerCnt;
	/// PQ Bin informaton array
	__u8 u8PQBinUFSCCnt;
	/// UFSC Bin count
	struct PQ_Bin_Info stPQBinInfo[MAX_PQ_BIN_NUM];
	/// PQ Text bin information array
	struct PQ_Bin_Info stPQTextBinInfo[MAX_PQ_TEXT_BIN_NUM];
	/// CF Bin count
	__u8 u8PQBinCFCnt;
	/// TMO Bin Count
	__u8 u8PQBinTMOCnt;
} __packed;

enum PQ_BIN_PATH {
	/// Customer
	E_PQ_BIN_PATH_CUSTOMER,
	/// Default
	E_PQ_BIN_PATH_DEFAULT,
	/// INI
	E_PQ_BIN_PATH_INI,
	/// Bandwidth
	E_PQ_BIN_PATH_BANDWIDTH,
	/// The max support number of paths
	E_PQ_BIN_PATH_MAX
};

enum MS_PQ_MEMORY_REQUEST_TYPE {
	E_PQ_MEMORY_TMO_VR_REQUEST,	///< memory request for TMO VR
	E_PQ_MEMORY_REQUEST_MAX,	///< Invalid request
};

struct MS_PQ_SET_MEMORY_INFO {
	enum MS_PQ_MEMORY_REQUEST_TYPE eType;
	size_t phyAddress;
	__u32 u32Size;
	__u32 u32HeapID;
} __packed;

enum MS_PQ_HDR_GRULE_INDEX {
	E_PQ_HDR_OFF,
	E_PQ_HDR_OPEN_AUTO,
	E_PQ_HDR_OPEN_HIGH,
	E_PQ_HDR_OPEN_MID,
	E_PQ_HDR_OPEN_LOW,
	E_PQ_HDR_OPEN_REF,
	E_PQ_HDR_DOLBY_VIVID,
	E_PQ_HDR_DOLBY_USER,
	E_PQ_HDR_DOLBY_BRIGHTNESS,
	E_PQ_HDR_DOLBY_DARK,
	E_PQ_HDR_HLG_AUTO,
	E_PQ_HDR_HLG_HIGH,
	E_PQ_HDR_HLG_MID,
	E_PQ_HDR_HLG_LOW,
};


extern void MDrv_PQ_Init(struct MS_PQ_Init_Info *pstPQInitInfo);

extern void MDrv_PQ_SetPQBinPath(
	enum PQ_BIN_PATH ePqBinPath, __u8 u8size,
	char *b_PQBinFilePath);


extern bool MDrv_PQ_SetMemoryAddress(
	struct MS_PQ_SET_MEMORY_INFO *pstPQ_Set_Memory_Address);

extern void MDrv_PQ_SetColorRange(
	__u8 u8Win, bool bColorRange0_255);

extern MS_BOOL MDrv_PQ_LoadTMOModeGRuleTable(
	__u8 u8Win, enum MS_PQ_HDR_GRULE_INDEX eHDRGRuleLevelIndex,
	__u16 u16Width, __u16 u16Height);

#endif				//_DRVPQ_H_

