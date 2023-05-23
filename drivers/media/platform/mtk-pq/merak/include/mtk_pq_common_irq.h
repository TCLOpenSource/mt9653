/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_PQ_COMMON_IRQ_H_
#define _MTK_PQ_COMMON_IRQ_H_

//------------------------------------------------------------------------------
//  Define
//------------------------------------------------------------------------------
#define PQ_IRQ_RES_MAX (1)

//------------------------------------------------------------------------------
//  Enum
//------------------------------------------------------------------------------
enum PQIRQTYPE {
	PQ_IRQTYPE_HK = 0,
	PQ_IRQTYPE_FRC,
	PQ_IRQTYPE_PQ,
	PQ_IRQTYPE_MAX,
};

enum PQIRQEVENT {
	PQ_IRQEVENT_SW_IRQ_TRIG_IP = 0,
	PQ_IRQEVENT_SW_IRQ_TRIG_OP2,
	PQ_IRQEVENT_MAX,
};
//------------------------------------------------------------------------------
//  Structures
//------------------------------------------------------------------------------
struct mtk_pq_irq_res {
	char *interrupt[PQ_IRQ_RES_MAX];
};
//------------------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------------------
int _mtk_pq_common_irq_set_mask(
	enum PQIRQTYPE irqType,
	enum PQIRQEVENT irqEvent,
	bool mask,
	uint32_t u32IRQ_Version);

int _mtk_pq_common_irq_set_clr(
	enum PQIRQTYPE irqType,
	enum PQIRQEVENT irqEvent,
	uint32_t u32IRQ_Version);

int _mtk_pq_common_irq_get_status(
	enum PQIRQTYPE irqType,
	enum PQIRQEVENT irqEvent,
	bool *IRQstatus,
	uint32_t u32IRQ_Version);

int mtk_pq_common_irq_init(
	struct mtk_pq_platform_device *pqdev);

int mtk_pq_common_irq_suspend(bool stub);

int mtk_pq_common_irq_resume(bool stub, uint32_t u32IRQ_Version);

#endif/*_MTK_PQ_COMMON_IRQ_H_*/
