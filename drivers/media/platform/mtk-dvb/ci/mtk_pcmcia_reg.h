/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _REG_PCMCIA_H_
#define _REG_PCMCIA_H_

//-----------------------------------------------------------------------------
//	Hardware Capability
//-----------------------------------------------------------------------------
// Base address should be initial.
#define PCMCIA_RIU_MAP //u32PCMCIA_RIU_BaseAdd	//obtain in init
#define EXTRIU_MAP //u32EXTRIU_BaseAdd	//obtain in init
//-----------------------------------------------------------------------------
//	Macro and Define
//-----------------------------------------------------------------------------
//hardware spec
#define REG_PCMCIA_PCM_MEM_IO_CMD			0x00UL
#define REG_PCMCIA_PCM_PIN					0x01UL
	#define PCM_PCM1_CE							0x10UL
	#define PCM_PCM2_CE							0x20UL
	#define PCM_PCM1_RESET						0x40UL
	#define PCM_PCM2_RESET						0x80UL

#define REG_PCMCIA_ADDR0					0x02UL
#define REG_PCMCIA_ADDR1					0x03UL
#define REG_PCMCIA_WRITE_DATA				0x04UL
#define REG_PCMCIA_FIRE_READ_DATA_CLEAR		0x06UL
#define REG_PCMCIA_READ_DATA				0x08UL
#define REG_PCMCIA_READ_DATA_DONE_BUS_IDLE	0x09UL
#define REG_PCMCIA_INT_MASK_CLEAR			0x0AUL
#define REG_PCMCIA_INT_MASK_CLEAR1			0x0BUL
#define REG_PCMCIA_STAT_INT_RAW_INT			0x0EUL
#define REG_PCMCIA_STAT_INT_RAW_INT1		0x0FUL
#define REG_PCMCIA_MODULE_VCC_OOB			0x10UL
#define REG_PCMCIA_CONFIG					0x11UL
	#define PCM_CD1_OR_CD2						0x01UL

#define PCMCIA_ATTRIBMEMORY_READ			0x03UL
#define PCMCIA_ATTRIBMEMORY_WRITE			0x04UL
#define PCMCIA_IO_READ						0x05UL
#define PCMCIA_IO_WRITE						0x06UL

#define PCMCIA_BASE_ADDRESS	(PCMCIA_RIU_MAP + (REG_PCMCIA_BASE * 2))

#define PCM_OOB_BIT_MASK					0x03UL
#define PCM_OOB_BIT_SHFT					6UL

#define PCM_OOB_CYCLE_EXTEND				0x3UL
// 00:th(CE)=4T (extend 3 active cycle)
// 01:th(CE)=3T (extend 2 active cycle)
// 10:th(CE)=2T (extend 1 active cycle)
// 11:th(CE)=1T


	#define REG_CLKGEN0_PCM_CLK				((0x00B00 >> 1) + 0x1A)
		#define REG_CLKGEN0_PCM_CLK_MASK	0x000FUL
		#define REG_CLKGEN0_PCM_CLK_DIS		0x0001UL

#define PCM_TOP_REG(addr) (*((volatile unsigned short*)(PCMCIA_RIU_MAP + 0x3C00UL + ((addr)<<2UL))))

//-----------------------------------------------------------------------------
//	Type and Structure
//-----------------------------------------------------------------------------

#endif // _REG_PCMCIA_H_
