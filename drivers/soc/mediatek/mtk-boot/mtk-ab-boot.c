// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
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
#include "mtk-ab-boot.h"
#include <linux/tee_drv.h>
#include <linux/tee.h>
#include <linux/sizes.h>

#define BASE64_TABLE_LEN 65

/* AVB TA */
#define TA_AVB_UUID UUID_INIT(0x023f8f1a, 0x292a, 0x432b, \
		0x8f, 0xc4, 0xde, 0x84, 0x71, 0x35, 0x80, 0x67)
#define TA_AVB_CMD_WRITE_ROLLBACK_INDEX	1
#define AVB_MAGIC "avb_version     "

static struct tee_context *mtk_tee_ctx;
static const char base64_table[BASE64_TABLE_LEN] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *platform = "Android"; //default

static int mtk_ramlog_open_session(uint32_t *session_id);

#define TA_CMD_WRITE_VERSION 105
#define RIU_BUS_ADDR            (0x1C000000UL)
#define RETRY_COUNT_BANK        (0x202)
#define REGISTER_OFFSET         (0x9)
#define RESET_DEFAULT           (0xFFFC)
#define RETRY_COUNT_RANGE       (0x3)

static int _optee_match(struct tee_ioctl_version_data *data, const void *vers)
{
	return 1;
}

#define BOOT_ROOT_DIR "ab_boot"
#define CHECK_BOOT_STATUS 1

static int base64_decode(const char *src, int srclen, u8 *dst)
{
	u32 ac = 0;
	int i, bits = 0;
	u8 *bp = dst;
	const char *p;

	for (i = 0; i < srclen; i++) {
		if (src[i] == '=') {
			ac = (ac << 6);
			bits += 6;
			if (bits >= 8)
				bits -= 8;
			continue;
		}

		p = strchr(base64_table, src[i]);
		if (p == NULL || src[i] == 0)
			return -1;

		ac = (ac << 6) | (p - base64_table);
		bits += 6;
		if (bits >= 8) {
			bits -= 8;
			*bp++ = (u8)(ac >> bits);
		}
	}
	if (ac & ((1 << bits) - 1))
		return -1;

	return bp - dst;
}

static int mtk_get_avb_data(struct uboot_avb_data *avb_data, bool *is_found)
{
	int ret = 0;
	const char *data;
	struct device_node *node = NULL;

	node = of_find_node_by_name(NULL, "avb_data");
	if (!node) {
		pr_info("%s(%d), avb_data is not found!\n", __func__, __LINE__);
		*is_found = false;
		return 0;
	}

	ret = of_property_read_string(node, "data", &data);
	if (ret) {
		pr_crit("%s(%d), no found data!\n", __func__, __LINE__);
		goto out_put_node;
	}

	ret = base64_decode(data, strlen(data), (u8 *)avb_data);
	if (ret < 0) {
		pr_crit("%s(%d) Decode Failed\n", __func__, __LINE__);
		ret = -1;
		goto out_put_node;
	}
	ret = 0;

out_put_node:
	*is_found = true;
	of_node_put(node);

	return ret;
}

static int mtk_open_avb_session(uint32_t *sess_id)
{
	struct tee_ioctl_open_session_arg arg;
	uuid_t avb_uuid = TA_AVB_UUID;
	int rc;

	memset(&arg, 0, sizeof(arg));
	memcpy(arg.uuid, avb_uuid.b, TEE_IOCTL_UUID_LEN);

	arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	arg.num_params = 0;

	rc = tee_client_open_session(mtk_tee_ctx, &arg, NULL);
	if ((rc < 0) || (arg.ret != 0)) {
		pr_crit("%s: tee_client_open_session failed, err=%x\n",
			__func__, arg.ret);
		rc = -EINVAL;
		return rc;
	}

	*sess_id = arg.session;

	return rc;
}

static int mtk_invoke_avb_cmd(uint32_t cmd, uint32_t sess_id)
{
	int rc, i;
	bool is_found = false;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param params[TEE_PAYLOAD_NUM];
	struct uboot_avb_data *avb_data;

	memset(&arg, 0, sizeof(arg));
	memset(&params, 0, sizeof(params));

	arg.num_params = TEE_PAYLOAD_NUM;
	arg.session = sess_id;
	arg.func = cmd;

	avb_data = kzalloc(sizeof(struct uboot_avb_data), GFP_KERNEL);
	if (!avb_data)
		return -ENOMEM;

	/* parse avb vbmeta from dtb */
	rc = mtk_get_avb_data(avb_data, &is_found);
	if (rc) {
		pr_crit("%s: get avb data from dtb fail\n", __func__);
		goto exit;
	}

	if (is_found && !strcmp(avb_data->magic, AVB_MAGIC)) {
		for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
			if (avb_data->rollback_index[i] == 0)
				continue;

			pr_info("%s: Update avb vbmeta index = %d\n", __func__, i);
			params[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
			params[0].u.value.a = i;
			params[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
			params[1].u.value.a = (uint32_t)(avb_data->rollback_index[i] >> 32);
			params[1].u.value.b = (uint32_t)(avb_data->rollback_index[i] & 0xFFFFFFFF);

			rc = tee_client_invoke_func(mtk_tee_ctx, &arg, params);
			if (rc < 0 || arg.ret != 0) {
				pr_crit("%s: tee_client_invoke_func failed, err=%x\n",
					__func__, arg.ret);
				rc = -EINVAL;
				goto exit;
			}
		}
	} else {
		pr_info("%s, No need to update avb version\n", __func__);
	}
exit:
	kfree(avb_data);
	return rc;
}

static int mtk_update_avb_vbmeta(void)
{
	int rc;
	uint32_t sess_id;
	struct tee_ioctl_version_data vers = {
		.impl_id = TEE_OPTEE_CAP_TZ,
		.impl_caps = TEE_IMPL_ID_OPTEE,
		.gen_caps = TEE_GEN_CAP_GP,
	};

	mtk_tee_ctx = tee_client_open_context(NULL,
					_optee_match, NULL, &vers);
	if (IS_ERR(mtk_tee_ctx)) {
		pr_crit("%s: context is NULL\n", __func__);
		return -EINVAL;
	}

	rc = mtk_open_avb_session(&sess_id);
	if (rc) {
		pr_crit("%s: open avb ta session failed\n", __func__);
		tee_client_close_context(mtk_tee_ctx);
		return rc;
	}

	rc = mtk_invoke_avb_cmd(TA_AVB_CMD_WRITE_ROLLBACK_INDEX, sess_id);
	if (rc)
		pr_crit("%s: invoke avb ta failed\n", __func__);

	tee_client_close_session(mtk_tee_ctx, sess_id);
	tee_client_close_context(mtk_tee_ctx);

	return rc;
}

static int mtk_update_version(void)
{
	int rc;
	uint32_t sess_id;
	struct tee_ioctl_invoke_arg arg;
	struct tee_ioctl_version_data vers = {
		.impl_id = TEE_OPTEE_CAP_TZ,
		.impl_caps = TEE_IMPL_ID_OPTEE,
		.gen_caps = TEE_GEN_CAP_GP,
	};

	memset(&arg, 0, sizeof(arg));

	mtk_tee_ctx = tee_client_open_context(NULL,
					_optee_match, NULL, &vers);
	if (IS_ERR(mtk_tee_ctx)) {
		pr_crit("%s: context is NULL\n", __func__);
		return -EINVAL;
	}

	rc = mtk_ramlog_open_session(&sess_id);
	if (rc) {
		pr_crit("%s: open ramlog ta session failed\n", __func__);
		tee_client_close_context(mtk_tee_ctx);
		return rc;
	}

	arg.num_params = 0;
	arg.session = sess_id;
	arg.func = TA_CMD_WRITE_VERSION;

	rc = tee_client_invoke_func(mtk_tee_ctx, &arg, NULL);
	if (rc < 0 || arg.ret != 0) {
		pr_crit("%s: tee_client_invoke_func failed, err=%x\n",
			__func__, arg.ret);
		rc = -EINVAL;
	}

	tee_client_close_session(mtk_tee_ctx, sess_id);
	tee_client_close_context(mtk_tee_ctx);

	return rc;
}

static int mtk_reset_retry_count(void)
{
	unsigned short value;
	void __iomem *retry_count_addr;

	retry_count_addr = ioremap(RIU_BUS_ADDR + (RETRY_COUNT_BANK<<REGISTER_OFFSET), PAGE_SIZE);
	value = readw((void __iomem *)(retry_count_addr));
	pr_info("Retry count address[%lx] value:%x", (unsigned long)retry_count_addr, value);
	writew((value&RESET_DEFAULT), (void __iomem *)retry_count_addr);
	value = readw((void __iomem *)(retry_count_addr));
	if ((value & RETRY_COUNT_RANGE) != 0)
		pr_info("%s: Erorr, Got value:%x\n", __func__, value);
	iounmap(retry_count_addr);

	return 0;
}

static long tee_boot_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	uint32_t boot_flag = 0;

	if (arg == 0)
		return -EINVAL;

	if (_IOC_SIZE(cmd) > sizeof(boot_flag))
		return -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (copy_from_user(&boot_flag, (void __user *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;
	}

	switch (cmd) {
	case TEE_BOOT_IOC:
		if (boot_flag != CHECK_BOOT_STATUS) {
			pr_crit("Not Boot Success\n");
			ret = -EINVAL;
			break;
		}
		/* write avb vbmeta */
		ret = mtk_update_avb_vbmeta();
		if (ret) {
			pr_crit("%s: wrtie avb vbmeta failed\n", __func__);
			break;
		}
		/* update blk0 data and TA version */
		ret = mtk_update_version();
		if (ret)
			pr_crit("%s: update version fail\n", __func__);
		break;
	case RETRY_COUNT_IOC:
		if (boot_flag != CHECK_BOOT_STATUS) {
			pr_crit("Not Boot Success\n");
			ret = -EINVAL;
			break;
		}
		mtk_reset_retry_count();
		break;
	default:
		ret = -ENOTTY;
	}

	if (!ret)
		pr_info("%s: MTK boot successful\n", __func__);

	return ret;
}

static const struct file_operations tz_fops_boot = {
	.owner = THIS_MODULE,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tee_boot_ioctl,
#endif
	.unlocked_ioctl = tee_boot_ioctl,
};

#define USER_ROOT_DIR "mtk_boot"

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

static int __init mtk_ab_boot_init(void)
{
	struct proc_dir_entry *root;
	struct proc_dir_entry *dir;

	root = proc_mkdir(USER_ROOT_DIR, NULL);
	if (!root) {
		pr_err("%s(%d): create /proc/%s failed!\n",
			__func__, __LINE__, USER_ROOT_DIR);
		return -ENOMEM;
	}

	dir = proc_create(BOOT_ROOT_DIR, 0660, root, &tz_fops_boot);
	if (!dir) {
		pr_err("%s(%d): create /proc/%s/%s failed!\n", __func__,  __LINE__,
							USER_ROOT_DIR, BOOT_ROOT_DIR);
		remove_proc_entry(USER_ROOT_DIR, NULL);
		return -ENOMEM;
	}

	if (strcmp(platform, "Android") == 0)
		mtk_reset_retry_count();
	else
		pr_info("Linux ref+ skip mtk_reset_retry_count\n");

	pr_info("mkt ab boot init done\n");

	return 0;
}
late_initcall(mtk_ab_boot_init);

static void __exit mtk_ab_boot_exit(void)
{
	remove_proc_subtree(USER_ROOT_DIR, NULL);
}
module_exit(mtk_ab_boot_exit);

module_param(platform, charp, 0);

MODULE_AUTHOR("MediaTek");
MODULE_DESCRIPTION("MediaTek Boot-Misc Driver");
MODULE_LICENSE("GPL v2");
