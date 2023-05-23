/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_DRV_VBI_H__
#define __MTK_DRV_VBI_H__

#include <linux/videodev2.h>
#include <linux/types.h>
#include "mtk_srccap.h"


#define TEST_MODE  0	// 1: for TEST


//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define VBI_TTX_DATA_LENGTH 48
#define VBI_TTX_PKT_SIZE    96
#define VBI_CC_PKT_SIZE     16

#define MSIF_VBI_LIB_CODE    {'V', 'B', 'I', '_'}	//Lib code
#define MSIF_VBI_LIBVER      {'0', '0'}	//LIB version
#define MSIF_VBI_BUILDNUM    {'0', '2'}	//Build Number
#define MSIF_VBI_CHANGELIST  {'0', '0', '5', '6', '5', '9', '8', '0'}	//P4 ChangeList Number

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT {
	V4L2_EXT_VBI_FAIL = -1,
	V4L2_EXT_VBI_OK = 0,
	V4L2_EXT_VBI_NOT_SUPPORT,
	V4L2_EXT_VBI_NOT_IMPLEMENT,
};

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
	MS_U8 u8NoInfo;
} VBI_DrvInfo;

typedef struct DLL_PACKED {
	VBI_INIT_TYPE eInitType;
} VBI_DrvStatus;

typedef struct DLL_PACKED {
	MS_U8 ptable[7];
} VBI_CC_TABLE;

typedef struct DLL_PACKED {
	MS_BOOL _bSuspend;
	VBI_INIT_TYPE eInitType;
	MS_VIRT _ptrVBIRiuBaseAddr;

	// TTX buffer address & buffer size
	MS_PHY _TTXBufAddr;
	MS_U16 _TTXBufLen;

	// CC buffer address & buffer size
	MS_PHY _CCBufAddr;
	MS_U16 _CCBufLen;

	// callback info
	MS_PHY _cbBufferAddr;
	MS_U32 _cbBufferLength;
	MS_U8 *_pCbVABuff;

	MS_BOOL _bTTXSupported;
	MS_BOOL _bVBIInit;
	MS_U16 _u16VBIDbgSwitch;
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
	VBI_ERRORCODE_SUCCESS = 0x0000,
	VBI_ERRORCODE_FAIL = 0x0001
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

typedef struct {
	unsigned char u8CcByte1;           ///< u8CcByte1[7:0]
	unsigned char u8CcByte2;           ///< u8CcByte2[15:8]
	unsigned char u8CcPacketCnt;       ///< u8CcPacketCnt[4:0]
	unsigned char u8CcFrameCnt;        ///< u8CcFrameCnt[4:0]
	unsigned char u8CcEvenOddFound;    ///< u8CcEvenOddFound[0]: even/odd,
		//u8CcEvenOddFound[6]: even found, u8CcEvenOddFound[7]: odd found
	unsigned char au8CcReserved[3];    ///< reserved for future use
} VBI_CcBufContent;

typedef struct {
	VBI_CcBufContent stCcData;
	MS_U8 au8CcReserved[8];
} VBI_CcBufContentDoubleSize_t;

typedef MS_U32(*VBI_CB_FN) (MS_U8 *pVBILine, MS_U32 length);
//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

/*
 *  VBI_General
 */
enum V4L2_VBI_RESULT mtk_vbi_isr(int eIntNum);

//-------------------------------------------------------------------------------------------------
/// Get VBI Driver Status.
/// @ingroup VBI_General
/// @param  pDrvStatus     OUT: driver status.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_get_status(VBI_DrvStatus *pDrvStatus);	///< Get VBI current status

//-------------------------------------------------------------------------------------------------
/// Initialize VBI module
/// @ingroup VBI_General
/// @param  type            IN: VBI type (Teletext/CC)
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_init(VBI_INIT_TYPE type);

//-------------------------------------------------------------------------------------------------
/// Finalize VBI module
/// @ingroup VBI_General
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_exit(EN_VBI_CMD cmd);


//-------------------------------------------------------------------------------------------------
/// Set Callback to VBI driver, the CB will be called if VBI interrupt is catched
/// @ingroup VBI_Task
/// @param  pFN             IN: call back function pointer.
/// @param  bufferAddr      IN: the physical address of buffer
///                             which will store VBI data and pass pointer to CB
/// @param  length          IN: the data lebgth of th input buffer.
///                             Note: it should be the multiple of VBI_TTX_DATA_LENGTH
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_register_cb(VBI_CB_FN pFN, MS_PHY bufferAddr, MS_U32 length);

//-------------------------------------------------------------------------------------------------
/// Get VBI module info. Get info from driver (without Mutex protect)
/// @ingroup VBI_General
/// @return VBI_DrvInfo
//-------------------------------------------------------------------------------------------------
const VBI_DrvInfo *mtk_vbi_get_info(void);

//-------------------------------------------------------------------------------------------------
/// Set VBI Debug Level. Set debug level (without Mutex protect)
/// @ingroup VBI_General
/// @param  u16DbgSwitch    IN: debug level.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_set_dbg_level(MS_U16 u16DbgSwitch);

//-------------------------------------------------------------------------------------------------
/// Reset TTX Ring Buffer
/// @ingroup VBI_Task
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ring_buffer_reset(void);

//-------------------------------------------------------------------------------------------------
/// VPS Is Ready or Not. (Refine function name: MDrv_VBI_TTX_IsVPS_Ready)
/// @ingroup VBI_TTX_ToBeModified
/// @param  bEnable        OUT: VPS is ready or not
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_is_vps_ready(bool *bEnable);

//-------------------------------------------------------------------------------------------------
/// TTX Is Ready or Not.
/// @ingroup VBI_TTX
/// @param  bEnable        OUT: TTX is ready or not
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_is_ttx_ready(bool *bEnable);

//-------------------------------------------------------------------------------------------------
/// WSS Is Ready or Not. (Refine function name: mtk_vbi_is_wss_ready)
/// @ingroup VBI_TTX_ToBeModified
/// @param  bReady         OUT: WSS is ready or not
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_is_wss_ready(bool *bReady);

//-------------------------------------------------------------------------------------------------
/// Get WSS Data. (Refine function name: MDrv_VBI_TTX_GetWSS_Data)
/// @ingroup VBI_TTX_ToBeModified
/// @param  wss_data        OUT: WSS data
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_get_wss_data(MS_U16 *u16WssData);

//-------------------------------------------------------------------------------------------------
/// Get VPS Data. (Refine function name: MDrv_VBI_TTX_GetVPS_Data)
/// @ingroup VBI_TTX_ToBeModified
/// @param  lowerWord      OUT: VPS lower data.
/// @param  higherWord     OUT: VPS higher data.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_get_vps_data(MS_U8 *lowerWord, MS_U8 *higherWord);

//-------------------------------------------------------------------------------------------------
/// Set Video Standard. (Refine function name: MDrv_VBI_TTX_SetVideoStandard)
/// @ingroup VBI_TTX_ToBeModified
/// @param  lowerWord       IN: type (NTSC/PAL/SECAM). See VBI_VIDEO_STANDARD.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_set_video_standard(VBI_VIDEO_STANDARD eStandard);

//-------------------------------------------------------------------------------------------------
/// Initialize TTX Slicer.
/// @ingroup VBI_TTX
/// @param  bufferAddr      IN: TTX buffer address.
/// @param  packetCount     IN: TTX packet count.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_initialize_slicer(MS_PHY bufferAddr, MS_U16 packetCount);

//-------------------------------------------------------------------------------------------------
/// Enable TTX Slicer.
/// @ingroup VBI_TTX
/// @param bEnable          IN: enable or disable TTX slicer.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_enable_slicer(bool bEnable);

//-------------------------------------------------------------------------------------------------
/// Check if the TTX packet in VBI buffer is empty.
/// @ingroup VBI_TTX
/// @return V4L2_EXT_VBI_OK   : TTX buffer is empty.
/// @return V4L2_EXT_VBI_FAIL : TTX buffer is not empty.
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_packet_buffer_is_empty(bool *IsEmpty);

//-------------------------------------------------------------------------------------------------
/// Check TTX circuit ready.
/// @ingroup VBI_TTX
/// @return V4L2_EXT_VBI_OK   : TTX circuit is ready.
/// @return V4L2_EXT_VBI_FAIL : TTX circuit is not ready.
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_check_circuit_ready(void);


//-------------------------------------------------------------------------------------------------
/// Get TTX Packet Count.
/// @ingroup VBI_TTX
/// @param  packet_count   OUT: TTX packet count
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_packet_count(MS_U16 *u16PacketCount);

//-------------------------------------------------------------------------------------------------
/// Get TTX Information.
/// @ingroup VBI_TTX
/// @param  u32TypeSelect   IN: TTX information type.
/// @param  u32Info        OUT: TTX information.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_info(MS_U32 u32TypeSelect, MS_U32 *u32Info);

// Get all of the packets in the VBI buffer (if the input buffer is big enough)
//-------------------------------------------------------------------------------------------------
/// Get TTX Packets. (Get all of the packets in the VBI buffer (if the input buffer is big enough))
/// @ingroup VBI_TTX
/// @param  dataAddr       IN : Data address.
/// @param  length         OUT: Packet length.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_packets(MS_PHY dataAddr, MS_U32 *length);

// Get 1 of the packets in the VBI buffer
//-------------------------------------------------------------------------------------------------
/// Get TTX Packets. (Get 1 of the packets in the VBI buffer)
/// @ingroup VBI_TTX
/// @param  packetAddress  OUT: Packet address
/// @return V4L2_EXT_VBI_OK   : Get TTX packet successfully.
/// @return V4L2_EXT_VBI_FAIL : Get TTX packet unsuccessfully.
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_packet(MS_PHY *packetAddress, MS_U8 *u8Success);

//-------------------------------------------------------------------------------------------------
/// Check if there is a packet buffer overflow. If there is an overflow,
/// the packet buffer should be cleared from the reading task.
/// @ingroup VBI_TTX
/// @param  packetAddress  OUT: TTX buffer is overflow or not.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_packet_buffer_is_overflow(bool *bIsOverflow);

//-------------------------------------------------------------------------------------------------
/// Resuren the nomber of packet buffer overflows since the last reset or creation.
/// @ingroup VBI_TTX
/// @param  u16NoOfPacketBufferOverflows  OUT: The number of packet buffer overflows.
/// @return V4L2_EXT_VBI_OK                  : Succeed
/// @return V4L2_EXT_VBI_FAIL                : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_packet_buffer_get_no_of_overflows(MS_U16 *
								   u16NoOfPacketBufferOverflows);

//-------------------------------------------------------------------------------------------------
/// Set TTX Enable Line
/// @ingroup VBI_TTX
/// @param  StartLine       IN: TTX start line
/// @param  EndLine         IN: TTX end line
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_ttx_enable_line(MS_U16 StartLine, MS_U16 EndLine);


/*
 *  ClosedCaption
 */
//-------------------------------------------------------------------------------------------------
/// Initialize CC Slicer.
/// @ingroup VBI_CC
/// @param  u32RiuAddr      IN: VBI CC RIU address.
/// @param  bufferAddr      IN: VBI CC buffer address.
/// @param  packetCount     IN: VBI CC packet count.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_init_slicer(MS_VIRT u32RiuAddr, MS_PHY bufferAddr,
					    MS_U16 packetCount);

//-------------------------------------------------------------------------------------------------
/// Enable CC Slicer.
/// @ingroup VBI_CC
/// @param  bEnable         IN: enable or disable CC slicer.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_enable_slicer(bool bEnable);

//-------------------------------------------------------------------------------------------------
/// Set CC Data Rate.
/// @ingroup VBI_CC
/// @param  ptable         OUT: data rate table.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_set_data_rate(MS_U8 *ptable);

//-------------------------------------------------------------------------------------------------
/// Set CC SC window length.
/// @ingroup VBI_CC
/// @param  u8Len           IN: windows length.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_set_sc_window_len(MS_U8 u8Len);

//-------------------------------------------------------------------------------------------------
/// Set CC standard.
/// @ingroup VBI_CC
/// @param  eStandard     IN: video standard.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_set_standard(VBI_VIDEO_STANDARD eStandard);

//-------------------------------------------------------------------------------------------------
/// Get CC Information.
/// @ingroup VBI_CC
/// @param  u32selector      IN: CC function select.
/// @param  u32PacketCount  OUT: packet count or indication
/// @return V4L2_EXT_VBI_OK    : Succeed
/// @return V4L2_EXT_VBI_FAIL  : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_get_info(MS_U32 u32selector, MS_U32 *u32PacketCount);

//-------------------------------------------------------------------------------------------------
/// Set CC Frame Count.
/// @ingroup VBI_CC
/// @param  cnt             IN: frame count.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_set_frame_cnt(MS_U8 cnt);

//-------------------------------------------------------------------------------------------------
/// Enable CC Line.
/// @ingroup VBI_CC
/// @param  StartLine       IN: start line.
/// @param  EndLine         IN: end line.
/// @param  mode            IN: NTSC/PAL/SECAM mode.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_cc_enable_line(MS_U16 StartLine, MS_U16 EndLine, MS_U8 mode);


/*
 *  Others
 */
//-------------------------------------------------------------------------------------------------
/// Set WSS/VPS Byte Number. (Refine function name: MDrv_VBI_TTX_WSS_SetVpsByteNum)
/// @ingroup VBI_TTX_ToBeModified
/// @param  cnt             IN: byte number.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_wss_set_vps_byte_num(MS_U8 cnt);

//-------------------------------------------------------------------------------------------------
/// Get WSS Packet Count.
/// @ingroup VBI_TTX
/// @param  u16PacketCount  OUT: WSS packet count
/// @return V4L2_EXT_VBI_OK    : Succeed
/// @return V4L2_EXT_VBI_FAIL  : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_wss_get_packet_count(MS_U16 *u16PacketCount);

//-------------------------------------------------------------------------------------------------
/// Save VBI driver states to DRAM.
/// @ingroup VBI_STR
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_suspend(void);

//-------------------------------------------------------------------------------------------------
/// Restore VBI driver states from DRAM
/// @ingroup VBI_STR
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_resume(void);

//-------------------------------------------------------------------------------------------------
/// Control VBI STR functions.
/// @ingroup VBI_STR
/// @param  u16PowerState   IN: STR power mode. (to control STR suspend & resume case.)
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_set_power_state(EN_POWER_MODE u16PowerState);

//-------------------------------------------------------------------------------------------------
/// Get VPS raw data functions.
/// @ingroup VPS_RAW
/// @param  byte0, byte1, byte2, byte3   OUT: 4 bytes of VPS raw data
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_get_raw_vps_data(MS_U8 *byte0, MS_U8 *byte1, MS_U8 *byte2,
		MS_U8 *byte3);

#if TEST_MODE
//-------------------------------------------------------------------------------------------------
/// Restrict vbi memory buffer range.
/// @ingroup VBI_Task
/// @param  bEnable         IN: set enable protect.
/// @param  phyAddr         IN: start address of the range.
/// @param  u32Size         IN: size of the range.
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_protect_memory(bool bEnable, MS_PHY phyAddr, MS_U32 u32Size);

//-------------------------------------------------------------------------------------------------
/// Fresh memory to avoid cache coherence issue. (Use MsOS function)
/// @ingroup VBI_ToBeRemove
/// @param  u32Start        IN: start address (must be 16-B aligned and in cacheable area).
/// @param  u32Size         IN: size (must be 16-B aligned).
/// @return V4L2_EXT_VBI_OK   : Succeed
/// @return V4L2_EXT_VBI_FAIL : Fail
//-------------------------------------------------------------------------------------------------
enum V4L2_VBI_RESULT mtk_vbi_sync_memory(MS_U32 u32Start, MS_U32 u32Size);
#endif

#endif /*__MTK_DRV_VBI_H__*/
