/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)*/
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _VOC_HAL_MAILBOX_H
#define _VOC_HAL_MAILBOX_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include "voc_common.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define ENABLE_CM4_CODE 0

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Enumerate And Structure
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
#if ENABLE_CM4_CODE
#ifdef _HAL_MBX_C
#define INTERFACE
#else
#define INTERFACE extern
#endif

//=============================================================================
// Includs
//=============================================================================

//=============================================================================
// Defines & Macros

//=============================================================================
//busy bit Set/Clear/Get
#define   _BUSY_S(arg)  {\
		U8 val; \
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val | MBX_STATE1_BUSY;\
	}

#define   _BUSY_C(arg)  {\
		U8 val; \
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val & ~MBX_STATE1_BUSY;\
	}

#define   _BUSY(arg)    (REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) & MBX_STATE1_BUSY)

//////////////////////////////////////////////////////////////
//error bit Set/Clear/Get
#define   _ERR_S(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val | MBX_STATE1_ERROR;\
	}

#define   _ERR_C(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val & ~MBX_STATE1_ERROR;\
	}

#define   _ERR(arg)    (REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) & MBX_STATE1_ERROR)

//////////////////////////////////////////////////////////////
//disabled bit Set/Clear/Get
#define   _DISABLED_S(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val | MBX_STATE1_DISABLED;\
	}

#define   _DISABLED_C(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val & ~MBX_STATE1_DISABLED;\
	}

#define   _DISABLED(arg)    (REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) & MBX_STATE1_DISABLED)

////////////////////////////////////////////////////////////////////////
//overflow bit Set/Clear/Get
#define   _OVERFLOW_S(arg)  {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val | MBX_STATE1_OVERFLOW;\
	}

#define   _OVERFLOW_C(arg)  {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val & ~MBX_STATE1_OVERFLOW;\
	}

#define   _OVERFLOW(arg)   (REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) & MBX_STATE1_OVERFLOW)

////////////////////////////////////////////////////////////////////////
//status bit clear
#define   _S1_C(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_1);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_1) = val & ~(MBX_STATE1_DISABLED |\
				MBX_STATE1_OVERFLOW |\
				MBX_STATE1_ERROR |\
				MBX_STATE1_BUSY);\
	}

////////////////////////////////////////////////////////////////////////
//fire bit Set/Clear/Get
#define   _FIRE_S(arg)  {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_CTRL);\
		REG8_MBX_GROUP(arg, REG8_MBX_CTRL) = val | MBX_CTRL_FIRE;\
	}

#define   _FIRE_C(arg)  {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_CTRL);\
		REG8_MBX_GROUP(arg, REG8_MBX_CTRL) = val & ~MBX_CTRL_FIRE;\
	}

#define   _FIRE(arg)   (REG8_MBX_GROUP(arg, REG8_MBX_CTRL) & MBX_CTRL_FIRE)

////////////////////////////////////////////////////////////////////////
//readback bit Set/Clear/Get
#define   _READBACK_S(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_CTRL);\
		REG8_MBX_GROUP(arg, REG8_MBX_CTRL) = val | MBX_CTRL_READBACK;\
	}

#define   _READBACK_C(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_CTRL);\
		REG8_MBX_GROUP(arg, REG8_MBX_CTRL) = val & ~MBX_CTRL_READBACK;\
	}

#define   _READBACK(arg)   (REG8_MBX_GROUP(arg, REG8_MBX_CTRL) & MBX_CTRL_READBACK)

////////////////////////////////////////////////////////////////////////
//instant bit Set/Clear/Get
#define   _INSTANT_S(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_CTRL);\
		REG8_MBX_GROUP(arg, REG8_MBX_CTRL) = val | MBX_CTRL_INSTANT;\
	}

#define   _INSTANT_C(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_CTRL);\
		REG8_MBX_GROUP(arg, REG8_MBX_CTRL) = val & ~MBX_CTRL_INSTANT;\
	}

#define   _INSTANT(arg)   (REG8_MBX_GROUP(arg, REG8_MBX_CTRL) & MBX_CTRL_INSTANT)



//////////////////////////////////////////////////////////////
//state0 error bit Set/Clear/Get
#define   _STATE0_ERR_S(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_0);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_0) = val | MBX_STATE0_ERROR;\
	}

#define   _STATE0_ERR_C(arg)   {\
		U8 val;\
		val = REG8_MBX_GROUP(arg, REG8_MBX_STATE_0);\
		REG8_MBX_GROUP(arg, REG8_MBX_STATE_0) = val & ~MBX_STATE0_ERROR;\
	}

#define   _STATE0_ERR(arg)    (REG8_MBX_GROUP(arg, REG8_MBX_STATE_0) & MBX_STATE0_ERROR)

//=============================================================================
// Type and Structure Declaration
//=============================================================================

//=============================================================================
// Enums
/// MBX HAL Recv Status Define
enum MBXHAL_Recv_Status {
	/// Recv Success
	E_MBXHAL_RECV_SUCCESS = 0,
	/// Recv Error: OverFlow
	E_MBXHAL_RECV_OVERFLOW = 1,
	/// Recv Error: Not Enabled
	E_MBXHAL_RECV_DISABLED = 2,
};

/// MBX HAL Fire Status Define
enum MBXHAL_Fire_Status {
	/// Fire Success
	E_MBXHAL_FIRE_SUCCESS = 0,
	/// Still Firing
	E_MBXHAL_FIRE_ONGOING = 1,
	/// Fire Error: Overflow:
	E_MBXHAL_FIRE_OVERFLOW = 2,
	/// Fire Error: Not Enabled
	E_MBXHAL_FIRE_DISABLED = 3,
};

/// MBX HAL Fire Cmd Status Define
enum MBXHAL_Cmd_Status {
	/// Cmd Success
	E_MBXHAL_CMD_SUCCESS = 0,
	/// Cmd Error
	E_MBXHAL_CMD_ERROR = 1,
};

//=============================================================================
// Mailbox HAL Driver Function
//=============================================================================

INTERFACE MBX_Result MHAL_VOC_MBX_ClearAll(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole);

INTERFACE MBX_Result MHAL_VOC_MBX_Init(MBX_ROLE_ID eHostRole);
INTERFACE MBX_Result MHAL_VOC_MBX_SetConfig(MBX_ROLE_ID eHostRole);

INTERFACE MBX_Result MHAL_VOC_MBX_Fire(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole);
INTERFACE void MHAL_VOC_MBX_ClearStatus(MBX_Msg *pMbxMsg, MBX_ROLE_ID eSrcRole);
INTERFACE MBX_Result MHAL_VOC_MBX_GetFireStatus(MBX_ROLE_ID eSrcRole,
				MBX_ROLE_ID eDstRole,
				MBXHAL_Fire_Status *pFireStatus);
INTERFACE MBX_Result MHAL_VOC_MBX_GetCmdStatus(MBX_ROLE_ID eSrcRole,
				MBX_ROLE_ID eDstRole,
				MBXHAL_Cmd_Status *pCmdStatus);

INTERFACE MBX_Result MHAL_VOC_MBX_Recv(MBX_Msg *pMbxMsg,
				MBX_ROLE_ID eDstRole);
INTERFACE MBX_Result MHAL_VOC_MBX_RecvEnd(MBX_ROLE_ID eSrcRole,
				MBX_ROLE_ID eDstRole, MBXHAL_Recv_Status eRecvSatus);
INTERFACE MBX_Result MHAL_VOC_MBX_RecvEndExt(MBX_ROLE_ID eSrcRole,
				MBX_ROLE_ID eDstRole, BOOL bSuccess);

#undef INTERFACE
#endif
#endif /* _VOC_HAL_MAILBOX_H */
