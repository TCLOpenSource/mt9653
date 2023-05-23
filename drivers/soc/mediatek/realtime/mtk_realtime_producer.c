// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include "mi_realtime.h"

struct rt_poll {
	char *read_buf;
	int read_buf_len;
	size_t read_len;
	wait_queue_head_t wq;
	struct mutex poll_mutex;
	MI_RESULT ret_value;
	MI_S32 param_out;
};

#define INIT_RET_VALUE (0x1234567)
#define SIGNLE_CMD_MAX_LEN  (80)


struct rt_poll mtk_realtime_poll;


static void mtk_realtime_get_ret_value(MI_RESULT *ret)
{
	const int times = 100000;
	int i = 0;

	while (mtk_realtime_poll.ret_value == INIT_RET_VALUE && i++ < times)
		usleep_range(5, 10);
	if (i > times)
		pr_alert("%s TIMEOUT=%d, ret=%d\n", __func__, i, mtk_realtime_poll.ret_value);
	//else
		//pr_alert("times=%d, ret=%d\n", i, mtk_realtime_poll.ret_value);
	*ret = mtk_realtime_poll.ret_value;
	mutex_unlock(&mtk_realtime_poll.poll_mutex);
}

static void mtk_realtime_get_param_out(MI_S32 *param_out)
{
	const int times = 100000;
	int i = 0;

	while (mtk_realtime_poll.ret_value == INIT_RET_VALUE && i++ < times)
		usleep_range(5, 10);
	if (i > times)
		pr_alert("%s TIMEOUT=%d, ret=%d\n", __func__, i, mtk_realtime_poll.ret_value);
	//else
		//pr_alert("times=%d, ret=%d\n", i, mtk_realtime_poll.ret_value);
	*param_out = mtk_realtime_poll.param_out;
}

static unsigned int mtk_realtime_send_poll_message(char *cmd)
{
	mutex_lock(&mtk_realtime_poll.poll_mutex);
	mtk_realtime_poll.ret_value = INIT_RET_VALUE;
	strncpy(mtk_realtime_poll.read_buf, cmd, SIGNLE_CMD_MAX_LEN-1);
	mtk_realtime_poll.read_buf[SIGNLE_CMD_MAX_LEN - 1] = '\0';
	//pr_alert("Send -> [%s]\n", mtk_realtime_poll.read_buf);
	mtk_realtime_poll.read_len = SIGNLE_CMD_MAX_LEN;
	wake_up(&mtk_realtime_poll.wq);
	return 0;
}


MI_RESULT
MI_REALTIME_Init(MI_REALTIME_MODE_e eRtMode)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%u",
				E_MI_REALTIME_Init, eRtMode) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_Init);


MI_RESULT
MI_REALTIME_Deinit(void)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u",
				E_MI_REALTIME_Deinit) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_Deinit);


MI_RESULT
MI_REALTIME_CreateDomain(MI_S8 *pszDomainName)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s",
				E_MI_REALTIME_CreateDomain, pszDomainName) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_CreateDomain);


MI_RESULT
MI_REALTIME_DeleteDomain(MI_S8 *pszDomainName)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s",
				E_MI_REALTIME_DeleteDomain, pszDomainName) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_DeleteDomain);


MI_RESULT
MI_REALTIME_AddThread(MI_S8 *pszDomainName, MI_S8 *pszClassName, MI_S32 s32Pid,
					void (*sigRoutine)(int))
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s,%s,%d,%d",
				E_MI_REALTIME_AddThread,
				pszDomainName, pszClassName, s32Pid, sigRoutine != NULL) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_AddThread);


MI_RESULT
MI_REALTIME_RemoveThread(MI_S8 *pszDomainName, MI_S32 s32Pid)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s,%d",
				E_MI_REALTIME_RemoveThread,
				pszDomainName, s32Pid) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_RemoveThread);


MI_RESULT
MI_REALTIME_QueryBudget(MI_S8 *pszDomainName, MI_REALTIME_Utilization_t *pstUtil,
						MI_S32 *s32CurBudget)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;
	MI_S32 param_out;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s,%d,%d,%d,%d",
				E_MI_REALTIME_QueryBudget,
				pszDomainName, pstUtil->s32Util, pstUtil->s32DlDeadlineUs,
				pstUtil->s32DlRuntimeUs, pstUtil->s32DlPeriodUs) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_param_out(&param_out);
	mtk_realtime_get_ret_value(&Ret);
	*s32CurBudget = param_out;
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_QueryBudget);


MI_RESULT
MI_REALTIME_AddThread_Lite(MI_S8 *pszDomainName, MI_S8 *pszClassName,
						MI_REALTIME_Utilization_t *pstUtil,
						MI_S32 s32Pid,
						void (*sigRoutine)(int))
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s,%s,%d,%d,%d,%d,%d,%d",
				E_MI_REALTIME_AddThread_Lite,
				pszDomainName, pszClassName,
				pstUtil->s32Util, pstUtil->s32DlDeadlineUs,
				pstUtil->s32DlRuntimeUs, pstUtil->s32DlPeriodUs,
				s32Pid, sigRoutine != NULL) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_AddThread_Lite);


MI_RESULT
MI_REALTIME_UpdateThreadUtil(MI_S8 *pszDomainName, MI_REALTIME_Utilization_t *pstUtil,
							MI_S32 s32Pid)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s,%d,%d,%d,%d,%d",
				E_MI_REALTIME_UpdateThreadUtil,
				pszDomainName, pstUtil->s32Util, pstUtil->s32DlDeadlineUs,
				pstUtil->s32DlRuntimeUs, pstUtil->s32DlPeriodUs, s32Pid) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_UpdateThreadUtil);


MI_RESULT
MI_REALTIME_DumpDomainInfo(MI_S8 *pszDomainName)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s",
				E_MI_REALTIME_DumpDomainInfo, pszDomainName) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_DumpDomainInfo);


MI_RESULT
MI_REALTIME_GetAllDomainName(MI_S8 *ps8Buf, MI_U32 u32BufSize)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%d",
				E_MI_REALTIME_GetAllDomainName, u32BufSize) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_GetAllDomainName);


MI_RESULT
MI_REALTIME_GetDomainCpuUsage(MI_S8 *pszDomainName, MI_U32 u32IntervalMs,
							MI_S32 s32Iteration)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%s,%d,%d",
				E_MI_REALTIME_GetDomainCpuUsage, pszDomainName, u32IntervalMs,
				s32Iteration) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_GetDomainCpuUsage);


MI_RESULT
MI_REALTIME_EnableMemoryHighPriority(MI_PHY phyStartAddr, MI_PHY phyEndAddr,
									MI_S32 *ps32MemIndex)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;
	MI_S32 param_out;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%lld,%lld",
				E_MI_REALTIME_EnableMemoryHighPriority,
				phyStartAddr, phyEndAddr) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_param_out(&param_out);
	mtk_realtime_get_ret_value(&Ret);
	*ps32MemIndex = param_out;
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_EnableMemoryHighPriority);


MI_RESULT
MI_REALTIME_DisableMemoryHighPriority(MI_S32 s32Index)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%d",
				E_MI_REALTIME_DisableMemoryHighPriority, s32Index) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_DisableMemoryHighPriority);


MI_RESULT
MI_REALTIME_GetAvailableHighPriorityResource(MI_S32 *ps32Avail)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;
	MI_S32 param_out;

	if (snprintf(Cmd, sizeof(Cmd), "%u",
				E_MI_REALTIME_GetAvailableHighPriorityResource) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_get_param_out(&param_out);
	mtk_realtime_get_ret_value(&Ret);
	*ps32Avail = param_out;
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_GetAvailableHighPriorityResource);


MI_RESULT
MI_REALTIME_SetDebugLevel(MI_DBG_LEVEL eDbgLevel)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%d",
				E_MI_REALTIME_SetDebugLevel, eDbgLevel) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_ret_value(&Ret);
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_SetDebugLevel);


MI_RESULT
MI_REALTIME_DmipsToUs(MI_U32 u32Dmips, MI_U32 *u32MicroSec)
{
	char Cmd[SIGNLE_CMD_MAX_LEN];
	MI_RESULT Ret;
	MI_S32 param_out;

	if (snprintf(Cmd, sizeof(Cmd), "%u,%d",
				E_MI_REALTIME_DmipsToUs, u32Dmips) < 0) {
		return MI_ERR_DATA_ERROR;
	}
	mtk_realtime_send_poll_message(Cmd);
	mtk_realtime_get_param_out(&param_out);
	mtk_realtime_get_ret_value(&Ret);
	*u32MicroSec = param_out;
	return Ret;
}
EXPORT_SYMBOL(MI_REALTIME_DmipsToUs);


static ssize_t proc_fd_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	#define  BUFFSIZE 80
	char buffer[BUFFSIZE];
	unsigned int input;
	size_t ret = count;
	char *token, *cur;
	char delim[] = " ";

	if (count >= BUFFSIZE)
		count = BUFFSIZE - 1;
	if (copy_from_user(buffer, buf, count))
		return -EFAULT;
	buffer[count] = '\0';
    //pr_alert("Receive -> [%s]\n", buffer);
	cur = buffer;
	token = strsep(&cur, delim);
	if (token != NULL) {
		if (kstrtos32(token, 10, &input)) {
			pr_alert("token1 Invalid input count\n");
			ret = -EINVAL;
		} else {
			mtk_realtime_poll.ret_value = input;
		}
	}
	token = strsep(&cur, delim);
	if (token != NULL) {
		if (kstrtos32(token, 10, &input)) {
			pr_alert("token2 Invalid input count\n");
			ret = -EINVAL;
		} else {
			mtk_realtime_poll.param_out = input;
		}
	}
	return ret;
}


static ssize_t event_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	ssize_t ret;

	if (copy_to_user(buf, mtk_realtime_poll.read_buf, mtk_realtime_poll.read_len)) {
		ret = -EFAULT;
	} else {
		ret = mtk_realtime_poll.read_len;
		memset(mtk_realtime_poll.read_buf, 0, SIGNLE_CMD_MAX_LEN);
		mtk_realtime_poll.read_buf[0] = '\0';
	}
	/* data gets drained once a reader reads from it. */
	mtk_realtime_poll.read_len = 0;
	return ret;
}


unsigned int event_poll(struct file *filp, struct poll_table_struct *wait)
{
	//pr_alert("poll\n");
	__poll_t mask = 0;
	poll_wait(filp, &mtk_realtime_poll.wq, wait);
	if (mtk_realtime_poll.read_len)
		mask |= POLLIN;
	return mask;
}

static const struct file_operations pq_rt_event_fops = {
	.owner = THIS_MODULE,
	.read = event_read,
	.poll = event_poll,
	.write = proc_fd_write,
};

#define PROC_RT_PATH    "MTK_RT_Producer"
#define PROC_RT_NODE    "epoll_notifier"
#ifdef SET_USER_ID
#define REALTIME_GID    SET_USER_ID
#define REALTIME_UID    SET_USER_ID
#endif


int realtime_module_init(void)
{
	struct proc_dir_entry *proc_pq_dir, *entry;

	proc_pq_dir = proc_mkdir(PROC_RT_PATH, NULL);
	if (!proc_pq_dir) {
		pr_alert("proc_mkdir(%s) failed\n", PROC_RT_PATH);
		return -ENOMEM;
	}
	entry = proc_create(PROC_RT_NODE, 0600, proc_pq_dir, &pq_rt_event_fops);
	if (entry == NULL) {
		pr_alert("proc_create failed.\n");
		return -ENOMEM;
	}
#ifdef SET_USER_ID
	pr_alert("%s [%d], REALTIME_UID=%d\n", __func__, __LINE__, REALTIME_UID);
	proc_set_user(entry, KUIDT_INIT(REALTIME_UID), KGIDT_INIT(REALTIME_GID));
#endif
	init_waitqueue_head(&mtk_realtime_poll.wq);
	mtk_realtime_poll.read_len = 0;
	mtk_realtime_poll.read_buf = kmalloc(SIGNLE_CMD_MAX_LEN, GFP_KERNEL);
	if (!mtk_realtime_poll.read_buf) {
		pr_alert("buffer allocation failed\n");
		return -ENOMEM;
	}
	mtk_realtime_poll.read_buf[0] = '\0';
	mtk_realtime_poll.read_buf_len = SIGNLE_CMD_MAX_LEN;
	mutex_init(&mtk_realtime_poll.poll_mutex);
	return 0;
}

static int __init realtime_framework_init(void)
{
	pr_alert("%s is invoked\n", __func__);
	return realtime_module_init();
}


static void __exit realtime_framework_exit(void)
{
	kfree(mtk_realtime_poll.read_buf);
	pr_alert("%s is invoked\n", __func__);
}


module_init(realtime_framework_init);
module_exit(realtime_framework_exit);
MODULE_LICENSE("GPL");
