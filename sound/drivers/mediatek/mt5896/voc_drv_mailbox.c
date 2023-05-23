// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
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
#include <linux/mailbox_client.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <sound/soc.h>
#include "voc_common.h"
#include "voc_common_reg.h"
#include "voc_drv_mailbox.h"
#include "vbdma.h"

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define VOICE_DEVICE_NODE "mediatek,mtk-mic-dma"

#define VOICE_COPROCESSOR_SUSPEND (0x88)
#define VOICE_COPROCESSOR_RESUME  (0x99)

#define VOICE_IMI_ISOINTB_ON      (0xFFFF)
#define VOICE_IMI_ISOINTB_OFF     (0x0000)

#define VOICE_MBOX_ACK_MASK       (0xC0)
#define VOICE_MBOX_DATA_MASK      (0x3F)
#define VOICE_MBOX_CMD_OP         (0xFF)
#define VOICE_MBOX_CMD_BAD_ACK    (0x40)
#define VOICE_MBOX_CMD_ACK        (0x80)
#define VOICE_MBOX_CMD_MSG        (0xC0)

#define VOICE_MBOX_PACKET_SIZE    (62)
#define VOICE_MBOX_DATA_SIZE      (VOICE_MBOX_PACKET_SIZE - 1)
#define VOICE_MBOX_NEED_DELAY     (2)
#define VOICE_MBOX_NEED_ACK       (1)
#define VOICE_MBOX_IGNORE_ACK     (0)
#define VOICE_MBOX_DELAY_TIME     (5000UL)

#define PTP_TIME_INDEX_0    0
#define PTP_TIME_INDEX_1    1
#define PTP_TIME_INDEX_2    2
#define PTP_TIME_INDEX_3    3
#define DUMP_COUNT_16       16
#define MAILBOX_MAGIC_02    2
#define MAILBOX_ELEM_ID_02  2
#define U32_BIT_NUM         32
#define MAILBOX_TIMEOUT_MS  50
#define MAILBOX_WAIT_MS     3000
#define MAILBOX_SLEEP_MS    200
//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
static struct voc_communication_operater voc_mailbox_op;
struct mtk_voc_mailbox *g_voc_mailbox;
//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------



enum mailbox_ack_t {
	MBOX_CMD_OP = VOICE_MBOX_CMD_OP,
	MBOX_CMD_BAD_ACK = VOICE_MBOX_CMD_BAD_ACK,
	MBOX_CMD_ACK = VOICE_MBOX_CMD_ACK,
	MBOX_CMD_MSG = VOICE_MBOX_CMD_MSG,
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

enum voice_status_mode {
	VOICE_STATUS_SUSPEND = 0,
	VOICE_STATUS_RESUME,
	VOICE_STATUS_MAX,
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

struct voice_mbox_msg {
	uint8_t cmd;
	uint8_t data[VOICE_MBOX_DATA_SIZE];
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

/* The mailbox bus device */
struct mtk_voc_mailbox {
	struct device *dev;
	struct mbox_chan *mchan;
	int (*txn)(struct mtk_voc_mailbox *voc_mailbox,
			struct voice_mbox_msg *mssg, int need_ack);
	int txn_result;
	int ack_status;
	int running_status;
	int resume_status;
	int notify;
	int time_notify;
	struct mbox_client mbox_client;
	struct voice_mbox_msg send_msg;
	struct voice_mbox_msg receive_msg;
	wait_queue_head_t kthread_waitq;
	struct completion ack;
	struct completion time_ack;
	struct mtk_snd_voc *voc;
	struct list_head list;
	struct voice_mbox_msg receive_vd;

	//register
	phys_addr_t pm_sleep;
	phys_addr_t pm_misc;
	phys_addr_t cpu_int;
	phys_addr_t coin_cpu;
	phys_addr_t pm_imi;
};

/* A list of mailbox_dev */
static LIST_HEAD(mailbox_dev_list);
/* Lock to serialise MAILBOX device and PWM device binding */
static DEFINE_MUTEX(mailbox_voc_bind_lock);
static DEFINE_MUTEX(mailbox_sending_lock);

static int __mtk_mailbox_timer_sending(void *arg)
{
	struct mtk_voc_mailbox *voc_mailbox = (struct mtk_voc_mailbox *)arg;
	struct snd_kcontrol *kctl = NULL;
	struct snd_ctl_elem_id elem_id;

	if (!voc_mailbox) {
		voc_dev_err(NULL, "mailbox is NULL\n");
		return -EINVAL;
	}

	for (;;) {
		wait_event_interruptible(voc_mailbox->kthread_waitq,
				kthread_should_stop() || voc_mailbox->notify);

		if (kthread_should_stop()) {
			voc_dev_err(voc_mailbox->dev,
				"%s Receive kthread_should_stop\n", __func__);
			break;
		}

		if (voc_mailbox->notify == (VOICE_CMD_HOTWORD_DETECTED+1)) {
			memset(&elem_id, 0, sizeof(elem_id));
			elem_id.numid = MAILBOX_ELEM_ID_02;
			kctl = snd_ctl_find_id(voc_mailbox->voc->card, &elem_id);
			if (kctl != NULL) {
				snd_ctl_notify(voc_mailbox->voc->card,
				SNDRV_CTL_EVENT_MASK_VALUE, &kctl->id);
			}
			voc_dev_dbg(voc_mailbox->dev,
				"HOTWORD DETECTED send event notify\n");
		} else if (voc_mailbox->notify == (VOICE_CMD_TIMESTAMP_NOTIFY+1)) {
			voc_drv_mailbox_ts_delay_req();
			voc_dev_dbg(voc_mailbox->dev,
				"TIMESTAMP_NOTIFY follow-up send delay request\n");
		}

		voc_mailbox->notify = 0;
	}

	return 0;
}

static int _mtk_mailbox_pm_imi_status(struct mtk_voc_mailbox *voc_mailbox, int status)
{
	int i;
	unsigned int reg_id;
	unsigned short val;

	if (!voc_mailbox) {
		voc_dev_err(NULL, "mailbox is NULL\n");
		return -EINVAL;
	}

	if (status) {
		for(reg_id = VOC_REG_0XA0; reg_id < VOC_REG_0XC0; reg_id+=VOC_VAL_2)
		{
			for(i = 0; i < VOC_VAL_16; i++)
			{
				val = INREG16((voc_mailbox->pm_imi)|(OFFSET(reg_id)));
				val |= (1 << i);
				OUTREG16((voc_mailbox->pm_imi)|(OFFSET(reg_id)), val);
			}
		}

		//delay 1us before isointb setting after sleep bank apply
		udelay(1);

		//disable isointb (bank0~bank31)
		OUTREG16((voc_mailbox->pm_imi)|(OFFSET(VOC_REG_0XC0)), VOICE_IMI_ISOINTB_ON);
		OUTREG16((voc_mailbox->pm_imi)|(OFFSET(VOC_REG_0XC2)), VOICE_IMI_ISOINTB_ON);
	} else {
                OUTREG16((voc_mailbox->pm_imi)|(OFFSET(VOC_REG_0XC0)), VOICE_IMI_ISOINTB_OFF);
                OUTREG16((voc_mailbox->pm_imi)|(OFFSET(VOC_REG_0XC2)), VOICE_IMI_ISOINTB_OFF);

                //delay 1us before sleep bank setting after isointb apply
                udelay(1);

                for(reg_id = VOC_REG_0XA0; reg_id < VOC_REG_0XC0; reg_id+=VOC_VAL_2)
                {
                        for(i = 0; i < VOC_VAL_16; i++)
                        {
                                val = INREG16((voc_mailbox->pm_imi)|(OFFSET(reg_id)));
                                val &= ~(1 << i);
                                OUTREG16((voc_mailbox->pm_imi)|(OFFSET(reg_id)), val);
                        }
                }
        }

	return 0;
}

static int __mtk_mailbox_resume(struct mtk_voc_mailbox *voc_mailbox)
{
	if (!voc_mailbox) {
		voc_dev_err(NULL, "mailbox is NULL\n");
		return -EINVAL;
	}

	OUTREG16((voc_mailbox->pm_sleep)|(OFFSET(VOC_REG_0XA0)), VOICE_COPROCESSOR_RESUME);
	SETREG16((voc_mailbox->cpu_int)|(OFFSET(VOC_REG_0X02)), BIT2);
	CLRREG16((voc_mailbox->cpu_int)|(OFFSET(VOC_REG_0X02)), BIT2);
	msleep(1);
	return 0;
}

static int __mtk_mailbox_suspend(struct mtk_voc_mailbox *voc_mailbox)
{
	if (!voc_mailbox) {
		voc_dev_err(NULL, "mailbox is NULL\n");
		return -EINVAL;
	}

	OUTREG16((voc_mailbox->pm_sleep)|(OFFSET(VOC_REG_0XA0)), VOICE_COPROCESSOR_SUSPEND);
	SETREG16((voc_mailbox->cpu_int)|(OFFSET(VOC_REG_0X02)), BIT2);
	CLRREG16((voc_mailbox->cpu_int)|(OFFSET(VOC_REG_0X02)), BIT2);
	return 0;
}

static inline int __mtk_mailbox_rx_handler(struct mtk_voc_mailbox *voc_mailbox)
{
	struct voice_mbox_msg *voice_msg = NULL;
	int ret = 0;

	/* sanity check */
	if (!voc_mailbox) {
		voc_dev_err(NULL, "mailbox is NULL\n");
		return -EINVAL;
	}

	if (!voc_mailbox->dev) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}
	voice_msg = (struct voice_mbox_msg *)&voc_mailbox->receive_msg;

	switch (voice_msg->cmd) {
	case VOICE_CMD_DATA_MOVING_NOTIFY: {
		if (voc_mailbox->voc->voc_pcm_running) {
			memcpy(&voc_mailbox->receive_vd, &voc_mailbox->receive_msg,
				sizeof(struct voice_mbox_msg));
			voc_mailbox->voc->vd_notify = 1;
			wake_up_interruptible(&voc_mailbox->voc->kthread_waitq);
		}

		break;
	}
	case VOICE_CMD_DATA_MOVING_STATUS: {
		enum voice_data_moving_status dm_status;

		/* sanity check */
		if (!voc_mailbox->voc) {
			voc_dev_err(NULL, "voc_mailbox->voc is NULL\n");
			return -EINVAL;
		}

		dm_status = ((struct data_moving_status *)voice_msg->data)->status;
		voc_dev_dbg(voc_mailbox->dev, "VOICE_CMD_DATA_MOVING_STATUS:%d\n", dm_status);
		voc_mailbox->voc->voc_pcm_status = dm_status;
		break;
	}
	case VOICE_CMD_HOTWORD_VERSION: {
		uint32_t ver;

		/* sanity check */
		if ((!voc_mailbox->voc) || (!voc_mailbox->voc->card)) {
			voc_dev_err(NULL, "voc_mailbox->voc or voc_mailbox->voc->card is NULL\n");
			return -EINVAL;
		}
		ver =
			((struct hotword_ver *)voice_msg->data)->ver;
		voc_dev_dbg(voc_mailbox->dev, "hotword version %d\n", ver);
		voc_mailbox->voc->voc_card->card_reg[VOC_REG_HOTWORD_VER] = ver;
		ret = voc_mailbox->voc->voc_card->card_reg[VOC_REG_HOTWORD_VER];
		break;
	}
	case VOICE_CMD_HOTWORD_IDENTIFIER: {
		uint8_t *identifier = ((struct hotword_id *)voice_msg->data)->id;
		int32_t size = ((struct hotword_id *)voice_msg->data)->size;
		int32_t array_size = 0;

		/* sanity check */
		if ((!voc_mailbox->voc) || (!voc_mailbox->voc->card)) {
			voc_dev_err(NULL, "voc_mailbox->voc or voc_mailbox->voc->card is NULL\n");
			return -EINVAL;
		}
		array_size = voc_mailbox->voc->voc_card->uuid_size;

		voc_dev_dbg(voc_mailbox->dev, "hotword id=[%s], size=%d, array size=%d\n",
			identifier, size, array_size);
		if (!voc_mailbox->voc->voc_card->uuid_array) {
			voc_dev_err(voc_mailbox->dev, "Invalid uuid array\n");
			return -EINVAL;
		}

		if (size <= array_size) {
			memcpy(voc_mailbox->voc->voc_card->uuid_array, identifier, size);
		} else {
			voc_dev_warn(voc_mailbox->dev, "hotword id size overflow.(%d>%d)",
				size, array_size);
			memcpy(voc_mailbox->voc->voc_card->uuid_array,
				identifier, array_size);
		}
		break;
	}
	case VOICE_CMD_HOTWORD_DETECTED: {
		bool enable = ((struct wakeup_enable *)voice_msg->data)->enable;

		/* sanity check */
		if ((!voc_mailbox->voc) || (!voc_mailbox->voc->card)) {
			voc_dev_err(NULL, "voc_mailbox->voc or voc_mailbox->voc->card is NULL\n");
			return -EINVAL;
		}

		if (enable) {
			voc_mailbox->notify = (VOICE_CMD_HOTWORD_DETECTED+1);
			wake_up_interruptible(&voc_mailbox->kthread_waitq);
		}
		break;
	}
	case VOICE_CMD_SLT_TEST: {
		int32_t status;

		status = ((struct slt_test *)voice_msg->data)->status;
		voc_mailbox->voc->voc_card->status = status;
		break;
	}
	case VOICE_CMD_TIMESTAMP_NOTIFY: {
		uint32_t stage = ((struct ts_config *)voice_msg->data)->stage;
		uint64_t ts = ((struct ts_config *)voice_msg->data)->ts;
		struct timespec64 current_time;

		voc_dev_dbg(voc_mailbox->dev, "TS_NOTIFY Stage: %d, TS: %llu\n", stage, ts);
		switch (stage) {
		case VOICE_TS_SYNC: {
			ktime_get_ts64(&current_time);
			voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_0] = ts*1000000LL;//ms -> ns
			voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_1] =
				(current_time.tv_sec*1000000000LL + current_time.tv_nsec);

			voc_dev_dbg(voc_mailbox->dev,
				"TS: %llu, Time = [(%lld)nsec, (%lld)sec, (%ld)nsec]\n",
				ts, voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_1],
				current_time.tv_sec, current_time.tv_nsec);

			break;
		}
		case VOICE_TS_FOLLOW_UP: {
			//Receive Follow-up message,
			//get time before send delay_request message.
			ktime_get_ts64(&current_time);
			voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_2] =
				(current_time.tv_sec*1000000000LL + current_time.tv_nsec);

			voc_dev_dbg(voc_mailbox->dev,
				"TS: %llu, Time = [(%lld)nsec, (%lld)sec, (%ld)nsec]\n",
				ts, voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_2],
				current_time.tv_sec, current_time.tv_nsec);

			voc_mailbox->notify = (VOICE_CMD_TIMESTAMP_NOTIFY+1);
			wake_up_interruptible(&voc_mailbox->kthread_waitq);
			voc_dev_dbg(voc_mailbox->dev, "TIMESTAMP_NOTIFY follow-up wakeup\n");
			break;
		}
		case VOICE_TS_DELAY_RESP: {
			int64_t diffpc = 0;
			int64_t diffcp = 0;

			//Receive Delay response message, calculate delta time.
			voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_3] = ts*1000000LL;//ms->ns
			voc_dev_dbg(voc_mailbox->dev, "TS: %llu\n", ts);

			//Delta time is average time of ([PMU->CPU] - [CPU->PMU]) / 2
			diffpc = (voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_1]
					- voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_0]);
			diffcp = (voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_3]
					- voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_2]);
			voc_mailbox->voc->sync_time = (diffpc - diffcp) / MAILBOX_MAGIC_02;

			voc_dev_dbg(voc_mailbox->dev,
				"Sync time = %lld, (PMU->CPU):%lld, (CPU->PMU):%lld\n",
				voc_mailbox->voc->sync_time, diffpc, diffcp);

			voc_dev_dbg(voc_mailbox->dev,
				"PTP time = [t0=%lld, t1=%lld, t2=%lld, t3=%lld]\n",
				voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_0],
				voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_1],
				voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_2],
				voc_mailbox->voc->ptp_time[PTP_TIME_INDEX_3]);

			voc_mailbox->time_notify = 1;
			complete(&voc_mailbox->time_ack);
			break;
		}
		case VOICE_TS_START:
		case VOICE_TS_DELAY_REQ:
		default:
			voc_dev_err(voc_mailbox->dev, "Invallid argument stage: %d\n", stage);
			break;

		}
		break;
	}
	default:
		return ret;
	}

	return ret;
}

static void mtk_voc_mailbox_callback(struct mbox_client *client, void *mssg)
{
	struct mtk_voc_mailbox *voc_mailbox;

	voc_mailbox = g_voc_mailbox;
	if (!voc_mailbox) {
		voc_dev_err(NULL, "mailbox device is NULL\n");
		return;
	}

	memcpy(&voc_mailbox->receive_msg, mssg, sizeof(struct voice_mbox_msg));

	switch (voc_mailbox->receive_msg.cmd & VOICE_MBOX_ACK_MASK) {
	case MBOX_CMD_ACK:
		voc_dev_dbg(voc_mailbox->dev, "Got OK ack, response for cmd %d\n",
			voc_mailbox->receive_msg.cmd);

		voc_mailbox->ack_status = 0;
		complete(&voc_mailbox->ack);
		break;
	case MBOX_CMD_BAD_ACK:
		voc_dev_err(voc_mailbox->dev, "Got BAD ack, result = %d\n",
			*(int *)&voc_mailbox->receive_msg.data[0]);
		complete(&voc_mailbox->ack);
		break;
	case MBOX_CMD_MSG:
		voc_dev_dbg(voc_mailbox->dev, "MBOX_CMD_MSG\n");
		/* store received data for transaction checking */
		voc_mailbox->receive_msg.cmd =
			voc_mailbox->receive_msg.cmd & VOICE_MBOX_DATA_MASK;
		__mtk_mailbox_rx_handler(voc_mailbox);
		break;
	default:
		voc_dev_err(voc_mailbox->dev, "Invalid command %d\n",
			voc_mailbox->receive_msg.cmd);
	}
}

static int mtk_voc_mailbox_txn(struct mtk_voc_mailbox *voc_mailbox,
		struct voice_mbox_msg *mssg, int need_ack)
{
	int ret = -1;

	/* sanity check */
	if (!voc_mailbox || !mssg) {
		voc_dev_err(NULL, "mailbox dev or message is NULL\n");
		return -EINVAL;
	}

	mutex_lock(&mailbox_sending_lock);

	if (need_ack == VOICE_MBOX_NEED_ACK) {
		voc_mailbox->ack_status = -EIO;

		if (completion_done(&voc_mailbox->ack))
			reinit_completion(&voc_mailbox->ack);

	} else if (need_ack == VOICE_MBOX_NEED_DELAY) {
		//delay 5ms
		usleep_range(VOICE_MBOX_DELAY_TIME, VOICE_MBOX_DELAY_TIME + 1);
	}

	memcpy(&voc_mailbox->send_msg, mssg, sizeof(struct voice_mbox_msg));
	ret = mbox_send_message(voc_mailbox->mchan, &voc_mailbox->send_msg);
	if (ret == -ETIME)
		ret = mbox_send_message(voc_mailbox->mchan, &voc_mailbox->send_msg);

	if (ret < 0) {
		voc_dev_err(voc_mailbox->dev, "mailbox send failed, ret=%d\n", ret);
		mutex_unlock(&mailbox_sending_lock);
		return ret;
	}

	// wait for ack
	if (need_ack == VOICE_MBOX_NEED_ACK) {
		ret = wait_for_completion_timeout(&voc_mailbox->ack,
						msecs_to_jiffies(MAILBOX_WAIT_MS));
		//timeout or -ERESTARTSYS
		if (ret <= 0) {
			voc_dev_err(voc_mailbox->dev,
				"[%s]wait for completion timeout\n", __func__);
		}
	} else { //Ignore ack
		mutex_unlock(&mailbox_sending_lock);
		return 0;
	}
	// return ack status
	voc_mailbox->txn_result = voc_mailbox->ack_status;

	mutex_unlock(&mailbox_sending_lock);
	return voc_mailbox->txn_result;
}

int voc_drv_mailbox_bind(struct mtk_snd_voc *snd_voc)
{
	mutex_lock(&mailbox_voc_bind_lock);

	if (snd_voc) {
		struct mtk_voc_mailbox *mailbox_dev, *n;

		// check mailbox device list,
		// link 1st mailbox dev which not link to voc soc card dev
		list_for_each_entry_safe(mailbox_dev, n, &mailbox_dev_list, list) {
			if (mailbox_dev->voc == NULL) {
				mailbox_dev->voc = snd_voc;
				g_voc_mailbox = mailbox_dev;
				break;
			}
		}

		kthread_run(__mtk_mailbox_timer_sending,
			(void *)g_voc_mailbox, "voice mailbox timer sending");
	}

	mutex_unlock(&mailbox_voc_bind_lock);
	return 0;
}

int voc_drv_mailbox_dma_init_channel(
		uint64_t dma_paddr,
		uint32_t buf_size,
		uint32_t channels,
		uint32_t sample_width,
		uint32_t sample_rate)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (!g_voc_mailbox->voc) {
		voc_dev_err(NULL, "voc is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_info(g_voc_mailbox->dev, "%s\n", __func__);
	voc_dev_info(g_voc_mailbox->dev,
			"dma_paddr=%llx, buf_size=%x\n", dma_paddr, buf_size);

	voc_dev_info(g_voc_mailbox->dev,
			"channels=%d, sample_width=%d, sample_rate=%d\n"
			, channels, sample_width, sample_rate);

	open_msg.cmd = VOICE_CMD_DATA_MOVING_CONFIG;

	((struct data_moving_config *)open_msg.data)->addr = (uint32_t)dma_paddr;
	((struct data_moving_config *)open_msg.data)->addr_H =
	 (uint32_t)(dma_paddr >> U32_BIT_NUM);
	((struct data_moving_config *)open_msg.data)->size = buf_size;
	((struct data_moving_config *)open_msg.data)->mic_numbers = channels;
	((struct data_moving_config *)open_msg.data)->bitwidth = sample_width;
	((struct data_moving_config *)open_msg.data)->sample_rate = sample_rate;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_ACK);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_dma_start_channel(void)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	if (!g_voc_mailbox->time_notify)
		wait_for_completion_timeout(&g_voc_mailbox->time_ack,
						msecs_to_jiffies(MAILBOX_WAIT_MS));

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_DATA_MOVING_ENABLE;
	((struct data_moving_enable *)open_msg.data)->enable = 1;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_ACK);
	if (ret) {
		voc_dev_err(g_voc_mailbox->dev, "mailbox send failed, ret=%d\n", ret);
		return ret;
	}
	if (!g_voc_mailbox->txn_result)
		g_voc_mailbox->running_status = 1;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_dma_stop_channel(void)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_DATA_MOVING_ENABLE;
	((struct data_moving_enable *)open_msg.data)->enable = 0;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret) {
		voc_dev_err(g_voc_mailbox->dev, "mailbox send failed, ret=%d\n", ret);
		return ret;
	}
	if (!g_voc_mailbox->txn_result)
		g_voc_mailbox->running_status = 0;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_dma_enable_sinegen(bool en)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_SIGEN_ENABLE;
	((struct sine_tone_enable *)open_msg.data)->enable = en;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_enable_vq(bool en)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_WAKEUP_ENABLE;
	((struct wakeup_enable *)open_msg.data)->enable = en;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_config_vq(uint8_t mode)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_WAKEUP_CONFIG;

	if (mode == VOICE_WAKEUP_PM)
		((struct wakeup_config *)open_msg.data)->mode = VOICE_WAKEUP_PM;
	else if (mode == VOICE_WAKEUP_BOTH)
		((struct wakeup_config *)open_msg.data)->mode = VOICE_WAKEUP_BOTH;
	else if (mode == VOICE_WAKEUP_AUTO_TEST)
		((struct wakeup_config *)open_msg.data)->mode = VOICE_WAKEUP_AUTO_TEST;
	else
		((struct wakeup_config *)open_msg.data)->mode = VOICE_WAKEUP_NORMAL;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_enable_hpf(int32_t stage)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_HPF_STAGE;
	((struct hpf_stage *)open_msg.data)->hpf_stage = stage;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_ACK);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_config_hpf(int32_t coeff)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_HPF_CONFIG;
	((struct hpf_config *)open_msg.data)->hpf_coeff = coeff;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_dmic_gain(int16_t gain)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_MIC_GAIN;
	((struct mic_gain *)open_msg.data)->steps = gain;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_dmic_mute(bool en)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_MIC_MUTE_ENABLE;
	((struct mic_mute_enable *)open_msg.data)->enable = en;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_dmic_switch(bool en)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_MIC_SWITCH;
	((struct mic_switch_enable *)open_msg.data)->enable = en;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_ACK);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_reset_voice(void)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_RESET;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_DELAY);
	msleep(MAILBOX_SLEEP_MS);
	return ret;
}


int voc_drv_mailbox_update_snd_model(
				dma_addr_t snd_model_paddr,
				uint32_t size)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	uint32_t src_addr_H;

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	src_addr_H = (uint32_t)(snd_model_paddr >> U32_BIT_NUM);

	open_msg.cmd = VOICE_CMD_SOUND_MODEL_UPDATE;
	((struct sound_model_config *)open_msg.data)->addr = (uint32_t)snd_model_paddr;
	((struct sound_model_config *)open_msg.data)->addr_H = src_addr_H;
	((struct sound_model_config *)open_msg.data)->size = size;
	((struct sound_model_config *)open_msg.data)->crc = 0;//calculated by CM4

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_ACK);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_get_hotword_ver(void)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_HOTWORD_VERSION;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_get_hotword_identifier(void)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_HOTWORD_IDENTIFIER;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_enable_slt_test(int32_t loop, uint32_t addr)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_SLT_TEST;
	((struct slt_test *)open_msg.data)->loop = loop;
	((struct slt_test *)open_msg.data)->addr = addr;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_ACK);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_ts_sync_start(void)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_TIMESTAMP_NOTIFY;

	((struct ts_config *)open_msg.data)->stage = (uint32_t)VOICE_TS_START;
	((struct ts_config *)open_msg.data)->ts = 0;

	g_voc_mailbox->time_notify = 0;
	if (completion_done(&g_voc_mailbox->time_ack))
		reinit_completion(&g_voc_mailbox->time_ack);

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_DELAY);

	return ret;
}

int voc_drv_mailbox_ts_delay_req(void)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);
	open_msg.cmd = VOICE_CMD_TIMESTAMP_NOTIFY;

	((struct ts_config *)open_msg.data)->stage = (uint32_t)VOICE_TS_DELAY_REQ;
	((struct ts_config *)open_msg.data)->ts = 0;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_DELAY);

	return ret;
}

int voc_drv_mailbox_enable_seamless(bool en)
{
	int ret = 0;
	int ack_type = VOICE_MBOX_NEED_ACK;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_SEAMLESS;
	((struct seamless_enable *)open_msg.data)->enable = en;

	if (g_voc_mailbox->running_status)
		ack_type = VOICE_MBOX_NEED_DELAY;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, ack_type);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_power_status(int32_t status)
{
	if (!g_voc_mailbox || !g_voc_mailbox->voc) {
		voc_dev_err(NULL, "mailbox or voc dev is NULL\n");
		return -EINVAL;
	}

	if (!g_voc_mailbox->voc->power_saving)
		return 0;

	if (status == VOC_POWER_MODE_SHUTDOWN) {
		CLRREG16((g_voc_mailbox->coin_cpu)|(OFFSET(VOC_REG_0X00)), BIT0|BIT1);
		CLRREG16((g_voc_mailbox->pm_misc)|(OFFSET(VOC_REG_0X4E)), BIT4);

		//reg_sw_en_imi2paganini_imi
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X1E, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm42cm4
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0C, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_core2cm4_core
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X02, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_systick2cm4_systick
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X06, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_tck2tck_int
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X08, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_tsvalueb2cm4_tsvalueb
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0A, 0x00, VOC_VAL_0, VOC_VAL_0);

		_mtk_mailbox_pm_imi_status(g_voc_mailbox, 0);
	} else if (status == VOC_POWER_MODE_SUSPEND) {
		while(!(INREG16((g_voc_mailbox->coin_cpu)|(OFFSET(VOC_REG_0X20))) & BIT0));

		//reg_sw_en_imi2paganini_imi
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X1E, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm42cm4
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0C, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_core2cm4_core
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X02, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_systick2cm4_systick
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X06, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_tck2tck_int
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X08, 0x00, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_tsvalueb2cm4_tsvalueb
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0A, 0x00, VOC_VAL_0, VOC_VAL_0);

		_mtk_mailbox_pm_imi_status(g_voc_mailbox, 0);
	} else if (status == VOC_POWER_MODE_RESUME) {
		_mtk_mailbox_pm_imi_status(g_voc_mailbox, 1);

		//reg_sw_en_imi2paganini_imi
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X1E, 0x01, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm42cm4
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0C, 0x01, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_core2cm4_core
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X02, 0x01, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_systick2cm4_systick
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X06, 0x01, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_tck2tck_int
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X08, 0x01, VOC_VAL_0, VOC_VAL_0);
		//reg_sw_en_cm4_tsvalueb2cm4_tsvalueb
		voc_common_clock_set(VOC_CLKGEN1, VOC_REG_0X0A, 0x01, VOC_VAL_0, VOC_VAL_0);
	}

	return 0;
}

int voc_drv_mailbox_notify_status(int32_t status)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_STATUS_UPDATE;
	((struct notify_status *)open_msg.data)->status = status;

	/* execute MAILBOX command */
	if (status == 1)
		__mtk_mailbox_resume(g_voc_mailbox);

	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_IGNORE_ACK);
	if (ret)
		return ret;

	if (status == 0)
		__mtk_mailbox_suspend(g_voc_mailbox);

	g_voc_mailbox->resume_status = status;
	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_dma_get_reset_status(void)
{
	int ret = 0;
	struct voice_mbox_msg open_msg = {
		.cmd = MBOX_CMD_OP,
		.data = {0},
	};

	/* sanity check */
	if (!g_voc_mailbox) {
		voc_dev_err(NULL, "mailbox dev is NULL\n");
		return -EINVAL;
	}

	if (g_voc_mailbox->resume_status == VOICE_STATUS_SUSPEND) {
		voc_dev_err(g_voc_mailbox->dev, "%s not resume status\n", __func__);
		return -EFAULT;
	}

	voc_dev_dbg(g_voc_mailbox->dev, "%s\n", __func__);

	open_msg.cmd = VOICE_CMD_DATA_MOVING_STATUS;

	/* execute MAILBOX command */
	ret = g_voc_mailbox->txn(g_voc_mailbox, &open_msg, VOICE_MBOX_NEED_ACK);
	if (ret)
		return ret;

	return g_voc_mailbox->txn_result;
}

int voc_drv_mailbox_vd_task_state(void)
{
	return 0;
}

void *voc_drv_mailbox_get_voice_packet_addr(void)
{
	return (void *)&g_voc_mailbox->receive_vd;
}

void voc_drv_mailbox_init(void)
{
	struct mtk_voc_mailbox *voc_mailbox;
	struct device_node *np;
	struct platform_device *pdev;
	phys_addr_t pm_sleep;
	phys_addr_t pm_misc;
	phys_addr_t cpu_int;
        phys_addr_t coin_cpu;
	phys_addr_t pm_imi;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dma");
	if (!np) {
		voc_dev_err(NULL, "Not found device node (%s)\n", VOICE_DEVICE_NODE);
		return;
	}

        pm_sleep = (phys_addr_t)of_iomap(np, VOC_VAL_1); //0x101
	pm_misc  = (phys_addr_t)of_iomap(np, VOC_VAL_3); //0x109
	cpu_int = (phys_addr_t)of_iomap(np, VOC_VAL_4); //0x3100
	coin_cpu = (phys_addr_t)of_iomap(np, VOC_VAL_15); //0x439
	pm_imi = (phys_addr_t)of_iomap(np, VOC_VAL_17); //0x438
	of_node_put(np);
	if (pm_sleep == 0 || pm_imi == 0 || pm_misc == 0 ||
		cpu_int == 0 || coin_cpu == 0) {
		voc_dev_err(NULL, "Not found register setting\n");
		return;
	}

	np = of_find_compatible_node(NULL, NULL, VOICE_DEVICE_NODE);
	if (!np) {
		voc_dev_err(NULL, "Not found device node (%s)\n", VOICE_DEVICE_NODE);
		return;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		voc_dev_err(NULL, "Not found platform device\n");
		return;
	}

	voc_dev_info(NULL, "[%s]\n", __func__);

	voc_mailbox = devm_kzalloc(&pdev->dev, sizeof(struct mtk_voc_mailbox), GFP_KERNEL);
	if (!voc_mailbox)
		return;

	// init paganini register
	voc_mailbox->pm_sleep = pm_sleep;
	voc_mailbox->pm_misc = pm_misc;
	voc_mailbox->pm_imi = pm_imi;
	voc_mailbox->cpu_int = cpu_int;
	voc_mailbox->coin_cpu = coin_cpu;

	// init mailbox channel
	memset(&voc_mailbox->mbox_client, 0, sizeof(struct mbox_client));
	voc_mailbox->mbox_client.dev = &pdev->dev;
	voc_mailbox->mbox_client.tx_block = true;
	voc_mailbox->mbox_client.tx_tout = MAILBOX_TIMEOUT_MS;
	voc_mailbox->mbox_client.knows_txdone = false;
	voc_mailbox->mbox_client.rx_callback = mtk_voc_mailbox_callback;
	voc_mailbox->mbox_client.tx_prepare = NULL;
	voc_mailbox->mbox_client.tx_done = NULL;

	voc_mailbox->mchan = mbox_request_channel(&voc_mailbox->mbox_client, 0);

	if (IS_ERR(voc_mailbox->mchan)) {
		voc_dev_err(&pdev->dev, "IPC Request for CPU to Voice Channel failed! %ld\n",
			PTR_ERR(voc_mailbox->mchan));
		return;
	}

	// assign the member
	voc_mailbox->dev = &pdev->dev;
	init_completion(&voc_mailbox->ack);
	init_completion(&voc_mailbox->time_ack);
	voc_mailbox->txn = mtk_voc_mailbox_txn;

	// Init timer kthread
	voc_mailbox->notify = 0;
	voc_mailbox->time_notify = 0;
	init_waitqueue_head(&voc_mailbox->kthread_waitq);

	// add to mailbox_dev list
	INIT_LIST_HEAD(&voc_mailbox->list);
	list_add_tail(&voc_mailbox->list, &mailbox_dev_list);
}

struct voc_communication_operater *voc_drv_mailbox_get_op(void)
{
	return &voc_mailbox_op;
}
EXPORT_SYMBOL_GPL(voc_drv_mailbox_get_op);

static struct voc_communication_operater voc_mailbox_op = {
	.bind = voc_drv_mailbox_bind,
	.dma_start_channel = voc_drv_mailbox_dma_start_channel,
	.dma_stop_channel = voc_drv_mailbox_dma_stop_channel,
	.dma_init_channel = voc_drv_mailbox_dma_init_channel,
	.dma_enable_sinegen =  voc_drv_mailbox_dma_enable_sinegen,
	.dma_get_reset_status = voc_drv_mailbox_dma_get_reset_status,
	.enable_vq = voc_drv_mailbox_enable_vq,
	.config_vq = voc_drv_mailbox_config_vq,
	.enable_hpf = voc_drv_mailbox_enable_hpf,
	.config_hpf = voc_drv_mailbox_config_hpf,
	.dmic_gain = voc_drv_mailbox_dmic_gain,
	.dmic_mute = voc_drv_mailbox_dmic_mute,
	.dmic_switch = voc_drv_mailbox_dmic_switch,
	.reset_voice = voc_drv_mailbox_reset_voice,
	.update_snd_model = voc_drv_mailbox_update_snd_model,
	.get_hotword_ver = voc_drv_mailbox_get_hotword_ver,
	.get_hotword_identifier = voc_drv_mailbox_get_hotword_identifier,
	.enable_slt_test = voc_drv_mailbox_enable_slt_test,
	.enable_seamless = voc_drv_mailbox_enable_seamless,
	.notify_status = voc_drv_mailbox_notify_status,
	.power_status = voc_drv_mailbox_power_status,
	.ts_sync_start = voc_drv_mailbox_ts_sync_start,
	.get_voice_packet_addr = voc_drv_mailbox_get_voice_packet_addr,
};

