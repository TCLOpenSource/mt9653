/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _AUDIO_MCU_RPMSG_H
#define _AUDIO_MCU_RPMSG_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define IPCM_MAX_DATA_SIZE	64
#define AUDIO_MCU_MAX_DATA_SIZE	(IPCM_MAX_DATA_SIZE - sizeof(uint8_t))

#define AUDIO_MCU_DEC_EPT_NAME "audio-mcu-dec-ept"
#define AUDIO_MCU_SND_EPT_NAME "audio-mcu-snd-ept"

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Enumerate
//------------------------------------------------------------------------------
enum ipcm_ack_t {
	IPCM_CMD_OP = 0,
	IPCM_CMD_BAD_ACK,
	IPCM_CMD_ACK,
	IPCM_CMD_MSG,
};

//------------------------------------------------------------------------------
// Structure
//------------------------------------------------------------------------------
struct ipcm_msg {
	uint8_t cmd;
	uint32_t size;
	uint8_t data[IPCM_MAX_DATA_SIZE];
};

struct ipcm_msg_packet {
	uint8_t cmd;
	uint8_t payload[AUDIO_MCU_MAX_DATA_SIZE];
};

struct audio_no_data_res_pkt {
	uint8_t cmd;
};

struct audio_mcu_rpmsg_packet {
	uint8_t rpmsg_device;
	uint8_t rpmsg_status;
	uint32_t rpmsg_ts;
};

typedef int (*AUD_MCU_RPMSG_RX_HANDLER_FP)(struct communication_struct *packet,
	struct audio_mcu_device_config *device);

struct mtk_audio_mcu_rpmsg {
	struct device *dev;
	struct rpmsg_device *rpdev;
	struct list_head list;
	int txn_result;
	struct ipcm_msg receive_msg;
	struct completion ack[AUDIO_MCU_DEVICE_NUM_MAX];
	AUD_MCU_RPMSG_RX_HANDLER_FP rx_callback;
	uint32_t ts;
};

//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
uint32_t mtk_audio_mcu_rpmsg_reset(uint8_t device);
uint32_t mtk_audio_mcu_rpmsg_performance(uint8_t device);
int mtk_audio_mcu_rpmsg_init(AUD_MCU_RPMSG_RX_HANDLER_FP audio_mcu_rx_handler);
int mtk_audio_mcu_rpmsg_ipcm_cmd_send(struct communication_struct *rpmsg_packet,
	struct audio_mcu_device_config *rpmsg_device, int size);
int mtk_audio_mcu_rpmsg_ipcm_cmd_send_ack(struct communication_struct *rpmsg_packet,
	struct audio_mcu_device_config *rpmsg_device, int size);

#endif /* _AUDIO_MCU_RPMSG_H */
