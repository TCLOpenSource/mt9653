// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author Weiting Tsai <weiting.tsai@mediatek.com>
 */

#include <linux/types.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/major.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/reboot.h>

#include <soc/mediatek/mtk-pm.h>

#define PM_WK_NAME_SIZE				(10)

#define PM_DRV_SHM_BIT_NUMS			(32)

#define PM_REG_PM_SLEEP				(0)
#define PM_OFS_CFG_00_15_DUMMY		(((0x28 << 1) * 2) + 0)
#define PM_OFS_CFG_16_31_DUMMY		(((0x29 << 1) * 2) + 0)
#define PM_OFS_WK_DUMMY				(((0x39 << 1) * 2) + 0)
#define PM_REG_PM_POR				(1)
#define PM_OFS_WK_KEY_DUMMY			(((0x02 << 1) * 2) + 0)
#define PM_OFS_BR_DUMMY				(((0x7E << 1) * 2) + 0)

struct pm_wakeup_source {
	const char *name;
	uint8_t id;
	uint8_t shm_bit;
	struct kobj_attribute kobj_attr;
	struct list_head node;
};

struct pm_device {
	struct device *dev;
	void __iomem *reg_pm_sleep;
	void __iomem *reg_pm_por;
	raw_spinlock_t lock;
	struct notifier_block reboot_notifier;
	struct notifier_block panic_notifier;
	uint32_t max_cnt;
	uint32_t valid_cnt;
	uint32_t total_cnt;
	struct kobject *power_kobj;
	struct list_head wk_src_list;
};

static struct pm_device *_default_pm_dev;

#ifdef CONFIG_OF
static const struct of_device_id pm_match[] = {
	{
		.compatible = "mediatek,pm",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, pm_match);
#endif

#ifdef CONFIG_PM
static int pm_suspend_noirq(struct device *dev)
{
	struct pm_device *pm = dev_get_drvdata(dev);

	if (!pm)
		return -EINVAL;

	/* Check max_cnt. */
	if ((pm->max_cnt) && (pm->valid_cnt + 1 >= pm->max_cnt))
		pm_set_boot_reason(PM_BR_MAX_CNT);
	else
		pm_set_boot_reason(PM_BR_STR_STANDBY);

	pr_crit("System STR off (boot_reason=0x%02X), occur %u/%u times.\n",
			pm_get_boot_reason(),
			++pm->valid_cnt,
			++pm->total_cnt);
	return 0;
}

static int pm_resume_noirq(struct device *dev)
{
	struct pm_device *pm = dev_get_drvdata(dev);
	const char *name = pm_get_wakeup_reason_str();

	if (!pm)
		return -EINVAL;

	/* Check invlid by wakeup source. */
	if ((name) && (!strcmp(name, "rtc0") || !strcmp(name, "wifi-gpio")))
		pm->valid_cnt--;

	pr_crit("System STR on (boot_reason=0x%02X, wakeup_reason=%s), occur %u/%u times.\n",
			pm_get_boot_reason(),
			name,
			pm->valid_cnt,
			pm->total_cnt);
	return 0;
}

static const struct dev_pm_ops pm_str_ops = {
	.suspend_noirq  = pm_suspend_noirq,
	.resume_noirq = pm_resume_noirq,
};
#endif

static int pm_reboot_notify(struct notifier_block *nb, unsigned long code, void *unused)
{
	struct pm_device *pm = container_of(nb, struct pm_device, reboot_notifier);
	char *reason = (char *)unused;

	switch (code) {
	case SYS_RESTART:
		if ((reason) && (!strcmp(reason, "shell"))) {
			pm_set_boot_reason(PM_BR_REBOOT_SHELL);
		} else {
			pm_set_boot_reason(PM_BR_REBOOT);
		}
		pr_crit("System Reboot (boot_reason=0x%02X), occur %u/%u times.\n",
				pm_get_boot_reason(),
				++pm->valid_cnt,
				++pm->total_cnt);
		break;
	case SYS_POWER_OFF:
		pm_set_boot_reason(PM_BR_DC);
		pr_crit("System DC off (boot_reason=0x%02X), occur %u/%u times.\n",
				pm_get_boot_reason(),
				++pm->valid_cnt,
				++pm->total_cnt);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int pm_panic_notify(struct notifier_block *nb, unsigned long event, void *unused)
{
	struct pm_device *pm = container_of(nb, struct pm_device, panic_notifier);

	pm_set_boot_reason(PM_BR_PANIC);
	pr_crit("System Panic (boot_reason=0x%02X), occur %u/%u times.\n",
			pm_get_boot_reason(),
			++pm->valid_cnt,
			++pm->total_cnt);

	return NOTIFY_OK;
}

static struct pm_wakeup_source *pm_find_wakeup_source(struct pm_device *pm,
								const char *name, uint8_t id)
{
	struct pm_wakeup_source *wk_src = NULL;

	list_for_each_entry(wk_src, &pm->wk_src_list, node) {
		if (((name) && (strcmp(wk_src->name, name) == 0)) ||
			((id) && (wk_src->id == id)))
			return wk_src;
	}

	return NULL;
}

/* sysfs for max_cnt. */
static ssize_t pm_max_cnt_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	memset(buf, '\0', strlen("65535") + 2);
	return snprintf(buf, strlen("65535") + 1, "%u\n", pm_get_max_cnt());
}

static ssize_t pm_max_cnt_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	struct pm_device *pm = _default_pm_dev;
	uint32_t tmp = 0;
	int rc = 0;

	rc = kstrtouint(buf, 0, &tmp);
	if (!rc) {
		if (pm_set_max_cnt(tmp) != 0)
			dev_err(pm->dev, "pm_set_max_cnt(%u) fail.\n", tmp);
	}

	return (rc == 0) ? n : rc;
}

static struct kobj_attribute pm_max_cnt_attr =
	__ATTR(max_cnt, 0640, pm_max_cnt_show, pm_max_cnt_store);

static struct attribute *pm_max_cnt_attrs[] = {
	&pm_max_cnt_attr.attr,
	NULL,
};

static struct attribute_group pm_max_cnt_attr_group = {
	.attrs = pm_max_cnt_attrs,
};

/* sysfs for boot reason. */
static ssize_t pm_boot_reason_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	memset(buf, '\0', strlen("0x00") + 2);
	return snprintf(buf, strlen("0x00") + 1, "0x%02X\n", pm_get_boot_reason());
}

static struct kobj_attribute pm_boot_reason_attr =
	__ATTR(boot_reason, 0640, pm_boot_reason_show, NULL);

static struct attribute *pm_boot_reason_attrs[] = {
	&pm_boot_reason_attr.attr,
	NULL,
};

static struct attribute_group pm_boot_reason_attr_group = {
	.attrs = pm_boot_reason_attrs,
};

/* sysfs for wakeup reason. */
static ssize_t pm_wakeup_reason_id_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	memset(buf, '\0', strlen("0x00") + 2);
	return snprintf(buf, strlen("0x00") + 1, "0x%02X\n", pm_get_wakeup_reason());
}

static struct kobj_attribute pm_wakeup_reason_id_attr =
	__ATTR(id, 0640, pm_wakeup_reason_id_show, NULL);

static ssize_t pm_wakeup_reason_name_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	memset(buf, '\0', PM_WK_NAME_SIZE);
	return snprintf(buf, PM_WK_NAME_SIZE - 1, "%s\n", pm_get_wakeup_reason_str());
}

static struct kobj_attribute pm_wakeup_reason_name_attr =
	__ATTR(name, 0640, pm_wakeup_reason_name_show, NULL);

static struct attribute *pm_wakeup_reason_attrs[] = {
	&pm_wakeup_reason_id_attr.attr,
	&pm_wakeup_reason_name_attr.attr,
	NULL,
};

static struct attribute_group pm_wakeup_reason_attr_group = {
	.name = "wakeup_reason",
	.attrs = pm_wakeup_reason_attrs,
};

/* sysfs for wakeup_key. */
static ssize_t pm_wakeup_key_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	memset(buf, '\0', strlen("0x0000") + 2);
	return snprintf(buf, strlen("0x0000") + 1, "0x%04X\n", pm_get_wakeup_key());
}

static struct kobj_attribute pm_wakeup_key_attr =
	__ATTR(wakeup_key, 0640, pm_wakeup_key_show, NULL);

static struct attribute *pm_wakeup_key_attrs[] = {
	&pm_wakeup_key_attr.attr,
	NULL,
};

static struct attribute_group pm_wakeup_key_attr_group = {
	.attrs = pm_wakeup_key_attrs,
};


/* sysfs for wakeup source. */
static ssize_t pm_wakeup_source_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct pm_device *pm = _default_pm_dev;
	bool enable = 0;

	if (pm_get_wakeup_config(attr->attr.name, &enable) != 0)
		dev_err(pm->dev, "pm_get_wakeup_config(%s) fail.\n", attr->attr.name);

	memset(buf, '\0', strlen("0") + 2);
	return snprintf(buf, strlen("0") + 1, "%d\n", enable);
}

static ssize_t pm_wakeup_source_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t n)
{
	struct pm_device *pm = _default_pm_dev;
	bool enable = 0;
	uint8_t tmp = 0;
	int rc = 0;

	rc = kstrtou8(buf, 0, &tmp);
	if (!rc)
		enable = (bool)tmp;
	if (pm_set_wakeup_config(attr->attr.name, enable) != 0)
		dev_err(pm->dev, "pm_set_wakeup_config(%s) fail.\n", attr->attr.name);

	return (rc == 0) ? n : rc;
}

static struct attribute *pm_wakeup_source_attrs[] = {
	NULL,
};

static struct attribute_group pm_wakeup_source_attr_group = {
	.name = "wakeup_source",
	.attrs = pm_wakeup_source_attrs,
};

/* sysfs list. */
static const struct attribute_group *pm_attr_groups[] = {
	&pm_max_cnt_attr_group,
	&pm_boot_reason_attr_group,
	&pm_wakeup_reason_attr_group,
	&pm_wakeup_key_attr_group,
	&pm_wakeup_source_attr_group,
	NULL,
};

static int pm_probe(struct platform_device *pdev)
{
	struct pm_device *pm = NULL;
	struct device_node *parent = NULL, *np = NULL;
	struct pm_wakeup_source *wk_src = NULL;
	int errno = 0;
	struct attribute_group attr_grp = {0};
	struct attribute *attrs[2] = {0};
	uint32_t id = 0;
	uint8_t shm_bit = 0;

	pm = devm_kzalloc(&pdev->dev, sizeof(*pm), GFP_KERNEL);
	if (!pm)
		return -ENOMEM;

	pm->reg_pm_sleep = of_iomap(pdev->dev.of_node, PM_REG_PM_SLEEP);
	if (!pm->reg_pm_sleep) {
		dev_err(&pdev->dev, "of_iomap(%d) fail.\n", PM_REG_PM_SLEEP);
		errno = -ENOMEM;
		goto pm_probe_err_iomap0;
	}
	pm->reg_pm_por = of_iomap(pdev->dev.of_node, PM_REG_PM_POR);
	if (!pm->reg_pm_por) {
		dev_err(&pdev->dev, "of_iomap(%d) fail.\n", PM_REG_PM_POR);
		errno = -ENOMEM;
		goto pm_probe_err_iomap1;
	}

	/* Register reboot notify. */
	pm->reboot_notifier.notifier_call = pm_reboot_notify;
	errno = register_reboot_notifier(&pm->reboot_notifier);
	if (errno) {
		dev_err(&pdev->dev, "register_reboot_notifier() fail errno=%d.\n", errno);
		goto pm_probe_err_notify;
	}

	/* Register panic notify. */
	pm->panic_notifier.notifier_call = pm_panic_notify;
	atomic_notifier_chain_register(&panic_notifier_list, &pm->panic_notifier);

	/* Init data. */
	platform_set_drvdata(pdev, pm);
	dev_set_drvdata(&pdev->dev, pm);
	pm->dev = &pdev->dev;
	raw_spin_lock_init(&pm->lock);

	/* Create sysfs for wakeup reason and wakeup source. */
	pm->power_kobj = kobject_create_and_add("mtk_pm", NULL);
	if (!pm->power_kobj) {
		dev_err(&pdev->dev, "kobject_create_and_add(mtk_pm) fail.\n");
		errno = -ENOMEM;
		goto pm_probe_err_kobj;
	}
	errno = sysfs_create_groups(pm->power_kobj, pm_attr_groups);
	if (errno) {
		dev_err(&pdev->dev, "sysfs_create_groups() fail errno=%d.\n", errno);
		goto pm_probe_err_sysfs;
	}

	INIT_LIST_HEAD(&pm->wk_src_list);
	parent = of_find_node_by_name(pdev->dev.of_node, "wakeup-source");
	for_each_child_of_node(parent, np) {
		if (shm_bit >= PM_DRV_SHM_BIT_NUMS) {
			dev_err(&pdev->dev, "wk_src name=%-10s can't get shm_bit.\n", np->name);
			continue;
		}
		wk_src = kzalloc(sizeof(*wk_src), GFP_ATOMIC);
		if (wk_src) {
			if (of_property_read_u32(np, "id", &id) < 0) {
				dev_err(&pdev->dev, "Read necessary attribute fail.\n");
				kfree(wk_src);
				continue;
			}
			raw_spin_lock(&pm->lock);
			wk_src->name = np->name;
			wk_src->id = (uint8_t)id;
			wk_src->shm_bit = shm_bit++;

			/* Create sysfs = /sys/power/wakeup_source/xxx. */
			wk_src->kobj_attr.attr.name = np->name;
			wk_src->kobj_attr.attr.mode = VERIFY_OCTAL_PERMISSIONS(0640);
			wk_src->kobj_attr.show = pm_wakeup_source_show;
			wk_src->kobj_attr.store = pm_wakeup_source_store;
			/* Add child node. */
			attrs[0] = &wk_src->kobj_attr.attr;
			attrs[1] = NULL;
			sysfs_attr_init(attrs[0]);
			/* Add child to parent node. */
			attr_grp.name = "wakeup_source";
			attr_grp.attrs = attrs;
			raw_spin_unlock(&pm->lock);
			/* Merge group. */
			errno = sysfs_merge_group(pm->power_kobj, &attr_grp);
			if (errno) {
				dev_err(&pdev->dev, "sysfs_merge_group() fail errno=%d.\n", errno);
				kfree(wk_src);
				continue;
			}
			list_add_tail(&wk_src->node, &pm->wk_src_list);
			dev_info(&pdev->dev, "wk_src name=%-10s, id=0x%02X, shm_bit=%02d.\n",
							wk_src->name, wk_src->id, wk_src->shm_bit);
		}
	}
	_default_pm_dev = pm;

	dev_info(&pdev->dev, "pm probe done.\n");
	return 0;

pm_probe_err_sysfs:
	kobject_del(pm->power_kobj);
pm_probe_err_kobj:
	unregister_reboot_notifier(&pm->reboot_notifier);
pm_probe_err_notify:
	iounmap(pm->reg_pm_por);
pm_probe_err_iomap1:
	iounmap(pm->reg_pm_sleep);
pm_probe_err_iomap0:
	devm_kfree(&pdev->dev, pm);

	dev_err(&pdev->dev, "pm probe fail(%d).\n", errno);
	return errno;
}

static int pm_remove(struct platform_device *pdev)
{
	struct pm_device *pm = platform_get_drvdata(pdev);
	struct pm_wakeup_source *wk_src = NULL, *temp = NULL;
	struct attribute_group attr_grp = {0};
	struct attribute *attrs[2] = {0};

	if (!pm)
		return -EINVAL;

	/* Destroy wakeup source list. */
	raw_spin_lock(&pm->lock);
	list_for_each_entry_safe(wk_src, temp, &pm->wk_src_list, node) {
		attrs[0] = &wk_src->kobj_attr.attr;
		attrs[1] = NULL;
		attr_grp.name = "wakeup_source";
		attr_grp.attrs = attrs;
		sysfs_unmerge_group(pm->power_kobj, &attr_grp);
		list_del(&wk_src->node);
		kfree(wk_src);
	}
	raw_spin_unlock(&pm->lock);

	/* Destroy sysfs for wakeup reason and wakeup source.. */
	sysfs_remove_groups(pm->power_kobj, pm_attr_groups);
	kobject_del(pm->power_kobj);

	/* Deinit data. */
	platform_set_drvdata(pdev, NULL);
	dev_set_drvdata(&pdev->dev, NULL);

	/* Un-register reboot notify. */
	unregister_reboot_notifier(&pm->reboot_notifier);

	/* Un-register panic notify. */
	atomic_notifier_chain_unregister(&panic_notifier_list, &pm->panic_notifier);

	iounmap(pm->reg_pm_sleep);
	iounmap(pm->reg_pm_por);

	devm_kfree(&pdev->dev, pm);

	dev_info(&pdev->dev, "pm remove done.\n");

	return 0;
}

static struct platform_driver pm_driver = {
	.driver = {
		.name			= "pm",
		.owner			= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(pm_match),
#endif
#ifdef CONFIG_PM
		.pm				= &pm_str_ops,
#endif
	},
	.probe		= pm_probe,
	.remove		= pm_remove,
};
module_platform_driver(pm_driver);

uint32_t pm_get_max_cnt(void)
{
	struct pm_device *pm = _default_pm_dev;

	if (!pm)
		return -EINVAL;

	return pm->max_cnt;
}
EXPORT_SYMBOL(pm_get_max_cnt);

int32_t pm_set_max_cnt(uint32_t count)
{
	struct pm_device *pm = _default_pm_dev;

	if (!pm)
		return -EINVAL;

	pm->max_cnt = count;

	return 0;
}
EXPORT_SYMBOL(pm_set_max_cnt);

int32_t pm_get_boot_reason(void)
{
	struct pm_device *pm = _default_pm_dev;
	uint8_t data = 0;

	if (!pm)
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	data = readb(pm->reg_pm_por + PM_OFS_BR_DUMMY);
	raw_spin_unlock(&pm->lock);

	return data;
}
EXPORT_SYMBOL(pm_get_boot_reason);

int32_t pm_set_boot_reason(uint8_t reason)
{
	struct pm_device *pm = _default_pm_dev;

	if (!pm)
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	writeb(reason, pm->reg_pm_por + PM_OFS_BR_DUMMY);
	raw_spin_unlock(&pm->lock);

	return 0;
}
EXPORT_SYMBOL(pm_set_boot_reason);

int32_t pm_get_wakeup_reason(void)
{
	struct pm_device *pm = _default_pm_dev;
	uint8_t data = 0;

	if (!pm)
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	data = readb(pm->reg_pm_sleep + PM_OFS_WK_DUMMY);
	raw_spin_unlock(&pm->lock);

	return data;
}
EXPORT_SYMBOL(pm_get_wakeup_reason);

int32_t pm_set_wakeup_reason(const char *name)
{
	struct pm_device *pm = _default_pm_dev;
	struct pm_wakeup_source *wk_src = NULL;

	if (!pm)
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	if (name) {
		wk_src = pm_find_wakeup_source(pm, name, 0x00);
		if (wk_src)
			writeb(wk_src->id, pm->reg_pm_sleep + PM_OFS_WK_DUMMY);
	} else {
		writeb(0x00, pm->reg_pm_sleep + PM_OFS_WK_DUMMY);
	}
	raw_spin_unlock(&pm->lock);

	return 0;
}
EXPORT_SYMBOL(pm_set_wakeup_reason);

char *pm_get_wakeup_reason_str(void)
{
	struct pm_device *pm = _default_pm_dev;
	struct pm_wakeup_source *wk_src = NULL;
	uint8_t data = 0;

	if (!pm)
		return NULL;

	data = (uint8_t)pm_get_wakeup_reason();
	raw_spin_lock(&pm->lock);
	wk_src = pm_find_wakeup_source(pm, NULL, data);
	raw_spin_unlock(&pm->lock);

	return wk_src ? (char *)wk_src->name : NULL;
}
EXPORT_SYMBOL(pm_get_wakeup_reason_str);

int32_t pm_get_wakeup_key(void)
{
	struct pm_device *pm = _default_pm_dev;
	uint16_t data = 0;

	if (!pm)
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	data = readw(pm->reg_pm_por + PM_OFS_WK_KEY_DUMMY);
	raw_spin_unlock(&pm->lock);

	return data;
}
EXPORT_SYMBOL(pm_get_wakeup_key);

int32_t pm_set_wakeup_key(uint16_t key)
{
	struct pm_device *pm = _default_pm_dev;

	if (!pm)
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	writew(key, pm->reg_pm_por + PM_OFS_WK_KEY_DUMMY);
	raw_spin_unlock(&pm->lock);

	return 0;
}
EXPORT_SYMBOL(pm_set_wakeup_key);

int32_t pm_get_wakeup_config(const char *name, bool *enable)
{
	struct pm_device *pm = _default_pm_dev;
	struct pm_wakeup_source *wk_src = NULL;
	void __iomem *base = NULL;

	if ((!pm) || (!name) || (!enable))
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	wk_src = pm_find_wakeup_source(pm, name, 0x00);
	if (wk_src) {
		if ((wk_src->shm_bit / 16) == 0)
			base = pm->reg_pm_sleep + PM_OFS_CFG_00_15_DUMMY;
		else if ((wk_src->shm_bit / 16) == 1)
			base = pm->reg_pm_sleep + PM_OFS_CFG_16_31_DUMMY;
		else
			goto pm_get_wakeup_config_end;

		*enable = !!(readw(base) & BIT(wk_src->shm_bit % 16));
	}
pm_get_wakeup_config_end:
	raw_spin_unlock(&pm->lock);

	return wk_src ? 0 : -ENXIO;
}
EXPORT_SYMBOL(pm_get_wakeup_config);

int32_t pm_set_wakeup_config(const char *name, bool enable)
{
	struct pm_device *pm = _default_pm_dev;
	struct pm_wakeup_source *wk_src = NULL;
	void __iomem *base = NULL;
	uint16_t data = 0, mask = 0;

	if ((!pm) || (!name))
		return -EINVAL;

	raw_spin_lock(&pm->lock);
	wk_src = pm_find_wakeup_source(pm, name, 0x00);
	if (wk_src) {
		if ((wk_src->shm_bit / 16) == 0)
			base = pm->reg_pm_sleep + PM_OFS_CFG_00_15_DUMMY;
		else if ((wk_src->shm_bit / 16) == 1)
			base = pm->reg_pm_sleep + PM_OFS_CFG_16_31_DUMMY;
		else
			goto pm_set_wakeup_config_end;

		mask = BIT(wk_src->shm_bit % 16);
		if (enable)
			data = (readw(base) & ~mask) | (0xFFFF & mask);
		else
			data = (readw(base) & ~mask) | (0x0000 & mask);
		writew(data, base);
	}
pm_set_wakeup_config_end:
	raw_spin_unlock(&pm->lock);

	return wk_src ? 0 : -ENXIO;
}
EXPORT_SYMBOL(pm_set_wakeup_config);

MODULE_DESCRIPTION("boot reason, wakeup reason and wakeup config driver for mediatek");
MODULE_AUTHOR("Weiting Tsai <weiting.tsai@mediatek.com>");
MODULE_LICENSE("GPL v2");
