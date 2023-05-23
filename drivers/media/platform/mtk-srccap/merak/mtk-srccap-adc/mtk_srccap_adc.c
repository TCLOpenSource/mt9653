// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ctrls.h>
#include <asm-generic/div64.h>

#include "mtk_srccap.h"
#include "mtk_srccap_adc.h"
#include "hwreg_srccap_adc.h"

/* ============================================================================================== */
/* -------------------------------------- Local Functions --------------------------------------- */
/* ============================================================================================== */
static enum reg_srccap_adc_pws_type adc_map_pws_reg(
	enum srccap_adc_source_info src)
{
	switch (src) {
	/* Non-ADC source case */
	case SRCCAP_ADC_SOURCE_NONADC:
		return REG_SRCCAP_ADC_PWS_NONE;
	/* Single source case */
	case SRCCAP_ADC_SOURCE_ONLY_CVBS:
		return REG_SRCCAP_ADC_PWS_CVBS;
	case SRCCAP_ADC_SOURCE_ONLY_SVIDEO:
		return REG_SRCCAP_ADC_PWS_SVIDEO;
	case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
		return REG_SRCCAP_ADC_PWS_YPBPR;
	case SRCCAP_ADC_SOURCE_ONLY_RGB:
		return REG_SRCCAP_ADC_PWS_RGB;
	case SRCCAP_ADC_SOURCE_ONLY_INT_DMD_ATV:
		return REG_SRCCAP_ADC_PWS_ATV_SSIF;
	case SRCCAP_ADC_SOURCE_ONLY_EXT_DMD_ATV:
		return REG_SRCCAP_ADC_PWS_ATV_VIF;
	case SRCCAP_ADC_SOURCE_ONLY_SCART_RGB:
	case SRCCAP_ADC_SOURCE_ONLY_SCART_Y:
		return REG_SRCCAP_ADC_PWS_SCART;
	/* Multi source case */
	case SRCCAP_ADC_SOURCE_MULTI_RGB_ATV:
		return REG_SRCCAP_ADC_PWS_RGB;
	case SRCCAP_ADC_SOURCE_MULTI_RGB_SCARTY:
		return REG_SRCCAP_ADC_PWS_SCART_RGB;
	case SRCCAP_ADC_SOURCE_MULTI_RGB_CVBS:
		return REG_SRCCAP_ADC_PWS_CVBS_RGB;
	case SRCCAP_ADC_SOURCE_MULTI_YPBPR_ATV:
		return REG_SRCCAP_ADC_PWS_YPBPR;
	case SRCCAP_ADC_SOURCE_MULTI_YPBPR_SCARTY:
		return REG_SRCCAP_ADC_PWS_SCART_YPBPR;
	case SRCCAP_ADC_SOURCE_MULTI_YPBPR_CVBS:
		return REG_SRCCAP_ADC_PWS_CVBS_YPBPR;
	default:
		SRCCAP_MSG_ERROR("invalid source type\n");
		return REG_SRCCAP_ADC_INPUTSOURCE_NONE;
	}
}

static enum reg_srccap_adc_input_port adc_map_port_reg(
	enum srccap_input_port port)
{
	switch (port) {
	case SRCCAP_INPUT_PORT_NONE:
		return REG_SRCCAP_ADC_INPUT_PORT_NONE;
	case SRCCAP_INPUT_PORT_RGB0_DATA:
		return REG_SRCCAP_ADC_INPUT_PORT_RGB0_DATA;
	case SRCCAP_INPUT_PORT_RGB1_DATA:
		return REG_SRCCAP_ADC_INPUT_PORT_RGB1_DATA;
	case SRCCAP_INPUT_PORT_RGB2_DATA:
		return REG_SRCCAP_ADC_INPUT_PORT_RGB2_DATA;
	case SRCCAP_INPUT_PORT_RGB0_SYNC:
		return REG_SRCCAP_ADC_INPUT_PORT_RGB0_SYNC;
	case SRCCAP_INPUT_PORT_RGB1_SYNC:
		return REG_SRCCAP_ADC_INPUT_PORT_RGB1_SYNC;
	case SRCCAP_INPUT_PORT_RGB2_SYNC:
		return REG_SRCCAP_ADC_INPUT_PORT_RGB2_SYNC;
	case SRCCAP_INPUT_PORT_YCVBS0:
		return REG_SRCCAP_ADC_INPUT_PORT_YCVBS0;
	case SRCCAP_INPUT_PORT_YCVBS1:
		return REG_SRCCAP_ADC_INPUT_PORT_YCVBS1;
	case SRCCAP_INPUT_PORT_YCVBS2:
		return REG_SRCCAP_ADC_INPUT_PORT_YCVBS2;
	case SRCCAP_INPUT_PORT_YCVBS3:
		return REG_SRCCAP_ADC_INPUT_PORT_YCVBS3;
	case SRCCAP_INPUT_PORT_YG0:
		return REG_SRCCAP_ADC_INPUT_PORT_YG0;
	case SRCCAP_INPUT_PORT_YG1:
		return REG_SRCCAP_ADC_INPUT_PORT_YG1;
	case SRCCAP_INPUT_PORT_YG2:
		return REG_SRCCAP_ADC_INPUT_PORT_YG2;
	case SRCCAP_INPUT_PORT_YR0:
		return REG_SRCCAP_ADC_INPUT_PORT_YR0;
	case SRCCAP_INPUT_PORT_YR1:
		return REG_SRCCAP_ADC_INPUT_PORT_YR1;
	case SRCCAP_INPUT_PORT_YR2:
		return REG_SRCCAP_ADC_INPUT_PORT_YR2;
	case SRCCAP_INPUT_PORT_YB0:
		return REG_SRCCAP_ADC_INPUT_PORT_YB0;
	case SRCCAP_INPUT_PORT_YB1:
		return REG_SRCCAP_ADC_INPUT_PORT_YB1;
	case SRCCAP_INPUT_PORT_YB2:
		return REG_SRCCAP_ADC_INPUT_PORT_YB2;
	case SRCCAP_INPUT_PORT_CCVBS0:
		return REG_SRCCAP_ADC_INPUT_PORT_CCVBS0;
	case SRCCAP_INPUT_PORT_CCVBS1:
		return REG_SRCCAP_ADC_INPUT_PORT_CCVBS1;
	case SRCCAP_INPUT_PORT_CCVBS2:
		return REG_SRCCAP_ADC_INPUT_PORT_CCVBS2;
	case SRCCAP_INPUT_PORT_CCVBS3:
		return REG_SRCCAP_ADC_INPUT_PORT_CCVBS3;
	case SRCCAP_INPUT_PORT_CR0:
		return REG_SRCCAP_ADC_INPUT_PORT_CR0;
	case SRCCAP_INPUT_PORT_CR1:
		return REG_SRCCAP_ADC_INPUT_PORT_CR1;
	case SRCCAP_INPUT_PORT_CR2:
		return REG_SRCCAP_ADC_INPUT_PORT_CR2;
	case SRCCAP_INPUT_PORT_CG0:
		return REG_SRCCAP_ADC_INPUT_PORT_CG0;
	case SRCCAP_INPUT_PORT_CG1:
		return REG_SRCCAP_ADC_INPUT_PORT_CG1;
	case SRCCAP_INPUT_PORT_CG2:
		return REG_SRCCAP_ADC_INPUT_PORT_CG2;
	case SRCCAP_INPUT_PORT_CB0:
		return REG_SRCCAP_ADC_INPUT_PORT_CB0;
	case SRCCAP_INPUT_PORT_CB1:
		return REG_SRCCAP_ADC_INPUT_PORT_CB1;
	case SRCCAP_INPUT_PORT_CB2:
		return REG_SRCCAP_ADC_INPUT_PORT_CB2;
	case SRCCAP_INPUT_PORT_DVI0:
		return REG_SRCCAP_ADC_INPUT_PORT_DVI0;
	case SRCCAP_INPUT_PORT_DVI1:
		return REG_SRCCAP_ADC_INPUT_PORT_DVI1;
	case SRCCAP_INPUT_PORT_DVI2:
		return REG_SRCCAP_ADC_INPUT_PORT_DVI2;
	case SRCCAP_INPUT_PORT_DVI3:
		return REG_SRCCAP_ADC_INPUT_PORT_DVI3;
	default:
		SRCCAP_MSG_ERROR("invalid port type\n");
		return REG_SRCCAP_ADC_INPUT_PORT_NONE;
	}
}

static enum reg_srccap_adc_input_source_type adc_map_source_reg(
	enum srccap_adc_source_info src)
{
	switch (src) {
	/* Non-ADC source case */
	case SRCCAP_ADC_SOURCE_NONADC:
		return REG_SRCCAP_ADC_INPUTSOURCE_NONE;
	/* Single source case */
	case SRCCAP_ADC_SOURCE_ONLY_CVBS:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_CVBS;
	case SRCCAP_ADC_SOURCE_ONLY_SVIDEO:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_SVIDEO;
	case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_YPBPR;
	case SRCCAP_ADC_SOURCE_ONLY_RGB:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_RGB;
	case SRCCAP_ADC_SOURCE_ONLY_INT_DMD_ATV:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_INT_DMD_ATV;
	case SRCCAP_ADC_SOURCE_ONLY_EXT_DMD_ATV:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_EXT_DMD_ATV;
	case SRCCAP_ADC_SOURCE_ONLY_SCART_RGB:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_SCART_RGB;
	case SRCCAP_ADC_SOURCE_ONLY_SCART_Y:
		return REG_SRCCAP_ADC_INPUTSOURCE_ONLY_SCART_Y;
	/* Multi source case */
	case SRCCAP_ADC_SOURCE_MULTI_RGB_ATV:
		return REG_SRCCAP_ADC_INPUTSOURCE_MULTI_RGB_ATV;
	case SRCCAP_ADC_SOURCE_MULTI_RGB_SCARTY:
		return REG_SRCCAP_ADC_INPUTSOURCE_MULTI_RGB_SCART;
	case SRCCAP_ADC_SOURCE_MULTI_RGB_CVBS:
		return REG_SRCCAP_ADC_INPUTSOURCE_MULTI_RGB_CVBS;
	case SRCCAP_ADC_SOURCE_MULTI_YPBPR_ATV:
		return REG_SRCCAP_ADC_INPUTSOURCE_MULTI_YPBPR_ATV;
	case SRCCAP_ADC_SOURCE_MULTI_YPBPR_SCARTY:
		return REG_SRCCAP_ADC_INPUTSOURCE_MULTI_YPBPR_SCART;
	case SRCCAP_ADC_SOURCE_MULTI_YPBPR_CVBS:
		return REG_SRCCAP_ADC_INPUTSOURCE_MULTI_YPBPR_CVBS;
	default:
		SRCCAP_MSG_ERROR("invalid source type\n");
		return REG_SRCCAP_ADC_INPUTSOURCE_NONE;
	}
}

static enum srccap_adc_source_info adc_map_analog_source(
	enum v4l2_srccap_input_source src)
{
	bool is_scart_rgb = 0;

	switch (src) {
	case V4L2_SRCCAP_INPUT_SOURCE_NONE:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI2:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI3:
	case V4L2_SRCCAP_INPUT_SOURCE_HDMI4:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI2:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI3:
	case V4L2_SRCCAP_INPUT_SOURCE_DVI4:
		return SRCCAP_ADC_SOURCE_NONADC;
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
		return SRCCAP_ADC_SOURCE_ONLY_CVBS;
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
		return SRCCAP_ADC_SOURCE_ONLY_SVIDEO;
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
		return SRCCAP_ADC_SOURCE_ONLY_YPBPR;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		return SRCCAP_ADC_SOURCE_ONLY_RGB;
	case V4L2_SRCCAP_INPUT_SOURCE_ATV:
		return SRCCAP_ADC_SOURCE_ONLY_INT_DMD_ATV; // need fix decide int/ext ATV
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
	{
		drv_hwreg_srccap_adc_get_scart_type(&is_scart_rgb);
		if (is_scart_rgb)
			return SRCCAP_ADC_SOURCE_ONLY_SCART_RGB;
		else
			return SRCCAP_ADC_SOURCE_ONLY_SCART_Y;
	}
	default:
		SRCCAP_MSG_ERROR("invalid source type\n");
		return SRCCAP_ADC_SOURCE_NONADC;
	}
}

static enum reg_srccap_adc_input_offline_mux adc_map_offline_port_reg(
	enum srccap_input_port port)
{
	switch (port) {
	case SRCCAP_INPUT_PORT_RGB0_DATA:
		return REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_G0;
	case SRCCAP_INPUT_PORT_RGB1_DATA:
		return REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_G1;
	case SRCCAP_INPUT_PORT_RGB0_SYNC:
		// TODO
	case SRCCAP_INPUT_PORT_YCVBS0:
		return REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_CVBS0;
	case SRCCAP_INPUT_PORT_YCVBS1:
		return REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_CVBS1;
	case SRCCAP_INPUT_PORT_YG0:
		return REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_G0;
	case SRCCAP_INPUT_PORT_YG1:
		return REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_G1;
	default:
		SRCCAP_MSG_ERROR("invalid port type\n");
		return REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_NUM;
	}
}

static int adc_set_hpol(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u16 h_pol = srccap_dev->timingdetect_info.data.h_pol;

	ret = drv_hwreg_srccap_adc_set_hpol(h_pol, true, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return 0;
}

static int adc_set_plldiv(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u16 htt = srccap_dev->timingdetect_info.data.h_total;

	if ((htt > 3) && (htt < SRCCAP_ADC_MAX_CLK)) {
		ret = drv_hwreg_srccap_adc_set_plldiv((htt - 3), TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	return 0;
}

static int adc_set_plla_reset(void)
{
	int ret = 0;

	/* toggle to reset */
	ret = drv_hwreg_srccap_adc_set_plla_reset(TRUE, TRUE, NULL, NULL);
	if (ret < 0) {
		SRCCAP_MSG_INFO("[ADC] Set plla reset failed\n");
		return ret;
	}

	// sleep 500us
	usleep_range(SRCCAP_ADC_UDELAY_500, SRCCAP_ADC_UDELAY_510);

	ret = drv_hwreg_srccap_adc_set_plla_reset(FALSE, TRUE, NULL, NULL);
	if (ret < 0)
		SRCCAP_MSG_INFO("[ADC] toggle plla reset failed\n");

	return ret;
}

static int adc_set_fixed_gain_offset_phase(void)
{
	int ret = 0;
	u16 adcpllmod = 0;
	struct reg_srccap_adc_rgb_offset fixed_offset;
	struct reg_srccap_adc_rgb_gain fixed_gain;

	memset(&fixed_offset, 0, sizeof(struct reg_srccap_adc_rgb_offset));
	memset(&fixed_offset, 0, sizeof(struct reg_srccap_adc_rgb_offset));

	SRCCAP_MSG_INFO("[ADC] set fixed gain, R=%u, G=%u, B=%u\n",
		ADC_YPBPR_FIXED_GAIN_R,
		ADC_YPBPR_FIXED_GAIN_G,
		ADC_YPBPR_FIXED_GAIN_B);
	fixed_gain.r_gain = ADC_YPBPR_FIXED_GAIN_R;
	fixed_gain.g_gain = ADC_YPBPR_FIXED_GAIN_G;
	fixed_gain.b_gain = ADC_YPBPR_FIXED_GAIN_B;
	ret = drv_hwreg_srccap_adc_set_rgb_gain(fixed_gain, true, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("set rgb gain fail\n");
		return ret;
	}

	SRCCAP_MSG_INFO("[ADC] set fixed offset, R=%u, G=%u, B=%u\n",
		ADC_YPBPR_FIXED_OFFSET_R,
		ADC_YPBPR_FIXED_OFFSET_G,
		ADC_YPBPR_FIXED_OFFSET_B);
	fixed_offset.r_offset = ADC_YPBPR_FIXED_OFFSET_R;
	fixed_offset.g_offset = ADC_YPBPR_FIXED_OFFSET_G;
	fixed_offset.b_offset = ADC_YPBPR_FIXED_OFFSET_B;
	ret = drv_hwreg_srccap_adc_set_rgb_offset(fixed_offset, true, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("set fixed offset fail\n");
		return ret;
	}

	SRCCAP_MSG_INFO("[ADC] set fixed phase = %u\n", ADC_YPBPR_FIXED_PHASE);

	ret = drv_hwreg_srccap_adc_get_adcpllmod(&adcpllmod);
	if (ret) {
		SRCCAP_MSG_ERROR("Get adcpllmod failed\n");
		return ret;
	}

	ret = drv_hwreg_srccap_adc_set_phase(ADC_YPBPR_FIXED_PHASE, adcpllmod, true, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("set fixed phase fail\n");
		return ret;
	}

	return ret;
}

static bool adc_check_same_port(
	enum srccap_input_port port1,
	enum srccap_input_port port2)
{
	bool result = FALSE;

	if (port1 == port2)
		return TRUE;

	switch (port1) {
	case SRCCAP_INPUT_PORT_RGB0_DATA:
		if (port2 == SRCCAP_INPUT_PORT_YG0)
			result = TRUE;
		break;
	case SRCCAP_INPUT_PORT_RGB1_DATA:
		if (port2 == SRCCAP_INPUT_PORT_YG1)
			result = TRUE;
		break;
	case SRCCAP_INPUT_PORT_RGB2_DATA:
		if (port2 == SRCCAP_INPUT_PORT_YG2)
			result = TRUE;
		break;
	case SRCCAP_INPUT_PORT_YG0:
		if (port2 == SRCCAP_INPUT_PORT_RGB0_DATA)
			result = TRUE;
		break;
	case SRCCAP_INPUT_PORT_YG1:
		if (port2 == SRCCAP_INPUT_PORT_RGB1_DATA)
			result = TRUE;
		break;
	case SRCCAP_INPUT_PORT_YG2:
		if (port2 == SRCCAP_INPUT_PORT_RGB2_DATA)
			result = TRUE;
		break;
	default:
		SRCCAP_MSG_ERROR("unsupported port1=%d port2=%d\n", port1, port2);
		break;
	}
	return result;
}

static int adc_check_port(
	struct mtk_srccap_dev *srccap_dev,
	struct v4l2_adc_source *source,
	enum srccap_input_port *port_container,
	u8 *port_cnt)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (source == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (port_container == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (port_cnt == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int i = 0;
	int j = 0;
	u8 cnt = 0;
	enum srccap_input_port *port = NULL;

	SRCCAP_MSG_INFO("source count = %u\n", source->src_count);

	port = kzalloc((sizeof(enum srccap_input_port) * source->src_count * 2), GFP_KERNEL);
	if (!port) {
		SRCCAP_MSG_ERROR("kzalloc fail\n");
		return -ENOMEM;
	}

	// Get data & sync port used by each source
	for (i = 0; i < source->src_count; i++) {
		port[i*2] = srccap_dev->srccap_info.map[source->p.adc_input_src[i]].data_port;
		port[i*2+1] = srccap_dev->srccap_info.map[source->p.adc_input_src[i]].sync_port;
		SRCCAP_MSG_INFO("adc_input_src[%d] = %u\n", i, source->p.adc_input_src[i]);
		SRCCAP_MSG_INFO("port[%d](data) = %u\n", i*2, port[i*2]);
		SRCCAP_MSG_INFO("port[%d](sync) = %u\n", i*2+1, port[i*2+1]);
	}

	// Check if multi-source data/sync used same port
	for (i = 0; i < source->src_count * 2; i++) {
		if (port[i] != 0) {
			for (j = i+1; j < source->src_count * 2; j++) {
				if (adc_check_same_port(port[i], port[j])) {
					SRCCAP_MSG_ERROR(
						"Conflict pad occur! pad[%d]= pad[%d]= %u\n",
						i, j, port[i]);
					kfree(port);
					return -EPERM;
				}
			}
			port_container[cnt] = port[i];
			cnt++;
		}
	}

	SRCCAP_MSG_INFO("adc check port pass! src_count:%d\n", source->src_count);
	*port_cnt = cnt;
	kfree(port);
	return 0;
}

static int adc_split_src(
	enum srccap_adc_source_info adc_src,
	enum v4l2_srccap_input_source *v4l2_src0,
	enum v4l2_srccap_input_source *v4l2_src1)
{
	if (v4l2_src0 == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (v4l2_src1 == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if ((adc_src < SRCCAP_ADC_SOURCE_NONADC) || (adc_src >= SRCCAP_ADC_SOURCE_ALL_NUMS)) {
		SRCCAP_MSG_ERROR("Invalid adc source : %d\n", adc_src);
		return -EINVAL;
	}

	if (adc_src == SRCCAP_ADC_SOURCE_NONADC) {
		*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_NONE;
		*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_NONE;
	} else {
		switch (adc_src) {
		case SRCCAP_ADC_SOURCE_ONLY_CVBS:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_CVBS2;
			break;
		case SRCCAP_ADC_SOURCE_ONLY_SVIDEO:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2;
			break;
		case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_YPBPR2;
			break;
		case SRCCAP_ADC_SOURCE_ONLY_RGB:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_VGA;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_VGA2;
			break;
		case SRCCAP_ADC_SOURCE_ONLY_SCART_RGB:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_SCART;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_SCART2;
			break;
		case SRCCAP_ADC_SOURCE_ONLY_INT_DMD_ATV:
		case SRCCAP_ADC_SOURCE_ONLY_EXT_DMD_ATV:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_ATV;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_NONE;
			break;
		case SRCCAP_ADC_SOURCE_ONLY_SCART_Y:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_SCART;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_SCART2;
			break;
		case SRCCAP_ADC_SOURCE_MULTI_RGB_ATV:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_VGA;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_ATV;
			break;
		case SRCCAP_ADC_SOURCE_MULTI_RGB_SCARTY:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_VGA;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_SCART;
			break;
		case SRCCAP_ADC_SOURCE_MULTI_RGB_CVBS:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_VGA;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
			break;
		case SRCCAP_ADC_SOURCE_MULTI_YPBPR_ATV:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_ATV;
			break;
		case SRCCAP_ADC_SOURCE_MULTI_YPBPR_SCARTY:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_SCART;
			break;
		case SRCCAP_ADC_SOURCE_MULTI_YPBPR_CVBS:
			*v4l2_src0 = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
			*v4l2_src1 = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
			break;
		default:
			SRCCAP_MSG_ERROR("unsupport adc source : %d\n", adc_src);
			return -EPERM;
		}
	}

	return 0;
}


static int adc_merge_dual_src(
	enum srccap_adc_source_info src0,
	enum srccap_adc_source_info src1,
	enum srccap_adc_source_info *result)
{
	if (result == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if ((src0 < SRCCAP_ADC_SOURCE_NONADC) || (src0 >= SRCCAP_ADC_SOURCE_SINGLE_NUMS)) {
		SRCCAP_MSG_ERROR("Invalid source 0: %d\n", src0);
		return -EINVAL;
	}
	if ((src1 < SRCCAP_ADC_SOURCE_NONADC) || (src1 >= SRCCAP_ADC_SOURCE_SINGLE_NUMS)) {
		SRCCAP_MSG_ERROR("Invalid source 1: %d\n", src1);
		return -EINVAL;
	}

	enum srccap_adc_source_info temp_src = SRCCAP_ADC_SOURCE_NONADC;
	static int _adc_dual_src_table[][SRCCAP_ADC_SOURCE_SINGLE_NUMS] = {
		/*  NON   CVBS   SVDO  YPBPR    VGA  ATV_I  ATV_E   SC_R   SC_Y */
		{  true,  true,  true,  true,  true,  true,  true,  true,  true,}, /*NONADC*/
		{  true, false, false,  true,  true,  true, false, false, false,}, /*CVBS*/
		{  true, false, false, false, false, false, false, false, false,}, /*SVIDEO*/
		{  true,  true, false, false, false,  true,  true, false,  true,}, /*YPBPR*/
		{  true,  true, false, false, false,  true,  true, false,  true,}, /*VGA*/
		{  true,  true, false,  true,  true,  true,  true, false,  true,}, /*INTATV*/
		{  true, false, false,  true,  true,  true, false, false, false,}, /*EXTATV*/
		{  true, false, false, false, false, false, false, false, false,}, /*SCARTR*/
		{  true, false, false,  true,  true,  true, false, false, false,}, /*SCARTY*/
	};

	SRCCAP_MSG_INFO("[ADC] src0=%d, src1=%d\n", src0, src1);

	// sort first to reduce complexity
	if (src0 > src1) {
		temp_src = src0;
		src0 = src1;
		src1 = temp_src;
	}

	if (_adc_dual_src_table[src0][src1] == true) {
		if (src0 == SRCCAP_ADC_SOURCE_NONADC) {
			*result = src1;
			goto SUCCESS;
		} else if (src1 == SRCCAP_ADC_SOURCE_NONADC) {
			*result = src0;
			goto SUCCESS;
		} else {
			switch (src0) {
			case SRCCAP_ADC_SOURCE_ONLY_CVBS:
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_YPBPR) {
					*result = SRCCAP_ADC_SOURCE_MULTI_YPBPR_CVBS;
					goto SUCCESS;
				}
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_RGB) {
					*result = SRCCAP_ADC_SOURCE_MULTI_RGB_CVBS;
					goto SUCCESS;
				}
				break;
			case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_INT_DMD_ATV) {
					*result = SRCCAP_ADC_SOURCE_MULTI_YPBPR_ATV;
					goto SUCCESS;
				}
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_EXT_DMD_ATV) {
					*result = SRCCAP_ADC_SOURCE_MULTI_YPBPR_ATV;
					goto SUCCESS;
				}
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_SCART_Y) {
					*result = SRCCAP_ADC_SOURCE_MULTI_YPBPR_SCARTY;
					goto SUCCESS;
				}
				break;
			case SRCCAP_ADC_SOURCE_ONLY_RGB:
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_INT_DMD_ATV) {
					*result = SRCCAP_ADC_SOURCE_MULTI_RGB_ATV;
					goto SUCCESS;
				}
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_EXT_DMD_ATV) {
					*result = SRCCAP_ADC_SOURCE_MULTI_RGB_ATV;
					goto SUCCESS;
				}
				if (src1 == SRCCAP_ADC_SOURCE_ONLY_SCART_Y) {
					*result = SRCCAP_ADC_SOURCE_MULTI_RGB_SCARTY;
					goto SUCCESS;
				}
				break;
			}
		}
	}

	SRCCAP_MSG_ERROR("Not support multi source case!, src0= (%d), src1= (%d)\n", src0, src1);
	return -EPERM;

SUCCESS:
	SRCCAP_MSG_INFO("[ADC] source decide = (%d)\n", *result);
	return 0;
}

static int adc_get_sog_mux_sel(
	enum srccap_input_port data_port0,
	enum srccap_input_port data_port1,
	enum srccap_adc_sog_online_mux *sog_mux)
{
	if (sog_mux == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (data_port0 == SRCCAP_INPUT_PORT_RGB0_DATA ||
		data_port1 == SRCCAP_INPUT_PORT_RGB0_DATA) {
		*sog_mux = SRCCAP_SOG_ONLINE_PADA_GIN0P;

		goto result;
	} else if (data_port0 == SRCCAP_INPUT_PORT_RGB1_DATA ||
		data_port1 == SRCCAP_INPUT_PORT_RGB1_DATA) {
		*sog_mux = SRCCAP_SOG_ONLINE_PADA_GIN1P;

		goto result;
	} else if (data_port0 == SRCCAP_INPUT_PORT_RGB2_DATA ||
		data_port1 == SRCCAP_INPUT_PORT_RGB2_DATA) {
		*sog_mux = SRCCAP_SOG_ONLINE_PADA_GIN2P;

		goto result;
	} else {
		*sog_mux = SRCCAP_SOG_ONLINE_PADA_GND;
		goto result;
	}

result:
	SRCCAP_MSG_INFO("[ADC] sog sel: %u\n", *sog_mux);
	return 0;
}

static int mtk_adc_set_sog_online_mux(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_adc_sog_online_mux muxsel)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;

	ret = drv_hwreg_srccap_adc_set_sog_online_mux(muxsel, true, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("set sog online mux fail\n");
		return ret;
	}

	return ret;
}

int adc_set_src_setting(
	struct v4l2_adc_source *source,
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (source == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	int idx = 0;
	u8 port_cnt = 0;
	enum srccap_adc_source_info adc_src[] = {
		SRCCAP_ADC_SOURCE_NONADC, SRCCAP_ADC_SOURCE_NONADC};
	enum srccap_input_port *port_used = NULL;
	enum reg_srccap_adc_input_port reg_port = 0;
	enum reg_srccap_adc_input_source_type reg_src_type = 0;
	enum srccap_adc_sog_online_mux sog_mux = 0;

	/* check src count */
	if (source->src_count > srccap_dev->srccap_info.cap.ipdma_cnt ||
		source->src_count <= 0 ||
		source->src_count > srccap_dev->srccap_info.cap.adc_multi_src_max_num) {
		SRCCAP_MSG_ERROR("Invalid src_count: %u, ipdma_cnt: %u, adc max num: %u\n",
			source->src_count,
			srccap_dev->srccap_info.cap.ipdma_cnt,
			srccap_dev->srccap_info.cap.adc_multi_src_max_num);
		return -EINVAL;
	}

	/* check adc port capability */
	port_used = kzalloc((sizeof(enum srccap_input_port)
		* srccap_dev->srccap_info.cap.ipdma_cnt * 2), GFP_KERNEL);
	if (!port_used) {
		SRCCAP_MSG_ERROR("kzalloc fail\n");
		return -ENOMEM;
	}
	ret = adc_check_port(srccap_dev, source, port_used, &port_cnt);
	if (ret) {
		SRCCAP_MSG_ERROR("adc_check_port fail!\n");
		goto out;
	}

	/* check multi-src hw capability, only support dual sources for now */
	for (idx = 0; idx < source->src_count; idx++)
		adc_src[idx] = adc_map_analog_source(source->p.adc_input_src[idx]);

	ret = adc_get_sog_mux_sel(srccap_dev->srccap_info.map[source->p.adc_input_src[0]].data_port,
		srccap_dev->srccap_info.map[source->p.adc_input_src[1]].data_port, &sog_mux);
	if (ret) {
		SRCCAP_MSG_ERROR("adc_get_sog_mux_sel fail!\n");
		goto out;
	}

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_online_source);

	ret = adc_merge_dual_src(adc_src[0], adc_src[1], &srccap_dev->adc_info.adc_src);
	if (ret) {
		SRCCAP_MSG_ERROR("adc_merge_dual_src fail!\n");
		mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_online_source);
		goto out;
	}

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_online_source);

	/* set mux table */
	for (idx = 0; idx < port_cnt; idx++) {
		reg_port = adc_map_port_reg(port_used[idx]);
		SRCCAP_MSG_INFO("[ADC] set mux table idx: (%d)\n", reg_port);
		if (reg_port != REG_SRCCAP_ADC_INPUT_PORT_NONE) {
			ret = drv_hwreg_srccap_adc_set_mux(reg_port);
			if (ret) {
				SRCCAP_MSG_ERROR("load mux table fail\n");
				goto out;
			}
		}
	}

	/* set SOG online mux */
	ret = mtk_adc_set_sog_online_mux(srccap_dev, sog_mux);
	if (ret) {
		SRCCAP_MSG_ERROR("set sog online mux failed!\n");
		goto out;
	}

	/* set source table */
	reg_src_type = adc_map_source_reg(srccap_dev->adc_info.adc_src);
	SRCCAP_MSG_INFO("[ADC] set source table idx: (%d)\n", reg_src_type);
	ret = drv_hwreg_srccap_adc_set_source(reg_src_type);
	if (ret) {
		SRCCAP_MSG_ERROR("load source table fail\n");
		goto out;
	}

out:
	kfree(port_used);
	return ret;
}

static int adc_get_clk(
	struct device *dev,
	char *s,
	struct clk **clk)
{
	if (dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (s == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (clk == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	*clk = devm_clk_get(dev, s);
	if (IS_ERR(*clk)) {
		SRCCAP_MSG_FATAL("unable to retrieve %s\n", s);
		return PTR_ERR(*clk);
	}

	return 0;
}

static void adc_put_clk(
	struct device *dev,
	struct clk *clk)
{
	if (dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}
	if (clk == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return;
	}

	devm_clk_put(dev, clk);
}

static int adc_toggle_swen(
	struct mtk_srccap_dev *srccap_dev,
	char *clk_name,
	char *enable)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (clk_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (enable == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	struct clk *target_clk = NULL;

	ret = adc_get_clk(srccap_dev->dev, clk_name, &target_clk);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (strcmp(enable, "ON") == 0) {
		if (!__clk_is_enabled(target_clk)) {
			ret = clk_prepare_enable(target_clk);
			if (ret < 0) {
				SRCCAP_MSG_RETURN_CHECK(ret);
				adc_put_clk(srccap_dev->dev, target_clk);
				return ret;
			}
		}
	}
	if (strcmp(enable, "OFF") == 0) {
		if (__clk_is_enabled(target_clk))
			clk_disable_unprepare(target_clk);
	}

	adc_put_clk(srccap_dev->dev, target_clk);
	return 0;
}

static int adc_set_clk_swen(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u8 idx = 0;
	u16 count = 0;
	enum reg_srccap_adc_input_source_type src_type =
			adc_map_source_reg(srccap_dev->adc_info.adc_src);
	struct reg_srccap_adc_clk_info *clk_info = NULL;

	clk_info = (struct reg_srccap_adc_clk_info *)
			vzalloc(sizeof(struct reg_srccap_adc_clk_info) * ADCTBL_CLK_SWEN_NUM);
	if (clk_info == NULL) {
		SRCCAP_MSG_ERROR("vzalloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = drv_hwreg_srccap_adc_get_clk_swen(src_type, ADCTBL_CLK_SWEN_NUM, clk_info, &count);
	if (ret) {
		SRCCAP_MSG_ERROR("hwreg get clk swen fail\n");
		ret = -EPERM;
		goto exit;
	}

	SRCCAP_MSG_INFO("[ADC] CLK SWEN nums from ADC table is: %u\n", count);
	for (idx = 0; idx < count; idx++) {
		SRCCAP_MSG_INFO("[ADC] SWEN[%2u]: Name= %-40s, enable=%s\n",
			idx, clk_info[idx].clk_name, clk_info[idx].clk_data);

		ret = adc_toggle_swen(srccap_dev,
				clk_info[idx].clk_name,
				clk_info[idx].clk_data);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			SRCCAP_MSG_ERROR("adc_toggle_swen fail\n");
			goto exit;
		}
	}

	//Bellow clk swens are ctrled in driver, not from table
	ret = adc_toggle_swen(srccap_dev, "adctbl_swen_dig_2162adc", "ON");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto exit;
	}

exit:
	if (clk_info != NULL)
		vfree(clk_info);

	return ret;
}

static int adc_enable_parent(
	struct mtk_srccap_dev *srccap_dev,
	char *clk_name,
	char *parent_name)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (clk_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (parent_name == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	struct clk *target_clk = NULL;
	struct clk *parent = NULL;

	ret = adc_get_clk(srccap_dev->dev, clk_name, &target_clk);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto out3;
	}

	ret = adc_get_clk(srccap_dev->dev, parent_name, &parent);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto out2;
	}
	ret = clk_prepare_enable(target_clk);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto out1;
	}
	ret = clk_set_parent(target_clk, parent);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto out1;
	}

out1:
	adc_put_clk(srccap_dev->dev, parent);
out2:
	adc_put_clk(srccap_dev->dev, target_clk);
out3:
	return ret;
}

static int adc_set_clk(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u8 idx = 0;
	u16 count = 0;
	enum reg_srccap_adc_input_source_type src_type =
			adc_map_source_reg(srccap_dev->adc_info.adc_src);
	struct reg_srccap_adc_clk_info *clk_info = NULL;

	clk_info = (struct reg_srccap_adc_clk_info *)
			vzalloc(sizeof(struct reg_srccap_adc_clk_info) * ADCTBL_CLK_SET_NUM);
	if (clk_info == NULL) {
		SRCCAP_MSG_ERROR("vzalloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = drv_hwreg_srccap_adc_get_clk_setting(src_type, ADCTBL_CLK_SET_NUM, clk_info, &count);
	if (ret) {
		SRCCAP_MSG_ERROR("get table clk setting fail\n");
		ret = -EPERM;
		goto exit;
	}

	SRCCAP_MSG_INFO("[ADC] CLK nums from ADC table = %u\n", count);
	for (idx = 0; idx < count; idx++) {
		SRCCAP_MSG_INFO("[ADC] CLK[%2u]: Name= %-35s, parent=%s\n",
			idx, clk_info[idx].clk_name, clk_info[idx].clk_data);

		ret = adc_enable_parent(srccap_dev,
				clk_info[idx].clk_name,
				clk_info[idx].clk_data);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			SRCCAP_MSG_ERROR("adc_enable_parent fail\n");
			goto exit;
		}
	}

exit:
	if (clk_info != NULL)
		vfree(clk_info);

	return ret;
}

static int adc_disable_clk_swen(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u8 idx = 0;
	u16 count = 0;
	enum reg_srccap_adc_input_source_type src_type = REG_SRCCAP_ADC_INPUTSOURCE_OFF;
	struct reg_srccap_adc_clk_info *clk_info = NULL;

	clk_info = (struct reg_srccap_adc_clk_info *)
			vzalloc(sizeof(struct reg_srccap_adc_clk_info) * ADCTBL_CLK_SWEN_NUM);
	if (clk_info == NULL) {
		SRCCAP_MSG_ERROR("vzalloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = drv_hwreg_srccap_adc_get_clk_swen(src_type, ADCTBL_CLK_SWEN_NUM, clk_info, &count);
	if (ret) {
		SRCCAP_MSG_ERROR("hwreg get clk swen fail\n");
		ret = -EPERM;
		goto exit;
	}

	SRCCAP_MSG_INFO("[ADC] CLK SWEN nums from ADC table is: %u\n", count);
	for (idx = 0; idx < count; idx++) {
		SRCCAP_MSG_INFO("[ADC] SWEN[%2u]: Name= %-40s, enable=%s\n",
			idx, clk_info[idx].clk_name, clk_info[idx].clk_data);

		ret = adc_toggle_swen(srccap_dev,
				clk_info[idx].clk_name,
				clk_info[idx].clk_data);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			SRCCAP_MSG_ERROR("adc_toggle_swen fail\n");
			goto exit;
		}
	}

	//Bellow clk swens are ctrled in driver, not from table
	ret = adc_toggle_swen(srccap_dev, "adctbl_swen_dig_2162adc", "OFF");
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		goto exit;
	}

exit:
	if (clk_info != NULL)
		vfree(clk_info);

	return ret;
}

static int adc_disable_clk(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u8 idx = 0;
	u16 count = 0;
	enum reg_srccap_adc_input_source_type src_type = REG_SRCCAP_ADC_INPUTSOURCE_OFF;
	struct reg_srccap_adc_clk_info *clk_info = NULL;

	clk_info = (struct reg_srccap_adc_clk_info *)
			vzalloc(sizeof(struct reg_srccap_adc_clk_info) * ADCTBL_CLK_SET_NUM);
	if (clk_info == NULL) {
		SRCCAP_MSG_ERROR("vzalloc fail\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = drv_hwreg_srccap_adc_get_clk_setting(src_type, ADCTBL_CLK_SET_NUM, clk_info, &count);
	if (ret) {
		SRCCAP_MSG_ERROR("get table clk setting fail\n");
		ret = -EPERM;
		goto exit;
	}

	SRCCAP_MSG_INFO("[ADC] CLK nums from ADC table = %u\n", count);
	for (idx = 0; idx < count; idx++) {
		SRCCAP_MSG_INFO("[ADC] CLK[%2u]: Name= %-35s, parent=%s\n",
			idx, clk_info[idx].clk_name, clk_info[idx].clk_data);

		ret = adc_enable_parent(srccap_dev,
				clk_info[idx].clk_name,
				clk_info[idx].clk_data);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			SRCCAP_MSG_ERROR("adc_enable_parent fail\n");
			goto exit;
		}
	}

exit:
	if (clk_info != NULL)
		vfree(clk_info);

	return ret;
}

static int adc_set_ckg_iclamp_rgb(
	struct mtk_srccap_dev *srccap_dev,
	u16 iclamp_clk_rate)
{
	int ret = 0;
	struct reg_srccap_adc_clk_info clk_info;

	memset(&clk_info, 0, sizeof(struct reg_srccap_adc_clk_info));

	strncpy(clk_info.clk_name, "adctbl_iclamp_rgb_int_ck", CLK_NAME_MAX_LENGTH);

	if (iclamp_clk_rate == 0) {
		strncpy(clk_info.clk_data, "adctbl_prnt_clkd_atop_buf_ck", CLK_DATA_MAX_LENGTH);
		ret = adc_enable_parent(srccap_dev,
				clk_info.clk_name,
				clk_info.clk_data);
		if (ret < 0)
			SRCCAP_MSG_INFO("[ADC] set iclamp_rgb clk error!\n");
	} else {
		strncpy(clk_info.clk_data, "adctbl_prnt_clkd_2iclamp_s_buf_ck",
			CLK_DATA_MAX_LENGTH);
		ret = adc_enable_parent(srccap_dev,
				clk_info.clk_name,
				clk_info.clk_data);
		if (ret < 0)
			SRCCAP_MSG_INFO("[ADC] set iclamp_rgb clk error\n");
	}

	return ret;
}


static int adc_check_offline_port_map(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_srccap_input_source src,
	bool *conflict)
{
	int ret = 0;
	int port_index = 0;
	enum srccap_input_port offline_port = 0;
	enum v4l2_srccap_input_source online_src = 0;
	enum v4l2_srccap_input_source online_src0 = 0;
	enum v4l2_srccap_input_source online_src1 = 0;

	if (srccap_dev == NULL || conflict == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	if (srccap_dev->srccap_info.map[src].sync_port != 0)
		offline_port = srccap_dev->srccap_info.map[src].sync_port;
	else
		offline_port = srccap_dev->srccap_info.map[src].data_port;

	mutex_lock(&srccap_dev->srccap_info.mutex_list.mutex_online_source);

	ret = adc_split_src(srccap_dev->adc_info.adc_src, &online_src0, &online_src1);
	if (ret) {
		SRCCAP_MSG_ERROR("adc_split_src fail!\n");
		mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_online_source);
		return ret;
	}

	for (port_index = 0; port_index < SRCCAP_DEV_NUM; port_index++) {
		if (port_index == 0)
			online_src = online_src0;
		else
			online_src = online_src1;

		if (adc_check_same_port(
				offline_port,
				srccap_dev->srccap_info.map[online_src].data_port)
			|| adc_check_same_port(
				offline_port,
				srccap_dev->srccap_info.map[online_src].sync_port)) {
			*conflict = TRUE;
			break;
		}
		*conflict = FALSE;
	}

	mutex_unlock(&srccap_dev->srccap_info.mutex_list.mutex_online_source);

	return 0;
}

/* ============================================================================================== */
/* --------------------------------------- v4l2_ctrl_ops ---------------------------------------- */
/* ============================================================================================== */
static int mtk_adc_set_gain(
	struct v4l2_adc_gain *gain,
	struct mtk_srccap_dev *srccap_dev)
{
	if (gain == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;
	struct reg_srccap_adc_rgb_gain hwreg_gain;

	memset(&hwreg_gain, 0, sizeof(struct reg_srccap_adc_rgb_gain));

	hwreg_gain.r_gain = gain->r_gain;
	hwreg_gain.g_gain = gain->g_gain;
	hwreg_gain.b_gain = gain->b_gain;
	ret = drv_hwreg_srccap_adc_set_rgb_gain(hwreg_gain, true, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("set rgb gain fail\n");
		return ret;
	}

	return ret;
}

static int mtk_adc_get_gain(
	struct v4l2_adc_gain *gain,
	struct mtk_srccap_dev *srccap_dev)
{
	if (gain == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;
	struct reg_srccap_adc_rgb_gain hwreg_gain;

	memset(&hwreg_gain, 0, sizeof(struct reg_srccap_adc_rgb_gain));

	ret = drv_hwreg_srccap_adc_get_rgb_gain(&hwreg_gain);
	if (ret) {
		SRCCAP_MSG_ERROR("get gain fail\n");
		return ret;
	}
	gain->r_gain = hwreg_gain.r_gain;
	gain->g_gain = hwreg_gain.g_gain;
	gain->b_gain = hwreg_gain.b_gain;

	return ret;
}

static int mtk_adc_set_offset(
	struct v4l2_adc_offset *offset,
	struct mtk_srccap_dev *srccap_dev)
{
	if (offset == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;
	struct reg_srccap_adc_rgb_offset hwreg_offset;

	memset(&hwreg_offset, 0, sizeof(struct reg_srccap_adc_rgb_offset));

	hwreg_offset.r_offset = offset->r_offset;
	hwreg_offset.g_offset = offset->g_offset;
	hwreg_offset.b_offset = offset->b_offset;
	ret = drv_hwreg_srccap_adc_set_rgb_offset(hwreg_offset, true, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("hwreg set rgb offset fail\n");
		return ret;
	}

	return ret;
}

static int mtk_adc_get_offset(
	struct v4l2_adc_offset *offset,
	struct mtk_srccap_dev *srccap_dev)
{
	if (offset == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;
	struct reg_srccap_adc_rgb_offset hwreg_offset;

	memset(&hwreg_offset, 0, sizeof(struct reg_srccap_adc_rgb_offset));

	ret = drv_hwreg_srccap_adc_get_rgb_offset(&hwreg_offset);
	if (ret) {
		SRCCAP_MSG_ERROR("hwreg get offset fail\n");
		return ret;
	}
	offset->r_offset = hwreg_offset.r_offset;
	offset->g_offset = hwreg_offset.g_offset;
	offset->b_offset = hwreg_offset.b_offset;

	return ret;
}

static int mtk_adc_set_phase(
	u16 *phase,
	struct mtk_srccap_dev *srccap_dev)
{
	if (phase == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;
	u16 adcpllmod = 0;

	ret = drv_hwreg_srccap_adc_get_adcpllmod(&adcpllmod);
	if (ret) {
		SRCCAP_MSG_ERROR("get adcpllmod fail\n");
		return ret;
	}

	ret = drv_hwreg_srccap_adc_set_phase(*phase, adcpllmod, true, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("set phase fail\n");
		return ret;
	}

	return ret;
}

static int mtk_adc_get_phase(
	u16 *phase,
	struct mtk_srccap_dev *srccap_dev)
{
	if (phase == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;
	u16 adcpllmod = 0;

	ret = drv_hwreg_srccap_adc_get_adcpllmod(&adcpllmod);
	if (ret) {
		SRCCAP_MSG_ERROR("get adcpllmod fail\n");
		return ret;
	}

	ret = drv_hwreg_srccap_adc_get_phase(phase, adcpllmod);
	if (ret) {
		SRCCAP_MSG_ERROR("get phase fail\n");
		return ret;
	}

	return ret;
}

static int _adc_g_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, adc_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_ADC_GAIN:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_GAIN\n");
		ret = mtk_adc_get_gain(
		(struct v4l2_adc_gain *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_OFFSET:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_OFFSET\n");
		ret = mtk_adc_get_offset(
		(struct v4l2_adc_offset *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_PHASE:
	{
		SRCCAP_MSG_DEBUG("Get V4L2_CID_ADC_PHASE\n");
		ret = mtk_adc_get_phase((__u16 *)&(ctrl->val), srccap_dev);
		break;
	}
	case V4L2_CID_ADC_IS_SCARTRGB:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_IS_SCARTRGB\n");
		ret = drv_hwreg_srccap_adc_get_scart_type((bool *)&(ctrl->val));
		break;
	}
	default:
		ret = -1;
		break;
	}

	return ret;
}

static int _adc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	if (ctrl == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	struct mtk_srccap_dev *srccap_dev = NULL;
	int ret = 0;

	srccap_dev = container_of(ctrl->handler, struct mtk_srccap_dev, adc_ctrl_handler);
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	switch (ctrl->id) {
	case V4L2_CID_ADC_SOURCE:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_SOURCE\n");
		ret = mtk_adc_set_source(
		(struct v4l2_adc_source *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_GAIN:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_GAIN\n");
		ret = mtk_adc_set_gain(
		(struct v4l2_adc_gain *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_OFFSET:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_OFFSET\n");
		ret = mtk_adc_set_offset(
		(struct v4l2_adc_offset *)ctrl->p_new.p_u8, srccap_dev);
		break;
	}
	case V4L2_CID_ADC_PHASE:
	{
		SRCCAP_MSG_DEBUG("Set V4L2_CID_ADC_PHASE\n");
		ret = mtk_adc_set_phase((__u16 *)&(ctrl->val), srccap_dev);
		break;
	}
	case V4L2_CID_ADC_AUTO_GAIN_OFFSET:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_AUTO_GAIN_OFFSET\n");
		break;
	}
	case V4L2_CID_ADC_AUTO_GEOMETRY:
	{
		SRCCAP_MSG_DEBUG("V4L2_CID_ADC_AUTO_GEOMETRY\n");
		break;
	}
	default:
		ret = -1;
		break;
	}
	return ret;
}

static const struct v4l2_ctrl_ops adc_ctrl_ops = {
	.g_volatile_ctrl = _adc_g_ctrl,
	.s_ctrl = _adc_s_ctrl,
};

static const struct v4l2_ctrl_config adc_ctrl[] = {
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_SOURCE,
		.name = "Srccap ADC Set Source",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xff,
		.step = 1,
		.dims = {sizeof(struct v4l2_adc_source)},
		.flags =
		V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_GAIN,
		.name = "Srccap ADC Gain",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.dims = {sizeof(struct v4l2_adc_gain)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_OFFSET,
		.name = "Srccap ADC Offset",
		.type = V4L2_CTRL_TYPE_U8,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.dims = {sizeof(struct v4l2_adc_offset)},
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_PHASE,
		.name = "Srccap ADC Phase",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.def = 0,
		.min = 0,
		.max = 0xffff,
		.step = 1,
		.flags =
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE | V4L2_CTRL_FLAG_VOLATILE,
	},
	{
		.ops = &adc_ctrl_ops,
		.id = V4L2_CID_ADC_IS_SCARTRGB,
		.name = "Srccap ADC Is ScartRGB",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.def = 0,
		.min = false,
		.max = true,
		.step = 1,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	},
};

/* subdev operations */
static const struct v4l2_subdev_ops adc_sd_ops = {
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops adc_sd_internal_ops = {
};

/* ============================================================================================== */
/* -------------------------------------- Global Functions -------------------------------------- */
/* ============================================================================================== */
int mtk_adc_enable_offline_sog(
	struct mtk_srccap_dev *srccap_dev,
	bool enable)
{
	int ret = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	SRCCAP_MSG_INFO("adc offline mux: %s\n", enable?("ENABLE"):("DISABLE"));

	ret = drv_hwreg_srccap_adc_enable_offline_sog(enable, TRUE, NULL, NULL);
	if (ret) {
		SRCCAP_MSG_ERROR("hwreg adc enable offline mux fail\n");
		return ret;
	}

	return ret;
}

int mtk_adc_set_offline_mux(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_srccap_input_source src)
{
	int ret = 0;
	enum srccap_input_port offline_port = 0;
	enum reg_srccap_adc_input_offline_mux offline_src = 0;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return -EPERM;

	SRCCAP_MSG_DEBUG("adc set offline mux: %d\n", src);

	switch (src) {
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS2:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS3:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS4:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS5:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS6:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS7:
	case V4L2_SRCCAP_INPUT_SOURCE_CVBS8:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO2:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO3:
	case V4L2_SRCCAP_INPUT_SOURCE_SVIDEO4:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR2:
	case V4L2_SRCCAP_INPUT_SOURCE_YPBPR3:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART:
	case V4L2_SRCCAP_INPUT_SOURCE_SCART2:
		if (srccap_dev->srccap_info.map[src].sync_port != 0)
			offline_port = srccap_dev->srccap_info.map[src].sync_port;
		else
			offline_port = srccap_dev->srccap_info.map[src].data_port;

		offline_src = adc_map_offline_port_reg(offline_port);
		if (offline_src == REG_SRCCAP_ADC_INPUT_OFFLINE_MUX_NUM)
			return -EINVAL;

		ret = drv_hwreg_srccap_adc_set_offline_sog_mux(
			offline_src, TRUE, NULL, NULL);
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set offline mux fail\n");
			return ret;
		}

		usleep_range(SRCCAP_ADC_UDELAY_1000, SRCCAP_ADC_UDELAY_1100); // sleep 1ms

		break;
	case V4L2_SRCCAP_INPUT_SOURCE_VGA:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA2:
	case V4L2_SRCCAP_INPUT_SOURCE_VGA3:
		//TODO
		break;
	default:
		return -EPERM;
	}

	return ret;
}

int mtk_adc_check_offline_source(
	struct mtk_srccap_dev *srccap_dev,
	enum v4l2_srccap_input_source src,
	bool *offline_status)
{
	int ret = 0;
	bool conflict = TRUE;

	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	ret = adc_check_offline_port_map(srccap_dev, src, &conflict);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	if (conflict)
		*offline_status = FALSE;
	else
		*offline_status = TRUE;

	return ret;
}

int mtk_adc_set_freerun(
	struct mtk_srccap_dev *srccap_dev,
	bool enable)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;

	SRCCAP_MSG_INFO("adc set freerun: %s\n", enable?("ENABLE"):("DISABLE"));

	ret = drv_hwreg_srccap_adc_set_freerun(enable);
	if (ret) {
		SRCCAP_MSG_ERROR("load adc freerun table fail\n");
		return ret;
	}

	return ret;
}

int mtk_adc_set_mode(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	u64 pix_clock = 0;
	u32 hfreqx10 = srccap_dev->timingdetect_info.data.h_freqx10;
	u16 htt = srccap_dev->timingdetect_info.data.h_total;
	u16 iclamp_clk_rate = 0;
	u16 phase = 0;
	enum srccap_adc_source_info adc_src = adc_map_analog_source(srccap_dev->srccap_info.src);
	bool iclamp_delay_div_two_enable = false;
	bool adc_double_sampling_enable = false;

	/* set mode */
	pix_clock = (u64)hfreqx10 * (u64)htt;
	do_div(pix_clock, 10000UL); // unit: 1000000Hz

	SRCCAP_MSG_INFO("adc set mode: %lu\n", pix_clock);

	if ((adc_double_sampling_enable == false) &&
		(srccap_dev->timingdetect_info.data.h_de == SRCCAP_ADC_DOUBLE_SAMPLE_RESOLUTION)) {
		iclamp_delay_div_two_enable = true;
		SRCCAP_MSG_INFO("iclamp_delay_div_two_enable:%d\n",
						iclamp_delay_div_two_enable);
	}

	switch (adc_src) {
	case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
		ret = drv_hwreg_srccap_adc_set_mode(REG_SRCCAP_ADC_SETMODETYPE_YUV, (u16)pix_clock,
			&iclamp_clk_rate, iclamp_delay_div_two_enable);
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set adc mode fail\n");
			return ret;
		}
		break;
	case SRCCAP_ADC_SOURCE_ONLY_RGB:
		ret = drv_hwreg_srccap_adc_set_mode(REG_SRCCAP_ADC_SETMODETYPE_RGB, (u16)pix_clock,
			&iclamp_clk_rate, iclamp_delay_div_two_enable);
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set adc mode fail\n");
			return ret;
		}
		break;
	case SRCCAP_ADC_SOURCE_ONLY_SCART_RGB:
		ret = drv_hwreg_srccap_adc_set_mode(REG_SRCCAP_ADC_SETMODETYPE_YUV_Y,
			(u16)pix_clock, &iclamp_clk_rate, iclamp_delay_div_two_enable);
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set adc mode fail\n");
			return ret;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("Invalid source:%d\n", adc_src);
		break;
	}

	if ((adc_src == SRCCAP_ADC_SOURCE_ONLY_YPBPR)
		|| (adc_src == SRCCAP_ADC_SOURCE_ONLY_RGB)
		|| (adc_src == SRCCAP_ADC_SOURCE_ONLY_SCART_RGB)) {
		/* set plldiv */
		ret = adc_set_plldiv(srccap_dev);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
		/* set hpol */
		ret = adc_set_hpol(srccap_dev);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		/* set ckg_iclamp_rgb */
		ret = adc_set_ckg_iclamp_rgb(srccap_dev, iclamp_clk_rate);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		/* Toggle PLLA reset */
		ret = adc_set_plla_reset();
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		/* toggle phase */
		ret = mtk_adc_get_phase(&phase, srccap_dev);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}

		ret = mtk_adc_set_phase(&phase, srccap_dev);
		if (ret < 0) {
			SRCCAP_MSG_RETURN_CHECK(ret);
			return ret;
		}
	}

	/* stop ISOG reset to prevent garbage on G channel */
	ret = mtk_adc_set_isog_detect_mode(srccap_dev, SRCCAP_ADC_ISOG_NORMAL_MODE);
	if (ret < 0) {
		SRCCAP_MSG_RETURN_CHECK(ret);
		return ret;
	}

	return ret;
}

int mtk_adc_load_cal_tbl(
	struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum srccap_adc_source_info adc_src = adc_map_analog_source(srccap_dev->srccap_info.src);

	SRCCAP_MSG_INFO("adc load calibration table: source=[%d]\n", adc_src);

	switch (adc_src) {
	case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
	case SRCCAP_ADC_SOURCE_ONLY_RGB:
		ret = drv_hwreg_srccap_adc_set_rgb_cal(FALSE, FALSE);
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set rgb cal fail\n");
			return ret;
		}
		break;
	case SRCCAP_ADC_SOURCE_ONLY_CVBS:
	case SRCCAP_ADC_SOURCE_ONLY_EXT_DMD_ATV:
	case SRCCAP_ADC_SOURCE_ONLY_SCART_Y:
		ret = drv_hwreg_srccap_adc_set_av_cal();
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set av cal fail\n");
			return ret;
		}
		break;
	case SRCCAP_ADC_SOURCE_ONLY_SCART_RGB:
	case SRCCAP_ADC_SOURCE_MULTI_RGB_CVBS:
	case SRCCAP_ADC_SOURCE_MULTI_RGB_SCARTY:
	case SRCCAP_ADC_SOURCE_MULTI_YPBPR_CVBS:
		ret = drv_hwreg_srccap_adc_set_rgb_cal(FALSE, FALSE);
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set rgb cal fail\n");
			return ret;
		}

		ret = drv_hwreg_srccap_adc_set_av_cal();
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set av cal fail\n");
			return ret;
		}
		break;
	case SRCCAP_ADC_SOURCE_ONLY_SVIDEO:
		ret = drv_hwreg_srccap_adc_set_sv_cal();
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg set sv cal fail\n");
			return ret;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("Invalid source:%d\n", adc_src);
		break;
	}

	return ret;
}

int mtk_adc_set_source(
	struct v4l2_adc_source *user_source,
	struct mtk_srccap_dev *srccap_dev)
{
	if (user_source == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return -EPERM;

	int ret = 0;
	struct v4l2_adc_source source;
	enum v4l2_srccap_input_source *input = NULL;
	enum reg_srccap_adc_pws_type reg_pws_type = 0;

#ifndef CONFIG_MSTAR_ARM_BD_FPGA
	/* handle user pointer */
	input = kzalloc((sizeof(enum v4l2_srccap_input_source)
		* user_source->src_count), GFP_KERNEL);
	if (!input) {
		SRCCAP_MSG_ERROR("kzalloc fail\n");
		return -ENOMEM;
	}
	if (copy_from_user((void *)input, (__u8 __user *)user_source->p.adc_input_src,
		sizeof(enum v4l2_srccap_input_source) * user_source->src_count)) {
		SRCCAP_MSG_ERROR("copy_from_user fail\n");
		ret = -EFAULT;
		goto out;
	}
	memset(&source, 0, sizeof(struct v4l2_adc_source));
	source.src_count = user_source->src_count;
	source.p.adc_input_src = input;

	/* set source setting */
	ret = adc_set_src_setting(&source, srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("adc_set_src_setting fail\n");
		goto out;
	}

	/* set clk swen */
	ret = adc_set_clk_swen(srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("adc_set_clk_swen fail\n");
		goto out;
	}
	/* set clk setting */
	ret = adc_set_clk(srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("adc_set_clk fail\n");
		goto out;
	}
	/* set pws setting */
	reg_pws_type = adc_map_pws_reg(srccap_dev->adc_info.adc_src);
	SRCCAP_MSG_INFO("[ADC] adc set pws table idx: (%d)\n", reg_pws_type);
	ret = drv_hwreg_srccap_adc_set_pws(reg_pws_type);
	if (ret) {
		SRCCAP_MSG_ERROR("load pws table fail\n");
		goto out;
	}

out:
	kfree(input);
#endif
	return ret;
}

int mtk_adc_set_isog_detect_mode(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_adc_isog_detect_mode mode)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}
	if ((mode < SRCCAP_ADC_ISOG_NORMAL_MODE) || (mode >= SRCCAP_ADC_ISOG_MODE_NUMS)) {
		SRCCAP_MSG_ERROR("Invalid isog detect mode %d\n", mode);
		return -EINVAL;
	}

	int ret = 0;
	bool enable = false;
	enum srccap_adc_source_info adc_src = adc_map_analog_source(srccap_dev->srccap_info.src);

	switch (adc_src) {
	case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
	case SRCCAP_ADC_SOURCE_ONLY_RGB:
	case SRCCAP_ADC_SOURCE_ONLY_SCART_RGB:
		ret = drv_hwreg_srccap_adc_get_isog_enable(&enable);
		if (ret) {
			SRCCAP_MSG_ERROR("hwreg get isog enable fail\n");
			goto out;
		}

		if (enable) {
			switch (mode) {
			case SRCCAP_ADC_ISOG_NORMAL_MODE:
				drv_hwreg_srccap_adc_set_isog_reset_width(0, true, NULL, NULL);
				break;
			case SRCCAP_ADC_ISOG_STANDBY_MODE:
				drv_hwreg_srccap_adc_set_isog_reset_width(1, true, NULL, NULL);
				break;
			default:
				SRCCAP_MSG_ERROR("Invalid isog detect mode %d\n", mode);
				goto out;
			}
		}
		break;
	default:
		SRCCAP_MSG_ERROR("Invalid source:%d\n", adc_src);
		break;
	}
out:
	return ret;
}

int mtk_adc_set_iclamp_rgb_powerdown(
	struct mtk_srccap_dev *srccap_dev,
	bool powerdown)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	int ret = 0;
	enum srccap_adc_source_info adc_src = adc_map_analog_source(srccap_dev->srccap_info.src);

	switch (adc_src) {
	case SRCCAP_ADC_SOURCE_ONLY_YPBPR:
	case SRCCAP_ADC_SOURCE_ONLY_RGB:
	case SRCCAP_ADC_SOURCE_ONLY_SCART_RGB:
		ret = drv_hwreg_srccap_adc_set_iclamp_rgb_powerdown(powerdown, TRUE, NULL, NULL);
		if (ret < 0) {
			SRCCAP_MSG_ERROR("hwreg set iclamp rgb power down fail\n");
			goto out;
		}
		break;
	default:
		SRCCAP_MSG_ERROR("Invalid source:%d\n", adc_src);
		break;
	}
out:
	return ret;
}

int mtk_adc_init(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;
	u32 hw_version = 0;

	memset(&(srccap_dev->adc_info), 0, sizeof(struct srccap_adc_info));
	hw_version = srccap_dev->srccap_info.cap.hw_version;

	/* set hw version*/
	ret = drv_hwreg_srccap_adc_set_hw_version(hw_version);

	/* set default swen */
	ret = adc_set_clk_swen(srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("adc set init clock swen fail\n");
		goto out;
	}

	/* set default clk setting */
	ret = adc_set_clk(srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("adc set init clk fail\n");
		goto out;
	}

	/* load pws table */
	ret = drv_hwreg_srccap_adc_set_pws(adc_map_pws_reg(srccap_dev->adc_info.adc_src));
	if (ret) {
		SRCCAP_MSG_ERROR("load pws table fail\n");
		goto out;
	}
	usleep_range(100, 101); //need to delay 100us for HW SOG cal

	/* set isog detect mode */
	ret = mtk_adc_set_isog_detect_mode(srccap_dev, SRCCAP_ADC_ISOG_STANDBY_MODE);
	if (ret) {
		SRCCAP_MSG_ERROR("set adc isog_detect_mode fail\n");
		goto out;
	}

	/* load init table */
	ret = drv_hwreg_srccap_adc_init();
	if (ret) {
		SRCCAP_MSG_ERROR("load adc init table fail\n");
		goto out;
	}

	/* adc free run for pll clk enable */
	ret = mtk_adc_set_freerun(srccap_dev, true);
	if (ret) {
		SRCCAP_MSG_ERROR("load adc freerun_en table fail\n");
		goto out;
	}

	/* set fixed gain/offset/phase */
	ret = adc_set_fixed_gain_offset_phase();
	if (ret) {
		SRCCAP_MSG_ERROR("set adc fixed gain_offset_phase fail\n");
		goto out;
	}

out:
	return ret;
}

int mtk_adc_deinit(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int ret = 0;

	ret = adc_disable_clk_swen(srccap_dev);

	if (ret) {
		SRCCAP_MSG_ERROR("adc set default off table fail\n");
		return ret;
	}

	ret = adc_disable_clk(srccap_dev);
	if (ret) {
		SRCCAP_MSG_ERROR("adc set disable unprepare clk fail\n");
		return ret;
	}

	return ret;
}

int mtk_adc_set_port(struct mtk_srccap_dev *srccap_dev)
{
	if (srccap_dev == NULL) {
		SRCCAP_MSG_POINTER_CHECK();
		return -ENXIO;
	}

	/* Only dev0 ctrl ADC HW */
	if (srccap_dev->dev_id != 0)
		return 0;

	int i = 0;
	u32 hdmi_cnt = srccap_dev->srccap_info.cap.hdmi_cnt;
	u32 cvbs_cnt = srccap_dev->srccap_info.cap.cvbs_cnt;
	u32 svideo_cnt = srccap_dev->srccap_info.cap.svideo_cnt;
	u32 ypbpr_cnt = srccap_dev->srccap_info.cap.ypbpr_cnt;
	u32 vga_cnt = srccap_dev->srccap_info.cap.vga_cnt;
	int ret = 0;

	if (srccap_dev->srccap_info.cap.hdmi_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_HDMI;
			i < (V4L2_SRCCAP_INPUT_SOURCE_HDMI + hdmi_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.cap.cvbs_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_CVBS;
			i < (V4L2_SRCCAP_INPUT_SOURCE_CVBS + cvbs_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.cap.svideo_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_SVIDEO;
			i < (V4L2_SRCCAP_INPUT_SOURCE_SVIDEO + svideo_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.cap.ypbpr_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_YPBPR;
			i < (V4L2_SRCCAP_INPUT_SOURCE_YPBPR + ypbpr_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}
	if (srccap_dev->srccap_info.cap.vga_cnt > 0)
		for (i = V4L2_SRCCAP_INPUT_SOURCE_VGA;
			i < (V4L2_SRCCAP_INPUT_SOURCE_VGA + vga_cnt);
			i++) {
			/* to be implemented */
			pr_info("src:%d data_port:%d\n",
				i, srccap_dev->srccap_info.map[i].data_port);
			pr_info("src:%d sync_port:%d\n",
				i, srccap_dev->srccap_info.map[i].sync_port);
		}

	return ret;
}

int mtk_srccap_register_adc_subdev(
	struct v4l2_device *v4l2_dev,
	struct v4l2_subdev *subdev_adc,
	struct v4l2_ctrl_handler *adc_ctrl_handler)
{
	int ret = 0;
	u32 ctrl_count;
	u32 ctrl_num = sizeof(adc_ctrl)/sizeof(struct v4l2_ctrl_config);

	v4l2_ctrl_handler_init(adc_ctrl_handler, ctrl_num);
	for (ctrl_count = 0; ctrl_count < ctrl_num; ctrl_count++) {
		v4l2_ctrl_new_custom(adc_ctrl_handler, &adc_ctrl[ctrl_count],
									NULL);
	}

	ret = adc_ctrl_handler->error;
	if (ret) {
		SRCCAP_MSG_ERROR("failed to create adc ctrl handler\n");
		goto exit;
	}
	subdev_adc->ctrl_handler = adc_ctrl_handler;

	v4l2_subdev_init(subdev_adc, &adc_sd_ops);
	subdev_adc->internal_ops = &adc_sd_internal_ops;
	strlcpy(subdev_adc->name, "mtk-adc", sizeof(subdev_adc->name));

	ret = v4l2_device_register_subdev(v4l2_dev, subdev_adc);
	if (ret) {
		SRCCAP_MSG_ERROR("failed to register adc subdev\n");
		goto exit;
	}

	return 0;

exit:
	v4l2_ctrl_handler_free(adc_ctrl_handler);
	return ret;
}

void mtk_srccap_unregister_adc_subdev(struct v4l2_subdev *subdev_adc)
{
	v4l2_ctrl_handler_free(subdev_adc->ctrl_handler);
	v4l2_device_unregister_subdev(subdev_adc);
}

