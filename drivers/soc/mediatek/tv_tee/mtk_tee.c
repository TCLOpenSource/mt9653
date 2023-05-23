// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/arm-smccc.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/cpumask.h>
#include <linux/compat.h>
#include "optee_private.h"
#include "optee_smc.h"
#include "shm_pool.h"
#include "mtk_tee.h"
#include <linux/tee_drv.h>
#include <linux/tee.h>
#include <linux/sizes.h>

#define tee_ramlog_write_rpoint(value)	\
	(*(volatile unsigned int *)(tee_ramlog_buf_adr+4) = (value - tee_ramlog_buf_adr))
#define tee_ramlog_read_wpoint	\
	((*(volatile unsigned int *)(tee_ramlog_buf_adr)) + tee_ramlog_buf_adr)
#define tee_ramlog_read_rpoint	\
	((*(volatile unsigned int *)(tee_ramlog_buf_adr+4)) + tee_ramlog_buf_adr)
#define MAX_PRINT_SIZE      256
#define TEESMC32_ST_FASTCALL_RAMLOG 0xb200585C


static bool is_ramlog_crit_en;
module_param(is_ramlog_crit_en, bool, 0644);

#define ramlog_printk(fmt, args...)	\
	do { \
		if (is_ramlog_crit_en)	\
			pr_crit(fmt, ##args);	\
		else	\
			pr_info(fmt, ##args);	\
	} while (0)

atomic_t tee_sup_used = ATOMIC_INIT(0);
atomic_t STR = ATOMIC_INIT(0);
EXPORT_SYMBOL(STR);
atomic_t SMC_UNLOCKED = ATOMIC_INIT(1);
EXPORT_SYMBOL(SMC_UNLOCKED);

int tee_get_supplicant_count(void)
{
	return (int)atomic_read(&tee_sup_used);
}
EXPORT_SYMBOL(tee_get_supplicant_count);

int optee_version = 3;
EXPORT_SYMBOL(optee_version);
static DEFINE_MUTEX(tee_ramlog_lock);
static struct task_struct *tee_ramlog_tsk;
static unsigned long tee_ramlog_buf_adr;
static unsigned long tee_ramlog_buf_len;
static void *ramlog_vaddr;
static struct tee_context *mtk_tee_ctx;
static struct tee_shm *mtk_ramlog_buf;
#define SUPPORT 0
#define TA_CMD_REGISTER_RAMLOG 104

static int _optee_match(struct tee_ioctl_version_data *data, const void *vers)
{
	return 1;
}

static void optee_smccc_smc(unsigned long a0, unsigned long a1,
			    unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5,
			    unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}

static void optee_smccc_hvc(unsigned long a0, unsigned long a1,
			    unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5,
			    unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_hvc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}

static optee_invoke_fn *get_invoke_func(struct device_node *np)
{
	const char *method;

	pr_info("probing for conduit method from DT.\n");

	if (of_property_read_string(np, "method", &method)) {
		pr_warn("missing \"method\" property\n");
		return ERR_PTR(-ENXIO);
	}

	if (!strcmp("hvc", method))
		return optee_smccc_hvc;
	else if (!strcmp("smc", method))
		return optee_smccc_smc;

	pr_warn("invalid \"method\" property: %s\n", method);
	return ERR_PTR(-EINVAL);
}

static inline bool valid_ramlog_ptr(unsigned long ptr)
{
	if (ptr < tee_ramlog_buf_adr ||
		ptr > tee_ramlog_buf_adr + tee_ramlog_buf_len)
		return false;

	return true;
}

static void tee_ramlog_dump(void)
{
	char log_buf[MAX_PRINT_SIZE];
	char *log_buff_read_point = NULL;
	char *tmp_point = NULL;
	unsigned int log_count = 0;
	unsigned long log_buff_write_point = tee_ramlog_read_wpoint;

	log_buff_read_point = (char *)tee_ramlog_read_rpoint;
	if (!valid_ramlog_ptr(log_buff_write_point) ||
			!valid_ramlog_ptr((unsigned long)log_buff_read_point)) {
		pr_err("tee ramlog: invalid read or write pointer\n");
		return;
	}
	if ((unsigned long)log_buff_read_point == log_buff_write_point)
		return;

	mutex_lock(&tee_ramlog_lock);

	while ((unsigned long)log_buff_read_point != log_buff_write_point) {
		if (isascii(*(log_buff_read_point)) &&
			*(log_buff_read_point) != '\0') {
			if (log_count >= MAX_PRINT_SIZE) {
				log_buf[log_count-2] = '\n';
				log_buf[log_count-1] = '\0';
				ramlog_printk("%s", log_buf);
				log_count = 0;
			} else {
				log_buf[log_count] = *(log_buff_read_point);
				log_count++;
				if (*(log_buff_read_point) == '\n') {
					if (log_count >= MAX_PRINT_SIZE) {
						log_buf[log_count-2] = '\n';
						log_buf[log_count-1] = '\0';
						ramlog_printk("%s", log_buf);
					} else {
						log_buf[log_count] = '\0';
						ramlog_printk("%s", log_buf);
					}
					log_count = 0;
				}
			}
		}

		log_buff_read_point++;
		tmp_point = (char *)(tee_ramlog_buf_adr+tee_ramlog_buf_len);
		if (log_buff_read_point == tmp_point) {
			tmp_point = (char *)(tee_ramlog_buf_adr + 8);
			log_buff_read_point = tmp_point;
		}
	}
	tee_ramlog_write_rpoint((unsigned long)log_buff_read_point);

	mutex_unlock(&tee_ramlog_lock);
}

static int tee_ramlog_loop(void *p)
{
	while (1) {
		tee_ramlog_dump();
		msleep(200);
	}
	return 0;
}

static int tee_ramlog_set_addr_len(uint64_t buf_adr, uint64_t buf_len)
{
	void *mapping_vaddr = NULL;

	if ((buf_adr == 0) || (buf_len == 0))
		return -EINVAL;

	if (tee_ramlog_tsk)
		return 0;

	//Check ramlog address and size were configured or not.
	if ((tee_ramlog_buf_adr || tee_ramlog_buf_len) == 0) {
		//Need to init ramlog_mutex
		mutex_init(&tee_ramlog_lock);
	}

	mutex_lock(&tee_ramlog_lock);
	mapping_vaddr = ioremap_cache(buf_adr, buf_len);
	if (mapping_vaddr == NULL) {
		mutex_unlock(&tee_ramlog_lock);
		pr_info("[OPTEE][RAMLOG] %s %d\n",
			__func__, __LINE__);
		return -ENOMEM;
	}

	tee_ramlog_buf_adr = (unsigned long)mapping_vaddr;
	tee_ramlog_buf_len = buf_len;
	mutex_unlock(&tee_ramlog_lock);

	if (tee_ramlog_tsk == NULL)
		tee_ramlog_tsk = kthread_run(tee_ramlog_loop,
					NULL, "tee_ramlog_loop");

	return 0;
}

static int tee_dyn_ramlog_set_addr_len(struct tee_shm *shm, unsigned long buf_len)
{
	void *ramlog_va = NULL;

	if ((shm == 0) || (buf_len == 0))
		return -EINVAL;

	if (tee_ramlog_tsk)
		return 0;

	//Check ramlog address and size were configured or not.
	if ((tee_ramlog_buf_adr || tee_ramlog_buf_len) == 0) {
		//Need to init ramlog_mutex
		mutex_init(&tee_ramlog_lock);
	}

	mutex_lock(&tee_ramlog_lock);
	ramlog_va = tee_shm_get_va(shm, 0);
	if (IS_ERR(ramlog_va)) {
		pr_err("tee_shm_get_va failed\n");
		return -EINVAL;
	}

	tee_ramlog_buf_adr = (unsigned long)ramlog_va;
	tee_ramlog_buf_len = buf_len;
	mutex_unlock(&tee_ramlog_lock);

	if (tee_ramlog_tsk == NULL)
		tee_ramlog_tsk = kthread_run(tee_ramlog_loop,
					NULL, "tee_dyn_ramlog");
	return 0;
}

static const struct of_device_id optee_match[] = {
	{ .compatible = "linaro,optee-tz" },
	{},
};

static unsigned long optee_disable_fastcall(optee_invoke_fn *invoke_fn)
{
	union {
		struct arm_smccc_res smccc;
		struct optee_smc_get_shm_config_result result;
	} res;

	invoke_fn(TEESMC32_ST_FASTCALL_RAMLOG, 0,
				0, 0, 0, 0, 0, 0, &res.smccc);

	pr_info("%s(%d): a0:%lx : disable OPTEE fastcall!\n",
		__func__, __LINE__, res.smccc.a0);
	return res.smccc.a0;
}

static unsigned long mtk_tee_probe(struct device_node *np)
{
	optee_invoke_fn *invoke_fn;

	invoke_fn = get_invoke_func(np);
	if (IS_ERR(invoke_fn))
		return -EINVAL;

	return optee_disable_fastcall(invoke_fn);
}

static void notify_lock_smc(void *info)
{
	pr_info("%s(%d): Notify TEE CPU to Back for STR CPU %u\n",
		__func__, __LINE__, smp_processor_id());
	atomic_set(&SMC_UNLOCKED, 0);
}

static void notify_unlock_smc(void)
{
	pr_info("%s(%d): Notify TEE CPU Lock SMC\n", __func__, __LINE__);
	atomic_set(&SMC_UNLOCKED, 1);
	atomic_set(&STR, 0);
}

static int optee_driver_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	switch (event) {
	case PM_HIBERNATION_PREPARE:
		pr_info("%s(%d): PM_HIBERNATION_PREPARE\n", __func__, __LINE__);
		break;
	case PM_POST_HIBERNATION:
		pr_info("%s(%d): PM_POST_HIBERNATION\n", __func__, __LINE__);
		break;
	case PM_SUSPEND_PREPARE:
		pr_info("%s(%d): PM_SUSPEND_PREPARE\n", __func__, __LINE__);
		atomic_set(&STR, 1);
		smp_call_function(notify_lock_smc, NULL, 1);
		break;
	case PM_POST_SUSPEND:
		pr_info("%s(%d): PM_POST_SUSPEND\n", __func__, __LINE__);
		notify_unlock_smc();
		break;
	case PM_RESTORE_PREPARE:
		pr_info("%s(%d): PM_RESTORE_PREPARE\n", __func__, __LINE__);
		atomic_set(&STR, 1);
		smp_call_function(notify_lock_smc, NULL, 1);
		break;
	case PM_POST_RESTORE:
		pr_info("%s(%d): PM_POST_RESTORE\n", __func__, __LINE__);
		notify_unlock_smc();
		break;
	default:
		pr_info("%s(%d): unknown event 0x%lx\n",
				__func__, __LINE__, event);
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block optee_pm_notifer = {
	.notifier_call = optee_driver_event,
};


static ssize_t tz_write(struct file *filp, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char local_buf[128];
	unsigned long loglevel;
	struct arm_smccc_res res;
	int ret = 0;

	if (count >= sizeof(local_buf))
		return -EINVAL;

	if (copy_from_user(local_buf, buffer, count))
		return -EFAULT;

	local_buf[count] = 0;

	ret = kstrtoul(local_buf, 10, &loglevel);
	if (ret) {
		pr_err("%s %d kstrtoul error:%d\n",
			__func__, __LINE__, ret);
		return ret;
	}

	pr_info("[OPTEE][LOGLEVEL] set log level to %lu\n", loglevel);

	// Mstar chip supported SMC method only!
	optee_smccc_smc(0xb2005858, loglevel, 0, 0, 0, 0, 0, 0, &res);

	return count;
}

static ssize_t tz_write_ramlog_addr(struct file *filp,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	char local_buf[256];
	char *const delim = " ";
	char *token, *cur;
	int i;
	uint64_t param_value[2];
	uint64_t val;

	if (count >= 256) {
		pr_err("%s %d count(%d) > 256\n", __func__, __LINE__, count);
		return -EINVAL;
	}

	if (copy_from_user(local_buf, buffer, count)) {
		pr_err("%s %d copy_from_user fail\n", __func__, __LINE__);
		return -EFAULT;
	}

	local_buf[count] = 0;
	cur = local_buf;
	for (i = 0 ; i < ARRAY_SIZE(param_value) ; i++) {
		int ret;

		token = strsep(&cur, delim);
		if (!token) {
			pr_err("%s %d token is NULL\n", __func__, __LINE__);
			return -EINVAL;
		}

		ret = kstrtoull(token, 16, &val);
		if (ret) {
			pr_err("%s %d kstrtoull error:%d\n",
				__func__, __LINE__, ret);
			return ret;
		}
		param_value[i] = val;
	}

	pr_info("[RAMLOG] %s addr = %pap , length = %pap\n",
		__func__, &param_value[0], &param_value[1]);

	if (mtk_ramlog_buf) {
		if (tee_dyn_ramlog_set_addr_len(mtk_ramlog_buf, SZ_1M))
			return -EINVAL;
	} else {
		if (tee_ramlog_set_addr_len(param_value[0], param_value[1])) {
			pr_err("%s - can't configure ramlog\n", __func__);
			return -EINVAL;
		}
	}
	return count;
}

static inline unsigned int _is_user_addr(unsigned long addr)
{
	if ((addr & PAGE_OFFSET) == PAGE_OFFSET)
		return 0; // kernel address
	else
		return 1;
}

static long tee_xtest_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	void __user *u_arg;
	uint32_t result = 0;

#ifdef CONFIG_COMPAT
	if (is_compat_task() && _is_user_addr(arg))
		u_arg = compat_ptr(arg);
	else
		u_arg = (void __user *)arg;
#else
	u_arg = (void __user *)arg;
#endif

	switch (cmd) {
	case TEE_XTEST_SUPPLICANT_CNT_IOC:
		result = tee_get_supplicant_count();
		pr_info("%s %d supplicant cnt:%d\n", __func__, __LINE__, result);
		if (result) {
			put_user(TEEC_SUCCESS, (unsigned int __user *) u_arg);
			ret = 0;
		} else {
			put_user(TEEC_ERROR_BAD_PARAMETERS, (unsigned int __user *)u_arg);
			ret = -EINVAL;
		}
		break;
	case TEE_XTEST_CHECK_SMP_IOC:
		result = num_online_cpus();
		pr_info("%s %d The number of online core:%d,and expect NR_CPUS:%d\n",
			__func__, __LINE__, result, num_possible_cpus());
		if (result == num_possible_cpus()) {
			put_user(TEEC_SUCCESS, (unsigned int __user *)u_arg);
			ret = 0;
		} else {
			put_user(TEEC_ERROR_BAD_PARAMETERS, (unsigned int __user *)u_arg);
			ret = -EINVAL;
		}
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}
	return ret;
}

static const struct file_operations tz_fops = {
	.owner   = THIS_MODULE, // system
	.write   = tz_write,
};

static const struct file_operations ramlog_fops = {
	.owner	= THIS_MODULE, // system
	.write	= tz_write_ramlog_addr,
};

static ssize_t tz_version_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos)
{
	static const char version_2_4[] = "2.4";
	static const char version_3_2[] = "3.2";
	int ret = 0;

	if (count < sizeof(version_3_2)) {
		pr_err("%s(%d): invalid count\n", __func__, __LINE__);
		return count;
	}
	if (optee_version == 3)
		ret = copy_to_user(buf, &version_3_2, sizeof(version_3_2));
	else if (optee_version == 2)
		ret = copy_to_user(buf, &version_2_4, sizeof(version_2_4));

	if (ret) {
		pr_err("%s(%d) copy_to_user failed!\n", __func__, __LINE__);
		return ret;
	}

	return count;
}

static const struct file_operations tz_fops_version = {
	.owner	= THIS_MODULE, // system
	.read	= tz_version_read,
};

#define XTEST_ROOT_DIR "mtk_xtest"
static const struct file_operations tz_fops_misc = {
	.owner   = THIS_MODULE,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tee_xtest_ioctl,
#endif
	.unlocked_ioctl = tee_xtest_ioctl,
};

static int __init tee_create_xtest(void)
{
	struct proc_dir_entry *timeout;
	struct proc_dir_entry *xtest_root;

	xtest_root = proc_mkdir(XTEST_ROOT_DIR, NULL);
	if (!xtest_root) {
		pr_err("Create dir /proc/%s error!\n", XTEST_ROOT_DIR);
		return -1;
	}

	timeout = proc_create("optee_xtest", 0644, xtest_root, &tz_fops_misc);
	if (!timeout) {
		pr_err("Create dir /proc/%s/optee_xtest error!\n", XTEST_ROOT_DIR);
		return -ENOMEM;
	}
	return 0;
}

#define USER_ROOT_DIR "tz2_mstar"

static int mtk_ramlog_open_session(uint32_t *session_id)
{
	uuid_t ramlog_uuid = UUID_INIT(0xa9aa0a93, 0xe9f5, 0x4234,
				0x8f, 0xec, 0x21, 0x09,
				0xcb, 0xa2, 0xf6, 0x70);
	int ret = 0;
	struct tee_ioctl_open_session_arg sess_arg;

	memset(&sess_arg, 0, sizeof(sess_arg));
	/* Open session with ramlog Trusted App */
	memcpy(sess_arg.uuid, ramlog_uuid.b, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;

	ret = tee_client_open_session(mtk_tee_ctx, &sess_arg, NULL);
	if ((ret < 0) || (sess_arg.ret != 0)) {
		pr_err("tee_client_open_session failed, err: %x\n",
			sess_arg.ret);
		return -EINVAL;
	}
	*session_id = sess_arg.session;
	return 0;
}

static int mtk_register_ramlog(uint32_t sess, struct tee_shm *shm)
{
	int ret = 0;
	struct tee_ioctl_invoke_arg inv_arg;
	struct tee_param param[4];

	memset(&inv_arg, 0, sizeof(inv_arg));
	memset(&param, 0, sizeof(param));

	/* Invoke TA_CMD_REGISTER_RAMLOG function of Trusted App */
	inv_arg.func = TA_CMD_REGISTER_RAMLOG;
	inv_arg.session = sess;
	inv_arg.num_params = 4;

	/* Fill invoke cmd params */
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;

	/* (value.a << 32) | (value.b) = pointer of shm */
#ifdef __aarch64__
	param[0].u.value.a = (uint32_t) ((uintptr_t)(void *)shm >> 32);
#else
	param[0].u.value.a = 0;
#endif
	param[0].u.value.b = (uint32_t) ((uintptr_t)(void *)shm & 0xFFFFFFFF);

	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].u.value.a = shm->size;

	ret = tee_client_invoke_func(mtk_tee_ctx, &inv_arg, param);
	if ((ret < 0) || (inv_arg.ret != 0)) {
		pr_err("TA_CMD_REGISTER_RAMLOG invoke err: %x\n",
			inv_arg.ret);
		tee_shm_free(shm);
	}
	tee_client_close_session(mtk_tee_ctx, sess);
	tee_client_close_context(mtk_tee_ctx);
	return ret;
}

static int mtk_prepare_dyn_ramlog_buffer(void)
{
	int res = 0;
	uint32_t session_id = 0;
	struct optee *optee = NULL;
	struct tee_ioctl_version_data vers = {
		.impl_id = TEE_OPTEE_CAP_TZ,
		.impl_caps = TEE_IMPL_ID_OPTEE,
		.gen_caps = TEE_GEN_CAP_GP,
	};

	mtk_tee_ctx = tee_client_open_context(NULL,
					_optee_match, NULL, &vers);
	if (IS_ERR(mtk_tee_ctx)) {
		pr_err("\033[1;31m[%s] context is NULL\033[m\n", __func__);
		return -EINVAL;
	}

	optee = tee_get_drvdata(mtk_tee_ctx->teedev);
	if (optee->sec_caps & OPTEE_SMC_SEC_CAP_DYNAMIC_SHM) {
		int ret = 0;

		mtk_ramlog_buf = tee_shm_alloc(mtk_tee_ctx, SZ_1M,
					TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
		if (IS_ERR(mtk_ramlog_buf)) {
			pr_err("\033[0;32;31m %s %d: tee_shm_alloc Fail\033[m\n",
				__func__, __LINE__);
			return -ENOMEM;
		}

		ret = mtk_ramlog_open_session(&session_id);
		if (ret) {
			pr_err("\033[0;32;31m %s %d: open session Fail\033[m\n",
				__func__, __LINE__);
			tee_shm_free(mtk_ramlog_buf);
			return ret;
		}
		return mtk_register_ramlog(session_id, mtk_ramlog_buf);
	}
	pr_crit("[MTK TEE] Static RAMLOG\n");
	return 0;
}

static int __init mtk_tee_init(void)
{
	struct proc_dir_entry *root;
	struct proc_dir_entry *dir;
	struct device_node *fw_np;
	struct device_node *np;
	unsigned long ramlog_supp = 0;

	fw_np = of_find_node_by_name(NULL, "firmware");
	if (!fw_np)
		return -ENODEV;

	np = of_find_matching_node(fw_np, optee_match);
	if (!np)
		return -ENODEV;

	ramlog_supp = mtk_tee_probe(np);
	of_node_put(np);

	if (tee_create_xtest())
		pr_err("%s - tee_create_xtest init fail!\n", __func__);

	root = proc_mkdir(USER_ROOT_DIR, NULL);
	if (!root) {
		pr_err("%s(%d): create /proc/%s failed!\n",
			__func__, __LINE__, USER_ROOT_DIR);
		return -ENOMEM;
	}

	dir = proc_create("ramlog_setup", 0644, root, &ramlog_fops);
	if (!dir) {
		pr_err("%s(%d): create /proc/%s/ramlog_setup failed!\n",
			__func__, __LINE__, USER_ROOT_DIR);
		return -ENOMEM;
	}

	dir = proc_create("log_level", 0644, root, &tz_fops);
	if (!dir) {
		pr_err("%s(%d): create /proc/%s/log_level failed!\n",
			__func__, __LINE__, USER_ROOT_DIR);
		remove_proc_entry(USER_ROOT_DIR, NULL);
		return -ENOMEM;
	}

	dir = proc_create("version", 0644, root, &tz_fops_version);
	if (!dir) {
		pr_err("%s(%d): create /proc/%s/version failed!\n",
			__func__, __LINE__, USER_ROOT_DIR);
		remove_proc_entry(USER_ROOT_DIR, NULL);
		return -ENOMEM;
	}

	if (ramlog_supp == SUPPORT) {
		if (mtk_prepare_dyn_ramlog_buffer())
			pr_crit("[RAMLOG] preparation was failed\n");
	}
	register_pm_notifier(&optee_pm_notifer);
	atomic_set(&tee_sup_used, 1);
	pr_info("mkt tee init done\n");
	return 0;
}
module_init(mtk_tee_init);

static void __exit mtk_tee_exit(void)
{
	if (mtk_ramlog_buf) {
		tee_shm_free(mtk_ramlog_buf);
	} else {
		if (ramlog_vaddr)
			iounmap(ramlog_vaddr);
	}
	remove_proc_subtree(USER_ROOT_DIR, NULL);
	remove_proc_subtree(XTEST_ROOT_DIR, NULL);
	unregister_pm_notifier(&optee_pm_notifer);
}
module_exit(mtk_tee_exit);

MODULE_AUTHOR("MediaTek");
MODULE_DESCRIPTION("MediaTek TEE Driver");
MODULE_LICENSE("GPL v2");
