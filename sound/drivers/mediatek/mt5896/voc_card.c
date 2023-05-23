// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <sound/soc.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <sound/initval.h>
#include <asm-generic/div64.h>
#include "mtk-reserved-memory.h"

#include "voc_common.h"
#include "voc_common_reg.h"
#include "voc_communication.h"
#include "voc_vd_task.h"
#include "voc_drv_rpmsg.h"
#include "voc_drv_stub.h"
#include "vbdma.h"
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
#define ENABLE_FORCE_STUB_MODE      (0)
#define ENABLE_OWL_MODE             (0)

#define SND_VOC_DRIVER				"voc-card"
#define SND_PCM_NAME				"voc-platform"
#define MAX_PCM_PLAYBACK_SUBSTREAMS		(0)
#define MAX_PCM_CAPTURE_SUBSTREAMS		(1)
#define DEFAULT_SUBSTREAMS			(8)
#define DEFAULT_CHANNELS			(2)
#define DEFAULT_BITWIDTHS			(16)
#define DEFAULT_RATES				(16000)
#define SOUND_TRIGGER_UUID_0        (0xed7a7d60)
#define STR2LONG_BASE_DECIMAL       (10)
#define VOC_MAGIC_02                (2)
#define SOC_ENUM_VOC(xname, xenum) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_enum_double, \
	.get = voc_card_get_enum, .put = voc_card_put_enum, \
	.private_value = (unsigned long)&xenum }

/* sound card controls */
// TODO: apply dts
#define TLV_HEADER_SIZE             (2 * sizeof(unsigned int))
#define SND_MODEL_DATA_SIZE         (102400)
#define SND_MODEL_SIZE              (SND_MODEL_DATA_SIZE + TLV_HEADER_SIZE)
#define DSP_PLATFORM_ID_DATA_SIZE   (37)
#define DSP_PLATFORM_ID_SIZE        (DSP_PLATFORM_ID_DATA_SIZE + TLV_HEADER_SIZE)

/*SLT temp solution:
 * Run SLT test funciton and talk to PMU voice driver by register instead of RPMsg communication
 * Inorder to avoid RPMsg communication fail, we use register to communication with PMU voice.
 * VAD DD trigger enable bit and send test address information by writing to the dummy register.
 * And then, PMU voice driver is polling and run test if the value is correct in dummy register.
 * PMU voice driver finish the test and write the result in the dummy register.
 */
//#define SLT_TEMP
#ifdef SLT_TEMP
#define GET_REG8_ADDR(x, y)         (x+(y)*2-((y)&1))
#define GET_REG16_ADDR(x, y)        (x+(y)*4)
#define RIU_BASE_ADDR                riu_base
#define IMI_TOP_BASE_ADDR            imi_base
#define REG_ADDR_BASE_IMI_TOP        GET_REG8_ADDR(RIU_BASE_ADDR, IMI_TOP_BASE_ADDR)
#define IMI_TOP_SET_REG(x)           GET_REG16_ADDR(REG_ADDR_BASE_IMI_TOP, x)
#define IMI_TOP_REG_BURST_0          IMI_TOP_SET_REG(0x10) //ENABLE
#define IMI_TOP_REG_BURST_1          IMI_TOP_SET_REG(0x11) //ADDR_LO
#define IMI_TOP_REG_BURST_2          IMI_TOP_SET_REG(0x12) //ADDR_HI
#define IMI_TOP_REG_BURST_3          IMI_TOP_SET_REG(0x13) //RESULT
#endif

#define SINE_TONE_SIZE (16)
#define WAIT_TIME_MS   (1)
#define WAIT_TIMEOUT   (100)

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
//ALSA card related
unsigned long mtk_log_level = LOGLEVEL_ERR;
//unsigned long mtk_log_level = LOGLEVEL_INFO;
#if ENABLE_FORCE_STUB_MODE
static int g_force_stub_mode;
#endif
static unsigned int g_pcm_muting;
static unsigned int g_pcm_resume;
static unsigned int g_pcm_xrun;
static uint8_t g_tlv_data[DSP_PLATFORM_ID_SIZE];
static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static bool enable[SNDRV_CARDS] = {1, [1 ... (SNDRV_CARDS - 1)] = 0};
static int pcm_capture_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = DEFAULT_SUBSTREAMS};
static int pcm_playback_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = DEFAULT_SUBSTREAMS};
static struct platform_device *devices[SNDRV_CARDS];
module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for vod soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for voc soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable this voc soundcard.");
module_param_array(pcm_capture_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_capture_substreams, "PCM capture substreams # (1) for voc driver.");
module_param_array(pcm_playback_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_playback_substreams, "PCM playback substreams # (1) for voc driver.");

//Mixer related
#ifdef SLT_TEMP
// TODO: we will put variables into card_drvdata
static void __iomem *riu_base;
static u32 imi_base;
#endif



// TODO: move these two global variables into voc_soc_card_drvdata
// instead of using these directly
#ifdef CONFIG_PM_SLEEP
static unsigned int card_reg_backup[VOC_REG_LEN] = {0};
#endif
static unsigned int card_reg[VOC_REG_LEN] = {
	0x0,			//VOC_REG_MIC_ONOFF
	0x0,			//VOC_REG_VQ_ONOFF
	0x0,			//VOC_REG_SEAMLESS_ONOFF
	0x0,			//VOC_REG_MIC_GAIN
	0x2,			//VOC_REG_HPF_ONOFF
	0x6,			//VOC_REG_HPF_CONFIG
	0x0,			//VOC_REG_SIGEN_ONOFF
	0x0,			//VOC_REG_SOUND_MODEL
	0x0,			//VOC_REG_HOTWORD_VER
	0x0,			//VOC_REG_DSP_PLATFORM_ID
	0x0			//VOC_REG_MUTE_SWITCH
};

static const char * const mic_onoff_text[] = { "Off", "On" };
static const char * const vq_config_text[] = { "Off", "Normal", "PM", "Both" };
static const char * const seamless_onoff_text[] = { "Off", "On" };
static const char * const hpf_onoff_text[] = { "Off", "1-stage", "2-stage" };
static const char * const hpf_config_text[] = {
	"-2", "-1", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static const char * const sigen_onoff_text[] = { "Off", "On" };
static const char * const mute_switch_text[] = { "Off", "On" };

static const struct soc_enum mic_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_MIC_ONOFF, 0, 2, mic_onoff_text);

static const struct soc_enum vq_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_VQ_ONOFF, 0, 4, vq_config_text);

static const struct soc_enum seamless_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_SEAMLESS_ONOFF, 0, 2, seamless_onoff_text);

static const struct soc_enum hpf_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_HPF_ONOFF, 0, 3, hpf_onoff_text);

static const struct soc_enum hpf_config_enum =
SOC_ENUM_SINGLE(VOC_REG_HPF_CONFIG, 0, 12, hpf_config_text);

static const struct soc_enum sigen_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_SIGEN_ONOFF, 0, 2, sigen_onoff_text);

static const struct soc_enum mute_switch_enum =
SOC_ENUM_SINGLE(VOC_REG_MUTE_SWITCH, 0, 2, mute_switch_text);

//PCM related
static const struct snd_pcm_hardware voc_pcm_capture_hardware = {
	.info =
#if defined(CONFIG_MS_VOC_MMAP_SUPPORT)
		SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
#endif
		SNDRV_PCM_INFO_INTERLEAVED,

	.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
		SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000,
	.rate_min = 8000,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = 1512 * 1024,
	.period_bytes_min = 1 * 1024,
	.period_bytes_max = 64 * 1024,
	.periods_min = 4,
	.periods_max = 1512,	//32,
	.fifo_size = 0,
};

static u64 voc_pcm_dmamask = DMA_BIT_MASK(32);
static struct voc_pcm_dma_data voc_pcm_dma_wr = {
	.name = "DMA writer",
	.channel = 0,
};

static const short sine_tone[SINE_TONE_SIZE] = {
	0, 1254, 2317, 3027,
	3277, 3027, 2317, 1254,
	0, -1254, -2317, -3027,
	-3277, -3027, -2317, -1254};

//------------------------------------------------------------------------------
//  Function
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Log Function
//------------------------------------------------------------------------------
static void voc_card_shutdown(struct platform_device *pdev)
{
	voc_info("%s, shutdown\n", __func__);
	if (card_reg[VOC_REG_VQ_ONOFF] == VOC_WAKEUP_MODE_OFF ||
		card_reg[VOC_REG_VQ_ONOFF] == VOC_WAKEUP_MODE_NORMAL) {
		voc_communication_dmic_switch(0x0);
	}

	voc_communication_notify_status(0x0);
	if (card_reg[VOC_REG_VQ_ONOFF] == VOC_WAKEUP_MODE_OFF ||
		card_reg[VOC_REG_VQ_ONOFF] == VOC_WAKEUP_MODE_NORMAL) {
		voc_communication_power_status(0x0);
	}
}

#ifdef CONFIG_PM_SLEEP
static int voc_audio_dapm_suspend(struct device *dev)
{
	//struct snd_card *card = dev_get_drvdata(dev);
	//struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;

	//voc_dev_info(dev, "%s, record pmu voice setting\r\n", __func__);
	memcpy(card_reg_backup, card_reg, VOC_REG_LEN);
	voc_communication_notify_status(0x0);
	return 0;
}

static int voc_audio_dapm_resume(struct device *dev)
{
	//struct snd_card *card = dev_get_drvdata(dev);
	//struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;

	//voc_dev_info(dev, "%s\n", __func__);
	g_pcm_resume = 1;
	voc_communication_notify_status(0x1);
	// TODO: TSTAMP PTP SYNC
	//voc_communication_ts_sync_start();
	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops voc_card_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(voc_audio_dapm_suspend, voc_audio_dapm_resume)
};
#endif
///sys/devices/platform/voc-card.0/mtk_dbg/event_log
static ssize_t event_log_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	int count = 0, ret = 0;
	int i = 0;

	for (i = 0; i < voc->dbgInfo.writed; i++) {
		if (strlen(voc->dbgInfo.log[i].event) != 0) {
			ret = snprintf(buf+count, MAX_LEN, "%d:%lld.%.9ld, %s\n", i,
				(long long)voc->dbgInfo.log[i].tstamp.tv_sec,
				voc->dbgInfo.log[i].tstamp.tv_nsec,
				voc->dbgInfo.log[i].event);
			if (ret > 0)
				count += ret;
		}
	}
	return count;
}

static ssize_t event_log_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	unsigned long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;
	if (item > 0) { //clear log
		voc->dbgInfo.writed = 0;
		memset(voc->dbgInfo.log, 0, sizeof(struct operation_log) * MAX_LOG_LEN);
	}
	return count;
}
static DEVICE_ATTR_RW(event_log);

///sys/devices/platform/voc-card.0/mtk_dbg/notify_test
static ssize_t notify_test_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;

	return scnprintf(buf, MAX_LEN, "%u\n", voc->sysfs_notify_test);
}

static ssize_t notify_test_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	unsigned long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;
	if (item >= 0)
		voc->sysfs_notify_test = item;
	return count;

}
static DEVICE_ATTR_RW(notify_test);

///sys/devices/platform/voc-card.0/self_Diagnostic
static ssize_t self_Diagnostic_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int ret = 0;

#ifdef SLT_TEMP
	u16 result;

	result = readw(IMI_TOP_REG_BURST_3);
	if (result == 0x900d)
		ret = 1;
	else if (result == 0xdead)
		ret = 0;
	else
		voc_dev_info(dev, "%s status :%d", __func__, ret);
#else
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;

	voc_communication_enable_slt_test(0, 0);
	ret = voc->voc_card->status;// 1: pass, otherwise: fail
	voc_dev_info(dev, "%s status :%d", __func__, ret);
#endif
	return scnprintf(buf, MAX_LEN, "%s\n", (ret)?"PASS":"FAIL");
}

static ssize_t self_Diagnostic_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	struct snd_pcm_substream *substream =
		(struct snd_pcm_substream *)voc->pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	long type = 0;
	int ret = 0;
	uint32_t addr = voc->voc_card->voc_buf_paddr
					+ voc->voc_card->snd_model_offset;

	voc_dev_dbg(dev, "%s", __func__);

	ret = kstrtol(buf, 10, &type);
	if (ret != 0)
		return ret;

#ifdef SLT_TEMP
	if (type > 0) {
		writew(type, IMI_TOP_REG_BURST_0);
		writew(addr >> 16, IMI_TOP_REG_BURST_1); //ADDR_HI
		writew(addr, IMI_TOP_REG_BURST_2); //ADDR_LO
	} else
		voc_dev_err(dev, "type is not support %ld\n", type);
#else
	//To avoid dma(vbdma) write to invalid address
	if (voc->voc_pcm == NULL && substream != NULL) {
		voc_communication_dma_init_channel(
			(uint64_t)substream->dma_buffer.addr,
			substream->dma_buffer.bytes,
			DEFAULT_CHANNELS,
			DEFAULT_BITWIDTHS,
			DEFAULT_RATES);
	}

	if (type > 0) {
		voc_communication_enable_slt_test(type, addr);
	} else if (type == 0) {
		voc_communication_enable_slt_test(type, 0);
		ret = voc->voc_card->status;// 1: pass, otherwise: fail
		voc_dev_info(dev, "%s\n", (ret)?"PASS":"FAIL");
	} else
		voc_dev_err(dev, "type is not support %ld\n", type);
#endif
	return count;
}
static DEVICE_ATTR_RW(self_Diagnostic);

///sys/devices/platform/voc-card.0/mtk_dbg/log_level
static ssize_t log_level_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, MAX_LEN, "%lu\n", mtk_log_level);
}

static ssize_t log_level_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned long level = 0;
	int ret;

	ret = kstrtoul(buf, 10, &level);

	if (ret != 0)
		return ret;
	if (level >= 0 && level <= 7)
		mtk_log_level = level;
	return count;

}
static DEVICE_ATTR_RW(log_level);

/*
 * Purpose: get currect hotword mode
 * HOW to test: reference usage
 * how to usage :
 * get currect status
 * (e.g. cat sys/devices/platform/voc-card.0/mtk_dbg/auto_kws_detect_test)
 */
//sys/devices/platform/voc-card.0/mtk_dbg/auto_kws_detect_test
static ssize_t auto_kws_detect_test_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;

	return scnprintf(buf, MAX_LEN, "KWS current mode: %u\n",
			voc->voc_card->card_reg[VOC_REG_VQ_ONOFF]);
}
/*
 * Purpose: use this function to verify the software process of howword detect
 * HOW to test: launch a thread to wait event through tinyalsa api, other thread
 * trigger auto_kws_detect test (ref hot to usage)
 * the pmu voice will send hotword events every second, a total of 5 times.
 * how to usage :
 * enable
 * (e.g. echo "1" > sys/devices/platform/voc-card.0/mtk_dbg/auto_kws_detect_test)
 * disable
 * (e.g. echo "0" > sys/devices/platform/voc-card.0/mtk_dbg/auto_kws_detect_test)
 */
static ssize_t auto_kws_detect_test_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	unsigned long para = 0;
	unsigned int  value = 0;
	int ret;

	ret = kstrtoul(buf, 10, &para);
	if (ret != 0)
		return ret;
	if (para == 1) { // auto kws test
		voc_dev_err(dev, "[%d]enable kws test\n", __LINE__);
		ret = voc_communication_config_vq(3);
		ret = voc_communication_enable_vq(1);
	} else if (para == 0) { // restore
		value = voc->voc_card->card_reg[VOC_REG_VQ_ONOFF];
		voc_dev_err(dev, "[%d]restore kws mode: %d\n", __LINE__, value);
		if (value == 0) { //disable kws detect
			ret = voc_communication_enable_vq(value);
		} else if (value == 1) { //enable kws detect in normal mode
			ret = voc_communication_config_vq(0);
			ret = voc_communication_enable_vq(1);
		} else if (value == 2) {  //enable kws detect in standby mode
			ret = voc_communication_config_vq(1);
			ret = voc_communication_enable_vq(1);
		} else if (value == 3) {  //enable kws detect in dual mode (normal/standby)
			ret = voc_communication_config_vq(2);
			ret = voc_communication_enable_vq(1);
		}
	}
	return count;
}
static DEVICE_ATTR_RW(auto_kws_detect_test);
/*
 * Purpose: use this function to get result
 * HOW to test: set 1 run suspend function and get the result from show function.
 * set 2 run resume function and get the result from show function.
 * otherwise, the resule is cleared.
 * how to usage :
 * get result
 * (e.g. cat /sys/devices/platform/voc-card.0/mtk_dbg/suspend_resume_test)
 */
//sys/devices/platform/voc-card.0/mtk_dbg/suspend_resume_test
static ssize_t suspend_resume_test_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	unsigned long tmp = 0;

	if ((voc->resume_duration_time.tv_nsec != 0) ||
		(voc->resume_duration_time.tv_sec != 0)) {
		tmp = voc->resume_duration_time.tv_sec * 1000000000;
		tmp += voc->resume_duration_time.tv_nsec;
	} else if ((voc->suspend_duration_time.tv_nsec != 0) ||
			(voc->suspend_duration_time.tv_sec != 0)) {
		tmp = voc->suspend_duration_time.tv_sec * 1000000000;
		tmp += voc->suspend_duration_time.tv_nsec;
	}
	return scnprintf(buf, MAX_LEN, "%lu\n", tmp);
}
/*
 * Purpose: use this function to detect the time spent on suspend and resume
 * HOW to test: set 1 run suspend function and get the result from show function.
 * set 2 run resume function and get the result from show function.
 * otherwise, the resule is cleared.
 * how to usage :
 * run suspend
 * (e.g. echo "1" > /sys/devices/platform/voc-card.0/mtk_dbg/suspend_resume_test)
 * run resume
 * (e.g. echo "2" > /sys/devices/platform/voc-card.0/mtk_dbg/suspend_resume_test)
 * clear
 * (e.g. echo "0" > /sys/devices/platform/voc-card.0/mtk_dbg/suspend_resume_test)
 */
static ssize_t suspend_resume_test_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	struct timespec64 t64begin, t64end;
	unsigned long item = 0;
	int ret = 0;

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;

	voc->resume_duration_time.tv_sec = 0;
	voc->resume_duration_time.tv_nsec = 0;
	voc->suspend_duration_time.tv_sec = 0;
	voc->suspend_duration_time.tv_nsec = 0;
	if (item == 1) { // trigger test
		ktime_get_ts64(&t64begin);
		voc_audio_dapm_suspend(dev);
		ktime_get_ts64(&t64end);
		voc->suspend_duration_time.tv_nsec = t64end.tv_nsec - t64begin.tv_nsec;
		voc->suspend_duration_time.tv_sec = t64end.tv_sec - t64begin.tv_sec;
	} else if (item == 2) {
		ktime_get_ts64(&t64begin);
		voc_audio_dapm_resume(dev);
		ktime_get_ts64(&t64end);
		voc->resume_duration_time.tv_nsec = t64end.tv_nsec - t64begin.tv_nsec;
		voc->resume_duration_time.tv_sec = t64end.tv_sec - t64begin.tv_sec;
	}
	return count;
}
static DEVICE_ATTR_RW(suspend_resume_test);

///sys/devices/platform/voc-platform/mtk_dbg/fake_capture
static ssize_t fake_capture_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;

	return scnprintf(buf, MAX_LEN, "%u\n", voc->sysfs_fake_capture);
}

static ssize_t fake_capture_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_card *card = dev_get_drvdata(dev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;
	long item = 0;
	int ret = 0;

	#if ENABLE_FORCE_STUB_MODE
	if (g_force_stub_mode)
		return count;
	#endif

	ret = kstrtol(buf, 10, &item);
	if (ret != 0)
		return ret;
	if (item >= 0 && item <= 1)
		voc->sysfs_fake_capture = (uint32_t)item;
	else
		voc_dev_err(dev, "item is not support %ld\n", item);
	return count;
}
static DEVICE_ATTR_RW(fake_capture);

static ssize_t help_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return scnprintf(buf, MAX_LEN, "Debug Help:\n"
				"- log_level <RW>: To control debug log level.\n"
				"                  Read log_level value.\n"
				"- notify_test <RW>: send value chagne event to notify user space.\n"
				"                  The input parameter represents the value of which\n"
				"                  item has changed\n"
				"- event_log <RW>: dump voc card event (max : 30)\n"
				"                  Enter a number greater than 0 to clear the log\n"
				"- fake_capture <RW>: To enable fake capture mode.\n"
				"                  Enable: 0; Disable: 1\n"
				"- auto_kws_detect_test <RW>: To enable/disable auto kws detect test.\n"
				"                  get kws current mode.\n"
				"- suspend_resume_test <RW>: write 1 to calculation suspend time. write 2 to calculation resume time.\n"
				"                  get result.\n");
}
static DEVICE_ATTR_RO(help);
static struct attribute *mtk_dtv_voc_card_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	&dev_attr_notify_test.attr,
	&dev_attr_event_log.attr,
	&dev_attr_fake_capture.attr,
	&dev_attr_auto_kws_detect_test.attr,
	&dev_attr_suspend_resume_test.attr,
	NULL,
};

static const struct attribute_group mtk_dtv_voc_card_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_voc_card_attrs
};

static int mtk_dtv_voc_card_create_sysfs_attr(struct platform_device *pdev)
{
	int ret = 0;

	/* Create device attribute files */
	voc_dev_info(&(pdev->dev), "Create device attribute files\n");
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_dtv_voc_card_attr_group);
	if (ret)
		voc_dev_err(&pdev->dev, "[%d]Fail to create sysfs files: %d\n", __LINE__, ret);
	return ret;
}

static int mtk_dtv_voc_card_remove_sysfs_attr(struct platform_device *pdev)
{
	/* Remove device attribute files */
	voc_dev_info(&(pdev->dev), "Remove device attribute files\n");
	sysfs_remove_group(&pdev->dev.kobj, &mtk_dtv_voc_card_attr_group);
	return 0;
}

//------------------------------------------------------------------------------
//  Voc PCM Function
//------------------------------------------------------------------------------
void voc_dma_params_init(struct voc_pcm_data *pcm_data, u32 dma_buf_size, u32 period_size)
{
	pcm_data->pcm_level_cnt = 0;
	pcm_data->pcm_last_period_num = 0;
	pcm_data->pcm_period_size = period_size;//frames_to_bytes(runtime, runtime->period_size);
	pcm_data->pcm_buf_size = dma_buf_size;//runtime->dma_bytes;
	pcm_data->pcm_cm4_period = 0;
	pcm_data->pcm_status = PCM_NORMAL;
}

//void voc_dma_reset(struct voc_pcm_data *pcm_data)
//{
//	pcm_data->pcm_level_cnt = 0;
//	pcm_data->pcm_last_period_num = 0;
//}

u32 voc_dma_get_level_cnt(struct voc_pcm_data *pcm_data)
{
	return pcm_data->pcm_level_cnt;
}

u32 voc_dma_trig_level_cnt(struct voc_pcm_data *pcm_data, u32 data_size)
{
	int ret = 0;

	if (pcm_data->pcm_level_cnt >= data_size) {
		pcm_data->pcm_level_cnt -= data_size;
		ret = data_size;
	} else {
		voc_err("%s size %u too large %u\n", __func__
		, data_size, pcm_data->pcm_level_cnt);
	}
	return ret;
}

void voc_pcm_write2Log(struct mtk_snd_voc *voc, char *str)
{
	if (voc->dbgInfo.writed < MAX_LOG_LEN) {
		if (scnprintf(voc->dbgInfo.log[voc->dbgInfo.writed].event,
								MAX_LEN, "%s", str) > 0) {
			ktime_get_ts(&(voc->dbgInfo.log[voc->dbgInfo.writed].tstamp));
			voc->dbgInfo.writed++;
		} else {
			voc_err("snprintf fail, event: %s\n",
					voc->dbgInfo.log[voc->dbgInfo.writed].event);
		}
	}
}

static void voc_pcm_runtime_free(struct snd_pcm_runtime *runtime)
{
	struct voc_pcm_data *pcm_data = runtime->private_data;

	kfree(pcm_data->prtd);
	kfree(pcm_data);
}

static int voc_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_voc *voc;
	struct snd_pcm *pcm;
	struct voc_pcm_data *pcm_data = NULL;
	struct voc_pcm_runtime_data *prtd = NULL;
	int ret = 0;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm = voc->pcm;

	voc_debug("%s: stream = %s, pcmC%dD%d (substream = %s)\n",
			__func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
			pcm->card->number,
			pcm->device, substream->name
			);

	pcm_data = kzalloc(sizeof(*pcm_data), GFP_KERNEL);
	if (!pcm_data) {
		ret = -ENOMEM;
		goto out;
	}

	if (voc->sysfs_fake_capture)
		voc_drv_stub_run();

	/* Ensure that buffer size is a multiple of period size */
//#if 0
//	ret = snd_pcm_hw_constraint_integer(runtime,
//						SNDRV_PCM_HW_PARAM_PERIODS);
//	if (ret < 0)
//		goto out;
//#endif

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (prtd == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	pcm_data->prtd = prtd;
	pcm_data->capture_stream = substream;	// for snd_pcm_period_elapsed
	pcm_data->instance = substream->number;

#if defined(CONFIG_MS_VOC_MMAP_SUPPORT)
	pcm_data->mmap_mode = 0;
#endif

	voc->voc_pcm = pcm_data;
	runtime = substream->runtime;
	runtime->private_data = pcm_data;
	runtime->private_free = voc_pcm_runtime_free;
	runtime->hw = voc_pcm_capture_hardware;

	voc_communication_ts_sync_start();
	g_pcm_resume = 0;
	g_pcm_xrun = 0;

	voc_pcm_write2Log(voc, "pcm open");

	return ret;

out:
	kfree(prtd);
	kfree(pcm_data);
	return ret;
}

static int voc_pcm_close(struct snd_pcm_substream *substream)
{
	struct mtk_snd_voc *voc;
	int ret = 0;
	int result = 0;
	int retry_times = 0;
	int state = false;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	voc = (struct mtk_snd_voc *)substream->private_data;


	voc_debug("%s: stream = %s\n", __func__,
			(substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));


	//Check PMU state whether is stop
	result = voc_communication_dma_get_reset_status();
	if (result) {
		voc_err("[%s]Fail to get reset status, result = %d\n", __func__, result);
		return -EINVAL;
	}

	//Check VD task thread whether is stop
	do {
		state = voc_vd_task_state(voc);
		if (state == true) {
			msleep(WAIT_TIME_MS);
			retry_times++;

			if (retry_times >= WAIT_TIMEOUT) {
				voc_err("Wait timeout\n");
				ret = -EINVAL;
				break;
			}
			continue;
		} else {
			//vd task already finish
			break;
		}
	} while (1);

	if (retry_times > 0) {
		voc_warn("[%s][%d] Retry:%d, pcm_status:%d\n"
			, __func__, __LINE__, retry_times, voc->voc_pcm_status);
	}

	voc_pcm_write2Log(voc, "pcm close");
	return ret;
}

/* this may get called several times by oss emulation */
//hw params: hw argument for user space, including
//channels, sample rate, pcm format, period size,
//period count. These are constrained by HW constraints.
static int voc_pcm_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_voc *voc;
	struct voc_pcm_data *pcm_data;
	struct voc_pcm_runtime_data *prtd;
	int ret = 0;
	unsigned int frame_bits;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	runtime = substream->runtime;
	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm_data = substream->runtime->private_data;
	prtd = pcm_data->prtd;

	runtime->format = params_format(params);
	runtime->subformat = params_subformat(params);
	runtime->channels = params_channels(params);
	runtime->rate = params_rate(params);

	voc_debug("%s: stream = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));
	voc_debug("params_channels     = %d\n", params_channels(params));
	voc_debug("params_rate         = %d\n", params_rate(params));
	voc_debug("params_period_size  = %d\n", params_period_size(params));
	voc_debug("params_periods      = %d\n", params_periods(params));
	voc_debug("params_buffer_size  = %d\n", params_buffer_size(params));
	voc_debug("params_buffer_bytes = %d\n", params_buffer_bytes(params));
	voc_debug("params_sample_width = %d\n",
			snd_pcm_format_physical_width(params_format(params)));
	voc_debug("params_access       = %d\n", params_access(params));
	voc_debug("params_format       = %d\n", params_format(params));
	voc_debug("params_subformat    = %d\n", params_subformat(params));

	frame_bits =
		snd_pcm_format_physical_width(params_format(params)) / 8 *
		params_channels(params);

	voc_debug("frame_bits    = %d\n", frame_bits);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = params_buffer_bytes(params);

	if (prtd->dma_data) {
		voc_warn("[%s]dma_data already exist\n", __func__);
		return 0;
	}

	prtd->dma_data = &voc_pcm_dma_wr;

	/*
	 * Link channel with itself so DMA doesn't need any
	 * reprogramming while looping the buffer
	 */
	voc_debug("dma name = %s, channel id = %lu\n",
			prtd->dma_data->name,
			prtd->dma_data->channel);

	voc_debug("dma buf vir addr = %p\n", runtime->dma_area);
	voc_debug("dma buf size = %zu\n", runtime->dma_bytes);

	if (voc->sysfs_fake_capture) {
		voc_pcm_write2Log(voc, "initial channel");
	} else {
		// Re-set up the underrun and overrun
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			//ToDo: modify LTP test item.
			if (voc->communication_path_id != COMMUNICATION_PATH_PAGANINI) {
				if (voc->voc_card->card_reg[0] == 0) {
					voc_err("[%s][%d]Please switch MIC on before capture\n",
						__func__, __LINE__);
					return -EINVAL;
				}
			}
//			if (voc_machine_get_mic_bitwidth(pcm_data->rpmsg_dev) > 16) {
//				if (snd_pcm_format_physical_width(params_format(params))
//					!= 32) {
//					voc_err("bitwidth %d should be 32\n",
//							snd_pcm_format_physical_width
//							(params_format(params)));
//					return -EFAULT;
//				}
//			} else if (snd_pcm_format_physical_width
//						(params_format(params)) != 16) {
//				pr_err("bitwidth %d should be 16\n",
//				snd_pcm_format_physical_width(params_format(params)));
//				return -EFAULT;
//			}
//			if(voc_soc_get_mic_num()+voc_soc_get_ref_num()
//				!=params_channels(params))
//			{
//				printk( "channel number should be %d\n",
//						voc_soc_get_mic_num()
//						+ voc_soc_get_ref_num());
//				return -EFAULT;
//			}
			//printk( "%s: param OK!!\n", __func__);
			/* enable sinegen for debug */
			//VocDmaEnableSinegen(true);
			voc_dma_params_init(pcm_data, params_buffer_bytes(params),
			params_period_bytes(params));

			// TODO: The hardcode setting will be got from device tree.
			ret = voc_communication_dma_init_channel(
				(uint64_t)runtime->dma_addr,
				runtime->dma_bytes,
				params_channels(params),
				snd_pcm_format_physical_width(params_format(params)),
				params_rate(params));

			if (ret) {
				voc_err("[%s][%d]Fail to initial channel\n", __func__, __LINE__);
				return -EINVAL;
			}
			voc_pcm_write2Log(voc, "initial channel");
		}
	}

	memset(runtime->dma_area, 0, runtime->dma_bytes);

	return ret;
}


static int voc_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_voc *voc;
	struct voc_pcm_data *pcm_data;
	struct voc_pcm_runtime_data *prtd;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}
	runtime = substream->runtime;
	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm_data = runtime->private_data;
	prtd = pcm_data->prtd;

	voc_debug("%s: stream = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	if (prtd->dma_data == NULL) {
		voc_err("[%s][%d]Invalid dma_data\n", __func__, __LINE__);
		return 0;
	}

	//free_irq(VOC_IRQ_ID, (void *)substream);

	prtd->dma_data = NULL;
	snd_pcm_set_runtime_buffer(substream, NULL);

	voc_pcm_write2Log(voc, "voc_pcm_hw_free");

	return 0;
}


static int voc_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_voc *voc;
	struct voc_pcm_data *pcm_data;
	struct voc_pcm_runtime_data *prtd;
	struct voc_pcm_dma_data *dma_data;
	int ret = 0;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	runtime = substream->runtime;
	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm_data = runtime->private_data;
	prtd = pcm_data->prtd;
	dma_data = prtd->dma_data;

	voc_debug("%s: stream = %s, channel = %s\n", __func__,
			(substream->stream ==
		SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
			dma_data->name);

	voc_pcm_write2Log(voc, "voc_pcm_prepare");

	return ret;
}


static int voc_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_voc *voc;
	struct voc_pcm_data *pcm_data;
	struct voc_pcm_runtime_data *prtd;
	int ret = 0;
//	  struct timespec	tv;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	runtime = substream->runtime;
	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm_data = substream->runtime->private_data;
	prtd = pcm_data->prtd;

	//voc_debug("%s: stream = %s, channel = %s, cmd = %d\n", __func__,
	//		(substream->stream ==
	//	SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
	//		dma_data->name, cmd);

// AUD_PRINTF(KERN_INFO, "!!BACH_DMA1_CTRL_0 = 0x%x,
//		BACH_DMA1_CTRL_8 = 0x%x, level count = %d\n",
//		InfinityReadReg(BACH_REG_BANK1,
//				BACH_DMA1_CTRL_0),
//		InfinityReadReg(BACH_REG_BANK1,
//				BACH_DMA1_CTRL_8),
//		InfinityDmaGetLevelCnt(prtd->dma_data->channel));
	if (substream->stream != SNDRV_PCM_STREAM_CAPTURE)
		ret = -EINVAL;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (g_pcm_resume || g_pcm_xrun) {
			voc_dma_params_init(pcm_data, pcm_data->pcm_buf_size,
						pcm_data->pcm_period_size);
			voc_communication_dma_init_channel(
					(uint64_t)runtime->dma_addr,
					runtime->dma_bytes,
					runtime->channels,
					snd_pcm_format_physical_width(runtime->format),
					runtime->rate);
			voc_communication_ts_sync_start();
			g_pcm_resume = 0;
			g_pcm_xrun = 0;
		}

		mutex_lock(&voc->pcm_lock);

		//voc_dma_reset();
		voc->voc_pcm_running = 1;
		if (voc->sysfs_fake_capture)
			voc_drv_stub_wakeup();
		else {
			pcm_data->pcm_status = PCM_NORMAL;
			voc_communication_dma_start_channel();
		}

		if (cmd == SNDRV_PCM_TRIGGER_START)
			voc_pcm_write2Log(voc, "alsa trigger vd start");
		else if (cmd == SNDRV_PCM_TRIGGER_RESUME)
			voc_pcm_write2Log(voc, "alsa trigger vd resume");
		else if (cmd == SNDRV_PCM_TRIGGER_PAUSE_RELEASE)
			voc_pcm_write2Log(voc, "alsa trigger PAUSE_RELEASE:");

		mutex_unlock(&voc->pcm_lock);

		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:

		mutex_lock(&voc->pcm_lock);

		if (prtd->state == DMA_FULL || prtd->state == DMA_LOCALFULL)
			g_pcm_xrun = 1;

		memset(runtime->dma_area, 0, runtime->dma_bytes);
		prtd->dma_level_count = 0;
		prtd->state = DMA_EMPTY;


		/* Ensure that the DMA done */
		wmb();
		voc->voc_pcm_running = 0;
		voc_communication_dma_stop_channel();

		if (cmd == SNDRV_PCM_TRIGGER_STOP)
			voc_pcm_write2Log(voc, "alsa trigger vd stop");
		else if (cmd == SNDRV_PCM_TRIGGER_SUSPEND)
			voc_pcm_write2Log(voc, "alsa trigger vd suspend");
		else if (cmd == SNDRV_PCM_TRIGGER_PAUSE_PUSH)
			voc_pcm_write2Log(voc, "alsa trigger PAUSE_PUSH");

		//voc_reset_audio(pcm_data->rpmsg_dev);
		//voc_dma_reset(pcm_data);

		mutex_unlock(&voc->pcm_lock);

		break;
	default:
		ret = -EINVAL;
	}

	// AUD_PRINTF(KERN_INFO,
	//	"!!Out BACH_DMA1_CTRL_0 = 0x%x,
	//	BACH_DMA1_CTRL_8 = 0x%x,
	//	level count = %d\n",
	//	InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_0),
	//	InfinityReadReg(BACH_REG_BANK1, BACH_DMA1_CTRL_8),
	//	InfinityDmaGetLevelCnt(prtd->dma_data->channel));

	return ret;
}


static snd_pcm_uframes_t voc_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_voc *voc;
	struct voc_pcm_data *pcm_data;
	struct voc_pcm_runtime_data *prtd;
	snd_pcm_uframes_t offset = 0;
	size_t last_dma_count_level = 0;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	runtime = substream->runtime;
	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm_data = substream->runtime->private_data;
	prtd = pcm_data->prtd;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {

		mutex_lock(&voc->pcm_lock);
		/* Ensure that the DMA done */
		rmb();

		if (prtd->state == DMA_FULL || prtd->state == DMA_LOCALFULL) {
			mutex_unlock(&voc->pcm_lock);
			voc_warn("BUFFER FULL : %d!!\n", prtd->state);
			return SNDRV_PCM_POS_XRUN;
		}

		//Update water level: point will call this function to
		//get current address of transmitted data every transmission,
		//then pcm native can calculate point address of dma buffer
		//and available space
		if (voc->sysfs_fake_capture) {
			if (prtd->dma_level_count > runtime->dma_bytes * 2)
				prtd->dma_level_count -= runtime->dma_bytes;
			offset = bytes_to_frames(runtime,
					((prtd->dma_level_count) % runtime->dma_bytes));
		} else {
			last_dma_count_level = voc_dma_get_level_cnt(pcm_data);

			if (prtd->dma_level_count > (runtime->dma_bytes) * 2)
				prtd->dma_level_count -= runtime->dma_bytes;
			offset = bytes_to_frames(runtime,
					((prtd->dma_level_count +
						last_dma_count_level) %
						runtime->dma_bytes));
		}
		mutex_unlock(&voc->pcm_lock);
	}
#if SHOW_BUSY_LOG
	voc_debug("%s: stream id = %d, channel id = %lu, frame offset = 0x%x\n",
			__func__, substream->stream, prtd->dma_data->channel,
			(unsigned int)offset);
#endif
	return offset;
}


static int voc_pcm_copy(struct snd_pcm_substream *substream,
			int channel, unsigned long pos,
			void __user *dst, unsigned long count)
{
	struct snd_pcm_runtime *runtime;
	struct mtk_snd_voc *voc;
	struct voc_pcm_data *pcm_data;
	struct voc_pcm_runtime_data *prtd;
	unsigned char *zeroBuf = NULL;
	int bytewidth = 0;
	int channels = 0;
	int sample_frame = 0;
	int i, j;
	static int z;
	unsigned long flags;
	unsigned char *hwbuf;
	unsigned int state;
	uint64_t count_tmp = 0;
	uint32_t val = 0;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	runtime = substream->runtime;
	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm_data = substream->runtime->private_data;
	prtd = pcm_data->prtd;
	bytewidth = snd_pcm_format_physical_width(runtime->format)/BITS_PER_BYTE;
	channels = runtime->channels;
	count_tmp = count;
	val = bytewidth * channels;
	do_div(count_tmp, val);
	sample_frame = count_tmp;
	hwbuf = runtime->dma_area + pos;

	//voc_info("[%s]count:%ld, addr:0x%lx\n", __func__, count, (unsigned long)hwbuf);
	if (voc->sysfs_fake_capture) {
		zeroBuf = kzalloc(count, GFP_KERNEL);
		if (!zeroBuf)
			return -ENOMEM;
		memset(zeroBuf, 0, count);
	}

	/* Ensure that the DMA done */
	rmb();
	state = prtd->state;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {

		/* Ensure that the DMA done */
		rmb();
		if (!voc->sysfs_fake_capture) {
			if (prtd->state == DMA_FULL) {
				voc_err("BUFFER FULL!!\n");
				snd_pcm_stream_lock_irqsave(substream, flags);
				snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
				snd_pcm_stream_unlock_irqrestore(substream, flags);
				return -EPIPE;
			}
		}

		if (voc->sysfs_fake_capture) {
			if (!g_pcm_muting) {
				for (i = 0; i < sample_frame; i++, z++) {
					for (j = 0; j < channels; j++)
						*((short *)zeroBuf + sample_frame*j + i)
						= sine_tone[z%SINE_TONE_SIZE];
				}
			}
			if (copy_to_user(dst, zeroBuf, count)) {
				kfree(zeroBuf);
				return -EFAULT;
			}
		} else {
			if (copy_to_user(dst, hwbuf, count))
				return -EFAULT;

			mutex_lock(&voc->pcm_lock);

			if (prtd->state == DMA_EMPTY && state != prtd->state) {
				mutex_unlock(&voc->pcm_lock);
				return -EPIPE;
			}
			voc_dma_trig_level_cnt(pcm_data, count);
		}
		prtd->state = DMA_NORMAL;

		/* Ensure that the DMA done */
		wmb();
		if (voc->sysfs_fake_capture)
			prtd->dma_level_count -= count;
		else {
			prtd->dma_level_count += count;
			mutex_unlock(&voc->pcm_lock);
		}
	}

#if SHOW_BUSY_LOG
	voc_debug("frame_pos = 0x%lx, frame_count = 0x%lx, framLevelCnt = %u\n",
			pos, count, voc_dma_get_level_cnt(pcm_data));
#endif
	if (voc->sysfs_fake_capture)
		kfree(zeroBuf);

	return 0;
}


static int voc_pcm_get_time_info(struct snd_pcm_substream *substream,
			struct timespec *system_ts, struct timespec *audio_ts,
			struct snd_pcm_audio_tstamp_config *audio_tstamp_config,
			struct snd_pcm_audio_tstamp_report *audio_tstamp_report)
{
	struct mtk_snd_voc *voc;
	struct voc_pcm_data *pcm_data;
	struct timespec64 systemts;
	uint64_t audio_time = 0;
	uint32_t mod = 0;

	if (substream == NULL) {
		voc_err("[%s]Invalid substream\n", __func__);
		return -EINVAL;
	}

	voc = (struct mtk_snd_voc *)substream->private_data;
	pcm_data = substream->runtime->private_data;

	ktime_get_ts64(&systemts);

	audio_ts->tv_sec = systemts.tv_sec;
	audio_ts->tv_nsec = systemts.tv_nsec;

	// Get timestamp from PMU set to audio_ts
	// Time of PMU + Time of clock sync
	audio_time = (pcm_data->pcm_time*1000000 //ms->ns
				+ voc->sync_time); //ns
	mod = do_div(audio_time, 1000000000);
	system_ts->tv_sec = audio_time; // ns->sec
	system_ts->tv_nsec = mod; // total ns remove sec
#if SHOW_BUSY_LOG
	voc_debug("[%s][%d]audio ts: %ld sec %ld nsec, system ts: %ld sec %ld nsec\n",
				__func__, __LINE__, audio_ts->tv_sec, audio_ts->tv_nsec,
				system_ts->tv_sec, system_ts->tv_nsec);
#endif
	return 0;
}


static struct snd_pcm_ops voc_pcm_ops = {
	.open = voc_pcm_open,
	.close = voc_pcm_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = voc_pcm_hw_params,
	.hw_free = voc_pcm_hw_free,
	.prepare = voc_pcm_prepare,
	.trigger = voc_pcm_trigger,
	.pointer = voc_pcm_pointer,
	.copy_user = voc_pcm_copy,
	.get_time_info = voc_pcm_get_time_info
};

//------------------------------------------------------------------------------
//  Voc Card Function
//------------------------------------------------------------------------------
void voc_free_dmem(void *addr)
{
	iounmap(addr);
}

static void voc_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;

	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	if (!substream)
		return;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return;

	voc_free_dmem((void *)buf->area);

	buf->area = NULL;

}

unsigned char *voc_alloc_dmem(struct mtk_snd_voc *voc,
			int size, dma_addr_t *addr)
{
	unsigned char *ptr = NULL;
	u32 mem_free_size = voc->voc_card->voc_pcm_size;
	phys_addr_t mem_cur_pos = voc->voc_card->voc_buf_paddr;

	if (!addr)
		return NULL;

	if (mem_free_size && mem_free_size > size) {
		ptr = (unsigned char *)ioremap_wc(mem_cur_pos + voc->miu_base, size);
		if (!ptr) {
			voc_err("ioremap NULL address,phys : %pa\n", &mem_cur_pos);
			*addr = 0x0;
		} else
			*addr = (dma_addr_t)(mem_cur_pos + voc->miu_base);
	} else {
		*addr = 0x0;
		voc_err("No enough memory, free = %u\n", mem_free_size);
	}

	return ptr;
}

static int voc_pcm_preallocate_dma_buffer(struct mtk_snd_voc *voc, int stream)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	unsigned int iIndex;
	size_t size = 0;

	if (!voc) {
		voc_err("Invalid voc argument\n");
		return -EINVAL;
	}

	if (stream < 0) {
		voc_err("Invalid stream argument\n");
		return -EINVAL;
	}
	iIndex = stream;

	substream = voc->pcm->streams[iIndex].substream;
	buf = &substream->dma_buffer;

	voc_debug("%s: stream = %s\r\n", __func__,
				(substream->stream ==
				SNDRV_PCM_STREAM_PLAYBACK ? "PLAYBACK" : "CAPTURE"));

	if (stream == SNDRV_PCM_STREAM_CAPTURE)  // CAPTURE device
		size = voc_pcm_capture_hardware.buffer_bytes_max;
	else
		return -EINVAL;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = voc->card->dev;
	buf->private_data = NULL;
	buf->area = voc_alloc_dmem(voc, PAGE_ALIGN(size), &buf->addr);

	if (!buf->area)
		return -ENOMEM;
	buf->bytes = PAGE_ALIGN(size);

	voc_debug("dma buffer align size 0x%lx, size 0x%zx\n", (unsigned long)buf->bytes, size);
	voc_debug("physical dma address %pad\n", &buf->addr);
	voc_debug("virtual dma address,%lx\n", (unsigned long)buf->area);

	return 0;
}

static int voc_pcm_dma_new(struct mtk_snd_voc *voc)
{
	struct snd_card *card;
	struct snd_pcm *pcm;
	struct snd_pcm_substream *substream;
	int ret = 0;

	if (!voc) {
		voc_err("Invalid voc argument\n");
		return -EINVAL;
	}

	card = voc->card;
	pcm = voc->pcm;

	voc_info("%s: snd_pcm device id = %d\r\n", __func__, pcm->device);

	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &voc_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (substream) {
		ret = voc_pcm_preallocate_dma_buffer(voc, SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}

out:
	/* free preallocated buffers in case of error */
	if (ret)
		voc_pcm_free_dma_buffers(pcm);

	return ret;
}


static int voc_card_add_thread(void *param)
{
	struct mtk_snd_voc *voc =
			(struct mtk_snd_voc *)param;

	voc_dev_info(voc->dev, "Wait voc soc card and rpmsg device binding...\n");

	// complete when rpmsg device and voc soc card device binded
	if (voc->communication_path_id == COMMUNICATION_PATH_RPMSG)
		wait_for_completion(&voc->rpmsg_bind);
	voc_dev_info(voc->dev, "voc soc card and rpmsg device binded\n");
	//Notify host cpu status
	voc_communication_notify_status(0x1);
	//voc_dev_info(voc->dev, "notify to PMU\n");
	//Notify for ready to start to execute timestamp synchronization
	voc_communication_ts_sync_start();
	//voc_dev_info(voc->dev, "voc soc PTP time sync\n");

	return 0;
}

static int voc_card_dai_init(void)
{
	struct device_node *np;
	void __iomem *vbdma_base = NULL;

#if CONFIG_OF
	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dai");
	if (np) {
		vbdma_base = of_iomap(np, 0);
		vbdma_get_base(vbdma_base);
	} else {
		voc_err("Fail to find DAI compatible node\n");
		return -ENODEV;
	}
#endif

	return 0;
}

static int voc_card_dma_init(struct platform_device *pdev, struct mtk_snd_voc *voc)
{
	int ret = 0;
	struct device_node *np;
	struct voc_card_drvdata *card_drvdata = voc->voc_card;
	u32 voc_pcm_size;

	if (pdev == NULL) {
		voc_err("pdev is NULL\n");
		return -ENODEV;
	}

	if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32))) {
		voc_dev_err(&pdev->dev, "dma_set_mask_and_coherent failed\n");
		return -ENODEV;
	}

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dma");
	ret = of_property_read_u32(np, "voc-communication-path", &voc->communication_path_id);
	if (ret < 0)
		voc->communication_path_id = COMMUNICATION_PATH_RPMSG;

	#if ENABLE_FORCE_STUB_MODE
	ret = of_property_read_u32(np, "voc_force_default_stub", &g_force_stub_mode);
	voc_info("voc_force_default_stub = %d\n", g_force_stub_mode);
	if (g_force_stub_mode) {
		voc->sysfs_fake_capture = 1;
		voc->communication_path_id = COMMUNICATION_PATH_STUB;
	}
	#endif

	voc_info("voc->communication_path_id = %d\n", voc->communication_path_id);

        ret = of_property_read_u32(np, "power-saving", &voc->power_saving);
        if (ret < 0)
                voc->power_saving = 0;

	ret = of_property_read_u32(np, "voc-pcm-size", &voc_pcm_size);
	card_drvdata->voc_pcm_size = voc_pcm_size;
	voc->sysfs_notify_test = 0;
	memset(&voc->dbgInfo, 0, sizeof(struct voc_dma_dbg));

	if (ret < 0) {
		voc_err("request_irq failed, err = %d!!\n", ret);
		return ret;
	}

	voc->dbgInfo.writed = 0;
	memset(voc->dbgInfo.log, 0, sizeof(struct operation_log) * MAX_LOG_LEN);

	return ret;
}

static int voc_card_machine_init(struct platform_device *pdev,  struct mtk_snd_voc *voc)
{
	int ret = 0;
	struct voc_card_drvdata *card_drvdata = voc->voc_card;
	struct device_node *np;
	struct of_mmap_info_data of_mmap_info = {0};
	u32 vbdma_ch;
	u32 miu_base;
#if ENABLE_OWL_MODE
	u32 owl_mem_cpu_paddr;
	u32 owl_mem_size;
#endif

#ifdef SLT_TEMP
	u32 addr;
	u32 imi_base_addr[2];
	int size;
	unsigned char *riu_map = NULL;
#endif

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-machine");

	ret = of_property_read_u32(np, "vbdma-ch", &vbdma_ch);
	if (ret < 0) {
		voc_err("Request property failed, err = %d!!\n", ret);
		return ret;
	}
	card_drvdata->vbdma_ch = vbdma_ch;

	ret = of_property_read_u32(np, "miu-base", &miu_base);
	voc->miu_base = miu_base;

	if (of_mtk_get_reserved_memory_info_by_idx(np, 0, &of_mmap_info) == 0) {
		card_drvdata->voc_buf_paddr = (u32)of_mmap_info.start_bus_address - voc->miu_base;
		card_drvdata->voc_buf_size = (u32)of_mmap_info.buffer_size;
	}

	#if ENABLE_OWL_MODE
	if (of_mtk_get_reserved_memory_info_by_idx(np, 1, &of_mmap_info) == 0) {
		owl_mem_cpu_paddr = (u32)of_mmap_info.start_bus_address;
		owl_mem_size = (u32)of_mmap_info.buffer_size;

		voc_info("owl_mem_cpu_paddr:0x%x\n", owl_mem_cpu_paddr);
		voc_info("owl_mem_size:0x%x\n", owl_mem_size);
	}
	#endif

#ifdef SLT_TEMP
	of_property_read_u32_array(np, "imi-top-base", imi_base_addr, 2);
	of_property_read_u32_index(np, "imi-top-base", 5, &addr);
	of_property_read_u32_index(np, "imi-top-base", 7, &size);
	riu_map = (unsigned char *)ioremap(addr, size);
	riu_base = riu_map;
	imi_base = imi_base_addr[1];
#endif

	card_drvdata->snd_model_array = dma_alloc_coherent(&pdev->dev,
		SND_MODEL_SIZE,
		&card_drvdata->snd_model_paddr,
		GFP_KERNEL);

	voc_info("snd_model_array address:[%p]\n", card_drvdata->snd_model_array);


	card_drvdata->card_reg = card_reg;

	return ret;
}

static int voc_card_module_init(struct platform_device *pdev, struct mtk_snd_voc *voc)
{
	int ret = 0;
	static struct task_struct *t;
	struct voc_card_drvdata *card_drvdata = NULL;

	card_drvdata = voc->voc_card;
	if (!card_drvdata) {
		card_drvdata = kzalloc(sizeof(struct voc_card_drvdata), GFP_KERNEL);
		if (!card_drvdata)
			return -ENOMEM;
		voc->voc_card = card_drvdata;
	}

	//DAI -> DMA -> MACHINE
	//1. DAI initial
	ret = voc_card_dai_init();
	if (ret < 0) {
		voc_dev_err(&pdev->dev, "Fail to init DAI\n");
		return ret;
	}

	//2. DMA initial
	ret = voc_card_dma_init(pdev, voc);
	if (ret < 0) {
		voc_dev_err(&pdev->dev, "Fail to init DMA\n");
		return ret;
	}

	//3. MACHINE initial
	ret = voc_card_machine_init(pdev, voc);
	if (ret < 0) {
		voc_dev_err(&pdev->dev, "Fail to init MACHINE\n");
		return ret;
	}

	voc_communication_init(voc);

	// add to mtk_snd_voc list
	INIT_LIST_HEAD(&voc->list);
	list_add_tail(&voc->list, &voc_card_dev_list);
	init_completion(&voc->rpmsg_bind);

	// check if there is a valid rpmsg_dev
	ret = voc_communication_bind(voc);
	if (ret) {
		voc_dev_err(&pdev->dev, "binding rpmsg-voc failed! (%d)\n", ret);
		goto bind_fail;
	}

	t = kthread_run(voc_card_add_thread,
			(void *)voc, "voc_card_add");

	if (IS_ERR(t)) {
		voc_dev_err(&pdev->dev, "create thread for add voc sound card failed!\n");
		ret = IS_ERR(t);
		goto bind_fail;
	}

	//4. New DMA buffer for pcm used
	ret = voc_pcm_dma_new(voc);
	if (ret < 0) {
		voc_dev_err(&pdev->dev, "Fail to new DMA buffer\n");
		return ret;
	}

	voc_debug("[%s][%d]Initial success\n", __func__, __LINE__);
	return ret;

bind_fail:
	list_del(&voc->list);
	return ret;

}

static int voc_card_pcm_new(struct mtk_snd_voc *voc,
			    int device, int playback_substreams, int capture_substreams)
{
	struct snd_pcm *pcm;
	int err;

	if (voc == NULL) {
		voc_err("[%s][%d]Invalid argument\n", __func__, __LINE__);
		return -EINVAL;
	}

	err = snd_pcm_new(voc->card, SND_PCM_NAME, device,
			  playback_substreams, capture_substreams, &pcm);

	if (err < 0) {
		voc_err("[%s][%d]Fail to create pcm instance err=%d\n"
			, __func__, __LINE__, err);
		return err;
	}

	if (playback_substreams) {
		voc_err("[%s][%d]Playback count:(%d>0)\n"
			, __func__, __LINE__, playback_substreams);
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &voc_pcm_ops);
	}

	if (capture_substreams)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &voc_pcm_ops);

	pcm->private_data = voc;
	pcm->info_flags = 0;
	pcm->nonatomic = true;
	strlcpy(pcm->name, SND_PCM_NAME, sizeof(pcm->name));

	voc->pcm = pcm;

	return err;
}

static unsigned int voc_card_read(struct mtk_snd_voc *voc,
			unsigned int reg)
{
	voc_debug("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, card_reg[reg]);

	if (!voc) {
		voc_err("Invalid voc argument\n");
		return -EINVAL;
	}

	if (reg == VOC_REG_HOTWORD_VER) {
		voc_communication_get_hotword_ver();
		voc_info("%s: hotword version reg = 0x%x, val = %d\n",
				__func__, reg, card_reg[reg]);
	}

	return card_reg[reg];
}

static int voc_card_write(struct mtk_snd_voc *voc, unsigned int reg,
				unsigned int value)
{
	int ret = 0;
	int16_t minus_value;

	voc_debug("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, value);
	if (!voc) {
		voc_err("Invalid voc argument\n");
		return -EINVAL;
	}

	if ((reg != VOC_REG_MIC_ONOFF) && !(card_reg[VOC_REG_MIC_ONOFF])) {
		voc_debug("%s: mic mute , reg = 0x%x, val = 0x%x\n",
				__func__, reg, value);
		return -1;
	}

	switch (reg) {
	case VOC_REG_MIC_ONOFF:
		voc_info("%s: mic switch reg: x%x, val = 0x%x\n", __func__,
				reg, value);
		// TODO: rename voc_dmic_switch()
		//       to distinguish voc_dmic_switch() from voc_dmic_mute()
			ret = voc_communication_dmic_switch(value);
		break;
	case VOC_REG_VQ_ONOFF:
		voc_info("%s: VQ switch reg: x%x, val = 0x%x\n", __func__,
				reg, value);
		if (value == VOC_WAKEUP_MODE_OFF) {
			//disable kws detect
			ret = voc_communication_enable_vq(value);
		} else if (value == VOC_WAKEUP_MODE_NORMAL) {
			//enable kws detect in normal mode
			ret = voc_communication_config_vq(0);
			ret = voc_communication_enable_vq(1);
		} else if (value == VOC_WAKEUP_MODE_PM) {
			//enable kws detect in standby mode
			ret = voc_communication_config_vq(1);
			ret = voc_communication_enable_vq(1);
		} else if (value == VOC_WAKEUP_MODE_BOTH) {
			//enable kws detect in dual mode (normal/standby)
			ret = voc_communication_config_vq(VOC_MAGIC_02);
			ret = voc_communication_enable_vq(1);
		}
		break;
	case VOC_REG_SEAMLESS_ONOFF:
		voc_info("%s: seamless reg: x%x, val = 0x%x\n", __func__, reg,
				value);
			ret = voc_communication_enable_seamless(value);
		break;
	case VOC_REG_SIGEN_ONOFF:
		voc_info("%s: sigen reg: x%x, val = 0x%x\n", __func__, reg,
				value);
			ret = voc_communication_dma_enable_sinegen(value);
		break;
	case VOC_REG_MIC_GAIN:
		voc_info("%s: mic gain reg: x%x, val = 0x%x\n", __func__, reg,
				value);
			minus_value = (int16_t)value;
			minus_value *= -1;
			ret = voc_communication_dmic_gain(minus_value);
		break;
	case VOC_REG_HPF_ONOFF:
		voc_info("%s: HPF switch reg: x%x, val = 0x%x\n", __func__,
				reg, value);
			ret = voc_communication_enable_hpf(value);
		break;
	case VOC_REG_HPF_CONFIG: {
		long val;

		voc_info("%s: HPF config reg: x%x, val = %s\n", __func__,
				reg, hpf_config_text[value]);
		if (kstrtol(hpf_config_text[value], STR2LONG_BASE_DECIMAL, &val) == 0)
			ret = voc_communication_config_hpf((int)val);
		break;
	}
	case VOC_REG_MUTE_SWITCH: {
		voc_info("%s: mute switch reg: x%x, val = 0x%x\n", __func__,
				reg, value);
		g_pcm_muting = value;
			ret = voc_communication_dmic_mute(value);
		break;
	}
	default:
		voc_err("%s: error parameter, reg = 0x%x, val = 0x%x\n",
				__func__, reg, value);
		return -1;
	}

	if (ret) {
		voc_err("%s: update error , reg = 0x%x, val = 0x%x\n",
				__func__, reg, value);
		ret = -1;
	} else {
		card_reg[reg] = value;
		ret = 0;
	}

	return ret;
}



static int voc_card_get_enum(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_voc *voc = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;

	ucontrol->value.integer.value[0] = voc_card_read(voc, e->reg);
	return 0;
}

static int voc_card_put_enum(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_voc *voc = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;

	if (item[0] >= e->items)
		return -EINVAL;

	return voc_card_write(voc, e->reg,
					ucontrol->value.integer.value[0]);
}

static int voc_card_get_volsw(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_voc *voc = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;

	ucontrol->value.integer.value[0] = voc_card_read(voc, mc->reg);
	return 0;
}

static int voc_card_put_volsw(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct mtk_snd_voc *voc = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
	    (struct soc_mixer_control *)kcontrol->private_value;

	if ((ucontrol->value.integer.value[0] < 0)
		|| (ucontrol->value.integer.value[0] > mc->max)) {
		voc_info("reg:%d range MUST is (0->%d), %ld is overflow\r\n",
		       mc->reg, mc->max, ucontrol->value.integer.value[0]);
		return -1;
	}
	return voc_card_write(voc, mc->reg,
					ucontrol->value.integer.value[0]);
}

static int voc_card_get_uuid(struct snd_kcontrol *kcontrol,
			unsigned int __user *data, unsigned int size)
{
	int ret = 0;
	struct mtk_snd_voc *voc = snd_kcontrol_chip(kcontrol);
	struct voc_card_drvdata *card_drvdata =	NULL;


	if (!voc) {
		voc_err("Invalid voc argument\n");
		return -EINVAL;
	}
	card_drvdata = voc->voc_card;

	if (!card_drvdata) {
		voc_err("Invalid card driver data\n");
		return -EINVAL;
	}

	if ((size - TLV_HEADER_SIZE) > DSP_PLATFORM_ID_DATA_SIZE) {
		voc_err("Size overflow.(%d>%d)\r\n", size, DSP_PLATFORM_ID_DATA_SIZE);
		return -EINVAL;
	}

	memset(g_tlv_data, 0, DSP_PLATFORM_ID_SIZE);

	card_drvdata->uuid_array = &g_tlv_data[TLV_HEADER_SIZE];

	card_drvdata->uuid_size = DSP_PLATFORM_ID_DATA_SIZE;

	ret = voc_communication_get_hotword_identifier();

	if (ret != 0) {
		voc_err("Fail to get identifier result = %d\n", ret);
		return -EINVAL;
	}
	voc_info("uuid array:[%s], size=%d\n", card_drvdata->uuid_array, card_drvdata->uuid_size);

	if (copy_to_user(data, g_tlv_data, size)) {
		voc_err("%s(), copy_to_user fail", __func__);
		return -EFAULT;
	}

	return ret;
}

static int voc_card_put_uuid(struct snd_kcontrol *kcontrol,
			const unsigned int __user *data, unsigned int size)
{
	voc_info("UUID is read-only\n");
	return 0;
}

static int voc_card_get_sound_model(struct snd_kcontrol *kcontrol,
						unsigned int __user *data,
						unsigned int size)
{
	int ret = 0;
	struct mtk_snd_voc *voc = snd_kcontrol_chip(kcontrol);
	struct voc_card_drvdata *card_drvdata =	voc->voc_card;

	if ((size - TLV_HEADER_SIZE) > SND_MODEL_DATA_SIZE) {
		voc_err("%s(), Size overflow.(%d>%d)\r\n", __func__, size, SND_MODEL_DATA_SIZE);
		return -EINVAL;
	}

	voc_info("snd_model_array address:[%p]\n", card_drvdata->snd_model_array);
	voc_info("user buffer address:[%p]\n", data);

	if (card_drvdata->snd_model_array == NULL)
		return ret;

	if (copy_to_user(data, card_drvdata->snd_model_array, size)) {
		voc_err("%s(), copy_to_user fail", __func__);
		ret = -EFAULT;
	}
	return ret;
}

static unsigned int voc_card_get_checksum(unsigned char *buf, int len)
{
	unsigned int sum = 0;
	int i;

	for (i = 0; i < len; i++)
		sum += buf[i];
	return sum;
}

static int voc_card_check_soundmodel(unsigned char *buf, int size)
{
	u32 sound_model_uuid;
	u32 sound_model_checksum;
	u32 checksum;

	memcpy(&sound_model_uuid, buf, sizeof(u32));
	memcpy(&sound_model_checksum, buf + sizeof(u32), sizeof(u32));

	checksum = voc_card_get_checksum(buf + TLV_HEADER_SIZE,
						size - TLV_HEADER_SIZE);

	if (sound_model_uuid != SOUND_TRIGGER_UUID_0)
		return -EFAULT;

	if (sound_model_checksum != checksum)
		return -EFAULT;

	return 0;
}

static int voc_card_put_sound_model(struct snd_kcontrol *kcontrol,
						const unsigned int __user *data,
						unsigned int size)
{
	int ret = 0;
	struct mtk_snd_voc *voc = snd_kcontrol_chip(kcontrol);
	struct voc_card_drvdata *card_drvdata = voc->voc_card;

	if (size <= TLV_HEADER_SIZE) {
		voc_err("%s(), Size is too small\r\n", __func__);
		return -EINVAL;
	}

	if ((size - TLV_HEADER_SIZE) > SND_MODEL_DATA_SIZE) {
		voc_err("%s(), Size overflow.(%d>%d)\r\n", __func__, size, SND_MODEL_DATA_SIZE);
		return -EINVAL;
	}

	if (card_drvdata->snd_model_array == NULL) {
		voc_err("%s(), card_drvdata->snd_model_array is null", __func__);
		return ret;
	}

	if (copy_from_user(card_drvdata->snd_model_array, data, size)) {
		voc_err("%s(), copy_to_user fail", __func__);
		ret = -EFAULT;
	}

	if (voc_card_check_soundmodel(card_drvdata->snd_model_array, size) != 0) {
		voc_err("%s(),check sound model fail", __func__);
		return ret;
	}

	voc_communication_update_snd_model(
				card_drvdata->snd_model_paddr + TLV_HEADER_SIZE - voc->miu_base,
				size - TLV_HEADER_SIZE);

	return ret;
}


static const struct snd_kcontrol_new voc_snd_controls[] = {

	SOC_ENUM_VOC("Mic switch", mic_onoff_enum),
	SOC_ENUM_VOC("Voice WakeUp Switch", vq_onoff_enum),
	SOC_ENUM_VOC("Seamless Mode", seamless_onoff_enum),
	SOC_SINGLE_EXT("Mic Gain Step", VOC_REG_MIC_GAIN, 0, 480, 0,
		       voc_card_get_volsw, voc_card_put_volsw),
	SOC_ENUM_VOC("HPF Switch", hpf_onoff_enum),
	SOC_ENUM_VOC("HPF Coef", hpf_config_enum),
	SOC_ENUM_VOC("Sigen Switch", sigen_onoff_enum),
	SND_SOC_BYTES_TLV("Sound Model", SND_MODEL_SIZE,
				voc_card_get_sound_model,
				voc_card_put_sound_model),
	SOC_SINGLE_EXT("Hotword Version", VOC_REG_HOTWORD_VER, 0, 0x7FFFFFFF, 0,
				voc_card_get_volsw, NULL),
	SND_SOC_BYTES_TLV("DSP platform identifier", DSP_PLATFORM_ID_SIZE,
				voc_card_get_uuid,
				voc_card_put_uuid),
	SOC_ENUM_VOC("Mute switch", mute_switch_enum)
};

static int voc_card_mixer_new(struct mtk_snd_voc *voc)
{
	struct snd_card *card = voc->card;
	struct snd_kcontrol *kcontrol;
	unsigned int idx;
	int err;

	strlcpy(card->mixername, "voc_snd_card", sizeof(card->mixername));

	for (idx = 0; idx < ARRAY_SIZE(voc_snd_controls); idx++) {
		kcontrol = snd_ctl_new1(&voc_snd_controls[idx], voc);
		err = snd_ctl_add(card, kcontrol);
		if (err < 0)
			return err;
	}

	return 0;
}
static int voc_card_bck_check(void)
{
	int ret = 0;
	struct device_node *np;
	u32 bck_mode;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mtk-mic-dma");
	if (!np)
		return ret;

	if (of_property_read_u32(np, "bck_mode", &bck_mode) >= 0) {
		if (bck_mode == 0) {
			voc_err("[%s] To disable voice since bck mode = 0\n", __func__);
			ret = -EFAULT;
		}
	}

	of_node_put(np);

	return ret;
}

static int voc_card_probe(struct platform_device *pdev)
{
	struct snd_card *card;
	struct mtk_snd_voc *voc;
	unsigned int dev = (unsigned int)pdev->id;
	int ret;

	ret = voc_card_bck_check();
	if (ret < 0)
		return ret;

	// Start to new card / pcm / mixer
	ret = snd_card_new(&pdev->dev, index[dev], id[dev], THIS_MODULE,
			   sizeof(struct mtk_snd_voc), &card);

	if (ret < 0)
		return ret;
	voc = card->private_data;
	if (pcm_capture_substreams[dev] < 1)
		pcm_capture_substreams[dev] = 1;
	if (pcm_capture_substreams[dev] > MAX_PCM_CAPTURE_SUBSTREAMS)
		pcm_capture_substreams[dev] = MAX_PCM_CAPTURE_SUBSTREAMS;

	if (pcm_playback_substreams[dev] < 1)
		pcm_playback_substreams[dev] = 1;
	if (pcm_playback_substreams[dev] > MAX_PCM_PLAYBACK_SUBSTREAMS)
		pcm_playback_substreams[dev] = MAX_PCM_PLAYBACK_SUBSTREAMS;

	voc->card = card;
	init_waitqueue_head(&voc->kthread_waitq);
	ret = voc_card_pcm_new(voc, 0, pcm_playback_substreams[dev], pcm_capture_substreams[dev]);
	voc_info("[%s][%d] pcm playback substreams[%d]=%d, pcm capture substreams[%d]=%d\n"
		, __func__, __LINE__, dev, pcm_playback_substreams[dev]
		, dev, pcm_capture_substreams[dev]);

	if (ret < 0) {
		voc_err("[%s][%d]Fail to create pcm error:%d\n"
			, __func__, __LINE__, ret);
		goto nodev;
	}


	strlcpy(card->driver, "vocsndcard", sizeof(card->driver));
	strlcpy(card->shortname, "vocsndcard", sizeof(card->shortname));
	snprintf(card->longname, sizeof(card->longname), "vocsndcard %i", dev + 1);

	snd_card_set_dev(card, &pdev->dev);
	voc->dev = &pdev->dev;

	ret = snd_card_register(card);
	if (!ret) {
		platform_set_drvdata(pdev, card);
		//return 0;
	}

	ret = voc_card_module_init(pdev, voc);
	if (ret < 0) {
		voc_err("[%s][%d]Fail to initial module:%d\n"
			, __func__, __LINE__, ret);
		goto nodev;
	}

	ret = voc_card_mixer_new(voc);
	if (ret < 0) {
		voc_err("[%s][%d]Fail to create mixer error:%d\n"
			, __func__, __LINE__, ret);
		goto nodev;
	}

	voc->vd_task = kthread_run(voc_vd_task_thread, (void *)voc, "vd_task");
	if (IS_ERR(voc->vd_task)) {
		voc_err("create thread for add vd_task failed!\n");
		ret = PTR_ERR(voc->vd_task);
		goto nodev;
	}

	mutex_init(&voc->pcm_lock);

	mtk_dtv_voc_card_create_sysfs_attr(pdev);

	ret = device_create_file(voc->dev, &dev_attr_self_Diagnostic);
	if (ret != 0) {
		voc_dev_err(voc->dev, "%s Create self diagnostic attr fail (%d)\n", __func__, ret);
		goto nodev;
	}

	return 0;

nodev:
	snd_card_free(card);
	return ret;

}

static int voc_card_remove(struct platform_device *pdev)
{
	struct snd_card *card = platform_get_drvdata(pdev);
	struct mtk_snd_voc *voc = (struct mtk_snd_voc *)card->private_data;

	voc_debug("%s\n", __func__);

	voc_communication_reset_voice();
	voc_communication_exit();
	list_del(&voc->list);
	kthread_stop(voc->vd_task);
	mtk_dtv_voc_card_remove_sysfs_attr(pdev);
	snd_card_free(platform_get_drvdata(pdev));
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver voc_card_driver = {
	.probe = voc_card_probe,
	.remove = voc_card_remove,
	.shutdown = voc_card_shutdown,
	.driver = {
			.name = SND_VOC_DRIVER,
			.owner = THIS_MODULE,
#ifdef CONFIG_PM_SLEEP
			.pm = &voc_card_pm_ops,
#endif
			},
};

static void voc_card_unregister_all(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devices); ++i)
		platform_device_unregister(devices[i]);
	platform_driver_unregister(&voc_card_driver);
}


static int __init voc_card_init(void)
{
	int i, err, cards;

	voc_info("[%s][%d]\n", __func__, __LINE__);

	#if ENABLE_FORCE_STUB_MODE
	g_force_stub_mode = 0;
	#endif
	g_pcm_muting = 0;

	err = platform_driver_register(&voc_card_driver);
	if (err < 0)
		return err;

	cards = 0;
	for (i = 0; i < SNDRV_CARDS; i++) {
		struct platform_device *device;

		if (!enable[i])
			continue;
		device = platform_device_register_simple(SND_VOC_DRIVER,
							 i, NULL, 0);
		if (IS_ERR(device))
			continue;
		if (!platform_get_drvdata(device)) {
			platform_device_unregister(device);
			continue;
		}
		devices[i] = device;
		cards++;
	}

	voc_info("[%s][%d]:Total cards = %d\n", __func__, __LINE__, cards);

	if (!cards) {
#ifdef MODULE
		voc_err("voc-card: No voc driver enabled\n");
#endif
		voc_card_unregister_all();
		return -ENODEV;
	}

	return 0;
}

static void __exit voc_card_exit(void)
{
	voc_info("%s\n", __func__);
	voc_card_unregister_all();
}

module_init(voc_card_init);
module_exit(voc_card_exit);


/* Module information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allen-HW Hsu, allen-hw.hsu@mediatek.com");
MODULE_DESCRIPTION("Voc Audio ALSA driver");
