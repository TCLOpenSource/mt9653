/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2018 MediaTek Inc.
 */
#ifndef _MI_REALTIME_H_
#define _MI_REALTIME_H_

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include "mi_common.h"

//-------------------------------------------------------------------------------------------------
//  Defines
//-------------------------------------------------------------------------------------------------

// MI_REALTIME_DBG_MAGIC is used for the Impl. of MI_REALTIME to use printf for debug
#define MI_REALTIME_DBG_MAGIC	(MI_DBG_ALL+1)

#ifdef __GNUC__
#define likely(x)	   __builtin_expect(!!(x), 1)
#define unlikely(x)	 __builtin_expect(!!(x), 0)
#else
#define likely(x)	   (x)
#define unlikely(x)	 (x)
#endif

#define MI_REALTIME_NAME_LEN		   (64)
#define MI_REALTIME_WRITE_MAX		  (64)
#define MI_REALTIME_PATH_MAX		   (MI_REALTIME_WRITE_MAX + MI_REALTIME_NAME_LEN)

#define MAX_DOMAINS_IN_PROCESS		 (8)
#define MAX_DOMAINS_ALL_PROCESS		(16)
#define PROCESS_ID_LIST_NUM			(8)

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------

typedef enum {
	E_MI_REALTIME_NORMAL = 0,
	E_MI_REALTIME_VERIFY,
} MI_REALTIME_MODE_e;

typedef enum E_MI_Realtime_Cmd_Func {
	E_MI_UNKNOWN = 0,
	E_MI_REALTIME_Init,
	E_MI_REALTIME_Deinit,
	E_MI_REALTIME_CreateDomain,
	E_MI_REALTIME_DeleteDomain,
	E_MI_REALTIME_AddThread,
	E_MI_REALTIME_RemoveThread,
	E_MI_REALTIME_QueryBudget,
	E_MI_REALTIME_AddThread_Lite,
	E_MI_REALTIME_UpdateThreadUtil,
	E_MI_REALTIME_DumpDomainInfo,
	E_MI_REALTIME_GetAllDomainName,
	E_MI_REALTIME_GetDomainCpuUsage,
	E_MI_REALTIME_EnableMemoryHighPriority,
	E_MI_REALTIME_DisableMemoryHighPriority,
	E_MI_REALTIME_GetAvailableHighPriorityResource,
	E_MI_REALTIME_SetDebugLevel,
	E_MI_REALTIME_DmipsToUs,
	__NR_Cmd_Func
} E_MI_Realtime_Cmd_Func_e;

typedef enum {
	E_MI_REALTIME_RET_OK				=  MI_OK,
	E_MI_REALTIME_RET_HAS_EXISTED		=  MI_OK,
	E_MI_REALTIME_RET_ALREADY_INITED	=  MI_OK,
	E_MI_REALTIME_RET_INVAL				=  MI_ERR_INVALID_PARAMETER,
	E_MI_REALTIME_RET_OVERUTIL			=  MI_ERR_RESOURCES,
	E_MI_REALTIME_RET_IO_FAIL			=  MI_ERR_DATA_ERROR,
	E_MI_REALTIME_RET_INIT_FAIL			=  MI_ERR_NOT_INITED,
	E_MI_REALTIME_RET_NULL_PTR			=  MI_ERR_MEMORY_ALLOCATE,
	E_MI_REALTIME_RET_LIST_FAIL			=  MI_ERR_CHAOS,
	E_MI_REALTIME_RET_SCHED_FAIL		=  MI_ERR_LIMITION,
	E_MI_REALTIME_RET_NOT_EXIST			=  MI_ERR_CHAOS,
	E_MI_REALTIME_RET_BUSY				=  MI_ERR_BUSY,
	E_MI_REALTIME_RET_TIMER_FAIL		=  MI_ERR_LIMITION,
	E_MI_REALTIME_RET_OOM_ADJ_FAIL		=  MI_ERR_LIMITION,
} MI_REALTIME_ERROR_e;


typedef struct MI_REALTIME_Utilization_s {
	// used for RR schedule policy
	MI_S32 s32Util;

	// used for Deadline schedule policy
	MI_S32 s32DlDeadlineUs;
	MI_S32 s32DlRuntimeUs;
	MI_S32 s32DlPeriodUs;
} MI_REALTIME_Utilization_t;

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// @brief init MI_REALTIME for user.
/// @param[in] different mode for using different json for verification purpose.
///			User should use E_MI_REALTIME_NORMAL, and E_MI_REALTIME_VERIFY is for VTS.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_INIT_FAIL: Fail from APIs that is not controlled by MI.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_Init(MI_REALTIME_MODE_e eRtMode);

//------------------------------------------------------------------------------
/// @brief deinit MI_REALTIME for user.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_Deinit(void);


//------------------------------------------------------------------------------
/// @brief create MI_REALTIME_DOMAIN_t for user.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_IO_FAIL: failed to operate cgroups.
/// @return E_MI_REALTIME_RET_INIT_FAIL: Fail from APIs that is not controlled by MI.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_CreateDomain(MI_S8 *pszDomainName);


//------------------------------------------------------------------------------
/// @brief delete MI_REALTIME_DOMAIN_t for user.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_DeleteDomain(MI_S8 *pszDomainName);


//------------------------------------------------------------------------------
/// @brief Add s32Pid to RR w/ s32Util(DMIPS) to the Domain.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @param[in] pszClassName is the class name you want to match.
/// @param[in] s32Pid. the target s32Pid.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: domain object is not initialized.
/// @return E_MI_REALTIME_RET_HAS_EXISTED: the s32Pid is already managed.
/// @return E_MI_REALTIME_RET_SCHED_FAIL: failed to set scheduler.
/// @return E_MI_REALTIME_RET_LIST_FAIL: failed to add to the list.
/// @return E_MI_REALTIME_RET_IO_FAIL: failed to move s32Pid to a cpuset.
/// @return E_MI_REALTIME_RET_OVERUTIL: There are too many threads for MI_REALTIME.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_AddThread(MI_S8 *pszDomainName, MI_S8 *pszClassName, MI_S32 s32Pid,
					void (*sigRoutine)(int));


//------------------------------------------------------------------------------
/// @brief Remove s32Pid from RR (and set to OTHER) to the Domain.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @param[in] s32Pid. the target s32Pid.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: domain object is not initialized.
/// @return E_MI_REALTIME_RET_NOT_EXIST: the s32Pid was not managed before.
/// @return E_MI_REALTIME_RET_SCHED_FAIL: failed to set scheduler.
/// @return E_MI_REALTIME_RET_LIST_FAIL: failed to remove from the list.
/// @return E_MI_REALTIME_RET_IO_FAIL: failed to move s32Pid to a cgroup.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_RemoveThread(MI_S8 *pszDomainName, MI_S32 s32Pid);


//------------------------------------------------------------------------------
/// @brief Query whether the available budget can meet the specified s32Util(DMIPS) or
///		deadline parameter (micro second) request.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @param[in] pstUtil is the utilization setting
/// @param[out] s32CurBudget is the current available budget
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: domain object is not initialized.
/// @return E_MI_REALTIME_RET_NOT_EXIST: the class was not managed before.
/// @return E_MI_REALTIME_RET_INVAL: the deadline parameters is invalid.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_QueryBudget(MI_S8 *pszDomainName, MI_REALTIME_Utilization_t *pstUtil,
						MI_S32 *s32CurBudget);

//------------------------------------------------------------------------------
/// @brief Add s32Pid to the Domain.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @param[in] pszClassName is the class name you want to match.
/// @param[in] pstUtil is the utilization setting
/// @param[in] s32Pid. the target s32Pid.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: domain object is not initialized.
/// @return E_MI_REALTIME_RET_NOT_EXIST: the s32Pid was not managed before.
/// @return E_MI_REALTIME_RET_SCHED_FAIL: failed to set scheduler.
/// @return E_MI_REALTIME_RET_LIST_FAIL: failed to remove from the list.
/// @return E_MI_REALTIME_RET_IO_FAIL: failed to move s32Pid to a cgroup.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_AddThread_Lite(MI_S8 *pszDomainName, MI_S8 *pszClassName,
							MI_REALTIME_Utilization_t *pstUtil,
							MI_S32 s32Pid,
							void (*sigRoutine)(int));


//------------------------------------------------------------------------------
/// @brief Update the utilization of the thread
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @param[in] pstUtil is the utilization setting
/// @param[in] s32Pid. the target s32Pid.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: domain object is not initialized.
/// @return E_MI_REALTIME_RET_NOT_EXIST: the s32Pid was not managed before.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_UpdateThreadUtil(MI_S8 *pszDomainName, MI_REALTIME_Utilization_t *pstUtil,
							MI_S32 s32Pid);



//------------------------------------------------------------------------------
/// @brief Dump the info from pstDomain.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: domain object is not initialized.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_DumpDomainInfo(MI_S8 *pszDomainName);



//------------------------------------------------------------------------------
/// @brief Get all the domain name in the central data
/// @param[out] pszBuf is a buffer to store all the domain name. The output string format is
///			 "Domain_XXXX Domain_YYY Domain_ZZZ"
/// @param[in] u32BufSize is the size of pszBuf
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_GetAllDomainName(MI_S8 *ps8Buf, MI_U32 u32BufSize);


//------------------------------------------------------------------------------
/// @brief Get the cpu time from pstDomain.
/// @param[in] pszDomainName is the name of domain in the json file that you want.
/// @param[in] u32IntervalMs is the delay time (in millisecond) between each cycle
/// @param[in] s32Iteration is to finish after number of iterations. -1 means infinite loop.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: domain object is not initialized.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_GetDomainCpuUsage(MI_S8 *pszDomainName, MI_U32 u32IntervalMs,
							MI_S32 s32Iteration);


//------------------------------------------------------------------------------
/// @brief Enable the high priority memory from phyStartAddr to phyEndAddr.
/// @param[in] phyStartAddr. start address of high priority memory.
/// @param[in] phyEndAddr. end address of high priority memory.
/// @param[out] s32MemIndex. return available memory index for user to use.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: global framework object is not initialized.
/// @return E_MI_REALTIME_RET_BUSY: No available high priority memory.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_EnableMemoryHighPriority(MI_PHY phyStartAddr, MI_PHY phyEndAddr,
									MI_S32 *ps32MemIndex);


//------------------------------------------------------------------------------
/// @brief Disable the high priority memory index.
/// @param[in] s32Index. the index of high priority memory.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: global framework object is not initialized.
/// @return E_MI_REALTIME_RET_INVAL: The memory index is not used.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_DisableMemoryHighPriority(MI_S32 s32Index);


//------------------------------------------------------------------------------
/// @brief Get the number of available high priority memory.
/// @param[out] ps32Avail. pointer to the variable to store the result.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: global framework object is not initialized.
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_GetAvailableHighPriorityResource(MI_S32 *ps32Avail);


//------------------------------------------------------------------------------
/// @brief Set debug level for RT framework.
/// @param[in] eDbgLevel. Available option:MI_DBG_INFO, MI_DBG_WRN, MI_DBG_ERR, MI_DBG_ALL, etc.
/// @return E_MI_REALTIME_RET_OK: Process success. All the ret mapped to MI_OK or MI_ERR*
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_SetDebugLevel(MI_DBG_LEVEL eDbgLevel);


//------------------------------------------------------------------------------
/// @brief convert dmips to microsecond depending on system capability
/// @param[in] u32Dmips: unit is dmips
/// @param[out] u32MicroSec: unit is microsecond.
/// @return E_MI_REALTIME_RET_OK: Success. All the ret mapped to MI_OK or MI_ERR*
/// @return E_MI_REALTIME_RET_NULL_PTR: global framework object is not initialized
///									 or param[out] is null
/// @return E_MI_REALTIME_RET_INVAL: system capability is invalid or param[in] is invalid
//------------------------------------------------------------------------------
MI_RESULT
MI_REALTIME_DmipsToUs(MI_U32 u32Dmips, MI_U32 *u32MicroSec);


#endif//_MI_REALTIME_H_

