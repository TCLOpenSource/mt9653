/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include "mtk_pcmcia_data_type.h"


typedef enum {
	STUB_STATE_CARD_DETECT,
	STUB_STATE_CARD_RESET,
	STUB_STATE_READ_CIS,
	STUB_STATE_WRITE_IO_MEM,
	STUB_STATE_READ_IO_MEM
} STUB_STATE;


typedef enum {
	STUB_READ,
	STUB_WRITE
} STUB_ACCESS;
void set_stub_statue(STUB_STATE stub_state, STUB_ACCESS stub_access, unsigned int u32Addr,
				   unsigned char *value, unsigned char expert_value);

