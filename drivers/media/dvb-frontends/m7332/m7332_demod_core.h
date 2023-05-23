/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _M7332_DEMOD_CORE_H_
#define _M7332_DEMOD_CORE_H_



#define DRV_NAME    "mediatek,demod"

#define TUNER_OK //ckang tuner
#define TUNER_OK_2 //ckang adapter

#ifdef TUNER_OK_2 //ckang adapter
extern struct dvb_adapter *mdrv_get_dvb_adapter(void);
#endif

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
#endif

