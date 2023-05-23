/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//-------------------------------------------------
/*! \defgroup VBI VBI interface
 *   \ingroup  VBI

 *   <b> Operation Code Flow: </b> \n
 *   \image html vbi1.png
 *   \image html vbi2.png

 *! \defgroup VBI_General VBI general functions
 *   \ingroup   VBI

 *! \defgroup VBI_Task VBI process functions.
 *   \ingroup   VBI

 *! \defgroup VBI_TTX VBI TTX related functions
 *   \ingroup   VBI

 *! \defgroup VBI_CC VBI CC related functions
 *   \ingroup   VBI

 *! \defgroup VBI_STR VBI STR functions
 *   \ingroup   VBI

 *! \defgroup VBI_ToBeModified VBI api to be modified
 *   \ingroup   VBI

 *! \defgroup VBI_TTX_ToBeModified VBI TTX api to be modified
 *   \ingroup   VBI_ToBeModified

 *! \defgroup VBI_ToBeRemove VBI api to be removed
 *   \ingroup   VBI
 */
//-------------------------------------------------

#ifndef _DRV_VBI_H
#define _DRV_VBI_H

//#include "MsTypes.h"
//#include "MsDevice.h"
#include "sti_msos.h"


//#include "ULog.h"
//#include "UFO.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define VBI_UTOPIA20 (1)

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define VBI_TTX_DATA_LENGTH 48

#define MSIF_VBI_LIB_CODE                   {'V', 'B', 'I', '_'} //Lib code
#define MSIF_VBI_LIBVER                        {'0', '0'}      //LIB version
#define MSIF_VBI_BUILDNUM                 {'0', '2' }    //Build Number
#define MSIF_VBI_CHANGELIST         {'0', '0', '5', '6', '5', '9', '8', '0'} //P4 ChangeList Number

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
typedef enum {
	VBI_VIDEO_SECAM,
	VBI_VIDEO_PAL_NC,
	VBI_VIDEO_PAL_M,
	VBI_VIDEO_NTSC,
	VBI_VIDEO_NTSC443_PAL60,
	VBI_VIDEO_PAL,
	VBI_VIDEO_OTHERS
} VBI_VIDEO_STANDARD;

typedef enum {
	E_VBI_TELETEXT,
	E_VBI_CC,
	E_VBI_WSS
} VBI_INIT_TYPE;

typedef struct {
	MS_U8   u8NoInfo;
} VBI_DrvInfo;

typedef struct DLL_PACKED {
	VBI_INIT_TYPE   eInitType;
} VBI_DrvStatus;

typedef struct DLL_PACKED {
	MS_U8 ptable[7];
} VBI_CC_TABLE;

typedef struct DLL_PACKED {
	MS_BOOL _bSuspend;
	VBI_INIT_TYPE eInitType;
	MS_VIRT _ptrVBIRiuBaseAddr;

	// TTX buffer address & buffer size
	MS_PHY  _TTXBufAddr;
	MS_U16  _TTXBufLen;

	// CC buffer address & buffer size
	MS_PHY  _CCBufAddr;
	MS_U16  _CCBufLen;

	// callback info
	MS_PHY  _cbBufferAddr;
	MS_U32  _cbBufferLength;
	MS_U8  *_pCbVABuff;

	MS_BOOL  _bTTXSupported;
	MS_BOOL  _bVBIInit;
	MS_U16  _u16VBIDbgSwitch;
} VBI_StoreInfo;

/*
 * ClosedCaption
 */
typedef enum {
	VBI_CC_PACKET_COUNT,
	VBI_CC_BYTE_FOUND_INDICATION,
	VBI_CC_DATA_GET,
	VBI_CC_PACKET_SIZE,

	VBI_TTX_PACKET_SIZE = 0x100
} EN_VBI_INFO;

/* Error code */
typedef enum {
	VBI_ERRORCODE_SUCCESS            = 0x0000,
	VBI_ERRORCODE_FAIL               = 0x0001

} EN_VBI_ERRORCODE;

typedef enum {
	EN_TTX_DMA_HEADER,
	EN_TTX_DMA_PACKET1_TO_25,
	EN_TTX_DMA_PACKET26_28_29,
	EN_TTX_DMA_PACKET27,
	EN_TTX_DMA_BTT,
	EN_TTX_DMA_AIT,
} EN_TTX_DMA_TYPE;

typedef enum {
	VBI_INIT,
	VBI_EXIT,
	VBI_SUSPEND,
	VBI_RESUME,
} EN_VBI_CMD;

typedef struct {
	MS_S32 ReadPacketNumber;
	MS_S32 WritePacketNumber;
	MS_U16 NoOfPacketBufferOverflows;

	MS_BOOL pkt_received;
	MS_BOOL PacketBufferOverflow;
} VBI_RING_BUFFER;


typedef MS_U32(*VBI_CB_FN)(MS_U8 *pVBILine, MS_U32 length);
//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

// must have functions
//-------------------------------------------------------------------------------------------------
/// Get VBI lib version.
/// @ingroup VBI_General
/// @param  pFN             \b OUT: return vbi lib version.
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_GetLibVer(const MSIF_Version **ppVersion); ///< Get version (without Mutex protect)
//-------------------------------------------------------------------------------------------------
/// Get VBI module info.
/// @ingroup VBI_General
/// @return VBI_DrvInfo
//-------------------------------------------------------------------------------------------------
const VBI_DrvInfo *MDrv_VBI_GetInfo(void);        ///< Get info from driver (without Mutex protect)
//-------------------------------------------------------------------------------------------------
/// Get VBI Driver Status.
/// @ingroup VBI_General
/// @param  pDrvStatus    \b OUT: driver status.
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_GetStatus(VBI_DrvStatus *pDrvStatus);   ///< Get VBI current status
//-------------------------------------------------------------------------------------------------
/// Set VBI Debug Level.
/// @ingroup VBI_General
/// @param  u16DbgSwitch    \b IN: debug level.
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_SetDbgLevel(MS_U16 u16DbgSwitch);      ///< Set debug level (without Mutex protect)

//-----------------
// VBI_General
//-----------------
//-------------------------------------------------------------------------------------------------
/// Initialize VBI module
/// @ingroup VBI_General
/// @param  type      \b IN: VBI type (Teletext/CC)
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_Init(VBI_INIT_TYPE type);
//-------------------------------------------------------------------------------------------------
/// Finalize VBI module
/// @ingroup VBI_General
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_Exit(EN_VBI_CMD cmd);

//-------------------------------------------------------------------------------------------------
/// Set Callback to VBI driver, the CB will be called if VBI interrupt is catched
/// @ingroup VBI_Task
/// @param  pFN              \b IN: call back function pointer.
/// @param  bufferAddr    \b IN: the physical address of buffer which will store VBI data
///                              and pass pointer to CB
/// @param  length           \b IN: the data length of th input buffer.
///                             Note: it should be the multiple of VBI_TTX_DATA_LENGTH
/// @return None
//-------------------------------------------------------------------------------------------------
#if defined(UFO_PUBLIC_HEADER_300)
void MDrv_VBI_RegisterCB(VBI_CB_FN pFN, MS_PHYADDR bufferAddr, MS_U32 length);
#else
void MDrv_VBI_RegisterCB(VBI_CB_FN pFN, MS_PHY bufferAddr, MS_U32 length);
#endif
//-------------------------------------------------------------------------------------------------
/// Reset TTX Ring Buffer
/// @ingroup VBI_Task
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_RingBuffer_Reset(void);

//-------------------------------------------------------------------------------------------------
/// Initialize TTX Slicer.
/// @ingroup VBI_TTX
/// @param  bufferAddr     \b IN: TTX buffer address.
/// @param  packetCount  \b IN: TTX packet count.
/// @return None
//-------------------------------------------------------------------------------------------------
#if defined(UFO_PUBLIC_HEADER_300)
void MDrv_VBI_InitializeTTXSlicer(MS_PHYADDR bufferAddr, MS_U16 packetCount);
#else
void MDrv_VBI_InitializeTTXSlicer(MS_PHY bufferAddr, MS_U16 packetCount);
#endif
//-------------------------------------------------------------------------------------------------
/// Enable TTX Slicer.
/// @ingroup VBI_TTX
/// @param bEnable     \b IN: enable or disable TTX slicer.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_EnableTTXSlicer(MS_BOOL bEnable);

//-------------------------------------------------------------------------------------------------
/// VPS Is Ready or Not. (Refine function name: MDrv_VBI_TTX_IsVPS_Ready)
/// @ingroup VBI_TTX_ToBeModified
/// @return TRUE  : VPS is ready.
/// @return FALSE : VPS is not ready.
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_IsVPS_Ready(void);

//-------------------------------------------------------------------------------------------------
/// TTX Is Ready or Not.
/// @ingroup VBI_TTX
/// @return TRUE  : TTX is ready.
/// @return FALSE : TTX is not ready.
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_IsTTX_Ready(void);

//-------------------------------------------------------------------------------------------------
/// WSS Is Ready or Not. (Refine function name: MDrv_VBI_TTX_IsWSS_Ready)
/// @ingroup VBI_TTX_ToBeModified
/// @return TRUE  : WSS is ready.
/// @return FALSE : WSS is not ready.
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_IsWSS_Ready(void);

//-------------------------------------------------------------------------------------------------
/// Get WSS Data. (Refine function name: MDrv_VBI_TTX_GetWSS_Data)
/// @ingroup VBI_TTX_ToBeModified
/// @return : WSS data
//-------------------------------------------------------------------------------------------------
MS_U16 MDrv_VBI_GetWSS_Data(void);

//-------------------------------------------------------------------------------------------------
/// Get VPS Data. (Refine function name: MDrv_VBI_TTX_GetVPS_Data)
/// @ingroup VBI_TTX_ToBeModified
/// @param  lowerWord    \b OUT: VPS lower data.
/// @param  higherWord   \b OUT: VPS higher data.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_GetVPS_Data(MS_U8 *lowerWord, MS_U8 *higherWord);

//-------------------------------------------------------------------------------------------------
/// Get Complete VPS Data.
/// @ingroup VBI_TTX_ToBeModified
/// @param  pBuffer    \b IN/OUT: VPS buffer.
/// @param  dataLen   \b IN: buffer size.
/// @return None
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_GetCompleteVPS_Data(MS_U8 **pBuffer, MS_U32 *dataLen);

//-------------------------------------------------------------------------------------------------
/// Set Video Standard. (Refine function name: MDrv_VBI_TTX_SetVideoStandard)
/// @ingroup VBI_TTX_ToBeModified
/// @param  lowerWord    \b IN: type (NTSC/PAL/SECAM). See VBI_VIDEO_STANDARD.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_SetVideoStandard(VBI_VIDEO_STANDARD eStandard);

//-------------------------------------------------------------------------------------------------
/// Check TTX circuit ready.
/// @ingroup VBI_TTX
/// @return TRUE  : TTX circuit is ready.
/// @return FALSE : TTX circuit is not ready.
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_TTX_CheckCircuitReady(void);

//-------------------------------------------------------------------------------------------------
/// Get TTX Packet Count.
/// @ingroup VBI_TTX
/// @return : TTX packet count
//-------------------------------------------------------------------------------------------------
MS_U16 MDrv_VBI_TTX_GetPacketCount(void);

//-------------------------------------------------------------------------------------------------
/// Get TTX Information.
/// @ingroup VBI_TTX
/// @param  selector  \b IN: TTX information type.
/// @return : MS_U32
//-------------------------------------------------------------------------------------------------
MS_U32 MDrv_VBI_TTX_GetInfo(MS_U32 u32Info);

//-------------------------------------------------------------------------------------------------
/// Check if the TTX packet in VBI buffer is empty.
/// @ingroup VBI_TTX
/// @return TRUE  : TTX buffer is empty.
/// @return FALSE : TTX buffer is not empty.
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_TTX_PacketBufferIsEmpty(void);

// Get all of the packets in the VBI buffer (if the input buffer is big enough)
//-------------------------------------------------------------------------------------------------
/// Get TTX Packets. (Get all of the packets in the VBI buffer (if the input buffer is big enough))
/// @ingroup VBI_TTX
/// @param  dataAddr   \b IN:   Data address.
/// @param  length       \b OUT: Packet length.
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
#if defined(UFO_PUBLIC_HEADER_300)
MS_BOOL MDrv_VBI_TTX_GetPackets(MS_PHYADDR dataAddr, MS_U32 *length);
#else
MS_BOOL MDrv_VBI_TTX_GetPackets(MS_PHY dataAddr, MS_U32 *length);
#endif
// Get 1 of the packets in the VBI buffer
//-------------------------------------------------------------------------------------------------
/// Get TTX Packets. (Get 1 of the packets in the VBI buffer)
/// @ingroup VBI_TTX
/// @param  packetAddress   \b OUT: Packet address
/// @return TRUE  : Get TTX packet successfully.
/// @return FALSE : Get TTX packet unsuccessfully.
//-------------------------------------------------------------------------------------------------
#if defined(UFO_PUBLIC_HEADER_300)
MS_BOOL MDrv_VBI_TTX_GetPacket(MS_PHYADDR *packetAddress);
#else
MS_BOOL MDrv_VBI_TTX_GetPacket(MS_PHY *packetAddress);
#endif
//-------------------------------------------------------------------------------------------------
/// Check if there is a packet buffer overflow. If there is an overflow,
/// the packet buffer should be cleared from the reading task.
/// @ingroup VBI_TTX
/// @return TRUE  : TTX buffer is overflow.
/// @return FALSE : Otherwise.
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_TTX_PacketBufferIsOverflow(void);

//-------------------------------------------------------------------------------------------------
/// Resuren the nomber of packet buffer overflows since the last reset or creation.
/// @ingroup VBI_TTX
/// @return : The number of packet buffer overflows.
//-------------------------------------------------------------------------------------------------
MS_U16 MDrv_VBI_TTX_PacketBufferGetNoOfOverflows(void);

//-------------------------------------------------------------------------------------------------
/// Set TTX Enable Line
/// @ingroup VBI_TTX
/// @param  StartLine   \b IN: TTX start line
/// @param  EndLine    \b IN: TTX end line
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_TTX_EnableLine(MS_U16 StartLine, MS_U16 EndLine);


//-------------------------------------------------------------------------------------------------
/// Restrict vbi memory buffer range.
/// @ingroup VBI_Task
/// @param  bEnable    \b IN: set enable protect.
/// @param  phyAddr   \b IN: start address of the range.
/// @param  u32Size    \b IN: size of the range.
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
#if defined(UFO_PUBLIC_HEADER_300)
MS_BOOL MDrv_VBI_ProtectMemory(MS_BOOL bEnable, MS_PHYADDR phyAddr, MS_U32 u32Size);
#else
MS_BOOL MDrv_VBI_ProtectMemory(MS_BOOL bEnable, MS_PHY phyAddr, MS_U32 u32Size);
#endif
/*
 *  ClosedCaption
 */
//-------------------------------------------------------------------------------------------------
/// Initialize CC Slicer.
/// @ingroup VBI_CC
/// @param  u32RiuAddr     \b IN: VBI CC RIU address.
/// @param  bufferAddr      \b IN: VBI CC buffer address.
/// @param  packetCount   \b IN: VBI CC packet count.
/// @return None
//-------------------------------------------------------------------------------------------------
#if defined(UFO_PUBLIC_HEADER_300)
void MDrv_VBI_CC_InitSlicer(MS_U32 u32RiuAddr, MS_PHYADDR bufferAddr, MS_U16 packetCount);
#else
void MDrv_VBI_CC_InitSlicer(MS_VIRT u32RiuAddr, MS_PHY bufferAddr, MS_U16 packetCount);
#endif
//-------------------------------------------------------------------------------------------------
/// Initialize CC CVBS number.
/// @ingroup VBI_CC
/// @param  cvbs_no     \b IN: CVBS number.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_CC_InitYPbYr(MS_U8 cvbs_no);
//-------------------------------------------------------------------------------------------------
/// Enable CC Slicer.
/// @ingroup VBI_CC
/// @param  bEnable     \b IN: enable or disable CC slicer.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_CC_EnableSlicer(MS_BOOL bEnable);
//-------------------------------------------------------------------------------------------------
/// Set CC Data Rate.
/// @ingroup VBI_CC
/// @param  ptable    \b OUT: data rate table.
/// @return : Data rate or 0 when failed
//-------------------------------------------------------------------------------------------------
MS_U8 MDrv_VBI_CC_SetDataRate(MS_U8 *ptable);
//-------------------------------------------------------------------------------------------------
/// Set CC Frame Count.
/// @ingroup VBI_CC
/// @param  cnt     \b IN: frame count.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_CC_SetFrameCnt(MS_U8 cnt);
//-------------------------------------------------------------------------------------------------
/// Get CC Information.
/// @ingroup VBI_CC
/// @param  selector  \b IN: CC function select.
/// @return : packet count or indication
//-------------------------------------------------------------------------------------------------
MS_U32 MDrv_VBI_CC_GetInfo(MS_U32 selector);
//-------------------------------------------------------------------------------------------------
/// Enable CC Line.
/// @ingroup VBI_CC
/// @param  StartLine    \b IN: start line.
/// @param  EndLine     \b IN: end line.
/// @param  mode        \b IN: NTSC/PAL/SECAM mode.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_CC_EnableLine(MS_U16 StartLine, MS_U16 EndLine, MS_U8 mode);
//-------------------------------------------------------------------------------------------------
/// Fresh memory to avoid cache coherence issue. (Use MsOS function)
/// @ingroup VBI_ToBeRemove
/// @param  u32Start    \b IN: start address (must be 16-B aligned and in cacheable area).
/// @param  u32Size     \b IN: size (must be 16-B aligned).
/// @return TRUE  : Succeed
/// @return FALSE : Fail due to invalid parameter
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_SyncMemory(MS_U32 u32Start, MS_U32 u32Size);
//-------------------------------------------------------------------------------------------------
/// Set CC SC window length.
/// @ingroup VBI_CC
/// @param  u8Len  \b IN: windows length.
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_CC_SetSCWindowLen(MS_U8 u8Len);
//-------------------------------------------------------------------------------------------------
/// Set CC standard.
/// @ingroup VBI_CC
/// @param  eStandard  \b IN: video standard.
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_CC_SetVideoStandard(VBI_VIDEO_STANDARD eStandard);

/*
 *  Others
 */
//-------------------------------------------------------------------------------------------------
/// Set WSS/VPS Byte Number. (Refine function name: MDrv_VBI_TTX_WSS_SetVpsByteNum)
/// @ingroup VBI_TTX_ToBeModified
/// @param  cnt    \b IN: byte number.
/// @return None
//-------------------------------------------------------------------------------------------------
void MDrv_VBI_WSS_SetVpsByteNum(MS_U8 cnt);

//-------------------------------------------------------------------------------------------------
/// Save VBI driver states to DRAM.
/// @ingroup VBI_STR
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_Suspend(void);
//-------------------------------------------------------------------------------------------------
/// Restore VBI driver states from DRAM
/// @ingroup VBI_STR
/// @return TRUE  : Succeed
/// @return FALSE : Fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_Resume(void);
//-------------------------------------------------------------------------------------------------
/// Control VBI STR functions.
/// @ingroup VBI_STR
/// @param  u16PowerState  \b IN: STR power mode. (to control STR suspend & resume case.)
/// @return : Utopia status - UTOPIA_STATUS_SUCCESS (Succeed) or UTOPIA_STATUS_FAIL (Fail)
//-------------------------------------------------------------------------------------------------
MS_U32  MDrv_VBI_SetPowerState(EN_POWER_MODE u16PowerState);

void MDrv_VBI_ISR(int eIntNum);

//-------------------------------------------------------------------------------------------------
/// Get WSS Packet Count.
/// @ingroup VBI_TTX
/// @return : WSS packet count
//-------------------------------------------------------------------------------------------------
MS_U16 MDrv_VBI_WSS_GetPacketCount(void);

#ifdef __cplusplus
}
#endif

#endif

