// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <media/dvb_frontend.h>
#include "drvDMD_DVBS.h"
#include "halDMD_INTERN_DVBS.h"
#include "m7332_demod_common.h"

int mdrv_dmd_dvbs_init(struct dvb_frontend *fe)
{
	u32 ret = 0;

	intern_dvbs_init_clk(fe);
	ret = intern_dvbs_load_code();

	return ret;
}

int mdrv_dmd_dvbs_exit(void)
{
	u32 ret = 0;

	ret = intern_dvbs_exit();

	return ret;
}

int mdrv_dmd_dvbs_config(struct dvb_frontend *fe)
{
	u32 ret = 0;

	ret = intern_dvbs_config(fe);

	return ret;
}

int mdrv_dmd_dvbs_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	u32 ret = 0;

	ret = intern_dvbs_read_status(fe, status);

	return ret;
}

int mdrv_dmd_dvbs_get_frontend(struct dvb_frontend *fe,
				 struct dtv_frontend_properties *p)
{
	return intern_dvbs_get_frontend(fe, p);
}

int mdrv_dmd_dvbs_get_ifagc(u16 *ifagc)
{
	return intern_dvbs_get_ifagc(ifagc);
}

int mdrv_dmd_dvbs_get_rf_level(u16 ifagc, u16 *rf_level)
{
	return intern_dvbs_get_rf_level(ifagc, rf_level);
}

int mdrv_dmd_dvbs_get_snr(s16 *snr)
{
	int bRet;

	bRet = intern_dvbs_get_snr(snr);
	return bRet;
}

int mdrv_dmd_dvbs_get_signal_strength(u16 rssi, u16 *strength)
{
	int bRet;

	bRet = intern_dvbs_get_signal_strength(rssi, strength);
	return bRet;
}

int mdrv_dmd_dvbs_get_signal_quality(u16 *quality)
{
	int bRet;

	bRet = intern_dvbs_get_signal_quality(quality);
	return bRet;
}

int mdrv_dmd_dvbs_get_ber(u32 *ber)
{
	int bRet;

	bRet = intern_dvbs_get_ber(ber);
	return bRet;
}

int mdrv_dmd_dvbs_get_pkterr(u32 *pktErr)
{
	int bRet;

	bRet = intern_dvbs_get_pkterr(pktErr);
	return bRet;
}

int mdrv_dmd_dvbs_get_freqoffset(s32 *cfo)
{
	int bRet;

	bRet = intern_dvbs_get_freqoffset(cfo);
	return bRet;
}

int mdrv_dmd_dvbs_get_ts_rate(u32 *clock_rate)
{
	u64  CLK_RATE = 0;

	intern_dvbs_get_ts_rate(&CLK_RATE);
	*clock_rate = (u32)(CLK_RATE/1000);
	return 1;
}
int mdrv_dmd_dvbs_diseqc_set_22k(enum fe_sec_tone_mode tone)
{
	int bRet;

	bRet = intern_dvbs_diseqc_set_22k(tone);

	return bRet;
}

int mdrv_dmd_dvbs_diseqc_send_cmd(struct dvb_diseqc_master_cmd *cmd)
{
	int bRet;

	bRet = intern_dvbs_diseqc_send_cmd(cmd);

	return bRet;
}

int mdrv_dmd_dvbs_diseqc_set_tone(enum fe_sec_mini_cmd minicmd)
{
	int bRet = 0;

	bRet = intern_dvbs_diseqc_set_tone(minicmd);

	return bRet;
}

ssize_t dvbs_get_information_show(struct device_driver *driver, char *buf)
{
	u8 u8temp;
	u16 u16temp;
	u32 u32temp;
	int chip_revision;
	int ts_rate;
	int fec_type;

	// Get TS_Rate_layerA/B/C
	mdrv_dmd_dvbs_get_ts_rate(&u32temp);
	ts_rate = (int)u32temp;

	// Get Chip_Revision
	intern_dvbs_version(&u16temp);
	chip_revision = (int)u16temp;

	// Get FEC Type
	intern_dvbs_get_fec_type(&u8temp);
	fec_type = (int)u8temp;

	return scnprintf(buf, PAGE_SIZE, "%d %d %d\n",
	ts_rate, chip_revision, fec_type);
}

