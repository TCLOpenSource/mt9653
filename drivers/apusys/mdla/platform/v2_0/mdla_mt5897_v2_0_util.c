// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/dma-direct.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <utilities/mdla_util.h>
#include <utilities/mdla_debug.h>
#include <platform/v2_0/mdla_hw_reg_v2_0.h>
#include "mdla_mt589x_reg_v2_0.h"
#include "mdla_mt5897_v2_0_util.h"

//#define TRACE_POWER_FLOW_REG 1

#ifdef RIUREAD16
static u32 riu_read32(u32 bank, u32 offset)
{
	void *addr;
	u32 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread32(addr);
	iounmap(addr);

	return regv;
}

static void riu_write32(u32 bank, u32 offset, u32 value)
{
	void *addr;

#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: bank:0x%04x offset:0x%04x value:0x%04x",
			__func__, bank, offset, value);
#endif
	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	iowrite32(value, addr);
	iounmap(addr);
}

static u16 riu_read16(u32 bank, u32 offset)
{
	void *addr;
	u16 regv;

	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	regv = ioread16(addr);
	iounmap(addr);

	return regv;
}

static void riu_write16(u32 bank, u32 offset, u16 value)
{
	void *addr;

#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: bank:0x%04x offset:0x%04x value:0x%04x",
			__func__, bank, offset, value);
#endif
	addr = ioremap_nocache(RIU_ADDR(bank, offset), PAGE_SIZE);
	iowrite16(value, addr);
	iounmap(addr);
}
#endif


#define riu_read16(name, offset)	\
		ioread16(reg_##name + (offset << 2))

#define riu_write16(name, offset, val)	\
		iowrite16(val, reg_##name + (offset << 2))

#define riu_read32(name, offset)	\
		ioread32(reg_##name + (offset << 2))

#define riu_write32(name, offset, val)	\
		iowrite32(val, reg_##name + (offset << 2))

#define RIU2IOMAP(bank, offset)	((phys_addr_t)RIU_ADDR(bank, offset))

static int plat_ioremap_init_done;

void __iomem *reg_ckgen00;
void __iomem *reg_x32_aia_top;
void __iomem *reg_tzpc_aid_0;
void __iomem *reg_tzpc_aid_1;
void __iomem *reg_iommu_local_ssc3;
void __iomem *reg_iommu_local_ssc4;
void __iomem *reg_iommu_global;
void __iomem *reg_mcusys_xiu_bridge;
void __iomem *reg_apusys_vcore_cg_clr;
void __iomem *reg_apu_conn_cg_clr;
void __iomem *reg_apu_mdla0_apu_mdla_cg_clr;
void __iomem *reg_apu_sctrl_pgrmp_sctrl_mdla_core0_ctxt;
void __iomem *reg_0x100c;
void __iomem *reg_0x1007;
void __iomem *reg_0x2809;
void __iomem *reg_0x2828;
void __iomem *reg_0x102a;

static void plat_ioremap_uninit(void)
{
	if (reg_ckgen00)
		iounmap(reg_ckgen00);
	if (reg_x32_aia_top)
		iounmap(reg_x32_aia_top);
	if (reg_tzpc_aid_0)
		iounmap(reg_tzpc_aid_0);
	if (reg_tzpc_aid_1)
		iounmap(reg_tzpc_aid_1);
	if (reg_iommu_local_ssc3)
		iounmap(reg_iommu_local_ssc3);
	if (reg_iommu_local_ssc4)
		iounmap(reg_iommu_local_ssc4);
	if (reg_iommu_global)
		iounmap(reg_iommu_global);
	if (reg_mcusys_xiu_bridge)
		iounmap(reg_mcusys_xiu_bridge);
	if (reg_apusys_vcore_cg_clr)
		iounmap(reg_apusys_vcore_cg_clr);
	if (reg_apu_conn_cg_clr)
		iounmap(reg_apu_conn_cg_clr);
	if (reg_apu_mdla0_apu_mdla_cg_clr)
		iounmap(reg_apu_mdla0_apu_mdla_cg_clr);
	if (reg_apu_sctrl_pgrmp_sctrl_mdla_core0_ctxt)
		iounmap(reg_apu_sctrl_pgrmp_sctrl_mdla_core0_ctxt);
	if (reg_0x100c)
		iounmap(reg_0x100c);
	if (reg_0x1007)
		iounmap(reg_0x1007);
	if (reg_0x2809)
		iounmap(reg_0x2809);
	if (reg_0x2828)
		iounmap(reg_0x2828);
	if (reg_0x102a)
		iounmap(reg_0x102a);
}

static int plat_ioremap_init(void)
{
	reg_ckgen00 = ioremap_nocache(RIU2IOMAP(CKGEN00, 0), PAGE_SIZE);
	reg_x32_aia_top = ioremap_nocache(RIU2IOMAP(X32_AIA_TOP, 0), PAGE_SIZE);
	reg_tzpc_aid_0 = ioremap_nocache(RIU2IOMAP(TZPC_AID_0, 0), PAGE_SIZE);
	reg_tzpc_aid_1 = ioremap_nocache(RIU2IOMAP(TZPC_AID_1, 0), PAGE_SIZE);
	reg_iommu_local_ssc3 = ioremap_nocache(RIU2IOMAP(IOMMU_LOCAL_SSC3, 0), PAGE_SIZE);
	reg_iommu_local_ssc4 = ioremap_nocache(RIU2IOMAP(IOMMU_LOCAL_SSC4, 0), PAGE_SIZE);
	reg_iommu_global = ioremap_nocache(RIU2IOMAP(IOMMU_GLOBAL, 0), PAGE_SIZE);
	reg_mcusys_xiu_bridge = ioremap_nocache(RIU2IOMAP(MCUSYS_XIU_BRIDGE, 0), PAGE_SIZE);
	reg_apusys_vcore_cg_clr = ioremap_nocache(APUSYS_VCORE_CG_CLR, PAGE_SIZE);
	reg_apu_conn_cg_clr = ioremap_nocache(APU_CONN_CG_CLR, PAGE_SIZE);
	reg_apu_mdla0_apu_mdla_cg_clr = ioremap_nocache(APU_MDLA0_APU_MDLA_CG_CLR, PAGE_SIZE);
	reg_apu_sctrl_pgrmp_sctrl_mdla_core0_ctxt =
		ioremap_nocache(APU_SCTRL_PGRMP_SCTRL_MDLA_CORE0_CTXT, PAGE_SIZE);

	reg_0x100c = ioremap_nocache(RIU2IOMAP(0x100C, 0), PAGE_SIZE);
	reg_0x1007 = ioremap_nocache(RIU2IOMAP(0x1007, 0), PAGE_SIZE);
	reg_0x2809 = ioremap_nocache(RIU2IOMAP(0x2809, 0), PAGE_SIZE);
	reg_0x2828 = ioremap_nocache(RIU2IOMAP(0x2828, 0), PAGE_SIZE);
	reg_0x102a = ioremap_nocache(RIU2IOMAP(0x102a, 0), PAGE_SIZE);
	if (!reg_ckgen00
		|| !reg_x32_aia_top
		|| !reg_tzpc_aid_0
		|| !reg_tzpc_aid_1
		|| !reg_iommu_local_ssc3
		|| !reg_iommu_local_ssc4
		|| !reg_iommu_global
		|| !reg_mcusys_xiu_bridge
		|| !reg_apusys_vcore_cg_clr
		|| !reg_apu_conn_cg_clr
		|| !reg_apu_mdla0_apu_mdla_cg_clr
		|| !reg_apu_sctrl_pgrmp_sctrl_mdla_core0_ctxt
		|| !reg_0x100c
		|| !reg_0x1007
		|| !reg_0x2809
		|| !reg_0x2828
		|| !reg_0x102a)
		return -ENOMEM;

	return 0;
}

static int aia_top_mcusys_xiu_bridge_init(void)
{
#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: entered", __func__);
#endif
	riu_write16(mcusys_xiu_bridge, 0x08, 0x0000);

	return 0;
}

static int aia_top_apusys_init(void)
{
#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: entered", __func__);
	mdla_err("[TRACE] riu_write32: bank:0x2828 offset:0x0002 value:0xffffffff\n");
	mdla_err("[TRACE] riu_write32: bank:0x2808 offset:0x0002 value:0xffffffff\n");
	mdla_err("[TRACE] riu_write32: bank:0x2830 offset:0x0002 value:0xffffffff\n");
#endif
	iowrite32(0xffffffff, reg_apusys_vcore_cg_clr);
	iowrite32(0xffffffff, reg_apu_conn_cg_clr);
	iowrite32(0xffffffff, reg_apu_mdla0_apu_mdla_cg_clr);

	return 0;
}

static int aia_top_iommu_init(void)
{
#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: entered", __func__);
#endif
	riu_write16(iommu_local_ssc3, 0x7b, 0x0000);
	riu_write16(iommu_local_ssc4, 0x7b, 0x0000);

	return 0;
}

static void aia_apu_pll_init(void)
{
	riu_write16(ckgen00, 0x3d, 0x0003);
	riu_write16(0x100c, 0x07, 0x0404);
#ifdef CLK_788
	riu_write16(0x100c, 0x30, 0xA122);
	riu_write16(0x100c, 0x31, 0x0034);
#else	//CLK 750
	riu_write16(0x100c, 0x30, 0x4bc6);
	riu_write16(0x100c, 0x31, 0x0037);
#endif

	riu_write16(0x100c, 0x32, 0x0001);
	riu_write16(0x100c, 0x01, 0x6280);
	riu_write16(0x100c, 0x02, 0x0100);
	riu_write16(0x100c, 0x03, 0x0118);
	riu_write16(0x100c, 0x04, 0x0030);
}

static void aia_bus_pll_init(void)
{
	riu_write16(0x1007, 0x44, 0x0000);
	riu_write16(0x1007, 0x46, 0x0400);
	riu_write16(0x1007, 0x47, 0x0000);
	riu_write16(0x1007, 0x48, 0x0034);
	riu_write16(0x1007, 0x42, 0x0000);
	riu_write16(0x1007, 0x41, 0x0001);
	riu_write16(0x1007, 0x40, 0x1110);
	udelay(500); //us
	riu_write16(0x1007, 0x40, 0x1010);
}

static int aia_top_ckgen_init(void)
{
	u16 data;

#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: entered", __func__);
#endif
	aia_apu_pll_init();
	aia_bus_pll_init();

	//init clk_mdla
	data = riu_read16(ckgen00, 0x40);
	data = (data & ~0x0700) | 0x0400;
	riu_write16(ckgen00, 0x40, data);

	//init clk_bus_die
	data = riu_read16(ckgen00, 0x3c);
	data = (data & ~0x0700) | 0x0400;
	riu_write16(ckgen00, 0x3c, data);

	// init clk_imi
	data = riu_read16(ckgen00, 0x44);
	data = (data & ~0x0003) | 0x0000;
	riu_write16(ckgen00, 0x44, data);

	//data = riu_read16(ckgen00, 0x52b);
	data = riu_read16(0x102a, 0x2b);
	data = (data & ~0x0002) | 0x0002;
	riu_write16(0x102a, 0x2b, data);

	return 0;
}

static int sram_csr_use_default_delsel(void)
{
	u32 regv;

#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: entered", __func__);
#endif
	//asr_use_default_delsel setting
	//2809, 34
	regv = riu_read32(0x2809, 0x34);
	regv |= (0x1 << 0);
	riu_write32(0x2809, 0x34, regv);

	return 0;
}

static int aia_top_init(int slpprot_rdy)
{
	u16 regv;
	u16 mask;
	u32 bit_ofs, reg_idx;

#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: entered", __func__);
#endif
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

	return 0;
}

static int aia_power_off(int vcore_sw_rst)
{
	u16 regv, mask;
	u32 bit_ofs, reg_idx;

#ifdef TRACE_POWER_FLOW_REG
	mdla_err("[TRACE] %s: entered", __func__);
#endif
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
	// [TODO] JIMMY:need check slpprot rdy offset is 0x5 or 0x11
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
		;

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

	return 0;
}

void mdla_mt5897_sw_reset(void)
{
	riu_write16(x32_aia_top, 0x10, 0x69);
	while ((riu_read16(x32_aia_top, 0x10) & 0xff) != 0xdb)
		;
	riu_write16(x32_aia_top, 0x03, 0xf3);
	riu_write16(x32_aia_top, 0x03, 0xff);
	riu_write16(x32_aia_top, 0x10, 0x00);
}

#ifdef EFUSE_CHECK
void mdla_efuse_overwrite(void)
{
	void *addr;
	u16 regv, efuse;

	mdla_drv_debug("%s: overwrite and test\n", __func__);
	//bank2e[22]:MDLA

	//overwrite efuse
	//0x58:outb[15:0], 0x59:outb[31:16]
	regv = riu_read16(0x129, 0x59);
	regv |= (0x1 << 6);
	riu_write16(0x129, 0x59, regv);

	//enable overwrite
	regv = riu_read16(0x129, 0x5c);
	regv |= 0x1;
	riu_write16(0x129, 0x5c, regv);

	//read efuse
	//0x50:outb[15:0], 0x51:outb[31:16]
	regv = riu_read16(0x129, 0x51);
	efuse = (regv >> 6) & 0x1;
}

unsigned int mdla_efuse_read(void)
{
	void *addr;
	u16 regv;
	u32 efuse;

	//read efuse
	//bank2e[22]:MDLA
	//0x50:outb[15:0], 0x51:outb[31:16]
	regv = riu_read16(0x129, 0x51);
	efuse = (regv >> 6) & 0x1;

	return efuse;
}
#endif
//---------------------------------------------------------------------------------------
//------------------------------------  COMMON SOURCE CODE HERE  ------------------------
//---------------------------------------------------------------------------------------

int mdla_mt5897_v2_0_hw_reset(u32 reset)
{
	static int init_once;
	u32 val;

	mdla_drv_debug("%s: hw_reset start(%x)\n", __func__, reset);
	if (!plat_ioremap_init_done) {
		mdla_err("%s: should init first", __func__);
		return -EFAULT;
	}

	if (reset == NORMAL_RESET) {
		if (!init_once) {
			aia_top_ckgen_init();
			aia_top_init(0);
			aia_top_apusys_init();
			aia_top_mcusys_xiu_bridge_init();
			sram_csr_use_default_delsel();
			init_once = 1;
		}
	} else if (reset == COLD_RESET) {
		aia_top_ckgen_init();
		aia_top_init(0);
		aia_top_apusys_init();
		aia_top_mcusys_xiu_bridge_init();
		sram_csr_use_default_delsel();
		init_once = 1;
	} else if (reset == FORCE_RESET) {
		aia_top_ckgen_init();
		aia_power_off(1);
		aia_top_init(0);
		aia_top_apusys_init();
		aia_top_mcusys_xiu_bridge_init();
		sram_csr_use_default_delsel();
		init_once = 1;
	}

	mdla_drv_debug("%s: hw_reset done.\n", __func__);

	val = ioread32(reg_apu_sctrl_pgrmp_sctrl_mdla_core0_ctxt);
	val = (val & ~0x3) | 0x2;
	iowrite32(val, reg_apu_sctrl_pgrmp_sctrl_mdla_core0_ctxt);

	return 0;
}

int mdla_util_mt5897_init(void)
{
	if (!plat_ioremap_init_done) {
		if (plat_ioremap_init()) {
			mdla_err("%s: ioremap_init fail\n", __func__);
			return -ENOMEM;
		}
		plat_ioremap_init_done = 1;
	}

	return 0;
}

void mdla_util_mt5897_uninit(void)
{
	if (plat_ioremap_init_done) {
		plat_ioremap_uninit();
		plat_ioremap_init_done = 0;
	}
}
