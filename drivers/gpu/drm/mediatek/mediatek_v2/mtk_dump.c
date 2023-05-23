// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "mtk_drm_ddp_comp.h"
#include "mtk_dump.h"

static const char * const ddp_comp_str[] = {DECLARE_DDP_COMP(DECLARE_STR)};

const char *mtk_dump_comp_str(struct mtk_ddp_comp *comp)
{
	return ddp_comp_str[comp->id];
}

const char *mtk_dump_comp_str_id(unsigned int id)
{
	if (likely(id < DDP_COMPONENT_ID_MAX))
		return ddp_comp_str[id];

	return "invalid";
}

int mtk_dump_reg(struct mtk_ddp_comp *comp)
{
	switch (comp->id) {
	case DDP_COMPONENT_OVL0:
	case DDP_COMPONENT_OVL0_2L:
	case DDP_COMPONENT_OVL1_2L:
	case DDP_COMPONENT_OVL2_2L:
	case DDP_COMPONENT_OVL3_2L:
		mtk_ovl_dump(comp);
		break;
	case DDP_COMPONENT_RDMA0:
	case DDP_COMPONENT_RDMA1:
		mtk_rdma_dump(comp);
		break;
	case DDP_COMPONENT_WDMA0:
	case DDP_COMPONENT_WDMA1:
		mtk_wdma_dump(comp);
		break;
	case DDP_COMPONENT_RSZ0:
	case DDP_COMPONENT_RSZ1:
		mtk_rsz_dump(comp);
		break;
	case DDP_COMPONENT_DSI0:
	case DDP_COMPONENT_DSI1:
		mtk_dsi_dump(comp);
		break;
	case DDP_COMPONENT_COLOR0:
	case DDP_COMPONENT_COLOR1:
	case DDP_COMPONENT_COLOR2:
		mtk_color_dump(comp);
		break;
	case DDP_COMPONENT_CCORR0:
	case DDP_COMPONENT_CCORR1:
		mtk_ccorr_dump(comp);
		break;
	case DDP_COMPONENT_AAL0:
	case DDP_COMPONENT_AAL1:
		mtk_aal_dump(comp);
		break;
	case DDP_COMPONENT_DMDP_AAL0:
		mtk_dmdp_aal_dump(comp);
		break;
	case DDP_COMPONENT_DITHER0:
	case DDP_COMPONENT_DITHER1:
		mtk_dither_dump(comp);
		break;
	case DDP_COMPONENT_GAMMA0:
	case DDP_COMPONENT_GAMMA1:
		mtk_gamma_dump(comp);
		break;
	case DDP_COMPONENT_POSTMASK0:
	case DDP_COMPONENT_POSTMASK1:
		mtk_postmask_dump(comp);
		break;
	case DDP_COMPONENT_DSC0:
		mtk_dsc_dump(comp);
		break;
	default:
		return 0;
	}

	return 0;
}

int mtk_dump_analysis(struct mtk_ddp_comp *comp)
{
	switch (comp->id) {
	case DDP_COMPONENT_OVL0:
	case DDP_COMPONENT_OVL1:
	case DDP_COMPONENT_OVL0_2L:
	case DDP_COMPONENT_OVL1_2L:
	case DDP_COMPONENT_OVL2_2L:
	case DDP_COMPONENT_OVL3_2L:
		mtk_ovl_analysis(comp);
		break;
	case DDP_COMPONENT_RDMA0:
	case DDP_COMPONENT_RDMA1:
		mtk_rdma_analysis(comp);
		break;
	case DDP_COMPONENT_WDMA0:
	case DDP_COMPONENT_WDMA1:
		mtk_wdma_analysis(comp);
		break;
	case DDP_COMPONENT_RSZ0:
	case DDP_COMPONENT_RSZ1:
		mtk_rsz_analysis(comp);
		break;
	case DDP_COMPONENT_DSI0:
	case DDP_COMPONENT_DSI1:
		mtk_dsi_analysis(comp);
		break;
	case DDP_COMPONENT_POSTMASK0:
	case DDP_COMPONENT_POSTMASK1:
		mtk_postmask_analysis(comp);
		break;
	case DDP_COMPONENT_DSC0:
		mtk_dsc_analysis(comp);
		break;
	default:
		return 0;
	}

	return 0;
}

#define SERIAL_REG_MAX 54
void mtk_serial_dump_reg(void __iomem *base, unsigned int offset,
			 unsigned int num)
{
	unsigned int i = 0, s = 0, l = 0;
	char buf[SERIAL_REG_MAX];

	if (num > 4)
		num = 4;

	l = snprintf(buf, SERIAL_REG_MAX, "0x%03x:", offset);

	for (i = 0; i < num; i++) {
		s = snprintf(buf + l, SERIAL_REG_MAX, "0x%08x ",
			     readl(base + offset + i * 0x4));
		l += s;
	}

	DDPDUMP("%s\n", buf);
}

#define CUST_REG_MAX 84
void mtk_cust_dump_reg(void __iomem *base, int off1, int off2, int off3,
		       int off4)
{
	unsigned int i = 0, s = 0, l = 0;
	int off[] = {off1, off2, off3, off4};
	char buf[CUST_REG_MAX];

	for (i = 0; i < 4; i++) {
		if (off[i] < 0)
			break;
		s = snprintf(buf + l, CUST_REG_MAX, "0x%03x:0x%08x ", off[i],
			     readl(base + off[i]));
		l += s;
	}

	DDPDUMP("%s\n", buf);
}
