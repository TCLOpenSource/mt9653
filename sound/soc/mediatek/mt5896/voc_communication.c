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
#include <sound/soc.h>
#include "voc_communication.h"
#include "vbdma.h"

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------
#define VOICE_REQUEST_TIMEOUT (5 * HZ)
#define FULL_THRES 3 //CM4_PERIOD
#define MAX_CM4_PERIOD (1ULL<<32)
#define MIU_BASE	0x20000000
#define TLV_HEADER_SIZE             8
#define IPCM_MAX_DATA_SIZE	64
#define VOICE_MAX_DATA_SIZE	32
#define DMA_EMPTY		0
#define DMA_UNDERRUN	1
#define DMA_OVERRUN		2
#define DMA_FULL		3
#define DMA_NORMAL		4
#define DMA_LOCALFULL	5

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
	VOICE_CMD_WAKEUP_ENABLE,
	VOICE_CMD_WAKEUP_CONFIG,
	VOICE_CMD_PREPROCESS_ENABLE,
	VOICE_CMD_PREPROCESS_CONFIG,
	VOICE_CMD_SIGEN_ENABLE,
	VOICE_CMD_MIC_CONFIG,
	VOICE_CMD_MIC_GAIN,
	VOICE_CMD_HPF_STAGE,
	VOICE_CMD_HPF_CONFIG,
	VOICE_CMD_RESET,
	VOICE_CMD_SOUND_MODEL_UPDATE,
	VOICE_CMD_HOTWORD_VERSION,
	VOICE_CMD_HOTWORD_DETECTED,
	VOICE_CMD_SLT_TEST,
	VOICE_CMD_MAX,
};

enum voice_wakeup_mode {
	VOICE_WAKEUP_OFF = 0,
	VOICE_WAKEUP_PM,
	VOICE_WAKEUP_AUTO_TEST,
};

struct ipcm_msg {
	uint8_t cmd;
	uint32_t size;
	uint8_t data[IPCM_MAX_DATA_SIZE];
};

struct voice_packet  {
	uint8_t cmd;
	uint8_t payload[VOICE_MAX_DATA_SIZE];
};

struct voc_no_data_res_pkt {
	uint8_t cmd;
};

struct data_moving_config {
	uint32_t addr;
	uint32_t size;
	uint8_t mic_numbers;
	uint32_t sample_rate;
	uint8_t bitwidth;
};

struct data_moving_enable {
	bool enable;
};

struct data_moving_notify {
	uint32_t period_count;
	uint32_t byte_count;
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

struct mic_config {
	uint8_t mic_numbers;
	uint32_t sample_rate;
	uint8_t bitwidth;
};

struct mic_gain {
	uint16_t steps;
};

struct hpf_stage {
	int32_t hpf_stage;
};

struct hpf_config {
	int32_t hpf_coeff;
};

struct sound_model_config {
	uint32_t addr;
	uint32_t size;
	int32_t crc;
};

struct hotword_ver {
	uint32_t ver;
};

struct slt_test {
	int32_t loop;
	uint32_t addr;
	int32_t status;
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
	struct voc_soc_card_drvdata *voc_card;
	struct voc_pcm_data *voc_pcm;
	struct list_head list;
	struct task_struct *vd_task;
	bool vd_notify;
	wait_queue_head_t kthread_waitq;
};

struct voc_pcm_dma_data {
	unsigned char *name;	/* stream identifier */
	unsigned long channel;	/* Channel ID */
};

struct voc_pcm_runtime_data {
	spinlock_t            lock;
	unsigned int state;
	size_t dma_level_count;
	size_t int_level_count;
	struct voc_pcm_dma_data *dma_data;
	void *private_data;
};

/* A list of voc_soc_card_dev */
LIST_HEAD(voc_soc_card_dev_list);
EXPORT_SYMBOL(voc_soc_card_dev_list);
/* A list of voc_pcm_dev */
LIST_HEAD(voc_pcm_dev_list);
EXPORT_SYMBOL(voc_pcm_dev_list);
/* A list of rpmsg_dev */
static LIST_HEAD(rpmsg_dev_list);
/* Lock to serialise RPMSG device and PWM device binding */
static DEFINE_MUTEX(rpmsg_voc_bind_lock);

unsigned int voc_machine_get_mic_num(struct mtk_voc_rpmsg *voc_rpmsg)
{
	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s card_reg %d\n", __func__
	, voc_rpmsg->voc_card->card_reg[3]);

	return voc_rpmsg->voc_card->card_reg[3];
}
EXPORT_SYMBOL_GPL(voc_machine_get_mic_num);

unsigned int voc_machine_get_mic_bitwidth(struct mtk_voc_rpmsg *voc_rpmsg)
{
	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s card_reg %d\n", __func__
	, voc_rpmsg->voc_card->card_reg[5]);

	switch (voc_rpmsg->voc_card->card_reg[5]) {
	case 0://E_MBX_AUD_BITWIDTH_16
		return 16;
	case 1://E_MBX_AUD_BITWIDTH_24
		return 24;
	case 2://E_MBX_AUD_BITWIDTH_32
		return 32;
	default:
		return 16;
	}
}
EXPORT_SYMBOL_GPL(voc_machine_get_mic_bitwidth);

bool voc_dma_data_ready(struct mtk_voc_rpmsg *voc_rpmsg)
{
	struct voc_pcm_data *pcm_data = NULL;

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	pcm_data = voc_rpmsg->voc_pcm;

	return (pcm_data->pcm_level_cnt >= pcm_data->pcm_period_size?true:false);
}

bool voc_dma_xrun(struct mtk_voc_rpmsg *voc_rpmsg)
{
	struct voc_pcm_data *pcm_data = NULL;

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	pcm_data = voc_rpmsg->voc_pcm;

	return (pcm_data->pcm_status == PCM_XRUN?true:false);
}

/*
 * VOC chip device and rpmsg device binder
 */
int _mtk_bind_rpmsg_voc_soc_card(struct mtk_voc_rpmsg *rpmsg_voc_dev,
				struct voc_soc_card_drvdata *voc_soc_card_dev)
{
	mutex_lock(&rpmsg_voc_bind_lock);

	if (rpmsg_voc_dev) {
		struct voc_soc_card_drvdata *card_dev, *n;

		// check voc soc card device list,
		// link 1st voc soc card dev which not link to rpmsg dev
		list_for_each_entry_safe(card_dev, n, &voc_soc_card_dev_list,
					 list) {
			if (card_dev->rpmsg_dev == NULL) {
				card_dev->rpmsg_dev = rpmsg_voc_dev;
				rpmsg_voc_dev->voc_card = card_dev;
				complete(&card_dev->rpmsg_bind);
				break;
			}
		}
	} else {
		struct mtk_voc_rpmsg *rpmsg_dev, *n;

		// check rpmsg device list,
		// link 1st rpmsg dev which not link to voc soc card dev
		list_for_each_entry_safe(rpmsg_dev, n, &rpmsg_dev_list, list) {
			if (rpmsg_dev->voc_card == NULL) {
				rpmsg_dev->voc_card = voc_soc_card_dev;
				voc_soc_card_dev->rpmsg_dev = rpmsg_dev;
				complete(&voc_soc_card_dev->rpmsg_bind);
				break;
			}
		}
	}

	mutex_unlock(&rpmsg_voc_bind_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(_mtk_bind_rpmsg_voc_soc_card);

int _mtk_bind_rpmsg_voc_pcm(struct mtk_voc_rpmsg *rpmsg_voc_dev,
				struct voc_pcm_data *pcm_data_dev)
{
	mutex_lock(&rpmsg_voc_bind_lock);

	if (rpmsg_voc_dev) {
		struct voc_pcm_data *pcm_dev, *n;

		// check voc pcm device list,
		// link 2nd voc pcm dev which not link to rpmsg dev
		list_for_each_entry_safe(pcm_dev, n, &voc_pcm_dev_list,
					 list) {
			if (pcm_dev->rpmsg_dev == NULL) {
				pcm_dev->rpmsg_dev = rpmsg_voc_dev;
				rpmsg_voc_dev->voc_pcm = pcm_dev;
				complete(&pcm_dev->rpmsg_bind);
				break;
			}
		}
	} else {
		struct mtk_voc_rpmsg *rpmsg_dev, *n;

		// check rpmsg device list,
		// link 2nd rpmsg dev which not link to voc pcm dev
		list_for_each_entry_safe(rpmsg_dev, n, &rpmsg_dev_list, list) {
			if (rpmsg_dev->voc_pcm == NULL) {
				rpmsg_dev->voc_pcm = pcm_data_dev;
				pcm_data_dev->rpmsg_dev = rpmsg_dev;
				complete(&pcm_data_dev->rpmsg_bind);
				break;
			}
		}
	}

	mutex_unlock(&rpmsg_voc_bind_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(_mtk_bind_rpmsg_voc_pcm);

/*
 * RPMSG functions
 */
static int mtk_voc_txn(struct mtk_voc_rpmsg *voc_rpmsg, void *data,
			   size_t len)
{
	int ret;

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		dev_err(voc_rpmsg->dev, "Invalid data or length\n");
		return -EINVAL;
	}
#ifdef DEBUG
	pr_info("Send:\n");
	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET, 16, 1, data, len,
		       true);
#endif
	voc_rpmsg->txn_result = -EIO;
	ret = rpmsg_send(voc_rpmsg->rpdev->ept, data, len);
	if (ret) {
		dev_err(voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}
	// wait for ack
	wait_for_completion_interruptible(&voc_rpmsg->ack);

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
		dev_err(voc_rpmsg->dev,
			"Out of sync command, send %d get %d\n", cmd,
			res_pk->cmd);
		return -ENOTSYNC;
	}

	return 0;
}

int voc_dma_update_level_cnt(struct mtk_voc_rpmsg *voc_rpmsg,
	uint32_t period_num, uint32_t period_size)
{
	struct voc_pcm_data *pcm_data = voc_rpmsg->voc_pcm;
	uint64_t interval;

	if (pcm_data->pcm_cm4_period == 0) {
		pcm_data->pcm_cm4_period = period_size;
		//g_level_cnt = period_size; //1st frame, period num = 0
	}

	if (period_num <= pcm_data->pcm_last_period_num) {
		pr_err("wrap around last= %u, new %u\n",
		pcm_data->pcm_last_period_num, period_num);
		interval =
			(uint64_t)period_num + MAX_CM4_PERIOD - pcm_data->pcm_last_period_num;
	} else {
		interval = (period_num - pcm_data->pcm_last_period_num);
		//if(interval>1)
	}

	pcm_data->pcm_level_cnt += interval*pcm_data->pcm_cm4_period;

    /* xrun threshold */
	if (pcm_data->pcm_level_cnt
			> pcm_data->pcm_buf_size - pcm_data->pcm_cm4_period*FULL_THRES) {
		pr_err("%s XRUN\n", __func__);
		pcm_data->pcm_status = PCM_XRUN;
	}

	pcm_data->pcm_last_period_num = period_num;
	return pcm_data->pcm_level_cnt;
}

static inline int __mtk_rpmsg_rx_handler(struct mtk_voc_rpmsg *voc_rpmsg)
{
	struct voice_packet *voice_msg =
	    (struct voice_packet *)&voc_rpmsg->receive_msg.data;
	int ret = 0;

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev,
			"MSG_PERIOD_NOTIFY, get %d\n", voice_msg->cmd);

	switch (voice_msg->cmd) {
	case VOICE_CMD_DATA_MOVING_NOTIFY: {
		if (voc_rpmsg->voc_pcm->voc_pcm_running) {
			voc_rpmsg->vd_notify = 1;
			wake_up_interruptible(&voc_rpmsg->kthread_waitq);
		}

		break;
	}
	case VOICE_CMD_HOTWORD_VERSION: {
		uint32_t ver;

		ver =
			((struct hotword_ver *)voice_msg->payload)->ver;
		dev_dbg(voc_rpmsg->dev, "hotword version %d\n", ver);
		voc_rpmsg->voc_card->card_reg[10] = ver;
		ret = voc_rpmsg->voc_card->card_reg[10];
		break;
	}
	case VOICE_CMD_HOTWORD_DETECTED: {
		struct snd_kcontrol *kctl = NULL;
		struct snd_ctl_elem_id elem_id;
		struct snd_pcm_substream *substream = NULL;
		bool enable = ((struct wakeup_enable *)voice_msg->payload)->enable;

		/* sanity check */
		if (!voc_rpmsg->voc_pcm->capture_stream) {
			dev_err(NULL, "voc_rpmsg->voc_pcm->capture_stream is NULL\n");
			return -EINVAL;
		}

		substream = voc_rpmsg->voc_pcm->capture_stream;

		if (enable) {
			memset(&elem_id, 0, sizeof(elem_id));
			elem_id.numid = 2;
			kctl = snd_ctl_find_id(substream->pcm->card, &elem_id);
			if (kctl != NULL)
				snd_ctl_notify(substream->pcm->card,
				SNDRV_CTL_EVENT_MASK_VALUE, &kctl->id);
		}

		break;
	}
	case VOICE_CMD_SLT_TEST: {
		int32_t status;

		status = ((struct slt_test *)voice_msg->payload)->status;
		voc_rpmsg->voc_card->status = status;
		break;
	}
	default:
		return ret;
	}

	return ret;
}

static int mtk_vd_task_thread(void *param)
{
	struct mtk_voc_rpmsg *voc_rpmsg = (struct mtk_voc_rpmsg *)param;

	for (;;) {

		wait_event_interruptible(voc_rpmsg->kthread_waitq,
						kthread_should_stop() || voc_rpmsg->vd_notify);

		if (kthread_should_stop())
			break;

		if (voc_rpmsg->voc_pcm->voc_pcm_running) {
			struct voice_packet *voice_msg =
				(struct voice_packet *)&voc_rpmsg->receive_msg.data;
			struct snd_pcm_substream *substream =
				(struct snd_pcm_substream *)voc_rpmsg->voc_pcm->capture_stream;
			struct snd_pcm_runtime *runtime = substream->runtime;
			struct voc_pcm_runtime_data *prtd = runtime->private_data;
			uint32_t period_cnt, period_size, level_cnt;
			unsigned long flag1, flag2;
			unsigned int state = 0;

			state = prtd->state;
			period_cnt =
				((struct data_moving_notify *)voice_msg->payload)->period_count;
			period_size =
				((struct data_moving_notify *)voice_msg->payload)->byte_count;
			dev_dbg(voc_rpmsg->dev,
				"period_cnt = %d, period_size = %d\n",
				period_cnt,
				period_size);
			spin_lock_irqsave(&voc_rpmsg->voc_pcm->lock, flag1);
			level_cnt =
				voc_dma_update_level_cnt(voc_rpmsg, period_cnt, period_size);
			spin_unlock_irqrestore(&voc_rpmsg->voc_pcm->lock, flag1);
			dev_dbg(voc_rpmsg->dev, "updated level count %d\n", level_cnt);

			spin_lock_irqsave(&voc_rpmsg->voc_pcm->lock, flag1);
			if (prtd->state != DMA_FULL) {
				if (voc_dma_xrun(voc_rpmsg)) {
					prtd->state = DMA_FULL;
					wmb();

					spin_unlock_irqrestore(&voc_rpmsg->voc_pcm->lock, flag1);
					snd_pcm_period_elapsed(substream);
					snd_pcm_stream_lock_irqsave(substream, flag2);
					if (snd_pcm_running(substream))
						snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
					snd_pcm_stream_unlock_irqrestore(substream, flag2);
					spin_lock_irqsave(&voc_rpmsg->voc_pcm->lock, flag1);
				} else if ((prtd->state != DMA_OVERRUN) &&
								voc_dma_data_ready(voc_rpmsg)) {
					prtd->state = DMA_OVERRUN;
					wmb();
					spin_unlock_irqrestore(&voc_rpmsg->voc_pcm->lock, flag1);
					snd_pcm_period_elapsed(substream);
					spin_lock_irqsave(&voc_rpmsg->voc_pcm->lock, flag1);
				}
			}
			spin_unlock_irqrestore(&voc_rpmsg->voc_pcm->lock, flag1);
		}
		voc_rpmsg->vd_notify = 0;
	}
	return 0;

}

//CM4 notification interval is based on the fixed frame size(16kHz,16bits),
//and it's irrelevant to bitwidth and sample rate.
//So we should update notification frequency
//in case the notification is frequent.
int voc_dma_init_channel(
		struct mtk_voc_rpmsg *voc_rpmsg,
		uint32_t dma_paddr,
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

	dma_paddr = dma_paddr + MIU_BASE;

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_err(voc_rpmsg->dev, "%s\n", __func__);
	dev_err(voc_rpmsg->dev,
			"dma_paddr=%x, buf_size=%x\n", dma_paddr, buf_size);

	dev_err(voc_rpmsg->dev,
			"channels=%d, sample_width=%d, sample_rate=%d\n"
			, channels, sample_width, sample_rate);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_DATA_MOVING_CONFIG;

	((struct data_moving_config *)voice_msg->payload)->addr = dma_paddr;
	((struct data_moving_config *)voice_msg->payload)->size = buf_size;
	((struct data_moving_config *)voice_msg->payload)->mic_numbers = channels;
	((struct data_moving_config *)voice_msg->payload)->bitwidth = sample_width;
	((struct data_moving_config *)voice_msg->payload)->sample_rate = sample_rate;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_init_channel);

int voc_dma_start_channel(struct mtk_voc_rpmsg *voc_rpmsg)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_DATA_MOVING_ENABLE;
	((struct data_moving_enable *)voice_msg->payload)->enable = 1;

	/* execute RPMSG command */
	//ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	ret = rpmsg_send(voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret) {
		dev_err(voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}


	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_start_channel);

int voc_dma_stop_channel(struct mtk_voc_rpmsg *voc_rpmsg)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_DATA_MOVING_ENABLE;
	((struct data_moving_enable *)voice_msg->payload)->enable = 0;

	/* execute RPMSG command */
	//ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	ret = rpmsg_send(voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));

	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_stop_channel);

int voc_dma_enable_sinegen(struct mtk_voc_rpmsg *voc_rpmsg, bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_SIGEN_ENABLE;
	((struct sine_tone_enable *)voice_msg->payload)->enable = en;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dma_enable_sinegen);

int voc_enable_vq(struct mtk_voc_rpmsg *voc_rpmsg, bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_WAKEUP_ENABLE;
	((struct wakeup_enable *)voice_msg->payload)->enable = en;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_vq);

int voc_config_vq(struct mtk_voc_rpmsg *voc_rpmsg, uint8_t mode)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_WAKEUP_CONFIG;

	if (mode == 1)
		((struct wakeup_config *)voice_msg->payload)->mode = VOICE_WAKEUP_PM;
	else if (mode == 2)
		((struct wakeup_config *)voice_msg->payload)->mode = VOICE_WAKEUP_AUTO_TEST;
	else
		((struct wakeup_config *)voice_msg->payload)->mode = VOICE_WAKEUP_OFF;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_config_vq);

int voc_enable_hpf(struct mtk_voc_rpmsg *voc_rpmsg, int32_t stage)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_HPF_STAGE;
	((struct hpf_stage *)voice_msg->payload)->hpf_stage = stage;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_hpf);

int voc_config_hpf(struct mtk_voc_rpmsg *voc_rpmsg, int32_t coeff)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_HPF_CONFIG;
	((struct hpf_config *)voice_msg->payload)->hpf_coeff = coeff;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_config_hpf);

int voc_enable_vp(struct mtk_voc_rpmsg *voc_rpmsg, bool en)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_PREPROCESS_ENABLE;
	voice_msg->payload[0] = en;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_vp);

int voc_config_vp(struct mtk_voc_rpmsg *voc_rpmsg, struct vp_config_s config)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_PREPROCESS_CONFIG;
	voice_msg->payload[0] = config.scale;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_config_vp);

int voc_dmic_number(struct mtk_voc_rpmsg *voc_rpmsg, uint8_t mic)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_MIC_CONFIG;
	((struct mic_config *)voice_msg->payload)->mic_numbers = mic;
	((struct mic_config *)voice_msg->payload)->sample_rate = 0;
	((struct mic_config *)voice_msg->payload)->bitwidth = 0;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dmic_number);

int voc_dmic_bitwidth(struct mtk_voc_rpmsg *voc_rpmsg, uint8_t bitwidth)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_MIC_CONFIG;
	((struct mic_config *)voice_msg->payload)->mic_numbers = 0;
	((struct mic_config *)voice_msg->payload)->sample_rate = 0;
	((struct mic_config *)voice_msg->payload)->bitwidth = bitwidth;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dmic_bitwidth);

int voc_dmic_gain(struct mtk_voc_rpmsg *voc_rpmsg, uint16_t gain)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_MIC_GAIN;
	((struct mic_gain *)voice_msg->payload)->steps = gain;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_dmic_gain);

int voc_reset_voice(struct mtk_voc_rpmsg *voc_rpmsg)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_RESET;

	/* execute RPMSG command */
	//ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	ret = rpmsg_send(voc_rpmsg->rpdev->ept, &open_msg, sizeof(struct ipcm_msg));
	if (ret) {
		dev_err(voc_rpmsg->dev, "rpmsg send failed, ret=%d\n", ret);
		return ret;
	}

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_reset_voice);

int voc_update_snd_model(
				struct mtk_voc_rpmsg *voc_rpmsg,
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

	u32 dram_src_crc, dram_des_crc, des_addr, src_addr;

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	src_addr = (snd_model_paddr + TLV_HEADER_SIZE) - MIU_BASE;
	des_addr = voc_rpmsg->voc_card->voc_buf_paddr
							+ voc_rpmsg->voc_card->snd_model_offset;

	//put data to DRAM and check CRC
	dram_src_crc = vbdma_crc32(voc_rpmsg->voc_card->vbdma_ch,
					src_addr, size, E_VBDMA_DEV_MIU);
	ret = vbdma_copy(voc_rpmsg->voc_card->vbdma_ch,
					src_addr, des_addr, size, E_VBDMA_HKtoHK);
	dram_des_crc = vbdma_crc32(voc_rpmsg->voc_card->vbdma_ch,
					des_addr, size, E_VBDMA_DEV_MIU);
	dev_dbg(voc_rpmsg->dev, "src_addr = 0x%x\n", src_addr);
	dev_info(voc_rpmsg->dev, "des_addr = 0x%x\n", des_addr);
	dev_info(voc_rpmsg->dev, "size = 0x%x\n", size);
	dev_dbg(voc_rpmsg->dev, "dram_src_crc = 0x%x\n", dram_src_crc);
	dev_info(voc_rpmsg->dev, "dram_crc = 0x%x\n", dram_des_crc);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_SOUND_MODEL_UPDATE;
	((struct sound_model_config *)voice_msg->payload)->addr =
					des_addr + MIU_BASE;
	((struct sound_model_config *)voice_msg->payload)->size = size;
	((struct sound_model_config *)voice_msg->payload)->crc = dram_des_crc;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(pdata_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_update_snd_model);

int voc_get_hotword_ver(struct mtk_voc_rpmsg *voc_rpmsg)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);

	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_HOTWORD_VERSION;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_get_hotword_ver);

int voc_enable_slt_test(struct mtk_voc_rpmsg *voc_rpmsg, int32_t loop, uint32_t addr)
{
	int ret = 0;
	struct voice_packet *voice_msg;
	struct ipcm_msg open_msg = {
		.cmd = IPCM_CMD_OP,
		.size = 0,
		.data = {0},
	};

	/* sanity check */
	if (!voc_rpmsg) {
		dev_err(NULL, "rpmsg dev is NULL\n");
		return -EINVAL;
	}

	dev_dbg(voc_rpmsg->dev, "%s\n", __func__);
	open_msg.size = sizeof(struct voice_packet);
	voice_msg = (struct voice_packet *)&open_msg.data;
	voice_msg->cmd = VOICE_CMD_SLT_TEST;
	((struct slt_test *)voice_msg->payload)->loop = loop;
	((struct slt_test *)voice_msg->payload)->addr = addr;

	/* execute RPMSG command */
	ret = voc_rpmsg->txn(voc_rpmsg, &open_msg, sizeof(struct ipcm_msg));
	if (ret)
		return ret;

	// check received result
//	return  __mtk_rpmsg_txn_sync(voc_rpmsg, voice_msg->cmd);

	return voc_rpmsg->txn_result;
}
EXPORT_SYMBOL_GPL(voc_enable_slt_test);

static int mtk_voc_rpmsg_callback(struct rpmsg_device *rpdev,
				       void *data, int len, void *priv, u32 src)
{
	struct mtk_voc_rpmsg *voc_rpmsg;
	struct ipcm_msg *msg;

	voc_rpmsg = dev_get_drvdata(&rpdev->dev);
	if (!voc_rpmsg) {
		dev_err(&rpdev->dev, "private data is NULL\n");
		return -EINVAL;
	}

	if (!data || len == 0) {
		dev_err(&rpdev->dev, "Invalid data or length from src:%d\n",
			src);
		return -EINVAL;
	}

	msg = (struct ipcm_msg *)data;

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
		dev_err(&rpdev->dev, "Got OK ack, response for cmd %d\n",
			msg->data[0]);
		if (len > sizeof(struct ipcm_msg))
			dev_err(&rpdev->dev, "Receive data length over size\n");

		/* store received data for transaction checking */
		memcpy(&voc_rpmsg->receive_msg, data,
				sizeof(struct ipcm_msg));
		voc_rpmsg->txn_result = 0;
		complete(&voc_rpmsg->ack);
		break;
	case IPCM_CMD_BAD_ACK:
		dev_err(&rpdev->dev, "Got BAD ack, result = %d\n",
			*(int *)&msg->data);
		complete(&voc_rpmsg->ack);
		break;
	case IPCM_CMD_MSG:
		dev_dbg(&rpdev->dev, "IPCM_CMD_MSG\n");
		if (len > sizeof(struct ipcm_msg))
			dev_err(&rpdev->dev, "Receive data length over size\n");
		/* store received data for transaction checking */
		memcpy(&voc_rpmsg->receive_msg, data,
				sizeof(struct ipcm_msg));
		__mtk_rpmsg_rx_handler(voc_rpmsg);
		break;
	default:
		dev_err(&rpdev->dev, "Invalid command %d from src:%d\n",
			msg->cmd, src);
		return -EINVAL;
	}
	return 0;
}

static int mtk_voc_rpmsg_probe(struct rpmsg_device *rpdev)
{
	int ret;
	struct mtk_voc_rpmsg *voc_rpmsg;

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

	// add to rpmsg_dev list
	INIT_LIST_HEAD(&voc_rpmsg->list);
	list_add_tail(&voc_rpmsg->list, &rpmsg_dev_list);

	// check if there is a valid voc_soc_card_dev
	ret = _mtk_bind_rpmsg_voc_soc_card(voc_rpmsg, NULL);
	if (ret) {
		dev_err(&rpdev->dev,
			"binding rpmsg-voc_soc_card failed! (%d)\n", ret);
		goto bind_fail;
	}

	ret = _mtk_bind_rpmsg_voc_pcm(voc_rpmsg, NULL);
	if (ret) {
		dev_err(&rpdev->dev,
			"binding rpmsg-voc_pcm failed! (%d)\n", ret);
		goto bind_fail;
	}

	init_waitqueue_head(&voc_rpmsg->kthread_waitq);
	voc_rpmsg->vd_task = kthread_run(mtk_vd_task_thread, (void *)voc_rpmsg, "vd_task");
	if (IS_ERR(voc_rpmsg->vd_task)) {
		dev_err(&rpdev->dev, "create thread for add vd_task failed!\n");
		ret = IS_ERR(voc_rpmsg->vd_task);
		goto err_kthread_run;
	}

	dev_set_drvdata(&rpdev->dev, voc_rpmsg);

	return 0;

bind_fail:
	list_del(&voc_rpmsg->list);
	return ret;

err_kthread_run:
	kthread_stop(voc_rpmsg->vd_task);
	return ret;
}

static void mtk_voc_rpmsg_remove(struct rpmsg_device *rpdev)
{
	struct mtk_voc_rpmsg *voc_rpmsg  = dev_get_drvdata(&rpdev->dev);

	list_del(&voc_rpmsg->list);
	kthread_stop(voc_rpmsg->vd_task);
	dev_set_drvdata(&rpdev->dev, NULL);
	dev_info(&rpdev->dev, "voice rpmsg remove done\n");
}

static struct rpmsg_device_id mtk_voc_rpmsg_driver_sample_id_table[] = {
	{ .name	= "voice-pmu-ept"},
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, mtk_voc_rpmsg_driver_sample_id_table);

static struct rpmsg_driver mtk_voc_rpmsg_driver = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= mtk_voc_rpmsg_driver_sample_id_table,
	.probe		= mtk_voc_rpmsg_probe,
	.callback	= mtk_voc_rpmsg_callback,
	.remove	= mtk_voc_rpmsg_remove,
};
module_rpmsg_driver(mtk_voc_rpmsg_driver);

MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
