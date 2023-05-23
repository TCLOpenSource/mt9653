/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef _DRV_PCMCIA_H_
#define _DRV_PCMCIA_H_

#include <linux/types.h>
#include <linux/dvb/ca.h>
#include "mtk_pcmcia_data_type.h"




#define MAX_CIS_SIZE                    0x100

#define PCMCIA_IRQ_ENABLE               0

/* Command Interface Hardware Registers */
#define PCMCIA_PHYS_REG_DATA            (0)
#define PCMCIA_PHYS_REG_COMMANDSTATUS   (1)
#define PCMCIA_PHYS_REG_SIZELOW         (2)
#define PCMCIA_PHYS_REG_SIZEHIGH        (3)

/* Status Register Bits */
#define PCMCIA_STATUS_DATAAVAILABLE     (0x80)
#define PCMCIA_STATUS_FREE              (0x40)
#define PCMCIA_STATUS_IIR               (0x10)
#define PCMCIA_STATUS_RESERVEDBITS      (0x2C)
#define PCMCIA_STATUS_WRITEERROR        (0x02)
#define PCMCIA_STATUS_READERROR         (0x01)
#define PCMCIA_STATUS_BUSY              (0x00)

/* Command Register Bits */
#define PCMCIA_COMMAND_DAIE             (0x80)
#define PCMCIA_COMMAND_FRIE             (0x40)
#define PCMCIA_COMMAND_RESERVEDBITS     (0x30)
#define PCMCIA_COMMAND_RESET            (0x08)
#define PCMCIA_COMMAND_SIZEREAD         (0x04)
#define PCMCIA_COMMAND_SIZEWRITE        (0x02)
#define PCMCIA_COMMAND_HOSTCONTROL      (0x01)

#define PCMCIA_DEFAULT_MODULE                   E_PCMCIA_MODULE_A

typedef void (*IsrCallback) (void *wparam, void *lparam);


typedef enum {
	E_POWER_SUSPEND = 1,
	E_POWER_RESUME = 2,
	E_POWER_MECHANICAL = 3,
	E_POWER_SOFT_OFF = 4,
} EN_POWER_MODE;

typedef enum {
	E_PCMCIA_MODULE_A = 0,
	E_PCMCIA_MODULE_B = 1,
	E_PCMCIA_MODULE_MAX = 2
} PCMCIA_MODULE;


typedef struct {
	unsigned char bISRCardInsert;
	unsigned char bISRCardRemove;
	unsigned char bISRCardData;
} PCMCIA_ISR;

void mdrv_pcmcia_init_sw(void __iomem *riu_vaddr,
			void __iomem *ext_riu_vaddr,
			unsigned char reverse);

void mdrv_pcmcia_init_hw(void);

void mdrv_pcmcia_isr(void);

unsigned int mdrv_pcmcia_set_power_state(EN_POWER_MODE power_state);

unsigned char mdrv_pcmcia_polling(PCMCIA_MODULE module);

void mdrv_pcmcia_reset_hw(PCMCIA_MODULE module);

void mdrv_pcmcia_read_attrib_mem(PCMCIA_MODULE module, unsigned short addr,
				 unsigned char *dest);

void mdrv_pcmcia_write_attrib_mem(PCMCIA_MODULE module, unsigned short addr,
				  unsigned char data);

void mdrv_pcmcia_write_io_mem(PCMCIA_MODULE module, unsigned short addr,
			      unsigned char data);

void mdrv_pcmcia_write_io_mem_long(PCMCIA_MODULE module, unsigned short addr,
				  unsigned short data_len, unsigned char *write_buffer);

unsigned char mdrv_pcmcia_read_io_mem(PCMCIA_MODULE module, unsigned short addr);

unsigned char mdrv_pcmcia_is_module_still_plugged(PCMCIA_MODULE module);

unsigned short mdrv_pcmcia_read_data(PCMCIA_MODULE module,
			      unsigned char *read_buffer,
			      unsigned short read_buffer_size);

unsigned char mdrv_pcmcia_ready_status(void);

void mdrv_pcmcia_exit(void);

#if PCMCIA_IRQ_ENABLE

void mdrv_pcmcia_Enable_InterruptV2(PCMCIA_MODULE module,
				    unsigned char bEnable);

void mdrv_pcmcia_Set_InterruptStatusV2(PCMCIA_MODULE module,
				       unsigned char Status);

unsigned char mdrv_pcmcia_Get_InterruptStatusV2(PCMCIA_MODULE module);

void mdrv_pcmcia_InstarllIsrCallbackV2(PCMCIA_MODULE module,
				       IsrCallback fnIsrCallback);

void mdrv_pcmcia_Enable_Interrupt(unsigned char bEnable);

void mdrv_pcmcia_Set_InterruptStatus(unsigned char Status);

unsigned char mdrv_pcmcia_Get_InterruptStatus(void);

void mdrv_pcmcia_InstarllIsrCallback(IsrCallback fnIsrCallback);
#endif

#endif
