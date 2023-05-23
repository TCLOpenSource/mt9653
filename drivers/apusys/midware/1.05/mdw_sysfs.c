// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include "mdw_rsc.h"
#include "mdw_queue.h"
#include "mdw_cmn.h"
#include "mdw_sched.h"
#include "comm_driver.h"

#define STRING_SIZE_RANGE 16

static struct device *mdw_dev;
static uint32_t g_sched_plcy_show;

static ssize_t dsp_task_num_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mdw_queue *mq = NULL;
	int ret = 0;

	mq = mdw_rsc_get_queue(APUSYS_DEVICE_VPU);
	if (!mq)
		return -EINVAL;

	ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)mq->normal_task_num);
	if (ret < 0)
		mdw_drv_warn("show dsp task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(dsp_task_num);

static ssize_t dla_task_num_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mdw_queue *mq = NULL;
	int ret = 0;

	mq = mdw_rsc_get_queue(APUSYS_DEVICE_MDLA);
	if (!mq)
		return -EINVAL;

	ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)mq->normal_task_num);
	if (ret < 0)
		mdw_drv_warn("show dla task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(dla_task_num);

static ssize_t dma_task_num_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mdw_queue *mq = NULL;
	int ret = 0;

	mq = mdw_rsc_get_queue(APUSYS_DEVICE_EDMA);
	if (!mq)
		return -EINVAL;

	ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)mq->normal_task_num);
	if (ret < 0)
		mdw_drv_warn("show dma task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(dma_task_num);

#ifdef COMMAND_UTILIZATION
extern u32 exec_time_log_period;
//extern void exec_time_log_proc(struct exec_time_result *result);
#define STRING_SIZE_UTILIZATION 128
//int utilization_init_timer_flag;
//#define UTILIZATION_FLAG_ON 123
#endif

static ssize_t utilization_task_num_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;
#ifdef COMMAND_UTILIZATION
	struct exec_time_result result;

	result.min = 0xffffffff;
	result.max = 0;
	result.avg = 0;
	result.tot = 0;
	result.cnt = 0;

	if (utilization_init_timer_flag == UTILIZATION_FLAG_ON) {
		exec_time_log_proc(&result);
		mdw_drv_debug("min(%x)\n", result.min);
		mdw_drv_debug("max(%x)\n", result.max);
		mdw_drv_debug("avg(%x)\n", result.avg);
		mdw_drv_debug("tot(%x)\n", result.tot);
		mdw_drv_debug("cnt(%x)\n", result.cnt);
		ret = snprintf(buf, STRING_SIZE_UTILIZATION
, "period(%d jiffies), min(%d ns), max(%d ns), avg(%d ns), tot(%d ns), cnt(%d)\n",
			exec_time_log_period, result.min
			, result.max, result.avg, result.tot, result.cnt);
	}
#endif
	if (ret < 0)
		mdw_drv_warn("show utilization task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(utilization_task_num);

static ssize_t dla_switch_utilization_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;

#ifdef COMMAND_UTILIZATION
	if (utilization_init_timer_flag != UTILIZATION_FLAG_ON) {
		utilization_init_timer_flag = UTILIZATION_FLAG_ON;
		mdw_drv_debug("utilization on\n");
		exec_time_log_init();
	} else {
		mdw_drv_debug("already utilization on\n");
	}
#endif
	return ret;
}
static DEVICE_ATTR_RO(dla_switch_utilization);

#define HW_INDO_DUMP
#ifdef HW_INDO_DUMP
struct apu_iommu_heap {
	char *name;
	void *data;
};
static const struct apu_iommu_heap iommu_heap[] = {
	{ .name = "mtk,mt5896-apusys"},
	{ .name = "mtk,mt5896e3-apusys"},
	{ .name = "mtk,mt5897-apusys"},
	{ .name = "mtk,mt5879-apusys"},
};

u32 gsmsize;
u32 conv_size;
u32 hw_version;
u32 mdw_version;
u32 edma_version;

char hw_version_map[5];
char mdw_version_map[5];
char edma_version_map[5];

int dts_info_get(void)
{
	int ret = 0;
	int i;
	//int project_id;
	struct device_node *np;
	int max_len = sizeof(hw_version_map);

	gsmsize = 0;
	conv_size = 0;
	hw_version = 0;
	mdw_version = 0;
	edma_version = 0;


	for (i = 0; i < ARRAY_SIZE(iommu_heap); i++) {
		np = of_find_compatible_node(NULL, NULL,
				iommu_heap[i].name);
		if (np) {
			mdw_drv_debug("of_node: %s\n", (char *)&iommu_heap[i]);
			break;
		}
	}

	mdw_drv_debug("device project(%s)\n", (char *)&iommu_heap[i]);

	//if (iommu_heap[i].name == "mtk,mt5896-mdla")
	//if (iommu_heap[i].name == "mtk,mt5896-apusys")
		//project_id = 5;

	if (np == NULL)
		return ret;

	ret = of_property_read_u32(np, "gsm_size", &gsmsize);
	ret = of_property_read_u32(np, "con_buf_size", &conv_size);
	ret = of_property_read_u32(np, "hw_version", &hw_version);
	ret = of_property_read_u32(np, "mdw_version", &mdw_version);
	ret = of_property_read_u32(np, "edma_version", &edma_version);

	//mdw_drv_debug("gsm size(%x)\n", gsmsize);
	//mdw_drv_debug("con_buf_size(%x)\n", conv_size);
	//mdw_drv_debug("hw_version(%x)\n", hw_version);
	switch (hw_version) {
	case 1:
		//hw_version_map = "1.00";
		snprintf(hw_version_map, max_len, "1.00");
		break;
	case 2:
		//hw_version_map = "1.70";
		snprintf(hw_version_map, max_len, "1.70");
		break;
	case 3:
		//hw_version_map = "2.00";
		snprintf(hw_version_map, max_len, "2.00");
		break;
	case 4:
		//hw_version_map = "2.10";
		snprintf(hw_version_map, max_len, "2.10");
		break;
	default:
		//hw_version_map = "0.00";
		snprintf(hw_version_map, max_len, "0.00");
		break;
	}
	switch (mdw_version) {
	case 1:
		//mdw_version_map = "1.00";
		snprintf(mdw_version_map, max_len, "1.00");
		break;
	case 2:
		//mdw_version_map = "1.05";
		snprintf(mdw_version_map, max_len, "1.05");
		break;
	case 3:
		//mdw_version_map = "1.10";
		snprintf(mdw_version_map, max_len, "1.10");
		break;
	case 4:
		//mdw_version_map = "2.00";
		snprintf(mdw_version_map, max_len, "2.00");
		break;
	default:
		//mdw_version_map = "0.00";
		snprintf(mdw_version_map, max_len, "0.00");
		break;
	}
	switch (edma_version) {
	case 1:
		//edma_version_map = "1.00";
		snprintf(edma_version_map, max_len, "1.00");
		break;
	case 2:
		//edma_version_map = "2.00";
		snprintf(edma_version_map, max_len, "2.00");
		break;
	case 3:
		//edma_version_map = "3.00";
		snprintf(edma_version_map, max_len, "3.00");
		break;
	case 4:
		//edma_version_map = "4.00";
		snprintf(edma_version_map, max_len, "4.00");
		break;
	default:
		//edma_version_map = "0.00";
		snprintf(edma_version_map, max_len, "0.00");
		break;
	}
	mdw_drv_debug("hw_version_map(%s)\n", hw_version_map);
	mdw_drv_debug("mdw_version_map(%s)\n", mdw_version_map);
	mdw_drv_debug("edma_version_map(%s)\n", edma_version_map);
	return ret;
}

static ssize_t gsm_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;

	mdw_drv_debug("gsm size(%x)\n", gsmsize);
	mdw_drv_debug("con_buf_size(%x)\n", conv_size);
	mdw_drv_debug("hw_version(%x)\n", hw_version);

	//mdw_drv_debug("hw_version_map(%f)\n", hw_version_map);
	//mdw_drv_debug("mdw_version_map(%f)\n", mdw_version_map);
	//mdw_drv_debug("edma_version_map(%f)\n", edma_version_map);

	//ret = snprintf(buf, STRING_SIZE_RANGE, "mt5896 gsm 1Mb\n");
	//ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)project_id);
	ret = snprintf(buf, STRING_SIZE_RANGE, "gsm_size: %u\n", (uint32_t)gsmsize);
	if (ret < 0)
		mdw_drv_warn("show dla task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(gsm_size);

static ssize_t con_buf_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;

	mdw_drv_debug("conb_size: %u\n", conv_size);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "mt5896 gsm 1Mb\n");
	//ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)project_id);
	ret = snprintf(buf, STRING_SIZE_RANGE, "conb_size: %u\n", conv_size);
	if (ret < 0)
		mdw_drv_warn("show dla task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(con_buf_size);


static ssize_t hw_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;

	mdw_drv_debug("hw_version_map: %s\n", hw_version_map);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "mt5896 gsm 1Mb\n");
	//ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)project_id);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "gsm_size: %u
		//con_buf_size: %u hw_version: %f mdw_version: %f
		//edma_version: %f\n", (uint32_t)gsmsize, conv_size,
		//hw_version_map, mdw_version_map, edma_version_map);
	ret = snprintf(buf, STRING_SIZE_RANGE, "hw_ver: %s\n", hw_version_map);
	if (ret < 0)
		mdw_drv_warn("show dla task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(hw_version);

static ssize_t mdw_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;

	mdw_drv_debug("mdw_version_map: %s\n", mdw_version_map);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "mt5896 gsm 1Mb\n");
	//ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)project_id);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "mdw_version: %f\n", mdw_version_map);
	ret = snprintf(buf, STRING_SIZE_RANGE, "mdw_ver: %s\n", mdw_version_map);
	if (ret < 0)
		mdw_drv_warn("show dla task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(mdw_version);

static ssize_t edma_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0;

	mdw_drv_debug("edma_version_map: %s\n", edma_version_map);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "mt5896 gsm 1Mb\n");
	//ret = snprintf(buf, STRING_SIZE_RANGE, "%u\n", (uint32_t)project_id);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "edma_version:: %u\n", edma_version_map);
	//ret = snprintf(buf, STRING_SIZE_RANGE, "edma_version:: %f\n", edma_version_map);
	ret = snprintf(buf, STRING_SIZE_RANGE, "edma_ver: %s\n", edma_version_map);
	if (ret < 0)
		mdw_drv_warn("show dla task num fail(%d)\n", ret);

	return ret;
}
static DEVICE_ATTR_RO(edma_version);
#endif

static struct attribute *mdw_task_attrs[] = {
	&dev_attr_dsp_task_num.attr,
	&dev_attr_dla_task_num.attr,
	&dev_attr_dma_task_num.attr,
	&dev_attr_gsm_size.attr,
	&dev_attr_con_buf_size.attr,
	&dev_attr_hw_version.attr,
	&dev_attr_mdw_version.attr,
	&dev_attr_edma_version.attr,
	&dev_attr_utilization_task_num.attr,
	&dev_attr_dla_switch_utilization.attr,
	NULL,
};

static struct attribute_group mdw_devinfo_attr_group = {
	.name	= "queue",
	.attrs	= mdw_task_attrs,
};

static ssize_t policy_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char *p = buf;

	mdw_drv_debug("g_sched_plcy_show(%u)\n", g_sched_plcy_show);
	if (!g_sched_plcy_show)
		p += sprintf(p, "%u\n", mdw_rsc_get_preempt_plcy());
	else {
		p += sprintf(p, "preemption(%u)\n", mdw_rsc_get_preempt_plcy());
		p += sprintf(p, "  0: rr,simple\n");
		p += sprintf(p, "  1: rr,prefer lower priority\n");
	}

	WARN_ON(p - buf >= PAGE_SIZE);

	return p - buf;
}

static ssize_t policy_store(struct device *dev,
	struct device_attribute *attr, const char *buf,
	size_t count)
{
	uint32_t in = 0;
	int ret = 0;
	char plcy[32];

	if (sscanf(buf, "%31s %u", plcy, &in) != 2)
		return -EPERM;

	mdw_drv_debug("plcy(%s), in(%u)\n", plcy, in);
	if (!strcmp(plcy, "preemption")) {
		ret = mdw_rsc_set_preempt_plcy(in);
		if (ret)
			mdw_drv_err("set preempt plcy(%u) fail(%d)\n",
				in, ret);
	} else if (!strcmp(plcy, "show_info")) {
		g_sched_plcy_show = in;
	}

	return count;
}
static DEVICE_ATTR_RW(policy);

static struct attribute *mdw_sched_attrs[] = {
	&dev_attr_policy.attr,
	NULL,
};

static struct attribute_group mdw_sched_attr_group = {
	.name	= "sched",
	.attrs	= mdw_sched_attrs,
};

int mdw_sysfs_init(struct device *kdev)
{
	int ret = 0;

	g_sched_plcy_show = 0;

	/* create /sys/devices/platform/apusys/device/xxx */
	mdw_dev = kdev;
	ret = sysfs_create_group(&mdw_dev->kobj, &mdw_devinfo_attr_group);
	if (ret)
		mdw_drv_err("create mdw devinfo attr fail, ret %d\n", ret);
	ret = sysfs_create_group(&mdw_dev->kobj, &mdw_sched_attr_group);
	if (ret)
		mdw_drv_err("create mdw sched attr fail, ret %d\n", ret);

	dts_info_get();

	return ret;
}

void mdw_sysfs_exit(void)
{
	if (mdw_dev)
		sysfs_remove_group(&mdw_dev->kobj, &mdw_devinfo_attr_group);

	mdw_dev = NULL;
}
