// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/pm_runtime.h>
#include <linux/debugfs.h>
#include <linux/devfreq.h>
#include <linux/of_address.h>
#include "comm_util.h"

#define COMM_LOG_SET 0x0
u32 comm_klog = COMM_LOG_SET;

struct comm_dbg {
	struct dentry *dir;
	struct dentry *file;
	int option;
} comm_dbg_ctx;

enum APUSYS_POWER_PARAM {
	PARAM_LOG_MASK,
	PARAM_TEE_LOGLVL,
	PARAM_GC_MECH,
};

u32 comm_dbg_get_log_mask(void)
{
	return comm_klog;
}

void comm_dbg_set_log_mask(u32 mask)
{
	comm_klog = mask;
}

void comm_dbg_set_tee_loglvl(u32 mask)
{
	comm_tee_set_loglvl(mask);
}

static int comm_dbg_show(struct seq_file *s, void *unused)
{
	switch (comm_dbg_ctx.option) {
	case PARAM_LOG_MASK:
		seq_printf(s, "log_level = %d\n", comm_dbg_get_log_mask());
		break;
	default:
		break;
	}
	return 0;
}

static int comm_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, comm_dbg_show, inode->i_private);
}

static int set_parameter(u32 param, int argc, int *args)
{
	int ret = 0;

	comm_dbg_ctx.option = param;

	switch (param) {
	case PARAM_LOG_MASK:
		comm_dbg_set_log_mask(args[0]);
		break;
	case PARAM_TEE_LOGLVL:
		comm_dbg_set_tee_loglvl(args[0]);
		break;
	default:
		pr_info("unsupport the parameter:%d\n", param);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static ssize_t comm_dbg_write(struct file *flip, const char __user *buffer,
			       size_t count, loff_t *f_pos)
{
#define MAX_ARG 5
	char *tmp, *token, *cursor;
	int ret, i, param;
	unsigned int args[MAX_ARG];

	tmp = kzalloc(count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = copy_from_user(tmp, buffer, count);
	if (ret) {
		pr_info("[%s] copy_from_user failed, ret=%d\n", __func__, ret);
		goto out;
	}

	tmp[count] = '\0';
	cursor = tmp;
	/* parse a command */
	token = strsep(&cursor, " ");
	if (!strcmp(token, "log_mask"))
		param = PARAM_LOG_MASK;
	else if (!strcmp(token, "tee_loglvl"))
		param = PARAM_TEE_LOGLVL;
	else {
		ret = -EINVAL;
		pr_info("no param[%s]!\n", token);
		goto out;
	}

	/* parse arguments */
	for (i = 0; i < MAX_ARG && (token = strsep(&cursor, " ")); i++) {
		ret = kstrtoint(token, 0, &args[i]);
		if (ret) {
			pr_info("fail to parse args[%d](%s)", i, token);
			goto out;
		}
	}
	set_parameter(param, i, args);
	ret = count;
out:
	kfree(tmp);
	return ret;
}

static const struct file_operations comm_dbg_fops = {
	.open = comm_dbg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = comm_dbg_write,
};

int comm_dbg_init(void)
{
	/* creating comm directory */
	comm_dbg_ctx.dir = debugfs_create_dir("apucomm", NULL);
	if (IS_ERR_OR_NULL(comm_dbg_ctx.dir)) {
		pr_info("failed to create \"apucomm\" debug dir.\n");
		goto out;
	}

	/* creating power file */
	comm_dbg_ctx.file = debugfs_create_file("comm", (0644),
		comm_dbg_ctx.dir, NULL, &comm_dbg_fops);
	if (IS_ERR_OR_NULL(comm_dbg_ctx.file)) {
		pr_info("failed to create \"apucomm\" debug file.\n");
		goto out;
	}

out:
	return 0;
}

void comm_dbg_exit(void)
{
	debugfs_remove(comm_dbg_ctx.file);
	debugfs_remove(comm_dbg_ctx.dir);
}

