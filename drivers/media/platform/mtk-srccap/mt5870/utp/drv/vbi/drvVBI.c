// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define _DRV_VBI_C

// Common Definition
#if defined(MSOS_TYPE_LINUX_KERNEL)
	#include <linux/slab.h>
	#include <linux/string.h> //for memcpy, memset
#else
	#include <string.h>
#endif
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

//#include "MsOS.h"

// Internal Definition

#include "drvVBI.h"
//#include "drvMMIO.h"
//#include "drvSYS.h"
//#include "mdrv_vbi.h"


//#include "utopia.h"

#if (VBI_UTOPIA20)
#include "drvVBI_v2.h"
#endif

//#include "drvVBI_private.h"
extern ptrdiff_t mstar_pm_base;
int vbi_interrupt_irq = -1;
struct mutex vbi_mutex;

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define _BIT0       BIT(0)
#define _BIT1       BIT(1)
#define _BIT2       BIT(2)
#define _BIT3       BIT(3)
#define _BIT4       BIT(4)
#define _BIT5       BIT(5)
#define _BIT6       BIT(6)
#define _BIT7       BIT(7)
#define _BIT8       BIT(8)
#define _BIT9       BIT(9)
#define _BIT10      BIT(10)
#define _BIT11      BIT(11)
#define _BIT12      BIT(12)
#define _BIT13      BIT(13)
#define _BIT14      BIT(14)
#define _BIT15      BIT(15)

#define UNUSED(x)       (void)((x) = (x))

#define DEFAULT_WAIT_TIME MSOS_WAIT_FOREVER  //ms

static VBI_CB_FN _pFN;

static VBI_RING_BUFFER rb_handle;
static VBI_DrvInfo     _drvInfo;
static VBI_DrvStatus   _drvStatus;
#define VBI_TTX_PKT_SIZE 96

VBI_StoreInfo _pStoreInfo = {
	._bSuspend = FALSE,
	.eInitType = E_VBI_TELETEXT,

	// TTX buffer address & buffer size
	._ptrVBIRiuBaseAddr = 0xFFFFFFFFUL,
	._TTXBufAddr = 0xFFFFFFFFUL,
	._TTXBufLen  = 0,

	// CC buffer address & buffer size
	._CCBufAddr = 0xFFFFFFFFUL,
	._CCBufLen  = 0,

	// callback info
	._cbBufferAddr   = 0xFFFFFFFFUL,
	._cbBufferLength = 0,
	._pCbVABuff      = NULL,

	._bTTXSupported = FALSE,
	._bVBIInit      = FALSE,
	._u16VBIDbgSwitch = 0,
};

// Mutex & Lock
static MS_S32 _s32VBIMutex = -1;

#if (VBI_UTOPIA20)
static void *pInstantVbi;
static void *pAttributeVbi;
#endif

#define VIDEO_TYPE_CNT          5
#define VIDEO_REGISTER_CNT      8
static MS_U8 video_standard_vbi_settings[VIDEO_TYPE_CNT][VIDEO_REGISTER_CNT + 1] = {
//      type                 41h[7:0] 42h[5:0] 44h[7:0] 4Bh[5:0] 4Dh[7:0] 50h[4:0] 51h[4:0] 53h[4:0]
	{VBI_VIDEO_NTSC,          0x52,   0x1c,   0x39,   0x24,   0x8e,   0xd2,   0x12,   0x90},
	{VBI_VIDEO_PAL_M,         0x52,   0x1c,   0x39,   0x26,   0x8e,   0xd2,   0x12,   0x90},
	{VBI_VIDEO_PAL_NC,        0x52,   0x1c,   0x39,   0x16,   0x8e,   0xd2,   0x12,   0x90},
	{VBI_VIDEO_NTSC443_PAL60, 0x52,   0x23,   0x47,   0x32,   0xb0,   0xd2,   0x12,   0xb2},
	{VBI_VIDEO_PAL,           0xb5,   0x23,   0x47,   0x22,   0xb0,   0xd5,   0x15,   0xb2},
};
//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function \b Name: _MDrv_VBI_ISR()
/// @brief \b Function \b Description:  This function is used to set the Decoder DSP ISR
////////////////////////////////////////////////////////////////////////////////

static void _MDrv_VBI_ISR(int eIntNum)
{
#if defined(MSOS_TYPE_LINUX_KERNEL)
	if (_pStoreInfo._pCbVABuff != NULL)
#else
	if (_pFN != NULL && _pStoreInfo._pCbVABuff != NULL)
#endif
	{
		MS_U32 length = _pStoreInfo._cbBufferLength;

		switch (_drvStatus.eInitType) {
		case E_VBI_TELETEXT:
			//_MDrv_VBI_TTX_GetPackets(_pStoreInfo._cbBufferAddr, &length);
			break;

		case E_VBI_CC:
			// need to implement
			break;

		default:
			break;
		}
	}
	vbi_interrupt_irq = eIntNum;
	enable_irq(eIntNum);
}


void MDrv_VBI_ISR(int eIntNum)
{
	_MDrv_VBI_ISR(eIntNum);
}


//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
MS_BOOL MDrv_VBI_GetStatus(VBI_DrvStatus *pDrvStatus)
{
	memcpy(pDrvStatus, &_drvStatus, sizeof(VBI_DrvStatus));
	return TRUE;
}

/****************************************************************************
 *@brief            : Initialize VBI
 *@param type       : [IN] VBI type (Teletext/CC)
 *@return
 *  - TRUE : initialize VBI successfully.
 *  - FALSE: initialize VBI unsuccessfully.
 ****************************************************************************/


MS_BOOL _MDrv_VBI_Init(EN_VBI_CMD cmd, VBI_INIT_TYPE type)
{
	ULOGE("VBI", "In _MDrv_VBI_Init cmd=%d, type =%d\n", cmd, type);
	MS_PHY phyNonPMBankSize;
	static const char _vbiSyncMutexName[] = "OS_VBI_MUTEX";

	mutex_init(&vbi_mutex);
	_pStoreInfo._ptrVBIRiuBaseAddr = (mstar_pm_base+0x200000UL);
	if (cmd == VBI_INIT) {
		if (type == E_VBI_WSS) {
			//KDrv_HW_VBI_WSSInit(_pStoreInfo._ptrVBIRiuBaseAddr);
			return TRUE;
		}

		//if (type == E_VBI_TELETEXT)
		//	KDrv_HW_VBI_TTXInit(_pStoreInfo._ptrVBIRiuBaseAddr);

		 _drvStatus.eInitType = type;
		_pStoreInfo._bVBIInit = TRUE;
		_pStoreInfo._bSuspend = FALSE;
		return TRUE;
	}

	//if (type == E_VBI_TELETEXT)
		//VBI_TTXInit(_pStoreInfo._ptrVBIRiuBaseAddr);
	//else
		// should call CC VBI init

	enable_irq((int)vbi_interrupt_irq);

	return TRUE;
}


MS_BOOL MDrv_VBI_Init(VBI_INIT_TYPE type)
{
	return _MDrv_VBI_Init(VBI_INIT, type);
}


/****************************************************************************
 *@brief            : VBI Exit
 *@return
 *  - TRUE : VBI exit successfully.
 *  - FALSE: VBI exit unsuccessfully.
 ****************************************************************************/

MS_BOOL MDrv_VBI_Exit(EN_VBI_CMD cmd)
{
	_pStoreInfo._bVBIInit = FALSE;
	if (cmd == VBI_EXIT)// vbi exit

#ifdef NO_DEFINE
		if (_s32VBIMutex != -1) {
			OS_OBTAIN_MUTEX(_s32VBIMutex, DEFAULT_WAIT_TIME);
			OS_RELEASE_MUTEX(_s32VBIMutex);
			OS_DELETE_MUTEX(_s32VBIMutex);
		}
#endif
		disable_irq_nosync((int)vbi_interrupt_irq);
	else // vbi suspend
		disable_irq_nosync((int)vbi_interrupt_irq);

	return TRUE;
}



/****************************************************************************
 *@brief            : Get VBI Information
 *@param
 *@return
 *@see VBI_DrvInfo
 ****************************************************************************/
const VBI_DrvInfo *MDrv_VBI_GetInfo(void)
{
	return &_drvInfo;
}

/****************************************************************************
 *@brief                : Set VBI Debug Level
 *@param u16DbgSwitch   : [IN] debug level
 *@return
 *  - TRUE : set debug level successfully.
 *  - FALSE: set debug level unsuccessfully.
 ****************************************************************************/
MS_BOOL MDrv_VBI_SetDbgLevel(MS_U16 u16DbgSwitch)
{
	_pStoreInfo._u16VBIDbgSwitch = u16DbgSwitch;

	return TRUE;
}


/****************************************************************************
 *@brief            : Get VBI Driver Status
 *@param pDrvStatus : [OUT] driver status
 *@return
 *  - TRUE : get status successfully.
 *  - FALSE: get status unsuccessfully.
 ****************************************************************************/





/****************************************************************************
 *@brief            : Set Callback to VBI driver, the CB will be called if VBI interrupt is catched
 *@param pFN : [IN] call back function pointer.
 *@param bufferAddr : [IN] the PA of buffer which stores VBI data and pass pointer to CB
 *@param length : [IN] the data length of input buffer.
 *	          Note: it should be the multiple of VBI_TTX_PKT_SIZE
 *@return
 ****************************************************************************/



/****************************************************************************
 *@brief            : Reset TTX Ring Buffer
 *@param
 *@return
 ****************************************************************************/
void MDrv_VBI_RingBuffer_Reset(void)
{

	MDrv_VBI_RingBuffer_Reset();

}



/****************************************************************************
 *@brief            : VPS Is Ready or Not
 *@param
 *@return
 *  - TRUE : VPS is ready.
 *  - FALSE: VPS is not ready.
 ****************************************************************************/

/****************************************************************************
 *@brief            : TTX Is Ready or Not
 *@param
 *@return
 *  - TRUE : TTX is ready.
 *  - FALSE: TTX is not ready.
 ****************************************************************************/


/****************************************************************************
 *@brief            : Get VPS Data
 *@param lowerWord  : [OUT] VPS lower data
 *@param higherWord : [OUT] VPS higher data
 *@return
 ****************************************************************************/


//-------------------------------------------------------------------------------------------------
/// Get Complete VPS Data.
/// @ingroup VBI_TTX_ToBeModified
/// @param  pBuffer    \b IN/OUT: VPS buffer.
/// @param  dataLen   \b IN: buffer size.
/// @return None
//-------------------------------------------------------------------------------------------------


/****************************************************************************
 *@brief            : Set Video Standard
 *@param eStandard  : [IN] type (NTSC/PAL/SECAM)
 *@see VBI_VIDEO_STANDARD
 *@return
 ****************************************************************************/


/****************************************************************************
 *@brief            : Check if the TTX packet in VBI buffer is empty
 *@param
 *@return
 *  - TRUE : is empty
 *  - FALSE: not empty
 ****************************************************************************/


/****************************************************************************
 *@brief            : Check TTX circuit ready
 *@param
 *@return
 *  - TRUE : TTX circuit is ready.
 *  - FALSE: TTX circuit is not ready.
 ****************************************************************************/

/****************************************************************************
 *@brief            : Get TTX Packet Count
 *@param
 *@return           : TTX packet count
 ****************************************************************************/



/****************************************************************************
 *@brief            : Get TTX Packets
 *@param dataAddr   : [IN] data address
 *@param length     : [OUT] packet length
 *@return
 *  - TRUE : get TTX packets successfully.
 *  - FALSE: get TTX packets unsuccessfully.
 ****************************************************************************/


/****************************************************************************
 *@brief                : Get TTX Packet
 *@param packetAddress  : [OUT] packet address
 *@return
 *  - TRUE : get TTX packet successfully.
 *  - FALSE: get TTX packet unsuccessfully.
 ****************************************************************************/


/***********************************************************************
 * FUNCTION: MDrv_VBI_TTX_PacketBufferIsOverflow
 *
 * DESCRIPTION:
 *   Check if there is a packet buffer overflow. If there is an overflow,
 *   the packet buffer should be cleared from the reading task.
 *
 * RETURN:
 *   TRUE if there is packet buffer overflow,
 *   NULL otherwise.
 ***********************************************************************/

/***********************************************************************
 * FUNCTION: MDrv_VBI_TTX_PacketBufferGetNoOfOverflows
 *
 * DESCRIPTION:
 *   Resuren the nomber of packet buffer overflows since the last reset
 *   or creation.
 *
 * RETURN:
 *   The number of packet buffer overflows.
 ***********************************************************************/


/****************************************************************************
 *@brief            : Set TTX Enable Line
 *@param StartLine  : [IN] TTX start line
 *@param EndLine    : [IN] TTX end line
 *@return
 ****************************************************************************/


/****************************************************************************
 *@brief            : Fresh memory to avoid cache coherence issue
 *@param  u32Start \b IN: start address (must be 16-B aligned and in cacheable area)
 *@param  u32Size  \b IN: size (must be 16-B aligned)
 *@return TRUE : succeed
 *@return FALSE : fail due to invalid parameter
 ****************************************************************************/


/****************************************************************************
 *@brief                     : restrict vbi memory buffer range
 *@phyAddr                   : start address of the range
 *@u32Size                   : size of the range
 *@return TRUE - Success
 *@return FALSE - Failure
 ****************************************************************************/



/****************************************************************************
 *@brief            : WSS Is Ready or Not
 *@param
 *@return
 *  - TRUE : WSS is ready.
 *  - FALSE: WSS is not ready.
 ****************************************************************************/
MS_BOOL MDrv_VBI_IsWSS_Ready(void)
{
	ULOGE("VBI", "In MDrv_VBI_IsWSS_Ready\n");
	MS_U8 regval;

	if (_pStoreInfo._ptrVBIRiuBaseAddr == 0x0)
		return FALSE;

//   regval = 0xF0;// KDrv_HW_VBI_TTX_GetHardware_Indication();
/*
 *	if (regval & _BIT6)
 *	{
 *	return TRUE;
 *	} else
 *	{
 *	return FALSE;
 *	}
 */
	return TRUE;
}

/****************************************************************************
 *@brief            : Get WSS Data
 *@param
 *@return           : return WSS data
 ****************************************************************************/
MS_U16 MDrv_VBI_GetWSS_Data(void)
{
	ULOGE("VBI", "In MDrv_VBI_GetWSS_Data\n");
	if (_pStoreInfo._ptrVBIRiuBaseAddr == 0x0)
		return FALSE;

	//return KDrv_HW_VBI_GetWSS_Data();
	return TRUE;
}


/****************************************************************************
 *@brief            : Initialize CC Slicer
 *@param u32RiuAddr : [IN] VBI CC RIU address
 *@param bufferAddr : [IN] VBI CC buffer address
 *@param packetCount: [IN] VBI CC packet count
 *@return
 ****************************************************************************/
void MDrv_VBI_CC_InitSlicer(MS_VIRT u32RiuAddr, MS_PHY bufferAddr, MS_U16 packetCount)
{
	ULOGE("VBI", "In MDrv_VBI_CC_InitSlicer u32RiuAddr=0x%x, bufferAddr=0x%x, packetCount =%d ",
		u32RiuAddr, bufferAddr, packetCount);
	ULOGE("VBI", "(mstar_pm_base+0x200000UL) = 0x%x\n", (mstar_pm_base+0x200000UL));
	MS_PHY phyNonPMBankSize;

	_pStoreInfo._ptrVBIRiuBaseAddr = (mstar_pm_base+0x200000UL);

	if (bufferAddr != 0 && packetCount != 0) {
		if (_pStoreInfo._ptrVBIRiuBaseAddr != 0)
			u32RiuAddr = _pStoreInfo._ptrVBIRiuBaseAddr;

		_pStoreInfo._CCBufAddr = bufferAddr;
		_pStoreInfo._CCBufLen  = packetCount;
		ULOGE("VBI", "In call [eddie] KDrv_HW_VBI_CC_Init u32RiuAddr=0x%x, ", u32RiuAddr);
		ULOGE("VBI", "bufferAddr=0x%x, packetCount =%d\n", bufferAddr, packetCount);
		//KDrv_HW_VBI_CC_Init(u32RiuAddr, bufferAddr, packetCount);
	}
}

/****************************************************************************
 *@brief            : Set CC Data Rate
 *@param ptable     : [OUT] data rate table
 *@return           : data rate or 0 when failed
 ****************************************************************************/

MS_U8 MDrv_VBI_CC_SetDataRate(MS_U8 *ptable)
{
	if (ptable != NULL) {
		ULOGE("VBI", "In MDrv_VBI_CC_SetDataRate ptable[0] = 0x%x, ", ptable[0]);
		ULOGE("VBI", "ptable[7]= 0x%x ptable[8]= 0x%x\n", ptable[7], ptable[8]);
	//return KDrv_HW_VBI_CC_DataRateSet(ptable);
		return TRUE;
	}

	return FALSE;
}

MS_BOOL MDrv_VBI_CC_SetSCWindowLen(MS_U8 u8Len)
{
	ULOGE("VBI", "In MDrv_VBI_CC_SetSCWindowLen  u8Len=%d\n", u8Len);
	return TRUE;
	//return KDrv_HW_VBI_CC_SetSCWindowLen(u8Len);
}


/****************************************************************************
 *@brief            : Set CC standard
 *@param eStandard  : [IN] video standard
 *@return           : TRUE or FALSE
 ****************************************************************************/

MS_BOOL MDrv_VBI_CC_SetStandard(VBI_VIDEO_STANDARD eStandard)
{
	ULOGE("VBI", "In MDrv_VBI_CC_SetStandard  eStandard=%d\n", eStandard);
	MS_U8 i = 0;

	// Get the video system's type
	for (i = 0; i < VIDEO_TYPE_CNT; i++) {
		if (video_standard_vbi_settings[i][0] == eStandard)
			break;
	}

	if (i >= VIDEO_TYPE_CNT)
		return FALSE;

	//KDrv_HW_VBI_CC_DataRateSet(video_standard_vbi_settings[i]);
	//KDrv_HW_VBI_CC_SetSCWindowLen(video_standard_vbi_settings[i][8]); // BK_VBI_53
	return TRUE;
}

/****************************************************************************
 *@brief            : Get CC Information
 *@param selector   : [IN] CC function select
 *@return           : packet count or indication
 ****************************************************************************/
#define VBI_CC_PKT_SIZE 16
MS_U32 MDrv_VBI_CC_GetInfo(MS_U32 selector)
{
	ULOGE("VBI", "In MDrv_VBI_CC_GetInfo  selector=%d, FALSE = %d, TRUE = %d\n",
		selector, FALSE, TRUE);
	// Functionality select
	switch (selector) {
	case VBI_CC_PACKET_COUNT:
		return 16;//((MS_U32)KDrv_HW_VBI_CC_GetPacketCnt());
	case VBI_CC_BYTE_FOUND_INDICATION:
		return  16;// ((MS_U32)KDrv_HW_VBI_CC_GetByteFoundIndication());
	case VBI_CC_DATA_GET:
		return  16;// ((MS_U32)KDrv_HW_VBI_CC_GetPacket()); // odd field + even field data
	case VBI_CC_PACKET_SIZE:
		return ((MS_U32)VBI_CC_PKT_SIZE);
	default:
		break;
	}

	return  (MS_U32)VBI_ERRORCODE_SUCCESS;
}


/****************************************************************************
 *@brief            : Set CC Frame Count
 *@param cnt        : [IN] frame count
 *@return
 ****************************************************************************/
void MDrv_VBI_CC_SetFrameCnt(MS_U8 cnt)
{
	ULOGE("VBI", "In MDrv_VBI_CC_SetFrameCnt  cnt=%d\n", cnt);
	//KDrv_HW_VBI_SetCCFrameCnt(cnt);
}

/****************************************************************************
 *@brief            : Enable CC Slicer
 *@param bEnable    : [IN] enable or disable CC slicer
 *@return
 ****************************************************************************/

void MDrv_VBI_CC_EnableSlicer(MS_BOOL bEnable)
{
	ULOGE("VBI", "In MDrv_VBI_CC_EnableSlicer  bEnable=%d\n", bEnable);
	//KDrv_HW_VBI_CC_EnableSlicer(bEnable);
}

void MDrv_VBI_CC_EnableLine(MS_U16 StartLine, MS_U16 EndLine, MS_U8 mode)
{
	// set line
	//KDrv_HW_VBI_CC_SetCCLine(StartLine, EndLine, mode);
}


MS_BOOL _MDrv_VBI_Suspend(void)
{
	if ((_pStoreInfo._bSuspend != FALSE) || (_s32VBIMutex == -1))
		return FALSE;

	_pStoreInfo._bSuspend = TRUE;
	_pStoreInfo.eInitType = _drvStatus.eInitType;

	//KDrv_HW_VBI_RegStateRestore();
	MDrv_VBI_Exit(VBI_SUSPEND);

	return TRUE;
}

MS_BOOL _MDrv_VBI_Resume(void)
{
	if ((_pStoreInfo._bSuspend != TRUE) || (_s32VBIMutex == -1))
		return FALSE;

	_pStoreInfo._bSuspend = FALSE;

	_MDrv_VBI_Init(VBI_RESUME, _pStoreInfo.eInitType);

	//if ((_pStoreInfo.eInitType == E_VBI_CC) && ((_pStoreInfo._CCBufAddr != 0xFFFFFFFFUL)
	//	&& (_pStoreInfo._CCBufLen != 0)))
	//KDrv_HW_VBI_CC_Init(_pStoreInfo._ptrVBIRiuBaseAddr,
	//	_pStoreInfo._CCBufAddr, _pStoreInfo._CCBufLen);

	if ((_pStoreInfo.eInitType == E_VBI_TELETEXT) && ((_pStoreInfo._TTXBufAddr != 0xFFFFFFFFUL)
		&& (_pStoreInfo._TTXBufLen != 0)))
		MDrv_VBI_InitializeTTXSlicer(_pStoreInfo._TTXBufAddr, _pStoreInfo._TTXBufLen);

	//KDrv_HW_VBI_RegStateRestore();

	return TRUE;
}

MS_U32 _MDrv_VBI_SetPowerState(EN_POWER_MODE u16PowerState)
{
	MS_U16 _ret = FALSE;

	switch (u16PowerState) {
	case E_POWER_SUSPEND:
	    _ret = _MDrv_VBI_Suspend();
	break;

	case E_POWER_RESUME:
	    _ret = _MDrv_VBI_Resume();
	break;

	case E_POWER_MECHANICAL:
	case E_POWER_SOFT_OFF:
	default:
		ULOGE("VBI", "[%s] %d Power state not support!!\n", __func__, __LINE__);
		break;
	}

	return ((_ret == TRUE) ? UTOPIA_STATUS_SUCCESS : UTOPIA_STATUS_FAIL);
}

#ifdef NO_DEFINE
/****************************************************************************
 *@brief            : Set WSS/VPS Byte Number
 *@param cnt        : [IN] byte number
 *@return
 ****************************************************************************/


/****************************************************************************
 *@brief            : Get WSS Packet Count
 *@param
 *@return           : WSS packet count
 ****************************************************************************/

/****************************************************************************
 *@brief            : Suspend VBI driver
 *@return TRUE - Success
 *@return FALSE - Failure
 *@note
 *@Save VBI driver states to DRAM
 ****************************************************************************/


/****************************************************************************
 *@brief            :  Resume VBI driver
 *@return TRUE - Success
 *@return FALSE - Failure
 *@note
 *@Save VBI driver states to DRAM
 *@Restore VBI driver states from DRAM
 ****************************************************************************/


/****************************************************************************
 *@brief            : Set VBI suspend & resume
 *@param u16PowerState              \b IN: VBI power state
 *@return TRUE - Success
 *@return FALSE - Failure
 *@note
 ****************************************************************************/

#endif
