/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_CEC_H_
#define _MTK_CEC_H_

#include <linux/io.h>
#include <linux/of.h>
#include <media/cec.h>
#include <linux/platform_device.h>


#define MTK_GenSys_CEC_DEFAULT_XTAL 12000000

#define CEC_RECEIVE_SUCCESS BIT(0)
#define CEC_SNED_SUCCESS BIT(1)
#define CEC_RETRY_FAIL BIT(2)
#define CEC_LOST_ARBITRRATION BIT(3)
#define CEC_TRANSMIT_NACK BIT(4)
#define CEC_SECOND_RECEIVE_SUCCESS BIT(5)
#define CEC_SECOND_SEND_NACK BIT(6)


enum en_cec_state {
	STATE_IDLE,
	STATE_BUSY,
	STATE_DONE,
	STATE_NACK,
	STATE_ERROR
};

struct mtk_cec_dev {
	struct cec_adapter    *adap;
	struct device    *dev;
	struct mutex    lock;
	struct cec_msg    msg;
	enum en_cec_state    rx;
	enum en_cec_state    tx;
	void __iomem *reg_base;
	u8  retrycnt;
	unsigned int xtal;
	int    irq;
	unsigned long log_level;
};

#endif /* _MTK_CEC_H_ */
