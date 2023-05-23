// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/iopoll.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "apusys_power_user.h"
#include "apu_devfreq.h"
#include "apu_clk.h"
#include "apu_io.h"
#include "apu_log.h"
#include "apu_common.h"
#include "apu_dbg.h"
#include "plat_mt589x_reg.h"
#include "plat_util.h"
#include "plat_teec_api.h"

static DEFINE_MUTEX(hw_lock);
static unsigned long power_on_cnt;

#define riu_read16(name, offset)	\
		ioread16(reg_##name + (offset << 2))

#define riu_write16(name, offset, val)	\
		iowrite16(val, reg_##name + (offset << 2))

#define riu_read32(name, offset)	\
		ioread32(reg_##name + (offset << 2))

#define riu_write32(name, offset, val)	\
		iowrite32(val, reg_##name + (offset << 2))

#define RIU2IOMAP(bank, offset)	((phys_addr_t)RIU_ADDR(bank, offset))

static int mtcmos_init_done;
static u32 slpprot_rdy;
static u32 vcore_sw_rst;

#ifndef MDLA_POWER_CTRL_BY_TEE

static void __iomem *reg_x32_aia_top;
static void __iomem *reg_iommu_local_ssc3;
static void __iomem *reg_iommu_local_ssc4;
static void __iomem *reg_mcusys_xiu_bridge;
static void __iomem *reg_apusys_vcore_cg_clr;
static void __iomem *reg_apu_conn_cg_clr;
static void __iomem *reg_apu_mdla0_apu_mdla_cg_clr;
static void __iomem *reg_apu_sctrl_pgrmp_sctrl_mdla0_ctxt;
static void __iomem *reg_apu_sctrl_pgrmp_sctrl_edma0_ctxt;
static void __iomem *reg_apu_conn_mbist_delsel;
static void __iomem *reg_apu_mdla_mbist_delsel;
static void __iomem *reg_0x2828;

static void ioremap_uninit(void)
{
	if (reg_x32_aia_top)
		iounmap(reg_x32_aia_top);
	if (reg_iommu_local_ssc3)
		iounmap(reg_iommu_local_ssc3);
	if (reg_iommu_local_ssc4)
		iounmap(reg_iommu_local_ssc4);
	if (reg_mcusys_xiu_bridge)
		iounmap(reg_mcusys_xiu_bridge);
	if (reg_apusys_vcore_cg_clr)
		iounmap(reg_apusys_vcore_cg_clr);
	if (reg_apu_conn_cg_clr)
		iounmap(reg_apu_conn_cg_clr);
	if (reg_apu_mdla0_apu_mdla_cg_clr)
		iounmap(reg_apu_mdla0_apu_mdla_cg_clr);
	if (reg_apu_sctrl_pgrmp_sctrl_mdla0_ctxt)
		iounmap(reg_apu_sctrl_pgrmp_sctrl_mdla0_ctxt);
	if (reg_apu_sctrl_pgrmp_sctrl_edma0_ctxt)
		iounmap(reg_apu_sctrl_pgrmp_sctrl_edma0_ctxt);
	if (reg_apu_conn_mbist_delsel)
		iounmap(reg_apu_conn_mbist_delsel);
	if (reg_apu_mdla_mbist_delsel)
		iounmap(reg_apu_mdla_mbist_delsel);
	if (reg_0x2828)
		iounmap(reg_0x2828);
}

static int ioremap_init(void)
{
	reg_x32_aia_top = ioremap_nocache(RIU2IOMAP(X32_AIA_TOP, 0), PAGE_SIZE);
	reg_iommu_local_ssc3 = ioremap_nocache(RIU2IOMAP(IOMMU_LOCAL_SSC3, 0), PAGE_SIZE);
	reg_iommu_local_ssc4 = ioremap_nocache(RIU2IOMAP(IOMMU_LOCAL_SSC4, 0), PAGE_SIZE);
	reg_mcusys_xiu_bridge = ioremap_nocache(RIU2IOMAP(MCUSYS_XIU_BRIDGE, 0), PAGE_SIZE);
	reg_apusys_vcore_cg_clr = ioremap_nocache(APUSYS_VCORE_CG_CLR, PAGE_SIZE);
	reg_apu_conn_cg_clr = ioremap_nocache(APU_CONN_CG_CLR, PAGE_SIZE);
	reg_apu_mdla0_apu_mdla_cg_clr = ioremap_nocache(APU_MDLA0_APU_MDLA_CG_CLR, PAGE_SIZE);
	reg_apu_sctrl_pgrmp_sctrl_mdla0_ctxt =
		ioremap_nocache(APU_SCTRL_PGRMP_SCTRL_MDLA_CORE0_CTXT, PAGE_SIZE);
	reg_apu_sctrl_pgrmp_sctrl_edma0_ctxt =
		ioremap_nocache(APU_SCTRL_PGRMP_SCTRL_EDMA0_CTXT, PAGE_SIZE);
	reg_apu_conn_mbist_delsel = ioremap_nocache(APU_CONN_MBIST_DEFAULT_DELSEL, PAGE_SIZE);
	reg_apu_mdla_mbist_delsel = ioremap_nocache(APU_MDLA0_APU_MDLA_MBIST_DEFAULT_DELSEL,
							PAGE_SIZE);
	reg_0x2828 = ioremap_nocache(RIU2IOMAP(0x2828, 0), PAGE_SIZE);
	if (!reg_x32_aia_top
		|| !reg_iommu_local_ssc3
		|| !reg_iommu_local_ssc4
		|| !reg_mcusys_xiu_bridge
		|| !reg_apusys_vcore_cg_clr
		|| !reg_apu_conn_cg_clr
		|| !reg_apu_mdla0_apu_mdla_cg_clr
		|| !reg_apu_sctrl_pgrmp_sctrl_mdla0_ctxt
		|| !reg_apu_sctrl_pgrmp_sctrl_edma0_ctxt
		|| !reg_apu_conn_mbist_delsel
		|| !reg_apu_mdla_mbist_delsel
		|| !reg_0x2828)
		return -ENOMEM;

	return 0;
}
#endif // MDLA_POWER_CTRL_BY_TEE

int mt589x_mtcmos_init(struct apu_dev *ad)
{
	if (!mtcmos_init_done) {
#ifndef MDLA_POWER_CTRL_BY_TEE
		if (ioremap_init()) {
			aprobe_err(ad->dev, "%s: ioremap_init fail\n", __func__);
			return -ENOMEM;
		}
#endif
		if (plat_teec_send_command(ad, E_MDLA_PWR_CMD_INIT_POWER, 0, NULL)) {
			aprobe_err(ad->dev, "%s: teec_power_init fail\n", __func__);
			return -EFAULT;
		}

		vcore_sw_rst = 1;
		slpprot_rdy = 0;
		mtcmos_init_done = 1;
	}

	return 0;
}

void mt589x_mtcmos_uninit(struct apu_dev *ad)
{
	if (mtcmos_init_done) {
#ifndef MDLA_POWER_CTRL_BY_TEE
		ioremap_uninit();
#endif
		mtcmos_init_done = 0;
		plat_teec_send_command(ad, E_MDLA_PWR_CMD_UNINIT_POWER, 0, NULL);
	}
}

#ifndef MDLA_POWER_CTRL_BY_TEE
static int sram_csr_use_default_delsel(void)
{
	//asr_use_default_delsel setting
	iowrite32(ioread32(reg_apu_conn_mbist_delsel) | (0x1 << 0),
			reg_apu_conn_mbist_delsel);

	iowrite32(ioread32(reg_apu_mdla_mbist_delsel) | 0x3,
			reg_apu_mdla_mbist_delsel);

	return 0;
}

static int aia_top_iommu_init(void)
{
	riu_write16(iommu_local_ssc3, 0x7b, 0x0000);
	riu_write16(iommu_local_ssc4, 0x7b, 0x0000);

	return 0;
}

static int aia_top_apusys_init(void)
{
	iowrite32(0xffffffff, reg_apusys_vcore_cg_clr);
	iowrite32(0xffffffff, reg_apu_conn_cg_clr);
	iowrite32(0xffffffff, reg_apu_mdla0_apu_mdla_cg_clr);

	return 0;
}

static int aia_top_mcusys_xiu_bridge_init(void)
{
	riu_write16(mcusys_xiu_bridge, 0x08, 0x0000);

	return 0;
}

static int mdla_core_sctrl(void)
{
	iowrite32((ioread32(reg_apu_sctrl_pgrmp_sctrl_mdla0_ctxt) & ~0x3) | 0x2,
			reg_apu_sctrl_pgrmp_sctrl_mdla0_ctxt);

	iowrite32((ioread32(reg_apu_sctrl_pgrmp_sctrl_edma0_ctxt) & ~0x3) | 0x2,
			reg_apu_sctrl_pgrmp_sctrl_edma0_ctxt);

	return 0;
}

int dtv_apu_mt589x_mtcmos_on(struct apu_dev *ad, int force)
{
	int ret = 0;
	u16 regv;
	u16 mask;
	u32 bit_ofs, reg_idx;

	if (!mtcmos_init_done) {
		if (mt589x_mtcmos_init(ad)) {
			ret = -EFAULT;
			arpc_err(ad->dev, "%s(): init fail\n",
					__func__);
			return -EFAULT;
		}
	}

	mutex_lock(&hw_lock);

	if (!force && power_on_cnt++) {
		arpc_info(ad->dev, "%s(): is on, count:%lu\n",
				__func__, power_on_cnt);
		ret = false;
		goto out;
	}

	// aia enable
	regv = riu_read16(x32_aia_top, 0x00);
	regv |= (0x1 << 4) | (0x1 << 3);
	riu_write16(x32_aia_top, 0x00, regv);

	// VCORE enable
	// slpprot dis
	regv = riu_read16(x32_aia_top, 0x11);
	regv &= ~((0x1 << 9) | (0x1 << 6) | (0x1 << 3) | (0x1 << 0));
	riu_write16(x32_aia_top, 0x11, regv);

	// slpprot rdy
	if (slpprot_rdy) {
		mask = (0x1 << 10) | (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
		while (riu_read16(x32_aia_top, 0x11) & mask)
			;
	}

	// reset release
	regv = riu_read16(x32_aia_top, 0x03);
	// do not set bit 0 which will reset iommu
	regv |= (0x1 << 7) | (0x1 << 6) | (0x1 << 5);
	riu_write16(x32_aia_top, 0x03, regv);

	// vcore sw_rst [2] = 0
	regv = riu_read32(0x2828, 0x03);
	regv &= ~(0x1 << 2);
	riu_write32(0x2828, 0x03, regv);

	//clk on 1
	regv = riu_read16(x32_aia_top, 0x01);
	regv &= ~(0x1 << 0);
	riu_write16(x32_aia_top, 0x01, regv);

	// GSM/Conn power on
	// sram pd dis
	regv = riu_read16(x32_aia_top, 0x06);
	regv &= ~(0x1 << 0);
	riu_write16(x32_aia_top, 0x06, regv);

	// sram pd dis ack
	mask = (0x1 << 1);
	while (riu_read16(x32_aia_top, 0x6) & mask)
		;

	// mem iso disable
	regv = riu_read16(x32_aia_top, 0x04);
	regv &= ~(0x1 << 2);
	riu_write16(x32_aia_top, 0x04, regv);

	// dig iso disable
	regv = riu_read16(x32_aia_top, 0x04);
	regv &= ~((0x1 << 1) | (0x1 << 0));
	riu_write16(x32_aia_top, 0x04, regv);

	// reset release
	regv = riu_read16(x32_aia_top, 0x03);
	regv |= (0x1 << 1);
	riu_write16(x32_aia_top, 0x03, regv);

	//clk on 1
	regv = riu_read16(x32_aia_top, 0x01);
	regv &= ~(0x1 << 1);
	riu_write16(x32_aia_top, 0x01, regv);

	//clk on 2
	regv = riu_read16(x32_aia_top, 0x00);
	regv |= (0x1 << 1);
	riu_write16(x32_aia_top, 0x00, regv);

	// mtcmos on1
	regv = riu_read16(x32_aia_top, 0x05);
	regv |= (0x1 << 0);
	riu_write16(x32_aia_top, 0x05, regv);

	// mtcmos on1 ack
	mask = (0x1 << 2);
	while (!(riu_read16(x32_aia_top, 0x5) & mask))
		;

	// mtcmos on2
	regv = riu_read16(x32_aia_top, 0x05);
	regv |= (0x1 << 1);
	riu_write16(x32_aia_top, 0x05, regv);

	// mtcmos on2 ack
	mask = (0x1 << 3);
	while (!(riu_read16(x32_aia_top, 0x5) & mask))
		;

	// sram pd dis
	for (reg_idx = 0x8; reg_idx <= 0xf; reg_idx++) {
		for (bit_ofs = 0; bit_ofs <= 0xf; bit_ofs++) {
			regv = riu_read16(x32_aia_top, reg_idx);
			regv &= ~(0x1 << bit_ofs);
			riu_write16(x32_aia_top, reg_idx, regv);
		}
	}

	// mem isolation dis
	regv = riu_read16(x32_aia_top, 0x04);
	regv &= ~(0x1 << 4);
	riu_write16(x32_aia_top, 0x04, regv);

	// dig isolation dis
	regv = riu_read16(x32_aia_top, 0x04);
	regv &= ~(0x1 << 3);
	riu_write16(x32_aia_top, 0x04, regv);

	// slpprot dis
	regv = riu_read16(x32_aia_top, 0x10);
	regv &= ~((0x1 << 6) | (0x1 << 3) | (0x1 << 0));
	riu_write16(x32_aia_top, 0x10, regv);

	// slpprot rdy
	mask = (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
	while (riu_read16(x32_aia_top, 0x10) & mask)
		;

	// reset release
	regv = riu_read16(x32_aia_top, 0x03);
	regv |= (0x1 << 4) | (0x1 << 3) | (0x1 << 2);
	riu_write16(x32_aia_top, 0x03, regv);

	//clk on 1
	regv = riu_read16(x32_aia_top, 0x01);
	regv &= ~(0x1 << 2);
	riu_write16(x32_aia_top, 0x01, regv);

	//clk on 2
	regv = riu_read16(x32_aia_top, 0x00);
	regv |= (0x1 << 2);
	riu_write16(x32_aia_top, 0x00, regv);

	aia_top_iommu_init();

	//bypass security check
	regv = riu_read16(x32_aia_top, 0x26);
	regv |= (0x1 << 2);
	riu_write16(x32_aia_top, 0x26, regv);

	aia_top_apusys_init();
	aia_top_mcusys_xiu_bridge_init();
	sram_csr_use_default_delsel();
	mdla_core_sctrl();

	ret = plat_teec_send_command(ad, E_MDLA_PWR_CMD_SET_BOOT_UP, 0, NULL);
	if (ret)
		arpc_err(ad->dev, "%s(): bootup fail, ret = %x\n", __func__, ret);

	arpc_info(ad->dev, "%s(): success\n", __func__);
out:
	mutex_unlock(&hw_lock);

	return ret;
}

void dtv_apu_mt589x_mtcmos_off(struct apu_dev *ad, int force)
{
	int ret;
	u16 regv, mask;
	u32 bit_ofs, reg_idx;

	if (!mtcmos_init_done) {
		arpc_err(ad->dev, "%s(): not init\n", __func__);
		return;
	}

	mutex_lock(&hw_lock);

	if (!power_on_cnt) {
		arpc_info(ad->dev, "%s(): is disabled\n", __func__);
		goto out;
	}

	if (!force && power_on_cnt-- > 1) {
		arpc_info(ad->dev, "%s(): not off, count:%lu\n",
				__func__, power_on_cnt);
		goto out;
	}

	// slpprot en
	regv = riu_read16(x32_aia_top, 0x10);
	regv |= ((0x1 << 6) | (0x1 << 3) | (0x1 << 0));
	riu_write16(x32_aia_top, 0x10, regv);

	// slpprot rdy
	mask = (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
	while ((riu_read16(x32_aia_top, 0x10) & mask) != mask)
		;

	//clk off 1
	regv = riu_read16(x32_aia_top, 0x01);
	regv |= (0x1 << 2);
	riu_write16(x32_aia_top, 0x01, regv);

	//clk off 2
	regv = riu_read16(x32_aia_top, 0x00);
	regv &= ~(0x1 << 2);
	riu_write16(x32_aia_top, 0x00, regv);

	//reset assert
	regv = riu_read16(x32_aia_top, 0x03);
	regv &= ~((0x1 << 4) | (0x1 << 3) | (0x1 << 2));
	riu_write16(x32_aia_top, 0x03, regv);

	//mem isolation en
	regv = riu_read16(x32_aia_top, 0x04);
	regv |= (0x1 << 4);
	riu_write16(x32_aia_top, 0x04, regv);

	//dig isolation en
	regv = riu_read16(x32_aia_top, 0x04);
	regv |= (0x1 << 3);
	riu_write16(x32_aia_top, 0x04, regv);

	// sram pd
	for (reg_idx = 0x8; reg_idx <= 0xf; reg_idx++) {
		for (bit_ofs = 0; bit_ofs <= 0xf; bit_ofs++) {
			regv = riu_read16(x32_aia_top, reg_idx);
			regv |= (0x1 << bit_ofs);
			riu_write16(x32_aia_top, reg_idx, regv);
		}
	}

	// mtcmos off1
	regv = riu_read16(x32_aia_top, 0x05);
	regv &= ~(0x1 << 0);
	riu_write16(x32_aia_top, 0x05, regv);

	// mtcmos off1 ack
	mask = (0x1 << 2);
	while (riu_read16(x32_aia_top, 0x05) & mask)
		;

	// mtcmos off2
	regv = riu_read16(x32_aia_top, 0x05);
	regv &= ~(0x1 << 1);
	riu_write16(x32_aia_top, 0x05, regv);

	// mtcmos off2 ack
	mask = (0x1 << 3);
	while (riu_read16(x32_aia_top, 0x05) & mask)
		;

	//clk off 1
	regv = riu_read16(x32_aia_top, 0x01);
	regv |= (0x1 << 1);
	riu_write16(x32_aia_top, 0x01, regv);

	//clk off 2
	regv = riu_read16(x32_aia_top, 0x00);
	regv &= ~(0x1 << 1);
	riu_write16(x32_aia_top, 0x00, regv);

	// slpprot en
	regv = riu_read16(x32_aia_top, 0x11);
	regv |= (0x1 << 9) | (0x1 << 6) | (0x1 << 3) | (0x1 << 0);
	riu_write16(x32_aia_top, 0x11, regv);

	// slpprot rdy
	mask = (0x1 << 10) | (0x1 << 7) | (0x1 << 4) | (0x1 << 1);
	while ((riu_read16(x32_aia_top, 0x11) & mask) != mask)
		;

	// reset assert
	regv = riu_read16(x32_aia_top, 0x03);
	regv &= ~(0x1 << 1);
	riu_write16(x32_aia_top, 0x03, regv);

	//mem isolation en
	regv = riu_read16(x32_aia_top, 0x04);
	regv |= (0x1 << 2);
	riu_write16(x32_aia_top, 0x04, regv);

	//dig isolation en
	regv = riu_read16(x32_aia_top, 0x04);
	regv |= ((0x1 << 1) | (0x1<<0));
	riu_write16(x32_aia_top, 0x04, regv);

	//sram pd en
	regv = riu_read16(x32_aia_top, 0x06);
	regv |= (0x1 << 0);
	riu_write16(x32_aia_top, 0x06, regv);

	//sram pd en ack
	regv = riu_read16(x32_aia_top, 0x06);
	mask = (0x1 << 1);
	//while (!(riu_read16(x32_aia_top, 0x06) & mask))
	//	;

	//clk off 1
	regv = riu_read16(x32_aia_top, 0x01);
	regv |= (0x1 << 0);
	riu_write16(x32_aia_top, 0x01, regv);

	// vcore sw_rst [2] = 1
	if (vcore_sw_rst) {
		regv = riu_read32(0x2828, 0x03);
		regv |= (0x1 << 2);
		riu_write32(0x2828, 0x03, regv);
	}

	// reset assert: skip imi rst
	regv = riu_read16(x32_aia_top, 0x03);
	regv &= ~((0x1 << 7) | (0x1 << 5));
	riu_write16(x32_aia_top, 0x03, regv);

	// aia disable
	regv = riu_read16(x32_aia_top, 0x00);
	regv &= ~((0x1 << 4) | (0x1 << 3));
	riu_write16(x32_aia_top, 0x00, regv);

	ret = plat_teec_send_command(ad, E_MDLA_PWR_CMD_SET_SHUT_DOWN, 0, NULL);
	if (ret)
		arpc_err(ad->dev, "%s(): shutdown fail, ret = %x\n", __func__, ret);

	arpc_info(ad->dev, "%s(): success\n", __func__);
out:
	mutex_unlock(&hw_lock);
}
#else
int dtv_apu_mt589x_mtcmos_on(struct apu_dev *ad, int force)
{
	int ret = 0;

	if (!mtcmos_init_done) {
		if (mt589x_mtcmos_init(ad)) {
			ret = -EFAULT;
			arpc_err(ad->dev, "%s(): init fail\n",
					__func__);
			goto out;
		}
	}

	mutex_lock(&hw_lock);
	if (!force && power_on_cnt++) {
		arpc_info(ad->dev, "%s(): is on, count:%lu\n",
				__func__, power_on_cnt);
		mutex_unlock(&hw_lock);
		goto out;
	}

	ret = plat_teec_send_command(ad, E_MDLA_PWR_CMD_SET_BOOT_UP, 0, NULL);
	if (ret) {
		arpc_err(ad->dev, "%s(): on fail, ret = %x\n", __func__, ret);
		power_on_cnt--;
		ret = false;
		mutex_unlock(&hw_lock);
		goto out;
	}

	arpc_info(ad->dev, "%s(): success\n", __func__);
	mutex_unlock(&hw_lock);
out:

	return ret;
}

void dtv_apu_mt589x_mtcmos_off(struct apu_dev *ad, int force)
{
	if (!mtcmos_init_done) {
		arpc_err(ad->dev, "%s(): not init\n", __func__);
		return;
	}

	mutex_lock(&hw_lock);
	if (!power_on_cnt) {
		arpc_info(ad->dev, "%s(): is disabled\n", __func__);
		goto out;
	}

	if (!force && power_on_cnt-- > 1) {
		arpc_info(ad->dev, "%s(): not off, count:%lu\n",
				__func__, power_on_cnt);
		goto out;
	}

	if (plat_teec_send_command(ad, E_MDLA_PWR_CMD_SET_SHUT_DOWN, 0, NULL)) {
		arpc_err(ad->dev, "%s(): off fail\n", __func__);
		power_on_cnt++;
		goto out;
	}

	arpc_info(ad->dev, "%s(): success\n", __func__);
out:
	mutex_unlock(&hw_lock);
}
#endif

void mt589x_mtcmos_suspend(struct apu_dev *ad)
{
	mutex_lock(&hw_lock);
	power_on_cnt = 0;
	plat_teec_send_command(ad, E_MDLA_PWR_CMD_UNINIT_POWER, 0, NULL);
	mutex_unlock(&hw_lock);
}

void mt589x_mtcmos_resume(struct apu_dev *ad)
{
	plat_teec_send_command(ad, E_MDLA_PWR_CMD_INIT_POWER, 0, NULL);
}

unsigned long dtv_apu_mt589x_power_status(void)
{
	unsigned long count;

	mutex_lock(&hw_lock);
	count = power_on_cnt;
	mutex_unlock(&hw_lock);

	return count;
}
