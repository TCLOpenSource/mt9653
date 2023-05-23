// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include "mtk_vcodec_dec_pm.h"
#include "mtk_vcodec_util.h"

static struct clk *vdec_sys_clk;
static struct clk_hw *vdec_sys_clk_parent;
static const char *clk_top_vdec_sys_name = "clk_top_vdec_sys";
static u32 clk_top_vdec_sys_sel;
static u32 vdecpll_mode;
static u32 vdec_option;

#define APMCU_RIUBASE 0x1C000000

/* SMI_COMMON REGMAP */
#define X32_SMI_COMMON2_BASE	(APMCU_RIUBASE + 0x19d5000)
#define X32_SMI_COMMON5_BASE	(APMCU_RIUBASE + 0x19d4000)

/* SMI_COMMON REG OFFSET */
#define SMI_L1LEN	0x100
#define SMI_BUS_SEL	0x220

#define SMI_MAP_LEN	0x1000

#define SMI_COM2_L1LEN_VAL	0x00000002
#define SMI_COM5_L1LEN_VAL	0x00000002
#define SMI_COM2_BUS_SEL_VAL	0x0005
#define SMI_COM5_BUS_SEL_VAL	0x0001

/* VDECPLL REGMAP */
#define VDECPLL_RIUBASE		(APMCU_RIUBASE + 0x0200900)
#define VDECPLL_LEN		0x100

#define VDECPLL_0x100480	0x00000
#define VDECPLL_0x100481	0x00001
#define VDECPLL_0x100482	0x00004
#define VDECPLL_0x100483	0x00005
#define VDECPLL_0x100484	0x00008
#define VDECPLL_0x100485	0x00009
#define VDECPLL_0x100488	0x00010
#define VDECPLL_0x100489	0x00011
#define VDECPLL_0x10048b	0x00015
#define VDECPLL_0x10048c	0x00018
#define VDECPLL_0x10048d	0x00019
#define VDECPLL_0x10048e	0x0001c
#define VDECPLL_0x10048f	0x0001d
#define VDECPLL_0x100490	0x00020
#define VDECPLL_0x100491	0x00021

#define VDECPLL_VAL_0x00	0x00
#define VDECPLL_VAL_0x01	0x01
#define VDECPLL_VAL_0x10	0x10
#define VDECPLL_VAL_0x11	0x11
#define VDECPLL_VAL_0x16	0x16
#define VDECPLL_VAL_0x2E	0x2E
#define VDECPLL_VAL_0x34	0x34

#define VDECPLL_DELAY		500//us

#define VDECPLL_MODE_1		1//624MHz
#define VDECPLL_MODE_2		2//552MHz
#define VDECPLL_MODE_3		3//528MHz

typedef unsigned char UINT8;

static void mtk_dec_vdecpll_mode1(bool bEn)
{
	unsigned int *vdecpll_addr = ioremap(VDECPLL_RIUBASE, VDECPLL_LEN);

	if (vdecpll_addr == NULL) {
		pr_err("[ERR]vdecpll addr ioremap Fail");
		return;
	}

	if (bEn) {
		//check whether vdecpll is enabled(0x00) or not(0x11)
		if (*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480)
				== (UINT8)VDECPLL_VAL_0x00) {
			mtk_v4l2_err("vdecpll is already enabled, disable vdecpll first");
			*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) =
				(UINT8)VDECPLL_VAL_0x11;
			while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480))
			!= (UINT8)VDECPLL_VAL_0x11)
				cpu_relax();
		}
		//check BRINGUP_VDECPLL.txt
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100488) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100489) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048c) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048d) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048e) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048f) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100490) = (UINT8)VDECPLL_VAL_0x34;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100491) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x10;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481) = (UINT8)VDECPLL_VAL_0x11;
		udelay(VDECPLL_DELAY);
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481) = (UINT8)VDECPLL_VAL_0x11;
		while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481))
		!= (UINT8)VDECPLL_VAL_0x11)
			cpu_relax();
	} else {
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x11;
		while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480))
		!= (UINT8)VDECPLL_VAL_0x11)
			cpu_relax();
	}
	iounmap(vdecpll_addr);
}

static void mtk_dec_vdecpll_mode2(bool bEn)
{
	unsigned int *vdecpll_addr = ioremap(VDECPLL_RIUBASE, VDECPLL_LEN);

	if (vdecpll_addr == NULL) {
		pr_err("[ERR]vdecpll addr ioremap Fail");
		return;
	}

	if (bEn) {
		//check whether vdecpll is enabled(0x00) or not(0x11)
		if (*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480)
				== (UINT8)VDECPLL_VAL_0x00) {
			mtk_v4l2_err("vdecpll is already enabled, disable vdecpll first");
			*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) =
				(UINT8)VDECPLL_VAL_0x11;
			while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480))
			!= (UINT8)VDECPLL_VAL_0x11)
				cpu_relax();
		}
		//check BRINGUP_VDECPLL.txt
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100488) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100489) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048c) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048d) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048e) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048f) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100490) = (UINT8)VDECPLL_VAL_0x2E;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100491) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x10;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481) = (UINT8)VDECPLL_VAL_0x11;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100482) = (UINT8)VDECPLL_VAL_0x01;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100483) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100484) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100485) = (UINT8)VDECPLL_VAL_0x00;
		udelay(VDECPLL_DELAY);
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x00;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481) = (UINT8)VDECPLL_VAL_0x11;
		while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481))
		!= (UINT8)VDECPLL_VAL_0x11)
			cpu_relax();
	} else {
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x11;
		while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480))
		!= (UINT8)VDECPLL_VAL_0x11)
			cpu_relax();
	}
	iounmap(vdecpll_addr);
}

static void mtk_dec_vdecpll_mode3(bool bEn)
{
	unsigned int *vdecpll_addr = ioremap(VDECPLL_RIUBASE, VDECPLL_LEN);

	if (vdecpll_addr == NULL) {
		pr_err("[ERR]vdecpll addr ioremap Fail");
		return;
	}

	//0x100480[0]: PLL pd. enable set 0
	//0x100480[4]: PLL div1 pd, disable set 1
	//0x100481[0]: PLL div2 pd, enable set 0 -> mode 3
	//0x100481[4]: PLL div3 pd, disable set 1
	if (bEn) {
		//check whether vdecpll is enabled(0x10) or not(0x11)
		if (*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480)
				== (UINT8)VDECPLL_VAL_0x10) {
			mtk_v4l2_err("vdecpll is already enabled, disable vdecpll first");
			*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) =
				(UINT8)VDECPLL_VAL_0x11;
			while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480))
			!= (UINT8)VDECPLL_VAL_0x11)
				cpu_relax();
		}
		//check BRINGUP_VDECPLL.txt
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048b) = (UINT8)VDECPLL_VAL_0x01;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x10048e) = (UINT8)VDECPLL_VAL_0x01;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100490) = (UINT8)VDECPLL_VAL_0x16;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100482) = (UINT8)VDECPLL_VAL_0x01;
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x10;
		udelay(VDECPLL_DELAY);
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481) = (UINT8)VDECPLL_VAL_0x10;
		while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100481))
		!= (UINT8)VDECPLL_VAL_0x10)
			cpu_relax();
	} else {
		*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480) = (UINT8)VDECPLL_VAL_0x11;
		while ((*(UINT8 *)(((uintptr_t)vdecpll_addr) + VDECPLL_0x100480))
		!= (UINT8)VDECPLL_VAL_0x11)
			cpu_relax();
	}
	iounmap(vdecpll_addr);
}


#ifdef CONFIG_MSTAR_ARM_BD_FPGA
static void mtk_dec_init_smi(void)
{
	unsigned int *smi_com2 = ioremap(X32_SMI_COMMON2_BASE, SMI_MAP_LEN);
	unsigned int *smi_com5 = ioremap(X32_SMI_COMMON5_BASE, SMI_MAP_LEN);

	if (smi_com2 == NULL || smi_com5 == NULL) {
		mtk_v4l2_err("[ERR]smi addr ioremap Fail");
		return;
	}

	/* [0]:0 Disable WARB channel and use arbiter in SMI for wdata (old mode)*/
	*(u32 *)(((uintptr_t)smi_com2) + SMI_L1LEN) = SMI_COM2_L1LEN_VAL;
	while ((*(u32 *)(((uintptr_t)smi_com2) + SMI_L1LEN)) != SMI_COM2_L1LEN_VAL)
		cpu_relax();
	*(u32 *)(((uintptr_t)smi_com5) + SMI_L1LEN) = SMI_COM5_L1LEN_VAL;
	while ((*(u32 *)(((uintptr_t)smi_com5) + SMI_L1LEN)) != SMI_COM5_L1LEN_VAL)
		cpu_relax();
	/* bus select */
	*(u32 *)(((uintptr_t)smi_com2) + SMI_BUS_SEL) = SMI_COM2_BUS_SEL_VAL;
	while ((*(u32 *)(((uintptr_t)smi_com2) + SMI_BUS_SEL)) != SMI_COM2_BUS_SEL_VAL)
		cpu_relax();
	*(u32 *)(((uintptr_t)smi_com5) + SMI_BUS_SEL) = SMI_COM5_BUS_SEL_VAL;
	while ((*(u32 *)(((uintptr_t)smi_com5) + SMI_BUS_SEL)) != SMI_COM5_BUS_SEL_VAL)
		cpu_relax();

	iounmap(smi_com2);
	iounmap(smi_com5);
}
#endif

void mtk_dec_init_ctx_pm(struct mtk_vcodec_ctx *ctx)
{
}

int mtk_vcodec_init_dec_pm(struct mtk_vcodec_dev *mtkdev)
{
	int i = 0, ret = 0;
	struct platform_device *pdev = mtkdev->plat_dev;
	struct mtk_vcodec_pm *pm = &mtkdev->pm;

#ifdef CONFIG_MSTAR_ARM_BD_FPGA
	//init smi larb
	u32 init_smi = 0;

	ret = of_property_read_u32(pdev->dev.of_node, "INIT_SMI", &init_smi);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get INIT_SMI. ret %d\n", ret);
		return -EINVAL;
	}
	if (init_smi)
		mtk_dec_init_smi();
#endif

	ret = of_property_read_u32(pdev->dev.of_node, "VDECPLL_MODE", &vdecpll_mode);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get VDECPLL_MODE. ret %d\n", ret);
		return -EINVAL;
	}

	//vdecpll handle
	if (vdecpll_mode == VDECPLL_MODE_1)
		mtk_dec_vdecpll_mode1(true);
	else if (vdecpll_mode == VDECPLL_MODE_2)
		mtk_dec_vdecpll_mode2(true);
	else if (vdecpll_mode == VDECPLL_MODE_3)
		mtk_dec_vdecpll_mode3(true);
	else {
		dev_err(&pdev->dev, "VDECPLL_MODE not support\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "CLK_COUNT", &pm->vdec_clk_count);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get CLK_COUNT. ret %d\n", ret);
		return -EINVAL;
	}

	if (pm->vdec_clk_count > E_VDEC_CLK_VDEC_NUM) {
		dev_err(&pdev->dev, "Failed! clk count %d is too large\n", pm->vdec_clk_count);
		return -EINVAL;
	}

	for (i = 0; i < pm->vdec_clk_count; i++) {
		if (!pm->vdec_clk[i]) {
			ret = of_property_read_string_index(pdev->dev.of_node,
						"clock-names", i, &pm->vdec_clkname[i]);
			if (ret < 0) {
				dev_info(&pdev->dev, "get %d clk name failed ret %d\n", i, ret);
				pm->vdec_clk[i] = NULL;
				continue;
			}

			pm->vdec_clk[i] = devm_clk_get(&pdev->dev, pm->vdec_clkname[i]);
			if (IS_ERR(pm->vdec_clk[i])) {
				mtk_v4l2_err("get clk[%d] %s fail", i, pm->vdec_clkname[i]);
				pm->vdec_clk[i] = NULL;
				continue;
			}

			//clk_top_vdec_sys
			if ((strlen(pm->vdec_clkname[i]) == strlen(clk_top_vdec_sys_name))
			&& (strncmp(pm->vdec_clkname[i], clk_top_vdec_sys_name,
						strlen(clk_top_vdec_sys_name)) == 0)) {
				ret = of_property_read_u32(pdev->dev.of_node,
						"VDEC_SYS_CLK_SEL", &clk_top_vdec_sys_sel);
				if (ret < 0) {
					dev_err(&pdev->dev, "Failed to get CLK_SEL ret %d\n", ret);
					return -EINVAL;
				}
				vdec_sys_clk = pm->vdec_clk[i];
				vdec_sys_clk_parent =
				clk_hw_get_parent_by_index(__clk_get_hw(vdec_sys_clk),
					clk_top_vdec_sys_sel);
				if (IS_ERR_OR_NULL(vdec_sys_clk_parent))
					mtk_v4l2_err("target_parent of clk_top_vdec_sys get fail");
			}
		}
	}

	ret = of_property_read_u32(pdev->dev.of_node, "VDEC_OPTION_SEL", &vdec_option);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get VDEC_OPTION_SEL. ret %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

void mtk_vcodec_release_dec_pm(struct mtk_vcodec_dev *dev)
{
	struct platform_device *pdev;
	struct mtk_vcodec_pm *pm;
	int i = 0;

	pdev = dev->plat_dev;
	pm = &dev->pm;

	for (i = 0; i < E_VDEC_CLK_VDEC_NUM; i++) {
		if (pm->vdec_clk[i])
			devm_clk_put(&pdev->dev, pm->vdec_clk[i]);
	}

	//vdecpll handle
	if (vdecpll_mode == VDECPLL_MODE_1)
		mtk_dec_vdecpll_mode1(false);
	else if (vdecpll_mode == VDECPLL_MODE_2)
		mtk_dec_vdecpll_mode2(false);
	else if (vdecpll_mode == VDECPLL_MODE_3)
		mtk_dec_vdecpll_mode3(false);
	else
		mtk_v4l2_err("VDECPLL_MODE not support");

}

void mtk_vcodec_resume_dec_pm(struct mtk_vcodec_dev *mtkdev)
{
	//vdecpll handle
	if (vdecpll_mode == VDECPLL_MODE_1)
		mtk_dec_vdecpll_mode1(true);
	else if (vdecpll_mode == VDECPLL_MODE_2)
		mtk_dec_vdecpll_mode2(true);
	else if (vdecpll_mode == VDECPLL_MODE_3)
		mtk_dec_vdecpll_mode3(true);
	else
		mtk_v4l2_err("VDECPLL_MODE not support");
}

void mtk_vcodec_dec_pw_on(struct mtk_vcodec_pm *pm, int hw_id)
{
}

void mtk_vcodec_dec_pw_off(struct mtk_vcodec_pm *pm, int hw_id)
{
}

void mtk_vcodec_dec_clock_on(struct mtk_vcodec_pm *pm, int hw_id)
{
	int i = 0, ret = 0;

	mtk_v4l2_debug(VCODEC_DBG_L4, "hw_id %d", hw_id);

	//clk_top_vdec_sys need mux to right sel
	if (vdec_sys_clk_parent) {
		ret = clk_set_parent(vdec_sys_clk, vdec_sys_clk_parent->clk);
		if (ret < 0)
			mtk_v4l2_err("set topgen clk fails ret %d", ret);
	}

	for (i = 0; i < pm->vdec_clk_count; i++) {
		if (pm->vdec_clk[i]) {
			ret = clk_prepare_enable(pm->vdec_clk[i]);
			if (ret < 0)
				mtk_v4l2_err("enable clk %d ret %d", i, ret);
		}
	}
}

void mtk_vcodec_dec_clock_off(struct mtk_vcodec_pm *pm, int hw_id)
{
	int i = 0;

	mtk_v4l2_debug(VCODEC_DBG_L4, "hw_id %d", hw_id);
	for (i = 0; i < pm->vdec_clk_count; i++) {
		if (pm->vdec_clk[i])
			clk_disable_unprepare(pm->vdec_clk[i]);
	}
}

void mtk_prepare_vdec_dvfs(void)
{
}

void mtk_unprepare_vdec_dvfs(void)
{
}

void mtk_prepare_vdec_emi_bw(void)
{
}

void mtk_unprepare_vdec_emi_bw(void)
{
}

void mtk_vdec_pmqos_prelock(struct mtk_vcodec_ctx *ctx, int hw_id)
{
}

void mtk_vdec_pmqos_begin_frame(struct mtk_vcodec_ctx *ctx, int hw_id)
{
}

void mtk_vdec_pmqos_end_frame(struct mtk_vcodec_ctx *ctx, int hw_id)
{
}
