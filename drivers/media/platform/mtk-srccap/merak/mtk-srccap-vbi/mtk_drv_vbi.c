// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>

#include <linux/mtk-v4l2-srccap.h>
#include "sti_msos.h"
#include "mtk_drv_vbi.h"
#include "mtk_srccap.h"
#include "show_param_vbi.h"
#include "vbi-ex.h"
#include "hwreg_srccap_vbi.h"


#define UTOPIA_DECOMPOSE	//for vbi decompose

#define MS_PA2KSEG1(x) x	// TODO: only for test!!!! need refactory in the future!!!!!


int vbi_interrupt_irq = -1;
struct mtk_srccap_dev vbi_mutex;

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

#define DEFAULT_WAIT_TIME MSOS_WAIT_FOREVER	//ms

static VBI_CB_FN _pFN;

static VBI_RING_BUFFER rb_handle;
static VBI_DrvInfo _drvInfo;
static VBI_DrvStatus _drvStatus;

VBI_StoreInfo _pStoreInfo = {
	._bSuspend = FALSE,
	.eInitType = E_VBI_TELETEXT,

	// TTX buffer address & buffer size
	._ptrVBIRiuBaseAddr = 0xFFFFFFFFUL,
	._TTXBufAddr = 0xFFFFFFFFUL,
	._TTXBufLen = 0,

	// CC buffer address & buffer size
	._CCBufAddr = 0xFFFFFFFFUL,
	._CCBufLen = 0,

	// callback info
	._cbBufferAddr = 0xFFFFFFFFUL,
	._cbBufferLength = 0,
	._pCbVABuff = NULL,

	._bTTXSupported = FALSE,
	._bVBIInit = FALSE,
	._u16VBIDbgSwitch = 0,
};


#if (VBI_UTOPIA20)
static void *pInstantVbi;
static void *pAttributeVbi;
#endif

#define VIDEO_TYPE_CNT          5
#define VIDEO_REGISTER_CNT      8
static MS_U8 video_standard_vbi_settings[VIDEO_TYPE_CNT][VIDEO_REGISTER_CNT + 1] = {
// type 41h[7:0] 42h[5:0] 44h[7:0] 4Bh[5:0] 4Dh[7:0] 50h[4:0] 51h[4:0] 53h[4:0]
	{VBI_VIDEO_NTSC, 0x52, 0x1c, 0x39, 0x24, 0x8e, 0xd2, 0x12, 0x90},	// NTSC
	{VBI_VIDEO_PAL_M, 0x52, 0x1c, 0x39, 0x26, 0x8e, 0xd2, 0x12, 0x90},	// PAL-M
	{VBI_VIDEO_PAL_NC, 0x52, 0x1c, 0x39, 0x16, 0x8e, 0xd2, 0x12, 0x90},	// PAL-NC
	{VBI_VIDEO_NTSC443_PAL60, 0x52, 0x23, 0x47, 0x32, 0xb0, 0xd2, 0x12,
	    0xb2},  //NTSC-443,PAL-60
	{VBI_VIDEO_PAL, 0xb5, 0x23, 0x47, 0x22, 0xb0, 0xd5, 0x15, 0xb2},	// PAL
};

//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------

/****************************************************************************
 *@brief  : This function is used to set the Decoder DSP ISR
 *@param
 *@return : None.
 ****************************************************************************/
static void _mtk_vbi_isr(int eIntNum)
{
}


/****************************************************************************
 *@brief    : Update TTX Write Pointer
 *@param
 *@return
 *  - TRUE  : update write pointer successfully.
 *  - FALSE : update write pointer unsuccessfully.
 ****************************************************************************/
static bool _mtk_vbi_ttx_update_write_pointer(void)
{
	SRCCAP_VBI_MSG_DEBUG("In\n");
	MS_S32 NumberOfFreePackets;

	if (HWREG_VBI_TTX_CheckCircuitReady() == FALSE) {
		SRCCAP_VBI_MSG_DEBUG("Out\n");
		return FALSE;
	}

	rb_handle.WritePacketNumber = HWREG_VBI_TTX_GetPacketCount();

	if (rb_handle.pkt_received == TRUE) {
		NumberOfFreePackets =
		    (MS_S32) (rb_handle.WritePacketNumber - rb_handle.ReadPacketNumber);
		if (NumberOfFreePackets < 0)
			NumberOfFreePackets += _pStoreInfo._TTXBufLen;

		NumberOfFreePackets = (MS_S32) (_pStoreInfo._TTXBufLen - NumberOfFreePackets);

		if (NumberOfFreePackets < 2) {
			rb_handle.NoOfPacketBufferOverflows++;
			rb_handle.PacketBufferOverflow = TRUE;
			SRCCAP_VBI_MSG_DEBUG("Out\n");
			return FALSE;
		}
	}

	if (rb_handle.WritePacketNumber == 1) {
		if (rb_handle.pkt_received)
			rb_handle.WritePacketNumber = _pStoreInfo._TTXBufLen;
		else
			rb_handle.WritePacketNumber = 0;
	} else {
		if (rb_handle.WritePacketNumber == 0) {
			rb_handle.ReadPacketNumber = 0;
			SRCCAP_VBI_MSG_ERROR("Out\n");
			return FALSE;
		}

		rb_handle.WritePacketNumber--;
		rb_handle.pkt_received = TRUE;
	}

	SRCCAP_VBI_MSG_DEBUG("Out\n");
	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------

/****************************************************************************
 *@brief                : Initialize VBI
 *@param type           : [IN] VBI type (Teletext/CC)
 *@return
 *  - V4L2_EXT_VBI_OK   : initialize VBI successfully.
 *  - V4L2_EXT_VBI_FAIL : initialize VBI unsuccessfully.
 ****************************************************************************/
enum V4L2_VBI_RESULT _mtk_vbi_init(EN_VBI_CMD cmd, VBI_INIT_TYPE type)
{
	SRCCAP_VBI_MSG_DEBUG("In cmd = %d, type = %d\n", cmd, type);
	MS_PHY phyNonPMBankSize;
//      static char _vbiSyncMutexName[] = "OS_VBI_MUTEX";

	mutex_init(&vbi_mutex.mutex);

	if (cmd == VBI_INIT) {
		if (type == E_VBI_WSS) {
			HWREG_VBI_WSSInit();
			SRCCAP_VBI_MSG_DEBUG("Out cmd = %d, type = %d\n", cmd, type);
			return V4L2_EXT_VBI_OK;
		}

		if (type == E_VBI_TELETEXT) {
//	TODO: _bTTXSupported = MDrv_SYS_Query(E_SYS_QUERY_TTXNICAM_SUPPORTED); hard-code TRUE first
			_pStoreInfo._bTTXSupported = TRUE;
			HWREG_VBI_TTXInit();
		}

		_drvStatus.eInitType = type;
		_pStoreInfo._bVBIInit = TRUE;
		_pStoreInfo._bSuspend = FALSE;

		SRCCAP_VBI_MSG_DEBUG("Out cmd = %d, type = %d\n", cmd, type);
	} else {
		if (type == E_VBI_TELETEXT)
			HWREG_VBI_TTXInit();

		enable_irq((int)vbi_interrupt_irq);
	}

	SRCCAP_VBI_MSG_DEBUG("Out cmd = %d, type = %d\n", cmd, type);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief               : This function is used to set the Decoder DSP ISR
 *@param
 *@return
 *  - V4L2_EXT_VBI_OK  : OK
 *  - V4L2_EXT_VBI_FAIL: fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_isr(int eIntNum)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get VBI Driver Status
 *@param pDrvStatus     : [OUT] driver status
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_get_status(VBI_DrvStatus *pDrvStatus)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Initialize VBI
 *@param type           : [IN] VBI type (Teletext/CC)
 *@return
 *  - V4L2_EXT_VBI_OK   : initialize VBI successfully.
 *  - V4L2_EXT_VBI_FAIL : initialize VBI unsuccessfully.
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_init(VBI_INIT_TYPE type)
{
	SRCCAP_VBI_MSG_DEBUG("In type = %d\n", type);
	return _mtk_vbi_init(VBI_INIT, type);
}


/****************************************************************************
 *@brief                : VBI Exit
 *@return
 *  - V4L2_EXT_VBI_OK   : VBI exit successfully.
 *  - V4L2_EXT_VBI_FAIL : VBI exit unsuccessfully.
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_exit(EN_VBI_CMD cmd)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Set Callback to VBI driver,
 *                        the CB will be called if VBI interrupt is catched
 *@param pFN            : [IN] call back function pointer.
 *@param bufferAddr     : [IN] the physical address of buffer
 *                             which will store VBI data and pass pointer to CB
 *@param length         : [IN] the data lebgth of th input buffer.
 *                             Note: it should be the multiple of VBI_TTX_PKT_SIZE
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_register_cb(VBI_CB_FN pFN, MS_PHY bufferAddr, MS_U32 length)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief : Get VBI Information
 *@param
 *@return
 *@see VBI_DrvInfo
 ****************************************************************************/
const VBI_DrvInfo *mtk_vbi_get_info(void)
{
	SRCCAP_VBI_MSG_DEBUG("In &_drvInfo=%d\n", &_drvInfo);
	return &_drvInfo;
}


/****************************************************************************
 *@brief                : Set VBI Debug Level
 *@param u16DbgSwitch   : [IN] debug level
 *@return
 *  - V4L2_EXT_VBI_OK   : set debug level successfully.
 *  - V4L2_EXT_VBI_FAIL : set debug level unsuccessfully.
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_set_dbg_level(MS_U16 u16DbgSwitch)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Reset TTX Ring Buffer
 *@param
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ring_buffer_reset(void)
{
	SRCCAP_VBI_MSG_DEBUG("In\n");
	// TODO: need to refactory MS_PA2KSEG1
//	memset((MS_U8 *) MS_PA2KSEG1(_pStoreInfo._TTXBufAddr),
//	    0x20, _pStoreInfo._TTXBufLen*VBI_TTX_PKT_SIZE);

	mutex_lock(&vbi_mutex.mutex);

	rb_handle.ReadPacketNumber = HWREG_VBI_TTX_GetPacketCount();
	rb_handle.WritePacketNumber = 0;
	rb_handle.pkt_received = FALSE;
	rb_handle.PacketBufferOverflow = FALSE;
	rb_handle.NoOfPacketBufferOverflows = 0;

	mutex_unlock(&vbi_mutex.mutex);

	SRCCAP_VBI_MSG_DEBUG("Out\n");
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Initialize TTX Slicer
 *@param bufferAddr     : [IN] TTX buffer address
 *@param packetCount    : [IN] TTX packet count
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_initialize_slicer(MS_PHY bufferAddr, MS_U16 packetCount)
{
	SRCCAP_VBI_MSG_DEBUG("In  bufferAddr = 0x%llx, packetCount = %d\n",
		    bufferAddr, packetCount);
	if (_pStoreInfo._bTTXSupported == FALSE) {
		SRCCAP_VBI_MSG_ERROR("TTX not support\n");
		return V4L2_EXT_VBI_FAIL;
	}

	HWREG_VBI_TTX_InitSlicer(bufferAddr, packetCount);
	_pStoreInfo._TTXBufAddr = bufferAddr;
	_pStoreInfo._TTXBufLen = packetCount;
	mtk_vbi_ring_buffer_reset();

	SRCCAP_VBI_MSG_DEBUG("Out bufferAddr = 0x%llx, packetCount = %d\n",
		    bufferAddr, packetCount);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Enable TTX Slicer
 *@param bEnable        : [IN] enable or disable TTX slicer
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_enable_slicer(bool bEnable)
{
	SRCCAP_VBI_MSG_DEBUG("In bEnable = %d\n", bEnable);
	if (_pStoreInfo._bTTXSupported == FALSE) {
		SRCCAP_VBI_MSG_ERROR("TTX not support\n");
		return V4L2_EXT_VBI_FAIL;
	}

	HWREG_VBI_TTX_EnableSlicer(bEnable);

	SRCCAP_VBI_MSG_DEBUG("Out bEnable = %d\n", bEnable);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : VPS Is Ready or Not
 *@param bEnable        : [OUT] VPS is ready or not
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_is_vps_ready(bool *bEnable)
{
	SRCCAP_VBI_MSG_DEBUG("In bEnable = %d\n", *bEnable);
	MS_U8 regval = 0;

	regval = HWREG_VBI_TTX_GetHardware_Indication();

	if (regval & _BIT4) {
		*bEnable = TRUE;
		SRCCAP_VBI_MSG_DEBUG("Out bEnable = %d\n", *bEnable);
	} else {
		*bEnable = FALSE;
		SRCCAP_VBI_MSG_DEBUG("Out  bEnable = %d\n", *bEnable);
	}
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : TTX Is Ready or Not
 *@param bEnable        : [OUT] TTX is ready or not
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_is_ttx_ready(bool *bEnable)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : WSS Is Ready or Not
 *@param bReady         : [OUT] WSS is ready or not
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_is_wss_ready(bool *bReady)
{
	SRCCAP_VBI_MSG_DEBUG("In  bEnable = %d\n", *bReady);
	MS_U8 regval = 0;

	regval = HWREG_VBI_TTX_GetHardware_Indication();

	if (regval & _BIT6) {
		*bReady = TRUE;
		SRCCAP_VBI_MSG_DEBUG("Out  bEnable = %d\n", *bReady);
	} else {
		*bReady = FALSE;
		SRCCAP_VBI_MSG_DEBUG("Out  bEnable = %d\n", *bReady);
	}
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get WSS Data
 *@param pDrvStatus     : [OUT] WSS data
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_get_wss_data(MS_U16 *u16WssData)
{
	SRCCAP_VBI_MSG_DEBUG("In u16WssData = %d\n", *u16WssData);

	*u16WssData = HWREG_VBI_GetWSS_Data();
	SRCCAP_VBI_MSG_DEBUG("Out u16WssData = %d\n", *u16WssData);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get VPS Data
 *@param lowerWord      : [OUT] VPS lower data
 *@param higherWord     : [OUT] VPS higher data
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_get_vps_data(MS_U8 *lowerWord, MS_U8 *higherWord)
{
	SRCCAP_VBI_MSG_DEBUG("In lowerWord = %d, higherWord = %d\n",
		    *lowerWord, *higherWord);
	MS_U8 ucLoop;
	MS_U8 ucVpsDataH, ucVpsDataL;
	MS_U8 VpsByte1, VpsByte2, VpsByte3, VpsByte4;

	HWREG_VBI_GetVPS_Data(&VpsByte1, &VpsByte2, &VpsByte3, &VpsByte4);

	ucVpsDataH = 0x00;
	ucVpsDataL = 0x00;

	// extract byte 14[2:7] as ucVpsDataL[5:0]
	for (ucLoop = _BIT7; ucLoop != _BIT1; ucLoop >>= 1) {
		ucVpsDataL >>= 1;
		if (VpsByte2 & ucLoop)
			ucVpsDataL |= _BIT7;
	}			// for

	// extract byte 11[0:1] as ucVpsDataL[7:6]
	for (ucLoop = _BIT1; ucLoop != 0; ucLoop >>= 1) {
		ucVpsDataL >>= 1;
		if (VpsByte4 & ucLoop)
			ucVpsDataL |= _BIT7;
	}			// for

	// extract byte 14[0:1] as ucVpsDataH[1:0]
	for (ucLoop = _BIT1; ucLoop != 0; ucLoop >>= 1)	{
		ucVpsDataH >>= 1;
		if (VpsByte2 & ucLoop)
			ucVpsDataH |= _BIT3;
	}			// for

	// extract byte-13[6:7] as ucVpsDataH[3:2]
	for (ucLoop = _BIT7; ucLoop != _BIT5; ucLoop >>= 1)	{
		ucVpsDataH >>= 1;
		if (VpsByte1 & ucLoop)
			ucVpsDataH |= _BIT3;
	}			// for

	*lowerWord = ucVpsDataL;
	*higherWord = ucVpsDataH;

	SRCCAP_VBI_MSG_DEBUG("Out  lowerWord = %d, higherWord = %d\n",
		    *lowerWord, *higherWord);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Set Video Standard
 *@param eStandard      : [IN] type (NTSC/PAL/SECAM)
 *@see VBI_VIDEO_STANDARD
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_set_video_standard(VBI_VIDEO_STANDARD eStandard)
{
	SRCCAP_VBI_MSG_DEBUG("In eStandard = %d\n", eStandard);

	switch (eStandard) {
	case VBI_VIDEO_SECAM:
		HWREG_VBI_Set_Secam_VideoStandard();
		break;

	case VBI_VIDEO_PAL_NC:
		HWREG_VBI_Set_PalNC_VideoStandard();
		break;

	default:
		SRCCAP_VBI_MSG_DEBUG("not support standard 0x%x, use PAL instead\n",
			(unsigned int)eStandard);
		HWREG_VBI_Set_Pal_VideoStandard();
		break;
	}

	SRCCAP_VBI_MSG_DEBUG("Out eStandard = %d\n", eStandard);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Check if the TTX packet in VBI buffer is empty
 *@param
 *@return
 *  - V4L2_EXT_VBI_OK   : is empty
 *  - V4L2_EXT_VBI_FAIL : not empty
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_packet_buffer_is_empty(bool *IsEmpty)
{
	SRCCAP_VBI_MSG_DEBUG("In\n");
	*IsEmpty = TRUE;

	// if overflow or slicer is not ready, define it is buffer empty
	if (_mtk_vbi_ttx_update_write_pointer() == FALSE) {
		SRCCAP_VBI_MSG_DEBUG("Out\n");
		*IsEmpty = TRUE;
	}

	if (rb_handle.WritePacketNumber - rb_handle.ReadPacketNumber) {	/* no of packets */
		SRCCAP_VBI_MSG_DEBUG("Out\n");
		*IsEmpty = FALSE;
	}

	SRCCAP_VBI_MSG_DEBUG("Out\n");
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Check TTX circuit ready
 *@param
 *@return
 *  - V4L2_EXT_VBI_OK   : TTX circuit is ready.
 *  - V4L2_EXT_VBI_FAIL : TTX circuit is not ready.
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_check_circuit_ready(void)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get TTX Packet Count
 *@param packet_count   : [OUT] TTX packet count
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_packet_count(MS_U16 *u16PacketCount)
{
	SRCCAP_VBI_MSG_DEBUG("In u16PacketCount = 0x%x\n", *u16PacketCount);
	if (_pStoreInfo._bTTXSupported == FALSE) {
		*u16PacketCount = 0;
		SRCCAP_VBI_MSG_ERROR("Out u16PacketCount = 0x%x\n",
			    *u16PacketCount);
		return V4L2_EXT_VBI_FAIL;
	}

	*u16PacketCount = HWREG_VBI_TTX_GetPacketCount();
	SRCCAP_VBI_MSG_DEBUG("u16PacketCount = 0x%x\n", *u16PacketCount);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get TTX Information
 *@param u32TypeSelect  : [IN] TTX information type
 *@param u32Info        : [OUT] TTX information
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_info(MS_U32 u32TypeSelect, MS_U32 *u32Info)
{
	SRCCAP_VBI_MSG_DEBUG("In u32TypeSelect = %d, u32Info = %d\n",
		    u32TypeSelect, *u32Info);

	enum V4L2_VBI_RESULT ret = V4L2_EXT_VBI_FAIL;

	switch (u32TypeSelect) {
	case VBI_TTX_PACKET_SIZE:
		*u32Info = (MS_U32) VBI_TTX_PKT_SIZE;
		SRCCAP_VBI_MSG_DEBUG("Out  u32TypeSelect = %d, u32Info = %d\n",
			    u32TypeSelect, *u32Info);
		ret = V4L2_EXT_VBI_OK;
		break;

	default:
		break;
	}

	SRCCAP_VBI_MSG_DEBUG("Out  u32TypeSelect = %d, u32Info = %d\n",
		    u32TypeSelect, *u32Info);
	return ret;
}


/****************************************************************************
 *@brief                : Get TTX Packets
 *@param dataAddr       : [IN] data address
 *@param length         : [OUT] packet length
 *@return
 *  - V4L2_EXT_VBI_OK   : get TTX packets successfully.
 *  - V4L2_EXT_VBI_FAIL : get TTX packets unsuccessfully.
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_packets(MS_PHY dataAddr, MS_U32 *length)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get TTX Packet
 *@param packetAddress  : [OUT] packet address
 *@return
 *  - V4L2_EXT_VBI_OK   : get TTX packet successfully.
 *  - V4L2_EXT_VBI_FAIL : get TTX packet unsuccessfully.
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_get_packet(MS_PHY *packetAddress, MS_U8 *u8Success)
{
	SRCCAP_VBI_MSG_DEBUG("In packetAddress = 0x%llx\n", *packetAddress);
	enum V4L2_VBI_RESULT ret = V4L2_EXT_VBI_FAIL;
	bool bIsEmpty = FALSE;

	mutex_lock(&vbi_mutex.mutex);
	*u8Success = FALSE;
	mtk_vbi_ttx_packet_buffer_is_empty(&bIsEmpty);

	if (bIsEmpty == FALSE) {
		rb_handle.ReadPacketNumber++;

		// Venus, 0 -> 1 -> 2 ->3 -> N-1 -> N -> 1 ->2 -> 3 ...
		if (rb_handle.ReadPacketNumber > _pStoreInfo._TTXBufLen)
			rb_handle.ReadPacketNumber = 1;

		*packetAddress = (MS_PHY) (rb_handle.ReadPacketNumber - 1) * VBI_TTX_PKT_SIZE;

		*u8Success = TRUE;
	}

	mutex_unlock(&vbi_mutex.mutex);
	SRCCAP_VBI_MSG_DEBUG("Out packetAddress = 0x%llx\n", *packetAddress);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Check if there is a packet buffer overflow. If there is an overflow,
 *                        the packet buffer should be cleared from the reading task.
 *@param bIsOverflow    : [OUT] TTX buffer is overflow or not.
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_packet_buffer_is_overflow(bool *bIsOverflow)
{
	SRCCAP_VBI_MSG_DEBUG("In bIsOverflow = %d\n", *bIsOverflow);
	*bIsOverflow = rb_handle.PacketBufferOverflow;
	SRCCAP_VBI_MSG_DEBUG("Out bIsOverflow = %d\n", *bIsOverflow);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Resuren the number of packet buffer overflows
 *                        since the last reset or creation.
 *@param u16NoOfPacketBufferOverflows : [OUT] The number of packet buffer overflows.
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_packet_buffer_get_no_of_overflows(MS_U16 *
								   u16NoOfPacketBufferOverflows)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Set TTX Enable Line
 *@param StartLine      : [IN] TTX start line
 *@param EndLine        : [IN] TTX end line
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_ttx_enable_line(MS_U16 StartLine, MS_U16 EndLine)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Initialize CC Slicer
 *@param u32RiuAddr     : [IN] VBI CC RIU address
 *@param bufferAddr     : [IN] VBI CC buffer address
 *@param packetCount    : [IN] VBI CC packet count
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_init_slicer(MS_VIRT u32RiuAddr, MS_PHY bufferAddr,
					    MS_U16 packetCount)
{
	SRCCAP_VBI_MSG_DEBUG
	    ("In u32RiuAddr = 0x%llx, bufferAddr = 0x%llx, packetCount = %d\n",
			u32RiuAddr, bufferAddr, packetCount);
	MS_PHY phyNonPMBankSize;

	if (bufferAddr != 0 && packetCount != 0) {
		_pStoreInfo._CCBufAddr = bufferAddr;
		_pStoreInfo._CCBufLen = packetCount;

		HWREG_VBI_CC_Init(bufferAddr, packetCount);
	}

	SRCCAP_VBI_MSG_DEBUG
	    ("Out u32RiuAddr = 0x%llx, bufferAddr = 0x%llx, packetCount = %d\n",
			u32RiuAddr, bufferAddr, packetCount);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Enable CC Slicer
 *@param bEnable        : [IN] enable or disable CC slicer
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_enable_slicer(bool bEnable)
{
	SRCCAP_VBI_MSG_DEBUG("In bEnable = %d\n", bEnable);
	HWREG_VBI_CC_EnableSlicer(bEnable);
	SRCCAP_VBI_MSG_DEBUG("Out bEnable = %d\n", bEnable);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Set CC Data Rate
 *@param ptable         : [OUT] data rate table
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_set_data_rate(MS_U8 *ptable)
{
	SRCCAP_VBI_MSG_DEBUG("In\n");
	if (ptable != NULL) {
		if (HWREG_VBI_CC_DataRateSet(ptable) == TRUE) {
			return V4L2_EXT_VBI_OK;
		}
	}

	return V4L2_EXT_VBI_FAIL;
}


/****************************************************************************
 *@brief                : Set CC SC window length
 *@param u8Len          : [IN] windows length
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_set_sc_window_len(MS_U8 u8Len)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Set CC standard
 *@param eStandard      : [IN] video standard
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_set_standard(VBI_VIDEO_STANDARD eStandard)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get CC Information
 *@param u32selector    : [IN] CC function select
 *@param u32PacketCount : [OUT] packet count or indication
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_get_info(MS_U32 u32selector, MS_U32 *u32PacketCount)
{
	SRCCAP_VBI_MSG_DEBUG("In selector = %d, u32PacketCount = %d\n",
		    u32selector, *u32PacketCount);

	enum V4L2_VBI_RESULT ret = V4L2_EXT_VBI_FAIL;

	// Functionality select
	switch (u32selector) {
	case VBI_CC_PACKET_COUNT:
		*u32PacketCount = ((MS_U32) HWREG_VBI_CC_GetPacketCnt());
		SRCCAP_VBI_MSG_DEBUG("Out  selector = %d, u32PacketCount = %d\n",
				u32selector, *u32PacketCount);
		ret = V4L2_EXT_VBI_OK;
		break;

	case VBI_CC_BYTE_FOUND_INDICATION:
		*u32PacketCount = ((MS_U32) HWREG_VBI_CC_GetByteFoundIndication());
		SRCCAP_VBI_MSG_DEBUG("Out selector = %d, u32PacketCount = %d\n",
				u32selector, *u32PacketCount);
		ret = V4L2_EXT_VBI_OK;
		break;

	case VBI_CC_DATA_GET:
		// odd_filed_data + even_field_data
		*u32PacketCount = ((MS_U32) HWREG_VBI_CC_GetPacket());
		SRCCAP_VBI_MSG_DEBUG("Out selector = %d, u32PacketCount = %lx\n",
				u32selector, *u32PacketCount);
		ret = V4L2_EXT_VBI_OK;
		break;

	case VBI_CC_PACKET_SIZE:
		*u32PacketCount = ((MS_U32) VBI_CC_PKT_SIZE);
		SRCCAP_VBI_MSG_DEBUG("Out selector = %d, u32PacketCount = %d\n",
				u32selector, *u32PacketCount);
		ret = V4L2_EXT_VBI_OK;
		break;

	default:
		break;
	}

	SRCCAP_VBI_MSG_DEBUG("Out selector=%d, u32PacketCount=%d\n",
		    u32selector, *u32PacketCount);
	return ret;
}


/****************************************************************************
 *@brief                : Set CC Frame Count
 *@param cnt            : [IN] frame count
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_set_frame_cnt(MS_U8 cnt)
{
	SRCCAP_VBI_MSG_DEBUG("In  cnt = %d\n", cnt);
	HWREG_VBI_SetCCFrameCnt(cnt);
	SRCCAP_VBI_MSG_DEBUG("Out  cnt = %d\n", cnt);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Enable CC Line
 *@param StartLine      : [IN] start line
 *@param EndLine        : [IN] end line
 *@param mode           : [IN] NTSC/PAL/SECAM mode
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_cc_enable_line(MS_U16 StartLine, MS_U16 EndLine, MS_U8 mode)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Set WSS/VPS Byte Number
 *@param cnt            : [IN] byte number
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_wss_set_vps_byte_num(MS_U8 cnt)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Get WSS Packet Count
 *@param u16PacketCount : [OUT] WSS packet count
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_wss_get_packet_count(MS_U16 *u16PacketCount)
{
	SRCCAP_VBI_MSG_DEBUG("In u16PacketCount = %d\n", *u16PacketCount);
	*u16PacketCount = HWREG_VBI_GetWSS_Count();
	SRCCAP_VBI_MSG_DEBUG("Out u16PacketCount = %d\n", *u16PacketCount);
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Suspend VBI driver
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 *@note
 *@Save VBI driver states to DRAM
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_suspend(void)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                :  Resume VBI driver
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 *@note
 *@Save VBI driver states to DRAM
 *@Restore VBI driver states from DRAM
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_resume(void)
{
	return V4L2_EXT_VBI_OK;
}


/****************************************************************************
 *@brief                : Set VBI suspend & resume
 *@param u16PowerState  : [IN] VBI power state
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 *@note
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_set_power_state(EN_POWER_MODE u16PowerState)
{
	return V4L2_EXT_VBI_OK;
}

enum V4L2_VBI_RESULT mtk_vbi_get_raw_vps_data(MS_U8 *byte0, MS_U8 *byte1, MS_U8 *byte2,
					      MS_U8 *byte3)
{
	SRCCAP_VBI_MSG_DEBUG("In  byte0 = %d, byte1 = %d, byte2 = %d, byte3 = %d\n",
			byte0, byte1, byte2, byte3);

	HWREG_VBI_GetVPS_Data(byte0, byte1, byte2, byte3);

	SRCCAP_VBI_MSG_DEBUG("Out byte0 = %d, byte1 = %d, byte2 = %d, byte3 = %d\n",
			byte0, byte1, byte2, byte3);
	return V4L2_EXT_VBI_OK;
}

#if TEST_MODE
/****************************************************************************
 *@brief                : restrict vbi memory buffer range
 *@phyAddr              : start address of the range
 *@u32Size              : size of the range
 *@return
 *  - V4L2_EXT_VBI_OK   : OK
 *  - V4L2_EXT_VBI_FAIL : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_protect_memory(bool bEnable, MS_PHY phyAddr, MS_U32 u32Size)
{
	SRCCAP_VBI_MSG_DEBUG("In bEnable = %d, phyAddr = 0x%llx, u32Size = %d\n",
			bEnable, phyAddr, u32Size);

	enum V4L2_VBI_RESULT ret = V4L2_EXT_VBI_FAIL;

	if (HWREG_VBI_ProtectMemory(bEnable, phyAddr, u32Size) == TRUE) {
		SRCCAP_VBI_MSG_DEBUG("Out bEnable = %d, phyAddr = 0x%llx, u32Size = %d\n",
			bEnable, phyAddr, u32Size);
		ret = V4L2_EXT_VBI_OK;
	} else {
		SRCCAP_VBI_MSG_DEBUG("Out bEnable = %d, phyAddr = 0x%llx, u32Size = %d\n",
			bEnable, phyAddr, u32Size);
		ret = V4L2_EXT_VBI_FAIL;
	}
	return ret;
}


/****************************************************************************
 *@brief                : Fresh memory to avoid cache coherence issue
 *@param  u32Start      : [IN] start address (must be 16-B aligned and in cacheable area)
 *@param  u32Size       : [IN] size (must be 16-B aligned)
 *@return
 *  - V4L2_EXT_VBI_OK    : OK
 *  - V4L2_EXT_VBI_FAIL  : fail
 ****************************************************************************/
enum V4L2_VBI_RESULT mtk_vbi_sync_memory(MS_U32 u32Start, MS_U32 u32Size)
{
	SRCCAP_VBI_MSG_DEBUG("In u32Start = %d, u32Size = %d\n", u32Start,
		    u32Size);

	enum V4L2_VBI_RESULT ret = V4L2_EXT_VBI_FAIL;

	MsOS_ReadMemory();
	if (MsOS_Dcache_Flush(u32Start, u32Size) == TRUE) {
		SRCCAP_VBI_MSG_DEBUG("Out u32Start = %d, u32Size = %d\n",
			    u32Start, u32Size);
		ret = V4L2_EXT_VBI_OK;
	} else {
		SRCCAP_VBI_MSG_ERROR("Out u32Start = %d, u32Size = %d\n",
			    u32Start, u32Size);
		ret = V4L2_EXT_VBI_FAIL;
	}
	return ret;
}

#endif
