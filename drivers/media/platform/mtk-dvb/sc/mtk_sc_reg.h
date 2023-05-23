/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef _REG_SC_H_
#define _REG_SC_H_
#include "mtk_sc_msriu.h"
#include "mtk_sc_smart.h"

#if defined(MSOS_TYPE_LINUX) || defined(MSOS_TYPE_LINUX_KERNEL)
#define REG_SC_BASE1						(_regSCBase[0])
#define REG_SC_BASE2						(_regSCBase[1])
#define RIU_BUS_BASE						_regSCHWBase
#else
#define REG_SC_BASE1				 0x00102900UL
#define REG_SC_BASE2				 0x00102A00UL	//not support
#define RIU_BUS_BASE				 0xFD000000UL
#endif

#define REG_CHIP_TOP_BASE			 0x00101E00UL
#define REG_CLK_GEN_BASE			 0x00100B00UL
#define REG_CLK_GEN1_BASE			 0x00103300UL
#define REG_CLK_GEN2_BASE			 0x00100A00UL
#define REG_CHIP_GPIO_BASE			 0x00322900UL	//gpio_func_mux bank 3229
// ------------------------------------------------------------------------------------------------
// UART / SC



#define UART1_READ(addr)		u2IO32Read2B((void *)addr)

#define UART2_READ(addr)		u2IO32Read2B((void *)addr)

#define UART1_WRITE(addr, val)	vIO32Write2B((void *)addr, val)

#define UART2_WRITE(addr, val)	vIO32Write2B((void *)addr, val)



#define UART1_OR(addr, val)		UART1_WRITE(addr, UART1_READ(addr) | (val))
#define UART1_AND(addr, val)	UART1_WRITE(addr, UART1_READ(addr) & (val))
#define UART1_XOR(addr, val)	UART1_WRITE(addr, UART1_READ(addr) ^ (val))
#define UART2_OR(addr, val)		UART2_WRITE(addr, UART2_READ(addr) | (val))
#define UART2_AND(addr, val)	UART2_WRITE(addr, UART2_READ(addr) & (val))
#define UART2_XOR(addr, val)	UART2_WRITE(addr, UART2_READ(addr) ^ (val))
//
// UART Register List
//
#define UART_RX		(nonPM_vaddr + REG_0000_SMART)
#define UART_TX		(nonPM_vaddr + REG_0000_SMART)
#define UART_DLL	(nonPM_vaddr + REG_0000_SMART)// Out: Divisor Latch Low (DLAB=1)
#define UART_DLM	(nonPM_vaddr + REG_0004_SMART)// Out: Divisor Latch High (DLAB=1)

//
// UART_IER(1)
// Interrupt Enable Register
//
#define UART_IER	(nonPM_vaddr + REG_0004_SMART)// Out: Interrupt Enable Register
	#define UART_IER_RDI	(0x00000001UL)
	#define UART_IER_THRI	(0x00000002UL)
	#define UART_IER_RLSI	(0x00000004UL)
	#define UART_IER_MSI	(0x00000008UL)	// Enable Modem status interrupt

//
// UART_IIR(2)
// Interrupt Identification Register
//
#define UART_IIR	(nonPM_vaddr + REG_0008_SMART)// In:  Interrupt ID Register
	#define UART_IIR_NO_INT		(0x00000001UL)	// No interrupts pending
	#define UART_IIR_ID			(0x0000000EUL)	// Mask for the interrupt ID
	#define UART_IIR_MSI		(0x00000000UL)	// Modem status interrupt
	#define UART_IIR_THRI		(0x00000002UL)
	#define UART_IIR_TOI		(0x0000000cUL)	// Receive time out interrupt
	#define UART_IIR_RDI		(0x00000004UL)	// Receiver data interrupt
	#define UART_IIR_RLSI		(0x00000006UL)	// Receiver line status interrupt

//
// UART_FCR(2)
// FIFO Control Register (16650 only)
//
#define UART_FCR	(nonPM_vaddr + REG_0008_SMART)// Out: FIFO Control Register
	#define UART_FCR_ENABLE_FIFO	(0x00000001UL)	// Enable the FIFO
	#define UART_FCR_CLEAR_RCVR		(0x00000002UL)	// Clear the RCVR FIFO
	#define UART_FCR_CLEAR_XMIT		(0x00000004UL)	// Clear the XMIT FIFO
	#define UART_FCR_DMA_SELECT		(0x00000008UL)	// For DMA applications
	#define UART_FCR_TRIGGER_MASK	(0x000000C0UL)	// Mask for the FIFO trigger range
	#define UART_FCR_TRIGGER_1		(0x00000000UL)	// Mask for trigger set at 1
	#define UART_FCR_TRIGGER_4		(0x00000040UL)	// Mask for trigger set at 4
	#define UART_FCR_TRIGGER_8		(0x00000080UL)	// Mask for trigger set at 8
	#define UART_FCR_TRIGGER_14		(0x000000C0UL)	// Mask for trigger set at 14



//
// UART_LCR(3)
// Line Control Register
// Note: if the word length is 5 bits (UART_LCR_WLEN5), then setting
// UART_LCR_STOP will select 1.5 stop bits, not 2 stop bits.
//
#define UART_LCR	(nonPM_vaddr + REG_000C_SMART)// Out: Line Control Register
	#define UART_LCR_WLEN5		(0x00000000UL)	// Wordlength: 5 bits
	#define UART_LCR_WLEN6		(0x00000001UL)	// Wordlength: 6 bits
	#define UART_LCR_WLEN7		(0x00000002UL)	// Wordlength: 7 bits
	#define UART_LCR_WLEN8		(0x00000003UL)	// Wordlength: 8 bits
	#define UART_LCR_STOP1		(0x00000000UL)	//
	#define UART_LCR_STOP2		(0x00000004UL)
	#define UART_LCR_PARITY		(0x00000008UL)	// Parity Enable
	#define UART_LCR_EPAR		(0x00000010UL)	// Even parity select
	#define UART_LCR_SPAR		(0x00000020UL)	// Stick parity (?)
	#define UART_LCR_SBC		(0x00000040UL)	// Set break control
	#define UART_LCR_DLAB		(0x00000080UL)	// Divisor latch access bit

//
// UART_MCR(4)
// Modem Control Register
//
#define UART_MCR	(nonPM_vaddr + REG_0010_SMART)// Out: Modem Control Register
	#define UART_MCR_DTR	(0x00000001UL)	// DTR complement
	#define UART_MCR_RTS	(0x00000002UL)	// RTS complement
	#define UART_MCR_OUT1	(0x00000004UL)	// Out1 complement
	#define UART_MCR_OUT2	(0x00000008UL)	// Out2 complement
	#define UART_MCR_LOOP	(0x00000010UL)	// Enable loopback test mode
	#define UART_MCR_FAST	(0x00000020UL)	// Slow / Fast baud rate mode

//
// UART_LSR(5)
// Line Status Register
//
#define UART_LSR	(nonPM_vaddr + REG_0014_SMART)// In:  Line Status Register
	#define UART_LSR_DR		(0x00000001UL)	// Receiver data ready
	#define UART_LSR_OE		(0x00000002UL)	// Overrun error indicator
	#define UART_LSR_PE		(0x00000004UL)	// Parity error indicator
	#define UART_LSR_FE		(0x00000008UL)	// Frame error indicator
	#define UART_LSR_BI		(0x00000010UL)	// Break interrupt indicator
	#define UART_LSR_THRE	(0x00000020UL)	// Transmit-hold-register empty
	#define UART_LSR_TEMT	(0x00000040UL)	// Transmitter empty

//
// UART_SCCR(8)
// SmartCard Control Register
//
#define UART_SCCR	(nonPM_vaddr + REG_0020_SMART)// Smartcard Control Register
	#define UART_SCCR_MASK_CARDIN	(0x00000001UL)	// Smartcard card in interrupt mask
	#define UART_SCCR_MASK_CARDOUT	(0x00000002UL)	// Smartcard card out interrupt mask
	#define UART_SCCR_TX_BINV		(0x00000004UL)	// Smartcard Tx bit invert
	#define UART_SCCR_TX_BSWAP		(0x00000008UL)	// Smartcard Tx bit swap
	#define UART_SCCR_RST			(0x00000010UL)
	#define UART_SCCR_RX_BINV		(0x00000020UL)	// Smartcard Rx bit inverse
	#define UART_SCCR_RX_BSWAP		(0x00000040UL)	// Smartcard Rx bit swap

//
// UART_SCSR(9)
// Smartcard Status Register
//
#define UART_SCSR	(nonPM_vaddr + REG_0024_SMART)// Smartcard Status Register
	#define UART_SCSR_CLK			(0x00000001UL)	// Smartcard clock out
	#define UART_SCSR_INT_CARDIN	(0x00000002UL)	// Smartcard card in interrupt
	#define UART_SCSR_INT_CARDOUT	(0x00000004UL)	// Smartcard card out interrupt
	#define UART_SCSR_DETECT		(0x00000008UL)	// Smartcard detection status
	#define UART_SCSR_BGT_MASK		(0x00000020UL)	// Smartcard BGT interrupt mask
	#define UART_SCSR_CWT_MASK		(0x00000040UL)	// Smartcard CWT interrupt mask
	#define UART_SCSR_CGT_MASK		(0x00000080UL)	// Smartcard CGT interrupt mask

//
// UART_SCFC(a)
// Smartcard Fifo Register
//
#define UART_SCFC	(nonPM_vaddr + REG_0028_SMART)// Smartcard Fifo Count Register
	#define UART_SCFC_RST_TO_IO_EDGE_DET_FAIL	(0x00000040UL)
	#define UART_SCFC_RST_TO_IO_EDGE_DET_EN		(0x00000080UL)

//
// UART_SCFR(c)
// Smartcard Fifo Read Delay Register
//
#define UART_SCFR	(nonPM_vaddr + REG_0030_SMART)// Smartcard Fifo Read Delay Register
	#define UART_SCFR_DELAY_NONE	(0x00000000UL)
	#define UART_SCFR_DELAY_ONE		(0x00000001UL)
	#define UART_SCFR_DELAY_TWO		(0x00000002UL)
	#define UART_SCFR_DELAY_THREE	(0x00000003UL)
	#define UART_SCFR_DELAY_MASK	(0x00000003UL)
	#define UART_SCFR_V_HIGH		(0x00000004UL)
	#define UART_SCFR_V_ENABLE		(0x00000008UL)

//
// UART_SCMR(d)
// SMart Mode Register
//
#define UART_SCMR	(nonPM_vaddr + REG_0034_SMART)// Smartcard Mode Register
	#define UART_SCMR_RETRY_MASK	(0x0000001FUL)
	#define UART_SCMR_SMARTCARD		(0x00000020UL)
	#define UART_SCMR_PARITYCHK		(0x00000040UL)
	#define UART_SCMR_FLOWCTRL		(0x00000080UL)

#define UART_SCCGT		(nonPM_vaddr + REG_0038_SMART)
#define UART_SCCWT_L	(nonPM_vaddr + REG_0040_SMART)// Smartcard char waiting time - LoByte
#define UART_SCCWT_H	(nonPM_vaddr + REG_0044_SMART)// Smartcard char waiting time - HiByte
#define UART_SCBGT		(nonPM_vaddr + REG_0048_SMART)// Smartcard block guard time
#define UART_SCBWT_0	(nonPM_vaddr + REG_0050_SMART)// Smartcard block waiting time bit 07:00
#define UART_SCBWT_1	(nonPM_vaddr + REG_0054_SMART)// Smartcard block waiting time bit 15:08
#define UART_SCBWT_2	(nonPM_vaddr + REG_0058_SMART)// Smartcard block waiting time bit 23:16
#define UART_SCBWT_3	(nonPM_vaddr + REG_005C_SMART)// Smartcard block waiting time bit 31:24
//
// UART_CTRL2(0x18)
//
#define UART_CTRL2	(nonPM_vaddr + REG_0060_SMART)// CGT/CWT/BGT/BWT int mask
	#define UART_CTRL2_FLAG_CLEAR			(0x00000001UL)
	#define UART_CTRL2_CGWT_MASK			(0x00000002UL)
	#define UART_CTRL2_BGWT_MASK			(0x00000004UL)
	#define UART_CTRL2_TX_LEVEL_MASK		(0x00000008UL)
	#define UART_CTRL2_TX_LEVEL				(0x00000030UL)
	#define UART_CTRL2_TX_LEVEL_5_TO_4		(0x00000000UL)
	#define UART_CTRL2_TX_LEVEL_9_TO_8		(0x00000010UL)
	#define UART_CTRL2_TX_LEVEL_17_TO_16	(0x00000020UL)
	#define UART_CTRL2_TX_LEVEL_31_TO_30	(0x00000030UL)
	#define UART_CTRL2_REC_PE_EN			(0x00000040UL)

//
// UART_GWT_INT(0x19)
//
#define UART_GWT_INT	(nonPM_vaddr + REG_0064_SMART)// CGT/CWT/BGT/BWT int status
	#define UART_GWT_BWT_FAIL		(0x00000001UL)
	#define UART_GWT_BGT_FAIL		(0x00000002UL)
	#define UART_GWT_CWT_RX_FAIL	(0x00000004UL)
	#define UART_GWT_CWT_TX_FAIL	(0x00000008UL)
	#define UART_GWT_CGT_RX_FAIL	(0x00000010UL)
	#define UART_GWT_CGT_TX_FAIL	(0x00000020UL)
	#define UART_GWT_TX_LEVEL_INT	(0x00000040UL)

//
// UART_TX_FIFO_COUNT(0x1A)
//
#define UART_TX_FIFO_COUNT	(nonPM_vaddr + REG_0068_SMART)// TX FIFO count
	#define UART_TX_FIFO_CWT_RX_FAIL_DET_EN (0x00000040UL)

#define UART_DEACTIVE_RST	(nonPM_vaddr + REG_006C_SMART)// Set the active time for RST pin
#define UART_DEACTIVE_CLK	(nonPM_vaddr + REG_0070_SMART)// Set the active time for CLK pin
#define UART_DEACTIVE_IO	(nonPM_vaddr + REG_0074_SMART)// Set the active time for IO pin
#define UART_DEACTIVE_VCC	(nonPM_vaddr + REG_0078_SMART)// Set the active time for VCC pin

//
// UART_CTRL3(0x1F)
//
#define UART_CTRL3	(nonPM_vaddr + REG_007C_SMART)
	//#define UART_NDS_FLC_BLK_DATA_CTRL	  (0x00000080UL)
	#define UART_AC_PWR_OFF_CTL		(0x00000040UL)
	#define UART_TRAN_ETU_CTL		(0x00000020UL)
	#define UART_INVERT_CD			(0x00000010UL)
	#define UART_AC_PWR_OFF_MASK	(0x00000008UL)
	#define UART_VCC_OFF_POL		(0x00000004UL)
	#define UART_DEACTIVE_SEQ_EN	(0x00000001UL)

#define UART_ACTIVE_RST	(nonPM_vaddr + REG_0080_SMART)// Set the active time for RST pin
#define UART_ACTIVE_CLK	(nonPM_vaddr + REG_0084_SMART)// Set the active time for CLK pin
#define UART_ACTIVE_IO	(nonPM_vaddr + REG_0088_SMART)// Set the active time for IO pin
#define UART_ACTIVE_VCC	(nonPM_vaddr + REG_008C_SMART)// Set the active time for VCC pin


//
// UART_CTRL5(0x25)
//
#define UART_CTRL5	(nonPM_vaddr + REG_0094_SMART)// control5 reg
	#define UART_CTRL5_PAD_MASK			(0x00000010UL)
	#define UART_CTRL5_PAD_RELEASE		(0x00000020UL)
	#define UART_CTRL5_ACTIVE_SEQ_EN	(0x00000001UL)	// Smartcard active sequence enable
	#define UART_CTRL5_AUTO_SEQ_CTRL	(0x00000008UL)	// Smartcard auto sequence enable



//
// UART_CTRL6(0x27)
//
#define UART_CTRL6						(nonPM_vaddr + REG_009C_SMART)
	#define UART_CTRL6_SHORT_CIRCUIT_AUTO_DEACTIVE	(0x00000080UL)
	#define UART_CTRL6_SHORT_CIRCUIT_MASK			(0x00000040UL)
	#define UART_CTRL6_SHORT_CIRCUIT_DISABLE		(0x00000020UL)
	#define UART_CTRL6_GCR_SMC_3P3_GPIO_EN			(0x00000010UL)
	#define UART_CTRL6_LDO_3V_5V_EN					(0x00000008UL)
	#define UART_CTRL6_PD_SMC_LDO					(0x00000004UL)


//
// UART_CTRL7(0x2c)
//
#define UART_CTRL7	(nonPM_vaddr + REG_00B0_SMART)// control7 reg
	#define UART_CTRL7_ACT_PULLUP_EN			(0x00000080UL)
	#define UART_CTRL7_PD_SMC_PAD				(0x00000040UL)
	#define UART_CTRL7_VL_SEL_DEL_CNT_SEL		(0x00000030UL)
	#define UART_CTRL7_GCR_SMC_VL_SEL_HW_MASK	(0x00000008UL)	// vsel HW mask
	#define UART_CTRL7_GCR_SMC_VL_SEL_SW		(0x00000004UL)	// vsel SW
	#define UART_CTRL7_LDO_5V_EN_SW				(0x00000002UL)	// LDO 5V enable
	#define UART_CTRL7_LDO_5V_DEBUG_MODE		(0x00000001UL)	// LDO debug mode

//
// UART_CTRL8 (0x2e)
//
#define UART_CTRL8	(nonPM_vaddr + REG_00B8_SMART)// control8 reg
	#define UART_CTRL8_ANALOG_CNT_DONT_CARE_CARD_RESET	(0x00000008UL)
	#define UART_CTRL8_ANALOG_DC_CARD_RESET				(0x00000004UL)
	#define UART_CTRL8_CWT_EXT_EN						(0x00000002UL)
	#define UART_CTRL8_OCP_INTRF_MASK					(0x00000001UL)

//
// UART_CTRL9 (0x2f)
//
#define UART_CTRL9	(nonPM_vaddr + REG_00BC_SMART)// control9 reg
	#define UART_CTRL9_CWT_EXT	(0x000000FFUL)	// Extend CWT

//
// UART_CTRL14 (0x34)
//
#define UART_CTRL14	(nonPM_vaddr + REG_00D0_SMART)// control14 reg
	#define UART_CTRL14_PD_SD_LDO			(0x00000002UL)//
	#define UART_CTRL14_GCR_SMC_LDO_1P8V_EN	(0x00000001UL)// 1.8v ldo enable

#define UART_ANALOG_STATUS2				// analog_status2 reg

//
// UART_LSR_CLR (0x36)
//
#define UART_LSR_CLR	(nonPM_vaddr + REG_00D8_SMART)// lsr clear reg
	#define UART_LSR_CLR_FLAG		(0x00000001UL)	// clear lsr reg
	#define UART_LSR_CLR_FLAG_OPT	(0x00000002UL)	// clear lsr reg

#define UART_RST_TO_IO_0	(nonPM_vaddr + REG_00DC_SMART)// rst to io timeout value[7:0]
#define UART_RST_TO_IO_1	(nonPM_vaddr + REG_00E0_SMART)// rst to io timeout value[15:8]
#define UART_RST_TO_IO_2	(nonPM_vaddr + REG_00E4_SMART)// rst to io timeout value[23:16]
#define UART_RST_TO_IO_3	(nonPM_vaddr + REG_00E8_SMART)// rst to io timeout value[31:24]

//
// UART_CTRL15 (0x3b)
//
#define UART_CTRL15		(nonPM_vaddr + REG_00EC_SMART)// control15 reg
	#define UART_CTRL15_RST_TO_IO_EDGE_DET_FAIL_MASK	(0x00000001UL)

#define REG_TOP_CKG_SM_CA			(nonPM_vaddr + REG_01B0_SMART)
#define TOP_CKG_SM_CA0_DIS				BIT(8)
#define TOP_CKG_SM_CA0_CLK_MASK		(0x0c00) //BMASK(11:10)

#define REG_TOP_3V_5V_SELECT		(nonPM_vaddr + REG_01BC_SMART)

//(REG_CLK_GEN_BASE + (0x0000006dUL << 1))
#define REG_TOP_CKG_SM_CACLK_EXT		(nonPM_vaddr + REG_01B4_SMART)
//(REG_CLK_GEN_BASE + (0x0000006dUL << 1))
#define REG_TOP_CKG_SM_CACLK_M			(nonPM_vaddr + REG_01B4_SMART)
//(REG_CLK_GEN_BASE + (0x0000006eUL << 1))
#define REG_TOP_CKG_SM_CACLK_N			(nonPM_vaddr + REG_01B8_SMART)

#endif				// _REG_SC_H_
