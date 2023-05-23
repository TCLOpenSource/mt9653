// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include "m7332_demod_core.h"
#include "m7332_demod_common.h"
#include "m7332_demod_drv_dvbt_t2.h"
#include "m7332_demod_drv_dvbc.h"
#include "drvDMD_ISDBT.h"
#include "drvDMD_DTMB.h"
#include "drvDMD_ATSC.h"
#include "drvDMD_VIF.h"
#include "drvDMD_DVBS.h"
#ifdef TUNER_OK
#include "mtkTunerType.h"
#include "mtk_tuner.h"
#endif

/* 200ms, dvb_frontend_thread wake up time */
#define THREAD_WAKE_UP_TIME    5
#define TMP_DRAM_START_ADDRESS 0x21000000
#define TMP_RIU_BASE_ADDRESS   0x1F000000
#define TMP_RIU_MAPPING_SIZE   0x400000

struct DMD_DTMB_InitData sDMD_DTMB_InitData;
struct dmd_isdbt_initdata sdmd_isdbt_initdata;
struct DMD_ATSC_INITDATA sdmd_atsc_initdata;

static DRIVER_ATTR_RO(atsc_get_information);
static DRIVER_ATTR_RO(isdbt_get_information);
static DRIVER_ATTR_RO(dtmb_get_information);
static DRIVER_ATTR_RO(dvbt_t2_get_information);
static DRIVER_ATTR_RO(dvbc_get_information);
static DRIVER_ATTR_RO(dvbs_get_information);

static struct attribute *demod_attrs[] = {
	&driver_attr_atsc_get_information.attr,
	&driver_attr_isdbt_get_information.attr,
	&driver_attr_dtmb_get_information.attr,
	&driver_attr_dvbt_t2_get_information.attr,
	&driver_attr_dvbc_get_information.attr,
	&driver_attr_dvbs_get_information.attr,
	NULL
};

static struct attribute_group demod_attrs_group = {
	.attrs = demod_attrs,
};

static const struct attribute_group *dmd_groups[] = {
	&demod_attrs_group,
	NULL
};

static int mtk_demod_read_status(struct dvb_frontend *fe,
				    enum fe_status *status);

static int mtk_demod_set_frontend_vif(struct dvb_frontend *fe,
	u32 if_freq)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	enum vif_freq_band vif_rf_band;
	enum vif_sound_sys vif_sound_std;
	u8 scan_flag = 0;

	if (c->frequency < 200*1000000)
		vif_rf_band = FREQ_VHF_L;
	else if (c->frequency < 470*1000000)
		vif_rf_band = FREQ_VHF_H;
	else
		vif_rf_band = FREQ_UHF;

	switch (c->ana_sound_std) {
	case FE_SOUND_B:
	case FE_SOUND_G:
	case FE_SOUND_B|FE_SOUND_G:
		//< 300 MHz is B, > 300 MHz is G
		if (c->frequency < 300*1000000)
			vif_sound_std = VIF_SOUND_B;
		else
			vif_sound_std = VIF_SOUND_GH;
		break;
	case FE_SOUND_D:
	case FE_SOUND_K:
	case FE_SOUND_D|FE_SOUND_K:
		vif_sound_std = VIF_SOUND_DK2;
		break;
	case FE_SOUND_I:
		vif_sound_std = VIF_SOUND_I;
		break;
	case FE_SOUND_M:
	case FE_SOUND_N:
	case FE_SOUND_M|FE_SOUND_N:
		vif_sound_std = VIF_SOUND_MN;
		break;
	case FE_SOUND_L:
		vif_sound_std = VIF_SOUND_L;
		break;
	case FE_SOUND_L_P:
		vif_sound_std = VIF_SOUND_LL;
		break;
	default:
		//TODO, should return error?
		vif_sound_std = VIF_SOUND_DK2;
		break;
	}

	if (c->tune_flag == SCAN_MODE)
		scan_flag = 1;
	else
		scan_flag = 0;

	drv_vif_set_freqband(vif_rf_band); //471250kHz
	drv_vif_set_sound_sys(vif_sound_std, if_freq); //dk
	drv_vif_set_scan(scan_flag); //is_scan = 0

	return 0;
}

static int mtk_demod_set_frontend(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;
	u32 u32IFFreq;
	int ret = 0;

	/*fake para*/
	struct analog_parameters a_para = {0};

	pr_info("[%s] is called\n", __func__);

	if ((c->delivery_system == SYS_DVBS) ||
			((c->delivery_system == SYS_ANALOG) &&
			(c->delivery_system != demod_prvi->previous_system))) {
#if defined(TUNER_OK)
		if (fe->ops.tuner_ops.release) {
			fe->ops.tuner_ops.release(fe);
		} else {
			pr_err("Failed to get tuner_ops.release\r\n");
			return -ENONET;
		}
		if (fe->ops.tuner_ops.init) {
			ret = fe->ops.tuner_ops.init(fe);
			if (ret) {
				pr_err("[%s][%d] tuner_ops.init fail !\n",
				__func__, __LINE__);
				return ret;
			}
		} else {
			pr_err("Failed to get tuner_ops.init\r\n");
			return -ENONET;
		}
#endif
	}
	/* Program tuner */
	/* DTV */
	if (c->delivery_system != SYS_ANALOG) {
#if defined(TUNER_OK)
		if (fe->ops.tuner_ops.set_params) {
			ret = fe->ops.tuner_ops.set_params(fe);
			if (ret) {
				pr_err("[%s][%d] tuner_ops.set_params fail !\n",
					__func__, __LINE__);
				return ret;
			}
		} else {
			pr_err("Failed to get tuner_ops.set_params\r\n");
			return -ENONET;
		}
#endif
	} else {
#if defined(TUNER_OK)
		if (fe->ops.tuner_ops.set_analog_params) {
			ret = fe->ops.tuner_ops.set_analog_params(fe, &a_para);
			if (ret) {
				pr_info("[%s][%d] set_analog_params fail !\n",
					__func__, __LINE__);
				return ret;
			}
		} else {
			pr_info("Failed to get tuner_ops.set_analog_params\r\n");
			return -ENONET;
		}
#endif
	}

	/* get IF frequency */
	if (c->delivery_system != SYS_DVBS) {
#if defined(TUNER_OK)
		if (fe->ops.tuner_ops.get_if_frequency) {
			ret = fe->ops.tuner_ops.get_if_frequency(fe,
				&u32IFFreq);
			if (ret) {
				pr_err("[%s][%d] tuner_ops.get_if_frequency fail !\n",
					__func__, __LINE__);
				return ret;
			}
		} else {
			pr_err("Failed to get tuner_ops.get_if_frequency\r\n");
			return -ENONET;
		}
#endif
	}
	pr_info("pre_system = %d, now_system = %d\n",
		demod_prvi->previous_system, c->delivery_system);

	/* system change.  source change / first time probe / other situation */
	if (demod_prvi->previous_system != c->delivery_system) {
		/* exit previous system */
		switch (demod_prvi->previous_system) {
		case SYS_DVBT:
		case SYS_DVBT2:
			ret = m7332_dmd_drv_dvbt_t2_exit();
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbt_t2_exit fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBS:
		case SYS_DVBS2:
			ret = mdrv_dmd_dvbs_exit();
			if (ret) {
				pr_err("[%s][%d] MDrv_DMD_DVBS_Exit fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBC_ANNEX_A:
			ret = m7332_dmd_drv_dvbc_exit();
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbc_exit fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBC_ANNEX_B:
		case SYS_ATSC:
			pr_info("ATSC_exit in remove is called\n");
			ret = !_mdrv_dmd_atsc_md_exit(0);
			break;
		case SYS_ISDBT:
			pr_info("ISDBT_exit in remove is called\n");
			ret = !_mdrv_dmd_isdbt_md_exit(0);
			break;
		case SYS_DTMB:
			pr_info("[%s][%d] DTMB_exit is called\n",
					__func__, __LINE__);
			ret = !_mdrv_dmd_dtmb_md_exit(0);
			break;
		case SYS_ANALOG:
			pr_info("vif_exit is called\n");
			drv_vif_exit();
			break;
		case SYS_UNDEFINED:
		default:
			break;
		}
		demod_prvi->fw_downloaded = 0;

		/* update system */
		demod_prvi->previous_system = c->delivery_system;

		/* init new system */
		switch (c->delivery_system) {
		case SYS_DVBT:
		case SYS_DVBT2:
			ret = m7332_dmd_drv_dvbt_t2_init(fe);
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbt_t2_init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBS:
		case SYS_DVBS2:
			ret = mdrv_dmd_dvbs_init(fe);
			if (ret) {
				pr_err("[%s][%d] MDrv_DMD_DVBS_Init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;

		case SYS_DVBC_ANNEX_A:
			ret = m7332_dmd_drv_dvbc_init(fe);
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbc_init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBC_ANNEX_B:
		case SYS_ATSC:
			// Init Parameter
			sdmd_atsc_initdata.biqswap =  0;
			sdmd_atsc_initdata.btunergaininvert = 0;
			sdmd_atsc_initdata.if_khz = 5000;
			sdmd_atsc_initdata.vsbagclockchecktime   = 100;
			sdmd_atsc_initdata.vsbprelockchecktime   = 300;
			sdmd_atsc_initdata.vsbfsynclockchecktime = 600;
			sdmd_atsc_initdata.vsbfeclockchecktime   = 4000;
			sdmd_atsc_initdata.qamagclockchecktime   = 60;
			sdmd_atsc_initdata.qamprelockchecktime   = 1000;
			sdmd_atsc_initdata.qammainlockchecktime  = 2000;
			sdmd_atsc_initdata.bisextdemod = FALSE;

			ret = !_mdrv_dmd_atsc_md_init(0, &sdmd_atsc_initdata,
				sizeof(sdmd_atsc_initdata));
			if (ret) {
				pr_info("[%s][%d] _mdrv_dmd_atsc_md_init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_ISDBT:
			sdmd_isdbt_initdata.is_ext_demod = 0;
			sdmd_isdbt_initdata.if_khz = 5000;
			sdmd_isdbt_initdata.agc_reference_value = 0x400;
			sdmd_isdbt_initdata.tuner_gain_invert = 0;
			sdmd_isdbt_initdata.tdi_start_addr =
				(TMP_DRAM_START_ADDRESS)>>4;
			ret = !_mdrv_dmd_isdbt_md_init(0, &sdmd_isdbt_initdata,
				sizeof(sdmd_isdbt_initdata));
			if (ret) {
				pr_info("[%s][%d] _MDrv_DMD_ISDBT_MD_Init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DTMB:
			sDMD_DTMB_InitData.bisextdemod = 0;
			sDMD_DTMB_InitData.uif_khz16 = 5000;
			sDMD_DTMB_InitData.btunergaininvert = 0;
			sDMD_DTMB_InitData.utdistartaddr32 =
				(TMP_DRAM_START_ADDRESS)>>4;
			ret = !_mdrv_dmd_dtmb_md_init(0, &sDMD_DTMB_InitData,
				sizeof(sDMD_DTMB_InitData));
			if (ret) {
				pr_info("[%s][%d] _MDrv_DMD_DTMB_MD_Init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_ANALOG:
			pr_info("vif_init is called\n");
			drv_vif_init();
			break;
		case SYS_UNDEFINED:
		default:
			break;
		}
		demod_prvi->fw_downloaded = 1;
	}

	/* Program demod */
	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = m7332_dmd_drv_dvbt_t2_config(fe, u32IFFreq);
		if (ret) {
			pr_err("[%s][%d] m7332_dmd_drv_dvbt_t2_config fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		ret = mdrv_dmd_dvbs_config(fe);
		if (ret) {
			pr_err("[%s][%d] MDrv_DMD_DVBS_Config fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_DVBC_ANNEX_A:
		ret = m7332_dmd_drv_dvbc_config(fe, u32IFFreq);
		if (ret) {
			pr_err("[%s][%d] m7332_dmd_drv_dvbc_config fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_DVBC_ANNEX_B:
		ret = !_mdrv_dmd_atsc_md_setconfig(0,
					DMD_ATSC_DEMOD_ATSC_256QAM, TRUE);
		if (ret) {
			pr_info("[%s][%d] _mdrv_dmd_atsc_md_setconfig fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_ATSC:
		ret = !_mdrv_dmd_atsc_md_setconfig(0,
				DMD_ATSC_DEMOD_ATSC_VSB, TRUE);
		if (ret) {
			pr_info("[%s][%d] _mdrv_dmd_atsc_md_setconfig fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_ISDBT:
		ret = !_mdrv_dmd_isdbt_md_advsetconfig(0,
				DMD_ISDBT_DEMOD_6M, TRUE);
		if (ret) {
			pr_info("[%s][%d] _MDrv_DMD_ISDBT_MD_AdvSetConfig fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_DTMB:
		ret = !_mdrv_dmd_dtmb_md_setconfig(0,
				DMD_DTMB_DEMOD_DTMB, TRUE);
		if (ret) {
			pr_info("[%s][%d] _MDrv_DMD_DTMB_MD_SetConfig fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_ANALOG:
		pr_info("mtk7332_demod set freqency to %d\n", (c->frequency));
		mtk_demod_set_frontend_vif(fe, u32IFFreq);
		demod_prvi->start_time = mtk_demod_get_time();
		break;
	case SYS_UNDEFINED:
	default:
		break;
	}

	return ret;
}

static int mtk_demod_read_signal_strength(struct dvb_frontend *fe,
				u16 *strength)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret = 0;
	u16 rssi = 0;
	u16 ifagc = 0;

	switch (c->delivery_system) {
	case SYS_DVBS:
	case SYS_DVBS2:
		if (fe->ops.tuner_ops.get_rf_strength) {
			ret = fe->ops.tuner_ops.get_rf_strength(fe, &rssi);
		} else {
			mdrv_dmd_dvbs_get_ifagc(&ifagc);
			mdrv_dmd_dvbs_get_rf_level(ifagc, &rssi);
		}
		ret &= mdrv_dmd_dvbs_get_signal_strength(rssi, strength);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mtk_demod_get_frontend(struct dvb_frontend *fe,
				 struct dtv_frontend_properties *p)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret = 0;
	enum fe_status status = 0;
	u16 strength = 0;

	pr_info("[%s] is called\n", __func__);

	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = m7332_dmd_drv_dvbt_t2_get_frontend(fe, p);
		if (ret)
			return ret;
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		mtk_demod_read_status(fe, &status);
		if (status == FE_HAS_LOCK) {
			ret = mdrv_dmd_dvbs_get_frontend(fe, p);
			ret &= mtk_demod_read_signal_strength(fe, &strength);
			p->strength.stat[0].scale = FE_SCALE_RELATIVE;
			p->strength.stat[0].uvalue = strength;
		} else {
			p->strength.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->block_error.stat[0].scale =
				FE_SCALE_NOT_AVAILABLE;
			p->post_bit_error.stat[0].scale =
				FE_SCALE_NOT_AVAILABLE;
			p->post_bit_count.stat[0].scale =
				FE_SCALE_NOT_AVAILABLE;
		}
		if (ret)
			return ret;
		break;
	case SYS_DVBC_ANNEX_A:
		ret = m7332_dmd_drv_dvbc_get_frontend(fe, p);
		if (ret)
			return ret;
		break;
	case SYS_DVBC_ANNEX_B:
	case SYS_ATSC:
	{
		pr_info("Steve DBG:[%s],[%d]\n", __func__, __LINE__);
		c->delivery_system = SYS_ATSC;
		mtk_demod_read_status(fe, &status);

		if (status == 0) {
			p->strength.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->post_bit_error.stat[0].scale =
				FE_SCALE_NOT_AVAILABLE;
			p->post_bit_count.stat[0].scale =
				FE_SCALE_NOT_AVAILABLE;
		} else {
			ret = _mdrv_dmd_atsc_md_get_info(fe, p);
		}
	}
		break;
	case SYS_ISDBT:
		//struct dtv_frontend_properties *c =
		//&fe->dtv_property_cache;
		c->delivery_system = SYS_ISDBT;
		//u32 delsys = c->delivery_system;
		mtk_demod_read_status(fe, &status);

		if (status == 0) {
			p->strength.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->post_bit_error.stat[0].scale =
					FE_SCALE_NOT_AVAILABLE;
			p->post_bit_count.stat[0].scale =
					FE_SCALE_NOT_AVAILABLE;
		} else {
			ret = _mdrv_dmd_isdbt_md_get_information(0, fe, p);
		}
		break;
	case SYS_DTMB:
		c->delivery_system = SYS_DTMB;
		mtk_demod_read_status(fe, &status);
		if (status == 0) {
			p->strength.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->block_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
			p->post_bit_error.stat[0].scale =
					FE_SCALE_NOT_AVAILABLE;
			p->post_bit_count.stat[0].scale =
					FE_SCALE_NOT_AVAILABLE;
		} else {
			ret = _mdrv_dmd_drv_dtmb_get_frontend(fe, p);

		if (ret)
			return ret;
		}
		break;
	case SYS_ANALOG:
		/* empty for now */
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int mtk_demod_init(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;
	int ret = 0;

	pr_info("[%s] is called\n", __func__);

	mtk_demod_init_riuaddr(fe);

	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		if (fe->exit == DVB_FE_DEVICE_RESUME) {
			ret = m7332_dmd_drv_dvbt_t2_init(fe);
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbt_t2_init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			demod_prvi->fw_downloaded = 1;
			/* update system */
			demod_prvi->previous_system = c->delivery_system;
		}
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		if (fe->exit == DVB_FE_DEVICE_RESUME) {
			ret = mdrv_dmd_dvbs_init(fe);
			if (ret) {
				pr_err("[%s][%d] MDrv_DMD_DVBS_Init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			demod_prvi->fw_downloaded = 1;
			/* update system */
			demod_prvi->previous_system = c->delivery_system;
		}
		break;

	case SYS_DVBC_ANNEX_A:
		if (fe->exit == DVB_FE_DEVICE_RESUME) {
			ret = m7332_dmd_drv_dvbc_init(fe);
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbc_init fail !\n",
					__func__, __LINE__);
				return ret;
			}
			demod_prvi->fw_downloaded = 1;
			/* update system */
			demod_prvi->previous_system = c->delivery_system;
		}
		break;
	case SYS_DVBC_ANNEX_B:
	case SYS_ATSC:
		if (fe->exit == DVB_FE_DEVICE_RESUME) {
			ret = _mdrv_dmd_atsc_md_setpowerstate(0,
					E_POWER_RESUME);
			if (ret) {
				pr_err("[%s][%d] ATSC_resume_fail !\n",
					__func__, __LINE__);
				return ret;
			}
			demod_prvi->fw_downloaded = 1;
			/* update system */
			demod_prvi->previous_system = c->delivery_system;
		}
		break;
	case SYS_ISDBT:
		if (fe->exit == DVB_FE_DEVICE_RESUME) {
			ret = _mdrv_dmd_isdbt_md_setpowerstate(0,
					E_POWER_RESUME);
			if (ret) {
				pr_err("[%s][%d] ISDBT_resume_fail !\n",
					__func__, __LINE__);
				return ret;
			}
			demod_prvi->fw_downloaded = 1;
			/* update system */
			demod_prvi->previous_system = c->delivery_system;
		}
		break;
	case SYS_DTMB:
		if (fe->exit == DVB_FE_DEVICE_RESUME) {
			ret = _mdrv_dmd_dtmb_md_setpowerstate(0,
					E_POWER_RESUME);
			if (ret) {
				pr_err("[%s][%d] DTMB_resume_fail !\n",
					__func__, __LINE__);
				return ret;
			}
			demod_prvi->fw_downloaded = 1;
			demod_prvi->previous_system = c->delivery_system;
		}
		break;
	case SYS_ANALOG:
		if (fe->exit == DVB_FE_DEVICE_RESUME) {
			ret = drv_vif_set_power_state(E_POWER_RESUME);
			if (ret) {
				pr_err("[%s][%d] VIF_resume_fail !\n",
					__func__, __LINE__);
				return ret;
			}
			demod_prvi->fw_downloaded = 1;
			/* update system */
			demod_prvi->previous_system = c->delivery_system;
		}
		break;
	case SYS_UNDEFINED:
	default:
		break;
	}


	return ret;
}

static int mtk_demod_sleep(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;
	int ret = 0;

	pr_info("[%s] is called\n", __func__);

	/* exit system */
	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = m7332_dmd_drv_dvbt_t2_exit();
		if (ret) {
			pr_err("[%s][%d] m7332_dmd_drv_dvbt_t2_exit fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		ret = mdrv_dmd_dvbs_exit();
		if (ret) {
			pr_err("[%s][%d] MDrv_DMD_DVBS_Exit fail !\n",
				__func__, __LINE__);
			return ret;
		}
#if defined(TUNER_OK)
		if (fe->ops.tuner_ops.release) {
			fe->ops.tuner_ops.release(fe);
		} else {
			pr_err("Failed to get tuner_ops.release\r\n");
			return -ENONET;
		}
#endif
		break;
	case SYS_DVBC_ANNEX_A:
		ret = m7332_dmd_drv_dvbc_exit();
		if (ret) {
			pr_err("[%s][%d] m7332_dmd_drv_dvbc_exit fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_DVBC_ANNEX_B:
	case SYS_ATSC:
		ret = _mdrv_dmd_atsc_md_setpowerstate(0, E_POWER_SUSPEND);
		if (ret) {
			pr_err("[%s][%d] ATSC_suspend_fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_ISDBT:
		ret = _mdrv_dmd_isdbt_md_setpowerstate(0, E_POWER_SUSPEND);
		if (ret) {
			pr_err("[%s][%d] ISDBT_suspend_fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_DTMB:
		ret = _mdrv_dmd_dtmb_md_setpowerstate(0, E_POWER_SUSPEND);
		if (ret) {
			pr_err("[%s][%d] DTMB_suspend_fail !\n",
				__func__, __LINE__);
			return ret;
		}
		break;
	case SYS_ANALOG:
		ret = drv_vif_set_power_state(E_POWER_SUSPEND);
		if (ret) {
			pr_err("[%s][%d] VIF_suspend_fail !\n",
				__func__, __LINE__);
			return ret;
		}
#if defined(TUNER_OK)
		//fix A->D, tuner if spectrum inverse problem
		if (fe->ops.tuner_ops.release) {
			fe->ops.tuner_ops.release(fe);
		} else {
			pr_err("Failed to get tuner_ops.release\r\n");
			return -ENONET;
		}
#endif
		break;
	case SYS_UNDEFINED:
	default:
		break;
	}

	demod_prvi->fw_downloaded = 0;
	demod_prvi->previous_system = SYS_UNDEFINED;
	c->delivery_system = SYS_UNDEFINED;
	return ret;
}

static int mtk_demod_get_tune_settings(struct dvb_frontend *fe,
				struct dvb_frontend_tune_settings *settings)
{
	pr_info("[%s] is called\n", __func__);
/*
 *	when we use DVBFE_ALGO_SW, aka sw zigzag, we define some para here
 *	otherwise this is useless
 */
	return 0;
}

static int mtk_demod_read_status(struct dvb_frontend *fe,
					enum fe_status *status)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct mtk_demod_dev *demod_prvi = fe->demodulator_priv;
	int ret = 0;
	int lock_stat;
	enum DMD_ATSC_LOCK_STATUS atsclockstatus = DMD_ATSC_NULL;
	enum dmd_isdbt_lock_status isdbt_lock_status = DMD_ISDBT_NULL;
	enum DMD_DTMB_LOCK_STATUS DtmbLockStatus  = DMD_DTMB_NULL;

	u8 foe;
	u8 agc_value = 0;

	//pr_info("[%s] is called\n", __func__);//temp remove printk

	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = m7332_dmd_drv_dvbt_t2_read_status(fe, status);
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
		ret = mdrv_dmd_dvbs_read_status(fe, status);
		break;
	case SYS_DVBC_ANNEX_A:
		ret = m7332_dmd_drv_dvbc_read_status(fe, status);
		break;
	case SYS_DVBC_ANNEX_B:
	case SYS_ATSC:
		atsclockstatus = _mdrv_dmd_atsc_md_getlock(0, DMD_ATSC_GETLOCK);
		*status = 0;
		if (atsclockstatus == DMD_ATSC_UNLOCK)
			*status = FE_TIMEDOUT;
		else if (atsclockstatus == DMD_ATSC_CHECKING)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
		else if (atsclockstatus == DMD_ATSC_LOCK)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER
				| FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
		break;
	case SYS_ISDBT:
		isdbt_lock_status = _mdrv_dmd_isdbt_md_getlock(0,
				DMD_ISDBT_GETLOCK);
		*status = 0;
		if (isdbt_lock_status == DMD_ISDBT_UNLOCK)
			*status = FE_TIMEDOUT;
		else if (isdbt_lock_status == DMD_ISDBT_CHECKING)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
		else if (isdbt_lock_status == DMD_ISDBT_LOCK)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER
				| FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
		break;
	case SYS_DTMB:
		DtmbLockStatus = _mdrv_dmd_dtmb_md_getlock(0,
				DMD_DTMB_GETLOCK);
		*status = 0;
		if (DtmbLockStatus == DMD_DTMB_UNLOCK)
			*status = FE_TIMEDOUT;//FE_NONE;
		else if (DtmbLockStatus == DMD_DTMB_CHECKING)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
		else if (DtmbLockStatus == DMD_DTMB_LOCK)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER
				| FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
		break;
	case SYS_ANALOG:
		drv_vif_read_statics(&agc_value);
		//if (polarity) agc_value = 0xFF - agc_value;
		c->agc_val = agc_value;

		lock_stat = drv_vif_read_cr_lock_status();
		if (lock_stat & 0x01)
			*status = FE_HAS_SIGNAL | FE_HAS_CARRIER
				| FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
		else if (mtk_demod_dvb_time_diff(demod_prvi->start_time) > 60)
			*status = FE_TIMEDOUT;
		else
			*status = FE_NONE;

		foe = drv_vif_read_cr_foe();
		if (*status & FE_HAS_LOCK) {
			c->frequency_offset = drv_vif_foe2hz(foe, 62500/4);

			if (c->ana_sound_std == FE_SOUND_L_P)
				c->frequency_offset *= -1;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum dvbfe_algo mtk_demod_get_frontend_algo(struct dvb_frontend *fe)
{
	pr_info("[%s] is called\n", __func__);

	return DVBFE_ALGO_HW;
}

static int mtk_demod_tune(struct dvb_frontend *fe,
			bool re_tune,
			unsigned int mode_flags,
			unsigned int *delay,
			enum fe_status *status)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret = 0;

	pr_info("[%s] is called\n", __func__);

	if (re_tune) {
#if defined(TUNER_OK)
		if (fe->ops.tuner_ops.init) {
			ret = fe->ops.tuner_ops.init(fe);
		} else {
			pr_err("Failed to get tuner_ops.init\r\n");
			return -ENONET;
		}
#endif
		switch (c->delivery_system) {
		case SYS_DVBT:
		case SYS_DVBT2:
		case SYS_DVBS:
		case SYS_DVBS2:
		case SYS_DVBC_ANNEX_A:
		case SYS_DVBC_ANNEX_B:
		case SYS_ATSC:
		case SYS_ISDBT:
		case SYS_DTMB:
			ret = mtk_demod_set_frontend(fe);
			if (ret)
				return ret;
			break;
		case SYS_ANALOG:
			ret = mtk_demod_set_frontend(fe);
			if (ret)
				return ret;
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}

	*delay = HZ / 5;

	return mtk_demod_read_status(fe, status);
}

static int mtk_demod_set_tone(struct dvb_frontend *fe,
				enum fe_sec_tone_mode tone)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret = 0;

	c->sectone = tone;
	ret = mdrv_dmd_dvbs_diseqc_set_22k(tone);
	return ret;
}

static int mtk_demod_diseqc_send_burst(struct dvb_frontend *fe,
				enum fe_sec_mini_cmd minicmd)
{
	int ret = 0;

	ret = mdrv_dmd_dvbs_diseqc_set_tone(minicmd);
	return ret;
}

static int mtk_demod_diseqc_send_master_cmd(struct dvb_frontend *fe,
				struct dvb_diseqc_master_cmd *cmd)
{
	int ret = 0;

	ret = mdrv_dmd_dvbs_diseqc_send_cmd(cmd);
	return ret;
}

static const struct dvb_frontend_ops mtk_demod_ops = {
	/* delsys only 8 first */
	.delsys = { SYS_DTMB, SYS_DVBT, SYS_DVBT2, SYS_DVBC_ANNEX_A,
SYS_ISDBT, SYS_ATSC, SYS_DVBC_ANNEX_B, SYS_ANALOG, SYS_DVBS, SYS_DVBS2},
	.info = {
		.name = "mtk_demod",
		.frequency_min_hz  =  42 * MHz,
		.frequency_max_hz  =  2150 * MHz,
		.caps = FE_CAN_QAM_64 |
			FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_MUTE_TS
	},

	.sleep = mtk_demod_sleep,
	.init = mtk_demod_init,
	.set_frontend = mtk_demod_set_frontend,
	.get_frontend = mtk_demod_get_frontend,
	.get_tune_settings = mtk_demod_get_tune_settings,
	.read_status = mtk_demod_read_status,
	.get_frontend_algo = mtk_demod_get_frontend_algo,
	.tune = mtk_demod_tune,
	.read_signal_strength = mtk_demod_read_signal_strength,
	.set_tone = mtk_demod_set_tone,
	.diseqc_send_burst = mtk_demod_diseqc_send_burst,
	.diseqc_send_master_cmd = mtk_demod_diseqc_send_master_cmd,
};

#if defined(TUNER_OK)
static const struct dvb_tuner_ops mtk_tuner_ops = {
	.info = {
		.name             = "mtk tuner",
		.frequency_min_hz =  42 * MHz,
		.frequency_max_hz = 2150 * MHz,
	},

	.init = mtk_tuner_init,
	.release = mtk_tuner_release,
	.sleep = mtk_tuner_sleep,
	.suspend = mtk_tuner_suspend,
	.resume = mtk_tuner_resume,
	.set_params = mtk_tuner_set_params,
	.set_analog_params = mtk_tuner_set_analog_parms,
	.set_config = mtk_tuner_set_config,
	.get_frequency = mtk_tuner_get_frequency,
	.get_bandwidth = mtk_tuner_get_bandwidth,
	.get_if_frequency = mtk_tuner_get_if_frequency,
	.get_status = mtk_tuner_get_status,
	.get_rf_strength = mtk_tuner_get_rf_strength,
	.get_afc = mtk_tuner_get_afc,
	.set_frequency = mtk_tuner_set_frequency,
	.set_bandwidth = mtk_tuner_set_bandwidth,
};
#endif

static int m7332_demod_probe(struct platform_device *pdev)
{
	struct mtk_demod_dev *demod_dev;
	struct resource *res;
	int ret = 0;


	demod_dev = devm_kzalloc(&pdev->dev, sizeof(struct mtk_demod_dev),
							GFP_KERNEL);
	if (!demod_dev)
		return -ENOMEM;

	demod_dev->pdev = pdev;

/*	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
 *	if (!res) {
 *		pr_err("Failed to get reg resource\r\n");
 *		return -ENONET;
 *	}
 *	pr_info("[%s][%d] : start = %llx, end = %llx\r\n",
 *		__func__, __LINE__, res->start, res->end);

 *	demod_dev->virt_riu_base_addr = devm_ioremap_resource(&pdev->dev, res);
 *	pr_info("[%s][%d]  : virtDMDBaseAddr = %lx\r\n",
 *		__func__, __LINE__, demod_dev->virt_riu_base_addr);
 *	if (IS_ERR(demod_dev->virt_riu_base_addr))
 *		return PTR_ERR(demod_dev->virt_riu_base_addr);
 *
 */
	demod_dev->virt_riu_base_addr = ioremap(TMP_RIU_BASE_ADDRESS,
					TMP_RIU_MAPPING_SIZE);
	pr_info("[%s][%d]  : virtDMDBaseAddr = %lx\r\n",
		__func__, __LINE__, demod_dev->virt_riu_base_addr);

	/* need to modify, get DRAM addr from dts. */
	demod_dev->dram_base_addr = TMP_DRAM_START_ADDRESS;
	sdmd_isdbt_initdata.tdi_start_addr = demod_dev->dram_base_addr;
	sdmd_isdbt_initdata.dmd_isdbt_init_ext = demod_dev->virt_riu_base_addr;
	sDMD_DTMB_InitData.utdistartaddr32 = demod_dev->dram_base_addr;
	sDMD_DTMB_InitData.u8DMD_DTMB_InitExt = demod_dev->virt_riu_base_addr;
	sdmd_atsc_initdata.dmd_atsc_initext = demod_dev->virt_riu_base_addr;

#ifdef TUNER_OK_2 //ckang adapter
	demod_dev->adapter = mdrv_get_dvb_adapter();
	if (!demod_dev->adapter) {
		pr_err("[%s][%d] get dvb adapter fail\n", __func__, __LINE__);
		return -ENODEV;
	}
#endif
	memcpy(&demod_dev->fe.ops, &mtk_demod_ops,
		sizeof(struct dvb_frontend_ops));
	demod_dev->fe.demodulator_priv = demod_dev;

#if defined(TUNER_OK)
	memcpy(&demod_dev->fe.ops.tuner_ops, &mtk_tuner_ops,
		sizeof(struct dvb_tuner_ops));
#endif

#ifdef TUNER_OK_2 //ckang adapter
	ret = dvb_register_frontend(demod_dev->adapter, &demod_dev->fe);
	if (ret < 0) {
		pr_err("dvb_register_frontend is fail\n");
		return ret;
	}
#endif
	/* demod_dev->previous_system = demod_dev->fe.ops.delsys[0]; */
	demod_dev->previous_system = SYS_UNDEFINED;
	demod_dev->fw_downloaded = 0;
	platform_set_drvdata(pdev, demod_dev);

	return 0;
}

static int m7332_demod_remove(struct platform_device *pdev)
{
	struct mtk_demod_dev *demod_dev;
	int ret = 0;

	pr_info("[%s] is called\n", __func__);

	demod_dev = platform_get_drvdata(pdev);

	if (!demod_dev) {
		pr_err("Failed to get demod_dev\n");
		return -EINVAL;
	}

	/* check if fw_loaded */
	if (demod_dev->fw_downloaded) {
		/* exit previous system */
		switch (demod_dev->previous_system) {
		case SYS_DVBT:
		case SYS_DVBT2:
			ret = m7332_dmd_drv_dvbt_t2_exit();
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbt_t2_exit fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBS:
		case SYS_DVBS2:
			ret = mdrv_dmd_dvbs_exit();
			if (ret) {
				pr_err("[%s][%d] MDrv_DMD_DVBS_Exit fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBC_ANNEX_A:
			ret = m7332_dmd_drv_dvbc_exit();
			if (ret) {
				pr_err("[%s][%d] m7332_dmd_drv_dvbc_exit fail !\n",
					__func__, __LINE__);
				return ret;
			}
			break;
		case SYS_DVBC_ANNEX_B:
		case SYS_ATSC:
			ret = !_mdrv_dmd_atsc_md_exit(0);
			break;
		case SYS_ISDBT:
			ret = !_mdrv_dmd_isdbt_md_exit(0);
			break;
		case SYS_DTMB:
			pr_info("[%s],[%d]:DTMB remove is called\n",
			__func__, __LINE__);
			ret = !_mdrv_dmd_dtmb_md_exit(0);
			break;
		case SYS_ANALOG:
			pr_info("vif_exit (borrow dab) in remove is called\n");
			drv_vif_exit();
			break;
		case SYS_UNDEFINED:
		default:
			break;
		}
		demod_dev->fw_downloaded = 0;

		/* update system */
		demod_dev->previous_system = SYS_UNDEFINED;
	}

	dvb_unregister_frontend(&demod_dev->fe);

	return ret;
}

static int m7332_demod_suspend(struct device *dev)
{
	/* @TODO: Not yet implement */
	return 0;
}

static int m7332_demod_resume(struct device *dev)
{
	/* @TODO: Not yet implement */
	return 0;
}

static const struct dev_pm_ops m7332_demod_pm_ops = {
	.suspend = m7332_demod_suspend,
	.resume = m7332_demod_resume,
};

static const struct of_device_id m7332_demod_dt_match[] = {
	{ .compatible = DRV_NAME },
	{},
};

MODULE_DEVICE_TABLE(of, m7332_demod_dt_match);

static struct platform_driver m7332_demod_driver = {
	.probe	  = m7332_demod_probe,
	.remove	 = m7332_demod_remove,
	.driver	 = {
		.name   = DRV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = m7332_demod_dt_match,
		.pm = &m7332_demod_pm_ops,
		.groups  = dmd_groups
	},
};

module_platform_driver(m7332_demod_driver);

MODULE_DESCRIPTION("Mediatek Demod Driver");
MODULE_AUTHOR("Mediatek Author");
MODULE_LICENSE("GPL");
