/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _HAL_PCMCIA_H_
#define _HAL_PCMCIA_H_


#include "mtk_pcmcia_moleskine.h"
#include "mtk_pcmcia_core.h"// for SPI


#ifdef __cplusplus
extern "C"
{
#endif
//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
//extern unsigned long u32PCMCIA_RIU_BaseAdd;

//#define RIU_READ_BYTE(Addr)         ( READ_BYTE( u32PCMCIA_RIU_BaseAdd + (Addr) ) )
//#define RIU_READ_2BYTE(Addr)        ( READ_WORD( u32PCMCIA_RIU_BaseAdd + (Addr) ) )
//#define RIU_WRITE_BYTE(Addr, Val)   { WRITE_BYTE( u32PCMCIA_RIU_BaseAdd + (Addr), Val) }
//#define RIU_WRITE_2BYTE(Addr, Val)  { WRITE_WORD( u32PCMCIA_RIU_BaseAdd + (Addr), Val) }

#define MOLESKINE_MSPI_CHANNEL 0x2UL
//extern unsigned long u32EXTRIU_BaseAdd;
//#define EXTRIU_READ_BYTE(Addr)         ( READ_BYTE( u32EXTRIU_BaseAdd + (Addr) ) )
//#define EXTRIU_READ_2BYTE(Addr)        ( READ_WORD( u32EXTRIU_BaseAdd + (Addr) ) )
//#define EXTRIU_WRITE_BYTE(Addr, Val)   { WRITE_BYTE( u32EXTRIU_BaseAdd + (Addr), Val) }
//#define EXTRIU_WRITE_2BYTE(Addr, Val)  { WRITE_WORD( u32EXTRIU_BaseAdd + (Addr), Val) }
//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
typedef struct {
unsigned char bCardAInsert;
unsigned char bCardARemove;
unsigned char bCardAData;
unsigned char bCardBInsert;
unsigned char bCardBRemove;
unsigned char bCardBData;
} ISR_STS;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
void HAL_PCMCIA_Set_RIU_base(void __iomem *RIU_vaddr);
void HAL_Set_EXTRIU_base(void __iomem *extRIU_vaddr);
unsigned char HAL_PCMCIA_Read_Byte(unsigned int ptrAddr);
void HAL_PCMCIA_Write_Byte(size_t ptrAddr, unsigned char u8Val);
void HAL_PCMCIA_Write_ByteMask(size_t u32Addr, unsigned char u8Val, unsigned char u8Mask);

unsigned char HAL_PCMCIA_GetIntStatus(ISR_STS *isr_status);
void HAL_PCMCIA_ClrInt(unsigned int bits);
void HAL_PCMCIA_MaskInt(unsigned int bits, unsigned char bMask);
void HAL_PCMCIA_ClkCtrl(unsigned char bEnable);

#ifdef __cplusplus
extern "C"
}
#endif

#endif // _HAL_PCMCIA_H_
