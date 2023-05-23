// SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause
/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will cd be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2019 MediaTek Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

//------------------------------------------------------------------------------
//  Include files
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <sound/soc.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/of_reserved_mem.h>
#include "voc_communication.h"
#include "mtk_dbg.h"

#ifdef SLT_TEMP
#include <linux/io.h>
#endif
/* TODO: Use reserved memory region as data buffer between linux and PMU
 *       maybe have different mechanism (ex. dma_ api) M6
 */
//#define DMA_BUFFER_IN_RESERVED_MEM
#define SOC_ENUM_VOC(xname, xenum) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_enum_double, \
	.get = voc_soc_card_get_enum, .put = voc_soc_card_put_enum, \
	.private_value = (unsigned long)&xenum }

/* sound card controls */

// TODO: apply dts
#define TLV_HEADER_SIZE             8
#define SND_MODEL_DATA_SIZE         10
#define SND_MODEL_SIZE              (SND_MODEL_DATA_SIZE + TLV_HEADER_SIZE)
static unsigned long mtk_log_level = LOGLEVEL_ERR;

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


#ifdef SLT_TEMP
// TODO: we will put variables into card_drvdata
static void __iomem *riu_base;
static u32 imi_base;
#endif

enum {
	VOC_REG_MIC_ONOFF = 0,
	VOC_REG_VQ_ONOFF,
	VOC_REG_SEAMLESS_ONOFF,
	VOC_REG_DMIC_NUMBER,
	VOC_REG_MIC_GAIN,
	VOC_REG_MIC_BITWIDTH,
	VOC_REG_HPF_ONOFF,
	VOC_REG_HPF_CONFIG,
	VOC_REG_SIGEN_ONOFF,
	VOC_REG_SOUND_MODEL,
	VOC_REG_HOTWORD_VER,
	VOC_REG_LEN,
};

// TODO: move these two global variables into voc_soc_card_drvdata
// instead of using these directly
static unsigned int card_reg_backup[VOC_REG_LEN] = {0};
static unsigned int card_reg[VOC_REG_LEN] = {
	0x0,			//VOC_REG_MIC_ONOFF
	0x0,			//VOC_REG_VQ_ONOFF
	0x0,			//VOC_REG_SEAMLESS_ONOFF
	0x2,			//VOC_REG_DMIC_NUMBER
	0x0,			//VOC_REG_MIC_GAIN
	0x0,			//VOC_REG_MIC_BITWIDTH
	0x2,			//VOC_REG_HPF_ONOFF
	0x6,			//VOC_REG_HPF_CONFIG
	0x0,			//VOC_REG_SIGEN_ONOFF
	0x0,			//VOC_REG_SOUND_MODEL
	0x0			//VOC_REG_HOTWORD_VER
};

static const char * const mic_onoff_text[] = { "Off", "On" };
static const char * const vq_config_text[] = { "Off", "PM", "AUTO_TEST" };
static const char * const seamless_onoff_text[] = { "Off", "On" };
static const char * const mic_bitwidth_text[] = { "16", "32" };
static const char * const hpf_onoff_text[] = { "Off", "1-stage", "2-stage" };
static const char * const hpf_config_text[] = {
	"-2", "-1", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static const char * const sigen_onoff_text[] = { "Off", "On" };

static const struct soc_enum mic_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_MIC_ONOFF, 0, 2, mic_onoff_text);

static const struct soc_enum vq_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_VQ_ONOFF, 0, 3, vq_config_text);

static const struct soc_enum seamless_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_SEAMLESS_ONOFF, 0, 2, seamless_onoff_text);

static const struct soc_enum mic_bitwidth_enum =
SOC_ENUM_SINGLE(VOC_REG_MIC_BITWIDTH, 0, 2, mic_bitwidth_text);

static const struct soc_enum hpf_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_HPF_ONOFF, 0, 3, hpf_onoff_text);

static const struct soc_enum hpf_config_enum =
SOC_ENUM_SINGLE(VOC_REG_HPF_CONFIG, 0, 12, hpf_config_text);

static const struct soc_enum sigen_onoff_enum =
SOC_ENUM_SINGLE(VOC_REG_SIGEN_ONOFF, 0, 2, sigen_onoff_text);

SND_SOC_DAILINK_DEFS(voc,
	DAILINK_COMP_ARRAY(COMP_CPU("voc-cpu-dai")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("voc-platform")));

static unsigned int voc_soc_card_read(struct snd_soc_card *card,
			unsigned int reg)
{
	struct voc_soc_card_drvdata *card_drvdata =
					snd_soc_card_get_drvdata(card);

	voc_debug("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, card_reg[reg]);

	if (reg == VOC_REG_HOTWORD_VER) {
		voc_info("%s: hotword version reg = 0x%x, val = 0x%x\n",
				__func__, reg, card_reg[reg]);
		voc_get_hotword_ver(card_drvdata->rpmsg_dev);
	}

	return card_reg[reg];
}

//TODO: AMIC
//static unsigned int voc_soc_get_mic_num(struct snd_soc_card *card)
//{
//	return card_reg[VOC_REG_DMIC_NUMBER];
//}

//static bool voc_soc_is_seamless_enabled(struct snd_soc_card *card)
//{
//	if (card_reg[VOC_REG_MIC_ONOFF] &&
//		card_reg[VOC_REG_SEAMLESS_ONOFF] &&
//		card_reg[VOC_REG_VQ_ONOFF])
//		return true;

//	return false;
//}

//static unsigned int voc_soc_get_mic_bitwidth(struct snd_soc_card *card)
//{
//	switch (card_reg[VOC_REG_MIC_BITWIDTH]) {
//	case 0:		//E_MBX_AUD_BITWIDTH_16
//		return 16;
//	case 2:		//E_MBX_AUD_BITWIDTH_32
//		return 32;
//	default:
//		return 16;
//	}
//}

static int voc_soc_card_write(struct snd_soc_card *card, unsigned int reg,
				unsigned int value)
{
	int ret = 0;
	struct voc_soc_card_drvdata *card_drvdata =
					snd_soc_card_get_drvdata(card);

	voc_debug("%s: reg = 0x%x, val = 0x%x\n", __func__, reg, value);

	if ((reg != VOC_REG_MIC_ONOFF) && !(card_reg[VOC_REG_MIC_ONOFF])) {
		voc_debug("%s: mic mute , reg = 0x%x, val = 0x%x\n",
				__func__, reg, value);
		return -1;
	}

	switch (reg) {
	case VOC_REG_MIC_ONOFF:
		voc_info("%s: cm4 switch reg: x%x, val = 0x%x\n", __func__,
				reg, value);
		break;
	case VOC_REG_VQ_ONOFF:
		voc_info("%s: VQ switch reg: x%x, val = 0x%x\n", __func__,
				reg, value);
		ret = voc_config_vq(card_drvdata->rpmsg_dev, value);
		break;
	case VOC_REG_SEAMLESS_ONOFF:
		voc_info("%s: seamless reg: x%x, val = 0x%x\n", __func__, reg,
				value);
		break;
	case VOC_REG_SIGEN_ONOFF:
		voc_info("%s: sigen reg: x%x, val = 0x%x\n", __func__, reg,
				value);
		ret = voc_dma_enable_sinegen(card_drvdata->rpmsg_dev, value);
		break;
	case VOC_REG_DMIC_NUMBER:
		voc_info("%s: dmic reg: x%x, val = 0x%x\n", __func__, reg,
				value);
		ret = voc_dmic_number(card_drvdata->rpmsg_dev, value);
		break;
	case VOC_REG_MIC_GAIN:
		voc_info("%s: mic gain reg: x%x, val = 0x%x\n", __func__, reg,
				value);
		ret = voc_dmic_gain(card_drvdata->rpmsg_dev, value);
		break;
	case VOC_REG_MIC_BITWIDTH: {
		long val;

		voc_info("%s: mic bitwidth reg: x%x, val = 0x%x\n", __func__,
				reg, value);
		if (!kstrtol(mic_bitwidth_text[value], 10, &val))
			ret = voc_dmic_bitwidth(card_drvdata->rpmsg_dev, (char)val);
		break;
	}
	case VOC_REG_HPF_ONOFF:
		voc_info("%s: HPF switch reg: x%x, val = 0x%x\n", __func__,
				reg, value);
		ret = voc_enable_hpf(card_drvdata->rpmsg_dev, value);
		break;
	case VOC_REG_HPF_CONFIG: {
		long val;

		voc_info("%s: HPF config reg: x%x, val = %s\n", __func__,
				reg, hpf_config_text[value]);
		if (!kstrtol(hpf_config_text[value], 10, &val))
			ret = voc_config_hpf(card_drvdata->rpmsg_dev, (int)val);
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

static int voc_soc_card_get_enum(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;

	ucontrol->value.integer.value[0] = voc_soc_card_read(card, e->reg);
	return 0;
}

static int voc_soc_card_put_enum(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;

	if (item[0] >= e->items)
		return -EINVAL;

	return voc_soc_card_write(card, e->reg,
					ucontrol->value.integer.value[0]);
}

static int voc_soc_card_get_volsw(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;

	ucontrol->value.integer.value[0] = voc_soc_card_read(card, mc->reg);
	return 0;
}

static int voc_soc_card_put_volsw(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
	    (struct soc_mixer_control *)kcontrol->private_value;

	if ((ucontrol->value.integer.value[0] < 0)
		|| (ucontrol->value.integer.value[0] > mc->max)) {
		voc_info("reg:%d range MUST is (0->%d), %ld is overflow\r\n",
		       mc->reg, mc->max, ucontrol->value.integer.value[0]);
		return -1;
	}
	return voc_soc_card_write(card, mc->reg,
					ucontrol->value.integer.value[0]);
}

static int voc_soc_card_get_sound_model(struct snd_kcontrol *kcontrol,
						unsigned int __user *data,
						unsigned int size)
{
	int ret = 0;
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct voc_soc_card_drvdata *card_drvdata =
					snd_soc_card_get_drvdata(card);

	if ((size - TLV_HEADER_SIZE) > SND_MODEL_DATA_SIZE) {
		voc_err("Size overflow.(%d>%d)\r\n", size, SND_MODEL_DATA_SIZE);
		return -EINVAL;
	}

	if (copy_to_user(data, card_drvdata->snd_model_array, size)) {
		voc_err("%s(), copy_to_user fail", __func__);
		ret = -EFAULT;
	}
	return ret;
}

static int voc_soc_card_put_sound_model(struct snd_kcontrol *kcontrol,
						const unsigned int __user *data,
						unsigned int size)
{
	int ret = 0;
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct voc_soc_card_drvdata *card_drvdata =
					snd_soc_card_get_drvdata(card);

	if ((size - TLV_HEADER_SIZE) > SND_MODEL_DATA_SIZE) {
		voc_err("Size overflow.(%d>%d)\r\n", size, SND_MODEL_DATA_SIZE);
		return -EINVAL;
	}

	if (copy_from_user(card_drvdata->snd_model_array, data, size)) {
		voc_err("%s(), copy_to_user fail", __func__);
		ret = -EFAULT;
	}

	voc_update_snd_model(
				card_drvdata->rpmsg_dev,
				card_drvdata->snd_model_paddr,
				size - TLV_HEADER_SIZE);

	return ret;
}

///sys/devices/platform/voc-audio/self_Diagnostic
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
	struct voc_soc_card_drvdata *card_drvdata =
					dev_get_drvdata(dev);

	voc_enable_slt_test(card_drvdata->rpmsg_dev, 0, 0);
	ret = card_drvdata->status;// 1: pass, otherwise: fail
	voc_dev_info(dev, "%s status :%d", __func__, ret);
#endif
	return scnprintf(buf, MAX_LEN, "%s\n", (ret)?"PASS":"FAIL");
}

static ssize_t self_Diagnostic_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct voc_soc_card_drvdata *card_drvdata =
					dev_get_drvdata(dev);
	long type = 0;
	int ret = 0;
	uint32_t addr = card_drvdata->voc_buf_paddr
					+ card_drvdata->snd_model_offset;

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
	if (type > 0) {
		voc_enable_slt_test(card_drvdata->rpmsg_dev, type, addr);
	} else if (type == 0) {
		voc_enable_slt_test(card_drvdata->rpmsg_dev, type, 0);
		ret = card_drvdata->status;// 1: pass, otherwise: fail
		voc_dev_info(dev, "%s\n", (ret)?"PASS":"FAIL");
	} else
		voc_dev_err(dev, "type is not support %ld\n", type);
#endif
	return count;
}
static DEVICE_ATTR_RW(self_Diagnostic);
///sys/devices/platform/voc-audio/log_level
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
static ssize_t help_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return scnprintf(buf, MAX_LEN, "Debug Help:\n"
				"- log_level <RW>: To control debug log level.\n"
				"                  Read log_level value.\n");
}
static DEVICE_ATTR_RO(help);
static struct attribute *mtk_dtv_snd_machine_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_log_level.attr,
	NULL,
};

static const struct attribute_group mtk_dtv_snd_machine_attr_group = {
	.name = "mtk_dbg",
	.attrs = mtk_dtv_snd_machine_attrs
};
static int mtk_dtv_snd_machine_create_sysfs_attr(struct platform_device *pdev)
{
	int ret = 0;

	/* Create device attribute files */
	voc_dev_info(&(pdev->dev), "Create device attribute files\n");
	ret = sysfs_create_group(&pdev->dev.kobj, &mtk_dtv_snd_machine_attr_group);
	if (ret)
		voc_dev_err(&pdev->dev, "[%d]Fail to create sysfs files: %d\n", __LINE__, ret);
	return ret;
}

static int mtk_dtv_snd_machine_remove_sysfs_attr(struct platform_device *pdev)
{
	/* Remove device attribute files */
	voc_dev_info(&(pdev->dev), "Remove device attribute files\n");
	sysfs_remove_group(&pdev->dev.kobj, &mtk_dtv_snd_machine_attr_group);
	return 0;
}
static int voc_soc_card_probe(struct snd_soc_card *card)
{
	struct voc_soc_card_drvdata *card_drvdata =
					snd_soc_card_get_drvdata(card);
	int ret = 0;

	dev_set_drvdata(card->dev, card_drvdata);

	ret = device_create_file(card->dev, &dev_attr_self_Diagnostic);
	if (ret != 0)
		voc_dev_err(card->dev, "%s Create self diagnostic attr fail (%d)\n", __func__, ret);

	return ret;
}

int voc_soc_card_suspend_pre(struct snd_soc_card *card)
{
	voc_debug("%s\r\n", __func__);
	return 0;
}

int voc_soc_card_suspend_post(struct snd_soc_card *card)
{
	voc_debug("%s\r\n", __func__);
	return 0;
}

int voc_soc_shutdown(void)
{
	voc_debug("%s\r\n", __func__);
	return 0;
}

int voc_soc_card_resume_pre(struct snd_soc_card *card)
{
	voc_debug("%s\r\n", __func__);
	return 0;
}

int voc_soc_card_resume_post(struct snd_soc_card *card)
{
	voc_debug("%s\r\n", __func__);
	return 0;
}

/*
 * TODO: Maybe we should use other mechanism (deferred probe mechanism or
 *       somthing like that) for binding rpmsg device driver and pwm chip device
 *       driver.
 */
static int voc_soc_card_add_thread(void *param)
{
	struct voc_soc_card_drvdata *card_drvdata =
			(struct voc_soc_card_drvdata *)param;

	voc_dev_info(card_drvdata->dev, "Wait voc soc card and rpmsg device binding...\n");

	// complete when rpmsg device and voc soc card device binded
	wait_for_completion(&card_drvdata->rpmsg_bind);
	voc_dev_info(card_drvdata->dev, "voc soc card and rpmsg device binded\n");

	return 0;
}

static struct snd_soc_dai_link voc_soc_dais[] = {
	{
		.name = "Voc Soc Dai Link",
		SND_SOC_DAILINK_REG(voc),
	}
};

static const struct snd_kcontrol_new voc_snd_controls[] = {
	SOC_ENUM_VOC("Mic switch", mic_onoff_enum),
	SOC_ENUM_VOC("Voice WakeUp Switch", vq_onoff_enum),
	SOC_ENUM_VOC("Seamless Mode", seamless_onoff_enum),
	SOC_SINGLE_EXT("Mic Number", VOC_REG_DMIC_NUMBER, 0, 8, 0,
		       voc_soc_card_get_volsw, voc_soc_card_put_volsw),
	SOC_SINGLE_EXT("Mic Gain Step", VOC_REG_MIC_GAIN, 0, 480, 0,
		       voc_soc_card_get_volsw, voc_soc_card_put_volsw),
	SOC_ENUM_VOC("Mic Bitwidth", mic_bitwidth_enum),
	SOC_ENUM_VOC("HPF Switch", hpf_onoff_enum),
	SOC_ENUM_VOC("HPF Coef", hpf_config_enum),
	SOC_ENUM_VOC("Sigen Switch", sigen_onoff_enum),
	SND_SOC_BYTES_TLV("Sound Model", SND_MODEL_SIZE,
				voc_soc_card_get_sound_model,
				voc_soc_card_put_sound_model),
	SOC_SINGLE_EXT("Hotword Version", VOC_REG_HOTWORD_VER, 0, 0x7FFFFFFF, 0,
				voc_soc_card_get_volsw, NULL),
};

static struct snd_soc_card voc_soc_card = {
	.name = "voc_snd_card",
	.owner = THIS_MODULE,
	.dai_link = voc_soc_dais,
	.num_links = 1,
	.probe = voc_soc_card_probe,

	.controls = voc_snd_controls,
	.num_controls = ARRAY_SIZE(voc_snd_controls),

	.suspend_pre = voc_soc_card_suspend_pre,
	.suspend_post = voc_soc_card_suspend_post,
	.resume_pre = voc_soc_card_resume_pre,
	.resume_post = voc_soc_card_resume_post,
};

static int voc_audio_probe(struct platform_device *pdev)
{
	struct voc_soc_card_drvdata *card_drvdata;
	static struct task_struct *t;
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	u32 snd_model_offset;
//	u32 snd_model_size;
	u32 voc_buf_paddr, voc_buf_size;
	u32 vbdma_ch;
#ifdef SLT_TEMP
	u32 addr;
	u32 imi_base_addr[2];
	int size;
	unsigned char *riu_map = NULL;
#endif

	/* init voc_soc_card_drvdata device struct */
	card_drvdata = devm_kzalloc(&pdev->dev,
					sizeof(struct voc_soc_card_drvdata),
					GFP_KERNEL);

	if (IS_ERR(card_drvdata)) {
		voc_dev_err(&pdev->dev, "unable to allocate memory (%ld)\n",
			PTR_ERR(card_drvdata));
		return PTR_ERR(card_drvdata);
	}

#ifdef DMA_BUFFER_IN_RESERVED_MEM
	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		voc_dev_err(&pdev->dev, "init reserved memory failed\n");
		return ret;
	}
#endif

	of_property_read_u32(np, "vbdma-ch", &vbdma_ch);
	card_drvdata->vbdma_ch = vbdma_ch;

	of_property_read_u32_index(np, "voc-buf-info", 0, &voc_buf_paddr);
	card_drvdata->voc_buf_paddr = voc_buf_paddr;

	of_property_read_u32_index(np, "voc-buf-info", 1, &voc_buf_size);
	card_drvdata->voc_buf_size = voc_buf_size;

	of_property_read_u32(np, "snd-model-offset", &snd_model_offset);
	card_drvdata->snd_model_offset = snd_model_offset;

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

	card_drvdata->card_reg = card_reg;
	platform_set_drvdata(pdev, card_drvdata);
	snd_soc_card_set_drvdata(&voc_soc_card, card_drvdata);

	// add to voc_soc_card list
	INIT_LIST_HEAD(&card_drvdata->list);
	list_add_tail(&card_drvdata->list, &voc_soc_card_dev_list);
	init_completion(&card_drvdata->rpmsg_bind);

	// check if there is a valid rpmsg_dev
	ret = _mtk_bind_rpmsg_voc_soc_card(NULL, card_drvdata);
	if (ret) {
		voc_dev_err(&pdev->dev, "binding rpmsg-voc failed! (%d)\n", ret);
		goto bind_fail;
	}

	card_drvdata->dev = &pdev->dev;
	voc_soc_card.dev = &pdev->dev; //register voc soc card

	/*
	 * TODO: check probe behavior match definition of platform driver
	 *       because initialization is not complete
	 */
	/* thread for waiting rpmsg binded, continue rest initial sequence after
	 * rpmsg device binded.
	 */

	t = kthread_run(voc_soc_card_add_thread,
			(void *)card_drvdata, "voc_soc_card_add");
	if (IS_ERR(t)) {
		voc_dev_err(&pdev->dev, "create thread for add voc sound card failed!\n");
		ret = IS_ERR(t);
		goto bind_fail;
	}

	mtk_dtv_snd_machine_create_sysfs_attr(pdev);
	return snd_soc_register_card(&voc_soc_card);

bind_fail:
	list_del(&card_drvdata->list);
	return ret;
}

int voc_audio_remove(struct platform_device *pdev)
{
	struct voc_soc_card_drvdata *card_drvdata = platform_get_drvdata(pdev);

	voc_debug("%s\r\n", __func__);

#ifdef DMA_BUFFER_IN_RESERVED_MEM
	of_reserved_mem_device_release(&pdev->dev);
#endif

	list_del(&card_drvdata->list);
	mtk_dtv_snd_machine_remove_sysfs_attr(pdev);
	return snd_soc_unregister_card(&voc_soc_card);
}

#ifdef CONFIG_PM_SLEEP
static int voc_dapm_suspend(struct device *dev)
{
	voc_dev_info(dev, "%s, record pmu voice setting\r\n", __func__);
	memcpy(card_reg_backup, card_reg, VOC_REG_LEN);

	return 0;
}

static int voc_dapm_resume(struct device *dev)
{
	struct voc_soc_card_drvdata *card_drvdata =
					dev_get_drvdata(dev);

	voc_dev_info(dev, "%s\n", __func__);
	//Be careful!! We have to disable VQ before reloading settings.
	//It's possible that VQ still keeps enabled
	//because wake up source is not CM4.

	// reload setting
	voc_dev_info(dev, "%s, reload pmu voice setting\r\n", __func__);
	if (card_reg[VOC_REG_SEAMLESS_ONOFF]) {
		voc_info("%s, don't reload CM4 setting\r\n", __func__);
	} else {
		voc_reset_voice(card_drvdata->rpmsg_dev);  // reset pmu voice task

	}

	return 0;
}
#endif

static const struct of_device_id mtk_mic_machine_dt_match[] = {
	{ .compatible = "mediatek,mtk-mic-machine" },
	{}
};
MODULE_DEVICE_TABLE(of, mtk_mic_machine_dt_match);

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops voc_snd_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(voc_dapm_suspend, voc_dapm_resume)
};
#endif

static struct platform_driver voc_audio_driver = {
	.driver = {
			.name = "voc-audio",
			.owner = THIS_MODULE,
			//.pm = &snd_soc_pm_ops,
#ifdef CONFIG_PM_SLEEP
			.pm = &voc_snd_pm_ops,
#endif
			.of_match_table = mtk_mic_machine_dt_match,
			},
	.probe = voc_audio_probe,
	.remove = voc_audio_remove,
};
module_platform_driver(voc_audio_driver);

/* Module information */
MODULE_DESCRIPTION("Voice Audio ASLA SoC Machine");
MODULE_AUTHOR("Allen-HW Hsu, allen-hw.hsu@mediatek.com");
MODULE_LICENSE("GPL");
