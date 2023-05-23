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
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <sound/soc.h>
#include "voc_common.h"
#include "voc_common_reg.h"
#include "voc_drv_rpmsg.h"
#include "vbdma.h"




//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define PTP_TIME_INDEX_0  0
#define PTP_TIME_INDEX_1  1
#define PTP_TIME_INDEX_2  2
#define PTP_TIME_INDEX_3  3
#define DUMP_COUNT_16     16
#define RPMSG_MAGIC_02    2
#define RPMSG_ELEM_ID_02  2
#define U32_BIT_NUM       32
#define RPMSG_TIMEOUT_MS  500

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static struct voc_communication_operater voc_rpmsg_op;
struct mtk_voc_rpmsg *g_voc_rpmsg;


unsigned long mtk_log_level = LOGLEVEL_ERR;
//unsigned long mtk_log_level = LOGLEVEL_INFO;
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------



enum ipcm_ack_t {
	IPCM_CMD_OP = 0,
	IPCM_CMD_BAD_ACK,
	IPCM_CMD_ACK,
	IPCM_CMD_MSG,
};

enum voice_cmd_t {
	VOICE_CMD_DATA_MOVING_ENABLE = 0,
	VOICE_CMD_DATA_MOVING_CONFIG,
	VOICE_CMD_DATA_MOVING_NOTIFY,
	VOICE_CMD_DATA_MOVING_STATUS,
	VOICE_CMD_WAKEUP_ENABLE,
	VOICE_CMD_WAKEUP_CONFIG,
	VOICE_CMD_PREPROCESS_ENABLE,
	VOICE_CMD_PREPROCESS_CONFIG,
	VOICE_CMD_SIGEN_ENABLE,
	VOICE_CMD_MIC_GAIN,
	VOICE_CMD_HPF_STAGE,
	VOICE_CMD_HPF_CONFIG,
	VOICE_CMD_RESET,
	VOICE_CMD_SOUND_MODEL_UPDATE,
	VOICE_CMD_HOTWORD_VERSION,
	VOICE_CMD_HOTWORD_DETECTED,
	VOICE_CMD_HOTWORD_IDENTIFIER,
	VOICE_CMD_SLT_TEST,
	VOICE_CMD_TIMESTAMP_NOTIFY,
	VOICE_CMD_SEAMLESS,
	VOICE_CMD_STATUS_UPDATE,
	VOICE_CMD_MIC_MUTE_ENABLE,
	VOICE_CMD_MIC_SWITCH,
	VOICE_CMD_MAX,
};

enum voice_wakeup_mode {
	VOICE_WAKEUP_NORMAL = 0,
	VOICE_WAKEUP_PM,
	VOICE_WAKEUP_BOTH,
	VOICE_WAKEUP_AUTO_TEST,
};

enum voice_ts_stage {
	VOICE_TS_OFF = 0,
	VOICE_TS_START,
	VOICE_TS_SYNC,
	VOICE_TS_FOLLOW_UP,
	VOICE_TS_DELAY_REQ,
	VOICE_TS_DELAY_RESP,
	VOICE_TS_MAX,
};

enum voice_data_moving_status {
	VOICE_DM_STOP = 0,
	VOICE_DM_START,
	VOICE_DM_MAX,
};

struct ipcm_msg {
	uint8_t cmd;
	uint32_t size;
	uint8_t data[IPCM_MAX_DATA_SIZE];
};


struct voc_no_data_res_pkt {
	uint8_t cmd;
};



struct data_moving_status {
	enum voice_data_moving_status status;
};

struct vp_config_s {
	int scale;
};

struct wakeup_enable {
	bool enable;
};

struct wakeup_config {
	enum voice_wakeup_mode mode;
};

struct sine_tone_enable {
	bool enable;
};

struct mic_gain {
	int16_t steps;
};

struct hpf_stage {
	int32_t hpf_stage;
};

struct hpf_config {
	int32_t hpf_coeff;
};

struct sound_model_config {
	uint32_t addr;
	uint32_t addr_H;
	uint32_t size;
	int32_t crc;
};

struct hotword_ver {
	uint32_t ver;
};

struct hotword_id {
	uint8_t id[VOICE_DSP_IDENTIFIER_SIZE];
	int32_t size;
};

struct slt_test {
	int32_t loop;
	uint32_t addr;
	int32_t status;
};

struct ts_config {
	uint32_t stage;
	uint64_t ts;
};

struct seamless_enable {
	bool enable;
};

struct notify_status {
	int32_t status;
};

struct mic_mute_enable {
	bool enable;
};

struct mic_switch_enable {
	bool enable;
};

/* The rpmsg bus device */
struct mtk_voc_rpmsg {
	struct device *dev;
	struct rpmsg_device *rpdev;
	int (*txn)(struct mtk_voc_rpmsg *voc_rpmsg, void *data,
			size_t len);
	int txn_result;
	int ack_status;
	struct ipcm_msg receive_msg;
	struct completion ack;
	struct mtk_snd_voc *voc;
	struct list_head list;
	struct ipcm_msg receive_vd;
};

/* A list of voc_soc_card_dev */
LIST_HEAD(voc_card_dev_list);
EXPORT_SYMBOL(voc_card_dev_list);
/* A list of rpmsg_dev */
static LIST_HEAD(rpmsg_dev_list);
/* Lock to serialise RPMSG device and PWM device binding */
static DEFINE_MUTEX(rpmsg_voc_bind_lock);
static DEFINE_MUTEX(rpmsg_sending_lock);

//unsigned int voc_machine_get_mic_num(struct mtk_voc_rpmsg *voc_rpmsg)
//{
//  /* sanity check */
//  if (!voc_rpmsg) {
//      voc_dev_err(NULL, "rpmsg dev is NULL\n");
//      return -EINVAL;
//  }
//
//  voc_dev_dbg(voc_rpmsg->dev, "%s card_reg %d\n", __func__
//  , voc_rpmsg->voc->voc_card->card_reg[3]);
//
//  return voc_rpmsg->voc->voc_card->card_reg[3];
//}
//EXPORT_SYMBOL_GPL(voc_machine_get_mic_num);

//unsigned int voc_machine_get_mic_bitwidth(struct mtk_voc_rpmsg *voc_rpmsg)
//{
//  /* sanity check */
//  if (!voc_rpmsg) {
//      voc_dev_err(NULL, "rpmsg dev is NULL\n");
//      return -EINVAL;
//  }
//
//  voc_dev_dbg(voc_rpmsg->dev, "%s card_reg %d\n", __func__
//  , voc_rpmsg->voc->voc_card->card_reg[5]);
//
//  switch (voc_rpmsg->voc->voc_card->card_reg[5]) {
//  case 0://E_MBX_AUD_BITWIDTH_16
//      return 16;
//  case 1://E_MBX_AUD_BITWIDTH_24
//      return 24;
//  case 2://E_MBX_AUD_BITWIDTH_32
//      return 32;
//  default:
//      return 16;
//  }
//}
//EXPORT_SYMBOL_GPL(voc_machine_get_mic_bitwidth);


/*
 * VOC chip device and rpmsg device binder
 */
int _mtk_bind_rpmsg_voc_card(struct mtk_voc_rpmsg *rpmsg_voc_dev)
{
	mutex_lock(&rpmsg_voc_bind_lock);

	if (rpmsg_voc_dev) {
		struct mtk_snd_voc *voc, *n;

		// check voc card device list,
		// link 1st voc card dev which not link to rpmsg dev
		list_for_each_entry_safe(voc, n, &voc_card_dev_list,
					 list) {
			if (rpmsg_voc_dev->voc == NULL) {
				rpmsg_voc_dev->voc = voc;
				complete(&voc->rpmsg_bind);
				break;
			}
		}
	}

	mutex_unlock(&rpmsg_voc_bind_lock);

	return 0;
}


int voc_link_rpmsg_bind(struct mtk_snd_voc *snd_voc)
{
	mutex_lock(&rpmsg_voc_bind_lock);

	if (snd_voc) {
		struct mtk_voc_rpmsg *rpmsg_dev, *n;

		// check rpmsg device list,
		// link 1st rpmsg dev which not link to voc soc card dev
		list_for_each_entry_safe(rpmsg_dev, n, &rpmsg_dev_list, list) {
			if (rpmsg_dev->voc == NULL) {
				rpmsg_dev->voc = snd_voc;
				complete(&snd_voc->rpmsg_bind);
				break;
			}
		}
	}

	mutex_unlock(&rpmsg_voc_bind_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(voc_link_rpmsg_bind);
/*
 * RPMSG functions
 */
static int mtk_voc_txn(struct mtk_voc_rpmsg *voc_rpmsg, void *data,
			   size_t len)
{
	int ret;

	/* sanity check */
	if (!voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		voc_dev_err(voc_rpmsg->dev, "Invalid data or length\n");
		return -EINVAL;
	}
#ifdef DEBUG
	pr_info("Send:\n");
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, DUMP_COUNT_16, 1, data, len,
			   true);
#endif

	mutex_lock(&rpmsg_sending_lock);

	voc_rpmsg->ack_status = -EIO;

	if (completion_done(&voc_rpmsg->ack))
		reinit_completion(&voc_rpmsg->ack);

	ret = rpmsg_send(voc_rpmsg->rpdev->ept, data, len);
	if (ret) {
		voc_dev_err(voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		mutex_unlock(&rpmsg_sending_lock);
		return ret;
	}
	// wait for ack
	//wait_for_completion_interruptible(&voc_rpmsg->ack);
	ret = wait_for_completion_timeout(&voc_rpmsg->ack,
						msecs_to_jiffies(RPMSG_TIMEOUT_MS));
	//timeout or -ERESTARTSYS
	if (ret <= 0) {
		voc_dev_err(voc_rpmsg->dev,
					"[%s]wait for completion timeout\n", __func__);
	}
	// return ack sattus
	voc_rpmsg->txn_result = voc_rpmsg->ack_status;

	mutex_unlock(&rpmsg_sending_lock);
	return voc_rpmsg->txn_result;
}

/*
 * Common procedures for communicate linux subsystem and rpmsg
 */
static inline int __mtk_rpmsg_txn_sync(struct mtk_voc_rpmsg *voc_rpmsg,
				 uint8_t cmd)
{
struct voc_no_data_res_pkt *res_pk =
		(struct voc_no_data_res_pkt *)&voc_rpmsg->receive_msg.data;


	if (res_pk->cmd != cmd) {
		voc_dev_err(voc_rpmsg->dev,
			"Out of sync command, send %d get %d\n", cmd,
			res_pk->cmd);
		return -ENOTSYNC;
	}

	return 0;
}



static inline int __mtk_rpmsg_rx_handler(struct mtk_voc_rpmsg *voc_rpmsg)
{
	struct voice_packet *voice_msg = NULL;
	int ret = 0;

	/* sanity check */
	if (!voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg is NULL\n");
		return -EINVAL;
	}

	if (!voc_rpmsg->dev) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}
	voice_msg = (struct voice_packet *)&voc_rpmsg->receive_msg.data;

	voc_dev_dbg(voc_rpmsg->dev,
			"MSG_PERIOD_NOTIFY, get %d\n", voice_msg->cmd);

	switch (voice_msg->cmd) {
	case VOICE_CMD_DATA_MOVING_NOTIFY: {
		if (voc_rpmsg->voc->voc_pcm_running) {
			memcpy(&voc_rpmsg->receive_vd, &voc_rpmsg->receive_msg,
				sizeof(struct ipcm_msg));
			voc_rpmsg->voc->vd_notify = 1;
			wake_up_interruptible(&voc_rpmsg->voc->kthread_waitq);
		}

		break;
	}
	case VOICE_CMD_DATA_MOVING_STATUS: {
		enum voice_data_moving_status dm_status;

		/* sanity check */
		if (!voc_rpmsg->voc) {
			voc_dev_err(NULL, "voc_rpmsg->voc is NULL\n");
			return -EINVAL;
		}

		dm_status = ((struct data_moving_status *)voice_msg->payload)->status;
		voc_dev_dbg(voc_rpmsg->dev, "VOICE_CMD_DATA_MOVING_STATUS:%d\n", dm_status);
		voc_rpmsg->voc->voc_pcm_status = dm_status;
		break;
	}
	case VOICE_CMD_HOTWORD_VERSION: {
		uint32_t ver;

		/* sanity check */
		if ((!voc_rpmsg->voc) || (!voc_rpmsg->voc->card)) {
			voc_dev_err(NULL, "voc_rpmsg->voc or voc_rpmsg->voc->card is NULL\n");
			return -EINVAL;
		}
		ver =
			((struct hotword_ver *)voice_msg->payload)->ver;
		voc_dev_dbg(voc_rpmsg->dev, "hotword version %d\n", ver);
		voc_rpmsg->voc->voc_card->card_reg[VOC_REG_HOTWORD_VER] = ver;
		ret = voc_rpmsg->voc->voc_card->card_reg[VOC_REG_HOTWORD_VER];
		break;
	}
	case VOICE_CMD_HOTWORD_IDENTIFIER: {
		uint8_t *identifier = ((struct hotword_id *)voice_msg->payload)->id;
		int32_t size = ((struct hotword_id *)voice_msg->payload)->size;
		int32_t array_size = 0;

		/* sanity check */
		if ((!voc_rpmsg->voc) || (!voc_rpmsg->voc->card)) {
			voc_dev_err(NULL, "voc_rpmsg->voc or voc_rpmsg->voc->card is NULL\n");
			return -EINVAL;
		}
		array_size = voc_rpmsg->voc->voc_card->uuid_size;

		voc_dev_dbg(voc_rpmsg->dev, "hotword id=[%s], size=%d, array size=%d\n",
			identifier, size, array_size);
		if (!voc_rpmsg->voc->voc_card->uuid_array) {
			voc_dev_err(voc_rpmsg->dev, "Invalid uuid array\n");
			return -EINVAL;
		}

		if (size <= array_size) {
			memcpy(voc_rpmsg->voc->voc_card->uuid_array, identifier, size);
		} else {
			voc_dev_warn(voc_rpmsg->dev, "hotword id size overflow.(%d>%d)",
				size, array_size);
			memcpy(voc_rpmsg->voc->voc_card->uuid_array,
				identifier, array_size);
		}
		break;
	}
	case VOICE_CMD_HOTWORD_DETECTED: {
		struct snd_kcontrol *kctl = NULL;
		struct snd_ctl_elem_id elem_id;
		bool enable = ((struct wakeup_enable *)voice_msg->payload)->enable;

		/* sanity check */
		if ((!voc_rpmsg->voc) || (!voc_rpmsg->voc->card)) {
			voc_dev_err(NULL, "voc_rpmsg->voc or voc_rpmsg->voc->card is NULL\n");
			return -EINVAL;
		}

		if (enable) {
			memset(&elem_id, 0, sizeof(elem_id));
			elem_id.numid = RPMSG_ELEM_ID_02;
			kctl = snd_ctl_find_id(voc_rpmsg->voc->card, &elem_id);
			if (kctl != NULL) {
				snd_ctl_notify(voc_rpmsg->voc->card,
				SNDRV_CTL_EVENT_MASK_VALUE, &kctl->id);
			}
		}
		break;
	}
	case VOICE_CMD_SLT_TEST: {
		int32_t status;

		status = ((struct slt_test *)voice_msg->payload)->status;
		voc_rpmsg->voc->voc_card->status = status;
		break;
	}
	case VOICE_CMD_TIMESTAMP_NOTIFY: {
		uint32_t stage = ((struct ts_config *)voice_msg->payload)->stage;
		uint64_t ts = ((struct ts_config *)voice_msg->payload)->ts;
		struct timespec64 current_time;

		voc_dev_dbg(voc_rpmsg->dev, "TS_NOTIFY Stage: %d, TS: %llu\n", stage, ts);
		switch (stage) {
		case VOICE_TS_SYNC: {
			ktime_get_ts64(&current_time);
			voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_0] = ts*1000000LL;//ms -> ns
			voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_1] =
				(current_time.tv_sec*1000000000LL + current_time.tv_nsec);

			voc_dev_dbg(voc_rpmsg->dev,
				"TS: %llu, Time = [(%lld)nsec, (%lld)sec, (%ld)nsec]\n",
				ts, voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_1],
				current_time.tv_sec, current_time.tv_nsec);

			break;
		}
		case VOICE_TS_FOLLOW_UP: {
			//Receive Follow-up message,
			//get time before send delay_request message.
			ktime_get_ts64(&current_time);
			voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_2] =
				(current_time.tv_sec*1000000000LL + current_time.tv_nsec);

			voc_dev_dbg(voc_rpmsg->dev,
				"TS: %llu, Time = [(%lld)nsec, (%lld)sec, (%ld)nsec]\n",
				ts, voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_2],
				current_time.tv_sec, current_time.tv_nsec);

			voc_ts_delay_req();
			voc_dev_dbg(voc_rpmsg->dev, "TIMESTAMP_NOTIFY follow-up send delay request\n");
			break;
		}
		case VOICE_TS_DELAY_RESP: {
			int64_t diffpc = 0;
			int64_t diffcp = 0;

			//Receive Delay response message, calculate delta time.
			voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_3] = ts*1000000LL;//ms->ns
			voc_dev_dbg(voc_rpmsg->dev, "TS: %llu\n", ts);

			//Delta time is average time of ([PMU->CPU] - [CPU->PMU]) / 2
			diffpc = (voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_1]
					- voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_0]);
			diffcp = (voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_3]
					- voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_2]);
			voc_rpmsg->voc->sync_time = (diffpc - diffcp) / RPMSG_MAGIC_02;

			voc_dev_dbg(voc_rpmsg->dev,
				"Sync time = %lld, (PMU->CPU):%lld, (CPU->PMU):%lld\n",
				voc_rpmsg->voc->sync_time, diffpc, diffcp);

			voc_dev_dbg(voc_rpmsg->dev,
				"PTP time = [t0=%lld, t1=%lld, t2=%lld, t3=%lld]\n",
				voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_0],
				voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_1],
				voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_2],
				voc_rpmsg->voc->ptp_time[PTP_TIME_INDEX_3]);
			break;
		}
		case VOICE_TS_START:
		case VOICE_TS_DELAY_REQ:
		default:
			voc_dev_err(voc_rpmsg->dev, "Invallid argument stage: %d\n", stage);
			break;

		}
		break;
	}
	default:
		return ret;
	}

	return ret;
}



//CM4 notification interval is based on the fixed frame size(16kHz,16bits),
//and it's irrelevant to bitwidth and sample rate.
//So we should update notification frequency
//in case the notification is frequent.
int voc_dma_init_channel(
		uint64_t dma_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t sample_width,
		uint32_t sample_rate)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (!g_voc_rpmsg->voc) {
		voc_dev_err(NULL, "voc is NULL\n");
		return -EINVAL;
	}

	voc_dev_info(g_voc_rpmsg->dev, "%s\n", __func__);
	voc_dev_info(g_voc_rpmsg->dev,
			"dma_paddr=%llx, buf_size=%x\n", dma_paddr, buf_size);

	voc_dev_info(g_voc_rpmsg->dev,
			"channels=%d, sample_width=%d, sample_rate=%d\n"
			, channels, sample_width, sample_rate);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_DATA_MOVING_CONFIG;

	((struct data_moving_config *)voice_msg->payload)->addr = (uint32_t)dma_paddr;
	((struct data_moving_config *)voice_msg->payload)->addr_H =
	 (uint32_t)(dma_paddr >> U32_BIT_NUM);
	((struct data_moving_config *)voice_msg->payload)->size = buf_size;
	((struct data_moving_config *)voice_msg->payload)->mic_numbers = channels;
	((struct data_moving_config *)voice_msg->payload)->bitwidth = sample_width;
	((struct data_moving_config *)voice_msg->payload)->sample_rate = sample_rate;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_init_channel);

int voc_dma_start_channel(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_DATA_MOVING_ENABLE;
	((struct data_moving_enable *)voice_msg->payload)->enable = 1;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	//ret = rpmsg_send(voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret) {
		voc_dev_err(g_voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}


	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_start_channel);

int voc_dma_stop_channel(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_DATA_MOVING_ENABLE;
	((struct data_moving_enable *)voice_msg->payload)->enable = 0;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	//ret = rpmsg_send(voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));

	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_stop_channel);

int voc_dma_enable_sinegen(bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_SIGEN_ENABLE;
	((struct sine_tone_enable *)voice_msg->payload)->enable = en;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_enable_sinegen);

int voc_enable_vq(bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_WAKEUP_ENABLE;
	((struct wakeup_enable *)voice_msg->payload)->enable = en;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_vq);

int voc_config_vq(uint8_t mode)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_WAKEUP_CONFIG;

	if (mode == VOICE_WAKEUP_PM)
		((struct wakeup_config *)voice_msg->payload)->mode = VOICE_WAKEUP_PM;
	else if (mode == VOICE_WAKEUP_BOTH)
		((struct wakeup_config *)voice_msg->payload)->mode = VOICE_WAKEUP_BOTH;
	else if (mode == VOICE_WAKEUP_AUTO_TEST)
		((struct wakeup_config *)voice_msg->payload)->mode = VOICE_WAKEUP_AUTO_TEST;
	else
		((struct wakeup_config *)voice_msg->payload)->mode = VOICE_WAKEUP_NORMAL;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_config_vq);

int voc_enable_hpf(int32_t stage)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_HPF_STAGE;
	((struct hpf_stage *)voice_msg->payload)->hpf_stage = stage;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_hpf);

int voc_config_hpf(int32_t coeff)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_HPF_CONFIG;
	((struct hpf_config *)voice_msg->payload)->hpf_coeff = coeff;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_config_hpf);

//int voc_enable_vp(struct mtk_voc_rpmsg *voc_rpmsg, bool en)
//{
//  int ret = 0;
//  struct voice_packet *voice_msg;
//  struct ipcm_msg open_msg = {
//      .cmd = IPCM_CMD_OP,
//      .size = 0,
//      .data = {0},
//  };
//
//  /* sanity check */
//  if (!voc_rpmsg) {
//      voc_dev_err(NULL, "rpmsg dev is NULL\n");
//      return -EINVAL;
//  }
//
//  voc_dev_dbg(voc_rpmsg->dev, "%s\n", __func__);
//
//  open_msg.size = sizeof(struct voice_packet);
//  voice_msg = (struct voice_packet *)&open_msg.data;
//  voice_msg->cmd = VOICE_CMD_PREPROCESS_ENABLE;
//  voice_msg->payload[0] = en;
//
//  /* execute RPMSG command */
//  ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
//  if (ret)
//      return ret;
//
//  // check received result
////    return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);
//
//  return voc_rpmsg->txn_result;
//}
//EXPORT_SYMBOL_GPL(voc_enable_vp);

//int voc_config_vp(struct mtk_voc_rpmsg *voc_rpmsg, struct vp_config_s config)
//{
//  int ret = 0;
//  struct voice_packet *voice_msg;
//  struct ipcm_msg open_msg = {
//      .cmd = IPCM_CMD_OP,
//      .size = 0,
//      .data = {0},
//  };
//
//  /* sanity check */
//  if (!voc_rpmsg) {
//      voc_dev_err(NULL, "rpmsg dev is NULL\n");
//      return -EINVAL;
//  }
//
//  voc_dev_dbg(voc_rpmsg->dev, "%s\n", __func__);
//
//  open_msg.size = sizeof(struct voice_packet);
//  voice_msg = (struct voice_packet *)&open_msg.data;
//  voice_msg->cmd = VOICE_CMD_PREPROCESS_CONFIG;
//  voice_msg->payload[0] = config.scale;
//
//  /* execute RPMSG command */
//  ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
//  if (ret)
//      return ret;
//
//  // check received result
////    return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);
//
//  return voc_rpmsg->txn_result;
//}
//EXPORT_SYMBOL_GPL(voc_config_vp);

int voc_dmic_gain(int16_t gain)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_MIC_GAIN;
	((struct mic_gain *)voice_msg->payload)->steps = gain;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dmic_gain);

int voc_dmic_mute(bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_MIC_MUTE_ENABLE;
	((struct mic_mute_enable *)voice_msg->payload)->enable = en;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//    return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
};
EXPORT_SYMBOL_GPL(voc_dmic_mute);

// TODO: rename voc_dmic_switch()
//       to distinguish voc_dmic_switch() from voc_dmic_mute()
int voc_dmic_switch(bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_MIC_SWITCH;
	((struct mic_switch_enable *)voice_msg->payload)->enable = en;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//    return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
};
EXPORT_SYMBOL_GPL(voc_dmic_switch);


int voc_reset_voice(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_RESET;

	/* execute RPMSG command */
	//ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	ret = rpmsg_send(g_voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret) {
		voc_dev_err(g_voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_reset_voice);

int voc_update_snd_model(
				dma_addr_t snd_model_paddr,
				uint32_t size)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	uint32_t src_addr_H;

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	src_addr_H = (uint32_t)(snd_model_paddr >> U32_BIT_NUM);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_SOUND_MODEL_UPDATE;
	((struct sound_model_config *)voice_msg->payload)->addr = (uint32_t)snd_model_paddr;
	((struct sound_model_config *)voice_msg->payload)->addr_H = src_addr_H;
	((struct sound_model_config *)voice_msg->payload)->size = size;
	((struct sound_model_config *)voice_msg->payload)->crc = 0;//calculated by CM4

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_update_snd_model);

int voc_get_hotword_ver(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_HOTWORD_VERSION;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_get_hotword_ver);

int voc_get_hotword_identifier(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_HOTWORD_IDENTIFIER;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_get_hotword_identifier);


int voc_enable_slt_test(int32_t loop, uint32_t addr)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);
	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_SLT_TEST;
	((struct slt_test *)voice_msg->payload)->loop = loop;
	((struct slt_test *)voice_msg->payload)->addr = addr;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_slt_test);

int voc_ts_sync_start(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);
	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_TIMESTAMP_NOTIFY;

	((struct ts_config *)voice_msg->payload)->stage = (uint32_t)VOICE_TS_START;
	((struct ts_config *)voice_msg->payload)->ts = 0;

	/* execute RPMSG command */
	//ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	ret = rpmsg_send(g_voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret) {
		voc_dev_err(g_voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_ts_sync_start);

int voc_ts_delay_req(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);
	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_TIMESTAMP_NOTIFY;

	((struct ts_config *)voice_msg->payload)->stage = (uint32_t)VOICE_TS_DELAY_REQ;
	((struct ts_config *)voice_msg->payload)->ts = 0;

	/* execute RPMSG command */
	//ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	ret = rpmsg_send(g_voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret) {
		voc_dev_err(g_voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}

	// check received result
//  return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}

int voc_enable_seamless(bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_SEAMLESS;
	((struct seamless_enable *)voice_msg->payload)->enable = en;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	//ret = rpmsg_send(voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
	// return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_seamless);

int voc_notify_status(int32_t status)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_STATUS_UPDATE;
	((struct notify_status *)voice_msg->payload)->status = status;

	/* execute RPMSG command */
	//ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	ret = rpmsg_send(g_voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret) {
		voc_dev_err(g_voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}

	// check received result
	// return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_notify_status);

int voc_dma_get_reset_status(void)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_rpmsg) {
		voc_dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_DATA_MOVING_STATUS;

	/* execute RPMSG command */
	ret = g_voc_rpmsg->txn(g_voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	return g_voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_get_reset_status);


void *voc_drv_rpmsg_get_voice_packet_addr(void)
{
	return (void *)&g_voc_rpmsg->receive_vd.data;
}
EXPORT_SYMBOL_GPL(voc_drv_rpmsg_get_voice_packet_addr);

static int mtk_voc_rpmsg_callback(struct rpmsg_device *rpdev,
				void *data, int len, void *priv, u32 src)
{
	struct mtk_voc_rpmsg *voc_rpmsg;
	struct ipcm_msg *msg;

	voc_rpmsg = dev_get_drvdata(&rpdev->dev);
	if (!voc_rpmsg) {
		voc_dev_err(&rpdev->dev, "private data is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		voc_dev_err(&rpdev->dev, "Invalid data or length from src:%d\n",
			src);
		return -EINVAL;
	}

	msg = (struct ipcm_msg *)data;

#ifdef DEBUG
	pr_info("%s:\n", __func__);
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, DUMP_COUNT_16, 1, data, len,
			true);
#endif

	switch (msg->cmd) {
	case IPCM_CMD_OP:
		voc_dev_dbg(&rpdev->dev, "OP CMD\n");
		break;
	case IPCM_CMD_ACK:
		voc_dev_dbg(&rpdev->dev, "Got OK ack, response for cmd %d\n",
			msg->data[0]);
		if (len > sizeof(struct ipcm_msg))
			voc_dev_err(&rpdev->dev, "Receive data length over size\n");

		/* store received data for transaction checking */
		memcpy(&voc_rpmsg->receive_msg, data,
				sizeof(struct ipcm_msg));
		voc_rpmsg->ack_status = 0;
		complete(&voc_rpmsg->ack);
		break;
	case IPCM_CMD_BAD_ACK:
		voc_dev_err(&rpdev->dev, "Got BAD ack, result = %d\n",
			*(int *)&msg->data);
		complete(&voc_rpmsg->ack);
		break;
	case IPCM_CMD_MSG:
		voc_dev_dbg(&rpdev->dev, "IPCM_CMD_MSG\n");
		if (len > sizeof(struct ipcm_msg))
			voc_dev_err(&rpdev->dev, "Receive data length over size\n");
		/* store received data for transaction checking */
		memcpy(&voc_rpmsg->receive_msg, data,
				sizeof(struct ipcm_msg));
		__mtk_rpmsg_rx_handler(voc_rpmsg);
		break;
	default:
		voc_dev_err(&rpdev->dev, "Invalid command %d from src:%d\n",
			msg->cmd, src);
		return -EINVAL;
	}
	return 0;
}

static int mtk_voc_rpmsg_probe(struct rpmsg_device *rpdev)
{
	int ret;
	struct mtk_voc_rpmsg *voc_rpmsg;

	voc_dev_info(&rpdev->dev, "[%s]\n", __func__);

	voc_rpmsg = devm_kzalloc(&rpdev->dev,
					sizeof(struct mtk_voc_rpmsg),
					GFP_KERNEL);
	if (!voc_rpmsg)
		return -ENOMEM;

	// assign the member
	voc_rpmsg->rpdev = rpdev;
	voc_rpmsg->dev = &rpdev->dev;
	init_completion(&voc_rpmsg->ack);
	voc_rpmsg->txn = mtk_voc_txn;
	g_voc_rpmsg = voc_rpmsg;
	// add to rpmsg_dev list
	INIT_LIST_HEAD(&voc_rpmsg->list);
	list_add_tail(&voc_rpmsg->list, &rpmsg_dev_list);

	// check if there is a valid voc_card_dev
	ret = _mtk_bind_rpmsg_voc_card(voc_rpmsg);
	if (ret) {
		voc_dev_err(&rpdev->dev,
			"binding rpmsg-voc_soc_card failed! (%d)\n", ret);
		goto bind_fail;
	}

	dev_set_drvdata(&rpdev->dev, voc_rpmsg);

	return 0;

bind_fail:
	list_del(&voc_rpmsg->list);

	return ret;
}


static void mtk_voc_rpmsg_remove(struct rpmsg_device *rpdev)
{
	struct mtk_voc_rpmsg *voc_rpmsg  = dev_get_drvdata(&rpdev->dev);

	list_del(&voc_rpmsg->list);
	dev_set_drvdata(&rpdev->dev, NULL);
	voc_dev_info(&rpdev->dev, "voice rpmsg remove done\n");
}

static struct rpmsg_device_id mtk_voc_rpmsg_driver_sample_id_table[] = {
	{ .name = "voice-pmu-ept"},
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, mtk_voc_rpmsg_driver_sample_id_table);

static struct rpmsg_driver mtk_voc_rpmsg_driver = {
	.drv.name   = KBUILD_MODNAME,
	.id_table   = mtk_voc_rpmsg_driver_sample_id_table,
	.probe      = mtk_voc_rpmsg_probe,
	.callback   = mtk_voc_rpmsg_callback,
	.remove = mtk_voc_rpmsg_remove,
};
module_rpmsg_driver(mtk_voc_rpmsg_driver);

struct voc_communication_operater *voc_drv_rpmsg_get_op(void)
{
	return &voc_rpmsg_op;
}
EXPORT_SYMBOL_GPL(voc_drv_rpmsg_get_op);

void voc_drv_rpmsg_init(void)
{

}
EXPORT_SYMBOL_GPL(voc_drv_rpmsg_init);

static struct voc_communication_operater voc_rpmsg_op = {
	.bind = voc_link_rpmsg_bind,
	.dma_start_channel = voc_dma_start_channel,
	.dma_stop_channel = voc_dma_stop_channel,
	.dma_init_channel = voc_dma_init_channel,
	.dma_enable_sinegen =  voc_dma_enable_sinegen,
	.dma_get_reset_status = voc_dma_get_reset_status,
	.enable_vq = voc_enable_vq,
	.config_vq = voc_config_vq,
	.enable_hpf = voc_enable_hpf,
	.config_hpf = voc_config_hpf,
	.dmic_gain = voc_dmic_gain,
	.dmic_mute = voc_dmic_mute,
	.dmic_switch = voc_dmic_switch,
	.reset_voice = voc_reset_voice,
	.update_snd_model = voc_update_snd_model,
	.get_hotword_ver = voc_get_hotword_ver,
	.get_hotword_identifier = voc_get_hotword_identifier,
	.enable_slt_test = voc_enable_slt_test,
	.enable_seamless = voc_enable_seamless,
	.notify_status = voc_notify_status,
	.power_status = 0,
	.ts_sync_start = voc_ts_sync_start,
	.get_voice_packet_addr = voc_drv_rpmsg_get_voice_packet_addr,
};


MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
