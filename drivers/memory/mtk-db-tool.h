/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_BW_H
#define __MTK_BW_H

/* misc */
#define DEFAULT_MON_TIME		16
#define CMD_NUM				1024
#define ENABLE				1
#define DISABLE				0
#define	bw_div_s(Divisor, Dividend) ({				\
	((!Dividend) ? 0 : div_s64((Divisor), (Dividend)));	\
})
#define	bw_div(Divisor, Dividend) ({				\
	u64 div_tmp;						\
	u32  __maybe_unused rem_tmp;				\
	div_tmp = Divisor;					\
	if (Dividend)						\
		rem_tmp = do_div((div_tmp), (Dividend));	\
	((!Dividend) ? 0 : div_tmp);				\
})
/* used for calculate nagtive value */
#define S_PERCENTAGE(numer, denom)	bw_div_s(((numer) * 100), (denom))
#define PERCENTAGE(numer, denom)	bw_div(((numer) * 100), (denom))
#define PERCENTAGE_ALIGN(numer)		bw_div((numer), (100))
#define PERMILLAGE(numer, denom)	bw_div(((numer) * 100), (denom))
#define PERMILLAGE_ALIGN(numer)		bw_div((numer), (1000))
#define RATIO_SCALE_FACTOR		2
/* We use 1000(not 1024) for Byte unit conversion to align BW estimation value */
#define B_MS_TO_MB_S(value)	(bw_div((bw_div(((value) * 1000), 1000)), 1000))
#define CAL_BUS_BW(data_bw, effi)	bw_div(((data_bw) * 100), (effi))
#define CAL_TOTAL_BW(width, clock)	\
	(width/8 * clock)
#define PSEC_PER_USEC		1000000L
#define EFFI_MON_DELAY_TIME		(50 * 1000)
#define FAKE_ENG_DELAY_TIME		500
#define STATIC_CPU_EFFI			64
#define STATIC_GPU_EFFI			64
#define STATIC_REF_EMI_EFFI		70
#define MAX_DEV_NAME_LEN		15
#define MAX_REG_OP_ARG			3

#define EMI_CEN_DRV		0
#define EMI_CHN_DRV		1
#define SMI_COMMON_DRV		2
#define SMI_LARB_DRV		3
#define EMI_DISPATCH_DRV	4
#define EFFI_MONITOR_DRV	5
#define CPU_PRI_DRV		6
#define MIU2GMC_DRV		7

#define EFFI_MON_CNT		(14)
#define EFFI_BURST_SET_CNT	(8)

#define CPU_PRI_ADDR_ALIGN	(0x1000)
#define CPU_PRI_SCOPE_NUM	(3)
#define CPU_PRI_NORMAL		(0)
#define CPU_PRI_PREULTRA	(1)
#define CPU_PRI_ULTRA		(2)

/* EMI magic number define */
#define EMI_0		(0)
#define EMI_1		(1)
#define EMI_2		(2)
#define EMI_DBW_MON_CNT	(6)
#define EMI_R_CONF	(0)
#define EMI_W_CONF	(1)
#define EMI_BW_LMT_PARA	(3)
#define EMI_TIMER_PARA	(4)
#define EMI_OSTD_PARA	(4)

/* EMI monitor Register offset */
#define EMI_CONM	(0x60)
#define EMI_IOCL	(0xD0)
#define EMI_IOCL_2ND	(0xD4)
#define EMI_IOCM	(0xD8)
#define EMI_IOCM_2ND	(0xDC)
#define EMI_ARBA	(0x100)
#define EMI_ARBC	(0x110)
#define EMI_ARBD	(0x118)
#define EMI_ARBE	(0x120)
#define EMI_ARBF	(0x128)
#define EMI_ARBG	(0x130)
#define EMI_ARBH	(0x138)
#define EMI_BMEN	(0x400)
#define EMI_BSTP	(0x404)
#define EMI_WACT	(0x420)
#define EMI_WSCT	(0x428)
#define EMI_MSEL	(0x440)
#define EMI_WSCT2	(0x458)
#define EMI_WSCT3	(0x460)
#define EMI_MSEL2	(0x468)
#define EMI_WSCT4	(0x464)
#define EMI_BMID0	(0x4B0)
#define EMI_BMID1	(0x4B4)
#define EMI_BMEN2	(0x4E8)
#define EMI_BMRW0	(0x4F8)
#define EMI_DBWA	(0xF00)
#define EMI_DBWB	(0xF04)
#define EMI_DBWC	(0xF08)
#define EMI_DBWD	(0xF0C)
#define EMI_DBWE	(0xF10)
#define EMI_DBWF	(0xF14)
#define EMI_DBWG	(0xF18)
#define EMI_DBWH	(0xF1C)
#define EMI_DBWA_2ND	(0xF2C)
#define EMI_DBWB_2ND	(0xF30)
#define EMI_DBWC_2ND	(0xF34)
#define EMI_DBWD_2ND	(0xF38)
#define EMI_DBWE_2ND	(0xF3C)
#define EMI_DBWF_2ND	(0xF40)
#define EMI_TTYPE1	(0x500)
#define EMI_TTYPE9	(0x540)
#define EMI_TTYPE_LAT(p)	(EMI_TTYPE1 + ((p) * 0x8))
#define EMI_TTYPE_TRANS(p)	(EMI_TTYPE9 + ((p) * 0x8))
#define EMI_ARB(p)		(EMI_ARBA + (((p) == 1) ? 0 : (p)) * 0x8)
#define EMI_OSTD_L(p)		((((p) / 4) == 1) ? EMI_IOCM : EMI_IOCL)
#define EMI_OSTD_H(p)		((((p) / 4) == 1) ? EMI_IOCM_2ND : EMI_IOCL_2ND)

/* EMI BIT operation */
#define AFIFO_CHN_DCM	(30)
#define EMI_DBW0	(0)
#define EMI_DBW1	(1)
#define EMI_DBW2	(2)
#define EMI_DBW3	(3)
#define EMI_DBW4	(4)
#define EMI_DBW5	(5)
#define EMI_RD_OSTD(port, low, high)		\
({						\
	typeof(port) _p = (port);		\
	typeof(low) _l = (low);			\
	typeof(high) _h = (high);		\
	(((_l >> ((_p % 4) * 8)) & 0xF) |	\
	(((_h >> ((_p % 4) * 8)) & 0xF) << 4));	\
})

#define EMI_WR_OSTD(port, low, high)			\
({							\
	typeof(port) _p = (port);			\
	typeof(low) _l = (low);				\
	typeof(high) _h = (high);			\
	(((_l >> (((_p % 4) * 8) + 4)) & 0xF) |		\
	(((_h >> (((_p % 4) * 8) + 4)) & 0xF) << 4));	\
})
#define	GET_LOOP_ROUND(port, mon_num)	\
	((port == 0) ? 0 : (((port - 1) / mon_num) + 1))

#define EMI_OSTD_HBIT(offset)	((offset) + 3)
#define EMI_BW_LMT_MASK		GENMASK(5, 0)
#define EMI_R_TIMER_MASK	GENMASK(23, 16)
#define EMI_W_TIMER_MASK	GENMASK(31, 24)
#define EMI_OSTD_MASK(offset)	GENMASK(EMI_OSTD_HBIT(offset), (offset))
/* low 6 bit */
#define EMI_GET_BW_LMT(ori)	((ori) & EMI_BW_LMT_MASK)
/* bit 23 : 16 */
#define EMI_GET_RD_TIMER(ori)	(((ori) & EMI_R_TIMER_MASK) >> 16)
#define EMI_SET_RD_TIMER(ori)	(((ori) << 16) & EMI_R_TIMER_MASK)
/* bit 31 : 24 */
#define EMI_GET_WR_TIMER(ori)	(((ori) & EMI_W_TIMER_MASK) >> 24)
#define EMI_SET_WR_TIMER(ori)	(((ori) << 24) & EMI_W_TIMER_MASK)

#define EMI_OSTD_L_VAL(ori)	(((ori) >> 4) & 0xF)
#define EMI_OSTD_H_VAL(ori)	((ori) & 0xF)
#define EMI_OSTD_PORT_OFFSET(port, flag)		\
({							\
	typeof(port) _p = (port);			\
	typeof(flag) _f = (flag);			\
	((_p % 4) * 8) + ((_f == EMI_R_CONF) ? 0 : 4);	\
})
#define EMI_BW_8BYTE(val)	((val) * 8)

#define BUS_MON_EN	(0)
#define BUS_MON_STP	(2)

#define DBW0_HPRI_DIS		(2)
#define DBW0_WCMD_GNT_SEL	(1)
#define DBW0_RCMD_GNT_SEL	(0)

#define HPRI_SEL_HIGH		(28)
#define HPRI_SEL_FLUSH		(29)
#define HPRI_SEL_PREULTRA	(30)
#define HPRI_SEL_ULTRA		(31)

#define BUS_MON_R		(4)
#define BUS_MON_W		(5)
#define DBW0_SEL_MASTER_M0	(8)
#define DBW0_SEL_MASTER_M1	(9)
#define DBW0_SEL_MASTER_M2	(10)
#define DBW0_SEL_MASTER_M3	(11)
#define DBW0_SEL_MASTER_M4	(12)
#define DBW0_SEL_MASTER_M5	(13)
#define DBW0_SEL_MASTER_M6	(14)
#define DBW0_SEL_MASTER_M7	(15)
#define BMEN_LAT_CNT_SEL	(25)

/* SMI magic number define */
#define SMI_PARA_MON			(4)

/* SMI LARB Register offset */
#define SMI_LARB_FORCE_ULTRA		(0x078)
#define SMI_LARB_FORCE_PREULTRA		(0x07c)
#define SMI_LARB_MON_EN			(0x400)
#define SMI_LARB_MON_CLR		(0x404)
#define SMI_LARB_MON_PORT		(0x408)
#define SMI_LARB_MON_CON		(0x40c)
#define SMI_LARB_MON_ACT_CNT		(0x410)
#define SMI_LARB_MON_REQ_CNT		(0x414)
#define SMI_LARB_MON_BEAT_CNT		(0x418)
#define SMI_LARB_MON_BYTE_CNT		(0x41c)
#define SMI_LARB_MON_CP_CNT		(0x420)
#define SMI_LARB_MON_DP_CNT		(0x424)
#define SMI_LARB_MON_CP_MAX		(0x430)
#define SMI_LARB_OSTDL_PORT             (0x200)
#define SMI_LARB_OSTDL_PORTx(id)        (SMI_LARB_OSTDL_PORT + ((id) << 2))

/* SMI LARB BIT operation */
#define PARA_MODE	(10)
#define PORT_SEL0	(0)
#define PORT_SEL1	(8)
#define PORT_SEL2	(16)
#define PORT_SEL3	(24)
#define DP_MODE		(9)
#define RW_SEL		(2)

/* SMI COMMON Register offset */
#define SMI_BUS_SEL		(0x220)

/* SMI COMMON monitor Register offset */
#define SMI_MON_AXI_ENA		(0x1A0)
#define SMI_MON_AXI_CLR		(0x1A4)
#define SMI_MON_AXI_TYPE	(0x1AC)
#define SMI_MON_AXI_CON		(0x1B0)
#define SMI_MON_AXI_REQ_CNT	(0x1C4)
#define SMI_MON_AXI_BYT_CNT	(0x1D0)
#define SMI_MON_AXI_CP_CNT	(0x1D4)
#define SMI_MON_AXI_DP_CNT	(0x1D8)
#define SMI_MON_AXI_CP_MAX	(0x1DC)

/* SMI COMMON BIT operation */
#define AXI_BUS_SELECT	(4)
#define MON_TYPE_RW_SEL	(0)
#define DP_SEL		(0)

/* EMI DISPATCH Register offset */
#define BROC_MODE0		(0x100)
#define BROC_MODE1		(0x104)

/* EMI DISPATCH Register value */
#define DISP_1_EMI_BORC_MODE0		(0x7f7f)
#define DISP_1_EMI_BORC_MODE1		(0x0000)

#define DISP_2_EMI_BORC_MODE0		(0x7f7f)
#define DISP_2_EMI_BORC_MODE1		(0x0002)

#define DISP_3_EMI_BORC_MODE0		(0x7f7f)
#define DISP_3_EMI_BORC_MODE1		(0x7f02)

/* EMI EFFI Register offset */
#define REG_MONITOR_OP		(0x00)
#define REG_MONITOR_ID		(0x04)
#define REG_MONITOR_ID_EN	(0x08)
#define BURST_EFFI_TABLE(index)	(0x0c + (0x4*index))
#define REG_MONITOR_OUT_0	(0x2C)
#define REG_MONITOR_OUT_1	(0x3C)

#define REG_MONITOR_OUT_SEL	(6)
#define REG_RPT_ON		(4)
#define REG_CMD_BOUND_STOP	(3)
#define REG_CMD_BOUND_SEL	(1)
#define REG_MONITOR_SW_RST	(0)
#define REG_SCOMM_INPUT		(8)

/* EMI EFFI Register value */
#define EN_EFFI_SCOMM_MONITOR	(0x700)
#define EN_EFFI_LARB_MONITOR	(0x7ff)

/* EMI CPU ultra/preultra range offset */
#define REG_CPU_PRI_EN		(0x18)
#define REG_CPU_PRI_S0_SADDR_L	(0x1C)
#define REG_CPU_PRI_S0_SADDR_H	(0x20)
#define REG_CPU_PRI_S0_EADDR_L	(0x24)
#define REG_CPU_PRI_S0_EADDR_H	(0x28)
#define REG_CPRI_SCOPE(reg, scope)	((reg) + ((scope) * 0x10))

/* EMI CPU ultra/preultra range BIT operation */
#define CPU_PRI_S0_EN		(1)
#define CPU_PRI_S1_EN		(2)
#define CPU_PRI_S2_EN		(3)
#define CPU_PRI_PREULTRA_EN	(4)
#define CPU_PRI_ULTRA_EN	(5)

/* MIU2GMC_OFFSET REG */
#define MISC				(0x00)
#define TEST_BASE_ADDR_L		(0x04)
#define TEST_BASE_ADDR_H		(0x08)
#define TEST_LEN_L			(0x0C)
#define TEST_LEN_H			(0x10)
#define TEST_DATA			(0x14)
#define GROUP_REQ_MASK			(0x18)

/* FAKE_ENG RESULT */
#define NO_RESULT			(0)
#define PASS				(1)
#define FAIL				(2)

/* MIU2GMC BIT OP */
#define REG_R_TEST_FINISH		(1 << 15)
#define REG_R_TEST_FAIL			(1 << 13)
#define REG_READ_ONLY			(1 << 8)
#define REG_WRITE_ONLY			(1 << 9)


/* Register opteration */
#define READ_REG(offset)	readl_relaxed(base + (offset))
#define READ_REG_16(offset)	readw_relaxed(base + (offset))
#define WRITE_REG(val, offset)	writel_relaxed((val), base + (offset))
#define LOW_16BIT		GENMASK(15, 0)
#define LOW_32BIT		GENMASK(31, 0)
#define RSHIFT_16BIT(val)	((val) >> 16)
#define LSHIFT_16BIT(val)	((val) << 16)
#define bw_setbit(offset, flag, reg)					\
({									\
	typeof(offset) _offset = (offset);				\
	typeof(flag) _flag = (flag);					\
	typeof(reg) _reg = (reg);					\
	(_flag) ?							\
	writel_relaxed((readw_relaxed(_reg) | (1 << _offset)), _reg) :	\
	writel_relaxed((readw_relaxed(_reg) & ~(1 << _offset)), _reg);	\
})									\

#define NODE_NAME_SIZE		(16)
#define DRAM_TPYE_NAME_SIZE	(10)

#define GET_LARB_NUM	(0)
#define GET_COMM_NUM	(1)

#define SMI_COMMON(id)	id
#define EFFI_MON(id)	id
#define MAX_LARB_NR	53
#define MAX_EMI_NR	3
#define MAX_CMP_NUM	15
#define P0		0
#define P1		1

/* for reg op */
#define REG_START_ADDR			0x1c000000
#define REG_END_ADDR			0x1ec00000
#define REG_OP_SIZE			0x4
#define REG_HIGH_16BIT_MAX		31
#define REG_HIGH_16BIT_MIN		16
#define REG_LOW_16BIT_MAX		15
#define REG_LOW_16BIT_MIN		0

#define DB_REG		(0x1c30f010)
#define DB_REG_SIZE	(0x4)
#define DB		(2)

/* for debug printf */
#define BW_DBG(fmt, ...)					\
	do {							\
		if (debug_on)					\
			pr_alert(fmt, ##__VA_ARGS__);		\
	} while (0)

struct mtk_effi {
	void __iomem		*base;
	const unsigned int	*data;
};

struct larb_config {
	unsigned int	pcount;
	unsigned int	*smi_plist;
};

struct common_config {
	unsigned int	pcount;
	unsigned int	*com_plist;
};

struct module_map {
	const char		*name;
	unsigned int		module_id;
	unsigned int		larb_num;
	unsigned int		common_num;
	unsigned int		*llist;
	struct larb_config	*plist;
	unsigned int		*clist;
	struct common_config	*cplist;
	const char		*domain;
	bool			enable;
};

struct bw_cfg {
	void __iomem		*base;
	void __iomem		*base1;
	void __iomem		*base2;
	const char		*name;
	bool			emi_detail;
	u32			emi_port;
	u32			t_ms;
	u32			m_id;
	u32			p_idx;
	u32			l_idx;
	u32			c_idx;
	u32			proc_mon;
};

struct bw_res {
	u64	bus_bw;
	u64	data_bw;
	u64	*pbw;
	u32	port_num;
	u32	m_effi;
	u32	*p_effi;
	u32	r_ultra;
	u32	r_pultra;
	u64	cnt_ultra;
	u64	cnt_pultra;
	u64	avg_burst;
	u64	tmp_ultra;
	u64	tmp_pultra;
};

struct emi_res {
	u64     *bus_bw;
	u64     *data_bw;
	u64     *latency;
	u32     *m_effi;
	u32	*arb_info;
	u32	*rd_ostd;
	u32	*wr_ostd;
};

struct scomm_res {
	u32	id;
	u32	ip_num;
	u32	op_num;
	u64	*r_bw;		/* Mbytes/sec) */
	u32	*r_max_cp;	/* ns */
	u32	*r_avg_cp;	/* ns */
	u32	*r_avg_dp;	/* ns */
	u64	*w_bw;		/* Mbytes/sec) */
	u32	*w_max_cp;	/* ns/trans */
	u32	*w_avg_cp;	/* ns */
	u32	*w_avg_dp;	/* ns/trans */
	u32	*r_ultra;
	u32	*r_pultra;
	u32	*effi;
	u32	*burst;
};

struct larb_res {
	u32	id;
	u64	larb_data_bw;	/* Mbytes/sec) */
	u64	larb_bus_bw;	/* Mbytes/sec) */
	u64	*port_bus_bw;	/* Mbytes/sec) */
	u64	*r_bw;		/* Mbytes/sec) */
	u32	*r_avg_dp;	/* ns */
	u64	*w_bw;		/* Mbytes/sec) */
	u32	*w_avg_dp;	/* ns/trans */
	u32	*max_cp;	/* ns */
	u32	*avg_cp;	/* ns */
	u32	*ostdl;
	bool	*ultra;
	bool	*pultra;
	u32	*r_ultra;
	u32	*r_pultra;
	u32	*effi;
	u32	*burst;
};

struct effi_mon_mapping {
	int common_id;
	int out_port_id;
	int effi_mon_id;
};

struct effi_res {
	u64	cnt_ultra;
	u64	cnt_pultra;
	u64	avg_burst;
};

enum larb_setting_mode {
	OSTDL_MODE,
	ULTRA_MODE,
	PREULTRA_MODE,
};

enum monitor_mode {
	SUMMARY_LARB_MODE,
	SUMMARY_COMMON_MODE,
	SMI_COMMON_MODE,
	SMI_LARB_MODE,
};

enum fake_engine_type {
	READ_ONLY,
	WRITE_ONLY,
	RW,
};

enum dispatch_rule {
	DISPATCH_1EMI_2CH = 1,
	DISPATCH_2EMI_4CH = 2,
	DISPATCH_3EMI_6CH = 3,
};

#endif

