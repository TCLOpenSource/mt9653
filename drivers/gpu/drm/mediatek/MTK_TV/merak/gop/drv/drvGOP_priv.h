/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _DRVGOP_PRIV_H_
#define _DRVGOP_PRIV_H_

////////////////////////////////////////////////////////////////////////////////
// Header Files
////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif

#include "drvGFLIP.h"
#include "drvGOP.h"

typedef struct DLL_PACKED _GOP_RESOURCE_PRIVATE
{

} GOP_RESOURCE_PRIVATE;

typedef struct DLL_PACKED _GOP_INSTANT_PRIVATE
{
	void *pResource;
	GOP_CTX_DRV_LOCAL g_gopDrvCtxLocal;
	MS_GOP_CTX_LOCAL *g_pGOPCtxLocal;
} GOP_INSTANT_PRIVATE;


void GOPRegisterToUtopia(void);
MS_U32 GOPOpen(void **ppInstance, const void * const pAttribute);
MS_U32 GOPClose(void *pInstance);
MS_U32 GOPIoctl(void *pInstance, MS_U32 u32Cmd, void *pArgs);

#ifdef __cplusplus
}
#endif
#endif // _DRVBDMA_PRIV_H_
