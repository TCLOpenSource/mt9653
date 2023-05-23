/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _STI_MSOS_H_
#define _STI_MSOS_H_

//      #include <sys/ioccom.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/slab.h>

//////////////////////////////////// extern VAR defines

extern ptrdiff_t mstar_pm_base;

#define HAL_MIU1_BASE               0x80000000UL	// 1512MB

#define MODULE_GOP 0

#define UTOPIA_STATUS_SUCCESS               0x00000000
#define UTOPIA_STATUS_FAIL                  0x00000001
#define UTOPIA_STATUS_NOT_SUPPORTED         0x00000002
#define UTOPIA_STATUS_PARAMETER_ERROR       0x00000003

//////////////////////////////////// compiler defines

#ifndef DLL_PACKED
#define DLL_PACKED __attribute__((__packed__))
#endif

#ifndef DLL_PUBLIC
#define DLL_PUBLIC __attribute__((visibility("default")))
#endif

#ifndef SYMBOL_WEAK
#define SYMBOL_WEAK
#endif

//////////////////////////////////// STDLIB defines

#ifdef MSOS_TYPE_LINUX_KERNEL
#undef printf
#define printf printk
#endif

#ifdef MSOS_TYPE_LINUX_KERNEL
#define free kfree
#define malloc(size) kmalloc((size), GFP_KERNEL)
#endif

//////////////////////////////////// type defines
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef int8_t __s8;
typedef uint8_t __u8;
typedef int16_t __s16;
typedef uint16_t __u16;
typedef int32_t __s32;
typedef uint32_t __u32;
typedef int64_t __s64;
typedef uint64_t __u64;

#ifndef MS_U8
typedef __u8 MS_U8;
#endif

#ifndef MS_U8
typedef __s8 MS_S8;
#endif

#ifndef MS_U16
typedef __u16 MS_U16;
#endif

#ifndef MS_S16
typedef __s16 MS_S16;
#endif

#ifndef MS_U32
typedef __u32 MS_U32;
#endif

#ifndef MS_S32
typedef __s32 MS_S32;
#endif

#ifndef MS_U64
typedef __u64 MS_U64;
#endif

#ifndef MS_S64
typedef __s64 MS_S64;
#endif

#ifndef MS_BOOL
typedef unsigned char MS_BOOL;
#endif

//      #ifndef MS_FLOAT
//      typedef float                           MS_FLOAT;
//      #endif

#ifndef MS_PHYADDR
typedef __u64 MS_PHYADDR;	// 8 bytes
#endif

#ifndef MS_PHY
typedef __u64 MS_PHY;		// 8 bytes
#endif

#ifndef MS_VIRT
typedef size_t MS_VIRT;		// 8 bytes
#endif

//////////////////////////////////// utility macro

#define ALIGN_4(_x_)	(((_x_) + 3) & ~3)
#define ALIGN_8(_x_)	(((_x_) + 7) & ~7)
#define ALIGN_16(_x_)	(((_x_) + 15) & ~15)
#define ALIGN_32(_x_)	(((_x_) + 31) & ~31)

#ifndef MIN
#define MIN(_a_, _b_)               ((_a_) < (_b_) ? (_a_) : (_b_))
#endif
#ifndef MAX
#define MAX(_a_, _b_)               ((_a_) > (_b_) ? (_a_) : (_b_))
#endif

#define UTP_GOP_LOGI(tag, fmt, ...)	pr_info("[GOP][" tag "]: " fmt, ##__VA_ARGS__)
#define UTP_GOP_LOGW(tag, fmt, ...)	pr_warn("[GOP][" tag "]: " fmt, ##__VA_ARGS__)
#define UTP_GOP_LOGD(tag, fmt, ...)	//need to implement debug level
#define UTP_GOP_LOGE(tag, fmt, ...)	pr_err("[GOP][" tag "]: " fmt, ##__VA_ARGS__)
#define UTP_GOP_LOGF(tag, fmt, ...)	pr_err("[GOP][" tag "]: " fmt, ##__VA_ARGS__)

//////////////////////////////////// enum defines

typedef enum {
	E_POWER_SUSPEND = 1,
	E_POWER_RESUME = 2,
	E_POWER_MECHANICAL = 3,
	E_POWER_SOFT_OFF = 4,
} EN_POWER_MODE;

//////////////////////////////////// MSOS function prototypes

typedef MS_U32(*FUtopiaOpen) (void **ppInstance, const void *const pAttribute);
typedef MS_U32(*FUtopiaIOctl) (void *pInstance, MS_U32 u32Cmd, void *const pArgs);
typedef MS_U32(*FUtopiaClose) (void *pInstance);


//////////////////////////////////// MSOS function defines

#define _miu_offset_to_phy(MiuSel, Offset, PhysAddr) {PhysAddr = Offset; }
#define _phy_to_miu_offset(MiuSel, Offset, PhysAddr) {MiuSel = 0; Offset = PhysAddr; }
#define MS_MODULE_GOP 0
#define MS_MODULE_FRC 1
MS_BOOL MDrv_MMIO_GetBASE(MS_VIRT *base, MS_PHY *sz, int type);

void MsOS_DelayTask(MS_U32 u32Ms);
MS_U32 MsOS_GetSystemTime(void);
MS_S32 MsOS_GetOSThreadID(void);


/*
 * instance functions
 */
MS_U32 UtopiaInstanceCreate(MS_U32 u32PrivateSize, void **ppInstance);
MS_U32 UtopiaInstanceDelete(void *pInstant);
MS_U32 UtopiaInstanceGetPrivate(void *pInstance, void **ppPrivate);
MS_U32 UtopiaInstanceGetModule(void *pInstance, void **ppModule);
/* We hope, we can support poly, ex: JPD and JPD3D as different Module */
MS_U32 UtopiaInstanceGetModuleID(void *pInstance, MS_U32 *pu32ModuleID);
/* We hope, we can support poly, ex: JPD and JPD3D as different Module */
MS_U32 UtopiaInstanceGetModuleVersion(void *pInstant, MS_U32 *pu32Version);
/* We hope we can support interface version mantain */
MS_U32 UtopiaInstanceGetAppRequiredModuleVersion(void *pInstance, MS_U32 *pu32ModuleVersion);
MS_U32 UtopiaInstanceGetPid(void *pInstance);

/*
 * module functions
 */
MS_U32 UtopiaModuleCreate(MS_U32 u32ModuleID, MS_U32 u32PrivateSize, void **ppModule);
MS_U32 UtopiaModuleGetPrivate(void *pModule, void **ppPrivate);
MS_U32 UtopiaModuleRegister(void *pModule);
MS_U32 UtopiaModuleSetupFunctionPtr(void *pModule, FUtopiaOpen fpOpen,
				    FUtopiaClose fpClose, FUtopiaIOctl fpIoctl);


/*
 * resource functions
 */
MS_U32 UtopiaResourceCreate(char *u8ResourceName, MS_U32 u32PrivateSize, void **ppResource);
MS_U32 UtopiaResourceGetPrivate(void *pResource, void **ppPrivate);
MS_U32 UtopiaResourceRegister(void *pModule, void *pResouce, MS_U32 u32PoolID);
MS_U32 UtopiaResourceObtain(void *pInstant, MS_U32 u32PoolID, void **ppResource);
MS_U32 UtopiaResourceTryObtain(void *pInstant, MS_U32 u32PoolID, void **ppResource);
MS_U32 UtopiaResourceRelease(void *pResource);
MS_U32 UtopiaResourceGetPid(void *pResource);
MS_U32 UtopiaResourceGetNext(void *pModTmp, void **ppResource);

MS_U32 UtopiaModuleAddResourceStart(void *psModule, MS_U32 u32PoolID);
MS_U32 UtopiaModuleAddResourceEnd(void *psModule, MS_U32 u32PoolID);

MS_U32 UtopiaOpen(MS_U32 u32ModuleID, void **pInstant, MS_U32 u32ModuleVersion,
		  const void *const pAttribte);
MS_U32 UtopiaIoctl(void *pInstant, MS_U32 u32Cmd, void *const pArgs);
MS_U32 UtopiaClose(void *pInstant);

#endif				// _STI_MSOS_H_
