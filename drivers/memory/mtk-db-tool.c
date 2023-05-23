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
#include "mtk-db-tool.h"

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
static u32 g_larb_id;
static u32 g_larb_port_id;

/* bandwidth monitor config */
static struct bw_cfg larb_mon_cfg;
static struct bw_cfg total_mon_cfg;
static struct bw_cfg scomm_mon_cfg;
static struct bw_cfg emi_mon_cfg;

/* register operation */
//static u32 dump_reg_addr;

/* debug */
static u32 debug_on;

//#define BW_EMI_MONITOR
#ifdef BW_EMI_MONITOR
#undef BW_EMI_MONITOR
#endif
//#define BW_LARB_MONITOR
#ifdef BW_LARB_MONITOR
#undef BW_LARB_MONITOR
#endif

static const u32 mt5896_lpddr4_effi_table[EFFI_BURST_SET_CNT] = {
	0x200a, 0x402d, 0x5446, 0x5654, 0x5656, 0x5656, 0x5656, 0x5656,
};

static const struct of_device_id mtk_effi_of_ids[] = {
	{
		.compatible = "mediatek,mt5896-effi-mon",
		.data = mt5896_lpddr4_effi_table
	},
	{}
};

static const struct effi_mon_mapping effi_device_map[EFFI_MON_CNT] = {
	{SMI_COMMON(0), P0, EFFI_MON(0)},
	{SMI_COMMON(0), P1, EFFI_MON(1)},
	{SMI_COMMON(1), P0, EFFI_MON(2)},
	{SMI_COMMON(1), P1, EFFI_MON(3)},
	{SMI_COMMON(2), P0, EFFI_MON(4)},
	{SMI_COMMON(5), P0, EFFI_MON(5)},
	{SMI_COMMON(3), P0, EFFI_MON(6)},
	{SMI_COMMON(4), P0, EFFI_MON(7)},
	{SMI_COMMON(6), P0, EFFI_MON(8)},
	{SMI_COMMON(6), P1, EFFI_MON(9)},
	{SMI_COMMON(7), P0, EFFI_MON(10)},
	{SMI_COMMON(7), P1, EFFI_MON(11)},
	{SMI_COMMON(2), P1, EFFI_MON(12)},
	{SMI_COMMON(5), P1, EFFI_MON(13)},
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

#ifdef BW_LARB_MONITOR
static const char *get_module_name_by_id(int id)
{
	u32 m_idx;

	for (m_idx = 0; m_idx < module_cnt; m_idx++) {
		if (mlist[m_idx].module_id == id)
			return mlist[m_idx].name;
	}

	return NULL;
}
#endif

static void put_buf_all_module(struct bw_cfg *cfg,
			       struct bw_res *res[],
			       char *buf,
			       unsigned int *len)
{
	u32 m_idx;
	s64 slack_ratio;
	s64 slack;

	putbuf(buf, len, "\033[K\n");
	for (m_idx = 0; m_idx < module_cnt; m_idx++) {
		if (!mlist[m_idx].enable)
			continue;
		/* if proc_mon = 1, only monitor processor */
		if (mlist[m_idx].larb_num != 0 && total_mon_cfg.proc_mon == 1)
			continue;
		if (strncmp(mlist[m_idx].name, "REF_EMI", sizeof("REF_EMI")) == 0)
			continue;

		putbuf(buf, len, "%-10s: %6llu MB/s\033[K\n",
				mlist[m_idx].name,
				res[m_idx]->bus_bw);
	}

	putbuf(buf, len, "\033[K\n");

	slack = (s64)total_bw-(s64)res[sum_id]->bus_bw;
	slack_ratio = S_PERCENTAGE(slack, (s32)total_bw);

	putbuf(buf, len, "%-10s: %6u MB/s (%3u%%)\033[K\n",
			"Total",
			total_bw,
			100);
	putbuf(buf, len, "%-10s: %6llu MB/s (%3llu%%)\033[K\n",
			"Useage",
			res[sum_id]->bus_bw,
			PERCENTAGE(res[sum_id]->bus_bw, total_bw));
	putbuf(buf, len, "%-10s: %6lld MB/s (%3lld%%)\033[K\n",
			"Slack",
			slack,
			slack_ratio);

	putbuf(buf, len, "\033[K\n");
}

#ifdef BW_LARB_MONITOR
static void put_buf_larb(struct bw_cfg *cfg,
			 struct larb_res *res[],
			 char *buf,
			 unsigned int *len)
{
	u32 i, j;
	u64 module_bus_bw = 0;
	struct module_map m = mlist[cfg->m_id];

	if (m.larb_num == 0) {
		putbuf(buf, len, "%s not support larb monitor\033[K\n", m.name);
	} else {
		putbuf(buf, len, "\033[K\n");
		putbuf(buf, len, "\033[4mModule: ");
		putbuf(buf, len, "%s\033[0m\033[K\n", m.name);
		for (i = 0; i < m.larb_num; i++) {
			for (j = 0; j < m.plist[i].pcount; j++) {
				putbuf(buf, len, "-->SMI_LARB(%d)-Port(%d) = %6lu MB/s",
						m.llist[i],
						m.plist[i].smi_plist[j],
						res[i]->port_bus_bw[j]);
				putbuf(buf, len, ", ostdl = 0x%x",
						res[i]->ostdl[j]);
				putbuf(buf, len, "\033[K\n");
			}
			module_bus_bw += res[i]->larb_bus_bw;
		}
		putbuf(buf, len, "\033[K\n");
		putbuf(buf, len, "Total %s BW = %8llu MB/s\033[K\n", m.name, module_bus_bw);
		putbuf(buf, len, "\033[K\n");
	}
}
#endif

static void put_buf_emi_total(struct bw_cfg *cfg,
			      struct emi_res *res,
			      char *buf,
			      unsigned int *len)
{
	int i;
	u64 total_data_bw = 0, total_bus_bw = 0;

	putbuf(buf, len, "\033[K\n");
	for (i = 0; i <= emi_port_cnt; i++) {
		putbuf(buf, len, "-->EMI-Port(%d) bw_limit: 0x%lx \033[K\n",
				i, EMI_GET_BW_LMT(res[0].arb_info[i]));
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

	return 0;
}

#ifdef BW_LARB_MONITOR
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
#endif

static int get_larb_ostdl(u32 larb_id, u32 port_id, u32 *ostdl)
{
	void __iomem *base;

	base = find_device_addr(SMI_LARB_DRV, larb_id);
	if (!base) {
		pr_err("[MTK_BW]Can't find SMI LARB %u device base!!\n", larb_id);
		return -1;
	}

	*ostdl = READ_REG(SMI_LARB_OSTDL_PORTx(port_id)) & 0xff;

	return 0;
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

	if (type == SUMMARY_LARB_MODE || type == SUMMARY_COMMON_MODE) {
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

	if (type == SUMMARY_LARB_MODE || type == SUMMARY_COMMON_MODE) {
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

static void emi_detail_bw_measure(struct bw_cfg *cfg, struct emi_res *res)
{
	u32 i, j, k;
	u32 monitor_round, monitor_count;
	u32 st_port;
	u32 offset;
	u64 data_bw_t, latency_t;
	void __iomem *base;

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
					B_MS_TO_MB_S((EMI_BW_8BYTE(READ_REG(offset))) / cfg->t_ms);

					data_bw_t += res[k].data_bw[(j + st_port)];
				}
				res[emi_cnt].data_bw[(j + st_port)] = data_bw_t;
			}
			break;
		} while (1);
	}

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

static u32 effi_measure(u32 lc_id,
			u32 port_id,
			u32 type,
			void *res)
{
	struct device_node *larb_np, *scomm_np;
	void __iomem *base;
	char dev_name[15];
	int ret;
	u32 effi_output;
	u32 i;
	u32 bus_sel;
	u32 out_sel;
	u32 scomm_in_id;
	u32 scomm_out_id;
	u32 scomm_id;
	u32 larb_id;
	u32 larb_port_id;
	u32 effi_device_id = 99;
	u32 ip_effi_id;

	if (type == SUMMARY_COMMON_MODE || type == SMI_COMMON_MODE) {
		scomm_id = lc_id;
		scomm_in_id = port_id;
		base = find_device_addr(SMI_COMMON_DRV, scomm_id);
		if (!base) {
			pr_err("[MTK_BW]Can't find SMI COMMON device base!!\n");
			return -1;
		}
	} else {
		larb_id = lc_id;
		larb_port_id = port_id;
		base = find_device_addr(SMI_LARB_DRV, larb_id);
		if (!base) {
			pr_err("[MTK_BW]Can't find SMI LARB device base!!\n");
			return 0;
		}

		/* find larb node */
		ret = snprintf(dev_name, MAX_DEV_NAME_LEN, "larb%d", larb_id);
		if (ret >= MAX_DEV_NAME_LEN)
			return 0;

		larb_np = of_find_node_by_name(NULL, dev_name);
		if (!larb_np)
			return 0;

		/* find smi common input port id by larb node */
		ret = of_property_read_u32(larb_np,
				"mediatek,smi-port-id", &scomm_in_id);
		if (ret)
			return 0;

		/* find smi common node by larb node*/
		scomm_np = of_parse_phandle(larb_np, "mediatek,smi", 0);
		if (!scomm_np)
			return 0;

		/* find smi common id */
		ret = of_property_read_u32(scomm_np,
				"mediatek,common-id", &scomm_id);
		if (ret)
			return 0;
		/* get smi common reg base */
		base = of_iomap(scomm_np, 0);
		if (!base) {
			pr_err("[MTK_BW]%pOF: of_iomap failed\n", scomm_np);
			return 0;
		}
	}

	bus_sel = READ_REG(SMI_BUS_SEL);

	if (bus_sel & (1 << (scomm_in_id*2)))
		scomm_out_id = P1;
	else
		scomm_out_id = P0;

	/* find which effi monitor */
	for (i = 0; i < EFFI_MON_CNT; i++) {
		if (scomm_id == effi_device_map[i].common_id &&
		    scomm_out_id == effi_device_map[i].out_port_id) {

			effi_device_id = effi_device_map[i].effi_mon_id;
			break;
		}
	}

	base = find_device_addr(EFFI_MONITOR_DRV, effi_device_id);
	if (!base) {
		pr_err("[MTK_BW]Can't find effi_monitor device base!!\n");
		return 0;
	}

	/* make sure settings are written */
	wmb();

	/* clear REG_MONITOR_OP */
	WRITE_REG(0, REG_MONITOR_OP);

	if (type == SUMMARY_COMMON_MODE || type == SMI_COMMON_MODE) {
		/* for common effi, need to set [10:8] to 1 */
		WRITE_REG(EN_EFFI_SCOMM_MONITOR, REG_MONITOR_ID_EN);

		/* [10:8]: common input id */
		ip_effi_id = (scomm_in_id << REG_SCOMM_INPUT);
		WRITE_REG(ip_effi_id, REG_MONITOR_ID);
	} else {
		/* for larb effi, need to set all en to 1 */
		WRITE_REG(EN_EFFI_LARB_MONITOR, REG_MONITOR_ID_EN);

		/* [10:8]: common input id */
		/* [4:0]: larb port id */
		ip_effi_id = ((scomm_in_id << REG_SCOMM_INPUT) | larb_port_id);
		WRITE_REG(ip_effi_id, REG_MONITOR_ID);
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

	/* wait 30 ms */
	usleep_range((EFFI_MON_DELAY_TIME), (EFFI_MON_DELAY_TIME));

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

	return effi_output;
}

static int emi_measure_wrap(struct bw_cfg *cfg, struct emi_res *res)
{
	int ret;

	ret = emi_set_base(cfg);
	if (ret < 0)
		return ret;

	emi_detail_bw_measure(cfg, res);

	return ret;
}

#ifdef BW_EMI_MONITOR
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
		emi_res[i].arb_info =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].rd_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].wr_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);

		if (!emi_res[i].bus_bw || !emi_res[i].data_bw ||
		    !emi_res[i].latency || !emi_res[i].m_effi ||
		    !emi_res[i].arb_info || !emi_res[i].rd_ostd ||
		    !emi_res[i].wr_ostd) {
			len = -ENOMEM;
			goto RET_FUNC;
		}
	}
	/* measure emi bw */
	cfg->t_ms = emi_mon_cfg.t_ms;
	if (emi_measure_wrap(cfg, emi_res) < 0) {
		len = -EPERM;
		goto RET_FUNC;
	}

	/* initial emi effi */
	for (i = 0; i < res_cnt; i++)
		for (j = 0; j < emi_port_cnt; j++)
			emi_res[i].m_effi[j] = STATIC_REF_EMI_EFFI;

	/* output emi bw result */
	cfg->emi_detail = emi_mon_cfg.emi_detail;

	put_buf_emi_total(cfg, emi_res, buf, &len);

RET_FUNC:
	/* release buffer */
	if (emi_res) {
		for (i = 0; i < res_cnt; i++) {
			kfree(emi_res[i].bus_bw);
			kfree(emi_res[i].data_bw);
			kfree(emi_res[i].latency);
			kfree(emi_res[i].m_effi);
			kfree(emi_res[i].arb_info);
			kfree(emi_res[i].rd_ostd);
			kfree(emi_res[i].wr_ostd);
		}
		kfree(emi_res);
	}
	kfree(cfg);
	return len;
}
#endif

#ifdef BW_LARB_MONITOR
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
		res->port_bus_bw[i] = 0;
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
			res->port_bus_bw[i] += CAL_BUS_BW(res->r_bw[i], res->effi[i]);
			res->larb_bus_bw += CAL_BUS_BW(res->r_bw[i], res->effi[i]);

			BW_DBG("[MTK-BW] SMI Larb: %u port: %d type: R\033[K\n",
								larb_id,
								plist[i]);
			BW_DBG("[MTK-BW] effi: %u\033[K\n", res->effi[i]);
			BW_DBG("[MTK-BW] cnt_ultra: %llu, cnt_pultra: %llu CMD_NUM: %d\033[K\n",
								e_res.cnt_ultra,
								e_res.cnt_pultra,
								CMD_NUM);
			BW_DBG("[MTK-BW] r_ultra: %u, r_pultra: %u, burst: %u\033[K\n",
								res->r_ultra[i],
								res->r_pultra[i],
								res->burst[i]);
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
			res->port_bus_bw[i] += CAL_BUS_BW(res->w_bw[i], res->effi[i]);
			res->larb_bus_bw += CAL_BUS_BW(res->w_bw[i], res->effi[i]);

			BW_DBG("[MTK-BW] SMI Larb: %u port: %d type: W\033[K\n",
								larb_id,
								plist[i]);
			BW_DBG("[MTK-BW] effi: %u\033[K\n", res->effi[i]);
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

		res->larb_data_bw += res->r_bw[i] + res->w_bw[i];

	}

}
#endif

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

#ifdef BW_LARB_MONITOR
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
	}

	return 0;
}
#endif

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

		if (strncmp(mlist[m_idx].name, "CPU", sizeof("CPU")) == 0)
			res[m_idx]->m_effi = STATIC_CPU_EFFI;
		else if (strncmp(mlist[m_idx].name, "GPU", sizeof("GPU")) == 0)
			res[m_idx]->m_effi = STATIC_GPU_EFFI;
		else if (strncmp(mlist[m_idx].name, "REF_EMI", sizeof("REF_EMI")) == 0)
			res[m_idx]->m_effi = STATIC_REF_EMI_EFFI;

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

#ifdef BW_LARB_MONITOR
static int malloc_larb_member(struct larb_res *res[],
			      unsigned int idx)
{
	int err = 0;
	struct module_map m = mlist[larb_mon_cfg.m_id];

	res[idx]->r_bw = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->r_bw)), GFP_KERNEL);
	res[idx]->w_bw = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->w_bw)), GFP_KERNEL);
	res[idx]->max_cp = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->max_cp)), GFP_KERNEL);
	res[idx]->avg_cp = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->avg_cp)), GFP_KERNEL);
	res[idx]->r_avg_dp = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->r_avg_dp)), GFP_KERNEL);
	res[idx]->w_avg_dp = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->w_avg_dp)), GFP_KERNEL);
	res[idx]->port_bus_bw = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->port_bus_bw)), GFP_KERNEL);
	res[idx]->ostdl = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->ostdl)), GFP_KERNEL);
	res[idx]->ultra = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->ultra)), GFP_KERNEL);
	res[idx]->pultra = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->pultra)), GFP_KERNEL);
	res[idx]->r_ultra = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->r_ultra)), GFP_KERNEL);
	res[idx]->r_pultra = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->r_pultra)), GFP_KERNEL);
	res[idx]->effi = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->effi)), GFP_KERNEL);
	res[idx]->burst = kmalloc_array(m.plist[idx].pcount,
			sizeof(*(res[idx]->burst)), GFP_KERNEL);

	if (!res[idx]->r_bw || !res[idx]->w_bw ||
	    !res[idx]->max_cp || !res[idx]->avg_cp ||
	    !res[idx]->r_avg_dp || !res[idx]->w_avg_dp ||
	    !res[idx]->port_bus_bw || !res[idx]->ostdl ||
	    !res[idx]->ultra || !res[idx]->pultra ||
	    !res[idx]->r_ultra || !res[idx]->r_pultra ||
	    !res[idx]->effi || !res[idx]->burst)
		err = -1;

	return err;
}

static ssize_t bw_larb_show(struct device_driver *drv, char *buf)
{
	struct larb_res **res;
	struct module_map m = mlist[larb_mon_cfg.m_id];
	u32 i;
	u32 len = 0;
	int err = 0;

	res = kmalloc_array(m.larb_num, sizeof(*res), GFP_KERNEL);
	if (!res)
		return -ENOMEM;

	for (i = 0; i < m.larb_num; i++) {
		res[i] = kmalloc(sizeof(struct larb_res), GFP_KERNEL);
		if (!res[i])
			goto larb_free;

		err = malloc_larb_member(res, i);
		if (err) {
			pr_err("[MTK_BW]Can't malloc larb result!!\n");
			goto larb_free;
		}
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
#endif

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
}

static ssize_t larb_ostdl_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;
	int err = 0;
	u32 ostdl = 0;

	err = get_larb_ostdl(g_larb_id, g_larb_port_id, &ostdl);
	if (err)
		pr_err("[MTK_BW]get_larb_ostdl fail!!\n");

	putbuf(buf, &len, "larb id: %u, port id: %u, ostdl: 0x%x\033[K\n", g_larb_id,
									   g_larb_port_id,
									   ostdl);

	return len;
}

static ssize_t larb_ostdl_store(struct device_driver *drv,
				const char *buf,
				size_t count)
{

	if (sscanf(buf, "%u %u", &g_larb_id, &g_larb_port_id) == 2) {

		pr_info("[MTK_BW]set larb id: %u, larb_port_id: %u\n",
					g_larb_id,
					g_larb_port_id);
	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
	}

	return count;
}

static ssize_t bw_total_store(struct device_driver *drv,
			      const char *buf,
			      size_t count)
{
	u32 m_id;
	u32 enable;
	u32 t_ms;
	u32 proc_mon;

	if (sscanf(buf, "%u%u%u%u", &m_id, &enable, &t_ms, &proc_mon) == 4) {
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

	if (sscanf(buf, "%d%d%d%x", &mode, &larb_id, &port_id, &val) == 4) {
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

static ssize_t module_id_show(struct device_driver *drv, char *buf)
{
	u32 len = 0;
	u32 mid = 0;

	for (mid = 0; mid < module_cnt; mid++) {
		if (mlist[mid].larb_num != 0)
			putbuf(buf, &len, "Module: %10s / ID: %2d\n", mlist[mid].name,
								      mlist[mid].module_id);
	}

	return len;
}

#ifdef BW_LARB_MONITOR
static ssize_t bw_larb_store(struct device_driver *drv,
			     const char *buf,
			     size_t count)
{
	if (sscanf(buf, "%d%d", &larb_mon_cfg.m_id, &larb_mon_cfg.t_ms) == 2) {
		pr_info("[MTK_BW]Set Module ID = %d, Time= %d\n",
					larb_mon_cfg.m_id,
					larb_mon_cfg.t_ms);
		larb_mon_cfg.name = get_module_name_by_id(larb_mon_cfg.m_id);

	} else {
		pr_err("[MTK_BW]Wrong Argument!!\n");
	}

	return count;
}
#endif

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

static ssize_t emi_setting_show(struct device_driver *drv, char *buf)
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
		emi_res[i].arb_info =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].rd_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);
		emi_res[i].wr_ostd =
			kcalloc(emi_port_cnt, sizeof(u32), GFP_KERNEL);

		if (!emi_res[i].bus_bw || !emi_res[i].data_bw ||
		    !emi_res[i].latency || !emi_res[i].m_effi ||
		    !emi_res[i].arb_info || !emi_res[i].rd_ostd ||
		    !emi_res[i].wr_ostd) {
			len = -ENOMEM;
			goto RET_FUNC;
		}
	}
	/* measure emi bw */
	cfg->t_ms = emi_mon_cfg.t_ms;
	if (emi_measure_wrap(cfg, emi_res) < 0) {
		len = -EPERM;
		goto RET_FUNC;
	}

	/* initial emi effi */
	for (i = 0; i < res_cnt; i++)
		for (j = 0; j < emi_port_cnt; j++)
			emi_res[i].m_effi[j] = STATIC_REF_EMI_EFFI;

	/* output emi bw result */
	cfg->emi_detail = emi_mon_cfg.emi_detail;

	put_buf_emi_total(cfg, emi_res, buf, &len);

RET_FUNC:
	/* release buffer */
	if (emi_res) {
		for (i = 0; i < res_cnt; i++) {
			kfree(emi_res[i].bus_bw);
			kfree(emi_res[i].data_bw);
			kfree(emi_res[i].latency);
			kfree(emi_res[i].m_effi);
			kfree(emi_res[i].arb_info);
			kfree(emi_res[i].rd_ostd);
			kfree(emi_res[i].wr_ostd);
		}
		kfree(emi_res);
	}
	kfree(cfg);
	return len;
}

static DRIVER_ATTR_RW(bw_total);
#ifdef BW_EMI_MONITOR
static DRIVER_ATTR_RO(bw_emi);
#endif
#ifdef BW_LARB_MONITOR
static DRIVER_ATTR_RW(bw_larb);
#endif
static DRIVER_ATTR_RO(emi_setting);
static DRIVER_ATTR_RW(larb_ostdl);
static DRIVER_ATTR_WO(larb_config);
static DRIVER_ATTR_RO(module_id);
static DRIVER_ATTR_RW(debug_log_on);

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
				if (get_port(cnp, mod_idx, larb_node_idx))
					larb_node_idx++;
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

	err = driver_create_file(drv, &driver_attr_module_id);
	if (err) {
		pr_err("[MTK_BW]module_id create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_bw_total);
	if (err) {
		pr_err("[MTK_BW]bw_total create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_emi_setting);
	if (err) {
		pr_notice("[MTK_BW]emi_setting create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_larb_ostdl);
	if (err) {
		pr_err("[MTK_BW]larb_ostdl create attribute error\n");
		return err;
	}
	err = driver_create_file(drv, &driver_attr_larb_config);
	if (err) {
		pr_err("[MTK_BW]larb_config create attribute error\n");
		return err;
	}

	return err;
}

static int create_sysfs_node(void)
{
	struct device_driver *drv;
	int err = 0;

	drv = driver_find("mtk-db-tool", &platform_bus_type);

	err = driver_create_file(drv, &driver_attr_debug_log_on);
	if (err) {
		pr_err("[MTK_BW]debug_log_on create attribute error\n");
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

	return 0;
}

static int mtk_bw_measure_init(void)
{
	struct device_driver *drv;
	int err = 0;

	drv = driver_find("mtk-db-tool", &platform_bus_type);

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

static int mtk_effi_probe(struct platform_device *pdev)
{
	int i;
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
	for (i = 0; i < EFFI_BURST_SET_CNT; i++)
		writel(effi_mon->data[i], effi_mon->base + BURST_EFFI_TABLE(i));

	platform_set_drvdata(pdev, effi_mon);

	return 0;
}

static int __maybe_unused mtk_effi_suspend(struct device *dev)
{
	return 0;
}

static int __maybe_unused mtk_effi_resume(struct device *dev)
{

	struct mtk_effi *effi_mon = dev_get_drvdata(dev);
	int i;

	/* set up effi table for all burst length */
	for (i = 0; i < EFFI_BURST_SET_CNT; i++)
		writel(effi_mon->data[i], effi_mon->base + BURST_EFFI_TABLE(i));

	return 0;
}

static int mtk_effi_remove(struct platform_device *pdev)
{
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

static struct platform_driver mtk_db_driver = {
	.driver	= {
		.name = "mtk-db-tool",
	}
};

static int __init mtk_db_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_effi_driver);
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to register mtk_effi_monitor driver\n");
		return ret;
	}
	ret = platform_driver_register(&mtk_db_driver);
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to register mtk_bw_monitor driver\n");
		return ret;
	}
	/* setup DRAM config */
	ret = mtk_bw_setup_dram_config();
	if (ret != 0) {
		pr_err("[MTK_BW]Failed to init mtk_bw_setup_dram_config\n");
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

static void __exit mtk_db_exit(void)
{
	remove_module_list();
	platform_driver_unregister(&mtk_effi_driver);
	platform_driver_unregister(&mtk_db_driver);
}

module_init(mtk_db_init);
module_exit(mtk_db_exit);

MODULE_LICENSE("GPL");
