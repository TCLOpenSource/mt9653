// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/time64.h>
#include <linux/pm_runtime.h>
#include <asm-generic/div64.h>
#include "mtk-bw.h"

/* DRAM config */
static u32 bus_width;
static u32 dram_clock;
static u32 emi_clock;
static u32 smi_clock;
static u32 dram_penalty;
static u32 total_bw;

/* EMI arch describe*/
static struct module_map *mlist;
static u32 emi_cnt;
static u32 smi_common_cnt;
static u32 emi_port_cnt;
static u32 module_cnt;
static u32 sum_id;
static u32 ref_emi_id;
static u32 emi_effi_mon;

/* fake engine */
static u32 fake_eng_st_align;
static u32 fake_eng_loop_mode;
static u32 fake_eng_num;
static u32 *fake_eng_list;
static u32 *fake_eng_result;
static u32 *fake_eng_loop;

static u32 fake_eng13_loop_mode;
static u32 fake_eng13_result;
static u32 fake_eng19_loop_mode;
static u32 fake_eng19_result;
static u32 fake_eng21_loop_mode;
static u32 fake_eng21_result;
static u32 fake_eng29_loop_mode;
static u32 fake_eng29_result;
static u32 fake_eng44_loop_mode;
static u32 fake_eng44_result;
static u32 fake_eng52_loop_mode;
static u32 fake_eng52_result;

/* bandwidth monitor config */
static struct bw_cfg larb_mon_cfg;
static struct bw_cfg total_mon_cfg;
static struct bw_cfg scomm_mon_cfg;
static struct bw_cfg emi_mon_cfg;

/* register operation */
static u32 dump_reg_addr;

/* debug */
static u32 debug_on;

static const u32 mt5896_lpddr4_effi_table[EFFI_BURST_SET_CNT] = {
	0x200a, 0x402d, 0x5446, 0x5654, 0x5656, 0x5656, 0x5656, 0x5656,
};

static const u32 mt5876_effi_table[EFFI_BURST_SET_CNT * mt5876_effi_cnt] = {
	0x1f10, 0x382a, 0x473b, 0x4e44, 0x5148, 0x524b, 0x524c, 0x584d, //2666
	0x1a0d, 0x3125, 0x4136, 0x4a40, 0x4e46, 0x5049, 0x514b, 0x554c, //3200
	0x1c0e, 0x3427, 0x4438, 0x4c42, 0x4f47, 0x514a, 0x514b, 0x564c, //2933
	0x2512, 0x4836, 0x5b4b, 0x5c51, 0x5d54, 0x5d55, 0x5d57, 0x5d58, //2133
};

static const u32 mt5879_effi_table[EFFI_BURST_SET_CNT * mt5879_effi_cnt] = {
	0x2311, 0x4232, 0x5547, 0x594F, 0x5B53, 0x5C54, 0x5C55, 0x5B55, //2666
	0x1D0E, 0x3428, 0x453A, 0x4F45, 0x544C, 0x554F, 0x564F, 0x5650, //3200
	0x200F, 0x3B2D, 0x4D40, 0x544A, 0x574F, 0x5851, 0x5952, 0x5852, //2933
};

static const struct of_device_id mtk_effi_of_ids[] = {
	{
		.compatible = "mediatek,mt5896-effi-mon",
		.data = mt5896_lpddr4_effi_table,
	},
	{
		.compatible = "mediatek,mt5876-effi-mon",
		.data = mt5876_effi_table,
	},
	{
		.compatible = "mediatek,mt5879-effi-mon",
		.data = mt5879_effi_table,
	},
	{}
};

static const struct effi_mon_mapping effi_device_map[EFFI_MON_CNT] = {
	{SMI_INPUT, SMI_COMMON(0), P0, EFFI_MON(0)},
	{SMI_INPUT, SMI_COMMON(0), P1, EFFI_MON(1)},
	{SMI_INPUT, SMI_COMMON(1), P0, EFFI_MON(2)},
	{SMI_INPUT, SMI_COMMON(1), P1, EFFI_MON(3)},
	{SMI_INPUT, SMI_COMMON(2), P0, EFFI_MON(4)},
	{SMI_INPUT, SMI_COMMON(5), P0, EFFI_MON(5)},
	{SMI_INPUT, SMI_COMMON(3), P0, EFFI_MON(6)},
	{SMI_INPUT, SMI_COMMON(4), P0, EFFI_MON(7)},
	{SMI_INPUT, SMI_COMMON(6), P0, EFFI_MON(8)},
	{SMI_INPUT, SMI_COMMON(6), P1, EFFI_MON(9)},
	{SMI_INPUT, SMI_COMMON(7), P0, EFFI_MON(10)},
	{SMI_INPUT, SMI_COMMON(7), P1, EFFI_MON(11)},
	{SMI_INPUT, SMI_COMMON(2), P1, EFFI_MON(12)},
	{SMI_INPUT, SMI_COMMON(5), P1, EFFI_MON(13)},
};

static const struct effi_mon_mapping emi_effi_device_map[EMI_EFFI_MON_CNT] = {
	{EMI_INPUT, CEN_EMI(0),    P0, EFFI_MON(0)},
	{EMI_INPUT, CEN_EMI(1),    P0, EFFI_MON(1)},
	{SMI_INPUT, SMI_COMMON(0), P0, EFFI_MON(2)},
	{SMI_INPUT, SMI_COMMON(0), P1, EFFI_MON(3)},
	{SMI_INPUT, SMI_COMMON(1), P0, EFFI_MON(4)},
	{SMI_INPUT, SMI_COMMON(2), P0, EFFI_MON(5)},
	{EMI_INPUT, CEN_EMI(6),    P0, EFFI_MON(6)},
	{EMI_INPUT, CEN_EMI(7),    P0, EFFI_MON(7)},
};

static void putbuf(char *buf, u32 *len, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	*len += vscnprintf(buf+(*len), PAGE_SIZE-(*len), fmt, args);
	va_end(args);
}

int get_module_id_by_name(char *name, unsigned int *id)
{
	u32 m_idx;

	for (m_idx = 0; m_idx < module_cnt; m_idx++) {
		if (strncmp(mlist[m_idx].name, name, MAX_CMP_NUM) == 0) {
			*id = mlist[m_idx].module_id;
			return 0;
		}
	}
	return -1;
}

static const char *get_module_name_by_id(int id)
{
	u32 m_idx;

	for (m_idx = 0; m_idx < module_cnt; m_idx++) {
		if (mlist[m_idx].module_id == id)
			return mlist[m_idx].name;
	}

	return NULL;
}

static void put_buf_all_module(struct bw_cfg *cfg,
			       struct bw_res *res[],
			       char *buf,
			       unsigned int *len)
{
	u32 ratio;
	u32 m_idx;
	u32 bus_bw_ratio;
	s64 slack_ratio;
	s64 slack;

	putbuf(buf, len, "Starting Monitor......\033[K\n\033[K\n");
	putbuf(buf, len, "\033[7m%4s%10s%12s%12s%12s%12s%12s%12s",
			"ID",
			"MODULE",
			"DOMAIN",
			"DATA_BW",
			"EFFI(%)",
			"BUS_BW",
			"ULTRA(%)",
			"PREULTRA(%)");
	putbuf(buf, len, "\033[0m\033[K\n");
	for (m_idx = 0; m_idx < module_cnt; m_idx++) {
		if (!mlist[m_idx].enable)
			continue;
		/* if proc_mon = 1, only monitor processor */
		if (mlist[m_idx].larb_num != 0 && total_mon_cfg.proc_mon == 1)
			continue;
		putbuf(buf, len, "%4u%10s%12s%12llu%12u%12llu%12u%12u",
				m_idx,
				mlist[m_idx].name,
				mlist[m_idx].domain,
				res[m_idx]->data_bw,
				res[m_idx]->m_effi,
				res[m_idx]->bus_bw,
				res[m_idx]->r_ultra,
				res[m_idx]->r_pultra);
		putbuf(buf, len, "\033[K\n");
	}

	putbuf(buf, len, "\033[K\n");

	/* print ratio */
	for (m_idx = 0; m_idx < module_cnt; m_idx++) {
		if (!mlist[m_idx].enable || res[m_idx]->bus_bw == 0)
			continue;
		ratio = PERCENTAGE(res[m_idx]->bus_bw, total_bw)/RATIO_SCALE_FACTOR;
		putbuf(buf, len,
			"%-9s :%7llu (MB/s)%4u o/oo[\033[7m%*s\033[0m%*s]",
			mlist[m_idx].name,
			res[m_idx]->bus_bw,
			(u32)(PERMILLAGE(res[m_idx]->bus_bw, total_bw)),
			ratio,
			"",
			50-ratio,
			"");
		putbuf(buf, len, "\033[K\n");
	}
	putbuf(buf, len, "\033[K\n");

	putbuf(buf, len, "\033[4m%23s\033[0m\033[K\n", "");
	putbuf(buf, len, "\033[7m|%8s  |%8s  |\033[0m\033[K\n",
			"Width",
			"Clock");
	putbuf(buf, len, "\033[0m\033[4m|%7u   |%8u  |\033[0m\033[K\n",
			bus_width,
			dram_clock);

	putbuf(buf, len, "\033[0m\033[K\n");

	slack = (s64)total_bw-(s64)res[sum_id]->bus_bw;
	slack_ratio = S_PERCENTAGE(slack, (s32)total_bw);

	putbuf(buf, len, "\033[4m%34s\033[0m\033[K\n", "");
	putbuf(buf, len, "\033[7m|%8s  |%8s  |%8s  |\033[0m\033[K\n",
			"TotalBW",
			"BusBW",
			"Slack");
	putbuf(buf, len, "\033[0m\033[4m|%8u  |%8llu  |%8lld  |\033[0m\033[K\n",
			total_bw,
			res[sum_id]->bus_bw,
			slack);
	putbuf(buf, len, "\033[4m|%7u%%  |%7llu%%  |%6lld%%   |\033[0m\033[K\n",
			100,
			PERCENTAGE(res[sum_id]->bus_bw, total_bw),
			slack_ratio);
	putbuf(buf, len, "\033[0m\033[K\n\033[K\n");

	/* print status for all*/
	bus_bw_ratio = PERCENTAGE(res[sum_id]->bus_bw, total_bw);

	if (bus_bw_ratio > dram_penalty)
		putbuf(buf, len, "Status: \033[41;30m  DANGER  ");
	else if (bus_bw_ratio > CAL_WARNING_BW_RATIO(dram_penalty))
		putbuf(buf, len, "Status: \033[43;30m  WARNING  ");
	else
		putbuf(buf, len, "Status: \033[42;30m   SAFE   ");

	putbuf(buf, len, "\033[0m\033[K\n\033[K\n");
}

static unsigned int put_buf_larb_left(struct bw_cfg *cfg,
				      struct larb_res *res[],
				      char *buf,
				      unsigned int *len,
				      unsigned int larb_num)
{
	u32 j;
	u32 left_pos;
	u32 next_p_nr;
	struct module_map m = mlist[cfg->m_id];

	putbuf(buf, len, "SMI_LARB %d\033[K\n", m.llist[larb_num]);
	putbuf(buf, len, "\033[7m");
	putbuf(buf, len, "%3s%4s%3s%3s%6s%6s%6s%5s%5s%5s%5s",
			"ID", "OL", "U", "PU",
			"R_BW", "W_BW",
			"M_CP", "B", "E", "UR", "PUR");
	putbuf(buf, len, "\033[0m\033[K\n");

	for (j = 0; j < m.plist[larb_num].pcount; j++) {
		putbuf(buf, len, "\033[0m");
		putbuf(buf, len, "%3d%4x%3u%3u%6llu%6llu%6u%5u%5u%5u%5u",
					m.plist[larb_num].smi_plist[j],
					res[larb_num]->ostdl[j],
					res[larb_num]->ultra[j],
					res[larb_num]->pultra[j],
					res[larb_num]->r_bw[j],
					res[larb_num]->w_bw[j],
					res[larb_num]->max_cp[j],
					res[larb_num]->burst[j],
					res[larb_num]->effi[j],
					res[larb_num]->r_ultra[j],
					res[larb_num]->r_pultra[j]);
		putbuf(buf, len, "\033[K\n");
	}
	left_pos = j;
	if (larb_num != m.larb_num-1) {
		next_p_nr = m.plist[larb_num+1].pcount;
		if (m.plist[larb_num].pcount < next_p_nr) {
			for (j = 0; j < next_p_nr - left_pos; j++)
				putbuf(buf, len, "\033[K\n");
			for (j = 0; j < next_p_nr+2; j++)
				putbuf(buf, len, "\033[1A");
		} else {
			for (j = 0; j < m.plist[larb_num].pcount+2; j++)
				putbuf(buf, len, "\033[1A");
		}
	} else {
		putbuf(buf, len, "\033[K\n");
	}

	return left_pos;

}

static void put_buf_larb_right(struct bw_cfg *cfg,
			       struct larb_res *res[],
			       char *buf,
			       unsigned int *len,
			       unsigned int larb_num,
			       unsigned int left_pos)
{
	u32 j, k;
	struct module_map m = mlist[cfg->m_id];

	putbuf(buf, len, "\033[60C");
	putbuf(buf, len, "SMI_LARB %d\033[K\n", m.llist[larb_num]);
	putbuf(buf, len, "\033[60C\033[7m");
	putbuf(buf, len, "%3s%4s%3s%3s%6s%6s%6s%5s%5s%5s%5s\033[0m",
			"ID", "OL", "U", "PU",
			"R_BW", "W_BW",
			"M_CP", "B", "E", "UR", "PUR");
	putbuf(buf, len, "\033[K\n");

	for (j = 0; j < m.plist[larb_num].pcount; j++) {
		putbuf(buf, len, "\033[60C\033[0m");
		putbuf(buf, len, "%3d%4x%3u%3u%6llu%6llu%6u%5u%5u%5u%5u",
					m.plist[larb_num].smi_plist[j],
					res[larb_num]->ostdl[j],
					res[larb_num]->ultra[j],
					res[larb_num]->pultra[j],
					res[larb_num]->r_bw[j],
					res[larb_num]->w_bw[j],
					res[larb_num]->max_cp[j],
					res[larb_num]->burst[j],
					res[larb_num]->effi[j],
					res[larb_num]->r_ultra[j],
					res[larb_num]->r_pultra[j]);
		putbuf(buf, len, "\033[K\n");
	}
	if (j < left_pos) {
		for (k = 0; k < (left_pos-j); k++)
			putbuf(buf, len, "\033[1B");
		putbuf(buf, len, "\033[60C\033[K");
	}
	putbuf(buf, len, "\033[K\n");
}

static void put_buf_larb(struct bw_cfg *cfg,
			 struct larb_res *res[],
			 char *buf,
			 unsigned int *len)
{
	u32 i;
	u32 left_pos;
	u64 data_bw = 0;
	u64 bus_bw = 0;
	u64 effi = 0;
	struct module_map m = mlist[cfg->m_id];

	if (m.larb_num == 0) {
		putbuf(buf, len, "%s not support larb monitor\033[K\n", m.name);
	} else {
		putbuf(buf, len, "Starting Monitor......\033[K\n\033[K\n");
		putbuf(buf, len, "Module : \033[46;30m  %s  ", m.name);
		putbuf(buf, len, "\033[0m\033[K\n\033[K\n");
		putbuf(buf, len, "OL: Outstanding Limit(Hex)\033[K\n");
		putbuf(buf, len, "U: Force ultra PU: Force pre-ultra\033[K\n");
		putbuf(buf, len, "R_BW / W_BW: R/W BW (MB/s)\033[K\n");
		putbuf(buf, len, "M_CP: MAX CP(ns)\033[K\n");
		putbuf(buf, len, "B: AVG Burst\033[K\n");
		putbuf(buf, len, "E: Efficiency\033[K\n");
		putbuf(buf, len, "UR: Ultra ratio\033[K\n");
		putbuf(buf, len, "PUR: Pre-Ultra ratio\033[K\n\033[K\n");
		for (i = 0; i < m.larb_num; i++) {
			if (i % 2 == 0)
				left_pos = put_buf_larb_left(&larb_mon_cfg,
							      res,
							      buf,
							      len,
							      i);
			else
				put_buf_larb_right(&larb_mon_cfg,
						   res,
						   buf,
						   len,
						   i,
						   left_pos);

			data_bw += res[i]->larb_data_bw;
			bus_bw += res[i]->larb_bus_bw;
		}
		if (bus_bw)
			effi = PERCENTAGE(data_bw, bus_bw);

		putbuf(buf, len, "\033[4m%33s\033[0m\033[K\n", "");
		putbuf(buf, len, "\033[7m|%8s  |%7s  |%8s  |\033[0m\033[K\n",
				"Data BW",
				"Effi",
				"Bus BW");
		putbuf(buf, len, "\033[0m\033[4m|%7llu   |%7llu  |%8llu  |",
				data_bw,
				effi,
				bus_bw);
		putbuf(buf, len, "\033[0m\033[K\n");
		putbuf(buf, len, "\033[0m\033[K\n");
	}
}

static void put_buf_common(struct bw_cfg *cfg,
			   struct scomm_res *res[],
			   char *buf,
			   unsigned int *len)
{
	u32 i, j;
	u32 skip_flag;

	putbuf(buf, len, "Starting Monitor......\033[K\n\033[K\n");
	for (i = 0; i < smi_common_cnt; i++) {
		skip_flag = 0;
		for (j = 0; j < res[i]->op_num; j++) {
			if (res[i]->r_bw[j] == 0 && res[i]->w_bw[j] == 0)
				skip_flag++;
		}
		if (skip_flag == res[i]->op_num)
			continue;

		putbuf(buf, len, "SMI_COMMON %d\033[K\n", res[i]->id);

		putbuf(buf, len, "\033[7m");
		putbuf(buf, len, "%5s%5s%9s%9s%7s%7s%6s%6s%6s%6s",
				"TYPE",
				"PORT",
				"R_BW",
				"W_BW",
				"M_RCP",
				"M_WCP",
				"BURST",
				"EFFI",
				"UR",
				"PUR");

		putbuf(buf, len, "\033[0m\033[K\n");

		for (j = 0; j < res[i]->ip_num; j++) {

			if (res[i]->r_bw[j + res[i]->op_num] == 0
					&& res[i]->w_bw[j + res[i]->op_num] == 0)
				continue;

			putbuf(buf, len, "\033[0m");
			putbuf(buf, len, "%5s%5d%9llu%9llu%7u%7u%6u%6u%6u%6u",
					"In",
					j,
					res[i]->r_bw[j + res[i]->op_num],
					res[i]->w_bw[j + res[i]->op_num],
					res[i]->r_max_cp[j + res[i]->op_num],
					res[i]->w_max_cp[j + res[i]->op_num],
					res[i]->burst[j + res[i]->op_num],
					res[i]->effi[j + res[i]->op_num],
					res[i]->r_ultra[j + res[i]->op_num],
					res[i]->r_pultra[j + res[i]->op_num]);
			putbuf(buf, len, "\033[K\n");
		}

		for (j = 0; j < res[i]->op_num; j++) {

			if (res[i]->r_bw[j] == 0 && res[i]->w_bw[j] == 0)
				continue;

			putbuf(buf, len, "\033[0m");
			putbuf(buf, len, "%5s%5d%9llu%9llu%7u%7u%6s%6s%6s%6s",
					"Out",
					j,
					res[i]->r_bw[j],
					res[i]->w_bw[j],
					res[i]->r_max_cp[j],
					res[i]->w_max_cp[j],
					"-",
					"-",
					"-",
					"-");
			putbuf(buf, len, "\033[K\n");
		}

		putbuf(buf, len, "\033[K\n");
	}
}

static void put_buf_emi_detail(struct bw_cfg *cfg,
			       struct emi_res *res,
			       char *buf,
			       unsigned int *len)
{
	int i, j;

	putbuf(buf, len, "Starting Monitor......\033[K\n\033[K\n");
	for (i = 0; i < emi_cnt; i++) {
		putbuf(buf, len, "CENTRAL_EMI %d\033[K\n", i);
		putbuf(buf, len, "\033[7m");
		putbuf(buf, len, "%5s%7s%7s%7s%7s%7s%10s%10s%10s%10s%10s",
				"PORT",
				"BW_LMT",
				"RTIMER",
				"WTIMER",
				"ROSTD",
				"WOSTD",
				"DATA_BW",
				"EFFI(%)",
				"BURST",
				"BUS_BW",
				"LATENCY");
		putbuf(buf, len, "\033[0m\033[K\n");
		for (j = 0; j < emi_port_cnt; j++) {
			putbuf(buf, len, "%5d", j);
			putbuf(buf, len, "%7lX", EMI_GET_BW_LMT(res[i].arb_info[j]));
			putbuf(buf, len, "%7lX", EMI_GET_RD_TIMER(res[i].arb_info[j]));
			putbuf(buf, len, "%7lX", EMI_GET_WR_TIMER(res[i].arb_info[j]));
			putbuf(buf, len, "%7X", res[i].rd_ostd[j]);
			putbuf(buf, len, "%7X", res[i].wr_ostd[j]);
			putbuf(buf, len, "%10llu", res[i].data_bw[j]);
			putbuf(buf, len, "%10u", res[i].m_effi[j]);
			putbuf(buf, len, "%10u", res[i].burst[j] + 1);
			putbuf(buf, len, "%10llu", CAL_BUS_BW(res[i].data_bw[j],
							      res[i].m_effi[j]));
			putbuf(buf, len, "%10llu", res[i].latency[j]);
			putbuf(buf, len, "\033[K\n");
		}
		putbuf(buf, len, "\033[K\n");
	}
}

static void put_buf_emi_total(struct bw_cfg *cfg,
			      struct emi_res *res,
			      char *buf,
			      unsigned int *len)
{
	int i;
	u64 total_data_bw, total_bus_bw;

	putbuf(buf, len, "Starting Monitor......\033[K\n\033[K\n");
	putbuf(buf, len, "BW_LMT; RD/WR_TIMER; RD/WR_OSTD: Bandwidth info.(HEX)\033[K\n");
	putbuf(buf, len, "DATA_BW: Data Bandwidth (MB/s)\033[K\n");
	putbuf(buf, len, "BUS_BW: Bus Bandwidth (MB/s)\033[K\n");
	putbuf(buf, len, "EFFI: Efficiency (%s)\033[K\n", "%");
	putbuf(buf, len, "LATENCY: Average latency (ns)\033[K\n");
	putbuf(buf, len, "\033[K\n");
	putbuf(buf, len, "CENTRAL_EMI ALL\033[K\n");
	putbuf(buf, len, "\033[7m");
	putbuf(buf, len, "%5s%10s%10s%10s%10s%10s",
			"PORT",
			"DATA_BW",
			"EFFI(%)",
			"BURST",
			"BUS_BW",
			"LATENCY");
	putbuf(buf, len, "\033[0m\033[K\n");
	total_data_bw = 0;
	total_bus_bw = 0;
	for (i = 0; i <= emi_port_cnt; i++) {
		if (i == emi_port_cnt) {
			putbuf(buf, len, "%5s", "ALL");
			putbuf(buf, len, "%10llu", total_data_bw);
			putbuf(buf, len, "%10llu", PERCENTAGE(total_data_bw,
							      total_bus_bw));
			putbuf(buf, len, "%10s", "-");
			putbuf(buf, len, "%10llu", total_bus_bw);
			putbuf(buf, len, "%10s", "-");
			putbuf(buf, len, "\033[K\n");
			break;
		}
		putbuf(buf, len, "%5d", i);
		putbuf(buf, len, "%10llu", res[emi_cnt].data_bw[i]);
		putbuf(buf, len, "%10u", res[emi_cnt].m_effi[i]);
		putbuf(buf, len, "%10u", res[emi_cnt].burst[i] + 1);
		putbuf(buf, len, "%10llu", CAL_BUS_BW(res[emi_cnt].data_bw[i],
						      res[emi_cnt].m_effi[i]));
		putbuf(buf, len, "%10llu", res[emi_cnt].latency[i]);
		putbuf(buf, len, "\033[K\n");
		total_data_bw += res[emi_cnt].data_bw[i];
		total_bus_bw += CAL_BUS_BW(res[emi_cnt].data_bw[i],
					   res[emi_cnt].m_effi[i]);
	}
	putbuf(buf, len, "\033[K\n");
}

static void *find_device_addr(char type, u32 id)
{
	struct device_node *np;
	char dev_name[MAX_DEV_NAME_LEN];
	void __iomem *base;
	int num;

	if (type == EMI_CEN_DRV)
		num = snprintf(dev_name, MAX_DEV_NAME_LEN, "emi_cen%d", id);
	else if (type == SMI_LARB_DRV)
		num = snprintf(dev_name, MAX_DEV_NAME_LEN, "larb%d", id);
	else if (type == SMI_COMMON_DRV)
		num = snprintf(dev_name, MAX_DEV_NAME_LEN, "smi_common%d", id);
	else if (type == EMI_DISPATCH_DRV)
		num = snprintf(dev_name, MAX_DEV_NAME_LEN, "emi_dispatch%d", id);
	else if (type == EFFI_MONITOR_DRV)
		num = snprintf(dev_name, MAX_DEV_NAME_LEN, "effi_monitor%d", id);
	else if (type == CPU_PRI_DRV)
		num = snprintf(dev_name, MAX_DEV_NAME_LEN, "cpu_pri");
	else if (type == MIU2GMC_DRV)
		num = snprintf(dev_name, MAX_DEV_NAME_LEN, "miu2gmc%d", id);
	else
		num = MAX_DEV_NAME_LEN;

	if (num >= MAX_DEV_NAME_LEN)
		return NULL;

	np = of_find_node_by_name(NULL, dev_name);
	if (!np)
		return NULL;

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("[MTK_BW]%pOF: of_iomap failed\n", np);
		return NULL;
	}

	return base;
}

static int emi_set_base(struct bw_cfg *cfg)
{
	int idx;

	/* get emi base */
	for (idx = 0; idx < emi_cnt; idx++) {
		switch (idx) {
		case EMI_0:
			cfg->base = find_device_addr(EMI_CEN_DRV, EMI_0);
			if (!cfg->base) {
				pr_err("[MTK_BW]Can't find EMI device base!!\n");
				return -ENXIO;
			}
			break;
		case EMI_1:
			cfg->base1 = find_device_addr(EMI_CEN_DRV, EMI_1);
			if (!cfg->base1) {
				pr_err("[MTK_BW]Can't find EMI device base!!\n");
				return -ENXIO;
			}
			break;
		case EMI_2:
			cfg->base2 = find_device_addr(EMI_CEN_DRV, EMI_2);
			if (!cfg->base2) {
				pr_err("[MTK_BW]Can't find EMI device base!!\n");
				return -ENXIO;
			}
			break;
		default:
			return -ENXIO;
		}
	}

	return 0;
}

static void emi_release_base(struct bw_cfg *cfg)
{
	int idx;

	/* get emi base */
	for (idx = 0; idx < emi_cnt; idx++) {
		switch (idx) {
		case EMI_0:
			if (cfg->base)
				iounmap(cfg->base);
			break;
		case EMI_1:
			if (cfg->base1)
				iounmap(cfg->base1);
			break;
		case EMI_2:
			if (cfg->base2)
				iounmap(cfg->base2);
			break;
		default:
			return;
		}
	}
}

static void __iomem *emi_get_base(struct bw_cfg *cfg, unsigned int emi_idx)
{
	switch (emi_idx) {
	case EMI_0:
		return cfg->base;
	case EMI_1:
		return cfg->base1;
	case EMI_2:
		return cfg->base2;
	default:
		return NULL;
	}
}

static int enable_broadcast_mode(bool enable)
{
	void __iomem *base;

	base = find_device_addr(EMI_DISPATCH_DRV, 0);
	if (!base) {
		pr_err("[MTK_BW]Can't find EMI_DISPATCH device base!!\n");
		return -1;
	}

	if (enable) {
		switch (emi_cnt) {
		case DISPATCH_1EMI_2CH:
			WRITE_REG(DISP_1_EMI_BORC_MODE0, BROC_MODE0);
			WRITE_REG(DISP_1_EMI_BORC_MODE1, BROC_MODE1);
			break;

		case DISPATCH_2EMI_4CH:
			WRITE_REG(DISP_2_EMI_BORC_MODE0, BROC_MODE0);
			WRITE_REG(DISP_2_EMI_BORC_MODE1, BROC_MODE1);
			break;

		case DISPATCH_3EMI_6CH:
			WRITE_REG(DISP_3_EMI_BORC_MODE0, BROC_MODE0);
			WRITE_REG(DISP_3_EMI_BORC_MODE1, BROC_MODE1);
			break;

		default:
			pr_err("[MTK_BW]wrong Dispatch Rule\n");
			return -1;
		}
	} else {
		WRITE_REG(0x0000, BROC_MODE0);
		WRITE_REG(0x0000, BROC_MODE1);
	}

	wmb(); /* make sure settings are written */

	if (base)
		iounmap(base);

	return 0;
}

static void get_larb_setting(struct bw_cfg *cfg, struct larb_res *res)
{
	u32 i;
	u32 port_num;
	u32 *plist;
	u32 pid;
	void __iomem *base;

	base = cfg->base;
	port_num = mlist[larb_mon_cfg.m_id].plist[res->id].pcount;
	plist = mlist[larb_mon_cfg.m_id].plist[res->id].smi_plist;

	for (i = 0; i < port_num; i++) {
		pid = plist[i];
		res->ostdl[i] = READ_REG(SMI_LARB_OSTDL_PORTx(pid)) & 0xff;
		res->ultra[i] = READ_REG(SMI_LARB_FORCE_ULTRA) & BIT(pid);
		res->pultra[i] = READ_REG(SMI_LARB_FORCE_PREULTRA) & BIT(pid);
	}
}

static void get_effi_result(void __iomem *base,
			    unsigned int type,
			    void *res)
{
	u32 out_sel;
	struct bw_res *module_res;
	struct effi_res *effi_res;

	/* output ultra cnt */
	out_sel = (2 << REG_MONITOR_OUT_SEL);
	WRITE_REG((READ_REG(REG_MONITOR_OP)|out_sel), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	if (type == SUMMARY_LARB_MODE || type == SUMMARY_COMMON_MODE || type == EMI_LARB_MODE) {
		module_res = (struct bw_res *)res;
		module_res->cnt_ultra = READ_REG(REG_MONITOR_OUT_0);
	} else if (type == SMI_COMMON_MODE || type == SMI_LARB_MODE) {
		effi_res = (struct effi_res *)res;
		effi_res->cnt_ultra = READ_REG(REG_MONITOR_OUT_0);
	} else {
		pr_err("[MTK_BW]get effi result type is wrong!!\n");
	}

	/* clear select effi output field */
	WRITE_REG((READ_REG(REG_MONITOR_OP) & 0xffffff3f), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	/* output avg_burst */
	out_sel = (0 << REG_MONITOR_OUT_SEL);
	WRITE_REG((READ_REG(REG_MONITOR_OP)|out_sel), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	if (type == SMI_COMMON_MODE || type == SMI_LARB_MODE)
		effi_res->avg_burst = READ_REG(REG_MONITOR_OUT_0);

	/* clear select effi output field */
	WRITE_REG((READ_REG(REG_MONITOR_OP) & 0xffffff3f), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	/* output pre_ultra cnt */
	out_sel = (3 << REG_MONITOR_OUT_SEL);
	WRITE_REG((READ_REG(REG_MONITOR_OP)|out_sel), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	if (type == SUMMARY_LARB_MODE || type == SUMMARY_COMMON_MODE || type == EMI_LARB_MODE) {
		module_res = (struct bw_res *)res;
		module_res->cnt_pultra = READ_REG(REG_MONITOR_OUT_0);
	} else if (type == SMI_COMMON_MODE || type == SMI_LARB_MODE) {
		effi_res = (struct effi_res *)res;
		effi_res->cnt_pultra = READ_REG(REG_MONITOR_OUT_0);
	} else {
		pr_err("[MTK_BW]get effi result type is wrong!!\n");
	}

	/* clear select effi output field */
	WRITE_REG((READ_REG(REG_MONITOR_OP) & 0xffffff3f), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

}

static int emi_effi_mon_set(unsigned int emi_port,
			    unsigned int ip_id,
			    unsigned int ip_id_en)
{
	int ret = 0;
	u32 out_sel;
	void __iomem *base;

	if (!emi_effi_mon)
		return -ENOENT;

	base = find_device_addr(EFFI_MONITOR_DRV, emi_port);
	if (!base) {
		pr_err("[MTK_BW]Can't find effi_monitor device base!!\n");
		return -ENOENT;
	}

	/* reset REG_MONITOR_OP */
	/* clear result */
	WRITE_REG(1, REG_MONITOR_OP);
	/* make sure settings are written */
	wmb();
	WRITE_REG(0, REG_MONITOR_OP);

	WRITE_REG(ip_id, REG_MONITOR_ID);
	WRITE_REG(ip_id_en, REG_MONITOR_ID_EN);

	/* set stop when reach count */
	__set_bit(REG_CMD_BOUND_STOP, base + REG_MONITOR_OP);

	/* clear select effi output field */
	WRITE_REG((READ_REG(REG_MONITOR_OP) & REG_MONITOR_CLN_OUT_SLE), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	/* select effi output */
	/* 7:6	reg_monitor_out_sel
	 * 0:avg_burst_cnt,
	 * 1:avg_effi
	 * 2:ultra_cnt
	 * 3:preultra_cnt
	 */
	out_sel = (1 << REG_MONITOR_OUT_SEL);

	WRITE_REG((READ_REG(REG_MONITOR_OP)|out_sel), REG_MONITOR_OP);

	if (base)
		iounmap(base);

	return ret;
}

static int emi_effi_mon_start(unsigned int emi_port)
{
	void __iomem *base;

	if (!emi_effi_mon)
		return -ENOENT;

	base = find_device_addr(EFFI_MONITOR_DRV, emi_port);
	if (!base) {
		pr_err("[MTK_BW]Can't find effi_monitor device base!!\n");
		return -ENOENT;
	}

	/* start to monitor */
	WRITE_REG((READ_REG(REG_MONITOR_OP)|BIT(REG_RPT_ON)), REG_MONITOR_OP);

	if (base)
		iounmap(base);

	return 0;
}

static int emi_effi_mon_get_by_port(unsigned int emi_port,
				    unsigned int *effi_output,
				    unsigned int *burst_output)
{
	void __iomem *base;
	u32 mon_done_timeout = 0;
	u32 mon_done_timeout_max = 0;

	if (!emi_effi_mon) {
		*effi_output = 0;
		return -ENOENT;
	}

	base = find_device_addr(EFFI_MONITOR_DRV, emi_port);
	if (!base) {
		pr_err("[MTK_BW]Can't find effi_monitor device base!!\n");
		*effi_output = 0;
		return -ENOENT;
	}

	if (emi_clock == MT5879_SMI_CLK || emi_clock == MT5876_SMI_CLK)
		mon_done_timeout_max = EFFI_MON_DELAY_PATCH_CNT;
	else
		mon_done_timeout_max = EFFI_MON_DELAY_CNT;

	/* wait 50 times */
	while (!(READ_REG(REG_MONITOR_HIT_BOUND) & 0x1)) {
		if (++mon_done_timeout > mon_done_timeout_max) {
			BW_DBG("[MTK-BW] EFFI [P%u] time out\n", emi_port);
			break;
		}
		/* wait 1 ms */
		usleep_range((EFFI_MON_DELAY_TIME_MIN), (EFFI_MON_DELAY_TIME_MAX));
	}

	/* output effi ratio */
	*effi_output = READ_REG(REG_MONITOR_OUT_0);

	/* select effi output */
	/* 7:6	reg_monitor_out_sel
	 * 0:avg_burst_cnt,
	 */
	bw_setbit(REG_MONITOR_OUT_SEL, false, base + REG_MONITOR_OP);
	bw_setbit((REG_MONITOR_OUT_SEL + 1), false, base + REG_MONITOR_OP);
	/* make sure settings are written */
	wmb();
	*burst_output = READ_REG(REG_MONITOR_OUT_0);

	/* clear result */
	WRITE_REG(1, REG_MONITOR_OP);
	/* make sure settings are written */
	wmb();
	WRITE_REG(0, REG_MONITOR_OP);

	if (base)
		iounmap(base);

	return 0;
}

static void emi_effi_mon_get(struct emi_res *res)
{
	u32 i, j;
	u32 effi_res, burst_res;

	if (!emi_effi_mon)
		return;

	for (i = 0; i < emi_port_cnt; i++) {
		res[emi_cnt].m_effi[i] = 0;
		res[emi_cnt].burst[i] = 0;
		for (j = 0; j < emi_cnt; j++) {
			if (emi_effi_mon_get_by_port(i, &effi_res, &burst_res) < 0)
				continue;
			res[j].m_effi[i] = effi_res;
			res[j].burst[i] = burst_res;
			res[emi_cnt].m_effi[i] += effi_res;
			res[emi_cnt].burst[i] += burst_res;
		}
		res[emi_cnt].m_effi[i] /= emi_cnt;
		res[emi_cnt].burst[i] /= emi_cnt;
	}
}

static u32 get_effi_monitor_id(u32 scomm_id, u32 scomm_out_id)
{
	int i;

	if (emi_effi_mon) {
		for (i = 0; i < EMI_EFFI_MON_CNT; i++) {
			if (emi_effi_device_map[i].mon_type == SMI_INPUT &&
			    scomm_id == emi_effi_device_map[i].common_id &&
			    scomm_out_id == emi_effi_device_map[i].out_port_id) {

				return emi_effi_device_map[i].effi_mon_id;
			}
		}
	} else {
		for (i = 0; i < EFFI_MON_CNT; i++) {
			if (scomm_id == effi_device_map[i].common_id &&
			    scomm_out_id == effi_device_map[i].out_port_id) {

				return effi_device_map[i].effi_mon_id;
			}
		}
	}

	return NULL_EFFI_ID;
}

static u32 get_effi_engine(u32 lc_id,
			   u32 port_id,
			   u32 type,
			   u32 *scomm_in_id,
			   u32 *larb_port_id)
{
	struct device_node *larb_np, *scomm_np;
	void __iomem *base;
	char dev_name[15];
	int ret_chk;
	u32 larb_id;
	u32 scomm_id;
	u32 scomm_out_id;
	u32 effi_device_id = NULL_EFFI_ID;
	u32 bus_sel;
	u32 ret = 0;

	if (type == SUMMARY_COMMON_MODE || type == SMI_COMMON_MODE) {
		scomm_id = lc_id;
		*scomm_in_id = port_id;
		base = find_device_addr(SMI_COMMON_DRV, scomm_id);
		if (!base) {
			pr_err("[MTK_BW]Can't find SMI COMMON device base!!\n");
			ret = effi_device_id;
			goto RET_FUNC;
		}
	} else if (type == SUMMARY_LARB_MODE || type == SMI_LARB_MODE) {
		larb_id = lc_id;
		*larb_port_id = port_id;
		base = find_device_addr(SMI_LARB_DRV, larb_id);
		if (!base) {
			pr_err("[MTK_BW]Can't find SMI LARB device base!!\n");
			ret = effi_device_id;
			goto RET_FUNC;
		}

		/* find larb node */
		ret_chk = snprintf(dev_name, MAX_DEV_NAME_LEN, "larb%d", larb_id);
		if (ret_chk >= MAX_DEV_NAME_LEN) {
			ret = effi_device_id;
			goto RET_FUNC;
		}

		larb_np = of_find_node_by_name(NULL, dev_name);
		if (!larb_np) {
			ret = effi_device_id;
			goto RET_FUNC;
		}

		/* find smi common input port id by larb node */
		ret_chk = of_property_read_u32(larb_np,
				"mediatek,smi-port-id", scomm_in_id);
		if (ret_chk) {
			ret = effi_device_id;
			goto RET_FUNC;
		}

		/* find smi common node by larb node*/
		scomm_np = of_parse_phandle(larb_np, "mediatek,smi", 0);
		if (!scomm_np) {
			ret = effi_device_id;
			goto RET_FUNC;
		}

		/* find smi common id */
		ret_chk = of_property_read_u32(scomm_np,
				"mediatek,common-id", &scomm_id);
		if (ret_chk) {
			ret = effi_device_id;
			goto RET_FUNC;
		}
		/* get smi common reg base */
		if (base)
			iounmap(base);
		base = of_iomap(scomm_np, 0);
		if (!base) {
			pr_err("[MTK_BW]%pOF: of_iomap failed\n", scomm_np);
			ret = effi_device_id;
			goto RET_FUNC;
		}
	} else if (type == EMI_LARB_MODE) {
		return port_id;
	} else {
		return NULL_EFFI_ID;
	}

	bus_sel = READ_REG(SMI_BUS_SEL);

	if (bus_sel & (1 << (*scomm_in_id * 2)))
		scomm_out_id = P1;
	else
		scomm_out_id = P0;

	ret = get_effi_monitor_id(scomm_id, scomm_out_id);

RET_FUNC:
	if (base)
		iounmap(base);
	return ret;
}

static u32 effi_measure(u32 lc_id,
			u32 port_id,
			u32 type,
			void *res)
{
	void __iomem *base;
	u32 mon_done_timeout = 0;
	u32 mon_done_timeout_max = 0;
	u32 effi_output = 0;
	u32 out_sel;
	u32 scomm_in_id; //need
	u32 larb_port_id; //need
	u32 effi_device_id = NULL_EFFI_ID;
	u32 ip_effi_id;

	effi_device_id = get_effi_engine(lc_id, port_id, type, &scomm_in_id, &larb_port_id);
	if (effi_device_id == NULL_EFFI_ID)
		return 0;

	base = find_device_addr(EFFI_MONITOR_DRV, effi_device_id);
	if (!base) {
		pr_err("[MTK_BW]Can't find effi_monitor device base!!\n");
		return 0;
	}

	/* make sure settings are written */
	wmb();

	/* reset REG_MONITOR_OP */
	WRITE_REG(1, REG_MONITOR_OP);
	WRITE_REG(0, REG_MONITOR_OP);

	if (type == SUMMARY_COMMON_MODE || type == SMI_COMMON_MODE) {
		/* for common effi, need to set [10:8] to 1 */
		WRITE_REG(EN_EFFI_SCOMM_MONITOR, REG_MONITOR_ID_EN);

		/* [10:8]: common input id */
		ip_effi_id = (scomm_in_id << REG_SCOMM_INPUT);
		WRITE_REG(ip_effi_id, REG_MONITOR_ID);
	} else if (type == SUMMARY_LARB_MODE || type == SMI_LARB_MODE) {
		/* for larb effi, need to set all en to 1 */
		WRITE_REG(EN_EFFI_LARB_MONITOR, REG_MONITOR_ID_EN);

		/* [10:8]: common input id */
		/* [4:0]: larb port id */
		ip_effi_id = ((scomm_in_id << REG_SCOMM_INPUT) | larb_port_id);
		WRITE_REG(ip_effi_id, REG_MONITOR_ID);
	} else if (type == EMI_LARB_MODE) {
		WRITE_REG(0, REG_MONITOR_ID);
		WRITE_REG(0, REG_MONITOR_ID_EN);
	}

	/* set stop when reach count */
	__set_bit(REG_CMD_BOUND_STOP, base + REG_MONITOR_OP);

	/* clear select effi output field */
	WRITE_REG((READ_REG(REG_MONITOR_OP) & 0xffffff3f), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	/* select effi output */
	/* 7:6	reg_monitor_out_sel
	 * 0:avg_burst_cnt,
	 * 1:avg_effi
	 * 2:ultra_cnt
	 * 3:preultra_cnt
	 */
	out_sel = (1 << REG_MONITOR_OUT_SEL);

	WRITE_REG((READ_REG(REG_MONITOR_OP)|out_sel), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	/* start to monitor */
	WRITE_REG((READ_REG(REG_MONITOR_OP)|BIT(REG_RPT_ON)), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	if (emi_clock == MT5879_SMI_CLK || emi_clock == MT5876_SMI_CLK)
		mon_done_timeout_max = EFFI_MON_DELAY_PATCH_CNT;
	else
		mon_done_timeout_max = EFFI_MON_DELAY_CNT;

	/* wait 50 times */
	while (!(READ_REG(REG_MONITOR_HIT_BOUND) & 0x1)) {
		if (++mon_done_timeout > mon_done_timeout_max) {
			BW_DBG("[MTK-BW] EFFI [P%u] time out\n", effi_device_id);
			break;
		}
		/* wait 1 ms */
		usleep_range((EFFI_MON_DELAY_TIME_MIN), (EFFI_MON_DELAY_TIME_MAX));
	}

	/* output effi ratio */
	effi_output = READ_REG(REG_MONITOR_OUT_0);

	/* clear select effi output field */
	WRITE_REG((READ_REG(REG_MONITOR_OP) & 0xffffff3f), REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	get_effi_result(base, type, res);

	/* monitor to 0*/
	__clear_bit(REG_RPT_ON, base + REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	/* clear result */
	__set_bit(REG_MONITOR_SW_RST, base + REG_MONITOR_OP);

	/* make sure settings are written */
	wmb();

	__clear_bit(REG_MONITOR_SW_RST, base + REG_MONITOR_OP);

	if (base)
		iounmap(base);

	return effi_output;
}

static void larb_bw_measure(struct bw_cfg *cfg, struct larb_res *res)
{
	u32 i;
	u32 sel;
	u32 con;
	u32 port_num;
	u32 *plist;
	u32 larb_id;
	u32 m_cp_t_time;
	u32 a_cp_t_time;
	u32 dp_time;
	u32 cmd_cnt;
	void __iomem *base;

	base = cfg->base;
	port_num = mlist[larb_mon_cfg.m_id].plist[res->id].pcount;
	plist = mlist[larb_mon_cfg.m_id].plist[res->id].smi_plist;
	larb_id = mlist[larb_mon_cfg.m_id].llist[res->id];

	for (i = 0; i < port_num; i++) {
		res->effi[i] = 0;
		res->r_ultra[i] = 0;
		res->r_pultra[i] = 0;
		res->burst[i] = 0;
	}

	/* read */
	for (i = 0; i < port_num; i++) {

		struct effi_res e_res;

		e_res = (struct effi_res) {
			.cnt_ultra = 0,
			.cnt_pultra = 0,
			.avg_burst = 0,
		};

		/* clear monitor setting register */
		WRITE_REG(1, SMI_LARB_MON_CLR);

		/* setup monitor setting */
		sel = (plist[i] << PORT_SEL0);
		WRITE_REG(sel, SMI_LARB_MON_PORT);

		/* setup dp mode 1 */
		/* setup monitor read */
		con = (1 << RW_SEL) | (1 << DP_MODE);
		WRITE_REG(con, SMI_LARB_MON_CON);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_LARB_MON_EN);
		usleep_range(cfg->t_ms*USEC_PER_MSEC, cfg->t_ms*USEC_PER_MSEC);

		/* stop monitor */
		WRITE_REG(0, SMI_LARB_MON_EN);

		/* get bw result */
		res->r_bw[i] = READ_REG(SMI_LARB_MON_BYTE_CNT)/cfg->t_ms;

		/* change from Byte/ms to MB/s */
		res->r_bw[i] = B_MS_TO_MB_S(res->r_bw[i]);

		if (res->r_bw[i]) {
			res->effi[i] = effi_measure(larb_id, plist[i],
					SMI_LARB_MODE, &e_res);
			res->r_ultra[i] = PERCENTAGE(e_res.cnt_ultra, CMD_NUM);
			res->r_pultra[i] = PERCENTAGE(e_res.cnt_pultra, CMD_NUM);
			res->burst[i] = e_res.avg_burst;
			res->larb_bus_bw += CAL_BUS_BW(res->r_bw[i], res->effi[i]);
			res->larb_data_bw += res->r_bw[i];

			BW_DBG("[MTK-BW] SMI Larb: %u port: %d type: R\033[K\n",
								larb_id,
								plist[i]);
			BW_DBG("[MTK-BW] r_bw: %llu, effi: %u, ", res->r_bw[i], res->effi[i]);
			BW_DBG("[MTK-BW] cnt_ultra: %llu, cnt_pultra: %llu CMD_NUM: %d\033[K\n",
								e_res.cnt_ultra,
								e_res.cnt_pultra,
								CMD_NUM);
			BW_DBG("[MTK-BW] r_ultra: %u, r_pultra: %u, burst: %u\033[K\n",
								res->r_ultra[i],
								res->r_pultra[i],
								res->burst[i]);
			BW_DBG("[MTK-BW] larb_data_bw: %llu, larb_bus_bw: %llu \033[K\n",
								res->larb_data_bw,
								res->larb_bus_bw);
			BW_DBG("\033[K\n");
		}

		/* get dp result */
		cmd_cnt = READ_REG(SMI_LARB_MON_REQ_CNT);
		dp_time = READ_REG(SMI_LARB_MON_DP_CNT);
		res->r_avg_dp[i] = NSEC_PER_USEC * dp_time / smi_clock / cmd_cnt;
	}

	/* write */
	for (i = 0; i < port_num; i++) {

		struct effi_res e_res;

		e_res = (struct effi_res) {
			.cnt_ultra = 0,
			.cnt_pultra = 0,
			.avg_burst = 0,
		};

		/* clear monitor setting register */
		WRITE_REG(1, SMI_LARB_MON_CLR);

		/* setup monitor setting */
		sel = (plist[i] << PORT_SEL0);
		WRITE_REG(sel, SMI_LARB_MON_PORT);

		/* setup dp mode 1 */
		/* setup monitor read */
		con = (2 << RW_SEL) | (1 << DP_MODE);
		WRITE_REG(con, SMI_LARB_MON_CON);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_LARB_MON_EN);

		usleep_range((cfg->t_ms*USEC_PER_MSEC), (cfg->t_ms*USEC_PER_MSEC));

		/* stop monitor */
		WRITE_REG(0, SMI_LARB_MON_EN);

		/* get bw result */
		res->w_bw[i] = READ_REG(SMI_LARB_MON_BYTE_CNT)/cfg->t_ms;

		/* change from Byte/ms to MB/s */
		res->w_bw[i] = B_MS_TO_MB_S(res->w_bw[i]);

		/* get dp result */
		cmd_cnt = READ_REG(SMI_LARB_MON_REQ_CNT);
		dp_time = READ_REG(SMI_LARB_MON_DP_CNT);
		res->w_avg_dp[i] = NSEC_PER_USEC * dp_time / smi_clock / cmd_cnt;

		if (res->w_bw[i]) {
			res->effi[i] = effi_measure(larb_id, plist[i],
					SMI_LARB_MODE, &e_res);
			res->r_ultra[i] = PERCENTAGE(e_res.cnt_ultra, CMD_NUM);
			res->r_pultra[i] = PERCENTAGE(e_res.cnt_pultra, CMD_NUM);
			res->burst[i] = e_res.avg_burst;
			res->larb_bus_bw += CAL_BUS_BW(res->w_bw[i], res->effi[i]);
			res->larb_data_bw += res->w_bw[i];

			BW_DBG("[MTK-BW] SMI Larb: %u port: %d type: W\033[K\n",
								larb_id,
								plist[i]);
			BW_DBG("[MTK-BW] w_bw: %llu, effi: %u, ", res->w_bw[i], res->effi[i]);
			BW_DBG("[MTK-BW] cnt_ultra: %llu, cnt_pultra: %llu CMD_NUM: %d\033[K\n",
								e_res.cnt_ultra,
								e_res.cnt_pultra,
								CMD_NUM);
			BW_DBG("[MTK-BW] r_ultra: %u, r_pultra: %u, burst: %u\033[K\n",
								res->r_ultra[i],
								res->r_pultra[i],
								res->burst[i]);
			BW_DBG("[MTK-BW] larb_data_bw: %llu, larb_bus_bw: %llu \033[K\n",
								res->larb_data_bw,
								res->larb_bus_bw);
			BW_DBG("\033[K\n");
		}
	}

	/* all */
	/* LARB do not support CP_CNT and CP MAX when read/write */
	/* individual measurement. So we can only monitor one more time */
	/* to record CP_CNT and CP_MAX */
	for (i = 0; i < port_num; i++) {

		/* clear monitor setting register */
		WRITE_REG(1, SMI_LARB_MON_CLR);

		/* setup monitor setting */
		sel = (plist[i] << PORT_SEL0);
		WRITE_REG(sel, SMI_LARB_MON_PORT);

		/* setup dp mode 1 */
		/* setup monitor read */
		con = (0 << RW_SEL) | (1 << DP_MODE);
		WRITE_REG(con, SMI_LARB_MON_CON);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_LARB_MON_EN);

		usleep_range((cfg->t_ms*USEC_PER_MSEC), (cfg->t_ms*USEC_PER_MSEC));

		/* stop monitor */
		WRITE_REG(0, SMI_LARB_MON_EN);

		/* get cp result */
		cmd_cnt = READ_REG(SMI_LARB_MON_REQ_CNT);
		m_cp_t_time = READ_REG(SMI_LARB_MON_CP_MAX);
		a_cp_t_time = READ_REG(SMI_LARB_MON_CP_CNT);
		res->max_cp[i] = NSEC_PER_USEC * m_cp_t_time / smi_clock;
		res->avg_cp[i] = NSEC_PER_USEC * a_cp_t_time / smi_clock / cmd_cnt;


	}

}

static bool is_smi_common_exist(int id)
{
	struct device_node *np;
	char dev_name[MAX_DEV_NAME_LEN];
	int num;

	num = snprintf(dev_name, MAX_DEV_NAME_LEN, "smi_common%d", id);
	if (num >= MAX_DEV_NAME_LEN)
		return false;
	np = of_find_node_by_name(NULL, dev_name);
	if (!np || !of_device_is_available(np))
		return false;

	return true;
}

static int get_smi_common_port_info(int *input, int *output, int id)
{
	struct device_node *np;
	char dev_name[15];
	int num;

	num = snprintf(dev_name, MAX_DEV_NAME_LEN, "smi_common%d", id);
	if (num >= MAX_DEV_NAME_LEN)
		return -1;

	np = of_find_node_by_name(NULL, dev_name);
	if (!np)
		return -1;

	if (of_property_read_u32(np, "input-port-nr", input)) {
		pr_err("[MTK_BW]can't get property: input-port-nr\n");
		return -1;
	}

	if (of_property_read_u32(np, "output-port-nr", output)) {
		pr_err("[MTK_BW]can't get property: output-port-nr\n");
		return -1;
	}

	return 0;
}

static void comm_bw_measure(struct bw_cfg *cfg, struct bw_res *res)
{
	int i;
	int err;
	u32 sel;
	u32 id;
	u32 rw_type;
	int ip_num;
	int op_num = 0;
	void __iomem *base;
	struct common_config m_common;
	u64 temp_cport_data_bw;
	u32 *p_effi;
	u32 idx;

	p_effi = res->p_effi;
	base = cfg->base;
	m_common = mlist[cfg->m_id].cplist[cfg->c_idx];

	err = get_smi_common_port_info(&ip_num, &op_num,
			mlist[cfg->m_id].clist[cfg->c_idx]);
	if (err)
		pr_err("[MTK_BW]Can't get smi common port info!!\n");

	/* read */
	for (i = 0; i < m_common.pcount; i++) {
		idx = cfg->c_idx + i;

		/* smi common measure start with output port */
		id = m_common.com_plist[i] + op_num;

		/* clear monitor setting register */
		WRITE_REG(1, SMI_MON_AXI_CLR);

		/* setup monitor setting */
		sel = (id << (AXI_BUS_SELECT)) | (1 << DP_SEL);
		WRITE_REG(sel, SMI_MON_AXI_CON);

		/* setop monitor read */
		rw_type = 0 << MON_TYPE_RW_SEL;
		WRITE_REG(rw_type, SMI_MON_AXI_TYPE);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_MON_AXI_ENA);

		usleep_range(cfg->t_ms*USEC_PER_MSEC, cfg->t_ms*USEC_PER_MSEC);

		/* stop monitor */
		WRITE_REG(0, SMI_MON_AXI_ENA);

		/* get read bw result */
		temp_cport_data_bw = READ_REG(SMI_MON_AXI_BYT_CNT)/cfg->t_ms;

		/* effi measure */
		if (temp_cport_data_bw) {
			res->p_effi[idx] = effi_measure(mlist[cfg->m_id].clist[cfg->c_idx],
							m_common.com_plist[i],
							SUMMARY_COMMON_MODE,
							res);

			/* Module ultra rate = */
			/* sum(data_bw[i]*ultra_cnt[i])/sum(data_bw)/CMD_CNT */
			/* we must have sum(data_bw) to calaulate */
			/* but we can't know in this time */
			/* so we save the data_bw[i]* ultra_cnt[i] first */
			res->tmp_ultra += temp_cport_data_bw * res->cnt_ultra;
			res->tmp_pultra += temp_cport_data_bw * res->cnt_pultra;


			res->bus_bw += CAL_BUS_BW(temp_cport_data_bw, res->p_effi[idx]);
		}

		/* Bytes/ms */
		res->data_bw += temp_cport_data_bw;
	}

	/* write */
	for (i = 0; i < m_common.pcount; i++) {

		/* smi common measure start with output port */
		id = m_common.com_plist[i] + op_num;

		/* clear monitor setting register */
		WRITE_REG(1, SMI_MON_AXI_CLR);

		/* setup monitor setting */
		sel = (id << AXI_BUS_SELECT) | (1 << DP_SEL);
		WRITE_REG(sel, SMI_MON_AXI_CON);

		/* setop monitor write */
		rw_type = 1 << MON_TYPE_RW_SEL;
		WRITE_REG(rw_type, SMI_MON_AXI_TYPE);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_MON_AXI_ENA);

		usleep_range(cfg->t_ms*USEC_PER_MSEC, cfg->t_ms*USEC_PER_MSEC);

		/* stop monitor */
		WRITE_REG(0, SMI_MON_AXI_ENA);

		/* get write bw result */
		temp_cport_data_bw = READ_REG(SMI_MON_AXI_BYT_CNT)/cfg->t_ms;

		/* effi measure */
		if (temp_cport_data_bw) {
			res->p_effi[idx] = effi_measure(mlist[cfg->m_id].clist[cfg->c_idx],
							m_common.com_plist[i],
							SUMMARY_COMMON_MODE,
							res);

			/* Module ultra rate = */
			/* sum(data_bw[i]*ultra_cnt[i])/sum(data_bw)/CMD_CNT */
			/* we must have sum(data_bw) to calaulate */
			/* but we can't know in this time */
			/* so we save the data_bw[i]* ultra_cnt[i] first */
			res->tmp_ultra += temp_cport_data_bw * res->cnt_ultra;
			res->tmp_pultra += temp_cport_data_bw * res->cnt_pultra;


			res->bus_bw += CAL_BUS_BW(temp_cport_data_bw, res->p_effi[idx]);
		}

		/* Bytes/ms */
		res->data_bw += temp_cport_data_bw;
	}


}

static void comm_cp_dp_bw_measure(struct bw_cfg *cfg, struct scomm_res *res)
{
	int i;
	u32 sel;
	u32 port_num;
	u32 rw_type;
	u32 cp_t_time;
	u32 dp_time;
	u32 cp_time;
	u32 cmd_cnt;
	void __iomem *base;

	base = cfg->base;
	port_num = res->ip_num + res->op_num;

	for (i = 0; i < port_num; i++) {
		res->effi[i] = 0;
		res->r_ultra[i] = 0;
		res->r_pultra[i] = 0;
		res->burst[i] = 0;
	}

	/* read */
	for (i = 0; i < port_num; i++) {

		/* clear monitor setting register */
		WRITE_REG(1, SMI_MON_AXI_CLR);

		/* setup monitor setting */
		sel = (i << AXI_BUS_SELECT) | (1 << DP_SEL);
		WRITE_REG(sel, SMI_MON_AXI_CON);

		/* setop monitor read */
		rw_type = 0 << MON_TYPE_RW_SEL;
		WRITE_REG(rw_type, SMI_MON_AXI_TYPE);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_MON_AXI_ENA);

		usleep_range(cfg->t_ms*USEC_PER_MSEC, cfg->t_ms*USEC_PER_MSEC);

		/* stop monitor */
		WRITE_REG(0, SMI_MON_AXI_ENA);

		/* get bw result */
		res->r_bw[i] = READ_REG(SMI_MON_AXI_BYT_CNT)/cfg->t_ms;

		/* change from Byte/ms to MB/s */
		res->r_bw[i] = B_MS_TO_MB_S(res->r_bw[i]);

		/* get cp result */
		cp_t_time = READ_REG(SMI_MON_AXI_CP_MAX);
		res->r_max_cp[i] = NSEC_PER_USEC * cp_t_time / smi_clock;

		/* get dp result */
		cmd_cnt = READ_REG(SMI_MON_AXI_REQ_CNT);
		cp_time = READ_REG(SMI_MON_AXI_CP_CNT);
		dp_time = READ_REG(SMI_MON_AXI_DP_CNT);
		res->r_avg_cp[i] = NSEC_PER_USEC * cp_time / smi_clock / cmd_cnt;
		res->r_avg_dp[i] = NSEC_PER_USEC * dp_time / smi_clock / cmd_cnt;
	}

	/* write */
	for (i = 0; i < port_num; i++) {

		/* clear monitor setting register */
		WRITE_REG(1, SMI_MON_AXI_CLR);

		/* setup monitor setting */
		sel = (i << AXI_BUS_SELECT) | (1 << DP_SEL);
		WRITE_REG(sel, SMI_MON_AXI_CON);

		/* setop monitor write */
		rw_type = 1 << MON_TYPE_RW_SEL;
		WRITE_REG(rw_type, SMI_MON_AXI_TYPE);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_MON_AXI_ENA);

		usleep_range(cfg->t_ms*USEC_PER_MSEC, cfg->t_ms*USEC_PER_MSEC);

		/* stop monitor */
		WRITE_REG(0, SMI_MON_AXI_ENA);

		/* get bw result */
		res->w_bw[i] = READ_REG(SMI_MON_AXI_BYT_CNT)/cfg->t_ms;

		/* change from Byte/ms to MB/s */
		res->w_bw[i] = B_MS_TO_MB_S(res->w_bw[i]);

		/* get cp result */
		cp_t_time = READ_REG(SMI_MON_AXI_CP_MAX);
		res->w_max_cp[i] = NSEC_PER_USEC * cp_t_time / smi_clock;

		/* get dp result */
		cmd_cnt = READ_REG(SMI_MON_AXI_REQ_CNT);
		cp_time = READ_REG(SMI_MON_AXI_CP_CNT);
		dp_time = READ_REG(SMI_MON_AXI_DP_CNT);
		res->w_avg_cp[i] = NSEC_PER_USEC * cp_time / smi_clock / cmd_cnt;
		res->w_avg_dp[i] = NSEC_PER_USEC * dp_time / smi_clock / cmd_cnt;
	}

	/* effi monitor */
	for (i = res->op_num; i < port_num; i++) {
		if (res->r_bw[i] || res->w_bw[i]) {
			struct effi_res e_res;

			e_res = (struct effi_res) {
				.cnt_ultra = 0,
				.cnt_pultra = 0,
				.avg_burst = 0,
			};

			res->effi[i] = effi_measure(res->id,
						    i - res->op_num,
						    SMI_COMMON_MODE,
						    &e_res);
			res->r_ultra[i] = PERCENTAGE(e_res.cnt_ultra, CMD_NUM);
			res->r_pultra[i] = PERCENTAGE(e_res.cnt_pultra, CMD_NUM);
			res->burst[i] = e_res.avg_burst;

			BW_DBG("[MTK-BW] SMI Common: %u port: %d\033[K\n", res->id,
							      i - res->op_num);
			BW_DBG("[MTK-BW] r_bw: %llu, w_bw: %llu, effi: %u, ",
								res->r_bw[i],
								res->w_bw[i],
								res->effi[i]);
			BW_DBG("[MTK-BW] cnt_ultra: %llu, cnt_pultra: %llu CMD_NUM: %d\033[K\n",
								e_res.cnt_ultra,
								e_res.cnt_pultra,
								CMD_NUM);
			BW_DBG("[MTK-BW] r_ultra: %u, r_pultra: %u, burst: %u\033[K\n",
								res->r_ultra[i],
								res->r_pultra[i],
								res->burst[i]);
		}
	}
}

static void emi_bw_measure(struct bw_cfg *cfg, struct bw_res *res)
{
	u32 emi_cycle;
	u64 bw_dw = 0;
	void __iomem *emi_base[MAX_EMI_NR];
	void __iomem *base;
	u32 reg_temp;
	int i;

	/* emi 0 base */
	base = cfg->base;

	enable_broadcast_mode(true);
	/* Disable AFIFO CHN DCM to make EMI_BMEN really */
	/* clear wsct counter */
	__set_bit(AFIFO_CHN_DCM, base + EMI_CONM);

	/* clear monitor setting register */
	WRITE_REG(0, EMI_BMEN);
	WRITE_REG(0, EMI_MSEL);
	WRITE_REG(0, EMI_MSEL2);
	WRITE_REG(0, EMI_BMEN2);
	WRITE_REG(0, EMI_BMID0);
	WRITE_REG(0, EMI_DBWA);

	/* count emi cycle */
	emi_cycle = cfg->t_ms * emi_clock * MSEC_PER_SEC;

	/* setup monitor setting */
	WRITE_REG(emi_cycle, EMI_BSTP);
	__set_bit(DBW0_RCMD_GNT_SEL, base + EMI_DBWA);
	__set_bit(DBW0_WCMD_GNT_SEL, base + EMI_DBWA);

	/* setup monitor all priority transaction */
	__set_bit(DBW0_HPRI_DIS, base + EMI_DBWA);
	enable_broadcast_mode(false);
	reg_temp = READ_REG(EMI_DBWA_2ND);
	reg_temp |= (BIT(HPRI_SEL_HIGH)|
		     BIT(HPRI_SEL_FLUSH)|
		     BIT(HPRI_SEL_PREULTRA)|
		     BIT(HPRI_SEL_ULTRA));
	enable_broadcast_mode(true);
	WRITE_REG(reg_temp, EMI_DBWA_2ND);

	if (strncmp(cfg->name, "CPU", sizeof("CPU")) == 0) {
		__set_bit(DBW0_SEL_MASTER_M0, base + EMI_DBWA);
		__set_bit(DBW0_SEL_MASTER_M1, base + EMI_DBWA);

	} else if (strncmp(cfg->name, "GPU", sizeof("GPU")) == 0) {
		__set_bit(DBW0_SEL_MASTER_M7, base + EMI_DBWA);
	}

	/* trigger monitor */
	__set_bit(BUS_MON_EN, base + EMI_BMEN);

	wmb(); /* make sure settings are written */

	enable_broadcast_mode(false);

	/* wait hw bw measure done */
	do {
		if (READ_REG(EMI_BMEN) & BIT(BUS_MON_STP)) {
			if (strncmp(cfg->name, "REF_EMI", sizeof("REF_EMI")) == 0)
				for (i = 0; i < emi_cnt; i++) {
					emi_base[i] = emi_get_base(cfg, i);
					bw_dw += readl_relaxed(emi_base[i] + EMI_WACT);
				}
			else
				for (i = 0; i < emi_cnt; i++) {
					emi_base[i] = emi_get_base(cfg, i);
					bw_dw += readl_relaxed(emi_base[i] + EMI_WSCT);
				}

			/* Bytes/ms */
			res->data_bw = bw_div(EMI_BW_8BYTE(bw_dw), cfg->t_ms);

			break;
		}
	} while (1);

	enable_broadcast_mode(true);
	/* Enable AFIFO CHN DCM */
	__clear_bit(AFIFO_CHN_DCM, base + EMI_CONM);
	enable_broadcast_mode(false);
}

static void emi_bw_measure_init_monitor(void __iomem *base,
					unsigned int offset,
					unsigned int offset_2ND)
{
	u32 reg_temp;

	/* setup monitor setting */
	bw_setbit(DBW0_RCMD_GNT_SEL, true, base + offset);
	bw_setbit(DBW0_WCMD_GNT_SEL, true, base + offset);
	/* setup monitor all priority transaction */
	bw_setbit(DBW0_HPRI_DIS, true, base + offset);
	enable_broadcast_mode(false);
	reg_temp = READ_REG(offset_2ND);
	reg_temp |= (BIT(HPRI_SEL_HIGH)|
		     BIT(HPRI_SEL_FLUSH)|
		     BIT(HPRI_SEL_PREULTRA)|
		     BIT(HPRI_SEL_ULTRA));
	enable_broadcast_mode(true);
	WRITE_REG(reg_temp, offset_2ND);
	wmb(); /* make sure settings are written */
}

static void emi_bw_measure_init(struct bw_cfg *cfg)
{
	u32 emi_cycle;
	void __iomem *base;

	base = cfg->base;

	/* clear monitor setting register */
	WRITE_REG(0, EMI_BMEN);
	WRITE_REG(0, EMI_MSEL);
	WRITE_REG(0, EMI_MSEL2);
	WRITE_REG(0, EMI_BMEN2);
	WRITE_REG(0, EMI_BMID0);
	WRITE_REG(0, EMI_DBWA);
	WRITE_REG(0, EMI_DBWB);
	WRITE_REG(0, EMI_DBWC);
	WRITE_REG(0, EMI_DBWD);
	WRITE_REG(0, EMI_DBWE);
	WRITE_REG(0, EMI_DBWF);
	WRITE_REG(0, EMI_BMRW0);

	emi_cycle = cfg->t_ms * emi_clock * MSEC_PER_SEC;

	/* setup all monitor setting */
	WRITE_REG(emi_cycle, EMI_BSTP);
	emi_bw_measure_init_monitor(base, EMI_DBWA, EMI_DBWA_2ND);
	emi_bw_measure_init_monitor(base, EMI_DBWB, EMI_DBWB_2ND);
	emi_bw_measure_init_monitor(base, EMI_DBWC, EMI_DBWC_2ND);
	emi_bw_measure_init_monitor(base, EMI_DBWD, EMI_DBWD_2ND);
	emi_bw_measure_init_monitor(base, EMI_DBWE, EMI_DBWE_2ND);
	emi_bw_measure_init_monitor(base, EMI_DBWF, EMI_DBWF_2ND);

	/* monitor read and write transaction for latency */
	WRITE_REG(LOW_32BIT, EMI_BMRW0);
	/* enable latency monitor */
	bw_setbit(BMEN_LAT_CNT_SEL, true, base + EMI_BMEN2);
}

static void emi_bw_measure_exit(struct bw_cfg *cfg)
{
	void __iomem *base;

	base = cfg->base;
	/* clear monitor setting register */
	WRITE_REG(0, EMI_BMEN);
	WRITE_REG(0, EMI_MSEL);
	WRITE_REG(0, EMI_MSEL2);
	WRITE_REG(0, EMI_BMEN2);
	WRITE_REG(0, EMI_BMID0);
	WRITE_REG(0, EMI_DBWA);
	WRITE_REG(0, EMI_DBWB);
	WRITE_REG(0, EMI_DBWC);
	WRITE_REG(0, EMI_DBWD);
	WRITE_REG(0, EMI_DBWE);
	WRITE_REG(0, EMI_DBWF);
	WRITE_REG(0, EMI_BMRW0);

	/* monitor read and write transaction for latency */
	WRITE_REG(0x0, EMI_BMRW0);
	/* enable latency monitor */
	bw_setbit(BMEN_LAT_CNT_SEL, false, base + EMI_BMEN2);
}

static unsigned int emi_get_bw_mon_offset(struct bw_cfg *cfg,
					  unsigned int emi_mon)
{
	switch (emi_mon) {
	case EMI_DBW0:
		return EMI_WSCT;
	case EMI_DBW1:
		return EMI_WSCT2;
	case EMI_DBW2:
		return EMI_WSCT3;
	case EMI_DBW3:
		return EMI_WSCT4;
	case EMI_DBW4:
		return EMI_DBWG;
	case EMI_DBW5:
		return EMI_DBWH;
	default:
		return 0;
	}
}

static void emi_set_dbw_monitor(void __iomem *base,
				unsigned int mon_id,
				unsigned int mon_port)
{
	u32 offset;

	switch (mon_id) {
	case EMI_DBW0:
		offset = EMI_DBWA;
		break;
	case EMI_DBW1:
		offset = EMI_DBWB;
		break;
	case EMI_DBW2:
		offset = EMI_DBWC;
		break;
	case EMI_DBW3:
		offset = EMI_DBWD;
		break;
	case EMI_DBW4:
		offset = EMI_DBWE;
		break;
	case EMI_DBW5:
		offset = EMI_DBWF;
		break;
	default:
		return;
	}
	bw_setbit((DBW0_SEL_MASTER_M0 + mon_port), true, (base + offset));
}

static void emi_detail_bw_measure(struct bw_cfg *cfg, struct emi_res *res)
{
	u32 i, j, k;
	u32 monitor_round, monitor_count;
	u32 st_port;
	u32 offset;
	u64 data_bw_t, latency_t;
	void __iomem *base;

	for (i = 0; i < emi_port_cnt; i++)
		emi_effi_mon_set(i, 0, 0);

	/* enable broadcast mode to setup all EMI monitor */
	enable_broadcast_mode(true);
	/* Disable AFIFO CHN DCM to make EMI_BMEN really */
	/* clear wsct counter */
	bw_setbit(AFIFO_CHN_DCM, true, cfg->base + EMI_CONM);

	monitor_round = GET_LOOP_ROUND(emi_port_cnt, EMI_DBW_MON_CNT);

	for (i = 0; i < monitor_round; i++) {
		/* enable broadcast mode to setup all EMI monitor */
		enable_broadcast_mode(true);
		st_port = i * EMI_DBW_MON_CNT;
		monitor_count = (i == (emi_port_cnt / EMI_DBW_MON_CNT)) ?
				(emi_port_cnt % EMI_DBW_MON_CNT) : EMI_DBW_MON_CNT;

		/* init emi bw measure setting */
		emi_bw_measure_init(cfg);

		/* set BDW for bw monitor */
		/* base, monitor index, emi port */
		for (j = 0; j < monitor_count; j++)
			emi_set_dbw_monitor(cfg->base, j, (j + st_port));

		/* trigger monitor */
		bw_setbit(BUS_MON_EN, true, cfg->base + EMI_BMEN);

		/* trigger effi monitor */
		for (j = 0; j < emi_port_cnt; j++)
			emi_effi_mon_start(j);

		wmb(); /* make sure settings are written */
		enable_broadcast_mode(false);

		base = cfg->base;
		/* wait hw bw measure done */
		do {
			if (!(READ_REG(EMI_BMEN) & BIT(BUS_MON_STP)))
				continue;
			for (j = 0; j < monitor_count; j++) {
				offset = emi_get_bw_mon_offset(cfg, j);
				if (offset == 0)
					goto RET_FUN;
				data_bw_t = 0;
				for (k = 0; k < emi_cnt; k++) {
					base = emi_get_base(cfg, k);
					if (!base)
						goto RET_FUN;
					res[k].data_bw[(j + st_port)] =
					B_MS_TO_MB_S(bw_div(EMI_BW_8BYTE(READ_REG(offset)),
							    cfg->t_ms));

					data_bw_t += res[k].data_bw[(j + st_port)];
				}
				res[emi_cnt].data_bw[(j + st_port)] = data_bw_t;
			}
			break;
		} while (1);
	}

	emi_effi_mon_get(res);

	enable_broadcast_mode(false);
	/* latency and emi information */
	for (i = 0; i < emi_port_cnt; i++) {
		latency_t = 0;
		for (j = 0; j < emi_cnt; j++) {
			base = emi_get_base(cfg, j);
			if (!base)
				goto RET_FUN;
			res[j].latency[i] = READ_REG(EMI_TTYPE_LAT(i)) /
					    READ_REG(EMI_TTYPE_TRANS(i));
			/* output unit: nsec */
			res[j].latency[i] =
			PERMILLAGE_ALIGN(bw_div((res[j].latency[i] * PSEC_PER_USEC), emi_clock));
			latency_t += res[j].latency[i];

			/* get emi information */
			res[j].arb_info[i] = READ_REG(EMI_ARB(i));
		}
		res[emi_cnt].latency[i] = bw_div(latency_t, emi_cnt);
	}

	/* outstanding control */
	for (i = 0; i < emi_port_cnt; i++) {
		for (j = 0; j < emi_cnt; j++) {
			base = emi_get_base(cfg, j);
			if (!base)
				goto RET_FUN;

			res[j].rd_ostd[i] = EMI_RD_OSTD(i,
							READ_REG(EMI_OSTD_L(i)),
							READ_REG(EMI_OSTD_H(i)));
			res[j].wr_ostd[i] = EMI_WR_OSTD(i,
							READ_REG(EMI_OSTD_L(i)),
							READ_REG(EMI_OSTD_H(i)));
		}
	}

RET_FUN:
	enable_broadcast_mode(true);
	emi_bw_measure_exit(cfg);
	/* Enable AFIFO CHN DCM */
	bw_setbit(AFIFO_CHN_DCM, false, cfg->base + EMI_CONM);
	enable_broadcast_mode(false);
}

static void smi_bw_measure(struct bw_cfg *cfg, struct bw_res *res)
{
	u32 i, j;
	u32 offset, idx, lidx;
	u32 loopnum;
	u32 sel;
	struct larb_config *mlarb;
	u64 *p_bw;
	u32 *p_effi;
	u32 larb_id;
	void __iomem *base;
	u32 mon_cnt;

	base = cfg->base;
	p_bw = res->pbw;
	p_effi = res->p_effi;
	mlarb = mlist[cfg->m_id].plist;
	offset = cfg->p_idx;
	lidx = cfg->l_idx;
	larb_id = mlist[cfg->m_id].llist[lidx];
	loopnum = ((mlarb[lidx].pcount+3)/4);

	/* parallel monitor four port at one time */
	for (i = 0; i < loopnum; i++) {
		idx = i*4 + offset;

		/* clear monitor setting register */
		WRITE_REG(1, SMI_LARB_MON_CLR);
		sel = 0;

		/* enable parallel mode */
		WRITE_REG(1 << PARA_MODE, SMI_LARB_MON_CON);

		/* setup monitor setting */
		if (mlarb[lidx].pcount > (i*4+3))
			sel = ((mlarb[lidx].smi_plist[i*4] << PORT_SEL0)|
				(mlarb[lidx].smi_plist[i*4+1] << PORT_SEL1)|
				(mlarb[lidx].smi_plist[i*4+2] << PORT_SEL2)|
				(mlarb[lidx].smi_plist[i*4+3] << PORT_SEL3));
		else if (mlarb[lidx].pcount > (i*4+2))
			sel = ((mlarb[lidx].smi_plist[i*4] << PORT_SEL0)|
				(mlarb[lidx].smi_plist[i*4+1] << PORT_SEL1)|
				(mlarb[lidx].smi_plist[i*4+2] << PORT_SEL2));
		else if  (mlarb[lidx].pcount > (i*4+1))
			sel = ((mlarb[lidx].smi_plist[i*4] << PORT_SEL0)|
				(mlarb[lidx].smi_plist[i*4+1] << PORT_SEL1));
		else
			sel = ((mlarb[lidx].smi_plist[i*4] << PORT_SEL0));

		WRITE_REG(sel, SMI_LARB_MON_PORT);

		/* make sure settings are written */
		wmb();

		/* trigger monitor */
		WRITE_REG(1, SMI_LARB_MON_EN);

//		mdelay(cfg->t_ms);
		usleep_range(cfg->t_ms*USEC_PER_MSEC, cfg->t_ms*USEC_PER_MSEC);

		WRITE_REG(0, SMI_LARB_MON_EN);

		/* data_bw: (kbytes/sec) */
		/* bus_bw: (kbytes/sec) */
		if (mlarb[lidx].pcount > (i*4+3)) {
			mon_cnt = SMI_PARA_MON;
			/* get bw result */
			p_bw[idx] = READ_REG(SMI_LARB_MON_ACT_CNT)/cfg->t_ms;
			p_bw[idx+1] = READ_REG(SMI_LARB_MON_REQ_CNT)/cfg->t_ms;
			p_bw[idx+2] = READ_REG(SMI_LARB_MON_BEAT_CNT)/cfg->t_ms;
			p_bw[idx+3] = READ_REG(SMI_LARB_MON_BYTE_CNT)/cfg->t_ms;

			if (p_bw[idx] != 0) {
				p_effi[idx] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx]*res->cnt_pultra;
			}
			if (p_bw[idx+1] != 0) {
				p_effi[idx+1] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4+1],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx+1]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx+1]*res->cnt_pultra;
			}
			if (p_bw[idx+2] != 0) {
				p_effi[idx+2] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4+2],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx+2]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx+2]*res->cnt_pultra;
			}
			if (p_bw[idx+3] != 0) {
				p_effi[idx+3] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4+3],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx+3]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx+3]*res->cnt_pultra;
			}

			res->data_bw += (p_bw[idx]+
				     p_bw[idx+1]+
				     p_bw[idx+2]+
				     p_bw[idx+3]);
		} else if (mlarb[lidx].pcount > (i*4+2)) {
			mon_cnt = mlarb[lidx].pcount % SMI_PARA_MON;
			/* get bw result */
			p_bw[idx] = READ_REG(SMI_LARB_MON_ACT_CNT)/cfg->t_ms;
			p_bw[idx+1] = READ_REG(SMI_LARB_MON_REQ_CNT)/cfg->t_ms;
			p_bw[idx+2] = READ_REG(SMI_LARB_MON_BEAT_CNT)/cfg->t_ms;

			if (p_bw[idx] != 0) {
				p_effi[idx] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx]*res->cnt_pultra;
			}
			if (p_bw[idx+1] != 0) {
				p_effi[idx+1] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4+1],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx+1]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx+1]*res->cnt_pultra;
			}
			if (p_bw[idx+2] != 0) {
				p_effi[idx+2] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4+2],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx+2]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx+2]*res->cnt_pultra;
			}

			res->data_bw += (p_bw[idx] + p_bw[idx+1] + p_bw[idx+2]);
		} else if (mlarb[lidx].pcount > (i*4+1)) {
			mon_cnt = mlarb[lidx].pcount % SMI_PARA_MON;
			/* get bw result */
			p_bw[idx] = READ_REG(SMI_LARB_MON_ACT_CNT)/cfg->t_ms;
			p_bw[idx+1] = READ_REG(SMI_LARB_MON_REQ_CNT)/cfg->t_ms;

			if (p_bw[idx] != 0) {
				p_effi[idx] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx]*res->cnt_pultra;
			}
			if (p_bw[idx+1] != 0) {
				p_effi[idx+1] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4+1],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx+1]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx+1]*res->cnt_pultra;
			}

			res->data_bw += (p_bw[idx] + p_bw[idx+1]);
		} else {
			mon_cnt = mlarb[lidx].pcount % SMI_PARA_MON;
			/* get bw result */
			p_bw[idx] = READ_REG(SMI_LARB_MON_ACT_CNT)/cfg->t_ms;

			if (p_bw[idx] != 0) {
				p_effi[idx] = effi_measure(larb_id,
					mlarb[lidx].smi_plist[i*4],
					SUMMARY_LARB_MODE,
					res);
				res->tmp_ultra += p_bw[idx]*res->cnt_ultra;
				res->tmp_pultra += p_bw[idx]*res->cnt_pultra;
			}

			res->data_bw += p_bw[idx];
		}
		/* bus_bw = data_bw / efficiency(%) */
		for (j = 0; j < mon_cnt; j++)
			res->bus_bw += CAL_BUS_BW(p_bw[idx + j], p_effi[idx + j]);
	}
}

static int monitor_larb(struct bw_cfg *cfg, struct larb_res *larb_res[])
{
	u32 i;
	struct module_map m = mlist[cfg->m_id];

	for (i = 0; i < m.larb_num; i++) {
		cfg->base = find_device_addr(SMI_LARB_DRV, m.llist[i]);
		if (!cfg->base) {
			pr_err("[MTK_BW]Can't find SMI LARB device base!!\n");
			return -1;
		}
		larb_res[i]->larb_data_bw = 0;
		larb_res[i]->larb_bus_bw = 0;
		larb_res[i]->id = i;
		larb_bw_measure(cfg, larb_res[i]);
		get_larb_setting(cfg, larb_res[i]);
		if (cfg->base)
			iounmap(cfg->base);
	}

	return 0;
}

static int monitor_comm(struct bw_cfg *cfg, struct scomm_res *comm_res[])
{
	u32 i, smi_common_id;

	for (i = 0, smi_common_id = 0; i < smi_common_cnt; i++, smi_common_id++) {
		while (!is_smi_common_exist(smi_common_id))
			smi_common_id++;

		comm_res[i]->id = smi_common_id;
		cfg->base = find_device_addr(SMI_COMMON_DRV, smi_common_id);
		if (!cfg->base) {
			pr_err("[MTK_BW]Can't find SMI COMMON device base!!\n");
			return -1;
		}
		comm_cp_dp_bw_measure(cfg, comm_res[i]);
		if (cfg->base)
			iounmap(cfg->base);
	}

	return 0;
}

static int hybrid_measure_wrap(struct bw_cfg *cfg, struct bw_res *res)
{
	u32 idx;
	u32 err = 0;
	int ret;
	u64 ultra_cmd_cnt;
	u64 pultra_cmd_cnt;

	cfg->p_idx = 0;

	if (strncmp(cfg->name, "CPU", sizeof("CPU")) == 0 ||
	    strncmp(cfg->name, "GPU", sizeof("GPU")) == 0 ||
	    strncmp(cfg->name, "REF_EMI", sizeof("REF_EMI")) == 0) {
		ret = emi_set_base(cfg);
		if (ret < 0)
			return ret;

		emi_bw_measure(cfg, res);

		/* change to MB/s */
		res->data_bw = B_MS_TO_MB_S(res->data_bw);
		/* Bus bw = data bw/effi */
		res->bus_bw = CAL_BUS_BW(res->data_bw, res->m_effi);

		emi_release_base(cfg);
	} else if (strncmp(cfg->name, "APU", sizeof("APU")) == 0) {
		for (idx = 0; idx < mlist[cfg->m_id].common_num; idx++) {
			cfg->base = find_device_addr(SMI_COMMON_DRV,
					mlist[cfg->m_id].clist[idx]);
			if (!cfg->base) {
				pr_err("[MTK_BW]Can't find SMI COMMON device base!!\n");
				return -1;
			}
			cfg->c_idx = idx;
			comm_bw_measure(cfg, res);
			if (cfg->base)
				iounmap(cfg->base);
		}
		/* change to MB/s */
		res->data_bw = B_MS_TO_MB_S(res->data_bw);
		/* change to MB/s */
		res->bus_bw = B_MS_TO_MB_S(res->bus_bw);

		res->m_effi = PERCENTAGE(res->data_bw, res->bus_bw);
	} else {
		for (idx = 0; idx < mlist[cfg->m_id].larb_num; idx++) {
			cfg->base = find_device_addr(SMI_LARB_DRV,
					mlist[cfg->m_id].llist[idx]);
			if (!cfg->base) {
				pr_err("[MTK_BW]Can't find SMI LARB device base!!\n");
				return -1;
			}
			cfg->l_idx = idx;
			if (idx != 0) {
				cfg->p_idx +=
					mlist[cfg->m_id].plist[idx-1].pcount;
			}
			smi_bw_measure(cfg, res);
			if (cfg->base)
				iounmap(cfg->base);
		}
		for (idx = 0; idx < mlist[cfg->m_id].common_num; idx++) {
			cfg->base = find_device_addr(SMI_COMMON_DRV,
					mlist[cfg->m_id].clist[idx]);
			if (!cfg->base) {
				pr_err("[MTK_BW]Can't find SMI COMMON device base!!\n");
				return -1;
			}
			cfg->c_idx = idx;
			comm_bw_measure(cfg, res);
			if (cfg->base)
				iounmap(cfg->base);
		}
		/* Module ultra rate = */
		/* sum(data_bw[i]*ultra_cnt[i])/sum(data_bw)/CMD_CNT */

		ultra_cmd_cnt = bw_div(res->tmp_ultra, res->data_bw);
		pultra_cmd_cnt = bw_div(res->tmp_pultra, res->data_bw);

		/* to calculate Module ultra/preultra rate */
		res->r_ultra = PERCENTAGE(ultra_cmd_cnt, CMD_NUM);
		res->r_pultra = PERCENTAGE(pultra_cmd_cnt, CMD_NUM);

		/* change to MB/s */
		res->data_bw = B_MS_TO_MB_S(res->data_bw);
		/* change to MB/s */
		res->bus_bw = B_MS_TO_MB_S(res->bus_bw);

		res->m_effi = PERCENTAGE(res->data_bw, res->bus_bw);

	}

	return err;
}

static int monitor_all_module(struct bw_cfg *cfg, struct bw_res *res[])
{
	u32 m_idx;

	for (m_idx = 0; m_idx < module_cnt; m_idx++) {

		cfg->m_id = m_idx;
		cfg->name = mlist[m_idx].name;

		/* do not monitor bw when enable = 0 */
		if (!mlist[m_idx].enable || m_idx == sum_id)
			continue;

		/* if proc_mon =1 only monitor processor */
		/* proc do not connect to LARB */
		if (mlist[m_idx].larb_num != 0 &&
		    total_mon_cfg.proc_mon == 1 &&
		    m_idx == ref_emi_id)
			continue;

		hybrid_measure_wrap(cfg, res[m_idx]);

		/* REF_EMI only for debug */
		if (m_idx != ref_emi_id) {
			res[sum_id]->data_bw += res[m_idx]->data_bw;
			res[sum_id]->bus_bw += res[m_idx]->bus_bw;
		}
	}

	res[sum_id]->m_effi = PERCENTAGE(res[sum_id]->data_bw, res[sum_id]->bus_bw);

	/* Get total weighting ultra/pre-ultra rate */
	for (m_idx = 0; m_idx < module_cnt; m_idx++) {

		if (!mlist[m_idx].enable || m_idx == sum_id || m_idx == ref_emi_id)
			continue;

		/* if proc_mon =1 only monitor processor */
		if (mlist[m_idx].larb_num != 0 && total_mon_cfg.proc_mon == 1)
			continue;

		res[sum_id]->r_ultra += res[m_idx]->r_ultra *
				PERCENTAGE(res[m_idx]->bus_bw, res[sum_id]->bus_bw);

		res[sum_id]->r_pultra += res[m_idx]->r_pultra *
				PERCENTAGE(res[m_idx]->bus_bw, res[sum_id]->bus_bw);
	}

	res[sum_id]->r_ultra = PERCENTAGE_ALIGN(res[sum_id]->r_ultra);
	res[sum_id]->r_pultra = PERCENTAGE_ALIGN(res[sum_id]->r_pultra);

	return 0;
}

static void get_emi_port_effi(u32 m_idx, void *res)
{
	u32 effi_tmp = 0;
	struct bw_res *module_res = (struct bw_res *)res;

	if (strncmp(mlist[m_idx].name, "CPU", sizeof("CPU")) == 0) {
		if (emi_effi_mon) {
			effi_tmp = effi_measure(0, CEN_EMI(0), EMI_LARB_MODE, res);
			effi_tmp += effi_measure(0, CEN_EMI(1), EMI_LARB_MODE, res);
			module_res->m_effi = (effi_tmp / 2);
		} else {
			module_res->m_effi = STATIC_CPU_EFFI;
		}
	} else if (strncmp(mlist[m_idx].name, "GPU", sizeof("GPU")) == 0) {
		if (emi_effi_mon)
			module_res->m_effi = effi_measure(0, CEN_EMI(7), EMI_LARB_MODE, res);
		else
			module_res->m_effi = STATIC_GPU_EFFI;
	} else if (strncmp(mlist[m_idx].name, "REF_EMI", sizeof("REF_EMI")) == 0) {
		module_res->m_effi = STATIC_REF_EMI_EFFI;
	}
}

static ssize_t bw_total_show(struct device_driver *drv, char *buf)
{
	struct bw_cfg *cfg;
	struct bw_res **res;
	u32 m_idx;
	u32 m_port_nr;
	u32 i;
	u32 len = 0;

	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;

	res = kcalloc(module_cnt, sizeof(*res), GFP_KERNEL);
	if (!res)
		goto total_free;

	for (m_idx = 0; m_idx < module_cnt; m_idx++) {
		res[m_idx] = kmalloc(sizeof(struct bw_res), GFP_KERNEL);
		if (!res[m_idx])
			goto total_free;
		*res[m_idx] = (struct bw_res) {
			.data_bw = 0,
			.bus_bw = 0,
			.m_effi = 0,
			.cnt_ultra = 0,
			.cnt_pultra = 0,
			.avg_burst = 0,
			.tmp_ultra = 0,
			.tmp_pultra = 0,
		};

		get_emi_port_effi(m_idx, res[m_idx]);

		m_port_nr = 0;
		if (mlist[m_idx].larb_num) {
			for (i = 0; i < mlist[m_idx].larb_num; i++)
				m_port_nr += mlist[m_idx].plist[i].pcount;
			res[m_idx]->port_num = m_port_nr;
			res[m_idx]->pbw = kmalloc_array(m_port_nr,
					sizeof(*(res[m_idx]->pbw)), GFP_KERNEL);
			if (!res[m_idx]->pbw)
				goto total_free;
			res[m_idx]->p_effi = kmalloc_array(m_port_nr,
					sizeof(*(res[m_idx]->p_effi)), GFP_KERNEL);
			if (!res[m_idx]->p_effi)
				goto total_free;
		} else if (mlist[m_idx].common_num) {
			for (i = 0; i < mlist[m_idx].common_num; i++)
				m_port_nr += mlist[m_idx].cplist[i].pcount;
			res[m_idx]->p_effi = kmalloc_array(m_port_nr,
					sizeof(*(res[m_idx]->p_effi)), GFP_KERNEL);
			if (!res[m_idx]->p_effi)
				goto total_free;

		}
	}
	cfg->t_ms = total_mon_cfg.t_ms;

	/* monitor all module */
	monitor_all_module(cfg, res);

	put_buf_all_module(cfg, res, buf, &len);

total_free:

	kfree(cfg);

	if (res) {
		for (m_idx = 0; m_idx < module_cnt; m_idx++) {
			if (mlist[m_idx].larb_num && res[m_idx]) {
				kfree(res[m_idx]->pbw);
				kfree(res[m_idx]->p_effi);
			}
			kfree(res[m_idx]);
		}
		kfree(res);
	}
	return len;
}

static ssize_t bw_larb_show(struct device_driver *drv, char *buf)
{
	struct larb_res **res;
	struct module_map m = mlist[larb_mon_cfg.m_id];
	u32 i;
	u32 len = 0;

	res = kmalloc_array(m.larb_num, sizeof(*res), GFP_KERNEL);
	if (!res)
		return -ENOMEM;

	for (i = 0; i < m.larb_num; i++) {
		res[i] = kmalloc(sizeof(struct larb_res), GFP_KERNEL);
		if (!res[i])
			goto larb_free;

		res[i]->r_bw = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->r_bw)), GFP_KERNEL);
		if (!res[i]->r_bw)
			goto larb_free;

		res[i]->w_bw = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->w_bw)), GFP_KERNEL);
		if (!res[i]->w_bw)
			goto larb_free;

		res[i]->max_cp = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->max_cp)), GFP_KERNEL);
		if (!res[i]->max_cp)
			goto larb_free;

		res[i]->avg_cp = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->avg_cp)), GFP_KERNEL);
		if (!res[i]->avg_cp)
			goto larb_free;

		res[i]->r_avg_dp = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->r_avg_dp)), GFP_KERNEL);
		if (!res[i]->r_avg_dp)
			goto larb_free;

		res[i]->w_avg_dp = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->w_avg_dp)), GFP_KERNEL);
		if (!res[i]->w_avg_dp)
			goto larb_free;

		res[i]->ostdl = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->ostdl)), GFP_KERNEL);
		if (!res[i]->ostdl)
			goto larb_free;

		res[i]->ultra = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->ultra)), GFP_KERNEL);
		if (!res[i]->ultra)
			goto larb_free;

		res[i]->pultra = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->pultra)), GFP_KERNEL);
		if (!res[i]->pultra)
			goto larb_free;

		res[i]->r_ultra = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->r_ultra)), GFP_KERNEL);
		if (!res[i]->r_ultra)
			goto larb_free;

		res[i]->r_pultra = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->r_pultra)), GFP_KERNEL);
		if (!res[i]->r_pultra)
			goto larb_free;

		res[i]->effi = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->effi)), GFP_KERNEL);
		if (!res[i]->effi)
			goto larb_free;

		res[i]->burst = kmalloc_array(m.plist[i].pcount,
				sizeof(*(res[i]->burst)), GFP_KERNEL);
		if (!res[i]->burst)
			goto larb_free;

	}


	/* monitor smi larb */
	monitor_larb(&larb_mon_cfg, res);

	put_buf_larb(&larb_mon_cfg, res, buf, &len);

larb_free:

	for (i = 0; i < m.larb_num; i++) {
		if (res[i]) {
			kfree(res[i]->max_cp);
			kfree(res[i]->avg_cp);
			kfree(res[i]->r_bw);
			kfree(res[i]->r_avg_dp);
			kfree(res[i]->w_bw);
			kfree(res[i]->w_avg_dp);
			kfree(res[i]->ostdl);
			kfree(res[i]->ultra);
			kfree(res[i]->pultra);
			kfree(res[i]->r_ultra);
			kfree(res[i]->r_pultra);
			kfree(res[i]->effi);
			kfree(res[i]->burst);
			kfree(res[i]);
		}
	};

	kfree(res);

	return len;
}

static int malloc_comm_member(struct scomm_res *comm_res[],
			      unsigned int port_num,
			      unsigned int idx)
{
	int err = 0;

	comm_res[idx]->r_bw = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->r_bw)), GFP_KERNEL);

	comm_res[idx]->w_bw = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->w_bw)), GFP_KERNEL);

	comm_res[idx]->r_max_cp = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->r_max_cp)), GFP_KERNEL);

	comm_res[idx]->w_max_cp = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->w_max_cp)), GFP_KERNEL);

	comm_res[idx]->r_avg_cp = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->r_avg_cp)), GFP_KERNEL);

	comm_res[idx]->w_avg_cp = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->w_avg_cp)), GFP_KERNEL);

	comm_res[idx]->r_avg_dp = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->r_avg_dp)), GFP_KERNEL);

	comm_res[idx]->w_avg_dp = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->w_avg_dp)), GFP_KERNEL);

	comm_res[idx]->r_ultra = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->r_ultra)), GFP_KERNEL);

	comm_res[idx]->r_pultra = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->r_pultra)), GFP_KERNEL);

	comm_res[idx]->effi = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->effi)), GFP_KERNEL);

	comm_res[idx]->burst = kmalloc_array(port_num,
			sizeof(*(comm_res[idx]->burst)), GFP_KERNEL);

	if (!comm_res[idx]->r_bw || !comm_res[idx]->w_bw ||
	    !comm_res[idx]->r_max_cp || !comm_res[idx]->w_max_cp ||
	    !comm_res[idx]->r_avg_cp || !comm_res[idx]->w_avg_cp ||
	    !comm_res[idx]->r_avg_dp || !comm_res[idx]->w_avg_dp ||
	    !comm_res[idx]->r_ultra || !comm_res[idx]->r_pultra ||
	    !comm_res[idx]->effi || !comm_res[idx]->burst)
		err = -1;

	return err;
}

static ssize_t bw_scomm_show(struct device_driver *drv, char *buf)
{
	struct bw_cfg *cfg;
	struct scomm_res **comm_res;
	int err;
	u32 port_num;
	u32 len = 0;
	u32 i, smi_common_id;

	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;

	comm_res = kmalloc_array(smi_common_cnt, sizeof(*comm_res), GFP_KERNEL);
	if (!comm_res)
		goto scomm_free;

	for (i = 0, smi_common_id = 0; i < smi_common_cnt; i++, smi_common_id++) {

		comm_res[i] = kmalloc(sizeof(struct scomm_res), GFP_KERNEL);
		if (!comm_res[i])
			goto scomm_free;

		while (!is_smi_common_exist(smi_common_id))
			smi_common_id++;

		err = get_smi_common_port_info(&(comm_res[i]->ip_num),
					 &(comm_res[i]->op_num),
					 smi_common_id);
		if (err) {
			pr_err("[MTK_BW]Can't get smi common port info!!\n");
			goto scomm_free;
		}

		port_num = comm_res[i]->ip_num + comm_res[i]->op_num;

		err = malloc_comm_member(comm_res, port_num, i);
		if (err) {
			pr_err("[MTK_BW]Can't malloc comm result!!\n");
			goto scomm_free;
		}
	}

	cfg->t_ms = scomm_mon_cfg.t_ms;

	/* monitor smi common */
	monitor_comm(cfg, comm_res);

	put_buf_common(cfg, comm_res, buf, &len);

scomm_free:

	if (comm_res) {
		for (i = 0; i < smi_common_cnt; i++) {
			if (comm_res[i]) {
				kfree(comm_res[i]->r_bw);
				kfree(comm_res[i]->r_max_cp);
				kfree(comm_res[i]->r_avg_cp);
				kfree(comm_res[i]->r_avg_dp);
				kfree(comm_res[i]->w_bw);
				kfree(comm_res[i]->w_max_cp);
				kfree(comm_res[i]->w_avg_cp);
				kfree(comm_res[i]->w_avg_dp);
				kfree(comm_res[i]->r_ultra);
				kfree(comm_res[i]->r_pultra);
				kfree(comm_res[i]->effi);
				kfree(comm_res[i]->burst);
				kfree(comm_res[i]);
			}
		}
		kfree(comm_res);
	}

	kfree(cfg);

	return len;
}

static int emi_measure_wrap(struct bw_cfg *cfg, struct emi_res *res)
{
	int ret;

	ret = emi_set_base(cfg);
	if (ret < 0)
		return ret;

	emi_detail_bw_measure(cfg, res);

	emi_release_base(cfg);

	return ret;
}

static ssize_t bw_bus_show(struct device_driver *drv, char *buf)
{
	int i, j;
	u32 len = 0;
	struct bw_cfg *cfg;
	struct emi_res *emi_res = NULL;
	const u32 res_cnt = emi_cnt + 1;
	u64 total_bus_bw = 0;

	/* allocate buffer */
	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg) {
		len = -ENOMEM;
		goto RET_FUNC;
	}

	emi_res = kmalloc(sizeof(*emi_res) * (res_cnt), GFP_KERNEL);
	if (!emi_res) {
		len = -ENOMEM;
		goto RET_FUNC;
	}
	for (i = 0; i < res_cnt; i++) {
		emi_res[i].bus_bw =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].data_bw =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].latency =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].m_effi =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].burst =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].arb_info =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].rd_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].wr_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);

		if (!emi_res[i].bus_bw || !emi_res[i].data_bw ||
		    !emi_res[i].latency || !emi_res[i].m_effi ||
		    !emi_res[i].burst || !emi_res[i].arb_info ||
		    !emi_res[i].rd_ostd || !emi_res[i].wr_ostd) {
			len = -ENOMEM;
			goto RET_FUNC;
		}
	}

	/* initial emi effi */
	for (i = 0; i < res_cnt; i++) {
		for (j = 0; j < emi_port_cnt; j++) {
			if (emi_effi_mon) {
				emi_res[i].m_effi[j] = 0;
				emi_res[i].burst[j] = 0;
			} else {
				emi_res[i].m_effi[j] = STATIC_REF_EMI_EFFI;
				emi_res[i].burst[j] = 0;
			}
		}
	}

	/* measure emi bw */
	cfg->t_ms = emi_mon_cfg.t_ms;
	if (emi_measure_wrap(cfg, emi_res) < 0) {
		len = -EPERM;
		goto RET_FUNC;
	}

	/* output emi bw result */
	for (i = 0; i < emi_port_cnt; i++)
		total_bus_bw += CAL_BUS_BW(emi_res[emi_cnt].data_bw[i], emi_res[emi_cnt].m_effi[i]);

	putbuf(buf, &len, "%llu%%\n", NORMALIZE_PERC(PERCENTAGE(total_bus_bw, total_bw)));

RET_FUNC:
	/* release buffer */
	if (emi_res) {
		for (i = 0; i < res_cnt; i++) {
			kfree(emi_res[i].bus_bw);
			kfree(emi_res[i].data_bw);
			kfree(emi_res[i].latency);
			kfree(emi_res[i].m_effi);
			kfree(emi_res[i].burst);
			kfree(emi_res[i].arb_info);
			kfree(emi_res[i].rd_ostd);
			kfree(emi_res[i].wr_ostd);
		}
		kfree(emi_res);
	}
	kfree(cfg);
	return len;
}

static ssize_t bw_emi_show(struct device_driver *drv, char *buf)
{
	int i, j;
	u32 len = 0;
	struct bw_cfg *cfg;
	struct emi_res *emi_res = NULL;
	const u32 res_cnt = emi_cnt + 1;

	/* allocate buffer */
	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg) {
		len = -ENOMEM;
		goto RET_FUNC;
	}

	emi_res = kmalloc(sizeof(*emi_res) * (res_cnt), GFP_KERNEL);
	if (!emi_res) {
		len = -ENOMEM;
		goto RET_FUNC;
	}
	for (i = 0; i < res_cnt; i++) {
		emi_res[i].bus_bw =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].data_bw =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].latency =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].m_effi =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].burst =
			kcalloc(emi_port_cnt, sizeof(u64), GFP_KERNEL);
		emi_res[i].arb_info =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].rd_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].wr_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);

		if (!emi_res[i].bus_bw || !emi_res[i].data_bw ||
		    !emi_res[i].latency || !emi_res[i].m_effi ||
		    !emi_res[i].burst || !emi_res[i].arb_info ||
		    !emi_res[i].rd_ostd || !emi_res[i].wr_ostd) {
			len = -ENOMEM;
			goto RET_FUNC;
		}
	}

	/* initial emi effi */
	for (i = 0; i < res_cnt; i++) {
		for (j = 0; j < emi_port_cnt; j++) {
			if (emi_effi_mon) {
				emi_res[i].m_effi[j] = 0;
				emi_res[i].burst[j] = 0;
			} else {
				emi_res[i].m_effi[j] = STATIC_REF_EMI_EFFI;
				emi_res[i].burst[j] = 0;
			}
		}
	}

	/* measure emi bw */
	cfg->t_ms = emi_mon_cfg.t_ms;
	if (emi_measure_wrap(cfg, emi_res) < 0) {
		len = -EPERM;
		goto RET_FUNC;
	}

	/* output emi bw result */
	cfg->emi_detail = emi_mon_cfg.emi_detail;
	if (cfg->emi_detail)
		put_buf_emi_detail(cfg, emi_res, buf, &len);
	else
		put_buf_emi_total(cfg, emi_res, buf, &len);

RET_FUNC:
	/* release buffer */
	if (emi_res) {
		for (i = 0; i < res_cnt; i++) {
			kfree(emi_res[i].bus_bw);
			kfree(emi_res[i].data_bw);
			kfree(emi_res[i].latency);
			kfree(emi_res[i].m_effi);
			kfree(emi_res[i].burst);
			kfree(emi_res[i].arb_info);
			kfree(emi_res[i].rd_ostd);
			kfree(emi_res[i].wr_ostd);
		}
		kfree(emi_res);
	}
	kfree(cfg);
	return len;
}

static void set_larb_ostdl(unsigned int larb_id,
			   unsigned int port_id,
			   unsigned int val)
{
	void __iomem *base;

	base = find_device_addr(SMI_LARB_DRV, larb_id);
	if (!base) {
		pr_err("[MTK_BW]Can't fine SMI LARB device base!!\n");
		return;
	}
	WRITE_REG(val, SMI_LARB_OSTDL_PORTx(port_id));
	if (base)
		iounmap(base);
}

static void set_larb_force_ultra(unsigned int larb_id,
				 unsigned int port_id,
				 unsigned int val)
{
	void __iomem *base;
	u32 ultra_val;

	base = find_device_addr(SMI_LARB_DRV, larb_id);
	if (!base) {
		pr_err("[MTK_BW]Can't fine SMI LARB device base!!\n");
		return;
	}

	ultra_val = READ_REG(SMI_LARB_FORCE_ULTRA);
	if (val == 0)
		ultra_val &= ~(1 << port_id);
	else
		ultra_val |= (1 << port_id);

	WRITE_REG(ultra_val, SMI_LARB_FORCE_ULTRA);
	if (base)
		iounmap(base);
}

static void set_larb_force_preultra(unsigned int larb_id,
				    unsigned int port_id,
				    unsigned int val)
{
	void __iomem *base;
	u32 preultra_val;

	base = find_device_addr(SMI_LARB_DRV, larb_id);
	if (!base) {
		pr_err("[MTK_BW]Can't fine SMI LARB device base!!\n");
		return;
	}

	preultra_val = READ_REG(SMI_LARB_FORCE_PREULTRA);
	if (val == 0)
		preultra_val &= ~(1 << port_id);
	else
		preultra_val |= (1 << port_id);

	WRITE_REG(preultra_val, SMI_LARB_FORCE_PREULTRA);
	if (base)
		iounmap(base);
}

static ssize_t bw_total_store(struct device_driver *drv,
			      const char *buf,
			      size_t count)
{
	u32 m_id;
	u32 enable;
	u32 t_ms;
	u32 proc_mon;

	if (sscanf(buf, "%u %u %u %u", &m_id, &enable, &t_ms, &proc_mon) == 4) {
		mlist[m_id].enable = enable;
		total_mon_cfg.t_ms = t_ms;
		total_mon_cfg.proc_mon = proc_mon;

		pr_info("[MTK_BW]set m_id: %u, enable: %u, time: %u ms, proc: %u\n",
					m_id,
					mlist[m_id].enable,
					total_mon_cfg.t_ms,
					total_mon_cfg.proc_mon);
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
	}

	return count;
}

static ssize_t larb_config_store(struct device_driver *drv,
				 const char *buf,
				 size_t count)
{
	u32 mode;
	u32 larb_id;
	u32 port_id;
	u32 val;

	if (sscanf(buf, "%d %d %d %x", &mode, &larb_id, &port_id, &val) == 4) {
		pr_info("[MTK_BW]mode = %d, larb ID = %d, port= %d, val=%d\n",
					mode,
					larb_id,
					port_id,
					val);
		if (larb_id >= MAX_LARB_NR) {
			pr_err("[MTK_BW]Wrong Argument!!\n");
			return count;
		}
		if (mode == OSTDL_MODE) {
			if (val > 0x3f) {
				pr_err("[MTK_BW]Wrong Argument!!\n");
				return count;
			}
			set_larb_ostdl(larb_id, port_id, val);
		} else if (mode == ULTRA_MODE) {
			if (val != 0 && val != 1) {
				pr_err("[MTK_BW]Wrong Argument!!\n");
				return count;
			}
			set_larb_force_ultra(larb_id, port_id, val);
		} else if (mode == PREULTRA_MODE) {
			if (val != 0 && val != 1) {
				pr_err("[MTK_BW]Wrong Argument!!\n");
				return count;
			}
			set_larb_force_preultra(larb_id, port_id, val);
		} else
			pr_err("[MTK_BW]Wrong Argument!!\n");
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
	}

	return count;
}

static ssize_t mask_store(struct device_driver *drv,
				 const char *buf,
				 size_t count)
{
	u32 miu2gmc_id;
	u32 port_id;
	u32 enable;
	int err;

	if (sscanf(buf, "%d %d %d", &miu2gmc_id, &port_id, &enable) == 3) {
		pr_info("[MTK_BW]MIU2GMC ID = %d, port= %d, enable=%d\n",
					miu2gmc_id,
					port_id,
					enable);
		err = mtk_miu2gmc_mask(miu2gmc_id, port_id, (bool)enable);
		if (err)
			pr_err("[MTK_BW]Mask error!!\n");
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
	}

	return count;
}

static ssize_t ip_config_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;
	u32 idx = 0, idx2 = 0;
	u32 mid = 0;

	for (mid = 0; mid < module_cnt; mid++) {
		if (strncmp(mlist[mid].name, "REF_EMI", sizeof("REF_EMI")) == 0 ||
		    strncmp(mlist[mid].name, "SUM", sizeof("SUM")) == 0)
			continue;
		putbuf(buf, &len, "\033[4mModule : ");
		if (!strncmp(mlist[mid].domain, "INPUT", sizeof("INPUT")))
			putbuf(buf, &len, "\033[43;30m  %s  ", mlist[mid].name);
		else if (!strncmp(mlist[mid].domain, "TS", sizeof("TS")))
			putbuf(buf, &len, "\033[46;30m  %s  ", mlist[mid].name);
		else if (!strncmp(mlist[mid].domain, "FRAME", sizeof("FRAME")))
			putbuf(buf, &len, "\033[42;30m  %s  ", mlist[mid].name);
		else if (!strncmp(mlist[mid].domain, "GFX", sizeof("GFX")))
			putbuf(buf, &len, "\033[45;30m  %s  ", mlist[mid].name);
		else
			putbuf(buf, &len, "\033[47;30m  %s  ", mlist[mid].name);
		putbuf(buf, &len, "\033[0m\033[4m / ID: %d\033[0m\n", mlist[mid].module_id);

		if (strncmp(mlist[mid].name, "CPU", sizeof("CPU")) == 0) {
			putbuf(buf, &len, "EMI_PORT[ 0 1 ]\n\n");
			continue;
		} else if (strncmp(mlist[mid].name, "GPU", sizeof("GPU")) == 0) {
			putbuf(buf, &len, "EMI_PORT[ 7 ]\n\n");
			continue;
		}
		for (idx = 0; idx < mlist[mid].larb_num; idx++) {
			putbuf(buf, &len, "SMI_LARB_%d : ", mlist[mid].llist[idx]);
			putbuf(buf, &len, "PORT[ ");
			for (idx2 = 0; idx2 < mlist[mid].plist[idx].pcount; idx2++)
				putbuf(buf, &len, "%d ", mlist[mid].plist[idx].smi_plist[idx2]);
			putbuf(buf, &len, "]\n");
		}
		for (idx = 0; idx < mlist[mid].common_num; idx++) {
			putbuf(buf, &len, "SMI_COMM_%d : ", mlist[mid].clist[idx]);
			putbuf(buf, &len, "PORT[ ");
			for (idx2 = 0; idx2 < mlist[mid].cplist[idx].pcount; idx2++)
				putbuf(buf, &len, "%d ", mlist[mid].cplist[idx].com_plist[idx2]);
			putbuf(buf, &len, "]\n");
		}
		putbuf(buf, &len, "\n");
	}

	return len;
}

static int fake_engine_loop(u32 id, u32 *loop)
{
	void __iomem *base;
	u32 time_out = 0;

	if (fake_eng_loop_mode != HW_LOOP)
		return 0;

	base = find_device_addr(MIU2GMC_DRV, id);
	if (!base) {
		pr_err("[MTK_BW] Can't find MIU2GMC_DRV[%d] device base!!\n", id);
		return -1;
	}

	if (*loop)
		WRITE_REG((READ_REG(TEST_MODE) | BIT(FAKE_ENG_LOOP)), TEST_MODE);
	else
		WRITE_REG((READ_REG(TEST_MODE) & ~BIT(FAKE_ENG_LOOP)), TEST_MODE);

	while (!(READ_REG(MISC) & REG_R_TEST_FINISH)) {
		usleep_range(FAKE_ENG_DELAY_TIME_MIN, FAKE_ENG_DELAY_TIME_MAX);
		if (++time_out > FAKE_ENG_TIME_OUT)
			break;
	}

	/* clean fake engine */
	WRITE_REG(0, MISC);

	if (base)
		iounmap(base);

	return 0;
}

int fake_engine_test(u64 start_addr,
		     u64 size,
		     u16 pattern,
		     u32 type,
		     u32 id,
		     u32 *loop)
{
	u32 base_addr_8KB_unit;
	u16 base_addr_l, base_addr_h;
	u32 len_16B_unit;
	u16 len_l, len_h;
	int res;
	int result = NO_RESULT;
	void __iomem *base;

	base = find_device_addr(MIU2GMC_DRV, id);
	if (!base) {
		pr_err("[MTK_BW]Can't find MIU2GMC_DRV [%d] device base!!\n", id);
		return -1;
	}

	/* address 8KB unit, start_addr need to shift right 13 */
	BW_DBG("[MTK-BW] start address alignment[%u]\n", fake_eng_st_align);
	base_addr_8KB_unit = (u32)(start_addr >> fake_eng_st_align);
	base_addr_l = (u16)(base_addr_8KB_unit & 0x0000ffff);
	base_addr_h = (u16)((base_addr_8KB_unit & 0xffff0000) >> 16);

	/* size 16B unit, size need to shift right 4 */
	len_16B_unit = (u32)(size >> 4);
	len_l = (u16)(len_16B_unit & 0x0000ffff);
	len_h = (u16)((len_16B_unit & 0xffff0000) >> 16);

	/* clean fake engine */
	WRITE_REG(0x0000, MISC);

	/* disable miu2gmc bridge mask */
	WRITE_REG(0x0, GROUP_REQ_MASK);

	/* setup test length(16B unit) */
	WRITE_REG(len_l, TEST_LEN_L);
	WRITE_REG(len_h, TEST_LEN_H);

	/* setup test data */
	WRITE_REG(pattern, TEST_DATA);

	/* setup test start addr(8KB unit, phys addr) */
	WRITE_REG(base_addr_l, TEST_BASE_ADDR_L);
	WRITE_REG(base_addr_h, TEST_BASE_ADDR_H);

	do {
		if (type == READ_ONLY) {
			/* MISC[3]: arb test engine selection */
			/* 0:rq2mi0, 1: rq2mi_test  */
			WRITE_REG((REG_READ_ONLY | 0x0008), MISC);
			/* MISC[0]: test enable */
			WRITE_REG((REG_READ_ONLY | 0x0009), MISC);

		} else if (type == WRITE_ONLY) {
			/* MISC[3]: arb test engine selection */
			/* 0:rq2mi0, 1: rq2mi_test  */
			WRITE_REG((REG_WRITE_ONLY | 0x0008), MISC);
			/* MISC[0]: test enable */
			WRITE_REG((REG_WRITE_ONLY | 0x0009), MISC);

		} else {
			/* MISC[3]: arb test engine selection */
			/* 0:rq2mi0, 1: rq2mi_test  */
			WRITE_REG(0x0008, MISC);
			/* MISC[0]: test enable */
			WRITE_REG(0x0009, MISC);
		}
		if (fake_eng_loop_mode == HW_LOOP && *loop)
			return PASS;

		/* We do not have loop mode in E1, */
		/* busy waiting will cause CPU 100% overhead */
		/* So we wait 500 us before checking reuslt*/
		usleep_range(FAKE_ENG_DELAY_TIME_MIN, FAKE_ENG_DELAY_TIME_MAX);

		do {

			res = READ_REG(MISC);
			if (res & REG_R_TEST_FINISH) {
				if (res & REG_R_TEST_FAIL) {
					result = FAIL;
					goto fake_eng_done;
				}
				break;
			}
		} while (1);

		result = PASS;

fake_eng_done:
		WRITE_REG(0x0000, MISC);

	} while (*loop);

	if (base)
		iounmap(base);

	return result;
}

static ssize_t fake_eng_loop52_store(struct device_driver *drv,
				     const char *buf,
				     size_t count)
{
	if (kstrtou32(buf, 0, &fake_eng52_loop_mode) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");
	else
		pr_info("[MTK_BW]fake engine loop mode= %u\n",
					fake_eng52_loop_mode);

	return count;
}

static ssize_t fake_engine52_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "Fake engine 52: ");

	if (fake_eng52_result <= NO_RESULT)
		putbuf(buf, &len, "NO RESULT\n");
	else if (fake_eng52_result == FAIL)
		putbuf(buf, &len, "\033[31mFAIL\033[0m\n");
	else
		putbuf(buf, &len, "\033[32mPASS\033[0m\n");

	return len;
}

static ssize_t fake_engine52_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	u64 start_addr;
	u64 size;
	u32 type;
	u16 pat;

	if (sscanf(buf, "%llx %llx %u %hx", &start_addr, &size, &type, &pat) == 4) {
		pr_info("start = %llx, size = %llx, type = %u, pattern = %x\n",
				start_addr, size, type, pat);
		if (size > 0x100000000) {
			pr_err("[MTK_BW]Wrong size > 4GB\n");
			return count;
		}
		if (type != READ_ONLY && type != WRITE_ONLY && type != RW) {
			pr_err("[MTK_BW]Wrong type, 0:Read 1:Write 2:R/W\n");
			return count;
		}
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	fake_eng52_result = fake_engine_test(start_addr,
					     size,
					     pat,
					     type,
					     52,
					     &fake_eng52_loop_mode);
	if (fake_eng52_result < 0)
		pr_err("[MTK_BW]Fail to start fake engine!!\n");

	return count;
}

static ssize_t fake_eng_loop44_store(struct device_driver *drv,
				     const char *buf,
				     size_t count)
{
	if (kstrtou32(buf, 0, &fake_eng44_loop_mode) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");
	else
		pr_info("[MTK_BW]fake engine loop mode= %u\n",
					fake_eng44_loop_mode);

	return count;
}

static ssize_t fake_engine44_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "Fake engine 44: ");

	if (fake_eng44_result <= NO_RESULT)
		putbuf(buf, &len, "NO RESULT\n");
	else if (fake_eng44_result == FAIL)
		putbuf(buf, &len, "\033[31mFAIL\033[0m\n");
	else
		putbuf(buf, &len, "\033[32mPASS\033[0m\n");

	return len;
}

static ssize_t fake_engine44_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	u64 start_addr;
	u64 size;
	u32 type;
	u16 pat;

	if (sscanf(buf, "%llx %llx %u %hx", &start_addr, &size, &type, &pat) == 4) {
		pr_info("start = %llx, size = %llx, type = %u, pattern = %x\n",
				start_addr, size, type, pat);
		if (size > 0x100000000) {
			pr_err("[MTK_BW]Wrong size > 4GB\n");
			return count;
		}
		if (type != READ_ONLY && type != WRITE_ONLY && type != RW) {
			pr_err("[MTK_BW]Wrong type, 0:Read 1:Write 2:R/W\n");
			return count;
		}
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	fake_eng44_result = fake_engine_test(start_addr,
					     size,
					     pat,
					     type,
					     44,
					     &fake_eng44_loop_mode);
	if (fake_eng44_result < 0)
		pr_err("[MTK_BW]Fail to start fake engine!!\n");

	return count;
}

static ssize_t fake_eng_loop29_store(struct device_driver *drv,
				     const char *buf,
				     size_t count)
{
	if (kstrtou32(buf, 0, &fake_eng29_loop_mode) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");
	else
		pr_info("[MTK_BW]fake engine loop mode= %u\n",
					fake_eng29_loop_mode);

	return count;
}

static ssize_t fake_engine29_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "Fake engine 29: ");

	if (fake_eng29_result <= NO_RESULT)
		putbuf(buf, &len, "NO RESULT\n");
	else if (fake_eng29_result == FAIL)
		putbuf(buf, &len, "\033[31mFAIL\033[0m\n");
	else
		putbuf(buf, &len, "\033[32mPASS\033[0m\n");

	return len;
}

static ssize_t fake_engine29_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	u64 start_addr;
	u64 size;
	u32 type;
	u16 pat;

	if (sscanf(buf, "%llx %llx %u %hx", &start_addr, &size, &type, &pat) == 4) {
		pr_info("start = %llx, size = %llx, type = %u, pattern = %x\n",
				start_addr, size, type, pat);
		if (size > 0x100000000) {
			pr_err("[MTK_BW]Wrong size > 4GB\n");
			return count;
		}
		if (type != READ_ONLY && type != WRITE_ONLY && type != RW) {
			pr_err("[MTK_BW]Wrong type, 0:Read 1:Write 2:R/W\n");
			return count;
		}
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	fake_eng29_result = fake_engine_test(start_addr,
					     size,
					     pat,
					     type,
					     29,
					     &fake_eng29_loop_mode);
	if (fake_eng29_result < 0)
		pr_err("[MTK_BW]Fail to start fake engine!!\n");

	return count;
}

static ssize_t fake_eng_loop21_store(struct device_driver *drv,
				     const char *buf,
				     size_t count)
{
	if (kstrtou32(buf, 0, &fake_eng21_loop_mode) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");
	else
		pr_info("[MTK_BW]fake engine loop mode= %u\n",
					fake_eng21_loop_mode);

	return count;
}

static ssize_t fake_engine21_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "Fake engine 21: ");

	if (fake_eng21_result <= NO_RESULT)
		putbuf(buf, &len, "NO RESULT\n");
	else if (fake_eng21_result == FAIL)
		putbuf(buf, &len, "\033[31mFAIL\033[0m\n");
	else
		putbuf(buf, &len, "\033[32mPASS\033[0m\n");

	return len;
}

static ssize_t fake_engine21_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	u64 start_addr;
	u64 size;
	u32 type;
	u16 pat;

	if (sscanf(buf, "%llx %llx %u %hx", &start_addr, &size, &type, &pat) == 4) {
		pr_info("start = %llx, size = %llx, type = %u, pattern = %x\n",
				start_addr, size, type, pat);
		if (size > 0x100000000) {
			pr_err("[MTK_BW]Wrong size > 4GB\n");
			return count;
		}
		if (type != READ_ONLY && type != WRITE_ONLY && type != RW) {
			pr_err("[MTK_BW]Wrong type, 0:Read 1:Write 2:R/W\n");
			return count;
		}
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	fake_eng21_result = fake_engine_test(start_addr,
					     size,
					     pat,
					     type,
					     21,
					     &fake_eng21_loop_mode);
	if (fake_eng21_result < 0)
		pr_err("[MTK_BW]Fail to start fake engine!!\n");

	return count;
}

static ssize_t fake_eng_loop19_store(struct device_driver *drv,
				     const char *buf,
				     size_t count)
{
	if (kstrtou32(buf, 0, &fake_eng19_loop_mode) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");
	else
		pr_info("[MTK_BW]fake engine loop mode= %u\n",
					fake_eng19_loop_mode);

	return count;
}

static ssize_t fake_engine19_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "Fake engine 19: ");

	if (fake_eng19_result <= NO_RESULT)
		putbuf(buf, &len, "NO RESULT\n");
	else if (fake_eng19_result == FAIL)
		putbuf(buf, &len, "\033[31mFAIL\033[0m\n");
	else
		putbuf(buf, &len, "\033[32mPASS\033[0m\n");

	return len;
}

static ssize_t fake_engine19_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	u64 start_addr;
	u64 size;
	u32 type;
	u16 pat;

	if (sscanf(buf, "%llx %llx %u %hx", &start_addr, &size, &type, &pat) == 4) {
		pr_info("start = %llx, size = %llx, type = %u, pattern = %x\n",
				start_addr, size, type, pat);
		if (size > 0x100000000) {
			pr_err("[MTK_BW]Wrong size > 4GB\n");
			return count;
		}
		if (type != READ_ONLY && type != WRITE_ONLY && type != RW) {
			pr_err("[MTK_BW]Wrong type, 0:Read 1:Write 2:R/W\n");
			return count;
		}
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	fake_eng19_result = fake_engine_test(start_addr,
					     size,
					     pat,
					     type,
					     19,
					     &fake_eng19_loop_mode);
	if (fake_eng19_result < 0)
		pr_err("[MTK_BW]Fail to start fake engine!!\n");

	return count;
}

static ssize_t fake_eng_loop13_store(struct device_driver *drv,
				     const char *buf,
				     size_t count)
{
	if (kstrtou32(buf, 0, &fake_eng13_loop_mode) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");
	else
		pr_info("[MTK_BW]fake engine loop mode= %u\n",
					fake_eng13_loop_mode);

	return count;
}

static ssize_t fake_engine13_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "Fake engine 13: ");

	if (fake_eng13_result <= NO_RESULT)
		putbuf(buf, &len, "NO RESULT\n");
	else if (fake_eng13_result == FAIL)
		putbuf(buf, &len, "\033[31mFAIL\033[0m\n");
	else
		putbuf(buf, &len, "\033[32mPASS\033[0m\n");

	return len;
}

static ssize_t fake_engine13_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	u64 start_addr;
	u64 size;
	u32 type;
	u16 pat;

	if (sscanf(buf, "%llx %llx %u %hx", &start_addr, &size, &type, &pat) == 4) {
		pr_info("start = %llx, size = %llx, type = %u, pattern = %x\n",
				start_addr, size, type, pat);
		if (size > 0x100000000) {
			pr_err("[MTK_BW]Wrong size > 4GB\n");
			return count;
		}
		if (type != READ_ONLY && type != WRITE_ONLY && type != RW) {
			pr_err("[MTK_BW]Wrong type, 0:Read 1:Write 2:R/W\n");
			return count;
		}
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	fake_eng13_result = fake_engine_test(start_addr,
					     size,
					     pat,
					     type,
					     13,
					     &fake_eng13_loop_mode);
	if (fake_eng13_result < 0)
		pr_err("[MTK_BW]Fail to start fake engine!!\n");

	return count;
}

static ssize_t fake_eng_loop_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	u32 i;
	u32 id;
	u32 loop;

	if (sscanf(buf, "%u %u", &id, &loop) == 2)
		for (i = 0; i < fake_eng_num; i++)
			if (fake_eng_list[i] == id)
				goto set_loop_mode;

	pr_err("[MTK_BW]Wrong Argument!!\n");
	return count;

set_loop_mode:
	fake_eng_loop[i] = loop;
	pr_info("[MTK_BW]fake engine %d loop %s\n", id, (fake_eng_loop[i] ? "on" : "off"));
	fake_engine_loop(id, &fake_eng_loop[i]);
	return count;
}

static ssize_t fake_engine_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;
	u32 i;

	for (i = 0; i < fake_eng_num; i++) {
		putbuf(buf, &len, "Fake engine %d: ", fake_eng_list[i]);

		if (fake_eng_result[i] <= NO_RESULT)
			putbuf(buf, &len, "NO RESULT\n");
		else if (fake_eng_result[i] == FAIL)
			putbuf(buf, &len, "\033[31mFAIL\033[0m\n");
		else
			putbuf(buf, &len, "\033[32mPASS\033[0m\n");
	}

	return len;
}

static ssize_t fake_engine_store(struct device_driver *drv,
				 const char *buf,
				 size_t count)
{
	u32 i;
	u32 id;
	u64 start_addr;
	u64 size;
	u32 type;
	u16 pat;

	if (sscanf(buf, "%u %llx %llx %u %hx", &id, &start_addr, &size, &type, &pat) == 5) {
		pr_info("id = %u, start = %llx, size = %llx, type = %u, pattern = %hx\n",
			id, start_addr, size, type, pat);
		if (size > 0x100000000) {
			pr_err("[MTK_BW]Wrong size > 4GB\n");
			return count;
		}
		if (type != READ_ONLY && type != WRITE_ONLY && type != RW) {
			pr_err("[MTK_BW]Wrong type, 0:Read 1:Write 2:R/W\n");
			return count;
		}
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}
	for (i = 0; i < fake_eng_num; i++)
		if (fake_eng_list[i] == id)
			break;

	if (i == fake_eng_num)
		return count;

	fake_eng_result[i] = fake_engine_test(start_addr,
					      size,
					      pat,
					      type,
					      id,
					      &fake_eng_loop[i]);
	if (fake_eng_result[i] < 0)
		pr_err("[MTK_BW]Fail to start fake engine!!\n");

	return count;
}

static ssize_t bw_scomm_store(struct device_driver *drv,
			      const char *buf,
			      size_t count)
{
	if (kstrtou32(buf, 0, &scomm_mon_cfg.t_ms) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");
	else
		pr_info("[MTK_BW]Set Monitor Time= %d\n",
					scomm_mon_cfg.t_ms);

	return count;
}

static ssize_t bw_emi_store(struct device_driver *drv,
			    const char *buf,
			    size_t count)
{
	u32 t_ms;

	if (kstrtou32(buf, 0, &t_ms) != 0) {
		pr_info("[MTK_BW]Wrong Argument!!\n");
		return count;
	} else if (!t_ms) {
		pr_info("[MTK_BW]time can not be 0\n");
		return count;
	}

	emi_mon_cfg.t_ms = t_ms;
	pr_info("[MTK_BW]Set Monitor Time= %d\n", emi_mon_cfg.t_ms);
	return count;
}

static ssize_t bw_emi_detail_store(struct device_driver *drv,
				   const char *buf,
				   size_t count)
{
	bool emi_detail;

	if (kstrtobool(buf, &emi_detail) != 0) {
		pr_info("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	emi_mon_cfg.emi_detail = emi_detail;
	if (emi_mon_cfg.emi_detail)
		pr_info("[MTK_BW]Monitor emi bw info in detail\n");
	else
		pr_info("[MTK_BW]Monitor emi bw info in total\n");

	return count;
}

static int emi_lmt_wrap(struct bw_cfg *cfg,
			unsigned int broadcast,
			unsigned int port,
			unsigned int bw_lmt)
{
	u32 value;
	void __iomem *base;

	if (emi_set_base(cfg) < 0)
		return -ENXIO;

	if (broadcast == emi_cnt) {
		base = emi_get_base(cfg, EMI_0);
		enable_broadcast_mode(true);
	} else {
		base = emi_get_base(cfg, broadcast);
		enable_broadcast_mode(false);
	}

	value = (READ_REG(EMI_ARB(port)) & ~EMI_BW_LMT_MASK);
	value |= (bw_lmt & EMI_BW_LMT_MASK);
	WRITE_REG(value, EMI_ARB(port));

	wmb(); /* make sure settings are written */
	enable_broadcast_mode(false);

	emi_release_base(cfg);

	return 0;
}

static int emi_timer_wrap(struct bw_cfg *cfg,
			  unsigned int rw_flag,
			  unsigned int broadcast,
			  unsigned int port,
			  unsigned int timer)
{
	u32 value;
	void __iomem *base;

	if (emi_set_base(cfg) < 0)
		return -ENXIO;

	if (broadcast == emi_cnt) {
		base = emi_get_base(cfg, EMI_0);
		enable_broadcast_mode(true);
	} else {
		base = emi_get_base(cfg, broadcast);
		enable_broadcast_mode(false);
	}

	if (rw_flag == EMI_R_CONF) {
		value = (READ_REG(EMI_ARB(port)) & ~EMI_R_TIMER_MASK);
		value |= EMI_SET_RD_TIMER(timer);
	} else if (rw_flag == EMI_W_CONF) {
		value = (READ_REG(EMI_ARB(port)) & ~EMI_W_TIMER_MASK);
		value |= EMI_SET_WR_TIMER(timer);
	} else {
		emi_release_base(cfg);
		return 0;
	}

	WRITE_REG(value, EMI_ARB(port));

	wmb(); /* make sure settings are written */
	enable_broadcast_mode(false);

	emi_release_base(cfg);

	return 0;
}

static int emi_ostd_wrap(struct bw_cfg *cfg,
			 unsigned int rw_flag,
			 unsigned int broadcast,
			 unsigned int port,
			 unsigned int ostd)
{
	void __iomem *base;
	u32 val_h, val_l;
	u32 t_val_h, t_val_l;
	u32 offset;

	if (emi_set_base(cfg) < 0)
		return -ENXIO;

	if (broadcast == emi_cnt) {
		base = emi_get_base(cfg, EMI_0);
		enable_broadcast_mode(true);
	} else {
		base = emi_get_base(cfg, broadcast);
		enable_broadcast_mode(false);
	}

	t_val_h = READ_REG(EMI_OSTD_H(port));
	t_val_l = READ_REG(EMI_OSTD_L(port));
	val_h = EMI_OSTD_L_VAL(ostd);
	val_l = EMI_OSTD_H_VAL(ostd);

	offset = EMI_OSTD_PORT_OFFSET(port, rw_flag);
	t_val_h &= (~EMI_OSTD_MASK(offset));
	t_val_h |= (val_h << offset);
	t_val_l &= (~EMI_OSTD_MASK(offset));
	t_val_l |= (val_l << offset);

	WRITE_REG(t_val_h, EMI_OSTD_H(port));
	WRITE_REG(t_val_l, EMI_OSTD_L(port));

	wmb(); /* make sure settings are written */
	enable_broadcast_mode(false);

	emi_release_base(cfg);

	return 0;
}

static ssize_t bw_emi_lmt_store(struct device_driver *drv,
				const char *buf,
				size_t count)
{
	u32 port;
	u32 bw_lmt;
	u32 broadcast;
	struct bw_cfg *cfg = NULL;

	if (sscanf(buf, "%d %d %x", &broadcast, &port, &bw_lmt) < EMI_BW_LMT_PARA)
		goto RET_FUNC;

	/* allocate buffer */
	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		goto RET_FUNC;

	if (port >= emi_port_cnt || broadcast > emi_cnt) {
		pr_info("Wrong Argument!!\n");
		goto RET_FUNC;
	}

	if (emi_lmt_wrap(cfg, broadcast, port, bw_lmt) < 0)
		pr_info("Something Wrong!!\n");

RET_FUNC:
	kfree(cfg);
	return count;
}

static ssize_t bw_emi_timer_store(struct device_driver *drv,
				  const char *buf,
				  size_t count)
{
	u32 rw_flag;
	u32 port;
	u32 timer;
	u32 broadcast;
	struct bw_cfg *cfg = NULL;

	if (sscanf(buf, "%d %d %d %x", &broadcast, &port, &rw_flag, &timer) < EMI_TIMER_PARA)
		goto RET_FUNC;

	/* allocate buffer */
	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		goto RET_FUNC;

	if (port >= emi_port_cnt || broadcast > emi_cnt) {
		pr_info("Wrong Argument!!\n");
		goto RET_FUNC;
	}

	if (emi_timer_wrap(cfg, rw_flag, broadcast, port, timer) < 0)
		pr_info("Something Wrong!!\n");

RET_FUNC:
	kfree(cfg);
	return count;
}

static ssize_t bw_emi_ostd_store(struct device_driver *drv,
				 const char *buf,
				 size_t count)
{
	u32 rw_flag;
	u32 port;
	u32 ostd;
	u32 broadcast;
	struct bw_cfg *cfg = NULL;

	if (sscanf(buf, "%d %d %d %x", &broadcast, &port, &rw_flag, &ostd) < EMI_OSTD_PARA)
		goto RET_FUNC;

	/* allocate buffer */
	cfg = kmalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		goto RET_FUNC;

	if (port >= emi_port_cnt || broadcast > emi_cnt) {
		pr_info("Wrong Argument!!\n");
		goto RET_FUNC;
	}

	if (emi_ostd_wrap(cfg, rw_flag, broadcast, port, ostd) < 0)
		pr_info("Something Wrong!!\n");

RET_FUNC:
	kfree(cfg);
	return count;
}

static ssize_t bw_emi_lmt_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "echo [EMI] [PORT] [BW LMT] > bw_emi_lmt\n");
	putbuf(buf, &len, "[EMI] : Set which EMI.\n");
	putbuf(buf, &len, "[PORT] : Set which port\n");
	putbuf(buf, &len, "[BW LMT] : Set bandwidth limitation value (HEX)\n");

	return len;
}

static ssize_t bw_emi_timer_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "echo [EMI] [PORT] [R/W] [TIMER] > bw_emi_timer\n");
	putbuf(buf, &len, "[EMI] : Set which EMI.\n");
	putbuf(buf, &len, "[PORT] : Set which port\n");
	putbuf(buf, &len, "[R/W] : 0:Read; 1:Write\n");
	putbuf(buf, &len, "[TIMER] : Set count down timer (HEX)\n");

	return len;
}

static ssize_t bw_emi_ostd_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "echo [EMI] [PORT] [R/W] [OSTD] > bw_emi_ostd\n");
	putbuf(buf, &len, "[EMI] : Set which EMI.\n");
	putbuf(buf, &len, "[PORT] : Set which port\n");
	putbuf(buf, &len, "[R/W] : 0:Read; 1:Write\n");
	putbuf(buf, &len, "[OSTD] : Set out standing limit value (HEX)\n");

	return len;
}

static ssize_t bw_larb_store(struct device_driver *drv,
			     const char *buf,
			     size_t count)
{
	if (sscanf(buf, "%d %d", &larb_mon_cfg.m_id, &larb_mon_cfg.t_ms) == 2) {
		pr_info("[MTK_BW]Set Module ID = %d, Time= %d\n",
					larb_mon_cfg.m_id,
					larb_mon_cfg.t_ms);
		larb_mon_cfg.name = get_module_name_by_id(larb_mon_cfg.m_id);

	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
	}

	return count;
}

static ssize_t reg_op_store(struct device_driver *drv,
			    const char *buf,
			    size_t count)
{
	u32 value = 0;
	u32 element;
	u32 bit;
	u32 b_set;
	u32 reg_val;
	void __iomem *base;

	element = sscanf(buf, "%x %u %u", &dump_reg_addr, &bit, &b_set);
	if (element > MAX_REG_OP_ARG)
		return count;

	if (dump_reg_addr < 0x1c000000 || dump_reg_addr > 0x1ec00000) {
		pr_err("[MTK_BW]Wrong REG\n");
		return count;
	}

	if (element == 3) {
		if (bit >= 32) {
			pr_err("[MTK_BW]Wong bit\n");
			return count;
		}
		if (b_set != 0 && b_set != 1) {
			pr_err("[MTK_BW]Wrong bit value\n");
			return count;
		}
		pr_info("reg = 0x%x, BIT(%u) = %u", dump_reg_addr, bit, b_set);
		base = ioremap(dump_reg_addr, 0x4);
		reg_val = READ_REG(0);
		if (b_set == 0)
			reg_val &= ~(1 << bit);
		else
			reg_val |= (1 << bit);

		WRITE_REG(reg_val, 0);

	} else if (element == 2) {
		if (sscanf(buf, "%x %x", &dump_reg_addr, &value) == 2) {
			pr_info("[MTK_BW]register = 0x%x", dump_reg_addr);
			pr_info("[MTK_BW]value = %x", value);
			base = ioremap(dump_reg_addr, 0x4);
			WRITE_REG(value, 0);
		}
	} else if (element == 1) {
		if (kstrtou32(buf, 0, &dump_reg_addr) != 0)
			pr_err("[MTK_BW]Wrong Argument!!");
		else
			pr_info("[MTK_BW]register = 0x%x", dump_reg_addr);

	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
		return count;
	}

	return count;
}

static ssize_t reg_op_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;
	u32 value;
	int i;
	void __iomem *base;

	if (dump_reg_addr < REG_START_ADDR || dump_reg_addr > REG_END_ADDR) {
		pr_err("[MTK_BW]Wrong REG\n");
		return len;
	}

	base = ioremap(dump_reg_addr, REG_OP_SIZE);
	if (!base)
		return len;

	value = READ_REG(0);

	putbuf(buf, &len, "\033[K\n");
	putbuf(buf, &len, "0x%x = 0x%x\033[K\n", dump_reg_addr, value);

	putbuf(buf, &len, "\033[4m%81s\033[0m\033[K\n", "");
	putbuf(buf, &len, "\033[7m| 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 ");
	putbuf(buf, &len, "| 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 |\033[0m");

	putbuf(buf, &len, "\033[0m\033[K\n\033[4m");

	for (i = REG_HIGH_16BIT_MAX; i >= REG_HIGH_16BIT_MIN; i--)
		putbuf(buf, &len, "|%3u ", (READ_REG(0)&BIT(i)) ? 1:0);

	putbuf(buf, &len, "|\033[0m\033[K\n");

	putbuf(buf, &len, "\033[K\n");

	putbuf(buf, &len, "\033[4m%81s\033[0m\033[K\n", "");
	putbuf(buf, &len, "\033[7m| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 ");
	putbuf(buf, &len, "|  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |\033[0m");
	putbuf(buf, &len, "\033[0m\033[K\n\033[4m");

	for (i = REG_LOW_16BIT_MAX; i >= REG_LOW_16BIT_MIN; i--)
		putbuf(buf, &len, "|%3u ", (READ_REG(0)&BIT(i)) ? 1:0);

	putbuf(buf, &len, "|\033[0m\033[K\n");
	putbuf(buf, &len, "\033[K\n\033[K\n");

	return len;
}

static ssize_t cpupri_slot_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;
	void __iomem *base;

	base = find_device_addr(CPU_PRI_DRV, 0);
	if (!base)
		return len;

	if (!(READ_REG(REG_CPU_PRI_EN) & BIT(CPU_PRI_S0_EN)))
		putbuf(buf, &len, "0\n");
	else if (!(READ_REG(REG_CPU_PRI_EN) & BIT(CPU_PRI_S1_EN)))
		putbuf(buf, &len, "1\n");
	else if (!(READ_REG(REG_CPU_PRI_EN) & BIT(CPU_PRI_S2_EN)))
		putbuf(buf, &len, "2\n");
	else
		putbuf(buf, &len, "-1\n");

	if (base)
		iounmap(base);

	return len;
}

static int mtk_cpupri_check_para(int flag,
				 int scope,
				 unsigned long long saddr,
				 unsigned long long addrlen)
{
	if (scope < 0 || scope > CPU_PRI_SCOPE_NUM)
		return -1;
	if (flag == CPU_PRI_NORMAL)
		return 0;
	if (flag != CPU_PRI_PREULTRA && flag != CPU_PRI_ULTRA)
		return -1;
	if (addrlen < CPU_PRI_ADDR_ALIGN ||
	!IS_ALIGNED(saddr, CPU_PRI_ADDR_ALIGN) ||
	!IS_ALIGNED(addrlen, CPU_PRI_ADDR_ALIGN))
		return -1;

	return 0;
}

static void mtk_cpupri_set(void __iomem *base,
			   int flag,
			   int scope,
			   unsigned long long saddr,
			   unsigned long long addrlen)
{
	/* set up start address */
	u32 set_saddr_l = ((saddr / CPU_PRI_ADDR_ALIGN) & LOW_16BIT);
	u32 set_saddr_h = ((RSHIFT_16BIT(saddr / CPU_PRI_ADDR_ALIGN)) & LOW_16BIT);

	/* set up end address */
	u32 set_eaddr_l =
	((((saddr + addrlen) / CPU_PRI_ADDR_ALIGN) - 1) & LOW_16BIT);
	u32 set_eaddr_h =
	((RSHIFT_16BIT(((saddr + addrlen) / CPU_PRI_ADDR_ALIGN) - 1)) & LOW_16BIT);

	WRITE_REG(set_saddr_l, REG_CPRI_SCOPE(REG_CPU_PRI_S0_SADDR_L, scope));
	WRITE_REG(set_saddr_h, REG_CPRI_SCOPE(REG_CPU_PRI_S0_SADDR_H, scope));
	WRITE_REG(set_eaddr_l, REG_CPRI_SCOPE(REG_CPU_PRI_S0_EADDR_L, scope));
	WRITE_REG(set_eaddr_h, REG_CPRI_SCOPE(REG_CPU_PRI_S0_EADDR_H, scope));

	if (flag == CPU_PRI_PREULTRA) {
		__set_bit(CPU_PRI_PREULTRA_EN, (base + REG_CPU_PRI_EN));
		__clear_bit(CPU_PRI_ULTRA_EN, (base + REG_CPU_PRI_EN));
	} else if (flag == CPU_PRI_ULTRA) {
		__set_bit(CPU_PRI_ULTRA_EN, (base + REG_CPU_PRI_EN));
		__clear_bit(CPU_PRI_PREULTRA_EN, (base + REG_CPU_PRI_EN));
	}
	__set_bit((CPU_PRI_S0_EN + scope), (base + REG_CPU_PRI_EN));
}

static ssize_t cpupri_store(struct device_driver *drv,
			    const char *buf,
			    size_t count)
{
	int flag = 0;
	int scope = -1;
	u64 saddr = 0x0;
	u64 addrlen = 0x0;
	void __iomem *base;

	base = find_device_addr(CPU_PRI_DRV, 0);
	if (!base)
		return -1;
	if (sscanf(buf, "%d %d 0x%llx 0x%llx", &flag,
					       &scope,
					       &saddr,
					       &addrlen) < 2)
		return -1;
	if (mtk_cpupri_check_para(flag, scope, saddr, addrlen) < 0)
		return -1;

	if (flag == CPU_PRI_NORMAL)
		__clear_bit((CPU_PRI_S0_EN + scope), (base + REG_CPU_PRI_EN));
	else
		mtk_cpupri_set(base, flag, scope, saddr, addrlen);

	if (base)
		iounmap(base);

	return count;
}

static ssize_t cpupri_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;
	u32 idx = 0;
	void __iomem *base;
	u32 tmp1 = 0, tmp2 = 0, tmp3 = 0, tmp4 = 0;
	u64 saddr = 0, eaddr = 0;

	base = find_device_addr(CPU_PRI_DRV, 0);
	for (idx = 0; idx < CPU_PRI_SCOPE_NUM; idx++) {
		if (!(READ_REG(REG_CPU_PRI_EN) & BIT(CPU_PRI_S0_EN + idx)))
			continue;
		tmp1 = READ_REG_16(REG_CPRI_SCOPE(REG_CPU_PRI_S0_SADDR_L, idx));
		tmp2 = READ_REG_16(REG_CPRI_SCOPE(REG_CPU_PRI_S0_SADDR_H, idx));
		tmp3 = READ_REG_16(REG_CPRI_SCOPE(REG_CPU_PRI_S0_EADDR_L, idx));
		tmp4 = READ_REG_16(REG_CPRI_SCOPE(REG_CPU_PRI_S0_EADDR_H, idx));
		saddr = ((u64)tmp1 | LSHIFT_16BIT((u64)tmp2)) * CPU_PRI_ADDR_ALIGN;
		eaddr = (((u64)tmp3 | LSHIFT_16BIT((u64)tmp4)) + 1) * CPU_PRI_ADDR_ALIGN - 1;
		putbuf(buf,
		       &len,
		       "scope:%d - start addr:0x%llx, end addr:0x%llx\n",
		       idx,
		       saddr,
		       eaddr);
	}
	if (base)
		iounmap(base);

	return len;
}

static ssize_t debug_log_on_store(struct device_driver *drv,
				  const char *buf,
				  size_t count)
{
	if (kstrtou32(buf, 0, &debug_on) != 0)
		pr_err("[MTK_BW]Wrong Argument!!\n");

	return count;
}

static ssize_t debug_log_on_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;

	putbuf(buf, &len, "debug_log_on: %u\n", debug_on);

	return len;
}

static DRIVER_ATTR_RW(bw_total);
static DRIVER_ATTR_RW(bw_larb);
static DRIVER_ATTR_RW(bw_scomm);
static DRIVER_ATTR_RW(bw_emi);
static DRIVER_ATTR_RO(bw_bus);
static DRIVER_ATTR_WO(bw_emi_detail);
static DRIVER_ATTR_RW(bw_emi_lmt);
static DRIVER_ATTR_RW(bw_emi_timer);
static DRIVER_ATTR_RW(bw_emi_ostd);
static DRIVER_ATTR_WO(larb_config);
static DRIVER_ATTR_RO(ip_config);
static DRIVER_ATTR_RW(fake_engine);
static DRIVER_ATTR_WO(fake_eng_loop);
static DRIVER_ATTR_RW(fake_engine13);
static DRIVER_ATTR_WO(fake_eng_loop13);
static DRIVER_ATTR_RW(fake_engine19);
static DRIVER_ATTR_WO(fake_eng_loop19);
static DRIVER_ATTR_RW(fake_engine21);
static DRIVER_ATTR_WO(fake_eng_loop21);
static DRIVER_ATTR_RW(fake_engine29);
static DRIVER_ATTR_WO(fake_eng_loop29);
static DRIVER_ATTR_RW(fake_engine44);
static DRIVER_ATTR_WO(fake_eng_loop44);
static DRIVER_ATTR_RW(fake_engine52);
static DRIVER_ATTR_WO(fake_eng_loop52);
static DRIVER_ATTR_RW(reg_op);
static DRIVER_ATTR_RW(debug_log_on);
static DRIVER_ATTR_RO(cpupri_slot);
static DRIVER_ATTR_RW(cpupri);
static DRIVER_ATTR_WO(mask);

static int get_lc_num(struct device_node *np, int get_type)
{
	struct device_node *cnp = NULL;
	struct device_node *lnp = NULL;
	char node_name[NODE_NAME_SIZE];
	int ret = 0;
	int num;

	if (get_type == GET_LARB_NUM)
		num = snprintf(node_name, sizeof(node_name), "larb-id");
	else if (get_type == GET_COMM_NUM)
		num = snprintf(node_name, sizeof(node_name), "common-id");
	else
		return ret;

	if (num >= NODE_NAME_SIZE)
		return ret;

	if (of_property_read_bool(np, "mix-module")) {
		for_each_available_child_of_node(np, cnp) {
			for_each_available_child_of_node(cnp, lnp) {
				if (of_property_read_bool(lnp, node_name))
					ret++;
			}
		}
	} else {
		for_each_available_child_of_node(np, cnp) {
			if (of_property_read_bool(cnp, node_name))
				ret++;
		}
	}
	return ret;
}

static int get_port(struct device_node *np, u32 mod_idx, u32 node_idx)
{

	pr_notice("[MTK-BW] add node: %s \033[K\n", np->name);

	if (of_property_read_bool(np, "larb-id"))
		goto GET_LARB;
	else if (of_property_read_bool(np, "common-id"))
		goto GET_COMMON;
	else
		return 0;

GET_LARB:
	if (!mlist[mod_idx].llist || !mlist[mod_idx].plist)
		return 0;
	of_property_read_u32(np, "larb-id", &mlist[mod_idx].llist[node_idx]);
	mlist[mod_idx].plist[node_idx].pcount = of_property_count_u32_elems(np, "port-list");
	mlist[mod_idx].plist[node_idx].smi_plist = kcalloc(mlist[mod_idx].plist[node_idx].pcount,
							   sizeof(unsigned int), GFP_KERNEL);
	if (!mlist[mod_idx].plist[node_idx].smi_plist)
		return 0;
	of_property_read_u32_array(np,
				   "port-list",
				   mlist[mod_idx].plist[node_idx].smi_plist,
				   mlist[mod_idx].plist[node_idx].pcount);
	return 1;

GET_COMMON:
	if (!mlist[mod_idx].clist || !mlist[mod_idx].cplist)
		return 0;
	of_property_read_u32(np, "common-id", &mlist[mod_idx].clist[node_idx]);
	mlist[mod_idx].cplist[node_idx].pcount = of_property_count_u32_elems(np, "port-list");
	mlist[mod_idx].cplist[node_idx].com_plist = kcalloc(mlist[mod_idx].cplist[node_idx].pcount,
							    sizeof(unsigned int), GFP_KERNEL);
	if (!mlist[mod_idx].cplist[node_idx].com_plist)
		return 0;
	of_property_read_u32_array(np,
				   "port-list",
				   mlist[mod_idx].cplist[node_idx].com_plist,
				   mlist[mod_idx].cplist[node_idx].pcount);
	return 1;
}

static void setup_module_port_info(unsigned int mod_idx, struct device_node *np)
{
	struct device_node *cnp = NULL;
	struct device_node *lcnp = NULL;
	u32 larb_node_idx = 0;
	u32 common_node_idx = 0;

	for_each_available_child_of_node(np, cnp) {
		if (of_property_read_bool(np, "mix-module")) {
			for_each_available_child_of_node(cnp, lcnp) {
				if (of_property_read_bool(lcnp, "larb-id")) {
					if (get_port(lcnp, mod_idx, larb_node_idx))
						larb_node_idx++;
				} else if (of_property_read_bool(lcnp, "common-id")) {
					if (get_port(lcnp, mod_idx, common_node_idx))
						common_node_idx++;
				} else {
					pr_err("[MTK_BW] error to parse larb/common id\n");
				}
			}
		} else {
			if (of_property_read_bool(cnp, "larb-id")) {
				if (get_port(cnp, mod_idx, larb_node_idx)) {
					if (strncmp(np->name, "bw_bist", sizeof("bw_bist")) == 0) {
						fake_eng_list[larb_node_idx] =
						mlist[mod_idx].llist[larb_node_idx];
						fake_eng_result[larb_node_idx] = NO_RESULT;
						fake_eng_loop[larb_node_idx] = 0;
					}
					larb_node_idx++;
				}
			} else if (of_property_read_bool(cnp, "common-id")) {
				if (get_port(cnp, mod_idx, common_node_idx))
					common_node_idx++;
			}
		}
	}
}

static void init_mlist_array(u32 mod_idx)
{
	mlist[mod_idx].llist = kcalloc(mlist[mod_idx].larb_num,
				       sizeof(unsigned int), GFP_KERNEL);
	mlist[mod_idx].plist = kcalloc(mlist[mod_idx].larb_num,
				       sizeof(struct larb_config), GFP_KERNEL);
	mlist[mod_idx].clist = kcalloc(mlist[mod_idx].common_num,
				       sizeof(unsigned int), GFP_KERNEL);
	mlist[mod_idx].cplist = kcalloc(mlist[mod_idx].common_num,
					sizeof(struct common_config), GFP_KERNEL);
}

static int set_module_list(void)
{
	struct device_node *np = NULL;
	u32 mod_idx = 0;
	int err = 0;

	/* get all module number */
	module_cnt = 0;
	for_each_node_by_type(np, "bw")
		module_cnt++;

	if (module_cnt == 0) {
		pr_err("[MTK_BW]no module list\n");
		return -1;
	}

	mlist = kcalloc(module_cnt, sizeof(struct module_map), GFP_KERNEL);

	/* parsing all module */
	fake_eng_loop_mode = SW_LOOP;
	for_each_node_by_type(np, "bw") {
		if (of_property_read_string(np, "module-name", &mlist[mod_idx].name) < 0)
			return -1;
		if (of_property_read_string(np, "domain", &mlist[mod_idx].domain) < 0)
			return -1;
		if (of_property_read_u32(np, "module-id", &mlist[mod_idx].module_id) < 0)
			return -1;
		mlist[mod_idx].enable = of_property_read_bool(np, "init-on");
		mlist[mod_idx].larb_num = get_lc_num(np, GET_LARB_NUM);
		mlist[mod_idx].common_num = get_lc_num(np, GET_COMM_NUM);
		if (of_property_read_bool(np, "hw-loop"))
			fake_eng_loop_mode = HW_LOOP;
		if (strncmp(mlist[mod_idx].name, "BIST", sizeof("BIST")) == 0) {
			if (of_property_read_u32(np, "alignment", &fake_eng_st_align) < 0)
				return -1;
			fake_eng_num = mlist[mod_idx].larb_num;
			if (fake_eng_num > 0) {
				fake_eng_list = kcalloc(fake_eng_num,
					sizeof(u32), GFP_KERNEL);
				fake_eng_result = kcalloc(fake_eng_num,
					sizeof(u32), GFP_KERNEL);
				fake_eng_loop = kcalloc(fake_eng_num,
					sizeof(u32), GFP_KERNEL);
				if (!fake_eng_list || !fake_eng_result || !fake_eng_loop)
					return -1;
			}
		}


		init_mlist_array(mod_idx);

		setup_module_port_info(mod_idx, np);

		mod_idx++;
	}

	err = get_module_id_by_name("REF_EMI", &ref_emi_id);
	err |= get_module_id_by_name("SUM", &sum_id);
	if (err)
		pr_err("[MTK_BW]get_module_id_by_name fail\n");

	return 0;
}

static void remove_module_list(void)
{
	int mod_idx, node_idx;

	for (mod_idx = 0; mod_idx < module_cnt; mod_idx++) {
		for (node_idx = 0; node_idx < mlist[mod_idx].larb_num; node_idx++)
			kfree(mlist[mod_idx].plist[node_idx].smi_plist);
		for (node_idx = 0; node_idx < mlist[mod_idx].common_num; node_idx++)
			kfree(mlist[mod_idx].cplist[node_idx].com_plist);
		kfree(mlist[mod_idx].llist);
		kfree(mlist[mod_idx].plist);
		kfree(mlist[mod_idx].clist);
		kfree(mlist[mod_idx].cplist);
	}
	kfree(mlist);
}

static int create_bw_related_node(struct device_driver *drv)
{
	int err = 0;

	err = driver_create_file(drv, &driver_attr_ip_config);
	if (err) {
		pr_err("[MTK_BW]ip_config create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_total);
	if (err) {
		pr_err("[MTK_BW]bw_total create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_larb);
	if (err) {
		pr_err("[MTK_BW]bw_larb create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_scomm);
	if (err) {
		pr_err("[MTK_BW]bw_scomm create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_emi);
	if (err) {
		pr_notice("[MTK_BW]bw_emi create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_bus);
	if (err) {
		pr_notice("[MTK_BW]bw_bus create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_emi_detail);
	if (err) {
		pr_notice("[MTK_BW]bw_emi_detail create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_emi_lmt);
	if (err) {
		pr_notice("[MTK_BW]bw_emi_lmt create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_emi_timer);
	if (err) {
		pr_notice("[MTK_BW]bw_emi_timer create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_emi_ostd);
	if (err) {
		pr_notice("[MTK_BW]bw_emi_ostd create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_larb_config);
	if (err) {
		pr_err("[MTK_BW]larb_config create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_cpupri_slot);
	if (err) {
		pr_err("[MTK_BW]cpupri_slot create attribute error\n");
		return err;
	}
	driver_attr_cpupri.attr.mode = (0664);
	err = driver_create_file(drv, &driver_attr_cpupri);
	if (err) {
		pr_err("[MTK_BW]cpupri create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_mask);
	if (err) {
		pr_err("[MTK_BW]mask create attribute error\n");
		return err;
	}

	return err;
}

static int create_fake_engine_node(struct device_driver *drv)
{
	int err = 0;

	err = driver_create_file(drv, &driver_attr_fake_engine13);
	if (err) {
		pr_err("[MTK_BW]fake_engine13 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_eng_loop13);
	if (err) {
		pr_err("[MTK_BW]fake_eng_loop13 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_engine19);
	if (err) {
		pr_err("[MTK_BW]fake_engine19 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_eng_loop19);
	if (err) {
		pr_err("[MTK_BW]fake_eng_loop19 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_engine21);
	if (err) {
		pr_err("[MTK_BW]fake_engine21 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_eng_loop21);
	if (err) {
		pr_err("[MTK_BW]fake_eng_loop21 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_engine29);
	if (err) {
		pr_err("[MTK_BW]fake_engine29 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_eng_loop29);
	if (err) {
		pr_err("[MTK_BW]fake_eng_loop29 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_engine44);
	if (err) {
		pr_err("[MTK_BW]fake_engine44 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_eng_loop44);
	if (err) {
		pr_err("[MTK_BW]fake_eng_loop44 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_engine52);
	if (err) {
		pr_err("[MTK_BW]fake_engine52 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_eng_loop52);
	if (err) {
		pr_err("[MTK_BW]fake_eng_loop52 create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_engine);
	if (err) {
		pr_err("[MTK_BW]fake_engine create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_fake_eng_loop);
	if (err) {
		pr_err("[MTK_BW]fake_eng_loop create attribute error\n");
		return err;
	}

	return err;
}

static int create_sysfs_node(void)
{
	struct device_driver *drv;
	int err = 0;

	drv = driver_find("mtk-bw", &platform_bus_type);

	err = driver_create_file(drv, &driver_attr_debug_log_on);
	if (err) {
		pr_err("[MTK_BW]debug_log_on create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_reg_op);
	if (err) {
		pr_err("[MTK_BW]regop create attribute error\n");
		return err;
	}
	err = create_fake_engine_node(drv);
	if (err) {
		pr_err("[MTK_BW]create_fake_engine_node fail\n");
		return err;
	}
	err = create_bw_related_node(drv);
	if (err) {
		pr_err("[MTK_BW]create_bw_related_node fail\n");
		return err;
	}

	return err;
}

static int set_bw_monitor_default_cfg(void)
{
	int err = 0;

	larb_mon_cfg = (struct bw_cfg) {
		.name = "GOP",
		.t_ms = DEFAULT_MON_TIME,
	};

	err = get_module_id_by_name("GOP", &larb_mon_cfg.m_id);

	total_mon_cfg = (struct bw_cfg) {
		.t_ms = DEFAULT_MON_TIME,
		.proc_mon = DISABLE,
	};

	scomm_mon_cfg = (struct bw_cfg) {
		.t_ms = DEFAULT_MON_TIME,
	};

	emi_mon_cfg = (struct bw_cfg) {
		.t_ms = DEFAULT_MON_TIME,
		.emi_detail = false,
	};

	return err;
}

static u32 get_dram_clock(int chip)
{
	u32 ret = clk_2666;
	void __iomem *base;
	u32 check_bond, ext_mcp, clk;

	base = ioremap((REG_START_ADDR + CHECK_BOND_BASE), CHECK_BOND_SIZE);
	if (!base)
		goto RET_FUNC;

	check_bond = READ_REG(0);
	ext_mcp = CHK_BOND_EXT_MCP(check_bond);
	clk = CHK_BOND_EXT_MCP(check_bond);

	switch(chip) {
	case mt5879:
		if (ext_mcp == CHK_BOND_EXT) {
			ret = clk_3200;
		} else if (ext_mcp == CHK_BOND_MCP_AND_EXT){
			if (clk == CHK_BOND_3200)
				ret = clk_3200;
			else
				ret = clk_2933;
		} else if (ext_mcp == CHK_BOND_MCP){
			if (clk == CHK_BOND_3200)
				ret = clk_3200;
			else
				ret = clk_2666;
		}
		break;
	default:
		break;
	}

RET_FUNC:
	iounmap(base);
	return ret;
}

static int mtk_bw_setup_dram_config(void)
{
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "dram_config");
	if (!np)
		return -1;

	if (of_property_read_u32(np, "bus_width", &bus_width) < 0)
		return -1;

	if (of_property_read_u32(np, "dram_clock", &dram_clock) < 0)
		return -1;

	if (of_property_read_u32(np, "dram_penalty", &dram_penalty) < 0)
		return -1;

	if (of_property_read_u32(np, "emi_clock", &emi_clock) < 0)
		return -1;

	if (of_property_read_u32(np, "smi_clock", &smi_clock) < 0)
		return -1;

	/* calculate total bandwidth */
	total_bw = CAL_TOTAL_BW(bus_width, dram_clock);

	return 0;
}

static int mtk_bw_setup_emi_arch_config(void)
{
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "emi_arch");
	if (!np)
		return -1;

	if (of_property_read_u32(np, "emi_cnt", &emi_cnt) < 0)
		return -1;

	if (of_property_read_u32(np, "emi_port_cnt", &emi_port_cnt) < 0)
		return -1;

	if (of_property_read_u32(np, "smi_common_cnt", &smi_common_cnt) < 0)
		return -1;

	emi_effi_mon = of_property_read_bool(np, "emi-effi-monitor");

	return 0;
}

static int mtk_bw_measure_init(void)
{
	struct device_driver *drv;
	int err = 0;

	drv = driver_find("mtk-bw", &platform_bus_type);

	err = set_module_list();
	if (err) {
		pr_err("[MTK_BW]set_module_list error\n");
		return err;
	}
	err = set_bw_monitor_default_cfg();
	if (err) {
		pr_err("[MTK_BW]set set_bw_monitor_default_cfg error\n");
		return err;
	}

	return err;
}

static int get_effi_table_index(void)
{
	int ret = 0;

	if (emi_clock == MT5879_SMI_CLK) {
		dram_clock = get_dram_clock(mt5879);
		if (dram_clock == clk_3200)
			ret = mt5879_effi_pcddr4_3200;
		else if (dram_clock == clk_2933)
			ret = mt5879_effi_pcddr4_2933;
		else
			ret = mt5879_effi_pcddr4_2666;
	} else {
		ret = mt5876_effi_pcddr4_2666;
	}

	return ret;
}

static int mtk_effi_probe(struct platform_device *pdev)
{
	int i, effi_table_idx;
	u32 array_base;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct mtk_effi *effi_mon;

	effi_mon = devm_kzalloc(dev, sizeof(*effi_mon), GFP_KERNEL);
	if (!effi_mon)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	effi_mon->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(effi_mon->base))
		return PTR_ERR(effi_mon->base);

	effi_mon->data = of_device_get_match_data(dev);
	if (!effi_mon->data)
		return -EINVAL;

	/* set up effi table for all burst length */
	effi_table_idx = get_effi_table_index();
	if (effi_table_idx < 0) {
		pr_err("get effi table fail, no type\n");
		return 0;
	}

	array_base = effi_table_idx * EFFI_BURST_SET_CNT;
	for (i = 0; i < EFFI_BURST_SET_CNT; i++)
		writel(effi_mon->data[i + array_base], effi_mon->base + BURST_EFFI_TABLE(i));

	platform_set_drvdata(pdev, effi_mon);

	return 0;
}

static int __maybe_unused mtk_effi_suspend(struct device *dev)
{
	pr_info("[MTK_BW] MTK EFFI Suspend\n");

	return 0;
}

static int __maybe_unused mtk_effi_resume(struct device *dev)
{

	struct mtk_effi *effi_mon = dev_get_drvdata(dev);
	int i;

	pr_info("[MTK_BW] MTK EFFI Resume\n");
	/* set up effi table for all burst length */
	for (i = 0; i < EFFI_BURST_SET_CNT; i++)
		writel(effi_mon->data[i], effi_mon->base + BURST_EFFI_TABLE(i));

	return 0;
}

static int mtk_effi_remove(struct platform_device *pdev)
{
	pr_info("[MTK_BW] MTK EFFI Remove\n");

	return 0;
}

static const struct dev_pm_ops effi_pm_ops = {
	.suspend = mtk_effi_suspend,
	.resume = mtk_effi_resume,
};

static struct platform_driver mtk_effi_driver = {
	.probe	= mtk_effi_probe,
	.remove = mtk_effi_remove,
	.driver	= {
		.name = "mtk-effi",
		.of_match_table = mtk_effi_of_ids,
		.pm = &effi_pm_ops,
	}
};

static struct platform_driver mtk_bw_driver = {
	.driver	= {
		.name = "mtk-bw",
	}
};

static int __init mtk_bw_init(void)
{
	int ret;

	/* setup DRAM config */
	ret = mtk_bw_setup_dram_config();
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to init mtk_bw_setup_dram_config\n");
		return ret;
	}
	ret = platform_driver_register(&mtk_effi_driver);
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to register mtk_effi_monitor driver\n");
		return ret;
	}
	ret = platform_driver_register(&mtk_bw_driver);
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to register mtk_bw_monitor driver\n");
		return ret;
	}
	/* setup EMI arch config */
	ret = mtk_bw_setup_emi_arch_config();
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to init mtk_bw_setup_emi_arch_config\n");
		return ret;
	}
	/* initial module list */
	ret = mtk_bw_measure_init();
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to init mtk_bw_measure_init\n");
		return ret;
	}
	ret = create_sysfs_node();
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to create_sysfs_node\n");
		return ret;
	}
	return ret;
}

static void __exit mtk_bw_exit(void)
{
	remove_module_list();
	platform_driver_unregister(&mtk_effi_driver);
	platform_driver_unregister(&mtk_bw_driver);
}

module_init(mtk_bw_init);
module_exit(mtk_bw_exit);

MODULE_LICENSE("GPL");
