// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>

#include <media/dvbdev.h>
#include "mdrv_adapter.h"

#define MODEL_NAME			"mediatek,DVB"
#define DRV_NAME			"mediatek,dvb-adapter"
#define DRV_LABEL			"[MDRV_ADAPTER]"

#define ADAPTER_DEBUG_ERR(fmt, ...)		\
	pr_err(DRV_LABEL "[%s][%d]" fmt,	\
	__func__, __LINE__, ##__VA_ARGS__)	\

#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
#define has_op(notifier, op)	\
	((notifier) && (notifier)->op)

#define call_op(notifier, op, args...)	\
	(has_op((notifier), op) ? (notifier)->op(args) : -ENOIOCTLCMD)

#define SOURCE_PAD_IDX	0
#define SINK_PAD_IDX	1

typedef enum {
	DVBDEV_HASH_TYPE_NONE,
	DVBDEV_HASH_TYPE_DEMOD,
	DVBDEV_HASH_TYPE_DEMUX,
	DVBDEV_HASH_TYPE_CI,
	DVBDEV_HASH_TYPE_CA,
	DVBDEV_HASH_TYPE_MAX
} dvbdev_hash_type_t;

typedef struct {
	struct list_head list;
	struct dvb_device *dev;
	struct media_entity *entity;
	struct dvbdev_notifier notifier;
} dvbdev_hash_node_t;
#endif

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static struct kobj_type dvb_adapter_ktype = {
	.sysfs_ops = &kobj_sysfs_ops,
};

static ssize_t kobj_attr_get_adap_sel_nr(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf);

static struct kobj_attribute kobj_attr_adap_sel_nr =
	__ATTR(number, 0444, kobj_attr_get_adap_sel_nr, NULL);

static struct attribute *dvb_adapter_attrs[] = {
	&kobj_attr_adap_sel_nr.attr,
	NULL
};

static struct attribute_group dvb_adapter_attrs_group = {
	.attrs = dvb_adapter_attrs,
};

struct mdrv_dvb_adapter {
	struct platform_device *pdev;
	struct dvb_adapter adapter;
	struct kobject kobj;
#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	struct mutex hash_mutex;
	struct list_head hash_list[DVBDEV_HASH_TYPE_MAX];
	struct media_device mdev;
#endif
};

static struct mdrv_dvb_adapter (*_mdrv_adapter);

#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
static int _mdev_link_notify(
	struct media_link *link,
	unsigned int flags,
	unsigned int notification)
{
	struct media_pad *source_pad = link->source;
	struct media_pad *sink_pad = link->sink;
	struct media_entity *source = source_pad->entity;
	struct media_entity *sink = sink_pad->entity;

	if (notification == MEDIA_DEV_NOTIFY_PRE_LINK_CH) {
		ADAPTER_DEBUG_ERR("======== MEDIA_DEV_NOTIFY_PRE_LINK_CH ========\n");
		ADAPTER_DEBUG_ERR("previous flags = %lx\n", link->flags);
		ADAPTER_DEBUG_ERR("source(%s,%u,%u) --> sink(%s,%u,%u)\n",
			source->name, source->graph_obj.id, source_pad->index,
			sink->name, sink->graph_obj.id, sink_pad->index);
		ADAPTER_DEBUG_ERR("==============================================\n");
	} else if (notification == MEDIA_DEV_NOTIFY_POST_LINK_CH) {
		ADAPTER_DEBUG_ERR("======== MEDIA_DEV_NOTIFY_POST_LINK_CH =======\n");
		ADAPTER_DEBUG_ERR("current flags = %lx\n", link->flags);
		ADAPTER_DEBUG_ERR("source(%s,%u,%u) --> sink(%s,%u,%u)\n",
			source->name, source->graph_obj.id, source_pad->index,
			sink->name, sink->graph_obj.id, sink_pad->index);
		ADAPTER_DEBUG_ERR("==============================================\n");
	}

	return 0;
}

static const struct media_device_ops mdev_ops = {
	.link_notify = _mdev_link_notify,
};

static int _create_links(
	struct media_entity *source,
	const u32 source_function,
	struct media_entity *sink,
	const u32 sink_function,
	u32 flags)
{
	int ret;
	struct media_device *mdev;
	struct media_entity *entity;
	struct media_pad *source_pad, *sink_pad;
	u32 function;

	/* n:n releation */
	if (!source && !sink)
		return -EINVAL;

	/* 1:1 relation */
	if (source && sink) {
		source_pad = &source->pads[SOURCE_PAD_IDX];
		sink_pad = &sink->pads[SINK_PAD_IDX];

		if (!media_entity_find_link(source_pad, sink_pad))
			return media_create_pad_link(
				source, SOURCE_PAD_IDX, sink, SINK_PAD_IDX, flags);
	}

	/* 1:n or n:1 relation */
	if (source) {
		mdev = source->graph_obj.mdev;
		function = sink_function;
	} else {
		mdev = sink->graph_obj.mdev;
		function = source_function;
	}

	media_device_for_each_entity(entity, mdev) {
		if (entity->function != function)
			continue;

		ret = 0;

		if (source) {
			source_pad = &source->pads[SOURCE_PAD_IDX];
			sink_pad = &entity->pads[SINK_PAD_IDX];

			if (!media_entity_find_link(source_pad, sink_pad))
				ret = media_create_pad_link(source, SOURCE_PAD_IDX,
					entity, SINK_PAD_IDX, flags);
		} else {
			source_pad = &entity->pads[SOURCE_PAD_IDX];
			sink_pad = &sink->pads[SINK_PAD_IDX];

			if (!media_entity_find_link(source_pad, sink_pad))
				ret = media_create_pad_link(entity, SOURCE_PAD_IDX,
					sink, SINK_PAD_IDX, flags);
		}

		if (ret)
			return ret;
	}

	return 0;
}

/*
 * dynamically create links:
 *		[Demod] -> [CI] -> [Demux] -> [CA]
 *			|             ^
 *			|_____________|
 */
static void _mdev_entity_notify(struct media_entity *entity, void *notify_data)
{
	switch (entity->function) {
	case MEDIA_ENT_F_DTV_DEMOD:
		_create_links(
			entity, MEDIA_ENT_F_DTV_DEMOD,
			NULL, MEDIA_ENT_F_TS_DEMUX,
			0);
		_create_links(
			entity, MEDIA_ENT_F_DTV_DEMOD,
			NULL, MEDIA_ENT_F_DTV_CA,
			0);
		break;
	case MEDIA_ENT_F_TS_DEMUX:
		_create_links(
			NULL, MEDIA_ENT_F_DTV_DEMOD,
			entity, MEDIA_ENT_F_TS_DEMUX,
			0);
		_create_links(
			NULL, MEDIA_ENT_F_DTV_CA,
			entity, MEDIA_ENT_F_TS_DEMUX,
			0);
		// @TODO: Demux -> DSC
		break;
	case MEDIA_ENT_F_DTV_CA:
	/*
	 * @NOTE: currently as CI device
	 * @TODO: consider CA device (But how to determine CI/CA/SC/AESDMA devices ?)
	 */
		_create_links(
			NULL, MEDIA_ENT_F_DTV_DEMOD,
			entity, MEDIA_ENT_F_DTV_CA,
			0);
		_create_links(
			entity, MEDIA_ENT_F_DTV_CA,
			NULL, MEDIA_ENT_F_TS_DEMUX,
			0);
		break;
	default:
		return;
	}

	ADAPTER_DEBUG_ERR("add new entity(%s)\n", entity->name);
}

static struct media_entity_notify entity_notify = {
	.list = LIST_HEAD_INIT(entity_notify.list),
	.notify = _mdev_entity_notify,
	.notify_data = NULL,
};

static int _init_mdev(struct media_device *mdev, struct dvb_adapter *adapter)
{
	strscpy(mdev->model, MODEL_NAME, sizeof(mdev->model));
	strscpy(mdev->driver_name, DRV_NAME, sizeof(mdev->driver_name));
	mdev->ops = &mdev_ops;
	mdev->dev = adapter->device;

	media_device_init(mdev);

	if (media_device_register(mdev) < 0) {
		ADAPTER_DEBUG_ERR("register media device fail\n");
		return -EFAULT;
	}

	media_device_register_entity_notify(mdev, &entity_notify);

	dvb_register_media_controller(adapter, mdev);

	return 0;
}

static void _release_mdev(struct media_device *mdev)
{
	media_device_unregister(mdev);
}

static void _init_hash_list(struct mdrv_dvb_adapter *adap)
{
	int i;

	mutex_init(&adap->hash_mutex);

	for (i = 0; i < DVBDEV_HASH_TYPE_MAX; i++)
		INIT_LIST_HEAD(&adap->hash_list[i]);
}

static dvbdev_hash_type_t _get_hash_list_type(struct media_entity *entity)
{
	switch (entity->function) {
	case MEDIA_ENT_F_DTV_DEMOD:
		return DVBDEV_HASH_TYPE_DEMOD;
	case MEDIA_ENT_F_TS_DEMUX:
		return DVBDEV_HASH_TYPE_DEMUX;
	case MEDIA_ENT_F_DTV_CA:
		// @TODO: how to determine CI/CA ?
		return DVBDEV_HASH_TYPE_CI;
	default:
		return DVBDEV_HASH_TYPE_NONE;
	}
}

static dvbdev_hash_node_t *_get_hash_list_node(struct media_entity *entity)
{
	dvbdev_hash_type_t type = _get_hash_list_type(entity);
	dvbdev_hash_node_t *node, *next;

	if (type == DVBDEV_HASH_TYPE_NONE)
		return NULL;

	mutex_lock(&_mdrv_adapter->hash_mutex);

	list_for_each_entry_safe(node, next, &_mdrv_adapter->hash_list[type], list)
		if (node->entity == entity) {
			mutex_unlock(&_mdrv_adapter->hash_mutex);
			return node;
		}

	mutex_unlock(&_mdrv_adapter->hash_mutex);
	return NULL;
}

static int _mdev_entity_link_setup(
	struct media_entity *entity,
	const struct media_pad *local,
	const struct media_pad *remote,
	u32 flags)
{
	int ret;
	struct dvbdev_link link;
	dvbdev_hash_node_t *local_node, *remote_node;

	local_node = _get_hash_list_node(local->entity);
	remote_node = _get_hash_list_node(remote->entity);

	if (!local_node || !remote_node) {
		ADAPTER_DEBUG_ERR("can't not find nodes\n");
		return -EINVAL;
	}

	link.remote = remote_node->dev;
	link.state = (flags & MEDIA_LNK_FL_ENABLED) ?
		DVBDEV_LINK_STATE_CONNECT : DVBDEV_LINK_STATE_DISCONNECT;
	ret = call_op(&local_node->notifier, link_notify, local_node->dev, &link);
	if (ret < 0) {
		ADAPTER_DEBUG_ERR("call_op: (%s,%s) link_notify fail\n",
			local_node->entity->name, remote_node->entity->name);
		return -EFAULT;
	}

	return ret;
}

static const struct media_entity_operations entify_ops = {
	.link_setup = _mdev_entity_link_setup,
};
#endif

static ssize_t kobj_attr_get_adap_sel_nr(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct mdrv_dvb_adapter *adap = container_of(kobj, struct mdrv_dvb_adapter, kobj);
	int len;

	len = scnprintf(buf, PAGE_SIZE, "%u\n", adap->adapter.num);

	return len;
}

struct dvb_adapter *mdrv_get_dvb_adapter(void)
{
	if (!_mdrv_adapter) {
		ADAPTER_DEBUG_ERR("_mdrv_adapter == NULL\n");
		return NULL;
	}

	return &_mdrv_adapter->adapter;
}
EXPORT_SYMBOL(mdrv_get_dvb_adapter);

int mdrv_adapter_register_device_notifier(
	struct dvb_device *dev, struct dvbdev_notifier *notifier)
{
#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	dvbdev_hash_type_t type = _get_hash_list_type(dev->entity);
	dvbdev_hash_node_t *node;

	if (type == DVBDEV_HASH_TYPE_NONE)
		return -EINVAL;

	node = devm_kzalloc(
		&_mdrv_adapter->pdev->dev,
		sizeof(dvbdev_hash_node_t),
		GFP_KERNEL);

	if (!node) {
		ADAPTER_DEBUG_ERR("allocate hash node fail\n");
		return -ENOMEM;
	}

	node->dev = dev;
	node->entity = dev->entity;
	memcpy(&node->notifier, notifier, sizeof(node->notifier));

	mutex_lock(&_mdrv_adapter->hash_mutex);
	list_add_tail(&node->list, &_mdrv_adapter->hash_list[type]);
	mutex_unlock(&_mdrv_adapter->hash_mutex);

	dev->entity->ops = &entify_ops;

	return 0;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(mdrv_adapter_register_device_notifier);

int mdrv_adapter_query_device_params(
	struct dvb_device *dev, struct dvbdev_properties *props)
{
#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	int ret;
	dvbdev_hash_node_t *node = _get_hash_list_node(dev->entity);

	if (!node) {
		ADAPTER_DEBUG_ERR("get hash node fail\n");
		return -EFAULT;
	}

	ret = call_op(&node->notifier, query_notify, dev, props);
	if (ret < 0) {
		ADAPTER_DEBUG_ERR("call_op: (%s,%x) query_notify fail\n",
			dev->name, dev->id);
		return -EFAULT;
	}

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(mdrv_adapter_query_device_params);

static int mdrv_adapter_probe(struct platform_device *pdev)
{
	struct mdrv_dvb_adapter *mdrv_adapter;
	int ret;
	struct device_node *node;
	u32 nr;

	if (_mdrv_adapter != NULL)
		return 0;

	mdrv_adapter = devm_kzalloc(&pdev->dev, sizeof(struct mdrv_dvb_adapter),
		GFP_KERNEL);
	if (!mdrv_adapter) {
		ADAPTER_DEBUG_ERR("allocate dvb adapter fail\n");
		return -ENOMEM;
	}

	of_node_get(pdev->dev.of_node);
	node = of_find_node_by_name(pdev->dev.of_node, "adap_nr");
	if (node != NULL) {
		ret = of_property_read_u32(node, "number", &nr);
		if (ret == 0 && nr != 0xFF)
			adapter_nr[0] = nr;
	}
	of_node_put(node);

	ret = dvb_register_adapter(&mdrv_adapter->adapter, DRV_NAME,
		THIS_MODULE, &pdev->dev, adapter_nr);
	if (ret < 0) {
		ADAPTER_DEBUG_ERR("register dvb adapter fail\n");
		return ret;
	}

#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	ret = _init_mdev(&mdrv_adapter->mdev, &mdrv_adapter->adapter);
	if (ret < 0) {
		ADAPTER_DEBUG_ERR("init media device fail\n");
		return ret;
	}

	_init_hash_list(mdrv_adapter);
#endif

	mdrv_adapter->pdev = pdev;

	platform_set_drvdata(pdev, mdrv_adapter);
	_mdrv_adapter = mdrv_adapter;

	ret = kobject_init_and_add(&mdrv_adapter->kobj, &dvb_adapter_ktype,
		&pdev->dev.kobj, "adapter_info");
	if (ret) {
		ADAPTER_DEBUG_ERR("kobj init & add fail\n");
		return -EFAULT;
	}
	sysfs_create_group(&mdrv_adapter->kobj, &dvb_adapter_attrs_group);

	return 0;
}

static int mdrv_adapter_remove(struct platform_device *pdev)
{
	struct mdrv_dvb_adapter *mdrv_adapter = platform_get_drvdata(pdev);

	sysfs_remove_group(&mdrv_adapter->kobj, &dvb_adapter_attrs_group);
	kobject_put(&mdrv_adapter->kobj);

	_mdrv_adapter = NULL;

#if defined(CONFIG_MEDIA_CONTROLLER_DVB)
	_release_mdev(&mdrv_adapter->mdev);
#endif

	dvb_unregister_adapter(&mdrv_adapter->adapter);

	return 0;
}

static const struct of_device_id mdrv_adapter_dt_match[] = {
	{ .compatible = DRV_NAME },
	{},
};
MODULE_DEVICE_TABLE(of, mdrv_adapter_dt_match);

static struct platform_driver mdrv_adapter_driver = {
	.probe          = mdrv_adapter_probe,
	.remove         = mdrv_adapter_remove,
	.driver         = {
		.name   = DRV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = mdrv_adapter_dt_match
	}
};

module_platform_driver(mdrv_adapter_driver);

MODULE_DESCRIPTION("Mediatek DVB Adapter Driver");
MODULE_AUTHOR("Mediatek Author");
MODULE_LICENSE("GPL");
