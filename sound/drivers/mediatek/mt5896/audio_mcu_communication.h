/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _AUDIO_MCU_COMMUNICATION_H
#define _AUDIO_MCU_COMMUNICATION_H
//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define MAX_LEN	512
#define MAX_LOG_LEN	30

#define DEVICE_NAME_LEN	64

#define DEBUG_GROUP_SIZE 16
#define DEBUG_DUMP_LEN 0x80

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Enumerate
//------------------------------------------------------------------------------
enum communication_config {
	COM_CONFIG_EXTERNAL_RPMSG = 0,
	COM_CONFIG_INTERNAL,
	COM_CONFIG_STUB,
	COM_CONFIG_MAX,
};

enum audio_mcu_buffer_type {
	AUDIO_MCU_INPUT_BUFFER = 0,
	AUDIO_MCU_OUTPUT_BUFFER,
};

enum audio_mcu_cmd {
	AUDIO_MCU_CMD_ADEC1 = 0,
	AUDIO_MCU_CMD_ASND,
	AUDIO_MCU_CMD_PARAMETER,
	AUDIO_MCU_CMD_SHM_BUFFER_CONFIG,
	AUDIO_MCU_CMD_PROCESS,
	AUDIO_MCU_CMD_MAX,
};

enum audio_mcu_device {
	AUDIO_MCU_DEVICE_ADEC1 = 0,
	AUDIO_MCU_DEVICE_ADEC2,
	AUDIO_MCU_DEVICE_ASND,
	AUDIO_MCU_DEVICE_ADVS,
	AUDIO_MCU_DEVICE_AENC,
	AUDIO_MCU_DEVICE_NUM_MAX,
};

enum audio_mcu_client {
	AUDIO_MCU_CLIENT_AUDEC = 0,
	AUDIO_MCU_CLIENT_AUSND,
	AUDIO_MCU_CLIENT_NUM_MAX,
};

//------------------------------------------------------------------------------
// Structure
//------------------------------------------------------------------------------
struct operation_log {
	struct timespec tstamp;
	char event[MAX_LEN];
};
struct audio_dma_dbg {
	struct operation_log log[MAX_LOG_LEN];
	uint32_t writed;
};

struct audio_mcu_buffer_info {
	uint32_t in_buf_addr;
	uint32_t in_buf_size;
	uint32_t in_buf_max_size;
	uint32_t in_buf_offset;
	uint32_t parm_addr;
	uint32_t parm_size;
};

struct audio_card_drvdata {
	u32 audio_buf_paddr;
	u32 audio_buf_size;
	struct audio_mcu_buffer_info device_buf_info[AUDIO_MCU_DEVICE_NUM_MAX];
	u32 *card_reg;
};

struct mtk_snd_audio {
	struct device *dev;
	struct snd_card *card;
	struct snd_pcm *pcm[AUDIO_MCU_DEVICE_NUM_MAX];
	struct list_head list;
	struct audio_card_drvdata *audio_card;
	struct audio_dma_dbg dbgInfo;
	struct mutex pcm_lock;
	uint32_t sysfs_test_config;
	bool audio_pcm_running;
};

struct communication_struct {
	uint8_t com_cmd;
	uint32_t com_size;
	void *com_data;
};

struct audio_metadata_struct {
	uint8_t audio_mcu_device_id;
	uint8_t	audio_meta_value1;
	uint16_t audio_meta_value2;
	uint32_t audio_meta_value3;
};

struct audio_parameter_config {
	uint8_t mcu_device_id;
	uint32_t pa_addr;
	uint32_t size;
};

struct audio_shm_buffer_config {
	uint8_t mcu_device_id;
	uint32_t pa_addr;
	uint32_t size;
	uint8_t type;
};

struct audio_process_config {
	uint8_t mcu_device_id;
	bool done;
	uint32_t size;
};

struct audio_mcu_device_config {
	uint8_t device;
	char device_name[DEVICE_NAME_LEN];
};

typedef int (*AUD_MCU_MSG_PARAMETER_FP)
	(struct audio_parameter_config *audio_packet,
		struct audio_mcu_device_config *audio_device);
typedef int (*AUD_MCU_MSG_SHM_BUFFER_FP)
	(struct audio_shm_buffer_config *audio_packet,
		struct audio_mcu_device_config *audio_device);

struct mtk_audio_mcu_communication {
	AUD_MCU_MSG_PARAMETER_FP audio_parameter_cb;
	AUD_MCU_MSG_SHM_BUFFER_FP audio_shm_buffer_cb;
};

//------------------------------------------------------------------------------
// Global Function
//------------------------------------------------------------------------------
int audio_mcu_communication_cmd_adec1(struct audio_metadata_struct *packet,
	struct audio_mcu_device_config *device);
int audio_mcu_communication_cmd_asnd(struct audio_metadata_struct *packet,
	struct audio_mcu_device_config *device);
int audio_mcu_communication_cmd_parameter(struct audio_parameter_config *packet,
	struct audio_mcu_device_config *device);
int audio_mcu_communication_cmd_shm_buffer(struct audio_shm_buffer_config *packet,
	struct audio_mcu_device_config *device);
int audio_mcu_communication_cmd_process(struct audio_process_config *packet,
	struct audio_mcu_device_config *device);

int audio_mcu_communication_init(void);
int audio_mcu_communication_register_notify(struct mtk_audio_mcu_communication *fp);
int audio_mcu_communication_reset(uint8_t device);

int audio_mcu_communication_set_config(uint16_t config);
int audio_mcu_communication_get_config(void);

int audio_mcu_communication_set_performance(uint8_t device);
int audio_mcu_communication_get_performance(void);

int audio_mcu_communication_rx_handler(struct communication_struct *packet,
	struct audio_mcu_device_config *device);

#endif /* _AUDIO_MCU_COMMUNICATION_H */
