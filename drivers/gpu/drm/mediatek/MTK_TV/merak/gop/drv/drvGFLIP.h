/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MDRV_GFLIP_H
#define _MDRV_GFLIP_H

#if defined(__cplusplus)
extern "C" {
#endif
#include "drvGOP.h"
#include "mdrv_gflip_st.h"

extern SYMBOL_WEAK MS_BOOL _MDrv_GFLIP_CSC_Calc(ST_GFLIP_GOP_CSC_PARAM *pstGflipCSCParam);
//=============================================================================
// Type and Structure Declaration
//=============================================================================

typedef struct __attribute__((__packed__))
{
	MS_GOP_CTX_SHARED apiCtxShared;
} GOP_CTX_DRV_SHARED;
typedef struct DLL_PACKED {
	MS_GOP_CTX_LOCAL  apiCtxLocal;
	GOP_CTX_DRV_SHARED *pDrvCtxShared;
	GOP_CHIP_PROPERTY       *pGopChipPro;
	MS_BOOL gop_gwin_frwr;
} GOP_CTX_DRV_LOCAL;

//=============================================================================
// Function
//=============================================================================
//Drv Interface related(drv interface):

#if defined(__cplusplus)
}
#endif

#undef INTERFACE

#endif //_MDRV_GFLIP_H

