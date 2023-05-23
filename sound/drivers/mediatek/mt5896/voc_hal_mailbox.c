// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <sound/soc.h>
#include "voc_common.h"
#include "voc_hal_mailbox.h"

#if ENABLE_CM4_CODE
//=============================================================================
// Local Defines
//=============================================================================
#define MBX_BASE_ADDR         REG_ADDR_BASE_MAILBOX
#define MBX_GROUP(gid)        GET_REG16_ADDR(MBX_BASE_ADDR, gid)
#define MBX_REG8(gid, addr)   INREG8(GET_REG8_ADDR(MBX_GROUP(gid), addr))

//Reg8 defines:
#define REG8_MBX_CTRL                 0x0000
	#define MBX_CTRL_FIRE                 BIT(0)
	#define MBX_CTRL_READBACK             BIT(1)
	#define MBX_CTRL_INSTANT              BIT(2)
#define REG8_MBX_MAIL_CLASS           0x0001
#define REG8_MBX_MAIL_IDX             0x0002
#define REG8_MBX_PARAMETER_CNT        0x0003
#define REG8_MBX_PARAMETER_S          0x0004
#define REG8_MBX_PARAMETER_E          0x000D
#define REG8_MBX_STATE_0              0x000E
	#define MBX_STATE0_ERROR              BIT(7)
#define REG8_MBX_STATE_1              0x000F
	#define MBX_STATE1_DISABLED           BIT(4)
	#define MBX_STATE1_OVERFLOW           BIT(5)
	#define MBX_STATE1_ERROR              BIT(6)
	#define MBX_STATE1_BUSY               BIT(7)

#define REG_MBX_GROUP0      0x00
#define REG_MBX_GROUP1      0x08
#define REG_MBX_GROUP2      0x10
#define REG_MBX_GROUP3      0x18
#define REG_MBX_GROUP4      0x20
#define REG_MBX_GROUP5      0x28

#define REG8_MBX_GROUP(gid, addr) MBX_REG8(gid, addr)

#define MBX_GROUPID_0XFF  0xFF
#define MBX_INDEX_MAX     16

//=============================================================================
// Debug Macros
//=============================================================================
#define MBXHAL_MSG(fmt, args...) MOS_DBG_PRINT(MOS_DBG_LEVEL_MBX, "[MBX] " fmt, ##args)
#define MBXHAL_ERROR(fmt, args...) MOS_DBG_ERROR("[MBX ERR] " fmt, ##args)

//=============================================================================
// Macros
//=============================================================================

//=============================================================================
// Local Variables
//=============================================================================
static U16 _u16MbxGroupIdMBXHAL[E_MBX_ROLE_MAX][E_MBX_ROLE_MAX] = {
	{MBX_GROUPID_0XFF, REG_MBX_GROUP0}, {REG_MBX_GROUP1, MBX_GROUPID_0XFF}};
//static MS_VIRT _virtRIUBaseAddrMBX = 0;
//=============================================================================
// Global Variables
//=============================================================================

//=============================================================================
// Local Function Prototypes
//=============================================================================
static void _MHAL_MBX_FireMsg(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole);
static void _MHAL_MBX_RecvMsg(MBX_Msg *pMbxMsg, MBX_ROLE_ID eDstRole);

//=============================================================================
// Local Function
//=============================================================================
//-------------------------------------------------------------------------------------------------
/// Fire Msg to MailBox hardware.
/// @param  eSrcCPUID                  \b IN: The Firer CPUID
/// @return void
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
void _MHAL_MBX_FireMsg(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole)
{
	S32 s32Idx;

	//fill mail box register.
	//REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcCPUID], REG8_MBX_CTRL) = pMbxMsg->u8Ctrl;
	REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID], REG8_MBX_MAIL_CLASS) =
		pMbxMsg->u8MsgClass;
	REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID], REG8_MBX_MAIL_IDX) =
		pMbxMsg->u8Index;
	REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID], REG8_MBX_PARAMETER_CNT) =
		pMbxMsg->u8ParameterCount;

	for (s32Idx = 0; s32Idx < pMbxMsg->u8ParameterCount; s32Idx++) {
		REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID],
				s32Idx+REG8_MBX_PARAMETER_S) = pMbxMsg->u8Parameters[s32Idx];
	}

	//REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcCPUID], REG8_MBX_STATE_0) = pMbxMsg->u8S0;
	//REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcCPUID], REG8_MBX_STATE_1) = pMbxMsg->u8S1;
}

//-------------------------------------------------------------------------------------------------
/// Recv Msg From MailBox hardware.
/// @param  pMbxMsg                  \b INOUT: The Recv CPUID, and where mail to put
/// @return void
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
void _MHAL_MBX_RecvMsg(MBX_Msg *pMbxMsg, MBX_ROLE_ID eDstRole)
{
	S32 s32Idx;

	pMbxMsg->u8Ctrl = REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole],
						REG8_MBX_CTRL);

	pMbxMsg->u8Index = REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole],
						REG8_MBX_MAIL_IDX);
	pMbxMsg->u8ParameterCount = REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole],
						REG8_MBX_PARAMETER_CNT);

	for (s32Idx = 0; s32Idx < pMbxMsg->u8ParameterCount; s32Idx++) {
		pMbxMsg->u8Parameters[s32Idx] = REG8_MBX_GROUP(
					_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole],
					s32Idx+REG8_MBX_PARAMETER_S);
	}

}

//=============================================================================
// Mailbox HAL Driver Function
//=============================================================================

//-------------------------------------------------------------------------------------------------
/// Init MailBox hardware.
/// @param  eHostRole                  \b IN: The host Role ID
/// @param  u32RIUBaseAddrMBX                  \b IN: The RIU Base Addr.
/// @return E_MBX_SUCCESS: success
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
MBX_Result MHAL_VOC_MBX_Init(MBX_ROLE_ID eHostRole)
{
	//_virtRIUBaseAddrMBX = virtRIUBaseAddrMBX;

	return MHAL_VOC_MBX_SetConfig(eHostRole);
}

//-------------------------------------------------------------------------------------------------
/// Set MailBox Group Regs.
/// @param  eHostRole                  \b IN: The host Role ID
/// @return E_MBX_SUCCESS: success
/// @attention
/// <b>[MXLIB] <em>Can't be called Before MHAL_VOC_MBX_Init </em></b>
//-------------------------------------------------------------------------------------------------
MBX_Result MHAL_VOC_MBX_SetConfig(MBX_ROLE_ID eHostRole)
{
	S32 s32MailIdx;

	//clear host mail box register.
	if (eHostRole == E_MBX_ROLE_HK) {//It is HouseKeeping
		for (s32MailIdx = 0; s32MailIdx < MBX_INDEX_MAX; s32MailIdx++) {
			REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[E_MBX_ROLE_HK][E_MBX_ROLE_CP],
			s32MailIdx) = 0x00;
		}
	} else {
		for (s32MailIdx = 0; s32MailIdx < MBX_INDEX_MAX; s32MailIdx++)
			REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eHostRole][E_MBX_ROLE_HK],
							s32MailIdx) = 0x00;

	}

	return E_MBX_SUCCESS;
}


MBX_Result MHAL_VOC_MBX_ClearAll(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole)
{
	U8 index = 0;

	for (index = 0; index < MBX_INDEX_MAX; index++)
		REG8_MBX_GROUP(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID], index) = 0x00;

	return E_MBX_SUCCESS;


}

//-------------------------------------------------------------------------------------------------
/// Fire Mail to MailBox hardware.
/// @param  eSrcCPUID                  \b IN: The Firer CPUID
/// @return E_MBX_ERR_PEER_CPU_NOTREADY: THE Peer CPU in fetching the msg
/// @return E_MBX_ERR_PEER_CPU_NOT_ALIVE: The Peer CPU Looks like not alive
/// @return E_MBX_SUCCESS: success
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
MBX_Result MHAL_VOC_MBX_Fire(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole)
{
	BOOL bIPState;

	if (_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID] == MBX_GROUPID_0XFF)
		return E_MBX_ERR_INVALID_PARAM;

	// check send bit in CONTROL register.
	bIPState = _FIRE(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);

	if (bIPState > 0) {
		bIPState = _BUSY(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);
		if (bIPState > 0) {
			//the mail is in process
			MBXHAL_ERROR("[Fire Mail Fail] : co-processor is not ready!\n");
			return  E_MBX_ERR_PEER_CPU_NOTREADY;
		}
		//mail not processed yet!
		MBXHAL_ERROR("[Fire Mail Fail] : co-processor is not alive!\n");
		return  E_MBX_ERR_PEER_CPU_NOT_ALIVE;
	}

	//clear status1 register.
	_S1_C(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);
	// Clear Instant Setting:
	_INSTANT_C(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);

	//fill mail box register.
	_MHAL_MBX_FireMsg(pMbxMsg, eSrcRole);

	//set instant message attribute
	if (pMbxMsg->eMsgType == E_MBX_MSG_TYPE_INSTANT)
		_INSTANT_S(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);

	_FIRE_S(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);

	return E_MBX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Clear HW  Status register
/// @param  eSrcRole                  \b IN: The Firer Src Role
/// @param  eDstRole                  \b IN: The Firer Dst Role
/// @param  pFireStatus               \b OUT: THE Fire Status
/// @return void
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
void MHAL_VOC_MBX_ClearStatus(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole)
{
	//clear status0 register.
	_STATE0_ERR_C(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);

	//clear status1 register.
	_S1_C(_u16MbxGroupIdMBXHAL[eSrcRole][pMbxMsg->eRoleID]);
}

//-------------------------------------------------------------------------------------------------
/// Get HW Fire Status
/// @param  eSrcRole                  \b IN: The Firer Src Role
/// @param  eDstRole                  \b IN: The Firer Dst Role
/// @param  pFireStatus               \b OUT: THE Fire Status
/// @return E_MBX_ERR_INVALID_PARAM
/// @return E_MBX_SUCCESS
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
MBX_Result MHAL_VOC_MBX_GetFireStatus(MBX_ROLE_ID eSrcRole,
		MBX_ROLE_ID eDstRole,
		MBXHAL_Fire_Status *pFireStatus)
{
	if ((_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole] == MBX_GROUPID_0XFF)
			|| (pFireStatus == NULL))
		return E_MBX_ERR_INVALID_PARAM;

	//still on Firing:
	if (_FIRE(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]))
		*pFireStatus = E_MBXHAL_FIRE_ONGOING;
	else if (_OVERFLOW(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]))
		*pFireStatus = E_MBXHAL_FIRE_OVERFLOW;//If Overflow:
	else if (_DISABLED(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]))
		*pFireStatus = E_MBXHAL_FIRE_DISABLED;//If Disabled:
	else
		*pFireStatus = E_MBXHAL_FIRE_SUCCESS;


	return E_MBX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Get Fire Command Status
/// @param  eSrcRole                  \b IN: The Firer Src Role
/// @param  eDstRole                  \b IN: The Firer Dst Role
/// @param  pFireStatus               \b OUT: THE Fire Command Status
/// @return E_MBX_ERR_INVALID_PARAM
/// @return E_MBX_SUCCESS
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
MBX_Result MHAL_VOC_MBX_GetCmdStatus(MBX_ROLE_ID eSrcRole,
					MBX_ROLE_ID eDstRole,
					MBXHAL_Cmd_Status *pCmdStatus)
{
	if ((_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole] == MBX_GROUPID_0XFF)
			|| (pCmdStatus == NULL))
		return E_MBX_ERR_INVALID_PARAM;

	//still on Firing:
	if (_STATE0_ERR(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]))
		*pCmdStatus = E_MBXHAL_CMD_ERROR;
	else
		*pCmdStatus = E_MBXHAL_CMD_SUCCESS;


	return E_MBX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// Recv Mail From MailBox hardware.
/// @param  pMbxMsg      \b INOUT: The Recv Src RoleID, and where mail to put
/// @param  eDstRole     \b IN: The Recv Dst RoleID
/// @return E_MBX_ERR_MSG_ALREADY_FETCHED: THE Msg has been fetched
/// @return E_MBX_ERR_INVALID_PARAM: THE Input Src Role & Dst Role is incorrect
/// @return E_MBX_SUCCESS: success
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
MBX_Result MHAL_VOC_MBX_Recv(MBX_Msg *pMbxMsg, MBX_ROLE_ID eDstRole)
{
	BOOL bIPState;

	if (_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole] == MBX_GROUPID_0XFF)
		return E_MBX_ERR_INVALID_PARAM;

	// check send bit in CONTROL register.
	bIPState = _FIRE(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole]);
	if (bIPState <= 0) //the message already fetched!
		return  E_MBX_ERR_MSG_ALREADY_FETCHED;

	//set busy bit
	_BUSY_S(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole]);

	bIPState = _INSTANT(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole]) |
				_READBACK(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole]);
	if (bIPState)
		pMbxMsg->eMsgType = E_MBX_MSG_TYPE_INSTANT;
	else
		pMbxMsg->eMsgType = E_MBX_MSG_TYPE_NORMAL;

	// recv mail msg
	_MHAL_MBX_RecvMsg(pMbxMsg, eDstRole);

	// clear busy bit
   // _BUSY_C(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole]);
   // clear send bit
   // _FIRE_C(_u16MbxGroupIdMBXHAL[pMbxMsg->eRoleID][eDstRole]);

	return E_MBX_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/// End Recv Mail From MailBox hardware. Set Status for FeedBack
/// @param  eSrcRole                  \b IN: The Recv Src RoleID
/// @param  eDstRole                  \b IN: The Recv Dst RoleID
/// @param  eRecvSatus                  \b IN: The Recv Status
/// @return E_MBX_ERR_INVALID_PARAM: THE Input Src Role & Dst Role is incorrect
/// @return E_MBX_SUCCESS: success
/// @attention
/// <b>[MXLIB] <em> </em></b>
//-------------------------------------------------------------------------------------------------
MBX_Result MHAL_VOC_MBX_RecvEnd(MBX_ROLE_ID eSrcRole,
								MBX_ROLE_ID eDstRole,
								MBXHAL_Recv_Status eRecvSatus)
{
	if (_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole] == MBX_GROUPID_0XFF) {
		//
		return E_MBX_ERR_INVALID_PARAM;
	}

	switch (eRecvSatus) {
	case E_MBXHAL_RECV_OVERFLOW:
		//Set OverFlow & Error:
		_OVERFLOW_S(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);
		_ERR_S(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);
		break;
	case E_MBXHAL_RECV_DISABLED:
		//Set Disable & Error:
		_DISABLED_S(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);
		_ERR_S(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);
		break;
	default:
		break;
	}

	//clear busy bit
	_BUSY_C(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);
	//clear send bit
	_FIRE_C(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);

	return E_MBX_SUCCESS;
}


MBX_Result MHAL_VOC_MBX_RecvEndExt(MBX_ROLE_ID eSrcRole, MBX_ROLE_ID eDstRole, BOOL bSuccess)
{
	if (_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole] == MBX_GROUPID_0XFF)
		return E_MBX_ERR_INVALID_PARAM;

	if (bSuccess == 0)
		_STATE0_ERR_S(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);

	//clear busy bit
	_BUSY_C(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);
	//clear send bit
	_FIRE_C(_u16MbxGroupIdMBXHAL[eSrcRole][eDstRole]);

	return E_MBX_SUCCESS;
}
#endif //ENABLE_CM4_CODE

