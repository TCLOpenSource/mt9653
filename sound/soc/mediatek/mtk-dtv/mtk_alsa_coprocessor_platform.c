// SPDX-License-Identifier: GPL-2.0
/*
 * mtk_alsa_coprocessor_platform.c  --  coprocessor driver
 *
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <sound/soc.h>
#include <sound/control.h>

#include "mtk_alsa_chip.h"
#include "mtk_alsa_coprocessor_platform.h"
#include "mtk-reserved-memory.h"

static ptrdiff_t g_dsp_va;
struct dsp_alg_info dsp_info[AU_CODE_SEGMENT_MAX];

DEFINE_MUTEX(audio_idma_mutex);

static struct copr_register REG;

#define ALSA_AUDIO_ENTER(dev, id, priv)	\
({do {AUD_DEV_PRINT_COP(dev, COPRO_DEVICE, AUD_INFO, priv	\
, "[%s:%d] !!!\n", __func__, __LINE__); } while (0); })

#define SOC_INTEGER_DSP(xname, xenum) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = copr_dsp_control_info, \
	.get = copr_dsp_control_get_enum, .put = copr_dsp_control_put_enum, \
	.private_value = (unsigned long)&xenum }

static const struct soc_enum dsp_main_counter =
	SOC_ENUM_SINGLE(DSP_MAIN_COUNTER, 0, 0xff, 0);

static const struct soc_enum dsp_isr_counter =
	SOC_ENUM_SINGLE(DSP_ISR_COUNTER, 0, 0xff, 0);

static struct snd_soc_dai_driver copr_dais[] = {
	{
		.name = "LOAD_DSP",
		.id = 0,
		.playback = {
			.stream_name = "DSP_LOAD",
			.channels_min = 2,
			.channels_max = 8,
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
					SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_U24_LE,
		},
	},
};

void copr_dsp_setmeminfo(bool miu_bank, ptrdiff_t dsp_base,
				ptrdiff_t icache_base)
{
	ptrdiff_t dsp_line = dsp_base >> BYTES_IN_MIU_LINE_LOG2;
	ptrdiff_t icache_offset = icache_base >> ICACHE_MEMOFFSET_LOG2;

	mtk_alsa_write_reg_mask_byte(REG.RIU_MAIL_00 + 1, 0x01, miu_bank);

	mtk_alsa_write_reg_mask_byte(REG.DEC_AUD_CTRL, 0x01, 0x00);
	mtk_alsa_write_reg_mask(REG.SE_AUD_CTRL, 0x04FF, 0x0400);

	/* COPR BASE */
	mtk_alsa_write_reg_mask_byte(REG.DEC_MAD_OFFSET_BASE_L + 1, 0xFF,
					(unsigned char)(dsp_line & 0xFF));
	mtk_alsa_write_reg_mask_byte(REG.SE_MAD_OFFSET_BASE_L + 1, 0xFF,
					(unsigned char)(dsp_line & 0xFF));
	mtk_alsa_write_reg(REG.DEC_MAD_OFFSET_BASE_H,
					(unsigned short)(dsp_line >> 8));
	mtk_alsa_write_reg(REG.SE_MAD_OFFSET_BASE_H,
					(unsigned short)(dsp_line >> 8));
	mtk_alsa_write_reg_mask_byte(REG.DEC_MAD_OFFSET_BASE_EXT, 0x0F,
					(unsigned char)(dsp_line >> 24));
	mtk_alsa_write_reg_mask_byte(REG.SE_MAD_OFFSET_BASE_EXT, 0x0F,
					(unsigned char)(dsp_line >> 24));

	mtk_alsa_write_reg_mask_byte(REG.DEC_AUD_CTRL, 0x40, 0x40);
	mtk_alsa_write_reg_mask_byte(REG.SE_AUD_CTRL, 0x40, 0x40);
	mtk_alsa_write_reg_mask_byte(REG.DEC_AUD_CTRL, 0x80, 0x80);
	mtk_alsa_write_reg_mask_byte(REG.SE_AUD_CTRL, 0x80, 0x80);

	/* ICACHE BASE */
	mtk_alsa_write_reg(REG.SE_DSP_ICACHE_BASE_L, (unsigned short)(icache_offset & 0xFFFF));
	mtk_alsa_write_reg_mask_byte(REG.SE_BDMA_CFG, 0x0F,
					(unsigned char)(icache_offset >> 16));

	mtk_alsa_write_reg_mask_byte(REG.DEC_AUD_CTRL, 0x80, 0x00);
	mtk_alsa_write_reg_mask_byte(REG.SE_AUD_CTRL, 0x80, 0x00);
	mtk_alsa_write_reg_mask_byte(REG.DEC_AUD_CTRL, 0x40, 0x00);
	mtk_alsa_write_reg_mask_byte(REG.SE_AUD_CTRL, 0x40, 0x00);
}

static int copr_check_dsp_alive(void)
{
	unsigned char CurrentSeWhile = 0;
	unsigned char CurrentSeISR = 0;
	static unsigned char  LastSeWhile = 1;
	static unsigned char  LastSeISR = 1;
	static unsigned char  SeWhileFailCount;
	static unsigned char  SeISRFailCount;
	int i;

	for (i = 0; i < 20; i++) {
		CurrentSeWhile = mtk_alsa_read_reg_byte(REG.DSP_WHILE1_CNTR);
		CurrentSeISR = mtk_alsa_read_reg_byte(REG.DSP_ISR_CNTR);

		if (CurrentSeWhile == LastSeWhile)
			SeWhileFailCount++;
		else
			SeWhileFailCount = 0;

		if (CurrentSeISR == LastSeISR)
			SeISRFailCount++;
		else
			SeISRFailCount = 0;

		if ((SeWhileFailCount >= 10) || (SeISRFailCount >= 10))
			return 1;

		LastSeWhile = CurrentSeWhile;
		LastSeISR = CurrentSeISR;

		mtk_alsa_delaytask(1);
	}

	return 0;
}

static void copr_get_dsp_info(struct copr_priv *priv, const struct firmware *fw)
{
	u8 *fw_data;

	fw_data = (u8 *)fw->data;

	/* AU_SE_SYSTEM */
	dsp_info[AU_SE_SYSTEM].cm_addr = 0x0008;
	dsp_info[AU_SE_SYSTEM].cm_len = MST_CODEC_PM1_SIZE - 24;
	dsp_info[AU_SE_SYSTEM].cm_buf = fw_data + MST_CODEC_PM1_FLASH_ADDR + 24;

	dsp_info[AU_SE_SYSTEM].pm_addr = MST_CODEC_PM2_ADDR;
	dsp_info[AU_SE_SYSTEM].pm_len = MST_CODEC_PM2_SIZE;
	dsp_info[AU_SE_SYSTEM].pm_buf = fw_data + MST_CODEC_PM2_FLASH_ADDR;

	dsp_info[AU_SE_SYSTEM].cache_addr = MST_CODEC_PM3_ADDR;
	dsp_info[AU_SE_SYSTEM].cache_len = MST_CODEC_PM3_SIZE;
	dsp_info[AU_SE_SYSTEM].cache_buf = fw_data + MST_CODEC_PM3_FLASH_ADDR;

	dsp_info[AU_SE_SYSTEM].prefetch_addr = MST_CODEC_PM4_ADDR;
	dsp_info[AU_SE_SYSTEM].prefetch_len = MST_CODEC_PM4_SIZE;
	dsp_info[AU_SE_SYSTEM].prefetch_buf = fw_data +
						MST_CODEC_PM4_FLASH_ADDR;

	/* AU_DVB2_NONE */
	dsp_info[AU_DVB2_NONE].cm_addr = MST_CODEC_DEC_PM1_ADDR;
	dsp_info[AU_DVB2_NONE].cm_len = MST_CODEC_DEC_PM1_SIZE;
	dsp_info[AU_DVB2_NONE].cm_buf = fw_data + MST_CODEC_DEC_PM1_FLASH_ADDR;

	dsp_info[AU_DVB2_NONE].pm_addr = MST_CODEC_DEC_PM2_ADDR;
	dsp_info[AU_DVB2_NONE].pm_len = MST_CODEC_DEC_PM2_SIZE;
	dsp_info[AU_DVB2_NONE].pm_buf = fw_data + MST_CODEC_DEC_PM2_FLASH_ADDR;

	dsp_info[AU_DVB2_NONE].cache_addr = MST_CODEC_DEC_PM3_ADDR;
	dsp_info[AU_DVB2_NONE].cache_len = MST_CODEC_DEC_PM3_SIZE;
	dsp_info[AU_DVB2_NONE].cache_buf = fw_data +
						MST_CODEC_DEC_PM3_FLASH_ADDR;

	dsp_info[AU_DVB2_NONE].prefetch_addr = MST_CODEC_DEC_PM4_ADDR;
	dsp_info[AU_DVB2_NONE].prefetch_len = MST_CODEC_DEC_PM4_SIZE;
	dsp_info[AU_DVB2_NONE].prefetch_buf = fw_data +
						MST_CODEC_DEC_PM4_FLASH_ADDR;

	/* SIF */
	dsp_info[AU_SIF].cm_addr = MST_CODEC_SIF_PM1_ADDR;
	dsp_info[AU_SIF].cm_len = MST_CODEC_SIF_PM1_SIZE;
	dsp_info[AU_SIF].cm_buf = fw_data + MST_CODEC_SIF_PM1_FLASH_ADDR;

	dsp_info[AU_SIF].pm_addr = MST_CODEC_SIF_PM2_ADDR;
	dsp_info[AU_SIF].pm_len = MST_CODEC_SIF_PM2_SIZE;
	dsp_info[AU_SIF].pm_buf = fw_data + MST_CODEC_SIF_PM2_FLASH_ADDR;

	dsp_info[AU_SIF].cache_addr = MST_CODEC_SIF_PM3_ADDR;
	dsp_info[AU_SIF].cache_len = MST_CODEC_SIF_PM3_SIZE;
	dsp_info[AU_SIF].cache_buf = fw_data + MST_CODEC_SIF_PM3_FLASH_ADDR;

	dsp_info[AU_SIF].prefetch_addr = 0;
	dsp_info[AU_SIF].prefetch_len = 0;
	dsp_info[AU_SIF].prefetch_buf = 0;


}

bool mtk_alsa_write_sram_segment(unsigned int sram_address,
	unsigned char *sram_seg_buf, unsigned int sram_seg_len, bool sram_type)
{
	bool ret;
	int i;

	mutex_lock(&audio_idma_mutex);

	mtk_alsa_write_reg_mask(REG.SE_BDMA_CFG, 0x8080, 0x0000);
	mtk_alsa_write_reg_mask_byte(REG.FD230_SEL, 0x01, 0x01);

	ret = mtk_alsa_read_reg_byte_polling(REG.SE_IDMA_CTRL0, 0x18, 0x00, 200);
	if (ret) {
		mutex_unlock(&audio_idma_mutex);
		return 1;
	}

	mtk_alsa_write_reg(REG.SE_IDMA_WRBASE_ADDR_L, sram_address);

	for (i = 0; i < (sram_seg_len / 3); i++) {
		mtk_alsa_write_reg_byte(REG.SE_DSP_BRG_DATA_L, *(sram_seg_buf + (3 * i) + 1));
		mtk_alsa_write_reg_byte(REG.SE_DSP_BRG_DATA_H, *(sram_seg_buf + (3 * i) + 2));
		ret = mtk_alsa_read_reg_byte_polling(REG.SE_IDMA_CTRL0, 0x04, 0x04, 200);
		if (ret) {
			mutex_unlock(&audio_idma_mutex);
			return 1;
		}

		mtk_alsa_write_reg_byte(REG.SE_DSP_BRG_DATA_L, *(sram_seg_buf + (3 * i)));
		mtk_alsa_write_reg_byte(REG.SE_DSP_BRG_DATA_H, 0x00);
		ret = mtk_alsa_read_reg_byte_polling(REG.SE_IDMA_CTRL0, 0x18, 0x00, 200);
		if (ret) {
			mutex_unlock(&audio_idma_mutex);
			return 1;
		}
	}
	mutex_unlock(&audio_idma_mutex);
	return 0;
}

static int copr_load_dsp_image(struct copr_priv *priv, const struct firmware *fw,
				unsigned char dsp_select)
{
	struct dsp_alg_info *pau_info = NULL;
	static ptrdiff_t dsp_ba;
	static ptrdiff_t dsp_va;
	static ptrdiff_t MIU_addr;
	int time_out = 0;
	int ret;

	/* get dsp base */
	dsp_ba = priv->mem_info.bus_addr;
	if ((dsp_ba % 0x1000)) {
		pr_err("[ALSA COP]Invalid bus addr,should aligned by 4 KB!\n");
		return -EFAULT;
	}

	/* convert Bus Address to Virtual Address */
	if (g_dsp_va == 0) {
		g_dsp_va = (ptrdiff_t)ioremap_wc(dsp_ba,
				(MST_CODEC_DEC_PM4_ADDR * 3) + MST_CODEC_DEC_PM4_SIZE);
		if (g_dsp_va == 0) {
			pr_err("[ALSA COP]fail to convert Bus Addr\n");
			return -ENOMEM;
		}
	}
	dsp_va = g_dsp_va;

	pau_info = &dsp_info[dsp_select];
	/* Download PM of Algorithm */
	ret = mtk_alsa_write_sram_segment(pau_info->pm_addr,
			pau_info->pm_buf, pau_info->pm_len, 0);
	if (ret) {
		pr_err("[ALSA COP]load dsp segememt %d pm fail\n", ret);
		return ret;
	}
	/* Download CM */
	ret = mtk_alsa_write_sram_segment(pau_info->cm_addr,
			pau_info->cm_buf, pau_info->cm_len, 0);
	if (ret) {
		pr_err("[ALSA COP]load dsp segememt %d cm fail\n", ret);
		return ret;
	}
	/* Download PM of PreFetch */
	if (pau_info->prefetch_len != 0) {
		MIU_addr = (ptrdiff_t)pau_info->prefetch_addr * 3 + dsp_va;
		memcpy((void *)MIU_addr, (void *)pau_info->prefetch_buf,
						pau_info->prefetch_len);
	}
	/* Download PM of Cache */
	if (pau_info->cache_len != 0) {
		MIU_addr = (ptrdiff_t)pau_info->cache_addr * 3 + dsp_va;
		memcpy((void *)MIU_addr, (void *)pau_info->cache_buf,
							pau_info->cache_len);
	}

	if (dsp_select == AU_SE_SYSTEM) {
		pau_info->cm_buf = pau_info->cm_buf - 24;
		mtk_alsa_write_reg_mask(REG.DSP_DBG_CMD1, 0xFFFF, 0x0000);
		mtk_alsa_delaytask(1);

		ret = mtk_alsa_write_sram_segment(0x0001,
					pau_info->cm_buf + 3, 21, 0);
		if (ret) {
			pr_err("[ALSA COP]load dsp seg %d pm 6 fail\n", ret);
			return ret;
		}
		ret = mtk_alsa_write_sram_segment(0x0000,
					pau_info->cm_buf, 3, 0);
		if (ret) {
			pr_err("[ALSA COP]load dsp seg %d pm 0 fail\n", ret);
			return ret;
		}
		/* Wait Dsp init finished Ack */
		while (time_out++ < 100) {
			if (mtk_alsa_read_reg_byte(REG.DSP_DBG_RESULT1) == 0xE3)
				break;
			mtk_alsa_delaytask(2);
		}

		if (time_out >= 100)
			pr_err("[ALSA COP]DSP2 Re-Active\n");
		else
			pr_err("[ALSA COP]audio DSP_SE LoadCode success..\n");

		/* inform DSP to start to run */
		mtk_alsa_write_reg_mask(REG.DSP_DBG_CMD1, 0xFF00, 0xF300);
	}

	return 0;
}

static void copr_load_image_to_dsp_and_run(const struct firmware *fw,
					void *context)
{
	struct copr_priv *priv = context;
	int ret;

	if (fw == NULL) {
		pr_err("[ALSA COP]fw = NULL, request audio fw not avail\n");
		return;
	}

	/* toggle to initialize DAC DATA SRAM. */
	mtk_alsa_write_reg_mask_byte(REG.RIU_MAIL_00, 0x02, 0x02);
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_mask_byte(REG.RIU_MAIL_00, 0x02, 0x00);
	/* reset dsp */
	mtk_alsa_write_reg_byte(REG.SE_IDMA_CTRL0, 0x02);
	mtk_alsa_delaytask(1);
	mtk_alsa_write_reg_byte(REG.SE_IDMA_CTRL0, 0x03);

	/* assign dsp info */
	copr_get_dsp_info(priv, fw);

	/* load dsp binary */
	/* load null */
	ret = copr_load_dsp_image(priv, fw, AU_SIF);
	if (ret)
		pr_err("[ALSA COP]load dsp code AU_DVB2_NONE fail\n");
	/* load aucode_s */
	ret = copr_load_dsp_image(priv, fw, AU_SE_SYSTEM);
	if (ret)
		pr_err("[ALSA COP]load dsp code AU_SE_SYSTEM fail\n");

	release_firmware(fw);

	if (!ret)
		pr_info("[ALSA COP]audio load firmware success\n");
}

static int copr_async_load_dsp_bin_and_run(struct snd_soc_component *component)
{
	struct copr_priv *priv = snd_soc_component_get_drvdata(component);
	int ret = 0;

	/* load dsp firmware and run dsp */
	ret = request_firmware_nowait(THIS_MODULE, 1, priv->fw_name, priv->dev,
			GFP_KERNEL, priv, copr_load_image_to_dsp_and_run);
	return ret;
}

int copr_suspend(struct snd_soc_component *component)
{
	struct copr_priv *priv = snd_soc_component_get_drvdata(component);

	if (priv->suspended)
		return 0;

	priv->suspended = true;

	return 0;
}

int copr_resume(struct snd_soc_component *component)
{
	struct copr_priv *priv = snd_soc_component_get_drvdata(component);
	const struct firmware *fw = NULL;
	phys_addr_t dsp_phy_addr;
	int ret;

	if (!priv->suspended)
		return 0;

	dev_info(priv->dev, "[ALSA COP]Reload audio DSP\n");

	dsp_phy_addr = priv->mem_info.bus_addr - priv->mem_info.memory_bus_base;
	/* DSP power up command, DO NOT touch bit3 */
	mtk_alsa_write_reg_mask_byte(REG.DSP_POWER_DOWN_H, 0x02, 0x00);
	/* set mem info */
	copr_dsp_setmeminfo(priv->mem_info.miu, dsp_phy_addr, dsp_phy_addr);

	ret = request_firmware(&fw, priv->fw_name, priv->dev);
	if (ret < 0) {
		dev_err(priv->dev, "[ALSA COP]Err! loading audio fw fail\n");
		release_firmware(fw);
		return -EINVAL;
	}

	copr_load_image_to_dsp_and_run(fw, priv);

	priv->suspended = false;

	return 0;
}


//[Sysfs] HELP command
static ssize_t help_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct copr_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][ERROR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_COP(dev, COPRO_DEVICE, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_COP(dev, COPRO_DEVICE, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 350, "Debug Help:\n"
		       "- coprocessor_log_level\n"
		       "	<W>: Set debug log level.\n"
		       "	(0:emerg, 1:alert, 2:critical, 3:error, 4:warn, 5:notice, 6:info, 7:debug)\n"
		       "	<R>: Read debug log level.\n");
}

static ssize_t copro_log_level_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t retval;
	int level;
	struct copr_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][COPR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_COP(dev, COPRO_DEVICE, AUD_ERR, priv
		, "[%s]attr can't be NULL!!!\n", __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_COP(dev, COPRO_DEVICE, AUD_ERR, priv
		, "[%s]buf can't be NULL!!!\n", __func__);
		return 0;
	}

	retval = kstrtoint(buf, 10, &level);
	if (retval == 0) {
		if ((level < AUD_EMERG) || (level > AUD_DEB))
			retval = -EINVAL;
		else
			priv->log_level = level;
	}

	return (retval < 0) ? retval : count;
}

static ssize_t copro_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct copr_priv *priv;

	if (dev == NULL) {
		pr_err("[AUDIO][ERROR][%s]dev can't be NULL!!!\n", __func__);
		return 0;
	}
	priv = dev_get_drvdata(dev);

	if (priv == NULL) {
		dev_err(dev, "[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return 0;
	}

	if (attr == NULL) {
		AUD_DEV_PRINT_COP(dev, COPRO_DEVICE, AUD_ERR, priv, "[%s]attr can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	if (buf == NULL) {
		AUD_DEV_PRINT_COP(dev, COPRO_DEVICE, AUD_ERR, priv, "[%s]buf can't be NULL!!!\n"
		, __func__);
		return 0;
	}

	return snprintf(buf, sizeof(char) * 35, "coprocessor_log_level:%d\n",
				  priv->log_level);
}

static ssize_t suspend_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct copr_priv *priv = dev_get_drvdata(dev);
	int ret;

	ret = copr_suspend(priv->component);
	return snprintf(buf, sizeof(char) * 5, "%d\n", ret);
}

static ssize_t resume_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct copr_priv *priv = dev_get_drvdata(dev);
	int ret;

	ret = copr_resume(priv->component);
	mtk_alsa_delaytask(2);
	return snprintf(buf, sizeof(char) * 10, "%d\n", ret);
}

static DEVICE_ATTR_RO(help);
static DEVICE_ATTR_RW(copro_log_level);
static DEVICE_ATTR_RO(suspend);
static DEVICE_ATTR_RO(resume);

static struct attribute *coprocessor_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_copro_log_level.attr,
	NULL,
};

static struct attribute *test_attrs[] = {
	&dev_attr_suspend.attr,
	&dev_attr_resume.attr,
	NULL,
};

static const struct attribute_group coprocessor_attr_group = {
	.name = "mtk_dbg",
	.attrs = coprocessor_attrs,
};

static const struct attribute_group test_attr_group = {
	.name = "mtk_test",
	.attrs = test_attrs,
};

static const struct attribute_group *coprocessor_attr_groups[] = {
	&coprocessor_attr_group,
	&test_attr_group,
	NULL,
};

static int copr_probe(struct snd_soc_component *component)
{
	struct copr_priv *priv;
	struct device *dev;
	phys_addr_t dsp_phy_addr;
	int ret;

	if (component == NULL) {
		pr_err("[AUDIO][ERROR][%s]component can't be NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (component->dev == NULL) {
		pr_err("[AUDIO][ERROR][%s]component->dev can't be NULL!!!\n", __func__);
		return -EINVAL;
	}

	priv = snd_soc_component_get_drvdata(component);

	if (priv == NULL) {
		pr_err("[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);
		return -EFAULT;
	}

	dev = component->dev;
	ret = 0;

	/*Set default log level*/
	priv->log_level = AUD_ERR;

	dsp_phy_addr = priv->mem_info.bus_addr - priv->mem_info.memory_bus_base;
	/* DSP power up command, DO NOT touch bit3 */
	mtk_alsa_write_reg_mask_byte(REG.DSP_POWER_DOWN_H, 0x02, 0x00);
	/* set mem info */
	copr_dsp_setmeminfo(priv->mem_info.miu, dsp_phy_addr, dsp_phy_addr);
	/* load dsp binary */
	ret = copr_async_load_dsp_bin_and_run(component);
	if (ret) {
		AUD_DEV_PRINT_COP(priv->dev, COPRO_DEVICE, AUD_ERR, priv,
			"[ALSA COP]audio load firmware fail\n");
	}
	/* Create device attribute files */
	ret = sysfs_create_groups(&dev->kobj, coprocessor_attr_groups);
	if (ret) {
		AUD_DEV_PRINT_COP(priv->dev, COPRO_DEVICE, AUD_ERR, priv,
			"[ALSA COP] Sysfs init fail\n");
	}

	priv->component = component;

	return 0;
}

static void copr_remove(struct snd_soc_component *component)
{
	struct copr_priv *priv;
	struct device *dev;

	priv = NULL;
	dev = NULL;

	if (component == NULL)
		pr_err("[AUDIO][ERROR][%s]component can't be NULL!!!\n", __func__);

	if ((component != NULL) && (component->dev == NULL))
		pr_err("[AUDIO][ERROR][%s]component->dev can't be NULL!!!\n", __func__);

	if ((component != NULL) && (component->dev != NULL))
		priv = snd_soc_component_get_drvdata(component);

	if (priv == NULL)
		pr_err("[AUDIO][ERROR][%s]priv can't be NULL!!!\n", __func__);

	if (component != NULL)
		dev = component->dev;

	if (dev != NULL)
		sysfs_remove_groups(&dev->kobj, coprocessor_attr_groups);

	if (g_dsp_va != 0) {
		iounmap((void *)g_dsp_va);
		g_dsp_va = 0;
	}
}

const struct snd_pcm_ops copr_pcm_ops = {
	.ioctl = snd_pcm_lib_ioctl,
};

static int copr_dsp_control_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0xFF;

	return 0;
}

unsigned int copr_dsp_kcontrol_read(struct snd_soc_component *component,
					unsigned int reg)
{
	int ret = 0;

	if (reg >= DSP_END)
		return -1;

	switch (reg) {
	case DSP_MAIN_COUNTER:
		ret = mtk_alsa_read_reg_byte(REG.DSP_WHILE1_CNTR);
		break;

	case DSP_ISR_COUNTER:
		ret = mtk_alsa_read_reg_byte(REG.DSP_ISR_CNTR);
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static int copr_dsp_control_get_enum(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
			snd_soc_kcontrol_component(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;

	ucontrol->value.integer.value[0] =
			copr_dsp_kcontrol_read(component, e->reg);

	return 0;
}

static int copr_dsp_control_put_enum(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int copr_dsp_sw_reset_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
			snd_soc_kcontrol_component(kcontrol);
	struct copr_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int val = ucontrol->value.integer.value[0];

	if (val != 1)
		return -EINVAL;
	dev_info(priv->dev, "[ALSA COP]SW Reset audio DSP\n");
	/* do dsp sw reset */
	mtk_alsa_write_reg_mask_byte(REG.SE_IDMA_CTRL0, 0x03, 0x00);

	return 0;
}

static int copr_dsp_sw_reset_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int copr_dsp_reload_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component =
				snd_soc_kcontrol_component(kcontrol);
	struct copr_priv *priv = snd_soc_component_get_drvdata(component);
	unsigned int val = ucontrol->value.integer.value[0];
	const struct firmware *fw = NULL;
	int ret;

	if (val != 1)
		return -EINVAL;

	dev_info(priv->dev, "[ALSA COP]Reload audio DSP\n");

	ret = request_firmware(&fw, priv->fw_name, priv->dev);
	if (ret < 0) {
		dev_err(priv->dev, "[ALSA COP]Err! loading audio fw fail\n");
		release_firmware(fw);
		return -EINVAL;
	}

	copr_load_image_to_dsp_and_run(fw, priv);

	/* check dsp alive or not */
	ret = copr_check_dsp_alive();
	if (ret != 0) {
		dev_err(priv->dev, "[ALSA COP]Err! dsp not alive\n");
		return -EINVAL;
	}

	return 0;
}

static int copr_dsp_reload_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static const struct snd_kcontrol_new copr_controls[] = {
	SOC_INTEGER_DSP("DSP Main Counter", dsp_main_counter),
	SOC_INTEGER_DSP("DSP Isr Counter", dsp_isr_counter),
	SOC_SINGLE_BOOL_EXT("DSP SW Reset", 0,
			copr_dsp_sw_reset_get, copr_dsp_sw_reset_put),
	SOC_SINGLE_BOOL_EXT("DSP Reload", 0,
			copr_dsp_reload_get, copr_dsp_reload_put),
};

const struct snd_soc_component_driver copr_component = {
	.name		= "coprocessor-platform",
	.probe		= copr_probe,
	.remove		= copr_remove,
	.ops		= &copr_pcm_ops,
	.controls	= copr_controls,
	.num_controls	= ARRAY_SIZE(copr_controls),
	.suspend	= copr_suspend,
	.resume		= copr_resume,
};

static unsigned int copr_get_dts_u32(struct device_node *np, const char *name)
{
	unsigned int value;
	int ret;

	ret = of_property_read_u32(np, name, &value);
	if (ret)
		WARN_ON("[ALSA COP]Can't get DTS property\n");

	return value;
}

static int copr_parse_registers(void)
{
	struct device_node *np;

	memset(&REG, 0x0, sizeof(struct copr_register));

	np = of_find_node_by_name(NULL, "coprocessor-register");
	if (!np)
		return -ENODEV;

	REG.DEC_AUD_CTRL = copr_get_dts_u32(np, "reg_dec_aud_ctrl");
	REG.DEC_MAD_OFFSET_BASE_L = copr_get_dts_u32(np, "reg_dec_mad_offset_base_l");
	REG.DEC_MAD_OFFSET_BASE_H = copr_get_dts_u32(np, "reg_dec_mad_offset_base_h");
	REG.DEC_MAD_OFFSET_BASE_EXT = copr_get_dts_u32(np, "reg_dec_mad_offset_base_ext");
	REG.SE_AUD_CTRL = copr_get_dts_u32(np, "reg_se_aud_ctrl");
	REG.SE_MAD_OFFSET_BASE_L = copr_get_dts_u32(np, "reg_se_mad_offset_base_l");
	REG.SE_MAD_OFFSET_BASE_H = copr_get_dts_u32(np, "reg_se_mad_offset_base_h");
	REG.SE_MAD_OFFSET_BASE_EXT = copr_get_dts_u32(np, "reg_se_mad_offset_base_ext");
	REG.SE_IDMA_CTRL0 = copr_get_dts_u32(np, "reg_se_idma_ctrl0");
	REG.SE_DSP_ICACHE_BASE_L = copr_get_dts_u32(np, "reg_se_dsp_icache_base_l");
	REG.SE_BDMA_CFG = copr_get_dts_u32(np, "reg_se_bdma_cfg");
	REG.FD230_SEL = copr_get_dts_u32(np, "reg_fd230_sel");
	REG.SE_DSP_BRG_DATA_L = copr_get_dts_u32(np, "reg_se_dsp_brg_data_l");
	REG.SE_DSP_BRG_DATA_H = copr_get_dts_u32(np, "reg_se_dsp_brg_data_h");
	REG.SE_IDMA_WRBASE_ADDR_L = copr_get_dts_u32(np, "reg_se_idma_wrbase_addr_l");
	REG.RIU_MAIL_00 = copr_get_dts_u32(np, "reg_riu_mail_00");
	REG.DSP_POWER_DOWN_H = copr_get_dts_u32(np, "reg_dsp_power_down_h");
	REG.DSP_DBG_CMD1 = copr_get_dts_u32(np, "reg_dsp_dbg_cmd1");
	REG.DSP_ISR_CNTR = copr_get_dts_u32(np, "reg_dsp_isr_cntr");
	REG.DSP_WHILE1_CNTR = copr_get_dts_u32(np, "reg_dsp_while1_cntr");
	REG.DSP_DBG_RESULT1 = copr_get_dts_u32(np, "reg_dsp_dbg_result1");

	return 0;
}

static int copr_parse_mmap(struct device_node *np, struct copr_priv *priv)
{
	struct of_mmap_info_data of_mmap_info = {};
	int ret;

	ret = of_mtk_get_reserved_memory_info_by_idx(np,
						0, &of_mmap_info);
	if (ret) {
		pr_err("[ALSA COP]get audio mmap fail\n");
		return -EINVAL;
	}

	priv->mem_info.miu = of_mmap_info.miu;
	priv->mem_info.bus_addr = of_mmap_info.start_bus_address;
	priv->mem_info.buffer_size = of_mmap_info.buffer_size;

	return 0;
}

static int copr_dev_probe(struct platform_device *pdev)
{
	struct copr_priv *priv;
	struct device *dev = &pdev->dev;
	struct device_node *node;
	int ret;

	node = dev->of_node;
	if (!node)
		return -ENODEV;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;

	/* parse dts */
	ret = of_property_read_string(node, "dsp_name", &priv->fw_name);
	if (ret) {
		dev_err(dev, "[ALSA COP]can't get audio firmware\n");
		return -EINVAL;
	}

	ret = of_property_read_u64(node, "memory_bus_base",
					&priv->mem_info.memory_bus_base);
	if (ret) {
		dev_err(dev, "[ALSA COP]can't get miu bus base\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "fw_size", &priv->fw_size);
	if (ret) {
		dev_err(dev, "[ALSA COP]can't get fw size\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, priv);

	ret = devm_snd_soc_register_component(&pdev->dev,
					&copr_component,
					copr_dais,
					ARRAY_SIZE(copr_dais));
	if (ret) {
		dev_err(dev, "[ALSA COP]soc_register_component fail %d\n", ret);
		return ret;
	}
	/* parse registers */
	ret = copr_parse_registers();
	if (ret) {
		dev_err(dev, "[ALSA COP]parse register fail %d\n", ret);
		return ret;
	}
	/* parse mmap */
	ret = copr_parse_mmap(node, priv);
	if (ret) {
		dev_err(dev, "[ALSA COP]parse mmap fail %d\n", ret);
		return ret;
	}

	return 0;
}

static int copr_dev_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id coprocessor_dt_match[] = {
	{ .compatible = "mediatek,mtk-coprocessor-platform", },
	{ }
};
MODULE_DEVICE_TABLE(of, coprocessor_dt_match);

static struct platform_driver coprocessor_driver = {
	.driver = {
		.name		= "coprocessor-platform",
		.of_match_table	= coprocessor_dt_match,
		.owner		= THIS_MODULE,
	},
	.probe	= copr_dev_probe,
	.remove	= copr_dev_remove,
};

module_platform_driver(coprocessor_driver);

MODULE_DESCRIPTION("ALSA SoC Coprocessor platform driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("snd coprocessor driver");

