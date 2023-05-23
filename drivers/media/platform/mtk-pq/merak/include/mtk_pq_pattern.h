/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _MTK_PQ_PATTERN
#define _MTK_PQ_PATTERN

#include <linux/device.h>
#include "m_pqu_pattern.h"
//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  enums
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  structs
//-------------------------------------------------------------------------------------------------
struct pq_pattern_size {
	unsigned short h_size;
	unsigned short v_size;
};

struct pq_pattern_status {
	unsigned char pattern_enable_position;
	unsigned char pattern_type;
};


//-------------------------------------------------------------------------------------------------
//  function
//-------------------------------------------------------------------------------------------------
int mtk_pq_pattern_set_size_info(struct pq_pattern_size_info *size_info);

int mtk_pq_pattern_fill_matrix(const char *string, struct pq_pattern_dot_matrix *dot_matrix);

ssize_t mtk_pq_pattern_show(char *buf,
	struct device *dev, enum pq_pattern_position position);

int mtk_pq_pattern_store(const char *buf,
	struct device *dev, enum pq_pattern_position position);

#endif
