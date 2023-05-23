// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <sound/soc.h>
#include "audio_mcu_communication.h"
#include "audio_mcu_rpmsg.h"
#include "audio_mtk_dbg.h"

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static unsigned long mtk_log_level = LOGLEVEL_INFO;
static LIST_HEAD(rpmsg_dev_list);
struct mtk_audio_mcu_rpmsg *g_audio_mcu_rpmsg[AUDIO_MCU_CLIENT_NUM_MAX];

/* ept-name table */
static const char *audio_mcu_ept[AUDIO_MCU_CLIENT_NUM_MAX] = {
	AUDIO_MCU_DEC_EPT_NAME,
	AUDIO_MCU_SND_EPT_NAME
};

static uint32_t audio_mcu_device_mapping[AUDIO_MCU_DEVICE_NUM_MAX] = {
	AUDIO_MCU_CLIENT_NUM_MAX,
	AUDIO_MCU_CLIENT_NUM_MAX,
	AUDIO_MCU_CLIENT_NUM_MAX,
	AUDIO_MCU_CLIENT_NUM_MAX,
	AUDIO_MCU_CLIENT_NUM_MAX
};

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
/*
 * RPMSG functions
 */
static int mtk_audio_mcu_rpmsg_txn(struct mtk_audio_mcu_rpmsg *audio_mcu_rpmsg,
	uint8_t mcu_device, void *data, size_t len)
{
	int ret;

	/* sanity check */
	if (!audio_mcu_rpmsg) {
		audio_err("rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		audio_err("Invalid data or length\n");
		return -EINVAL;
	}

	// audio_info("Send:\n");
	// print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, DEBUG_GROUP_SIZE, 1, data, len, true);

	audio_mcu_rpmsg->txn_result = -EIO;
	ret = rpmsg_send(audio_mcu_rpmsg->rpdev->ept, data, len);
	if (ret) {
		audio_err("rpmsg send failed, ret=%d\n", ret);
		return ret;
	}
	// wait for ack
	ret = wait_for_completion_interruptible(&audio_mcu_rpmsg->ack[mcu_device]);
	if (!ret) {
		audio_err("wait_for_completion_interruptible failed, ret=%d\n", ret);
		return ret;
	}

	return audio_mcu_rpmsg->txn_result;
}

static inline int mtk_audio_mcu_rpmsg_txn_sync(struct mtk_audio_mcu_rpmsg *audio_mcu_rpmsg,
	uint8_t cmd)
{
struct audio_no_data_res_pkt *res_pk =
	    (struct audio_no_data_res_pkt *)&audio_mcu_rpmsg->receive_msg.data;

	if (res_pk->cmd != cmd) {
		audio_err("Out of sync command, send %d get %d\n", cmd,
			res_pk->cmd);
		return -ENOTSYNC;
	}

	return 0;
}

static int mtk_audio_mcu_rpmsg_rx_handler(struct mtk_audio_mcu_rpmsg *audio_mcu_rpmsg,
	struct audio_mcu_device_config *audio_com_device, int mcu)
{
	struct ipcm_msg_packet *audio_mcu_msg_packet = NULL;
	// struct audio_mcu_device_config *audio_com_device = NULL;
	struct communication_struct audio_com_packet = {
		.com_cmd = 0,
		.com_size = 0,
		.com_data = 0,
	};
	int ret = 0;

	/* sanity check */
	if (!audio_mcu_rpmsg) {
		audio_err("rpmsg is NULL\n");
		return -EINVAL;
	}

	if (!audio_mcu_rpmsg->dev) {
		audio_err("rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (mcu >= AUDIO_MCU_CLIENT_NUM_MAX ||
		mcu < AUDIO_MCU_CLIENT_AUDEC) {
		audio_err("rpmsg mcu is NULL\n");
		return -EINVAL;
	}

	audio_mcu_msg_packet = (struct ipcm_msg_packet *)&audio_mcu_rpmsg->receive_msg.data;

	dev_dbg(audio_mcu_rpmsg->dev,
			"MSG_PERIOD_NOTIFY, get %d\n", audio_mcu_msg_packet->cmd);

	audio_com_packet.com_cmd = audio_mcu_msg_packet->cmd;
	audio_com_packet.com_data = &audio_mcu_msg_packet->payload;

	ret = g_audio_mcu_rpmsg[mcu]->rx_callback(&audio_com_packet, audio_com_device);

	return ret;
}

static int mtk_audio_mcu_rpmsg_rx_ack_handler(struct mtk_audio_mcu_rpmsg *audio_mcu_rpmsg,
	struct audio_mcu_device_config *audio_com_device, int mcu)
{
	struct audio_mcu_rpmsg_packet audio_com_packet = {
		.rpmsg_device = 0,
		.rpmsg_status = 0,
	};
	int ret = 0;

	/* sanity check */
	if (!audio_mcu_rpmsg) {
		audio_err("rpmsg is NULL\n");
		return -EINVAL;
	}

	if (!audio_mcu_rpmsg->dev) {
		audio_err("rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (mcu >= AUDIO_MCU_CLIENT_NUM_MAX ||
		mcu < AUDIO_MCU_CLIENT_AUDEC) {
		audio_err("rpmsg mcu is NULL\n");
		return -EINVAL;
	}

	// dev_dbg(audio_mcu_rpmsg->dev,
			// "MSG_PERIOD_NOTIFY, get %d\n", audio_mcu_msg_packet->cmd);

	memcpy(&audio_com_packet, &audio_mcu_rpmsg->receive_msg.data,
		sizeof(struct audio_mcu_rpmsg_packet));

	audio_info("%s ack device: %d status: %d ts: %d us", __func__,
		audio_com_packet.rpmsg_device,
		audio_com_packet.rpmsg_status,
		audio_com_packet.rpmsg_ts);

	// Timestamp for perforamce
	audio_mcu_rpmsg->ts = audio_com_packet.rpmsg_ts;

	// check device status
	if (audio_com_packet.rpmsg_status) {
		audio_mcu_device_mapping[audio_com_packet.rpmsg_device] = mcu;
		audio_err("%s mcu:%d support device:%d", __func__,
			mcu, audio_com_packet.rpmsg_device);
	} else {
		audio_err("%s mcu:%d not support device:%d", __func__,
			mcu, audio_com_packet.rpmsg_device);
	}

	complete(&g_audio_mcu_rpmsg[mcu]->ack[audio_com_packet.rpmsg_device]);

	return ret;
}

uint32_t mtk_audio_mcu_rpmsg_reset(uint8_t device)
{
	audio_mcu_device_mapping[device] = AUDIO_MCU_CLIENT_NUM_MAX;

	audio_info("%s %d\n", __func__, device);
	return 0;
}

uint32_t mtk_audio_mcu_rpmsg_performance(uint8_t device)
{
	return g_audio_mcu_rpmsg[device]->ts;
}

int mtk_audio_mcu_rpmsg_ipcm_cmd_send(struct communication_struct *rpmsg_packet,
	struct audio_mcu_device_config *rpmsg_device, int size)
{
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};
	struct ipcm_msg_packet *audio_mcu_msg_packet;
	uint8_t mcu_device = 0;
	int mcu = 0;

	audio_mcu_msg_packet = (struct ipcm_msg_packet *)&open_msg.data;
	audio_mcu_msg_packet->cmd = rpmsg_packet->com_cmd;
	memcpy(&audio_mcu_msg_packet->payload, rpmsg_packet->com_data, rpmsg_packet->com_size);

	if (rpmsg_device->device >= AUDIO_MCU_DEVICE_NUM_MAX) {
		audio_err("rpmsg device is NULL\n");
		return -EINVAL;
	}

	mcu_device = rpmsg_device->device;

	if (audio_mcu_device_mapping[mcu_device] == AUDIO_MCU_CLIENT_NUM_MAX) {
		audio_err("rpmsg device mapping is NULL\n");
		return -EINVAL;
	}

	rpmsg_send(g_audio_mcu_rpmsg[audio_mcu_device_mapping[mcu_device]]->
		rpdev->ept, &open_msg, sizeof(struct ipcm_msg));

	return g_audio_mcu_rpmsg[mcu]->txn_result;
}

int mtk_audio_mcu_rpmsg_ipcm_cmd_send_ack(struct communication_struct *rpmsg_packet,
	struct audio_mcu_device_config *rpmsg_device, int size)
{
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};
	struct ipcm_msg_packet *audio_mcu_msg_packet;
	uint8_t mcu_device = 0;
	// int ret = 0;
	int mcu = 0;

	audio_mcu_msg_packet = (struct ipcm_msg_packet *)&open_msg.data;
	audio_mcu_msg_packet->cmd = rpmsg_packet->com_cmd;
	memcpy(&audio_mcu_msg_packet->payload, rpmsg_packet->com_data, rpmsg_packet->com_size);

	if (rpmsg_device->device >= AUDIO_MCU_DEVICE_NUM_MAX) {
		audio_err("rpmsg device is NULL\n");
		return -EINVAL;
	}

	mcu_device = rpmsg_device->device;

	audio_err("%s %d\n", __func__, mcu_device);

	// frist time need
	if (audio_mcu_device_mapping[mcu_device] == AUDIO_MCU_CLIENT_NUM_MAX) {
		mtk_audio_mcu_rpmsg_txn(g_audio_mcu_rpmsg[AUDIO_MCU_CLIENT_AUDEC],
			mcu_device, &open_msg, sizeof(struct ipcm_msg));
		mtk_audio_mcu_rpmsg_txn(g_audio_mcu_rpmsg[AUDIO_MCU_CLIENT_AUSND],
			mcu_device, &open_msg, sizeof(struct ipcm_msg));
	} else {
		mtk_audio_mcu_rpmsg_txn(g_audio_mcu_rpmsg[audio_mcu_device_mapping[mcu_device]],
			mcu_device, &open_msg, sizeof(struct ipcm_msg));
	}

	return g_audio_mcu_rpmsg[mcu]->txn_result;

}

int mtk_audio_mcu_rpmsg_init(AUD_MCU_RPMSG_RX_HANDLER_FP audio_mcu_rx_handler)
{
	int mcu = 0;

	for (mcu = 0; mcu < AUDIO_MCU_CLIENT_NUM_MAX; mcu++) {
		if (!g_audio_mcu_rpmsg[mcu])
			return -ENOMEM;

		g_audio_mcu_rpmsg[mcu]->rx_callback = audio_mcu_rx_handler;
	}
	return 0;

}

static int mtk_audio_mcu_rpmsg_callback(struct rpmsg_device *rpdev,
	void *data, int len, void *priv, u32 src)
{
	struct audio_mcu_device_config audio_mcu_device;
	struct ipcm_msg *msg;
	int mcu = 0;

	if (strncmp(rpdev->id.name, audio_mcu_ept[AUDIO_MCU_CLIENT_AUDEC],
		strlen(audio_mcu_ept[AUDIO_MCU_CLIENT_AUDEC])) == 0)
		mcu = AUDIO_MCU_CLIENT_AUDEC;
	else if (strncmp(rpdev->id.name, audio_mcu_ept[AUDIO_MCU_CLIENT_AUSND],
		strlen(audio_mcu_ept[AUDIO_MCU_CLIENT_AUSND])) == 0)
		mcu = AUDIO_MCU_CLIENT_AUSND;
	else
		dev_err(&rpdev->dev, "invalid device id %s\n", rpdev->id.name);

	if (!g_audio_mcu_rpmsg[mcu]) {
		audio_err("private data is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		audio_err("Invalid data or length from src:%d\n",
			src);
		return -EINVAL;
	}

	msg = (struct ipcm_msg *)data;

	audio_info("%s %s %d\n", __func__, rpdev->id.name, msg->cmd);
	// print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, DEBUG_GROUP_SIZE, 1, data, len, true);

	switch (msg->cmd) {
	case IPCM_CMD_OP:
	{
		audio_info("OP CMD\n");
		break;
	}
	case IPCM_CMD_ACK:
	{
		audio_info("Got OK ack, response for cmd %d\n", msg->data[0]);
		if (len > sizeof(struct ipcm_msg))
			audio_err("Receive data length over size\n");

		/* store received data for transaction checking */
		memcpy(&g_audio_mcu_rpmsg[mcu]->receive_msg, data,
				sizeof(struct ipcm_msg));
		g_audio_mcu_rpmsg[mcu]->txn_result = 0;
		mtk_audio_mcu_rpmsg_rx_ack_handler(g_audio_mcu_rpmsg[mcu], &audio_mcu_device, mcu);
		// complete(&g_audio_mcu_rpmsg[mcu]->ack);
		break;
	}
	case IPCM_CMD_BAD_ACK:
	{
		audio_err("Got BAD ack, result = %d\n", *(int *)&msg->data);

		complete(&g_audio_mcu_rpmsg[mcu]->ack[0]);//fix me
		break;
	}
	case IPCM_CMD_MSG:
	{
		audio_info("MSG CMD\n");
		if (len > sizeof(struct ipcm_msg))
			audio_err("Receive data length over size\n");

		/* store received data for transaction checking */
		memcpy(&g_audio_mcu_rpmsg[mcu]->receive_msg, data,
				sizeof(struct ipcm_msg));
		mtk_audio_mcu_rpmsg_rx_handler(g_audio_mcu_rpmsg[mcu], &audio_mcu_device, mcu);
		break;
	}
	default:
		audio_err("Invalid command %d from src:%d\n", msg->cmd, src);
		return -EINVAL;
	}
	return 0;
}

static int mtk_audio_mcu_rpmsg_probe(struct rpmsg_device *rpdev)
{
	int mcu = 0;
	int i = 0;

	for (mcu = 0; mcu < AUDIO_MCU_CLIENT_NUM_MAX; mcu++) {
		if (strncmp(rpdev->id.name, audio_mcu_ept[mcu],
			strlen(audio_mcu_ept[mcu])) == 0) {
			audio_err("%s/channel %d : %s ok", __func__, mcu, rpdev->id.name);

			if (!g_audio_mcu_rpmsg[mcu]) {
				g_audio_mcu_rpmsg[mcu] = devm_kzalloc(&rpdev->dev,
					sizeof(struct mtk_audio_mcu_rpmsg), GFP_KERNEL);
			}

			if (!g_audio_mcu_rpmsg[mcu])
				return -ENOMEM;

			g_audio_mcu_rpmsg[mcu]->rpdev = rpdev;
			g_audio_mcu_rpmsg[mcu]->dev = &rpdev->dev;
			// g_audio_mcu_rpmsg[mcu]->txn = mtk_audio_mcu_rpmsg_txn;
			for (i = 0; i < AUDIO_MCU_DEVICE_NUM_MAX; i++)
				init_completion(&g_audio_mcu_rpmsg[mcu]->ack[i]);

			// add to rpmsg_dev list
			INIT_LIST_HEAD(&g_audio_mcu_rpmsg[mcu]->list);
			list_add_tail(&g_audio_mcu_rpmsg[mcu]->list, &rpmsg_dev_list);

			dev_set_drvdata(&rpdev->dev, g_audio_mcu_rpmsg[mcu]);

			return 0;
		}
	}
	return 0;
}

static void mtk_audio_mcu_rpmsg_remove(struct rpmsg_device *rpdev)
{
	struct mtk_audio_mcu_rpmsg *audio_mcu_rpmsg = dev_get_drvdata(&rpdev->dev);

	list_del(&audio_mcu_rpmsg->list);
	dev_set_drvdata(&rpdev->dev, NULL);
	dev_info(&rpdev->dev, "Audio MCU rpmsg remove done\n");
}

static struct rpmsg_device_id mtk_audio_mcu_rpmsg_id_table[] = {
	{ .name	= AUDIO_MCU_DEC_EPT_NAME},
	{ .name	= AUDIO_MCU_SND_EPT_NAME},
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, mtk_audio_mcu_rpmsg_id_table);

static struct rpmsg_driver mtk_audio_mcu_rpmsg_driver = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= mtk_audio_mcu_rpmsg_id_table,
	.probe		= mtk_audio_mcu_rpmsg_probe,
	.callback	= mtk_audio_mcu_rpmsg_callback,
	.remove		= mtk_audio_mcu_rpmsg_remove,
};
module_rpmsg_driver(mtk_audio_mcu_rpmsg_driver);

MODULE_DESCRIPTION("ALSA Audio MCU communication driver");
MODULE_LICENSE("GPL v2");
