/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _MTK_SRCCAP_COMMON_IRQ_H_
#define _MTK_SRCCAP_COMMON_IRQ_H_

/* ============================================================================================== */
/* ------------------------------------------ Defines ------------------------------------------- */
/* ============================================================================================== */
#define SRCCAP_COMMON_IRQ_RES_MAX (1)
#define SRCCAP_COMMON_IRQ_MAX_NUM (16)
#define MASKIRQ		(true)
#define UNMASKIRQ	(false)

/* ============================================================================================== */
/* ------------------------------------------- Macros ------------------------------------------- */
/* ============================================================================================== */

/* ============================================================================================== */
/* ------------------------------------------- Enums -------------------------------------------- */
/* ============================================================================================== */
enum srccap_common_irq_type {
	SRCCAP_COMMON_IRQTYPE_HK = 0,
	SRCCAP_COMMON_IRQTYPE_FRC,
	SRCCAP_COMMON_IRQTYPE_PQ,
	SRCCAP_COMMON_IRQTYPE_MAX,
};

enum srccap_common_irq_event {
	SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC0 = 0,
	SRCCAP_COMMON_IRQEVENT_SW_IRQ_TRIG_SRC1,
	SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT0,
	SRCCAP_COMMON_IRQEVENT_DV_HW5_IP_INT1,
	SRCCAP_COMMON_IRQEVENT_MAX,
};

///< Interrupt callback function
typedef void (*SRCCAP_COMMON_INTCB) (void *param);
/* ============================================================================================== */
/* ------------------------------------------ Structs ------------------------------------------- */
/* ============================================================================================== */
struct mtk_srccap_dev;

struct srccap_common_irq_type_event {
	enum srccap_common_irq_type type;
	enum srccap_common_irq_event event;
	bool set_force;
};

struct srccap_common_irq_res {
	char *interrupt[SRCCAP_COMMON_IRQ_RES_MAX];
};

struct srccap_common_isr_info {
	void *param;			// parameter will be passed to ISR when calling it.
	SRCCAP_COMMON_INTCB isr;	// attach ISR
};

struct srccap_common_irq_info {
	struct srccap_common_isr_info isr_info
	[SRCCAP_COMMON_IRQ_MAX_NUM][SRCCAP_COMMON_IRQTYPE_MAX][SRCCAP_COMMON_IRQEVENT_MAX];
};

/* ============================================================================================== */
/* ----------------------------------------- Functions ------------------------------------------ */
/* ============================================================================================== */
int mtk_common_attach_irq(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	SRCCAP_COMMON_INTCB intcb,
	void *param);

int mtk_common_detach_irq(
	struct mtk_srccap_dev *srccap_dev,
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	SRCCAP_COMMON_INTCB intcb,
	void *param);

int mtk_common_set_irq_mask(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	bool mask,
	uint32_t u32IRQ_Version);

int mtk_common_force_irq(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	bool force,
	uint32_t u32IRQ_Version);

int mtk_common_clear_irq(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	uint32_t u32IRQ_Version);

int mtk_common_get_irq_status(
	enum srccap_common_irq_type type,
	enum srccap_common_irq_event event,
	bool *status,
	uint32_t u32IRQ_Version);

int mtk_common_init_irq(
	struct mtk_srccap_dev *srccap_dev);

int mtk_srccap_common_irq_suspend(void);

int mtk_srccap_common_irq_resume(void);
#endif/*_MTK_SRCCAP_COMMON_IRQ_H_*/
