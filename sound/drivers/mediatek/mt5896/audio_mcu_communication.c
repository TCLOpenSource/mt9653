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
uint8_t performance_device = AUDIO_MCU_DEVICE_ADEC1;
uint16_t g_communication_config = COM_CONFIG_EXTERNAL_RPMSG;
struct mtk_audio_mcu_communication g_audio_mcu_communication;

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------

int audio_mcu_communication_cmd_adec1(struct audio_metadata_struct *packet,
			struct audio_mcu_device_config *device)
{
	struct communication_struct com_packet = {
		.com_cmd = AUDIO_MCU_CMD_ADEC1,
		.com_size = sizeof(struct audio_metadata_struct),
		.com_data = packet,
	};
	int ret = 0;

	/* execute RPMSG command */
	ret = mtk_audio_mcu_rpmsg_ipcm_cmd_send(&com_packet,
		device, sizeof(struct communication_struct));

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_cmd_adec1);

int audio_mcu_communication_cmd_asnd(struct audio_metadata_struct *packet,
			struct audio_mcu_device_config *device)
{
	struct communication_struct com_packet = {
		.com_cmd = AUDIO_MCU_CMD_ASND,
		.com_size = sizeof(struct audio_metadata_struct),
		.com_data = packet,
	};
	int ret = 0;

	/* execute RPMSG command */
	ret = mtk_audio_mcu_rpmsg_ipcm_cmd_send(&com_packet,
		device, sizeof(struct communication_struct));

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_cmd_asnd);

int audio_mcu_communication_cmd_parameter(struct audio_parameter_config *packet,
			struct audio_mcu_device_config *device)
{
	struct communication_struct com_packet = {
		.com_cmd = AUDIO_MCU_CMD_PARAMETER,
		.com_size = sizeof(struct audio_parameter_config),
		.com_data = packet,
	};
	int ret = 0;

	/* execute RPMSG command */
	ret = mtk_audio_mcu_rpmsg_ipcm_cmd_send_ack(&com_packet,
		device, sizeof(struct communication_struct));

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_cmd_parameter);

int audio_mcu_communication_cmd_shm_buffer(struct audio_shm_buffer_config *packet,
			struct audio_mcu_device_config *device)
{
	struct communication_struct com_packet = {
		.com_cmd = AUDIO_MCU_CMD_SHM_BUFFER_CONFIG,
		.com_size = sizeof(struct audio_shm_buffer_config),
		.com_data = packet,
	};
	int ret = 0;

	/* execute RPMSG command */
	ret = mtk_audio_mcu_rpmsg_ipcm_cmd_send(&com_packet,
		device, sizeof(struct communication_struct));

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_cmd_shm_buffer);

int audio_mcu_communication_init(void)
{
	int ret = 0;
	// RPMSG init
	ret = mtk_audio_mcu_rpmsg_init(&audio_mcu_communication_rx_handler);

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_init);

int audio_mcu_communication_register_notify(struct mtk_audio_mcu_communication *fp)
{
	int ret = 0;

	g_audio_mcu_communication.audio_parameter_cb = fp->audio_parameter_cb;
	g_audio_mcu_communication.audio_shm_buffer_cb = fp->audio_shm_buffer_cb;
	// g_audio_mcu_communication.audio_process_cb = fp->audio_process_cb;

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_register_notify);

int audio_mcu_communication_reset(uint8_t device)
{
	int ret = 0;

	mtk_audio_mcu_rpmsg_reset(device);

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_reset);

int audio_mcu_communication_set_config(uint16_t config)
{
	int ret = 0;
	// Communication config
	// 0:external rpmsg
	// 1:internal
	// 2:stub
	g_communication_config = config;

	return ret;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_set_config);

int audio_mcu_communication_get_config(void)
{
	return g_communication_config;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_get_config);

int audio_mcu_communication_set_performance(uint8_t device)
{
	performance_device = device;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_set_performance);

int audio_mcu_communication_get_performance(void)
{
	return mtk_audio_mcu_rpmsg_performance(performance_device);
}
EXPORT_SYMBOL_GPL(audio_mcu_communication_get_performance);

int audio_mcu_communication_rx_handler(struct communication_struct *packet,
			struct audio_mcu_device_config *device)
{
	int ret = 0;

	switch (packet->com_cmd) {
	case AUDIO_MCU_CMD_ADEC1: {
		struct audio_metadata_struct config;
		uint32_t *va_addr = NULL;
		uint32_t pa_addr = 0x0;
		uint16_t size = 0x0;

		/* Get RPMSG packet */
		memcpy(&config, packet->com_data, sizeof(struct audio_metadata_struct));

		// update to global value
		pa_addr = config.audio_meta_value3;
		size = config.audio_meta_value2;

		va_addr = memremap(pa_addr, size, MEMREMAP_WB);

		audio_info("[%s][%d]\n", __func__, __LINE__);
		audio_info("value 0x%x\n", config.audio_meta_value1);
		audio_info("value 0x%x\n", config.audio_meta_value2);
		audio_info("value 0x%x\n", config.audio_meta_value3);
		audio_info("buf pa: 0x%x va: 0x%p size: 0x%x\n", pa_addr, va_addr, size);

		print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET,
			DEBUG_GROUP_SIZE, 1, va_addr, DEBUG_DUMP_LEN, true);
		break;
	}
	case AUDIO_MCU_CMD_ASND: {
		struct audio_metadata_struct config;

		/* Get RPMSG packet */
		memcpy(&config, packet->com_data, sizeof(struct audio_metadata_struct));

		audio_info("[%s][%d]\n", __func__, __LINE__);
		audio_info("value 0x%x\n", config.audio_meta_value1);
		audio_info("value 0x%x\n", config.audio_meta_value2);
		audio_info("value 0x%x\n", config.audio_meta_value3);

		break;
		}
	case AUDIO_MCU_CMD_PARAMETER: {
		struct audio_parameter_config config;

		/* Get RPMSG packet */
		memcpy(&config, packet->com_data, sizeof(struct audio_parameter_config));

		audio_info("[%s][%d]\n", __func__, __LINE__);
		audio_info("value 0x%x\n", config.pa_addr);
		audio_info("value 0x%x\n", config.size);

		g_audio_mcu_communication.audio_parameter_cb(&config, device);
		break;
		}
	case AUDIO_MCU_CMD_SHM_BUFFER_CONFIG: {
		// MCU won't send it cmd.
		audio_info("[%s][%d]\n", __func__, __LINE__);

		// g_audio_mcu_communication.audio_shm_buffer_cb();
		break;
		}
	default:
		audio_info("[%s][%d]\n", __func__, __LINE__);
		break;
	}

	return ret;
}
