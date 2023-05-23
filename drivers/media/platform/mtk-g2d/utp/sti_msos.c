// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/sched.h>

#include "sti_msos.h"

static struct UTOPIA_MODULE *g_ge_mudule_handle;
static struct semaphore sem;

#define UTOPIA_RESOURCE_NO_OCCUPY 0

void MsOS_DelayTask(MS_U32 u32Ms)
{
	msleep((u32Ms));
}

MS_U32 MsOS_GetSystemTime(void)
{
	struct timespec ts;

	getrawmonotonic(&ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

MS_S32 MsOS_GetOSThreadID(void)
{
	MS_S32 s32Tid = -1;
	struct task_struct *self = current;

	if (self)
		s32Tid = self->pid;

	return s32Tid;
}

void MsOS_CreateMutex(void)
{
	sema_init(&sem, 1);
}

MS_BOOL MsOS_ObtainMutex(void)
{
	int ret;

	ret = down_timeout(&sem, msecs_to_jiffies(0xffffff00));

	return ((ret == 0) ? TRUE : FALSE);
}

MS_BOOL MsOS_ReleaseMutex(void)
{
	up(&sem);
	return TRUE;
}

struct UTOPIA_RESOURCE {
	MS_BOOL bInUse;
	MS_U32 u32Pid;
	struct semaphore sem;
	void *pPrivate;
};

struct UTOPIA_MODULE {
	MS_U32 u32ModuleID;
	FUtopiaOpen fpOpen;
	FUtopiaClose fpClose;
	FUtopiaIOctl fpIoctl;
	void *pPrivate;
	struct UTOPIA_RESOURCE *psResource;
};

struct UTOPIA_INSTANCE {
	void *pPrivate;
	MS_U32 u32LastAccessTime;
	MS_U32 u32Pid;
	struct UTOPIA_MODULE *psModule;
};

MS_U32 UtopiaInstanceCreate(MS_U32 u32PrivateSize, void **ppInstance)
{
	struct UTOPIA_INSTANCE *pInstance = NULL;

	/* check param. */
	if (ppInstance == NULL) {
		printf("[utopia param error] instance ppointer should not be null\n");
		return -1;
	}

	pInstance = malloc(sizeof(struct UTOPIA_INSTANCE));
	if (pInstance != NULL) {
		memset(pInstance, 0, sizeof(struct UTOPIA_INSTANCE));
		if (u32PrivateSize != 0) {
			pInstance->pPrivate = malloc(u32PrivateSize);
			if (pInstance->pPrivate != NULL)
				memset(pInstance->pPrivate, 0, u32PrivateSize);
		}
		pInstance->u32Pid = current->tgid;
		*ppInstance = pInstance;
	}
	return 0;
}

MS_U32 UtopiaInstanceGetPrivate(void *pInstance, void **ppPrivate)
{
	MS_U32 u32Ret = UTOPIA_STATUS_SUCCESS;

	/* check param. */
	if (pInstance == NULL) {
		printf("[G2D][utopia param error] instance pointer should not be null\n");
		u32Ret = UTOPIA_STATUS_FAIL;
	} else if (ppPrivate == NULL) {
		printf("[G2D][utopia param error] private ppointer should not be null\n");
		u32Ret = UTOPIA_STATUS_FAIL;
	} else {
		*ppPrivate = ((struct UTOPIA_INSTANCE *)pInstance)->pPrivate;
	}
	return u32Ret;
}

MS_U32 UtopiaInstanceGetModule(void *pInstance, void **ppModule)
{
	MS_U32 u32Ret = UTOPIA_STATUS_SUCCESS;

	/* check param. */
	if (pInstance == NULL) {
		printf("[G2D][uopia param error] instance pointer should not be null\n");
		return UTOPIA_STATUS_FAIL;
	} else if (ppModule == NULL) {
		printf("[G2D][utopia param error] module ppointer should not be null\n");
		return UTOPIA_STATUS_FAIL;
	} else {
		*ppModule = ((struct UTOPIA_INSTANCE *)pInstance)->psModule;
	}

	/* check module pointer */
	if (*ppModule == NULL) {
		printf("[utopia param error] module pointer should not be null\n");
		printf("forgot to call UtopiaOpen first?\n");
		u32Ret = UTOPIA_STATUS_FAIL;
	}

	return u32Ret;
}

MS_U32 UtopiaInstanceDelete(void *pInstance)
{
	free(((struct UTOPIA_INSTANCE *)pInstance)->pPrivate);
	free(pInstance);

	return UTOPIA_STATUS_SUCCESS;
}

/*
 * assume one module for each driver
 * otherwise they have to pass module name as parameter
 */
MS_U32 UtopiaModuleCreate(MS_U32 u32ModuleID, MS_U32 u32PrivateSize, void **ppModule)
{
	MS_U32 u32Status;
	struct UTOPIA_MODULE *pModule = NULL;

	if (g_ge_mudule_handle == NULL) {
		pModule = malloc(sizeof(struct UTOPIA_MODULE));
		*ppModule = pModule;
		if (pModule == NULL) {
			printf("[utopia malloc error] Line: %d malloc fail\n", __LINE__);
			*ppModule = NULL;
			u32Status = UTOPIA_STATUS_FAIL;
		} else {
			memset(pModule, 0, sizeof(struct UTOPIA_MODULE));
			pModule->u32ModuleID = u32ModuleID;
			pModule->pPrivate = malloc(u32PrivateSize);
			*ppModule = pModule;
			g_ge_mudule_handle = pModule;
			u32Status = UTOPIA_STATUS_SUCCESS;
		}
	} else {
		*ppModule = g_ge_mudule_handle;
		u32Status = UTOPIA_STATUS_SUCCESS;
	}

	return u32Status;
}

MS_U32 UtopiaModuleGetPrivate(void *pModuleTmp, void **ppPrivate)
{
	struct UTOPIA_MODULE *pModule = (struct UTOPIA_MODULE *)pModuleTmp;
	*ppPrivate = pModule->pPrivate;
	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 UtopiaModuleRegister(void *pModuleTmp)
{
	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 UtopiaModuleSetupFunctionPtr(void *pModuleTmp, FUtopiaOpen fpOpen,
				    FUtopiaClose fpClose, FUtopiaIOctl fpIoctl)
{
	struct UTOPIA_MODULE *pModule = (struct UTOPIA_MODULE *)pModuleTmp;

	pModule->fpOpen = fpOpen;
	pModule->fpClose = fpClose;
	pModule->fpIoctl = fpIoctl;

	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 UtopiaModuleAddResourceStart(void *psModuleTmp, MS_U32 u32PoolID)
{
	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 UtopiaResourceCreate(char *resourceName, MS_U32 u32PrivateSize, void **ppResource)
{
	MS_U32 u32Status;
	struct UTOPIA_RESOURCE *pResource = NULL;

	/* 1. create resource */
	pResource = malloc(sizeof(struct UTOPIA_RESOURCE));
	*ppResource = pResource;

	if (pResource == NULL) {
		printf("[utopia malloc error] Line: %d malloc fail\n", __LINE__);
		*ppResource = NULL;
		u32Status = UTOPIA_STATUS_FAIL;
	} else {
		sema_init(&pResource->sem, 1);
		pResource->bInUse = 0;
		pResource->u32Pid = UTOPIA_RESOURCE_NO_OCCUPY;
		if (u32PrivateSize > 0)
			pResource->pPrivate = malloc(u32PrivateSize);
		else
			pResource->pPrivate = NULL;

		*ppResource = pResource;
		u32Status = UTOPIA_STATUS_SUCCESS;
	}
	return u32Status;
}

MS_U32 UtopiaResourceRegister(void *pModuleTmp, void *pResourceTmp, MS_U32 u32RPoolID)
{
	struct UTOPIA_MODULE *pModule = (struct UTOPIA_MODULE *)pModuleTmp;
	struct UTOPIA_RESOURCE *pResource = (struct UTOPIA_RESOURCE *)pResourceTmp;

	if (pModule == NULL) {
		printf("[utopia param error] module pointer should not be null\n");
		return UTOPIA_STATUS_FAIL;
	}

	if (pResource == NULL) {
		printf("[utopia param error] resource ppointer should not be null\n");
		return UTOPIA_STATUS_FAIL;
	}

	pModule->psResource = pResource;

	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 UtopiaResourceObtain(void *pModTmp, MS_U32 u32RPoolID, void **ppResource)
{
//          int ret; /* check semop return value */
	struct UTOPIA_MODULE *pModule;
	struct UTOPIA_RESOURCE *pAvailResource;

//          if(in_interrupt())
//          {
//              printf("[utopia resource warn] obtain resource in interrupt\n");
//              return UTOPIA_STATUS_ERR_NOT_AVAIL;
//          }
	/* check param. */
	if (pModTmp == NULL) {
		printf("[utopia param error] module pointer should not be null\n");
		return UTOPIA_STATUS_FAIL;
	}
	if (ppResource == NULL) {
		printf("[utopia param error] resource ppointer should not be null\n");
		return UTOPIA_STATUS_FAIL;
	}

	pModule = (struct UTOPIA_MODULE *)pModTmp;
	pAvailResource = pModule->psResource;

	down(&pAvailResource->sem);
//          if (ret == 0)
	{
		while (1) {
			if (!(pAvailResource->bInUse)) {
				pAvailResource->bInUse = true;
				pAvailResource->u32Pid = (unsigned int)current->tgid;
				*ppResource = pAvailResource;
				up(&pAvailResource->sem);
				return UTOPIA_STATUS_SUCCESS;
			}
		}
	}

	printf("[utopia error][function: %s] code flow should not reach here\n", __func__);
	return UTOPIA_STATUS_FAIL;
}

MS_U32 UtopiaResourceRelease(void *pResource)
{
//          int ret; /* check semop return value */
	struct UTOPIA_RESOURCE *pAvailResource = ((struct UTOPIA_RESOURCE *)pResource);

	down(&pAvailResource->sem);
//          if (ret == 0)
	{
		pAvailResource->bInUse = false;
		pAvailResource->u32Pid = UTOPIA_RESOURCE_NO_OCCUPY;
	}
	up(&pAvailResource->sem);
	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 UtopiaModuleAddResourceEnd(void *pModuleTmp, MS_U32 u32RPoolID)
{
	return UTOPIA_STATUS_SUCCESS;
}

MS_U32 UtopiaOpen(MS_U32 u32ModuleID, void **ppInstanceTmp, MS_U32 u32ModuleVersion,
		  const void *const pAttribute)
{
	struct UTOPIA_MODULE *psUtopiaModule = NULL;
	struct UTOPIA_INSTANCE *pStInstLock = NULL;

	if (g_ge_mudule_handle != NULL) {
		int ret;

		psUtopiaModule = g_ge_mudule_handle;

		ret = psUtopiaModule->fpOpen((void **)&pStInstLock, pAttribute);
		if (ret) {
			printf("[utopia open error] fail to create instance\n");
			return ret;
		}

		(pStInstLock)->psModule = psUtopiaModule;
		*ppInstanceTmp = pStInstLock;
		return ret;	/* depend on fpOpen, may not be UTOPIA_STATUS_SUCCESS */
	}
	return UTOPIA_STATUS_FAIL;
}

MS_U32 UtopiaIoctl(void *pInstanceTmp, MS_U32 u32Cmd, void *const pArgs)
{
	struct UTOPIA_INSTANCE *pInstance = (struct UTOPIA_INSTANCE *)pInstanceTmp;
	struct UTOPIA_MODULE *psUtopiaModule = g_ge_mudule_handle;

	if (pInstance == NULL) {
		printf("[utopia param error] instance pointer should not be null\n");
		return UTOPIA_STATUS_FAIL;
	}

	return psUtopiaModule->fpIoctl(pInstance, u32Cmd, pArgs);
}

MS_U32 UtopiaClose(void *pInstantTmp)
{
	struct UTOPIA_INSTANCE *pInstant = (struct UTOPIA_INSTANCE *)pInstantTmp;
	struct UTOPIA_MODULE *psUtopiaModule = g_ge_mudule_handle;

	if (psUtopiaModule != NULL)
		return psUtopiaModule->fpClose(pInstant);

	return 0;
}
