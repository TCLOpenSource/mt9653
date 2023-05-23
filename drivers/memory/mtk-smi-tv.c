// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/smi.h>
#include <dt-bindings/memory/mtk-smi-larb-port.h>
#include <dt-bindings/memory/mt2701-larb-port.h>

/* mt8173 */
#define SMI_LARB_MMU_EN		0xf00

/* mt2701 */
#define REG_SMI_SECUR_CON_BASE		0x5c0

/* every register control 8 port, register offset 0x4 */
#define REG_SMI_SECUR_CON_OFFSET(id)	(((id) >> 3) << 2)
#define REG_SMI_SECUR_CON_ADDR(id)	\
	(REG_SMI_SECUR_CON_BASE + REG_SMI_SECUR_CON_OFFSET(id))

/*
 * every port have 4 bit to control, bit[port + 3] control virtual or physical,
 * bit[port + 2 : port + 1] control the domain, bit[port] control the security
 * or non-security.
 */
#define SMI_SECUR_CON_VAL_MSK(id)	(~(0xf << (((id) & 0x7) << 2)))
#define SMI_SECUR_CON_VAL_VIRT(id)	BIT((((id) & 0x7) << 2) + 3)
/* mt2701 domain should be set to 3 */
#define SMI_SECUR_CON_VAL_DOMAIN(id)	(0x3 << ((((id) & 0x7) << 2) + 1))

/* mt2712 */
#define SMI_LARB_NONSEC_CON(id)	(0x380 + ((id) * 4))
#define F_MMU_EN		BIT(0)
#define BANK_SEL(a)		((((a) & 0x3) << 8) | (((a) & 0x3) << 10) |\
				 (((a) & 0x3) << 12) | (((a) & 0x3) << 14))
/* mt6873 */
#define SMI_LARB_OSTDL_PORT		0x200
#define SMI_LARB_OSTDL_PORTx(id)	(SMI_LARB_OSTDL_PORT + ((id) << 2))

#define SMI_LARB_SLP_CON		0x00c
#define SLP_PROT_EN			BIT(0)
#define SLP_PROT_RDY			BIT(16)
#define SMI_LARB_CMD_THRT_CON		0x24
#define SMI_LARB_SW_FLAG		0x40
#define SMI_LARB_WRR_PORT		0x100
#define SMI_LARB_WRR_PORTx(id)		(SMI_LARB_WRR_PORT + ((id) << 2))
/* SMI COMMON */
#define SMI_BUS_SEL			0x220
#define SMI_BUS_LARB_SHIFT(larbid)	((larbid) << 1)
/* All are MMU0 defaultly. Only specialize mmu1 here. */
#define F_MMU1_LARB(larbid)		(0x1 << SMI_BUS_LARB_SHIFT(larbid))
#define SMI_L1LEN			0x100
#define SMI_L1ARB0			0x104
#define SMI_L1ARB(id)			(SMI_L1ARB0 + ((id) << 2))
#define SMI_M4U_TH			0x234
#define SMI_FIFO_TH1			0x238
#define SMI_FIFO_TH2			0x23c
#define SMI_DCM				0x300
#define SMI_DUMMY			0x444
#define SMI_LARB_PORT_NR_MAX		32
#define SMI_COMMON_NR_MAX		8
#define SMI_COMMON_LARB_NR_MAX		8
#define SMI_LARB_MISC_NR		2
#define SMI_COMMON_MISC_NR		6
#define MAX_MIU2GMC_NAME_LEN		10
#define MIU2GMC_MASK			0x18
#define MAX_SMILARB_NAME_LEN		7
#define LARB_MAX_OSTDL			0x3f

struct mtk_smi_reg_pair {
	u16	offset;
	u32	value;
};


#define SMI_L1LEN			0x100
#define SMI_L1ARB0			0x104
#define SMI_L1ARB(id)			(SMI_L1ARB0 + ((id) << 2))

enum mtk_smi_gen {
	MTK_SMI_GEN1,
	MTK_SMI_GEN2
};

struct mtk_smi_common_plat {
	enum				mtk_smi_gen gen;
	bool				has_gals;
	u32				bus_sel;
	bool				has_bwl;
	u16				*bwl;
	struct mtk_smi_reg_pair		*misc;
};

struct mtk_smi_larb_gen {
	int port_in_larb[MTK_LARB_NR_MAX + 1];
	int port_in_larb_gen2[MTK_LARB_NR_MAX + 1];
	void (*config_port)(struct device *dev);
	void (*sleep_ctrl)(struct device *dev, bool toslp);
	unsigned int			larb_direct_to_common_mask;
	bool				has_gals;
	bool		has_bwl;
	u8		*bwl;
	struct mtk_smi_reg_pair *misc;
};

struct mtk_smi {
	struct device			*dev;
	struct clk			*clk_apb, *clk_smi;
	struct clk			*clk_gals0, *clk_gals1;
	struct clk			*clk_async; /*only needed by mt2701*/
	union {
		void __iomem		*smi_ao_base; /* only for gen1 */
		void __iomem		*base;	      /* only for gen2 */
	};
	const struct mtk_smi_common_plat *plat;
	int				common_id;
};

struct mtk_smi_larb { /* larb: local arbiter */
	struct mtk_smi			smi;
	void __iomem			*base;
	struct device			*smi_common_dev;
	const struct mtk_smi_larb_gen	*larb_gen;
	int				larbid;
	u32				*mmu;
	u32				*bank;
};

int mtk_miu2gmc_mask(u32 miu2gmc_id, u32 port_id, bool enable)
{
	void __iomem *base;
	struct device_node *np;
	char dev_name[MAX_MIU2GMC_NAME_LEN];
	int num = 0;

	num = snprintf(dev_name, MAX_MIU2GMC_NAME_LEN, "miu2gmc%d", miu2gmc_id);

	if (num >= MAX_MIU2GMC_NAME_LEN)
		return -1;

	np = of_find_node_by_name(NULL, dev_name);
	if (!np) {
		pr_err("[MTK-SMI]can not find node\n");
		return -1;
	}

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("[MTK-SMI]%pOF: of_iomap failed\n", np);
		return -1;
	}

	if (enable)
		__set_bit(port_id, (base + MIU2GMC_MASK));
	else
		__clear_bit(port_id, (base + MIU2GMC_MASK));

	iounmap(base);
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_miu2gmc_mask);

void mtk_smi_common_bw_set(struct device *dev, const u32 port, const u32 val)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);

	writel(val, common->base + SMI_L1ARB(port));
}
EXPORT_SYMBOL_GPL(mtk_smi_common_bw_set);

void mtk_smi_larb_bw_set(struct device *dev, const u32 port, const u32 val)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);

	if (val)
		writel(val, larb->base + SMI_LARB_OSTDL_PORTx(port));
}
EXPORT_SYMBOL_GPL(mtk_smi_larb_bw_set);

#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
static int mtk_smi_clk_enable(const struct mtk_smi *smi)
{
	int ret;

	ret = clk_prepare_enable(smi->clk_apb);
	if (ret)
		return ret;

	ret = clk_prepare_enable(smi->clk_smi);
	if (ret)
		goto err_disable_apb;

	ret = clk_prepare_enable(smi->clk_gals0);
	if (ret)
		goto err_disable_smi;

	ret = clk_prepare_enable(smi->clk_gals1);
	if (ret)
		goto err_disable_gals0;

	return 0;

err_disable_gals0:
	clk_disable_unprepare(smi->clk_gals0);
err_disable_smi:
	clk_disable_unprepare(smi->clk_smi);
err_disable_apb:
	clk_disable_unprepare(smi->clk_apb);
	return ret;
}

static void mtk_smi_clk_disable(const struct mtk_smi *smi)
{
	clk_disable_unprepare(smi->clk_gals1);
	clk_disable_unprepare(smi->clk_gals0);
	clk_disable_unprepare(smi->clk_smi);
	clk_disable_unprepare(smi->clk_apb);
}
#endif

int mtk_smi_larb_get(struct device *larbdev)
{
	int ret = pm_runtime_get_sync(larbdev);

	return (ret < 0) ? ret : 0;
}
EXPORT_SYMBOL_GPL(mtk_smi_larb_get);

void mtk_smi_larb_put(struct device *larbdev)
{
	pm_runtime_put_sync(larbdev);
}
EXPORT_SYMBOL_GPL(mtk_smi_larb_put);

static int
mtk_smi_larb_bind(struct device *dev, struct device *master, void *data)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct mtk_smi_larb_iommu *larb_mmu = data;
	unsigned int         i;

	for (i = 0; i < MTK_LARB_NR_MAX; i++) {
		if (dev == larb_mmu[i].dev) {
			larb->larbid = i;
			larb->mmu = &larb_mmu[i].mmu;
			larb->bank = &larb_mmu[i].bank[0];
			return 0;
		}
	}
	return -ENODEV;
}

static void mtk_smi_larb_config_port_gen2_general(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	u32 reg;
	int i;

	if (BIT(larb->larbid) & larb->larb_gen->larb_direct_to_common_mask)
		return;

	if (larb->mmu) {
		for_each_set_bit(i, (unsigned long *)larb->mmu, 32) {
			reg = readl_relaxed(larb->base + SMI_LARB_NONSEC_CON(i));
			reg |= F_MMU_EN;
			reg |= BANK_SEL(larb->bank[i]);
			writel(reg, larb->base + SMI_LARB_NONSEC_CON(i));
		}
	}
	if (!larb->larb_gen->has_bwl)
		return;
	for (i = 0; i < larb->larb_gen->port_in_larb_gen2[larb->larbid]; i++)
		mtk_smi_larb_bw_set(larb->smi.dev, i, larb->larb_gen->bwl[
			larb->larbid * SMI_LARB_PORT_NR_MAX + i]);
#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
	for (i = 0; i < SMI_LARB_MISC_NR; i++)
		writel_relaxed(larb->larb_gen->misc[
			larb->larbid * SMI_LARB_MISC_NR + i].value,
			larb->base + larb->larb_gen->misc[
			larb->larbid * SMI_LARB_MISC_NR + i].offset);
#endif
	wmb(); /* make sure settings are written */

}

static void mtk_smi_larb_config_port_mt8173(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);

	writel(*larb->mmu, larb->base + SMI_LARB_MMU_EN);
}

static void mtk_smi_larb_config_port_gen1(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	const struct mtk_smi_larb_gen *larb_gen = larb->larb_gen;
	struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);
	int i, m4u_port_id, larb_port_num;
	u32 sec_con_val, reg_val;

	m4u_port_id = larb_gen->port_in_larb[larb->larbid];
	larb_port_num = larb_gen->port_in_larb[larb->larbid + 1]
			- larb_gen->port_in_larb[larb->larbid];

	for (i = 0; i < larb_port_num; i++, m4u_port_id++) {
		if (*larb->mmu & BIT(i)) {
			/* bit[port + 3] controls the virtual or physical */
			sec_con_val = SMI_SECUR_CON_VAL_VIRT(m4u_port_id);
		} else {
			/* do not need to enable m4u for this port */
			continue;
		}
		reg_val = readl(common->smi_ao_base
			+ REG_SMI_SECUR_CON_ADDR(m4u_port_id));
		reg_val &= SMI_SECUR_CON_VAL_MSK(m4u_port_id);
		reg_val |= sec_con_val;
		reg_val |= SMI_SECUR_CON_VAL_DOMAIN(m4u_port_id);
		writel(reg_val,
			common->smi_ao_base
			+ REG_SMI_SECUR_CON_ADDR(m4u_port_id));
	}
}

#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
static void mtk_smi_larb_sleep_ctrl(struct device *dev, bool toslp)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	void __iomem *base = larb->base;
	u32 tmp;

	if (toslp) {
		writel_relaxed(SLP_PROT_EN, base + SMI_LARB_SLP_CON);
		if (readl_poll_timeout_atomic(base + SMI_LARB_SLP_CON,
				tmp, !!(tmp & SLP_PROT_RDY), 10, 10000))
			dev_notice(dev, "sleep cond not ready(%d)\n", tmp);
	} else
		writel_relaxed(0, base + SMI_LARB_SLP_CON);
}
#endif

static void
mtk_smi_larb_unbind(struct device *dev, struct device *master, void *data)
{
	/* Do nothing as the iommu is always enabled. */
}

static const struct component_ops mtk_smi_larb_component_ops = {
	.bind = mtk_smi_larb_bind,
	.unbind = mtk_smi_larb_unbind,
};

static u8
mtk_smi_larb_mt6873_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x2, 0x2, 0x28, 0xa, 0xc, 0x28,},
	{0x2, 0x2, 0x18, 0x18, 0x18, 0xa, 0xc, 0x28,},
	{0x5, 0x5, 0x5, 0x5, 0x1,},
	{},
	{0x28, 0x19, 0xb, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x4, 0x1,},
	{0x1, 0x1, 0x4, 0x1, 0x1, 0x1, 0x1, 0x16,},
	{},
	{0x1, 0x3, 0x2, 0x1, 0x1, 0x5, 0x2, 0x12, 0x13, 0x4, 0x4, 0x1,
	 0x4, 0x2, 0x1,},
	{},
	{0xa, 0x7, 0xf, 0x8, 0x1, 0x8, 0x9, 0x3, 0x3, 0x6, 0x7, 0x4,
	 0xa, 0x3, 0x4, 0xe, 0x1, 0x7, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x1, 0x1, 0x1, 0x1, 0x1,},
	{},
	{0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
	 0x1, 0x1, 0x1, 0xe, 0x1, 0x7, 0x8, 0x7, 0x7, 0x1, 0x6, 0x2,
	 0xf, 0x8, 0x1, 0x1, 0x1,},
	{},
	{0x2, 0xc, 0xc, 0xe, 0x6, 0x6, 0x6, 0x6, 0x6, 0x12, 0x6, 0x28,},
	{0x2, 0xc, 0xc, 0x28, 0x12, 0x6,},
	{},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x28, 0x14, 0x2, 0xc, 0x18, 0x4, 0x28, 0x14, 0x4, 0x4, 0x4, 0x2,
	 0x4, 0x2, 0x8, 0x4, 0x4,},
	{0x2, 0x2, 0x4, 0x2,},
	{0x9, 0x9, 0x5, 0x5, 0x1, 0x1,},
};

static u8
mtk_smi_larb_mt5896_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0xf, 0xf}, /* larb0 */
	{0x8, 0x8, 0x8, 0x8}, /* larb1 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8}, /* larb2 */
	{0xf, 0xf}, /* larb3 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8}, /* larb4 */
	{0x8, 0x8, 0x8, 0x8}, /* larb5 */
	{0xf, 0xf}, /* larb6 */
	{0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
	 0x1e, 0x1e, 0x1e, 0x1e,}, /* larb7 */
	{0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf,
	 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb8 */
	{0xf, 0xf, 0xf, 0x6, 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb9 */
	{0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf,
	 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb10 */
	{0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf,
	 0xf, 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb11 */
	{0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e}, /* larb12 */
	{0xf, 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb13 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb14 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb15 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb16 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb17 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb18 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb19 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb20 */
	{0x1f, 0x1f, 0x1f}, /* larb21 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb22 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb23 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb24 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb25 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /*larb26 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb27 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb28 */
	{0xf, 0xf, 0xf, 0x1f, 0xf, 0xf}, /* larb29 */
	{0x8, 0x8, 0x8, 0x8}, /* larb30 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8}, /* larb31 */
	{0xf, 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb32 */
	{0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb33 */
	{0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf}, /* larb34 */
	{0x1f, 0x1f, 0x1f}, /* larb35 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb36 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb37 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb38 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb39 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb40 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb41 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb42 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb43 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb44 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb45 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb46 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb47 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb48 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb49 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb50 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb51 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb52 */
};

static u8
mtk_smi_larb_mt5876_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8, 0x8, 0x8, 0x8, 0x8}, /* larb0 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8, 0x8, 0x8, 0x8}, /* larb1 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8}, /* larb2 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
	 0x1f, 0x1f, 0x1f}, /* larb3 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8}, /* larb4 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8}, /* larb5 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8}, /* larb6 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x3f, 0x8, 0x8}, /* larb7 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	 0x8, 0x8}, /* larb8 */
	{0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8}, /* larb9 */
	{0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, /* larb10 */
	{0x8, 0x8, 0x8, 0x8, 0x8}, /* larb11 */
	{0x8, 0x8, 0x8, 0x8}, /* larb12 */
};

static u8
mtk_smi_larb_mt5879_bwl[MTK_LARB_NR_MAX][SMI_LARB_PORT_NR_MAX] = {
	{0x3f, 0x08, 0x0a, 0x08, 0x08, 0x14, 0x08, 0x08, 0x08,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, /*larb0 15*/
	{0x3f, 0x08, 0x15, 0x1e, 0x08, 0x1e, 0x08, 0x09, 0x09,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, /*larb1 17*/
	{0x3f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	 0x08}, /*larb2 10*/
	{0x3f, 0x0f, 0x0f, 0x0f, 0x08, 0x0f, 0x0f, 0x0f, 0x0f,
	 0x0f, 0x0f, 0x0f, 0x0f}, /*larb3 13*/
	{0x3f, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
	 0x14, 0x14, 0x14}, /*larb4 12*/
	{0x3f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, /*larb5 18*/
	{0x3f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	 0x08, 0x08}, /*larb6 29*/
	{0x3f, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
	 0x14, 0x14, 0x3f, 0x14, 0x14}, /*larb7 14*/
	{0x3f, 0x08, 0x22, 0x08, 0x11, 0x08, 0x08, 0x08, 0x08,
	 0x08, 0x08}, /*larb8 11*/
	{0x3f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	 0x08}, /*larb9 10*/
	{0x3f,  0x30, 0x0f, 0x1f, 0x1f, 0x0f, 0x0f}, /*larb10 7*/
	{0x08, 0x08}, /*larb11 2*/
	{0x3f, 0x08, 0x08, 0x08, 0x08}, /*larb 12 5*/
};

static struct mtk_smi_reg_pair
mtk_smi_larb_mt6873_misc[MTK_LARB_NR_MAX][SMI_LARB_MISC_NR] = {
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x370256}, {SMI_LARB_SW_FLAG, 0x1},},
	{},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
	{{SMI_LARB_CMD_THRT_CON, 0x300256}, {SMI_LARB_SW_FLAG, 0x1},},
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8173 = {
	/* mt8173 do not need the port in larb */
	.config_port = mtk_smi_larb_config_port_mt8173,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt2701 = {
	.port_in_larb = {
		LARB0_PORT_OFFSET, LARB1_PORT_OFFSET,
		LARB2_PORT_OFFSET, LARB3_PORT_OFFSET
	},
	.config_port = mtk_smi_larb_config_port_gen1,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt2712 = {
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(8) | BIT(9),      /* bdpsys */
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt6873 = {
	.port_in_larb_gen2 = {6, 8, 5, 0, 11, 8, 0, 15, 0, 29, 0, 29,
			      0, 12, 6, 0, 17, 17, 17, 4, 6,},
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(3) | BIT(6) | BIT(8) |
				      BIT(10) | BIT(12) | BIT(15) | BIT(21) |
					  BIT(22),
				      /*skip larb: 3,6,8,10,12,15,21,22*/
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt6873_bwl,
	.misc = (struct mtk_smi_reg_pair *)mtk_smi_larb_mt6873_misc,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt8183 = {
	.has_gals                   = true,
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.larb_direct_to_common_mask = BIT(2) | BIT(3) | BIT(7),
				      /* IPU0 | IPU1 | CCU */
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt5896 = {
	.port_in_larb_gen2 = {2, 4, 12, 2, 12, 4, 2, 14, 15, 10,
			      15, 16, 10, 6, 5, 5, 5, 5, 5, 5,
			      10, 3, 5, 5, 5, 5, 5, 5, 8, 6,
			      4, 12, 6, 7, 7, 3, 5, 5, 5, 5,
			      5, 5, 5, 8, 8, 5, 5, 6, 5, 5,
			      5, 5, 8},
	.has_gals                   = false,
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt5896_bwl,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt5876 = {
	.port_in_larb_gen2 = {16, 15, 11, 13, 12, 19,
			      31, 14, 12, 10, 7, 5, 4},
	.has_gals                   = false,
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt5876_bwl,
};

static const struct mtk_smi_larb_gen mtk_smi_larb_mt5879 = {
	.port_in_larb_gen2 = {15, 17, 10, 13, 12, 18,
			      29, 14, 11, 10, 7, 2, 5},
	.has_gals                   = false,
	.config_port                = mtk_smi_larb_config_port_gen2_general,
	.has_bwl                    = true,
	.bwl                        = (u8 *)mtk_smi_larb_mt5879_bwl,
};

static const struct of_device_id mtk_smi_larb_of_ids[] = {
	{
		.compatible = "mediatek,mt8173-smi-larb",
		.data = &mtk_smi_larb_mt8173
	},
	{
		.compatible = "mediatek,mt2701-smi-larb",
		.data = &mtk_smi_larb_mt2701
	},
	{
		.compatible = "mediatek,mt6873-smi-larb",
		.data = &mtk_smi_larb_mt6873
	},
	{
		.compatible = "mediatek,mt2712-smi-larb",
		.data = &mtk_smi_larb_mt2712
	},
	{
		.compatible = "mediatek,mt8183-smi-larb",
		.data = &mtk_smi_larb_mt8183
	},
	{
		.compatible = "mediatek,mt5896-smi-larb",
		.data = &mtk_smi_larb_mt5896
	},
	{
		.compatible = "mediatek,mt5876-smi-larb",
		.data = &mtk_smi_larb_mt5876
	},
	{
		.compatible = "mediatek,mt5879-smi-larb",
		.data = &mtk_smi_larb_mt5879
	},
	{}
};

static bool ostdl_set_deny;
module_param(ostdl_set_deny, bool, 0644);
MODULE_PARM_DESC(ostdl_set_deny, "deny ostdl dynamic setting\n");
/*
 * Provide for smi larb ostdl dynamic setting
 * @larb, port: target larb/port
 * @ostdl: target ostdl value, 0 is not allowed and then reset to default
 */
int mtk_smi_larb_ostdl(u32 larb, u32 port, u32 ostdl)
{
	int ret = 0, count;
	struct device_node *np;
	char dev_name[MAX_SMILARB_NAME_LEN];
	void __iomem *base;
	struct platform_device *pdev;
	struct mtk_smi_larb_gen *larb_data;
	u8 ostdl_ori;

	pr_info("[MTK-SMI][%d-%d-%d]%pS\n",
		larb, port, ostdl, __builtin_return_address(0));
	if (ostdl_set_deny)
		return -EACCES;

	count = snprintf(dev_name, MAX_SMILARB_NAME_LEN, "larb%u", larb);

	if (count >= MAX_SMILARB_NAME_LEN || count <= 0)
		return -EINVAL;

	if (larb >= SMI_LARB_PORT_NR_MAX) {
		pr_err("[MTK-SMI]bad larb: %u-%u\n", larb, port);
		return -ENODEV;
	}

	np = of_find_node_by_name(NULL, dev_name);
	if (!np) {
		pr_err("[MTK-SMI]larb node not fount%u-%u\n", larb, port);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		pr_err("[MTK-SMI]larb dev not found: %u-%u\n", larb, port);
		ret = -ENODEV;
		goto ret_devnode;
	}

	larb_data = (struct mtk_smi_larb_gen *)of_device_get_match_data(&pdev->dev);
	if (port >= larb_data->port_in_larb_gen2[larb]) {
		pr_err("[MTK-SMI]larb port not found: %u-%u\n", larb, port);
		goto ret_devnode;
	}

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("[MTK-SMI]%pOF: larb bank failed\n", np);
		ret = PTR_ERR(base);
		goto ret_devnode;
	}

	ostdl_ori = larb_data->bwl[SMI_LARB_PORT_NR_MAX * larb + port];

	ret = ostdl_ori;

	if (ostdl > 0 && ostdl <= LARB_MAX_OSTDL)
		writel(ostdl, base + SMI_LARB_OSTDL_PORTx(port));
	else {
		pr_err("[MTK-SMI]ostdl:0x%x then set to org:0x%x\n", ostdl, ostdl_ori);
		writel(ostdl_ori, base + SMI_LARB_OSTDL_PORTx(port));
	}

	iounmap(base);
ret_devnode:
	of_node_put(np);

	return ret;

}
EXPORT_SYMBOL(mtk_smi_larb_ostdl);

static int mtk_smi_larb_probe(struct platform_device *pdev)
{
	struct mtk_smi_larb *larb;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *smi_node;
	struct platform_device *smi_pdev;
	struct device_link *link;
	int ret;

	larb = devm_kzalloc(dev, sizeof(*larb), GFP_KERNEL);
	if (!larb)
		return -ENOMEM;

	larb->larb_gen = of_device_get_match_data(dev);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	larb->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(larb->base))
		return PTR_ERR(larb->base);

#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
	larb->smi.clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(larb->smi.clk_apb))
		return PTR_ERR(larb->smi.clk_apb);

	larb->smi.clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(larb->smi.clk_smi))
		return PTR_ERR(larb->smi.clk_smi);

	if (larb->larb_gen->has_gals) {
		/* The larbs may still haven't gals even if the SoC support.*/
		larb->smi.clk_gals0 = devm_clk_get(dev, "gals");
		if (PTR_ERR(larb->smi.clk_gals0) == -ENOENT)
			larb->smi.clk_gals0 = NULL;
		else if (IS_ERR(larb->smi.clk_gals0))
			return PTR_ERR(larb->smi.clk_gals0);
	}
#endif
	larb->smi.dev = dev;

	smi_node = of_parse_phandle(dev->of_node, "mediatek,smi", 0);
	if (!smi_node)
		return -EINVAL;

	smi_pdev = of_find_device_by_node(smi_node);
	of_node_put(smi_node);
	if (smi_pdev) {
		if (!platform_get_drvdata(smi_pdev))
			return -EPROBE_DEFER;
		larb->smi_common_dev = &smi_pdev->dev;
		link = device_link_add(dev, larb->smi_common_dev,
				       DL_FLAG_PM_RUNTIME);
		if (!link) {
			dev_notice(dev, "Unable to link smi_common device\n");
			return -ENODEV;
		}
	} else {
		dev_err(dev, "Failed to get the smi_common device\n");
		return -EINVAL;
	}

	pm_runtime_enable(dev);
	platform_set_drvdata(pdev, larb);
	if (of_property_read_bool(dev->of_node, "decentralized-m4u"))
		ret = of_property_read_u32(dev->of_node,
				"mediatek,larb-id", &(larb->larbid));
	else
		ret = component_add(dev, &mtk_smi_larb_component_ops);

	if (of_property_read_bool(dev->of_node, "init-power-on"))
		pm_runtime_get_sync(dev);

	return ret;
}

static int mtk_smi_larb_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	component_del(&pdev->dev, &mtk_smi_larb_component_ops);
	return 0;
}

static int __maybe_unused mtk_smi_larb_resume(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	const struct mtk_smi_larb_gen *larb_gen = larb->larb_gen;
	int ret;

	/* Power on smi-common. */
	ret = pm_runtime_get_sync(larb->smi_common_dev);
	if (ret < 0) {
		dev_err(dev, "Failed to pm get for smi-common(%d).\n", ret);
		return ret;
	}

#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
	ret = mtk_smi_clk_enable(&larb->smi);
	if (ret < 0) {
		dev_err(dev, "Failed to enable clock(%d).\n", ret);
		pm_runtime_put_sync(larb->smi_common_dev);
		return ret;
	}
#endif
	if (larb_gen->sleep_ctrl)
		larb_gen->sleep_ctrl(dev, false);

	/* Configure the basic setting for this larb */
	larb_gen->config_port(dev);

	return 0;
}

static int __maybe_unused mtk_smi_larb_suspend(struct device *dev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	const struct mtk_smi_larb_gen *larb_gen = larb->larb_gen;

	if (larb_gen->sleep_ctrl)
		larb_gen->sleep_ctrl(dev, true);

#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
	mtk_smi_clk_disable(&larb->smi);
#endif
	pm_runtime_put_sync(larb->smi_common_dev);
	return 0;
}

static const struct dev_pm_ops smi_larb_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_smi_larb_suspend, mtk_smi_larb_resume, NULL)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				     pm_runtime_force_resume)
};

static struct platform_driver mtk_smi_larb_driver = {
	.probe	= mtk_smi_larb_probe,
	.remove	= mtk_smi_larb_remove,
	.driver	= {
		.name = "mtk-smi-larb",
		.of_match_table = mtk_smi_larb_of_ids,
		.pm             = &smi_larb_pm_ops,
	}
};

static u16 mtk_smi_common_mt6873_bwl[SMI_COMMON_LARB_NR_MAX] = {
	0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000,
};

static u16 mtk_smi_common_mt5896_bwl[SMI_COMMON_NR_MAX][SMI_COMMON_LARB_NR_MAX] = {
	{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
	{0x13fa, 0x11f9, 0x1100, 0x13f2, 0x1100, 0x0000, 0x0000, 0x0000},
	{0x11e1, 0x10f1, 0x10f1, 0x11e1, 0x13c1, 0x11e1, 0x1100, 0x11e1},
	{0x11e1, 0x11e1, 0x11e1, 0x11e1, 0x11e1, 0x11e1, 0x11e1, 0x13c1},
	{0x13f2, 0x1100, 0x11f9, 0x11f9, 0x169c, 0x1179, 0x1179, 0x13f2},
	{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
};

static u16 mtk_smi_common_mt5876_bwl[SMI_COMMON_NR_MAX][SMI_COMMON_LARB_NR_MAX] = {
	{0x1562, 0x1566, 0x10ed, 0x1172, 0x10f1, 0x11e1, 0x10f1, 0x11e1},
	{0x11e1, 0x11e1, 0x1100, 0x11e1, 0x11e1, 0x11e1, 0x11e1, 0x1100},
	{0x12b2, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x1553, 0x11e1},
};

static u16 mtk_smi_common_mt5879_bwl[SMI_COMMON_NR_MAX][SMI_COMMON_LARB_NR_MAX] = {
	{0x1a16, 0x0909, 0x11b6, 0x1a70, 0x11e1, 0x11e1, 0x1100, 0x11e1},
	{0x11e1, 0x11e1, 0x1540, 0x11e1, 0x11e1, 0x11e1, 0x1100, 0x1100},
	{0x1320, 0x1540, 0x1100, 0x1100, 0x1100, 0x1100, 0x169f, 0x11e1},
};

static struct mtk_smi_reg_pair
mtk_smi_common_mt6873_misc[SMI_COMMON_MISC_NR] = {
	{SMI_L1LEN, 0xb},
	{SMI_M4U_TH, 0xe100e10},
	{SMI_FIFO_TH1, 0x90a090a},
	{SMI_FIFO_TH2, 0x506090a},
	{SMI_DCM, 0x4f1},
	{SMI_DUMMY, 0x1},
};

static const struct mtk_smi_common_plat mtk_smi_common_gen1 = {
	.gen = MTK_SMI_GEN1,
};

static const struct mtk_smi_common_plat mtk_smi_common_gen2 = {
	.gen = MTK_SMI_GEN2,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt6873 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(4) |
		    F_MMU1_LARB(5) | F_MMU1_LARB(7),
	.has_bwl  = true,
	.bwl      = mtk_smi_common_mt6873_bwl,
	.misc     = mtk_smi_common_mt6873_misc,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt8183 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = true,
	.bus_sel  = F_MMU1_LARB(1) | F_MMU1_LARB(2) | F_MMU1_LARB(5) |
		    F_MMU1_LARB(7),
};

static const struct mtk_smi_common_plat mtk_smi_common_mt5896 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = false,
	.bus_sel  = false,
	.has_bwl  = true,
	.bwl      = (u16 *)mtk_smi_common_mt5896_bwl,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt5876 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = false,
	.bus_sel  = false,
	.has_bwl  = true,
	.bwl      = (u16 *)mtk_smi_common_mt5876_bwl,
};

static const struct mtk_smi_common_plat mtk_smi_common_mt5879 = {
	.gen      = MTK_SMI_GEN2,
	.has_gals = false,
	.bus_sel  = false,
	.has_bwl  = true,
	.bwl      = (u16 *)mtk_smi_common_mt5879_bwl,
};

static const struct of_device_id mtk_smi_common_of_ids[] = {
	{
		.compatible = "mediatek,mt8173-smi-common",
		.data = &mtk_smi_common_gen2,
	},
	{
		.compatible = "mediatek,mt2701-smi-common",
		.data = &mtk_smi_common_gen1,
	},
	{
		.compatible = "mediatek,mt2712-smi-common",
		.data = &mtk_smi_common_gen2,
	},
	{
		.compatible = "mediatek,mt6873-smi-common",
		.data = &mtk_smi_common_mt6873,
	},
	{
		.compatible = "mediatek,mt8183-smi-common",
		.data = &mtk_smi_common_mt8183,
	},
	{
		.compatible = "mediatek,mt5896-smi-common",
		.data = &mtk_smi_common_mt5896,
	},
	{
		.compatible = "mediatek,mt5876-smi-common",
		.data = &mtk_smi_common_mt5876,
	},
	{
		.compatible = "mediatek,mt5879-smi-common",
		.data = &mtk_smi_common_mt5879,
	},

	{}
};

static int mtk_smi_common_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_smi *common;
	struct resource *res;
	int ret;
	u32 id;

	common = devm_kzalloc(dev, sizeof(*common), GFP_KERNEL);
	if (!common)
		return -ENOMEM;
	common->dev = dev;
	common->plat = of_device_get_match_data(dev);
	if (!common->plat)
		return -EINVAL;

#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
	common->clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(common->clk_apb))
		return PTR_ERR(common->clk_apb);

	common->clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(common->clk_smi))
		return PTR_ERR(common->clk_smi);

	if (common->plat->has_gals) {
		common->clk_gals0 = devm_clk_get(dev, "gals0");
		if (IS_ERR(common->clk_gals0))
			return PTR_ERR(common->clk_gals0);

		common->clk_gals1 = devm_clk_get(dev, "gals1");
		if (IS_ERR(common->clk_gals1))
			return PTR_ERR(common->clk_gals1);
	}
#endif
	/*
	 * for mtk smi gen 1, we need to get the ao(always on) base to config
	 * m4u port, and we need to enable the aync clock for transform the smi
	 * clock into emi clock domain, but for mtk smi gen2, there's no smi ao
	 * base.
	 */
	if (common->plat->gen == MTK_SMI_GEN1) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		common->smi_ao_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(common->smi_ao_base))
			return PTR_ERR(common->smi_ao_base);

		common->clk_async = devm_clk_get(dev, "async");
		if (IS_ERR(common->clk_async))
			return PTR_ERR(common->clk_async);

		ret = clk_prepare_enable(common->clk_async);
		if (ret)
			return ret;
	} else {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		common->base = devm_ioremap_resource(dev, res);
		if (IS_ERR(common->base))
			return PTR_ERR(common->base);
	}
	if (!of_property_read_u32(dev->of_node, "mediatek,common-id", &(id)))
		common->common_id = id;

	pm_runtime_enable(dev);
	platform_set_drvdata(pdev, common);

	return 0;
}

static int mtk_smi_common_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static int __maybe_unused mtk_smi_common_resume(struct device *dev)
{
	struct mtk_smi *common = dev_get_drvdata(dev);
	u32 bus_sel = common->plat->bus_sel;
	int i, ret;

#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
	ret = mtk_smi_clk_enable(common);
	if (ret) {
		dev_err(common->dev, "Failed to enable clock(%d).\n", ret);
		return ret;
	}

	if (common->plat->gen == MTK_SMI_GEN2 && bus_sel)
		writel(bus_sel, common->base + SMI_BUS_SEL);
	if (common->plat->gen != MTK_SMI_GEN2 || !common->plat->has_bwl)
		return 0;
	for (i = 0; i < SMI_COMMON_LARB_NR_MAX; i++)
		writel_relaxed(common->plat->bwl[i],
			common->base + SMI_L1ARB(i));
	for (i = 0; i < SMI_COMMON_MISC_NR; i++)
		writel_relaxed(common->plat->misc[i].value,
			common->base + common->plat->misc[i].offset);
#else
	ret = 0;
	if (common->plat->gen == MTK_SMI_GEN2 && bus_sel)
		writel(bus_sel, common->base + SMI_BUS_SEL);
	if (common->plat->gen != MTK_SMI_GEN2 || !common->plat->has_bwl)
		return 0;
	for (i = 0; i < SMI_COMMON_LARB_NR_MAX; i++)
		writel_relaxed(common->plat->bwl[common->common_id
				* SMI_COMMON_LARB_NR_MAX + i],
				common->base + SMI_L1ARB(i));
#endif
	wmb(); /* make sure settings are written */

	return 0;
}

static int __maybe_unused mtk_smi_common_suspend(struct device *dev)
{
#if !defined(CONFIG_ARCH_MEDIATEK_DTV)
	struct mtk_smi *common = dev_get_drvdata(dev);

	mtk_smi_clk_disable(common);
#endif
	return 0;
}

static const struct dev_pm_ops smi_common_pm_ops = {
	SET_RUNTIME_PM_OPS(mtk_smi_common_suspend, mtk_smi_common_resume, NULL)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				     pm_runtime_force_resume)
};

static struct platform_driver mtk_smi_common_driver = {
	.probe	= mtk_smi_common_probe,
	.remove = mtk_smi_common_remove,
	.driver	= {
		.name = "mtk-smi-common",
		.of_match_table = mtk_smi_common_of_ids,
		.pm             = &smi_common_pm_ops,
	}
};

static int __init mtk_smi_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_smi_common_driver);
	if (ret != 0) {
		pr_err("Failed to register SMI driver\n");
		return ret;
	}

	ret = platform_driver_register(&mtk_smi_larb_driver);
	if (ret != 0) {
		pr_err("Failed to register SMI-LARB driver\n");
		goto err_unreg_smi;
	}
	return ret;

err_unreg_smi:
	platform_driver_unregister(&mtk_smi_common_driver);
	return ret;
}

module_init(mtk_smi_init);
MODULE_LICENSE("GPL v2");
