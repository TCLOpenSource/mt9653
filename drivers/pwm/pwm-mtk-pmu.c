// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pwm.h>		// for struct pwm_state and enum pwm_polarity

#define IPCM_MAX_DATA_SIZE 64
#define PWMPMU_TIMEOUT		msecs_to_jiffies(1000)

enum pwm_ipcm_ack_t {
	IPCM_CMD_OP = 0,
	IPCM_CMD_BAD_ACK,
	IPCM_CMD_ACK,
};

struct pwm_ipcm_msg {
	uint8_t cmd;
	uint32_t size;
	uint8_t data[IPCM_MAX_DATA_SIZE];
};

enum pwm_proxy_cmd_t {
	PWM_PROXY_CMD_OPEN = 0,
	PWM_PROXY_CMD_CONFIG,
	PWM_PROXY_CMD_SET_ENABLE,
	PWM_PROXY_CMD_SET_POLARITY,
	PWM_PROXY_CMD_GET_STATE,
	PWM_PROXY_CMD_CONFIG_PRIVATE,
	PWM_PROXY_CMD_GET_PRIVATE_STATE,
	PWM_PROXY_CMD_CLOSE,
	//...
	PWM_PROXY_CMD_MAX,
};

/*
 * struct pwm_state - state of a PWM channel
 * @period: PWM period (in nanoseconds)
 * @duty_cycle: PWM duty cycle (in nanoseconds)
 * @polarity: PWM polarity
 * @enabled: PWM enabled status
 */
struct pwm_proxy_host_state {
	uint32_t period;
	uint32_t duty_cycle;
	int32_t polarity;
	bool enabled;
};

struct pwm_proxy_host_prv_config {
	uint32_t shift;
	uint32_t div;
	bool rst_mux;
	bool rst_vsync;
	uint32_t rst_cnt;
};

struct pwm_proxy_msg_open {
	uint8_t cmd;
	char name[IPCM_MAX_DATA_SIZE - sizeof(uint8_t)];
};

struct pwm_proxy_msg_resp_open {
	uint8_t cmd;
	uint32_t hwpwm;
};

struct pwm_proxy_msg_close {
	uint8_t cmd;
};

struct pwm_proxy_msg_resp_nodata {
	uint8_t cmd;
};

struct pwm_proxy_msg_config {
	uint8_t cmd;
	uint32_t channel;
	int32_t duty_ns;
	int32_t period_ns;
};

struct pwm_proxy_msg_set_enable {
	uint8_t cmd;
	uint32_t channel;
	bool enable;
};

struct pwm_proxy_msg_set_pol {
	uint8_t cmd;
	uint32_t channel;
	int32_t polarity; /* NOTE: enum in linux and rtos is different */
};

struct pwm_proxy_msg_get_state {
	uint8_t cmd;
	uint32_t channel;
};

struct pwm_proxy_msg_resp_get_state {
	uint8_t cmd;
	struct pwm_proxy_host_state state;
};

struct pwm_proxy_msg_config_prv {
	uint8_t cmd;
	uint32_t channel;
	struct pwm_proxy_host_prv_config prv_config;
};

struct pwm_proxy_msg_get_prv_state {
	uint8_t cmd;
	uint32_t channel;
};

struct pwm_proxy_msg_resp_get_prv_state {
	uint8_t cmd;
	struct pwm_proxy_host_prv_config prv_config;
};

struct mtk_pmpwm_chip_dev;

/* The rpmsg bus device */
struct mtk_rpmsg_pwm {
	struct device *dev;
	struct rpmsg_device *rpdev;
	int (*txn)(struct mtk_rpmsg_pwm *rpmsg_pwm, void *data,
		    size_t len);
	int txn_result;
	struct pwm_ipcm_msg receive_msg;
	struct completion ack;
	struct mtk_pmpwm_chip_dev *pwm_dev;
	struct list_head list;
};

/**
 * struct mtk_pmpwm_chip_dev - pwm chip driver private data
 *
 * @dev: platform device of pwm chip
 * @chip: pwm chip of pwm subsystem
 * @rpmsg_dev: rpmsg device for send cmd to PMU
 * @rpmsg_bind: completion when rpmsg and pwm device binded
 * @pmu_pwm_name: PMU side pwm chip name
 * @pmu_pwm_opened: PMU side pwm chip handle status (opened/closed)
 * @list: for create a list
 * @hwpwm: pwm channel count, reported when open rpmsg pwm device
 */
struct mtk_pmpwm_chip_dev {
	struct device *dev;
	struct pwm_chip chip;
	struct mtk_rpmsg_pwm *rpmsg_dev;
	struct completion rpmsg_bind;
	const char *pmu_pwm_name;
	bool pmu_pwm_opened;
	struct list_head list;
	uint32_t hwpwm;
};

/* A list of pmpwm_chip_dev */
static LIST_HEAD(pmpwm_chip_dev_list);
/* A list of rpmsg_dev */
static LIST_HEAD(rpmsg_dev_list);
/* Lock to serialise RPMSG device and PWM device binding */
static DEFINE_MUTEX(rpmsg_pwm_bind_lock);

/*
 * PWM chip device and rpmsg device binder
 */
static int _mtk_bind_rpmsg_pmpwm(struct mtk_rpmsg_pwm *rpmsg_pwm_dev,
				struct mtk_pmpwm_chip_dev *pmpwm_chip_dev)
{
	mutex_lock(&rpmsg_pwm_bind_lock);

	if (rpmsg_pwm_dev) {
		struct mtk_pmpwm_chip_dev *pwm_chip_dev, *n;

		// check pmpwm chip device list,
		// link 1st pmpwm chip dev which not link to rpmsg dev
		list_for_each_entry_safe(pwm_chip_dev, n, &pmpwm_chip_dev_list,
					 list) {
			if (pwm_chip_dev->rpmsg_dev == NULL) {
				pwm_chip_dev->rpmsg_dev = rpmsg_pwm_dev;
				rpmsg_pwm_dev->pwm_dev = pwm_chip_dev;
				complete(&pwm_chip_dev->rpmsg_bind);
				break;
			}
		}
	} else {
		struct mtk_rpmsg_pwm *rpmsg_dev, *n;

		// check rpmsg device list,
		// link 1st rpmsg dev which not link to pmpwm chip dev
		list_for_each_entry_safe(rpmsg_dev, n, &rpmsg_dev_list, list) {
			if (rpmsg_dev->pwm_dev == NULL) {
				rpmsg_dev->pwm_dev = pmpwm_chip_dev;
				pmpwm_chip_dev->rpmsg_dev = rpmsg_dev;
				complete(&pmpwm_chip_dev->rpmsg_bind);
				break;
			}
		}
	}

	mutex_unlock(&rpmsg_pwm_bind_lock);

	return 0;
}

/*
 * RPMSG functions
 */
static int mtk_pmu_pwm_txn(struct mtk_rpmsg_pwm *rpmsg_pwm, void *data,
			   size_t len)
{
	int ret;

	/* sanity check */
	if (!rpmsg_pwm) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		dev_err(rpmsg_pwm->dev, "Invalid data or length\n");
		return -EINVAL;
	}
#ifdef DEBUG
	pr_info("Send:\n");
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1, data, len,
		       true);
#endif
	rpmsg_pwm->txn_result = -EIO;
	ret = rpmsg_send(rpmsg_pwm->rpdev->ept, data, len);
	if (ret) {
		dev_err(rpmsg_pwm->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}
	// wait for ack
	ret = wait_for_completion_interruptible_timeout
				(&rpmsg_pwm->ack, PWMPMU_TIMEOUT);
	if (ret) {
		dev_err(rpmsg_pwm->dev, "proxy timeout, ret=%d\n", ret);
		return ret;
	}

	return rpmsg_pwm->txn_result;
}

static int mtk_rpmsg_pwm_callback(struct rpmsg_device *rpdev, void *data,
				  int len, void *priv, u32 src)
{
	struct mtk_rpmsg_pwm *rpmsg_pwm;
	struct pwm_ipcm_msg *msg;

	/* sanity check */
	if (!rpdev) {
		dev_err(NULL, "rpdev is NULL\n");
		return -EINVAL;
	}

	rpmsg_pwm = dev_get_drvdata(&rpdev->dev);
	if (!rpmsg_pwm) {
		dev_err(&rpdev->dev, "private data is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		dev_err(&rpdev->dev, "Invalid data or length from src:%d\n",
			src);
		return -EINVAL;
	}

	msg = (struct pwm_ipcm_msg *)data;
#ifdef DEBUG
	pr_info("%s:\n", __func__);
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1, data, len,
		       true);
#endif
	switch (msg->cmd) {
	case IPCM_CMD_OP:
		dev_dbg(&rpdev->dev, "OP CMD\n");
		break;
	case IPCM_CMD_ACK:
		dev_dbg(&rpdev->dev, "Got OK ack, response for cmd %d\n",
			msg->data[0]);
		if (len > sizeof(struct pwm_ipcm_msg))
			dev_err(&rpdev->dev, "Receive data length over size\n");

		/* store received data for transaction checking */
		memcpy(&rpmsg_pwm->receive_msg, data,
		       sizeof(struct pwm_ipcm_msg));
		rpmsg_pwm->txn_result = 0;
		complete(&rpmsg_pwm->ack);
		break;
	case IPCM_CMD_BAD_ACK:
		dev_err(&rpdev->dev, "Got BAD ack\n");
		complete(&rpmsg_pwm->ack);
		// TODO: panic here?
		return -EINVAL;
	default:
		dev_err(&rpdev->dev, "Invalid command %d from src:%d\n",
			msg->cmd, src);
		return -EINVAL;
	}

	return 0;
}

static int mtk_rpmsg_pwm_probe(struct rpmsg_device *rpdev)
{
	int ret;
	struct mtk_rpmsg_pwm *rpmsg_pwm;

	rpmsg_pwm =
	    devm_kzalloc(&rpdev->dev, sizeof(struct mtk_rpmsg_pwm), GFP_KERNEL);
	if (!rpmsg_pwm)
		return -ENOMEM;

	// assigne the member
	rpmsg_pwm->rpdev = rpdev;
	rpmsg_pwm->dev = &rpdev->dev;
	init_completion(&rpmsg_pwm->ack);
	rpmsg_pwm->txn = mtk_pmu_pwm_txn;

	// add to rpmsg_dev list
	INIT_LIST_HEAD(&rpmsg_pwm->list);
	list_add_tail(&rpmsg_pwm->list, &rpmsg_dev_list);

	// check if there is a valid pmpwm_chip_dev
	ret = _mtk_bind_rpmsg_pmpwm(rpmsg_pwm, NULL);
	if (ret) {
		dev_err(&rpdev->dev, "binding rpmsg-pmpwm failed! (%d)\n", ret);
		goto bind_fail;
	}

	dev_set_drvdata(&rpdev->dev, rpmsg_pwm);
	return 0;

bind_fail:
	list_del(&rpmsg_pwm->list);
	return ret;
}

static void mtk_rpmsg_pwm_remove(struct rpmsg_device *rpdev)
{
	struct mtk_rpmsg_pwm *rpmsg_pwm = dev_get_drvdata(&rpdev->dev);

	list_del(&rpmsg_pwm->list);
	dev_set_drvdata(&rpdev->dev, NULL);
	dev_info(&rpdev->dev, "%s\n", __func__);
}

#ifdef CONFIG_PM
static int _mtk_open_pmu_device(struct mtk_pmpwm_chip_dev *pwm_chip_dev);
static int _mtk_close_pmu_device(struct mtk_pmpwm_chip_dev *pwm_chip_dev);

static int mtk_rpmsg_pwm_suspend(struct device *dev)
{
	struct mtk_rpmsg_pwm *rpmsg_pwm = dev_get_drvdata(dev);

	// close pmu pwm handle
	return _mtk_close_pmu_device(rpmsg_pwm->pwm_dev);
}

static int mtk_rpmsg_pwm_resume(struct device *dev)
{
	struct mtk_rpmsg_pwm *rpmsg_pwm = dev_get_drvdata(dev);

	// open pmu pwm handle
	return _mtk_open_pmu_device(rpmsg_pwm->pwm_dev);
}

static const struct dev_pm_ops mtk_rpmsg_pwm_pm_ops = {
	.suspend = mtk_rpmsg_pwm_suspend,
	.resume = mtk_rpmsg_pwm_resume,
};
#endif

static struct rpmsg_device_id mtk_rpmsg_pwm_id_table[] = {
	{.name = "pwm-pmu-ept"},	// this name must match with rtos
	{},
};

MODULE_DEVICE_TABLE(rpmsg, mtk_rpmsg_pwm_id_table);

static struct rpmsg_driver mtk_rpmsg_pwm_driver = {
	.drv.name = KBUILD_MODNAME,
	.drv.owner = THIS_MODULE,
#ifdef CONFIG_PM
	.drv.pm = &mtk_rpmsg_pwm_pm_ops,
#endif
	.id_table = mtk_rpmsg_pwm_id_table,
	.probe = mtk_rpmsg_pwm_probe,
	.callback = mtk_rpmsg_pwm_callback,
	.remove = mtk_rpmsg_pwm_remove,
};

/*
 * Common procedures for communicate linux subsystem and rpmsg
 */
static inline int __mtk_rpmsg_txn_sync(struct mtk_pmpwm_chip_dev *pwm_chip_dev,
				       uint8_t cmd)
{
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_chip_dev->rpmsg_dev;
	struct pwm_proxy_msg_resp_nodata *res_msg =
	    (struct pwm_proxy_msg_resp_nodata *)&rpmsg_pwm->receive_msg.data;

	if (res_msg->cmd != cmd) {
		dev_err(pwm_chip_dev->dev,
			"Out of sync command, send %d get %d\n", cmd,
			res_msg->cmd);
		return -ENOTSYNC;
	}

	return 0;
}

static int __mtk_pmpwm_open_pmu_handle(struct mtk_pmpwm_chip_dev *pwm_dev,
				       const char *pmu_dev_name,
				       uint32_t *hwpwm)
{
	int ret;
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_dev->rpmsg_dev;
	struct pwm_proxy_msg_open *pproxy_msg;
	struct pwm_ipcm_msg open = {0};
	struct pwm_proxy_msg_resp_open *resp_msg =
	    (struct pwm_proxy_msg_resp_open *)&rpmsg_pwm->receive_msg.data;

	if (hwpwm)
		*hwpwm = 0;

	open.cmd = IPCM_CMD_OP;
	open.size = sizeof(struct pwm_proxy_msg_open);
	pproxy_msg = (struct pwm_proxy_msg_open *)&open.data;
	pproxy_msg->cmd = PWM_PROXY_CMD_OPEN;
	strncpy(pproxy_msg->name, pmu_dev_name, sizeof(pproxy_msg->name));
	pproxy_msg->name[sizeof(pproxy_msg->name)-1] = '\0';

	/* execute RPMSG command */
	ret = rpmsg_pwm->txn(rpmsg_pwm, &open, sizeof(struct pwm_ipcm_msg));
	if (ret)
		return ret;

	// check received result
	ret = __mtk_rpmsg_txn_sync(pwm_dev, pproxy_msg->cmd);
	if (ret)
		return ret;

	dev_dbg(pwm_dev->dev, "got channel count %d\n", resp_msg->hwpwm);

	if (!__mtk_rpmsg_txn_sync(pwm_dev, pproxy_msg->cmd) && hwpwm)
		*hwpwm = resp_msg->hwpwm;

	return rpmsg_pwm->txn_result;
}

static int __mtk_pmpwm_close_pmu_handle(struct mtk_pmpwm_chip_dev *pwm_dev)
{
	int ret;
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_dev->rpmsg_dev;
	struct pwm_proxy_msg_close *pproxy_msg;
	struct pwm_ipcm_msg close = {0};

	close.cmd = IPCM_CMD_OP;
	close.size = sizeof(struct pwm_proxy_msg_close);
	pproxy_msg = (struct pwm_proxy_msg_close *)&close.data;
	pproxy_msg->cmd = PWM_PROXY_CMD_CLOSE;

	/* execute RPMSG command */
	ret = rpmsg_pwm->txn(rpmsg_pwm, &close, sizeof(struct pwm_ipcm_msg));
	if (ret)
		return ret;

	// check received result
	ret = __mtk_rpmsg_txn_sync(pwm_dev, pproxy_msg->cmd);
	if (ret)
		return ret;

	return rpmsg_pwm->txn_result;
}

static int _mtk_open_pmu_device(struct mtk_pmpwm_chip_dev *pwm_chip_dev)
{
	int ret = 0;

	if (pwm_chip_dev->rpmsg_dev == NULL) {
		dev_err(pwm_chip_dev->dev,
			"Can not find valid rpmsg device(s)\n");
		return -EBUSY;
	}

	if (pwm_chip_dev->pmu_pwm_opened == false) {
		// open pmu side pwm handle
		ret =
		    __mtk_pmpwm_open_pmu_handle(pwm_chip_dev,
						pwm_chip_dev->pmu_pwm_name,
						&pwm_chip_dev->hwpwm);
		if (ret) {
			dev_err(pwm_chip_dev->dev,
				"failed to open pmu pwm device %s\n",
				pwm_chip_dev->pmu_pwm_name);
			return ret;
		}

		pwm_chip_dev->pmu_pwm_opened = true;
	}

	return ret;
}

static int _mtk_close_pmu_device(struct mtk_pmpwm_chip_dev *pwm_chip_dev)
{
	int ret = 0;

	if (pwm_chip_dev->rpmsg_dev == NULL) {
		dev_err(pwm_chip_dev->dev,
			"Can not find valid rpmsg device(s)\n");
		return -EBUSY;
	}

	if (pwm_chip_dev->pmu_pwm_opened == true) {
		// close pmu side pwm handle
		ret = __mtk_pmpwm_close_pmu_handle(pwm_chip_dev);
		if (ret) {
			dev_err(pwm_chip_dev->dev,
				"failed to close pmu pwm device %s\n",
				pwm_chip_dev->pmu_pwm_name);
			return ret;
		}

		pwm_chip_dev->pmu_pwm_opened = false;
	}

	return ret;
}

/*
 * PWM subsystem callback functions
 */
static inline struct mtk_pmpwm_chip_dev *to_mtk_pmpwm_chip(struct pwm_chip *c)
{
	return container_of(c, struct mtk_pmpwm_chip_dev, chip);
}

static int mtk_pmpwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			    int duty_ns, int period_ns)
{
	struct mtk_pmpwm_chip_dev *pwm_chip_dev = to_mtk_pmpwm_chip(chip);
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_chip_dev->rpmsg_dev;
	struct pwm_ipcm_msg msg = {0};
	struct pwm_proxy_msg_config *pproxy_msg =
	    (struct pwm_proxy_msg_config *)&msg.data;
	int ret = 0;

	msg.cmd = IPCM_CMD_OP;
	msg.size = sizeof(struct pwm_proxy_msg_config);
	pproxy_msg->cmd = PWM_PROXY_CMD_CONFIG;
	pproxy_msg->channel = pwm->hwpwm;
	pproxy_msg->duty_ns = duty_ns;
	pproxy_msg->period_ns = period_ns;
	ret = rpmsg_pwm->txn(rpmsg_pwm, (void *)&msg, sizeof(msg));
	if (ret)
		return ret;

	return __mtk_rpmsg_txn_sync(pwm_chip_dev, pproxy_msg->cmd);
}

static int mtk_pmpwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pmpwm_chip_dev *pwm_chip_dev = to_mtk_pmpwm_chip(chip);
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_chip_dev->rpmsg_dev;
	struct pwm_ipcm_msg msg = {0};
	struct pwm_proxy_msg_set_enable *pproxy_msg =
	    (struct pwm_proxy_msg_set_enable *)&msg.data;
	int ret = 0;

	msg.cmd = IPCM_CMD_OP;
	msg.size = sizeof(struct pwm_proxy_msg_set_enable);
	pproxy_msg->cmd = PWM_PROXY_CMD_SET_ENABLE;
	pproxy_msg->channel = pwm->hwpwm;
	pproxy_msg->enable = true;
	ret = rpmsg_pwm->txn(rpmsg_pwm, (void *)&msg, sizeof(msg));
	if (ret)
		return ret;

	return __mtk_rpmsg_txn_sync(pwm_chip_dev, pproxy_msg->cmd);
}

static void mtk_pmpwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct mtk_pmpwm_chip_dev *pwm_chip_dev = to_mtk_pmpwm_chip(chip);
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_chip_dev->rpmsg_dev;
	struct pwm_ipcm_msg msg = {0};
	struct pwm_proxy_msg_set_enable *pproxy_msg =
	    (struct pwm_proxy_msg_set_enable *)&msg.data;
	int ret = 0;

	msg.cmd = IPCM_CMD_OP;
	msg.size = sizeof(struct pwm_proxy_msg_set_enable);
	pproxy_msg->cmd = PWM_PROXY_CMD_SET_ENABLE;
	pproxy_msg->channel = pwm->hwpwm;
	pproxy_msg->enable = false;
	ret = rpmsg_pwm->txn(rpmsg_pwm, (void *)&msg, sizeof(msg));
	if (ret)
		return;

	__mtk_rpmsg_txn_sync(pwm_chip_dev, pproxy_msg->cmd);
}

static int mtk_pmpwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm,
				  enum pwm_polarity polarity)
{
	struct mtk_pmpwm_chip_dev *pwm_chip_dev = to_mtk_pmpwm_chip(chip);
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_chip_dev->rpmsg_dev;
	struct pwm_ipcm_msg msg = {0};
	struct pwm_proxy_msg_set_pol *pproxy_msg =
	    (struct pwm_proxy_msg_set_pol *)&msg.data;
	int ret = 0;

	msg.cmd = IPCM_CMD_OP;
	msg.size = sizeof(struct pwm_proxy_msg_set_pol);
	pproxy_msg->cmd = PWM_PROXY_CMD_SET_POLARITY;
	pproxy_msg->channel = pwm->hwpwm;
	pproxy_msg->polarity = (int32_t)polarity;
	ret = rpmsg_pwm->txn(rpmsg_pwm, (void *)&msg, sizeof(msg));
	if (ret)
		return ret;

	return __mtk_rpmsg_txn_sync(pwm_chip_dev, pproxy_msg->cmd);
}

static void mtk_pmpwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
				struct pwm_state *state)
{
	struct mtk_pmpwm_chip_dev *pwm_chip_dev = to_mtk_pmpwm_chip(chip);
	struct mtk_rpmsg_pwm *rpmsg_pwm = pwm_chip_dev->rpmsg_dev;
	struct pwm_ipcm_msg msg = {0};
	struct pwm_proxy_msg_get_state *pproxy_msg =
	    (struct pwm_proxy_msg_get_state *)&msg.data;
	struct pwm_proxy_msg_resp_get_state *resp_msg =
	    (struct pwm_proxy_msg_resp_get_state *)&rpmsg_pwm->receive_msg.data;
	int ret = 0;

	msg.cmd = IPCM_CMD_OP;
	msg.size = sizeof(struct pwm_proxy_msg_get_state);
	pproxy_msg->cmd = PWM_PROXY_CMD_GET_STATE;
	pproxy_msg->channel = pwm->hwpwm;
	ret = rpmsg_pwm->txn(rpmsg_pwm, (void *)&msg, sizeof(msg));
	if (ret)
		return;

	if (!__mtk_rpmsg_txn_sync(pwm_chip_dev, pproxy_msg->cmd)) {
		dev_dbg(pwm_chip_dev->dev, "pwm %d state\n",
			pproxy_msg->channel);
		dev_dbg(pwm_chip_dev->dev, "period %d\n",
			resp_msg->state.period);
		dev_dbg(pwm_chip_dev->dev, "duty cycle %d\n",
			resp_msg->state.duty_cycle);
		dev_dbg(pwm_chip_dev->dev, "polarity %d\n",
			resp_msg->state.polarity);
		dev_dbg(pwm_chip_dev->dev, "enabled %d\n",
			resp_msg->state.enabled);
		state->period = resp_msg->state.period;
		state->duty_cycle = resp_msg->state.duty_cycle;
		state->polarity = resp_msg->state.polarity;
		state->enabled = resp_msg->state.enabled;
	}
}

static const struct pwm_ops mtk_pmpwm_ops = {
	.config = mtk_pmpwm_config,
	.enable = mtk_pmpwm_enable,
	.disable = mtk_pmpwm_disable,
	.set_polarity = mtk_pmpwm_set_polarity,
	.get_state = mtk_pmpwm_get_state,
	.owner = THIS_MODULE,
};

/*
 * TODO: Maybe we should use other mechanism (deferred probe mechanism or
 *       somthing like that) for binding rpmsg device driver and pwm chip device
 *       driver.
 */
static int pwm_chip_add_thread(void *param)
{
	int ret;
	struct mtk_pmpwm_chip_dev *pwm_dev = (struct mtk_pmpwm_chip_dev *)param;

	dev_info(pwm_dev->dev, "Wait pwm chip and rpmsg device binding...\n");

	// complete when rpmsg device and pwm chip device binded
	wait_for_completion(&pwm_dev->rpmsg_bind);
	dev_dbg(pwm_dev->dev, "pwm chip and rpmsg device binded\n");

	ret = _mtk_open_pmu_device(pwm_dev);
	if (ret)
		goto end;

	pwm_dev->chip.npwm = 6;
	pwm_dev->chip.base = -1;
	ret = pwmchip_add(&pwm_dev->chip);
	if (ret < 0) {
		dev_err(pwm_dev->dev, "pwmchip_add failed (%d)\n", ret);
		goto end;
	}

	dev_info(pwm_dev->dev, "add pwm chip start with pwm channel %d\n",
		 pwm_dev->chip.base);

end:
	do_exit(0);
	return 0;
}

static int mtk_pmpwm_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mtk_pmpwm_chip_dev *pwm_dev;
	static struct task_struct *t;

	dev_dbg(&pdev->dev, "%s\n", __func__);

	/* init mtk_pmpwm_chip_dev device struct */
	pwm_dev =
	    devm_kzalloc(&pdev->dev, sizeof(struct mtk_pmpwm_chip_dev),
			 GFP_KERNEL);
	if (IS_ERR(pwm_dev)) {
		dev_err(&pdev->dev, "unable to allocate memory (%ld)\n",
			PTR_ERR(pwm_dev));
		return PTR_ERR(pwm_dev);
	}

	platform_set_drvdata(pdev, pwm_dev);

	// add to pmpwm_chip_dev list
	INIT_LIST_HEAD(&pwm_dev->list);
	list_add_tail(&pwm_dev->list, &pmpwm_chip_dev_list);

	init_completion(&pwm_dev->rpmsg_bind);

	// check if there is a valid rpmsg_dev
	ret = _mtk_bind_rpmsg_pmpwm(NULL, pwm_dev);
	if (ret) {
		dev_err(&pdev->dev, "binding rpmsg-pmpwm failed! (%d)\n", ret);
		goto bind_fail;
	}

	pwm_dev->dev = &pdev->dev;
	pwm_dev->pmu_pwm_name = of_node_full_name(pdev->dev.of_node);
	pwm_dev->pmu_pwm_opened = false;

	/* register pwm chip and pwm ops */
	pwm_dev->chip.dev = &pdev->dev;
	pwm_dev->chip.ops = &mtk_pmpwm_ops;
	pwm_dev->chip.base = -1;
	/* to get # of channel(s) of pmu pwm chip in open pmu pwm chip */

	/*
	 * TODO: check probe behavior match definition of platform driver
	 *       because initialization is not complete
	 */
	/* thread for waiting rpmsg binded, continue rest initial sequence after
	 * rpmsg device binded.
	 */
	t = kthread_run(pwm_chip_add_thread, (void *)pwm_dev, "pwm_chip_add");
	if (IS_ERR(t)) {
		dev_err(&pdev->dev, "create thread for add pwm chip failed!\n");
		ret = IS_ERR(t);
		goto bind_fail;
	}

	return 0;

bind_fail:
	list_del(&pwm_dev->list);
	return ret;
}

static int mtk_pmpwm_remove(struct platform_device *pdev)
{
	struct mtk_pmpwm_chip_dev *pwm_dev = platform_get_drvdata(pdev);

	// close pmu pwm handle
	_mtk_close_pmu_device(pwm_dev);

	pwmchip_remove(&pwm_dev->chip);

	list_del(&pwm_dev->list);

	return 0;
}

static const struct of_device_id mt5896_pmpwm_of_match[] = {
	{.compatible = "mediatek,mt5896-rpmsg-pwm"},
	{},
};

MODULE_DEVICE_TABLE(of, mt5896_pmpwm_of_match);

static struct platform_driver mt5896_pmpwm_driver = {
	.probe = mtk_pmpwm_probe,
	.remove = mtk_pmpwm_remove,
	.driver = {
		.of_match_table = of_match_ptr(mt5896_pmpwm_of_match),
		.name = "mtk-mt5896-pmpwm",
		.owner = THIS_MODULE,
	},
};

static int __init mtk_rpmsg_pmpwm_init_driver(void)
{
	int ret = 0;

	ret = platform_driver_register(&mt5896_pmpwm_driver);
	if (ret) {
		pr_err("pwm chip driver register failed (%d)\n", ret);
		return ret;
	}

	ret = register_rpmsg_driver(&mtk_rpmsg_pwm_driver);
	if (ret) {
		pr_err("rpmsg bus driver register failed (%d)\n", ret);
		platform_driver_unregister(&mt5896_pmpwm_driver);
		return ret;
	}

	return ret;
}

module_init(mtk_rpmsg_pmpwm_init_driver);

static void __exit mtk_rpmsg_pmpwm_exit_driver(void)
{
	unregister_rpmsg_driver(&mtk_rpmsg_pwm_driver);
	platform_driver_unregister(&mt5896_pmpwm_driver);
}

module_exit(mtk_rpmsg_pmpwm_exit_driver);

MODULE_AUTHOR("xxx <xxx@mediatek.com>");
MODULE_DESCRIPTION("Mediatek PMU pwm driver");
MODULE_LICENSE("GPL v2");
