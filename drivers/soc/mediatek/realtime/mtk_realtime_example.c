// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>
#include "mi_realtime.h"

#define PRINT_RT_LOG(fmt, args...) \
	do { \
		pr_alert(ASCII_COLOR_BLUE"%s[%d]: ", __func__, __LINE__); \
		pr_alert(fmt, ##args); \
		pr_alert(ASCII_COLOR_END"\n"); \
	} while (0)

#define GTEST_FAIL() { \
	PRINT_RT_LOG("RT Framework Test Fail\n"); \
	return; \
}

#define GTEST_SUCCEED() PRINT_RT_LOG("RT Framework Test Succeed\n")


struct task_struct *thread_1;
struct task_struct *thread_2;
struct task_struct *thread_3;
struct task_struct *thread_4;
struct task_struct *thread_5;
struct task_struct *thread_6;
struct task_struct *thread_7;
struct task_struct *thread_1000;
struct task_struct *thread_1001;

void sigroutine(int unused)
{
	// empty
}

int _MI_REALTIME_TestTask(void *args)
{
	while (1)
		usleep_range(500, 600);

	return 0;
}

int _MI_REALTIME_TestTask_DL_Overrun(void *args)
{
	struct timespec start;
	struct timespec end;
	int iter = 15000; //15000000;
	int sum = 0;
	MI_U64 diff;
	int run = 0;

	allow_signal(SIGXCPU);
	while (1) {
		getnstimeofday(&start);
		while (run < iter) {
			if (sum & 1)
				run += 1;
			else
				run += 2;
			sum += run;
		}
		if (sum & 1)
			run = 1;
		else
			run = 0;
		getnstimeofday(&end);
		diff = (MI_U64)1000000L * (end.tv_sec - start.tv_sec) +
				(end.tv_nsec - start.tv_nsec) / 1000;
		//PRINT_RT_LOG("S2=%ld.%06ld\n", start.tv_sec, start.tv_nsec / 1000);
		//PRINT_RT_LOG("E1=%ld.%06ld\n", end.tv_sec, end.tv_usec);
		//PRINT_RT_LOG("D2=%ld us, sum=%d\n", diff, sum);
		// note: android ndk doesn't support pthread_yield, so we use sched_yield() instead.
		// This behavior of sched_yield() allows the task to wake-up exactly at
		// the beginning of the next period
		if (signal_pending(current))
			pr_alert("Catch a SIGXCPU signal, tid=%d\n", current->pid);
		yield();
	}
	return 0;
}


void use_case_01(void)
{
	MI_RESULT ret;
	MI_U32 u32MicroSec;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DmipsToUs(12000, &u32MicroSec);
	if (ret != E_MI_REALTIME_RET_INVAL)
		GTEST_FAIL();
	ret = MI_REALTIME_DmipsToUs(1000, &u32MicroSec);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}

void use_case_02(void)
{
	MI_RESULT ret;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_CreateDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();

	GTEST_SUCCEED();
}

void use_case_03(void)
{
	MI_RESULT ret;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_CreateDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR1", thread_1->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();

	GTEST_SUCCEED();
}

#define test_Multiple_RR()  do { \
	ret = MI_REALTIME_CreateDomain("Domain_Vts1"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR1", thread_1->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR2", thread_2->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_1->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR2", thread_3->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_3->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_2->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
} while (0)


#define test_Multiple_RR_UnlimitedUtil() do { \
	ret = MI_REALTIME_CreateDomain("Domain_Vts2"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_1->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_2->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_3->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_4->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_5->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_6->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_7->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_1->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_2->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_3->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_4->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_5->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_6->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_7->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts2"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_DeleteDomain("Domain_Vts2"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
} while (0)


#define test_Multiple_Deadline()  do { \
	ret = MI_REALTIME_CreateDomain("Domain_Vts3"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_30FPS", thread_1->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_50FPS", thread_2->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_60FPS", thread_3->pid, NULL); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_2->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_1->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_3->pid); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
	ret = MI_REALTIME_DeleteDomain("Domain_Vts3"); \
	if (ret != MI_OK)  \
		GTEST_FAIL(); \
} while (0)

void use_case_04(void)
{
	MI_RESULT ret;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test multiple tasks in RR
	test_Multiple_RR();
	// test unlimited util
	test_Multiple_RR_UnlimitedUtil();
	// test multiple tasks in Deadline
	test_Multiple_Deadline();

	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_05_1(void)
{
	MI_RESULT ret;

	// test: not invoke Init() before CreateDomain()
	ret = MI_REALTIME_CreateDomain("Domain_Vts4");
	if (ret != E_MI_REALTIME_RET_NULL_PTR)
		GTEST_FAIL();

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: RR and Deadline cannot be in the same Domain
	ret = MI_REALTIME_CreateDomain("Domain_Vts4");
	if (ret != E_MI_REALTIME_RET_INVAL)
		GTEST_FAIL();

	// test: CreateDomain more than one time.
	ret = MI_REALTIME_CreateDomain("Domain_Vts5");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_CreateDomain("Domain_Vts5");
	if (ret != MI_OK)
		GTEST_FAIL();

	// test: Deadline over util
	ret = MI_REALTIME_CreateDomain("Domain_Vts5");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts5", "Class_DL", thread_1->pid, NULL);
	if (ret != E_MI_REALTIME_RET_OVERUTIL)
		GTEST_FAIL();
	// test: Deadline invalid json config
	ret = MI_REALTIME_AddThread("Domain_Vts5", "Class_DL_error", thread_1->pid, NULL);
	if (ret != E_MI_REALTIME_RET_INVAL)
		GTEST_FAIL();
	// test: RR over util
	ret = MI_REALTIME_CreateDomain("Domain_Vts6");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts6", "Class_RR", thread_1->pid, NULL);
	if (ret != E_MI_REALTIME_RET_OVERUTIL)
		GTEST_FAIL();
	// test: RR invalid priority
	ret = MI_REALTIME_AddThread("Domain_Vts6", "Class_RR_invalid_priority", thread_1->pid,
								NULL);
	if (ret != E_MI_REALTIME_RET_INVAL)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts4");
	if (ret != E_MI_REALTIME_RET_NULL_PTR)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts5");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts6");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_05_2(void)
{
	MI_RESULT ret;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_CreateDomain("Domain_Vts2");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_1->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: add an already added pid again
	ret = MI_REALTIME_AddThread("Domain_Vts2", "Class_RR_All", thread_1->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: remove a non-exist pid
	ret = MI_REALTIME_RemoveThread("Domain_Vts2", thread_1->pid);
	if (ret != E_MI_REALTIME_RET_NOT_EXIST)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts2");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}

void use_case_06(void)
{
	MI_RESULT ret;
	MI_S32 s32MemIndex;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();

	ret = MI_REALTIME_EnableMemoryHighPriority(0x4000000, 0x8000000, &s32MemIndex);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DisableMemoryHighPriority(s32MemIndex);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_07(void)
{
	MI_RESULT ret;
	MI_S32 s32MemIndex;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_GetAvailableHighPriorityResource(&s32MemIndex);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_08(void)
{
	MI_RESULT ret;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: not existed Domain
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts7");
	if (ret != E_MI_REALTIME_RET_NULL_PTR)
		GTEST_FAIL();
	// test: dump Domains in RR & DL
	ret = MI_REALTIME_CreateDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR1", thread_1->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR2", thread_2->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_CreateDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_60FPS", thread_3->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_50FPS", thread_4->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	/*ret = MI_REALTIME_GetDomainCpuUsage("Domain_Vts1", 1000, 3);
	if (ret != MI_OK)
		GTEST_FAIL();
	*/
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_2->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_3->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_4->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: dump empty domain
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_09_1(void)
{
	MI_RESULT ret;
	MI_REALTIME_Utilization_t stUtil;
	MI_S32 s32CurBudget;

	memset(&stUtil, 0, sizeof(MI_REALTIME_Utilization_t));
	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: RR. QueryBudget (return OK) -> Add thread -> QueryBudget (return overutil)
	ret = MI_REALTIME_CreateDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32Util = 456;
	ret = MI_REALTIME_QueryBudget("Domain_Vts1", &stUtil, &s32CurBudget);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR1", thread_1->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR2", thread_2->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32Util = 1456;
	ret = MI_REALTIME_QueryBudget("Domain_Vts1", &stUtil, &s32CurBudget);
	if (ret != E_MI_REALTIME_RET_OVERUTIL)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_2->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_09_2(void)
{
	MI_RESULT ret;
	MI_REALTIME_Utilization_t stUtil;
	MI_S32 s32CurBudget;

	memset(&stUtil, 0, sizeof(MI_REALTIME_Utilization_t));
	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: DL. QueryBudget (return OK) -> Add thread -> QueryBudget (return overutil)
	ret = MI_REALTIME_CreateDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32DlRuntimeUs = 1000;
	stUtil.s32DlDeadlineUs = 12000;
	stUtil.s32DlPeriodUs = 16000;
	ret = MI_REALTIME_QueryBudget("Domain_Vts3", &stUtil, &s32CurBudget);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_60FPS", thread_3->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_50FPS", thread_4->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32DlRuntimeUs = 8000;
	stUtil.s32DlDeadlineUs = 12000;
	stUtil.s32DlPeriodUs = 16000;
	ret = MI_REALTIME_QueryBudget("Domain_Vts3", &stUtil, &s32CurBudget);
	if (ret != E_MI_REALTIME_RET_OVERUTIL)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_4->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_3->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_10(void)
{
	MI_RESULT ret;
	MI_REALTIME_Utilization_t stUtil;
	MI_S32 s32CurBudget;

	memset(&stUtil, 0, sizeof(MI_REALTIME_Utilization_t));
	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: RR. QueryBudget (return OK) -> Add thread
	ret = MI_REALTIME_CreateDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32Util = 456;
	ret = MI_REALTIME_QueryBudget("Domain_Vts1", &stUtil, &s32CurBudget);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread_Lite("Domain_Vts1", "Class_RR1", &stUtil, thread_1->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: DL. QueryBudget (return OK) -> Add thread
	ret = MI_REALTIME_CreateDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32DlRuntimeUs = 1000;
	stUtil.s32DlDeadlineUs = 12000;
	stUtil.s32DlPeriodUs = 16000;
	ret = MI_REALTIME_QueryBudget("Domain_Vts3", &stUtil, &s32CurBudget);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread_Lite("Domain_Vts3", "Class_DL_60FPS", &stUtil, thread_3->pid,
									NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_3->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_11_1(void)
{
	MI_RESULT ret;
	MI_REALTIME_Utilization_t stUtil;

	memset(&stUtil, 0, sizeof(MI_REALTIME_Utilization_t));
	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: RR.
	ret = MI_REALTIME_CreateDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR1", thread_1->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR2", thread_2->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32Util = 456;
	ret = MI_REALTIME_UpdateThreadUtil("Domain_Vts1", &stUtil, thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_2->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


void use_case_11_2(void)
{
	MI_RESULT ret;
	MI_REALTIME_Utilization_t stUtil;

	memset(&stUtil, 0, sizeof(MI_REALTIME_Utilization_t));
	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: DL.
	ret = MI_REALTIME_CreateDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_60FPS", thread_3->pid, sigroutine);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_50FPS", thread_4->pid, NULL);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32DlRuntimeUs = 700;
	stUtil.s32DlDeadlineUs = 10000;
	stUtil.s32DlPeriodUs = 10000;
	ret = MI_REALTIME_UpdateThreadUtil("Domain_Vts3", &stUtil, thread_3->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test over util
	stUtil.s32DlRuntimeUs = 9000;
	stUtil.s32DlDeadlineUs = 12000;
	stUtil.s32DlPeriodUs = 16000;
	ret = MI_REALTIME_UpdateThreadUtil("Domain_Vts3", &stUtil, thread_4->pid);
	if (ret != E_MI_REALTIME_RET_OVERUTIL)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32DlRuntimeUs = 200;
	stUtil.s32DlDeadlineUs = 10000;
	stUtil.s32DlPeriodUs = 10000;
	ret = MI_REALTIME_UpdateThreadUtil("Domain_Vts3", &stUtil, thread_3->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test over util
	stUtil.s32DlRuntimeUs = 1000;
	stUtil.s32DlDeadlineUs = 12000;
	stUtil.s32DlPeriodUs = 16000;
	ret = MI_REALTIME_UpdateThreadUtil("Domain_Vts3", &stUtil, thread_4->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	stUtil.s32DlRuntimeUs = 5000;
	stUtil.s32DlDeadlineUs = 10000;
	stUtil.s32DlPeriodUs = 10000;
	// test over util
	ret = MI_REALTIME_UpdateThreadUtil("Domain_Vts3", &stUtil, thread_3->pid);
	if (ret != E_MI_REALTIME_RET_OVERUTIL)
		GTEST_FAIL();
	stUtil.s32DlRuntimeUs = 800;
	stUtil.s32DlDeadlineUs = 12000;
	stUtil.s32DlPeriodUs = 16000;
	ret = MI_REALTIME_UpdateThreadUtil("Domain_Vts3", &stUtil, thread_4->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DumpDomainInfo("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();

	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_4->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_3->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}

void use_case_12(void)
{
	MI_RESULT ret;

	ret = MI_REALTIME_Init(E_MI_REALTIME_VERIFY);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: RR.
	ret = MI_REALTIME_CreateDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	// the sigroutine has no effect
	ret = MI_REALTIME_AddThread("Domain_Vts1", "Class_RR1", thread_1->pid, sigroutine);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts1", thread_1->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	// test: DL.
	ret = MI_REALTIME_CreateDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	// the threads will trigger DL overrun nofitication. i.e. SIGXCPU
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_30FPS", thread_1000->pid, sigroutine);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_AddThread("Domain_Vts3", "Class_DL_30FPS", thread_1001->pid, sigroutine);
	if (ret != MI_OK)
		GTEST_FAIL();
	// sleep 1 second to check whether the threads trigger DL overrun signal.
	msleep(1000);
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_1000->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_RemoveThread("Domain_Vts3", thread_1001->pid);
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts1");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_DeleteDomain("Domain_Vts3");
	if (ret != MI_OK)
		GTEST_FAIL();
	ret = MI_REALTIME_Deinit();
	if (ret != MI_OK)
		GTEST_FAIL();
	GTEST_SUCCEED();
}


int test_func(void *p)
{
	use_case_01();
	use_case_02();
	use_case_03();
	use_case_04();
	use_case_05_1();
	use_case_05_2();
	use_case_06();
	use_case_07();
	use_case_08();
	use_case_09_1();
	use_case_09_2();
	use_case_10();
	use_case_11_1();
	use_case_11_2();
	use_case_12();

	return 0;
}

int test_RT_init(void)
{
	struct task_struct *test_thread;

	thread_1 = kthread_run(_MI_REALTIME_TestTask, NULL, "RT_kthread1");
	thread_2 = kthread_run(_MI_REALTIME_TestTask, NULL, "RT_kthread2");
	thread_3 = kthread_run(_MI_REALTIME_TestTask, NULL, "RT_kthread3");
	thread_4 = kthread_run(_MI_REALTIME_TestTask, NULL, "RT_kthread4");
	thread_5 = kthread_run(_MI_REALTIME_TestTask, NULL, "RT_kthread5");
	thread_6 = kthread_run(_MI_REALTIME_TestTask, NULL, "RT_kthread6");
	thread_7 = kthread_run(_MI_REALTIME_TestTask, NULL, "RT_kthread7");
	thread_1000 = kthread_run(_MI_REALTIME_TestTask_DL_Overrun, NULL, "RT_kthread1000");
	thread_1001 = kthread_run(_MI_REALTIME_TestTask_DL_Overrun, NULL, "RT_kthread1001");

	msleep(1000);

	test_thread = kthread_create(test_func, NULL, "test_realtime_framework");
	wake_up_process(test_thread);

	return 0;
}

module_init(test_RT_init);
MODULE_LICENSE("GPL");
